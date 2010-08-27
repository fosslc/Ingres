/*
** Copyright (c) 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <cs.h>
#include <me.h>
#include <bt.h>
#include <ci.h>
#include <cv.h>
#include <mh.h>
#include <qu.h>
#include <st.h>
#include <tr.h>
#include <tm.h>
#include <tmtz.h>
#include <cm.h>
#include <er.h>
#include <pc.h>
#include <iicommon.h>
#include <dbdbms.h>
#include <gca.h>
#include <ddb.h>
#include <dmf.h>
#include <dmtcb.h>
#include <adf.h>
#include <ade.h>
#include <adfops.h>
#include <adudate.h>
#include <ulf.h>
#include <ulm.h>
#include <qsf.h>
#include <qefrcb.h>
#include <rdf.h>
#include <sxf.h>
#include <qefmain.h>
#include <qefcb.h>
#include <qefnode.h>
#include <psfparse.h>
#include <ex.h>
#include <psfindep.h>
#include <pshparse.h>
#include <usererror.h>
#include <dudbms.h>
#include <psftrmwh.h>
#include <psqcvtdw.h>
#include <psqmonth.h>
#include <psttprpnd.h>
#include <psyaudit.h>
#include <uld.h>
#include <cui.h>

/**
**
**  Name: pslssflat.c - Flatten subselects
**
**  Description:
**      This file contains the code to do the primary flattening of 
**	scalar sub-queries.
**
**          psl_ss_flatten - flattens one query.
**
**
**  History:
**	01-Apr-2010 (kiria01) b123535
**	    Pulled out flattening code from parser grammar file.
**	    Addressed joinid processing and nested uncorrelations.
**	    Improved handling of predicate scope and extended cover
**	    to flatten the WHERE clause singleton subselects. This
**	    substantially moves the cardinality checks from the QEN
**	    nodes using the SINGLETON aggregate.
**	07-Apr-2010 (kiria01) b123535
**	    Fixed a problem with DISTINCT singletons. Previously these
**	    were transformed from:
**		SELECT DISTINCT col FROM ...
**	    to:
**		SELECT gbys, SINGLETON(DISTINCT col) FROM ... GROUP BY gbys
**	    in the derived table. This should work except it confuses
**	    QEF and defeats the intent which is to remove the duplicates
**	    that would trip the SINGLETON in the first place. The transform
**	    now maps to:
**		SELECT DISTINCT gbys, col FROM ... GROUP BY gbys
**	    For this to work, the nesting join is now set to INNER if the
**	    subselect is in a WHERE clause and not using a COUNT aggregate.
**	    This has required a finer scope tracking of WHERE clause scope.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	09-Apr-2010 (kiria01) b123555
**	    Propagate the from list bitmaps from the elided subselects to
**	    their parents.
*/


/*
** Global Variable Definitions
*/

enum _FLAT_CAP {
    FLAT_CAP_OFF =	0x00, /* disables SS flattening */
    FLAT_CAP_NOMETA =	0x01, /* leaves WHERE clause to OPF*/
    FLAT_CAP_01 =	0x02, /* handles WHERE clause except ANY, ALL, IN, EXISTS *
    /*experimental*/
    FLAT_CAP_ANY =	0x04, /* handles WHERE clause except ALL, IN, EXISTS */
    FLAT_CAP_UNCORR =	0x08, /* fold uncorellated as CP */
};


u_i4 Psf_ssflatten = FLAT_CAP_01; /* MO:exp.psf.pst.ssflatten */
static const u_i4 opmeta_unsupp[] = {
    /*PST_NOMETA*/	0,
    /*PST_CARD_01R*/	FLAT_CAP_NOMETA,
    /*PST_CARD_ANY*/	FLAT_CAP_NOMETA | FLAT_CAP_01,
    /*PST_CARD_ALL*/	FLAT_CAP_NOMETA | FLAT_CAP_01 | FLAT_CAP_ANY,
    /*PST_CARD_01L*/	FLAT_CAP_NOMETA
};
/*
**  Forward and/or External function references.
*/

static DB_STATUS
psl_add_resdom_var(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	*var);

static DB_STATUS
psl_add_resdom_agh(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	**aghp);

static PST_QNODE**
psl_lu_resdom_var(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	*var);

static VOID
psl_retarget_vars(
	PSS_SESBLK	*cb,
      PST_QNODE		*tree,
      PST_QNODE		*dtroot,
      PSS_RNGTAB	*resrange);

static DB_STATUS
psl_float_factors(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PST_QNODE	*from,
	PST_QNODE	*to);

static DB_STATUS
psl_project_vars (
	PSS_SESBLK	*cb,
	PST_QNODE	*srctree,
	PST_QNODE	*desttree);

static DB_STATUS
psl_project_corr_vars (
	PSS_SESBLK	*cb,
	PST_QNODE	*srctree,
	PST_QNODE	*desttree);


/*
** Name: psl_flatten1 - flattens out singleton sub-selects from 1 tree.
**
** Description:
**	Does a flattened, depth first traversal of the parse tree looking
**	for correllations (bool factors) and sub-queries etc and where
**	appropriate, quietly folds them into tehe main query fixing up
**	the tree for opa.
**
** Input:
**	nodep		Address of pointer to global tree to flatten
**	yyvarsp		Address of PSS_YYVARS context
**	cb		PSS_SESBLK Parser session block
**	psq_cb		Query parse control block
**	stk		Shared stack context
**
** Output:
**	nodep		The tree may be updated.
**
** Side effects:
**	Also flattens any derived tables.
**
** History:
**	02-Nov-2009 (kiria01) 
**	    Created to flatten 1 full tree at a time.
**	22-Apr-2010 (kiria01) b123618
**	    Aggregate restrictions were being moved from the AGHEAD to
**	    derived table root regardless of need. However, if the
**	    happened to be a join between tables referenced in the AGHEAD
**	    then the aggregate could see more rows than it should. The
**	    AGHEAD qualifier is now treated with psl_float_vars to make
**	    sure that only the required floating happens.
**	25-May-2010 (kschendel) b123796
**	    A query such as:
**	    select ... where a1 < (select a1 from t1 where a1='' and a2=''
**	      and a3='' and a4='' and ...)
**	    loses all but the last two conditions out of the subselect.
**	    The cause was premature variable initialization; psl-float-factors
**	    will rearrange the qtree fragment with the ANDs, and the
**	    rearranged qtree is what needs to be attached to the generated
**	    aghead, not a pointer to the original qtree.
**	29-Jun-2010 (kiria01) b123988
**	    Correct the maintenance of qual_depth with PST_ROOT otherwise
**	    subsequent PST_ROOT nodes from unions appear at ever deeper parse
**	    depths resulting in setting bits out of range in in_WHERE and
**	    has_COUNT.
**	19-Jun-2010 (kiria01) b123951
**	    Ensure we flatten the WITH-element trees too.
*/

static DB_STATUS
psl_flatten1(
	PST_QNODE	**nodep,
	PSS_YYVARS	*yyvarsp,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PST_STK		*stk)
{
    ADI_OPINFO		opinfo;
    i4			whlist_count = 0;
    i4			qual_depth = cb->pss_qualdepth;
    bool		descend = TRUE;
    bool		correl = FALSE;
    PST_QNODE		*subsel = NULL;
    i4			err_code;
    DB_STATUS		status = E_DB_OK;
    PST_STK		norm_stk;
    u_char		in_WHERE[MAX_NESTING / BITS_IN(u_char) + 1];
    u_char		has_COUNT[MAX_NESTING / BITS_IN(u_char) + 1];

    MEfill(sizeof(in_WHERE), 0, (PTR)in_WHERE);
    MEfill(sizeof(has_COUNT), 0, (PTR)has_COUNT);

    if (nodep && *nodep)
    while (nodep)
    {
	PST_QNODE	*node = *nodep;

	/* A sub-query in a case selector is ok but elsewhere and
	** we leave the subquery alone. We will maintain a counted
	** flag when we push a PST_WHLIST node which would indicate
	** this state,
	** The other complexity here is that we don't want to fold
	** the external joining sub-queries such as: IN, =ANY etc.
	** or the EXISTS operator - these we will leave to OPF.*/
	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && (node->pst_left || node->pst_right))
	{
	    i4 i;
	    /* Increment depth counters for children */
	    switch (node->pst_sym.pst_type)
	    {
	    case PST_WHLIST: whlist_count++; break;
	    case PST_ROOT:
	    case PST_SUBSEL:
		subsel = node;
		correl = FALSE;
		/* Temporarily reset the validity flag and have it shadow
		** the correl setting as determined with BOPs. This may
		** show a discrepancy due to apparent correlations that
		** ought to be higher up the tree. */
		subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &=
					~PST_1CORR_VAR_FLAG_VALID;

		qual_depth++;
		BTclear(qual_depth, (PTR)in_WHERE);
		BTclear(qual_depth, (PTR)has_COUNT);
		/* Include any unions into the mix */
		if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    pst_push_item(stk,
			(PTR)&node->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    /* Mark that the top node needs descending */
		    pst_push_item(stk, (PTR)PST_DESCEND_MARK);
		}

		break;
	    }
	    /* Delay node evaluation */
	    pst_push_item(stk, (PTR)nodep);
	    if (node->pst_left)
	    {
		if (node->pst_right)
		{
		    /* Delay RHS */
		    pst_push_item(stk, (PTR)&node->pst_right);
		    if (node->pst_right->pst_right ||
				node->pst_right->pst_left)
			/* Mark that the top node needs descending */
			pst_push_item(stk, (PTR)PST_DESCEND_MARK);
		}
		nodep = &node->pst_left;
		continue;
	    }
	    else if (node->pst_right)
	    {
		nodep = &node->pst_right;
		continue;
	    }
	}

	/*** Depth first traverse ***/
	switch (node->pst_sym.pst_type)
	{
	case PST_AOP:
	    if (node->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNTAL_OP ||
		    node->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNT_OP)
		BTset(qual_depth, (PTR)has_COUNT);
	    /*FALLTHROUGH*/
	case PST_BOP:
	case PST_UOP:
	case PST_COP:
	case PST_MOP:
	    {
		PST_QNODE *rt_node = pst_antecedant_by_4types(
					stk, NULL, PST_SUBSEL, PST_AGHEAD, PST_RESDOM, PST_ROOT);
		PST_QNODE *l = NULL, *r = NULL, *parent = NULL;
		bool in_agg = FALSE; /* just local agg */
		if (status = adi_op_info(cb->pss_adfcb,
					node->pst_sym.pst_value.pst_s_op.pst_opno, &opinfo))
		    break;
		/* Having a comparison operator, we are in one of three cases:
		**  1 - implicit join info for a containing SS. 
		**  2 - join with a SSs target. Presently folded in OPF so do nothing
		**  3 - boolean in SS tergetlist. Ignore.
		**
		** If no containing PST_SUBSEL, PST_AGHEAD, PST_RESDOM, PST_ROOT then ignore,
		** if ROOT found first we are in a case 2,
		** if RESDOM found first we are in case 3.
		**
		** We do the actual drop of the non-compare nodes in a bit.
		*/
		if (!rt_node ||
		    rt_node->pst_sym.pst_type == PST_ROOT ||
		    rt_node->pst_sym.pst_type == PST_RESDOM)
		    break;
		
		/* If containing subsel's parent OP is a comparison BOP or EXISTS UOP then
		** maybe leave sub-query intact for OPF */
		in_agg = rt_node->pst_sym.pst_type == PST_AGHEAD;
		if (subsel &&
		    subsel->pst_sym.pst_type == PST_SUBSEL &&
		    (parent = pst_parent_node(stk, subsel)))
		{
		    if ((parent->pst_sym.pst_type == PST_BOP ||
			 parent->pst_sym.pst_type == PST_UOP) &&
			(in_agg ||
			 (Psf_ssflatten &
			    opmeta_unsupp[parent->pst_sym.pst_value.pst_s_op.pst_opmeta])))
		    {
	    		/* We leave these for folding later in OPF */
			if (!correl &&
			    (subsel->pst_sym.pst_value.pst_s_root.pst_mask1&
					PST_2CORR_VARS_FOUND))
			{
			    /* Will need to trust flags */
			    correl = 1;
			    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
							PST_1CORR_VAR_FLAG_VALID;
			}
			break;
		    }
		}
		rt_node->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_8FLATN_SUBSEL;

		/* Only interested in comparisons */
		if (opinfo.adi_optype != ADI_COMPARISON)
		    break;

		if (node->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN ||
		    (Psf_ssflatten &
			opmeta_unsupp[node->pst_sym.pst_value.pst_s_op.pst_opmeta]))
		    break;

		if (correl || (Psf_ssflatten & FLAT_CAP_UNCORR)) /* Don't set join id on uncorr */
		    node->pst_sym.pst_value.pst_s_op.pst_joinid =
			subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid;


		/* Only type 1 left: We check for VAR nodes either side
		** looking for inner-outer reference level mismatches. If found we
		** identify the join ends and augment the join info with an outer join.
		** If we are in a sub-query then having identified the outer join ops,
		** we will flag the SUBSEL node for folding.
		** Also, if we are not in an aggregate qualifier, set the pst_opmeta
		** field to trap cardinality errors. (In agg card count will be 1). */
		if (psl_find_node(node->pst_left, &l, PST_VAR) &&
		    psl_find_node(node->pst_right, &r, PST_VAR))
		{
		    /* Special processing for when post-processing an enclosing scope
		    ** i.e. we have a PST_SUBSEL above somewhere. */
		    i4 l_vno = l->pst_sym.pst_value.pst_s_var.pst_vno;
		    i4 l_hidepth = yyvarsp->rng_vars[l_vno]->pss_rgparent;
		    i4 r_vno = r->pst_sym.pst_value.pst_s_var.pst_vno;
		    i4 r_hidepth = yyvarsp->rng_vars[r_vno]->pss_rgparent;
		    i4 l_lodepth = l_hidepth;
		    i4 r_lodepth = r_hidepth;
		    i4 inner, outer, depth, card;
		    PST_J_MASK masks[2]; /* left, right */
		    PSS_1JOIN *join_info;
		    DB_JNTYPE jtype;
		    bool in_qual = FALSE; /* just local where */ 

		    /* We will now decend LHS & RHS collecting info about the
		    ** VAR nodes in each. Usually there will be 1 in each. */
		    MEfill(sizeof(masks), 0, (PTR)masks);
		    BTset(l_vno, (char *)(masks[0].pst_j_mask));
		    BTset(r_vno, (char *)(masks[1].pst_j_mask));
		    while (psl_find_node(node->pst_left, &l, PST_VAR))
		    {
			i4 vno = l->pst_sym.pst_value.pst_s_var.pst_vno;
			i4 depth = yyvarsp->rng_vars[vno]->pss_rgparent;
			BTset(vno, (char *)(masks[0].pst_j_mask));
			l_hidepth = max(l_hidepth, depth);
			l_lodepth = min(l_lodepth, depth);
		    }
		    while (psl_find_node(node->pst_right, &r, PST_VAR))
		    {
			i4 vno = r->pst_sym.pst_value.pst_s_var.pst_vno;
			i4 depth = yyvarsp->rng_vars[vno]->pss_rgparent;
			BTset(vno, (char *)(masks[1].pst_j_mask));
			r_hidepth = max(r_hidepth, depth);
			r_lodepth = min(r_lodepth, depth);
		    }
		    /* Usual case is easy - one in each but if there are
		    ** more they may be inconsistant for an outer join */
		    if (l_hidepth < r_lodepth)
		    {
			/* expr(sup-vars) .cf. expr(inf-vars) */
			depth = l_lodepth;
			outer = 0; /* is left */
			inner = 1; /* is right */
			jtype = DB_LEFT_JOIN;
			card = PST_CARD_01R;
		    }
		    else if (r_hidepth < l_lodepth)
		    {
			/* expr(inf-var) .cf. expr(sup-var) */
			depth = r_lodepth;
			outer = 1; /* is right */
			inner = 0; /* is left */
			jtype = DB_RIGHT_JOIN;
			card = PST_CARD_01L;
		    }
		    else
			break;

		    /* This can't be uncorrelated after all */
		    if (!correl && subsel)
		    {
			if (~Psf_ssflatten & FLAT_CAP_UNCORR)
			    node->pst_sym.pst_value.pst_s_op.pst_joinid =
					subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid;
			correl = TRUE;
			subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
							PST_1CORR_VAR_FLAG_VALID;
		    }

		    if (!in_agg)
		    {
			node->pst_sym.pst_value.pst_s_op.pst_opmeta = card;
			if (~rt_node->pst_sym.pst_value.pst_s_root.pst_mask1
				& PST_9SINGLETON)
			    BTor(sizeof(PST_J_MASK), (char *)(masks[outer].pst_j_mask),
				(char *)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm);
		    }
		    if (subsel && subsel->pst_sym.pst_type == PST_SUBSEL)
		    {
			/* If current subselect (qual_depth-1) is not in a where
			** clause or the current subselect has a COUNT aggregate,
			** use an outer join rather than an inner */
			if (!BTtest(qual_depth-1, (PTR)in_WHERE) || 
				BTtest(qual_depth, (PTR)has_COUNT))
			{
			    cb->pss_stmt_flags |= PSS_OUTER_OJ;
			    psl_set_jrel(&masks[inner], &masks[outer],
				    node->pst_sym.pst_value.pst_s_op.pst_joinid,
				    yyvarsp->rng_vars, jtype);
			}
			else
			{
			    psl_set_jrel(&masks[inner], NULL,
				    node->pst_sym.pst_value.pst_s_op.pst_joinid,
				    yyvarsp->rng_vars, DB_INNER_JOIN);
			}
		    }
		}
	    }
	    break;
	case PST_AND:
	case PST_OR:
	    {
		PST_QNODE *rt_node;
		if (node->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN &&
		    BTtest(qual_depth, (PTR)in_WHERE) &&
		    (rt_node = pst_antecedant_by_3types(
				stk, NULL, PST_SUBSEL, PST_AGHEAD, PST_ROOT)))
		{
		    PST_J_ID joinid = rt_node->pst_sym.pst_value.pst_s_root.pst_ss_joinid;
		    PST_QNODE *l = node->pst_left;
		    PST_QNODE *r = node->pst_right;
		    i4 op;
		    if (l && ((op = l->pst_sym.pst_type) == PST_BOP ||
			op == PST_UOP || op == PST_MOP || op == PST_COP ||
			op == PST_AOP || op == PST_AND || op == PST_OR))
		    {
			if (r && ((op = r->pst_sym.pst_type) == PST_BOP ||
			    op == PST_UOP || op == PST_MOP || op == PST_COP ||
			    op == PST_AOP || op == PST_AND || op == PST_OR))
			{
			    if (l->pst_sym.pst_value.pst_s_op.pst_joinid ==
				    r->pst_sym.pst_value.pst_s_op.pst_joinid)
				joinid = l->pst_sym.pst_value.pst_s_op.pst_joinid;
			    else
				break; /* Leave as NOJOIN */
			}
			else
			    joinid = l->pst_sym.pst_value.pst_s_op.pst_joinid;
		    }
		    node->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
		}
	    }
	    break;
	case PST_SUBSEL:
	    /*
	    ** In this context we re-model the SUBSEL from LHS to RHS
	    ** with two similar transforms depending on nature of subsel:
	    ** case 1 - Aggregate header (card=0/1) uncorrellated
	    **
	    **           ROOT                            ROOT
	    **           /   \                           /   \
	    **         RSD'  qual                     RSD'   QEND
	    **        /   \                          /   \
	    **    [RSDs]   SUBSEL                [RSDs]   AGH
	    **     /       /                      /      /   \
	    **  TRE      RSD                   TRE     AOP   qual
	    **          /   \                          /
	    **       TRE     AGH                     exp
	    **              /   \
	    **           AOP     qual
	    **          /
	    **       exp
	    **
	    ** case 2 - Aggregate header (card=0/1) correlated
	    **		Pull out sub-select into derived table for the aggregate
	    **		vRSDs are grouped on the inner table columns used in the join
	    **
	    **           ROOT                      ROOT           drROOT
	    **           /   \                     /   \            /
	    **         RSD'  QEND                RSD'   qual      RSD''
	    **        /   \                    /   \             /  \
	    **    [RSDs]   SUBSEL          [RSDs]   VAR     [vRSDs]  AGH
	    **     /       /                /                 /     /
	    **  TRE      RSD             TRE               TRE    BYH
	    **          /   \                                    /   \
	    **       TRE     AGH                            [vRSDs]   AOP
	    **              /   \                             /      /
	    **           AOP     qual                      TRE    exp
	    **          /
	    **       exp
	    **
	    ** case 3 - non-Aggregate, safe if card <= 1 but need a check.
	    **
	    **           ROOT                         ROOT
	    **           /   \                       /    \
	    **         RSD'   qual1                 /      AND
	    **        /   \                        /      /   \
	    **    [RSDs]   SUBSEL                RSD'  qual1  qual2
	    **     /       /   \                /   \
	    **  TRE      RSD    qual2       [RSDs] [tree]  
	    **          /   \                /
	    **       TRE     [tree]       TRE
	    **
	    ** NOTE that the qual2 move will involve converting the subsel
	    ** join to an OUTER one if not already.
	    **
	    ** case 4 - non-Aggregate singleton - change to aggregate on singleton()
	    **		Pull out sub-select into derived table for the aggregate
	    **		vRSDs are grouped on the inner table columns used in the join
	    **
	    **           ROOT                      ROOT           drROOT
	    **           /   \                     /   \            /
	    **         RSD'   qual1             RSD'   qual1     RSD''
	    **        /   \                    /   \             /  \
	    **    [RSDs]   SUBSEL          [RSDs]   VAR     [vRSDs]  AGH
	    **     /       /   \            /                 /     /
	    **  TRE      RSD    qual2    TRE               TRE    BYH
	    **          /   \                                    /   \
	    **       TRE     [tree]                         [vRSDs]   AOP
	    **                                                /      /
	    **                                             TRE    exp
	    */
	    for (;;) /* To break out from */
	    {
		PST_QNODE *rt_node = pst_antecedant_by_3types(
					stk, NULL, PST_SUBSEL, PST_AGHEAD, PST_ROOT);
		PST_QNODE *psubsel = pst_antecedant_by_2types(
					stk, NULL, PST_SUBSEL, PST_ROOT);
		PST_QNODE *parent = pst_parent_node(stk, NULL);
		PST_QNODE *residue = subsel->pst_left;
		PST_QNODE **agheadp = NULL;
		PST_J_ID joinid = PST_NOJOIN;
		bool has_DISTINCT = subsel->pst_sym.pst_value.pst_s_root.pst_dups == PST_NODUPS;

		PST_STK_INIT(norm_stk, cb);

		if (BTtest(qual_depth--, (PTR)in_WHERE) &&
		    parent)
		{
		    if ((parent->pst_sym.pst_type == PST_BOP ||
			 parent->pst_sym.pst_type == PST_UOP) &&
			(Psf_ssflatten &
			    opmeta_unsupp[parent->pst_sym.pst_value.pst_s_op.pst_opmeta]))
		    {
	    		/* We leave these for folding later in OPF */
			break;
		    }
		}

		/* If not flattening - don't try anything
		** If a sub-query is not a simple scalar and either
		**  in a where list context or not marked for folding */

		if (subsel->pst_sym.pst_value.pst_s_root.pst_tvrc != 0 && (
			whlist_count ||
			correl &&
			(subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &
				PST_8FLATN_SUBSEL) == 0))
		    break;

		if ((Psf_ssflatten & FLAT_CAP_UNCORR) && !correl &&
			(joinid = subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid) &&
			subsel->pst_sym.pst_value.pst_s_root.pst_tvrc &&
			psubsel->pst_sym.pst_value.pst_s_root.pst_tvrc)
		{
		    i4 i;
		    /* We have a subselect but if the qualifier didn't mark the
		    ** pss_inner_rel/pss_outer_rels then we have a problem as the
		    ** qualifier doesn't actually join the relations. The join can
		    ** still be valid and in fact, uncorrellated but restricted vars
		    ** do need to be collected. However, as we don't have join columns
		    ** to identify the join we must use the tvrm lists */
		    for (i = 0; i < PST_NUMVARS; i++)
		    {
			PSS_RNGTAB *rngvar = &cb->pss_auxrng.pss_rngtab[i];
			if (rngvar->pss_used &&
			    rngvar->pss_rgno >= 0 &&
			    BTtest(joinid, (char*)&rngvar->pss_inner_rel))
			{
			    break;
			}
		    }
		    if (i >= PST_NUMVARS)
		    {
			cb->pss_stmt_flags |= PSS_OUTER_OJ;
			psl_set_jrel(&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm,
					&psubsel->pst_sym.pst_value.pst_s_root.pst_tvrm,
					joinid,
					yyvarsp->rng_vars,
					DB_LEFT_JOIN);
		    }
		}
		if ((subsel->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_9SINGLETON) &&
		    ((~cb->pss_ses_flag & PSS_NOCHK_SINGLETON_CARD) ||
			!rt_node))
		{
		    PST_QNODE **varp = NULL;

		    /* If we get through with a WHERE clause and we are singleton,
		    ** we won't be needing opmeta set unless we have a GROUP BY in an
		    ** uncorrelated aggregate. This latter case presents an awkward problem
		    ** and is caused by such as: (qp035)
		    **  SELECT (SELECT AGG(A) FROM T GROUP BY B) FROM ...
		    ** This will rarely be single valued due to the grouping and will usually
		    ** need a restricting HAVING clause. This really needs the SINGLETON aggr
		    ** applying but we already have an aggregate so we would need to project
		    ** this to:
		    **  SELECT (SELECT SINGLETON((SELECT AGG(A) FROM T GROUP BY B))) FROM ...
		    ** This will work but for the time being we leave this aspect to OPF. 
		    */
		    if (parent->pst_sym.pst_type == PST_BOP &&
			    (parent->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_01L ||
		    	     parent->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_01R) &&
			     !(cb->pss_distrib & DB_3_DDB_SESS) &&
			     (correl ||
			     (~subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &PST_3GROUP_BY)))
			 parent->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;

		    /* If in sub-select and there are VAR nodes not aggregated in the value
		    ** and the context is a singleton then insert a singleton operator on
		    ** the var node(s). We do this now before the tree has had aggregate
		    ** processing done (in psl_p_tlist). Note that if subselect is DISTINCT
		    ** and correlated, then the SINGLETON aggregate is not needed but the
		    ** group by handling will still be. */
		    if ((has_DISTINCT == FALSE || !correl) &&
			psl_find_nodep(&residue->pst_right, &varp, PST_VAR) &&
			BTtest((*varp)->pst_sym.pst_value.pst_s_var.pst_vno,
				(char *)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm) &&
				!(psl_find_node(residue->pst_right, NULL, PST_AGHEAD)))
		    {
			do
			{
			    /* No aggregation on result - we will insert an aggregation on
			    ** singleton operator to ensure cardinality error flagged in the
			    ** awkward cases that there are no join ops that can be tested. */
			    PST_QNODE *aop, *var_node;
			    var_node = *varp;
			    *varp = NULL;
			    {
				PST_OP_NODE opnode;
				DB_DATA_VALUE tmp_dv = var_node->pst_sym.pst_dataval;
				MEfill(sizeof(opnode), 0, &opnode);

				/* SINGLETON is strictly a nullable operator but could be
				** used in either a context where nullability would be handled
				** such as comparison, where it cannot be is like in assignment.
				** Thus if the result will be passed to a compare, we enable
				** nullability and allow the testing for such support as outer
				** joins but in other cases we may need the operator to force
				** the real error. */
				if (tmp_dv.db_datatype > 0 &&
					abs(parent->pst_sym.pst_dataval.db_datatype) == DB_BOO_TYPE)
				{
				    tmp_dv.db_datatype = -tmp_dv.db_datatype;
				    tmp_dv.db_length++;
				}
				opnode.pst_opno = ADI_SINGLETON_OP;

				if (!correl)
				{
				    /* Move the distinct into the operator & off the subsel */
				    if (subsel->pst_sym.pst_value.pst_s_root.pst_dups == PST_NODUPS)
					opnode.pst_distinct = PST_DISTINCT;
				    subsel->pst_sym.pst_value.pst_s_root.pst_dups = PST_ALLDUPS;
				}

				opnode.pst_retnull = TRUE;
				opnode.pst_opmeta = PST_NOMETA;
				opnode.pst_joinid = correl
					? subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid
					: PST_NOJOIN;
				if (status = pst_node(cb, &cb->pss_ostream, var_node, NULL, 
						PST_AOP, (PTR) &opnode, sizeof(opnode), 
						tmp_dv.db_datatype,
						tmp_dv.db_prec, 
						tmp_dv.db_length, 
						(DB_ANYTYPE *) NULL, &aop,
						&psq_cb->psq_error, PSS_JOINID_PRESET))
				    return status;
			    }
			    {
				PST_RT_NODE agnode;
				YYAGG_NODE_PTR *agglist_elem;
				PST_QNODE *agqual = NULL;
				if (status = pst_node(cb, &cb->pss_ostream, NULL, NULL, 
						    PST_QLEND, NULL, 0, 0, 0, 0, NULL,
						    &agqual, &psq_cb->psq_error, 0))
				    return status;

				MEfill(sizeof(agnode), 0, (char *)&agnode);
				agnode.pst_dups =
					subsel->pst_sym.pst_value.pst_s_root.pst_dups;
				agnode.pst_tvrc =
					subsel->pst_sym.pst_value.pst_s_root.pst_tvrc;
				agnode.pst_tvrm =
					subsel->pst_sym.pst_value.pst_s_root.pst_tvrm;
				agnode.pst_ss_joinid =
					subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid;

				if (status = pst_node(cb, &cb->pss_ostream, aop, agqual, 
						PST_AGHEAD, (PTR) &agnode, sizeof(agnode), 
						aop->pst_sym.pst_dataval.db_datatype,
						aop->pst_sym.pst_dataval.db_prec, 
						aop->pst_sym.pst_dataval.db_length, 
						(DB_ANYTYPE *) NULL, varp,
						&psq_cb->psq_error, 0))
				    return status;
			    }
			    /* Setup those things that are needed for the aggregate 
			    ** we have just added */
			    if (cb->pss_flattening_flags & PSS_AGHEAD)
				cb->pss_flattening_flags |= PSS_AGHEAD_MULTI;
			    cb->pss_flattening_flags |= PSS_AGHEAD;
     			    cb->pss_stmt_flags |= PSS_AGINTREE;	/* pss_agintree = TRUE */
			} while (psl_find_nodep(&residue->pst_right, &varp, PST_VAR) &&
			    BTtest((*varp)->pst_sym.pst_value.pst_s_var.pst_vno,
				(char *)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm));
		    }
		}
		if (psl_find_nodep(&residue->pst_right, &agheadp, PST_AGHEAD) ||
		    has_DISTINCT)
		{
		    if (correl || (Psf_ssflatten & FLAT_CAP_UNCORR))
		    {
			PST_QNODE *aghead = NULL;
			PST_QNODE *v;
			PSS_RNGTAB *resrange;
			PST_RSDM_NODE resdom;
			i4 n;

			if (!agheadp)
			{
			    /* We are here as has_DISTINCT was set.
			    ** We will setup aghead in this case to refer to the whole
			    ** singleton expression */
			    agheadp = &residue->pst_right;
			}
			else if (has_DISTINCT)
			{
			    /* An aggregate really present make distinct irrelevant*/
			    has_DISTINCT = FALSE;
			}

			/* Unlink residue from subsel. We will pull bits off residue and add
			** resdom (nodes pointing to each part) to the subsel column list */
			subsel->pst_left = residue->pst_left;
			do {
			    /* Put resdom for this aggr/expr at head of subsel list.
			    ** psl_add_resdom_var usually has to add VARs but will
			    ** also manage the arrangement for general expressions
			    ** but they must be normalised.
			    ** This also addresses the needs for has_DISTINCT as
			    ** the expression in that instance will be the var/expression
			    ** in question. The degenerative case of it being a simple
			    ** VAR will not matter as the group by is not needed as
			    ** the distinct will be active */
			    if (status = pst_qtree_norm(&norm_stk, agheadp, psq_cb))
				return status;
			    aghead = *agheadp;
			    if ((*agheadp)->pst_sym.pst_type == PST_AGHEAD)
			    {
				if (status = psl_add_resdom_agh(cb, &subsel->pst_left,
							agheadp))
				    return status;
			    }
			    else
			    {
				if (status = psl_add_resdom_var(cb, &subsel->pst_left,
							aghead))
				    return status;
			    }

			    if (!has_DISTINCT)
			    {
				if (status = psl_project_corr_vars(cb, aghead, subsel))
				    return status;

				/* Ensure we collect any byheads already present for grouping
				** and publish in derived list. These will also form the
				** consolidated group by shortly. */
				if (aghead->pst_left->pst_sym.pst_type == PST_BYHEAD)
				{
				    if (status = psl_project_vars(cb,
						    aghead->pst_left->pst_left, subsel))
					return status;
				}
			    }
			} while(psl_find_nodep(&residue->pst_right, &agheadp, PST_AGHEAD));

			/* Also ensure we collect references in the having list. */
			if (psl_project_corr_vars(cb, subsel, subsel))
			    return status;

			/* The resdom list for the subsel now has columns for the join
			** variables followed by the aggregates. These aggregates are ALSO
			** pointed at by their original parent node which will be under
			** residue (or be residue itself)
			** If the aggregate expression was complex then there could have been
			** more nodes in the sub-tree and there may be var nodes too - add these
			** taking care not to unnecessariliy add aggregated variables. */
			if (psl_project_vars(cb, residue, subsel))
			    return status;
			/* With the new resdom list count them and then we'll process
			** them to make sure that the names are unique. */
			for (v = subsel->pst_left, n=0;
				v && v->pst_sym.pst_type == PST_RESDOM; v=v->pst_left)
			    n++;
			for (v = subsel->pst_left;
				v && v->pst_sym.pst_type == PST_RESDOM; n--,v=v->pst_left)
			{
			    bool makename = TRUE;
			    u_i4 i;
			    v->pst_sym.pst_value.pst_s_rsdm.pst_rsno = n;
			    v->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = n;
			    for (i = 0; i < DB_ATT_MAXNAME; i++)
			    {
				if (v->pst_sym.pst_value.pst_s_rsdm.pst_rsname[i] != ' ')
				{
				    PST_QNODE *rsd;
				    makename = FALSE;
				    /* Scan the whole list each time - there could ba a col'n'
				    ** later that would need switching to its real col.*/
				    for (rsd = subsel->pst_left;
					    rsd && rsd->pst_sym.pst_type == PST_RESDOM; rsd=rsd->pst_left)
				    {
					if (rsd != v &&
					    !MEcmp((PTR)v->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
						    (PTR)rsd->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
						    DB_ATT_MAXNAME))
					{
					    makename = TRUE;
					    break;
					}
				    }
				    break;
				}
			    }
			    if (makename)
			    {
				char colname[DB_ATT_MAXNAME + 2];
				STprintf(colname, "col%d", n);
				MEmove(STlength(colname), (PTR)colname, ' ', 
					DB_ATT_MAXNAME, 
					(PTR)v->pst_sym.pst_value.pst_s_rsdm.pst_rsname); 
			    }
			}
			/* Change SUBSEL into root & update flags
			** We can now almost create the drtable.*/
			subsel->pst_sym.pst_type = PST_ROOT;
			/* Also detach the new root from the main tree keeping a
			** pointer to where the residue value will eventually be stitched
			** back in. */
			*nodep = NULL;
			{
			    char tmptbl[DB_TAB_MAXNAME + 2];
			    bool match = TRUE;
			    i4 i, n=1;
			    while(match)
			    {
				/* Create a unique name - diagnostic aid */
				STprintf(tmptbl,"dt%d", n);
				MEfill(DB_TAB_MAXNAME-STlen(tmptbl), ' ',
				    tmptbl+STlen(tmptbl));
				match = FALSE;
				for (i = 0; i < PST_NUMVARS; i++)
				{
				    PSS_RNGTAB *rngvar = &cb->pss_auxrng.pss_rngtab[i];
				    if (rngvar->pss_used &&
					rngvar->pss_rgno >= 0 &&
					!MEcmp(tmptbl, rngvar->pss_rgname, DB_TAB_MAXNAME))
				    {
					match = TRUE;
					n++;
					break;
				    }
				}
			    }
			    /* Now we have a locally unique name we can build the drtable.
			    ** With its vno, we can fixup the references to it */
			    if (status = psl_drngent(&cb->pss_auxrng, qual_depth, 
				    tmptbl, cb, &resrange, subsel, PST_DRTREE, &psq_cb->psq_error))
				return status;
			    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &=
					    ~(PST_2CORR_VARS_FOUND|PST_8FLATN_SUBSEL);
			    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
					    PST_3GROUP_BY;
			    cb->pss_ses_flag |= PSS_DERTAB_INQ;
			    resrange->pss_var_mask |= PSS_EXPLICIT_CORR_NAME;
			    yyvarsp->rng_vars[resrange->pss_rgno] = resrange;
			}

			/* If has_DISTINCT then the next loop is a no-ops as there
			** will not be any aggregates and no BYHEAD to build */
			while (psl_find_nodep(&residue->pst_right, &agheadp, PST_AGHEAD))
			{
			    PST_QNODE *byhead = NULL;
			    PST_QNODE *aop;
			    PSS_DUPRB dup_rb;
			    aghead = *agheadp;
			    v = subsel->pst_left;
			    /* Find start of VAR RESDOMs for the groupby list(s)*/
			    while (v && v->pst_sym.pst_type == PST_RESDOM &&
						v->pst_right->pst_sym.pst_type != PST_VAR)
				v = v->pst_left;
			    dup_rb.pss_tree = v;
			    dup_rb.pss_dup = &byhead;
			    dup_rb.pss_1ptr = NULL;
			    dup_rb.pss_op_mask = 0;
			    dup_rb.pss_num_joins = PST_NOJOIN;
			    dup_rb.pss_tree_info = NULL;
			    dup_rb.pss_mstream = &cb->pss_ostream;
			    dup_rb.pss_err_blk = &psq_cb->psq_error;
			    if (status = pst_treedup(cb, &dup_rb))
				return(status);

			    /* Attach the copied resdoms as group by list under a BYHEAD */
			    MEfill(sizeof(resdom), 0, (PTR)&resdom);
			    aop = aghead->pst_left;
			    if (aop->pst_sym.pst_type == PST_BYHEAD)
			    {
				/* 'aop' is really the byhead - leavein place and
				** replace its list */
				aop->pst_left = byhead;
				/* The real aop is down aop->pst_right so to avoid
				** further confusion... */
				aop = aop->pst_right;
			    }
			    else
			    {
				aghead->pst_left = NULL;
				if (status = pst_node(cb, &cb->pss_ostream, byhead, aop, 
					    PST_BYHEAD, (PTR) &resdom, sizeof(resdom), 
					    aop->pst_sym.pst_dataval.db_datatype,
					    aop->pst_sym.pst_dataval.db_prec, 
					    aop->pst_sym.pst_dataval.db_length, 
					    (DB_ANYTYPE *) NULL,
					    &aghead->pst_left, &psq_cb->psq_error, (i4) 0))
				    return status;
			    }
			    /* A quick check that the agg variable(s) is in the correct table */
			    v = NULL;
			    while (psl_find_node(aop, &v, PST_VAR))
			    {
				if (!BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
				    (char*)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm))
				{
				    /* VAR not in this from list - complain */
				    (VOID) psf_error(2923L, 0L, PSF_USERERR, &err_code,
					&psq_cb->psq_error, 1, (i4) sizeof(cb->pss_lineno),
					&cb->pss_lineno);
				    return E_DB_ERROR;
				}
			    }
			    /* Float non-local factors from the aggregate - will not leave
			    ** AGH qual unterminated */
			    if (status = psl_float_factors(cb, psq_cb, aghead, subsel))
				return status;
			}
			/* Retarget any HAVING vars or qual var nodes just added
			** to refer to drtab */
			psl_retarget_vars(cb, subsel->pst_right, subsel, resrange);


			/* We need to retarget the inner rels for joinid in rng_vars. */
			if ((joinid = subsel->pst_sym.pst_value.pst_s_root.pst_ss_joinid) != PST_NOJOIN)
			{
			    u_i4 i;
			    for (i = 0; i < PST_NUMVARS; i++)
			    {
				PSS_RNGTAB *rngvar = &cb->pss_auxrng.pss_rngtab[i];
				if (rngvar->pss_used && rngvar->pss_rgno >= 0 &&
				    BTtest(joinid, (char*)&rngvar->pss_inner_rel))
				{
				    BTclear(joinid, (char*)&rngvar->pss_inner_rel);
				    if (!BTcount((char *)&rngvar->pss_inner_rel,
					    BITS_IN(rngvar->pss_inner_rel)))
					rngvar->pss_var_mask &= ~PSS_INNER_RNGVAR;
				}
			    }
			    BTset(joinid, (char*)&resrange->pss_inner_rel);
			    resrange->pss_var_mask |= PSS_INNER_RNGVAR;
			}

			/* Setup a VAR node to refer to each agg dr result and any
			** stray vars - first each agg */
			while (agheadp = psl_find_nodep(&residue->pst_right, NULL, PST_AGHEAD))
			{
			    PST_VAR_NODE varnode;
			    PST_QNODE *ag_resdom = subsel->pst_left;
			    PST_QNODE *res = NULL;
			    PST_QNODE *aop;
			    PST_STK_CMP cmp_ctx;

			    PST_STK_CMP_INIT(cmp_ctx, cb, NULL);
			    /* Find the agg resdom in question - remember that 
			    ** both the original and the new pointed to the same AGH */
			    while (ag_resdom->pst_sym.pst_type == PST_RESDOM)
			    {
				if (!pst_qtree_compare(&cmp_ctx, &ag_resdom->pst_right,
						agheadp, FALSE))
				    break;
				ag_resdom = ag_resdom->pst_left;
			    }
			    if (!ag_resdom || ag_resdom->pst_sym.pst_type != PST_RESDOM)
				psl_bugbp();/*FIXME error*/
			    varnode.pst_vno = resrange->pss_rgno;
			    varnode.pst_atno.db_att_id =
					ag_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
			    MEcopy((PTR)ag_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
				    sizeof(varnode.pst_atname),(PTR)&varnode.pst_atname);
			    aop = (*agheadp)->pst_left->pst_right;
			    if (status = pst_node(cb, &cb->pss_ostream, NULL, NULL, 
				    PST_VAR, (PTR) &varnode, sizeof(varnode), 
				    ag_resdom->pst_sym.pst_dataval.db_datatype,
				    ag_resdom->pst_sym.pst_dataval.db_prec, 
				    ag_resdom->pst_sym.pst_dataval.db_length, 
				    (DB_ANYTYPE *) NULL,
				    agheadp, &psq_cb->psq_error, (i4) 0))
				return status;
			    /* Will need to force rescan as we've updated the tree */

			    /* The COUNT() aggregate needs to return 0 instead of NULL
			    ** in the event of no rows */
			    switch (aop->pst_sym.pst_value.pst_s_op.pst_opno)
			    {
			    case ADI_CNTAL_OP:
			    case ADI_CNT_OP:
				/* COUNT() needs to return 0 instead of NULL in the
				** event of no rows */
				{
				    PST_CNST_NODE const0;
				    i2 zero = 0;
				    PST_QNODE *zeronode;
				    PST_OP_NODE	opnode;
				    MEfill(sizeof(const0), 0, &const0);
				    const0.pst_tparmtype = PST_USER;
				    /*const0.pst_parm_no = 0;*/
				    const0.pst_pmspec = PST_PMNOTUSED;
				    const0.pst_cqlang = DB_SQL;
				    /*const0.pst_origtxt = (char *)NULL;*/
				    if (status = pst_node(cb, &cb->pss_ostream, NULL, NULL,
					    PST_CONST, (char*)&const0, sizeof(const0),
					    DB_INT_TYPE, 0, sizeof(zero),(DB_ANYTYPE *)&zero,
					    &zeronode, &psq_cb->psq_error, 0))
					return status;
				    MEfill(sizeof(opnode), 0, &opnode);
				    opnode.pst_opno = ADI_IFNUL_OP;
				    opnode.pst_opmeta = PST_NOMETA;
				    /*opnode.pst_escape = 0;*/
				    /*opnode.pst_pat_flags = 0;*/
				    opnode.pst_joinid = joinid;
				    if (status = pst_node(cb, &cb->pss_ostream, *agheadp, zeronode,
					    PST_BOP, (char*)&opnode, sizeof(opnode),
					    0, 0, 0,(DB_ANYTYPE *)NULL,
					    agheadp, &psq_cb->psq_error, PSS_JOINID_PRESET))
					return status;
				}
			    }
			    /* Force the rescan */
			    agheadp = NULL;
			}
			
			/* Retarget any naked VARs in the residue */
			psl_retarget_vars(cb, residue, subsel, resrange);

			/* Scan the combined list and move bool factors up that require
			** tables outside of this scope - from subsel to rt_node. */
			if (status = psl_float_factors(cb, psq_cb, subsel, rt_node))
			    return status;

			/* Now the same for the VARs (skipping those already fixed) */
			psl_retarget_vars(cb, rt_node->pst_right, subsel, resrange);

			/* Now we have the residual tree which was the sub-select
			** expression and now has the aggregates set to VAR nodes
			** and any 'stray' vars all waiting to be re-targetted. */
			v = NULL;
			while (psl_find_node(residue->pst_right, &v, PST_VAR))
			{
			    PST_VAR_NODE varnode;
			    PST_QNODE *ag_resdom = subsel->pst_left;
			    PST_QNODE *res = NULL;
			    PST_QNODE **p;
			    if (v->pst_sym.pst_value.pst_s_var.pst_vno == resrange->pss_rgno)
				continue; /* Already done */
			    if (p = psl_lu_resdom_var(cb, &subsel->pst_left, v))
			    {
				v->pst_sym.pst_value.pst_s_var.pst_vno = resrange->pss_rgno;
				v->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
					(*p)->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
				MEcopy((PTR)(*p)->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
				    sizeof(v->pst_sym.pst_value.pst_s_var.pst_atname),
				    (PTR)&v->pst_sym.pst_value.pst_s_var.pst_atname);
			    }
			}
			/* The *nodep started out indirectly pointing at the subsel -
			** we now slot in the fixed up residual tree. */
			*nodep = residue->pst_right;

			BTset(resrange->pss_rgno,
			    (char*)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm);
			BTclearmask(sizeof(PST_J_MASK),
			    (char*)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm,
			    (char*)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm);
			rt_node->pst_sym.pst_value.pst_s_root.pst_tvrc =
			    BTcount((char *)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm,
			    BITS_IN(rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm));
		    }
		    else if (~subsel->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
		    {
			PST_QNODE *v;

			/* Move bool factors up that require tables outside of
			** this scope - from subsel to rt_node - if we can.
			** This also may rearrange the qtree into a more
			** canonical form.
			*/
			if (rt_node &&
				(status = psl_float_factors(cb, psq_cb, subsel, rt_node)))
			    return status;

			/* Attach the qual to the AGHEAD, and to any other
			** AGHEAD's in the subselect.  The first AGHEAD gets
			** the "original" (possibly rearranged) qual, any
			** later ones get a copy.
			*/
			v = subsel->pst_right;	/* First AGH gets original */
			do
			{
			    /* Same for AGHs */
			    if (rt_node &&
				    (status = psl_float_factors(cb, psq_cb, *agheadp, rt_node)))
				return status;
			    if (v == NULL)
			    {
				/* ... and copies thereof to any other ag-WHERE
				** clause in this subsel */
				PSS_DUPRB dup_rb;
				dup_rb.pss_tree = subsel->pst_right;
				dup_rb.pss_dup = &v;
				dup_rb.pss_1ptr = NULL;
				dup_rb.pss_op_mask = 0;
				dup_rb.pss_num_joins = PST_NOJOIN;
				dup_rb.pss_tree_info = NULL;
				dup_rb.pss_mstream = &cb->pss_ostream;
				dup_rb.pss_err_blk = &psq_cb->psq_error;
				if (status = pst_treedup(cb, &dup_rb))
				    return(status);
			    }
			    if (status = psy_apql(cb, &cb->pss_ostream,
					v, *agheadp, &psq_cb->psq_error))
				return status;
			    v = NULL;	/* Ask for copies now */
			    while (psl_find_node((*agheadp)->pst_left, &v, PST_VAR))
			    {
				if (!BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
				    (char*)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm))
				{
				    /* VAR not in this from list - complain */
				    (VOID) psf_error(2923L, 0L, PSF_USERERR, &err_code,
					&psq_cb->psq_error, 1, (i4) sizeof(cb->pss_lineno),
					&cb->pss_lineno);
				    return E_DB_ERROR;
				}
			    }
			} while (psl_find_nodep(&residue->pst_right, &agheadp, PST_AGHEAD));
			/* Propagate FROM list to containing subquery if it had none */
			if (rt_node && !rt_node->pst_sym.pst_value.pst_s_root.pst_tvrc)
			{
			    rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm =
				    subsel->pst_sym.pst_value.pst_s_root.pst_tvrm;
			    rt_node->pst_sym.pst_value.pst_s_root.pst_tvrc =
				    subsel->pst_sym.pst_value.pst_s_root.pst_tvrc;
			}
			/* Drop the SUBSEL */
			*nodep = subsel->pst_left->pst_right;
		    }
		    else /* With the grouped case we leave the subsel in place */
		    {
			/* Move bool factors up that require tables outside of this
			** scope - from subsel to rt_node - if we can. */
			if (rt_node)
			{
			    if (status = psl_float_factors(cb, psq_cb, subsel, rt_node))
				return status;

			    /* and copies thereof to any other ag-WHERE clause in this subsel */
			    do
			    {
				/* A quick check that the agg variable(s) is in the correct table */
				PST_QNODE *v = NULL;
				while (psl_find_node((*agheadp)->pst_left, &v, PST_VAR))
				{
				    if (!BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
					(char*)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm))
				    {
					/* VAR not in this from list - complain */
					(VOID) psf_error(2923L, 0L, PSF_USERERR, &err_code,
					    &psq_cb->psq_error, 1, (i4) sizeof(cb->pss_lineno),
					    &cb->pss_lineno);
					return E_DB_ERROR;
				    }
				}
				if (status = psl_float_factors(cb, psq_cb, *agheadp, rt_node))
				    return status;
			    } while (psl_find_nodep(&residue->pst_right, &agheadp, PST_AGHEAD));
			}
		    }
		}
		else if (rt_node)
		{
		    /*
		    **           ROOT                         ROOT
		    **           /   \                       /    \
		    **         RSD    qual1                 /      AND
		    **        /   \                        /      /   \
		    **    [RSDs]   SUBSEL                RSD'  qual2  qual1
		    **     /       /   \                /   \
		    **  TRE      RSD'   qual        [RSDs] [tree]
		    **          /   \                /
		    **       TRE     [tree]       TRE
		    */
		    *nodep = residue->pst_right;
		    /* Append qual */
		    if (status = psy_apql(cb, &cb->pss_ostream, subsel->pst_right,
					rt_node, &psq_cb->psq_error))
			return status;
		    BTor(sizeof(PST_J_MASK),
			    (char *)&subsel->pst_sym.pst_value.pst_s_root.pst_tvrm,
			    (char *)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm);
		    rt_node->pst_sym.pst_value.pst_s_root.pst_tvrc =
			    BTcount((char *)&rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm,
			    BITS_IN(rt_node->pst_sym.pst_value.pst_s_root.pst_tvrm));
		}
		break;
	    }
	    pst_pop_all(&norm_stk);
	    if ((node = *nodep) && 
		node->pst_sym.pst_type == PST_SUBSEL)
	    {
		/* There really is still a subselect in tree */
		cb->pss_stmt_flags |= PSS_SUBINTREE;

		/* As node is still a SUBSEL, create a QLEND node if needed */
		if (!node->pst_right &&
		    (status = pst_node(cb, &cb->pss_ostream, NULL, NULL,
		    		PST_QLEND, NULL, 0, DB_NODT, 0, 0,
				(DB_ANYTYPE *)NULL, &node->pst_right, &psq_cb->psq_error, 0)))
		    return status;

	    }
	    if (correl)
		subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
				PST_2CORR_VARS_FOUND;
	    else
		subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &=
				~PST_2CORR_VARS_FOUND;
	    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
			    PST_1CORR_VAR_FLAG_VALID;
	    /* Reset 'active subsel' */
	    if ((subsel = pst_antecedant_by_2types(stk, NULL, PST_SUBSEL, PST_ROOT)) &&
			subsel->pst_sym.pst_type == PST_SUBSEL)
		correl = (subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &
				PST_1CORR_VAR_FLAG_VALID) != 0;
	    else
		correl = FALSE;

	    break;

	case PST_ROOT:
	    /* Usually not significant except in the context of unions! */
	    qual_depth--;
	    break;

	case PST_AGHEAD:
	    {
		if (subsel)
		{
		    if ((subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &
				(PST_1CORR_VAR_FLAG_VALID|PST_2CORR_VARS_FOUND)) ==
					    PST_2CORR_VARS_FOUND)
			/* Correlation flags are being corrected */
			subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &=
				~PST_2CORR_VARS_FOUND;
		    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
			    PST_1CORR_VAR_FLAG_VALID;
		    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |=
			    (node->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_8FLATN_SUBSEL);
		}
		/* Create a QLEND node if needed */
		if (!node->pst_right &&
		    (status = pst_node(cb, &cb->pss_ostream, NULL, NULL,
				PST_QLEND, NULL, 0, DB_NODT, 0, 0,
				(DB_ANYTYPE *)NULL, &node->pst_right, &psq_cb->psq_error, 0)))
		    return status;
	    }
	    break;
	case PST_BYHEAD:
	    {
		PST_QNODE *rt_node = pst_antecedant_by_1type(stk, NULL, PST_AGHEAD);
		/* For us to find a BYHEAD implies an explicit, user specified, GROUP BY
		** as opposed to one we've added. If this flag is noted in the subsel code
		** on uncorrelated, we will leave the subselect unflattend */
		if (subsel)
		    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_3GROUP_BY;
		if (rt_node)
		    rt_node->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_3GROUP_BY;
	    }
	    break;
	case PST_WHLIST:
	    whlist_count--;
	    break;
	case PST_RESDOM:
	    {
		PST_QNODE *parent = pst_parent_node(stk, NULL);
		if (parent && parent->pst_sym.pst_type != PST_RESDOM)
		    /* Finished this levels tgtlist - onto WHERE clause*/
		    BTset(qual_depth, (PTR)in_WHERE);
		if (node->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype == PST_USER)
		{
		    /* Ensure any remaining empty names are filled with the column default */
		    i4 i;
		    for (i = 0; i < DB_ATT_MAXNAME; i++)
			if (node->pst_sym.pst_value.pst_s_rsdm.pst_rsname[i] != ' ')
			    break;
		    if (i >= DB_ATT_MAXNAME)
		    {
			STprintf(node->pst_sym.pst_value.pst_s_rsdm.pst_rsname, "col%d",
				    node->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
			i = STlength(node->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
			MEfill(DB_ATT_MAXNAME-i, ' ', node->pst_sym.pst_value.pst_s_rsdm.pst_rsname+i);
		    }
		}
	    }
	    break;
	}
	nodep = (PST_QNODE**)pst_pop_item(stk);
	if (descend = (nodep == PST_DESCEND_MARK))
	    nodep = (PST_QNODE**)pst_pop_item(stk);
    }

    return status;
}

/*
** Name: psl_ss_flatten - flattens out singleton sub-selects for all trees.
**
** Description:
** Input:
**	nodep		Address of pointer to global tree to flatten
**	yyvarsp		Address of PSS_YYVARS context
**	cb		PSS_SESBLK Parser session block
**	psq_cb		Query parse control block
**
** Output:
**	nodep		The tree may be updated.
**
** Side effects:
**	Also flattens any derived tables.
**
** History:
**	02-Nov-2009 (kiria01) 
**	    Created to keep target lists distinct between derived tables
**	    and main.
*/

DB_STATUS
psl_ss_flatten(
	PST_QNODE	**nodep,
	PSS_YYVARS	*yyvarsp,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb)
{
    PST_STK		stk;
    i4			err_code;
    DB_STATUS		status = E_DB_OK;
    i4 i;

    if (Psf_ssflatten == FLAT_CAP_OFF)
	return status;

    PST_STK_INIT(stk, cb);

    /* Rebuild/complete rng_vars index to rngtab & summarise seen join ids */
    for (i = 0; i < PST_NUMVARS; i++)
    {
	PSS_RNGTAB *rngvar = &cb->pss_auxrng.pss_rngtab[i];
	if (rngvar->pss_used && rngvar->pss_rgno >= 0)
	    yyvarsp->rng_vars[rngvar->pss_rgno] = rngvar;
    }
    /* Reset session summary flags */
    cb->pss_stmt_flags &= ~PSS_SUBINTREE;

    /* Flatten the derived tables */
    for (i = 0; i < PST_NUMVARS; i++)
    {
	PSS_RNGTAB *rngvar = &cb->pss_auxrng.pss_rngtab[i];
	if (rngvar->pss_used &&
	    rngvar->pss_rgno >= 0 &&
	    (rngvar->pss_rgtype == PST_DRTREE ||
	    rngvar->pss_rgtype == PST_WETREE) &&
	    rngvar->pss_qtree)
	{
	    status = psl_flatten1(&rngvar->pss_qtree,
			yyvarsp, cb, psq_cb, &stk);
	    pst_pop_all(&stk);
	    if (status)
		return status;
	}
    }
    status = psl_flatten1(nodep, yyvarsp, cb, psq_cb, &stk);
    pst_pop_all(&stk);
    return status;
}


/*
** Name:	psl_add_resdom_var
**
** Description:	this function attempts to add a VAR node as a new column
**		to a drtable under construction. The RESDOM list is scanned
**		to see if the VAR node is already referenced and if not a new
**		resdom is created and set to point to the var node.
**		The list is maintained in sorted order to avoid duplicates
**		and to promote common attribute order with base tables.
**
** Input:
**	cb			PSF session CB
**	chainp			Addresss of pointer to resdom list
**	var			Address of VAR to add (might be expr)
**	
**
** Output:
**	chainp			Resdom chain may be updated
**
** Side effects:
**	may allocate memory
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** History:
**	16-Mar-2010 (kiria01)
**	    Created.
*/

DB_STATUS
psl_add_resdom_var(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	*var)
{   
    DB_STATUS		status = E_DB_OK;
    PST_SYMBOL		*v = &var->pst_sym;
    DB_DATA_VALUE	*d = &v->pst_dataval;
    PST_QNODE		*node, *rvar;
    DB_ERROR		error;
    PST_RSDM_NODE	resdom;
    PSS_DUPRB		dup_rb;

    while ((node = *chainp) && node->pst_sym.pst_type == PST_RESDOM)
    {
	rvar = node->pst_right;
	if (rvar->pst_sym.pst_type != PST_VAR)
	{
	    if (var->pst_sym.pst_type == PST_VAR ||
		var->pst_sym.pst_type != PST_AGHEAD &&
			rvar->pst_sym.pst_type == PST_AGHEAD)
	    {
		/* If we are placing a VAR node, skip resdoms that
		** are not pointing at VAR nodes - we want the vars
		** together at the end of the chain OR
		** we have a non AGH and are in the AGHs we also
		** skip as we want AGGs in the beginning. */
		;
	    }
	    else if (var->pst_sym.pst_type == PST_AGHEAD &&
			rvar->pst_sym.pst_type != PST_AGHEAD)
	    {
		/* We are placing an AGH and have found then last of
		** them (or rather the first of the non-AGH) */
		break;
	    }
	    else
	    {
		PST_STK_CMP cmp_ctx;
		i4 res;
		PST_STK_CMP_INIT(cmp_ctx, cb, NULL);
		if (pst_qtree_compare(&cmp_ctx, &node->pst_right,
				&var, FALSE) < 0)
		    break;
		if (cmp_ctx.same)
		    return status;
	    }
	}
	else if (rvar->pst_sym.pst_value.pst_s_var.pst_vno ==
		v->pst_value.pst_s_var.pst_vno)
	{
	    if (rvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    v->pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		/* Already present */
		return status;
	    }
	    else if (rvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id <
		    v->pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		break; /* add */
	    }
	}
	else if (rvar->pst_sym.pst_value.pst_s_var.pst_vno <
		var->pst_sym.pst_value.pst_s_var.pst_vno)
	{
	    break; /* add */
	}
	/* Not found yet */
	chainp = &node->pst_left;
    }

    MEfill(sizeof(resdom), 0, (char *)&resdom);
    /*resdom.pst_rsno = 0;*/
    /*resdom.pst_ntargno = (i4) 0;*/
    resdom.pst_ttargtype = (i4) PST_USER;
    /*resdom.pst_rsupdt = FALSE;*/
    resdom.pst_rsflags = PST_RS_PRINT;
    /*resdom.pst_dmuflags = 0;*/

    if (var->pst_sym.pst_type == PST_VAR)
    {
	if (status = pst_node(cb, &cb->pss_ostream, NULL, NULL,
			PST_VAR, (char *)&v->pst_value.pst_s_var,
			(i4)sizeof(v->pst_value.pst_s_var),
			d->db_datatype, d->db_prec, d->db_length,
			NULL, &rvar, &error, 0))
		return status;
    	    MEcopy((char *)&v->pst_value.pst_s_var.pst_atname,
		    sizeof(DB_ATT_NAME), (char *)resdom.pst_rsname);
    }
    else
    {
	DB_ERROR err_blk;
	dup_rb.pss_tree = var;
	dup_rb.pss_dup = &rvar;
	dup_rb.pss_1ptr = NULL;
	dup_rb.pss_op_mask = 0;
	dup_rb.pss_num_joins = PST_NOJOIN;
	dup_rb.pss_tree_info = NULL;
	dup_rb.pss_mstream = &cb->pss_ostream;
	dup_rb.pss_err_blk = &err_blk;

	if (status = pst_treedup(cb, &dup_rb))
	    return (status);
	if (rvar->pst_sym.pst_type == PST_AGHEAD &&
	    (node = rvar->pst_left) &&
	    node->pst_sym.pst_type == PST_AOP &&
	    node->pst_sym.pst_value.pst_s_op.pst_opno == ADI_SINGLETON_OP &&
	    (node = node->pst_left) &&
	    node->pst_sym.pst_type == PST_VAR)
	{
	    MEcopy(&node->pst_sym.pst_value.pst_s_var.pst_atname,
			sizeof(DB_ATT_NAME), (PTR)resdom.pst_rsname);
	}
	else
	    MEfill(sizeof(DB_ATT_NAME), ' ', (char *)resdom.pst_rsname);
    }

    return pst_node(cb, &cb->pss_ostream, *chainp, rvar,
		    PST_RESDOM, (char *)&resdom, (i4)sizeof(resdom),
		    d->db_datatype, d->db_prec, d->db_length,
		    NULL, chainp, &error, 0);
}

/*
** Name:	psl_add_resdom_agh
**
** Description:	this function attempts to add a AGHEAD node as a new column
**		to a drtable under construction. The RESDOM list is scanned
**		to see if the AGHEAD is already referenced and if not this
**		a resdom is created and set to point to the AGH node.
**		It is expected that it will be taking ownership of the AGHEAD
**		sub-tree and the caller must relinquish it.
**		The list is maintained in sorted order to avoid duplicates
**		and to promote common attribute order with base tables.
**
** Input:
**	cb			PSF session CB
**	chainp			Addresss of pointer to resdom list
**	aghp			Address of pointer to AGH to add
**	
**
** Output:
**	chainp			Resdom chain may be updated
**	*aghp			May be updated if a duplicate found
**
** Side effects:
**	may allocate memory
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** History:
**	16-Mar-2010 (kiria01)
**	    Created.
*/

DB_STATUS
psl_add_resdom_agh(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	**aghp)
{   
    DB_STATUS		status = E_DB_OK;
    PST_SYMBOL		*v = &(*aghp)->pst_sym;
    DB_DATA_VALUE	*d = &v->pst_dataval;
    PST_QNODE		*node, *rvar;
    DB_ERROR		error;
    PST_RSDM_NODE	resdom;
    PSS_DUPRB		dup_rb;

    if ((*aghp)->pst_sym.pst_type != PST_AGHEAD)
	psl_bugbp();

    while ((node = *chainp) && node->pst_sym.pst_type == PST_RESDOM)
    {
	rvar = node->pst_right;
	if (rvar->pst_sym.pst_type != PST_AGHEAD)
	{
	    /* We are placing an AGH and have found the last of
	    ** them (or rather the first of the non-AGH) */
	    break;
	}
	else if (*aghp == node->pst_right)
	    return status;
	else
	{
	    PST_STK_CMP cmp_ctx;
	    i4 res;
	    PST_STK_CMP_INIT(cmp_ctx, cb, NULL);
	    if (pst_qtree_compare(&cmp_ctx, &node->pst_right,
			    aghp, FALSE) < 0)
		break;
	    if (cmp_ctx.same)
	    {
		*aghp = node->pst_right;
		return status;
	    }
	}
	/* Not found yet */
	chainp = &node->pst_left;
    }

    MEfill(sizeof(resdom), 0, (char *)&resdom);
    /*resdom.pst_rsno = 0;*/
    /*resdom.pst_ntargno = (i4) 0;*/
    resdom.pst_ttargtype = (i4) PST_USER;
    /*resdom.pst_rsupdt = FALSE;*/
    resdom.pst_rsflags = PST_RS_PRINT;
    /*resdom.pst_dmuflags = 0;*/

    if ((node = (*aghp)->pst_left) &&
	node->pst_sym.pst_type == PST_AOP &&
	node->pst_sym.pst_value.pst_s_op.pst_opno == ADI_SINGLETON_OP &&
	(node = node->pst_left) &&
	node->pst_sym.pst_type == PST_VAR)
    {
	MEcopy(&node->pst_sym.pst_value.pst_s_var.pst_atname,
		    sizeof(DB_ATT_NAME), (PTR)resdom.pst_rsname);
    }
    else
	MEfill(sizeof(DB_ATT_NAME), ' ', (char *)resdom.pst_rsname);

    return pst_node(cb, &cb->pss_ostream, *chainp, *aghp,
		    PST_RESDOM, (char *)&resdom, (i4)sizeof(resdom),
		    d->db_datatype, d->db_prec, d->db_length,
		    NULL, chainp, &error, 0);
}

/*
** Name:	psl_lu_resdom_var
**
** Description:	this function attempts to locate a VAR node as a column
**		in a drtable under construction. The RESDOM list is scanned
**		to see if the VAR node is present and if found, the address
**		of the pointer to the relevant resdom is returned.
**
** Input:
**	cb			PSF session CB
**	chainp			Addresss of pointer to resdom list
**	var			Address of to add
**	
**
** Output:
**	chainp			Resdom chain may be updated
**
** Side effects:
**	None
**
** Returns:
**	Address of pointer to node found or NULL if not found.
**
** History:
**	16-Mar-2010 (kiria01)
**	    Created.
*/

PST_QNODE **
psl_lu_resdom_var(
	PSS_SESBLK	*cb,
	PST_QNODE	**chainp,
	PST_QNODE	*var)
{   
    PST_SYMBOL		*v = &var->pst_sym;
    PST_QNODE		*node;

    while ((node = *chainp) && node->pst_sym.pst_type == PST_RESDOM)
    {
	PST_QNODE *rvar = node->pst_right;
	if (rvar->pst_sym.pst_type != PST_VAR)
	{
	    if (var->pst_sym.pst_type == PST_VAR ||
		var->pst_sym.pst_type != PST_AGHEAD &&
			rvar->pst_sym.pst_type == PST_AGHEAD)
	    {
		/* If we are looking for a VAR node, skip resdoms that
		** are not pointing at VAR nodes - we have the vars
		** together at the end of the chain
		** OR
		** we are looking for a non AGH and are in the AGHs we also
		** skip as we have AGGs in the beginning. */
		;
	    }
	    else if (var->pst_sym.pst_type == PST_AGHEAD &&
			rvar->pst_sym.pst_type != PST_AGHEAD)
	    {
		/* We are looking for an AGH and have run out of them */
		break;
	    }
	    else if (node->pst_right == var)
		/* AGHEADS are 'owned' by the resdom list and may be compared
		** directly. We don't loookup via compare as the trees might get
		** out of sync due to augmentation of BYHEAD data so we share the
		** AGHEAD node addresses. */
		return chainp;
	    else
	    {
		PST_STK_CMP cmp_ctx;
		i4 res;
		PST_STK_CMP_INIT(cmp_ctx, cb, NULL);
		if (pst_qtree_compare(&cmp_ctx, &node->pst_right,
				&var, FALSE) < 0)
		    break;
		if (cmp_ctx.same)
		    return chainp;
	    }
	}
	else if (rvar->pst_sym.pst_value.pst_s_var.pst_vno ==
		v->pst_value.pst_s_var.pst_vno)
	{
	    if (rvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    v->pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		return chainp;
	    }
	    else if (rvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id <
		    v->pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		break;
	    }
	}
	else if (rvar->pst_sym.pst_value.pst_s_var.pst_vno <
		var->pst_sym.pst_value.pst_s_var.pst_vno)
	{
	    break;
	}
	/* Not found yet */
	chainp = &node->pst_left;
    }
    return NULL;
}

/*
** Name:	psl_retarget_vars
**
** Description:	this function retargets VAR nodes in a passed subtree to
**		refer indirectly via the columns of a derived table.
**		To do this, walks the tree and examines each VAR node. If
**		the node is in the table representing the dtroot then
**		the dtroots resdoms are scanned looking for the specified
**		VAR - when found,  the VAR is adjusted to refer to the
**		relevant attribute fo the dtroot.
**
** Input:
**	tree			Addresss of pointer to tree needing fixup
**	dtroot			Address of node representing root of new
**				derived table. Its pst_tvrm will be set
**				correctly and the resdoms will be the columns.
**	resrange		This will be the range table entry for the
**				derived table.
**
** Output:
**	indirectly tree		Its var nodes will be updated.
**
** Side effects:
**	None
**
** Returns:
**	void
**
** History:
**	01-Apr-2010 (kiria01)
**	    Created to reduce code repetition.
*/

static VOID
psl_retarget_vars(
	PSS_SESBLK	*cb,
      PST_QNODE		*tree,
      PST_QNODE		*dtroot,
      PSS_RNGTAB	*resrange)
{
    PST_QNODE *v = NULL;
    /* Renaming HAVING var nodes to refer to drtab */
    while (psl_find_node(tree, &v, PST_VAR))
    {
	i4 vno = v->pst_sym.pst_value.pst_s_var.pst_vno;
	if (BTtest(vno,
	    (char*)&dtroot->pst_sym.pst_value.pst_s_root.pst_tvrm))
	{
	    PST_QNODE **p = psl_lu_resdom_var(cb, &dtroot->pst_left->pst_left, v);
	    if (p)
	    {
		v->pst_sym.pst_value.pst_s_var.pst_vno = resrange->pss_rgno;
		v->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
			    (*p)->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		MEcopy((PTR)(*p)->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			    sizeof(v->pst_sym.pst_value.pst_s_var.pst_atname),
			    (PTR)&v->pst_sym.pst_value.pst_s_var.pst_atname);
	    }
	}
    }
}

/*
** Name:	psl_float_factors
**
** Description:	this function moves predicates whose scope is not wholly 
**		satisfied by the current scope.  It may also rearrange
**		the subtree even if no predicates are relocated.
**		The 'from' predicate is treated as a list of AND nodes
**		in partial CNF. The normalisation is performed ad-hoc sf
**		needed during the single pass.
** Input:
**	cb			Address of session cb
**	psq_cb			Address of parser cb
**	from			Address of the source predicate tree
**	to			Address or recipient predicate tree
**
** Output:
**	from & to		Boolean factors may be moved from
**				the source to the target if appropriate
**				Also the from list will be partly in
**				CNF - AND nodes all down the right.
**
** Side effects:
**	None
**
** Returns:
**	void
**
** History:
**	01-Apr-2010 (kiria01)
**	    Created to reduce code repetition.
*/

static DB_STATUS
psl_float_factors(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PST_QNODE	*from,
	PST_QNODE	*to)
{
    DB_STATUS status = E_DB_OK;
    PST_QNODE **andp = &from->pst_right;

    /* This outer loop will be passed once per 'factor' as if
    ** the list were constructed:
    **        AND
    **       /   \
    **   factor   AND
    **           /   \
    **       factor   AND
    **               /   \
    **           factor   factor
    */
    while (*andp)
    {
	PST_QNODE *v = NULL;
	PST_QNODE *a;

	/* The above structure (CNF) may not be what we are passed
	** but the following loop forces this into happening as needed
	** using the following:
	** 1) If node is not an AND - pass on the whole. [a = *andp]
	**	(This is the RHS terminal factor)
	** 2a) Is an AND and no LHS - drop the AND and loop.
	** 2b) Is an AND and LHS is not AND. (Segment in CNF) - simply pass
	**	on the LHS and loop for next. [a != *andp]
	** 3) Is an AND & its LHS is AND. We pause and 'rotate'
	**	into CNF. Might need multiple 'rotates' but can always
	**	be done. One 'rotate' does this
	**
	**	   \                 \
	**          ANDa              ANDb
	**         /    \            /    \
	**     ANDb     tree3     tree1    ANDa
	**    /    \                      /    \
	** tree1   tree2               tree2   tree3
	*/
	while ((a = *andp)->pst_sym.pst_type == PST_AND)
	{
	    /* Flattening pass */
	    if (a = a->pst_left)
	    {
		if (a->pst_sym.pst_type == PST_AND)
		{
		    /* AND to side move in line or drop */
		    if ((*andp)->pst_left = a->pst_right)
		    {
			a->pst_right = *andp; /* rotate */
			*andp = a;
		    }
		    else if (!((*andp)->pst_left = a->pst_left))
			*andp = (*andp)->pst_right; /* drop */
		}
		else
		    break; /* a is the expression */
	    }
	    else /* <> AND x -> x*/
		*andp = (*andp)->pst_right;
	}

	/* Having identified the next 'boolean factor', scan the
	** segment for non-local VAR nodes and if found, cause this segment
	** to move to the outer scope - in effect for later restruction. */
	while (psl_find_node(a, &v, PST_VAR))
	{
	    if (!BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
		(char*)&from->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    {
		/* Non-local VAR found - add to the 'to' predicate list */
		if (status = psy_apql(cb, &cb->pss_ostream, a, to,
					&psq_cb->psq_error))
		    return status;
		break;
	    }
	}
	if (!v)
	{
	    /* Didn't trip BTtest so will be local. We will leave this factor
	    ** in place and to do so we either step over it if it was on LHS of
	    ** AND or break out if at RHS (a == *andp) */
	    if (a == *andp)
		break;
	    andp = &(*andp)->pst_right;
	}
	else
	{
	    /* The segment has been moved so we remove from the from list
	    ** thus: (The NULL will also trigger loop exit.)*/
	    *andp = (a == *andp) ? NULL : (*andp)->pst_right;
	}
    }
    /* If we floated all - don't leave a NULL list */
    if (!from->pst_right)
	status = pst_node(cb, &cb->pss_ostream, NULL, NULL,
			PST_QLEND, NULL, 0, DB_NODT, 0, 0,
			(DB_ANYTYPE *)NULL, &from->pst_right,
			&psq_cb->psq_error, 0);
    return status;
}

/*
** Name:	psl_project_vars
**
** Description:	this function projects vars to a derived tables attribute
**		list for subsequent lookup.
** Input:
**	cb			Address of session cb
**	srctree			Address of the source tree - typically
**				this will not be a predicate tree but
**				resdom list from a byhead.
**	desttree		Address of dtroot
**
** Output:
**	desttree		Address of dtroot whose resdom list will
**				be added to
**
** Side effects:
**	May cause allocation of memory
**
** Returns:
**	void
**
** History:
**	01-Apr-2010 (kiria01)
**	    Created to reduce code repetition.
*/
static DB_STATUS
psl_project_vars (
	PSS_SESBLK	*cb,
	PST_QNODE	*srctree,
	PST_QNODE	*desttree)
{
    DB_STATUS status = E_DB_OK;
    PST_QNODE *v = NULL;
    while (psl_find_node(srctree, &v, PST_VAR))
    {
	i4 vno = v->pst_sym.pst_value.pst_s_var.pst_vno;
	if (BTtest(vno,
	    (char*)&desttree->pst_sym.pst_value.pst_s_root.pst_tvrm))
	{
	    /* Add resdom & var after agg resdom */
	    if (status = psl_add_resdom_var(cb,
			    &desttree->pst_left, v))
		return status;
	}
    }
    return status;
}

/*
** Name:	psl_project_corr_vars
**
** Description:	this function projects vars to a derived tables attribute
**		list for subsequent lookup. Unlike psl_project_vars(),
**		this routine is selective - it only projects VARS that it
**		can see will be required to be exported through a boolean
**		factor.
**
**		Like psl_float_factors() and for similar reasons, this
**		routine, will leave the srctree in partial CNF form. See
**		psl_float_factors() notes for algorithm.
**
** Input:
**	cb			Address of session cb
**	srctree			Address of the source tree - this will be
**				a predicate tree containing lists of
**				correlations
**	desttree		Address of dtroot
**
** Output:
**	desttree		Address of dtroot whose resdom list will
**				be added to
**
** Side effects:
**	May cause allocation of memory
**
** Returns:
**	void
**
** History:
**	01-Apr-2010 (kiria01)
**	    Created to reduce code repetition.
*/
static DB_STATUS
psl_project_corr_vars (
	PSS_SESBLK	*cb,
	PST_QNODE	*srctree,
	PST_QNODE	*desttree)
{
    DB_STATUS status = E_DB_OK;
    PST_QNODE **andp = &srctree->pst_right;

    /* Scan the passed predicate and 'cut' into 'boolean factors'.
    ** See psl_float_factors() for algorithm description. */
    while (*andp)
    {
	PST_QNODE *v = NULL;
	PST_QNODE *a;
	bool inner_seen = FALSE;
	bool outer_seen = FALSE;

	while ((a = *andp)->pst_sym.pst_type == PST_AND)
	{
	    /* Flattening pass */
	    if (a = a->pst_left)
	    {
		if (a->pst_sym.pst_type == PST_AND)
		{
		    /* AND to side move in line or drop */
		    if ((*andp)->pst_left = a->pst_right)
		    {
			a->pst_right = *andp; /* rotate */
			*andp = a;
		    }
		    else if (!((*andp)->pst_left = a->pst_left))
			*andp = (*andp)->pst_right; /* drop */
		}
		else
		    break; /* a is the expression */
	    }
	    else /* <> AND x -> x*/
		*andp = (*andp)->pst_right;
	}

	/* The code above just 'cuts' the tree into a list of ANDed boolean
	** factors and we get these delivered to us one at a time. So, we look
	** at the whole sub-tree and see if its VAR nodes reflect BOTH local
	** and non-local VARs. If they do rescan the vars adding the locals to
	** the projection list. If only local then we need not project and if only
	** non-local - we cannot project it.
	** The astute will note that not all these ANDed sub-trees will be
	** VAR-BOP-VAR but could have OR, UOP etc clouding things. In that
	** case we err on the causious side and project anyway. We could project
	** all the locals but this would cause the tendancy to restrict later
	** than we could. */
	while (psl_find_node(a, &v, PST_VAR))
	{
	    if (BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
		(char*)&desttree->pst_sym.pst_value.pst_s_root.pst_tvrm))
		inner_seen = TRUE;
	    else
		outer_seen = TRUE;
	}
	if (inner_seen && outer_seen)
	{
	    /* factor features local and non-local - project local */
	    while (psl_find_node(a, &v, PST_VAR))
	    {
		if (BTtest(v->pst_sym.pst_value.pst_s_var.pst_vno,
			    (char*)&desttree->pst_sym.pst_value.pst_s_root.pst_tvrm) &&
		    (status = psl_add_resdom_var(cb, &desttree->pst_left, v)))
			return status;
	    }
	}
	/* Look to next factor if any - if (a == *andp) we are at RH end and
	** thus finished */
	if (a == *andp)
	    break;
	andp = &(*andp)->pst_right;
    }
    return status;
}

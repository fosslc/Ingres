/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPASUBOJ.C - routines to detect qualifying "not exists",
**	"not in", "not = any" and "<> all" subselects and transform 
**	them to outer joins.
**
**  Description:
**      perform "not exists" subselect to outer join transform.
**
**  History:    
**      9-aug-99 (inkdo01)
**	    Written.
**	20-mar-02 (hayke02)
**	    Added new function opa_subsel_byheadchk() to check for byhead
**	    nodes - if we find one, we don't do the OJ transform.
**	1-apr-04 (inkdo01)
**	    Change TID to 8 bytes for partitioning support.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar Subselects - extend scope of opa_subsel_to_oj
**	    to trawl other parts of the query tree.
**	21-may-10 (smeke01) b123752
**	    Change parameter bool newoj to PST_J_ID *pjoinid.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static void opa_subsel_joinid(
	OPS_STATE *global,
	PST_QNODE *nodep,
	PST_J_ID joinid);
static bool opa_subsel_viewchk(
	OPS_STATE *global,
	PST_QNODE *subselp);
static bool opa_subsel_byheadchk(
	PST_QNODE *subselp);
static void opa_notex_transform(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	PST_QNODE *inner_root,
	bool *gotone,
	PST_J_ID *pjoinid);
static bool opa_notex_analyze(
	OPS_STATE *global,
	PST_QNODE *outer_root,
	PST_QNODE *inner_root,
	PST_QNODE *whnode,
	bool *outer,
	bool *inner,
	bool *nested_subsel);
static void opa_notin_transform(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	PST_QNODE *bopp,
	bool *gotone,
	PST_J_ID *pjoinid);
static bool opa_notin_analyze(
	OPS_STATE *global,
	PST_QNODE *root,
	PST_QNODE *opp);
static void opa_subsel_search(
	OPS_STATE *global,
	PST_QNODE **nodep,
	PST_QTREE *header,
	PST_QNODE *outer_root,
	PST_QNODE *root,
	bool *gotone,
	PST_J_ID *pjoinid);
static void opa_subsel_to_oj1(
	OPS_STATE *global,
	PST_QNODE *outer_root,
	bool *gotone);
static void opa_process_one_qnode(
	OPS_STATE *global,
	PST_QNODE *root,
	bool *gotone);
void opa_subsel_to_oj(
	OPS_STATE *global,
	bool *gotone);

OPV_DEFINE_STK_FNS(node, PST_QNODE**);


/* Static variables and other such stuff. */

static DB_DATA_VALUE	tid_dv = {NULL, DB_TID8_LENGTH, DB_INT_TYPE, 0, -1};
static DB_ATT_ID	tid_att = { 0 };


/*{
** Name: opa_subsel_joinid	- assigns new OJ joinid to ON clause
**
** Description:
**      This function descends the subselect where clause, assigning 
**	the joinid of the new outer join to all its connective and 
**	relop nodes (thus making into the ON clause).
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to subselect where clause parse tree fragment
**	joinid		joinid of new outer join
**
** Outputs:
**      none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**      16-aug-2004 (huazh01)
**          Add support for PST_COP, PST_CASEOP, PST_WHOP, 
**          PST_WHLIST, PST_MOP and PST_SEQOP. 
**          b112821, INGSRV2934. 
**	16-aug-04 (inkdo01)
**	    Add PST_COP to list of transformees.
**	19-feb-07 (kibro01) b117485
**	    Only change the joinid if the symbol is a structure which
**	    contains a JOINID (i.e. PST_OP_NODEs, but not things like NOT)
**
[@history_template@]...
*/
static void
opa_subsel_joinid(
	OPS_STATE	   *global,
	PST_QNODE	   *nodep,
	PST_J_ID	   joinid)

{

    /* Simply descend the fragment looking for operator nodes whose
    ** pst_joinid's must be reassigned. */

    while (nodep)	/* loop is returned from */
     switch (nodep->pst_sym.pst_type) {

      case PST_AND:
      case PST_OR:
      case PST_BOP:
      case PST_CASEOP:
      case PST_WHOP:
      case PST_WHLIST:
      case PST_MOP:
      case PST_SEQOP: 
	opa_subsel_joinid(global, nodep->pst_right, joinid);
				/* recurse on right side */

	/* Fall through to iterate on left with the UOPs. */
      case PST_UOP:
      case PST_COP:
      case PST_NOT:
	/* This is where pst_joinid is reassigned. */
	if (nodep->pst_sym.pst_type == PST_AND ||
	    nodep->pst_sym.pst_type == PST_OR ||
	    nodep->pst_sym.pst_type == PST_UOP ||
	    nodep->pst_sym.pst_type == PST_BOP ||
	    nodep->pst_sym.pst_type == PST_AOP ||
	    nodep->pst_sym.pst_type == PST_COP ||
	    nodep->pst_sym.pst_type == PST_MOP)
	{
	    if (nodep->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN)
		nodep->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
	}
	nodep = nodep->pst_left;
	break;

      default:
	return;			/* return from anything else */
    }	/* end of switch and while loop */

}


/*{
** Name: opa_subsel_viewchk	- check for unflattened view in subsel
**
** Description:
**      This function checks the subselect from clause for an unflattened
**	view. This is indication that the view failed PSF's flattening 
**	rules (union, agg., etc.) and is likely too complex for this 
**	transform.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	subselp		ptr to subselect where clause parse tree fragment
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE - if subsel has view in from clause
**	    FALSE - if subsel doesn't have view in from clause
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-nov-01 (inkdo01)
**	    Written.
**
[@history_template@]...
*/
static bool
opa_subsel_viewchk(
	OPS_STATE	   *global,
	PST_QNODE	   *subselp)

{
    PST_RNGENTRY	**rangetab = global->ops_qheader->pst_rangetab;
    i4			i;

    /* Simply loop over bits in subsel's from clause bit map, checking
    ** corresponding entries in range table for views. */

    for (i = -1; (i = BTnext(i, (char *)&subselp->pst_sym.pst_value.pst_s_root.pst_tvrm,
	BITS_IN(subselp->pst_sym.pst_value.pst_s_root.pst_tvrm))) >= 0;)
     if (rangetab[i]->pst_rgtype == PST_RTREE) 
	return(TRUE);

    return(FALSE);		/* no views if we get here */
}


/*{
** Name: opa_subsel_byheadchk	- check for byhead node in subsel
**
** Description:
**	This function checks the subselect for a byhead node (aggregate
**	group by or having). This is an indication that there will be a
**	aggregate temp table which will break the transform.
**
** Inputs:
**	subselp		ptr to subselect where clause parse tree fragment
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE - if subsel has a byhead node
**	    FALSE - if subsel doesn't have a byhead node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-mar-02 (hayke02)
**	    Written.
**
[@history_template@]...
*/
static bool
opa_subsel_byheadchk(
	PST_QNODE	   *subselp)

{
    PST_QNODE		*r_qnode;

    for (r_qnode = subselp->pst_right; r_qnode; r_qnode = r_qnode->pst_right)
    {
	PST_QNODE	*l_qnode;

	for (l_qnode = r_qnode->pst_left; l_qnode; l_qnode = l_qnode->pst_left)
	    if (l_qnode->pst_sym.pst_type == PST_BYHEAD)
		return(TRUE);
    }
    return(FALSE);		/* no byhead nodes if we get here */
}


/*{
** Name: opa_notex_transform	- verify transform potential and do it
**
** Description:
**      This function confirms that the subselect it was given satisfies
**	the requirements for transformation and then applies said
**	transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to head of NOT EXISTS parse tree 
**			fragment (i.e., the PST_NOT node)
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	inner_root	ptr to root node of subselect 
**	pjoinid		ptr to a PST_J_ID variable that contains the joinid value
**			to be incremented and used for each new join created. Callers
**			can reset this to a base value deliberately so that repeated
**			sections of the tree get the same sequence of join ids.
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means a transform was done
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	header->pst_numjoins may be incremented.
**	    
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**	21-nov-01 (inkdo01)
**	    Add check for views in subsel from clause - these are a complication 
**	    we can't handle yet (the fact that they're still views means PSF
**	    couldn't flatten them - agg, union views or sommat like that).
**	23-nov-01 (inkdo01)
**	    Minor heursitic that avoids the transform when combination
**	    of subsel and main query tables exceeds 8 (OPF CPU time
**	    may go off the chart).
**	16-may-05 (inkdo01)
**	    Changes to aloow OJ transform under PST_AGHEAD (and tidied 
**	    up placement of new predicates in parse tree).
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**	21-may-10 (smeke01) b123752
**	    Use new parameter pjoinid passed in by caller as basis for joinids
**	    for new joins. This enables the caller to make sure that the same
**	    joinid(s) is/are used for any repeat copies of the AGHEAD WHERE clause.
**	    Ensure that global header->pst_numjoins is incremented only for the
**	    first copy.
**	07-jun-10 (smeke01) b123874
**	    Ensure that certain global maps are updated only for the first copy of
**	    the AGHEAD. 'global maps' in this context means the global rangetable
**	    pst_outer_rel and pst_inner_rel maps, and the table map for the root
**	    of the query (pointed to by outer_root).
[@history_template@]...
*/
static void
opa_notex_transform(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	PST_QNODE	   *inner_root,
	bool		   *gotone,
	PST_J_ID	   *pjoinid)

{
    PST_QNODE	*andp, *opp;
    OPV_IGVARS	varno;
    PST_J_ID	joinid;
    i4		ocnt = outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    i4		icnt = inner_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    bool	outer = FALSE, inner = FALSE, nested_subsel = FALSE;


    /* First verify that subselect has no view (union/agg) in its from. */
    if (opa_subsel_viewchk(global, inner_root))
	return;

    /* Second verify that subselect has no byhead nodes */
    if (opa_subsel_byheadchk(inner_root))
	return;

    /* If resulting from clause exceeds 8 because of addition of subsel,
    ** query may be too expensive to compile. */
    if (ocnt + icnt > 8 && ocnt < 8)
	return;

    /* Next check if this subselect really is transformable. */
    if (!opa_notex_analyze(global, outer_root, inner_root,
	    inner_root->pst_right, &outer, &inner, &nested_subsel)) 
	return;				/* if not, leave right away */

    /* Subselect passes the test, now do the transform:
    **   - add "subselect_table.TID is null" to where clause
    **   - subselect where clause becomes outer select on clause
    **   - subselect tables become inner to outer tables
    **   - outer tables become outer to subselect tables
    **   - subselect pst_tvrm is ORed to outer select pst_tvrm
    */

    varno = BTnext((i4)-1, (PTR)&inner_root->pst_sym.pst_value.pst_s_root.
	pst_tvrm, BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* subselect table varno */
    joinid = (*nodep)->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid;
    opp = (*nodep)->pst_left;		/* old EXISTS UOP becomes "is null" */
    andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, joinid);
    andp->pst_left = root->pst_right;
    if ((*nodep) == root->pst_right)
	andp->pst_left = opp;
    else (*nodep) = opp;
    root->pst_right = andp;
    andp->pst_right = inner_root->pst_right;
					/* new ON clause */
    opp->pst_sym.pst_type = PST_UOP;
    opp->pst_sym.pst_value.pst_s_op.pst_opno = 
			global->ops_cb->ops_server->opg_isnull;
    opp->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
    opp->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					/* stick TID var under isnull */
    opp->pst_right = NULL;		/* finish up */

    {
	/* Have to resolve the new "is null". Normally, opa_resolve does
	** this for OPF. But we don't have a subquery structure yet, and
	** the subquery is required by opa_resolve. So we'll just hard
	** code the pst_resolve call which opa_resolve does. */
	DB_STATUS	status;
	DB_ERROR	error;

	status = pst_resolve((PSF_SESSCB_PTR) NULL, 
		global->ops_adfcb, opp, 
		outer_root->pst_sym.pst_value.pst_s_root.pst_qlang, &error);
	if (status != E_DB_OK)
	    opx_verror(status, E_OP0685_RESOLVE, error.err_data);
    }

    /*
    ** The following 'global maps' are updated the first time through
    ** for one complete aghead:
    ** 1. The global rangetable pst_outer_rel and pst_inner_rel maps
    ** 2. The table map for the root of the query ('outer_root')
    ** We must make sure that it isn't done again if there are subsequent
    ** copies of the aghead, as there are when we have more than one
    ** aggregate operator. We can detect that this is the first time
    ** through if the joinid passed in to opa_notex_transform(), when
    ** incremented, is greater than the global joinid variable
    ** header->pst_numjoins. header->pst_numjoins in this context acts as
    ** a high water mark for the joinid numbering.
    */
    joinid = ++*pjoinid;

    /* Assign joinid to all connectives and relops in subsel where clause. */
    opa_subsel_joinid(global, inner_root->pst_right, joinid);
    global->ops_goj.opl_mask = OPL_OJFOUND;
    global->ops_goj.opl_glv = joinid;	/* set some global OJ stuff */

    if (joinid > header->pst_numjoins)
    {
	header->pst_numjoins = joinid;

	/* Loop over outer_root pst_tvrm, ORing joinid into their range table 
	** pst_outer_rel's. Then loop over inner_root pst_tvrm, ORing joinid
	** into their range table pst_inner_rel's. */

	for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
	{
	    PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	    BTset((i4)joinid, (PTR)&rngp->pst_outer_rel.pst_j_mask);
	}

	for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
	{
	    PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	    BTset((i4)joinid, (PTR)&rngp->pst_inner_rel.pst_j_mask);
	}

	/* Finally, OR the inner_root pst_tvrm into the outer_root pst_tvrm. */

	BTor((i4)BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	    BTcount((PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
		BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* update from clause cardinality */
    }

    /* And really finally, if we're under an AGHEAD, OR the inner_root 
    ** pst_tvrm into the AGHEAD (root) pst_tvrm. */

    if (root->pst_sym.pst_type == PST_AGHEAD)
    {
	BTor((i4)BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	    BTcount((PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm,
		BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm));
    }

    *gotone = TRUE;			/* tell the outside world */

}

/*{
** Name: opa_notex_analyze	- analyze "not exists" subselect for
**	transformation potential
**
** Description:
**      Thus function searches the where clause of a "not exists" subselect
**	to see if it can benefit from the outer join transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	outer_root	ptr to root node of outer select 
**	inner_root	ptr to root node of subselect
**	whnode		ptr to where clause node currently being examined
**	outer		ptr to switch indicating outer VAR reference
**	inner		ptr to switch indicating inner VAR reference
**
** Outputs:
**      outer, inner	set to TRUE if outer/inner VAR reference found
**			in subtree
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-99 (inkdo01)
**	    Written.
**	14-june-01 (inkdo01)
**	    Fixed logic for analysis, properly look for correlation predicate
**	    and tolerate simple restrictions.
**	21-june-02 (inkdo01)
**	    Minor changes to allow subselect with multiple predicates to be
**	    properly processed (they used to incorrectly miss the flattening
**	    potential).
**	5-Mar-09 (kibro01) b121740
**	    Allow a check on both sides for collation.  If two aggregates
**	    exist, you can end up with two correlated subqueries on the same
**	    table, which both ought to be combined into the top-level query.
**	    The else was only in place before for optimisation, since it was
**	    assumed that it was impossible to have the same table on both sides
**	    of the query, but in the case where the first instance of this
**	    correlated subquery has been combined into the main query and the
**	    second instance has not yet been combined, you can end up with the
**	    table being registered on both sides of this join.
**
[@history_template@]...
*/
static bool
opa_notex_analyze(
	OPS_STATE	   *global,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *inner_root,
	PST_QNODE	   *whnode,
	bool		   *outer,
	bool		   *inner,
	bool		   *nested_subsel)
	
{


    /* This recursive function searches a where clause fragment (from 
    ** the subselect under consideration) to determine whether there
    ** are any correlation predicates. This is the criterion which 
    ** must be met for the subselect to be transformed to an outer
    ** join.
    **
    ** the function is a big loop which switches on the where clause node 
    ** type and tries to set "outer" and "inner" to indicate some 
    ** correlation predicate.
    */

    while (whnode && whnode->pst_sym.pst_type != PST_QLEND)
     switch (whnode->pst_sym.pst_type) 
    {
      case PST_AND:
      /* case PST_OR: */
	/*	*outer = *inner = FALSE;	*/ /* init flags for analysis */
	opa_notex_analyze(global, outer_root, inner_root,
		whnode->pst_left, outer, inner, nested_subsel);
					/* recurse down left side */
	if (*nested_subsel) return(FALSE);  /* nested subselect forces return */

	whnode = whnode->pst_right;	/* iterate down right side */
	break;

      case PST_BOP:
	opa_notex_analyze(global, outer_root, inner_root,
		whnode->pst_left, outer, inner, nested_subsel);
					/* recurse down left side */
	if (*nested_subsel) return(FALSE);  /* nested subselect forces return */

	whnode = whnode->pst_right;	/* iterate down right side */
	break;

      case PST_NOT:
      case PST_UOP:
	whnode = whnode->pst_left;	/* just iterate on NOT & UOP */
	break;

      case PST_CONST:
	return(*outer && *inner &&!(*nested_subsel));

      case PST_SUBSEL:
	/* Transform doesn't work with nested subselects. */
	*nested_subsel = TRUE;
	return(FALSE);

      case PST_VAR:
	/* Check if column covers either outer or inner select. */
	if (BTtest((u_i2)whnode->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    *outer = TRUE;
	if (BTtest((u_i2)whnode->pst_sym.pst_value.pst_s_var.pst_vno,
		(PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    *inner = TRUE;

	return(*outer && *inner &&!(*nested_subsel));

      default:
	return(FALSE);			/* all others return empty */

    }
    return(*outer && *inner && !(*nested_subsel));

}

/*{
** Name: opa_notin_transform	- verify transform potential and do it
**
** Description:
**      This function confirms that the subselect it was given satisfies
**	the requirements for transformation and then applies said
**	transformation.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to head of NOT IN parse tree 
**			fragment (i.e., the PST_BOP node)
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	bopp		ptr to "=" or "<>" node of subsel compare
**	pjoinid		ptr to a PST_J_ID variable that contains the joinid value
**			to be incremented and used for each new join created. Callers
**			can reset this to a base value deliberately so that repeated
**			sections of the tree get the same sequence of join ids.
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means a transform was done
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	header->pst_numjoins may be incremented.
**	    
**
** History:
**      12-aug-99 (inkdo01)
**	    Written.
**	21-nov-01 (inkdo01)
**	    Add check for views in subsel from clause: these are a complication 
**	    we can't handle yet (the fact that they're still views means PSF
**	    couldn't flatten them - agg, union views or sommat like that).
**	23-nov-01 (inkdo01)
**	    Minor heursitic that avoids the transform when combination
**	    of subsel and main query tables exceeds 8 (OPF CPU time
**	    may go off the chart).
**	20-mar-01 (hayke02)
**	    Add a pst_resolve() call for the bopp node ("=" for on clause).
**	    This prevents "<>" predicates from erroneously appearing in the
**	    query tree. This change fixes bug 107309.
**	25-oct-02 (inkdo01)
**	    Disallow "not in"s when both comparands are expressions (creates
**	    CP join instead of more efficient join).
**	30-mar-04 (hayke02)
**	    Back out the previous change and fix INGSRV 1957, bug 108930 in
**	    opz_fboth() instead. This change fixes problem INGSRV 2732, bug
**	    111899.
**	16-may-05 (inkdo01)
**	    Changes to aloow OJ transform under PST_AGHEAD (and tidied 
**	    up placement of new predicates in parse tree).
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**	21-may-10 (smeke01) b123752
**	    Use new parameter pjoinid passed in by caller as basis for joinids
**	    for new joins. This enables the caller to make sure that the same
**	    joinid(s) is/are used for any repeat copies of the AGHEAD WHERE clause.
**	    Ensure that global header->pst_numjoins is incremented only for the
**	    first copy.
**	01-jul-10 (smeke01) b124009
**	    Ensure that certain global maps are updated only for the first copy of
**	    the AGHEAD. 'global maps' in this context means the global rangetable
**	    pst_outer_rel and pst_inner_rel maps, and the table map for the root
**	    of the query (pointed to by outer_root).
[@history_template@]...
*/
static void
opa_notin_transform(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	PST_QNODE	   *bopp,
	bool		   *gotone,
	PST_J_ID	   *pjoinid)

{
    PST_QNODE	*andp, *opp;
    PST_QNODE	*inner_root = bopp->pst_right;	/* PST_SUBSEL ptr */
    OPV_IGVARS	varno;
    PST_J_ID	joinid;
    i4		ocnt = outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    i4		icnt = inner_root->pst_sym.pst_value.pst_s_root.pst_tvrc;
    bool	outer = FALSE, inner = FALSE, nested_subsel = FALSE;
    DB_STATUS	status;
    DB_ERROR	error;


    /* First check if this subselect really is transformable. */
    if (inner_root->pst_left->pst_sym.pst_type != PST_RESDOM) 
			return;		/* sanity check */
    if (opa_subsel_viewchk(global, inner_root))
			return;		/* fails view-in-subsel-from test */

    /* Verify that subselect has no byhead nodes */
    if (opa_subsel_byheadchk(inner_root))
	return;

    if (ocnt + icnt > 8 && ocnt < 8)
			return;		/* resulting from is too big */
    if (!opa_notin_analyze(global, outer_root, bopp->pst_left))
			return;		/* fails outer operand check */
    if (!opa_notin_analyze(global, inner_root, inner_root->pst_left->pst_right))
			return;		/* fails inner operand check (subselect
					** RESDOM) */
    /* Borrow "not exists" check rtne to determine if there are nested
    ** subselects. This isn't allowed. */
    opa_notex_analyze(global, outer_root, inner_root,
	    inner_root->pst_right, &outer, &inner, &nested_subsel);
    if (nested_subsel) return;

    /* Subselect passes the test, now do the transform:
    **   - add "subselect_table.TID is null" to where clause
    **   - and the bopp comparison to subselect where clause
    **   - subselect where clause becomes outer select on clause
    **   - subselect tables become inner to outer tables
    **   - outer tables become outer to subselect tables
    **   - subselect pst_tvrm is ORed to outer select pst_tvrm
    */

    varno = BTnext((i4)-1, (PTR)&inner_root->pst_sym.pst_value.pst_s_root.
	pst_tvrm, BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* subselect table varno */
    if ((*nodep)->pst_sym.pst_type == PST_NOT)
	joinid = (*nodep)->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid;
    else joinid = (*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid;

    andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, joinid);
    andp->pst_left = opp = opv_opnode(global, PST_UOP, 
	(ADI_OP_ID)global->ops_cb->ops_server->opg_isnull, joinid);
					/* make UOP (isnull) to AND to outer
					** where clause */
    (*nodep) = andp;			/* splice and into where clause */
    opp->pst_left = opv_varnode(global, &tid_dv, varno, &tid_att);
					/* stick TID var under isnull */
    opp->pst_right = NULL;		/* finish up is null pred. */

    /* Have to resolve the new "is null". Normally, opa_resolve does
    ** this for OPF. But we don't have a subquery structure yet, and
    ** the subquery is required by opa_resolve. So we'll just hard
    ** code the pst_resolve call which opa_resolve does. */

    status = pst_resolve((PSF_SESSCB_PTR) NULL, 
	global->ops_adfcb, opp, 
	outer_root->pst_sym.pst_value.pst_s_root.pst_qlang, &error);
    if (status != E_DB_OK)
	opx_verror(status, E_OP0685_RESOLVE, error.err_data);

    /* Now add another AND which will include "NOT IN"/"<> ALL" comparison
    ** and remainder of subselect where clause. */
    andp->pst_right = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
    bopp->pst_right = inner_root->pst_left->pst_right;
					/* suck subsel resdom expr under 
					** "="/"<>" BOP */
    bopp->pst_sym.pst_value.pst_s_op.pst_opno = 
				global->ops_cb->ops_server->opg_eq;
					/* make it an "=" for on clause */
    bopp->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;

    status = pst_resolve((PSF_SESSCB_PTR) NULL, 
	global->ops_adfcb, bopp, 
	outer_root->pst_sym.pst_value.pst_s_root.pst_qlang, &error);
					/* resolve the new "=" */
    if (status != E_DB_OK)
	opx_verror(status, E_OP0685_RESOLVE, error.err_data);

    andp->pst_right->pst_left = bopp;   /* attach bop to and */
    andp->pst_right->pst_right = inner_root->pst_right;
					/* subselect where clause */

    /*
    ** The following 'global maps' are updated the first time through
    ** for one complete aghead:
    ** 1. The global rangetable pst_outer_rel and pst_inner_rel maps
    ** 2. The table map for the root of the query ('outer_root')
    ** We must make sure that it isn't done again if there are subsequent
    ** copies of the aghead, as there are when we have more than one
    ** aggregate operator. We can detect that this is the first time
    ** through if the joinid passed in to opa_notex_transform(), when
    ** incremented, is greater than the global joinid variable
    ** header->pst_numjoins. header->pst_numjoins in this context acts as
    ** a high water mark for the joinid numbering.
    */
    joinid = ++*pjoinid;

    /* Assign it to all connectives and relops in subsel where clause. */
    opa_subsel_joinid(global, andp->pst_right, joinid);
    global->ops_goj.opl_mask = OPL_OJFOUND;
    global->ops_goj.opl_glv = joinid;	/* set some global OJ stuff */

    if (joinid > header->pst_numjoins)
    {
	header->pst_numjoins = joinid;

	/* Loop over outer_root pst_tvrm, ORing joinid into their range table 
	** pst_outer_rel's. Then loop over inner_root pst_tvrm, ORing joinid
	** into their range table pst_inner_rel's. */

	for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
	{
	    PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	    BTset((i4)joinid, (PTR)&rngp->pst_outer_rel.pst_j_mask);
	}

	for (varno = -1; (varno = BTnext((i4)varno,
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    BITS_IN(inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm))) 
	    != -1; )
	{
	    PST_RNGENTRY	*rngp = header->pst_rangetab[varno];

	    BTset((i4)joinid, (PTR)&rngp->pst_inner_rel.pst_j_mask);
	}

	/* Finally, OR the inner_root pst_tvrm into the outer_root pst_tvrm. */

	BTor((i4)BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	outer_root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	    BTcount((PTR)&outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    BITS_IN(outer_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
					/* update from clause cardinality */
    }

    /* And really finally, if we're under an AGHEAD, OR the inner_root 
    ** pst_tvrm into the AGHEAD (root) pst_tvrm. */

    if (root->pst_sym.pst_type == PST_AGHEAD)
    {
	BTor((i4)BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm),
	    (PTR)&inner_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);
	root->pst_sym.pst_value.pst_s_root.pst_tvrc = 
	    BTcount((PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm,
		BITS_IN(root->pst_sym.pst_value.pst_s_root.pst_tvrm));
    }

    *gotone = TRUE;			/* tell the outside world */

}

/*{
** Name: opa_notin_analyze	- analyze "not in"/"<> all" subselect for
**	transformation potential
**
** Description:
**      This function examines the select list of the subselect for a 
**	column in the subselect from list, and the "not in"/"<>" comparand for 
**	a column in the outer select.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	root		ptr to root node of outer/inner select 
**	opp		ptr to comparand being checked
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE - if operand expression contains VAR in from clause
**	    FALSE - otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-aug-99 (inkdo01)
**	    Written.
**	25-oct-02 (inkdo01)
**	    Disallow "not in"s when both comparands are expressions (creates
**	    CP join instead of more efficient join).
**	30-mar-04 (hayke02)
**	    Back out the previous change and fix INGSRV 1957, bug 108930 in
**	    opz_fboth() instead. This change fixes problem INGSRV 2732, bug
**	    111899.
[@history_template@]...
*/
static bool
opa_notin_analyze(
	OPS_STATE	   *global,
	PST_QNODE	   *root,
	PST_QNODE	   *opp)
	
{


    /* This recursive function searches an operand expression (from the
    ** "not in"/"<>" comparison) to determine if it contains a column 
    ** reference to a table in the corresponding root/subsel node. 
    **
    ** The function is a big loop which switches on the operand node 
    ** type, looking for a PST_VAR which maps onto the appropriate
    ** from clause.
    */

    while (opp)
     switch (opp->pst_sym.pst_type) 
    {

      case PST_BOP:
	if (opa_notin_analyze(global, root, opp->pst_left))
		return(TRUE);		/* recurse down left side */

	opp = opp->pst_right;		/* iterate down right side */
	break;

      case PST_NOT:
      case PST_UOP:
	opp = opp->pst_left;		/* just iterate on NOT & UOP */
	break;

      case PST_CONST:
	return(FALSE);

      case PST_VAR:
	/* Check if column covers either outer or inner select. */
	if (BTtest((u_i2)opp->pst_sym.pst_value.pst_s_var.pst_vno, 
		(PTR)&root->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    return(TRUE);
	else return(FALSE);

      default:
	return(FALSE);			/* all others return empty */

    }
    return(FALSE);

}

/*{
** Name: opa_subsel_search	- search for "not exists" subselect
**
** Description:
**      Thus function searches a where clause parse tree fragment for 
**	a "not exists" subselect.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**	nodep		ptr to ptr to parse tree fragment being examined
**	header		ptr to parse tree header (PST_QTREE)
**	outer_root	ptr to root node of outer select 
**	root		ptr to root of fragment to transform (either PST_ROOT
**			or PST_AGHEAD)
**	pjoinid		ptr to a PST_J_ID variable that contains the joinid value
**			to be incremented and used for each new join created. Callers
**			can reset this to a base value deliberately so that repeated
**			sections of the tree get the same sequence of join ids.
**
** Outputs:
**      header		ptr to possibly modifed parse tree
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-aug-99 (inkdo01)
**	    Written.
**	18-jan-05 (inkdo01)
**	    Exclude "not in"/"<> ALL" subselects with nullable subselect 
**	    comparand. OJ doesn't handle nulls correctly. This could be 
**	    handled by including an OR'ed is null test to ON clause.
**	13-may-05 (inkdo01)
**	    Fix some sloppy keying of previous change.
**	16-may-05 (inkdo01)
**	    Changes to allow not exists transform under PST_AGHEAD.
**	16-mar-06 (dougi)
**	    New parm to accomodate multiple aggs in same containing query.
**      13-apr-07 (huazh01)
**          after transforming nodes under PST_AND nodes, further search
**          the updated tree and see if there are more nodes that are
**          suitable to be transformed into OJ. This fixes b117988.  
**	13-mar-08 (hayke02)
**	    Extend the 18-jan-05 (inkdo01) fix for bug 113772 to the other
**	    (left) side of the "not in"/"<> ALL" subselect. This change
**	    fixes bug 120034.
**	21-may-10 (smeke01) b123752
**	    Use new parameter joinid passed in by caller as basis for joinids
**	    for new joins. This enables the caller to make sure that the same
**	    joinid(s) is/are used for any repeat copies of the AGHEAD WHERE clause.
[@history_template@]...
*/
static VOID
opa_subsel_search(
	OPS_STATE	   *global,
	PST_QNODE	   **nodep,
	PST_QTREE	   *header,
	PST_QNODE	   *outer_root,
	PST_QNODE	   *root,
	bool		   *gotone,
	PST_J_ID	   *pjoinid)

{
    PST_QNODE	*nodep1;
    bool        local_gotone = FALSE; 


    /* Loop over where clause fragment, searching for one of the magic 
    ** sequences of PST_NOT->PST_UOP (exists)->PST_SUBSEL (the NOT EXISTS
    ** case), PST_NOT->PST_BOP (=)->PST_SUBSEL (the NOT = ANY case) or 
    ** PST_BOP (<>)->PST_SUBSEL (the <> ALL case - same as NOT = ANY).
    ** These guys must be conjunctive factors, so anything other than 
    ** PST_AND or one of the sequences will trigger a return. The loop 
    ** recurses on the left side of ANDs and iterates on the right. */

    while ((*nodep)) 
    {
	switch ((*nodep)->pst_sym.pst_type) {

	  case PST_NOT:
	    /* Look for magic sequence. */
	    if ((nodep1 = (*nodep)->pst_left)->pst_sym.pst_type == PST_UOP
		&&
		nodep1->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_exists
		&&
		nodep1->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN
		&&
		(nodep1 = nodep1->pst_left)->pst_sym.pst_type == PST_SUBSEL 
		&&
		nodep1->pst_sym.pst_value.pst_s_root.pst_union.pst_next == NULL)
	    {
		/* Ta, da! Now call analyzer to determine if this one can
		** be transformed, and do it. */
		opa_notex_transform(global, nodep, header, outer_root, 
			root, nodep1, gotone, pjoinid);
	    }
	    else if ((nodep1 = (*nodep)->pst_left)->pst_sym.pst_type == PST_BOP
		&&
		nodep1->pst_sym.pst_value.pst_s_op.pst_opno ==
			global->ops_cb->ops_server->opg_eq
		&&
		nodep1->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ANY
		&&
		nodep1->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN
		&&
		nodep1->pst_right->pst_sym.pst_type == PST_SUBSEL 
		&&
		nodep1->pst_right->pst_sym.pst_dataval.db_datatype > 0
		&&
		nodep1->pst_left->pst_sym.pst_dataval.db_datatype > 0
		&&
		nodep1->pst_right->pst_sym.pst_value.pst_s_root.
							pst_union.pst_next == NULL)
	    {
		/* This one is "... not c1 = any (select ..." which is the same
		** as "... c1 <> all (select ..." which is the same as
		** "... c1 not in (select ...". */
		opa_notin_transform(global, nodep, header, outer_root, 
			root, nodep1, gotone, pjoinid);
	    }
	    return;

	  case PST_BOP:
	    if ((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_ne
		&&
		(*nodep)->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ALL
		&&
		(*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN
		&&
		(*nodep)->pst_right->pst_sym.pst_type == PST_SUBSEL 
		&&
		(*nodep)->pst_right->pst_sym.pst_dataval.db_datatype > 0
		&&
		(*nodep)->pst_left->pst_sym.pst_dataval.db_datatype > 0
		&&
		(*nodep)->pst_right->pst_sym.pst_value.pst_s_root.
							pst_union.pst_next == NULL)
	    {
		/* This one is "... c1 NOT IN (select ..." which is the same as
		** "... c1 <> ALL (select ...". */
		opa_notin_transform(global, nodep, header, outer_root, 
			root, (*nodep), gotone, pjoinid);
	    }
	    return;

	  case PST_AND:
	    opa_subsel_search(global, &(*nodep)->pst_left, header, 
			outer_root, root, &local_gotone, pjoinid);
                                        /* recurse down left */

            /* b117988: 
            ** if got one, check if there are any other possible 
            ** nodes under the new nodep->pst_left that are suitable to
            ** be transformed.
            */
            if (local_gotone)
            {
               *gotone = local_gotone; 
               do
               {
                  local_gotone = FALSE; 
                  opa_subsel_search(global, &(*nodep)->pst_left, header, 
                                    outer_root, root, &local_gotone, pjoinid);
               }
               while (local_gotone);
            }

	    nodep = &(*nodep)->pst_right;
					/* iterate down right */
	    continue;
	default:
	    break;
	}    /* end of switch */

	break;				/* everything else exits loop */
    }

}

/*{
** Name: opa_subsel_to_oj1	- perform NOT EXISTS/NOT IN subselect
**	to oj transformations
**
** Description:
**      Thus function examines the current query's parse tree for 
**	qualifying NOT EXISTS, NOT IN and <> ALL subselects, and oversees
**	their transformation to equivalent outer joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**
** Outputs:
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-aug-99 (inkdo01)
**	    Written.
**	16-may-05 (inkdo01)
**	    Changes to support transform even under AGHEADs.
**	16-mar-06 (dougi)
**	    Aggregate queries with multiple agg funcs in the SELECT list
**	    generate a WHERE clause per agg func and that confuses the 
**	    transform. Only increment the OJ joinid once.
**	8-Jan-2010 (kschendel) b123126
**	    121883 change broke stack-empty check, fix.
**	21-may-10 (smeke01) b123752
**	    Make sure that the same joinid(s) is/are used for any 
**	    repeat copies of the AGHEAD WHERE clause.
*/
static VOID
opa_subsel_to_oj1(
	OPS_STATE	*global,
	PST_QNODE	*outer_root,
	bool		*gotone)

{
    PST_QTREE	*header = global->ops_qheader;
    PST_J_ID	joinid, save_joinid;

    joinid = header->pst_numjoins;
    opa_subsel_search(global, &outer_root->pst_right, header, outer_root, 
				outer_root, gotone, &joinid);
					/* check where clause for subsels */

    /* Aggregate queries hide the where clause under the AGHEAD node
    ** under the RESDOM. This loop checks the RESDOMs for simple
    ** aggregate queries with "not exists"/"not in" subsels. */
    /* Because of its Quel roots, Ingres builds one AGHEAD for each
    ** aggregate function and, in SQL, each has the same BYHEAD 
    ** subtree (defining the GROUP BY cols) and the same rhs (its
    ** WHERE clause). opa later consolidates these into a single
    ** subquery, but here it causes problems for the OJ transform.
    ** We need to make sure that the same joinid(s) is/are used for
    ** the repeat copies of the AGHEAD WHERE clause. We do this by
    ** saving the starting value of header->pst_numjoins, and passing
    ** the value in a write parameter so that it can be incremented
    ** as required by later functions. These functions also handle
    ** incrementing header->pst_numjoins. */

    save_joinid = joinid = header->pst_numjoins;
    if (outer_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
    {
	OPV_STK stk;
	PST_QNODE *resdom;

	OPV_STK_INIT(stk, global);
	for (resdom = outer_root->pst_left;
		resdom && resdom->pst_sym.pst_type == PST_RESDOM;
		resdom = resdom->pst_left)
	{
	    PST_QNODE **nodep = &resdom->pst_right, *node;
	    bool descend = TRUE;

	    while (nodep && (node = *nodep))
	    {
                if (descend)
                {
		    if (node->pst_sym.pst_type == PST_CONST ||
			node->pst_sym.pst_type == PST_VAR ||
			node->pst_sym.pst_type == PST_BYHEAD ||
			node->pst_sym.pst_type == PST_SUBSEL)
		    {
			/* Ensure these don't descend. */
		    }
                    else if (node->pst_left)
                    {
                        /* Delay node evaluation */
                        opv_push_node(&stk, nodep);
                        if (node->pst_right)
                        {
                            /* Delay RHS */
                            opv_push_node(&stk, &node->pst_right);
                            if (node->pst_right->pst_right ||
                                        node->pst_right->pst_left)
                                /* Mark that the top node needs descending */
                                opv_push_node(&stk, OPV_DESCEND_MARK);
                        }
                        nodep = &node->pst_left;
                        continue;
                    }
                    else if (node->pst_right)
                    {
                        /* Delay node evaluation */
                        opv_push_node(&stk, nodep);
                        nodep = &node->pst_right;
                        continue;
                    }
                }

		switch (node->pst_sym.pst_type)
		{
		case PST_AGHEAD:
		    opa_subsel_search(global, &node->pst_right, 
				header, outer_root, node, gotone, &joinid);
		    /* make sure that subsequent copies of the aggregate start at the same join id */
		    joinid = save_joinid; 
		    break;
		case PST_SUBSEL:
		    opa_subsel_to_oj1(global, node, gotone);
		    break;
		default:
		    break;
		}
                nodep = opv_pop_node(&stk);
		if (nodep == NULL)
		    break;
                if (descend = (nodep == OPV_DESCEND_MARK))
                   nodep = opv_pop_node(&stk);
	    }
	    opv_pop_all(&stk);
	}
    }
}

/*{
** Name: opa_subsel_to_oj	- perform NOT EXISTS/NOT IN subselect
**	to oj transformations
** Name: opa_process_one_qtree	- perform NOT EXISTS/NOT IN subselect
**	to oj transformations on one query tree fragment
**
** Description:
**      Thus function examines one query parse tree for 
**	qualifying NOT EXISTS, NOT IN and <> ALL subselects, and oversees
**	their transformation to equivalent outer joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**      root		ptr to the root node of the query parse tree 
**
** Outputs:
**      gotone		write-back boolean to indicate to caller whether 
**      		at least one transformation was performed.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-feb-10 (smeke01) b122066
**	    Split out from opa_subsel_to_oj. 
[@history_template@]...
*/
static
VOID
opa_process_one_qnode(
	OPS_STATE	   *global,
	PST_QNODE	   *root,
	bool		   *gotone)
{
    PST_QNODE	*outer_root;

    /* This function simply loops over the union'ed selects in the query's
    ** parse tree, calling the transformation functions for each. */
    for (outer_root = root;
	outer_root && outer_root->pst_sym.pst_type == PST_ROOT;
	outer_root = outer_root->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {
	opa_subsel_to_oj1(global, outer_root, gotone);
    }
}


/*{
** Name: opa_subsel_to_oj	- perform NOT EXISTS/NOT IN subselect
**	to oj transformations on all appropriate query tree fragments.
**
** Description:
**      Thus function examines the current query's parse tree(s) for 
**	qualifying NOT EXISTS, NOT IN and <> ALL subselects, and oversees
**	their transformation to equivalent outer joins.
**
** Inputs:
**      global		ptr to OPS_GLOBAL structure
**
** Outputs:
**	gotone		ptr to switch - TRUE means we did a transform
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-aug-99 (inkdo01)
**	    Written.
**	16-may-05 (inkdo01)
**	    Changes to support transform even under AGHEADs.
**	16-mar-06 (dougi)
**	    Aggregate queries with multiple agg funcs in the SELECT list
**	    generate a WHERE clause per agg func and that confuses the 
**	    transform. Only increment the OJ joinid once.
**	19-feb-10 (smeke01) b122066
**	    Also check (and transform if applicable) any query tree 
**	    fragment(s) held in the range table (ie as happens with view 
**	    handling). In order to be able to do this, the union-select 
**	    for-loop part of opa_subsel_to_oj has been split out into a 
**	    separate utility function opa_process_one_qnode.
**	11-mar-10 (smeke01) b123408
**	    Also check (and transform if applicable) derived table query
**	    tree fragments.
[@history_template@]...
*/
VOID
opa_subsel_to_oj(
	OPS_STATE	   *global,
	bool		   *gotone)

{
    PST_QTREE	*header = global->ops_qheader;
    int i;

    *gotone = FALSE;			/* init */

    /* Process the 'main query' */
    opa_process_one_qnode( global, header->pst_qtree, gotone); 
    /* Process any query fragments held in the range table (views) */
    if (header->pst_rngvar_count > 0 && header->pst_rangetab != NULL)
    {
	for (i = header->pst_rngvar_count - 1; i >= 0; i--)
	{
	    PST_RNGENTRY *rngentry = header->pst_rangetab[i];
	    if (rngentry != NULL && 
		(rngentry->pst_rgtype == PST_RTREE || rngentry->pst_rgtype == PST_DRTREE))
		opa_process_one_qnode( global, rngentry->pst_rgroot, gotone); 
	}
    }
}

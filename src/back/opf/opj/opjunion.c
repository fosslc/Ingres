/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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

/* external routine declarations definitions */
#define        OPJ_UNION      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPJUNION.C - contains routines used to optimize unions
**
**  Description:
**      Routines used to optimize unions, union views 
**
**
**  History:    
**      28-jun-89 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	01-Oct-2001 (hanal04) Bug 105464 INGSRV 1517
**	    If we copy query trees from a non-CNF subquery to a
**	    CNF subquery we must mark the target subquery as
**	    non-CNF to avoid E_OP0681 errors in OPC.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static void opj_uvar(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qual,
	OPV_IVARS varno,
	DB_ATT_ID *attidp);
static void opj_utree(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qual,
	OPV_IVARS varno);
static bool opj_ctree(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qual,
	OPV_IVARS varno,
	OPS_SUBQUERY *usubquery,
	PST_QNODE *uqual);
void opj_union(
	OPS_SUBQUERY *subquery);
static PST_QNODE **opj_subboolfact(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qualpp);
void opj_uboolfact(
	OPS_SUBQUERY *subquery);

/*{
** Name: opj_uvar	- get the attribute number of the union view var node
**
** Description:
**      This routine will calculate the attribute number of the union view 
**      given the equivalence class of this var node 
**
** Inputs:
**      subquery                        ptr to subquery containing equivalence
**					class of var node
**      qual                            ptr to var node
**
** Outputs:
**      attidp                          ptr to attribute ID for union view
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opj_uvar(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qual,
	OPV_IVARS	   varno,
	DB_ATT_ID	   *attidp)
{
    OPZ_BMATTS	    *attrmap;	/* bit map of attributes in the
			    ** equivalence class */
    OPZ_IATTS	    attno;  /* attribute currently being looked at */
    OPZ_IATTS	    maxattr; /* maximum attribute */
    OPZ_AT	    *abase;

    maxattr = subquery->ops_attrs.opz_av;
    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
				** to joinop attributes */
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist
	[subquery->ops_attrs.opz_base->opz_attnums
	    [qual->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
	    ]->opz_equcls
	]->ope_attrmap;
    for (attno = -1; 
	 (attno = BTnext((i4)attno, (char *)attrmap, maxattr)
	 ) >= 0;)
    {
	if (abase->opz_attnums[attno]->opz_varnm == varno)
	{   /* found the union view attribute number so copy it in */
	    STRUCT_ASSIGN_MACRO(abase->opz_attnums[attno]->opz_attnm, *attidp);
	    break;
	}
    }
    if (attno < 0)
	opx_error(E_OP0384_NOATTS); /* attribute not found when
			    ** expected */
}

/*{
** Name: opj_utree	- convert qualification to union variable attributes
**
** Description:
**      This routine will traverse the qualification and convert all the var nodes 
**      to use var-number 0, and attribute number of the union view.  This normalized 
**      form can be used for comparision of qualifications and copying into various 
**      parts of the union view 
**
** Inputs:
**      subquery                        ptr to subquery containing original qualification
**      qual                            current node to be converted
**	varno				var number of union view
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opj_utree(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qual,
	OPV_IVARS	   varno)
{

    for (;qual; qual = qual->pst_right)
    {
	if (qual->pst_sym.pst_type == PST_VAR)
	{
	    opj_uvar(subquery, qual, varno, 
		&qual->pst_sym.pst_value.pst_s_var.pst_atno);
	    qual->pst_sym.pst_value.pst_s_var.pst_vno = 0; /* use same
		** variable number, will be ignored */
	}
	if (qual->pst_left)
	    opj_utree(subquery, qual->pst_left, varno);
    }
}

/*{
** Name: opj_ctree	- compare tree for purposes of "union equality"
**
** Description:
**      This routine will return TRUE if the qualifications are 
**      structurally equivalent, and reference the same var nodes 
**
** Inputs:
**      subquery                        ptr to subquery containing qual
**      qual                            qual to compare
**      usubquery                       ptr to union subquery
**      uqual                           ptr to qual in union subquery
**
** Outputs:
**	Returns:
**	    TRUE if qualifications are equivalent 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opj_ctree(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qual,
	OPV_IVARS	   varno,
	OPS_SUBQUERY       *usubquery,
	PST_QNODE          *uqual)
{
    for (;qual; qual = qual->pst_right, uqual = uqual->pst_right)
    {
	if (!uqual)
	    break;
	if (qual->pst_sym.pst_type == PST_VAR)
	{
	    DB_ATT_ID	attid;

	    if (uqual->pst_sym.pst_type != PST_VAR)
		return(FALSE);
	    opj_uvar(subquery, qual, varno, &attid); /* get the
				    ** union view attribute */
	    return (attid.db_att_id 
		== 
		uqual->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id);
				    /* make sure the same attribute number
				    ** is referenced */
	}
	else
	{   /* use regular comparision routine for all other nodes */
	    PST_QNODE	*lqual;
	    PST_QNODE	*rqual;
	    PST_QNODE	*luqual;
	    PST_QNODE	*ruqual;
	    bool	ret_val;

	    lqual = qual->pst_left;
	    rqual = qual->pst_right;
	    qual->pst_left = (PST_QNODE *)NULL;
	    qual->pst_right = (PST_QNODE *)NULL;
	    luqual = uqual->pst_left;
	    ruqual = uqual->pst_right;
	    uqual->pst_left = (PST_QNODE *)NULL;
	    uqual->pst_right = (PST_QNODE *)NULL;
	    ret_val = opv_ctrees(subquery->ops_global, qual, uqual);
	    qual->pst_left= lqual;
	    qual->pst_right = rqual;
	    uqual->pst_left= luqual;
	    uqual->pst_right = ruqual;
	    if (!ret_val)
		return(FALSE);
	}
	if (qual->pst_left)
	{
	    if (!opj_ctree(subquery, qual->pst_left, varno, usubquery, uqual->pst_left))
		return(FALSE);
	}
    }
    return(qual == uqual);
}

/*{
** Name: opj_union	- optimize unions
**
** Description:
**      The routine determines which qualifications of a parent query 
**      can be copied to the union, so that the union can be more restrictive 
**      or partitions of the union can be eliminated entirely in some cases. 
**
** Inputs:
**      subquery                        ptr to subquery be analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jun-89 (seputis)
**          initial creation
**	01-Oct-2001 (hanal04) Bug 105464 INGSRV 1517
**	    After calling opv_copytree() mark the target subquery
**	    as non-CNF if the source suquery was non-CNF.
**	11-may-05 (inkdo01)
**	    Don't copy ON clause BFs to union subqueries - fixes 114493.
**	20-apr-07 (hayke02)
**	    Limit the fix for bug 114493 (test for a OPL_NOOUTER pst_joinid)
**	    to those subqueries containing outer joins, otherwise pst_joinid
**	    will be PST_NOJOIN (0). This change fixes bug 118088.
**      27-mar-2009 (huazh01)
**          don't copy OPZ_FANUM, OPZ_SNUM, OPZ_CORRELATED attribute into 
**          'unionbm'. Doing so will cause opj_utree() to convert the  
**          union_subquery->ops_sunion.opu_qual qualification into a 
**          PST_QNODE node containing negative attribute id, and causes 
**          E_OP0384 error. (bug 121602)
[@history_template@]...
*/
VOID
opj_union(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS	    lvar;
    for (lvar = subquery->ops_vars.opv_prv; --lvar>=0;)
    {	/* look at the union views referenced in this subquery */
	OPV_VARS    *unionp;
	unionp = subquery->ops_vars.opv_base->opv_rt[lvar];
	if (unionp->opv_grv 
	    && 
	    (unionp->opv_grv->opv_created == OPS_VIEW)
	    &&
	    unionp->opv_grv->opv_gquery
	    &&
	    unionp->opv_grv->opv_gquery->ops_union
	    )
	{   /* a union view has been found, so search the list of boolean
	    ** factors to determine which terms can be placed into the qualifications
	    ** of the union */
	    OPE_BMEQCLS	    unionbm;	/* bitmap of the equivalence classes which
					** are available in the union view */
            OPS_SUBQUERY    *union_subquery;
	    OPB_BMBF	    bfbm;
	    OPB_IBF	    maxbf;	/* current boolean factor number */
	    		    
	    maxbf = BITS_IN(bfbm);
	    union_subquery = unionp->opv_grv->opv_gquery;

	    if (union_subquery->ops_sunion.opu_mask & OPU_QUAL)
	    {
		if (union_subquery->ops_sunion.opu_qual == (PST_QNODE *)NULL)
		    continue;		/* since entire union is required by
					** another subquery, no need to proceed */
		MEfill(sizeof(bfbm), (u_char)0, (PTR)&bfbm);
		BTnot((i4)BITS_IN(bfbm), (char *)&bfbm); /* bits will get
					** reset as identical qualifications are 
					** found */
	    }
	    MEfill(sizeof(unionbm), (u_char)0, (PTR)&unionbm);
	    {	/* search thru list of attributes and mark 
		** appropriate equivalence classes*/
		/* opv_eqcmp not initialized yet */
		OPZ_IATTS	attr;
		OPZ_AT		*abase;
		abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
		for (attr = subquery->ops_attrs.opz_av; --attr>=0;)
		{   /* opv_eqcmp not initialized yet */
		    OPZ_ATTS	    *attrp;
		    attrp = abase->opz_attnums[attr];
		    if ((attrp->opz_varnm == lvar)
			&&
			(attrp->opz_equcls >= 0)
                        &&
                        attrp->opz_attnm.db_att_id >= 0
                       )
			BTset ((i4)attrp->opz_equcls, (char *)&unionbm);
		}
	    }
	    {	/* search list of conjuncts, boolean factor structure has
		** not been created at this point */
		OPB_IBF	    total_bfs;	/* number of qualifications placed into
					** union view list */
		PST_QNODE   *qual;

		total_bfs = 0;
		for (qual = subquery->ops_root->pst_right;
		    qual->pst_sym.pst_type != PST_QLEND;
		    qual = qual->pst_right)
		{
		    OPE_BMEQCLS	    qual_eqcmp;
		    MEfill(sizeof(qual_eqcmp), (u_char)0, (PTR)&qual_eqcmp);
		    ope_aseqcls(subquery, &qual_eqcmp, qual->pst_left);
					/* assign
                                        ** equivalence classes for vars in
                                        ** the qualification which have not
                                        ** been assigned yet
                                        */
		    if (BTsubset((char *)&qual_eqcmp, (char *)&unionbm, (i4)BITS_IN(unionbm))
			&&
			!(subquery->ops_oj.opl_base &&
			(qual->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid
			!= OPL_NOOUTER)))
		    {	/* this boolean factor contains equivalence classes which
			** are all in the union view, so it can be evaluated inside
			** the union view */
			if (union_subquery->ops_sunion.opu_mask & OPU_QUAL)
			{   /* qualification must already be in the list
			    ** so compare to existing list of entries */
			    PST_QNODE	    *old_qual;
			    OPB_IBF	    bfcount;
			    bfcount = 0;
			    for (old_qual = union_subquery->ops_sunion.opu_qual;
				old_qual; old_qual = old_qual->pst_right)
			    {
				if (BTtest((i4)bfcount, (char *)&bfbm) /* if this
						    ** boolean factor has not been
						    ** matched */
				    &&
				    opj_ctree(subquery, qual->pst_left, lvar,
					union_subquery, old_qual->pst_left) /*
						    ** and if the qualification
						    ** matches the current one */
				    )
				{   /* found a match so mark the boolean
				    ** factor as being in common */
				    BTclear((i4)bfcount, (char *)&bfbm);
				    break;
				}
				bfcount++;
			    }
			}
			else
			{   /* if this is the first time then just copy the
			    ** qualification and place into the list */
			    PST_QNODE	    *and_node;
			    total_bfs++;
			    if (total_bfs >= (i4)BITS_IN(unionbm))
				break;		    /* cannot add anymore qualifications
						    ** since the max boolean factor
						    ** count has been reached */
			    and_node = opv_opnode(subquery->ops_global, PST_AND, (ADI_OP_ID)0,
				(OPL_IOUTER)OPL_NOOUTER);
			    and_node->pst_left = qual->pst_left;
			    opv_copytree(subquery->ops_global, &and_node->pst_left);
			    and_node->pst_right = union_subquery->ops_sunion.opu_qual;
			    union_subquery->ops_sunion.opu_qual = and_node;
                            if(subquery->ops_mask & OPS_NONCNF)
                            {
				union_subquery->ops_mask |= OPS_NONCNF;
			    }

			}
		    }
		}
	    }
	    if (union_subquery->ops_sunion.opu_mask & OPU_QUAL)
	    {	/* previous qualification exists so need to get common qualifications,
		** by using boolean factor bitmap of those qualifications which are
		** to be eliminated */
		OPB_IBF		bfno;
		PST_QNODE	**qualpp;
		OPB_IBF		current;

		current = 0;
		qualpp = &union_subquery->ops_sunion.opu_qual;
		for (bfno = -1; (bfno = BTnext((i4)bfno, (char *)&bfbm, (i4)maxbf)) >= 0;)
		{
		    while ((current < bfno) && *qualpp)
		    {	/* find next qualification to eliminate */
			current++;
			qualpp = &(*qualpp)->pst_right;
		    }
		    if (*qualpp)
			*qualpp = (*qualpp)->pst_right; /* remove this qual since
					    ** no match was found for this subquery */
		    else
			break;		    /* end of qual list reached */
		}
	    }
	    else
	    {	/* this is the first qualification in the list, so it can be applied */
		union_subquery->ops_sunion.opu_mask |= OPU_QUAL;
		if (union_subquery->ops_sunion.opu_qual)
		    opj_utree(subquery, union_subquery->ops_sunion.opu_qual, lvar); /* 
					    ** convert the 
					    ** qualification to reference var nodes
					    ** of the union view, so that comparisons
					    ** can be made later */
	    }
	}
    }
}

/*{
** Name: opj_subboolfact	- substitute union target list for var nodes
**
** Description:
**      This routine will traverse the query tree and substitute the var nodes 
**      for the union view, with the target list element in the union subquery 
**
** Inputs:
**      subquery                        ptr to union subquery
**      qual                            ptr to qualification to substitute
**
** Outputs:
**	Returns:
**	    PST_QNODE, of the last conjunct in the qualification
**	Exceptions:
**
** Side Effects:
**
** History:
**      28-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static PST_QNODE **
opj_subboolfact(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qualpp)
{
    for(;*qualpp; qualpp = &(*qualpp)->pst_right)
    {
	PST_QNODE	*qual;

	qual = *qualpp;
	if (qual->pst_sym.pst_type == PST_VAR)
	{   /* this should be a VAR node which represents a target
	    ** list element of the union */
	    OPZ_IATTS		atno;
	    PST_QNODE		*resdomp;

	    if (qual->pst_sym.pst_value.pst_s_var.pst_vno)
		opx_error( E_OP0387_VARNO); /* by convention this tree
				** should only have var nodes which reference
				** variable 0 */
	    atno = qual->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    for (resdomp = subquery->ops_root->pst_left;
		resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
		resdomp = resdomp->pst_left)
	    {
		if (resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno == atno)
		{
		    *qualpp = resdomp->pst_right;
		    opv_copytree(subquery->ops_global, qualpp); /* make a copy of
					    ** the resdom expression for the qualification */
		    return((PST_QNODE **) NULL);
		}
	    }
	    opx_error(E_OP0384_NOATTS); /* expected to find attribute
					** in union view target list */
	}
	if (qual->pst_left)
	    (VOID)opj_subboolfact(subquery, &qual->pst_left);
    }
    return (qualpp);
}

/*{
** Name: opj_uboolfact	- copy boolean factors into all parts of the union
**
** Description:
**      A previous pass of all the subqueries determined the equivalence classes 
**      and that some conjuncts could be evaluated in the union view instead of 
**      the parent query.  This routine will attach this qualification list to
**	all parts of the union. 
**
** Inputs:
**      subquery                        ptr to beginning of union list
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-jun-89 (seputis)
**          initial creation
**      7-dec-93 (ed)
**          b56139 - remove unused attributes from union views, i.e.
**		create a projection
**	01-Oct-2001 (hanal04) Bug 105464 INGSRV 1517
**	    After calling opv_copytree() or opj_subboolfact() mark the
**	    target subquery as non-CNF if the source subquery was non-CNF.
[@history_template@]...
*/
VOID
opj_uboolfact(
	OPS_SUBQUERY       *subquery)
{
    OPS_SUBQUERY	*unionp;
    OPV_GRV		*grvp;
    for (unionp = subquery->ops_next; 
	(unionp->ops_sqtype == OPS_UNION)
	&&
	unionp->ops_union; 
	unionp = unionp->ops_next)
	;				/* search for the first union
					** element of the list */
    subquery = unionp;
    grvp = subquery->ops_global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry];
    if ((   !subquery->ops_sunion.opu_qual 
	    &&
	    !(grvp->opv_gmask & OPV_UVPROJECT)
	)
	|| 
	(subquery->ops_sunion.opu_mask & OPU_PROCESSED))
	return;				/* return if no qualifications
					** exist to propagate */
    subquery->ops_sunion.opu_mask |= OPU_PROCESSED;

    for (; unionp; unionp=unionp->ops_union)
    {
	PST_QNODE	*qual;

	qual = subquery->ops_sunion.opu_qual;
	if (qual)
	{
	    PST_QNODE	**qualpp;
	    if (unionp->ops_union)
		/* if there is still another union then make a copy
		** of the query tree, so that the original can be used
		** for subsequent unions */
		opv_copytree(subquery->ops_global, &qual);
	    qualpp = opj_subboolfact(unionp, &qual);
	    unionp->ops_sunion.opu_mask |= OPU_EQCLS;
	    if(subquery->ops_mask & OPS_NONCNF)
	    {
		unionp->ops_mask |= OPS_NONCNF;
	    }
#if 0
	    /* cannot assign equivalence classes here since the ope_ebase array
	    ** does not have any more room */
	    ope_aseqcls(unionp, (OPE_BMEQCLS *) NULL, qual); /* assign
					** equivalence classes for vars in
					** the qualification which have not
					** been assigned yet
					*/
#endif
	    *qualpp = unionp->ops_root->pst_right;
	    unionp->ops_root->pst_right = qual;
	}
	unionp->ops_gentry = subquery->ops_gentry;
	if (grvp->opv_gmask & OPV_UVPROJECT)
	{   /* remove attributes from target list which are not needed
	    ** by parent query */
	    PST_QNODE	**qnodepp;	/* used to traverse resdom list
					** to create projection */
	    PST_QNODE	*savep;		/* save at least one resdom in
					** case entire list is eliminated */
	    bool	insert_resdom;	/* TRUE if all resdoms are eliminated */
	    savep = NULL;
	    insert_resdom = TRUE;
	    for (qnodepp = &unionp->ops_root->pst_left; 
		(*qnodepp) && ((*qnodepp)->pst_sym.pst_type == PST_RESDOM);)
	    {
		if ((*qnodepp)->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT
		    &&
		    !BTtest((i4)(*qnodepp)->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
			(char *)grvp->opv_attrmap))
		{
		    savep = *qnodepp;
		    *qnodepp = (*qnodepp)->pst_left;	/* eliminate unneeded
					    ** resdom */
		}
		else
		{
		    qnodepp = &(*qnodepp)->pst_left; /* resdom needed by parent */
		    insert_resdom = FALSE;
		}
	    }
	    if (insert_resdom && savep)
	    {	/* make sure at least one printing resdom remains */
		savep->pst_left = unionp->ops_root->pst_left;
		unionp->ops_root->pst_left = savep;
		savep->pst_right = opv_i1const(subquery->ops_global, 0);
		STRUCT_ASSIGN_MACRO(savep->pst_right->pst_sym.pst_dataval, 
		    savep->pst_sym.pst_dataval); /* create i1 constant since only
					    ** a place holder is needed and the
					    ** parent query does not reference
					    ** this attribute so change it to a
					    ** one byte integer constant */
		savep->pst_sym.pst_dataval.db_data = NULL;
	    }
	}
    }
}

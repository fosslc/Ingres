/*
**Copyright (c) 2006, 2010 Ingres Corporation
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
#define        OPN_HINT      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNHINT.C	- return conditions related to table level hints
**
**  Description:
**	Routines which return conditions concerning table level optimizer
**	hints and the current subquery
**
**  History:
**      21-mar-06 (dougi)
**	    Written for the optimizer hints project.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opn_index_hint(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES combination,
	OPN_LEAVES numleaves);
bool opn_nonkj_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);
bool opn_fsmCO_hint(
	OPS_SUBQUERY *subquery,
	OPO_CO *nodep);
bool opn_kj_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	bool *lkjonly,
	bool *rkjonly);
bool opn_order_hint(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);

/*{
** Name: opn_index_hint	- check current table/index combination against
**	index hints
**
** Description:
**	Valid combinations of tables/indexes must include both table
**	and index for any index hint in subquery hint array.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      combination			current combination of tables/indexes
**      numleaves                       number of tables/indexes in current
**					combination
**
** Outputs:
**	Returns:
**	    TRUE if combination satisfies all relevant index hints
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-mar-06 (dougi)
**	    Written for optimzer hints project.
*/
bool
opn_index_hint(
	OPS_SUBQUERY       *subquery,
	OPN_STLEAVES	   combination,
	OPN_LEAVES	   numleaves)
{
    i4			i, j;
    OPS_TABHINT		*hintp;
    OPV_VARS		*varp;
    OPV_RT		*vbase = subquery->ops_vars.opv_base;
    bool		gotone;


    /* Search the hint list for index hints. Then search the current
    ** combination for the table in the hint. Then search the current
    ** combination for the index in the hint. If the table is there and
    ** the index isn't, return FALSE. Otherwise return TRUE. */

    for (i = 0, hintp = subquery->ops_hints; i < subquery->ops_hintcount;
						i++, hintp++)
    {
	if (hintp->ops_hintcode != PST_THINT_INDEX)
	    continue;			/* only index hints */

	for (j = 0, gotone = FALSE; j < numleaves && !gotone; j++)
	{
	    varp = vbase->opv_rt[combination[j]];
	    if (MEcmp((char *)&hintp->ops_name1,
		(char *)&varp->opv_grv->opv_relation->rdr_rel->tbl_name,
		sizeof(DB_TAB_NAME)) == 0)
		gotone = TRUE;
	}

	if (!gotone)
	    continue;			/* hint table isn't in combo */

	for (j = 0, gotone = FALSE; j < numleaves && !gotone; j++)
	{
	    varp = vbase->opv_rt[combination[j]];
	    if (MEcmp((char *)&hintp->ops_name2,
		(char *)&varp->opv_grv->opv_relation->rdr_rel->tbl_name,
		sizeof(DB_TAB_NAME)) == 0)
		gotone = TRUE;
	}

	if (!gotone)
	    return(FALSE);		/* didn't find the index when 
					** table IS in the combination */
    }

    return(TRUE);

}

/*{
** Name: opn_nonkj_hint	- check join tree fragment for relevant non-key
**	join hints
**
** Description:
**	Check current join tree fragment for non Kjoin hints in which one
**	table is in left subtree and other is in right subtree. 
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      nodep				ptr to join tree fragment to examine
**
** Outputs:
**	Returns:
**	    TRUE - if at least one hint has tables in left and right subtrees
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-06 (dougi)
**	    Written for optimzer hints project.
*/
bool
opn_nonkj_hint(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*nodep)
{
    i4			i, j;
    OPS_TABHINT		*hintp;
    bool		gotone;


    /* Search the hint list for non-Kjoin hints. */

    for (i = 0, hintp = subquery->ops_hints; i < subquery->ops_hintcount;
						i++, hintp++)
    {
	if (hintp->ops_hintcode != PST_THINT_FSMJ &&
	    hintp->ops_hintcode != PST_THINT_PSMJ &&
	    hintp->ops_hintcode != PST_THINT_HASHJ)
	    continue;			/* only merge/hash join hints */

	/* See if vno1 is in left child and vno2 is in right child. */
	for (j = 0, gotone = FALSE; j < nodep->opn_child[0]->opn_nleaves && 
			!gotone; j++)
	 if (hintp->ops_vno1 == nodep->opn_child[0]->opn_prb[j])
	    gotone = TRUE;
	if (gotone)
	{
	    /* vno1 is in left - check vno2 in right. */
	    for (j = 0, gotone = FALSE; j < nodep->opn_child[1]->
			opn_nleaves && !gotone; j++)
	     if (hintp->ops_vno2 == nodep->opn_child[1]->opn_prb[j])
		return(TRUE);		/* got a match - return TRUE */

	    continue;			/* hint doesn't match */
	}

	/* Try reverse - vno1 in right child, vno2 in left child. */
	for (j = 0, gotone = FALSE; j < nodep->opn_child[0]->opn_nleaves && 
			!gotone; j++)
	 if (hintp->ops_vno2 == nodep->opn_child[0]->opn_prb[j])
	    gotone = TRUE;
	if (gotone)
	{
	    /* vno2 is in left - check vno1 in right. */
	    for (j = 0, gotone = FALSE; j < nodep->opn_child[1]->
			opn_nleaves && !gotone; j++)
	     if (hintp->ops_vno1 == nodep->opn_child[1]->opn_prb[j])
		return(TRUE);		/* got a match - return TRUE */
	}

    }
    return(FALSE);

}

/*{
** Name: opn_fsmCO_hint	- check CO node for relevant FSM join hint
**
** Description:
**	Check current CO node to see if a FSM join hint is relevant.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      cop				ptr to CO tree fragment to examine
**
** Outputs:
**	Returns:
**	    TRUE - if at least one hint has tables in left and right subtrees
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-06 (dougi)
**	    Written for optimzer hints project.
*/
bool
opn_fsmCO_hint(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*nodep)
{
    return FALSE;
}

/*{
** Name: opn_kj_hint	- check join tree fragment for relevant Kjoin
**	hint
**
** Description:
**	Check current join tree fragment for a Kjoin hint in which 
**	one table is in outer subtree and other is the single inner subtree
**	node
**
** Inputs:
**      subquery		ptr to current subquery being analyzed
**      nodep			ptr to join tree fragment to examine
**	lkjonly, rkjonly	ptrs to bools returning hint relevance
**
** Outputs:
**	Returns:
**	TRUE - if current node has relevant kjoin hint. If neither lkjonly
**	    nor rkjonly are set, this means the join order should be skipped
**	    since it will produce a merge/hash join that might override the
**	    key join
**	FALSE - otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-06 (dougi)
**	    Written for optimzer hints project.
*/
bool
opn_kj_hint(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*nodep,
	bool		*lkjonly,
	bool		*rkjonly)
{

    i4			i, j;
    OPS_TABHINT		*hintp;
    bool		gotone, retval;


    /* Search the hint list for Kjoin hints in which one table is in
    ** the outer and the other is the (only) inner. Flags are set
    ** accordingly. */

    *lkjonly = *rkjonly = retval = FALSE;

    for (i = 0, hintp = subquery->ops_hints; i < subquery->ops_hintcount;
						i++, hintp++)
    {
	if (hintp->ops_hintcode != PST_THINT_KEYJ)
	    continue;			/* only key join hints */

	/* See if vno1 is in left child and vno2 is the right child. */
	for (j = 0, gotone = FALSE; j < nodep->opn_child[0]->opn_nleaves && 
			!gotone; j++)
	 if (hintp->ops_vno1 == nodep->opn_child[0]->opn_prb[j])
	    gotone = TRUE;
	if (gotone)
	{
	    /* vno1 is in left - check vno2 in right. */
	    for (j = 0, gotone = FALSE; j < nodep->opn_child[1]->
			opn_nleaves && !gotone; j++)
	     if (hintp->ops_vno2 == nodep->opn_child[1]->opn_prb[j])
	     {
		if (nodep->opn_child[1]->opn_nleaves == 1)
		    *lkjonly = TRUE;
		break;
	     }
	    if (nodep->opn_child[0]->opn_nleaves == 1)
		retval = TRUE;		/* hint is still relevant */

	}

	/* Try reverse - vno1 in right child, vno2 in left child. */
	for (j = 0, gotone = FALSE; j < nodep->opn_child[1]->opn_nleaves && 
			!gotone; j++)
	 if (hintp->ops_vno1 == nodep->opn_child[1]->opn_prb[j])
	    gotone = TRUE;
	if (gotone)
	{
	    /* vno1 is in right - check vno2 in left. */
	    for (j = 0, gotone = FALSE; j < nodep->opn_child[0]->
			opn_nleaves && !gotone; j++)
	     if (hintp->ops_vno2 == nodep->opn_child[0]->opn_prb[j])
	     {
		if (nodep->opn_child[0]->opn_nleaves == 1)
		    *rkjonly = TRUE;
		break;
	     }
	    if (nodep->opn_child[1]->opn_nleaves == 1)
		retval = TRUE;		/* hint is still relevant */
	}

    }

    retval |= *lkjonly | *rkjonly;
    return(retval);

}

/*{
** Name: opn_order_hint	- validate current join permutation
**
** Description:
**	ORDER hint is in effect, so validate current join permutation 
**	against FROM clause sequence
**
** Inputs:
**      subquery		ptr to current subquery being analyzed
**      nodep			ptr to join tree to examine
**
** Outputs:
**	Returns:
**	TRUE - if base tables (or replacement indexes) in join tree 
**	are in same sequence as FROM clause
**	FALSE - otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-06 (dougi)
**	    Written for optimzer hints project.
*/
bool
opn_order_hint(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*nodep)

{
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPV_VARS	*varp;
    OPV_IGVARS	lastvno = -1;
    i4		i;


    /* Loop over join order, assuring that base tables (or replacement
    ** indexes) are in same sequence as in original FROM clause. That
    ** simply means that the corresponding global range table indexes 
    ** are in increasing sequence. Note, that secondary indexes may be
    ** interleaved in any sequence. */

    for (i = 0; i < nodep->opn_nleaves; i++)
    {
	varp = vbase->opv_rt[nodep->opn_prb[i]];
	if (varp->opv_mask & OPV_CPLACEMENT)
	    continue;			/* 2ary ix (not replacement) */

	if (varp->opv_mask & OPV_CINDEX)
	    varp = vbase->opv_rt[varp->opv_index.opv_poffset];

	if (varp->opv_gvar <= lastvno)
	    return(FALSE);		/* sequence failure */
	else lastvno = varp->opv_gvar;
    }

    /* If we get here, the sequence is ok. */
    return(TRUE);

}

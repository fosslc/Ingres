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
#define             OPN_RO4             TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNRO4.C - check index placement for usefulness
**
**  Description:
**      Routines which check some hueristics for index placement 
**
**  History:    
**      10-jun-86 (seputis)    
**          initial creation from rlordr4.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPV_IVARS opn_ro4(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPV_BMVARS *rmap);

/*{
** Name: opn_ro4	- check index placement for usefulness
**
** Description:
**	if tTf(82, 6) is false then this heuristic is:
**
**	if an index has no matching boolean factor and it is complete on
**	the indexing attribute then the index is only useful if it can
**	use its TID's to access its primary relation.  otherwise joining
**	with it does nothing since it provides no selectivity.
**	this heuristic must always be true.
**
**	if tTf(82, 6) is true then this heuristic is:
**
**	an index must have its primary as a leaf higher up in the tree.
**	this is so an index is only used if it can access its primary
**	via TIDs. This is a stronger and inclusive version of relordr3
**	and so if it is used rlordr3 does not have to be checked. this
**	heuristic may exclude optimal solutions however.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           current node of join tree being 
**                                      analzyed
**
** Outputs:
**      rmap                            bitmap of primaries needed further
**                                      up the tree
**	Returns:
**	    a joinop range variable number if the node is a leaf and the
**          range variable is a primary
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-jun-86 (seputis)
**          initial creation from rlordr4
**	29-mar-90 (seputis)
**	    fix byte alignment problems
**	8-feb-93 (ed)
**	    fix incorrectly initialized outer join id
**	25-apr-96 (inkdo01)
**	    Don't apply "complete" check on SPATF indexes. 
[@history_line@]...
*/
OPV_IVARS
opn_ro4(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep,
	OPV_BMVARS         *rmap)
{
    if (nodep->opn_nleaves == 1)
    {	/* process a leaf node */
	OPV_IVARS           leafvar;	    /* if this node is a leaf then this
					    ** will be set to the range variable
					    ** number associate with the leaf */
        
        /* initialize bitmap */
        MEfill(sizeof(*rmap), (u_char)0, (PTR)rmap);

	nodep->opn_ojid = OPL_NOOUTER;	    /* initialize outer join id */
	if ((leafvar = *nodep->opn_prb) < subquery->ops_vars.opv_prv)
	    return (leafvar);		    /* return leaf range variable if 
					    ** it is a primary relation */
	else
	{
	    OPV_VARS           *varp;	    /* ptr to joinop range variable
                                            ** element */

	    varp = subquery->ops_vars.opv_base->opv_rt[leafvar]; /* get ptr
                                            ** to range variable associated
                                            ** with index */

#	    ifdef xNTR1
	    if (tTf(82, 6))
		goto needprim;
#	    endif

	    if ((varp->opv_mbf.opb_bfcount == 0) /* are there any
					    ** boolean factors than can be
                                            ** applied to this index */
		&& !(varp->opv_mask & OPV_SPATF) /* SPATF's are only here
					    ** if we're to use 'em */
		&&
		(varp->opv_index.opv_eqclass != OPE_NOEQCLS)
		&&
		((*opn_eqtamp (subquery, &varp->opv_trl->opn_histogram, 
		    varp->opv_index.opv_eqclass, TRUE))->oph_complete)/* is
					    ** the attribute the index is
                                            ** ordered on complete ? */
		)
	    {
# ifdef xNTR1
	needprim: 
# endif
		BTset((i4)varp->opv_index.opv_poffset, (char *)rmap); /* then
					    ** the primary is required further
                                            ** up the tree */
	    }
	    return (OPV_NOVAR);
	}
    }
    {	/* a non-leaf node */
	OPV_IVARS              leftvar;		/* varno of left child if it is
                                                ** a primary relation 
                                                ** (-1 otherwise) */
	OPV_IVARS              rightvar;	/* varno of right child if it is
                                                ** a primary relation 
                                                ** (-1 otherwise) */
	OPV_BMVARS             rightbitmap;     /* bitmap of required primaries
                                                ** from the right child subtree
                                                */

	leftvar = opn_ro4(subquery, nodep->opn_child[OPN_LEFT], rmap);
	rightvar = opn_ro4(subquery, nodep->opn_child[OPN_RIGHT], &rightbitmap);
	BTor((i4)BITS_IN(*rmap), (char *)&rightbitmap, (char *)rmap ); /* add the
                                                ** required primaries to the
                                                ** ones from the left subtree*/
	if (leftvar >= 0)
	    BTclear((i4)leftvar, (char *)rmap);/* cancel a needed primary */
	if (rightvar >= 0)
	    BTclear((i4)rightvar, (char *)rmap);/* cancel a needed primary */
    }
    return (OPV_NOVAR);
}

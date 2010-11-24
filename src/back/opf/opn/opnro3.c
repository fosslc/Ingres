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

/* external routine declarations definitions */
#define             OPN_RO3             TRUE
#include    <bt.h>
#include    <opxlint.h>
/**
**
**  Name: OPNRO3.C - relorder3 hueristic routines
**
**  Description:
**      routines which apply the relorder3 hueristic
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation from hcheck.c
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
static bool opn_iro3(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);
bool opn_ro3(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep);

/*{
** Name: opn_iro3	- apply rel order 3 hueristic
**
** Description:
**	let one subtree contain only indexes, call this the index 
**	subtree.  let P denote the set of primaries associated with
**	these indexes.  the other subtree is called the primary subtree.
**	if  the primary subtree is not a leaf then it may not contain
**	the set P.
**	
**	if the primary subtree is a leaf and contains the set P then
**	all indexes in the index subtree must have matching boolean
**	factors.
**
**	the previous paragraph is not always true because the index could
**	be sorted and then used to access the relation producing
**	it in sorted order for a merge-scan later on.
**		(we should make the restriction that it must be sorted on
**		 the indexing value - not tid -, we do not do this now)
**		we could order the indexes so those with matching boolean
**		factors are tried first.
**		we could also only use indexes with matching boolean factors
**		since the other uses don't seem to be generally useful?
**
**	NEXT PARAGRAPH HAS BEEN REVOKED, SEE EXPLANATION
**	therefore we modify it so that if it does not have a matching
**	predicate then it must index an eqclass with 3 or more entries.
**	this means that after joining with the primary on this eqclass,
**	there will still be another join on this eqclass later
**	which could benefit from the output of this join being already sorted.
**        
**	EXPLANATION
**	Do not allow the usage of index just because it indexes an eqclass 
**	with 3 or more atts cause cost functions are not good enough and
**	(especially without statistics) the indexes are used
**	often when they should not be.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to operator tree node to which
**                                      this hueristic is applied
**
** Outputs:
**	Returns:
**	    TRUE - if hueristic is satisfied
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-jun-86 (seputis)
**          initial creation from relorder3
[@history_line@]...
*/
static bool
opn_iro3(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep)
{
    OPN_CHILD           primarychild;	    /* child with all primaries */
    OPN_CHILD           indexchild;	    /* child with all indexes */
    OPV_IVARS           maxprimary;	    /* number of primary relations
                                            ** defined */

#   ifdef xNTR2
    if (tTf(31, 4))
	TRdisplay("relorder3\n");
#   endif
    maxprimary = subquery->ops_vars.opv_prv;    /* number of primary relations
                                            ** defined */
    {	/* find the child with the indexes and the child with the primaries */
	OPV_IVARS              *varp;	    /* ptr to range var number in
                                            ** set of relations associated
                                            ** with left child */
	OPN_LEAVES             leafcount;   /* number of range variables in
                                            ** left child */

	varp = nodep->opn_prb;		    /* ptr to first var in partition*/
	primarychild = OPN_RIGHT;	    /* child with primaries */
	indexchild = OPN_LEFT;		    /* child with all indexes */

	for (leafcount = nodep->opn_csz[indexchild]; leafcount--;)
	{
	    if (*varp++ < maxprimary)	    /* "indexchild" has a primary so 
					    ** switch "primarychild" and 
                                            ** "indexchild" */
	    {
		primarychild = OPN_LEFT;
		indexchild = OPN_RIGHT;
		break;
	    }				    /* we know now that if either
					    ** indexchild or primarychild has
					    ** all indexes that it must be 
					    ** indexchild */
	}
    }

    {
	OPV_IVARS              *firstprimp; /* ptr to current varno in the
                                            ** partition of relations of child
                                            ** with primaries */
	OPN_LEAVES             primsize;    /* number of vars in the child
                                            ** with the primaries */
	OPV_IVARS              *indxvarnop;  /* ptr to current varno of child
                                            ** with indexes */
	OPN_LEAVES             indexsize;   /* number of vars in the child
                                            ** with the indexes */
	bool		       no_mbfs;     /* TRUE if one leaf does not have a
                                            ** matching boolean factor on the
                                            ** ordering attribute in the
                                            ** child with indexes */
	firstprimp = nodep->opn_prb + nodep->opn_pi[primarychild];
	primsize = nodep->opn_csz[primarychild];
	indxvarnop = nodep->opn_prb + nodep->opn_pi[indexchild];
	indexsize = nodep->opn_csz[indexchild]; 
	no_mbfs = FALSE;		    /* indicates all indexes have
                                            ** matching boolean factors on
                                            ** ordering attribute */
	while (indexsize--)
	{
	    OPV_IVARS          indexvar;    /* current var to be tested
                                            ** (of child with indexes) */
	    OPV_VARS	       *ivarp;	    /* ptr to "index" range variable
                                            ** element being analyzed */
	    indexvar = *indxvarnop++;	    /* get next varno of "indexchild"*/
	    if (indexvar < maxprimary)
		return(TRUE);		    /* indexchild does not have all 
					    ** indexes after all, so neither 
                                            ** indexchild or primarychild do. 
                                            ** so return okay since test does
                                            ** not apply */
	    ivarp = subquery->ops_vars.opv_base->opv_rt[indexvar];
	    if ( (ivarp->opv_mbf.opb_bfcount == 0) 
/*		&& 
		BTcount(Jn.Eqclist[ivarp->idomnumb]->attrmap, (i4) Jn.Av) < 3 
*/		)
		no_mbfs = TRUE;		    /* remember if this index has
					    ** no matching boolean factor
					    ** and does not index an
					    ** eqclass with 3 or more atts 
					    */
	    {
		OPV_IVARS          *primvarp; /* ptr to current var to be tested
                                            ** (of child with primaries) */
		OPN_LEAVES         remaining; /* number of primaries remaining
                                            ** to be tested */
		OPV_IVARS          primofindex; /* primary varno associated
                                            ** with index being tested */

		primofindex = ivarp->opv_index.opv_poffset;
		primvarp = firstprimp;
		remaining = primsize; 
		while (remaining--)
		{
		    if (primofindex == *primvarp++)
			break;		    /* found associated primary */
		}
		if (remaining < 0)
		    return(TRUE);	    /* passed test because this index's
					    ** associated primary not contained
                                            ** in other subtree */
	    }
	}
	if ((nodep->opn_csz[primarychild] == 1) && (!no_mbfs))
	    return(TRUE);		    /* primary subtree is a leaf
					    ** and all indexes have
					    ** matching boolean factors */
    }

    return (FALSE);			    /* failed test */
}

/*{
** Name: opn_ro3	- relorder3 hueristic
**
** Description:
**	apply heuristic relorder3 to all nodes in the operator tree
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      node                            ptr to node of operator upon which
**                                      to apply the hueristic
**
** Outputs:
**	Returns:
**	    TRUE - if the relorder3 hueristic is satisfied
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-jun-86 (seputis)
**          initial creation from hcheck
[@history_line@]...
*/
bool
opn_ro3(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep)
{
    if (nodep->opn_nleaves == 1)
	return (TRUE);
    return (/*static*/ opn_iro3 (subquery, nodep) 
	    && 
	    opn_ro3(subquery, nodep->opn_child[OPN_LEFT])
	    && 
	    opn_ro3(subquery, nodep->opn_child[OPN_RIGHT]));
}

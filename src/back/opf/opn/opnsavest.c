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
#define        OPN_SAVEST      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNSAVEST.C - routine to save subtree and its cost ordering set
**
**  Description:
**      Contains routine to save subtree and its associated cost ordering
**      set
**
**  History:    
**      7-jun-86 (seputis)    
**          initial creation from savesbtre.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_savest(
	OPS_SUBQUERY *subquery,
	i4 nleaves,
	OPN_SUBTREE *subtp,
	OPN_EQS *eqclp,
	OPN_RLS *rlmp);

/*{
** Name: opn_savest	- save subtree and its associated cost ordering set
**
** Description:
**      Save a subtree with its' associated cost-ordering set away for future
**      use.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nleaves                         number of leaves in subtree being
**                                      analyzed
**      subtp                           ptr to cost ordering set to be
**                                      added to saved work in eqclp
**      eqclp                           ptr to list of cost ordering sets
**                                      to be saved
**      rlmp                            ptr to header of list which contains
**                                      all cost orderings for a particular
**                                      set of relation... to be saved
**      jeqcls                          joining equivalence class used to
**                                      create the subtp cost ordering set
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
**	7-jun-86 (seputis)
**          initial creation from savesbtre
**	3-aug-88 (seputis)
**          cannot deallocate OPN_EQS since top CO node has ptr to eqcmap
**	18-jan-91 (seputis)
**	    - rearrange code to fix b35358, infinite loop
**	 5-mar-01 (inkdo01 & hayke02)
**	    Save histograms, row count in SUBTREE (since they are specific
**	    to a tree shape/leaf assignment combo. This change fixes bug
**	    103858.
**	19-nov-01 (inkdo01)
**	    Add nleaves to opn_deltrl call to allow chain to be resolved
**	    around deleted OPN_RLS.
[@history_line@]...
*/
VOID
opn_savest(
	OPS_SUBQUERY       *subquery,
	i4                nleaves,
	OPN_SUBTREE        *subtp,
	OPN_EQS            *eqclp,
	OPN_RLS            *rlmp)
{
    OPN_ST              *sbase;			/* base of array of ptrs to 
                                                ** cost ordering lists of saved
                                                ** work */
    OPN_RLS             *rlp;			/* used to traverse the list
                                                ** of OPN_RLS structures with
                                                ** a similiar number of leaves*/
    OPN_EQS             *eqp;                   /* used to traverse the list
                                                ** of OPN_EQS structures with
                                                ** the same set of relations
                                                ** attached to the leaves */
    OPS_STATE		*global;
    OPH_HISTOGRAM	*histp = rlmp->opn_histogram;
    OPO_TUPLES		tups = rlmp->opn_reltups;


    global = subquery->ops_global;
    sbase = global->ops_estate.opn_sbase; /* ptr to base of array 
					    ** of ptrs to saved cost 
                                            ** ordering sets */

    for (rlp = sbase->opn_savework[nleaves]; 
	 rlp; 
	 rlp = rlp->opn_rlnext)
	if (rlp == rlmp)
	    break;			    /* check to see if OPN_RLS
                                            ** structure is not already in the
                                            ** list */

    if (!rlp)
    {
	if (global->ops_estate.opn_rootflg
	    &&
	    (nleaves != 1)				/* if this is the root and the
						    ** number of leaves is one then
						    ** this is a primary relation
						    ** with indexes, ... would be
						    ** useful to save the calcuation
						    ** of the primary, so it does
						    ** not need to be recalculated
						    ** when the indexes are
						    ** considered */
	    )
	{	/* if at the routine then we can delete all associated cost orderings */
	    opn_dsmemory(subquery, subtp);		/* reclaim subtree memory */
#if 0
	    do not deallocate OPN_EQS memory since the equivalence class map
	    within the structure is being used by the root COP node
	    opn_dememory(subquery, eqclp);          /* reclaim OPN_EQS memory */
#endif
	    if (nleaves > 1)
		opn_deltrl (subquery, rlmp, OPE_NOEQCLS, nleaves); /* FIXME - 
						    ** pass a set
						    ** of histograms to be deleted
						    ** from calling routine */
	    return;
	}
	else
	{   /* if NULL then rlmp was not found in list so put it there */
	    rlp = rlmp;
	    rlp->opn_rlnext = sbase->opn_savework[nleaves];
	    sbase->opn_savework[nleaves] = rlp;
	}
    }

    for (eqp = rlp->opn_eqp; eqp; eqp = eqp->opn_eqnext)
	if (eqp == eqclp)
	    break;                          /* check if this element is already
                                            ** in the equivalence class list
                                            */
    if (!eqp)
	{   /* if NULL then eqp was not found in list so put it there */
	    eqp = eqclp;
	    eqp->opn_eqnext = rlp->opn_eqp;
	    rlp->opn_eqp = eqp;
	}

    /* the cost ordering cannot be in the list or this routine would not
    ** have been called, so put it there.
    */
    subtp->opn_stnext = eqp->opn_subtree;
    eqp->opn_subtree = subtp;
    if (subtp->opn_coforw != (OPO_CO *)subtp)
    {   /* Save hist. chain, row count in SUBTREE struct. */
	subtp->opn_histogram = histp;
	subtp->opn_reltups = tups;
    }

#   ifdef xNTR1
    if (tTf(20, 1))
    {
	TRdisplay("savesubtree:\n");
	ptrls (rlp);
    }
#   endif

}


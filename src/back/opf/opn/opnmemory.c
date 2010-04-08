/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <ex.h>
#define	       OPX_EXROUTINES  TRUE
#define	       OPN_JMEMORY     TRUE
#define	       OPN_FMEMORY     TRUE
#define        OPN_MEMORY      TRUE
#include    <me.h>
#include    <opxlint.h>

/**
**
**  Name: OPNMEMORY.C - allocate some enumeration memory
**
**  Description:
**      routine to allocate enumeration memory
**      FIXME - routines to do garbage collection are mostly concerned
**      with CO trees; since these lists get pretty long.  The following
**      strategy should be implemented.
**      PHASE I)  This phase attempts to do intelligent garbage collection
**      and avoid regeneration of intermediate CO trees.
**      If memory is exhausted then a traversal of the savesubtree structs
**      should be made and any cotrees (and related structures) which
**      are greater than the minimum cost tree should be deleted. (currently
**      all structures are deleted, except the minumum cost tree and current
**      tree being worked on).  By taking advantage of the enumeration order
**      if a search of the subtrees fails it is because the cost of the subtree
**      exceeded the minimum cost tree (if indexes in the current tree being
**      worked on are not all included) since it must have been generated and
**      deleted.
**      Fix the dangling pointer problem for histograms in OPN_RLS structs.
**      Also delete OPN_RLS and OPN_EQS structures which have had all
**      respective CO trees deleted.
**
**      PHASE II) Partial deletion of work which has been done.  Attempt
**      to delete work which has been done for (n-1) leaves where n is the
**      number of leaves currently being worked on.  Repeat this (for n-2 etc.)
**      until some
**      memory has been freed to satisfy the current request.  The hueristic
**      for searching saved work now (i.e. if it cannot be found then it is too
**      expensive) becomes limited to the subtrees which
**      have fewer leaves than those deleted.
**
**      PHASE III) This phase begins when memory is exhausted in spite of
**      intelligent garbage collection.  Find and save information about
**      the current position in the enumeration and the current best
**      minimum CO tree.  Delete the enumeration stream and recreate it.
**      Move forward in the enumeration to the point at which memory was
**      exhausted and begin.  The hueristics used in PHASE I and II cannot be
**      used now.
**
**      PHASE IV) If no progress can be made then too little memory exists
**      at this point, so exit with the best CO if it exists or report an
**      error.
**
**  History:    
**      15-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
[@history_line@]...
**/

/*{
** Name: opn_jmemory	- allocate OPN_JTREE enumeration memory
**
** Description:
**      This routine will allocate memory from the enumeration stream which
**      will be available for the duration of the enumeration of one subquery.
**      It will also persist if an attempt is made to recover from an out
**      of memory error is made during the enumeration phase, by re-initializing
**      the main enumeration stream.  This will also enumeration to continue
**      processing from the proper point in the search space.
**
** Inputs:
**      global                          ptr to global state variable
**      size                            size of memory space to allocate
**
** Outputs:
**	Returns:
**	    PTR to memory space with "size" bytes allocated - aligned for
**          all data types.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-jun-86 (seputis)
**          initial creation
**	13-sep-02 (inkdo01)
**	    JTREE memory has always (from rev 1 of this module) been init'ed with
**	    0x7f's. It is now zeroed to clarify some debug displays.
[@history_line@]...
*/
PTR
opn_jmemory(
	OPS_STATE          *global,
	i4                size)
{
    ULM_RCB             *ulm_rcb;	    /* ptr to ULM_RCB that represents
                                            ** the enumeration memory */
    DB_STATUS           ulmstatus;          /* return status from ULM */

    ulm_rcb = &global->ops_mstate.ops_ulmrcb;   	    /* ptr to control block
                                            ** containing state of
                                            ** enumeration memory */
    ulm_rcb->ulm_streamid_p = &global->ops_estate.opn_jtreeid; /* allocate
                                            ** memory from the enumeration
                                            ** stream */
    ulm_rcb->ulm_psize = size;		    /* get size of memory block
                                            ** required */
    if ((ulmstatus = ulm_palloc( ulm_rcb )) != E_DB_OK)
	opx_verror(ulmstatus, E_OP0400_MEMORY, ulm_rcb->ulm_error.err_code);
    /* FIXME - initial memory to non-zero value temporarily for testing */
    /* Changed from 127 (0x7f's) to 0 to make structures more readable. */
    MEfill( size, (u_char)0, (PTR) ulm_rcb->ulm_pptr);
    return( ulm_rcb->ulm_pptr );	    /* return pointer */
}

/*{
** Name: opn_fmemory	- recover from out of memory errors
**
** Description:
**      This routine will recover from out of memory errors in the enumeration
**      phase, by searching for structures associated with CO nodes which are
**      not useful.  If this fails then useful structures are deleted, which
**      will slow down future processing since this information will need to
**      be recalculated.
**
** Inputs:
**      subquery                        ptr to subquery being enumerated
**      target                          ptr to beginning of a free list
**                                      which should be non-NULL if the
**                                      garbage collection routine was
**                                      successful
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory management free lists may be updated
**
** History:
**      9-nov-86 (seputis)
**          initial creation
**      14-oct-89 (seputis)
**          deleted rls structure at incorrect time, causing unnecessary computations
**      15-dec-91 (seputis)
**          fix 33551 - make better guess for initial set of relations to enumerate
**      8-may-92 (seputis)
**          - b44212, check for overflow in opo_pointercnt
**	19-nov-01 (inkdo01)
**	    Resolve OPN_RLS chain around deleted entries.
**	21-feb-02 (inkdo01)
**	    Remove above change - it was only the logic in opn_deltrl that neglected 
**	    to resolve OPN_RLS ptr chain.
[@history_template@]...
*/
VOID
opn_fmemory(
	OPS_SUBQUERY       *subquery,
	PTR                *target)
{
    OPS_STATE           *global;	    /* ptr to global state variable */
    OPV_IVARS           maxvars;	    /* number of vars in joinop range
                                            ** table */
    OPN_ST              *savebase;          /* base of array of ptrs to saved
                                            ** work area */
    global = subquery->ops_global;
    maxvars = subquery->ops_vars.opv_rv;    /* number of range variables */
    savebase = global->ops_estate.opn_sbase; /* base of array
                                            ** of ptrs to saved work area */
    if (!global->ops_estate.opn_search)
    {	/* if more CO nodes are required then try to free up nodes which
        ** can be deleted */
	OPV_IVARS	    varno;	    /* number of leaves in temporary
                                            ** result */
	global->ops_estate.opn_search = TRUE;   /* set TRUE if all free CO nodes
						** have been purged ... would be
						** reset to FALSE if a new best
						** CO tree is found, since new
						** CO nodes could be released */
	for (varno = maxvars; varno-- > 1;)
	{
	    OPN_RLS	    **rlpp;	    /* temporary used to traverse saved
                                            ** relation descriptors */
	    bool            notneeded;      /* TRUE - if the RLS structures
                                            ** are not needed for this number
                                            ** of leaves */
	    notneeded = (varno <= global->ops_estate.opn_swcurrent); /* can 
                                            ** delete RLS structures here 
                                            ** since all subtrees have been 
                                            ** considered, make sure leaf
                                            ** nodes are not deleted since
                                            ** these are referenced directly
                                            ** by the range variable structs 
                                            */
	    for (rlpp = &savebase->opn_savework[varno]; *rlpp; )
	    {
                OPN_RLS     *rlp;           /* dereference rlpp */
		OPN_EQS	    **ecpp;         /* temporary used to traverse EQS
                                            ** structs for this set of relations
                                            */
		bool	    deleteable;     /* TRUE - if this RLS struct is not
                                            ** in the current cost tree being
                                            ** evaluated and can be deleted */

                rlp = *rlpp;
		deleteable = rlp->opn_delflag && notneeded;/* check flag set in 
                                            ** opn_nodecost, indicating that
                                            ** this structure and its children
                                            ** are deletable */
		for (ecpp = &rlp->opn_eqp; *ecpp; )
		{
		    OPN_SUBTREE **sbtpp;    /* temporary used to traverse 
                                            ** co subtree list */
                    OPN_EQS     *ecp;
                    ecp = *ecpp;            
		    for (sbtpp = &ecp->opn_subtree; *sbtpp; )
		    {
			OPO_CO *cop;         /* ptr to root of CO subtree */
                        OPO_CO *subtp_cop;  /* beginning and end of list */
                        subtp_cop = (OPO_CO *) (*sbtpp); /* the coerced subtree
                                            ** structure is the beginning and
                                            ** end of the list of CO nodes */
			for (cop = subtp_cop->opo_coforw;
			     cop != subtp_cop; 
                             cop = cop->opo_coforw)
			    if (!opn_goodco (subquery, cop) 
				&& 
				!cop->opo_pointercnt)
			    {
				OPO_CO	*cb;

				cb = cop->opo_coback;    /*remove CO root from list*/
				cb->opo_coforw = cop->opo_coforw;
				cop->opo_coforw->opo_coback = cb;
				opn_dcmemory (subquery, cop); /* place CO in free
                                                    ** list */
				cop = cb;
			    }
			if (deleteable 
			    && 
			    (subtp_cop->opo_coforw == subtp_cop) )
			{   /* release this subtree since it will not reference
                            ** any useful CO nodes */
			    *sbtpp = (*sbtpp)->opn_stnext;
			    opn_dsmemory(subquery, (OPN_SUBTREE *) subtp_cop); /*
					    ** release subtree which is not
                                            ** useful */
			}
			else
			{
			    sbtpp = &(*sbtpp)->opn_stnext; /* get next element
                                            ** to be analyzed */
			}
		    }
		    if (deleteable 
			&& 
			!ecp->opn_subtree) 
		    {   /* release this OPN_EQS since it will not reference
			** any useful subtree structure nodes */
			*ecpp = (*ecpp)->opn_eqnext;
			opn_dememory(subquery, ecp); /*
					** release subtree which is not
					** useful */
		    }
		    else
		    {
			ecpp = &(*ecpp)->opn_eqnext; /* get next element
					** to be analyzed */
		    }
		}
		if (deleteable && !rlp->opn_eqp && (varno>1)) 
		{   /* release this OPN_RLS since it will not be referenced
		    ** by any useful query plans, do not delete leaves
                    ** since they are referenced directly by joinop range
                    ** variables */
		    *rlpp = rlp->opn_rlnext;
		    {	/* release histogram structures which are not useful
                        ** and are attached to this OPN_RLS */
			OPH_HISTOGRAM		*hp;
			for (hp = rlp->opn_histogram; hp;)
			{
			    OPH_HISTOGRAM   *temphp;
			    temphp = hp;
			    hp = hp->oph_next; /* get next element */
                            /* FIXME - if usage counts existed for the histogram
                            ** then it could also be deallocated here */
			    oph_dmemory(subquery, temphp); /* restore histogram
					      ** structure to free list */
			}
		    }
		    opn_drmemory(subquery, rlp); /* release OPN_RLS which is not
				    ** useful */
		}
		else
		{
		    rlpp = &(*rlpp)->opn_rlnext; /* get next element
				    ** to be analyzed */
		}
	    }
	    if (*target)
		return;				    /* the appropriate data structure
						    ** was freed so return, moved into
						    ** loop to avoid deleting more than
						    ** necessary, part of b40402 */
	}
    }
    /* 
    ** see if there are any subtrees structs where all
    ** co structs are not pointed at.  if so delete the
    ** co's and the subtrees struct.  this deleted info
    ** may have to be regenerated later, slowing down
    ** processing 
    */

    {
	OPN_LEAVES	    leaves;	    /* number of leaves in RLS structure
                                            ** being considered */
	for (leaves = maxvars; leaves-- > 1;)
	{
	    OPN_RLS		**trlpp;    /* temp relation structure to be
                                            ** analyzed */
	    for (trlpp = &savebase->opn_savework[leaves]; *trlpp;)
	    {
		if ((*trlpp)->opn_delflag)
		{
		    OPN_EQS	    **tecpp; /* equivalence class structure
                                            ** associated with rlp */
		    for (tecpp = &(*trlpp)->opn_eqp; *tecpp; )
		    {
			OPN_SUBTREE **tsbtpp; /* used to eliminate subtrees from
					    ** the linked list */
			for (tsbtpp = &(*tecpp)->opn_subtree; *tsbtpp; )
			{
			    OPO_CO	    *marker; /* used to mark beginning and end
						** of CO list */
			    OPN_SUBTREE *tsbtp;  /* subtree structure associated with
						** ecp */
			    OPO_CO	*cp; /* CO node at top of subtree */
                            tsbtp = *tsbtpp;
                            marker = (OPO_CO *)tsbtp;
			    for (cp = tsbtp->opn_coforw; cp != marker; 
				 cp = cp->opo_coforw)
			    {
				if (cp->opo_pointercnt != 0)
				    break;
			    }
			    if (cp != marker)
			    {   /* not all CO's are useless so skip to
                                ** next subtree structure */
				tsbtpp = &tsbtp->opn_stnext;
				continue;
			    }
                            /* at this point the subtree and all associated
			    ** CO nodes are useless */
			    for (cp = tsbtp->opn_coforw; cp != marker; )
			    {
				OPO_CO	    *tempcop;
				tempcop = cp;
				cp = cp->opo_coforw;
				opn_dcmemory(subquery, tempcop);
			    }
			    *tsbtpp = tsbtp->opn_stnext;
			    opn_dsmemory(subquery, tsbtp);
			}
			if (!(*tecpp)->opn_subtree)
			{   /* place OPN_EQS structure into free list */
			    OPN_EQS	    *tempecp;
			    tempecp = *tecpp;
			    *tecpp = tempecp->opn_eqnext;
			    opn_dememory(subquery, tempecp);
			}
			else
			    tecpp = &(*tecpp)->opn_eqnext; /* still in use so
					    ** skip to next structure */
		    }
		    if ((!(*trlpp)->opn_eqp) && (leaves>1))
		    {   /* place OPN_RLS structure into free list if it
			** is empty of eqp structures
			** - do not delete OPN_RLS structure for leaves since
                        ** they are referenced directly by range variable
                        ** elements
                        */
			OPN_RLS	    *temprlp;
			temprlp = *trlpp;
			*trlpp = temprlp->opn_rlnext;
			{   /* release histogram structures which are not useful
			    ** and are attached to this OPN_RLS */
			    OPH_HISTOGRAM		*hp;
			    for (hp = temprlp->opn_histogram; hp;)
			    {
				OPH_HISTOGRAM   *temphp;
				temphp = hp;
				hp = hp->oph_next; /* get next element */
				/* FIXME - if usage counts existed for the histogram
				** then it could also be deallocated here */
				oph_dmemory(subquery, temphp); /* restore 
					    ** histogram structure to free list
					    */
			    }
			}
			opn_drmemory(subquery, temprlp);
			continue;
		    }
		}
		trlpp = &(*trlpp)->opn_rlnext; /* still in use so
				    ** skip to next structure */
	    }
	    if (global->ops_estate.opn_swcurrent > leaves)
		global->ops_estate.opn_swcurrent = leaves-1; /* indicates that 
					    ** some useful work has been lost */
            if (global->ops_estate.opn_swlevel > leaves)
		global->ops_estate.opn_swlevel = leaves-1; /* indicates that the
                                            ** next time a leaf is added, that
                                            ** the useful work saved will be
                                            ** at a lower level */
	    if (*target)
		return;			    /* return if this pass freed up some
                                            ** useful structures */
	}
    }
}

/*{
** Name: opn_memory	- allocate enumeration memory
**
** Description:
**      This routine will allocate memory from the enumeration stream which
**      will be available for the duration of the enumeration of one subquery.
**      All memory will be deallocated after the best co tree has been copied
**      from the enumeration memory space, to the optimization memory space.
**      This distinction is made since the enumeration memory space tends to
**      grow quite large and will be "reused" by successive enumerations.
**
** Inputs:
**      global                          ptr to global state variable
**      size                            size of memory space to allocate
**
** Outputs:
**	Returns:
**	    PTR to memory space with "size" bytes allocated - aligned for
**          all data types.
**	Exceptions:
**	    (EX)EX_JNP_LJMP is generated if an error is received from ULM
**          - the exception handler in OPN_CEVAL should catch this error
**          and attempt to recover from it.
**
** Side Effects:
**	    none
**
** History:
**	15-jun-86 (seputis)
**          initial creation
**	20-feb-91 (seputis)
**	    make non-zero initialization of memory an xDEBUG option
**	09-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
[@history_line@]...
*/
PTR
opn_memory(
	OPS_STATE          *global,
	i4                size)
{
    ULM_RCB             *ulm_rcb;	    /* ptr to ULM_RCB that represents
                                            ** the enumeration memory */

    if (global->ops_mstate.ops_usemain)
    {	/* normally the enumeration memory stream is used, but
        ** this flag indicates the enumeration memory is not available
        ** and that the main memory stream should be used instead */
	return(opu_memory(global, size));
    }
    ulm_rcb = &global->ops_mstate.ops_ulmrcb; /* ptr to control block
                                            ** containing state of
                                            ** enumeration memory */
    ulm_rcb->ulm_streamid_p = &global->ops_estate.opn_streamid; /* allocate
                                            ** memory from the enumeration
                                            ** stream */
    ulm_rcb->ulm_psize = size;		    /* get size of memory block
                                            ** required */
    if (ulm_palloc( ulm_rcb ) != E_DB_OK)
	EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)E_OP0400_MEMORY);/* signal an
                                            ** optimizer long jump so that the
                                            ** exception handler OPN_CEVAL can 
                                            ** attempt to recover from this 
                                            ** error */
#ifdef xDEBUG
    /* FIXME - initial memory to non-zero value temporarily for testing */
    MEfill( size, (u_char)127, (PTR) ulm_rcb->ulm_pptr);
#endif
    return( ulm_rcb->ulm_pptr );	    /* return pointer */
}

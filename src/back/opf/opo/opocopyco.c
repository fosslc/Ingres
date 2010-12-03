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
#define        OPO_COPYCO      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPOCOPYCO.C - copy CO tree from enum space to joinop space
**
**  Description:
**      Routine will copy the best co tree from the enum memory stream to the
**      global OPF memory stream (i.e. joinop memory stream).  This is so that
**      the enumeration memory stream can be deleted, so the memory is
**      made available, for OPC, or for recovering from out of memory errors
**      in enumeration.
**
**  History:
**      15-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static void opo_cco(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPO_CO *marker,
	OPO_CO **base);
static void opo_subtree(
	OPS_SUBQUERY *subquery,
	OPO_CO **copp,
	OPO_PERM **saveptr,
	bool remove);
static void opo_error(
	OPS_SUBQUERY *subquery);
void opo_copyfragco(
	OPS_SUBQUERY *subquery,
	OPO_CO **copp,
	bool top);
i4 opo_copyco(
	OPS_SUBQUERY *subquery,
	OPO_CO **copp,
	bool remove);

/*{
** Name: opo_cco	- mark CO nodes of best CO tree
**
** Description:
**      This routine marks the CO nodes which have been used in the best CO tree
**      This information will be used to determine if the CO nodes used in the
**      best CO tree are already outside of enumeration memory, and also to
**      indicate which preallocated CO nodes (allocated outside of enumeration
**      memory) are available to be used for copying CO nodes from the best
**      CO tree which are still inside enumeration memory.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      copp                            ptr to ptr to co node to copy to
**                                      joinop memory
**      marker                          unique value - ptr to CO node in stack
**                                      which is uninitialized, used to mark
**                                      CO nodes found in best CO tree
**
** Outputs:
**      base                            - linked list of CO nodes found in
**                                      best CO tree
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-jun-86 (seputis)
**          initial creation
**	16-may-90 (seputis)
**	    - fix lint errors 
[@history_line@]...
*/
static VOID
opo_cco(
	OPS_SUBQUERY       *subquery,
	OPO_CO		   *cop,
	OPO_CO             *marker,
	OPO_CO             **base)
{
    do
    {
	if (cop->opo_coback == marker)
	{
	    cop->opo_pointercnt += 1;   /* increment the pointer count
					** so that common subtrees can be
					** copied */
	    /* node has already been linked in so do not link in again */
	}
	else
	{
	    cop->opo_pointercnt = 1;    /* initialize pointer count if this
					** is the first time thru */
	    cop->opo_coforw = *base;
	    *base = cop;		    /* link in postfix order */
	    cop->opo_coback = marker;   /* save unique identifier */
	}
	if (cop->opo_outer)
	    opo_cco(subquery, cop->opo_outer, marker, base); /* recurse on
					** the outer; mark and link 
					** outer */
    }
    while    (cop = cop->opo_inner);	/* iterate on the inner; mark and
					** link outer */
}

/*{
** Name: opo_subtree	- copy subtree out of enumeration memory
**
** Description:
**      Used when dealing with arrays of CO trees to ensure no subtrees
**      are shared.  Assumes that all elements of opn_colist are not
**      used in any of the best CO trees to be copied.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      copp                            ptr to ptr to root of subtree to be
**					copied
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
**      23-oct-88 (seputis)
**          initial creation
**      8-may-89 (seputis)
**          fix mismatched parameter
*/
static VOID
opo_subtree(subquery, copp, saveptr, remove)
OPS_SUBQUERY       *subquery;
OPO_CO		   **copp;
OPO_PERM	   **saveptr;
bool		   remove;
{
    OPO_PERM	*permptr;	/* ptr to unused CO which is out
				** of enumeration memory */
    OPO_CO	*cop;		/* ptr to CO to be copied out of
				** enumeration memory */
    OPS_STATE	*global;

    global = subquery->ops_global;
    do
    {	/* iterate on inner, recurse on outer */
	cop = *copp;
	permptr = global->ops_estate.opn_colist;
	if (permptr)
	{
	    global->ops_estate.opn_colist = permptr->opo_next;
	    if (remove)
		global->ops_estate.opn_cocount--; /* decrement count of
					** available CO nodes since it is
					** being removed from the list */
	    else
	    {	/* place in linked list of nodes to be reused */
		permptr->opo_next = *saveptr;
		*saveptr = permptr;
	    }
	}
	else
	{
	    if (!remove)
		opx_error(E_OP0499_NO_CO);	/* expected enough CO nodes to be
				    ** allocated out of enumeration memory here, since
				    ** remove is FALSE only if we run out of memory */
	    permptr = (OPO_PERM *)opu_memory(subquery->ops_global, 
		(i4)sizeof(OPO_PERM));
	}
	/* FIXME - need to get forward address pointers set correctly */
	MEcopy((PTR)cop->opo_maps, sizeof(permptr->opo_mapcopy),
	    (PTR)&permptr->opo_mapcopy); /* copy map
				    ** to non-enumeration memory */
	MEcopy((PTR)cop, sizeof(permptr->opo_conode), 
	    (PTR)&permptr->opo_conode);
	permptr->opo_conode.opo_maps= &permptr->opo_mapcopy; /* equivalence class
				    ** map out of enumeration memory */
	cop->opo_pointercnt -= 1;	/*decrement pointer count for future copies */
	permptr->opo_conode.opo_pointercnt = 1; /* only one parent for new
				    ** copy */
	permptr->opo_conode.opo_coback=  /* reset this value */
	permptr->opo_conode.opo_coforw = /* make sure forwarding address is
				    ** set correctly for initial call point
				    ** in opo_copyco */
	*copp = &permptr->opo_conode; /* replace CO node with copied CO node */
	if (cop->opo_outer)
	    opo_subtree(subquery, &permptr->opo_conode.opo_outer, saveptr, remove);
	copp = &permptr->opo_conode.opo_inner; /* iterate on inner */
    }
    while    (*copp);
}

/*{
** Name: opo_error	- report query plan error
**
** Description:
**      This routine will generate an OPF error related to the fact
**	that a query plan could not be found.  Since several conditions
**	can cause no qep to be found, this routine will check those
**	conditions to determine which error message to print.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**
**	Returns:
**	    The routine will not return
**	Exceptions:
**
** Side Effects:
**
** History:
**      16-jul-91 (seputis)
**          initial creation
**	15-dec-91 (seputis)
**	    fix error code number assignment conflict
[@history_template@]...
*/
static VOID
opo_error(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;

    global = subquery->ops_global;
    /* if no query plan exists then report error */
    if (global->ops_terror)
	opx_error(E_OP0009_TUPLE_SIZE); /* error was probably caused by
				** tuples in the intermediate result
				** being too width, for all cases */
    else if (subquery->ops_mask & OPS_TMAX)
	opx_error(E_OP000F_TEMP_SIZE); /* search space was reduced due
				** to SORTMAX limitations */
    else if (global->ops_gmask & OPS_FLSPACE)
	opx_error(E_OP000B_EXCEPTION);
    else if (subquery->ops_mask & OPS_CAGGTUP)
	opx_error (E_OP000E_AGGTUP);
    else
	opx_error(E_OP0491_NO_QEP); /* internal consistency check */
}

/*{
** Name: opo_copyfragco	- copy fragment CO to permanent space  joinop space
**
** Description:
**      This routine copies a fragment CO-tree from the enumeration memory
**      space to the joinop memory space. It is used as part of the new
**	enumeration algorithm to save plan fragments generated by each 
**	phase of the lookahead enumeration process.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      copp                            ptr to ptr to co node to copy to
**                                      joinop memory (non-enumeration memory)
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
**	3-oct-02 (inkdo01)
**	    Written. 
**	23-oct-03 (inkdo01)
**	    Copy from OPO_COMAPS ptr, not from addr of ptr.
**	31-oct-04 (inkdo01)
**	    Take PERMs from global list.
**	24-jan-05 (inkdo01)
**	    Prevent endless loop caused by opo_coforw linkback.
[@history_line@]...
*/
VOID
opo_copyfragco(
	OPS_SUBQUERY       *subquery,
	OPO_CO		   **copp,
	bool		   top)
{
    OPS_STATE	*global = subquery->ops_global;
    OPO_PERM	*permp = global->ops_estate.opn_fragcolist;
    OPO_CO	*cop;
    OPO_CO	*linkco = (OPO_CO *) NULL;


    /* If this is the top node of the fragment, loop across the opo_coforw
    ** chain, looking for node to link replacement to because fragment
    ** may still be part of a OPN_SUBTREE plan list. If a opo_coforw points
    ** to itself, quit the search. This happens in nodes that are already
    ** in permanent storage. */
    /* This code doesn't even look used - there is code later on with a 
    ** "if (FALSE ..." that may have been intended to use it. */
    if (top && (linkco = (*copp)->opo_coforw) != (OPO_CO *) NULL)
    {
	for (; linkco != (OPO_CO *) NULL && linkco != linkco->opo_coforw &&
	    linkco->opo_coforw != *copp; linkco = linkco->opo_coforw);
    }

    /* Just descend the CO-tree, allocating new nodes (in PERM structures),
    ** and copying CO-node and referenced EQC maps. We iterate down inner side
    ** and recurse down outer side. */

    do {
	for (; permp->opo_conode.opo_held; permp = permp->opo_next); 
					/* next available PERM */
	cop = &permp->opo_conode;
	STRUCT_ASSIGN_MACRO((**copp), *cop);
	MEcopy ((char *)(*copp)->opo_maps, sizeof(OPO_COMAPS),
				(char *)&permp->opo_mapcopy);
	cop->opo_maps = &permp->opo_mapcopy;
	cop->opo_held = TRUE;

	if (FALSE && top)
	{
	    /* Patch OPN_SUBTREE->OPO_CO chain to skip new CO. After
	    ** an enumeration storage recovery, we don't want the 
	    ** permanent nodes addr'ing non-existant nodes. */
	    if (cop->opo_coforw)
		cop->opo_coforw->opo_coback = cop->opo_coback;
	    if (cop->opo_coback)
		cop->opo_coback->opo_coforw = cop->opo_coforw;
	    cop->opo_coforw = cop->opo_coback = (OPO_CO *) NULL;
	}
	    cop->opo_coforw = cop->opo_coback = (OPO_CO *) NULL;
	top = FALSE;			/* only for very first CO-node */

	(*copp) = cop;			/* link new CO-node to copied fragment */
	if (cop->opo_outer)
	    opo_copyfragco(subquery, &cop->opo_outer, FALSE);
					/* recurse down left */
    } while (*(copp = &cop->opo_inner));	/* iterate down right */

}

/*{
** Name: opo_copyco	- copy best co tree from enum space to joinop space
**
** Description:
**      This routine will copy the best co tree from the enumeration memory
**      space to the joinop memory space.  This routine will be used when
**      enumeration has completed, or memory is exhausted.  
**      The CO nodes necessary for copying must have
**      been allocated outside of the enumeration workspace, prior to
**      calling this routine.  In order to efficiently use memory, the
**      preallocated CO nodes are "mixed" and used during enumeration.  This
**      routine needs to be aware of this and manage the CO nodes located
**      in non-enumeration memory accordingly.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      copp                            ptr to ptr to co node to copy to
**                                      joinop memory (non-enumeration memory)
**      remove                          TRUE if CO nodes need to be removed
**                                      from the opn_colist linked list i.e.
**                                      these nodes cannot be reused
**                                      - this flag will be TRUE at the end
**                                      of enumeration of a subquery,
**                                      - it will be FALSE if enumeration
**                                      is not completed, so that preallocated
**                                      CO nodes are not lost for the remainder
**                                      of the enumeration of this subquery, in
**                                      case a better CO tree is found... this
**                                      case occurs if the enumeration memory
**                                      stream runs out of memory
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
**	28-oct-86 (seputis)
**          initial creation
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	16-jul-91 (seputis)
**	    fix access violation in star tests, star query plan 
**	    for function aggregates was not
**	    copied out of OPF enumeration memory properly
**	12-aug-91 (seputis)
**	    - fix b38691, print error if aggregate temp too large
**	26-oct-93 (ed)
**	    - fix 53174 - star CO nodes are lost when aggregates are
**	    processed
**	20-mar-06 (dougi)
**	    Return DB_STATUS from opo_copyco() for optimizer hint project.
**	3-may-06 (dougi)
**	    Replace "return" incorrectly removed from Star queries.
[@history_line@]...
*/
DB_STATUS
opo_copyco(
	OPS_SUBQUERY       *subquery,
	OPO_CO		   **copp,
	bool               remove)
{
    OPO_PERM        *saveptr;       /* linked list of OPN_PERM nodes used to
				    ** contain best co tree nodes */
    OPO_PERM	    **previous;	    /* initialize CO nodes in permanent list */
    OPO_CO          *base;          /* base of linked list of CO nodes in 
                                    ** postfix order */
    OPO_CO          co_marker;      /* address of this struct is used to mark
                                    ** all CO nodes in the best CO tree in the
                                    ** opo_coback field... the components
                                    ** of co_marker are not ever used or
                                    ** initialized, the address of this
				    ** structure is unique and will never
				    ** be found in opo_coback until it is
				    ** used below */
    OPO_CO          co_okay;        /* used to mark CO nodes which are in the
                                    ** best CO tree and also out of enumeration
                                    ** memory ... like co_marker only the 
                                    ** address is used and no components are
                                    ** ever initialized */
    OPS_STATE       *global;        /* ptr to global state variable */

    global = subquery->ops_global;
    base = NULL;                    /* mark end of CO list */
    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
	if (subquery->ops_dist.opd_bestco)
	{   /* an array of CO nodes exist so all of these must be copied out of
	    ** enumeration memory, this is tricky since some plans to be
	    ** copied may share subtrees with one another, so care needs
	    ** to be taken to copy the shared subtrees since different
	    ** site selection may occur depending on the result site */
	    OPD_ISITE	siteno;
	    bool	found_one;

	    found_one = FALSE;
	    for (siteno = global->ops_gdist.opd_dv; --siteno >= 0;)
	    {
		OPO_CO	*site_cop;

		site_cop = subquery->ops_dist.opd_bestco->opd_colist[siteno];
		if (site_cop)
		{
		    if ((subquery->ops_mode == PSQ_RETINTO) && remove)
			opn_cleanup(subquery, site_cop, &site_cop->opo_cost.opo_dio,
					&site_cop->opo_cost.opo_cpu); /* add cost to top node
					** for retrieve into */	
		    opo_cco(subquery, site_cop, &co_marker, &base); /* mark all nodes in the best CO tree
					** by setting opo_coback = &co_marker, and
					** link nodes of tree in postfix order
					** using opo_coforw */
		    found_one = TRUE;
		}
	    }
	    if (!found_one)
	    {
#ifdef    E_OP0491_NO_QEP
		if (remove)
		{
		    if (subquery->ops_mask2 & OPS_HINTPASS)
			return(E_DB_ERROR); /* go back for 2nd pass */

		    opo_error(subquery);    /* report query plan error
					** & die */
		}
		return E_DB_OK;
#endif
		
	    }
	}
	else
	{
	    if ((subquery->ops_mode == PSQ_RETINTO) && remove)
		opn_cleanup(subquery, *copp, &(*copp)->opo_cost.opo_dio,
					&(*copp)->opo_cost.opo_cpu); /* add cost to top node
				    ** for retrieve into, if this is the final copy
				    ** out of enumeration memory */	
	    if (!global->ops_gdist.opd_scopied)
	    {	/* label sites given the target site for the query, this will
		** destroy the site cost array */
		(VOID)opd_msite(subquery, *copp, subquery->ops_dist.opd_target, 
		    TRUE, (OPO_CO *)NULL, remove);
		global->ops_gdist.opd_scopied = TRUE;
	    }
	    opo_cco(subquery, *copp, &co_marker, &base); /* mark all nodes in the best CO tree
				    ** by setting opo_coback = &co_marker, and
				    ** link nodes of tree in postfix order
				    ** using opo_coforw */
	}
    }
    else
    {
#ifdef    E_OP0491_NO_QEP
	if (!(*copp))
	{
	    if (subquery->ops_mask2 & OPS_HINTPASS)
		return(E_DB_ERROR); /* return error to initiate 2nd pass */

	    opo_error(subquery);    /* report query plan error */
	}
#endif
	if ((subquery->ops_mode == PSQ_RETINTO) && remove)
	    opn_cleanup(subquery, *copp, &(*copp)->opo_cost.opo_dio,
					&(*copp)->opo_cost.opo_cpu); /* add cost to top node
				    ** for retrieve into, if this is the final copy
				    ** out of enumeration memory */	
	opo_cco(subquery, *copp, &co_marker, &base);
				    /* mark all nodes in the best CO tree
                                    ** by setting opo_coback = &co_marker, and
                                    ** link nodes of tree in postfix order
                                    ** using opo_coforw */
    }
    saveptr = NULL;
    for ( previous = &global->ops_estate.opn_colist; *previous; )
    {   /* this pass of the OPN_PERM list will copy any bitmaps, and
        ** remove OPN_PERM nodes if indicated by the "remove" flag
	*/

	if ((*previous)->opo_conode.opo_coback == &co_marker)
	{   /* this node is already out of enumeration memory */
	    OPO_COMAPS	    *sourcep;	/* ptr to equivalence class bit map
                                        ** to copy */
	    OPO_COMAPS      *destp;     /* ptr to destination of equivalence
                                        ** class bit map */
	    OPO_PERM        *live;      /* ptr to OPN_PERM structure being
					** used inside the best CO tree */
            live = *previous;
	    live->opo_conode.opo_coback = &co_okay; /* mark this node
                                        ** as being in non-enumeration memory
                                        */
	    destp = &live->opo_mapcopy; /* equivalence class map out
                                        ** of enumeration memory */
	    sourcep = live->opo_conode.opo_maps; /* equivalence class
                                        ** map located somewhere in enumeration
                                        ** memory */
	    MEcopy((PTR)sourcep, sizeof(*destp), (PTR)destp); /* copy map
                                        ** to non-enumeration memory */
            live->opo_conode.opo_maps = destp; /* save ptr to new map */
	    *previous = live->opo_next; /* remove OPO_PERM node from
					** list so it will not be reused */
	    if (remove)
		global->ops_estate.opn_cocount--; /* decrement count of
					** available CO nodes since it is
					** being removed from the list */
	    else
	    {
		live->opo_next = saveptr;
		saveptr = live;
	    }
	    continue;
	}
	previous = &(*previous)->opo_next;	/* get ptr to next node */
    }

    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	&&
	subquery->ops_dist.opd_bestco)
    {   /* the array of CO trees require special processing to
	** make sure no shared subtrees exist */
	OPD_ISITE	siteno1;
	OPO_CO		*subtree;

	for (siteno1 = global->ops_gdist.opd_dv; --siteno1 >= 0;)
	{
	    OPO_CO	**copp1; /* ptr to ptr to CO node to consider
				** for copying */

	    copp1 = &subquery->ops_dist.opd_bestco->opd_colist[siteno1];
	    if (*copp1
		&&
		((*copp1)->opo_pointercnt > 1))
		/* copy the CO tree since two result sites share the
		** same plan */
		opo_subtree(subquery, copp1, &saveptr, remove);

	}
	/* traverse the list of CO nodes and copy any subtrees
	** which still are shared */
	for ( subtree = base; subtree; subtree = subtree->opo_coforw)
	{   
	    if (subtree->opo_pointercnt == 1)
	    {
		if (subtree->opo_outer
		    &&
		    (subtree->opo_outer->opo_pointercnt >= 2))
		    opo_subtree( subquery, &subtree->opo_outer, &saveptr, remove);
		if (subtree->opo_inner
		    &&
		    (subtree->opo_inner->opo_pointercnt >= 2))
		    opo_subtree( subquery, &subtree->opo_inner, &saveptr, remove);
	    }
	}
	for ( previous = &global->ops_estate.opn_colist; *previous; )
	    previous = &(*previous)->opo_next;	/* find end of list */
    }

    {
	/* At this point all nodes in the opn_colist contain OPN_PERM nodes which
	** are not used in the best CO tree... while saveptr contains a list of
	** OPN_PERM nodes which are used in the best CO tree */
	OPO_PERM    **endoflist;    /* will contain ptr to NULL ptr at end of
                                    ** OPN_PERM list */
	OPO_PERM    *permptr;	    /* used to traverse list of OPN_PERM struct 
				    */

	endoflist = previous;
	permptr = global->ops_estate.opn_colist;

	while (TRUE)
	{
	    while( (base->opo_coback == &co_okay))
	    {   /* this CO node is already outside of enumeration memory so
		** only need to update the inner and outer pointers 
		** - the postfix ordering guarantees the children have been
		** processed */
		OPO_CO	    *temp;

		base->opo_coback = NULL;
		if (base->opo_outer)
		    base->opo_outer = base->opo_outer->opo_coforw; /* copy the
					    ** forwarding address */
		if (base->opo_inner)
		    base->opo_inner = base->opo_inner->opo_coforw; /* copy the
					    ** forwarding address */
		temp = base;
		base = base->opo_coforw;
		temp->opo_coforw = temp;    /* point to self since no copy 
					    ** was required */
		if (!base)
		{   /* the end of the CO tree has been reached so that
		    ** this routine can exit */
		    *copp = temp;	    /* return the top of the new co
                                            ** tree */
		    *endoflist = saveptr;   /* return to OPN_PERM list so 
					    ** that next copy of a co tree 
					    ** can find them */
		    /*global->ops_copiedco = *copp;*/ /* save root of best CO, since
					    ** it is now copied out of enumeration
					    ** memory */
		    /* FIXME place remainder of CO nodes into CO free list
                    ** by using permptr */
		    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
			&&
			subquery->ops_dist.opd_bestco)
		    {   /* each bestco node copied out of enumeration memory has
			** "private" pointer to the site cost array, so that
			** site binding can be done at this point, for the
			** array of CO nodes, previously shared subtrees prevented
			** this from being done */
			OPD_ISITE	siteno2;
			for (siteno2 = global->ops_gdist.opd_dv; --siteno2 >= 0;)
			{
			    OPO_CO	*site2_cop;

			    site2_cop = subquery->ops_dist.opd_bestco->opd_colist[siteno2];
			    if (site2_cop
				&&
				!global->ops_gdist.opd_copied[siteno2])
			    {
				subquery->ops_dist.opd_bestco->opd_colist[siteno2]
				    = site2_cop->opo_coforw; /* need to place the
						** new CO tree which is out of enumeration
						** memory into the subquery list */
				(VOID) opd_msite(subquery, site2_cop->opo_coforw, siteno2, 
				    TRUE, &co_marker, remove); /* for 
						** distributed, mark each site
						** in the CO node, since the distributed
						** site cost arrays will be destroyed */
				global->ops_gdist.opd_copied[siteno2] = TRUE;
			    }
			}
		    }
		    return(E_DB_OK);
		}
	    }
	    STRUCT_ASSIGN_MACRO((*base), (permptr->opo_conode)); /* copy 
					    ** the structure out of enumeration
					    ** memory */
	    {   /* copy the equivalence class information */
		OPO_COMAPS	 *source;   /* ptr to equivalence class bit map
					    ** to copy */
		OPO_COMAPS       *dest;     /* ptr to destination of equivalence
					    ** class bit map */
		dest = &permptr->opo_mapcopy; /* equivalence class map out
					    ** of enumeration memory */
		source = permptr->opo_conode.opo_maps; /* equivalence class
					    ** map located somewhere in 
					    ** enumeration memory */
		MEcopy((PTR)source, sizeof(*dest), (PTR)dest); /* copy
					    ** map to non enumeration memory */
		permptr->opo_conode.opo_maps = dest; /* save ptr to new map */
	    }
	    base->opo_coforw = &permptr->opo_conode; /* copy forwarding address
                                            ** to original node */
	    base = &permptr->opo_conode;    /* place node in list so next pass
					    ** of loop will process it */
	    base->opo_coback = &co_okay;    /* mark node as being out of 
					    ** enumeration memory */
	    if (remove)
	    {
		global->ops_estate.opn_colist = permptr->opo_next;
		global->ops_estate.opn_cocount--; /* decrement count of
					** available CO nodes since it is
					** being removed from the list */
	    }
	    permptr = permptr->opo_next;
	}
    }

}

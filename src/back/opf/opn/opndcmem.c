/*
**Copyright (c) 2004 Ingres Corporation
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
#define             OPN_DCMEMORY        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNDCMEM.C - return CO structure to memory manager
**
**  Description:
**      routine to return CO structure to memory manager
**
**  History:    
**      29-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opn_tdcmemory	- delete a bestco tree
**
** Description:
**      This is a special case of deleting a CO tree which has been 
**      copied out of enumeration memory, and as a result does not
**      have any direct ptrs to any subtrees.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-may-88 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opn_tdcmemory(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    while (cop)
    {
	cop->opo_variant.opo_scost = NULL; /* if this node was copied out
				    ** of enumeration memory then the site
				    ** array will not exist */
	if (cop->opo_outer)
	    opn_tdcmemory(subquery, cop->opo_outer); /* recurse down the
				    ** outer */
	/* add CO to free list */
	cop->opo_coforw = subquery->ops_global->ops_estate.opn_cofree;
	subquery->ops_global->ops_estate.opn_cofree = cop; 
	cop = cop->opo_inner;	    /* iterate down inner */
    }
}

/*{
** Name: opn_dcmemory	- return CO structure to memory manager
**
** Description:
**      The routine will free an unused cost-ordering structure
**
**	In order to do some garbage collection we keep
**	counts of the number of people pointing to us.
**	This routine is used to decrement the usage counter
** 
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to CO structure to return
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
**	28-may-86 (seputis)
**          initial creation
{@history_line@}
[@history_line@]...
*/
VOID
opn_dcmemory(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    if (cop)
    {
        /* decrement counters of immediate children */
	if (cop->opo_inner)
	    cop->opo_inner->opo_pointercnt--;
	if (cop->opo_outer)
	    cop->opo_outer->opo_pointercnt--;

	if (cop == subquery->ops_global->ops_copiedco)
	{   /* free entire tree since it was created by copying into non
            ** enumeration memory, and now all the nodes are available */
	    subquery->ops_global->ops_copiedco = NULL;
	    opn_tdcmemory(subquery, cop);
	}
	else
	{
	    /* add CO to free list */
	    cop->opo_coforw = subquery->ops_global->ops_estate.opn_cofree;
	    subquery->ops_global->ops_estate.opn_cofree = cop; 
	}
    }
}

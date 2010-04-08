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
#define        OPN_EMEMORY      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNEMEMORY.C - get memory for a OPN_EQS element
**
**  Description:
**      routines to get memory for a OPN_EQS element
**
**  History:    
**      24-may-86 (seputis)    
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
{@history_line@}...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**/

/*{
** Name: opn_ememory	- get new OPN_EQS structure
**
** Description:
**      This routine is the "memory manager" for OPN_EQS structure,
**      It will be replace by the free space routines once they are
**      integrated into the memory manager.
**
**      If a free element is not available then this routine will allocate
**      a new OPN_EQS element 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    ptr to OPN_EQS element 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
OPN_EQS *
opn_ememory(
	OPS_SUBQUERY       *subquery)
{
    OPN_EQS		*freeqp;	    /* ptr to free OPN_EQS element */
    OPS_STATE           *global;

    global = subquery->ops_global;
    if (freeqp = global->ops_estate.opn_eqsfree)
    {	/* an element exists in the free list of OPN_EQSs so remove it */
	global->ops_estate.opn_eqsfree = freeqp->opn_eqnext;
    }
    else
    {	/* need to allocate a new OPN_EQS element */
	if (global->ops_mstate.ops_mlimit > global->ops_mstate.ops_memleft)
	{   /* check if memory limit has been reached prior to allocating
            ** another RLS node from the stream */
	    opn_fmemory(subquery, (PTR *)&global->ops_estate.opn_eqsfree);
	    if (freeqp = global->ops_estate.opn_eqsfree)
	    {   /* check if garbage collection has found any free nodes */
		global->ops_estate.opn_eqsfree = freeqp->opn_eqnext; /* deallocate from
						** free list */
            }
	}
	if (!freeqp)
	    freeqp = (OPN_EQS *) opn_memory(global, (i4) sizeof(OPN_EQS) );
				    /* allocate memory for OPN_EQS element */
    }

    return (freeqp);
}

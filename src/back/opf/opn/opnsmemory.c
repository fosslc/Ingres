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
#define        OPN_SMEMORY      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPHMEMORY.C - get memory for a OPN_SUBTREE element
**
**  Description:
**      routines to get memory for a OPN_SUBTREE element
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
** Name: opn_smemory	- get new OPN_SUBTREE structure
**
** Description:
**      This routine is the "memory manager" for OPN_SUBTREE structure,
**      It will be replace by the free space routines once they are
**      integrated into the memory manager.
**
**      If a free element is not available then this routine will allocate
**      a new OPN_SUBTREE element 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    ptr to OPN_SUBTREE element 
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
OPN_SUBTREE *
opn_smemory(
	OPS_SUBQUERY       *subquery)
{
    OPN_SUBTREE		*freest;	    /* ptr to free OPN_SUBTREE element*/
    OPS_STATE           *global;

    global = subquery->ops_global;
    if (freest = global->ops_estate.opn_stfree)
    {	/* an element exists in the free list of OPN_SUBTREEs so remove it */
	global->ops_estate.opn_stfree = 
	    (OPN_SUBTREE *)freest->opn_coforw; /* remove it from the 
					    ** list */
    }
    else
    {	/* need to allocate a new OPN_SUBTREE element */
	if (global->ops_mstate.ops_mlimit > global->ops_mstate.ops_memleft)
	{   /* check if memory limit has been reached prior to allocating
            ** another RLS node from the stream */
	    opn_fmemory(subquery, (PTR *)&global->ops_estate.opn_stfree); /* 
					** attempt to free unused subtree 
                                        ** structures */
	    if (freest = global->ops_estate.opn_stfree)
	    {   /* check if garbage collection has found any free nodes */
		global->ops_estate.opn_stfree = 
		    (OPN_SUBTREE *)freest->opn_coforw; /* deallocate from
					    ** free list */
            }
	}
	if (!freest)
	    freest = (OPN_SUBTREE *) opn_memory( subquery->ops_global, 
		(i4) sizeof(OPN_SUBTREE) );    /* allocate memory for OPN_SUBTREE 
					    ** element */
    }

    return (freest);
}

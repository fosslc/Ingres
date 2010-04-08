/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
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
#define        OPN_RMEMORY      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNRMEMORY.C - return free OPN_RLS structure
**
**  Description:
**      Memory management routine for OPN_RLS structures.
**
**  History:    
**      26-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	24-jan-1994 (gmanning)
**	    Added #include of me.h in order to compile on NT.
[@history_line@]...
**/

/*{
** Name: opn_rmemory	- OPN_RLS memory management routines
**
** Description:
**      This routine returns the next free OPN_RLS structure.
**      FIXME - this routine should be replaced by a general memory
**      management routine in the ULM.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    ptr to newly allocated OPN_RLS structure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-86 (seputis)
**          initial creation
**	21-feb-91 (seputis)
**	    - part of 33279 fix, added initialization of opn_boolfact
**	17-may-93 (ed)
**	    - initialize opn_mask for bug 47377
[@history_line@]...
*/
OPN_RLS *
opn_rmemory(
	OPS_SUBQUERY         *subquery)
{
    OPS_STATE           *global;	/* ptr to global state variable */
    OPN_RLS             *trl;		/* newly allocated OPN_RLS structure */

    global = subquery->ops_global;      /* get global state variable */
    if (trl = global->ops_estate.opn_rlsfree)
    {
	global->ops_estate.opn_rlsfree = trl->opn_rlnext; /* deallocate from
					** free list */
    }
    else
    {
	if (global->ops_mstate.ops_mlimit > global->ops_mstate.ops_memleft)
	{   /* check if memory limit has been reached prior to allocating
            ** another RLS node from the stream */
	    opn_fmemory(subquery, (PTR *)&global->ops_estate.opn_rlsfree);
	    if (trl = global->ops_estate.opn_rlsfree)
	    {   /* check if garbage collection has found any free nodes */
		global->ops_estate.opn_rlsfree = trl->opn_rlnext; /* deallocate from
						** free list */
            }
	}
	if (!trl)
	    trl = (OPN_RLS *) opn_memory( global, (i4) sizeof( OPN_RLS ) ); /*
					** get new element if none available */
    }
    MEfill(sizeof(*trl), (u_char)0, (PTR)trl); /* initialize structure */
    trl->opn_reltups = 0.0;
    return (trl);
}

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
#define        OPU_MEMORY      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPUMEMORY.C - get joinop memory
**
**  Description:
**      Routines to get joinop memory
**
**  History:    
**      16-jun-86 (seputis)    
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
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
PTR opu_memory(
	OPS_STATE *global,
	i4 size);
void opu_mark(
	OPS_STATE *global,
	ULM_RCB *ulmrcb,
	ULM_SMARK *mark);
void opu_release(
	OPS_STATE *global,
	ULM_RCB *ulmrcb,
	ULM_SMARK *mark);

/*{
** Name: opu_memory	- get joinop memory
**
** Description:
**      This routine will allocate the requested size of memory from the 
**      joinop memory stream.  Memory in this stream is not deallocated 
**      until the optimization has completed.  The allocated memory will
**      be aligned for any datatype.
**
** Inputs:
**      global                          ptr to global state variable
**      size                            size of memory block requested.
**
** Outputs:
**	Returns:
**	    PTR to aligned memory of "size" bytes
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-86 (seputis)
**          initial creation
**	4-mar-91 (seputis)
**	    make initialization of memory an xDEBUG feature
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
**	5-oct-2007 (dougi)
**	    Accumulate memory acquisition stats.
*/
PTR
opu_memory(
	OPS_STATE          *global,
	i4                size)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    global->ops_mstate.ops_countalloc++;
    if (size >= 2048)
	global->ops_mstate.ops_count2kalloc++;
    global->ops_mstate.ops_totalloc += size;

    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_streamid; /* allocate memory
					    ** from the global stream */
    global->ops_mstate.ops_ulmrcb.ulm_psize = size;    /* size of request */
    ulmstatus = ulm_palloc( &global->ops_mstate.ops_ulmrcb );
    if (DB_FAILURE_MACRO(ulmstatus))
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0, 0, 0, 0, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
#ifdef xDEBUG
    MEfill( size, (u_char)247, (PTR)global->ops_mstate.ops_ulmrcb.ulm_pptr); /*FIXME
                                            ** remove this initialization after
                                            ** test for uninitialized memory
                                            ** is not required any more */
#endif
    return( global->ops_mstate.ops_ulmrcb.ulm_pptr );  /* return the allocated
					    ** memory */
}

/*{
** Name: OPU_MARK	- Mark a point in the stack ULM memory stream
**
** Description:
**      This routine marks a point in the ULM memory stream. The mark may
**      be later used to reclaim unused memory. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**      ulmrcb -
**          address of local control block, or NULL if
**	    global->ops_mstate.ops_ulmrcb should be used
**	mark -
**	    Pointer to uninitialized memory of sufficient size.
**
** Outputs:
**	mark -
**	    Filled in with info marking the current point in the memory stream.
**
**	Returns:
**	    Nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-apr-88 (seputis)
**          initial creation
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_mark(
	OPS_STATE   *global,
	ULM_RCB     *ulmrcb,
	ULM_SMARK   *mark)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    if (!ulmrcb)
    {
	ulmrcb = &global->ops_mstate.ops_ulmrcb; /* use global ulmrcb if
					    ** control block is not defined */
	ulmrcb->ulm_streamid_p = &global->ops_mstate.ops_streamid; 
					    /* mark memory
					    ** in the global stream */
    }
    /* store the mark to be initialized */
    ulmrcb->ulm_smark = mark;

    ulmstatus = ulm_mark( ulmrcb );
    if ( DB_FAILURE_MACRO(ulmstatus) )
    {
	if (ulmrcb->ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0, 0, 0, 0, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		ulmrcb->ulm_error.err_code); /* check for error */
#endif
    }	
}

/*{
** Name: OPU_RELEASE	- Return memory to the global memory pool
**
** Description:
**      This routine returns memory from the given mark to the end of the
**	ULM memory stream to the global memory pool.
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**      ulmrcb -
**          address of local control block, or NULL if
**	    global->ops_mstate.ops_ulmrcb should be used
**	mark -
**	    The mark stating where to start returning memory.
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory is returned to the global memory pool
**
** History:
**      18-apr-87 (seputis)
**          initial creation
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_release(
	OPS_STATE   *global,
	ULM_RCB     *ulmrcb,
	ULM_SMARK   *mark)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    if (!ulmrcb)
    {
	ulmrcb = &global->ops_mstate.ops_ulmrcb; /* use global ulmrcb if
					    ** control block is not defined */
	ulmrcb->ulm_streamid_p = &global->ops_mstate.ops_streamid; 
					    /* mark memory
					    ** in the global stream */
    }
    /* store the mark to be initialized */
    ulmrcb->ulm_smark = mark;

    ulmstatus = ulm_reclaim( ulmrcb );
    if ( DB_FAILURE_MACRO(ulmstatus) )
    {
	if (ulmrcb->ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0, 0, 0, 0, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		ulmrcb->ulm_error.err_code); /* check for error */
#endif
    }	
}

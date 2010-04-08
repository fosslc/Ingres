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
#define        OPU_SMEMORY      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPUSMEMORY.C - Routines for stack style memory allocation.
**
**  Description:
**      These routines are intended to be used for the allocation of 
**      data structures that normally would be allocated off of the  
**      stack, but can be because they are variable in size. 
[@comment_line@]...
**
**          opu_Osmemory_open() - Open the stack ULM memory stream.
**          opu_Csmemory_close() - Close the stack ULM memory stream.
**          opu_Gsmemory_get() - Get memory from the stack ULM memory stream.
**          opu_Msmemory_mark() - Mark a point in the stack ULM memory stream.
**          opu_Rsmemory_reclaim() - Reclaim memory past a mark.
[@func_list@]...
**
**
**  History:    
**      19-Jul-87 (eric)
**          created
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
[@history_template@]...
**/


/*
**  Forward and/or External typedef/struct references.
*/

/*
**  Forward and/or External function references.
*/


/*
**  Defines of other constants.
*/


/*
**  Definition of static variables and forward static functions.
*/


/*{
** Name: OPU_OSMEMORY_OPEN	- Initialize the stack ULM memory stream
**
** Description:
**      This routine initializes the ULM memory stream that will be used 
**      for the stack style memory allocation and deallocation. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**	global->ops_mstate.ops_ulmrcb -
**	    The ULM control block.
**
** Outputs:
**	global->ops_mstate.ops_sstreamid -
**	    The new stream id.
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
**      19-July-87 (eric)
**          written
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_Osmemory_open(
	OPS_STATE   *global)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    /* Set the output streamid location for ULM */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;

    if ( (ulmstatus = ulm_openstream( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
}

/*{
** Name: OPU_CSMEMORY_CLOSE	- Close the stack ULM memory stream
**
** Description:
**      This routine closes the ULM memory stream that was used 
**      for the stack style memory allocation and deallocation. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**	global->ops_mstate.ops_ulmrcb -
**	    The ULM control block.
**	global->ops_mstate.ops_sstreamid -
**	    The stream id.
**
** Outputs:
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
**      19-July-87 (eric)
**          written
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_Csmemory_close(
	OPS_STATE   *global)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    /* store the stream id */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;
    /* ULM will nullify ops_sstreamid */

    if ( (ulmstatus = ulm_closestream( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
}

/*{
** Name: OPU_GSMEMORY_CLOSE	- Get memory from the stack ULM memory stream
**
** Description:
**      This routine allocates memory from the ULM memory stream that is used 
**      for the stack style memory usage.
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**	global->ops_mstate.ops_ulmrcb -
**	    The ULM control block.
**	global->ops_mstate.ops_sstreamid -
**	    The stream id.
**	size -
**	    size of the piece of memory to allocate.
**
** Outputs:
**
**	Returns:
**	    The address of the allocated memory.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-July-87 (eric)
**          written
**      21-feb-91 (seputis)
**          make non-zero initialization of memory an xDEBUG feature
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_line@]...
[@history_template@]...
*/
PTR
opu_Gsmemory_get(
	OPS_STATE   *global,
	i4	    size)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    /* store the stream id */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;

    /* store the size to be allocate */
    global->ops_mstate.ops_ulmrcb.ulm_psize = size;

    if ( (ulmstatus = ulm_palloc( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
#ifdef xDEBUG
    MEfill( size, (u_char)127, (PTR)global->ops_mstate.ops_ulmrcb.ulm_pptr); /*FIXME
                                            ** remove this initialization after
                                            ** test for uninitialized memory
                                            ** is not required any more */
#endif
    /* return the allocated memory */
    return( global->ops_mstate.ops_ulmrcb.ulm_pptr );
}

/*{
** Name: OPU_MSMEMORY_MARK	- Mark a point in the stack ULM memory stream
**
** Description:
**      This routine marks a point in the ULM memory stream. The mark may
**      be later used to reclaim unused memory. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**	global->ops_mstate.ops_ulmrcb -
**	    The ULM control block.
**	global->ops_mstate.ops_sstreamid -
**	    The stream id.
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
**      19-July-87 (eric)
**          written
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_Msmemory_mark(
	OPS_STATE   *global,
	ULM_SMARK   *mark)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    /* store the stream id */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;

    /* store the mark to be initialized */
    global->ops_mstate.ops_ulmrcb.ulm_smark = mark;

    if ( (ulmstatus = ulm_mark( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
}

/*{
** Name: OPU_RSMEMORY_RECLAIM	- Return memory to the global memory pool
**
** Description:
**      This routine returns memory from the given mark to the end of the
**	ULM memory stream to the global memory pool.
[@comment_line@]...
**
** Inputs:
**	global -
**	    State info for the current query.
**	global->ops_mstate.ops_ulmrcb -
**	    The ULM control block.
**	global->ops_mstate.ops_sstreamid -
**	    The stream id.
**	mark -
**	    The mark stating where to start returning memory.
**
** Outputs:
**
**	Returns:
**	    The address of the allocated memory.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory is returned to the global memory pool
**
** History:
**      19-July-87 (eric)
**          written
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_template@]...
*/
VOID
opu_Rsmemory_reclaim(
	OPS_STATE   *global,
	ULM_SMARK   *mark)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */

    /* store the stream id */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;

    /* store the mark to be initialized */
    global->ops_mstate.ops_ulmrcb.ulm_smark = mark;

    if ( (ulmstatus = ulm_reclaim( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
    {
	if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	}
#ifdef E_OP0093_ULM_ERROR
	else
	    opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for error */
#endif
    }	
}

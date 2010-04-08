/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tm.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dm0llctx.h>

/**
** Name: DMCFABRT.C  - The force abort thread of the server
**
** Description:
**    
**      dmc_force_abort()  -  The force abort thread of the server
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Function prototypes.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <lgdef.h>.
**	19-oct-1993 (rmuth)
**	    Remove <lgdef.h> this has a GLOBALREF in it which produces a
**	    link warning on VMS.
**	31-jan-1994 (bryanp) B58035
**	    Set dmc_cb->error.err_code if LGalter fails.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/*{
**
** Name: dmc_force_abort	-- Performs force abort tasks in the server
**
** EXTERNAL call format:	status = dmf_call(DMC_FORCE_ABORT, &dmc_cb);
**
** Description:
**	This routine implements the force abort thread in a server.
**
**	If the log file should become full, the logging system will request that
**	the oldest active transaction abort itself. The logging system does
**	this by awakening the force abort thread, which then "calls back" the
**	SCF-provided force abort handler subroutine to notify the active
**	thread that it must abort itself.
**
**	Then the force abort thread goes back to sleep until the next time
**	that a force abort is required.
**
**	If all goes well, this routine will not return under normal
**	circumstances until server shutdown time.
**
** Inputs:
**     dmc_cb
** 	.type		    Must be set to DMC_CONTROL_CB.
** 	.length		    Must be at least sizeof(DMC_CB).
**
** Outputs:
**     dmc_cb
** 	.error.err_code	    One of the following error numbers.
**			    E_DB_OK
**			    E_DM004A_INTERNAL_ERROR
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**	31-jan-1994 (bryanp) B58035
**	    Set dmc_cb->error.err_code if LGalter fails.
*/
DB_STATUS
dmc_force_abort(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    i4	    	    error_code;
    DB_STATUS	    status = E_DB_OK;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;

#ifdef xDEBUG
    TRdisplay("Starting server Force Abort Thread for server id 0x%x\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    /*
    ** Notify LG that this process has a force abort thread now:
    */
    cl_status = LGalter(LG_A_FABRT_SID, (PTR)&dmf_svcb->svcb_lctx_ptr->lctx_lgid,
			sizeof(LG_LGID), &sys_err);
    if (cl_status)
    {
	uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	SETDBERR(&dmc->error, 0, E_DM014C_FORCE_ABORT);
	return (E_DB_ERROR);
    }

    for (;;)
    {
	/*
	** Wait for a thread to need force aborting.
	*/
	cl_status = CSsuspend(CS_INTERRUPT_MASK, 0, 0);

	if (cl_status == E_CS0008_INTERRUPTED)
	{
	    /*
	    ** Special threads are interrupted when they should shut down.
	    */

	    /* display a message? */
	    status = E_DB_OK;
	    break;
	}

	/*
	** The force abort thread is awakened when a thread in this server
	** needs to be aborted. It then calls LG_force_abort which will locate
	** the actual thread's session ID and call-back the SCF handler with
	** the appropriate thread ID.
	*/
	cl_status = LG_force_abort(&sys_err);

	if (cl_status)
	{
	    /*
	    ** Something is fatally wrong in the CL.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM014C_FORCE_ABORT);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** loop back around and wait for the next time to abort a thread:
	*/
    }

    return (status);
}

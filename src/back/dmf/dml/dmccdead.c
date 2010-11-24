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
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
/* these to get dml.h */
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>

/**
** Name: DMCCDEAD.C  - The check_dead thread of the recovery server
**
** Description:
**    
**      dmc_check_dead()  -  The check_dead thread of the server
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**	8-oct-1992 (bryanp)
**	    Make the check-dead interval configurable.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Function protoypes.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <lgdef.h>, <di.h>
**	19-oct-1993 (rmuth)
**	    Remove <lgdef.h> this has a GLOBALREF in it which produces a
**	    link warning on VMS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-apl-2004 (mutma03)
** 	    code to wakeup sleeping threads which missed AST
**	7-Sep-2004 (mutma03)
** 	    Removed the call to CSpoll_for_AST for missed ASTs which is
**	    taken care with CSsuspend_for_AST/CSresume_from_AST
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include dml.h to get dmc_check_dead() prototype.
**/

/*{
**
** Name: dmc_check_dead	    -- Performs check_dead tasks in the server
**
** EXTERNAL call format:	status = dmf_call(DMC_CDEAD_THREAD, &dmc_cb);
**
** Description:
**	This routine implements the check_dead thread in a server, whose
**	job it is to periodically call the logging system to check for any
**	dead processes. If a dead process is found, the logging system then
**	schedules cleanup processing for that process.
**
**	If all goes will, this routine will not return under normal
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
**	8-oct-1992 (bryanp)
**	    Make the check-dead interval configurable.
**	4-Apr-2004 (mutma03)
**	    called CSpoll_for_AST periodically to signal the thread
**	which missed an AST due to suspend/resume race condition.
**	30-Apr-2004 (mutma03)
**	    Excluded CSpoll_for_AST for non-linux builds.
*/

DB_STATUS
dmc_check_dead(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    i4	    	    error_code;
    i4		    check_dead_timeout = svcb->svcb_check_dead_interval;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;

#ifdef xDEBUG
    TRdisplay("Starting server check_dead Thread for server id 0x%x\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    /*
    ** Need to get the timeout value passed in from SCF.
    */

    for (;;)
    {
	cl_status = CSsuspend(CS_TIMEOUT_MASK | CS_INTERRUPT_MASK,
				check_dead_timeout, 0);

	if (cl_status == E_CS0008_INTERRUPTED)
	{
	    /*
	    ** Special threads are interrupted when they should shut down.
	    */

	    /* display a message? */
	    break;
	}

	/*
	** Normally, we wake up because we time out. However, it is not an
	** error for us to be explicitly woken up by the Logging System because
	** it wants us to perform a check-dead pass.
	*/
	if (cl_status != OK && cl_status != E_CS0009_TIMEOUT)
	{
	    /*
	    ** Something is fatally wrong in the CL.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM014A_CHECK_DEAD);
	    break;
	}

	/*
	** Each time we wake up, go check for dead processes:
	*/
	cl_status = LG_check_dead(&sys_err);
	if (cl_status)
	{
	    /*
	    ** Something is fatally wrong in the Logging System.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM014A_CHECK_DEAD);
	    break;
	}
	/*
	** loop back around and wait for the next time to check for dead
	** processes.
	*/
    }

    return( (dmc->error.err_code) ? E_DB_ERROR : E_DB_OK );
}

/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dmfrcp.h>

/**
** Name: DMCRCVRY.C  - The recovery thread of the recovery server
**
** Description:
**    
**      dmc_recovery()  -  The recovery thread of the recovery server
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Function protoypes.
**	26-apr-1993 (bryanp)
**	    Call dmr_check_event with a transaction ID of 0, indicating that
**	    dmr_check_event should use its local transaction ID for waiting on
**	    events.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dmfrcp.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/*
**  Definition of static variables and forward static functions.
*/


/*{
**
** Name: dmc_recovery	    -- Performs recovery tasks in the recovery server
**
** EXTERNAL call format:	status = dmf_call(DMC_RECOVERY_THREAD, &dmc_cb);
**
** Description:
**	This routine implements the recovery thread in a recovery server, whose
**	job it is to wait for recovery actions to be needed and then perform
**	them. If all goes will, this routine will not return under normal
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
**	26-apr-1993 (bryanp)
**	    Call dmr_check_event with a transaction ID of 0, indicating that
**	    dmr_check_event should use its local transaction ID for waiting on
**	    events.
*/
DB_STATUS
dmc_recovery(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    i4	    	    error_code;
    DB_STATUS	    status = E_DB_OK;

#ifdef xDEBUG
    TRdisplay("Starting server Recovery Thread for server id 0x%x\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    /*	Wait around for something to do. */

    while (status == E_DB_OK)
    {
	status = dmr_check_event(NULL, 0, 0);
    }

    /*
    ** E_DB_INFO from dmr_check_event means that LGevent was interrupted,
    ** which in turn means that the server is shutting down.
    */
    if (status == E_DB_INFO)
	status = E_DB_OK;

    if (status != E_DB_OK)
	SETDBERR(&dmc->error, 0, E_DM014E_RECOVERY_THREAD);
    return (status);
}

/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <me.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <lgdstat.h>
#include    <lgdef.h>
#include    <lgkdef.h>
#include    <dm0llctx.h>

/**
** Name: DMCCPTMR.C  - The Consistency Point timer thread of the Recovery server
**
** Description:
**    
**      dmc_cp_timer()  -  The CP Timer thread of the Recovery server
**
** History:
**	18-jan-1993 (bryanp)
**	    Working on the new portable logging and locking system
**	26-apr-1992 (bryanp)
**	    Put the TRdisplays into xDEBUG ifdefs, because people don't like
**	    them.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rogerk)
**	    Made the Consistency Point timer not signal new consistency points
**	    if the system is in Logfull state or if no transactions have ended
**	    since the last time it woke up and signaled a Consistency Point.
**	31-jan-1994 (bryanp) B57939, B57940
**	    Log LG return status if an LG call fails.
**	    Conditionally set error_code if an error occurs during cleanup.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	30-Nov-1999 (jenjo02)
**	    The CP Timer thread has no use for a lock list. Removed call
**	    to LKcreate_list().
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**/

/*{
**
** Name: dmc_cp_timer	-- Performs CP Timer tasks in the server
**
** EXTERNAL call format:	status = dmf_call(DMC_CP_TIMER, &dmc_cb);
**
** Description:
**	This routine implements the Consistency Point timer thread in a server.
**
**	If CPTIMER is not 0, then it is interpreted as a number of seconds;
**	every N seconds a new consistency point is requested. These consistency
**	points occur in addition to the consistency point which normally are
**	taken when a specified percentage of the log file fills up.
**
**	If the user desires to have log file percentage-driven consistency
**	points only, and no timer driven consistency points, the user should
**	specify CPTIMER = 0 for the Recovery Server. If the user desires to
**	have timer-driven consistency points only, and no percentage-driven
**	consistency points, the user should reconfigure the logging system with
**	a large value for the percentage of the log file to signal a
**	consistency point, and should specify CPTIMER=N for the Recovery Server.
**
**	The Consistency Point timer will not signal new consistency points if
**	the system is in Logfull state or if no transactions have ended since
**	the last time it woke up and signaled a Consistency Point.
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
**	18-jan-1993 (bryanp)
**	    Working on the new portable logging and locking system
**	26-apr-1992 (bryanp)
**	    Put the TRdisplays into xDEBUG ifdefs, because people don't like
**	    them.
**      20-sep-1993 (smc)
**          Made TRdisplay use %p for pointer.
**	18-oct-1993 (rogerk)
**	    Made the Consistency Point timer not signal new consistency points
**	    if the system is in Logfull state or if no transactions have ended
**	    since the last time it woke up and signaled a Consistency Point.
**	10-oct-93 (swm)
**	    Bug #56438
**	    Put LG_DBID into automatic variable lg_dbid rather than overloading
**	    dmc_cb->dmc_db_id.
**	31-jan-1994 (bryanp) B57939, B57940
**	    Log LG return status if an LG call fails.
**	    Conditionally set error_code if an error occurs during cleanup.
**	24-feb-1994 (rogerk)
**	    Removed duplicated code block around LGevent call.
**	    Looks like there was an integration merge error at some point.
**      16-Aug-1999 (wanya01)
**          Bug 96106
**          Archiver now wakes up when consistency point is forced by the cp_timer 
**	30-Nov-1999 (jenjo02)
**	    The CP Timer thread has no use for a lock list. Removed call
**	    to LKcreate_list().
*/
DB_STATUS
dmc_cp_timer(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    i4	    error_code = E_DM014F_CP_TIMER;
    DB_STATUS	    status = E_DB_OK;
    i4		    cp_timeout = svcb->svcb_cp_timer;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4		    item;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    i4		    len_add_info;
    i4		    have_transaction = FALSE;
    STATUS	    stat;
    i4	    error;
    DB_OWN_NAME	    user_name;
    i4	    event;
    LG_STAT	    lgstats;
    i4	    length;
    i4	    lgd_status;
    i4	    system_commits = 0;
    LG_DBID	    lg_dbid;
    LGD             *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    i4              cp_count = 0;

#ifdef xDEBUG
    TRdisplay("Starting Consistency Point Timer Thread for server id 0x%p\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    if (cp_timeout == 0)
    {
	/*
	** CPTIMER thread should not have been started if the CPTIMER value
	** was 0.
	*/
	SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	return (E_DB_ERROR);
    }

    STmove((PTR)DB_CPTIMER_THREAD, ' ', sizeof(add_info.ad_dbname),
	(PTR) &add_info.ad_dbname);
    MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	(PTR) &add_info.ad_dbowner);
    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
    add_info.ad_dbid = 0;
    add_info.ad_l_root = 4;
    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

    stat = LGadd(dmf_svcb->svcb_lctx_ptr->lctx_lgid, LG_NOTDB, (char *)&add_info,
		    len_add_info, &lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 4, 0,
	    dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	    sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
	    sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
	    4, (PTR) &add_info.ad_root);
	if (stat == LG_EXCEED_LIMIT)
	    SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	return (E_DB_ERROR);
    }

    for (;;)
    {
    	if (stat != OK)
	    break;

	STmove((PTR)DB_CPTIMER_THROWN, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT, lg_dbid, &tran_id, &lx_id,
			sizeof(DB_OWN_NAME), (char *)&user_name, 
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &error, 0);
	    uleFormat(&dmc->error, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, 
	    	NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    status = E_DB_ERROR;
	    break;
	}
	have_transaction = TRUE;
	break;
    }

    for (;status == E_DB_OK;)
    {
	cl_status = CSsuspend(CS_TIMEOUT_MASK | CS_INTERRUPT_MASK,
				cp_timeout, 0);

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
	** Normally, we wake up because we time out. Thus an OK return code is
	** an error, because it means that somebody explicitly woke us up.
	*/
	if (cl_status != E_CS0009_TIMEOUT)
	{
	    /*
	    ** Something is fatally wrong in the CL.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Get logging system information to determine whether or not to
	** signal a new Consistency Point:
	**
	**   - Transaction End count
	**   - System Logfull Status
	*/
	cl_status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, 
                        sizeof(lgd_status), &length, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error_code, 1, 
		0, LG_S_LGSTS);
	    SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	    status = E_DB_ERROR;
	    break;
	}

	cl_status = LGshow(LG_S_STAT, (PTR)&lgstats, sizeof(lgstats), 
			    &length, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error_code, 1, 
		0, LG_S_STAT);
	    SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** If new transactions have been run since the last time we forced
	** a consistency point and the system is not in logfull state, then
	** fire off another one.
	*/
	if ((lgstats.lgs_end != system_commits) && 
	    ((lgd_status & LGD_LOGFULL) == 0))
	{
	    /* Save new commit count */
	    system_commits = lgstats.lgs_end;

#ifdef xDEBUG
	    TRdisplay("%@ RCP: %d seconds elapsed, signal Consistency Point\n",
		    cp_timeout);
#endif

	    /*
	    ** Signal the Consistency Point and then wait for its completion.
	    **/

	    item = 1;
	    cl_status = LGalter(LG_A_CPNEEDED, (PTR)&item, sizeof(item),
				&sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error_code,
		    1, 0, LG_A_CPNEEDED);
		SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
		status = E_DB_ERROR;
		break;
	    }

	    cl_status = LGevent(0, lx_id, LG_E_ECPDONE, &event, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error_code,
		    1, 0, LG_E_ECPDONE);
		SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
		status = E_DB_ERROR;
		break;
	    }
            else 
                cp_count++;

            if ( cp_count >= lgd->lgd_local_lfb.lfb_header.lgh_cpcnt )  
            {
                cl_status = LGalter(LG_A_ARCHIVE, (PTR)0, 0, &sys_err);
                if (cl_status != OK)
                {
                   uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error_code, 0);
                   uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error_code,
                            1, 0, LG_A_ARCHIVE);
		   SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
                   status = E_DB_ERROR;
                   break;
                }
                else
                   cp_count = 0;
             }
	}

	/*
	** loop back around and wait for the next time to signal a CP
	*/
    }

    /*
    ** Clean up transaction list left hanging around.
    */
    if (have_transaction)
    {
	stat = LGend(lx_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if (status == E_DB_OK)
	    {
		status = E_DB_ERROR;
		SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	    }
	}
    }

    stat = LGremove(lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	if (status == E_DB_OK)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&dmc->error, 0, E_DM014F_CP_TIMER);
	}
    }

    return (status);
}

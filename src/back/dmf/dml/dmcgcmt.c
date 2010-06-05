/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
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
** Name: DMCGCMT.C  - The group commit thread of the server
**
** Description:
**    
**      dmc_group_commit()  -  The group commit thread of the server
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	20-oct-1992 (bryanp)
**	    Group Commit sleep interval is now configurable.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Function protoypes.
**	26-apr-1993 (bryanp)
**	    Fix parameter passing errors revealed by prototyping LK.
**	21-jun-1992 (mikem)
**	    Added xDEBUG logging of group commit timer parameters to dmc_group_commit.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <lgdef.h>.
**	19-oct-1993 (rmuth)
**	    Remove <lgdef.h> this has a GLOBALREF in it which produces a
**	    link warning on VMS.
**	31-jan-1994 (bryanp) B58040
**	    Set error_code if an error occurs during cleanup
**	03-Feb-1998 (jenjo02)
**	    The group commit thread has no use for a lock list,
**	    removed call to LKcreate_list().
**	    Modified to use log transaction id instead of log process,
**	    do ms sleeps in LG_pgyback_write() instead of here.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
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
** Name: dmc_group_commit	-- Performs group commit tasks in the server
**
** EXTERNAL call format:	status = dmf_call(DMC_GROUP_COMMIT, &dmc_cb);
**
** Description:
**	This routine implements the group commit thread in a server.
**
**	The group commit thread wakes up when at least one user thread is
**	ready for the current log buffer to be written. Rather than write the
**	buffer immediately, we awaken the group commit thread. The group
**	commit thread then "watches" the buffer while allowing additional
**	threads to continue to write to it.
**
**	If the buffer becomes full, it is written out by the thread which
**	fills it. If it does not become full within a "reasonable" time, the
**	group commit thread will write the buffer out.
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
**	20-oct-1992 (bryanp)
**	    Group Commit sleep interval is now configurable.
**	21-jun-1992 (mikem)
**	    Added xDEBUG logging of group commit timer parameters to dmc_group_commit.
**	10-oct-93 (swm)
**	    Bug #56438
**	    Put LG_DBID into automatic variable lg_dbid rather than overloading
**	    dmc_cb->dmc_db_id.
**	31-jan-1994 (bryanp) B58040
**	    Set error_code if an error occurs during cleanup
**	03-Feb-1998 (jenjo02)
**	    The group commit thread has no use for a lock list,
**	    removed call to LKcreate_list().
**	    Modified to use log transaction id instead of log process,
**	    do ms sleeps in LG_pgyback_write() instead of here.
**	28-Apr-2010 (jonj) B123649
**	    Change 494685 for SIR 120874 inadvertantly stopped
**	    group commit from functioning.
**	    "if ( status = E_DB_OK )" isn't as useful as "if ( status == E_DB_OK )"
*/
DB_STATUS
dmc_group_commit(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    DB_STATUS	    status = E_DB_OK;
    i4		    timer_interval_ms = svcb->svcb_gc_interval;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    i4		    len_add_info;
    i4		    have_transaction = FALSE;
    STATUS	    stat;
    i4	    error;
    DB_OWN_NAME	    user_name;
    LG_DBID	    lg_dbid;

#ifdef xDEBUG
    TRdisplay("Starting server Group Commit Thread for server id 0x%x, with interval %d\n",
	dmc->dmc_id, timer_interval_ms);
#endif

    CLRDBERR(&dmc->error);

    /*
    ** Need to get timer_interval_ms, other parameters from SCF.
    */

    STmove((PTR)DB_GROUPCOMMIT_THREAD, ' ', sizeof(add_info.ad_dbname),
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
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 4, 0,
	    dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	    sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
	    sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
	    4, (PTR) &add_info.ad_root);
	if (stat == LG_EXCEED_LIMIT)
	    SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	return (E_DB_ERROR);
    }

    for (;;)
    {
    	if (stat != OK)
	    break;

	STmove((PTR)DB_GROUPCOMMIT_THROWN, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT, lg_dbid, &tran_id, &lx_id,
			sizeof(DB_OWN_NAME), (char *)&user_name, 
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	    status = E_DB_ERROR;
	    break;
	}
	have_transaction = TRUE;
	break;
    }

    /*
    ** Notify LG that this process has a Group Commit thread now:
    */
    if ( status == E_DB_OK )
    {
	cl_status = LGalter(LG_A_GCMT_SID, (PTR)&lx_id,
			    sizeof(lx_id), &sys_err);
	if (cl_status)
	{
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	    status = E_DB_ERROR;
	}
    }

    for (;status == E_DB_OK;)
    {
	/*
	** Wait for a buffer to need watching:
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
	** The group commit thread is awakened when the current log buffer
	** needs "watching".
	** It then calls LG_pgyback_write to monitor the current buffer,
	** which returns here when there is no longer a current buffer to watch.
	*/
	status = E_DB_OK;

	cl_status = LG_pgyback_write(lx_id, timer_interval_ms, &sys_err);

	if (cl_status != LG_NO_BUFFER)
	{
	    /*
	    ** Something is fatally wrong in the CL.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** loop back around and wait for the next time to watch a buffer:
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
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if (status == E_DB_OK)
	    {
		status = E_DB_ERROR;
		SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	    }
	}
    }

    stat = LGremove(lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	if (status == E_DB_OK)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&dmc->error, 0, E_DM014B_GROUP_COMMIT);
	}
    }

    return (status);
}

/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpecpn.h>
#include    <dm2t.h>
#include    <dm2d.h>
#include    <dm0s.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dmxe.h>
#include    <dmfgw.h>
#include    <ddb.h>		/* Buncha stuff needed just for rdf.h */
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>

/**
** Name: DMCEND.C - Functions used to end a session.
**
** Description:
**      This file contains the functions necessary to end a session.
**      This file defines:
**    
**      dmc_end_session()  -  Routine to perform the end 
**                            session operation.
**
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.      
**	18-nov-1985 (derek)
**	    Completed code for dmc_end_session().
**	26-jan-1989 (roger)
**	    LGend() and ule_format() were being called with (i4*) error
**	    instead of correct (CL_ERR_DESC*) in release_cb().
**	 1-Feb-1989 (ac)
**	    Added 2PC support.
**	 3-apr-1989 (rogerk)
**	    Make sure return with correct error status on error.
**	    Pass correct parm length to ule_format.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	20-Oct-1992 (daveb)
**	    remove call to dmf_gws_term; it's done by SCF now.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Changed handling of dmc_end while there
**	    is an open transaction to call dmxe_pass_abort.  The old code
**	    did not account for all the cache protocols needed to request an
**	    abort action by the RCP.
**	11-jan-1993 (mikem)
**	    Fix "used before set" problem in release_cb().  Initialized odcb
**	    before using it in error path through the code.  
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fixed error handling in release_cb following dmxe_pass_abort call.
**	31-jan-1994 (bryanp) B58032, B58033.
**	    Log scf_error.err_code if scf_call fails.
**	    Log LKrelease return code if LKrelease fails.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_destroy_temp_tcb() calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	    DML_SCB is now embedded in SCF's SCB and not
**	    separately dm0m_allocate/deallocate-d.
**	5-Feb-2004 (schka24)
**	    Toss temp tables out of RDF too.
**      16-oct-2006 (stial01)
**          Free locator context.
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**	12-Apr-2008 (kschendel)
**	    Move adf.h include sooner.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

static DB_STATUS release_cb(
    DMC_CB	*dmc,
    DML_SCB	*scb);


/*
** {
** Name: dmc_end_session - Ends a session.
**
**  INTERNAL DMF call format:    status = dmc_end_session(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_END_SESSION,&dmc_cb);
**
** Description:
**    The dmc_end_session function handles the ending of a session.  This
**    operation causes the server to release all internal control information
**    used to establish an execution environment for the session identified
**    by dmc_session_id.  If a transaction is progress or a database is still
**    open for this session the session cannot be deleted and an error will
**    be returned to the caller.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SERVER_OP.
**          .dmc_session_id		    Must be a valid session id
**                                          obtained from SCF.
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM003F_DB_OPEN
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0060_TRAN_IN_PROGRESS
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0106_ERROR_ENDING_SESSION
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	 1-Feb-1989 (ac)
**	    Added 2PC support.
**	 3-apr-1989 (rogerk)
**	    Make sure return with correct error status on error.
**	    Pass correct parm length to ule_format.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Call to gateway to disconnect.
**	20-Oct-1992 (daveb)
**	    remove call to dmf_gws_term; it's done by SCF now.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Changed handling of dmc_end while there
**	    is an open transaction to call dmxe_pass_abort.  The old code
**	    did not account for all the cache protocols needed to request an
**	    abort action by the RCP.
**	11-jan-1993 (mikem)
**	    Fix "used before set" problem in release_cb().  Initialized odcb
**	    before using it in error path through the code.  
**       5-Nov-1993 (fred)
**          Deallocate any leftover dmt_cb's which might be hanging
**          around after failure to delete large object temporaries...
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B58032, B58033.
**	    Log scf_error.err_code if scf_call fails.
**	    Log LKrelease return code if LKrelease fails.
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	05-May-2004 (jenjo02)
**	    Factotum SCBs aren't allocated a scb_lock_list
**	    until needed, so don't LKrelease if we don't
**	    have one.
**	10-May-2004 (schka24)
**	    Non-factotum thread needs to release lo-cleanup sem.
**	    Deallocate blob stuff in release-cb.
**	11-Jun-2004 (schka24)
**	    Release odcb cq sem.
**	21-Jun-2004 (schka24)
**	    Get rid of separate blob-cleanup list entirely.
**	22-Jul-2004 (schka24)
**	    Recalc server trace bits so session tracing wrappers aren't
**	    left active indefinitely.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Destroy BQCB mutex.
*/

DB_STATUS
dmc_end_session(
DMC_CB    *dmc_cb)
{
    DM_SVCB             *svcb;
    DMC_CB		*dmc = dmc_cb;
    DML_SCB		*scb;
    DB_STATUS		status;
    STATUS		cl_status;
    i4		error, local_error;
    CL_ERR_DESC		clerror;
    DML_SLCB            *slcb;
    DMT_CB              *dmtcb;
    DB_ERROR		local_dberr;


    svcb = dmf_svcb;
    svcb->svcb_stat.ses_end++;

    CLRDBERR(&dmc->error);

    for (status = E_DB_ERROR;;)
    {
	if (dmc->dmc_op_type != DMC_SESSION_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}

	if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 ||
	      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
	{
	    SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
	    break;
	}

	/* Toss any blob holding temps left in this session */

	if (scb->scb_oq_next != (DML_ODCB *) &scb->scb_oq_next)
	    (void) dmpe_free_temps(scb->scb_oq_next, &local_dberr);

	if (dmc_cb->dmc_flags_mask & DMC_FORCE_END_SESS)
	{
	    /* Release all the DMF data structures held by the session. */

	    status = release_cb(dmc, scb);
	    if (status)
		break;
	}

        if (scb->scb_x_ref_count != 0)
	{
	    SETDBERR(&dmc->error, 0, E_DM0060_TRAN_IN_PROGRESS);
	    status = E_DB_ERROR;
	    break;
	}

	if (scb->scb_o_ref_count != 0)
	{
	    SETDBERR(&dmc->error, 0, E_DM003F_DB_OPEN);
	    status = E_DB_ERROR;
	    break;
	}

	/* Remove SLCBs. */

	while (scb->scb_kq_next != (DML_SLCB *) &scb->scb_kq_next)
	{
	    /*  Get next SLCB. */

	    slcb = (DML_SLCB *) scb->scb_kq_next;

	    /*	Remove from queue. */

	    slcb->slcb_q_next->slcb_q_prev = slcb->slcb_q_prev;
	    slcb->slcb_q_prev->slcb_q_next = slcb->slcb_q_next;

	    /*	Deallocate. */

	    dm0m_deallocate((DM_OBJECT **)&slcb);

	}

	/* 
	** Don't release the session lock list if the session is forced
	** to terminate or if the session has a pending WILLING COMMIT 
	** transaction. 
	** The session lock list will be released by the recovery process,
	** if the session is going to be aborted by the recovery process or
	** in the session re-association time.
	**
	** If Factotum session, there may not be a session lock list.
	*/

	if ( (dmc_cb->dmc_flags_mask & DMC_FORCE_END_SESS) == 0 &&
	     (scb->scb_state & SCB_WILLING_COMMIT) == 0 &&
	     scb->scb_lock_list )
	{
	    /*  Release the session lock list. */
	    i4	lock_list = scb->scb_lock_list;

	    /* attempt this but once */
	    scb->scb_lock_list = 0;

	    cl_status = LKrelease(LK_ALL, lock_list, (LK_LKID *)0,
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &clerror);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &clerror, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 0);
		uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &clerror, ULE_LOG, NULL,
	            (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		    0, lock_list);
		SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
		status = E_DB_ERROR;
		break;
	    }
	}
	/* Clean up mutexes. */
	dm0s_mrelease(&scb->scb_bqcb_mutex);

	/* Free session's memory pool(s), if any */
	(VOID) dm0m_destroy(scb, &local_dberr);

	/*  Remove SCB from SVCB queue of active sessions. */
	
	dm0s_mlock(&svcb->svcb_sq_mutex);
	scb->scb_q_next->scb_q_prev = scb->scb_q_prev;
        scb->scb_q_prev->scb_q_next = scb->scb_q_next;
        svcb->svcb_cnt.cur_session--;
	/* Recalc serverwide session-trace bits if we had any turned
	** on.  Don't bother for factota, session bit is probably still
	** in the parent session (and this is an optimization anyway).
	** 96 session tracepoints == 3 i4's.
	** Do this while the mutex is held.
	*/
	if ((scb->scb_s_type & SCB_S_FACTOTUM) == 0
	  && (scb->scb_trace[0] | scb->scb_trace[1] | scb->scb_trace[2]) != 0)
	    dmf_trace_recalc();
	dm0s_munlock(&svcb->svcb_sq_mutex);

	/* The DML_SCB is permanently housed in SCF's SCB */
	status = E_DB_OK;

	break;
    }

    return(status);
}

/*
** History:
**	26-jul-1993 (bryanp)
**	    Fixed error handling in release_cb following dmxe_pass_abort call.
**	01-Oct-2004 (jenjo02)
**	    With Factotum threads, scb_oq_next will be empty
**	    but xcb_odcb_ptr will be correct when calling
**	    dmxe_pass_abort().
**	6-Jul-2006 (kschendel)
**	    Pass the db id to RDF in the right place.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update destroy-temp call parameters.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Deallocate lctx, jctx if allocated.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Force a BLOB query-end during cleanup.
*/
static DB_STATUS
release_cb(
DMC_CB		*dmc,
DML_SCB		*scb)
{
    DML_XCB		*xcb;
    DMP_DCB		*dcb;
    DMP_RCB		*rcb;
    i4		error,local_error;
    CL_ERR_DESC		clerror;
    DB_STATUS		status;
    DML_ODCB		*odcb;
    DML_SPCB		*spcb;
    DML_XCCB            *xccb;
    DB_TAB_ID		tbl_id;

    CLRDBERR(&dmc->error);

    while (scb->scb_x_next != (DML_XCB *) &scb->scb_x_next)
    {
	/*  Get next XCB. */

	xcb = (DML_XCB *) scb->scb_x_next;
	odcb = (DML_ODCB *) xcb->xcb_odcb_ptr;

	/* Remove blob Locator context */
	if (scb->scb_lloc_cxt)
	    dm0m_deallocate((DM_OBJECT **)&scb->scb_lloc_cxt);

	/* Close off any in-flight DMPE stuff */
	status = dmpe_query_end(TRUE, TRUE, &dmc->error);
	if ( status != E_DB_OK )
	{
	    SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
	    return (E_DB_FATAL);
	}

	/* Remove blob PCB's */
	while (xcb->xcb_pcb_list != NULL)
	    dmpe_deallocate(xcb->xcb_pcb_list);

	/*  Close all open tables and destroy all open temporary tables. */

	while (xcb->xcb_rq_next != (DMP_RCB*) &xcb->xcb_rq_next)
	{
	    /*	Get next RCB. */

	    rcb = (DMP_RCB *)((char *)xcb->xcb_rq_next -
		(char *)&(((DMP_RCB*)0)->rcb_xq_next));

	    /*	Remove from the XCB. */

	    rcb->rcb_xq_next->rcb_q_prev = rcb->rcb_xq_prev;
	    rcb->rcb_xq_prev->rcb_q_next = rcb->rcb_xq_next;

	    /*	Remember DCB and table id of a temporary. */

	    dcb = 0;
	    if (rcb->rcb_tcb_ptr->tcb_temporary == TCB_TEMPORARY)
	    {
		dcb = rcb->rcb_tcb_ptr->tcb_dcb_ptr;
		tbl_id = rcb->rcb_tcb_ptr->tcb_rel.reltid;
	    }

	    /*	Deallocate the RCB. */

	    status = dm2t_close(rcb, (i4)0, &dmc->error);
	    if (status != E_DB_OK)
	    {
		if ((dmc->error.err_code != E_DM004B_LOCK_QUOTA_EXCEEDED) &&
		    (dmc->error.err_code != E_DM0042_DEADLOCK) &&
		    (dmc->error.err_code != E_DM004D_LOCK_TIMER_EXPIRED) &&
		    (dmc->error.err_code != E_DM004A_INTERNAL_ERROR))
		{
		    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
		}
		return (E_DB_FATAL);
	    }

	    /*	Now destroy the TCB if it's a temporary. */
	    /*  I can't be bothered to change this code to run off of the
	    **  xccb list, so can't use dmt-destroy-temp. (schka24)
	    **  I may regret this if it turns out that factotum can get
	    **  here -- could end up deleting session temp...
	    */

	    if (dcb)
	    {
		RDF_CB rdfcb;

		/* RDF doesn't use its end-session call, and anyway
		** we know the table ID and it doesn't.   Toss the
		** table out of RDF so that it's not clogging up the
		** RDF-works.  This also ensures that nobody will accidently
		** find the table by ID in RDF if the ID is reused.
		** (Unlikely, but possible.)
		** Zero in rdr_fcb says don't send invalidate dbevents
		** to other servers.  Zero in the rdfcb rdf_info_blk says
		** we don't have anything fixed.
		*/

		MEfill(sizeof(RDF_CB), 0, &rdfcb);
		STRUCT_ASSIGN_MACRO(tbl_id, rdfcb.rdf_rb.rdr_tabid);
		rdfcb.rdf_rb.rdr_session_id = scb->scb_sid;
		rdfcb.rdf_rb.rdr_unique_dbid = dcb->dcb_id;
		rdfcb.rdf_rb.rdr_db_id = (PTR) odcb;
		/* Ignore error on this call */
		(void) rdf_call(RDF_INVALIDATE, &rdfcb);

		/*
		**  Another RCB, yet to be deleted, could still be referencing
		**  the TCB.  Handle the associated error from destroy as normal
		**  in this case.
		*/

		status = dm2t_destroy_temp_tcb(scb->scb_lock_list, 
			    dcb, &tbl_id, &dmc->error);

		if (status != E_DB_OK &&
		    dmc->error.err_code != E_DM005D_TABLE_ACCESS_CONFLICT)
		{
		    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
		    return (E_DB_FATAL);
		}
	    }
	}
	/* One more time after table closes, guarantees BQCB's deleted */
	status = dmpe_query_end(TRUE, FALSE, &dmc->error);
	if ( status != E_DB_OK )
	{
	    SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
	    return (E_DB_FATAL);
	}

	/*  Remove SPCBs. */

	while (xcb->xcb_sq_next != (DML_SPCB*) &xcb->xcb_sq_next)
	{
	    /*	Get next SPCB. */

	    spcb = xcb->xcb_sq_next;

	    /*	Remove SPCB from XCB queue. */

	    spcb->spcb_q_next->spcb_q_prev = spcb->spcb_q_prev;
	    spcb->spcb_q_prev->spcb_q_next = spcb->spcb_q_next;

	    /*	Deallocate the SPCB. */

	    dm0m_deallocate((DM_OBJECT **)&spcb);
	}

	/*  Remove XCCBs. */

	while (xcb->xcb_cq_next != (DML_XCCB*)&xcb->xcb_cq_next)
	{
	    /* Get pend XCCB */

	    xccb = xcb->xcb_cq_next;

	    /*	Remove from queue. */

	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;

	    /*	Deallocate. */

	    dm0m_deallocate((DM_OBJECT **)&xccb);
	}

	/* Deallocate lctx, jctx if allocated */
	if ( xcb->xcb_lctx_ptr )
	    dm0m_deallocate(&xcb->xcb_lctx_ptr);
	if ( xcb->xcb_jctx_ptr )
	{
	    /* Close any open jnl file, deallocate jctx */
	    status = dm0j_close(&xcb->xcb_jctx_ptr, &dmc->error);
	    if ( status )
	    {
		SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
		return (E_DB_FATAL);
	    }
	}

	/*
	** Since we are deallocating a session which has an open transaction,
	** we signal the RCP to abort the transaction for us before deallocating
	** its context.
	**
	** Call dmxe to flush any required pages from the Buffer Manager and
	** to make the abort request to the RCP.
	**
	** The LG context will be freed up by the RCP when the abort is
	** complete.
	**
	** Reference the DB opened on the XCB, not SCB; if this
	** is a Factotum thread, scb_oq_next will be empty,
	** but xcb_odcb_ptr will be valid.
	** Likewise, use xcb_lk_id rather than scb_lock_list.
	*/

	/* XXXX log error message about why pass abort is called */
	status = dmxe_pass_abort(xcb->xcb_log_id, xcb->xcb_lk_id, 
		    &xcb->xcb_tran_id, odcb->odcb_dcb_ptr->dcb_id, &dmc->error);
	if (status)
	{
	    /* XXXX May want to dmd_check here */
	    uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
	    return (E_DB_FATAL);
	}

	/*  Remove XCB from SCB queue. */

	xcb->xcb_q_next->xcb_q_prev = xcb->xcb_q_prev;
	xcb->xcb_q_prev->xcb_q_next = xcb->xcb_q_next;
	scb->scb_x_ref_count--;

	dm0m_deallocate((DM_OBJECT **)&xcb);
    }

    /* Close all the opened database of the session. */

    while (scb->scb_oq_next != (DML_ODCB *) &scb->scb_oq_next)
    {
	/*  Get next ODCB. */

	odcb = (DML_ODCB *) scb->scb_oq_next;

	status = dm2d_close_db(odcb->odcb_dcb_ptr, scb->scb_lock_list, 
				DM2D_NLG | DM2D_NLK_SESS, &dmc->error);
	if (status != E_DB_OK)
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
		    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0106_ERROR_ENDING_SESSION);
	    }
	    return (E_DB_FATAL);
	}

	scb->scb_o_ref_count--;
	odcb->odcb_q_next->odcb_q_prev = odcb->odcb_q_prev;
	odcb->odcb_q_prev->odcb_q_next = odcb->odcb_q_next;
	dm0s_mrelease(&odcb->odcb_cq_mutex);
	dm0m_deallocate((DM_OBJECT **)&odcb);
	
    }
    return(E_DB_OK);
}

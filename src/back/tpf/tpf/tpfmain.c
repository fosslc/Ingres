/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>

/**
**
** Name: TPFMAIN.C - TPF external primitives
** 
** Description: 
**	Transaction process facility primitives.  All transaction processing    
**	requests are done by these primitives via tpf_call.
**
**  	tpf_m1_process	    - transaction event processing
**	tpf_m2_trace	    - (un)set trace/debug flags
**	tpf_m3_abort_to_svpt- abort to a save point
**	tpf_m4_savepoint    - transaction save point
**  	tpf_m5_startup_svr  - facility initialization
**	tpf_m6_shutdown_svr - facility shutdown
**  	tpf_m7_init_sess    - session initialization
**	tpf_m8_term_sess    - session termination
**	tpf_m9_transition_ok  (internal) 
**			    - protocol check
**	tpf_call	    - facility entry points
**
**  History:    $Log-for RCS$
**      mar-9-88 (alexh)
**          created
**      mar-29-88 (carl)
**	    fixed tpf_call to NOT expect session information on system shut
**	    down because there is none 
**      may-18-89 (carl)
**	    added tpf_m11_true_ddl_conc and tpf_m12_false_ddl_conc to TPF_CALL 
**	    for DDL_CONCURRENCY mamangement and coordination
**
**	    added tpf_m11_true_ddl_conc and tpf_m12_false_ddl_conc
**      
**	    changed tpf_m7_init_sess to set session->tss_ddl_cc to TRUE
**
**	    changed tpf_m4_savepoint to skip sending the savepoint for the 
**	    privileged CDB association if DDL concurrency is ON
**      jun-01-89 (carl)
**	    changed tpf_m4_savepoint and tpf_ab_savepoint to maintain 
**	    savepoints as a doubly linked list
**      jun-27-89 (carl)
**	    fixed return inconsistencies discovered by lint in 
**	    tpf_m7_init_sess, tpf_m9_transition_ok, and tpf_m10_process;
**	    fixed casting problem discovered by lint in tpf_m7_init_sess
**      jul-11-89 (carl)
**	    changed tpf_m5_startup_svr and tpf_m6_shutdown_svr to allocate/
**	    deallocate an ulm pool for session streams and their CBs
**
**	    changed tpf_m8_term_sess and tpf_m7_init_sess to (de)allocate 
**	    session CB from dedicated stream 
**      jul-25-89 (carl)
**	    1)  set ulm_memleft to the maximum poolsize in tpf_m5_startup_svr
**	    2)  increased the stream size for each session to allow a 
**		maximum number of savepoints of 75
**	oct-15-89 (carl)
**	    converted to use TPFDDB.H adapted from the old TPFPRVT.H
**	nov-05-89 (carl)
**	    folded tpf_auto_on/tpf_no_auto and tpf_m11_true_ddl_conc/
**	    tpf_m12_false_ddl_conc back into tpf_call
**      08-jun-90 (carl)
**	    modified tpf_call to properly set the 2PC flag
**      01-sep-90 (carl)
**	    1)  removed tpf_m1_dump_event and moved tpf_m2_dump_state to 
**		TPFUTIL.C,
**	    2)	modified this module to process tracing
**      09-oct-90 (carl)
**          process trace points
**	13-aug-91 (fpang)
**	    Use new max trace point.
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	feb-02-1993 (fpang)
**	    In tpf_m3_abort_to_svpt(), if DX mode after rollback is 0 (no action 
**	    yet), artificially make it a read, to not lose subsequent commits 
**	    aborts.
**	    Fixes B49222.
**	feb-05-1993 (fpang)
**	    Added support to detect commit/aborts when executing local database
**	    procedures, and when executing direct execute immediate.
**	    Basically, when QEF registers a statement, return a flag indicating
**	    whether the DX is in 2PC or not.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	 6-sep-93 (swm/smc)
**	    Specify %p print option when printing the sscb_p pointer and
**	    eliminate truncating cast to nat.
**	    Changed i4  parameter to tpf_m6_shutdown_svr() to TPR_CB pointer.
**      11-oct-93 (tam)
**	    In tpf_m1_process(), in handling TPF_ABORT, add tpd_m3_term_dx_cb() 
**	    call to reset dtx state.  This was already done for TPF_COMMIT.  This 
**	    missing call causes tpd to reuse the state of an already aborted dtx.  
**	10-oct-93 (swm)
**	    Bug #56448
**	    Altered tpf_u0_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to tpf_u0_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    tpf_u0_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	23-sep-1996 (canor01)
**	    Move global data definitions into tpfdata.c.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	04-Sep-1997 (bonro01)
**	    Include break statement in comments to eliminated compiler error
**	    for dg8_us5.
**	23-Sep-1997 (jenjo02)
**	    Initialize tsv_11_srch_sema4, which wasn't being.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN VOID		 tpd_m2_init_dx_cb();

FUNC_EXTERN TPD_SP_CB		*tpd_s1_srch_sp();

/* global variables */

GLOBALREF   struct _TPD_SV_CB	*Tpf_svcb_p;
					/* TPF facility control block */
GLOBALREF   char                *IITP_00_tpf_p;

GLOBALREF   char                *IITP_05_reader_p;
 
GLOBALREF   char                *IITP_06_updater_p;
 
GLOBALREF   char                *IITP_07_tracing_p;

GLOBALREF   char                *IITP_10_savepoint_p;
 
GLOBALREF   char                *IITP_13_allowed_in_dx_p;

bool tpf_m9_transition_ok();

/*{
** Name: tpf_m1_process - transaction event processing
** 
** Description:  
**	TPF request codes TPF_READ_DML, TPF_COMMIT, TPF_WRITE_DML, TPF_ABORT, 
**	are processed as a a transactional  
**	event.  The validity of the current event is checked by calling
**	tpf_m9_transition_ok.  This routine executes the commit and terminating
**	sub-protocols and of the coordinator 2PC.  See TITAN Transaction 
**	Protocol Specification document.
**
**	The tpr_lang_type indicator  of  the first  DML  event  (
**	TPF_WRITE_DML  or   TPR_READ_DML decides the   type   of
**	transaction protcol used (SQL or QUEL.)
**
** Inputs:
**		i_call			valid transaction event:
**						TPF_READ_DML, 
**						TPF_COMMIT,
**						TPF_WRITE_DML,
**						TPF_ABORT,
**		v_tpr_p->			TPF request control block
**			tpr_session		session control block pointer
**			tpr_site		transaction slave id
**			tpr_lang_type		language indicator.
** Outputs:
**		v_tpr_p->
**			tpr_session		session context is updated
**	Returns:
**		E_TP0000_OK
**		E_TP0004_MULTI_SITE_WRITE
**		E_TP0005_UNKNOWN_STATE	  
**		E_TP0006_INVALID_EVENT	  
**		E_TP0007_INVALID_TRANSITIO
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      mar-3-88 (alexh)
**          created
**      jun-27-89 (carl)
**	    fixed return inconsistencies discovered by lint 
**      09-oct-90 (carl)
**          process trace points
**	feb-05-1993 (fpang)
**	    Added support to detect commit/aborts when executing local database
**	    procedures, and when executing direct execute immediate.
**	    Basically, when QEF registers a statement, return a flag indicating
**	    whether the DX is in 2PC or not.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-oct-93 (tam)
**	    In handling TPF_ABORT, add tpd_m3_term_dx_cb() call to reset dtx
**	    state.  This was already done for TPF_COMMIT.  This missing call
**	    causes tpd to reuse the state of an already aborted dtx.  
*/


DB_STATUS
tpf_m1_process(
	i4	i_call,
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *)v_tpr_p->tpr_session;
    DD_LDB_DESC	*ldb_p = v_tpr_p->tpr_site;
    bool	ok;
    bool        trace_ldbs_105 = FALSE,
                trace_err_104 = FALSE;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;
	

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_LDBS_105,
            & i4_1, & i4_2))
    {
        trace_ldbs_105 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    /* check for valid state transition */

    ok = tpf_m9_transition_ok(sscb_p->tss_dx_cb.tdx_state, i_call, v_tpr_p);
    if (! ok)
    {
    	return( tpd_u1_error(E_TP0007_INVALID_TRANSITION, v_tpr_p) );
    }

    switch(i_call)
    {
    case TPF_READ_DML:
	if (trace_ldbs_105)
	{
	    STprintf(cbuf, "%s %p: %s %s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_07_tracing_p,
		IITP_05_reader_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

	    tpd_p9_prt_ldbdesc(v_tpr_p, ldb_p, TPD_0TO_FE);
	}
	    
	status = tpd_m6_register_ldb(FALSE, v_tpr_p);
	if (status)
	   {
	    if (! trace_ldbs_105 && trace_err_104)
	    {
		STprintf(cbuf, "%s %p: %s NOT %s:\n",
		    IITP_00_tpf_p,
		    (PTR) sscb_p,
		    IITP_05_reader_p,
		    IITP_13_allowed_in_dx_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		tpd_p9_prt_ldbdesc(v_tpr_p, ldb_p, TPD_0TO_FE);
	    }
	}
	break;

    case TPF_COMMIT:
	status = tpd_s2_clear_all_sps(v_tpr_p);    /* clear all savepoints */

	/* ignore error */

	status = tp2_m0_commit_dx(v_tpr_p);
	(VOID) tpd_m3_term_dx_cb(v_tpr_p, (TPD_SS_CB *) v_tpr_p->tpr_session);
	break;
    case TPF_WRITE_DML:
	if (trace_ldbs_105)
	{
	    STprintf(cbuf, "%s %p: %s %s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_07_tracing_p,
		IITP_06_updater_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

	    tpd_p9_prt_ldbdesc(v_tpr_p, ldb_p, TPD_0TO_FE);
	}
	    
	status = tpd_m6_register_ldb(TRUE, v_tpr_p);
	if (status)
	   {
	    if (! trace_ldbs_105 && trace_err_104)
	    {
		STprintf(cbuf, "%s %p: %s NOT %s:\n",
		    IITP_00_tpf_p,
		    (PTR) sscb_p,
		    IITP_06_updater_p,
		    IITP_13_allowed_in_dx_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		tpd_p9_prt_ldbdesc(v_tpr_p, ldb_p, TPD_0TO_FE);
	    }
	}

	/* Tell caller whether this DX is in 2PC or not */
	if (sscb_p->tss_dx_cb.tdx_mode == DX_2MODE_2PC)
	    v_tpr_p->tpr_22_flags |= TPR_01FLAGS_2PC;
	else
	    v_tpr_p->tpr_22_flags |= ~(TPR_01FLAGS_2PC);

	break;
    case TPF_ABORT:

	/* 1 phase abort in these cases */

	status = tpd_s2_clear_all_sps(v_tpr_p);    /* clear all savepoints */

	/* ignore error of clear savepoints */

	status = tp2_m1_end_1pc(v_tpr_p, TPD_0DO_ABORT);
	(VOID) tpd_m3_term_dx_cb(v_tpr_p, (TPD_SS_CB *) v_tpr_p->tpr_session);
	break;
    case TPF_BEGIN_DX:
	/* just mark the state */
	sscb_p->tss_dx_cb.tdx_state = DX_1STATE_BEGIN;
	break;
    default:
	status = tpd_u1_error(E_TP0006_INVALID_EVENT, v_tpr_p);
    }
    return(status);
}


/*{
** Name: tpf_m2_trace - (un)set trace flags
**
** External TPF call:   status = tpf_call(TPF_TRACE, & tpf_cb);
**
** Description:
**      Set and unset 2PC trace points 60 to 127. 
**
**	TPF trace points can be set from TM (or ing_set logical) by the command:
**
**	    SET TRACE POINT QEnnn [m1 m2]
**
**	where <nnn> is an integer with the following semantics:
**
**	    60 - 127
**
**	qec_trace() is called by SCF when the SET [NO]TRACE POINT command is 
**	recognized and qec_trace calls this routine via tpf_call().
**
** Inputs:
**      debug_cb
**	    .db_trswitch                operation to perform
**              DB_TR_NOCHANGE          do nothing
**              DB_TR_ON                set trace point
**              DB_TR_OFF               clear trace point
**          .db_trace_point             trace point number
**          .db_vals[2]                 optional values
**          .db_value_count             number of optional values
**
** Outputs:
**	none
**
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	24-apr-90 (carl)
**	    adapted from qec_trace
*/


DB_STATUS
tpf_m2_trace(
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    i4     tr_point;
    i4     val1;
    i4     val2;
    bool        sess_point = FALSE;
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB   *dxcb_p = (TPD_DX_CB *) & sscb_p->tss_dx_cb;
    DB_DEBUG_CB	*dbgcb_p = v_tpr_p->tpr_18_dbgcb_p;


    tr_point = dbgcb_p->db_trace_point;

    /* if session trace point */

    if (tr_point < TPF_MIN_TRACE_60
	||
	tr_point > TPF_MAX_TRACE_119)
    {
/*
	v_tpr_p->tpr_error.err_code = E_TP0001_INVALID_REQUEST;
        return (E_DB_ERROR);
*/
	return( tpd_u1_error(E_TP0001_INVALID_REQUEST, v_tpr_p) );
    }

    /* There can be UP TO two values, but maybe they weren't given */

    if (dbgcb_p->db_value_count > 0)
        val1 = dbgcb_p->db_vals[0];
    else
        val1 = 0L;

    sscb_p->tss_5_dbg_timer = val1;

    if (dbgcb_p->db_value_count > 1)
        val2 = dbgcb_p->db_vals[1];
    else
        val2 = 0L;

    /*
    ** Three possible actions: turn on flag, turn it off, or do nothing.
    */

    switch (dbgcb_p->db_trswitch)
    {
    case DB_TR_ON:
	switch (tr_point)
	{
	case TSS_60_CRASH_BEF_SECURE:
	    TRdisplay("%s %p: QE60 will crash server before SECURE state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_60_CRASH_BEF_SECURE;
	    break;	    

	case TSS_61_CRASH_IN_SECURE:
	    TRdisplay("%s %p: QE61 will crash server after SECURE state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_61_CRASH_IN_SECURE;
	    break;	    

	case TSS_62_CRASH_AFT_SECURE:
	    TRdisplay("%s %p: QE62 will crash server after SECURE state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_62_CRASH_AFT_SECURE;
	    break;	    

	case TSS_63_CRASH_IN_POLLING:
	    TRdisplay("%s %p: QE63 will crash server while polling the\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    last site of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_63_CRASH_IN_POLLING;
	    break;	    

	case TSS_65_SUSPEND_BEF_SECURE:
	    TRdisplay("%s %p: QE65 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    before SECURE state of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_65_SUSPEND_BEF_SECURE;
	    break;	    

	case TSS_66_SUSPEND_IN_SECURE:
	    TRdisplay("%s %p: QE66 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after SECURE state of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_66_SUSPEND_IN_SECURE;
	    break;	    

	case TSS_67_SUSPEND_AFT_SECURE:
	    TRdisplay("%s %p: QE67 will suspend session for %d seconds after\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    SECURE state of DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_67_SUSPEND_AFT_SECURE;
	    break;	    

	case TSS_68_SUSPEND_IN_POLLING:
	    TRdisplay("%s %p: QE68 will suspend session for %d seconds while\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    polling the last sites of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_68_SUSPEND_IN_POLLING;
	    break;	    

	case TSS_70_CRASH_BEF_COMMIT:
	    TRdisplay("%s %p: QE70 will crash server before COMMIT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_70_CRASH_BEF_COMMIT;
	    break;	    

	case TSS_71_CRASH_IN_COMMIT:
	    TRdisplay("%s %p: QE71 will crash server after COMMIT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_71_CRASH_IN_COMMIT;
	    break;	    

	case TSS_72_CRASH_AFT_COMMIT:
	    TRdisplay("%s %p: QE72 will crash server after COMMIT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_72_CRASH_AFT_COMMIT;
	    break;	    

	case TSS_73_CRASH_LX_COMMIT:
	    TRdisplay("%s %p: QE73 will crash server after committing\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    some sites of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_73_CRASH_LX_COMMIT;
	    break;	    

	case TSS_75_SUSPEND_BEF_COMMIT:
	    TRdisplay("%s %p: QE75 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    before COMMIT state of %d %d is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_75_SUSPEND_BEF_COMMIT;
	    break;	    

	case TSS_76_SUSPEND_IN_COMMIT:
	    TRdisplay("%s %p: QE76 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after COMMIT state of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_76_SUSPEND_IN_COMMIT;
	    break;	    

	case TSS_77_SUSPEND_AFT_COMMIT:
	    TRdisplay("%s %p: QE77 will suspend session for %d seconds after\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    COMMIT state of DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_77_SUSPEND_AFT_COMMIT;
	    break;	    

	case TSS_78_SUSPEND_LX_COMMIT:
	    TRdisplay("%s %p: QE78 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after committing some sites of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_78_SUSPEND_LX_COMMIT;
	    break;	    

	case TSS_80_CRASH_BEF_ABORT:
	    TRdisplay("%s %p: QE80 will crash server before ABORT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_80_CRASH_BEF_ABORT;
	    break;	    

	case TSS_81_CRASH_IN_ABORT:
	    TRdisplay("%s %p: QE81 will crash server after ABORT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_81_CRASH_IN_ABORT;
	    break;	    

	case TSS_82_CRASH_AFT_ABORT:
	    TRdisplay("%s %p: QE82 will crash server after ABORT state of\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_82_CRASH_AFT_ABORT;
	    break;	    

	case TSS_83_CRASH_LX_ABORT:
	    TRdisplay("%s %p: QE83 will crash server after aborting some\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    some sites of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_83_CRASH_LX_ABORT;
	    break;	    

	case TSS_85_SUSPEND_BEF_ABORT:
	    TRdisplay("%s %p: QE85 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    before ABORT state of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_85_SUSPEND_BEF_ABORT;
	    break;	    

	case TSS_86_SUSPEND_IN_ABORT:
	    TRdisplay("%s %p: QE86 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after ABORT state of DX %x %x is logged.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_86_SUSPEND_IN_ABORT;
	    break;	    

	case TSS_87_SUSPEND_AFT_ABORT:
	    TRdisplay("%s %p: QE87 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after ABORT state of DX %x %x is committed.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_87_SUSPEND_AFT_ABORT;
	    break;	    

	case TSS_88_SUSPEND_LX_ABORT:
	    TRdisplay("%s %p: QE88 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    after aborting some sites of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_88_SUSPEND_LX_ABORT;
	    break;	    

	case TSS_90_CRASH_STATE_LOOP:
	    TRdisplay("%s %p: QE90 will crash the server before executing\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    the loop to log and commit the next state\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    of DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_90_CRASH_STATE_LOOP;
	    break;	    

	case TSS_91_CRASH_DX_DEL_LOOP:
	    TRdisplay("%s %p: QE91 will crash the server before executing\n",
		IITP_00_tpf_p,
		sscb_p);
	    TRdisplay("%s %p:    the loop to delete DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_91_CRASH_DX_DEL_LOOP;
	    break;	    

	case TSS_95_SUSPEND_STATE_LOOP:
	    TRdisplay("%s %p: QE90 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    for DX %x %x before executing loop\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    TRdisplay("%s %p:    to log and commit state.\n",
		IITP_00_tpf_p,
		sscb_p);
	    sscb_p->tss_4_dbg_2pc = TSS_95_SUSPEND_STATE_LOOP;
	    break;	    

	case TSS_96_SUSPEND_DX_DEL_LOOP:
	    TRdisplay("%s %p: QE91 will suspend session for %d seconds\n",
		IITP_00_tpf_p,
		sscb_p,
		val1);
	    TRdisplay("%s %p:    before executing loop to delete DX %x %x\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = TSS_96_SUSPEND_DX_DEL_LOOP;
	    break;	    

	default:
            ult_set_macro(& sscb_p->tss_3_trace_vector, tr_point, val1, val2);
	}
        break;

    case DB_TR_OFF:
	switch (tr_point)
	{
	case TSS_60_CRASH_BEF_SECURE:
	case TSS_61_CRASH_IN_SECURE:
	case TSS_62_CRASH_AFT_SECURE:
	case TSS_63_CRASH_IN_POLLING:

	case TSS_65_SUSPEND_BEF_SECURE:
	case TSS_66_SUSPEND_IN_SECURE:
	case TSS_67_SUSPEND_AFT_SECURE:
	case TSS_68_SUSPEND_IN_POLLING:

	case TSS_70_CRASH_BEF_COMMIT:
	case TSS_71_CRASH_IN_COMMIT:
	case TSS_72_CRASH_AFT_COMMIT:
	case TSS_73_CRASH_LX_COMMIT:

	case TSS_75_SUSPEND_BEF_COMMIT:
	case TSS_76_SUSPEND_IN_COMMIT:
	case TSS_77_SUSPEND_AFT_COMMIT:
	case TSS_78_SUSPEND_LX_COMMIT:

	case TSS_80_CRASH_BEF_ABORT:
	case TSS_81_CRASH_IN_ABORT:
	case TSS_82_CRASH_AFT_ABORT:
	case TSS_83_CRASH_LX_ABORT:

	case TSS_85_SUSPEND_BEF_ABORT:
	case TSS_86_SUSPEND_IN_ABORT:
	case TSS_87_SUSPEND_AFT_ABORT:
	case TSS_88_SUSPEND_LX_ABORT:

	case TSS_90_CRASH_STATE_LOOP:
	case TSS_91_CRASH_DX_DEL_LOOP:

	case TSS_95_SUSPEND_STATE_LOOP:
	case TSS_96_SUSPEND_DX_DEL_LOOP:
	    TRdisplay("%s %p: Reset trace point QE%d for DX %x %x.\n",
		IITP_00_tpf_p,
		sscb_p,
		tr_point,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	    sscb_p->tss_4_dbg_2pc = 0;
	    sscb_p->tss_5_dbg_timer = 0;
	    break;	    
	default:
	    ult_clear_macro(& sscb_p->tss_3_trace_vector, tr_point);
	}
        break;

    case DB_TR_NOCHANGE:
        /* nothing to do */
        break;

    default:
	ult_clear_macro(& sscb_p->tss_3_trace_vector, tr_point);
    }
    return (E_DB_OK);
}


/*{ 
** Name: tpf_m3_abort_to_svpt	- abort to a save point
** 
** Description: 
**	Abort the distributed transaction to a save point by broadcasting
**	the abort to all LDB transactions. LDB 'save point does not exist'
**	can be safely ignored when at least one LDB accepts the abort.
**
** Inputs:
**	v_tpr_p->			- TPR_CB ptr
**	    tpr_save_name		- save point name
**	    tpr_lang_type		- QUEL or SQL
**
** Outputs:
**	None
**
**	Returns:
**		E_TP0000_OK
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      Nov-9-88 (alexh)
**          created
**      jun-01-89 (carl)
**	    changed to maintain savepoints as a doubly linked list
**	feb-02-1993 (fpang)
**	    If DX mode after rollback is 0 (no action yet), artificially make 
**	    it a read, to not lose subsequent commits or aborts.
**	    Fixes B49222.
*/


DB_STATUS
tpf_m3_abort_to_svpt(
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    bool	abort_ok = TRUE;    /* assume */
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    TPD_SP_CB	*spcb_p = NULL,	/* target savepoint */
		*tmp_spcb_p = NULL;
    i4		new_spcnt;
    RQR_CB	rqr_cb;
    char	svpt_name[sizeof(DB_SP_NAME) + 1];


    /* validate the savepoint name */

    spcb_p = tpd_s1_srch_sp(sscb_p, v_tpr_p->tpr_save_name, & new_spcnt);
    if (spcb_p == NULL)
	return( tpd_u1_error(E_TP0012_SP_NOT_EXIST, v_tpr_p) );

    /* set up to call RQF */

    tpd_u0_trimtail(v_tpr_p->tpr_save_name, sizeof(DB_SP_NAME), svpt_name);

    rqr_cb.rqr_q_language = v_tpr_p->tpr_lang_type;
    rqr_cb.rqr_session = sscb_p->tss_rqf;

    /* default is ok */

    abort_ok = TRUE;

    /* abort to the target savepoint on all associations */

    status = tpd_s4_abort_to_svpt(v_tpr_p, new_spcnt, svpt_name);
    if (status)
	abort_ok = FALSE;

    /* pop to the savepoint, need to restore transaction state  */
/*
    dxcb_p->tdx_ldb_cnt = spcb_p->tsp_ldb_cnt;
*/
    dxcb_p->tdx_mode = spcb_p->tsp_2_dx_mode;

    /* If dx mode after rollback is 'no action', make it read only */
    if (dxcb_p->tdx_mode == DX_0MODE_NO_ACTION)
	dxcb_p->tdx_mode = DX_1MODE_READ_ONLY;

    if (new_spcnt < lmcb_p->tlm_2_cnt)  /* new_spcnt can never be 0 */
    {
	lmcb_p->tlm_4_last_ptr = (PTR) spcb_p;
	spcb_p->tsp_4_next_p = (TPD_SP_CB *) NULL;	    
					    /* discard obsolete SPs */
	lmcb_p->tlm_2_cnt = new_spcnt;
    }

    /* if all savepoint abort is successful then it is considered successful */

    if (abort_ok == TRUE)
	return(E_DB_OK);
    else
	return( tpd_u1_error(E_TP0012_SP_NOT_EXIST, v_tpr_p) );
}


/*{ 
** Name: tpf_m4_savepoint	- transaction save point
** 
** Description: 
**	Set transaction transaction save point. A save point request
**	will be sent to each LDB trasnaction.
**
** Inputs:
**	v_tpr_p->			- TPF session 
**	    tpr_save_name		- save point name
**	    tpr_lang_type		- QUEL or SQL
**
** Outputs:
**	None
**
**	Returns:
**		E_TP0000_OK
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None.
**
** History:
**      Nov-9-88 (alexh)
**          created
**      may-18-89 (carl)
**	    changed to skip sending the savepoint for privileged CDB 
**	    association if DDL concurrency is ON
**      jun-01-89 (carl)
**	    changed to maintain savepoints as a doubly linked list
**      09-oct-90 (carl)
**          process trace points
**	02-Jul-2001 (jenjo02)
**	    Replaced STncpy+'\0' of savepoint name with MEcopy.
**	    Placing EOS at last byte of name causes abort to
**	    savepoint to fail because MEcmp of savepoint name 
**	    with this null-terminated string mismatches.
*/


DB_STATUS
tpf_m4_savepoint(
	TPR_CB		*v_tpr_p)
{
    i4		i;
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    ULM_RCB	ulm;
    TPD_SP_CB	*new_spcb_p = NULL,
		*tmp_spcb_p = NULL;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    DD_LDB_DESC	*ldb_p = (DD_LDB_DESC *) NULL;
    RQR_CB	rqr_cb;
    bool	trace_qry_102 = FALSE,
                trace_err_104 = FALSE;
    i4     i4_1,
                i4_2;
    char	svpt_name[sizeof(DB_SP_NAME) + 1],
		qrytxt[1000];


    if (sscb_p->tss_auto_commit)
	/* no savepoints while auto commit is on */
	return( tpd_u1_error(E_TP0013_AUTO_ON_NO_SP, v_tpr_p) );

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_QUERIES_102,
            & i4_1, & i4_2))
    {
        trace_qry_102 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    /* borrow QEF's id for stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPD_SP_CB);
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
    ulm.ulm_flags = ULM_SHARED_STREAM;

    /* see if savepoint stream has been allocated */
    ulm.ulm_psize = sizeof(TPD_SP_CB);

    if (lmcb_p->tlm_1_streamid == (PTR) NULL)
    {
	/* Open and allocate TPD_SP_CB with one effort */
	ulm.ulm_flags |= ULM_OPEN_AND_PALLOC;
	status = ulm_openstream(& ulm);
	if (status)
	{
	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    TRdisplay("%s %p: Stream allocation failed\n",
		IITP_00_tpf_p,
		sscb_p);
	    return(status);
	}
    }
    else
    {
	/* allocate a TPF savepoint CB */

	status = ulm_palloc(& ulm);
	if (status)
	{
	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    TRdisplay("%s %p: Savepoint allocation failed\n",
		    IITP_00_tpf_p,
		    sscb_p);
	    return(status);
	}
    }

    /* remember the context of the savepoint */

    new_spcb_p = (TPD_SP_CB *)ulm.ulm_pptr;
    MEcopy((char *)v_tpr_p->tpr_save_name, 
	    sizeof(DB_SP_NAME),
	   (char *)&new_spcb_p->tsp_1_name);

    /* register the LDB count */
/*
    new_spcb_p->tsp_ldb_cnt = dxcb_p->tdx_ldb_cnt;
*/
    new_spcb_p->tsp_2_dx_mode = dxcb_p->tdx_mode;
    new_spcb_p->tsp_3_prev_p = NULL;
    new_spcb_p->tsp_4_next_p = NULL;

    /* prepend the new sp */

    if (lmcb_p->tlm_4_last_ptr == (PTR) NULL)
    {
	/* first savepoint */

	lmcb_p->tlm_2_cnt = 1;		
	lmcb_p->tlm_3_frst_ptr = 
	    lmcb_p->tlm_4_last_ptr = (PTR) new_spcb_p;
    }
    else
    {
	/* insert at end of list */

	tmp_spcb_p = (TPD_SP_CB *) lmcb_p->tlm_4_last_ptr;
	tmp_spcb_p->tsp_4_next_p = new_spcb_p;	    /* first set next ptr */
	new_spcb_p->tsp_3_prev_p = tmp_spcb_p;	    /* then set prev ptr */
	lmcb_p->tlm_4_last_ptr = (PTR) new_spcb_p;  /* must pt to this most 
						    ** recent SP */
	lmcb_p->tlm_2_cnt += 1;		
    }

    /* construct the save point query */

    tpd_u0_trimtail(v_tpr_p->tpr_save_name, sizeof(DB_SP_NAME), svpt_name);

    STprintf(qrytxt, "%s %s;", IITP_10_savepoint_p, svpt_name);

    /* set up to call RQF */

    rqr_cb.rqr_q_language = v_tpr_p->tpr_lang_type;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_cb.rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_cb.rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    /* broadcast message */

    status = E_RQ0000_OK;

    lxcb_p = (TPD_LX_CB *) dxcb_p->tdx_22_lx_ulmcb.tlm_3_frst_ptr;
    for (i = dxcb_p->tdx_ldb_cnt; 
	 lxcb_p != NULL && status == E_RQ0000_OK; 
	 i--)
    {
/*
	lxcb_p = & dxcb_p->tdx_head_slave[i-1];
	ldb_p = & dxcb_p->tdx_head_slave[i-1].tlx_site;
*/
	ldb_p = & lxcb_p->tlx_site;
	rqr_cb.rqr_1_site = ldb_p;

	/* skip the privileged CDB if DDL concurrency is ON, an "autocommit"
	** condition */

	if (! (sscb_p->tss_ddl_cc 
	    &&
	    rqr_cb.rqr_1_site->dd_l1_ingres_b))	
	{
	    if (trace_qry_102)
		tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, ldb_p, TPD_0TO_FE);

	    rqr_cb.rqr_timeout = 0;
	    rqr_cb.rqr_dv_cnt = 0;
	    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
	    status = tpd_u4_rqf_call(RQR_QUERY, &rqr_cb, v_tpr_p);
	    if (status == E_RQ0000_OK)
		lxcb_p->tlx_sp_cnt += 1;
	    else
	    {
		if (! trace_qry_102 && trace_err_104)
		    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, ldb_p, TPD_0TO_FE);
	    }
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
    }

    /* cleanup savepoints if not all successful */
    if (status)
    {
/*
	new_spcb_p->tsp_ldb_cnt = i;     * reflect the actual savepoint stack *
*/
	(VOID) tpf_m3_abort_to_svpt(v_tpr_p);
	return( tpd_u1_error(E_TP0010_SAVEPOINT_FAILED, v_tpr_p) );
    }
    return(E_DB_OK);
}


/*{
** Name: tpf_m5_startup_svr	- facility initialization
** 
** Description: Initialize  TP  facility. Facility control
**	block  is  allocated  from  SCF. Transaction logging  is
**	started  up. If  tpr_need_recover   is set,  TPF_RECOVER
**	should  be  called  before  the server   takes on client
**	requests. Successful start  up implies the  log file has
**	been allocated and initialized.
**
** Inputs:
**		v_tpr_p	 TPR_CB pointer
** Outputs:
**		v_tpr_p->
**			tpr_need_to_recover	pointed content is set to 
**						indicate whether recovery 
**						is necessary.
**	Returns:
**		E_TP0000_OK
**		E_TP0002_SCF_MALLOC_FAILED
**	Exceptions:
**	    None
**
** Side Effects:
**	    Set Tpf_svcb_p.
**
** History:
**      mar-10-88 (alexh)
**          created
**      jul-11-89 (carl)
**	    changed to allocate pool for session streams and their CBs
**      jul-25-89 (carl)
**	    set ulm_memleft to the maximum pool size for all sessions
*/


DB_STATUS
tpf_m5_startup_svr(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    SCF_CB	scf_cb;
    i4		ss_estimate =			/* estimated space */
		    (sizeof(TPD_SS_CB) +	/* 1 session CB */
		     sizeof(TPD_DX_CB) +	/* 1 DX CB */
		     sizeof(TPD_LX_CB) * TPD_0MAX_LX_PER_SESSION +
						/* 20 reg-LXs/session */
		     sizeof(TPD_LX_CB) * TPD_0MAX_LX_PER_SESSION +
						/* 20 pre-LXs/session */
		     sizeof(TPD_SP_CB) * TPD_1MAX_SP_PER_SESSION),
						/* 20 SPs/session */
		poolsize;			
    ULM_RCB	tmp_ulm;
    TPD_LM_CB	*lmcb_p = (TPD_LM_CB *) NULL;
    char	*tran_place;


    /* set up scf control block to allocate memory */

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_scm.scm_functions = SCU_MZERO_MASK;
    scf_cb.scf_scm.scm_in_pages = sizeof(TPD_SV_CB)/SCU_MPAGESIZE + 1;

    /* request memory allocation to SCF */

    if (scf_call(SCU_MALLOC, &scf_cb) != E_DB_OK)
	return( tpd_u1_error(E_TP0002_SCF_MALLOC_FAILED, v_tpr_p) );

    /* initialize facility server control block */

    Tpf_svcb_p = (TPD_SV_CB *)scf_cb.scf_scm.scm_addr;
    v_tpr_p->tpr_need_recovery = FALSE;

    /* Initialize tsv_11_srch_sema4 */
    CSw_semaphore(&Tpf_svcb_p->tsv_11_srch_sema4, CS_SEM_SINGLE, "TPF tsv_srch");

    /* startup TPF ulm pool but borrow QEF's id */

    tmp_ulm.ulm_facility = DB_QEF_ID;
    Tpf_svcb_p->tsv_5_max_poolsize = 
    Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4 = tmp_ulm.ulm_sizepool = 
	(ss_estimate * v_tpr_p->tpr_11_max_sess_cnt);
				    /* double for safety */
    tmp_ulm.ulm_blocksize = sizeof(TPD_LX_CB);/* preferred size for ULM */
    status = ulm_startup(& tmp_ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0014_ULM_STARTUP_FAILED, v_tpr_p) );

    Tpf_svcb_p->tsv_6_ulm_poolid = tmp_ulm.ulm_poolid;  /* must save */
/*
    * initialize ulm structure for allocating session stream *

    sv_ulm_p = & Tpf_svcb_p->tsv_ss_stream_ulm;
    sv_ulm_p->ulm_poolid = tmp_ulm.ulm_poolid;	    
    sv_ulm_p->ulm_streamid = NULL;
    sv_ulm_p->ulm_pptr = NULL;
    sv_ulm_p->ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    sv_ulm_p->ulm_sizepool = ss_estimate * v_tpr_p->tpr_11_max_sess_cnt;
    sv_ulm_p->ulm_blocksize = 0;
    sv_ulm_p->ulm_facility = DB_QEF_ID;
    sv_ulm_p->ulm_psize = 0;
    sv_ulm_p->ulm_smark = NULL;

    * allocate a 2nd pool for session streams *

    status = ulm_startup(sv_ulm_p);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0014_ULM_STARTUP_FAILED, v_tpr_p) );

    Tpf_svcb_p->tsv_memleft = sizeof(TPD_SS_CB) * TPD_1MAX_SESSION_COUNT; 
    				    * set to pool size *
    sv_ulm_p->ulm_memleft = & Tpf_svcb_p->tsv_memleft;
					    * for ULM *
*/
    /*
    ** Control of the print flag for RQF log messages.
    */
    
    NMgtAt("II_TPF_LOG", &tran_place);
    if ((tran_place) && (*tran_place))
    {
	Tpf_svcb_p->tsv_1_printflag = TRUE;
    }
    else
    {
	Tpf_svcb_p->tsv_1_printflag = FALSE;
    }
    
    lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;        
    lmcb_p->tlm_1_streamid = (PTR) NULL;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;
    
    lmcb_p = & Tpf_svcb_p->tsv_9_odx_ulmcb;        
    lmcb_p->tlm_1_streamid = (PTR) NULL;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;
    
    Tpf_svcb_p->tsv_10_slx_ulmcb_p = (TPD_LM_CB *) NULL;
    
    return(E_DB_OK);

#ifdef TPF_2PC
    
    /* transaction log initialization */

    return(tpl_startup(&((*serverpp)->tsv_log)), recover);
#endif
}


/*{
** Name: tpf_m6_shutdown_svr	- TP facility shutdown.
**
** Description: 
**
** Inputs:
**	v_tpr_p
**
** Outputs:
**	None
**
**	Returns:
**		E_DB__OK
**		E_DB_ERROR
**
** Side Effects:
**	    Tpf_svcb_p is freed.
**
** History:
**      mar-10-88 (alexh)
**          created
**      jul-11-89 (carl)
**	    changed to deallocate pool for session streams 
*/


DB_STATUS
tpf_m6_shutdown_svr(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS   status = E_DB_OK;
    SCF_CB	scf_cb;
    ULM_RCB	ulm;


    /* close server pool */

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm_shutdown(& ulm);
/*
    ** close 2nd pool **

    ses_ulm->ulm_facility = DB_QEF_ID;
    ulm_shutdown(ses_ulm);
*/
    /* set up scf control block to free memory */

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_scm.scm_in_pages = sizeof(TPD_SV_CB)/SCU_MPAGESIZE + 1;
    scf_cb.scf_scm.scm_addr = (PTR)Tpf_svcb_p;

    /* free ulm pool */

    status = scf_call(SCU_MFREE, &scf_cb);
    if (status)
	status = tpd_u1_error(E_TP0003_SCF_MFREE_FAILED, v_tpr_p);

    return(status);
}


/*{
** Name: tpf_m7_init_sess	- TPF session initialization.
** 
** Description:  
**	A TPF session control block is allocated from SCF.  Transaction
**	information is initialized.
**
**	
**
** Inputs:
**	v_tpr_p->		TPF request control block
**		tpr_rqf		initialized RQF session control block.
**
** Outputs:
**	v_tpr_p->
**		tpr_session	pointer to facility session control block.
**			tss_rqf	set to tpr_rqf
**
**	Returns:
**	    E_TP0000_OK
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      mar-10-88 (alexh)
**          created
**      may-18-89 (carl)
**	    changed to initialize session->tss_ddl_cc to TRUE
**      jun-27-89 (carl)
**	    1)  fixed return inconsistencies discovered by lint
**	    2)  fixed casting problem discovered by lint in 
**      jul-11-89 (carl)
**	    changed to allocate stream for session CB from server pool
**      oct-06-90 (carl)
**	    added trace-point vector initialization
**
*/


DB_STATUS
tpf_m7_init_sess(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = NULL;
    ULM_RCB	ulm;


    /* 1.  open a stream for session CB */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    /* 1.  Open stream and allocate the session CB */

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPD_SS_CB);	/* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    /* Session's stream is private */
    ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm.ulm_psize = sizeof(TPD_SS_CB);
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0015_ULM_OPEN_FAILED, v_tpr_p) );

    sscb_p = (TPD_SS_CB *) ulm.ulm_pptr;	/* ptr to CB */

    v_tpr_p->tpr_session = (PTR) sscb_p;	/* must return this ptr for QEF 
						   to save away */
    /* initialize this session CB */

    /* rememebr the RQF session id */

    sscb_p->tss_rqf = v_tpr_p->tpr_rqf;
    sscb_p->tss_auto_commit = FALSE;		/* assume */
    sscb_p->tss_ddl_cc = TRUE;			/* assume */

    ult_init_macro(& sscb_p->tss_3_trace_vector, TSS_119_TRACE_POINTS, 0, 0);
    sscb_p->tss_4_dbg_2pc = 0;
    sscb_p->tss_5_dbg_timer = 0;
    sscb_p->tss_6_sscb_streamid = ulm.ulm_streamid;
						/* must save away stream id */
    sscb_p->tss_7_sscb_ulmptr = ulm.ulm_pptr; 

    /* initialize the DX CB within this session CB */

    tpd_m2_init_dx_cb(v_tpr_p, sscb_p);

    return(E_DB_OK);
}


/*{
** Name: tpf_m8_term_sess	- TPF session termination
**
** Description: TPF session control block is freed.
**
** Inputs:
**	v_tpr_p
**		tpr_session		session control block
**
** Outputs:
**
**	v_tpr_p
**		tpr_session		session control block is freed
**
**	Returns:
**		E_TP0000_OK
**		E_TP0003_SCF_MFREE_FAILED
**
** Side Effects:
**	    None
**
** History:
**      aug-15-88 (alexh)
**          created
**      jul-11-89 (carl)
**	    changed to deallocate session CB by closing the dedicated stream
**	jul-10-97 (nanpr01)
**	    Donot deallocate the session cb first and then reference the
**	    deallocated blocks for pointers to deallocate. It might have
**	    disasterous effect when using threads.
**	07-Aug-1997 (jenjo02)
**	    Clear streamid BEFORE deallocating stream.
*/


DB_STATUS
tpf_m8_term_sess(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = (TPD_LM_CB *) NULL;
    ULM_RCB	ulm;


    /* close stream for session savepoints if any */

    ulm.ulm_facility = DB_QEF_ID;
    lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	/* ULM has nullified tlm_1_streamid */

	/* change to ignore any errors */
    }
/*
    ** close stream for pre-registered LXs if any **

    lmcb_p = & dxcb_p->tdx_21_pre_lx_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	** ULM has nullified tlm_1_streamid **

	** change to ignore any errors **
    }
*/
    /* close stream for session registered LXs if any */

    lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	/* ULM has nullified tlm_1_streamid */

	/* change to ignore any errors */
    }

    /* close stream for session CB */

    ulm.ulm_streamid_p = &sscb_p->tss_6_sscb_streamid;
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    v_tpr_p->tpr_session = NULL;

    status = ulm_closestream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
    return(E_DB_OK);
    /* ULM has nullified tss_6_sscb_streamid */
}


/*{
** Name: tpf_m9_transition_ok	- validate state transition
** 
** Description: 
**	Validate a transition from current DX state. This is an internal 
**	primitive.
**
** Inputs:
**      i1_dxstate		current transaction state
**	i2_call		proposed transition
** Outputs:
**		None
**	Returns:
**		TRUE	valid transition
**		FALSE	invalid transition
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      apr-25-88 (alexh)
**          created
**      jun-27-89 (carl)
**	    fixed return inconsistencies discovered by lint 
**	Aug-10-1993 (fpang)
**	    Instead of mid function 'returns()', set 'ret' with the
**	    return value, and return it at one common place at the bottom
**	    of the function. 
*/


bool
tpf_m9_transition_ok(
	i4	i1_dxstate,
	i4	i2_call,
	TPR_CB	*v_tpr_p)
{
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    bool	ret	= FALSE;


    switch(i2_call)
    {
    case TPF_BEGIN_DX:
	switch(i1_dxstate)
	{
	case DX_1STATE_BEGIN:

	    /* error */
	    
	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
	    tpd_p3_prt_dx_state(v_tpr_p);
	    TRdisplay("%s %p: Already in multi-statement transaction!\n",
		IITP_00_tpf_p,
		sscb_p);
	    ret = TRUE;
	    break;
	case DX_0STATE_INIT:
	case DX_5STATE_CLOSED:
	    ret = TRUE;
	    break;
	default:

	    /* error */

	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
	    tpd_p3_prt_dx_state(v_tpr_p);
	    TRdisplay("%s %p: BEGIN TRANSACTION not allowed!\n",
		IITP_00_tpf_p,
		sscb_p);
	    ret = FALSE;
	    break;
	}
	break;
    case TPF_COMMIT:
    case TPF_ABORT:
    	switch(i1_dxstate)
    	{
    	case DX_0STATE_INIT:
    	case DX_1STATE_BEGIN:
	case DX_2STATE_SECURE:
	    ret = TRUE;
	    break;
	case DX_4STATE_COMMIT:
	    if (i2_call == TPF_COMMIT)
	    {
		ret = TRUE;
	    }
	    else
	    {
		tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
		tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
		tpd_p3_prt_dx_state(v_tpr_p);
		TRdisplay("%s %p: Bad DX state transition detected!\n",
		    IITP_00_tpf_p,
		    sscb_p);
 
		ret = FALSE;
	    }
	    break;
	case DX_3STATE_ABORT:
	    if (i2_call == TPF_ABORT)
	    {
		ret = TRUE;
	    }
	    else
	    {
		tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
		tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
		tpd_p3_prt_dx_state(v_tpr_p);
		TRdisplay("%s %p: Bad DX state transition detected!\n",
		    IITP_00_tpf_p,
		    sscb_p);
 
		ret = FALSE;
	    }
	    break;
	default:
	    /* dump state */

	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
	    tpd_p3_prt_dx_state(v_tpr_p);
	    TRdisplay("%s %p: Bad DX state transition detected!\n",
		IITP_00_tpf_p,
		sscb_p);
	    ret = FALSE;
	    break;
	}
	break;
    case TPF_READ_DML:
    case TPF_WRITE_DML:
    	switch(i1_dxstate)
	{
	case DX_0STATE_INIT:
	case DX_1STATE_BEGIN:
	    ret = TRUE;
	    break;

	case DX_2STATE_SECURE:
	case DX_3STATE_ABORT:
	case DX_4STATE_COMMIT:
	default:
	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
	    tpd_p3_prt_dx_state(v_tpr_p);
	    TRdisplay("%s %p: Bad DX state transition detected!\n",
		IITP_00_tpf_p,
		sscb_p);
	    ret = FALSE;
	    break;
    	}
	break;
    default:

	/* error */

	tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	tpd_p4_prt_tpfcall(v_tpr_p, i2_call);
	tpd_p3_prt_dx_state(v_tpr_p);
	ret = FALSE;
	break;
    }
    return(ret);
}


/*{
** Name: tpf_call	- TPF main entry point
** 
** Description: This is the entry point for all TPF requests.
**	tpf_call dispatches the  request to other external
**	primitives. The following  protocol should be followed:
**	tpf_m5_startup_svr must be called before any other request.
**
** Recovery is not done currently but it would've worked as
**	follow: tpr_need_recovery is set the TPR_RECOVER must be
**	called until tpr_need_recovery is reset; TPR_INITIALIZE
**	must be called first before any other session related
**	requests.
**
** Inputs: 
**		i_request		TPF request code
**		v_tpr_p			valid TPR_CB pointer
**
** Outputs:
**	tpr_rcbp
**		tpf_error		detail tpf error info
**		tpf_need_recover	flag indicating recovery is needed
**					only tpf_m5_startup_svr may set 
**					this.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      apr-10-88 (alexh)
**          created
**      mar-29-88 (carl)
**	    fixed NOT to expect session information on system shut
**	    down because there is none 
**      may-18-89 (carl)
**	    added tpf_m11_true_ddl_conc and tpf_m12_false_ddl_conc for 
**	    DDL_CONCURRENCY coordination
**      06-jun-90 (carl)
**	    modified for TPF_INITIALIZE and TPF_SET_DDBINFO
**      08-jun-90 (carl)
**	    modified to properly set the 2PC flag
**      01-SEP-90 (carl)
**	    added normal and error tracing
*/


DB_STATUS
tpf_call(
	i4	i_request,
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = (TPD_DX_CB *) & sscb_p->tss_dx_cb;
    DD_LDB_DESC	*in_cdb_p = & v_tpr_p->tpr_16_ddb_p->
		    dd_d3_cdb_info.dd_i1_ldb_desc,
	    	*dx_cdb_p = (DD_LDB_DESC *) NULL;   /* to be initialized
						    ** before use */
    bool	trace_call_101 = FALSE,
		trace_err_104 = FALSE;
    i4	i4_1, i4_2;
     

    if (i_request > TPF_INITIALIZE)  /* session CB initialized ? */
    {
	/* ok to access session CB */

	if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_TPFCALL_101,
	    & i4_1, & i4_2))
	{
	    trace_call_101 = TRUE;
	    tpd_p4_prt_tpfcall(v_tpr_p, i_request);
	}
	if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
	    & i4_1, & i4_2))
	    trace_err_104 = TRUE;
    }
    v_tpr_p->tpr_error.err_code = E_TP0000_OK;	/* must initialize */

    if (v_tpr_p->tpr_lang_type == 0 || v_tpr_p->tpr_lang_type > DB_SQL)
    {
	v_tpr_p->tpr_lang_type = DB_SQL;	/* always */
    }

    switch(i_request)
    {
/*
    case TPF_RECOVER:
	status = tp2_r0_recover(v_tpr_p);
	Tpf_svcb_p->tsv_4_flags &= ~TSV_01_IN_RECOVERY;
					    ** signal done with recovery **
	break;
*/
    case TPF_P1_RECOVER:
	/* protect the uninitialized search list */
	tpf_u1_p_sema4(TPD_1SEM_EXCLUSIVE, & Tpf_svcb_p->tsv_11_srch_sema4);

	Tpf_svcb_p->tsv_4_flags |= TSV_01_IN_RECOVERY;
	status = tp2_r8_p1_recover(v_tpr_p);

	tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);
	break;
    case TPF_P2_RECOVER:
	status = tp2_r9_p2_recover(v_tpr_p);
	Tpf_svcb_p->tsv_4_flags &= ~TSV_01_IN_RECOVERY;
					    /* signal done with recovery */
	break;
    case TPF_IS_DDB_OPEN:
	status = tpd_m11_is_ddb_open(v_tpr_p);
	break;
    case TPF_SP_ABORT:
	status = tpf_m3_abort_to_svpt(v_tpr_p);
	break;
    case TPF_SAVEPOINT:
	status = tpf_m4_savepoint(v_tpr_p);
	break;
    case TPF_STARTUP:
	status = tpf_m5_startup_svr(v_tpr_p);
	break;
    case TPF_SHUTDOWN:
	status = tpf_m6_shutdown_svr(v_tpr_p);
	break;
    case TPF_INITIALIZE:
	status = tpf_m7_init_sess(v_tpr_p);
	if (status)
	    return(status);

	if (v_tpr_p->tpr_16_ddb_p != (DD_DDB_DESC *) NULL)
	{
	    /* MUST set up uninitialized pointers to session and DX CBs
	    ** before using them */

	    sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
	    dxcb_p = (TPD_DX_CB *) & sscb_p->tss_dx_cb;
	    STRUCT_ASSIGN_MACRO(*v_tpr_p->tpr_16_ddb_p, dxcb_p->tdx_ddb_desc);
	    dx_cdb_p = & dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
	    if (! (dx_cdb_p->dd_l6_flags & DD_01FLAG_1PC))
		dx_cdb_p->dd_l6_flags |= DD_02FLAG_2PC;
					/* a 2PC DDB */
	}
	break;
    case TPF_TERMINATE:
	status = tpf_m8_term_sess(v_tpr_p);
	break;
    case TPF_COMMIT:
    case TPF_ABORT:
    case TPF_READ_DML:
    case TPF_WRITE_DML:
    case TPF_BEGIN_DX:
	status = tpf_m1_process(i_request, v_tpr_p);
	break;
    case TPF_TRACE:
	status = tpf_m2_trace(v_tpr_p);
	break;
    case TPF_SET_DDBINFO:
	STRUCT_ASSIGN_MACRO(*v_tpr_p->tpr_16_ddb_p, dxcb_p->tdx_ddb_desc);
    	dx_cdb_p = & dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
	if (! (dx_cdb_p->dd_l6_flags & DD_01FLAG_1PC))
	    dx_cdb_p->dd_l6_flags |= DD_02FLAG_2PC;
					/* a 2PC DDB */
	break;

    case TPF_1T_DDL_CC:	    /* SET DDL_CONCURRENCY ON */
	sscb_p->tss_ddl_cc = TRUE;
	break;

    case TPF_0F_DDL_CC:	    /* SET DDL_CONCURRENCY OFF */
	sscb_p->tss_ddl_cc = FALSE;
	break;

    case TPF_OK_W_LDBS:    /* validate updating LDBs for this DX */
	status = tpd_m5_ok_write_ldbs(v_tpr_p);
	break;

    default:
	if (trace_err_104)
	{
	    tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	    TRdisplay("%s %p: UNRECOGNIZD request %d detected!\n",
		IITP_00_tpf_p,
		sscb_p,
		i_request);
	}
/*
	v_tpr_p->tpr_error.err_code = E_TP0001_INVALID_REQUEST;
	status = E_DB_ERROR;
*/
	return( tpd_u1_error(E_TP0001_INVALID_REQUEST, v_tpr_p) );
	break;
    }
    return(status);
}

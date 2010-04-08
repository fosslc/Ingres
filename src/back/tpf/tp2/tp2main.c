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
#include    <tm.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpferr.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**
** Name: TP2MAIN.C - TPF's 2PC primitives
** 
** Description: 
**	This file includes all the 2PC primitives.
** 
**	    tp2_m0_commit_dx	- commit the DX
**	    tp2_m1_end_1pc	- do 1PC termination: commit or abort
**	    tp2_m2_end_2pc	- do 2PC commit (or abort if necessary)
**	    tp2_m3_p0_commit_read_lxs	
**				- phase 0: commit all the read-only LXs
**	    tp2_m4_p1_secure_write_lxs	
**				- phase 1: do 2PC secure
**	    tp2_m5_p2_close_all_lxs	
**				- phase 2: do 2PC commit or abort
**	    tp2_m6_p3_loop_to_close_lxs
**				- loop until all LXs are committed or aborted
**	    tp2_m7_restart_and_close_lxs
**				- restart the lost LDBs and close their LXs
**
**  History:    $Log-for RCS$
**      nov-07-89 (carl)
**          split off from tpdmisc.c for 2PC
**      jul-07-89 (carl)
**	    changed tp2_m4_p1_secure_write_lxs to use LX id instead of DX id
**      oct-13-90 (carl)
**	    process trace points
**	feb-11-91 (fpang)
**	    In tp2_m4_p1_secure_write_lxs(), tp2_m7_restart_and_close_lxs(),
**	    and tp2_m5_p2_close_all_lxs(), check for tracepoints before 
**	    advancing the lxcb_p pointer. Will get a SEGV if only 1 lxcb in 
**	    list.
**	    Fixes B34866.
**	    Also added tracepoints TSS_76_SUSPEND_IN_COMMIT, 
**	    and TSS_86_SUSPEND_IN_ABORT to tp2_m7_restart_and_close_lxs().
**	09-APR-1992 (fpang)
**	    In tp2_m5_p2_close_all_lxs(), in the willing commit case, if 
**	    connection is lost, and restart succeeds, resend the commit/abort. 
**	    This will usually happen if only the local thread is lost, but the 
**	    local server stays up. The restart will succeed because the local 
**	    server is still up. The second case is the local server crashing, 
**	    so there are no local servers, and the restart here will fail. 
**	    This latter case is taken care of in tp2_m7_restart_and_close_lxs().
**	    Fixes B42810 (SEGV if LDB killed between SECURE and COMMIT).
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	13-oct-1993 (tam)
**	    Changed 2PC/1PC such that if read only sites error on commit (due to
**	    communication or whatever error), the entire dtx is aborted.
**	    Changes in tp2_m1_end_1pc(), tp2_m2_end_2pc() and 
**	    tp2_m3_p0_commit_read_lxs()
**	    Fixes B46672 - 1PC transactions may result in inconsistant db's
**	        when errors occur on read-only sites.
**	28-jan-1994 (peters)
**	    Changed tp2_m1_end_1pc to fix b58439:
**	    Changed the 'if' condition in the 'round-loop'  from
**		(((round == 0) && (lxcb_p->tlx_write == TRUE)) ||
**		 ((round == 1) && (lxcb_p->tlx_write == FALSE))))
**	    to
**		(((round == 1) && (lxcb_p->tlx_write == TRUE)) ||
**		 ((round == 0) && (lxcb_p->tlx_write == FALSE))))
**	    So that in round 0, readers would be processed and in round 1,
**	    writer(s) would be processed.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in tp2_m1_end_1pc(),
**	    tp2_m2_end_2pc(), tp2_m3_p0_commit_read_lxs().
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
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


GLOBALREF   struct _TPD_SV_CB	*Tpf_svcb_p;
						/* TPF facility control block */
GLOBALREF   char                *IITP_00_tpf_p;
 
GLOBALREF   char                *IITP_02_commit_p;
 
GLOBALREF   char                *IITP_03_abort_p;
 
GLOBALREF   char                *IITP_05_reader_p;
 
GLOBALREF   char                *IITP_06_updater_p;
 
GLOBALREF   char                *IITP_09_3dots_p;
 
GLOBALREF   char                *IITP_14_aborting_p;
 
GLOBALREF   char                *IITP_15_committing_p;
 
GLOBALREF   char                *IITP_17_failed_with_err_p;
 
GLOBALREF   char                *IITP_18_fails_p;
 
GLOBALREF   char                *IITP_19_succeeds_p;
 
GLOBALREF   char                *IITP_21_2pc_cats_p;
 
GLOBALREF   char                *IITP_22_aborted_ok_p;
 
GLOBALREF   char                *IITP_23_committed_ok_p;
 

/*{
** Name: tp2_m0_commit_dx - commit current transaction
** 
**  Description:    
**	Depending on the DX being 1pc or 2pc, tp2_m1_end_1pc or tp2_m2_end_2pc
**	is called to commit the transaction.
**
** Inputs:
**		v_tpr_p			
**		    tpr_session		pointer to session control block
** Outputs:
**		v_tpr_p->
**		    tpr_session
**			.tpd_state	set to closing state of the transaction
**	Returns:
**	    DB_STATUS
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    May log.
**
** History:
**      11-3-88 (alexh)
**          created
**      jun-22-89 (carl)
**	    fixed return inconsistencies discovered by lint 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	2-oct-98 (stephenb)
**	    Update code to deal with concept of 1PC+
*/


DB_STATUS
tp2_m0_commit_dx(
		 TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB   *dxcb_p = & sscb_p->tss_dx_cb;


    switch(dxcb_p->tdx_mode)
    {
    case DX_0MODE_NO_ACTION:

    	return(E_DB_OK);	    /* nothing to do */
    	break;

    case DX_1MODE_READ_ONLY:
    case DX_3MODE_HARD_1PC:
    case DX_4MODE_SOFT_1PC:

	status = tp2_m1_end_1pc(v_tpr_p, TPD_1DO_COMMIT);
	break;

    case DX_2MODE_2PC:
    case DX_5MODE_1PC_PLUS:

	status = tp2_m2_end_2pc(dxcb_p, v_tpr_p);

	/* enter CLOSED state */

	dxcb_p->tdx_state = DX_5STATE_CLOSED;
	break;

    default:
	return( tpd_u1_error(E_TP0005_UNKNOWN_STATE, v_tpr_p) );
	break;
    }

    return(status);
}


/*{ 
** Name: tp2_m1_end_1pc	- send 1pc message to all LDBs in DX
** 
**   Description:  
**	Execute the 1pc protocol.  This is done for read-only, 1-site update,
**	user-requested abort.  Acknowldgements from LDBs are not necessary.
**
**	The commit message is sent to all active LDBs. The DX's
**	state is reset to DX_2STATE_INIT and the state of all the LXs
**	to the idle LX_0STATE_NIL state.
** 
** Inputs: 
**		v_tpr_p			TPF_RCB
**		i_to_commit		TRUE if committing a 1PC, 
**					FALSE if aborting either a 1PC or 2PC 
**					    transaction
** Outputs:
**		v_tpr_p->
**		    tpr_session->
**			tss_dxcb_p->
**			    tdx_state	set to DX_2STATE_INIT
**	Returns:
**		E_TP0009_TXN_FAILED
**		E_TP0000_OK 
**	Exceptions: 
**		None
** 	Side Effects: 
**		None 
** History: 
**	may-02-88 (alexh) 
**	    created 
**	nov-16-89 (carl)
**	    extended for 2PC
**      oct-13-90 (carl)
**	    process trace points
**	oct-13-93 (tam  & fpang)
**	    The existing protocol is incorrect in handling commit. 
**	    The correct protocol is two rounds. In round 1, the readers are sent
**	    the commit messages.  Only if everyone succeeds, then is the write 
**	    site (one in the case for commit) sent the commit message.
**	    Fixes B46672 - 1PC transactions may result in inconsistant db's
**	        when errors occur on read-only sites.
**	jan-28-94 (peters)
**	    Changed the 'if' condition in the 'round-loop'  from
**		(((round == 0) && (lxcb_p->tlx_write == TRUE)) ||
**		 ((round == 1) && (lxcb_p->tlx_write == FALSE))))
**	    to
**		(((round == 1) && (lxcb_p->tlx_write == TRUE)) ||
**		 ((round == 0) && (lxcb_p->tlx_write == FALSE))))
**	    So that in round 0, readers would be processed and in round 1,
**	    writer(s) would be processed.
**	    
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/

 
DB_STATUS
tp2_m1_end_1pc(
	TPR_CB	    *v_tpr_p,
	bool	    i_to_commit)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK; /* must intialize to OK */
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p;
    i4		ldb_cnt;
    RQR_CB	rqr_cb;
    i4		rqf_op;
    bool	trace_err_104 = FALSE,
		trace_dx_100 = FALSE,
		round_1_abort = FALSE;
    i4		round;
    i4     i4_1,
                i4_2;
    char        *who_p = (char *) NULL,
		*which_p = (char *) NULL;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
    }

    /* build a list of RQF requests and call RQF */ 

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    rqr_cb.rqr_session = sscb_p->tss_rqf;

    if (trace_dx_100)
    {
	if (i_to_commit)
	    which_p = IITP_15_committing_p;
	else
	    which_p = IITP_14_aborting_p;

	STprintf(cbuf, 
		"%s %p: %s DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		which_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    /* set up for calling RQF */
/*
    if (i_to_commit)
    	rqf_op = RQR_COMMIT;
    else
    	rqf_op = RQR_ABORT;
*/
    ldb_cnt = 1;

    /*
    ** two rounds
    */
    for (round = 0; round < 2; round++)
    {
	for (lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
	     lxcb_p != (TPD_LX_CB *) NULL;
	     lxcb_p = lxcb_p->tlx_22_next_p)
	{
	    /* do something only if LDB is NOT in the idle LX_1STATE_INITIAL 
	    ** state 
	    ** In round 0, skip the writer(s)
	    ** In round 1, skip the readers
	    */

	    if (lxcb_p->tlx_20_flags & LX_01FLAG_REG 
		&&
		lxcb_p->tlx_state != LX_1STATE_INITIAL
		&&
		(((round == 1) && (lxcb_p->tlx_write == TRUE)) ||
		 ((round == 0) && (lxcb_p->tlx_write == FALSE))))
	    {
		if (lxcb_p->tlx_write)
		{
		    who_p = IITP_06_updater_p;

		    if (i_to_commit && (round_1_abort == FALSE))
		    {
			rqf_op = RQR_COMMIT;
			which_p = IITP_15_committing_p;
		    }
		    else
		    {
			rqf_op = RQR_ABORT;
			which_p = IITP_14_aborting_p;
		    }
		}
		else
		{
		    /* always commit a reader */

		    rqf_op = RQR_COMMIT;
		    who_p = IITP_05_reader_p;
		    which_p = IITP_15_committing_p;
		}
		/* reset the LX state before ending the LX */

		lxcb_p->tlx_state = LX_0STATE_NIL;
		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;

		rqr_cb.rqr_1_site = &lxcb_p->tlx_site;

		/* determine the appropriate transaction protocol message */

		rqr_cb.rqr_q_language = lxcb_p->tlx_txn_type;
		rqr_cb.rqr_timeout = 0;
		status = tpd_u4_rqf_call(rqf_op, & rqr_cb, v_tpr_p);
		if (status)
		{
		    /* round 0 has failed; abort the writer */
		    if (round == 0)
			round_1_abort = TRUE;

		    prev_sts = status;
		    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		    if (trace_err_104)
		    {
			/* report failure */

			STprintf(cbuf, 
			        "%s %p: %s%s %s %d)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				which_p,
				who_p,
				ldb_cnt);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, 
					   & lxcb_p->tlx_site, TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %x: %sfailed with ERROR %x\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p,
			    v_tpr_p->tpr_error.err_code);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		}
		else
		{
		    /* close ok */

		    if (trace_dx_100)
		    {
		        STprintf(cbuf, 
				"%s %p: %s%s %s %d)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				which_p,
				who_p,
				ldb_cnt);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, 
					   & lxcb_p->tlx_site, TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %p: %ssucceeds\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		}
		ldb_cnt++;
		lxcb_p->tlx_write = FALSE;	/* must defer until now */
	    }   /* if .. */
	}    /* for lxcb_p*/
    }	/* for round */
/*
    status = tpd_m3_term_dx_cb(v_tpr_p, sscb_p);    
*/
    if (prev_sts)
    {
	/* return failure */

	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }
EXIT_1PC:
    /* set up for tracing */

    if (i_to_commit)
    	which_p = IITP_23_committed_ok_p;
    else
    	which_p = IITP_22_aborted_ok_p;

    if (status)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		    "%s %p: DX completes\n",	/* considered OK */
		    IITP_00_tpf_p,
		    sscb_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else 
    {
	/* no error */

	if (trace_dx_100)
	{
	}
    }
    dxcb_p->tdx_state = DX_5STATE_CLOSED;

    return(status);
}


/*{
** Name: tp2_m2_end_2pc - do 2PC termination
** 
**  Description:    
**	2PC termination:
**
**	    1.  Phase 0: Commit all readers if any.
**	    2.  Phase 1: Secure all updaters.
**	    3.  Phase 2: Commit or abort all updaters.
**
** Inputs:
**	v_dxcb_p			DX CB ptr
**	v_tpr_p				session control block pointer
**	    tpr_session			pointer to session control block
** Outputs:
**	v_tpr_p->
**	    tpr_session
**		.tpd_state		set to closing state of the transaction
**	Returns:
**	    DB_STATUS
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    
**
** History:
**      jun-22-89 (carl)
**          created
**      oct-13-90 (carl)
**	    process trace points
**	oct-13-93 (tam)
**	    In phase0, if committing any read site has failed, abort DX.
**	    Fixes B46672 - 1PC transactions may result in inconsistant db's
**	        when errors occur on read-only sites.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


DB_STATUS
tp2_m2_end_2pc(
	TPD_DX_CB   *v_dxcb_p,
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    char	*state_p = IITP_02_commit_p;	/* assume */
    bool	logged = FALSE,	    /* assume */
		secured = FALSE,    /* assume */
		fall_thru_b = FALSE,	/* must so initialized */
                trace_err_104 = FALSE,
		trace_dx_100 = FALSE;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
	STprintf(cbuf,
	    "%s %p: %s DX %x %x%s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_15_committing_p,
	    v_dxcb_p->tdx_id.dd_dx_id1, 
	    v_dxcb_p->tdx_id.dd_dx_id2,
	    IITP_09_3dots_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    /* Phase 0.  commit all the readers */

    if (trace_dx_100)
    {
	STprintf(cbuf,
	    "%s %p: Phase 0) Committing all READ-ONLY LXs%s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    status = tp2_m3_p0_commit_read_lxs(v_tpr_p);

    if (status)
    {
	/* 
	** any error in read phase should abort DX 
	*/
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %x: %sFailed to commit all READ-ONLY LXs\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: Aborting all LXs%s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	v_dxcb_p->tdx_state = DX_3STATE_ABORT;
	status = tp2_m1_end_1pc(v_tpr_p, TPD_0DO_ABORT);

	v_dxcb_p->tdx_state = DX_5STATE_CLOSED;

	goto EXIT_2PC;
    }

    /* Phase 1.  do DX SECURE */

    if (trace_dx_100)
    {
	STprintf(cbuf,
	    "%s %p: Phase 1) Securing all UPDATE LXs%s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    v_dxcb_p->tdx_state = DX_2STATE_SECURE;
    status = tp2_m4_p1_secure_write_lxs(v_dxcb_p, v_tpr_p, & logged, 
		& secured);
    if (status)
    {
	if (status == E_DB_FATAL)
	    goto EXIT_2PC;

	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    if (! logged)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: %sCannot log SECURE state due to ERROR %x\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		v_tpr_p->tpr_error.err_code);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	/* Phase 2.  failed to log the secure state, do 1PC ABORT */

	if (trace_dx_100)
	{
	    STprintf(cbuf,
		"%s %p: Phase 2) Aborting all UPDATE LXs%s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	v_dxcb_p->tdx_state = DX_3STATE_ABORT;
	status = tp2_m1_end_1pc(v_tpr_p, TPD_0DO_ABORT);

	if (prev_sts)
	{
	    status = prev_sts;
	    STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	}
	else
	    status = tpd_u1_error(E_TP0021_LOGDX_FAILED, v_tpr_p);

	v_dxcb_p->tdx_state = DX_5STATE_CLOSED;

	goto EXIT_2PC;
    }

    if (! secured)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: %sCannot secure DX %x %x due to ERROR %x\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		v_dxcb_p->tdx_id.dd_dx_id1, 
		v_dxcb_p->tdx_id.dd_dx_id2,
		v_tpr_p->tpr_error.err_code);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	/* Phase 2.  DX logged but failed to secure, do 2PC ABORT */

	state_p = IITP_03_abort_p;

	if (trace_dx_100)
	{
	    STprintf(cbuf,
		"%s %p: Phase 2) Aborting all UPDATE LXs%s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	v_dxcb_p->tdx_state = DX_3STATE_ABORT;

	status = tp2_m5_p2_close_all_lxs(v_dxcb_p, TPD_0DO_ABORT, 
		    TPD_1LOG_STATE_YES, v_tpr_p);
	
	if (status == E_DB_FATAL)
	    goto EXIT_2PC;
/*
	v_dxcb_p->tdx_state = DX_5STATE_CLOSED;
	if (prev_sts)
	{
	    STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	    status = prev_sts;
	}
	goto EXIT_2PC;
*/
    }
    else
    {
	/* Phase 2.  do COMMIT */

	if (trace_dx_100)
	{
	    STprintf(cbuf,
		"%s %p: Phase 2) Committing all UPDATE LXs%s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	state_p = IITP_02_commit_p;

	v_dxcb_p->tdx_state = DX_4STATE_COMMIT;
	status = tp2_m5_p2_close_all_lxs(v_dxcb_p, TPD_1DO_COMMIT, 
		    TPD_1LOG_STATE_YES, v_tpr_p);
	if (status == E_DB_FATAL)
	    goto EXIT_2PC;
/*
	if (status)
	{
	    ** failed to commit **

	    v_dxcb_p->tdx_state = DX_5STATE_CLOSED;

	    goto EXIT_2PC;
	}
*/
    }

    if (status)
    {
	if (prev_sts == E_DB_OK)
	{
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	}
	/* Phase 3.  failed to close all LXs, go into looping */

	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: ...Broadcasting %s to all LDBs failed\n", 
		IITP_00_tpf_p,
		sscb_p,
		state_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

	    STprintf(cbuf, 
		"%s %p: ...Rebroadcasting %s to all LDBs begins%s\n", 
		IITP_00_tpf_p,
		sscb_p,
		state_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	if (trace_dx_100)
	{
	    STprintf(cbuf,
		"%s %p: Phase 3) Entering loop to close LXs%s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	status = tp2_m6_p3_loop_to_close_lxs(v_dxcb_p, v_tpr_p);
	if (status)
	{
	    if (trace_err_104)
	    {
		STprintf(cbuf, 
"%s %p: ...Loop to broadcast %s for DX failed with ERROR %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_tpr_p->tpr_error.err_code);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }

	    if (status == E_DB_FATAL)
		goto EXIT_2PC;
	    if (prev_sts)
	    {
		status = prev_sts;
		STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	    }
	    goto EXIT_2PC;
	}
	else
	{
	    /* here if phase 3 successful */

	    if (trace_dx_100)
	    {
		STprintf(cbuf, 
		    "%s %p: ...Rebroadcasting %s for DX succeeds\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	}
    }

    /* 4.  delete the DX from the log; must always succeed */

    status = tp2_c5_loop_to_delete_dx(v_dxcb_p, v_tpr_p);  
    if (status)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: %sPurging DX from 2PC catalogs failed\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
	if (status == E_DB_FATAL)
	    goto EXIT_2PC;

	if (prev_sts)
	{
	    status = prev_sts;
	    STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	}
	goto EXIT_2PC;
    }
    else
    {
	/* DX purged from catalogs */

	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: %sDX purged from %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_21_2pc_cats_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }

    /* 3.  enter CLOSED state */

    v_dxcb_p->tdx_state = DX_5STATE_CLOSED;

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

EXIT_2PC:

    if (status)
    {
	/* here only if fatal error */

	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: DX fails to commit\n",
		IITP_00_tpf_p,
		sscb_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else
    {
	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: DX %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_23_committed_ok_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    return(status);
}


/*{ 
** Name: tp2_m3_p0_commit_read_lxs - commit all the read-only LXs
** 
**   Description:  
**	Send commit message to all the read-only sites.  Acknowldgements 
**	from such sites are not necessary.
**
** Inputs: 
**	v_tpr_p->			    TPF_RCB
**	    tpr_session->
**		.tss_dxcb
** Outputs:
**	none
**	
**	Returns:
**	    DB_STATUS
**		
**	Exceptions: 
**		None
** 	Side Effects: 
**		None 
** History: 
**	nov-16-89 (carl)
**	    created 
**      oct-13-90 (carl)
**	    process trace points
**	oct-13-93 (tam)
**	    Changed protocol to return error status if ldb errors out.
**	    dx should be aborted.
**	    Fixes B46672 - 1PC transactions may result in inconsistant db's
**	        when errors occur on read-only sites.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/

 
DB_STATUS
tp2_m3_p0_commit_read_lxs(
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p;
    i4		lx_cnt,
		ldb_cnt,
		tmp_cnt;
    RQR_CB	rqr_cb;
    bool	trace_err_104 = FALSE,
		trace_dx_100 = FALSE;
    i4     i4_1,
                i4_2;
    char	*str_p = (char *) NULL;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
    }

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    rqr_cb.rqr_session = sscb_p->tss_rqf;

    /* send commit message to all read-only sites */

    lx_cnt = 0;
    ldb_cnt = 1;
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG    /* registered in the DX */
	    &&
	    (! lxcb_p->tlx_write))		    /* a reader */
	{
	    /* check LX state */

	    switch (lxcb_p->tlx_state)
	    {
	    case LX_1STATE_INITIAL:
	    case LX_7STATE_ACTIVE:
		break;
	    default:
		return( tpd_u2_err_internal(v_tpr_p) );
	    }

	    /* reset the LX state before ending the LX */

	    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; /* unregister it */
	    lxcb_p->tlx_state = LX_0STATE_NIL;	    /* reset state */
	    lxcb_p->tlx_write = FALSE;

	    rqr_cb.rqr_1_site = & lxcb_p->tlx_site;

	    /* determine the appropriate transaction protocol message */

	    rqr_cb.rqr_q_language = lxcb_p->tlx_txn_type;
	    rqr_cb.rqr_timeout = 0;
	    status = tpd_u4_rqf_call(RQR_COMMIT, & rqr_cb, v_tpr_p);

	    if (status)
	    {
		/*
		** error return; dx has to be aborted.
		*/

		if (trace_err_104)
		{
	    	    STprintf(cbuf, 
			"%s %p: %sLDB %d)\n",
			IITP_00_tpf_p,
			sscb_p,
		        IITP_09_3dots_p,
			ldb_cnt);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
	        STprintf(cbuf, 
		    "%s %p: %sFAILED to commit\n",
		    IITP_00_tpf_p,
		    sscb_p,
                    IITP_09_3dots_p);
	        tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		return(status);
	    }

	    if (trace_dx_100 || trace_err_104)
            {
		STprintf(cbuf, 
		    "%s %p: %sLDB %d)\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p,
		    ldb_cnt);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		STprintf(cbuf, 
		    "%s %p: %sdone\n",
		    IITP_00_tpf_p,
		    sscb_p,
                    IITP_09_3dots_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    ++ldb_cnt;
	    ++lx_cnt;
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
    }

    if (trace_dx_100)
    {
	tmp_cnt = ldb_cnt - 1;
	switch(tmp_cnt)
	{
	case 0:
	    STprintf(cbuf, 
		"%s %p: %sNo %ss to commit\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_05_reader_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    break;
	case 1:
	    STprintf(cbuf, 
		"%s %p: %s%d LX %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		tmp_cnt,
		IITP_23_committed_ok_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    break;
	default:
	    STprintf(cbuf, 
		"%s %p: %s%d LXs %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		tmp_cnt,
		IITP_23_committed_ok_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    return(E_DB_OK);
}


/*{
** Name: tp2_m4_p1_secure_write_lxs - poll all sites for 2pc decision
**
** Description:
**
**	This routine does 2 things:
**
**	    1.  Log a) the DX in SECURE state, b) all LXs.
**	    2.  Boradcast SECURE message to all the update LDBs and return 
**		the polling result.  
**
**	There are 2 error situations:
**	
**	1)  An LDB refuses to commit.  ABORT is returned.
**	2)  Any other error will cause an immediate return.  
**
** Inputs:
**	i1_dxcb_p			ptr to DX conotrol block 
**	v_tpr_p				ptr to session context
**
** Outputs:
**	o1_logged_p			ptr to boolean result: TRUE if
**					logging successful, FALSE otherwise
**					implying that all the sites have been
**					1pc-aborted
**	o2_secured_p			ptr to boolean result: TRUE if all
**					sites secured, FALSE otherwise implying 
**					that the DX must be 2pc-aborted
**	Returns:
**	    DB_STATUS			E_DB_ERROR implies failure in 1 way or
**					or another
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
**      jul-07-89 (carl)
**	    changed to use LX id instead of DX id
**      oct-13-90 (carl)
**	    process trace points
**	feb-11-91 (fpang)
**	    Check for tracepoints before advancing lxcb_p pointer.
**	    Will get a SEGV if only 1 lxcb in list.
**	1-oct-98 (stephenb)
**	    We can now handle 1PC plus transactions, don't poll sites that
**	    are not capable of 2PC (there should be only 1).
*/


DB_STATUS
tp2_m4_p1_secure_write_lxs(
	TPD_DX_CB	*i1_dxcb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_logged_p,
	bool		*o2_secured_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & i1_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    i4		secure_cnt;
    RQR_CB	rqr_cb;
    bool	restart_b = FALSE,
                trace_err_104 = FALSE,
		trace_dx_100 = FALSE;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
    }
/*
    ** done by tp2_c1_log_dx **

    if (sscb_p->tss_4_dbg_2pc == TSS_60_CRASH_BEF_SECURE)
    {
	status = tp2_d0_crash(v_tpr_p);
	goto EXIT_SECURE;
    }
*/
    *o1_logged_p = FALSE;			/* assume */
    *o2_secured_p = FALSE;			/* assume */

    /* 1.  log the DX and all LX information in SECURE state */

    status = tp2_c1_log_dx(i1_dxcb_p, v_tpr_p);
    if (status)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: %sFailed to log SECURE state\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
	goto EXIT_SECURE;
    }
    else
    {
	/* DX logged OK */

	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: %sSECURE state of DX logged to %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_21_2pc_cats_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }

    *o1_logged_p = TRUE;


    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    rqr_cb.rqr_session = sscb_p->tss_rqf;

    /* 2.  send the SECURE message to all the remaining updates */

    rqr_cb.rqr_session = sscb_p->tss_rqf;	    /* RQF ssession ptr */
    rqr_cb.rqr_timeout = 0;
/*
    rqr_cb.rqr_2pc_dx_id = & i1_dxcb_p->tdx_id;
*/
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    secure_cnt = 0;
    while(lxcb_p != (TPD_LX_CB *) NULL)
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG    /* registered */
	    &&
	    lxcb_p->tlx_write			    /* an updater */
	    &&
	    lxcb_p->tlx_state == LX_7STATE_ACTIVE  /* not polled yet */
	    &&
	    (lxcb_p->tlx_20_flags & LX_02FLAG_2PC)) /* database is 2PC capable */
	{
	    /* set the LX state before polling */

	    lxcb_p->tlx_state = LX_2STATE_POLLED;

	    rqr_cb.rqr_1_site = & lxcb_p->tlx_site;
	    rqr_cb.rqr_q_language = lxcb_p->tlx_txn_type;
	    rqr_cb.rqr_2pc_dx_id = & lxcb_p->tlx_23_lxid;

	    status = tpd_u4_rqf_call(RQR_SECURE, & rqr_cb, v_tpr_p);
	    if (status == E_DB_OK)
	    {
		lxcb_p->tlx_state = LX_3STATE_WILLING;
		if (trace_dx_100)
		{
		    STprintf(cbuf, 
			"%s %p: %sLDB %d)\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p,
			secure_cnt + 1);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %ssecured\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		++secure_cnt;
	    }
	    else
	    {
		/* error condition, there are 2 cases: 

		    1)  SECURE refused, 
		    2)  an error occurred
		*/

		if (rqr_cb.rqr_error.err_code == E_RQ0048_SECURE_FAILED)
		{
		    /* SECURE refused */

		    if (trace_err_104)
		    {
			STprintf(cbuf, 
			    "%s %p: LDB %d)\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    secure_cnt + 1);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
			    TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %p: %sREFUSED to commit\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		    (VOID) tpd_u9_2pc_ldb_status(& rqr_cb, lxcb_p);
		    lxcb_p->tlx_state = LX_8STATE_REFUSED;  
		}
		else if (rqr_cb.rqr_error.err_code == E_RQ0045_CONNECTION_LOST) 
							    /* case 2: LDB
							    ** lost */
		{
		    lxcb_p->tlx_state = LX_7STATE_ACTIVE;   /* restore state */
		    lxcb_p->tlx_ldb_status = LX_09LDB_ASSOC_LOST;
		}
		goto EXIT_SECURE;	/* return to abort DX */
	    }
	}
	if (secure_cnt > 1 || lxcb_p->tlx_22_next_p == (TPD_LX_CB *) NULL)
	{
	    switch (sscb_p->tss_4_dbg_2pc)
	    {
	    case TSS_63_CRASH_IN_POLLING:
		status = tp2_d0_crash(v_tpr_p);
		goto EXIT_SECURE;
		break;

	    case TSS_68_SUSPEND_IN_POLLING:
		tp2_d1_suspend(v_tpr_p);
		break;

	    case TSS_80_CRASH_BEF_ABORT:
	    case TSS_81_CRASH_IN_ABORT:
	    case TSS_82_CRASH_AFT_ABORT:
	    case TSS_83_CRASH_LX_ABORT:

	    case TSS_85_SUSPEND_BEF_ABORT:
	    case TSS_86_SUSPEND_IN_ABORT:
	    case TSS_87_SUSPEND_AFT_ABORT:
	    case TSS_88_SUSPEND_LX_ABORT:

		/* simulate a REFUSE situation */

		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p:    simulated REFUSE\n",
			IITP_00_tpf_p,
			sscb_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		rqr_cb.rqr_error.err_code = E_RQ0048_SECURE_FAILED;
		status = tpd_u1_error(E_RQ0048_SECURE_FAILED, v_tpr_p);

		(VOID) tpd_u9_2pc_ldb_status(& rqr_cb, lxcb_p);
		lxcb_p->tlx_state = LX_8STATE_REFUSED;  
		goto EXIT_SECURE;    /* *o2_secured_p == FALSE */
		break;

	    default:
		break;
	    }
	}
	lxcb_p = lxcb_p->tlx_22_next_p; /* advance to next LX */
    }
/*
    ** done by tp2_c1_log_dx **

    if (sscb_p->tss_4_dbg_2pc == TSS_62_CRASH_AFT_SECURE) 
    {
	status = tp2_d0_crash(v_tpr_p);
	goto EXIT_SECURE;
    }
*/
EXIT_SECURE:
    
    if (status)
    {
	if (trace_err_104)
	{
	    STprintf(cbuf, 
		"%s %p: %sFailed to secure all LXs\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else
    {
	*o2_secured_p = TRUE;

	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: %sAll LXs secured successfully\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }

    return(status);
}


/*{
** Name: tp2_m5_p2_close_all_lxs - commit/abort a DX 
** 
**  Description:  
**	Phase 2 for closing a SECURED DX.  OK to return if LDBs are lost.
**
** Inputs:
**	i1_dxcb_p			DX CB ptr
**	i2_commit_b			TRUE if committing, FALSE if aborting
**	i3_logstate_B			TRUE if to log state, FALSE otherwise
**	v_tpr_p				session control block pointer
**
** Outputs:
**	v_sscb_p->tss_txn.
**			tpd_state	set to tp2_m4_abort_dx
**			tpd_head_slave	acknowledge LDBs are marked off.
**	Returns:
**	    E_DB_OK			DX termination OK
**	    E_DB_ERROR			DX cannot be closed
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-19-89 (carl)
**          created
**      oct-13-90 (carl)
**	    process trace points
**	feb-11-91 (fpang)
**	    Check for tracepoints before advancing lxcb_p pointer.
**	    Will get a SEGV if only 1 lxcb in list.
**	09-APR-1992 (fpang)
**	    In the willing commit case, if connection is lost, and restart 
**	    succeeds, resend the commit/abort. This will usually happen if 
**	    only the local thread is lost, but the local server stays up. The 
**	    restart will succeed because the local server is still up. The 
**	    second case is the local server crashing, so there are no local 
**	    servers, and the restart here will fail. This latter case is taken 
**	    care of in tp2_m7_restart_and_close_lxs().
**	    Fixes B42810 (SEGV if LDB killed between SECURE and COMMIT).
**	1-oct-98 (stephenb)
**	    We now support 1PC plus in this function. We must check for that
**	    state and make sure we send the commit to the 1PC (only) capable
**	    site first.
**      13-feb-03 (chash01) x-integrate change#461908
**          Initialization of ldb_cnt falls behind referencing it. Move it
**          up to the top of the routine.
*/


DB_STATUS
tp2_m5_p2_close_all_lxs(
	TPD_DX_CB	*i1_dxcb_p,
	bool		 i2_commit_b,
	bool		 i3_logstate_b,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & i1_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p;
    i4		lx_cnt,
		ldb_cnt,
		result;
    bool	displayed = FALSE,
                trace_err_104 = FALSE,
		trace_dx_100 = FALSE;
    i4     i4_1,
                i4_2;
    char	*state_p = (char *) NULL,
		*which_p = (char *) NULL;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;

    ldb_cnt = 1;

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
    }

    if (i2_commit_b)
    {
	state_p = IITP_02_commit_p;
	which_p = IITP_15_committing_p;	/* must set up */
    }
    else
    {
	state_p = IITP_03_abort_p;
	which_p = IITP_14_aborting_p;	/* must set up */
    }
    if (i3_logstate_b)
    {
	/* 1.  logging the COMMIT/ABORT state must always succeed */

	status = tp2_c4_loop_to_log_dx_state(i2_commit_b, i1_dxcb_p, v_tpr_p);
	if (status)
	{
	    /* fatal error */

	    if (trace_err_104)
	    {
		STprintf(cbuf, 
		    "%s %p: %sFailed to log %s state of DX\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p,
		    state_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    return(status);
	}
	else
	{
	    /* state logged ok */

	    if (trace_dx_100)
	    {
		STprintf(cbuf, 
		    "%s %p: %s%s state of DX logged to %s\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p,
		    state_p,
		    IITP_21_2pc_cats_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	}
    }
    else
    {
	/* NOT required to log the state */

	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: %s%s state of DX NOT required to be logged\n",
		IITP_00_tpf_p,
		sscb_p,
		state_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }

    /* 2.  send COMMIT/ABORT message to all the updates */

    /* 
    ** if we are commiting a 1PC+ transaction, we must 
    ** commit the 1PC site first 
    */
    if (i1_dxcb_p->tdx_mode == DX_5MODE_1PC_PLUS && i2_commit_b)
    {
	/* find 1PC site */
	lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
	lx_cnt = 0;
	while (lxcb_p != (TPD_LX_CB *) NULL)
	{
	    if (lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	    {
		lxcb_p = lxcb_p->tlx_22_next_p;
		continue;
	    }
	    lx_cnt++;
	    if (lx_cnt > 1)
		/* more than 1 non 2PC capable site, this is an error */
		return( tpd_u2_err_internal(v_tpr_p) );
	    if (lxcb_p->tlx_state != LX_7STATE_ACTIVE)
		/* site not active, error */
		return( tpd_u2_err_internal(v_tpr_p) );
	    /* commit that site */
	    status = tp2_u2_msg_to_ldb(i1_dxcb_p, i2_commit_b, lxcb_p, v_tpr_p);
	    if (status == E_DB_OK)
	    {
		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
		/* unregister */
		
		lxcb_p->tlx_state = LX_5STATE_COMMITTED;
		which_p = IITP_15_committing_p;

		lxcb_p->tlx_write = FALSE;

		if (trace_dx_100)
		{
		    STprintf(cbuf, 
			     "%s %p: %s%s LDB %d)\n",
			     IITP_00_tpf_p,
			     sscb_p,
			     IITP_09_3dots_p,
			     which_p,
			     ldb_cnt);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
				       TPD_0TO_FE);
		    STprintf(cbuf, 
			     "%s %p: %s%s\n",
			     IITP_00_tpf_p,
			     sscb_p,
			     IITP_09_3dots_p,
			     IITP_19_succeeds_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
	    }
	    else
	    {
		/* here if error */
		if (trace_err_104)
		{	
		    STprintf(cbuf, 
			     "%s %p: %s%s LDB %d)\n",
			     IITP_00_tpf_p,
			     sscb_p,
			     IITP_09_3dots_p,
			     which_p,
			     ldb_cnt);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
				       TPD_0TO_FE);
		    STprintf(cbuf, 
			     "%s %p: %sfailed with ERROR %x\n",
			     IITP_00_tpf_p,
			     sscb_p,
			     IITP_09_3dots_p,
			     v_tpr_p->tpr_error.err_code);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		if (v_tpr_p->tpr_error.err_code == E_TP0545_CONNECTION_LOST)
		{
		    status = tp2_u1_restart_1_ldb(i1_dxcb_p, lxcb_p, v_tpr_p,
						  & result);
		    if (status)
		    {
			switch (result)
			{
			case TPD_3RESTART_LDB_UNKNOWN:
			case TPD_4RESTART_UNKNOWN_DX_ID:
			case TPD_5RESTART_NO_SUCH_LDB:
			    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
			    /* unregister */
			    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
			    lxcb_p->tlx_write = FALSE;
			    break;
			case TPD_6RESTART_LDB_NOT_OPEN:
			case TPD_2RESTART_NOT_DX_OWNER:
			default:
			    prev_sts = status;
			    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, 
						prev_err);
			    break;
			}
			if (trace_err_104)
			{	
			    STprintf(cbuf, 
				     "%s %p: ...Restarting\n",
				     IITP_00_tpf_p,
				     sscb_p);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
					       TPD_0TO_FE);
			    STprintf(cbuf, 
				     "%s %p: %sfailed with ERROR %x\n",
				     IITP_00_tpf_p,
				     sscb_p,
				     IITP_09_3dots_p,
				     v_tpr_p->tpr_error.err_code);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			}
		    }
		    else 
		    {
			/* here if OK */

			if (trace_dx_100)
			{
			    STprintf(cbuf, 
				     "%s %p: ...Restarting LDB %d)\n",
				     IITP_00_tpf_p,
				     sscb_p,
				     ldb_cnt);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    tpd_p9_prt_ldbdesc(v_tpr_p, 
					       & lxcb_p->tlx_site, 
					       TPD_0TO_FE);
			    STprintf(cbuf, 
				     "%s %p: %ssucceeds\n",
				     IITP_00_tpf_p,
				     sscb_p,
				     IITP_09_3dots_p);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			}
			/* Restart successfull, so resend message */

			status = tp2_u2_msg_to_ldb(i1_dxcb_p, i2_commit_b, 
						   lxcb_p, v_tpr_p);
			if (status == E_DB_OK)
			{
			    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
			    /* unregister */
			    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
			    which_p = IITP_15_committing_p;
			    lxcb_p->tlx_write = FALSE;

			    if (trace_dx_100)
			    {
				STprintf(cbuf, 
					 "%s %p: %s%s LDB %d)\n",
					 IITP_00_tpf_p,
					 sscb_p,
					 IITP_09_3dots_p,
					 which_p,
					 ldb_cnt);
				tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
				tpd_p9_prt_ldbdesc(v_tpr_p, 
						   & lxcb_p->tlx_site, 
						   TPD_0TO_FE);
				STprintf(cbuf, 
					 "%s %p: %s%s\n",
					 IITP_00_tpf_p,
					 sscb_p,
					 IITP_09_3dots_p,
					 IITP_19_succeeds_p);
				tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    }
			}
			else
			{
			        /* here if error, resend failed */
			    if (trace_err_104)
			    {	
				STprintf(cbuf, 
					 "%s %p: %s%s LDB %d)\n",
					 IITP_00_tpf_p,
					 sscb_p,
					 IITP_09_3dots_p,
					 which_p,
					 ldb_cnt);
				tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
				tpd_p9_prt_ldbdesc(v_tpr_p, 
						   & lxcb_p->tlx_site, 
						   TPD_0TO_FE);
				STprintf(cbuf, 
					 "%s %p: %sfailed with ERROR %x\n",
					 IITP_00_tpf_p,
					 sscb_p,
					 IITP_09_3dots_p,
					 v_tpr_p->tpr_error.err_code);
				tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    } 
			} /* Resend */
		    } /* Restart */
		} /* Connection lost */
		else
		{
		    /* here if error */

		    prev_sts = status;
		    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
		}
	    }
	    lxcb_p = lxcb_p->tlx_22_next_p; /* advance to next LX */
	} /* while lcxb_p */
    } /* if commiting 1PC+ */
    lx_cnt = 0;
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	displayed = FALSE;  /* must initialize */

	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG) /* a registered LX */
	{
	    if (! lxcb_p->tlx_write)		    /* must be an updater */
		return( tpd_u2_err_internal(v_tpr_p) );

	    switch (lxcb_p->tlx_state)
	    {
	    case LX_2STATE_POLLED:	/* polled but never responded */

		if (i2_commit_b)	/* error condition if not WILLING */
		    return( tpd_u2_err_internal(v_tpr_p) );
               
		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin POLLED state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		/* must restart according to the RQF-defined protocol */

		status = tp2_u1_restart_1_ldb(i1_dxcb_p, lxcb_p, v_tpr_p, 
			    & result);
		if (status)
		{
		    switch (result)
		    {
		    case TPD_3RESTART_LDB_UNKNOWN:
		    case TPD_4RESTART_UNKNOWN_DX_ID:
		    case TPD_5RESTART_NO_SUCH_LDB:
			lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
			if (i2_commit_b)
			    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
			else
			    lxcb_p->tlx_state = LX_4STATE_ABORTED;
			lxcb_p->tlx_write = FALSE;
			break;
		    case TPD_6RESTART_LDB_NOT_OPEN:
		    case TPD_2RESTART_NOT_DX_OWNER:
		    default:
			prev_sts = status;
			STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
			break;
		    }
		    if (trace_err_104)
		    {	
			STprintf(cbuf, 
			    "%s %p: ...Restarting\n",
				IITP_00_tpf_p,
				sscb_p);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
			    TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %p: ...failed with ERROR %p\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    v_tpr_p->tpr_error.err_code);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		}
		else
		{
		    status = tp2_u2_msg_to_ldb(i1_dxcb_p, i2_commit_b, 
					lxcb_p, v_tpr_p);
		    if (status)
		    {
			if (trace_err_104)
			{		
			    STprintf(cbuf, 
				"%s %p: %s%s LDB %d)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				which_p,
				ldb_cnt);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
				TPD_0TO_FE);
			    STprintf(cbuf, 
				"%s %p: %sfailed with ERROR %x)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				v_tpr_p->tpr_error.err_code);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			}
		    }
		    else
		    {
			/* closing OK */

			lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
			lxcb_p->tlx_state = LX_4STATE_ABORTED;
			lxcb_p->tlx_write = FALSE;
               
			if (trace_dx_100)
			{
			    STprintf(cbuf, 
				"%s %p: %s%s LDB %d)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
                                which_p,
				ldb_cnt);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
				TPD_0TO_FE);
			    STprintf(cbuf, 
				"%s %p: %s%s\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				IITP_19_succeeds_p);
			    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			}
		    }
 		}
		break;

	    case LX_3STATE_WILLING:	/* polled and acknowledged positive */
/*
		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin WILLING state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
*/
		/* send message */

		status = tp2_u2_msg_to_ldb(
			    i1_dxcb_p, i2_commit_b, lxcb_p, v_tpr_p);
		if (status == E_DB_OK)
		{
		    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
		    if (i2_commit_b)
		    {
			lxcb_p->tlx_state = LX_5STATE_COMMITTED;
			which_p = IITP_15_committing_p;
		    }
		    else
		    {
			lxcb_p->tlx_state = LX_4STATE_ABORTED;
			which_p = IITP_14_aborting_p;
		    }

		    lxcb_p->tlx_write = FALSE;

		    if (trace_dx_100)
		    {
			STprintf(cbuf, 
				"%s %p: %s%s LDB %d)\n",
				IITP_00_tpf_p,
				sscb_p,
				IITP_09_3dots_p,
				which_p,
				ldb_cnt);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
			    TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %p: %s%s\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p,
			    IITP_19_succeeds_p);
			tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		}
		else
		{
		    /* here if error */
		    if (trace_err_104)
		    {	
			STprintf(cbuf, 
			    "%s %p: %s%s LDB %d)\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p,
			    which_p,
			    ldb_cnt);
		        tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
			    TPD_0TO_FE);
			STprintf(cbuf, 
			    "%s %p: %sfailed with ERROR %x\n",
			    IITP_00_tpf_p,
			    sscb_p,
			    IITP_09_3dots_p,
			    v_tpr_p->tpr_error.err_code);
		        tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		    }
		    if (v_tpr_p->tpr_error.err_code == E_TP0545_CONNECTION_LOST)
		    {
			status = tp2_u1_restart_1_ldb(
					i1_dxcb_p, lxcb_p, v_tpr_p,
					& result);
			if (status)
			{
			    switch (result)
			    {
			    case TPD_3RESTART_LDB_UNKNOWN:
			    case TPD_4RESTART_UNKNOWN_DX_ID:
			    case TPD_5RESTART_NO_SUCH_LDB:
				lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
				if (i2_commit_b)
				    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
				else
				    lxcb_p->tlx_state = LX_4STATE_ABORTED;
				lxcb_p->tlx_write = FALSE;
				break;
			    case TPD_6RESTART_LDB_NOT_OPEN:
			    case TPD_2RESTART_NOT_DX_OWNER:
			    default:
				prev_sts = status;
				STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, 
				    prev_err);
				break;
			    }
			    if (trace_err_104)
			    {	
				STprintf(cbuf, 
				    "%s %p: ...Restarting\n",
				    IITP_00_tpf_p,
				    sscb_p);
		        	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
				tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, 
				    TPD_0TO_FE);
				STprintf(cbuf, 
				    "%s %p: %sfailed with ERROR %x\n",
				    IITP_00_tpf_p,
				    sscb_p,
				    IITP_09_3dots_p,
				    v_tpr_p->tpr_error.err_code);
		        	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    }
			}
			else 
			{
			    /* here if OK */

			    if (trace_dx_100)
			    {
				STprintf(cbuf, 
				    "%s %p: ...Restarting LDB %d)\n",
				    IITP_00_tpf_p,
				    sscb_p,
				    ldb_cnt);
		        	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
				tpd_p9_prt_ldbdesc(v_tpr_p, 
				    & lxcb_p->tlx_site, 
				    TPD_0TO_FE);
				STprintf(cbuf, 
				    "%s %p: %ssucceeds\n",
				    IITP_00_tpf_p,
				    sscb_p,
				    IITP_09_3dots_p);
		        	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    }
			    /* Restart successfull, so resend message */

			    status = tp2_u2_msg_to_ldb(
				    i1_dxcb_p, i2_commit_b, lxcb_p, v_tpr_p);
			    if (status == E_DB_OK)
			    {
			    	lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
						    /* unregister */
			    	if (i2_commit_b)
			    	{
				    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
				    which_p = IITP_15_committing_p;
			    	}
			    	else
			    	{
				    lxcb_p->tlx_state = LX_4STATE_ABORTED;
				    which_p = IITP_14_aborting_p;
			    	}

			    	lxcb_p->tlx_write = FALSE;

			    	if (trace_dx_100)
			    	{
					STprintf(cbuf, 
						"%s %p: %s%s LDB %d)\n",
						IITP_00_tpf_p,
						sscb_p,
						IITP_09_3dots_p,
						which_p,
						ldb_cnt);
					tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
					tpd_p9_prt_ldbdesc(v_tpr_p, 
							   & lxcb_p->tlx_site, 
							   TPD_0TO_FE);
					STprintf(cbuf, 
						 "%s %p: %s%s\n",
						 IITP_00_tpf_p,
						 sscb_p,
						 IITP_09_3dots_p,
						 IITP_19_succeeds_p);
					tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    	}
			    }
			    else
			    {
			        /* here if error, resend failed */
			        if (trace_err_104)
			        {	
				    STprintf(cbuf, 
						"%s %p: %s%s LDB %d)\n",
						IITP_00_tpf_p,
						sscb_p,
						IITP_09_3dots_p,
						which_p,
						ldb_cnt);
				    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
				    tpd_p9_prt_ldbdesc(v_tpr_p, 
						       & lxcb_p->tlx_site, 
						       TPD_0TO_FE);
				    STprintf(cbuf, 
				    	"%s %p: %sfailed with ERROR %x\n",
				    	IITP_00_tpf_p,
				    	sscb_p,
				    	IITP_09_3dots_p,
				    	v_tpr_p->tpr_error.err_code);
				    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
			    	} 
			    } /* Resend */
			} /* Restart */
		    } /* Connection lost */
		    else
		    {
			/* here if error */

		    	prev_sts = status;
			STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
		    }
		}
		break;

	    case LX_7STATE_ACTIVE:	/* not yet polled */

		if (i2_commit_b)	/* error condition if not aborting */
		    return( tpd_u2_err_internal(v_tpr_p) );

		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin ACTIVE state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		/* send abort message and reset to abort state */

		status = tp2_u2_msg_to_ldb(
			    i1_dxcb_p, i2_commit_b, lxcb_p, v_tpr_p);

		/* ignore error as for 1PC */

		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
		lxcb_p->tlx_state = LX_4STATE_ABORTED;

		lxcb_p->tlx_write = FALSE;
		break;

	    case LX_4STATE_ABORTED:

		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin ABORTED state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		return( tpd_u2_err_internal(v_tpr_p) );
		break;

	    case LX_5STATE_COMMITTED:

		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin COMMITTED state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		}
		status = tpd_u2_err_internal(v_tpr_p);
		goto EXIT_CLOSE;
		break;

	    case LX_8STATE_REFUSED:	

		if (trace_dx_100)
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin REFUSE state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		    displayed = TRUE;
		}
		/* fall thru */

	    case LX_1STATE_INITIAL:

		if (trace_dx_100 && (! displayed))
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin INITIAL state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		    displayed = TRUE;
		}
		/* fall thru */

	    case LX_6STATE_CLOSED:

		if (trace_dx_100 && (! displayed))
		{
		    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
		    STprintf(cbuf, 
			"%s %p: %sin CLOSED state\n",
			IITP_00_tpf_p,
			sscb_p,
			IITP_09_3dots_p);
		    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		    displayed = TRUE;
		}
		/* fall thru */

	    default:
		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister to forget */
		lxcb_p->tlx_write = FALSE;
		break;
	    } /* end switch */
	    ldb_cnt++;
	} /* end if */

	if (status)
	{
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	}
	++lx_cnt;
	if (lx_cnt > 1 || lxcb_p->tlx_22_next_p == (TPD_LX_CB *) NULL)
	{
	    switch (sscb_p->tss_4_dbg_2pc)
	    {
	    case TSS_73_CRASH_LX_COMMIT:
		if (i2_commit_b)
		{
		    status = tp2_d0_crash(v_tpr_p);
		    goto EXIT_CLOSE;
		}
		break;

	    case TSS_78_SUSPEND_LX_COMMIT:
		if (i2_commit_b)
		{
		    tp2_d1_suspend(v_tpr_p);
		}
		break;

	    case TSS_83_CRASH_LX_ABORT:
		if (! i2_commit_b)
		{
		    status = tp2_d0_crash(v_tpr_p);
		    goto EXIT_CLOSE;
		}
		break;

	    case TSS_88_SUSPEND_LX_ABORT:
		if (! i2_commit_b)
		{
		    tp2_d1_suspend(v_tpr_p);
		}
		break;

	    default:
		break;
	    }
	} /* end switch */
	lxcb_p = lxcb_p->tlx_22_next_p; /* advance to next LX */
    } /* end while */

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

EXIT_CLOSE:

    if (status)
    {
	if (trace_err_104)
	{	
	    STprintf(cbuf, 
		"%s %p: %sThis phase %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_18_fails_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else if (trace_dx_100)
    {
	STprintf(cbuf, 
	    "%s %p: %sAll LXs %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_23_committed_ok_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    return(status);
}


/*{
** Name: tp2_m6_p3_loop_to_close_lxs - loop until the DX is closed
**
** Description:
**	Loop until the DX can be closed.
**
**	    1.  Loop 20 times with no timer in between.
**	    2.  Loop 20 times with a 1-second timer.
**	    3.  Loop forever with a 5-second timer.
**
** Inputs:
**	v_dxcb_p			ptr to TPD_DX_CB
**	v_tpr_p				ptr to session context
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**					
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
**      oct-13-90 (carl)
**	    process trace points
*/


DB_STATUS
tp2_m6_p3_loop_to_close_lxs(
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    i4		cs_mask = CS_TIMEOUT_MASK,
		timer = 0,
		i = 0;
    char	*state_p = (char *) NULL;
    bool	commit_b,
		restart_b = TRUE,
                trace_err_104 = FALSE,
		trace_dx_100 = FALSE;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_END_DX_100,
            & i4_1, & i4_2))
    {
        trace_dx_100 = TRUE;
    }

    if (v_dxcb_p->tdx_state == DX_4STATE_COMMIT)
    {
	state_p = IITP_02_commit_p;
	commit_b = TRUE;
    }
    else if (v_dxcb_p->tdx_state == DX_3STATE_ABORT)
    {
	commit_b = FALSE;
	state_p = IITP_03_abort_p;
    }
    else
	return( tpd_u2_err_internal(v_tpr_p) );

    if (trace_dx_100)
    {
	STprintf(cbuf, 
	    "%s %p: ...1st %s broadcast loop of DX %x %x begins...\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    state_p,
	    v_dxcb_p->tdx_id.dd_dx_id1,
	    v_dxcb_p->tdx_id.dd_dx_id2);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    /* 1.  Loop 20 times with no timer in between */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: ...%dth iteration through 1st broadcast loop\n", 
		IITP_00_tpf_p,
		sscb_p,
		i);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    STprintf(cbuf, 
		"%s %p:    of DX %x %x\n", 
		IITP_00_tpf_p,
		sscb_p,
		v_dxcb_p->tdx_id.dd_dx_id1,
		v_dxcb_p->tdx_id.dd_dx_id2);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
	status = tp2_m7_restart_and_close_lxs(restart_b, v_dxcb_p, commit_b, 
	    v_tpr_p);
	if (status)
	{
	    if (trace_err_104)
	    {
		STprintf(cbuf, 
		    "%s %p: ...1st %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    fails\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	}
	else
	{
	    if (trace_dx_100)
	    {
		STprintf(cbuf, 
		    "%s %p: ...1st %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    ends successfully\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    return(E_DB_OK);
	}
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    STprintf(cbuf, 
	"%s %p: ...1st %s broadcast loop of DX %x %x failed\n", 
	IITP_00_tpf_p,
	sscb_p,
	state_p,
	v_dxcb_p->tdx_id.dd_dx_id1,
	v_dxcb_p->tdx_id.dd_dx_id2);
    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

    prev_sts = E_DB_OK;	   /* reset for the next loop */

    if (trace_dx_100)
    {
	STprintf(cbuf, 
	    "%s %p: ...2nd %s broadcast loop of DX %x %x begins...\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    state_p,
	    v_dxcb_p->tdx_id.dd_dx_id1,
	    v_dxcb_p->tdx_id.dd_dx_id2);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
	    
    /* 2.  Loop 20 times with a 1-second timer */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: ...%dth iteration through 2nd broadcast loop\n", 
		IITP_00_tpf_p,
		sscb_p,
		i);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    STprintf(cbuf, "%s %p:    of DX %x %x\n", 
		IITP_00_tpf_p,
		sscb_p,
		v_dxcb_p->tdx_id.dd_dx_id1,
		v_dxcb_p->tdx_id.dd_dx_id2);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
	/* do suspend for 1 second and then resume */

	timer = 1;
	
	status = CSsuspend(cs_mask, timer, (PTR) NULL);
		/* suspend 1 second */

	status = tp2_m7_restart_and_close_lxs(restart_b, v_dxcb_p, commit_b, 
		    v_tpr_p);
	if (status)
	{
	    if (trace_err_104)
	    {
		STprintf(cbuf, 
		    "%s %p: ...2nd %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    fails\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	}
	else
	{
	    if (trace_dx_100)
	    {
		STprintf(cbuf, 
		    "%s %p: ...2nd %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    ends successfully\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    return(E_DB_OK);
	}
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    STprintf(cbuf, 
	"%s %p: ...2nd %s broadcast loop of DX %x %x failed\n", 
	IITP_00_tpf_p,
	sscb_p,
	state_p,
	v_dxcb_p->tdx_id.dd_dx_id1,
	v_dxcb_p->tdx_id.dd_dx_id2);
    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

    /* 3.  Loop forever with a 5-second timer (termination condtition:
	   prev_sts == E_DB_OK */

    if (trace_dx_100)
    {
	STprintf(cbuf, 
	    "%s %p: ...3rd %s broadcast loop of DX %x %x begins...\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    state_p,
	    v_dxcb_p->tdx_id.dd_dx_id1,
	    v_dxcb_p->tdx_id.dd_dx_id2);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
	    
    for (i = 0; prev_sts != E_DB_OK; i++)
    {
	if (trace_dx_100)
	{
	    STprintf(cbuf, 
		"%s %p: ...%dth iteration through 3rd broadcast loop\n", 
		IITP_00_tpf_p,
		sscb_p,
		i);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    STprintf(cbuf, 
		"%s %p:    DX %x %x\n", 
		IITP_00_tpf_p,
		sscb_p,
		v_dxcb_p->tdx_id.dd_dx_id1,
		v_dxcb_p->tdx_id.dd_dx_id2);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
        prev_sts = E_DB_OK;	   /* preset termination condition */

	/* do suspend for 5 seconds and then resume */

	timer = 5;

	status = CSsuspend(cs_mask, timer, (PTR) NULL);	
		/* suspend 5 seconds */

	status = tp2_m7_restart_and_close_lxs(restart_b, v_dxcb_p, commit_b, 
		    v_tpr_p);
	if (status)
	{
	    if (trace_err_104)
	    {
		STprintf(cbuf, 
		    "%s %p: ...3rd %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    fails\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	}
	else
	{
	    if (trace_dx_100)
	    {
		STprintf(cbuf, 
		    "%s %p: ...3rd %s broadcast loop of DX %x %x\n", 
		    IITP_00_tpf_p,
		    sscb_p,
		    state_p,
		    v_dxcb_p->tdx_id.dd_dx_id1,
		    v_dxcb_p->tdx_id.dd_dx_id2);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
		STprintf(cbuf, 
		    "%s %p:    ends successfully\n", 
		    IITP_00_tpf_p,
		    sscb_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	    }
	    return(E_DB_OK);
	}
    }

    return(E_DB_OK);
}


/*{
** Name: tp2_m7_restart_and_close_lxs - close remaining LXs of given DX
**
** Description:
**	Commit/abort remaining LXs by restarting the remaining LDBs 
**
** Inputs:
**	i1_restart_b			TRUE if restart is necessary
**	v_dxcb_p			ptr to TPD_DX_CB
**	v_tpr_p				ptr to session context
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**					
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
**	feb-11-91 (fpang)
**	    Check for tracepoints before advancing lxcb_p pointer.
**	    Will get a SEGV if only 1 lxcb in list.
**	    Also added processing of TSS_76, and TSS_86.
**	19-oct-92 (teresa)
**	    tp2_d1_suspend() is a void, so stop attempting to assign the
**	    return value to variable status.
*/


DB_STATUS
tp2_m7_restart_and_close_lxs(
	bool		i1_restart_b,
	TPD_DX_CB	*v_dxcb_p,
	bool		i2_commit_b,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & v_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    i4		lx_cnt,
		dropout_cnt;	/* number of terminated LDBs that will 
				** not restart */
    bool	giveup_b = FALSE;


    if (i1_restart_b)
    {
	status = tp2_u8_restart_all_ldbs(FALSE, v_dxcb_p, v_tpr_p, 
		    & dropout_cnt, & giveup_b);
	if (status)
	    return(status);

	if (giveup_b)
	    return(E_DB_OK);
    }

    /* 2.  send COMMIT/ABORT message to all the updates */

    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    lx_cnt = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG) /* a registered LX */
	{
	    if (! lxcb_p->tlx_write)		    /* must be an updater */
		return( tpd_u2_err_internal(v_tpr_p) );

	    switch (lxcb_p->tlx_state)
	    {
	    case LX_2STATE_POLLED:	/* polled but never responded */

		if (i2_commit_b)	/* error condition if not WILLING */
		    return( tpd_u2_err_internal(v_tpr_p) );
		
		/* fall thru */

	    case LX_3STATE_WILLING:	/* polled and acknowledged positive */
	    case LX_7STATE_ACTIVE:	/* not yet polled */

		status = tp2_u2_msg_to_ldb(v_dxcb_p, i2_commit_b, 
			    lxcb_p, v_tpr_p);
		if (status == E_DB_OK)
		{
		    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
		    if (i2_commit_b)
			lxcb_p->tlx_state = LX_5STATE_COMMITTED;
		    else
			lxcb_p->tlx_state = LX_4STATE_ABORTED;
		    lxcb_p->tlx_write = FALSE;
		}
		else
		{
		    prev_sts = status;
		    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
		}
		break;

	    case LX_4STATE_ABORTED:
	    case LX_5STATE_COMMITTED:

		return( tpd_u2_err_internal(v_tpr_p) );
		break;

	    case LX_8STATE_REFUSED:	
	    case LX_1STATE_INITIAL:
	    case LX_6STATE_CLOSED:

	    default:
		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister to forget */
		lxcb_p->tlx_write = FALSE;
		break;
	    } /* end switch */
	} /* end if */
	++lx_cnt;

	if (lx_cnt > 1 || lxcb_p->tlx_22_next_p == (TPD_LX_CB *) NULL)
	{
	    if (sscb_p->tss_4_dbg_2pc == TSS_71_CRASH_IN_COMMIT)
	    {
		if (i2_commit_b)		/* commit ? */
		{
		    status = tp2_d0_crash(v_tpr_p);
		    return(status);
		}
	    }
	    if (sscb_p->tss_4_dbg_2pc == TSS_76_SUSPEND_IN_COMMIT)
	    {
		if (i2_commit_b)		/* commit ? */
		{
		    tp2_d1_suspend(v_tpr_p);
		}
	    }
	    if (sscb_p->tss_4_dbg_2pc == TSS_81_CRASH_IN_ABORT)
	    {
		if (! i2_commit_b)		/* abort ? */
		{
		    status = tp2_d0_crash(v_tpr_p);
		    return(status);
		}
	    }
	    if (sscb_p->tss_4_dbg_2pc == TSS_86_SUSPEND_IN_ABORT)
	    {
		if (! i2_commit_b)		/* abort ? */
		{
		    tp2_d1_suspend(v_tpr_p);
		}
	    }
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
    } /* end while */


    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }
    return(status);
}

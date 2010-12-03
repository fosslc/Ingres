/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ddb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulx.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <sxf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psyrereg.h>
#include    <psydint.h>
#include    <psykview.h>
#include    <psyaudit.h>
/**
**
**  Name: PSFCALL.C - Entry points to the parser.
**
**  Description:
**      This file contains the entry points to the control-block interfaces 
**      in the parser.
**
**          psq_call - Call a query-related or general purpose operation.
**          psy_call - Call a qrymod operation.
**
**
**  History:
**      11-oct-85 (jeff)    
**          written
**	22-apr-87 (puree)
**	    add PSQ_COMMIT function call.
**	21-dec-87 (stec)
**	    Define a separate exception handler for PSF server-wide operations.
**	23-mar-88 (stec)
**	    Implement RETRY for `psy' calls.
**	27-apr-88 (stec)
**	    Make changes for DB PROCs.
**	02-aug-88 (stec)
**	    Correct if (psq_cb->psq_mode ...) for PSQ_RECREATE case.
**	17-aug-88 (stec)
**	    Update a comment.
**	17-aug-88 (stec)
**	    Fixed bad forward declarations of psf_ses_handle().
**	18-aug-88 (stec)
**	    Added missing EXdelete as requested by Mikem.
**	23-aug-88 (stec)
**	    pss_ruser -> pss_rusr.
**	25-aug-88 (stec)
**	    Report errors on execution of nonexistent DB procedures.
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem.
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added calls for PSY_GROUP and PSY_APLID.
**	20-mar-89 (neil)
**	    Rules - added dispatching of PSY_DROOL and PSY_CRUEL.
**	24-apr-89 (neil)
**	    Rules/cursor - added PSQ_DELCURS.
**	10-may-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added call for PSY_DBPRIV
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added call for PSY_MXQUERY
**	04-sep-89 (ralph)
**	    For B1: Added calls for PSY_USER, PSY_LOC, PSY_AUDIT, PSY_ALARM
**	11-oct-89 (ralph)
**	    Reset the session's group/role id's during recovery
**	25-oct-89 (neil)
**	    Alerters - added dispatching of PSY_DEVENT and PSY_KEVENT.
**	08-aug-90 (ralph)
**	    Change transaction semantics for PSY_MXQUERY in psy_call.
**      11-sep-90 (teresa)
**          changed psq_force, pss_ruset, pss_rgset and pss_raset to be bitmasks
**	    instead of booleans.  Also fixed faulty definition/use of pss_retry
**	08-jan-91 (andre)
**	    psf_ses_handle() will report system/hardware errors besides SEGVIO;
**	    all errors not recognized by adx_handler() will be flagged as
**	    internal errors (E_PS0002);
**	    psf_svr_handle() will report system/hardware errors besides SEGVIO.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	24-mar-92 (barbara)
**	    Sybil merge.  Added PSY_REREGISTER opcode for Star.
**	03-aug-92 (barbara)
**	    During qrymod we no longer flush the entire RDF cache.  Instead
**	    call psy_invalidate which invalidates individual objects, depending
**	    on the statement mode.  Also for PSF and SCF retry, the RDF cache
**	    is no longer flushed.  Instead the PSS_REFRESH_CACHE bit is set
**	    which will cause RDF to refresh its cached items.
**	25-nov-1992 (markg)
**	    Modified psy_call to not begin/end a transaction for the PSY_AUDIT
**	    operation, as this is handled by SXF.
**	03-dec-92 (tam)
**	    Added call to psq_dbpreg in PSQ_RECREATE to support registered
**	    dbproc execution in star.
**	22-dec-92 (rblumer)
**	    clean up pss_tchain2 in exception handler (just like pss_tchain).
**	06-apr-93 (anitap)
**	    Do not abort the transaction for PSQ_GRANT if error for CREATE 
**	    SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	26-jul-1993 (rog)
**	    General exception handler cleanup.
**	10-aug-93 (andre)
**	    removed FUNC_EXTERN for scf_call()
**	17-aug-1993 (rog)
**	    Have psf_ses_handler()() call adx_handler() when working on user data.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	02-nov-93 (anitap)
**	    A fix has been put in qeu_atran() so that the dsh is not destroyed.
**	    So remove check for PSY_SCHEMA_MODE.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qeu_owner which has changed type to PTR.
**	4-mar-94 (robf)
**          Add PSY_RPRIV request for role privileges
**	6-jul-95 (stephenb)
**	    Add PSQ_RESOLVE opcode.
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	04-apr-97 (nanpr01)
**	    Fix dbevent intermittent hang problem.
**	21-May-1997 (shero03)
**	    Added function tracing diagnostic
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS. Supply scf_session to SCF when it's known.
**	24-jul-2003 (stial01) 
**          changes for prepared inserts with blobs (b110620
**      15-sep-2003 (stial01) 
**          Blob optimization for prepared updates (b110923)
**      01-dec-2004 (stial01)
**	    Blob optimization for updates only if PSQ_REPCURS (b112427, b113501)
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structures.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psq_call(
	i4 opcode,
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);
bool psf_retry(
	PSS_SESBLK *cb,
	PSQ_CB *psq_cb,
	i4 ret_val);
bool psf_in_retry(
	PSS_SESBLK *cb,
	PSQ_CB *psq_cb);
i4 psy_call(
	i4 opcode,
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
static i4 psf_ses_handle(
	EX_ARGS *exargs);
static i4 psf_2handle(
	EX_ARGS *exargs);
static i4 psf_svr_handle(
	EX_ARGS *exargs);

/*{
** Name: psq_call	- Call a query-related or general-purpose operation.
**
** Description:
**      This function is the entry point to the parser for all query-related 
**      or general-purpose operations.
**
** Inputs:
**      opcode                          Code representing the operation to be
**					performed.
**      psq_cb                          Pointer to the control block containing
**					the parameters.  Which fields are used
**					depends on the operation.
**	sess_cb				Pointer to the session control block.
**					(Can be NULL)
**
** Outputs:
**	psq_cb
**	    .psq_error			Error information
**		.err_code		    Filled in with an error number if
**					    an error occurs, E_PS0000_OK
**					    otherwise
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    None
**
** Side Effects:
**	    Depends on operation.
**
** History:
**	11-oct-85 (jeff)
**          written
**      2-sep-86 (seputis)
**          changed some (PSQ_CB *) to (PSY_CB *)
**      10-sep-86 (seputis)
**          defined the SCF_CB_TYPE
**	22-apr-87 (puree)
**	    add PSQ_COMMIT function call.
**	07-oct-87 (stec)
**	    Removed references to HELP related routines.
**	21-dec-87 (stec)
**	    Define a separate exception handler for PSF server-wide operations.
**	02-aug-88 (stec)
**	    Correct if (psq_cb->psq_mode ...) for PSQ_RECREATE case.
**	17-aug-88 (stec)
**	    Update a comment related to reparsing a query within PSF.
**	17-aug-88 (stec)
**	    Fixed bad forward declarations of psf_ses_handle().
**	18-aug-88 (stec)
**	    Added missing EXdelete as requested by Mikem.
**	25-aug-88 (stec)
**	    Report errors on execution of nonexistent DB procedures.
**	24-apr-89 (neil)
**	    Rules/cursor - added PSQ_DELCURS.
**	20-nov-89 (andre)
**	    if just successfully parsed CREATE/DEFINE INDEX or MODIFY,
**	    invalidate the cache entry for the table.
**	06-aug-90 (andre)
**	    The above (6/11/90) fix works OK unless we try to EXECUTE a
**	    PREPAREd statement, in which case sess_cb->pss_resrng is NULL.
**      11-sep-90 (teresa)
**          changed psq_force, pss_ruset, pss_rgset and pss_raset to be bitmasks
**	    instead of booleans.  Also fixed faulty definition/use of pss_retry
**	6-nov-90 (andre)
**	    replaced pss_flag with pss_stmt_flag and pss_dbp_flag.  The idea is
**	    that a dbproc may consist of more than one statement, and so it
**	    would be desirable to have a field which does not get reset for
**	    every statement.  On the other hand, there is a number of flags
**	    which do have to be reset for every query other than a dbproc and
**	    for every statement inside a dbproc.  Accordingly, pss_stmt_flag
**	    must be set to 0L before calling psq_parseqry(); after every
**	    statement which is a part of a dbproc, all of its bits except for
**	    PSS_TXTEMIT must be also reset.  pss_dbp_flag must be set to 0L
**	    before calling psq_parseqry() from psq_call(), and to PSS_RECREATE
**	    when calling psq_recreate() from psq_call().  psq_recreate() should
**	    set pss_stmt_flag to 0L, but pss_dbp_flag should not be reset as it
**	    may be carrying useful info.
**	20-may-91 (andre)
**	    Add code to handle PSQ_RESET_EFF_USER_ID, PSQ_RESET_EFF_ROLE_ID, and
**	    PSQ_RESET_EFF_GROUP_ID.
**	03-aug-92 (barbara)
**	    If a user error is encountered when parsing a query, we no longer
**	    flush the entire RDF cache; instead set the REFRESH bit.  Set
**	    REFRESH bit when in SCF retry, too.  Note that when in SCF retry
**	    SCF no longer calls PSF with PSQ_DELRNG and that opcode has been
**	    eliminated from the case statement.
**	28-sep-92 (andre)
**	    if an error occurred during processing of a dbproc,
**	    pss_dependencies_stream must be closed regardless of whether we are
**	    going to retry and, besides, when processing dbprocs, we will always
**	    require up-to-date cache entries so no retry will be necessary
**	02-dec-92 (andre)
**	    remove code to handle PSQ_RESET_EFF_GROUP_ID and
**	    PSQ_RESET_EFF_ROLE_ID since we are desupporting SET GROUP/ROLE
**	03-dec-92 (tam)
**	    Added call to psq_dbpreg in PSQ_RECREATE to support registered
**	    dbproc execution in star.
**	01-mar-93 (barbara)
**	    Initialize pss_distr_sflags before parsing (new statement flags
**	    field for distrib).
**	01-jul-1993 (rog)
**	    psf_exc_handle should return STATUS, not nat.
**	    If EXdeclare returns !OK, we should report E_PS0002, not E_PS0004.
**	    The exception handler can take care of reporting E_PS0004.
**	08-sep-93 (swm)
**	    Changed sizeof(DB_SESSID) to sizeof(CS_SID) to reflect recent CL
**	    interface revision.
**	15-nov-93 (andre)
**	    add code to initialize a newly added PSS_SESBLK.pss_flattening_flags
**	15-mar-94 (andre)
**	    call a newly defined function - psf_retry() - to determine whether
**	    we should retry parsing a query.
**	17-mar-94 (andre)
**	    added code to handle a new opcode - PSQ_OPEN_REP_CURS
**	6-jul-94 (stephenb)
**	    Add PSQ_RESOLVE opcode.
**	5-oct-95 (thaju02)
**	    Added PSQ_SESSOWN for obtaining temp table's session owner.
**     27-oct-98 (stial01)
**          Added PSQ_ULMCONCHK
**     27-Jul-2009 (coomi01) b122237
**          Add wrapper for pst_resolve() call, named as the PSQ_OPRESLV op.
**     November 2009 (stephenb)
**     	    Batch processing: set flags around parsequery; add PSQ_COPYBUF.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type call to use the proper struct pointer.
**	12-Mar-2010 (thaju02) Bug 123440
**	    Add PSQ_GET_SESS_INFO to return session cache_dynamic setting.
**	29-apr-2010 (stephenb)
**	    Batch flags are now on psq_flag2.
**	04-may-2010 (miket) SIR 122403
**	    Init new sess_cb->pss_stmt_flags2.
**	18-jun-2010 (stephenb)
**	    clear pss_last_sname on commit in case we are in cleanup mode
**	    after terminating a copy optimization (Bug 123939)
*/
DB_STATUS
psq_call(
	i4                opcode,
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		ret_val;
    bool		copy_in_progress = FALSE;
    EX_CONTEXT		context;
    i4		err_code;
    STATUS		(*psf_exc_handle)();
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_PSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_PSF_ID, opcode, psq_cb);
#endif

    /*
    ** Handle all exceptions here the same way: internal DB_ERROR.
    ** Pointer to the exception handler is initialized depending on
    ** whether PSF session exists or not.
    */
    switch (opcode)
    {
	case PSQ_ULMCONCHK:
	    {
		extern PSF_SERVBLK  *Psf_srvblk;
		RDF_CCB             rdf_ccb;
		rdf_ccb.rdf_fcb = Psf_srvblk->psf_rdfid;
		rdf_call(RDF_ULM_TRACE, (PTR) &rdf_ccb);
		return(E_DB_OK);
	    }

	case PSQ_STARTUP:
	case PSQ_SRVDUMP:
	case PSQ_SHUTDOWN:
	case PSQ_PRMDUMP:
	    {
		psf_exc_handle = psf_svr_handle;
		break;
	    }
	default:
	    {
		psf_exc_handle = psf_ses_handle;
		break;
	    }
    }

    if (EXdeclare(psf_exc_handle, &context) != OK)
    {
	(VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR, &err_code,
	    &psq_cb->psq_error, 0);
	EXdelete();
	return (E_DB_SEVERE);
    }

    /*
    ** If the caller has given a PSQ_CB, clear the error; also clear the
    ** psq_ret_flag field
    */
    if (psq_cb != (PSQ_CB *) NULL)
    {
	CLRDBERR(&psq_cb->psq_error);
	psq_cb->psq_ret_flag = 0L;
    }

    /*
    ** If the operation requires a session control block, and the caller
    ** has not supplied it, get it from SCF.
    */
    if (sess_cb == (PSS_SESBLK *) NULL && 
	opcode != PSQ_STARTUP &&
	opcode != PSQ_SRVDUMP && 
	opcode != PSQ_SHUTDOWN &&
	opcode != PSQ_PRMDUMP)
    {
	/* Get session block */
	sess_cb = psf_sesscb();
    }
    if (sess_cb	&&
	opcode != PSQ_STARTUP &&
	opcode != PSQ_SRVDUMP && 
	opcode != PSQ_SHUTDOWN &&
	opcode != PSQ_PRMDUMP
       )
    {
	sess_cb->pss_retry = PSS_REPORT_MSGS;
    }

    switch (opcode)
    {

    case PSQ_ALTER:
	ret_val = psq_alter(psq_cb, sess_cb);
	break;

    case PSQ_BGN_SESSION:
	ret_val = psq_bgn_session(psq_cb, sess_cb);
	break;

    case PSQ_CLSCURS:
	ret_val = psq_clscurs(psq_cb, sess_cb);
	break;

    case PSQ_DELCURS:
	ret_val = psq_dlcsrrow(psq_cb, sess_cb);
	break;

    case PSQ_COMMIT:
	/* cleanup last_sname in case we are terminating a copy optimization */
	sess_cb->pss_last_sname[0] = EOS;
	ret_val = pst_commit_dsql(psq_cb, sess_cb);
	break;

    case PSQ_CURDUMP:
	ret_val = psq_crdump(psq_cb, sess_cb);
	break;

    case PSQ_END_SESSION:
	ret_val = psq_end_session(psq_cb, sess_cb);
	break;

    case PSQ_PARSEQRY:
	/* start out with all bits turned off */
	sess_cb->pss_stmt_flags = sess_cb->pss_stmt_flags2 =
	    sess_cb->pss_dbp_flags = 0L;
	sess_cb->pss_flattening_flags = 0;
	if (psq_cb->psq_ret_flag & PSQ_FINISH_COPY)
	    sess_cb->pss_last_sname[0] = EOS;
	psq_cb->psq_ret_flag &= ~(PSQ_INSERT_TO_COPY | PSQ_CONTINUE_COPY | PSQ_FINISH_COPY);

	/* Initialize distributed statement flags */
	sess_cb->pss_distr_sflags = PSS_PRINT_WITH;

	/* 
	** If SCF tells PSF to retry the query, set the REFRESH_CACHE
	** flag so that access to data descriptions will not rely on
	** RDF cached (and potentially stale) information.  Also set
	** REPORT_MSGS because we need not be concerned that errors
	** are due to stale cache entries.  If this is not retry, i.e.,
	** the first attempt at parsing the statement, set SUPPRESS_MSGS.
	** In this case any user error during parsing (e.g. column does
	** not exist) will go unreported.  After parsing, test for
	** errors (ERR_ENCOUNTERED) and reparse with the REFRESH_CACHE and
	** REPORT_MSGS flags set.  If the error was due to stale cached
	** information it will not recur.  Note that previous to 6.5 the
	** whole cache was cleared before retry.
	*/ 
	if (psq_cb->psq_flag & PSQ_REDO)
	{
	    sess_cb->pss_retry = (PSS_REPORT_MSGS | PSS_REFRESH_CACHE);
	}
	else
	{
	    sess_cb->pss_retry = PSS_SUPPRESS_MSGS;
	}
	if (sess_cb->pss_last_sname[0] != EOS)
	    copy_in_progress = TRUE;
	
	ret_val = psq_parseqry(psq_cb, sess_cb);
	
	if (copy_in_progress && 
		((psq_cb->psq_ret_flag & PSQ_CONTINUE_COPY) == 0 || 
			psq_cb->psq_flag2 & PSQ_LAST_BATCH))
	{
	    psq_cb->psq_ret_flag |= PSQ_FINISH_COPY;
	    sess_cb->pss_last_sname[0] = EOS;
	    psq_cb->psq_flag2 &= ~(PSQ_BATCH | PSQ_LAST_BATCH);
	}

	/*
	** if we encountered an error while processing CREATE PROCEDURE,
	** cb->pss_dependencies_stream could have been opened to collect
	** information about objects/privileges on which the new dbproc will
	** depend - now would be the right time to close it
	*/
	if (   DB_FAILURE_MACRO(ret_val)
	    && sess_cb->pss_dbp_flags & PSS_DBPROC
	    && sess_cb->pss_dependencies_stream
	    && sess_cb->pss_dependencies_stream->psf_mstream.qso_handle !=
	           (PTR) NULL)
	{
	    DB_STATUS	    stat;

	    stat = psf_mclose(sess_cb, sess_cb->pss_dependencies_stream, &psq_cb->psq_error);

	    /*
	    ** ensure that no one tries to use the address that is no longer
	    ** valid
	    */
	    sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;

	    if (DB_FAILURE_MACRO(stat) && stat > ret_val)
		ret_val = stat;
	}

	if (!psf_retry(sess_cb, psq_cb, ret_val))
	{
	    break;
	}

	/* PSF retry */
	/* start out with all bits turned off */
	sess_cb->pss_stmt_flags = sess_cb->pss_stmt_flags2 =
	    sess_cb->pss_dbp_flags = 0L;
	sess_cb->pss_flattening_flags = 0;

	/* Initialize distributed statement flags */
	sess_cb->pss_distr_sflags = PSS_PRINT_WITH;

	/* reset all bits in psq_ret_flag as well */
	psq_cb->psq_ret_flag = 0L;

	sess_cb->pss_retry = (PSS_REPORT_MSGS | PSS_REFRESH_CACHE);
	ret_val = psq_parseqry(psq_cb, sess_cb);
	    
	break;
    case PSQ_PRMDUMP:
	ret_val = psq_prmdump(psq_cb);
	break;

    case PSQ_SESDUMP:
	ret_val = psq_sesdump(psq_cb, sess_cb);
	break;

    case PSQ_SHUTDOWN:
	ret_val = psq_shutdown(psq_cb);
	break;

    case PSQ_SRVDUMP:
	ret_val = psq_srvdump(psq_cb);
	break;

    case PSQ_STARTUP:
	ret_val = psq_startup(psq_cb);
	break;

    case PSQ_RECREATE:
	/* If executing registered procedure in star, call psq_dbpreg */
	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    ret_val = psq_dbpreg(psq_cb, sess_cb);
	    break;
	}
	/*
	** If SCF tells us we are in "redo" mode, set PSS_REFRESH_CACHE
	** bit which will have the effect of refreshing any cached RDF
	** objects (because these objects might be stale).  PSS_REPORT_ERRORS
	** prevents PSF from initiating its own retry in the event of a user
	** error.
	*/
	sess_cb->pss_retry = (PSS_REFRESH_CACHE | PSS_REPORT_MSGS);
	sess_cb->pss_dbp_flags = PSS_RECREATE;

	ret_val = psq_recreate(psq_cb, sess_cb);
	break;

    case PSQ_RESET_EFF_USER_ID:
	STRUCT_ASSIGN_MACRO(psq_cb->psq_user.db_tab_own, sess_cb->pss_user);
	ret_val = E_DB_OK;
	break;

    case PSQ_OPEN_REP_CURS:
	ret_val = psq_open_rep_cursor(psq_cb, sess_cb);
	break;

    case PSQ_OPRESLV:
    {
	i4        undoWork;
	i4        oldLock;
	QSF_RCB   *qsfcb = (QSF_RCB *)psq_cb->psq_qsfcb;

	/*
	** A flag to note entry to following block
	*/
	undoWork = 0;

	/* 
	** Caution : If pst_resolve() uses psf_malloc() to create memory, 
	**           it will need a stream in which to do it. Eventually
	**           qso_palloc() will perform the equivalent of the 
	**           following test.
	*/
	if ( NULL == sess_cb->pss_ostream.psf_mstream.qso_handle )
	{
	    /*
	    ** If this test fails, we will get a E_QS000F_BAD_HANDLE error message.
	    ** - So we fixup for that condition
	    */

	    /*
	    ** Note we have performed this adjustment
	    */
	    undoWork = 1;

	    /* 
	    ** Save some old detail
	    ** - With no object, we expect this to be 0
	    **   Don't test, just save it.
	    */
	    oldLock = sess_cb->pss_ostream.psf_mlock;

	    /*
	    ** Assign the stream details, including the lock
	    */
	    sess_cb->pss_ostream.psf_mstream.qso_handle = qsfcb->qsf_obj_id.qso_handle;
	    sess_cb->pss_ostream.psf_mlock = qsfcb->qsf_lk_id;
	}

	/* 
	** Go ahead and call
	*/
	ret_val = pst_resolve(
	    sess_cb,
	    (ADF_CB *)psq_cb->psq_adfcb,
	    (PST_QNODE *)psq_cb->psq_qnode,
	    psq_cb->psq_qlang,
	    &psq_cb->psq_error);

	if ( undoWork )
	{
	    /* 
	    ** Restore details
	    */
	    sess_cb->pss_ostream.psf_mstream.qso_handle = NULL;
	    sess_cb->pss_ostream.psf_mlock = oldLock;
	}

	break;
    }

    case PSQ_RESOLVE:
    {
	STATUS		local_stat;
	DB_TEXT_STRING	*out, *obj_owner, *obj_name;
	PTR		base;
	/*
	** grab memory for strings
	*/
	base = MEreqmem((u_i4)0, 
	    (u_i4)((3 * sizeof(DB_TEXT_STRING)) + (4 * DB_GW1_MAXNAME)),
	    TRUE, &local_stat);
	if (local_stat != OK)
	{
	    ret_val = E_DB_ERROR;
	    (VOID)MEfree(base);
	    break;
	}
	out = (DB_TEXT_STRING *)base;
	obj_owner = (DB_TEXT_STRING * )
	    ME_ALIGN_MACRO(base + sizeof(DB_TEXT_STRING)  - 1 + 
	    (2 * DB_GW1_MAXNAME), sizeof(i4));
	obj_name = (DB_TEXT_STRING *)
	    ME_ALIGN_MACRO(obj_owner + sizeof(DB_TEXT_STRING) - 1 + 
	    DB_GW1_MAXNAME, sizeof(i4));
	/*
	** overload psq_user with table owner (if provided)
	*/
	MEcopy(psq_cb->psq_user.db_tab_own.db_own_name, 
	    sizeof(psq_cb->psq_user.db_tab_own.db_own_name), 
	    obj_owner->db_t_text);
	obj_owner->db_t_count = STnlength(DB_GW1_MAXNAME,
					    (char *)obj_owner->db_t_text);
	MEcopy(psq_cb->psq_tabname.db_tab_name, 
	    sizeof(psq_cb->psq_tabname.db_tab_name), obj_name->db_t_text);
	obj_name->db_t_count = STnlength(DB_GW1_MAXNAME, 
					    (char *)obj_name->db_t_text);
	ret_val = pst_resolve_table(obj_owner, obj_name, out);
	if (ret_val == E_DB_OK && out->db_t_count == 0)
	    ret_val = E_DB_WARN;
	local_stat = MEfree(base);
	break;
    }

    case PSQ_SESSOWN:
    {
	/* overload psq_owner with temporary table session owner */
	psq_cb->psq_owner = (char *)&sess_cb->pss_sess_owner.db_own_name;
	ret_val = E_DB_OK;
	break;
    }

    case PSQ_GET_STMT_INFO:
    {
	PSQ_STMT_INFO	*stmt_info = NULL;

	psq_cb->psq_stmt_info = NULL;
	psq_cb->psq_ret_flag = pst_get_stmt_mode(sess_cb, psq_cb->psq_qid, 
			&stmt_info);
	if ((psq_cb->psq_ret_flag == PSQ_APPEND ||
		psq_cb->psq_ret_flag == PSQ_REPCURS) && stmt_info)
	{
	    psq_cb->psq_stmt_info = stmt_info;
	}
	ret_val = E_DB_OK;
	break;
    }
    
    case PSQ_COPYBUF:
    {
	if (psq_cb->psq_ret_flag & PSQ_FINISH_COPY)
	    sess_cb->pss_last_sname[0] = EOS;
	psq_cb->psq_ret_flag &= ~(PSQ_INSERT_TO_COPY | PSQ_CONTINUE_COPY | PSQ_FINISH_COPY);

	ret_val = pst_cpdata(sess_cb, psq_cb, NULL, TRUE);
	
	if (psq_cb->psq_flag2 & PSQ_LAST_BATCH)
	{
	    psq_cb->psq_ret_flag |= PSQ_FINISH_COPY;
	    sess_cb->pss_last_sname[0] = EOS;
	    psq_cb->psq_flag2 &= ~(PSQ_BATCH | PSQ_LAST_BATCH);
	}

	break;
    }
	    
    case PSQ_GET_SESS_INFO:
    {
	if (sess_cb->pss_ses_flag & PSS_CACHEDYN)
	    psq_cb->psq_ret_flag = PSQ_SESS_CACHEDYN;
	else if (sess_cb->pss_ses_flag & PSS_NO_CACHEDYN)
	    psq_cb->psq_ret_flag = PSQ_SESS_NOCACHEDYN;	
	ret_val = E_DB_OK;
	break;
    }

    default:
	ret_val = E_DB_SEVERE;
	if (psq_cb != (PSQ_CB *) NULL)
	    SETDBERR(&psq_cb->psq_error, 0, E_PS0006_UNKNOWN_OPERATION);
	break;
    }

    EXdelete();
#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_PSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_PSF_ID, opcode, psq_cb);
#endif
    return    (ret_val);
}

/*
** Name: psf_retry - determine whether the current query shoudl be retried
**
** Description:
**	This function should be used to determine whether the current query 
**	should be retried.  The test is very simple: check whether 
**	PSS_ERR_ENCOUNTERED is set in cb->pss_retry.  This replaces a more 
**	complicated test which used to be in psq_call 
**	    (   !(psq_cb->psq_flag & PSQ_REDO)
**	     && !(cb->pss_dbp_flags & PSS_DBPROC)
**	     && !(   (   ret_val == E_DB_OK
**	              || psq_cb->psq_error.err_code != E_PS0001_USER_ERROR)
**	          && ~cb->pss_retry & PSS_ERR_ENCOUNTERED))
**	and a somewhat simple-minded and not always equivalent test in 
**	psq_cbinit()
**	    (   (sess_cb->pss_retry & PSS_REPORT_MSGS) == 0 &&
**	     && psq_cb->psq_error.err_code == E_PS0001_USER_ERROR)
**
**	The two tests were not quite equivalent because it is possible for 
**	PSS_ERR_ENCOUNTERED to be set in cb->pss_retry without 
**	psq_cb->psq_error.err_code being set to E_PS0001_USER_ERROR (this 
**	happened when we were processing a definition of a UNION view which a 
**	user tried to create WITH CHECK OPTION because err_code would get 
**	subsequently reset to 0.)  Furthermore, the first test relied on its 
**	position (i.e. after the first attempt to psq_parseqry()) to know that 
**	we may need to retry.  
** 
**	In order for a separate function to effectively determine whether we 
**	should retry a query, I will merge the above 2 tests.  (I almost 
**	simplified the whole thing to 
**	    (cb->pss_retry & PSS_ERR_ENCOUNTERED), 
**	but then it turned out that in some cases rather than set 
**	PSS_REPORT_MSGS in cb->pss_retry we are setting PSQ_REDO in 
**	psq_cb->psq_flag, meaning that, I cannot replace 
**	    !(psq_cb->psq_flag & PSQ_REDO) && !(cb->pss_dbp_flags & PSS_DBPROC)
**	with
**	    !(sess_cb->pss_retry & PSS_REPORT_MSGS)
**	so I gave it up.)
**
** Input:
**	cb		PSF session CB
**	psq_cb		PSF request CB
**	ret_val		return status from parsing the query
**
** Output:
**	none
**
** Returns:
**	TRUE if we should retry, FALSE otherwise
**
** History:
**	16-mar-94 (andre)
**	    written
**	9-oct-2008 (dougi) Bug 120998
**	    No retry for failed cached dynamic queries.
*/
bool
psf_retry(
	  PSS_SESBLK	*cb,
	  PSQ_CB	*psq_cb,
	  DB_STATUS	ret_val)
{
    return(   ~cb->pss_retry & PSS_REPORT_MSGS
	   && ~psq_cb->psq_flag & PSQ_REDO
	   && !(cb->pss_retry & PSS_CACHEDYN_NORETRY)
	   && ~cb->pss_dbp_flags & PSS_DBPROC
	   && (   (   ret_val != E_DB_OK
		   && psq_cb->psq_error.err_code == E_PS0001_USER_ERROR)
		|| cb->pss_retry & PSS_ERR_ENCOUNTERED));
}

/*
** Name: psf_in_retry	- determine whether we are retrying a query
**
** Description:
**	This function will be used to determine whether are are retrying a query
**	after the first attempt to parse it failed for some reason.
**
**	It relies on the fact that having PSS_REPORT_MSGS set in cb->pss_retry
**	means that this is the last attempt at a query and it would be set when
**	retrying a query unless we are reparsing a dbproc definition or we were 
**	told by SCF to redo a query.
**
** Input
**      cb              PSF session CB
**      psq_cb          PSF request CB
**
** Output:
**      none
**
** Returns:
**      TRUE if we are in "true" retry, FALSE otherwise
**
** History:
**      16-mar-94 (andre)
**          written
*/
bool
psf_in_retry(
	     PSS_SESBLK		*cb,
	     PSQ_CB		*psq_cb)
{
    /*
    ** we are in true retry if PSS_REPORT_MSGS is set in cb->pss_retry and 
    ** neither PSQ_REDO is set in psq_cb->psq_flag nor PSS_RECREATE is set in
    ** cb->pss_dbp_flags
    */
    return(   cb->pss_retry & PSS_REPORT_MSGS
	   && ~psq_cb->psq_flag & PSQ_REDO
	   && ~cb->pss_dbp_flags & PSS_RECREATE);
}

/*{
** Name: psy_call	- Call a qrymod operation.
**
** Description:
**      This function is the entry point to the parser for all qrymod
**      operations.
**
** Inputs:
**      opcode                          Code representing the operation to be
**					performed.
**      psy_cb                          Pointer to the control block containing
**					the parameters.  Which fields are used
**					depends on the operation.
**	sess_cb				Pointer to the session control block
**
** Outputs:
**	psy_cb
**	    .psy_error			Error information
**		.err_code		    Filled in with error number if error
**					    occurs, E_PS0000_OK otherwise.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    None
**
** Side Effects:
**	    Depends on operation.
**
** History:
**	11-oct-85 (jeff)
**          written
**	09-sep-1986 (fred)
**	    Added EXdelete call to prevent SCF from having its stack smashed.
**	14-oct-87 (stec)
**	    Added transaction handling.
**	23-mar-88 (stec)
**	    Implement RETRY feature. This part of PSF should behave as QEF, i.e.,
**	    operations need to be retried in cases when it is necessary. This
**	    is caused by the fact that the RDF cache in PSF sometimes is out
**	    of sync with reality. In that case reexecution of the query, after
**	    the cache gets invalidated by SCF, should reveal the fact that
**	    objects specified in the query have changed or ceased to exist.
**	27-apr-88 (stec)
**	    Make changes for DB PROCs.
**	17-aug-88 (stec)
**	    Fixed bad forward declarations of psf_ses_handle().
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem (0038 -> 38).
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added calls for PSY_GROUP and PSY_APLID.
**	20-mar-89 (neil)
**	    Rules - added dispatching of PSY_DROOL and PSY_CRUEL.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added call for PSY_DBPRIV
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added call for PSY_MXQUERY
**	04-sep-89 (ralph)
**	    For B1: Added calls for PSY_USER, PSY_LOC, PSY_AUDIT, ALARM.
**	25-oct-89 (neil)
**	    Alerters - added dispatching of PSY_DEVENT and PSY_KEVENT.
**	21-feb-90 (andre)
**	    handle PSY_COMMENT
**	19-apr-90 (andre)
**	    handle PSY_CSYNONYM, PSY_DSYNONYM
**	08-aug-90 (ralph)
**	    Change transaction semantics for PSY_MXQUERY in psy_call.
**      11-sep-90 (teresa)
**          changed psq_force, pss_ruset, pss_rgset and pss_raset to be 
**	    bitmasks instead of booleans.  Also fixed faulty pss_retry logic.
**	23-jan-91 (andre)
**	    Set QEU_CB.qeu_mask to 0.
**	24-mar-92 (barbara)
**	    Sybil merge.  Added PSY_REREGISTER opcode for Star and
**	    inforporated Star comments:
**	    04-apr-89 (andre)
**		Added call to handle PSY_REREGISTER.
**	    11-jul-89 (andre)
**		For PSY_KVIEW, rather than invalidating the entire RDF cache,
**		we will call psq_delrng() once to clear the range tables and
**		invalidate the first entry, and then will repeatedly call 
**		psq_invalidate() to invalidate RDF cache entries for the rest 
**		of the tables (if any).
**	    End of merged comments.
**	02-aug-92 (andre)
**	    if opcode==PSY_GRANT, call psy_dgrant() which will no longer be
**	    called through psy_dpermit()
**
**	    if opcode==PSY_REVOKE, call psy_revoke()
**	03-aug-92 (barbara)
**	    Instead of invalidating the entire RDF cache after DDL, the
**	    various qrymod modules now invalidate individual items.  All
**	    that is done here is to clear the range table and unfix any
**	    objects that may have been requested during DDL.
**	28-sep-92 (andre)
**	    before processing CREATE PERMIT and GRANT, set PSS_REFRESH_CACHE in
**	    sess_cb->pss_retry to ensure that we get up-tp-date RDF cache info
**	25-nov-1992 (markg)
**	    Modified psy_call to not begin/end a transaction for the PSY_AUDIT
**	    operation, as this is handled by SXF.
**	22-dec-92 (andre)
**	    removed references to PSY_DVIEW which was undefined; psy_dview() is
**	    also going away
**	19-feb-93 (andre)
**	    added code to handle PSY_DSCHEMA for DROP SCHEMA processing
**	09-mar-93 (andre)
**	    successful cleanup would overwrite the error status in ret_val
**	    because the return value from the second call to pst_clrrng() was
**	    being assigned to ret_val instead of status
**	06-mar-93 (anitap)
**	    Do not abort the transaction if error in PSQ_GRANT in CREATE SCMA.
**	    We do not want to destroy the dsh.
**	01-jul-1993 (rog)
**	    If EXdeclare returns !OK, we should report E_PS0002, not E_PS0004.
**	    The exception handler can take care of reporting E_PS0004.
**	02-nov-93 (anitap)
**	    Go ahead and abort transaction if error in PSQ_GRANT in CREATE 
**	    SCHEMA. qeu_atran() now takes care of not destroying the dsh and
**	    so no AV occurs.
**	04-apr-97 (nanpr01)
**	    Fix dbevent intermittent hang problem.
**	7-mar-02 (inkdo01)
**	    Added support for create/alter/drop sequence operations.
*/
DB_STATUS
psy_call(
	i4             opcode,
	PSY_CB          *psy_cb,
	PSS_SESBLK	*sess_cb)
{
    DB_STATUS   ret_val;
    DB_STATUS   status;
    EX_CONTEXT	context;
    i4	err_code;
    PSQ_CB	psq_cb;
    QEU_CB	qeu_cb;
    DB_ERROR	db_error;
    DB_STATUS	(*psy_entry)();	    /* Routine to process the request */
    bool	xact_needed = TRUE;

#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_PSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_PSF_ID, opcode, psy_cb);
#endif

    /* Required by psf_error */
    sess_cb->pss_retry = PSS_REPORT_MSGS;

    CLRDBERR(&psy_cb->psy_error);
    CLRDBERR(&db_error);

    /*
    ** Handle all exceptions here the same way: internal DB_ERROR.
    */
    if (EXdeclare(psf_ses_handle, &context) != OK)
    {
	(VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR, &err_code,
	    &psy_cb->psy_error, 0);
	EXdelete();
	return (E_DB_SEVERE);
    }

    /* Check validity of arguments */
    if ((psy_cb == (PSY_CB *) NULL) || (sess_cb == (PSS_SESBLK *) NULL))
    {
	/* Should never be reached */
	EXdelete();
	return (E_DB_SEVERE);
    }

    status = ret_val = E_DB_OK;

    /* 
    ** Prepare to begin a transaction -- 
    ** but defer beginning it until we examine the opcode.
    **
    ** This is done because some opcodes may want to change
    ** the transaction semantics.  The default is to start
    ** an update MST.  This is changed to a read only
    ** transaction for PSY_MXQUERY (for example).
    */

    qeu_cb.qeu_length	= sizeof(qeu_cb);
    qeu_cb.qeu_type	= QEUCB_CB;
    qeu_cb.qeu_owner	= (PTR)DB_PSF_ID;
    qeu_cb.qeu_ascii_id = QEUCB_ASCII_ID;
    qeu_cb.qeu_d_id	= sess_cb->pss_sessid;
    qeu_cb.qeu_db_id	= sess_cb->pss_dbid;
    qeu_cb.qeu_flag	= 0;
    qeu_cb.qeu_mask	= 0;
    qeu_cb.qeu_eflag	= QEF_INTERNAL;
    qeu_cb.qeu_qmode	= opcode;

    /* Find the PSY entry to process the opcode */
    
    switch (opcode)
    {
	case PSY_DINTEG:
	    psy_entry = psy_dinteg;
	    break;

	case PSY_DPERMIT:
	    sess_cb->pss_retry |= PSS_REFRESH_CACHE;
	    psy_entry = psy_dpermit;
	    break;

	case PSY_KINTEG:
            psy_entry = psy_kinteg;
	    break;

	case PSY_KPERMIT:
            psy_entry = psy_kpermit;
	    break;

	case PSY_KVIEW:
            psy_entry = psy_kview;
	    break;

	case PSY_CREDBP:
            psy_entry = psy_cproc;
	    break;

	case PSY_DRODBP:
            psy_entry = psy_kproc;
	    break;

	case PSY_GROUP:
            psy_entry = psy_group;
	    break;

	case PSY_APLID:
            psy_entry = psy_aplid;
	    break;

	case PSY_DRULE:
            psy_entry = psy_drule;
	    break;

	case PSY_KRULE:
            psy_entry = psy_krule;
	    break;

	case PSY_DBPRIV:
            psy_entry = psy_dbpriv;
	    break;

	case PSY_RPRIV:
            psy_entry = psy_rolepriv;
	    break;

	case PSY_MXQUERY:
	    qeu_cb.qeu_flag = QEF_READONLY;		/* Don't start  MST */
            psy_entry = psy_maxquery;
	    break;

	case PSY_USER:
            psy_entry = psy_user;
	    break;

	case PSY_LOC:
            psy_entry = psy_loc;
	    break;

	case PSY_AUDIT:
	    xact_needed = FALSE;
            psy_entry = psy_audit;
	    break;

	case PSY_ALARM:
            psy_entry = psy_alarm;
	    break;

	case PSY_DEVENT:
            psy_entry = psy_devent;
	    break;

	case PSY_KEVENT:
            psy_entry = psy_kevent;
	    break;

	case PSY_COMMENT:
            psy_entry = psy_comment;
	    break;

	case PSY_CSYNONYM:
	    psy_entry = psy_create_synonym;
	    break;

	case PSY_DSYNONYM:
	    psy_entry = psy_drop_synonym;
	    break;

	case PSY_GRANT:
	    sess_cb->pss_retry |= PSS_REFRESH_CACHE;
	    psy_entry = psy_dgrant;
	    break;

	case PSY_REVOKE:
	    psy_entry = psy_revoke;
	    break;

	case PSY_REREGISTER:
	    psy_entry = psy_reregister;
	    break;

	case PSY_DSCHEMA:
	    psy_entry = psy_dschema;
	    break;

	case PSY_CSEQUENCE:
            psy_entry = psy_csequence;
	    break;

	case PSY_ASEQUENCE:
            psy_entry = psy_asequence;
	    break;

	case PSY_DSEQUENCE:
            psy_entry = psy_dsequence;
	    break;

	default:
            psy_entry = NULL;
	    ret_val = E_DB_SEVERE;
	    SETDBERR(&psy_cb->psy_error, 0, E_PS0006_UNKNOWN_OPERATION);
	    goto exit;		/* bad opcode; only need to report an error */
    }

    /* Begin transaction */
    if (xact_needed)
	status = qef_call(QEU_BTRAN, ( PTR ) &qeu_cb);
    else
	status = E_DB_OK;

    if (DB_FAILURE_MACRO(status))
    {
	switch (qeu_cb.error.err_code)
	{
	    /* QEF internal */
	    case E_QE0017_BAD_CB:
	    case E_QE0018_BAD_PARAM_IN_CB:
	    case E_QE0019_NON_INTERNAL_FAILURE:
	    case E_QE0002_INTERNAL_ERROR:
	    {
		(VOID) psf_error(E_PS0D22_QEF_ERROR, qeu_cb.error.err_code,
		    PSF_INTERR, &err_code, &db_error, 0);
		break;
	    }
	    /* interrupt */
	    case E_QE0022_QUERY_ABORTED:
	    {
		(VOID) psf_error(E_PS0003_INTERRUPTED, 0L, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* resource related */
	    case E_QE000D_NO_MEMORY_LEFT:
	    case E_QE000E_ACTIVE_COUNT_EXCEEDED:
	    case E_QE000F_OUT_OF_OTHER_RESOURCES:
	    case E_QE001E_NO_MEM:
	    {
		(VOID) psf_error(E_PS0D23_QEF_ERROR, qeu_cb.error.err_code,
		    PSF_USERERR, &err_code, &db_error, 0);
		break;
	    }
	    /* resource quota */
	    case E_QE0052_RESOURCE_QUOTA_EXCEED:
	    case E_QE0067_DB_OPEN_QUOTA_EXCEEDED:
	    case E_QE0068_DB_QUOTA_EXCEEDED:
	    case E_QE006B_SESSION_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4707L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    case E_QE0086_ERROR_BEGINNING_TRAN:
	    {
		(VOID) psf_error(E_PS0D24_BGN_TX_ERR, qeu_cb.error.err_code,
		    PSF_USERERR, &err_code, &db_error, 0);
		break;
	    }
	    /* log full */
	    case E_QE0024_TRANSACTION_ABORTED:
	    {
		(VOID) psf_error(4706L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* deadlock */
	    case E_QE002A_DEADLOCK:
	    {
		(VOID) psf_error(4700L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* lock quota */
	    case E_QE0034_LOCK_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4705L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* inconsistent database */
	    case E_QE0099_DB_INCONSISTENT:
	    {
		(VOID) psf_error(38L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    case E_QE0025_USER_ERROR:
	    {
		SETDBERR(&db_error, 0, E_PS0001_USER_ERROR);
		break;
	    }
	    default:
	    {
		(VOID) psf_error(E_PS0D22_QEF_ERROR, qeu_cb.error.err_code,
		    PSF_INTERR, &err_code, &db_error, 0);
		break;
	    }
	}

	goto exit;
    }
    
    /* Call the PSY processing routine */
    ret_val = (*psy_entry)(psy_cb, sess_cb, &qeu_cb);

    /* End the transaction */
    if (xact_needed)
    {
	if (DB_SUCCESS_MACRO(ret_val))
	{
	    /* Commit */
	    status = qef_call(QEU_ETRAN, ( PTR ) &qeu_cb);
	}
	else
	{
	    /* Abort */
	    status = qef_call(QEU_ATRAN, ( PTR ) &qeu_cb);
	}
    }
    else
    {
	status = E_DB_OK;
    }

    if (DB_FAILURE_MACRO(status))
    {
	switch (qeu_cb.error.err_code)
	{
	    /* no transaction; should not happen unless aborted by the BE */
	    case E_QE0004_NO_TRANSACTION:
	    {
		SETDBERR(&db_error, 0, E_PS0001_USER_ERROR);
		break;
	    }
	    /* interrupt */
	    case E_QE0022_QUERY_ABORTED:
	    {
		(VOID) psf_error(E_PS0003_INTERRUPTED, 0L, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* resource related */
	    case E_QE000D_NO_MEMORY_LEFT:
	    case E_QE000E_ACTIVE_COUNT_EXCEEDED:
	    case E_QE000F_OUT_OF_OTHER_RESOURCES:
	    case E_QE001E_NO_MEM:
	    {
		(VOID) psf_error(E_PS0D23_QEF_ERROR, qeu_cb.error.err_code,
		    PSF_USERERR, &err_code, &db_error, 0);
		break;
	    }
	    /* lock timer */
	    case E_QE0035_LOCK_RESOURCE_BUSY:
	    case E_QE0036_LOCK_TIMER_EXPIRED:
	    {
		(VOID) psf_error(4702L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* resource quota */
	    case E_QE0052_RESOURCE_QUOTA_EXCEED:
	    case E_QE0067_DB_OPEN_QUOTA_EXCEEDED:
	    case E_QE0068_DB_QUOTA_EXCEEDED:
	    case E_QE006B_SESSION_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4707L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    case E_QE0087_ERROR_COMMITING_TRAN:
	    {
		(VOID) psf_error(E_PS0D25_CMT_TX_ERR, qeu_cb.error.err_code,
		    PSF_USERERR, &err_code, &db_error, 0);
		break;
	    }
	    case E_QE0088_ERROR_ABORTING_TRAN:
	    {
		(VOID) psf_error(E_PS0D26_ABT_TX_ERR, qeu_cb.error.err_code,
		    PSF_USERERR, &err_code, &db_error, 0);
		break;
	    }
	    /* log full */
	    case E_QE0024_TRANSACTION_ABORTED:
	    {
		(VOID) psf_error(4706L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* deadlock */
	    case E_QE002A_DEADLOCK:
	    {
		(VOID) psf_error(4700L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* lock quota */
	    case E_QE0034_LOCK_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4705L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    /* inconsistent database */
	    case E_QE0099_DB_INCONSISTENT:
	    {
		(VOID) psf_error(38L, qeu_cb.error.err_code, PSF_USERERR,
		    &err_code, &db_error, 0);
		break;
	    }
	    case E_QE0025_USER_ERROR:
	    {
		SETDBERR(&db_error, 0, E_PS0001_USER_ERROR);
		break;
	    }
	    default:
	    {
		(VOID) psf_error(E_PS0D22_QEF_ERROR, qeu_cb.error.err_code,
		    PSF_INTERR, &err_code, &db_error, 0);
		break;
	    }
	}
    }

exit:

    if (status > ret_val)
    {
	ret_val = status;
	STRUCT_ASSIGN_MACRO(db_error, psy_cb->psy_error);
    }

    /* Clear range tables and unfix objects */
    status = pst_clrrng(sess_cb, &sess_cb->pss_usrrange,
				&psq_cb.psq_error);
    if (status > ret_val)
    {
	ret_val = status;
    }
    status = pst_clrrng(sess_cb, &sess_cb->pss_auxrng, &psq_cb.psq_error);
    
    if (status > ret_val)
    {
	ret_val = status;
    }

    EXdelete();

#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_PSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_PSF_ID, opcode, psy_cb);
#endif
return (ret_val);
}

/*{
** Name: psf_ses_handle	- Exception handler for psf_call or psq_call
**			  when PSF session exists.
**
** Description:
**      This function is the exception handler for psq_call.  All exceptions
**	in PSF cause session shut down.
**
**	Unfortunately, we can't tell the caller what exception has happened,
**	in the general case.  This is because we can't count on there being
**	a session control block, since the session (or server) might not be
**	started yet.  Therefore, we will only report the fact that an exception
**	has occurred.  In the future, we might do something more informative.
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
** Returns:
**	EXDECLARE
** Exceptions:
**	None
**
** Side Effects:
**	    Can send error message to terminal and/or log file
**
** History:
**	09-jun-86 (jeff)
**          written
**	17-dec-86 (daved)
**	    handle access violations and cleanup RDF.
**	23-aug-88 (stec)
**	    pss_ruser -> pss_rusr.
**	23-may-89 (jrb)
**	    Changed ule_format calls for new i/f.
**	11-oct-89 (ralph)
**	    Reset the session's group/role id's during recovery
**      11-sep-90 (teresa)
**          changed psq_force, pss_retry, pss_ruset, pss_rgset and
**          pss_raset to be bitmasks instead of flags.
**	08-jan-91 (andre)
**	    initialize ret_val; check for system error only if adx_handler()
**	    refuses to deal with it; use EXsys_report() to determine if
**	    this was, indeed, a system error - comparing to EXSEGVIO misses
**	    other system errors (e.g. bus error)
**	20-feb-91 (andre)
**	    we no longer need to restore effective group/role identifiers since
**	    they will no longer be reset as a part of dbproc recreation.
**	    This was done as a part of fix for bug # 32457
**	30-sep-91 (andre)
**	    if we cannot (EX)continue and are closing the memory streams used in
**	    the query and sess_cb->pss_dependencies_stream does not point to
**	    sess_cb->pss_ostream, we must explicitly close it here
**	15-jun-92 (barbara)
**	    Sybil merge.  Pass in session control block to pst_clrrng().
**	24-oct-92 (andre)
**	    in both ADF_ERROR and SCF_CB generic error code has been replaced
**	    wth SQLSTATE + ule_format() expects a (DB_SQLSTATE *) instead of
**	    (i4 *)
**	22-dec-92 (rblumer)
**	    clean up after pss_tchain2 just like pss_tchain.
**	01-jul-93 (rog)
**	    Call ulx_exception() to report the exception.  Delete calls to
**	    adx_handler(): PSF should not be modifying it's behavior on math
**	    exceptions depending on the user's setting of how math exceptions
**	    should be handled since PSF does not do any calculations on behalf
**	    of user data.
**	17-aug-1993 (rog)
**	    It turns out that PSF actually does do some calculations involving
**	    user data, so a flag was added to tell when this was occuring so
**	    that psf_ses_handler() can call adx_handler() when working on user
**	    data.
*/
static STATUS
psf_ses_handle(
	EX_ARGS            *exargs)
{
    EX_CONTEXT		context;
    PSS_SESBLK		*sess_cb;
    DB_ERROR		err_blk;
    STATUS		ret_val = OK;

    /*
    ** Handle all exceptions here the same way: internal DB_ERROR.
    */
    if (EXdeclare(psf_2handle, &context) == OK)
    {
	sess_cb = psf_sesscb();

	/* Were we handling user data when we got the exception? */
	if (sess_cb->pss_stmt_flags & PSS_CALL_ADF_EXCEPTION
	    && sess_cb->pss_adfcb != NULL)
	{
	    ADF_CB      *adf_cb = (ADF_CB *) sess_cb->pss_adfcb;
	    i4	error;
	    DB_STATUS   status;
 
	    /* Call ADF to see whether this is a math exception in user data. */
	    status = adx_handler(adf_cb, exargs);
	    error  = adf_cb->adf_errcb.ad_errcode;
	    if (status == E_DB_OK || error == E_AD0001_EX_IGN_CONT
		|| error == E_AD0115_EX_WRN_CONT)  /* handle warning messages */
	    {
		ret_val = EXCONTINUES;
	    }
	    /* handle other errors adf knows about */
	    else if (adf_cb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		SCF_CB          scf_cb;
 
		scf_cb.scf_facility = DB_PSF_ID;
		scf_cb.scf_session = sess_cb->pss_sessid;
		scf_cb.scf_length = sizeof(SCF_CB);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_ptr_union.scf_buffer = adf_cb->adf_errcb.ad_errmsgp;
		scf_cb.scf_len_union.scf_blength = adf_cb->adf_errcb.ad_emsglen;
		scf_cb.scf_nbr_union.scf_local_error =
						adf_cb->adf_errcb.ad_usererr;
		STRUCT_ASSIGN_MACRO(adf_cb->adf_errcb.ad_sqlstate,
					    scf_cb.scf_aux_union.scf_sqlstate);
		(VOID) scf_call(SCC_ERROR, &scf_cb);

		ret_val = EXDECLARE;   /* abort execution */
	    }
	}
	
	/*
	** If the above code didn't set ret_val, then we need to fall 
	** through to the following code.
	*/
	if (ret_val == OK)
	{
	    /* Report the exception */
	    ulx_exception(exargs, DB_PSF_ID, E_PS0004_EXCEPTION, TRUE);
	    ret_val = EXDECLARE;
	}

	/* Do some cleanup if we can't continue */
	if (ret_val != EXCONTINUES)
	{
	    /* Restore the original user if necessary */
	    if (sess_cb->pss_dbp_flags & PSS_RUSET)
	    {
		STRUCT_ASSIGN_MACRO(sess_cb->pss_rusr, sess_cb->pss_user);
		sess_cb->pss_dbp_flags &= ~PSS_RUSET;
	    }

	    if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_ostream, &err_blk);
		sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
	    }
		
	    if (sess_cb->pss_tstream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_tstream, &err_blk);
		sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
	    }
	    
	    if (sess_cb->pss_cbstream.psf_mstream.qso_handle != (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, &sess_cb->pss_cbstream, &err_blk);
		sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;
	    }

	    if (   sess_cb->pss_dependencies_stream
		&& sess_cb->pss_dependencies_stream->psf_mstream.qso_handle !=
		    (PTR) NULL)
	    {
		(VOID) psf_mclose(sess_cb, sess_cb->pss_dependencies_stream, &err_blk);

		/*
		** ensure that no one tries to use the address that is no longer
		** valid
		*/
		sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;
	    }

	    /*
	    ** If the statement was a "define cursor" or a "define repeat cursor",
	    ** we have to deallocate the cursor control block, if it was allocated.
	    */
	    if (sess_cb->pss_cstream != (PTR) NULL)
	    {
		psq_delcursor(sess_cb->pss_crsr, &sess_cb->pss_curstab,
		    &sess_cb->pss_memleft, &err_blk);
	    }

	    /*
	    ** If the statement has emitted query text, close the stream.
	    */
	    if (sess_cb->pss_tchain != (PTR) NULL)
	    {
		psq_tclose(sess_cb->pss_tchain, &err_blk);
		sess_cb->pss_tchain = (PTR) NULL;
	    }
	    if (sess_cb->pss_tchain2 != (PTR) NULL)
	    {
		psq_tclose(sess_cb->pss_tchain2, &err_blk);
		sess_cb->pss_tchain2 = (PTR) NULL;
	    }

	    /*
	    ** Clear out the user's range table.
	    */
	    pst_clrrng(sess_cb, &sess_cb->pss_usrrange, &err_blk);

	    /*
	    ** Clear out the auxliary range table.
	    */
	    pst_clrrng(sess_cb, &sess_cb->pss_auxrng, &err_blk);
	}
    }

    EXdelete();
    return(ret_val);
}

/*{
** Name: psf_2handle - called if exception caught during exception processing
**
** Description:
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
** Returns:
**	EXDECLARE
** Exceptions:
**	None
**
** Side Effects:
**	    Can send error message to terminal and/or log file
**
** History:
**	17-dec-86 (daved)
**	    handle access violations and cleanup RDF.
*/
static STATUS
psf_2handle(
	EX_ARGS            *exargs)
{
    return (EXDECLARE);
}

/*{
** Name: psf_svr_handle	- Exception handler used when there is no session.
**
** Description:
**      This function is the exception handler for psq_call.
**
**	Unfortunately, we can't tell the caller what exception has happened,
**	in the general case.  This is because we can't count on there being
**	a session control block, since the session (or server) might not be
**	started yet.  Therefore, we will only report the fact that an exception
**	has occurred.  In the future, we might do something more informative.
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
**
** Returns:
**	    EXDECLARE
**
** Exceptions:
**	    none
**
** Side Effects:
**	    Can send error message to terminal and/or log file
**
** History:
**	21-dec-87 (stec)
**          written
**	23-may-89 (jrb)
**	    Changed ule_format calls for new interface.
**	08-jan-91 (andre)
**	    check for other system/hardware errors (in addition to SEGVIO)
**	30-jun-1993 (rog)
**	    Modified to call ulx_exception() to handle the reporting of AV's.
*/
static STATUS
psf_svr_handle(
	EX_ARGS            *exargs)
{
    EX_CONTEXT		context;

    /*
    ** Handle all exceptions here the same way: internal DB_ERROR.
    */
    if (EXdeclare(psf_2handle, &context) == OK)
    {
	/* Report the exception */
	ulx_exception(exargs, DB_PSF_ID, E_PS0004_EXCEPTION, TRUE);
    }

    EXdelete();
    return(EXDECLARE);
}

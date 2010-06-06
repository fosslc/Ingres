/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <ex.h>
#include    <st.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulx.h>
#include    <qsf.h>
#include    <qsfint.h>

/**
**
**  Name: QSFCALL.C - Provide standard entry point to QSF
**
**  Description:
**      This file contains the entry point level services of QSF.
**
**          qsf_call - Main entry point to the Query Storage Facility
**
**
**  History:
**      28-apr-86 (thurston)
**          Initial creation.
**	02-sep-86 (thurston)
**	    Fixed bug that could have locked up the server.  If an unexpected
**	    exception occurred while QSF had its server control block locked
**	    with a semaphore, it would never get unlocked.  This has now been
**	    fixed by having the exception handler make sure the semaphore has
**	    been released.
**	02-mar-87 (thurston)
**	    GLOBALDEF changes.
**	18-mar-87 (stec)
**	    Added case QSO_CHTYPE to the switch stmnt.
**	13-may-87 (thurston)
**	    Added some trace output stuff which are currently xDEBUG'ed and
**	    should become legitimate trace points.
**	05-jul-87 (thurston)
**	    Added code to qsf_call() to check for server wide tracing before
**	    even attempting to look for a particular trace flag.
**	25-nov-87 (thurston)
**	    Fixed up the calls to ule_format() so that arguments are compatible
**	    with what is expected.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	23-jan-88 (eric)
**	    Added support for the QSO_TRANS_OR_DEFINE operation.
**	24-may-88 (eric)
**	    Added support for the QSO_JUST_TRANS operation.
**	21-may-89 (jrb)
**	    Changed ule_format calls to conform to new interface.
**	07-jun-90 (andre)
**	    Added support for QSO_CRTALIAS operation.
**	25-jul-91 (rog)
**	    Fixed the a_vio_msg buffer in qsf_handler to use a macro instead of
**	    a number.
**	13-nov-91 (rog)
**	    Integrated 6.4 changes into 6.5.
**	09-jan-1992 (fred)
**	    Added support for QSO_ASSOCIATE operation (for blob support).
**	    Also added appropriate strings to debug string for operations >=
**	    QSO_TRANS_OR_DEFINE, since they were missing.
**	28-aug-1992 (rog)
**	    Prototype QSF.
**	10-sep-1992 (bryanp)
**	    Repair "if (error)" test following integration problems.
**	19-oct-1992 (rog)
**	    Make sure exception handler only releases the QSF semaphore if
**	    the offending session was the one holding it.
**	20-oct-1992 (rog)
**	    Add new calls: QSO_GETNEXT and QSO_GET_OR_CREATE.
**	20-oct-1992 (rog)
**	    Added support for the QSO_DESTROYALL opcode.
**	29-jun-1993 (rog)
**	    Include <tr.h> for the xDEBUGed TRdisplay()s.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	21-may-1997 (shero03)
**	    Added facility tracing diagnostic
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro,
**	    callers of QSF now must supply session's CS_SID in QSF_RCB
**	    (qsf_sid).
**	26-Sep-2003 (jenjo02)
**	    Deleted qsf_handler() as qsr_psem is no longer the
**	    sole gate into QSF. Exceptions occuring in QSF are bugs
**	    and server-fatal conditions.
**	21-Feb-2005 (schka24)
**	    Remove unused qsf entry points.
**	9-feb-2007 (kibro01) b113972
**	    Added parameter to qso_rmvobj to ay if we have a lock yet on
**	    the LRU queue.
**	11-feb-2008 (smeke01) b113972
**	    Backed out above change.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, just a comment about qso_names
**/


/*
**  Forward and/or External function references.
*/


#ifdef    xDEBUG

/*
**  Define op code names for debug tracing printout.
*/

static	char	qsf_opnames[] = "\
<unknown>,\
QSR_STARTUP,\
QSR_SHUTDOWN,\
QSS_BGN_SESSION,\
QSS_END_SESSION,\
QSO_CREATE,\
QSO_DESTROY,\
QSO_LOCK,\
QSO_UNLOCK,\
QSO_SETROOT,\
QSO_INFO,\
QSO_PALLOC,\
QSO_GETHANDLE,\
QSO_SES_COMMIT,\
QSD_OBJ_DUMP,\
QSD_OBQ_DUMP,\
QSO_CHTYPE,\
QSD_BO_DUMP,\
QSD_BOQ_DUMP,\
QSO_TRANS_OR_DEFINE,\
QSO_JUST_TRANS,\
QSO_unused,\
QSO_unused,\
QSO_GETNEXT,\
QSO_unused,\
QSO_DESTROYALL";

/*
**  Define DB_STATUS names for debug tracing printout.
*/

static	char	dbstat_names[] = "\
E_DB_OK,\
<unknown>,\
E_DB_INFO,\
<>,\
E_DB_WARN,\
E_DB_ERROR,\
<>,\
E_DB_SEVERE,\
<>,\
E_DB_FATAL";

#endif /* xDEBUG */


/*{
** Name: qsf_call - Client entry point for the Query Storage Facility.
**
** Description:
**      This routine is the entry point for those clients wishing to 
**      invoke the service of QSF.
**        
**      This routine validates the "global" parameters (such as facility,
**      session, cb type, cb size, etc).  It then invokes the appropriate
**      sub function to perform the service for the client.
**
** Inputs:
**      qsf_operation			The operation requested by the client.
**      qsf_rb				Pointer to the QSF_RCB (request control
**                                      block) which fully describes the
**					requested action.
**
** Outputs:
**	qsf_rb
**		.qsf_error		Filled in as appropriate; for details,
**					see the individual operations.
**
**	Returns:
**	    E_DB_OK
**          E_DB_WARN
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-apr-86 (thurston)
**          Initial creation.
**	18-mar-87 (stec)
**	    Added case QSO_CHTYPE to the switch stmnt.
**	06-apr-87 (thurston)
**	    Fix bug:  QSS_END_SESSION was calling qss_bgn_session() instead of
**	    qss_end_session().
**	13-may-87 (thurston)
**	    Added some trace output stuff which are currently xDEBUG'ed and
**	    should become legitimate trace points.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	23-jan-88 (eric)
**	    Added support for the QSO_TRANS_OR_DEFINE operation.
**	24-may-88 (eric)
**	    Added support for the QSO_JUST_TRANS operation.
**	07-jun-90 (andre)
**	    Added support for QSO_CRTALIAS operation.
**	04-feb-91 (jrb)
**	    Avoided declaration of exception handler when using QSO_PALLOC to
**	    improve performance.
**	09-jan-1992 (fred)
**	    Added QSO_ASSOCIATE operation.
**	10-sep-1992 (bryanp)
**	    Repair "if (error)" test following integration problems.
**	20-oct-1992 (rog)
**	    Add new calls: QSO_GETNEXT and QSO_GET_OR_CREATE.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner comparison since it has changed
**	    type to PTR.
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro,
**	    callers of QSF now must supply session's CS_SID in QSF_RCB
**	    (qsf_sid).
**      18-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Added qsf_clrsesobj() which allows a session to flush any
**          objects it owns in the QSF cache.
**	26-May-2010 (kschendel) b123814
**	    Add session-commit call to complement the session-rollback
**	    entry point (clrsesobj).
*/

DB_STATUS
qsf_call( i4 qsf_operation, QSF_RCB *qsf_rb )
{
    DB_STATUS           status;
#ifdef    xDEBUG
    QSF_CB		*scb = NULL;
    i4			trace_002 = FALSE;
#endif /* xDEBUG */
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_QSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_QSF_ID, qsf_operation, qsf_rb);
#endif

    CLRDBERR(&qsf_rb->qsf_error);

    /*
    ** First, the generic system checks:
    ** Make sure that the request block is good,
    ** that the operation code is valid,
    ** and that QSF has been inited (or is going to be).
    ** If not, return appropriate error.
    */
 
    if (   (qsf_rb->qsf_type != QSFRB_CB)
	|| (qsf_rb->qsf_ascii_id != QSFRB_ASCII_ID)
	|| (qsf_rb->qsf_length != sizeof(QSF_RCB))
	|| (qsf_rb->qsf_owner < (PTR)DB_MIN_FACILITY)
	|| (qsf_rb->qsf_owner > (PTR)DB_MAX_FACILITY)
       )
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0017_BAD_REQUEST_BLOCK);
    }
    else if ( (qsf_operation < QSF_MIN_OPCODE)
	      || (qsf_operation > QSF_MAX_OPCODE)
	    )
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0003_OP_CODE);
    }
    else if ( Qsr_scb == NULL )
    {
	if ( qsf_operation != QSR_STARTUP )
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS001A_NOT_STARTED);
    }
    else if ( qsf_operation != QSR_SHUTDOWN )
    {
	/* Guarantee pointer to this session's QSF_CB in QSF_RCB */
	if ( qsf_rb->qsf_sid )
	    qsf_rb->qsf_scb = GET_QSF_CB(qsf_rb->qsf_sid);
	else
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS0017_BAD_REQUEST_BLOCK);
    }
 

    if (qsf_rb->qsf_error.err_code != 0)
	return (E_DB_ERROR);


#ifdef    xDEBUG
    if (    qsf_operation != QSR_STARTUP
	&&  qsf_operation != QSS_BGN_SESSION
	&&  qsf_operation != QSR_SHUTDOWN
       )
    {
	trace_002 = (Qsr_scb->qsr_tracing && qst_trcheck(scb, QSF_002_CALLS));
	if (trace_002)
	{
	    TRdisplay("QSF called for operation :::::::::::::::::>  %w\n",
		      qsf_opnames,
		      qsf_operation);
	    TRdisplay("    ...  memleft = %d\n", Qsr_scb->qsr_memleft);
	}
    }
#endif /* xDEBUG */


    switch (qsf_operation)
    {
      case QSR_STARTUP:
	status = qsr_startup(qsf_rb);
	if (DB_CURSOR_ID_OFFSET != 0 || DB_CUR_NAME_OFFSET != 2*sizeof(i4)
			|| sizeof(DB_CURSOR_ID) % 4 != 0)
	{
	    /*
	    ** DB_CURSOR_ID is used to build a qso_name
	    ** There is code that assumes that a DB_CURSOR_ID is (i4+i4+NAME)
	    ** and that the name is right after the 2 i4s. e.g. qso_hash()
	    **
	    ** Code that builds a qso_name will sometimes copy 
	    ** sizeof(DB_CURSOR_ID) into the qso_name.
	    ** If there are any compiler added PAD bytes in the DB_CURSOR_ID,
	    ** they must be EXPLICITYLY defined AFTER the name 
	    **
	    ** ALL fields in DB_CURSOR_ID must be initalized before
	    ** copying into a qso_name.
	    **
	    ** See dbdbms.h
	    */
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    status = E_DB_FATAL;
	}
	break;

      case QSR_SHUTDOWN:
	status = qsr_shutdown(qsf_rb);
	break;

      case QSS_BGN_SESSION:
	status = qss_bgn_session(qsf_rb);
	break;

      case QSS_END_SESSION:
	status = qss_end_session(qsf_rb);
	break;

      case QSO_CREATE:
	status = qso_create(qsf_rb);
	break;

      case QSO_DESTROY:
	status = qso_destroy(qsf_rb);
	break;

      case QSO_DESTROYALL:
	status = qso_destroyall(qsf_rb);
	break;

      case QSO_LOCK:
	status = qso_lock(qsf_rb);
	break;

      case QSO_UNLOCK:
	status = qso_unlock(qsf_rb);
	break;

      case QSO_SETROOT:
	status = qso_setroot(qsf_rb);
	break;

      case QSO_INFO:
	status = qso_info(qsf_rb);
	break;

      case QSO_PALLOC:
	status = qso_palloc(qsf_rb);
	break;

      case QSO_GETHANDLE:
	status = qso_gethandle(qsf_rb);
	break;

      case QSO_TRANS_OR_DEFINE:
	status = qso_trans_or_define(qsf_rb);
	break;

      case QSO_JUST_TRANS:
	status = qso_just_trans(qsf_rb);
	break;

      case QSO_CHTYPE:
	status = qso_chtype(qsf_rb);
	break;

      case QSD_OBJ_DUMP:
      case QSD_BO_DUMP:
	status = qsd_obj_dump(qsf_rb);
	break;

      case QSD_OBQ_DUMP:
      case QSD_BOQ_DUMP:
	status = qsd_obq_dump(qsf_rb);
	break;

      case QSO_GETNEXT:
	status = qso_getnext(qsf_rb);
	break;

      case QSO_SES_COMMIT:
	status = qso_ses_commit(qsf_rb);
	break;

      case QSF_SES_FLUSH_LRU:
        status = qsf_clrsesobj(qsf_rb);
        break;

      default:
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	status = E_DB_FATAL;
	break;
    }
	    

#ifdef    xDEBUG
    if (trace_002)
    {
	TRdisplay("    %w\treturn status = %w",
		  qsf_opnames, qsf_operation,
		  dbstat_names, status);
	if (status)
	    TRdisplay(" (err = %x)", qsf_rb->qsf_error.err_code);
	TRdisplay(" ... memleft = %d\n", Qsr_scb->qsr_memleft);
    }
#endif /* xDEBUG */

#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_QSF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_QSF_ID, qsf_operation, qsf_rb);
#endif
    return (status);
}

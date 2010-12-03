/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qsfint.h>

/*
**
**  Name: QSS.C - QSF's Session Services Module.
**
**  Description:
**      This file contains all of the routines necessary
**      to perform the all of QSF's Session Services op-codes.
**      This includes the opcodes to begin and terminate a
**      QSF session.
** 
**      This file contains the following routines:
**
**          qss_bgn_session() - Do QSF session initialization.
**          qss_end_session() - Terminate a QSF session.
**
**
**  History:    $Log-for RCS$
**      06-mar-86 (thurston)    
**          Initial creation.
**	12-may-87 (thurston)
**	    Added an xDEBUG section to qss_end_session() that dumps the QSF
**	    object queue whenever a session termination occurrs.  This should
**	    ultimately become a trace point.
**	05-jul-87 (thurston)
**	    Added code to qss_bgn_session() and qss_end_session() to check for
**	    server wide tracing before even attempting to look for a particular
**	    trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	26-jan-88 (thurston)
**	    qss_bgn_session() now updates the "max # sessions ever" counter in
**	    QSF's server CB.
**	28-aug-92 (rog)
**	    Prototype QSF.
**	20-oct-1992 (rog)
**	    Update accounting of new reliability and performance statistics.
**	    Include ulf.h before qsf.h.
**	12-mar-1993 (rog)
**	    Include <me.h> because ult_init_macro uses MEfill().
**	29-jun-1993 (rog)
**	    Include <tr.h> for the xDEBUGed TRdisplay()s.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro,
**	    callers of QSF now supply session's CS_SID in QSF_RCB
**	    (qsf_sid).
**	26-Sep-2003 (jenjo02)
**	    Deleted qsr_psem_owner.
**	29-Oct-2010 (jonj) SIR 120874
**	    Conform to use new uleFormat, DB_ERROR constructs.
*/


/*{
** Name: QSS_BGN_SESSION - Do QSF session initialization.
**
** Description:
**      This is the entry point to the QSF QSS_BGN_SESSION routine. 
**      This opcode does everything necessary to initialize QSF for 
**      a session.  This includes linking the QSF session control block
**	into the list of all active QSF session control blocks.
**
**	The System Control Sequencer Module (SCS) of SCF will call this
**	routine during session initiation.
**
** Inputs:
**      QSS_BGN_SESSION                 Op code to qsf_call().
**      qsf_rb                          QSF request block.
**	    .qsf_server			Ptr to the QSF server control block.
**	    .qsf_scb			Ptr to the QSF session control block.
**
** Outputs:
**      qsf_rb                          QSF request block.
**	    .qsf_scb			Ptr to the QSF session control block.
**		.qsf_next  \		These two ptrs will be set appropriately
**		.qsf_prev  /		for THIS session CB, as well as others
**					in the list of all QSF session CB's in
**					the server.
**	    .qsf_error			Filled in with error if encountered.
**		.err_code		The QSF error that occurred.  One of:
**				(Error constants not yet defined)
**				E_QS0000_OK
**				    All is fine.
**				E_QS0001_NOMEM
**				    Not enough memory.
**                              E_QS0004_SEMRELEASE
**                                  Error releasing semaphore.
**                              E_QS0008_SEMWAIT
**                                  Error waiting for semaphore.
**				E_QS001B_NO_SESSION_CB
**				    Ptr to session control block is NULL.
**				E_QS9999_INTERNAL_ERROR
**				    Meltdown.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_ERROR
**	    E_DB_SEVERE
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The QSF session control block will be linked into the list of
**	    all active QSF session control blocks.  This list is rooted in
**	    QSF's global SERVER CONTROL BLOCK.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	26-jan-88 (thurston)
**	    Now updates the "max # sessions ever" counter in QSF's server CB.
**	20-oct-1992 (rog)
**	    Eliminate qsr_selast by adding to the head of the list instead
**	    of the tail.  Elimate session use of QSF memory by eliminating the
**	    qss_memory structure.  Set the owner of the QSF semaphore.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-May-2010 (kschendel) b123814
**	    Initialize uncommitted objects list.
*/

DB_STATUS
qss_bgn_session( QSF_RCB *qsf_rb )
{
    STATUS		status;
    QSF_CB		*scb = qsf_rb->qsf_scb;
#ifdef    xDEBUG
    i4		n;
    QSF_CB		*prevses;
    QSF_CB		*nextses;
#endif /* xDEBUG */

    CLRDBERR(&qsf_rb->qsf_error);

    Qsr_scb = (QSR_CB *) qsf_rb->qsf_server;

    if (scb == NULL)
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS001B_NO_SESSION_CB);
	return (E_DB_SEVERE);
    }

    /* Before anything, set up the session CB's standard header portion */
    /* ---------------------------------------------------------------- */
    scb->qsf_ascii_id = QSFCB_ASCII_ID;
    scb->qsf_type = QSFCB_CB;
    scb->qsf_length = sizeof(QSF_CB);
    scb->qsf_prev = NULL;

    /* Update the other structure information.  */
    /* ---------------------------------------- */
    scb->qss_obj_list = (QSO_OBJ_HDR *) NULL;
    scb->qss_master = (QSO_MASTER_HDR *) NULL;
    scb->qss_snamed_list = (QSO_OBJ_HDR *) NULL;

    /* Init the tracing vector */
    /* ----------------------- */
    ult_init_macro(&scb->qss_trstruct.trvect, 128, 0, 8);

    /* Get exclusive access to QSF's SERVER CONTROL BLOCK */
    /* -------------------------------------------------- */
    if (CSp_semaphore((i4) TRUE, &Qsr_scb->qsr_psem))
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0008_SEMWAIT);
	/* Return now, instead of attempting to do a v() */
	return (E_DB_ERROR);
    }

    /* Make the session known to QSF's server CB */
    /* ----------------------------------------- */
    if (Qsr_scb->qsr_se1st != (QSF_CB *) NULL)
	Qsr_scb->qsr_se1st->qsf_prev = scb;
    scb->qsf_next = Qsr_scb->qsr_se1st;
    Qsr_scb->qsr_se1st = scb;
    Qsr_scb->qsr_nsess++;
    if (Qsr_scb->qsr_nsess > Qsr_scb->qsr_mxsess)
	Qsr_scb->qsr_mxsess = Qsr_scb->qsr_nsess;

#ifdef    xDEBUG
    if (Qsr_scb->qsr_tracing && qst_trcheck(&scb, QSF_003_CHK_SCB_LIST))
    {
	/* This code just verifies that the session CB list is intact */
	/* ---------------------------------------------------------- */
	n = Qsr_scb->qsr_nsess;
	prevses = NULL;
	nextses = Qsr_scb->qsr_se1st;

	while (n-- > 0)
	{
	    if (nextses == NULL)
	    {
		TRdisplay("*** Not enough QSF session CBs found");
		TRdisplay(" while beginning a session.\n");
		SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
		return (E_DB_FATAL);
	    }

	    if (	(nextses->qsf_prev != prevses)
		||	(nextses->qsf_ascii_id != QSFCB_ASCII_ID)
		||	(nextses->qsf_type != QSFCB_CB)
		||	(nextses->qsf_length != sizeof(QSF_CB))
	       )
	    {
		TRdisplay("*** QSF's session CB list found trashed");
		TRdisplay(" while beginning a session.\n");
		SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
		return (E_DB_FATAL);
	    }

	    prevses = nextses;
	    nextses = nextses->qsf_next;
	}

	if (nextses != NULL)
	{
	    TRdisplay("*** Too many QSF session CBs detected");
	    TRdisplay(" while beginning a session.\n");
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    return (E_DB_FATAL);
	}
    }
#endif /* xDEBUG */


    status = E_DB_OK;

    /* Release exclusive access to QSF's SERVER CONTROL BLOCK */
    /* ------------------------------------------------------ */
    if (CSv_semaphore(&Qsr_scb->qsr_psem))
    {
	status = E_DB_ERROR;
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0004_SEMRELEASE);
    }

    return (status);
}

/*{
** Name: QSS_END_SESSION - Terminate a QSF session.
**
** Description:
**      This is the entry point to the QSF QSS_END_SESSION routine. 
**      This opcode does everything necessary to terminate QSF for 
**      a session.  This includes removing the QSF session control
**	block from the list of all active QSF session control blocks.
**
**	The System Control Sequencer Module (SCS) of SCF will call this
**	routine during session termination.
**
** Inputs:
**      QSS_END_SESSION                 Op code to qsf_call().
**      qsf_rb                          QSF request block.
**	    .qsf_scb			Pointer to the QSF session
**					control block.
**
** Outputs:
**      qsf_rb                          QSF request block.
**	    .qsf_scb			Ptr to the QSF session control block.
**		.qsf_next  \		These two ptrs will be set appropriately
**		.qsf_prev  /		for THIS session CB, as well as others
**					in the list of all QSF session CB's in
**					the server.
**	    .qsf_error			Filled in with error if encountered.
**		.err_code		The QSF error that occurred.  One of:
**			E_QS0000_OK
**				    All is fine.
**			E_QS0004_SEMRELEASE
**                                  Error releasing semaphore.
**			E_QS0008_SEMWAIT
**                                  Error waiting for semaphore.
**			E_QS0011_ULM_STREAM_NOT_CLOSED
**				    ULM would not close the memory stream for
**				    this session.
**			E_QS9999_INTERNAL_ERROR
**				    Meltdown.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_ERROR
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The QSF session control block will be removed from the list of
**	    all active QSF session control blocks.  This list is rooted in
**	    QSF's global SERVER CONTROL BLOCK.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	12-may-87 (thurston)
**	    Added an xDEBUG section that dumps the QSF object queue whenever
**	    a session termination occurrs.  This should ultimately become a
**	    trace point.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	    Also, look for unnamed objects that should have been destroyed.
**	30-nov-92 (rog)
**	    Set the owner of the QSF semaphore.  Added code to write an error
**	    to the errlog.log if a session exits while still owning unnamed
**	    objects in QSF.  Also, destroy those objects and any named, but
**	    unshared objects owned by this session.
**	04-jan-1993 (rog)
**	    Fix an unreleased bug in the orphaned object code.
**	15-jan-1993 (rog)
**	    Limit the orphaned object detection code to a certain number
**	    of iterations to prevent infinite loops in case something is
**	    really wrong.
**	13-apr-1993 (rog)
**	    We were using the qsf_rb errcode to call ule_format when we should
**	    have been using the int_qsf_rb errcode.
**	23-Feb-2005 (schka24)
**	    Kill brief tracing (xdebug anyway).
**	27-feb-2007 (kibro01) b113972
**	    Add extra parameter to qso_destroy
**	11-feb-2008 (smeke01) b113972
**	    Back out above change.
**	26-May-2010 (kschendel) b123814
**	    Flush uncommitted session-owned objects.  There shouldn't be any
**	    at this point, but do it anyway to make sure.  Clear qso_session
**	    from any LRU-able persistent objects that this session created.
*/

DB_STATUS
qss_end_session( QSF_RCB *qsf_rb)
{
    STATUS		 status = E_DB_OK;
    QSF_CB		*scb = qsf_rb->qsf_scb;
    QSF_RCB		int_qsf_rb;
    QSO_OBJ_HDR		*objects = (QSO_OBJ_HDR *) scb->qss_obj_list;
    QSO_OBJ_HDR		*obj;
    i4		 error;
    i4		 count;
#ifdef    xDEBUG
    QSF_CB		*prevses;
    QSF_CB		*nextses;
    i4			 trace_003;
#endif /* xDEBUG */

    CLRDBERR(&qsf_rb->qsf_error);

    /* Toss any uncommitted named session-owned objects first.
    ** There shouldn't be any, if QEF did its job right, but who knows.
    ** The important thing is that we NOT have objects pretending to be
    ** on a session list when the session has ended.
    */
    int_qsf_rb.qsf_sid = qsf_rb->qsf_sid;
    int_qsf_rb.qsf_scb = scb;
    CLRDBERR(&int_qsf_rb.qsf_error);
    (void) qsf_clrsesobj(&int_qsf_rb);

    /*
    ** Look for orphaned objects that should not exist and destroy unshareable
    ** named objects.
    */
    count = QSF_MAX_ORPHANS;
    while (objects != (QSO_OBJ_HDR *) NULL && --count > 0)
    {
	STATUS		status;	
	QSO_OBJ_HDR	*current_obj = objects;

	objects = objects->qso_obnext;

	if (current_obj->qso_obid.qso_lname == 0)
	{
	    char	*type;

	    switch (current_obj->qso_obid.qso_type)
	    {
		case QSO_QTEXT_OBJ:
		     type = "Query Text";
		     break;
		case QSO_QTREE_OBJ:
		     type = "Query Tree";
		     break;
		case QSO_QP_OBJ:
		     type = "Query Plan";
		     break;
		case QSO_SQLDA_OBJ:
		     type = "SQLDA";
		     break;
		case QSO_ALIAS_OBJ:
		     type = "Alias";
		     break;
		default:
		     type = "Unknown";
		     break;
	    }

	    /* Report the orphaned object. */
	    uleFormat( &int_qsf_rb.qsf_error, E_QS001E_ORPHANED_OBJ,
	    		NULL, (i4) ULE_LOG, NULL,
			NULL, (i4) 0, NULL, &error, 1, 0, type);
	}

	int_qsf_rb.qsf_obj_id = current_obj->qso_obid;
	int_qsf_rb.qsf_lk_state = QSO_EXLOCK;
	int_qsf_rb.qsf_sid = qsf_rb->qsf_sid;
	int_qsf_rb.qsf_scb = scb;
	status = qso_lock(&int_qsf_rb);
	if (DB_FAILURE_MACRO(status))
	{
	    uleFormat( &int_qsf_rb.qsf_error, 0, NULL, (i4) ULE_LOG,
			NULL, NULL, (i4) 0, NULL, &error, 0);
	}

	int_qsf_rb.qsf_lk_id = current_obj->qso_lk_id;
	status = qso_destroy(&int_qsf_rb);
	if (DB_FAILURE_MACRO(status))
	{
	    uleFormat( &int_qsf_rb.qsf_error, 0, NULL, (i4) ULE_LOG,
			NULL, NULL, (i4) 0, NULL, &error, 0);
	}
    }

    if (count <= 0)
    {
	uleFormat( &int_qsf_rb.qsf_error, E_QS001F_TOO_MANY_ORPHANS,
		    NULL, (i4) ULE_LOG,
		    NULL, NULL, (i4) 0, NULL, &error, 0);
    }

#ifdef    xDEBUG
    if (Qsr_scb->qsr_tracing && qst_trcheck(&scb, QSF_001_ENDSES_OBJQ))
    {
	DB_ERROR		save_err = qsf_rb->qsf_error;

	TRdisplay("<<< Dumping QSF object queue before ending session >>>\n");
	(void) qsd_obq_dump(qsf_rb);

	qsf_rb->qsf_error = save_err;
    }


    trace_003 = (   Qsr_scb->qsr_tracing
		 && qst_trcheck(&scb, QSF_003_CHK_SCB_LIST)
		);					    /* Must check before
							    ** we blow away the
							    ** the session CB.
							    */
#endif /* xDEBUG */


    /* First, wait to get exclusive access to QSF's SERVER CONTROL BLOCK */
    /* ----------------------------------------------------------------- */
    if (CSp_semaphore((i4) TRUE, &Qsr_scb->qsr_psem))
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0008_SEMWAIT);
	/* Return now, instead of attempting to do a v() */
	return (E_DB_ERROR);
    }

    /* Do a quick run through the global LRU-able (persistent) object list
    ** and clear the session if it's us.  This indicates that the object
    ** is not on any session list, and prevents someone else from believing
    ** a stale qso_session.
    */

    obj = Qsr_scb->qsr_1st_lru;
    while (obj != NULL)
    {
	if (obj->qso_session == scb)
	    obj->qso_session = NULL;
	obj = obj->qso_lrnext;
    }

    /* Now remove this session from QSF's server CB */
    /* -------------------------------------------- */
    if (scb == Qsr_scb->qsr_se1st)
	Qsr_scb->qsr_se1st = scb->qsf_next;
    else
	scb->qsf_prev->qsf_next = scb->qsf_next;

    if (scb->qsf_next != NULL)
	scb->qsf_next->qsf_prev = scb->qsf_prev;

    scb->qsf_prev = scb->qsf_next = NULL;
    Qsr_scb->qsr_nsess--;


#ifdef    xDEBUG
    if (trace_003)
    {
	/* This code just verifies that the session CB list is intact */
	/* ---------------------------------------------------------- */
	count = Qsr_scb->qsr_nsess;
	prevses = NULL;
	nextses = Qsr_scb->qsr_se1st; 

	while (count-- > 0)
	{
	    if (nextses == NULL)
	    {
		TRdisplay("*** Not enough QSF session CBs found");
		TRdisplay(" while ending a session.\n"); 
		SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
		return (E_DB_FATAL);
	    }

	    if (	(nextses->qsf_prev != prevses)
		||	(nextses->qsf_ascii_id != QSFCB_ASCII_ID)
		||	(nextses->qsf_type != QSFCB_CB)
		||	(nextses->qsf_length != sizeof(QSF_CB))
	       )
	    {
		TRdisplay("*** QSF's session CB list found trashed");
		TRdisplay(" while ending a session.\n");
		SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
		return (E_DB_FATAL);
	    }
	    prevses = nextses;
	    nextses = nextses->qsf_next;
	}

	if (nextses != NULL)
	{
	    TRdisplay("*** Too many QSF session CBs detected");
	    TRdisplay(" while ending a session.\n");
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    return (E_DB_FATAL);
	}
    }
#endif /* xDEBUG */


    status = E_DB_OK;

    /* Release exclusive access to QSF's SERVER CONTROL BLOCK */
    /* ------------------------------------------------------ */
    if (CSv_semaphore(&Qsr_scb->qsr_psem))
    {
	status = E_DB_ERROR;
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0004_SEMRELEASE);
    }

    return (status);
}
/*[@function_definition@]...*/

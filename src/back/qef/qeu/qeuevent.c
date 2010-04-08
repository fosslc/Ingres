/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <sxf.h>
#include    <dmacb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
#include    <usererror.h>
/**
**  Name: qeuevent.c - Provide qrymod event support for RDF and PSF.
**
**  Description:
**	The routines defined in this file provide the qrymod semantics for
**	RDF (relation description facility) event processing. The routines
**	in this file create and destroy event objects.
**
**  Defines:
**      qeu_cevent		- Create an event
**      qeu_devent		- Destroy an event
**
**  History:
** 	25-oct-89 (neil)
**	   Written for Terminator II/alerters.
**	09-feb-90 (neil)
**	   Added auditing functionality.
**	03-mar-90 (neil)
**	   Clear adf_constants before calling ADF.
**	   Modify interface to que_dprot.
**	02-feb-91 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	27-nov-1992 (pholman)
**	   C2: upgrade obsolete qeu_audit to use new qeu_secaudit call.
**	19-may-93 (anitap)
**	    Added support for creation of implicit schema in qeu_cevent().
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	07-jul-93 (anitap)
**	    Added two arguments qef_rcb & qeuq_cb to qea_schema() instead of
**	    PTR in qeu_cevent().
**	    Changed assignment of flag to IMPLICIT_SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	21-sep-93 (stephenb)
**	    Replace generic I_SX2032_EVENT_ACCESS with either 
**	    I_SX2032_EVENT_CREATE or I_SX203C_EVENT_DROP in calls to 
**	    qeu_audit().
**	26-nov-93 (robf)
**          Added qeu_evraise to allow an event to raised externally.
**	12-oct-94 (ramra01)
**	    Initialize variable local_status before use in qeu_devent
**	    reported as a bug 65258
**	13-jun-96 (nick)
**	    Above change didn't actually work.
**	09-Mar-99 (schte01)
**	    Removed extraneous status assignment which caused compiler warning.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	16-dec-04 (inkdo01)
**	    Inited a couple of collID's.
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
*/

static DB_STATUS
qeu_ev_used_with_alarm(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DB_TAB_ID	*event_id
);

/*{
** Name: qeu_cevent 	- Store event information for one event.
**
** External QEF call:   status = qef_call(QEU_CEVENT, &qeuq_cb);
**
** Description:
**	Add the event tuple and the event query text to the appropriate
**	tables (iievent and iiqrytext).
**
**	Most of the event tuple was filled by the parser from the CREATE EVENT
**	statement, except that since this is the first access to iievent,
**	the uniqueness of the event name (within the current user scope) is
**	validated here.  First, all events that have the same name and
**	owner are fetched (the iievent table is hashed on name and owner
**	so this is a singleton keyed retrieval).  If there are any then
**	the event is a duplicate and an error is returned.  If this is
**	unique then the event is entered into iievent.
**
**	To allow permissions to apply to events, an event tuple includes
**	a "dummy table id" (much like database procedures).  This id is
**	retrieved from DMU_GET_TABID.
**
**	An internal qrytext id (time-stamp) is created and all the qrytext
**	tuples associated with the event are entered into iiqrytext.
**	Note that the size of CREATE EVENT query text is never likely to
**	be more than one iiqrytext tuple, but the code allows more than
**	one tuple in case a WITH clause is added in the future (extending
**	the length of the statement).
**
**	The current "create date" is retrieved from ADF (using date("now")).
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_cq		Number of query text tuples.
**	    .qeuq_qry_tup	Query text tuples.
**	    .qeuq_culd		Number of event tuples.  Must be 1.
**	    .qeuq_uld_tup	Event tuple:
**		.dbe_name	Event name.
**		.dbe_owner	Event owner.
**		.dbe_type	Event type.
**				The remaining fields are filled here.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_QE020A_EVENT_EXISTS (ret E_QE0025_USER_ERROR)
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	28-aug-89 (neil)
**	    Written for Terminator II/alerters.
**	09-feb-90 (neil)
**	   Added auditing functionality.
**	03-mar-90 (neil)
**	   Clear adf_constants before calling ADF.  The problem was that the
**	   session CB may have been pointing at "old" data which had been
**	   reused for other things.
**	19-may-93 (anitap)
**	    Added support to create implicit schema for dbevent.
**	07-jul-93 (anitap)
**	    Added two arguments qef_rcb & qeuq_cb to qea_schema() instead of
**	    PTR.
**	    Changed assignment of flag to IMPLICIT_SCHEMA.
**	21-sep-93 (stephenb)
**	    Replace generic I_SX2032_EVENT_ACCESS with I_SX2032_EVENT_CREATE. 
**	27-oct-93 (andre)
**	    As a part of fix for bug 51852 (which was reported in 6.4 but can
**	    and will manifest itself in 6.5 as soon as someone places some 
**	    stress on the DBMS), we want to use the dbevent id (really 
**	    guaranteed to be unique) instead of timestamps (allegedly unique, 
**	    but in fact may be non-unique if several objects get created in 
**	    rapid succession on a reasonably fast box) to identify IIQRYTEXT 
**	    tuples associated with a given dbevent.  This id (pumped through 
**	    randomizing function) will be used to key into IIQRYTEXT 
*/
DB_STATUS
qeu_cevent(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_IIEVENT    	*evtuple;	/* New event tuple */
    DB_IIEVENT    	evtuple_temp;	/* Tuple and 			   */
    DMR_ATTR_ENTRY  	evkey_array[2];	/*       keys for uniqueness check */
    DMR_ATTR_ENTRY  	*evkey_ptr_array[2];
    DMU_CB		dmu_cb;		/* For unique table-id request */
    DB_DATA_VALUE	evcreate_dv;	/* For create date */
    DB_IIQRYTEXT	*qtuple;	/* Event text */
    DB_TAB_ID		randomized_id;
    i4		    	i;		/* Querytext counter  */
    QEF_DATA	    	*next;		/*   and data pointer */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;	/* May need a transaction */
    bool	    	transtarted = FALSE;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    i4			flag = IMPLICIT_SCHEMA;
    QEF_RCB	        *qef_rcb = (QEF_RCB *)NULL;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_cq == 0   || qeuq_cb->qeuq_qry_tup == NULL)
            || (qeuq_cb->qeuq_culd != 1 || qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	/*
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	    transtarted = TRUE;
	}
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	evtuple = (DB_IIEVENT *)qeuq_cb->qeuq_uld_tup->dt_data;

	/* Validate that the event name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_EVENT_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_EVENT_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named/owned event - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IIEVENT);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DB_IIEVENT);
        qef_data.dt_data = (PTR)&evtuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 2;			/* Keyed on name and owner */
	qeu.qeu_key = evkey_ptr_array;
	evkey_ptr_array[0] = &evkey_array[0];
	evkey_ptr_array[0]->attr_number = DM_1_EVENT_KEY;
	evkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	evkey_ptr_array[0]->attr_value = (char *)&evtuple->dbe_name;
	evkey_ptr_array[1] = &evkey_array[1];
	evkey_ptr_array[1]->attr_number = DM_2_EVENT_KEY;
	evkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	evkey_ptr_array[1]->attr_value = (char *)&evtuple->dbe_owner;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)		/* Found the same event! */
	{
	    (VOID)qef_error(E_QE020A_EVENT_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(evtuple->dbe_name),
					  (char *)&evtuple->dbe_name),
			    (PTR)&evtuple->dbe_name);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    error = qeu.error.err_code;	 /* Some other error */
	    break;
	}
	/* The event is unique - append it */

	status = E_DB_OK;

	/*
	** Get unique event id (a table id) from DMF.  This id enables
	** permissions to be associated with the event.
	*/
	dmu_cb.type = DMU_UTILITY_CB;
	dmu_cb.length = sizeof(DMU_CB);
	dmu_cb.dmu_flags_mask = 0;
	dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;
	dmu_cb.dmu_db_id = qeuq_cb->qeuq_db_id;
	status = dmf_call(DMU_GET_TABID, &dmu_cb);
	if (status != E_DB_OK)
	{
	    error = dmu_cb.error.err_code;
	    break;
	}
	evtuple->dbe_uniqueid.db_tab_base  = dmu_cb.dmu_tbl_id.db_tab_base;
	evtuple->dbe_uniqueid.db_tab_index = dmu_cb.dmu_tbl_id.db_tab_index;

	/*
	** use dbevent id to generate randomized id which will be used to 
	** identify IIQRYTEXT tuples associated with this dbevent
	*/
	randomized_id.db_tab_base = 
	    ulb_rev_bits(evtuple->dbe_uniqueid.db_tab_base);
	randomized_id.db_tab_index = 
	    ulb_rev_bits(evtuple->dbe_uniqueid.db_tab_index);

	/* set text id to "randomized" id */
	evtuple->dbe_txtid.db_qry_high_time = randomized_id.db_tab_base;
	evtuple->dbe_txtid.db_qry_low_time  = randomized_id.db_tab_index;

	/* Get the create date of the event */
	evcreate_dv.db_datatype = DB_DTE_TYPE;
	evcreate_dv.db_prec = 0;
	evcreate_dv.db_length = sizeof(DB_DATE);
	evcreate_dv.db_data = (PTR)&evtuple->dbe_create;
	evcreate_dv.db_collID = -1;
	qef_cb->qef_adf_cb->adf_constants = NULL;
	status = adu_datenow(qef_cb->qef_adf_cb, &evcreate_dv);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}

	/* Insert single event tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IIEVENT);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup; /* Which points at evtuple */
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/*
	** Update all query text tuples with query id - validate list.  Even
	** though we will probably have just one text tuple this is set up
	** for syntactic extensions to the CREATE EVENT statement.
	*/
	next = qeuq_cb->qeuq_qry_tup;
	for (i = 0; i < qeuq_cb->qeuq_cq; i++)
	{
	    qtuple = (DB_IIQRYTEXT *)next->dt_data;
	    next = next->dt_next;
	    qtuple->dbq_txtid.db_qry_high_time = randomized_id.db_tab_base;
	    qtuple->dbq_txtid.db_qry_low_time  = randomized_id.db_tab_index;
	    if (i < (qeuq_cb->qeuq_cq -1) && next == NULL)
	    {
		error = E_QE0018_BAD_PARAM_IN_CB;
		status = E_DB_ERROR;
		break;
	    }
	} /* for all query text tuples */
	if (status != E_DB_OK)
	    break;

	/* Insert all query text tuples */
	qeu.qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = 0L;
	qeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;
	qeu.qeu_count = qeuq_cb->qeuq_cq;
        qeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);
	qeu.qeu_input = qeuq_cb->qeuq_qry_tup;
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

        status = qea_schema(qef_rcb, qeuq_cb, qef_cb, 
		(DB_SCHEMA_NAME *)&evtuple->dbe_owner, flag);
        if (status != E_DB_OK)
        {
           error = qeuq_cb->error.err_code;
           break;
        }

	if (transtarted)		   /* If we started a transaction */
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}


	/* Audit CREATE EVENT */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
		  evtuple->dbe_name.db_name, &evtuple->dbe_owner, DB_MAXNAME,
		  SXF_E_EVENT, I_SX2032_EVENT_CREATE, 
		  SXF_A_SUCCESS | SXF_A_CREATE, &e_error);

	    if (status != E_DB_OK)
		break;
	}

	qeuq_cb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    } /* End dummy for */

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);

    /* Close off all the tables. */
    local_status = E_DB_OK;
    if (tbl_opened)		    /* If system table opened, close it */
    {
	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qeu.error.err_code, 0L, local_status, &error,
		    	     &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (transtarted)		   /* If we started a transaction */
    {
        local_status = qeu_atran(qef_cb, &tranqeu);
        if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status,
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_cevent */

/*{
** Name: qeu_devent 	- Drop an event definition.
**
** External QEF call:   status = qef_call(QEU_DEVENT, &qeuq_cb);
**
** Description:
**	Drop the event tuple and the event text from the appropriate
**	tables (iievent and iiqrytext).  An event may only be dropped
**	by name as a result of the DROP EVENT statement.
**
**	The single event tuple is deleted based on its name and owner (since
**	the iievent table is hashed on name and owner this is a singleton
**	keyed retrieval).  If the event tuple wasn't fetched, then an error
**	is returned to the user.
**
**	If the event was found then all permits applied to this event
**	are dropped (using qeu_dprot with the dummy event/table ids and
**	without a permit number to indicate ALL protection tuples).  In
**	this case qeu_dprot need not recheck that the event does exist.
**	Note that the qeu_dprot routine will open and close iiqrytext
**	to remove the text associated with the event permit.
**
**	Following permit removal all query text tuples associated with the
**	CREATE EVENT statement are removed from iiqrytext based on the
**	query text ids in the fetched event tuple.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of event tuples.  Must be 1.
**	    .qeuq_uld_tup	Event tuple:
**		.dbe_name	Event name.
**		.dbe_owner	Event owner.
**		.dbe_type	Event type - ignored until there are more.
**				Remaining fields are assigned and used here.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_QE020B_EVENT_ABSENT (ret E_QE0025_USER_ERROR)
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	28-aug-89 (neil)
**	    Written for Terminator II/alerters.
**	09-feb-90 (neil)
**	   Added auditing functionality.
**	03-mar-90 (neil)
**	   Modify interface to que_dprot to indicate from DROP EVENT.
**	24-jun-92 (andre)
**	    after deleting IIEVENT tuple, call qeu_d_cascade to handle
**	    destruction of IIPROTECT, IIQRYTEXT tuples and marking dependent
**	    dbprocs dormant
**	08-sep-92 (andre)
**	    before calling qeu_d_cascade(), reset QEU_DROP_TEMP_TABLE in
**	    qeuq_flag_mask
**	24-jun-93 (robf)
**	    Perform MAC access check prior to drop of event. If no access is
**	    allowed then treat as non-existent event.
**	10-sep-93 (andre)
**	    before calling qeu_d_cascade(), set QEU_FORCE_QP_INVALIDATION in
**	    qeuq_flag_mask
**	21-sep-93 (stephenb)
**	    Replace generic I_SX2032_EVENT_ACCESS with I_SX203C_EVENT_DROP.
**	08-oct-93 (andre)
**	    qeu_d_cascade() expects one more parameter - an address of a 
**	    DMT_TBL_ENTRY describing table/view/index being dropped; for all 
**	    other types of objects, NULL must be passed
**	14-oct-93 (robf)
**          Make sure access check is on tuple being  deleted, not input tuple.
**	7-dec-93 (robf)
**          Check if event is in used by an alarm, if so reject the operation.
**	7-jun-94 (robf)
**          audit the security label for the event correctly on B1, this 
**	    ensures level auditing picks up the change. (Previously we were 
**	    auditing the input tuple security label, which was empty,
**	    not the delete tuple security label). Also don't try to
**	    but this makes the case clearer)
**	12-oct-94 (ramra01)
**	    Initialize variable local_status before use - 65258
**	13-jun-96 (nick)
**	    Above change didn't actually work.
**
*/
DB_STATUS
qeu_devent(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;
    QEU_CB	    	evqeu;		/* For iievent table */
    QEF_DATA	    	evqef_data;
    DB_IIEVENT	    	*evtuple;	/* Input tuple with name and owner */
    DB_IIEVENT	    	evtuple_del;	/* Tuple currently being deleted */
    bool	    	event_opened = FALSE;
    DMR_ATTR_ENTRY  	evkey_array[2];	/* Key values from iievent */
    DMR_ATTR_ENTRY  	*evkey_ptr_array[2];
    DB_STATUS	    	status;
    DB_STATUS		local_status = E_DB_OK;
    i4	    	error;
    DB_ERROR		err;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_culd != 1 || qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
	/*
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	    transtarted = TRUE;
	}
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	evtuple  = (DB_IIEVENT *)qeuq_cb->qeuq_uld_tup->dt_data;

	/* Open iievent tables */
	evqeu.qeu_type = QEUCB_CB;
        evqeu.qeu_length = sizeof(QEUCB_CB);
        evqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        evqeu.qeu_lk_mode = DMT_IX;
        evqeu.qeu_flag = DMT_U_DIRECT;
        evqeu.qeu_access_mode = DMT_A_WRITE;
	evqeu.qeu_mask = 0;

        evqeu.qeu_tab_id.db_tab_base  = DM_B_EVENT_TAB_ID;   /* Open iievent */
        evqeu.qeu_tab_id.db_tab_index = DM_I_EVENT_TAB_ID; 
	status = qeu_open(qef_cb, &evqeu);
	if (status != E_DB_OK)
	{
	    error = evqeu.error.err_code;
	    break;
	}
	event_opened = TRUE;

	evqeu.qeu_count = 1;	   	    /* Initialize event-specific qeu */
        evqeu.qeu_tup_length = sizeof(DB_IIEVENT);
	evqeu.qeu_output = &evqef_data;
	evqef_data.dt_next = NULL;
        evqef_data.dt_size = sizeof(DB_IIEVENT);
        evqef_data.dt_data = (PTR)&evtuple_del;

	/*
	** Position iievent table based on name & owner and drop specific
	** event.  If we can't find it issue an error.
	*/
	evqeu.qeu_getnext = QEU_REPO;

	/* Get event that matches (keyed on) event name and owner */
	evqeu.qeu_klen = 2;
	evqeu.qeu_key = evkey_ptr_array;
	evkey_ptr_array[0] = &evkey_array[0];
	evkey_ptr_array[0]->attr_number = DM_1_EVENT_KEY;
	evkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	evkey_ptr_array[0]->attr_value = (char *)&evtuple->dbe_name;
	evkey_ptr_array[1] = &evkey_array[1];
	evkey_ptr_array[1]->attr_number = DM_2_EVENT_KEY;
	evkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	evkey_ptr_array[1]->attr_value = (char *)&evtuple->dbe_owner;
	evqeu.qeu_qual = NULL;
	evqeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &evqeu);
	if (status != E_DB_OK)
	{
	    if (evqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		(VOID)qef_error(E_QE020B_EVENT_ABSENT, 0L,
				E_DB_ERROR, &error, &qeuq_cb->error, 1,
				qec_trimwhite(sizeof(evtuple->dbe_name),
					      (char *)&evtuple->dbe_name),
				(PTR)&evtuple->dbe_name);
		error = E_QE0025_USER_ERROR;
	    }
	    else	/* Other error */
	    {
		error = evqeu.error.err_code;
	    }
	    break;
	} /* If no event found */
	
	break;
    } /* End dummy for loop */

    /* Handle any error messages */
    if (status != E_DB_OK)
    {
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    }
    else
    {
	QEUQ_CB	    dqeuq;
	STRUCT_ASSIGN_MACRO(*qeuq_cb, dqeuq);
	dqeuq.qeuq_flag_mask &= ~QEU_DROP_TEMP_TABLE;
	dqeuq.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
	dqeuq.qeuq_rtbl = &evtuple_del.dbe_uniqueid;
	STRUCT_ASSIGN_MACRO(evtuple_del.dbe_txtid, dqeuq.qeuq_qid);
	dqeuq.qeuq_uld_tup = &evqef_data;
	/*
	** Check if dbevent used by alarm, if so we can't drop it
	*/
	if(qeu_ev_used_with_alarm(qef_cb, qeuq_cb, &evtuple_del.dbe_uniqueid)
				!=E_DB_OK)
	{
	        (VOID) qef_error(E_US2479_9337_ALARM_USES_EVENT, 
			0L, local_status,
			 &error, &qeuq_cb->error, 2,
			 qec_trimwhite(sizeof(evtuple->dbe_owner),
					(char*)&evtuple->dbe_owner),
			 (PTR)&evtuple->dbe_owner,
			 qec_trimwhite(sizeof(evtuple->dbe_name),
					(char*)&evtuple->dbe_name),
			 (PTR)&evtuple->dbe_name);
		error = E_QE0025_USER_ERROR;
		status=E_DB_ERROR;
	}
	
	if(status==E_DB_OK)
	{
		/*
		** perform cascading destruction - qeu_d_cascade() handles its
		** own error reporting
		*/
		STRUCT_ASSIGN_MACRO(*qeuq_cb, dqeuq);
		dqeuq.qeuq_flag_mask &= ~((i4) QEU_DROP_TEMP_TABLE);
		dqeuq.qeuq_rtbl = &evtuple_del.dbe_uniqueid;
		STRUCT_ASSIGN_MACRO(evtuple_del.dbe_txtid, dqeuq.qeuq_qid);
		dqeuq.qeuq_uld_tup = &evqef_data;

	/* perform cascading destruction of the dbevent */
	status = qeu_d_cascade(qef_cb, &dqeuq, (DB_QMODE) DB_EVENT, 
	    (DMT_TBL_ENTRY *) NULL);

		if (status != E_DB_OK)
		{
		    STRUCT_ASSIGN_MACRO(dqeuq.error, qeuq_cb->error);
		}
	}
    }

    /* Audit DROP EVENT if so far so good */
    if ( status == E_DB_OK && Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	DB_ERROR    e_error;
	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
	      evtuple->dbe_name.db_name, &evtuple->dbe_owner, DB_MAXNAME,
	      SXF_E_EVENT, I_SX203C_EVENT_DROP, SXF_A_SUCCESS | SXF_A_DROP,
	      &e_error);
    }

    /* Close off all the tables and transaction */
    if (event_opened)
    {
	local_status = qeu_close(qef_cb, &evqeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(evqeu.error.err_code, 0L, local_status,
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (transtarted)
    {
	if (status == E_DB_OK)
	    local_status = qeu_etran(qef_cb, &tranqeu);
	else
	    local_status = qeu_atran(qef_cb, &tranqeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status,
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_devent */

/*
** Name: qeu_evraise
**
** Description:
**	This routine raises an event, it processes the external request
**	QEU_RAISE_EVENT, typically when raising the event associated with
**	a security alarm.
**
**	Note this *does* do event tracing (SET PRINTEVENTS/LOGEVENTS)
**	but does *not* do security checks/auditing on the operation, this
**	is assumed to be handled at a higher level.
**
** Inputs:
**	qef_rcb.evname   - Event name
**	qef_rcb.evowner  - Event owner
**	qef_rcb.evtext   - Event text 
**	qef_rcb.ev_l_text- Length of text
**
** History:
**	26-nov-93 (robf)
**         Created for secure 2.0
**	12-Jan-1998 (kinpa04/merja01)
**		Remove (i4) casting of session id.  This caused the 
**		session ID to get truncated on axp_osf while raising events.
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	30-Dec-2005 (kschendel)
**	    Update call to qef-adf-error.
**
*/
DB_STATUS
qeu_evraise(
	QEF_CB  *qef_cb,
	QEF_RCB *qef_rcb
)
{
    DB_DATA_VALUE tm;					/* Timestamp */
    DB_DATE	tm_date;
    SCF_ALERT	scfa;					
    SCF_CB	scf_cb;				
    SCF_SCI		sci_list[1];
    DB_ALERT_NAME alert;
    i4	err;
    char	*errstr;
    DB_STATUS   status=E_DB_OK;
    i4	tr1 = 0, tr2 = 0;			/* Dummy trace values */

    /*
    ** Build alert name
    */
    STRUCT_ASSIGN_MACRO(*qef_rcb->qef_evname, alert.dba_alert);
    STRUCT_ASSIGN_MACRO(*qef_rcb->qef_evowner, alert.dba_owner);

    scf_cb.scf_length	= sizeof(SCF_CB);
    scf_cb.scf_type	= SCF_CB_TYPE;
    scf_cb.scf_facility	= DB_QEF_ID;
    scf_cb.scf_session	= qef_cb->qef_ses_id;

    /* Get the database name */
    scf_cb.scf_ptr_union.scf_sci   = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length  = sizeof(DB_DB_NAME);
    sci_list[0].sci_code    = SCI_DBNAME;
    sci_list[0].sci_aresult = (char *) &alert.dba_dbname;
    sci_list[0].sci_rlength = NULL;
    status = scf_call(SCU_INFORMATION, &scf_cb);
    if (status != E_DB_OK)
    {
         _VOID_ qef_error(E_QE022F_SCU_INFO_ERROR, 0L, status, &err,
			     &qef_rcb->error, 0);
	return E_DB_ERROR;
    }

    /* Format SCF event request block */
    scf_cb.scf_ptr_union.scf_alert_parms = &scfa;

    scfa.scfa_name = &alert;
    scfa.scfa_text_length = 0;
    scfa.scfa_user_text = NULL;
    scfa.scfa_flags = 0;

    if (qef_rcb->qef_ev_l_text > 0)		/* No need for empty strings */
    {
	if (qef_rcb->qef_ev_l_text > DB_EVDATA_MAX)
	    qef_rcb->qef_ev_l_text = DB_EVDATA_MAX;
	scfa.scfa_text_length = qef_rcb->qef_ev_l_text;
	scfa.scfa_user_text = qef_rcb->qef_evtext;
    }
    tm.db_datatype = DB_DTE_TYPE; 	/* Get time stamp for the event */
    tm.db_prec = 0;
    tm.db_length = sizeof(tm_date);
    tm.db_data = (PTR)&tm_date;
    tm.db_collID = -1;
    status = adu_datenow(qef_cb->qef_adf_cb, &tm);
    if (status != E_DB_OK)
    {
	    if ((status = qef_adf_error(&qef_cb->qef_adf_cb->adf_errcb,
			status, qef_cb, &qef_rcb->error)) != E_DB_OK)
		return (status);
    }
    scfa.scfa_when = &tm_date;

    /* If tracing and/or logging events then display event information */
    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_EVENTS, &tr1, &tr2))
         qea_evtrace(qef_rcb, QEF_T_EVENTS, &alert, &tm, 
			(i4)qef_rcb->qef_ev_l_text,
			(char*)qef_rcb->qef_evtext);

    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_LGEVENTS, &tr1, &tr2))
        qea_evtrace(qef_rcb, QEF_T_LGEVENTS, &alert, &tm, 0, (char *)NULL);

    /*
    ** Raise the event in SCF
    */
    status = scf_call(SCE_RAISE, &scf_cb);
    if (status != E_DB_OK)
    {
	char	*enm, *onm;	/* Event and owner names */

	enm = (char *)&alert.dba_alert;
	onm = (char *)&alert.dba_owner;
	errstr="RAISE";
	switch (scf_cb.scf_error.err_code)
	{
	  case E_SC0270_NO_EVENT_MESSAGE:
	    _VOID_ qef_error(E_QE019A_EVENT_MESSAGE, 0L, status, &err,
			     &qef_rcb->error, 1,
			     (i4)STlength(errstr), errstr);
	    break;
	  case E_SC0280_NO_ALERT_INIT:
	    _VOID_ qef_error(E_QE0200_NO_EVENTS, 0L, status, &err,
			     &qef_rcb->error, 1,
			     (i4)STlength(errstr), errstr);
	    break;
	  default:
	    _VOID_ qef_error(E_QE020F_EVENT_SCF_FAIL, 0L, status, &err,
			     &qef_rcb->error, 4,
			     (i4)STlength(errstr), errstr,
			     (i4)sizeof(i4),
					(PTR)&scf_cb.scf_error.err_code,
			     qec_trimwhite(DB_MAXNAME, onm), onm,
			     qec_trimwhite(DB_MAXNAME, enm), enm);
	    break;
	}
	qef_rcb->error.err_code = E_QE0025_USER_ERROR;
	status = E_DB_ERROR;
    } /* If SCF not ok */
    return status;
}

/*
** Name: qeu_ev_used_with_alarm
**
** Description:
**	Determines whether a security alarm is using this event
**
** Inputs:
**	event_id	- Event id
**	
** History:
**	7-dec-93 (robf)
**	    Created
**	21-dec-93 (robf)
**	    Lookup by id now.
**	09-Mar-99 (schte01)
**	    Removed extraneous status assignment which caused compiler warning.
**	06-Aug-2002 (jenjo02)
**	    Init qeu_mask before calling qeu_open() lest DMF think we're
**	    passing bogus lock information (QEU_TIMEOUT,QEU_MAXLOCKS).
*/
static DB_STATUS
qeu_ev_used_with_alarm(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DB_TAB_ID	*event_id
)
{
    QEU_CB		aqeu;
    QEF_DATA		aqef_data;
    DB_SECALARM		alarm;
    DB_STATUS		status=E_DB_OK;
    bool		loop=FALSE;
    bool		found=FALSE;
    i4		error=0;

    do
    {
	/* Process security alarms */
	aqeu.qeu_type = QEUCB_CB;
	aqeu.qeu_length = sizeof(QEUCB_CB);
        aqeu.qeu_tab_id.db_tab_base  = DM_B_IISECALARM_TAB_ID;
        aqeu.qeu_tab_id.db_tab_index  = DM_I_IISECALARM_TAB_ID;
        aqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        aqeu.qeu_lk_mode = DMT_S;
        aqeu.qeu_flag = QEU_BYPASS_PRIV;
        aqeu.qeu_access_mode = DMT_A_READ;
	aqeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &aqeu);
	if(status!=E_DB_OK)
		break;
	aqeu.qeu_getnext = QEU_REPO;
	aqeu.qeu_klen = 0;
	aqeu.qeu_qual = 0;
	aqeu.qeu_qarg = 0;
        aqeu.qeu_count = 1;
        aqeu.qeu_tup_length = sizeof(DB_SECALARM);
        aqeu.qeu_output = &aqef_data;
        aqef_data.dt_next = 0;
        aqef_data.dt_size = sizeof(DB_SECALARM);
        aqef_data.dt_data = (PTR)&alarm;

	while (status == E_DB_OK)
	{
	    /* Read all iisecalarm records. */


	    status = qeu_get(qef_cb, &aqeu);
	    if ((status != E_DB_OK) &&
		(aqeu.error.err_code == E_QE0015_NO_MORE_ROWS))
	    {
		status = E_DB_OK;
		break;
	    }
	    else if ((status == E_DB_OK) && (aqeu.qeu_count != 1))
	    {
		error = E_US2476_9334_IISECALARM_ERROR;
		if (status < E_DB_ERROR)
		    status = E_DB_ERROR;
		break;
	    }
	    if (event_id->db_tab_base==alarm.dba_eventid.db_tab_base &&
		event_id->db_tab_index==alarm.dba_eventid.db_tab_index)
	    {
		found=TRUE;
		break;
	    }
	    aqeu.qeu_getnext = QEU_NOREPO;
	    aqeu.qeu_klen = 0;
	}
        status = qeu_close(qef_cb, &aqeu);    
	if (status != E_DB_OK)
	{
	    error= E_US2476_9334_IISECALARM_ERROR;
	}
    } while (loop);

    /*
    ** Report any errors
    */
    if(error)
    {
	    i4		err;
	    DB_ERROR		err_blk;
	    _VOID_ qef_error(error, 0L, status, &err,
			     &err_blk, 0);
    }
    if(status==E_DB_OK)
    {
	    if(found)
		status=E_DB_WARN;
	    else
		status=E_DB_OK;
    }
    
    return status;
}

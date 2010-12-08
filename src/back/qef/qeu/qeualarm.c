/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <cm.h>
#include    <cv.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <ddb.h>
#include    <usererror.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
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
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: qeualarm.c - Provide qrymod security alarm support for RDF and PSF.
**
**  Description:
**      The routines defined in this file provide the qrymod semantics for
**	RDF (relation description facility) security alarm processing.	Qrymod
**	provides query modification to query trees in the form of added
**	protections, integrities, alarm and views. The routines in this file
**	create and destroy security alarm  objects.  See qeuqmod.c for other 
**	qrymod objects.
**
**  Defines:
**      qeu_csecalarm		- Create a security alarm
**      qeu_dsecalarm		- Destroy a security alarm
**	qeu_db_exists		- Does a database exist
**
**  Locals:
**	qeu_qalarm_by_name  	- Qualify alarm by name.
**
**  History:
** 	26-nov-93 (robf)
**	   Written for Secure 2.0
**      19-oct-94 (canor01)
**         remove extraneous casts for que_d_id to get through
**         AIX 4.1 compiler
**	24-jan-95 (stephenb)
**	   Fix bug 66255: we must check that we are refferencing the correct
**	   table when dropping alarm by name in qeu_qalarm_by_name().
**      03-mar-95 (sarjo01)
**         Bug no. 67035: for DROP SECURITY_ALARM ON CURRENT INSTALLATION,
**         return tuple as qualified to drop if objname is null and
**         objtype == 'D'
**      20-mar-95 (inkdo01)
**         Bug 67526: DROP SECURITY_ALARM on named alarm for database/
**         current installation doesn't assure that alarm is for THAT
**         database or current installation
**	13-jun-96 (nick)
**	    LINT directed changes.
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
**	    Deleted redeclaration of dmf_call().
**      18-Apr-2001 (hweho01)
**          Removed the prototype declaration of qef_error(), since it
**          is defined in qefprotos.h header file.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**	11-Apr-2008 (kschendel)
**	    Updated qeu-qualification call sequences.
**	4-Jun-2009 (kschendel) b122118
**	    Make sure dmt-cb doesn't have junk in it.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/


static DB_STATUS qeu_qalarm_by_name(
    void		*search_tup,
    QEU_QUAL_PARAMS	*cur_tup);

static DB_STATUS
qeu_gen_alarm_name(
    i4                   obj_type,
    char                 *obj_name,
    QEF_RCB		 *qef_rcb,
    DB_TAB_ID		 *alarm_id,
    char		 *alarm_name,
    i4		 *err_code
);
static VOID
seedToDigits(
    DB_TAB_ID	*seed,
    char	*digitString,
    QEF_RCB	*qef_rcb
);

/*{
** Name: qeu_csecalarm 	- Store information for one security alarm.
**
** External QEF call:   status = qef_call(QEU_CSECALM, &qeuq_cb);
**
** Description:
**	Add the alarm tuple, the alarm query text, to the appropriate tables 
**	(iisecalarm, iiqrytext)
**
**	The alarm tuple was filled from the CREATE SECURITY_ALARM statement, 
**      except that since this is the first access to iisecalarm, the uniqueness
**	of the alarm name (within the current user scope) is validated here.
**	First, all alarms that have the same name are fetched.
**	If there are any then the alarm is a duplicate and an error is
**	returned.  If this is unique then the alarm is entered into iisecalarm.
**	Note that for fast alarm access during query processing alarms are
**	qualified and keyed off of the object ids they reference.  In this
**	case the alarm is qualified by the name.
**
**	An internal qrytext id (timestamp) is created and all the qrytext
**	tuples associated with the alarm are entered into iiqrytext.  An
**	internal tree id (timestamp) is created and all the tree tuples
**	associated with the alarm are entered into iiqrytext. Note that
**	based on size of text and/or tree there may be more than one
**	tuple of each for the single alarm.  
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_cq		Number of query text tuples.
**	    .qeuq_qry_tup	Query text tuples.
**	    .qeuq_uld_tup	Alarm tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	26-nov-93 (robf)
**	    Written for Secure 2.0
*/

DB_STATUS
qeu_csecalarm(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_IIQRYTEXT	*qtuple;	/* Alarm text */
    DB_SECALARM    	*atuple;	/* New alarm tuple */
    DB_SECALARM    	atuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status, local_status;
    i4	    	error=0;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;		/* Querytext tuple counter */
    QEF_DATA	    	*next;		/*	     and data pointer       */
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEU_QUAL_PARAMS	qparams;
    DB_QRY_ID	    	qryid;		/* Ids for query text */
    QEF_DATA	    	qef_data;
    bool		loop=FALSE;
    QEF_RCB		qef_rcb;
    i4		number;
    DMR_ATTR_ENTRY  	akey_array[3];
    DMR_ATTR_ENTRY  	*akey_ptr_array[3];
    DMT_CHAR_ENTRY  	char_spec;	/* DMT_ALTER specification */
    DMT_CB	    	dmt_cb;
    DB_ERROR 		e_error;
    SXF_ACCESS		access;
    SXF_EVENT		evtype;
    DB_OWN_NAME		objowner;

    do
    {
	MEfill(sizeof(objowner),' ',(PTR)&objowner);

	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   qeuq_cb->qeuq_cq == 0 || qeuq_cb->qeuq_qry_tup == NULL
            || ( qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
        qef_rcb.qef_cb = qef_cb;
        qef_rcb.qef_db_id = qeuq_cb->qeuq_db_id;
        qef_cb->qef_dbid = qeuq_cb->qeuq_db_id;
		
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
	    tranqeu.qeu_mask = 0;
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

	atuple = (DB_SECALARM *)qeuq_cb->qeuq_uld_tup->dt_data;

        /* For  database alarms, make sure the database exists */
	if(atuple->dba_objtype==DBOB_DATABASE &&
	   !(atuple->dba_flags&DBA_ALL_DBS))
	{
	    status=qeu_db_exists(qef_cb, qeuq_cb, 
		(DB_DB_NAME *)&atuple->dba_objname, SXF_A_CONTROL, &objowner);
	    if(status==E_DB_ERROR)
	    {
		/* Error checking database name */
		break;
	    }
	    else  if  (status==E_DB_WARN)
	    {
		/* Database not found */
		status=E_DB_ERROR;
		/* E_US2474_9332_ALARM_NO_DB */
	        (VOID)qef_error(9332, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(atuple->dba_objname),
					  (char *)&atuple->dba_objname),
			    (PTR)&atuple->dba_objname);
		break;
	    }
	}
	else if(atuple->dba_objtype==DBOB_TABLE)
	{
	    /* 
            ** Alter the table relstat to flag alarms exist. This validates that
            ** the table exists and ensures that we get an exclusive lock on it.
            */
	    char_spec.char_id = DMT_C_ALARM;      /* create alarm code */
	    char_spec.char_value = DMT_C_ON;
	    MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	    dmt_cb.dmt_flags_mask = 0;
	    dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	    dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	    dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
	    dmt_cb.length = sizeof(DMT_CB);
	    dmt_cb.type = DMT_TABLE_CB;
	    dmt_cb.dmt_id.db_tab_base  = atuple->dba_objid.db_tab_base;
	    dmt_cb.dmt_id.db_tab_index = atuple->dba_objid.db_tab_index;
	    dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
	    status = dmf_call(DMT_ALTER, &dmt_cb);
	    if (status != E_DB_OK)
    	    {
	        error = dmt_cb.error.err_code;
	        break;
	    }
	}
	/* Get a unique query id */
	TMnow((SYSTIME *)&qryid);
	atuple->dba_txtid.db_qry_high_time = qryid.db_qry_high_time;
	atuple->dba_txtid.db_qry_low_time  = qryid.db_qry_low_time;

	/* Get a unique alarm id */
        
        status = qea_reserveID(&atuple->dba_alarmid, 
			qef_cb, &qef_rcb.error);

	if (status != E_DB_OK)
	{   
		error = qef_rcb.error.err_code;  
		break; 
	}
	/*
	** Generate an alarm name if needed, all blanks indicates no
	** name specified. This is required for uniqueness check below.
	*/
	if(!STskipblank( (char*)&atuple->dba_alarmname, 
				sizeof(atuple->dba_alarmname)))
	{
		status=qeu_gen_alarm_name(
			atuple->dba_objtype,
			(char*)&atuple->dba_objname,
			&qef_rcb,
			&atuple->dba_alarmid,
			(char*)&atuple->dba_alarmname,
			&error);
		if(status!=E_DB_OK)
			break;
	}
	/* Validate that the alarm name is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_IISECALARM_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_IISECALARM_TAB_ID;
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
	atuple->dba_alarmno=0;

	/* Retrieve the same named alarm - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_SECALARM);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DB_SECALARM);
        qef_data.dt_data = (PTR)&atuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 0;				/* Not keyed */
	qparams.qeu_qparms[0] = (PTR) atuple;	/* What we're looking for */
	qeu.qeu_qual = qeu_qalarm_by_name;
	qeu.qeu_qarg = &qparams;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)		/* Found the same alarm! */
	{
	    (VOID)qef_error(E_US2472_9330_ALARM_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(atuple->dba_alarmname),
					  (char *)&atuple->dba_alarmname),
			    (PTR)&atuple->dba_alarmname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened=FALSE;
	/*
	** Generate a unique alarm number. This is primarily for
	** backwards compatibility with "anonymous" alarms which had
	** to be dropped by number, not name.
	*/
	
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_SECALARM);
	qeu.qeu_input = &qef_data;
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
        qef_data.dt_size = sizeof(DB_SECALARM);
        qef_data.dt_data = (PTR) &atuple_temp;
	
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 3;       
	qeu.qeu_key = akey_ptr_array;
	akey_ptr_array[0] = &akey_array[0];
	akey_ptr_array[1] = &akey_array[1];
	akey_ptr_array[2] = &akey_array[2];

	akey_ptr_array[0]->attr_number = DM_1_IISECALARM_KEY;
	akey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	akey_ptr_array[0]->attr_value = (char*) &atuple->dba_objtype;

	akey_ptr_array[1]->attr_number = DM_2_IISECALARM_KEY;
	akey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	akey_ptr_array[1]->attr_value = (char*) &atuple->dba_objid.db_tab_base;
        
	akey_ptr_array[2]->attr_number = DM_3_IISECALARM_KEY;
	akey_ptr_array[2]->attr_operator = DMR_OP_EQ;
	akey_ptr_array[2]->attr_value = (char*) &atuple->dba_objid.db_tab_index;
        
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
     
	/* 
        ** Get all alarm tuples for this object
        ** and determine the next alarm number.
        */
	status = E_DB_OK;
	number = 0;
	while (status == E_DB_OK)
	{
            status = qeu_get(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		error = qeu.error.err_code;
		break;
	    }
	    qeu.qeu_getnext = QEU_NOREPO;
            qeu.qeu_klen = 0;
	    if (atuple_temp.dba_alarmno > number)
                number = atuple_temp.dba_alarmno;
	}
	if (error != E_QE0015_NO_MORE_ROWS)
	    break;
 
	/* 
        ** We have to add 1 to derive the next alarm number.
	*/
	atuple->dba_alarmno=number+1;

	/* Save the alarm tuple */
	status = E_DB_OK;
		
	/* Insert single alarm tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_SECALARM);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
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

	/* Update all query text tuples with query id - validate list */
	next = qeuq_cb->qeuq_qry_tup;
	for (i = 0; i < qeuq_cb->qeuq_cq; i++)
	{
	    qtuple = (DB_IIQRYTEXT *)next->dt_data;
	    next = next->dt_next;
	    qtuple->dbq_txtid.db_qry_high_time = qryid.db_qry_high_time;
	    qtuple->dbq_txtid.db_qry_low_time  = qryid.db_qry_low_time;
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
	
	if (transtarted)		   /* If we started a transaction */
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}
    }
    while(loop);

    /* call qef_error to handle error messages */
    if(error)
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
  
    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	if(atuple->dba_objtype==DBOB_DATABASE)
	{
	    evtype=SXF_E_DATABASE;
	}
	else
	{
	    evtype=SXF_E_TABLE;
	}
	/*
	** Creating an alarm is a control operation on the base object
	** (database/table)
	*/
	if(status==E_DB_OK)
	    access=(SXF_A_CONTROL|SXF_A_SUCCESS);
	else
	    access=(SXF_A_CONTROL|SXF_A_FAIL);
	if(qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		    (char *)&atuple->dba_objname,
		    &objowner,
		    sizeof(atuple->dba_objname),
		    evtype,
		    I_SX202D_ALARM_CREATE,
		    access,
		    &e_error)!=E_DB_OK)
	{
		error = e_error.err_code;
		status=E_DB_ERROR;
	}
    }

    /*
    ** Log QEF create error
    */
    if(status!=E_DB_OK)
    {
    	(VOID) qef_error(E_QE028B_CREATE_ALARM_ERR, 0L, local_status, 
		     &error, &qeuq_cb->error, 0);
	qeuq_cb->error.err_code= E_QE0025_USER_ERROR;
    }
    return (status);
} /* qeu_csecalarm */

/*{
** Name: qeu_dsecalarm 	- Drop single or all alarm definition.
**
** External QEF call:   status = qef_call(QEU_DSECALM, &qeuq_cb);
**
** Description:
**	Drop the alarm tuple, the alarm text from the appropriate tables 
**	(iisecalarm, iiqrytext and update the table "relstat" field. An
**	alarm can be dropped either by name (when the DROP SECURITY_ALARM
**      statement is issued) or by object id (when its associated object 
**      is dropped).
**
**	When originating from the DROP SECURITY_ALARM  statement the single 
**	alarm tuple is deleted based on its name - provided in the
**	qeuq_uld_tup parameter.  In this case the single alarm is fetched
**	(to collect the text, table id) and deleted.  If it wasn't
**	found then an error is returned.  
**
**	When originating from a DROP TABLE statement (this is similar to
**	the DROP INTEGRITY ON table ALL), all tuples that satisfy the
**	table-id are fetched (to collect their text ids) and dropped.
**	If none are found no error is returned.  
**
**	For each alarm that is dropped the text tuples are
**	also dropped based on the ids collected from the fetched alarm.
**
**	The way this routine works is the following:
**
**	    set up QEU CB's for iisecalarm, iiqrytext and open tables;
**	    if (DROP SECURITY_ALARM) then
**		set up iisecalarm "qualified" name and owner;
**	    else
**		set up iisecalarm "keyed" on DROP object id;
**	    endif;
**	    for (all alarm tuples found) loop
**		get alarm tuple;
**		if (not found) then
**		    if (DROP SECURITY_ALARM) then
**			error - alarm does not exist;
**			break;
**		    else
**			clear error;
**			break;
**		    endif;
**		endif;
**		set up iiqrytext keys based on alarm text id;
**		for (all text tuples found) loop
**		    get text tuple;
**		    delete text tuple;
**		endloop;
**		delete current alarm tuple;
**		if (DROP SECURITY_ALARM) then
**		    endloop;
**		endif;
**	    endloop;
**	    close tables;
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_rtbl		Table id of alarm - if DROP TABLE.  If DROP
**				SECURITY_ALARM then this is ignored.
**	    .qeuq_cr		Must be 1 if DROP SECURITY_ALARM, 0 
**				if DROP TABLE.
**	    .qeuq_uld_tup	Alarm tuple to be deleted - if DROP 
**				SECURITY_ALARM, otherwise this is ignored:
**		.dba_alarmname	alarm name *or*
**		.dba_alarmno	Alarm number
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	from_drop_alarm		TRUE if called from DROP SECURITY_ALARM
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	26-nov-93 (robf)
**	    Written for Secure 2.0
**	30-dec-93 (robf)
**          Don't  write audit records if called implicitly during drop
**	    of a table.
**	16-feb-94 (robf)
**          Check DROP ALL flag from  qeuq_cb.qeuq_permit_mask & QEU_DROP_ALL,
**	    so we loop appropriately when there are several alarms on
**	    the object to be dropped.
*/
DB_STATUS
qeu_dsecalarm(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
bool		from_drop_alarm)
{
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;	    
    QEU_CB	    	aqeu;		/* For iisecalarm table */
    QEF_DATA	    	aqef_data;
    DB_SECALARM	    	*atuple_name;	/* Input tuple with name or id */
    DB_SECALARM	    	atuple;		/* Tuple currently being deleted */
    bool	    	alarm_opened = FALSE;
    DMR_ATTR_ENTRY  	akey_array[3];
    DMR_ATTR_ENTRY  	*akey_ptr_array[3];
    QEU_CB	    	qqeu;		/* For iiqrytext table */
    QEU_QUAL_PARAMS	qparams;
    DB_IIQRYTEXT	qtuple;
    QEF_DATA	    	qqef_data;
    bool	    	qtext_opened = FALSE;
    DMR_ATTR_ENTRY  	qkey_array[2];
    DMR_ATTR_ENTRY  	*qkey_ptr_array[2];
    bool		drop_specific_alarm;	/* TRUE if dropping a specific
						** alarm */
    char		obj_type;
    char		*alarm_name;
    DB_STATUS	    	status, local_status;
    i4	    	error;
    DB_ERROR 		e_error;
    SXF_ACCESS		access;
    SXF_EVENT		evtype;
    DB_OWN_NAME		objowner;

    for (;;)	/* Dummy for loop for error breaks */
    {
	MEfill(sizeof(objowner),' ',(PTR)&objowner);

	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_rtbl == NULL && qeuq_cb->qeuq_cr == 0)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	drop_specific_alarm = from_drop_alarm;

	/*
	** If passed DROP ALL then not dropping  specific alarm
	*/
	if(qeuq_cb->qeuq_permit_mask & QEU_DROP_ALL)
		drop_specific_alarm=FALSE;


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
	    tranqeu.qeu_mask = 0;
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


	/* Open iisecalarm, iiqrytext tables */
	aqeu.qeu_type = QEUCB_CB;
        aqeu.qeu_length = sizeof(QEUCB_CB);
        aqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        aqeu.qeu_lk_mode = DMT_IX;
        aqeu.qeu_flag = DMT_U_DIRECT;
        aqeu.qeu_access_mode = DMT_A_WRITE;
	aqeu.qeu_mask = 0;
	STRUCT_ASSIGN_MACRO(aqeu, qqeu);	/* Quick initialization */

	/* Open iisecalarm */
        aqeu.qeu_tab_id.db_tab_base = DM_B_IISECALARM_TAB_ID;    
        aqeu.qeu_tab_id.db_tab_index = DM_I_IISECALARM_TAB_ID; 	
	status = qeu_open(qef_cb, &aqeu);
	if (status != E_DB_OK)
	{
	    error = aqeu.error.err_code;
	    break;
	}
	alarm_opened = TRUE;

        qqeu.qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID; /* Open iiqrytext */
        qqeu.qeu_tab_id.db_tab_index = DM_I_QRYTEXT_TAB_ID; 	
	status = qeu_open(qef_cb, &qqeu);
	if (status != E_DB_OK)
	{
	    error = qqeu.error.err_code;
	    break;
	}
	qtext_opened = TRUE;

	aqeu.qeu_count = 1;	   	    /* Initialize alarm-specific qeu */
        aqeu.qeu_tup_length = sizeof(DB_SECALARM);
	aqeu.qeu_output = &aqef_data;
	aqef_data.dt_next = NULL;
        aqef_data.dt_size = sizeof(DB_SECALARM);
        aqef_data.dt_data = (PTR)&atuple;

	qqeu.qeu_count = 1;	   	    /* Initialize qtext-specific qeu */
        qqeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);
	qqeu.qeu_output = &qqef_data;
	qqef_data.dt_next = NULL;
        qqef_data.dt_size = sizeof(DB_IIQRYTEXT);
        qqef_data.dt_data = (PTR)&qtuple;

	/*
	** If DROP SECURITY_ALARM then position iisecalarm table based on 
	** alarm name or numbr
	** and drop the specific alarm (if we can't find it issue an error).
	** Positioning is done via qeu_qalarm_by_name/id.  
	** If DROP TABLE then fetch and drop ALL alarms applied to the table.
	*/
	aqeu.qeu_getnext = QEU_REPO;
	if (from_drop_alarm)		
	{
	    /*
	    ** Might be scanning for database/installation alarms
	    ** or special DROP ALL processing.
	    */
            atuple_name   = (DB_SECALARM *)qeuq_cb->qeuq_uld_tup->dt_data;
	    alarm_name=(char*)&atuple_name->dba_alarmname;

	    /*
	    ** alarmno -1 indicates all alarms when qualifying
	    */
	    if(qeuq_cb->qeuq_permit_mask & QEU_DROP_ALL)
		atuple_name->dba_alarmno= -1;
	    /* Get alarm that matches alarm name (qualified) */
	    qparams.qeu_qparms[0] = (PTR) atuple_name;
	    aqeu.qeu_klen = 0;				/* Not keyed */
	    aqeu.qeu_qual = qeu_qalarm_by_name;
	    aqeu.qeu_qarg = &qparams;
	}
	else				/* DROP TABLE */
	{
	    /* Get alarm that applies to specific table id (keyed) */
	    obj_type=DBOB_TABLE;
	    alarm_name="unknown";

	    aqeu.qeu_klen = 3;
	    aqeu.qeu_key = akey_ptr_array;
	    akey_ptr_array[0] = &akey_array[0];
	    akey_ptr_array[1] = &akey_array[1];
	    akey_ptr_array[2] = &akey_array[2];

	    akey_ptr_array[0]->attr_number = DM_1_IISECALARM_KEY;
	    akey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    akey_ptr_array[0]->attr_value = (char*)&obj_type;

	    akey_ptr_array[1]->attr_number = DM_2_IISECALARM_KEY;
	    akey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    akey_ptr_array[1]->attr_value =
				    (char *)&qeuq_cb->qeuq_rtbl->db_tab_base;
	    akey_ptr_array[2]->attr_number = DM_3_IISECALARM_KEY;
	    akey_ptr_array[2]->attr_operator = DMR_OP_EQ;
	    akey_ptr_array[2]->attr_value =
				    (char *)&qeuq_cb->qeuq_rtbl->db_tab_index;
	    aqeu.qeu_qual = NULL;
	    aqeu.qeu_qarg = NULL;
	}
	for (;;)		/* For all alarm tuples found */
	{
	    status = qeu_get(qef_cb, &aqeu);
	    if (status != E_DB_OK)
	    {
		if (aqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    if (drop_specific_alarm)
		    {
			(VOID)qef_error(E_US2473_9331_ALARM_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(atuple_name->dba_alarmname),
						alarm_name),
				    (PTR)alarm_name);
			error = E_QE0000_OK;
			status=E_DB_OK;
		    }
		    else	/* No [more] alarms for table */
		    {
			error = E_QE0000_OK;
			status = E_DB_OK;
		    }
		}
		else	/* Other error */
		{
		    error = aqeu.error.err_code;
		}
		break;
	    } /* If no alarms found */
	    aqeu.qeu_getnext = QEU_NOREPO;
            aqeu.qeu_klen = 0;
	    
	    status = E_DB_OK;

            /* Postion iiqrytxt table for query id associated with this alarm */
	    qkey_ptr_array[0] = &qkey_array[0];
	    qkey_ptr_array[1] = &qkey_array[1];
	    qqeu.qeu_getnext = QEU_REPO;
	    qqeu.qeu_klen = 2;       
	    qqeu.qeu_key = qkey_ptr_array;
	    qkey_ptr_array[0]->attr_number = DM_1_QRYTEXT_KEY;
	    qkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    qkey_ptr_array[0]->attr_value =
				(char *)&atuple.dba_txtid.db_qry_high_time;
	    qkey_ptr_array[1]->attr_number = DM_2_QRYTEXT_KEY;
	    qkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    qkey_ptr_array[1]->attr_value =
				(char *)&atuple.dba_txtid.db_qry_low_time;
	    qqeu.qeu_qual = NULL;
	    qqeu.qeu_qarg = NULL;
	    for (;;)		/* For all query text tuples */
	    {
		status = qeu_get(qef_cb, &qqeu);
		if (status != E_DB_OK)
		{
		    error = qqeu.error.err_code;
		    break;
		}
		qqeu.qeu_klen = 0;
		qqeu.qeu_getnext = QEU_NOREPO;
		status = qeu_delete(qef_cb, &qqeu);
		if (status != E_DB_OK)
		{
		    error = qqeu.error.err_code;
		    break;
		}
	    } /* For all query text tuples */
	    if (error != E_QE0015_NO_MORE_ROWS)
		break;

	    if(atuple.dba_objtype==DBOB_TABLE)
	    {
    		DMT_CHAR_ENTRY  	char_spec;	
    		DMT_CB	    	dmt_cb;
	    	/* 
            	** Alter the table relstat to flag alarms. This 
		** validates that the table exists and ensures that we 
		** get an exclusive lock on it.
           	*/
	    	char_spec.char_id = DMT_C_ALARM;    
	    	char_spec.char_value = DMT_C_ON;
		MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	    	dmt_cb.dmt_flags_mask = 0;
	    	dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	    	dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	    	dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
	    	dmt_cb.length = sizeof(DMT_CB);
	    	dmt_cb.type = DMT_TABLE_CB;
	    	dmt_cb.dmt_id.db_tab_base  = atuple.dba_objid.db_tab_base;
	    	dmt_cb.dmt_id.db_tab_index = atuple.dba_objid.db_tab_index;
	    	dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
	    	status = dmf_call(DMT_ALTER, &dmt_cb);
	    	if (status != E_DB_OK)
    	    	{
		    error = dmt_cb.error.err_code;
		    if (error == E_DM0054_NONEXISTENT_TABLE)
		    {
			error = E_QE0025_USER_ERROR;
			status = E_DB_OK;	/* Still remove the alarm */
		    }
		    else
		    {
			/* 
			** Some other error
			*/
	        	break;
		    }
	    	}
	     }
	     else if(!(atuple.dba_flags&DBA_ALL_DBS))
	     {
		/*
		** Check access to the  database, also get the security label
		** for later use
		*/
	    	status=qeu_db_exists(qef_cb, qeuq_cb,
		    (DB_DB_NAME *)&atuple.dba_objname, SXF_A_CONTROL, &objowner);
	    	if(status==E_DB_ERROR)
	    	{
			/* Error, no access to database */
			break;
	    	}
	    	else  if  (status==E_DB_WARN)
	    	{
			/* Database not found */
			status=E_DB_ERROR;
			/* E_US2474_9332_ALARM_NO_DB */
	        	(VOID)qef_error(9332, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(atuple.dba_objname),
					  (char *)&atuple.dba_objname),
			    (PTR)&atuple.dba_objname);
			break;
		}
	    }
	    /* Now delete the current alarm tuple */
	    status = qeu_delete(qef_cb, &aqeu);
	    if (status != E_DB_OK)
	    {
		error = aqeu.error.err_code;
		break;
	    }
		
	    /*
	    ** If  doing a specific DROP SECURITY_ALARM we're done with this 
	    ** loop. 
	    ** Otherwise continue with next alarm applied to table.
	    */
	    if (drop_specific_alarm)
		break;
	} /* For all alarm found */

	if (status != E_DB_OK)
	    break;

	break;
    } /* End dummy for loop */

    /* Handle any error messages */
    if (status != E_DB_OK)
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off all the tables and transaction */
    if (alarm_opened)
    {
	local_status = qeu_close(qef_cb, &aqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(aqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (qtext_opened)
    {
	local_status = qeu_close(qef_cb, &qqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qqeu.error.err_code, 0L, local_status, 
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
    /*
    ** Dropping an alarm is a control operation on the base object
    ** (database/table).
    */
    if( from_drop_alarm==TRUE && Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
        if(atuple.dba_objtype==DBOB_DATABASE)
        {
	    evtype=SXF_E_DATABASE;
	    /*
	    ** Database label/owner should be set in db_exists request above
	    */
        }
        else
        {
	    evtype=SXF_E_TABLE;
        }
        if(status==E_DB_OK)
	    access=(SXF_A_CONTROL|SXF_A_SUCCESS);
        else
	    access=(SXF_A_CONTROL|SXF_A_FAIL);
        if(qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		(char *)&atuple.dba_objname,
		&objowner,
		sizeof(atuple.dba_objname), 
		evtype,
		I_SX202E_ALARM_DROP,
		access,
		&e_error)!=E_DB_OK)
        {
	    error = e_error.err_code;
	    status=E_DB_ERROR;
        }
    }
    if(status!=E_DB_OK)
    {
    	(VOID) qef_error(E_QE028C_DROP_ALARM_ERR, 0L, local_status, 
		     &error, &qeuq_cb->error, 0);
	qeuq_cb->error.err_code= E_QE0025_USER_ERROR;
    }
    return (status);
} /* qeu_dsecalarm */

/*{
** Name: qeu_qalarm_by_name - Qualify a alarm by alarm name and/or number
**
** Description:
**	This routine is provided to allow iisecalarm to be accessed by 
**	name and/or number.
**	For extraction performance reasons iisecalarm is hashed on object ids 
**	but various operations require that we look up the alarm by name
**	(alarm deletion and insertion).  This routine is called indirectly
**	from within DMF and passed the original alarm tuple we are searching
**	for and the current (next) tuple.
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info
**	    .qeu_qparms[0]	Name-qualified tuple we're looking for:
**		.dba_alarmname	Alarm name.
**		.dba_alarmno	Alarm number.
**	    .qeu_rowaddr	Current row we're qualifying
**	
** Outputs:
**	qparams.qeu_retval	Set to ADE_TRUE if qualifies, else ADE_NOT_TRUE
**
**	Returns E_DB_OK or E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**	26-nov-93 (robf)
**	    Written for Secure 2.0
**	24-jan-95 (stephenb)
**	    We need to check that we are refferencing the correct table before
**	    allowing user to drop alarm by name
**      03-mar-95 (sarjo01)
**         Bug no. 67035: for DROP SECURITY_ALARM ON CURRENT INSTALLATION,
**         return tuple as qualified to drop if objname is null and
**         objtype == 'D'
**      20-mar-95 (inkdo01)
**         Bug 67526: DROP SECURITY_ALARM on named alarm for database/
**         current installation doesn't assure that alarm is for THAT
**         database or current installation
**	11-Apr-2008 (kschendel)
**	    New qual-function call sequence.
*/

static DB_STATUS
qeu_qalarm_by_name(
	void *toss, QEU_QUAL_PARAMS *qparams)
{
    DB_SECALARM *search_tup;
    DB_SECALARM *cur_tup;

    search_tup = (DB_SECALARM *) qparams->qeu_qparms[0];
    cur_tup = (DB_SECALARM *) qparams->qeu_rowaddr;
    qparams->qeu_retval = ADE_NOT_TRUE;
    if (search_tup == NULL || cur_tup == NULL)	/* Consistency check */
    {
	return (E_DB_ERROR);
    }

    if (*search_tup->dba_objname.db_name == '\000' &&
        search_tup->dba_objtype == 'D')
    {
	qparams->qeu_retval = ADE_TRUE;
        return (E_DB_OK);                  /* It's ours */
    }

    /*
    ** If user specified a number, then look for that.
    ** Note that alarm -1 indicates all alarms for that object
    */
    if(((search_tup->dba_alarmno>0 && 
       search_tup->dba_alarmno==cur_tup->dba_alarmno) ||
	 search_tup->dba_alarmno==-1) &&
       search_tup->dba_objid.db_tab_base==cur_tup->dba_objid.db_tab_base &&
       search_tup->dba_objid.db_tab_index==cur_tup->dba_objid.db_tab_index &&
       search_tup->dba_objtype==cur_tup->dba_objtype &&
       search_tup->dba_objtype==DBOB_TABLE
       )
    {
	qparams->qeu_retval = ADE_TRUE;
	return (E_DB_OK);			/* It's ours */
    }
    /*
    ** Check for databases, which all have the same object id so
    ** we have to check by name instead
    */
    if(((search_tup->dba_alarmno>0 && 
       search_tup->dba_alarmno==cur_tup->dba_alarmno) ||
	 search_tup->dba_alarmno==-1) &&
       !MEcmp((PTR)&search_tup->dba_objname,(PTR)&cur_tup->dba_objname,
		sizeof(cur_tup->dba_objname)) &&
       search_tup->dba_objtype==cur_tup->dba_objtype &&
       search_tup->dba_objtype==DBOB_DATABASE
       )
    {
	qparams->qeu_retval = ADE_TRUE;
	return (E_DB_OK);			/* It's ours */
    }
    /*
    ** Check by name
    */
    if (MEcmp((PTR)&search_tup->dba_alarmname, 
		   (PTR)&cur_tup->dba_alarmname,
		  sizeof(cur_tup->dba_alarmname)) == 0 &&
       search_tup->dba_objtype==cur_tup->dba_objtype &&
       (search_tup->dba_objtype == DBOB_TABLE &&
       search_tup->dba_objid.db_tab_base == cur_tup->dba_objid.db_tab_base &&
       search_tup->dba_objid.db_tab_index == cur_tup->dba_objid.db_tab_index ||
       search_tup->dba_objtype == DBOB_DATABASE &&
       !(MEcmp((PTR)&search_tup->dba_objname,(PTR)&cur_tup->dba_objname,
                 sizeof(cur_tup->dba_objname))))
       )
    {
	qparams->qeu_retval = ADE_TRUE;
    }
    return (E_DB_OK);
} /* qeu_qalarm_by_name */

/*
** Name: qeu_gen_alarm_name
**
** Description:
**	Generates a default alarm name when none has been specified
**
**	Name will consist of a $ followed by first 5 chars of the object name
**	(blank padded if necessary), followed by _X_ where X is D or T 
**	followed by the hex representation of the alarm id.  Before
**	returning to the caller, we will call cui_idxlate() to convert name to
**	the case appropriate for the database to which we are connected.
**
** Inputs:
**	obj_name - Object name
**
**	obj_type - Object type
**
**	qef_rcb	 - RCB
**
**	alarm_name - Where to put result.
**
**	alarm_id   - Unique id for number.
**	
** Outputs:
**
**	alarm_name  - Name
** 
** History:
**	26-nov-93 (robf)
**	   Created
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning fixes, CMcpychar not to be used with single letter
**	    ascii constants.  (array out of bounds warnings.)
*/
static DB_STATUS
qeu_gen_alarm_name(
    i4                   obj_type,
    char                 *obj_name,
    QEF_RCB		 *qef_rcb,
    DB_TAB_ID		 *alarm_id,
    char		 *alarm_name,
    i4		 *err_code
)
{
    DB_STATUS	status = E_DB_OK;
    QEF_CB      *qef_cb = qef_rcb->qef_cb;
    u_i4	cui_flags =
		    ( * ( qef_cb->qef_dbxlate ) ) | CUI_ID_DLM | CUI_ID_NORM;
    u_i4   ret_mode;
    u_i4	len_untrans, len_trans = DB_ALARM_MAXNAME;
    u_char	untrans_str[DB_ALARM_MAXNAME];
    i4		j;
    char	*p = (char *) untrans_str;
    char	*limit;
    char	id_str[20], *id_p = id_str;
    DB_ERROR	err_blk;

    *p++ = '$';
    for (j = 0; j < 5; j++)
	CMcpyinc(obj_name, p);
    *p++ = '_';
    if(obj_type==DBOB_TABLE)
	*p++ = 'T';
    else if(obj_type==DBOB_DATABASE)
	*p++ = 'D';
    else
    {
	/* Unknown object type */
	*err_code=E_QE0018_BAD_PARAM_IN_CB;
	return E_DB_ERROR;
    }
    *p++ = '_';

    /* convert alarm id into a hex string */
    seedToDigits(alarm_id, id_str, qef_rcb );

    /* append id string to the name built so far */
    for (; *id_p != EOS; CMcpyinc(id_p, p))
    ;

    /* remember length of the name */
    len_untrans = p - (char *) untrans_str;

    status = cui_idxlate(untrans_str, &len_untrans, (u_char *) alarm_name,
	&len_trans, cui_flags, &ret_mode, &qef_rcb->error);
    if (DB_FAILURE_MACRO(status))
    {
	/*
	** cui_idxlate errors have two params currently.
	** We report here to avoid errors in generic code which
	** doens't have params.
	*/
        (VOID) qef_error(qef_rcb->error.err_code, 0L, status, 
			     err_code, &err_blk, 2,
			     sizeof(ERx("System generated alarm name"))-1,
			     ERx("System generated alarm name"),
			     len_untrans,
			     untrans_str);
	/* Assume passed bad input */
	*err_code=E_QE0018_BAD_PARAM_IN_CB;
	return status;
    }

    /* blank pad */
    p = alarm_name + len_trans;
    limit = alarm_name + DB_ALARM_MAXNAME;
    while (p < limit)
	*p++ = ' ';

    return(E_DB_OK);
}	

/*{
** Name: seedToDigits - turn a DB_TAB_ID into a hex number
**
** Description:
**	This routine converts a DB_TAB_ID into a hex number.  This is called
**	by the alarm name builder.
**
**
** Inputs:
**	seed		number used to construct name
**
** Outputs:
**	digitString	where to write the string of hex digits
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	2-dec-93 (robf)
**	    Cloned from qea/qeaddl.c
**
*/

#define	DIGITS_IN_I4	8
#define	STRING_LENGTH	( 2 * DIGITS_IN_I4 )

static VOID
seedToDigits(
    DB_TAB_ID	*seed,
    char	*digitString,
    QEF_RCB	*qef_rcb
)
{
    i4		leadingZeroes;
    i4		firstStringLength;
    i4		secondStringLength;
    char	firstString[ DIGITS_IN_I4 + 1 ];
    char	secondString[ DIGITS_IN_I4 + 1 ];

    CVlx( seed->db_tab_base, firstString );
    firstString[ DIGITS_IN_I4 ] = 0;	/* just in case */
    firstStringLength = STlength( firstString );

    CVlx( seed->db_tab_index, secondString );
    secondString[ DIGITS_IN_I4 ] = 0;	/* just in case */
    secondStringLength = STlength( secondString );

    leadingZeroes = DIGITS_IN_I4 - firstStringLength;
    STmove( "", '0', leadingZeroes, digitString );
    STcopy( firstString, &digitString[ leadingZeroes ] );

    leadingZeroes = DIGITS_IN_I4 - secondStringLength;
    STmove( "", '0', leadingZeroes, &digitString[ DIGITS_IN_I4 ] );
    STcopy( secondString, &digitString[ DIGITS_IN_I4 + leadingZeroes ] );
}

/*
** Name: qeu_db_exists
**
** Description:
**	Checks whether a particular database exists, including MAC
**	access where appropriate. Denied access due to MAC will be
**	audited here, and a no-such-database error returned to the
**	caller. Successfull access should be audited by the caller.
**
** Inputs:
**	dbname	- Database name to check
**
**	access  - SXF-access on the database
**
** Outputs:
**	dbowner - Owner of database
**
** Returns:
**	E_DB_OK - Exists
**	E_DB_WARN - Doesn't exists
**	(error)   - Something went wrong
**
** History:
**	3-dec-93 (robf)
**	   Created
*/
DB_STATUS
qeu_db_exists(QEF_CB * qef_cb, QEUQ_CB *qeuq_cb, DB_DB_NAME *dbname, 
	i4 access, 
	DB_OWN_NAME *dbowner)
{
    DB_STATUS 	status=E_DB_OK;
    bool      	tbl_opened=FALSE;
    bool      	loop=FALSE;
    DU_DATABASE	dbtuple;
    bool	not_found=TRUE;
    i4 	error=0;
    QEU_CB	    qeu;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];
    DB_ERROR	    err_blk;

    do
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
	qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_flag = 0;
	qeu.qeu_mask = 0; 

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
		error = E_QE022D_IIDATABASE;
		status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		break;
	}
	tbl_opened = TRUE;

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_DATABASE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_DATABASE);
	qef_data.dt_data = (PTR)&dbtuple;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;
	key_ptr_array[0]= &key_array[0];
	qeu.qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_1_DATABASE_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) dbname;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;

	status = qeu_get(qef_cb, &qeu);
	if (((status != E_DB_OK) &&
		 (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)) ||
		((status == E_DB_OK) &&
		 (qeu.qeu_count != 1))
	   )
	{
		error = E_QE022D_IIDATABASE;
		status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		break;
	}

	if (status != E_DB_OK)
	{
		not_found=TRUE;
		status=E_DB_OK;
	}
	else
	{
		not_found=FALSE;
	}
		
	status = qeu_close(qef_cb, &qeu);
	tbl_opened = FALSE;
	if (status != E_DB_OK)
	{
		error = E_QE022D_IIDATABASE;
		status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		break;
	}
	/*
	** Check MAC access to the database, if  we found it.
	*/
	if(not_found==TRUE)
		break;

	 if(dbowner)
		STRUCT_ASSIGN_MACRO(dbtuple.du_own, *dbowner);
    } while(loop);

    if(tbl_opened)
    {
	(VOID) qeu_close(qef_cb, &qeu);
    }
    if(not_found && status==E_DB_OK)
	status=E_DB_WARN;
    if(error)
    {
	    (VOID) qef_error(error, 0L, status, 
			     &error, &qeuq_cb->error, 0);
    }
    return status;
}

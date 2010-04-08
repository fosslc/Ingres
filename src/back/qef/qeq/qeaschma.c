/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <cs.h>
#include    <lk.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>	
#include    <qsf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>

#include    <dmmcb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/*
**
**  Name: QEASCHMA.C - schema creating routine
**
** Description:
**	The routine in this module creates both implicitly and explicitly
** 	specified schemas.
**
**	qea_schema() - create a schema	
**
**  History:
**	30-nov-92 (anitap)
**	    Written.
**	20-dec-92 (anitap)
**	    Changed qea_cschema() to enable the creation of implicit schemas.
**	18-jan-93 (rickh)
**	    Use SET_SCHEMA_ID macro.
**	21-jan-93 (anitap)
**	    Fixed acc warnings in qea_cschema().
**	04-mar-93 (anitap)
**	    Changed qea_cschema() to qea_schema() to conform to standards.
**	    Do not look up iischema catalog for creation of implicit schema
**	    for objects owned by '$ingres'.
**	26-apr-93 (anitap)
**	    Added support for creating implicit for CREATE TABLE.
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use qef_cat_owner instead of "$ingres" in qea_schema()
**	07-jun-93 (anitap)
**	    If qeu_get returns any other error than "NO_MORE_ROWS", we need to
**	    break out of the qea_schema() routine.
**	    To reduce deadlocks, we will take a read lock on iischema and only
**	    escalate to write/exclusive lock if we need to add a schema.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (anitap)
**	    Call qea_reserveID() to assign schema id.	
**	    Take read lock on iischema catalog only in case of inplicit schema
**	    creation to avoid possible deadlock during explicit creation while
**	    escalating to write lock.
**	    Assign err at the beginning according to implicit/explicit schema
**	    creation.
**      13-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	1-oct-93 (stephenb)
**	    CREATE SCHEMA is an auditable event, added calls to qeu_secaudit().
**	11-oct-93 (rickh)
**	    When opening IISCHEMA for scanning, ask for page rather than table
**	    locks.  The shared table level lock was locking out other schema
**	    creators.
**	7-jan-94 (robf)
**          Add new parameters to qeu_secaudit() calls.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**      07-mar-2000 (gupsh01)
**          qea_schema now does not handle error E_DM006A_TRAN_ACCESS_CONFLICT, 
**          as it is being handled by the calling facility. (BUG 100777).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to qef_error() call
**          in qea_schema() routine.
*/

/*{
** Name: QEA_SCHEMA		- create a schema 
**
** Description:
**	qea_schema() will create the schema in iischema catalog. It will do
**	qeu_open(), qeu_get(), qeu_append() and qeu_close(). Both implicit 
**	and explicit declared schemas will be created. qea_schema() will 
**	check if the flag is set which implies implicit creation of 
**	schema (for 6.5). When the rest of the DDL statements are also put into
**	query plans in post 6.5, there will always be a QEA_CREATE_SCHEMA 
**	action header. 
**
** Inputs:
**	qef_schname	   	Schema data:
**	    .db_schname		Schema name	
**	    .db_schowner	Schema owner
**	cb			Pointer to QEF control block. Contains other 
**				CBs through which SCF can be accessed.
**	qef_cb			QEF control block
**	flag			indicates implicit or explicit creation of 
**				schema
**
** Outputs:
**	qef_rcb
**	    .error.err_code	one of the following:
**			        E_QE0000_OK
**                              E_QE0025_USER_ERROR - on all user errors
**				E_QE0302_SCHEMA_EXISTS - on duplicate 
**				 explicit declared schema insertion.
**	or
**	
**	qeuq_cb
**	    .error.err_code	one of the following:
**				E_QE0000_OK
**				E_QE0025_USER_ERROR - on all user errors
**				E_QE0302_SCHEMA_EXISTS - on duplicate
**				explicit declared schema insertion.
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	Schema name, schema owner and schema id will be entered into 
**	iischema catalog.
**
** History:
**	30-nov-92 (anitap)
**	    Written for CREATE SCHEMA project.
**	17-dec-92 (anitap)
**	    Modified to enable the creation of implicit schemas.
**	18-jan-93 (rickh)
**	    Use SET_SCHEMA_ID macro.
**	21-jan-93 (anitap)
**	    Fixed acc warnings.
**	04-mar-93 (anitap)
**	    Changed qea_cschema() to qea_schema() so that the first 7 letters
**	    are unique. Also changed (VOID) to (void). Do not look up iischema
**	    catalog for objects owned by $ingres.
**	23-apr-93 (anitap)
**	  Added support for creation of implicit schema for tables and some 
**	  error handling.
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use qef_cat_owner instead of "$ingres" in qea_schema().
** 	13-may-93 (anitap)
**	    Changed above so that qef_cat_owner is accessed directly through
**	    qef_cb. Change required for implicit schema creation.
**	07-jun-93 (anitap)
**	    If qeu_get() gets any other error than "NO_MORE_ROWS", break out
**	    the routine. Also removed define for ingres_name.
**	    To reduce deadlocks on iischema catalog, we will take a read lock
**	    and only escalate to write lock if we need to update iischema
**	    catalog.
**	07-jul-93 (anitap)
**	    use qea_reserveID() to assign table id for schema. 	
**	    Take read lock only for implicit schema creation to avoid deadlocks
**	    while escalating to write lock.
**	    Assign err at the beginning according to implicit/explicit schema
**	    creation.
**	19-aug-93 (anitap)
**	    Assign database id to qef_cb->qef_dbid.
**	    Removed redundant lines of code due to mis-integration.
**	1-oct-93 (stephenb)
**	    CREATE SCHEMA is an auditable event, added calls to qeu_secaudit().
**	11-oct-93 (rickh)
**	    When opening IISCHEMA for scanning, ask for page rather than table
**	    locks.  The shared table level lock was locking out other schema
**	    creators.
**      07-mar-2000 (gupsh01)
**          qea_schema now does not handle error E_DM006A_TRAN_ACCESS_CONFLICT, 
**          as it is being handled by the calling facility. (BUG 100777).
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	21-apr-04 (inkdo01)
**	    Revert to qef_rcb->error - qeu calls qea_schema() and has no DSH.
*/

DB_STATUS
qea_schema(
	QEF_RCB         *qef_rcb,
	QEUQ_CB		*qeuq_cb,
	QEF_CB		*qef_cb,
	DB_SCHEMA_NAME	*qef_schname,
	i4		flag)	
{
    bool		tbl_opened = FALSE;
    i4		error;
    DB_STATUS   	status = E_DB_OK;
    DB_STATUS 		local_status;
    QEU_CB		qeu;
    QEF_DATA		qef_data;
    DB_SCHEMA_NAME	*schname;
    DB_OWN_NAME		*schowner;
    DB_IISCHEMA 	schtuple;	/* New schema tuple */
    DB_IISCHEMA 	schtuple_temp;	/* Tuple  and */
    DMR_ATTR_ENTRY 	schkey_array[1]; /* keys for uniqueness check */
    DMR_ATTR_ENTRY 	*schkey_ptr_array[1];
    DB_ERROR		err;

    for (;;) 	/* Dummy for loop for error breaks */
    {

    /* we do not want to create implicit schema for objects owned
    ** by $ingres. This is because the iischema catalog itself has
    ** columns defined to be NOT NULL. This definition needs to go into
    ** the iiintegrity catalog along with schema name. Since iischema
    ** table itself would not have been created, there is no way to get
    ** the schema name. A canonical schema is created for $ingres.
    */

    if ( MEcmp(	(PTR)qef_schname, (PTR)qef_cb->qef_cat_owner,
		sizeof(DB_SCHEMA_NAME)) == 0)
	break;

    STRUCT_ASSIGN_MACRO(*qef_schname, schtuple.db_schname);
    MEcopy((PTR)qef_schname, sizeof(schtuple.db_schowner), 
			(PTR)&schtuple.db_schowner);

    schname = qef_schname;
    schowner = (DB_OWN_NAME *) qef_schname;

    /* validate that the schema name/owner is unique */
    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_tab_id.db_tab_base  = DM_B_IISCHEMA_TAB_ID;
    qeu.qeu_tab_id.db_tab_index  = DM_I_IISCHEMA_TAB_ID;	

    /* check for implicit schema */
    /* set up err at the beginning */

    if (flag == IMPLICIT_SCHEMA) 
    {
       qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
       qef_cb->qef_dbid = qeuq_cb->qeuq_db_id;	
       err = qeuq_cb->error;
    }
    else 
    {
       qeu.qeu_db_id = qef_rcb->qef_db_id;
       qef_cb->qef_dbid = qef_rcb->qef_db_id;
       err = qef_rcb->error;
    }	

    /* Take read lock only if implicit schema.
    ** We do not want to deadlock trying to escalate to write lock
    ** in case of explicit schema creation.
    */
    if (flag == IMPLICIT_SCHEMA || flag == IMPSCH_FOR_TABLE)
    {		   
       qeu.qeu_lk_mode = DMT_IS;
       qeu.qeu_flag = 0;
       qeu.qeu_access_mode = DMT_A_READ; 
    }
    else
    {
       qeu.qeu_lk_mode = DMT_IX;
       qeu.qeu_flag = DMT_U_DIRECT;
       qeu.qeu_access_mode = DMT_A_WRITE;	 
    }
    
    qeu.qeu_mask = 0;
    status = qeu_open(qef_cb, &qeu);
    if (status != E_DB_OK)
    {
       error = qeu.error.err_code;
       status = E_DB_ERROR;	
       break;
    }
    tbl_opened = TRUE;

    /* Retrieve the same named/owned schema - if not there then ok */
    qeu.qeu_count = 1;
    qeu.qeu_tup_length = sizeof(DB_IISCHEMA);
    qeu.qeu_output = &qef_data;
    qef_data.dt_next = NULL;
    qef_data.dt_size = sizeof(DB_IISCHEMA);
    qef_data.dt_data = (PTR)&schtuple_temp;
    qeu.qeu_getnext = QEU_REPO;
    qeu.qeu_klen = 1;			/* keyed on name */
    qeu.qeu_key = schkey_ptr_array;
    schkey_ptr_array[0] = &schkey_array[0];
    schkey_ptr_array[0]->attr_number = DM_1_IISCHEMA_KEY;
    schkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
    schkey_ptr_array[0]->attr_value = (char *)schname;
    qeu.qeu_qual = NULL;
    qeu.qeu_qarg = NULL;
    status = qeu_get(qef_cb, &qeu);

    /* Should not return error if implicit schema. */	

    if (status == E_DB_OK)          /* Found the same schema! */
    {

       /* NOTE: we do not want to return an error to the user if
       ** we are creating an implicit schema. The user did not explicitly
       ** request for the creation of the schema.
       ** In post 6.5, when we support most of the DDL statements in
       ** query plans, we will not use the flag field. QEF_CREATE_SCHEMA
       ** action wil have a field stating whether the schema we are
       ** creating is an explicit/implicit schema.
       */

       if (flag == IMPLICIT_SCHEMA || flag == IMPSCH_FOR_TABLE)
       {
          break;
       }
       else
       {
          (void)qef_error(E_QE0302_SCHEMA_EXISTS, 0L, E_DB_ERROR,
		&error, &qef_rcb->error, 1,
		qec_trimwhite(sizeof(*schname),
			(char *)schname),
			(PTR)schname);
           error = E_QE0025_USER_ERROR;
	   status = E_DB_ERROR;	
	   break;
       }
    }

    if (qeu.error.err_code !=  E_QE0015_NO_MORE_ROWS)
    {
       error = qeu.error.err_code;  /* Some other error */
       break;
    }

    status = E_DB_OK;

    if (flag == IMPLICIT_SCHEMA || flag == IMPSCH_FOR_TABLE)
    {
       /* We need to update iischema catalog. So let us escalate to
       ** Write/Exclusive lock after closing iischema which was opened 
       ** in read mode.
       */

       status = qeu_close(qef_cb, &qeu);
       if (status != E_DB_OK)
       {
          error = qeu.error.err_code;
          status = E_DB_ERROR;
          break;
       }
       tbl_opened = FALSE;

       /* The schema is unique - append it */
       /* Escalate to exclusive/write lock. */	

       qeu.qeu_lk_mode = DMT_IX;
       qeu.qeu_flag = DMT_U_DIRECT;
       qeu.qeu_access_mode = DMT_A_WRITE;
  
       status = qeu_open(qef_cb, &qeu);
       if (status != E_DB_OK)
       {
          error = qeu.error.err_code;
          status = E_DB_ERROR;	
          break;
       }
       tbl_opened = TRUE;

    } /* endif implicit schema creation and we escalated to write lock */

    /*
    ** Get unique schema id (a table id) from DMF. This id enables
    ** constraints to be associated with the schema.
    */
    status = qea_reserveID(&schtuple.db_schid, qef_cb, &err);
    if (status != E_DB_OK)
    {
       status = E_DB_ERROR;	
       break;
    }

    /* Insert single schema tuple */
    qeu.qeu_count = 1;
    qeu.qeu_tup_length = sizeof(DB_IISCHEMA);
    qeu.qeu_input = &qef_data;
    qef_data.dt_next = 0;
    qef_data.dt_size = sizeof(DB_IISCHEMA);
    qef_data.dt_data = (PTR)&schtuple;
    status = qeu_append(qef_cb, &qeu);
    if (status != E_DB_OK)
    {
       error = qeu.error.err_code;
       status = E_DB_ERROR;	
       break;
    }
    status = qeu_close(qef_cb, &qeu);
    if (status != E_DB_OK)
    {
       error = qeu.error.err_code;
       status = E_DB_ERROR;
       break;
    }
    tbl_opened = FALSE;

    /*
    ** Audit successful creation of schema
    */
    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	DB_ERROR	e_error;

    	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
	    schtuple.db_schname.db_schema_name,
	    &schtuple.db_schowner, sizeof(schtuple.db_schname.db_schema_name),
	    SXF_E_SECURITY, I_SX203E_SCHEMA_CREATE, 
	    SXF_A_SUCCESS | SXF_A_CREATE, &e_error);
	if (status != E_DB_OK)
	{
	    error = e_error.err_code;
	    break;
	}
    }

    err.err_code = E_QE0000_OK;
    return (E_DB_OK);

  } /* End dummy for loop */ 

  /* call qef_error() to handle error messages */
  if (status != E_DB_OK)
  {
     /*    
     ** Do not handle error E_DM006A_TRAN_ACCESS_CONFLICT here as it will
     ** be handled by the calling facility. (BUG 100777).
     */
     if (qeu.error.err_code == E_DM006A_TRAN_ACCESS_CONFLICT)
     {
       qef_rcb->error.err_code = qeu.error.err_code;
       qef_rcb->error.err_data = qeu.error.err_data;
     }
     else
	(void) qef_error(error, 0L, status, &error, &err, 0);
  }

  /* close off the table */ 
  local_status = E_DB_OK;
  if (tbl_opened)
  {
     local_status = qeu_close(qef_cb, &qeu);	      /* Close system table */
     if (local_status != E_DB_OK)
     {
	qef_error(qeu.error.err_code, 0L, local_status, &error, 
			&err, 0);	 	
	if (local_status > status)
	   status = local_status;
     } /* endif local_status not OK */
   } /* endif table opened */	

  return (status);
} /* qea_schema */ 

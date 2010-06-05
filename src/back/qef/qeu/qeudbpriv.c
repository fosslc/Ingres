/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <cui.h>
#include    <er.h>
#include    <tm.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

#include    <ade.h>
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
/**
**
**  Name: QEUDBPRIV.C - routines for maintaining database privileges
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iidbpriv catalog.
**	The operations supported include GRANT and REVOKE for
**	database privileges.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_dbpriv	- manipulates the iidbpriv catalog for qrymod
**	    qeu_pdbpriv	- purges grantee tuples in iidbpriv for QEU routines
**
**
**  History:
**	17-may-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Created.
**	06-jun-89 (ralph)
**	    Corrected minor unix portability problems
**      29-jun-89 (jennifer)
**          Added security auditing.
**	06-sep-89 (ralph)
**	    Add support for GRANT/REVOKE ON ALL DATABASES
**      11-apr-90 (jennifer)
**          bug 20787, 20788 Fix audit message output.
**      13-apr-90 (jennifer)
**          bug 20788, Fixed audit message description so that the
**          message parameter always is initialized.
**	16-apr-90 (ralph)
**	    Don't allow session to issue GRANT/REVOKE ON DATABASE
**          if it only has UPDATE_SYSCAT privileges.
**	08-aug-90 (ralph)
**          Don't allow session to issue GRANT/REVOKE ON DATABASE
**          if it only has UPDATE_SYSCAT privileges.
**	    Add support for GRANT/REVOKE ALL [PRIVILEGES] ON DATABASE....
**	    Audit GRANT/REVOKE failures due to security violation (bug 20790)
**	    Add audit description for [NO]DB_ADMIN
**	25-sep-90 (ralph)
**	    Audit implicit dbpriv revokes
**	05-dec-90 (ralph)
**	    Audit GRANT/REVOKE LOCKMODE (bug 34532)
**	28-dec-90 (ralph)
**	    Add support for GRANT/REVOKE ON INSTALLATION
**	    Add support for GRANT/REVOKE ALL ON DATABASE/INSTALLATION
**	15-jan-91 (ralph)
**	    Be sure to remove tuple from iidbpriv if REVOKE ALL specified.
**	    Issue E_QE0229_DBPRIV_NOT_GRANTED instead of 
**		  E_QE021D_DBPRIV_NOT_GRANTED when ON INSTALLATION specified.
**	02-feb-91 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	04-oct-92 (ed)
**	    remove DB_MAXNAME dependency
**	27-nov-1992 (pholman)
**	    Replace obsolete qeu_audit call with qeu_secaudit call.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	20-oct-93 (stephenb)
**	    GRANT/REVOKE on a database is a control action not create/drop 
**	    action, altered call to qeu_secaudit to fix this.
**	31-mar-94 (rblumer)       in qeu_dbpriv,
**	    Changed error handling to correctly handle blanks in delimited ids,
**	    by using cus_trmwhite instead of STindex to trim trailing blanks.
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
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
**	11-Apr-2008 (kschendel)
**	    Update qualification-function call.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Definition of static variables and forward static functions 
*/

    FUNC_EXTERN DB_STATUS scf_call();

static DB_STATUS
qeu_qdbpriv(
	void		*toss,
	QEU_QUAL_PARAMS	*qparams);


/*{
** Name: QEU_DBPRIV	- Process requests to manipulate the iidbpriv catalog
**
** External QEF call:   status = qef_call(QEU_DBPRIV, &qeuq_cb);
**
** Description:
**	This procedure appends,  modifies or deletes tuples in iidbpriv.
**	If a tuple does not already exist for the grantee/database,
**	a template is used that has all privileges undefined;
**	if a tuple does exist for the grantee/database, it is
**	read into memory.  The input tuple is then merged with 
**	the existing (or template) tuple.  If the resultant tuple
**	has any privileges remaining as defined, it is (re)written
**	to iidbpriv; otherwise it is deleted from iidbpriv.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	    .qeuq_cdbpriv	number of iidbpriv tuples (must be one)
**	    .qeuq_dbpr_tup	input iidbpriv tuples
**	    .qeuq_dbpr_mask	operation mask (GRANT/REVOKE)
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE021A_DBPRIV_NOT_AUTH
**				E_QE021B_DBPRIV_NOT_DBDB
**				E_QE021C_DBPRIV_DB_UNKNOWN
**				E_QE021D_DBPRIV_NOT_GRANTED
**				E_QE021E_DBPRIV_USER_UNKNOWN
**				E_QE021F_DBPRIV_GROUP_UNKNOWN
**				E_QE0220_DBPRIV_ROLE_UNKNOWN
**				E_QE0229_DBPRIV_NOT_GRANTED
**				E_QE022A_IIUSERGROUP
**				E_QE022B_IIAPPLICATION_ID
**				E_QE022C_IIDBPRIV
**				E_QE022D_IIDATABASE
**				E_QE022E_IIUSER
**				E_QE022F_SCU_INFO_ERROR
**	Returns:
**	    E_DB_OK		The template tuple was successfully processed
**	    E_DB_INFO		The grantee or privilege was rejected
**	    E_DB_WARN		The database was rejected
**	    E_DB_ERROR		A fatal error occurred
**	    E_DB_SEVERE		A severe fatal error occurred
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will insert, update, or delete tuples in iidbpriv.
**
** History:
**	12-apr-89 (ralph)
**          written
**      29-jun-89 (jennifer)
**          Added security auditing.
**	06-sep-89 (ralph)
**	    Add support for GRANT/REVOKE ON ALL DATABASES
**      11-apr-90 (jennifer)
**          bug 20787, 20788 Fix audit message output.
**      13-apr-90 (jennifer)
**          bug 20788, Fixed audit message description so that the
**          message parameter always is initialized.
**	16-apr-90 (ralph)
**	    Don't allow session to issue GRANT/REVOKE ON DATABASE
**          if it only has UPDATE_SYSCAT privileges.
**	08-aug-90 (ralph)
**          Don't allow session to issue GRANT/REVOKE ON DATABASE
**          if it only has UPDATE_SYSCAT privileges.
**	    Add support for GRANT/REVOKE ALL [PRIVILEGES] ON DATABASE....
**	    Audit GRANT/REVOKE failures due to security violation (bug 20790)
**	    Add audit description for [NO]DB_ADMIN
**	28-dec-90 (ralph)
**	    Add support for GRANT/REVOKE ON INSTALLATION
**	    Add support for GRANT/REVOKE ALL ON DATABASE/INSTALLATION
**	15-jan-91 (ralph)
**	    Be sure to remove tuple from iidbpriv if REVOKE ALL specified.
**	    Issue E_QE0229_DBPRIV_NOT_GRANTED instead of 
**		  E_QE021D_DBPRIV_NOT_GRANTED when ON INSTALLATION specified.
**	17-jun-92 (andre)
**	    initialize QEU_CB.qeu_flag before calling qeu_get() since qeu_get()
**	    will check for QEU_BY_TID to determine whether it needs to get by
**	    tid
**	11-may-1993 (robf)
**	    replaced SUPERUSER by SECURITY privilege
**	20-oct-93 (stephenb)
**	    GRANT/REVOKE on a database is a control action not create/drop 
**	    action, altered call to qeu_secaudit to fix this.
**	20-oct-93 (robf)
**          Add detail logging for priv list, and now use MAINTAIN_USER
**	    instead of SECURITY.
**	6-jan-94 (robf)
**	    Audit GRANT/REVOKE as CONTROL operations on the database, not
**	    CREATE/DROP (which apply to creating/destroying the database).
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.    
w
*/
DB_STATUS
qeu_dbpriv(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_DATABASE	    dbtuple;
    DB_PRIVILEGES   *intuple = NULL;
    DB_PRIVILEGES   outuple;
    DB_STATUS	    status, local_status;
    DB_ERROR	    e_error;
    i4	    error = 0;
    i4	    local_error = 0;
    bool	    transtarted = FALSE;
    bool	    tbl_opened = FALSE;
    bool	    tuple_exists = FALSE;
    QEU_CB	    tranqeu;
    QEU_CB	    qeu;
    i4	    glen, dlen;	
    i4		    i;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[3];
    DMR_ATTR_ENTRY  *key_ptr_array[3];
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[3];
    i4		    user_status;
    char	    dbname[DB_DB_MAXNAME];
    char	    username[DB_OWN_MAXNAME];
    i4              access;
    char            string[256];
    i4              l_string;
    i4         msgid;
    
    for (i=0; i<3; i++)
	key_ptr_array[i] = &key_array[i];

    for (;;)
    {

	/* Check the control block. */

	error = E_QE0017_BAD_CB;
	status = E_DB_SEVERE;
	if (qeuq_cb->qeuq_type != QEUQCB_CB || 
	    qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	    break;

	/* Initialize values */
	MEfill(sizeof(dbtuple.du_own),' ', (PTR)&dbtuple.du_own);

	error = E_QE0018_BAD_PARAM_IN_CB;
        if ((qeuq_cb->qeuq_db_id == 0)
            || (qeuq_cb->qeuq_d_id == 0))

	    break;

	if (qeuq_cb->qeuq_cdbpriv != 1 || qeuq_cb->qeuq_dbpr_tup == 0)

		break;

	intuple = (DB_PRIVILEGES *)qeuq_cb->qeuq_dbpr_tup->dt_data;

	if (intuple == NULL)

	    break;

	if (qeuq_cb->qeuq_dbpr_mask != QEU_GDBPRIV &&
	    qeuq_cb->qeuq_dbpr_mask != QEU_RDBPRIV)
	
		break;

	/* 
	** Check to see if a transaction is in progress,
	** if so, set transtarted flag to FALSE, otherwise
	** set it to TRUE after beginning a transaction.
	** If we started a transaction then we will abort
        ** it if an error occurs, otherwise we will just
        ** return the error and let the caller abort it.
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
	/* escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/* Get info about the user and the current database */

	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type		= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_QEF_ID;
	scf_cb.scf_session	= qef_cb->qef_ses_id;
	scf_cb.scf_ptr_union.scf_sci	 = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 3;
	sci_list[0].sci_length	= sizeof(dbname);
	sci_list[0].sci_code	= SCI_DBNAME;
	sci_list[0].sci_aresult	= (char *) dbname;
	sci_list[0].sci_rlength	= NULL;
	sci_list[1].sci_length	= sizeof(user_status);
	sci_list[1].sci_code	= SCI_USTAT;
	sci_list[1].sci_aresult	= (char *) &user_status;
	sci_list[1].sci_rlength	= NULL;
	sci_list[2].sci_length	= sizeof(username);
	sci_list[2].sci_code	= SCI_USERNAME;
	sci_list[2].sci_aresult	= (char *) username;
	sci_list[2].sci_rlength	= NULL;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    error = E_QE022F_SCU_INFO_ERROR;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;

	}
	
	/* Make sure we are in the iidbdb */

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    error = E_QE021B_DBPRIV_NOT_DBDB;
	    status = E_DB_ERROR;
	    break;

	}

	/* Ensure database exists if GRANT/REVOKE ON DATABASE was specified */

	if (!(intuple->dbpr_dbflags & DBPR_ALLDBS))
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
	    qeu.qeu_key = key_ptr_array;
	    key_ptr_array[0]->attr_number = DM_1_DATABASE_KEY;
	    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    key_ptr_array[0]->attr_value = (char *) &intuple->dbpr_database;
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
		error = E_QE021C_DBPRIV_DB_UNKNOWN;
	    else
		error = 0;
         	
	    status = qeu_close(qef_cb, &qeu);
	    tbl_opened = FALSE;
	    if (status != E_DB_OK)
	    {
		error = E_QE022D_IIDATABASE;
		status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		break;
	    }
	}
	else		    /* GRANT/REVOKE ON ALL DATABASES was specified */
	    error = 0;

	/* Prepare information for auditing this transaction */

	if (qeuq_cb->qeuq_dbpr_mask & QEU_RDBPRIV)
	{
	    access = SXF_A_CONTROL;
	    msgid = I_SX2006_DBPRIV_DROP;
	}
	else
	{
	    access = SXF_A_CONTROL;
	    msgid = I_SX2005_DBPRIV_CREATE;
	}

	(VOID) qeu_xdbpriv(intuple->dbpr_control,
			   intuple->dbpr_flags,
			   string,
			   &l_string);

	/* Make sure the user is authorized to grant on the database */


	if (intuple->dbpr_dbflags & DBPR_ALLDBS)
	{
	    /* Must have  maintain_users */

	    if (!(user_status & DU_UMAINTAIN_USER))
		error = E_QE0233_ALLDBS_NOT_AUTH;
	}
	else if (error == 0)			/* If the database exists     */
	{
	    /* Must be superuser, or be the dba, or  have MAINTAIN_DBPRIV */

	    if (!(user_status & DU_UMAINTAIN_USER) && 
		 (MEcmp((PTR)&dbtuple.du_own,
			(PTR)username,
			sizeof(username)) != 0))
		error = E_QE021A_DBPRIV_NOT_AUTH;
	}
	else					/* The database doesn't exist */
	{
	    /* Must be superuser, and REVOKE ONLY  */

	    if ((user_status & DU_UMAINTAIN_USER) &&
		(qeuq_cb->qeuq_dbpr_mask & QEU_RDBPRIV))
		error = 0;
	}

	if (error != 0)				/* If we're not authorized    */
	{
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
		status = qeu_secaudit_detail(TRUE, qef_cb->qef_ses_id, 
		    intuple->dbpr_database.db_db_name, &dbtuple.du_own, 
		    DB_DB_MAXNAME, SXF_E_DATABASE, msgid,
		    SXF_A_FAIL | access, 
		    &e_error,
		    string,
		    l_string,
		    0);

	    status = E_DB_WARN;
	    break;					/* kill the query     */
	}

	/* For GRANT, make sure the grantee exists */

	if (qeuq_cb->qeuq_dbpr_mask & QEU_GDBPRIV)
	switch (intuple->dbpr_gtype)
	{
	    case DBGR_USER:				/* Grantee is USER */
		status = qeu_guser(qef_cb, qeuq_cb,
				   (char *)&intuple->dbpr_gname, NULL, NULL);
		if (DB_FAILURE_MACRO(status))
		    error  = E_QE022E_IIUSER;
		else if (status != E_DB_OK)
		    error = E_QE021E_DBPRIV_USER_UNKNOWN;
		break;

	    case DBGR_GROUP:				/* Grantee is GROUP */
		status = qeu_ggroup(qef_cb, qeuq_cb,
				    (char *)&intuple->dbpr_gname, NULL, NULL);
		if (DB_FAILURE_MACRO(status))
		    error  = E_QE022A_IIUSERGROUP;
		else if (status != E_DB_OK)
		    error = E_QE021F_DBPRIV_GROUP_UNKNOWN;
		break;

	    case DBGR_APLID:				/* Grantee is ROLE */
		status = qeu_gaplid(qef_cb, qeuq_cb,
				   (char *)&intuple->dbpr_gname, NULL);
		if (DB_FAILURE_MACRO(status))
		    error  = E_QE022B_IIAPPLICATION_ID;
		else if (status != E_DB_OK)
		    error = E_QE0220_DBPRIV_ROLE_UNKNOWN;
		break;

	    default:		/* None of the above; don't check grantee */
		status = E_DB_OK;
		break;
	}
	else			/* Don't check grantee for REVOKE statement */
	    status = E_DB_OK;

	if (status != E_DB_OK)				/* Exit if error */
	    break;

	/* open the iidbpriv table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DBPRIV_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DBPRIV_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0; 
         	
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = E_QE022C_IIDBPRIV;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	tbl_opened = TRUE;

	/* Read the existing iidbpriv record for the database/grantee */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DB_PRIVILEGES);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_PRIVILEGES);
	qef_data.dt_data = (PTR)&outuple;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 3;
	qeu.qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_3_DBPRIV_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &intuple->dbpr_database;
	key_ptr_array[1]->attr_number = DM_1_DBPRIV_KEY;
	key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value = (char *) &intuple->dbpr_gname;
	key_ptr_array[2]->attr_number = DM_2_DBPRIV_KEY;
	key_ptr_array[2]->attr_operator = DMR_OP_EQ;
	key_ptr_array[2]->attr_value = (char *) &intuple->dbpr_gtype;
	
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;

	status = qeu_get(qef_cb, &qeu);
	if (((status != E_DB_OK) &&
	     (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)) ||
	    ((status == E_DB_OK) &&
	     (qeu.qeu_count != 1)))
	{
	    error = E_QE022C_IIDBPRIV;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	if (status != E_DB_OK)			/* No iidbpriv tuple exists  */
	{
	    /* Initialize tuple with no privs defined */

	    STRUCT_ASSIGN_MACRO(intuple->dbpr_database, outuple.dbpr_database);
	    STRUCT_ASSIGN_MACRO(intuple->dbpr_gname, outuple.dbpr_gname);
	    outuple.dbpr_gtype		= intuple->dbpr_gtype;
	    outuple.dbpr_control	= 0;
	    outuple.dbpr_flags		= 0;
	    outuple.dbpr_dbflags    	= intuple->dbpr_dbflags;
	    outuple.dbpr_qrow_limit	= 0;
	    outuple.dbpr_qdio_limit	= 0;
	    outuple.dbpr_qcpu_limit	= 0;
	    outuple.dbpr_qpage_limit	= 0;
	    outuple.dbpr_qcost_limit	= 0;
	    outuple.dbpr_connect_time_limit	= 0;
	    outuple.dbpr_idle_time_limit	= 0;
	    outuple.dbpr_priority_limit	= 0;
	    MEfill(sizeof(outuple.dbpr_reserve),
		   (u_char)' ',
		   (PTR)outuple.dbpr_reserve);
	    status = E_DB_OK;
	}
	else					/* An iidbpriv tuple esisted */
	    tuple_exists = TRUE;

	/*
	** If REVOKE was specified, ensure that the dbprivs were
	** previously GRANTed.  If not, reject the request.
	*/

	if ((qeuq_cb->qeuq_dbpr_mask & QEU_RDBPRIV) &&
	    (!(intuple->dbpr_control & DBPR_ALLPRIVS)) &&
	    (((outuple.dbpr_control & intuple->dbpr_control) !=
				      intuple->dbpr_control) ||
	     ((outuple.dbpr_flags   & intuple->dbpr_control) !=
				      intuple->dbpr_flags)
	    )
	   )
	{
            if (intuple->dbpr_dbflags & DBPR_ALLDBS)
                error = E_QE0229_DBPRIV_NOT_GRANTED;
            else
                error = E_QE021D_DBPRIV_NOT_GRANTED;
	    status = E_DB_INFO;
	    break;
	}

	/*
	** Merge the privileges being granted or revoked into the
	** iidbpriv tuple.
	*/

	if (qeuq_cb->qeuq_dbpr_mask & QEU_GDBPRIV)	/* GRANT secified*/
	{
	    outuple.dbpr_control |=  intuple->dbpr_control;
	    outuple.dbpr_flags   &= ~intuple->dbpr_control;
	    outuple.dbpr_flags   |=  intuple->dbpr_flags;
	    
	}
	else						/* REVOKE specified */
        {
            if (intuple->dbpr_control & DBPR_ALLPRIVS)
            {
                outuple.dbpr_control = 0L;
                outuple.dbpr_flags   = 0L;
            }
            else
            {
                outuple.dbpr_control &= ~intuple->dbpr_control;
                outuple.dbpr_flags   &= ~intuple->dbpr_control;
            }
        }

	if (intuple->dbpr_control & DBPR_IDLE_LIMIT)
	    outuple.dbpr_idle_time_limit = intuple->dbpr_idle_time_limit;
	if (intuple->dbpr_control & DBPR_PRIORITY_LIMIT)
	    outuple.dbpr_priority_limit = intuple->dbpr_priority_limit;
	if (intuple->dbpr_control & DBPR_CONNECT_LIMIT)
	    outuple.dbpr_connect_time_limit = intuple->dbpr_connect_time_limit;
	if (intuple->dbpr_control & DBPR_QROW_LIMIT)
	    outuple.dbpr_qrow_limit = intuple->dbpr_qrow_limit;
	if (intuple->dbpr_control & DBPR_QDIO_LIMIT)
	    outuple.dbpr_qdio_limit = intuple->dbpr_qdio_limit;
	if (intuple->dbpr_control & DBPR_QCPU_LIMIT)
	    outuple.dbpr_qcpu_limit = intuple->dbpr_qcpu_limit;
	if (intuple->dbpr_control & DBPR_QPAGE_LIMIT)
	    outuple.dbpr_qpage_limit = intuple->dbpr_qpage_limit;
	if (intuple->dbpr_control & DBPR_QCOST_LIMIT)
	    outuple.dbpr_qcost_limit = intuple->dbpr_qcost_limit;

	/*
	** If the privilege descriptor already existed, delete it now.
	*/

	if (tuple_exists)
	{
	    /* Delete the exsting tuple */
	    status = qeu_delete(qef_cb, &qeu);
	    if ((status != E_DB_OK) ||
		(qeu.qeu_count != 1))
	    {
		error = E_QE022C_IIDBPRIV;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		break;
	    }
	}

	/*
	** If any privileges are defined, write the tuple out to iidbpriv.
	** If no privileges are defined, don't bother to write it.
	*/

	if (outuple.dbpr_control != 0x00L)
	{
	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DB_PRIVILEGES);
	    qeu.qeu_input = &qef_data;

	    status = qeu_append(qef_cb, &qeu);
	    if ((status != E_DB_OK) ||
		(qeu.qeu_count != 1))
	    {
		error = E_QE022C_IIDBPRIV;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		break;
	    }
	}

	/* This is a security event, audit it. */

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
		    intuple->dbpr_database.db_db_name, &dbtuple.du_own, 
		    DB_DB_MAXNAME, SXF_E_DATABASE, msgid,
		    SXF_A_SUCCESS | access, &e_error);
	break;

    } /* end for */

    /* call qef_error to handle error messages, if any */

    if (status != E_DB_OK)
    {
	if (intuple != NULL)
	{
	    glen = cus_trmwhite(sizeof(intuple->dbpr_gname.db_own_name),
				intuple->dbpr_gname.db_own_name);

	    dlen = cus_trmwhite(sizeof(intuple->dbpr_database.db_db_name),
				intuple->dbpr_database.db_db_name);

	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error,
			     (i4)2,
			     dlen, intuple->dbpr_database.db_db_name,
			     glen, intuple->dbpr_gname.db_own_name);
	}
	else
	{
	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error,
			     (i4)0);
	}
    }

    /* Close the table. */

    if (tbl_opened)
    {
	local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qeu.error.err_code, 0L, local_status, 
			&error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    /* End the transaction if one was started */

    if (transtarted)
    {
	if (status == E_DB_OK)
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		(VOID) qef_error(tranqeu.error.err_code, 0L, status, 
			    &error, &qeuq_cb->error, 0);
	    }
	}
	else
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
    }

    return (status);
}

/*{
** Name: QEU_PDBPRIV	- Purge database privileges associated with grantee
**
** Internal QEF call:   status = qeu_pdbpriv(&grantee, &gtype)
**
** Description:
**	This procedure removes all iidbpriv tuples for a specified
**	grantee.  It is used by internal QEU routines when deleting
**	a grantee.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	grantee			the grantee for which privs are purged
**	gtype			the type of grantee
**
** Outputs:
**	None.
**
**	Returns:
**	    E_DB_OK		Success; no tuples deleted
**	    E_DB_INFO		Success; tuples deleted
**	    E_DB_ERROR		Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine removes tuples from iidbpriv.
**
** History:
**	12-apr-89 (ralph)
**          written
**	25-sep-90 (ralph)
**	    added qualification to audit implicit revokes
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
*/
DB_STATUS
qeu_pdbpriv(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*grantee,
char		*gtype)
{
    DB_PRIVILEGES   dbptuple;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    DB_STATUS	    status = E_DB_OK, local_status;
    i4		    i;
    DMR_ATTR_ENTRY  key_array[3];
    DMR_ATTR_ENTRY  *key_ptr_array[3];

    for (i=0; i<3; i++)
	key_ptr_array[i] = &key_array[i];

    for (;;)
    {
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_DBPRIV_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_DBPRIV_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0; 

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	qeu.qeu_count = 0;
	qeu.qeu_tup_length = sizeof(DB_PRIVILEGES);
	qeu.qeu_output	= NULL;
	qeu.qeu_getnext = QEU_REPO;
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    qparams.qeu_qparms[0] = (PTR)qef_cb;
	    qeu.qeu_qual	= qeu_qdbpriv;
	    qeu.qeu_qarg	= &qparams;
	}
	else
	{
	    qeu.qeu_qual	= NULL;
	    qeu.qeu_qarg	= NULL;
	}

	qeu.qeu_klen = 3;
	qeu.qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number   = DM_1_DBPRIV_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value    = (char *) grantee;
	key_ptr_array[1]->attr_number   = DM_2_DBPRIV_KEY;
	key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value    = (char *) gtype;
	dbptuple.dbpr_gtype = DBGR_APLID;
	key_ptr_array[2]->attr_number   = DM_3_DBPRIV_KEY;
	key_ptr_array[2]->attr_operator = DMR_OP_GTE;
	key_ptr_array[2]->attr_value    = (char *) &dbptuple.dbpr_database;
	MEfill(sizeof(DB_OWN_NAME), (u_char)' ',
	       (PTR)&dbptuple.dbpr_database);

	status = qeu_delete(qef_cb, &qeu);
	
	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count != 0)
	    status = E_DB_INFO;
	else
	    status = E_DB_OK;

	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	    local_status = (local_status > E_DB_ERROR) ?
			    local_status : E_DB_ERROR;

	status = (status > local_status) ? status : local_status;
	
	break;
    }

    return(status);
}

/*{
** Name: QEU_QDBPRIV	- Qualify (audit) iidbpriv tuple 
**
** Description:
**	This procedure qualifies an iidbpriv tuple and audits it for
**	deletion.  It is used by internal QEU routines when deleting
**	a grantee.  Actually, no qualification is done, we always return
**	"qualified" -- this is just a hook for auditing.
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	The QEF_CB for the session
**	    .qeu_rowaddr	Current iidbpriv tuple
**
** Outputs:
**	qparams.qeu_retval	Always ADE_TRUE - row qualifies
**
**	Returns E_DB_OK or error
**	Exceptions:
**	    none
**
** Side Effects:
**	     Produces audit records indicating the dbprivs are dropped
**
** History:
**      25-sep-90 (ralph)
**          written
**	20-oct-93 (stephenb)
**	    GRANT/REVOKE on a database is a control action not create/drop 
**	    action, altered call to qeu_secaudit to fix this.
**	20-oct-93 (robf)
**          write priv info to detail list instead of discarding
**	11-Apr-2008 (kschendel)
**	    Revised call sequence.
*/
static DB_STATUS
qeu_qdbpriv(
	void *toss, QEU_QUAL_PARAMS *qparams)

{
    QEF_CB          *qef_cb;
    i4         error = 0;
    DB_STATUS       status;
    DB_ERROR	    e_error;
    DB_PRIVILEGES   *dbpriv_tuple;
    char            string[256];
    i4              l_string;

    qef_cb      = (QEF_CB *)qparams->qeu_qparms[0];
    dbpriv_tuple = (DB_PRIVILEGES *) qparams->qeu_rowaddr;
    qparams->qeu_retval = ADE_TRUE;

    if (qef_cb == NULL || dbpriv_tuple == NULL)      /* Bad parm; abort! */
	return(E_DB_ERROR);

    /* Generate the privilege list being implicitly revoked */

    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	(VOID) qeu_xdbpriv(dbpriv_tuple->dbpr_control,
			   dbpriv_tuple->dbpr_flags,
			   string,
			   &l_string);

	status = qeu_secaudit_detail(FALSE, qef_cb->qef_ses_id, 
		    dbpriv_tuple->dbpr_database.db_db_name, (DB_OWN_NAME *)NULL, 
		    DB_DB_MAXNAME, SXF_E_DATABASE, I_SX2006_DBPRIV_DROP,
		    SXF_A_SUCCESS | SXF_A_CONTROL ,
		    &e_error,
		    string,
		    l_string,
		    0);

	if (status != E_DB_OK)
	    return(E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*{
** Name: QEU_XDBPRIV	- Expand privileges in a dbpriv tuple
**
** Internal QEF call:   qeu_xdbpriv(control, flags, string, &l_string);
**
** Description:
**	This procedure expands the privileges described by an iidbpriv
**	tuple into a string that is suitable for auditing.
**	It is used by internal QEU routines when preparing to audit
**	changes to database privileges.
**
** Inputs:
**      control			Control bits from iidbpriv tuple
**	flags			Flag bits from iidbpriv tuple
**
** Outputs:
**	string			Output buffer containing privilege description
**	l_string		Pointer to length of output string
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	     Produces audit records indicating the dbprivs are dropped
**
** History:
**      25-sep-90 (ralph)
**          written
**	05-dec-90 (ralph)
**	    Audit GRANT/REVOKE LOCKMODE (bug 34532)
**	20-oct-93 (robf)
**          Add QUERY_SYSCAT/TABLE_STATISTICS/MAINTAIN_DBPRIV
**	3-dec-93 (robf)
**          QUERY_SYSCAT now SELECT_SYSCAT per LRC.
**	10-may-02 (inkdo01)
**	    Add SEQ_CREATE for sequence support.
*/
VOID
qeu_xdbpriv(
i4		control,
i4		flags,
char		*string,
i4		*l_string)
{
    char            *p;

    *l_string = 0;
    p = string;

    if (control & DBPR_QROW_LIMIT)
    {
        if ((flags & DBPR_QROW_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("ROW_LIMIT ",10,p);
        p += 10;
        *l_string +=10;
	control ^= DBPR_QROW_LIMIT;
    }

    if (control & DBPR_QDIO_LIMIT)
    {
        if ((flags & DBPR_QDIO_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("DIO_LIMIT ",10,p);
        p += 10;
        *l_string +=10;
	control ^= DBPR_QDIO_LIMIT;
    }

    if (control & DBPR_QCPU_LIMIT)
    {
        if ((flags & DBPR_QCPU_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("CPU_LIMIT ",10,p);
        p += 10;
        *l_string +=10;
	control ^= DBPR_QCPU_LIMIT;
    }

    if (control & DBPR_QPAGE_LIMIT)
    {
        if ((flags & DBPR_QPAGE_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("PAGE_LIMIT ",11,p);
        p += 11;
        *l_string +=11;
	control ^= DBPR_QPAGE_LIMIT;
    }

    if (control & DBPR_QCOST_LIMIT)
    {
        if ((flags & DBPR_QCOST_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("COST_LIMIT ",11,p);
        p += 11;
        *l_string +=11;
	control ^= DBPR_QCOST_LIMIT;
    }

    if (control & DBPR_TAB_CREATE)
    {
        if ((flags & DBPR_TAB_CREATE) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("TAB_CREATE ",11,p);
        p += 11;
        *l_string +=11;
	control ^= DBPR_TAB_CREATE;
    }

    if (control & DBPR_PROC_CREATE)
    {
        if ((flags & DBPR_PROC_CREATE) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("PROC_CREATE ",12,p);
        p += 12;
        *l_string +=12;
	control ^= DBPR_PROC_CREATE;
    }

    if (control & DBPR_SEQ_CREATE)
    {
        if ((flags & DBPR_SEQ_CREATE) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("SEQ_CREATE ",11,p);
        p += 11;
        *l_string +=11;
	control ^= DBPR_SEQ_CREATE;
    }

    if (control & DBPR_ACCESS)
    {
        if ((flags & DBPR_ACCESS) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("ACCESS ",7,p);
        p += 7;
        *l_string +=7;
	control ^= DBPR_ACCESS;
    }

    if (control & DBPR_UPDATE_SYSCAT)
    {
        if ((flags & DBPR_UPDATE_SYSCAT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("UPDATE_SYSCAT ",14,p);
        p += 14;
        *l_string +=14;
	control ^= DBPR_UPDATE_SYSCAT;
    }

    if (control & DBPR_DB_ADMIN)
    {
        if ((flags & DBPR_DB_ADMIN) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("DB_ADMIN ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_DB_ADMIN;
    }

    if (control & DBPR_LOCKMODE)
    {
        if ((flags & DBPR_LOCKMODE) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("LOCKMODE ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_LOCKMODE;
    }

    if (control & DBPR_QUERY_SYSCAT)
    {
        if ((flags & DBPR_QUERY_SYSCAT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("SELECT_SYSCAT ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_QUERY_SYSCAT;
    }
    if (control & DBPR_TABLE_STATS)
    {
        if ((flags & DBPR_TABLE_STATS) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("TABLE_STATS ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_TABLE_STATS;
    }
    if (control & DBPR_CONNECT_LIMIT)
    {
        if ((flags & DBPR_CONNECT_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("CONNECT_TIME_LIMIT ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_CONNECT_LIMIT;
    }
    if (control & DBPR_IDLE_LIMIT)
    {
        if ((flags & DBPR_IDLE_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("IDLE_TIME_LIMIT ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_IDLE_LIMIT;
    }
    if (control & DBPR_PRIORITY_LIMIT)
    {
        if ((flags & DBPR_PRIORITY_LIMIT) == 0)
        {
            MEcopy("NO",2,p);
            p +=2;
            *l_string  +=2;
        }
        MEcopy("SESSION_PRIORITY ",9,p);
        p += 9;
        *l_string +=9;
	control ^= DBPR_PRIORITY_LIMIT;
    }
    if (control != 0)
    {
            MEcopy("UNKNOWN ",8,p);
            p += 8;
            *l_string +=8;
    }

    return;
}

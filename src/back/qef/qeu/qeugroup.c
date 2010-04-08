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
**  Name: QEUGROUP.C - routines for maintaining group identifiers
**
**  Description:
**      The routines defined in this file provide an interface for
**	qurymod to manipulate the contents of the iiusergroup catalog.
**	The operations supported include CREATE for groups and members,
**	DROP for groups and members (including purge).
**
**	The following routines are defined in this file:
**
**	    qeu_group	- dispatches iiusergroup processing routines
**	    qeu_cgroup	- appends tuples to iiusergroup
**	    qeu_dgroup	- deletes member tuples from iiusergroup
**	    qeu_pgroup  - deletes all tuples associated with a specific member
**	    qeu_ggroup	- gets iiusergroup tuples for internal QEF routines
**	    qeu_qgroup	- qualifies iiusergroup tuples for qeu_pgroup
**
**
**  History:
**	17-may-89 (ralph)
**	    Created this file from queqmod.c routines.
**	06-jun-89 (ralph)
**	    Corrected minor unix portability problems
**      29-jun-89 (jennifer)
**          Added security auditing calls.
**	08-oct-89 (ralph)
**	    Added support for default group, QEU_DEFAULT_GROUP.
**	26-oct-89 (ralph)
**	    Added qeu_pgroup() to delete specific user from all groups
**	24-sep-90 (ralph)
**	    Correct audit outcome
**	02-feb-91 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	27-nov-1992 (pholman)
**	    C2: Replace obsolete qeu_audit calls with new qeu_secaudit.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	16-aug-93 (stephenb)
**          Changed all calls to qeu_secaudit() to audit SXF_E_SECURITY instead
**          of SXF_E_USER, group manipulation is a security event not a
**          user event.
**	3-sep-93 (stephenb)
**	    Fixed calls to qeu_secaudit() so that we distinguish between
**	    DROP GROUP and ALTER GROUP DROP ALL when auditing. Also make
**	    add/delete group members an SXF_A_ALTER not SXF_A_CREATE
**	    SXF_A_DROP.
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	3-nov-93 (robf)
**          Pass security label when auditing, since groups don't have a
**	    distinct label currently we use the label for iigroup.
**	03-feb-94 (rickh)
**	    Fixed up an error return from a security audit call in qeu_qgroup
**	    to shut up a compiler.
**	31-mar-94 (rblumer)       in qeu_group,
**	    Changed error handling to correctly handle blanks in delimited ids,
**	    by using cus_trmwhite instead of STindex to trim trailing blanks.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	05-Jul-99 (carsu07)
**	    If a user isn't previously a member of a group do not issue the
**	    following message after a successful alter user with group 
**	    statement. "E_US18F7 CREATE/ALTER USER: User '%1c' added to group
**	    '%0c'. The user was not a member of the specified group, but was
**	    added to the group due to the CREATE/ALTER USER request."
**	    This message is misleading as it begins with E_ when it's purely
**	    informational and it gives an errorno of 50. (Bug 93851)
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
**	    Revise DMF-qualification call sequence.
**/

/*
**  Definition of static variables and forward static functions 
*/

static DB_STATUS qeu_qgroup(
	void		*toss,
	QEU_QUAL_PARAMS	*qparams);


/*{
** Name: QEU_GROUP	- Dispatch QEU routines to manipulate iiusergroup.
**
** External QEF call:   status = qef_call(QEU_GROUP, &qeuq_cb);
**
** Description:
**	This procedure checks the qeuq_agid_mask field of the QEUQ_CB
**	to determine which qrymod processing routine to call:
**		QEU_CGROUP results in call to qeu_cgroup()
**		QEU_DGROUP results in call to qeu_dgroup()
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_agid_mask	iiusergroup operation code
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0160_INVALID_GRPID_OP
**				E_QE022A_IIUSERGROUP
**				Other error codes from qeu_[cd]group
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_SEVERE
**	    Other status codes from qeu_[cd]group
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-mar-89 (ralph)
**          written
**	08-oct-89 (ralph)
**	    Added support for default group, QEU_DEFAULT_GROUP.
**	31-mar-94 (rblumer)
**	    Changed error handling to correctly handle blanks in delimited ids,
**	    by using cus_trmwhite instead of STindex to trim trailing blanks.
*/
DB_STATUS
qeu_group(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_USERGROUP    *ugtuple = NULL;
    DB_STATUS	    status, local_status;
    i4	    error = 0;
    bool	    transtarted = FALSE;
    bool	    tbl_opened = FALSE;
    QEU_CB	    tranqeu;
    QEU_CB	    qeu;
    i4	    glen, mlen;

    for (;;)
    {

	/* Check the control block. */

	error = E_QE0017_BAD_CB;
	status = E_DB_SEVERE;
	if (qeuq_cb->qeuq_type != QEUQCB_CB || 
	    qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	    break;

	error = E_QE0018_BAD_PARAM_IN_CB;
        if ((qeuq_cb->qeuq_db_id == 0)
            || (qeuq_cb->qeuq_d_id == 0))

	    break;

	if (qeuq_cb->qeuq_cagid != 1 || qeuq_cb->qeuq_agid_tup == 0)

		break;

        ugtuple = (DB_USERGROUP *)qeuq_cb->qeuq_agid_tup->dt_data;

	if (ugtuple == NULL)

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

	/* prepare the qeu_cb and open the table for processing */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_GRPID_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_GRPID_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
         	
	/* open the iiusergroup table */

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = E_QE022A_IIUSERGROUP;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	tbl_opened = TRUE;

	/* Call the appropriate group function */

	switch (qeuq_cb->qeuq_agid_mask &
		(QEU_CGROUP | QEU_DGROUP | QEU_PGROUP))
	{
	    case QEU_CGROUP:
		status = qeu_cgroup(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    case QEU_DGROUP:
	    case QEU_PGROUP:
		status = qeu_dgroup(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    default:
		status = E_DB_SEVERE;
		error = E_QE0160_INVALID_GRPID_OP;
		break;
	}
	
	break;

    
    } /* end for */

    /* call qef_error to handle error messages, if any */

    if (status != E_DB_OK)
    {
	if (ugtuple != NULL)
	{
	    glen = cus_trmwhite(sizeof(ugtuple->dbug_group.db_own_name),
				ugtuple->dbug_group.db_own_name);

	    mlen = cus_trmwhite(sizeof(ugtuple->dbug_member.db_own_name),
				ugtuple->dbug_member.db_own_name);

	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, (i4)2,
			     glen, ugtuple->dbug_group.db_own_name,
			     mlen, ugtuple->dbug_member.db_own_name);
	}
	else
	{
	    (VOID) qef_error(error, 0L, status, &error,
			     &qeuq_cb->error, (i4)0);
	}
    }  /* end if status not OK */

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
	if (status <= E_DB_WARN)
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
** Name: QEU_CGROUP	- Create group
**
** Internal	call:   status = qeu_cgroup(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**	Appends tuples to the iiusergroup catalog.
**	A group anchor tuple (blank member name) must exist before
**	member tuples (nonblank member name) can be added;
**	if not, an error is returned.
**	If a member tuple already exists, a warning message is
**	issued, but processing continues.
**	If a group tuple already exists, an error message is returned.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_cagid		number of group tuples
**	    .qeuq_agid_tup	group tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iiusergroup
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0213_GRPID_EXISTS
**				E_QE0214_GRPID_NOT_EXIST
**				E_QE0215_GRPMEM_EXISTS
**				E_QE0221_GROUP_USER
**				E_QE0222_GROUP_ROLE
**				E_QE0227_GROUP_BAD_USER
**				E_QE022A_IIUSERGROUP
**				E_QE022B_IIAPPLICATION_ID
**				E_QE022E_IIUSER
**	Returns:
**	    E_DB_OK
**		Processing completed normally.
**	    E_DB_INFO
**		For ALTER GROUP...ADD, the member already exists in the group,
**		or the specified user is not defined in the iiuser catalog.
**	    E_DB_WARN
**		For CREATE GROUP, the group name conflicts.
**		For ALTER GROUP...ADD, the group does not exist.
**	    E_DB_ERROR
**		IIUSERGROUP, IIUSER, or IIROLE could not be processed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in the iiusergroup catalog.
**
** History:
**	13-mar-89 (ralph)
**          written.
**	06-may-89 (ralph)
**	    Check for name space collision with iiuser, iirole;
**	    Ensure member name exists in iiuser.
**      29-jun-89 (jennifer)
**          Added security auditing calls.
**	08-oct-89 (ralph)
**	    Added support for default group, QEU_DEFAULT_GROUP.
**	16-aug-93 (stephenb)
**          Changed all calls to qeu_secaudit() to audit SXF_E_SECURITY instead
**          of SXF_E_USER, group manipulation is a security event not a
**          user event.
**	3-sep-93 (stephenb)
**	    Do not audit group creation if action is QEU_PGROUP, otherwise
**	    we end up with two audit records for one action. Also make
**	    addition of new group member SXF_A_ALTER not SXF_A_CREATE
**	6-may-94 (robf)
**          Update check for user name clash to not lose error status.
**      05-Jul-99 (carsu07)
**          If a user isn't previously a member of a group do not issue the
**          following message after a successful alter user with group
**          statement. "E_US18F7 CREATE/ALTER USER: User '%1c' added to group
**          '%0c'. The user was not a member of the specified group, but was
**          added to the group due to the CREATE/ALTER USER request."
**          This message is misleading as it begins with E_ when it's purely
**          informational and it gives an errorno of 50. (Bug 93851)
*/
DB_STATUS
qeu_cgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_USERGROUP    *ugtuple, antuple;
    DB_STATUS	    status = E_DB_OK;
    i4	    error = 0;
    i4		    i;
    bool	    anchor;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[2];
    DMR_ATTR_ENTRY  *key_ptr_array[2];
    DB_ERROR	     e_error;
    DB_TAB_ID	     tabid;
    
    for (i=0; i< 2; i++)
	key_ptr_array[i] = &key_array[i];

    ugtuple = (DB_USERGROUP *)qeuq_cb->qeuq_agid_tup->dt_data;
    anchor  = ((char *)STskipblank((char *)&ugtuple->dbug_member,
				   (i4)sizeof(DB_OWN_NAME))
		       == (char *)NULL
	      );

    /* If we are creating an anchor, ensure no name collision */

    if (anchor)
    for (;;)
    {
	/* Check the iirole catalog to ensure no name collision */

	status = qeu_gaplid(qef_cb, qeuq_cb,
			   (char *)&ugtuple->dbug_group, NULL);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    error = E_QE022B_IIAPPLICATION_ID;
	else if (status == E_DB_OK)		/* Role exists! */
	{
	    error = E_QE0222_GROUP_ROLE;
	    status = E_DB_WARN;
	}
	else					/* Role does not exist */
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;

	/* Check the iiuser catalog to ensure no name collision */

	status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&ugtuple->dbug_group, NULL, NULL);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    error = E_QE022E_IIUSER;
	else if (status == E_DB_OK)		/* User exists! */
	{
	    error = E_QE0221_GROUP_USER;
	    status = E_DB_WARN;
	}
	else					/* User does not exist */
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;

        /* Check the iiprofile catalog to ensure no name collision */

        status = qeu_gprofile(qef_cb, qeuq_cb,
                           (char *)&ugtuple->dbug_group, NULL);

        if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
            error = E_QE0282_IIPROFILE;
        else if (status == E_DB_OK)		    /* Profile exists! */
        {
	    (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 2,
			    5, "group",
			    qec_trimwhite(sizeof(ugtuple->dbug_group),
					  (char *)&ugtuple->dbug_group),
			    (PTR)&ugtuple->dbug_group,
			    7, "profile" );
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
        }
        else					    /* Profile does not exist */
            status = E_DB_OK;

	break;
    } /* End loop */

    /*
    ** If creating a member,
    ** ensure anchor exists in iiusergroup,
    ** and user exists in iiuser.
    */

    if ((status == E_DB_OK) &&
	(!anchor))
    for (;;)
    {
	/* Ensure anchor exists in iiusergroup */

	qeu_cb->qeu_count = 1;
	qeu_cb->qeu_tup_length = sizeof(DB_USERGROUP);
	qeu_cb->qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_USERGROUP);
	qef_data.dt_data = (PTR)&antuple;
	qeu_cb->qeu_getnext = QEU_REPO;
	qeu_cb->qeu_klen = 2;       
	qeu_cb->qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_1_GRPID_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &ugtuple->dbug_group;
	key_ptr_array[1]->attr_number = DM_2_GRPID_KEY;
	key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value = (char *) &antuple.dbug_member;
	MEfill(sizeof(DB_OWN_NAME), (u_char)' ',
	       (PTR)&antuple.dbug_member);
	
	qeu_cb->qeu_qual = 0;
	qeu_cb->qeu_qarg = 0;
     
	status = qeu_get(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		error = E_QE0214_GRPID_NOT_EXIST;
		status = E_DB_WARN;
	    }
	    else
	    {
		error = E_QE022A_IIUSERGROUP;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;
	}
	else
	if (qeu_cb->qeu_count != 1)
	{
	    error = E_QE0214_GRPID_NOT_EXIST;
	    status = E_DB_WARN;
	    break;
	}

	/* Bypass user ckeck if bypass flag is on */

	if (qeuq_cb->qeuq_agid_mask & QEU_DEFAULT_GROUP)
	    break;

	/* Ensure member exists in iiuser */

	status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&ugtuple->dbug_member, NULL, NULL);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	{
	    error = E_QE022E_IIUSER;
	    break;
	}
	else if (status != E_DB_OK)		/* User does not exist! */
	{
	    error  = E_QE0227_GROUP_BAD_USER;
	    status = E_DB_INFO;
	    break;
	}

	break;
    }

    /* Add the group or member to the iiusergroup catalog */

    if (status == E_DB_OK)
    for (;;)
    {
	/* Like other QEU routines, this assumes one tuple per call */

	qeu_cb->qeu_count = qeuq_cb->qeuq_cagid;
	qeu_cb->qeu_tup_length = sizeof(DB_USERGROUP);
	qeu_cb->qeu_input = qeuq_cb->qeuq_agid_tup;
	status = qeu_append(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    error = E_QE022A_IIUSERGROUP;
	    break;
	}
	else
	if (qeu_cb->qeu_count != qeuq_cb->qeuq_cagid)
	{
	    if (anchor)				/* Duplicate group anchor */
	    {
		error  = E_QE0213_GRPID_EXISTS;
		status = E_DB_WARN;
	    }
	    else				/* Duplicate group member */
	    {
		error  = E_QE0215_GRPMEM_EXISTS;
		status = E_DB_INFO;
	    }
	    break;
	}

	
	/* This is a security event, audit it. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    tabid.db_tab_base=DM_B_GRPID_TAB_ID;
	    tabid.db_tab_index=DM_I_GRPID_TAB_ID;

	    if (anchor)
	    {
		/*
		** Only audit if this is not part of a purge group
		** (we have already audited this)
		*/
		if ( !(qeuq_cb->qeuq_agid_mask & QEU_PGROUP) )
		    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		    ugtuple->dbug_group.db_own_name, (DB_OWN_NAME *)NULL, 
		    DB_MAXNAME, SXF_E_SECURITY, I_SX2007_GROUP_CREATE, 
		    SXF_A_SUCCESS | SXF_A_CREATE,
		    &e_error);
	    }
	    else
	    {
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		  ugtuple->dbug_group.db_own_name, (DB_OWN_NAME *)NULL, DB_MAXNAME,
		  SXF_E_SECURITY,I_SX2008_GROUP_MEM_CREATE,
		  SXF_A_SUCCESS | SXF_A_ALTER,
		  &e_error);
	    }
	}
	break;
    
    } /* end for */

    /*
    ** If adding a user to a default group (CREATE/ALTER USER).
    ** translate error messages now.
    */
    if (qeuq_cb->qeuq_agid_mask & QEU_DEFAULT_GROUP)
    {
	if (status == E_DB_WARN)		/* Group did not exist */
	{
	    error = E_QE0230_USRGRP_NOT_EXIST;
	}
	else if (status == E_DB_INFO)	/* User was already a member; OK */
	{
	    status = E_DB_OK;
	    error  = 0;
	}
	else if (status == E_DB_OK)	/* User wasn't a member; WARN */
	{
	    status = E_DB_OK;
	    error  = 0;
	}
    }

    qeu_cb->error.err_code = error;
    return (status);
}

/*{
** Name: QEU_DGROUP	- Destroys group information
**
** Internal	call:   status = qeu_dgroup(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_cagid		number of group tuples
**	    .qeuq_agid_tup	group tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iiusergroup
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0162_GRPID_ANCHOR_MISSING
**				E_QE0163_GRPMEM_PURGE
**				E_QE0164_GRPID_PURGE
**				E_QE0211_DEFAULT_GROUP
**				E_QE0212_DEFAULT_MEMBER
**				E_QE0214_GRPID_NOT_EXIST
**				E_QE0216_GRPMEM_NOT_EXIST
**				E_QE0217_GRPID_NOT_EMPTY
**				E_QE0228_DBPRIVS_REMOVED
**				E_QE022A_IIUSERGROUP
**				E_QE022C_IIDBPRIV
**				E_QE022E_IIUSER
**	Returns:
**	    E_DB_OK
**		Processing completed normally.
**	    E_DB_INFO
**		For ALTER GROUP...DROP, the user does not exist in the group,
**		or the group is the user's default, as specified in iiuser.
**		For DROP GROUP, database privileges were removed.
**	    E_DB_WARN
**		For DROP GROUP, the group does not exist.
**		For DROP GROUP, members existed in one or more groups.
**		For DROP GROUP, the group is default in iiuser
**		For ALTER GROUP...DROP, the group does not exist.
**		For ALTER GROUP...DROP ALL, the group is default in iiuser
**	    E_DB_ERROR
**		IIUSERGROUP, IIUSER, or IIDBPRIV could not be processed.
**		Group anchor is missing.
**		For ALTER GROUP...DROP ALL, unable to recreate anchor
**	    E_DB_SEVERE
**		Control block error:
**		    Request to purge, but tuple was not an anchor.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removes tuples from the iiusergroup catalog.
**
** History:
**	13-mar-89 (ralph)
**          written.
**      29-jun-89 (jennifer)
**          Added security auditing calls.
**	3-sep-93 (stephenb)
**	    altered calls to qeu_secaudit() so that we distinguish between
**	    DROP GROUP and ALTER GROUP DROP ALL when auditing. Also make
**	    removal of a group members(s) SXF_A_ALTER not SXF_A_DROP.
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
*/
DB_STATUS
qeu_dgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_USERGROUP    *ugtuple, antuple;
    DB_STATUS	    status = E_DB_OK;
    DB_ERROR	    e_error;
    i4	    error = 0;
    i4		    i;
    bool	    anchor;
    bool	    purge;
    bool	    drop;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[3];
    DMR_ATTR_ENTRY  *key_ptr_array[3];
    DB_TAB_ID	    tabid;

    for (i=0; i<3; i++)
	key_ptr_array[i] = &key_array[i];

    ugtuple = (DB_USERGROUP *)qeuq_cb->qeuq_agid_tup->dt_data;
    anchor  = ((char *)STskipblank((char *)&ugtuple->dbug_member,
				   (i4)sizeof(DB_OWN_NAME))
		       == (char *)NULL
	      );
    drop    = (qeuq_cb->qeuq_agid_mask & QEU_DGROUP);
    purge   = (qeuq_cb->qeuq_agid_mask & QEU_PGROUP);

    /* We can only purge groups, not members */

    if (!anchor && purge)
    {
	error = E_QE0163_GRPMEM_PURGE;
	status = E_DB_SEVERE;
    }

    /*
    ** Verify that the group anchor exists.
    */

    if (status == E_DB_OK)
    for (;;)
    {
	/* Make sure the group anchor exists */

	qeu_cb->qeu_count = 1;
	qeu_cb->qeu_tup_length = sizeof(DB_USERGROUP);
	qeu_cb->qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_USERGROUP);
	qef_data.dt_data = (PTR)&antuple;
	qeu_cb->qeu_getnext = QEU_REPO;
	qeu_cb->qeu_klen = 2;       
	qeu_cb->qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_1_GRPID_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &ugtuple->dbug_group;
	key_ptr_array[1]->attr_number = DM_2_GRPID_KEY;

	/* Use GTE scan in case we need to check for members */

	key_ptr_array[1]->attr_operator = DMR_OP_GTE;
	key_ptr_array[1]->attr_value = (char *) &antuple.dbug_member;
	MEfill(sizeof(DB_OWN_NAME), (u_char)' ',
	       (PTR)&antuple.dbug_member);
	qeu_cb->qeu_qual = 0;
	qeu_cb->qeu_qarg = 0;
	status = qeu_get(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		error  = E_QE0214_GRPID_NOT_EXIST;
		status = E_DB_WARN;
	    }
	    else
	    {
		error  = E_QE022A_IIUSERGROUP;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;
	}
	else if (qeu_cb->qeu_count != 1) 
	{
	    /* No group anchor or member tuples exist */
	    error  = E_QE0214_GRPID_NOT_EXIST;
	    status = E_DB_WARN;
	    break;
	}
	else if ((char *)STskipblank((char *)&antuple.dbug_member,
				     (i4)sizeof(DB_OWN_NAME))
			!= (char *)NULL)
	{
	    /* Uh oh.... Members exist, but group anchor does not! */
	    error  = E_QE0162_GRPID_ANCHOR_MISSING;
	    status = E_DB_ERROR;
	    break;
	}

	break;
    }

    /*
    ** If dropping a group without purge, 
    ** ensure there are no members in the group.
    */

    if ((status == E_DB_OK) &&
	(anchor && !purge))
    for (;;)
    {
	/*
	** We should be positioned at the anchor.
	** Get the next tuple -- if it's member of this group,
	** reject the drop!
	*/

	qeu_cb->qeu_getnext = QEU_NOREPO;
	qeu_cb->qeu_klen = 0;
	status = qeu_get(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* This is goodness! */
		status = E_DB_OK;
	    }
	    else
	    {
		/* Why did I get here? */
		error  = E_QE022A_IIUSERGROUP;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;
	}
	else
	if (qeu_cb->qeu_count != 1)
	{
	    /* If there are more rows, why wasn't one returned? */
	    error  = E_QE022A_IIUSERGROUP;
	    status = E_DB_ERROR;
	    break;
	}

	/* Is this tuple a member of the group? */

	if (MEcmp((PTR)&ugtuple->dbug_group, (PTR)&antuple.dbug_group,
		  sizeof(DB_OWN_NAME)) == 0)
	{
	    error  = E_QE0217_GRPID_NOT_EMPTY;
	    status = E_DB_WARN;				/* Members exist */
	    break;
	}

	break;
    }

    /*
    ** If dropping or purging a group, make sure no iiuser tuple has
    ** the group in the du_group field.
    ** If dropping a member, ensure the associated iiuser tuple does
    ** not have the group in the du_group field.
    */

    if (status == E_DB_OK)
    {
	status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&ugtuple->dbug_member,
			   (char *)&ugtuple->dbug_group, NULL);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    error = E_QE022E_IIUSER;
	else if (status == E_DB_OK)		/* The group is a default! */
	{
	    if (anchor)
	    {
		error = E_QE0211_DEFAULT_GROUP;
		status = E_DB_WARN;
	    }
	    else
	    {
		error = E_QE0212_DEFAULT_MEMBER;
		status = E_DB_INFO;
	    }
	}
	else
	    status = E_DB_OK;

    }

    /* If all checks out, drop the group and/or member(s) */

    if (status == E_DB_OK)
    for (;;)
    {
	/* Like other QEU routines, this assumes one tuple per call */

	qeu_cb->qeu_getnext = QEU_REPO;
	qeu_cb->qeu_tup_length = sizeof(DB_USERGROUP);
	qeu_cb->qeu_klen = 2;       
	qeu_cb->qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_1_GRPID_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &ugtuple->dbug_group;
	key_ptr_array[1]->attr_number = DM_2_GRPID_KEY;

	/*
	** If we are purging, it's easiest to
	** drop all group records (including anchor),
	** then add anchor record back later.
	*/

	if (purge)
	    key_ptr_array[1]->attr_operator = DMR_OP_GTE;
	else
	    key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value = (char *) &ugtuple->dbug_member;
	qeu_cb->qeu_tup_length = sizeof(DB_USERGROUP);
	qeu_cb->qeu_qual = 0;
	qeu_cb->qeu_qarg = 0;

	/* Delete the tuple(s) from iiusergroup */

	status = qeu_delete(qef_cb, qeu_cb);
	if (status != E_DB_OK)			/* Uh-oh. An error.... */
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		if (anchor)
		{
		    /* I thought I ensured the anchor existed! */
		    error  = E_QE0214_GRPID_NOT_EXIST;
		    status = E_DB_WARN;
		}
		else
		{
		    /* The user tried to drop a non-existing member */
		    error  = E_QE0216_GRPMEM_NOT_EXIST;
		    status = E_DB_INFO;
		}
	    }
	    else
	    {
		/* Who knows why we're here?!.... */
		error = qeu_cb->error.err_code;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;	/* For whatever reason we failed, get outta here! */
	}
	else		/* Make sure *something* was deleted */
	if (( purge && (qeu_cb->qeu_count <= 0)) ||
	    (!purge && (qeu_cb->qeu_count != 1)))
	{
		if (anchor)
		{				/* How did we get here? */
		    /* I thought I ensured the anchor existed! */
		    error  = E_QE0214_GRPID_NOT_EXIST;
		    status = E_DB_WARN;
		}
		else
		{
		    /* The user tried to drop a non-existing member */
		    error  = E_QE0216_GRPMEM_NOT_EXIST;
		    status = E_DB_INFO;
		}
	    break;	/* For whatever reason we failed, get outta here! */
	}
	else
	{
            /*
	    ** Get security label
	    */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		tabid.db_tab_base=DM_B_GRPID_TAB_ID;
		tabid.db_tab_index=DM_I_GRPID_TAB_ID;

		/* Delete happened correctly. */
		if (anchor)		/* Dropping/purging the group */
		{
		    i4		msg_id;
		    i4 	accessmask = SXF_A_SUCCESS;

		    /*
		    ** We must distinguish between dropping and purging for
		    ** auditing
		    */
		    if (purge)
		    {
			msg_id = I_SX200A_GROUP_MEM_DROP;
			accessmask |= SXF_A_ALTER;
		    }
		    else
		    {
			msg_id = I_SX2009_GROUP_DROP;
			accessmask |= SXF_A_DROP;
		    }
		    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		       ugtuple->dbug_group.db_own_name, (DB_OWN_NAME *)NULL, 
		       DB_MAXNAME, SXF_E_SECURITY, 
		       msg_id, accessmask,
		       &e_error);
		}
		else		/* Dropping specific member */
		{
		    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		       ugtuple->dbug_group.db_own_name, (DB_OWN_NAME *)NULL, 
		       DB_MAXNAME, SXF_E_SECURITY, I_SX200A_GROUP_MEM_DROP,
		       SXF_A_SUCCESS | SXF_A_ALTER, 
		       &e_error);
		}
	    }

	    if (purge && !drop)	/* Add anchor back if purging w/o drop */
	    {
		status = qeu_cgroup(qef_cb, qeuq_cb, qeu_cb);
		if (status != E_DB_OK)
		{
		    /* Uh oh.... We couldn't recreate the group anchor */
		    error  = E_QE0164_GRPID_PURGE;
		    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		}
		break;
	    }
	}
    
	break;
    }

    /* If dropping a group, delete any database privileges for the group */

    if ((status == E_DB_OK) && anchor && drop)
    {
	i2  gtype = DBGR_GROUP;

	status = qeu_pdbpriv(qef_cb, qeuq_cb,
			     (char *)&ugtuple->dbug_group, (char *)&gtype);

	if (DB_FAILURE_MACRO(status))
	    error = E_QE022C_IIDBPRIV;
	else if (status != E_DB_OK)
	    error = E_QE0228_DBPRIVS_REMOVED;

    }

    qeu_cb->error.err_code = error;
    return (status);
}

/*{
** Name: QEU_GGROUP- Obtain tuple from iiusergroup
**
** Internal QEF call:   status = qeu_ggroup(&qef_cb, &qeuq_cb,
**					   &group, &member, &ugtuple)
**
** Description:
**	This procedure obtains requested tuple from the iiusergroup catalog.
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
**	group			the group's name
**				if null or blank, a scan is performed.
**	member			the member's name
**				if null, blanks are used.
**	ugtuple			the iiusergroup tuple buffer
**				if null, only status is returned.
**
** Outputs:
**	ustuple			filled with requested iiusergroup tuple
**
**	Returns:
**	    E_DB_OK		Success; tuple was found
**	    E_DB_INFO		Success; tuple was not found.
**	    E_DB_ERROR		Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	18-may-89 (ralph)
**          written
**	17-jun-92 (andre)
**	    init QEU_CB.qeu_flag before calling qeu_get() as it will be looking
**	    for QEU_BY_TID
*/
DB_STATUS
qeu_ggroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*group,
char		*member,
DB_USERGROUP	*ugtuple)
{
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    QEF_DATA	    qef_data;
    DB_USERGROUP    altuple, qualtuple;
    i4		    audit = FALSE;
    DMR_ATTR_ENTRY  key_array[2];
    DMR_ATTR_ENTRY  *key_ptr_array[2];

    key_ptr_array[0] = &key_array[0];
    key_ptr_array[1] = &key_array[1];

    for (;;)
    {
	/* Open the iiusergroup table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_GRPID_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_GRPID_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_flag = 0;
	qeu.qeu_mask = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to query the iiusergroup table */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DB_USERGROUP);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_USERGROUP);
	qeu.qeu_getnext = QEU_REPO;

	/*
	** If member is null, use blanks.
	*/

	if (member == NULL)
	    MEfill(sizeof(qualtuple.dbug_member), (u_char)' ',
		   (PTR)&qualtuple.dbug_member);
	else
	    MEmove(DB_MAXNAME, (PTR) member, (char)' ',
		   sizeof(qualtuple.dbug_member),
		   (PTR)&qualtuple.dbug_member);

	/*
	** If group is null or blank, scan entire iiusergroup table.
	** Otherwise, look up specific group tuple.
	*/

	if ((group == NULL) ||
	    (STskipblank(group, (i4)DB_MAXNAME) == NULL)
	   )
	{
	    qeu.qeu_klen = 0;
	    qeu.qeu_key = NULL;
	    qeu.qeu_qual = qeu_qgroup;
	    qeu.qeu_qarg = &qparams;
	    qparams.qeu_qparms[0]	= (PTR)qef_cb;
	    qparams.qeu_qparms[1]	= (PTR)&qualtuple;
	    qparams.qeu_qparms[2]	= (PTR)&audit;
	}
	else
	{
	    MEmove(DB_MAXNAME, (PTR) group, (char)' ',
		   sizeof(qualtuple.dbug_group),
		   (PTR)&qualtuple.dbug_group);
	    qeu.qeu_klen = 2;
	    qeu.qeu_key = key_ptr_array;
	    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    key_ptr_array[0]->attr_number   = DM_1_GRPID_KEY;
	    key_ptr_array[0]->attr_value    = (char *)&qualtuple.dbug_group;
	    key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    key_ptr_array[1]->attr_number   = DM_2_GRPID_KEY;
	    key_ptr_array[1]->attr_value    = (char *)&qualtuple.dbug_member;
	    qeu.qeu_qual = NULL;
	    qeu.qeu_qarg = NULL;
	}

	/* If output tuple is null, use our own storage */

	if (ugtuple == NULL)
	    qef_data.dt_data = (PTR)&altuple;
	else
	    qef_data.dt_data = (PTR) ugtuple;

	/* Get a tuple from iiusergroup */

	status = qeu_get(qef_cb, &qeu);
	
	/* Check the result */

	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count == 0)
	    status = E_DB_INFO;
	else
	    status = E_DB_OK;

	/* Close the iiusergroup table */

	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	    local_status = (local_status > E_DB_ERROR) ?
			    local_status : E_DB_ERROR;

	/* Set the status and exit */

	status = (status > local_status) ? status : local_status;
	
	break;
    }

    return(status);
}

/*{
** Name: QEU_PGROUP - Remove all iiusergroup tuples associated with a member.
**
** Internal QEF call:   status = qeu_pgroup(&qef_cb, &qeuq_cb, &member)
**
** Description:
**	This procedure obtains requested tuple from the iiusergroup catalog.
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
**	member			the member's name
**
** Outputs:
**	None
**
**	Returns:
**	    E_DB_OK		Success; no tuples were deleted.
**	    E_DB_INFO		Success; one or more tuples deleted.
**	    E_DB_ERROR		Failure.
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	26-oct-89 (ralph)
**          written
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
**	06-Aug-2002 (jenjo02)
**	    Init qeu_mask before calling qeu_open() lest DMF think we're
**	    passing bogus lock information (QEU_TIMEOUT,QEU_MAXLOCKS).
*/
DB_STATUS
qeu_pgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*member)
{
    i4		    i;
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    DB_USERGROUP    qualtuple;
    DMR_ATTR_ENTRY  key_array[3];
    DMR_ATTR_ENTRY  *key_ptr_array[3];
    i4		    audit;
    DB_TAB_ID	    tabid;
    i4	    error;

    for (i=0; i<3; i++)
	key_ptr_array[i] = &key_array[i];

    for (;;)
    {
        /*
	** Get security label, used when auditing
	*/
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    audit = TRUE;
	
	    tabid.db_tab_base=DM_B_GRPID_TAB_ID;
	    tabid.db_tab_index=DM_I_GRPID_TAB_ID;
	}
	else
	    audit = FALSE;

	/* Open the iiusergroup table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_GRPID_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_GRPID_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_flag = QEU_BYPASS_PRIV;
	qeu.qeu_lk_mode = DMT_IX;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to process the iiusergroup table */

	qeu.qeu_getnext = QEU_REPO;

	qeu.qeu_tup_length = sizeof(DB_USERGROUP);
	qeu.qeu_klen = 2;       
	qeu.qeu_key = key_ptr_array;
	qeu.qeu_qual = qeu_qgroup;
	qeu.qeu_qarg = &qparams;
	qparams.qeu_qparms[0]	= (PTR)qef_cb;
	qparams.qeu_qparms[1]	= (PTR)&qualtuple;
	qparams.qeu_qparms[2]	= (PTR)&audit;

	key_ptr_array[0]->attr_number = DM_1_GRPID_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_GTE;
	key_ptr_array[0]->attr_value = (char *) &qualtuple.dbug_group;
	key_ptr_array[1]->attr_number = DM_2_GRPID_KEY;
	key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value = (char *) &qualtuple.dbug_member;

	MEfill(sizeof(DB_OWN_NAME), (u_char)' ',
	       (PTR)&qualtuple.dbug_group);

	MEmove(DB_MAXNAME, (PTR) member, (char)' ',
	       sizeof(qualtuple.dbug_member),
	       (PTR)&qualtuple.dbug_member);

	/* Delete tuples from iiusergroup */

	status = qeu_delete(qef_cb, &qeu);

	/* Check the result */

	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count == 0)
	    status = E_DB_OK;
	else
	    status = E_DB_INFO;

	/* Close the iiusergroup table */

	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	    local_status = (local_status > E_DB_ERROR) ?
			    local_status : E_DB_ERROR;

	/* Set the status and exit */

	status = (status > local_status) ? status : local_status;
	
	break;
    }

    return(status);
}

/*{
** Name: QEU_QGROUP- Qualify an iiusergroup tuple by member with audit
**
** External QEF call:   status = (*que_qual)(qeu_qarg, &current_tuple)
**
** Description:
**	This procedure qualifies an iiusergroup tuple by member name.
**	It is called by DMF as part of tuple qualification when
**	deleting tuples for qeu_duser.
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	Current qef_cb
**	    .qeu_qparms[1]	Qualification tuple
**	    .qeu_qparms[2]	Points to TRUE if auditing record
**	    .qeu_rowaddr	Row being qualified
**
** Outputs:
**	qparams.qeu_retval	ADE_TRUE if match, else ADE_NOT_TRUE
**
**	Returns:
**	    E_DB_OK or E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    Produces audit records indicating the member is dropped 
**	    from the group if audit is TRUE.
**
** History:
**	18-may-89 (ralph)
**          written
**	25-sep-90 (ralph)
**	    added auditing
**	03-feb-94 (rickh)
**	    Fixed up an error return from a security audit call in qeu_qgroup
**	    to shut up a compiler.
**	11-Apr-2008 (kschendel)
**	    Revised the call sequence.
*/
static DB_STATUS
qeu_qgroup(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    QEF_CB          *qef_cb;
    DB_USERGROUP    *qual_tuple;
    DB_USERGROUP    *group_tuple;
    i4		    *audit;
    i4	    error = 0;
    DB_STATUS	    status;

    group_tuple = (DB_USERGROUP *) qparams->qeu_rowaddr;
    qef_cb = (QEF_CB *) qparams->qeu_qparms[0];
    qual_tuple = (DB_USERGROUP *) qparams->qeu_qparms[1];
    audit = (i4 *) qparams->qeu_qparms[2];

    qparams->qeu_retval = ADE_NOT_TRUE;

    if (qual_tuple == NULL || group_tuple == NULL)	/* Bad parm; abort! */

	return(E_DB_ERROR);

    if (MEcmp((PTR)&qual_tuple->dbug_member, 
	      (PTR)&group_tuple->dbug_member,
	      sizeof(qual_tuple->dbug_member))
	    != 0)

	return(E_DB_OK);				/* Unqualified.... */

    /* The tuple is qualified.  Audit if reqested */

    qparams->qeu_retval = ADE_TRUE;
    status = E_DB_OK;
    if ( *audit && Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	DB_ERROR	e_error;

	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
                group_tuple->dbug_group.db_own_name,(DB_OWN_NAME *)NULL, DB_MAXNAME,
                SXF_E_SECURITY, I_SX200A_GROUP_MEM_DROP,
		SXF_A_SUCCESS | SXF_A_ALTER, 
		&e_error);
    }

    return(status);

}

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
#include    <usererror.h>

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
/*
**
**  Name: QEUPROF.C - routines for maintaining profiles
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iiprofile catalog.
**
**	The following routine(s) are defined in this file:
**
**          qeu_aprofile   - alter a profile tuple
**	    qeu_cprofile   - appends a profile tuple
**	    qeu_dprofile   - deletes a profile tuple
**	    qeu_gprofile   - gets a requested profile tuple
**
**  History:
**	27-aug-93 (robf)
**	    Written 
**	3-nov-93 (robf)
**          Pass security label when auditing, since groups don't have a
**	    distinct label currently we use the label for iigroup.
**	3-feb-94 (robf)
**          Change DM_1_USER_KEY to DM_1_IIPROFILE_KEY when looking
**          up values in iiprofile (both are 1 currently, but should use
**	    the right one in case they change later)
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
**      18-Apr-2001 (hweho01)
**          Fixed the argument type mismatch in qef_error() call,
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**	11-Apr-2008 (kschendel)
**	    Update DMF row qualification calls.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes: tighten function-pointer prototypes.
**/

/*
**  Definition of static variables and forward static functions 
*/
typedef struct  {
	QEF_CB          *qef_cb;
	QEUQ_CB		*qeuq_cb;
	DU_PROFILE *old_ptuple;
	DU_PROFILE *new_ptuple;
} QEU_PROF_OLD_NEW_PROF;

static DB_STATUS qual_user_profile(
	void		*toss,
	QEU_QUAL_PARAMS	*qparams);

static i4  update_user_profile( void *, void * );

/*
** Default profile name, all blanks currently
*/
#define DEFAULT_PROFILE_NAME "                                "

/*{
** Name: qeu_cprofile 	- Store profile information for one profile.
**
** External QEF call:   status = qef_call(QEU_CPROFILE, &qeuq_cb);
**
** Description:
**	Add the profile tuple to the iiprofile table.
**
**	The profile tuple was filled from the CREATE profile statement, except
**	that since this is the first access to iiprofile, the uniqueness
**	of the profile name (within the current profile scope) is validated 
**	here.
**	First the iiprofile table is retrieved by profile name to insure
**      that the profile name provided is unqiue.
**
**	If a match is found then the profile is a duplicate and an error is
**	returned.  If this is unique then the profile is entered into 
**	iiprofile.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for profile errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to profile.
**	    .qeuq_culd		Number of profile tuples.  Must be 1.
**	    .qeuq_uld_tup	profile tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US18B6_6326_USER_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	27-aug-93 (robf)
**	    Written 
**	13-oct-93 (robf)
**          Fix prototype problems.
**	18-jul-94 (robf)
**          Initialize status to E_DB_OK. 
*/

DB_STATUS
qeu_cprofile(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_PROFILE     	*ptuple;	/* New profile tuple */
    DU_PROFILE     	ptuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status=E_DB_OK, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	ukey_array[2];
    DMR_ATTR_ENTRY  	*ukey_ptr_array[2];

    bool		group_specified;

    ptuple = (DU_PROFILE *)qeuq_cb->qeuq_uld_tup->dt_data;

    /* Ensure no name collision */

    for (;;)
    {
	/* Validate CB and parameters */
	if (!qef_cb ||
	    qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	/*
	** If "default" profile don't check for other name collisions
	*/
	if(MEcmp((PTR)&ptuple->du_name,
		(PTR)DEFAULT_PROFILE_NAME,
		GL_MAXNAME)!=0)
	{
            /* Check the iiusergroup catalog to ensure no name collision */

            status = qeu_ggroup(qef_cb, qeuq_cb,
                           (char *)&ptuple->du_name, NULL, NULL);

            if (DB_FAILURE_MACRO(status))               /* Fatal error! */
                error = E_QE022A_IIUSERGROUP;
            else if (status == E_DB_OK)                 /* Group exists! */
            {
	        (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    7, "profile",
			    qec_trimwhite(sizeof(ptuple->du_name),
					  (char *)&ptuple->du_name),
			    (PTR)&ptuple->du_name,
			    5, "group");
	        error = E_QE0025_USER_ERROR;
	        status = E_DB_WARN;
            }
            else                                        
                status = E_DB_OK;

            /* Terminate if error occurred */

            if (status != E_DB_OK)
                break;

            /* Check the iirole catalog to ensure no name collision */

            status = qeu_gaplid(qef_cb, qeuq_cb,
                           (char *)&ptuple->du_name, NULL);

            if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
                error = E_QE022B_IIAPPLICATION_ID;
            else if (status == E_DB_OK)		    /* Role exists! */
            {
	        (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    7, "profile",
			    qec_trimwhite(sizeof(ptuple->du_name),
					  (char *)&ptuple->du_name),
			    (PTR)&ptuple->du_name,
			    4, "role");
	        error = E_QE0025_USER_ERROR;
	        status = E_DB_WARN;
            }
            else		    /* Role does not exist */
                status = E_DB_OK;

	    /* Check the iiuser catalog to ensure no name collision */

	    status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&ptuple->du_name, NULL, NULL);

	    if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
	        qeuq_cb->error.err_code = E_QE022E_IIUSER;
	    else if (status == E_DB_OK)		    /* User exists! */
	    {
	        (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    7, "profile",
			    qec_trimwhite(sizeof(ptuple->du_name),
					  (char *)&ptuple->du_name),
			    (PTR)&ptuple->du_name,
			    4, "user");
	        error = E_QE0025_USER_ERROR;
	        status = E_DB_WARN;
	    }
	    else		    /* User does not exist */
	        status = E_DB_OK;
        }
	/*
	** If DEFAULT=ALL specified make that true
	*/
	if ( ptuple->du_flagsmask & DU_UDEFALL)
	{
		ptuple->du_defpriv = (ptuple->du_status& DU_UALL_PRIVS);

	}
	/* Make sure default privs are a subset of base privs */
	if(ptuple->du_defpriv)
	{
		if((ptuple->du_defpriv & ptuple->du_status ) !=
			ptuple->du_defpriv)
		{
			(VOID)qef_error(E_US1967_6503_DEF_PRIVS_NSUB,0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 0);
			error = E_QE0025_USER_ERROR;
			status= E_DB_ERROR;
			break;
		}
	}
        break;
    }

    /* If no errors, attempt to add the profile */

    if (status == E_DB_OK)
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
	** transaction, otherwise we'll use the profile's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = (CS_SID) qeuq_cb->qeuq_d_id;
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


	group_specified = (STskipblank((char *)&ptuple->du_group,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char *)NULL);

	status = E_DB_OK;
    
	/* Validate that the profile name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_IIPROFILE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_IIPROFILE_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = qeu_open(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    (VOID)qef_error(qeu.error.err_code, 0L,
		    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0282_IIPROFILE;
	    status = E_DB_WARN;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named profile - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_PROFILE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_PROFILE);
        qef_data.dt_data = (PTR)&ptuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = ukey_ptr_array;
	ukey_ptr_array[0] = &ukey_array[0];
	ukey_ptr_array[1] = &ukey_array[1];
	ukey_ptr_array[0]->attr_number = DM_1_IIPROFILE_KEY;
	ukey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ukey_ptr_array[0]->attr_value = (char *)&ptuple->du_name;
					
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	local_status = qeu_get(qef_cb, &qeu);
	if (local_status == E_DB_OK)		/* Found the same profile! */
	{
	    (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    7, "profile",
			    qec_trimwhite(sizeof(ptuple->du_name),
					  (char *)&ptuple->du_name),
			    (PTR)&ptuple->du_name,
			    7, "profile");
	    error = E_QE0025_USER_ERROR;
	    status = (status > E_DB_ERROR ? status : E_DB_ERROR);
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    status = (status > E_DB_ERROR ? status : E_DB_ERROR);
	    break;
	}

	/*
	** Check group exists
	*/

	if (group_specified)
	{
		status = qeu_ggroup(qef_cb, qeuq_cb,
				   (char *)&ptuple->du_group, NULL, NULL);

		if (DB_FAILURE_MACRO(status))               /* Fatal error! */
		{
		    error = E_QE022A_IIUSERGROUP;
		    break;
		}
		else if (status != E_DB_OK)                 
		{
		     (VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 2,
				    5, "Group",
				    qec_trimwhite(sizeof(ptuple->du_group),
						(char *)&ptuple->du_group),
				    (PTR)&ptuple->du_group);
		    error = E_QE0025_USER_ERROR;
		    status = E_DB_WARN;
		}
		else                                        
		    status = E_DB_OK;
		if(status!=E_DB_OK)
			break;
	}

	/* Insert single profile tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_PROFILE);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
	local_status = qeu_append(qef_cb, &qeu);
        if ((qeu.qeu_count == 0) && (local_status == E_DB_OK))
        {
	    (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    7, "profile",
			    qec_trimwhite(sizeof(ptuple->du_name),
					  (char *)&ptuple->du_name),
			    (PTR)&ptuple->du_name,
			    7, "profile");
	    error = E_QE0025_USER_ERROR;
	    status = (status > E_DB_ERROR ? status : E_DB_ERROR);
	    break;
	}
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	local_status = qeu_close(qef_cb, &qeu);    
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* This is a security event, must audit. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&ptuple->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(ptuple->du_name), SXF_E_SECURITY,
			    I_SX272E_CREATE_PROFILE, SXF_A_SUCCESS | SXF_A_CREATE,
			    &e_error);
	    error = e_error.err_code;

	    status = (status > local_status ? status : local_status);
	    if (local_status != E_DB_OK)
		break;
	}

	if (transtarted)		   /* If we started a transaction */
	{
	    local_status = qeu_etran(qef_cb, &tranqeu);
	    status = (status > local_status ? status : local_status);
	    if (local_status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}
	qeuq_cb->error.err_code = E_QE0000_OK;
	return (status);
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
} /* qeu_cprofile */

/*{
** Name: qeu_aprofile 	- Alter profile information for one profile.
**
** External QEF call:   status = qef_call(QEU_APROFILE, &qeuq_cb);
**
** Description:
**	Change the profile tuple to the iiprofile table.
**
**      This is done with a drop followed by a replace.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for profile errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to profile.
**	    .qeuq_culd		Number of profile tuples.  Must be 1.
**	    .qeuq_uld_tup	profile tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US189D_6301_USER_ABSENT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	27-aug-93 (robf)
**	    Written 
**	29-nov-93 (robf)
**	    ALTER PROFILE WITH NOPRIVILEGES now follows DROP PRIVILEGES in
**	    updating  the default privileges (if any) in sync, also same as
**	    ALTER USER.
**	18-jul-94 (robf)
**          Initialize status to E_DB_OK
**	12-oct-2001 (toumi01)
**	    Set bool group_specified based on ptuple->du_group being non-blank
**	    rather than on DU_UGROUP setting, since group_specified drives
**	    catalog validation of the group id.  DU_UGROUP may be set if
**	    NOGROUP has been specified in the syntax.  ALTER PROFILE ...
**	    NOGROUP will fail with the old test if there are no groups at all
**	    stored in the catalog.
*/

DB_STATUS
qeu_aprofile(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_PROFILE     	*ptuple;	/* New profile tuple */
    DU_PROFILE     	ptuple_temp;	/* Tuple to check for uniqueness */
    DU_PROFILE     	old_ptuple;	
    DB_STATUS	    	status=E_DB_OK, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	ukey_array[2];
    DMR_ATTR_ENTRY  	*ukey_ptr_array[2];

    bool		group_specified;

    bool		def_priv_all=FALSE;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (!qef_cb ||
	    qeuq_cb->qeuq_type != QEUQCB_CB ||
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
	    tranqeu.qeu_d_id = (CS_SID) qeuq_cb->qeuq_d_id;
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

	ptuple 	     = (DU_PROFILE *)qeuq_cb->qeuq_uld_tup->dt_data;

	group_specified = (STskipblank((char *)&ptuple->du_group,
					(i4)sizeof(DB_OWN_NAME))
				!= (char *)NULL);

	status = E_DB_OK;
    
	/* Validate that the profile name is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_IIPROFILE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_IIPROFILE_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = qeu_open(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    (VOID)qef_error(qeu.error.err_code, 0L,
		    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0282_IIPROFILE;
	    status = E_DB_WARN;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named profile */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_PROFILE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_PROFILE);
        qef_data.dt_data = (PTR)&ptuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = ukey_ptr_array;
	ukey_ptr_array[0] = &ukey_array[0];
	ukey_ptr_array[1] = &ukey_array[1];
	ukey_ptr_array[0]->attr_number = 1;
	ukey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ukey_ptr_array[0]->attr_value =
				    (char *)&ptuple->du_name;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	local_status = qeu_get(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
			(VOID)qef_error(E_QE0286_PROFILE_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(ptuple->du_name),
						(char *)&ptuple->du_name),
				    (PTR)&ptuple->du_name);
			error = E_QE0025_USER_ERROR;
	    }
	    else	/* Other error */
	    {
		error = qeu.error.err_code;
	    }
	    break;		
	}

	/*
	** Save the old profile unchanged for later
	*/
	STRUCT_ASSIGN_MACRO(ptuple_temp, old_ptuple);
	/*
	** Add the profile to the group first, so we'll know if 
	** we should leave the default group as is.
	*/

	if (group_specified)
	{
		status = qeu_ggroup(qef_cb, qeuq_cb,
				   (char *)&ptuple->du_group, NULL, NULL);

		if (DB_FAILURE_MACRO(status))               /* Fatal error! */
		{
		    error = E_QE022A_IIUSERGROUP;
		    break;
		}
		else if (status != E_DB_OK)                 
		{
		     (VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 2,
				    5, "Group",
				    qec_trimwhite(sizeof(ptuple->du_group),
						(char *)&ptuple->du_group),
				    (PTR)&ptuple->du_group);
		    error = E_QE0025_USER_ERROR;
		    status = E_DB_WARN;
		}
		else                                        
		    status = E_DB_OK;

		if(status!=E_DB_OK)
			break;
	}

	/* Found the profile, merge the information. */

     	if (ptuple->du_flagsmask & DU_UPRIV)
	{
	    /*
	    ** Preserve security audit bits in status
	    */
	    ptuple_temp.du_status = 
			(ptuple->du_status |
			(ptuple_temp.du_status&(DU_UAUDIT|DU_UAUDIT_QRYTEXT))); 

	    if((ptuple->du_status&DU_UALL_PRIVS)==0)
	    {
	       /* No privileges, so make default empty (same as
	       ** dropping all the privs).
	       */
	       ptuple_temp.du_defpriv=0;
	    }
	}
        else if (ptuple->du_flagsmask & DU_UAPRIV)
	    ptuple_temp.du_status |= ptuple->du_status; 
        else if (ptuple->du_flagsmask & DU_UDPRIV)
	{
	    ptuple_temp.du_status &= ~ptuple->du_status; 
	    /* Remove from default privs also */
	    ptuple_temp.du_defpriv &= ~ptuple->du_status; 
	}
	/*
	** Check to see if old default privileges equalled full
	** set. If so, we add any new privs to default set. This maintains
	** backwards compatibility. We have to check this here prior
	** to changing the profile status further on.
	*/
	if ((ptuple_temp.du_status &  DU_UALL_PRIVS) == ptuple_temp.du_defpriv
	   && !(ptuple->du_flagsmask & DU_UDEFPRIV)
	   )
	{
		def_priv_all=TRUE;
	}
	/* Check security audit status */
	if (ptuple->du_flagsmask & DU_UQRYTEXT)
	    ptuple_temp.du_status |= DU_UAUDIT_QRYTEXT;
	
	if (ptuple->du_flagsmask & DU_UNOQRYTEXT)
	    ptuple_temp.du_status &= ~DU_UAUDIT_QRYTEXT;
	
	if (ptuple->du_flagsmask & DU_UALLEVENTS)
	    ptuple_temp.du_status |= DU_UAUDIT;
	
	if (ptuple->du_flagsmask & DU_UDEFEVENTS)
	    ptuple_temp.du_status &= ~DU_UAUDIT;
	
	/* Default privileges */
	if (ptuple->du_flagsmask & DU_UDEFPRIV)
	{
	    /* User specified default privileges */
	    ptuple_temp.du_defpriv = ptuple->du_defpriv; 
	}
	else if(def_priv_all)
	{
	    /*
	    ** No specified default privs, and need to keep 
	    ** default same as full set.
	    ** Remember to mask out any non-privilege status bits
	    */
	    ptuple_temp.du_defpriv = (ptuple_temp.du_status& DU_UALL_PRIVS);
	}

	if(!ptuple_temp.du_defpriv)
		ptuple_temp.du_flagsmask &= ~DU_UDEFPRIV;

	/*
	** If DEFAULT=ALL specified make that true
	*/
	if ( ptuple->du_flagsmask & DU_UDEFALL)
	{
	    ptuple_temp.du_defpriv = (ptuple_temp.du_status& DU_UALL_PRIVS);

	}
	else 
	{
	    ptuple_temp.du_flagsmask&= ~DU_UDEFALL;
	}

	/* Make sure default privs are a subset of base privs */
	if(ptuple_temp.du_defpriv)
	{
		if((ptuple_temp.du_defpriv & ptuple_temp.du_status ) !=
			ptuple_temp.du_defpriv)
		{
			(VOID)qef_error(E_US1967_6503_DEF_PRIVS_NSUB,0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 0);
			error = E_QE0025_USER_ERROR;
			status= E_DB_ERROR;
			break;
		}
	}

	if (ptuple->du_flagsmask & DU_UEXPDATE)
	    MECOPY_CONST_MACRO((PTR)&ptuple->du_expdate, sizeof(DB_DATE), 
				(PTR)&ptuple_temp.du_expdate);

     	if (ptuple->du_flagsmask & DU_UGROUP)
	{
            STRUCT_ASSIGN_MACRO(ptuple->du_group, ptuple_temp.du_group);
	}


	qeu.qeu_getnext = QEU_NOREPO;
        qeu.qeu_klen = 0;
	    
	/* 
        ** Delete old one, append new one. 
	*/
	local_status = qeu_delete(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}

	/* Insert single profile tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_PROFILE);
	qeu.qeu_input = &qef_data;
	local_status = qeu_append(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	/*
	** Propagate changes to any user tuples with this profile
	*/
	status=qeu_profile_user(qef_cb, qeuq_cb, &old_ptuple, &ptuple_temp);
	if(status!=E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	local_status = qeu_close(qef_cb, &qeu);    
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&ptuple->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(ptuple->du_name), SXF_E_SECURITY,
			    I_SX272F_ALTER_PROFILE, SXF_A_SUCCESS | SXF_A_ALTER,
			    &e_error);

	    error = e_error.err_code;

	    status = (status > local_status ? status : local_status);
	    if (local_status != E_DB_OK)
		break;
	}

	if (transtarted)		   /* If we started a transaction */
	{
	    local_status = qeu_etran(qef_cb, &tranqeu);
	    status = (status > local_status ? status : local_status);
	    if (local_status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}
	qeuq_cb->error.err_code = E_QE0000_OK;
	return (status);
    } /* End dummy for */

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, local_status, &error, &qeuq_cb->error, 0);
    
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
	    (VOID) qef_error(tranqeu.error.err_code, 0L, status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_aprofile */

/*{
** Name: qeu_dprofile 	- Drop profile.
**
** External QEF call:   status = qef_call(QEU_DPROFILE, &qeuq_cb);
**
** Description:
**	Drop the profile tuple, possibly doing  cascaded deletes.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for profile errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to profile.
**	    .qeuq_culd		Must be 1 if DROP profile, 0 if DROP TABLE.
**	    .qeuq_uld_tup	profile tuple to be deleted - if DROP profile, 
**				otherwise this is ignored:
**		.du_name	profile name.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US189D_6301_profile_ABSENT
**				E_US18A8_6312_profile_DRPTAB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	27-aug-93 (robf)
**	    Written 
**	25-feb-94 (robf)
**          Cut & paste error: change IIPROFILE to IIUSER when 
**	    checking RESTRICT condition.
**	18-jul-94 (robf)
**          Initialize status to E_DB_OK
*/

DB_STATUS
qeu_dprofile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;	    
    QEU_CB	    	pqeu;		/* For iiprofile table */
    QEF_DATA	    	uqef_data;
    QEF_DATA	    	qef_data;
    DU_PROFILE	    	*ptuple_name;	/* Input tuple with name. */
    DU_PROFILE	    	ptuple;		/* Tuple currently being deleted */
    bool	    	profile_opened = FALSE;
    DMR_ATTR_ENTRY  	key_array[2];
    DMR_ATTR_ENTRY  	*key_ptr_array[2];
    DB_STATUS	    	status=E_DB_OK, local_status;
    DB_ERROR		e_error;
    i4	    	error;

    bool	    	usr_opened = FALSE;
    DU_USER		usr_tuple;
    QEF_DATA		usrqef_data;
    QEU_CB		usrqeu;
    QEU_QUAL_PARAMS	qparams;
    bool		cascade=FALSE;

    ptuple_name = (DU_PROFILE *)qeuq_cb->qeuq_uld_tup->dt_data;

    if(ptuple_name->du_flagsmask&DU_UCASCADE)
	cascade=TRUE;
    else
	cascade=FALSE;

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
	if (   (qeuq_cb->qeuq_culd == 0)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

        if(MEcmp((PTR)&ptuple_name->du_name,
		(PTR)DEFAULT_PROFILE_NAME,
		GL_MAXNAME)==0)
	{
		/* Can't drop the default profile */
		(VOID)qef_error(E_QE0288_DROP_DEFAULT_PROFILE, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
		error = E_QE0025_USER_ERROR;
		status=E_DB_WARN;
		break;
	}

	/* 
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the profile's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = (CS_SID) qeuq_cb->qeuq_d_id;
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


	/* Open iiprofile. */
	pqeu.qeu_type = QEUCB_CB;
        pqeu.qeu_length = sizeof(QEUCB_CB);
        pqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        pqeu.qeu_lk_mode = DMT_IX;
        pqeu.qeu_flag = DMT_U_DIRECT;
        pqeu.qeu_access_mode = DMT_A_WRITE;

        pqeu.qeu_tab_id.db_tab_base = DM_B_IIPROFILE_TAB_ID;    /* Open iiprofile */
        pqeu.qeu_tab_id.db_tab_index = DM_I_IIPROFILE_TAB_ID;
	pqeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &pqeu);
	if (status != E_DB_OK)
	{
	    (VOID)qef_error(pqeu.error.err_code, 0L,
		    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0282_IIPROFILE;
	    status = E_DB_WARN;
	    break;
	}
	profile_opened = TRUE;
	/*
	** Get current profile tuple
	*/
	pqeu.qeu_count = 1;
        pqeu.qeu_tup_length = sizeof(DU_PROFILE);
	pqeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_PROFILE);
        qef_data.dt_data = (PTR)&ptuple;
	pqeu.qeu_getnext = QEU_REPO;
	pqeu.qeu_klen = 1;			
	pqeu.qeu_key = key_ptr_array;
	key_ptr_array[0] = &key_array[0];
	key_ptr_array[1] = &key_array[1];
	key_ptr_array[0]->attr_number = 1;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value =
				    (char *)&ptuple_name->du_name;
	pqeu.qeu_qual = NULL;
	pqeu.qeu_qarg = NULL;
	local_status = qeu_get(qef_cb, &pqeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    if (pqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
			(VOID)qef_error(E_QE0286_PROFILE_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(ptuple_name->du_name),
						(char *)&ptuple_name->du_name),
				    (PTR)&ptuple_name->du_name);
			error = E_QE0025_USER_ERROR;
	    }
	    else	/* Other error */
	    {
		error = pqeu.error.err_code;
	    }
	    break;		
	}

	/* Check if profile is in use already by some users 
	** - If REJECT, error
	** - If CASCADE, need to reset user profile to default
	*/
	/* Open iiuser */
	usrqeu.qeu_type = QEUCB_CB;
        usrqeu.qeu_length = sizeof(QEUCB_CB);
        usrqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        usrqeu.qeu_lk_mode = DMT_IS;
	usrqeu.qeu_access_mode = DMT_A_READ;
	usrqeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
	usrqeu.qeu_tab_id.db_tab_index  = DM_I_USER_TAB_ID;
	usrqeu.qeu_mask = 0;
	usrqeu.qeu_flag = 0;

	status = qeu_open(qef_cb, &usrqeu);
	if (status != E_DB_OK)
	{
	    error = E_QE022E_IIUSER;
	    break;
	}
	usr_opened = TRUE;

	/* Scan iiuser for users with this profile */
        usrqeu.qeu_count = 1;
        usrqeu.qeu_tup_length = sizeof(DU_USER);
        usrqeu.qeu_output = &usrqef_data;
	usrqef_data.dt_next = NULL;
	usrqef_data.dt_size = sizeof(DU_USER);
	usrqef_data.dt_data = (PTR)&usr_tuple;
        usrqeu.qeu_getnext = QEU_REPO;
        usrqeu.qeu_klen = 0;
	qparams.qeu_qparms[0] = (PTR) &ptuple; /* for current profile name */
	usrqeu.qeu_qual=qual_user_profile;
	usrqeu.qeu_qarg= &qparams;

        status = qeu_get(qef_cb, &usrqeu);
        if ((status != E_DB_OK) &&
            (usrqeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	   )
        {
	    error = E_QE022E_IIUSER;
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
            break;
	}

        if (status == E_DB_OK && !cascade)
	{
	    (VOID)qef_error(E_QE0287_PROFILE_IN_USE, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ptuple_name->du_name),
			    (char *)&ptuple_name->du_name),
			    (PTR)&ptuple_name->du_name);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
	    break;
	}

	/* Close the iiuser table */
        status = qeu_close(qef_cb, &usrqeu);
        usr_opened = FALSE;
        if (status != E_DB_OK)
        {
	    error = E_QE022E_IIUSER;
            status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
            break;
	}

	pqeu.qeu_count = 1;	   	    /* Initialize profile-specific qeu */
        pqeu.qeu_tup_length = sizeof(DU_PROFILE);
	pqeu.qeu_output = &uqef_data;
	uqef_data.dt_next = NULL;
        uqef_data.dt_size = sizeof(DU_PROFILE);
        uqef_data.dt_data = (PTR)&ptuple;

	/*
	** If DROP PROFILE then position iiprofile table based on profile name
	** and drop the specific profile (if we can't find it issue an error).
	*/
	pqeu.qeu_getnext = QEU_REPO;
	pqeu.qeu_klen = 1;		
	pqeu.qeu_key = key_ptr_array;
	key_ptr_array[0] = &key_array[0];
	key_ptr_array[1] = &key_array[1];
	key_ptr_array[0]->attr_number = 1;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *)&ptuple_name->du_name;
	pqeu.qeu_qual = NULL;
	pqeu.qeu_qarg = NULL;
	for (;;)		/* For all profile tuples found */
	{
	    status = qeu_delete(qef_cb, &pqeu);
	    if (status != E_DB_OK)
	    {
		if (pqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
			(VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 2,
				    7, "Profile",
				    qec_trimwhite(sizeof(ptuple_name->du_name),
						(char *)&ptuple_name->du_name),
				    (PTR)&ptuple_name->du_name);
			error = E_QE0025_USER_ERROR;
			status = E_DB_WARN;
		}
		else	/* Other error */
		{
		    error = pqeu.error.err_code;
		}
		break;
	    }
	    else if (pqeu.qeu_count != 1)
	    {
		(VOID)qef_error(E_QE0282_IIPROFILE, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
		error = E_QE0025_USER_ERROR;
		status = E_DB_WARN;
		break;
	    }
		
	    /* This is a security event, must audit. */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&ptuple_name->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(ptuple_name->du_name), SXF_E_SECURITY,
			    I_SX2730_DROP_PROFILE, SXF_A_DROP | SXF_A_SUCCESS,
			    &e_error);
		error = e_error.err_code;
	    }

	    break;		    

	} /* For all profiles found */

	if (status!=E_DB_OK)
	{
		break;
	}
	local_status = qeu_close(qef_cb, &pqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(pqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
	profile_opened=FALSE;
	/*
	** Cascade, replace user tuples with this profile with the
	** default profile
	*/
	if(cascade)
	{
		DU_PROFILE default_profile;
		/* Open iiprofile. */
		pqeu.qeu_type = QEUCB_CB;
		pqeu.qeu_length = sizeof(QEUCB_CB);
		pqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
		pqeu.qeu_lk_mode = DMT_IX;
		pqeu.qeu_flag = DMT_U_DIRECT;
		pqeu.qeu_access_mode = DMT_A_WRITE;

		pqeu.qeu_tab_id.db_tab_base = DM_B_IIPROFILE_TAB_ID;    
		pqeu.qeu_tab_id.db_tab_index = DM_I_IIPROFILE_TAB_ID;
		pqeu.qeu_mask = 0;
		status = qeu_open(qef_cb, &pqeu);
		if (status != E_DB_OK)
		{
		    (VOID)qef_error(pqeu.error.err_code, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
		    error = E_QE0282_IIPROFILE;
		    status = E_DB_WARN;
		    break;
		}
		profile_opened = TRUE;
		/*
		** Get default profile tuple
		*/
		pqeu.qeu_count = 1;
		pqeu.qeu_tup_length = sizeof(DU_PROFILE);
		pqeu.qeu_output = &qef_data;
		qef_data.dt_next = NULL;
		qef_data.dt_size = sizeof(DU_PROFILE);
		qef_data.dt_data = (PTR)&default_profile;
		pqeu.qeu_getnext = QEU_REPO;
		pqeu.qeu_klen = 1;			
		pqeu.qeu_key = key_ptr_array;
		key_ptr_array[0] = &key_array[0];
		key_ptr_array[1] = &key_array[1];
		key_ptr_array[0]->attr_number = 1;
		key_ptr_array[0]->attr_operator = DMR_OP_EQ;
		key_ptr_array[0]->attr_value = DEFAULT_PROFILE_NAME;
		pqeu.qeu_qual = NULL;
		pqeu.qeu_qarg = NULL;

		local_status = qeu_get(qef_cb, &pqeu);
		status = (status > local_status ? status : local_status);
		if (local_status != E_DB_OK)
		{
		    if (pqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			(VOID)qef_error(E_QE0286_PROFILE_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    15, "default profile");
			error = E_QE0025_USER_ERROR;
		    }
		    else	/* Other error */
		    {
			error = pqeu.error.err_code;
		    }
		    break;		
		}
		/*
		** No go replace old profile by default for users
		*/
		status=qeu_profile_user(qef_cb, qeuq_cb, 
					&ptuple, 
					&default_profile);
		if(status!=E_DB_OK)
		{
		    break;
		}
	}
	break;	

    } /* End dummy for loop */

    /* Handle any error messages */

    if (status > E_DB_INFO)
        (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);

    /* Close off all the tables and transaction */
    if (profile_opened)
    {
	local_status = qeu_close(qef_cb, &pqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(pqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (usr_opened)
    {
	local_status = qeu_close(qef_cb, &usrqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(usrqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (transtarted)
    {
	if (status > E_DB_INFO)
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
} /* qeu_dprofile */

/*{
** Name: QEU_GPROFILE  - Obtain tuple from iiprofile
**
** Internal QEF call:   status = qeu_gprofile(&qef_cb, &qeuq_cb,
**					   &profile,  &protuple)
**
** Description:
**	This procedure obtains the requested tuple from the iiprofile catalog.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for profile errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to profile.
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	profile			The profile's name.
**	protuple		The iiprofile tuple buffer,
**				   if null, only status is returned.
**
** Outputs:
**	protuple     filled with requested iiprofile tuple, if found.
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
**	27-aug-93 (robf)
**          written
*/
DB_STATUS
qeu_gprofile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*profile,
DU_PROFILE		*protuple)
{
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEF_DATA	    qef_data;
    DU_PROFILE	    altuple, qualtuple;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];
    i4	    error;

    key_ptr_array[0] = &key_array[0];

    for (;;)
    {
	/* Open the iiprofile table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_IIPROFILE_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_IIPROFILE_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask = 0;
	qeu.qeu_flag = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    (VOID)qef_error(qeu.error.err_code, 0L,
		    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0282_IIPROFILE;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to query the iiprofile table */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_PROFILE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_PROFILE);
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;

	MEmove(DB_OWN_MAXNAME, (PTR) profile, (char)' ',
		   sizeof(qualtuple.du_name), (PTR)&qualtuple.du_name);
	qeu.qeu_klen = 1;
	qeu.qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_number   = DM_1_IIPROFILE_KEY;
	key_ptr_array[0]->attr_value    = (char *)&qualtuple.du_name;
	

	/* If output tuple is null, use our own storage */

	if (protuple == NULL)
	    qef_data.dt_data = (PTR)&altuple;
	else
	    qef_data.dt_data = (PTR) protuple;

	/* Get a tuple from iiprofile */

	status = qeu_get(qef_cb, &qeu);
	
	/* Check the result */

	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count == 0)
	    status = E_DB_INFO;
	else
	    status = E_DB_OK;

	/* Close the iiprofile table */

	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	    local_status = (local_status > E_DB_ERROR) ?
			    local_status : E_DB_ERROR;

	/* Set the status and exit */

	status = (status > local_status) ? status : local_status;
	
	break;
    }
    if(status!=E_DB_OK)
    {
	qeuq_cb->error.err_code=error;
    }
    return(status);
}

/*{
** Name: QEU_PROFILE_USER  - Propagate profile changes to users
**
** Description:
**	This function propagates changes in profiles to users with that
**	profile, in an efficient fashion. It is assumed the caller has
**	already started a transaction for the operation.
**
**	The interface is configured as a replace operation, either replacing
**	an "old" set of profile values by a "new" set of profile values
**	when doing an ALTER PROFILE statement, or replacing a "old" profile
**	by the "default" profile when doing a DROP PROFILE statement.
**
**    Algorithm:
**	- Open iiuser
**	- Loop over all user tuples with the old profile
**	- For each matching tuple:
**   	  - Unmerge the old profile
**	  - Merge the new profile
**	  - Update the iiuser tuple with the new value
**	- Close iiuser
**
** Internal QEF call:   status = qeu_profile_user(&qef_cb, &qeuq_cb,
**					   &old_ptuple, &new_ptuple,
**					   check_only)
**
** Description:
**	This procedure obtains the requested tuple from the iiprofile catalog.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**
**	old_ptuple		Old Profile Tuple
**
**	new_ptuple		New Profile Tuple
**
** Outputs:
**	None
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
**	13-sep-93 (robf)
**          written
**	29-nov-93 (robf)
**          Scan through iiuser, not iiprofile. 
*/
DB_STATUS
qeu_profile_user(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_PROFILE	*old_ptuple,
DU_PROFILE	*new_ptuple
)
{
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];
    bool	    tbl_opened=FALSE;
    QEU_PROF_OLD_NEW_PROF oldnewtuple;
    i4	    error=0; 
    
    key_ptr_array[0] = &key_array[0];
    for(;;)
    {
	oldnewtuple.qef_cb=qef_cb;
	oldnewtuple.qeuq_cb=qeuq_cb;
	oldnewtuple.old_ptuple=old_ptuple;
	oldnewtuple.new_ptuple=new_ptuple;

	/* Open iiuser */
	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_USER_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = E_QE022E_IIUSER;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	tbl_opened=TRUE;

	/* 
	** Scan through tuples updating profiles. This is done
	** indirectly through qeu_replace, which calls a replacement
	** function on our behalf.
	*/
	qeu.qeu_klen=0;
	qeu.qeu_flag=0;
	qeu.qeu_f_qual = update_user_profile;
	qeu.qeu_f_qarg = &oldnewtuple;
	qparams.qeu_qparms[0] = (PTR) old_ptuple;
	qeu.qeu_qual=qual_user_profile;
	qeu.qeu_qarg= &qparams;
        qeu.qeu_tup_length = sizeof(DU_USER);
	status=qeu_replace(qef_cb, &qeu);
	if(status!=E_DB_OK)
	{
		error=qeu.error.err_code;
		break;
	}

	/* Close iiuser */
	local_status = qeu_close(qef_cb, &qeu);    
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;
	break;
    }
    if(status!=E_DB_OK)
    {
        if (status > E_DB_INFO)
	{
		(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
		qeuq_cb->error.err_code=E_QE0285_PROFILE_USER_MRG_ERR;
	}

	if(tbl_opened)
	{
		local_status = qeu_close(qef_cb, &qeu);    
		status = (status > local_status ? status : local_status);
		if (local_status != E_DB_OK)
		{
		    error = qeu.error.err_code;
		}
	}
    }
    return status;
}

/*
** Name: qual_user_profile
**
** Description: Internal routine used to qualify a user tuple by profile name
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info
**	    .qeu_qparms[0]	Profile tuple to match against
**	    .qeu_rowaddr	Current user tuple to qualify
**
** Outputs:
**	qparams.qeu_retval	ADE_TRUE if qualified, else ADE_NOT_TRUE
**
** Returns E_DB_OK
**
** History:
**	13-sep-93 (robf)
**	   Written
**	11-Apr-2008 (kschendel)
**	    Revised the calling sequence.
*/
static DB_STATUS
qual_user_profile(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    DU_PROFILE *ptuple = (DU_PROFILE *) qparams->qeu_qparms[0];
    DU_USER *utuple = (DU_USER *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;

	if(!ptuple || !utuple)
		return E_DB_ERROR;

	if(!MEcmp((PTR)&ptuple->du_name,
		  (PTR)&utuple->du_profile,
		  sizeof(ptuple->du_name)))
	    qparams->qeu_retval = ADE_TRUE;

    return (E_DB_OK);
}

/*
** Name: update_user_profile
**
** Description: Internal routine used to update a user tuple when
**	        changing profiles
**
**	Please note that this is a QEU qualification routine, not a
**	DMF qualification routine.  This qualifier follows the
**	(rarely used) QEU qualification call mechanism.
**
** Inputs:
**	utuple	     - User tuple being updated
**
**	oldnewtuple - Old/New tuples
**
** Outputs:
**	utuple	     - Updated user tuple
**
** Returns:
**	    QEU_F_RETURN	Update this row
**	    QEU_F_NEXT		Skip this row
**
** History:
**	13-sep-93 (robf)
**	   Written
**	9-feb-94 (robf)
**         When updating the default group for the user, make sure  the
**	   user is a member of the group (this follows the CREATE USER 
**	   processing and automatically adds the user to the group if
**	   necessary)
*/
static i4  
update_user_profile(
	void *parm1, void *parm2
)
{
    DU_USER	*utuple = (DU_USER *) parm1;
    QEU_PROF_OLD_NEW_PROF *oldnew = (QEU_PROF_OLD_NEW_PROF *) parm2;
    QEF_CB      *qef_cb;
    QEUQ_CB	*qeuq_cb;
    DU_PROFILE 	*old_ptuple;
    DU_PROFILE 	*new_ptuple;
    DB_STATUS	status=E_DB_OK;
    DB_ERROR	error;
    DB_USERGROUP  	ugtuple;
    bool		group_specified;
    QEUQ_CB		qeuq_group;
    QEF_DATA		qef_qrygroup;
    DB_STATUS		local_status;

    if(!oldnew || !utuple)
	return QEU_F_NEXT;

    for(;;)
    {
	old_ptuple=oldnew->old_ptuple;
	new_ptuple=oldnew->new_ptuple;
	qef_cb=oldnew->qef_cb;
	qeuq_cb=oldnew->qeuq_cb;
	/*
	** Unmerge the old info
	*/
	status=qeu_unmerge_user_profile(qef_cb,qeuq_cb, utuple,old_ptuple);
	if(status!=E_DB_OK)
	{
		/* Unable to unmerge*/
		break;
	}
	/*
	** Merge in the new info
	*/
	status=qeu_merge_user_profile(qef_cb,qeuq_cb, utuple,new_ptuple);
	if(status!=E_DB_OK)
	{
		/* Unable to merge*/
		break;
	}
	/*
	** Check if group specified for default, if so make sure
	** user is a member of the group.
	*/
	group_specified = (STskipblank((char *)&utuple->du_group,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char *)NULL);

	if( group_specified)
	{
	    MEcopy((PTR)&utuple->du_name,
		   sizeof(utuple->du_name),
		   (PTR)&ugtuple.dbug_member);
	    MEcopy((PTR)&utuple->du_group,
		   sizeof(utuple->du_name),
		   (PTR)&ugtuple.dbug_group);
	    MEfill(sizeof(ugtuple.dbug_reserve),
		   (u_char)' ',
		   (PTR)ugtuple.dbug_reserve);

	    qeuq_group.qeuq_flag_mask = 0;
	    qeuq_group.qeuq_cagid = 1;
	    qef_qrygroup.dt_next = NULL;
	    qef_qrygroup.dt_size = sizeof(DB_USERGROUP);
	    qef_qrygroup.dt_data = (PTR)&ugtuple;
	    qeuq_group.qeuq_agid_tup  = &qef_qrygroup;
	    qeuq_group.qeuq_agid_mask = QEU_CGROUP | QEU_DEFAULT_GROUP;
	    qeuq_group.qeuq_eflag   = QEF_EXTERNAL;
	    qeuq_group.qeuq_type    = QEUQCB_CB;
	    qeuq_group.qeuq_length  = sizeof(QEUQ_CB);
	    qeuq_group.qeuq_tsz	    = qeuq_cb->qeuq_tsz;
	    qeuq_group.qeuq_tbl_id  = qeuq_cb->qeuq_tbl_id;
    	    qeuq_group.qeuq_d_id    = qeuq_cb->qeuq_d_id;
	    qeuq_group.qeuq_db_id   = qeuq_cb->qeuq_db_id;

	    local_status = qeu_group(qef_cb, &qeuq_group);
	    status = (status > local_status ? status : local_status);
	    if (DB_FAILURE_MACRO(status))
		break;
	    else if(status==E_DB_INFO)
	    {
		/*
		** Special flag indicating the user was added to the
		** group since default member. We allow this here so
		** reset status.
		*/
		status=E_DB_OK;
	    }
	}

	break;
    }
    if(status!=E_DB_OK)
    {
        (VOID) qef_error(E_QE0285_PROFILE_USER_MRG_ERR, 0L, status, 
		     &error.err_code, &qeuq_cb->error, 0);
	/* Error occurred */
	return QEU_F_NEXT;
    }
    else
    {
	/* Tuple updated OK */
	return QEU_F_RETURN;
    }
}

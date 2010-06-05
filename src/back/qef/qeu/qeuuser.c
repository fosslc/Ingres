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
/**
**
**  Name: QEUUSER.C - routines for maintaining users
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iiuser catalog.
**
**	The following routine(s) are defined in this file:
**
**          qeu_auser   - alter a user tuple
**	    qeu_cuser	- appends a user tuple
**	    qeu_duser	- deletes a user tuple
**	    qeu_guser	- gets a requested user tuple
**	    qeu_quser	- qualifies a user tuple by group
**	    qeu_dbowner	- qualifies an iidatabase tuple by owner
**
**  History:
**	18-may-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Created.
**	06-jun-89 (ralph)
**	    Corrected minor unix portability problems
**      25-jun-89 (jennifer)
**          Added create, drop and alter  of user for security project.
**          Added security auditing to creat and drop.
**	21-sep-89 (ralph)
**	    Fix qeu_duser() for DROP USER.
**	26-oct-89 (ralph)
**	    Remove iidbpriv and iiusergroup tuples for user if dropped.
**	    Check for name space collision with groups/roles
**	11-dec-89 (ralph)
**	    Add support for user passwords.
**      11-apr-90 (jennifer)
**          Bug 20484 - Add DMA_A_SUCCESS to access field for qeu_audit.
**      11-apr-90 (jennifer)
**          Bug 20736 - Add drop audit record along with create when 
**          altering a user.
**	08-aug-90 (ralph)
**	    Ensure old password matches existing password for non-super user
**	    on ALTER USER statement.
**	13-nov-90 (ralph)
**	    Don't drop user if owner of any database (qeu_duser).
**	    Added qeu_dbowner to qualify an iidatabase tuple by owner.
**      21-jan-91 (jennifer)
**          Fix bug B21760 to provide error if user is duplicate.
**	02=feb-91 (davebf)
**	    Add init of qeu_cb.qeu_mask.
**	27-oct-92 (robf)
**	    For auditing purposes, make both SUCCESS and FAILURE of
**	    ALTER USER log ALTER, (previously on SUCCESS you got two
**	    records, DROP then CREATE).
**	07-dev-1992 (pholman)
**	    Replace obsolete qeu_audit with new qeu_secaudit.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	25-jun-1993 (robf)
**	    Process new options (expdate, default privs etc)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	9-jul-93 (rickh)
**	    Made a ME call agree with its prototype. Removed spurious
**	    re-declarations of some external functions.
**      14-sep-93 (smc)
**          Removed (PTR) cast of qeu_d_id now session ids are CS_SID.
**          Removed redundant & of array dbug_reserve.
**	13-oct-93 (robf)
**          More prototype cleanups.
**	19-oct-93 (robf)
**          Add support for DEFAULT_PRIVS=ALL, merging privs individually
**	3-nov-93 (robf)
**          Add security label when auditing actions. Since users don't
**	    have an explicit label currently we use the label for the
**	    underlying IIUSER table.
**	10-jan-95 (carly01)
**		b66234 - ALTER USER=uuuuu WITH GROUP=ggggg failed to clear
**		GROUP when GROUP didn't exist.
**	17-feb-95 (carly01)
**		66863: accessdb - can't edit user's permissions,
**		66866: ALTER USER with privileges=(...) has no effect.
**		Fix for bug 66234 was bypassing the delete of group in
**		all cases and resulted in the bugs mentioned.
**	12-apr-96 (forky01)
**	    Bug 67979 and about 15 other unreported bugs found while
**	    checking operation for 67979.  Loads of changes to do with
**	    alter/create user and alter profile that performs unmerge/merge
**	    on existing users.  See the various functions in this file
**	    for more detail.
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
**	28-Jul-2003 (gupsh01)
**	    Refixed bug 74205, in function qeu_merge_user_profile().
**	    correctly merge the profile information,  
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	16-dec-04 (inkdo01)
**	    Inited various collIDs.
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**	12-Apr-2008 (kschendel)
**	    Revise DMF qualification interface
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Definition of static variables and forward static functions 
*/

static DB_STATUS qeu_quser(
    void *toss, QEU_QUAL_PARAMS *qparams);

static DB_STATUS qeu_dbowner(
    void *toss, QEU_QUAL_PARAMS *qparams);


/*{
** Name: qeu_auser 	- Alter user information for one user.
**
** External QEF call:   status = qef_call(QEU_AUSER, &qeuq_cb);
**
** Description:
**	Change the user tuple to the iiuser table.
**
**      This is done with a drop followed by a replace.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of user tuples.  Must be 1.
**	    .qeuq_uld_tup	user tuple.
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
**	25-jun-89 (jennifer)
**	    Written for Security.
**	07-sep-89 (ralph)
**	    Changed protocol for updating user status and group fields
**	    Call qeu_cgroup to add user to default group
**	11-dec-89 (ralph)
**	    Add support for user passwords.
**      11-apr-90 (jennifer)
**          Bug 20736 - Add drop audit record along with create when 
**          altering a user.
**	08-aug-90 (ralph)
**	    Ensure old password matches existing password for non-super user
**	    on ALTER USER statement.
**	27-oct-92 (robf)
**	    For auditing purposes, make both SUCCESS and FAILURE of
**	    ALTER USER log ALTER, (previously on SUCCESS you got two
**	    records, DROP then CREATE).
**	11-may-1993 (robf)
**	    Remove SUPERUSER privilege reference.
**	25-jun-1993 (robf)
**	    Process new options (expdate, default privs etc)
**	20-sep-93 (robf)
**          Update merge/unmerge of profiles
**	22-nov-93
**	    ALTER USER WITH NOPRIVILEGES now follows DROP PRIVILEGES in
**	    updating  the default privileges (if any) in sync. 
**	15-mar-94 (robf)
**          Add check for EXTERNAL_PASSWORD
**	17-mar-94 (robf)
**          Validate session privileges if assigning a profile which
**	    has privileged attributes.
**	19-apr-94 (robf)
**	    Improve error handling if non-existent profile is specified,
**	    breaking out right away. This causes only a single
**	    error to be printed.
**      12-apr-95 (forky01)
**          Add code to look for def_privs==privs passed when DU_UDEFPRIV
**          was not set in flagsmask.  This is for backwards compatibility
**          to allow alter user calls that didn't know about new default
**          privileges to function as before.  We turn on the DU_UDEFALL
**          bit in the flagsmask for them, as long as the DU_UDEFPRIV.
**          Also set default privs from privs if our profile said DU_UDEFALL
**          and we aren't overriding the profile with a DU_UDEFPRIV in the
**          user tuple's flagsmask.  Removed code which checked for success
**	    on find group, and if failed didn't delete user tuple before
**	    adding another with same name.  This led to duplicate user
**	    tuples in iiuser.  Also, unmerge was only done when profile
**	    names of old/new matched, changed to always unmerge.
**	    Default privs are now set correctly based on user and profile
**	    flags.
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	    Only call SCF when we need to know user_status, not
**	    "just in case".
**	26-Oct-2004 (schka24)
**	    Fix IF test for security priv broken by B1 removal.
**	17-Jul-2005 (thaju02)
**	    Clear DU_UGROUP from utuple->du_flagsmask, if group
**	    specified is same as the user's current default group
**	    and not null. (B114890)
**       9-Mar-2010 (hanal04) Bug 123379
**          ALTER USER ADD PRIVILEGES (TRACE) WITH DEFAULT_PRIVILEGES = (TRACE)
**          should not remove existing default prvilieges.
*/

DB_STATUS
qeu_auser(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_USER     	*utuple;	/* New user tuple */
    DU_USER     	utuple_temp;	/* Tuple to check for uniqueness */
    DU_USER		*utuple_pass;	/* Pointer to tuple with oldpassword */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;		
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	ukey_array[2];
    DMR_ATTR_ENTRY  	*ukey_ptr_array[2];

    DB_USERGROUP	ugtuple;
    bool		group_specified;
    bool		profile_specified;
    bool		old_profile;
    QEUQ_CB		qeuq_group;
    QEF_DATA		qef_qrygroup;

    i4			user_status;
    SCF_CB              scf_cb;
    SCF_SCI             sci_list[1];
    typedef struct	_UTUPLE_PAIR
    {
        DU_USER     ustuple;
        DU_USER     ustuple2;
    }			UTUPLE_PAIR;
    UTUPLE_PAIR		*utuple_pair;
    DU_PROFILE		ptuple;

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

	utuple_pair = (UTUPLE_PAIR *)qeuq_cb->qeuq_uld_tup->dt_data;
	utuple	    = &utuple_pair->ustuple;
	utuple_pass = &utuple_pair->ustuple2;

	group_specified = ((char *)STskipblank((char *)&utuple->du_group,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char *)NULL);
	/*
	** Check to see if default privileges equalled privileges specified
	** so we can turn on the DU_DEFALL flag because there may still be
	** old code around that does not specify DU_DEFPRIV.
	*/
	if ( utuple->du_defpriv
	    && (utuple->du_status &  DU_UALL_PRIVS) == utuple->du_defpriv
	    && !(utuple->du_flagsmask & DU_UDEFPRIV)
	   )
	{
		utuple->du_flagsmask |=  DU_UDEFALL;
	}
	if ( utuple->du_flagsmask & DU_UDEFALL )
	    utuple->du_flagsmask |= DU_UDEFPRIV;

	if(utuple->du_flagsmask&DU_UHASPROFILE)
		profile_specified=TRUE;
	else 
		profile_specified = (STskipblank((char *)&utuple->du_profile,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char*)NULL);

	status = E_DB_OK;
    
	/* Validate that the user name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_USER_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = qeu_open(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named user - if not there then OK */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_USER);
        qef_data.dt_data = (PTR)&utuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = ukey_ptr_array;
	ukey_ptr_array[0] = &ukey_array[0];
	ukey_ptr_array[1] = &ukey_array[1];
	ukey_ptr_array[0]->attr_number = DM_1_USER_KEY;
	ukey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ukey_ptr_array[0]->attr_value =
				    (char *)&utuple->du_name;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	local_status = qeu_get(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
			(VOID)qef_error(E_US18B7_6327_USER_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(utuple->du_name),
						(char *)&utuple->du_name),
				    (PTR)&utuple->du_name);
			error = E_QE0025_USER_ERROR;
	    }
	    else	/* Other error */
	    {
		error = qeu.error.err_code;
	    }
	    break;		/* Gotta get out of here now */
	}

	/*
	** If user has a profile, check to make sure the profile exists.
	*/

	if(profile_specified)
	{
            status = qeu_gprofile(qef_cb, qeuq_cb,
                           (char *)&utuple->du_profile, &ptuple);

            if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
                error = E_QE0282_IIPROFILE;
            else if (status != E_DB_OK)		    /* Profile doesn't exist */
            {
	        (VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 2,
			    7, "Profile",
			    qec_trimwhite(sizeof(utuple->du_profile),
					  (char *)&utuple->du_profile),
			    (PTR)&utuple->du_profile);
	        error = E_QE0025_USER_ERROR;
	        status = E_DB_WARN;
		break;
            }
	    else
	    {
		    /*
		    ** Check if profile adds any security or audit
		    ** attributes which are privileged. In this case we
		    ** have to ensure the current session has appropriate
		    ** privileges to assign them
		    */
		    if( ( (ptuple.du_status & DU_USECURITY)
			  && ! (qef_cb->qef_ustat & DU_USECURITY) )
		      || ( (ptuple.du_status & DU_UAUDIT_PRIVS)
			   && ! (qef_cb->qef_ustat & DU_UALTER_AUDIT) ) )
		    {
			 (VOID)qef_error(E_US18D3_6355_NOT_AUTH, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				     sizeof("ALTER USER WITH PROFILE")-1,
				     "ALTER USER WITH PROFILE");
			error = E_QE0025_USER_ERROR;
			status = E_DB_ERROR;
			break;
		    }
	    }
	}
	if (utuple->du_flagsmask & DU_UEXPDATE)
	{
	    if(MEcmp((PTR)&utuple->du_expdate, 
		      (PTR)&utuple_temp.du_expdate,
	   		 sizeof(DB_DATE))==0)
				utuple->du_flagsmask &= ~DU_UEXPDATE;	
	}

     	if (utuple->du_flagsmask & DU_UGROUP)
	{
	    if (group_specified && (MEcmp((PTR)&utuple->du_group,
			(PTR)&utuple_temp.du_group, 
			sizeof(utuple->du_group))==0))
	    {
		utuple->du_flagsmask&=~DU_UGROUP;
		group_specified = FALSE;
	    }
	}
	
	/*
	** Check if previously had profile
	*/
	if(utuple_temp.du_flagsmask&DU_UHASPROFILE)
		old_profile=TRUE;
	else
		old_profile= ((char *)STskipblank((char *)&utuple_temp.du_profile,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char *)NULL);
	
	if(old_profile && profile_specified)
	{
		DU_PROFILE oldptuple;
		/*
		** Get the old profile 
		*/
		status = qeu_gprofile(qef_cb, qeuq_cb,
			   (char *)&utuple_temp.du_profile, &oldptuple);

		if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
		{
			error = E_QE0282_IIPROFILE;
			break;
		}
		else if (status != E_DB_OK)		    /* Profile doesn't exist */
		{
			(VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 2,
			    7, "Profile",
			    qec_trimwhite(sizeof(DB_OWN_NAME),
					   (char *)&utuple_temp.du_profile),
			    (char *)&utuple_temp.du_profile);
			error = E_QE0025_USER_ERROR;
			status = E_DB_WARN;
			break;
		}

		status=qeu_unmerge_user_profile(qef_cb,qeuq_cb, 
				&utuple_temp,
				&oldptuple);
		if(status!=E_DB_OK)
			break;
	}
	/*
	** Check to see if default privileges equalled privileges specified
	** so we can turn on the DU_DEFALL flag because there may still be
	** old code around that does not specify DU_DEFPRIV.
	*/
	if ( utuple_temp.du_defpriv
	    &&(utuple_temp.du_status & DU_UALL_PRIVS) == utuple_temp.du_defpriv
	    && !(utuple_temp.du_flagsmask & DU_UDEFPRIV)
	    && !(utuple->du_flagsmask & DU_UDEFPRIV)
	   )
	{
		utuple_temp.du_flagsmask |= DU_UDEFALL;
	}
	if (utuple_temp.du_flagsmask & DU_UDEFALL)
	    utuple_temp.du_flagsmask |= DU_UDEFPRIV;

	/* Found the user, merge the information. */

     	if (utuple->du_flagsmask & DU_UPRIV)
	{
	    /*
	    ** Preserve security audit bits in status
	    */
	    utuple_temp.du_status = 
			(utuple->du_status |
			(utuple_temp.du_status&(DU_UAUDIT|DU_UAUDIT_QRYTEXT))); 
	    utuple_temp.du_userpriv = utuple_temp.du_status;
	    /*
	    ** If NOPRIVILEGES specified remove from default
	    */
	    if((utuple->du_status&DU_UALL_PRIVS)==0)
	    {
	       /* No privileges, so make default empty (same as
	       ** dropping all the privs).
	       */
	       utuple_temp.du_defpriv=0;
	       if ( !(utuple->du_flagsmask & DU_UDEFPRIV) )
	       {
		   utuple_temp.du_flagsmask &= ~DU_UDEFPRIV;
		   utuple_temp.du_flagsmask &= ~DU_UDEFALL;
	       }
	    }
	}
        else if (utuple->du_flagsmask & DU_UAPRIV)
	{
	    utuple_temp.du_status |= utuple->du_status; 
	    utuple_temp.du_userpriv |= utuple->du_status;
	}
        else if (utuple->du_flagsmask & DU_UDPRIV)
	{
	    utuple_temp.du_status &= ~utuple->du_status; 
            utuple_temp.du_userpriv &= ~utuple->du_status;

	    /* Remove from default privs also */
	    utuple_temp.du_defpriv &= ~utuple->du_status; 
	}
	/* Check security audit status */
	if (utuple->du_flagsmask & DU_UQRYTEXT)
        {
	    utuple_temp.du_status |= DU_UAUDIT_QRYTEXT;
            utuple_temp.du_userpriv |= DU_UAUDIT_QRYTEXT;
	}
	
	if (utuple->du_flagsmask & DU_UNOQRYTEXT)
	{
	    utuple_temp.du_status &= ~DU_UAUDIT_QRYTEXT;
	    utuple_temp.du_userpriv &= ~DU_UAUDIT_QRYTEXT;
	}
	
	if (utuple->du_flagsmask & DU_UALLEVENTS)
	{
	    utuple_temp.du_status |= DU_UAUDIT;
	    utuple_temp.du_userpriv |= DU_UAUDIT;
	}
	
	if (utuple->du_flagsmask & DU_UDEFEVENTS)
	{
	    utuple_temp.du_status &= ~DU_UAUDIT;
	    utuple_temp.du_userpriv &= ~DU_UAUDIT;
	}
	
	/* Default privileges */
	if (utuple->du_flagsmask & DU_UDEFPRIV)
	{
	    /* User specified default privileges */
            if (utuple->du_flagsmask & DU_UAPRIV)
                utuple_temp.du_defpriv |= utuple->du_defpriv;
            else
	        utuple_temp.du_defpriv = utuple->du_defpriv; 
	    utuple_temp.du_flagsmask |= DU_UDEFPRIV;
	    if (utuple->du_flagsmask & DU_UDEFALL)
	        utuple_temp.du_flagsmask |= DU_UDEFALL;
	    else
	        utuple_temp.du_flagsmask &= ~DU_UDEFALL;
	}
	else if(utuple_temp.du_flagsmask & DU_UDEFALL)
	{
	    /*
	    ** No specified default privs, and need to keep 
	    ** default same as full set.
	    ** Remember to mask out any non-privilege status bits
	    */
	    utuple_temp.du_defpriv = (utuple_temp.du_status& DU_UALL_PRIVS);
	}
	/*
	** Save real user status bits prior to remerge
	*/
	if ( profile_specified )
	    utuple_temp.du_userpriv = utuple_temp.du_status;


	if (utuple->du_flagsmask & DU_UEXPDATE)
	{
	    MECOPY_CONST_MACRO((PTR)&utuple->du_expdate, sizeof(DB_DATE), 
				(PTR)&utuple_temp.du_expdate);
	    utuple_temp.du_flagsmask|=DU_UEXPDATE;
	}

     	if (utuple->du_flagsmask & DU_UGROUP)
	{
            STRUCT_ASSIGN_MACRO(utuple->du_group, utuple_temp.du_group);
	    utuple_temp.du_flagsmask|=DU_UGROUP;
	}

	/* Check password for user */
     	if (utuple->du_flagsmask & DU_UPASS) 
	{
	    /*
	    ** Obtain information about the current user, just in case.
	    */
	    scf_cb.scf_length       = sizeof (SCF_CB);
	    scf_cb.scf_type		= SCF_CB_TYPE;
	    scf_cb.scf_facility     = DB_QEF_ID;
	    scf_cb.scf_session      = qef_cb->qef_ses_id;
	    scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
	    scf_cb.scf_len_union.scf_ilength = 1;
	    sci_list[0].sci_code    = SCI_USTAT;
	    sci_list[0].sci_length  = sizeof(user_status);
	    sci_list[0].sci_aresult = (char *) &user_status;
	    sci_list[0].sci_rlength = NULL;

	    status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
		error = E_QE022F_SCU_INFO_ERROR;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		break;
	    }

	    /* Ensure oldpassword matches for regular users */
	    if (!(user_status & DU_UMAINTAIN_USER) &&
	   	!(utuple->du_flagsmask & DU_UEXTPASS) &&
		(MEcmp( ( PTR ) &utuple_pass->du_pass,
			( PTR ) &utuple_temp.du_pass,
		        ( u_i2 ) sizeof(utuple_temp.du_pass)) != 0)
		)
	    {
		if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
		    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&utuple->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(utuple->du_name), SXF_E_SECURITY,
			    I_SX2023_USER_ALTER, SXF_A_FAIL | SXF_A_ALTER,
			    &e_error);

	        error = E_US18D6_6358_ALTER_BAD_PASS;

		status = (status > E_DB_ERROR ? status : E_DB_ERROR);
		break;
	    }

            STRUCT_ASSIGN_MACRO(utuple->du_pass, utuple_temp.du_pass);
	    /*
	    ** Pass external flag down
	    */
	    if(utuple->du_flagsmask & DU_UEXTPASS) 
		utuple_temp.du_flagsmask|=DU_UEXTPASS;
	    else
		utuple_temp.du_flagsmask&= ~DU_UEXTPASS;
	}

	/* If profile specified merge that in to the tuple now */
	if (profile_specified)
	{
		status=qeu_merge_user_profile(qef_cb,qeuq_cb, 
				&utuple_temp,
				&ptuple);
		if(status!=E_DB_OK)
			break;
		utuple_temp.du_flagsmask|=DU_UHASPROFILE;
	}
	/*
	** If DEFAULT=ALL specified make that true, includes both 
	** specified and profile privs
	*/
	if ( ( utuple_temp.du_flagsmask & DU_UDEFALL)
	     || ( profile_specified
		  && (ptuple.du_flagsmask & DU_UDEFALL) 
		  && !(utuple_temp.du_flagsmask & DU_UDEFPRIV)
	        )
	   )
	{
	    utuple_temp.du_defpriv = (utuple_temp.du_status& DU_UALL_PRIVS);
	}

	/* Make sure default privs are a subset of base privs */
	if(utuple_temp.du_defpriv)
	{
		if((utuple_temp.du_defpriv & utuple_temp.du_status ) !=
			utuple_temp.du_defpriv)
		{
			(VOID)qef_error(E_US1967_6503_DEF_PRIVS_NSUB,0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 0);
			error = E_QE0025_USER_ERROR;
			status= E_DB_ERROR;
			break;
		}
	}
	/*
	** Add the user to the group, so we'll know if 
	** we should leave the default group as is.
	*/

	if (group_specified)
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
	    if (local_status == E_DB_WARN)	/* Group doesn't exist */
	    {
		 /* Ignore the group */
	
		/*b66234 from qeu_cuser(): */ 
		MEfill (sizeof ( utuple_temp.du_group ),	/*No group*/
			(u_char) ' ',
			(PTR) &utuple_temp.du_group ) ;
	        utuple_temp.du_flagsmask &= ~DU_UGROUP;

		group_specified = FALSE;
	    }
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

	/* Insert single user tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_input = &qef_data;
	local_status = qeu_append(qef_cb, &qeu);

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

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&utuple->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(utuple->du_name), SXF_E_SECURITY,
			    I_SX2023_USER_ALTER, SXF_A_SUCCESS | SXF_A_ALTER,
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
} /* qeu_auser */

/*{
** Name: qeu_cuser 	- Store user information for one user.
**
** External QEF call:   status = qef_call(QEU_CUSER, &qeuq_cb);
**
** Description:
**	Add the user tuple to the iiuser table.
**
**	The user tuple was filled from the CREATE user statement, except
**	that since this is the first access to iiuser, the uniqueness
**	of the user name (within the current user scope) is validated here.
**	First the iiuser table is retrieved by user name to insure
**      that the user name provided is unqiue.
**	If a match is found then the user is a duplicate and an error is
**	returned.  If this is unique then the user is entered into iiuser.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of user tuples.  Must be 1.
**	    .qeuq_uld_tup	user tuple.
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
**	25-jun-89 (jennifer)
**	    Written for Security.
**	08-oct-89 (ralph)
**	    Call qeu_cgroup to add user to default group
**	26-oct-89 (ralph)
**	    Check for name space collision with groups/roles
**	30-jun-93 (robf)
**	    Check to make sure default privs is subset of full privs
**	3-feb-94 (robf)
**          Reset status if default profile not found to allow 
**	    processing to continue.
**	17-mar-94 (robf)
**          Validate session privileges if assigning a profile which
**	    has privileged attributes.
**	6-may-94 (robf)
**          Update check for role name clash to not lose error status.
**	12-apr-95 (forky01)
**	    Add code to look for def_privs==privs passed when DU_UDEFPRIV
**	    was not set in flagsmask.  This is for backwards compatibility
**	    to allow create user calls that didn't know about new default
**	    privileges to function as before.  We turn on the DU_UDEFALL
**	    bit in the flagsmask for them, as long as the DU_UDEFPRIV.
**	    Also set default privs from privs if our profile said DU_UDEFALL
**	    and we aren't overriding the profile with a DU_UDEFPRIV in the
**	    user tuple's flagsmask. 
**	14-jul-2004 (stephenb)
**	    fix up bad "if" logic after recent code removal.
**	26-Oct-2004 (schak24)
**	    Fix same again.  Deleted "DU_UMAC_PRIVS" included security...
*/

DB_STATUS
qeu_cuser(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_USER     	*utuple;	/* New user tuple */
    DU_USER     	utuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;		
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	ukey_array[2];
    DMR_ATTR_ENTRY  	*ukey_ptr_array[2];

    DB_USERGROUP	ugtuple;
    bool		group_specified;
    bool		profile_specified;
    QEUQ_CB		qeuq_group;
    QEF_DATA		qef_qrygroup;
    DU_PROFILE		ptuple;

    utuple = (DU_USER *)qeuq_cb->qeuq_uld_tup->dt_data;

    /* Ensure no name collision */

    for (;;)
    {
        /* Check the iiusergroup catalog to ensure no name collision */

        status = qeu_ggroup(qef_cb, qeuq_cb,
                           (char *)&utuple->du_name, NULL, NULL);

        if (DB_FAILURE_MACRO(status))               /* Fatal error! */
            error = E_QE022A_IIUSERGROUP;
        else if (status == E_DB_OK)                 /* Group exists! */
        {
	    (VOID)qef_error(E_QE0235_USER_GROUP, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(utuple->du_name),
					  (char *)&utuple->du_name),
			    (PTR)&utuple->du_name);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
        }
        else                                        /* Group does not exist */
            status = E_DB_OK;

        /* Terminate if error occurred */

        if (status != E_DB_OK)
            break;

        /* Check the iirole catalog to ensure no name collision */

        status = qeu_gaplid(qef_cb, qeuq_cb,
                           (char *)&utuple->du_name, NULL);

        if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
            error = E_QE022B_IIAPPLICATION_ID;
        else if (status == E_DB_OK)		    /* Role exists! */
        {
	    (VOID)qef_error(E_QE0234_USER_ROLE, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(utuple->du_name),
					  (char *)&utuple->du_name),
			    (PTR)&utuple->du_name);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
        }
        else					    /* Role does not exist */
            status = E_DB_OK;

        /* Terminate if error occurred */

        if (status != E_DB_OK)
            break;

        /* Check the iiprofile catalog to ensure no name collision */

        status = qeu_gprofile(qef_cb, qeuq_cb,
                           (char *)&utuple->du_name, NULL);

        if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
            error = E_QE0282_IIPROFILE;
        else if (status == E_DB_OK)		    /* Profile exists! */
        {
	    (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    4, "user",
			    qec_trimwhite(sizeof(utuple->du_name),
					  (char *)&utuple->du_name),
			    (PTR)&utuple->du_name,
			    7, "profile" );
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
        }
        else					    /* Profile does not exist */
            status = E_DB_OK;

        /* Terminate if error occurred */

        if (status != E_DB_OK)
            break;

	/*
	** If user has a profile, check to make sure the profile exists.
	*/
	if(utuple->du_flagsmask&DU_UHASPROFILE)
		profile_specified=TRUE;
	else 
		profile_specified = (STskipblank((char *)&utuple->du_profile,
					       (i4)sizeof(DB_OWN_NAME))
					    != (char*)NULL);

	if(profile_specified)
	{
            status = qeu_gprofile(qef_cb, qeuq_cb,
                           (char *)&utuple->du_profile, &ptuple);

            if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
                error = E_QE0282_IIPROFILE;
            else if (status != E_DB_OK)		    /* Profile doesn't exist */
            {
		if((STskipblank((char *)&utuple->du_profile,
					       (i4)sizeof(DB_OWN_NAME))
					    == (char*)NULL))
		{
		    /*
		    ** Special case, default profile is missing, allow 
		    ** processing to continue in this case.
		    */
		    profile_specified=FALSE;
		    status=E_DB_OK;
		}
		else
		{
		    /*
		    ** Special case to allow processing to continue if
		    ** no default profile 
		    */
	            (VOID)qef_error(E_QE0284_OBJECT_NOT_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 2,
			    7, "Profile",
			    qec_trimwhite(sizeof(utuple->du_profile),
					  (char *)&utuple->du_profile),
			    (PTR)&utuple->du_profile);
	             error = E_QE0025_USER_ERROR;
	             status = E_DB_WARN;
		}
            }
	    else
	    {
		    /*
		    ** Check if profile adds any security or audit
		    ** attributes which are privileged. In this case we
		    ** have to ensure the current session has appropriate
		    ** privileges to assign them
		    */
		    if ( ( (ptuple.du_status & DU_USECURITY)
			   && ! (qef_cb->qef_ustat & DU_USECURITY) )
		      || ( (ptuple.du_status&DU_UAUDIT_PRIVS)
			   && ! (qef_cb->qef_ustat &DU_UALTER_AUDIT) ) )
		    {
			 (VOID)qef_error(E_US18D3_6355_NOT_AUTH, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				     sizeof("CREATE USER WITH PROFILE")-1,
				     "CREATE USER WITH PROFILE");
			error = E_QE0025_USER_ERROR;
			status = E_DB_ERROR;
			break;
		    }
	    }
	}
        break;
    }

    /* If no errors, attempt to add the user */

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

	/*
	** Check to see if default privileges equalled privileges specified
	** so we can turn on the DU_DEFALL flag because there may still be
	** old code around that does not specify DU_DEFPRIV.
	*/
	if ( utuple->du_defpriv
	    && (utuple->du_status &  DU_UALL_PRIVS) == utuple->du_defpriv
	    && !(utuple->du_flagsmask & DU_UDEFPRIV)
	   )
	{
		utuple->du_flagsmask |= DU_UDEFALL;
	}
	if (utuple->du_flagsmask & DU_UDEFALL)
	    utuple->du_flagsmask |= DU_UDEFPRIV;
	/*
	** Save user status bits prior to merge
	*/
	utuple->du_userpriv = utuple->du_status;

	/* If profile specified merge that in to the tuple now */
	if (profile_specified)
	{
		status=qeu_merge_user_profile(qef_cb,qeuq_cb, utuple,
				&ptuple);
		if(status!=E_DB_OK)
			break;
	}
	/*
	** If DEFAULT=ALL specified make that true, includes both 
	** specified and profile privs
	*/
	if ( ( utuple->du_flagsmask & DU_UDEFALL)
	     || ( profile_specified
		  && (ptuple.du_flagsmask & DU_UDEFALL) 
		  && !(utuple->du_flagsmask & DU_UDEFPRIV)
	        )
	   )
	{
		utuple->du_defpriv = (utuple->du_status& DU_UALL_PRIVS);
	}
	/* Make sure default privs are a subset of base privs */
	if(utuple->du_defpriv)
	{
		if((utuple->du_defpriv & utuple->du_status ) !=
			utuple->du_defpriv)
		{
			(VOID)qef_error(E_US1967_6503_DEF_PRIVS_NSUB,0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 0);
			error = E_QE0025_USER_ERROR;
			status= E_DB_ERROR;
			break;
		}
	}
	group_specified = (STskipblank((char *)&utuple->du_group,
					       (i4)sizeof(DB_OWN_NAME))
			    != (char *)NULL);

	status = E_DB_OK;
    
	/* Validate that the user name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_USER_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	local_status = qeu_open(qef_cb, &qeu);
	status = (status > local_status ? status : local_status);
	if (local_status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named user - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_USER);
        qef_data.dt_data = (PTR)&utuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = ukey_ptr_array;
	ukey_ptr_array[0] = &ukey_array[0];
	ukey_ptr_array[1] = &ukey_array[1];
	ukey_ptr_array[0]->attr_number = DM_1_USER_KEY;
	ukey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ukey_ptr_array[0]->attr_value = (char *)&utuple->du_name;
					
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	local_status = qeu_get(qef_cb, &qeu);
	if (local_status == E_DB_OK)		/* Found the same user! */
	{
	    (VOID)qef_error(E_US18B6_6326_USER_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(utuple->du_name),
					  (char *)&utuple->du_name),
			    (PTR)&utuple->du_name);
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
	** Add the user to the group first, so we'll know if 
	** we should leave the default group as is.
	*/

	if (group_specified)
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
	    if (local_status == E_DB_WARN)	/* Group doesn't exist */
	    {
		MEfill(sizeof(utuple->du_group),	/* No group */
		       (u_char)' ',
		       (PTR)&utuple->du_group);
		group_specified = FALSE;
	    }
	}

	/* Insert single user tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
	local_status = qeu_append(qef_cb, &qeu);
        if ((qeu.qeu_count == 0) && (local_status == E_DB_OK))
        {
	    (VOID)qef_error(E_US18B6_6326_USER_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(utuple->du_name),
					  (char *)&utuple->du_name),
			    (PTR)&utuple->du_name);
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
			    (char *)&utuple->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(utuple->du_name), SXF_E_SECURITY,
			    I_SX200C_USER_CREATE, SXF_A_SUCCESS | SXF_A_CREATE,
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
} /* qeu_cuser */

/*{
** Name: qeu_duser 	- Drop single.
**
** External QEF call:   status = qef_call(QEU_DUSER, &qeuq_cb);
**
** Description:
**	Drop the user tuple.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Must be 1 if DROP user, 0 if DROP TABLE.
**	    .qeuq_uld_tup	user tuple to be deleted - if DROP user, 
**				otherwise this is ignored:
**		.du_name	user name.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US189D_6301_user_ABSENT
**				E_US18A8_6312_user_DRPTAB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	25-jun-89 (jennifer)
**	    Written for Security/users.
**	21-sep-89 (ralph)
**	    Fix qeu_duser() for DROP USER.
**	26-oct-89 (ralph)
**	    Remove iidbpriv and iiusergroup tuples for user if dropped.
**	13-nov-90 (ralph)
**	    Don't drop user if owner of any database
**	17-jun-92 (andre)
**	    init qeu_flag before calling qeu_get() since it looks for QEU_BY_TID
**	7-mar-94 (robf)
**          Purge role grants before purging the user.
*/

DB_STATUS
qeu_duser(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;	    
    QEU_CB	    	uqeu;		/* For iiuser table */
    QEF_DATA	    	uqef_data;
    DU_USER	    	*utuple_name;	/* Input tuple with name. */
    DU_USER	    	utuple;		/* Tuple currently being deleted */
    bool	    	user_opened = FALSE;
    DMR_ATTR_ENTRY  	ukey_array[2];
    DMR_ATTR_ENTRY  	*ukey_ptr_array[2];
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    i2			gtype;		/* For purging iidbpriv tuples */
    char		*tempstr;
    i4		alen;

    QEU_CB		dbqeu;		/* For iidatabase table */
    bool	    	db_opened = FALSE;
    QEU_QUAL_PARAMS	qparams;
    DU_DATABASE		dbqual_tuple;
    DU_DATABASE		db_tuple;
    QEF_DATA		dbqef_data;

    utuple_name = (DU_USER *)qeuq_cb->qeuq_uld_tup->dt_data;

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

	/* Ensure user does not own any databases */

	/* Open iidatabase */
	dbqeu.qeu_type = QEUCB_CB;
        dbqeu.qeu_length = sizeof(QEUCB_CB);
        dbqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        dbqeu.qeu_lk_mode = DMT_IS;
	dbqeu.qeu_access_mode = DMT_A_READ;
	dbqeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
	dbqeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
	dbqeu.qeu_mask = 0;
	dbqeu.qeu_flag = 0;
	status = qeu_open(qef_cb, &dbqeu);
	if (status != E_DB_OK)
	{
	    error = E_QE022D_IIDATABASE;
	    break;
	}
	db_opened = TRUE;

	/* Scan iidatabase for databases owned by the user */
        dbqeu.qeu_count = 1;
        dbqeu.qeu_tup_length = sizeof(DU_DATABASE);
        dbqeu.qeu_output = &dbqef_data;
	dbqef_data.dt_next = NULL;
	dbqef_data.dt_size = sizeof(DU_DATABASE);
	dbqef_data.dt_data = (PTR)&db_tuple;
        dbqeu.qeu_getnext = QEU_REPO;
        dbqeu.qeu_klen = 0;
        dbqeu.qeu_key = NULL;
	qparams.qeu_qparms[0] = (PTR) &dbqual_tuple;
        dbqeu.qeu_qual = qeu_dbowner;
        dbqeu.qeu_qarg = &qparams;
	MEcopy((PTR)&utuple_name->du_name,
	       sizeof(utuple_name->du_name),
	       (PTR)&dbqual_tuple.du_own);

        status = qeu_get(qef_cb, &dbqeu);
        if ((status != E_DB_OK) &&
            (dbqeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	   )
        {
	    error = E_QE022D_IIDATABASE;
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
            break;
	}

        if (status == E_DB_OK)
	{
	    (VOID)qef_error(E_QE0237_DROP_DBA, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(utuple_name->du_name),
			    (char *)&utuple_name->du_name),
			    (PTR)&utuple_name->du_name);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
	    break;
	}

	/* Close the iidbdb database */
        status = qeu_close(qef_cb, &dbqeu);
        db_opened = FALSE;
        if (status != E_DB_OK)
        {
            error = E_QE022D_IIDATABASE;
            status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
            break;
	}

	/* Open iiuser. */
	uqeu.qeu_type = QEUCB_CB;
        uqeu.qeu_length = sizeof(QEUCB_CB);
        uqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        uqeu.qeu_lk_mode = DMT_IX;
        uqeu.qeu_flag = DMT_U_DIRECT;
        uqeu.qeu_access_mode = DMT_A_WRITE;

        uqeu.qeu_tab_id.db_tab_base = DM_B_USER_TAB_ID;    /* Open iiuser */
        uqeu.qeu_tab_id.db_tab_index = DM_I_USER_TAB_ID;
	uqeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &uqeu);
	if (status != E_DB_OK)
	{
	    error = uqeu.error.err_code;
	    break;
	}
	user_opened = TRUE;

	uqeu.qeu_count = 1;	   	    /* Initialize user-specific qeu */
        uqeu.qeu_tup_length = sizeof(DU_USER);
	uqeu.qeu_output = &uqef_data;
	uqef_data.dt_next = NULL;
        uqef_data.dt_size = sizeof(DU_USER);
        uqef_data.dt_data = (PTR)&utuple;

	/*
	** If DROP user then position iiuser table based on user name
	** and drop the specific user (if we can't find it issue an error).
	*/
	uqeu.qeu_getnext = QEU_REPO;
	uqeu.qeu_klen = 1;		
	uqeu.qeu_key = ukey_ptr_array;
	ukey_ptr_array[0] = &ukey_array[0];
	ukey_ptr_array[1] = &ukey_array[1];
	ukey_ptr_array[0]->attr_number = DM_1_USER_KEY;
	ukey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ukey_ptr_array[0]->attr_value =
				(char *)&utuple_name->du_name;
	uqeu.qeu_qual = NULL;
	uqeu.qeu_qarg = NULL;
	for (;;)		/* For all user tuples found */
	{
	    status = qeu_delete(qef_cb, &uqeu);
	    if (status != E_DB_OK)
	    {
		if (uqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
			(VOID)qef_error(E_US18B7_6327_USER_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(utuple_name->du_name),
						(char *)&utuple_name->du_name),
				    (PTR)&utuple_name->du_name);
			error = E_QE0025_USER_ERROR;
			status = E_DB_WARN;
		}
		else	/* Other error */
		{
		    error = uqeu.error.err_code;
		}
		break;
	    }
	    else if (uqeu.qeu_count != 1)
	    {
		(VOID)qef_error(E_US18B7_6327_USER_ABSENT, 0L,
				E_DB_ERROR, &error, &qeuq_cb->error, 1,
				qec_trimwhite(sizeof(utuple_name->du_name),
				(char *)&utuple_name->du_name),
				(PTR)&utuple_name->du_name);
		error = E_QE0025_USER_ERROR;
		status = E_DB_WARN;
		break;
	    }
		
	    /* This is a security event, must audit. */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&utuple_name->du_name,
			    (DB_OWN_NAME *)NULL,
			    sizeof(utuple_name->du_name), SXF_E_SECURITY,
			    I_SX200D_USER_DROP, SXF_A_DROP | SXF_A_SUCCESS,
			    &e_error);
		error = e_error.err_code;
	    
		if (status != E_DB_OK)
		    break;
	    }

	    /* Purge all dbprivs associated with the user */

	    gtype = DBGR_USER;
	    local_status = qeu_pdbpriv(qef_cb, qeuq_cb,
				 (char *)&utuple_name->du_name,
				 (char *)&gtype);
	    status = (status > local_status) ? status : local_status;
	    if (local_status > E_DB_INFO)
	    {
		error = E_QE022C_IIDBPRIV;
		break;
	    }
	    else if (local_status != E_DB_OK)
		(VOID)qef_error(E_QE0228_DBPRIVS_REMOVED, 0L,
				E_DB_ERROR, &error, &qeuq_cb->error, 1,
				qec_trimwhite(sizeof(utuple_name->du_name),
				(char *)&utuple_name->du_name),
				(PTR)&utuple_name->du_name);

	    /* Purge all iiusergroup tuples associated with the user */

	    local_status = qeu_pgroup(qef_cb, qeuq_cb,
				 (char *)&utuple_name->du_name);
	    status = (status > local_status) ? status : local_status;
	    if (local_status > E_DB_INFO)
	    {
		error = E_QE022A_IIUSERGROUP;
		break;
	    }
	    else if (local_status != E_DB_OK)
		(VOID)qef_error(E_QE0232_USRMEM_DROPPED, 0L,
				E_DB_ERROR, &error, &qeuq_cb->error, 1,
				qec_trimwhite(sizeof(utuple_name->du_name),
				(char *)&utuple_name->du_name),
				(PTR)&utuple_name->du_name);

	    /* Purge any related role grants */
	    local_status =qeu_prolegrant(qef_cb, qeuq_cb,
		(char *)&utuple_name->du_name,
		(char*)NULL);

	    status = (status > local_status) ? status : local_status;

	    if (status!=E_DB_OK)
	    {
		error  = E_US247C_9340_IIROLEGRANT_ERROR;
		break;
	    }

	    break;
	} /* For all users found */

	break;			    /* We gotta get outta here somehow! */

    } /* End dummy for loop */

    /* Handle any error messages */

    if (status > E_DB_INFO)
        (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);

    /* Close off all the tables and transaction */
    if (user_opened)
    {
	local_status = qeu_close(qef_cb, &uqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(uqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (db_opened)
    {
	local_status = qeu_close(qef_cb, &dbqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(dbqeu.error.err_code, 0L, local_status, 
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
} /* qeu_duser */

/*{
** Name: QEU_GUSER  - Obtain tuple from iiuser
**
** Internal QEF call:   status = qeu_guser(&qef_cb, &qeuq_cb,
**					   &user, &group, &ustuple)
**
** Description:
**	This procedure obtains the requested tuple from the iiuser catalog.
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
**	user			the user's name
**				if null or blank, a scan is performed.
**	group			the default group
**				if not null, iiuser tuples are qualified
**				on the default_group field for this value.
**	ustuple			the iiuser tuple buffer
**				if null, only status is returned.
**
** Outputs:
**	ustuple			filled with requested iiuser tuple, if found.
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
**	    init qeu_flag before calling qeu_get() since it looks for QEU_BY_TID
*/
DB_STATUS
qeu_guser(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*user,
char		*group,
DU_USER		*ustuple)
{
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    QEF_DATA	    qef_data;
    DU_USER	    altuple, qualtuple;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];

    key_ptr_array[0] = &key_array[0];

    for (;;)
    {
	/* Open the iiuser table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_USER_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask = 0;
	qeu.qeu_flag = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to query the iiuser table */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DU_USER);
	qeu.qeu_getnext = QEU_REPO;

	/*
	** If user is null or blank, scan entire iiuser table.
	** Otherwise, look up specific user tuple.
	*/

	if ((user == NULL) ||
	    (STskipblank(user, (i4)DB_OWN_MAXNAME) == NULL)
	   )
	{
	    qeu.qeu_klen = 0;
	    qeu.qeu_key = NULL;
	}
	else
	{
	    MEmove(DB_OWN_MAXNAME, (PTR) user, (char)' ',
		   sizeof(qualtuple.du_name), (PTR)&qualtuple.du_name);
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = key_ptr_array;
	    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    key_ptr_array[0]->attr_number   = DM_1_USER_KEY;
	    key_ptr_array[0]->attr_value    = (char *)&qualtuple.du_name;
	}

	/* If group is not null, set up qualification */

	if (group == NULL)
	{
	    qeu.qeu_qual = NULL;
	    qeu.qeu_qarg = NULL;
	}
	else
	{
	    MEmove(DB_OWN_MAXNAME, (PTR) group, (char)' ',
		   sizeof(qualtuple.du_group), (PTR)&qualtuple.du_group);
	    qparams.qeu_qparms[0] = (PTR) &qualtuple;
	    qeu.qeu_qual = qeu_quser;
	    qeu.qeu_qarg = &qparams;
	}

	/* If output tuple is null, use our own storage */

	if (ustuple == NULL)
	    qef_data.dt_data = (PTR)&altuple;
	else
	    qef_data.dt_data = (PTR) ustuple;

	/* Get a tuple from iiuser */

	status = qeu_get(qef_cb, &qeu);
	
	/* Check the result */

	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count == 0)
	    status = E_DB_INFO;
	else
	    status = E_DB_OK;

	/* Close the iiuser table */

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
** Name: QEU_QUSER  - Qualify an iiuser tuple by group
**
** Description:
**	This procedure qualifies an iiuser tuple by group name.
**	It is called by DMF as part of tuple qualification when
**	reading tuples for qeu_guser.
**
** Inputs:
**	qparams
**	    .qeu_qparms[0]	Template iiuser tuple to qualify against
**		.du_group	Default group
**	    .qeu_rowaddr	iiuser tuple being considered
**
** Outputs:
**	qparams.qeu_retval	ADE_TRUE if match, else ADE_NOT_TRUE
**
**	Returns: E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	18-may-89 (ralph)
**          written
**	11-Apr-2008 (kschendel)
**	    Update to new style calling sequence.
*/
static DB_STATUS
qeu_quser(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    DU_USER *qual_tuple = (DU_USER *) qparams->qeu_qparms[0];
    DU_USER *user_tuple = (DU_USER *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;

    if (MEcmp((PTR)&qual_tuple->du_group, 
	      (PTR)&user_tuple->du_group,
	      sizeof(qual_tuple->du_group)) == 0)
	qparams->qeu_retval = ADE_TRUE;		/* Qualified */
    return (E_DB_OK);
}

/*{
** Name: QEU_DBOWNER - Qualify an iidatabase tuple  by owner
**
** Description:
**	This procedure qualifies an iidatabase tuple by owner name.
**	It is called by DMF as part of tuple qualification when
**	reading tuples for qeu_guser.
**
** Inputs:
**	qparams
**	    .qeu_qparms[0]	iidatabase tuple to qualify against
**	    .qeu_rowaddr	iidatabase tuple being considered
**
** Outputs:
**	qparams.qeu_retval	ADE_TRUE if row matches, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK
**
** Side Effects:
**	    None.
**
** History:
**	13-nov-90 (ralph)
**          written
**	11-Apr-2008 (kschendel)
**	    Update to new style calling sequence.
*/
static DB_STATUS
qeu_dbowner(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    DU_DATABASE	    *qual_tuple;
    DU_DATABASE	    *db_tuple;

    qual_tuple  = (DU_DATABASE *)qparams->qeu_qparms[0];
    db_tuple = (DU_DATABASE *) qparams->qeu_rowaddr;
    qparams->qeu_retval = ADE_NOT_TRUE;

    if (qual_tuple == NULL || db_tuple == NULL)		/* Bad parm; abort! */

	return(E_DB_ERROR);

    if (MEcmp((PTR)&qual_tuple->du_own, 
	      (PTR)&db_tuple->du_own,
	      sizeof(qual_tuple->du_own)) == 0)
	qparams->qeu_retval = ADE_TRUE;		/* Qualifies */
    return (E_DB_OK);
}

/*
** Name: qeu_merge_user_profile
**
** Description:
**	This merges profile information with a user tuple
**
**       Algorithm:
**	 - For each attribute, if not specified by the user then use the
**	   profile value. For privileges the result is the union of the
**	   two sets.
** Inputs:
**	qef_cb,qeuq_cb  - useful QEF stuff
**
**	utuple		- formatted user tuple
**
** 	ptuple		- formatted profile tuple
**
** Outputs:
**	utuple		- merged user tuple
**
** History:
**	9-sep-93 (robf)
**	    Created
**	17-jul-96 (cwaldman)
**	    Overwrite profile specification for privileges and/or security
**	    audit options with user specification (if present) instead of
**	    merging them. 
**	    Treat privileges and security audit options as separate entities.
**	    (Historically, audit options are stored as privileges.)
**	28-Jul-2003 (gupsh01)
**	    Refix bug #74205, correctly merge the privileges information 
**	    from the profile with the users. If noprivileges is specified, 
**	    then do not merge privileges.
*/
DB_STATUS 
qeu_merge_user_profile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_USER		*utuple,
DU_PROFILE	*ptuple
) 
{
    DB_STATUS status=E_DB_OK;
    i4	error=0;

    for(;;)
    {
	/*
	** For each attribute merge in the info
	*/
	/* Privs/auditing, or together user and profile privs */
	/* check if the -noprivileges is specified */
	if ((utuple->du_flagsmask & DU_UPRIV) &&
	    (utuple->du_flagsmask & DU_UNOPRIV))
	{
	    utuple->du_status |= utuple->du_userpriv;

	    if (!((utuple->du_flagsmask & DU_UQRYTEXT) ||
	          (utuple->du_flagsmask & DU_UNOQRYTEXT) ||
	          (utuple->du_flagsmask & DU_UALLEVENTS) ||
		  (utuple->du_flagsmask & DU_UDEFEVENTS)))
	    {
		utuple->du_status |= (ptuple->du_status & DU_UAUDIT);
		utuple->du_status |= (ptuple->du_status & DU_UAUDIT_QRYTEXT);
	    }
	}
	else 
	{
	    /* if privileges are set then merge it 
	    ** do not merge if noprivilige flag is specified
	    */
	    utuple->du_status |= ptuple->du_status;
	    if ((utuple->du_flagsmask & DU_UDEFEVENTS) &&
		(ptuple->du_status & DU_UAUDIT))
		utuple->du_status ^= DU_UAUDIT;
	    if (utuple->du_flagsmask & DU_UALLEVENTS)
		utuple->du_status |= DU_UAUDIT;
	    if (utuple->du_flagsmask & DU_UQRYTEXT)
	        utuple->du_status |= DU_UAUDIT_QRYTEXT;
	    if ((utuple->du_flagsmask & DU_UNOQRYTEXT) &&
		(ptuple->du_status & DU_UAUDIT_QRYTEXT))
		utuple->du_status ^= DU_UAUDIT_QRYTEXT;
	}

	/* Default priv, or in profile defaults */
	if (!(utuple->du_flagsmask&DU_UDEFPRIV)
	    || (utuple->du_flagsmask&DU_UDEFALL))
		utuple->du_defpriv|=ptuple->du_defpriv;

	/* Expire date */
	if(!(utuple->du_flagsmask&DU_UEXPDATE))
	{
	    i4  result;
	    ADF_CB		    *adf_scb=qef_cb->qef_adf_cb; 
	    ADF_CB		    adf_cb;
	    DB_DATA_VALUE	    tdv;
	    DB_DATA_VALUE	    fdv;
	    DB_DATA_VALUE	    date_now;
	    DB_DATE		    dn_now;
	    DB_DATE		    dn_add;

	
   	   /* Copy the session ADF block into local one */
	   STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);

	   adf_cb.adf_errcb.ad_ebuflen = 0;
	   adf_cb.adf_errcb.ad_errmsgp = 0;


	   tdv.db_prec		    = 0;
	   tdv.db_length	    = DB_DTE_LEN;
	   tdv.db_datatype	    = DB_DTE_TYPE;
	   tdv.db_data		    = (PTR) &ptuple->du_expdate;
	   tdv.db_collID	    = -1;
	   /*
	   ** Check for date interval, convert to absolute if
	   ** necessary. We do NOT do this for profiles, since the resolution
	   ** occurs at user creation time
	   */
           status=adc_date_chk(&adf_cb, &tdv, &result);
	   if(status!=E_DB_OK)
	   	return E_DB_ERROR;
		
	   if(result==2)
	   {
	    	    /*
		    ** Interval, so normalize as offset from current date
		    */
	           /* fdv alreadyset  up as a CHA */
		   fdv.db_prec		= 0;
		   fdv.db_datatype	= DB_CHA_TYPE;
	           fdv.db_length	= 3;
	           fdv.db_data		=(PTR)"now";
		   fdv.db_collID	= -1;

	           date_now.db_prec	    = 0;
	           date_now.db_length	    = DB_DTE_LEN;
	           date_now.db_datatype	    = DB_DTE_TYPE;
	           date_now.db_data	    = (PTR) &dn_now;
	           date_now.db_collID	    = -1;
	    
	           if (adc_cvinto(&adf_cb, &fdv, &date_now) != E_DB_OK)
	           {
	    		return (E_DB_ERROR);
	           }
	           /*
	           ** Add 'now' and current to get final value
	           */
	           fdv.db_data 	= (PTR) &dn_add;
	           fdv.db_length    = DB_DTE_LEN;
	           fdv.db_datatype  = DB_DTE_TYPE;
	           status=adc_date_add(&adf_cb, &date_now, &tdv, &fdv);
	           if(status!=E_DB_OK)
	           {
			return status;
	           }
	           /*
	           ** Copy added date to output date
	           */
	           MECOPY_CONST_MACRO((PTR)&dn_add, sizeof(DB_DATE), 
				(PTR)&utuple->du_expdate);
	       }
	       else
	       {
		   MECOPY_CONST_MACRO((PTR)&ptuple->du_expdate,sizeof(DB_DATE), 
				(PTR)&utuple->du_expdate);
	       }
	}

	/* Default group */
     	if (!(utuple->du_flagsmask & DU_UGROUP))
	{
            STRUCT_ASSIGN_MACRO(ptuple->du_group, utuple->du_group);
	}

	/* Assign the new profile name */
        STRUCT_ASSIGN_MACRO(ptuple->du_name, utuple->du_profile);
	break;
    }
    if(error)
    {
	(VOID)qef_error(error, 0L, E_DB_ERROR,
	    &error, &qeuq_cb->error, 0);
    }
    return status;
}

/*
** Name: qeu_unmerge_user_profile
**
** Description:
**	This is the opposite of qeu_merge_user_profile and un-merges 
**	profile information from a user tuple
**
**       Algorithm:
**	 - For each attribute, if specified by the user then leave alone,
**	   otherwise set empty. For privileges leave alone.
**	   two sets.
** Inputs:
**	qef_cb,qeuq_cb  - useful QEF stuff
**
**	utuple		- formatted user tuple
**
** 	ptuple		- formatted profile tuple
**
** Outputs:
**	utuple		- unmerged user tuple
**
** History:
**	9-sep-93 (robf)
**	    Created
**	15-jul-94 (robf)
**          Preserve any audit bits when unmerging.
**	12-apr-95 (forky01)
**	    Checking for absence of DU_UPRIV is removed since it could
**	    never be stored in a tuple.  It was by definition an internal
**	    flag that was not stored in iiuser table, but was for
**	    showing a user was specifying privileges on a ALTER USER stmt.
**	    Also took out stmt which zeroed out du_defpriv when DU_UDEFPRIV
**	    wasn't on in tuple.  It was possible in pre-Secure 2.0 user tables
**	    to have default privs without the DU_UDEFPRIV bit on.
*/
DB_STATUS 
qeu_unmerge_user_profile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_USER		*utuple,
DU_PROFILE	*ptuple
) 
{
    DB_STATUS status=E_DB_OK;
    i4	error=0;

    for(;;)
    {
	/*
	** For each attribute unmerge the info
	*/

	/* Turn off privs from profile */
	utuple->du_status &= ~ptuple->du_status;

	/* Turn on user privs */
	utuple->du_status |= utuple->du_userpriv;

	/* Make sure default privs subset of privs */
	utuple->du_defpriv&=  utuple->du_status;

	/* Expire date */
	if(!(utuple->du_flagsmask&DU_UEXPDATE))
	{
	    /* Empty date */
    	    DB_DATA_VALUE	db_data;
	    /*
	    ** Initialize to the empty date
	    */
	    db_data.db_datatype  = DB_DTE_TYPE;
	    db_data.db_prec	 = 0;
	    db_data.db_length    = DB_DTE_LEN;
	    db_data.db_data	 = (PTR)&utuple->du_expdate;
	    db_data.db_collID	 = -1;
	    status = adc_getempty(qef_cb->qef_adf_cb, &db_data);
	    if(status)
		return status;
	}

	/* Default group */
     	if (!(utuple->du_flagsmask & DU_UGROUP))
	{
		/* No default group */
		MEfill(sizeof(utuple->du_group),
			' ',
			(PTR)&utuple->du_group);
	}

	MEfill(sizeof(utuple->du_profile),
		' ',
		(PTR)&utuple->du_profile);
	break;
    }
    if(error)
    {
	(VOID)qef_error(error, 0L, E_DB_ERROR,
	    &error, &qeuq_cb->error, 0);
    }
    return status;
}

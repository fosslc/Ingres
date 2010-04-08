/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = su4_u42 su4_cmw
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>
#include    <gcm.h>
#include    <gcn.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <cui.h>

/**
**
**  Name: SCSROLE.C - Role services for sessions
**
**  Description:
**      This file contains role services.
**
**	    scs_get_session_roles    - Get session roles.
**          scs_check_session_role   - Check if session can use a role.
**	    scs_load_role_dbpriv     - Load dbprivs for roles
**
**  History:
**      22-feb-94 (robf)
**          Created.
**	8-jun-94 (robf)
**          Error handling improvements
**	31-jun-94 (robf)
**          IIROLEGRANT now stores public grants as grantee "public" so
**	    use this instead of DB_ABSENT_NAME.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**    16-jul-96 (stephenb)
**        Add efective username to scs_check_external_password() parameters.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-dec-2008 (stial01)
**          Redefine PUBLIC_NAME without trailing blanks
**	30-Mar-2010 (kschendel) SIR 123485
**	    qef_call doesn't need PTR casts, fix.
*/
/*
**  Forward and/or External function references.
*/

/*
** Definition of all global variables owned by this file.
*/
# define PUBLIC_NAME "public"

GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* Central structure for SCF */
/*
** Static declarations
*/
static DB_STATUS get_roles_by_grantee ( SCD_SCB *scb, 
	QEU_CB 	*qeu, 
	QEU_CB 	*rqeu,
	DB_ROLEGRANT *rgrtuple,
	DB_APPLICATION_ID *roletuple 
);

/*{
** Name: scs_get_session_roles	- Get session roles. 
**
** Description:
**      This routine looks through iirolegrant for roles which
**	apply to the current session then saves them.
**
**	This routine must be called only when connected to the iidbdb,
**	and already in a transaction.
**
**	It is assumed  that scs_check_session_role() will be called at
**	some later point to selectively apply roles
** 
** Inputs:
**      scb				the session control block
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-feb-94  (robf)
**	   Created
**      17-oct-94 (canor01
**         scb->cs_scb.cs_self does not need casting to PTR
**         for assignment to qeu.qeu_d_id
*/
DB_STATUS
scs_get_session_roles(SCD_SCB *scb)
{
    DB_STATUS 		status=E_DB_OK;
    bool		loop=FALSE;
    bool		opened=FALSE;
    bool		role_opened=FALSE;
    QEU_CB		qeu;
    DMR_ATTR_ENTRY	key[1];
    DMR_ATTR_ENTRY	*key_ptr[1];
    QEU_CB		rqeu;
    DMR_ATTR_ENTRY	rkey[1];
    DMR_ATTR_ENTRY	*rkey_ptr[1];
    QEF_DATA		data;
    QEF_DATA		rdata;
    DB_ROLEGRANT 	rgrtuple;
    DB_APPLICATION_ID   roletuple;
    DB_OWN_NAME		grantee;

    do 
    {
	/*
	** If no roles, nothing to do
	*/
        if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NONE)
		break;

	scb->scb_sscb.sscb_rlcb=NULL; /* No role list */

	/*
	** Open iirolegrant
	*/
    	key_ptr[0] = &key[0];
	MEfill(sizeof(qeu), 0, (PTR)&qeu);
        qeu.qeu_eflag = QEF_INTERNAL;
        qeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
        qeu.qeu_d_id = scb->cs_scb.cs_self;
    	qeu.qeu_mask = 0;
	qeu.qeu_tab_id.db_tab_base = DM_B_IIROLEGRANT_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_IIROLEGRANT_TAB_ID;
        qeu.qeu_qual = 0;
        qeu.qeu_qarg = 0;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_access_mode = DMT_A_READ;

	status = qef_call(QEU_OPEN, &qeu);
	if (status)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_US247B_9339_IIROLEGRANT_OPEN, 0);
		break;
	}
	opened = TRUE;

	qeu.qeu_getnext = QEU_REPO;

	key[0].attr_number = DM_1_IIROLEGRANT_KEY;
	key[0].attr_operator = DMR_OP_EQ;
	key[0].attr_value = (char *) & scb->scb_sscb.sscb_ics.ics_rusername;
	qeu.qeu_klen = 1;
        qeu.qeu_key = (DMR_ATTR_ENTRY **) key_ptr;

	data.dt_data = (PTR) &rgrtuple;
	data.dt_size = sizeof(rgrtuple);
	data.dt_next=(QEF_DATA*)0;
	qeu.qeu_output= &data;
	qeu.qeu_count = 1;

	qeu.qeu_tup_length = sizeof(rgrtuple);
	/*
	** Open iirole 
	*/
	MEfill(sizeof(rqeu), 0, (PTR)&rqeu);
	rkey_ptr[0] = &rkey[0];
	rqeu.qeu_eflag = QEF_INTERNAL;
	rqeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
	rqeu.qeu_d_id = scb->cs_scb.cs_self;
	rqeu.qeu_mask = 0;
	rqeu.qeu_tab_id.db_tab_base = DM_B_APLID_TAB_ID;
	rqeu.qeu_tab_id.db_tab_index = DM_I_APLID_TAB_ID;
	rqeu.qeu_qual = 0;
	rqeu.qeu_qarg = 0;
	rqeu.qeu_lk_mode = DMT_IS;
	rqeu.qeu_access_mode = DMT_A_READ;

	status = qef_call(QEU_OPEN, &rqeu);
	if (status)
	{
		sc0e_0_put(rqeu.error.err_code, 0);
		sc0e_0_put(E_SC0352_IIROLE_ERROR, 0);
		break;
	}

	role_opened=TRUE;
	rkey[0].attr_number = DM_1_APLID_KEY;
	rkey[0].attr_operator = DMR_OP_EQ;
	rkey[0].attr_value = (char *) &rgrtuple.rgr_rolename;
	rqeu.qeu_klen = 1;
	rqeu.qeu_key = (DMR_ATTR_ENTRY **) rkey_ptr;

	rdata.dt_data = (PTR) &roletuple;
	rdata.dt_size = sizeof(roletuple);
	rdata.dt_next=(QEF_DATA*)0;
	rqeu.qeu_output= &rdata;
	rqeu.qeu_count = 1;

	rqeu.qeu_tup_length = sizeof(roletuple);

	/*
	** Look up PUBLIC grants
	*/
	STmove((char *)PUBLIC_NAME, ' ', sizeof(DB_OWN_NAME), grantee.db_own_name);
	key[0].attr_value = (char *) grantee.db_own_name;
	qeu.qeu_getnext = QEU_REPO;
	status=get_roles_by_grantee(scb, &qeu, &rqeu, &rgrtuple, &roletuple);

	if(status!=E_DB_OK)
	{
		break;
	}

	/*
	** Look up USER grants
	*/
	key[0].attr_value = (char *) & scb->scb_sscb.sscb_ics.ics_rusername;
	qeu.qeu_getnext = QEU_REPO;
	status=get_roles_by_grantee(scb, &qeu, &rqeu, &rgrtuple, &roletuple);
	if(status!=E_DB_OK)
	{
		break;
	}

    } while (loop);

    if(opened)
    {
	if( qef_call(QEU_CLOSE, &qeu)!=E_DB_OK)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_US247C_9340_IIROLEGRANT_ERROR, 0);
	}
    }

    if(role_opened)
    {
	if( qef_call(QEU_CLOSE, &rqeu)!=E_DB_OK)
	{
		sc0e_0_put(rqeu.error.err_code, 0);
		sc0e_0_put(E_SC0352_IIROLE_ERROR, 0);
	}
    }
    /*
    ** Report any errors
    */
    if(status!=E_DB_OK)
    {
	sc0e_0_put(E_SC0353_SESSION_ROLE_ERR, 0);
    }
    return status;
}

/*{
** Name: scs_check_session_role	- Checks whether a session may
**				          access a role
**
** Description:
**      This routine checks whether a session may use a role.
**
**	It is assumed  that a list of roles has already been built for
**	it to look through (e.g. in scs_get_session_role())
** 
** Inputs:
**      scb			 the session control block
**	rolename/password        Role name and password to check
**
** Outputs:
**	roleprivs                Pointer to privileges associated with 
**				 the role.
**
**	roledbpriv		 Pointer to dbprivs associated with role
**
**	Returns:
**	    E_DB_OK    Access is allowed
**	    E_DB_WARN  Access is not allowed
**	    E_DB_ERROR  Unexpected error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-feb-94  (robf)
**	   Created
**	18-mar-94 (robf)
**         Pass back dbprivs if requested.
**	21-mar-94 (robf)
**         Handle UPDATE_SYSCAT as special case - this is defined as a 
**	   dbpriv but is still handled as privilege in the code.
**    16-jul-96 (stephenb)
**        Add efective username to scs_check_external_password() parameters.
*/
DB_STATUS
scs_check_session_role(SCD_SCB *scb, 
	DB_OWN_NAME *rolename, 
	DB_PASSWORD *rolepassword,
	i4	    *roleprivs,
	SCS_DBPRIV  **roledbpriv)
{
    DB_STATUS status=E_DB_WARN;
    SCS_ROLE  *role;
    SCF_CB	scf_cb;

    for(;;)
    {
	/*
	** If no roles, nothing to do
	*/
        if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NONE)
	{
		status=E_DB_OK;
		break;
	}

	role=scb->scb_sscb.sscb_rlcb;
	/*
	** Loop over all roles looking for a matching name
	*/
	while(role)
	{
	    if(! MEcmp((PTR)rolename,(PTR)&role->roletuple.dbap_aplid,
			sizeof(DB_OWN_NAME)))
	    {
		break;
	    }
	    role=role->scrl_next;
	}
	if(role==NULL)
	{
	    /*
	    ** Not found, so no access.
	    */
	    status=E_DB_WARN;
	    break;
	}
	if (scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY)
	{
	    status=E_DB_OK;
	    break;
	}
	/*
	** Found the role, now do password checks
	*/
	/*
	** Check for external passwords
	*/
	if(role->roletuple.dbap_flagsmask & DU_UEXTPASS)
	{
	    if(!rolepassword)
	    {
		/* User didn't specify the password so error*/
		status=E_DB_WARN;
		break;
	    }
	    status=scs_check_external_password(scb, rolename, 
			(char*)scb->scb_sscb.sscb_ics.ics_eusername, 
			rolepassword, TRUE);
	    break;
	}
	/*
	** Check for regular passwords
	*/
	if ((STskipblank((PTR)&role->roletuple.dbap_apass,
		   sizeof(role->roletuple.dbap_apass))
		    != NULL) )
	{
	    /*
	    ** Role has a password
	    */
	    if(!rolepassword)
	    {
		/* User didn't specify the password so error*/
		status=E_DB_WARN;
		break;
	    }
	    /*
	    ** Encrypt password ready to check
	    */
	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type   = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_SCF_ID;
	    scf_cb.scf_session  = DB_NOSESSION;
	    /* key is role name */
	    scf_cb.scf_nbr_union.scf_xpasskey = (PTR)rolename;
	    /* role password */
	    scf_cb.scf_ptr_union.scf_xpassword = (PTR)rolepassword;

	    scf_cb.scf_len_union.scf_xpwdlen = sizeof(DB_PASSWORD);

	    status = scf_call(SCU_XENCODE,&scf_cb);
	    if (status)
	    {
	        status = (status > E_DB_ERROR) ?
	    	      status : E_DB_ERROR;
	        sc0e_0_put(E_SC0260_XENCODE_ERROR, 0);
	        break;
	    }
	    /*
	    ** Now compare encrypted password against saved value
	    */
	    if(MEcmp((PTR)rolepassword,
	       (PTR)&role->roletuple.dbap_apass,
	       sizeof(DB_PASSWORD)))
	    {
		/*
		** Invalid password
		*/
		status=E_DB_WARN;
		break;
	    }
	}
	else if(rolepassword)
	{
		/*
		** Role doesn't have a password but user specified one,
		** so fail.
		*/
		status=E_DB_WARN;
		break;
	}
	/*
	** No errors, so sucess
	*/
	status=E_DB_OK;
	break;
    }
    /*
    ** Pass back role privileges if OK.
    */
    if(roleprivs && status==E_DB_OK)
    {
	*roleprivs=role->roletuple.dbap_status;

	/*
	** If got role dbprivs, merge any update_syscat dbpriv
	** into the role privileges.
	*/
        if(role->roleflags&SCR_ROLE_DBPRIV) 
	{
	    /*
	    ** Since the update system catalog bit in iiuser is 
	    ** obsolete, we must handle this privilege with care.
	    */
	    if (role->roledbpriv.fl_dbpriv & DBPR_UPDATE_SYSCAT)
	    {
		*roleprivs |= DU_UUPSYSCAT;
	    }
	    else
	    {
		*roleprivs &= ~DU_UUPSYSCAT;
	    }
	}
    }

    if( status==E_DB_OK && roledbpriv)
    {
        if(role->roleflags&SCR_ROLE_DBPRIV) 
		*roledbpriv= &role->roledbpriv;
	else
		*roledbpriv= NULL;
    }
	  
    return status;
}

/*
** Name: get_roles_by_grantee
**
** Description:
**       Looks up (and saves if found) roles for a particular grantee
**	 It assumes the caller has already set up the iirolegrant/iirole
**	 catalog info ready for querying.
**	 This code is modularized to allow multiple calls, e.g 
**	 for user/public
**
** Inputs:
**	scb - scb
**
**	qeu - iirolegrant opened qeu block
** 
**	rqeu- iirole opened qeu block
**
** Outputs:
**	none
**
** History:
**	7-mar-94 (robf)
**	   created
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/
static DB_STATUS
get_roles_by_grantee ( SCD_SCB *scb, 
	QEU_CB 	*qeu, 
	QEU_CB 	*rqeu,
	DB_ROLEGRANT *rgrtuple,
	DB_APPLICATION_ID *roletuple
)
{
    DB_STATUS 	status=E_DB_OK;
    SCS_ROLE	*scs_role;
    SCS_ROLE	*role;

    for (;;)
    {
	/*
	** Loop over all rolegrants
	*/
	qeu->error.err_code=0;
        qeu->qeu_getnext = QEU_REPO;

	do {
	    status = qef_call(QEU_GET, qeu);

	    if(status!=E_DB_OK || qeu->error.err_code)
	    {
		if( qeu->error.err_code != E_QE0015_NO_MORE_ROWS)
		{
		    sc0e_0_put(qeu->error.err_code, 0);
		    sc0e_0_put(E_US247C_9340_IIROLEGRANT_ERROR, 0);
		    status=E_DB_ERROR;
		}
		else
		    status=E_DB_OK;
		break;
	    }


	    qeu->qeu_getnext = QEU_NOREPO;

	    /*
	    ** Check if this role already granted
	    */
	    role=scb->scb_sscb.sscb_rlcb;
	    /*
	    ** Loop over all roles looking for a matching name
	    */
	    while(role)
	    {
	        if(! MEcmp((PTR)&rgrtuple->rgr_rolename,
			(PTR)&role->roletuple.dbap_aplid,
			sizeof(DB_OWN_NAME)))
	        {
		    break;
	        }
	        role=role->scrl_next;
	    }
	    if(role!=NULL)
		continue;
	    /*
	    ** Got a role grant, so save
	    */
	    status = sc0m_allocate(SCU_MZERO_MASK,
			    (i4)sizeof(SCS_ROLE),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCR_TAG,
			    (PTR*)&scs_role);
	    if(status!=E_DB_OK)
	    {
		/* sc0m_allocate puts error codes in return status */
	        sc0e_0_put(status, 0);
		status=E_DB_SEVERE;
		break;
	    }
	    scs_role->scrl_next= scb->scb_sscb.sscb_rlcb;
	    scb->scb_sscb.sscb_rlcb= scs_role;

	    scs_role->roleflags=0;
	    scs_init_dbpriv(scb, &(scs_role->roledbpriv), FALSE);
	    /*
	    ** Process this role grant. 
	    */
	    rqeu->qeu_getnext = QEU_REPO;
	    status = qef_call(QEU_GET, rqeu);

	    if(status!=E_DB_OK || rqeu->error.err_code)
            {		
		if(rqeu->error.err_code>0)
			sc0e_0_put(rqeu->error.err_code,0);

		sc0e_put(E_SC0351_IIROLE_GET_ERROR, 0,1,
			scs_trimwhite(sizeof(rgrtuple->rgr_rolename),
				(char*)&rgrtuple->rgr_rolename),
			(PTR)&rgrtuple->rgr_rolename,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0 );
		if(status<E_DB_ERROR)
			status=E_DB_ERROR;
		break;
	    }
	    /*
	    ** Got role, so save info for later use
	    */
	    MEcopy( (PTR)roletuple, sizeof(DB_APPLICATION_ID),
			(PTR)&scs_role->roletuple);	}
	while (status == 0);

	break;
    }
    return status;
}

/*
** Name: scs_load_role_dbpriv - Load dbprivs for a role
**
** Description:
**	This walks through all the session roles loading associated
**	dbprivs from iidbpriv and saving with the role info for later
**	use.
**
** Inputs:
**	scb 		- SCB
**
**	qeu     	- Open QEU CB for iidbpriv
**
**	dbpriv_tup 	- dbpriv tuple
**
** History:
**	18-mar-94 (robf)
**         Created
*/
DB_STATUS
scs_load_role_dbpriv(SCD_SCB *scb ,
    QEU_CB 		*qeu,
    DB_PRIVILEGES	*dbpriv_tup
)
{
    DB_STATUS 		status=E_DB_OK;
    SCS_ROLE  		*role;
    SCS_DBPRIV 		dbpriv;
    bool      		loop=FALSE;
    DMR_ATTR_ENTRY	**key_ptr;

    do
    {
        key_ptr=qeu->qeu_key;

        dbpriv_tup->dbpr_gtype = DBGR_APLID;
	/*
	** Loop over all roles
	*/
        for (role=scb->scb_sscb.sscb_rlcb; 
		role!=NULL;
	        role=role->scrl_next
	)
	{
		/*
		** Initialize to no privileges
		*/
		scs_init_dbpriv(scb, &dbpriv, FALSE);

	        /* Get dbprivs granted to role on the specific database */
		key_ptr[0]->attr_value = (char*)&role->roletuple.dbap_aplid;
		key_ptr[1]->attr_value = (char *) &dbpriv_tup->dbpr_gtype;
		key_ptr[2]->attr_value =
			(char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		status = scs_get_dbpriv(scb, qeu, dbpriv_tup, &dbpriv);
		if (status == E_DB_OK)
			role->roleflags|=SCR_ROLE_DBPRIV;
		else if (status==E_DB_INFO)
			status=E_DB_OK;
		else
			break;

		/* Get dbprivs granted to role on all databases */
		dbpriv_tup->dbpr_gtype = DBGR_APLID;
		key_ptr[2]->attr_value = (char *)DB_ABSENT_NAME;
		status = scs_get_dbpriv(scb, qeu, dbpriv_tup, &dbpriv);
		if (status == E_DB_OK)
			role->roleflags|=SCR_ROLE_DBPRIV;
		else if (status==E_DB_INFO)
			status=E_DB_OK;
		else
			break;

		if ( role->roleflags&SCR_ROLE_DBPRIV)
		{
		    /*
		    ** Save dbprivs with role
		    */
		    STRUCT_ASSIGN_MACRO(dbpriv, role->roledbpriv);
		}
	}
    } while(loop);

    if(status!=E_DB_OK)
    {
	/* Log trace back message */
	sc0e_0_put(E_SC035E_LOAD_ROLE_DBPRIV, 0);
    }
    return status;
}

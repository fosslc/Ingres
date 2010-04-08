/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcs.h>
#include <servermeth.h>

/*
**
**  Name: wmo_init.c - Web Monitoring Object
**
**  Description:
**		Declare all server functions with their parameters
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      03-Nov-98 (fanra01)
**          Renamed app to sess;
**      27-Nov-1998 (fanra01)
**          Add WMOGetSvrVariables.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add an icevariables function that supports full range of actions
**          and returns scope.
**/

#define WMO_FUNCTION(X) GSTATUS X(ACT_PSESSION act_session, char* params[], PTR*  user_info)

WMO_FUNCTION(WMODBUser);
WMO_FUNCTION(WMORole);
WMO_FUNCTION(WMOUser);
WMO_FUNCTION(WMODB);
WMO_FUNCTION(WMOUserRole);
WMO_FUNCTION(WMOUserDB);
WMO_FUNCTION(WMOUnit);
WMO_FUNCTION(WMODocument);
WMO_FUNCTION(WMODocRole);
WMO_FUNCTION(WMODocUser);
WMO_FUNCTION(WMOUnitRole);
WMO_FUNCTION(WMOUnitUser);
WMO_FUNCTION(WMOLocation);
WMO_FUNCTION(WMOUnitLoc);
WMO_FUNCTION(WMOCache);
WMO_FUNCTION(WMOUserSession);
WMO_FUNCTION(WMOActiveSession);
WMO_FUNCTION(WMOProfile);
WMO_FUNCTION(WMOProfileRole);
WMO_FUNCTION(WMOProfileDB);
WMO_FUNCTION(WMOTag2String);
WMO_FUNCTION(WMOGetLocFiles);
WMO_FUNCTION(WMOGetVariables);
WMO_FUNCTION(WMOGetSvrVariables);
WMO_FUNCTION(WMOIceVariables);
WMO_FUNCTION(WMOUserTransaction);
WMO_FUNCTION(WMOUserCursor);
WMO_FUNCTION(WMOConnection);
WMO_FUNCTION(WMOUnitCopy);
WMO_FUNCTION(WMOApplication);

/*
** Name: WMOInitialize() - Initialize Server Functions 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Jan-1999 (fanra01)
**          Add auth type to ice users.
**      25-Jan-1999 (fanra01)
**          Update profile parameter list. profile_monitor is parameter 7 and
**          profile_timeout is parameter 8.
**      04-May-2001 (fanra01)
**          Add the initialization of ice_var as a server function.
*/
GSTATUS 
WMOInitialize () 
{
	GSTATUS err = GSTAT_OK;
	/* security */
	char* DBuser[] = {"action", "dbuser_id", "dbuser_alias", "dbuser_name", "dbuser_password1", "dbuser_password2", "dbuser_comment", NULL};
	char* Role[] = {"action", "role_id", "role_name", "role_comment", NULL};
	char* User[] = {"action", "user_id", "user_name", "user_password1", "user_password2", "user_dbuser", "user_comment", "user_administration", "user_security", "user_unit", "user_timeout", "user_monitor", "user_authtype", NULL};
	char* DB[] = {"action", "db_id", "db_name", "db_dbname", "db_dbuser", "db_comment", NULL};
	char* UserRole[] = {"action", "ur_user_id", "ur_role_id", "ur_role_name", NULL};
	char* UserDB[] = {"action", "ud_user_id", "ud_db_id", "ud_db_name", NULL};

        /* session group */
	char* sess[] = {"action", "sess_id", "sess_name", NULL};

	/* business unit */
	char* unit[] = {"action", "unit_id", "unit_name", "unit_owner", NULL};
	char* doc[] = {"action", "doc_id", "doc_unit_id", "doc_unit_name", "doc_name", "doc_suffix", "doc_public", "doc_pre_cache", "doc_perm_cache", "doc_session_cache", "doc_file", "doc_ext_loc", "doc_ext_file", "doc_ext_suffix", "doc_owner", "doc_transfer"
 , "doc_type", NULL};
	char* docRole[] = {"action", "dr_doc_id", "dr_role_id", "dr_role_name", "dr_read", "dr_update", "dr_delete", "dr_execute", NULL};
	char* docUser[] = {"action", "du_doc_id", "du_user_id", "du_user_name", "du_read", "du_update", "du_delete", "du_execute", NULL};
	char* unitRole[] = {"action", "ur_unit_id", "ur_role_id", "ur_role_name", "ur_execute", "ur_read", "ur_insert", NULL};
	char* unitUser[] = {"action", "uu_unit_id", "uu_user_id", "uu_user_name", "uu_execute", "uu_read", "uu_insert", NULL};
	char* unitLoc[] = {"action", "ul_unit_id", "ul_location_id", "ul_location_name", NULL};
	char* unitCopy[] = {"action", "unit_id", "copy_file", "default_loc", NULL};

	/* monitoring */
	char* location[] = {"action", "loc_id", "loc_name", "loc_path", "loc_extensions", "loc_http", "loc_ice", "loc_public", NULL};

	char* activeSession[] = {"action", "name", "ice_user", "host", "query", "err_count", NULL};
	char* userSession[] = {"action", "name", "user", "req_count", "timeout", NULL};
	char* userTransaction[] = {"action", "key", "name", "owner", "connection", NULL};
	char* userCursor[] = {"action", "key", "name", "query", "owner", NULL};
	char* cache[] = {"action", "key", "name", "loc_name", "status", "file_counter", "exist", "owner", "timeout", "in_use", "req_count", NULL};
	char* connection[] = {"action", "key", "driver", "dbname", "used", "timeout", NULL};

	char* profile[] = {"action", "profile_id", "profile_name", "profile_dbuser", "profile_administration", "profile_security", "profile_unit", "profile_monitor", "profile_timeout", NULL};
	char* profileRole[] = {"action", "pr_profile_id", "pr_role_id", "pr_role_name", NULL};
	char* profileDB[] = {"action", "pd_profile_id", "pd_db_id", "pd_db_name", NULL};
	char* tag2String[] = {"tags", "string", NULL};
	char* getLocFiles[] = {"location_id", "prefix", "suffix", NULL};
	char* getVariables[] = {"page", "session", "server", "name", "value", NULL};
	char* getSvrVariables[] = {"action", "name", "value", NULL};

	char* icevariables[] = {"action", "scope", "name", "value", NULL};

	err = WSFAddServerFunction(NULL, WSF_SVR_DBUSER, (PTR) WMODBUser, DBuser);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_ROLE, (PTR) WMORole, Role);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_USER, (PTR) WMOUser, User);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_DB, (PTR) WMODB, DB);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UR, (PTR) WMOUserRole, UserRole);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UD, (PTR) WMOUserDB, UserDB);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UNIT, (PTR) WMOUnit, unit);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_DOCUMENT, (PTR) WMODocument, doc);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_DOCROLE, (PTR) WMODocRole, docRole);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_DOCUSER, (PTR) WMODocUser, docUser);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UNITROLE, (PTR) WMOUnitRole, unitRole);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UNITUSER, (PTR) WMOUnitUser, unitUser);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_LOC, (PTR) WMOLocation, location);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UNITLOC, (PTR) WMOUnitLoc, unitLoc);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_UNITCOPY, (PTR) WMOUnitCopy, unitCopy);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_CACHE, (PTR) WMOCache, cache);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_USR_SESS, (PTR) WMOUserSession, userSession);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_ACT_SESS, (PTR) WMOActiveSession, activeSession);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_USR_TRANS, (PTR) WMOUserTransaction, userTransaction);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_USR_CURS, (PTR) WMOUserCursor, userCursor);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_CONN, (PTR) WMOConnection, connection);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_PROFILE, (PTR) WMOProfile, profile);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_PR, (PTR) WMOProfileRole, profileRole);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_PD, (PTR) WMOProfileDB, profileDB);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_TAG_2_STR, (PTR) WMOTag2String, tag2String);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_GET_LOC, (PTR) WMOGetLocFiles, getLocFiles);
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_GET_VAR, (PTR) WMOGetVariables, getVariables);
	if (err == GSTAT_OK)
        {
		err = WSFAddServerFunction(NULL, WSF_SVR_GET_SVRVAR,
                    (PTR) WMOGetSvrVariables, getSvrVariables);
        }
	if (err == GSTAT_OK)
        {
		err = WSFAddServerFunction(NULL, WSF_SVR_ICE_VAR,
                    (PTR) WMOIceVariables, icevariables);
        }
	if (err == GSTAT_OK)
		err = WSFAddServerFunction(NULL, WSF_SVR_SESSGRP, (PTR) WMOApplication, sess);

return(err);
}

/*
** Name: WMOTypeAction() - Convert the action to a number
**
** Description:
**	The action text can be select, insert, update, delete and retrieve
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WMOTypeAction (
	char* action,
	u_i4 *type)
{
	*type = 0;

	if (STcompare(action, WSF_SVR_ACT_SEL) == 0)
		*type = WSS_ACTION_TYPE_SEL;
	else if (STcompare(action, WSF_SVR_ACT_RET) == 0)
		*type = WSS_ACTION_TYPE_RET;
	else if (STcompare(action, WSF_SVR_ACT_INS) == 0)
		*type = WSS_ACTION_TYPE_INS;
	else if (STcompare(action, WSF_SVR_ACT_UPD) == 0)
		*type = WSS_ACTION_TYPE_UPD;
	else if (STcompare(action, WSF_SVR_ACT_DEL) == 0)
		*type = WSS_ACTION_TYPE_DEL;

return(GSTAT_OK);
}

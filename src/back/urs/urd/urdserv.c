/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include    <compat.h>
#include    <ddgexec.h>
#include    <cs.h>
#include    <er.h>
#include    <gl.h>
#include    <lo.h>
#include    <nm.h>
#include    <pm.h>
#include    <sl.h>
#include    <iicommon.h>

#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>

#include    <ursm.h>

/**
**
**  Name: urdserv - User Request Data Dictionary Services
**
**  Description:
**	Provide a set of functions to execute Data
**	Dictionary Services for the URSM.
**
** 	urd_ddg_init - Initialize the data dictionary services
** 	urd_ddg_term - Terminate the data dictionary services
** 	urd_connect - Connect a data dictionary session
** 	urd_disconnect - Disconnect from the data dictionary facility
** 	urd_get_appl	- Get the definition of the application
** 	urd_get_intfc	- Get the definition of the interface
** 	urd_get_method	- Get the definition of the method
** 	urd_get_arg	- Get the definition of the arg
**
**  History:
**  17-mar-1998 (wonst02)
**	created
**  21-Mar-1998 wonst02
**	Add the missing Interface layer.
**  21-may-1998 (wonst02)
** 	Had to modify DDG calls for the new parameters required.
**  11-Aug-1998 (fanra01)
**	Fixed incomplete last line.
**  27-Apr-1999 (fanra01)
**      Removed warning message for missing config.dat parameter.  icesvr is
**      the only component of the application server at the moment.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
/*
** Global constants
*/
static char szIngres_Bin[] = "\\ingres\\bin\\";
static char szDll[] = ".dll";
static char szSlash[] = "\\";
static char szIngres_W4glapps[] = "\\ingres\\w4glapps\\";

/*
** Forward references
*/

/*{
** Name: urd_ddg_init - Initialize the data dictionary services
**
** Description:
**  Initialize the data dictionary services for the User Request
**  Services Manager (URSM). This routine should be called once, at
**  startup. It does not contain semaphore protection for being called
**  by multiple callers.
**
**  The data dictionary service is described in the config file:
**
**	ii.<host>.frontier.dictionary.name:	<database name>
**	ii.<host>.frontier.dictionary.driver:	<dll name>
**	ii.<host>.frontier.dictionary.node:	<server node name>
**	ii.<host>.frontier.dictionary.class:	<server class>
**
** Inputs:
**  ursm		The URSM main control block.
**
** Outputs:
**  ursm
**	.ursm_ddg_descr     	Contains a ptr to the DDG_DESCR block
**		    		if E_DB_OK returned; otherwise, NULL.
**
**  Returns:
**	E_DB_OK 		Initialized properly
** 	E_DB_WARN		The dictionary was not defined in config.dat
**	E_DB_ERROR	    	Failure... see ursm->ursm_error.
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  17-mar-1998 (wonst02)
**	Original
**  21-may-1998 (wonst02)
** 	Add userid and password to DDG call.
**  27-Apr-1999 (fanra01)
**      Removed warning message for missing config.dat parameter.  icesvr is
**      the only component of the application server at the moment.
*/
DB_STATUS
urd_ddg_init(URS_MGR_CB *ursm,
	     URSB	*ursb)
{
    ULM_RCB	*ulm = &ursm->ursm_ulm_rcb;
    DDG_DESCR	*pdescr;
    GSTATUS	err = GSTAT_OK;
    char	*dictionary = NULL;
    char	*node = NULL;
    char	*svrclass = NULL;
    char	*dllname = NULL;
    char	*hostname = NULL;
    char	szConfig [256];
    u_i4	module_number;
    DB_STATUS	status = E_DB_OK;

    /*
    ** Use the URSM memory stream for data dictionary services.
    */
    if ((ursm->ursm_ddg_descr = 
    	 urs_palloc(ulm, ursb, sizeof(DDG_DESCR))) == NULL)
	return E_DB_ERROR;
    pdescr = (DDG_DESCR *)ursm->ursm_ddg_descr;
    ursm->ursm_ddg_connect = 0;
    CSw_semaphore(&ursm->ursm_ddg_sem, CS_SEM_SINGLE, "URSM_DDG_SEM");

    hostname = PMhost();
    CVlower(hostname);

    do					/* A block to break from */
    {
	STprintf (szConfig, FAS_CONF_REPOSITORY, hostname);
	if ( (PMget( szConfig, &dictionary ) != OK) ||
	     (*dictionary == '\0') )	
	{
	    status = E_DB_WARN;
	    break;
	}

	STprintf (szConfig, FAS_CONF_DLL_NAME, hostname);
	if (PMget( szConfig, &dllname ) != OK)
	{
	    urs_error(E_UR0414_UNDEF_DLL_NAME, URS_INTERR, 1,
		      STlength(szConfig), szConfig);
	    status = E_DB_ERROR;
	    break;
	}

	STprintf (szConfig, FAS_CONF_NODE, hostname);
	PMget( szConfig, &node );

	STprintf (szConfig, FAS_CONF_CLASS, hostname);
	PMget( szConfig, &svrclass );

	module_number = _UR_CLASS * 0x10000 + ursm->ursm_srvr_id;
	err = DDGInitialize(module_number, dllname,
			    node, dictionary, svrclass, "", "", pdescr);
	if (err != GSTAT_OK)
	{
	    i4		len = 0;
	    char	*ptr = "";

	    if (err->info)
	    {
	    	len = STlength(err->info);
		ptr = err->info;
	    }
	    urs_error(err->number, URS_INTERR, 0);
	    urs_error(E_UR0350_DDG_INIT_ERROR, URS_INTERR, 1, len, ptr);
	    DDFStatusFree(TRC_EVERYTIME, &err);
	    ursm->ursm_ddg_descr = NULL;
	    status = E_DB_ERROR;
	    break;
	}
    } while (0);

    if (status)
	SET_URSB_ERROR(ursb, E_UR0103_URD_DDG_INIT, status)

    return status;
}

/*{
** Name: urd_ddg_term - Terminate the data dictionary services
**
** Description:
**  Terminate the data dictionary services for the User Request
**  Services Manager (URSM). This routine should be called once, at
**  shutdown. It does not contain semaphore protection for being called
**  by multiple callers.
**
** Inputs:
**  ursm		The URSM main control block.
**	.ursm_ddg_descr     A ptr to the DDG_DESCR block.
**
** Outputs:
**  ursm
**	.ursm_ddg_descr     Set to NULL.
**
**  Returns:
**	E_DB_OK 	Terminated properly
**	E_DB_WARN	    The ursm_ddg_descr ptr was already NULL.
**	E_DB_ERROR	    Failure... see ursb_error.
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  19-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_ddg_term(URS_MGR_CB *ursm,
	     URSB	*ursb)
{
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (ursm->ursm_ddg_descr == NULL)
    {
	SET_URSB_ERROR(ursb, E_UR0104_URD_DDG_DESCR_NULL, E_DB_WARN)
	return E_DB_WARN;
    }

    err = DDGTerminate((PDDG_DESCR)ursm->ursm_ddg_descr);
    if (err != GSTAT_OK)
    {
	i4	len = 0;
	char	*ptr = "";

	if (err->info)
	{
	    len = STlength(err->info);
	    ptr = err->info;
	}
	urs_error(err->number, URS_INTERR, 0);
	urs_error(E_UR0351_DDG_TERM_ERROR, URS_INTERR, 1, len, ptr);
	DDFStatusFree(TRC_EVERYTIME, &err);
	SET_URSB_ERROR(ursb, E_UR0351_DDG_TERM_ERROR, E_DB_ERROR)
	status = E_DB_ERROR;
    }

    CSr_semaphore( &ursm->ursm_ddg_sem );
    ursm->ursm_ddg_descr = NULL;
    return status;
}

/*{
** Name: init_session - Initialize the data dictionary session
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  21-mar-1998 (wonst02)
**	Original
**  21-may-1998 (wonst02)
** 	Add userid and password to DDG call.
*/
DDG_SESSION *
init_session(URS_MGR_CB *ursm,
	     URSB	*ursb)
{
    DDG_SESSION *session;
    ULM_RCB	*ulm = &ursm->ursm_ulm_rcb;
    GSTATUS	err = GSTAT_OK;

    if (ursm->ursm_ddg_session == NULL)
    {
    	/*
    	** Use the URSM memory stream for data dictionary services.
    	*/
    	if ((ursm->ursm_ddg_session = 
    	 	urs_palloc(ulm, ursb, sizeof(DDG_SESSION))) == NULL)
	    return NULL;
    }
    session = (DDG_SESSION *)ursm->ursm_ddg_session;

    err = DDGConnect((PDDG_DESCR)ursm->ursm_ddg_descr, 
    		     "",			/* username */
		     "",			/* password */
		     session);

    if (err != GSTAT_OK)
    {
	i4	len = 0;
	char	*ptr = "";

	if (err->info)
	{
	    len = STlength(err->info);
	    ptr = err->info;
	}
	urs_error(err->number, URS_INTERR, 0);
	urs_error(E_UR0352_DDG_CONNECT_ERROR, URS_INTERR, 1, len, ptr);
	DDFStatusFree(TRC_EVERYTIME, &err);
	return NULL;
    }

    return session;
}

/*{
** Name: urd_connect - Connect a data dictionary session
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**	15-Apr-1998 (wonst02)
**	    Original
*/
DB_STATUS
urd_connect(URS_MGR_CB	*ursm,
	    URSB	*ursb)
{
    if (ursm->ursm_ddg_descr == NULL)
    {
	urs_error(E_UR0104_URD_DDG_DESCR_NULL, URS_INTERR, 0);
	return E_DB_ERROR;
    }

    CSp_semaphore( TRUE, &ursm->ursm_ddg_sem );
    if (ursm->ursm_ddg_connect == 0)
    {
    	DDG_SESSION	*session;

	session = init_session(ursm, ursb);
	if (session == NULL)
	{	
	    CSv_semaphore( &ursm->ursm_ddg_sem );
	    SET_URSB_ERROR(ursb, E_UR0352_DDG_CONNECT_ERROR, E_DB_ERROR)
	    return E_DB_ERROR;
	}
    }

    ursm->ursm_ddg_connect++;
    CSv_semaphore( &ursm->ursm_ddg_sem );
    return E_DB_OK;
}

/*{
** Name: urd_disconnect - Disconnect from the data dictionary facility
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
** 	E_DB_OK				Disconnected OK
** 	E_DB_WARN			Was not connected
** 	E_DB_ERROR			Error while disconnecting
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  30-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_disconnect(URS_MGR_CB   *ursm)
{
    DDG_SESSION *session = (DDG_SESSION *)ursm->ursm_ddg_session;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    CSp_semaphore( TRUE, &ursm->ursm_ddg_sem );

    if (ursm->ursm_ddg_connect == 0)
    	status = E_DB_WARN;
    else if (--ursm->ursm_ddg_connect == 0)
    {	
    	err = DDGDisconnect(session);
	MEfill(sizeof *session, 0, session);
    }

    CSv_semaphore( &ursm->ursm_ddg_sem );

    if (err != GSTAT_OK)
    {
	i4	len = 0;
	char	*ptr = "";

	if (err->info)
	{
	    len = STlength(err->info);
	    ptr = err->info;
	}
	urs_error(err->number, URS_INTERR, 0);
	urs_error(E_UR0354_DDG_DISCONNECT_ERROR, URS_INTERR, 1, len, ptr);
	DDFStatusFree(TRC_EVERYTIME, &err);
	status = E_DB_ERROR;
    }

    return status;
}

/*{
** Name: get_appl   Get the application information
**
** Description:
**	Get the application information for the next application defined.
**  This information includes:
**	    fas_appl_name	Name of the application
**	    fas_appl_loc	Location (path)
**	    fas_appl_type	Type: OpenRoad or C++
**
** Inputs:
**	ursm		    The URS Manager Control Block
**  session		The data dictionary session block
**  appl		Address of an FAS application block
**
** Outputs:
**  appl		A filled-in FAS application block
**
**  Returns:
**	E_DB_OK 	Application info returned OK
**	E_DB_INFO	    No more applications defined
**	E_DB_ERROR	    Error getting applications definition
**			    (ursb_error field has more info)
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  21-Mar-1998 wonst02
**	Add the missing Interface layer.
*/
DB_STATUS
get_appl(URS_MGR_CB	*ursm,
	 DDG_SESSION	*session,
	 FAS_APPL	*appl,
	 URSB		*ursb)
{
    bool	exist = FALSE;
    i4		*srvr_id = NULL;
    i4		*appl_id = NULL;
    char	*appl_name = NULL;
    char	*appl_path = NULL;
    i4		*appl_type = NULL;
    char	*appl_parm = NULL;
    ULM_RCB	*ulm = &ursm->ursm_srvr->fas_srvr_ulm_rcb;
    char	*path;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (appl->fas_appl_id == 0)
	err = DDGSelectAll(session, FAS_OBJ_APPL);

    if (err == GSTAT_OK)
	err = DDGNext(session, &exist);

    if (err == GSTAT_OK && exist)
    {
	err = DDGProperties(session,
			    "%d%d%s%s%d%s",
			    &srvr_id,
			    &appl_id,
			    &appl_name,
			    &appl_path,
			    &appl_type,
			    &appl_parm);
    }
    if (err == GSTAT_OK && exist)
    {
	appl->fas_appl_id = *appl_id;
	appl->fas_appl_type = *appl_type;
	appl->fas_appl_flags = 0;
	STmove(appl_name, ' ', sizeof appl->fas_appl_name.db_name,
	       appl->fas_appl_name.db_name);
	path = urs_palloc(ulm, ursb, MAX_LOC + STlength(appl_parm) + 2);
	if (path == NULL)
	    return E_DB_ERROR;
	if (appl_path[0] == '\0')
	{
	    char    *p = NULL;

	    NMgtAt (SYSTEM_LOCATION_VARIABLE, &p);
	    STprintf(path, INGRES_BIN_DIR, p);
	}
	else
	{
	    STcopy(appl_path, path);
	}
	LOfroms(PATH, path, &appl->fas_appl_loc);
	appl->fas_appl_parm = (char *)path + MAX_LOC + 1;
	STcopy(appl_parm, appl->fas_appl_parm);
	status = E_DB_OK;
    }
    else
    {
	DDGClose(session);
	if (err != GSTAT_OK)
	{
	    i4		len = 0;
	    char	*ptr = "";

	    if (err->info)
	    {
	    	len = STlength(err->info);
		ptr = err->info;
	    }
	    urs_error(err->number, URS_INTERR, 0);
	    urs_error(E_UR0353_DDG_SELECT_ERROR, URS_INTERR, 1, len, ptr);
	    DDFStatusFree(TRC_EVERYTIME, &err);
	    status = E_DB_ERROR;
	}
	else
	    status = E_DB_INFO;
    }

    return status;
}

/*{
** Name: urd_get_appl	- Get the definition of the application
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  17-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_get_appl(URS_MGR_CB *ursm,
	     FAS_APPL	*appl,
	     URSB	*ursb)
{
    DDG_SESSION *session = (DDG_SESSION *)ursm->ursm_ddg_session;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (ursm->ursm_ddg_descr == NULL)
    {
	urs_error(E_UR0104_URD_DDG_DESCR_NULL, URS_INTERR, 0);
	return E_DB_ERROR;
    }

    if (session == NULL)
    {
	urs_error(E_UR0107_URD_DDG_SESSION_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0170_APPL_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    status = get_appl(ursm, session, appl, ursb);
    if (DB_FAILURE_MACRO(status))
	SET_URSB_ERROR(ursb, E_UR0170_APPL_LOOKUP, status)

    return status;
}

/*{
** Name: get_intfc  Get the interface information
**
** Description:
**	Get the interface information for the next interface defined.
**	This information includes:
**	    fas_intfc_name	Name of the interface
**	    fas_appl_loc	Location (path)
**	    fas_appl_type	Type: OpenRoad or C++
**
** Inputs:
**	ursm		    	The URS Manager Control Block
**	session			The data dictionary session block
**	intfc			Address of an FAS interface block
**
** Outputs:
**	intfc			A filled-in FAS interface block
**
**  Returns:
**	E_DB_OK 	    Interface info returned OK
**	E_DB_INFO	    No more interfaces defined
**	E_DB_ERROR	    Error getting interfaces definition
**			    (ursb_error field has more info)
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  26-Mar-1998 wonst02
**	Add the missing Interface layer.
*/
DB_STATUS
get_intfc(URS_MGR_CB	*ursm,
	  DDG_SESSION	*session,
	  FAS_INTFC	*intfc,
	  URSB		*ursb)
{
    bool	exist = FALSE;
    i4		*srvr_id = NULL;
    i4		*appl_id = NULL;
    i4		*intfc_id = NULL;
    char	*intfc_name = NULL;
    char	*intfc_path = NULL;
    char	*intfc_parm = NULL;
    ULM_RCB	*ulm = &ursm->ursm_srvr->fas_srvr_ulm_rcb;
    char	*path;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (intfc->fas_intfc_flags & FAS_SEARCH)
    {					/* Search by name */
	intfc_name = intfc->fas_intfc_name.db_name;
	err = DDGKeySelect(session, FAS_OBJ_INTFC, "%s",
			   &intfc_name);
    }
    else
    {					/* Scan all */
    	if (intfc->fas_intfc_id == 0)
	    err = DDGSelectAll(session, FAS_OBJ_INTFC);
    }

    if (err == GSTAT_OK)
        err = DDGNext(session, &exist);

    if (err == GSTAT_OK && exist)
    {
        err = DDGProperties(session,
			    "%d%d%d%s%s%s",
    		    	    &srvr_id,
    		    	    &appl_id,
    		    	    &intfc_id,
    		    	    &intfc_name,
    		    	    &intfc_path,
    		    	    &intfc_parm);
    }
    if (err == GSTAT_OK && exist)
    {
	intfc->fas_intfc_appl_id = *appl_id;
	intfc->fas_intfc_id = *intfc_id;
	intfc->fas_intfc_flags = 0;
	STmove(intfc_name, ' ', sizeof intfc->fas_intfc_name.db_name,
	       intfc->fas_intfc_name.db_name);
	path = urs_palloc(ulm, ursb, MAX_LOC + STlength(intfc_parm) + 2);
	if (path == NULL)
	    return E_DB_ERROR;
	if (intfc_path[0] == '\0')
	{
	    char    *p = NULL;

	    NMgtAt (SYSTEM_LOCATION_VARIABLE, &p);
	    STprintf(path, INGRES_W4GL_DIR, p);
	}
	else
	{
	    STcopy(intfc_path, path);
	}
	LOfroms(PATH, path, &intfc->fas_intfc_loc);
	intfc->fas_intfc_parm = (char *)path + MAX_LOC + 1;
	STcopy(intfc_parm, intfc->fas_intfc_parm);
	status = E_DB_OK;
    }
    if (err != GSTAT_OK || !exist || intfc->fas_intfc_flags & FAS_SEARCH)
    {
	DDGClose(session);
	if (err != GSTAT_OK)
	{
	    i4		len = 0;
	    char	*ptr = "";

	    if (err->info)
	    {
	    	len = STlength(err->info);
		ptr = err->info;
	    }
	    urs_error(err->number, URS_INTERR, 0);
	    urs_error(E_UR0353_DDG_SELECT_ERROR, URS_INTERR, 1, len, ptr);
	    DDFStatusFree(TRC_EVERYTIME, &err);
	    status = E_DB_ERROR;
	}
	else
	    status = E_DB_INFO;
    }

    return status;
}

/*{
** Name: urd_get_intfc	- Get the definition of the interface
**
** Description:
** 	Get the definition of the next interface from the repository.
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  26-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_get_intfc(URS_MGR_CB    *ursm,
	      FAS_INTFC     *intfc,
	      URSB	    *ursb)
{
    DDG_SESSION *session = (DDG_SESSION *)ursm->ursm_ddg_session;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (ursm->ursm_ddg_descr == NULL)
    {
	urs_error(E_UR0104_URD_DDG_DESCR_NULL, URS_INTERR, 0);
	return E_DB_ERROR;
    }

    if (session == NULL)
    {
	urs_error(E_UR0107_URD_DDG_SESSION_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0180_INTFC_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    status = get_intfc(ursm, session, intfc, ursb);
    if (DB_FAILURE_MACRO(status))
	SET_URSB_ERROR(ursb, E_UR0180_INTFC_LOOKUP, status)

    return status;
}

/*{
** Name: get_method Get the method information
**
** Description:
**	Get the method information for the next method defined.
**	This information includes:
**	    fas_method_name	Name of the method
**
** Inputs:
**	ursm		    	The URS Manager Control Block
**	session			The data dictionary session block
**	method			Address of an FAS method block
**
** Outputs:
**	method			A filled-in FAS method block
**
**  Returns:
**	E_DB_OK 	    	Method info returned OK
**	E_DB_INFO	    	No more methods defined
**	E_DB_ERROR	    	Error getting methods definition
**			    	(ursb_error field has more info)
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  28-mar-1998 (wonst02)
**	Original.
*/
DB_STATUS
get_method(URS_MGR_CB	*ursm,
	  DDG_SESSION	*session,
	  FAS_METHOD	*method,
	  URSB		*ursb)
{
    bool	exist = FALSE;
    i4		*srvr_id = NULL;
    i4		*appl_id = NULL;
    i4		*intfc_id = NULL;
    i4		*method_id = NULL;
    char	*method_name = NULL;
    char	*method_path = NULL;
    char	name[sizeof method->fas_method_name + 1];
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (method->fas_method_id == 0)
	err = DDGSelectAll(session, FAS_OBJ_METHOD);

    if (err == GSTAT_OK)
	err = DDGNext(session, &exist);

    if (err == GSTAT_OK && exist)
    {
	err = DDGProperties(session,
			    "%d%d%d%d%s",
			    &srvr_id,
			    &appl_id,
			    &intfc_id,
			    &method_id,
			    &method_name);
    }
    if (err == GSTAT_OK && exist)
    {
    	method->fas_method_appl_id = *appl_id;
	method->fas_method_intfc_id = *intfc_id;
	method->fas_method_id = *method_id;
	method->fas_method_flags = 0;
	STcopy(method_name, name);
	STmove(name, ' ', sizeof method->fas_method_name.db_name,
	       method->fas_method_name.db_name);

	status = E_DB_OK;
    }
    else
    {
	DDGClose(session);
	if (err != GSTAT_OK)
	{
	    i4		len = 0;
	    char	*ptr = "";

	    if (err->info)
	    {
	    	len = STlength(err->info);
		ptr = err->info;
	    }
	    urs_error(err->number, URS_INTERR, 0);
	    urs_error(E_UR0353_DDG_SELECT_ERROR, URS_INTERR, 1, len, ptr);
	    DDFStatusFree(TRC_EVERYTIME, &err);
	    status = E_DB_ERROR;
	}
	else
	    status = E_DB_INFO;
    }

    return status;
}

/*{
** Name: urd_get_method - Get the definition of the method
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  28-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_get_method(URS_MGR_CB   *ursm,
	       FAS_METHOD   *method,
	       URSB	    *ursb)
{
    DDG_SESSION *session = (DDG_SESSION *)ursm->ursm_ddg_session;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (ursm->ursm_ddg_descr == NULL)
    {
	urs_error(E_UR0104_URD_DDG_DESCR_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0150_METHOD_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    if (session == NULL)
    {
	urs_error(E_UR0107_URD_DDG_SESSION_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0150_METHOD_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    status = get_method(ursm, session, method, ursb);
    if (DB_FAILURE_MACRO(status))
	SET_URSB_ERROR(ursb, E_UR0150_METHOD_LOOKUP, status)

    return status;
}

/*{
** Name: get_arg    Get the argument information
**
** Description:
**	Get the argument information for the next argument defined.
**  This information includes:
**	    fas_arg_name	Name of the argument
**
** Inputs:
**	ursm		    The URS Manager Control Block
**  session		The data dictionary session block
**  arg 	    Address of an FAS arg block
**
** Outputs:
**  arg 	    A filled-in FAS argument block
**
**  Returns:
**	E_DB_OK 	    Method info returned OK
**	E_DB_INFO	    No more args defined
**	E_DB_ERROR	    Error getting args definition
**			    (ursb_error field has more info)
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  28-mar-1998 (wonst02)
**	Original.
*/
DB_STATUS
get_arg(URS_MGR_CB  *ursm,
	DDG_SESSION *session,
	FAS_ARG     *arg)
{
    bool	exist = FALSE;
    i4		*srvr_id = NULL;
    i4		*appl_id = NULL;
    i4		*intfc_id = NULL;
    i4		*method_id = NULL;
    i4		*arg_id = NULL;
    char	*arg_name = NULL;
    i4		*arg_type = NULL;
    i4		*arg_length = NULL;
    i2		*arg_datatype = NULL;
    i2		*arg_prec = NULL;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (arg->fas_arg_id == 0)
	err = DDGSelectAll(session, FAS_OBJ_ARG);

    if (err == GSTAT_OK)
	err = DDGNext(session, &exist);

    if (err == GSTAT_OK && exist)
    {
	err = DDGProperties(session,
			    "%d%d%d%d%d%s%d%d%d%d",
			    &srvr_id,
			    &appl_id,
			    &intfc_id,
			    &method_id,
			    &arg_id,
			    &arg_name,
			    &arg_type,
			    &arg_datatype,
			    &arg_length,
			    &arg_prec);
    }
    if (err == GSTAT_OK && exist)
    {
	arg->fas_arg_id = *arg_id;
	arg->fas_arg_method_id = *method_id;
	arg->fas_arg_intfc_id = *intfc_id;
	arg->fas_arg_appl_id = *appl_id;
	arg->fas_arg_flags = 0;
	STmove(arg_name, ' ', sizeof arg->fas_arg_name.db_name,
	       arg->fas_arg_name.db_name);
	arg->fas_arg_type = *arg_type;
	arg->fas_arg_data.db_length = *arg_length;
	arg->fas_arg_data.db_datatype = *arg_datatype;
	arg->fas_arg_data.db_prec = *arg_prec;

	status = E_DB_OK;
    }
    else
    {
	DDGClose(session);
	if (err != GSTAT_OK)
	{
	    i4		len = 0;
	    char	*ptr = "";

	    if (err->info)
	    {
	    	len = STlength(err->info);
		ptr = err->info;
	    }
	    urs_error(err->number, URS_INTERR, 0);
	    urs_error(E_UR0353_DDG_SELECT_ERROR, URS_INTERR, 1, len, ptr);
	    DDFStatusFree(TRC_EVERYTIME, &err);
	    status = E_DB_ERROR;
	}
	else
	    status = E_DB_INFO;
    }

    return status;
}

/*{
** Name: urd_get_arg	- Get the definition of the arg
**
** Description:
**
** Inputs:
**
** Outputs:
**
**  Returns:
**
**  Exceptions:
**
** Side Effects:
**	none
**
** History:
**  28-mar-1998 (wonst02)
**	Original
*/
DB_STATUS
urd_get_arg(URS_MGR_CB	*ursm,
	    FAS_ARG	*arg,
	    URSB	*ursb)
{
    DDG_SESSION *session = (DDG_SESSION *)ursm->ursm_ddg_session;
    GSTATUS	err = GSTAT_OK;
    DB_STATUS	status = E_DB_OK;

    if (ursm->ursm_ddg_descr == NULL)
    {
	urs_error(E_UR0104_URD_DDG_DESCR_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0160_ARG_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    if (session == NULL)
    {
	urs_error(E_UR0107_URD_DDG_SESSION_NULL, URS_INTERR, 0);
	SET_URSB_ERROR(ursb, E_UR0160_ARG_LOOKUP, E_DB_ERROR)
	return E_DB_ERROR;
    }

    status = get_arg(ursm, session, arg);
    if (DB_FAILURE_MACRO(status))
	SET_URSB_ERROR(ursb, E_UR0160_ARG_LOOKUP, status)

    return status;
}

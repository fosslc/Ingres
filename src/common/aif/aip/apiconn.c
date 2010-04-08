/*
** Copyright (c) 2004, 2007 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cv.h>
# include <er.h>
# include <gc.h>
# include <id.h>
# include <nm.h>
# include <pc.h>
# include <st.h>
# include <sl.h>
# include <te.h>
# include <tmtz.h>

# include <iicommon.h>
# include <adf.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apitname.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiconn.c
**
** Description:
**	This file contains the IIapi_connect() function interface.
**
**      IIapi_connect  API connect function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of connection handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	14-Nov-94 (gordy)
**	    Allow connection handle to be created in IIapi_setConnectParam().
**	18-Nov-94 (gordy)
**	    Added co_tranhandle to connection parameter block.
**	19-Jan-95 (gordy)
**	    Support both II_TIMEZONE and II_TIMEZONE_NAME.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	24-Mar-95 (gordy)
**	    Add tmtz.h include file.
**	10-May-95 (gordy)
**	    Changed return value of IIapi_setConnParm().  Don't set
**	    GCA_EUSER when passed a username, let GCA take care of
**	    that through the request parms.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Pass transaction handle when reconnecting to a distributed xact.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	13-Mar-97 (gordy)
**	    Input handle may be environment handle.  Added connection
**	    types for SQL and Name Server.
**	17-Mar-97 (gordy)
**	    Use global saved timezone name rather than retrieving again.
**	 3-Jul-97 (gordy)
**	    Make sure state machines are initialized.
**	03-Feb-99 (rajus01)
**	    Renamed IIAPI_UP_CAN_PROMPT to IIAPI_UP_PROMPT_FUNC.
**	 7-Dec-99 (gordy)
**	    Make sure the ADF connection parameters are sent including
**	    the value for II_DATE_CENTURY_BOUNDARY.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	15-Mar-04 (gordy)
**	    Added env/conn parameter for I8 width.
**	22-Oct-04 (gordy)
**	    If connection handle is passed in, make sure to use the
**	    associated environment handle rather than the default.
**	11-Jan-2005 (wansh01)
**	    INGNET 161 bug # 113737
**	    For vnodeless connection:
**               @hostname,protocol,installid[uid,pwd]::dbname
**          password is displayed in monitor applications.
**	    A new field ch_target_display is added to the connHndl.
**	    This contains target info with masked pwd. 
**      14-May-2007 (gefei01) Bug 116825
**          In setConnectParams(), replace the call to IDname()
**	    with GCusername() on Windows.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	 8-Aug-07 (gordy)
**	    Add support for date datatype alias.
**    08-Apr-2008 (rajus01) SD issue 126704 Bug 120317
**	    JDBC server receives program exception when longer database name 
**	    is provided in the JDBC url. The SEGV is raised by the limited 
**	    buffer size provided to build the Ingres client info for display 
**	    purpose in the connection handle.
**	    Added IIAPI_TARGET_MAX_LEN.
*/


static IIAPI_STATUS setConnectParams( IIAPI_CONNHNDL * connHndl, ADF_CB * );
static IIAPI_STATUS setEnvParams( IIAPI_ENVHNDL * envHndl, 
				  IIAPI_CONNHNDL *connHndl );

/*
**  To avoid buffer overrun while building the client info
**  the following define truncates the target to this limit
**  when longer targets are provided in the JDBC url.
**  The target could be containing vnode(host,port,proto), login
**  (username/password), database name, and server class )
**  This info is held in the connection handle for display purposes.
**
*/
# define IIAPI_TARGET_MAX_LEN 256

/*
** Name: IIapi_connect
**
** Description:
**	This function provide an interface for the frontend application
**      to connect to a DBMS server.  Please refer to the API user
**      specification for function description.
**
**      connParm  input and output parameters for IIapi_connect()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of connection handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	14-Nov-94 (gordy)
**	    Allow connection handle to be created in IIapi_setConnectParam().
**	18-Nov-94 (gordy)
**	    Added co_tranhandle to connection parameter block for
**	    restarting distributed transactions.
**	19-Jan-95 (gordy)
**	    Support both II_TIMEZONE and II_TIMEZONE_NAME.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	10-May-95 (gordy)
**	    Changed return value of IIapi_setConnParm().  Don't set
**	    GCA_EUSER when passed a username, let GCA take care of
**	    that through the request parms.
**	14-Jun-95 (gordy)
**	    Pass transaction handle when reconnecting to a distributed xact.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      13-Dec-95 (chash01)
**          change to qlang=DB_ODQL to test ODB2
**	13-Mar-97 (gordy)
**	    Input handle may be environment handle.  Added connection
**	    types for SQL and Name Server.
**	17-Mar-97 (gordy)
**	    Use global saved timezone name rather than retrieving again.
**	 3-Jul-97 (gordy)
**	    Make sure state machines are initialized.
**	15-Jul-98 (gordy)
**	    Send client info to server.
**	 7-Dec-99 (gordy)
**	    The environment params should be processed before the required
**	    connection parameters to get the correct precedence.
**	22-Oct-04 (gordy)
**	    If connection handle is passed in, make sure to use the
**	    associated environment handle rather than the default.
**	11-Jan-2005 (wansh01)
**	    INGNET 161 bug # 113737
**	    For vnodeless connection:
**               @hostname,protocol,installid[uid,pwd]::dbname
**          password is displayed in monitor applications.
**	    A new field ch_target_display is added to the connHndl.
**	    This contains target info with supressed login info(user,pwd). 
**	    and it will be used for trace message and MO.  
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	08-Apr-2008 (rajus01) SD issue 126704 Bug 120317
**	    JDBC server receives program exception when longer database name is
**	    provided in the JDBC url. The SEGV is raised by the limited buffer
**	    size provided to build the Ingres client info. 
**	    Also fixed typo error.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_connect( IIAPI_CONNPARM II_FAR *connParm )
{
    IIAPI_ENVHNDL	*envHndl = NULL;
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_STATUS	api_status;
    STATUS		status;
    II_BOOL		new_handle = FALSE;
    char  		*pleft, *pright, *pcomma;
    int 		len;	

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_connect: connect to DBMS Server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! connParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: null connect parameters\n" );
	return;
    }
    
    connParm->co_genParm.gp_completed = FALSE;
    connParm->co_genParm.gp_status = IIAPI_ST_SUCCESS;
    connParm->co_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)connParm->co_connHandle;
    connParm->co_connHandle = NULL;
    tranHndl = (IIAPI_TRANHNDL *)connParm->co_tranHandle;
    connParm->co_tranHandle = NULL;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: API is not initialized\n" );
	IIapi_appCallback( &connParm->co_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }

    if ( connHndl )
	if ( IIapi_isEnvHndl( (IIAPI_ENVHNDL *)connHndl ) )
	{
	    envHndl = (IIAPI_ENVHNDL *)connHndl;
	    connHndl = NULL;
	}
	else  if ( ! IIapi_isConnHndl( connHndl )  ||
		   IIAPI_STALE_HANDLE( connHndl ) )
	{
	    IIAPI_TRACE(IIAPI_TR_ERROR)
		( "IIapi_connect: invalid connection handle\n" );
	    IIapi_appCallback( &connParm->co_genParm, NULL, 
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}
    
    if ( tranHndl  &&  ! IIapi_isTranName( (IIAPI_TRANNAME *)tranHndl ) )
    {
	IIAPI_TRACE(IIAPI_TR_ERROR)
	    ( "IIapi_connect: invalid transaction name handle\n" );
	IIapi_appCallback( &connParm->co_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
        ( "IIapi_connect: envHndl = %p, connHndl = %p, tranHndl= %p\n",
	  envHndl, connHndl, tranHndl );
    
    if ( envHndl )  
	IIapi_clearAllErrors( (IIAPI_HNDL *)envHndl );
    else  if ( connHndl )
	envHndl = IIapi_getEnvHndl( (IIAPI_HNDL *)connHndl );
    else
	envHndl = IIapi_defaultEnvHndl();

    if ( connHndl ) IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );

    /*
    ** Create new connection handle if needed.
    */
    if ( ! connHndl )
	if ( (connHndl = IIapi_createConnHndl( envHndl )) )
	    new_handle = TRUE;
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_connect: createConnHndl failed\n" );
	    api_status = IIAPI_ST_OUT_OF_MEMORY;
	    goto hndlAllocFail;
	}

    /*
    ** Adjust the connection type at Version 2 and above.
    */
    if ( envHndl->en_version >= IIAPI_VERSION_2 )
	switch( connParm->co_type )
	{
	    case IIAPI_CT_SQL :	connHndl->ch_type = IIAPI_SMT_SQL;	break;
	    case IIAPI_CT_NS :	connHndl->ch_type = IIAPI_SMT_NS;	break;

	    default :
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_connect: invalid connection type %d\n",
		      connParm->co_type );

		api_status = IIAPI_ST_FAILURE;
		goto hndlAllocFail;
	}

    /*
    ** Set the correct state machine (initialize if necessary).
    */
    if ( ! IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_CONN ]  &&
	 (api_status = IIapi_sm_init(connHndl->ch_type)) != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: error initializing state machines\n" );
	goto hndlAllocFail;
    }

    connHndl->ch_header.hd_smi.smi_sm =
			IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_CONN ];

    /*
    ** Save connection info.
    **
    ** Allow room to manipulate the partner
    ** target ID if needed.
    */
    connHndl->ch_target = (char *)
	MEreqmem( 0, ((connParm->co_target  &&  *connParm->co_target) 
		      ? STlength(connParm->co_target) : 0) + IIAPI_CONN_EXTRA,
		  TRUE, &status );

    if ( ! connHndl->ch_target )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: target allocation error\n" );

	api_status = IIAPI_ST_OUT_OF_MEMORY;
	goto hndlAllocFail;
    }
    else  if (connParm->co_target )
	STcopy( connParm->co_target, connHndl->ch_target );

    if ( ! (connHndl->ch_target_display = STalloc( connHndl->ch_target )))
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	( "IIapi_connect: ch_target_display allocation error\n" );

	api_status = IIAPI_ST_OUT_OF_MEMORY;
	goto targetAllocFail;
    }	
	
    if (( connHndl->ch_target[0] == '@' )  
	&& (pleft = STchr(connHndl->ch_target,'[')) 
	&& (pright = STchr(connHndl->ch_target,']'))
	&& ((pleft) && (pcomma= STchr(pleft,','))))
    {
	    len = pcomma - connHndl->ch_target +1 ;
	    STncpy(connHndl->ch_target_display,connHndl->ch_target, len);
	    connHndl->ch_target_display[len]='\0';
	    STcat(connHndl->ch_target_display,"****"); 
	    STcat(connHndl->ch_target_display,pright); 
    }
    else 
	STcpy(connHndl->ch_target_display,connHndl->ch_target);

    if ( connHndl->ch_target_display )
	IIAPI_TRACE( IIAPI_TR_INFO ) 
	    ( "IIapi_connect: target = %s\n", connHndl->ch_target_display );

    if ( connParm->co_username  &&  *connParm->co_username  &&
	 ! (connHndl->ch_username = STalloc( connParm->co_username )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: username allocation error\n" );

	api_status = IIAPI_ST_OUT_OF_MEMORY;
	goto userAllocFail;
    }

    if ( connParm->co_password  &&  *connParm->co_password  &&
	 ! (connHndl->ch_password = STalloc( connParm->co_password )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connect: password allocation error\n" );

	api_status = IIAPI_ST_OUT_OF_MEMORY;
	goto passAllocFail;
    }

    if ( tranHndl )
    {
	if ( connHndl->ch_type == IIAPI_SMT_NS )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		("IIapi_connect: can't restart transaction for Name Server\n");

	    api_status = IIAPI_ST_INVALID_HANDLE;
	    goto setParamFailed;
	}

	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_RESTART, tranHndl );
	if ( api_status != IIAPI_ST_SUCCESS )  goto setParamFailed;

	if ( ! ( tranHndl = (II_PTR)IIapi_createTranHndl( 
			    (IIAPI_TRANNAME *)tranHndl, connHndl ) ) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_connect: can't create transaction handle\n" );

	    api_status = IIAPI_ST_OUT_OF_MEMORY;
	    goto setParamFailed;
	}
    }

    /* 
    ** Set connection parameters from the environment handle
    */
    api_status = setEnvParams( envHndl, connHndl );
    if ( api_status != IIAPI_ST_SUCCESS )  goto setParamFailed;

    /*
    ** Set the required connection parameters.
    */
    api_status = setConnectParams( connHndl, (ADF_CB *)envHndl->en_adf_cb );
    if ( api_status != IIAPI_ST_SUCCESS )  goto setParamFailed;

    connParm->co_connHandle = (II_PTR)connHndl;
    connParm->co_tranHandle = (II_PTR)tranHndl;

    /*
    ** When reconnecting to a distributed transaction,
    ** we need to make sure the transaction state manager
    ** gets called.  We pass the transaction handle in 
    ** this case (the connection handle can be obtained
    ** from the transaction handle to drive the connection
    ** state manager).
    */
    IIapi_uiDispatch( IIAPI_EV_CONNECT_FUNC, 
		      tranHndl ? (IIAPI_HNDL *)tranHndl 
			       : (IIAPI_HNDL *)connHndl,
		      (II_PTR)connParm );

    return;

  passAllocFail:

    if ( connHndl->ch_username )
    {
	MEfree( (PTR)connHndl->ch_username );
	connHndl->ch_username = NULL;
    }

  userAllocFail:
    if ( connHndl->ch_target_display )
    {
	MEfree( (PTR)connHndl->ch_target_display );
	connHndl->ch_target_display = NULL;
    }

  targetAllocFail:
    if ( connHndl->ch_target )
    {
	MEfree( (PTR)connHndl->ch_target );
	connHndl->ch_target = NULL;
    }

  setParamFailed:

    if ( connHndl->ch_password )
    {
	MEfree( (PTR)connHndl->ch_password );
	connHndl->ch_password = NULL;
    }

  hndlAllocFail:

    IIapi_appCallback( &connParm->co_genParm, 
		       (IIAPI_HNDL *)connHndl, api_status );
    if ( new_handle )  IIapi_deleteConnHndl( connHndl);

    return;
}

/*
** Name: setConnectParams
**
** Description:
**	Send required connection parameters which were not set by
**	the application either as connection or environment params.
**
**      connHndl  	Connection Handle
**	adf_cb		ADF control block.
**
** Return value:
**      IIAPI_STATUS    IIAPI_ST_SUCCESS
**                      IIAPI_ST_OUT_OF_MEMORY
**                      IIAPI_ST_FAILURE - bad parameter
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-Aug-98 (rajus01)
**          Created.
**	14-Oct-98 (rajus01)
**	    Added support for GCA_PROMPT. Changed sltype to be II_LONG.
**	 7-Dec-99 (gordy)
**	    Make sure the ADF connection parameters are sent including
**	    the value for II_DATE_CENTURY_BOUNDARY.
**	11-Jan-2005 (wansh01)
**	    INGNET 161 bug # 113737
**	    ch_target_display is used for "conn=" client info. 
**      14-May-2007 (gefei01) Bug 116825
**          Replace the call to IDname() with GCusername() on Windows.
**          On Unix/Linux the real user ID is more appropriate,
**          therefore IDname() is stilled called on Unix/Linux.
**          The change for Window should revert back to IDname()
**          when caching is removed from the Windows version of IDname().
**          The performance impact of removing the caching should be studied.
**	 8-Aug-07 (gordy)
**	    Add support for date datatype alias.
**	08-Apr-2008 (rajus01) SD issue 126704 Bug 120317
**	    A reasonable length (2*IIAPI_TARGET_MAX_LEN) for 'buff' is
**	    chosen and a logic is added to prevent buffer overrun that resulted
**	    in SEGV in JDBC server when longer target name was provided in
**	    JDBC url.
*/

static IIAPI_STATUS
setConnectParams( IIAPI_CONNHNDL * connHndl, ADF_CB *adf_cb )
{
    IIAPI_STATUS	api_status = IIAPI_ST_SUCCESS;

    if ( connHndl->ch_type != IIAPI_SMT_NS )
    {
	II_ULONG sp_mask1 = 0;
	II_ULONG sp_mask2 = 0;
	II_LONG	 qlang = DB_SQL;

	if ( connHndl->ch_sessionParm )
	{
	    sp_mask1 = connHndl->ch_sessionParm->sp_mask1;
	    sp_mask2 = connHndl->ch_sessionParm->sp_mask2;
	}

	api_status = IIapi_setConnParm( connHndl, 
					IIAPI_CP_QUERY_LANGUAGE, (PTR)&qlang );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );

	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_DATABASE, 
				        connHndl->ch_target
					? connHndl->ch_target : "" );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );

	if ( ! (sp_mask1 & IIAPI_SP_TIMEZONE)  &&
	     ! (sp_mask1 & IIAPI_SP_TIMEZONE_NAME) &&
	     IIapi_static->api_timezone )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_TIMEZONE,
					    IIapi_static->api_timezone );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask1 & IIAPI_SP_DATE_FORMAT)  && 
	     adf_cb->adf_dfmt != DB_US_DFMT )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_DATE_FORMAT,
					    &adf_cb->adf_dfmt );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask2 & IIAPI_SP_YEAR_CUTOFF )  &&
	     adf_cb->adf_year_cutoff != TM_DEF_YEAR_CUTOFF )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_CENTURY_BOUNDARY,
					    &adf_cb->adf_year_cutoff );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask1 & IIAPI_SP_MSIGN)  &&
	     adf_cb->adf_mfmt.db_mny_sym[0] != '$' )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MONEY_SIGN,
					    adf_cb->adf_mfmt.db_mny_sym );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask1 & IIAPI_SP_MLORT)  && 
	     adf_cb->adf_mfmt.db_mny_lort != DB_LEAD_MONY )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MONEY_LORT,
					    &adf_cb->adf_mfmt.db_mny_lort );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask1 & IIAPI_SP_MPREC)  &&
	     adf_cb->adf_mfmt.db_mny_prec != 2 )
	{
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MONEY_PRECISION,
					    &adf_cb->adf_mfmt.db_mny_prec );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask1 & IIAPI_SP_DECIMAL)   &&
	     adf_cb->adf_decimal.db_decimal != '.' )
	{
	    char decimal[ 2 ];

	    decimal[ 0 ] = adf_cb->adf_decimal.db_decimal; 
	    decimal[ 1 ] = '\0'; 
	    api_status = IIapi_setConnParm( connHndl, IIAPI_CP_DECIMAL_CHAR,
					    decimal );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	if ( ! (sp_mask2 & IIAPI_SP_DATE_ALIAS) )
	{
	    char *alias;

	    if ( IIapi_static->api_date_alias == AD_DATE_TYPE_ALIAS_ANSI )
	    	alias = IIAPI_CPV_ANSIDATE;
	    else
	    	alias = IIAPI_CPV_INGDATE;

	    api_status = IIapi_setConnParm( connHndl, 
	    				    IIAPI_CP_DATE_ALIAS, alias );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}

	/*
	** Include Client info along with other WITH clause data.
	*/
	{
	    char	buff[ IIAPI_TARGET_MAX_LEN * 2 ];
	    char	name[ GL_MAXNAME + 1 ];
	    char	*ptr = name;
	    PID		pid;

#ifdef NT_GENERIC
	    GCusername(ptr, sizeof(name));
#else
	    IDname( &ptr );
#endif	    
	    STzapblank( name, name );
	    STpolycat( 3, ERx("user='"), name, ERx("',host='"), buff );
	    GChostname( name, sizeof( name ) );
	    STcat( buff, name );
	    STcat( buff, ERx("',tty='") );
	    TEname( name );
	    STcat( buff, name );
	    STcat( buff, ERx("',pid=") );
	    PCpid( &pid );
	    CVna( pid, name );
	    STcat( buff, name );
	    STcat( buff, ERx(",conn='") );
	    if( connHndl->ch_target_display )
 	    {
 		i2 buff_len = STlength(buff); 
 		if( STlength(connHndl->ch_target_display) >= sizeof(buff)-buff_len )
 		{
 		    i2 copy_len = sizeof(buff)-buff_len-1;
 		    STncpy( buff+buff_len, connHndl->ch_target_display, copy_len);
 		    buff[buff_len+copy_len]=EOS;
 		}
 		else
 		    STcat( buff, connHndl->ch_target_display );
 	    }	
	    STcat( buff, ERx("'") );

	    api_status = IIapi_setConnParm( connHndl, 
					    IIAPI_CP_GATEWAY_PARM, buff );
	    if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
	}
    }

    return ( api_status );
}

/*
** Name: setEnvParams
**
** Description:
**	Send the connection parameters which were set as environment
**	params and not overriden by the application.
**
**      envHndl  	Environment Handle
**      connHndl  	Connection Handle
**
** Return value:
**      IIAPI_STATUS    IIAPI_ST_SUCCESS
**                      IIAPI_ST_OUT_OF_MEMORY
**                      IIAPI_ST_FAILURE - bad parameter
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-Aug-98 (rajus01)
**          Created.
**	25-Sep-98 (rajus01)
**	    Don't set environment parameters for Name server connections.
**	    Removed the trace statements.
**	 7-Dec-99 (gordy)
**	    Moved all required parameter sending to setConnectParams.
**	15-Mar-04 (gordy)
**	    Added env/conn parameter for I8 width.
**	 8-Aug-07 (gordy)
**	    Added env/conn parameter for date alias.
*/

static IIAPI_STATUS
setEnvParams( IIAPI_ENVHNDL *envHndl, IIAPI_CONNHNDL *connHndl )
{
    IIAPI_USRPARM	*usrParm = &envHndl->en_usrParm;
    ADF_CB		*adf_cb = ( ADF_CB * )envHndl->en_adf_cb;
    IIAPI_STATUS	api_status = IIAPI_ST_SUCCESS;
    II_ULONG            sp_mask1 = 0;
    II_ULONG            sp_mask2 = 0;

    if ( connHndl->ch_type == IIAPI_SMT_NS )  return( api_status );

    if ( connHndl->ch_sessionParm )
    {
	sp_mask1 = connHndl->ch_sessionParm->sp_mask1;
	sp_mask2 = connHndl->ch_sessionParm->sp_mask2;
    }

    if( ! (sp_mask1 & IIAPI_SP_CWIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_CWIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_CHAR_WIDTH, 
				       &adf_cb->adf_outarg.ad_c0width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }
		
    if( ! (sp_mask1 & IIAPI_SP_TWIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_TWIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_TXT_WIDTH,
					&adf_cb->adf_outarg.ad_t0width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_I1WIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_I1WIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_INT1_WIDTH,
				        &adf_cb->adf_outarg.ad_i1width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_I2WIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_I2WIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_INT2_WIDTH,
					&adf_cb->adf_outarg.ad_i2width);
    }

    if( ! (sp_mask1 & IIAPI_SP_I4WIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_I4WIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, 
					IIAPI_CP_INT4_WIDTH,
					&adf_cb->adf_outarg.ad_i4width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask2 & IIAPI_SP_I8WIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_I8WIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, 
					IIAPI_CP_INT8_WIDTH,
					&adf_cb->adf_outarg.ad_i8width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_F4WIDTH) && 
    	(usrParm->up_mask1 & IIAPI_UP_F4WIDTH ) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_FLOAT4_WIDTH,
					&adf_cb->adf_outarg.ad_f4width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_F4PRECISION) &&
	(usrParm->up_mask1 & IIAPI_UP_F4PRECISION) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_FLOAT4_PRECISION,
					&adf_cb->adf_outarg.ad_f4prec);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_F8WIDTH) &&
	(usrParm->up_mask1 & IIAPI_UP_F8WIDTH) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_FLOAT8_WIDTH,
					&adf_cb->adf_outarg.ad_f8width);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_F8PRECISION) &&
	(usrParm->up_mask1 & IIAPI_UP_F8PRECISION) ) 
    {  
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_FLOAT8_PRECISION,
					&adf_cb->adf_outarg.ad_f8prec);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_F4STYLE) && 
	(usrParm->up_mask1 & IIAPI_UP_F4STYLE) ) 
    {
	char	f4style[ 2 ];

	f4style[0] = adf_cb->adf_outarg.ad_f4style;
	f4style[1] = '\0';
	api_status = IIapi_setConnParm(connHndl,IIAPI_CP_FLOAT4_STYLE,f4style);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if(	! (sp_mask1 & IIAPI_SP_F8STYLE) && 
        (usrParm->up_mask1 & IIAPI_UP_F8STYLE) ) 
    {
	char	f8style[ 2 ];

	f8style[0] = adf_cb->adf_outarg.ad_f8style;
	f8style[1] = '\0';
	api_status = IIapi_setConnParm(connHndl,IIAPI_CP_FLOAT8_STYLE,f8style);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_NLANGUAGE) &&
    	( (usrParm->up_mask1 & IIAPI_UP_NLANGUAGE) ||
	( adf_cb->adf_slang != 0 && adf_cb->adf_slang != 1 ) ) )
    {
	II_CHAR		langbuf[ER_MAX_LANGSTR];
	
	if( ERlangstr( adf_cb->adf_slang, langbuf ) != OK )
	   return (IIAPI_ST_FAILURE);

	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_NATIVE_LANG,
								langbuf );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_MPREC)  &&
    	 usrParm->up_mask1 & IIAPI_UP_MPREC )
    {
	api_status = IIapi_setConnParm( connHndl,
					IIAPI_CP_MONEY_PRECISION,
					&adf_cb->adf_mfmt.db_mny_prec );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if(	! (sp_mask1 & IIAPI_SP_DECFLOAT) && 
    	(usrParm->up_mask1 & IIAPI_UP_DECFLOAT) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_NUMERIC_TREATMENT,
					usrParm->up_decfloat );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_MSIGN)  &&
	 usrParm->up_mask1 & IIAPI_UP_MSIGN )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MONEY_SIGN,
					adf_cb->adf_mfmt.db_mny_sym );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_MLORT)  && 
	 usrParm->up_mask1 & IIAPI_UP_MLORT )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MONEY_LORT,
					&adf_cb->adf_mfmt.db_mny_lort );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_DECIMAL)   &&
	 usrParm->up_mask1 & IIAPI_UP_DECIMAL )
    {
	char decimal[ 2 ];

	decimal[ 0 ] = adf_cb->adf_decimal.db_decimal; 
	decimal[ 1 ] = '\0'; 
	api_status = IIapi_setConnParm(connHndl,IIAPI_CP_DECIMAL_CHAR,decimal);
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_TIMEZONE)  &&
	 ! (sp_mask1 & IIAPI_SP_TIMEZONE_NAME)  &&
    	 usrParm->up_mask1 & IIAPI_UP_TIMEZONE )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_TIMEZONE,
					usrParm->up_timezone );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask1 & IIAPI_SP_MATHEX) &&
 	(usrParm->up_mask1 & IIAPI_UP_MATHEX) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_MATH_EXCP,
					usrParm->up_mathex );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if( ! (sp_mask2 & IIAPI_SP_STRTRUNC ) &&
    	(usrParm->up_mask1 & IIAPI_UP_STRTRUNC) )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_STRING_TRUNC,
					usrParm->up_strtrunc );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask1 & IIAPI_SP_DATE_FORMAT)  && 
	 usrParm->up_mask1 & IIAPI_UP_DATE_FORMAT )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_DATE_FORMAT,
					&adf_cb->adf_dfmt );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask2 & IIAPI_SP_YEAR_CUTOFF )  &&
	 usrParm->up_mask1 & IIAPI_UP_CENTURY_BOUNDARY )
    {
	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_CENTURY_BOUNDARY,
					&adf_cb->adf_year_cutoff );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ( usrParm->up_mask1 & IIAPI_UP_PROMPT_FUNC ) && 
		usrParm->up_prompt_func != NULL )
    {
	II_LONG	can_prompt = GCA_ON;

	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_CAN_PROMPT,
					    &can_prompt );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    if ( ! (sp_mask2 & IIAPI_SP_DATE_ALIAS)  &&
         usrParm->up_mask1 & IIAPI_UP_DATE_ALIAS )
    {
    	api_status = IIapi_setConnParm( connHndl, IIAPI_CP_DATE_ALIAS,
					usrParm->up_date_alias );
	if ( api_status != IIAPI_ST_SUCCESS )  return( api_status );
    }

    return( api_status );
}

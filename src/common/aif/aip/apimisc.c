/*
** Copyright (c) 2007, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <pc.h>
# include <er.h>
# include <st.h>
# include <iicommon.h>
# include <adf.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apiadf.h>
# include <apimisc.h>
# include <apitrace.h>
# include <apidspth.h>

/*
** Name: apimisc.c
**
** Description:
**	This file defines the common functions shared by modules
**      internal of API-GCA.
**
**	IIapi_initAPI		Initialize the API.
**	IIapi_termAPI		Shutdown the API.
**	IIapi_appCallback	Application callback invocation
**	IIapi_thread		Returns thread specific data.
**	IIapi_isVAR		Is data type variable length.
**	IIapi_isBLOB		Is data type a BLOB.
**	IIapi_isUCS2		Is data type a Unicode type.
**	IIapi_isLocator		Is data type a Blob/Clob locator type.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      22-apr-94 (ctham)
**          deal with null input handle.  Both IIapi_isCliHndl and
**          IIapi_isSEHndl are updated.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	17-Jan-95 (gordy)
**	    Conditionalize code for non-BLOB support.
**	19-Jan-95 (gordy)
**	    Conditionalize support for timezones.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Feb-95 (gordy)
**	    Added ADF interface.
**	10-Feb-95 (gordy)
**	    Moved IIapi_appCallback().
**	16-Feb-95 (gordy)
**	    Added ID interface.
**	 8-Mar-95 (gordy)
**	    Added local status variables and access functions.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	23-Mar-95 (gordy)
**	    Removed MEadvise() call.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Remove DB_LTXT_TYPE as BLOB type.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	11-Oct-95 (gordy)
**	    Removed IIapi_initError() and IIapi_termError().
**	17-Jan-96 (gordy)
**	    Added environment handles.  Added global data structure.
**	    Implemented short functions as macros.
**	15-Mar-96 (gordy)
**	    Added IIapi_isVAR().
**	24-May-96 (gordy)
**	    Added IIapi_newMemTag() and IIapi_freeMemTag() for
**	    tagged memory support.
**	 2-Oct-96 (gordy)
**	    Save error handles if callback cannot be made and the
**	    handle is flagged for deletion.
**	11-Nov-96 (gordy)
**	    Converted IIapi_newMemTag() and IIapi_freeMemTag() to
**	    macros (its amazing what you'll find in the GL/CL).
**	14-Nov-96 (gordy)
**	    Added IIapi_thread() for supporting thread specific data.
**	    Moved serialization control data to thread-local-storage.
**	 7-Feb-97 (gordy)
**	    Default environment handle always exists and may be used
**	    by other than just version 1 initializers.
**	18-Feb-97 (gordy)
**	    Moved IIapi_createParmNMem() to this file for general access.
**	 3-Jul-97 (gordy)
**	    Initialize default state machines.
**	26-Aug-98 (rajus01)
**	    Added IIapi_snd_sl(). Set the default spoken language.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	 1-Mar-01 (gordy)
**	    Added support for Unicode datatypes.
**	 6-Jun-03 (gordy)
**	    Initialize API MIB info.
**	2-mar-2004 (gupsh01)
**	    Initialize colinit to indicate if unicode collation was read in.
**	24-Oct-06 (gordy)
**	    Added IIapi_isLocator().
**	16-Jan-2007 (lunbr01)  Bug 117496
**	    OpenAPI apps fail with status IIAPI_ST_OUT_OF_MEMORY (8)
**	    when II_SYSTEM not set; this status is misleading.
**	    Check for this error and output meaningful trace statement
**	    (if possible...trace variables set in environ, not symbol.tbl).
**	05-mar-2007 (abbjo03)
**	    Change 16-jan fix to use SYSTEM_LOCATION_VARIABLE.
**	25-Mar-10 (gordy)
**	    Enhanced GCA parameter block to support packed byte stream
**	    buffer without tagged memory.
*/


/*
** Name: IIapi_static
**
** Description:
**	Global API data.  This is allocated when API is first
**	initialized and freed at final shutdown.
**
** History:
**	19-Jan-96 (gordy)
**	    Created.
*/

GLOBALDEF IIAPI_STATIC	*IIapi_static = NULL;

#define IIapi_snd_sl( sl ) \
	   ( !sl || ! *sl ) ? TRUE : \
	   ((STcasecmp(sl, ERx("off") )  && \
	   STcasecmp(sl, ERx("no") )) ? TRUE : \
	   FALSE )



/*
** Name: IIapi_initAPI
**
** Description:
**	Initialize API.  Perform global initialization on first call.
**	Allocate and initialize an environment handle (if required).
**	For API version 1, there is a single environment handle used by
**	all initializers.  For subsequent versions, each initializer gets
**	their own environment handle.
**
** Input:
**	version		API version used by application.
**      timeout   	Timeout value to initialize GCA.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_ENVHNDL *	NULL if error, environment handle othewise.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	19-Jan-95 (gordy)
**	    Conditionalize support for timezones.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Feb-95 (gordy)
**	    Added ADF interface.
**	16-Feb-95 (gordy)
**	    Added ID interface.
**	 8-Mar-95 (gordy)
**	    Added local status variables.
**	23-Mar-95 (gordy)
**	    Removed MEadvise() call.  If we are in a user
**	    application, we should use the default allocator.
**	    If we are in an Ingres application, the application
**	    should call MEadvise().
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	11-Oct-95 (gordy)
**	    Removed IIapi_initError() and IIapi_termError().
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	 7-Feb-97 (gordy)
**	    Default environment handle always exists and may be used
**	    by other than just version 1 initializers.
**	 3-Jul-97 (gordy)
**	    Initialize default state machines.
**	 6-Jun-03 (gordy)
**	    Initialize API MIB info.
**	16-Jan-2007 (lunbr01)  Bug 117496
**	    OpenAPI apps fail with status IIAPI_ST_OUT_OF_MEMORY (8)
**	    when II_SYSTEM not set; this status is misleading.
**	    Check for this error and output meaningful trace statement
**	    (if possible...trace variables set in environ, not symbol.tbl).
*/

II_EXTERN IIAPI_ENVHNDL *
IIapi_initAPI( II_LONG version, II_LONG timeout )
{
    IIAPI_ENVHNDL	*envHndl;
    IIAPI_ENVHNDL	*defEnvHndl;
    II_BOOL		first_init = FALSE;
    STATUS		status;
    char		*env;

    if ( ! IIapi_static )
    {
	/*
	** Perform global initializations which are
	** environment independent.  
	*/
	first_init = TRUE;

	if ( ! ( IIapi_static = (IIAPI_STATIC *)
		 MEreqmem( 0, sizeof( IIAPI_STATIC ), TRUE, &status ) ) )
	    return( NULL );

	QUinit( &IIapi_static->api_env_q );

	if ( MUi_semaphore( &IIapi_static->api_semaphore ) != OK )
	    goto muiFail;

	if ( MEtls_create( &IIapi_static->api_thread ) != OK )
	    goto metFail;

	/*
	** Initialize sub-systems.
	*/
	IIAPI_INITTRACE();
	IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_initAPI: initializing API.\n" );

	/*
	** Make sure II_SYSTEM or similar is set to provide useful feedback
	** rather than just failing in one of following subsystems.
	*/
	NMgtAt( SYSTEM_LOCATION_VARIABLE, &env );
	if ( env == NULL || *env == EOS )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_initAPI: error - %s not set.\n",
		  SYSTEM_LOCATION_VARIABLE );
	    goto adfFail;
	}
	
	IIapi_init_mib();
	if ( ! IIapi_initADF() )	goto adfFail;
	if ( ! IIapi_initGCA(timeout) )	goto gcaFail;

	/* Initialize the unicode collation values */
	IIapi_static->api_unicol_init = FALSE;
	IIapi_static->api_ucode_ctbl = NULL; 
	IIapi_static->api_ucode_cvtbl = NULL;

	/*
	** Create the default environment.
	*/
	if ( ! (IIapi_static->api_env_default = 
				(PTR)IIapi_createEnvHndl( IIAPI_VERSION_1 )) )
	    goto defFail;

	/* Spoken Language to use */

    	if( ( status = ERlangcode( (char *)NULL, 
				   &IIapi_static->api_slang ) != OK) )
    	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_initAPI: error initializing lang 0x%x\n", status );

	    return( NULL );
    	}

	/*
	** The SQL state machines are used by default prior to
	** IIapi_connect() being called, so make sure they are
	** initialized.
	*/
	if ( IIapi_sm_init( IIAPI_SMT_SQL ) != IIAPI_ST_SUCCESS )
	    goto envFail;
    }

    /*
    ** Now do environment specific initialization.
    */
    defEnvHndl = IIapi_defaultEnvHndl();

    if ( version == IIAPI_VERSION_1 )
    {
	/*
	** Version 1 initializers share the default
	** environment.  Keep track of the number of
	** initializers so that IIapi_termAPI() can
	** determine when to do global shutdown.
	*/
	envHndl = defEnvHndl;

	MUp_semaphore( &defEnvHndl->en_semaphore );
	defEnvHndl->en_initCount++;
	MUv_semaphore( &defEnvHndl->en_semaphore );
    }
    else
    {
	/*
	** Create a new environment for the initializer.
	** These environments are saved on the global
	** environment queue.  The caller may also make
	** API calls using the default environment handle, 
	** so increment the default environment handle
	** initialization count.
	*/
	if ( (envHndl = IIapi_createEnvHndl( version )) )
	{
	    MUp_semaphore( &IIapi_static->api_semaphore );
	    QUinsert( (QUEUE *)envHndl, &IIapi_static->api_env_q );
	    MUv_semaphore( &IIapi_static->api_semaphore );

	    MUp_semaphore( &defEnvHndl->en_semaphore );
	    defEnvHndl->en_initCount++;
	    MUv_semaphore( &defEnvHndl->en_semaphore );
	}
	else
	{
	    /*
	    ** We may need to undo the global initialization.
	    ** If not, the NULL environment handle will simply
	    ** be returned.
	    */
	    if ( first_init )  goto envFail;
	}
    }

    return( envHndl );

  envFail:
    IIapi_deleteEnvHndl( (IIAPI_ENVHNDL *)IIapi_static->api_env_default );

  defFail:
    IIapi_termGCA();

  gcaFail:
    IIapi_termADF();

  adfFail:
    IIAPI_TERMTRACE();
    MEtls_destroy( &IIapi_static->api_thread, NULL );

  metFail:
    MUr_semaphore( &IIapi_static->api_semaphore );

  muiFail:
    MEfree( (PTR)IIapi_static );
    IIapi_static = NULL;

    return( NULL );
}




/*
** Name: IIapi_termAPI
**
** Description:
**	This function terminates the API.  If no environment handle 
**	is provided, the default environment handle will be used.  
**	Environment handles are freed when no longer active.  Global 
**	API termination occurs when the last environment handle is 
**	freed.
**
**	NOTE: this routine must be called twice for API users at
**	version 2 or greater: once for the version 2 environment
**	handle and once for the version 1 default environment
**	handle.  This corresponds to the required IIapi_releaseEnv()
**	and IIapi_terminate() calls.
**
** Input:
**	envHndl		Environment handle, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if last term, FALSE if more terms required.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	16-Feb-95 (gordy)
**	    Added ID interface.
**	 8-Mar-95 (gordy)
**	    Added local status variables.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	11-Oct-95 (gordy)
**	    Removed IIapi_initError() and IIapi_termError().
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	24-May-96 (gordy)
**	    Clean up tagged memory.
**	11-Nov-96 (gordy)
**	    Converted IIapi_newMemTag() and IIapi_freeMemTag() to
**	    macros (its amazing what you'll find in the GL/CL).
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	 7-Feb-97 (gordy)
**	    Default environment handle always exists and may be used
**	    by other than just version 1 initializers.  Global shutdown
**	    occurs when all environment handles are freed and all
**	    level 1 terminations completed.
*/

II_EXTERN II_BOOL
IIapi_termAPI( IIAPI_ENVHNDL *envHndl )
{
    IIAPI_ENVHNDL	*defEnvHndl;
    II_BOOL		last_term = FALSE;

    if ( ! IIapi_static )  return( TRUE );

    defEnvHndl = IIapi_defaultEnvHndl();

    if ( envHndl )  
    {
	/*
	** Free the environment handle.
	*/
	MUp_semaphore( &IIapi_static->api_semaphore );
	QUremove( (QUEUE *)envHndl );
	MUv_semaphore( &IIapi_static->api_semaphore );

	IIapi_deleteEnvHndl( envHndl );
    }
    else
    {
	/*
	** Decrement the usage counter in 
	** the default environment handle.
	*/
	MUp_semaphore( &defEnvHndl->en_semaphore );
	defEnvHndl->en_initCount = max( 0, defEnvHndl->en_initCount - 1 );
	MUv_semaphore( &defEnvHndl->en_semaphore );
    }

    /*
    ** Shutdown API when all version 1 initializers have terminated
    ** and there are no active version 2 environments.
    */
    if ( ! defEnvHndl->en_initCount  &&
         IIapi_isQueEmpty( &IIapi_static->api_env_q ) )
    {
	QUEUE		*q;

	IIAPI_TRACE( IIAPI_TR_TRACE )
	    ( "IIapi_termAPI: shutting down API completely.\n" );

	/*
	** Free the default environment.
	*/
	IIapi_deleteEnvHndl( (IIAPI_ENVHNDL *)IIapi_static->api_env_default );
	IIapi_static->api_env_default = NULL;

	/*
	** Shutdown sub-systems.
	*/
	IIapi_termGCA();
	IIapi_termADF();

	IIAPI_TRACE( IIAPI_TR_INFO )( "IIapi_termAPI: API shutdown.\n" );
	IIAPI_TERMTRACE();

	/*
	** Free the remaining global resources.
	*/
	if (IIapi_static->api_ucode_ctbl)
	  MEfree (IIapi_static->api_ucode_ctbl);
	if (IIapi_static->api_ucode_cvtbl)
	  MEfree (IIapi_static->api_ucode_cvtbl);

	MEtls_destroy( &IIapi_static->api_thread, MEfree );
	MUr_semaphore( &IIapi_static->api_semaphore );
	MEfree( (PTR)IIapi_static );
	IIapi_static = NULL;
	last_term = TRUE;
    }

    return( last_term );
}


/*
** Name: IIapi_appCallback
**
** Description:
**	This function invoke the appropriate callback specified
**     by the frontend application.
**
**     genParm      parameter block of the API function for the callback
**     handle       connection, transaction, statement or event handle
**     status       status of the API function
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      21-apr-94 (ctham)
**          set gp_errorInfoAvail flag when there are additional error
**          information available.  IIapi_appCallback() has changed
**          in its interface to deal with this automatically.
**	 31-may-94 (ctham)
**	     use correct parameters in trace message.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	 2-Oct-96 (gordy)
**	    Save error handles if callback cannot be made and the
**	    handle is flagged for deletion.
*/

II_EXTERN II_VOID
IIapi_appCallback
(
    IIAPI_GENPARM	*genParm,
    IIAPI_HNDL		*handle,
    IIAPI_STATUS	status
)
{
    IIAPI_STATUS	err_status;

    if ( handle )
	if ( ( err_status = IIapi_errorStatus( handle ) ) != IIAPI_ST_SUCCESS )
	    status = IIAPI_WORST_STATUS( status, err_status );
	else
	{
	    /*
	    ** There are no errors on the handle, don't 
	    ** return an error handle to the application.
	    */
	    handle = NULL;
	}

    genParm->gp_completed = TRUE;
    genParm->gp_status = status;
    genParm->gp_errorHandle = (II_PTR)handle;

    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_appCallback: request completed, status = %s\n",
	  IIAPI_PRINTSTATUS( status ) );

    if ( handle )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_appCallback: error handle %p\n", handle );
    }

    if ( genParm->gp_callback )
    {
	IIAPI_TRACE( IIAPI_TR_INFO )
	    ( "IIapi_appCallback: application callback\n" );

	(*genParm->gp_callback)( genParm->gp_closure, (II_PTR)genParm );
    }
    else  if ( handle  &&  handle->hd_delete )
    {
	genParm->gp_errorHandle = (II_PTR)IIapi_saveErrors( handle );
    }

    return;
}


/*
** Name: IIapi_thread
**
** Description:
**	Returns the local data structure for the current thread.
**	NULL is returned if any error occurs.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_THREAD *	Thread's local data structure, may be NULL.
**
**	14-Nov-96 (gordy)
**	    Added IIapi_thread() for supporting thread specific data.
*/

II_EXTERN IIAPI_THREAD *
IIapi_thread( II_VOID )
{
    IIAPI_THREAD	*thread;
    STATUS		status;
    i4		tid = PCtid();

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_thread(%d): retrieving local storage\n", tid );

    status = MEtls_get( &IIapi_static->api_thread, (PTR *)&thread );

    if ( status != OK )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_thread: error retrieving local storage: 0x%x\n", status );
	
	return( NULL );
    }

    if ( ! thread )
    {
	thread = (IIAPI_THREAD *)MEreqmem( 0, sizeof( IIAPI_THREAD ),
					   TRUE, &status );
	
	if ( ! thread )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_thread: error allocating local storage 0x%x\n",
		  status );
	}
	else
	{
	    IIAPI_TRACE( IIAPI_TR_INFO )
		("IIapi_thread(%d): allocated local storage %p\n",tid,thread);
	
	    QUinit( &thread->api_op_q );

	    status = MEtls_set( &IIapi_static->api_thread, (PTR)thread );

	    if ( status != OK )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_thread: error saving local storage 0x%x\n",
		      status );
	    
		MEfree( (PTR)thread );
		thread = NULL;
	    }
	}
    }

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_thread(%d): retrieved local storage %p\n", tid, thread );

    return( thread );
}


/*
** Name: IIapi_isVAR
**
** Description:
**	This function checks if the input data type is variable length
**      or not.
**
** Input:
**      dataType	data type to be checked
**
** Output:
**	None.
**
** Returns:
**      II_BOOL		TRUE if input data type is variable length,
**			FALSE otherwise.
** History:
**	15-Mar-96 (gordy)
**	    Created.
**	 1-Mar-01 (gordy)
**	    Added support for IIAPI_NVCH_TYPE.
*/

II_EXTERN II_BOOL
IIapi_isVAR( IIAPI_DT_ID dataType )
{
    switch( abs( dataType ) )
    {
	case IIAPI_VCH_TYPE:
	case IIAPI_VBYTE_TYPE:
	case IIAPI_NVCH_TYPE :
	case IIAPI_TXT_TYPE:
	case IIAPI_LTXT_TYPE:
	    return( TRUE );
    }

    return( FALSE );
}



/*
** Name: IIapi_isBLOB
**
** Description:
**	This function checks if the input data type is a large object or
**      not.
**
**      dataType   data type to be checked
**
** Return value:
**      status  TRUE if input data type is a BLOB data type,
**              FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	17-Jan-95 (gordy)
**	    Conditionalize code for non-BLOB support.
**	13-Jun-95 (gordy)
**	    Remove DB_LTXT_TYPE.
**	 1-Mar-01 (gordy)
**	    Added support for IIAPI_LNVCH_TYPE.
*/

II_EXTERN II_BOOL
IIapi_isBLOB( IIAPI_DT_ID dataType )
{
    switch( abs( dataType ) )
    {
#ifdef DB_LBIT_TYPE
	/* !!!!! FIX-ME: need to add BIT datatypes to API */
	case DB_LBIT_TYPE:
#endif
	case IIAPI_LVCH_TYPE:
	case IIAPI_LBYTE_TYPE:
	case IIAPI_LNVCH_TYPE:
	    return( TRUE );
    }

    return( FALSE );
}



/*
** Name: IIapi_isUCS2
**
** Description:
**	This function checks if the input data type is a Unicode type
**      or not.
**
** Input:
**      dataType	data type to be checked
**
** Output:
**	None.
**
** Returns:
**      II_BOOL		TRUE if input data type is a Unicode type,
**			FALSE otherwise.
** History:
**	 1-Mar-01 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_isUCS2( IIAPI_DT_ID dataType )
{
    switch( abs( dataType ) )
    {
	case IIAPI_NCHA_TYPE:
	case IIAPI_NVCH_TYPE:
	case IIAPI_LNVCH_TYPE :
	    return( TRUE );
    }

    return( FALSE );
}



/*
** Name: IIapi_isLocator
**
** Description:
**	This function checks if the input data type is a 
**	Blob/Clob locator type
**
** Input:
**      dataType	data type to be checked
**
** Output:
**	None.
**
** Returns:
**      II_BOOL		TRUE if input data type is a locator type,
**			FALSE otherwise.
** History:
**	25-Oct-06 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_isLocator( IIAPI_DT_ID dataType )
{
    switch( abs( dataType ) )
    {
	case IIAPI_LCLOC_TYPE:
	case IIAPI_LBLOC_TYPE:
	case IIAPI_LNLOC_TYPE :
	    return( TRUE );
    }

    return( FALSE );
}



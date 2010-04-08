/*
** Multi-threaded OpenAPI test
**
** Runs a variable number of simultaneous threads,
** where each thread will connect to the database,
** issue a query, receive the results, and disconnect
** from the database.
**
** Arguments:	[-f] <db> <tasks>
**
**	-f	Optional flag forces threads to yield between operations.
**	<db>	Database name in standard ingres format.
**	<tasks>	Number of simultaneous threads/queries.
**
** History:
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	29-Sep-2004 (drivi01)
**	    Added NEEDLIBS jam hint for generating Jamfiles.
*/

/* Jam hints
**
NEEDLIBS = COMPATLIB APILIB CUFLIB 
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <iiapi.h>
#include <compat.h>
#include <sqlstate.h>

static II_BOOL	yield = FALSE;

static void	Usage( char *name );
static void	Wait( IIAPI_GENPARM *genParm );
static void	Print_Errors( II_PTR handle );

static void 	api_init( void );
static void 	api_term( void );
static void 	api_connect( char *, IIAPI_CONNPARM * );
static void 	api_disconn( IIAPI_CONNPARM * );
static void 	api_query( IIAPI_CONNPARM *, IIAPI_QUERYPARM * );
static void 	api_descr( IIAPI_QUERYPARM *, IIAPI_GETCOLPARM * );
static II_BOOL	api_data( IIAPI_GETCOLPARM *, int );
static void 	api_qinfo( IIAPI_QUERYPARM * );
static void 	api_close( IIAPI_QUERYPARM * );
static void 	api_rollback( IIAPI_QUERYPARM * );

static void *task_func( void * );

int		TaskCount;


int 
main( int argc, char **argv )
{
    char		database[ 33 ];
    int  		tid;
    int			i, tasks = -1;

    database[ 0 ] = '\0';

    /*
    ** Flags:
    ** Arguments: [-f] <db> <tasks>
    */
    for( i = 1; i < argc; i++ )
	if ( argv[ i ][ 0 ] == '-' )
	{
	    switch( argv[ i ][ 1 ] )
	    {
		case 'f' :
		    if ( yield )
			Usage( argv[ 0 ] );
		    else
			yield = TRUE;
		    break;
		
		default :
		    Usage( argv[ 0 ] );
		    break;
	    }
	}
	else if ( database[ 0 ] == '\0' )
	    strcpy( database, argv[ i ] );
	else  if ( tasks < 0 )
	    tasks = atoi( argv[ i ] );
	else
	    Usage( argv[ 0 ] );

    if ( database[ 0 ] == '\0'  ||  tasks < 0 )  
	Usage( argv[ 0 ] );

    TaskCount = tasks;

    api_init();

    for( i = 0; i < tasks; i++ )
    {
	CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) task_func, 
				database, 0, &tid );
	printf( "[%d] Created task %d\n", GetCurrentThreadId(), tid );
	if ( yield )  Sleep(0);
    }


    while (TaskCount > 0)
	Sleep (1000);

    printf( "[%d] Exiting\n", GetCurrentThreadId() );
    
    api_term();

    return( 0 );
}

static void
Usage( char *name )
{
    printf( "\nInvalid Parameter!\n" );
    printf( "Usage: %s [-f] <db> <tasks>\n\n", name );
    exit( 0 );
}

static void *
task_func( void *arg )
{
    IIAPI_CONNPARM	conn;
    IIAPI_QUERYPARM	query;
    IIAPI_GETCOLPARM	gcol;
    char		*database = (char *)arg;
    int			tuple = 0;

    printf( "[%d] Starting\n", GetCurrentThreadId() );

    api_connect( database, &conn );
    if ( yield )  Sleep(0);

    query.qy_tranHandle = NULL;

    api_query( &conn, &query );
    if ( yield )  Sleep(0);

    api_descr( &query, &gcol );
    if ( yield )  Sleep(0);

    while( api_data( &gcol, tuple++ ) )
        if ( yield )  Sleep(0);

    api_qinfo( &query );
    if ( yield )  Sleep(0);

    api_close( &query );
    if ( yield )  Sleep(0);

    api_rollback( &query );
    if ( yield )  Sleep(0);

    api_disconn( &conn );
    if ( yield )  Sleep(0);

    printf( "[%d] Exiting\n", GetCurrentThreadId() );

    InterlockedDecrement (&TaskCount);
    return( NULL );
}

static void
Wait( IIAPI_GENPARM *genParm )
{
    IIAPI_WAITPARM	wait;

    wait.wt_timeout = -1;

    while( ! genParm->gp_completed )  IIapi_wait( &wait );

    switch( genParm->gp_status )
    {
	case  IIAPI_ST_SUCCESS : break;

	case IIAPI_ST_MESSAGE :
	    printf( "[%d] User Message:\n", GetCurrentThreadId() );
	    break;

	case IIAPI_ST_WARNING :
	    printf( "[%d] Warning:\n", GetCurrentThreadId() );
	    break;

	case IIAPI_ST_NO_DATA :
	    printf( "[%d] *** end-of-data ***\n", GetCurrentThreadId() );
	    break;

	default :
	    printf( "[%d] Error: %s\n", GetCurrentThreadId(),
		(genParm->gp_status == IIAPI_ST_ERROR) ? "ERROR" :
		(genParm->gp_status == IIAPI_ST_FAILURE) ? "FAILURE" :
		(genParm->gp_status == IIAPI_ST_NOT_INITIALIZED) ? "NO INIT" :
		(genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY) ? "NO MEMORY" :
		(genParm->gp_status == IIAPI_ST_INVALID_HANDLE) ? "BAD HNDL" :
		"??" );

	    if ( genParm->gp_errorHandle )  
		Print_Errors( genParm->gp_errorHandle );
	    exit( 0 );
    }

    if ( genParm->gp_errorHandle )  Print_Errors( genParm->gp_errorHandle );

    return;
}

static void
Print_Errors( II_PTR handle )
{
    IIAPI_GETEINFOPARM	gete;
    char 		sqlstate[ 6 ], type[ 33 ];
    int			i;

    do
    {
	gete.ge_errorHandle = (II_PTR)handle;

	IIapi_getErrorInfo( &gete );
	if ( gete.ge_status != IIAPI_ST_SUCCESS )  break;
	    
	switch( gete.ge_type )
	{
	    case IIAPI_GE_ERROR		: strcpy( type, "ERROR" );	break;
	    case IIAPI_GE_WARNING	: strcpy( type, "WARNING" );	break;
	    case IIAPI_GE_MESSAGE	: strcpy( type, "USER" );	break;
	    default : sprintf( type, "????? (%d)", gete.ge_type );	break;
	}

	printf( "[%d] Info: %s '%s' %d: %s\n", GetCurrentThreadId(),
		type, gete.ge_SQLSTATE, gete.ge_errorCode,
		gete.ge_message ? gete.ge_message : "NULL" );

	if ( gete.ge_serverInfoAvail )
	{
	    ss_decode( sqlstate, gete.ge_serverInfo->svr_id_error );
	    printf( "[%d]     : %s (0x%x) %d (0x%x) %d %d 0x%x\n",
		    GetCurrentThreadId(), sqlstate, 
		    gete.ge_serverInfo->svr_id_error,
		    gete.ge_serverInfo->svr_local_error,
		    gete.ge_serverInfo->svr_local_error,
		    gete.ge_serverInfo->svr_id_server,
		    gete.ge_serverInfo->svr_server_type,
		    gete.ge_serverInfo->svr_severity );

	    for( i = 0; i < gete.ge_serverInfo->svr_parmCount; i++ )
	    {
		printf( "[%d]     : ", GetCurrentThreadId() );

		if ( gete.ge_serverInfo->svr_parmDescr[i].ds_columnName && 
		     *gete.ge_serverInfo->svr_parmDescr[i].ds_columnName)
		    printf("%s = ", 
			   gete.ge_serverInfo->svr_parmDescr[i].ds_columnName );

		if ( gete.ge_serverInfo->svr_parmDescr[i].ds_nullable  &&  
		     gete.ge_serverInfo->svr_parmValue[i].dv_null )
		    printf( "NULL" );
		else
		{
		    switch( gete.ge_serverInfo->svr_parmDescr[i].ds_dataType )
		    {
			case IIAPI_CHA_TYPE :
			    printf( "'%*.*s'", 
				gete.ge_serverInfo->svr_parmValue[i].dv_length,
				gete.ge_serverInfo->svr_parmValue[i].dv_length,
				gete.ge_serverInfo->svr_parmValue[i].dv_value );
			    break;

			default : 
			    printf( "Datatype %d not displayed",
			    gete.ge_serverInfo->svr_parmDescr[i].ds_dataType );
			    break;
		    }
		}

		printf( "\n" );
	    }
	}

    } while( 1 );

    return;
}


static void
api_init( void )
{
    IIAPI_INITPARM	init;

    init.in_timeout = -1;
    init.in_version = IIAPI_VERSION_1;

    printf( "[%d] Initializing API\n", GetCurrentThreadId() );
    IIapi_initialize( &init );

    if ( init.in_status != IIAPI_ST_SUCCESS )
    {
	printf( "[%d] Error in IIapi_initialize: %d\n", 
		GetCurrentThreadId(), init.in_status );
	exit( 0 );
    }

    return;
}

static void
api_term( void )
{
    IIAPI_TERMPARM	term;

    printf( "[%d] Shutting down API\n", GetCurrentThreadId() );
    IIapi_terminate( &term );

    if ( term.tm_status != IIAPI_ST_SUCCESS )
    {
	printf( "[%d] Error in IIapi_terminate: %d\n", 
		GetCurrentThreadId(), term.tm_status );
	exit( 0 );
    }

    return;
}

static void
api_connect( char *database, IIAPI_CONNPARM *conn )
{
    conn->co_genParm.gp_callback = NULL;
    conn->co_genParm.gp_closure = NULL;
    conn->co_connHandle = NULL;
    conn->co_tranHandle = NULL;
    conn->co_target = database;
    conn->co_username = NULL;
    conn->co_password = NULL;
    conn->co_timeout = -1;

    printf( "[%d] Connecting to %s\n", GetCurrentThreadId(), database );

    IIapi_connect( conn );
    Wait( &conn->co_genParm );

    printf( "[%d] Connected: API level %d\n", 
	    GetCurrentThreadId(), conn->co_apiLevel );

    return;
}

static void
api_disconn( IIAPI_CONNPARM *conn )
{
    IIAPI_DISCONNPARM	dcon;

    dcon.dc_genParm.gp_callback = NULL;
    dcon.dc_genParm.gp_closure = NULL;
    dcon.dc_connHandle = conn->co_connHandle;

    printf( "[%d] Disconnecting\n", GetCurrentThreadId() );
    IIapi_disconnect( &dcon );
    Wait( &dcon.dc_genParm );

    return;
}

static void
api_query( IIAPI_CONNPARM *conn, IIAPI_QUERYPARM *query )
{
    query->qy_genParm.gp_callback = NULL;
    query->qy_genParm.gp_closure = NULL;
    query->qy_connHandle = conn->co_connHandle;
    query->qy_queryText = "select * from iicolumns";
    query->qy_stmtHandle = NULL;
    query->qy_parameters = FALSE;
    query->qy_queryType = IIAPI_QT_QUERY;

    printf( "[%d] Starting Query\n", GetCurrentThreadId() );
    IIapi_query( query );
    Wait( &query->qy_genParm );

    return;
}

static void
api_descr( IIAPI_QUERYPARM *query, IIAPI_GETCOLPARM *gcol )
{
    IIAPI_GETDESCRPARM	gdesc;
    int			i, len;

    gdesc.gd_genParm.gp_callback = NULL;
    gdesc.gd_genParm.gp_closure = NULL;
    gdesc.gd_stmtHandle = query->qy_stmtHandle;
    gdesc.gd_descriptorCount = 0;
    gdesc.gd_descriptor = NULL;

    printf( "[%d] Reading Descriptor\n", GetCurrentThreadId() );
    IIapi_getDescriptor( &gdesc );
    Wait( &gdesc.gd_genParm );

    gcol->gc_genParm.gp_callback = NULL;
    gcol->gc_genParm.gp_closure = NULL;
    gcol->gc_stmtHandle = query->qy_stmtHandle;
    gcol->gc_rowCount = 1;
    gcol->gc_columnCount = gdesc.gd_descriptorCount;
    gcol->gc_columnData = (IIAPI_DATAVALUE *)malloc(sizeof(IIAPI_DATAVALUE) *
						    gdesc.gd_descriptorCount);

    for( i = len = 0; i < gdesc.gd_descriptorCount; i++ )
	len += gdesc.gd_descriptor[ i ].ds_length;
    
    gcol->gc_columnData[ 0 ].dv_value = malloc( len );

    for( 
	 i = 1, len = gdesc.gd_descriptor[ 0 ].ds_length; 
	 i < gdesc.gd_descriptorCount; 
	 len += gdesc.gd_descriptor[ i++ ].ds_length 
       )
	gcol->gc_columnData[ i ].dv_value = 
		    (II_PTR)((char *)gcol->gc_columnData[ 0 ].dv_value + len);

    return;
}

static II_BOOL
api_data( IIAPI_GETCOLPARM *gcol, int tuple )
{
    printf( "[%d] Reading Tuple %d\n", GetCurrentThreadId(), tuple );
    IIapi_getColumns( gcol );
    Wait( &gcol->gc_genParm );

    return( gcol->gc_genParm.gp_status == IIAPI_ST_SUCCESS );
}

static void
api_qinfo( IIAPI_QUERYPARM *query )
{
    IIAPI_GETQINFOPARM getq;

    getq.gq_genParm.gp_callback = NULL;
    getq.gq_genParm.gp_closure = NULL;
    getq.gq_stmtHandle = query->qy_stmtHandle;

    printf( "[%d] Reading Query Info\n", GetCurrentThreadId() );
    IIapi_getQueryInfo( &getq );
    Wait( &getq.gq_genParm );

    printf( "[%d] Query Info: flags = 0x%x\n", GetCurrentThreadId(), getq.gq_flags );

    if ( getq.gq_mask & IIAPI_GQ_ROW_COUNT )
	printf( "[%d] Query Info: rows = %d\n", 
		GetCurrentThreadId(), getq.gq_rowCount );

    return;
}

static void
api_close( IIAPI_QUERYPARM *query )
{
    IIAPI_CLOSEPARM	clse;

    clse.cl_genParm.gp_callback = NULL;
    clse.cl_genParm.gp_closure = NULL;
    clse.cl_stmtHandle = query->qy_stmtHandle;
    query->qy_stmtHandle = NULL;

    printf( "[%d] Closing query\n", GetCurrentThreadId() );
    IIapi_close( &clse );
    Wait( &clse.cl_genParm );

    return;
}

static void
api_rollback( IIAPI_QUERYPARM *query )
{
    IIAPI_ROLLBACKPARM	roll;

    roll.rb_genParm.gp_callback = NULL;
    roll.rb_genParm.gp_closure = NULL;
    roll.rb_tranHandle = query->qy_tranHandle;
    roll.rb_savePointHandle = NULL;
    query->qy_tranHandle = NULL;

    printf( "[%d] Rolling back Transaction\n", GetCurrentThreadId() );
    IIapi_rollback( &roll );
    Wait( &roll.rb_genParm );

    return;
}

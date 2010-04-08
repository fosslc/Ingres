/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apiademo.c
**
** Description:
**	OpenAPI asynchronous demo main control module.
**
**	This program demonstrates calling OpenAPI asynchronously, 
**	using callback functions for notification of OpenAPI call 
**	completion and the processing of database events using
**	OpenAPI.  The program consist of four modules: a main 
**	control module (apiademo.c), a server module (apiasvr.c), 
**	a client module (apiaclnt.c) and a utility module 
**	(apiautil.c). 
**
**	The server module establishes a connection to the target 
**	database, creates a database event and registers to receive
**	event notification.  The server then waits for database 
**	events to be received.  The client module establishes it's 
**	own database connection, raises a series of database 
**	events and terminates.  The server terminates when a
**	database event is received indicating that termination
**	has been requested.
**
**	The main control module processes the command line arguments
**	and initializes, runs and terminates the client and server
**	modules.  The utility module provides common synchronous
**	OpenAPI calls used during initialization and termination.
**
**	The program can be run as a client, a server or both.  Multiple
**	clients and multiple servers may be run concurrently.
**
** Command syntax:
**	apiademo [-s] [-c] [-t] [-i] dbname [count]
**	-s	Act as server   
**	-c	Act as client   
**	-t	Terminating server 
**      -i      dbevent info 
**	dbname	Target node, database and server
**	count	Number of DB events to be raised by client. 
**
**	'apiademo db' is the same as 'apiademo -s -c -t db 5'.
**	'apiademo -c db' is the same as apiademo -c db 5'.
**	'apiademo -t db' is the same as apiademo -c -t db 0'.
*/

# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>
# include "apiademo.h"
# include "apiasvr.h"
# include "apiaclnt.h"


/*
** Constants
*/

#define  DEFAULT_CLIENT_COUNT   5 


/*
** Global Variables
*/

typedef struct
{
    II_PTR	envHandle;	/* API environment handle */
    int		async_count; 	/* Number of async API calls active */
    II_BOOL	mode_abort; 	/* Abort operation */
    II_BOOL	mode_server;	/* Run server */
    II_BOOL	mode_client;	/* Run client */
    II_BOOL	mode_term;	/* Terminate client */
    II_BOOL	mode_verbose;	/* Verbose output */
    II_BOOL	mode_info;   	/* failed dbevent info */
    char	*dbname;	/* Target database */
    int		client_count; 	/* Number of events to raise */
} GLOBAL;

static	GLOBAL	global;


/*
** Local functions
*/

static	void	IIdemo_usage();		/* Print command line format */
static	void	IIdemo_args();		/* Process command line */
static	void	IIdemo_init();		/* Initialize OpenAPI */
static	void	IIdemo_term();		/* Terminate OpenAPI */



/*
** Name: main - main loop
**
** Description:
**	Central processing function of the OpenAPI async demo program.
**	Initializes, runs and terminates client and server modules.
**	Main processing primarily occurs in a loop calling IIapi_wait()
**	which allows OpenAPI to process the async requests from the
**	client and server modules.  Client/server module processing
**	occurs in the callbacks from the API async requests.
*/

int
main( int argc, char **argv )
{
    II_BOOL	 server_init = FALSE;
    II_BOOL	 client_init = FALSE;
    IIAPI_STATUS status;

    printf( "\nOpenAPI Asynchronous Demo\n");

    IIdemo_args( argc, argv );	/* Process command line */
    IIdemo_init();		/* Initialize OpenAPI */

    if ( global.mode_server )
    {
	printf( "Initializing Server Module\n" );

	status = IIdemo_init_server( global.envHandle, 
				     global.dbname, 
				     global.mode_verbose );

	if ( status == IIAPI_ST_SUCCESS )
	    server_init = TRUE;
	else
	   global.mode_server = global.mode_client = FALSE;
    }

    if ( global.mode_client ) 
    {
	printf( "Initializing Client Module\n");

	status = IIdemo_init_client( global.envHandle, 
				     global.dbname, global.mode_verbose );
	if ( status  == IIAPI_ST_SUCCESS )
	    client_init = TRUE;
	else
	    global.mode_client = global.mode_server = FALSE;
    }     

    if ( global.mode_server )
    {
       printf( "Running Server Module\n" );
       IIdemo_run_server( global.mode_info, global.mode_verbose );
    }

    if ( global.mode_client ) 
    {
       printf( "Running Client Module\n" );
       IIdemo_run_client( global.client_count, 
			  global.mode_term, global.mode_verbose );
    }

    /*
    ** Server and client modules run asynchronously.  Once they have
    ** posted their first requests, control returns to this module.
    ** Here we loop calling IIapi_wait() until an abort is requested
    ** or all asynchronous requests have completed.  Subsequent
    ** server and client operations all happen as callbacks from
    ** OpenAPI when the async requests complete (sometime during
    ** the execution of IIapi_wait()).
    */
    while( ! global.mode_abort  &&  global.async_count > 0 )
    {
	IIAPI_WAITPARM      waitParm;

	if ( global.mode_verbose )  printf( "\t\tCalling IIapi_wait()\n" );
	waitParm.wt_timeout = -1;
	IIapi_wait( &waitParm );
	if ( waitParm.wt_status != IIAPI_ST_SUCCESS )  IIdemo_set_abort();
    }
     
    if ( server_init )
    {
       printf( "Terminating Server Module\n" );
       IIdemo_term_server( global.mode_abort, global.mode_verbose );
    }

    if ( client_init )
    {
       printf( "Terminating Client Module\n" );
       IIdemo_term_client( global.mode_abort, global.mode_verbose );
    }

    IIdemo_term();		/* Terminate OpenAPI */
    printf( "Done!\n" );
    return( 0 );
}


/*
** Name: IIdemo_start_async
**
** Description:
**	Notes start of async API request.  Multiple concurrent
**	async requests are permitted.  A matching call to
**	IIdemo_complete_async() is required when the request
**	completes.  
**
**	Control module will loop calling IIapi_wait() as long 
**	as there are active async requests.
*/

void 
IIdemo_start_async()
{
    global.async_count++;
    return;
}


/*
** Name: IIdemo_complete_async
**
** Description:
**	Notes end of async API request.  A prior matching call
**	to IIdemo_start_async() must have been made when the 
**	async request was started.
*/

void 
IIdemo_complete_async()
{
    global.async_count--;
    return;
}


/*
** Name: IIdemo_set_abort
**
** Description:
**	Signals abort condition to control module to terminate
**	processing and shutdown client/server modules.
*/

void 
IIdemo_set_abort()
{
    if ( global.mode_verbose )  printf( "\t\tAbort requested!\n" );
    global.mode_abort = TRUE; 
    return;
}


/*
** Name: IIdemo_usage
**
** Description:
**	Print command line usage and exit.
*/

static void 
IIdemo_usage()
{
   printf( "usage: apiademo [-c] [-s] [-t] dbname [count]\n" );
   exit( 1 );
}


/*
** Name: IIdemo_args
**
** Description:
**	Process command line arguments.
*/

static void
IIdemo_args( int argc, char **argv )
{
    II_BOOL		arg_count = FALSE; 
    int                 i;

    global.mode_server  = FALSE;
    global.mode_client  = FALSE;
    global.mode_term    = FALSE; 
    global.mode_verbose = FALSE;
    global.mode_info    = FALSE;
    global.dbname	= NULL;
    global.client_count = 0; 

    /*
    ** Process command line arguments
    */
    for( i = 1; i < argc; i++ )
    {
	if ( argv[i][0] == '-' )	/* Switches */
	{
	    if ( ! argv[i][1]  ||  argv[i][2] )  IIdemo_usage();

	    switch( argv[i][1] )
	    {
		case 's': global.mode_server = TRUE;	break;
		case 'c': global.mode_client = TRUE;	break;
		case 't': global.mode_term = TRUE;	break;
		case 'v': global.mode_verbose = TRUE;	break;
		case 'i': global.mode_info    = TRUE;	break;
		default:  IIdemo_usage();
	    }
	}
	else  if ( ! global.dbname )	/* First non-switch is database name */
	    global.dbname = argv[i];
	else  if ( ! arg_count )	/* Second non-switch is count */
	{
	    global.client_count = atoi( argv[i] );
	    arg_count = TRUE; 
	   
	    /* minimal check for valid numeric */
	    if ( ! global.client_count  &&  argv[i][0] != '0' )
		IIdemo_usage();
	}
	else				/* Two many arguments */
	    IIdemo_usage();
    }

    /*
    ** Database name is required
    */
    if ( ! global.dbname  ||  ! global.dbname[0] )  IIdemo_usage();

    /*
    ** Default operation mode if not specified.
    */
    if ( ! global.mode_client  && ! global.mode_server  && ! global.mode_term )
	global.mode_server = global.mode_client = global.mode_term = TRUE; 
    
    /*
    ** Default the raise event counter if not specified.
    */
    if ( global.mode_client  &&  ! arg_count )
        global.client_count = DEFAULT_CLIENT_COUNT; 

    /*
    ** Server termination request implies running as client.
    */
    if ( global.mode_term )  global.mode_client = TRUE;   

    if ( global.mode_verbose ) 
    {
       printf( "\t\tDatabase : %s\n", global.dbname );
       printf( "\t\tServer   : %s\n", global.mode_server ? "Yes" : "No" );
       printf( "\t\tClient   : %s\n", global.mode_client ? "Yes" : "No" );
       printf( "\t\tCount    : %d\n", global.client_count );
       printf( "\t\tInfo     : %s\n", global.mode_info ? "Yes" : "No" );
       printf( "\t\tTerminate: %s\n", global.mode_term ? "Yes" : "No" );
    } 

    return;
}


/*
** Name: IIdemo_init
**
** Description:
**	Initialize OpenAPI.
*/

static void
IIdemo_init()
{
    IIAPI_INITPARM	initParm;

    if ( global.mode_verbose )  printf( "\t\tCalling IIapi_initialize()\n" );
    initParm.in_version = IIAPI_VERSION_2;
    initParm.in_timeout = -1;
    IIapi_initialize( &initParm );

    if ( initParm.in_status != IIAPI_ST_SUCCESS )
    {
	printf( "\tIIapi_initialize() failed!\n");
	exit( 1 );
    }

    global.envHandle = initParm.in_envHandle;
    global.async_count = 0;
    global.mode_abort = FALSE;  

    return;
}


/*
** Name: IIdemo_term
**
** Description:
**	Terminate OpenAPI.
*/

static void 
IIdemo_term()
{
    IIAPI_RELENVPARM	relParm;
    IIAPI_TERMPARM	termParm;

    if ( global.mode_verbose )  printf( "\t\tCalling IIapi_releaseEnv()\n" );
    relParm.re_envHandle = global.envHandle;
    IIapi_releaseEnv( &relParm );

    if ( global.mode_verbose )  printf( "\t\tCalling IIapi_terminate()\n" );
    IIapi_terminate( &termParm );

    return;
}


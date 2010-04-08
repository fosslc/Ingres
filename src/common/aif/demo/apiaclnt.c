/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiaclnt.c 
**
** Description:
**	OpenAPI asynchronous demo client module.
**
**	The client demo establishes a connection with the target
**	database and raises database events at (somewhat) random
**	intervals.  Client execution is implemented as a series
**	of callback functions.  Each asynchronous OpenAPI request
**	specifies the next sequential function as its callback.
**	Each function represents a state in a simple finite state
**	machine as follows:
**
**	PAUSE	Pause some number of seconds.
**	RAISE	Call IIapi_query() to raise a database event.
**	INFO	Call IIapi_getQueryInfo() to check raise event status.
**	CLOSE	Call IIapi_close() to close request statement handle.
**	DONE	Check if all events raised (done) or continue with PAUSE.
*/

# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>
# include "apiademo.h"
# include "apiautil.h"
# include "apiaclnt.h"


/*
** Local definitions
*/

typedef struct pcb
{
    II_PTR	stmtHandle;	/* API statement handle */
    int		pid;		/* Client identifier */
    int		raiseCount;	/* Number of database event raised */
    int		waitsec;	/* Current wait time */

    struct
    {
	char		*name;				/* API function name */
	II_VOID		(*function)();			/* API function */
	II_VOID		(*callback)(struct pcb *);	/* CallBack function */
	IIAPI_STATUS	status;				/* Call status */

	union						/* API param blocks */
	{
	    IIAPI_GENPARM	genParm;
	    IIAPI_GETEVENTPARM	gEventParm;
            IIAPI_QUERYPARM	quryParm; 	
	    IIAPI_GETQINFOPARM	getQInfoParm; 
	    IIAPI_CLOSEPARM	close;
	} parm;
    } api;
} CLNT_PCB;


/*
** Global definitions
*/

typedef struct
{
   CLNT_PCB	clntPCB;		/* Client process control block */
   II_PTR	clnt_connHandle; 	/* API connection handle */
   II_PTR	clnt_tranHandle; 	/* API transaction handle */
   int		clnt_count;		/* Number of db events to be raised */
   II_BOOL	mode_term;		/* Terminate server */
   II_BOOL	mode_verbose;  		/* Verbose tracing */
} GLOBAL;

static GLOBAL global;


/*
** Local functions.
*/

static	void	IIdemo_pause_client( CLNT_PCB * );
static	void	IIdemo_raise_client( CLNT_PCB * );
static	void	IIdemo_info_client( CLNT_PCB * );
static	void	IIdemo_close_client( CLNT_PCB * );
static	void	IIdemo_done_client( CLNT_PCB * );
static	void	clientAPICall( CLNT_PCB * );
II_FAR II_CALLBACK II_VOID clientCallback( II_PTR, II_PTR );



/*
** Name: IIdemo_init_client 
**
** Description:
**	Initialize client module by establishing a connection to
**	the target DBMS and enable autocommit.
*/

IIAPI_STATUS 
IIdemo_init_client( II_PTR env, char *dbname, II_BOOL verbose ) 
{
    IIAPI_STATUS status;

    printf( "\tEstablishing Connection\n");

    global.clnt_connHandle = NULL;
    global.clnt_tranHandle = NULL;
    status = IIdemo_conn( env, dbname, &global.clnt_connHandle, verbose );

    if ( status == IIAPI_ST_SUCCESS )  
    {
	status = IIdemo_autocommit( &global.clnt_connHandle,
				    &global.clnt_tranHandle, verbose );

	if ( status != IIAPI_ST_SUCCESS )
	    IIdemo_abort( &global.clnt_connHandle, verbose );
    }

    return( status );  
}
 
/*
** Name: IIdemo_term_client 
**
** Description:
**	Normal termination will disable autocommit and shutdown the
**	connection. An abort termination will abort the connection. 
**
*/

void 
IIdemo_term_client( II_BOOL mode_abort, II_BOOL verbose ) 
{
    II_PTR nullHandle = NULL; 
   
    printf( "\tDisconnecting\n");

    if ( mode_abort ) 
    {
	printf( "\tAborting connection\n");
	IIdemo_abort( &global.clnt_connHandle, verbose );
    }
    else 
    {
	IIdemo_autocommit( &nullHandle, &global.clnt_tranHandle, verbose ); 
	IIdemo_disconn( &global.clnt_connHandle, verbose );
    }
     
    return;
}



/*
** Name: IIdemo_run_client 
**
** Description:
**	Main client processing will be performed asynchronously by a
**	Finite State Machine. The FSM is implemented as a series of 
**	callback functions, starting with IIdemo_pause_client().
**	Initialize a single process control block and begin processing.
*/
  
void 
IIdemo_run_client( int count, II_BOOL tmode, II_BOOL vmode) 
{
    global.clnt_count = count;  
    global.mode_term    = tmode; 
    global.mode_verbose = vmode; 

    /*
    ** If termination is requested, raise an additional
    ** database event to signal server termination.
    */
    if ( global.mode_term )  global.clnt_count++;

    if ( global.clnt_count > 0 )	/* Make sure there's something to do */
    {
	global.clntPCB.waitsec = 1; 
	global.clntPCB.raiseCount = 1;
	global.clntPCB.pid = getpid();
	IIdemo_pause_client( &global.clntPCB );
    }

    return;
}



/*
** Name: IIdemo_pause_client
**
** Description:
**	Pause the client processing for a (somewhat) random time.
**	Processing continues to IIdemo_raise_client().
**
**	Since this demo is not multi-threaded, we can't block the
**	process completely.  The server module must be given the
**	opportunity to do work.  The easiest way to pause here is
**	issue an IIapi_getEvent() request with the desired timeout.
**	Since the client connection is not registered to receive
**	database events, the API call should block until the time
**	expires (or something bad, like connection failure, occurs).
*/

static void 
IIdemo_pause_client( CLNT_PCB *pcb ) 
{
    if ( global.mode_verbose )  printf( "\t\tClient: state PAUSE\n" );

    /*
    ** Calculate new wait time.
    */
    if ( (pcb->waitsec = pcb->waitsec + pcb->raiseCount) > 10 )   
	pcb->waitsec = pcb->waitsec - 10; 

    printf( "\tClient %d: Pausing for %d sec\n", pcb->pid, pcb->waitsec );

    pcb->api.parm.gEventParm.gv_connHandle = global.clnt_connHandle; 
    pcb->api.parm.gEventParm.gv_timeout = pcb->waitsec * 1000;
    pcb->api.name = "IIapi_getEvent()";
    pcb->api.function =  IIapi_getEvent; 
    pcb->api.callback =  IIdemo_raise_client; 

    clientAPICall( pcb ); 	
    return;
}
 

/*
** Name: IIdemo_raise_client
**
** Description:
**	Call IIapi_query() to raise database event.  Processing
**	continues to IIdemo_info_client().
**
**	Checks the status of prior IIapi_getEvent().  A warning 
**	means the request timed out as expected.  Any other 
**	error status indicates a problem with the connection.
**
**	May raise a regular or termination (no event info) event.
*/

static void 
IIdemo_raise_client( CLNT_PCB *pcb ) 
{
    char raiseEvText[] = "raise dbevent %s '%d'";
    char lastEvText[]  = "raise dbevent %s ";
    char queryText[512];

    if ( global.mode_verbose )  printf( "\t\tClient: state RAISE\n" );

    /*
    ** Check previous API request status.
    */
    if ( pcb->api.status > IIAPI_ST_WARNING ) 
    {
	IIdemo_set_abort();
	return;
    }

    /*
    ** Check to see if server should be terminated.
    */
    if ( global.mode_term  &&  pcb->raiseCount == global.clnt_count )  
    {
	printf( "\tClient %d: Terminating server\n", pcb->pid );
	sprintf( queryText, lastEvText, IIdemo_eventName );
    }
    else 
    {
	printf( "\tClient %d: Raising event #%d\n", pcb->pid, pcb->raiseCount );
	sprintf( queryText, raiseEvText, IIdemo_eventName, pcb->pid );		
    }

    pcb->api.parm.quryParm.qy_connHandle = global.clnt_connHandle; 
    pcb->api.parm.quryParm.qy_queryType = IIAPI_QT_QUERY;
    pcb->api.parm.quryParm.qy_queryText = (char *)queryText;
    pcb->api.parm.quryParm.qy_parameters = FALSE;
    pcb->api.parm.quryParm.qy_tranHandle	= global.clnt_tranHandle;
    pcb->api.parm.quryParm.qy_stmtHandle = ( II_PTR )NULL;
    pcb->api.name = "IIapi_query()";
    pcb->api.function =  IIapi_query;    
    pcb->api.callback =  IIdemo_info_client;  

    clientAPICall( pcb ); 	
    return;
}


/*
** Name: IIdemo_info_client
**
** Description:
**	Call IIapi_getQueryInfo() to determine status of the
**	raise database event request.  Processing continues
**	to IIdemo_close_client().
**
**	Checks the status of prior IIapi_query().  Skip info 
**	request if error.
**
**	The API statement handle is saved for later closing.
*/

static void 
IIdemo_info_client( CLNT_PCB *pcb ) 
{
    if ( global.mode_verbose )  printf( "\t\tClient: state INFO\n" );
    pcb->stmtHandle = pcb->api.parm.quryParm.qy_stmtHandle;

    if ( pcb->api.status != IIAPI_ST_SUCCESS )
	IIdemo_close_client( pcb );
    else
    {
	pcb->api.parm.getQInfoParm.gq_stmtHandle = pcb->stmtHandle;   
	pcb->api.name = "IIapi_getQueryInfo()";
	pcb->api.function = IIapi_getQueryInfo; 
	pcb->api.callback =  IIdemo_close_client;

	clientAPICall( pcb ); 	
    }

    return;
}


/*
** Name: IIdemo_close_client
**
** Description:
**	Close the current statement handle using IIapi_close().
**	Processing continues to IIdemo_done_client().
**
**	If an error occured during prior processing, abort the 
**	demo and set count to end client processing.
*/

static void 
IIdemo_close_client( CLNT_PCB *pcb ) 
{
    if ( global.mode_verbose )  printf( "\t\tClient: state CLOSE\n" );

    if ( pcb->api.status != IIAPI_ST_SUCCESS )
    {
	pcb->raiseCount = global.clnt_count;
	IIdemo_set_abort();
    }

    if ( ! pcb->stmtHandle )
	IIdemo_done_client( pcb );
    else
    {
	pcb->api.parm.close.cl_stmtHandle   = pcb->stmtHandle;   
	pcb->api.name = "IIapi_close()";
	pcb->api.function =  IIapi_close; 
	pcb->api.callback =  IIdemo_done_client;  

	clientAPICall(pcb); 	
    }

    return;
}


/*
** Name: IIdemo_done_client
**
** Description:
**	Determine if client processing has completed.  If not,
**	processing continues to IIdemo_pause_client().
*/

static void 
IIdemo_done_client( CLNT_PCB *pcb )
{
    if ( global.mode_verbose )  printf( "\t\tClient: state DONE\n" );

    if ( pcb->raiseCount >= global.clnt_count  )
       printf( "\tClient %d: Done\n", pcb->pid );
    else
    {
       pcb->raiseCount++;
       IIdemo_pause_client(pcb);
    }

    return;  
}


/*
** Name: clientAPICall      
**
** Description:
**	Initiates the processing of an async OpenAPI request.
*/

static void 
clientAPICall( CLNT_PCB *pcb )
{
    if ( global.mode_verbose )
	printf( "\t\tClient: calling %s\n", pcb->api.name );

    pcb->api.parm.genParm.gp_callback = clientCallback; 
    pcb->api.parm.genParm.gp_closure  = (II_PTR)pcb; 
    IIdemo_start_async();
    pcb->api.function( &pcb->api.parm.genParm ); 	

    return;
}

 
/*
** Name: clientCallback      
**
** Description:
**	Completes the processing of an async OpenAPI request.
**	Continues processing with the requested client function.
*/

II_FAR II_CALLBACK II_VOID
clientCallback( II_PTR arg, II_PTR parm )
{
    CLNT_PCB		*pcb = (CLNT_PCB *)arg;
    IIAPI_GENPARM	*genParm = (IIAPI_GENPARM *)parm;

    if ( global.mode_verbose )
	printf( "\t\tClient: %s completed\n", pcb->api.name );

    IIdemo_complete_async(); 
    IIdemo_checkError( pcb->api.name, genParm ); 
    pcb->api.status = genParm->gp_status;
    pcb->api.callback( (II_PTR )pcb );

    return;
     
}
 

/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiasvr.c 
**
** Description:
**	OpenAPI asynchronous demo server module.
**
**	The server demo establishes a connection with the target
**	database, creates and registers a database event and waits
**	to receive event notifications.  Server execution is 
**	implemented in a single processing function with a set 
**	of actions.  Each action represents a state in a finite 
**	state machine as follows:
**
**	GET  	Call IIapi_getEvent() to wait for event notifications.
**	CHECK	Check status of IIapi_getEvent().
**	CANCEL	Cancel catch event request.
**	EXIT    Nothing further to do.
**	CATCH	Call IIapi_catchEvent() to catch database event notification.
**	NEW 	Check status of IIapi_catchEvent().
**	DESCR	Call IIapi_getDescriptor() to get event info description.
**	INFO 	Call IIapi_getColumns() to get database event info.
**	PROCESS Process database event info.
**	RECATCH	Call IIapi_getEvent() for next database event.
**	CLOSE  	Call IIapi_close() to free catch event resources.
**	DONE	Server processing completed.
**
**	While the server module is single threaded, there are two
**	independent logical threads of execution, each with it's
**	own process control block and state.  
**	
**	One thread (GET_EVENT) loops calling IIapi_getEvent(), 
**	with timeout, to allow OpenAPI to check for database 
**	event notifications on the database connection.  The 
**	other thread (CATCH_EVENT) issues the call to 
**	IIapi_catchEvent() and handles processing of database
**	event notifications.
**
**	A termination request is indicated by the reception of a
**	database event with no additional info.  When detected, the
**	CATCH_EVENT thread sets a termination request indicator for
**	the other thread and continues normal processing.  When the
**	GET_EVENT thread sees the termination request indication,
**	it cancels the catch event request in the other thread and
**	exits.  When the CATCH_EVENT thread receives the catch
**	event cancellation, it closes the event handle and ends
**	server processing.
**
**	Normally, the catch event would not be re-issued after
**	seeing the termination request, but for the purposes of
**	this demo it allows demonstration of IIapi_cancel() with
**	an event handle.
*/

# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <iiapi.h>
# include "apiademo.h"
# include "apiautil.h"
# include "apiasvr.h"


/*
** Local definitons
*/

/* Server FSM states */
#define	SVR_GET  	0	/* Issue IIapi_getevent()         */
#define	SVR_CHECK	1	/* Check for timeout,errors,done  */
#define	SVR_CANCEL	2	/* Cancel the active event handle */
#define SVR_EXIT        3	/* Return to caller               */
#define	SVR_CATCH	4	/* Issue IIapi_catchEvent()       */ 
#define	SVR_NEW 	5	/* Check additional DB event info */
#define	SVR_DESCR	6	/* Issue IIapi_getDescriptor()    */
#define	SVR_INFO 	7	/* Issue IIapi_getColumns()       */
#define	SVR_PROCESS     8	/* Process DB event info          */
#define	SVR_RECATCH	9	/* Catch event for existing event handle */
#define	SVR_CLOSE  	10	/* Issue IIapi_close()            */
#define	SVR_DONE	11	/* Done                           */

#define SVR_TIMEOUT     2000;   /* server getEvent timeout        */

typedef struct
{
    II_INT2             sequence;	/* Current state */
    int                 pid;		/* Server ID */

    II_PTR              eventHandle;	/* Active event handle */
    IIAPI_STATUS	api_status;	/* API function status */
    IIAPI_DATAVALUE     datavalue;	/* Event info storage */
    char                var1[ 20 ]; 

    struct
    {
	char			*name;		/* API function name */
	II_VOID                 (*function)();	/* API function */

	IIAPI_CATCHEVENTPARM    cEventParm;	/* API parameter blocks */
	union
	{
	    IIAPI_GETDESCRPARM	gDescParm;
	    IIAPI_GETCOLPARM	gColParm;
	    IIAPI_CLOSEPARM     close;
     	    IIAPI_CANCELPARM	cancel;
	    IIAPI_GETEVENTPARM  gEventParm;
	} parm;
    } api;
} SVR_PCB;


/*
** Global variables
*/

typedef struct  
{
    II_PTR    svr_connHandle; 		/* API connection handle */
    II_PTR    svr_tranHandle; 		/* API transaction handle */
    SVR_PCB   eventPCB; 		/* Catch event process control block */
    SVR_PCB   getPCB; 			/* Get event process control block */
    II_BOOL   mode_term; 		/* Terminate request received */
    II_BOOL   mode_verbose;		/* Verbose tracing */
}GLOBAL;

static GLOBAL global;

static char *state[] =
{
    "GET",   "CHECK", "CANCEL",	 "EXIT",    "CATCH", "NEW",
    "DESCR", "INFO",  "PROCESS", "RECATCH", "CLOSE", "DONE"
};

#define	ARRAY_SIZE( array )	(sizeof(array) / sizeof(array[0]))


/*
** Local functions
*/

static	void	IIdemo_process_server( SVR_PCB * );
static	void	serverAPICall( SVR_PCB *, IIAPI_GENPARM * );
II_FAR II_CALLBACK II_VOID serverCallback( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID eventCallback( IIAPI_EVENTPARM * );




/*
** Name: IIdemo_init_server 
**
** Description:
**	Initialize server module by establishing a connection to
**	the target DBMS, enable autocommit, create and register
**	database event.
*/

IIAPI_STATUS 
IIdemo_init_server( II_PTR env, char *dbname, II_BOOL verbose ) 
{
    IIAPI_SETENVPRMPARM	setEnvPrmParm;
    IIAPI_STATUS	status; 

    /*
    ** Set the callback function for uncaught Database Events.
    */
    setEnvPrmParm.se_envHandle = env;
    setEnvPrmParm.se_paramID = IIAPI_EP_EVENT_FUNC;
    setEnvPrmParm.se_paramValue = (void *)eventCallback; 
    
    if ( verbose )  printf( "\t\tCalling IIapi_setEnvParam()\n" );
    IIapi_setEnvParam( &setEnvPrmParm);
    
    if ( setEnvPrmParm.se_status != IIAPI_ST_SUCCESS )
	return( setEnvPrmParm.se_status );

    printf( "\tEstablishing Connection \n");
    status = IIdemo_conn( env, dbname, &global.svr_connHandle, verbose );

    if ( status == IIAPI_ST_SUCCESS )
    {
	status = IIdemo_autocommit( &global.svr_connHandle,
				    &global.svr_tranHandle, verbose );

	if ( status == IIAPI_ST_SUCCESS )
	{
	    char creEvText[] = "create dbevent %s";
	    char regEvText[] = "register dbevent %s";
	    char queryText[512];

	    printf( "\tCreating DB Event  \n");
	    sprintf( queryText, creEvText, IIdemo_eventName );
	    IIdemo_query( &global.svr_connHandle, 
			  &global.svr_tranHandle, queryText, verbose );

	    printf( "\tRegistering for DB Event  \n");
	    sprintf( queryText, regEvText, IIdemo_eventName );
	    status = IIdemo_query( &global.svr_connHandle,
				   &global.svr_tranHandle,
				   queryText, verbose ); 
	}

	if ( status != IIAPI_ST_SUCCESS )
	    IIdemo_abort( &global.svr_connHandle, verbose );
    }

    return( status );
}


/*
** Name: IIdemo_term_server 
**
** Description:
**	Normal termination will de-register and drop the database
**	event, disable autocommit and shutdown the connection. An 
**	abort termination will abort the connection. 
*/

void 
IIdemo_term_server( II_BOOL mode_abort, II_BOOL verbose ) 
{
    if ( mode_abort ) 
    {
	printf( "\tAbort server connection\n" );
	IIdemo_abort( &global.svr_connHandle, verbose );
    }
    else 
    {
	II_PTR   nullHandle = NULL; 
	char     dropEvText[]   = "drop dbevent %s";
	char     removeEvText[] = "remove dbevent %s";
	char     queryText[ 64 ];
   
	printf( "\tDe-register DB Event\n" );
	sprintf( queryText, removeEvText, IIdemo_eventName );
	IIdemo_query( &global.svr_connHandle, &global.svr_tranHandle, 
		      queryText, verbose );

	printf( "\tDestroying DB Event  \n");
	sprintf( queryText, dropEvText, IIdemo_eventName );
	IIdemo_query( &global.svr_connHandle, &global.svr_tranHandle, 
		      queryText, verbose );

	printf( "\tDisconnecting \n");
	IIdemo_autocommit( &nullHandle, &global.svr_tranHandle, verbose );    
	IIdemo_disconn( &global.svr_connHandle, verbose );
    }
     
    return;
}


/*
** Name: IIdemo_run_server 
**
** Description:
**	Main client processing will be performed asynchronously by a
**	Finite State Machine. The FSM is implemented as a single
**	processing function.  Initialize two process control blocks
**	and start each processing in the FSM.
*/

void 
IIdemo_run_server( II_BOOL mode_info, II_BOOL mode_verbose ) 
{
    global.mode_verbose = mode_verbose; 
    global.mode_term = FALSE; 
    global.eventPCB.eventHandle = NULL;    

    if ( !mode_info )
    {
    	global.eventPCB.sequence = SVR_CATCH;
    	global.eventPCB.pid = getpid();
    	IIdemo_process_server( &global.eventPCB );
    }

    global.getPCB.sequence = SVR_GET;   
    global.getPCB.pid = getpid();
    IIdemo_process_server( &global.getPCB );

    return;
}



/*
** Name: IIdemo_process_server 
**
** Description:
**	Server FSM processing.  Each state is executed as a case of
**	a large switch statement.  State transition to the next
**	sequential state occurs automatically.  Branching is done
**	by assigning the next desired state explicitly.  A state
**	either executes an async OpenAPI request and returns to
**	wait for the request to complete, or breaks from the switch
**	to allow processing immediatly with the next state.
*/

static void
IIdemo_process_server( SVR_PCB *pcb )
{
  top:

    if ( global.mode_verbose )
	printf( "\t\tServer: state %s\n",
		(pcb->sequence >= 0  &&  pcb->sequence < ARRAY_SIZE( state )) 
		? state[ pcb->sequence ] : "?" );

    /*
    ** Execute current state (proceed to next sequential state).
    */
    switch( pcb->sequence++ )
    {
    case SVR_GET :		/* Check connection for event notification */
	pcb->api.parm.gEventParm.gv_connHandle = global.svr_connHandle; 
	pcb->api.parm.gEventParm.gv_timeout = SVR_TIMEOUT;
        pcb->api.name = "IIapi_getEvent()";
	pcb->api.function =  IIapi_getEvent;

	serverAPICall( pcb, &pcb->api.parm.gEventParm.gv_genParm );
        return;

    case SVR_CHECK :		/* Check for error or termination condition */
	if ( !(global.mode_term || pcb->api_status > IIAPI_ST_WARNING ))
	    pcb->sequence = SVR_GET;   
	break; 

    case SVR_CANCEL:		/* Cancel catch event request */
	if ( global.eventPCB.eventHandle != NULL )
	{
	    printf( "\tServer %d: Cancelling DB Event\n", pcb->pid );
        
	    pcb->api.parm.cancel.cn_stmtHandle = global.eventPCB.eventHandle;
	    pcb->api.name = "IIapi_cancel()";
	    pcb->api.function = IIapi_cancel;

	    serverAPICall( pcb, &pcb->api.parm.cancel.cn_genParm );
	}
	return;

    case SVR_EXIT :		/* Nothing more to do */
	return;

    case SVR_CATCH :		/* Catch a database event */
	printf( "\tServer %d: Waiting for DB Event\n", pcb->pid );

	pcb->api.cEventParm.ce_connHandle = global.svr_connHandle; 
	pcb->api.cEventParm.ce_selectEventName = IIdemo_eventName;  
	pcb->api.cEventParm.ce_selectEventOwner = NULL;    
	pcb->api.cEventParm.ce_eventHandle = NULL;    
	pcb->api.name = "IIapi_catchEvent()";
	pcb->api.function = IIapi_catchEvent;

	serverAPICall( pcb, &pcb->api.cEventParm.ce_genParm );

	/*
	** If request has not completed, need to save the
	** event handle in case request needs to be cancelled
	** before it completes.
	*/
	if ( ! pcb->api.cEventParm.ce_genParm.gp_completed )
            pcb->eventHandle = pcb->api.cEventParm.ce_eventHandle;
	return;     

    case SVR_NEW :	/* Check catch status: failure, terminate, normal */
	/*
	** Save event handle for later reference.
	*/
	pcb->eventHandle = pcb->api.cEventParm.ce_eventHandle;

	if ( pcb->api_status != IIAPI_ST_SUCCESS )
	    pcb->sequence = SVR_CLOSE;		/* Catch failed */
	else
        {
	    if ( pcb->api.cEventParm.ce_eventInfoAvail )
		pcb->sequence = SVR_DESCR; 	/* Normal event */
	    else 
	    {
		printf( "\tServer %d: Received Termination Event\n",
			pcb->pid );
		pcb->sequence     = SVR_RECATCH;  
		global.mode_term  = TRUE;  
	    }
        }
        break;  

    case SVR_DESCR :		/* Get description of event info */
	pcb->api.parm.gDescParm.gd_stmtHandle = pcb->eventHandle;   
	pcb->api.name = "IIapi_getDescriptor()";
	pcb->api.function = IIapi_getDescriptor;

	serverAPICall( pcb, &pcb->api.parm.gDescParm.gd_genParm );
	return;

    case SVR_INFO :		/* Retrieve event info */
	if ( pcb->api_status == IIAPI_ST_SUCCESS )
	{
	    IIAPI_DESCRIPTOR *desc = pcb->api.parm.gDescParm.gd_descriptor;

	    /*
	    ** Make sure results match what is expected:
	    ** single column of type VARCHAR and length
	    ** which will fit in pcb storage (allowing
	    ** room for null terminator).
	    */
	    if (
		 pcb->api.parm.gDescParm.gd_descriptorCount != 1  ||
		 desc[0].ds_dataType != IIAPI_VCH_TYPE  ||
		 desc[0].ds_length >= sizeof( pcb->var1 ) 
	       )
	    {
		pcb->sequence = SVR_RECATCH; 
		break;
	    }

	    pcb->api.parm.gColParm.gc_stmtHandle = pcb->eventHandle;   
	    pcb->api.parm.gColParm.gc_rowCount = 1;   
	    pcb->api.parm.gColParm.gc_columnCount = 1;                
	    pcb->api.parm.gColParm.gc_columnData = &pcb->datavalue;   
	    pcb->api.parm.gColParm.gc_columnData[0].dv_value  = pcb->var1;    
	    pcb->api.name = "IIapi_getColumns()";
	    pcb->api.function = IIapi_getColumns;    

	    serverAPICall( pcb, &pcb->api.parm.gColParm.gc_genParm );
	}
	return;

    case SVR_PROCESS :		/* Display event info */
        if ( pcb->api_status == IIAPI_ST_SUCCESS  &&
	     ! pcb->api.parm.gColParm.gc_columnData[0].dv_null )
        {
	    short	len;

	    memcpy( (char *)&len, pcb->var1, 2 );
	    pcb->var1[ len + 2 ] = '\0';
	    printf( "\tServer %d: Received DB Event: %s\n", 
		    pcb->pid, &pcb->var1[ 2 ] );
        }
        break; 

    case SVR_RECATCH :		/* Re-issue catch event request */
	printf( "\tServer %d: Waiting for DB Event\n",pcb->pid);

	pcb->api.cEventParm.ce_connHandle = global.svr_connHandle; 
	pcb->api.cEventParm.ce_selectEventName = IIdemo_eventName;  
	pcb->api.cEventParm.ce_selectEventOwner = NULL;    
	pcb->api.cEventParm.ce_eventHandle = pcb->eventHandle;    
        pcb->sequence = SVR_NEW; 
	pcb->api.name = "IIapi_catchEvent()";
	pcb->api.function =  IIapi_catchEvent;

	serverAPICall( pcb, &pcb->api.cEventParm.ce_genParm );
	return;

    case SVR_CLOSE :		/* Close event handle */
	pcb->api.parm.close.cl_stmtHandle = pcb->eventHandle;  
	pcb->api.name = "IIapi_close()";
	pcb->api.function =  IIapi_close;     

	serverAPICall( pcb, &pcb->api.parm.close.cl_genParm );
	return;

    case SVR_DONE :		/* Server is done */
        printf( "\tServer %d: Done \n", pcb->pid );
        return; 
	  
    default :			/* Programming error */
        printf( "\tServer %d: Invalid server state %d\n", pcb->pid,
		pcb->sequence - 1 );
	IIdemo_set_abort(); 
        return; 
    }

    goto top;
}


/*
** Name: serverAPICall
**
** Description:
**	Initiates the processing of an async OpenAPI request.
*/

static void 
serverAPICall( SVR_PCB *pcb, IIAPI_GENPARM *genParm )
{
    if ( global.mode_verbose )  
	printf( "\t\tServer: calling %s\n", pcb->api.name );

    genParm->gp_callback = serverCallback; 
    genParm->gp_closure  = (II_PTR)pcb; 
    IIdemo_start_async();
    pcb->api.function( genParm );

    return;
}

/*
** Name: serverCallback
**
** Description:
**	Completes the processing of an async OpenAPI request.
**	Returns control to the FSM processing function.
*/

II_FAR II_CALLBACK II_VOID
serverCallback( II_PTR arg, II_PTR parm )
{
    SVR_PCB		*pcb = (SVR_PCB *)arg;
    IIAPI_GENPARM	*genParm = (IIAPI_GENPARM *)parm;

    if ( global.mode_verbose )  
	printf( "\t\tServer: %s completed\n", pcb->api.name );

    IIdemo_complete_async(); 
    IIdemo_checkError( pcb->api.name, genParm ); 
    pcb->api_status = genParm->gp_status; 
    IIdemo_process_server( pcb );

    return;
}


/*
** Name: eventCallback
**
** Description:
**	OpenAPI invokes this function to send information on database
**      events which failed to match an existing event handle. 
*/

II_FAR II_CALLBACK II_VOID
eventCallback( IIAPI_EVENTPARM *parm )
{
    printf( "\tServer: missed event %s,%s,%s\n",
	    parm->ev_eventDB, parm->ev_eventOwner, parm->ev_eventName );
    IIdemo_set_abort();             /* Abort operation */
    return;
}


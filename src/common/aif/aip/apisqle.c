/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apisql.h>
# include <apichndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apidatav.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisqle.c
**
** Description:
**	SQL Database Event State Machine.
**
** History:  Original history found in apiestbl.c (now defunct).
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	 3-Jul-97 (gordy)
**	    Re-worked state machine initialization.  Added
**	    IIapi_sql_dinit().
**	 3-Sep-98 (gordy)
**	    Added abort state to sm interface.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_close().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
*/

/*
** Forward references
*/

static IIAPI_SM_OUT     *sm_evaluate( IIAPI_EVENT, IIAPI_STATE, IIAPI_HNDL *,
				      IIAPI_HNDL *, II_PTR, IIAPI_SM_OUT * );

static II_BOOL          sm_execute( IIAPI_ACTION, 
				    IIAPI_HNDL *, IIAPI_HNDL *, II_PTR );

static II_BOOL		SetEventParam( char *, char ** );


/*
** Information to be exported through
** the state machine interface.
*/

/*
** SQL DBEvent States.
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
**
** Note that the IDLE state must be 0 to
** match IIAPI_IDLE.
*/

# define SQL_ES_IDLE	((IIAPI_STATE)  0)	/* Idle */
# define SQL_ES_WDBE	((IIAPI_STATE)  1)	/* Waiting for DBEvent */
# define SQL_ES_DONE	((IIAPI_STATE)  2)	/* DBEvent received */
# define SQL_ES_CNCL	((IIAPI_STATE)  3)	/* Cancelled */

# define SQL_ES_CNT	(SQL_ES_CNCL + 1)

static char *sql_ds_id[] =
{
    "IDLE", "WDBE", "DONE", "CNCL"
};


/*
** SQL DBEvent actions
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
*/

# define SQL_EA_REMC	((IIAPI_ACTION)  0)	/* Remember callback */
# define SQL_EA_DBEV	((IIAPI_ACTION)  1)	/* Return event info */
# define SQL_EA_DESC	((IIAPI_ACTION)  2)	/* Return descriptor */
# define SQL_EA_DATA	((IIAPI_ACTION)  3)	/* Return data */
# define SQL_EA_CNCL	((IIAPI_ACTION)  4)	/* Cancel and event handle */
# define SQL_EA_REDO	((IIAPI_ACTION)  5)	/* Restart an event handle */
# define SQL_EA_MOVE    ((IIAPI_ACTION)  6)	/* Move event handle to Conn */
# define SQL_EA_DECR	((IIAPI_ACTION)  7)	/* Decrement notify count */
# define SQL_EA_DELH	((IIAPI_ACTION)  8)	/* Delete event handle */
# define SQL_EA_ERIN	((IIAPI_ACTION)  9)	/* Incomplete query */
# define SQL_EA_ERDN	((IIAPI_ACTION) 10)	/* Query complete */
# define SQL_EA_ERIF	((IIAPI_ACTION) 11)	/* Invalid API function */
# define SQL_EA_CBOK	((IIAPI_ACTION) 12)	/* Callback Success */
# define SQL_EA_CBWN	((IIAPI_ACTION) 13)	/* Callback Warning */
# define SQL_EA_CBFL	((IIAPI_ACTION) 14)	/* Callback Failure */
# define SQL_EA_CBND	((IIAPI_ACTION) 15)	/* Callback No Data */

# define SQL_EA_CNT	(SQL_EA_CBND + 1)

static char *sql_da_id[] =
{
    "REMC", "DBEV", "DESC", "DATA", 
    "CNCL", "REDO", "MOVE", "DECR", 
    "DELH", "ERIN", "ERDN", "ERIF", 
    "CBOK", "CBWN", "CBFL", "CBND"
};


/*
** Name: sql_dbev_sm
**
** Description:
**      The SQL Database Event State Machine Interface
*/

static IIAPI_SM sql_dbev_sm =
{
    "SQL Dbev",
    SQL_ES_CNT,
    sql_ds_id,
    SQL_EA_CNT,
    sql_da_id,
    sm_evaluate,
    sm_execute,
    SQL_ES_DONE
};



/*
** Name: sql_act_seq
**
** Description:
**      Action sequences define the groups of
**      actions which occur at state transitions.
**      An action sequence may be used by several
**      state transitions, so the next state
**      value is not defined here.  The SM_OUT
**      structure is used for conveniance in
**      assigning the action sequences during
**      evaluation.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

static IIAPI_SM_OUT sql_act_seq[] =
{
    { 0, 1, { SQL_EA_REMC, 0, 0 } },				/* EAS_0  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_DELH, SQL_EA_CBOK } },	/* EAS_1  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_ERIN, SQL_EA_CBFL } },	/* EAS_2  */
    { 0, 3, { SQL_EA_DBEV, SQL_EA_CBOK, SQL_EA_DECR } },	/* EAS_3  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_DATA, SQL_EA_CBOK } },	/* EAS_4  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_DESC, SQL_EA_CBOK } },	/* EAS_5  */
    { 0, 3, { SQL_EA_CNCL, SQL_EA_REMC, SQL_EA_CBOK } },	/* EAS_6  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_ERDN, SQL_EA_CBFL } },	/* EAS_7  */
    { 0, 2, { SQL_EA_REMC, SQL_EA_CBWN, 0 } },			/* EAS_8  */
    { 0, 2, { SQL_EA_REMC, SQL_EA_REDO, 0 } },			/* EAS_9  */
    { 0, 3, { SQL_EA_REMC, SQL_EA_REDO, SQL_EA_MOVE } },	/* EAS_10 */
    { 0, 2, { SQL_EA_REMC, SQL_EA_CBND, 0 } },			/* EAS_11 */
    { 0, 3, { SQL_EA_REMC, SQL_EA_ERIF, SQL_EA_CBFL } },	/* EAS_12 */
};

/*
** Indexes into the action sequence array.
*/

# define SQL_EAS_0      0
# define SQL_EAS_1	1
# define SQL_EAS_2	2
# define SQL_EAS_3	3
# define SQL_EAS_4	4
# define SQL_EAS_5	5
# define SQL_EAS_6	6
# define SQL_EAS_7	7
# define SQL_EAS_8	8
# define SQL_EAS_9	9
# define SQL_EAS_10	10
# define SQL_EAS_11	11
# define SQL_EAS_12	12

/*
** Name: smt_list
**
** Description:
**      State machine transitions define the
**      next state and action sequence for
**      static evaluations (transitions which
**      depend solely on the current state
**      and input event).
**
**      These transitions entries are used
**      to populate a sparse matrix for quick
**      access via the current state and event.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_close().
*/

typedef struct
{

    IIAPI_EVENT         smt_event;
    IIAPI_STATE         smt_state;
    IIAPI_STATE         smt_next;
    IIAPI_ACTION        smt_action;

} SM_TRANSITION;

static SM_TRANSITION smt_list[] =
{
    /*
    ** API Functions permitted with DBEvents.
    */
    { IIAPI_EV_CANCEL_FUNC,	SQL_ES_WDBE,	SQL_ES_CNCL,	SQL_EAS_6  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_ES_DONE,	SQL_ES_DONE,	SQL_EAS_7  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_ES_CNCL,	SQL_ES_CNCL,	SQL_EAS_8  },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_ES_IDLE, 	SQL_ES_WDBE,	SQL_EAS_0  },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_ES_DONE,	SQL_ES_WDBE,	SQL_EAS_10 },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_ES_CNCL,	SQL_ES_WDBE,	SQL_EAS_9  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_ES_IDLE,	SQL_ES_IDLE,	SQL_EAS_1  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_ES_WDBE,	SQL_ES_WDBE,	SQL_EAS_2  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_ES_DONE,	SQL_ES_IDLE,	SQL_EAS_1  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_ES_CNCL,	SQL_ES_IDLE,	SQL_EAS_1  },

    /*
    ** Database Event messages are actually processed
    ** by the SQL Connection state machine.  Database
    ** events are then re-dispatched against DBEvent
    ** handles using the same API event as the orignal
    ** message.  The two events are distinguished by
    ** their associated event handles.
    */
    { IIAPI_EV_EVENT_RCVD,	SQL_ES_WDBE,	SQL_ES_DONE,	SQL_EAS_3  }
};



/*
** Name: smt_array
**
** Description:
**      Two dimensional array used for static state
**      transition evaluations.  The array provides
**      quick access based on the current state and
**      event.  The separation of the array and the
**      state transition entry list is primarily for
**      ease of maintenance.  Initialization occurs
**      at the first evaluation.
**
**      The array is sparsly populated; missing
**      entries are either dynamically evaluated
**      transitions or invalid events.
*/

static SM_TRANSITION    *smt_array[ IIAPI_EVENT_CNT ][ SQL_ES_CNT ] ZERO_FILL;
static II_BOOL          initialized = FALSE;



/*
** Name: IIapi_sql_dinit
**
** Description:
**	Initialize the SQL DBEvent state machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS		API status.
**
** History:
**	 3-Jul-97 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_sql_dinit( VOID )
{
    if ( ! initialized )
    {
	i4 i;

	/*
	** Fill in the transition array from the transition list.
	*/
	for( i = 0; i < (sizeof( smt_list ) / sizeof( smt_list[0] )); i++ )
	smt_array[ smt_list[i].smt_event ]
		 [ smt_list[i].smt_state ] = &smt_list[i];

	initialized = TRUE;
    }

    IIapi_sm[ IIAPI_SMT_SQL ][ IIAPI_SMH_DBEV ] = &sql_dbev_sm;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: sm_evaluate
**
** Description:
**	Evaluation function for the Database Event State Machine
**
**	Evaluates input (state, event) and returns output (next
**	state, actions).  A NULL return value indicates the event
**	is to be ignored.
**
**	An output buffer is provided by the caller so the output
**	can be built dynamically (return value identical to the
**	parameter).  The output buffer may be ignored and a static
**	buffer returned instead.
**	
**	Evaluations which are solely dependent on the input are
**	done statically through a sparse two dimensional array.
**	If no entry is found in the array, dynamic evaluations
**	(dependent on addition information in event handles or 
**	parameter block) are performed.  If neither evaluation
**	produces output, default error handling evaluations
**	are done.
**
** Input:
**	state		Current state.
**	event		API event.
**	ev_hndl		Handle associated with event.
**	sm_hndl		State machine instance handle.
**	parmBlock	Parameter block associated with event.
**
** Output:
**	smo		Buffer for state machine output, may be ignored.
**
** Returns:
**	IIAPI_SM_OUT *	State machine output, may be NULL.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

static IIAPI_SM_OUT *
sm_evaluate
(
    IIAPI_EVENT         event,
    IIAPI_STATE         state,
    IIAPI_HNDL          *ev_hndl,
    IIAPI_HNDL          *sm_hndl,
    II_PTR              parmBlock,
    IIAPI_SM_OUT        *smo_buff
)
{
    IIAPI_DBEVHNDL	*dbevHndl = (IIAPI_DBEVHNDL *)sm_hndl;
    IIAPI_SM_OUT	*smo = NULL;
    SM_TRANSITION	*smt;
    IIAPI_DBEVCB	*dbevCB;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "%s evaluate: evaluating event %s in state %s\n",
	  sql_dbev_sm.sm_id, IIAPI_PRINTEVENT( event ),
	  IIAPI_PRINT_ID( state, SQL_ES_CNT, sql_ds_id ) );

    /*
    ** Static evaluations depend solely on the current state and event.
    */
    if ( (smt = smt_array[ event ][ state ]) )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "%s evaluate: static evaluation\n", sql_dbev_sm.sm_id );

	smo = smo_buff;
	STRUCT_ASSIGN_MACRO( sql_act_seq[ smt->smt_action ], *smo );
	smo->smo_next_state = smt->smt_next;
    }
    else                /* Dynamic evaluations require additional predicates */
    switch( event )     /* to determine the correct state transition.        */
    {
	/*
	** IIapi_getColumns() and IIapi_getDescriptor() 
	** either return the requested data or a NO_DATA
	** status.
	*/
	case IIAPI_EV_GETCOLUMN_FUNC :
	    switch( state )
	    {
		case SQL_ES_DONE :
		    smo = smo_buff;
		    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;

		    if ( dbevHndl->eh_done  ||  ! dbevCB->ev_count )
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_EAS_11 ], *smo );
		    else
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_EAS_4 ], *smo );

		    smo->smo_next_state = state;
		    break;
	    }
	    break;

	case IIAPI_EV_GETDESCR_FUNC :
	    switch( state )
	    {
		case SQL_ES_DONE :
		    smo = smo_buff;
		    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;

		    if ( dbevHndl->eh_done  ||  ! dbevCB->ev_count )
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_EAS_11 ], *smo );
		    else
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_EAS_5 ], *smo );

		    smo->smo_next_state = state;
		    break;
	    }
	    break;
    }

    /*
    ** If a specific response was not generated above,
    ** produce a generalized error response.
    */
    if ( ! smo )
    {
        smo = smo_buff;

        if ( event <= IIAPI_EVENT_FUNC_MAX )
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: API function called in wrong state\n",
                  sql_dbev_sm.sm_id );

            STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_EAS_12 ], *smo );
            smo->smo_next_state = state;
        }
        else
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: unexpected I/O completion\n",
                  sql_dbev_sm.sm_id );

            smo->smo_next_state = state;	/* Ignore the error */
	    smo->smo_action_cnt = 0;
        }
    }

    /*
    ** If the input resulted in no state change
    ** and no actions to be executed, we can
    ** just ignore the event.
    */
    if ( smo->smo_next_state == state  &&  ! smo->smo_action_cnt )
    {
        IIAPI_TRACE( IIAPI_TR_DETAIL )
            ( "%s evaluate: nothing to do, transition ignored\n",
              sql_dbev_sm.sm_id );
        smo = NULL;
    }

    return( smo );
}


/*
** Name: sm_execute
**
** Description:
**	Action execution function for the Database Event State Machine
**
**	Executes and output action as defined by the sm_evaluate().
**	A FALSE return value indicates that any remaining output
**	actions should be skipped and the event dispatching should
**	be aborted.
**
** Input:
**	action		Action ID.
**	ev_hndl		Handle associated with event.
**	sm_hndl		State machine instance handle.
**	parmBlock	Parameter block associated with event.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE if event should be cancelled.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
**       9-Aug-99 (rajus01)
**          Asynchronous behaviour of API requires the removal of 
**          handles marked for deletion from the queue. ( Bug #98303 )
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
*/

static II_BOOL
sm_execute
(
    IIAPI_ACTION        action,
    IIAPI_HNDL          *ev_hndl,
    IIAPI_HNDL          *sm_hndl,
    II_PTR              parmBlock
)
{
    IIAPI_DBEVHNDL		*dbevHndl = (IIAPI_DBEVHNDL *)sm_hndl;
    IIAPI_CONNHNDL	 	*connHndl;
    IIAPI_DBEVCB		*dbevCB;
    IIAPI_CATCHEVENTPARM	*catchEventParm;
    IIAPI_GETCOLPARM		*getColParm;
    IIAPI_GETDESCRPARM  	*getDescrParm;
    IIAPI_STATUS		status;
    II_BOOL			success = TRUE;
    i4				i;

    switch( action )
    {
	case SQL_EA_REMC :
	    /*
	    ** Remember callback.
	    */
	    dbevHndl->eh_callback = TRUE;
	    dbevHndl->eh_parm = (IIAPI_GENPARM *)parmBlock;
	    break;

	case SQL_EA_DBEV :
	    /*
	    ** Return event info.
	    */
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
	    catchEventParm = (IIAPI_CATCHEVENTPARM *)dbevHndl->eh_parm;

	    /*
	    ** The actual data continues to reside in the
	    ** database event control block and is simply
	    ** referrenced by the catch event parameters.
	    */
	    catchEventParm->ce_eventName = dbevCB->ev_name;
	    catchEventParm->ce_eventOwner = dbevCB->ev_owner;
	    catchEventParm->ce_eventDB = dbevCB->ev_database;
	    MEcopy( (PTR)&dbevCB->ev_time, sizeof(dbevCB->ev_time), 
		    (PTR)&catchEventParm->ce_eventTime );
	    catchEventParm->ce_eventInfoAvail = (dbevCB->ev_count > 0);
	    break;

	case SQL_EA_DESC :
	    /*
	    ** Return descriptor.
	    */
	    getDescrParm = (IIAPI_GETDESCRPARM *)parmBlock;
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
	    getDescrParm->gd_descriptorCount = dbevCB->ev_count;
	    getDescrParm->gd_descriptor = dbevCB->ev_descriptor;
	    break;

	case SQL_EA_DATA :
	    /*
	    ** Return data.
	    */
	    getColParm = (IIAPI_GETCOLPARM *)parmBlock;
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
    
	    for( i = 0; i < dbevCB->ev_count; i++ )
		if ( dbevCB->ev_data[ i ].dv_null )
		{
		    getColParm->gc_columnData[ i ].dv_null = TRUE;
		    getColParm->gc_columnData[ i ].dv_length = 0;
		}
		else
		{
		    getColParm->gc_columnData[ i ].dv_null = FALSE;
		    getColParm->gc_columnData[ i ].dv_length = 
					    dbevCB->ev_data[ i ].dv_length;

		    MEcopy( dbevCB->ev_data[ i ].dv_value,
			    dbevCB->ev_data[ i ].dv_length,
			    getColParm->gc_columnData[ i ].dv_value );
		}

	    getColParm->gc_rowsReturned = 1;
	    getColParm->gc_moreSegments = FALSE;
	    dbevHndl->eh_done = TRUE;
	    break;

	case SQL_EA_CNCL :
	    /*
	    ** Cancel an event handle.
	    **
	    ** The event handle is on a queue of the connection handle
	    ** waiting for a matching database event.  Since there is
	    ** no place to move the event handle, we will flag it as
	    ** cancelled so subsequent database events will ignore it.
	    */
	    dbevHndl->eh_cancelled = TRUE;

	    /*
	    ** Callback original request with 'query cancelled'.
	    */
	    if ( dbevHndl->eh_callback )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "%s: DB Event cancelled\n", sql_dbev_sm.sm_id );

		if ( ! IIapi_localError( sm_hndl, E_AP0009_QUERY_CANCELLED, 
					 II_SS5000R_RUN_TIME_LOGICAL_ERROR,
					 IIAPI_ST_FAILURE ) )
		{
		    status = IIAPI_ST_OUT_OF_MEMORY;
		    success = FALSE;
		    break;
		}

		IIapi_appCallback(dbevHndl->eh_parm, sm_hndl, IIAPI_ST_FAILURE);
		dbevHndl->eh_callback = FALSE;
	    }
	    break;

	case SQL_EA_REDO :
	    /*
	    ** Restart a completed or cancelled database event handle.
	    */
	    catchEventParm = (IIAPI_CATCHEVENTPARM *)parmBlock;

	    /*
	    ** The event selection criteria should not
	    ** be changing, but just in case . . .
	    */
	    if ( ! SetEventParam( catchEventParm->ce_selectEventName, 
				  &dbevHndl->eh_dbevName )  ||
		 ! SetEventParam( catchEventParm->ce_selectEventOwner, 
				  &dbevHndl->eh_dbevOwner ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    dbevHndl->eh_cancelled = FALSE;
	    dbevHndl->eh_done = FALSE;
	    break;

	case SQL_EA_MOVE :
	    /*
	    ** Move event handle from control block 
	    ** queue to connection handle queue.
	    */
	    catchEventParm = (IIAPI_CATCHEVENTPARM *)parmBlock;
	    connHndl = (IIAPI_CONNHNDL *)catchEventParm->ce_connHandle;
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;

	    QUremove( (QUEUE *)dbevHndl );
	    dbevHndl->eh_parent = connHndl;
	    QUinsert( (QUEUE *)dbevHndl, &connHndl->ch_dbevHndlList );

	    /*
	    ** If this is the last event handle on the event 
	    ** control block, the event control block should 
	    ** be deleted.  This can be done by removing the 
	    ** event handle (already done) and calling 
	    ** IIapi_processDbevCB().
	    */
	    if ( IIapi_isQueEmpty( &dbevCB->ev_dbevHndlList ) )
		IIapi_processDbevCB( dbevCB->ev_connHndl );
	    break;

	case SQL_EA_DECR :
	    /*
	    ** Decrement the notify count.
	    */
	    dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
	    dbevCB->ev_notify = max( 0, dbevCB->ev_notify - 1 );

	    /*
	    ** When the notify count drops to 0, 
	    ** dispatching of the database event 
	    ** has completed and additional events 
	    ** may be dispatched.
	    */
	    if ( ! dbevCB->ev_notify )  
		IIapi_processDbevCB( dbevCB->ev_connHndl );
	    break;

	case SQL_EA_DELH :
	    /*
	    ** Mark handle for deletion.
	    */
	    QUremove( (QUEUE *)dbevHndl );
	    sm_hndl->hd_delete = TRUE;
	    break;

	case SQL_EA_ERIN :
	    /*
	    ** Incomplete query.
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s: DB Event active - must be cancelled\n", 
		  sql_dbev_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0007_INCOMPLETE_QUERY, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_EA_ERDN :
	    /*
	    ** Query complete.
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s: DB Event done - cannot be cancelled\n", 
		  sql_dbev_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0008_QUERY_DONE, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;
    
	case SQL_EA_ERIF :
	    /*
	    ** Invalid API function.
	    */
	    if ( ! IIapi_localError( sm_hndl, E_AP0006_INVALID_SEQUENCE,
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_EA_CBOK :
	    /*
	    ** Callback with success.
	    */
	    if ( dbevHndl->eh_callback )
	    {
		/*
		** Unlike other handles, event handles
		** do not pick up errors that may have
		** been generated from other places.
		** We only want to return 'success', so
		** don't use the event handle for the
		** callback.
		*/
		IIapi_appCallback(dbevHndl->eh_parm, NULL, IIAPI_ST_SUCCESS);
		dbevHndl->eh_callback = FALSE;
	    }
	    break;

	case SQL_EA_CBWN :
	    /*
	    ** Callback with warning.
	    */
	    if ( dbevHndl->eh_callback )
	    {
		/*
		** We only want to return the warning so don't
		** reference the event handle in the callback.
		** This way we avoid returning any errors or
		** other messages which may be sitting on the
		** event handle.
		*/
		IIapi_appCallback( dbevHndl->eh_parm, NULL, IIAPI_ST_WARNING );
		dbevHndl->eh_callback = FALSE;
	    }
	    break;

	case SQL_EA_CBFL :
	    /*
	    ** Callback with failure.
	    */
	    if ( dbevHndl->eh_callback )
	    {
		IIapi_appCallback(dbevHndl->eh_parm, sm_hndl, IIAPI_ST_FAILURE);
		dbevHndl->eh_callback = FALSE;
   	    } 
	    break;

	case SQL_EA_CBND :
	    /*
	    ** Callback with no data.
	    */
	    if ( dbevHndl->eh_callback )
	    {
		IIapi_appCallback(dbevHndl->eh_parm, sm_hndl, IIAPI_ST_NO_DATA);
		dbevHndl->eh_callback = FALSE;
   	    } 
	    break;
    }

    /*
    ** If we couldn't complete the action, callback with failure.
    */
    if ( ! success  &&  dbevHndl->eh_callback )
    {
	IIapi_appCallback( dbevHndl->eh_parm, sm_hndl, status );
	dbevHndl->eh_callback = FALSE;
    }

    return( success );
}




/*
** Name: SetEventParam
**
** Description:
**	Updates the value of an event parameter to match a new value.
**	The original parameter may be freed, (re)allocated, or left
**	as is.
**
** Input:
**	str		New value for parameter.
**	eventParm	Address of original parameter.
**
** Output:
**	eventParm	Updated parameter value.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE if memory allocation failure.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

static II_BOOL
SetEventParam( char *str, char **eventParm )
{
    if ( ! str  &&  *eventParm )
    {
	MEfree( (PTR)*eventParm );
	*eventParm = NULL;
    }
    else  if ( str  &&  ( ! *eventParm  ||  STcompare( str, *eventParm ) ) )
    {
	if ( *eventParm )  MEfree( (PTR)*eventParm );
	if ( ! ( *eventParm = STalloc( str ) ) )  return( FALSE );
    }

    return( TRUE );
}


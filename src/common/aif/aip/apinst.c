/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apins.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apisphnd.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apinst.c
**
** Description:
**	Name Server Transaction State Machine.
**
** History:
**	12-Feb-97 (gordy)
**	    Created.
**	 3-Jul-97 (gordy)
**	    Re-worked state machine initialization.  Added IIapi_ns_tinit().
**	 4-Sep-98 (gordy)
**	    Added ABORT state to handle connection aborts better.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_commit()
**	    and IIapi_rollback().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Mar-10 (gordy)
**	    Replaced formatted receive buffer with combined
**	    send/receive byte stream buffer.
*/

/*
** Forward references
*/

static IIAPI_SM_OUT	*sm_evaluate( IIAPI_EVENT, IIAPI_STATE, IIAPI_HNDL *, 
				      IIAPI_HNDL *, II_PTR, IIAPI_SM_OUT * );

static II_BOOL		sm_execute( IIAPI_ACTION, IIAPI_HNDL *, 
				    IIAPI_HNDL *, II_PTR parmBlock );

/*
** Information to be exported through
** the state machine interface.
*/

/*
** Name Server Transaction States.
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

# define NS_TS_IDLE	((IIAPI_STATE)  0)	/* Idle */
# define NS_TS_AUTO	((IIAPI_STATE)  1)	/* Autocommit transaction */
# define NS_TS_ABORT	((IIAPI_STATE)  2)	/* Transaction aborted */

# define NS_TS_CNT	(NS_TS_ABORT + 1)

static char *ns_ts_id[] =
{
    "IDLE", "AUTO", "ABORT"
};

/*
** Name Server Transaction actions
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
*/

# define NS_TA_REMC	((IIAPI_ACTION)  0)	/* Remember callback */
# define NS_TA_DELH	((IIAPI_ACTION)  1)	/* Delete transaction handle */
# define NS_TA_CBOK	((IIAPI_ACTION)  2)	/* Callback Success */
# define NS_TA_CBIF	((IIAPI_ACTION)  3)	/* Callback Invalid Function */
# define NS_TA_RCVE	((IIAPI_ACTION)  4)	/* Receive Error */

# define NS_TA_CNT	(NS_TA_RCVE + 1)

static char *ns_ta_id[] =
{
    "REMC", "DELH", "CBOK", "CBIF", "RCVE"
};

/*
** Name: ns_tran_sm
**
** Description:
**	The Name Server Transaction State Machine Interface
*/

static IIAPI_SM ns_tran_sm =
{
    "NS Tran",
    NS_TS_CNT,
    ns_ts_id,
    NS_TA_CNT,
    ns_ta_id,
    sm_evaluate,
    sm_execute,
    NS_TS_ABORT
};


/*
** Name: ns_act_seq
**
** Description:
**	Action sequences define the groups of
**	actions which occur at state transitions.
**	An action sequence may be used by several
**	state transitions, so the next state
**	value is not defined here.  The SM_OUT
**	structure is used for conveniance in
**	assigning the action sequences during
**	evaluation.
**
** History:
**	12-Feb-97 (gordy)
**	    Created.
*/

static IIAPI_SM_OUT ns_act_seq[] =
{
    { 0, 0, { 0, 0, 0 } },					/* TAS_0  */
    { 0, 2, { NS_TA_REMC, NS_TA_CBOK, 0 } },			/* TAS_1  */
    { 0, 3, { NS_TA_REMC, NS_TA_DELH, NS_TA_CBOK } },		/* TAS_2  */
    { 0, 1, { NS_TA_CBIF, 0, 0 } },				/* TAS_3  */
    { 0, 1, { NS_TA_RCVE, 0, 0 } }				/* TAS_4  */
};

/*
** Indexes into the action sequence array.
*/

# define NS_TAS_0	0
# define NS_TAS_1	1
# define NS_TAS_2	2
# define NS_TAS_3	3
# define NS_TAS_4	4

/*
** Name: smt_list
**
** Description:
**	State machine transitions define the 
**	next state and action sequence for 
**	static evaluations (transitions which 
**	depend solely on the current state 
**	and input event).
**
**	These transitions entries are used 
**	to populate a sparse matrix for quick 
**	access via the current state and event.
**
** History:
**	12-Feb-97 (gordy)
**	    Created.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_commit()
**	    and IIapi_rollback().
*/

typedef struct
{

    IIAPI_EVENT		smt_event;
    IIAPI_STATE 	smt_state;
    IIAPI_STATE		smt_next;
    IIAPI_ACTION	smt_action;

} SM_TRANSITION;

static SM_TRANSITION smt_list[] =
{
    /*
    ** API Functions which directly affect the transaction state.
    */
    { IIAPI_EV_AUTO_FUNC,	NS_TS_IDLE,	NS_TS_AUTO,	NS_TAS_1  },
    { IIAPI_EV_AUTO_FUNC,	NS_TS_AUTO,	NS_TS_IDLE,	NS_TAS_2  },
    { IIAPI_EV_AUTO_FUNC,	NS_TS_ABORT,	NS_TS_IDLE,	NS_TAS_2  },
    { IIAPI_EV_COMMIT_FUNC,	NS_TS_IDLE,	NS_TS_IDLE,	NS_TAS_2  },
    { IIAPI_EV_COMMIT_FUNC,	NS_TS_ABORT,	NS_TS_IDLE,	NS_TAS_2  },
    { IIAPI_EV_ROLLBACK_FUNC,	NS_TS_IDLE,	NS_TS_IDLE,	NS_TAS_2  },
    { IIAPI_EV_ROLLBACK_FUNC,	NS_TS_ABORT,	NS_TS_IDLE,	NS_TAS_2  },

    /*
    ** The following functions are permitted while transactions, 
    ** are active but do not directly affect the transaction state.
    */
    { IIAPI_EV_CANCEL_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },

    /*
    ** The following messages are ignored during transactions.
    ** They are the results of actions in other state machines.
    */
    { IIAPI_EV_ERROR_RCVD,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_ERROR_RCVD,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_RELEASE_RCVD,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_RELEASE_RCVD,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_GCN_RESULT_RCVD, NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_GCN_RESULT_RCVD, NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_UNEXPECTED_RCVD,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_UNEXPECTED_RCVD,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },

    /*
    ** GCA request completions.
    */
    { IIAPI_EV_RESUME,		NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_RESUME,		NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_RECV_ERROR,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_RECV_ERROR,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },
    { IIAPI_EV_SEND_ERROR,	NS_TS_AUTO,	NS_TS_AUTO,	NS_TAS_0  },
    { IIAPI_EV_SEND_ERROR,	NS_TS_ABORT,	NS_TS_ABORT,	NS_TAS_0  },

};


/*
** Name: smt_array
**
** Description:
**	Two dimensional array used for static state 
**	transition evaluations.  The array provides
**	quick access based on the current state and
**	event.  The separation of the array and the
**	state transition entry list is primarily for
**	ease of maintenance.  Initialization occurs
**	at the first evaluation.
**
**	The array is sparsly populated; missing
**	entries are either dynamically evaluated
**	transitions or invalid events.
*/

static SM_TRANSITION	*smt_array[ IIAPI_EVENT_CNT ][ NS_TS_CNT ] ZERO_FILL;
static II_BOOL		initialized = FALSE;



/*
** Name: IIapi_ns_tinit
**
** Description:
**	Initialize the Name Server Transaction state machine.
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
IIapi_ns_tinit( VOID )
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

    IIapi_sm[ IIAPI_SMT_NS ][ IIAPI_SMH_TRAN ] = &ns_tran_sm;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: sm_evaluate
**
** Description:
**	Evaluation function for the Transaction State Machine
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
**	If no entry is found in the array, default error handling 
**	evaluations are done.
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
**	12-Feb-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Replaced formatted receive buffer with combined
**	    send/receive byte stream buffer.
*/

static IIAPI_SM_OUT *
sm_evaluate
(
    IIAPI_EVENT		event,
    IIAPI_STATE 	state,
    IIAPI_HNDL		*ev_hndl,
    IIAPI_HNDL		*sm_hndl,
    II_PTR		parmBlock,
    IIAPI_SM_OUT	*smo_buff
)
{
    IIAPI_TRANHNDL	*tranHndl = (IIAPI_TRANHNDL *)sm_hndl;
    IIAPI_SM_OUT	*smo = NULL;
    SM_TRANSITION	*smt;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "%s evaluate: evaluating event %s in state %s\n",
	  ns_tran_sm.sm_id, IIAPI_PRINTEVENT( event ),
	  IIAPI_PRINT_ID( state, NS_TS_CNT, ns_ts_id ) );

    /*
    ** Static evaluations depend solely on the current state and event.
    */
    if ( (smt = smt_array[ event ][ state ]) )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "%s evaluate: static evaluation\n", ns_tran_sm.sm_id );

	smo = smo_buff;
	STRUCT_ASSIGN_MACRO( ns_act_seq[ smt->smt_action ], *smo );
	smo->smo_next_state = smt->smt_next;
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
		  ns_tran_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_TAS_3 ], *smo );
	    smo->smo_next_state = state;
	}
        else  if ( event <= IIAPI_EVENT_MSG_MAX )
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: invalid message received\n",
                  ns_tran_sm.sm_id );

            STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_TAS_4 ], *smo );
            smo->smo_next_state = state;
        }
        else
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: unexpected I/O completion\n",
                  ns_tran_sm.sm_id );

            STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_TAS_0 ], *smo );
            smo->smo_next_state = state;
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
              ns_tran_sm.sm_id );
        smo = NULL;
    }

    return( smo );
}



/*
** Name: sm_execute
**
** Description:
**	Action execution function for the Transaction State Machine
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
**	12-Feb-97 (gordy)
**	    Created.
**       9-Aug-99 (rajus01)
**          Asynchronous behaviour of API requires the removal of 
**          handles marked for deletion from the queue. ( Bug #98303 )  
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
    IIAPI_TRANHNDL      *tranHndl = (IIAPI_TRANHNDL *)sm_hndl;
    IIAPI_SAVEPTHNDL	*savePtHndl;
    IIAPI_SAVEPTPARM    *savePtParm;
    IIAPI_STATUS        status = OK;
    II_BOOL             success = TRUE;
    char		queryText[ 64 ];

    switch( action )
    {
        case NS_TA_REMC :
	    /*
	    ** Remember callback.
	    */
	    tranHndl->th_callback = TRUE;
	    tranHndl->th_parm = (IIAPI_GENPARM *)parmBlock;
	    break;

	case NS_TA_DELH :
	    /*
	    ** Mark handle for deletion.
	    */
	    QUremove( (QUEUE *)tranHndl );
	    sm_hndl->hd_delete = TRUE;
	    break;

	case NS_TA_CBOK :
	    /*
	    ** Callback with success.  Note that
	    ** this will pick up the most severe
	    ** status from the errors associated
	    ** with the event handle.
	    */
	    if ( tranHndl->th_callback )
	    {
		IIapi_appCallback(tranHndl->th_parm, sm_hndl, IIAPI_ST_SUCCESS);
		tranHndl->th_callback = FALSE;
	    }
	    break;

	case NS_TA_CBIF :
	    /*
	    ** API function called in wrong state.
	    */
	    if ( ! IIapi_localError( sm_hndl, E_AP0006_INVALID_SEQUENCE, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
		status = IIAPI_ST_OUT_OF_MEMORY;
	    else
		status = IIAPI_ST_FAILURE;

            /*
            ** This may not have been a transaction
            ** related function, and we may have a
            ** callback saved on the transaction
            ** handle, so we carefully make the
            ** callback to the caller making sure
            ** not to disturb the transaction handle.
            */
            IIapi_appCallback( (IIAPI_GENPARM *)parmBlock, sm_hndl, status );

            /*
            ** We must return the failure here rather
            ** than following the normal exit route
            ** to ensure that the transaction handle
            ** callback does not get made.
            */
            return( FALSE );

	case NS_TA_RCVE :
	    /*
	    ** Receive error.  
	    **
	    ** We have received an invalid message.  
	    ** Since the connection state machine 
	    ** may ignore this particular message
	    ** type, convert to a type which will
	    ** ensure the proper handling in all
	    ** state machines.
	    */
	    IIapi_liDispatch( IIAPI_EV_UNEXPECTED_RCVD, sm_hndl, NULL, NULL );
	    return( FALSE );
    }

    /*
    ** If we couldn't complete the action, callback with failure.
    */
    if ( ! success  &&  tranHndl->th_callback )
    {
        IIapi_appCallback( tranHndl->th_parm, sm_hndl, status );
        tranHndl->th_callback = FALSE;
    }

    return( success );
}


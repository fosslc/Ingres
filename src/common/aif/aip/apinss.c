/*
** Copyright (c) 1993, 2010 Ingres Corporation All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>

# include <iicommon.h>
# include <gcn.h>
# include <generr.h>

# include <iiapi.h>
# include <api.h>
# include <apins.h>
# include <apichndl.h>
# include <apishndl.h>
# include <apiihndl.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apimsg.h>
# include <apidatav.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>
# include <apige2ss.h>

/*
** Name: apinss.c
**
** Description:
**	Name Server Statement State Machine.
**
** History:
**	18-Feb-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    At protocol level 63 Name Server protocol fixed to
**	    handle end-of-group and end-of-data by GCA standards.
**	 3-Jul-97 (gordy)
**	    Re-worked state machine initialization.  Added IIapi_ns_sinit().
**	 3-Sep-98 (gordy)
**	    Added abort state to sm interface.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_close().
**	22-Sep-99 (rajus01)
**	    [Issue Number: 7707961] Added NS_SA_RLSB action.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	20-Feb-03 (gordy)
**	    Fixed the length of data used when returning an object key.
**	 6-Jun-03 (gordy)
**	    Clear query info mask once data has been returned.
**	 8-Apr-04 (gordy)
**	    To be consistent, clear the query info flags along with mask.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Enhanced parameter memory block handling.
*/

/*
** Forward references
*/

static IIAPI_SM_OUT	*sm_evaluate( IIAPI_EVENT, IIAPI_STATE, IIAPI_HNDL *,
				      IIAPI_HNDL *, II_PTR, IIAPI_SM_OUT * );

static II_BOOL		sm_execute( IIAPI_ACTION, IIAPI_HNDL *,
				    IIAPI_HNDL *, II_PTR parmBlock );

static IIAPI_STATUS	ns_set_descr( IIAPI_STMTHNDL *, IIAPI_SETDESCRPARM * );

static II_VOID		ns_read_result( IIAPI_STMTHNDL *, IIAPI_MSG_BUFF * );

static II_BOOL		ns_eval_data( IIAPI_STMTHNDL *, 
				      IIAPI_GETCOLPARM	*, IIAPI_MSG_BUFF * );

II_EXTERN i4	ss_encode( char * );


/*
** Information to be exported through
** the state machine interface.
*/

/*
** Name Server Statement States.
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

# define NS_SS_IDLE	((IIAPI_STATE)  0)	/* Idle */
# define NS_SS_DESC	((IIAPI_STATE)  1)	/* Describe query parameters */
# define NS_SS_PARM	((IIAPI_STATE)  2)	/* Process query parameters */
# define NS_SS_SQRY	((IIAPI_STATE)  3)	/* Sending query to server */
# define NS_SS_SENT	((IIAPI_STATE)  4)	/* Query sent */
# define NS_SS_WQDR	((IIAPI_STATE)  5)	/* Wait query desc response */
# define NS_SS_TUPL	((IIAPI_STATE)  6)	/* Get tuple data */
# define NS_SS_WTR	((IIAPI_STATE)  7)	/* Wait for tuple response */
# define NS_SS_WQR	((IIAPI_STATE)  8)	/* Wait for query response */
# define NS_SS_SMSG	((IIAPI_STATE)  9)	/* Message sent (cancel) */
# define NS_SS_WIR	((IIAPI_STATE) 10)	/* Wait interrupt response */
# define NS_SS_CNCL	((IIAPI_STATE) 11)	/* Cancelled */
# define NS_SS_DONE	((IIAPI_STATE) 12)	/* Done */

# define NS_SS_CNT     (NS_SS_DONE + 1)

static char *ns_ss_id[] =
{
    "IDLE", "DESC", "PARM", "SQRY", "SENT", 
    "WQDR", "TUPL", "WTR",  "WQR",  
    "SMSG", "WIR",  "CNCL", "DONE"
};

/*
** Name Server Statement actions
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
*/

# define NS_SA_REMC	((IIAPI_ACTION)  0)	/* Remember callback */
# define NS_SA_CNCL	((IIAPI_ACTION)  1)	/* Cancel query */
# define NS_SA_RECV	((IIAPI_ACTION)  2)	/* Receive message */
# define NS_SA_RCVB	((IIAPI_ACTION)  3)	/* Receive message buffered */
# define NS_SA_SQRY	((IIAPI_ACTION)  4)	/* Send query */
# define NS_SA_SETD	((IIAPI_ACTION)  5)	/* Save parameter descriptor */
# define NS_SA_SPRM	((IIAPI_ACTION)  6)	/* Save query parameters */
# define NS_SA_RERR	((IIAPI_ACTION)  7)	/* Read error */
# define NS_SA_RRES	((IIAPI_ACTION)  8)	/* Read result */
# define NS_SA_BRES	((IIAPI_ACTION)  9)	/* Read result (buffered) */
# define NS_SA_INFO	((IIAPI_ACTION) 10)	/* Return query info */
# define NS_SA_DESC	((IIAPI_ACTION) 11)	/* Return descriptor */
# define NS_SA_RSLT	((IIAPI_ACTION) 12)	/* Fake GCN_RESULT message */
# define NS_SA_BUFF	((IIAPI_ACTION) 13)	/* Handle GCA receive buffer */
# define NS_SA_QBUF	((IIAPI_ACTION) 14)	/* Queue message buffer */
# define NS_SA_DELH	((IIAPI_ACTION) 15)	/* Delete statement handle */
# define NS_SA_ERQA	((IIAPI_ACTION) 16)	/* Query active */
# define NS_SA_ERQD	((IIAPI_ACTION) 17)	/* Query done */
# define NS_SA_ERQC	((IIAPI_ACTION) 18)	/* Query cancelled */
# define NS_SA_CBOK	((IIAPI_ACTION) 19)	/* Callback Success */
# define NS_SA_CBFL	((IIAPI_ACTION) 20)	/* Callback Failure */
# define NS_SA_CBND	((IIAPI_ACTION) 21)	/* Callback No Data */
# define NS_SA_CBGC	((IIAPI_ACTION) 22)	/* Callback getColumns status */
# define NS_SA_CBIF	((IIAPI_ACTION) 23)	/* Callback Invalid Function */
# define NS_SA_CNOK	((IIAPI_ACTION) 24)	/* Callback Cancel Success */
# define NS_SA_CNWN	((IIAPI_ACTION) 25)	/* Callback Cancel Warning */
# define NS_SA_CNFL	((IIAPI_ACTION) 26)	/* Callback Cancel Failure */
# define NS_SA_RCVE	((IIAPI_ACTION) 27)	/* Receive Error */
# define NS_SA_RLSB	((IIAPI_ACTION) 28)	/* Release Buffer */

# define NS_SA_CNT     (NS_SA_RLSB + 1)

static char *ns_sa_id[] =
{
    "REMC", "CNCL", "RECV", "RCVB", "SQRY", 
    "SETD", "SPRM", "RERR", "RRES", "BRES", 
    "INFO", "DESC", "RSLT", "BUFF", "QBUF",
    "DELH", "ERQA", "ERQD", "ERQC", 
    "CBOK", "CBFL", "CBND", "CBGC", "CBIF", 
    "CNOK", "CNWN", "CNFL", "RCVE", "RLSB"
};

/*
** Name: ns_stmt_sm
**
** Description:
**	The Name Server Statement State Machine Interface
*/

static IIAPI_SM ns_stmt_sm =
{
    "NS Stmt",
    NS_SS_CNT,
    ns_ss_id,
    NS_SA_CNT,
    ns_sa_id,
    sm_evaluate,
    sm_execute,
    NS_SS_DONE
};


/*
** Name: ns_act_seq
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
**	18-Feb-97 (gordy)
**	    Created.
*/

static IIAPI_SM_OUT ns_act_seq[] =
{
    { 0, 0, { 0, 0, 0 } },					/* SAS_0  */
    { 0, 1, { NS_SA_RECV, 0, 0 } },				/* SAS_1  */
    { 0, 2, { NS_SA_REMC, NS_SA_RECV, 0 } },			/* SAS_2  */
    { 0, 3, { NS_SA_REMC, NS_SA_BRES, NS_SA_RECV } },		/* SAS_3  */
    { 0, 2, { NS_SA_REMC, NS_SA_CBND, 0 } },			/* SAS_4  */
    { 0, 2, { NS_SA_REMC, NS_SA_CBOK, 0 } },			/* SAS_5  */
    { 0, 3, { NS_SA_REMC, NS_SA_DELH, NS_SA_CBOK } },		/* SAS_6  */
    { 0, 3, { NS_SA_REMC, NS_SA_ERQA, NS_SA_CBFL } },		/* SAS_7  */
    { 0, 3, { NS_SA_REMC, NS_SA_ERQC, NS_SA_CBFL } },		/* SAS_8  */
    { 0, 3, { NS_SA_REMC, NS_SA_INFO, NS_SA_CBOK } },		/* SAS_9  */
    { 0, 3, { NS_SA_REMC, NS_SA_SETD, NS_SA_CBOK } },		/* SAS_10 */
    { 0, 3, { NS_SA_REMC, NS_SA_SPRM, NS_SA_CBOK } },		/* SAS_11 */
    { 0, 3, { NS_SA_REMC, NS_SA_SPRM, NS_SA_SQRY } },		/* SAS_12 */
    { 0, 2, { NS_SA_REMC, NS_SA_RSLT, 0 } },			/* SAS_13 */
    { 0, 2, { NS_SA_REMC, NS_SA_SQRY, 0 } },			/* SAS_14 */
    { 0, 1, { NS_SA_CNCL, 0, 0 } },				/* SAS_15 */
    { 0, 2, { NS_SA_CNCL, NS_SA_CNOK, 0 } },			/* SAS_16 */
    { 0, 2, { NS_SA_CNCL, NS_SA_RECV, 0 } },			/* SAS_17 */
    { 0, 3, { NS_SA_CNCL, NS_SA_ERQD, NS_SA_CNFL } },		/* SAS_18 */
    { 0, 3, { NS_SA_DESC, NS_SA_BUFF, NS_SA_CBOK } },		/* SAS_19 */
    { 0, 2, { NS_SA_RERR, NS_SA_CBND, 0 } },			/* SAS_20 */
    { 0, 3, { NS_SA_RERR, NS_SA_RLSB, NS_SA_CBGC } },		/* SAS_21 */
    { 0, 3, { NS_SA_RERR, NS_SA_INFO, NS_SA_CBOK } },		/* SAS_22 */
    { 0, 2, { NS_SA_RERR, NS_SA_RECV, 0 } },			/* SAS_23 */
    { 0, 2, { NS_SA_RRES, NS_SA_CBND, 0 } },			/* SAS_24 */
    { 0, 3, { NS_SA_RRES, NS_SA_RLSB, NS_SA_CBGC } },		/* SAS_25 */
    { 0, 3, { NS_SA_RRES, NS_SA_INFO, NS_SA_CBOK } },		/* SAS_26 */
    { 0, 2, { NS_SA_RRES, NS_SA_RCVB, 0 } },			/* SAS_27 */
    { 0, 2, { NS_SA_BUFF, NS_SA_CBOK, 0 } },			/* SAS_28 */
    { 0, 1, { NS_SA_RCVB, 0, 0 } },				/* SAS_29 */
    { 0, 1, { NS_SA_CBOK, 0, 0 } },				/* SAS_30 */
    { 0, 2, { NS_SA_RLSB, NS_SA_CBGC, 0 } },			/* SAS_31 */
    { 0, 1, { NS_SA_CNWN, 0, 0 } },				/* SAS_32 */
    { 0, 3, { NS_SA_CNOK, NS_SA_ERQC, NS_SA_CBFL } },		/* SAS_33 */
    { 0, 1, { NS_SA_CBIF, 0, 0 } },				/* SAS_34 */
    { 0, 1, { NS_SA_RCVE, 0, 0 } },				/* SAS_35 */
    { 0, 2, { NS_SA_QBUF, NS_SA_RECV, 0 } },			/* SAS_36 */
};

/*
** Indexes into the action sequence array.
*/

# define NS_SAS_0	0
# define NS_SAS_1	1
# define NS_SAS_2	2
# define NS_SAS_3	3
# define NS_SAS_4	4
# define NS_SAS_5	5
# define NS_SAS_6	6
# define NS_SAS_7	7
# define NS_SAS_8	8
# define NS_SAS_9	9
# define NS_SAS_10	10
# define NS_SAS_11	11
# define NS_SAS_12	12
# define NS_SAS_13	13
# define NS_SAS_14	14
# define NS_SAS_15	15
# define NS_SAS_16	16
# define NS_SAS_17	17
# define NS_SAS_18	18
# define NS_SAS_19	19
# define NS_SAS_20	20
# define NS_SAS_21	21
# define NS_SAS_22	22
# define NS_SAS_23	23
# define NS_SAS_24	24
# define NS_SAS_25	25
# define NS_SAS_26	26
# define NS_SAS_27	27
# define NS_SAS_28	28
# define NS_SAS_29	29
# define NS_SAS_30	30
# define NS_SAS_31	31
# define NS_SAS_32	32
# define NS_SAS_33	33
# define NS_SAS_34	34
# define NS_SAS_35	35
# define NS_SAS_36	36


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
**	18-Feb-97 (gordy)
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
    ** API Functions which directly affect the statement state.
    */
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_DESC,	NS_SS_CNCL,	NS_SAS_16 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_PARM,	NS_SS_CNCL,	NS_SAS_16 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_SQRY,	NS_SS_SMSG,	NS_SAS_15 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_SENT,	NS_SS_WIR,	NS_SAS_17 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_WQDR,	NS_SS_WIR,	NS_SAS_15 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_TUPL,	NS_SS_WIR,	NS_SAS_17 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_WTR,	NS_SS_WIR,	NS_SAS_15 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_WQR,	NS_SS_WIR,	NS_SAS_15 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_SMSG,	NS_SS_SMSG,	NS_SAS_32 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_WIR,	NS_SS_WIR,	NS_SAS_32 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_32 },
    { IIAPI_EV_CANCEL_FUNC,	NS_SS_DONE,	NS_SS_DONE,	NS_SAS_18 },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_DESC,	NS_SS_DESC,	NS_SAS_7  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_PARM,	NS_SS_PARM,	NS_SAS_7  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_SENT,	NS_SS_SENT,	NS_SAS_7  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_TUPL,	NS_SS_TUPL,	NS_SAS_7  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_IDLE,	NS_SS_IDLE,	NS_SAS_6  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_CNCL,	NS_SS_IDLE,	NS_SAS_6  },
    { IIAPI_EV_CLOSE_FUNC,	NS_SS_DONE,	NS_SS_IDLE,	NS_SAS_6  },
    { IIAPI_EV_GETCOLUMN_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_8  },
    { IIAPI_EV_GETCOLUMN_FUNC,	NS_SS_DONE,	NS_SS_DONE,	NS_SAS_4  },
    { IIAPI_EV_GETDESCR_FUNC,	NS_SS_SENT,	NS_SS_WQDR,	NS_SAS_2  },
    { IIAPI_EV_GETDESCR_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_8  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_DESC,	NS_SS_DESC,	NS_SAS_7  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_PARM,	NS_SS_PARM,	NS_SAS_7  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_SENT,	NS_SS_WQR,	NS_SAS_2  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_TUPL,	NS_SS_WQR,	NS_SAS_3  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_8  },
    { IIAPI_EV_GETQINFO_FUNC,	NS_SS_DONE,	NS_SS_DONE,	NS_SAS_9  },
    { IIAPI_EV_PUTPARM_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_8  },
    { IIAPI_EV_SETDESCR_FUNC,	NS_SS_DESC,	NS_SS_PARM,	NS_SAS_10 },
    { IIAPI_EV_SETDESCR_FUNC,	NS_SS_CNCL,	NS_SS_CNCL,	NS_SAS_8  },

    /*
    ** GCA request completions.
    */
    { IIAPI_EV_SEND_CMPL,	NS_SS_SQRY,	NS_SS_SENT,	NS_SAS_30 },
    { IIAPI_EV_SEND_CMPL,	NS_SS_SMSG,	NS_SS_WIR,	NS_SAS_1  },
    
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

static SM_TRANSITION	*smt_array[ IIAPI_EVENT_CNT ][ NS_SS_CNT ] ZERO_FILL;
static II_BOOL		initialized = FALSE;


/*
** Name: IIapi_ns_sinit
**
** Description:
**	Initialize the Name Server Statement state machine.
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
IIapi_ns_sinit( VOID )
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

    IIapi_sm[ IIAPI_SMT_NS ][ IIAPI_SMH_STMT ] = &ns_stmt_sm;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: sm_evaluate
**
** Description:
**	Evaluation function for the Statement State Machine
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
**	18-Feb-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    At protocol level 63 Name Server protocol fixed to
**	    handle end-of-group and end-of-data by GCA standards.
**	25-Oct-06 (gordy)
**	    Query flags saved as statement flags.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Enhanced parameter memory block handling.
*/

static IIAPI_SM_OUT *
sm_evaluate
(
    IIAPI_EVENT		event,
    IIAPI_STATE		state,
    IIAPI_HNDL		*ev_hndl,
    IIAPI_HNDL		*sm_hndl,
    II_PTR		parmBlock,
    IIAPI_SM_OUT	*smo_buff
)
{
    IIAPI_STMTHNDL	*stmtHndl = (IIAPI_STMTHNDL *)sm_hndl;
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl( sm_hndl );
    IIAPI_GETCOLPARM	*getColParm;
    IIAPI_SM_OUT	*smo = NULL;
    SM_TRANSITION	*smt;
    IIAPI_MSG_BUFF	*msgBuff;
    IIAPI_STATUS	api_status;
    STATUS		status;
    i4			value;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "%s evaluate: evaluating event %s in state %s\n",
	  ns_stmt_sm.sm_id, IIAPI_PRINTEVENT( event ),
	  IIAPI_PRINT_ID( state, NS_SS_CNT, ns_ss_id ) );

    /*
    ** Static evaluations depend solely on the current state and event.
    */
    if ( (smt = smt_array[ event ][ state ]) )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "%s evaluate: static evaluation\n", ns_stmt_sm.sm_id );

	smo = smo_buff;
	STRUCT_ASSIGN_MACRO( ns_act_seq[ smt->smt_action ], *smo );
	smo->smo_next_state = smt->smt_next;
    }
    else		/* Dynamic evaluations require additional predicates */
    switch( event )	/* to determine the correct state transition.        */
    {
	/*
	** Check to see if we have buffered data
	** which may satisfy the request.
	*/
	case IIAPI_EV_GETCOLUMN_FUNC :
	    switch( state )
	    {
		case NS_SS_TUPL :
		    smo = smo_buff;

		    if ( stmtHndl->sh_rcvBuff )
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_13 ], *smo );
		    else
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_2 ], *smo );

		    smo->smo_next_state = NS_SS_WTR;
		    break;
	    }
	    break;
    
	/*
	** Validate NS query request.
	*/
	case IIAPI_EV_QUERY_FUNC :
	    switch( state )
	    {
		case NS_SS_IDLE :
		    smo = smo_buff;
		    api_status = IIapi_parseNSQuery( stmtHndl, &status );

		    if ( api_status != IIAPI_ST_SUCCESS )
		    {
			IIAPI_TRACE( IIAPI_TR_ERROR )
			    ( "%s: Error parsing Name Server query\n",
			      ns_stmt_sm.sm_id );

			IIapi_localError( sm_hndl, status, 
					  II_SS42000_SYN_OR_ACCESSERR, 
					  api_status );

			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_6 ], *smo );
			smo->smo_next_state = NS_SS_IDLE;
		    }
		    else  if ( stmtHndl->sh_flags & IIAPI_SH_PARAMETERS )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_5 ], *smo );
			smo->smo_next_state = NS_SS_DESC;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_14 ], *smo );
			smo->smo_next_state = NS_SS_SQRY;
		    }
		    break;
	    }
	    break;

	/*
	** Validate NS query parameters.
	*/
	case IIAPI_EV_PUTPARM_FUNC :
	    switch( state )
	    {
		case NS_SS_PARM :
		    smo = smo_buff;

		    if ( ! IIapi_validNSParams(stmtHndl, 
					       (IIAPI_PUTPARMPARM *)parmBlock) )
		    {
			IIAPI_TRACE( IIAPI_TR_ERROR )
			    ( "%s: Invalid Name Server query parameter value\n",
			      ns_stmt_sm.sm_id );

			IIapi_localError(sm_hndl, E_AP0011_INVALID_PARAM_VALUE,
					 II_SS22023_INV_PARAM_VAL, 
					 IIAPI_ST_FAILURE);

			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_5 ], *smo );
			smo->smo_next_state = NS_SS_PARM;
		    }
		    else if ( stmtHndl->sh_parmCount >
			      (stmtHndl->sh_parmIndex + stmtHndl->sh_parmSend) )
		    {
			/*
			** More parameters.
			*/
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_11 ], *smo );
			smo->smo_next_state = NS_SS_PARM;
		    }
		    else
		    {
			/*
			** Last parameter.
			*/
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_12 ], *smo );
			smo->smo_next_state = NS_SS_SQRY;
		    }
		    break;
	    }
	    break;

	/*
	** Check to see if current request is complete.
	*/
	case IIAPI_EV_ERROR_RCVD :
	    switch( state )
	    {
		case NS_SS_WQDR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_20 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else  if ( NS_EOD( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_23 ], *smo );
			smo->smo_next_state = state;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_36 ], *smo );
			smo->smo_next_state = state;
		    }
		    break;

		case NS_SS_WTR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_21 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else  if ( NS_EOD( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_23 ], *smo );
			smo->smo_next_state = state;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_36 ], *smo );
			smo->smo_next_state = state;
		    }
		    break;

		case NS_SS_WQR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_22 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else  if ( NS_EOD( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_23 ], *smo );
			smo->smo_next_state = state;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_36 ], *smo );
			smo->smo_next_state = state;
		    }
		    break;

		case NS_SS_WIR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_33 ], *smo );
			smo->smo_next_state = NS_SS_CNCL;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_1 ], *smo );
			smo->smo_next_state = NS_SS_WIR;
		    }
		    break;
	    }
	    break;

	case IIAPI_EV_GCN_RESULT_RCVD :
	    switch( state )
	    {
		case NS_SS_WQDR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    /*
		    ** Figure out what type of results we have.
		    ** We don't return a descriptor until we see
		    ** GCN_RET results.  We accumulate results
		    ** until EOG or GCN_RET received.  We don't
		    ** expect continued messages since this
		    ** should only happen with GCN_RET results
		    ** and that would have taken us into a
		    ** different state.  The savest course
		    ** here is to assume non GCN_RET results
		    ** for continued messages.
		    */ 
		    if ( connHndl->ch_flags & IIAPI_CH_MSG_CONTINUED )
			value = ! GCN_RET;
		    else
			gcu_get_int( msgBuff->buffer, &value );

		    if ( value == GCN_RET )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_19 ], *smo );
			smo->smo_next_state = NS_SS_TUPL;
		    }
		    else  if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_24 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_27 ], *smo );
			smo->smo_next_state = NS_SS_WQDR;
		    }
		    break;

		case NS_SS_WTR :
		    getColParm = (IIAPI_GETCOLPARM *)stmtHndl->sh_parm;
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    /*
		    ** Figure out what type of results we have.
		    ** GCN_RET info is processed like tuples.
		    ** Other result info is simply accumulated
		    ** (we don't actually expect other info).
		    ** We stay in the current state until the
		    ** tuple request is complete or EOG.
		    ** Continued message are a problem since
		    ** we have no way of knowing what type
		    ** of results they are.  Since only GCN_RET
		    ** results return a large amount of data,
		    ** we choose this assumption as being the
		    ** safest.
		    */ 
		    if ( connHndl->ch_flags & IIAPI_CH_MSG_CONTINUED )
			value = GCN_RET;
		    else
		    {
			/*
			** Note that the GCA data pointer may be
			** pointing to the next tuple, but we still
			** need to access the GCN header info at
			** the start of the buffer.
			*/
			gcu_get_int( msgBuff->buffer, &value );
		    }

		    if ( value == GCN_RET )
			if ( ns_eval_data( stmtHndl, getColParm, msgBuff ) )
			{
			    /*
			    ** Request completed.  Even if all the
			    ** tuples in the GCA buffer have been
			    ** processed, we save the buffer since
			    ** we will need to at least check for
			    ** EOG on the next request.
			    */
			    STRUCT_ASSIGN_MACRO(ns_act_seq [NS_SAS_28 ], *smo);
			    smo->smo_next_state = NS_SS_TUPL;
			}
			else  if ( NS_EOG( connHndl, msgBuff ) )
			{
			    /*
			    ** Done.
			    */
			    STRUCT_ASSIGN_MACRO(ns_act_seq[ NS_SAS_31 ], *smo);
			    smo->smo_next_state = NS_SS_DONE;
			}
			else
			{
			    /*
			    ** Get more tuples.  Since the request
			    ** is not complete, we assume all tuples
			    ** in the GCA buffer have been processed.
			    ** Mark the buffer as consumed so that
			    ** no garbage is carried over to the next
			    ** message.
			    */
			    msgBuff->length = 0;
			    STRUCT_ASSIGN_MACRO(ns_act_seq[ NS_SAS_29 ], *smo );
			    smo->smo_next_state = NS_SS_WTR;
			}
		    else  if ( NS_EOG( connHndl, msgBuff ) )
		    {
			/*
			** Done.
			*/
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_25 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else
		    {
			/*
			** Get more results.  The result info
			** accumulation action (SA_RRES) empties
			** the buffer so that no garbage is left
			** over to the next request.
			*/
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_27 ], *smo );
			smo->smo_next_state = NS_SS_WTR;
		    }
		    break;

		case NS_SS_WQR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_26 ], *smo );
			smo->smo_next_state = NS_SS_DONE;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_27 ], *smo );
			smo->smo_next_state = NS_SS_WQR;
		    }
		    break;

		case NS_SS_WIR :
		    msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		    smo = smo_buff;

		    if ( NS_EOG( connHndl, msgBuff ) )
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_33 ], *smo );
			smo->smo_next_state = NS_SS_CNCL;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_1 ], *smo );
			smo->smo_next_state = NS_SS_WIR;
		    }
		    break;
	    }
	    break;

	/*
	** The following events are ignored at the
	** statement level.  They will be processed
	** by the connection state machine.  Since
	** they are unconditionally ignored, it is
	** easier to handle them here than maintaining
	** static transitions for every state.
	*/
	case IIAPI_EV_RELEASE_RCVD :
	case IIAPI_EV_UNEXPECTED_RCVD :
	case IIAPI_EV_RESUME :
	case IIAPI_EV_RECV_ERROR :
	case IIAPI_EV_SEND_ERROR :
	    smo = smo_buff;
	    smo->smo_next_state = state;
	    smo->smo_action_cnt = 0;
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
		  ns_stmt_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_34 ], *smo );
	    smo->smo_next_state = state;
	}
	else  if ( event <= IIAPI_EVENT_MSG_MAX )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s Evaluate: invalid message received\n",
		  ns_stmt_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_35 ], *smo );
	    smo->smo_next_state = state;
	}
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s Evaluate: unexpected I/O completion\n",
		  ns_stmt_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( ns_act_seq[ NS_SAS_0 ], *smo );
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
	      ns_stmt_sm.sm_id );
	smo = NULL;
    }

    return( smo );
}


/*
** Name: sm_execute
**
** Description:
**	Action execution function for the Statement State Machine
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
**	18-Feb-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    At protocol level 63 Name Server protocol fixed to
**	    handle end-of-group and end-of-data by GCA standards.
**       9-Aug-99 (rajus01)
**          Asynchronous behaviour of API requires the removal of 
**          handles marked for deletion from the queue. ( Bug #98303 )  
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	20-Feb-03 (gordy)
**	    Fixed the length of data used when returning an object key.
**	 6-Jun-03 (gordy)
**	    Clear query info mask once data has been returned.
**	 8-Apr-04 (gordy)
**	    To be consistent, clear the query info flags along with mask.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Enhanced parameter memory block handling.
*/

static II_BOOL
sm_execute
(
    IIAPI_ACTION	action,
    IIAPI_HNDL		*ev_hndl,
    IIAPI_HNDL		*sm_hndl,
    II_PTR		parmBlock
)
{
    IIAPI_STMTHNDL	*stmtHndl = (IIAPI_STMTHNDL *)sm_hndl;
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl( sm_hndl );
    IIAPI_STATUS	status;
    II_BOOL		success = TRUE;

    switch( action )
    {
	case NS_SA_REMC :
	    /*
	    ** Remember callback.
	    */
	    stmtHndl->sh_callback = TRUE;
	    stmtHndl->sh_parm = (IIAPI_GENPARM *)parmBlock;
	    break;

	case NS_SA_CNCL :
	    /*
	    ** Remember cancel callback.
	    */
	    stmtHndl->sh_cancelled = TRUE;
	    stmtHndl->sh_cancelParm = (IIAPI_GENPARM *)parmBlock;
	    break;
    
	case NS_SA_RECV :
	{
	    /*
	    ** Issue receive message request.
	    ** Allocate new GCA buffer for
	    ** unformatted receives.
	    */
	    IIAPI_MSG_BUFF *msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)
							    sm_hndl );

	    if ( ! msgBuff )
		status = IIAPI_ST_OUT_OF_MEMORY;
	    else
		status = IIapi_rcvNormalGCA( sm_hndl, msgBuff, (II_LONG)(-1) );

	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case NS_SA_RCVB :
	{
	    /*
	    ** Issue receive message request 
	    ** using current GCA buffer.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    status = IIapi_rcvNormalGCA( sm_hndl, msgBuff, (II_LONG)(-1) );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case NS_SA_SQRY :
	{
	    /*
	    ** Format and send GCN_NS_OPER message.
	    */
	    IIAPI_MSG_BUFF *msgBuff;

	    if ( ! (msgBuff = IIapi_createGCNOper( stmtHndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case NS_SA_SPRM :
	    /*
	    ** Save query parameters.
	    */
	    IIapi_saveNSParams( stmtHndl, (IIAPI_PUTPARMPARM *)parmBlock );
	    break;

	case NS_SA_RERR :
	{
	    /*
	    ** Read GCA_ERROR message and save on statement handle.
	    **
	    ** Message buffer is reserved when placed on
	    ** the receive queue.  It will be released
	    ** when removed from the queue and processed.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    IIapi_reserveMsgBuffer( msgBuff );
	    QUinsert( &msgBuff->queue, connHndl->ch_rcvQueue.q_prev );

	    status = IIapi_readMsgError( ev_hndl );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case NS_SA_RRES :
	    /*
	    ** Read a GCN_RESULT message.
	    */
	    ns_read_result( stmtHndl, (IIAPI_MSG_BUFF *)parmBlock );
	    break;

	case NS_SA_BRES :
	    /*
	    ** Read a buffered GCN_RESULT message.
	    */
	    if ( stmtHndl->sh_rcvBuff )
		ns_read_result( stmtHndl, stmtHndl->sh_rcvBuff );
	    break;

	case NS_SA_SETD :
	    /*
	    ** Set descriptor.
	    */
	    status = ns_set_descr( stmtHndl, (IIAPI_SETDESCRPARM *)parmBlock );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case NS_SA_INFO :
	{
	    /*
	    ** Return query info.
	    */
	    IIAPI_GETQINFOPARM  *gqiParm, *qinfo;

	    gqiParm = (IIAPI_GETQINFOPARM *)stmtHndl->sh_parm;
	    qinfo = &stmtHndl->sh_responseData;

	    gqiParm->gq_flags = qinfo->gq_flags;
	    gqiParm->gq_mask = qinfo->gq_mask;
	    gqiParm->gq_rowCount = qinfo->gq_rowCount;
	    gqiParm->gq_readonly = qinfo->gq_readonly;
	    gqiParm->gq_procedureReturn = qinfo->gq_procedureReturn;
	    gqiParm->gq_procedureHandle = qinfo->gq_procedureHandle;
	    gqiParm->gq_repeatQueryHandle = qinfo->gq_repeatQueryHandle;
	    MEcopy( qinfo->gq_tableKey, IIAPI_TBLKEYSZ, gqiParm->gq_tableKey );
	    MEcopy( qinfo->gq_objectKey,IIAPI_OBJKEYSZ,gqiParm->gq_objectKey );
	    qinfo->gq_flags = 0;
	    qinfo->gq_mask = 0;
	    break;
	}
	case NS_SA_DESC :
	    /*
	    ** Build descriptor.
	    */
	    if ((status = IIapi_getNSDescriptor(stmtHndl)) != IIAPI_ST_SUCCESS)
		success = FALSE;
	    else
	    {
		/*
		** Return descriptor.
		*/
		IIAPI_GETDESCRPARM *getDescrParm;

		getDescrParm = (IIAPI_GETDESCRPARM *)stmtHndl->sh_parm;
		getDescrParm->gd_descriptorCount = stmtHndl->sh_colCount;
		getDescrParm->gd_descriptor = stmtHndl->sh_colDescriptor;
	    }
	    break;

	case NS_SA_RSLT :
	    /*
	    ** Fake a GCN_RESULT message using buffered data.
	    */
	    IIapi_liDispatch( IIAPI_EV_GCN_RESULT_RCVD, sm_hndl, 
			      (II_PTR)stmtHndl->sh_rcvBuff, NULL );
	    break;

	case NS_SA_BUFF :
	{
	    /*
	    ** Handle GCA receive buffer.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    /*
	    ** Save the GCA receive buffer if more data
	    ** available and buffer is not already saved.
	    */
	    if ( msgBuff != stmtHndl->sh_rcvBuff )
	    {
		if ( stmtHndl->sh_rcvBuff )
		    IIapi_releaseMsgBuffer( stmtHndl->sh_rcvBuff );

		IIapi_reserveMsgBuffer( msgBuff );
		stmtHndl->sh_rcvBuff = msgBuff;
	    }
	    break;
	}
	case NS_SA_QBUF :
	{
	    /*
	    ** Message buffer is reserved when placed on
	    ** the receive queue.  It will be released
	    ** when removed from the queue and processed.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    IIapi_reserveMsgBuffer( msgBuff );
	    QUinsert( &msgBuff->queue, connHndl->ch_rcvQueue.q_prev );
	    break;
	}
	case NS_SA_RLSB :
	    /*
	    ** Release saved GCA buffer if no more data available.
	    */
	    if ( stmtHndl->sh_rcvBuff )
	    {
		IIapi_releaseMsgBuffer( stmtHndl->sh_rcvBuff );
		stmtHndl->sh_rcvBuff = NULL;
	    }
	    break;

	case NS_SA_DELH :
	    /*
	    ** Mark handle for deletion.
	    */
	    sm_hndl->hd_delete = TRUE;
	    QUremove( (QUEUE *)stmtHndl );
	    break;

	case NS_SA_ERQA :
	    /*
	    ** Query active.
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s: Query active - must be cancelled\n", ns_stmt_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0007_INCOMPLETE_QUERY, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case NS_SA_ERQD :
	    /*
	    ** Query completed.
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s: Query done - cannot be cancelled\n", ns_stmt_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0008_QUERY_DONE, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case NS_SA_ERQC :
	    /*
	    ** Query cancelled .
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s: Query has been cancelled\n", ns_stmt_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0009_QUERY_CANCELLED, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case NS_SA_CBOK :
	    /*
	    ** Callback with success.
	    */
	    if ( stmtHndl->sh_callback )
	    {
		IIapi_appCallback(stmtHndl->sh_parm, sm_hndl, IIAPI_ST_SUCCESS);
		stmtHndl->sh_callback = FALSE;
	    }
	    break;

	case NS_SA_CBFL :
	    /*
	    ** Callback with failure.
	    */
	    if ( stmtHndl->sh_callback )
	    {
		IIapi_appCallback(stmtHndl->sh_parm, sm_hndl, IIAPI_ST_FAILURE);
		stmtHndl->sh_callback = FALSE;
	    }
	    break;

	case NS_SA_CBND :
	    /*
	    ** Callback with no data.
	    */
	    if ( stmtHndl->sh_callback )
	    {
		IIapi_appCallback(stmtHndl->sh_parm, sm_hndl, IIAPI_ST_NO_DATA);
		stmtHndl->sh_callback = FALSE;
	    }
	    break;

	case NS_SA_CBGC :
	    /*
	    ** Callback IIapi_getColumns() with status.
	    */
	    if ( stmtHndl->sh_callback )
	    {
		IIAPI_GETCOLPARM *getColParm;

		getColParm = (IIAPI_GETCOLPARM *)stmtHndl->sh_parm;
		IIapi_appCallback( stmtHndl->sh_parm, sm_hndl, 
				   (getColParm->gc_rowsReturned < 1) 
				       ? IIAPI_ST_NO_DATA : IIAPI_ST_SUCCESS );
		stmtHndl->sh_callback = FALSE;
	    }
	    break;

        case NS_SA_CBIF :
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
            ** This may not have been a statement
            ** related function, and we may have a
            ** callback saved on the statement
            ** handle, so we carefully make the
            ** callback to the caller making sure
            ** not to disturb the statement handle.
            */
            IIapi_appCallback( (IIAPI_GENPARM *)parmBlock, sm_hndl, status );

            /*
            ** We must return the failure here rather
            ** than following the normal exit route
            ** to ensure that the statement handle
            ** callback does not get made.
            */
            return( FALSE );

	case NS_SA_CNOK :
	    /*
	    ** Callback with success: query cancelled.
	    */
	    if ( stmtHndl->sh_cancelled )
	    {
		IIapi_appCallback( stmtHndl->sh_cancelParm, 
				   NULL, IIAPI_ST_SUCCESS );
		stmtHndl->sh_cancelled = FALSE;
	    }
	    break;

	case NS_SA_CNWN :
	    /*
	    ** Callback with warning: duplicate cancel.
	    **
	    ** We don't want to disturb the existing
	    ** function or cancel callback info on the
	    ** statement handle, nor do we want to
	    ** return any other status other than the
	    ** warning (this will all be done when
	    ** the original cancel completes).
	    */
	    IIapi_appCallback( (IIAPI_GENPARM *)parmBlock, 
			       NULL, IIAPI_ST_WARNING );
	    break;

	case NS_SA_CNFL :
	    /*
	    ** Callback with failure: cancel failed
	    */
	    if ( stmtHndl->sh_cancelled )
	    {
		IIapi_appCallback( stmtHndl->sh_cancelParm, 
				   sm_hndl, IIAPI_ST_FAILURE );
		stmtHndl->sh_cancelled = FALSE;
	    }
	    break;

        case NS_SA_RCVE :
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
    if ( ! success  &&  stmtHndl->sh_callback )
    {
	IIapi_appCallback( stmtHndl->sh_parm, sm_hndl, status );
	stmtHndl->sh_callback = FALSE;
    }

    return( success );
}


/*
** Name: ns_set_descr
**
** Description:
**	Saves the applications descriptor for IIapi_setDescriptor().
**
** Input:
**	stmtHndl	Statement handle to receive descriptor.
**	setDescrParm	Parameter block with descriptor to be saved.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS, IIAPI_ST_OUT_OF_MEMORY.
**
** History:
**	10-Mar-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Free existing descriptor (if any).
*/

static IIAPI_STATUS
ns_set_descr( IIAPI_STMTHNDL *stmtHndl, IIAPI_SETDESCRPARM *setDescrParm )
{
    STATUS	status;
    i4		i;

    /*
    ** Free any existing saved descriptor.
    */
    if ( stmtHndl->sh_parmDescriptor )
    {
	for( i = 0; i < stmtHndl->sh_parmCount; i++ )
	    if ( stmtHndl->sh_parmDescriptor[i].ds_columnName )
		MEfree( (II_PTR)stmtHndl->sh_parmDescriptor[i].ds_columnName );
	
	MEfree( (II_PTR)stmtHndl->sh_parmDescriptor );
        stmtHndl->sh_parmDescriptor = NULL;
    }
    
    stmtHndl->sh_parmIndex = stmtHndl->sh_parmSend = 0;

    if ( (stmtHndl->sh_parmCount = setDescrParm->sd_descriptorCount) )
    {
	if ( ! (stmtHndl->sh_parmDescriptor = (IIAPI_DESCRIPTOR *)
		    MEreqmem(0, sizeof( IIAPI_DESCRIPTOR ) * 
				stmtHndl->sh_parmCount, TRUE, &status)) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "ns_set_descr: can't allocate descriptor\n" );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}
	
	for( i = 0; i < stmtHndl->sh_parmCount; i++ )
	{
	    STRUCT_ASSIGN_MACRO( setDescrParm->sd_descriptor[i],
				 stmtHndl->sh_parmDescriptor[i] );

	    if ( 
		 setDescrParm->sd_descriptor[i].ds_columnName   &&
		 ! (stmtHndl->sh_parmDescriptor[i].ds_columnName =
			STalloc(setDescrParm->sd_descriptor[i].ds_columnName)) 
	       )
	    {
		IIAPI_TRACE( IIAPI_TR_FATAL )
		    ( "ns_set_descr: can't allocate column name\n" );
		return( IIAPI_ST_OUT_OF_MEMORY );
	    }
	}
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: ns_read_result
**
** Description:
**	Read/accumulate GCN result info.  Contents of message 
**	buffer discarded.
**
** Input:
**	stmtHndl	Statement handle.
**	msgBuff		GCA message buffer.
**
** Output:
**	None.
**
** Returns:
**	II_VOID
**
** History:
**	 5-May-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    At protocol level 63 Name Server protocol fixed to
**	    handle end-of-group and end-of-data by GCA standards.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Enhanced parameter memory block handling.
*/

static II_VOID
ns_read_result( IIAPI_STMTHNDL *stmtHndl, IIAPI_MSG_BUFF *msgBuff )
{
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl((IIAPI_HNDL *)stmtHndl);
    IIAPI_GETQINFOPARM  *qinfo = &stmtHndl->sh_responseData;
    i4			row_count;

    /*
    ** We only want the row count info,
    ** the rest of the message is ignored.
    */
    if ( ! (connHndl->ch_flags & IIAPI_CH_MSG_CONTINUED) )
    {
	/*
	** Start of message is at top of buffer.
	** Reset data which may be pointing at
	** next result row.  Skip the opcode.
	*/
	msgBuff->data = msgBuff->buffer;
	msgBuff->data += gcu_get_int( msgBuff->data, &row_count );
	msgBuff->data += gcu_get_int( msgBuff->data, &row_count );

	qinfo->gq_mask |= IIAPI_GQ_ROW_COUNT;
	qinfo->gq_rowCount += (II_LONG)row_count;
    }

    /*
    ** Set/reset the continuation flag if the message is
    ** continued/done so that we will process the correct
    ** portion of the message next time this routine is
    ** called.
    */
    if ( NS_EOD( connHndl, msgBuff ) )
	connHndl->ch_flags &= ~IIAPI_CH_MSG_CONTINUED;
    else
	connHndl->ch_flags |= IIAPI_CH_MSG_CONTINUED;

    /*
    ** We toss the rest of the message,
    ** in case this is a saved buffer or
    ** a new receive will be posted with
    ** this buffer.
    */
    msgBuff->length = 0;
    return;
}


/*
** Name: ns_eval_data
**
** Description:
**	Determines if a GCN_RESULT message satisfies the current
**	request.  Tuple data is moved from the GCA buffer to the
**	API output parameters at the same time.
**
**	The GCA buffer is handled in a non-standard way.  For new 
**	messages, the data pointer is at the start of the message 
**	(header containing opcode and row_count).  During processing 
**	the data pointer is set to the start of the current row.  We 
**	may be called again in this state, but we continue to use 
**	the row_count in the message header.
**
**	We never completely consume the GCA buffer in this routine.  
**	Instead, we process rows until the row count goes to zero.  
**	If the request is not satisfied at this point, the state
**	machine logic will handle further processing depending on
**	the state of the message stream.  Handling completion of
**	requests takes precedence over fully processed buffers.
**	If the request is satisfied by the last row in the buffer,
**	the application callback will be handled first.  We will
**	subsequently get called again with the same buffer which
**	will fail to satisfy the new request and will be handled
**	as above.
**
** Input:
**	stmtHndl	Statement handle.
**	getColParm	IIapi_getColumns() parameters.
**	msgBuff		GCA receive buffer.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if request satisfied.
**
** History:
**	 5-May-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    At protocol level 63 Name Server protocol fixed to
**	    handle end-of-group and end-of-data by GCA standards.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Enhanced parameter memory block handling.
*/

static II_BOOL
ns_eval_data
( 
    IIAPI_STMTHNDL	*stmtHndl, 
    IIAPI_GETCOLPARM	*getColParm,
    IIAPI_MSG_BUFF	*msgBuff 
)
{
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl((IIAPI_HNDL *)stmtHndl);
    IIAPI_GETQINFOPARM  *qinfo = &stmtHndl->sh_responseData;
    IIAPI_GCN_REQ_TUPLE	tuple;
    char		*ptr, *rc_ptr;
    i4			row_count;

    /*
    ** First, make sure the request is not already satisfied.
    */
    if ( ! stmtHndl->sh_colFetch  ||
	 getColParm->gc_rowsReturned >= getColParm->gc_rowCount )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )( "ns_data: nothing to do.\n" );
	return( TRUE );
    }

    /*
    ** Save location of row count and extract count.  
    */
    ptr = msgBuff->buffer;
    ptr += gcu_get_int( ptr, &row_count );	/* Opcode ignored */
    rc_ptr = ptr;				/* Save row count location */
    ptr += gcu_get_int( ptr, &row_count );	/* Extract row count */

    /*
    ** When starting a new message, set the data
    ** pointer to the start of the first tuple.
    */
    if ( 
	 ! (connHndl->ch_flags & IIAPI_CH_MSG_CONTINUED)  &&  
	 msgBuff->data == msgBuff->buffer 
       )
	msgBuff->data = ptr;

    /*
    ** We may be called with a buffer containing
    ** no rows.  Process rows until request is
    ** complete or no more rows in buffer.
    */
    while( stmtHndl->sh_colFetch  &&  row_count )
    {
	/*
	** Extract the entire tuple even though
	** we may only want a subset of the
	** columns.  We will consume the tuple
	** only when all the columns have been
	** processed.
	*/
	ptr = msgBuff->data;
	ptr += gcu_get_str( ptr, &tuple.gcn_type.gcn_value );
	tuple.gcn_type.gcn_l_item = STlength( tuple.gcn_type.gcn_value );
	ptr += gcu_get_str( ptr, &tuple.gcn_uid.gcn_value );
	tuple.gcn_uid.gcn_l_item = STlength( tuple.gcn_uid.gcn_value );
	ptr += gcu_get_str( ptr, &tuple.gcn_obj.gcn_value );
	tuple.gcn_obj.gcn_l_item = STlength( tuple.gcn_obj.gcn_value );
	ptr += gcu_get_str( ptr, &tuple.gcn_val.gcn_value );
	tuple.gcn_val.gcn_l_item = STlength( tuple.gcn_val.gcn_value );

	/*
	** Process columns in the current row.  The current
	** column request should not span rows, but may be
	** fewer than the remaining columns in the row.  If
	** this is a multi-row fetch, we will reset the
	** column request for the next row after processing
	** the current row.  Since we provide an entire
	** tuple for processing, the current row request
	** will be satisfied by this call.
	*/
	IIapi_loadNSColumns( stmtHndl, getColParm, &tuple );

	/*
	** We have completed the request for the current row.
	** We bump the number of rows returned since we either
	** competed a row or completed a partial fetch of a
	** row.
	*/
	getColParm->gc_rowsReturned++;

	/*
	** Check to see if we have completed the row.
	*/
	if ( stmtHndl->sh_colIndex >= stmtHndl->sh_colCount )
	{
	    /*
	    ** Finished processing current row.
	    */
	    msgBuff->data = ptr;
	    qinfo->gq_mask |= IIAPI_GQ_ROW_COUNT;
	    qinfo->gq_rowCount++;
	    row_count--;

	    /*
	    ** Set the column index for the start
	    ** of the next row.  If there are more
	    ** rows in the current request, set the
	    ** column fetch count for the next row.
	    */
	    stmtHndl->sh_colIndex = 0;

	    if ( getColParm->gc_rowsReturned < getColParm->gc_rowCount )
		stmtHndl->sh_colFetch = stmtHndl->sh_colCount;
	}
    }

    /*
    ** Update the current tuple info.
    */
    gcu_put_int( rc_ptr, row_count );

    /*
    ** Once we are done with the buffer
    ** we can update the continuation
    ** flag.
    */
    if ( ! row_count )
    {
	/*
	** Set/reset the continuation flag if the message is
	** continued/done so that we will process the correct
	** portion of the message next time this routine is
	** called.
	*/
	if ( NS_EOD( connHndl, msgBuff ) )
	    connHndl->ch_flags &= ~IIAPI_CH_MSG_CONTINUED;
	else
	    connHndl->ch_flags |= IIAPI_CH_MSG_CONTINUED;
    }

    /*
    ** Request is complete when no more columns to fetch.
    */
    return( ! stmtHndl->sh_colFetch );
}



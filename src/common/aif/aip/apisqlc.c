/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apisql.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apimsg.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisqlc.c
**
** Description:
**	SQL Connection State Machine.
**
** History:  Original history found in apicstbl.c (now defunct).
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	12-Feb-97 (gordy)
**	    Remove the PARM state.  Connection type is not known
**	    until a connection is made, so handles created by
**	    setting parameters are defaulted to SQL.  These must
**	    remain in the IDLE state until connection requested.
**	13-Mar-97 (gordy)
**	    Errors during GCA requests require a GCA disconnect.
**	    A send error in the REQ state now goes to the FAIL
**	    state and doesn't delete the connection handle.  The
**	    application is responsible for calling IIapi_disconnect()
**	    if a connection handle is returned.
**	 3-Jul-97 (gordy)
**	    Re-worked state machine initialization.  Added
**	    IIapi_sql_init(), IIapi_jas_init(), IIapi_sql_cinit().
**	 15-Aug-98 (rajus01)
**	    Added support for IIapi_abort().
**	 3-Sep-98 (gordy)
**	    Call functions to abort transaction, statement and
**	    database event handles.
**	 15-Sep-98 (rajus01)
**	    Added SQL_CA_OTRM, SQL_CAS_31 to output trace messages from the
**	    backends to the application.
**	 14-Oct-98 (rajus01)
**	    Added support for GCA_PROMPT. Added SQL_CA_RPMT, SQL_CA_CBPM,
**	    SQL_CA_SPRY actions. Added SQL_CS_PRMT state, IIapi_buildPrmpt().
**      03-Feb-99 (rajus01)
**	    Renamed IIAPI_UP_CAN_PROMPT to IIAPI_UP_PROMPT_FUNC. Trace message
**	    is null terminated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 7-Sep-00 (gordy)
**	    GCA errors should be marked as failures, not warnings.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	15-Mar-04 (gordy)
**	    Removed Jasmine support.
**	11-Oct-05 (gordy)
**	    Provide more explicit handling for operations after a
**	    connection abort.
**	 9-Dec-05 (gordy)
**	    Ignore messages received waiting for disconnect to complete.
**	 7-Jul-06 (gordy)
**	    Added support for XA functions.
**	27-Jul-06 (gordy)
**	    Handle additional messages during XA operations.
**	15-Mar-07 (gordy)
**	    Added support for scrollable cursors.
**	25-Mar-10 (gordy)
**	    Support batch processing.  Replaced GCA formatted interface
**	    with byte stream.  Enhanced parameter memory block handling.
*/

/*
** Forward references
*/

static IIAPI_SM_OUT	*sm_evaluate( IIAPI_EVENT, IIAPI_STATE, IIAPI_HNDL *, 
				      IIAPI_HNDL *, II_PTR, IIAPI_SM_OUT * );

static II_BOOL		sm_execute( IIAPI_ACTION, IIAPI_HNDL *, 
				    IIAPI_HNDL *, II_PTR parmBlock );

static IIAPI_STATUS	buildPrompt( IIAPI_MSG_BUFF *, IIAPI_PROMPTPARM * );


/*
** Information to be exported through
** the state machine interface.
*/

/*
** SQL Connection States.
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
**
** Note that the IDLE state must be 0 to
** match IIAPI_IDLE.
**
** History:
**	 7-Jul-06 (gordy)
**	    Added XA states.
*/

# define SQL_CS_IDLE	((IIAPI_STATE)  0)	/* Idle */
# define SQL_CS_CONN	((IIAPI_STATE)  1)	/* Connected */
# define SQL_CS_DBEV	((IIAPI_STATE)  2)	/* Waiting for database event */
# define SQL_CS_FAIL	((IIAPI_STATE)  3)	/* Connection failure */
# define SQL_CS_REQ	((IIAPI_STATE)  4)	/* Connection requested */
# define SQL_CS_WCR	((IIAPI_STATE)  5)	/* Wait for connect response */
# define SQL_CS_MOD	((IIAPI_STATE)  6)	/* Modify connection sent */
# define SQL_CS_WMR	((IIAPI_STATE)  7)	/* Wait for modify response */
# define SQL_CS_XA	((IIAPI_STATE)  8)	/* XA request sent */
# define SQL_CS_WXA	((IIAPI_STATE)  9)	/* Wait for XA response */
# define SQL_CS_PRMT	((IIAPI_STATE) 10)	/* Prompt message handling */
# define SQL_CS_RLS	((IIAPI_STATE) 11)	/* Release sent */
# define SQL_CS_DISC	((IIAPI_STATE) 12)	/* Disconnect requested */

# define SQL_CS_CNT	(SQL_CS_DISC + 1)

static char *sql_cs_id[] =
{
    "IDLE", "CONN", "DBEV", "FAIL", "REQ",  
    "WCR",  "MOD",  "WMR",  "XA",   "WXA", 
    "PRMT", "RLS",  "DISC"
};

/*
** SQL Connection actions
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
**
** History:
**	11-Oct-05 (gordy)
**	    Add action to halt state machine execution.
**	 7-Jul-06 (gordy)
**	    Added XA actions.
*/

# define SQL_CA_REMC	((IIAPI_ACTION)  0)	/* Remember callback */
# define SQL_CA_CONN	((IIAPI_ACTION)  1)	/* Connect */
# define SQL_CA_DISC	((IIAPI_ACTION)  2)	/* Disconnect */
# define SQL_CA_RECV	((IIAPI_ACTION)  3)	/* Receive message */
# define SQL_CA_RCVT	((IIAPI_ACTION)  4)	/* Receive message, timeout */
# define SQL_CA_REDO	((IIAPI_ACTION)  5)	/* Resume GCA request */
# define SQL_CA_SMDA	((IIAPI_ACTION)  6)	/* Send connection params */
# define SQL_CA_SRLS	((IIAPI_ACTION)  7)	/* Send release */
# define SQL_CA_RNPI	((IIAPI_ACTION)  8)	/* Read interrupt */
# define SQL_CA_RERR	((IIAPI_ACTION)  9)	/* Read error */
# define SQL_CA_REVE	((IIAPI_ACTION) 10)	/* Read database event */
# define SQL_CA_CPRM	((IIAPI_ACTION) 11)	/* Clear connection params */
# define SQL_CA_SPRM	((IIAPI_ACTION) 12)	/* Set connection parameters */
# define SQL_CA_PEND	((IIAPI_ACTION) 13)	/* Abort pending operations */
# define SQL_CA_DELH	((IIAPI_ACTION) 14)	/* Delete connection handle */
# define SQL_CA_ERGC	((IIAPI_ACTION) 15)	/* GCA failure/error */
# define SQL_CA_ERIP	((IIAPI_ACTION) 16)	/* Invalid connection params */
# define SQL_CA_ERAB	((IIAPI_ACTION) 17)	/* Connection failure */
# define SQL_CA_CBOK	((IIAPI_ACTION) 18)	/* Callback Success */
# define SQL_CA_CBWN	((IIAPI_ACTION) 19)	/* Callback Warning */
# define SQL_CA_CBIF	((IIAPI_ACTION) 20)	/* Callback Invalid Function */
# define SQL_CA_CBAB	((IIAPI_ACTION) 21)	/* Callback Abort */
# define SQL_CA_CBDC	((IIAPI_ACTION) 22)	/* Callback disconnect */
# define SQL_CA_OTRM	((IIAPI_ACTION) 23)	/* Output Trace mesg to app.*/
# define SQL_CA_RPMT	((IIAPI_ACTION) 24)	/* Read prompt data */
# define SQL_CA_CBPM	((IIAPI_ACTION) 25)	/* Callback prompt */
# define SQL_CA_SPRY	((IIAPI_ACTION) 26)	/* Send prompt reply message */
# define SQL_CA_SXAP	((IIAPI_ACTION) 27)	/* Send XA Prepare message */
# define SQL_CA_SXAC	((IIAPI_ACTION) 28)	/* Send XA Commit message */
# define SQL_CA_SXAR	((IIAPI_ACTION) 29)	/* Send XA Rollback message */
# define SQL_CA_RXAR	((IIAPI_ACTION) 30)	/* Read XA error response */
# define SQL_CA_RRSP	((IIAPI_ACTION) 31)	/* Read response message */
# define SQL_CA_QBUF	((IIAPI_ACTION) 32)	/* Queue message buffer */
# define SQL_CA_HALT	((IIAPI_ACTION) 33)	/* Halt SM execution */

# define SQL_CA_CNT	(SQL_CA_HALT + 1)

static char *sql_ca_id[] =
{
    "REMC", "CONN", "DISC", "RECV", "RCVT", "REDO", "SMDA", "SRLS", 
    "RNPI", "RERR", "REVE", "CPRM", "SPRM", "PEND", "DELH", 
    "ERGC", "ERIP", "ERAB", "CBOK", "CBWN", "CBIF", "CBAB", "CBDC",
    "OTRM", "RPMT", "CBPM", "SPRY", "SXAP", "SXAC", "SXAR", 
    "RXAR", "RRSP", "QBUF", "HALT"
};

/*
** Name: sql_conn_sm
**
** Description:
**	The SQL Connection State Machine Interface
*/

static IIAPI_SM sql_conn_sm =
{
    "SQL Conn",
    SQL_CS_CNT,
    sql_cs_id,
    SQL_CA_CNT,
    sql_ca_id,
    sm_evaluate,
    sm_execute,
    SQL_CS_FAIL
};


/*
** Name: sql_act_seq
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
**	 2-Oct-96 (gordy)
**	    Created.
**	13-Mar-97 (gordy)
**	    Errors during GCA requests require a GCA disconnect.
**	    Combined SQL Action Sequence 26 with SAS 8 and removed
**	    disconnect and handle deletion actions.  These actions 
**	    will occur when the application calls IIapi_disconnect().
**	11-Oct-05 (gordy)
**	    Add sequence to indicate connection abort.
**	 7-Jul-06 (gordy)
**	    Added action sequences for XA support.
*/

static IIAPI_SM_OUT sql_act_seq[] =
{
    { 0, 0, { 0, 0, 0 } },					/* CAS_0  */
    { 0, 1, { SQL_CA_RECV, 0, 0 } },				/* CAS_1  */
    { 0, 1, { SQL_CA_DISC, 0, 0 } },				/* CAS_2  */
    { 0, 1, { SQL_CA_SMDA, 0, 0 } },				/* CAS_3  */
    { 0, 2, { SQL_CA_REMC, SQL_CA_SMDA, 0 } },			/* CAS_4  */
    { 0, 2, { SQL_CA_REMC, SQL_CA_CONN, 0 } },			/* CAS_5  */
    { 0, 2, { SQL_CA_REMC, SQL_CA_SRLS, 0 } },			/* CAS_6  */
    { 0, 2, { SQL_CA_REMC, SQL_CA_DISC, 0 } },			/* CAS_7  */
    { 0, 2, { SQL_CA_ERGC, SQL_CA_CBAB, 0 } },			/* CAS_8  */
    { 0, 3, { SQL_CA_PEND, SQL_CA_DELH, SQL_CA_CBOK } },	/* CAS_9  */
    { 0, 3, { SQL_CA_ERGC, SQL_CA_PEND, SQL_CA_CBAB } },	/* CAS_10 */
    { 0, 3, { SQL_CA_RERR, SQL_CA_PEND, SQL_CA_CBAB } },	/* CAS_11 */
    { 0, 3, { SQL_CA_RERR, SQL_CA_ERIP, SQL_CA_CBWN } },	/* CAS_12 */
    { 0, 1, { SQL_CA_CBOK, 0, 0 } },				/* CAS_13 */
    { 0, 1, { SQL_CA_RNPI, 0, 0 } },				/* CAS_14 */
    { 0, 2, { SQL_CA_RERR, SQL_CA_RECV, 0 } },			/* CAS_15 */
    { 0, 2, { SQL_CA_RERR, SQL_CA_CBWN, 0 } },			/* CAS_16 */
    { 0, 2, { SQL_CA_REVE, SQL_CA_CBOK, 0 } },			/* CAS_17 */
    { 0, 2, { SQL_CA_REMC, SQL_CA_RCVT, 0 } },			/* CAS_18 */
    { 0, 1, { SQL_CA_CBWN, 0, 0 } },				/* CAS_19 */
    { 0, 3, { SQL_CA_REMC, SQL_CA_SPRM, SQL_CA_CBOK } },	/* CAS_20 */
    { 0, 2, { SQL_CA_CPRM, SQL_CA_RECV, 0 } },			/* CAS_21 */
    { 0, 2, { SQL_CA_REVE, SQL_CA_RECV, 0 } },			/* CAS_22 */
    { 0, 3, { SQL_CA_SRLS, SQL_CA_PEND, SQL_CA_CBAB } },	/* CAS_23 */
    { 0, 2, { SQL_CA_CBIF, SQL_CA_HALT, 0 } },			/* CAS_24 */
    { 0, 3, { SQL_CA_RERR, SQL_CA_ERAB, SQL_CA_DISC } },	/* CAS_25 */
    { 0, 3, { SQL_CA_REMC, SQL_CA_DELH, SQL_CA_CBOK } },	/* CAS_26 */
    { 0, 3, { SQL_CA_CBAB, SQL_CA_REMC, SQL_CA_SRLS } },	/* CAS_27 */
    { 0, 1, { SQL_CA_REDO, 0, 0 } },				/* CAS_28 */
    { 0, 3, { SQL_CA_CBAB, SQL_CA_REMC, SQL_CA_DISC } },	/* CAS_29 */
    { 0, 2, { SQL_CA_REMC, SQL_CA_CBDC, 0 } },			/* CAS_30 */
    { 0, 2, { SQL_CA_OTRM, SQL_CA_RECV, 0 } },			/* CAS_31 */
    { 0, 3, { SQL_CA_RPMT, SQL_CA_CBPM, SQL_CA_SPRY } },	/* CAS_32 */
    { 0, 3, { SQL_CA_REMC, SQL_CA_CBAB, SQL_CA_HALT } },	/* CAS_33 */
    { 0, 2, { SQL_CA_REMC, SQL_CA_SXAP, 0 } },			/* CAS_34 */
    { 0, 2, { SQL_CA_REMC, SQL_CA_SXAC, 0 } },			/* CAS_35 */
    { 0, 2, { SQL_CA_REMC, SQL_CA_SXAR, 0 } },			/* CAS_36 */
    { 0, 2, { SQL_CA_RXAR, SQL_CA_CBOK, 0 } },			/* CAS_37 */
    { 0, 2, { SQL_CA_RRSP, SQL_CA_CBOK, 0 } },			/* CAS_38 */
    { 0, 2, { SQL_CA_QBUF, SQL_CA_RECV, 0 } },			/* CAS_39 */
};

/*
** Indexes into the action sequence array.
*/

# define SQL_CAS_0	0
# define SQL_CAS_1	1
# define SQL_CAS_2	2
# define SQL_CAS_3	3
# define SQL_CAS_4	4
# define SQL_CAS_5	5
# define SQL_CAS_6	6
# define SQL_CAS_7	7
# define SQL_CAS_8	8
# define SQL_CAS_9	9
# define SQL_CAS_10	10
# define SQL_CAS_11	11
# define SQL_CAS_12	12
# define SQL_CAS_13	13
# define SQL_CAS_14	14
# define SQL_CAS_15	15
# define SQL_CAS_16	16
# define SQL_CAS_17	17
# define SQL_CAS_18	18
# define SQL_CAS_19	19
# define SQL_CAS_20	20
# define SQL_CAS_21	21
# define SQL_CAS_22	22
# define SQL_CAS_23	23
# define SQL_CAS_24	24
# define SQL_CAS_25	25
# define SQL_CAS_26	26
# define SQL_CAS_27	27
# define SQL_CAS_28	28
# define SQL_CAS_29	29
# define SQL_CAS_30	30
# define SQL_CAS_31	31
# define SQL_CAS_32	32
# define SQL_CAS_33	33
# define SQL_CAS_34	34
# define SQL_CAS_35	35
# define SQL_CAS_36	36
# define SQL_CAS_37	37
# define SQL_CAS_38	38
# define SQL_CAS_39	39

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
**	 2-Oct-96 (gordy)
**	    Created.
**	12-Feb-97 (gordy)
**	    Remove the PARM state.
**	13-Mar-97 (gordy)
**	    Errors during GCA requests require a GCA disconnect.
**	    A send error in the REQ state now goes to the FAIL
**	    state and doesn't delete the connection handle.  The
**	    application is responsible for calling IIapi_disconnect()
**	    if a connection handle is returned.
**	11-Oct-05 (gordy)
**	    Add explicit handling for connection abort state.
**	 9-Dec-05 (gordy)
**	    Ignore messages received waiting for disconnect to complete.
**	 7-Jul-06 (gordy)
**	    Support XA functions and responses.
**	27-Jul-06 (gordy)
**	    Permit error and trace messages during XA operations.
**	    Read response message for successful XA operations.
**	15-Mar-07 (gordy)
**	    Added position and scrolling functions.
**	25-Mar-10 (gordy)
**	    Added batch function.
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
    ** API Functions which directly affect the connection state.
    */
    { IIAPI_EV_CONNECT_FUNC,	SQL_CS_IDLE,	SQL_CS_REQ,	SQL_CAS_5  },
    { IIAPI_EV_DISCONN_FUNC,	SQL_CS_IDLE,	SQL_CS_IDLE,	SQL_CAS_26 },
    { IIAPI_EV_DISCONN_FUNC,	SQL_CS_CONN,	SQL_CS_RLS,	SQL_CAS_6  },
    { IIAPI_EV_DISCONN_FUNC,	SQL_CS_DBEV,	SQL_CS_RLS,	SQL_CAS_27 },
    { IIAPI_EV_DISCONN_FUNC,	SQL_CS_FAIL,	SQL_CS_DISC,	SQL_CAS_7  },
    { IIAPI_EV_GETEVENT_FUNC,	SQL_CS_CONN,	SQL_CS_DBEV,	SQL_CAS_18 },
    { IIAPI_EV_MODCONN_FUNC,	SQL_CS_CONN,	SQL_CS_MOD,	SQL_CAS_4  },
    { IIAPI_EV_SETCONNPARM_FUNC,SQL_CS_IDLE,	SQL_CS_IDLE,	SQL_CAS_20 },
    { IIAPI_EV_SETCONNPARM_FUNC,SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_20 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_IDLE,	SQL_CS_IDLE,	SQL_CAS_26 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_CONN,	SQL_CS_DISC,	SQL_CAS_7 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_DBEV,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_FAIL,	SQL_CS_DISC,	SQL_CAS_7 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_WCR,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_MOD,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_WMR,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_XA,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_WXA,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_PRMT,	SQL_CS_DISC,	SQL_CAS_29 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_30 },
    { IIAPI_EV_ABORT_FUNC,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_30 },
    { IIAPI_EV_XAPREP_FUNC,	SQL_CS_CONN,	SQL_CS_XA,	SQL_CAS_34 },
    { IIAPI_EV_XACOMMIT_FUNC,	SQL_CS_CONN,	SQL_CS_XA,	SQL_CAS_35 },
    { IIAPI_EV_XAROLL_FUNC,	SQL_CS_CONN,	SQL_CS_XA,	SQL_CAS_36 },

    /*
    ** The following functions must be allowed while waiting 
    ** for database events so that event processing, which 
    ** doesn't affect the connection, can be completed.
    */
    { IIAPI_EV_CANCEL_FUNC,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_0  },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_0  },

    /*
    ** The following functions are permitted while connected, 
    ** but do not directly affect the SQL connection state.
    */
    { IIAPI_EV_AUTO_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_AUTO_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_BATCH_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_COMMIT_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_COMMIT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_PRECOMMIT_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_ROLLBACK_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_ROLLBACK_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_SAVEPOINT_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_XASTART_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_XAEND_FUNC,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_XAEND_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },

    /*
    ** The following functions are not permitted after a connection abort.
    */
    { IIAPI_EV_BATCH_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_CATCHEVENT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_GETEVENT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_MODCONN_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_POSITION_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_PRECOMMIT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_QUERY_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_SAVEPOINT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_SCROLL_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_SETCONNPARM_FUNC,SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_XASTART_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_XAPREP_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_XACOMMIT_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },
    { IIAPI_EV_XAROLL_FUNC,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_33 },

    /*
    ** The following messages are processed at the connection 
    ** level.  Note that during the connected state the message
    ** resulted from actions in another state machine.
    */
    { IIAPI_EV_ACCEPT_RCVD,	SQL_CS_WCR,	SQL_CS_MOD,	SQL_CAS_3  },
    { IIAPI_EV_ACCEPT_RCVD,	SQL_CS_WMR,	SQL_CS_CONN,	SQL_CAS_13 },
    { IIAPI_EV_DONE_RCVD,	SQL_CS_WXA,	SQL_CS_CONN,	SQL_CAS_38 },
    { IIAPI_EV_REFUSE_RCVD,	SQL_CS_WXA,	SQL_CS_CONN,	SQL_CAS_37 },
    { IIAPI_EV_NPINTERUPT_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_14 },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_31 },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_31 },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_WCR,	SQL_CS_WCR,	SQL_CAS_31 },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_WMR,	SQL_CS_WMR,	SQL_CAS_31 },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_WXA,	SQL_CS_WXA,	SQL_CAS_31 },

    /*
    ** The following messages are ignored in the connected 
    ** state.  They are the results of actions in other
    ** state machines.
    */
    { IIAPI_EV_CFROM_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_DONE_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_REFUSE_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_RESPONSE_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },

    /*
    ** Ignore messages received during shutdown/disconnect.
    */
    { IIAPI_EV_ACCEPT_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_ACCEPT_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_DONE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_DONE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_ERROR_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_ERROR_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_EVENT_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_EVENT_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_NPINTERUPT_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_NPINTERUPT_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_PROMPT_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_PROMPT_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_REFUSE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_REFUSE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_REJECT_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_REJECT_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_RELEASE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_RELEASE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_RESPONSE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_RESPONSE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_TRACE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },

    /*
    ** GCA request completions.
    */
    { IIAPI_EV_RESUME,		SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_DBEV,	SQL_CS_DBEV,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_REQ,	SQL_CS_REQ,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_WCR,	SQL_CS_WCR,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_MOD,	SQL_CS_MOD,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_WMR,	SQL_CS_WMR,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_XA,	SQL_CS_XA,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_WXA,	SQL_CS_WXA,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_PRMT,	SQL_CS_PRMT,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_28 },
    { IIAPI_EV_RESUME,		SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_28 },
    { IIAPI_EV_CONNECT_CMPL,	SQL_CS_REQ,	SQL_CS_WCR,	SQL_CAS_1 },
    { IIAPI_EV_DISCONN_CMPL,	SQL_CS_DISC,	SQL_CS_IDLE,	SQL_CAS_9  },
    { IIAPI_EV_SEND_CMPL,	SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_CS_MOD,	SQL_CS_WMR,	SQL_CAS_21 },
    { IIAPI_EV_SEND_CMPL,	SQL_CS_XA,	SQL_CS_WXA,	SQL_CAS_1 },
    { IIAPI_EV_SEND_CMPL,	SQL_CS_RLS,	SQL_CS_DISC,	SQL_CAS_2  },
    { IIAPI_EV_SEND_CMPL,	SQL_CS_PRMT,	SQL_CS_WMR,	SQL_CAS_1  },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_CONN,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_DBEV,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_REQ,	SQL_CS_FAIL,	SQL_CAS_8  },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_MOD,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_WMR,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_XA,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_WXA,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_PRMT,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_RLS,	SQL_CS_DISC,	SQL_CAS_2  },
    { IIAPI_EV_SEND_ERROR,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_CONN,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_FAIL,	SQL_CS_FAIL,	SQL_CAS_0  },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_WCR,	SQL_CS_FAIL,	SQL_CAS_8  },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_MOD,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_WMR,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_XA,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_WXA,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_PRMT,	SQL_CS_FAIL,	SQL_CAS_10 },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_RLS,	SQL_CS_RLS,	SQL_CAS_0  },
    { IIAPI_EV_RECV_ERROR,	SQL_CS_DISC,	SQL_CS_DISC,	SQL_CAS_0  },
    { IIAPI_EV_DONE,		SQL_CS_CONN,	SQL_CS_CONN,	SQL_CAS_0  },
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

static SM_TRANSITION	*smt_array[ IIAPI_EVENT_CNT ][ SQL_CS_CNT ] ZERO_FILL;
static II_BOOL		initialized = FALSE;



/*
** Name: IIapi_sql_init
**
** Description:
**	Initialize the SQL state machines.  Adds entries for the 
**	SQL state machines in the global dispatch table IIapi_sm.
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
IIapi_sql_init( VOID )
{
    IIAPI_STATUS status;

    for(;;)
    {
	/*
	** Initialize SQL state machines.
	*/
	if ( (status = IIapi_sql_cinit()) != IIAPI_ST_SUCCESS )  break;
	if ( (status = IIapi_sql_tinit()) != IIAPI_ST_SUCCESS )  break;
	if ( (status = IIapi_sql_sinit()) != IIAPI_ST_SUCCESS )  break;
	status = IIapi_sql_dinit();

	break;
    }

    return( status );
}


/*
** Name: IIapi_sql_cinit
**
** Description:
**	Initialize the SQL Connection state machine.
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
IIapi_sql_cinit( VOID )
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

    IIapi_sm[ IIAPI_SMT_SQL ][ IIAPI_SMH_CONN ] = &sql_conn_sm;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: sm_evaluate
**
** Description:
**	Evaluation function for the Connection State Machine
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
**	25-Mar-10 (gordy)
**	    Replaced GCA formatted interface with byte stream.  
**	    Enhanced parameter memory block handling.
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
    IIAPI_CONNHNDL	*connHndl = (IIAPI_CONNHNDL *)sm_hndl;
    IIAPI_SM_OUT	*smo = NULL;
    SM_TRANSITION	*smt;
    STATUS		gcaError;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "%s evaluate: evaluating event %s in state %s\n",
	  sql_conn_sm.sm_id, IIAPI_PRINTEVENT( event ),
	  IIAPI_PRINT_ID( state, SQL_CS_CNT, sql_cs_id ) );

    /*
    ** Static evaluations depend solely on the current state and event.
    */
    if ( (smt = smt_array[ event ][ state ]) )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "%s evaluate: static evaluation\n", sql_conn_sm.sm_id );

	smo = smo_buff;
	STRUCT_ASSIGN_MACRO( sql_act_seq[ smt->smt_action ], *smo );
	smo->smo_next_state = smt->smt_next;
    }
    else		/* Dynamic evaluations require additional predicates */
    switch( event )	/* to determine the correct state transition.	     */
    {
    /*
    ** GCA_ERROR, GCA_REJECT, and GCA_RELEASE messages
    ** are all handled by this state machine.  These
    ** messages may be split.  We don't change states
    ** on these messages (when connection related)
    ** until the end of the message is received.
    */
    case IIAPI_EV_ERROR_RCVD :
	switch( state )
	{
	case SQL_CS_CONN :
	case SQL_CS_WCR :
	case SQL_CS_WMR :
	case SQL_CS_WXA :
	{
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
	    smo = smo_buff;

	    if ( msgBuff->flags & IIAPI_MSG_EOD )
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_15 ], *smo );
		smo->smo_next_state = state;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_39 ], *smo );
		smo->smo_next_state = state;
	    }
	    break;
	}
	case SQL_CS_DBEV :
	{
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
	    smo = smo_buff;

	    if ( msgBuff->flags & IIAPI_MSG_EOD )
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_16 ], *smo );
		smo->smo_next_state = SQL_CS_CONN;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_39 ], *smo );
		smo->smo_next_state = state;
	    }
	    break;
	}
	}
	break;

    case IIAPI_EV_REJECT_RCVD :
	switch( state )
	{
	case SQL_CS_WMR :
	{
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
	    smo = smo_buff;

	    if ( msgBuff->flags & IIAPI_MSG_EOD )
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_12 ], *smo );
		smo->smo_next_state = SQL_CS_CONN;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_39 ], *smo );
		smo->smo_next_state = state;
	    }
	    break;
	}
	}
	break;

    case IIAPI_EV_RELEASE_RCVD :
    {
	IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	if ( ! (msgBuff->flags & IIAPI_MSG_EOD) )
	{
	    smo = smo_buff;
	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_39 ], *smo );
	    smo->smo_next_state = state;
    	    break;
	}

	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "%s: Server released connection\n", sql_conn_sm.sm_id );

	switch( state )
	{
	case SQL_CS_CONN :
	case SQL_CS_WMR :
	case SQL_CS_DBEV :
	    smo = smo_buff;
	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_11 ], *smo );
	    smo->smo_next_state = SQL_CS_FAIL;
	    break;

	case SQL_CS_WCR :
	    smo = smo_buff;
	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_25 ], *smo );
	    smo->smo_next_state = SQL_CS_DISC;
	    break;
	}
	break;
    }
    /*
    ** Database event messages are received as a result
    ** of connection, transaction, or statement state
    ** machine activity.  During processing they are
    ** dispatched again with a database event handle.
    ** The original message is processed here, but the
    ** re-dispatched events are ignored.
    */
    case IIAPI_EV_EVENT_RCVD :
	switch( state )
	{
	case SQL_CS_CONN :
	case SQL_CS_WMR :
	    smo = smo_buff;

	    if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)ev_hndl ) )
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_0 ], *smo );
	    else
	    {
		IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

		if ( msgBuff->flags & IIAPI_MSG_EOD )
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_CAS_22 ], *smo);
		else
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_CAS_39 ], *smo);
	    }

	    smo->smo_next_state = state;
	    break;
	    
	case SQL_CS_DBEV :
	    smo = smo_buff;

	    if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)ev_hndl ) )
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_0 ], *smo );
		smo->smo_next_state = state;
	    }
	    else
	    {
		IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

		if ( msgBuff->flags & IIAPI_MSG_EOD )
		{
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_CAS_17 ], *smo);
		    smo->smo_next_state = SQL_CS_CONN;
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_CAS_39 ], *smo);
		    smo->smo_next_state = state;
		}
	    }
	    break;
	}
	break;

    case IIAPI_EV_PROMPT_RCVD :
	switch( state )
	{
	case SQL_CS_WMR :
	{
	    IIAPI_ENVHNDL	*envHndl= connHndl->ch_envHndl;
	    IIAPI_USRPARM	*usrParm = &envHndl->en_usrParm;

	    if( ( usrParm->up_mask1 & IIAPI_UP_PROMPT_FUNC ) )
	    {
		smo = smo_buff;
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_32 ], *smo );
		smo->smo_next_state = SQL_CS_PRMT;
	    }
	    break;
	}
	}
	break;

    /*
    ** Timeouts may occur while waiting for
    ** database events.  These are dispatched
    ** as receive failures, but get turned
    ** into warnings with a return to the
    ** regular connected state.  Any other
    ** error is treated as a connection
    ** failure.
    */
    case IIAPI_EV_RECV_ERROR :
	switch( state )
	{
	case SQL_CS_DBEV :
	    gcaError = parmBlock ? *((STATUS *)parmBlock) : FAIL;
	    smo = smo_buff;

	    if ( gcaError == E_GC0020_TIME_OUT )
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_19 ], *smo );
		smo->smo_next_state = SQL_CS_CONN;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_10 ], *smo );
		smo->smo_next_state = SQL_CS_FAIL;
	    }
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
		  sql_conn_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_24 ], *smo );
	    smo->smo_next_state = state;
	}
	else  if ( event <= IIAPI_EVENT_MSG_MAX )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s Evaluate: invalid message received\n",
		  sql_conn_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_23 ], *smo );
	    smo->smo_next_state = SQL_CS_RLS;
	}
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "%s Evaluate: unexpected I/O completion\n",
		  sql_conn_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_CAS_0 ], *smo );
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
	      sql_conn_sm.sm_id );
	smo = NULL;
    }

    return( smo );
}			    




/*
** Name: sm_execute
**
** Description:
**	Action execution function for the Connection State Machine
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
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	 3-Sep-98 (gordy)
**	    Call functions to abort transaction, statement and
**	    database event handles.
**       9-Aug-99 (rajus01)
**          Asynchronous behaviour of API requires the removal of 
**          handles marked for deletion from the queue. ( Bug #98303 )
**	 7-Sep-00 (gordy)
**	    GCA errors should be marked as failures, not warnings.
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	11-Oct-05 (gordy)
**	    Added HALT action to stop execution.
**	 7-Jul-06 (gordy)
**	    Added XA actions.
**	28-Jul-06 (gordy)
**	    Read response message.
**	25-Mar-10 (gordy)
**	    Replaced GCA formatted interface with byte stream.  
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
    IIAPI_CONNHNDL	*connHndl = (IIAPI_CONNHNDL *)sm_hndl;
    IIAPI_STATUS	status;
    II_BOOL		success = TRUE;

    switch( action )
    {
	case SQL_CA_REMC :
	    /*
	    ** Remember callback
	    */
	    connHndl->ch_callback = TRUE;
	    connHndl->ch_parm = (IIAPI_GENPARM *)parmBlock;
	    break;

	case SQL_CA_CONN :
	    /* 
	    ** Issue connection request 
	    */
	    status = IIapi_connGCA( ev_hndl, 
	    			    ((IIAPI_CONNPARM *)parmBlock)->co_timeout );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_CA_DISC :
	    /*
	    ** Issue disconnect request
	    */
	    status = IIapi_disconnGCA( connHndl );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_CA_RECV :
	{
	    /*
	    ** Issue receive message request.
	    */
	    IIAPI_MSG_BUFF *msgBuff = IIapi_allocMsgBuffer( sm_hndl );

	    if ( ! msgBuff )
		status = IIAPI_ST_OUT_OF_MEMORY;
	    else
		status = IIapi_rcvNormalGCA( ev_hndl, msgBuff, (II_LONG)(-1) );

	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_RCVT :
	{
	    /*
	    ** Issue receive message request with timeout.
	    */
	    IIAPI_MSG_BUFF *msgBuff = IIapi_allocMsgBuffer( sm_hndl );

	    if ( ! msgBuff )
		status = IIAPI_ST_OUT_OF_MEMORY;
	    else
		status = IIapi_rcvNormalGCA( sm_hndl, msgBuff, 
				((IIAPI_GETEVENTPARM *)parmBlock)->gv_timeout );

	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_REDO :
	    /*
	    ** Resume the GCA request.
	    */
	    status = IIapi_resumeGCA( ev_hndl, parmBlock );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_CA_SMDA :
	{
	    /*
	    ** Format and send GCA_MD_ASSOC message.
	    */
	    IIAPI_MSG_BUFF *msgBuff;

	    if ( ! (msgBuff = IIapi_createMsgMDAssoc( connHndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( ev_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
   	    break;
	}
	case SQL_CA_SRLS :
	{
	    /*
	    ** Format and send GCA_RELEASE message.
	    */
	    IIAPI_MSG_BUFF *msgBuff;

	    if ( ! (msgBuff = IIapi_createMsgRelease( connHndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_RNPI :
	    /*
	    ** Read GCA_NP_INTERRUPT message.
	    ** We do not care about the message
	    ** contents, just flag the interrupt.
	    */
	    connHndl->ch_flags |= IIAPI_CH_INTERRUPT;
	    break;

	case SQL_CA_RERR :
	{
	    /*
	    ** Read a GCA_ER_DATA object (GCA_ERROR, GCA_REJECT, GCA_RELEASE)
	    **
	    ** This message object may be split across multiple message
	    ** buffers.  The current (last or only) buffer must be saved
	    ** on the receive queue for processing.  Message buffer is 
	    ** reserved when placed on the receive queue.  It will be 
	    ** released when removed from the queue and processed.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    IIapi_reserveMsgBuffer( msgBuff );
	    QUinsert( &msgBuff->queue, connHndl->ch_rcvQueue.q_prev );

	    status = IIapi_readMsgError( ev_hndl );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_RPMT :
	{
            IIAPI_ENVHNDL	*envHndl= connHndl->ch_envHndl;
    	    IIAPI_PROMPTPARM	*promptParm;
    	    STATUS		temp_status;

	    /*
	    ** Read a GCA_PROMPT_DATA object. ( GCA_PROMPT )
	    */
	    connHndl->ch_prmptParm = MEreqmem( 0, sizeof( IIAPI_PROMPTPARM ),
					 		  TRUE, &temp_status );
	    if ( temp_status != OK )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    promptParm = ( IIAPI_PROMPTPARM * )connHndl->ch_prmptParm; 
	    promptParm->pd_envHandle = 
		    (envHndl == IIapi_defaultEnvHndl()) ? NULL : envHndl;
	    promptParm->pd_connHandle = connHndl; 

	    status = buildPrompt( (IIAPI_MSG_BUFF *)parmBlock, promptParm );

	    if ( status != IIAPI_ST_SUCCESS )
	    {
		success = FALSE;
		break;
	    }
	    break;
	}
	case SQL_CA_CBPM:
	{
            IIAPI_ENVHNDL	*envHndl = connHndl->ch_envHndl;
	    IIAPI_USRPARM	*usrParm = &envHndl->en_usrParm;

	    /*
	    ** Callback to application
	    */
	    IIAPI_TRACE( IIAPI_TR_INFO )
		("sm_execute: Application prompt func callback..\n");

	    (*usrParm->up_prompt_func)
		( (IIAPI_PROMPTPARM *)connHndl->ch_prmptParm );
	    break;
	}
	case SQL_CA_SPRY:
	{
	    /*
	    ** Send a GCA_PRREPLY_DATA object. ( GCA_PROMPT )
	    */
	    IIAPI_PROMPTPARM	*promptParm =
				    (IIAPI_PROMPTPARM *)connHndl->ch_prmptParm; 
	    IIAPI_MSG_BUFF	*msgBuff;

	    if( ! (msgBuff = IIapi_createMsgPRReply( connHndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY ;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;

	    MEfree( (PTR) promptParm->pd_message );
	    MEfree( (PTR) promptParm->pd_reply );
	    MEfree( (PTR) promptParm ); 
	    connHndl->ch_prmptParm = NULL;
	    break;
	}
	case SQL_CA_REVE :
	{
	    /*
	    ** Read GCA_EVENT message.
	    **
	    ** This message object may be split across multiple message
	    ** buffers.  The current (last or only) buffer must be saved
	    ** on the receive queue for processing.  Message buffer is 
	    ** reserved when placed on the receive queue.  It will be 
	    ** released when removed from the queue and processed.
	    */
	    IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

	    IIapi_reserveMsgBuffer( msgBuff );
	    QUinsert( &msgBuff->queue, connHndl->ch_rcvQueue.q_prev );
	    IIapi_createDbevCB( connHndl );
	    break;
	}
	case SQL_CA_CPRM :
	    /*
	    ** Clear connection parameters.
	    */
	    IIapi_clearConnParm( connHndl );
	    break;

	case SQL_CA_SPRM :
	{
	    /*
	    ** Set connection parameter.
	    */
	    IIAPI_SETCONPRMPARM *setConPrmParm = 
	    				(IIAPI_SETCONPRMPARM *)parmBlock;

	    status = IIapi_setConnParm( connHndl, setConPrmParm->sc_paramID, 
					setConPrmParm->sc_paramValue );
	    if ( 
		 status == IIAPI_ST_FAILURE  &&
		 ! IIapi_localError( sm_hndl, E_AP0011_INVALID_PARAM_VALUE, 
				     II_SS22023_INV_PARAM_VAL, status ) 
	       )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;
	}
	case SQL_CA_PEND :
	    /*
	    ** Return connection aborted to all pending operations
	    */
	    {
		IIAPI_TRANHNDL	*tranHndl;
		IIAPI_DBEVHNDL	*dbevHndl;
    
		/*
		** Statement handles are aborted along with the transaction.
		*/
		for( 
		     tranHndl = (IIAPI_TRANHNDL *)connHndl->
						ch_tranHndlList.q_next;
		     tranHndl != (IIAPI_TRANHNDL *)&connHndl->ch_tranHndlList;
		     tranHndl = (IIAPI_TRANHNDL *)tranHndl->
						th_header.hd_id.hi_queue.q_next
		   )
		    IIapi_abortTranHndl( tranHndl, 
					 E_AP0001_CONNECTION_ABORTED,
					 II_SS08006_CONNECTION_FAILURE, 
					 IIAPI_ST_FAILURE );
    
		for( 
		     dbevHndl = (IIAPI_DBEVHNDL *)connHndl->
						ch_dbevHndlList.q_next;
		     dbevHndl != (IIAPI_DBEVHNDL *)&connHndl->ch_dbevHndlList;
		     dbevHndl = (IIAPI_DBEVHNDL *)dbevHndl->
						eh_header.hd_id.hi_queue.q_next
		   )
		    IIapi_abortDbevHndl( dbevHndl, 
					 E_AP0001_CONNECTION_ABORTED,
					 II_SS08006_CONNECTION_FAILURE, 
					 IIAPI_ST_FAILURE );
	    }
	    break;

	case SQL_CA_DELH :
	    /*
	    ** Mark handle for deletion.
	    */
	    QUremove( (QUEUE *)connHndl );
	    sm_hndl->hd_delete = TRUE;
	    break;

	case SQL_CA_ERGC :
	    /*
	    ** GCA error.
	    */
	    if ( parmBlock )
	    {
		STATUS gcaError = *((STATUS *)parmBlock);

		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "%s: GCA request failed 0x%x\n", 
		      sql_conn_sm.sm_id, gcaError );

		if ( ! IIapi_localError( ev_hndl, gcaError, 
					 II_SS08004_CONNECTION_REJECTED,
					 IIAPI_ST_FAILURE ) )
		{
		    status = IIAPI_ST_OUT_OF_MEMORY;
		    success = FALSE;
		}
	    }
	    break;

	case SQL_CA_OTRM :
	{
            IIAPI_ENVHNDL	*envHndl = connHndl->ch_envHndl;
	    IIAPI_USRPARM	*usrParm = &envHndl->en_usrParm;

	    /*
	    ** Send GCA error message to the application if it
	    ** had been requested.
	    */
	    if ( ( usrParm->up_mask1 & IIAPI_UP_TRACE_FUNC ) )
	    {
		IIAPI_TRACEPARM	traceParm;
		IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)parmBlock;

		IIAPI_TRACE( IIAPI_TR_INFO )
			("sm_execute: Application trace func callback...\n");

		traceParm.tr_connHandle = connHndl;
		traceParm.tr_envHandle = (envHndl == IIapi_defaultEnvHndl())
					 ? NULL : envHndl;

		traceParm.tr_message = MEreqmem( 0, msgBuff->length + 1,
						 TRUE, NULL );
		if ( ! traceParm.tr_message )
		{
		    IIAPI_TRACE( IIAPI_TR_ERROR )
			( "%s: can't allocate memory for message...\n",
			  sql_conn_sm.sm_id );

		    success = FALSE;
		}
		else
		{
		    MEcopy( msgBuff->data, 
			    msgBuff->length, traceParm.tr_message );
		    traceParm.tr_message[ traceParm.tr_length ] = EOS;
		    traceParm.tr_length = msgBuff->length;
		    msgBuff->length = 0;

		    (*usrParm->up_trace_func)( &traceParm );
		    MEfree( (PTR)traceParm.tr_message );
		}
	    }
	}
	break;

	case SQL_CA_ERIP :
	    /*
	    ** Invalid connection parameter.
	    */
	    IIAPI_TRACE( IIAPI_TR_WARNING )
		( "%s: Invalid connection parameter\n", sql_conn_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP000E_INVALID_CONNECT_PARM, 
				     II_SS08004_CONNECTION_REJECTED,
				     IIAPI_ST_WARNING ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
   	    break; 

	case SQL_CA_ERAB :
	    /*
	    ** Connection failure.
	    */
	    if ( ! IIapi_localError( sm_hndl, E_AP0001_CONNECTION_ABORTED, 
				     II_SS08006_CONNECTION_FAILURE,
				     IIAPI_ST_FAILURE ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_CA_CBOK :
	    /*
	    ** Callback with success.  Note that
	    ** this will pick up the most severe
	    ** status from the errors associated
	    ** with the event handle.
	    */
	    if ( connHndl->ch_callback )
	    {
		IIapi_appCallback(connHndl->ch_parm, ev_hndl, IIAPI_ST_SUCCESS);
		connHndl->ch_callback = FALSE;
	    }
   	    break;

	case SQL_CA_CBWN :
	    /*
	    ** Callback with warning.
	    */
	    if ( connHndl->ch_callback )
	    {
		IIapi_appCallback(connHndl->ch_parm, ev_hndl, IIAPI_ST_WARNING);
		connHndl->ch_callback = FALSE;
	    }
	    break;

	case SQL_CA_CBIF :
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
	    ** This may not have been a connection
	    ** related function, and we may have a
	    ** callback saved on the connection
	    ** handle, so we carefully make the
	    ** callback to the caller making sure
	    ** not to disturb the connection handle.
	    */
	    IIapi_appCallback( (IIAPI_GENPARM *)parmBlock, sm_hndl, status );
	    break;

	case SQL_CA_CBAB :
	    /*
	    ** Callback with connection failure.
	    */
	    if ( connHndl->ch_callback )
	    {
		if ( ! IIapi_localError( sm_hndl, E_AP0001_CONNECTION_ABORTED, 
					 II_SS08006_CONNECTION_FAILURE,
					 IIAPI_ST_FAILURE ) )
		    status = IIAPI_ST_OUT_OF_MEMORY;
		else
		    status = IIAPI_ST_FAILURE;

		IIapi_appCallback( connHndl->ch_parm, sm_hndl, status );
		connHndl->ch_callback = FALSE;
	    }
	    break;

	case SQL_CA_CBDC :
	    /*
	    ** API abort function is called while disconnect is in
	    ** progress. Callback with connection failure.
	    */
	    if ( connHndl->ch_callback )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ("%s: Disconnect in progress\n", sql_conn_sm.sm_id);

	        if ( ! IIapi_localError( sm_hndl, E_AP0018_INVALID_DISCONNECT, 
				     II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				     IIAPI_ST_FAILURE ) )
		    status = IIAPI_ST_OUT_OF_MEMORY;
	    	else
		    status = IIAPI_ST_FAILURE;

		IIapi_appCallback( connHndl->ch_parm, sm_hndl, status );

		connHndl->ch_callback = FALSE;
	    }
	    break;

	case SQL_CA_SXAP :
	{
	    /*
	    ** Send XA Prepare message.
	    */
	    IIAPI_XAPREPPARM	*prepParm = (IIAPI_XAPREPPARM *)parmBlock;
	    IIAPI_MSG_BUFF	*msgBuff;

	    if ( ! (msgBuff = IIapi_createMsgXA( sm_hndl, GCA_XA_PREPARE, 
					&prepParm->xp_tranID.ti_value.xaXID, 
					prepParm->xp_flags )) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_SXAC :
	{
	    /*
	    ** Send XA Commit message.
	    */
	    IIAPI_XACOMMITPARM	*commitParm = (IIAPI_XACOMMITPARM *)parmBlock;
	    IIAPI_MSG_BUFF	*msgBuff;

	    if ( ! (msgBuff = IIapi_createMsgXA( sm_hndl, GCA_XA_COMMIT, 
					&commitParm->xc_tranID.ti_value.xaXID, 
					commitParm->xc_flags )) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_SXAR :
	{
	    /*
	    ** Send XA Rollback message.
	    */
	    IIAPI_XAROLLPARM	*rollParm = (IIAPI_XAROLLPARM *)parmBlock;
	    IIAPI_MSG_BUFF	*msgBuff;

	    if ( ! (msgBuff = IIapi_createMsgXA( sm_hndl, GCA_XA_ROLLBACK, 
					&rollParm->xr_tranID.ti_value.xaXID, 
					rollParm->xr_flags )) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_CA_RXAR :
	{
	    /*
	    ** Read XA response message.
	    */
	    IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
	    GCA_RE_DATA		respData;

	    if ( (status = IIapi_readMsgResponse( msgBuff, &respData, TRUE ))
							!= IIAPI_ST_SUCCESS )
	    {
		success = FALSE;
		break;
	    }
	    
	    /*
	    ** If no XA error is returned, generate generic XA error.
	    */
	    if ( ! (respData.gca_rqstatus & GCA_XA_ERROR_MASK)  ||
	         ! respData.gca_errd5 )  
		respData.gca_errd5 = IIAPI_XAER_RMERR;

	    if ( ! IIapi_xaError( ev_hndl, respData.gca_errd5 ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
	    break;
	}
	case SQL_CA_RRSP :
	{
	    /*
	    ** Read GCA_RESPONSE message.
	    */
	    IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
	    GCA_RE_DATA		respData;

	    if ( (status = IIapi_readMsgResponse( msgBuff, &respData, TRUE ))
							!= IIAPI_ST_SUCCESS )
	    {
		success = FALSE;
		break;
	    }
	    
	    /*
	    ** Check for an XA error code.
	    */
	    if ( 
	         respData.gca_rqstatus & GCA_XA_ERROR_MASK  &&
	         respData.gca_errd5  && 
	         ! IIapi_xaError( ev_hndl, respData.gca_errd5 ) 
	       )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
	    break;
	}
	case SQL_CA_QBUF :
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
        case SQL_CA_HALT :
	    /*
	    ** Halt state machine execution.
	    */
	    return( FALSE );
    }

    /*
    ** If we couldn't complete the action, callback with failure.
    */
    if ( ! success  &&  connHndl->ch_callback )
    {
	IIapi_appCallback( connHndl->ch_parm, ev_hndl, status );
	connHndl->ch_callback = FALSE;
    }

    return( success );
}


/*
** Name: IIapi_buildPrompt
**
** Description:
**    This function builds prompt data from GCA_PROMPT message.
**
** Input:
**   msgBuff		GCA buffer containing GCA_PROMPT message.
**   promptParm		Prompt parameter Block.
**
** Output:
**   None.
**
** Returns:
**      IIAPI_STATUS    API status.
**
** History:
**	14-Oct-98 (rajus01)
**	    Created
**	25-Mar-10 (gordy)
**	    Replaced GCA formatted interface with byte stream.  
**	    Enhanced parameter memory block handling.
*/

static IIAPI_STATUS 
buildPrompt( IIAPI_MSG_BUFF *msgBuff, IIAPI_PROMPTPARM *promptParm )
{
    GCA_PROMPT_DATA     prData;
    char		*text;
    i4			length;
    STATUS		status;

    if ( (status = IIapi_readMsgPrompt( msgBuff, &prData, &length, &text )) 
							!= IIAPI_ST_SUCCESS )
	return( status );

    if ( prData.gca_prflags & GCA_PR_NOECHO )
	promptParm->pd_flags |= IIAPI_PR_NOECHO;

    if ( prData.gca_prflags & GCA_PR_TIMEOUT )
	promptParm->pd_flags |= IIAPI_PR_TIMEOUT;

    promptParm->pd_timeout = prData.gca_prtimeout;
    promptParm->pd_max_reply = prData.gca_prmaxlen;

    if ( length )
    {
	promptParm->pd_message = (II_CHAR *)MEreqmem( 0, length + 1, 
						      FALSE, NULL );
	if ( ! promptParm->pd_message )  return( IIAPI_ST_OUT_OF_MEMORY );

	promptParm->pd_reply = (II_CHAR *)MEreqmem( 0, prData.gca_prmaxlen + 1,
						    FALSE, NULL );
	if ( ! promptParm->pd_reply )
	{
	   MEfree( (PTR)promptParm->pd_message );
	   return( IIAPI_ST_OUT_OF_MEMORY );
	}

	MEcopy( text, length, promptParm->pd_message );  
	promptParm->pd_message[ length ] = EOS;
	promptParm->pd_msg_len = length;
    }

    return( IIAPI_ST_SUCCESS );
}


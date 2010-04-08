/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apisql.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apisphnd.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apimsg.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisqlt.c
**
** Description:
**	SQL Transaction State Machine.
**
** History:  Original history found in apitstbl.c (now defunct).
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	12-Feb-97 (gordy)
**	    Fixing the action count symbol.
**	 3-Jul-97 (gordy)
**	    Re-worked state machine initialization.  Added
**	    IIapi_sql_tinit().
**	 3-Sep-98 (gordy)
**	    Call IIapi_abortStmtHndl() for transaction aborts.
**	14-Oct-98 (rajus01)
**	    Ignore IIAPI_EV_PROMPT_RCVD message.
**	19-Jan-99 (gordy)
**	    Combine transaction abort (DONE) and connection
**	    aborts (ABORT) into single state.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_commit()
**	    and IIapi_rollback().
**	 05-07-99 (rajus01)
**          Fix for Bug 96844. [ IIapi_autocommit() fails following an
**          IIapi_commit() call for a transaction which contained
**          a single statement which was cancelled with IIapi_cancel() ].
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Mar-01 (gordy)
**	    Disassociate transaction from distributed transaction
**	    name prior to making callback.
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	11-Oct-05 (gordy)
**	    Provide more explicit handling for operations after a
**	    transaction abort.
**	20-Jul-06 (gordy)
**	    Permit regular commit and rollback on XA transaction.
**	15-Mar-07 (gordy)
**	    Added support for scrollable cursors.
**	25-Mar-10 (gordy)
**	    Support batch processing.  Pass GCA message type to 
**	    IIapi_createGCAEmpty().  Enhanced parameter memory 
**	    block handling.
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
** SQL Transaction States.
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
**	10-Jul-06 (gordy)
**	    Added XA states.
*/

# define SQL_TS_IDLE	((IIAPI_STATE)  0)	/* Idle */
# define SQL_TS_BEG	((IIAPI_STATE)  1)	/* Begin transaction */
# define SQL_TS_TRAN	((IIAPI_STATE)  2)	/* Transaction active */
# define SQL_TS_AUTO	((IIAPI_STATE)  3)	/* Autocommit transaction */
# define SQL_TS_XA	((IIAPI_STATE)  4)	/* XA Transaction active */
# define SQL_TS_PREP	((IIAPI_STATE)  5)	/* Prepared 2PC */
# define SQL_TS_ABORT	((IIAPI_STATE)  6)	/* Transaction aborted */
# define SQL_TS_COMM	((IIAPI_STATE)  7)	/* Commit sent */
# define SQL_TS_WCR	((IIAPI_STATE)  8)	/* Wait for commit response */
# define SQL_TS_ROLL	((IIAPI_STATE)  9)	/* Rollback sent */
# define SQL_TS_WRR	((IIAPI_STATE) 10)	/* Wait for rollback response */
# define SQL_TS_AON	((IIAPI_STATE) 11)	/* Set auto on sent */
# define SQL_TS_WONR	((IIAPI_STATE) 12)	/* Wait for auto on response */
# define SQL_TS_AOFF	((IIAPI_STATE) 13)	/* Set auto off sent */
# define SQL_TS_WOFR	((IIAPI_STATE) 14)	/* Wait for auto off response */
# define SQL_TS_SAVE	((IIAPI_STATE) 15)	/* Savepoint sent */
# define SQL_TS_WSPR	((IIAPI_STATE) 16)	/* Wait savepoint response */
# define SQL_TS_RBSP	((IIAPI_STATE) 17)	/* Rollback to savepoint sent */
# define SQL_TS_WRSR	((IIAPI_STATE) 18)	/* Wait rollback sp response */
# define SQL_TS_SEC	((IIAPI_STATE) 19)	/* 2PC secure (prepare) sent */
# define SQL_TS_WSR	((IIAPI_STATE) 20)	/* Wait for secure response */
# define SQL_TS_XASTR	((IIAPI_STATE) 21)	/* XA Start sent */
# define SQL_TS_WXSR	((IIAPI_STATE) 22)	/* Wait for XA Start response */
# define SQL_TS_XAEND	((IIAPI_STATE) 23)	/* XA End sent */
# define SQL_TS_WXER	((IIAPI_STATE) 24)	/* Wait for XA End response */
# define SQL_TS_CONN	((IIAPI_STATE) 25)	/* Connect 2PC transaction */

# define SQL_TS_CNT	(SQL_TS_CONN + 1)

static char *sql_ts_id[] =
{
    "IDLE",  "BEG",   "TRAN",  "AUTO",  "XA",    
    "PREP",  "ABORT", "COMM",  "WCR",   "ROLL",  "WRR",   
    "AON",   "WONR",  "AOFF",  "WOFR",  "SAVE",  "WSPR",  
    "RBSP",  "WRSR",  "SEC",   "WSR",
    "XASTR", "WXSR",  "XAEND", "WXER",  "CONN"
};

/*
** SQL Transaction actions
**
** The following values are used as array
** indices.  Care must be taken when adding
** new entries to ensure there are no gaps
** or duplicates and all associated arrays
** are kept in sync with these definitions.
**
** History:
**	11-Oct-05 (gordy)
**	    Added action to halt state machine execution.
**	10-Jul-06 (gordy)
**	    Added XA actions.
**	28-Jul-06 (gordy)
**	    Add action to read response message.
*/

# define SQL_TA_REMC	((IIAPI_ACTION)  0)	/* Remember callback */
# define SQL_TA_RECV	((IIAPI_ACTION)  1)	/* Receive message */
# define SQL_TA_SCOM	((IIAPI_ACTION)  2)	/* Send commit */
# define SQL_TA_SRB	((IIAPI_ACTION)  3)	/* Send rollback */
# define SQL_TA_SSP	((IIAPI_ACTION)  4)	/* Send savepoint */
# define SQL_TA_SRBS	((IIAPI_ACTION)  5)	/* Send rollback to <sp> */
# define SQL_TA_SAON	((IIAPI_ACTION)  6)	/* Send autocommit on */
# define SQL_TA_SAOF	((IIAPI_ACTION)  7)	/* Send autocommit off */
# define SQL_TA_SSEC	((IIAPI_ACTION)  8)	/* Send secure */
# define SQL_TA_SXAS	((IIAPI_ACTION)  9)	/* Send XA Start */
# define SQL_TA_SXAE	((IIAPI_ACTION) 10)	/* Send XA End */
# define SQL_TA_PEND	((IIAPI_ACTION) 11)	/* Abort pending operations */
# define SQL_TA_DELH	((IIAPI_ACTION) 12)	/* Delete transaction handle */
# define SQL_TA_DELX	((IIAPI_ACTION) 13)	/* Delete XA trans handle */
# define SQL_TA_ERAB	((IIAPI_ACTION) 14)	/* Transaction aborted */
# define SQL_TA_ERCF	((IIAPI_ACTION) 15)	/* Commit failed */
# define SQL_TA_ERPC	((IIAPI_ACTION) 16)	/* 2PC failed */
# define SQL_TA_RRSP	((IIAPI_ACTION) 17)	/* Read response message */
# define SQL_TA_RXAR	((IIAPI_ACTION) 18)	/* Read XA error response*/
# define SQL_TA_CBOK	((IIAPI_ACTION) 19)	/* Callback Success */
# define SQL_TA_CBFL	((IIAPI_ACTION) 20)	/* Callback Failure */
# define SQL_TA_CBIF	((IIAPI_ACTION) 21)	/* Callback Invalid Function */
# define SQL_TA_CBAB	((IIAPI_ACTION) 22)	/* Callback Abort */
# define SQL_TA_CBABX	((IIAPI_ACTION) 23)	/* Callback XA Abort */
# define SQL_TA_RCVE    ((IIAPI_ACTION) 24)	/* Receive Error */
# define SQL_TA_HALT	((IIAPI_ACTION) 25)	/* Halt SM execution */

# define SQL_TA_CNT	(SQL_TA_HALT + 1)

static char *sql_ta_id[] =
{
    "REMC", "RECV", "SCOM", "SRB",  "SSP",  "SRBS", 
    "SAON", "SAOF", "SSEC", "SXAS", "SXAE", "PEND", 
    "DELH", "DELX", "ERAB", "ERCF", "ERPC", "RRSP", "RXAR", 
    "CBOK", "CBFL", "CBIF", "CBAB", "CBABX","RCVE", 
    "HALT"
};

/*
** Name: sql_tran_sm
**
** Description:
**	The SQL Transaction State Machine Interface
*/

static IIAPI_SM sql_tran_sm =
{
    "SQL Tran",
    SQL_TS_CNT,
    sql_ts_id,
    SQL_TA_CNT,
    sql_ta_id,
    sm_evaluate,
    sm_execute,
    SQL_TS_ABORT
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
**	19-Jan-99 (gordy)
**	    Make sure transaction abort error created even if
**	    no callback on current handle (TAS_7).
**	11-Oct-05 (gordy)
**	    Handle invalid function call after transaction abort.
**	10-Jul-06 (gordy)
**	    Added action sequences for XA support.
**	28-Jul-06 (gordy)
**	    Read response message.
*/

static IIAPI_SM_OUT sql_act_seq[] =
{
    { 0, 0, { 0, 0, 0 } },					/* TAS_0  */
    { 0, 1, { SQL_TA_RECV, 0, 0 } },				/* TAS_1  */
    { 0, 3, { SQL_TA_RRSP, SQL_TA_ERCF, SQL_TA_CBFL } },	/* TAS_2  */
    { 0, 3, { SQL_TA_RRSP, SQL_TA_ERPC, SQL_TA_CBFL } },	/* TAS_3  */
    { 0, 2, { SQL_TA_RRSP, SQL_TA_CBOK, 0 } },			/* TAS_4  */
    { 0, 3, { SQL_TA_RRSP, SQL_TA_DELH, SQL_TA_CBOK } },	/* TAS_5  */
    { 0, 1, { SQL_TA_DELH, 0, 0 } },				/* TAS_6  */
    { 0, 3, { SQL_TA_PEND, SQL_TA_ERAB, SQL_TA_CBFL } },	/* TAS_7  */
    { 0, 3, { SQL_TA_REMC, SQL_TA_DELH, SQL_TA_CBAB } },	/* TAS_8  */
    { 0, 2, { SQL_TA_RRSP, SQL_TA_CBFL, 0 } },			/* TAS_9  */
    { 0, 3, { SQL_TA_REMC, SQL_TA_DELH, SQL_TA_CBOK } },	/* TAS_10 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SSP, 0 } },			/* TAS_11 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SRBS, 0 } },			/* TAS_12 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SRB, 0 } },			/* TAS_13 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SSEC, 0 } },			/* TAS_14 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SCOM, 0 } },			/* TAS_15 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SAON, 0 } },			/* TAS_16 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SAOF, 0 } },			/* TAS_17 */
    { 0, 3, { SQL_TA_RRSP, SQL_TA_DELH, SQL_TA_CBFL } },	/* TAS_18 */
    { 0, 2, { SQL_TA_CBIF, SQL_TA_HALT, 0 } },			/* TAS_19 */
    { 0, 2, { SQL_TA_RCVE, SQL_TA_HALT, 0 } },			/* TAS_20 */
    { 0, 3, { SQL_TA_REMC, SQL_TA_CBAB, SQL_TA_HALT } },	/* TAS_21 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SXAS, 0 } },			/* TAS_22 */
    { 0, 2, { SQL_TA_REMC, SQL_TA_SXAE, 0 } },			/* TAS_23 */
    { 0, 3, { SQL_TA_RRSP, SQL_TA_DELX, SQL_TA_CBOK } },	/* TAS_24 */
    { 0, 3, { SQL_TA_RXAR, SQL_TA_DELX, SQL_TA_CBOK } },	/* TAS_25 */
    { 0, 3, { SQL_TA_REMC, SQL_TA_DELX, SQL_TA_CBABX } }	/* TAS_26 */
};

/*
** Indexes into the action sequence array.
*/

# define SQL_TAS_0	0
# define SQL_TAS_1	1
# define SQL_TAS_2	2
# define SQL_TAS_3	3
# define SQL_TAS_4	4
# define SQL_TAS_5	5
# define SQL_TAS_6	6
# define SQL_TAS_7	7
# define SQL_TAS_8	8
# define SQL_TAS_9	9
# define SQL_TAS_10	10
# define SQL_TAS_11	11
# define SQL_TAS_12	12
# define SQL_TAS_13	13
# define SQL_TAS_14	14
# define SQL_TAS_15	15
# define SQL_TAS_16	16
# define SQL_TAS_17	17
# define SQL_TAS_18	18
# define SQL_TAS_19	19
# define SQL_TAS_20	20
# define SQL_TAS_21	21
# define SQL_TAS_22	22
# define SQL_TAS_23	23
# define SQL_TAS_24	24
# define SQL_TAS_25	25
# define SQL_TAS_26	26

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
**	19-Jan-99 (gordy)
**	    Combine transaction abort (DONE) and connection
**	    aborts (ABORT) into single state.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_commit().
**	11-Oct-05 (gordy)
**	    Explicitly disallow functions in ABORT state which would
**	    otherwise be handled as invalid function sequence.
**	 7-Jul-06 (gordy)
**	    Support XA functions and responses.
**	20-Jul-06 (gordy)
**	    Permit regular commit on XA transaction.
**	15-Mar-07 (gordy)
**	    Added position and scroll functions.
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
    ** API Functions which directly affect the transaction state.
    */
    { IIAPI_EV_AUTO_FUNC,	SQL_TS_IDLE,	SQL_TS_AON,	SQL_TAS_16 },
    { IIAPI_EV_AUTO_FUNC,	SQL_TS_AUTO,	SQL_TS_AOFF,	SQL_TAS_17 },
    { IIAPI_EV_AUTO_FUNC,	SQL_TS_ABORT,	SQL_TS_IDLE,	SQL_TAS_10 },
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_IDLE,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_IDLE,	SQL_TS_IDLE,	SQL_TAS_10 },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_BEG,	SQL_TS_IDLE,	SQL_TAS_10 },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_TRAN,	SQL_TS_COMM,	SQL_TAS_15 },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_XA,	SQL_TS_XAEND,	SQL_TAS_15 },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_PREP,	SQL_TS_COMM,	SQL_TAS_15 },
    { IIAPI_EV_COMMIT_FUNC,	SQL_TS_ABORT,	SQL_TS_IDLE,	SQL_TAS_8  },
    { IIAPI_EV_PRECOMMIT_FUNC,	SQL_TS_BEG,	SQL_TS_SEC,	SQL_TAS_14 },
    { IIAPI_EV_PRECOMMIT_FUNC,	SQL_TS_TRAN,	SQL_TS_SEC,	SQL_TAS_14 },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_IDLE,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_SAVEPOINT_FUNC,	SQL_TS_BEG,	SQL_TS_SAVE,	SQL_TAS_11 },
    { IIAPI_EV_SAVEPOINT_FUNC,	SQL_TS_TRAN,	SQL_TS_SAVE,	SQL_TAS_11 },
    { IIAPI_EV_XASTART_FUNC,	SQL_TS_IDLE,	SQL_TS_XASTR,	SQL_TAS_22 },
    { IIAPI_EV_XAEND_FUNC,	SQL_TS_XA,	SQL_TS_XAEND,	SQL_TAS_23 },
    { IIAPI_EV_XAEND_FUNC,	SQL_TS_ABORT,	SQL_TS_IDLE,	SQL_TAS_26 },

    /*
    ** The following functions are permitted while transactions, 
    ** are active but do not directly affect the transaction state.
    */
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CANCEL_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CLOSE_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_GETQINFO_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },

    /*
    ** The following functions are not permitted after a transaction abort.
    */
    { IIAPI_EV_BATCH_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_GETCOLUMN_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_GETCOPYMAP_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_GETDESCR_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_POSITION_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_PRECOMMIT_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_PUTCOLUMN_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_PUTPARM_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_QUERY_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_SAVEPOINT_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_SCROLL_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },
    { IIAPI_EV_SETDESCR_FUNC,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_21 },

    /*
    ** The following messages are processed at the transaction level.
    */
    { IIAPI_EV_DONE_RCVD,	SQL_TS_WCR,	SQL_TS_IDLE,	SQL_TAS_5  },
    { IIAPI_EV_DONE_RCVD,	SQL_TS_WRR,	SQL_TS_IDLE,	SQL_TAS_5  },
    { IIAPI_EV_DONE_RCVD,	SQL_TS_WSR,	SQL_TS_PREP,	SQL_TAS_4  },
    { IIAPI_EV_DONE_RCVD,	SQL_TS_WXSR,	SQL_TS_XA,	SQL_TAS_4  },
    { IIAPI_EV_DONE_RCVD,	SQL_TS_WXER,	SQL_TS_IDLE,	SQL_TAS_24 },
    { IIAPI_EV_REFUSE_RCVD,	SQL_TS_WCR,	SQL_TS_TRAN,	SQL_TAS_2  },
    { IIAPI_EV_REFUSE_RCVD,	SQL_TS_WSR,	SQL_TS_TRAN,	SQL_TAS_3  },
    { IIAPI_EV_REFUSE_RCVD,	SQL_TS_WXSR,	SQL_TS_IDLE,	SQL_TAS_25 },
    { IIAPI_EV_REFUSE_RCVD,	SQL_TS_WXER,	SQL_TS_IDLE,	SQL_TAS_25 },

    /*
    ** The following events may occur when
    ** connecting a distributed transaction.
    */
    { IIAPI_EV_CONNECT_FUNC,	SQL_TS_IDLE,	SQL_TS_IDLE,	SQL_TAS_0  },
    { IIAPI_EV_ACCEPT_RCVD,	SQL_TS_IDLE,	SQL_TS_CONN,	SQL_TAS_0  },
    { IIAPI_EV_ACCEPT_RCVD,	SQL_TS_CONN,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CONNECT_CMPL,	SQL_TS_IDLE,	SQL_TS_IDLE,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_CONN,	SQL_TS_CONN,	SQL_TAS_0  },
    { IIAPI_EV_SEND_ERROR,	SQL_TS_IDLE,	SQL_TS_IDLE,	SQL_TAS_6  },
    { IIAPI_EV_SEND_ERROR,	SQL_TS_CONN,	SQL_TS_IDLE,	SQL_TAS_6  },
    { IIAPI_EV_RECV_ERROR,	SQL_TS_IDLE,	SQL_TS_IDLE,	SQL_TAS_6  },
    { IIAPI_EV_RECV_ERROR,	SQL_TS_CONN,	SQL_TS_IDLE,	SQL_TAS_6  },

    /*
    ** The following messages are ignored during transactions.
    ** They are the results of actions in other state machines.
    */
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CFROM_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_CINTO_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_BEG,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_IACK_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_QCID_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_RESPONSE_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_RETPROC_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_TDESCR_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_TUPLE_RCVD,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },

    /*
    ** GCA request completions.
    */
    { IIAPI_EV_SEND_CMPL,	SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_COMM,	SQL_TS_WCR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_ROLL,	SQL_TS_WRR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_AON,	SQL_TS_WONR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_AOFF,	SQL_TS_WOFR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_SAVE,	SQL_TS_WSPR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_RBSP,	SQL_TS_WRSR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_SEC,	SQL_TS_WSR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_XASTR,	SQL_TS_WXSR,	SQL_TAS_1  },
    { IIAPI_EV_SEND_CMPL,	SQL_TS_XAEND,	SQL_TS_WXER,	SQL_TAS_1  },
    { IIAPI_EV_DONE,		SQL_TS_BEG,	SQL_TS_BEG,	SQL_TAS_0  },
    { IIAPI_EV_DONE,		SQL_TS_TRAN,	SQL_TS_TRAN,	SQL_TAS_0  },
    { IIAPI_EV_DONE,		SQL_TS_AUTO,	SQL_TS_AUTO,	SQL_TAS_0  },
    { IIAPI_EV_DONE,		SQL_TS_XA,	SQL_TS_XA,	SQL_TAS_0  },
    { IIAPI_EV_DONE,		SQL_TS_PREP,	SQL_TS_PREP,	SQL_TAS_0  },
    { IIAPI_EV_DONE,		SQL_TS_ABORT,	SQL_TS_ABORT,	SQL_TAS_0  },
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

static SM_TRANSITION	*smt_array[ IIAPI_EVENT_CNT ][ SQL_TS_CNT ] ZERO_FILL;
static II_BOOL		initialized = FALSE;



/*
** Name: IIapi_sql_tinit
**
** Description:
**	Initialize the SQL Transaction state machine.
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
IIapi_sql_tinit( VOID )
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

    IIapi_sm[ IIAPI_SMT_SQL ][ IIAPI_SMH_TRAN ] = &sql_tran_sm;

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
**	19-Jan-99 (gordy)
**	    Combine transaction abort (DONE) and connection
**	    aborts (ABORT) into single state.
**	 9-Feb-99 (gordy)
**	    Handles in IDLE state should allow IIapi_rollback().
**	11-Oct-05 (gordy)
**	    Rollback to savepoint not allowed after transaction abort.
**	20-Jul-06 (gordy)
**	    Permit regular rollback on XA transaction.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
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
    IIAPI_TRANHNDL	*tranHndl = (IIAPI_TRANHNDL *)sm_hndl;
    IIAPI_SM_OUT	*smo = NULL;
    SM_TRANSITION	*smt;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "%s evaluate: evaluating event %s in state %s\n",
	  sql_tran_sm.sm_id, IIAPI_PRINTEVENT( event ),
	  IIAPI_PRINT_ID( state, SQL_TS_CNT, sql_ts_id ) );

    /*
    ** Static evaluations depend solely on the current state and event.
    */
    if ( (smt = smt_array[ event ][ state ]) )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "%s evaluate: static evaluation\n", sql_tran_sm.sm_id );

	smo = smo_buff;
	STRUCT_ASSIGN_MACRO( sql_act_seq[ smt->smt_action ], *smo );
	smo->smo_next_state = smt->smt_next;
    }
    else		/* Dynamic evaluations require additional predicates */
    switch( event )	/* to determine the correct state transition.	     */
    {
	/*
	** Rollback requests may be for savepoints
	** or the entire transaction.
	*/
	case IIAPI_EV_ROLLBACK_FUNC :
	    if ( ((IIAPI_ROLLBACKPARM *)parmBlock)->rb_savePointHandle )
	    {
		/*
		** Rollback to savepoint.
		*/
		switch( state )
		{
		    case SQL_TS_TRAN :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_12 ], *smo );
			smo->smo_next_state = SQL_TS_RBSP;
			break;

		    case SQL_TS_ABORT :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_21 ], *smo );
			smo->smo_next_state = state;
			break;
		}
	    }
	    else
	    {
		/*
		** Rollback transaction.
		*/
		switch( state )
		{
		    case SQL_TS_IDLE :
		    case SQL_TS_BEG :
		    case SQL_TS_ABORT :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_10 ], *smo );
			smo->smo_next_state = SQL_TS_IDLE;
			break;

		    case SQL_TS_TRAN :
		    case SQL_TS_PREP :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_13 ], *smo );
			smo->smo_next_state = SQL_TS_ROLL;
			break;

		    case SQL_TS_XA :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_13 ], *smo );
			smo->smo_next_state = SQL_TS_XAEND;
			break;
		}
	    }
	    break;

	/*
	** GCA_RESPONSE messages include the current DBMS 
	** transaction state, allowing detection of the 
	** start of a transaction and transaction aborts.
	** In some cases, the query status is also of
	** interest.
	*/
	case IIAPI_EV_RESPONSE_RCVD :
	{
	    /*
	    ** A fake GCA_RESPONSE message is generated for some cases 
	    ** of procedure execution.  The GCA message buffer will be 
	    ** NULL in this case and it is assumed that the procedure
	    ** executed successfully.
	    **
	    ** The RESPONSE message is not consumed during evaluation.
	    */
	    GCA_RE_DATA	respData;

	    if ( ! parmBlock  ||
	         IIapi_readMsgResponse((IIAPI_MSG_BUFF *)parmBlock,
					&respData, FALSE) != IIAPI_ST_SUCCESS )
		respData.gca_rqstatus = 0;

	    if ( respData.gca_rqstatus & GCA_LOG_TERM_MASK )
	    {
		/*
		** Transaction inactive (possibly aborted).
		*/
		switch( state )
		{
		    case SQL_TS_BEG :
		    case SQL_TS_ABORT :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_0 ], *smo );
			smo->smo_next_state = state;
			break;

		    case SQL_TS_TRAN :
		    case SQL_TS_XA :
		    case SQL_TS_PREP :
		    case SQL_TS_WSPR :
		    case SQL_TS_WRSR :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_7 ], *smo );
			smo->smo_next_state = SQL_TS_ABORT;
			break;
		}
	    }
	    else
	    {
		/*
		** Transaction active.
		*/
		switch( state )
		{
		    case SQL_TS_BEG :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_0 ], *smo );
			smo->smo_next_state = SQL_TS_TRAN;
			break;

		    case SQL_TS_TRAN :
		    case SQL_TS_XA :
		    case SQL_TS_PREP :
		    case SQL_TS_ABORT :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_0 ], *smo );
			smo->smo_next_state = state;
			break;

		    case SQL_TS_WSPR :
		    case SQL_TS_WRSR :
			smo = smo_buff;

			if ( respData.gca_rqstatus & GCA_FAIL_MASK )
			    STRUCT_ASSIGN_MACRO(sql_act_seq[SQL_TAS_9], *smo);
			else
			    STRUCT_ASSIGN_MACRO(sql_act_seq[SQL_TAS_4], *smo);

			smo->smo_next_state = SQL_TS_TRAN;
			break;
		}
	    }

	    if ( respData.gca_rqstatus & GCA_FAIL_MASK )
	    {
		/*
		** Request failed.
		*/
		switch( state )
		{
		    case SQL_TS_WONR :
		    case SQL_TS_WOFR :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_18 ], *smo );
			smo->smo_next_state = SQL_TS_IDLE;
			break;
		}
	    }
	    else
	    {
		/*
		** Request succedded.
		*/
		switch( state )
		{
		    case SQL_TS_WONR :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_TAS_4 ], *smo);
			smo->smo_next_state = SQL_TS_AUTO;
			break;

		    case SQL_TS_WOFR :
			smo = smo_buff;
			STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_TAS_5 ], *smo);
			smo->smo_next_state = SQL_TS_IDLE;
			break;
		}
	    }
	    break;
	}
	/*
	** GCA_REJECT messages should only occur in
	** the connected state.  They are processed 
	** by the connection SM.  When significant, 
	** only handle when EOD.
	*/
	case IIAPI_EV_REJECT_RCVD :
	    switch( state )
	    {
	    case SQL_TS_CONN :
	    {
		IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		smo = smo_buff;

		if ( msgBuff->flags & IIAPI_MSG_EOD )
		{
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_TAS_6 ], *smo);
		    smo->smo_next_state = SQL_TS_IDLE;
		}
		else
		{
		    smo->smo_action_cnt = 0;
		    smo->smo_next_state = state;
		}
		break;
	    }
	    }
	    break;

	/*
	** GCA_RELEASE messages are mostly ignored.
	** They are processed by the connection SM.
	** When significant, only handle when EOD.
	*/
	case IIAPI_EV_RELEASE_RCVD :
	    switch( state )
	    {
	    case SQL_TS_IDLE :
	    case SQL_TS_CONN :
	    {
		IIAPI_MSG_BUFF *msgBuff = (IIAPI_MSG_BUFF *)parmBlock;
		smo = smo_buff;

		if ( msgBuff->flags & IIAPI_MSG_EOD )
		{
		    STRUCT_ASSIGN_MACRO(sql_act_seq[ SQL_TAS_6 ], *smo);
		    smo->smo_next_state = SQL_TS_IDLE;
		}
		else
		{
		    smo->smo_action_cnt = 0;
		    smo->smo_next_state = state;
		}
		break;
	    }
	    default :
		/*
		** Ignored in all other states.
		*/
		smo = smo_buff;
		smo->smo_next_state = state;
		smo->smo_action_cnt = 0;
		break;
	    }
	    break;

	/*
	** The following events are ignored at the
	** transaction level.  They will be processed
	** by the connection state machine.  Since
	** they are unconditionally ignored, it is
	** easier to handle them here than maintaining
	** static transitions for every state.
	*/
    	case IIAPI_EV_ERROR_RCVD :
    	case IIAPI_EV_EVENT_RCVD :
	case IIAPI_EV_NPINTERUPT_RCVD :
    	case IIAPI_EV_TRACE_RCVD :
    	case IIAPI_EV_PROMPT_RCVD :
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
		  sql_tran_sm.sm_id );

	    STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_19 ], *smo );
	    smo->smo_next_state = state;
	}
        else  if ( event <= IIAPI_EVENT_MSG_MAX )
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: invalid message received\n",
                  sql_tran_sm.sm_id );

            STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_20 ], *smo );
            smo->smo_next_state = state;
        }
        else
        {
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s Evaluate: unexpected I/O completion\n",
                  sql_tran_sm.sm_id );

            STRUCT_ASSIGN_MACRO( sql_act_seq[ SQL_TAS_0 ], *smo );
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
              sql_tran_sm.sm_id );
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
**	 2-Oct-96 (gordy)
**	    Created.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	 3-Sep-98 (gordy)
**	    Call IIapi_abortStmtHndl() for transaction aborts.
**	19-Jan-99 (gordy)
**	    Added ERAB.  Issue transaction abort for invalid
**	    functions calls in ABORT state.
**       9-Aug-99 (rajus01)
**          Asynchronous behaviour of API requires the removal of 
**          handles marked for deletion from the queue. ( Bug #98303 )
**	21-Mar-01 (gordy)
**	    Disassociate transaction from distributed transaction
**	    name prior to making callback.
**	11-Dec-02 (gordy)
**	    Tracing added for local error conditions.
**	11-Oct-05 (gordy)
**	    Added action to halt state machine execution.
**	10-Jul-06 (gordy)
**	    Added XA actions.
**	28-Jul-06 (gordy)
**	    Read response message.
**	25-Mar-10 (gordy)
**	    Pass GCA message type to IIapi_createGCAEmpty().
**	    Enhanced parameter memory block handling.
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
    IIAPI_MSG_BUFF      *msgBuff;
    IIAPI_STATUS        status;
    II_BOOL             success = TRUE;
    char		queryText[ 64 ];

    switch( action )
    {
        case SQL_TA_REMC :
	    /*
	    ** Remember callback.
	    */
	    tranHndl->th_callback = TRUE;
	    tranHndl->th_parm = (IIAPI_GENPARM *)parmBlock;
	    break;

	case SQL_TA_RECV :
	{
	    /*
	    ** Issue receive message request.
	    */
	    IIAPI_MSG_BUFF *msgBuff = IIapi_allocMsgBuffer( sm_hndl );

	    if ( ! msgBuff )
		status = IIAPI_ST_OUT_OF_MEMORY;
	    else
		status = IIapi_rcvNormalGCA( sm_hndl, msgBuff, (II_LONG)(-1) );

	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_TA_SCOM :
	    /*
	    ** Format and send GCA_COMMIT message.
	    */
	    if ( ! (msgBuff = IIapi_allocMsgBuffer( sm_hndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    msgBuff->msgType = GCA_COMMIT;
	    msgBuff->flags = IIAPI_MSG_EOD;
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_TA_SRB :
	    /*
	    ** Format and send GCA_ROLLBACK message.
	    */
	    if ( ! (msgBuff = IIapi_allocMsgBuffer( sm_hndl )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    msgBuff->msgType = GCA_ROLLBACK;
	    msgBuff->flags = IIAPI_MSG_EOD;
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_TA_SSP :
	{
	    /*
	    ** Create savepoint handle.  Format and
	    ** send GCA_QUERY message 'savepoint <sp>'.
	    */
	    IIAPI_SAVEPTHNDL	*savePtHndl;
	    IIAPI_SAVEPTPARM    *savePtParm = (IIAPI_SAVEPTPARM *)parmBlock;

	    if ( ! IIapi_createSavePtHndl( savePtParm ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    savePtHndl = savePtParm->sp_savePointHandle;
	    STprintf( queryText, "savepoint %s\n", savePtHndl->sp_savePtName );

	    if ( ! (msgBuff = IIapi_createMsgQuery( sm_hndl, queryText )) )
	    {
		IIapi_deleteSavePtHndl( savePtHndl );
		savePtParm->sp_savePointHandle = NULL;
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );

	    if ( status != IIAPI_ST_SUCCESS )
	    {
		IIapi_deleteSavePtHndl( savePtHndl );
		savePtParm->sp_savePointHandle = NULL;
		success = FALSE;
	    }
	    break;
	}
	case SQL_TA_SRBS :
	{
	    /*
	    ** Format and send GCA_QUERY message 'rollback to <savepoint>'.
	    */
	    IIAPI_ROLLBACKPARM	*rollParm = (IIAPI_ROLLBACKPARM *)parmBlock;
	    IIAPI_SAVEPTHNDL	*savePtHndl = rollParm->rb_savePointHandle;

	    STprintf(queryText, "rollback to %s\n", savePtHndl->sp_savePtName);
    
	    if ( ! (msgBuff = IIapi_createMsgQuery( sm_hndl, queryText )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_TA_SAON :
	    /*
	    ** Format and send GCA_QUERY message 'set autocommit on'.
	    */
	    if ( ! (msgBuff = IIapi_createMsgQuery( sm_hndl, 
						     "set autocommit on" )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_TA_SAOF :
	    /*
	    ** Format and send GCA_QUERY message 'set autocommit off'.
	    */
	    if ( ! (msgBuff = IIapi_createMsgQuery( sm_hndl,
						     "set autocommit off" )) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_TA_SSEC :
	    /*
	    ** Format and send GCA_SECURE message.
	    */
	    if ( ! ( msgBuff = IIapi_createMsgSecure( tranHndl ) ) )
	    {
		status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
    
	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;

	case SQL_TA_SXAS :
	{
	    /*
	    ** Send XA Start message.
	    */
	    IIAPI_XASTARTPARM *startParm = (IIAPI_XASTARTPARM *)parmBlock;

	    if ( ! (msgBuff = IIapi_createMsgXA( sm_hndl, GCA_XA_START,
	    				&startParm->xs_tranID.ti_value.xaXID,
					startParm->xs_flags )) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_TA_SXAE :
	{
	    /*
	    ** Send XA End message.
	    */
	    IIAPI_XAENDPARM *endParm = (IIAPI_XAENDPARM *)parmBlock;

	    if ( ! (msgBuff = IIapi_createMsgXA( sm_hndl, GCA_XA_END,
	    				&endParm->xe_tranID.ti_value.xaXID,
					endParm->xe_flags )) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }

	    status = IIapi_sndGCA( sm_hndl, msgBuff, NULL );
	    if ( status != IIAPI_ST_SUCCESS )  success = FALSE;
	    break;
	}
	case SQL_TA_PEND :
	    /*
	    ** Return transaction aborted to all pending operations.
	    */
	    {
		IIAPI_STMTHNDL	*stmtHndl;

		for( 
		     stmtHndl = (IIAPI_STMTHNDL *)tranHndl->
						th_stmtHndlList.q_next;
		     stmtHndl != (IIAPI_STMTHNDL *)&tranHndl->th_stmtHndlList;
		     stmtHndl = (IIAPI_STMTHNDL *)stmtHndl->
						sh_header.hd_id.hi_queue.q_next
		   )
		    IIapi_abortStmtHndl( stmtHndl, 
					 E_AP0002_TRANSACTION_ABORTED,
					 II_SS40001_SERIALIZATION_FAIL, 
					 IIAPI_ST_FAILURE );
	    }
	    break;

	case SQL_TA_DELH :
	    /*
	    ** Mark handle for deletion.
	    */
	    QUremove( (QUEUE *)tranHndl );
	    sm_hndl->hd_delete = TRUE;

	    /*
	    ** If this was a distributed transaction, 
	    ** the associated distributed transaction 
	    ** name should be releasable during the 
	    ** callback.  Since the transaction handle 
	    ** has not actually been deleted, it is 
	    ** still associated with the distributed 
	    ** transaction name.  Normally, cases such
	    ** as this are serialized by the dispatch
	    ** operations queue, but IIapi_releaseXID()
	    ** is not a dispatched request.  So we need
	    ** to drop the association prior to making
	    ** the callback.
	    */
	    if ( tranHndl->th_tranName )  
	    {
		QUremove( &tranHndl->th_tranNameQue );
		tranHndl->th_tranName = NULL;
	    }
	    break;

	case SQL_TA_DELX :
	    /*
	    ** Free associated transaction name handle.
	    */
	    if ( tranHndl->th_tranName )  	/* Should never be NULL */
	    {
		QUremove( &tranHndl->th_tranNameQue );
		IIapi_deleteTranName( tranHndl->th_tranName );
		tranHndl->th_tranName = NULL;
	    }

	    /*
	    ** Mark handle for deletion.
	    */
	    QUremove( (QUEUE *)tranHndl );
	    sm_hndl->hd_delete = TRUE;
	    break;

	case SQL_TA_ERAB :
	    /*
	    ** Transaction Aborted.
	    */
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s: Transaction aborted\n", sql_tran_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP0002_TRANSACTION_ABORTED, 
				     II_SS40001_SERIALIZATION_FAIL,
				     IIAPI_ST_FAILURE ) )
	    {
	        status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_TA_ERCF :
	    /*
	    ** Commit failed.
	    */
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s: transaction commit failed\n", sql_tran_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP000B_COMMIT_FAILED, 
				     II_SS40001_SERIALIZATION_FAIL,
				     IIAPI_ST_FAILURE ) )
	    {
	        status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_TA_ERPC :
	    /*
	    ** Prepare-to-commit failed.
	    */
            IIAPI_TRACE( IIAPI_TR_ERROR )
                ( "%s: transaction prepare failed\n", sql_tran_sm.sm_id );

	    if ( ! IIapi_localError( sm_hndl, E_AP000C_2PC_REFUSED, 
				     II_SS40001_SERIALIZATION_FAIL,
				     IIAPI_ST_FAILURE ) )
	    {
	        status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
	    }
	    break;

	case SQL_TA_RRSP :
	{
	    /*
	    ** Read response message.
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
	    ** Check for XA error
	    */
	    if ( 
	         respData.gca_rqstatus & GCA_XA_ERROR_MASK  &&
	         respData.gca_errd5  &&
	         ! IIapi_xaError( sm_hndl, respData.gca_errd5 ) 
	       )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
	    break;
	}
	case SQL_TA_RXAR :
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

	    if ( ! IIapi_xaError( sm_hndl, respData.gca_errd5 ) )
	    {
	    	status = IIAPI_ST_OUT_OF_MEMORY;
		success = FALSE;
		break;
	    }
	    break;
	}
	case SQL_TA_CBOK :
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

	case SQL_TA_CBFL :
	    /*
	    ** Callback with failure.
	    */
	    if ( tranHndl->th_callback )
	    {
		IIapi_appCallback(tranHndl->th_parm, sm_hndl, IIAPI_ST_FAILURE);
		tranHndl->th_callback = FALSE;
	    }
	    break;

	case SQL_TA_CBIF :
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
	    break;

	case SQL_TA_CBAB :
	    /*
	    ** Callback with transaction abort.
	    */
	    if ( tranHndl->th_callback )
	    {
		if ( ! IIapi_localError( sm_hndl, E_AP0002_TRANSACTION_ABORTED, 
					 II_SS40001_SERIALIZATION_FAIL,
					 IIAPI_ST_FAILURE ) )
		    status = IIAPI_ST_OUT_OF_MEMORY;
		else
		    status = IIAPI_ST_FAILURE;

		IIapi_appCallback( tranHndl->th_parm, sm_hndl, status );
		tranHndl->th_callback = FALSE;
	    }
	    break;

	case SQL_TA_CBABX :
	    /*
	    ** Callback with XA transaction abort.
	    */
	    if ( tranHndl->th_callback )
	    {
		if ( ! IIapi_xaError( sm_hndl, IIAPI_XA_RBROLLBACK ) )
		    status = IIAPI_ST_OUT_OF_MEMORY;
		else
		    status = IIAPI_ST_FAILURE;

		IIapi_appCallback( tranHndl->th_parm, sm_hndl, status );
		tranHndl->th_callback = FALSE;
	    }
	    break;

	case SQL_TA_RCVE :
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
	    break;

	case SQL_TA_HALT :
	    /*
	    ** Halt state machine execution.
	    */
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


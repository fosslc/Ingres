/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: api.h
**
** Description:
**	Common internal API data structures and definitions.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     added hd_memTag and hd_errorQue to IIAPI_HNDLHEADER.
**	20-Dec-94 (gordy)
**	    Cleaned up handle definitions.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	13-Jun-95 (gordy)
**	    Added IIAPI_MAX_SEGMENT_LEN.
**	20-Jun-95 (gordy)
**	    Added pm_formatted for support of multi-segment BLOBs.
**	17-Jan-96 (gordy)
**	    Added environment handles and global data structure.
**	24-May-96 (gordy)
**	    New tagged memory support.  Added api_memTags and
**	    api_memTagCount to API_STATIC.  Removed hd_memTag
**	    from IIAPI_HNDLHEADER.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	11-Nov-96 (gordy)
**	    Removed tagged memory data elements.  Tagged memory
**	    support now implemented entirely through GL/CL.
**	14-Nov-96 (gordy)
**	    Moved serialization elements to thread specific data structure
**	    and replaced with thread-local-storage object key.
**	07-Feb-97 (somsa01)
**	    Added structure members for II_DATE_FORMAT, money formats, and
**	    II_DECIMAL to _API_STATIC. (Bug #80254)
**	27-Feb-97 (gordy)
**	    Added Jasmine and Name Server connection support.
**	15-Aug-98 (rajus01)
**	    Added support for IIapi_abort() function.
**	02-Sep-98 (rajus01)
**	    Added security label, spoken language to IIAPI_STATIC.
**	 3-Sep-98 (gordy)
**	    Added value for abort state to SM so handle functions 
**	    can set the state machine dependent value.
**	06-Oct-98 (rajus01)
**	    Added IIAPI_EV_PROMPT_RCVD.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Jun-03 (gordy)
**	    Added API MIB information.
**	02-Mar-2004 (gupsh01)
**	    Added new parameter colinit to indicate if
**	    the collation table is already initiated.
**	15-Mar-04 (gordy)
**	    Removed Jasmine support.
**	31-May-05 (gordy)
**	    Add IIAPI_STALE_HANDLE to check for stale handles.
**	 7-Jul-06 (gordy)
**	    Added XA function events.
**	15-Mar-07 (gordy)
**	    Added support for scrollable cursors.
**	 8-Aug-7 (gordy)
**	    Support date type alias.
**	25-Mar-10 (gordy)
**	    Added support for batch query processing.
*/

# ifndef __API_H__
# define __API_H__

# include <me.h>
# include <mu.h>
# include <qu.h>

# include <gca.h>

/*
** Common data structures.  Forward references.
*/

typedef struct _IIAPI_STATIC	IIAPI_STATIC;
typedef struct _IIAPI_THREAD	IIAPI_THREAD;
typedef struct _IIAPI_HNDLID	IIAPI_HNDLID;
typedef struct _IIAPI_HNDL	IIAPI_HNDL;
typedef struct _IIAPI_SM	IIAPI_SM;
typedef struct _IIAPI_SMI	IIAPI_SMI;
typedef struct _IIAPI_SM_OUT	IIAPI_SM_OUT;


/*
** Common datatypes used in state machines.
*/

typedef II_UINT2 IIAPI_STATE;
typedef II_UINT2 IIAPI_EVENT;
typedef II_UINT2 IIAPI_ACTION;


/*
** Name: IIAPI_SM_EVAL
**
** Description:
**	Prototype for the state machine evaluator function.
**
**	Evaluates input (state, event) and returns output (next 
**	state, actions).  A NULL return value indicates the event 
**	is to be ignored.
**
**	An output buffer is provided so the evaluator can build the
**	output dynamically (return value identical to parameter).
**	The output buffer may be ignored and a static buffer from
**	the state machine returned instead.
**
** Input:
**	state		Current state
**	event		API event
**	ev_hndl		Handle associated with event.
**	sm_hndl		State machine instance handle, derived from ev_hndl.
**	parmBlock	Paramter block associated with event.
**
** Output:
**	smo		Buffer for state machine output, may be ignored.
**
** Returns
**	IIAPI_SM_OUT *	State machine output, may be NULL.
*/

typedef IIAPI_SM_OUT *	(*IIAPI_SM_EVAL)( IIAPI_EVENT event,
					  IIAPI_STATE state, 
					  IIAPI_HNDL *ev_hndl, 
					  IIAPI_HNDL *sm_hndl, 
					  II_PTR parmBlock, 
					  IIAPI_SM_OUT *smo );

/*
** Name: IIAPI_SM_ACT
**
** Description:
**	Prototype for the state machine action execution function.
**
**	Executes an action returned by the state machine evaluator
**	function.  A FALSE return value indicates that any remaining
**	output actions should be skipped and the event should not be
**	dispatched to any other state machines.
**
** Input:
**	action		Action ID.
**	ev_hndl		Handle associated with event.
**	sm_hndl		State machine instance handle, derived from ev_hndl.
**	parmBlock	Paramter block associated with event.
**
** Output:
**	None.
**
** Returns
**	II_BOOL		TRUE if successful, FALSE if event should be cancelled.
*/

typedef II_BOOL		(*IIAPI_SM_ACT)( IIAPI_ACTION action,
					 IIAPI_HNDL *ev_hndl, 
					 IIAPI_HNDL *sm_hndl, 
					 II_PTR parmBlock );



/*
** IIAPI_STATE values.
**
** Only the common state value is defined here.
** All other values are defined in individual
** state machine declarations.
*/

# define IIAPI_IDLE   0

/* 
** IIAPI_EVENT values.
**
** The upper level dispatcher is driven by API
** function invocations while the lower level
** dispatcher is driven by GCA callbacks.  Event
** values are grouped accordingly.  GCA message
** types are significant in API processing of
** GCA_RECEIVE callbacks, so events are defined
** for the messages of interest.
**
** The following values are used as indexes into
** various tables throughout the API.  Care must
** be taken when adding new entries to ensure
** there are no duplicates or gaps and all tables
** are kept in sync with these definitions.
*/

/*
** API Functions
*/

# define IIAPI_EV_AUTO_FUNC		((IIAPI_EVENT)  0)
# define IIAPI_EV_BATCH_FUNC		((IIAPI_EVENT)  1)
# define IIAPI_EV_CANCEL_FUNC		((IIAPI_EVENT)  2)
# define IIAPI_EV_CATCHEVENT_FUNC	((IIAPI_EVENT)  3)
# define IIAPI_EV_CLOSE_FUNC		((IIAPI_EVENT)  4)
# define IIAPI_EV_COMMIT_FUNC		((IIAPI_EVENT)  5)
# define IIAPI_EV_CONNECT_FUNC		((IIAPI_EVENT)  6)
# define IIAPI_EV_DISCONN_FUNC		((IIAPI_EVENT)  7)
# define IIAPI_EV_GETCOLUMN_FUNC	((IIAPI_EVENT)  8)
# define IIAPI_EV_GETCOPYMAP_FUNC	((IIAPI_EVENT)  9)
# define IIAPI_EV_GETDESCR_FUNC		((IIAPI_EVENT) 10)
# define IIAPI_EV_GETEVENT_FUNC		((IIAPI_EVENT) 11)
# define IIAPI_EV_GETQINFO_FUNC		((IIAPI_EVENT) 12)
# define IIAPI_EV_MODCONN_FUNC		((IIAPI_EVENT) 13)
# define IIAPI_EV_POSITION_FUNC		((IIAPI_EVENT) 14)
# define IIAPI_EV_PRECOMMIT_FUNC	((IIAPI_EVENT) 15)
# define IIAPI_EV_PUTCOLUMN_FUNC	((IIAPI_EVENT) 16)
# define IIAPI_EV_PUTPARM_FUNC		((IIAPI_EVENT) 17)
# define IIAPI_EV_QUERY_FUNC		((IIAPI_EVENT) 18)
# define IIAPI_EV_ROLLBACK_FUNC		((IIAPI_EVENT) 19)
# define IIAPI_EV_SAVEPOINT_FUNC	((IIAPI_EVENT) 20)
# define IIAPI_EV_SCROLL_FUNC		((IIAPI_EVENT) 21)
# define IIAPI_EV_SETCONNPARM_FUNC	((IIAPI_EVENT) 22)
# define IIAPI_EV_SETDESCR_FUNC		((IIAPI_EVENT) 23)
# define IIAPI_EV_ABORT_FUNC		((IIAPI_EVENT) 24)
# define IIAPI_EV_XASTART_FUNC		((IIAPI_EVENT) 25)
# define IIAPI_EV_XAEND_FUNC		((IIAPI_EVENT) 26)
# define IIAPI_EV_XAPREP_FUNC		((IIAPI_EVENT) 27)
# define IIAPI_EV_XACOMMIT_FUNC		((IIAPI_EVENT) 28)
# define IIAPI_EV_XAROLL_FUNC		((IIAPI_EVENT) 29)

# define IIAPI_EVENT_FUNC_MAX		IIAPI_EV_XAROLL_FUNC

/*
** GCA Messages
*/

# define IIAPI_EV_ACCEPT_RCVD		((IIAPI_EVENT) 30)
# define IIAPI_EV_CFROM_RCVD		((IIAPI_EVENT) 31)
# define IIAPI_EV_CINTO_RCVD		((IIAPI_EVENT) 32)
# define IIAPI_EV_DONE_RCVD		((IIAPI_EVENT) 33)
# define IIAPI_EV_ERROR_RCVD		((IIAPI_EVENT) 34)
# define IIAPI_EV_EVENT_RCVD		((IIAPI_EVENT) 35)
# define IIAPI_EV_IACK_RCVD		((IIAPI_EVENT) 36)
# define IIAPI_EV_NPINTERUPT_RCVD	((IIAPI_EVENT) 37)
# define IIAPI_EV_PROMPT_RCVD           ((IIAPI_EVENT) 38)
# define IIAPI_EV_QCID_RCVD		((IIAPI_EVENT) 39)
# define IIAPI_EV_REFUSE_RCVD		((IIAPI_EVENT) 40)
# define IIAPI_EV_REJECT_RCVD		((IIAPI_EVENT) 41)
# define IIAPI_EV_RELEASE_RCVD		((IIAPI_EVENT) 42)
# define IIAPI_EV_RESPONSE_RCVD		((IIAPI_EVENT) 43)
# define IIAPI_EV_RETPROC_RCVD		((IIAPI_EVENT) 44)
# define IIAPI_EV_TDESCR_RCVD		((IIAPI_EVENT) 45)
# define IIAPI_EV_TRACE_RCVD		((IIAPI_EVENT) 46)
# define IIAPI_EV_TUPLE_RCVD		((IIAPI_EVENT) 47)
# define IIAPI_EV_GCN_RESULT_RCVD	((IIAPI_EVENT) 48)
# define IIAPI_EV_UNEXPECTED_RCVD       ((IIAPI_EVENT) 49)

# define IIAPI_EVENT_MSG_MAX		IIAPI_EV_UNEXPECTED_RCVD

/*
** GCA operation results
*/

# define IIAPI_EV_RESUME		((IIAPI_EVENT) 50)
# define IIAPI_EV_CONNECT_CMPL		((IIAPI_EVENT) 51)
# define IIAPI_EV_DISCONN_CMPL		((IIAPI_EVENT) 52)
# define IIAPI_EV_SEND_CMPL		((IIAPI_EVENT) 53)
# define IIAPI_EV_SEND_ERROR		((IIAPI_EVENT) 54)
# define IIAPI_EV_RECV_ERROR		((IIAPI_EVENT) 55)
# define IIAPI_EV_DONE			((IIAPI_EVENT) 56)

# define IIAPI_EVENT_MAX		IIAPI_EV_DONE
# define IIAPI_EVENT_CNT		(IIAPI_EV_DONE + 1)


/*
** Name: IIAPI_STATIC
**
** Description:
**	Structure defining the global data for API.
**
**	api_semaphore		Semaphore protecting this structure.
**
**	api_thread		Thread-local-storage object key.
**
**	api_env_q		Queue for environment handles.
**
**	api_env_default		Default environment for IIAPI_VERSION_1.
**
**	api_adf_cb		ADF control block.
**
**	api_date_alias		Date type alias.
**
**	api_dfmt		Default date format.
**
**	api_mfmt		Default money format.
**
**	api_decimal		Default decimal character.
**
**	api_tz_cb		Default timezone control block.
**
**	api_timezone		Default timezone name.
**
**	api_year_cutoff		Default century boundary year cutoff.
**
**	api_trace_level		Output trace level, 0 = no tracing.
**
**	api_trace_file		Name of the output trace file.
**
** History:
**	19-Jan-96 (gordy)
**	    Created.
**	24-May-96 (gordy)
**	    Added api_memTags, api_memTagCount for tagged memory support.
**	11-Nov-96 (gordy)
**	    Removed tagged memory data elements.  Tagged memory
**	    support now implemented entirely through GL/CL.
**	14-Nov-96 (gordy)
**	    Moved serialization elements to thread specific data structure
**	    and replaced with thread-local-storage object key.
**	07-Feb-97 (somsa01)
**	    Added structure members for II_DATE_FORMAT, money formats, and
**	    II_DECIMAL to _API_STATIC. (Bug #80254)
**      02-Mar-2004 (gupsh01)
**          Added new parameter colinit to indicate if
**          the collation table is already initiated.
**	 8-Aug-7 (gordy)
**	    Added date type alias.
*/

struct _IIAPI_STATIC
{

    MU_SEMAPHORE	api_semaphore;			/* Multi-threading */
    METLS_KEY		api_thread;

    QUEUE		api_env_q;			/* Environments */
    PTR			api_env_default;

    PTR			api_adf_cb;			/* ADF parameters */
    i2			api_date_alias;
    DB_DATE_FMT		api_dfmt;
    DB_MONY_FMT		api_mfmt;
    DB_DECIMAL		api_decimal;
    PTR			api_tz_cb;
    char 		*api_timezone;
    i4			api_year_cutoff;
    i4			api_slang;			/* Spoken Lang */

    II_LONG		api_trace_level;		/* Tracing */
    char		*api_trace_file;
    II_BOOL		api_unicol_init;		/* Unicode collation 
						        ** is initialized. */
    PTR			api_ucode_ctbl;
    PTR			api_ucode_cvtbl;
};


/*
** Name: IIAPI_THREAD
**
** Description:
**	Structure defining the thread specific data for API.
**
**	api_gca_active		Number of active GCA requests.
**
**	api_dispatch		Flag indicating if dispatcher is active.
**
**	api_op_q		Operations queue for serializing events.
**
**	14-Nov-96 (gordy)
**	    Created.
*/

struct _IIAPI_THREAD
{

    u_i4		api_gca_active;
    II_BOOL		api_dispatch;
    QUEUE		api_op_q;

};


/*
** Name: IIAPI_SM
**
** Description:
**	Structure defining the interface to a state machine.  ID 
**	information is provided for tracing purposes.  State and 
**	action counts permit validation.  The actual interface 
**	is provided through the evaluation and action functions.  
**	The evaluation function determines the next state and 
**	output actions to perform given the current state and 
**	input event.  The output actions are executed using the 
**	action function.
**
**	sm_id			ID of state machine.
**
**	sm_state_cnt		Number of states.
**
**	sm_state_id		Array of state IDs.
**
**	sm_action_cnt		Number of actions.
**
**	sm_action_id		Array of action IDs.
**
**	sm_evaluate		Event evaluation function.
**
**	sm_action		Action execution function.
**
**	sm_abort_state		State to be assigned when aborting.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
**	 3-Sep-98 (gordy)
**	    Added value for abort state so handle functions 
**	    can set the state machine dependent value.
*/

struct _IIAPI_SM
{

    char		*sm_id;
    i4			sm_state_cnt;
    char		**sm_state_id;
    i4			sm_action_cnt;
    char		**sm_action_id;
    IIAPI_SM_EVAL	sm_evaluate;
    IIAPI_SM_ACT	sm_action;
    IIAPI_STATE		sm_abort_state;

};


/*
** Name: IIAPI_SMI
**
** Description:
**	State machine instance defining the state machine which
**	manages and object and the current state of the object.
**
**	smi_sm			State machine.
**	smi_state		Current state.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

struct _IIAPI_SMI
{

    IIAPI_SM		*smi_sm;		/* State machine */
    IIAPI_STATE		smi_state;		/* Current state */

};


/*
** Name: IIAPI_SM_OUT
**
** Description:
**	Defines the state machine output in relation to the
**	current state and input event.
**
**	smo_next_state		Next state.
**
**	smo_action_cnt		Number of actions.
**
**	smo_actions		Array of action IDs.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

# define IIAPI_SM_ACTION_MAX	3

struct _IIAPI_SM_OUT
{

    IIAPI_STATE		smo_next_state;
    i4			smo_action_cnt;
    IIAPI_ACTION	smo_actions[ IIAPI_SM_ACTION_MAX ];

};


/*
** Name: IIAPI_HNDLID
**
** Description:
**	Common header to all API handles.
**
**	hi_queue	Queue for handles of same type.  Must be first
**			item in every handle.
**
**	hi_hndlID	Identifies the handle type.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      20-Dec-94 (gordy)
**          Made into structure to support all handles.
**      25-Apr-95 (gordy)
**          Cleaned up Database Events.
**      17-Jan-96 (gordy)
**          Added environment handles.
*/

struct _IIAPI_HNDLID
{

    QUEUE	hi_queue;
    II_ULONG	hi_hndlID;

# define IIAPI_HI_ENV_HNDL	0x0101
# define IIAPI_HI_CONN_HNDL	0x0202
# define IIAPI_HI_TRAN_HNDL	0x0303
# define IIAPI_HI_STMT_HNDL	0x0404
# define IIAPI_HI_DBEV_HNDL	0x0505
# define IIAPI_HI_DBEV		0x0606
# define IIAPI_HI_IDENTIFIER	0x0707
# define IIAPI_HI_SAVEPOINT	0x0808
# define IIAPI_HI_TRANNAME	0x0909
# define IIAPI_HI_ERROR		0x0A0A

};


/*
** Name: IIAPI_HNDL
**
** Description:
**	Common header for handles managed by state machines.
**
**      hd_id		Common handle information.
**
**      hd_smi		State machine instance.  Defines state machine
**			which manages handle.
**
**      hd_delete	Indicator that handle can be deleted.
**
**	hd_errorQue	Next error handle in hd_errorList which has
**			not yet been retrieved by the application.
**			If this points at hd_errorList then either
**			there are no error handles or all handles
**			have been retrieved by the application.
**
**      hd_errorList	Queue for associated error handles.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**       31-may-94 (ctham)
**           added hd_memTag and hd_errorQue.
**      20-Dec-94 (gordy)
**          Consolidated hd_queue and hd_hndlID for use in all handles.
**      24-May-96 (gordy)
**          Removed hd_memTag, not needed with new tagged memory support.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      28-jun-2002 (loera01) Bug 108147
**          Added hd_sem to IIAPI_HNDL for mutex protection.
**	31-May-05 (gordy)
**	    Add macro to check for stale handles (flagged for deletion).
*/

struct _IIAPI_HNDL
{
    
    IIAPI_HNDLID	hd_id;
    IIAPI_SMI		hd_smi;
    MU_SEMAPHORE        hd_sem;
    II_BOOL		hd_delete;
    QUEUE		*hd_errorQue;
    QUEUE		hd_errorList;
    
};

#define	IIAPI_STALE_HANDLE( handle )	(((IIAPI_HNDL *)handle)->hd_delete)


/*
** Global API data.
*/

GLOBALREF IIAPI_STATIC	*IIapi_static;


/*
** API State machine interfaces.
*/

# define IIAPI_SMT_SQL		((II_UINT2) 0)	/* SQL, IIAPI_CT_SQL */
# define IIAPI_SMT_NS		((II_UINT2) 2)	/* Name Server, IIAPI_CT_NS */

# define IIAPI_SMT_MAX		IIAPI_SMT_NS
# define IIAPI_SMT_CNT		(IIAPI_SMT_MAX + 1)

# define IIAPI_SMH_CONN		((II_UINT2) 0)
# define IIAPI_SMH_TRAN		((II_UINT2) 1)
# define IIAPI_SMH_STMT		((II_UINT2) 2)
# define IIAPI_SMH_DBEV		((II_UINT2) 3)

# define IIAPI_SMH_MAX		IIAPI_SMH_DBEV
# define IIAPI_SMH_CNT		(IIAPI_SMH_MAX + 1)

GLOBALREF IIAPI_SM	*IIapi_sm[ IIAPI_SMT_CNT ][ IIAPI_SMH_CNT ];

/*
** Function references
*/

II_EXTERN II_VOID		IIapi_init_mib( VOID );

# endif /* __API_H__ */

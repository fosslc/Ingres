/*
** Copyright (c) 2003, 2004 Computer Associates Intl. Inc. All Rights Reserved.
**
** This is an unpublished work containing confidential and proprietary
** information of Computer Associates.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Computer Associates is prohibited.
*/

/* 
** Name: gcadm.h
**
** Description:
**	GCF Administrator Interface.
**
**	The GCF administrator module provides the communications
**	logic for support of administrative (iimonitor) sessions.
**	Using this module, a GCF based server need only deal with
**	request parsing, execution, and result production.
**
**	A server passes control of a GCA association to the admin
**	module using gcadm_session().  Once passed to the admin
**	module, the server's only interaction with the association
**	is via administrator callbacks and request result functions.
**
**	When a request is received on an association, it is passed
**	to the server via a request callback function as a text
**	string.  The administrator provides interface functions 
**	for returning text results and errors to the client.  An
**	interface function is also provided to indicate completion
**	(success or error) of the request.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

#ifndef _GCADM_INCLUDED_
#define _GCADM_INCLUDED_


#include <qu.h>
#define ARR_SIZE( arr )     (sizeof(arr)/sizeof((arr)[0]))
 
/*
** Administrator Error Codes
*/

#define E_GC5000_NOT_ACTIVE		(E_GCF_MASK | 0x5000)
#define E_GC5001_NOT_IDLE		(E_GCF_MASK | 0x5001)
#define E_GC5002_NO_MEMORY		(E_GCF_MASK | 0x5002)
#define E_GC5003_FSM_STATE		(E_GCF_MASK | 0x5003)
#define E_GC5004_INVALID_MSG		(E_GCF_MASK | 0x5004)
#define E_GC5005_INTERNAL_ERROR		(E_GCF_MASK | 0x5005)
#define E_GC5007_INVALID_MD_PARMS 	(E_GCF_MASK | 0x5007)
#define E_GC5008_INVALID_MSG_STATE	(E_GCF_MASK | 0x5008)
#define E_GC5009_DUP_RQST		(E_GCF_MASK | 0x5009)


/*
** Name: GCADM_ALLOC_FUNC
**
** Description:
**	Function prototype which allocates a block of memory of a 
**	requested size.  Returns NULL if memory could not be allocated.
**
** Input:
**	size 	Size of memory to allocate.
**
** Output:
**	None.
**
** Returns:
**	PTR	Allocated memory, or NULL.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef PTR (*GCADM_ALLOC_FUNC)( u_i4 size );


/*
** Name: GCADM_DEALLOC_FUNC
**
** Description:
**	Function prototype which frees a block of memory 
**	allocated by GCADM_ALLOC_FUNC.
**
** Input:
**	buffer	Memory block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef VOID (*GCADM_DEALLOC_FUNC)( PTR buffer );


/*
** Name: GCADM_ERRLOG_FUNC
**
** Description:
**	Function prototype which logs an error to server's log.
**
** Input:
**	errcode		Error code to be logged.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef void (*GCADM_ERRLOG_FUNC)( STATUS errcode );


/*
** Name: gcadm_init
**
** Description:
**	Initialize the administrator module.  Optional parameters
**	allow the caller to configure GCA message buffers, memory
**	management and error logging.  Should only be called once 
**	(unless there is an intervening call to gcadm_term()).
**
**	Error logging is only performed for errors which are not
**	returned by the administrator.  Any error returned by 
**	administrator functions or in callbacks has not been 
**	logged.  FAIL will be returned when an error has been
**	logged but the error condition must also be exposed to
**	the caller.
**
**	The minimum GCA buffer size will be 1024 bytes.  MEreqmem()
**	and MEfree() will be used if no memory management functions
**	are provided.  No error logging will be done if an error
**	logging function is not provided (errors normally logged
**	will be lost).
**
**	Use gcadm_term() to shutdown the admin module.
**
** Input:
**	GCADM_INIT_PARMS	Initialization parameters.
**	    msg_buff_len 	Message buffer size.
**	    alloc_rtn		Memory alloc function, may be NULL.
**	    dealloc_rtn 	Memory free function, may be NULL.
**	    errlog_rtn		Error logging function, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK, E_GC5001_NOT_IDLE.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    i4 			msg_buff_len;   /* Message buffer length */	
    GCADM_ALLOC_FUNC	alloc_rtn;	/* Memory alloc function */ 
    GCADM_DEALLOC_FUNC	dealloc_rtn;	/* Memory free function */ 
    GCADM_ERRLOG_FUNC	errlog_rtn;	/* Error logging function */

} GCADM_INIT_PARMS;


FUNC_EXTERN STATUS gcadm_init( GCADM_INIT_PARMS * );


/*
** Name: GCADM_RQST_FUNC
**
** Description:
**	Function prototype which handles individual requests 
**	received on an administrator session (request callback).
**
**	A session status of anything other than OK indicates
**	that the session has been terminated and no further
**	requests/callbacks may/will be made on the session.
**
**	A session status of OK indicates that a new client
**	request has been received.  The administrator and
**	client are waiting for request results.
**
**	Requests results may returned to the client using the
**	functions gcadm_rqst_rslt(), gcadm_rqst_rlstlst() and 
**	gcadm_rqst_error().  Completion of request processing 
**	must be indicated by a call to gcadm_rqst_done().
**
**	An administrator session control block is provided
**	which must be used to identify the session context
**	when calling other administrator functions for this
**	request.  The callback parameter specified along
**	with the callback function is also provided.
**
**	A session request is simply a text string as sent
**	by the client.  
**
** Input:
**	GCADM_RQST_PARMS	Request parameters.
**	    gcadm_scb		Session control block.
**	    rqst_cb_parms	Request callback parameter.
**	    sess_status		Session status.
**	    length		Length of request text.
**	    request		Request text.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    PTR			gcadm_scb;	/* Session control block */
    PTR			rqst_cb_parms;	/* Callback parameter */
    STATUS		sess_status;	/* Session status */
    i4			length;		/* Request text length */
    char		*request;	/* Request text */

} GCADM_RQST_PARMS;


typedef void (*GCADM_RQST_FUNC)( GCADM_RQST_PARMS * );


/*
** Name: gcadm_session
**
** Description:
**	Add a new session to the administrator.  Must be called
**	following the completion of GCA_RQRESP on the association,
**	and prior to any other GCA call.
**
**	If successful, the administrator assumes all GCA control 
**	of the session.  If an error is returned, caller retains 
**	GCA control of the session and must perform any error 
**	reporting and session clean-up.
**
**	Caller provides a callback routine to receive notification
**	of the next request made on the session, and a parameter 
**	block to be passed to the callback routine which may be 
**	used to identify the session context.
**
** Input:
**	GCADM_SESS_PARMS	Session parameters.
**	    assoc_id		GCA association ID.
**	    gca_cb		GCA control block.
**	    rqst_cb		Request callback function.
**	    rqst_cb_parms	Request callback parameter.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK,
**				E_GC5000_NOT_ACTIVE,
**				E_GC5002_NO_MEMORY.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    i4			assoc_id;	/* GCA association ID */
    PTR			gca_cb;		/* GCA control block */
    GCADM_RQST_FUNC	rqst_cb;	/* Request callback function */
    PTR			rqst_cb_parms;	/* Request callback parameter */

} GCADM_SESS_PARMS;


FUNC_EXTERN STATUS gcadm_session( GCADM_SESS_PARMS * );


/*
** Name: GCADM_CMPL_FUNC
**
** Description:
**	Function prototype which receives status of request
**	result processing (result callback).  
**
**	If call succeeded (call status is OK), additional 
**	request result calls may be made.
**
**	If call failed (call status is not OK), no subsequent
**	request result calls may be made for the session 
**	(session will be terminated).
**
**      An administrator session control block is provided
**      which must be used to identify the session context
**      when calling other administrator functions for the
**      request.  Also provides the callback parameter
**      specified when the request result call was made.
**
** Input:
**	GCADM_CMPL_PARMS	Result parameters.
**	    gcadm_scb		Session control block.
**	    rslt_cb_parms	Result callback parameter.
**	    rslt_status		Call status. 
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    PTR			gcadm_scb;	/* Session control block */
    PTR			rslt_cb_parms;	/* Callback parameter */
    STATUS		rslt_status;	/* Call status */

} GCADM_CMPL_PARMS;


typedef void (*GCADM_CMPL_FUNC)( GCADM_CMPL_PARMS * );


/*
** Name: gcadm_rqst_rslt
**
** Description:
**	Returns a single string of text to the client as a 
**	request result.
**
**	The session control block returned to the request
**	callback function or to a preceeding result callback
**	function identifies the session associated with the
**	request result.
**
**	Caller provides a callback routine for notification 
**	that the call has completed, and a parameter block 
**	to be passed to the callback routine which may be 
**	used to identify the session context.
**
** Input:
**	GCADM_RSLT_PARMS	Request result parameters.
**	    gcadm_scb		Session control block.
**	    rslt_len		Result text length.
**	    rslt_text		Result text.
**	    rslt_cb		Result callback function.
**	    rslt_cb_parms	Result callback parameter.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK, 
**				E_GC5000_NOT_ACTIVE,
**				E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    PTR			gcadm_scb;	/* Session control block */
    i4			rslt_len;	/* Result text length */
    char 		*rslt_text;	/* Result text */
    GCADM_CMPL_FUNC	rslt_cb;	/* Result callback function */
    PTR			rslt_cb_parms;	/* Result callback parameter */

} GCADM_RSLT_PARMS;


FUNC_EXTERN STATUS gcadm_rqst_rslt( GCADM_RSLT_PARMS * );


/*
** Name: gcadm_rqst_rsltlst
**
** Description:
**	Returns a series of result strings of text to the 
**	client as a request result.  The result list is a
**	QUEUE of GCADM_RSLT structures.  The result list
**	and it's contents are not changed.
**
**	The maximum string length must be provided and any 
**	result string which exceeds the maximum length will 
**	be truncated.
**
**	The session control block returned to the request
**	callback function or to a preceeding result callback
**	function identifies the session associated with the
**	request result.
**
**	Caller provides a callback routine for notification 
**	that the call has completed, and a parameter block 
**	to be passed to the callback routine which may be 
**	used to identify the session context.
**
** Input:
**	GCADM_RSLTLIST_PARMS	Request result list parameters.
**	    gcadm_scb		Session control block.
**	    rsltlst_max_len	Maximum text length.
**	    rsltlst_q		Result list (QUEUE of GCADM_RSLT).
**	    rslt_cb		Result callback function.
**	    rslt_cb_parms	Result callback parameter.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK, 
**				E_GC5000_NOT_ACTIVE,
**				E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    QUEUE		rslt_q;		/* Result list */
    i4			rslt_len;	/* Result text length */
    char		*rslt_text;	/* Result text */

} GCADM_RSLT;    


typedef struct
{

    PTR			gcadm_scb;		/* Session control block */
    i4			rsltlst_max_len;	/* Maximum text length */
    QUEUE		*rsltlst_q;		/* Result list */
    GCADM_CMPL_FUNC	rslt_cb;		/* Result callback function */
    PTR			rslt_cb_parms;		/* Result callback parameter */

} GCADM_RSLTLST_PARMS;


FUNC_EXTERN STATUS gcadm_rqst_rsltlst( GCADM_RSLTLST_PARMS * );


/*
** Name: gcadm_rqst_error
**
** Description:
**	Returns an error code, SQLSTATE, and (optional) error
**	message text to the client as a request result.
**
**	The SQLSTATE value SS50000_MISC_ERRORS will be used
**	by default if the SQLSTATE parameter is NULL.
**
**	The session control block returned to the request
**	callback function or to a preceeding result callback
**	function identifies the session associated with the
**	request result.
**
**	Caller provides a callback routine for notification 
**	that the call has completed, and a parameter block 
**	to be passed to the callback routine which may be 
**	used to identify the session context.
**
** Input:
**	GCADM_ERROR_PARMS	Request result error parameters.
**	    gcadm_scb		Session control block.
**	    error_code		Error code.
**	    error_sqlstate	SQLSTATE, may be NULL.
**	    error_len		Error text length.
**	    error_text		Error message text, may be NULL.
**	    rslt_cb		Result callback function.
**	    rslt_cb_parms	Result callback parameter.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK, 
**				E_GC5000_NOT_ACTIVE, 
**				E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    PTR			gcadm_scb;		/* Session control block */
    i4			error_code; 		/* Error code */
    char		*error_sqlstate;	/* SQLSTATE  */ 
    i4			error_len;		/* Length of error text */
    char		*error_text;		/* Error message text */
    GCADM_CMPL_FUNC	rslt_cb;		/* Result callback function */
    PTR			rslt_cb_parms;		/* Result callback parameter */

} GCADM_ERROR_PARMS;


FUNC_EXTERN STATUS gcadm_rqst_error( GCADM_ERROR_PARMS  * );


/*
** Name: gcadm_rqst_done
**
** Description:
**	Indicates that a client administrator request has 
**	completed.  Depending on the request status, the 
**	session may be terminated or the next request may
**	be retrieved.
**
**	A request status of OK will result in the client
**	being notified of the successful completion of
**	the request.  The administrator will then wait
**	for additional requests from the client.
**
**	A request status other than OK will result in the
**	termination of the administrator session.  Note
**	that errors should be reported to the client using
**	gcadm_rqst_error().
**
**	The session control block returned to the request
**	callback function or to a preceeding result callback
**	function identifies the session for which the
**	request has completed.
**
**	Caller provides a callback routine to receive notification
**	of the next request made on the session, and a parameter 
**	block to be passed to the callback routine which may be 
**	used to identify the session context.
**
** Input:
**	GCADM_DONE_PARMS	Request result done parameters.
**	    gcadm_scb		Session control block.
**	    rqst_status 	Request status.
**	    rslt_cb		Result callback function, may be NULL.
**	    rslt_cb_parms	Result callback parameter.
**
** Output:
**	None.
**
** Returns:
**	STATUS			OK, 
**				E_GC5000_NOT_ACTIVE,
**				E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

typedef struct
{

    PTR			gcadm_scb;	/* Session control block */
    STATUS		rqst_status;	/* Request status */
    GCADM_RQST_FUNC	rqst_cb;	/* Request callback function */
    PTR			rqst_cb_parms;	/* Request callback parameter */

} GCADM_DONE_PARMS;


FUNC_EXTERN STATUS gcadm_rqst_done( GCADM_DONE_PARMS * );


/*
** Name: gcadm_term
**
** Description:
**	Terminate and shutdown the administrator module.  Any 
**	active administrator sessions will be terminated.
**
**	A call to gcadm_init() must have preceeded this call.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, E_GC5000_NOT_ACTIVE.
**
** History:
**	 9-Sept-2003 (wansh01)
**	    Created.
*/

FUNC_EXTERN STATUS gcadm_term( void );


/*
** Name: gcadm_keyword
**
** Description:
**	Convert the input array of tokens to corresponding token ids
**	using the pre-defined keyword tables. 
**
**
** Input:
**      token_count     Numbers of tokens.
**      pToken          ptr to array of ptrs to char tokens.
**      keyword_count   Numbers of keywords.
**      pKeywords       pointer to array of defined keywords.
**      default_id      default token id
**
** Output:
**	token_ids       array of token_ids.
**
** Returns:
**	void
**
** History:
**	 18-Mar-04 (wansh01)
**	    Created.
*/

typedef u_i2	ADM_ID; 

typedef struct
{
    char	*token;
    ADM_ID	id;
} ADM_KEYWORD; 

FUNC_EXTERN void gcadm_keyword ( u_i2 ,char **, u_i2, ADM_KEYWORD *,
					ADM_ID *, ADM_ID );

/*
** Name: gcadm_command
**
** Description:
**	Convert the input array of tokens_ids to corresponding command
**	using the pre-defined commands table. 
**
**
** Input:
**	token_count	Numbers of tokens_ids.
**	token_ids	ptr to array of token_ids. 
**	command_count	Numbers of commands.
**	commands	pointer to array of defined commands. 
**
** Output:
**	rqst_cmd 	ptr to requested command struct.
**
** Returns:
**	bool  		request command found indicator 
**
** History:
**	 18-Mar-04 (wansh01)
**	    Created.
*/

typedef struct
{
    u_i2	command;
    u_i2	count;
    ADM_ID	*keywords;

} ADM_COMMAND;


FUNC_EXTERN bool gcadm_command ( u_i2 , ADM_ID *,  u_i2, ADM_COMMAND *,
			ADM_COMMAND **  rqst_cmd );

#endif /* _GCADM_INCLUDED_ */



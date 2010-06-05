/*
** Copyright (c) 2003, 2004 Computer Associates Intl. Inc. All Rights Reserved.
**
** This is an unpublished work containing confidential and proprietary
** information of Computer Associates.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Computer Associates is prohibited.
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <sp.h>
#include <mo.h>
#include <st.h>
#include <tr.h>
#include <cv.h>

#include <iicommon.h>
#include <gca.h>
#include <gc.h>
#include <gcaint.h>
#include <gcu.h>
#include <gcadm.h>
#include <gcadmint.h>

/*
** Name: gcadm.c 
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
**	to the server via a equest callback function as a text
**	string.  The administrator provides interface functions 
**	for returning text results and errors to the client.  An
**	interface function is also provided to indicate completion
**	(success or error) of the request.
**	an interface with GCF servers and iimonitor. 
**
** History:
**	 9-10-2003 (wansh01) 
**	    Created.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

static PTR  gcadm_alloc( u_i4 );
static VOID gcadm_free( PTR );



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
**	STATUS		OK, E_GC5001_NOT_IDLE.
**
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_init( GCADM_INIT_PARMS *gcadm_init )
{
    char    *env;

    /* already init ??  */
    if (GCADM_global.gcadm_state != GCADM_IDLE )  
	return( E_GC5001_NOT_IDLE );

    GCADM_global.gcadm_state  = GCADM_ACTIVE;

    /* init gcadm global varibales */
    if ( gcadm_init->alloc_rtn )
	GCADM_global.alloc_rtn = gcadm_init->alloc_rtn;
    else 
	GCADM_global.alloc_rtn = gcadm_alloc;
		
    if ( gcadm_init->dealloc_rtn )
	GCADM_global.dealloc_rtn = gcadm_init->dealloc_rtn; 
    else 
	GCADM_global.dealloc_rtn =  gcadm_free;

    GCADM_global.errlog_rtn = gcadm_init->errlog_rtn;

    GCADM_global.gcadm_buff_len = max( gcadm_init->msg_buff_len, 
					GCADM_MIN_BUFLEN );

    GCADM_global.id_tdesc = 0; 
    QUinit( &GCADM_global.scb_q );   
    QUinit( &GCADM_global.rcb_q );   

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.gcadm_trace_level"), &env );
    if ( env  &&  *env )  CVal( env, &GCADM_global.gcadm_trace_level );

    /* 
    ** Init gcadm state machine 
    */ 	
    gcadm_smt_init();

    return ( OK );
}


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
**	Caller provides a callback routine to handle individual 
**	requests made on the session, and a parameter block to 
**	be passed to the callback routine which may be used to 
**	identify the session context.  The callback parameter 
**	must remain valid for the duration of the session (until
**	session termination is indicated by GCADM_RQST_FUNC()).
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
**	STATUS		OK,
**			E_GC5000_NOT_ACTIVE,
**			E_GC5002_NO_MEMORY.
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_session( GCADM_SESS_PARMS *gcadm_sess )
{
    GCADM_SCB  *scb; 
	
    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM session: ADM not active\n", -1 );
       return( E_GC5000_NOT_ACTIVE );
    }

    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM session: entry \n", gcadm_sess->assoc_id);

    if ( ! (scb = gcadm_new_scb( gcadm_sess->assoc_id, gcadm_sess->gca_cb )) )
	return ( E_GC5002_NO_MEMORY );

    scb->rqst_cb = gcadm_sess->rqst_cb;
    scb->rqst_cb_parms = gcadm_sess->rqst_cb_parms;

    return ( gcadm_request( GCADM_RQST_NEW_SESSION, scb ));

}


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
**	used to identify the session context.  The callback 
**	parameter must remain valid for the duration of the 
**	call.
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
**	STATUS		OK, 
**			E_GC5000_NOT_ACTIVE,
**			E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_rqst_rslt( GCADM_RSLT_PARMS *rslt )
{
    GCADM_SCB  *scb = (GCADM_SCB *)rslt->gcadm_scb; 
	
    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM rqst_rslt: ADM not active\n", -1 );
       return( E_GC5000_NOT_ACTIVE );
    }

    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM rqst_rslt: entry \n", scb->aid );

    if ( GCADM_global.gcadm_trace_level >= 3 )
	TRdisplay( "%4d   GCADM rqst_rslt: rslt_len=%d, rslt_text=%s\n", 
			scb->aid, rslt->rslt_len, rslt->rslt_text );

    /*
    ** Is there anything to send?
    */
    if ( rslt->rslt_len == 0  ||  ! rslt->rslt_text )
    {
	GCADM_CMPL_PARMS cmpl_parms;
	cmpl_parms.gcadm_scb = rslt->gcadm_scb;
	cmpl_parms.rslt_cb_parms = rslt->rslt_cb_parms;
	cmpl_parms.rslt_status = OK;
	(*rslt->rslt_cb)( &cmpl_parms );
	return( OK );
    }

    scb->msg_type = GCA_TRACE;
    scb->rslt_cb = rslt->rslt_cb;
    scb->rslt_cb_parms = rslt->rslt_cb_parms; 

    /* 
    ** set and adjust scb->buf_len if rslt_len is too long  
    */
    scb->buf_len = min (rslt->rslt_len, GCADM_global.gcadm_buff_len );
    MEcopy( rslt->rslt_text, scb->buf_len, scb->buffer );

    return ( gcadm_request ( GCADM_RQST_RSLT, scb )); 
}


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
**	used to identify the session context.  The callback 
**	parameter must remain valid for the duration of the 
**	call.
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
**	STATUS		OK,
**			E_GC5000_NOT_ACTIVE,
**			E_GC5009_DUP_RQST.
**
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_rqst_rsltlst( GCADM_RSLTLST_PARMS *rsltlst )
{
    GCADM_SCB   *scb= (GCADM_SCB *)rsltlst->gcadm_scb;
    char	*td_data;
    i4		one = 1;
    i4		zero = 0; 
    i4		modifier = 0;
    i4		data_type = DB_VCH_TYPE;
    u_i4	id_tdesc;

    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM rqst_rsltlst: ADM not active\n", -1 );
        return( E_GC5000_NOT_ACTIVE );
    }

    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM rqst_rsltlst: entry \n", scb->aid );

    if ( GCADM_global.gcadm_trace_level >= 3 )
	TRdisplay( "%4d   GCADM rqst_rsltlst: rsltlst_max_len = %d\n", 
				scb->aid, rsltlst->rsltlst_max_len );

    /*
    ** Is there anything to send?
    */
    if ( rsltlst->rsltlst_q->q_next == rsltlst->rsltlst_q )
    {
	GCADM_CMPL_PARMS cmpl_parms;
	cmpl_parms.gcadm_scb = rsltlst->gcadm_scb;
	cmpl_parms.rslt_cb_parms = rsltlst->rslt_cb_parms;
	cmpl_parms.rslt_status = OK;
	(*rsltlst->rslt_cb)( &cmpl_parms );
	return( OK );
    }

    scb->msg_type = GCA_TDESCR;
    scb->rslt_cb = rsltlst->rslt_cb;
    scb->rslt_cb_parms = rsltlst->rslt_cb_parms; 
    scb->rsltlst_entry = (GCADM_RSLT *)rsltlst->rsltlst_q->q_next;
    scb->rsltlst_end  = rsltlst->rsltlst_q;
    scb->row_len = rsltlst->rsltlst_max_len + 2;

    id_tdesc = GCADM_global.id_tdesc++;
    if ( scb->admin_flag & GCADM_COMP_MASK )  
	modifier |= GCA_COMP_MASK;

    /* 
    ** Format GCA_TD_DATA object
    */
    td_data = (char  *) scb->buffer;
    td_data += GCA_PUTI4_MACRO( &scb->row_len, td_data);/* gca_tsize */
    td_data += GCA_PUTI4_MACRO( &modifier, td_data);	/* gca_result_modifier*/
    td_data += GCA_PUTI4_MACRO( &id_tdesc, td_data);	/* gca_id_tdescr */
    td_data += GCA_PUTI4_MACRO( &one, td_data);		/* gca_l_col_desc */
    td_data += GCA_PUTI4_MACRO( &zero, td_data);	/* db_data */
    td_data += GCA_PUTI4_MACRO( &scb->row_len, td_data);/* db_length */
    td_data += GCA_PUTI2_MACRO( &data_type, td_data);	/* db_datatype */
    td_data += GCA_PUTI4_MACRO( &zero, td_data);	/* db_prec */
    td_data += GCA_PUTI4_MACRO( &zero, td_data);	/* gca_l_attname  */

    scb->buf_len = td_data - (char *)scb->buffer;  

    /* save GCA_TD_DATA object in scb for GCA_TUPLE send */ 
    MEcopy( scb->buffer, scb->buf_len, (PTR)(&scb->tdesc_buf) );

    return ( gcadm_request( GCADM_RQST_RSLTLST, scb )); 
}


/*
** Name: gcadm_rqst_error 
**
*r Description:
r

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
**	used to identify the session context.  The callback 
**	parameter must remain valid for the duration of the 
**	call.
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
**	STATUS		OK, 
**			E_GC5000_NOT_ACTIVE,
**			E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_rqst_error( GCADM_ERROR_PARMS *error )
{
    GCADM_SCB	*scb = (GCADM_SCB *) error->gcadm_scb;
    i4		ss_code; 
	
    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM rqst_error: ADM not active\n", -1 );
        return( E_GC5000_NOT_ACTIVE );
    }
	
    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM rqst_error: entry\n", scb->aid );

    if ( GCADM_global.gcadm_trace_level >= 3 )
	TRdisplay( "%4d   GCADM rqst_error: errcode =%d\n", 
			scb->aid, error->error_code );
	
    scb->msg_type = GCA_ERROR;
    scb->rslt_cb = error->rslt_cb;
    scb->rslt_cb_parms = error->rslt_cb_parms; 

    scb->buf_len  = gcadm_format_err_data
			( scb->buffer, error->error_sqlstate,
			 error->error_code, error->error_len,
			 error->error_text);


    return ( gcadm_request( GCADM_RQST_ERROR, scb )); 
}


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
**	Caller provides a callback routine (optional) for 
**	notification that the call has completed, and 
**	a parameter block to be passed to the callback 
**	routine which may be used to identify the session 
**	context.  The callback parameter must remain valid 
**	for the duration of the call.
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
**	STATUS		OK, 
**			E_GC5000_NOT_ACTIVE,
**			E_GC5009_DUP_RQST.
**
** History:
**	 9-Sept-2003 (wansh01) 
**	    Created.
*/

STATUS
gcadm_rqst_done( GCADM_DONE_PARMS *doneparm )
{
    GCADM_SCB *scb = (GCADM_SCB *)doneparm->gcadm_scb;   
    u_i1	request;

    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM rqst_done: ADM not active\n", -1 );
        return( E_GC5000_NOT_ACTIVE );
    }

    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM rqst_done: entry\n", scb->aid );

    if ( GCADM_global.gcadm_trace_level >= 3 )
	TRdisplay( "%4d   GCADM rqst_done: rqst_status=%d \n", 
			scb->aid, doneparm->rqst_status );

    if ( doneparm->rqst_status == OK )
	request = GCADM_RQST_DONE;
    else 
    {
	request = GCADM_RQST_DISC;
	scb->status = doneparm->rqst_status;
    } 

    scb->rqst_cb = doneparm->rqst_cb;
    scb->rqst_cb_parms = doneparm->rqst_cb_parms;

    return ( gcadm_request ( request, scb )); 
}


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
**	 11-Sep-2003 (wansh01)
**	    Created.
*/

STATUS  
gcadm_term( void )
{
    GCADM_SCB *scb, *next_scb; 
    STATUS 	status; 
    GCA_DA_PARMS   gca_da_parm;

    if ( GCADM_global.gcadm_state != GCADM_ACTIVE ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	   TRdisplay( "%4d   GCADM term: ADM not active\n", -1 );
        return( E_GC5000_NOT_ACTIVE );
    }
	
    if ( GCADM_global.gcadm_trace_level >= 2 )
	TRdisplay( "%4d   GCADM term: entry \n", -1 );

    GCADM_global.gcadm_state = GCADM_TERM; 
	
    for ( 
	  next_scb = (GCADM_SCB *)GCADM_global.scb_q.q_next;
	  (PTR)next_scb != (PTR)(&GCADM_global.scb_q); 
	) 
    {
	scb =  (GCADM_SCB  *) next_scb; 
	next_scb = (GCADM_SCB *)scb->q.q_next;
	if ( scb->admin_flag & GCADM_DISC )   /* already disconneting ... */
	{
	    if ( GCADM_global.gcadm_trace_level >= 3 )
		TRdisplay( "%4d   GCADM term: skip scb (%p)\n",scb->aid, scb );
	    continue; 	
	}

	if ( GCADM_global.gcadm_trace_level >= 3 )
	    TRdisplay( "%4d   GCADM term: terminating scb (%p)\n",
					scb->aid, scb );
	MEfill( sizeof( GCA_DA_PARMS ), 0, &gca_da_parm );
	gca_da_parm.gca_association_id = scb->aid;

	gca_call( &scb->gca_cb, GCA_DISASSOC, 
		(GCA_PARMLIST *)&gca_da_parm,
		GCA_SYNC_FLAG, NULL, -1, &status);
		    
	  
	if ( ( status != OK ) || 
	    ( gca_da_parm.gca_status != OK ))
	    if ( GCADM_global.gcadm_trace_level >= 1 )
		TRdisplay( "%4d   GCADM term: disassociate error: 0x%x\n",
			   gca_da_parm.gca_association_id, status );
	
	gcadm_free_scb ( scb );
	
    }

    GCADM_global.gcadm_state = GCADM_IDLE; 
    return( OK );
}


/*
** Name: gcadm_alloc
**
** Description:
**	default routine to dynamically allocate memory.
**
** Input:
**	length	Amount of memory to be allocated.
**
** Output:
**	None.
**
** Returns:
**	PTR	Allocated memory or NULL if error.
**
** History:
**	 24-Dec-03 (wansh01)
**	    Created.
*/

static PTR
gcadm_alloc( u_i4 length )
{
    STATUS status;
    return( MEreqmem( 0, length, TRUE, &status ) );
}

/*
** Name: gcadm_free
**
** Description:
**	default routine to free dynamically allocated memory.
**
** Input:
**	ptr	Memory to be freed.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 24-Dec-03 (wansh01)
**	    Created.
*/

static VOID
gcadm_free( PTR ptr )
{
    MEfree( ptr );
    return;
}


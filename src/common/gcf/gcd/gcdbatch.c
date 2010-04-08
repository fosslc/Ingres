/*
** Copyright (c) 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>

#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>
#include <gcu.h>

/*
** Name: gcdbatch.c
**
** Description:
**	GCD batch message processing.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static GCULIST qryMap[] = 
{
    { MSG_BAT_EXQ,	"EXECUTE QUERY" },
    { MSG_BAT_EXS,	"EXECUTE STATEMENT" },
    { MSG_BAT_EXP,	"EXECUTE PROCEDURE" },
    { 0, NULL }
};

#define	BATCH_BEGIN	0	/* Init batch processing */
#define	BATCH_QUERY	1	/* Send (s) query */
#define	BATCH_PDESC	2	/* Receive (c) parameter descriptor */
#define	BATCH_SETD	3	/* Send (s) parameter descriptor */
#define	BATCH_PDATA	4	/* Receive (c) parameter data */
#define	BATCH_PUTP	5	/* Send (s) parameter data */
#define	BATCH_NEXT	6	/* Receive (c) next query */
#define	BATCH_GETQI	7	/* Receive (s) query results */
#define	BATCH_RESULT	8	/* Send (c) query results */

#define	BATCH_CANCEL	96	/* Cancel query */
#define	BATCH_CLOSE	97	/* Close query */
#define	BATCH_XACT	98	/* Check transaction state */
#define	BATCH_EXIT	99	/* Respond to client */

static	GCULIST batSeqMap[] =
{
    { BATCH_BEGIN,	"BEGIN" },
    { BATCH_QUERY,	"QUERY" },
    { BATCH_PDESC,	"PDESC" },
    { BATCH_SETD,	"SETD" },
    { BATCH_PDATA,	"PDATA" },
    { BATCH_PUTP,	"PUTP" },
    { BATCH_NEXT,	"NEXT" },
    { BATCH_GETQI,	"GETQI" },
    { BATCH_RESULT,	"RESULT" },
    { BATCH_CANCEL,	"CANCEL" },
    { BATCH_CLOSE,	"CLOSE" },
    { BATCH_XACT,	"XACT" },
    { BATCH_EXIT,	"EXIT" },
    { 0, NULL }
};


/*
** Parameter flags and query required/optional parameters.
*/
#define	BP_QTXT		0x01	/* Query text */
#define	BP_STMT		0x02	/* Statement name */
#define	BP_SCHM		0x04	/* Schema name */
#define	BP_PNAM		0x08	/* Procedure name */
#define	BP_FLAG		0x10	/* Query flags */

static	u_i2	p_req[] =
{
    0,			/* unused */
    BP_QTXT,		/* EXQ */
    0,			/* unused */
    0,			/* unused */
    BP_STMT,		/* EXS */
    0,			/* unused */
    BP_PNAM,		/* EXP */
    0,			/* unused */
    0,			/* unused */
    0,			/* unused */
};

static	u_i2	p_rpt[] =
{
    0,			/* unused */
    0,			/* EXQ */
    0,			/* unused */
    0,			/* unused */
    0,			/* EXS */
    0,			/* unused */
    0,			/* EXP */
    0,			/* unused */
    0,			/* unused */
    0,			/* unused */
};

static	u_i2	p_opt[] =
{
    0,			/* unused */
    BP_FLAG,		/* EXQ */
    0,			/* unused */
    0,			/* unused */
    BP_FLAG,		/* EXS */
    0,			/* unused */
    BP_SCHM | BP_FLAG,	/* EXP */
    0,			/* unused */
    0,			/* unused */
    0,			/* unused */
};


static	void	msg_batch_sm( PTR );



/*
** Name: gcd_msg_batch
**
** Description:
**	Process a GCD batch message.
**
** Input:
**	ccb	Connection control block.
**	first	Is this first request in batch.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

void
gcd_msg_batch( GCD_CCB *ccb, bool first )
{
    GCD_PCB		*pcb;
    GCD_SCB		*scb;
    GCD_RCB		*rcb = NULL;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    bool		incomplete = FALSE;
    STATUS		status;
    u_i4		qtxt_len, flags;
    u_i1		t_ui1;
    u_i2		t_ui2, qtype;
    u_i2		p_rec, req, opt;
    u_i2		p1_len, p2_len;
    char		p1_buff[ GCD_NAME_MAX + 1 ];
    char		*p1 = NULL;
    char		p2_buff[ GCD_NAME_MAX + 1 ];
    char		*p2 = NULL;

    /*
    ** Read an validate the query type.
    */
    if ( ! gcd_get_i2( ccb, (u_i1 *)&qtype ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no batch op\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    switch( qtype )
    {
    case MSG_BAT_EXQ :
    case MSG_BAT_EXS :
    case MSG_BAT_EXP :
	/* OK */
	break;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid batch op: %d\n", ccb->id, qtype );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Batch operation: %s\n",
		   ccb->id, gcu_lookup( qryMap, qtype ) );

    if ( ccb->msg.dtmc )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD Queries not allowed on DTMC\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( ccb->qry.qtxt )  ccb->qry.qtxt[0] = EOS;
    ccb->qry.flags = 0;
    qtxt_len = 0;
    flags = 0;
    p_rec = 0;

    /*
    ** Read the batch parameters.
    */
    while( ccb->msg.msg_len )
    {
	DAM_ML_PM bp;

	incomplete = TRUE;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&bp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&bp.val_len ) )  break;

	switch( bp.param_id )
	{
	case MSG_BP_QTXT :
	    if ( ! gcd_expand_qtxt( ccb, qtxt_len + bp.val_len + 1, TRUE )  ||
	    	 ! gcd_get_bytes( ccb, bp.val_len,
		 		  (u_i1 *)ccb->qry.qtxt + qtxt_len ) )
		break;

	    qtxt_len += bp.val_len;
	    ccb->qry.qtxt[ qtxt_len ] = EOS;
	    if ( bp.val_len > 0 )  p_rec |= BP_QTXT;
	    incomplete = FALSE;
	    break;

	case MSG_BP_STMT_NAME :
	case MSG_BP_PROC_NAME :
	    if ( p1 )  break;	/* Already set? Should not happen */
	    p1_len = bp.val_len;

	    if ( p1_len < sizeof( p1_buff ) )
		p1 = p1_buff;
	    else  if ( ! (p1 = (char *)MEreqmem(0, p1_len + 1, FALSE, NULL)) )
		break;

	    if ( ! gcd_get_bytes( ccb, p1_len, (u_i1 *)p1 ) )  break;
	    p1[ p1_len ] = EOS;
	    incomplete = FALSE;

	    if ( p1_len > 0 )
		switch( bp.param_id )
		{
		case MSG_BP_STMT_NAME :  p_rec |= BP_STMT;	break;
		case MSG_BP_PROC_NAME :  p_rec |= BP_PNAM;	break;
		}
	    break;

	case MSG_BP_SCHEMA_NAME :
	    if ( p2 )  break;	/* Already set?  Should not happen */
	    p2_len = bp.val_len;

	    if ( p2_len < sizeof( p2_buff ) )
		p2 = p2_buff;
	    else  if ( ! (p2 = (char *)MEreqmem(0, p2_len + 1, FALSE, NULL)) )
		break;

	    if ( ! gcd_get_bytes( ccb, p2_len, (u_i1 *)p2 ) )  break;
	    p2[ p2_len ] = EOS;
	    incomplete = FALSE;
	    p_rec |= BP_SCHM;
	    break;

	case MSG_BP_FLAGS :
	    incomplete = FALSE;

	    switch( bp.val_len )
	    {
	    case 1 :
		if ( ! gcd_get_i1( ccb, (u_i1 *)&t_ui1 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui1;
		    p_rec |= BP_FLAG;
		}
		break;

	    case 2 :
		if ( ! gcd_get_i2( ccb, (u_i1 *)&t_ui2 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui2;
		    p_rec |= BP_FLAG;
		}
		break;

	    case 4 :
		if ( ! gcd_get_i4( ccb, (u_i1 *)&flags ) )
		    incomplete = TRUE;
		else
		    p_rec |= BP_FLAG;
		break;

	    default :
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD     Invalid query flag len: %d\n",
		    	       ccb->id, bp.val_len );
		incomplete = TRUE;
		break;
	    }
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     Unknown parameter ID %d\n",
			   ccb->id, bp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    req = (flags & MSG_BF_REPEAT) ? p_rpt[ qtype ] : p_req[ qtype ];
    opt = p_opt[ qtype ];

    if ( incomplete  ||  (p_rec & req) != req  ||  (p_rec & ~(req | opt)) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    if ( incomplete )
		TRdisplay( "%4d    GCD unable to read all query parameters\n",
			   ccb->id );
	    else
		TRdisplay( "%4d    GCD invalid or missing query parameters\n",
			   ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( ! (pcb = gcd_new_pcb( ccb )) )
    {
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	goto cleanup;
    }

    if ( ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	goto cleanup;
    }

    gcd_alloc_qdesc( &ccb->qry.svc_parms, 0, NULL );	/* Init query params */
    gcd_alloc_qdesc( &ccb->qry.qry_parms, 0, NULL );
    gcd_alloc_qdesc( &ccb->qry.all_parms, 0, NULL );
    ccb->qry.api_parms = &ccb->qry.qry_parms;
    status = OK;

    /*
    ** Only the REPEAT flag is applicable to batch processing,
    ** and it is only used within this routine.  Other flags
    ** are ignored (they would otherwise be propogated here).
    */

    switch( qtype )
    {
    case MSG_BAT_EXQ :
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.text = ccb->qry.qtxt;
	pcb->data.query.params = 
	    (ccb->msg.msg_flags & (MSG_HDR_EOB | MSG_HDR_EOG)) ? FALSE : TRUE;
	break;

    case MSG_BAT_EXS :
	pcb->data.query.type = IIAPI_QT_EXEC;

	if ( ! (flags & MSG_BF_REPEAT) )
	{
	    if ( ! gcd_expand_qtxt( ccb, p1_len + 9, FALSE ) )
	    {
		status = E_GC4808_NO_MEMORY;
		break;
	    }

	    STprintf( ccb->qry.qtxt, "execute %s", p1 );
	}

	pcb->data.query.text = ccb->qry.qtxt;
	pcb->data.query.params = 
	    (ccb->msg.msg_flags & (MSG_HDR_EOB | MSG_HDR_EOG)) ? FALSE : TRUE;
	break;

    case MSG_BAT_EXP :
    {
	u_i2	schema_len, proc_len;

	ccb->qry.flags |= QRY_PROCEDURE;
	pcb->data.query.type = IIAPI_QT_EXEC_PROCEDURE;
	pcb->data.query.text = NULL;
	pcb->data.query.params = TRUE;
	
	if ( ! (flags & MSG_BF_REPEAT) )
	{
	    /*
	    ** Save procedure name and (optional) schema 
	    ** name on first execution of a procedure.
	    */
	    proc_len = p1_len;

	    if ( p1 == p1_buff )
		ccb->qry.proc_name = STalloc( p1 );
	    else
	    {
		/* Hijack dynamic allocation of parameter */
		ccb->qry.proc_name = p1;
		p1 = NULL;
	    }

	    if ( ! (p_rec & BP_SCHM) )
		schema_len = 0;
	    else
	    {
		schema_len = p2_len;

		if ( p2 == p2_buff )
		    ccb->qry.schema_name = STalloc( p2 );
		else
		{
		    /* Hijack dynamic allocation of parameter */
		    ccb->qry.schema_name = p2;
		    p2 = NULL;
		}
	    }
	}
	else
	{
	    /*
	    ** Use saved procedure and (optional) schema
	    ** names when repeating procedure execution.
	    */
	    if ( ccb->qry.proc_name )  
		proc_len = STlength( ccb->qry.proc_name );
	    else
		proc_len = 0;

	    if ( ccb->qry.schema_name )
		schema_len = STlength( ccb->qry.schema_name );
	    else
		schema_len = 0;
	}

	if ( (proc_len  &&  ! ccb->qry.proc_name)  ||
	     (schema_len  &&  ! ccb->qry.schema_name) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms,
				schema_len ? 2 : 1, NULL ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_nullable = FALSE;
	desc->ds_length = proc_len;

	if ( schema_len )
	{
	    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[1];
	    desc->ds_columnType = IIAPI_COL_SVCPARM;
	    desc->ds_dataType = IIAPI_CHA_TYPE;
	    desc->ds_nullable = FALSE;
	    desc->ds_length = schema_len;
	}

	if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
	data->dv_null = FALSE;
	data->dv_length = proc_len;
	if ( proc_len )  MEcopy( (PTR)ccb->qry.proc_name, 
				 proc_len, data->dv_value );

	if ( schema_len )
	{
	    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[1];
	    data->dv_null = FALSE;
	    data->dv_length = schema_len;
	    MEcopy( (PTR)ccb->qry.schema_name, schema_len, data->dv_value );
	}

	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;
    }
    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid query operation: %d\n",
	    	       ccb->id, qtype );
	status = E_GC480A_PROTOCOL_ERROR;
	break;
    }

    if ( status != OK )
    {
	gcd_sess_error( ccb, &pcb->rcb, status );
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );
	gcd_msg_pause( ccb, TRUE );
	goto cleanup;
    }

    if ( first )
    {
	/*
	** Initialize for new batch.
	*/
	ccb->api.stmt = NULL;
	ccb->sequence = BATCH_BEGIN;
    }
    else
    {
	/*
	** Additional batch request, possibly repeat of previous.
	*/
	ccb->sequence = (flags & MSG_BF_REPEAT) ? BATCH_PDESC : BATCH_QUERY;
    }

    ccb->qry.qry_sm = msg_batch_sm;
    msg_batch_sm( (PTR)pcb );

  cleanup:

    /*
    ** Release allocated resources.
    */
    if ( p1  &&  p1 != p1_buff )  MEfree( (PTR)p1 );
    if ( p2  &&  p2 != p2_buff )  MEfree( (PTR)p2 );

    return;
}


/*
** Name: msg_batch_sm
**
** Description:
**	Batch processing sequencer.  Callback function for API
**	batch functions.
**
** Input:
**	arg	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static void
msg_batch_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    GCD_SCB	*scb;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Batch: %s\n",
		   ccb->id, gcu_lookup( batSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case BATCH_BEGIN :		/* Initialize batch state */
	/*
	** Establish transaction state for batch processing
	** on first request.  Subsequent requests should
	** enter at BATCH_QUERY.
	*/
	gcd_push_callback( pcb, msg_batch_sm );
	gcd_xact_check( pcb, GCD_XQOP_NON_CURSOR );
	return;

    case BATCH_QUERY :		/* Batch query */
	if ( pcb->api_error )
	{
	    ccb->sequence = BATCH_XACT;
	    break;
	}

	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_batch( pcb );
	return;

    case BATCH_PDESC :		/* Receive parameter descriptor? */
	if ( pcb->api_error )
	{
	    ccb->sequence = BATCH_CANCEL;
	    break;
	}

	/*
	** Read DESC message if not at end of current batch entry.
	*/
	if ( ! (ccb->msg.msg_flags & (MSG_HDR_EOB | MSG_HDR_EOG)) )
	{
	    ccb->qry.flags |= QRY_NEED_DESC;
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, FALSE );
	    return;
	}

	/*
	** There are no query parameters, but there may be
	** API service parameters which need to be sent.
	*/
	if ( ccb->qry.svc_parms.max_cols < 1 )
	    ccb->sequence = BATCH_NEXT;		/* No parameters */
	else
	{
	    /* API service parameters */
	    ccb->qry.api_parms = &ccb->qry.svc_parms;
	    ccb->qry.flags |= QRY_HAVE_DESC;
	}
	break;

    case BATCH_SETD :		/* Send descriptor to server */
	if ( ! (ccb->qry.flags & QRY_HAVE_DESC) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD batch: no descriptor\n", ccb->id );
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return;
	}

	/*
	** If there are any query parameters, allocate
	** storage space and combine with the API service
	** parameters (if any).
	*/
	if ( ccb->qry.qry_parms.max_cols )
	{
	    /*
	    ** Allocate space for one set of parameters.
	    */
	    if ( ! gcd_alloc_qdata( &ccb->qry.qry_parms, 1 ) )
	    {
		gcd_del_pcb( pcb );
		gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		return;
	    }

	    /*
	    ** If there are any API server parameters, they 
	    ** need to be combined with the query parameters.
	    */
	    if ( ccb->qry.svc_parms.max_cols )
	    {
		if ( ! gcd_build_qdata( &ccb->qry.svc_parms,
					&ccb->qry.qry_parms,
					&ccb->qry.all_parms ) )
		{
		    gcd_del_pcb( pcb );
		    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		    return;
		}

		ccb->qry.api_parms = &ccb->qry.all_parms;
	    }
	}

	ccb->qry.flags &= ~(QRY_NEED_DESC | QRY_HAVE_DESC);
	pcb->data.desc.count = ccb->qry.api_parms->max_cols;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_setDesc( pcb );
        return;

    case BATCH_PDATA :		/* Receive Parameter Data? */
	if ( pcb->api_error )
	{
	    ccb->sequence = BATCH_CANCEL;
	    break;
	}

	/*
	** Read DATA message if any query parameters.
	*/
	if ( ccb->qry.qry_parms.max_cols )
	{
	    ccb->qry.flags |= QRY_NEED_DATA;
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, FALSE );
	    return;
	}

	/*
	** There are only service parameters and data is available.
	*/
	ccb->qry.flags |= QRY_HAVE_DATA;
	break;

    case BATCH_PUTP :		/* Send parameter data to server */
	if ( ! (ccb->qry.flags & QRY_HAVE_DATA) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
	        TRdisplay( "%4d    GCD batch: no parameter data\n", ccb->id );
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return;
	}

	ccb->qry.flags &= ~(QRY_NEED_DATA | QRY_HAVE_DATA);
	
	/*
	** If API service parameters and query parameters
	** have been combined, determine what portion of the
	** combined parameters are available for processing.
	** If parameters have not been combined, api_parms
	** points at either the API service parameters or 
	** query parameters and is ready for processing.
	*/
	if ( ccb->qry.api_parms == &ccb->qry.all_parms )
	    gcd_combine_qdata( &ccb->qry.svc_parms, &ccb->qry.qry_parms,
			       &ccb->qry.all_parms );

	/*
	** It is possible for the query parameters to include
	** a LOB which is not a part of the combined set.
	** During the processing of the LOB, there would be
	** no parameters to send to the server, so we need
	** to loop receiving (and discarding) the LOB from
	** the client until a processable parameter is
	** received (normal processing) or end-of-data.  The
	** next action handles any branching/looping needed.
	*/
	if ( ! ccb->qry.api_parms->col_cnt )  break;

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = &((IIAPI_DATAVALUE *)ccb->qry.api_parms->data)
					[ ccb->qry.api_parms->cur_col ];
	pcb->data.data.more_segments = ccb->qry.api_parms->more_segments;
	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_putParm( pcb );
        return;

    case BATCH_NEXT :		/* Check for next batch query */
	if ( pcb->api_error )
	{
	    ccb->sequence = BATCH_CANCEL;
	    break;
	}

	/*
	** Consume the available parameters.  Don't consume 
	** the last parameter during LOB processing.
	*/
	ccb->qry.api_parms->cur_col += ccb->qry.api_parms->col_cnt;
	if ( ccb->qry.api_parms->more_segments ) ccb->qry.api_parms->cur_col--;
	ccb->qry.api_parms->col_cnt = 0;

	if ( ccb->qry.qry_parms.cur_col < ccb->qry.qry_parms.max_cols )
	{
	    /* Get more query parameter data */
	    ccb->qry.flags |= QRY_NEED_DATA;
	    ccb->sequence = BATCH_PUTP;
	    gcd_del_pcb( pcb );

	    if ( ccb->msg.msg_len )
	    	gcd_msg_data( ccb );		/* Finish current message */
	    else
		gcd_msg_pause( ccb, FALSE );	/* Check for more messages */
	    return;
	}

	/*
	** Loop processing batch messages until EOG.
	*/
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG ) )
	{
	    ccb->sequence = BATCH_QUERY;
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, FALSE );
	    return;
	}
	break;

    case BATCH_GETQI :		/* Get query result info */
	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_gqi( pcb );
	return;

    case BATCH_RESULT :		/* Send query result info */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = BATCH_EXIT;
	    break;
	}

	/*
	** Result processing loops until no further results
	** are available.
	*/
	if ( pcb->api_status == IIAPI_ST_NO_DATA )
	    ccb->sequence = BATCH_CLOSE;
	else
	{
	    /*
	    ** Return result info for batch entry.
	    */
	    gcd_send_result( ccb, &pcb->rcb, pcb, MSG_HDR_EOB );
	    ccb->sequence = BATCH_GETQI;
	}
	break;

    case BATCH_CANCEL :		/* Cancel batch */
	gcd_msg_flush( ccb );

	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = BATCH_EXIT;
	    break;
	}

	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_cancel( pcb );
	return;

    case BATCH_CLOSE :		/* Close batch */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = BATCH_EXIT;
	    break;
	}

	gcd_push_callback( pcb, msg_batch_sm );
	gcd_api_close( pcb, NULL );
	return;

    case BATCH_XACT :		/* Check transaction state */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	    break;	/* Handled below */

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	{
	    gcd_push_callback( pcb, msg_batch_sm );
	    gcd_xact_abort( pcb );
	    return;
	}
	break;

    case BATCH_EXIT :		/* Batch completed */
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );

	if ( ccb->cib->flags & GCD_CONN_ABORT )
	    gcd_sess_abort( ccb, FAIL );
	else  if ( ccb->cib->flags & GCD_LOGIC_ERR )
	    gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	else
	    gcd_msg_pause( ccb, TRUE );
	return;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid batch processing sequence: %d\n",
	    	       ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	return;
    }

    goto top;
}


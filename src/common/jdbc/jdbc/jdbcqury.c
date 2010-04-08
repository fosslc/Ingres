/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcqury.c
**
** Description:
**	JDBC query message processing.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Extracted to this file.
**	26-Oct-99 (gordy)
**	    Implemented support for cursor positioned DELETE/UPDATE.
**	 4-Nov-99 (gordy)
**	    Updated calling sequence of jdbc_send_done() to allow
**	    control of ownership of RCB parameter.
**	13-Dec-99 (gordy)
**	    Calculate the pre-fetch limit for READONLY cursors.
**	17-Jan-00 (gordy)
**	    If autocommit requested but not enabled, enable autocommit
**	    before processing query.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	19-May-00 (gordy)
**	    Added new execution sequence to handle select loops.
**	28-Jul-00 (gordy)
**	    Reworked query state machine to combine processing into
**	    a single generic sequence driven by client data and flags
**	    specific to each query type.  Added support for EXECUTE
**	    PROCEDURE.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	12-Oct-00 (gordy)
**	    Localize transaction handling in jdbc_xact_check().
**	19-Oct-00 (gordy)
**	    Added XACT action to signal cursor close to transaction
**	    sub-system for autocommit simulation.
**	15-Nov-00 (gordy)
**	    Pre-fetch limit now calculated for all result-sets and
**	    limit is only sent if greater than one.
**	 7-Mar-01 (gordy)
**	    Rename expand_qtxt() to jdbc_expand_qtxt() and made global.
**	18-Apr-01 (gordy)
**	    Queries not permitted on DTM connections.
**	10-May-01 (gordy)
**	    Added support for UCS2 data.
**	 1-Jun-01 (gordy)
**	    Fixed BUG 104830: protocol error when DATA message contains
**	    a BLOB segment, end-of-segments indicator and an additional
**	    parameter value.
**	 2-Jul-01 (gordy)
**	    Calculate character and byte length of BLOB segments.
**      13-Aug-01 (loera01) SIR 105521
**          Added new proc parameter flag JDBC_DSC_GTT to support global temp 
**          table parameters for db procedures.
**	20-Aug-01 (gordy)
**	    Added query flag parameter and READONLY flag so that
**	    the 'FOR READONLY' clause can be appended for OpenAPI.
**	20-May-2002 (hanje04)
**	    Backed out change 457149 (no comment). Not needed in main
**      13-Jan-03 (weife01) Bug 109369
**          Fixed problem when inserting multiple short blob.    
**	 7-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
*/	

#define	FOR_READONLY	" for readonly"

/*
** Query sequencing.
*/

static	GCULIST	qryMap[] =
{
    { JDBC_QRY_EXQ,	"EXECUTE QUERY" },
    { JDBC_QRY_OCQ,	"OPEN CURSOR FOR QUERY" },
    { JDBC_QRY_PREP,	"PREPARE STATEMENT" },
    { JDBC_QRY_EXS,	"EXECUTE STATEMENT" },
    { JDBC_QRY_OCS,	"OPEN CURSOR FOR STATEMENT" },
    { JDBC_QRY_EXP,	"EXECUTE PROCEDURE" },
    { JDBC_QRY_CDEL,	"CURSOR DELETE" },
    { JDBC_QRY_CUPD,	"CURSOR UPDATE" },
    { 0, NULL }
};

#define	QRY_QUERY	1	/* Send (s) query */
#define	QRY_PDESC	2	/* Receive (c) parameter descriptor */
#define	QRY_SETD	3	/* Send (s) parameter descriptor */
#define	QRY_PDATA	4	/* Receive (c) parameters */
#define	QRY_PUTP	5	/* Send (s) parameters */
#define	QRY_GETD	6	/* Receive (s) result descriptor */
#define	QRY_GETQI	7	/* Receive (s) query results */
#define	QRY_DONE	8	/* Finalize query processing */

#define	QRY_CANCEL	96	/* Cancel query */
#define	QRY_CLOSE	97	/* Close query */
#define	QRY_XACT	98	/* Check transaction state */
#define	QRY_EXIT	99	/* Respond to client */

static	GCULIST qrySeqMap[] =
{
    { QRY_QUERY,	"QUERY" },
    { QRY_PDESC,	"PDESC" },
    { QRY_SETD,		"SETD" },
    { QRY_PDATA,	"PDATA" },
    { QRY_PUTP,		"PUTP" },
    { QRY_GETD,		"GETD" },
    { QRY_GETQI,	"GETQI" },
    { QRY_DONE,		"DONE" },
    { QRY_CANCEL,	"CANCEL" },
    { QRY_CLOSE,	"CLOSE" },
    { QRY_XACT,		"XACT" },
    { QRY_EXIT,		"EXIT" },
    { 0, NULL }
};


/*
** Parameter flags and query required/optional parameters.
*/
#define	QP_QTXT		0x01	/* Query text */
#define	QP_CRSR		0x02	/* Cursor name */
#define	QP_STMT		0x04	/* Statement name */
#define	QP_SCHM		0x08	/* Schema name */
#define	QP_PNAM		0x10	/* Procedure name */
#define	QP_FLAG		0x20	/* Query flags */

static	u_i2	p_req[] =
{
    0,			/* unused */
    QP_QTXT,		/* EXQ */
    QP_CRSR | QP_QTXT,	/* OCQ */
    QP_STMT | QP_QTXT,	/* PREP */
    QP_STMT,		/* EXS */
    QP_CRSR | QP_STMT,	/* OCS */
    QP_PNAM,		/* EXP */
    QP_CRSR | QP_QTXT,	/* CDEL */
    QP_CRSR | QP_QTXT,	/* CUPD */
};

static	u_i2	p_opt[] = 
{ 
    0, 			/* unused */
    0, 			/* EXQ */
    QP_FLAG,		/* OCQ */
    0, 			/* PREP */
    0, 			/* EXS */
    QP_FLAG, 		/* OCS */
    QP_SCHM, 		/* EXP */
    0, 			/* CDEL */
    0, 			/* CDEL */
};

static	void	msg_query_sm( PTR );
static	void	send_desc( JDBC_CCB *, JDBC_RCB **, u_i2, IIAPI_DESCRIPTOR * );
static	void	calc_pre_fetch( JDBC_PCB *, u_i2, IIAPI_DESCRIPTOR *, u_i2 );
static	void	calc_params( QDATA *, QDATA *, QDATA * );



/*
** Name: jdbc_msg_query
**
** Description:
**	Process a JDBC query message.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Renamed for external reference.
**	26-Oct-99 (gordy)
**	    Implemented support for cursor positioned DELETE/UPDATE.
**	17-Jan-00 (gordy)
**	    If autocommit requested but not enabled, enable autocommit
**	    before processing query.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	31-May-00 (rajus01)
**	    Send error message to the client instead of aborting
**	    the connection.
**	28-Jul-00 (gordy)
**	    Generalized parameter handling with flag sets for required
**	    and optional parameters.  Initialize each query type for
**	    new state machine settings.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	12-Oct-00 (gordy)
**	    Localize transaction handling in jdbc_xact_check().
**	18-Apr-01 (gordy)
**	    Queries not permitted on DTM connections.
**	20-Aug-01 (gordy)
**	    Added query flag parameter and READONLY flag so that
**	    the 'FOR READONLY' clause can be appended for OpenAPI.
*/

void
jdbc_msg_query( JDBC_CCB *ccb )
{
    JDBC_PCB		*pcb;
    JDBC_SCB		*scb;
    JDBC_RCB		*rcb = NULL;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    bool		incomplete = FALSE;
    STATUS		status;
    u_i1		t_ui1, xqop = JDBC_XQOP_NON_CURSOR;
    u_i2		t_ui2, qtype, p_rec = 0;
    u_i2		crsr_name_len, stmt_name_len;
    u_i2		schema_name_len, proc_name_len;
    u_i4		flags = 0;
    char		stmt_name[ JDBC_NAME_MAX + 1 ];
    char		schema_name[ JDBC_NAME_MAX + 1 ];
    char		proc_name[ JDBC_NAME_MAX + 1 ];

    /*
    ** Read and validate the query type.
    */
    if ( ! jdbc_get_i2( ccb, (u_i1 *)&qtype ) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC no query type\n", ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( qtype > JDBC_QRY_MAX )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid query type: %d\n", ccb->id, qtype );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC query request: %s\n", 
		   ccb->id, gcu_lookup( qryMap, qtype ) );

    if ( ccb->conn_info.dtmc )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC Queries not allowed on DTMC\n", ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( ccb->qry.qtxt_max )  ccb->qry.qtxt[0] = EOS;
    ccb->qry.crsr_name[0] = EOS;
    ccb->qry.flags = 0;

    /*
    ** Read the query parameters.
    */
    while( ccb->msg.msg_len )
    {
	JDBC_MSG_PM qp;

	incomplete = TRUE;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&qp.param_id ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&qp.val_len ) )  break;

	switch( qp.param_id )
	{
	case JDBC_QP_QTXT :
	    if ( ! jdbc_expand_qtxt( ccb, qp.val_len + 1, FALSE )  ||
	         ! jdbc_get_bytes( ccb, qp.val_len, (u_i1 *)ccb->qry.qtxt ) )
		break;

	    ccb->qry.qtxt[ qp.val_len ] = EOS;
	    if ( qp.val_len > 0 )  p_rec |= QP_QTXT;
	    incomplete = FALSE;
	    break;
	
	case JDBC_QP_CRSR_NAME :
	    if ( qp.val_len >= sizeof( ccb->qry.crsr_name )  ||
	    	 ! jdbc_get_bytes(ccb, qp.val_len, (u_i1 *)ccb->qry.crsr_name) )
		break;
	    
	    ccb->qry.crsr_name[ qp.val_len ] = EOS;
	    crsr_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= QP_CRSR;
	    incomplete = FALSE;
	    break;

	case JDBC_QP_STMT_NAME :
	    if ( qp.val_len >= sizeof( stmt_name )  ||
	    	 ! jdbc_get_bytes( ccb, qp.val_len, 
				   (u_i1 *)stmt_name ) )
		break;
	    
	    stmt_name[ qp.val_len ] = EOS;
	    stmt_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= QP_STMT;
	    incomplete = FALSE;
	    break;

	case JDBC_QP_SCHEMA_NAME :
	    if ( qp.val_len >= sizeof( schema_name )  ||
	    	 ! jdbc_get_bytes( ccb, qp.val_len, (u_i1 *)schema_name ) )
		break;
	    
	    schema_name[ qp.val_len ] = EOS;
	    schema_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= QP_SCHM;
	    incomplete = FALSE;
	    break;
	
	case JDBC_QP_PROC_NAME :
	    if ( qp.val_len >= sizeof( proc_name )  ||
	    	 ! jdbc_get_bytes(ccb, qp.val_len, (u_i1 *)proc_name) )
		break;
	    
	    proc_name[ qp.val_len ] = EOS;
	    proc_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= QP_PNAM;
	    incomplete = FALSE;
	    break;

	case JDBC_QP_FLAGS :
	    incomplete = FALSE;

	    switch( qp.val_len )
	    {
	    case 1 :
		if ( ! jdbc_get_i1( ccb, (u_i1 *)&t_ui1 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui1;
		    p_rec |= QP_FLAG;
		}
		break;

	    case 2 :
		if ( ! jdbc_get_i2( ccb, (u_i1 *)&t_ui2 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui2;
		    p_rec |= QP_FLAG;
		}
		break;

	    case 4 :
		if ( ! jdbc_get_i4( ccb, (u_i1 *)&flags ) )
		    incomplete = TRUE;
		else
		    p_rec |= QP_FLAG;
		break;

	    default :
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC     Invalid query flag len: %d\n",
			       ccb->id, qp.val_len );
		incomplete = TRUE;
		break;
	    }
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC     unkown parameter ID %d\n",
			   ccb->id, qp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete  ||
         (p_rec & p_req[ qtype ]) != p_req[ qtype ]  ||
	 (p_rec & ~(p_req[ qtype ] | p_opt[ qtype ])) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    if ( incomplete )
		TRdisplay( "%4d    JDBC unable to read all query parameters\n",
			   ccb->id );
	    else
		TRdisplay( "%4d    JDBC invalid or missing query parameters\n",
			   ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    jdbc_alloc_qdesc( &ccb->qry.svc_parms, 0, NULL );  /* Init query params */
    jdbc_alloc_qdesc( &ccb->qry.qry_parms, 0, NULL );
    jdbc_alloc_qdesc( &ccb->qry.all_parms, 0, NULL );
    ccb->sequence = QRY_QUERY;	
    ccb->qry.api_parms = &ccb->qry.qry_parms;
    status = OK;

    switch( qtype )
    {
    case JDBC_QRY_EXQ :	
	ccb->qry.flags |= QRY_TUPLES;
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case JDBC_QRY_OCQ :	
	ccb->qry.flags |= QRY_CURSOR;
	xqop = JDBC_XQOP_CURSOR_OPEN;

	if ( flags & JDBC_QF_READONLY )
	{
	    u_i2 l1 = STlength( ccb->qry.qtxt );
	    u_i2 l2 = STlength( FOR_READONLY );

	    if ( ! jdbc_expand_qtxt( ccb, l1 + l2 + 1, TRUE ) )
	    {
		status = E_JD0108_NO_MEMORY;
		break;
	    }

	    STcat( ccb->qry.qtxt, FOR_READONLY );
	}

	pcb->data.query.type = IIAPI_QT_OPEN;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! jdbc_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_length = crsr_name_len;
	desc->ds_nullable = FALSE;

	if ( ! jdbc_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = crsr_name_len;
	MEcopy( (PTR)ccb->qry.crsr_name, crsr_name_len, data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case JDBC_QRY_PREP:	
	{
	    char prefix[ 64 ];
	    u_i2 i, j, l1, l2 = STlength( ccb->qry.qtxt );

	    STprintf( prefix, "prepare %s into sqlda from ", stmt_name );
	    l1 = STlength( prefix );

	    if ( ! jdbc_expand_qtxt( ccb, l1 + l2 + 1, TRUE ) )
	    {
		status = E_JD0108_NO_MEMORY;
		break;
	    }

	    for( i = l2, j = i + l1, l2++; l2; i--, j--, l2-- )
		ccb->qry.qtxt[ j ] = ccb->qry.qtxt[ i ];

	    MEcopy( (PTR)prefix, l1, (PTR)ccb->qry.qtxt );
	}

	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case JDBC_QRY_EXS :	
	if ( ! jdbc_expand_qtxt( ccb, stmt_name_len + 9, FALSE ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	STprintf( ccb->qry.qtxt, "execute %s", stmt_name );
	pcb->data.query.type = IIAPI_QT_EXEC;
	pcb->data.query.params = (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case JDBC_QRY_OCS :	
	ccb->qry.flags |= QRY_CURSOR;
	xqop = JDBC_XQOP_CURSOR_OPEN;

	{
	    u_i2 len = stmt_name_len + (flags & JDBC_QF_READONLY)  
				       ? STlength( FOR_READONLY ) : 0;

	    if ( ! jdbc_expand_qtxt( ccb, len + 1, FALSE ) )
	    {
		status = E_JD0108_NO_MEMORY;
		break;
	    }

	    STcopy( stmt_name, ccb->qry.qtxt );
	    if (flags & JDBC_QF_READONLY)  STcat(ccb->qry.qtxt, FOR_READONLY);
	}

	pcb->data.query.type = IIAPI_QT_OPEN;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! jdbc_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_length = crsr_name_len;
	desc->ds_nullable = FALSE;

	if ( ! jdbc_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = crsr_name_len;
	MEcopy( (PTR)ccb->qry.crsr_name, crsr_name_len, data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case JDBC_QRY_EXP :	
	ccb->qry.flags |= QRY_PROCEDURE | QRY_TUPLES;
	pcb->data.query.type = IIAPI_QT_EXEC_PROCEDURE;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = NULL;

	if ( ! jdbc_alloc_qdesc( &ccb->qry.svc_parms, 
				 (p_rec & QP_SCHM) ? 2 : 1, NULL ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_nullable = FALSE;
	desc->ds_length = proc_name_len;

	if ( p_rec & QP_SCHM )
	{
	    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[1];
	    desc->ds_columnType = IIAPI_COL_SVCPARM;
	    desc->ds_dataType = IIAPI_CHA_TYPE;
	    desc->ds_nullable = FALSE;
	    desc->ds_length = schema_name_len;
	}

	if ( ! jdbc_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}
	
	data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
	data->dv_null = FALSE;
	data->dv_length = proc_name_len;
	MEcopy( (PTR)proc_name, proc_name_len, data->dv_value );

	if ( p_rec & QP_SCHM )
	{
	    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[1];
	    data->dv_null = FALSE;
	    data->dv_length = schema_name_len;
	    MEcopy( (PTR)schema_name, schema_name_len, data->dv_value );
	}

	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case JDBC_QRY_CDEL :
    case JDBC_QRY_CUPD :
	xqop = JDBC_XQOP_CURSOR_UPDATE;

	if ( ! (scb = jdbc_find_cursor( ccb, ccb->qry.crsr_name )) )
	{
	    status = E_JD0111_NO_STMT;
	    break;
	}

	pcb->data.query.type = (qtype == JDBC_QRY_CDEL)
				? IIAPI_QT_CURSOR_DELETE 
				: IIAPI_QT_CURSOR_UPDATE;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! jdbc_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}
	
	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_HNDL_TYPE;
	desc->ds_length = sizeof( scb->handle );
	desc->ds_nullable = FALSE;

	if ( ! jdbc_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}
	
	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = sizeof( ccb->qry.handle );
	MEcopy( (PTR)&scb->handle, sizeof( scb->handle ), data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    default : /* shouldn't happen, qtype checked above */
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay("%4d    JDBC invalid query type: %d\n", ccb->id, qtype);
	status = E_JD010A_PROTOCOL_ERROR;
	break;
    }

    if ( status != OK )
    {
	jdbc_sess_error( ccb, &pcb->rcb, status);
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	return;
    }

    /*
    ** Before processing the query request,
    ** make sure the transaction state has
    ** been properly established for the
    ** current transaction mode and query
    ** type.
    */
    jdbc_push_callback( pcb, msg_query_sm );
    jdbc_xact_check( pcb, xqop );
    return;
}


/*
** Name: msg_query_sm
**
** Description:
**	Query processing sequencer.  Callback function for API
**	query functions.
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
**	 3-Jun-99 (gordy)
**	    Created.
**	26-Oct-99 (gordy)
**	    Implemented support for cursor positioned DELETE/UPDATE.
**	13-Dec-99 (gordy)
**	    Calculate the pre-fetch limit for READONLY cursors.
**	17-Jan-00 (gordy)
**	    Calling sequence of jdbc_new_stmt() changed to permit
**	    temporary statements.
**	19-May-00 (gordy)
**	    When executing a query, look to see if there is a tuple
**	    descriptor.  If not, functionality remains the same.
**	    Otherwise, prepare to process a select loop, which when
**	    using OpenAPI is very similar to cursor processing,
**	    the only main difference being that the select loop
**	    statement remains active until end-of-tuples or closed.
**	28-Jul-00 (gordy)
**	    Combined query specific actions into a single sequence
**	    of generalized actions driver by the messages in the
**	    message stream and flags specific to each query type.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	19-Oct-00 (gordy)
**	    Added XACT action to signal cursor close to transaction
**	    sub-system for autocommit simulation.
**	15-Nov-00 (gordy)
**	    We now calculate the pre-fetch limit for all result-sets.
**	 7-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
*/

static void
msg_query_sm( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;
    JDBC_SCB	*scb;

  top:

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC query process seq %s\n",
		   ccb->id, gcu_lookup( qrySeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case QRY_QUERY :	
	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_query( pcb );
	return;
    
    case QRY_PDESC :	
	if ( pcb->api_error )  
	    ccb->sequence = QRY_CANCEL;
	else  if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) ) 
	{
	    /*
	    ** Need DESC message.  We either have already
	    ** received the message (in which case the call
	    ** below will recurse) or need to wait for the
	    ** message to arrive (in which case the call
	    ** below will do nothing).
	    */
	    ccb->qry.flags |= QRY_NEED_DESC;
	    jdbc_del_pcb( pcb );
	    jdbc_msg_pause( ccb, FALSE );
	    return;
	}
	else  if ( ccb->qry.svc_parms.max_cols < 1 )
	    ccb->sequence = QRY_GETD;	/* No parameters */
	else
	{
	    /*
	    ** No query parameters, only API service parameters.
	    */
	    ccb->qry.api_parms = &ccb->qry.svc_parms;
	    ccb->qry.flags |= QRY_HAVE_DESC;
	}
   	break;

    case QRY_SETD :
	if ( ! (ccb->qry.flags & QRY_HAVE_DESC) )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC query: no descriptor\n", 
			    ccb->id );
	    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	    jdbc_del_pcb( pcb );
	    return;
	}

	if ( ccb->qry.qry_parms.max_cols )
	{
	    if ( ! jdbc_alloc_qdata( &ccb->qry.qry_parms, 1 ) )
	    {
		jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		jdbc_del_pcb( pcb );
		return;
	    }

	    if ( ccb->qry.svc_parms.max_cols )
	    {
		if ( ! jdbc_build_qdata( &ccb->qry.svc_parms, 
					 &ccb->qry.qry_parms,
					 &ccb->qry.all_parms ) )
		{
		    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		    jdbc_del_pcb( pcb );
		    return;
		}

		ccb->qry.api_parms = &ccb->qry.all_parms;
	    }
	}

	ccb->qry.flags &= ~(QRY_NEED_DESC | QRY_HAVE_DESC);
	pcb->data.desc.count = ccb->qry.api_parms->max_cols;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_setDesc( pcb );
	return;

    case QRY_PDATA :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CANCEL;
	    break;
	}
	
	if ( ccb->qry.qry_parms.max_cols )
	{
	    /*
	    ** Need DATA message.  We either have already
	    ** received the message (in which case the call
	    ** below will recurse) or need to wait for the
	    ** message to arrive (in which case the call
	    ** below will do nothing).
	    */
	    ccb->qry.flags |= QRY_NEED_DATA;
	    jdbc_del_pcb( pcb );
	    jdbc_msg_pause( ccb, FALSE );
	    return;
	}

	ccb->qry.flags |= QRY_HAVE_DATA;
	break;

    case QRY_PUTP :
	if ( ! (ccb->qry.flags & QRY_HAVE_DATA) )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC query: no data\n", ccb->id );
	    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	    jdbc_del_pcb( pcb );
	    return;
	}

	ccb->qry.flags &= ~(QRY_NEED_DATA | QRY_HAVE_DATA);

	/*
	** If API service parameters and query parametres
	** have been combined, determine what portion of the
	** combined parameters are available for processing.
	** If there is no combination, qry.all_parms points
	** at either the API service parameters or the query
	** parameters, both of which are ready for processing.
	*/
	if ( ccb->qry.api_parms == &ccb->qry.all_parms )
	    calc_params( &ccb->qry.svc_parms, &ccb->qry.qry_parms, 
			 &ccb->qry.all_parms );

	/*
	** It is possible for the query parameters to include
	** a BLOB which is not a part of the combined set.
	** During the processing of the BLOB, there would
	** be no parameters to send to the server, so we
	** need to loop receiving (and discarding) the BLOB
	** from the client until a processable parameter is
	** received (normal processing) or end-of-data. The 
	** next action handles any branching/looping required.
	*/
	if ( ! ccb->qry.api_parms->col_cnt )  break;

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = &((IIAPI_DATAVALUE *)ccb->qry.api_parms->data)
					    [ ccb->qry.api_parms->cur_col ];
	pcb->data.data.more_segments = ccb->qry.api_parms->more_segments;
	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_putParm( pcb );
	return;

    case QRY_GETD :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CANCEL;
	    break;
	}

	/*
	** Consume the available parameters.  Don't consume
	** the last parameter during BLOB processing.
	*/
	ccb->qry.api_parms->cur_col += ccb->qry.api_parms->col_cnt -
				  (ccb->qry.api_parms->more_segments ? 1 : 0);
	ccb->qry.api_parms->col_cnt = 0;

	if ( ccb->qry.qry_parms.cur_col < ccb->qry.qry_parms.max_cols )
	{
	    /*
	    ** Get more query parameter data.
	    */
	    ccb->qry.flags |= QRY_NEED_DATA;
	    ccb->sequence = QRY_PUTP;
	    jdbc_del_pcb( pcb );

	    if ( ccb->msg.msg_len )
		jdbc_msg_data( ccb );		/* Finish current message */
	    else
		jdbc_msg_pause( ccb, FALSE );	/* Check for more messages */
	    return;
	}

	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_getDesc( pcb );
	return;

    case QRY_GETQI :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CANCEL;
	    break;
	}

	/*
	** If there is no result set, then the
	** current statement simply needs to be
	** closed.  
	**
	** Query info required for non-select 
	** statements and cursors but not for 
	** tuple streams since the call would 
	** close the stream.
	*/
	if ( ! pcb->data.desc.count )  
	    ccb->sequence = QRY_CLOSE;
	else  if ( ccb->qry.flags & QRY_TUPLES )  
	    break;

	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_gqi( pcb );
	return;

    case QRY_DONE :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CLOSE;
	    break;
	}

	/*
	** Tuple streams and read-only cursors can return an
	** entire buffer of tuples per fetch request.  Calc
	** the number of tuples which fit in a message buffer.
	*/
	if ( ccb->qry.flags & (QRY_TUPLES | QRY_CURSOR) )
	    calc_pre_fetch( pcb, pcb->data.desc.count, 
			    pcb->data.desc.desc, ccb->max_data_len );

	/*
	** Return the tuple descriptor.
	*/
	send_desc( ccb, &pcb->rcb, pcb->data.desc.count, pcb->data.desc.desc );

	/*
	** Cursor and tuple stream statements are kept
	** open for future reference.  Close statements
	** which are not to be kept open.
	*/
	if ( ! (ccb->qry.flags & (QRY_CURSOR | QRY_TUPLES)) )
	{
	    ccb->sequence = QRY_CLOSE;
	    break;
	}

	/*
	** Save the current statement for future reference.
	*/
	if ( ! (scb = jdbc_new_stmt( ccb, pcb, FALSE )) )
	{
	    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	    jdbc_del_pcb( pcb );
	    return;
	}

	/*
	** For tuple streams the data stream is still active
	** on the connection, so make sure we keep the statement
	** active locally.  Cursors are dormant after being opened
	** and in between fetches.
	*/
	if ( ccb->qry.flags & QRY_TUPLES )  ccb->api.stmt = scb->handle;
	ccb->sequence = QRY_EXIT;
	break;

    case QRY_CANCEL :		/* Cancel the current query */
	jdbc_flush_msg( ccb );
	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_cancel( pcb );
	return;

    case QRY_CLOSE :		/* Close current query */
	jdbc_push_callback( pcb, msg_query_sm );
	jdbc_api_close( pcb, NULL );	
	return;

    case QRY_XACT :		/* Check transaction state */
	if( ccb->cib->conn_abort ) break;

	if ( ccb->cib->xact_abort )
	{
	    jdbc_push_callback( pcb, msg_query_sm );
	    jdbc_xact_abort( pcb );
	    return;
	}

	if ( ccb->qry.flags & QRY_CURSOR )
	{
	    jdbc_push_callback( pcb, msg_query_sm );
	    jdbc_xact_check( pcb, JDBC_XQOP_CURSOR_CLOSE );
	    return;
	}
	break;

    case QRY_EXIT :		/* Query completed. */
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	if ( ccb->cib->conn_abort )
	    jdbc_gcc_abort( ccb, FAIL );
	else
	    jdbc_msg_pause( ccb, TRUE );
	return;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid query processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	jdbc_del_pcb( pcb );
	jdbc_gcc_abort( ccb, E_JD0109_INTERNAL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: jdbc_msg_desc
**
** Description:
**	Reads a DESC message and calls into the query sequencer
**	to continue query processing.
**
** Input:
**	ccb
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Connection info moved for connection pooling.
**	28-Jul-00 (gordy)
**	    Service parameters separated from query parameters.
**	    Save column names for database procedure parameters.
**	    Nullable field in DESC message converted to flags to
**	    indicate procedure parameter types.
**	11-Oct-00 (gordy)
**	    No longer need to set callback when calling directly
**	    into msg_query_sm.
**	10-May-01 (gordy)
**	    Client may now specify DBMS datatype so that UCS2 and
**	    ASCII data may be distinguished.  Provide default if
**	    DBMS datatype not provided by client.
*/

void
jdbc_msg_desc( JDBC_CCB *ccb )
{
    IIAPI_DESCRIPTOR	*desc;
    JDBC_PCB		*pcb;
    STATUS		status;
    u_i2		count, param;
    i2			def_type;

    if ( ! (ccb->qry.flags & QRY_NEED_DESC)  ||
	 ! jdbc_get_i2( ccb, (u_i1 *)&count ) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    if ( ccb->qry.flags & QRY_NEED_DESC )
		TRdisplay( "%4d    JDBC no descriptor info\n", ccb->id );
	    else
		TRdisplay( "%4d    JDBC unexpected descriptor\n", ccb->id );

	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC parameter descriptor: %d params\n", 
		   ccb->id, count );

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    if ( ! jdbc_alloc_qdesc( &ccb->qry.qry_parms, count, NULL ) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	jdbc_del_pcb( pcb );
	return;
    }

    /*
    ** Until positional procedure parameters are supported,
    ** mark default parameters as TUPLE parameters so that
    ** they may be removed from the API parameter set but
    ** still processed by the messaging system.
    */
    desc = (IIAPI_DESCRIPTOR *)ccb->qry.qry_parms.desc;
    def_type = (ccb->qry.flags & QRY_PROCEDURE) ? IIAPI_COL_TUPLE 
						: IIAPI_COL_QPARM;

    for( 
	 param = 0, status = OK; 
	 param < count  &&  status == OK; 
	 param++ 
       )
    {
	i2	sqlType;
	u_i2	dfltType, dbmsType, length, len;
	u_i1	prec, scale, flags;
	char	*name = NULL;

	if ( ! jdbc_get_i2( ccb, (u_i1 *)&sqlType ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&dbmsType ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&length ) )  break;
	if ( ! jdbc_get_i1( ccb, &prec ) )  break;
	if ( ! jdbc_get_i1( ccb, &scale ) )  break;
	if ( ! jdbc_get_i1( ccb, &flags ) )  break;

	if ( ! jdbc_get_i2( ccb, (u_i1 *)&len ) )  break;
	if ( len )
	{
	    if ( ! (name = MEreqmem( 0, len + 1, FALSE, NULL )) )  break;
	    if ( ! jdbc_get_bytes( ccb, len, (u_i1 *)name ) )  break;
	    name[ len ] = EOS;
	}

	switch( sqlType )
	{
	case SQL_TINYINT :
	    dfltType = IIAPI_INT_TYPE;
	    length = 1;
	    prec = scale = 0;
	    break;

	case SQL_SMALLINT :
	    dfltType = IIAPI_INT_TYPE;
	    length = 2;
	    prec = scale = 0;
	    break;

	case SQL_INTEGER :
	    dfltType = IIAPI_INT_TYPE;
	    length = 4;
	    prec = scale = 0;
	    break;

	case SQL_REAL :
	    dfltType = IIAPI_FLT_TYPE;
	    length = 4;
	    prec = scale = 0;
	    break;

	case SQL_FLOAT :
	case SQL_DOUBLE :
	    dfltType = IIAPI_FLT_TYPE;
	    length = 8;
	    prec = scale = 0;
	    break;

	case SQL_DECIMAL :
	case SQL_NUMERIC :
	    if ( ccb->cib->level >= IIAPI_LEVEL_1 )
	    {
		dfltType = IIAPI_DEC_TYPE;
		length = (prec / 2) + 1;
	    }
	    else
	    {
		/*
		** Since decimal is not supported by the
		** server, leave the parameter in the form
		** passed used by the message protocol.
		*/
		dbmsType = IIAPI_VCH_TYPE;
		length = 36;
		prec = scale = 0;
	    }
	    break;

	case SQL_CHAR :
	    dfltType = IIAPI_CHA_TYPE;
	    prec = scale = 0;
	    break;

	case SQL_VARCHAR :
	    dfltType = IIAPI_VCH_TYPE;
	    length += 2;
	    prec = scale = 0;
	    break;

	case SQL_LONGVARCHAR :
	    dfltType = IIAPI_LVCH_TYPE;
	    length = ccb->max_data_len;	/* Segments may not span buffers */
	    prec = scale = 0;
	    break;

	case SQL_BINARY :
	    if ( ccb->cib->level >= IIAPI_LEVEL_1 )
		dfltType = IIAPI_BYTE_TYPE;
	    else
		dbmsType = IIAPI_CHA_TYPE;

	    prec = scale = 0;
	    break;

	case SQL_VARBINARY :
	    if ( ccb->cib->level >= IIAPI_LEVEL_1 )
		dfltType = IIAPI_VBYTE_TYPE;
	    else
		dbmsType = IIAPI_VCH_TYPE;

	    length += 2;
	    prec = scale = 0;
	    break;

	case SQL_LONGVARBINARY :
	    dfltType = IIAPI_LBYTE_TYPE;
	    length = ccb->max_data_len;	/* Segments may not span buffers */
	    prec = scale = 0;
	    break;

	case SQL_TIMESTAMP :
	    dfltType = IIAPI_DTE_TYPE;
	    length = 12;
	    prec = scale = 0;
	    break;

	case SQL_NULL :
	    dfltType = IIAPI_LTXT_TYPE;
	    length = 2;
	    prec = scale = 0;
	    break;

	case SQL_BIT :
	case SQL_BIGINT :
	case SQL_DATE :
	case SQL_TIME :
	case SQL_OTHER :
	default :
	    status = E_JD0112_UNSUPP_SQL_TYPE;
	    break;
	}

	desc[ param ].ds_dataType = (dbmsType != 0) ? dbmsType : dfltType;
	desc[ param ].ds_nullable = (flags & JDBC_DSC_NULL) ? TRUE : FALSE;
	desc[ param ].ds_length = length;
	desc[ param ].ds_precision = prec;
	desc[ param ].ds_scale = scale;
	if ( desc[ param ].ds_columnName )  MEfree(desc[param].ds_columnName);
	desc[ param ].ds_columnName = name;
	desc[ param ].ds_columnType = 
	    (flags & JDBC_DSC_POUT) ? IIAPI_COL_PROCBYREFPARM :
	    (flags & JDBC_DSC_GTT) ? IIAPI_COL_PROCGTTPARM :
	    ((flags & JDBC_DSC_PIN) ? IIAPI_COL_PROCPARM : def_type);
    }

    if ( param < count  ||  ccb->msg.msg_len )
    {
	if ( status == OK )  status = E_JD010A_PROTOCOL_ERROR;
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid DESC message format (%d,%d,%d)\n", 
			ccb->id, param, count, ccb->msg.msg_len );
	jdbc_gcc_abort( ccb, status );
	jdbc_del_pcb( pcb );
    }

    ccb->qry.flags |= QRY_HAVE_DESC;
    msg_query_sm( (PTR)pcb );
    return;
}


/*
** Name: jdbc_msg_data
**
** Description:
**	Reads a DATA message and calls into the query sequencer
**	to continue query processing.
**
** Input:
**	ccb
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Rename for external reference.
**	28-Jul-00 (gordy)
**	    Service parameters separated from query parameters.
**	11-Oct-00 (gordy)
**	    No longer need to set callback when calling directly
**	    into msg_query_sm.
**	10-May-01 (gordy)
**	    Added support for UCS2 data.
**	 1-Jun-01 (gordy)
**	    Improper use of done and more_segments flags resulted in
**	    invalid PROTOCOL_ERROR messages when the end-of-segments
**	    indicator immediatly followed a BLOB segment in the same
**	    message.
**	 2-Jul-01 (gordy)
**	    Calculate character and byte length of BLOB segments.
*/

void
jdbc_msg_data( JDBC_CCB *ccb )
{
    JDBC_PCB		*pcb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    u_i2		param;
    bool		done, more_data;

    if ( ! (ccb->qry.flags & QRY_NEED_DATA) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC unexpected data message\n", ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC parameter data: current %d, total %d\n", 
		   ccb->id, ccb->qry.qry_parms.cur_col, 
		   ccb->qry.qry_parms.max_cols );

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    desc = (IIAPI_DESCRIPTOR *)ccb->qry.qry_parms.desc;
    data = (IIAPI_DATAVALUE *)ccb->qry.qry_parms.data;

    for( 
	 param = ccb->qry.qry_parms.cur_col; 
	 param < ccb->qry.qry_parms.max_cols; 
	 param++
       )
    {
	bool	error = FALSE;
	STATUS	status = E_JD010A_PROTOCOL_ERROR;
	u_i1	indicator;
	u_i2	length;
	char	dbuff[ 34 ];	/* Precision 31+sign+decimal+terminator */

	/*
	** If not processing a BLOB, then begin new parameter.
	*/
	if ( ! ccb->qry.qry_parms.more_segments )
	{
	    /*
	    ** The first byte indicates if data is present.
	    ** If no data, the parameter is NULL.  Except
	    ** for BLOBs, the data indicator must be in the
	    ** same message (possibly split) as the data.
	    */
	    if ( ! jdbc_get_i1( ccb, &indicator ) )  break;
	    if ( ! indicator )
	    {
		data[ param ].dv_length = 0;
		data[ param ].dv_null = TRUE;
		continue;
	    }
	}

	data[ param ].dv_length = desc[ param ].ds_length;
	data[ param ].dv_null = FALSE;

	switch( desc[ param ].ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( desc[ param ].ds_length )
	    {
	    case 1 :	if ( ! jdbc_get_i1( ccb, data[ param ].dv_value ) )  
			    error = TRUE; 
			break;

	    case 2 :	if ( ! jdbc_get_i2(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE; 
			break;

	    case 4 :	if ( ! jdbc_get_i4(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE; 
			break;

	    default :	error = TRUE;	break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( desc[ param ].ds_length )
	    {
	    case 4 :	if ( ! jdbc_get_f4(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE;
			break;

	    case 8 :	if ( ! jdbc_get_f8(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE;
			break;

	    default :	error = TRUE;	break;
	    }
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_BYTE_TYPE :
	    if ( ! jdbc_get_bytes( ccb, desc[ param ].ds_length,
				   (u_i1 *)data[ param ].dv_value ) )  
		error = TRUE; 
	    break;

	case IIAPI_NCHA_TYPE :
	    if ( ! jdbc_get_ucs2( ccb, desc[ param ].ds_length / sizeof(UCS2),
				  (u_i1 *)data[ param ].dv_value ) )
		error = TRUE;
	    break;

	case IIAPI_VCH_TYPE :
	case IIAPI_VBYTE_TYPE :
	    if ( 
		 ! jdbc_get_i2( ccb, (u_i1 *)&length )  ||
	         (length + 2) > desc[ param ].ds_length  ||
	         ! jdbc_get_bytes( ccb, length,
				   (u_i1 *)data[ param ].dv_value + 2 )
	       )  
		error = TRUE; 
	    else
	    {
		MEcopy( (PTR)&length, 2, data[ param ].dv_value );
		data[ param ].dv_length = length + 2;
	    }
	    break;

	case IIAPI_NVCH_TYPE :
	    if ( 
		 ! jdbc_get_i2( ccb, (u_i1 *)&length )  ||
	         (length * sizeof(UCS2) + 2) > desc[ param ].ds_length  ||
		 ! jdbc_get_ucs2( ccb, length,
				  (u_i1 *)data[ param ].dv_value + 2 )
	       )
		error = TRUE;
	    else
	    {
		MEcopy( (PTR)&length, 2, data[ param ].dv_value );
		data[ param ].dv_length = length * sizeof( UCS2 ) + 2;
	    }
	    break;

	case IIAPI_LVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LBYTE_TYPE :
	    ccb->qry.qry_parms.more_segments = TRUE;

	    if ( ! jdbc_get_i2( ccb, (u_i1 *)&length ) )
	    {
		/*
		** This should only happen if the data/NULL
		** indicator is not followed directly by a
		** segment.  BLOBs are the only datatypes
		** for which this may happen.  Create a zero
		** length segment.
		*/
		length = 0;
		MEcopy( (PTR)&length, 2, data[ param ].dv_value );
		data[ param ].dv_length = length + 2;
	    }
	    else  if ( ! length )
	    {
		/*
		** A zero length segment marks the end of the BLOB.
		*/
		if ( JDBC_global.trace_level >= 5 )
		    TRdisplay( "%4d    JDBC recv end-of-segments\n", ccb->id );
		MEcopy( (PTR)&length, 2, data[ param ].dv_value );
		data[ param ].dv_length = length + 2;
		ccb->qry.qry_parms.more_segments = FALSE;
	    }
	    else  
	    {
		u_i2	chrs, char_size = sizeof( char );
		bool	ucs2 = FALSE;

		if ( desc[ param ].ds_dataType == IIAPI_LNVCH_TYPE )
		{
		    char_size = sizeof( UCS2 );
		    ucs2 = TRUE;
		}

		chrs = length;
		length = chrs * char_size;

		if ( (length + 2) > desc[ param ].ds_length )
		    error = TRUE; 
		else  if ( ! ucs2 )
		{
		    if ( ! jdbc_get_bytes( ccb, length,
					   (u_i1 *)data[param].dv_value + 2 ) )
			error = TRUE; 
		}
		else
		{
		    if ( ! jdbc_get_ucs2( ccb, chrs,
					  (u_i1 *)data[param].dv_value + 2 ) )
			error = TRUE;
		}

		if ( ! error )
		{
		    if ( JDBC_global.trace_level >= 5 )
			TRdisplay( "%4d    JDBC recv segment: %d\n", 
				    ccb->id, length );

		    MEcopy( (PTR)&chrs, 2, data[ param ].dv_value );
		    data[ param ].dv_length = length + 2;

		    /*
		    ** The segment should be the last item in the
		    ** message.  The only exception is the last
		    ** segment which may be followed by the end-
		    ** of-segments indicator and possibly more
		    ** parameters.  The end-of-segments indicator
		    ** is a zero length segment.
		    */
		    if ( jdbc_get_i2( ccb, (u_i1 *)&length ) )
		    {
			/*
			** Additional segment present, must
			** be zero length (end-of-segments).
			*/
			if ( length )  
			    error = TRUE;
			else  
			{
			    if ( JDBC_global.trace_level >= 5 )
				TRdisplay( "%4d    JDBC recv end-of-segments\n",
					    ccb->id );

			    ccb->qry.qry_parms.more_segments = FALSE;
			}
		    }
		}
	    }
	    break;

	case IIAPI_DEC_TYPE :
	    if ( 
		 ! jdbc_get_i2( ccb, (u_i1 *)&length )  ||
	         length >= sizeof( dbuff )  ||
	         ! jdbc_get_bytes( ccb, length, (u_i1 *)dbuff )
	       )  
		error = TRUE; 
	    else
	    {
		IIAPI_DESCRIPTOR sdesc;
		IIAPI_DATAVALUE	 sdata;

		sdesc.ds_dataType = IIAPI_CHA_TYPE;
		sdesc.ds_nullable = FALSE;
		sdesc.ds_length = length;
		sdesc.ds_precision = 0;
		sdesc.ds_scale = 0;
		sdata.dv_null = FALSE;
		sdata.dv_length = length;
		sdata.dv_value = dbuff;
		dbuff[ length ] = EOS;

		status = jdbc_api_format( ccb, &sdesc, &sdata,
					  &desc[ param ], &data[ param ] );
		if ( status != OK )  error = TRUE;
	    }
	    break;

	case IIAPI_DTE_TYPE :
	    if ( 
		 ! jdbc_get_i2( ccb, (u_i1 *)&length )  ||
	         length >= sizeof( dbuff )  ||
	         ! jdbc_get_bytes( ccb, length, (u_i1 *)dbuff )
	       )  
		error = TRUE; 
	    else
	    {
		IIAPI_DESCRIPTOR sdesc;
		IIAPI_DATAVALUE	 sdata;

		sdesc.ds_dataType = IIAPI_CHA_TYPE;
		sdesc.ds_nullable = FALSE;
		sdesc.ds_length = length;
		sdesc.ds_precision = 0;
		sdesc.ds_scale = 0;
		sdata.dv_null = FALSE;
		sdata.dv_length = length;
		sdata.dv_value = dbuff;
		dbuff[ length ] = EOS;

		status = jdbc_api_format( ccb, &sdesc, &sdata,
					  &desc[ param ], &data[ param ] );
		if ( status != OK )  error = TRUE;
	    }
	    break;

	default :
	    /*
	    ** Shouldn't happen since we assigned
	    ** the types in msg_desc().
	    */
	    status = E_JD0112_UNSUPP_SQL_TYPE;
	    error = TRUE;
	    break;
	}

	if ( error )  
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC error processing data (%d): 0x%x\n", 
			   ccb->id, param, status );
	    jdbc_gcc_abort( ccb, status );
	    jdbc_del_pcb( pcb );
	    ccb->qry.qry_parms.more_segments = FALSE;
	    return;
    }

	/*
	** BLOB handling requires individual segments
	** to be passed on to the server prior to
	** processing subsequent segments or columns.
	*/
    if ( desc[ param ].ds_dataType == IIAPI_LVCH_TYPE ||
	     desc[ param ].ds_dataType == IIAPI_LBYTE_TYPE ||
	     desc[ param ].ds_dataType == IIAPI_LNVCH_TYPE  )
    {
	    /*
	    ** Must bump parameter count since we are
	    ** skipping this action in the looping code.
	    */
	    param++;
	    break;
    }
    }

    ccb->qry.qry_parms.col_cnt = param - ccb->qry.qry_parms.cur_col;

    /*
    ** Sanity check:
    **	1) At least one column should have been processed.
    **	2) EOG if and only if last column completed.
    **  3) Message empty when last column completed.
    */
    done = ( param >= ccb->qry.qry_parms.max_cols  &&  
	     ! ccb->qry.qry_parms.more_segments );

    more_data = ccb->msg.msg_len || ! ( ccb->msg.msg_flags & JDBC_MSGFLG_EOG );

    if ( ccb->qry.qry_parms.col_cnt <= 0 || (!more_data && !done)||(done && more_data))
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay("%4d    JDBC invalid parameter set: %d,%d,%d%s (%d%s)\n", 
		      ccb->id, ccb->qry.qry_parms.cur_col, 
		      ccb->qry.qry_parms.col_cnt, ccb->qry.qry_parms.max_cols, 
		      ccb->qry.qry_parms.more_segments ? " <more>" : "",
		      ccb->msg.msg_len,
		      ccb->msg.msg_flags & JDBC_MSGFLG_EOG ? " EOG" : "" );

	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	jdbc_del_pcb( pcb );
	return;
    }

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC processed parameters: %d to %d %s\n", 
		   ccb->id, ccb->qry.qry_parms.cur_col, 
		   ccb->qry.qry_parms.cur_col + ccb->qry.qry_parms.col_cnt - 1,
		   ccb->qry.qry_parms.more_segments ? "(more segments)" : "" );

    ccb->qry.flags |= QRY_HAVE_DATA;
    msg_query_sm( (PTR)pcb );
    return;
}


/*
** Name: send_desc
**
** Description:
**	Format and send a DESCriptor message.  The message
**	is appended to an existing request control block
**	if provided.  Otherwise, a new request control block
**	is created and returned with the message.
**
** Input:
**	ccb	Connection control block.
**	rcb_ptr	Request control block.
**	pcb	Parameter control block.
**
** Ouput:
**	rcb	Request control block.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	28-Jul-00 (gordy)
**	    Nullable byte in DESC converted to flags.
**	10-May-01 (gordy)
**	    Added support for UCS2 data.
*/

static void
send_desc( JDBC_CCB *ccb, JDBC_RCB **rcb_ptr, 
	   u_i2 count, IIAPI_DESCRIPTOR *desc )
{
    u_i2 i;

    jdbc_msg_begin( rcb_ptr, JDBC_MSG_DESC, 0 );
    jdbc_put_i2( rcb_ptr, count );

    for( i = 0; i < count; i++ )
    {
	i2	type;
	u_i2 	len;
	u_i1 	null, prec, scale;

	type = SQL_OTHER;
	null = desc[i].ds_nullable ? JDBC_DSC_NULL : 0;
	len = desc[i].ds_length;
	prec = scale = 0;

	switch( desc[i].ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( len )
	    {
	    case 1 : type = SQL_TINYINT;	break;
	    case 2 : type = SQL_SMALLINT;	break;
	    case 4 : type = SQL_INTEGER;	break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( len )
	    {
	    case 4 : type = SQL_REAL;	prec = 7;	break;
	    case 8 : type = SQL_DOUBLE;	prec = 15;	break;
	    }
	    break;

	case IIAPI_DEC_TYPE :
	    type = SQL_DECIMAL;
	    prec = desc[i].ds_precision;
	    scale = desc[i].ds_scale;
	    len = 0;
	    break;

	case IIAPI_MNY_TYPE :
	    type = SQL_DECIMAL; 
	    prec = 14; 
	    scale = 2;	
	    len = 0;
	    break;

	case IIAPI_DTE_TYPE :
	    type = SQL_TIMESTAMP;
	    len = 0;
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_NCHA_TYPE :
	case IIAPI_CHR_TYPE :
	    type = SQL_CHAR;
	    break;

	case IIAPI_VCH_TYPE :
	case IIAPI_NVCH_TYPE :
	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	    type = SQL_VARCHAR;
	    len -= 2;
	    break;

	case IIAPI_LVCH_TYPE :	
	case IIAPI_LNVCH_TYPE :
	    type = SQL_LONGVARCHAR; 
	    len = 0; 
	    break;

	case IIAPI_BYTE_TYPE :	type = SQL_BINARY;		break;
	case IIAPI_VBYTE_TYPE :	type = SQL_VARBINARY; len -= 2;	break;
	case IIAPI_LBYTE_TYPE :	
	    type = SQL_LONGVARBINARY; 
	    len = 0; 
	    break;

	case IIAPI_HNDL_TYPE :	/* Shouldn't happen */
	case IIAPI_LOGKEY_TYPE :
	case IIAPI_TABKEY_TYPE :
	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC invalid datatype: %d\n", 
			   ccb->id, desc[i].ds_dataType );
	    jdbc_gcc_abort( ccb, E_JD0112_UNSUPP_SQL_TYPE );
	    return;
	}

	if ( type == SQL_OTHER )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC invalid data length: %d (%d)\n", 
			   ccb->id, len, desc[i].ds_dataType );
	    jdbc_gcc_abort( ccb, E_JD0112_UNSUPP_SQL_TYPE );
	    return;
	}

	jdbc_put_i2( rcb_ptr, type );
	jdbc_put_i2( rcb_ptr, desc[i].ds_dataType );
	jdbc_put_i2( rcb_ptr, len );
	jdbc_put_i1( rcb_ptr, prec );
	jdbc_put_i1( rcb_ptr, scale );
	jdbc_put_i1( rcb_ptr, null );

	if ( ! desc[i].ds_columnName  ||  
	     ! (len = STlength( desc[i].ds_columnName )) )
	    jdbc_put_i2( rcb_ptr, 0 );
	else
	{
	    jdbc_put_i2( rcb_ptr, len );
	    jdbc_put_bytes( rcb_ptr, len, (u_i1 *)desc[i].ds_columnName );
	}
    }

    jdbc_msg_end( *rcb_ptr, FALSE );
    return;
}


/*
** Name: jdbc_expand_qtxt
**
** Description:
**	Expand query buffer if needed.  Current query text is saved.
**
** Input:
**	ccb
**	 qry
**	  qtxt		Query text buffer.
**	length		Length required.
**	save		Save current buffer contents.
**
** Output:
**	ccb
**	 qry
**	  qtxt_max	Length of query text buffer.
**	  qtxt		Query text buffer.
**
** Returns:
**	bool		TRUE if successful, FALSE if memory allocation error.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 7-Mar-01 (gordy)
**	    Renamed and made global.
*/

bool
jdbc_expand_qtxt( JDBC_CCB *ccb, u_i2 length, bool save )
{
    char *qtxt = ccb->qry.qtxt;

    if ( length <= ccb->qry.qtxt_max )  return( TRUE );
    ccb->qry.qtxt_max = length;

    if ( ! (ccb->qry.qtxt = (char *)MEreqmem( 0, length, FALSE, NULL )) )
    {
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC couldn't allocate query text: %d bytes\n",
		       ccb->id, length );

	if ( qtxt )  MEfree( (PTR)qtxt );
	ccb->qry.qtxt = NULL;
	ccb->qry.qtxt_max = 0;
	return( FALSE );
    }

    if ( qtxt )  
    {
	if ( save )  STcopy( qtxt, ccb->qry.qtxt );
	MEfree( (PTR)qtxt );
    }

    return( TRUE );
}


/*
** Name: calc_pre_fetch
**
** Description:
**
** Input:
**	pcb		Parameter control block.
**	count		Number of descriptors.
**	desc		Descriptors.
**	max_data_len	Space available for tuples.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-Dec-99 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Pre-fetch limit is now only sent if greater than one.
**	    Limit does not need to be turned off.
**	10-May-01 (gordy)
**	    Added support for UCS2 data.
*/

static void
calc_pre_fetch
( 
    JDBC_PCB		*pcb, 
    u_i2		count, 
    IIAPI_DESCRIPTOR	*desc, 
    u_i2		max_data_len 
)
{
    u_i4		len = 0;
    u_i2		col;

    for( col = 0; col < count; col++ )
    {
	len++;	/* Data/NULL indicator always present */

	switch( desc[ col ].ds_dataType )
	{
	case IIAPI_INT_TYPE :	
	    switch( desc[ col ].ds_length )
	    {
	    case 1 :	len += CV_N_I1_SZ;	break;
	    case 2 :	len += CV_N_I2_SZ;	break;
	    case 4 :	len += CV_N_I4_SZ;	break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( desc[ col ].ds_length )
	    {
	    case 4 :	len += CV_N_F4_SZ;	break;
	    case 8 :	len += CV_N_F8_SZ;	break;
	    }
	    break;

	case IIAPI_DEC_TYPE :
	    len += desc[ col ].ds_precision + 3;  /* precision, sign, len */
	    if ( desc[ col ].ds_scale )  len++;	  /* decimal point */
	    break;

	case IIAPI_MNY_TYPE :
	    len += 18;				/* prec,sign,dec,len */
	    break;

	case IIAPI_DTE_TYPE :
	    len += 21;				/* YYYY-MM-DD hh:mm:ss */
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_NCHA_TYPE :
	case IIAPI_CHR_TYPE :
	case IIAPI_VCH_TYPE :
	case IIAPI_NVCH_TYPE :
	case IIAPI_BYTE_TYPE :
	case IIAPI_VBYTE_TYPE :
	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	    len += desc[ col ].ds_length;
	    break;

	case IIAPI_LVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LBYTE_TYPE :
	default :
	    /*
	    ** Disable pre-fetch.
	    */
	    len = -1;
	    col = count;
	    break;
	}
    }

    /*
    ** Disable pre-fetch for unknown tuple lengths
    ** or calculate how many tuples can fit.  The
    ** limit is only sent if greater than one.
    */
    if ( len > 0 )
    {
	pcb->result.fetch_limit = max( 1, max_data_len / len );

	if ( pcb->result.fetch_limit > 1 )
	    pcb->result.flags |= PCB_RSLT_FETCH_LIMIT;
    }

    return;
}


/*
** Name: calc_params
**
** Description:
**	Determine what subset of parameters are available
**	for processing given two sets of parameters (API
**	service parameters and query parameters) which have
**	been combined into a single parameter set for
**	processing.
**
** Input;
**	svc	API service parameters.
**	qry	Query parameters.
**
** Output:
**	all	Combined parameters.
**
** Returns:
**	void
**
** History:
**	28-Jul-00 (gordy)
**	    Created.
*/

static void
calc_params( QDATA *svc, QDATA *qry, QDATA *all )
{
    IIAPI_DESCRIPTOR	*desc;
    bool		qry_cols = (qry->col_cnt > 0);

    all->col_cnt = 0;
    all->more_segments = FALSE;

    /*
    ** If there are any unprocessed API service parameters
    ** (should be only on first pass) include them in sub-
    ** set as they are pre-loaded and are first in the
    ** combined set of parameters.  API service parameters
    ** do not include BLOBs.
    */
    for( ; svc->col_cnt > 0; svc->col_cnt-- )
    {
	STRUCT_ASSIGN_MACRO( ((IIAPI_DATAVALUE *)svc->data)[ svc->cur_col ], 
	     ((IIAPI_DATAVALUE *)all->data)[ all->cur_col + all->col_cnt ] );
	all->col_cnt++;
	svc->cur_col++;
    }

    /*
    ** Now include the sub-set of query parameters which 
    ** are available.
    */
    for( ; qry->col_cnt > 0; qry->col_cnt-- )
    {
	desc = &((IIAPI_DESCRIPTOR *)qry->desc)[ qry->cur_col ];

	/*
	** The more_segments flag should only be set
	** TRUE if the query parameter segment flag
	** is true for the last column available.
	** Rather than test for the last column, we
	** just set the flag for each column (FALSE
	** for skipped columns).
	*/
	if ( desc->ds_columnType == IIAPI_COL_TUPLE )
	    all->more_segments = FALSE;
	else
	{
	    STRUCT_ASSIGN_MACRO( ((IIAPI_DATAVALUE *)qry->data)[qry->cur_col], 
		 ((IIAPI_DATAVALUE *)all->data)[all->cur_col + all->col_cnt] );
	    all->col_cnt++;
	    all->more_segments = qry->more_segments;
	}

	qry->cur_col++;
    }

    /*
    ** If the last column is an unfinished BLOB,
    ** do not include it as a completed column.
    */
    if ( qry_cols  &&  qry->more_segments )  qry->cur_col--;

    return;
}


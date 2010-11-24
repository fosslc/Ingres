/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdquery.c
**
** Description:
**	GCD query message processing.
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**      13-Jan-03 (weife01) Bug 109369
**          Fixed problem when inserting multiple short blob.    
**	15-Jan-03 (gordy)
**	    Added gcd_sess_abort().  Rename gcd_flush_msg() to gcd_msg_flush().
**	 6-Jun-03 (gordy)
**	    Allow room for DONE message along with tuple data. 
**	 4-Aug-03 (gordy)
**	    Cleaned up row pre-fetch calculations to more accurately
**	    reflect situations where pre-fetching is restricted.
**	20-Aug-03 (gordy)
**	    Query flags now optional for all queries.  Added AUTO_CLOSE flag.
**	 6-Oct-03 (gordy)
**	    Added ability to return rows with initial query request: new
**	    query flags, a separate send descriptor state, and transfer
**	    of control to the fetch module.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Check for errors in transaction processing.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time.
**	28-Jun-06 (gordy)
**	    Added support for OUT procedure parameters.
**	29-Jun-06 (lunbr01)  Bug 115970
**	    Skip calling API cancel or close on a failed request when
**	    the connection has been aborted.  This will prevent potential
**	    access violation or logic error due to cancel/close accessing 
**	    deleted API handles for the session.
**	30-Jun-06 (gordy)
**	    Added support for DESCRIBE INPUT.
**	31-Aug-06 (gordy)
**	    ANSI time precision carried as scale (fractional digits).
**	22-Sep-06 (gordy)
**	    Ingres stores time precision as scale in the standard
**	    catalogs.  While DAM also carries this value as scale,
**	    API still handles it as precision.
**	 6-Nov-06 (gordy)
**	    Support query text over 64K in length.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 4-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 9-Sep-08 (gordy)
**	    Expand temp buffer to support larger decimal precision.
**	23-Mar-09 (gordy)
**	    Support long database object names.
**	21-Jul-09 (gordy)
**	    Remove restrictions on length of cursor names.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.  Moved calc_params()
**	    to gcdutil.c and renamed gcd_combine_qdata().
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs or NBR type as BYTE.
*/	

#define	FOR_READONLY	" for readonly"

/*
** Query sequencing.
*/

static	GCULIST	qryMap[] =
{
    { MSG_QRY_EXQ,	"EXECUTE QUERY" },
    { MSG_QRY_OCQ,	"OPEN CURSOR FOR QUERY" },
    { MSG_QRY_PREP,	"PREPARE STATEMENT" },
    { MSG_QRY_EXS,	"EXECUTE STATEMENT" },
    { MSG_QRY_OCS,	"OPEN CURSOR FOR STATEMENT" },
    { MSG_QRY_EXP,	"EXECUTE PROCEDURE" },
    { MSG_QRY_CDEL,	"CURSOR DELETE" },
    { MSG_QRY_CUPD,	"CURSOR UPDATE" },
    { MSG_QRY_DESC,	"DESCRIBE STATEMENT" },
    { 0, NULL }
};

#define	QRY_BEGIN	0	/* Init query processing */
#define	QRY_QUERY	1	/* Send (s) query */
#define	QRY_PDESC	2	/* Receive (c) parameter descriptor */
#define	QRY_SETD	3	/* Send (s) parameter descriptor */
#define	QRY_PDATA	4	/* Receive (c) parameters */
#define	QRY_PUTP	5	/* Send (s) parameters */
#define	QRY_GETD	6	/* Receive (s) result descriptor */
#define	QRY_GETQI	7	/* Receive (s) query results */
#define	QRY_SDESC	8	/* Send (c) result descriptor */
#define	QRY_DONE	9	/* Finalize query processing */

#define	QRY_CANCEL	96	/* Cancel query */
#define	QRY_CLOSE	97	/* Close query */
#define	QRY_XACT	98	/* Check transaction state */
#define	QRY_EXIT	99	/* Respond to client */

static	GCULIST qrySeqMap[] =
{
    { QRY_BEGIN,	"BEGIN" },
    { QRY_QUERY,	"QUERY" },
    { QRY_PDESC,	"PDESC" },
    { QRY_SETD,		"SETD" },
    { QRY_PDATA,	"PDATA" },
    { QRY_PUTP,		"PUTP" },
    { QRY_GETD,		"GETD" },
    { QRY_GETQI,	"GETQI" },
    { QRY_SDESC,	"SDESC" },
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
    QP_STMT,		/* DESC */
};

static	u_i2	p_opt[] = 
{ 
    0, 			/* unused */
    QP_FLAG,		/* EXQ */
    QP_FLAG,		/* OCQ */
    QP_FLAG, 		/* PREP */
    QP_FLAG, 		/* EXS */
    QP_FLAG, 		/* OCS */
    QP_SCHM | QP_FLAG,	/* EXP */
    QP_FLAG, 		/* CDEL */
    QP_FLAG, 		/* CUPD */
    QP_FLAG,		/* DESC */
};

static	void	msg_query_sm( PTR );
static	void	send_desc( GCD_CCB *, GCD_RCB **, u_i2, IIAPI_DESCRIPTOR * );
static	i4	calc_tuple_length( GCD_CCB *, u_i2, IIAPI_DESCRIPTOR * );



/*
** Name: gcd_msg_query
**
** Description:
**	Process a GCD query message.
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
**	 4-Aug-03 (gordy)
**	    QRY_TUPLES flag now also set for cursors.
**	20-Aug-03 (gordy)
**	    Added query execution flag AUTO_CLOSE.
**	 6-Oct-03 (gordy)
**	    Added query fetch flags.
**	11-Oct-05 (gordy)
**	    Move transaction initialization to sequencer
**	28-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	30-Jun-06 (gordy)
**	    Added support for DESCRIBE INPUT.
**	 6-Nov-06 (gordy)
**	    Concatenate multiple query text parameters to support
**	    lengths greater than 64K.
**	15-Nov-06 (gordy)
**	    Added Locators flag.
**	 4-Apr-07 (gordy)
**	    Added scrolling cursor flag.
**	23-Mar-09 (gordy)
**	    Support for long database object names.  Use local buffer
**	    for more common short names.  Allocate buffer when length
**	    exceeds local buffer size.  Mutually exclusive request
**	    parameters now share buffers to reduce code duplication.
**	21-Jul-09 (gordy)
**	    Process cursor name as a dynamic length parameter.
**	    Cursor name in CCB is dynamically allocated.
**	25-Mar-10 (gordy)
**	    Init CCB fields shared with batch processing.
*/

void
gcd_msg_query( GCD_CCB *ccb )
{
    GCD_PCB		*pcb;
    GCD_SCB		*scb;
    GCD_RCB		*rcb = NULL;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    bool		incomplete = FALSE;
    STATUS		status;
    u_i1		t_ui1;
    u_i2		t_ui2, qtype, p_rec = 0;
    u_i4		qtxt_len, flags = 0;
    u_i2		p1_len, p2_len;
    char		p1_buff[ GCD_NAME_MAX + 1 ];
    char		*p1 = NULL;
    char		p2_buff[ GCD_NAME_MAX + 1 ];
    char		*p2 = NULL;

    /*
    ** Read and validate the query type.
    */
    if ( ! gcd_get_i2( ccb, (u_i1 *)&qtype ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no query op\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( qtype > DAM_ML_QRY_MAX )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid query op: %d\n", ccb->id, qtype );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Query operation: %s\n", 
		   ccb->id, gcu_lookup( qryMap, qtype ) );

    if ( ccb->msg.dtmc )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD Queries not allowed on DTMC\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( ccb->qry.qtxt )  ccb->qry.qtxt[0] = EOS;
    qtxt_len = 0;
    ccb->qry.flags = 0;

    /*
    ** Read the query parameters.
    */
    while( ccb->msg.msg_len )
    {
	DAM_ML_PM qp;

	incomplete = TRUE;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&qp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&qp.val_len ) )  break;

	switch( qp.param_id )
	{
	case MSG_QP_QTXT :
	    if ( ! gcd_expand_qtxt( ccb, qtxt_len + qp.val_len + 1, TRUE )  ||
	         ! gcd_get_bytes( ccb, qp.val_len, 
		 		  (u_i1 *)ccb->qry.qtxt + qtxt_len ) )
		break;

	    qtxt_len += qp.val_len;
	    ccb->qry.qtxt[ qtxt_len ] = EOS;
	    if ( qp.val_len > 0 )  p_rec |= QP_QTXT;
	    incomplete = FALSE;
	    break;
	
	case MSG_QP_STMT_NAME :
	case MSG_QP_PROC_NAME :
	    if ( p1 )  break;	/* Already set?  Should not happen */
	    p1_len = qp.val_len;

	    if ( p1_len < sizeof( p1_buff ) )
		p1 = p1_buff;
	    else  if ( ! (p1 = (char *)MEreqmem(0, p1_len + 1, FALSE, NULL)) )
	    	break;

	    if ( ! gcd_get_bytes( ccb, p1_len, (u_i1 *)p1 ) )  break;
	    p1[ p1_len ] = EOS;
	    incomplete = FALSE;

	    if ( p1_len > 0 )
		switch( qp.param_id )
		{
	    	case MSG_QP_STMT_NAME :  p_rec |= QP_STMT;	break;
	    	case MSG_QP_PROC_NAME :  p_rec |= QP_PNAM;	break;
	    	}
	    break;

	case MSG_QP_CRSR_NAME :
	case MSG_QP_SCHEMA_NAME :
	    if ( p2 )  break;	/* Already set?  Should not happen */
	    p2_len = qp.val_len;

	    if ( p2_len < sizeof( p2_buff ) )
		p2 = p2_buff;
	    else  if ( ! (p2 = (char *)MEreqmem(0, p2_len + 1, FALSE, NULL)) )
	    	break;

	    if ( ! gcd_get_bytes( ccb, p2_len, (u_i1 *)p2 ) )  break;
	    p2[ p2_len ] = EOS;
	    incomplete = FALSE;

	    if ( p2_len > 0 )
	    	switch( qp.param_id )
		{
		case MSG_QP_CRSR_NAME :	  p_rec |= QP_CRSR;	break;
		case MSG_QP_SCHEMA_NAME : p_rec |= QP_SCHM;	break;
		}
	    break;
	
	case MSG_QP_FLAGS :
	    incomplete = FALSE;

	    switch( qp.val_len )
	    {
	    case 1 :
		if ( ! gcd_get_i1( ccb, (u_i1 *)&t_ui1 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui1;
		    p_rec |= QP_FLAG;
		}
		break;

	    case 2 :
		if ( ! gcd_get_i2( ccb, (u_i1 *)&t_ui2 ) )
		    incomplete = TRUE;
		else
		{
		    flags = t_ui2;
		    p_rec |= QP_FLAG;
		}
		break;

	    case 4 :
		if ( ! gcd_get_i4( ccb, (u_i1 *)&flags ) )
		    incomplete = TRUE;
		else
		    p_rec |= QP_FLAG;
		break;

	    default :
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD     Invalid query flag len: %d\n",
			       ccb->id, qp.val_len );
		incomplete = TRUE;
		break;
	    }
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     unkown parameter ID %d\n",
			   ccb->id, qp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete  ||
         (p_rec & p_req[ qtype ]) != p_req[ qtype ]  ||
	 (p_rec & ~(p_req[ qtype ] | p_opt[ qtype ])) )
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

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	if ( pcb )  gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	goto cleanup;
    }

    gcd_alloc_qdesc( &ccb->qry.svc_parms, 0, NULL );  /* Init query params */
    gcd_alloc_qdesc( &ccb->qry.qry_parms, 0, NULL );
    gcd_alloc_qdesc( &ccb->qry.all_parms, 0, NULL );
    ccb->qry.api_parms = &ccb->qry.qry_parms;
    status = OK;

    /*
    ** Propogate query execution flags.
    */
    if ( flags & MSG_QF_FETCH_FIRST )	ccb->qry.flags |= QRY_FETCH_FIRST;
    if ( flags & MSG_QF_FETCH_ALL )	ccb->qry.flags |= QRY_FETCH_ALL;
    if ( flags & MSG_QF_AUTO_CLOSE )	ccb->qry.flags |= QRY_AUTO_CLOSE;
    if ( flags & MSG_QF_LOCATORS )  pcb->data.query.flags |= IIAPI_QF_LOCATORS;
    if ( flags & MSG_QF_SCROLL )  pcb->data.query.flags |= IIAPI_QF_SCROLL;

    switch( qtype )
    {
    case MSG_QRY_EXQ :	
	ccb->qry.flags |= QRY_TUPLES;
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = (ccb->msg.msg_flags & MSG_HDR_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_QRY_OCQ :	
	ccb->qry.flags |= QRY_CURSOR | QRY_TUPLES;
	ccb->qry.crsr_name = STalloc( p2 );

	if ( flags & MSG_QF_READONLY )
	{
	    u_i4 len = STlength( FOR_READONLY );

	    if ( ! gcd_expand_qtxt( ccb, qtxt_len + len + 1, TRUE ) )
	    {
		status = E_GC4808_NO_MEMORY;
		break;
	    }

	    STcopy( FOR_READONLY, ccb->qry.qtxt + qtxt_len );
	}

	pcb->data.query.type = IIAPI_QT_OPEN;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_length = p2_len;
	desc->ds_nullable = FALSE;

	if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = p2_len;
	MEcopy( (PTR)p2, p2_len, data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case MSG_QRY_PREP:	
	{
	    char prefix[ 64 ];
	    i4 i, j, len;

	    STprintf( prefix, "prepare %s into sqlda from ", p1 );
	    len = STlength( prefix );

	    if ( ! gcd_expand_qtxt( ccb, qtxt_len + len + 1, TRUE ) )
	    {
		status = E_GC4808_NO_MEMORY;
		break;
	    }

	    for( i = qtxt_len, j = i + len; i >= 0; i--, j-- )
		ccb->qry.qtxt[ j ] = ccb->qry.qtxt[ i ];

	    MEcopy( (PTR)prefix, len, (PTR)ccb->qry.qtxt );
	}

	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = (ccb->msg.msg_flags & MSG_HDR_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_QRY_DESC:	
	if ( ! gcd_expand_qtxt( ccb, p1_len + 16, FALSE ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	STprintf( ccb->qry.qtxt, "describe %s%s", 
	    	      (flags & MSG_QF_DESC_OUTPUT) ? "" : "input ", p1 );

	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = (ccb->msg.msg_flags & MSG_HDR_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_QRY_EXS :	
	if ( ! gcd_expand_qtxt( ccb, p1_len + 9, FALSE ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	STprintf( ccb->qry.qtxt, "execute %s", p1 );
	pcb->data.query.type = IIAPI_QT_EXEC;
	pcb->data.query.params = (ccb->msg.msg_flags & MSG_HDR_EOG) 
				 ? FALSE : TRUE;
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_QRY_OCS :	
	ccb->qry.flags |= QRY_CURSOR | QRY_TUPLES;
	ccb->qry.crsr_name = STalloc( p2 );

	{
	    u_i4 len = p1_len + (flags & MSG_QF_READONLY)  
				       ? STlength( FOR_READONLY ) : 0;

	    if ( ! gcd_expand_qtxt( ccb, len + 1, FALSE ) )
	    {
		status = E_GC4808_NO_MEMORY;
		break;
	    }

	    STcopy( p1, ccb->qry.qtxt );
	    if (flags & MSG_QF_READONLY)  STcat(ccb->qry.qtxt, FOR_READONLY);
	}

	pcb->data.query.type = IIAPI_QT_OPEN;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_length = p2_len;
	desc->ds_nullable = FALSE;

	if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = p2_len;
	MEcopy( (PTR)p2, p2_len, data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case MSG_QRY_EXP :	
	ccb->qry.flags |= QRY_PROCEDURE | QRY_TUPLES;
	pcb->data.query.type = IIAPI_QT_EXEC_PROCEDURE;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = NULL;

	if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 
				 (p_rec & QP_SCHM) ? 2 : 1, NULL ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_CHA_TYPE;
	desc->ds_nullable = FALSE;
	desc->ds_length = p1_len;

	if ( p_rec & QP_SCHM )
	{
	    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[1];
	    desc->ds_columnType = IIAPI_COL_SVCPARM;
	    desc->ds_dataType = IIAPI_CHA_TYPE;
	    desc->ds_nullable = FALSE;
	    desc->ds_length = p2_len;
	}

	if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}
	
	data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
	data->dv_null = FALSE;
	data->dv_length = p1_len;
	MEcopy( (PTR)p1, p1_len, data->dv_value );

	if ( p_rec & QP_SCHM )
	{
	    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[1];
	    data->dv_null = FALSE;
	    data->dv_length = p2_len;
	    MEcopy( (PTR)p2, p2_len, data->dv_value );
	}

	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    case MSG_QRY_CDEL :
    case MSG_QRY_CUPD :
	if ( ! (scb = gcd_find_cursor( ccb, p2 )) )
	{
	    status = E_GC4811_NO_STMT;
	    break;
	}

	pcb->data.query.type = (qtype == MSG_QRY_CDEL)
				? IIAPI_QT_CURSOR_DELETE 
				: IIAPI_QT_CURSOR_UPDATE;
	pcb->data.query.params = TRUE;
	pcb->data.query.text = ccb->qry.qtxt;

	if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}
	
	desc = (IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc;
	desc->ds_columnType = IIAPI_COL_SVCPARM;
	desc->ds_dataType = IIAPI_HNDL_TYPE;
	desc->ds_length = sizeof( scb->handle );
	desc->ds_nullable = FALSE;

	if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}
	
	data = (IIAPI_DATAVALUE *)ccb->qry.svc_parms.data;
	data->dv_null = FALSE;
	data->dv_length = sizeof( scb->handle );
	MEcopy( (PTR)&scb->handle, sizeof( scb->handle ), data->dv_value );
	ccb->qry.svc_parms.col_cnt = ccb->qry.svc_parms.max_cols;
	break;

    default : /* shouldn't happen, qtype checked above */
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid query operation: %d\n", 
			ccb->id, qtype);
	status = E_GC480A_PROTOCOL_ERROR;
	break;
    }

    if ( status != OK )
    {
	gcd_sess_error( ccb, &pcb->rcb, status);
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );
	gcd_msg_pause( ccb, TRUE );
	goto cleanup;
    }

    ccb->sequence = QRY_BEGIN;	
    ccb->qry.qry_sm = msg_query_sm;
    msg_query_sm( (PTR)pcb );

  cleanup:

    /*
    ** Release allocated resources.
    */
    if ( p1  &&  p1 != p1_buff )  MEfree( (PTR)p1 );
    if ( p2  &&  p2 != p2_buff )  MEfree( (PTR)p2 );

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
**	    Calling sequence of gcd_new_stmt() changed to permit
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
**	 4-Aug-03 (gordy)
**	    QRY_TUPLES flag now includes cursors.  Don't calculate
**	    pre-fetch limit for updatable cursors or when BLOBs or
**	    BYREF procedure parameters are present.
**	 6-Oct-03 (gordy)
**	    Added ability to return data rows with query results:
**	    new query fetch flags, separated out returning data
**	    descriptor as new state, call fetch module when rows
**	    requested.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Initialize transaction state as first operation in sequencer.
**	    Check for logical processing errors.
**	25-Mar-10 (gordy)
**	    Flush flag changed to message header flags in gcd_send_result().
*/

static void
msg_query_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    GCD_SCB	*scb;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Query: %s\n",
		   ccb->id, gcu_lookup( qrySeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case QRY_BEGIN :
    {
	/*
	** Establish the proper transaction state for the
	** current transaction mode and query being processed.
	*/
	u_i1	xqop;

	switch( pcb->data.query.type )
	{
	case IIAPI_QT_OPEN :		xqop = GCD_XQOP_CURSOR_OPEN;	break;
	case IIAPI_QT_CURSOR_DELETE :
	case IIAPI_QT_CURSOR_UPDATE :	xqop = GCD_XQOP_CURSOR_UPDATE;	break;
	default :			xqop = GCD_XQOP_NON_CURSOR;	break;
	}

	gcd_push_callback( pcb, msg_query_sm );
	gcd_xact_check( pcb, xqop );
        return;
    }

    case QRY_QUERY :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_XACT;
	    break;
	}

	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_query( pcb );
	return;
    
    case QRY_PDESC :	
	if ( pcb->api_error )  
	    ccb->sequence = QRY_CANCEL;
	else  if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) ) 
	{
	    /*
	    ** Need DESC message.  We either have already
	    ** received the message (in which case the call
	    ** below will recurse) or need to wait for the
	    ** message to arrive (in which case the call
	    ** below will do nothing).
	    */
	    ccb->qry.flags |= QRY_NEED_DESC;
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, FALSE );
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
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD query: no descriptor\n", 
			    ccb->id );
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return;
	}

	if ( ccb->qry.qry_parms.max_cols )
	{
	    if ( ! gcd_alloc_qdata( &ccb->qry.qry_parms, 1 ) )
	    {
		gcd_del_pcb( pcb );
		gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		return;
	    }

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
	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_setDesc( pcb );
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
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, FALSE );
	    return;
	}

	ccb->qry.flags |= QRY_HAVE_DATA;
	break;

    case QRY_PUTP :
	if ( ! (ccb->qry.flags & QRY_HAVE_DATA) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD query: no data\n", ccb->id );
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
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
	    gcd_combine_qdata( &ccb->qry.svc_parms, &ccb->qry.qry_parms, 
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
	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_putParm( pcb );
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
	    gcd_del_pcb( pcb );

	    if ( ccb->msg.msg_len )
		gcd_msg_data( ccb );		/* Finish current message */
	    else
		gcd_msg_pause( ccb, FALSE );	/* Check for more messages */
	    return;
	}

	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_getDesc( pcb );
	return;

    case QRY_GETQI :	
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CANCEL;
	    break;
	}

	/*
	** If there is no result set, then the current
	** statement simply needs to be closed.
	*/
	if ( ! pcb->data.desc.count )  
	{
	    ccb->qry.flags &= ~QRY_TUPLES;	/* No tuples */
	    ccb->sequence = QRY_CLOSE;
	}

	/*
	** Retrieving query info on tuple streams,
	** except for cursors, would close the
	** stream and therefore must be skipped.
	*/
	if ( ccb->qry.flags & QRY_TUPLES  &&  ! (ccb->qry.flags & QRY_CURSOR) ) 
	    break;

	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_gqi( pcb );
	return;

    case QRY_SDESC :
	if ( pcb->api_error )
	{
	    ccb->sequence = QRY_CLOSE;
	    break;
	}

	/*
	** Return the tuple descriptor.
	*/
	send_desc( ccb, &pcb->rcb, pcb->data.desc.count, pcb->data.desc.desc );

	/*
	** Non-tuple statements are now complete and can be closed.
	*/
	if ( ! (ccb->qry.flags & QRY_TUPLES) )  ccb->sequence = QRY_CLOSE;
	break;

    case QRY_DONE :	
	/*
	** For cursors, check if updatable or readonly.
	**
	** DBMS returns READONLY flag for preceding query
	** info request which is then saved in result data.
	**
	** Pre-fetch is not allowed with updatable cursors,
	** so the associated actions to return rows with
	** the query results are also disabled.
	*/
	if ( ccb->qry.flags & QRY_CURSOR  &&
	     ! (pcb->result.flags & PCB_RSLT_READ_ONLY) )
	{
	    ccb->qry.flags |= QRY_UPDATE;
	    ccb->qry.flags &= ~(QRY_FETCH_FIRST | QRY_FETCH_ALL);
	}

	/*
	** A suggested row pre-fetch size is returned when pre-
	** fetching is allowed for the associated query.  The 
	** suggested size is the number of tuples which will fit 
	** in a single message buffer.
	**
	** Row pre-fetching is disabled for updatable cursors,
	** procedure BYREF parameters (since there should only
	** be one row), and when the tuple set contains BLOB 
	** columns (determined in calc_tuple_length()).
	*/
	if ( 
	     ! (ccb->qry.flags & QRY_UPDATE)  &&
	     ! (ccb->qry.flags & QRY_BYREF) 
	   )
	{
	    i4 len = calc_tuple_length( ccb, pcb->data.desc.count, 
					pcb->data.desc.desc );
	    /*
	    ** Skip if tuple length can't be determined or
	    ** tuple contains a BLOB.  Also disable initial
	    ** fetch which is just an automatic pre-fetch.
	    */
	    if ( len <= 0  ||  ccb->qry.flags & QRY_BLOB )
		ccb->qry.flags &= ~QRY_FETCH_FIRST;
	    else
	    {
		/*
		** Calculate the buffer space remaining for tuples
		** while reserving space for TL header, DATA msg
		** header and a DONE message.
		*/
		i4 buff_size = ccb->max_buff_len - GCD_global.tl_hdr_sz -
				MSG_HDR_LEN - (MSG_HDR_LEN + 8);

		/*
		** Min suggested size is one row.
		*/
		if ( len > buff_size )
		    pcb->result.fetch_limit = 1;
		else
		    pcb->result.fetch_limit = buff_size / len;

		pcb->result.flags |= PCB_RSLT_FETCH_LIMIT;
	    }
	}

	/*
	** Save the current statement for future reference.
	*/
	if ( (scb = gcd_new_stmt( ccb, pcb )) == NULL )
	{
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return;
	}

	pcb->result.flags |= PCB_RSLT_STMT_ID;
	pcb->result.stmt_id.id_high = scb->stmt_id.id_high;
	pcb->result.stmt_id.id_low = scb->stmt_id.id_low;

	/*
	** For tuple queries, rows may now be returned
	** as a part of the query results.
	*/
	if ( ccb->qry.flags & (QRY_FETCH_FIRST | QRY_FETCH_ALL) )
	{
	    /*
	    ** Send intermediate RESULT message to provide
	    ** query execution results then pass control to
	    ** fetch module.  Pre-fetch size is cleared by
	    ** gcd_send_results(), so save for gcd_fetch().
	    */
	    u_i4 pre_fetch = (pcb->result.flags & PCB_RSLT_FETCH_LIMIT) 
			     ? pcb->result.fetch_limit : 1;
	    gcd_send_result( ccb, &pcb->rcb, pcb, 0 );
	    gcd_fetch( ccb, scb, pcb, pre_fetch );
	    return;
	}

	/*
	** For tuple queries the data stream is still active
	** on the connection, so make sure we keep the statement
	** active locally.  Cursors are dormant after being opened
	** and in between fetches.
	*/
	if ( ! (ccb->qry.flags & QRY_CURSOR) )  ccb->api.stmt = scb->handle;
	ccb->sequence = QRY_EXIT;
	break;

    case QRY_CANCEL :		/* Cancel the current query */
	gcd_msg_flush( ccb );
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = QRY_EXIT;
	    break;
	}
	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_cancel( pcb );
	return;

    case QRY_CLOSE :		/* Close current query */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = QRY_EXIT;
	    break;
	}
	gcd_push_callback( pcb, msg_query_sm );
	gcd_api_close( pcb, NULL );	
	return;

    case QRY_XACT :		/* Check transaction state */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )  
	    break;	/* Handled below */

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	{
	    gcd_push_callback( pcb, msg_query_sm );
	    gcd_xact_abort( pcb );
	    return;
	}

	if ( ccb->qry.flags & QRY_CURSOR )
	{
	    gcd_push_callback( pcb, msg_query_sm );
	    gcd_xact_check( pcb, GCD_XQOP_CURSOR_CLOSE );
	    return;
	}
	break;

    case QRY_EXIT :		/* Query completed. */
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
	    TRdisplay( "%4d    GCD invalid query processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: gcd_msg_desc
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
**	 4-Aug-03 (gordy)
**	    Flag BYREF procedure parameters.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time.
**	28-Jun-06 (gordy)
**	    Added support for OUT procedure parameters.  RCB no longer 
**	    allocated along with PCB.  Allocate explicitly to ensure 
**	    error messages returned to client.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
**	25-Mar-10 (gordy)
**	    Parameter processing shared between regular and batched
**	    queries.  Callback the runtime processing routine.
*/

void
gcd_msg_desc( GCD_CCB *ccb )
{
    IIAPI_DESCRIPTOR	*desc;
    GCD_PCB		*pcb;
    STATUS		status;
    u_i2		count, param;
    i2			def_type;

    if ( ! (ccb->qry.flags & QRY_NEED_DESC)  ||
	 ! gcd_get_i2( ccb, (u_i1 *)&count ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    if ( ccb->qry.flags & QRY_NEED_DESC )
		TRdisplay( "%4d    GCD no descriptor info\n", ccb->id );
	    else
		TRdisplay( "%4d    GCD unexpected descriptor\n", ccb->id );

	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD parameter descriptor: %d params\n", 
		   ccb->id, count );

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	if ( pcb )  gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return;
    }

    if ( ! gcd_alloc_qdesc( &ccb->qry.qry_parms, count, NULL ) )
    {
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
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

	if ( ! gcd_get_i2( ccb, (u_i1 *)&sqlType ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&dbmsType ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&length ) )  break;
	if ( ! gcd_get_i1( ccb, &prec ) )  break;
	if ( ! gcd_get_i1( ccb, &scale ) )  break;
	if ( ! gcd_get_i1( ccb, &flags ) )  break;

	if ( ! gcd_get_i2( ccb, (u_i1 *)&len ) )  break;
	if ( len )
	{
	    if ( ! (name = MEreqmem( 0, len + 1, FALSE, NULL )) )  break;
	    if ( ! gcd_get_bytes( ccb, len, (u_i1 *)name ) )  break;
	    name[ len ] = EOS;
	}

	switch( sqlType )
	{
	case SQL_TINYINT :
	    dfltType = IIAPI_INT_TYPE;
	    length = IIAPI_I1_LEN;
	    prec = scale = 0;
	    break;

	case SQL_SMALLINT :
	    dfltType = IIAPI_INT_TYPE;
	    length = IIAPI_I2_LEN;
	    prec = scale = 0;
	    break;

	case SQL_INTEGER :
	    dfltType = IIAPI_INT_TYPE;
	    length = IIAPI_I4_LEN;
	    prec = scale = 0;
	    break;

	case SQL_BIGINT :
	    dfltType = IIAPI_INT_TYPE;
	    length = IIAPI_I8_LEN;
	    prec = scale = 0;
	    break;

	case SQL_REAL :
	    dfltType = IIAPI_FLT_TYPE;
	    length = IIAPI_F4_LEN;
	    prec = scale = 0;
	    break;

	case SQL_FLOAT :
	case SQL_DOUBLE :
	    dfltType = IIAPI_FLT_TYPE;
	    length = IIAPI_F8_LEN;
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
	    prec = scale = 0;

	    switch( dbmsType )
	    {
	    case IIAPI_LCLOC_TYPE :
	    case IIAPI_LNLOC_TYPE :
		length = IIAPI_LOCATOR_LEN;
		break;

	    default :
		length = ccb->max_buff_len;  /* Segments may not span buffers */
		break;
	    }
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
	    prec = scale = 0;

	    switch( dbmsType )
	    {
	    case IIAPI_LBLOC_TYPE :
		length = IIAPI_LOCATOR_LEN;
		break;

	    default :
		length = ccb->max_buff_len;  /* Segments may not span buffers */
		break;
	    }
	    break;

	case SQL_BOOLEAN :
	    dfltType = IIAPI_BOOL_TYPE;
	    length = IIAPI_BOOL_LEN;
	    prec = scale = 0;
	    break;

	case SQL_DATE :
	    dfltType = IIAPI_ADATE_TYPE;
	    length = IIAPI_ADATE_LEN;
	    prec = scale = 0;
	    break;

	case SQL_TIME :
	    dfltType = IIAPI_TIME_TYPE;
	    length = IIAPI_TIME_LEN;

	    /*
	    ** DAM carries scale, API carries precision
	    */
	    prec = scale;
	    scale = 0;
	    break;

	case SQL_TIMESTAMP :
	    switch( dbmsType )
	    {
	    case IIAPI_TS_TYPE :
	    case IIAPI_TSWO_TYPE :
	    case IIAPI_TSTZ_TYPE :
		length = IIAPI_TS_LEN;

		/*
		** DAM carries scale, API carries precision
		*/
		prec = scale;
		scale = 0;
	        break;

	    default :
		dfltType = IIAPI_IDATE_TYPE;
		length = IIAPI_IDATE_LEN;
		prec = scale = 0;
	        break;
	    }
	    break;

	case SQL_INTERVAL :
	    switch( dbmsType )
	    {
	    case IIAPI_INTYM_TYPE :
		length = IIAPI_INTYM_LEN;
		prec = scale = 0;
		break;

	    case IIAPI_INTDS_TYPE :
	    	length = IIAPI_INTDS_LEN;

		/*
		** DAM carries scale, API carries precision
		*/
		prec = scale;
		scale = 0;
		break;

	    default :
		dfltType = IIAPI_IDATE_TYPE;
		length = IIAPI_IDATE_LEN;
		prec = scale = 0;
	        break;
	    }
	    break;

	case SQL_NULL :
	    dfltType = IIAPI_LTXT_TYPE;
	    length = 2;
	    prec = scale = 0;
	    break;

	case SQL_BIT :
	case SQL_OTHER :
	default :
	    status = E_GC4812_UNSUPP_SQL_TYPE;
	    break;
	}

	desc[ param ].ds_dataType = (dbmsType != 0) ? dbmsType : dfltType;
	desc[ param ].ds_nullable = (flags & MSG_DSC_NULL) ? TRUE : FALSE;
	desc[ param ].ds_length = length;
	desc[ param ].ds_precision = prec;
	desc[ param ].ds_scale = scale;
	if ( desc[ param ].ds_columnName )  MEfree(desc[param].ds_columnName);
	desc[ param ].ds_columnName = name;

	/*
	** !!! TODO: change when fixed in DBMS
	**
	** There is a bug in the server where using both
	** GCA_PARAM_NAMES_MASK and GCA_IS_OUTPUT causes
	** GCA message corruption.  Since OUT requires a
	** NULL value at least, flagging these as INOUT
	** at time time is the best that can be done.
	*/ 
	desc[ param ].ds_columnType = 
	    (flags & MSG_DSC_GTT) ? IIAPI_COL_PROCGTTPARM :
	    (flags & MSG_DSC_PIO) ? IIAPI_COL_PROCINOUTPARM :
	    (flags & MSG_DSC_POUT) ? IIAPI_COL_PROCINOUTPARM : /* PROCOUTPARM */
	    ((flags & MSG_DSC_PIN) ? IIAPI_COL_PROCINPARM : def_type);

	if ( desc[ param ].ds_columnType == IIAPI_COL_PROCOUTPARM  ||
	     desc[ param ].ds_columnType == IIAPI_COL_PROCINOUTPARM )
	    ccb->qry.flags |= QRY_BYREF;
    }

    if ( param < count  ||  ccb->msg.msg_len )
    {
	if ( status == OK )  status = E_GC480A_PROTOCOL_ERROR;
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid DESC message format (%d,%d,%d)\n", 
			ccb->id, param, count, ccb->msg.msg_len );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, status );
	return;
    }

    ccb->qry.flags |= QRY_HAVE_DESC;
    (*ccb->qry.qry_sm)( (PTR)pcb );
    return;
}


/*
** Name: gcd_msg_data
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
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	 4-Aug-05 (gordy)
**	    Allow room for leading zero in temp decimal buffer.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time.
**	28-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 9-Sep-08 (gordy)
**	    Expand temp buffer to support larger decimal precision.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
**	25-Mar-10 (gordy)
**	    Parameter processing shared between regular and batched
**	    queries.  Callback the runtime processing routine.  Check
**	    for end-of-batch which separates query requests in a batch.
*/

void
gcd_msg_data( GCD_CCB *ccb )
{
    GCD_PCB		*pcb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    u_i2		param;
    bool		done, more_data;
    i2 blobFound = 0;

    if ( ! (ccb->qry.flags & QRY_NEED_DATA) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD unexpected data message\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD parameter data: current %d, total %d\n", 
		   ccb->id, ccb->qry.qry_parms.cur_col, 
		   ccb->qry.qry_parms.max_cols );

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	if ( pcb )  gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
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
	STATUS	status = E_GC480A_PROTOCOL_ERROR;
	u_i1	indicator;
	u_i2	length;

	/*
	** Buffer for char(128) value which is large
	** enough for conversion of any standard type.
	*/
	char	dbuff[ 129 ];

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
	    if ( ! gcd_get_i1( ccb, &indicator ) )  break;
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
	    case 1 :	if ( ! gcd_get_i1( ccb, data[ param ].dv_value ) )  
			    error = TRUE; 
			break;

	    case 2 :	if ( ! gcd_get_i2(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE; 
			break;

	    case 4 :	if ( ! gcd_get_i4(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE; 
			break;

	    case 8 :	if ( ! gcd_get_i8(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE; 
			break;

	    default :	error = TRUE;	break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( desc[ param ].ds_length )
	    {
	    case 4 :	if ( ! gcd_get_f4(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE;
			break;

	    case 8 :	if ( ! gcd_get_f8(ccb, (u_i1 *)data[param].dv_value) )
			    error = TRUE;
			break;

	    default :	error = TRUE;	break;
	    }
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_BYTE_TYPE :
	case IIAPI_NBR_TYPE :
	    if ( ! gcd_get_bytes( ccb, desc[ param ].ds_length,
				   (u_i1 *)data[ param ].dv_value ) )  
		error = TRUE; 
	    break;

	case IIAPI_NCHA_TYPE :
	    if ( ! gcd_get_ucs2( ccb, desc[ param ].ds_length / sizeof(UCS2),
				  (u_i1 *)data[ param ].dv_value ) )
		error = TRUE;
	    break;

	case IIAPI_VCH_TYPE :
	case IIAPI_VBYTE_TYPE :
	    if ( 
		 ! gcd_get_i2( ccb, (u_i1 *)&length )  ||
	         (length + 2) > desc[ param ].ds_length  ||
	         ! gcd_get_bytes( ccb, length,
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
		 ! gcd_get_i2( ccb, (u_i1 *)&length )  ||
	         (length * sizeof(UCS2) + 2) > desc[ param ].ds_length  ||
		 ! gcd_get_ucs2( ccb, length,
				  (u_i1 *)data[ param ].dv_value + 2 )
	       )
		error = TRUE;
	    else
	    {
		MEcopy( (PTR)&length, 2, data[ param ].dv_value );
		data[ param ].dv_length = length * sizeof( UCS2 ) + 2;
	    }
	    break;

	case IIAPI_LCLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	case IIAPI_LBLOC_TYPE :
	    if ( ! gcd_get_i4(ccb, (u_i1 *)data[param].dv_value) )
		error = TRUE; 
	    break;

	case IIAPI_LVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LBYTE_TYPE :
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
	    ccb->qry.qry_parms.more_segments = TRUE;

	    if ( ! gcd_get_i2( ccb, (u_i1 *)&length ) )
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
		if ( GCD_global.gcd_trace_level >= 5 )
		    TRdisplay( "%4d    GCD recv end-of-segments\n", ccb->id );
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
		    if ( ! gcd_get_bytes( ccb, length,
					   (u_i1 *)data[param].dv_value + 2 ) )
			error = TRUE; 
		}
		else
		{
		    if ( ! gcd_get_ucs2( ccb, chrs,
					  (u_i1 *)data[param].dv_value + 2 ) )
			error = TRUE;
		}

		if ( ! error )
		{
		    if ( GCD_global.gcd_trace_level >= 5 )
			TRdisplay( "%4d    GCD recv segment: %d\n", 
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
		    if ( gcd_get_i2( ccb, (u_i1 *)&length ) )
		    {
			/*
			** Additional segment present, must
			** be zero length (end-of-segments).
			*/
			if ( length )  
			    error = TRUE;
			else  
			{
			    if ( GCD_global.gcd_trace_level >= 5 )
				TRdisplay( "%4d    GCD recv end-of-segments\n",
					    ccb->id );

			    ccb->qry.qry_parms.more_segments = FALSE;
			}
		    }
		}
	    }
	    break;

	case IIAPI_BOOL_TYPE :
	    if ( ! gcd_get_i1( ccb, data[ param ].dv_value ) )  error = TRUE; 
	    break;

	case IIAPI_DEC_TYPE :
	case IIAPI_IDATE_TYPE :
	case IIAPI_ADATE_TYPE :
	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	case IIAPI_TMTZ_TYPE :
	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	case IIAPI_TSTZ_TYPE :
	case IIAPI_INTYM_TYPE :
	case IIAPI_INTDS_TYPE :
	    if ( 
		 ! gcd_get_i2( ccb, (u_i1 *)&length )  ||
	         length >= sizeof( dbuff )  ||
	         ! gcd_get_bytes( ccb, length, (u_i1 *)dbuff )
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

		status = gcd_api_format( ccb, &sdesc, &sdata,
					  &desc[ param ], &data[ param ] );
		if ( status != OK )  error = TRUE;
	    }
	    break;

	default :
	    /*
	    ** Shouldn't happen since we assigned
	    ** the types in msg_desc().
	    */
	    status = E_GC4812_UNSUPP_SQL_TYPE;
	    error = TRUE;
	    break;
	}

	if ( error )  
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD error processing data (%d): 0x%x\n", 
			   ccb->id, param, status );
	    ccb->qry.qry_parms.more_segments = FALSE;
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, status );
	    return;
	}

	/*
	** BLOB handling requires individual segments
	** to be passed on to the server prior to
	** processing subsequent segments or columns.
	*/
	switch ( desc[ param ].ds_dataType )
	{
	     case IIAPI_LVCH_TYPE:
	     case IIAPI_LBYTE_TYPE:
	     case IIAPI_GEOM_TYPE:
	     case IIAPI_POINT_TYPE:
	     case IIAPI_MPOINT_TYPE:
	     case IIAPI_LINE_TYPE:
	     case IIAPI_MLINE_TYPE:
	     case IIAPI_POLY_TYPE:
	     case IIAPI_MPOLY_TYPE:
	     case IIAPI_GEOMC_TYPE:
	     case IIAPI_LNVCH_TYPE:
	     {
		blobFound = 1;
		break;
	     }
	}
	if( blobFound == 1)
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
    **	2) end-of-data if and only if last column completed.
    */
    done = (param >= ccb->qry.qry_parms.max_cols  &&  
	    ! ccb->qry.qry_parms.more_segments);
    more_data = (ccb->msg.msg_len  || 
		! (ccb->msg.msg_flags & (MSG_HDR_EOB | MSG_HDR_EOG)));

    if ( ccb->qry.qry_parms.col_cnt <= 0  || 
	 (! done  &&  ! more_data)  ||  (done  &&  more_data) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay("%4d    GCD invalid parameter set: %d,%d,%d%s (%d%s%s)\n",
		      ccb->id, ccb->qry.qry_parms.cur_col, 
		      ccb->qry.qry_parms.col_cnt, ccb->qry.qry_parms.max_cols, 
		      ccb->qry.qry_parms.more_segments ? " <more>" : "",
		      ccb->msg.msg_len,
		      (ccb->msg.msg_flags & MSG_HDR_EOB) ? " EOB" : "",
		      (ccb->msg.msg_flags & MSG_HDR_EOG) ? " EOG" : "" );

	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD processed parameters: %d to %d %s\n", 
		   ccb->id, ccb->qry.qry_parms.cur_col, 
		   ccb->qry.qry_parms.cur_col + ccb->qry.qry_parms.col_cnt - 1,
		   ccb->qry.qry_parms.more_segments ? "(more segments)" : "" );

    ccb->qry.flags |= QRY_HAVE_DATA;
    (*ccb->qry.qry_sm)( (PTR)pcb );
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
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time.
**	31-Aug-06 (gordy)
**	    ANSI time precision carried as scale (fractional digits).
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
*/

static void
send_desc( GCD_CCB *ccb, GCD_RCB **rcb_ptr, 
	   u_i2 count, IIAPI_DESCRIPTOR *desc )
{
    u_i2 i;

    gcd_msg_begin( ccb, rcb_ptr, MSG_DESC, 0 );
    gcd_put_i2( rcb_ptr, count );

    for( i = 0; i < count; i++ )
    {
	i2	type;
	u_i2 	len;
	u_i1 	null, prec, scale;

	type = SQL_OTHER;
	null = desc[i].ds_nullable ? MSG_DSC_NULL : 0;
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
	    case 8 : type = SQL_BIGINT;		break;
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

	case IIAPI_BOOL_TYPE :
	    type = SQL_BOOLEAN;
	    len = 0;
	    break;

	case IIAPI_IDATE_TYPE :
	    type = SQL_TIMESTAMP;
	    len = 0;
	    break;

	case IIAPI_ADATE_TYPE :
	    type = SQL_DATE;
	    len = 0;
	    break;

	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	case IIAPI_TMTZ_TYPE :
	    type = SQL_TIME;

	    /*
	    ** API carries precision, DAM carries scale.
	    */
	    scale = desc[i].ds_precision;
	    len = 0;
	    break;

	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	case IIAPI_TSTZ_TYPE :
	    type = SQL_TIMESTAMP;

	    /*
	    ** API carries precision, DAM carries scale.
	    */
	    scale = desc[i].ds_precision;
	    len = 0;
	    break;

	case IIAPI_INTYM_TYPE :
	    type = SQL_INTERVAL;
	    len = 0;
	    break;

	case IIAPI_INTDS_TYPE :
	    type = SQL_INTERVAL;

	    /*
	    ** API carries precision, DAM carries scale.
	    */
	    scale = desc[i].ds_precision;
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
	case IIAPI_LCLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	    type = SQL_LONGVARCHAR; 
	    len = 0; 
	    break;

	case IIAPI_BYTE_TYPE :	type = SQL_BINARY;		break;
	case IIAPI_NBR_TYPE :	type = SQL_BINARY;		break;
	case IIAPI_VBYTE_TYPE :	type = SQL_VARBINARY; len -= 2;	break;

	case IIAPI_LBYTE_TYPE :	
	case IIAPI_LBLOC_TYPE :
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
	    type = SQL_LONGVARBINARY; 
	    len = 0; 
	    break;

	case IIAPI_LOGKEY_TYPE :
	case IIAPI_TABKEY_TYPE :
	    type = SQL_CHAR;
	    break;

	case IIAPI_HNDL_TYPE :	/* Shouldn't happen */
	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid datatype: %d\n", 
			   ccb->id, desc[i].ds_dataType );
	    gcd_sess_abort( ccb, E_GC4812_UNSUPP_SQL_TYPE );
	    return;
	}

	if ( type == SQL_OTHER )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid data length: %d (%d)\n", 
			   ccb->id, len, desc[i].ds_dataType );
	    gcd_sess_abort( ccb, E_GC4812_UNSUPP_SQL_TYPE );
	    return;
	}

	gcd_put_i2( rcb_ptr, type );
	gcd_put_i2( rcb_ptr, desc[i].ds_dataType );
	gcd_put_i2( rcb_ptr, len );
	gcd_put_i1( rcb_ptr, prec );
	gcd_put_i1( rcb_ptr, scale );
	gcd_put_i1( rcb_ptr, null );

	if ( ! desc[i].ds_columnName  ||  
	     ! (len = STlength( desc[i].ds_columnName )) )
	    gcd_put_i2( rcb_ptr, 0 );
	else
	{
	    gcd_put_i2( rcb_ptr, len );
	    gcd_put_bytes( rcb_ptr, len, (u_i1 *)desc[i].ds_columnName );
	}
    }

    gcd_msg_end( rcb_ptr, FALSE );
    return;
}


/*
** Name: gcd_expand_qtxt
**
** Description:
**	Expand query buffer if needed.  Current query text is saved
**	if requested, otherwise buffer is initialized..
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
**	 6-Nov-06 (gordy)
**	    Allow text longer than 64K.  Initialize buffer if no
**	    carry-over text.
*/

bool
gcd_expand_qtxt( GCD_CCB *ccb, u_i4 length, bool save )
{
    char *qtxt = ccb->qry.qtxt;

    if ( length <= ccb->qry.qtxt_max )
    {
	if ( qtxt  &&  ! save )  qtxt[0] = EOS;
    	return( TRUE );
    }

    ccb->qry.qtxt_max = length;

    if ( ! (ccb->qry.qtxt = (char *)MEreqmem( 0, length, FALSE, NULL )) )
    {
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD couldn't allocate query text: %d bytes\n",
		       ccb->id, length );

	if ( qtxt )  MEfree( (PTR)qtxt );
	ccb->qry.qtxt = NULL;
	ccb->qry.qtxt_max = 0;
	return( FALSE );
    }

    if ( ! qtxt )  
    	ccb->qry.qtxt[0] = EOS;
    else
    {
	if ( save )  
	    STcopy( qtxt, ccb->qry.qtxt );
	else
	    ccb->qry.qtxt[0] = EOS;

	MEfree( (PTR)qtxt );
    }

    return( TRUE );
}



/*
** Name: calc_tuple_length
**
** Description:
**	Calculates the length of tuple associated with a set of
**	column descriptors.  Sets the query BLOB flag if any
**	column is a BLOB.  Returns a negative value if tuple
**	length cannot be calculated (BLOB present primarily).
**
** Input:
**	ccb		Connection control block.
**	count		Number of descriptors.
**	desc		Descriptors.
**
** Output:
**	None.
**
** Returns:
**	i4		Tuple length (negative if BLOB present).
**
** History:
**	13-Dec-99 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Pre-fetch limit is now only sent if greater than one.
**	    Limit does not need to be turned off.
**	10-May-01 (gordy)
**	    Added support for UCS2 data.
**	 6-Jun-03 (gordy)
**	    Allow room for DONE message along with tuple data.  Make
**	    sure fetch_limit is always at least one.
**	 4-Aug-03 (gordy)
**	    Renamed and changed to just calculate tuple length.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
*/

static i4
calc_tuple_length
( 
    GCD_CCB		*ccb, 
    u_i2		count, 
    IIAPI_DESCRIPTOR	*desc 
)
{
    i4		len;
    u_i2	col;

    for( len = 0, col = 0; col < count; col++ )
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
	    case 8 :	len += CV_N_I8_SZ;	break;
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

	case IIAPI_BOOL_TYPE :
	    len += CV_N_I1_SZ;
	    break;

	case IIAPI_IDATE_TYPE :
	    len += 21;				/* YYYY-MM-DD hh:mm:ss */
	    break;

	case IIAPI_ADATE_TYPE :
	    len += 12;				/* YYYY-MM-DD */
	    break;

	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	    len += 10 + 			/* hh:mm:ss[.p] */
	    	   (desc[col].ds_precision ? desc[col].ds_precision + 1 : 0);
	    break;

	case IIAPI_TMTZ_TYPE :
	    len += 16 + 			/* hh:mm:ss[.p]Shh:mm */
	    	   (desc[col].ds_precision ? desc[col].ds_precision + 1 : 0);
	    break;

	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	    len += 21 +				/* YYYY-MM-DD hh:mm:ss[.p] */
	    	   (desc[col].ds_precision ? desc[col].ds_precision + 1 : 0);
	    break;

	case IIAPI_TSTZ_TYPE :
	    len += 27 +			/* YYYY-MM-DD hh:mm:ss[.p]Shh:mm */
	    	   (desc[col].ds_precision ? desc[col].ds_precision + 1 : 0);
	    break;

	case IIAPI_INTYM_TYPE :
	    len += 8;				/* SYY-MM */
	    break;

	case IIAPI_INTDS_TYPE :
	    len += 14 +				/* SDD hh:mm:ss[.p] */
	    	   (desc[col].ds_precision ? desc[col].ds_precision + 1 : 0);
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_NCHA_TYPE :
	case IIAPI_CHR_TYPE :
	case IIAPI_VCH_TYPE :
	case IIAPI_NVCH_TYPE :
	case IIAPI_BYTE_TYPE :
	case IIAPI_NBR_TYPE :
	case IIAPI_VBYTE_TYPE :
	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	    len += desc[ col ].ds_length;
	    break;

	case IIAPI_LCLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	case IIAPI_LBLOC_TYPE :
	    len += CV_N_I4_SZ;
	    break;

	case IIAPI_LVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LBYTE_TYPE :
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
	    ccb->qry.flags |= QRY_BLOB;

	    /*
	    ** Fall through
	    */

	default :
	    /*
	    ** Length cannot be calculated.
	    */
	    return( -1 );
	}
    }

    return( len );
}



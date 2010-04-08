/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <cv.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcrqst.c
**
** Description:
**	JDBC request message processing.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	25-Jul-00 (gordy)
**	    Added support for parameter names request.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	12-Oct-00 (gordy)
**	    Transaction handling localized in jdbc_xact_check().
**	    Only check transaction state when query is required.
**	24-Oct-00 (gordy)
**	    Added support for gateway procedure catalogs.
**	 7-Mar-01 (gordy)
**	    Added support for dbmsinfo().
**	18-Apr-01 (gordy)
**	    Added request to return prepared transaction IDs.
**	18-May-01 (gordy)
**	    Info request ID and value stored in CCB.  Generate
**	    JDBC Server capabilities.
*/	

static	GCULIST	reqMap[] =
{
    { JDBC_REQ_CAPABILITY,	"DBMS Capabilities" },
    { JDBC_REQ_PARAM_NAMES,	"Parameter Names" },
    { JDBC_REQ_DBMSINFO,	"DbmsInfo()" },
    { JDBC_REQ_2PC_XIDS,	"2PC Transaction IDs" },
    { 0, NULL }
};

#define	REQ_CAPS	0	/* DBMS Capabilities */
#define	REQ_CAPS_1	1
#define	REQ_CAPS_2	2
#define	REQ_CAPS_3	3
#define	REQ_CAPS_4	4
#define	REQ_CAPS_5	5
#define	REQ_CAPS_6	6
#define	REQ_PNAM	10
#define	REQ_PNAM_1	11
#define	REQ_PNAM_2	12
#define	REQ_PNAM_3	13
#define	REQ_PNAM_4	14
#define	REQ_PNAM_5	15
#define	REQ_PNAM_6	16
#define	REQ_PNAM_7	17
#define	REQ_PNAM_8	18
#define	REQ_INFO	20
#define	REQ_INFO_1	21
#define	REQ_INFO_2	22
#define	REQ_INFO_3	23
#define	REQ_INFO_4	24
#define	REQ_INFO_5	25
#define	REQ_INFO_6	26
#define	REQ_XIDS	30
#define	REQ_XIDS_1	31
#define	REQ_XIDS_2	32
#define	REQ_XIDS_3	33
#define	REQ_XIDS_4	34
#define	REQ_XIDS_5	35
#define	REQ_XIDS_6	36
#define	REQ_CANCEL	97	/* Cancel request */
#define	REQ_CANCEL_1	98
#define	REQ_DONE	99	/* Request completed */

static	GCULIST reqSeqMap[] =
{
    { REQ_CAPS,		"CAPS 0" },
    { REQ_CAPS_1,	"CAPS 1" },
    { REQ_CAPS_2,	"CAPS 2" },
    { REQ_CAPS_3,	"CAPS 3" },
    { REQ_CAPS_4,	"CAPS 4" },
    { REQ_CAPS_5,	"CAPS 5" },
    { REQ_CAPS_6,	"CAPS 6" },
    { REQ_PNAM,		"PNAM 0" },
    { REQ_PNAM_1,	"PNAM 1" },
    { REQ_PNAM_2,	"PNAM 2" },
    { REQ_PNAM_3,	"PNAM 3" },
    { REQ_PNAM_4,	"PNAM 4" },
    { REQ_PNAM_5,	"PNAM 5" },
    { REQ_PNAM_6,	"PNAM 6" },
    { REQ_PNAM_7,	"PNAM 7" },
    { REQ_PNAM_8,	"PNAM 8" },
    { REQ_INFO,		"INFO 0" },
    { REQ_INFO_1,	"INFO 1" },
    { REQ_INFO_2,	"INFO 2" },
    { REQ_INFO_3,	"INFO 3" },
    { REQ_INFO_4,	"INFO 4" },
    { REQ_INFO_5,	"INFO 5" },
    { REQ_INFO_6,	"INFO 6" },
    { REQ_XIDS,		"XIDS 0" },
    { REQ_XIDS_1,	"XIDS 1" },
    { REQ_XIDS_2,	"XIDS 2" },
    { REQ_XIDS_3,	"XIDS 3" },
    { REQ_XIDS_4,	"XIDS 4" },
    { REQ_XIDS_5,	"XIDS 5" },
    { REQ_XIDS_6,	"XIDS 6" },
    { REQ_CANCEL,	"CANCEL 0" },
    { REQ_CANCEL_1,	"CANCEL 1" },
    { REQ_DONE,		"DONE" },
    { 0, NULL }
};

/*
** Parameter flags and request required/optional parameters.
*/
#define	RQP_SCHM	0x01
#define	RQP_PNAM	0x02
#define	RQP_INFO	0x04
#define	RQP_DB		0x08

static	u_i2	p_req[] =
{
    0,
    0,
    RQP_PNAM,
    RQP_INFO,
    RQP_DB,
};

static	u_i2	p_opt[] = 
{
    0,
    0,
    RQP_SCHM,
    0,
};


#define	CAPS_ROW_MAX		30

static	char	*qry_caps = 
    "select cap_capability, cap_value from iidbcapabilities";

static char	*qry_info = "select dbmsinfo('%s')";

static char	*qry_xids = 
"select trim(xa_dis_tran_id) from lgmo_xa_dis_tran_ids\
 where xa_database_name = '%s' and xa_seqnum = 0";

static char	*qry_param_ing_schema =
"select param_sequence, param_name, procedure_owner,\
 user_name, dba_name, system_owner\
 from iiproc_params, iidbconstants where procedure_name = ~V\
 and (procedure_owner = user_name or procedure_owner = dba_name\
 or procedure_owner = system_owner) order by param_sequence";

static char	*qry_param_ing = 
"select param_sequence, param_name, procedure_owner\
 from iiproc_params where procedure_name = ~V and procedure_owner = ~V\
 order by param_sequence";

static char	*qry_param_gtw_schema =
"select param_sequence, param_name, proc_owner,\
 user_name, dba_name, system_owner\
 from iigwprocparams, iidbconstants where proc_name = ~V\
 and (proc_owner = user_name or proc_owner = dba_name\
 or proc_owner = system_owner) order by param_sequence";

static char	*qry_param_gtw = 
"select param_sequence, param_name, proc_owner\
 from iigwprocparams where proc_name = ~V and proc_owner = ~V\
 order by param_sequence";


/*
** Temporary storage for XIDs
*/

typedef struct
{
    QUEUE			q;
    IIAPI_XA_DIS_TRAN_ID	xid;
} JDBC_XIDS;


/*
** Forward references.
*/
static	void	msg_req_sm( PTR );
static	bool	save_caps( JDBC_PCB * );
static	void	rqst_caps( JDBC_PCB * );
static	void	copy_caps( JDBC_PCB *, char *, char * );
static	bool	param_names( JDBC_PCB * );
static	bool	save_xids( JDBC_PCB * );
static	void	rqst_xids( JDBC_PCB * );
static	u_i2	column_length( IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE * );
static	u_i2	extract_string( IIAPI_DESCRIPTOR *, 
				IIAPI_DATAVALUE *, char *, u_i2 );
static	i4	extract_int( IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE * );
static	bool	parseXID( char *, IIAPI_XA_DIS_TRAN_ID * );



/*
** Name: jdbc_msg_request
**
** Description:
**	Process a JDBC request message.
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
**	17-Jan-00 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	25-Jul-00 (gordy)
**	    Added REQUEST sub-type for procedure parameter names.
**	    Added support for request parameters.  Generalized the
**	    processing similar to query processing.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	12-Oct-00 (gordy)
**	    Only check transaction state when query is required.
**	24-Oct-00 (gordy)
**	    Added support for gateway procedure catalogs.
**	 7-Mar-01 (gordy)
**	    Added support for dbmsinfo().
**	18-Apr-01 (gordy)
**	    Added support for prepared transaction IDs.
**	18-May-01 (gordy)
**	    Info ID and value storage now in CCB.
*/

void
jdbc_msg_request( JDBC_CCB *ccb )
{
    JDBC_PCB		*pcb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    STATUS		status;
    u_i2		req_id, p_rec = 0;
    u_i2		schema_name_len, proc_name_len; 
    u_i2		db_name_len, info_id_len;
    char		schema_name[ JDBC_NAME_MAX + 1 ];
    char		proc_name[ JDBC_NAME_MAX + 1 ];
    char		db_name[ JDBC_NAME_MAX + 1 ];
    bool		incomplete = FALSE;

    /*
    ** Read and validate the request type.
    */
    if ( ! jdbc_get_i2( ccb, (u_i1 *)&req_id ) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC no request ID\n", ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( req_id > JDBC_REQ_MAX )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay("%4d    JDBC invalid request ID: %d\n", ccb->id, req_id);
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC Request: %s\n", 
		   ccb->id, gcu_lookup( reqMap, req_id ) );

    /*
    ** Read the request parameters.
    */
    while( ccb->msg.msg_len )
    {
	JDBC_MSG_PM qp;

	incomplete = TRUE;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&qp.param_id ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&qp.val_len ) )  break;

	switch( qp.param_id )
	{
	case JDBC_RQP_SCHEMA_NAME :
	    if ( qp.val_len >= sizeof( schema_name )  ||
		 ! jdbc_get_bytes( ccb, qp.val_len, (u_i1 *)schema_name ) )
		break;
	    
	    schema_name[ qp.val_len ] = EOS;
	    schema_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= RQP_SCHM;
	    incomplete = FALSE;
	    break;

	case JDBC_RQP_PROC_NAME :
	    if ( qp.val_len >= sizeof( proc_name )  ||
		 ! jdbc_get_bytes(ccb, qp.val_len, (u_i1 *)proc_name) )
		break;
	    
	    proc_name[ qp.val_len ] = EOS;
	    proc_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= RQP_PNAM;
	    incomplete = FALSE;
	    break;

	case JDBC_RQP_INFO_ID :
	    if ( qp.val_len >= sizeof( ccb->rqst.rqst_id )  ||
		 ! jdbc_get_bytes(ccb, qp.val_len, (u_i1 *)ccb->rqst.rqst_id) )
		break;
	    
	    ccb->rqst.rqst_id[ qp.val_len ] = EOS;
	    info_id_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= RQP_INFO;
	    incomplete = FALSE;
	    break;

	case JDBC_RQP_DB_NAME :
	    if ( qp.val_len >= sizeof( db_name )  ||
		 ! jdbc_get_bytes(ccb, qp.val_len, (u_i1 *)db_name) )
		break;
	    
	    db_name[ qp.val_len ] = EOS;
	    db_name_len = qp.val_len;
	    if ( qp.val_len > 0 )  p_rec |= RQP_DB;
	    incomplete = FALSE;
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC     unkown parameter ID %d\n",
			   ccb->id, qp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete   ||
	 (p_rec & p_req[ req_id ]) != p_req[ req_id ]  ||
	 (p_rec & ~(p_req[ req_id ] | p_opt[ req_id ])) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    if ( incomplete )
		TRdisplay( "%4d    JDBC unable to read all request params %d\n",
			   ccb->id );
	    else
		TRdisplay( "%4d    JDBC invalid or missing request params %d\n",
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
    ccb->qry.api_parms = &ccb->qry.qry_parms;
    status = OK;

    switch( req_id )
    {
    case JDBC_REQ_CAPABILITY :
	if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) )
	    status = E_JD010A_PROTOCOL_ERROR;
	else
	{
	    ccb->sequence = REQ_CAPS;
	    pcb->data.query.type = IIAPI_QT_QUERY;
	    pcb->data.query.params = FALSE;
	    pcb->data.query.text = qry_caps;
	}
	break;

    case JDBC_REQ_PARAM_NAMES :
	if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) )
	    status = E_JD010A_PROTOCOL_ERROR;
	else
	{
	    /*
	    ** Set-up request parameters.  Initialize the
	    ** API service parameters for the procedure
	    ** and schema names.
	    */
	    ccb->sequence = REQ_PNAM;
	    ccb->qry.api_parms = &ccb->qry.svc_parms;
	    pcb->data.query.type = IIAPI_QT_QUERY;
	    pcb->data.query.params = TRUE;

	    if ( ! ccb->cib->gateway )
		if ( p_rec & RQP_SCHM )  
		    pcb->data.query.text = qry_param_ing;
		else
		    pcb->data.query.text = qry_param_ing_schema;
	    else  if ( p_rec & RQP_SCHM )  
		pcb->data.query.text = qry_param_gtw;
	    else
		pcb->data.query.text = qry_param_gtw_schema;

	    /*
	    ** Always allocate two parameters.  The second will
	    ** be used for the schema name, either provided as
	    ** a request parameter or resolved during parameter
	    ** processing.
	    */
	    if ( ! jdbc_alloc_qdesc( &ccb->qry.svc_parms, 2, NULL ) )
	    {
		jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		jdbc_del_pcb( pcb );
		return;
	    }
	
	    if  ( ccb->cib->db_name_case == DB_CASE_UPPER )
		CVupper( proc_name );
	    else  if  ( ccb->cib->db_name_case == DB_CASE_LOWER )
		CVlower( proc_name );

	    if ( ! (p_rec & RQP_SCHM) )  
		schema_name_len = JDBC_NAME_MAX + 1;
	    else  if  ( ccb->cib->db_name_case == DB_CASE_UPPER )
		CVupper( schema_name );
	    else  if  ( ccb->cib->db_name_case == DB_CASE_LOWER )
		CVlower( schema_name );

	    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
	    desc->ds_columnType = IIAPI_COL_QPARM;
	    desc->ds_dataType = IIAPI_CHA_TYPE;
	    desc->ds_length = proc_name_len;
	    desc->ds_nullable = FALSE;

	    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[1];
	    desc->ds_columnType = IIAPI_COL_QPARM;
	    desc->ds_dataType = IIAPI_CHA_TYPE;
	    desc->ds_length = schema_name_len;
	    desc->ds_nullable = FALSE;

	    if ( ! jdbc_alloc_qdata( &ccb->qry.svc_parms, 1 ) )
	    {
		jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		jdbc_del_pcb( pcb );
		return;
	    }
	
	    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
	    data->dv_null = FALSE;
	    data->dv_length = proc_name_len;
	    MEcopy( (PTR)proc_name, proc_name_len, data->dv_value );
	    ccb->qry.api_parms->col_cnt = 1;

	    if ( p_rec & RQP_SCHM )  
	    {
		data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[1];
		data->dv_null = FALSE;
		data->dv_length = schema_name_len;
		MEcopy( (PTR)schema_name, schema_name_len, data->dv_value );
		ccb->qry.api_parms->col_cnt++;
	    }
	}
	break;

    case JDBC_REQ_DBMSINFO :
	if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) )
	    status = E_JD010A_PROTOCOL_ERROR;
	else  if ( ! jdbc_expand_qtxt( ccb, STlength( qry_info ) + 
					    info_id_len, FALSE ) )
	    status = E_JD0108_NO_MEMORY;
	else
	{
	    ccb->sequence = REQ_INFO;
	    pcb->data.query.type = IIAPI_QT_QUERY;
	    pcb->data.query.params = FALSE;
	    STprintf( ccb->qry.qtxt, qry_info, ccb->rqst.rqst_id );
	    pcb->data.query.text = ccb->qry.qtxt;
	}
	break;

    case JDBC_REQ_2PC_XIDS :
	if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) )
	    status = E_JD010A_PROTOCOL_ERROR;
	else  if ( ! jdbc_expand_qtxt( ccb, STlength( qry_xids ) + 
					    db_name_len, FALSE ) )
	    status = E_JD0108_NO_MEMORY;
	else
	{
	    ccb->sequence = REQ_XIDS;
	    pcb->data.query.type = IIAPI_QT_QUERY;
	    pcb->data.query.params = FALSE;
	    STprintf( ccb->qry.qtxt, qry_xids, db_name );
	    pcb->data.query.text = ccb->qry.qtxt;
	}
	break;

    default : /* shouldn't happen, req_id checked above */
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay("%4d    JDBC invalid request ID: %d\n", ccb->id, req_id);
	status = E_JD010A_PROTOCOL_ERROR;
	break;
    }

    if ( status != OK )
    {
	jdbc_sess_error( ccb, &pcb->rcb, status );
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	return;
    }

    msg_req_sm( (PTR)pcb );
    return;
}


/*
** Name: msg_req_sm
**
** Description:
**	Issue a query to the server and load the information
**	from iidbcapabilities.
**
** Input:
**	arg	Parameter Control Block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
**	25-Jul-00 (gordy)
**	    Renamed and generalized to support additional requests.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	12-Oct-00 (gordy)
**	    Transaction handling localized in jdbc_xact_check().
**	    Only check transaction state when query is required.
**	 7-Mar-01 (gordy)
**	    Added support for dbmsinfo().
**	18-Apr-01 (gordy)
**	    Added support for prepared transaction IDs.
**	18-May-01 (gordy)
**	    Info ID and value storage now in CCB.
*/

static void
msg_req_sm( PTR arg )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    JDBC_CCB		*ccb = pcb->ccb;
    JDBC_SCB		*scb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;

  top:

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC request process seq %s\n",
		   ccb->id, gcu_lookup( reqSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case REQ_CAPS :		/* Begin DBMS capabilities request */
	/*
	** If DBMS capabilities already loaded, skip
	** directly to response processing.  Otherwise,
	** begin the capabilties query.
	*/
	if ( ! QUEUE_EMPTY( &ccb->cib->caps ) )
	{
	    ccb->sequence = REQ_CAPS_6;
	    break;
	}

	/*
	** Before issuing a query, make sure
	** the transaction state has been
	** properly established for the current
	** transaction mode and query type.
	*/
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_xact_check( pcb, JDBC_XQOP_NON_CURSOR );
	return;

    case REQ_CAPS_1 :		/* Issue query */
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_query( pcb );
	return;
    
    case REQ_CAPS_2 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_getDesc( pcb );
	return;

    case REQ_CAPS_3 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->result.fetch_limit = CAPS_ROW_MAX;

	if ( ! (scb = jdbc_new_stmt( ccb, pcb, TRUE )) )
	{
	    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	    jdbc_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_CAPS_4 :		/* Read next set of data */
	/*
	** Read next set of data.
	*/
	pcb->data.data.tot_row = 0;
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	jdbc_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & JDBC_MSG_XOFF) )
	    jdbc_api_getCol( pcb );
	else
	{
	    if ( JDBC_global.trace_level >= 4 )
		TRdisplay( "%4d    JDBC request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_CAPS_5 :		/* Process data results */
	/*
	** Extract and save DBMS capabilities (if any)
	** Loop until all capabilities retrieved, then
	** free statement resources.
	*/
	if ( pcb->data.data.row_cnt  &&  save_caps( pcb ) )
	{
	    ccb->sequence = REQ_CAPS_4;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_del_stmt( pcb, scb );
	return;

    case REQ_CAPS_6 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_caps( pcb );
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	ccb->sequence = REQ_DONE;
	break;

    case REQ_PNAM :		/* Begin parameter name request */
	/*
	** Before issuing a query, make sure
	** the transaction state has been
	** properly established for the current
	** transaction mode and query type.
	*/
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_xact_check( pcb, JDBC_XQOP_NON_CURSOR );
	return;

    case REQ_PNAM_1 :		/* Issue query */
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_query( pcb );
	return;
    
    case REQ_PNAM_2 :		/* Describe parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.desc.count = ccb->qry.api_parms->col_cnt;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_setDesc( pcb );
	return;
    
    case REQ_PNAM_3 :		/* Send parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = (IIAPI_DATAVALUE *)ccb->qry.api_parms->data;
	pcb->data.data.more_segments = FALSE;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_putParm( pcb );
	return;

    case REQ_PNAM_4 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_getDesc( pcb );
	return;

    case REQ_PNAM_5 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	/*
	** If the schema name is not known, we may require
	** upto three rows to distinguish between a user,
	** dba, or system procedure.
	*/
	pcb->result.fetch_limit = 3;

	if ( ! (scb = jdbc_new_stmt( ccb, pcb, TRUE )) )
	{
	    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	    jdbc_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_PNAM_6 :		/* Read next set of data */
	pcb->data.data.tot_row = 0;
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	jdbc_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & JDBC_MSG_XOFF) )
	    jdbc_api_getCol( pcb );
	else
	{
	    if ( JDBC_global.trace_level >= 4 )
		TRdisplay( "%4d    JDBC request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_PNAM_7 :		/* Process data results */
	if ( pcb->data.data.row_cnt  &&  param_names( pcb ) )
	{
	    ccb->sequence = REQ_PNAM_6;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_del_stmt( pcb, scb );
	return;

    case REQ_PNAM_8 :		/* End of query */
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	ccb->sequence = REQ_DONE;
	break;

    case REQ_INFO :		/* Begin DBMSINFO request */
	/*
	** Before issuing a query, make sure
	** the transaction state has been
	** properly established for the current
	** transaction mode and query type.
	*/
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_xact_check( pcb, JDBC_XQOP_NON_CURSOR );
	return;

    case REQ_INFO_1 :		/* Issue query */
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_query( pcb );
	return;
    
    case REQ_INFO_2 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_getDesc( pcb );
	return;

    case REQ_INFO_3 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->result.fetch_limit = 1;

	if ( ! (scb = jdbc_new_stmt( ccb, pcb, TRUE )) )
	{
	    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	    jdbc_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_INFO_4 :		/* Read info */
	/*
	** Read next set of data.
	*/
	pcb->data.data.tot_row = 0;
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	jdbc_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & JDBC_MSG_XOFF) )
	    jdbc_api_getCol( pcb );
	else
	{
	    if ( JDBC_global.trace_level >= 4 )
		TRdisplay( "%4d    JDBC request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_INFO_5 :		/* Process data results */
	/*
	** Extract and save DBMSINFO (if any).
	*/
	if ( ! pcb->data.data.row_cnt )  
	    ccb->rqst.rqst_val[ 0 ] = EOS;
	else
	    extract_string( (IIAPI_DESCRIPTOR *)pcb->scb->column.desc, 
			    (IIAPI_DATAVALUE *)pcb->scb->column.data, 
			    ccb->rqst.rqst_val, sizeof( ccb->rqst.rqst_val ) );
	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_del_stmt( pcb, scb );
	return;

    case REQ_INFO_6 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	{
	    u_i2 len = STlength( ccb->rqst.rqst_val );

	    jdbc_msg_begin( &pcb->rcb, JDBC_MSG_DATA, 0 );
	    jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	    jdbc_put_i2( &pcb->rcb, len );
	    jdbc_put_bytes( &pcb->rcb, len, (u_i1 *)ccb->rqst.rqst_val );
	    jdbc_msg_end( pcb->rcb, FALSE );

	    jdbc_send_done( ccb, &pcb->rcb, pcb );
	    ccb->sequence = REQ_DONE;
	}
	break;

    case REQ_XIDS :		/* Begin 2PC XIDS request */
	/*
	** Before issuing a query, make sure
	** the transaction state has been
	** properly established for the current
	** transaction mode and query type.
	*/
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_xact_check( pcb, JDBC_XQOP_NON_CURSOR );
	return;

    case REQ_XIDS_1 :		/* Issue query */
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_query( pcb );
	return;
    
    case REQ_XIDS_2 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_getDesc( pcb );
	return;

    case REQ_XIDS_3 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->result.fetch_limit = 1;

	if ( ! (scb = jdbc_new_stmt( ccb, pcb, TRUE )) )
	{
	    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	    jdbc_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_XIDS_4 :		/* Read info */
	/*
	** Read next set of data.
	*/
	pcb->data.data.tot_row = 0;
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	jdbc_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & JDBC_MSG_XOFF) )
	    jdbc_api_getCol( pcb );
	else
	{
	    if ( JDBC_global.trace_level >= 4 )
		TRdisplay( "%4d    JDBC request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_XIDS_5 :		/* Process data results */
	/*
	** Extract and save transaction IDs (if any).
	** Loop until all XIDs retrieved, then free 
	** statement resources.
	*/
	if ( pcb->data.data.row_cnt  &&  save_xids( pcb ) )
	{
	    ccb->sequence = REQ_XIDS_4;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_del_stmt( pcb, scb );
	return;

    case REQ_XIDS_6 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_xids( pcb );
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	ccb->sequence = REQ_DONE;
	break;

    case REQ_CANCEL :		/* Cancel the current query */
	/*
	** Flush input and cancel current statement.
	*/
	jdbc_flush_msg( ccb );
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_cancel( pcb );
	return;

    case REQ_CANCEL_1 :
	/*
	** Send the DONE message and close the current
	** statement with no further messaging from the
	** API layer.
	*/
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	if ( pcb->rcb )  
	{
	    jdbc_del_rcb( pcb->rcb );
	    pcb->rcb = NULL;	/* no messages from close() */
	}
	jdbc_push_callback( pcb, msg_req_sm );
	jdbc_api_close( pcb, NULL );	
	return;

    case REQ_DONE :		/* Query completed. */
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	return;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid request processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	jdbc_gcc_abort( ccb, E_JD0109_INTERNAL_ERROR );
	jdbc_del_pcb( pcb );
	return;
    }

    goto top;
}


/*
** Name: save_caps
**
** Description:
**	Save DBMS capabilities in the CIB CAPS structure.
**
** Input:
**	pcb	Parameter Control Block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if successfull, FALSE if memory allocation error.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
**	25-Jul-00 (gordy)
**	    Extracted column handling code as utility functions.
**	24-Oct-00 (gordy)
**	    Flag gateways to modify server operations.
*/

static bool
save_caps( JDBC_PCB *pcb )
{
    JDBC_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    JDBC_CAPS		*caps;
    u_i2		i, row, col, len;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 2 )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid caps info (%d,%d)\n", ccb->id, 
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( TRUE );
    }

    /*
    ** Sum the lengths of the column values (including
    ** the NULL terminator).
    */
    for( row = len = i = 0; row < pcb->data.data.row_cnt; row++ )
    {
	len += column_length( &desc[ 0 ], &data[ i++ ] ) + 1;
	len += column_length( &desc[ 1 ], &data[ i++ ] ) + 1;
    }

    if ( ! (caps = (JDBC_CAPS *)
	    MEreqmem( 0, sizeof(JDBC_CAPS) + (sizeof(CAPS_INFO) * 
			 (pcb->data.data.row_cnt - 1)), FALSE, NULL ))  ||
         ! (caps->data = MEreqmem( 0, len, FALSE, NULL )) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC error allocating caps info\n", ccb->id );
	if ( caps )  MEfree( (PTR)caps );
	return( FALSE );
    }

    caps->cap_cnt = pcb->data.data.row_cnt;

    for( row = len = i = 0; row < caps->cap_cnt; row++ )
    {
	caps->caps[ row ].name = &caps->data[ len ];
	len += extract_string( &desc[0], &data[i++], &caps->data[len], 0 ) + 1;

	caps->caps[ row ].value = &caps->data[ len ];
	len += extract_string( &desc[1], &data[i++], &caps->data[len], 0 ) + 1;

	/*
	** Look for interesting capabilities which we need to
	** tune the operation of the server for the current
	** connection.
	*/
	if ( STcompare( caps->caps[ row ].name, "DB_NAME_CASE" ) == 0 )
	    if ( STcompare( caps->caps[ row ].value, "UPPER" ) == 0 )
		ccb->cib->db_name_case = DB_CASE_UPPER;
	    else  if ( STcompare( caps->caps[ row ].value, "LOWER" ) == 0 )
		ccb->cib->db_name_case = DB_CASE_LOWER;
	    else  if ( STcompare( caps->caps[ row ].value, "MIXED" ) == 0 )
		ccb->cib->db_name_case = DB_CASE_MIXED;

	if ( STcompare( caps->caps[ row ].name, "DBMS_TYPE" ) == 0  &&
	     ! (STcompare( caps->caps[ row ].value, "INGRES" ) == 0) )
	    ccb->cib->gateway = TRUE;
    }

    QUinsert( &caps->q, ccb->cib->caps.q_prev );

    return( TRUE );
}


/*
** Name: rqst_caps
**
** Description:
**	Format the response to the DBMS capabilities request.
**
** Input:
**	pcb	Parameter Control Block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
**	18-May-01 (gordy)
**	    Generate JDBC Server specific capabilities.
*/

static void
rqst_caps( JDBC_PCB *pcb )
{
    JDBC_CCB	*ccb = pcb->ccb;
    QUEUE	*q;
    char	value[ 33 ];

    jdbc_msg_begin( &pcb->rcb, JDBC_MSG_DATA, 0 );
    
    for( q = ccb->cib->caps.q_next; q != &ccb->cib->caps; q = q->q_next )
    {
	JDBC_CAPS	*caps = (JDBC_CAPS *)q;
	u_i2		i;

	for( i = 0; i < caps->cap_cnt; i++ )
	    copy_caps( pcb, caps->caps[ i ].name, caps->caps[ i ].value );
    }

    /*
    ** Generate JDBC server capabilities.
    */
    CVla( ccb->cib->level, value );
    copy_caps( pcb, "JDBC_CONNECT_LEVEL", value );

    jdbc_msg_end( pcb->rcb, FALSE );
    return;
}


/*
** Name: copy_caps
**
** Description:
**	Copy capability name and value into data message as
**	column values.
**
** Input:
**	pcb	Parameter Control Block.
**	name	Capability name.
**	value	Capability value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-May-01 (gordy)
**	    Created.
*/

static void
copy_caps( JDBC_PCB *pcb, char *name, char *value )
{
    u_i2 len;
    
    len = STlength( name );
    jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
    jdbc_put_i2( &pcb->rcb, len );
    jdbc_put_bytes( &pcb->rcb, len, (u_i1 *)name );

    len = STlength( value );
    jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
    jdbc_put_i2( &pcb->rcb, len );
    jdbc_put_bytes( &pcb->rcb, len, (u_i1 *)value );

    return;
}


/*
** Name: param_names
**
** Description:
**	Process the results of the parameter names query.
**	If a schema name has not been resolved, the first
**	set of data will be used to resolve the schema.
**	Builds a single row with a single column for each 
**	parameter who's procedure name and owner matches 
**	the request info.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if successful, FALSE if memory allocation failure.
**
** History:
**	25-Jul-00 (gordy)
**	    Created.
*/

static bool
param_names( JDBC_PCB *pcb )
{
    JDBC_CCB		*ccb = pcb->ccb;
    i4			row, i;
    char		name[ JDBC_NAME_MAX + 1 ];
    char		owner[ JDBC_NAME_MAX + 1 ];
    char		schema[ JDBC_NAME_MAX + 1 ];
    IIAPI_DESCRIPTOR	*ddesc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*ddata = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    IIAPI_DATAVALUE	*pdata = 
			    &((IIAPI_DATAVALUE *)ccb->qry.api_parms->data)[1];

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt < 3 )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid param info (%d,%d)\n", pcb->ccb->id,
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( TRUE );
    }

    if ( ccb->qry.api_parms->col_cnt == 2 )
    {
	/*
	** Get the target schema name.
	*/
	if ( pdata->dv_length )  
	    MEcopy( pdata->dv_value, pdata->dv_length, (PTR)schema );
	schema[ pdata->dv_length ] = EOS;
    }
    else
    {
	/*
	** Resolve the target schema name.
	*/
	i4	seq, level;

	if ( ! ccb->cib->usr )
	{
	    if ( pcb->data.data.col_cnt < 6 )
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC invalid const info (%d,%d)\n", 
				pcb->ccb->id, pcb->data.data.row_cnt, 
				pcb->data.data.col_cnt );
		return( TRUE );
	    }

	    extract_string( &ddesc[3], &ddata[3], name, sizeof( name ) );
	    ccb->cib->usr = STalloc( name );
	    extract_string( &ddesc[4], &ddata[4], name, sizeof( name ) );
	    ccb->cib->dba = STalloc( name );
	    extract_string( &ddesc[5], &ddata[5], name, sizeof( name ) );
	    ccb->cib->sys = STalloc( name );

	    if ( ! ccb->cib->usr  ||  ! ccb->cib->dba  ||  ! ccb->cib->sys )
	    {
		if ( ccb->cib->usr )  MEfree( ccb->cib->usr );
		if ( ccb->cib->dba )  MEfree( ccb->cib->dba );
		if ( ccb->cib->sys )  MEfree( ccb->cib->sys );
		ccb->cib->usr = ccb->cib->dba = ccb->cib->sys = NULL;

		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC mem alloc failed: DB constants\n",
				ccb->id );
		return( FALSE );
	    }
	}

	schema[0] = EOS;

	for( 
	     row = i = 0, level = 4; 
	     row < pcb->data.data.row_cnt  &&  level > 1; 
	     row++, i += pcb->data.data.col_cnt 
	   )
	{
	    seq = extract_int( &ddesc[0], &ddata[i] );
	    if ( seq != 1 )  break;
	    extract_string( &ddesc[2], &ddata[i+2], owner, sizeof( owner ) );

	    switch( level )
	    {
	    case 4 :
		if ( STcompare( owner, ccb->cib->sys ) == 0 )
		{
		    STcopy( owner, schema );
		    level = 3;
	   	}

	    case 3 :
		if ( STcompare( owner, ccb->cib->dba ) == 0 )
		{
		    STcopy( owner, schema );
		    level = 2;
	   	}
	    
	    case 2 :
		if ( STcompare( owner, ccb->cib->usr ) == 0 )
		{
		    STcopy( owner, schema );
		    level = 1;
	   	}

	    case 1 :
		break;
	    }
	}

	/*
	** Save schema name if resolved in case there are
	** additional parameters beyond the current set.
	** Otherwise, return no rows.
	*/
	pdata->dv_length = STlength( schema );

	if ( ! pdata->dv_length )
	    return( TRUE );
	else
	{
	    MEcopy( (PTR)schema, pdata->dv_length, pdata->dv_value );
	    ccb->qry.api_parms->col_cnt = 2;
	}
    }

    for( 
	 row = i = 0; 
	 row < pcb->data.data.row_cnt; 
	 row++, i += pcb->data.data.col_cnt 
       )
    {
	extract_string( &ddesc[1], &ddata[i+1], name, sizeof( name ) );
	extract_string( &ddesc[2], &ddata[i+2], owner, sizeof( owner ) );

	if ( STcompare( owner, schema ) == 0 )
	{
	    u_i2 len = STlength( name );

	    /*
	    ** Its a little perverse to place each parameter
	    ** name in its own DATA message, but they will
	    ** all be concatenated in the same buffer so it
	    ** shouldn't cause any problems.  This way we
	    ** don't need to track the message state.
	    */
	    jdbc_msg_begin( &pcb->rcb, JDBC_MSG_DATA, 0 );
	    jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	    jdbc_put_i2( &pcb->rcb, len );
	    jdbc_put_bytes( &pcb->rcb, len, (u_i1 *)name );
	    jdbc_msg_end( pcb->rcb, FALSE );
	}
    }

    return( TRUE );
}


/*
** Name: save_xids
**
** Description:
**	Save transaction IDs in the CCB.
**
** Input:
**	pcb	Parameter Control Block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if successfull, FALSE if memory allocation error.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

static bool
save_xids( JDBC_PCB *pcb )
{
    JDBC_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    u_i2		row;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 1 )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid xid info (%d,%d)\n", ccb->id, 
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( TRUE );
    }

    for( row = 0; row < pcb->data.data.row_cnt; row++, data++ )
    {
	JDBC_XIDS	*info;
	char		str[ 512 ];
	u_i2		len;

	if ( ! (info = (JDBC_XIDS *)MEreqmem( 0, sizeof( JDBC_XIDS ), 
					      FALSE, NULL )) )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay("%4d    JDBC error allocating XID info\n", ccb->id);
	    return( FALSE );
	}

	if ( column_length( desc, data ) >= sizeof( str ) )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC Invalid XID length!\n", ccb->id );
	    continue;
	}
	
	extract_string( desc, data, str, sizeof( str ) );

	if ( ! parseXID( str, &info->xid ) )  
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC Invalid XID format!\n", ccb->id );
	    continue;
	}

	QUinsert( &info->q, ccb->rqst.rqst_q.q_prev );
    }

    return( TRUE );
}



/*
** Name: rqst_xids
**
** Description:
**	Format the response to the XIDs request.
**
** Input:
**	pcb	Parameter Control Block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

static void
rqst_xids( JDBC_PCB *pcb )
{
    JDBC_CCB	*ccb = pcb->ccb;
    QUEUE	*q;

    if ( QUEUE_EMPTY( &ccb->rqst.rqst_q ) )  return;
    jdbc_msg_begin( &pcb->rcb, JDBC_MSG_DATA, 0 );

    for( 
	 q = ccb->rqst.rqst_q.q_next; 
	 q != &ccb->rqst.rqst_q; 
	 q = ccb->rqst.rqst_q.q_next 
       )
    {
	IIAPI_XA_DIS_TRAN_ID	*xid = &((JDBC_XIDS *)q)->xid;

	jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	jdbc_put_i4( &pcb->rcb, xid->xa_tranID.xt_formatID );

	jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	jdbc_put_i2( &pcb->rcb, (i2)xid->xa_tranID.xt_gtridLength );
	jdbc_put_bytes( &pcb->rcb, (i2)xid->xa_tranID.xt_gtridLength, 
			(u_i1 *)&xid->xa_tranID.xt_data[ 0 ] );

	jdbc_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	jdbc_put_i2( &pcb->rcb, (i2)xid->xa_tranID.xt_bqualLength );
	jdbc_put_bytes( &pcb->rcb, (i2)xid->xa_tranID.xt_bqualLength, 
			(u_i1 *)&xid->xa_tranID.xt_data[
				    xid->xa_tranID.xt_gtridLength ] );
	QUremove( q );
	MEfree( (PTR)q );
    }

    jdbc_msg_end( pcb->rcb, FALSE );
    return;
}


/*
** Name: column_length
**
** Description:
**	Returns the length of a column.  For strings, this is
**	the actual length of data string (not including any
**	NULL terminator).  For all other data types, this is 
**	the descriptor length.
**
** Input:
**	desc	API Descriptor.
**	data	API Data value.
**
** Output:
**	None.
**
** Returns:
**	u_i2	Length of column.
**
** History:
**	25-Jul-00 (gordy)
**	    Created.
*/

static u_i2
column_length( IIAPI_DESCRIPTOR *desc, IIAPI_DATAVALUE *data )
{
    u_i2 len;

    switch( desc->ds_dataType )
    {
    case IIAPI_CHA_TYPE :
    case IIAPI_CHR_TYPE :
	len = data->dv_null ? 0 : desc->ds_length;
	break;

    case IIAPI_TXT_TYPE :
    case IIAPI_LTXT_TYPE :
    case IIAPI_VCH_TYPE :
	if ( data->dv_null  ||  data->dv_length < 2 )
	    len = 0;
	else
	    MEcopy( data->dv_value, 2, (PTR)&len );
	break;

    default :
	len = desc->ds_length;
	break;
    }

    return( len );
}


/*
** Name: extract_string
**
** Description:
**	Extracts a string column into a provided buffer.
**	The extracted string is NULL terminated.  If the
**	length of the buffer is provided, the string will
**	be truncated to fit.  Otherwise, the buffer must 
**	be as big as the length returned by column_length() 
**	plus room for the NULL terminator.  The buffered 
**	string is trimmed before returning.
**
**	The length of the string (not including the NULL
**	terminator) is returned.  This will be no greater
**	than the length returned by column_length(), but
**	may be less due to trimming.
**
** Input:
**	desc	API descriptor of column.
**	data	API data value for column.
**	length	Length of output buffer or 0.
**
** Output:
**	str	Buffer to receive extracted string.
**
** Returns:
**	u_i2	String length.
**
** History:
**	25-Jul-00 (gordy)
**	    Created.
*/

static u_i2
extract_string
( 
    IIAPI_DESCRIPTOR	*desc, 
    IIAPI_DATAVALUE	*data, 
    char		*str,
    u_i2		length
)
{
    static char empty[] = "";
    char	*ptr = empty;
    u_i2	len = 0;

    switch( desc->ds_dataType )
    {
    case IIAPI_CHA_TYPE :
    case IIAPI_CHR_TYPE :
	if ( ! data->dv_null )
	{
	    len = desc->ds_length;
	    ptr = (char *)data->dv_value;
	}
	break;

    case IIAPI_TXT_TYPE :
    case IIAPI_LTXT_TYPE :
    case IIAPI_VCH_TYPE :
	if ( ! data->dv_null  &&  data->dv_length >= 2 )
	{
	    MEcopy( data->dv_value, 2, (PTR)&len );
	    ptr = (char *)data->dv_value + 2;
	}
	break;
    }

    if ( length > 0  &&  len >= length )  len = length - 1;
    if ( len )  MEcopy( (PTR)ptr, len, (PTR)str );
    str[ len ] = EOS;
    return( STtrmwhite( str ) );
}


/*
** Name: extract_int
**
** Description:
**	Extracts an integer column.
**
** Input:
**	desc	API descriptor of column.
**	data	API data value for column.
**
** Output:
**	None.
**
** Returns:
**	i4	Integer value.
**
** History:
**	25-Jul-00 (gordy)
**	    Created.
**	18-May-01 (gordy)
**	    Added length limit.
*/

static i4
extract_int( IIAPI_DESCRIPTOR *desc, IIAPI_DATAVALUE *data )
{
    i4 val = 0;
    i2 i2val;

    if ( ! data->dv_null )
	switch( desc->ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( data->dv_length )
	    {
	    case 1 :
		val = *(i1 *)data->dv_value;
		break;

	    case 2 :
		MEcopy( data->dv_value, 2, (PTR)&i2val );
		val = i2val;
		break;

	    case 4 :
		MEcopy( data->dv_value, 4, (PTR)&val );
		break;
	    }
	}

    return( val );
}


/*
** Name: parseXID
**
** Description:
**	Parses XA distributed transaction ID from text based
**	on algorithm implemented in IICXconv_to_struct_xa_xid()
**	in libqxa.
**
**	The XID text is modified during the parse.
**
** Input:
**	str	Formatted XA XID.
**
** Output:
**	xid	XA Distributed Transaction ID.
**
** Returns:
**	bool	TRUE if successful, FALSE if invalid format.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

static bool
parseXID( char *str, IIAPI_XA_DIS_TRAN_ID *xid )
{
    IIAPI_XA_TRAN_ID	*xp = &xid->xa_tranID;
    char		*cp;
    u_i1		*data;
    i4			num, count;
    u_i4		unum;

    if ( (cp = STindex( str, ERx(":"), 0 )) == NULL )  return( FALSE );
    *cp = EOS;
    if ( CVuahxl( str, &unum ) != OK )  return( FALSE );
    str = ++cp;
    xp->xt_formatID = unum;

    if ( (cp = STindex( str, ERx(":"), 0 )) == NULL )  return( FALSE );
    *cp = EOS;
    if ( CVal( str, &num ) != OK )  return( FALSE );
    str = ++cp;
    xp->xt_gtridLength = num;

    if ( (cp = STindex( str, ERx(":"), 0 )) == NULL )  return( FALSE );
    *cp = EOS;
    if ( CVal( str, &num ) != OK )  return( FALSE );
    str = ++cp;
    xp->xt_bqualLength = num;

    data = (u_i1 *)xp->xt_data;
    count = (xp->xt_gtridLength + xp->xt_bqualLength + sizeof( i4 ) - 1) /
	    sizeof( i4 );
    
    while( count-- )
    {
	if ( (cp = STindex( str, ERx(":"), 0 )) == NULL )  return( FALSE );
	*cp = EOS;
	if ( CVuahxl( str, &unum ) != OK )  return( FALSE );
	str = ++cp;

	*data++ = (u_i1)(unum >> 24);
	*data++ = (u_i1)(unum >> 16);
	*data++ = (u_i1)(unum >> 8);
	*data++ = (u_i1)unum;
    }

    return( TRUE );
}


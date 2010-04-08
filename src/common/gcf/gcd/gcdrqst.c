/*
** Copyright (c) 2004, 2009 Ingres Corporation
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
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdrqst.c
**
** Description:
**	GCD request message processing.
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	23-Dec-02 (gordy)
**	    Connection level capability ID is protocol level dependent.
**	15-Jan-03 (gordy)
**	    Added gcd_sess_abort().  Rename gcd_flush_msg() to gcd_msg_flush().
**	 4-Aug-03 (gordy)
**	    Allocate query data buffers.
**	 2-Jun-05 (gordy)
**	    Changed CIB values to flags.  Check for connection and
**	    transaction aborts.
**	11-Oct-05 (gordy)
**	    Check for API logical sequence errors.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	28-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	29-Jun-06 (lunbr01)  Bug 115970
**	    Skip calling API cancel or close on a failed request when
**	    the connection has been aborted.  This will prevent potential
**	    access violation or logic error due to cancel/close accessing 
**	    deleted API handles for the session.
**	18-Jul-06 (gordy)
**	    Maintain consistent transaction state.
**	25-Aug-06 (gordy)
**	    Add request for database procedure parameter information.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	23-Mar-09 (gordy)
**	    Support for long database object names.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
*/	

static	GCULIST	reqMap[] =
{
    { MSG_REQ_CAPABILITY,	"DBMS Capabilities" },
    { MSG_REQ_PARAM_NAMES,	"Parameter Names" },
    { MSG_REQ_DBMSINFO,		"DbmsInfo()" },
    { MSG_REQ_2PC_XIDS,		"2PC Transaction IDs" },
    { MSG_REQ_PARAM_INFO,	"Parameter Info" },
    { 0, NULL }
};

#define	REQ_CAPS	0	/* DBMS Capabilities */
#define	REQ_CAPS_1	1
#define	REQ_CAPS_2	2
#define	REQ_CAPS_3	3
#define	REQ_CAPS_4	4
#define	REQ_CAPS_DATA	5
#define	REQ_INFO	10
#define	REQ_INFO_1	11
#define	REQ_INFO_2	12
#define	REQ_INFO_3	13
#define	REQ_INFO_4	14
#define	REQ_POWN	20
#define	REQ_POWN_1	21
#define	REQ_POWN_2	22
#define	REQ_POWN_3	23
#define	REQ_POWN_4	24
#define	REQ_POWN_5	25
#define	REQ_POWN_6	26
#define	REQ_PNAME	30
#define	REQ_PNAM_1	31
#define	REQ_PNAM_2	32
#define	REQ_PNAM_3	33
#define	REQ_PNAM_4	34
#define	REQ_PNAM_5	35
#define	REQ_PNAM_6	36
#define	REQ_PNAM_7	37
#define	REQ_PINFO	40
#define	REQ_PINF_1	41
#define	REQ_PINF_2	42
#define	REQ_PINF_3	43
#define	REQ_PINF_4	44
#define	REQ_PINF_5	45
#define	REQ_PINF_6	46
#define	REQ_PINF_7	47
#define	REQ_XIDS	50
#define	REQ_XIDS_1	51
#define	REQ_XIDS_2	52
#define	REQ_XIDS_3	53
#define	REQ_XIDS_4	54
#define	REQ_XIDS_5	55
#define	REQ_CANCEL	96	/* Cancel request */
#define	REQ_CLOSE	97
#define	REQ_CHECK	98
#define	REQ_DONE	99	/* Request completed */

static	GCULIST reqSeqMap[] =
{
    { REQ_CAPS,		"CAPS 0" },
    { REQ_CAPS_1,	"CAPS 1" },
    { REQ_CAPS_2,	"CAPS 2" },
    { REQ_CAPS_3,	"CAPS 3" },
    { REQ_CAPS_4,	"CAPS 4" },
    { REQ_CAPS_DATA,	"CAPS 5" },
    { REQ_INFO,		"INFO 0" },
    { REQ_INFO_1,	"INFO 1" },
    { REQ_INFO_2,	"INFO 2" },
    { REQ_INFO_3,	"INFO 3" },
    { REQ_INFO_4,	"INFO 4" },
    { REQ_POWN,		"POWN 0" },
    { REQ_POWN_1,	"POWN 1" },
    { REQ_POWN_2,	"POWN 2" },
    { REQ_POWN_3,	"POWN 3" },
    { REQ_POWN_4,	"POWN 4" },
    { REQ_POWN_5,	"POWN 5" },
    { REQ_POWN_6,	"POWN 6" },
    { REQ_PNAME,	"PNAM 0" },
    { REQ_PNAM_1,	"PNAM 1" },
    { REQ_PNAM_2,	"PNAM 2" },
    { REQ_PNAM_3,	"PNAM 3" },
    { REQ_PNAM_4,	"PNAM 4" },
    { REQ_PNAM_5,	"PNAM 5" },
    { REQ_PNAM_6,	"PNAM 6" },
    { REQ_PNAM_7,	"PNAM 7" },
    { REQ_PINFO,	"PINF 0" },
    { REQ_PINF_1,	"PINF 1" },
    { REQ_PINF_2,	"PINF 2" },
    { REQ_PINF_3,	"PINF 3" },
    { REQ_PINF_4,	"PINF 4" },
    { REQ_PINF_5,	"PINF 5" },
    { REQ_PINF_6,	"PINF 6" },
    { REQ_PINF_7,	"PINF 7" },
    { REQ_XIDS,		"XIDS 0" },
    { REQ_XIDS_1,	"XIDS 1" },
    { REQ_XIDS_2,	"XIDS 2" },
    { REQ_XIDS_3,	"XIDS 3" },
    { REQ_XIDS_4,	"XIDS 4" },
    { REQ_XIDS_5,	"XIDS 5" },
    { REQ_CANCEL,	"CANCEL" },
    { REQ_CLOSE,	"CLOSE" },
    { REQ_CHECK,	"CHECK" },
    { REQ_DONE,		"DONE" },
    { 0, NULL }
};

/*
** Parameter flags and request required/optional parameters.
*/
#define	RQP_SCHEMA	0x01
#define	RQP_PNAME	0x02
#define	RQP_INFO	0x04
#define	RQP_DB		0x08

static	u_i2	p_req[] =
{
    0,
    0,
    RQP_PNAME,
    RQP_INFO,
    RQP_DB,
    RQP_PNAME,
};

static	u_i2	p_opt[] = 
{
    0,
    0,
    RQP_SCHEMA,
    0,
    0,
    RQP_SCHEMA,
};


#define	CAPS_ROW_MAX		30

static	char	*qry_caps = 
    "select cap_capability, cap_value from iidbcapabilities";

static char	*qry_info = "select dbmsinfo('%s')";

static char	*qry_xids = 
"select trim(xa_dis_tran_id) from lgmo_xa_dis_tran_ids\
 where xa_database_name = '%s' and xa_seqnum = 0";

static char	*qry_proc_owner_ing =
"select procedure_owner, user_name, dba_name, system_owner\
 from iiprocedures, iidbconstants\
 where procedure_name = ~V and text_sequence = 1 and\
 (procedure_owner = user_name or procedure_owner = dba_name or\
  procedure_owner = system_owner)";

static char	*qry_proc_owner_gtw =
"select proc_owner, user_name, dba_name, system_owner\
 from iigwprocedures, iidbconstants\
 where proc_name = ~V and text_sequence = 1 and\
 (proc_owner = user_name or proc_owner = dba_name or\
  proc_owner = system_owner)";

static char	*qry_param_name_ing = 
"select param_sequence, param_name from iiproc_params\
 where procedure_owner = ~V and procedure_name = ~V order by param_sequence";

static char	*qry_param_name_gtw = 
"select param_sequence, param_name from iigwprocparams\
 where proc_owner = ~V and proc_name = ~V order by param_sequence";

static char	*qry_param_info_ing = 
"select param_sequence, param_name, param_datatype_code,\
 param_length, param_scale, param_nulls from iiproc_params\
 where procedure_owner = ~V and procedure_name = ~V order by param_sequence";

static char	*qry_param_info_gtw = 
"select param_sequence, param_name, param_type,\
 param_length, param_scale, param_null from iigwprocparams\
 where proc_owner = ~V and proc_name = ~V order by param_sequence";


/*
** Temporary storage for query results
*/

typedef struct
{
    QUEUE	q;
    char	name[ 1 ];	/* Variable length */
} RQST_PNAME;

typedef struct
{
    QUEUE	q;
    i4		type;
    i4		length;
    i4		scale;
    char	nulls[ 2 ];
    char	name[ 1 ];	/* Variable length */
} RQST_PINFO;

typedef struct
{
    QUEUE			q;
    IIAPI_XA_DIS_TRAN_ID	xid;
} RQST_XIDS;


/*
** Forward references.
*/
static	void	msg_req_sm( PTR );
static	bool	save_caps( GCD_PCB * );
static	void	rqst_caps( GCD_PCB * );
static	void	copy_caps( GCD_PCB *, char *, char * );
static	bool	proc_owner_init( GCD_PCB * );
static	bool	proc_owner( GCD_PCB * );
static	bool	param_query_init( GCD_PCB * );
static	bool	save_pname( GCD_PCB * );
static	void	rqst_pname( GCD_PCB * );
static	bool	save_pinfo( GCD_PCB * );
static	void	rqst_pinfo( GCD_PCB * );
static	bool	save_xids( GCD_PCB * );
static	void	rqst_xids( GCD_PCB * );
static	u_i2	column_length( IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE * );
static	u_i2	extract_string( IIAPI_DESCRIPTOR *, 
				IIAPI_DATAVALUE *, char *, u_i2 );
static	i4	extract_int( IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE * );
static	bool	parseXID( char *, IIAPI_XA_DIS_TRAN_ID * );



/*
** Name: gcd_msg_request
**
** Description:
**	Process a GCD request message.
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
**	 2-Jun-05 (gordy)
**	    Gateways and case indicators changed to flag bits.
**	28-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	18-Jul-06 (gordy)
**	    Check initial transaction state and flag if reset needed.
**	25-Aug-06 (gordy)
**	    Add request for database procedure parameter information.
**	23-Mar-09 (gordy)
**	    Support for long database object names.  Use local buffer
**	    for more common short names.  Allcoate buffer when length
**	    exceeds local buffer size.  Mutually exclusive request
**	    parameters now share buffers to reduce code duplication.
*/

void
gcd_msg_request( GCD_CCB *ccb )
{
    GCD_PCB		*pcb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    STATUS		status;
    u_i2		req_id, p_rec = 0;
    u_i2		p1_len;
    char		p1_buff[ GCD_NAME_MAX + 1 ];
    char		*p1 = NULL;
    char		*p2 = NULL;
    bool		incomplete = FALSE;
    bool		xact_check = TRUE;

    /*
    ** Free any resources from prior request.
    */
    if ( ccb->rqst.rqst_id0 )  
    {
	MEfree( (PTR)ccb->rqst.rqst_id0 );
	ccb->rqst.rqst_id0 = NULL;
    }

    if ( ccb->rqst.rqst_id1 )  
    {
	MEfree( (PTR)ccb->rqst.rqst_id1 );
	ccb->rqst.rqst_id1 = NULL;
    }

    /*
    ** Read and validate the request type.
    */
    if ( ! gcd_get_i2( ccb, (u_i1 *)&req_id ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no request ID\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( req_id > DAM_ML_REQ_MAX )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay("%4d    GCD invalid request ID: %d\n", ccb->id, req_id);
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	goto cleanup;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Request operation: %s\n", 
		   ccb->id, gcu_lookup( reqMap, req_id ) );

    /*
    ** Read the request parameters.
    */
    while( ccb->msg.msg_len )
    {
	DAM_ML_PM qp;

	incomplete = TRUE;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&qp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&qp.val_len ) )  break;

	switch( qp.param_id )
	{
	case MSG_RQP_SCHEMA_NAME :
	    /*
	    ** This parameter must be saved for later processing
	    ** and is best handled dynamically and may occur in
	    ** combination with procedure name.
	    */
	    if ( 
	         p2  ||
		 ! (p2 = (char *)MEreqmem(0, qp.val_len + 1, FALSE, NULL))  ||
		 ! gcd_get_bytes( ccb, qp.val_len, (u_i1 *)p2 ) 
	       )
		break;

	    p2[ qp.val_len ] = EOS;
	    if ( qp.val_len > 0 )  p_rec |= RQP_SCHEMA;
	    incomplete = FALSE;
	    break;

	case MSG_RQP_PROC_NAME :
	    /*
	    ** This parameter must be saved for later processing
	    ** and is best handled dynamically and only occurs in
	    ** combination with schema name.
	    */
	    if ( 
	         p1  ||  
	    	 ! (p1 = (char *)MEreqmem(0, qp.val_len + 1, FALSE, NULL))  ||
	         ! gcd_get_bytes( ccb, qp.val_len, (u_i1 *)p1 )
	       )
	        break;

	    p1[ qp.val_len ] = EOS;
	    if ( qp.val_len > 0 )  p_rec |= RQP_PNAME;
	    incomplete = FALSE;
	    break;

	case MSG_RQP_INFO_ID :
	case MSG_RQP_DB_NAME :
	    /*
	    ** These parameters are only used locally and can be 
	    ** handled more efficiently when the value is short 
	    ** (common case).  They do not occur in combination
	    ** with any other parameter.
	    */
	    if ( p1 )  break;	/* Already set?  Should not happen */
	    p1_len = qp.val_len;

	    if ( p1_len < sizeof( p1_buff ) )	/* Use available buffer? */
		p1 = p1_buff;	
	    else  if ( ! (p1 = (char *)MEreqmem(0, p1_len + 1, FALSE, NULL)) )
		break;

	    if ( ! gcd_get_bytes( ccb, p1_len, (u_i1 *)p1 ) )  break;
	    p1[ p1_len ] = EOS;
	    incomplete = FALSE;

	    if ( p1_len > 0 )  
		switch( qp.param_id )
		{
		case MSG_RQP_INFO_ID :	  p_rec |= RQP_INFO;	break;
		case MSG_RQP_DB_NAME :    p_rec |= RQP_DB;	break;
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

    if ( incomplete   ||
	 (p_rec & p_req[ req_id ]) != p_req[ req_id ]  ||
	 (p_rec & ~(p_req[ req_id ] | p_opt[ req_id ])) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    if ( incomplete )
		TRdisplay( "%4d    GCD unable to read all request params %d\n",
			   ccb->id );
	    else
		TRdisplay( "%4d    GCD invalid or missing request params %d\n",
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
    ccb->rqst.req_id = req_id;
    ccb->qry.api_parms = &ccb->qry.qry_parms;
    status = OK;

    switch( req_id )
    {
    case MSG_REQ_CAPABILITY :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	/*
	** If DBMS capabilities already loaded, skip
	** directly to response processing.  Otherwise,
	** begin the capabilties query.
	*/
	if ( QUEUE_EMPTY( &ccb->cib->caps ) )
	{
	    ccb->sequence = REQ_CAPS;
	    pcb->data.query.type = IIAPI_QT_QUERY;
	    pcb->data.query.params = FALSE;
	    pcb->data.query.text = qry_caps;
	}
	else
	{
	    ccb->sequence = REQ_CAPS_DATA;
	    xact_check = FALSE;
	}
	break;

    case MSG_REQ_PARAM_NAMES :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	/*
	** Save procedure name in rqst_id1.
	*/
	if  ( GCD_IS_UPPER( ccb->cib->flags ) )
	    CVupper( p1 );
	else  if  ( GCD_IS_LOWER( ccb->cib->flags ) )
	    CVlower( p1 );

	ccb->rqst.rqst_id1 = p1;
	p1 = NULL;

	/*
	** If schema name is provided, query parameter names
	** directly.  Otherwise, appropriate schema name must
	** be resolved by precedence: user, dba, system.
	*/
	if ( p_rec & RQP_SCHEMA )  
	{
	    /*
	    ** Query set-up is deferred to allow shared usage 
	    ** after alternate (schema resolution) path.
	    */
	    ccb->sequence = REQ_PNAME;

	    if  ( GCD_IS_UPPER( ccb->cib->flags ) )
		CVupper( p2 );
	    else  if  ( GCD_IS_LOWER( ccb->cib->flags ) )
		CVlower( p2 );

	    ccb->rqst.rqst_id0 = p2;
	    p2 = NULL;
	}
	else
	{
	    /*
	    ** Initialize for procedure owner query.
	    */
	    if ( ! proc_owner_init( pcb ) )
	    {
		gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		gcd_del_pcb( pcb );
		goto cleanup;
	    }
	}
	break;

    case MSG_REQ_DBMSINFO :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	if ( ! gcd_expand_qtxt(ccb, STlength(qry_info) + p1_len, FALSE) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	ccb->sequence = REQ_INFO;
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = FALSE;
	STprintf( ccb->qry.qtxt, qry_info, p1 );
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_REQ_2PC_XIDS :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	if ( ! gcd_expand_qtxt(ccb, STlength(qry_xids) + p1_len, FALSE) )
	{
	    status = E_GC4808_NO_MEMORY;
	    break;
	}

	ccb->sequence = REQ_XIDS;
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = FALSE;
	STprintf( ccb->qry.qtxt, qry_xids, p1 );
	pcb->data.query.text = ccb->qry.qtxt;
	break;

    case MSG_REQ_PARAM_INFO :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	/*
	** Save procedure name in rqst_id1.
	*/
	if  ( GCD_IS_UPPER( ccb->cib->flags ) )
	    CVupper( p1 );
	else  if  ( GCD_IS_LOWER( ccb->cib->flags ) )
	    CVlower( p1 );

	ccb->rqst.rqst_id1 = p1;
	p1 = NULL;

	/*
	** If schema name is provided, query parameter info
	** directly.  Otherwise, appropriate schema name must
	** be resolved by precedence: user, dba, system.
	*/
	if ( p_rec & RQP_SCHEMA )  
	{
	    /*
	    ** Query set-up is deferred to allow shared usage 
	    ** after alternate (schema resolution) path.
	    */
	    ccb->sequence = REQ_PINFO;

	    if  ( GCD_IS_UPPER( ccb->cib->flags ) )
		CVupper( p2 );
	    else  if  ( GCD_IS_LOWER( ccb->cib->flags ) )
		CVlower( p2 );

	    ccb->rqst.rqst_id0 = p2;
	    p2 = NULL;
	}
	else
	{
	    /*
	    ** Initialize for procedure owner query.
	    */
	    if ( ! proc_owner_init( pcb ) )
	    {
		gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		gcd_del_pcb( pcb );
		goto cleanup;
	    }
	}
    	break;

    default : /* shouldn't happen, req_id checked above */
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay("%4d    GCD invalid request ID: %d\n", ccb->id, req_id);
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

    /*
    ** Requests should not change the transaction state.
    ** During autocommit or with an active transaction,
    ** no change will occur.  Otherwise, flag the need
    ** to clear the transaction initiated by the request.
    */
    if ( ! (ccb->cib->autocommit || ccb->xact.xacm_multi) && ! ccb->cib->tran )
	ccb->cib->flags |= GCD_XACT_RESET;
    else
    	ccb->cib->flags &= ~GCD_XACT_RESET;	/* Make sure flag is off */

    /*
    ** Before issuing a query, make sure the transaction 
    ** state has been properly established for the current
    ** transaction mode and query type.
    */
    if ( ! xact_check )
	msg_req_sm( (PTR)pcb );
    else
    {
	gcd_push_callback( pcb, msg_req_sm );
	gcd_xact_check( pcb, GCD_XQOP_NON_CURSOR );
    }

  cleanup:

    /*
    ** Release allocated resources.
    */
    if ( p1  &&  p1 != p1_buff )  MEfree( (PTR)p1 );
    if ( p2 )  MEfree( (PTR)p2 );

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
**	 4-Aug-03 (gordy)
**	    Allocate query data buffers (no longer done in gcd_new_stmt()).
**	 6-Oct-03 (gordy)
**	    Removed total row count.
**	 2-Jun-05 (gordy)
**	    Consolidate final actions to allow checks for connection
**	    and transaction aborts.
**	11-Oct-05 (gordy)
**	    Check for API logical sequence errors and errors during
**	    transaction processing.
**	18-Jul-06 (gordy)
**	    Reset transaction to match initial state.
**	25-Aug-06 (gordy)
**	    Add request for database procedure parameter information.
**	23-Mar-09 (gordy)
**	    Support for long database object names.  Combined INFO 
**	    actions to make it easier to handle dynamic data.
*/

static void
msg_req_sm( PTR arg )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    GCD_CCB		*ccb = pcb->ccb;
    GCD_SCB		*scb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Request: %s\n",
		   ccb->id, gcu_lookup( reqSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case REQ_CAPS :		/* Begin DBMS capabilities request */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_CAPS_1 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_CAPS_2 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, CAPS_ROW_MAX )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_CAPS_3 :		/* Read next set of data */
	/*
	** Read next set of data.
	*/
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_CAPS_4 :		/* Process data results */
	/*
	** Extract and save DBMS capabilities (if any)
	** Loop until all capabilities retrieved, then
	** free statement resources.
	*/
	if ( pcb->data.data.row_cnt  &&  save_caps( pcb ) )
	{
	    ccb->sequence = REQ_CAPS_3;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case REQ_CAPS_DATA :	/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_caps( pcb );
	ccb->sequence = REQ_CHECK;
	break;

    case REQ_INFO :		/* Begin DBMSINFO request */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_INFO_1 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_INFO_2 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, 1 )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_INFO_3 :		/* Read info */
	/*
	** Read next set of data.
	*/
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_INFO_4 :		/* Process data results */
    {
	/*
	** Extract and save DBMSINFO (if any).
	*/
	char	info_buff[ GCD_NAME_MAX + 1 ];
	char	*info = NULL;
	u_i2	len = 0;

	if ( pcb->data.data.row_cnt )  
	{
	    len = column_length( (IIAPI_DESCRIPTOR *)pcb->scb->column.desc, 
				 (IIAPI_DATAVALUE *)pcb->scb->column.data );

	    if ( len < sizeof( info_buff ) )
	        info = info_buff;
	    else  if ( ! (info = (char *)MEreqmem( 0, len + 1, FALSE, NULL )) )
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD mem alloc failed: DB info\n",
				ccb->id );
		gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
		gcd_del_pcb( pcb );
		return;
	    }

	    extract_string( (IIAPI_DESCRIPTOR *)pcb->scb->column.desc, 
			    (IIAPI_DATAVALUE *)pcb->scb->column.data, 
			    info, len + 1 );
	}

	/*
	** Build and send the response messages.
	*/
	gcd_msg_begin( ccb, &pcb->rcb, MSG_DATA, 0 );
	gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	gcd_put_i2( &pcb->rcb, len );
	if ( len )  gcd_put_bytes( &pcb->rcb, len, (u_i1 *)info );
	gcd_msg_end( &pcb->rcb, FALSE );

	if ( info  &&  info != info_buff )  MEfree( (PTR)info );

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	ccb->sequence = REQ_CHECK;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;
    }
    case REQ_POWN :		/* Begin procedure owner lookup */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_POWN_1 :		/* Describe parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.desc.count = ccb->qry.api_parms->col_cnt;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_setDesc( pcb );
	return;
    
    case REQ_POWN_2 :		/* Send parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = (IIAPI_DATAVALUE *)ccb->qry.api_parms->data;
	pcb->data.data.more_segments = FALSE;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_putParm( pcb );
	return;

    case REQ_POWN_3 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_POWN_4 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	/*
	** Allocate new statement and three row buffers 
	** to hold potential owners: user, dba, system.
	*/
	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, 3 )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_POWN_5 :		/* Read next set of data */
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_POWN_6 :		/* Process data results */
	/*
	** If procedure owner resolved, continue with parameter 
	** query.  Otherwise, return no results.
	*/
	if ( pcb->data.data.row_cnt < 1  ||  ! proc_owner( pcb ) )
	    ccb->sequence = REQ_CHECK;
	else
	    switch( ccb->rqst.req_id )
	    {
	    case MSG_REQ_PARAM_NAMES :	ccb->sequence = REQ_PNAME;	break;
	    case MSG_REQ_PARAM_INFO :	ccb->sequence = REQ_PINFO;	break;
	    default :			ccb->sequence = REQ_CHECK;	break;
	    }

	/*
	** Clean-up query
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case REQ_PNAME :		/* Begin parameter name request */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	/*
	** Initialize for parameter query.
	*/
	if ( ! param_query_init( pcb ) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	pcb->data.query.text = (ccb->cib->flags & GCD_IS_INGRES) 
			       ? qry_param_name_ing : qry_param_name_gtw;

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_PNAM_1 :		/* Describe parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.desc.count = ccb->qry.api_parms->col_cnt;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_setDesc( pcb );
	return;
    
    case REQ_PNAM_2 :		/* Send parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = (IIAPI_DATAVALUE *)ccb->qry.api_parms->data;
	pcb->data.data.more_segments = FALSE;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_putParm( pcb );
	return;

    case REQ_PNAM_3 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_PNAM_4 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	/*
	** Allocate new statement an one row buffer.
	*/
	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, 1 )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_PNAM_5 :		/* Read next set of data */
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_PNAM_6 :		/* Process data results */
	if ( pcb->data.data.row_cnt  &&  save_pname( pcb ) )
	{
	    ccb->sequence = REQ_PNAM_5;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case REQ_PNAM_7 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_pname( pcb );
	ccb->sequence = REQ_CHECK;
	break;

    case REQ_PINFO :		/* Begin parameter info request */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	/*
	** Initialize for parameter query.
	*/
	if ( ! param_query_init( pcb ) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	pcb->data.query.text = (ccb->cib->flags & GCD_IS_INGRES) 
			       ? qry_param_info_ing : qry_param_info_gtw;

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_PINF_1 :		/* Describe parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.desc.count = ccb->qry.api_parms->col_cnt;
	pcb->data.desc.desc = (IIAPI_DESCRIPTOR *)ccb->qry.api_parms->desc;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_setDesc( pcb );
	return;
    
    case REQ_PINF_2 :		/* Send parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	pcb->data.data.col_cnt = ccb->qry.api_parms->col_cnt;
	pcb->data.data.data = (IIAPI_DATAVALUE *)ccb->qry.api_parms->data;
	pcb->data.data.more_segments = FALSE;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_putParm( pcb );
	return;

    case REQ_PINF_3 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_PINF_4 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	/*
	** Allocate new statement an one row buffer.
	*/
	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, 1 )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_PINF_5 :		/* Read next set of data */
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_PINF_6 :		/* Process data results */
	if ( pcb->data.data.row_cnt  &&  save_pinfo( pcb ) )
	{
	    ccb->sequence = REQ_PINF_5;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case REQ_PINF_7 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_pinfo( pcb );
	ccb->sequence = REQ_CHECK;
	break;

    case REQ_XIDS :		/* Begin 2PC XIDS request */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CHECK;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_query( pcb );
	return;
    
    case REQ_XIDS_1 :		/* Get result descriptor */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_getDesc( pcb );
	return;

    case REQ_XIDS_2 :		/* Prepare to retrieve rows */
	if ( pcb->api_error )
	{
	    ccb->sequence = REQ_CANCEL;
	    break;
	}

	if ( ! (scb = gcd_new_stmt( ccb, pcb ))  ||
	     ! (gcd_alloc_qdata( &scb->column, 1 )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    gcd_del_pcb( pcb );
	    return;
	}

	/*
	** Make the new statement current.
	*/
	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	break;

    case REQ_XIDS_3 :		/* Read info */
	/*
	** Read next set of data.
	*/
	pcb->data.data.max_row = pcb->scb->column.max_rows;
	pcb->data.data.col_cnt = pcb->scb->column.max_cols;
	pcb->data.data.data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
	gcd_push_callback( pcb, msg_req_sm );

	if ( ! (ccb->msg.flags & GCD_MSG_XOFF) )
	    gcd_api_getCol( pcb );
	else
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD request delayed pending OB XON\n", 
			   ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	}
	return;

    case REQ_XIDS_4 :		/* Process data results */
	/*
	** Extract and save transaction IDs (if any).
	** Loop until all XIDs retrieved, then free 
	** statement resources.
	*/
	if ( pcb->data.data.row_cnt  &&  save_xids( pcb ) )
	{
	    ccb->sequence = REQ_XIDS_3;
	    break;
	}

	/*
	** End of data.
	*/
	scb = pcb->scb;
	pcb->scb = NULL;
	ccb->api.stmt = NULL;
	gcd_push_callback( pcb, msg_req_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case REQ_XIDS_5 :		/* End of query */
	/*
	** Build and send the response messages.
	*/
	rqst_xids( pcb );
	ccb->sequence = REQ_CHECK;
	break;

    case REQ_CANCEL :		/* Cancel the current query */
	gcd_msg_flush( ccb );
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = REQ_DONE;
	    break;
	}
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_cancel( pcb );
	return;

    case REQ_CLOSE :		/* Close statement handle */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = REQ_DONE;
	    break;
	}
	gcd_push_callback( pcb, msg_req_sm );
	gcd_api_close( pcb, NULL );	
	return;

    case REQ_CHECK :		/* Check transaction state */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )  
	    break;	/* Handled below */

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	{
	    gcd_push_callback( pcb, msg_req_sm );
	    gcd_xact_abort( pcb );
	    return;
	}

	if ( ccb->cib->flags & GCD_XACT_RESET )
	{
	    ccb->cib->flags &= ~GCD_XACT_RESET;

	    /*
	    ** If a transaction was started by the request,
	    ** commit the transaction to ensure the same
	    ** transaction state as before the request.
	    */
	    if ( ccb->cib->tran )
	    {
		gcd_push_callback( pcb, msg_req_sm );
		gcd_api_commit( pcb );
	    	return;
	    }
	}
	break;

    case REQ_DONE :		/* Request completed. */
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
	    TRdisplay( "%4d    GCD invalid request processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	gcd_del_pcb( pcb );
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
**	 2-Jun-05 (gordy)
**	    Gateways and case indicators changed to flag bits.
*/

static bool
save_caps( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    GCD_CAPS		*caps;
    u_i2		i, row, col, len;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 2 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid caps info (%d,%d)\n", ccb->id, 
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( FALSE );
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

    if ( ! (caps = (GCD_CAPS *)
	    MEreqmem( 0, sizeof(GCD_CAPS) + (sizeof(CAPS_INFO) * 
			 (pcb->data.data.row_cnt - 1)), FALSE, NULL ))  ||
         ! (caps->data = MEreqmem( 0, len, FALSE, NULL )) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD error allocating caps info\n", ccb->id );
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
		ccb->cib->flags |= GCD_CASE_UPPER;
	    else  if ( STcompare( caps->caps[ row ].value, "LOWER" ) == 0 )
		ccb->cib->flags |= GCD_CASE_LOWER;
	    else  if ( STcompare( caps->caps[ row ].value, "MIXED" ) == 0 )
		ccb->cib->flags |= GCD_CASE_MIXED;

	if ( STcompare( caps->caps[ row ].name, "DBMS_TYPE" ) == 0  &&
	     STcompare( caps->caps[ row ].value, "INGRES" ) == 0 )
	    ccb->cib->flags |= GCD_IS_INGRES;
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
**	23-Dec-02 (gordy)
**	    Connection level capability ID is protocol level dependent.
*/

static void
rqst_caps( GCD_PCB *pcb )
{
    GCD_CCB	*ccb = pcb->ccb;
    QUEUE	*q;
    char	value[ 33 ];

    gcd_msg_begin( ccb, &pcb->rcb, MSG_DATA, 0 );
    
    for( q = ccb->cib->caps.q_next; q != &ccb->cib->caps; q = q->q_next )
    {
	GCD_CAPS	*caps = (GCD_CAPS *)q;
	u_i2		i;

	for( i = 0; i < caps->cap_cnt; i++ )
	    copy_caps( pcb, caps->caps[ i ].name, caps->caps[ i ].value );
    }

    /*
    ** Generate GCD server capabilities.
    */
    CVla( ccb->cib->level, value );
    copy_caps( pcb, (ccb->msg.proto_lvl < MSG_PROTO_3) 
		    ? "JDBC_CONNECT_LEVEL" : "DBMS_CONNECT_LEVEL", value );

    gcd_msg_end( &pcb->rcb, FALSE );
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
copy_caps( GCD_PCB *pcb, char *name, char *value )
{
    u_i2 len;
    
    len = STlength( name );
    gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
    gcd_put_i2( &pcb->rcb, len );
    gcd_put_bytes( &pcb->rcb, len, (u_i1 *)name );

    len = STlength( value );
    gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
    gcd_put_i2( &pcb->rcb, len );
    gcd_put_bytes( &pcb->rcb, len, (u_i1 *)value );

    return;
}


/*
** Name: proc_owner_init
**
** Description:
**	Set-up request parameters to query procedure owner.  
**	Initialize API service parameters with procedure name.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if successful, FALSE if memory allocation error.
**
** History:
**	25-Aug-06 (gordy)
**	    Created.
**	23-Mar-09 (gordy)
**	    Request parameters now dynamic.
*/

static bool
proc_owner_init( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;

    /*
    ** Procedure name should be in rqst_id1.
    */
    if ( ! ccb->rqst.rqst_id1 )  return( FALSE );	/* Should not happen */

    ccb->sequence = REQ_POWN;
    ccb->qry.api_parms = &ccb->qry.svc_parms;
    pcb->data.query.type = IIAPI_QT_QUERY;
    pcb->data.query.params = TRUE;
    pcb->data.query.text = (ccb->cib->flags & GCD_IS_INGRES) 
			   ? qry_proc_owner_ing : qry_proc_owner_gtw;

    /*
    ** Allocate one parameter: procedure name.
    */
    if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 1, NULL ) )  return( FALSE );
	
    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
    desc->ds_columnType = IIAPI_COL_QPARM;
    desc->ds_dataType = IIAPI_CHA_TYPE;
    desc->ds_length = STlength( ccb->rqst.rqst_id1 );
    desc->ds_nullable = FALSE;

    if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )  return( FALSE );
	
    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
    data->dv_null = FALSE;
    data->dv_length = desc->ds_length;
    MEcopy( (PTR)ccb->rqst.rqst_id1, desc->ds_length, data->dv_value );
    ccb->qry.api_parms->col_cnt = 1;

    return( TRUE );
}


/*
** Name: proc_owner
**
** Description:
**	Resolve procedure schema/owner with standard precedence:
**	user, dba, system.
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
**	25-Aug-06 (gordy)
**	    Created.
**	23-Mar-09 (gordy)
**	    Support long database object names. Removed fixed sized
**	    local buffer and made all dynamic.
*/

static bool
proc_owner( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    i4			row, i, level;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    bool		found = FALSE;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 4 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid proc info (%d,%d)\n", pcb->ccb->id,
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( FALSE );
    }

    /*
    ** Save schema resolution owners: user, dba, system
    */
    if ( ! ccb->cib->usr )
    {
	u_i2	usr_len = column_length( &desc[ 1 ], &data[ 1 ] ) + 1;
	u_i2	dba_len = column_length( &desc[ 2 ], &data[ 2 ] ) + 1;
	u_i2	sys_len = column_length( &desc[ 3 ], &data[ 3 ] ) + 1;

	ccb->cib->usr = (char *)MEreqmem( 0, usr_len, FALSE, NULL );
	ccb->cib->dba = (char *)MEreqmem( 0, dba_len, FALSE, NULL );
	ccb->cib->sys = (char *)MEreqmem( 0, sys_len, FALSE, NULL );

	if ( ! ccb->cib->usr  ||  ! ccb->cib->dba  ||  ! ccb->cib->sys )
	{
	    if ( ccb->cib->usr )  MEfree( ccb->cib->usr );
	    if ( ccb->cib->dba )  MEfree( ccb->cib->dba );
	    if ( ccb->cib->sys )  MEfree( ccb->cib->sys );
	    ccb->cib->usr = ccb->cib->dba = ccb->cib->sys = NULL;

	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD mem alloc failed: DB constants\n",
			    ccb->id );
	    return( FALSE );
	}

	extract_string( &desc[1], &data[1], ccb->cib->usr, usr_len );
	extract_string( &desc[2], &data[2], ccb->cib->dba, dba_len );
	extract_string( &desc[3], &data[3], ccb->cib->sys, sys_len );
    }

    for( 
	 row = i = 0, level = 4; 
	 row < pcb->data.data.row_cnt  &&  level > 1; 
	 row++, i += pcb->data.data.col_cnt 
       )
    {
	u_i2	len = column_length( &desc[ 0 ], &data[ i ] ) + 1;
	bool	match = FALSE;
	char	*owner;

	if ( ! (owner = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD mem alloc failed: proc owner\n",
			    ccb->id );
	    return( FALSE );
	}

	extract_string( &desc[0], &data[i], owner, len );

	switch( level )
	{
	case 4 :
	    if ( STcompare( owner, ccb->cib->sys ) == 0 )
	    {
		level = 3;
	    	match = TRUE;
	    }

	case 3 :
	    if ( STcompare( owner, ccb->cib->dba ) == 0 )
	    {
		level = 2;
	    	match = TRUE;
	    }
	    
	case 2 :
	    if ( STcompare( owner, ccb->cib->usr ) == 0 )
	    {
		level = 1;
	    	match = TRUE;
	    }

	case 1 :
	    break;
	}

        if ( ! match )
	    MEfree( (PTR)owner );
	else
	{
	    if ( ccb->rqst.rqst_id0 )  MEfree( (PTR) ccb->rqst.rqst_id0 );
	    ccb->rqst.rqst_id0 = owner;
	    found = TRUE;
	}
    }

    return( found );
}


/*
** Name: param_query_init
**
** Description:
**	Set-up request parameters to query parameter names and info.  
**	Initialize API service parameters with procedure owner and name.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if successful, FALSE if memory allocation error.
**
** History:
**	25-Aug-06 (gordy)
**	    Created.
**	23-Mar-09 (gordy)
**	    Request parameters now dynamic.
*/

static bool
param_query_init( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;

    /*
    ** Schema name should be in rqst_id0.
    ** Procedure name should be in rqst_id1.
    */
    if ( ! ccb->rqst.rqst_id0 )  return( FALSE );  /* Should not happen */
    if ( ! ccb->rqst.rqst_id1 )  return( FALSE );

    ccb->qry.api_parms = &ccb->qry.svc_parms;
    pcb->data.query.type = IIAPI_QT_QUERY;
    pcb->data.query.params = TRUE;

    /*
    ** Allocate two parameters: procedure schema & name.
    */
    if ( ! gcd_alloc_qdesc( &ccb->qry.svc_parms, 2, NULL ) )  return( FALSE );
	
    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[0];
    desc->ds_columnType = IIAPI_COL_QPARM;
    desc->ds_dataType = IIAPI_CHA_TYPE;
    desc->ds_length = STlength( ccb->rqst.rqst_id0 );
    desc->ds_nullable = FALSE;

    desc = &((IIAPI_DESCRIPTOR *)ccb->qry.svc_parms.desc)[1];
    desc->ds_columnType = IIAPI_COL_QPARM;
    desc->ds_dataType = IIAPI_CHA_TYPE;
    desc->ds_length = STlength( ccb->rqst.rqst_id1 );
    desc->ds_nullable = FALSE;

    if ( ! gcd_alloc_qdata( &ccb->qry.svc_parms, 1 ) )  return( FALSE );
	
    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[0];
    data->dv_null = FALSE;
    data->dv_length = STlength( ccb->rqst.rqst_id0 );
    MEcopy( (PTR)ccb->rqst.rqst_id0, data->dv_length, data->dv_value );
    ccb->qry.api_parms->col_cnt = 1;

    data = &((IIAPI_DATAVALUE *)ccb->qry.svc_parms.data)[1];
    data->dv_null = FALSE;
    data->dv_length = STlength( ccb->rqst.rqst_id1 );
    MEcopy( (PTR)ccb->rqst.rqst_id1, data->dv_length, data->dv_value );
    ccb->qry.api_parms->col_cnt++;

    return( TRUE );
}


/*
** Name: save_pname
**
** Description:
**	Process the results of the parameter names query.
**	Save parameter names in ccb.
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
**	25-Aug-06 (gordy)
**	    Split into two routines: one to save and one to send names.
**	23-Mar-09 (gordy)
**	    Support long database object names.  Result structure is
**	    now variable length with variable length object name.
*/

static bool
save_pname( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    u_i2		row, i;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 2 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid parameter name data (%d,%d)\n", 
	    		pcb->ccb->id, pcb->data.data.row_cnt, 
			pcb->data.data.col_cnt );
	return( FALSE );
    }

    for( 
	 row = i = 0; 
	 row < pcb->data.data.row_cnt; 
	 row++, i += pcb->data.data.col_cnt 
       )
    {
	RQST_PNAME	*name;
	u_i2		len = column_length( &desc[ 1 ], &data[ i + 1 ] );

	if ( ! (name = (RQST_PNAME *)
			MEreqmem( 0, sizeof(RQST_PNAME) + len, FALSE, NULL )) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD error allocating PNAME\n", ccb->id );
	    return( FALSE );
	}

	extract_string( &desc[ 1 ], &data[ i + 1 ], 
			name->name, sizeof( name->name ) + len );
	QUinsert( &name->q, ccb->rqst.rqst_q.q_prev );
    }

    return( TRUE );
}


/*
** Name: rqst_pname
**
** Description:
**	Format the response to the parameter name request.
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
**	25-Aug-06 (gordy)
**	    Created.
*/

static void
rqst_pname( GCD_PCB *pcb )
{
    GCD_CCB	*ccb = pcb->ccb;
    QUEUE	*q;

    if ( QUEUE_EMPTY( &ccb->rqst.rqst_q ) )  return;
    gcd_msg_begin( ccb, &pcb->rcb, MSG_DATA, 0 );

    for( 
	 q = ccb->rqst.rqst_q.q_next; 
	 q != &ccb->rqst.rqst_q; 
	 q = ccb->rqst.rqst_q.q_next
       )
    {
	char	*name = ((RQST_PNAME *)q)->name;
	u_i2	len = STlength( name );

	gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	gcd_put_i2( &pcb->rcb, len );
	gcd_put_bytes( &pcb->rcb, len, (u_i1 *)name );

	QUremove( q );
	MEfree( (PTR)q );
    }

    gcd_msg_end( &pcb->rcb, FALSE );
    return;
}


/*
** Name: save_pinfo
**
** Description:
**	Process the results of the parameter information query.
**	Save parameter info in the CCB.
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
**	25-Aug-06 (gordy)
**	    Created.
**	23-Mar-09 (gordy)
**	    Support long database object names.  Result structure is
**	    now variable length with variable length object name.
*/

static bool
save_pinfo( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    u_i2		row, i;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 6 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid parameter info (%d,%d)\n", ccb->id, 
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( FALSE );
    }

    for( 
    	 row = i = 0; 
	 row < pcb->data.data.row_cnt; 
	 row++, i += pcb->data.data.col_cnt )
    {
	RQST_PINFO	*info;
	u_i2		len = column_length( &desc[ 1 ], &data[ i + 1 ] );

	if ( ! (info = (RQST_PINFO *)
			MEreqmem( 0, sizeof(RQST_PINFO) + len, FALSE, NULL )) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD error allocating PINFO\n", ccb->id );
	    return( FALSE );
	}

	extract_string( &desc[ 1 ], &data[ i + 1 ], 
			info->name, sizeof( info->name ) + len );
	info->type = extract_int( &desc[2], &data[i+2] );
	info->length = extract_int( &desc[3], &data[i+3] );
	info->scale = extract_int( &desc[4], &data[i+4] );
	extract_string(&desc[5], &data[i+5], info->nulls, sizeof(info->nulls));

	QUinsert( &info->q, ccb->rqst.rqst_q.q_prev );
    }

    return( TRUE );
}



/*
** Name: rqst_pinfo
**
** Description:
**	Format the response to the parameter info request.
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
**	25-Aug-06 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
*/

static void
rqst_pinfo( GCD_PCB *pcb )
{
    GCD_CCB	*ccb = pcb->ccb;
    QUEUE	*q;
    u_i2	count = 0;

    /*
    ** Count the number of parameters
    */
    for( q = ccb->rqst.rqst_q.q_next; q != &ccb->rqst.rqst_q; q = q->q_next )
	count++;

    gcd_msg_begin( ccb, &pcb->rcb, MSG_DESC, 0 );
    gcd_put_i2( &pcb->rcb, count );

    for( 
	 q = ccb->rqst.rqst_q.q_next; 
	 q != &ccb->rqst.rqst_q; 
	 q = ccb->rqst.rqst_q.q_next 
       )
    {
	RQST_PINFO	*info = (RQST_PINFO *)q;
	i2		type;
	u_i2		len;
	u_i1		prec, scale, flags;

	type = SQL_OTHER;
	flags = (info->nulls[0] == 'Y') ? MSG_DSC_NULL : 0;
	len = info->length;
	prec = scale = 0;

	switch( abs( info->type ) )
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
	    prec = info->length;
	    scale = info->scale;
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
	    
	case IIAPI_DTE_TYPE :
	    type = SQL_TIMESTAMP;
	    len = 0;
	    break;

	case IIAPI_DATE_TYPE :
	    type = SQL_DATE;
	    len = 0;
	    break;

	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	case IIAPI_TMTZ_TYPE :
	    type = SQL_TIME;
	    scale = info->scale;
	    len = 0;
	    break;

	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	case IIAPI_TSTZ_TYPE :
	    type = SQL_TIMESTAMP;
	    scale = info->scale;
	    len = 0;
	    break;

	case IIAPI_INTYM_TYPE :
	    type = SQL_INTERVAL;
	    len = 0;
	    break;

	case IIAPI_INTDS_TYPE :
	    type = SQL_INTERVAL;
	    scale = info->scale;
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
	    break;

	case IIAPI_LVCH_TYPE :	
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LCLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	    type = SQL_LONGVARCHAR; 
	    len = 0; 
	    break;

	case IIAPI_BYTE_TYPE :	type = SQL_BINARY;		break;
	case IIAPI_VBYTE_TYPE :	type = SQL_VARBINARY;		break;

	case IIAPI_LBYTE_TYPE :	
	case IIAPI_LBLOC_TYPE :
	    type = SQL_LONGVARBINARY; 
	    len = 0; 
	    break;
	}

	if ( type == SQL_OTHER  &&  GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid data type: %d (%d)\n", ccb->id,
		       info->type, len );

	gcd_put_i2( &pcb->rcb, type );
	gcd_put_i2( &pcb->rcb, (i2)abs( info->type ) );
	gcd_put_i2( &pcb->rcb, len );
	gcd_put_i1( &pcb->rcb, prec );
	gcd_put_i1( &pcb->rcb, scale );
	gcd_put_i1( &pcb->rcb, flags );

	if ( ! (len = STlength( info->name )) )
	    gcd_put_i2( &pcb->rcb, 0 );
	else
	{
	    gcd_put_i2( &pcb->rcb, len );
	    gcd_put_bytes( &pcb->rcb, len, (u_i1 *)info->name );
	}

	QUremove( q );
	MEfree( (PTR)q );
    }

    gcd_msg_end( &pcb->rcb, FALSE );
    return;
}


/*
** Name: save_xids
**
** Description:
**	Process the results of the transaction ID query.
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
save_xids( GCD_PCB *pcb )
{
    GCD_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)pcb->scb->column.desc;
    IIAPI_DATAVALUE	*data = (IIAPI_DATAVALUE *)pcb->scb->column.data;
    u_i2		row;

    if ( pcb->data.data.row_cnt < 1  ||  pcb->data.data.col_cnt != 1 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid xid info (%d,%d)\n", ccb->id, 
			pcb->data.data.row_cnt, pcb->data.data.col_cnt );
	return( FALSE );
    }

    for( row = 0; row < pcb->data.data.row_cnt; row++, data++ )
    {
	RQST_XIDS	*info;
	char		str[ 512 ];
	u_i2		len;

	if ( ! (info = (RQST_XIDS *)
			MEreqmem( 0, sizeof( RQST_XIDS ), FALSE, NULL )) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD error allocating XIDS\n", ccb->id );
	    return( FALSE );
	}

	if ( column_length( desc, data ) >= sizeof( str ) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD Invalid XID length!\n", ccb->id );
	    continue;
	}
	
	extract_string( desc, data, str, sizeof( str ) );

	if ( ! parseXID( str, &info->xid ) )  
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD Invalid XID format!\n", ccb->id );
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
rqst_xids( GCD_PCB *pcb )
{
    GCD_CCB	*ccb = pcb->ccb;
    QUEUE	*q;

    if ( QUEUE_EMPTY( &ccb->rqst.rqst_q ) )  return;
    gcd_msg_begin( ccb, &pcb->rcb, MSG_DATA, 0 );

    for( 
	 q = ccb->rqst.rqst_q.q_next; 
	 q != &ccb->rqst.rqst_q; 
	 q = ccb->rqst.rqst_q.q_next 
       )
    {
	IIAPI_XA_DIS_TRAN_ID	*xid = &((RQST_XIDS *)q)->xid;

	gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	gcd_put_i4( &pcb->rcb, xid->xa_tranID.xt_formatID );

	gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	gcd_put_i2( &pcb->rcb, (i2)xid->xa_tranID.xt_gtridLength );
	gcd_put_bytes( &pcb->rcb, (i2)xid->xa_tranID.xt_gtridLength, 
			(u_i1 *)&xid->xa_tranID.xt_data[ 0 ] );

	gcd_put_i1( &pcb->rcb, 1 );	/* Not NULL */
	gcd_put_i2( &pcb->rcb, (i2)xid->xa_tranID.xt_bqualLength );
	gcd_put_bytes( &pcb->rcb, (i2)xid->xa_tranID.xt_bqualLength, 
			(u_i1 *)&xid->xa_tranID.xt_data[
				    xid->xa_tranID.xt_gtridLength ] );
	QUremove( q );
	MEfree( (PTR)q );
    }

    gcd_msg_end( &pcb->rcb, FALSE );
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


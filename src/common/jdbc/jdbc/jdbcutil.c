/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <me.h>
#include <qu.h>
#include <st.h>

#include <iicommon.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcutil.c
**
** Description:
**	JDBC server utility functions.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	 4-Nov-99 (gordy)
**	    Free RCB associated with PCB.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	21-Dec-99 (gordy)
**	    Place active CCBs on a queue for idle limit checking.
**	 6-Jan-00 (gordy)
**	    Connection info for connection pooling now placed in
**	    Connection Information Block.
**	 3-Mar-00 (gordy)
**	    Implement free queue for CIB.
**	28-Jul-00 (gordy)
**	    Enhanced query parameter support with separate storage
**	    for service and query parameters and jdbc_build_qdata()
**	    to combine the parameter sets into a single set.  Free
**	    storage for column names.
**	12-Oct-00 (gordy)
**	    Initialize the autocommit mode.
**	18-Apr-01 (gordy)
**	    Added request result queue to CCB.
**	10-May-01 (gordy)
**	    Added support for UCS2 data and datatypes.
**	13-Nov-01 (loera01) SIR 106366
**	    Conditionally compile Unicode support.
**	    BACKED OUT, NOT NEEDED IN MAIN. (hanje04)
*/	

/*
** This is not defined by ci.h, but is
** necessary for determining block size
** of the CI encryption algorithm.
*/
#define	CRYPT_SIZE	8

static	bool	alloc_qdata( QDATA *, u_i2, bool );
static	void	copy_i1( u_i1 *, u_i1 * );
static	void	copy_i2( u_i1 *, u_i1 * );
static	void	copy_i4( u_i1 *, u_i1 * );
static	void	copy_f4( u_i1 *, u_i1 * );
static	void	copy_f8( u_i1 *, u_i1 * );
static	bool	copy_atom( JDBC_CCB *, u_i2, 
			   void (*)( u_i1 *, u_i1 * ), u_i1 * );


/*
** Name: jdbc_new_ccb
**
** Description:
**	Allocate and initialize a new client connection control block.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JDBC_CCB *	New client connection control block.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Place active CCBs on a queue for idle limit checking.
**	 6-Jan-00 (gordy)
**	    Moved some connection info to CIB.
**	12-Oct-00 (gordy)
**	    Initialize the autocommit mode.
**	18-Apr-01 (gordy)
**	    Initialize the request result queue.
*/

JDBC_CCB *
jdbc_new_ccb( void )
{
    JDBC_CCB	*ccb;

    if ( ! (ccb = (JDBC_CCB *)QUremove( JDBC_global.ccb_free.q_next )) )
	if ( (ccb = (JDBC_CCB *)MEreqmem( 0, sizeof(JDBC_CCB), FALSE, NULL )) )
	    ccb->id = JDBC_global.ccb_total++;

    if ( ! ccb )
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
    else
    {
	u_i4	id = ccb->id;

	MEfill( sizeof( JDBC_CCB ), 0, (PTR)ccb );
	ccb->id = id;
	ccb->use_cnt = 1;
	ccb->max_buff_len = 1 << JDBC_TL_PKT_MIN;
	ccb->gcc.proto_lvl = JDBC_TL_PROTO_1;
	ccb->xact.auto_mode = JDBC_XACM_DBMS;
	ccb->api.env = JDBC_global.api_env;
	QUinit( &ccb->q );
	QUinit( &ccb->gcc.send_q );
	QUinit( &ccb->msg.msg_q );
	QUinit( &ccb->stmt.stmt_q );
	QUinit( &ccb->rqst.rqst_q );

	JDBC_global.ccb_active++;
	QUinsert( &ccb->q, &JDBC_global.ccb_q );

	if ( JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC new CCB (%d)\n", ccb->id, ccb->id );
    }

    return( ccb );
}


/*
** Name: jdbc_del_ccb
**
** Description:
**
** Input:
**	ccb	Client connection control block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Free xoff control blocks.
**	21-Dec-99 (gordy)
**	    Place active CCBs on a queue for idle limit checking.
**	28-Jul-00 (gordy)
**	    Separated service parameters from query parameters and
**	    provided structure for combined parameters as well.
**	 7-Mar-01 (gordy)
**	    Free the query text buffer.
**	18-Apr-01 (gordy)
**	    Clear the request result queue.
*/

void
jdbc_del_ccb( JDBC_CCB *ccb )
{
    if ( ccb->use_cnt )  ccb->use_cnt--;

    if ( ccb->use_cnt )
    {
	if ( JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC del CCB (use count %d)\n", 
		       ccb->id, ccb->use_cnt );
    }
    else
    {
	JDBC_RCB *rcb;
	JDBC_SCB *scb;
	QUEUE	 *q;

	QUremove( &ccb->q );
	JDBC_global.ccb_active--;
	if ( ccb->qry.qtxt_max )  MEfree( (PTR)ccb->qry.qtxt );
	jdbc_free_qdata( &ccb->qry.svc_parms );
	jdbc_free_qdata( &ccb->qry.qry_parms );
	jdbc_free_qdata( &ccb->qry.all_parms );

	/*
	** Free control blocks and request queues.
	*/
	if ( ccb->cib )  jdbc_del_cib( ccb->cib );

	if ( ccb->msg.xoff_pcb )
	    jdbc_del_pcb( (JDBC_PCB *)ccb->msg.xoff_pcb );

	if ( ccb->gcc.xoff_rcb )
	    jdbc_del_rcb( (JDBC_RCB *)ccb->gcc.xoff_rcb );

	if ( ccb->gcc.abort_rcb )
	    jdbc_del_rcb( (JDBC_RCB *)ccb->gcc.abort_rcb );

	while( (scb = (JDBC_SCB *)QUremove( ccb->stmt.stmt_q.q_next )) )
	{
	    jdbc_free_qdata( &scb->column );
	    MEfree( (PTR)scb );
	}

	while( (rcb = (JDBC_RCB *)QUremove( ccb->gcc.send_q.q_next )) )
	    jdbc_del_rcb( rcb );

	while( (rcb = (JDBC_RCB *)QUremove( ccb->msg.msg_q.q_next )) )
	    jdbc_del_rcb( rcb );

	while( (q = QUremove( ccb->rqst.rqst_q.q_next )) )
	    MEfree( (PTR)q );

	/*
	** Save CCB on free queue.
	*/
	QUinsert( &ccb->q, JDBC_global.ccb_free.q_prev );

	if ( JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC del CCB (%d)\n", ccb->id, ccb->id );
    }

    return;
}


/*
** Name: jdbc_new_cib
**
** Description:
**	Allocate a new Connection Information Block.
**
** Input:
**	count		Number of connection parameters.
**
** Output:
**	None.
**
** Returns
**	JDBC_CIB *	New Connection Information Block.
**
** History:
**	 6-Jan-00 (gordy)
**	    Created.
**	 3-Mar-00 (gordy)
**	    Implement free queue for CIB.
*/

JDBC_CIB *
jdbc_new_cib( u_i2 count )
{
    JDBC_CIB	*cib = NULL;
    u_i2	len = sizeof( JDBC_CIB ) + (sizeof(CONN_PARM) * (count - 1));

    if ( count < ARR_SIZE( JDBC_global.cib_free ) )
	if (! (cib = (JDBC_CIB *)QUremove(JDBC_global.cib_free[count].q_next)))
	    if ( (cib = (JDBC_CIB *)MEreqmem( 0, len, FALSE, NULL )) )
		cib->id = JDBC_global.cib_total++;

    if ( ! cib )
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
    else  
    {
	u_i4	id = cib->id;

	MEfill( len, 0, (PTR)cib );
	cib->id = id;
	cib->parm_max = count;
	QUinit( &cib->caps );
	JDBC_global.cib_active++;

	if ( JDBC_global.trace_level >= 6 )
	    TRdisplay( "%4d    JDBC new CIB (%d)\n", -1, cib->id );
    }

    return( cib );
}


/*
** Name: jdbc_del_cib
**
** Description:
**	Free a Connection Information Block and the associated resources.
**
** Input:
**	cib	Connection Information Block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 6-Jan-00 (gordy)
**	    Created.
**	 3-Mar-00 (gordy)
**	    Implement free queue for CIB.
*/

void
jdbc_del_cib( JDBC_CIB *cib )
{
    JDBC_CAPS	*caps;
    u_i2	i;

    JDBC_global.cib_active--;

    while( (caps = (JDBC_CAPS *)QUremove( cib->caps.q_next )) )
    {
	MEfree( caps->data );
	MEfree( (PTR)caps );
    }

    if ( cib->database )  MEfree( (PTR)cib->database );
    if ( cib->username )  MEfree( (PTR)cib->username );
    if ( cib->password )  MEfree( (PTR)cib->password );
    for( i = 0; i < cib->parm_cnt; i++ )  MEfree( (PTR)cib->parms[i].value );

    if ( JDBC_global.trace_level >= 6 )
	TRdisplay( "%4d    JDBC del CIB (%d)\n", -1, cib->id );

    if ( cib->parm_max < ARR_SIZE( JDBC_global.cib_free ) )
	QUinsert( &cib->q, JDBC_global.cib_free[ cib->parm_max ].q_prev );
    else
    {
	MEfree( (PTR)cib );
	JDBC_global.cib_total--;
    }

    return;
}


/*
** Name: jdbc_new_rcb
**
** Description:
**	Allocate a new request control block and optionally
**	a message buffer.
**
** Input:
**	size	Length of message buffer, 0 for no buffer.
**
** Output:
**	None.
**
** Returns:
**	JDBC_RCB *	New request control block.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
*/

JDBC_RCB *
jdbc_new_rcb( JDBC_CCB *ccb )
{
    JDBC_RCB *rcb;

    rcb = (JDBC_RCB *)MEreqmem( 0, sizeof(JDBC_RCB), TRUE, NULL );

    if ( rcb  &&  ccb )
    {
	rcb->ccb = ccb;
	rcb->buffer = (u_i1 *)MEreqmem( 0, ccb->max_buff_len + SZ_NL_PCI, 
					FALSE, NULL );
	if ( ! rcb->buffer )
	{
	    MEfree( (PTR)rcb );
	    rcb = NULL;
	}
	else
	{
	    /*
	    ** The start of the message buffer is
	    ** reserved for Network Layer use and
	    ** our TL Header.
	    */
	    rcb->buffer += SZ_NL_PCI + JDBC_TL_HDR_SZ;
	    rcb->buf_max = ccb->max_buff_len - JDBC_TL_HDR_SZ;
	    rcb->buf_len = 0;
	    rcb->buf_ptr = 0;
	}
    }

    if ( ! rcb )
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
    else if ( JDBC_global.trace_level >= 6 )
	TRdisplay( "%4d    JDBC new RCB (%p)\n", -1, rcb );

    return( rcb );
}


/*
** Name: jdbc_del_rcb
**
** Description:
**	Free a request control block and the associated message buffer.
**
** Input:
**	rcb	Request control block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
*/

void
jdbc_del_rcb( JDBC_RCB *rcb )
{
    if ( rcb->buffer )  
	MEfree( (PTR)(rcb->buffer - SZ_NL_PCI - JDBC_TL_HDR_SZ) );

    if ( JDBC_global.trace_level >= 6 )
	TRdisplay( "%4d    JDBC del RCB (%p)\n", -1, rcb );

    MEfree( (PTR)rcb );

    return;
}


/*
** Name: jdbc_new_pcb
**
** Description:
**	Allocate a new parameter control block and optionally
**	a request control block.  The caller must explicitly
**	call jdbc_del_rcb() on the allocated RCB.
**
** Input:
**	ccb		Connection control block.
**	alloc_rcb	TRUE to allocate an associated RCB.
**
** Output:
**	None.
**
** Returns:
**	JDBC_PCB *	New parameter control block.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
*/

JDBC_PCB *
jdbc_new_pcb( JDBC_CCB *ccb, bool alloc_rcb )
{
    JDBC_PCB *pcb;

    if ( ! (pcb = (JDBC_PCB *)MEreqmem( 0, sizeof( JDBC_PCB ), TRUE, NULL )) )
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
    else
    {
	if ( JDBC_global.trace_level >= 6 )
	    TRdisplay( "%4d    JDBC new PCB (%p)\n", ccb->id, pcb );

	pcb->ccb = ccb;

	if ( alloc_rcb  &&  ! (pcb->rcb = jdbc_new_rcb( pcb->ccb )) )
	{
	    MEfree( (PTR)pcb );
	    pcb = NULL;
	}
    }

    return( pcb );
}


/*
** Name: jdbc_del_pcb
**
** Description:
**	Free a parameter control block.  Any associated request
**	control block is not freed and must be freed explicitly
**	by the caller.
**
** Input:
**	pcb	Parameter control block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Free associated Request control block.
*/

void
jdbc_del_pcb( JDBC_PCB *pcb )
{
    if ( JDBC_global.trace_level >= 6 )
	TRdisplay( "%4d    JDBC del PCB (%p)\n", pcb->ccb->id, pcb );

    if ( pcb->rcb )  jdbc_del_rcb( pcb->rcb );
    MEfree( (PTR)pcb );
    return;
}

/*
** Name: jdbc_alloc_qdesc
**
** Description:
**	Initializes the Query data descriptor array.  Since column
**	descriptors are allocated by the API, the descriptor may
**	be passed into this routine.  Parameter descriptors are
**	not allocated by the API, and may be allocated here by
**	setting desc to NULL.  
**
**	This routine may be called with a count of 0 and a NULL
**	descriptor to initialize the qdata structure without
**	affecting the allocated buffers.  In this case, a FALSE
**	value is returned even though there wasn't a memory
**	allocation error.
**
** Input:
**	qdata		Query data.
**	count		Number of descriptors.
**	desc		Data descriptor array, may be NULL.
**
** Output:
**	qdata
**	  desc		Data descriptor.
**
** Returns:
**	bool		TRUE if successful, FALSE if memory allocation error.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	28-Jul-00 (gordy)
**	    Free column name storage.
*/

bool
jdbc_alloc_qdesc( QDATA *qdata, u_i2 count, PTR desc )
{
    qdata->max_rows = 1;
    qdata->max_cols = count;
    qdata->col_cnt = 0;
    qdata->cur_col = 0;
    qdata->more_segments = FALSE;
    qdata->desc = NULL;
    if ( ! qdata->max_cols )  return( FALSE );

    if ( desc )
	qdata->desc = desc;
    else
    {
	/*
	** Expand descriptor array if needed.
	*/
	if ( qdata->max_cols > qdata->desc_max )
	{
	    if ( qdata->param_desc )
	    {
		IIAPI_DESCRIPTOR *desc = (IIAPI_DESCRIPTOR *)qdata->param_desc;
		u_i2		 i;

		for( i = 0; i < qdata->desc_max; i++ )
		    if ( desc[i].ds_columnName )  
			MEfree( desc[i].ds_columnName );

		MEfree( qdata->param_desc );
	    }


	    if ( ! (qdata->param_desc = 
	    		MEreqmem( 0, sizeof(IIAPI_DESCRIPTOR) * 
				     qdata->max_cols, TRUE, NULL )) )
	    {
		gcu_erlog( 0, JDBC_global.language, 
			   E_JD0108_NO_MEMORY, NULL, 0, NULL);
		return( FALSE );
	    }

	    qdata->desc_max = qdata->max_cols;
	}

	qdata->desc = qdata->param_desc;
    }

    return( TRUE );
}

/*
** Name: jdbc_alloc_qdata
**
** Description:
**	Allocate API query data value array and data buffers
**	based on an API data descriptor.
**
** Input:
**	qdata
**	  max_cols	Number of descriptors.
**	  desc		Data descriptors.
**
** Output:
**	qdata
**	  data		Data values.
**	  data_buff	Buffer for data values.
**	  blob_buff	Buffer for blob segments.
**
** Returns:
**	bool		TRUE if successful, FALSE if memory allocation error.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	28-Jul-00 (gordy)
**	    Converted to cover routine for local function which
**	    permits the caller to control buffer allocation.
**       18-Aug-04 (rajus01)  Bug #112848 ; Startrak Problem #EDJDBC92
**          It is noted that the u_i2 variable 'length' gets
**          overrun by the long parameter data and resulted in JDBC
**          server crashing. Changed the above u_i2 variable to u_i4.
*/

bool
jdbc_alloc_qdata( QDATA *qdata, u_i2 max_rows )
{
    return( alloc_qdata( qdata, max_rows, TRUE ) );
}

static bool
alloc_qdata( QDATA *qdata, u_i2 max_rows, bool alloc_buffers )
{
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    u_i2		row, col, dv, seg_len;
    u_i4		blen, length, tot_cols = max_rows * qdata->max_cols;

    qdata->max_rows = max_rows;
    qdata->col_cnt = 0;
    qdata->cur_col = 0;
    qdata->more_segments = FALSE;
    if ( ! qdata->max_rows  ||  ! qdata->max_cols )  return( FALSE );
    
    /*
    ** Expand data value array if needed.
    */
    if ( tot_cols > qdata->data_max )
    {
	if ( qdata->data )  MEfree( qdata->data );
	if ( ! (qdata->data = MEreqmem( 0, sizeof( IIAPI_DATAVALUE ) * 
					   tot_cols, TRUE, NULL )) )
	{
	    gcu_erlog( 0, JDBC_global.language, 
		       E_JD0108_NO_MEMORY, NULL, 0, NULL);
	    return( FALSE );
	}

	qdata->data_max = tot_cols;
    }

    if ( ! alloc_buffers )  return( TRUE );

    desc = (IIAPI_DESCRIPTOR *)qdata->desc;
    data = (IIAPI_DATAVALUE *)qdata->data;

    for( col = length = seg_len = 0; col < qdata->max_cols; col++ )
	if ( desc[ col ].ds_dataType == IIAPI_LVCH_TYPE  ||
	     desc[ col ].ds_dataType == IIAPI_LNVCH_TYPE ||
	     desc[ col ].ds_dataType == IIAPI_LBYTE_TYPE )
	    seg_len = max( seg_len, desc[ col ].ds_length );
	else
	    length += desc[ col ].ds_length;

    if ( (blen = (length * qdata->max_rows)) > qdata->db_max )
    {
	if ( qdata->data_buff )  MEfree( qdata->data_buff );
	if ( ! (qdata->data_buff = MEreqmem( 0, blen, FALSE, NULL )) )
	{
	    gcu_erlog( 0, JDBC_global.language, 
		       E_JD0108_NO_MEMORY, NULL, 0, NULL);
	    return( FALSE );
	}

	qdata->db_max = blen;
    }

    if ( seg_len > qdata->bb_max )
    {
	if ( qdata->blob_buff )  MEfree( qdata->blob_buff );
	if ( ! (qdata->blob_buff = MEreqmem( 0, seg_len, FALSE, NULL )) )
	{
	    gcu_erlog( 0, JDBC_global.language, 
		       E_JD0108_NO_MEMORY, NULL, 0, NULL);
	    return( FALSE );
	}

	qdata->bb_max = seg_len;
    }

    for( row = dv = length = 0; row < qdata->max_rows; row++ )
	for( col = 0; col < qdata->max_cols; col++, dv++ )
	    if ( desc[ col ].ds_dataType == IIAPI_LVCH_TYPE  ||
		 desc[ col ].ds_dataType == IIAPI_LNVCH_TYPE ||
		 desc[ col ].ds_dataType == IIAPI_LBYTE_TYPE )
		    data[ dv ].dv_value = (char *)qdata->blob_buff;
	    else
	    {
		data[ dv ].dv_value = (char *)qdata->data_buff + length;
		length += desc[ col ].ds_length;
	    }

    return( TRUE );
}

/*
** Name: jdbc_build_qdata
**
** Description:
**	Combines two QDATA structures and creates a new
**	QDATA.  Descriptor entries marked as type TUPLE
**	are removed and not included in the combined set.  
**	Data buffers are not allocated for the new QDATA,
**	but the data array is allocated so that buffers
**	from the original QDATA structures may be shared.
**
** Input:
**	qd1	First QDATA set.
**	qd2	Second QDATA set.
**
** Output:
**	qd3	Combined QDATA set.
**
** Returns:
**	bool	TRUE if successful, FALSE if memory allocation failed.
**
** History:
**	28-Jul-00 (gordy)
**	    Created.
*/

bool
jdbc_build_qdata( QDATA *qd1, QDATA *qd2, QDATA *qd3 )
{
    IIAPI_DESCRIPTOR	*src_desc, *dst_desc;
    u_i2		i, count = 0;

    /*
    ** Count the number of non-tuple columns
    ** in the two source descriptors.
    */
    for( 
	 i = 0, src_desc = (IIAPI_DESCRIPTOR *)qd1->desc;
	 i < qd1->max_cols; 
	 i++, src_desc++
       )
	if ( src_desc->ds_columnType != IIAPI_COL_TUPLE )  
	    count++;

    for( 
	 i = 0, src_desc = (IIAPI_DESCRIPTOR *)qd2->desc;
	 i < qd2->max_cols; 
	 i++, src_desc++
       )
	if ( src_desc->ds_columnType != IIAPI_COL_TUPLE )  
	    count++;

    /*
    ** Allocate a new descriptor and data array
    ** (but not the data buffers).
    */
    if ( ! jdbc_alloc_qdesc( qd3, count, NULL )  ||
         ! alloc_qdata( qd3, 1, FALSE ) )  
	return( FALSE );

    /*
    ** Copy source descriptors.
    */
    dst_desc = (IIAPI_DESCRIPTOR *)qd3->desc;

    for( 
	 i = 0, src_desc = (IIAPI_DESCRIPTOR *)qd1->desc;
	 i < qd1->max_cols; 
	 i++, src_desc++
       )
	if ( src_desc->ds_columnType != IIAPI_COL_TUPLE )  
	{
	    if ( dst_desc->ds_columnName )  
		MEfree( (PTR)dst_desc->ds_columnName );

	    STRUCT_ASSIGN_MACRO( *src_desc, *dst_desc );
	    if ( src_desc->ds_columnName )
		dst_desc->ds_columnName = STalloc( src_desc->ds_columnName );
	    dst_desc++;
	}

    for( 
	 i = 0, src_desc = (IIAPI_DESCRIPTOR *)qd2->desc;
	 i < qd2->max_cols; 
	 i++, src_desc++
       )
	if ( src_desc->ds_columnType != IIAPI_COL_TUPLE )  
	{
	    if ( dst_desc->ds_columnName )  
		MEfree( (PTR)dst_desc->ds_columnName );

	    STRUCT_ASSIGN_MACRO( *src_desc, *dst_desc );
	    if ( src_desc->ds_columnName )
		dst_desc->ds_columnName = STalloc( src_desc->ds_columnName );
	    dst_desc++;
	}

    return( TRUE );
}


/*
** Name: jdbc_free_qdata
**
** Description:
**	Free memory allocated by jdbc_alloc_qdesc()
**	and jdbc_alloc_qdata().
**
** Input:
**	qdata		Query data.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	28-Jul-00 (gordy)
**	    Free column name storage.
*/

void
jdbc_free_qdata( QDATA *qdata )
{
    qdata->max_rows = 1;
    qdata->max_cols = 0;
    qdata->col_cnt = 0;
    qdata->cur_col = 0;
    qdata->more_segments = FALSE;
    qdata->desc = NULL;

    if ( qdata->param_desc )
    {
	IIAPI_DESCRIPTOR *desc = (IIAPI_DESCRIPTOR *)qdata->param_desc;
	u_i2		 i;

	for( i = 0; i < qdata->desc_max; i++ )
	    if ( desc[i].ds_columnName )  MEfree( desc[i].ds_columnName );

	MEfree( qdata->param_desc );
	qdata->param_desc = NULL;
    }

    qdata->desc_max = 0;

    if ( qdata->data )  MEfree( qdata->data );
    qdata->data = NULL;
    qdata->data_max = 0;

    if ( qdata->data_buff )  MEfree( qdata->data_buff );
    qdata->data_buff = NULL;
    qdata->db_max = 0;

    if ( qdata->blob_buff )  MEfree( qdata->blob_buff );
    qdata->blob_buff = NULL;
    qdata->bb_max = 0;

    return;
}


/*
** Name: jdbc_decode
**
** Description:
**	Decrypt data with provided key using Ingres CL module CI.
**	The input and output buffers may be the same.  The key
**	must be a null terminated string as must the resulting
**	decoded data.
**
** Input:
**	key	Encryption key.
**	data	Encrypted data.
**	length	Length of encrypted data.
**
** Output
**	buff	Decrypted data.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
*/

void
jdbc_decode( char *key, u_i1 *data, u_i2 length, char *buff )
{
    CI_KS	ks;
    u_i1	kbuff[ CRYPT_SIZE ];
    char	*ptr;
    u_i2	i, j;

    /*
    ** The key schedule is built from a single CRYPT_SIZE
    ** byte array.  We use the input key to build the byte
    ** array, truncating or duplicating as necessary.
    */
    for( i = 0, ptr = key; i < CRYPT_SIZE; i++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ i ] = *ptr++;
    }

    CIsetkey( (PTR)kbuff, ks );
    CIdecode( (PTR)data, length, ks, (PTR)buff );

    /*
    ** The encoded data must be padded to a multiple of
    ** CRYPT_SIZE bytes.  Random data is used for the pad,
    ** so for strings the null terminator is included in
    ** the data.  A random value is added to each block
    ** so that a given key/data pair does not encode the
    ** same way twice.  
    */
    for( i = 0, ptr = buff; i < length; i += CRYPT_SIZE )
	for( j = 1; j < CRYPT_SIZE; j++ )
	    if ( ! (*ptr++ = buff[ i + j ]) )	/* stop at EOS */
		break;

    return;
}


/*
** Name: jdbc_put_[i,f][1,2,4]{p}
**
** Description:
**	Append an atomic value to the current message.  If
**	there is insufficient room for the atomic value in
**	the current message buffer, the message is split
**	and the atomic value placed in a new allocated
**	request control block.
**
** Input:
**	rcb_ptr	    Request control block.
**	val	    Atomic value.
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE if allocation fails.
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_put_i1( JDBC_RCB **rcb_ptr, u_i1 val )
{
    return( jdbc_put_i1p( rcb_ptr, &val ) );
}

bool
jdbc_put_i1p( JDBC_RCB **rcb_ptr, u_i1 *val )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( CV_N_I1_SZ > RCB_AVAIL( rcb ) )  
	if ( ! jdbc_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I1_MACRO( val, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    return( TRUE );
}

bool
jdbc_put_i2( JDBC_RCB **rcb_ptr, u_i2 val )
{
    return( jdbc_put_i2p( rcb_ptr, (u_i1 *)&val ) );
}

bool
jdbc_put_i2p( JDBC_RCB **rcb_ptr, u_i1 *val )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( CV_N_I2_SZ > RCB_AVAIL( rcb ) )  
	if ( ! jdbc_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I2_MACRO( val, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I2_SZ;

    return( TRUE );
}

bool
jdbc_put_i4( JDBC_RCB **rcb_ptr, u_i4 val )
{
    return( jdbc_put_i4p( rcb_ptr, (u_i1 *)&val ) );
}

bool
jdbc_put_i4p( JDBC_RCB **rcb_ptr, u_i1 *val )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( CV_N_I4_SZ > RCB_AVAIL( rcb ) )  
	if ( ! jdbc_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I4_MACRO( val, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I4_SZ;

    return( TRUE );
}

bool
jdbc_put_f4p( JDBC_RCB **rcb_ptr, u_i1 *val )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( CV_N_F4_SZ > RCB_AVAIL( rcb ) )  
	if ( ! jdbc_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_F4_MACRO( val, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_F4_SZ;

    return( TRUE );
}

bool
jdbc_put_f8p( JDBC_RCB **rcb_ptr, u_i1 *val )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( CV_N_F8_SZ > RCB_AVAIL( rcb ) )  
	if ( ! jdbc_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_F8_MACRO( val, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_F8_SZ;

    return( TRUE );
}


/*
** Name: jdbc_put_bytes
**
** Description:
**	Append a buffer to the current message.  The current
**	message will be split (and a new request control block
**	allocated) if the input length exceeds the space left
**	in the current message buffer.
**
** Input:
**	rcb_ptr	    Request control block.
**	length	    Number of bytes to write.
**	ptr	    Source buffer.
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE if allocation error.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_put_bytes( JDBC_RCB **rcb_ptr, u_i2 length, u_i1 *ptr )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    u_i2	len;

    while( length )
    {
	if ( rcb->buf_len >= rcb->buf_max )
	    if ( ! jdbc_msg_split( rcb_ptr ) )  
		return( FALSE );
	    else
		rcb = *rcb_ptr;

	len = min( length, RCB_AVAIL( rcb ) );
	MEcopy( (PTR)ptr, len, (PTR)&rcb->buffer[ rcb->buf_len ] );
	rcb->buf_len += len;
	ptr += len;
	length -= len;
    }

    return( TRUE );
}


/*
** Name: jdbc_put_ucs2
**
** Description:
**	Append a UCS2 array to the current message.  The current
**	message will be split (and a new request control block
**	allocated) if the input length exceeds the space left
**	in the current message buffer.
**
** Input:
**	rcb_ptr	    Request control block.
**	length	    Number of UCS2 characters to write.
**	ptr	    Source buffer.
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE if allocation error.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

bool
jdbc_put_ucs2( JDBC_RCB **rcb_ptr, u_i2 length, u_i1 *ptr )
{
    JDBC_RCB	*rcb = *rcb_ptr;
    u_i2	len;

    length *= sizeof( UCS2 );	/* convert array length to byte length */

    while( length )
    {
	if ( (rcb->buf_len + CV_N_I2_SZ) > rcb->buf_max )
	    if ( ! jdbc_msg_split( rcb_ptr ) )  
		return( FALSE );
	    else
		rcb = *rcb_ptr;

	for( 
	     len = min( length, RCB_AVAIL( rcb ) );
	     len >= sizeof( UCS2 ); 
	     len -= sizeof( UCS2 ) 
	   )
	{
	    STATUS status;

	    CV2N_I2_MACRO( ptr, &rcb->buffer[ rcb->buf_len ], &status );
	    rcb->buf_len += CV_N_I2_SZ;
	    ptr += sizeof( UCS2 );
	    length -= sizeof( UCS2 );
	}
    }

    return( TRUE );
}


/*
** Name: jdbc_get_i{1,2,4}
**
** Description:
**	Read an atomic value from the current message.
**
** Input:
**	ccb	    Connection control block.
**
** Output:
**	ptr	    Atomic value.
**
** Returns:
**	bool	    TRUE if value copied, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_get_i1( JDBC_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I1_SZ, copy_i1, ptr ) );
}

bool
jdbc_get_i2( JDBC_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I2_SZ, copy_i2, (u_i1 *)ptr ) );
}

bool
jdbc_get_i4( JDBC_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I4_SZ, copy_i4, (u_i1 *)ptr ) );
}

bool
jdbc_get_f4( JDBC_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_F4_SZ, copy_f4, (u_i1 *)ptr ) );
}

bool
jdbc_get_f8( JDBC_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_F8_SZ, copy_f8, (u_i1 *)ptr ) );
}


/*
** Name: jdbc_get_bytes
**
** Description:
**	Read data from the current message.
**
** Input:
**	ccb	    Connection control block.
**	length	    Number of bytes to copy.
**
** Output:
**	ptr	    Output buffer.
**
** Returns:
**	bool	    TRUE if data copied, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_get_bytes( JDBC_CCB *ccb, u_i2 length, u_i1 *ptr )
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    JDBC_RCB	*rcb = (JDBC_RCB *)ccb->msg.msg_q.q_next;
	    u_i2	len;

	    if ( (len = min( length, rcb->msg.msg_len )) )
	    {
		MEcopy( (PTR)&rcb->buffer[ rcb->buf_ptr ], len, (PTR)ptr );
		rcb->buf_ptr += len;
		rcb->buf_len -= len;
		rcb->msg.msg_len -= len;
		ccb->msg.msg_len -= len;
		ptr += len;
		length -= len;
	    }

	    if ( ! rcb->msg.msg_len )
	    {
		QUremove( &rcb->q );
		jdbc_del_rcb( rcb );
	    }
	}

    return( ! length );
}


/*
** Name: jdbc_get_ucs2
**
** Description:
**	Read UCS2 data from the current message.
**
** Input:
**	ccb	    Connection control block.
**	length	    Number of UCS2 characters to copy.
**
** Output:
**	ptr	    Output buffer.
**
** Returns:
**	bool	    TRUE if data copied, FALSE otherwise.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

bool
jdbc_get_ucs2( JDBC_CCB *ccb, u_i2 length, u_i1 *ptr )
{
    for( ; length > 0; length--, ptr += sizeof( UCS2 ) )
	if ( ! copy_atom( ccb, CV_N_I2_SZ, copy_i2, (u_i1 *)ptr ) )
	    break;

    return( ! length );
}


/*
** Name: jdbc_skip
**
** Description:
**	Skip data in current message.
**
** Input:
**	ccb	    Connection control block.
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	bool	    TRUE if data skipped, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_skip( JDBC_CCB *ccb, u_i2 length )
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    JDBC_RCB	*rcb = (JDBC_RCB *)ccb->msg.msg_q.q_next;
	    u_i2	len;

	    if ( (len = min( length, rcb->msg.msg_len )) )
	    {
		rcb->buf_ptr += len;
		rcb->buf_len -= len;
		rcb->msg.msg_len -= len;
		ccb->msg.msg_len -= len;
		length -= len;
	    }

	    if ( ! rcb->msg.msg_len )
	    {
		QUremove( &rcb->q );
		jdbc_del_rcb( rcb );
	    }
	}

    return( ! length );
}


/*
** Name: copy_i{1,2,4}
**
** Description:
**	Functions to copy atomic values performing
**	conversion from network to local format.
**
** Input:
**	src	Source buffer.
**
** Output:
**	dst	Destination buffer.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

static void
copy_i1( u_i1 *src, u_i1 *dst )
{
    STATUS status;
    CV2L_I1_MACRO( src, dst, &status );
    return;
}

static void
copy_i2( u_i1 *src, u_i1 *dst )
{
    STATUS status;
    CV2L_I2_MACRO( src, dst, &status );
    return;
}

static void
copy_i4( u_i1 *src, u_i1 *dst )
{
    STATUS	status;
    CV2L_I4_MACRO( src, dst, &status );
    return;
}

static void
copy_f4( u_i1 *src, u_i1 *dst )
{
    STATUS	status;
    CV2L_F4_MACRO( src, dst, &status );
    return;
}

static void
copy_f8( u_i1 *src, u_i1 *dst )
{
    STATUS	status;
    CV2L_F8_MACRO( src, dst, &status );
    return;
}


/*
** Name: copy_atom
**
** Description:
**	Copies an atomic value out of the connection
**	message list.  Atomic values are copied whole
**	and may not be split across message segments.
**	The caller must provide a function to copy the
**	atomic value.
**
** Input:
**	ccb	Connection control block.
**	length	Length of atomic value.
**	func	Function to copy atomic value.
**
** Output:
**	ptr	Atomic value.
**
** Returns:
**	bool	TRUE if atomic value copied, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

static bool
copy_atom
( 
    JDBC_CCB	*ccb, 
    u_i2	length, 
    void	(*func)( u_i1 *, u_i1 * ), 
    u_i1	*ptr
)
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    JDBC_RCB *rcb = (JDBC_RCB *)ccb->msg.msg_q.q_next;

	    if ( rcb->msg.msg_len )
		if ( length > rcb->msg.msg_len )
		{
		    /*
		    ** Atomic values must not be split.
		    */
		    break;
		}
		else
		{
		    (*func)( &rcb->buffer[ rcb->buf_ptr ], ptr );
		    rcb->buf_ptr += length;
		    rcb->buf_len -= length;
		    rcb->msg.msg_len -= length;
		    ccb->msg.msg_len -= length;
		    length = 0;
		}

	    if ( ! rcb->msg.msg_len )
	    {
		QUremove( &rcb->q );
		jdbc_del_rcb( rcb );
	    }
	}

    return( ! length );
}


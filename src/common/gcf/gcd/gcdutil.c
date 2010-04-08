/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <me.h>
#include <qu.h>
#include <st.h>
#include <sp.h>
#include <mo.h>

#include <iicommon.h>
#include <gcm.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdutil.c
**
** Description:
**	GCD server utility functions.
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	15-Jan-03 (gordy)
**	    Reworked RCB buffer parameters.
**	14-Feb-03 (gordy)
**	    Added support for savepoints: gcd_release_sp().
**	 6-Jun-03 (gordy)
**	    CIB is now the DBMS connection MIB object.
**	15-Mar-04 (gordy)
**	    Added functions to handle i8 values.
**	16-Jun-04 (gordy)
**	    Add mask to encryption key.
**	18-Aug-04 (gordy)
**	    Allow for data which exceeds 64K.
**      30-Apr-07 (rajus01, Fei Ge ) SD issue 114414, Bug # 117551 
**	    Fixed JDBC application crash when statement.setFetchSize() is 
**	    set to a bigger value.
**	21-Apr-06 (gordy)
**	    Added outbound message info queue. Simplified PCB/RCB handling.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level.
**	23-Mar-09 (gordy)
**	    Support long database object names.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions on cursor names.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN data type.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	25-Mar-10 (gordy)
**	    Added gcd_combine_qdata() for batch processing support.
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
static	void	copy_i8( u_i1 *, u_i1 * );
static	void	copy_f4( u_i1 *, u_i1 * );
static	void	copy_f8( u_i1 *, u_i1 * );
static	bool	copy_atom( GCD_CCB *, u_i2, 
			   void (*)( u_i1 *, u_i1 * ), u_i1 * );


/*
** Holds the API environment handle for each supported API version.
*/
static struct
{
    u_i2	api_vers;          /* API version */
    PTR 	api_envhndl;       /* API environment handle */
}api_env[] =
{
    { IIAPI_VERSION_1, 0 },
    { IIAPI_VERSION_2, 0 },
    { IIAPI_VERSION_3, 0 },
    { IIAPI_VERSION_4, 0 },
    { IIAPI_VERSION_5, 0 },
    { IIAPI_VERSION_6, 0 },
    { IIAPI_VERSION_7, 0 },
};


/*
** Name: gcd_get_env
**
** Description:
**	Initializes environment handle for the given API version. If exists
**	one in the static storage returns it.
**
** Input:
**	api_vers	API version.
**	hndl		API environment handle
**
** Output:
**	hndl		API environment handle.
**
** Returns:
**	STATUS.	
**
** History:
**	 20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906
**	    Created.
**	26-Oct-09 (gordy)
**	    Added API version 7.
*/

STATUS 
gcd_get_env( u_i2 api_vers, PTR *hndl )
{
    STATUS status = OK;

    switch( api_vers )
    {
    case IIAPI_VERSION_1 :
    case IIAPI_VERSION_2 :
    case IIAPI_VERSION_3 :
    case IIAPI_VERSION_4 :
    case IIAPI_VERSION_5 :
    case IIAPI_VERSION_6 :
    case IIAPI_VERSION_7 :
    {
	u_i2 version = api_vers - 1;

	if ( api_env[ version ].api_envhndl )
	    *hndl = api_env[ version ].api_envhndl;
	else
	{
	    status = gcd_api_init( api_vers,  hndl );
	    if ( status == OK )  api_env[ version ].api_envhndl = *hndl;  
	}
	break;
    }
    default:
	status = FAIL;
	break;
    }

    return( status );
}


/*
** Name: gcd_rel_env
**
** Description:
**	Release environment handle for the given API version. If the 
**	API version is 0, it releases all the environment handles initialized
**	previously.
**
** Input:
**	api_vers	API version.
**
** Output:
**	None.	
**
** Returns:
**	None.	
**
** History:
**	 20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906
**	    Created.
*/
void 
gcd_rel_env( u_i2 api_vers )
{
    switch( api_vers )
    {
	case IIAPI_VERSION_1:
	case IIAPI_VERSION_2:
	case IIAPI_VERSION_3:
	case IIAPI_VERSION_4:
	case IIAPI_VERSION_5:
	case IIAPI_VERSION_6:
	{
	    u_i2 version = api_vers - 1;
	    if(  api_env[version].api_envhndl )
	    {
		gcd_api_term( api_env[version].api_envhndl );
		api_env[version].api_envhndl = NULL;
	    }
	}
	break;
	default:
        {
	    int i;
	    for ( i=0; i < IIAPI_VERSION_6; i++ )
		if( api_env[i].api_envhndl )
		{
		    gcd_api_term( api_env[i].api_envhndl );
		    api_env[i].api_envhndl = NULL ;
		}
        }
	break;
    }

    return;
}


/*
** Name: gcd_new_ccb
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
**	GCD_CCB *	New client connection control block.
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
**	21-Apr-06 (gordy)
**	    Added message INFO queue.
*/

GCD_CCB *
gcd_new_ccb( void )
{
    GCD_CCB	*ccb;

    if ( ! (ccb = (GCD_CCB *)QUremove( GCD_global.ccb_free.q_next )) )
	if ( (ccb = (GCD_CCB *)MEreqmem( 0, sizeof(GCD_CCB), FALSE, NULL )) )
	    ccb->id = GCD_global.ccb_total++;

    if ( ! ccb )
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
    else
    {
	u_i4	id = ccb->id;

	MEfill( sizeof( GCD_CCB ), 0, (PTR)ccb );
	ccb->id = id;
	ccb->use_cnt = 1;
	ccb->max_buff_len = 1 << DAM_TL_PKT_MIN;
	ccb->tl.proto_lvl = DAM_TL_PROTO_1;
	ccb->xact.auto_mode = GCD_XACM_DBMS;
	ccb->api.env = NULL;
	QUinit( &ccb->q );
	QUinit( &ccb->gcc.send_q );
	QUinit( &ccb->msg.msg_q );
	QUinit( &ccb->msg.info_q );
	QUinit( &ccb->stmt.stmt_q );
	QUinit( &ccb->rqst.rqst_q );

	GCD_global.ccb_active++;
	QUinsert( &ccb->q, &GCD_global.ccb_q );

	if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD new CCB (%d)\n", ccb->id, ccb->id );
    }

    return( ccb );
}


/*
** Name: gcd_del_ccb
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
**	15-Jan-03 (gordy)
**	    Free client info.  Removed XOFF RCB.
**	14-Feb-03 (gordy)
**	    Free savepoints.
**	23-Mar-09 (gordy)
**	    Free request parameters.
**	21-Jul-09 (gordy)
**	    Cursor name now dynamically allocated.
**	25-Mar-10 (gordy)
**	    Free batch database procedure info.
*/

void
gcd_del_ccb( GCD_CCB *ccb )
{
    if ( ccb->use_cnt )  ccb->use_cnt--;

    if ( ccb->use_cnt )
    {
	if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD del CCB (use count %d)\n", 
		       ccb->id, ccb->use_cnt );
    }
    else
    {
	GCD_RCB *rcb;
	GCD_SCB *scb;
	QUEUE	 *q;

	QUremove( &ccb->q );
	GCD_global.ccb_active--;
	if ( ccb->qry.crsr_name )  MEfree( (PTR)ccb->qry.crsr_name );
	if ( ccb->qry.schema_name )  MEfree( (PTR)ccb->qry.schema_name );
	if ( ccb->qry.proc_name )  MEfree( (PTR)ccb->qry.proc_name );
	if ( ccb->qry.qtxt_max )  MEfree( (PTR)ccb->qry.qtxt );
	if ( ccb->rqst.rqst_id0 )  MEfree( (PTR)ccb->rqst.rqst_id0 );
	if ( ccb->rqst.rqst_id1 )  MEfree( (PTR)ccb->rqst.rqst_id1 );
	if ( ccb->client_user )  MEfree( (PTR)ccb->client_user );
	if ( ccb->client_host )  MEfree( (PTR)ccb->client_host );
	if ( ccb->client_addr )  MEfree( (PTR)ccb->client_addr );
	gcd_free_qdata( &ccb->qry.svc_parms );
	gcd_free_qdata( &ccb->qry.qry_parms );
	gcd_free_qdata( &ccb->qry.all_parms );

	/*
	** Free control blocks and request queues.
	*/
	if ( ccb->cib )  gcd_del_cib( ccb->cib );

	if ( ccb->xact.savepoints )  gcd_release_sp( ccb, NULL );

	if ( ccb->msg.xoff_pcb )
	    gcd_del_pcb( (GCD_PCB *)ccb->msg.xoff_pcb );

	if ( ccb->gcc.abort_rcb )
	    gcd_del_rcb( (GCD_RCB *)ccb->gcc.abort_rcb );

	while( (scb = (GCD_SCB *)QUremove( ccb->stmt.stmt_q.q_next )) )
	{
	    gcd_free_qdata( &scb->column );
	    MEfree( (PTR)scb );
	}

	while( (rcb = (GCD_RCB *)QUremove( ccb->gcc.send_q.q_next )) )
	    gcd_del_rcb( rcb );

	while( (rcb = (GCD_RCB *)QUremove( ccb->msg.msg_q.q_next )) )
	    gcd_del_rcb( rcb );

	while( (rcb = (GCD_RCB *)QUremove( ccb->msg.info_q.q_next )) )
	    gcd_del_rcb( rcb );

	while( (q = QUremove( ccb->rqst.rqst_q.q_next )) )
	    MEfree( (PTR)q );

	/*
	** Save CCB on free queue.
	*/
	QUinsert( &ccb->q, GCD_global.ccb_free.q_prev );

	if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD del CCB (%d)\n", ccb->id, ccb->id );
    }

    return;
}


/*
** Name: gcd_new_cib
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
**	GCD_CIB *	New Connection Information Block.
**
** History:
**	 6-Jan-00 (gordy)
**	    Created.
**	 3-Mar-00 (gordy)
**	    Implement free queue for CIB.
**	 6-Jun-03 (gordy)
**	    Attach CIB as DBMS connection MIB object.
*/

GCD_CIB *
gcd_new_cib( u_i2 count )
{
    GCD_CIB	*cib = NULL;
    u_i2	len = sizeof( GCD_CIB ) + (sizeof(CONN_PARM) * (count - 1));

    if ( count < ARR_SIZE( GCD_global.cib_free ) )
	if (! (cib = (GCD_CIB *)QUremove(GCD_global.cib_free[count].q_next)))
	    if ( (cib = (GCD_CIB *)MEreqmem( 0, len, FALSE, NULL )) )
		cib->id = GCD_global.cib_total++;

    if ( ! cib )
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
    else  
    {
	u_i4	id = cib->id;
	char	buff[16];

	MEfill( len, 0, (PTR)cib );
	cib->id = id;
	cib->parm_max = count;
	QUinit( &cib->caps );

	MOulongout( 0, (u_i8)cib->id, sizeof( buff ), buff );
	MOattach( MO_INSTANCE_VAR, GCD_MIB_DBMS, buff, (PTR)cib );

	GCD_global.cib_active++;

	if ( GCD_global.gcd_trace_level >= 6 )
	    TRdisplay( "%4d    GCD new CIB (%d)\n", -1, cib->id );
    }

    return( cib );
}


/*
** Name: gcd_del_cib
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
**	 6-Jun-03 (gordy)
**	    Detach CIB from MIB.
*/

void
gcd_del_cib( GCD_CIB *cib )
{
    GCD_CAPS	*caps;
    char	buff[16];
    u_i2	i;

    GCD_global.cib_active--;

    MOulongout( 0, (u_i8)cib->id, sizeof( buff ), buff );
    MOdetach( GCD_MIB_DBMS, buff );

    while( (caps = (GCD_CAPS *)QUremove( cib->caps.q_next )) )
    {
	MEfree( caps->data );
	MEfree( (PTR)caps );
    }

    if ( cib->database )  MEfree( (PTR)cib->database );
    if ( cib->username )  MEfree( (PTR)cib->username );
    if ( cib->password )  MEfree( (PTR)cib->password );
    for( i = 0; i < cib->parm_cnt; i++ )  MEfree( (PTR)cib->parms[i].value );

    if ( GCD_global.gcd_trace_level >= 6 )
	TRdisplay( "%4d    GCD del CIB (%d)\n", -1, cib->id );

    if ( cib->parm_max < ARR_SIZE( GCD_global.cib_free ) )
	QUinsert( &cib->q, GCD_global.cib_free[ cib->parm_max ].q_prev );
    else
    {
	MEfree( (PTR)cib );
	GCD_global.cib_total--;
    }

    return;
}


/*
** Name: gcd_release_sp
**
** Description:
**	Free savepoint control blocks upto but excluding the
**	target savepoint.  Use NULL to free all savepoints.
**
**	Warning: all savepoints will be freed if target is
**	not in the savepoint list.
**
** Input:
**	cib	Connection Information Block.
**	target	Target savepoint.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

void
gcd_release_sp( GCD_CCB *ccb, GCD_SPCB *target )
{
    while( ccb->xact.savepoints  &&  ccb->xact.savepoints != target )
    {
	GCD_SPCB *sp = ccb->xact.savepoints;
	ccb->xact.savepoints = sp->next;
	sp->next = NULL;
	MEfree( (PTR)sp );
    }
    return;
}


/*
** Name: gcd_new_rcb
**
** Description:
**	Allocate a new request control block.  The data buffer
**	is initialized for building MSG layer messages.  The
**	buffer size allocated may be defaulted or requested
**	as follows:
**
**	    len < 0	Default negotiated buffer size.
**	    len = 0	No buffer allocation.
**	    len > 0	Allocate buffer of size len.
**
**	Requested buffer length will be increased to allow room
**	for a TL header.
**
** Input:
**	ccb		Connection control block.
**	len		Buffer length.
**
** Output:
**	None.
**
** Returns:
**	GCD_RCB *	New request control block.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Reworked RCB buffer fields to simplify operations.
**	21-Apr-06 (gordy)
**	    Added buffer length request instead of optional CCB
**	    to determine buffer allocation.
*/

GCD_RCB *
gcd_new_rcb( GCD_CCB *ccb, i2 len )
{
    GCD_RCB *rcb = (GCD_RCB *)MEreqmem( 0, sizeof(GCD_RCB), TRUE, NULL );

    if ( rcb )
    {
	rcb->ccb = ccb;

        if ( len )
	{
	    /*
	    ** Increase requested buffer length by the TL header
	    ** size or use the negotiated TL protocol buffer size
	    ** from the CCB (TL header length already included).
	    */
	    if ( len > 0 )  
		len = min( len + GCD_global.tl_hdr_sz, ccb->max_buff_len );
	    else
	        len = ccb->max_buff_len;

	    /*
	    ** Need to provide space for NL header.
	    */
	    rcb->buf_max = (u_i2)(len + GCD_global.nl_hdr_sz);
	    rcb->buffer = (u_i1 *)MEreqmem( 0, rcb->buf_max, FALSE, NULL );

	    if ( rcb->buffer )
		gcd_init_rcb( rcb );
	    else
	    {
		MEfree( (PTR)rcb );
		rcb = NULL;
	    }
	}
    }

    if ( ! rcb )
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
    else if ( GCD_global.gcd_trace_level >= 6 )
	TRdisplay( "%4d    GCD new RCB (%p)\n", -1, rcb );

    return( rcb );
}


/*
** Name: gcd_init_rcb
**
** Description:
**	Initialize the RCB data buffer in preparation
**	for building MSG layer messages.  Reserves
**	sufficient space at start of buffer for lower
**	layer packaging.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

void
gcd_init_rcb( GCD_RCB *rcb )
{
    /* 
    ** Reserver space for NL & TL headers.  
    */
    rcb->buf_ptr = rcb->buffer + GCD_global.nl_hdr_sz + GCD_global.tl_hdr_sz;
    rcb->buf_len = 0;
    return;
}


/*
** Name: gcd_del_rcb
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
gcd_del_rcb( GCD_RCB *rcb )
{
    if ( GCD_global.gcd_trace_level >= 6 )
	TRdisplay( "%4d    GCD del RCB (%p)\n", -1, rcb );

    if ( rcb->buffer )  MEfree( (PTR)rcb->buffer );
    MEfree( (PTR)rcb );
    return;
}


/*
** Name: gcd_new_pcb
**
** Description:
**	Allocate a new parameter control block.
**
** Input:
**	ccb		Connection control block.
**
** Output:
**	None.
**
** Returns:
**	GCD_PCB *	New parameter control block.
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	21-Apr-06 (gordy)
**	    No longer need to pre-allocate RCB.
*/

GCD_PCB *
gcd_new_pcb( GCD_CCB *ccb )
{
    GCD_PCB *pcb;

    if ( ! (pcb = (GCD_PCB *)MEreqmem( 0, sizeof( GCD_PCB ), TRUE, NULL )) )
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
    else
    {
	if ( GCD_global.gcd_trace_level >= 6 )
	    TRdisplay( "%4d    GCD new PCB (%p)\n", ccb->id, pcb );

	pcb->ccb = ccb;
    }

    return( pcb );
}


/*
** Name: gcd_del_pcb
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
gcd_del_pcb( GCD_PCB *pcb )
{
    if ( GCD_global.gcd_trace_level >= 6 )
	TRdisplay( "%4d    GCD del PCB (%p)\n", pcb->ccb->id, pcb );

    if ( pcb->rcb )  gcd_del_rcb( pcb->rcb );
    MEfree( (PTR)pcb );
    return;
}

/*
** Name: gcd_alloc_qdesc
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
gcd_alloc_qdesc( QDATA *qdata, u_i2 count, PTR desc )
{
    qdata->max_rows = 0;
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
		gcu_erlog( 0, GCD_global.language, 
			   E_GC4808_NO_MEMORY, NULL, 0, NULL);
		return( FALSE );
	    }

	    qdata->desc_max = qdata->max_cols;
	}

	qdata->desc = qdata->param_desc;
    }

    return( TRUE );
}

/*
** Name: gcd_alloc_qdata
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
**	18-Aug-04 (gordy)
**	    Length can overflow an i2, changed to i4.
*/

bool
gcd_alloc_qdata( QDATA *qdata, u_i2 max_rows )
{
    return( alloc_qdata( qdata, max_rows, TRUE ) );
}

static bool
alloc_qdata( QDATA *qdata, u_i2 max_rows, bool alloc_buffers )
{
    IIAPI_DESCRIPTOR	*desc;
    IIAPI_DATAVALUE	*data;
    u_i2		row, col, seg_len;
    u_i4                dv;
    u_i4		blen, length;
    u_i4		tot_cols = max_rows * qdata->max_cols;

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
	    gcu_erlog( 0, GCD_global.language, 
		       E_GC4808_NO_MEMORY, NULL, 0, NULL);
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
	    gcu_erlog( 0, GCD_global.language, 
		       E_GC4808_NO_MEMORY, NULL, 0, NULL);
	    return( FALSE );
	}

	qdata->db_max = blen;
    }

    if ( seg_len > qdata->bb_max )
    {
	if ( qdata->blob_buff )  MEfree( qdata->blob_buff );
	if ( ! (qdata->blob_buff = MEreqmem( 0, seg_len, FALSE, NULL )) )
	{
	    gcu_erlog( 0, GCD_global.language, 
		       E_GC4808_NO_MEMORY, NULL, 0, NULL);
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
** Name: gcd_build_qdata
**
** Description:
**	Create a new QDATA using two existing QDATA structures
**	Descriptor entries marked as type TUPLE are removed 
**	and not included in the combined set.  Data buffers 
**	are not allocated for the new QDATA, but the data 
**	array is allocated so that buffers from the original 
**	QDATA structures may be shared (gcd_combine_qdata()).
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
gcd_build_qdata( QDATA *qd1, QDATA *qd2, QDATA *qd3 )
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
    if ( ! gcd_alloc_qdesc( qd3, count, NULL )  ||
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
** Name: gcd_combine_qdata
**
** Description:
**	Combine the available parameters from two QDATA
**	parameter sets (API service parameters and query
**	parameters) into a third QDATA set which was
**	generated from the two parameters sets using
**	gcd_build_qdata().
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
**	25-Mar-10 (gordy)
**	    Created.
*/

void
gcd_combine_qdata( QDATA *svc, QDATA *qry, QDATA *all )
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


/*
** Name: gcd_free_qdata
**
** Description:
**	Free memory allocated by gcd_alloc_qdesc()
**	and gcd_alloc_qdata().
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
gcd_free_qdata( QDATA *qdata )
{
    qdata->max_rows = 0;
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
** Name: gcd_decode
**
** Description:
**	Decrypt data with provided key using Ingres CL module CI.
**	The input and output buffers may be the same.  The key
**	must be a null terminated string as must the resulting
**	decoded data.  An optional binary mask may be provided
**	which will be combined with the key.  The mask must be
**	eight-bytes in length.
**
** Input:
**	key	Encryption key.
**	mask	Optional binary mask.
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
**	16-Jun-04 (gordy)
**	    Added mask parameter.
*/

void
gcd_decode( char *key, u_i1 *mask, u_i1 *data, u_i2 length, char *buff )
{
    CI_KS	ks;
    u_i1	kbuff[ CRYPT_SIZE ];
    char	*ptr;
    u_i2	i, j;

    /*
    ** The key schedule is built from a single CRYPT_SIZE
    ** byte array.  We use the input key to build the byte
    ** array, truncating or duplicating as necessary.
    ** The key is also combined with an optional mask.
    */
    for( i = 0, ptr = key; i < CRYPT_SIZE; i++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ i ] = *ptr++ ^ (mask ? mask[ i ] : 0);
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
** Name: gcd_put_[i,f][1,2,4]{p}
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
**	21-Apr-06 (gordy)
**	    Make sure RCB exists.
*/

bool
gcd_put_i1( GCD_RCB **rcb_ptr, u_i1 val )
{
    return( gcd_put_i1p( rcb_ptr, &val ) );
}

bool
gcd_put_i1p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_I1_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I1_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    return( TRUE );
}

bool
gcd_put_i2( GCD_RCB **rcb_ptr, u_i2 val )
{
    return( gcd_put_i2p( rcb_ptr, (u_i1 *)&val ) );
}

bool
gcd_put_i2p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_I2_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I2_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I2_SZ;

    return( TRUE );
}

bool
gcd_put_i4( GCD_RCB **rcb_ptr, u_i4 val )
{
    return( gcd_put_i4p( rcb_ptr, (u_i1 *)&val ) );
}

bool
gcd_put_i4p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_I4_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I4_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I4_SZ;

    return( TRUE );
}

bool
gcd_put_i8( GCD_RCB **rcb_ptr, u_i8 val )
{
    return( gcd_put_i8p( rcb_ptr, (u_i1 *)&val ) );
}

bool
gcd_put_i8p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_I8_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_I8_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I8_SZ;

    return( TRUE );
}

bool
gcd_put_f4p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_F4_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_F4_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_F4_SZ;

    return( TRUE );
}

bool
gcd_put_f8p( GCD_RCB **rcb_ptr, u_i1 *val )
{
    GCD_RCB	*rcb = *rcb_ptr;
    STATUS	status;

    if ( ! rcb )  return( FALSE );

    if ( CV_N_F8_SZ > RCB_AVAIL( rcb ) )  
	if ( ! gcd_msg_split( rcb_ptr ) )  
	    return( FALSE );
	else
	    rcb = *rcb_ptr;

    CV2N_F8_MACRO( val, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_F8_SZ;

    return( TRUE );
}


/*
** Name: gcd_put_bytes
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
**	21-Apr-06 (gordy)
**	    Make sure RCB exists.
*/

bool
gcd_put_bytes( GCD_RCB **rcb_ptr, u_i2 length, u_i1 *ptr )
{
    GCD_RCB	*rcb = *rcb_ptr;
    u_i2	len;

    if ( ! rcb )  return( FALSE );

    while( length )
    {
	if ( RCB_AVAIL( rcb ) < 1 )
	    if ( ! gcd_msg_split( rcb_ptr ) )  
		return( FALSE );
	    else
		rcb = *rcb_ptr;

	len = min( length, RCB_AVAIL( rcb ) );
	MEcopy( (PTR)ptr, len, (PTR)&rcb->buf_ptr[ rcb->buf_len ] );
	rcb->buf_len += len;
	ptr += len;
	length -= len;
    }

    return( TRUE );
}


/*
** Name: gcd_put_ucs2
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
**	21-Apr-06 (gordy)
**	    Make sure RCB exists.
*/

bool
gcd_put_ucs2( GCD_RCB **rcb_ptr, u_i2 length, u_i1 *ptr )
{
    GCD_RCB	*rcb = *rcb_ptr;
    u_i2	len;

    if ( ! rcb )  return( FALSE );

    length *= sizeof( UCS2 );	/* convert array length to byte length */

    while( length )
    {
	if ( RCB_AVAIL( rcb ) < CV_N_I2_SZ )
	    if ( ! gcd_msg_split( rcb_ptr ) )  
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

	    CV2N_I2_MACRO( ptr, &rcb->buf_ptr[ rcb->buf_len ], &status );
	    rcb->buf_len += CV_N_I2_SZ;
	    ptr += sizeof( UCS2 );
	    length -= sizeof( UCS2 );
	}
    }

    return( TRUE );
}


/*
** Name: gcd_get_i{1,2,4}
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
gcd_get_i1( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I1_SZ, copy_i1, ptr ) );
}

bool
gcd_get_i2( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I2_SZ, copy_i2, (u_i1 *)ptr ) );
}

bool
gcd_get_i4( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I4_SZ, copy_i4, (u_i1 *)ptr ) );
}

bool
gcd_get_i8( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_I8_SZ, copy_i8, (u_i1 *)ptr ) );
}

bool
gcd_get_f4( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_F4_SZ, copy_f4, (u_i1 *)ptr ) );
}

bool
gcd_get_f8( GCD_CCB *ccb, u_i1 *ptr )
{
    return( copy_atom( ccb, CV_N_F8_SZ, copy_f8, (u_i1 *)ptr ) );
}


/*
** Name: gcd_get_bytes
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
gcd_get_bytes( GCD_CCB *ccb, u_i2 length, u_i1 *ptr )
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    GCD_RCB	*rcb = (GCD_RCB *)ccb->msg.msg_q.q_next;
	    u_i2	len;

	    if ( (len = min( length, rcb->msg.msg_len )) )
	    {
		MEcopy( (PTR)rcb->buf_ptr, len, (PTR)ptr );
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
		gcd_del_rcb( rcb );
	    }
	}

    return( ! length );
}


/*
** Name: gcd_get_ucs2
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
gcd_get_ucs2( GCD_CCB *ccb, u_i2 length, u_i1 *ptr )
{
    for( ; length > 0; length--, ptr += sizeof( UCS2 ) )
	if ( ! copy_atom( ccb, CV_N_I2_SZ, copy_i2, (u_i1 *)ptr ) )
	    break;

    return( ! length );
}


/*
** Name: gcd_skip
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
gcd_skip( GCD_CCB *ccb, u_i2 length )
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    GCD_RCB	*rcb = (GCD_RCB *)ccb->msg.msg_q.q_next;
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
		gcd_del_rcb( rcb );
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
copy_i8( u_i1 *src, u_i1 *dst )
{
    STATUS	status;
    CV2L_I8_MACRO( src, dst, &status );
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
    GCD_CCB	*ccb, 
    u_i2	length, 
    void	(*func)( u_i1 *, u_i1 * ), 
    u_i1	*ptr
)
{
    if ( length <= ccb->msg.msg_len )
	while( length )
	{
	    GCD_RCB *rcb = (GCD_RCB *)ccb->msg.msg_q.q_next;

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
		    (*func)( rcb->buf_ptr, ptr );
		    rcb->buf_ptr += length;
		    rcb->buf_len -= length;
		    rcb->msg.msg_len -= length;
		    ccb->msg.msg_len -= length;
		    length = 0;
		}

	    if ( ! rcb->msg.msg_len )
	    {
		QUremove( &rcb->q );
		gcd_del_rcb( rcb );
	    }
	}

    return( ! length );
}


/*
** Copyright (c) 2004, 2009 Ingres Corporation
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
** Name: gcdcrsr.c
**
** Description:
**	GCD cursor message processing.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Extracted to this file.
**	 4-Nov-99 (gordy)
**	    Updated calling sequence of jdbc_send_done() to allow
**	    control of ownership of RCB parameter.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	19-May-00 (gordy)
**	    Separate request/result row counters to better
**	    detect end-of-tuples.  Only deactivate a statement
**	    when end-of-tuples detected or for cursors at the
**	    end of the fetch.
**	 3-Aug-00 (gordy)
**	    Get query info when statement goes dormant to pick
**	    up the query results.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	19-Oct-00 (gordy)
**	    Signal cursor close to transaction sub-system to
**	    allow handling of autocommit simulation.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 2-Jul-01 (gordy)
**	    Make sure UCS2 characters are not split across buffers.
**	13-Nov-01 (loera01) SIR 106366
**	    Conditionally compile Unicode support.
**	    BACKED OUT, NOT NEEDED IN MAIN. (hanje04)
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	15-Jan-03 (gordy)
**	    Added gcd_sess_abort().
**	 4-Aug-03 (gordy)
**	    Query data buffers can now be dynamically increased
**	    to allow pre-fetch limits to be set by client rather
**	    than limited to server calculation.  Special handling
**	    added for BYREF parameter results.
**	20-Aug-03 (gordy)
**	    Added ability to automatically close statement at EOD.
**	 6-Oct-03 (gordy)
**	    Reorganized into state machines for fetch/close.  Added
**	    entry point for query fetch actions.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Clear active statement if error in FETCH.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time types.
**	29-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 4-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 9-Sep-08 (gordy)
**	    Expand temp buffer to support larger decimal precision.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs or NBR type as BYTE.

*/	

/*
** Cursor requests.
*/

static	GCULIST curMap[] =
{
    { MSG_CUR_CLOSE,	"CLOSE" },
    { MSG_CUR_FETCH,	"FETCH" },
    { 0, NULL }
};

/*
** Fetch sequencing.
*/

#define	FETCH_INIT	0
#define	FETCH_COLS	1
#define	FETCH_POS	2
#define	FETCH_FETCH	3
#define	FETCH_ROWS	4
#define	FETCH_QINFO	5
#define	FETCH_CLEAR	6
#define	FETCH_DONE	7
#define	FETCH_CHECK	8
#define	FETCH_EXIT	9

static	GCULIST	fetchMap[] =
{
    { FETCH_INIT,	"INIT" },
    { FETCH_COLS,	"COLUMNS" },
    { FETCH_POS,	"POSITION" },
    { FETCH_FETCH,	"FETCH" },
    { FETCH_ROWS,	"ROWS" },
    { FETCH_QINFO,	"QINFO" },
    { FETCH_CLEAR,	"CLEAR" },
    { FETCH_DONE,	"DONE" },
    { FETCH_CHECK,	"CHECK" },
    { FETCH_EXIT,	"EXIT" },
    { 0, NULL }
};

/*
** Close sequencing.
*/

#define	CLOSE_CURSOR	0
#define	CLOSE_XACT	1
#define	CLOSE_DONE	2

static	GCULIST	closeMap[] =
{
    { CLOSE_CURSOR,	"CLOSE" },
    { CLOSE_XACT,	"XACT" },
    { CLOSE_DONE,	"DONE" },
    { 0, NULL }
};

static	void	crsr_init_fetch( GCD_CCB *, GCD_SCB *, GCD_PCB *, u_i4 );
static	void	crsr_fetch_sm( PTR );
static	void	crsr_close_sm( PTR );
static	bool	crsr_rows( GCD_CCB *, GCD_SCB *, GCD_PCB * );
static	void	crsr_cols( GCD_CCB *, GCD_SCB *, GCD_RCB **, u_i2, bool );


/*
** Name: gcd_msg_cursor
**
** Description:
**	Process a GCD cursor message.
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
**	13-Dec-99 (gordy)
**	    Check for the cursor pre-fetch parameter.
**	19-May-00 (gordy)
**	    Separate request/result row counters to better
**	    detect end-of-tuples.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	 6-Oct-03 (gordy)
**	    Fetch initialization moved to crsr_fetch_init() and
**	    other processing code moved into crsr_fetch_sm() and
**	    crsr_close_sm().
**	29-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	 4-Apr-07 (gordy)
**	    Support positioned fetchs.
*/

void
gcd_msg_cursor( GCD_CCB *ccb )
{
    DAM_ML_CUR	cursor;
    STMT_ID	stmt_id;
    GCD_SCB	*scb;
    GCD_PCB	*pcb;
    STATUS	status;
    u_i2	pos_anchor = MSG_POS_CURRENT;	/* Default to NEXT */
    i4		pos_offset = 1;
    u_i4	pre_fetch = 1;
    bool	incomplete = FALSE;
    bool	got_id = FALSE;

    if ( ! gcd_get_i2( ccb, (u_i1 *)&cursor.cursor_op ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no cursor op specified\n", ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Cursor operation: %s\n", 
		   ccb->id, gcu_lookup( curMap, cursor.cursor_op ) );

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	if ( pcb )  gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return;
    }

    while( ccb->msg.msg_len )
    {
	DAM_ML_PM	cp;
	u_i1		u_i1_val;
	u_i2		u_i2_val;
	u_i4		u_i4_val;

	incomplete = TRUE;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&cp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&cp.val_len ) )  break;

	switch( cp.param_id )
	{
	case MSG_CUR_STMT_ID :
	    if ( cp.val_len != (CV_N_I4_SZ * 2)  ||
	         ! gcd_get_i4( ccb, (u_i1 *)&stmt_id.id_high )  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&stmt_id.id_low ) )
		break;

	    incomplete = FALSE;
	    got_id = TRUE;
	    break;
	
	case MSG_CUR_PRE_FETCH :
	    switch( cp.val_len )
	    {
	    case 1 : 
		if ( gcd_get_i1( ccb, (u_i1 *)&u_i1_val ) )
		{
		    pre_fetch = max( pre_fetch, u_i1_val );
		    incomplete = FALSE;
		}
		break;

	    case 2 :
		if ( gcd_get_i2( ccb, (u_i1 *)&u_i2_val ) )
		{
		    pre_fetch = max( pre_fetch, u_i2_val );
		    incomplete = FALSE;
		}
		break;

	    case 4 :
		if ( gcd_get_i4( ccb, (u_i1 *)&u_i4_val ) )
		{
		    pre_fetch = max( pre_fetch, u_i4_val );
		    incomplete = FALSE;
		}
		break;
	    }
	    break;

	case MSG_CUR_POS_ANCHOR :
	    switch( cp.val_len )
	    {
	    case 1 : 
		if ( gcd_get_i1( ccb, (u_i1 *)&u_i1_val ) )
		{
		    pos_anchor = (u_i2)u_i1_val;
		    incomplete = FALSE;
		}
		break;

	    case 2 :
		if ( gcd_get_i2( ccb, (u_i1 *)&u_i2_val ) )
		{
		    pos_anchor = (u_i2)u_i2_val;
		    incomplete = FALSE;
		}
		break;

	    case 4 :
		if ( gcd_get_i4( ccb, (u_i1 *)&u_i4_val ) )
		{
		    pos_anchor = (u_i2)u_i2_val;
		    incomplete = FALSE;
		}
		break;
	    }
	    break;

	case MSG_CUR_POS_OFFSET :
	    if ( cp.val_len != CV_N_I4_SZ  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&pos_offset ) )
		break;

	    incomplete = FALSE;
	    break;
	
	default :
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     unkown parameter ID %d\n",
			   ccb->id, cp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD unable to read all cursor parameters %d\n",
		       ccb->id );
	status = E_GC480A_PROTOCOL_ERROR;
    }
    else  if ( ! got_id )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no cursor ID for cursor message\n", 
			ccb->id );
	status = E_GC480A_PROTOCOL_ERROR;
    }
    else
	status = OK;

    if ( status != OK )
    {
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, status );
	return;
    }

    if ( ! (scb = gcd_find_stmt( ccb, &stmt_id )) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD no active statement for cursor ID\n", 
			ccb->id );
	gcd_sess_error( ccb, &pcb->rcb, E_GC4811_NO_STMT );
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );
	gcd_msg_pause( ccb, TRUE );
	return;
    }

    switch( cursor.cursor_op )
    {
    case MSG_CUR_CLOSE :
	ccb->sequence = CLOSE_CURSOR;
	pcb->scb = scb;
	crsr_close_sm( (PTR)pcb );
	break;
    
    case MSG_CUR_FETCH :
	ccb->sequence = FETCH_INIT;
	ccb->api.stmt = scb->handle;		/* Activate statement */
	pcb->scb = scb;
	pcb->data.data.reference = pos_anchor;
	pcb->data.data.offset = pos_offset;
	pcb->data.data.max_row = pre_fetch;

	crsr_fetch_sm( (PTR)pcb );
	break;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid cursor op: %d\n", 
			ccb->id, cursor.cursor_op );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
    }

    return;
}


/*
** Name: gcd_fetch
**
** Description:
**	Fetch rows for a statement (entry point for non-CURSOR messages).
**	Fetches all rows if FETCH_ALL flag is set in SCB.  Otherwise,
**	only first block (defined by pre_fetch size) of rows is fetched.
**
** Input:
**	ccb		Connection control block.
**	scb		Statement control block.
**	pcb		Parameter control block.
**	pre_fetch	Number of rows per fetch request.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Oct-03 (gordy)
**	    Created.
*/

void
gcd_fetch( GCD_CCB *ccb, GCD_SCB *scb, GCD_PCB *pcb, u_i4 pre_fetch )
{
    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD fetch first block of %d rows\n", 
		   ccb->id, pre_fetch );

    ccb->sequence = FETCH_INIT;
    ccb->api.stmt = scb->handle;		/* Activate statement */
    pcb->scb = scb;
    pcb->data.data.reference = MSG_POS_CURRENT;		/* Next row */
    pcb->data.data.offset = 1;
    pcb->data.data.max_row = pre_fetch;

    crsr_fetch_sm( (PTR)pcb ); 
    return;
}


/*
** Name: crsr_fetch_sm
**
** Description:
**	CURSOR message FETCH state machine.
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
**	 4-Nov-99 (gordy)
**	    Be sure to drop reference to cursor statement handle
**	    when fetch has completed.
**	13-Dec-99 (gordy)
**	    Check for cases which disable cursor pre-fetching:
**	    BLOBs and partial row fetches following BLOBs.
**	19-May-00 (gordy)
**	    Only deactivate the statement at the end-of-data, or 
**	    for cursor at the end of the fetch.  Separate request
**	    and result row counters to better detect end-of-tuples.
**	 3-Aug-00 (gordy)
**	    Get query info when statement goes dormant to pick
**	    up the query results.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	20-Aug-03 (gordy)
**	    Check to see if statement/cursor should be closed.
**	 6-Oct-03 (gordy)
**	    Renamed and expanded into FETCH state machine.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Clear active statement if error in FETCH.
**	    Also check for logical processing errors.
**	 4-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

static void
crsr_fetch_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    GCD_SCB	*scb = pcb->scb;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Fetch: %s\n", 
		    ccb->id, gcu_lookup( fetchMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case FETCH_INIT :
	/*
	** Validate current fetch state.
	*/
	if ( scb->column.cur_col != 0  ||  scb->column.more_segments )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay("%4d    GCD previous fetch not complete: (%d,%d)\n",
			  ccb->id, scb->column.cur_col, scb->column.max_cols);
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return;
	}

	/*
	** Allocate row/column buffers, if needed.
	** Note: for BYREF procedure parameters we
	** force at least two rows even though only
	** one is expected.  This allows detection
	** of end-of-data during row processing so
	** that the statement can be easily closed.
	*/
	if ( scb->flags & GCD_STMT_BYREF )  pcb->data.data.max_row = 2;

	if ( pcb->data.data.max_row > scb->column.max_rows  && 
	     ! gcd_alloc_qdata( &scb->column, pcb->data.data.max_row ) )
	{
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return;
	}

	/*
	** Skip column set-up if only positioning.
	*/
	if ( ! pcb->data.data.max_row )  ccb->sequence = FETCH_POS;
        break;

    case FETCH_COLS :
    {
	IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)scb->column.desc;
	u_i2			column;

	/*
	** If there is a BLOB column in the data set,
	** only fetch upto and including the BLOB (for
	** the first segment).  Additional segments will 
	** be fetched individually.  Remaining columns 
	** will be fetched once the BLOB is done.
	*/
	for( 
	     column = scb->column.cur_col; 
	     column < scb->column.max_cols; 
	     column++ 
	   )
	{
            i2 blobFound = 0;
	    switch( desc[ column ].ds_dataType )
            {
	    case IIAPI_LVCH_TYPE:
	    case IIAPI_LNVCH_TYPE:
	    case IIAPI_LBYTE_TYPE:
	    case IIAPI_GEOM_TYPE:
	    case IIAPI_POINT_TYPE:
	    case IIAPI_MPOINT_TYPE:
	    case IIAPI_LINE_TYPE:
	    case IIAPI_MLINE_TYPE:
	    case IIAPI_POLY_TYPE:
	    case IIAPI_MPOLY_TYPE:
	    case IIAPI_GEOMC_TYPE:
	      {
		/* Include BLOB and stop */
		column++;
		pcb->data.data.max_row = 1; 	/* BLOBs force single row */
                blobFound = 1;
		break;
	      }
            }
            if (blobFound == 1) break;
	}

	/*
	** Only one row allowed if partial row.  Preceding loop 
	** checked last column, need to check first column.
	*/
	if ( scb->column.cur_col )  pcb->data.data.max_row = 1;
	scb->column.col_cnt = column - scb->column.cur_col;
	pcb->data.data.col_cnt = scb->column.col_cnt;
	pcb->data.data.data = 
	    &((IIAPI_DATAVALUE *)scb->column.data)[ scb->column.cur_col ];

	/*
	** Skip positioning if doing simple NEXT.
	*/
	if ( pcb->data.data.reference == MSG_POS_CURRENT  &&
	     pcb->data.data.offset == 1 )
	    ccb->sequence = FETCH_FETCH;
	break;
    }

    case FETCH_POS :
	/*
	** Skip row processing if no rows are requested.
	*/
	if ( ! pcb->data.data.max_row )  ccb->sequence = FETCH_QINFO;
	gcd_push_callback( pcb, crsr_fetch_sm );
	gcd_api_position( pcb );
	return;

    case FETCH_FETCH :
	/*
	** Subsequent fetch (if any) will be simple NEXT.
	*/
	pcb->data.data.reference = MSG_POS_CURRENT;
	pcb->data.data.offset = 1;

	gcd_push_callback( pcb, crsr_fetch_sm );

	if ( ccb->msg.flags & GCD_MSG_XOFF )
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD fetch pending OB XON\n", ccb->id );
	    ccb->msg.xoff_pcb = (PTR)pcb;
	    return;
	}

	gcd_api_getCol( pcb );
	return;

    case FETCH_ROWS :
	/*
	** Send row/column data to client.  
	**
	** Fetching continues if partial row received.
	** End-of-data if didn't receive all requested rows.
	** Non-cursor queries remain active (except at EOD).
	*/
	if ( pcb->api_error )
	    ccb->sequence = FETCH_CLEAR;
	else  if ( ! crsr_rows( ccb, scb, pcb ) )
	    ccb->sequence = FETCH_COLS;			/* partial row */
	else  if ( pcb->data.data.row_cnt < pcb->data.data.max_row )
	{
	    /*
	    ** For scrollable, sensitive cursor, several reasons
	    ** exist for receiving fewer than expected rows.  EOD
	    ** is never returned for a scrollable cursors.  The
	    ** DBMS returns the EOD indication for forward-only
	    ** cursors.  For all others, EOD must be generated.
	    */
	    if ( ! (scb->flags & GCD_STMT_CURSOR) )  
		pcb->result.flags |= PCB_RSLT_EOD;	/* end-of-data */
	}
	else  if ( ! (scb->flags & GCD_STMT_CURSOR) )  
	    ccb->sequence = FETCH_DONE;			/* keep active */
	break;

    case FETCH_QINFO : 
	gcd_push_callback( pcb, crsr_fetch_sm );
	gcd_api_gqi( pcb );
	return;

    case FETCH_CLEAR :
	ccb->api.stmt = NULL;	/* Statement is now dormant. */
	if ( pcb->api_error )  ccb->sequence = FETCH_CHECK;
	break;

    case FETCH_DONE :
	/*
	** Close statement if end-of-data and 
	** AUTO_CLOSE has been requested.
	*/
	if ( pcb->result.flags & PCB_RSLT_EOD  &&
	     scb->flags & GCD_STMT_AUTO_CLOSE )
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD closing statement at EOD\n", ccb->id );
	    ccb->sequence = CLOSE_CURSOR;
	    crsr_close_sm( (PTR)pcb );
	    return;
	}

	/*
	** Continue fetching rows if requested
	** and not end-of-data.
	*/
	if ( scb->flags & GCD_STMT_FETCH_ALL  &&
	     ! (pcb->result.flags & PCB_RSLT_EOD) )
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD fetch next block of rows\n", ccb->id );
	    ccb->sequence = FETCH_COLS;
	    ccb->api.stmt = scb->handle;	/* May have cleared above */
	}
	break;

    case FETCH_CHECK :
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )  
	    break;	/* Handled below */

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	{
	    gcd_push_callback( pcb, crsr_fetch_sm );
	    gcd_xact_abort( pcb );
	    return;
	}
	break;

    case FETCH_EXIT :
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
	    TRdisplay( "%4d    GCD invalid fetch sequence: %d\n", 
			ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: crsr_close_sm
**
** Description:
**	State mechine for CURSOR CLOSE requests.
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
**	19-Oct-00 (gordy)
**	    Signal cursor close to transaction sub-system to
**	    allow handling of autocommit simulation.
**	20-Aug-03 (gordy)
**	    Return indication that statement/cursor has been closed.
**	 6-Oct-03 (gordy)
**	    Renamed and expanded into CLOSE state machine.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Also check for logical processing errors.
*/

static void
crsr_close_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    GCD_SCB	*scb = pcb->scb;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Close: %s\n", 
		    ccb->id, gcu_lookup( closeMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case CLOSE_CURSOR :
	pcb->scb = NULL;
	gcd_push_callback( pcb, crsr_close_sm );
	gcd_del_stmt( pcb, scb );
	return;

    case CLOSE_XACT : 
	pcb->result.flags |= PCB_RSLT_CLOSED;

	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )  
	    break;	/* Handled below */

	gcd_push_callback( pcb, crsr_close_sm );

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	    gcd_xact_abort( pcb );
	else
	    gcd_xact_check( pcb, GCD_XQOP_CURSOR_CLOSE );
	return;
    
    case CLOSE_DONE :
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
	    TRdisplay( "%4d    GCD invalid close sequence: %d\n", 
			ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: crsr_rows
**
** Description:
**	Formats DATA message.  Data message is started when the 
**	first column is a part of the column sub-set processed.  
**	Data message is completed when the last column has 
**	completed.  Returns indication of processing state.
**
** Input:
**	arg	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if complete, FALSE if partial row fetched.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Process multiple rows for cursor pre-fetch.
**	 6-Oct-03 (gordy)
**	    Renamed and reqworked to only handle row process.  
**	    Basic control functionality moved to crsr_fetch_sm().
*/

static bool
crsr_rows( GCD_CCB *ccb, GCD_SCB *scb, GCD_PCB *pcb )
{
    u_i2	row;

    if ( pcb->data.data.row_cnt )
    {
	/*
	** Start the DATA message at first column.
	** Be sure to handle the case where first
	** column is a BLOB which has previously
	** returned segments.
	*/
	if ( ! scb->column.cur_col  &&  ! scb->column.more_segments )
	    gcd_msg_begin( ccb, &pcb->rcb, MSG_DATA, 0 );

	/*
	** Process the available columns.  If processing
	** a BLOB, don't skip past the last column until
	** all segments have been sent.
	*/
	if ( GCD_global.gcd_trace_level >= 4 )
	    TRdisplay( "%4d    GCD Fetch: col %d of %d, %d rows %d cols %s\n", 
		       ccb->id, scb->column.cur_col, scb->column.max_cols,
		       pcb->data.data.row_cnt, scb->column.col_cnt, 
		       pcb->data.data.more_segments ? "(more segments)" : "" );

	for( row = 0; row < pcb->data.data.row_cnt; row++ )
	{
	    crsr_cols(ccb, scb, &pcb->rcb, row, pcb->data.data.more_segments);
	    scb->column.cur_col += scb->column.col_cnt -
				   (scb->column.more_segments ? 1 : 0);

	    /*
	    ** Continue fetching if this was a partial fetch.
	    */
	    if ( scb->column.cur_col < scb->column.max_cols )
		return( FALSE );

	    /*
	    ** Reset for start of next row.
	    */
	    scb->column.cur_col = 0;
	}

	gcd_msg_end( &pcb->rcb, FALSE );
    }

    /*
    ** Fetch processing completed.
    */
    scb->column.cur_col = scb->column.col_cnt = 0;
    scb->column.more_segments = FALSE;
    return( TRUE );
}


/*
** Name: crsr_cols
**
** Description:
**	Format and send a DATA message.  The message
**	is appended to an existing request control block
**	if provided.  Otherwise, a new request control block
**	is created and returned with the message.
**
** Input:
**	ccb		Connection control block.
**	scb		Statement control block.
**	rcb_ptr		Request control block.
**	row		Which data row is being processed.
**	more_segments	Are additional BLOB segments available?
**
** Ouput:
**	rcb_ptr	Request control block.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Added row parameter for handling cursor pre-fetch.
**	01-Feb-00 (rajus01) 
**	    Client expects date string to include timestamp also. 
**	30-May-00 (gordy & rajus01)
**	    Bug #s 101677,101678: Removed length conditional
**	    statement for date strings.
**	11-Oct-00 (gordy)
**	    Added CCB to jdbc_sess_error() parameters.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 2-Jul-01 (gordy)
**	    Make sure UCS2 characters are not split across buffers.
**	 4-Aug-03 (gordy)
**	    Conversions errors no longer split data messages to send
**	    error messages.  Tuple data now only split at tuple and
**	    BLOB segment boundaries.
**	 6-Oct-03 (gordy)
**	    Standardized names.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time types.
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators.
**	 9-Sep-08 (gordy)
**	    Expand temp buffer to support larger decimal precision.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN.
*/

static void
crsr_cols
( 
    GCD_CCB	*ccb, 
    GCD_SCB	*scb, 
    GCD_RCB	**rcb_ptr, 
    u_i2	row,
    bool	more_segments 
)
{
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)scb->column.desc;
    IIAPI_DATAVALUE	*data = &((IIAPI_DATAVALUE *)scb->column.data)[ 
						row * scb->column.max_cols ];
    u_i2		end = scb->column.cur_col + scb->column.col_cnt;
    u_i2		col, len;

    for( col = scb->column.cur_col; col < end; col++ )
    {
	if ( desc[ col ].ds_nullable  &&  data[ col ].dv_null )
	{
	    gcd_put_i1( rcb_ptr, 0 );	/* No data - NULL value */
	    continue;
	}

	/*
	** Write the data indicator byte, for BLOBs this is
	** only done on the first segment (more_segments is
	** saved below once the segment has been processed,
	** so the saved value is FALSE on the first segment).
	*/
	switch(desc[ col ].ds_dataType)
	{
	case IIAPI_LVCH_TYPE:
        case IIAPI_LNVCH_TYPE:
	case IIAPI_LBYTE_TYPE:
	case IIAPI_GEOM_TYPE:
	case IIAPI_POINT_TYPE:
	case IIAPI_MPOINT_TYPE:
	case IIAPI_LINE_TYPE:
	case IIAPI_MLINE_TYPE:
	case IIAPI_POLY_TYPE:
	case IIAPI_MPOLY_TYPE:
	case IIAPI_GEOMC_TYPE:
            {
		if ( !scb->column.more_segments )
		    gcd_put_i1( rcb_ptr, 1 );
		break;
            }
	default:
	    gcd_put_i1( rcb_ptr, 1 );
            break;
	}

	switch( desc[ col ].ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( desc[ col ].ds_length )
	    {
	    case 1 : gcd_put_i1p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 2 : gcd_put_i2p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 4 : gcd_put_i4p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 8 : gcd_put_i8p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( desc[ col ].ds_length )
	    {
	    case 4 : gcd_put_f4p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 8 : gcd_put_f8p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    }
	    break;

	case IIAPI_MNY_TYPE :
	    {
		IIAPI_DESCRIPTOR	idesc, ddesc;
		IIAPI_DATAVALUE		idata, ddata;
		STATUS			status;
		char			dbuff[ 130 ];	/* varchar(128) */
		char			dec[ 8 ];

		/*
		** It would be nice to convert directly to
		** varchar, but money formatting is nasty.  
		** So we first convert to decimal, then to 
		** varchar.
		*/
		idesc.ds_dataType = IIAPI_DEC_TYPE;
		idesc.ds_nullable = FALSE;
		idesc.ds_length = sizeof( dec );
		idesc.ds_precision = 15;
		idesc.ds_scale = 2;
		idata.dv_null = FALSE;
		idata.dv_length = sizeof( dec );
		idata.dv_value = (PTR)&dec;

		ddesc.ds_dataType = IIAPI_VCH_TYPE;
		ddesc.ds_nullable = FALSE;
		ddesc.ds_length = sizeof( dbuff );
		ddesc.ds_precision = 0;
		ddesc.ds_scale = 0;
		ddata.dv_null = FALSE;
		ddata.dv_length = sizeof( dbuff );
		ddata.dv_value = dbuff;

		if ( (status = gcd_api_format( ccb, &desc[ col ], &data[ col ],
						&idesc, &idata )) != OK  ||
		     (status = gcd_api_format( ccb, &idesc, &idata, 
						&ddesc, &ddata )) != OK )
		{
		    /*
		    ** Conversion error.  Send a zero-length 
		    ** string as error indication and log error.
		    */
		    gcd_put_i2( rcb_ptr, 0 );
		    gcu_erlog( 0, GCD_global.language, status, NULL, 0, NULL );
		}
		else
		{
		    MEcopy( (PTR)dbuff, 2, (PTR)&len );
		    gcd_put_i2( rcb_ptr, len );
		    gcd_put_bytes( rcb_ptr, len, (u_i1 *)&dbuff[2] );
		}
	    }
	    break;

	case IIAPI_BOOL_TYPE :
	    gcd_put_i1p( rcb_ptr, (u_i1 *)data[ col ].dv_value );
	    break;

	case IIAPI_DEC_TYPE :	/* These types are all sent in text format */
	case IIAPI_DTE_TYPE :
	case IIAPI_DATE_TYPE :
	case IIAPI_TIME_TYPE :
	case IIAPI_TMWO_TYPE :
	case IIAPI_TMTZ_TYPE :
	case IIAPI_TS_TYPE :
	case IIAPI_TSWO_TYPE :
	case IIAPI_TSTZ_TYPE :
	case IIAPI_INTYM_TYPE :
	case IIAPI_INTDS_TYPE :
	    {
		IIAPI_DESCRIPTOR	ddesc;
		IIAPI_DATAVALUE		ddata;
		STATUS			status;
		char			dbuff[ 130 ];	/* varchar(128) */

		ddesc.ds_dataType = IIAPI_VCH_TYPE;
		ddesc.ds_nullable = FALSE;
		ddesc.ds_length = sizeof( dbuff );
		ddesc.ds_precision = 0;
		ddesc.ds_scale = 0;
		ddata.dv_null = FALSE;
		ddata.dv_length = sizeof( dbuff );
		ddata.dv_value = dbuff;

		status = gcd_api_format( ccb, &desc[ col ], &data[ col ], 
					  &ddesc, &ddata );

		if ( status != OK )  
		{
		    /*
		    ** Conversion error.  Send a zero-length 
		    ** string as error indication and log error.
		    */
		    gcd_put_i2( rcb_ptr, 0 );
		    gcu_erlog( 0, GCD_global.language, status, NULL, 0, NULL );
		}
		else
		{
		    MEcopy( (PTR)dbuff, 2, (PTR)&len );
		    gcd_put_i2( rcb_ptr, len );
		    gcd_put_bytes( rcb_ptr, len, (u_i1 *)&dbuff[2] );
		}
	    }
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_CHR_TYPE :
	case IIAPI_BYTE_TYPE :
	case IIAPI_NBR_TYPE :
	    gcd_put_bytes( rcb_ptr, desc[ col ].ds_length, 
			    (u_i1 *)data[ col ].dv_value );
	    break;

	case IIAPI_NCHA_TYPE :
	    gcd_put_ucs2( rcb_ptr, desc[ col ].ds_length / sizeof( UCS2 ),
			   (u_i1 *)data[ col ].dv_value );
	    break;

	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	case IIAPI_VCH_TYPE :	
	case IIAPI_VBYTE_TYPE :
	    MEcopy( data[ col ].dv_value, 2, (PTR)&len );
	    gcd_put_i2( rcb_ptr, len );
	    gcd_put_bytes( rcb_ptr, len, (u_i1 *)data[ col ].dv_value + 2 );
	    break;

	case IIAPI_NVCH_TYPE :
	    MEcopy( data[ col ].dv_value, 2, (PTR)&len );
	    gcd_put_i2( rcb_ptr, len );
	    gcd_put_ucs2( rcb_ptr, len, (u_i1 *)data[ col ].dv_value + 2 );
	    break;

	case IIAPI_LCLOC_TYPE :
	case IIAPI_LNLOC_TYPE :
	case IIAPI_LBLOC_TYPE :
	    gcd_put_i4p( rcb_ptr, (u_i1 *)data[ col ].dv_value );
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
	    {
		u_i1	*ptr;
		u_i2	seg_len, chrs, char_size = sizeof( char );
		bool	ucs2 = FALSE;

		if ( desc[ col ].ds_dataType == IIAPI_LNVCH_TYPE )
		{
		    ucs2 = TRUE;
		    char_size = sizeof( UCS2 );
		}

		if ( data[ col ].dv_length < 2 )
		    seg_len = 0;
		else
		{
		    MEcopy( data[ col ].dv_value, 2, (PTR)&seg_len );
		    ptr = (u_i1 *)data[ col ].dv_value + 2;
		    seg_len *= char_size;  /* convert array len to byte len */
		}

		/*
		** Output data as long as there is sufficient
		** data to fill the current message.  Any data
		** remaining is saved until additional data is
		** received or the end of the BLOB is reached.
		**
		** We actually make sure that room for the
		** segment (length indicator and data) and an
		** end-of-segments indicator is left in the
		** message buffer.  This way the end-of-BLOB
		** processing below does not need to worry
		** about splitting the message.
		**
		** The test against the save buffer size is
		** redundent since the buffer should be at 
		** least as big as a message buffer, but we 
		** make the test just to be safe.
		*/
		while( (seg_len + scb->seg_len + 4) > RCB_AVAIL(*rcb_ptr)  ||
		       (seg_len + scb->seg_len) > scb->seg_max )
		{
		    /*
		    ** Can a valid data segment be placed in the buffer?
		    */
		    if ( RCB_AVAIL(*rcb_ptr) >= (2 + char_size)  &&  
			 (seg_len + scb->seg_len) >= char_size )
		    {
			len = min( seg_len + scb->seg_len, 
				   RCB_AVAIL( *rcb_ptr ) - 2 );
			chrs = len / char_size;
			len = chrs * char_size;

			gcd_put_i2( rcb_ptr, chrs );

			if ( GCD_global.gcd_trace_level >= 5 )
			    TRdisplay( "%4d    GCD send segment: %d (%d,%d)\n",
					ccb->id, len, scb->seg_len, seg_len );

			/*
			** First, send saved data.
			*/
			if ( scb->seg_len >= char_size )
			{
			    len = min( scb->seg_len, RCB_AVAIL( *rcb_ptr ) );
			    chrs = len / char_size;
			    len = chrs * char_size;

			    if ( ! ucs2 )
				gcd_put_bytes( rcb_ptr, len, scb->seg_buff );
			    else
				gcd_put_ucs2( rcb_ptr, chrs, scb->seg_buff );

			    scb->seg_len -= len;
			    if ( scb->seg_len ) 
				MEcopy( (PTR)(scb->seg_buff + len), 
					scb->seg_len, (PTR)scb->seg_buff );
			}

			/*
			** Now send data from current segment.
			*/
			if ( seg_len >= char_size  &&  
			     RCB_AVAIL( *rcb_ptr ) >= char_size )
			{
			    len = min( seg_len, RCB_AVAIL( *rcb_ptr ) );
			    chrs = len / char_size;
			    len = chrs * char_size;

			    if ( ! ucs2 )
				gcd_put_bytes( rcb_ptr, len, ptr );
			    else
				gcd_put_ucs2( rcb_ptr, chrs, ptr );

			    ptr += len;
			    seg_len -= len;
			}
		    }

		    gcd_msg_end( rcb_ptr, FALSE );
		    gcd_msg_begin( ccb, rcb_ptr, MSG_DATA, 0 );
		}

		/*
		** Save any data left in the current segment.
		** The preceding loop makes sure there is room
		** in the buffer for the remainder of the
		** current segment.
		*/
		if ( seg_len )
		{
		    if ( ! scb->seg_buff )
		    {
			scb->seg_buff = (u_i1 *)MEreqmem( 0, scb->seg_max,
							  FALSE, NULL );
			if ( ! scb->seg_buff )
			{
			    if ( GCD_global.gcd_trace_level >= 1 )
				TRdisplay( "%4d    GCD alloc fail seg: %d\n",
					   ccb->id, scb->seg_max );
			    gcu_erlog( 0, GCD_global.language, 
				       E_GC4808_NO_MEMORY, NULL, 0, NULL );
			    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
			    return;
			}
		    }

		    if ( GCD_global.gcd_trace_level >= 5 )
			TRdisplay( "%4d    GCD save segment: %d (total %d) \n",
				    ccb->id, seg_len, scb->seg_len + seg_len );
		    MEcopy( (PTR)ptr, seg_len, 
			    (PTR)(scb->seg_buff + scb->seg_len) );
		    scb->seg_len += seg_len;
		}

		/*
		** When the BLOB is complete, write the last
		** segment and end-of-segments indicator.
		*/
		if ( ! (scb->column.more_segments = more_segments) )
		{
		    /*
		    ** The processing loop above makes sure there
		    ** is room for the last segment.
		    */
		    if ( scb->seg_len )
		    {
			if ( GCD_global.gcd_trace_level >= 5 )
			    TRdisplay( "%4d    GCD send segment: %d \n",
					ccb->id, scb->seg_len );

			chrs = scb->seg_len / char_size;
			len = chrs * char_size;
			scb->seg_len = 0;

			gcd_put_i2( rcb_ptr, chrs );

			if ( ! ucs2 )
			    gcd_put_bytes( rcb_ptr, len, scb->seg_buff );
			else
			    gcd_put_ucs2( rcb_ptr, chrs, scb->seg_buff );
		    }

		    if ( GCD_global.gcd_trace_level >= 5 )
			TRdisplay( "%4d    GCD send end-of-segments\n",
				    ccb->id );
		    gcd_put_i2( rcb_ptr, 0 );
		}
	    }
	    break;

	default :
	    /* Should not happen since checked in send_desc() */
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid datatype: %d\n",
				    ccb->id, desc[ col ].ds_dataType );
	    gcd_sess_abort( ccb, E_GC4812_UNSUPP_SQL_TYPE );
	    return;
	}
    }

    return;
}


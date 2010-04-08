/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <qu.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdstmt.c
**
** Description:
**	GCD Statement control block management function.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	26-Oct-99 (gordy)
**	    Added jdbc_find_cursor() to support 
**	    cursor positioned DELETE/UPDATE.
**	17-Jan-00 (gordy)
**	    Added connection pooling and select-loop processing.
**	19-May-00 (gordy)
**	    Statements which are still active (ccb->api.stmt)
**	    need to be cancelled before being closed. 
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Reworked
**	    jdbc_del_stmt() to use stack for nested request.
**	    Added jdbc_purge_stmt().  Allocate RCB in
**	    jdbc_sess_error() if needed (CCB added to params).
**	19-Oct-00 (gordy)
**	    Added jdbc_active_stmt().
**	15-Jan-03 (gordy)
**	    Extracted from gcdsess.c
**	 4-Aug-03 (gordy)
**	    Statement creation now sets the statement type.  Query
**	    data buffer allocation delayed until needed.
**	20-Aug-03 (gordy)
**	    Propogate query execution flags.
**	11-Oct-05 (gordy)
**	    Ignore non-critical errors while purging statements.
**	21-Jul-09 (gordy)
**	    Remove cursor name length restrictions.
*/	

static	void	cancel_cmpl( PTR arg );
static	void	purge_cmpl( PTR arg );



/*
** Name: gcd_new_stmt
**
** Description:
**	Create a statement control block for the current API
**	statement.  Statements generally represent an open
**	cursor, but temporary statements for select loops may
**	be requested.
**
**	For non-temporary statements:
**	  * Save the current API statement (on the ccb) for later access.  
**	  * Generates a unique ID for the statement and places the ID in 
**	    the pcb as a result value.
**
** Input:
**	ccb		Connection control block.
**	pcb		Parameter control block.
**	 data
**	  desc
**	   count	Number of columns.
**	   desc		Column descriptors.
**
** Output:
**	pcb
**	 result	
**	  stmt_id	Statement ID.
**
** Returns:
**	bool	TRUE if successful, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	17-Jan-00 (gordy)
**	    Allow temporary (non-cursor) statements to be created.
**	 4-Aug-03 (gordy)
**	    Removed temp parameter.  Set statement type based on
**	    query type.  Query data buffers no longer allocated.
**	20-Aug-03 (gordy)
**	    Propogate query execution flags.
**	 6-Oct-03 (gordy)
**	    Extend previous change to include fetch flags.
**	21-Jul-09 (gordy)
**	    Cursor in CCB is dynamically allocated.  Move to SCB
**	    for cursor queries.
*/

GCD_SCB *
gcd_new_stmt( GCD_CCB *ccb, GCD_PCB *pcb )
{
    GCD_SCB	*scb;

    if ( ! (scb = (GCD_SCB *)MEreqmem( 0, sizeof( GCD_SCB ), TRUE, NULL )) )
    {
	gcu_erlog(0, GCD_global.language, E_GC4808_NO_MEMORY, NULL, 0, NULL);
	return( NULL );
    }

    if ( ! gcd_alloc_qdesc( &scb->column, pcb->data.desc.count, 
			     (PTR)pcb->data.desc.desc ) )
    {
	MEfree( (PTR)scb );
	return( NULL );
    }

    scb->stmt_id.id_high = ccb->xact.tran_id;
    scb->stmt_id.id_low = ++ccb->stmt.stmt_id;
    scb->seg_max = ccb->max_buff_len;
    scb->handle = ccb->api.stmt;
    ccb->api.stmt = NULL;

    /*
    ** Determine statement type.
    */
    if ( ccb->qry.flags & QRY_PROCEDURE )
	if ( ccb->qry.flags & QRY_BYREF )
	    scb->flags = GCD_STMT_BYREF;
	else 
	    scb->flags = GCD_STMT_PROCEDURE;
    else  if ( ccb->qry.flags & QRY_CURSOR )
    {
	scb->flags = GCD_STMT_CURSOR;
	scb->crsr_name = ccb->qry.crsr_name;
	ccb->qry.crsr_name = NULL;
    }
    else
	scb->flags = GCD_STMT_SELECT;

    /*
    ** Propogate statement execution flags.
    */
    if ( ccb->qry.flags & QRY_FETCH_ALL )  scb->flags |= GCD_STMT_FETCH_ALL;
    if ( ccb->qry.flags & QRY_AUTO_CLOSE )  scb->flags |= GCD_STMT_AUTO_CLOSE;

    QUinsert( &scb->q, &ccb->stmt.stmt_q );

    if ( GCD_global.gcd_trace_level >= 5 )
	TRdisplay( "%4d    GCD new STMT (%d,%d)\n", 
		   ccb->id, scb->stmt_id.id_high, scb->stmt_id.id_low );

    return( scb );
}


/*
** Name: gcd_del_stmt
**
** Descriptions:
**	Close a statement associated with a connection.
**	and free its resources.  A callback function is
**	called from the PCB stack when complete.
**	
** Input:
**	pcb	Parameter control block.
**	scb	Statement control block.
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
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	19-May-00 (gordy)
**	    Statements which are still active (ccb->api.stmt)
**	    need to be cancelled before being closed.  This
**	    is especially true for execution of select loops
**	    which do not become inactive until the last tuple
**	    is received.
**	11-Oct-00 (gordy)
**	    Nested request now handled through stack rather than
**	    extra PCB (in SCB).  Since handle in callback is
**	    available in PCB, SCB is now freed before any API
**	    operations.  The call to jdbc_api_close() does the
**	    jdbc_pop_callback() of the callback required by
**	    this routine.
**	21-Jul-09 (gordy)
**	    Cursor name dynamically allocated.
*/

void
gcd_del_stmt( GCD_PCB *pcb, GCD_SCB *scb )
{
    GCD_CCB	*ccb = pcb->ccb;
    PTR		handle = scb->handle;

    if ( GCD_global.gcd_trace_level >= 5 )
	TRdisplay( "%4d    GCD del STMT (%d,%d)\n", ccb->id, 
		   scb->stmt_id.id_high, scb->stmt_id.id_low );

    QUremove( &scb->q );
    gcd_free_qdata( &scb->column );
    if ( scb->crsr_name )  MEfree( (PTR)scb->crsr_name );
    if ( scb->seg_buff ) MEfree( (PTR)scb->seg_buff );
    MEfree( (PTR)scb );

    if ( handle != ccb->api.stmt )
	gcd_api_close( pcb, handle );
    else
    {
	/*
	** The statement is active, so we need to
	** cancel the statement before closing.
	*/
	gcd_push_callback( pcb, cancel_cmpl );
	gcd_api_cancel( pcb );
    }

    return;
}

static void
cancel_cmpl( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    PTR		handle = ccb->api.stmt;

    /*
    ** Close the active statment which has been cancelled.
    */
    ccb->api.stmt = NULL;
    gcd_api_close( pcb, handle );

    return;
}


/*
** Name: gcd_purge_stmt
**
** Description:
**	Delete all statements associated with a connection.
**	Statement related errors are ignored, connection
**	related errors are signaled with pcb->api_error.
**	A callback function is popped from the PCB stack 
**	when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	11-Oct-00 (gordy)
**	    Created.
**	11-Oct-05 (gordy)
**	    Continue purging statements even if errors occur.
*/

void
gcd_purge_stmt( GCD_PCB *pcb )
{
    GCD_CCB	*ccb = pcb->ccb;

    purge_cmpl( (PTR)pcb );
    return;
}

static void
purge_cmpl( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;

    /*
    ** We don't really care about errors while purging statements,
    ** unless something drastic, like connection abort, occurs.
    */
    if ( ccb->cib->flags & GCD_CONN_ABORT )
    {
	if ( ! pcb->api_error )  pcb->api_error = FAIL;
	gcd_pop_callback( pcb );
    }
    else  if ( ! QUEUE_EMPTY( &ccb->stmt.stmt_q ) )
    {
	/*
	** Delete statement at head of queue.
	*/
	gcd_push_callback( pcb, purge_cmpl );
	gcd_del_stmt( pcb, (GCD_SCB *)ccb->stmt.stmt_q.q_next );
    }
    else
    {
	/*
	** Done, don't return errors.
	*/
	pcb->api_error = OK;
	gcd_pop_callback( pcb );
    }

    return;
}


/*
** Name: gcd_active_stmt
**
** Description:
**	Returns and indication that there are active statements
**	associated with the connection.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if active statements, FALSE otherwise.
**
** History:
**	19-Oct-00 (gordy)
**	    Created.
*/

bool
gcd_active_stmt( GCD_CCB *ccb )
{
    return( ! QUEUE_EMPTY( &ccb->stmt.stmt_q ) );
}


/*
** Name: gcd_find_stmt
**
** Description:
**	Search the connection statement queue for a statement
**	control block with a matching statement ID.
**
** Input:
**	ccb		Connection control block.
**	stmt_id		Statement ID.
**
** Output:
**	None.
**
** Returns:
**	GCD_SCB *	Statement control block, or NULL if not found.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

GCD_SCB *
gcd_find_stmt( GCD_CCB *ccb, STMT_ID *stmt_id )
{
    GCD_SCB	*scb;
    QUEUE	*q;

    for(
	 q = ccb->stmt.stmt_q.q_next, scb = NULL;
	 q != &ccb->stmt.stmt_q;
	 q = q->q_next, scb = NULL
       )
    {
	scb = (GCD_SCB *)q;

	if ( scb->stmt_id.id_high == stmt_id->id_high  &&
	     scb->stmt_id.id_low  == stmt_id->id_low )
	    break;
    }

    if ( ! scb  &&  GCD_global.gcd_trace_level >= 5 )
	TRdisplay( "%4d    GCD STMT (%d,%d) does not exist!\n", 
		   ccb->id, stmt_id->id_high, stmt_id->id_low );

    return( scb );
}


/*
** Name: gcd_find_cursor
**
** Description:
**	Search the connection statement queue for a statement
**	control block with a matching cursor name.
**
** Input:
**	ccb		Connection control block.
**	cursor		Cursor name.
**
** Output:
**	None.
**
** Returns:
**	GCD_SCB *	Statement control block, or NULL if not found.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	21-Jul-09 (gordy)
**	    Cursor name dynamically allocated.
*/

GCD_SCB *
gcd_find_cursor( GCD_CCB *ccb, char *cursor )
{
    GCD_SCB	*scb;
    QUEUE	*q;

    for(
	 q = ccb->stmt.stmt_q.q_next, scb = NULL;
	 q != &ccb->stmt.stmt_q;
	 q = q->q_next, scb = NULL
       )
    {
	scb = (GCD_SCB *)q;
	if ( ! scb->crsr_name )  continue;
	if ( ! STbcompare( cursor, 0, scb->crsr_name, 0, TRUE ) )  break;
    }

    if ( ! scb  &&  GCD_global.gcd_trace_level >= 5 )
	TRdisplay( "%4d    GCD cursor %s does not exist!\n", 
		   ccb->id, cursor );

    return( scb );
}


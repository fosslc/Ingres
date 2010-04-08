/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdpool.c
**
** Description:
**	GCD connection pool manager.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Turn off autocommit or abort connection
**	    if there is an active transaction.
**	 3-Mar-00 (gordy)
**	    Added jdbc_pool_flush().
**	 10-dec-2002 (wansh01)
**          in jdbc_pool_find(), check isLocal flag for matching pool.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	26-Dec-02 (gordy)
**	    Check for compatible transaction state.
**	24-Aug-04 (wansh01)
**	    Added gcd_pool_info() to support SHOW POOLED SESSION command 	
**	    Added gcd_pool_remove() to support REMOVE sid command 	
**	20-Apr-05 (wansh01)
**	    Changed gcd_pool_info() parm from GCD_SESS_INFO to GCD_POOL_INFO.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/

/*
** Connection pool: queue to hold CIBs.
*/
static	QUEUE	pool;

/*
** Forward references.
*/

static	void	gcd_pool_del( GCD_CIB *, bool );

II_FAR II_CALLBACK II_VOID	gcd_pool_auto( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_pool_disc( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_pool_abrt( II_PTR, II_PTR );

FUNC_EXTERN void  gcd_get_gca_id (PTR, i4 * );



/*
** Name: gcd_pool_init
**
** Description:
**	Initialize the connection pool manager.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
*/

STATUS
gcd_pool_init( void )
{
    QUinit( &pool );

    if ( GCD_global.pool_idle_limit )
    {
	TMnow( &GCD_global.pool_check );
	GCD_global.pool_check.TM_secs += GCD_global.pool_idle_limit;
    }

    return( OK );
}


/*
** Name: gcd_pool_term
**
** Description:
**	Terminate the connection pool manager.  Saved connections
**	will be disconnected and their Connection Information
**	Blocks freed.  
**
**	NOTE: this function makes synchronous API calls and will
**	not return until all connections are closed.
**
** Input:
**	None.
**
** Ouput:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
*/

void
gcd_pool_term()
{
    QUEUE *q;

    for( q = pool.q_next; q != &pool; q = pool.q_next )
    {
	QUremove( &((GCD_CIB *)q)->q );
	GCD_global.pool_cnt--;
	gcd_pool_del( (GCD_CIB *)q, FALSE );
    }

    return;
}


/*
** Name: gcd_pool_save
**
** Description:
**	Save a connection in the connection pool.  Returns FALSE
**	if the connection pool is full and the connection was not
**	saved.
**
**	If the Connection Information Block is saved, it will 
**	eventually either be returned by gcd_pool_find() or 
**	freed by gcd_del_cib() and the connection disconnected.
**
** Input:
**	cib	Connection Information Block
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if connection saved, FALSE otherwise.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
**	 3-Mar-00 (gordy)
**	    Use new gcd_pool_flush() when pool filled.
*/

bool
gcd_pool_save( GCD_CIB *cib )
{
    /*
    ** We only save a CIB if it could have
    ** come from the pool originally.  Test
    ** should be the same as gcd_pool_find().
    */
    if ( ! GCD_global.pool_max  ||  cib->pool_off  ||
	 (GCD_global.client_pooling  &&  ! cib->pool_on) )
	return( FALSE );	/* Pooling is off */

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD pool: saving CIB (%d)\n", -1, cib->id );

    /*
    ** Set the expiration time.  If no expiration, set to 0.
    ** Otherwise, set to now plus expiration interval.
    */
    if ( ! GCD_global.pool_idle_limit )
	cib->expire.TM_secs = 0;
    else
    {
	TMnow( &cib->expire );
	cib->expire.TM_secs += GCD_global.pool_idle_limit;
    }

    QUinsert( &cib->q, &pool );
    GCD_global.pool_cnt++;

    /*
    ** If the pool is full, release the oldest CIB.
    */
    if ( GCD_global.pool_max > 0  &&  
	 GCD_global.pool_cnt > GCD_global.pool_max )
	gcd_pool_flush( GCD_global.pool_cnt - GCD_global.pool_max );

    return( TRUE );
}


/*
** Name: gcd_pool_find
**
** Description:
**	Searches the connection pool for an information block
**	matching the input parameter.  If a match is found,
**	the saved connection is removed from the pool and
**	returned, otherwise NULL is returned.
**
** Input:
**	target		Connection Information Block.
**
** Output:
**	None.
**
** Returns:
**	GCD_CIB *	Connection Information Block, NULL if no match.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
**	10-dec-2002 (wansh01)
**          Added checking for isLocal flag for matching pool.
**	26-Dec-02 (gordy)
**	    Check for compatible transaction state.
**	26-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level. Also, matches the API version 
**	    of the connections in the pool.
*/

GCD_CIB *
gcd_pool_find( GCD_CIB *target )
{
    QUEUE	*q;
    GCD_CIB	*cib;

    /*
    ** Pooling is disabled if configured at the server,
    ** inhibited by the client, or if the client has not
    ** requested pooling when the server is configured
    ** for client control of pooling.
    */
    if ( ! GCD_global.pool_max  ||  target->pool_off  ||
	 (GCD_global.client_pooling  &&  ! target->pool_on) )
	return( NULL );		/* Pooling is off */

    /*
    ** Search for the oldest matching connection.
    */
    for( 
	 q = pool.q_prev, cib = NULL; 
	 q != &pool; 
	 q = ((GCD_CIB *)q)->q.q_prev, cib = NULL
       )
    {
	bool match = TRUE;
	cib = (GCD_CIB *)q;

	/*
	** Compare connection info.
	*/
	if ( cib->api_ver != target->api_ver )  
	    match = FALSE;

	if ( ! cib->database  ||  ! target->database  ||
	     STcompare( cib->database, target->database ) )  
	    if ( cib->database  ||  target->database )
		match = FALSE;

	if ( ! cib->username  ||  ! target->username  ||
	     STcompare( cib->username, target->username ) )  
	    if ( cib->username  ||  target->username )
		match = FALSE;

	if ( ! cib->password  ||  ! target->password  ||
	     STcompare( cib->password, target->password ) )  
	    if ( cib->password  ||  target->password )
		match = FALSE;

	if ( cib->isLocal != target->isLocal )  
	    match = FALSE;

	/*
	** Make sure the transaction state is compatible:
	** if there is an active transaction, it must be
	** of the correct type.  
	*/
	if ( cib->tran  &&  cib->autocommit != target->autocommit )
	    match = FALSE;

	/*
	** Compare connection parameters.
	*/
	if ( cib->parm_cnt != target->parm_cnt )
	    match = FALSE;
	else
	{
	    u_i2 i, j;

	    /*
	    ** For each parameter, see if it matches
	    ** one of the target parameters.  Since
	    ** the parameter counts are the same and
	    ** there should be no duplicate IDs, there
	    ** is no need to check the other direction.
	    */
	    for( i = 0; match  &&  i < cib->parm_cnt; i++ )
	    {
		for( j = 0; j < target->parm_cnt; j++ )
		    if ( cib->parms[i].id == target->parms[j].id  &&
			 ! STcompare( cib->parms[i].value, 
				      target->parms[j].value ) )  
			break;
		
		if ( j >= target->parm_cnt )  match = FALSE;
	    }
	}

	if ( match )  
	{
	    QUremove( &cib->q );
	    GCD_global.pool_cnt--;
	    break;
	}
    }

    if ( cib  &&  GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD pool: activating CIB (%d)\n", -1, cib->id );

    return( cib );
}


/*
** Name: gcd_pool_flush
**
** Description:
**	Deletes the oldest entries in the connection pool.
**
** Input:
**	count	Number of entries to delete.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if count entries deleted.
**
** History:
**	 3-Mar-00 (gordy)
**	    Created.
*/

bool
gcd_pool_flush( u_i4 count )
{
    for( ; count > 0  &&  pool.q_prev != &pool; count-- )
    {
	GCD_CIB *cib = (GCD_CIB *)pool.q_prev;

	QUremove( &cib->q );
	GCD_global.pool_cnt--;

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD pool: flushing CIB (%d)\n", -1, cib->id );

	gcd_pool_del( cib, TRUE );
    }

    return( count <= 0 );
}


/*
** Name: gcd_pool_check
**
** Description:
**	Scan the connection pool for connections which have been
**	dormant beyond the configured limit.  Old connections are
**	disconnected and their Connection Information Block is
**	freed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
*/

void
gcd_pool_check( void )
{
    QUEUE	*q, *next;
    SYSTIME	now;

    /*
    ** Is it time to check the pool?
    */
    TMnow( &now );
    if ( ! GCD_global.pool_idle_limit  ||
	 TMcmp( &now, &GCD_global.pool_check ) < 0 )  return;

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD checking connection pool\n", -1 );

    /*
    ** Set next check time to one full period.  If
    ** there are any CIBs which will expire in less
    ** than a full period, we adjust the next check
    ** time accordingly below.
    */
    STRUCT_ASSIGN_MACRO( now, GCD_global.pool_check );
    GCD_global.pool_check.TM_secs += GCD_global.pool_idle_limit;

    for( q = pool.q_prev; q != &pool; q = next )
    {
	GCD_CIB *cib = (GCD_CIB *)q;
	next = cib->q.q_prev;

	/*
	** If expiration were off and then turned on,
	** its possible that a CIB with no expiration
	** time may exist.  We check for this case and
	** assign an expiration time.  Otherwise, the
	** CIB is removed from the pool if it has
	** expired.
	*/
	if ( ! cib->expire.TM_secs )
	{
	    /*
	    ** Set expiration time.
	    */
	    TMnow( &cib->expire );
	    cib->expire.TM_secs += GCD_global.pool_idle_limit;
	}
	else  if ( TMcmp( &now, &cib->expire ) >= 0 )
	{
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD pool: expired CIB (%d)\n", 
			   -1, cib->id );

	    QUremove( &cib->q );
	    GCD_global.pool_cnt--;
	    gcd_pool_del( cib, TRUE );
	}
	else  if ( TMcmp( &cib->expire, &GCD_global.pool_check ) < 0 )
	{
	    /*
	    ** Reset the check time to match this CIB
	    ** since it will expire prior to the current
	    ** check time.
	    */
	    STRUCT_ASSIGN_MACRO( cib->expire, GCD_global.pool_check );
	}
    }

    return;
}


/*
** Name: gcd_pool_del
**
** Description:
**	Disconnect a pooled connection and free the Connection
**	Information Block.
**
** Input:
**	cib	Connection Information Block.
**	async	TRUE for async, FALSE for sync.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Turn off autocommit or abort connection
**	    if there is an active transaction.
*/

static void
gcd_pool_del( GCD_CIB *cib, bool async )
{
    IIAPI_WAITPARM 	wp;
    IIAPI_GENPARM	*gp = NULL;

    if ( cib->tran )
    {
	if ( cib->autocommit  &&  async )
	{
	    IIAPI_AUTOPARM	*ac = &cib->api.ac;

	    MEfill( sizeof( *ac ), 0, (PTR)ac );
	    ac->ac_tranHandle = cib->tran;
	    ac->ac_connHandle = cib->tran = NULL;
	    ac->ac_genParm.gp_callback = gcd_pool_auto;
	    ac->ac_genParm.gp_closure = (PTR)cib;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD pool: autocommit off CIB (%d)\n", 
			   -1, cib->id);

	    IIapi_autocommit( ac );
	}
	else
	{
	    IIAPI_ABORTPARM		*ap = &cib->api.abrt;

	    MEfill( sizeof( *ap ), 0, (PTR)ap );
	    ap->ab_connHandle = cib->conn;
	    gp = &ap->ab_genParm;

	    if ( async )
	    {
		ap->ab_genParm.gp_callback = gcd_pool_abrt;
		ap->ab_genParm.gp_closure = (PTR)cib;
	    }

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD pool: aborting CIB (%d)\n", 
			   -1, cib->id );

	    IIapi_abort( ap );
	}
    }
    else
    {
	IIAPI_DISCONNPARM	*dp = &cib->api.disc;

	MEfill( sizeof( *dp ), 0, (PTR)dp );
	dp->dc_connHandle = cib->conn;
	gp = &dp->dc_genParm;

	if ( async )
	{
	    dp->dc_genParm.gp_callback = gcd_pool_disc;
	    dp->dc_genParm.gp_closure = (PTR)cib;
	}

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD pool: disconnecting CIB (%d)\n", 
		       -1, cib->id );

	IIapi_disconnect( dp );
    }

    if ( async )  return;
    MEfill( sizeof( wp ), 0, (PTR)&wp );
    wp.wt_timeout = -1;
    while( ! gp->gp_completed )  IIapi_wait( &wp );

    if ( gp->gp_status != IIAPI_ST_SUCCESS )
    {
	IIAPI_ABORTPARM		*ap = &cib->api.abrt;

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD disconnect failed (0x%x) aborting CIB (%d)\n", 
		       -1, gp->gp_status, cib->id );

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_connHandle = cib->conn;
	IIapi_abort( ap );
	while( ! ap->ab_genParm.gp_completed )  IIapi_wait( &wp );

	if ( ap->ab_genParm.gp_status != IIAPI_ST_SUCCESS  &&
	     GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD abort failed (0x%x) for CIB (%d)\n", 
		       -1, ap->ab_genParm.gp_status, cib->id );
    }

    gcd_del_cib( cib );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_pool_auto( II_PTR arg, II_PTR parm )
{
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;
    GCD_CIB		*cib = (GCD_CIB *)arg;

    if ( gp->gp_status == IIAPI_ST_SUCCESS )
    {
	IIAPI_DISCONNPARM	*dp = &cib->api.disc;

	MEfill( sizeof( *dp ), 0, (PTR)dp );
	dp->dc_genParm.gp_callback = gcd_pool_disc;
	dp->dc_genParm.gp_closure = (PTR)cib;
	dp->dc_connHandle = cib->conn;

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD pool: disconnecting CIB (%d)\n", 
		       -1, cib->id );

	IIapi_disconnect( dp );
    }
    else
    {
	IIAPI_ABORTPARM 	*ap = &cib->api.abrt;

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_genParm.gp_callback = gcd_pool_abrt;
	ap->ab_genParm.gp_closure = (PTR)cib;
	ap->ab_connHandle = cib->conn;

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD autocommit failed (0x%x) aborting CIB (%d)\n", 
		       -1, gp->gp_status, cib->id );

	IIapi_abort( ap );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
gcd_pool_disc( II_PTR arg, II_PTR parm )
{
    GCD_CIB		*cib = (GCD_CIB *)arg;
    IIAPI_DISCONNPARM	*dp = (IIAPI_DISCONNPARM *)parm;

    if ( dp->dc_genParm.gp_status == IIAPI_ST_SUCCESS )
	gcd_del_cib( cib );
    else
    {
	IIAPI_ABORTPARM *ap = &cib->api.abrt;

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD disconnect failed (0x%x) aborting CIB (%d)\n", 
		       -1, dp->dc_genParm.gp_status, cib->id );

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_genParm.gp_callback = gcd_pool_abrt;
	ap->ab_genParm.gp_closure = (PTR)cib;
	ap->ab_connHandle = cib->conn;
	IIapi_abort( ap );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
gcd_pool_abrt( II_PTR arg, II_PTR parm )
{
    GCD_CIB		*cib = (GCD_CIB *)arg;
    IIAPI_ABORTPARM	*ap = (IIAPI_ABORTPARM *)parm;

    if ( ap->ab_genParm.gp_status != IIAPI_ST_SUCCESS  &&
	 GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD abort failed (0x%x) for CIB (%d)\n", 
		   -1, ap->ab_genParm.gp_status, cib->id );

    gcd_del_cib( cib );
    return;
}

/*
** Name: gcd_pool_info
**
** Description:
**	Loop thru pool queue to retrieve cib information. 
**
** Input:
**	GCD_POOL_INFO * pointer to pool info. 
**
** Output:
**	None.
**
** Returns:
**	count
**
** History:
**	 24-Aug-04 ( wansh01) 
**	    Created.
**	 20-Apr-05 ( wansh01) 
**	    Changed input parm from GCD_SESS_INFO to GCD_POOL_INFO.
**	  6-Jul-06 (usha)
**	    Get GCA assoc_id from the API connection handle.
**	 23-Aug-06 (usha)
**	    Added some stats ( autocommit, isLocal ) for pooled sessions. 
**	 02-Oct-06 (usha)
**	    Simplified the routine to make it work with re-worked gcdadm.c
**	21-Jul-09 (gordy)
**	    User ID and databse are pointer references.
*/

i4
gcd_pool_info( PTR info, bool flag )
{
    
    GCD_SESS_INFO       *sess = (GCD_SESS_INFO *)info;
    QUEUE		*q, *next;
    i4			count=0;
    bool   		selected = FALSE;

    for( q = pool.q_prev; q != &pool; q = next )
    {
	GCD_CIB *cib = (GCD_CIB *)q;
	next = cib->q.q_prev;
        switch( flag )
        {
	    case GCD_POOL_COUNT:
            count++;
            break;
            case GCD_POOL_INFO:
            count++;
            selected = TRUE;
            break;
        }
	if ( selected )
	{
	    i4 gca_id = 0;
	    sess->sess_id = (PTR)cib;
	    gcd_get_gca_id( cib->conn, &gca_id );
	    sess->assoc_id = gca_id;
	    sess->is_system = TRUE;
	    STcopy( "POOLED", sess->state ); 
	    sess->user_id = cib->username;
	    sess->database = cib->database;
	    sess->autocommit = cib->autocommit; 
	    sess->isLocal = cib->isLocal; 
	    sess++;
        }

    }

    if ( GCD_global.gcd_trace_level >= 4 )
        TRdisplay (" gcd_pool_info  count= %d \n", count );

    return ( count );
}
/*
** Name: gcd_pool_remove 
**
** Description:
**	Remove input pool entries in the connection pool.
**
** Input:
**	sid    input pool entry session id. 
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if count entries deleted.
**
** History:
**	14-Sep-04 (wansh01) 
**	    Created.
*/

bool
gcd_pool_remove ( GCD_CIB *  sid )
{

    i4 count = GCD_global.pool_cnt; 
    for( ;count > 0  &&  pool.q_prev != &pool; count-- )
    {
	GCD_CIB *cib = (GCD_CIB *)pool.q_prev;
	if ( cib == sid ) 
	{
	    QUremove( &cib->q );
	    GCD_global.pool_cnt--;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD pool: flushing CIB (%d)\n", -1, cib->id);

	    gcd_pool_del( cib, TRUE );
	    break;
	}
    }

    return( count <= 0 );
}

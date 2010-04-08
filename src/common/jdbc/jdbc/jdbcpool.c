/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcpool.c
**
** Description:
**	JDBC connection pool manager.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Turn off autocommit or abort connection
**	    if there is an active transaction.
**	 3-Mar-00 (gordy)
**	    Added jdbc_pool_flush().
*/	

/*
** Connection pool: queue to hold CIBs.
*/
static	QUEUE	pool;

/*
** Forward references.
*/
static	void	jdbc_pool_del( JDBC_CIB *, bool );

II_FAR II_CALLBACK II_VOID	jdbc_pool_auto( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_pool_disc( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_pool_abrt( II_PTR, II_PTR );



/*
** Name: jdbc_pool_init
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
jdbc_pool_init( void )
{
    QUinit( &pool );

    if ( JDBC_global.pool_idle_limit )
    {
	TMnow( &JDBC_global.pool_check );
	JDBC_global.pool_check.TM_secs += JDBC_global.pool_idle_limit;
    }

    return( OK );
}


/*
** Name: jdbc_pool_term
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
jdbc_pool_term()
{
    QUEUE *q;

    for( q = pool.q_next; q != &pool; q = pool.q_next )
    {
	QUremove( &((JDBC_CIB *)q)->q );
	JDBC_global.pool_cnt--;
	jdbc_pool_del( (JDBC_CIB *)q, FALSE );
    }

    return;
}


/*
** Name: jdbc_pool_save
**
** Description:
**	Save a connection in the connection pool.  Returns FALSE
**	if the connection pool is full and the connection was not
**	saved.
**
**	If the Connection Information Block is saved, it will 
**	eventually either be returned by jdbc_pool_find() or 
**	freed by jdbc_del_cib() and the connection disconnected.
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
**	    Use new jdbc_pool_flush() when pool filled.
*/

bool
jdbc_pool_save( JDBC_CIB *cib )
{
    /*
    ** We only save a CIB if it could have
    ** come from the pool originally.  Test
    ** should be the same as jdbc_pool_find().
    */
    if ( ! JDBC_global.pool_max  ||  cib->pool_off  ||
	 (JDBC_global.client_pooling  &&  ! cib->pool_on) )
	return( FALSE );	/* Pooling is off */

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC pool: saving CIB (%d)\n", -1, cib->id );

    /*
    ** Set the expiration time.  If no expiration, set to 0.
    ** Otherwise, set to now plus expiration interval.
    */
    if ( ! JDBC_global.pool_idle_limit )
	cib->expire.TM_secs = 0;
    else
    {
	TMnow( &cib->expire );
	cib->expire.TM_secs += JDBC_global.pool_idle_limit;
    }

    QUinsert( &cib->q, &pool );
    JDBC_global.pool_cnt++;

    /*
    ** If the pool is full, release the oldest CIB.
    */
    if ( JDBC_global.pool_max > 0  &&  
	 JDBC_global.pool_cnt > JDBC_global.pool_max )
	jdbc_pool_flush( JDBC_global.pool_cnt - JDBC_global.pool_max );

    return( TRUE );
}


/*
** Name: jdbc_pool_find
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
**	JDBC_CIB *	Connection Information Block, NULL if no match.
**
** History:
**	 7-Jan-00 (gordy)
**	    Created.
*/

JDBC_CIB *
jdbc_pool_find( JDBC_CIB *target )
{
    QUEUE	*q;
    JDBC_CIB	*cib;

    /*
    ** Pooling is disabled if configured at the server,
    ** inhibited by the client, or if the client has not
    ** requested pooling when the server is configured
    ** for client control of pooling.
    */
    if ( ! JDBC_global.pool_max  ||  target->pool_off  ||
	 (JDBC_global.client_pooling  &&  ! target->pool_on) )
	return( NULL );		/* Pooling is off */

    /*
    ** Search for the oldest matching connection.
    */
    for( 
	 q = pool.q_prev, cib = NULL; 
	 q != &pool; 
	 q = ((JDBC_CIB *)q)->q.q_prev, cib = NULL
       )
    {
	bool match = TRUE;
	cib = (JDBC_CIB *)q;

	/*
	** Compare connection info.
	*/
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
	    JDBC_global.pool_cnt--;
	    break;
	}
    }

    if ( cib  &&  JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC pool: activating CIB (%d)\n", -1, cib->id );

    return( cib );
}


/*
** Name: jdbc_pool_flush
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
jdbc_pool_flush( u_i4 count )
{
    for( ; count > 0  &&  pool.q_prev != &pool; count-- )
    {
	JDBC_CIB *cib = (JDBC_CIB *)pool.q_prev;

	QUremove( &cib->q );
	JDBC_global.pool_cnt--;

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC pool: flushing CIB (%d)\n", -1, cib->id );

	jdbc_pool_del( cib, TRUE );
    }

    return( count <= 0 );
}


/*
** Name: jdbc_pool_check
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
jdbc_pool_check( void )
{
    QUEUE	*q, *next;
    SYSTIME	now;

    /*
    ** Is it time to check the pool?
    */
    TMnow( &now );
    if ( ! JDBC_global.pool_idle_limit  ||
	 TMcmp( &now, &JDBC_global.pool_check ) < 0 )  return;

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC checking connection pool\n", -1 );

    /*
    ** Set next check time to one full period.  If
    ** there are any CIBs which will expire in less
    ** than a full period, we adjust the next check
    ** time accordingly below.
    */
    STRUCT_ASSIGN_MACRO( now, JDBC_global.pool_check );
    JDBC_global.pool_check.TM_secs += JDBC_global.pool_idle_limit;

    for( q = pool.q_prev; q != &pool; q = next )
    {
	JDBC_CIB *cib = (JDBC_CIB *)q;
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
	    cib->expire.TM_secs += JDBC_global.pool_idle_limit;
	}
	else  if ( TMcmp( &now, &cib->expire ) >= 0 )
	{
	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC pool: expired CIB (%d)\n", 
			   -1, cib->id );

	    QUremove( &cib->q );
	    JDBC_global.pool_cnt--;
	    jdbc_pool_del( cib, TRUE );
	}
	else  if ( TMcmp( &cib->expire, &JDBC_global.pool_check ) < 0 )
	{
	    /*
	    ** Reset the check time to match this CIB
	    ** since it will expire prior to the current
	    ** check time.
	    */
	    STRUCT_ASSIGN_MACRO( cib->expire, JDBC_global.pool_check );
	}
    }

    return;
}


/*
** Name: jdbc_pool_del
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
jdbc_pool_del( JDBC_CIB *cib, bool async )
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
	    ac->ac_genParm.gp_callback = jdbc_pool_auto;
	    ac->ac_genParm.gp_closure = (PTR)cib;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC pool: autocommit off CIB (%d)\n", 
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
		ap->ab_genParm.gp_callback = jdbc_pool_abrt;
		ap->ab_genParm.gp_closure = (PTR)cib;
	    }

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC pool: aborting CIB (%d)\n", 
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
	    dp->dc_genParm.gp_callback = jdbc_pool_disc;
	    dp->dc_genParm.gp_closure = (PTR)cib;
	}

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC pool: disconnecting CIB (%d)\n", 
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

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC disconnect failed (0x%x) aborting CIB (%d)\n", 
		       -1, gp->gp_status, cib->id );

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_connHandle = cib->conn;
	IIapi_abort( ap );
	while( ! ap->ab_genParm.gp_completed )  IIapi_wait( &wp );

	if ( ap->ab_genParm.gp_status != IIAPI_ST_SUCCESS  &&
	     JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC abort failed (0x%x) for CIB (%d)\n", 
		       -1, ap->ab_genParm.gp_status, cib->id );
    }

    jdbc_del_cib( cib );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_pool_auto( II_PTR arg, II_PTR parm )
{
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;
    JDBC_CIB		*cib = (JDBC_CIB *)arg;

    if ( gp->gp_status == IIAPI_ST_SUCCESS )
    {
	IIAPI_DISCONNPARM	*dp = &cib->api.disc;

	MEfill( sizeof( *dp ), 0, (PTR)dp );
	dp->dc_genParm.gp_callback = jdbc_pool_disc;
	dp->dc_genParm.gp_closure = (PTR)cib;
	dp->dc_connHandle = cib->conn;

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC pool: disconnecting CIB (%d)\n", 
		       -1, cib->id );

	IIapi_disconnect( dp );
    }
    else
    {
	IIAPI_ABORTPARM 	*ap = &cib->api.abrt;

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_genParm.gp_callback = jdbc_pool_abrt;
	ap->ab_genParm.gp_closure = (PTR)cib;
	ap->ab_connHandle = cib->conn;

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC autocommit failed (0x%x) aborting CIB (%d)\n", 
		       -1, gp->gp_status, cib->id );

	IIapi_abort( ap );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_pool_disc( II_PTR arg, II_PTR parm )
{
    JDBC_CIB		*cib = (JDBC_CIB *)arg;
    IIAPI_DISCONNPARM	*dp = (IIAPI_DISCONNPARM *)parm;

    if ( dp->dc_genParm.gp_status == IIAPI_ST_SUCCESS )
	jdbc_del_cib( cib );
    else
    {
	IIAPI_ABORTPARM *ap = &cib->api.abrt;

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC disconnect failed (0x%x) aborting CIB (%d)\n", 
		       -1, dp->dc_genParm.gp_status, cib->id );

	MEfill( sizeof( *ap ), 0, (PTR)ap );
	ap->ab_genParm.gp_callback = jdbc_pool_abrt;
	ap->ab_genParm.gp_closure = (PTR)cib;
	ap->ab_connHandle = cib->conn;
	IIapi_abort( ap );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_pool_abrt( II_PTR arg, II_PTR parm )
{
    JDBC_CIB		*cib = (JDBC_CIB *)arg;
    IIAPI_ABORTPARM	*ap = (IIAPI_ABORTPARM *)parm;

    if ( ap->ab_genParm.gp_status != IIAPI_ST_SUCCESS  &&
	 JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC abort failed (0x%x) for CIB (%d)\n", 
		   -1, ap->ab_genParm.gp_status, cib->id );

    jdbc_del_cib( cib );
    return;
}


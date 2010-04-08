/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <er.h>
#include    <me.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcm.h>
#include    <gcu.h>


/*
** Name: gcnbchk.c
**
** Description:
**	Function for Name Server bedchecking operations.
**
**	gcn_bchk_all		Initializes bedcheck address queue.
**	gcn_bchk_class		Add servers for class to bedcheck queue.
**	gcn_bchk_next		Returns next address for GCA_REQUEST.
**	gcn_bchk_check		Checks gca request bedcheck status.
**
** History:
**	17-Feb-93 (edg)
**	    Written.
**	11-Mar-93 (edg)
**	    gcn_bchk_check() now takes an argument for server load.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	12-Aug-93 (brucek)
**	    Use single IIGCn_static.next_cleanup instead of the old
**	    per-nq next_cleanup timestamp;
**	    eliminate duplicate entries for same listen address;
**	    don't fail bedcheck if FASTSELECT times out;
**	    bchk_init() now returns number of entries to be checked.
**	19-Aug-93 (brucek)
**	    Add several more no-fail status codes for bedcheck.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	04-Jan-94 (brucek)
**	    Don't fail bedcheck if no MONITOR permission.
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Added prototypes.
**	27-sep-1996 (canor01)
**	    Move global data definitions to gcndata.c.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	29-May-97 (gordy)
**	    Delete registered server info when dropping server.
**	17-Feb-98 (gordy)
**	    Extended configurable GCN parameters.
**	 4-Jun-98 (gordy)
**	    Made gcn_bchk_init() local and added gcn_bchk_all() and
**	    gcn_bchk_class().
**	10-Jun-98 (gordy)
**	    Bedcheck interval reset no matter what reason the queue
**	    is checked.  Added force param to gcn_bchk_init() to
**	    override the bedcheck interval.
**	28-Jul-98 (gordy)
**	    Added checks for server class and installation registration
**	    problems.
**	26-Aug-98 (gordy)
**	    For the SERVERS queue, save the correct class instead of
**	    trying to fix-up when same server is found in class queue.
**	    SHOW SERVERS will not check the correct class otherwise.
**	17-Sep-98 (gordy)
**	    Replaced gcn_bchk_addr() with gcn_bchk_next() for the
**	    generalized FASTSELECT operations.  Do not check 
**	    installation IDs for Name Servers registered in other 
**	    installations.
**	 8-Jan-99 (gordy)
**	    Minor adjustments to tracing.
**	24-Mar-99 (gordy)
**	    Make sure to always delete entries from SERVERS queue
**	    even if the class queue is not accessible.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**      10-Sep-02 (hanal04) Bug 110641 INGSRV2436
**          Make gcn_bchk_check() case insensitive when chekcing the
**          server class.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS class.  Connected registered servers do
**	    not need to be bedchecked.
**      26-Oct-06 (Ralph Loen) Bug 116972
**          In gcn_bchk_check(), add E_GC0157_GCN_BCHK_INFO,
**          E_GC0158_GCN_BCHK_ACCESS, and E_GC0159_GCN_BCHK_FAIL
**          to report Name Server deregistration.
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.  Storage for listen
**	    address is now allocated as part of variable length
**	    queue entries.
*/	

/* Structure used to hold listen addresses to bedcheck */

typedef struct _GCN_BCHK_QUE
{
    QUEUE 	q;
    char	*gcn_type;
    char	listen_addr[ 1 ];	/* Variable length */

} GCN_BCHK_QUE;


/*
** Local functions.
*/
static	VOID	gcn_bchk_del( char *, char * );



/*
** Name: gcn_bchk_init
**
** Description:
**	Add server listen addresses from a Name Queue to a
**	bedcheck queue.  Bedchecking only occurs at set
**	intervals unlessed forced.
**
** Inputs:
**	bchk_q		Bedcheck server queue.
**	nq		Name queue.
**	force		TRUE to force bedcheck, FALSE to check timer.
**
** Outputs:
**	None.
**
** Returns:
**	i4		Number of servers added to bedcheck queue.
**
** Side effects:
**	Memory allocated for bedcheck queue entries.
**
** History:
**      17-Feb-93 (edg)
**          Written.
**	17-Feb-98 (gordy)
**	    Extended configurable GCN parameters.
**	18-Mar-98 (gordy)
**	    Now that we also bedcheck the generic servers queue,
**	    be sure and save the specific queue for an entry so
**	    the queue entry will get properly removed.
**	 4-Jun-98 (gordy)
**	    Made local and extracted some functionality to gcn_bchk_all().
**	10-Jun-98 (gordy)
**	    Bedcheck interval reset no matter what reason the queue
**	    is checked.  Added force param to gcn_bchk_init() to
**	    override the bedcheck interval.
**	26-Aug-98 (gordy)
**	    For the SERVERS queue, save the correct class instead of
**	    trying to fix-up when same server is found in class queue.
**	    SHOW SERVERS will not check the correct class otherwise.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS class.  Connected registered servers do
**	    not need to be bedchecked.
**	18-Aug-09 (gordy)
**	    Listen address storage in queue entries is now variable length.
*/	

static i4
gcn_bchk_init( QUEUE *bchk_q, GCN_QUE_NAME *nq, bool force )
{
    GCN_TUP	tupvec[ GCN_SVR_MAX ];
    GCN_TUP	tupmask;
    GCN_TUP	*tup;
    ER_ARGUMENT earg[1];
    i4		tupcount;
    i4		tuptotal = 0;
    i4		bchktotal = 0;

    /* 
    ** Only transient queues should be bedchecked.
    */
    if ( ! nq->gcn_transient )  return( 0 );

    /*
    ** Only bedcheck if forced or interval has expired.
    */
    if ( ! force  &&  TMcmp( &IIGCn_static.now, &nq->gcn_next_cleanup ) < 0 )
	return( 0 );

    STRUCT_ASSIGN_MACRO( IIGCn_static.now, nq->gcn_next_cleanup );
    nq->gcn_next_cleanup.TM_secs += IIGCn_static.bedcheck_interval;

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_bchk_init: Checking %s\n", nq->gcn_type );

    /* 
    ** Prepare nq for reading.
    ** If we can't get a lock for reading, continue with the
    ** next nq, assuming we'll be able to do it next time 'round
    */
    if ( gcn_nq_lock( nq, FALSE ) != OK )  return( 0 );

    /*
    ** do-while loop gets tuples for this nq a batch at a time via
    ** gcn_nq_ret().  As long as gcn_nq_ret() returns GCN_SVR_MAX for
    ** a tuple cound we have to assume there's more tups to read.
    */
    do
    {
	tupmask.uid = tupmask.obj = tupmask.val = "";
	tupcount = gcn_nq_ret( nq, &tupmask, tuptotal, GCN_SVR_MAX, tupvec );

	/*
	** Loop thru batch of entries we got.  Allocate a becheck q entry
	** for each one and insert into q.
	*/
	for( tup = tupvec; tup < tupvec + tupcount; tuptotal++, tup++ )
	{
	    QUEUE		*q;
	    GCN_BCHK_QUE	*bq;

	    /* Do not need to bedcheck self */
	    if ( ! STcompare( tup->val, IIGCn_static.listen_addr ) )
	    {
		if ( IIGCn_static.trace >= 5 )
		    TRdisplay( "gcn_bchk_init: skip self (%s,%s)\n", 
			       nq->gcn_type, tup->val );
		continue;
	    }

	    /* check for duplicate entries */
	    for( q = bchk_q->q_next; q != bchk_q; q = q->q_next )
		if ( ! STcompare(tup->val, ((GCN_BCHK_QUE *)q)->listen_addr) )
		    break;

	    if ( q != bchk_q )
	    {
		if ( IIGCn_static.trace >= 5 )
		    TRdisplay( "gcn_bchk_init: duplicate entry (%s,%s)\n", 
			       nq->gcn_type, tup->val );
		continue;
	    }

	    /*
	    ** Registered servers maintain a connection with the NS.
	    ** No need to bedcheck the server if it is active.
	    */
	    if ( gcn_get_server( tup->val ) )
	    {
		if ( IIGCn_static.trace >= 5 )
		    TRdisplay( "gcn_bchk_init: server active (%s,%s)\n", 
			       nq->gcn_type, tup->val );
		continue;
	    }

	    /*
	    ** Allocate queue entry and storage for listen address.
	    */
	    bq = (GCN_BCHK_QUE *)MEreqmem( 0, sizeof( GCN_BCHK_QUE ) + 
	    				      STlength( tup->val ), 
					   TRUE, NULL );

	    /*
	    ** if memory allocation failed, log error and continue
	    */
	    if ( ! bq )
	    {
		earg->er_value = ERx("bedcheck queue entries");
		earg->er_size = STlength( earg->er_value );
		gcu_erlog( 0, IIGCn_static.language, 
			   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
		continue; /* maybe take more drastic measure here ? */
	    }

	    /* 
	    ** Populate bedcheck queue entry and insert into queue.
	    */
	    STcopy( tup->val, bq->listen_addr );
	    bq->gcn_type = nq->gcn_type;
	    QUinsert( &bq->q, bchk_q );
	    bchktotal++;

	    if ( IIGCn_static.trace >= 4 )
		TRdisplay( "gcn_bchk_init: class %s port %s\n", 
			   bq->gcn_type, bq->listen_addr );
	}

    } while( tupcount == GCN_SVR_MAX );

    gcn_nq_unlock( nq );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_bchk_init: %d entries to check\n", bchktotal );

    return( bchktotal );
}



/*
** Name: gcn_bchk_all
**
** Description:
**	Loop thru all name q's and load server addresses
**	into bedcheck queue for testing.
**
** Inputs:
**	bchk_q		Queue for bedcheck server entries.
**
** Outputs:
**	None.
**
** Returns:
**	i4		Number of entries added to bedcheck queue.
**
** Side effects:
**	Memory allocated for bedcheck queue entries.
**
** History:
**	 4-Jun-98 (gordy)
**	    Created.
*/	

i4
gcn_bchk_all( QUEUE *bchk_q )
{
    QUEUE 	*q;
    i4		bchk_count = 0;

    /*
    ** Loop thru all name queues and load all 
    ** unique addresses into bedcheck queue.
    */
    for( 
	 q = IIGCn_static.name_queues.q_next;
	 q != &IIGCn_static.name_queues;
	 q = q->q_next 
       )
	bchk_count += gcn_bchk_init( bchk_q, (GCN_QUE_NAME *)q, FALSE );

    return( bchk_count );
}


/*
** Name: gcn_bchk_class
**
** Description:
**	Load server addresses for a specific server class
**	into bedcheck queue for testing.
**
** Inputs:
**	class		Server class.
**	bchk_q		Queue for bedcheck server entries.
**
** Outputs:
**	None.
**
** Returns:
**	i4		Number of entries added to bedcheck queue.
**
** Side effects:
**	Memory allocated for bedcheck queue entries.
**
** History:
**	 4-Jun-98 (gordy)
**	    Created.
*/	

i4
gcn_bchk_class( char *class, QUEUE *bchk_q )
{
    GCN_QUE_NAME	*nq;
    i4			bchk_count = 0;

    if ( (nq = gcn_nq_find( class )) )
	bchk_count += gcn_bchk_init( bchk_q, nq, TRUE );

    return( bchk_count );
}


/*
** Name: gcn_bchk_next
**
** Description:
**	Returns next bedcheck server listen address.
**
** Input:
**	bchk_q		Bedcheck server queue.
**	grcb		Resolve control block.
**	mbreq		Message buffer queue for request info.
**	mbmsg		Message buffer queue for message.
**
** Output:
**	None.
**
** Returns: 
**	STATUS		OK, FAIL if no server, or error code.
**
** History:
**      17-Feb-93 (edg)
**          Written.
**	17-Sep-98 (gordy)
**	    Rewritten to support the new generalized FASTSELECT
**	    operations in the Name Server.
**	15-Jul-04 (gordy)
**	    Empty strings should be passed to gcn_connect_info().
**	13-Aug-09 (gordy)
**	    Removed delegated authetication from grcb.  Reference
**	    listen address rather than copying.
*/	

STATUS
gcn_bchk_next
( 
    QUEUE		*bchk_q,
    GCN_RESOLVE_CB	*grcb, 
    GCN_MBUF		*mbreq,
    GCN_MBUF		*mbmsg 
)
{
    GCN_MBUF	*mbuf;
    STATUS	status;
    char	*p;

    /*
    ** Check to see if there is anything to do.
    */
    if ( bchk_q->q_next == bchk_q )  return( FAIL );

    /*
    ** Initialize the resolve control block.
    */
    gcn_rslv_cleanup( grcb );
    MEfill( sizeof( *grcb ), EOS, (PTR)grcb );

    /*
    ** Setup local catalogs for next server.
    */
    grcb->catl.host[0] = "";
    grcb->catl.addr[0] = ((GCN_BCHK_QUE *)bchk_q->q_next)->listen_addr;
    grcb->catl.tupc = 1;

    if ( (status = gcn_connect_info( grcb, mbreq, "", "", 0, NULL )) != OK )
    {
	MEfree( (PTR)QUremove( bchk_q->q_next ) );
	return( status );
    }

    /*
    ** Build GCM_GET request.
    */
    mbuf = gcn_add_mbuf( mbmsg, TRUE, &status );
    if ( status != OK )  
    {
	MEfree( (PTR)QUremove( bchk_q->q_next ) );
	return( status );
    }

    mbuf->type = GCM_GET;
    p = mbuf->data;

    p += gcu_put_int( p, 0 );		/* Skip error_status */
    p += gcu_put_int( p, 0 );		/* Skip error_index */
    p += gcu_put_int( p, 0 );		/* Skip future[0] */
    p += gcu_put_int( p, 0 );		/* Skip future[1] */
    p += gcu_put_int( p, -1 );		/* Client_perms */
    p += gcu_put_int( p, 1 );		/* Row_count */
    p += gcu_put_int( p, 3 );		/* Element_count */

    p += gcu_put_str( p, GCA_MIB_NO_ASSOCS );	/* Classid */
    p += gcu_put_str( p, "0" );			/* Instance */
    p += gcu_put_str( p, "" );			/* Value */
    p += gcu_put_int( p, 0 );			/* Perms */
    p += gcu_put_str( p, GCA_MIB_INSTALL_ID );
    p += gcu_put_str( p, "0" );
    p += gcu_put_str( p, "" );
    p += gcu_put_int( p, 0 );
    p += gcu_put_str( p, GCA_MIB_CLIENT_SRV_CLASS );
    p += gcu_put_str( p, "0" );
    p += gcu_put_str( p, "" );
    p += gcu_put_int( p, 0 );

    mbuf->used = p - mbuf->data;

    return( OK );
}


/*
** Name: gcn_bchk_check
**
** Description:
**	Check status and server parameters returned by GCA_FASTSELECT 
**	bedcheck call.  Delete address from name queue if necessary.
**
** Inputs:
**	bchk_q		Bedcheck server queue.
**	status		Status of the bedcheck request.
**	no_assocs	Number of associations active in server.
**	install_id	Server's installation ID.
**	class		Server's class.
**
** Outputs:
**	None.
**
** Returns: 
**	VOID
**
** Side effects:
**	Memory is freed for the bedcheck server queue entries.
**
** History:
**      17-Feb-93 (edg)
**          Written.
**	29-May-97 (gordy)
**	    Delete registered server info when dropping server.
**	 4-Jun-98 (gordy)
**	    Moved test for empty queue to Name Server state machine
**	    and changed return type.
**	28-Jul-98 (gordy)
**	    Added checks for server class and installation registration
**	    problems.
**	16-Sep-98 (gordy)
**	    Do not check installation IDs for Name Servers registered
**	    in other installations.
**	24-Mar-99 (gordy)
**	    Only open name queue when need to update.  Pass queue type
**	    to gcn_bchk_del() rather than the name queue.
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**      10-Sep-02 (hanal04) Bug 110641 INGSRV2436
**          Make server_class check case insensitive in order to
**          maintain the behaviour seen in releases prior to AI 2.6.
**      26-Oct-06 (Ralph Loen) Bug 116972
**          Add E_GC0157_GCN_BCHK_INFO, E_GC0158_GCN_BCHK_ACCESS, and 
**          E_GC0159_GCN_BCHK_FAIL to report Name Server deregistration.
*/	

VOID
gcn_bchk_check( QUEUE *bchk_q, STATUS status, 
		i4 no_assocs, char *install_id, char *class )
{
    GCN_BCHK_QUE 	*bq = (GCN_BCHK_QUE *)bchk_q->q_next;    
    ER_ARGUMENT         earg[4];

    /*
    ** Check status from bedcheck operation.
    */
    switch( status )
    {
    case E_GC0000_OK:
	if ( IIGCn_static.flags & GCN_BCHK_INST  &&  
	     STcompare( bq->gcn_type, GCN_NS_REG )  &&
	     STcompare( install_id, IIGCn_static.install_id ) )
	{
	    if ( IIGCn_static.trace >= 1 )
		TRdisplay("bchk_check %s %s: invalid install ID: '%s'\n",
			  bq->gcn_type, bq->listen_addr, install_id);
            earg[0].er_value = bq->gcn_type;
            earg[0].er_size = STlength( earg[0].er_value );
            earg[1].er_value = bq->listen_addr;
            earg[1].er_size = STlength( earg[1].er_value );
            earg[2].er_value = install_id;
            earg[2].er_size = STlength( earg[2].er_value );
            earg[3].er_value = class;
            earg[3].er_size = STlength( earg[3].er_value );
            gcu_erlog( 0, IIGCn_static.language,
                E_GC0157_GCN_BCHK_INFO, NULL, 4, (PTR)earg );

	    gcn_bchk_del( bq->gcn_type, bq->listen_addr );
	}
	else  if ( IIGCn_static.flags & GCN_BCHK_CLASS  &&  
		   STbcompare( class, STlength(class),
			       bq->gcn_type, STlength(bq->gcn_type),
			       TRUE) )
	{
	    if ( IIGCn_static.trace >= 1 )
		TRdisplay( "bchk_check %s %s: invalid server class: '%s'\n",
			   bq->gcn_type, bq->listen_addr, class );
            earg[0].er_value = ERx(bq->gcn_type);
            earg[0].er_size = STlength( earg[0].er_value );
            earg[1].er_value = ERx(bq->listen_addr);
            earg[1].er_size = STlength( earg[1].er_value );
            earg[2].er_value = ERx(install_id);
            earg[2].er_size = STlength( earg[2].er_value );
            earg[3].er_value = ERx(class);
            earg[3].er_size = STlength( earg[3].er_value );
            gcu_erlog( 0, IIGCn_static.language,
                E_GC0157_GCN_BCHK_INFO, NULL, 4, (PTR)earg );

	    gcn_bchk_del( bq->gcn_type, bq->listen_addr );
	}
	else
	{
	    GCN_QUE_NAME	*nq;

	    if ( IIGCn_static.trace >= 3 )
		TRdisplay( "bchk_check %s %s: OK (load %d)\n", 
			    bq->gcn_type, bq->listen_addr, no_assocs );

	    /*
	    ** Update the load in the name queue cache.
	    */
	    if ( no_assocs >= 0  &&  (nq = gcn_nq_find( bq->gcn_type )) )
		gcn_nq_assocs_upd( nq, bq->listen_addr, no_assocs );
	}
	break;

    case E_GC0020_TIME_OUT:
    case E_GC0032_NO_PEER:
    case E_GC0033_GCM_NOSUPP:
    case E_GC003F_PM_NOPERM:
	if ( IIGCn_static.flags & GCN_BCHK_PERM )
	{
	    if ( IIGCn_static.trace >= 1 )
		TRdisplay( "bchk_check %s %s: FAIL (status 0x%x)\n", 
			   bq->gcn_type, bq->listen_addr, status );

            earg[0].er_value = bq->gcn_type;
            earg[0].er_size = STlength( earg[0].er_value );
            earg[1].er_value = bq->listen_addr;
            earg[1].er_size = STlength( earg[1].er_value );
            earg[2].er_value = (PTR)&status;
            earg[2].er_size = ER_PTR_ARGUMENT;
            gcu_erlog( 0, IIGCn_static.language,
                E_GC0158_GCN_BCHK_ACCESS, NULL, 3, (PTR)earg );

	    gcn_bchk_del( bq->gcn_type, bq->listen_addr );
	}
	else  if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "bchk_check %s %s: Can't check port (status 0x%x)\n", 
			bq->gcn_type, bq->listen_addr, status );

	break;

    default:
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "bchk_check %s %s: FAIL (status 0x%x)\n", 
			bq->gcn_type, bq->listen_addr, status );

        earg[0].er_value = bq->gcn_type;
        earg[0].er_size = STlength( earg[0].er_value );
        earg[1].er_value = bq->listen_addr;
        earg[1].er_size = STlength( earg[1].er_value );
        earg[2].er_value = (PTR)&status;
        earg[2].er_size = ER_PTR_ARGUMENT;
        gcu_erlog( 0, IIGCn_static.language,
            E_GC0159_GCN_BCHK_FAIL, NULL, 3, (PTR)earg );

	gcn_bchk_del( bq->gcn_type, bq->listen_addr );
	break;
    }

    /*
    ** We're done with the current listen address, free it up.
    */
    MEfree( (PTR)QUremove( (QUEUE *)bq ) );

    return;
}


/*
** Name: gcn_bchk_del
**
** Description:
**	Deletes Name Queue entries for a server.
**
** Input:
**	type		Name Queue type.
**	addr		Listen address of server.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	28-Jul-98 (gordy)
**	    Extracted from gcn_bchk_check().
**	24-Mar-99 (gordy)
**	    Pass in queue type rather than the queue itself so queue
**	    operations can all be done locally.  Always remove entries
**	    from SERVERS queue even if can't access the class queue.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS queue.
*/

static VOID
gcn_bchk_del( char *type, char *addr )
{
    GCN_QUE_NAME	*nq; 
    GCN_TUP		tupmask;

    if ( ! (nq = gcn_nq_find( type ))  ||  gcn_nq_lock( nq, TRUE ) != OK )
    {
	/*
	** Should not happen!
	*/
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "bchk_check: can't find %s queue\n", type );
    }
    else
    {
	/* 
	** Delete row(s) with the listen addr 
	*/
	tupmask.uid = tupmask.obj = "";
	tupmask.val = addr;
	gcn_nq_del( nq, &tupmask );
	gcn_nq_unlock( nq );
    }

    return;
}

/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <pm.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tm.h>
#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcm.h>
#include <gcu.h>
#include <gcxdebug.h>

/*
** Name: gcnsreg.c
**
** Description:
**	Routines involved in the implementation of the installation
**	registry.
**
**	gcn_ir_init		Initialize registry protocol tables.
**	gcn_ir_proto		Prepare rmt connect using registry protocols.
**	gcn_ir_comsvr		Prepare for local connection to comsvr.
**	gcn_ir_bedcheck		Bedcheck the IR Master.
**	gcn_ir_error		Check for error message from bedcheck.
**	gcn_ir_update		Update ID of IR Master (after bedcheck).
**	gcn_ir_register		Register with IR Master.
**	gcn_ir_master		Request Comm Server to become IR Master.
**	gcn_ir_save		Save IR Master info.
**
** History:
**	14-Sep-98 (gordy)
**	    Created.
**	28-Sep-98 (gordy)
**	    Added gcn_ir_init() and gcn_ir_proto() to save/load the
**	    registry protocol information.  Added gcn_ir_master()
**	    to nudge Comm Server into registry master mode.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**	15-Jul-04 (gordy)
**	    Enhanced password encryption between gcn and gcc.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Use gcx_getGCAMsgMap() to retrieve the global gcx_gca_msg
**          reference, otherwise Windows cannot resolve the reference.
**  28-Jan-2004 (fanra01)
**      Bug 113812
**      A fully qualified domain hostname specified as the machine name
**      causes a hostname resolve error.
**      The hostname is used in a PM expression to search for registry
**      entries.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**	18-Aug-09 (gordy)
**	    Remove string length restrictions.
*/

/*
** Why isn't this declared in pm.h?
*/
FUNC_EXTERN	char	*PMgetElem( i4, char * );



/*
** Name: gcn_ir_init
**
** Description:
**	Initialize the global registry protocol tables.
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
**	28-Sep-98 (gordy)
**	    Created.
**  28-Jan-2004 (fanra01)
**      Replace use of global hostname retrieved from GChostname with PMhost.
*/

STATUS
gcn_ir_init( VOID )
{
    STATUS	status;
    PM_SCAN_REC	state;
    char	expbuf[ 80 ], *regexp;
    char	*name, *value;
    i4		count = 0;
    char*   pmhostname = PMhost();
    
    QUinit( &IIGCn_static.registry );
    STprintf( expbuf, ERx("%s.%s.registry.%%.status"),
	      SystemCfgPrefix, pmhostname );

    regexp = PMexpToRegExp( expbuf );
    status = PMscan( regexp, &state, NULL, &name, &value );

    while( status == OK )
    {
	/*
	** Skip protocol if not enabled.
	*/
	if ( ! STcasecmp( value, ERx("ON") ) )
	{
	    GCN_REG	*reg;
	    char	*protocol, *port, *node;
	    char	request[ 128 ], *pv[3];

	    /*
	    ** Get the registry listen address for protocol.
	    */
	    protocol = PMgetElem( 3, name );
	    STprintf( request, ERx("%s.%s.registry.%s.port"),
		      SystemCfgPrefix, pmhostname, protocol );
	    
	    if ( PMget( request, &port ) == OK )
	    {
		/*
		** See if there is a local host configured
		** for the protocol.
		*/
		STprintf( request, ERx("%s.%s.registry.%s.local_host"),
			  SystemCfgPrefix, pmhostname, protocol );
	    
		if ( PMget( request, &node ) != OK )  node = NULL;

		/*
		** Save remote connection info.
		*/
		if ( ! (reg = (GCN_REG *)MEreqmem( 0, sizeof(GCN_REG), 
						   TRUE, NULL )) )
		    return( E_GC0121_GCN_NOMEM );
		else
		{
		    reg->reg_proto = STalloc( protocol );
		    reg->reg_port = STalloc( port );

		    if ( ! reg->reg_proto  ||  ! reg->reg_port )
			return( E_GC0121_GCN_NOMEM );
		    else
		    {
			if ( node )  reg->reg_host = STalloc( node );
			QUinsert( &reg->reg_q, &IIGCn_static.registry );
			count++;
		    }
		}
	    }
	}

	status = PMscan( NULL, &state, NULL, &name, &value );
    }

    /*
    ** If no registry protocols, disable registry.
    */
    if ( ! count )  IIGCn_static.registry_type = GCN_REG_NONE;

    return( OK );
}


/*
** Name: gcn_ir_proto
**
** Description:
**	Append the registry protocol tables on to the 
**	resolve control block remote protocol tables.
**
** Input:
**	grcb		Name Server resolve control block.
**	host		Target host, set to NULL for local host.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	28-Sep-98 (gordy)
**	    Created.
**	28-Jan-04 (gordy)
**	    Check for empty host strings in addition to NULL.
**	18-Aug-09 (gordy)
**	    Entries are now appeneded.  Caller must ensure that
**	    resolve control block is in correct state.  Remote
**	    tables may now reference existing storage.
*/

VOID
gcn_ir_proto( GCN_RESOLVE_CB *grcb, char *host )
{
    GCN_REG	*reg;
    
    for(
	 reg = (GCN_REG *)IIGCn_static.registry.q_next;
	 (QUEUE *)reg != &IIGCn_static.registry;
	 reg = (GCN_REG *)reg->reg_q.q_next
       )
    {
	if ( grcb->catr.tupc >= GCN_SVR_MAX )  break;

	grcb->catr.node[ grcb->catr.tupc ] = (host  &&  *host) ? host 
				: (reg->reg_host  &&  *reg->reg_host) 
				? reg->reg_host : IIGCn_static.hostname;
	grcb->catr.protocol[ grcb->catr.tupc ] = reg->reg_proto;
	grcb->catr.port[ grcb->catr.tupc ] = reg->reg_port;
	grcb->catr.tupc++;
    }

    return;
}


/*
** Name: gcn_ir_comsvr
**
** Description:
**	Initializes the resolve control block and builds the
**	local connection catalogs for available Comm Servers.
**
** Input:
**	grcb		Name Server resolve control block.
**
** Output:
**	None.
**
** Returns:
**	i4		Number of Comm Servers available.
**
** History:
**	14-Sep-98 (gordy)
**	    Created.
**	13-Aug-09 (gordy)
**	    Removed delegated authentication from grcb.  Reference
**	    listen address rather than copying.
*/

i4
gcn_ir_comsvr( GCN_RESOLVE_CB *grcb )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tupmask, tupv[ GCN_SVR_MAX ];
    i4			i, tupc = 0;

    /*
    ** Initialize the resolve control block.
    */
    gcn_rslv_cleanup( grcb );
    MEfill( sizeof( *grcb ), EOS, (PTR)grcb );

    /* 
    ** Setup local catalogs for Comm Servers 
    */
    if ( (nq = gcn_nq_find( GCN_COMSVR ))  &&  gcn_nq_lock( nq, FALSE ) == OK )
    {
	tupmask.uid = tupmask.val = tupmask.obj = "";
	tupc = gcn_nq_ret( nq, &tupmask, 0, GCN_SVR_MAX, tupv );

	for( grcb->catl.tupc = 0; grcb->catl.tupc < tupc; grcb->catl.tupc++ )
	{
	    grcb->catl.host[ grcb->catl.tupc ] = "";
	    grcb->catl.addr[ grcb->catl.tupc ] = tupv[ grcb->catl.tupc ].val;
	}

	gcn_nq_unlock( nq );
    }

    return( tupc );
}


/*
** Name: gcn_ir_bedcheck
**
** Description:
**	Initialize send buffers with connection information and GCM_GET
**	request to obtain ID of the currrent Installation Registry Master
**	Name Server.
**
** Input:
**	grcb		Resolve control block.
**	mbreq		Message buffer queue for connect info.
**	mbmsg		Message buffer queue for message.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	14-Sep-98 (gordy)
**	    Created.
**	28-Sep-98 (gordy)
**	    Extracted protocol handling to gcn_ir_init(), gcn_ir_proto().
**	15-Jul-04 (gordy)
**	    Enhanced password encryption between gcn and gcc.
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough to hold encrypted 
**	    password (even though our password is very small!).
**	 3-Jul-06 (rajus01)
**	    Added support for "register server", "show server" commands to
**	    GCN admin interface.
**	21-Jul-09 (gordy)
**	    Removed fixed length definitions.  Minimum output
**	    password buffer size is now better defined.
*/

STATUS
gcn_ir_bedcheck( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbreq, GCN_MBUF *mbmsg )
{
    GCN_MBUF		*mbuf;
    GCN_REG		*reg;
    char		passwd[ 20 ];	/* Need size > ((len + 8) * 2) */
    char		*m;
    STATUS		status;

    /*
    ** Build GCA_FASTSELECT resolved connection info.
    */
    gcn_ir_proto( grcb, NULL );
    gcn_login( GCN_VLP_COMSVR, 1, TRUE, GCN_NOUSER, GCN_NOPASS, passwd );
    if ( (status = gcn_connect_info( grcb, mbreq, 
    				     GCN_NOUSER, passwd, 0, NULL )) != OK )
	return( status );

    /* 
    ** Build GCM_GET request 
    */
    mbuf = gcn_add_mbuf( mbmsg, TRUE, &status );
    if ( status != OK )  return( status );

    mbuf->type = GCM_GET;
    m = mbuf->data;

    m += gcu_put_int( m, 0 );				/* Skip error_status */
    m += gcu_put_int( m, 0 );				/* Skip error_index */
    m += gcu_put_int( m, 0 );				/* Skip future[ 0 ] */
    m += gcu_put_int( m, 0 );				/* Skip future[ 1 ] */
    m += gcu_put_int( m, -1 );				/* Client_perms */
    m += gcu_put_int( m, 1 );				/* Row_count */
    m += gcu_put_int( m, 2 );				/* Element_count */

    m += gcu_put_str( m, GCA_MIB_CLIENT_LISTEN_ADDRESS );	/* Classid */
    m += gcu_put_str( m, "0" );					/* Instance */
    m += gcu_put_str( m, "" );					/* Value */
    m += gcu_put_int( m, 0 );					/* Perms */

    m += gcu_put_str( m, GCA_MIB_INSTALL_ID );			/* Classid */
    m += gcu_put_str( m, "0" );					/* Instance */
    m += gcu_put_str( m, "" );					/* Value */
    m += gcu_put_int( m, 0 );					/* Perms */

    mbuf->used = m - mbuf->data;

    return( OK );
}


/*
** Name: gcn_ir_error
**
** Description:
**	Check for GCM error response.  Returns OK if valid GCM response.
**
** Input:
**	mbin		Message buffer.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	15-Sep-98 (gordy)
**	    Created.
*/

STATUS
gcn_ir_error( GCN_MBUF *mbin )
{
    STATUS	status;
    char	*m = mbin->data;

    switch( mbin->type )
    {
	case GCA_RELEASE :
	case GCA_ERROR :
	    m += gcu_get_int( m, &status );	/* Skip element count */
	    m += gcu_get_int( m, &status );
	    if ( IIGCn_static.trace >= 4 )
		TRdisplay( "gcn_ir_error: error 0x%x\n", status );
	    break;
	
	case GCM_RESPONSE :
	    m += gcu_get_int( m, &status );
	    if ( status != OK  &&  IIGCn_static.trace >= 4 )
		TRdisplay( "gcn_ir_error: GCM error 0x%x\n", status );
	    break;

	default :
	    if ( IIGCn_static.trace >= 1 )
		TRdisplay( "gcn_ir_error: invalid message type = %s\n",
			   gcx_look( gcx_getGCAMsgMap(), mbin->type ) );
	    status = E_GC0034_GCM_PROTERR;
	    break;
    }

    return( status );
}


/*
** Name: gcn_ir_update
**
** Description:
**	Extract master Name Server listen address and compare
**	with previous master.
**
** Input:
**	mbin		Message buffer.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if new master Name Server.
**
** History:
**	15-Sep-98 (gordy)
**	    Created.
**	 3-Jul-06 (rajus01)
**	    Added support for "register server", "show server" commands to
**	    GCN admin interface.
**	13-Aug-09 (gordy)
**	    Use more appropriate sized temp buffers.  Registry info
**	    is now dynamically allocated.
*/

bool
gcn_ir_update( GCN_MBUF *mbin )
{
    char	addr[ 33 ];
    char	id[ 33 ];
    char	*m = mbin->data;
    i4		elem;
    bool	found = FALSE;
    i4          cnt=0;

    m += sizeof( i4 );				/* Skip error_status */
    m += sizeof( i4 );				/* Skip error_index */
    m += sizeof( i4 );				/* Skip future[0] */
    m += sizeof( i4 );				/* Skip future[1] */
    m += sizeof( i4 );				/* Skip client_perms */
    m += sizeof( i4 );				/* Skip row_count */

    m += gcu_get_int( m, &elem );		/* Element count */

    while( elem-- > 0 )
    {
	char	str[ 65 ];
	char	*class, *value;
	i4	len_c, len_v;

	m += gcm_get_str( m, &class, &len_c );	/* Classid */
	m += gcm_get_str( m, &value, &len_v );	/* Skip instance */
	m += gcm_get_str( m, &value, &len_v );	/* Value */
	m += sizeof( i4 );			/* Skip perms */

	len_c = min( len_c, sizeof( str ) - 1 );
	MEcopy( class, len_c, str );
	str[ len_c ] = EOS;

	if ( ! STcompare( str, GCA_MIB_CLIENT_LISTEN_ADDRESS ) )
	{
	    len_v = min( len_v, sizeof( addr ) - 1 );
	    MEcopy( value, len_v, addr );
	    addr[ len_v ] = EOS;
	    cnt++;
	}
        else if ( ! STcompare( str, GCA_MIB_INSTALL_ID ) )
	{
	    len_v = min( len_v, sizeof( id ) - 1 );
	    MEcopy( value, len_v, id );
	    id[ len_v ] = EOS;
	    cnt++;
	}
    }

    if( cnt > 0 )
        found = TRUE;

    if ( found )
	if ( ! STcompare( addr, IIGCn_static.registry_addr ) && 
	    ! STcompare( id, IIGCn_static.registry_id ) )
	    found = FALSE;	/* No change in registry master */
	else
	{
	    gcn_ir_save( id, addr );

	    /*
	    ** Don't indicate a new master if it is
	    ** ourself.  We are already registered
	    ** as a registry Name Server with ourself.
	    */
	    if ( ! STcompare( addr, IIGCn_static.listen_addr ) && 
		 ! STcompare( id, IIGCn_static.install_id ) )
		found = FALSE;
	}

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_ir_update: %sMaster address = %s , Master id = %s\n",
		   found ? "new " : "", IIGCn_static.registry_addr, 
						IIGCn_static.registry_id );

    return( found );
}


/*
** Name: gcn_ir_register
**
** Description:
**	Build connection info and GCA_NS_OPER message to
**	register with master Name Server.
**
** Input:
**	grcb		Name Server resolve control block.
**	mbout		Message buffer queue for connect info & message.
**
** Output:
**	None.
**
** Returns:
**	i4		Number of Comm Servers available.
**
** History:
**	14-Sep-98 (gordy)
**	    Created.
**      13-Jan-1999 (fanra01)
**          Add registered name server MIB flag.
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**	15-Jul-04 (gordy)
**	    Pass empty strings to gcn_connect_info() rather than NULL.
**	13-Aug-09 (gordy)
**	    Removed delegated authentication from grcb.  Reference
**	    listen address rather than copying.
*/

STATUS
gcn_ir_register( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbout )
{
    STATUS	status;
    GCN_MBUF	*mbuf;
    char	*m;

    /*
    ** Initialize the resolve control block.
    */
    gcn_rslv_cleanup( grcb );
    MEfill( sizeof( *grcb ), EOS, (PTR)grcb );

    grcb->catl.host[0] = "";
    grcb->catl.addr[0] = IIGCn_static.registry_addr;
    grcb->catl.tupc = 1;

    if ( (status = gcn_connect_info( grcb, mbout, "", "", 0, NULL )) != OK )
	return( status );

    /* 
    ** Build GCN_OPER request 
    */
    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  return( status );

    mbuf->type = GCN_NS_OPER;
    m = mbuf->data;

    m += gcu_put_int( m, GCN_PUB_FLAG | GCA_RG_NMSVR);
    m += gcu_put_int( m, GCN_ADD );
    m += gcu_put_str( m, IIGCn_static.install_id );
    m += gcu_put_int( m, 1 );
    m += gcu_put_str( m, GCN_NS_REG );
    m += gcu_put_str( m, IIGCn_static.username );
    m += gcu_put_str( m, IIGCn_static.install_id );
    m += gcu_put_str( m, IIGCn_static.listen_addr );

    mbuf->used = m - mbuf->data;
    return( OK );
}


/*
** Name: gcn_ir_master
**
** Description:
**	Initialize send buffers with connection information and GCM_SET
**	request to place Comm Server in master registry mode.
**
** Input:
**	grcb		Resolve control block.
**	mbreq		Message buffer queue for connect info.
**	mbmsg		Message buffer queue for message.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	28-Sep-98 (gordy)
**	    Created.
**	15-Jul-04 (gordy)
**	    Pass empty strings to gcn_connect_info() rather than NULL.
*/

STATUS
gcn_ir_master( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbreq, GCN_MBUF *mbmsg )
{
    GCN_MBUF		*mbuf;
    STATUS		status;
    GCN_REG		*reg;
    char		*m;

    /*
    ** Build GCA_FASTSELECT resolved connection info.
    */
    if ( (status = gcn_connect_info( grcb, mbreq, "", "", 0, NULL )) != OK )
	return( status );

    /* 
    ** Build GCM_SET request 
    */
    mbuf = gcn_add_mbuf( mbmsg, TRUE, &status );
    if ( status != OK )  return( status );

    mbuf->type = GCM_SET;
    m = mbuf->data;

    m += gcu_put_int( m, 0 );				/* Skip error_status */
    m += gcu_put_int( m, 0 );				/* Skip error_index */
    m += gcu_put_int( m, 0 );				/* Skip future[ 0 ] */
    m += gcu_put_int( m, 0 );				/* Skip future[ 1 ] */
    m += gcu_put_int( m, -1 );				/* Client_perms */
    m += gcu_put_int( m, 1 );				/* Row_count */
    m += gcu_put_int( m, 1 );				/* Element_count */

    m += gcu_put_str( m, GCC_MIB_REGISTRY_MODE );		/* Classid */
    m += gcu_put_str( m, "0" );					/* Instance */
    m += gcu_put_str( m, ERx("master") );			/* Value */
    m += gcu_put_int( m, 0 );					/* Perms */

    mbuf->used = m - mbuf->data;

    return( OK );
}


/*
** Name: gcn_ir_save
**
** Description:
**	Save IR Master information.
**
** Input:
**	id	IR Master installation ID, NULL if no change.
**	addr	IR Master listen address, NULL if no change.
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

STATUS
gcn_ir_save( char *id, char *addr )
{
    if ( id )
    {
	/*
	** Special case our own installation ID which can be
	** simply referenced rather than dynamically allocated.
	*/
	if ( IIGCn_static.registry_id  &&
	     IIGCn_static.registry_id != IIGCn_static.install_id )  
	    MEfree( (PTR)IIGCn_static.registry_id );

	if ( STcompare( id, IIGCn_static.install_id ) == 0 )
	    IIGCn_static.registry_id = IIGCn_static.install_id;
	else  if ( ! (IIGCn_static.registry_id = STalloc( id )) )
            return( E_GC0121_GCN_NOMEM );
    }

    if ( addr )
    {
	/*
	** Special case our own listen address which can be
	** simply referenced rather than dynamically allocated.
	*/
	if ( IIGCn_static.registry_addr  &&
	     IIGCn_static.registry_addr != IIGCn_static.listen_addr )  
	    MEfree( (PTR)IIGCn_static.registry_addr );

	if ( STcompare( addr, IIGCn_static.listen_addr ) == 0 )
	    IIGCn_static.registry_addr = IIGCn_static.listen_addr;
	else  if ( ! (IIGCn_static.registry_addr = STalloc( addr )) )
            return( E_GC0121_GCN_NOMEM );
    }

    return( OK );
}


/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <gc.h>
#include    <mh.h>
#include    <me.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <tm.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcs.h>
#include    <gcu.h>
#include    <gcaimsg.h>

/*
** Name: gcnsdrct.c
**
** Description:
**	Routines to facilitate direct client connections to remote
**	systems bypassing Ingres/Net.  The local Name Server connects
**	to the remote Name Server using Ingres/Net to determine if
**	the two installations are sufficiently compatible for direct
**	connections (homogeneous, GCA version), resolve the address
**	of the remote server, and authenticate the client on the
**	remote system.  If everything is OK, the client resolve
**	request is formatted such that the client will connect
**	directly to the remote server rather than making a remote
**	connection through a local Comm Server.
**	
**	gcn_direct		Format GCN_NS_RESOLVE message on client
**	gcn_resolved		Process GCN2_RESOLVED message on client
**	gcn_connect_info	Build GCN2_RESOLVED for GCA_REQUEST.
**
** History:
**	14-Sep-98 (gordy)
**	    Extracted from gcnsauth.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Jul-04 (gordy)
**	    Enhanced password encryption between gcn and gcc.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/	


/*
** Name: gcn_direct
**
** Description:
**	Builds GCN2_RESOLVED and GCN_NS_RESOLVE messages for
**	connecting to a remote Name Server so the client can
**	make a direct server connection bypassing the comm
**	server.
**	
** Inputs:
**	grcb		Resolve control block.
**	mbout		MBUF queue for formatted message.
**	deleg_len	Delegated authentication length, 0 if none.
**	deleg		Delegated authentication (NULL if deleg_len is 0).
**	
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	12-Jan-98 (gordy)
**	    Created.
**	 2-Oct-98 (gordy)
**	    Need to restore quoting to database name when
**	    passing on to the remote system.
**	15-Jul-04 (gordy)
**	    Enhanced password encryption between gcn and gcc.
**	    Check flag to see if resolved password is encrypted.
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough for encrypted password.
**	21-Jul-09 (gordy)
**	    Removed delegated authentication from grcb and added 
**	    as parameters.  Declare default sized buffer.  Use
**	    dynamic storage if actual length exceeds default size.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/	

STATUS
gcn_direct( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbout, i4 deleg_len, PTR deleg )
{
    GCN_MBUF 	*mbuf;
    char	buff[ 128 ];
    char	*m, *ptr;
    i4		i, len;
    STATUS	status;

    /*
    ** Produce the GCN2_RESOLVED message to make connection.
    ** Password must be encrypted for the COMSVR, but may
    ** already be encrypted as well.
    */
    if ( grcb->flags & GCN_RSLV_PWD_ENC )
	ptr = grcb->pwd;
    else
    {
	len = (STlength( grcb->pwd ) + 8) * 2;
	ptr = (len < sizeof( buff ))
	      ? buff : (char *)MEreqmem( 0, len + 1, FALSE, NULL );
	if ( ! ptr )  return(  E_GC0121_GCN_NOMEM );

        gcn_login(GCN_VLP_COMSVR, GCN_VLP_V1, TRUE, grcb->usr, grcb->pwd, ptr);
    }

    status = gcn_connect_info( grcb, mbout, 
    			       grcb->usr, ptr, deleg_len, deleg );
    if ( ptr != buff  &&  ptr != grcb->pwd )  MEfree( (PTR)ptr );
    if ( status != OK )  return( status );

    /*
    ** Prepare GCN_NS_RESOLVE message.
    */
    len = STlength( grcb->dbname ) + STlength( grcb->class ) + 10;
    ptr = (len <= sizeof( buff ))
    	  ? buff : (char *)MEreqmem( 0, len, FALSE, NULL );
    if ( ! ptr )  return( E_GC0121_GCN_NOMEM );

    if ( grcb->flags & GCN_RSLV_QUOTE )  gcn_quote( &grcb->dbname );
    gcn_unparse( "", grcb->dbname, grcb->class, ptr );
    if ( grcb->flags & GCN_RSLV_QUOTE )  gcn_unquote( &grcb->dbname );

    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  return status;

    m = mbuf->data;
    m += gcu_put_str( m, "" );
    m += gcu_put_str( m, "" );
    m += gcu_put_str( m, ptr );
    m += gcu_put_str( m, "" );
    m += gcu_put_str( m, "" );
    mbuf->used = m - mbuf->data;
    mbuf->type = GCN_NS_RESOLVE;

    if ( ptr != buff )  MEfree( (PTR)ptr );
    return( OK );
}


/*
** Name: gcn_resolved
**
** Description:
**	Process a GCN2_RESOLVED message response for a direct
**	connection request.  Replaces the local/remote connection
**	info for the current RESOLVE request with the values
**	from the processed message.  Also checks to see if the
**	remote Name Server returned an error instead.
**
** Input:
**	grcb		Resolve control block.
**	mbuf		Message buffer.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	12-Jan-98 (gordy)
**	    Created.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	15-Jul-04 (gordy)
**	    Flag password as encrypted.
**	 3-Aug-09 (gordy)
**	    There doesn't seem to be a good reason to support a direct
**	    connect which is ultimately going to make a remote connection.
**	    This should only happen if the remote system has a remote
**	    vnode defined to force connections to go remote.  Do regular
**	    connect for this case, also if no target server is found or
**	    any memory allocation fails.  Skip target server if no auth
**	    provided.  Added buffer sizes to gcn_copy_str().  Use default
**	    sized buffers when possible, otherwise allocate storage.
*/

STATUS
gcn_resolved( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbin )
{
    struct val
    {
    	i4	len;
	char	*ptr;
    };

    struct val	user;
    struct val	host[ GCN_SVR_MAX ];
    struct val	addr[ GCN_SVR_MAX ];
    struct val	auth[ GCN_SVR_MAX ];
    char	*lclptr[ GCN_SVR_MAX ];
    char	*m = mbin->data;
    char	*end = m + mbin->len;
    i4		i, len, count, tupc;
    STATUS	status;

    /*
    ** Check for error.
    */
    if ( mbin->type == GCA_ERROR )
    {
	m += gcu_get_int( m, &count );		/* toss gca_l_e_element */
	m += gcu_get_int( m, &status );
	return( status );
    }

    if ( mbin->type != GCN2_RESOLVED )  return( E_GC0008_INV_MSG_TYPE );

    /*
    ** If a direct connection is not possible, the current connection
    ** control block must be used to provide the remote connection info
    ** to the client.  This forces the following to be done:
    **     1. Extract resolved connection information.
    **     2. Validate extracted info.
    **     3. Allocate memory as needed to rebuild control block.
    **     4. Selectively rebuild control block only when it can
    **	      be done without the possibility of any further error.
    */

    /*
    ** 1. Extract information.
    */

    if ( (end - m) >= sizeof( i4 ) )
	m += gcu_get_int( m, &user.len );	/* Connection user ID */
    else
    {
        user.len = 0;
    	m = end;
    }

    user.len = min( user.len, end - m );
    user.ptr = m;
    m += user.len;

    if ( (end - m) >= sizeof( i4 ) )
	m += gcu_get_int( m, &count );		/* Server connection info */
    else
    {
    	count = 0;
	m = end;
    }

    for( tupc = 0, i = 0; i < count; i++ )
    {
	if ( (end - m) < sizeof( i4 ) )  break;
	m += gcu_get_int( m, &len );		/* Host name */
	len = min( len, end - m );

	if ( tupc < GCN_SVR_MAX )
	{
	    /*
	    ** If the remote Name Server does not return
	    ** a host name, use the node name which was
	    ** used for the remote resolve request.
	    */
	    if ( ! len  ||  *m == EOS )
	    {
	    	host[ tupc ].len = STlength( grcb->catr.node[0] );
		host[ tupc ].ptr = grcb->catr.node[0];
	    }
	    else
	    {
		host[ tupc ].len = len;
		host[ tupc ].ptr = m;
	    }
	}

	m += len;
	if ( (end - m) < sizeof( i4 ) )  break;
	m += gcu_get_int( m, &len );		/* Server address */
	len = min( len, end - m );

	if ( tupc < GCN_SVR_MAX )
	{
	    addr[ tupc ].len = len;
	    addr[ tupc ].ptr = m;
	}

	m += len;
	if ( (end - m) < sizeof( i4 ) )  break;
	m += gcu_get_int( m, &len );		/* Authentication */
	len = min( len, end - m );

	if ( tupc < GCN_SVR_MAX )
	{
	    auth[ tupc ].len = len;
	    auth[ tupc ].ptr = m;

	    /*
	    ** If the remote Name Server could not generate
	    ** a server authentication, we can't permit a
	    ** direct connection to this server, so ignore
	    ** this entry.
	    */
	    if ( auth[ tupc ].len )  tupc++;
	}

	m += len;
    }

    /*
    ** 2. Validate information.
    */

    /*
    ** Make sure a target server was found.
    */
    if ( ! tupc )  return( E_GC0163_GCN_DC_NO_SERVER );

    /*
    ** No vnode was used when making the RESOLVE request,
    ** so we don't expect remote connection info to be 
    ** returned.  Interestingly enough, the remote
    ** installation could have a remote_vnode set in the
    ** config file which would force requests local to
    ** the remote machine to go remote as well.  Direct
    ** connects are not supported in this situation.
    */
    if ( (end - m) >= sizeof( i4 ) )
	m += gcu_get_int( m, &count );			/* remote addr count */
    else
    {
    	count = 0;
	m = end;
    }

    if ( ! count )
        if ( (end - m) >= sizeof( i4 ) )  
	    m += gcu_get_int( m, &count );		/* remote data count */
	else
    	    m = end;

    /* Unusual case, no local server on remote machine */
    if ( count )  return( E_GC0163_GCN_DC_NO_SERVER );

    /*
    ** 3. Allocate memory.
    */

    /*
    ** Use declared buffer in connection control block
    ** for username if large enough.
    */
    if ( user.len >= sizeof( grcb->usrbuf ) )
    {
	/*
	** Declared buffer too small, allocate buffer.
	*/
	m = user.ptr;
    	user.ptr = (char *)MEreqmem( 0, user.len + 1, FALSE, NULL );
	if ( ! user.ptr )  return( E_GC0121_GCN_NOMEM );
	MEcopy( (PTR)m, user.len, (PTR)user.ptr );
	user.ptr[ user.len ] = EOS;
    }
    
    /* Local server information */
    for( status = OK, i = 0; i < tupc; i++ )
    {
	/* Authentication */
    	m = auth[i].ptr;
	auth[i].ptr = (char *)MEreqmem( 0, auth[i].len, FALSE, NULL );

	if ( ! auth[i].ptr )
	{
	    status = E_GC0121_GCN_NOMEM;
	    break;
	}

        MEcopy( (PTR)m, auth[i].len, (PTR)auth[i].ptr );

	/*
	** Use declared buffer for first local server 
	** entry if large enough.
	*/
	len = host[i].len + addr[i].len + 2;

	if ( i == 0  &&  len <= sizeof( grcb->catl.lclbuf ) )
	    lclptr[i] = NULL;	/* Assign later */
	else  if ( ! (lclptr[i] = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	{
	    MEfree( (PTR)auth[i].ptr );
	    status = E_GC0121_GCN_NOMEM;
	    break;
	}
    }

    /*
    ** Free any allocated resources if an allocation error occured.
    */
    if ( status != OK )
    {
	while( i-- )  
	{
	    MEfree( (PTR)auth[i].ptr );
	    if ( lclptr[i] )  MEfree( (PTR)lclptr[i] );
	}

	if ( user.len >= sizeof( grcb->usrbuf ) )  MEfree( (PTR)user.ptr );

	return( status );
    }

    /*
    ** 4. Rebuild connection control block.
    */

    /* Release user ID buffer resources */
    if ( grcb->usrptr  &&  grcb->usrptr != grcb->usrbuf )  
    	MEfree( (PTR)grcb->usrptr );

    /* Release local server resources */
    for( i = 0; i < grcb->catl.tupc; i++ )
    {
	if ( grcb->catl.lclptr[i]  &&  
	     grcb->catl.lclptr[i] != grcb->catl.lclbuf )
	{
	    MEfree( (PTR)grcb->catl.lclptr[i] );
	    grcb->catl.lclptr[i] = NULL;
	}

	if ( grcb->catl.auth_len[i] )
	{
	    MEfree( (PTR)grcb->catl.auth[i] );
	    grcb->catl.auth[i] = NULL;
	    grcb->catl.auth_len[i] = 0;
    	}
    }

    grcb->catl.tupc = 0;

    /* Release remote server resources */
    for( i = 0; i < grcb->catr.tupc; i++ )
    {
	if ( grcb->catr.rmtptr[i]  &&  
	     grcb->catr.rmtptr[i] != grcb->catr.rmtbuf )
	{
	    MEfree( (PTR)grcb->catr.rmtptr[i] );
	    grcb->catr.rmtptr[i] = NULL;
	}
    }

    grcb->catr.tupc = 0;

    /* Save username */
    if ( user.len >= sizeof( grcb->usrbuf ) )
    	grcb->usrptr = user.ptr;
    else
    {
    	MEcopy( (PTR)user.ptr, user.len, (PTR)grcb->usrbuf );
	grcb->usrbuf[ user.len ] = EOS;
	grcb->usrptr = grcb->usrbuf;
    }

    grcb->usr = grcb->usrptr;
    grcb->pwd = "";

    /* Save local server information */
    for( i = 0; i < tupc; i++ )
    {
	grcb->catl.lclptr[i] = lclptr[i] ? lclptr[i] : grcb->catl.lclbuf;
	m = grcb->catl.lclptr[i];

	/* Server host */
	MEcopy( (PTR)host[i].ptr, host[i].len, (PTR)m );
	grcb->catl.host[i] = m;
	grcb->catl.host[i][ len ] = EOS;
	m += len + 1;

	/* Server address */
	MEcopy( (PTR)addr[i].ptr, addr[i].len, (PTR)m );
	grcb->catl.addr[i] = m;
	grcb->catl.addr[i][ len ] = EOS;

	/* Authentication */
	grcb->catl.auth[i] = auth[i].ptr;
	grcb->catl.auth_len[i] = auth[i].len;

    	grcb->catl.tupc++;
    }

    return( OK );
}


/*
** Name: gcn_connect_info
**
** Description:
**	Format a GCN2_RESOLVED message to be used as resolved
**	connection info for GCA_REQUEST.
**
** Input:
**	grcb		Resolve Info.
**	mbout		MBUF queue for formatted message.
**	user		User ID for connection.
**	pwd		Password, encrypted for COMSVR.
**	deleg_len	Delegated authentication length, 0 if none.
**	deleg		Delegated authentication (NULL if deleg_len is 0).
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	12-Jan-98 (gordy)
**	    Extracted from gcn_get_auth().
**	17-Mar-98 (gordy)
**	    User GCusername() for effective user ID.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	 5-May-98 (gordy)
**	    Client user ID added to server auth.
**	17-Sep-98 (gordy)
**	    Generate user auth if can't generate server auth.
**	13-Aug-09 (gordy)
**	    Removed delegated authentication from grcb and added as
**	    parameters.
*/

STATUS
gcn_connect_info
(
    GCN_RESOLVE_CB	*grcb,
    GCN_MBUF		*mbout,
    char		*user,
    char		*pwd,
    i4			deleg_len,
    PTR			deleg
)
{
    char	*m, *cnt_ptr;
    GCN_MBUF 	*mbuf;
    STATUS	status;
    i4		i, count;

    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  return( status );

    m = mbuf->data;
    m += gcu_put_str( m, IIGCn_static.username );	/* Local userID */

    /* 
    ** Put the local address catalogs into the buffer 
    */
    m += gcu_put_int( m, grcb->catl.tupc );

    for( i = 0; i < grcb->catl.tupc; i++ )
    {
	char	*ptr;
	i4	len;

	m += gcu_put_str( m, grcb->catl.host[ i ] );
	m += gcu_put_str( m, grcb->catl.addr[ i ] );

	/*
	** Create a server auth for Name Server to
	** local server connection.  If server auth
	** fails (we may not have the server key),
	** generate a user auth instead.
	*/
	m += gcu_put_int( ptr = m, 0 );		/* Place holder */
	len = gcn_server_auth( grcb->catl.addr[ i ], 
			       IIGCn_static.username, IIGCn_static.username,
			       mbuf->size - (m - mbuf->data), (PTR)m );
	if ( ! len )  
	    len = gcn_user_auth( mbuf->size - (m - mbuf->data), (PTR)m );

	gcu_put_int( ptr, len );
	m += len;
    }

    /* 
    ** Put the remote address catalogs into the buffer 
    */
    m += gcu_put_int( m, grcb->catr.tupc );
		
    for( i = 0; i < grcb->catr.tupc; i++ )
    {
	m += gcu_put_str( m, grcb->catr.node[ i ] );
	m += gcu_put_str( m, grcb->catr.protocol[ i ] );
	m += gcu_put_str( m, grcb->catr.port[ i ] );
    }

    /* 
    ** Fill in the user id to data message 
    */
    cnt_ptr = m;
    count = 0;
    m += gcu_put_int( m, count );		/* place holder */

    if ( grcb->catr.tupc > 0 )
    {
	m += gcu_put_int( m, GCN_RMT_DB );
	m += gcu_put_str( m, "/IINMSVR" );
	count++;

	/*
	** Provide authentication info with the following precedence:
	**
	** 1. Existing remote authentication (installation password).
	** 2. Generate remote auth (delegated auth) for client.
	** 3. Password.
	*/
	if ( grcb->auth_len > 0 )
	{
	    m += gcu_put_int( m, GCN_RMT_AUTH );
	    m += gcu_put_int( m, grcb->auth_len );
	    MEcopy( grcb->auth, grcb->auth_len, m );
	    m += grcb->auth_len;
	    MEfree( grcb->auth );
	    grcb->auth = NULL;
	    grcb->auth_len = 0;
	    count++;
	}
	else  if ( grcb->rmech[0]  &&  deleg_len )
	{
	    char	*ptr = m;
	    char	*len_ptr;
	    i4		len;

	    /* Build header (temporary until remote auth succeeds) */
	    ptr += gcu_put_int( ptr, GCN_RMT_AUTH );
	    len_ptr = ptr;
	    ptr += gcu_put_int( ptr, 0 );
	    len = mbuf->size - (ptr - mbuf->data);

	    if ( gcn_rem_auth( grcb->rmech, grcb->catr.node[0], 
			       deleg_len, deleg, &len, ptr ) == OK )
	    {
		gcu_put_int( len_ptr, len );
		m = ptr + len;
		user = grcb->username;
		count++;
	    }
	    else  if ( *pwd )
	    {
		m += gcu_put_int( m, GCN_RMT_PWD );
		m += gcu_put_str( m, pwd );
		count++;
	    }
	}
	else  if ( *pwd )
	{
	    m += gcu_put_int( m, GCN_RMT_PWD );
	    m += gcu_put_str( m, pwd );
	    count++;
	}

	if ( *user )
	{
	    m += gcu_put_int( m, GCN_RMT_USR );
	    m += gcu_put_str( m, user );
	    count++;
	}

	if ( *grcb->emech )
	{
	    m += gcu_put_int( m, GCN_RMT_EMECH );
	    m += gcu_put_str( m, grcb->emech );
	    count++;
	}

	if ( *grcb->emode )
	{
	    m += gcu_put_int( m, GCN_RMT_EMODE );
	    m += gcu_put_str( m, grcb->emode );
	    count++;
	}
    }

    if ( count )  gcu_put_int( cnt_ptr, count );

    mbuf->used = m - mbuf->data;
    mbuf->type = GCN2_RESOLVED;

    return( OK );
}


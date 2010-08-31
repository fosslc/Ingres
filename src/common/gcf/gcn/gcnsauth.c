/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <ci.h>
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
** Name: GCNSAUTH.C - Name Server <-> Name Server authentication
**
** Description:
**	This module handles the Client <-> Server Name Server authentication
**	protocol, acting as the glue between the GCN_RESOLVE state
**	machine in gcnslsn.c and the ticket handling code in gcnstick.c
**	
**    External routines:
**	gcn_pwd_auth	validate user's password/ticket on server
**	gcn_make_auth	generate authorization tickets on server
**	gcn_can_auth	can this client connection use installation password
**	gcn_use_auth	use a ticket on the client
**	gcn_get_auth	format GCN_NS_AUTHORIZE message on client
**	gcn_got_auth	process GCN_AUTHORIZED message on client
**	gcn_user_auth	generate user authentication
**	gcn_server_auth	generate authentication for registered server
**	gcn_rem_auth	generate remote (delegated) authentication
**	gcn_login	encode/decode vnode login passwords.
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	23-Mar-92 (seiwald)
**	    Hand in length of ticket buffers to gcn_make_tickets() and
**	    gcn_save_tickets() so that we don't have to know their
**	    exact size.
**	23-Mar-92 (seiwald)
**	    Installation password error message shuffled.
**	26-Mar-92 (seiwald)
**	    Installation password now just a GCN_LOGIN entry with user "*".
**	1-Apr-92 (seiwald)
**	    The catalogs [catl, catr] in the GCN_RESOLVE_CB are now
**	    string buffers rather than name queue tuple pointers.
**	1-Apr-92 (seiwald)
**	    Documented.
**	25-Jan-93 (brucek)
**	    Modified gcn_can_auth so that it zeros out the installation
**	    password when making a local connection.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Added prototypes.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	27-May-97 (thoda04)
**	    Included gc.h headers for function prototype of GCusrpwd().
**	29-May-97 (gordy)
**	    New GCF security handling.  Added routines to process
**	    info on registered servers to support GCS server keys.
**	    New GCN format for resolved connection info.  Redefined 
**	    usage of user/password entries.
**	 5-Aug-97 (gordy)
**	    gcn_pwd_auth() return value updated for GCS support.
**	 9-Jan-98 (gordy)
**	    If authorization message is incorrect, format error 
**	    message for client.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	17-Feb-98 (gordy)
**	    Made ticket cache size configurable.
**	17-Mar-98 (gordy)
**	    User GCusername() for effective user ID.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	 5-May-98 (gordy)
**	    Client user ID added to server auth.
**	27-Aug-98 (gordy)
**	    Added gcn_rem_auth() for creating remote authentications
**	    from delegated authentication tokens.
**	17-Sep-98 (gordy)
**	    Extracted functions related to direct connections.
**	    Added gcn_user_auth().
**	22-Oct-98 (gordy)
**	    Cleaned up handling of reserved internal login (NOUSER,NOPASS).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Jan-04 (gordy)
**	    Do not permit installation passwords as password authentication.
**	15-Jul-04 (gordy)
**	    Added gcn_login() to provide enhanced encryption for stored
**	    LOGIN passwords and passwords passed to GCC.
**	16-Jul-04 (gordy)
**	    Extended ticket versions.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**      25-Jan-06 (loera01) SIR 115667
**          In gcn_make_auth(), if the requested ticket cache size is 
**          greater than IIGCn_static.ticket_cache_size (the server-side ticket
**          cache size), adjust the requested size to the value of
**          IIGCn_static.ticket_cache_size.
**	 1-Mar-06 (gordy)
**	    SERVERS queue removed, removed gcn_add_server(), 
**	    gcn_get_server() and gcn_del_server().
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/	

/*
** Password encryption parameters.  These values must
** match similar values in CI, gcu and gcc so that
** encryption of passwords between utilities, gcn and
** gcc will be consistent.  Common encode/decode
** routines are NOT used so as to restrict decryption
** code to servers and not be accessible in libraries.
*/

#define	CRYPT_SIZE	8
#define	CRYPT_DELIM	0xFF
#define	CSET_BASE	0x40
#define	CSET_RANGE	0x80
#define	GCN_PASSWD_END	'0'


/*
** Default encryption mask.
*/
static	u_i1		zeros[ CRYPT_SIZE ] ZERO_FILL;


/*
** Forward references.
*/
static	STATUS		gcn_decrypt( char *, char *, char * );
static	STATUS		gcn_encode( char *, u_i1 *, char *, char * );
static	STATUS		gcn_decode( char *, u_i1 *, char *, char * );


/*{
** Name: gcn_pwd_auth() - validate user's password/ticket on server
**
** Description:
**	Checks a username/password to see if it is a GCN special case.
**
**	Allows a reserved internal login (GCN_NOUSER, GCN_NOPASS) to 
**	access the Name Server (which greatly restricts the activities
**	permitted for GCN_NOUSER).  Installation passwords (GCN_NOUSER,
**	pwd) are rejected since an IP ticket is expected instead.  The
**	password may in fact be an IP ticket, in which case the ticket
**	is validated.
**
**	Returns OK if login validates successfully.  Returns an error
**	code if login fails validation.  Returns FAIL if login is not
**	special GCN login and has not been validated.
**
** Inputs:
**	user		Username
**	passwd		Password
**
** Returns:
**	STATUS		OK, error code, or FAIL if caller must validate.
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	 5-Aug-97 (gordy)
**	    We no longer call GCusrpwd() directly.  GCS will do this
**	    for us if we return FAIL.
**	22-Oct-98 (gordy)
**	    Check for both GCN_NOUSER and GCN_NOPASS.
**	28-Jan-04 (gordy)
**	    Do not permit installation passwords (IP will likely fail
**	    the OS verification, but an explicit error is preferred).
**	16-Jul-04 (gordy)
**	    gcn_use_lticket() now takes raw ticket which is cleaner since
**	    we are reversing the CItotext() done when password built.
*/	

STATUS
gcn_pwd_auth( char *user, char *passwd )
{
    STATUS	status = FAIL;

    /*
    ** The reserved internal login is accepted.
    ** An installation password is rejected.
    ** If the password appears to be a ticket,
    ** it must match an existing ticket.  Other-
    ** wise, the caller must validate the login.
    */
    if ( ! STcompare( user, GCN_NOUSER ) )
    {
	if ( ! STcompare(passwd, GCN_NOPASS) )  
	    status = OK;
	else
	    status = E_GC0145_GCN_INPW_NOT_ALLOWED;
    }
    else  if ( ! MEcmp( passwd, GCN_AUTH_TICKET_MAG, GCN_L_AUTH_TICKET_MAG ) )
    {
	u_i1	lticket[ GCN_L_TICKET ];
	i4	len;

	CItobin( (PTR)(passwd + GCN_L_AUTH_TICKET_MAG), &len, lticket );
	status = gcn_use_lticket( user, lticket, len );
    }

    return( status );
}


/*{
** Name: gcn_make_auth() - generate authorization tickets on server
**
** Description:
**	Responds to a GCN_NS_AUTHORIZE message from a client Name Server
**	by calling gcn_make_tickets() to load a GCN_AUTHORIZED message 
**	with tickets for the user.
**	
** Inputs:
**	mbin	GCN_NS_AUTHORIZE message
**
** Outputs:
**	mbout	GCN_AUTHORIZED or GCN_ERROR message
**
** Returns:
**	STATUS
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	4-Dec-97 (rajus01)
**	   gcn_make_auth() now takes an additonal protocol parm. 
**	 9-Jan-98 (gordy)
**	    If authorization message is incorrect, format error 
**	    message for client.
**      25-Jan-06 (loera01) SIR 115667
**          If the requested ticket cache size is greater than
**          IIGCn_static.ticket_cache_size (the server-side ticket
**          cache size), adjust the requested size to the value of
**          IIGCn_static.ticket_cache_size.
**	 3-Aug-09 (gordy)
**	    We don't actually use the client/server installation
**	    fields, so skip them.  Username is now used in place.
**	    Use more appropriate size for ticket buffer.
*/	

STATUS
gcn_make_auth( GCN_MBUF *mbin, GCN_MBUF *mbout, i4 prot )
{
    char	*username;
    i4		rq_count;
    i4		auth_type;
    i4		i, userlen;
    STATUS	status;
    GCN_MBUF	*mbuf;
    char	*msg_out;
    char	*m, *end;
    char	rtickvec[ GCN_DFLT_L_TICKVEC ][ GCN_L_TICKET ];

    /* 
    ** Unpack GCN_NS_AUTHORIZE message 
    */
    m = mbin->data;
    end = m + mbin->len;
    m += gcu_get_int( m, &auth_type );

    m += gcu_get_int( m, &userlen );
    username = m;
    userlen = min( userlen, end - m );
    m += userlen;

    m += gcu_get_int( m, &i );		/* Skip */
    m += min( i, end - m );

    m += gcu_get_int( m, &i );		/* Skip */
    m += min( i, end - m );

    m += gcu_get_int( m, &rq_count );
    if ( rq_count > GCN_DFLT_L_TICKVEC )  rq_count = GCN_DFLT_L_TICKVEC;
    if ( rq_count > IIGCn_static.ticket_cache_size )  
        rq_count = IIGCn_static.ticket_cache_size;

    /*
    ** Verify message.
    */
    if ( auth_type != GCN_AUTH_TICKET )  
    {
	gcn_error( E_GC0141_GCN_INPW_NOSUPP, NULL, NULL, mbout );
	return( E_GC0141_GCN_INPW_NOSUPP );
    }

    /* 
    ** Generate tickets 
    */
    username[ userlen ] = EOS;
    status = gcn_make_tickets( username, IIGCn_static.lcl_vnode,
			       &rtickvec[0][0], rq_count, GCN_L_TICKET,
			       prot );
    if ( status != OK )
    {
	gcn_error( status, (CL_ERR_DESC *)0, (char *)0, mbout );
	return( status );
    }

    /* 
    ** Pack GCN_AUTHORIZED message 
    */
    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  return( status );

    m = mbuf->data;
    m += gcu_put_int( m, auth_type );
    m += gcu_put_int( m, rq_count );

    for( i = 0; i < rq_count; i++ )  m += gcu_put_str( m, rtickvec[ i ] );

    mbuf->used = m - mbuf->data;
    mbuf->type = GCN_AUTHORIZED;

    return( OK );
}


/*{
** Name: gcn_can_auth
**
** Description:
**	Check to see if the connection resolve control block is for  
**	a remote connection and if there is an installation password 
**	defined for the vnode.  Prepare the installation password 
**	for ticket processing.
**
**	Installation passwords are identified by a special user ID.
**
** Inputs:
**	grcb		GCN_RESOLVE control block
**
** Returns:
**	bool		TRUE if installation password, FALSE if not.
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	22-Oct-98 (gordy)
**	    Don't treat reserved internal login (NOUSER,NOPASS)
**	    as installation password (NOUSER,pwd).
**	15-Jul-04 (gordy)
**	    Resolved password is in plain text unless flagged as encrypted.
**	 3-Aug-09 (gordy)
**	    Installation password buffer is dynamically allocated.
*/

bool
gcn_can_auth( GCN_RESOLVE_CB *grcb )
{
    /*
    ** If the installation password has already
    ** been processed, just use it.  Otherwise,
    ** check if there is an installation password.
    ** (check for reserved internal login).
    */
    if ( grcb->flags & GCN_RSLV_IP )
    {
	grcb->pwd = grcb->ip;
	grcb->flags &= ~GCN_RSLV_PWD_ENC;
    }
    else  if ( *grcb->vnode  &&  ! STcompare( grcb->usr, GCN_NOUSER )  &&
	       STcompare( grcb->pwd, GCN_NOPASS ) )
    {
	/*
	** Save installation password.  Flag the fact that 
	** the installation password has been decoded in 
	** case we come through here again.
	*/
	if ( grcb->ip )  MEfree( (PTR)grcb->ip );	/* Should not happen */
	grcb->ip = STalloc( grcb->pwd );
	grcb->pwd = grcb->ip;
	grcb->flags |= GCN_RSLV_IP;
    }

    return( ((grcb->flags & GCN_RSLV_IP) != 0) );
}


/*{
** Name: gcn_use_auth() - use a ticket on the client
**
** Description:
**	Calls gcn_use_rticket() to get a ticket for the user from the
**	saved cache of tickets, and formats it properly for use as a
**	password.  Returns FAIL if no tickets in the cache.
**	
** Inputs:
**	grcb		GCN_RESOLVE control block
**	
** Returns:
**	STATUS
**	    OK
**	    FAIL	no tickets left - call gcn_get_auth()
**	    any other status
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	29-May-97 (gordy)
**	    Redefined usage of user/password entries.
**	 4-Dec-97 (rajus01)
**	    Clients using GCA_PROTOCOL_LEVEL_63 and above 
**	    no longer send tickets in text form. Tickets
**	    are transmitted as remote authentication. GCS
**	    builds the remote authentication using the raw
**	    ticket.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 9-Jul-04 (gordy)
**	    Use password encapsulation if any failure while trying
**	    to build GCS authentication.  IP Auth now distinct GCS
**	    operation.
**	15-Jul-04 (gordy)
**	    Resolved password may now be left in plain text.
**	16-Jul-04 (gordy)
**	    Extended ticket versions.
**	25-Oct-05 (gordy)
**	    Use GCN_AUTH_MAX_LEN for authentication buffer size.
**	 3-Aug-09 (gordy)
**	    Use more appropriate size for ticket buffer.  Use dynamic
**	    storage if password length exceeds size of default buffer.
*/	

STATUS
gcn_use_auth( GCN_RESOLVE_CB *grcb, i4 prot )
{
    STATUS		status;
    u_i1		ticket[ GCN_L_TICKET ];
    GCS_REM_PARM        rem;
    i4			ticket_len = 0;

    /*
    ** Try to get a usable ticket. 
    */
    status = gcn_use_rticket( grcb->username, grcb->vnode, grcb->pwd, 
			      ticket, &ticket_len );
    if ( status != OK )  return( status );

    /* 
    ** Generate a remote authentication if supported
    ** by client and remote Name Server.  Otherwise,
    ** the ticket is encapsulated in password. Client
    ** support is determined by the protocol level,
    ** while the ticket indicates Name Server support.
    */
    if ( prot >= GCA_PROTOCOL_LEVEL_63  &&  
	 ticket[ 0 ] == GCN_TICK_MAG  &&  ticket[ 1 ] >= GCN_V1_TICK_MAG )
    {
 	/*
	** Allocate 256 bytes buffer to hold RA.
	*/
	grcb->auth = MEreqmem( 0, GCN_AUTH_MAX_LEN, FALSE, &status );

	if (  grcb->auth )
	{
	    rem.token    = (PTR)ticket;
	    rem.length   = ticket_len;
	    rem.size     = GCN_AUTH_MAX_LEN;
	    rem.buffer   = grcb->auth;

	    /*
	    ** Call GCS to build RA for Server Name Server.
	    */
	    status = IIgcs_call( GCS_OP_IP_AUTH, GCS_MECH_INGRES, (PTR)&rem );

	    if ( status == OK )  
	    {
		grcb->usr = grcb->username;
		grcb->pwd = "";
		grcb->flags &= ~GCN_RSLV_PWD_ENC;
		grcb->auth_len = rem.size;
		return( OK );
	    }

	    /*
	    ** Restore state and fall-through to password encapsulation.
	    */
	    MEfree( grcb->auth );
	    grcb->auth = NULL;
	    grcb->auth_len = 0;
	}
    }

    /* 
    ** Convert ticket into a password which can be detected as
    ** a ticket by the remote NS.  The installation password
    ** (which may have been in grcb->pwdbuf) has been saved,
    ** so use the password buffer to hold the formatted ticket.
    */
    if ( grcb->pwdptr  &&  grcb->pwdptr != grcb->pwdbuf )  
    	MEfree( (PTR)grcb->pwdptr );
    grcb->pwdptr = grcb->pwdbuf;

    MEcopy( GCN_AUTH_TICKET_MAG, GCN_L_AUTH_TICKET_MAG, grcb->pwdptr );
    CItotext( ticket, ticket_len, grcb->pwdptr + GCN_L_AUTH_TICKET_MAG );

    /*
    ** Send the ticket as the connection password.
    */
    grcb->usr = grcb->username;
    grcb->pwd = grcb->pwdptr;
    grcb->flags &= ~GCN_RSLV_PWD_ENC;
    return( OK );
}


/*{
** Name: gcn_get_auth() - format GCN_NS_AUTHORIZE message on client
**
** Description:
**	Builds a GCN_NS_AUTHORIZE message on client Name Server for sending
**	to server Name Server, and places on the outgoing MBUF chain for
**	sending.
**	
** Inputs:
**	grcb		GCN_RESOLVE control block
**	mbout		queue to place GCN_NS_AUTHORIZE message
**	
** Returns:
**	STATUS
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	29-May-97 (gordy)
**	    New GCN format for resolved connection info.
**	    Redefined usage of user/password entries.
**	 4-dec-97 (rajus01)
**	    Now, resolved message carries length of remote auth,
**	    remote auth, emech, emode entries.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	17-Feb-98 (gordy)
**	    Made ticket cache size configurable.
**	15-Jul-04 (gordy)
**	    Enhanced encryption of passwords passed to GCC.
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough to hold encrypted
**	    password (even though our password is very short!).
**	21-Jul-09 (gordy)
**	    Minimum output password buffer size is now better defined.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/	

STATUS
gcn_get_auth( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbout )
{
    char	passwd[ 20 ];	/* Need size > ((len + 8) * 2) */
    char	*m;
    GCN_MBUF 	*mbuf;
    STATUS	status;
    i4		i;

    /* 
    ** Encrypt the NOUSER password, so the local GCC can decrypt it.
    */
    gcn_login(GCN_VLP_COMSVR, GCN_VLP_V1, TRUE, GCN_NOUSER, GCN_NOPASS, passwd);

    /*
    ** Produce the GCN2_RESOLVED message.
    */
    if ( (status = gcn_connect_info( grcb, mbout, 
    				     GCN_NOUSER, passwd, 0, NULL )) != OK )
	return( status );

    /*
    ** Prepare GCN_NS_AUTHORIZE message.
    */
    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  return status;

    m = mbuf->data;
    m += gcu_put_int( m, GCN_AUTH_TICKET );
    m += gcu_put_str( m, grcb->username );
    m += gcu_put_str( m, IIGCn_static.lcl_vnode );
    m += gcu_put_str( m, grcb->vnode );
    m += gcu_put_int( m, IIGCn_static.ticket_cache_size );
    mbuf->used = m - mbuf->data;
    mbuf->type = GCN_NS_AUTHORIZE;

    return( OK );
}


/*{
** Name: gcn_got_auth() - process GCN_AUTHORIZED message on client
**
** Description:
**	Extracts tickets from the GCN_AUTHORIZED message received from
**	the server Name Server in response to the GCN_NS_AUTHORIZE message
**	we sent.  Caches the tickets locally by calling gcn_save_tickets().
**	Handles GCN_ERROR by extracting and returning the status.
**	
** Inputs:
**	grcb		GCN_RESOLVE control block
**	mbin		queue where GCN_AUTHORIZED message is
**	
** Returns:
**	STATUS
**
** History:
**	2-Jan-92 (seiwald)
**	    Written.
**	29-May-97 (gordy)
**	    Redefined usage of user/password entries.
**	 3-Aug-09 (gordy)
**	    Use more appropriate size for ticket buffer.
*/	

STATUS
gcn_got_auth( GCN_RESOLVE_CB *grcb, GCN_MBUF *mbin )
{
    char	*m = mbin->data;
    char	*end = m + mbin->len;
    i4		auth_type;
    i4		rq_count;
    char	rtickvec[ GCN_DFLT_L_TICKVEC ][ GCN_L_TICKET ];
    STATUS	status;
    i4		i;

    /*
    ** Check for error.
    */
    if ( mbin->type == GCA_ERROR )
    {
	m += gcu_get_int( m, &i );	/* toss gca_l_e_element */
	m += gcu_get_int( m, &status );
	return( status );
    }

    /*
    ** Check for valid response.
    */
    if ( mbin->type != GCN_AUTHORIZED )  return( E_GC0144_GCN_INPW_BROKEN );

    m += gcu_get_int( m, &auth_type );
    m += gcu_get_int( m, &rq_count );

    if ( auth_type != GCN_AUTH_TICKET )  return( E_GC0144_GCN_INPW_BROKEN );

    for( i = 0; i < rq_count && i < GCN_DFLT_L_TICKVEC; i++ )
	m += gcn_copy_str( m, end - m, rtickvec[ i ], GCN_L_TICKET );

    status = gcn_save_rtickets( grcb->username, grcb->vnode, 
				&rtickvec[0][0], rq_count, GCN_L_TICKET );

    return( status );
}


/*
** Name: gcn_user_auth
**
** Description:
**	Generate a user authentication.
**
** Input:
**	length		Length of authentication buffer.
**
** Output:
**	buffer		Authentication.
**
** Returns:
**	i4		Length of authentication.
**
** History:
**	17-Sep-98 (gordy)
**	    Created.
*/

i4
gcn_user_auth( i4  length, PTR buffer )
{
    GCS_USR_PARM	parm;
    STATUS		status;

    parm.buffer = (PTR)buffer;
    parm.length = length;
    length = 0;

    status = IIgcs_call( GCS_OP_USR_AUTH, GCS_NO_MECH, (PTR)&parm );

    if ( status == OK )  
	length = parm.length;
    else  if ( IIGCn_static.trace >= 1 )
	TRdisplay( "gcn_user_auth: GCS user auth failure: 0x%x\n", status );

    return( length );
}


/*
** Name: gcn_server_auth
**
** Description:
**	Generate a server authentication for the server
**	and user ID specified.
**
** Input:
**	server		Server listen address.
**	user		User ID of client.
**	alias		User alias connecting to server.
**	length		Length of authentication buffer.
**
** Output:
**	buffer		Authentication.
**
** Returns:
**	i4		Length of authentication.
**
** History:
**	12-Jan-98 (gordy)
**	    Created.
**	20-Feb-98 (gordy)
**	    Make sure the returned length is valid.
**	 5-May-98 (gordy)
**	    Client user ID added to server auth.
**	 1-Mar-06 (gordy)
**	    Server key now directly returned by gcn_get_server().
*/

i4
gcn_server_auth(char *server, char *user, char *alias, i4  length, PTR buffer)
{
    GCS_SRV_PARM	parm;
    STATUS		status;

    /*
    ** A server auth is not absolutely required.  Without a 
    ** server auth, GCA will just produce a user auth for the 
    ** connection instead.  Failure to obtain a server key
    ** is therefore a 'soft' error.
    */
    if ( ! (parm.key = gcn_get_server( server )) )  return( 0 );

    parm.user = user;
    parm.alias = alias;
    parm.server = server;
    parm.length = length;
    parm.buffer = (PTR)buffer;

    if ((status = IIgcs_call(GCS_OP_SRV_AUTH, GCS_NO_MECH, (PTR)&parm)) != OK)
    {
	/*
	** While not a critical error (see note above),
	** we should have been able to produce a server
	** auth since we have a server key.
	*/
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "gcn_server_auth: GCS server (%s) auth failure: 0x%x\n", 
		       server, status );

        return( 0 );
    }

    return( parm.length );
}


/*
** Name: gcn_rem_auth
**
** Description:
**	Generate a remote auth.  A delegated authentication token
**	may be provided, in which case the remote auth will be for
**	the user represented by the token.  Otherwise, the remote
**	auth will be for the user ID associated with the current
**	process.
**
** Input:
**	mech		Mechanism name.
**	host		Remote host name.
**	len		Length of auth token (may be 0).
**	token		Auth token (may be NULL).
**	size		Length of remote auth buffer.
**	buff		Remote auth buffer.
**
** Output:
**	size		Length of remote auth.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	27-Aug-98 (gordy)
**	    Created.
*/

STATUS
gcn_rem_auth( char *mech, char *host, i4  len, PTR token, i4  *size, PTR buff )
{
    GCS_REM_PARM	parm;
    STATUS		status;
    GCS_MECH		mech_id;

    if ( (mech_id = gcs_mech_id( mech )) == GCS_NO_MECH )
    {
	*size = 0;
	return( E_GC1004_SEC_MECH_UNKNOWN );
    }

    parm.host = host;
    parm.length = len;
    parm.token = token;
    parm.size = *size;
    parm.buffer = buff;

    if ( (status = IIgcs_call( GCS_OP_REM_AUTH, mech_id, (PTR)&parm)) != OK )
	*size = 0;
    else
	*size = parm.size;

    return( status );
}


/*
** Name: gcn_login
**
** Description:
**	Handle encryption/decryption of passwords in their various 
**	contexts.  Provides separate encryption for passwords passed 
**	from CLIENT utilities such as netutil, storage of LOGIN 
**	passwords, and passwords being passed to the COMSVR.  Also
**	provides for versioning within the various contexts.
**
**	For encryption, an output buffer of length (pwd_len + 8) * 2
**	is needed.  For decryption, an output buffer the same size
**	as the input will be sufficient.
**
** Input:
**	type	Encryption type: GCN_VLP_CLIENT, GCN_VLP_LOGIN, GCN_VLP_COMSVR.
**	version	Level of encryption: GCN_VLP_V0, GCN_VLP_V1.
**	encrypt	TRUE to encrypt, FALSE to decrypt.
**	key	Encryption key.
**	ipb	Password buffer: pwd to encrypt or encrypted pwd to decrypt.
**
** Output:
**	opb	Password buffer: encrypted password or decrypted password.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/

STATUS
gcn_login( i4 type, i4 version, bool encrypt, char *key, char *ipb, char *opb )
{
    STATUS status = FAIL;

    switch( type )
    {
    case GCN_VLP_CLIENT :
	if ( ! encrypt )  
	    switch( version )
	    {
	    case GCN_VLP_V0 :	status = gcn_decrypt( key, ipb, opb );	break;
	    }
	break;

    case GCN_VLP_LOGIN  :
	if ( encrypt )
	    switch( version )
	    {
	    case GCN_VLP_V0 :	status = gcu_encode( key, ipb, opb );	break;
	    case GCN_VLP_V1 :	status = gcn_encode( key, 
					    (u_i1 *)IIGCn_static.login_mask, 
					    ipb, opb );			break;
	    }
	else
	    switch( version )
	    {
	    case GCN_VLP_V0 :	status = gcn_decrypt( key, ipb, opb );	break;
	    case GCN_VLP_V1 :	status = gcn_decode( key, 
					    (u_i1 *)IIGCn_static.login_mask, 
					    ipb, opb );			break;
	    }
	break;

    case GCN_VLP_COMSVR :
	if ( encrypt )  
	    switch( version )
	    {
	    case GCN_VLP_V0 :	status = gcu_encode( key, ipb, opb );	break;
	    case GCN_VLP_V1 :	status = gcn_encode( key, 
					    (u_i1 *)IIGCn_static.comsvr_mask, 
					    ipb, opb );			break;
	    }
	break;
    }

    return( status );
}


/*
** Name: gcn_decrypt
**
** Description:
**	Decrypt password encrypted by gcu_encode().
**
**	Decrypted password will be smaller than input.
**	An output buffer of the same size as the input
**	will be sufficient to hold the output.
**	
** Input:
**	key	Encryption key.
**	pbuff	Encrypted password.
**
** Output:
**	passwd	Decrypted password.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	15-Jul-04 (gordy)
**	    Moved here from gcnencry.c
**	21-Jul-09 (gordy)
**	    Declare a more appropriate sized key buffer.  Encode
**	    directly into output buffer.
*/

static STATUS
gcn_decrypt( char *key, char *pbuff, char *passwd )
{
    CI_KS	ksch;
    char	kbuff[ CRYPT_SIZE + 1 ];
    i4		len;

    /*
    ** Validate input.
    */
    if ( key == NULL  ||  passwd == NULL )  return( FAIL );

    if ( pbuff == NULL  || *pbuff == EOS )  
    {
	*passwd = EOS;
	return( OK );
    }

    /*
    ** Adjust key to CRYPT_SIZE by padding/truncation.
    */
    for( len = 0; len < CRYPT_SIZE; len++ )  kbuff[len] = *key ? *key++ : ' ';
    kbuff[ CRYPT_SIZE ] = EOS;
    CIsetkey( kbuff, ksch );

    /*
    ** Password is passed as text, convert back to encoded 
    ** binary.  Encoded password should be of correct size.
    ** Decrypt password.
    */
    CItobin( pbuff, &len, (u_char *)passwd );
    if ( len % CRYPT_SIZE )  return( FAIL );
    CIdecode( passwd, len, ksch, passwd );

    /*
    ** Cleanup password by removing trailing
    ** spaces and the end marker.
    */
    passwd[ len ] = EOS;
    len = STtrmwhite( passwd );
    if ( passwd[ --len ] != GCN_PASSWD_END )  return( FAIL );
    passwd[ len ] = EOS;

    return( OK );
}


/*
** Name: gcn_encode
**
** Description:
**	Encrypt a password using key and optional mask for encoding
**	(suitable as input to gcn_decode()).
**
**	An output buffer of size (pwd_len + 8) * 2 is sufficient to
**	hold the encrypted password.
**
** Input:
**	key	Encryption key.
**	mask	Optional mask.
**	ipb	Password.
**
** Output:
**	opb	Encrypted password.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
**	21-Jul-09 (gordy)
**	    Declare default sized temp buffer.  Use dynamic storage
**	    if actual length exceeds default size.
*/

static STATUS
gcn_encode( char *key, u_i1 *mask, char *ipb, char *opb )
{
    CI_KS	ksch;
    char	kbuff[ CRYPT_SIZE + 1 ];
    char	tbuff[ 128 ];
    char	*ptr, *tmp = tbuff;
    i4		tsize = sizeof( tbuff );
    i4		len;
    bool	done;

    /*
    ** Validate input.
    */
    if ( key == NULL  ||  opb == NULL )  return( FAIL );

    if ( ipb == NULL  || *ipb == EOS )  
    {
	*opb = EOS;
	return( OK );
    }

    if ( mask == NULL )  mask = zeros;

    /*
    ** Ensure temp buffer is large enoush.
    */
    len = ((STlength(ipb) + (CRYPT_SIZE - 1)) / (CRYPT_SIZE - 1)) * CRYPT_SIZE;

    if ( len >= tsize )
    {
	tsize = len + 1;
    	tmp = (char *)MEreqmem( 0, tsize, FALSE, NULL );
	if ( ! tmp )  return( FAIL );
    }

    /*
    ** Adjust key to CRYPT_SIZE by duplication/truncation.
    */
    for( ptr = key, len = 0; len < CRYPT_SIZE; len++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ len ] = *ptr++ ^ mask[ len ];
    }

    kbuff[ len ] = EOS;
    CIsetkey( kbuff, ksch );

    /*
    ** The string to be encrypted must be a multiple of
    ** CRYPT_SIZE in length for the block encryption
    ** algorithm used by CIencode().  To provide some
    ** randomness in the output, the first character
    ** or each CRYPT_SIZE block is assigned a random
    ** value.  The remainder of the block is filled
    ** from the string to be encrypted.  If the final
    ** block is not filled, random values are used as
    ** padding.  A fixed delimiter separates the 
    ** original string and the padding (the delimiter 
    ** must always be present).
    */
    for( done = FALSE, len = 0; ! done  &&  (len + CRYPT_SIZE) < tsize; )
    {
	/*
	** First character in each encryption block is random.
	*/
	tmp[ len++ ] = (i4)(CSET_RANGE * MHrand()) + CSET_BASE;

	/*
	** Load string into remainder of encryption block.
	*/
        while( *ipb  &&  len % CRYPT_SIZE )  tmp[ len++ ] = *ipb++;

	/*
	** If encryption block not filled, fill with random padding.
	*/
	if ( len % CRYPT_SIZE )
	{
	    /*
	    ** Padding begins with fixed delimiter.
	    */
	    tmp[ len++ ] = CRYPT_DELIM;
	    done = TRUE;	/* Only done when delimiter appended */

	    /*
	    ** Fill encryption block with random padding.
	    */
	    while( len % CRYPT_SIZE )
		tmp[ len++ ] = (i4)(CSET_RANGE * MHrand()) + CSET_BASE;
	}
    }

    /*
    ** Encrypt the password and convert to text.
    */
    tmp[ len ] = EOS;
    CIencode( tmp, len, ksch, (PTR)tmp );
    CItotext( (u_char *)tmp, len, opb );

    if ( tmp != tbuff )  MEfree( (PTR)tmp );
    return( OK );
}


/*
** Name: gcn_decode
**
** Description:
**	Decrypt a password encoded with a key and optional mask
**	(as produced by gcn_encode()).
**
**	Decrypted password will be smaller than input.  An output
**	buffer same size as input will be sufficient.
**
** Input:
**	key	Encryption key.
**	mask	Optional mask.
**	ipb	Encrypted password.
**
** Output:
**	opb	Decrypted password.
**
** Returns;
**	STATUS	OK or FAIL.
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
**	21-Jul-09 (gordy)
**	    Remove password length restrictions.  Decode directly
**	    into output buffer.
*/

static STATUS
gcn_decode( char *key, u_i1 *mask, char *ipb, char *opb )
{
    CI_KS	ksch;
    char	kbuff[ CRYPT_SIZE + 1 ];
    char	*ptr;
    i4		len;

    /*
    ** Validate input.
    */
    if ( key == NULL  ||  opb == NULL )  return( FAIL );

    if ( ipb == NULL  ||  *ipb == EOS )  
    {
	*opb = EOS;
	return( OK );
    }

    if ( mask == NULL )  mask = zeros;

    /*
    ** Adjust key to CRYPT_SIZE by duplication/truncation.
    */
    for( ptr = key, len = 0; len < CRYPT_SIZE; len++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ len ] = *ptr++ ^ mask[ len ];
    }

    CIsetkey( kbuff, ksch );

    /*
    ** Password is passed as text, convert back to encoded 
    ** binary.  Encoded password should be of correct size.
    ** Decrypt password.
    **
    ** Decode password will be smaller than input.  The
    ** decoded password also needs to be deformatted, which
    ** can be done in place, so decode directly into output
    ** buffer.
    */
    CItobin( ipb, &len, (u_char *)opb );
    if ( len % CRYPT_SIZE )  return( FAIL );
    CIdecode( (PTR)opb, len, ksch, (PTR)opb );

    /*
    ** Remove padding and delimiter from last encryption block.
    */
    while( len  &&  (u_char)opb[ --len ] != CRYPT_DELIM );
    opb[ len ] = EOS;

    /*
    ** Extract original password by skipping random salt at
    ** start of each encryption block.
    */
    for( ptr = opb, len = 0; opb[ len ]; len++ )
	if ( len % CRYPT_SIZE )  *ptr++ = opb[ len ];

    *ptr = EOS;
    return( OK );
}


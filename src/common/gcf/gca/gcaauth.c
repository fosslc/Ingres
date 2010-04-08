/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <gc.h>
#include <me.h>
#include <mu.h>
#include <sl.h>
#include <st.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcs.h>

/*
** Name: gcaauth.c
**
** Description:
**	GCA interface to GCS security facility.
**
**	gca_seclab		Add security label to aux data.
**	gca_usr_auth		Add user authentication to aux data.
**	gca_auth		Add appropriate authentication to aux data.
**	gca_rem_auth		Generate remote authentication.
**	gca_srv_key		Add server key to aux data.
**	gca_cnvrt_v5_peer	Process original peer info.
**
** History:
**	29-May-97 (gordy)
**	    Created.
**	 4-Dec-97 (rajus01)
**	    gca_pwd_auth(), gca_srv_auth() are made static.
**	    Added gca_rem_auth().
**	29-Dec-97 (gordy)
**	    Addec gca_seclab().
**	 5-May-98 (gordy)
**	    User alias added go GCS_OP_PWD_AUTH.  Client user ID added 
**	    to server auth.
**	14-May-98 (gordy)
**	    Support actual remote authentication in addition to pass through.
**	27-May-98 (gordy)
**	    gca_rem_auth() no longer supports 'none'.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jul-09 (gordy)
**	    Support long user names.
*/

/*
** Forward references.
*/

static STATUS	gca_get_key( GCS_GET_KEY_PARM * );
static STATUS   gca_pwd_auth( GCA_ACB *, char *, char *, i4, PTR );
static STATUS   gca_srv_auth( GCA_ACB *, char *, i4, PTR );


/*
** Name: gca_seclab
**
** Description:
**	Append a security label to the aux data if provided in
**	the request parameters and expected by the server.
**
** Input:
**	acb		Association control block.
**	peer		Peer information.
**	parms		GCA_REQUEST parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-Dec-97 (gordy)
**	    Created.
**	17-Jul-09 (gordy)
**	    Use actual size of buffer.
*/

STATUS
gca_seclab( GCA_ACB *acb, GCA_PEER_INFO *peer, GCA_RQ_PARMS *parms )
{
    STATUS status = OK;

    if ( parms->gca_peer_protocol >= GCA_PROTOCOL_LEVEL_61  &&
	 parms->gca_modifiers & GCA_RQ_SECLABEL  &&  parms->gca_sec_label )
    {
	GCA_SEC_LABEL_DATA sldata;

	sldata.label_type = SL_LABEL_TYPE;
	sldata.version = 1;
	sldata.flags = 0;
	MEcopy( parms->gca_sec_label, 
		sizeof( sldata.sec_label ), (PTR)&sldata.sec_label );

	status = gca_aux_element( acb, GCA_ID_SECLAB, 
				  sizeof( sldata ), (PTR)&sldata );
	if ( status == OK )  peer->gca_aux_count++;
    }

    return( status );
}


/*
** Name: gca_usr_auth
**
** Description:
**	Create a user authentication for the current user
**	and append to the aux data area.
**
** Input:
**	acb		Association control block.
**	length		Length of buffer.
**	buffer		Temp buffer to generate user auth.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-May-97 (gordy)
**	    Created.
*/

STATUS
gca_usr_auth( GCA_ACB *acb, i4  length, PTR buffer )
{
    GCS_USR_PARM	parm;
    STATUS		status;

    parm.length = length;
    parm.buffer = buffer;

    if ((status = IIgcs_call(GCS_OP_USR_AUTH, GCS_NO_MECH, (PTR)&parm)) == OK)
	status = gca_aux_element( acb, GCA_ID_AUTH, parm.length, parm.buffer );

    return( status );
}


/*
** Name: gca_pwd_auth
**
** Description:
**	Create a password authentication and append to the aux data area.
**
** Input:
**	acb		Association control block.
**	user		Username.
**	pwd		Password.
**	length		Length of buffer.
**	buffer		Temp buffer to generate password auth.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-May-97 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    User alias added go GCS_OP_PWD_AUTH.
*/

static STATUS
gca_pwd_auth( GCA_ACB *acb, char *user, char *pwd, i4  length, PTR buffer )
{
    GCS_PWD_PARM	parm;
    STATUS		status;

    parm.user = user;
    parm.password = pwd;
    parm.length = length;
    parm.buffer = buffer;

    if ((status = IIgcs_call(GCS_OP_PWD_AUTH, GCS_NO_MECH, (PTR)&parm)) == OK)
	status = gca_aux_element( acb, GCA_ID_AUTH, parm.length, parm.buffer );

    return( status );
}


/*
** Name: gca_rem_auth
**
** Description:
**	Create a remote authentication.
**
** Input:
**	mech		Mechanism name, may be 'default'.
**	host		Name of target host.
**	length		Length of buffer.
**	buffer		Temp buffer to generate password auth.
**
** Output:
**	length		Length of authentication.
**	buffer		Authentication
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	14-May-98 (gordy)
**	    Created.
**	27-May-98 (gordy)
**	    'none' specification no handled by the Name Server.
*/

STATUS
gca_rem_auth( char *mech, char *host, i4  *length, PTR buffer )
{
    GCS_REM_PARM	parm;
    STATUS		status;
    char		*lptr, *buff = (char *)buffer;
    i4			len, blen;
    GCS_MECH		mech_id;

    if ( (mech_id = gcs_mech_id( mech )) == GCS_NO_MECH )
	return( E_GC1004_SEC_MECH_UNKNOWN );

    if ( *length < (2 * sizeof( i4 )) )  
	return( E_GC1010_INSUFFICIENT_BUFFER );

    len = gcu_put_int( buff, GCA_RMT_AUTH );
    buff += len;
    blen = *length - len;
    *length = len;

    lptr = buff;
    len = gcu_put_int( buff, 0 );
    buff += len;
    blen -= len;
    *length += len;

    parm.host = host;
    parm.size = blen;
    parm.buffer = (PTR)buff;
    parm.length = 0;
    parm.token = NULL;

    if ( (status = IIgcs_call(GCS_OP_REM_AUTH, mech_id, (PTR)&parm)) != OK )
	*length = 0;
    else
    {
	gcu_put_int( lptr, parm.size );
	*length += parm.size;
    }

    return( status );
}


/*
** Name: gca_srv_auth
**
** Description:
**	Create a server authentication and append to the aux data area.
**	A server key must have been created, using gca_srv_key(), prior
**	to calling this routine.
**
** Input:
**	acb		Association control block.
**	user		User ID.
**	length		Length of buffer.
**	buffer		Temp buffer to generate password auth.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-May-97 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    Client user ID added to server auth.
*/

static STATUS
gca_srv_auth( GCA_ACB *acb, char *user, i4  length, PTR buffer )
{
    GCS_SRV_PARM	parm;
    STATUS		status;

    parm.user = IIGCa_global.gca_uid;
    parm.alias = user;
    parm.server = IIGCa_global.gca_srvr_id;
    parm.key = IIGCa_global.gca_srvr_key;
    parm.length = length;
    parm.buffer = buffer;

    if ((status = IIgcs_call( GCS_OP_SRV_AUTH, GCS_NO_MECH, (PTR)&parm)) == OK)
	status = gca_aux_element( acb, GCA_ID_AUTH, parm.length, parm.buffer );
    
    return( status );
}


/*
** Name: gca_auth
**
** Description:
**	Generates an authentication based on request information.
**	Remote username/password given precedence over local
**	username/password (accept for Name Server resolution).
**	Registered servers connecting to the Name Server can
**	generate server authentications if no password is given.  
**	A user authentication is generated if no other type of 
**	authentication can be used.
**
** Input:
**	acb		Association control block.
**	peer		Peer information.
**	parms		GCA_REQUEST parameters.
**	nm_svr		TRUE for Name Server resolution.
**	length		Length of buffer.
**	buffer		Temp buffer to generate authentication.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-May-97 (gordy)
**	    Created.
**	 4-Dec-97 (rajus01)
**	    Added check for remote authentication.
**	 5-May-98 (gordy)
**	    User alias added go GCS_OP_PWD_AUTH.
**	17-Jul-09 (gordy)
**	    Peer user ID's dynamically allocated.
*/

STATUS
gca_auth
( 
    GCA_ACB		*acb, 
    GCA_PEER_INFO	*peer, 
    GCA_RQ_PARMS	*parms, 
    bool		nm_svr,
    i4			length, 
    PTR			buffer 
)
{
    STATUS status;

    /*
    ** Saving NULL user cleans up peer state.
    */
    gca_save_peer_user( peer, NULL );

    /*
    ** Remote username/password processed through the
    ** server resolve mechanism when connecting to the
    ** Name Server.  Only use for authentication when
    ** connecting to other servers.
    */
    if ( ! nm_svr  &&  parms->gca_modifiers & GCA_RQ_REMPW )
    {
	gca_save_peer_user( peer, parms->gca_rem_username );
	status = gca_pwd_auth( acb, parms->gca_rem_username,
				    parms->gca_rem_password, length, buffer );
    }
    else
    {
	if ( parms->gca_user_name  &&  *parms->gca_user_name )
	    gca_save_peer_user( peer, parms->gca_user_name );

	if ( parms->gca_modifiers & GCA_RQ_AUTH  &&  parms->gca_l_auth )
	{
	    status = gca_aux_element( acb, GCA_ID_AUTH, 
				      parms->gca_l_auth, parms->gca_auth );
	}
	else  if ( parms->gca_password  &&  *parms->gca_password )
	{
	    status = gca_pwd_auth( acb, 
			    (parms->gca_user_name && *parms->gca_user_name)
				? parms->gca_user_name : IIGCa_global.gca_uid,
			    parms->gca_password, length, buffer );
	}
	else  if ( IIGCa_global.gca_srvr_key[0] != EOS  &&  nm_svr  && 
		   parms->gca_user_name  &&  *parms->gca_user_name )
	{
	    status = gca_srv_auth( acb, parms->gca_user_name, length, buffer );
	}
	else
	{
	    status = gca_usr_auth( acb, length, buffer );
	}
    }

    if ( status == OK )  peer->gca_aux_count++;

    return( status );
}


/*
** Name: gca_srv_key
**
** Description:
**	Create a server key for generating server authentications
**	and append the key to the aux data area.  Only one key is
**	created per server.  If multiple calls are made to this
**	routine, only the key from the first call is used.  Also
**	registers a GCS callback function to retrieve the key
**	generated.
**
** Input:
**	acb		Association control block.
**	server		Server ID.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	29-May-97 (gordy)
**	    Created.
*/

STATUS
gca_srv_key( GCA_ACB *acb, char *server )
{
    GCS_KEY_PARM	key;
    GCS_SET_PARM	set;
    PTR			func;
    STATUS		status;

    if ( IIGCa_global.gca_srvr_key[0] == EOS )
    {
	/*
	** Generate and save server key.
	*/
	key.length = sizeof( IIGCa_global.gca_srvr_key );
	key.buffer = IIGCa_global.gca_srvr_key;

	if ( (status = IIgcs_call( GCS_OP_SRV_KEY, 
				   GCS_NO_MECH, (PTR)&key )) != OK )
	    return( status );

	STcopy( server, IIGCa_global.gca_srvr_id );

	/*
	** Register server key callback function.
	*/
	func = (PTR)gca_get_key;
	set.parm_id = GCS_PARM_GET_KEY_FUNC;
	set.length = sizeof( func );
	set.buffer = (PTR)&func;

	if ( (status = IIgcs_call(GCS_OP_SET, GCS_NO_MECH, (PTR)&set)) != OK )
	    return( status );
    }

    /*
    ** Append key to aux data area.
    */
    status = gca_aux_element( acb, GCA_ID_SRV_KEY, 
			      STlength( IIGCa_global.gca_srvr_key ),
			      IIGCa_global.gca_srvr_key );

    return( status );
}


/*
** Name: gca_get_key
**
** Description:
**	GCS callback function for server key retrieval.
**
** Input:
**	parm		GCS parameters.
**	    server	    Server ID (ignored).
**	    length	    Length of buffer.
**
** Output:
**	parm		GCS parameters.
**	    length	    Length of key.
**	    buffer	    Server key, null terminated.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	29-May-97 (gordy)
**	    Created.
*/

static STATUS
gca_get_key( GCS_GET_KEY_PARM *parm )
{
    STATUS	status = FAIL;
    i4		len = STlength( IIGCa_global.gca_srvr_key );

    if ( len  &&  parm->length >= (len + 1)  &&
	 ! STcompare( IIGCa_global.gca_srvr_id, parm->server ) )  
    {
	STcopy( IIGCa_global.gca_srvr_key, parm->buffer );
	parm->length = len;
	status = OK;
    }

    return( status );
}


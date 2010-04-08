/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <gc.h>
#include <me.h>
#include <mu.h>
#include <st.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcxdebug.h>
#include <gcs.h>
#include "gcsint.h"

/*
** Name: gcssys.c
**
** Description:
**	GCS security mechanism providing backward compatibility 
**	with original Ingres GCA Operating System security.
**
**	User authentication: no user alias is allowed.
**	Password authentication: validated through O.S.
**	Server authentication: client must be trusted or no alias.
**		
** History:
**	19-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Mechanism info now localized to mechanism.
**	 5-Sep-97 (gordy)
**	    Added ID to GCS objects.  Extracted object validation
**	    to gcs_validate().
**      22-Oct-97 (rajus01)
**          Added encryption overhead for mechanism processing.
**	20-Mar-98 (gordy)
**	    Added mechanism encryption level.
**	31-Mar-98 (gordy)
**	    Enhanced the server authentication to validate non-aliased
**	    connections and trusted clients.  This is required to support
**	    installation passwords when this mechanism is configured as
**	    the installation mechanism.  Before this change, no server
**	    auth was sent from Name Server to server, so the Comm Server
**	    would send the password containing the installation password
**	    to the server where it could not be validated.
**	 5-May-98 (gordy)
**	    User alias added to GCS_OP_PWD_AUTH.  Added versioning to 
**	    GCS objects.
**	21-Aug-98 (gordy)
**	    Changes for delegation (not supported) of authentication.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	 6-Jul-99 (rajus01)
**	    Map GC CL errors into GCA errors as done in GCA.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/

/*
** GCS control block.
*/

static	GCS_GLOBAL	*IIgcs_global = NULL;


/*
** External mechanism info.
*/

static	GCS_MECH_INFO	mech_info =
{ 
    "system", 
    GCS_MECH_SYSTEM, 
    GCS_MECH_VERSION_1,
    GCS_CAP_USR_AUTH | GCS_CAP_PWD_AUTH | GCS_CAP_SRV_AUTH, 
    GCS_AVAIL, GCS_ENC_LVL_0, 0 
}; 

static	GCS_MECH_INFO	*gcs_info[] = { &mech_info };


/*
** Forward references
*/

static STATUS           gcs_validate( i4, PTR, GCS_OBJ_HDR * );



/*
** Name: gcs_system
**
** Description:
**	Mechanism main entry point.
**
** Input:
**	op		Operation.
**	parms		Parameters for operation.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	19-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Mechanism info now localized to mechanism.
**	 5-Sep-97 (gordy)
**	    Added ID to GCS objects.  Extracted object validation
**	    to gcs_validate().
**	31-Mar-98 (gordy)
**	    Enhanced the server authentication to validate non-aliased
**	    connections and trusted clients.
**	 5-May-98 (gordy)
**	    User alias added to GCS_OP_PWD_AUTH.  Added versioning to 
**	    GCS objects.
**	21-Aug-98 (gordy)
**	    Changes for delegation (not supported) of authentication.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	 6-Jul-99 (rajus01)
**	    Map GC CL errors into GCA errors as done in GCA.
**	21-Jul-09 (gordy)
**	    Declare standard size password buffer and dynamically
**	    allocate space if larger size is needed.
*/

STATUS
gcs_system( i4  op, PTR parms )
{
    STATUS status = E_GC1000_GCS_FAILURE;

    /*
    ** Tracing needs global info, so check for init request.
    */
    if ( ! IIgcs_global  &&  op == GCS_OP_INIT )  
	IIgcs_global = (GCS_GLOBAL *)parms;

    GCS_TRACE( 3 )
	( "GCS %s: %s\n", mech_info.mech_name, 
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_ops, op ) );

    switch( op )
    {
	case GCS_OP_INIT :
	    status = OK;
	    break;

	case GCS_OP_TERM :
	    status = OK;
	    break;

	case GCS_OP_INFO :
	    {
		GCS_INFO_PARM	*info_parm = (GCS_INFO_PARM *)parms;

		info_parm->mech_count = 1;
		info_parm->mech_info = gcs_info;
		status = OK;
	    }
	    break;

	case GCS_OP_VALIDATE :
	    {
		GCS_VALID_PARM	*valid = (GCS_VALID_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		u_i2		len;

		valid->size = 0;	/* Can't delegate */
		status = gcs_validate( valid->length, valid->auth, &hdr );
		if ( status != OK )  break;
		len = GCS_GET_I2( hdr.obj_len );

		GCS_TRACE( 4 )
		    ( "GCS %s: validating %s (%d bytes)\n",
		      mech_info.mech_name, 
		      (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
						  hdr.obj_id ), (i4)len );

		switch( hdr.obj_id )
		{
		    case GCS_USR_AUTH :
			/*
			** GCA originally did not do user
			** authentication, but did make
			** sure the user wasn't trying to
			** use an alias.
			*/
			GCS_TRACE( 4 )
			    ( "GCS %s: user %s, alias %s\n",
			      mech_info.mech_name, valid->user, valid->alias );

			if ( ! STcompare( valid->user, valid->alias ) )
			    status = OK;
			else
			{
			    GCS_TRACE( 1 )( "GCS %s: user %s can not be %s\n",
					    mech_info.mech_name, 
					    valid->user, valid->alias );
			    status = E_GC1008_INVALID_USER;
			}
			break;
		    
		    case GCS_PWD_AUTH :
		    {
			/*
			** Validate password through O.S.
			*/
			char	pwdbuf[ 128 ];
			char	*pwd;

			pwd = (len >= sizeof( pwdbuf ))
			      ? (char *)(*IIgcs_global->mem_alloc_func)(len + 1)
			      : pwdbuf;

			if ( ! pwd )
			{
			    GCS_TRACE( 1 )( "GCS %s: password too long (%d)\n",
					    mech_info.mech_name, (i4)len );
			    status = E_GC1011_INVALID_DATA_OBJ;
			}
			else
			{
			    MEcopy( (PTR)((char *)valid->auth + sizeof(hdr)), 
				    len, (PTR)pwd );
			    pwd[ len ] = EOS;

			    GCS_TRACE( 4 )
				( "GCS %s: alias %s, password %s\n",
				  mech_info.mech_name, valid->alias, pwd );

			    if ( ! IIgcs_global->usr_pwd_func )
				status = FAIL;		/* Call GCusrpwd() */
			    else
			    {
				GCS_USR_PWD_PARM parm;

				parm.user = valid->alias;
				parm.password = pwd;

				status = (*IIgcs_global->usr_pwd_func)(&parm);
			    }

			    if ( status == FAIL )
			    {
				CL_ERR_DESC sys_err;

				status = GCusrpwd(valid->alias, pwd, &sys_err);
			    }

			    if ( ( status & ~0xFF ) == 
						( E_CL_MASK | E_GC_MASK ) )
				   status ^= 
					( E_CL_MASK | E_GC_MASK ) ^ E_GCF_MASK;

			    if ( status != OK )
			    {
				GCS_TRACE( 1 )( "GCS %s: invalid password\n",
						mech_info.mech_name );
			    }

			    if ( pwd != pwdbuf )
			        (*IIgcs_global->mem_free_func)( (PTR)pwd );
			}
			break;
		    }
		    case GCS_SRV_AUTH :
			{
			    GCS_GET_KEY_PARM	getkey;
			    char		buff[ (sizeof(hdr) * 2) + 1 ];
			    u_char		obj[ (sizeof(hdr) * 2) + 1 ];
			    i4		klen;

			    status = E_GC1009_INVALID_SERVER;
			    if ( ! IIgcs_global->get_key_func )
			    {
				GCS_TRACE( 1 )( "GCS %s: no server key func\n",
						mech_info.mech_name );
				break;
			    }

			    getkey.server = (char *)valid->auth + sizeof(hdr);
			    getkey.length = sizeof( buff );
			    getkey.buffer = buff;

			    GCS_TRACE( 4 )("GCS %s: requesting key for %s\n",
					   mech_info.mech_name, getkey.server);
			    
			    if (((*IIgcs_global->get_key_func)(&getkey)) != OK)
			    {
				GCS_TRACE( 1 )( "GCS %s: key request failed!\n",
						mech_info.mech_name );
				break;
			    }

			    /*
			    ** Validate server key.
			    */
			    buff[ getkey.length ] = EOS;
			    CItobin( buff, &klen, obj );
			    if ( (gcs_validate( klen, (PTR)obj, &hdr )) != OK )
				break;

			    /*
			    ** GCA used the concept of trusted clients 
			    ** to authenticate servers.  Trusted clients 
			    ** run under the same user ID as the server 
			    ** or have been granted the TRUSTED privilege.  
			    ** Non-aliased connections are also allowed.
			    */
			    GCS_TRACE( 4 )
				( "GCS %s: user %s, alias %s, server %s\n",
				  mech_info.mech_name, valid->user, 
				  valid->alias, IIgcs_global->user );

			    if ( 
				 ! STcompare( valid->user, valid->alias )  ||
				 ! STcompare(valid->user, IIgcs_global->user) ||
				 gca_chk_priv( valid->user, 
					       GCA_PRIV_TRUSTED ) == OK
			       )
				status = OK;
			    else
			    {
				GCS_TRACE( 1 )( "GCS %s: user %s not trusted\n",
						mech_info.mech_name, 
						valid->user );
				break;
			    }
			}
			break;

		    default :
			GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
					mech_info.mech_name, (i4)hdr.obj_id );
			status = E_GC1011_INVALID_DATA_OBJ;
			break;
		}
	    }
	    break;

	case GCS_OP_USR_AUTH :
	    {
		GCS_USR_PARM	*usr_parm = (GCS_USR_PARM *)parms;
		GCS_OBJ_HDR	hdr;

		if ( usr_parm->length < sizeof( hdr ) )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
				    mech_info.mech_name, usr_parm->length,
				    (i4)sizeof( hdr ) );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_SYSTEM;
		hdr.obj_id = GCS_USR_AUTH;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, 0 );
		MEcopy( (PTR)&hdr, sizeof( hdr ), usr_parm->buffer );
		usr_parm->length = sizeof( hdr );
	    }
	    break;

	case GCS_OP_PWD_AUTH :
	    {
		GCS_PWD_PARM	*pwd_parm = (GCS_PWD_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		i4		len = STlength( pwd_parm->password );

		GCS_TRACE( 4 )
		    ( "GCS %s: alias %s, password %s\n", 
		      mech_info.mech_name, pwd_parm->user, pwd_parm->password );

		if ( pwd_parm->length < (sizeof( hdr ) + len) )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
				    mech_info.mech_name, pwd_parm->length,
				    (i4)sizeof( hdr ) + len );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_SYSTEM;
		hdr.obj_id = GCS_PWD_AUTH;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, len );
		MEcopy( (PTR)&hdr, sizeof( hdr ), pwd_parm->buffer );
		MEcopy( (PTR)pwd_parm->password, len,
			(PTR)((char *)pwd_parm->buffer + sizeof( hdr )) );
		pwd_parm->length = sizeof( hdr ) + len;
	    }
	    break;

	case GCS_OP_SRV_KEY :
	    {
		GCS_KEY_PARM	*key_parm = (GCS_KEY_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		char		obj[ (sizeof( hdr ) * 2) + 1 ];
		i4		key_len;

		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_SYSTEM;
		hdr.obj_id = GCS_SRV_KEY;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, 0 );

		CItotext( (u_char *)&hdr, sizeof( hdr ), obj );
		key_len = STlength( obj );

		if ( key_parm->length < (key_len + 1) )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
				    mech_info.mech_name, key_parm->length,
				    key_len + 1 );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		STcopy( obj, key_parm->buffer );
		key_parm->length = key_len;
	    }
	    break;

	case GCS_OP_SRV_AUTH :
	    {
		GCS_SRV_PARM	*srv_parm = (GCS_SRV_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		i4		len;
		u_char		obj[ (sizeof( hdr ) * 2) + 1];

		/*
		** GCA used the concept of trusted clients 
		** to authenticate servers.  Trusted clients 
		** run under the same user ID as the server 
		** or have been granted the TRUSTED privilege.  
		** Non-aliased connections are also allowed.
		*/
		if ( 
		     STcompare( srv_parm->user, srv_parm->alias )  &&
		     STcompare( srv_parm->user, IIgcs_global->user )  &&
		     gca_chk_priv( srv_parm->user, GCA_PRIV_TRUSTED ) != OK
		   )
		{
		    GCS_TRACE( 1 )( "GCS %s: user '%s' not trusted\n",
				    mech_info.mech_name, srv_parm->user );
		    status = E_GC1003_GCS_OP_UNSUPPORTED;
		    break;
		}

		/*
		** Validate server key.
		*/
		if ( STlength( srv_parm->key ) > sizeof( obj ) )
		{
		    GCS_TRACE( 1 )( "GCS %s: invalid key length (%d)\n",
				    mech_info.mech_name, 
				    STlength( srv_parm->key ) );
		    status = E_GC1011_INVALID_DATA_OBJ;
		    break;
		}

		CItobin( srv_parm->key, &len, obj );
		status = gcs_validate( len, (PTR)obj, &hdr );
		if ( status != OK )  break;

		/*
		** Generate server auth.
		*/
		len = STlength( srv_parm->server ) + 1;

		if ( srv_parm->length < sizeof( hdr ) + len )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
				    mech_info.mech_name, srv_parm->length,
				    (i4)sizeof( hdr ) + len );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_SYSTEM;
		hdr.obj_id = GCS_SRV_AUTH;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, len );
		MEcopy( (PTR)&hdr, sizeof( hdr ), srv_parm->buffer );
		MEcopy( (PTR)srv_parm->server, len, 
			(PTR)((char *)srv_parm->buffer + sizeof( hdr )) );
		srv_parm->length = sizeof( hdr ) + len;
	    }
	    break;

	case GCS_OP_RELEASE :
	    {
		GCS_REL_PARM	*rel = (GCS_REL_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		u_i2		len;

		status = gcs_validate( rel->length, rel->token, &hdr );
		if ( status != OK )  break;
		len = GCS_GET_I2( hdr.obj_len );

		GCS_TRACE( 4 )
		    ( "GCS %s: releasing %s (%d bytes)\n",
		      mech_info.mech_name, 
		      (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
						  hdr.obj_id ), (i4)len );

		switch( hdr.obj_id )
		{
		    case GCS_USR_AUTH :
		    case GCS_PWD_AUTH :
		    case GCS_SRV_AUTH :
			break;

		    default :
			GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
					mech_info.mech_name, (i4)hdr.obj_id );
			status = E_GC1011_INVALID_DATA_OBJ;
			break;
		}
	    }
	    break;

	case GCS_OP_SET :
	case GCS_OP_REM_AUTH :
	case GCS_OP_E_INIT :
	case GCS_OP_E_CONFIRM :
	case GCS_OP_E_ENCODE :
	case GCS_OP_E_DECODE :
	case GCS_OP_E_TERM :
	    GCS_TRACE( 1 )
		( "GCS %s: unsupported request\n", mech_info.mech_name );
	    status = E_GC1003_GCS_OP_UNSUPPORTED;
	    break;

	default :
	    GCS_TRACE( 1 )
		( "GCS %s: invalid request (%d)\n", mech_info.mech_name, op );
	    status = E_GC1002_GCS_OP_UNKNOWN;
	    break;
    }

    GCS_TRACE( 3 )
	( "GCS %s: %s status 0x%x\n", mech_info.mech_name, 
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_ops, op ), status );

    /*
    ** Clean up global info after term request and final tracing.
    */
    if ( IIgcs_global  &&  op == GCS_OP_TERM )  IIgcs_global = NULL;

    return( status );
}


/*
** Name: gcs_validate
**
** Description:
**	Validates the header information for a GCS object.
**
** Input:
**	length		Length of GCS object.
**	buffer		Location of GCS object.
**
** Output:
**	hdr		GCS object header.
**
** Returns
**	STATUS		OK or E_GC1011_INVALID_DATA_OBJ.
**
** History:
**	 5-Sep-97 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
*/

static STATUS
gcs_validate( i4  length, PTR buffer, GCS_OBJ_HDR *hdr )
{
    u_i4 id;
    u_i2 len;

    if ( length < sizeof( *hdr ) )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object length (%d of %d)\n",
			mech_info.mech_name, length, (i4)sizeof( *hdr ) );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    MEcopy( buffer, sizeof( *hdr ), (PTR)hdr );

    if ( (id = GCS_GET_I4( hdr->gcs_id )) != GCS_OBJ_ID )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid GCS id (0x%x)\n",
			mech_info.mech_name, id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    if ( hdr->mech_id != GCS_MECH_SYSTEM )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid mechanism ID (%d)\n",
			mech_info.mech_name, (i4)hdr->mech_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    if ( hdr->obj_ver != GCS_OBJ_V0  ||  hdr->obj_info != 0 )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object version (%d,%d)\n",
			mech_info.mech_name, 
			(i4)hdr->obj_ver, (i4)hdr->obj_info );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    len = GCS_GET_I2( hdr->obj_len );

    if ( length < (sizeof( *hdr ) + len) )
    {
	GCS_TRACE( 1 )( "GCS %s: insufficient data (%d of %d)\n",
			mech_info.mech_name, 
			length - sizeof( *hdr ), (i4)len );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    return( OK );
}


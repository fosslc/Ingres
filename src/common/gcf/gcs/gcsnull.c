/*
** Copyright (c) 2004 Ingres Corporation
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
** Name: gcsnull.c
**
** Description:
**	GCS security mechanism which effectively disables security
**	checking.  Provides user, password and server authentication
**	which will always validate successfully.  Remote operations
**	not supported.
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
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	21-Aug-98 (gordy)
**	    Changes for delegation (not supported) of authentication.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** GCS control block.
*/

static	GCS_GLOBAL	*IIgcs_global = NULL;


/*
** External mechanism info.
**
** Due to the inherent danger in providing this
** mechanism, it is disabled by default.  This
** mechanism must be explicitly enabled in the
** installation configuration file to be used.
*/

static	GCS_MECH_INFO	mech_info = 
{ 
    "null", 
    GCS_MECH_NULL, 
    GCS_MECH_VERSION_1,
    GCS_CAP_USR_AUTH | GCS_CAP_PWD_AUTH | GCS_CAP_SRV_AUTH, 
    GCS_DISABLED, GCS_ENC_LVL_0, 0 
};

static	GCS_MECH_INFO	*gcs_info[] = { &mech_info };


/*
** Forward references
*/

static STATUS		gcs_validate( i4, PTR, GCS_OBJ_HDR * );


/*
** Name: gcs_null
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
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	21-Aug-98 (gordy)
**	    Changes for delegation (not supported) of authentication.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
*/

STATUS
gcs_null( i4  op, PTR parms )
{
    STATUS		status = E_GC1000_GCS_FAILURE;

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
		    case GCS_PWD_AUTH :
		    case GCS_SRV_AUTH :
			/*
			** Validation always successful.
			*/
			status = OK;
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
		hdr.mech_id = GCS_MECH_NULL;
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

		if ( pwd_parm->length < sizeof( hdr ) )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n", 
			 	     mech_info.mech_name, pwd_parm->length, 
				     (i4)sizeof( hdr ) );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_NULL;
		hdr.obj_id = GCS_PWD_AUTH;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, 0 );
		MEcopy( (PTR)&hdr, sizeof( hdr ), pwd_parm->buffer );
		pwd_parm->length = sizeof( hdr );
	    }
	    break;

	case GCS_OP_SRV_KEY :
	    {
		GCS_KEY_PARM	*key_parm = (GCS_KEY_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		char		obj[ (sizeof( hdr ) * 2) + 1 ];
		i4		key_len;

		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_NULL;
		hdr.obj_id = GCS_SRV_KEY;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, 0 );

		CItotext( (u_char *)&hdr, sizeof( hdr ), (PTR)obj );
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
		if ( srv_parm->length < sizeof( hdr ) )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n", 
			 	     mech_info.mech_name, srv_parm->length, 
				     (i4)sizeof( hdr ) );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = OK;
		GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
		hdr.mech_id = GCS_MECH_NULL;
		hdr.obj_id = GCS_SRV_AUTH;
		hdr.obj_ver = GCS_OBJ_V0;
		hdr.obj_info = 0;
		GCS_PUT_I2( hdr.obj_len, 0 );
		MEcopy( (PTR)&hdr, sizeof( hdr ), srv_parm->buffer );
		srv_parm->length = sizeof( hdr );
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

    if ( hdr->mech_id != GCS_MECH_NULL )
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


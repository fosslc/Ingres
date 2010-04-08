/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <gc.h>
#include <me.h>
#include <mh.h>
#include <mu.h>
#include <st.h>
#include <tm.h>
#include <qu.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcs.h>
#include "gcsint.h"

/*
** Name: gcsing.c
**
** Description:
**	GCS security mechanism providing default Ingres security.
**
**	User auth: user ID of client process encoded with host name.
**
**	Password auth: password encoded with host name, validated
**	through O.S.
**
**	Server auth: Server ID encoded with host name, user alias
**	encoded with server key.
**
**	Installation Password: tickets provided and validated by
**	external entity (Name Server).
**
**	Encryption: a simple substitution and block shuffle algorithm
**	scrambles plain text.  Robust data encryption is not attempted.
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
**	 4-Dec-97 (rajus01)
**	    Installation passwords handling as remote authentications.
**	25-Feb-98 (gordy)
**	    Added support for encryption.
**	20-Mar-98 (gordy)
**	    Added mechanism encryption level.
**	 5-May-98 (gordy)
**	    User alias added GCS_OP_PWD_AUTH.  Added versioning to 
**	    GCS objects.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
**	26-May-98 (gordy)
**	    Adjust server key length so that resulting text encoding
**	    does not exceed 32 bytes (current GCN limit).
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
**	 5-May-03 (gordy)
**	    Privileged programs may change the effective user ID
**	    at run-time.  Pick up the current user ID on each
**	    call rather than using the cached initial user ID.
**	23-Jun-04 (gordy)
**	    Added expiration times to authentication objects
**	    to limit the ability to re-use an authentication.
**	 9-Jul-04 (gordy)
**	    IP Auth now distinct operation (bump mech API version).
**	14-Sep-04 (gordy)
**	    Used correct error status for corrupted data objects.  Validate 
**	    expirations after validation of data on which they are dependent.
**	20-Oct-05 (gordy)
**	    Change char buffers/pointers to u_i1 except where explicitly
**	    and strictly character oriented (extracting u_i1 via a signed
**	    char pointer mis-interprets the high order bit).  Ensure
**	    buffers are large enough to hold encoded values (current
**	    encoding scheme can double string lengths: verify input
**	    lengths and declare appropriate buffer sizes).
**	 6-Dec-05 (gordy)
**	    Differentiate conditions causing server validation failure.
**	 15-Dec-2005 (drivi01)
**	    Newly included gcnint.h in this file uses QUEUE type
**	    defined in qucl.h which isn't included anywhere.
**	    Include qu.h to avoid compile errors.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/


#define	GCS_CNF_MECH_EXP_REQ	"%s.%s.gcf.mech.%s.expirations_required"
#define	GCS_CNF_MECH_EXP_TIME	"%s.%s.gcf.mech.%s.expiration_time"

#define	CRYPT_SIZE		8
#define	CRYPT_DELIM		'*'

/*
** The equation for the encoded length of a string is
** complicated due to various 'salt' values and padding.
** The following is a simplification which over-estimates
** the size.  The additional 8 bytes are needed for short
** strings where the padding of the last block can be a
** significant part of the length.  The factor of 2 is
** excessive for long strings where 1.6 is closer to
** reality.
*/
#define	GCS_ENC_SIZE( len )	(((len)+8)*2)

/*
** The following are default sizes for declared strings
** and buffers.  They should not be used as max limits.
** Dynamic storage should be used when actual lengths
** exceed these sizes.
*/
#define	GCS_MAX_STR		127
#define	GCS_MAX_BUF		255

#define	GCS_EXP_LEN		32	/* Buffer length for expiration time */


/*
** Expiration time
*/

#define	GCS_EXP_ID	0x45585054		/* 'EXPT' (ascii big-endian) */

typedef struct
{
    u_i1	id[4];
    u_i1	secs[4];
} GCS_EXP;

static	bool	expirations_required = FALSE;	/* Expirations required? */
static	i4	expiration_time = 10;		/* Expiration time (seconds) */


/*
** Server key object.
*/

#define	SRV_KEY_LEN	CRYPT_SIZE

typedef struct 
{

    GCS_OBJ_HDR	hdr;
    char	key[ SRV_KEY_LEN + 1 ];

} GCS_KEY;


/*
** Encryption character set.
*/

static	char	*cset = 
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


/*
** Encryption Session Control Block
**
** The 'ingres' mechanism provides a minimal form of encryption which
** is not expected to be secure but which ensures that plain text does
** not cross the network.  To provide for future extensions, different
** encryption algorithm types can be specified.  The actual algorithm
** used is negotiated during init/confirm operations.
**
** Type 0:
**	No encryption.  Not actually used; provided for testing and
**	backward compatibility.
**
** Type 1:
**	Simple substitution and block shuffle.
*/

#define	ENC_KEY_LEN	(CRYPT_SIZE * 2)
#define	ENC_BLOCK_SIZE	32
#define	ENC_OVHD	(ENC_BLOCK_SIZE + sizeof( GCS_OBJ_HDR ))

typedef struct
{

    u_i1	type;			/* Encryption algorith type */

#define	ETYPE_0		GCS_OBJ_V0	/* No encryption */
#define ETYPE_1		GCS_OBJ_V1	/* Substitution & block shuffle */
#define ETYPE_MAX	ETYPE_1		/* What is currently support */

    union
    {
	struct				/* Type 1 information */
	{
	    u_i4	enc_key;		/* Encoding key */
	    u_i4	dec_key;		/* Decoding key */
	    u_i1	enc_arr[ 256 ];		/* Encoding substitution */
	    u_i1	dec_arr[ 256 ];		/* Decoding substitution */
	    u_i1	pos_arr[ ENC_BLOCK_SIZE ];	/* Block shuffle */
	} etype_1;
    } info;

} ESCB;

/*
** Pseudo-random number generator used by Type 1 encryption.
** We need to generate the same random number stream on both
** ends of the connection.  The following generator provides
** adaquate randomness characteristics and will not overflow
** a 32 bit unsigned integer.
*/

#define	RAND_MUL	4096L
#define	RAND_INC	150889L
#define	RAND_MOD	714025L

#define	SRAND( seed )	(seed % RAND_MOD)
#define	RAND( seed )	((seed * RAND_MUL + RAND_INC) % RAND_MOD)

/*
** GCS control block.
*/

static	GCS_GLOBAL	*IIgcs_global = NULL;


/*
** External mechanism info.
*/

static	GCS_MECH_INFO	mech_info =
{ 
    "ingres", 
    GCS_MECH_INGRES, 
    GCS_MECH_VERSION_2,
    GCS_CAP_USR_AUTH | GCS_CAP_PWD_AUTH | GCS_CAP_SRV_AUTH |
			GCS_CAP_IP_AUTH | GCS_CAP_ENCRYPT, 
    GCS_AVAIL,  GCS_ENC_LVL_1, ENC_OVHD
}; 

static	GCS_MECH_INFO	*gcs_info[] = { &mech_info };


/*
** Forward references.
*/

static	STATUS	gcs_usr_auth( GCS_USR_PARM *usr_parm );
static	STATUS	gcs_val_usr( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_pwd_auth( GCS_PWD_PARM *pwd_parm );
static	STATUS	gcs_val_pwd( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_srv_key( GCS_KEY_PARM *key_parm );
static	STATUS	gcs_srv_auth( GCS_SRV_PARM *srv_parm );
static	STATUS	gcs_val_srv( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_rem_auth( GCS_REM_PARM *rem_parm );
static	STATUS	gcs_val_rem( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_e_init( GCS_EINIT_PARM *init, ESCB *escb );
static	STATUS	gcs_e_ipeer( GCS_EINIT_PARM *, GCS_OBJ_HDR *, ESCB * );
static	STATUS	gcs_e_confirm( GCS_ECONF_PARM *conf );
static	STATUS	gcs_e_cpeer( GCS_ECONF_PARM *conf, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_e_encode( GCS_EENC_PARM *enc );
static	STATUS	gcs_e_decode( GCS_EDEC_PARM *dec, GCS_OBJ_HDR *hdr );
static	STATUS	gcs_validate( i4  len, PTR obj, GCS_OBJ_HDR *hdr );
static	i4	gcs_gen_exp( i4 limit, char *key, char *buff );
static	STATUS	gcs_chk_exp( char *estr, char *key );
static	i4	gcs_encode( char *str, char *key, char *buff );
static	i4	gcs_decode( char *str, char *key, char *buff );
static	VOID	gcs_e1_init( ESCB *escb );
static	VOID	gcs_e1_encode( ESCB *escb, i4  len, u_i1 *buff );
static	VOID	gcs_e1_decode( ESCB *escb, i4  len, u_i1 *buff );




/*
** Name: gcs_ingres
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
**	25-Feb-98 (gordy)
**	    Added support for encryption.
**	21-Aug-98 (gordy)
**	    Changes for delegation (not supported) of authentication.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	 6-Jul-99 (rajus01)
**	    Map GC CL errors into GCA errors as done in GCA.
**	23-Jun-04 (gordy)
**	    Load config info for expiration times.
**	 9-Jul-04 (gordy)
**	    IP auth is now distinct operation.
**	 6-Dec-05 (gordy)
**	    Return E_GC0146 if IP not supported by server.
**	21-Jul-09 (gordy)
**	    Ensure space for config string including host name.
*/

STATUS
gcs_ingres( i4  op, PTR parms )
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
	{
	    char	*p;
	    char	str[ GCS_MAX_STR + GC_HOSTNAME_MAX + 1 ];

	    MHsrand( TMsecs() );

	    STprintf( str, GCS_CNF_MECH_EXP_REQ, SystemCfgPrefix,
		      IIgcs_global->config_host, mech_info.mech_name );

	    if ( (*IIgcs_global->config_func)( str, &p ) == OK )
		expirations_required = (*p == 'Y' ||  *p == 'y');

	    GCS_TRACE(4)( "GCS %s: expirations required: %s\n",
			  mech_info.mech_name, 
			  expirations_required ? "Yes" : "No" );

	    STprintf( str, GCS_CNF_MECH_EXP_TIME, SystemCfgPrefix,
		      IIgcs_global->config_host, mech_info.mech_name );

	    if ( (*IIgcs_global->config_func)( str, &p ) == OK )
		CVal( p, &expiration_time );

	    GCS_TRACE(4)( "GCS %s: expiration time %d seconds\n",
			  mech_info.mech_name, expiration_time );

	    status = OK;
	    break;
	}

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
			status = gcs_val_usr( valid, &hdr );
			break;
		    
		    case GCS_PWD_AUTH :
			status = gcs_val_pwd( valid, &hdr );
			break;

		    case GCS_SRV_AUTH :
			if ( ! IIgcs_global->get_key_func )
			{
			    GCS_TRACE( 1 )( "GCS %s: no server key callback\n",
					    mech_info.mech_name );
			    status = E_GC1009_INVALID_SERVER;
			    break;
			}

			status = gcs_val_srv( valid, &hdr );
			break;

		    case GCS_REM_AUTH :
			if ( ! IIgcs_global->ip_func )
			{
			    GCS_TRACE( 1 )
				( "GCS %s: unable to validate ticket\n",
				  mech_info.mech_name );
			    status = E_GC0146_GCN_INPW_SRV_NOSUPP;
			    break;
			}

			status = gcs_val_rem( valid, &hdr );
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
	    status = gcs_usr_auth( (GCS_USR_PARM *)parms );
	    break;

	case GCS_OP_PWD_AUTH :
	    status = gcs_pwd_auth( (GCS_PWD_PARM *)parms );
	    break;

	case GCS_OP_SRV_KEY :
	    status = gcs_srv_key( (GCS_KEY_PARM *)parms );
	    break;

	case GCS_OP_SRV_AUTH :
	    status = gcs_srv_auth( (GCS_SRV_PARM *)parms );
	    break;

	case GCS_OP_IP_AUTH :
	    {
		GCS_REM_PARM	*rem_parm = (GCS_REM_PARM *)parms;

		if (rem_parm->size < (sizeof(GCS_OBJ_HDR) + rem_parm->length))
		{
		    GCS_TRACE( 1 )
			( "GCS %s: insufficient buffer (%d of %d)\n",
			  mech_info.mech_name, rem_parm->size,
			  (i4)sizeof( GCS_OBJ_HDR ) + rem_parm->length );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = gcs_rem_auth( rem_parm );
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
		    case GCS_REM_AUTH :
			break;

		    default :
			GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
					mech_info.mech_name, (i4)hdr.obj_id );
			status = E_GC1011_INVALID_DATA_OBJ;
			break;
		}
	    }
	    break;

	case GCS_OP_E_INIT :
	    {
		GCS_EINIT_PARM	*init = (GCS_EINIT_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		ESCB		*escb;

		/*
		** Allocate the encryption session control block.
		*/
		escb = (ESCB *)(*IIgcs_global->mem_alloc_func)( sizeof(ESCB) );
		if ( ! escb )
		{
		    status = E_GC1013_NO_MEMORY;
		    break;
		}

		if ( init->initiator )
		    status = gcs_e_init( init, escb );
		else  if ((status = gcs_validate( init->length, 
						  init->buffer, &hdr )) == OK)
		    status = gcs_e_ipeer( init, &hdr, escb );

		if ( status == OK )
		    init->escb = (PTR)escb;
		else
		   (*IIgcs_global->mem_free_func)( (PTR)escb );
	    }
	    break;

	case GCS_OP_E_CONFIRM :
	    {
		GCS_ECONF_PARM	*conf = (GCS_ECONF_PARM *)parms;
		GCS_OBJ_HDR	hdr;

		if ( ! conf->initiator )
		    status = gcs_e_confirm( conf );
		else  if ((status = gcs_validate( conf->length, 
						  conf->buffer, &hdr )) == OK)
		    status = gcs_e_cpeer( conf, &hdr );
	    }
	    break;

	case GCS_OP_E_ENCODE :
	    {
		GCS_EENC_PARM	*enc = (GCS_EENC_PARM *)parms;

		if ( (enc->size - enc->length) < ENC_OVHD )
		{
		    GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
				    mech_info.mech_name, 
				    enc->size - enc->length, ENC_OVHD );
		    status = E_GC1010_INSUFFICIENT_BUFFER;
		    break;
		}

		status = gcs_e_encode( enc );
	    }
	    break;

	case GCS_OP_E_DECODE :
	    {
		GCS_EDEC_PARM	*dec = (GCS_EDEC_PARM *)parms;
		GCS_OBJ_HDR	hdr;

		if ( (status = gcs_validate( dec->length, 
					     dec->buffer, &hdr )) == OK )
		    status = gcs_e_decode( dec, &hdr );
	    }
	    break;

	case GCS_OP_E_TERM :
	    {
		GCS_ETERM_PARM	*term = (GCS_ETERM_PARM *)parms;

		(*IIgcs_global->mem_free_func)( term->escb );
		status = OK;
	    }
	    break;

	case GCS_OP_REM_AUTH :
	case GCS_OP_SET :
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
** Name: gcs_usr_auth
**
** Description:
**	Creates a GCS_USR_AUTH object.
**
**	A version 0 object contains just the user ID (encrypted
**	with the host name).
**
**	A version 1 object contains the user ID (encrypted with
**	the host name) and an expiration time (encrypted with the
**	user ID).
**
** Input:
**	usr_parm	GCS_OP_USR_AUTH parameters.
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
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	 5-May-03 (gordy)
**	    Privileged programs may change the effective user ID
**	    at run-time.  Pick up the current user ID on each
**	    call rather than using the cached initial user ID.
**	23-Jun-04 (gordy)
**	    Added expiration time.
**	 9-Jul-04 (gordy)
**	    Check global version to access user function.
**	21-Jul-09 (gordy)
**	    Remove username length restrictions.  User more appropriate 
**	    sizes for string buffers.  Use dynamic storage if length 
**	    exceeds default declared buffer lengths.
*/

static STATUS
gcs_usr_auth( GCS_USR_PARM *usr_parm )
{
    GCS_OBJ_HDR	hdr;
    char	user[ GC_USERNAME_MAX + 1 ];
    char	ubuff[ GCS_MAX_BUF + 1 ];
    char	ebuff[ GCS_EXP_LEN + 1 ];
    char	*uptr;
    u_i1	*ptr = (u_i1 *)usr_parm->buffer;
    u_i2	ulen, elen;
    i4		len;
    STATUS	status = OK;

    if ( IIgcs_global->version >= GCS_CB_VERSION_2 )
	(*IIgcs_global->usr_func)( user, sizeof( user ) );
    else
	STcopy( IIgcs_global->user, user );

    GCS_TRACE( 4 )
	( "GCS %s: host %s, user %s, expire %d\n", mech_info.mech_name, 
	  IIgcs_global->host, user, expiration_time );

    /*
    ** Ensure encoded user buffer is large enough.
    */
    ulen = GCS_ENC_SIZE( STlength( user ) );

    uptr = (ulen < sizeof( ubuff ))
    	   ? ubuff : (char *)(*IIgcs_global->mem_alloc_func)( ulen + 1 );

    if ( ! uptr )  return( E_GC1013_NO_MEMORY );

    /*
    ** Encode components.
    */
    if ( (len = gcs_encode( user, IIgcs_global->host, uptr )) < 0 )
    {
	status = E_GC1013_NO_MEMORY;
    	goto done;
    }

    ulen = (u_i2)len + 1;

    if ( (len = gcs_gen_exp( expiration_time, user, ebuff )) < 0 )
    {
	status = E_GC1013_NO_MEMORY;
    	goto done;
    }

    elen = (u_i2)len + 1;

    if ( usr_parm->length < (sizeof( hdr ) + ulen + elen + 2) )
    {
	GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
			mech_info.mech_name, usr_parm->length,
			(i4)sizeof( hdr ) + ulen + elen + 2 );

	status = E_GC1010_INSUFFICIENT_BUFFER;
    	goto done;
    }

    /*
    ** Build the authentication object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );		/* Header */
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id = GCS_USR_AUTH;
    hdr.obj_ver = GCS_OBJ_V1;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, ulen + elen + 2 );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    *ptr++ = ulen;					/* User ID */
    MEcopy( (PTR)uptr, ulen, (PTR)ptr );
    ptr += ulen;

    *ptr++ = elen;					/* Expiration time */
    MEcopy( (PTR)ebuff, elen, (PTR)ptr );

    usr_parm->length = sizeof( hdr ) + ulen + elen + 2;

  done:

    if ( uptr != ubuff )  (*IIgcs_global->mem_free_func)( (PTR)uptr );
    return( status );
}


/*
** Name: gcs_val_usr
**
** Description:
**	Validates a GCS_USR_AUTH object.  Client user ID must 
**	match validation user ID and alias.
**
** Input:
**	valid		GCS_OP_VALIDATE parameters.
**	hdr		User authentication object header.
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
**	23-Jun-04 (gordy)
**	    Added expiration time.  Object header now provided as parameter.
**	14-Sep-04 (gordy)
**	    Decoding of expiration time dependent on user ID, so validate
**	    after user ID validation.  Also, fixed various error codes.
**	21-Jul-09 (gordy)
**	    Remove username length restrictions.  Use dynamic storage if 
**	    username exceeds default declared buffer length.
*/

static STATUS
gcs_val_usr( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr )
{
    char	ubuff[ GCS_MAX_BUF + 1 ];
    char	*user;
    u_i1	*uptr, *eptr;
    u_i2	obj_len = GCS_GET_I2( hdr->obj_len );
    u_i2	ulen = obj_len;
    u_i2	elen = 0;
    STATUS	status = OK;

    GCS_TRACE( 4 )( "GCS %s: user '%s', alias '%s'\n",
		    mech_info.mech_name, valid->user, valid->alias );

    /*
    ** Extract authentication compenents.
    */
    uptr = (u_i1 *)valid->auth + sizeof( GCS_OBJ_HDR );

    if ( hdr->obj_ver >= GCS_OBJ_V1 )
	if ( (ulen = *uptr++) < obj_len )
	{
	    eptr = uptr + ulen;
	    elen = *eptr++;
	}

    /*
    ** Validate user ID.  Make sure object is formatted correctly.
    */
    if ( 
	 obj_len != (ulen + (hdr->obj_ver >= GCS_OBJ_V1 ? elen + 2 : 0))  ||
         *(uptr + ulen - 1) != EOS 
       )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid user ID object: %d, %d, %d\n", 
			mech_info.mech_name, obj_len, ulen, elen );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    /*
    ** Ensure username buffer is large enough.
    */
    user = (ulen < sizeof( ubuff ))
    	   ? ubuff : (char *)(*IIgcs_global->mem_alloc_func)( ulen + 1 );

    if ( ! user )  return( E_GC1013_NO_MEMORY );
    gcs_decode( (char *)uptr, IIgcs_global->host, user );

    if ( STcompare( user, valid->user ) )
    {
	GCS_TRACE( 1 )( "GCS %s: user '%s' does not match auth: '%s'\n",
		        mech_info.mech_name, valid->user, user );
	status = E_GC1008_INVALID_USER;
    	goto done;
    }

    if ( STcompare( user, valid->alias ) )
    {
	GCS_TRACE( 1 )( "GCS %s: alias '%s' does not match auth: '%s'\n",
			mech_info.mech_name, valid->alias, user );
	status = E_GC1008_INVALID_USER;
    	goto done;
    }

    /*
    ** Validate expiration time.
    */
    if ( elen )
    {
	if ( elen > GCS_EXP_LEN  ||  *(eptr + elen - 1) != EOS )
	{
	    GCS_TRACE( 1 )( "GCS %s: invalid expiration length: %d\n", 
			    mech_info.mech_name, elen );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    goto done;
	}

	if ( (status = gcs_chk_exp( (char *)eptr, user )) != OK )
	    goto done;
    }
    else  if ( expirations_required )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid GCS user object version!\n", 
			mech_info.mech_name );
	status = E_GC100B_NO_EXPIRATION;
    	goto done;
    }

  done:

    if ( user != ubuff )  (*IIgcs_global->mem_free_func)( (PTR)user );
    return( status );
}


/*
** Name: gcs_pwd_auth
**
** Description:
**	Creates a GCS_PWD_AUTH object. 
**
**	A version 0 object contains the password (encrypted 
**	with the host name).
**
**	A version 1 object contains the password (encrypted
**	with the user alias and host name).
**
**	A version 2 object contains the password (encrypted
**	with the user alias and host name) and an expiration
**	time (encrypted with the user alias).
**
** Input:
**	pwd_parm	GCS_OP_PWD_AUTH parameters.
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
**	 5-May-98 (gordy)
**	    User alias added GCS_OP_PWD_AUTH.  Added versioning to 
**	    GCS objects.
**	23-Jun-04 (gordy)
**	    Added expiration time.
**      28-Mar-08 (rajus01) SD issue 126704, Bug 120136
**	    JDBC applications report E_GC1010 error when either password
**	    or username specified in the JDBC URL is longer but less than the
**	    supported limit (GC_L_PASSWORD, GC_L_USER_ID). Also when user
**	    and/or password length exceed the supported lengths
**	    GCN crashes and E_CLFE07 is returned to the application.
**	    Replaced E_GC1010 with E_GC1026 error code to provide a more
**	    meaningful error message.
**	21-Jul-09 (gordy)
**	    Remove password length restrictions.  Use dynamic storage
**	    if length exceeds default declared buffer size.
*/

static STATUS
gcs_pwd_auth( GCS_PWD_PARM *pwd_parm )
{
    GCS_OBJ_HDR	hdr;
    char	pbuff[ GCS_MAX_BUF + 1 ];
    char	ebuff[ GCS_EXP_LEN + 1 ];
    char	*pwd;
    u_i1	*ptr = (u_i1 *)pwd_parm->buffer;
    u_i1	version = GCS_OBJ_V0;
    u_i2	plen, elen = 0;
    i4		len;
    STATUS	status = OK;

    GCS_TRACE( 4 )( "GCS %s: host %s, alias %s, password %s, expire %d\n", 
		    mech_info.mech_name, IIgcs_global->host, 
		    pwd_parm->user, pwd_parm->password, expiration_time );

    /*
    ** Encode components.
    */
    if ( ! pwd_parm->user  ||  ! *pwd_parm->user )
    {
	plen = GCS_ENC_SIZE( STlength( pwd_parm->password ) );

	pwd = (plen < sizeof( pbuff ))
    	      ? pbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! pwd )  return( E_GC1013_NO_MEMORY );

	if ( (len = gcs_encode( pwd_parm->password, 
				IIgcs_global->host, pwd )) < 0 )
	{
	    status = E_GC1013_NO_MEMORY;
	    goto done;
	}

        plen = (u_i2)len + 1;
    }
    else
    {
	char	tbuff[ GCS_MAX_BUF + 1 ];
	char	*tmp;

	version = GCS_OBJ_V2;
	plen = GCS_ENC_SIZE( STlength( pwd_parm->password ) );

	tmp = (plen < sizeof( tbuff ))
    	      ? tbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! tmp )  return( E_GC1013_NO_MEMORY );

	if ( (len = gcs_encode(pwd_parm->password, pwd_parm->user, tmp)) < 0 )
	{
	    if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
	    return( E_GC1013_NO_MEMORY );
	}

	plen = GCS_ENC_SIZE( (u_i2)len + 1 );

	pwd = (plen < sizeof( pbuff ))
    	      ? pbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! pwd )
	{
	    if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
	    return( E_GC1013_NO_MEMORY );
	}

	if ( (len = gcs_encode( tmp, IIgcs_global->host, pwd )) < 0 )
	{
	    if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
	    status = E_GC1013_NO_MEMORY;
	    goto done;
	}

	plen = (u_i2)len + 1;

	len = gcs_gen_exp( expiration_time, pwd_parm->user, ebuff );
	if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );

        if ( len < 0 )
	{
	    status = E_GC1013_NO_MEMORY;
	    goto done;
	}

        elen = (u_i2)len + 1;
    }

    if ( pwd_parm->length < (sizeof( hdr ) + plen + elen + 2) )
    {
	GCS_TRACE( 1 )( "GCS %s: insufficient buffer: %d (need %d)\n",
			mech_info.mech_name, pwd_parm->length,
			(i4)sizeof( hdr ) + plen + elen + 2 );
	status = E_GC1010_INSUFFICIENT_BUFFER;
    	goto done;
    }

    /*
    ** Build the authentication object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );		/* Header */
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id = GCS_PWD_AUTH;
    hdr.obj_ver = version;
    hdr.obj_info = 0;
    GCS_PUT_I2(hdr.obj_len, (version == GCS_OBJ_V0) ? plen : plen + elen + 2);
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    if ( version == GCS_OBJ_V0 )
    {
	MEcopy( (PTR)pwd, plen, (PTR)ptr );		/* Password */
	pwd_parm->length = sizeof( hdr ) + plen;
    }
    else
    {
	*ptr++ = plen;					/* Password */
	MEcopy( (PTR)pwd, plen, (PTR)ptr );
	ptr += plen;

	*ptr++ = elen;					/* Expiration time */
	MEcopy( (PTR)ebuff, elen, (PTR)ptr );

	pwd_parm->length = sizeof( hdr ) + plen + elen + 2;
    }

  done:

    if ( pwd != pbuff )  (*IIgcs_global->mem_free_func)( (PTR)pwd );
    return( status );
}


/*
** Name: gcs_val_pwd
**
** Description:
**	Validates a GCS_PWD_AUTH object.  The password is validated 
**	using the GCS password validation callback function (if 
**	specified).  If no callback function has been specified or 
**	the callback function indicates that the password has not 
**	been validated, the password will be validated through the 
**	Operating System using GCusrpwd().
**
** Input:
**	valid		GCS_OP_VALIDATE parameters.
**	hdr		Password authentication object header.
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
**	 5-May-98 (gordy)
**	    User alias added GCS_OP_PWD_AUTH.
**	23-Jun-04 (gordy)
**	    Added expiration time.
**	14-Sep-04 (gordy)
**	    Decoding of expiration time dependent on user alias, so validate
**	    after password validation.  Also, fixed various error codes.
**	 6-Dec-05 (gordy)
**	    Return E_GC0146 if IP not supported by server.
**	21-Jul-09 (gordy)
**	    Remove password length restrictions.  Use dynamic storage
**	    if length exceeds default declared buffer size.
*/

static STATUS
gcs_val_pwd( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr )
{
    GCS_USR_PWD_PARM	parm;
    CL_ERR_DESC		sys_err;
    char		pbuff[ GCS_MAX_STR + 1 ];
    char		*pwd;
    u_i1		*pptr, *eptr;
    u_i2		obj_len = GCS_GET_I2( hdr->obj_len );
    u_i2		plen = obj_len;
    u_i2		elen = 0;
    STATUS		status = OK;

    GCS_TRACE( 4 )( "GCS %s: alias '%s'\n",
		    mech_info.mech_name, valid->alias );

    /*
    ** Extract components.
    */
    pptr = (u_i1 *)valid->auth + sizeof( GCS_OBJ_HDR );

    if ( hdr->obj_ver >= GCS_OBJ_V2 )
	if ( (plen = *pptr++) < obj_len )
	{
	    eptr = pptr + plen;
	    elen = *eptr++;
	}

    /*
    ** Validate password.  Make sure object is formatted correctly.
    */
    if ( 
         obj_len != (plen + (hdr->obj_ver >= GCS_OBJ_V2 ? elen + 2 : 0))  ||
	 *(pptr + plen - 1) != EOS 
       )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid password length: %d, %d, %d\n", 
			mech_info.mech_name, hdr->obj_len, plen, elen );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    if ( hdr->obj_ver == GCS_OBJ_V0 )
    {
	pwd = (plen < sizeof( pbuff ))
	       ? pbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! pwd )  return( E_GC1013_NO_MEMORY );
	gcs_decode( (char *)pptr, IIgcs_global->host, pwd );
    }
    else
    {
	char	tbuff[ GCS_MAX_STR + 1 ];
	char	*tmp;

	tmp = (plen < sizeof( tbuff ))
	       ? tbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! tmp )  return( E_GC1013_NO_MEMORY );
	plen = gcs_decode( (char *)pptr, IIgcs_global->host, tmp );

	pwd = (plen < sizeof( pbuff ))
	      ? pbuff : (char *)(*IIgcs_global->mem_alloc_func)( plen + 1 );

	if ( ! pwd )
	{
	    if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
	    return( E_GC1013_NO_MEMORY );
	}

	gcs_decode( tmp, valid->alias, pwd );
	if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
    }

    if ( ! IIgcs_global->usr_pwd_func )
	status = FAIL;		/* Call GCusrpwd() */
    else
    {
	parm.user = valid->alias;
	parm.password = pwd;
	status = (*IIgcs_global->usr_pwd_func)( &parm );
    }

    if ( status == FAIL )
	status = GCusrpwd( valid->alias, pwd, &sys_err );

    if ( ( status & ~0xFF ) == ( E_CL_MASK | E_GC_MASK ) )
	status ^= ( E_CL_MASK | E_GC_MASK ) ^ E_GCF_MASK;

    if ( status != OK )
    {
	/*
	** Only the Name Server can validate installation
	** passwords.  Other servers will treat IPs as
	** regular passwords and generate an invalid
	** password error.  Return a better error in this
	** case.
	**
	** This is a bit of a kludge since the internal
	** installation password format should only be
	** known to the Name Server.  
	*/
	if ( status == E_GC000B_RMT_LOGIN_FAIL  &&
	     ! MEcmp( pwd, GCN_AUTH_TICKET_MAG, GCN_L_AUTH_TICKET_MAG ) )
	{
	    GCS_TRACE( 1 )( "GCS %s: IP couldn't be validated by server\n", 
			    mech_info.mech_name );
	    status = E_GC0146_GCN_INPW_SRV_NOSUPP;
	}
	else
	{
	    GCS_TRACE( 1 )( "GCS %s: invalid password: '%s'\n", 
			    mech_info.mech_name, pwd );
	}

	goto done;
    }

    /*
    ** Validate expiration time.
    */
    if ( elen )
    {
	if ( elen > GCS_EXP_LEN  ||  *(eptr + elen - 1) != EOS )
	{
	    GCS_TRACE( 1 )( "GCS %s: invalid expiration length: %d\n", 
			    mech_info.mech_name, elen );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    goto done;
	}

	if ( (status = gcs_chk_exp( (char *)eptr, valid->alias )) != OK )
	    goto done;
    }
    else  if ( expirations_required )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid GCS password object version!\n", 
			mech_info.mech_name );
	status = E_GC100B_NO_EXPIRATION;
    	goto done;
    }

  done:

    if ( pwd != pbuff )  (*IIgcs_global->mem_free_func)( (PTR)pwd );
    return( status );
}


/*
** Name: gcs_srv_key
**
** Description:
**	Creates a GCS_SRV_KEY object by generating a random string
**	of alphanumeric characters.  The entire binary object is then
**	encoded in text format as a server key.
**
** Input:
**	key_parm	GCS_OP_SRV_KEY parameters.
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
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	21-Jul-09 (gordy)
**	    Size buffer based on size of object it will hold.
*/

static STATUS
gcs_srv_key( GCS_KEY_PARM *key_parm )
{
    char	buff[ (sizeof( GCS_KEY ) * 2) + 1 ];
    i4		i, key_len, csize = STlength( cset );
    GCS_KEY	obj;

    for( i = 0; i < SRV_KEY_LEN; i++ )
	obj.key[i] = cset[ (i4)(MHrand() * csize) % csize ];

    obj.key[ SRV_KEY_LEN ] = EOS;
    GCS_TRACE( 4 )( "GCS %s: server key %s\n", mech_info.mech_name, obj.key );

    GCS_PUT_I4( obj.hdr.gcs_id, GCS_OBJ_ID );
    obj.hdr.mech_id = GCS_MECH_INGRES;
    obj.hdr.obj_id = GCS_SRV_KEY;
    obj.hdr.obj_ver = GCS_OBJ_V0;
    obj.hdr.obj_info = 0;
    GCS_PUT_I2( obj.hdr.obj_len, SRV_KEY_LEN + 1 );
    CItotext( (u_char *)&obj, sizeof( obj ), (PTR)buff );
    key_len = STlength( buff );

    if ( key_parm->length < (key_len + 1) )
    {
	GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
			mech_info.mech_name, key_parm->length, key_len + 1 );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    STcopy( buff, key_parm->buffer );
    key_parm->length = key_len;

    return( OK );
}


/*
** Name: gcs_srv_auth
**
** Description:
**	Creates a GCS_SRV_AUTH object.
**
**	A version 0 object contains the server ID (encrypted
**	with the host name) and the user alias (encrypted with
**	the server key).
**
**	A version 1 object contains the server ID (encrypted
**	with the host name), the user alias (encrypted with
**	the server key) and an expiration time (encrypted with 
**	the user ID).
**
** Input:
**	srv_parm	GCS_OP_SRV_AUTH parameters.
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
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	23-Jun-04 (gordy)
**	    Added expiration time.
**	14-Sep-04 (gordy)
**	    Use user alias encoded in GCS object to encode expiration time.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.  Use dynamic storage if
**	    lengths exceed default declared buffer sizes.
*/

static STATUS
gcs_srv_auth( GCS_SRV_PARM *srv_parm )
{
    GCS_OBJ_HDR	hdr;
    u_i1	*ptr = (u_i1 *)srv_parm->buffer;
    u_i1	sbuff[ GCS_MAX_BUF + 1 ];
    u_i1	ubuff[ GCS_MAX_BUF + 1 ];
    u_i1	ebuff[ GCS_EXP_LEN + 1 ];
    u_i1	*sptr, *uptr;
    u_i2	slen, ulen, elen;
    i4		len;
    STATUS	status = OK;

    union
    {
	GCS_KEY	key;
	u_char	obj[ (sizeof( GCS_KEY ) * 2) + 1 ];
    } obj;

    /*
    ** Validate server key.
    */
    if ( STlength( srv_parm->key ) >= sizeof( obj.obj ) )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid key length (%d)\n", 
			mech_info.mech_name, STlength( srv_parm->key ) );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    CItobin( (PTR)srv_parm->key, &len, obj.obj );
    status = gcs_validate( len, (PTR)&obj.key, &hdr );
    if ( status != OK )  return( status );

    GCS_TRACE( 4 )( "GCS %s: key %s, user %s, alias %s,\n\
 host %s, server %s, expire %d\n",
		    mech_info.mech_name, obj.key.key, srv_parm->user,
		    srv_parm->alias, IIgcs_global->host, srv_parm->server,
		    expiration_time );

    /*
    ** Ensure buffers are large enough.
    */
    slen = GCS_ENC_SIZE( STlength( srv_parm->server ) );

    sptr = (slen < sizeof( sbuff ))
    	   ? sbuff : (u_i1 *)(*IIgcs_global->mem_alloc_func)( slen + 1 );

    if ( ! sptr )  return( E_GC1013_NO_MEMORY );
    ulen = GCS_ENC_SIZE( STlength( srv_parm->alias ) );

    uptr = (ulen < sizeof( ubuff ))
    	   ? ubuff : (u_i1 *)(*IIgcs_global->mem_alloc_func)( ulen + 1 );

    if ( ! uptr )
    {
	if ( sptr != sbuff )  (*IIgcs_global->mem_free_func)( (PTR)sptr );
    	return( E_GC1013_NO_MEMORY );
    }

    /*
    ** Encode components.
    */
    if ( (len = gcs_encode( srv_parm->server, IIgcs_global->host, sptr )) < 0 )
    {
	status = E_GC1013_NO_MEMORY;
    	goto done;
    }

    slen = (u_i2)len + 1;

    if ( (len = gcs_encode( srv_parm->alias, obj.key.key, uptr )) < 0 )
    {
	status = E_GC1013_NO_MEMORY;
    	goto done;
    }

    ulen = (u_i2)len + 1;

    if ( (len = gcs_gen_exp( expiration_time, srv_parm->alias, ebuff )) < 0 )
    {
	status = E_GC1013_NO_MEMORY;
    	goto done;
    }

    elen = (u_i2)len + 1;

    if ( srv_parm->length < (sizeof(hdr) + slen + ulen + elen + 3) )
    {
	GCS_TRACE( 1 )( "GCS %s: insufficient buffer (%d of %d)\n",
			mech_info.mech_name, srv_parm->length,
			(i4)sizeof( hdr ) + slen + ulen + elen + 3 );
	status = E_GC1010_INSUFFICIENT_BUFFER;
    	goto done;
    }

    /*
    ** Build authentication object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );		/* Header */
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id = GCS_SRV_AUTH;
    hdr.obj_ver = GCS_OBJ_V1;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, slen + ulen + elen + 3 );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    *ptr++ = (char)slen;				/* Server ID */
    MEcopy( (PTR)sptr, slen, (PTR)ptr );
    ptr += slen;

    *ptr++ = (char)ulen;				/* User ID */
    MEcopy( (PTR)uptr, ulen, (PTR)ptr );
    ptr += ulen;

    *ptr++ = (char)elen;				/* Expiration time */
    MEcopy( (PTR)ebuff, elen, (PTR)ptr );

    srv_parm->length = sizeof( hdr ) + slen + ulen + elen + 3;

  done:

    if ( sptr != sbuff )  (*IIgcs_global->mem_free_func)( (PTR)sptr );
    if ( uptr != ubuff )  (*IIgcs_global->mem_free_func)( (PTR)uptr );
    return( status );
}


/*
** Name: gcs_val_srv
**
** Description:
**	Validates a GCS_SRV_AUTH object.  The server ID 
**	is used to obtain the server key by calling the 
**	GCS server key callback function.  The user alias 
**	is validated against the client alias.
**
** Input:
**	valid		GCS_OP_VALIDATE parameters.
**	hdr		Server authentication object header.
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
**	23-Jun-04 (gordy)
**	    Added expiration time.  Auth object header available as parameter.
**	14-Sep-04 (gordy)
**	    Decoding of expiration time dependent on user ID, so validate
**	    after user ID validation.  Also, fixed various error codes.
**	 6-Dec-05 (gordy)
**	    Differentiate conditions causing validation failure.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.  Use dynamic storage if
**	    lengths exceed default declared buffer lengths.  Size server 
**	    key buffer based on size of object it will hold.
*/

static STATUS
gcs_val_srv( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr )
{
    GCS_GET_KEY_PARM	getkey;
    GCS_OBJ_HDR		tmp;
    char		sbuff[ GCS_MAX_STR + 1 ];
    char		ubuff[ GCS_MAX_STR + 1 ];
    char		kbuff[ (sizeof( GCS_KEY ) * 2) + 1 ];
    char		*server, *user;
    u_i1		*sptr, *uptr, *eptr;
    u_i2		obj_len = GCS_GET_I2( hdr->obj_len );
    u_i2		slen;
    u_i2		ulen = 0; 
    u_i2		elen = 0;
    i4			len;
    STATUS		status = OK;

    union
    {
	GCS_KEY	key;
	u_char	obj[ (sizeof( GCS_KEY ) * 2) + 1 ];
    } obj;

    GCS_TRACE( 4 )( "GCS %s: alias '%s'\n",
		    mech_info.mech_name, valid->alias );

    /*
    ** Extract components.
    */
    sptr = (u_i1 *)valid->auth + sizeof( GCS_OBJ_HDR );

    if ( (slen = *sptr++) < obj_len )
    {
	uptr = sptr + slen;
	ulen = *uptr++;

	if ( hdr->obj_ver >= GCS_OBJ_V1  &&  (slen + ulen + 2) < obj_len )
	{
	    eptr = uptr + ulen;
	    elen = *eptr++;
	}
    }

    /*
    ** Validate server ID.  Make sure object is formatted correctly.
    */
    if (
	 obj_len != ( slen + ulen + 
	 	      (hdr->obj_ver >= GCS_OBJ_V1 ? elen + 3 : 2) )  ||
         *(sptr + slen - 1) != EOS 
       )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid server ID length: %d, %d, %d, %d\n", 
			mech_info.mech_name, obj_len, slen, ulen, elen );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    server = (slen < sizeof( sbuff ))
    	     ? sbuff : (char *)(*IIgcs_global->mem_alloc_func)( slen + 1 );

    if ( ! server )  return( E_GC1013_NO_MEMORY );
    gcs_decode( (char *)sptr, IIgcs_global->host, server );

    /*
    ** Retrieve serve key.
    */
    GCS_TRACE( 4 )( "GCS %s: requesting key for server '%s'\n", 
		    mech_info.mech_name, server );

    getkey.server = server;
    getkey.length = sizeof( kbuff );
    getkey.buffer = kbuff;

    if ( (status = (*IIgcs_global->get_key_func)( &getkey )) != OK )
    {
	GCS_TRACE( 1 )( "GCS %s: key request failed!\n", mech_info.mech_name );
	status = E_GC1009_INVALID_SERVER;
    	goto done1;
    }

    /*
    ** Validate server key.
    */
    kbuff[ getkey.length ] = EOS;
    CItobin( (PTR)kbuff, &len, obj.obj );
    status = gcs_validate( len, (PTR)&obj.key, &tmp );
    if ( status != OK )  goto done1;

    GCS_TRACE( 4 )( "GCS %s: server key '%s'\n",
		    mech_info.mech_name, obj.key.key );

    /*
    ** Validate user alias.  Make sure object is formatted correctly.
    */
    if ( *(uptr + ulen - 1) != EOS )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid alias ID length: %d\n", 
			mech_info.mech_name, ulen );
	status =  E_GC1011_INVALID_DATA_OBJ;
    	goto done1;
    }

    user = (ulen < sizeof( ubuff ))
    	   ? ubuff : (char *)(*IIgcs_global->mem_alloc_func)( ulen + 1 );

    if ( ! user )  
    {
        status = E_GC1013_NO_MEMORY;
	goto done1;
    }

    gcs_decode( (char *)uptr, obj.key.key, user );

    if ( STcompare( user, valid->alias ) )
    {
	GCS_TRACE( 1 )( "GCS %s: alias '%s' does not match auth: '%s'\n",
			mech_info.mech_name, valid->alias, user );
	status = E_GC100D_SRV_AUTH_FAIL;
    	goto done;
    }

    /*
    ** Validate expiration time.
    */
    if ( elen )
    {
	if ( elen > GCS_EXP_LEN  ||  *(eptr + elen - 1) != EOS )
	{
	    GCS_TRACE( 1 )( "GCS %s: invalid expiration length: %d\n", 
			    mech_info.mech_name, elen );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    goto done;
	}

	if ( (status = gcs_chk_exp( (char *)eptr, user )) != OK )
	    goto done;
    }
    else  if ( expirations_required )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid GCS server object version!\n", 
			mech_info.mech_name );
	status = E_GC100B_NO_EXPIRATION;
    	goto done;
    }

  done:

    if ( user != ubuff )  (*IIgcs_global->mem_free_func)( (PTR)user );

  done1:

    if ( server != sbuff )  (*IIgcs_global->mem_free_func)( (PTR)server );
    return( status );
}


/*
** Name: gcs_rem_auth
**
** Description:
**	Creates a GCS_REM_AUTH object which contains the remote
**	authentication token provided.
**
** Input:
**	rem_parm	GCS_OP_REM_AUTH parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 4-Dec-97 (rajus01)
**	    Created.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
*/

static STATUS
gcs_rem_auth( GCS_REM_PARM *rem_parm )
{
    GCS_OBJ_HDR	hdr;
    u_i1	*ptr = (u_i1 *)rem_parm->buffer;

    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id  = GCS_REM_AUTH;
    hdr.obj_ver = GCS_OBJ_V0;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, rem_parm->length );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );
    MEcopy( rem_parm->token, rem_parm->length, (PTR)ptr );
    rem_parm->size = sizeof( hdr ) + rem_parm->length;

    return( OK );
}


/*
** Name: gcs_val_rem
**
** Description:
**	Validates a GCS_REM_AUTH object.  The remote authentication
**	token and the client user alias are passed to the GCS
**	installation password callback function for validation.
**
** Input:
**	valid		GCS_OP_VALIDATE parameters.
**	hdr		Remote authentication object header.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 4-Dec-97 (rajus01)
**	    Created.
*/

static STATUS
gcs_val_rem( GCS_VALID_PARM *valid, GCS_OBJ_HDR *hdr )
{
    GCS_IP_PARM parm;
    STATUS	status;
    u_i2	len = GCS_GET_I2( hdr->obj_len );

    GCS_TRACE( 4 )( "GCS %s: alias '%s'\n",
		    mech_info.mech_name, valid->alias );

    /* 
    ** Invoke the callback function to validate the ticket.
    */
    parm.user   = valid->alias;
    parm.length = len;
    parm.ticket = (char *)valid->auth + sizeof( GCS_OBJ_HDR );

    if ( (status = (*IIgcs_global->ip_func)( &parm )) != OK )
	GCS_TRACE( 1 )( "GCS %s: invalid ticket\n", mech_info.mech_name );

    return( status );
}


/*
** Name: gcs_e_init
**
** Description:
**	Create a GCS_E_INIT object.  Different types of encryption
**	algorithms are supported.  Each type provides its own data 
**	in the object.
**
**	    ETYPE_0:	No additional data.
**	    ETYPE_1:	Random initial key (encoding).
**
** Input:
**	init		GCS_OP_E_INIT parameters.
**	escb		Encryption session control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_init( GCS_EINIT_PARM *init, ESCB *escb )
{
    GCS_OBJ_HDR	hdr;
    u_i2	length;
    u_i1	*ptr = (u_i1 *)init->buffer;

    /*
    ** Our initial object contains the header.
    */
    length = sizeof( hdr );
    escb->type = ETYPE_MAX;

    if ( escb->type >= ETYPE_1 )  
    {
	/*
	** Generate random initial encode key (also save as decode
	** key since it is needed later and gcs_e1_init() updates
	** the encode key).  Our actual decode key will be obtained
	** from the peer confirm.
	*/
	length += sizeof( u_i4 );	/* Send initial key to peer */
	escb->info.etype_1.enc_key = (u_i4)(MHrand() * MAXI4);
	escb->info.etype_1.dec_key = escb->info.etype_1.enc_key;
	gcs_e1_init( escb );
    }

    if ( init->length < length )
    {
	GCS_TRACE( 1 )
	    ( "GCS %s: insufficient buffer (%d of %d)\n",
	      mech_info.mech_name, init->length, (i4)length );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    /*
    ** Build the E_INIT object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id  = GCS_E_INIT;
    hdr.obj_ver = escb->type;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, length - sizeof( hdr ) );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    GCS_TRACE( 4 )( "GCS %s: requested encryption type: %d\n",
		    mech_info.mech_name,  (i4)escb->type );

    if ( escb->type >= ETYPE_1 )
    {
	GCS_TRACE( 4 )( "GCS %s: encode key: %d\n",
			mech_info.mech_name,  escb->info.etype_1.dec_key );

	/*
	** Send our initial encode key (saved as decode key) to peer.
	*/
	GCS_PUT_I4( (u_i1 *)ptr, escb->info.etype_1.dec_key );
	ptr += sizeof( u_i4 );
    }

    init->length = length;

    return( OK );
}


/*
** Name: gcs_e_ipeer
**
** Description:
**	Process a GCS_E_INIT object and initialize encryption.
**	Data for each encryption algorithm type are interpreted 
**	as follows:
**	
**	    ETYPE_0:	No additional data.
**	    ETYPE_1:	Random initial key (decoding).
**
** Input:
**	init		GCS_OP_E_INIT parameters.
**	hdr		GCS_E_INIT object header.
**	escb		Encryption session control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_ipeer( GCS_EINIT_PARM *init, GCS_OBJ_HDR *hdr, ESCB *escb )
{
    u_i1	*ptr = (u_i1 *)init->buffer + sizeof( *hdr );
    u_i2	length = GCS_GET_I2( hdr->obj_len );

    GCS_TRACE( 4 )
	( "GCS %s: validating %s (%d bytes)\n", mech_info.mech_name, 
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
				      hdr->obj_id ), (i4)length );

    if ( hdr->obj_id != GCS_E_INIT )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
			mech_info.mech_name, (i4)hdr->obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    /*
    ** Negotiate the encryption type used.
    */
    escb->type = (u_i1)(min( hdr->obj_ver, ETYPE_MAX ));

    GCS_TRACE( 4 )( "GCS %s: negotiated encryption type: %d\n",
		    mech_info.mech_name,  (i4)escb->type );

    if ( escb->type >= ETYPE_1 )
    {
	if ( length < sizeof( u_i4 ) )
	{
	    GCS_TRACE( 1 )( "GCS %s: insufficient data (%d of %d)\n",
			    mech_info.mech_name, (i4)length, sizeof( u_i4 ) );
	    return( E_GC1011_INVALID_DATA_OBJ );
	}

	/*
	** Get the initial encode key.  Once the encryption
	** system is initialized, the resulting encode key
	** is used by our peer for encoding, so we must use
	** it for decoding.
	*/
	escb->info.etype_1.enc_key = GCS_GET_I4( ptr );
	ptr += sizeof( u_i4 );
	length -= sizeof( u_i4 );

	GCS_TRACE( 4 )( "GCS %s: decode key: %d\n",
			mech_info.mech_name,  escb->info.etype_1.enc_key );

	gcs_e1_init( escb );
	escb->info.etype_1.dec_key = escb->info.etype_1.enc_key;
    }

    return( OK );
}


/*
** Name: gcs_e_confirm
**
** Description:
**	Create a GCS_E_CONFIRM object.  Different types of encryption
**	algorithms are supported.  Each type provides its own data 
**	in the object.
**
**	    ETYPE_0:	No additional data.
**	    ETYPE_1:	Random initial key (encoding).
**
** Input:
**	conf		GCS_OP_E_CONFIRM parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_confirm( GCS_ECONF_PARM *conf )
{
    ESCB	*escb = (ESCB *)conf->escb;
    GCS_OBJ_HDR	hdr;
    u_i2	length;
    u_i1	*ptr = (u_i1 *)conf->buffer;

    /*
    ** Our initial object contains the header and
    ** the negotiated encryption type.
    */
    length = sizeof( hdr );

    if ( escb->type >= ETYPE_1 )  
    {
	/*
	** Generate our random initial encode key.
	*/
	length += sizeof( u_i4 );	/* Send initial key to peer */
	escb->info.etype_1.enc_key = (u_i4)(MHrand() * MAXI4);
    }

    if ( conf->length < length )
    {
	GCS_TRACE( 1 )
	    ( "GCS %s: insufficient buffer (%d of %d)\n",
	      mech_info.mech_name, conf->length, (i4)length );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    /*
    ** Build the E_CONFIRM object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id = GCS_E_CONFIRM;
    hdr.obj_ver = escb->type;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, length - sizeof( hdr ) );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    GCS_TRACE( 4 )( "GCS %s: encryption type: %d\n",
		    mech_info.mech_name,  (i4)escb->type );

    if ( escb->type >= ETYPE_1 )
    {
	GCS_TRACE( 4 )( "GCS %s: encode key: %d\n",
		        mech_info.mech_name,  escb->info.etype_1.enc_key );

	/*
	** Send our initial encode key to peer.
	*/
	GCS_PUT_I4( ptr, escb->info.etype_1.enc_key );
	ptr += sizeof( u_i4 );
	escb->info.etype_1.enc_key = SRAND( escb->info.etype_1.enc_key );
    }

    conf->length = length;

    return( OK );
}


/*
** Name: gcs_e_cpeer
**
** Description:
**	Process a GCS_E_CONFIRM object.  Data for each encryption 
**	algorithm type are interpreted as follows:
**	
**	    ETYPE_0:	No additional data.
**	    ETYPE_1:	Random initial key (decoding).
**
** Input:
**	conf		GCS_OP_E_INIT parameters.
**	hdr		GCS_E_CONFIRM object header.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_cpeer( GCS_ECONF_PARM *conf, GCS_OBJ_HDR *hdr )
{
    ESCB	*escb = (ESCB *)conf->escb;
    u_i1	*ptr = (u_i1 *)conf->buffer + sizeof( *hdr );
    u_i2	length = GCS_GET_I2( hdr->obj_len );

    GCS_TRACE( 4 )
	( "GCS %s: validating %s (%d bytes)\n", mech_info.mech_name,
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs,
				      hdr->obj_id ), (i4)length );

    if ( hdr->obj_id != GCS_E_CONFIRM )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
			mech_info.mech_name, (i4)hdr->obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    /*
    ** Get the negotiated encryption type.
    */
    if ( (escb->type = hdr->obj_ver) > ETYPE_MAX )
    {
	GCS_TRACE( 1 )( "GCS %s: algorithm negotiation failed (%d > %d)\n",
			mech_info.mech_name, (i4)escb->type, ETYPE_MAX );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    GCS_TRACE( 4 )( "GCS %s: negotiated encryption type: %d\n",
		    mech_info.mech_name,  (i4)escb->type );

    if ( escb->type >= ETYPE_1 )
    {
	if ( length < sizeof( u_i4 ) )
	{
	    GCS_TRACE( 1 )( "GCS %s: insufficient data (%d of %d)\n",
			    mech_info.mech_name, (i4)length, sizeof( u_i4 ) );
	    return( E_GC1011_INVALID_DATA_OBJ );
	}

	/*
	** Get our peers initial encode key 
	** which we must use for decoding.
	*/
	escb->info.etype_1.dec_key = GCS_GET_I4( ptr );
	ptr += sizeof( u_i4 );
	length -= sizeof( u_i4 );

	GCS_TRACE( 4 )( "GCS %s: decode key: %d\n",
			mech_info.mech_name,  escb->info.etype_1.dec_key );

	escb->info.etype_1.dec_key = SRAND( escb->info.etype_1.dec_key );
    }

    return( OK );
}


/*
** Name: gcs_e_encode
**
** Description:
**	Create a GCS_E_DATA encryped data object.  Different types 
**	of encryption algorithms are supported.  Each type provides 
**	its own data in the object.
**
**	    ETYPE_0:	No additional data (hdr->obj_info = 0).
**	    ETYPE_1:	No additional data (padding size in hdr->obj_info).
**
**	Sufficient room is made for the GCS_E_DATA object header and 
**	info in the data buffer by appending encrypted data from the 
**	start of the data buffer at the end of the buffer.
**
** Input:
**	enc		GCS_OP_E_ENCODE parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_encode( GCS_EENC_PARM *enc )
{
    ESCB	*escb = (ESCB *)enc->escb;
    GCS_OBJ_HDR	hdr;
    u_i2	length;
    u_i1	*src, *dst;
    u_i1	pad = 0;
    u_i1	*ptr = (u_i1 *)enc->buffer;

    /*
    ** Determine amount of space needed for
    ** E_DATA object, starting with header.
    */
    length = sizeof( hdr );

    switch( escb->type )
    {
	case ETYPE_0 : break;

	case ETYPE_1 :
	    /*
	    ** Data to be encrypted must be an integral 
	    ** multiple of the encryption block size. 
	    */
	    if ( (pad = enc->length % ENC_BLOCK_SIZE) )
	    {
		pad = ENC_BLOCK_SIZE - pad;
		MEfill( pad, 0, (PTR)(ptr + enc->length) );
		enc->length += pad;
	    }

	    GCS_TRACE( 4 )( "GCS %s: encoding %d bytes (%d pad) with key %d\n",
			    mech_info.mech_name,  enc->length, (i4)pad,
			    escb->info.etype_1.enc_key );

	    gcs_e1_encode( escb, enc->length, ptr );
	    break;
    }

    /*
    ** Make room for the E_DATA object at the start
    ** of the buffer by moving data to end of buffer.
    ** May overlap and src < dst so can't use MEcopy().
    */
    for( src = ptr + length, dst = ptr + enc->length + length; src > ptr; )
	*(--dst) = *(--src);

    enc->length += length;

    /*
    ** Build the E_DATA object.
    */
    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_INGRES;
    hdr.obj_id = GCS_E_DATA;
    hdr.obj_ver = escb->type;
    hdr.obj_info = pad;
    GCS_PUT_I2( hdr.obj_len, enc->length - sizeof( hdr ) );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    ptr += sizeof( hdr );

    return( OK );
}


/*
** Name: gcs_e_decode
**
** Description:
**	Process a GCS_E_DATA encrypted data object.  Data for each 
**	encryption algorithm type are interpreted as follows:
**	
**	    ETYPE_0:	No additional data (hdr->obj_info = 0).
**	    ETYPE_1:	No additional data (padding size in hdr->obj_info).
**
**	The GCS_E_DATA object header and info is removed from the
**	buffer and the encrypted data appended to the end of the
**	buffer is restored to the start of the buffer.
**
** Input:
**	dec		GCS_OP_E_DECODE parameters.
**	hdr		GCS_E_DATA object header.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
**	 8-May-98 (gordy)
**	    Use new object header values for encryption information.
*/

static STATUS
gcs_e_decode( GCS_EDEC_PARM *dec, GCS_OBJ_HDR *hdr )
{
    ESCB	*escb = (ESCB *)dec->escb;
    u_i2	length = GCS_GET_I2( hdr->obj_len );
    u_i1	*buff = (u_i1 *)dec->buffer;
    u_i1	*ptr = buff + sizeof( *hdr );
    u_i1	pad = 0;

    GCS_TRACE( 4 )
	( "GCS %s: validating %s (%d bytes)\n", mech_info.mech_name,
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs,
				      hdr->obj_id ), (i4)length );

    if ( hdr->obj_id != GCS_E_DATA )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object ID (%d)\n",
			mech_info.mech_name, (i4)hdr->obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    if ( hdr->obj_ver != escb->type )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid object version (%d)\n",
			mech_info.mech_name, (i4)hdr->obj_ver );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    /*
    ** Remove E_DATA object from buffer.
    */
    MEcopy( buff + length, (ptr - buff), buff );
    dec->length = length;

    switch( escb->type )
    {
	case ETYPE_0 : break;

	case ETYPE_1 :
	    /*
	    ** Decrypt data and remove padding.
	    */
	    GCS_TRACE( 4 )( "GCS %s: decoding %d bytes (%d pad) with key %d\n",
			    mech_info.mech_name,  length, (i4)hdr->obj_info, 
			    escb->info.etype_1.dec_key );

	    gcs_e1_decode( escb, length, buff );
	    dec->length -= hdr->obj_info;
	    break;
    }

    return( OK );
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
**	23-Jun-04 (gordy)
**	    Bumped versions for objects with expiration times.
*/

static STATUS
gcs_validate( i4  length, PTR buffer, GCS_OBJ_HDR *hdr )
{
    u_i4 id;
    u_i2 len;
    u_i1 max_ver;

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

    if ( hdr->mech_id != GCS_MECH_INGRES )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid mechanism ID (%d)\n",
			mech_info.mech_name, (i4)hdr->mech_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    /*
    ** Version for GCS_E_INIT is negotiated down to ETYPE_MAX,
    ** all other versions must be within valid range.
    */
    switch( hdr->obj_id )
    {
	case GCS_USR_AUTH :	max_ver = GCS_OBJ_V1;	break;
	case GCS_PWD_AUTH :	max_ver = GCS_OBJ_V2;	break;
	case GCS_SRV_AUTH :	max_ver = GCS_OBJ_V1;	break;
	case GCS_E_CONFIRM :	max_ver = ETYPE_MAX;	break;
	case GCS_E_DATA :	max_ver = ETYPE_MAX;	break;
	default	:		max_ver = GCS_OBJ_V0;	break;
    }

    if ( hdr->obj_id != GCS_E_INIT  &&  hdr->obj_ver > max_ver )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid version %d, expected %d\n",
			mech_info.mech_name, (i4)hdr->obj_ver, (i4)max_ver );
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


/*
** Name: gcs_gen_exp
**
** Description:
**	Generates an encoded expiration time.  Output
**	buffer should be at least GCS_EXP_LEN bytes in 
**	length.
**
** Input:
**	limit	Number of seconds to expiration.
**	key	Encryption key.
**
** Output:
**	buff	Encrypted expiration.
**
** Returns:
**	i4	Length of output, -1 if error.
**
** History:
**	23-Jun-04 (gordy)
**	    Created.
**	21-Jul-09 (gordy)
**	    May return a negative value if error in gcs_encode().
**	    User more appropriate size for buffers.
*/

static i4 
gcs_gen_exp( i4 limit, char *key, char *buff )
{
    GCS_EXP	exp;
    char	str[ GCS_EXP_LEN + 1 ];

    GCS_PUT_I4( exp.id, GCS_EXP_ID );
    GCS_PUT_I4( exp.secs, TMsecs() + limit );
    CItotext( (u_char *)&exp, sizeof( exp ), (PTR)str );

    return( gcs_encode( str, key, buff ) );
}


/*
** Name: gcs_chk_exp
**
** Description:
**	Validate encode expiration time.
**
** Input:
**	estr	Encoded expiration time.
**	key	Decryption key.
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	23-Jun-04 (gordy)
**	    Created.
**	14-Sep-04 (gordy)
**	    Fix error code for corrupted data.
*/

static STATUS
gcs_chk_exp( char *estr, char *key )
{
    char	buff[ GCS_EXP_LEN + 1 ];
    i4		len;

    union
    {
	GCS_EXP	exp;
	u_char	obj[ GCS_EXP_LEN + 1 ];
    } obj;

    gcs_decode( estr, key, buff );
    CItobin( (PTR)buff, &len, obj.obj );

    if ( len != sizeof(obj.exp)  ||  GCS_GET_I4(obj.exp.id) != GCS_EXP_ID )
    {
	GCS_TRACE( 1 )( "GCS %s: invalid expire object: len=%d, id=0x%x!\n", 
			mech_info.mech_name, len, GCS_GET_I4( obj.exp.id ) );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    if ( TMsecs() > GCS_GET_I4( obj.exp.secs ) )
    {
	GCS_TRACE(1)( "GCS %s: authentication expired: expire=%d, now=%d!\n", 
		      mech_info.mech_name, GCS_GET_I4(obj.exp.secs), TMsecs() );
	return( E_GC100A_AUTH_EXPIRED );
    }

    return( OK );
}


/*
** Name: gcs_encode
**
** Description:
**	Encrypts a string using key provided.  Encrypted
**	value is returned in text format.
**
**	Output buffer length should be at least GCS_ENC_SIZE( len )
**	bytes in length.
**
** Input:
**	str		Value to be encrypted.
**	key		Key for encryption.
**
** Output:
**	buff		Encrypted value in text format.
**
** Returns:
**	i4		Length of encrypted value, -1 if error.
**
** History:
**	19-May-97 (gordy)
**	    Created.
**	17-Jul-09 (gordy)
**	    Remove string length restrictions.  Return negative
**	    value if error occurs.  Use dynamic storage if length
**	    exceeds default declared buffer length.
*/

static i4
gcs_encode( char *str, char *key, char *buff )
{
    CI_KS	ksch;
    u_i1	kbuff[ ENC_KEY_LEN + 1 ];
    u_i1	tbuff[ GCS_MAX_BUF + 1 ];
    u_i1	*tmp = tbuff;
    i4		tsize = sizeof( tbuff );
    i4		len;
    char	*ptr;
    bool	done;

    /*
    ** Ensure temp buffer is large enough.
    */
    len = GCS_ENC_SIZE( STlength( str ) ) + 1;

    if ( len >= tsize )
    {
	tsize = len + 1;
    	tmp = (char *)(*IIgcs_global->mem_alloc_func)( tsize );
        if ( ! tmp )  return( -1 );
    }

    /*
    ** Generate a ENC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.  
    ** ENC_KEY_LEN must be a multiple of CRYPT_SIZE.
    */
    for( ptr = key, len = 0; len < ENC_KEY_LEN; len++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ len ] = *ptr++;
    }

    kbuff[ len ] = EOS;
    CIsetkey( (PTR)kbuff, ksch );

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
    for( done = FALSE, len = 0; ! done  &&  len < (tsize - 1); )
    {
	i4 csize = STlength( cset );

	/*
	** First character in each encryption block is random.
	*/
	tmp[ len++ ] = cset[ (i4)(MHrand() * csize) % csize ];

	/*
	** Load string into remainder of encryption block.
	*/
        while( *str  &&  len % CRYPT_SIZE )  tmp[ len++ ] = *str++;

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
		tmp[ len++ ] = cset[ (i4)(MHrand() * csize) % csize ];
	}
    }

    /*
    ** Encrypt and convert to text.
    */
    tmp[ len ] = EOS;
    CIencode( (PTR)tmp, len, ksch, (PTR)tmp );
    CItotext( (uchar *)tmp, len, (PTR)buff );

    if ( tmp != tbuff )  (*IIgcs_global->mem_free_func)( (PTR)tmp );
    return( STlength( buff ) );
}


/*
** Name: gcs_decode
**
** Description:
**	Decrypt a string encrypted by gcs_encode().
**
**	Decrypted string will be smaller than input.  A output
**	buffer the same size as the input should be provided
**	to ensure sufficient space.
**
** Input:
**	str		Encrypted value to be decrypted.
**	key		Key for decryption.
**
** Output:
**	buff		Decrypted value.
**
** Returns:
**	i4		length of decrypted value or -1 if error.
**
** History:
**	19-May-97 (gordy)
**	    Created.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.  Password can be
**	    decoded and deformatted directly into output buffer.
*/

static i4
gcs_decode( char *str, char *key, char *buff )
{
    CI_KS	ksch;
    u_i1	kbuff[ ENC_KEY_LEN + 1 ];
    u_i1	*ptr;
    i4		len;

    /*
    ** Generate a ENC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.
    ** ENC_KEY_LEN must be a multiple of CRYPT_SIZE.
    */
    for( ptr = key, len = 0; len < ENC_KEY_LEN; len++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ len ] = *ptr++;
    }

    kbuff[ len ] = EOS;
    CIsetkey( (PTR)kbuff, ksch);

    /*
    ** Convert encode password back to binary format.
    **
    ** Output buffer should be same size as input, so the
    ** decoded result will fit.  The result string will
    ** be shorter still when formatting is removed.  The
    ** string can be rebuilt in place, so decoding is
    ** done directly into output buffer.
    **
    ** Encrypted string should already be the correct 
    ** size, but ensure that it really is a multiple 
    ** of CRYPT_SIZE.
    */
    CItobin( (PTR)str, &len, (u_char *)buff );
    while( len % CRYPT_SIZE )  buff[ len++ ] = EOS;
    CIdecode( (PTR)buff, len, ksch, (PTR)buff );

    /*
    ** Remove padding and delimiter from last encryption block.
    */
    while( len  &&  buff[ --len ] != CRYPT_DELIM );
    buff[ len ] = EOS;

    /*
    ** Extract original string skipping first character in each 
    ** encryption block.
    */
    for( ptr = buff, len = 0; buff[ len ]; len++ )
	if ( len % CRYPT_SIZE )  *ptr++ = buff[ len ];

    *ptr = EOS;
    return( str - buff );
}


/*
** Name: gcs_e1_init
**
** Description:
**	Initializes the Type 1 encryption algorithm.  The encoding
**	key is used (and modified) to generate an encoding random 
**	substitution array and a random block shuffle array.  A 
**	decoding substitution array is generated from the encoding 
**	array.
**
** Input:
**	escb		Encryption session control block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
*/

static VOID
gcs_e1_init( ESCB *escb )
{
    u_i4	seed = SRAND( escb->info.etype_1.enc_key );
    u_i1	tmp;
    i4		i, j;

    /*
    ** Initialize the arrays as direct pass through.
    */
    for( i = 0; i < 256; i++ )  
	escb->info.etype_1.enc_arr[ i ] = (u_i1)i;

    for( i = 0; i < ENC_BLOCK_SIZE; i++ )  
	escb->info.etype_1.pos_arr[ i ] = (u_i1)i;

    /*
    ** Randomize the arrays by switching each entry
    ** with a random entry.
    */
    for( i = 0; i < 256; i++ )
    {
	j = (u_i1)((seed = RAND(seed)) & 0xff);
	tmp = escb->info.etype_1.enc_arr[ i ];
	escb->info.etype_1.enc_arr[ i ] = escb->info.etype_1.enc_arr[ j ];
	escb->info.etype_1.enc_arr[ j ] = tmp;
    }

    for( i = 0; i < ENC_BLOCK_SIZE; i++ )
    {
	j = (u_i1)((seed = RAND(seed)) % ENC_BLOCK_SIZE);
	tmp = escb->info.etype_1.pos_arr[ i ];
	escb->info.etype_1.pos_arr[ i ] = escb->info.etype_1.pos_arr[ j ];
	escb->info.etype_1.pos_arr[ j ] = tmp;
    }

    /*
    ** The decode array is built as the inverse of
    ** the encode array.  The position (shuffle)
    ** array does not require an inverse as it may
    ** be used directly.
    */
    for( i = 0; i < 256; i++ )  
	escb->info.etype_1.dec_arr[ escb->info.etype_1.enc_arr[i] ] = (u_i1)i;

    escb->info.etype_1.enc_key = seed;

    return;
}


/*
** Name: gcs_e1_encode
**
** Description:
**	Encode a data buffer using the Type 1 encryption algorithm.
**	The data length must be an integral multiple of the block
**	size (ENC_BLOCK_SIZE) or trailing data may not be encrypted.
**	Each character in the buffer is exclusive-or'd with a random
**	value generated from the encode key and the resulting value 
**	is replaced using the encoding substitution array generated 
**	in gcs_e1_init().  Each block in the buffer is reordered 
**	using the shuffle array generated in gcs_e1_init().
**
** Input:
**	escb		Encryption session control block.
**	len		Length of data.
**	buff		Data to be encrypted.
**
** Output:
**	buff		Encrypted data.
**
** Returns:
**	VOID
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
*/

static VOID
gcs_e1_encode( ESCB *escb, i4  len, u_i1 *buff )
{
    u_i4	seed = escb->info.etype_1.enc_key;
    u_i1	c, temp[ ENC_BLOCK_SIZE ];
    i4  	i;
      
    for( ; len >= ENC_BLOCK_SIZE; 
	 len -= ENC_BLOCK_SIZE, buff += ENC_BLOCK_SIZE )  
    {
	for( i = 0; i < ENC_BLOCK_SIZE; i++ )
	{
	    c = buff[ i ] ^ ((seed = RAND(seed)) & 0xff);
	    temp[ escb->info.etype_1.pos_arr[ i ] ] = 
		escb->info.etype_1.enc_arr[ c ];
	}

	MEcopy( (PTR)temp, ENC_BLOCK_SIZE, (PTR)buff );
    }

    escb->info.etype_1.enc_key = seed;

    return;
}


/*
** Name: gcs_e1_decode
**
** Description:
**	Decode a data buffer encrypted using the Type 1 encryption
**	algorithm (see gcs_e1_encode()).  The data length must be 
**	an integral multiple of the block size (ENC_BLOCK_SIZE) or 
**	extraneous data may be interspersed in the final block.  
**	Each block in the buffer is reordered using the shuffle 
**	array generated in gcs_e1_init().  Each character in the
**	buffer is then restored using the decoding substitution
**	array generated in gcs_e1_init() and exclusive-or'd with
**	the same random value (generated from the decode key) used
**	in gcs_e1_encode().
**
** Input:
**	escb		Encryption session control block.
**	len		Length of data.
**	buff		Data to be decrypted.
**
** Output:
**	buff		Decrypted data.
**
** Returns:
**	VOID
**
** History:
**	25-Feb-98 (gordy)
**	    Created.
*/

static VOID
gcs_e1_decode( ESCB *escb, i4  len, u_i1 *buff )
{
    u_i4	seed = escb->info.etype_1.dec_key;
    u_i1	c, temp[ ENC_BLOCK_SIZE ];
    i4		i;
	
    for( ; len >= ENC_BLOCK_SIZE; 
         len -= ENC_BLOCK_SIZE, buff += ENC_BLOCK_SIZE )  
    {
	for( i = 0; i < ENC_BLOCK_SIZE; i++ )
	{
	    c = escb->info.etype_1.dec_arr[
					buff[ escb->info.etype_1.pos_arr[i] ]];
	    temp[ i ] = c ^ ((seed = RAND(seed)) & 0xff);
	}

	MEcopy( (PTR)temp, ENC_BLOCK_SIZE, (PTR)buff );
    }

    escb->info.etype_1.dec_key = seed;

    return;
}


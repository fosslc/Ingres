/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <gc.h>
#include <st.h>
#include <me.h>
#include <mu.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcs.h>
#include <gcsint.h>

#ifdef NT_GENERIC
/* Undefine u_long, u_short and u_char which are defined in compat.h */
/* so that the kerberos win-mac.h header can define them along with */
/* uint32_t without getting redefinition errors */
#undef u_long
#undef u_short
#undef u_char

#elif defined(sparc_sol) || defined(VMS)
#include <inttypes.h>

#elif defined(thr_hpux)
/* No additional definitions required */

#else
/* include stdint.h on unix for uint32_t */
#include <stdint.h>

#endif

#include <gssapi/gssapi_generic.h>

/*
** Name: gcskrb.c
**
** Description:
**	Dynamically loadable GCS security mechanism based on Kerberos.
**	This security mechanism provides the following capabilities.
**
**	User Authentication
**	Remote Authentication
**	Encryption/Decryption
**
** History:
**	19-Mar-99 (rajus01)
**	   Created.
**	17-Apr-00 (rajus01)
**	   Added init_crypt.
**      17-jan-2002 (loera01) SIR 106858
**         Use config_host for searching config.dat.
**      13-mar-2002 (loera01)
**         Ported to VMS.
**	10-feb-2003 (chash01)
**         Use non-ingres type for variables used in interface
**      20-apr-2005 (loera01) SIR 114358
**         The gss_c_nt_service_name object id is deprecated in Windows;
**         using the more current GSS_C_NT_HOSTBASED_SERVICE.
**      03-aug-2005 (loera01) Bug 114981
**         Doubled the size of the GCS_ENC_KRB_OVHD constant. 64 bytes of
**         encryption overhead was insufficient in some cases.
**      17-oct-2005 (loera01) Bug 115408
**         The gss_c_nt_service_name object id is now deprecated in VMS,
**         as Kerberos 2.0 is the minimum version supported.  Superseded 
**         by GSS_C_NT_HOSTBASED_SERVICE.
**	6-Feb-2007 (bonro01)
**	   Update Kerberos headers to v5 r1.6
** 	06-mar-2007 (abbjo03)
**	    Include inttypes.h on VMS.        
**	23-Mar-2007 (bonro01)
**	   Fix compile problem for HPUX
**      01-Aug-2007 (Ralph Loen) SIR 118898
**         In GSSdisplay_status_1(), added provision to write GSS API
**         error message detail to the error log.
**      24-Oct-2007 (Ralph Loen) Bug 119358
**         Remove GCS_CAP_USR_AUTH from the GCS capabilities mask.  Kerberos
**         has been deprecated for local authentication.
**      19-Mar-2008 (Ralph Loen)  Bug 120132
**         Replaced global "init_crypt" boolean with a new "initiator" field 
**         in GSS_CB, since the calling process may be in both initiator and
**         receptor modes.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/


/*
** GSS Service Control Block
*/

typedef struct _GSS_CB
{
    gss_ctx_id_t    	gss_context;
    gss_buffer_desc	iToken;
    gss_buffer_desc 	oToken;
    gss_buffer_t	tToken;
    gss_name_t 		target_name;
    OM_uint32           reqFlags;
    gss_name_t          client;
    gss_cred_id_t	delegated_cred;
    OM_uint32           retFlags;
    OM_uint32           initiator;
}GSS_CB;
 
#define GCS_CNF_MECH_PRINCIPAL	"%s.%s.gcf.mech.%s.domain" /* FQDN host */
#define GCS_CNF_MECH_DELEGATE	"%s.%s.gcf.mech.%s.delegation" /* Delegate ? */
#define	SERVICE_NAME_PREFIX  	"$ingres"	/* Service prefix */
#define GCS_ENC_KRB_OVHD        128      	/* Encryption overhead */
#define MAXSTRLEN		256

/*
** GCS control block.
*/

static  GCS_GLOBAL      *IIgcs_global = NULL;


/*
** External mechanism info.
*/

static  GCS_MECH_INFO   mech_info =
{
    "kerberos",
    GCS_MECH_KERBEROS,
    GCS_MECH_VERSION_1,
    GCS_CAP_ENCRYPT | GCS_CAP_REM_AUTH,
    GCS_AVAIL, GCS_ENC_LVL_2, GCS_ENC_KRB_OVHD

};

static  GCS_MECH_INFO   *gcs_info[] = { &mech_info };


/*
** Forward references.
*/

static  STATUS 	import_name( gss_name_t * target, char *host );
static  STATUS	gcs_validate(i4 length, PTR buffer, GCS_OBJ_HDR *hdr );
static  STATUS  gss_val_auth( GCS_VALID_PARM *valid, bool usr );
static  STATUS  gss_val_delegatedCred( GSS_CB *cb, PTR token, i4 *length );
static  STATUS	gss_init( GSS_CB *gss_cb );
static  STATUS  gss_accept( GSS_CB *gss_cb );
static  STATUS	gss_acquire();
static  STATUS  gss_auth( PTR parms, bool usr );
static  STATUS  gss_term( GSS_CB *gss_cb );
static  STATUS  gss_e_init( GCS_EINIT_PARM *initParm, GSS_CB *gss_cb );
static  STATUS  gss_e_accept( GCS_EINIT_PARM *initParm, GCS_OBJ_HDR *hdr,
			      GSS_CB *gss_cb );
static  STATUS  gss_e_decode( GCS_EDEC_PARM *dcdParm,
			      GCS_OBJ_HDR *hdr, GSS_CB *gss_cb );
static  STATUS  gss_e_encode( GCS_EENC_PARM *ecdParm, GSS_CB *gss_cb );
static  STATUS  gss_se_confirm( GCS_ECONF_PARM *cnfParm, GSS_CB *gss_cb );
static  STATUS  gss_ce_confirm( GCS_ECONF_PARM *cnfParm, GSS_CB *gss_cb,
				GCS_OBJ_HDR *obj_hdr );
static  VOID    GSSdisplay_status( char *msg, OM_uint32 maj_stat,
				   OM_uint32 min_stat );
static  VOID	GSSdisplay_status_1( char *m, OM_uint32 code, i4 type );

static  bool	acquire = FALSE;    		 /* bool for cred acquiring */
static  char	fqdn_str[ MAXSTRLEN ] = {'\0'};  /* Global for hostname */

static  gss_cred_id_t	server_creds;   /* Global for server credentials */
static  OM_uint32   	delegate = 0;	/* Delegation OFF */


/*
** Name: gcs_kerberos
**
** Description:
**	Main entry point for the GCS security mechanism. 
**
** Input:
**      op              Operation.
**      parms           Parameters for operation.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
** 	19-Mar-99 (rajus01)
**	   Created.
**	17-Apr-00 (rajus01)
**	   Don't call gss_term() for server side GCS_OP_E_TERM operation.
**	   Release the oToken buffer.
*/

STATUS
gcs_kerberos( i4 op, PTR parms )
{
    STATUS	status = E_GC1000_GCS_FAILURE;

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
        case GCS_OP_INIT:
	    {
		char	*p;
		char 	str[ MAXSTRLEN ];

		status = OK;

		/*
		** Get the Fully Qualified Domain name of the host
		** from the config file.
		*/
		if ( ! fqdn_str[0] )
		{

		    STprintf ( str, GCS_CNF_MECH_PRINCIPAL, SystemCfgPrefix, 
			      IIgcs_global->config_host, mech_info.mech_name );
		    if ( (*IIgcs_global->config_func)( str, &p ) != OK )
		    {
			GCS_TRACE( 1 )
			    ( "GCS %s: Hostname definiton does not exist.\n",
			      mech_info.mech_name );
			status = E_GC1100_KRB_FQDNHOST;
			break;
		    }

		    STcopy( p, fqdn_str );
		}

		/* Initialize the socket on Windows NT platforms,
		** before obtaining User Authentication or Remote
		** Authentication. Actually this initialization is
		** not required for the Encryption/decryption 
		** operations, since ComSvr performs the initialization
		** while loading the network drivers.
		*/

# ifdef NT_GENERIC
		{
		    WSADATA wsaData;

		    if ( WSAStartup((WORD)0x0101, &wsaData ) )
		    {
			GCS_TRACE( 1 )
			    ( "Unable to initialize WINSOCK: %d",
			      WSAGetLastError() );
			status = E_GC1101_KRB_WINSOCK_INIT; 
		    }
		}
# endif

		/*
		** Determine whether to delegate credentials.
		*/
		STprintf ( str, GCS_CNF_MECH_DELEGATE, SystemCfgPrefix, 
			   IIgcs_global->config_host, mech_info.mech_name );
		if ( (*IIgcs_global->config_func)( str, &p ) == OK  &&
		     !STbcompare( p, 0, "ON", 0, TRUE ) )
		delegate = GSS_C_DELEG_FLAG;
	    }   
	    break;

        case GCS_OP_TERM:
	    {
		OM_uint32	 gMin;

		status = OK;

		if ( acquire )
		{
		    gss_release_cred( &gMin, &server_creds );
		    acquire = FALSE;
		}

		delegate = 0;

# ifdef NT_GENERIC

		WSACleanup();
#endif
	    }
	    break;

	case GCS_OP_INFO :
	    {
		GCS_INFO_PARM   *info_parm = (GCS_INFO_PARM *)parms;

		info_parm->mech_count = 1;
		info_parm->mech_info = gcs_info;
		status = OK;
	    }
	    break;

	case GCS_OP_VALIDATE :
	    {
		GCS_VALID_PARM  *valid = (GCS_VALID_PARM *)parms;
		GCS_OBJ_HDR     hdr;
		u_i2	        len;

		status = gcs_validate( valid->length, valid->auth, &hdr );
		if ( status != OK )  break;

		len = GCS_GET_I2( hdr.obj_len );

		GCS_TRACE( 4 )
		    ( "GCS %s: validating %s (%d bytes)\n",
		      mech_info.mech_name,
		      (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs,
		      hdr.obj_id ), (i4)len );

		switch( hdr.obj_id)
		{
		    case GCS_USR_AUTH :
			status = gss_val_auth( valid, TRUE );
			break;

		    case GCS_REM_AUTH :
			status = gss_val_auth( valid, FALSE );
			break;

		    default :
			GCS_TRACE( 1 )
			    ( "GCS %s: invalid object ID (%d)\n",
			      mech_info.mech_name,
			      (i4)hdr.obj_id );
			status = E_GC1011_INVALID_DATA_OBJ;
			break;
		}

	    }
	    break;

	case GCS_OP_USR_AUTH:
	    status = gss_auth( parms, TRUE );
	    break;

	case GCS_OP_REM_AUTH :
	    status = gss_auth( parms, FALSE );
	    break;

	case GCS_OP_RELEASE :
	    {
		GCS_REL_PARM	*rel = (GCS_REL_PARM *)parms;
		GCS_OBJ_HDR	hdr;
		u_i2		len;

		status = gcs_validate( rel->length, rel->token, &hdr );
		if ( status != OK )  break;
		len = GCS_GET_I2( hdr.obj_len );
		rel->token += sizeof( hdr );

		GCS_TRACE( 4 )
		    ( "GCS %s: releasing %s (%d bytes)\n", mech_info.mech_name, 
		      (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
		      hdr.obj_id ), (i4)len );

		switch( hdr.obj_id )
		{
		    case GCS_USR_AUTH :
		    case GCS_REM_AUTH :
			break;

		    case GCS_DELEGATE :
			{
			    OM_uint32		gMaj, gMin;
			    gss_cred_id_t 	cred;	

			    MEcopy( (PTR)rel->token, len, &cred );

			    gMaj = gss_release_cred( &gMin, &cred );
			    if ( GSS_ERROR( gMaj ) )
			    {
				status = FAIL;
				GSSdisplay_status( "gss_release_cred",
						   gMaj, gMin );
			    }
			}
			break;

			default:
			    GCS_TRACE( 1 )
				( "GCS %s: invalid object ID (%d)\n",
				  mech_info.mech_name, (i4)hdr.obj_id );
			    status = E_GC1011_INVALID_DATA_OBJ;
			    break;
		}
	    }
	    break;

        case GCS_OP_E_INIT:
	    {	
		GCS_EINIT_PARM	*initParm = ( GCS_EINIT_PARM * )parms;  
		GSS_CB		*gss_cb;
		GCS_OBJ_HDR    	hdr;

		/* Allocate GSS service control block */

		if ( ! ( gss_cb = ( GSS_CB * )(*IIgcs_global->mem_alloc_func)
		     ( sizeof( *gss_cb ) ) ) )
	        {
		    status = E_GC1013_NO_MEMORY;
		    break;
	        }

		gss_cb->initiator = initParm->initiator;
		if ( gss_cb->initiator )
		{
		    status = gss_e_init( initParm, gss_cb );
		}
		else if ( ( status = gcs_validate( initParm->length, 
						   initParm->buffer,
						   &hdr ) ) == OK )
		    status = gss_e_accept( initParm, &hdr, gss_cb );

		if ( status == OK )
		    initParm->escb = ( PTR )gss_cb;
		else
		    (*IIgcs_global->mem_free_func)( (PTR)gss_cb );
	    }
	    break;

        case GCS_OP_E_CONFIRM :
	    {
		GCS_ECONF_PARM	*cnfParm = (GCS_ECONF_PARM *)parms;  
		GSS_CB		*gss_cb = (GSS_CB *)cnfParm->escb; 

		if ( ! cnfParm->initiator )
		    status = gss_se_confirm( cnfParm, gss_cb );
		else
		{
		    GCS_OBJ_HDR		obj_hdr;

		    if ( (status = gcs_validate( cnfParm->length,
						 cnfParm->buffer,
						 &obj_hdr ) ) == OK )
			status = gss_ce_confirm( cnfParm, gss_cb, &obj_hdr );
                }
            }
	    break;

        case GCS_OP_E_ENCODE :
	    {	
		GCS_EENC_PARM	*ecdParm = (GCS_EENC_PARM *)parms;  
		GSS_CB		*gss_cb =  (GSS_CB *)ecdParm->escb; 

		status = gss_e_encode( ecdParm, gss_cb );
	    }
	    break;

        case GCS_OP_E_DECODE :
	    {	
		GCS_EDEC_PARM	*dcdParm = (GCS_EDEC_PARM *)parms;  
		GSS_CB		*gss_cb = (GSS_CB *)dcdParm->escb; 
		GCS_OBJ_HDR	hdr;

		if ( (status = gcs_validate( dcdParm->length,
				    	     dcdParm->buffer, &hdr ) ) == OK )
		    status = gss_e_decode( dcdParm, &hdr, gss_cb );
	    }
	    break;

        case GCS_OP_E_TERM :
	    {	
    		OM_uint32	maj_stat, min_stat;
		GCS_ETERM_PARM	*trmParm = (GCS_ETERM_PARM *)parms;  
		GSS_CB		*gss_cb = (GSS_CB *)trmParm->escb; 

		if (gss_cb->initiator)
		    gss_term( gss_cb );
		else
		    if ( gss_cb->oToken.length )
    		    {
		        maj_stat = gss_release_buffer( &min_stat,
							&gss_cb->oToken );
		        if ( GSS_ERROR( maj_stat ) )
			    GSSdisplay_status( "gss_release_buffer", 
							maj_stat, min_stat );
		    }

		(*IIgcs_global->mem_free_func)( (PTR) gss_cb );

		status = OK;
	    }
	    break;

	case GCS_OP_SET :
	case GCS_OP_PWD_AUTH :
	case GCS_OP_SRV_KEY :
	case GCS_OP_SRV_AUTH :
	    GCS_TRACE( 1 ) 
		( "GCS %s: unsupported request\n",
		  mech_info.mech_name );
	    status = E_GC1003_GCS_OP_UNSUPPORTED;
	    break;

	default :
	   GCS_TRACE( 1 ) 
	       ( "GCS %s: invalid request (%d)\n",
		 mech_info.mech_name, op );
	   status = E_GC1002_GCS_OP_UNKNOWN;
	   break;
    }

    GCS_TRACE( 3 )
	( "GCS %s: %s status 0x%x\n", mech_info.mech_name,
	  (*IIgcs_global->tr_lookup)(IIgcs_global->tr_ops, op), status );
    /*
    ** Clean up global info after term request and final tracing.
    */
    if ( IIgcs_global  &&  op == GCS_OP_TERM )
	IIgcs_global = NULL;

   return( status );
}


/*
** Name: gss_acquire
**
** Description:
**	Instantiate a verifier credentials. The credential's mode
**	ACCEPT. i.e., credentials can be only used to accept the
**	security context.
**
** Input:
**      None.
**
** Output:
**      server_creds. 	Credentials handle.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
** 	19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS
gss_acquire()
{
    OM_uint32	maj_stat, min_stat;
    OM_uint32	retTime;
    gss_name_t	target;
    gss_OID_set	vMechSet = GSS_C_NULL_OID_SET;

    STATUS	status = OK;

    if ( ! acquire )
    {
        if ( ( status = import_name( &target, fqdn_str ) ) != OK )
	   return( status );

	maj_stat = gss_acquire_cred( &min_stat, 
				     target,
				     GSS_C_INDEFINITE,
	 			     GSS_C_NULL_OID_SET, 
				     GSS_C_ACCEPT,
  	 			     &server_creds,
				     &vMechSet, 
				     &retTime );

	/* Check for function error */

	if ( ! GSS_ERROR( maj_stat ) )
	{
	    acquire = TRUE;
	    gss_release_name( &min_stat, &target );
	}
	else
    	{
	    status = E_GC1102_KRB_GSS_ACQUIRE_CRED;

	    if ( IIgcs_global->err_log_func )
		(*IIgcs_global->err_log_func)
		    ( E_GC1102_KRB_GSS_ACQUIRE_CRED, 0, NULL );

	    GSSdisplay_status( "gss_acquire_cred", maj_stat, min_stat );

	    gss_release_name( &min_stat, &target );
            gss_release_cred( &min_stat, &server_creds );
	}

    } 

    GCS_TRACE( 4 ) 
    	( "GCS %s: gss_acquire status 0x%x\n", mech_info.mech_name, status );

    return( status );
}



/*
** Name: gss_init
**
** Description:
**	Client or initiator initializes security context. During the
**	context establishment phase , default( NULL ) credentials have
**	been used as the claimant credentials. This will search the
**	client's credentials cache.
**
** Input:
**      gss_cb        	GSS control block.
**
** Output:
**	gss_cb->iToken	Output Token.	
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**	19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS
gss_init( GSS_CB *gss_cb )
{
    OM_uint32	maj_stat = 0, min_stat = 0;
    OM_uint32	retFlags;
    gss_OID	theActualMech = GSS_C_NULL_OID;
    OM_uint32	reqTime = 0, retTime;

    STATUS	status = OK;

    maj_stat = gss_init_sec_context( &min_stat,
				     gss_cb->delegated_cred,
		       	     	     &gss_cb->gss_context,
			     	     gss_cb->target_name,
		       	     	     GSS_C_NULL_OID, 
				     gss_cb->reqFlags, reqTime, 
		       	     	     NULL,
			     	     gss_cb->tToken,
			     	     &theActualMech, 
                       	     	     &gss_cb->iToken,
				     &retFlags, &retTime );

    /* Check for function error. */

    if ( GSS_ERROR( maj_stat ) )
    {

	status = E_GC1103_KRB_GSS_INIT_SEC_CTX;

	if ( IIgcs_global->err_log_func )
	    (*IIgcs_global->err_log_func)
		    ( E_GC1103_KRB_GSS_INIT_SEC_CTX, 0, NULL );

	GSSdisplay_status( "gss_init_sec_context", maj_stat, min_stat );

	maj_stat = gss_release_name(&min_stat, &gss_cb->target_name );
        if ( GSS_ERROR( maj_stat ) )
	    GSSdisplay_status( "gss_release_name", maj_stat, min_stat );
	maj_stat = gss_release_buffer( &min_stat, &gss_cb->iToken );
        if ( GSS_ERROR( maj_stat ) )
	    GSSdisplay_status( "gss_release_buffer", maj_stat, min_stat );
	maj_stat = gss_release_buffer( &min_stat, gss_cb->tToken );
        if ( GSS_ERROR( maj_stat ) )
	    GSSdisplay_status( "gss_release_buffer", maj_stat, min_stat );
	return( status );
    }

    /* 
    ** The following trace stmt indicates that for the encode/decode
    ** operations, gss_init_sec_context needs to be invoked again to
    ** complete mutual authentication between the client and server
    ** Comsvrs. It is ignored for the user and remote authentication
    ** operations.
    */

    if ( maj_stat && GSS_S_CONTINUE_NEEDED )   /* E_INIT operation */
    	GCS_TRACE( 3 ) 
	    ( "GCS %s: gss_init_sec_context, continue needed..\n",
	      mech_info.mech_name );

    return( status );
}



/*
** Name: gss_accept
**
** Description:
**	Server or  acceptor accepts the security context. It uses the
**	verifier credentials constructed by gss_acquire() routine.
**
** Input:
**      gss_cb        GSS control block.
**
** Output:
**	gss_cb->oToken	Output Token only for the
**			encode/decode operations.
**
**	gss_cb->client  Authenticated name of security
**			context initiator. This name is
**			an internal form name.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**	19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS
gss_accept( GSS_CB  *gss_cb ) 
{
    OM_uint32		maj_stat = 0, min_stat = 0;
    OM_uint32		ret_flags, retTime;
    gss_cred_id_t	delegatedCred;
    gss_OID 		doid = GSS_C_NULL_OID;

    STATUS		status = OK;

    gss_cb->gss_context = NULL;
    gss_cb->client 	= NULL;

    maj_stat = gss_accept_sec_context( &min_stat,
             		        	&gss_cb->gss_context,
	   	     		        server_creds,
				        &gss_cb->iToken,
	               			NULL,
		                     	&gss_cb->client,
		                     	&doid,
		                     	&gss_cb->oToken,
                                       	&ret_flags,
	                             	&retTime,   
	                             	&delegatedCred ); 

    /* Check for function error. */

    if ( GSS_ERROR( maj_stat ) )
    {
	status = E_GC1104_KRB_GSS_ACCEPT_SEC_CTX;

	if ( IIgcs_global->err_log_func )
	    (*IIgcs_global->err_log_func)
	      ( E_GC1104_KRB_GSS_ACCEPT_SEC_CTX, 0, NULL );

	GSSdisplay_status( "gss_accept_sec_context", maj_stat, min_stat );

	maj_stat = gss_delete_sec_context( &min_stat, &gss_cb->gss_context,
					   &gss_cb->oToken );
	if( GSS_ERROR( min_stat ) )
	    GSSdisplay_status( "gss_delete_sec_context", maj_stat, min_stat );

	maj_stat = gss_release_buffer( &min_stat, &gss_cb->oToken );
	if ( GSS_ERROR( maj_stat ) )
	    GSSdisplay_status( "gss_release_buffer", maj_stat, min_stat );
    }
    else if ( ! gss_cb->oToken.length )
    {
	/* No token needs to be passed to the other side
	** of the application for the user and remote 
	** authentication operations. For the encode/decode operations,
	** this output buffer is released in gss_se_confirm()
	** routine.
	*/

        OM_uint32    major = 0, minor = 0;

	major = gss_release_buffer( &minor, &gss_cb->oToken );

	if ( GSS_ERROR( major ) )
	    GSSdisplay_status( "gss_release_buffer", major, minor );
    }

    /*
    ** If credentials were delegated copy it to the buffer
    ** and set the delegation flag in the control block.
    */
    if ( ret_flags & GSS_C_DELEG_FLAG )
    {
        GCS_TRACE( 3 ) 
    	    ( "GCS %s: gss_accept_sec_context, Credentials were delegated.\n",
	      mech_info.mech_name ); 
	MEcopy( (PTR)&delegatedCred, sizeof(delegatedCred),
		&gss_cb->delegated_cred );
        gss_cb->retFlags = GSS_C_DELEG_FLAG;
    }

    GCS_TRACE( 3 ) 
    	( "GCS %s: gss_accept_sec_context, status 0x%x\n",
				mech_info.mech_name, status ); 

    return( status );
}

/*
** Name: gcs_validate
**
** Description:
**      Validates the header information for a GCS object.
**
** Input:
**      length          Length of GCS object.
**      buffer          Location of GCS object.
**
** Output:
**      hdr             GCS object header.
**
** Returns
**      STATUS          OK or E_GC1011_INVALID_DATA_OBJ.
**
** History:
**	19-Mar-99 (rajus01)
**         Yanked from gcsing.c.
*/

static STATUS
gcs_validate( i4 length, PTR buffer, GCS_OBJ_HDR *hdr )
{
    u_i4 id;
    u_i2 len;

    if ( length < sizeof( *hdr ) )
    {
	GCS_TRACE( 1 )
	    ( "GCS %s: invalid object length (%d of %d)\n",
		mech_info.mech_name, length, (i4)sizeof( *hdr ) );
	return( E_GC1011_INVALID_DATA_OBJ );
     }

     MEcopy( buffer, sizeof( *hdr ), (PTR)hdr );

     if ( (id = GCS_GET_I4( hdr->gcs_id )) != GCS_OBJ_ID )
     {
	GCS_TRACE( 1 )
	    ( "GCS %s: invalid GCS id (0x%x)\n", mech_info.mech_name, id );
	return( E_GC1011_INVALID_DATA_OBJ);
     }

     if ( hdr->mech_id != GCS_MECH_KERBEROS  )
     {

	GCS_TRACE( 1 )
	    ( "GCS %s: invalid mechanism ID (%d)\n",
	      mech_info.mech_name, (i4)hdr->mech_id );
	return( E_GC1011_INVALID_DATA_OBJ );
     }

     if ( hdr->obj_ver != GCS_OBJ_V0  ||  hdr->obj_info != 0 )
     {
	 GCS_TRACE( 1 )
	     ( "GCS %s: invalid object version (%d,%d)\n",
	        mech_info.mech_name,
	        (i4)hdr->obj_ver, (i4)hdr->obj_info );
	 return( E_GC1011_INVALID_DATA_OBJ );
     }

     len = GCS_GET_I2( hdr->obj_len );

     if ( length < (sizeof( *hdr) + len))
     {
   	GCS_TRACE( 1 )
	    ( "GCS %s: insufficient data (%d of %d)\n",
	      mech_info.mech_name, length - sizeof( *hdr), (i4)len );
	return( E_GC1011_INVALID_DATA_OBJ );
     }

     return( OK );
}


/*
** Name:import_name 
**
** Description:
**      Constructs the principal name in ASCII form and converts it
**	into internal form.
**
** Input:
**      target          buffer to hold the name in internal form.
**      host          	hostname portion of the principal name
**
** Output:
**      target          Name of the principal in internal form. 
**
** Returns
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**         Created.
*/

static STATUS 
import_name( gss_name_t * target, char *host )
{
    gss_buffer_desc	inputNameBuf = GSS_C_EMPTY_BUFFER;
    OM_uint32		gMaj = 0, gMin = 0;
    gss_OID		nameOID = GSS_C_NULL_OID;
    char		buf[ 256 ];
    char		*name;

    STATUS		status = OK;

    STprintf ( buf, "%s@%s", SERVICE_NAME_PREFIX, host );
    inputNameBuf.length = STlength( buf );

    if ( ! ( name = 
	   (*IIgcs_global->mem_alloc_func) ( STlength( buf ) + 1 ) ) )
    {
	status = E_GC1013_NO_MEMORY;
   	return ( status );
    }

    STcopy ( buf, name );
    inputNameBuf.value  =  name;
		
    gMaj = gss_import_name( &gMin, &inputNameBuf, 
#if defined(NT_GENERIC) || defined(VMS)
                  (gss_OID) GSS_C_NT_HOSTBASED_SERVICE, target );
#else
                  (gss_OID) gss_nt_service_name, target );
#endif /* VMS */

    if ( GSS_ERROR( gMaj ) ) 
    {
	status = E_GC1105_KRB_GSS_IMPORT_NAME;
	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT             args[1];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)inputNameBuf.value;

	    (*IIgcs_global->err_log_func)( E_GC1105_KRB_GSS_IMPORT_NAME,
					   sizeof(args) / sizeof(args[0]),
				   	   (PTR)args );
   	}
	GSSdisplay_status( "gss_import_name", gMaj, gMin );

	(*IIgcs_global->mem_free_func)( (PTR)name );
	gMaj= gss_release_buffer( &gMin, &inputNameBuf );
	if ( GSS_ERROR( gMaj ) )
	    GSSdisplay_status( "gss_release_buffer", gMaj, gMin );

	return( status );
    }

    GCS_TRACE( 4 )
        ("GCS %s: import_name, Imported principal \"%s\".\n",
			mech_info.mech_name, inputNameBuf.value );     

    (*IIgcs_global->mem_free_func)( (PTR)name );

    return ( status );
}	


/*
** Name: gss_auth
**
** Description:
**      Produces the user authentication token.
**
** Input:
**	length		buffer length.
**      buffer          buffer to hold the token.
**      host          	hostname.
**	usr		user auth or remote auth.
**
** Output:
**      buffer          buffer holding the token 
**	length		token length.
**
** Returns
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**         Created.
*/

static STATUS
gss_auth( PTR parms, bool usr )
{
    GCS_OBJ_HDR 	hdr;
    GSS_CB		cb; 
    STATUS 		status = OK;
    GCS_USR_PARM        *usr_parm;
    GCS_REM_PARM        *rem;
    char       		*ptr;
    i4			*length;
    char		*host = fqdn_str;

    cb.gss_context = NULL;
    cb.target_name = NULL;
    cb.client = NULL;
    cb.tToken= NULL;
    cb.delegated_cred = NULL;

    if ( usr )
    {
	usr_parm = ( GCS_USR_PARM * ) parms;
	ptr = (char *)usr_parm->buffer;
	length = &usr_parm->length;
	if( delegate & GSS_C_DELEG_FLAG )
           cb.reqFlags = GSS_C_DELEG_FLAG;
    }
    else
    {
	rem = ( GCS_REM_PARM * )parms;
	ptr = (char *)rem->buffer;
	length = &rem->size;
	host = rem->host;
	cb.reqFlags = 0;

	if( rem->length > 0 )
	    if( ( status = gss_val_delegatedCred( &cb, rem->token,
						&rem->length ) ) !=OK )
		return( status );
    }

    if ( ( status = import_name( &cb.target_name, host ) ) != OK )
	return( status );

    status = gss_init( &cb );

    if ( status != OK  &&  usr && ( cb.reqFlags & GSS_C_DELEG_FLAG) )
    {
	/*
	** Try without delegation flag set.
	** If it fails again,then return the error.
	*/
	cb.reqFlags = 0;
	cb.gss_context = NULL;
	cb.target_name = NULL;
	cb.client = NULL;
	cb.tToken= NULL;
	cb.delegated_cred = NULL;

	if ( ( status = import_name( &cb.target_name, host ) ) == OK )
	    status = gss_init( &cb );
    }

    if ( status != OK )  return( status );

    if ( *length < (sizeof( hdr ) + cb.iToken.length ) )
    {
	GCS_TRACE( 1 )
	    ( "GCS %s: insufficient buffer (%d of %d)\n",
	      mech_info.mech_name, *length,
	      (i4)sizeof( hdr ) + cb.iToken.length );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_KERBEROS;

    hdr.obj_id = usr ? GCS_USR_AUTH : GCS_REM_AUTH ;

    hdr.obj_ver = GCS_OBJ_V0;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, cb.iToken.length );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ptr );
    MEcopy( (PTR)cb.iToken.value, cb.iToken.length, (PTR)(ptr + sizeof(hdr)) );
    *length = sizeof( hdr ) + cb.iToken.length;

    GCS_TRACE( 3 ) 
	( "GCS %s: gss_auth, %s, length %d\n",
	  mech_info.mech_name, usr ? "GCS_USR_AUTH" : "GCS_REM_AUTH", *length );

    status = gss_term( &cb );

    return( status );
}


/*
** Name: gss_val_auth
**
** Description:
**      Validates a GCS_USR_AUTH, GCS_REM_AUTH objects. 
**      Client user ID must match validation user ID and alias
**	for user auth. Client user ID must match validation alias
**	for remote auth.
**
** Input:
**      valid           GCS_OP_VALIDATE parameters.
**	bool		user_auth object or rem_auth object?
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**         Created.
*/

static STATUS
gss_val_auth( GCS_VALID_PARM *valid, bool usr )
{
    char		*p;
    GSS_CB		cb;
    OM_uint32 		gMaj=0, gMin=0;
    gss_buffer_desc	outputNameBuf = GSS_C_EMPTY_BUFFER;
    gss_OID		nameOID = GSS_C_NULL_OID;
    GCS_OBJ_HDR		hdr;
    u_i4		len;

    STATUS		status = OK;

    cb.delegated_cred = NULL;
    cb.retFlags	      = 0;

    MEcopy( valid->auth, sizeof( hdr ), (PTR)&hdr );

    len = GCS_GET_I2( hdr.obj_len );

    if ( ( status = gss_acquire () ) != OK )
       return( status );

    cb.iToken.value  = valid->auth + sizeof( hdr );
    cb.iToken.length = len;

    if ( ( status = gss_accept( &cb ) ) != OK )
        return( status );

    if ( cb.retFlags & GSS_C_DELEG_FLAG )
    {
	GCS_OBJ_HDR	hdr;
	u_i4		len;

	GCS_TRACE( 1 )
	    ( "GCS %s: Delegation credentials buffer length %d\n",
	      mech_info.mech_name, valid->size );

	if ( valid->size < sizeof( hdr ) + sizeof( cb.delegated_cred ) )
	{
	    GCS_TRACE( 1 )
		( "GCS %s: insufficient buffer (%d of %d)\n",
		  mech_info.mech_name, valid->size,
		  (i4)sizeof( hdr ) + sizeof(cb.delegated_cred) );
	    return( E_GC1010_INSUFFICIENT_BUFFER );
	}
	GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
	hdr.mech_id = GCS_MECH_KERBEROS;
	hdr.obj_id = GCS_DELEGATE;
	hdr.obj_ver = GCS_OBJ_V0;
	hdr.obj_info = 0;
	GCS_PUT_I2( hdr.obj_len, sizeof( cb.delegated_cred ) );
	MEcopy( (PTR)&hdr, sizeof( hdr ), valid->buffer );
	MEcopy( (PTR)&(cb.delegated_cred), (u_i2)sizeof(cb.delegated_cred),
		(PTR)((char *)valid->buffer + sizeof( hdr )) );
	valid->size = sizeof( hdr ) + sizeof( cb.delegated_cred );
    }
    else
	valid->size = 0;

    gMaj = gss_display_name( &gMin, cb.client, &outputNameBuf, &nameOID );

    if ( GSS_ERROR( gMaj ) ) 
    {
	status = E_GC1106_KRB_GSS_DISPLAY_NAME;
	if ( IIgcs_global->err_log_func )
	    (*IIgcs_global->err_log_func)
		    ( E_GC1106_KRB_GSS_DISPLAY_NAME, 0, NULL );
	GSSdisplay_status( "gss_display_name", gMaj, gMin );
        gMaj= gss_release_buffer( &gMin, &outputNameBuf );
       	if ( GSS_ERROR( gMaj ) )
	    GSSdisplay_status( "gss_release_buffer", gMaj, gMin );

	return( status );
    }

    if ( p = STindex ( (char*)outputNameBuf.value, "@", 0 ) )  *p = '\0';

    GCS_TRACE( 4 )
	( "GCS %s: auth %s, user %s, alias %s\n",
	  mech_info.mech_name, (char*)outputNameBuf.value,
	  valid->user, valid->alias );
    if ( usr )
    {
        if ( STcompare( (char*)outputNameBuf.value, valid->user ) )
        {
	    GCS_TRACE( 1 )
		( "GCS %s: gss_val_auth, user %s auth failed: %s\n",
		  mech_info.mech_name, (char*)outputNameBuf.value,
		  valid->user );
	    return( E_GC1008_INVALID_USER);
	}
    }

    if ( STcompare( (char*)outputNameBuf.value, valid->alias ) )
    {
 	GCS_TRACE( 1 )
	    ( "GCS %s: gss_val_auth, user %s can not be %s\n",
	      mech_info.mech_name, (char*)outputNameBuf.value, valid->alias );
	return( E_GC1008_INVALID_USER );
    }

    GCS_TRACE( 3 )
        ( "GCS %s: gss_val_auth, user %s is validated, status 0x%x\n",
	  mech_info.mech_name, (char*)outputNameBuf.value, status );

    gMaj= gss_release_buffer( &gMin, &outputNameBuf );
    if ( GSS_ERROR( gMaj ) )
	GSSdisplay_status( "gss_release_buffer", gMaj, gMin );

    return ( status );
}


/*
** Name: gss_e_init
**
** Description:
**      Create a GCS_E_INIT object. 
**
** Input:
**      initParm        GCS_OP_E_INIT parameters.
**      gss_cb          GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code
**
** History:
**      19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS 
gss_e_init( GCS_EINIT_PARM *initParm, GSS_CB *gss_cb )
{
    GCS_OBJ_HDR		hdr;
    STATUS		status = OK;

    gss_cb->target_name = NULL;
    gss_cb->reqFlags 	= GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG;
    gss_cb->tToken 	= NULL;
    gss_cb->delegated_cred = NULL;

    GCS_TRACE( 3 ) 
	( "GCS %s: gss_e_init, peer %s\n",
	  mech_info.mech_name, initParm->peer );

    if ( ( status = import_name( &gss_cb->target_name, initParm->peer ) )
								  != OK )
	return( status );

    if ( ( status = gss_init( gss_cb ) ) != OK )
	return( status );

    if ( ( sizeof( hdr ) + gss_cb->iToken.length )  > initParm->length )
    {
	GCS_TRACE( 1 ) 
	    ( "GCS %s: insufficient buffer (%d of %d)\n",
	      mech_info.mech_name, initParm->length,
	      (i4) ( sizeof( hdr ) + gss_cb->iToken.length ) );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_KERBEROS;
    hdr.obj_id = GCS_E_INIT;
    hdr.obj_ver = GCS_OBJ_V0;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, gss_cb->iToken.length );
    MEcopy( (PTR)&hdr, sizeof( hdr ), initParm->buffer );
    initParm->buffer += sizeof( hdr );
    MEcopy( (PTR)gss_cb->iToken.value, (u_i2)gss_cb->iToken.length,
						(PTR)initParm->buffer );
    initParm->length = sizeof( hdr ) + gss_cb->iToken.length;

    GCS_TRACE( 4 )
	( "GCS %s: gss_e_init, Token length %d, status 0x%x\n",
	  mech_info.mech_name, initParm->length, status );

   return( status);
}


/*
** Name: gss_e_accept
**
** Description:
**      A GCS_E_INIT object is processed and a security context is
**	is established. During this process an output token is produced
**	which is passed to the initiator of security context.
**
** Input:
**      initParm        GCS_OP_E_INIT parameters.
**      hdr             GCS_E_INIT object header.
**      gss_cb          GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS 
gss_e_accept( GCS_EINIT_PARM *initParm,
	      GCS_OBJ_HDR *hdr,
	      GSS_CB *gss_cb )
{
    u_i2	len;
    STATUS	status = OK;

    if ( ( status = gss_acquire() ) != OK )
	return( status );
    
    len = GCS_GET_I2( hdr->obj_len);

    if ( hdr->obj_id != GCS_E_INIT )
    {
 	GCS_TRACE( 1 )
	    ( "GCS %s: invalid object ID (%d)\n",
	      mech_info.mech_name, (i4)hdr->obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    initParm->buffer += sizeof( *hdr );
    gss_cb->iToken.value = initParm->buffer;
    gss_cb->iToken.length = len;
    gss_cb->retFlags      = 0;

    GCS_TRACE( 4 )
    	( "GCS %s: gss_e_accept, input token length %d\n",
	  mech_info.mech_name, gss_cb->iToken.length );

    if( ( status = gss_accept( gss_cb ) ) != OK )
        return( status );

    GCS_TRACE( 4 )
	( "GCS %s: gss_e_accept, output token length %d\n",
	  mech_info.mech_name, gss_cb->oToken.length );

    return( status );
}

/*
** Name: gss_ce_confirm - Client side GCS_E_CONFIRM object processing.
**
** Description:
**	Output token produced by gss_accept_sec_context() on the server,
**	is passed to gss_init_sec_context() to complete the mutual
**	authentication between the Comm servers.
**
** Input:
**      cnfParm        GCS_OP_E_CONFIRM parameters.
**      gss_cb         GSS control block.
**      hdr            GCS_E_CONFIRM object header.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**	   Created.
*/

static STATUS
gss_ce_confirm( GCS_ECONF_PARM *cnfParm,
		GSS_CB *gss_cb,
		GCS_OBJ_HDR *obj_hdr )
{
    u_i2	len;
    STATUS	status = OK;
    gss_cb->delegated_cred = NULL;

    len = GCS_GET_I2( obj_hdr->obj_len );

    GCS_TRACE( 3 ) 
	( "GCS %s: validating %s (%d bytes)\n", mech_info.mech_name,
	  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, obj_hdr->obj_id ),
	  (i4)len );

    if ( obj_hdr->obj_id != GCS_E_CONFIRM)
    {
 	GCS_TRACE( 1 )
	    ( "GCS %s: invalid object ID (%d)\n",
	      mech_info.mech_name, (i4)obj_hdr->obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    cnfParm->buffer += sizeof( *obj_hdr );

    gss_cb->oToken.value = cnfParm->buffer;
    gss_cb->oToken.length = len;
    gss_cb->tToken = &gss_cb->oToken;

    if( ( status = gss_init( gss_cb ) != OK ) )
    	return( status );

    return( status );
}



/*
** Name: gss_se_confirm 
**
** Description:
**	Constructs a GCS_E_CONFIRM object.
**
** Input:
**      cnfParm        GCS_OP_E_CONFIRM parameters.
**      gss_cb         GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**      19-Mar-99 (rajus01)
**	   Created.
*/
static STATUS
gss_se_confirm( GCS_ECONF_PARM *cnfParm, GSS_CB *gss_cb )
{
    GCS_OBJ_HDR	hdr;
    OM_uint32	major = 0, minor = 0;

    STATUS	status = OK;

    if ( cnfParm->length < ( sizeof( hdr ) + gss_cb->oToken.length ) )
    { 
	GCS_TRACE( 1 )
	    ( "GCS %s: insufficient buffer (%d of %d)\n",
	      mech_info.mech_name, cnfParm->length,
	      (i4)( sizeof( hdr ) + gss_cb->oToken.length ) );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_KERBEROS;
    hdr.obj_id = GCS_E_CONFIRM;
    hdr.obj_ver = GCS_OBJ_V0;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, gss_cb->oToken.length );
    MEcopy( (PTR)&hdr, sizeof( hdr), cnfParm->buffer);
    cnfParm->buffer += sizeof( hdr );
    MEcopy( (PTR)gss_cb->oToken.value,
	    (u_i2)gss_cb->oToken.length, (PTR)cnfParm->buffer);
    cnfParm->length = sizeof( hdr ) + gss_cb->oToken.length;

    /* Release the output token buffer from accept_sec_context */

    major = gss_release_buffer( &minor, &gss_cb->oToken );
    if ( GSS_ERROR( major ) )
	GSSdisplay_status( "gss_release_buffer", major, minor );

    GCS_TRACE( 3 )
    	( "GCS %s: gss_se_confirm, token length %d\n", mech_info.mech_name, 
	  gss_cb->oToken.length );

    return( status );
}

/*
** Name: gss_e_encode 
**
** Description:
**	Constructs a GCS_E_DATA encrypted object.
**
** Input:
**      ecdParm        GCS_OP_E_ENCODE parameters.
**      gss_cb         GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**	19-Mar-99 ( rajus01 )
**	   Created.
**      10-feb-2003 (chash01)
**         use type int not OM_uint32 for theConfState.
*/
static STATUS
gss_e_encode( GCS_EENC_PARM *ecdParm, GSS_CB *gss_cb )
{
    gss_buffer_desc	in_buf, out_buf;
    OM_uint32 		maj_stat, min_stat;
    int			theConfState;
    GCS_OBJ_HDR		hdr;

    STATUS	status = OK;

    GCS_TRACE( 3 ) 
	( "GCS %s: encoding %d bytes (%d bytes of expansion)\n",
	  mech_info.mech_name, ecdParm->length,
	  ecdParm->size - ecdParm->length );

    in_buf.value = ecdParm->buffer;
    in_buf.length = ecdParm->length;

    maj_stat = gss_wrap( &min_stat,
			 gss_cb->gss_context, TRUE,
			 GSS_C_QOP_DEFAULT,
			 &in_buf,
			 &theConfState,
			 &out_buf );

    if ( GSS_ERROR( maj_stat ) )
    {
	status = E_GC1107_KRB_GSS_ENCRYPT;
	if ( IIgcs_global->err_log_func )
	    (*IIgcs_global->err_log_func)
		    ( E_GC1107_KRB_GSS_ENCRYPT, 0, NULL );
	GSSdisplay_status( "gss_wrap", maj_stat, min_stat );
       	gss_release_buffer( &min_stat, &out_buf );
	return( status );
    }

    if ( ecdParm->size < ( sizeof(hdr) + out_buf.length ) )
    {
        GCS_TRACE( 3 )
	    ( "GCS %s: gss_e_encode, ( %d of %d ): insufficient buffer\n",
	      mech_info.mech_name, ecdParm->size,
	      (i4) ( sizeof( hdr ) + out_buf.length ) );
	status = E_GC1010_INSUFFICIENT_BUFFER;
	return( status );
    }

    GCS_PUT_I4( hdr.gcs_id, GCS_OBJ_ID );
    hdr.mech_id = GCS_MECH_KERBEROS;
    hdr.obj_id = GCS_E_DATA;
    hdr.obj_ver = GCS_OBJ_V0;
    hdr.obj_info = 0;
    GCS_PUT_I2( hdr.obj_len, out_buf.length );
    MEcopy( (PTR)&hdr, sizeof( hdr ), (PTR)ecdParm->buffer );
    ecdParm->buffer += sizeof( hdr );
    MEcopy((PTR)out_buf.value, (u_i2)out_buf.length,
				(PTR)ecdParm->buffer );
    ecdParm->length = sizeof( hdr ) + out_buf.length;

    GCS_TRACE( 3 )
    	( "GCS %s: gss_e_encode, encrypt length %d, status 0x%x\n",
	  mech_info.mech_name, ecdParm->length, status );

    return( status );
}

/*
** Name: gss_e_decode 
**
** Description:
**	Process, Decode a GCS_E_DATA encrypted object and create
**	a GCS_E_DATA decrypted object.
**
** Input:
**      dcdParm        GCS_OP_E_DECODE parameters.
**	hdr            GCS_E_DATA object header.
**      gss_cb         GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**	19-Mar-99 ( rajus01 )
**	   Created.
**	17-Apr-00 (rajus01)
**	   Adjust the length of buffer to be decoded.
**      10-feb-2003 (chash01)
**         use type int, not OM_uint32, for conf_state;
*/

static STATUS 
gss_e_decode( GCS_EDEC_PARM *dcdParm,
	      GCS_OBJ_HDR *hdr,
	      GSS_CB *gss_cb )
{
    gss_buffer_desc	xmit_buf, msg_buf;
    OM_uint32 		maj_stat, min_stat;
    OM_uint32		state;
    int		        conf_state;
    u_i2 		len;

    STATUS 		status = OK;

    len = GCS_GET_I2( hdr->obj_len);

    GCS_TRACE( 3 )
	( "GCS %s: gss_e_decode, validating %s (%d bytes)\n",
	  mech_info.mech_name,
	 (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, hdr->obj_id ),
	 (i4)len );

    if ( hdr->obj_id != GCS_E_DATA )
    {
	GCS_TRACE( 1 )
	    ( "GCS %s: invalid object ID (%d)\n",
	      mech_info.mech_name, (i4)hdr->obj_id );
        return( E_GC1011_INVALID_DATA_OBJ );
    }

    dcdParm->buffer += sizeof( *hdr );
    dcdParm->length -= sizeof( *hdr );

    GCS_TRACE( 4 )
	( "GCS %s: decoding %d bytes\n", mech_info.mech_name,
	  dcdParm->length );

    xmit_buf.value  =	dcdParm->buffer;
    xmit_buf.length =	dcdParm->length;

    maj_stat = gss_unwrap( &min_stat,
			   gss_cb->gss_context, 
			   &xmit_buf,
			   &msg_buf,
			   &conf_state,
			   &state);

    if ( GSS_ERROR( maj_stat ) )
    {
	status = E_GC1108_KRB_GSS_DECRYPT;
	if ( IIgcs_global->err_log_func )
	    (*IIgcs_global->err_log_func)
		    ( E_GC1108_KRB_GSS_DECRYPT, 0, NULL );
	GSSdisplay_status( "gss_unwrap", maj_stat, min_stat );
        gss_release_buffer( &min_stat, &msg_buf );
	return( status );
    }

    if( dcdParm->size < msg_buf.length )
    {
    	GCS_TRACE( 1 ) 
	    ( "GCS %s: ( %d of %d ): insufficient buffer\n",
	      mech_info.mech_name, dcdParm->size, msg_buf.length );
	return( E_GC1010_INSUFFICIENT_BUFFER );
    }

    (VOID)MEcopy( (PTR)msg_buf.value, (u_i2)msg_buf.length,
			(PTR)( dcdParm->buffer - sizeof(*hdr) ) );
    dcdParm->length = msg_buf.length; 

    GCS_TRACE( 4 )
    	( "GCS %s: gss_e_decode, msg length %d\n", mech_info.mech_name, 
	  dcdParm->length );

    return( status );
}


/*
** Name: gss_term 
**
** Description:
**	Free the memory allocated for token, name buffer in GSS control block.
**
** Input:
**      gss_cb         GSS control block.
**
** Output:
**      None.
**
** Returns:
**      STATUS          OK or error code.
**
** History:
**	19-Mar-99 ( rajus01 )
**	   Created.
*/
static STATUS 
gss_term( GSS_CB *gss_cb )
{
	
    STATUS	status = OK;
    OM_uint32 	maj_stat, min_stat;

    if ( gss_cb->iToken.length )
    {
	maj_stat = gss_release_buffer( &min_stat, &gss_cb->iToken );
	if ( GSS_ERROR( maj_stat ) )
	   GSSdisplay_status( "gss_release_buffer", maj_stat, min_stat );
    }

    maj_stat = gss_release_name( &min_stat, &gss_cb->target_name );
    if ( GSS_ERROR( maj_stat ) )
	GSSdisplay_status( "gss_release_name", maj_stat, min_stat );

    return( status );
}

/*
** Name: gss_val_delegatedCred 
**
** Description:
**	Validate credentials.
**
** Input:
**      cb         GSS control block.
**
**	token	   Credentials to be validated.
**
**	length	   Credential length;
**
** Output:
**      None.
**
** Returns:
**      STATUS     OK or error code.
**
** History:
**	19-Mar-99 ( rajus01 )
**	    Created.
**      10-feb-2003 (chash01)
**          change type OM_uint32 to gss_cred_usage_t for cred_usage.
*/

static STATUS
gss_val_delegatedCred( GSS_CB *cb, PTR token, i4 *length )
{
    GCS_OBJ_HDR		hdr;
    u_i2		len;
    OM_uint32		gMaj = 0, gMin = 0;
    gss_name_t		name;
    OM_uint32		lifetime;
    gss_cred_usage_t	cred_usage;
    gss_OID_set		vMechSet = GSS_C_NULL_OID_SET;
    gss_buffer_desc	outputNameBuf = GSS_C_EMPTY_BUFFER;
    gss_OID		nameOID = GSS_C_NULL_OID;
    gss_cred_id_t	cred;
    char		*p;
    STATUS		status = OK;

    status = gcs_validate( *length, token, &hdr );
    if ( status != OK )  return( status );

    len = GCS_GET_I2( hdr.obj_len );

    if ( hdr.obj_id != GCS_DELEGATE )
    {
 	GCS_TRACE( 1 )
	    ( "GCS %s: invalid object ID (%d)\n",
	      mech_info.mech_name, (i4)hdr.obj_id );
	return( E_GC1011_INVALID_DATA_OBJ );
    }

    token += sizeof( hdr );
    *length = len;

    GCS_TRACE( 4 ) 
	( "GCS %s: validating %s (%d bytes)\n", mech_info.mech_name,
	  (*IIgcs_global->tr_lookup)(IIgcs_global->tr_objs, hdr.obj_id),
	  (i4)len );

    MEcopy( (PTR)token, len, &cred );

    gMaj = gss_inquire_cred( &gMin, cred, &name, &lifetime,
			     &cred_usage, &vMechSet );
    if ( ! GSS_ERROR( gMaj ) )
    {
	gMaj = gss_display_name(&gMin, name, &outputNameBuf, &nameOID);

	if( GSS_ERROR( gMaj ) )
	{
	    status = E_GC1106_KRB_GSS_DISPLAY_NAME;
	    if ( IIgcs_global->err_log_func )
	    	(*IIgcs_global->err_log_func)
		    ( E_GC1106_KRB_GSS_DISPLAY_NAME, 0, NULL );
	    GSSdisplay_status( "gss_display_name", gMaj, gMin );
	}
    }
    else
    {
	status = E_GC1109_KRB_GSS_INQUIRE_CRED;
	if ( IIgcs_global->err_log_func )
	     (*IIgcs_global->err_log_func)
		    ( E_GC1109_KRB_GSS_INQUIRE_CRED, 0, NULL );
	GSSdisplay_status( "gss_inquire_cred", gMaj, gMin );
    }

    if ( p = STindex ( (char*)outputNameBuf.value, "@", 0 ) )  *p = '\0';

    GCS_TRACE( 1 )
        ( "GCS %s: Delegated client name %s\n", mech_info.mech_name,
	  (char *)outputNameBuf.value );

    MEcopy( (PTR)token, len, &cb->delegated_cred );

    return( status );
}

/*
** Name: GSSdisplay_status 
**
** Description:
**      Displays GSS-API messages.
**
** Input:
**	 msg             A string to be displayed with the message
**       maj_stat        The GSS-API major status code
**       min_stat        The GSS-API minor status code
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**	19-Mar-99 ( rajus01 )
**	    Created.
*/

static VOID
GSSdisplay_status( char *msg, OM_uint32 maj_stat,
		   OM_uint32 min_stat 
		 )
{
      GSSdisplay_status_1(msg, maj_stat, GSS_C_GSS_CODE);
      GSSdisplay_status_1(msg, min_stat, GSS_C_MECH_CODE);
}

static
VOID GSSdisplay_status_1( char *m, OM_uint32 code, i4 type)
{
    OM_uint32 		maj_stat, min_stat;
    gss_buffer_desc 	msg;
    OM_uint32 		msg_ctx;
    ER_ARGUMENT         args[2];

    msg_ctx = 0;
    args[0].er_size = ER_PTR_ARGUMENT;
    args[1].er_size = ER_PTR_ARGUMENT;

    while ( 1 ) 
    {
	maj_stat = gss_display_status( &min_stat, code,
				       type, GSS_C_NULL_OID,
				       &msg_ctx, &msg );
	GCS_TRACE( 1 )
	    ( "GSS-API error %s: %s\n", m, (char *)msg.value );

        if (!GSS_ERROR(maj_stat))
        {
            args[0].er_value = (PTR)&code;
            args[1].er_value = (PTR)msg.value;
            if ( IIgcs_global->err_log_func )
                (*IIgcs_global->err_log_func)
                    ( E_GC1200_KRB_GSS_API_ERR,
                        sizeof(args) / sizeof(args[0]), (PTR)args );
        }

	(VOID) gss_release_buffer( &min_stat, &msg );

	if ( ! msg_ctx )
	    break;
    }
}

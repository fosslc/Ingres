/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <erglf.h>
#include <gl.h>
#include <cv.h>
#include <er.h>
#include <gc.h>
#include <id.h>
#include <lo.h>
#include <dl.h>
#include <me.h>
#include <mu.h>
#include <nm.h>
#include <pm.h>
#include <sp.h>
#include <st.h>
#include <mo.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcxdebug.h>
#include <gcs.h>
#include "gcsint.h"

/*
** Name: gcscall.c
**
** Description:
**	Main entry point and generic processing for GCS.
**
** History:
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Added support for dynamically loading of mechanisms.
**	20-Aug-97 (gordy)
**	    Added capability to load mechanisms specific to a server.
**	    Other minor changes were made to the config settings.
**	 5-Sep-97 (gordy)
**	    Added ID to GCS objects.
**	22-Oct-97 (rajus01)
**	    Added encryption overhead for mechanism processing.
**	20-Jan-98 (gordy)
**	    Added configuration lookup function in global structure.
**	20-Feb-98 (gordy)
**	    Defined MO objects.
**	17-Mar-98 (gordy)
**	    Use GCusername() for effective user ID.  Use system prefix
**	    for PM symbols.  Added ability to specify library path for
**	    each mechanism.
**	20-Mar-98 (gordy)
**	    Added mechanism encryption level.
**	15-May-98 (gordy)
**	    Made the GCS control block global for access by utility
**	    functions (should not be accessed directly by mechanisms).
**	10-Jul-98 (gordy)
**	    Support 'none' in mechanism list so server specific lists can
**	    be set to override the default list and load no mechanisms.
**	26-Jul-98 (gordy)
**	    Changed config disabled param to enabled.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	 6-Nov-99 (gordy)
**	    Map CL GC error codes to GCA error codes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-sep-2000 (somsa01)
**	    PMinit() can return either OK or PM_NO_INIT.
**	22-sep-2000 (somsa01)
**	    Amended previous change with proper multiple status check.
**      17-jan-2002 (loera01) SIR 106858
**          Added GCS global config_host for searching config.dat.
**	 5-May-03 (gordy)
**	    Save GCusername() reference in globals.
**	 9-Jul-04 (gordy)
**	    Expanded configuration with individual authentication mechanisms.
**      31-Oct-1007 (Ralph Loen) Bug 119358
**          Added E_GC1028_GCF_MECH_UNSUPPORTED and E_GC1027_GCF_MECH_DISABLED
**          to log detail for unsupported or disabled mechanisms.  Return
**          and log error code from gcs_init_mechs() if a configured mechanism
**          is disabled or unsupported.
**      01-Nov-2007 (Ralph Loen) Bug 119358
**          In gcs_init_mechs(), don't invoke gcs_check_caps() if the 
**          configured mechanism is GCS_MECH_NONE for password and remote 
**          authorization.  In these cases, the appropriate internal mechanism 
**          is substituted at a later time.
**      17-Dec-2007 (Ralph Loen) Bug 119621
**          In gcs_init_mechs(), when initializing remote authorization via 
**          gcs_config_mech(), the default mechanism argument should be 
**          GCS_NO_MECH, not IIgcs_global->install_mech.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

/*
** Forward references.
*/
static STATUS	gcs_init( GCS_INIT_PARM * );
static STATUS	gcs_term( VOID );
static STATUS	gcs_cntrl( i4, PTR );
static STATUS	gcs_init_mechs( VOID );
static STATUS	gcs_load_mechs( VOID );
static STATUS	gcs_mech_load( char *, char *, LOCATION *, char * );
static STATUS	gcs_mech_init( char *, MECH_FUNC, PTR );
static STATUS	gcs_call_mech( GCS_MECH, i4, PTR );
static STATUS	gcs_call_caps( i4, PTR );
static STATUS	gcs_config_mech( char *, GCS_MECH, GCS_MECH * );
static STATUS	gcs_check_caps( GCS_MECH, u_i4 );
static PTR	gcs_alloc( u_i4 );
static VOID	gcs_free( PTR );

/*
** Dummy external info for generic processing mechanism.
*/

static GCS_MECH_INFO gcs_info = 
{ "<internal>", GCS_NO_MECH, GCS_MECH_VERSION_2, 0, GCS_AVAIL, 0 };


/*
** Static mechanism info.
*/

static struct
{

    char	*name;
    MECH_FUNC	func;

} gcs_mech_info[] =
{
    { "null",	gcs_null }, 
    { "system",	gcs_system }, 
    { "ingres",	gcs_ingres }
};

#define	MECH_COUNT	(sizeof( gcs_mech_info ) / sizeof( gcs_mech_info[0] ))


/*
** GCS control block.
*/

static		GCS_GLOBAL	gcs_global;
GLOBALREF	GCS_GLOBAL	*IIgcs_global;


GLOBALREF	GCXLIST		gcs_tr_ops[];		/* OP code tracing */
GLOBALREF	GCXLIST		gcs_tr_objs[];		/* Object tracing */
GLOBALREF	GCXLIST		gcs_tr_parms[];		/* Set parm tracing */

static		MO_CLASS_DEF	gcs_classes[] =
{
    {
	0,
	GCS_MIB_DEFAULT_MECH,
	MO_SIZEOF_MEMBER( GCS_GLOBAL, default_mech ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCS_GLOBAL, default_mech ),
	MOintget,
	MOintset,
	(PTR)&gcs_global,
	MOcdata_index
    },
    {
	0,
	GCS_MIB_INSTALL_MECH,
	MO_SIZEOF_MEMBER( GCS_GLOBAL, install_mech ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCS_GLOBAL, install_mech ),
	MOintget,
	MOintset,
	(PTR)&gcs_global,
	MOcdata_index
    },
    {
	0,
	GCS_MIB_TRACE_LEVEL,
	MO_SIZEOF_MEMBER( GCS_GLOBAL, gcs_trace_level ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCS_GLOBAL, gcs_trace_level ),
	MOintget,
	MOintset,
	(PTR)&gcs_global,
	MOcdata_index
    },
    {
	0,
	GCS_MIB_VERSION,
	MO_SIZEOF_MEMBER( GCS_GLOBAL, version ),
	MO_READ,
	0,
	CL_OFFSETOF( GCS_GLOBAL, version ),
	MOintget,
	MOnoset,
	(PTR)&gcs_global,
	MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCS_MIB_MECHANISM,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_id ),
	MO_READ,
	0,
	CL_OFFSETOF( GCS_MECH_INFO, mech_id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_CAPS,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_caps ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_caps ),
	MOuintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_ELVL,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_enc_lvl ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_enc_lvl ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_NAME,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_name ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_name ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_OVHD,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_ovhd ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_ovhd ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_STATUS,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_status ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_status ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCS_MIB_MECH_VERSION,
	MO_SIZEOF_MEMBER( GCS_MECH_INFO, mech_ver ),
	MO_READ,
	GCS_MIB_MECHANISM,
	CL_OFFSETOF( GCS_MECH_INFO, mech_ver ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    { 0 }
};



/*
** Name: IIgcs_call
**
** Description:
**	Main entry point for GCS requests.  Dispatchs request
**	to security mechanisms.
**
** Input:
**	op		Operation.
**	mech		Mechanism ID.
**	parms		Parameter for operation.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Global intitialization now done on first request.
**	 6-Nov-99 (gordy)
**	    Map CL GC error codes to GCA error codes.
*/

STATUS
IIgcs_call( i4  op, GCS_MECH mech, PTR parms )
{
    STATUS status = OK;

    /*
    ** Initialize first time through.
    */
    if ( ! IIgcs_global )
	if ( op == GCS_OP_INIT  &&  mech == GCS_NO_MECH )
	    status = gcs_init( (GCS_INIT_PARM *)parms );
	else
	    status = E_GC1001_GCS_NOT_INITIALIZED;

    GCS_TRACE( 2 )
	( "GCS call: %s, mechanism %s\n",
	  (*IIgcs_global->tr_lookup)( gcs_tr_ops, op ), GCS_TR_MECH( mech ) );

    /*
    ** Make mechanism call if available.
    */
    if ( status == OK )  status = gcs_call_mech( mech, op, parms );

    /*
    ** Map GC CL errors into GCA errors.  Ugly.
    ** This relies on gca.h and gc.h remaining in sync.
    */
    if ( ( status & ~0xFF ) == ( E_CL_MASK | E_GC_MASK ) )
	status ^= ( E_CL_MASK | E_GC_MASK ) ^ E_GCF_MASK;

    GCS_TRACE( 2 )
	( "GCS call: %s, status 0x%x\n", 
	  (*IIgcs_global->tr_lookup)( gcs_tr_ops, op ), status );

    /*
    ** Clean-up on last request or initialization failure.
    */
    if ( IIgcs_global  &&  !  IIgcs_global->initialized )
	IIgcs_global = NULL;

    return( status );
}


/*
** Name: gcs_cntrl
**
** Description:
**	Mechanism entry point for GCS_NO_MECH.  Here we attempt
**	to do the right thing for callers who have no mechanism
**	preference.  This is the preferred handler for INIT, TERM
**	and INFO requests.  For VALIDATE requests, we extract the
**	mechanism from the authentication object and pass on the
**	request to the object owner.  For AUTH requests, we attempt
**	to satisfy the request through the installation mechanism
**	(if mechanism supports the capability) or the general
**	default mechanism (GCS_MECH_INGRES).  There is no current
**	handling for ENCRYPT requests.
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
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Parameters added to GCS_OP_INIT.  Global initialization
**	    now done in IIgcs_call() (mechanism init still done here).
**	 5-Sep-97 (gordy)
**	    Added ID to GCS objects.
**	 4-Dec-97 (rajus01)
**	    Added GCS_PARM_IP_FUNC case in GCS_OP_SET operation for
**	    installation passwords handling.
**	 4-Sep-98 (gordy)
**	    Added RELEASE operation.
**	 9-Jul-04 (gordy)
**	    IP auths added as distinct operation.  Check for restricted
**	    validations with configured mechnisms.  Capability no longer 
**	    needed as parameter to gcs_call_caps().
*/

static STATUS
gcs_cntrl( i4  op, PTR parms )
{
    STATUS status = OK;

    switch( op )
    {
    case GCS_OP_INIT :
    {
	GCS_INIT_PARM	*init_parm = (GCS_INIT_PARM *)parms;

	switch( init_parm->version )
	{
	case GCS_API_VERSION_1 :
	case GCS_API_VERSION_2 :
	    /*
	    ** Only init mechanisms on first time through.
	    */
	    if ( ! IIgcs_global->initialized++ )  
	    {
		status = gcs_init_mechs();

		if ( status != OK )  
		{
	            GCS_TRACE( 1 )
	   	       ( "GCS init: mechanism init failure, status %x\n",
                           status );
                    /*
                    ** If any mechanism fails, be sure to
                    ** terminate initialized mechanisms.
                    */
                    gcs_term();
                    IIgcs_global->initialized--;
                } /* if ( status != OK ) */
            } /* if ( ! IIgcs_global->initialized++ )  */
	    break;

	default :
	    status = E_GC1012_INVALID_PARM_VALUE;
	    break;
	}

	break;
    }

    case GCS_OP_TERM :
	/*
	** Do full term on last request.
	*/
	if ( IIgcs_global->initialized  &&  ! --IIgcs_global->initialized )
	    status = gcs_term();
	break;

    case GCS_OP_INFO :
    {
	GCS_INFO_PARM	*info_parm = (GCS_INFO_PARM *)parms;

	info_parm->mech_count = IIgcs_global->mech_cnt;
	info_parm->mech_info = IIgcs_global->mechanisms;
	break;
    }

    case GCS_OP_SET :
    {
	GCS_SET_PARM	*set = (GCS_SET_PARM *)parms;
	PTR		ptr;

	GCS_TRACE( 2 )("GCS set: parameter %s\n",
		       (*IIgcs_global->tr_lookup)(gcs_tr_parms, set->parm_id));

	switch( set->parm_id )
	{
	case GCS_PARM_GET_KEY_FUNC :
	    if ( set->length != sizeof( PTR ) )
		status = E_GC1012_INVALID_PARM_VALUE;
	    else
	    {
		MEcopy( set->buffer, sizeof( PTR ), &ptr );
		IIgcs_global->get_key_func = (GET_KEY_FUNC)ptr;
	    }
	    break;

	case GCS_PARM_USR_PWD_FUNC :
	    if ( set->length != sizeof( PTR ) )
		status = E_GC1012_INVALID_PARM_VALUE;
	    else
	    {
		MEcopy( set->buffer, sizeof( PTR ), &ptr );
		IIgcs_global->usr_pwd_func = (USR_PWD_FUNC)ptr;
	    }
	    break;

	case GCS_PARM_IP_FUNC :
	    if ( set->length != sizeof( PTR ) )
		status = E_GC1012_INVALID_PARM_VALUE;
	    else
	    {
		MEcopy( set->buffer, sizeof( PTR ), &ptr );
		IIgcs_global->ip_func = (IP_FUNC)ptr;
	    }
	    break;

	default :
	    status = E_GC1012_INVALID_PARM_VALUE;
	    break;
	}

	break;
    }

    case GCS_OP_VALIDATE :
    {
	GCS_VALID_PARM	*valid = (GCS_VALID_PARM *)parms;
	GCS_MECH	mech_id;
	GCS_OBJ_HDR	hdr;
	u_i4		id;

	if ( valid->length < sizeof( hdr ) )
	{
	    GCS_TRACE( 1 )
		( "GCS call: invalid object length: %d\n", valid->length );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    break;
	}

	MEcopy( valid->auth, sizeof( hdr ), (PTR)&hdr );

	if ( (id = GCS_GET_I4( hdr.gcs_id )) != GCS_OBJ_ID )
	{
	    GCS_TRACE( 1 )( "GCS call: invalid object ID: 0x%x\n", id );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    break;
	}

	/*
	** Check restriction of auth objects to configured mechanisms.
	*/
	mech_id = hdr.mech_id;

	switch( hdr.obj_id )
	{
	case GCS_USR_AUTH :
	    if ( IIgcs_global->restrict_usr )  
		mech_id = IIgcs_global->user_mech;
	    break;

	case GCS_PWD_AUTH :
	    if ( IIgcs_global->restrict_pwd )  
		mech_id = IIgcs_global->password_mech;
	    break;

	case GCS_SRV_AUTH :
	    if ( IIgcs_global->restrict_srv )  
		mech_id = IIgcs_global->server_mech;
	    break;

	case GCS_REM_AUTH :
	    if ( IIgcs_global->restrict_rem )  
		mech_id = IIgcs_global->remote_mech;
	    break;
	}

	if ( mech_id == GCS_NO_MECH  ||  mech_id != hdr.mech_id )
	{
	    GCS_TRACE( 1 )
		( "GCS call: %s validation restricted to %s: %s object\n",
		  (*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
					      hdr.obj_id ),
		  GCS_TR_MECH( mech_id ), GCS_TR_MECH( hdr.mech_id ) );

	    if ( IIgcs_global->err_log_func )
	    {
		ER_ARGUMENT	args[3];

		args[0].er_size = ER_PTR_ARGUMENT;
		args[0].er_value = 
		    (PTR)(*IIgcs_global->tr_lookup)( IIgcs_global->tr_objs, 
						     hdr.obj_id );
		args[1].er_size = ER_PTR_ARGUMENT;
		args[1].er_value = 
		    (PTR)IIgcs_global->mech_array[hdr.mech_id].info->mech_name;
		args[2].er_size = ER_PTR_ARGUMENT;
		args[2].er_value = 
		    (PTR)IIgcs_global->mech_array[ mech_id ].info->mech_name;

		(*IIgcs_global->err_log_func)( E_GC1026_GCF_RESTRICTED_AUTH,
					       sizeof(args) / sizeof(args[0]),
					       (PTR)args );
	    }

	    status = E_GC100C_RESTRICTED_AUTH;
	    break;
	}

	GCS_TRACE( 2 )
	    ( "GCS call: validating %s with mechanism %s\n",
	      (*IIgcs_global->tr_lookup)( gcs_tr_objs, hdr.obj_id),
	      GCS_TR_MECH( hdr.mech_id ) );

	status = gcs_call_mech( hdr.mech_id, op, parms );
	break;
    }

    case GCS_OP_USR_AUTH :  
    case GCS_OP_PWD_AUTH :
    case GCS_OP_SRV_KEY  :
    case GCS_OP_SRV_AUTH :
    case GCS_OP_REM_AUTH :
    case GCS_OP_IP_AUTH :
	status = gcs_call_caps( op, parms );
	break;

    case GCS_OP_RELEASE :
    {
	GCS_REL_PARM	*rel = (GCS_REL_PARM *)parms;
	GCS_OBJ_HDR	hdr;
	u_i4		id;

	if ( rel->length < sizeof( hdr ) )
	{
	    GCS_TRACE( 1 )
		( "GCS call: invalid object length: %d\n", rel->length );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    break;
	}

	MEcopy( rel->token, sizeof( hdr ), (PTR)&hdr );

	if ( (id = GCS_GET_I4( hdr.gcs_id )) != GCS_OBJ_ID )
	{
	    GCS_TRACE( 1 )( "GCS call: invalid object ID: 0x%x\n", id );
	    status = E_GC1011_INVALID_DATA_OBJ;
	    break;
	}

	GCS_TRACE( 2 )
	    ( "GCS call: release %s with mechanism %s\n",
	      (*IIgcs_global->tr_lookup)( gcs_tr_objs, hdr.obj_id),
	      GCS_TR_MECH( hdr.mech_id ) );

	status = gcs_call_mech( hdr.mech_id, op, parms );
	break;
    }

    case GCS_OP_E_INIT :
    case GCS_OP_E_CONFIRM :
    case GCS_OP_E_ENCODE :
    case GCS_OP_E_DECODE :
    case GCS_OP_E_TERM :
	status = E_GC1003_GCS_OP_UNSUPPORTED;
	break;

    default :
	status = E_GC1002_GCS_OP_UNKNOWN;
	break;
    }

    return( status );
}


/*
** Name: gcs_call_mech
**
** Description:
**	Check status of requested mechanism and call if available.
**
** Input:
**	mech		Mechanism ID.
**	op		Operation
**	parms		Parameters for operation.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	16-May-97 (gordy)
**	    Created.
**	 5-Aug-97 (gordy)
**	    Mechanism abstraction eliminated unavailable status.
**	 9-Jul-04 (gordy)
**	    Installation Password authentication made distinct operation
**	    at mech API version 2.
*/

static STATUS
gcs_call_mech( GCS_MECH mech, i4 op, PTR parms )
{
    GCS_MECH_INFO 	*info = IIgcs_global->mech_array[ mech ].info;
    STATUS		status;

    if ( ! info )
	status = E_GC1004_SEC_MECH_UNKNOWN;
    else  if ( info->mech_status == GCS_DISABLED )
	status = E_GC1005_SEC_MECH_DISABLED;
    else  if ( info->mech_status != GCS_AVAIL )
	status = E_GC1000_GCS_FAILURE;	/* should not happen */
    else
    {
	switch( op )
	{
	case GCS_OP_REM_AUTH :
	    /*
	    ** Installation Password authentication was
	    ** made a distinct operation with mech API
	    ** version 2.  Convert to IP Auth if mech
	    ** is version 2 or greater and supports IP
	    ** auths.
	    */
	    if ( info->mech_ver > GCS_MECH_VERSION_1  &&
		 info->mech_caps & GCS_CAP_IP_AUTH )
		op = GCS_OP_IP_AUTH;
	    status = (*(IIgcs_global->mech_array[mech].func))( op, parms );
	    break;

	case GCS_OP_IP_AUTH :
	    /*
	    ** Installation Password authentication was
	    ** made a distinct operation with mech API
	    ** version 2.  Convert to Remote Auth if 
	    ** mech is version 1 and supports IP auths.
	    */
	    if ( info->mech_ver == GCS_MECH_VERSION_1  &&
		 info->mech_caps & GCS_CAP_IP_AUTH )
		op = GCS_OP_REM_AUTH;
	    status = (*(IIgcs_global->mech_array[mech].func))( op, parms );
	    break;

	default :
	    status = (*(IIgcs_global->mech_array[mech].func))( op, parms );
	    break;
	}
    }

    return( status );
}


/*
** Name: gcs_call_caps
**
** Description:
**	Call the configured mechanism for the requested operation.
**	The mechanism status and capabilities are checked.
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
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-04 (gordy)
**	    Capability no determined from operation rather than
**	    passed in.  Mechanism determined by configuration
**	    associated with operation rather than install/default
**	    mechanism.
*/

static STATUS
gcs_call_caps( i4  op, PTR parms )
{
    GCS_MECH	mech;
    u_i4	cap;
    STATUS	status = E_GC1003_GCS_OP_UNSUPPORTED;

    switch( op )
    {
    case GCS_OP_USR_AUTH :
	mech = IIgcs_global->user_mech;	
        cap = GCS_CAP_USR_AUTH;
	break;

    case GCS_OP_PWD_AUTH :
	mech = IIgcs_global->password_mech;
	cap = GCS_CAP_PWD_AUTH;
	break;

    case GCS_OP_SRV_KEY :
    case GCS_OP_SRV_AUTH :
	mech = IIgcs_global->server_mech;
	cap = GCS_CAP_SRV_AUTH;
	break;

    case GCS_OP_REM_AUTH :
	mech = IIgcs_global->remote_mech;
    	cap = GCS_CAP_REM_AUTH;
	break;

    case GCS_OP_IP_AUTH :
	mech = GCS_MECH_INGRES;
	cap = GCS_CAP_IP_AUTH;
	break;

    default :
	GCS_TRACE( 1 )
	    ( "GCS call: invalid operation: %d\n", op );
	return( E_GC1000_GCS_FAILURE );
    }

    if ( mech == GCS_NO_MECH )
    {
	GCS_TRACE( 1 )
	    ( "GCS call: no mechanism configured for operation: %s\n",
	      (*IIgcs_global->tr_lookup)( gcs_tr_ops, op ) );
    }
    else  if ( (status = gcs_check_caps( mech, cap )) == OK )
        status = gcs_call_mech( mech, op, parms );
    else
    {
	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[2];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)GCS_TR_MECH( mech );
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = (PTR)(*IIgcs_global->tr_lookup)
                                       ( gcs_tr_ops, op );

	    (*IIgcs_global->err_log_func)( E_GC1028_GCF_MECH_UNSUPPORTED,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
        }
	GCS_TRACE( 1 )
	    ( "GCS call: op unsupported by configured mechanism: %s %s 0x%x\n",
	      (*IIgcs_global->tr_lookup)( gcs_tr_ops, op ), 
	      GCS_TR_MECH( mech ), status );
    }

    return( status );
}


/*
** Name: gcs_init
**
** Description:
**	Initialize the GCS global data structure.
**
** Input:
**	init_parms	GCS_OP_INIT parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Added initialization parameters.  Mechanism 
**	    initialization moved to gcs_init_mechs().
**	20-Feb-98 (gordy)
**	    Define MO object classes.
**	17-Mar-98 (gordy)
**	    Use GCusername() for effective user ID.
**	21-sep-2000 (somsa01)
**	    PMinit() can return either OK or PM_NO_INIT.
**	22-sep-2000 (somsa01)
**	    Amended previous change with proper multiple status check.
**	 5-May-03 (gordy)
**	    Save GCusername() reference in globals.
**	 9-Jul-04 (gordy)
**	    Added individual mechanism configurations (bump version).
**	    Default mechanism is now obsolete.  Assign ingres mechanism
**	    as initial value for installation mechanism (default).
**	21-Jul-09 (gordy)
**	    Use max sized temp buffers for host and user names then
**	    dynamically allocate proper sized global storage.
*/

static STATUS
gcs_init( GCS_INIT_PARM	*init_parm )
{
    STATUS	status = OK, status2;
    char	host[ GC_HOSTNAME_MAX + 1 ];
    char	user[ GC_USERNAME_MAX + 1 ];
    char	*ptr;

    MEfill( sizeof( gcs_global ), '\0', (PTR)&gcs_global );
    IIgcs_global = &gcs_global;
    IIgcs_global->version = GCS_CB_VERSION_3;

    IIgcs_global->config_host = STalloc( PMhost() );

    GChostname( host, sizeof( host ) );
    IIgcs_global->host = STalloc( host );

    GCusername( user, sizeof( user ) );	
    IIgcs_global->user = STalloc( user );

    IIgcs_global->usr_func = GCusername;
    IIgcs_global->config_func = PMget;
    IIgcs_global->tr_func = (TR_TRACE_FUNC)TRdisplay;
    IIgcs_global->tr_lookup = gcx_look;
    IIgcs_global->tr_ops = gcs_tr_ops;
    IIgcs_global->tr_objs = gcs_tr_objs;
    IIgcs_global->tr_parms = gcs_tr_parms;

    IIgcs_global->install_mech = GCS_MECH_INGRES;	/* Default */
    IIgcs_global->default_mech = GCS_NO_MECH;		/* Obsolete! */
    IIgcs_global->user_mech = GCS_NO_MECH;
    IIgcs_global->restrict_usr = FALSE;
    IIgcs_global->password_mech = GCS_NO_MECH;
    IIgcs_global->restrict_pwd = FALSE;
    IIgcs_global->server_mech = GCS_NO_MECH;
    IIgcs_global->restrict_srv = FALSE;
    IIgcs_global->remote_mech = GCS_NO_MECH;
    IIgcs_global->restrict_rem = FALSE;

    /*
    ** Initialize the internal generic mechanism info.
    ** This mechanism is used to initialize the other
    ** mechanisms and the info is required to be able
    ** to access the mechanism.
    */
    IIgcs_global->mech_array[ GCS_NO_MECH ].info = &gcs_info;
    IIgcs_global->mech_array[ GCS_NO_MECH ].func = gcs_cntrl;

    /*
    ** Save (optional) callback functions.
    */
    IIgcs_global->mem_alloc_func = 
	init_parm->mem_alloc_func ? (MEM_ALLOC_FUNC)init_parm->mem_alloc_func
				  : gcs_alloc;

    IIgcs_global->mem_free_func = 
	init_parm->mem_free_func ? (MEM_FREE_FUNC)init_parm->mem_free_func
				 : gcs_free;

    if ( init_parm->err_log_func )
	IIgcs_global->err_log_func = (ERR_LOG_FUNC)init_parm->err_log_func;

    if ( init_parm->msg_log_func )
	IIgcs_global->msg_log_func = (MSG_LOG_FUNC)init_parm->msg_log_func;

    /*
    ** Initialize tracing (initialize PM 
    ** if it has not already been done).
    */
    status2 = PMinit();
    if ( status2 == OK || status2 == PM_NO_INIT )  PMload( NULL, NULL );
    gcu_get_tracesym( "II_GCS_TRACE", "!.gcs_trace_level", &ptr );
    if ( ptr  &&  *ptr )  CVal( ptr, &IIgcs_global->gcs_trace_level );

    MOclassdef( MAXI2, gcs_classes );

    return( status );
}


/*
** Name: gcs_term
**
** Description:
**	Shutdown each available mechanism and free GCS resources.
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
**	16-May-97 (gordy)
**	    Created.
**	 9-Jul-97 (gordy)
**	    Abstract mechanisms even further to support dynamic mechanisms.
**	20-Feb-98 (gordy)
**	    Detach MO mechanism instances.
*/

static STATUS
gcs_term( VOID )
{
    CL_ERR_DESC	clerr;
    char	str[ 16 ];
    i4		i;

    /*
    ** Shutdown mechanisms.
    */
    for( i = 0; i < IIgcs_global->mech_cnt; i++ )
    {
	GCS_MECH mech_id = IIgcs_global->mechanisms[ i ]->mech_id;

	GCS_TRACE( 2 )
	    ( "GCS term: shutting down mechanism: %s\n",
	      GCS_TR_MECH( mech_id ) );

	MOulongout( 0, (u_i8)mech_id, sizeof( str ), str );
	MOdetach( GCS_MIB_MECHANISM, str );
	(*(IIgcs_global->mech_array[ mech_id ].func))( GCS_OP_TERM, NULL );

	/*
	** Unload mechanism if dynamic.
	*/
	if ( IIgcs_global->mech_array[ mech_id ].hndl )
	    DLunload( IIgcs_global->mech_array[ mech_id ].hndl, &clerr );
    }

    return( OK );
}


/*
** Name: gcs_init_mechs
**
** Description:
**	Initialize internal mechanisms.  Load and initialize
**	dynamic mechanisms.  Configure authentication mechanisms.
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
**	 5-Aug-97 (gordy)
**	    Created.
**	17-Mar-98 (gordy)
**	    Use system prefix for PM symbols.
**	 9-Jul-04 (gordy)
**	    Added individual auth mechanism configurations.
**      31-Oct-2007 (Ralph Loen) Bug 119358
**          Return and log error code if a configured mechanism is disabled 
**          or unsupported.
**      01-Nov-2007 (Ralph Loen) Bug 119358
**          Don't invoke gcs_check_caps() if the configured mechanism is
**          GCS_MECH_NONE for password and remote authorization.  In these
**          cases, the appropriate internal mechanism is substituted at
**          a later time.
**      17-Dec-2007 (Ralph Loen) Bug 119621
**          When initializing remote authorization via gcs_config_mech(), 
**          the default mechanism argument should be GCS_NO_MECH, not
**          IIgcs_global->install_mech.
**      21-Jul-09 (gordy)
**	    Ensure there is space for config string including hostname.
*/

static STATUS
gcs_init_mechs( VOID )
{
    STATUS	status = OK;
    char	*ptr, str[ GC_HOSTNAME_MAX + 128 ];
    i4		i;
    GCS_MECH    mech;
    i4          op;

    /*
    ** Initialize mechanisms, internal then dynamic.
    */
    for( i = 0; i < MECH_COUNT; i++ )  
	if ( gcs_mech_init( gcs_mech_info[i].name, 
			    gcs_mech_info[i].func, NULL ) != OK )
    {
	    status = E_GC1006_SEC_MECH_FAIL;
            goto routine_exit;
    }

    if ( gcs_load_mechs() != OK )  
    {
        status = E_GC1006_SEC_MECH_FAIL;
        goto routine_exit;
    }

    /*
    ** Determine configuration of mechanisms for 
    ** GCS operations and check capabilities.
    */
    STprintf( str, GCS_CNF_MECHANISM, 
	      SystemCfgPrefix, IIgcs_global->config_host );

    gcs_config_mech( str, IIgcs_global->install_mech, 
    			  &IIgcs_global->install_mech );

    GCS_TRACE( 2 )
	( "GCS init: installation mechanism: %s\n",
	  GCS_TR_MECH( IIgcs_global->install_mech ) );
    
    STprintf( str, GCS_CNF_USR_MECH, 
	      SystemCfgPrefix, IIgcs_global->config_host );

    gcs_config_mech( str, IIgcs_global->install_mech,
				   &IIgcs_global->user_mech );
    status = gcs_check_caps( IIgcs_global->user_mech, GCS_CAP_USR_AUTH );

    STprintf( str, GCS_CNF_RSTRCT_USR_AUTH,
	      SystemCfgPrefix, IIgcs_global->config_host );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
        if ( ! STcasecmp( ptr, "true" ) )
	    IIgcs_global->restrict_usr = TRUE;
        else  if ( ! STcasecmp( ptr, "false" ) )
	    IIgcs_global->restrict_usr = FALSE;

    GCS_TRACE( 2 )
	( "GCS init: user authentication mechanism: %s %s %s\n",
	  GCS_TR_MECH( IIgcs_global->user_mech ),
	  (status == E_GC1005_SEC_MECH_DISABLED) ? "(disabled)" : 
	  (status == E_GC1003_GCS_OP_UNSUPPORTED) ? "(unsupported)" : "",
	  IIgcs_global->restrict_usr ? "[restricted]" : "" );
    
    if ( status != OK )
    {
        op = GCS_OP_USR_AUTH;
        mech = IIgcs_global->user_mech; 
        goto routine_exit;
    }

    STprintf( str, GCS_CNF_PWD_MECH, 
	      SystemCfgPrefix, IIgcs_global->config_host );

    gcs_config_mech( str, IIgcs_global->install_mech,
				   &IIgcs_global->password_mech );
    /*
    ** Don't check capabilities if "none" is configured for password
    ** authorization.  
    */
    if ( IIgcs_global->password_mech != GCS_NO_MECH )
        status = gcs_check_caps( IIgcs_global->password_mech, 
            GCS_CAP_PWD_AUTH );

    STprintf( str, GCS_CNF_RSTRCT_PWD_AUTH,
	      SystemCfgPrefix, IIgcs_global->config_host );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
        if ( ! STcasecmp( ptr, "true" ) )
	    IIgcs_global->restrict_pwd = TRUE;
        else  if ( ! STcasecmp( ptr, "false" ) )
	    IIgcs_global->restrict_pwd = FALSE;

    GCS_TRACE( 2 )
	( "GCS init: password authentication mechanism: %s %s %s\n",
	  GCS_TR_MECH( IIgcs_global->password_mech ),
	  (status == E_GC1005_SEC_MECH_DISABLED) ? "(disabled)" : 
	  (status == E_GC1003_GCS_OP_UNSUPPORTED) ? "(unsupported)" : "",
	  IIgcs_global->restrict_pwd ? "[restricted]" : "" );
    
    if ( status != OK )
    {
        op = GCS_OP_PWD_AUTH;
        mech = IIgcs_global->password_mech; 
        goto routine_exit;
    }
    
    STprintf( str, GCS_CNF_SRV_MECH, 
	      SystemCfgPrefix, IIgcs_global->config_host );

    gcs_config_mech( str, IIgcs_global->install_mech,
				   &IIgcs_global->server_mech );
    status = gcs_check_caps( IIgcs_global->server_mech, GCS_CAP_SRV_AUTH );

    STprintf( str, GCS_CNF_RSTRCT_SRV_AUTH,
	      SystemCfgPrefix, IIgcs_global->config_host );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
        if ( ! STcasecmp( ptr, "true" ) )
	    IIgcs_global->restrict_srv = TRUE;
        else  if ( ! STcasecmp( ptr, "false" ) )
	    IIgcs_global->restrict_srv = FALSE;

    GCS_TRACE( 2 )
	( "GCS init: server authentication mechanism: %s %s %s\n",
	  GCS_TR_MECH( IIgcs_global->server_mech ),
	  (status == E_GC1005_SEC_MECH_DISABLED) ? "(disabled)" : 
	  (status == E_GC1003_GCS_OP_UNSUPPORTED) ? "(unsupported)" : "",
	  IIgcs_global->restrict_srv ? "[restricted]" : "" );
    
    if ( status != OK )
    {
        op = GCS_OP_SRV_AUTH;
        mech = IIgcs_global->server_mech; 
        goto routine_exit;
    }
    
    STprintf( str, GCS_CNF_REM_MECH, 
	      SystemCfgPrefix, IIgcs_global->config_host );

    gcs_config_mech( str, GCS_NO_MECH, &IIgcs_global->remote_mech );

    /*
    ** Don't check capabilities if "none" is configured for remote
    ** authorization.  
    */
    if ( IIgcs_global->remote_mech != GCS_NO_MECH )
        status = gcs_check_caps( IIgcs_global->remote_mech, GCS_CAP_REM_AUTH );

    STprintf( str, GCS_CNF_RSTRCT_REM_AUTH,
	      SystemCfgPrefix, IIgcs_global->config_host );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
        if ( ! STcasecmp( ptr, "true" ) )
	    IIgcs_global->restrict_rem = TRUE;
        else  if ( ! STcasecmp( ptr, "false" ) )
	    IIgcs_global->restrict_rem = FALSE;

    GCS_TRACE( 2 )
	( "GCS init: remote authentication mechanism: %s %s %s\n",
	  GCS_TR_MECH( IIgcs_global->remote_mech ),
	  (status == E_GC1005_SEC_MECH_DISABLED) ? "(disabled)" : 
	  (status == E_GC1003_GCS_OP_UNSUPPORTED) ? "(unsupported)" : "",
	  IIgcs_global->restrict_rem ? "[restricted]" : "" );
    
    if ( status != OK )
    {
        op = GCS_OP_REM_AUTH;
        mech = IIgcs_global->remote_mech; 
        goto routine_exit;
    }

routine_exit:
    
    if ( IIgcs_global->err_log_func )
    {
        ER_ARGUMENT		args[2];

        if ( status == E_GC1003_GCS_OP_UNSUPPORTED )
        {
	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)GCS_TR_MECH( mech );
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = (PTR)(*IIgcs_global->tr_lookup)
                                       ( gcs_tr_ops, op );

	    (*IIgcs_global->err_log_func)( E_GC1028_GCF_MECH_UNSUPPORTED,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
        }
        if ( status == E_GC1005_SEC_MECH_DISABLED )
        {
	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)GCS_TR_MECH( mech );
	    (*IIgcs_global->err_log_func)( E_GC1027_GCF_MECH_DISABLED,
					   sizeof(args), (PTR)args );
        }
    }
    return( status );
}


/*
** Name: gcs_load_mechs
**
** Description:
**	Obtains info on dynamic mechanisms from config file.
**	Calls gcs_mech_load() to dynamically load each mechanism.
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
**	24-Jul-97 (gordy)
**	    Created.
**	20-Aug-97 (gordy)
**	    Add ability to load mechanisms for specific to servers.
**	17-Mar-98 (gordy)
**	    Use system prefix for PM symbols.  Added ability to specify 
**	    library path for each mechanism.
**	10-Jul-98 (gordy)
**	    Support 'none' in mechanism list so server specific lists can
**	    be set to override the default list and load no mechanisms.
**      21-Jul-09 (gordy)
**	    Ensure there is space for config string including hostname.
**	    Use GCS_NAME_MAX for standard sized names.
*/

static STATUS
gcs_load_mechs( VOID )
{
    STATUS	status, ret_stat = OK;
    CL_ERR_DESC	clerr;
    LOCATION	*loc, def_loc, mech_loc;
    char	*ptr, *path, str[ GC_HOSTNAME_MAX + 128 ];
    char	*names[ GCS_MAX_DYNAMIC ];
    char	nbuff[ GCS_MAX_DYNAMIC * (GCS_NAME_MAX + 1) ];
    char	pbuff[ MAX_LOC + 1 ];
    char	module[ GCS_NAME_MAX + 1 ];
    char	entry[ GCS_NAME_MAX + 1 ];
    i4		i, cnt;

    /*
    ** Get list of mechanisms to be dynamically loaded.
    ** Check for server specific list then general list.
    */
    status = (*IIgcs_global->config_func)( GCS_CNF_SRV_LOAD, &ptr );

    if ( status != OK  ||
         ! (cnt = gcu_words( ptr, nbuff, names, ',', GCS_MAX_DYNAMIC )) )
    {
	STprintf(str, GCS_CNF_LOAD, SystemCfgPrefix, IIgcs_global->config_host);
	status = (*IIgcs_global->config_func)( str, &ptr );

	if ( status != OK  ||
	     ! (cnt = gcu_words( ptr, nbuff, names, ',', GCS_MAX_DYNAMIC )) )
	    return( OK );
    }

    /*
    ** Find location of mechanism libraries.  Hopefully
    ** this is specified in the config file since the
    ** default location can be overridden by environment
    ** logicals allowing spoofing of security mechanisms.
    */
    STprintf( str, GCS_CNF_PATH, SystemCfgPrefix, IIgcs_global->config_host );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
    {
	STcopy( ptr, pbuff );
	LOfroms( PATH, pbuff, &def_loc );
    }
    else
    {
#ifdef NT_GENERIC
	NMloc( BIN, PATH, NULL, &def_loc );
#else
	NMloc( LIB, PATH, NULL, &def_loc );
#endif
	LOcopy( &def_loc, pbuff, &def_loc );
    }

    for( i = 0; i < cnt; i++ )
    {
	/*
	** We support the special case 'none' to permit 
	** configurations in which servers do not load
	** mechanisms in the default mechanism list.
	*/
	if ( ! STcasecmp( names[i], ERx("none") ) )
	    continue;

	STprintf( str, GCS_CNF_MECH_PATH, 
		  SystemCfgPrefix, IIgcs_global->config_host, names[i] );
	if ( (*IIgcs_global->config_func)( str, &ptr ) != OK )  
	    loc = &def_loc;
	else
	{
	    STcopy( ptr, pbuff );
	    LOfroms( PATH, pbuff, &mech_loc );
	    loc = &mech_loc;
	}

	LOtos( loc, &path );

	STprintf( str, GCS_CNF_MECH_MODULE, 
		  SystemCfgPrefix, IIgcs_global->config_host, names[i] );
	if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )  
	    STcopy( ptr, module );
	else
	    STcopy( names[i], module );

	STprintf( str, GCS_CNF_MECH_ENTRY, 
		  SystemCfgPrefix, IIgcs_global->config_host, names[i] );
	if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )  
	    STcopy( ptr, entry );
	else
	    STcopy( GCS_DYNAMIC_ENTRY, entry );

	GCS_TRACE( 2 )
	    ( "GCS load: loading %s (module %s) in %s, entry %s\n",
	      names[i], module, path, entry );

	if ( (status = gcs_mech_load( names[i], module, loc, entry )) != OK )
	    ret_stat = status;
    }

    return( ret_stat );
}


/*
** Name: gcs_mech_load
**
** Description:
**	Dynamically load a mechanism and determine main entry point.
**	Calls gcs_mech_init() to initialize each mechanism loaded.
**
** Input:
**	name		Name of mechanism.
**	module		Module name identifying dynamic library.
**	loc		Location of dynamic libraries.
**	entry		Name of entry point function.
**
** Output:
**	None.
**
** Returns:
**	Status		OK or error code.
**
** History:
**	 5-Aug-97 (gordy)
**	    Created.
*/

static STATUS
gcs_mech_load( char *name, char *module, LOCATION *loc, char *entry )
{
    LOCATION	ploc;
    CL_ERR_DESC	clerr;
    STATUS	status;
    MECH_FUNC	func;
    PTR		hndl;
    char	pbuff[ MAX_LOC ];
    char	*syms[ 2 ];
    char	*path;

    LOcopy( loc, pbuff, &ploc );
    LOtos( loc, &path );
    syms[ 0 ] = entry;
    syms[ 1 ] = NULL;

    status = DLprepare_loc( NULL, module, syms, 
			    &ploc, DL_RELOC_DEFAULT, &hndl, &clerr );

    if ( status != OK )
    {
	GCS_TRACE( 1 )
	    ( "GCS load: error loading %s in %s (module %s), status 0x%x\n", 
	      name, path, module, status );

	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[4];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)name;
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = (PTR)path;
	    args[2].er_size = ER_PTR_ARGUMENT;
	    args[2].er_value = (PTR)module;
	    args[3].er_size = ER_PTR_ARGUMENT;
	    args[3].er_value = (PTR)&status;

	    (*IIgcs_global->err_log_func)( E_GC1023_GCF_MECH_LOAD,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
	    if ( status != FAIL )
		(*IIgcs_global->err_log_func)( status, 0, NULL );
	}

	return( status );
    }

    status = DLbind( hndl, entry, (PTR *)&func, &clerr );

    if ( status == OK )
	status = gcs_mech_init( name, func, hndl );
    else
    {
	GCS_TRACE( 1 )
	    ( "GCS load: error binding %s in %s (module %s), status 0x%x\n", 
	      entry, path, module, status );

	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[5];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)name;
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = (PTR)entry;
	    args[2].er_size = ER_PTR_ARGUMENT;
	    args[2].er_value = (PTR)module;
	    args[3].er_size = ER_PTR_ARGUMENT;
	    args[3].er_value = (PTR)path;
	    args[4].er_size = ER_PTR_ARGUMENT;
	    args[4].er_value = (PTR)&status;

	    (*IIgcs_global->err_log_func)( E_GC1024_GCF_MECH_BIND,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
	    if ( status != FAIL )
		(*IIgcs_global->err_log_func)( status, 0, NULL );
	}
    }

    if ( status != OK )  DLunload( hndl, &clerr );

    return( status );
}


/*
** Name: gcs_mech_init
**
** Description:
**	Initialize a mechanism.  If successful, mechanism 
**	is added to the global mechanism info.  Checks config
**	file to see if mechanism is disabled.
**	
**
** Input:
**	name		Mechnism name.
**	func		Mechanism entry point (static or dynamic).
**	hndl		Dynamic library handle for mechanism (may be NULL).
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	 9-Jul-97 (gordy)
**	    Created.
**	20-Feb-98 (gordy)
**	    Attach MO mechanism instances.
**	17-Mar-98 (gordy)
**	    Use system prefix for PM symbols.
**	26-Jul-98 (gordy)
**	    Changed config disabled param to enabled.
**      21-Jul-09 (gordy)
**	    Ensure there is space for config string including hostname.
*/

static STATUS
gcs_mech_init( char *name, MECH_FUNC func, PTR hndl )
{
    GCS_INFO_PARM	parm;
    GCS_MECH_INFO	*info;
    STATUS		status;
    char		*ptr, str[ GC_HOSTNAME_MAX + 128 ];

    GCS_TRACE( 2 )
	( "GCS init: initializing mechanism: %s\n", name );

    /*
    ** When initializing mechanisms, we pass the global 
    ** data pointer rather than the external init parms 
    ** to support dynamically loaded mechanisms which 
    ** may not be able to access data objects outside 
    ** the scope of their dynamic load modules.  All 
    ** pertinent info from the init parms has been moved 
    ** to the global data structure.
    */
    if ( (status = (*func)( GCS_OP_INIT, (PTR)IIgcs_global )) != OK )
    {
	GCS_TRACE( 1 )
	    ( "GCS init: mechanism %s init failed, status 0x%x\n", 
	      name, status );

	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[2];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)name;
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = (PTR)&status;

	    (*IIgcs_global->err_log_func)( E_GC1020_GCF_MECH_INIT,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
	    if ( status != FAIL )
		(*IIgcs_global->err_log_func)( status, 0, NULL );
	}

	return( FAIL );
    }

    if ( (status = (*func)(GCS_OP_INFO, (PTR)&parm)) != OK  || 
	 parm.mech_count != 1 )
    {
	GCS_TRACE( 1 )
	    ( "GCS init: mechanism %s info request failed, status 0x%x\n", 
	      name, status );

	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[1];

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)name;

	    (*IIgcs_global->err_log_func)( E_GC1021_GCF_MECH_INFO,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
	}

	(*func)( GCS_OP_TERM, NULL );		/* Clean-up */
	return( FAIL );
    }

    info = parm.mech_info[0];

    if ( IIgcs_global->mech_array[ info->mech_id ].info )
    {
	GCS_TRACE( 1 )
	    ( "GCS init: mechanism %s has same ID (%d) as mechanism %s\n", 
	      name, info->mech_id,
	      IIgcs_global->mech_array[ info->mech_id ].info->mech_name );

	if ( IIgcs_global->err_log_func )
	{
	    ER_ARGUMENT		args[3];
	    i4			mech_id = info->mech_id;

	    args[0].er_size = ER_PTR_ARGUMENT;
	    args[0].er_value = (PTR)name;
	    args[1].er_size = ER_PTR_ARGUMENT;
	    args[1].er_value = 
		(PTR)IIgcs_global->mech_array[info->mech_id].info->mech_name;
	    args[2].er_size = ER_PTR_ARGUMENT;
	    args[2].er_value = (PTR)&mech_id;

	    (*IIgcs_global->err_log_func)( E_GC1025_GCF_MECH_DUP_ID,
					   sizeof(args) / sizeof(args[0]),
					   (PTR)args );
	}

	(*func)( GCS_OP_TERM, NULL );		/* Clean-up */
	return( FAIL );
    }

    IIgcs_global->mech_array[ info->mech_id ].info = info;
    IIgcs_global->mech_array[ info->mech_id ].func = func;
    IIgcs_global->mech_array[ info->mech_id ].hndl = hndl;
    IIgcs_global->mechanisms[ IIgcs_global->mech_cnt++ ] = info;

    MOulongout( 0, (u_i8)info->mech_id, sizeof( str ), str );
    MOattach( MO_INSTANCE_VAR, GCS_MIB_MECHANISM, str, (PTR)info );

    /*
    ** Check to see if mechanism has been
    ** explicitly enabled or disabled.
    */
    STprintf( str, GCS_CNF_MECH_ENABLED, 
	      SystemCfgPrefix, IIgcs_global->config_host, info->mech_name );

    if ( (*IIgcs_global->config_func)( str, &ptr ) == OK )
	if ( ! STcasecmp( ptr, "true" ) )
	    info->mech_status = GCS_AVAIL;
	else  if ( ! STcasecmp( ptr, "false" ) )
	    info->mech_status = GCS_DISABLED;
	else
	{
	    GCS_TRACE( 1 )
		( "GCS init: invalid configuration value, '%s', for %s\n",
		  ptr, str );

	    info->mech_status = GCS_DISABLED;
	}

    if ( info->mech_status != GCS_AVAIL )
    {
	GCS_TRACE( 4 )
	    ( "GCS init: mechanism disabled: %s\n", info->mech_name );

	info->mech_status = GCS_DISABLED;
    }

    return( OK );
}


/*
** Name: gcs_config_mech
**
** Description:
**	Loads config information and determines the referenced
**	mechanism.  Returns GCS_NO_MECH if 'none' is configured
**	or mechanism cannot be identified.  Returns the default,
**	provided by caller, if 'default' is configured or no
**	configuration exists.
**
** Input:
**	cfg		Configuration string.
**	def_mech	Default mechanism.
**
** Output:
**	mech		Configured mechanism.
**
** Returns:
**	STATUS		OK or E_GC1004_SEC_MECH_UNKNOWN.
**
** History:
**	 9-Jul-04 (gordy)
**	   Created.
*/

static STATUS
gcs_config_mech( char *cfg, GCS_MECH def_mech, GCS_MECH *mech )
{
    char	*ptr;
    STATUS	status = OK;

    if ( (*IIgcs_global->config_func)( cfg, &ptr ) != OK )
	*mech = def_mech;
    else  if ( ! STcasecmp( ptr, "none" ) )
	*mech = GCS_NO_MECH;
    else  if ( ! STcasecmp( ptr, "default" ) )
	*mech = def_mech;
    else
    {
	i4 i;

	for( i = 0; i < IIgcs_global->mech_cnt; i++ )
	    if ( ! STcasecmp( IIgcs_global->mechanisms[i]->mech_name, ptr ) )
	    {
		*mech = IIgcs_global->mechanisms[i]->mech_id;
		break;
	    }

	if ( i >= IIgcs_global->mech_cnt ) 
	{
	    GCS_TRACE( 1 )( "GCS: unknown mechanism: %s\n", ptr );

	    if ( IIgcs_global->err_log_func )
	    {
		ER_ARGUMENT		args[1];

		args[0].er_size = ER_PTR_ARGUMENT;
		args[0].er_value = (PTR)ptr;

		(*IIgcs_global->err_log_func)( E_GC1022_GCF_MECH_INSTALL,
					       sizeof(args) / sizeof(args[0]), 
					       (PTR)args );
	    }

	    *mech = GCS_NO_MECH;
	    status = E_GC1004_SEC_MECH_UNKNOWN;
	}
    }

    return( status );
}


/*
** Name: gcs_check_caps
**
** Description:
**	Check that a mechanism can support a requested capability.
**	Note: while capabilities are bit mask values, only a single 
**	capability should be tested to avoid erroneous results.
**
** Input:
**	mech	Mechanism ID.
**	cap	Capability
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK
**		E_GC1005_SEC_MECH_DISABLED
**		E_GC1003_GCS_OP_UNSUPPORTED
**
** History:
**	 9-Jul-04 (gordy)
**	    Created.
*/

static STATUS
gcs_check_caps( GCS_MECH mech, u_i4 cap )
{
    STATUS status = OK;

    if ( IIgcs_global->mech_array[ mech ].info->mech_status != GCS_AVAIL )
	status = E_GC1005_SEC_MECH_DISABLED;
    else  if ( ! (IIgcs_global->mech_array[ mech ].info->mech_caps & cap) )
	status = E_GC1003_GCS_OP_UNSUPPORTED;

    return( status );
}


/*
** Name: gcs_alloc
**
** Description:
**	Mechanism callback routine to dynamically allocate memory.
**
** Input:
**	length		Amount of memory to be allocated.
**
** Output:
**	None.
**
** Returns:
**	PTR		Allocated memory or NULL if error.
**
** History:
**	 5-Aug-97 (gordy)
**	    Created.
*/

static PTR
gcs_alloc( u_i4 length )
{
    STATUS	status;

    return( MEreqmem( 0, length, TRUE, &status ) );
}


/*
** Name: gcs_free
**
** Description:
**	Mechanism callback routine to free dynamically allocated memory.
**
** Input:
**	ptr		Memory to be freed.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 5-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcs_free( PTR ptr )
{
    MEfree( ptr );
    return;
}



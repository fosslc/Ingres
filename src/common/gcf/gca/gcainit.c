/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <cv.h>
#include    <cs.h>
#include    <er.h>
#include    <gc.h>
#include    <me.h>
#include    <mu.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <sp.h>
#include    <mo.h>
#include    <lo.h>
#include    <pm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcocomp.h>
#include    <gcm.h>
#include    <gcs.h>
#include    <gcr.h>
#include    <gcxdebug.h>
#include    <gcu.h>

/**
**
**  Name: gcainit.c - Initialize GCA communications
**
**  Description:
**
**          gca_initiate - Initialize User/GCA Association
**
**
**  History:    $Log-for RCS$
**      13-Apr-1987 (berrow)
**          Created.
**      05-Aug-87 (jbowers)
**          Added user specification for "P" and "V" semaphore routines to
**          parameter list.
**          Changed cleanup and exit code to call gca_complete to perform
**          completion processing, for consistency w/ other GCA routines.
**      11-Aug-87 (jbowers)
**          Added invocation of GCdefexh to establish an exit handler
**      22-Oct-87 (jbowers)
**          Changed check for P and V semaphore routines to ensure both are
**          specified, otherwise ignore.
**      23-Feb-88 (jbowers)
**          Added initialization of gca_listen_assoc in GCA_STATIC data area.
**      11-Jul-88 (jbowers)
**          Updated to Rev. level 2.0 interface.  Added additional input
**	    parameters, and updates to GCA_STATIC area.
**	31-Jul-89 (seiwald)
**	    Check and install gca_api_version.
**	04-Aug-89 (seiwald)
**	    Default level is GCI_API_LEVEL_0, not just 0.
**	21-Sep-89 (seiwald)
**	    New ACB management structure initialized.
**	27-Sep-89 (seiwald)
**	    GCA_API_LEVEL_2.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	15-Nov-89 (seiwald)
**	    Decrufted.  New II_GCAxx_TRACE tracing.
**	25-Nov-89 (seiwald)
**	    II_GCAxx_TRACE is now just II_GCA_TRACE, like II_GCC_TRACE,
**	    II_GCN_TRACE, and II_GC_TRACE.
**	12-Dec-89 (seiwald)
**	    Removed antiquated gca_listen_assoc.  
**	25-Jan-90 (seiwald)
**	    Removed senseless zeroing of acb structure.
**	15-Feb-90 (ham)
**	    Removed references to gca_timechk.
**	06-Jun-90 (seiwald)
**	    Made gca_alloc() return the pointer to allocated memory.
**	15-Aug-90 (seiwald)
**	    GCinititate returns STATUS.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	26-Dec-91 (seiwald)
**	    GCA_API_LEVEL_3.
**	28-may-92 (seiwald)
**	    Support for II_GCF_SECURITY "OFF": set gca_no_security on
**	    startup.
**	08-Jul-92 (brucek)
**	    GCA_API_LEVEL_4.
**	01-Oct-92 (brucek)
**	    Added MIB definitions.
**	09-Oct-92 (brucek)
**	    Look in send_peer_info for GCA_MIB_ASSOC_USERID.
**	10-Nov-92 (brucek)
**	    Ifdef out gca_getflag for GCF64.
**	11-nov-92 (seiwald) 
**	    Declare gca_getflag as static, not FUNC_EXTERN
**	19-Nov-92 (brucek)
**	    Remove the ill-fated gca_getflag.  All integer-type objects
**	    must be returned in decimal, by convention.
**	31-Dec-92 (edg)
**	    Added PMget for II_GCF_SECURITY ifndef GCF64.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	11-Jan-93 (edg)
**	    Call gcu_get_tracesym() to get tracing symbols.
**	12-Jan-93 (brucek)
**	    Add support for gca_auth_user.
**	10-Mar-93 (edg)
**	    Add support for acbs_in_use in MIB.
**	14-Mar-94 (seiwald)
**	    GCA_API_LEVEL_5.
**	4-apr-94 (seiwald)
**	    At API_LEVEL_5, no GCA_FORMAT, GCA_INTERPRET.
**      13-Apr-1994 (daveb) 62240
**          Add statistic objects:
**		 "exp.gcf.gca.msgs_in" - GCA packets in
**		 "exp.gcf.gca.msgs_out"- GCA packets out
**		 "exp.gcf.gca.data_in" - GCA data in ( in kbytes )
**		 "exp.gcf.gca.data_out"- GCA data out ( in kbytes )
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**      12-jun-1995 (chech02)
**          Added semaphores to protect critical sections (MCT).
**	30-Aug-1995 (sweeney)
**	    got rid of an unused buffer left over from NMgtAt days.
**	10-Oct-95 (gordy)
**	    Completion moved to gca_service().
**	 3-Nov-95 (gordy)
**	    Removed MCT semaphores since initialization is only done once.
**	20-Nov-95 (gordy)
**	    Added prototypes.  Cleanup semaphore if initialization fails.
**	21-Nov-95 (gordy)
**	    Added gcr.h for embedded Comm Server configuration.
**	 4-Dec-95 (gordy)
**	    Added support for embedded Name Server configuration.
**	20-Dec-95 (gordy)
**	    Added incremental initialization support to gcd_init().
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
**	 3-Sep-96 (gordy)
**	    Restructured GCA global data.  Added GCA control blocks.
**	 9-Sep-96 (gordy)
**	    Don't use MOulongout() to create instance ID, it zero-fills.
**	    Before the addition of control blocks the client stuff could
**	    be found with and instance id of '0'.  There is generally
**	    only one GCA client and by using STprint() the instance ID
**	    will still be '0'.  There are a number of non-GCA entities
**	    which are looking for this instance ID.
**	23-May-97 (thoda04)
**	    Included me.h, pm.h, gcxdebug.h headers for function protos.
**	13-jun-97 (hayke02)
**	    Added include of lo.h (which includes locl.h), so that pm.h
**	    will have the definition of LOCATION.
**	30-sep-1996 (canor01)
**	    Move global data definitions to gcastitc.c.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declarations.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	23-May-97 (thoda04)
**	    Included me.h, pm.h, gcxdebug.h headers for function protos.
**	29-May-97 (gordy)
**	    New GCF security handling.  Initialize GCS.  No longer need
**	    to check for security disabling (GCS supports through NULL
**	    security mechanism).  Removed support for auth function
**	    (now done through GCS registration).
**	13-jun-97 (hayke02)
**	    Added include of lo.h (which includes locl.h), so that pm.h
**	    will have the definition of LOCATION.
**	 5-Aug-97 (gordy)
**	    GCS_OP_INIT now takes parameters, pass memory management routines.
**	17-Feb-98 (gordy)
**	    Allow tracing to be managed through GCM.
**	17-Mar-98 (gordy)
**	    Use GCusername() for effective user ID.
**	26-May-98 (gordy)
**	    Check environment variable for remote auth ignore/fail.
**	28-Jul-98 (gordy)
**	    Added installation ID to GLOBALs for server bedcheck.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	 8-Jan-99 (gordy)
**	    Add flag for including timestamps in tracing.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	15-Oct-03 (gordy)
**	    GCA_API_LEVEL_6.
**	11-May-06 (gordy)
**	    Semaphore protect registration ACB.
**	21-Jul-09 (gordy)
**	    Remove username length restriction.
**/

/*
**  Forward and/or External function references.
*/
static	VOID	gca_init_mib( VOID );


/*{
** Name: gca_initiate	- Initialize User/GCA Association
**
** Description:
**      This routine is used to set up necessary internal data structures 
**	when a user (either client or server) makes itself known to GCF.
**	In particular, the expedited and normal completion exits and the
**	users alternate memory management functions are recorded (if they
**	were specified).
**	
**	All parameters (both input and output) are handled via a pointer to
**	SVC_PARMS structure containing the following elements ...
**
** Inputs:
**
**      gca_expedited_completion_exit   Pointer to an exit routine to be driven
**					upon completion of an asynchronous
**					request to receive an expedited message.
**      gca_normal_completion_exit      Pointer to an exit routine to be driven
**					upon completion of an asynchronous
**					request to receive a normal message.
**      gca_alloc_mgr                   Pointer to memory allocation management
**					function to be used instead of MEalloc.
**      gca_dalloc_mgr                  Pointer to memory de-allocation function
**					to be used instead of MEfree.
**	gca_lsncnfrm			Pointer to a routine to be called when
**					an association request is received, to
**					allow the server to specify whether it
**					wishes to continue the association.
**	gca_modifiers			A set of flags specifying particular
**					modifications to the characteristics of
**					or services requested by the invoking
**					server.  No values are currently
**					defined.
**
** Outputs:
**      gca_status                      Success or failure indication.
**
** Returns:
**	None
**
** Exceptions:
**	None
**
** Side Effects:
**	None.
**
** History:
**	10-Oct-95 (gordy)
**	    Completion moved to gca_service().
**	20-Nov-95 (gordy)
**	    Added prototypes.  Cleanup semaphore if initialization fails.
**	 4-Dec-95 (gordy)
**	    Added support for embedded Name Server configuration.
**	20-Dec-95 (gordy)
**	    Added incremental initialization support to gcd_init().
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
**	 3-Sep-96 (gordy)
**	    Restructured GCA global data.  Added GCA control blocks.
**	 9-Sep-96 (gordy)
**	    Use STprintf() rather than MOulongout() to create the instance ID.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	29-May-97 (gordy)
**	    New GCF security handling.  Initialize GCS.  No longer need
**	    to check for security disabling (GCS supports through NULL
**	    security mechanism).  Removed support for auth function
**	    (now done through GCS registration).
**	 5-Aug-97 (gordy)
**	    GCS_OP_INIT now takes parameters, pass memory management routines.
**	17-Feb-98 (gordy)
**	    Allow tracing to be managed through GCM.
**	17-Mar-98 (gordy)
**	    Use GCusername() for effective user ID.
**	26-May-98 (gordy)
**	    Check environment variable for remote auth ignore/fail.
**	28-Jul-98 (gordy)
**	    Added installation ID to GLOBALs for server bedcheck.
**	 8-Jan-99 (gordy)
**	    Add flag for including timestamps in tracing.
**	 4-Aug-99 (rajus01)
**	    Initialize gca_embed_gcc_flag.
**       06-07-01 (wansh01) 
**          Removed gcx_init. init now is done in gcd_init and gcc_init.
**	11-May-06 (gordy)
**	    Semaphore protect registration ACB.
**	21-Jul-09 (gordy)
**	    Declare max username buffer.  Use dynamic storage for
**	    actual value.
*/

VOID
gca_initiate( GCA_SVC_PARMS *svc_parms )
{
    GCA_CB		*gca_cb = (GCA_CB *)svc_parms->gca_cb;
    GCA_IN_PARMS	*in_parms = (GCA_IN_PARMS *)svc_parms->parameter_list;
    GCS_INIT_PARM	gcs_parms;
    char		*nm, buff[ 128 ];
    char		user[ GC_USERNAME_MAX + 1 ];
    char		*embed = NULL;

    /*
    ** Check for global initialization.
    */
    if ( ! IIGCa_global.gca_initialized )
    {
	/*
	** Clear the global data area in case there
	** is any garbage from a previous life.
	*/
	MEfill( sizeof( IIGCa_global ), (u_char)0, (PTR)&IIGCa_global );

	/*
	** We must use the same memory management routines 
	** for all dynamic memory processing.  We will use
	** the routines specified in the first GCA_INITIALIZE 
	** request, if provided.  Otherwise, the default ME 
	** routines will be used.  Subsequent GCA_INITIALZE 
	** requests specifying memory management routines 
	** will be ignored.
	*/
	IIGCa_global.gca_alloc = in_parms->gca_alloc_mgr;
	IIGCa_global.gca_free = in_parms->gca_dealloc_mgr;

	/* 
	** Initialize Tracing... 
	*/


	IIGCa_global.gca_trace_log[0] = EOS;
	gcu_get_tracesym( "II_GCA_LOG", "!.gca_trace_log", &nm );
	if ( nm  &&  *nm )  gca_trace_log( 0, STlength( nm ), nm, 0, NULL );

	gcu_get_tracesym( "II_GCA_TRACE", "!.gca_trace_level", &nm );
	if ( nm  &&  *nm )  CVal( nm, &IIGCa_global.gca_trace_level );

	gcu_get_tracesym( "II_GCA_TIME", "!.gca_trace_time", &nm );
	IIGCa_global.gca_trace_time = (nm && *nm);

	  
	/* 
	** Initialize security.
	**
	** We may be running as part of a GCF utility (such as the
	** Name Server) which also uses GCS.  Such utilities should
	** initialize GCS prior to GCA, in which case our request
	** is ignored and it is the utilities responsibility to
	** properly intialize GCS.  Our initialization request is
	** formatted for GCA being the sole GCS client.
	*/
	MEfill( sizeof( gcs_parms ), (u_char)0, (PTR)&gcs_parms );
	gcs_parms.version = GCS_API_VERSION_1;
	gcs_parms.mem_alloc_func = (PTR)gca_alloc;
	gcs_parms.mem_free_func = (PTR)gca_free;

        if ( (svc_parms->gc_parms.status = 
	      IIgcs_call( GCS_OP_INIT, GCS_NO_MECH, (PTR)&gcs_parms )) != OK )
	    goto complete;

	IIGCa_global.gca_rmt_auth_fail = FALSE;
	STprintf( buff, "%s.%s.gcf.remote_auth_error", 
		  SystemCfgPrefix, PMhost() );
	gcu_get_tracesym( "II_GCF_AUTH_ERROR", buff, &nm );

	if ( nm  &&  *nm  &&  ! STcasecmp( nm, "fail" ) )
	    IIGCa_global.gca_rmt_auth_fail = TRUE;

#ifdef GCF_EMBEDDED_GCC
	 IIGCa_global.gca_embed_gcc_flag = TRUE;

	 NMgtAt( "II_EMBEDDED_GCC", &embed );
	 if ( embed  || *embed )
	    if ( ! STbcompare( embed, 0, "OFF", 0, TRUE )   ||
		 ! STbcompare( embed, 0, "DISABLED", 0, TRUE) ||
		 ! STbcompare( embed, 0, "FALSE", 0, TRUE ) )
		IIGCa_global.gca_embed_gcc_flag = FALSE;
#endif

#ifdef GCF_EMBEDDED_GCN

	IIGCa_global.gca_embedded_gcn = TRUE;
	STprintf( buff, "%s.%s.gca.embedded_gcn", SystemCfgPrefix, PMhost() );
	gcu_get_tracesym( "II_GCA_EMBEDDED_GCN", buff, &nm );

	if ( nm  &&  *nm )
	    if ( ! STcasecmp( nm, "OFF" ) )
		IIGCa_global.gca_embedded_gcn = FALSE;
	    else  if ( ! STcasecmp( nm, "ON" ) )
		IIGCa_global.gca_embedded_gcn = TRUE;

	/*
	** Initialize the embedded Name Server interface.
	*/
	if ( IIGCa_global.gca_embedded_gcn  &&
	     (svc_parms->gc_parms.status = gca_ns_init()) != OK )
	    goto complete;

#endif /* GCF_EMBEDDED_GCN */

	/*
	** Initialize the CL w/ the memory mgt routines 
	*/
	svc_parms->gc_parms.status = GCinitiate( gca_alloc, gca_free );
	if ( svc_parms->gc_parms.status != OK )  goto complete;

	MUi_semaphore( &IIGCa_global.gca_acb_semaphore );
	MUn_semaphore( &IIGCa_global.gca_acb_semaphore, "GCA ACB" );

	gcu_get_tracesym( "II_INSTALLATION", NULL, &nm );
	STncpy( IIGCa_global.gca_install_id,  nm ? nm : "*",
		 sizeof( IIGCa_global.gca_install_id ) - 1 );
	IIGCa_global.gca_install_id[ sizeof( IIGCa_global.gca_install_id ) - 1]
	    = EOS;

	GCusername( user, sizeof( user ) );
	IIGCa_global.gca_uid = STalloc( user );
	gca_init_mib();

	IIGCa_global.gca_initialized = TRUE;
    }

    /*
    ** Initialize the GCA control block.
    */
    MUi_semaphore( &gca_cb->gca_reg_semaphore );
    MUn_semaphore( &gca_cb->gca_reg_semaphore, "GCA REG ACB" );
    gca_cb->gca_flags |= in_parms->gca_modifiers;
    gca_cb->normal_ce = in_parms->gca_normal_completion_exit;
    gca_cb->expedited_ce = in_parms->gca_expedited_completion_exit;
    gca_cb->lsncnfrm = in_parms->gca_lsncnfrm;

    gca_cb->gca_local_protocol = in_parms->gca_local_protocol ?
				       in_parms->gca_local_protocol :
				       GCA_PROTOCOL_LEVEL;

    if ( ! ( in_parms->gca_modifiers & GCA_API_VERSION_SPECD ) )
    {
	gca_cb->gca_api_version = GCA_API_LEVEL_0;
    } 
    else 
    {
	switch( in_parms->gca_api_version )
	{
	    case GCA_API_LEVEL_0:
	    case GCA_API_LEVEL_1:
	    case GCA_API_LEVEL_2:
	    case GCA_API_LEVEL_3:
	    case GCA_API_LEVEL_4:
	    case GCA_API_LEVEL_5:
	    case GCA_API_LEVEL_6:
		gca_cb->gca_api_version = in_parms->gca_api_version;
		break;

	    default:
		svc_parms->gc_parms.status = E_GC000C_API_VERSION_INVALID;
		goto complete;
	}
    }

    /* 
    ** At API_LEVEL_5, no more message header across the interface 
    */
    gca_cb->gca_header_length = in_parms->gca_header_length = 
	    gca_cb->gca_api_version < GCA_API_LEVEL_5
	    ? sizeof(GCA_MSG_HDR) : 0;

    /*
    ** Only initialize the object descriptor sub-system 
    ** if it will potentially be used.  Note that gco_init() 
    ** may be called multiple times.
    */
    if ( gca_cb->gca_api_version >= GCA_API_LEVEL_5 )
	if ( (svc_parms->gc_parms.status = gco_init( FALSE )) != OK )
	    goto complete;

    /*
    ** We assign a monotonically increasing but meaningless
    ** value to the control block to server as a MIB index
    ** value.  Tell MO of the new control block.  Don't use 
    ** MOulongout() to create instance ID, it zero-fills.
    ** Before the addition of control blocks the client stuff 
    ** could be found with and instance id of '0'.  There is 
    ** generally only one GCA client and by using STprint() 
    ** the instance ID will still be '0'.  There are a number 
    ** of non-GCA entities which are looking for this instance 
    ** ID.
    */
    gca_cb->gca_cb_id = IIGCa_global.gca_initiate;
    STprintf( buff, "%d", gca_cb->gca_cb_id );
    MOattach( MO_INSTANCE_VAR, GCA_MIB_CLIENT, buff, (PTR)gca_cb );

    /*
    ** Flag successful initialization and bump
    ** the init counter so gca_term() can tell
    ** when to perform global shutdown.
    */
    gca_cb->gca_initialized = TRUE;
    IIGCa_global.gca_initiate++;

  complete:

    return;
}


/*
** Name: gca_trace_log
**
** Description:
**	MO callback function to set the GCA trace log.  Tracing is
**	enabled by passing in a non-zero length string.  Tracing
**	will be disabled if the passed in string is zero length.
**
** History:
**	17-Feb-98 (gordy)
**	    Created.
**	17-Jun-2004 (schak24)
**	    Input str is untrusted (env var), length-limit it.
*/

STATUS
gca_trace_log( i4  offset, i4  len, char *str, i4  size, PTR obj )
{
    CL_ERR_DESC		system_status;
    
    if ( IIGCa_global.gca_trace_log[0] )
	TRset_file( TR_F_CLOSE, NULL, 0, &system_status );

    STlcopy( str, IIGCa_global.gca_trace_log, sizeof(IIGCa_global.gca_trace_log)-1 );

    if ( IIGCa_global.gca_trace_log[0] )
	TRset_file( TR_F_OPEN, str, STlength( str ), &system_status );

    return( OK );
}


/*
**	GCA MIB Class definitions
*/	

GLOBALREF MO_CLASS_DEF	gca_classes[];

/*{
** Name: gca_init_mib() - initialize GCA MIB
**
** Description:
**      Issues MO calls to define all GCA MIB objects.
**
** Inputs:
**      None.
**
** Side effects:
**      MIB objects defined to MO.
**
** Called by:
**      gca_init()
**
** History:
**      01-Oct-92 (brucek)
**          Created.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      13-Apr-94 62240
**          generalize init to look at all objects in the array, and
**  	    fill in IIGCa_static for those with compile-time cdata of -1.
**	 3-Sep-96 (gordy)
**	    Restructured GCA global data.
*/

static VOID
gca_init_mib( VOID )
{
    MOclassdef( MAXI2, gca_classes );

    return;
}


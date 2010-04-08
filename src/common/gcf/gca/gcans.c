/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>

# include <cv.h>
# include <er.h>
# include <gc.h>
# include <id.h>
# include <lo.h>
# include <mu.h>
# include <pm.h>
# include <qu.h>
# include <si.h>
# include <sl.h>
# include <st.h>

# include <iicommon.h>
# include <gca.h>
# include <gcaint.h>
# include <gcn.h>
# include <gcnint.h>
# include <gcu.h>

#ifdef WIN16
bool	gcpasswd_prompt(char*, char *, char *);
#endif /* WIN16 */


/*
** Name: gcans.c 
**
** Description:
**	Routines to access the embedded Name Server interface
**	replacing GCA communications with the Name Server.
**
** History:
**	 4-Dec-95
**	    Reworked for new embedded Name Server interface.
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
**	 29-may-96 (chech02)
**	    Added call to gcpasswd_prompt()  to support IIPROMPTALL,
**	    IIPROMPTONCE, and IIPROMPT1 for windows 3.1 port.
**	 26-aug-96 (rosga02)
**	    ifdefed  include <gcnint.h> to avoid compiling errors
**          for platforms where GCF_EMBEDDED_GCN is not used
**	 1-Oct-96 (gordy)
**	    Name Server now uses local username to access private vnodes.
**	    Process the local username/password info even if the remote
**	    info is provided.  Provide the local username in the request
**	    control block along with the existing info.
**	 1-Oct-96 (gordy)
**	    Need to include si.h since gcnint.h uses the FILE datatype.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable overrides.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	23-May-97 (thoda04)
**	    Included cv.h for function prototypes.  WIN16 fixes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Aug-09 (gordy)
**	    Remove string length restrictions.
*/



/*
** Name: gca_ns_init
**
** Description:
**	Initialize the embedded Name Server interface.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	 4-Dec-95 (gordy)
**	    Created.
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable overrides.
*/

STATUS
gca_ns_init( VOID )
{
#ifdef GCF_EMBEDDED_GCN

    char	*ptr, host[ 50 ];
    char	*pm_default[ PM_MAX_ELEM ];
    i4		count;
    STATUS	status = OK;

    /*
    ** We do not want to initialize the embedded Name Server
    ** interface if this is the actual Name Server.  The Name
    ** Server initializes the interface before initializing
    ** GCA, so check to see if IIGCn_static is initialized,
    ** indicating that gcn_init() has already been called.
    */
    if ( IIGCn_static.maxsessions )  return( OK );

    /*
    ** Check to see if tracing of Name Server code
    ** is enabled (for whoever we are) before resetting
    ** PM for the Name Server.
    */
    gcu_get_tracesym( "II_GCN_TRACE", "!.gcn_trace_level", &ptr );
    if ( ptr  &&  *ptr )  CVal( ptr, &IIGCn_static.trace );

    /*
    ** Save current PM defaults and set up for Name Server.
    */
    for( count = 0; count < PM_MAX_ELEM; count++ )
	pm_default[ count ] = PMgetDefault( count );

    PMsetDefault( 0, SystemCfgPrefix );	/* Name Server defaults */
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx( "gcn" ) );

    for( count = 3; count < PM_MAX_ELEM; count++ )
	PMsetDefault( count, NULL );

    /*
    ** Initialize embedded Name Server.
    */
    GChostname( host, sizeof( host ) );
    if ( gcn_srv_init() != OK  ||  gcn_srv_load( host, NULL ) != OK )
	status = E_GC0013_ASSFL_MEM;

    /*
    ** Reset PM defaults.
    */
    for( count = 0; count < PM_MAX_ELEM; count++ )
	PMsetDefault( count, pm_default[ count ] );


    return( status );

#else /* GCA_EMBEDDED_GCN */

    return( OK );

#endif /* GCA_EMBEDDED_GCN */
}


/*
** Name: gca_ns_term
**
** Description:
**	Terminate the embedded Name Server interface.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 4-Dec-95 (gordy)
**	    Created.
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
*/

VOID
gca_ns_term( VOID )
{
#ifdef GCF_EMBEDDED_GCN

    gcn_srv_term();

#endif /* GCF_EMBEDDED_GCN */

    return;
}


/*
** Name: gca_ns_resolve
**
** Description:
**	Resolve a formatted database reference (vnode, db, class)
**	using the internal Name Database interface.  Pointers to
**	the results are returned in the ns structure of the request
**	control block, rqcb.  Since the storage for these values
**	must persist for the duration of the request processing,
**	the Name Server control block, rqcb->grcb, is used to call
**	the embedded Name Server interface.
**
** Input:
**	target		Formatted database reference.
**	user		Local user ID, may be NULL.
**	password	Local password, may be NULL.
**	rem_user	Remote user ID, may be NULL.
**	rem_pass	Remote password, may be NULL.
**
** Output:
**	rqcb		Request Control Block
**	    ns		    GCN resolve info
**	    grcb	    Storage for ns values.
**
** Returns:
**	STATUS
**
** History:
**	 4-Dec-95 (gordy)
**	    Created.
**	28-May-96 (gordy)
**	    Remove support for embedded Name Server unless explicitly
**	    called for by compilation symbol.  Otherwise, GCN support
**	    routines must be provided in libingres which we don't
**	    currently wish to do because it makes the encryption
**	    routines readily available to anyone snooping the library.
**	 1-Oct-96 (gordy)
**	    Name Server now uses local username to access private vnodes.
**	    Process the local username/password info even if the remote
**	    info is provided.  Provide the local username in the request
**	    control block along with the existing info.
**	11-Aug-09 (gordy)
**	    Provide more appropriate size for username buffer.
*/

STATUS
gca_ns_resolve( char *target, char *user, char *password,
		char *rem_user, char *rem_pass, GCA_RQCB *rqcb )
{
#ifdef GCF_EMBEDDED_GCN

    GCA_RQNS		*ns = &rqcb->ns;
    GCN_RESOLVE_CB	*grcb;
    char		uid[ GC_USERNAME_MAX + 1 ];
    char		*puid = uid;
    char		empty[ 1 ] = { '\0' };
    STATUS		status;
    i4			i;

    /*
    ** Allocate the embedded Name Server interface control block
    ** and save it in the request control block to provide storage
    ** for the resolved values.
    */
    if ( ! rqcb->grcb  &&  ! (rqcb->grcb = gca_alloc(sizeof(GCN_RESOLVE_CB))) )
	return( E_GC0013_ASSFL_MEM );

    grcb = (GCN_RESOLVE_CB *)rqcb->grcb;

    /*
    ** Default local username and password if required.
    ** Only use the provided info if both a username and
    ** password are provided.
    */
    IDname( &puid );
    STzapblank( uid, uid );

    if ( ! user )  user = empty;
    if ( ! password )  password = empty;

    if ( ! user[0]  ||  ! password[ 0 ] )
    {
	user = uid;
	password = empty;
    }

    /*
    ** Validate username and password if attempt is
    ** made to change the user's ID.  This would
    ** normally be done in the Name Server GCA_LISTEN.
    */
    if ( user != uid  &&  STcompare( user, uid ) )
    {
	CL_ERR_DESC sys_err;
	status = GCusrpwd( user, password, &sys_err );
	if ( status != OK )  return( status );
    }

    /*
    ** Setup input parameters to Name Database interface.
    */
    STcopy( target, grcb->target );
    STcopy( user, grcb->user );

    if ( rem_user  &&  rem_pass )
    {
	/*
	** Use the requested remote username and password.
	*/
	grcb->gca_message_type = GCN_NS_2_RESOLVE;
	STcopy( rem_user, grcb->user_in ); 
	STcopy( rem_pass, grcb->passwd_in );
    }
    else
    {
	/*
	** Use the local username and password.
	*/
	grcb->gca_message_type = GCN_NS_RESOLVE;
	STcopy( user, grcb->user_in ); 
	STcopy( password, grcb->passwd_in );
    }

    /*
    ** Call embedded Name Server to resolve the target.
    */
    if ( 
	 (status = gcn_rslv_parse( grcb ))  == OK  &&
         (status = gcn_rslv_server( grcb )) == OK  &&
         (status = gcn_rslv_node( grcb ))   == OK  &&
         (status = gcn_rslv_login( grcb ))  == OK 
       )
    {

#ifdef WIN16
   	gcpasswd_prompt(grcb->user_out, grcb->passwd_out, grcb->vnode);
#endif /* WIN16 */

	/*
	** Return resolved info.  The new partner ID should
	** have the vnode removed, but still retain the server
	** class.  We use the grcb as static storage to rebuild
	** the partner ID from the dbname and class components.
	*/
	ns->svr_class = grcb->class ? grcb->class : "";
	ns->username = grcb->user_out ? grcb->user_out : "";
	ns->password = grcb->passwd_out ? grcb->passwd_out : "";
	ns->rmt_dbname = grcb->target;

	STprintf( grcb->target, "%s%s%s", 
		  grcb->dbname ? grcb->dbname : "", 
		  ns->svr_class[0] ? "/" : "", ns->svr_class );

	for( i = 0, ns->lcl_count = grcb->catl.tupc; i < ns->lcl_count; i++ )
	    ns->lcl_addr[ i ] = grcb->catl.tupv[ i ];

	for( i = 0, ns->rmt_count = grcb->catr.tupc; i < ns->rmt_count; i++ )
	{
	    char *pv[3];

	    gcu_words( grcb->catr.tupv[ i ], NULL, pv, ',', 3 );

	    ns->node[ i ] = pv[ 0 ];
	    ns->protocol[ i ] = pv[ 1 ];
	    ns->port[ i ] = pv[ 2 ];
	}
	
	if ( ! ns->lcl_count )  status = E_GC0021_NO_PARTNER;
    }

    return( status );

#else /* GCF_EMBEDDED_GCN */

    return( E_GC0021_NO_PARTNER );

#endif /* GCF_EMBEDDED_GCN */
}


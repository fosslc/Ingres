/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <cm.h>
#include <er.h>
#include <gc.h>
#include <me.h>
#include <nm.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <dbms.h>
#include <gca.h>
#include <gcs.h>


/*
**
** Name: gcsinfo.c
**
** Description:
**	GCS information display.  Tests loading/initialization 
**	of requested mechanisms and display any errors/messages 
**	logged during GCS initialization.  If GCS initialization 
**	is successful, displays information on GCS mechanisms.
**
**	GCS tracing (if enabled) is also displayed.
**
** History:
**	25-Jul-97 (gordy)
**	    Created.
**	17-Mar-98 (gordy)
**	    Use command line argument to generate PM default settings.
**	    This will permit initialization of GCS with the same config
**	    settings as a server.
**	20-Mar-98 (gordy)
**	    Added mechanism encryption level.
**	31-Mar-98 (gordy)
**	    Added indicator for installation mechanism.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 9-Jul-04 (gordy)
**	    GCS configuration now supports default mechanisms for
**	    individual operations.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
*/

/*
**      mkming hints
NEEDLIBS =      GCFLIB SHCOMPATLIB
OWNER =         INGUSER
PROGRAM =       gcsinfo
*/

static void	check_def_mech( char *, GCS_MECH, u_i4 );
static VOID	display_error( char *, STATUS, CL_ERR_DESC *, i4, PTR );
static VOID	pm_err_callback( STATUS, i4, ER_ARGUMENT * );
static VOID	error_log( STATUS, i4, PTR );
static VOID	message_log( char * );


/*
** Name: main
**
** Description:
**
** Inputs:
**	argc		Number of command line arguments.
**	argv		Command line arguments.
**
** Outputs:
**	None.
**
** Returns:
**	i4		0
**
** History:
**	25-Jul-97 (gordy)
**	    Created.
**	17-Mar-98 (gordy)
**	    Use command line argument to generate PM default settings.
**	    This will permit initialization of GCS with the same config
**	    settings as a server.
**	31-Mar-98 (gordy)
**	    Added indicator for installation mechanism.
**	 9-Jul-04 (gordy)
**	    Indicate GCS operation default mechanisms by case of the
**	    capability character.  Check availability and capability
**	    of the default mechanisms.
*/

i4
main( i4  argc, char **argv )
{
    GCS_INIT_PARM	gcs_init;
    GCS_INFO_PARM	gcs_info;
    STATUS		status;
    CL_ERR_DESC		clerr;

    /*
    ** Initialize CL components.
    */
    MEadvise( ME_INGRES_ALLOC );
    SIeqinit();

    status = CMset_charset(&clerr);
    if (status != OK)
    {
	display_error( "Init charset", status, &clerr, 0, NULL );
	PCexit( FAIL );
    }
    
    switch( (status = PMinit()) )
    {
	case OK :		/* Success */
	case PM_NO_INIT :	/* Duplicate init OK */
	    break;

	default :
	    display_error( "Init config", status, NULL, 0, NULL );
	    PCexit( FAIL );
    }

    switch( PMload( NULL, pm_err_callback ) )
    {
	case OK :  break;

	case PM_FILE_BAD :
	    display_error( "Init config", E_GC003D_BAD_PMFILE, NULL, 0, NULL );
	    PCexit( FAIL );
	
	default :
	    display_error( "Init config", E_GC003E_PMLOAD_ERR, NULL, 0, NULL );
	    PCexit( FAIL );
    }

    if ( argc <= 1 )
    {
	PMsetDefault( 0, SystemCfgPrefix );
	PMsetDefault( 1, PMhost() );
    }
    else
    {
	char	*val[ 10 ];
	i4	i, cnt;

	cnt = gcu_words( argv[ 1 ], NULL, val, '.', 10 ); 
	for( i = 0; i < cnt; i ++ )  PMsetDefault( i, val[ i ] );
    }

    if ((status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &clerr)) != OK)
	display_error( "Init trace", status, &clerr, 0, NULL );

    /*
    ** Initialize GCS and display info.
    */
    MEfill( sizeof( gcs_init ), '\0', &gcs_init );
    gcs_init.version = GCS_API_VERSION_2;
    gcs_init.err_log_func = (PTR)error_log;
    gcs_init.msg_log_func = (PTR)message_log;

    status = IIgcs_call( GCS_OP_INIT, GCS_NO_MECH, (PTR)&gcs_init );

    if ( status != OK )
	display_error( "Error initializing GCS", status, NULL, 0, NULL );
    else
    {
	MEfill( sizeof( gcs_info ), '\0', &gcs_info );

	status = IIgcs_call( GCS_OP_INFO, GCS_NO_MECH, (PTR)&gcs_info );

	if ( status != OK )
	    display_error( "Error obtaining GCS information", 
			   status, NULL, 0, NULL );
	else
	{
	    GCS_MECH	install_mech = GCS_NO_MECH;
	    GCS_MECH	usr_mech = GCS_NO_MECH;
	    GCS_MECH	pwd_mech = GCS_NO_MECH;
	    GCS_MECH	srv_mech = GCS_NO_MECH;
	    GCS_MECH	rem_mech = GCS_NO_MECH;
	    GCS_MECH	ip_mech = GCS_NO_MECH;
	    GCS_MECH	enc_mech = GCS_NO_MECH;
	    i4  	i;

	    install_mech = gcs_mech_id( "default" );
	    usr_mech = gcs_default_mech( GCS_OP_USR_AUTH );
	    pwd_mech = gcs_default_mech( GCS_OP_PWD_AUTH );
	    srv_mech = gcs_default_mech( GCS_OP_SRV_AUTH );
	    rem_mech = gcs_default_mech( GCS_OP_REM_AUTH );
	    ip_mech = gcs_default_mech( GCS_OP_IP_AUTH );
	    enc_mech = gcs_default_mech( GCS_OP_E_ENCODE );

	    SIprintf( "\n%-14s %-3s %-3s %-6s %-3s %-10s\n",
		      "Mechanism Name", "ID ", "Ver", 
		      " Caps ", "Lvl", "  Status  " );
	    SIprintf( "%-14s %-3s %-3s %-6s %-3s %-10s\n", 
		      "--------------", "---", "---", 
		      "------", "---", "----------" );

	    for( i = 0; i < gcs_info.mech_count; i++ )
	    {
		char	caps[ 10 ];
		char	*ptr = caps;

		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_USR_AUTH )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == usr_mech) 
			     ? 'U' : 'u';
		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_PWD_AUTH )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == pwd_mech)
			     ? 'P' : 'p';
		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_SRV_AUTH )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == srv_mech)
			     ? 'S' : 's';
		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_IP_AUTH )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == ip_mech)
			     ? 'I' : 'i';
		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_REM_AUTH )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == rem_mech)
			     ? 'R' : 'r';
		if ( gcs_info.mech_info[i]->mech_caps & GCS_CAP_ENCRYPT )  
		    *ptr++ = (gcs_info.mech_info[i]->mech_id == enc_mech)
			     ? 'E' : 'e';
		*ptr = EOS;

		SIprintf( "%-14s %3d %3d %-6s %3d %s%-9s\n", 
			  gcs_info.mech_info[i]->mech_name,
			  gcs_info.mech_info[i]->mech_id,
			  gcs_info.mech_info[i]->mech_ver, caps,
			  gcs_info.mech_info[i]->mech_enc_lvl,
	                  (gcs_info.mech_info[i]->mech_id == install_mech)
			      ? "*" : "",
			  (gcs_info.mech_info[i]->mech_status == GCS_AVAIL)
			      ? "Available" : "Disabled" );
	    }

	    SIprintf( "\n" );

	    check_def_mech( "User", usr_mech, GCS_CAP_USR_AUTH );
	    check_def_mech( "Password", pwd_mech, GCS_CAP_PWD_AUTH );
	    check_def_mech( "Server", srv_mech, GCS_CAP_SRV_AUTH );
	    check_def_mech( "Remote", rem_mech, GCS_CAP_REM_AUTH );
	    check_def_mech( "Installation Password", ip_mech, GCS_CAP_IP_AUTH );

#if 0
/*
** There is no current configuration
** for a default encryption mechanism.
*/
	    check_def_mech( "Encryption", enc_mech, GCS_CAP_ENCRYPT );
#endif

	}

	status = IIgcs_call( GCS_OP_TERM, GCS_NO_MECH, NULL );

	if ( status != OK )
	    display_error( "Error shutting down GCS", status, NULL, 0, NULL );
    }

    PCexit( OK );
    return( 0 );
}


/*
** Name: check_def_mech
**
** Description:
**	Checks a mechanism for availability and capability.
**	Since GCS capabilities are bit flags, caller must
**	ensure that only a single capability is provided to
**	avoid erroneous results.
**
** Input:
**	cap_desc	Description of capability.
**	mech_id		GCS mechanism ID.
**	cap		GCS capability.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Jul-04 (gordy)
**	    Created.
*/

static void
check_def_mech( char *cap_desc, GCS_MECH mech_id, u_i4 cap )
{
    GCS_INFO_PARM	info;

    if ( mech_id == GCS_NO_MECH )
	SIprintf( "%s auth mechanism is not configured\n", cap_desc );
    else  if ( IIgcs_call( GCS_OP_INFO, mech_id, (PTR)&info ) != OK )
	SIprintf( "%s auth mechanism is unavailable\n", cap_desc );
    else  if ( info.mech_info[0]->mech_status != GCS_AVAIL )
	SIprintf( "%s auth mechanism '%s' is disabled\n",
		  cap_desc, info.mech_info[0]->mech_name );
    else  if ( ! (info.mech_info[0]->mech_caps & cap) )
	SIprintf( "%s auth mechanism '%s' does not have that capability\n",
		  cap_desc, info.mech_info[0]->mech_name );

    return;
}


/*
** Name: display_error
**
** Description:
**	Outputs error message info.
**
** Input:
**	tag		String to identify caller activity.
**	status		Error code.
**	clerr		CL error information.
**	count		Number of parameters.
**	parms		Array of ER_ARGUMENT parameters.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	25-Jul-97 (gordy)
**	    Created.
*/

static VOID
display_error( char *tag, STATUS status, 
	       CL_ERR_DESC *clerr, i4  count, PTR parms )
{
    STATUS	er_stat;
    CL_ERR_DESC	er_clerr;
    char	buff[ ER_MAX_LEN ];
    i4		len;
    i4		lang = 1;	/* Not sure if this should be anything else? */

    if ( status != OK  &&  status != FAIL )
    {
	er_stat = ERlookup( status, NULL, 0, NULL, buff, sizeof( buff ), lang, 
			    &len, &er_clerr, count, (ER_ARGUMENT *)parms );

	if ( er_stat != OK )
	    STprintf( buff, "Couldn't find text for error 0x%x\n", status );
	else 
	    buff[ len ] = EOS;

	SIprintf( "%s: %s\n", tag, buff );
    }

    if ( clerr  &&  (status == FAIL  ||  CLERROR( status )) )
    {
	er_stat = ERlookup( OK, clerr, 0, NULL, buff, sizeof( buff ), lang, 
			    &len, &er_clerr, 0, NULL );

	if ( er_stat != OK )  len = 0;

	if ( len )
	{
	    buff[ len ] = EOS;
	    SIprintf( "%s: %s\n", tag, buff );
	}
    }

    return;
}


/*
** Callback functions for error processing.
*/

static VOID
pm_err_callback( STATUS status, i4  cnt, ER_ARGUMENT *parms )
{
    display_error( "Init config", status, NULL, cnt, (PTR)parms );
    return;
}

static VOID
error_log( STATUS error_code, i4  parm_count, PTR parms )
{
    display_error( "GCS error", error_code, NULL, parm_count, parms );
    return;
}

static VOID
message_log( char *message )
{
    SIprintf( "GCS message: %s\n", message );
    return;
}


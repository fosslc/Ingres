/*
** Copyright (c) 1998, 2004 Ingres Corporation All Rights Reserved.
*/

#include <compat.h>
#include <gl.h>

#include <er.h>
#include <gc.h>
#include <me.h>
#include <pc.h>
#include <qu.h>
#include <si.h>
#include <sp.h>
#include <st.h>
#include <te.h>
#include <tr.h>

#include <mo.h>
#include <sl.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcm.h>

/*
** Name: iibrowse.sc
**
** Description:
**	Ingres Installation Browser.  Demonstrates/Tests the
**	capabilities of the Ingres Installation Registry.
**
**	Prompts for target host name.  Local browsing may be
**	done by omitting host name.  If host name provided,
**	prompts for username and password for target host.
**
**	Connects to installation registry master Name Server
**	on target host and obtains registered Name Server
**	and installation information for display using Name
**	Server (GCN protocol) requests.  Prompts for 
**	installation ID to browse.
**
**	Connects to Name Server in selected installation and
**	retrieves registered server information using GCM/MO
**	requests.  Connects to Communications Servers in 
**	installation and obtains active protocols and ports
**	using GCM/MO.  Connect to Ingres DBMS servers in
**	installation and obtains list of databases in the
**	installation using ESQL.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	21-Oct-98 (gordy)
**	    Check to see if registry is active on target host
**	    before prompting for username and password.
**	05-feb-1999 (somsa01)
**	    NULL fill in_parms before using it, so as to clear out any
**	    garbage.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Apr-04 (gordy)
**	    Added GCM lookup to allow remote browsing without user ID.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries. Added NEEDLIBSW hint which is used
**	    for compiling on windows and compliments NEEDLIBS.	    
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** mkming hints
**
NEEDLIBS =      SHQLIB COMPATLIB 
NEEDLIBSW = 	SHEMBEDLIB SHGCFLIB 
OWNER =         INGUSER
PROGRAM =       iibrowse
*/

EXEC SQL INCLUDE SQLCA;

typedef	struct
{

    QUEUE	q;
    char	id[ GCN_VAL_MAX_LEN + 1 ];
    char	addr[ GCN_VAL_MAX_LEN + 1 ];

} INST_DATA;

typedef struct
{

    QUEUE	q;
    char	*data[1];	/* Variable length */

} GCM_DATA;

typedef struct
{

    char	*classid;
    char	value[ GCN_VAL_MAX_LEN + 1 ];

} GCM_INFO;

/*
** Name Server GCM information for registered installations.
*/
static	GCM_INFO	inst_info[] =
{
    { GCN_MIB_SERVER_CLASS, {'\0'} },
    { GCN_MIB_SERVER_ADDRESS, {'\0'} },
    { GCN_MIB_SERVER_OBJECT, {'\0'} }
};

/*
** Name Server GCM information for registered servers.
*/
static	GCM_INFO	svr_info[] = 
{
    { GCN_MIB_SERVER_CLASS, {'\0'} },
    { GCN_MIB_SERVER_ADDRESS, {'\0'} },
    { GCN_MIB_SERVER_OBJECT, {'\0'} }
};

/*
** Communications Server GCM information for protocols.
*/
static	GCM_INFO	net_info[] =
{
    { GCC_MIB_PROTOCOL, {'\0'} },
    { GCC_MIB_PROTO_PORT, {'\0'} }
};

#define ARRAY_SIZE( array )	(sizeof( array ) / sizeof( (array)[0] ))

static	i4	msg_max;
static	char	*msg_buff;

EXEC SQL BEGIN DECLARE SECTION;

static	char	username[ 32 ];
static	char	password[ 32 ];

EXEC SQL END DECLARE SECTION;

/*
** Local Functions.
*/
static	bool	ibr_ping( char * );
static	STATUS	ibr_install_gcn( char *, QUEUE * );
static	STATUS	ibr_install_gcm( char *, QUEUE * );
static	VOID	ibr_unload_install( QUEUE * );
static	VOID	ibr_install( char *, QUEUE * );
static	STATUS	ibr_load_server( char *, char *, QUEUE * );
static	STATUS	ibr_load_proto( char *, QUEUE *, QUEUE * );
static	STATUS	ibr_show_db( char *, QUEUE * );
static	STATUS	gcm_load(char *, i4, GCM_INFO *, char *, i4, char *, QUEUE *);
static	VOID	gcm_unload( i4, QUEUE * );
static	VOID	gcm_display( i4, QUEUE * );
static	STATUS	gca_request( char *, i4 * );
static	STATUS	gca_fastselect( char *, i4 *, i4 *, char *, i4 );
static	STATUS	gca_receive( i4, i4 *, i4 *, char * );
static	STATUS  gca_send( i4, i4, i4, char * );
static	STATUS	gca_release( i4 );
static	STATUS	gca_disassoc( i4 );
static	VOID	getrec_noecho( i4, char * );
static	VOID	esql_request( char *, char * );


/*
** Name: main
**
** Description:
**	Program entry point.
**
**	Loops prompting for host name (and possibly username
**	and password), loads installation information and
**	passes to function for display/process.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	21-Oct-98 (gordy)
**	    Check to see if registry is active on target host
**	    before prompting for username and password.
**	05-feb-1999 (somsa01)
**	    NULL fill in_parms before using it, so as to clear out any
**	    garbage.
**	 6-Apr-04 (gordy)
**	    Added GCM lookup to allow remote browsing without user ID.
*/

main()
{
    STATUS		status;
    STATUS		call_status;
    GCA_IN_PARMS	in_parms;
    GCA_TE_PARMS	te_parms;
    CL_ERR_DESC		err_code;
    QUEUE		install;
    char		response[80];
    char		host[80];

    /*
    ** CL initialization.
    */
    MEadvise(ME_INGRES_ALLOC);
    SIeqinit();
    QUinit( &install );
    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &err_code);

    SIprintf("\nIngres Installation Browser\n");

    /*
    ** Initialize communications.
    */
    MEfill(sizeof(in_parms), (u_char)0, (PTR)&in_parms);

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
    in_parms.gca_api_version = GCA_API_LEVEL_5;
    in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

    call_status = IIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms,
                              GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  in_parms.gca_status != OK )
	status = in_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )  
    {
	SIprintf( "\nGCA_INITIATE failed: 0x%x\n", status );
	PCexit( FAIL );
    }

    msg_max = GCA_FS_MAX_DATA + in_parms.gca_header_length;

    if ( ! (msg_buff = (char *)MEreqmem( 0, msg_max, FALSE, NULL )) )
    {
	PCexit( FAIL );
    }

    do
    {
	/*
	** Prompt for host name.
	*/
	SIprintf( "\nTarget Host Name (<CR> for local): " );
	SIflush( stdout );
	SIgetrec( response, sizeof( response ), stdin );
        STtrmwhite( response );
	STcopy( response, host );
	*username = *password = EOS;

	/*
	** Check if registry is active.
	*/
	if ( ibr_ping( host ) )
	{
	    /*
	    ** Remote browsing requires username and password.
	    */
	    if ( *host )
	    {
		SIprintf( "Username: " );
		SIflush( stdout );
		SIgetrec( response, sizeof( response ), stdin );
		STtrmwhite( response );
		STcopy( response, username );

		if ( *username )
		{
		    SIprintf( "Password: " );
		    SIflush( stdout );
		    getrec_noecho( sizeof( response ), response );
		    STtrmwhite( response );
		    STcopy( response, password );
		    if ( ! *password )  *username = EOS;
		    SIprintf( "\n" );
		}
	    }

	    /*
	    ** Connect to master Name Server and load
	    ** installation information.
	    */
	    if ( ! *host  ||  *username )
		status = ibr_install_gcn( host, &install );
	    else
		status = ibr_install_gcm( host, &install );

	    if ( status == OK )
	    {
		/*
		** Display/process installation information.
		*/
		ibr_install( host, &install );
		ibr_unload_install( &install );
	    }
	}

	SIprintf( "\nSelect new Host? " );
	SIflush( stdout );
	SIgetrec( response, sizeof( response ), stdin );

    } while( response[0] == 'y'  ||  response[0] == 'Y' );

    /*
    ** Shutdown communications.
    */
    call_status = IIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms,
                              GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  te_parms.gca_status != OK )
	status = te_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_TERMINATE failed: 0x%x\n", status );

    PCexit( OK );
}


/*
** Name: ibr_ping
**
** Description:
**	Checks for active registry on target host.  Local connections
**	are always considered active since the local Name Server always
**	provides some degree of registry support even if the registry
**	is disabled for the current installation.
**
** Input:
**	host		Target host name, zero length string for local.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if registry active.  FALSE otherwise.
**
** History:
**	21-Oct-98 (gordy)
**	    Created.
*/

static bool
ibr_ping( char *host )
{
    STATUS	status;
    char	target[ 64 ];
    i4		assoc_id;
    bool	active = TRUE;

    if ( *host )
    {
	/*
	** Build target specification for master Name Server:
	** @::/iinmsvr for local, @host::/iinmsvr for remote.
	** Use GCF reserved internal user ID for remote.
	*/
	STprintf( target, "@%s::/iinmsvr", host );
	STcopy( "*", username );
	STcopy( "*", password );

	/*
	** Connect to master Name Server.
	*/
	if ( (status = gca_request( target, &assoc_id )) != OK )
	    active = FALSE;
	else
	    gca_release( assoc_id );

	gca_disassoc( assoc_id );
	*username = *password = EOS;
    }

    return( active );
}


/*
** Name: ibr_install_gcn
**
** Description:
**	Connects to installation registry master Name Server
**	on target host.  If target host name is zero length
**	string, connection is made to master Name Server on
**	local host.
**
**	Uses GCN_NS_OPER request to obtain installation info
**	from master Name Server and adds INST_DATA entries
**	to the install queue.  The install queue may be
**	cleared by calling ibr_unload_install().
**
**	No entries are added to the queue if an error code
**	is returned.
**
** Input:
**	host		Target host name, zero length string for local.
**	install		Queue to receive installation information.
**
** Output:
**	install		Queue entries.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	 6-Apr-04 (gordy)
**	    Renamed as companion to new ibr_install_gcm().
*/

static STATUS
ibr_install_gcn( char *host, QUEUE *install )
{
    STATUS	status;
    char	target[ 64 ];
    char	*ptr = msg_buff;
    i4		assoc_id, msg_type;
    i4		msg_len = msg_max;

    /*
    ** Build target specification for master Name Server:
    ** @::/iinmsvr for local, @host::/iinmsvr for remote.
    */
    STprintf( target, "@%s::/iinmsvr", host );

    /*
    ** Connect to master Name Server.
    */
    if ( (status = gca_request( target, &assoc_id )) == OK )
    {
	/*
	** Build and send GCN_NS_OPER request.
	*/
	ptr += gcu_put_int( ptr, GCN_DEF_FLAG );
	ptr += gcu_put_int( ptr, GCN_RET );
	ptr += gcu_put_str( ptr, "" );
	ptr += gcu_put_int( ptr, 1 );
	ptr += gcu_put_str( ptr, "NMSVR" );
	ptr += gcu_put_str( ptr, "" );
	ptr += gcu_put_str( ptr, "" );
	ptr += gcu_put_str( ptr, "" );

	status = gca_send( assoc_id, GCN_NS_OPER, ptr - msg_buff, msg_buff );

	/*
	** Receive response.
	*/
	if ( status == OK )
	    status = gca_receive( assoc_id, &msg_type, &msg_len, msg_buff );

	if ( status == OK )
	    switch( msg_type )
	    {
		case GCA_ERROR :
		{
		    i4	ele_cnt, prm_cnt, val;

		    ptr = msg_buff;
		    ptr += gcu_get_int( ptr, &ele_cnt );

		    while( ele_cnt-- )
		    {
			ptr += gcu_get_int( ptr, &status );
			ptr += gcu_get_int( ptr, &val ); /* skip id_server */
			ptr += gcu_get_int( ptr, &val ); /* skip server_type */
			ptr += gcu_get_int( ptr, &val ); /* skip severity */
			ptr += gcu_get_int( ptr, &val ); /* skip local_error */
			ptr += gcu_get_int( ptr, &prm_cnt );

			while( prm_cnt-- )
			{
			    ptr += gcu_get_int( ptr, &val );
			    ptr += gcu_get_int( ptr, &val );
			    ptr += gcu_get_int( ptr, &val );
			    SIprintf("\nERROR 0x%x: %.*s\n", status, val, ptr);
			    ptr += val;
			}
		    }
		}
		break;
		
		case GCN_RESULT :
		{
		    INST_DATA	*data;
		    char	*val;
		    i4		i, count;

		    ptr = msg_buff;
		    ptr += gcu_get_int( ptr, &count );	/* skip gcn_op */
		    ptr += gcu_get_int( ptr, &count );

		    for( i = 0; i < count; i++ )
		    {
			/*
			** Add entry to install queue.
			*/
			data = (INST_DATA *)MEreqmem( 0, sizeof( *data ), 
						      FALSE, NULL );
			if ( ! data )
			{
			    SIprintf( "\nMemory allocation failed\n" );
			    break;
			}

			QUinsert( &data->q, install->q_prev );

			/*
			** Installation ID is is in gcn_obj
			** and Name Server listen address is
			** in gcn_val.
			*/
			ptr += gcu_get_str( ptr, &val );	/* skip type */
			ptr += gcu_get_str( ptr, &val );	/* skip uid */
			ptr += gcu_get_str( ptr, &val );	
			STcopy( val, data->id );
			ptr += gcu_get_str( ptr, &val );
			STcopy( val, data->addr );
		    }
		}
		break;

		default :
		    SIprintf( "\nInvalid GCA message type: %d\n", msg_type );
		    break;
	    }

	gca_release( assoc_id );
    }

    gca_disassoc( assoc_id );

    return( status );
}


/*
** Name: ibr_install_gcm
**
** Description:
**	Connects to installation registry master Name Server
**	on target host.  If target host name is zero length
**	string, connection is made to master Name Server on
**	local host.
**
**	Uses FASTSELECT request to obtain installation info
**	from master Name Server and adds INST_DATA entries
**	to the install queue.  The install queue may be
**	cleared by calling ibr_unload_install().
**
**	No entries are added to the queue if an error code
**	is returned.
**
** Input:
**	host		Target host name, zero length string for local.
**	install		Queue to receive installation information.
**
** Output:
**	install		Queue entries.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 6-Apr-04 (gordy)
**	    Created.
*/

static STATUS
ibr_install_gcm( char *host, QUEUE *install )
{
    STATUS	status;
    QUEUE	inst;
    GCM_DATA	*data;
    char	target[ 64 ];
    char	beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    bool	generic = FALSE;

    /*
    ** Build target specification for master Name Server:
    ** @::/iinmsvr for local, @host::/iinmsvr for remote.
    */
    STprintf( target, "@%s::/iinmsvr", host );
    QUinit( &inst );

    /*
    ** Use GCF reserved internal user ID for remote if needed.
    */
    if ( *host  &&  ! *username )
    {
	STcopy( "*", username );
	STcopy( "*", password );
	generic = TRUE;
    }

    do
    {
	status = gcm_load( target, ARRAY_SIZE( inst_info ), inst_info,
			   beg_inst, msg_max, msg_buff, &inst );
    } while( status == FAIL );

    if ( generic )  *username = *password = '\0';

    if ( status != OK )
    {
	gcm_unload( ARRAY_SIZE( inst_info ), &inst );
	return( status );
    }

    for( 
	 data = (GCM_DATA *)inst.q_next;
	 (QUEUE *)data != &inst;
	 data = (GCM_DATA *)data->q.q_next
       )
    {
	INST_DATA *id;

	/*
	** Installations are represented by their
	** registry Name Server registrations.  
	** Ignore other server registrations.
	*/
        if ( STcasecmp( data->data[0], "NMSVR" ) )  continue;

	/*
	** Add entry to install queue.
	*/
	id = (INST_DATA *)MEreqmem( 0, sizeof( *id ), FALSE, NULL );

	if ( ! id )
	{
	    SIprintf( "\nMemory allocation failed\n" );
	    break;
	}

	QUinsert( &id->q, install->q_prev );
	STcopy( data->data[ 1 ], id->addr );
	STcopy( data->data[ 2 ], id->id );
    }

    gcm_unload( ARRAY_SIZE( inst_info ), &inst );
    return( status );
}


/*
** Name: ibr_unload_install
**
** Description:
**	Removes entries from the installation queue as built by
**	ibr_install_gc*(), and frees associated resources.
**
** Input:
**	install		Installation queue.
**
** Output:
**	install		Entries removed.
**
** Returns:
**	VOID
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
ibr_unload_install( QUEUE *install )
{
    INST_DATA	*data, *next;

    for( 
	 data = (INST_DATA *)install->q_next;
	 (QUEUE *)data != install;
	 data = next
       )
    {
	next = (INST_DATA *)data->q.q_next;
	QUremove( &data->q );
	MEfree( (PTR)data );
    }

    return;
}


/*
** Name: ibr_install
**
** Description:
**	Displays information in installation queue as built by
**	ibr_install_gc*().  Prompts for installation ID.  Loads
**	and displays registered server and protocol information
**	for selected installation.
**
** Input:
**	host		Target hostname.
**	install		Installation queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
ibr_install( char *host, QUEUE *install )
{
    INST_DATA	*data;
    QUEUE	server, proto;
    char	response[80];

    for(;;)
    {
	/*
	** Display installation info and prompt
	** for installation ID.
	*/
	SIprintf( "\nID\tAddr\n" );
	SIprintf( "--\t----\n" );

	for( 
	     data = (INST_DATA *)install->q_next;
	     (QUEUE *)data != install;
	     data = (INST_DATA *)data->q.q_next 
	   )
	    SIprintf( "%s\t%s\n", data->id, data->addr );

	SIprintf( "\nSelect installation ID (<CR> to exit): " );
	SIflush( stdout );
	SIgetrec( response, sizeof( response ), stdin );
        STtrmwhite( response );
	if ( ! response[0] )  break;		/* Done with this host. */

	/*
	** Search for selected installation ID.
	*/
	for( 
	     data = (INST_DATA *)install->q_next;
	     (QUEUE *)data != install;
	     data = (INST_DATA *)data->q.q_next 
	   )
	    if ( ! STcompare( response, data->id ) )
	    {
		QUinit( &server );
		QUinit( &proto );

		/*
		** Load and display registered servers.
		*/
		if ( ibr_load_server( host, data->addr, &server ) == OK )
		{
		    SIprintf( "\nClass\tAddr\tObj\n" );
		    SIprintf( "-----\t----\t---\n" );

		    gcm_display( ARRAY_SIZE( svr_info ), &server );

		    /*
		    ** Load and display protocols.
		    */
		    if ( ibr_load_proto( host, &server, &proto ) == OK )
		    {
			SIprintf( "\nProto\tAddr\n" );
			SIprintf( "-----\t----\n" );

			gcm_display( ARRAY_SIZE( net_info ), &proto );
			gcm_unload( ARRAY_SIZE( net_info ), &proto );
		    }

		    ibr_show_db( host, &server );
		    gcm_unload( ARRAY_SIZE( svr_info ), &server );
		}
		break;
	    }

	if ( (QUEUE *)data == install )
	    SIprintf( "\nInvalid installation ID: %s\n", response );
    }

    return;
}


/*
** Name: ibr_load_server
**
** Description:
**	Connects to Name Server listening at a known address on
**	target host.  If target host name is zero length string, 
**	connection is made to Name Server on local host.
**
**	Uses GCM FASTSELECT request to obtain registered server
**	info (svr_info array) from Name Server and adds GCM_DATA 
**	entries to the server queue.  The server queue may be 
**	cleared by calling gcm_unload().
**
**	No entries are added to the queue if an error code
**	is returned.
**
**
** Input:
**	host		Target host name, zero length string for local.
**	addr		Name Server listen address.
**	server		Server info queue.
**
** Output:
**	server		Entries added.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	 6-Apr-04 (gordy)
**	    Use generic access if user ID not provided.
*/

static STATUS
ibr_load_server( char *host, char *addr, QUEUE *server )
{
    STATUS	status = OK;
    char	target[ 64 ];
    char	beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    bool	generic = FALSE;

    /*
    ** Build target specification for Name Server:
    ** @::/@addr for local, @host::/@addr for remote.
    */
    STprintf( target, "@%s::/@%s", host, addr );

    /*
    ** Use GCF reserved internal user ID for remote if needed.
    */
    if ( *host  &&  ! *username )
    {
	STcopy( "*", username );
	STcopy( "*", password );
	generic = TRUE;
    }

    /*
    ** Issue GCM FASTSELECT requests to Name Server
    ** until all data retrieved or error occurs.
    */
    do
    {
	status = gcm_load( target, ARRAY_SIZE( svr_info ), svr_info,
			   beg_inst, msg_max, msg_buff, server );
    } while( status == FAIL );

    if ( generic )  *username = *password = '\0';
    if ( status != OK )  gcm_unload( ARRAY_SIZE( svr_info ), server );

    return( status );
}


/*
** Name: ibr_load_proto
**
** Description:
**	Scans the registered servers queue and connects to any
**	registered Comm Servers on the target host.  If target 
**	host name is zero length string, connections are made 
**	to Comm Servers on the local host.
**
**	Uses GCM FASTSELECT request to obtain network protocol
**	info (net_info array) from the Comm Servers and adds 
**	GCM_DATA entries to the protocol queue.  The protocol 
**	queue may be cleared by calling gcm_unload().
**
**	No entries are added to the queue if an error code
**	is returned.
**
**
** Input:
**	host		Target host name, zero length string for local.
**	server		Server info queue.
**	proto		Protocol info queue.
**
** Output:
**	proto		Entries added.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	 6-Apr-04 (gordy)
**	    Use generic access if user ID not provided.
*/

static STATUS
ibr_load_proto( char *host, QUEUE *server, QUEUE *proto )
{
    GCM_DATA	*data;
    STATUS	status = OK;
    char	target[ 64 ];
    char	beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    bool	generic = FALSE;

    /*
    ** Use GCF reserved internal user ID for remote if needed.
    */
    if ( *host  &&  ! *username )
    {
	STcopy( "*", username );
	STcopy( "*", password );
	generic = TRUE;
    }

    /*
    ** Scan the registered server queue looking for Comm Servers.
    */
    for(
	 data = (GCM_DATA *)server->q_next;
	 (QUEUE *)data != server;
	 data = (GCM_DATA *)data->q.q_next
       )
	if ( ! STcompare( GCA_COMSVR_CLASS, data->data[0] ) )
	{
	    /*
	    ** Build target specification for Comm Server:
	    ** @::/@addr for local, @host::/@addr for remote.
	    */
	    STprintf( target, "@%s::/@%s", host, data->data[1] );
	    /*
	    ** Issue GCM FASTSELECT requests to Name Server
	    ** until all data retrieved or error occurs.
	    */
	    do
	    {
		status = gcm_load( target, ARRAY_SIZE( net_info ), net_info,
				   beg_inst, msg_max, msg_buff, proto );
	    } while( status == FAIL );
	}

    if ( generic )  *username = *password = '\0';
    if ( status != OK )  gcm_unload( ARRAY_SIZE( net_info ), proto );

    return( status );
}


/*
** Name: ibr_show_db
**
** Description:
**	Scans the registered servers queue and connects to any
**	registered Ingres DBMS Servers on the target host.  If 
**	target host name is zero length string, connections are 
**	made to Ingres DBMS Servers on the local host.
**
**	Uses ESQL to obtain and display Ingres database names 
**	and owners.
**
** Input:
**	host		Target host name, zero length string for local.
**	server		Server info queue.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
ibr_show_db( char *host, QUEUE *server )
{
    GCM_DATA	*data;
    STATUS	status = OK;

    SIprintf( "\nDatabase\tOwner\n" );
    SIprintf( "--------\t-----\n" );

    /*
    ** Scan the registered server queue looking for Ingres DBMS Servers.
    */
    for(
	 data = (GCM_DATA *)server->q_next;
	 (QUEUE *)data != server;
	 data = (GCM_DATA *)data->q.q_next
       )
	if ( ! STcompare( GCA_INGRES_CLASS, data->data[0] ) )
	    esql_request( host, data->data[1] );

    return( status );
}


/*
** Name: gcm_load
**
** Description:
**	Builds a GCM request to retrieve the GCM information
**	represented by a GCM_INFO array and issues a FASTSELECT
**	request for the target server.  The GCM response data
**	is added to the GCM data queue.  The GCM data queue
**	may be cleared by calling gcm_unload().
**
**	Returns FAIL if there is additional data to be retrieved
**	and places the instance to be used for the next request
**	in beg_inst (which should be initialized to a zero length
**	string for the first call).
**
** Input:
**	target		Target server.
**	gcm_count	Number of entries in gcm_info array.
**	gcm_info	GCM info array.
**	beg_inst	Beginning instance, init to zero length string.
**	msg_len		Length of msg_buff.
**	msg_buff	Buffer for GCM messages.
**	gcm_data	GCM data queue.
**
** Output:
**	beg_inst	Instance for next call if return FAIL.
**	gcm_data	Entries added.
**
** Returns:
**	STATUS		OK, FAIL or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**	 6-Apr-04 (gordy)
**	    Handle end-of-data error case.
*/

static STATUS
gcm_load( char *target, i4 gcm_count, GCM_INFO *gcm_info, char *beg_inst, 
	  i4 msg_len, char *msg_buff, QUEUE *gcm_data )
{
    STATUS	status;
    char	*ptr = msg_buff;
    i4		i, msg_type, len;
    i4		data_count = 0;

    /*
    ** Build GCM request.
    */
    msg_type = GCM_GETNEXT;
    ptr += gcu_put_int( ptr, 0 );		/* error_status */
    ptr += gcu_put_int( ptr, 0 );		/* error_index */
    ptr += gcu_put_int( ptr, 0 );		/* future[2] */
    ptr += gcu_put_int( ptr, 0 );
    ptr += gcu_put_int( ptr, -1 );		/* client_perms: all */
    ptr += gcu_put_int( ptr, 0 );		/* row_count: no limit */
    ptr += gcu_put_int( ptr, gcm_count );

    for( i = 0; i < gcm_count; i++ )
    {
	ptr += gcu_put_str( ptr, gcm_info[i].classid );
	ptr += gcu_put_str( ptr, beg_inst );
	ptr += gcu_put_str( ptr, "" );
	ptr += gcu_put_int( ptr, 0 );
    }

    len = ptr - msg_buff;

    /*
    ** Connect to target, send GCM request and
    ** receive GCM response.
    */
    status = gca_fastselect( target, &msg_type, &len, msg_buff, msg_len );

    if ( status == OK )
    {
	char	*str; 
	char	classid[ GCN_OBJ_MAX_LEN ]; 
	char	instance[ GCN_OBJ_MAX_LEN ];
	char	cur_inst[ GCN_OBJ_MAX_LEN ];
	i4	len, err_index, junk, elements;

	/*
	** Extract GCM info.
	*/
	ptr = msg_buff;
	ptr += gcu_get_int( ptr, &status );
	ptr += gcu_get_int( ptr, &err_index );
	ptr += gcu_get_int( ptr, &junk );		/* skip future[2] */
	ptr += gcu_get_int( ptr, &junk );
	ptr += gcu_get_int( ptr, &junk );		/* skip client_perms */
	ptr += gcu_get_int( ptr, &junk );		/* skip row_count */
	ptr += gcu_get_int( ptr, &elements );

	if ( status )  
	{
	    /*
	    ** Limit results to non-error elements.
	    */
	    elements = max( 0, err_index );

	    /*
	    ** Ignore possible end-of-elements status.
	    */
	    if ( status == MO_NO_NEXT )
		status = OK;
	    else
		SIprintf( "\nGCM error: 0x%x\n", status );
	}

	for(;;)
	{
	    for( i = 0; i < gcm_count; i++ )
	    {
		if ( ! elements-- )
		{
		    /*
		    ** Completed processing of current GCM response.
		    ** Return FAIL to indicate need for additional
		    ** request beginning with last valid instance
		    ** (make sure we actually loaded something from
		    ** the current message to avoid an infinite loop).
		    */
		    if ( data_count )  status = FAIL;
		    break;
		}
		else
		{
		    /*
		    ** Extract classid, instance, and value.
		    */
		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, classid );
		    classid[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, instance );
		    instance[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, gcm_info[i].value );
		    gcm_info[i].value[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &junk );	/* skip perms */
		}

		/*
		** A mis-match in classid indicates the end of
		** the entries we requested.
		*/
		if ( STcompare( classid, gcm_info[i].classid ) )
		    break;

		/*
		** Instance value should be same for each 
		** classid in group.  Save for first classid,
		** check all others.
		*/
		if ( ! i )
		    STcopy( instance, cur_inst );	/* Save new instance */
		else  if ( STcompare( instance, cur_inst ) )
		{
		    SIprintf( "\nFound invalid GCM instance: '%s' != '%s'\n",
			      instance, cur_inst );
		    break;
		}
	    }

	    if ( i < gcm_count )
		break;		/* Completed processing of GCM response info */
	    else
	    {
		GCM_DATA	*data;

		/*
		** Save instance in case we need to issue
		** another GCM FASTSELECT request for more
		** info.
		*/
		STcopy( cur_inst, beg_inst );

		/*
		** Add entry to GCM data queue.
		*/
		data = (GCM_DATA *)MEreqmem( 0, sizeof( GCM_DATA ) + 
						sizeof( char * ) * 
						(gcm_count - 1), 
					     FALSE, NULL );
		if ( ! data )
		{
		    SIprintf( "\nMemory allocation failed\n" );
		    break;
		}

		data_count++;
		QUinsert( &data->q, gcm_data->q_prev );

		for( i = 0; i < gcm_count; i++ )
		{
		    data->data[i] = STalloc( gcm_info[i].value );

		    if ( ! data->data[i] )
		    {
			i4 j;

			SIprintf( "\nMemory allocation failed\n" );
			QUremove( &data->q );
			for( j = 0; j < i; j++ )  MEfree( (PTR)data->data[j] );
			MEfree( (PTR)data );
			break;
		    }
		}
	    }
	}
    }

    return( status );
}


/*
** Name: gcm_unload
**
** Description:
**	Removes entries from a GCM data queue as built by
**	gcm_load(), and frees associated resources.
**
** Input:
**	count		Number of GCM classids for each instance.
**	gcm_data	GCM data queue.
**
** Output:
**	gcm_data	Entries removed.
**
** Returns:
**	VOID.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
gcm_unload( i4 count, QUEUE *gcm_data )
{
    GCM_DATA	*data, *next;
    i4		i;

    for(
	 data = (GCM_DATA *)gcm_data->q_next;
	 (QUEUE *)data != gcm_data;
	 data = next
       )
    {
	next = (GCM_DATA *)data->q.q_next;

	for( i = 0; i < count; i++ )  MEfree( (PTR)data->data[i] );
	QUremove( &data->q );
	MEfree( (PTR)data );
    }

    return;
}


/*
** Name: gcm_display
**
** Description:
**	Displays info in a GCM data queue.
**
** Input:
**	count		Number of GCM classids for each instance.
**	data		GCM data queue.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
gcm_display( i4 count, QUEUE *dataq )
{
    GCM_DATA	*data;
    i4		i;

    for(
	 data = (GCM_DATA *)dataq->q_next;
	 (QUEUE *)data != dataq;
	 data = (GCM_DATA *)data->q.q_next
       )
    {
	for( i = 0; i < count; i++ )
	    if ( i )
		SIprintf( "\t%s", data->data[i] );
	    else
		SIprintf( "%s", data->data[i] );

	SIprintf( "\n" );
    }

    return;
}


/*
** Name: gca_request
**
** Description:
**	Issues GCA_REQUEST for target server and returns
**	association ID.  Uses username and password in
**	global variables if available.
**
** Input:
**	target		Target server.
**
** Output:
**	assoc_id	Association ID.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_request( char *target, i4 *assoc_id )
{
    GCA_RQ_PARMS	rq_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rq_parms ), 0, (PTR)&rq_parms );
    rq_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    rq_parms.gca_partner_name = target;

    if ( *username  &&  *password )
    {
	rq_parms.gca_modifiers |= GCA_RQ_REMPW;
	rq_parms.gca_rem_username = username;
	rq_parms.gca_rem_password = password;
    }

    call_status = IIGCa_call( GCA_REQUEST, (GCA_PARMLIST *)&rq_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    *assoc_id = rq_parms.gca_assoc_id;

    if ( call_status == OK  &&  rq_parms.gca_status != OK )
	status = rq_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_REQUEST failed: 0x%x\n", status );
    
    return( call_status );
}


/*
** Name: gca_fastselect
**
** Description:
**	Issues GCA_FASTSELECT for target server, sending
**	GCM message provided and returning GCM response.
**	Uses username and password in global variables if 
**	available.
**
** Input:
**	target		Target server.
**	msg_type	GCM message type.
**	msg_len		Length of GCM message.
**	msg_buff	GCM message.
**	buff_max	Length of buffer for response.
**
** Output:
**	msg_type	GCM response message type.
**	msg_len		Length of GCM response.
**	msg_buff	GCM response.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_fastselect( char *target, i4 *msg_type, 
		i4 *msg_len, char *msg_buff, i4 buff_max )
{
    GCA_FS_PARMS	fs_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( fs_parms ), 0, (PTR)&fs_parms );
    fs_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    fs_parms.gca_modifiers = GCA_RQ_GCM;
    fs_parms.gca_partner_name = target;
    fs_parms.gca_buffer = msg_buff;
    fs_parms.gca_b_length = buff_max;
    fs_parms.gca_message_type = *msg_type;
    fs_parms.gca_msg_length = *msg_len;

    if ( *username  &&  *password )
    {
	fs_parms.gca_modifiers |= GCA_RQ_REMPW;
	fs_parms.gca_rem_username = username;
	fs_parms.gca_rem_password = password;
    }

    call_status = IIGCa_call( GCA_FASTSELECT, (GCA_PARMLIST *)&fs_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  fs_parms.gca_status != OK )
	status = fs_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
    {
	SIprintf( "\nGCA_FASTSELECT failed: 0x%x\n", status );
	call_status = status;
    }
    else
    {
	*msg_type = fs_parms.gca_message_type;
	*msg_len = fs_parms.gca_msg_length;
    }

    return( call_status );
}


/*
** Name: gca_receive
**
** Description:
**	Issues a GCA_RECEIVE request and returns GCA message.
**
** Input;
**	assoc_id	Association ID.
**	msg_len		Length of message buffer.
**	msg_buff	Message buffer.
**
** Output:
**	msg_type	GCA message type.
**	msg_len		Length of GCA message.
**	msg_buff	GCA Message.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_receive( i4 assoc_id, i4 *msg_type, i4 *msg_len, char *msg_buff )
{
    GCA_RV_PARMS	rv_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rv_parms ), 0, (PTR)&rv_parms );
    rv_parms.gca_assoc_id = assoc_id;
    rv_parms.gca_b_length = *msg_len;
    rv_parms.gca_buffer = msg_buff;

    call_status = IIGCa_call( GCA_RECEIVE, (GCA_PARMLIST *)&rv_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  rv_parms.gca_status != OK )
	status = rv_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_RECEIVE failed: 0x%x\n", status );
    else  if ( ! rv_parms.gca_end_of_data )
    {
	SIprintf( "\nGCA_RECEIVE buffer overflow\n" );
	call_status = FAIL;
    }
    else
    {
	*msg_type = rv_parms.gca_message_type;
	*msg_len = rv_parms.gca_d_length;
    }

    return( call_status );
}


/*
** Name: gca_send
**
** Description:
**	Issues GCA_SEND request to send GCA message.
**
** Input:
**	assoc_id	Association ID.
**	msg_type	GCA message type.
**	msg_len		Length of GCA message.
**	msg_buff	GCA message.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_send( i4 assoc_id, i4 msg_type, i4 msg_len, char *msg_buff )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_association_id = assoc_id;
    sd_parms.gca_message_type = msg_type;
    sd_parms.gca_msg_length = msg_len;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
	status = sd_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_SEND failed: 0x%x\n", status );

    return( call_status );
}


/*
** Name: gca_release
**
** Description:
**	Issues GCA_SEND request to send GCA_RELEASE message.
**
** Input:
**	assoc_id	Association ID.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_release( i4 assoc_id )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;
    char		*ptr = msg_buff;

    ptr += gcu_put_int( ptr, 0 );

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_assoc_id = assoc_id;
    sd_parms.gca_message_type = GCA_RELEASE;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_msg_length = ptr - msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
	status = sd_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_SEND(GCA_RELEASE) failed: 0x%x\n", status );

    return( call_status );
}


/*
** Name: gca_disassoc
**
** Description:
**	Issues GCA_DISASSOC request.
**
** Input:
**	assoc_id	Association ID.
**
** Ouput:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static STATUS
gca_disassoc( i4 assoc_id )
{
    GCA_DA_PARMS	da_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( da_parms ), 0, (PTR)&da_parms );
    da_parms.gca_association_id = assoc_id;

    call_status = IIGCa_call( GCA_DISASSOC, (GCA_PARMLIST *)&da_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  da_parms.gca_status != OK )
	status = da_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	SIprintf( "\nGCA_DISASSOC failed: 0x%x\n", status );

    return( call_status );
}


/*
** Name: getrec_noecho
**
** Description:
**	Routine extracted from NETU to input password
**	without displaying (echoing) the input.
**
** Input:
**	len		Length of buffer.
**	buf		Input buffer.
**
** Output:
**	buf		User input.
**
** Returns:
**	VOID.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
getrec_noecho( i4 len, char *buf )
{
    static TEENV_INFO	te;
    static i4		teopened = 0;
    static bool		tefile = FALSE;
    char		*obuf = buf;
    i4			c;

    if ( ! teopened )
    {
	if ( TEopen( &te ) != OK  ||  ! SIterminal( stdin ) )  tefile = TRUE;
	teopened++;
    }

    if ( tefile )  
    {
	SIgetrec( buf, (i4)len, stdin );
	return;
    }

    /* make sure that the buffer is allways NULL terminated */
    if ( --len >= 0 )  *(buf + len) = 0;

    TErestore( TE_FORMS );

    while( len-- > 0 )
	switch( c = TEget( 0 ) ) 	/* No timeout */
	{
	    case TE_TIMEDOUT:	/* just incase of funny TE CL */
	    case EOF:
	    case '\n':
	    case '\r':
		*buf++ = 0;
		len = 0;
		break;

	    default:
		*buf++ = c;
		break;
	}

    TErestore( TE_NORMAL );

    return;
}


/*
** Name: esql_request
**
** Description:
**	Uses ESQL to connect to server listening at addr on target
**	host and retrieve/display database info.  If target host 
**	name is zero length string, connections are made to server 
**	on the local host.  Server must support iidbdb database 
**	with table iidatabase having columns name and own.
**	
** Input:
**	host		Target host name, zero length string for local.
**	addr		Listen address of target server.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
*/

static VOID
esql_request( char *host, char *addr )
{
EXEC SQL BEGIN DECLARE SECTION;

    char	target[ 128 ];
    char	name[ 33 ];
    char	own[ 33 ];

EXEC SQL END DECLARE SECTION;

    /*
    ** Build target specification for Ingres Server:
    ** @::iidbdb/@addr for local, 
    ** @host::iidbdb/@addr for remote.
    **
    ** NOTE: we could have provided the username and
    ** password in the target string, rather than 
    ** as connection options, using the format
    ** @host[user,pwd]::db/@addr.
    **
    ** NOTE: this uses the installation registry master
    ** Name Server and Comm Server for the connection.
    ** It would be a lot better if the Comm Server in
    ** the same installation as the target server were 
    ** used.  This requires the protocol and network 
    ** listen port for the connection formatted as 
    ** follows: @host,proto,port::db/@addr.  We did
    ** retrieve the network protocol information else-
    ** where in this program, but the protocol driver
    ** ID for the remote system may require translation
    ** to a local system ID. This enhancement is left 
    ** as an exercise for the reader.
    */
    STprintf( target, "@%s::iidbdb/@%s", host, addr );

    EXEC SQL WHENEVER SQLERROR CALL sqlprint;

    if ( *username  &&  *password )
    {
	EXEC SQL CONNECT :target
	    OPTIONS = '-remote_system_user', :username,
		      '-remote_system_password', :password;
    }
    else
    {
	EXEC SQL CONNECT :target;
    }

    EXEC SQL SELECT name, own INTO :name, :own FROM iidatabase;
    EXEC SQL BEGIN;
	STtrmwhite( name );
	STtrmwhite( own );
	SIprintf( "%-8s\t%s\n", name, own );
    EXEC SQL END;

    EXEC SQL COMMIT;
    EXEC SQL DISCONNECT;

    EXEC SQL WHENEVER SQLERROR CONTINUE;

    return;
}


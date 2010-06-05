/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <ci.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <ex.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <qu.h>
#include <si.h>
#include <sl.h>
#include <st.h>
#include <tm.h>
#include <tr.h>

#include <erglf.h>
#include <sp.h>
#include <mo.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcs.h>
#include <gcu.h>

/*
**
** Name: IIGCN.C - Name Server main
**
** Other relevant source files:
**	gcnscln.c	- clean lists of active servers 
**	gcnseror.c	- format error msg to send to IIGCN client
**	gcnslsn.c	- main state machine for IIGCN
**	gcnsfile.c	- load/store IIGCN database files
**	gcnsinit.c	- initialize name queues
**	gcnsoper.c	- perform operation on IIGCN name queue
**	gcnsrslv.c	- resolve address for client
**	gcnutil.c	- low level message rolling/unrolling
**	gcnsadm.c	- gcadm interface
**
**  Description:
**	IIGCN, the name server, is an asynchronous server, built upon
**	GCA, to manage a dinky database of connection information.  The
**	database consist of various tables (here called "name queues")
**	each with three columns: uid, obj, and val.  Uid is the id of
**	the user who added the row, so users may store private data.
**	Obj and val are attribute/value pairs whose meaning varies with
**	the table.  The tables are (currently):
**	
**		Table	Obj	Val
**		----------------------------
**		iinmsvr	*	address of IIGCN server
**		ingres	dbname	address of INGRES server for "dbname"
**		star	dbname	address of STAR server for "dbname"
**		commsvr	host	address of IIGCC server serving "host"
**		node	host	connect info for host
**		login	host	login info for host
**	
**	IIGCN performs three basic operations: 1) acts as a DBMS for
**	the tables, providing the GCN_ADD/GCN_DEL/GCN_RETRIEVE
**	functions, 2) handles GCN_RESOLVE requests, resolving
**	connection requests into connection information, and 3)
**	periodically purges defunct servers by verifying addresses with
**	GCdetect().
**	
**	INITIALIZATION
**	
**	IIGCN calls gcn_srv_load() to read the file
**	$II_CONFIG/name/iiname.all for the list of name queues IIGCN is
**	to handle.  The first word on each line gives the name of the
**	name queue, while the other words are modifiers: "local"
**	denotes a cached name queue (the default), "global" denotes a
**	shared name queue (only on a VAX-cluster, don't you know), and
**	"transient" indicates a name queue whose val fields are the
**	addresses of servers.  Transient name queues get periodically
**	cleaned out.
**	
**	TABLE MANAGEMENT
**	
**	A GCN_QUE_NAME structure describes each IIGCN name queue.  Each
**	is an in-memory QUEUE of GCN_TUP_QUE structures, which contain
**	the variable length uid, obj, and val strings.  Database files
**	in the $II_CONFIG/name subdirectory shadow each in-memory name
**	queue.  Before IIGCN accesses any name queue, it calls
**	gcn_load_nq() to refresh the in-memory queue from the database
**	file; after updating the name queue, IIGCN calls gcn_dump_nq()
**	to save the queue.  At initialization, the GCN_QUE_NAME
**	structure may indicate that the name queue is cached: in this
**	case, only the first call to gcn_load_nq() refreshes the
**	queue.
**	
**	The GCN_DB_RECORD structure describes each name queue entry as
**	it sits in the database file.  The format is currently
**	fixed-field, but could (should) be converted to white space
**	separated fields for editing with text editors.
**	
**	An array of GCN_QUE_NAME structures, Name_list[], enumerates
**	all name queues known to IIGCN; gcn_srv_load() fills this
**	array.
**	
**	GCA I/O TO IIGCN
**	
**	IIGCN is an asynchronous server client of GCA, starting
**	connections with a GCA_LISTEN.  The main state machine,
**	gcn_server(), is primed by a call to gcn_listen() to post the
**	first listen, and thereafter runs entirely by callback from GCA
**	I/O completion.  Gcns_server() is essentially a loop: accepts
**	and reposts the listen, reads from the client, dispatches the
**	request to the appropriate routine, sends the response, sends a
**	GCA_RELEASE message, and then invokes GCA_DISASSOC.
**	
**	IIGCN allows I/O to its clients to block via asynchronous calls
**	to GCA; however, all other operations, particularly access to
**	the name queues, are atomic with respect to client I/O.  To
**	handle long responses, particularly voluminous GCN_RETRIEVE's,
**	IIGCN chains together lists of GCN_MBUF's, or memory buffers,
**	to store the responses before sending.
**	
**	GCN_ADD/GCN_DEL/GCN_RETRIEVE/GCN_RESOLVE
**	
**	CLEANUP
**	
**	Before every GCA_LISTEN is accepted gcn_server() calls
**	gcn_cleanup() to clean any transient name queues.
**	Gcns_cleanup() proceeds with the cleanup at most once every 5
**	minutes.  It calls GCdetect() with the server address listed in
**	the val field of the GCN_TUP_QUE, and removes the entry if
**	GCdetect() returns FALSE.  Gcns_cleanup() is careful to call
**	each server only once.
**
**	TRACING
**
**	Tracing output goes to II_GCN_LOG; II_GCN_TRACE controls the
**	amount of tracing:
**		0	- errors 
**			  GCN_RESOLVE's returning 0 rows
**			  GCdetect()'s revealing dead servers
**		1	- all user commands
**		2	- gcn_server state machine states
**			  GCdetect()'s revealing live servers
**		3	- file operations
**
**  History:    $Log-for RCS$
**	    
**      08-Sep-87   (lin)
**          Initial creation.
**	20-Mar-88   (lin)
**	    Formally coded the routines for each function.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.   Tracing added.
**	25-Nov-89 (seiwald)
**	    Major reworking for asynchronous support.  The Name server
**	    handles multiple, concurrent clients now.  Name server
**	    operations are atomic, so no semaphore protection of name
**	    server data structures is needed.  Requests and responses are
**	    built upon queues of memory buffers.
**	    
**	    The gcnslsn.c file has been gutted: it now contains
**	    gcn_server(), the state machine for each client thread in the
**	    name server.  The main routine of iigcn starts one client
**	    thread, which posts a listen.  As the listen completes, the
**	    thread reposts a new listen and then continues processing.
**	    
**	    Initialization of name queues severly reworked.
**	    
**	    Most shared global variables are now stuck into IIGCn_static,
**	    including all those culled from the enviroment, which are
**	    initialized at startup (rather than at various times throughout
**	    the code).
**	    
**	    The gcn_resolve() operation can now return GCA_ER_DATA on
**	    resolve error.  Formerly, because GCA_REQUEST only expected a
**	    GCN_RESOLVED response, gcn_resolve could only indicate error by
**	    returning 0 rows.  Now, any of a dozen new messages may be
**	    returned.
**	    
**	    Gcn.h has been split into gcn.h and gcnint.h, for name server
**	    internal data structures.
**	    
**	    II_GCN_LOG has gone away.  Use II_GCAxx_LOG.
**	    
**	    New CL function GCexec(), called repeatedly to drive async
**	    GCA events.  This function must return (every now and then)
**	    so we can check the shutdown flag.
**
**	    Many other, minor changes were made.
**
**	11-Dec-89 (seiwald)
**	    Reverted to having CL, rather than mainline, loop to drive 
**	    async I/O events.  VMS doesn't loop.  A call to GCshut stops
**	    GCexec.
**	06-Feb-90 (seiwald)
**	    Call gcn_cleanup with the name of the queue to clean.
**      21-Feb-90 (cmorris)
**	    Added call to EXdeclare to set up GCX tracing exception handler.
**	21-Mar-90 (seiwald)
**	    Default LCL_VNODE is now the local host name, as returned by
**	    GChostname().
**	24-Aug-90 (seiwald)
**	    Set maxsessions from symbol II_GCNxx_MAX.
**	06-Dec-90 (seiwald)
**	    Use (now adjusted) GCA_SERVER_ID_LNG for length of local server
**	    listen address.
**      09-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	23-apr-91 (seiwald)
**	    Make stevet's change conditionally compile with GCF63.
**	24-apr-91 (brucek)
**	    Added include for tm.h
**	28-Jun-91 (seiwald)
**	    Internal name queue access restructured.  See gcnsfile.c
**	    for new description.
**	13-aug-91 (stevet)
**	    Change read-in character support to use II_CHARSET symbol.
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**	01-Oct-92 (brucek)
**	    Added call to gcn_init_mib.
**	3-Dec-92 (gautam)
**	    PM calls added in for PM support. 
**	31-Dec-92 (brucek)
**	    Don't abort if PMload fails. 
**	11-Jan-93 (edg)
**	    Call gcu_get_tracesym() to get trace level.
**	19-Jan-93 (edg)
**	    Changed PM symbol names to conform to LRC proposal.  TRUE/FALSE
**	    values changes to ON/OFF standard.  Removed svr_reset -- does
**	    nothing.
**	21-Jan-93 (edg)
**	    Fixed bug - symbol search string used by PMget should not include
**	    "gcn" in the string as that is a PMsetdefault.
**	21-May-93 (edg)
**	    Start bedchecking right at server start.  This is better than wait-
**	    ing for the first op or resolve message to come in since there's
**	    most likely cruft left around in the transient q's from the last
**	    time the installation was taken down.
**      11-aug-93 (ed)
**          added missing includes
**      12-Nov-93 (brucek)
**          Changed names of a few PM variables.
**	16-dec-93 (tyler)
**	    Replaced GChostname() with call to PMhost().
**	01-apr-94 (eml)
**	    BUG #60458.
**	    Moved the call to gca_terminate out of the shutdown logic in
**	    module GCNSLSN and into main(). The gca terminate logic is no
**	    longer executed in callback mode, thus fixing bug #60458 where,
**	    under some conditions, the gca terminate logic called GCsync
**	    and waited for an I/O to complete that was blocked by being in
**	    Callback.
**	30-Nov-95 (gordy)
**	    Moved some initialization code to gcnsinit.c to abstract the
**	    Name Database interface for embedded Name Server support.
**	 3-Sep-96 (gordy)
**	    Renamed internal server functions to further separate from
**	    the client interface.  Use client interface to check for an
**	    existing Name Server prior to starting up GCA.
**	27-sep-1996 (canor01)
**	    Move global data definitions to gcndata.c.  Move gcn_getflag()
**	    to gcnutil.c so it can reside in an exportable library for NT.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	21-May-97 (gordy)
**	    Initialize GCS and register Name Server callback functions.
**	10-Jul-97 (gordy)
**	    GCS init now takes parameters, provide logging function.
**	20-Aug-97 (gordy)
**	    Added message logging function for GCS.
**	 4-Dec-97 (rajus01)
**	    Added gcn_ip_auth(), the GCS callback function to validate
**	    tickets.
**	17-Feb-98 (gordy)
**	    Expanded global info.
**	21-May-98 (gordy)
**	    Added support for remote authentication mechanism.
**	27-May-98 (gordy)
**	    Converted remote auth mech from pointer to char array.
**	 4-Jun-98 (gordy)
**	    Removed reserved bedcheck thread.
**      25-Sep-98 (fanra01)
**          Moved mib initialization after gcn_srv_init to preserve
**          GCN_MIB_INIT flag.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**  10-Feb-2004 (fanra01)
**      SIR 111718
**      Add call to initialize version MIB objects.
**	24-Mar-2004 (wansh01)
**	    added call to gcn_adm_init and gcn_adm_term to support
**	    gcadm interface.
**	    added CUFLIB in ming hint NEEDLIBS 
**	15-Jul-04 (gordy)
**	    Enhanced encryption of LOGIN passwords and passwords
**	    passed between gcn and gcc.
**	16-Jul-04 (gordy)
**	    Extended ticket versions.
**	04-Aug-2004 (wansh01)
**	    replacing gcn_server() with gcn_start_session().
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**       3-Jul-06 (rajus01)
**	    Added support for "register server", "show server" commands for GCN
**	    admin interface.
**	 4-Sep-09 (gordy)
**	    Remove string length restrictions.
**/

/*
NEEDLIBS =      GCFLIB COMPATLIB CUFLIB

OWNER =         INGUSER

PROGRAM =       (PROG1PRFX)gcn
*/


/*
** Fixed prefixes used to initialize masks.
*/
#define	GCN_LOGIN_PREFIX	"IILOGIN-"
#define	GCN_COMSVR_PREFIX	"IINMSVR-"

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN	i4	GCX_exit_handler(EX_ARGS *);

static  VOID	gcn_err_log( STATUS, i4, PTR );
static	VOID	gcn_msg_log( char * );
static	STATUS	gcn_get_key( GCS_GET_KEY_PARM * );
static	STATUS	gcn_usr_pwd( GCS_USR_PWD_PARM * );
static	VOID	gcn_init_mib( VOID );
static	STATUS	gcn_ip_auth( GCS_IP_PARM * );


/*{
** Name: main() - entry point for IIGCN
**
** Description:
**	- initializes from symbols
**	- registers for listening
**	- reads in list of name queues
**	- posts first listen
**	- drives everything from callback (via GCNexec())
**	
** History:
**	21-Mar-89 (seiwald)
**	    Written.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	19-jan-94 (edg)
**	   Added standard argc and argv paramters to main() so that this
**	   will compile under NT.`
**	30-Nov-95 (gordy)
**	    Moved some initialization code to gcnsinit.c to abstract the
**	    Name Database interface for embedded Name Server support.
**	 3-Sep-96 (gordy)
**	    Renamed internal server functions to further separate from
**	    the client interface.  Use client interface to check for an
**	    existing Name Server prior to starting up GCA.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	21-May-97 (gordy)
**	    Initialize GCS and register Name Server callback functions.
**	10-Jul-97 (gordy)
**	    GCS init now takes parameters, provide logging function.
**	20-Aug-97 (gordy)
**	    Added message logging function for GCS.
**	17-Feb-98 (gordy)
**	    Expanded global info.
**	21-May-98 (gordy)
**	    Added support for remote authentication mechanism.
**	27-May-98 (gordy)
**	    Converted remote auth mech from pointer to char array.
**	 4-Jun-98 (gordy)
**	    Removed reserved bedcheck thread.
**	14-Sep-98 (gordy)
**	    Start with self as registry master.
**	23-Sep-98 (gordy)
**	    Move MIB initialization after general server initialization
**	    so that MIB flag bit is preserved.  Initialize the registry.
**	17-Jun-2004 (schka24)
**	    Length-limit env variable values.
**	 9-Jul-04 (gordy)
**	    GCS version bumped to 2 for distinct IP Auth operation.
**	    Use new gcs_default_mech() to get default REM Auth mechanism.
**	15-Jul-04 (gordy)
**	    Initialize masks used for password encryption.
**      26-Apr-2007 (hanal04) SIR 118196
**          Make sure PCpid will pick up the current process pid on Linux.
**      15-Apr-2009 (hanal04) Bug 121931
**          gcn_register() performs a gca initialisation against the gca_cb
**          which is a global static in gcnslsn.c. IIGCa_call() with
**          GCA_TERMINATE will use the global gca_default_cb which was not
**          not used during the GCA_INITIATE and will silently return an error
**          without cleaning up.
**          Call the new gcn_deregister() to GCA_TERMINATE using the
**          gca_cb used during registration.
**	 4-Sep-09 (gordy)
**	    Use dynamic storage for remote mechanism and registry info.
**      08-Apr-10 (ashco01) Bug: 123551 
**          Added E_GC0173_GCN_HOSTNAME_MISMATCH informational message to
**          report difference between local hostname and config.dat
**          '.local_vnode' value. 
*/	

i4
main( argc, argv )
i4  	argc;
char 	**argv;
{
    EX_CONTEXT		context;
    STATUS		status;
    CL_ERR_DESC		cl_err;
    char		*env = 0;
    char		name[MAX_LOC+CM_MAXATTRNAME];
    GCA_PARMLIST	parms;
    char                lvn_value[GCN_TYP_MAX_LEN], *buf;
    ER_ARGUMENT         earg[2];

#ifdef LNX
    PCsetpgrp();
#endif

    /* 
    ** Issue initialization call to ME 
    */
    MEadvise( ME_INGRES_ALLOC );

    /* 
    ** Declare the GCA real-time tracing exception handler. 
    */
    EXdeclare( GCX_exit_handler, &context );

    /*
    ** Call erinit() now for logging of startup messages.
    */
    GChostname( name, sizeof( name ) );
    IIGCn_static.hostname = STalloc( name );
    gcu_erinit( IIGCn_static.hostname, "IIGCN", "" );

    /* 
    **	Get character set logical.
    */
    NMgtAt( ERx("II_INSTALLATION"), &env );

    if ( env && *env )
    {
	STlcopy(env, name, 10);		/* Arbitrary length limit */
	IIGCn_static.install_id = STalloc( name );
	STprintf( name, ERx("II_CHARSET%s"), IIGCn_static.install_id );
    }
    else
    {
	STprintf( name, ERx("II_CHARSET") );
	IIGCn_static.install_id = STalloc( ERx("*") );
    }

    NMgtAt( name, &env );

    if ( env && *env )
    {
	/* If non-default set is defined */
	STlcopy( env, name, CM_MAXATTRNAME);
	status = CMset_attr( name, &cl_err);

	if ( status != OK )
	{
	    gcu_erlog( 0, 1, E_GC0105_GCN_CHAR_INIT, NULL, 0, NULL );
	    PCexit(FAIL);
	}
    }

    /* 
    ** Load PM resource values from the standard location.
    */
    PMinit();

    switch( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) )
    {
	case OK:
	    /* loaded successfully */
	    break;

	case PM_FILE_BAD:
	    gcu_erlog( 0, 1, E_GC003D_BAD_PMFILE, NULL, 0, NULL );
	    break;

	default:
	    gcu_erlog( 0, 1, E_GC003E_PMLOAD_ERR, NULL, 0, NULL );
	    break;
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx( "gcn" ) );

    /*
    ** Initialize Name Server tracing.
    */
    gcu_get_tracesym( "II_GCN_TRACE", "!.gcn_trace_level", &env );
    if ( env && *env )  CVal( env, &IIGCn_static.trace );

    gcu_get_tracesym( "II_GCN_LOG", "!.gcn_trace_log", &env );
    if ( env && *env )
	TRset_file( TR_F_OPEN, env, (i4)STlength( env ), &cl_err );

    /*
    ** General Initialization.
    */
    if ( (status = gcn_srv_init()) != OK )  PCexit( FAIL );
    if ( (status = gcn_ir_init()) != OK )  PCexit( FAIL );
    gcn_init_mib();

    STpolycat( 2, GCN_LOGIN_PREFIX, IIGCn_static.hostname, name );
    gcn_init_mask( name, sizeof( IIGCn_static.login_mask ), 
			 IIGCn_static.login_mask );

    /*
    ** Initialize GCS prior to GCA to ensure our logging
    ** routines are available during GCS initialization.
    */
    {
	GCS_INIT_PARM parm;

	MEfill( sizeof( parm ), (u_char)0, (PTR)&parm );
	parm.version = GCS_API_VERSION_2;
	parm.err_log_func = (PTR)gcn_err_log;
	parm.msg_log_func = (PTR)gcn_msg_log;

	status = IIgcs_call( GCS_OP_INIT, GCS_NO_MECH, (PTR)&parm );

	if ( status != OK )
	{
	    gcu_erlog( 0, 1, status, NULL, 0, NULL );
	    PCexit( FAIL );
	}
    }

    /*
    ** Make sure there are no other Name Servers
    ** running.  We use the client name server
    ** interface, but call gcn_bedcheck() directly
    ** rather than IIgcn_check() since we don't
    ** want embedded name server functionality.
    */
    if ( (status = IIGCn_call( GCN_INITIATE, NULL )) == OK )
    {
	if ( gcn_bedcheck() == OK )  status = E_GC0122_GCN_2MANY;
	IIGCn_call( GCN_TERMINATE, NULL );
    }

    if ( status != OK )
    {
	gcu_erlog( 0, 1, status, NULL, 0, NULL );
	PCexit( FAIL );
    }

    /*
    ** Initialize GCA interface.
    */
    if ( (status = gcn_register( name )) != OK )  PCexit( FAIL );

    if ( ! (IIGCn_static.listen_addr = STalloc( name )) )
        status = E_GC0121_GCN_NOMEM;
    else
        status = gcn_ir_save( IIGCn_static.install_id, 
			      IIGCn_static.listen_addr );

    if ( status != OK )
    {
	gcu_erlog( 0, 1, status, NULL, 0, NULL );
	PCexit( FAIL );
    }

    STpolycat( 2, GCN_COMSVR_PREFIX, IIGCn_static.listen_addr, name );
    gcn_init_mask( name, sizeof( IIGCn_static.comsvr_mask ),
			 IIGCn_static.comsvr_mask );

    /*
    ** Now configure GCS.  We do this after GCA
    ** initialization so as to override any GCA
    ** settings.  If this fails, we can still
    ** operate with reduced functionality.
    */
    {
	GCS_SET_PARM	parm;
	PTR		func;

	parm.parm_id = GCS_PARM_GET_KEY_FUNC;
	parm.length = sizeof( func );
	parm.buffer = (PTR)&func;
	func = (PTR)gcn_get_key;

	IIgcs_call( GCS_OP_SET, GCS_NO_MECH, (PTR)&parm );

	parm.parm_id = GCS_PARM_USR_PWD_FUNC;
	parm.length = sizeof( func );
	parm.buffer = (PTR)&func;
	func = (PTR)gcn_usr_pwd;

	IIgcs_call( GCS_OP_SET, GCS_NO_MECH, (PTR)&parm );

	parm.parm_id = GCS_PARM_IP_FUNC;
	parm.length = sizeof( func );
	parm.buffer = (PTR)&func;
	func = (PTR)gcn_ip_auth;

	IIgcs_call( GCS_OP_SET, GCS_NO_MECH, (PTR)&parm );
    }

    if ( ! IIGCn_static.rmt_mech  ||  ! IIGCn_static.rmt_mech[0] )
    {
	GCS_INFO_PARM	info;
	GCS_MECH	mech_id = gcs_default_mech( GCS_OP_REM_AUTH );

	/*
	** Use default mechanism for remote authentication if supported.
	*/
	if ( 
	     (status = IIgcs_call(GCS_OP_INFO, mech_id, (PTR)&info)) == OK  &&
	     info.mech_info[0]->mech_status == GCS_AVAIL  &&
	     info.mech_info[0]->mech_caps & GCS_CAP_REM_AUTH 
	   )
	    IIGCn_static.rmt_mech = STalloc( info.mech_info[0]->mech_name );
	else
	    IIGCn_static.rmt_mech = ERx("none");
    }

    /*
    ** Initialize GCADM interface.
    */
    if ( ( status = gcn_adm_init()) != OK ) 
	PCexit( FAIL );

    /*
    ** Load server class info.
    */
    if ( (status = gcn_srv_load( IIGCn_static.hostname, 
				 IIGCn_static.listen_addr )) != OK )  
	PCexit( FAIL );

    /*
    ** Call erinit again to put listen address in with hostname.
    ** Announce startup.
    */

    gcu_erinit( IIGCn_static.hostname, "IIGCN", IIGCn_static.listen_addr );
    gcu_erlog( 0, 1, E_GC0151_GCN_STARTUP, NULL, 0, NULL );

    /* Log informational message if IIGCn_static.hostname and cbf .local_vnode param 
    ** values differ.
    */
    
    if ( PMget( "!.local_vnode",  &buf) != OK )
    {
        /* Param not found */	
        STncpy( lvn_value, "N/A", GCN_TYP_MAX_LEN );
    }
    else
        STncpy( lvn_value, buf, GCN_TYP_MAX_LEN );

    /* compare them */
    if ( STcompare( IIGCn_static.hostname, lvn_value ) != 0) 
    {
        /* Mismatch - Log informational message */
        earg[0].er_value = ERx(IIGCn_static.hostname);
        earg[0].er_size = STlength( earg[0].er_value );
        earg[1].er_value = ERx(lvn_value );
        earg[1].er_size = STlength( earg[1].er_value );
        gcu_erlog( 0, 1, E_GC0173_GCN_HOSTNAME_MISMATCH, NULL, 2, (PTR)earg );
    }

    /*
    ** Post initial GCA_LISTEN.  If a thread is started with
    ** an active listen, only bedchecking will be done, so
    ** also start a thread to do initial bedcheck.  We then
    ** loop in GCexec() waiting for a shutdown request.
    */
    gcn_start_session();
    gcn_start_session();
    GCexec();

    /* 
    ** Shutdown 
    */
    IIgcs_call( GCS_OP_TERM, GCS_NO_MECH, NULL );


    gcn_deregister();

    gcn_srv_term();
    gcn_adm_term();

    gcu_erlog( 0, 1, E_GC0152_GCN_SHUTDOWN, NULL, 0, NULL );

    PCexit( OK );
}


/*
** Name: gcn_err_log
**
** Description:
**	Callback function for GCS to log errors.
**
** Input:
**	error_code	Error code.
**	parm_count	Number of parameters.
**	parms		Parameters (ER_ARGUMENT array).
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	10-Jul-97 (gordy)
**	    Created.
*/

static VOID
gcn_err_log( STATUS error_code, i4  parm_count, PTR parms )
{
    gcu_erlog( 0, 1, error_code, NULL, parm_count, parms );
    return;
}


/*
** Name: gcn_msg_log
**
** Description:
**	Callback function for GCS to log messages.
**
** Input:
**	message		Message to be logged.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcn_msg_log( char *message )
{
    gcu_msglog( 0, message );
    return;
}


/*
** Name: gcn_get_key
**
** Description:
**	Callback function for GCS to retrieve server key based on
**	server listen address.
**
** Input:
**	parm		GCS get key parm.
**	    server	    Server ID (listen address)
**	    length	    Size of key buffer.
**	    buffer	    Buffer to receive key.
**
** Output:
**	parm
**	    length	    Length of key.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	21-May-97 (gordy)
**	    Created.
**	 1-Mar-06 (gordy)
**	    Server key now returned directly from gcn_get_server().
*/

static STATUS
gcn_get_key( GCS_GET_KEY_PARM *parm )
{
    char	*key;
    i4		len;
    STATUS	status = FAIL;

    if ( 
         (key = gcn_get_server( parm->server ))  &&
	 (len = (i4)STlength( key ))  &&  
	 parm->length >= (len + 1) 
       )
    {
	STcopy( key, parm->buffer );
	parm->length = len;
	status = OK;
    }

    return( status );
}


/*
** Name: gcn_usr_pwd
**
** Description:
**	Callback function for GCS to validate user and password.
**
** Input:
**	parm		GCS usr_pwd parm.
**	    user	    User ID.
**	    password	    Password.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	21-May-97 (gordy)
**	    Created.
*/

static STATUS
gcn_usr_pwd( GCS_USR_PWD_PARM *parm )
{
    return( gcn_pwd_auth( parm->user, parm->password ) );
}

/*
** Name: gcn_ip_auth
**
** Description:
**	Callback function for GCS to validate ticket.
**
** Input:
**	parm		GCS ip_parm parm.
**	    user	    User ID.
**	    ticket	    ticket.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	4-Dec-97 (rajus01)
**	    Created.
**	16-Jul-04 (gordy)
**	    gcn_use_lticket() now takes raw ticket.
*/

static STATUS
gcn_ip_auth( GCS_IP_PARM *parm )
{
    return( gcn_use_lticket(parm->user, (u_i1 *)parm->ticket, parm->length) );
}


/*
**	Name Server MIB Class definitions
*/	
GLOBALREF MO_CLASS_DEF	gcn_classes[];

/*{
** Name: gcn_init_mib() - initialize Name Server MIB
**
** Description:
**      Issues MO calls to define all GCN MIB objects.
**
** Inputs:
**      None.
**
** Side effects:
**      MIB objects defined to MO.
**
** Called by:
**      main()
**
** History:
**      28-Sep-92 (brucek)
**          Created.
*/

static VOID
gcn_init_mib( VOID )
{
    MOclassdef( MAXI2, gcn_classes );
    IIGCn_static.flags |= GCN_MIB_INIT;

    GVmo_init();
    return;
}

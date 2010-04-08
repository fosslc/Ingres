/*
** Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <erglf.h>
# include    <ci.h>
# include    <cv.h>		 
# include    <er.h>
# include    <ex.h>
# include    <gc.h>
# include    <gcccl.h>
# include    <lo.h>
# include    <me.h>
# include    <nm.h>
# include    <pc.h>		 
# include    <pm.h>
# include    <qu.h>
# include    <si.h>
# include    <sl.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>
# include    <sp.h>
# include    <mo.h>

# include    <iicommon.h>
# include    <gca.h>
# include    <gcc.h>
# include    <gccer.h>
# include    <gccgref.h>
# include    <gcm.h>
# include    <gcxdebug.h>

/*
** Name: iigcb.c 
**
** Description:
**	Main GCB entry point and layer execution control.
**      The module contains the following routines: 
** 
**          main - Main GCB entry point
**
**	This module contains the main entry point for the GCF Protocol
**	Bridge.  It controls the execution of the bridge.  It is
**	the purpose of the bridge to provide communication
**	capability between incoming connections and outgoing connections
**      using a different underlying protocol. It listens for and accepts 
**      incoming connection requests and establishes corresponding connections
**      to a local or remote comm. server, allows bi-directional data
**	transfer over the established connections, and terminates connections in
**	an orderly way.
**
**	Following is a high level description of the structure and design of
**	the Protocol Bridge and its component elements.  Descriptions in lower
**	level routines provide progresively more detail.
**
**	OVERVIEW:
**
**
**	DATA FLOW:
**
**	A Message Data Elements (MDE) is a data structure representing a
**	message which is to be processed by the an LE.
**	An MDE corresponds an OSI-defined Service Data Unit (SDU).
**
**	ASYNCHRONOUS EVENT HANDLING:
**
**	The Protocol bridge is driven by external events which occur
**	asynchronously. The main events of this type are connection requests
**	from clients, messages to be sent or received on a connection,
**      and connection termination requests. The image of asynchronous events
**  	and the way with which they are dealt is environment-specific.
**	ILC invokes certain CL routines, discussed later, which are
**	responsible for performing the operations required in a particular
**	environment for managing asynchronous events.
**
**
**	COMMUNICATION SERVER EXECUTION CONTROL
**
**	ILC invokes three CL functions in succession which control execution of
**	the Comm Server.  These are GCcinit, GCexec and GCcterm.  GCcinit is
**	responsible for server initialization in the local environment.  In
**	addition, it is invoked with a list of "callback" functions which are
**	invoked by it prior to returning.  These are mainline functions which
**	perform layer initialization and which prepare for the occurrence of
**	external events by establishing a "listening" posture and establishing
**	the callback functions associated with the external events which signal
**	the completion of the local services requested by the LE's during
**	initialization.
**
**	When and how external events are allowed and how they actually occur is
**	environment specific, and is controlled by GCexec.  This is invoked
**	by ILC upon return from GCcinit.  Control is not returned to ILC by
**	GCexec until it is time for server shutdown.  Thus GCexec is
**	responsible for managing server execution in an environment-specific
**	way which is transparent to ILC.  Server shutdown is indicated by the
**	invocation of the CL routine GCshut by an LE during execution.  This
**	eventually causes GCexec to return control to its caller.  Upon
**	return from GCexec, ILC invokes GCcterm with a callback list.
**	These routines are the individual termination routines for the LE's.
**	Upon return from GCcterm, everything required for graceful server
**	termination is complete, and ILC terminates execution.
**
**	The exact nature of server execution is transparent to ILC.  It simply
**	invokes local initialization, execution and termination functions which
**	perform the required operations in a way appropriate to the local
**	environment, and which ensure the invocation of mainline callback
**	routines at appropriate times.
**	
**
**	SUMMARY:
**
**	The functions performed by this module are the control of
**	initialization, exeution and termination.  These are done by invoking
**	CL routines which are specific to the environment.  Initialization is
**	performed by GCcinit(); execution control is performed by GCexec; and
**	termination is performed by GCcterm.
**
** History:
**      30-Apr-90 (cmorris)
**          Initial module creation
**  	04-Apr-92 (cmorris)
**  	    Added INGNETLIB to ming's NEEDLIBS
**  	08-Apr-92 (cmorris)
**  	    Removed Async from name of bridge.
**  	08-Apr-92 (cmorris)
**  	    Added support for environment variable to
**  	    indicate number of connections brdige will support.
**	08-jun-92 (leighb) 
**	    DeskTop Porting Change: added cv.h & pc.h
**	02-Sep-92 (seiwald)
**	    NEEDLIB ULFLIB no longer, as we don't use ule_format anymore.
**	10-Feb-93 (seiwald)
**	    gcc_er_init is now void.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	16-Feb-93 (seiwald)
**	    Including gcc.h now requires including gca.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	16-dec-93 (tyler)
**	    Replaced GChostname() with call to PMhost().
**	20-Nov-95 (gordy)
**	    Extracted initialization/termination routines to gccinit.c.
**	20-Nov-95 (gordy)
**	    Added standalone parameter to gcc_init().
**	05-Dec-95 (rajus01)
**	    Added some error handling support when reading command
**	    line parameters.
**	    Added new GLOBALDEF GCC_BPMR to read from config.dat.
**	    Added support to run multiple instances of bridge.
**	01-Feb-96 (rajus01)
**	    Added gcb_alinit(), gcb_alterm() in the call_list. 
**	09-Feb-96 (rajus01)
**	    Set the revision level of the server to 1.1/02.
**	29-Mar-96 (rajus01)
**	    Added #ifdef CI_INGRES_BRIDGE stmt., to stop compilation
**	    errors.  
**	22-Apr-96 (hanch04)
**	    remove ** in front of ming hints.
**	22-May-96 (rajus01)
**          Change #ifdef statement around gcc_init() function call.
**	27-sep-1996 (canor01)
**	    Move global data definitions to gccdata.c.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().  Moved MIB stuff to this file.
**	12-Nov-97 (rajus01)
**	    Removed the semicolon in the 'if' statement.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitons to gcm.h.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	15-jun-1999 (somsa01)
**	    Printout E_GC2A10_LOAD_CONFIG also on sucessful startup.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_GC2A10_LOAD_CONFIG.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	28-jul-2000 (somsa01)
**	    Print a FAIL message to standard output upon startup failure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
*/

/*
** Ming hints:
**
NEEDLIBS= GCFLIB COMPATLIB INGNETLIB
PROGRAM= (PROG1PRFX)gcb
*/

/*
**  Forward and/or External function references.
*/
static		VOID		gcb_init_mib( VOID );
FUNC_EXTERN	i4		GCX_exit_handler();
FUNC_EXTERN	STATUS		gcc_pbinit();
FUNC_EXTERN	STATUS		gcc_pbterm();
GLOBALREF	N_ADDR		gcb_from_addr;
GLOBALREF	N_ADDR		gcb_to_addr;
GLOBALREF	GCC_BPMR	gcb_pm_reso;


/*
** Name: main
**
** Description:
**	This is the main entry point for the Protocol bridge.  It is a driver
**	routine whose sole function is invoking other functions in the correct
**	order to drive the execution of the bridge.  The routines and the
**	order in which they are invoked are as follows:
**
**	gcc_init()	Global Protocol bridge main line initialization
**	GCcinit()	CL level initialization.
**	GCexec()	CL routine that enables and controls execution.
**	GCcterm()	CL level initialization.
**	gcc_term()	Global Protocol Bridge main line termination.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Apr-90 (cmorris)
**          Initial routine creation
**	20-Nov-95 (gordy)
**	    Added standalone parameter to gcc_init().
**	05-Dec-95 (rajus01)
**	    Added error handling support when reading command
**	    line parameters.
**	    Added new GLOBALDEF GCC_BPMR to read from config.dat.
**	    Added support to run multiple instances of bridge.
**	01-Feb-96 (rajus01)
**	    Added gcb_alinit(), gcb_alterm() in the call_list. 
**	09-Feb-96 (rajus01)
**	    Seperate trace log file is no longer needed
**	    since bridge calls GCA now.
**	27-Feb-96 (rajus01)
**          Added CI_INGRES_NET parameter to gcc_init(). This enables
**          Protocol Bridge check for it's AUTH_STRING.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().
**	15-jun-1999 (somsa01)
**	    Printout E_GC2A10_LOAD_CONFIG also on sucessful startup.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_GC2A10_LOAD_CONFIG.
**      26-Apr-2007 (hanal04) SIR 118196
**          Make sure PCpid will pick up the current process pid on Linux.
*/

i4
main( i4  argc, char **argv )
{
    STATUS	generic_status = OK;
    CL_ERR_DESC	system_status;
    EX_CONTEXT  context;
    STATUS	status;
    GCC_ERLIST	erlist;
    STATUS	(*call_list[7])();
    i4		i, call_count;
    i4		cmd_args   = 0;
    char	usage[] = { "Usage: iigcb -from <protocol>" };
    char	usage1[] = { " -to <protocol> <hostname> < listen_address>" };

#ifdef LNX
    PCsetpgrp();
#endif

    MEfill( sizeof( system_status ), 0, (PTR)&system_status );
    MEadvise( ME_INGRES_ALLOC );
    EXdeclare( GCX_exit_handler, &context );
    SIeqinit();

    /* 
    ** Parse command line. 
    */
    for( i = 1; i < argc; i++ )
	if ( ! STcompare( argv[i], "-from" ) )
	{
	    if ( ++i >= argc )
	    {
		SIfprintf( stderr, "\n%s%s\n\n", usage, usage1 );
		SIflush( stderr );
		SIstd_write(SI_STD_OUT, "\nFAIL\n");
		EXdelete();
		PCexit( FAIL );
	    }

	    STcopy( argv[i], gcb_from_addr.n_sel );
	    CVupper( gcb_from_addr.n_sel );
	    ++cmd_args;
	}
	else  if ( ! STcompare( argv[ i ], "-to" ) )
	{
	    if ( (i + 3) >= argc )
	    {
		SIfprintf( stderr, "\n%s%s\n\n", usage, usage1 );
		SIflush( stderr );
		SIstd_write(SI_STD_OUT, "\nFAIL\n");
		EXdelete();
		PCexit( FAIL );
	    }

	    STcopy( argv[ ++i ], gcb_to_addr.n_sel );
	    STcopy( argv[ ++i ], gcb_to_addr.node_id );
	    STcopy( argv[ ++i ], gcb_to_addr.port_id );
	    CVupper( gcb_to_addr.n_sel );
	    ++cmd_args;
	}

    if ( cmd_args )
	if ( cmd_args == 2 )
	    gcb_pm_reso.cmd_line = TRUE;
	else
	{
	    SIfprintf( stderr, "\n%s%s\n\n", usage, usage1 );
	    SIflush( stderr );
	    SIstd_write(SI_STD_OUT, "\nFAIL\n");
	    EXdelete();
	    PCexit( FAIL );
	}

    /*
    ** Perform general initialization.  The bridge may be run
    ** for async lines as a part of Ingres/Net.  Otherwise,
    ** Bridge authorization is required.
    */
    if ( gcb_pm_reso.cmd_line  &&
	 ! STcasecmp( gcb_from_addr.n_sel, ERx("async") ) ) 
	status = gcc_init( FALSE, CI_INGRES_NET, argc, argv,
			   &generic_status, &system_status );
    else
    {
#ifdef CI_INGRES_BRIDGE
	status = gcc_init( FALSE, CI_INGRES_BRIDGE, argc, argv,
			   &generic_status, &system_status );
#else 
	generic_status = E_GC2A0F_NO_AUTH_BRIDGE;
	status = FAIL;
#endif
    }

    if ( status != OK )
    {
	gcc_er_log( &generic_status, &system_status, NULL, NULL );
	generic_status = E_GC2A01_INIT_FAIL;
	gcc_er_log( &generic_status, NULL, NULL, NULL );
	SIstd_write(SI_STD_OUT, "\nFAIL\n");
	EXdelete();
	PCexit( FAIL );
    }

    /*
    ** Perform standalong Bridge Server initialization.
    */
    IIGCc_global->gcc_flags |= GCC_STANDALONE;
    gcb_init_mib();

    /*
    ** Initialize the protocol layers.
    */
    call_list[0] = GCcinit;
    call_list[1] = gcb_alinit;
    call_list[2] = gcc_pbinit;
    call_count   = 3;

    if ( gcc_call( &call_count, call_list, 
		   &generic_status, &system_status ) != OK )
    {
	gcc_er_log( &generic_status, &system_status, NULL, NULL );
	generic_status = E_GC2A01_INIT_FAIL;
	gcc_er_log( &generic_status, NULL, NULL, NULL );
	SIstd_write(SI_STD_OUT, "\nFAIL\n");
    }
    else
    {
	{
	    /*
	    ** Obtain and fix up the configuration name, then log it.
	    */
	    char	server_flavor[256], server_type[32];
	    char	*tmpbuf = PMgetDefault(3);

	    if (!STbcompare( tmpbuf, 0, "*", 0, TRUE ))
		STcopy("(DEFAULT)", server_flavor);
	    else
		STcopy(tmpbuf, server_flavor);

	    STcopy(PMgetDefault(2), server_type);
	    CVupper(server_type);

	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].value = server_flavor;
	    erlist.gcc_erparm[0].size = STlength(server_flavor);
	    erlist.gcc_erparm[1].value = server_type;
	    erlist.gcc_erparm[1].size = STlength(server_type);

	    generic_status = E_GC2A10_LOAD_CONFIG;
	    gcc_er_log( &generic_status, (CL_ERR_DESC *)NULL,
			(GCC_MDE *)NULL, &erlist);
	}

	erlist.gcc_parm_cnt 	   = 1;
	erlist.gcc_erparm[0].size  = STlength( IIGCc_rev_lvl );
	erlist.gcc_erparm[0].value = IIGCb_rev_lvl;
	generic_status 		   = E_GC2A03_STARTUP;
	gcc_er_log( &generic_status, NULL, NULL, &erlist );

	/*
	** Invoke GCexec to enter the main phase of Protocol bridge
	** execution.  GCexec does not return until bridge shutdown 
	** is initiated by invoking GCshut.
	*/
	GCexec();

	generic_status = E_GC2A04_SHUTDOWN;
	gcc_er_log( &generic_status, &system_status, NULL, NULL );
    }

    /*
    ** GCexec has returned.  Bridge shutdown is in process.
    ** Initialize the call list to specify termination rotutines.
    ** Call in reverse order of initialization but don't call 
    ** termination routines whose initialization routines were 
    ** not called (call_count has the number of successful 
    ** initialization calls).
    */
    call_list[0] = gcb_alterm;
    call_list[1] = gcc_pbterm;
    call_list[2] = GCcterm;

    if ( gcc_call( &call_count, &call_list[ 3 - call_count ], 
		   &generic_status, &system_status ) )
	gcc_er_log( &generic_status, &system_status, NULL, NULL );

    /*
    ** Do general server termination.
    */
    if ( gcc_term( &generic_status, &system_status ) != OK )
	gcc_er_log( &generic_status, &system_status, NULL, NULL );

    EXdelete();
    PCexit( OK );
} /* end main */


/*
** Name: gcb_classes
**
** Description:
**	MO definitions for Bridge MIB objects.
**
** History:
**      22-Apr-96 (rajus01)
**          Created.
*/
static MO_CLASS_DEF	gcb_classes[] =
{
    {
	0,
	GCB_MIB_CONN_COUNT,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_conn_ct ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_conn_ct ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCB_MIB_IB_CONN_COUNT,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ib_conn_ct ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ib_conn_ct ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCB_MIB_CONNECTION,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.lcl_cei ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.lcl_cei ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCB_MIB_CONNECTION_FLAGS,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.flags ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	0,
	GCB_MIB_MSGS_IN,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_msgs_in ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_msgs_in ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCB_MIB_MSGS_OUT,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_msgs_out ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_msgs_out ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCB_MIB_DATA_IN,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_data_in ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_data_in ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCB_MIB_DATA_OUT,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_data_out ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_data_out ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0
    }
};

/*
** Name: gcb_init_mib()
**
** Description:
**      Issues MO calls to define all Bridge MIB objects.
**
** Inputs:
**      None.
**
** Side effects:
**      MIB objects defined to MO.
**
** History:
**      22-Apr-96 (rajus01)
**          Created.
*/

static VOID
gcb_init_mib( VOID )
{
    MO_CLASS_DEF	*class_def;

    for( class_def = &gcb_classes[0]; class_def->classid; class_def++ )
	if( class_def->cdata == (PTR)-1 )
	    class_def->cdata = (PTR)IIGCc_global;

    MOclassdef( MAXI2, gcb_classes );

    return;
}

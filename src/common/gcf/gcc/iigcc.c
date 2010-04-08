
/*
** Copyright (c) 1987, 2009 Ingres Corporation All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>
#include    <cv.h>
#include    <ci.h>
#include    <er.h>
#include    <ex.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <pm.h>
#include    <qu.h>
#include    <si.h>
#include    <sl.h>
#include    <st.h>
#include    <sp.h>
#include    <mo.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcc.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcm.h>
#include    <gcxdebug.h>

#if defined(OS_THREADS_USED) && defined(VMS)
#include <astmgmt.h>
#endif

/*
**
** Name: iigcc.c
**
** Description:
**	Main Comm Server entry point and layer execution control.
**      The module contains the following routines: 
** 
**          main - Main GCC entry point
**
**	This module contains the main entry point for the GCF Communication
**	Server.  It controls the execution of the Communicaton Server.  It is
**	the purpose of the Communication Server to provide communication
**	capability with remote nodes to clients and servers executing in the
**	local node.  It establishes outgoing connections, listens for and
**	accepts incoming connection requests, allows bi-directional data
**	transfer over an established connection, and terminates connections in
**	an orderly way.
**
**	Following is a high level description of the structure and design of
**	the Comm Server and its component elements.  Descriptions in lower
**	level routines provide progresively more detail.
**
**	OVERVIEW:
**
**	The comm server is based on the OSI seven layer basic reference model
**	for inter-system connection and communication, and contains the upper
**	four layers.  The implementation within the comm server of each of the
**	layers abstractly defined in the OSI model is generically referred to
**	as a Layer Entity (LE).  Four LE's are part of the comm server.  These
**	are the Application Layer (AL), the Presentation Layer (PL), the
**	Session Layer (SL) and the Transport Layer (TL).  The present module
**	provides the Inter Layer Control, or ILC, functions and services
**	required to coordinate the execution of the LE's.  It is in the LE's
**	that the actual business of communication is carried out.  the ILC
**	functions as a global supervisor for the execution of the Comm Server
**	and the associated LE's.
**
**	An LE may be viewed as an independent executable entity.  It performs an
**	operation on a data structure representing a message, and passes it on
**	to another LE to perform additional processing.  The fundamental
**	notion, then, is that of messages flowing bi-directionally throught the
**	communication server, being passed from one LE to the next, in the
**	sequence appropriate to the flow direction.
**
**
**	DATA FLOW:
**
**	A Message Data Elements (MDE) is a data structure representing a
**	message which is to be processed by the an LE.
**	An MDE corresponds an OSI-defined Service Data Unit (SDU).
**      Inside an LE the MDE is transformed by the addition of layer-specific  
**      protocol information to an OSI-defined Protocol Data Unit (PDU).  With 
**      the addition of a set of service request parameters, the MDE becomes 
**      an SDU which is then sent to an adjoining LE to invoke the layer 
**	services provided by that LE (in the case of a lower layer) or to notify
**	the layer of the completion of a service (in the case of a lower layer).
**
**	The MDE is the vehicle for constructing a message which is actually
**	sent out over a network connection to a peer comm server in a target
**	system.  Each layer adds protocol control information to that of 
**	previous layers, in the fashion of constructing an onion.  It also
**	inserts service request parameters in an area of the MDE reserved for 
**	that purpose, in order to specify the service function required of an
**	adjoining layer.  The motivation for performing all this within the MDE
**	is two-fold: to minimize data movement required as a message is built,
**	and to simplify the inter-layer communication and execution control.
**	A single entity flows through the layer structure and is eventually 
**	sent and received over the network.
** 
**	For example, consider the case where an MDE is passed to Presentation
**	Layer (i.e., the LE embodying the services and protocols defined for
**	the PL).  The MDE embodies, say, a P-CONNECT Request SDU.   It contains
**	user data from the adjoining layer (AL) and the parameters for the
**	P-CONNECT service.  Within the PL LE, a Connect Presentation (CP) PDU
**	is constructed within the MDE by the setting of appropriate fields
**	based on the service parameters.  A new group of service parameters are
**	set in the appropriate area of the MDE, and it now becomes an S-CONNECT
**	SDU destined for the Session Layer.  It is sent to the SL LE by PL.
**
**
**	ASYNCHRONOUS EVENT HANDLING:
**
**	The communication server is driven by external events which occur
**	asynchronously. The main events of this type are connection requests
**	from clients, messages to be sent or received on a connection, and
**	connection termination requests.  These events may be locally generated
**	and received on a local IPC connection, or remotely generated and
**	received on a network connection. The image of asynchronous events and
**	the way with which they are dealt is environment-specific.  ILC invokes
**	certain CL routines, discussed later, which are responsible for
**	performing the operations required in a particular environment for
**	managing asynchronous events.
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
**	initialization.  Any layer may have an initialization routine.
**	Currently, only AL, PL and TL are implemented.
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
**  History:
**      07-Jan-88 (jbowers)
**          Initial module creation
**      12-Oct-88 (jbowers)
**          Added error logging control to gcc_init and gcc_term.
**      22-Feb-89 (jbowers)
**          Added support for specification of max inbound and outbound
**	    connections in gcc_init().
**      22-Mar-89 (jbowers)
**          Fix for bug # 5106:
**	    Check return code from CVan in gcc_init().
**	12-jun-89 (jorge)
**	    added include descrip.h
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**	    Added call to gcx_init.
**	29-jun-89 (seiwald)
**	    Include gcxdebug.h.
**      14-Nov-89 (cmorris)
**          Cleaned up comments.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: declare a list of the queues
**	     of MDE's.
**	22-Feb-90 (seiwald)
**	    GCcexec and GCcshut removed.  IIGCC now shares GCexec and 
**	    GCshut with IIGCN.
**      01-Mar-90 (cmorris)
**	    Put and fetch installation id from GCC global.
**	06-Mar-90 (seiwald)
**	    Removed GCcstart and callback list operations of GCcinit
**	    and GCcterm, which IIGCC now handles.
**	03-May-90 (jorge)
**	    Copy result of NMgtAt("II_INSTALLATION") and remove call to 
**	    SIeqinit(). 
**	12-Jun-90 (jorge)
**	    re-apply NMgtAt("II_INSTALLATION") fix.
**	19-Jun-90 (seiwald)
**	     Cured the CCB table of tables mania - active CCB's now live
**	     on a single array.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	    Don't log startup and shutdown messages for PMFE.
**	23-apr-91 (seiwald)
**	    Make stevet's change conditionally compile with GCF63.
**	13-aug-91 (stevet)
**	    Change read-in character set support to use II_CHARSET symbol.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**  	18-Dec-91 (cmorris)
**  	    Made gcc_init and gcc_term static and corrected declarations 
**          for them.
**  	04-Apr-92 (cmorris)
**  	    Added INGNETLIB to ming's NEEDLIBS
**	02-Sep-92 (seiwald)
**	    NEEDLIB ULFLIB no longer, as we don't use ule_format anymore.
**	02-Dec-92 (brucek)
**	    Added MIB support.
**	16-Dec-92 (gautam)
**	    Added PM support and support for the [ -instance=xx ] 
**	    command line.
**	31-Dec-92 (brucek)
**	    Don't abort if PMload fails. 
**	11-Jan-92 (edg)
**	    Moved gcx_init() so that it comes after PM stuff.
**	04-Feb-93 (edg)
**	    Change PM symbol name to conform to LRC.
**	10-Feb-93 (seiwald)
**	    gcc_er_init is now void.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-Sep-93 (brucek)
**	    Removed ADFLIB from NEEDLIBS.
**	16-dec-93 (tyler)
**	    Replaced GChostname() with call to PMhost().
**      13-Apr-1994 (daveb) 62240
**          Add new metric objects:
**		 "exp.gcf.gcc.msgs_in" - Network packets in
**		 "exp.gcf.gcc.msgs_out"- Network packets out
**		 "exp.gcf.gcc.data_in" - Network data in ( in kbytes )
**		 "exp.gcf.gcc.data_out"- Network data out ( in kbytes )
**	20-Nov-95 (gordy)
**	    Extracted initialization/termination functions to gccinit.c.
**	20-Nov-95 (gordy)
**	    Added standalone parameter to gcc_init().
**	27-Feb-96 (rajus01)
**          Added CI_INGRES_NET parameter to gcc_init(). This enables
**          Protocol Bridge check for AUTH STRING. Added after consulting
**	    with gordy.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().  Moved MIB stuff into this file.
**	17-Feb-98 (gordy)
**	    Permit ib/ob max to be set through GCM.
**	31-Mar-98 (gordy)
**	    Adding MO objects for default encryption mechanism and mode.
**	 8-Sep-98 (gordy)
**	    Start installation registry protocols if master.
**	30-Sep-98 (gordy)
**	    Added MIB for registry mode.  Start protocols if set 'master'.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitons to gcm.h.
**	15-jun-1999 (somsa01)
**	    Printout E_GC2A10_LOAD_CONFIG also on sucessful startup.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_GC2A10_LOAD_CONFIG.
**      24-Feb-00 (rajus01)
**          Reset the call_count. Using iigcstop() to stop the
**          net server caused coredump.
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
**	26-Jun-2001 (wansh01)
**	    Moved gcxlevel declaration from gcudata.c to here. 
**          replace gcxlevel with IIGCa_global->gcc_trace_level.
**	10-Dec-02 (gordy)
**	    GCC now needs CUFLIB.
**	 6-Jun-03 (gordy)
**	    Enhanced MIB information.
**	21-Jan-04 (gordy)
**	    Added cover routine for gcc_tl_registry() to handle the
**	    gcc_call() requirements and exposable to AL.  Use flags
**	    to signal registry initialization between MO set routine
**	    an AL.
**	28-Jan-2004 (fanra01)
**	    Moved gcc_init_registry into gccal.c for inclusion in
**	    the gcf library.
**	 6-Apr-04 (gordy)
**	    Made the protocol MIB entries MO_ANY_READ for browsing.
**	23-Sep-04 (wansh01)
**	   added gcc_adm_init() and gcc_adm_term to support GCC/ADMIN. 
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/

/*
** Ming hints:
**

NEEDLIBS= GCFLIB CUFLIB COMPATLIB INGNETLIB
PROGRAM= (PROG1PRFX)gcc
*/

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN	i4	GCX_exit_handler();
static		VOID	gcc_init_mib( VOID );
static		STATUS	gcc_get_mode( i4, i4, PTR, i4, char * );
static		STATUS	gcc_set_mode( i4, i4, char *, i4, PTR );
static		STATUS	gcc_get_reg( i4, i4, PTR, i4, char * );
static		STATUS	gcc_set_reg( i4, i4, char *, i4, PTR );


/*
** Name: main	- Main entry and Inter-Layer Control
**
** Description:
**	This is the main entry point for the GCC comm server.  It is a driver
**	routine whose sole function is invoking other functions in the correct
**	order to drive the execution of the Comm Server.  The routines and the
**	order in which they are invoked are as follows:
**
**	gcc_init()	Global Comm Server main line initialization
**	GCcinit()	Comm Server CL level initialization
**	GCexec()	CL routine that enables and controls execution.
**	GCcterm()	CL routine that performs system-specific termination
**	gcc_term()	Global Comm Server main line termination.
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
**      07-Jan-88 (jbowers)
**          Initial routine creation
**	21-jun-89 (seiwald)
**	    Added call to gcx_init to initialize tracing.
**	29-jun-89 (seiwald)
**	    Moved blasted MEadvise() up to be the absolute, first thing.
**      04-Oct-89 (cmorris)
**          Removed void designation from main function. This blows some
**          compilers minds...
**      16-Nov-89 (cmorris)
**          Added gcc_slinit to initialization call list, gcc_slterm to
**          termination call list; they exist so we might as well call it.
**       9-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**      13-aug-91 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**  	16-Mar-94 (cmorris)
**  	    On initialization and termination error, only log a generic
**  	    status if one has been returned from an initialization or
**  	    termination function.
**	20-Nov-95 (gordy)
**	    Added standalone parameter to gcc_init().
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().
**	 8-Sep-98 (gordy)
**	    Start installation registry protocols if master.
**	15-jun-1999 (somsa01)
**	    Printout E_GC2A10_LOAD_CONFIG also on sucessful startup.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_GC2A10_LOAD_CONFIG.
**	21-Jan-04 (gordy)
**	    Added cover routine for gcc_tl_registry() to handle the
**	    gcc_call() requirements and exposable to AL.
**      26-Apr-2007 (hanal04) SIR 118196
**          Make sure PCpid will pick up the current process pid on Linux.
**      07-oct-2009 (stegr01)
**          Start the AST dispatcher thread here to avoid startup races
**          for Posix threaded VMS Itanium

*/

i4
main( i4  argc, char **argv )
{
    STATUS              generic_status = OK;
    CL_ERR_DESC		system_status;
    EX_CONTEXT          context;
    GCC_ERLIST	    	erlist;
    STATUS		(*call_list[7])();
    i4			call_count;

#ifdef LNX
    PCsetpgrp();
#endif


#if defined(OS_THREADS_USED) && defined(VMS)
    IIMISC_initialise_astmgmt (NULL);
#endif


    MEfill( sizeof( system_status ), 0, (PTR)&system_status );
    MEadvise( ME_INGRES_ALLOC );
    EXdeclare( GCX_exit_handler, &context );

    /*
    ** Perform general initialization.
    */
    if ( gcc_init( TRUE, CI_INGRES_NET, argc, argv, 
		   &generic_status, &system_status ) != OK )
    {
	gcc_er_log( &generic_status, &system_status, NULL, NULL );
	generic_status = E_GC2005_INIT_FAIL;
	gcc_er_log( &generic_status, NULL, NULL, NULL );
	SIstd_write(SI_STD_OUT, "\nFAIL\n");
	EXdelete();
	PCexit(OK);
    }

    /*
    ** Perform standalone Comm Server initialization.
    */
    IIGCc_global->gcc_flags |= GCC_STANDALONE;
    gcc_init_mib();
    gcc_adm_init();

    /*
    ** Initialize protocol layers.
    */
    call_list[0] = GCcinit;
    call_list[1] = gcc_alinit;
    call_list[2] = gcc_plinit;
    call_list[3] = gcc_slinit;
    call_list[4] = gcc_tlinit;
    call_count = 5;

    /*
    ** If installation registry master, 
    ** initialize the registry protocols.
    */
    if ( IIGCc_global->gcc_reg_mode == GCC_REG_MASTER )
    {
	call_list[ call_count ] = gcc_init_registry;
	call_count++;
    }

    if ( gcc_call( &call_count, call_list, 
		   &generic_status, &system_status ) != OK )
    {
	gcc_er_log( &generic_status, &system_status, NULL, NULL );
	generic_status = E_GC2005_INIT_FAIL;
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
	    gcc_er_log( &generic_status, (CL_ERR_DESC *)NULL, (GCC_MDE *)NULL,
			&erlist);
	}

	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = STlength(IIGCc_rev_lvl);
	erlist.gcc_erparm[0].value = IIGCc_rev_lvl;
	generic_status = E_GC2006_STARTUP;
	gcc_er_log( &generic_status, NULL, NULL, &erlist );

	/*
	** Invoke GCexec to enter the main phase of communication server
	** execution.  GCexec does not return until server shutdown is
	** initiated by invoking GCshut.
	*/
	GCexec();

	generic_status = E_GC2002_SHUTDOWN;
	gcc_er_log( &generic_status, NULL, NULL, NULL );
    }

    /*
    ** GCexec has returned.  Server shutdown is in process.
    ** Initialize the call list to specify termination routines.
    ** Call in reverse order of initialization but don't call 
    ** termination routines whose initialization routines were 
    ** not called (call_count has the number of successful 
    ** initialization calls).
    */
    call_list[0] = gcc_tlterm;
    call_list[1] = gcc_slterm;
    call_list[2] = gcc_plterm;
    call_list[3] = gcc_alterm;
    call_list[4] = GCcterm;
    call_count = min( 5, call_count );

    if ( gcc_call( &call_count, &call_list[ 5 - call_count ], 
		   &generic_status, &system_status ) != OK )
	gcc_er_log( &generic_status, &system_status, NULL, NULL );

    gcc_adm_term();
    /*
    ** Do general server termination.
    */
    if ( gcc_term( &generic_status, &system_status ) != OK )
	gcc_er_log( &generic_status, &system_status, NULL, NULL );

    EXdelete();
    PCexit( OK );
} /* end main */



/*
** Name: gcc_classes
**
** Description:
**	MO definitions for GCC MIB objects.
*/
static MO_CLASS_DEF	gcc_classes[] =
{
    {
        0,
        GCC_MIB_TRACE_LEVEL,
       	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_trace_level),
        MO_READ | MO_WRITE,
        0,
       	CL_OFFSETOF( GCC_GLOBAL, gcc_trace_level),
        MOintget,
        MOintset,
        (PTR) -1,
        MOcdata_index
    },
    {
	0,
	GCC_MIB_CONN_COUNT,
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
	GCC_MIB_IB_CONN_COUNT,
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
	0,
	GCC_MIB_OB_CONN_COUNT,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ob_conn_ct ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ob_conn_ct ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_IB_MAX,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ib_max ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ib_max ),
	MOintget,
	MOintset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_OB_MAX,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ob_max ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ob_max ),
	MOintget,
	MOintset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_IB_MECH,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ib_mech ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ib_mech ),
	MOstrpget,
	MOstrset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_OB_MECH,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_ob_mech ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ob_mech ),
	MOstrpget,
	MOstrset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_IB_MODE,
	16,
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ib_mode ),
	gcc_get_mode,
	gcc_set_mode,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_OB_MODE,
	16,
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_ob_mode ),
	gcc_get_mode,
	gcc_set_mode,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_DATA_IN,
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
	GCC_MIB_DATA_OUT,
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
	0,
	GCC_MIB_MSGS_IN,
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
	GCC_MIB_MSGS_OUT,
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
	GCC_MIB_PL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_GLOBAL, gcc_pl_protocol_vrsn ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_pl_protocol_vrsn ),
	MOintget,
	MOnoset,
	(PTR) -1,
	MOcdata_index
    },
    {
	0,
	GCC_MIB_REGISTRY_MODE,
	16,
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCC_GLOBAL, gcc_reg_mode ),
	gcc_get_reg,
	gcc_set_reg,
	(PTR) -1,
	MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCC_MIB_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_PIB, pid ),
	MO_ANY_READ,
	0,
	CL_OFFSETOF( GCC_PIB, pid ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_PROTO_HOST,
	MO_SIZEOF_MEMBER( GCC_PIB, host ),
	MO_ANY_READ,
	GCC_MIB_PROTOCOL,
	CL_OFFSETOF( GCC_PIB, host ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_PROTO_ADDR,
	MO_SIZEOF_MEMBER( GCC_PIB, addr ),
	MO_ANY_READ,
	GCC_MIB_PROTOCOL,
	CL_OFFSETOF( GCC_PIB, addr ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_PROTO_PORT,
	MO_SIZEOF_MEMBER( GCC_PIB, port ),
	MO_ANY_READ,
	GCC_MIB_PROTOCOL,
	CL_OFFSETOF( GCC_PIB, port ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCC_MIB_REGISTRY,
	MO_SIZEOF_MEMBER( GCC_PIB, pid ),
	MO_READ,
	0,
	CL_OFFSETOF( GCC_PIB, pid ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_REG_HOST,
	MO_SIZEOF_MEMBER( GCC_PIB, host ),
	MO_READ,
	GCC_MIB_REGISTRY,
	CL_OFFSETOF( GCC_PIB, host ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_REG_ADDR,
	MO_SIZEOF_MEMBER( GCC_PIB, addr ),
	MO_READ,
	GCC_MIB_REGISTRY,
	CL_OFFSETOF( GCC_PIB, addr ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_REG_PORT,
	MO_SIZEOF_MEMBER( GCC_PIB, port ),
	MO_READ,
	GCC_MIB_REGISTRY,
	CL_OFFSETOF( GCC_PIB, port ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCC_MIB_CONNECTION,
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
	GCC_MIB_CONN_TRG_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.trg_addr.n_sel ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.trg_addr.n_sel ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TRG_NODE,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.trg_addr.node_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.trg_addr.node_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TRG_PORT,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.trg_addr.port_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.trg_addr.port_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_LCL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.lcl_addr.n_sel ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.lcl_addr.n_sel ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_LCL_NODE,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.lcl_addr.node_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.lcl_addr.node_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_LCL_PORT,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.lcl_addr.port_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.lcl_addr.port_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_RMT_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.rmt_addr.n_sel ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.rmt_addr.n_sel ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_RMT_NODE,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.rmt_addr.node_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.rmt_addr.node_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_RMT_PORT,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.rmt_addr.port_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.rmt_addr.port_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TARGET,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.target ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.target ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_USERID,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.username ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.username ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_INBOUND,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_hdr.inbound ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_hdr.inbound ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_GCA_ID,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_aae.assoc_id ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_aae.assoc_id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_FLAGS,
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
	MO_CDATA_INDEX,
	GCC_MIB_CONN_AL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_aae.a_version ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_aae.a_version ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_AL_FLAGS,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_aae.a_flags ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_aae.a_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_PL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_pce.p_version ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_pce.p_version ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_PL_FLAGS,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_pce.p_flags ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_pce.p_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_SL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_sce.s_version ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_sce.s_version ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_SL_FLAGS,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_sce.s_flags ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_sce.s_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_tce.version_no ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_tce.version_no ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TL_FLAGS,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_tce.t_flags ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_tce.t_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TL_LCL_ID,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_tce.src_ref ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_tce.src_ref ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCC_MIB_CONN_TL_RMT_ID,
	MO_SIZEOF_MEMBER( GCC_CCB, ccb_tce.dst_ref ),
	MO_READ,
	GCC_MIB_CONNECTION,
	CL_OFFSETOF( GCC_CCB, ccb_tce.dst_ref ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	0
    }
};

/*
** Name: gcc_init_mib()
**
** Description:
**      Issues MO calls to define all GCC MIB objects.
**
** Inputs:
**      None.
**
** Side effects:
**      MIB objects defined to MO.
**
** History:
**      02-Dec-92 (brucek)
**          Created.
*/

static VOID
gcc_init_mib( VOID )
{
    MO_CLASS_DEF	*class_def;

    for( class_def = &gcc_classes[0]; class_def->classid; class_def++ )
	if ( class_def->cdata == (PTR) -1 )
	    class_def->cdata = (PTR)IIGCc_global;

    MOclassdef( MAXI2, gcc_classes );
    return;
}

/*
** Name: gcc_get_mode
**
** Description:
**	MO callback function to get encryption mode, converting
**	internal numeric value to external string.
**
** History:
**	31-Mar-98 (gordy)
**	    Created
*/

static STATUS
gcc_get_mode( i4  offset, i4  size, PTR object, i4  len, char *str )
{
    i4		imode = *(i4 *)((char *)object + offset);
    char 	*smode;

    switch( imode )
    {
	case GCC_ENCRYPT_OFF :	smode = GCC_ENC_OFF_STR; break;
	case GCC_ENCRYPT_OPT :	smode = GCC_ENC_OPT_STR; break;
	case GCC_ENCRYPT_ON  :	smode = GCC_ENC_ON_STR;  break;
	case GCC_ENCRYPT_REQ :	smode = GCC_ENC_REQ_STR; break;
	default :		smode = "?"; break;
    }

    return( MOlstrout( MO_VALUE_TRUNCATED, size, smode, len, str ) );
}

/*
** Name: gcc_set_mode
**
** Description:
**	MO callback function to set encryption mode, converting
**	external string to internal numeric value.
**
** History:
**	31-Mar-98 (gordy)
**	    Created
*/

static STATUS
gcc_set_mode( i4  offset, i4  len, char *str, i4  size, PTR object )
{
    STATUS status;

    status = gcc_encrypt_mode( str, (i4 *)((char *)object + offset) );

    return( (status == OK) ? OK : E_GC200A_ENCRYPT_MODE & ~GCLVL_MASK);
}

/*
** Name: gcc_get_reg
**
** Description:
**	MO callback function to get registry mode, converting
**	internal numeric value to external string.
**
** History:
**	30-Sep-98 (gordy)
**	    Created
*/

static STATUS
gcc_get_reg( i4 offset, i4 size, PTR object, i4 len, char *str )
{
    u_i2	imode = *(u_i2 *)((char *)object + offset);
    char 	*smode;

    switch( imode )
    {
	case GCC_REG_NONE   :	smode = GCC_REG_NONE_STR;	break;
	case GCC_REG_SLAVE  :	smode = GCC_REG_SLAVE_STR;	break;
	case GCC_REG_PEER   :	smode = GCC_REG_PEER_STR;	break;
	case GCC_REG_MASTER :	smode = GCC_REG_MASTER_STR;	break;
	default :		smode = "?";			break;
    }

    return( MOlstrout( MO_VALUE_TRUNCATED, size, smode, len, str ) );
}

/*
** Name: gcc_set_reg
**
** Description:
**	MO callback function to set registry mode.  The actual
**	mode is not changed.  If the requested mode is 'master'
**	and we're configured as 'master' or 'peer', call TL to
**	open the registry protocols.  Fails otherwise.
**
** History:
**	30-Sep-98 (gordy)
**	    Created
**	21-Jan-04 (gordy)
**	    Since we are called from MO, avoid direct registry
**	    initialization which can result in additional MO calls.
**	    Set registry startup flag which will be checked in the 
**	    AL when GCA_LISTEN fails with error E_GC0032_NO_PEER 
**	    (GCM operations cause such a failure and this routine 
**	    is called as a result of a GCM operation).
*/

static STATUS
gcc_set_reg( i4 offset, i4 len, char *str, i4 size, PTR object )
{
    STATUS	status = FAIL;

    if ( ! STbcompare( str, 0, GCC_REG_MASTER_STR, 0, TRUE )  &&
         ( IIGCc_global->gcc_reg_mode == GCC_REG_MASTER  ||
	   IIGCc_global->gcc_reg_mode == GCC_REG_PEER ) )
    {
	IIGCc_global->gcc_reg_flags |= GCC_REG_STARTUP;
	status = OK;
    }

    return( status );
}


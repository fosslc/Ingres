/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<cs.h>
# include	<gcccl.h>

/*
**
**
**  History:
**      12-Feb-88 (jbowers)
**          Initial module implementation
**      18-May-88 (berrow)
**          UNIX/TCP/IP specialization.
**	5-Dec-1988 (Gordon Wright)
**	    move protocol specific data out to a configurable file
**	11-may-89 (daveb)
**	    Use <systypes.h> instead of <sys/types.h>.  Now it compiles.
**	22-aug-89 (seiwald)
**	    Declare and install a signal handler, GCcex(), to ignore
**	    SIGPIPE and EX_NO_FD.  Also, handle E_INTERRUPT from iiCLpoll().
**	23-aug-89 (seiwald)
**	    ... and removed GCcex() to ignore EX_NO_FD.  It was a silly idea.
**	17-sep-89 (seiwald)
**	    Removed old slave code; integrated functions of GCc_exec with
**	    GCcexec, GCc_init with GCcinit, etc.
**	14-Nov-89 (seiwald)
**	    Interface changes to iiCLpoll: void, and takes a i4 timer.
**	05-Jan-90 (seiwald)
**	    Don't defeat BS buffering by calling ii_BSinitiate(0,0).
**	    The CL for IIGCC on unix formerly had to run entirely 
**	    unbuffered, since IIGCC maintains full duplex connections,
**	    and the CL used buffering to deferred writes for efficiency.
**	    Now, because mainline GCA buffers, the CL doesn't defer writes,
**	    and buffering is safe.
**	06-Feb-90 (seiwald)
**	    Removed GCC_PROTO structure, which duplicated GCC_PCT.
**	    Protocol table is declared directly as GCC_PCT, so we
**	    don't have to copy from GCC_PROTO to GCC_PCT.
**	    Gutted useless debug code (nothing to debug).
**	22-Feb-90 (seiwald)
**	    GCcexec and GCcshut removed.  IIGCC now shares GCexec and 
**	    GCshut with IIGCN.
**	06-Mar-90 (seiwald)
**	    Removed GCcstart and callback list operations of GCcinit
**	    and GCcterm, which IIGCC now handles.
**	30-May-90 (seiwald)
**	    Allow the IIGCc_gcc_pct to be terminated by a "" protocol id.
**	    This make it easier to assemble the IIGCc_gcc_pct using
**	    conditional compilation.
**  	12-Feb-91 (cmorris)
**  	    Moved driver specific trace initialization where it
**  	    belongs:- in the drivers.
**	25-mar-91 (kirke)
**	    Substituted clsigs.h for signal.h as per standard.  Added
**	    systypes.h because HP-UX's signal.h includes sys/types.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Global definition of available protocols
*/
GLOBALREF GCC_PCT	IIGCc_gcc_pct;

FUNC_EXTERN STATUS GCcinit();
FUNC_EXTERN STATUS GCcterm();
FUNC_EXTERN STATUS GCpinit();
FUNC_EXTERN STATUS GCpterm();

/*
**  Definition of static variables and forward static functions.
*/


/*{
**
**  Name: GCCCL.C - CL routines used by GCC, the GCF communication server
**
**  Description:
**
**	This module contains all of the system-specific routines associated
**	with the GCF communication server.  There are two intended purposes of
**	this CL module.  The first is to isolate the CL client from
**	details of either a protocol itself or its impelementation in a
**	specific system environment (and possibly by a specific vendor).
**	Protocol access is offered in a generic way which relieves the mainline
**	client of the necessity of knowledge of protocol detail.  This also
**	makes possible the implementation of additional protocols without
**	requiring modifications to mainline code.  The second is to provide
**	access to the system-specific environment at appropriate points in the
**	execution control sequence to allow the functional abstraction of the
**	Communication Server to be implemented in a reasonable way in any
**	system environment.
**
**	This module contains two general types of routines: protocol 
**	management routines and server execution control routines.  There is a
**	protocol driver routine for each protocol supported in a specific
**	environment.  All protocol drivers have identical interfaces.  They
**	differ in internal implementation detail depending on the environment
**	and the  support for the protocol offered in that environment.  Each
**	offers to the client a reperoire of communication functions which are
**	performed identically irrespective of the particular protocol which is
**	being used (there are certain optional or additional capabilities which
**	may be available through some protocols but not others; these are
**	identified in a "protocol capability indicator" explained below, and
**	may be requested by the CL client as part of a service function
**	request).  There are, in addition to the protocol drivers a single
**	protocol initialization routine and a single protocol termination
**	routine.  Each is called once, respectively before and after all
**	invocations of protocol drivers.
**
**	The execution control routines comprise initialization, server
**	execution, shutdown signal, and termination routines.  The routine
**	GCcstart() is responsible for obtaining the command line startup
**	parameters and placing them in a table accessible by the mainline
**	client.  It would normally be invoked first, although this is not
**	guaranteed and should not be required.  The initialization routine
**	GCcinit() is invoked before any other functions except GCcstart().
**	It initializes a protocol characteristics table which is used by the
**	invoker of the services provided by the protocol driver routines to
**	ascertain certain attribute of an individual protocol, such as packet
**	size, which it must know when the protocol is invoked.  It also allows
**	any other required environment and protocol-specific initialization
**	functions to be performed at that time.
**
**	The server execution management routine GCexec() is responsible for
**	controlling normal execution of the communication server.  It is
**	invoked after completion of initialization, and retains control for the
**	entire duration of server execution.  It returns only when the server
**	is to shut down.  It performs the functions necessary in a particular
**	environment to allow and handle the occurrence of asynchronous external
**	events.
**
**	The shutdown signal routine GCshut() is invoked to request server
**	shutdown.  Shutdown may be indicated as graceful or immediate.  As a
**	result of this invocation, the server execution management routine will
**	complete execution and return to its invoker.
**
**	The termination routine GCcterm() is guaranteed to be called subsequent
**	to completion of the server execution management routine, before the
**	server process terminates.  It is responsible for environment specific
**	cleanup, including any cleanup necessary for any of the individual
**	protocols.  Upon return, the communication server goes away.
**
** 
**  Intended Uses: 
**
**	These routines are intended for use exclusively by the the GCF
**	communication server, designated GCC.  These routines are not
**	externally available.  They are used by GCC to control execution of the
**	communication server in a particular environment and to perform network
**	communication functions in support of client/server connections.
**
**
**  Assumptions:
**
**      This specification makes the following assumptions:
**
**	    - Any combination of protocol and system environment which will 
**	      be used provides sufficient functionality that it can be mapped 
**	      to the generic set of functions specified at the interface;
**	      
**	    - All native protocol interfaces which will be used deal in 8-bit 
**	      bytes or octets;
**
**	    - Routines external to the CL which are invoked as "callback"
**	      routines may execute in a "run to completion" mode, i.e., their
**	      execution will not be preemptively interrupted by other, higher
**	      priority routines which may expose global data structures to
**	      contention for alteration.  Specifically, other "callback"
**	      routines, or a separate execution instantiation of a currently
**	      executing one, will not execute preemptively.  If the local system
**	      does not guarantee this, provision must be made in CL code to
**	      guarantee this.
**
**
**  Definitions/Concepts:
**
**	Execution control routines are invoked to manage global aspects of Comm
**	Server execution.  Protocol routines are invoked to prepare for and
**	perform operations in the local network environment.  The execution
**	control routines  are guaranteed to be invoked in a particular
**	order.The first, initialization, is invoked prior to any other CL
**	function.  As part of its invocation parameters, a list of callback
**	roitines may be supplied, which it is obliged to invoke.  These
**	callback routines may invoke CL protocol routines.  Next in order after
**	initialization, the CL execution management routine is invoked to
**	perform the environment-specific functions required to implemment
**	server operation in the local environment.  Upon invocation of the
**	shutdown signal routine, execution management terminates.  Finally,
**	execution termination is invoked to perform cleanup.
**
**	Protocol handling consists of initialization and termination routines,
**	and protocol driver routines.  The protocol initialization routine is
**	guaranteed to be the first invoked, and protocol termination the last.
**	The protocol drivers are a set of routines presenting identical
**	interfaces to implement the protocols available for network
**	communication in a particular environment. Each protocol driver offers
**	the following set of executable functions:
**
**	    - OPEN: Connect to the local protocol implementation;
**	    - LISTEN : Listen on the network port for this protocol for 
**		       incoming connection requests;
**	    - CONNECT : Establish a network connection to a specified target  
**			entity;
**	    - SEND : Send a specified string of bytes over an existing network
**		     connection;
**	    - RECEIVE : Recieve a string of bytes over an existing 
**			network connection;
**	    - DISCONNECT : Terminate an existing network connection.
** 
**	These functions are invoked by the CL client, normally the Transport
**	Layer of the GCF communication server.  The function invocation
**	consists of specifying the requested function, and passing a list which
**	contains the parameters required for the execution of the function. The
**	abstract functionality implemented for each element of the function
**	repertoire is described in this section.  The quantitative details such
**	as the parameters in the parm list are specified in the Interface
**	section.
**
**	OPEN:
**
**	The invoker is requesting that initialization of this protocol be done
**	in preparation for subsequent operations.  The identity of the local
**	"public" port on which subsequent LISTEN's will be established is
**	specified to the protocol handler in the parm list.  This function is
**	always called first.
** 
**      LISTEN: 
** 
**	The invoker is requesting that the protocol handler begin listening on
**	the local port for incoming connection requests, and that notification
**	be provided via the specified completion exit when such a request is
**	received.  The LISTEN function is the most complex of the function
**	repertoire.  It is the responsibility of LISTEN to establish a
**	listening posture on the local "public" port.  The identity of the port
**	is specified to the protocol handler in the parm list.  A message
**	received on the port constitutes a request for connection from an
**	initiating host.  The protocol handler is responsible for establishing
**	the connection channel over which subsequent communication will take
**	place.  This is done in a way appropriate to the protocol and the
**	environment.  It must be done in a way which will free up the "public"
**	port for a subsequent LISTEN request.  It may involve a peer data
**	exchange.  User data received may be specified in the parameter list.
**	All information which will be subsequently required to manage the
**	connection is placed in a Protocol Control Block (PCB), which is
**	normally allocated at this time by the protocol handler. The PCB is
**	discussed in greater detail below.
**
**      CONNECT : 
** 
**      The invoker is requesting connection to a specific network entity.
**	User data to be sent may be specified in the parameter list.
**      An address is provided as a parameter in the parm list.  The form and 
**      content of the address is dependent on the  particular network,  
**      protocol and system environment.  It is in the form of two null- 
**      terminated strings.  The syntax and semantics for the various  
**      combinations of environment, protocol and protocol implementation
**      are as follows: 
**	    
**	    UNIX:TCP/IP:Internet sockets - The first element is the host name,
**	    the second is the Internet port address specified in character 
**	    form.
**
**	    VMS:DECnet - The first element is the host name;
**	    the second is the DECnet object identifier.
**
**	All information which will be subsequently required to manage the
**	connection is placed in a Protocol Control Block (PCB), which is
**	normally allocated at this time by the protocol handler. The PCB is
**	discussed in greater detail below.  The PCB will be passed by the CL
**	client on all subsequent calls as the sole identifier of the connection.
**
**      SEND: 
**
**      The invoker is requesting the transmission of a packet of data over a 
**      previously established connection.  The parm list points to the data
**      to be sent, and specifies its length.  The length must be less than or 
**      equal to the packet size for the protocol, as specified in the Protocol 
**      Control Table (PCT) entry.  If it is not, the function is not performed 
**      and an error status is returned. 
** 
**      RECEIVE: 
** 
**      The invoker is requesting the receipt of a packet of data over a 
**      previously established connection.  The parm list points to a buffer
**      area in which to receive the data, and specifies its length.  The 
**      length specifies a maximum amount of data to be received.  The protocol
**      handler sets the value upon completion of the operation to the actual 
**      length of the received data. 
** 
**	DISCONNECT:
**
**	The invoker is requesting the termination of a previously established
**      connection with another node.  The connection is broken unilaterally. 
**	User data to be sent may be specified in the parameter list.
**      It is assumed that the caller has placed the connection in a state 
**      appropriate for termination, i.e., no pending states or other events. 
**      No checks are made to prevent data flow disruption or other untoward 
**      consequences of termination in an inappropriate state.  Caveat Emptor!
**	The PCB is deallocated at this time.
** 
**	
**	The Protocol Control Block (PCB):
**
**	The PCB is a control structure managed by the CL layer for its own
**	private purposes.  The format and contents of the PCB are known
**      only to the protocol handler.  The PCB is the medium by which a 
**	particular connection is characterized to the protocol handler for its 
**      duration.  The PCB is the connection identification entity passed
**	by the client of the protocol handler on all function requests
**	pertaining to a specific connection.
** 
**      The use of a PCB is optional for any specific implementation of a  
**      protocol handler.  In the normal sequence of events, a NULL pointer is 
**      passed by a caller requesting either the CONNECT or LISTEN function. 
**      It is expected that the protocol handler will set the pointer to some 
**      value which uniquely identifies the connection established as a result 
**      of the completion of LISTEN or CONNECT.  Typically, the protocol handler
**      will require some sort of data structure which represents a context
**      element characterizing a particular connection.  It is for this  
**      purpose that the PCB abstraction exists.  The pointer value returned 
**      by the protocol handler upon completion of LISTEN or CONNECT is passed 
**      as a parameter by the CL client on all requests for operations to be 
**      executed across the established connection.  It is considered by the 
**      caller to be the sole and unique identifier of the connection at the CL 
**      interface.  Usually, the value of the pointer is the address of a PCB 
**      which contains the connection context.  This is, of course, a matter
**      private to the CL, and the value may be anything so long as it uniquely
**      identifies the connection.  It is usual, however, for the protocol 
**      handler to allocate and initialize a PCB at the time of a LISTEN or
**      CONNECT request, and to expect a pointer to that PCB to be passed on all
**      succeeding function requests pertaining to that connection.  The PCB is
**	a data structure internal to the CL.  Its content and structure are not
**	known by clients of the CL.
**
**
**	The Protocol Control Table (PCT)
**
**	The PCT identifies to the client of the CL the protocol driver routines
**	which are available, provides pointers to their entry points, and
**	specifies various attributes of a particular protocol driver such as
**	packet size.  It is constructed by the protocol initialization routine
**	GCpinit() and returned to the caller.  This is an external data
**	structure known to both the CL and the CL client.
**
**
**  Interface:
**
**	There are seven CL routines defined for the Communication
**	Server CL:
**
**	GCcinit() - The Communication Server initialization routine;
**	GCcterm() - The Communication Server termination routine;
**	GCpinit()   - The protocol interface initialization routine;
**	GCpterm()   - The protocol interface termination routine;
**	GCpdrvr()   - The protocol driver service routine(s);
**
**	The interface associated with each is described below.
*/


/*{
** Name: GCcinit()	- Communication Server initialization routine
**
** Description: 
**	This routine is called before any other Communication Server CL
**	routine.  It performs all required system-specific initialization
**	functions, including such things as disabling interrupts if necessary
**	or appropriate in the particular environment during the initialization
**	process.  Any mechanisms for dealing with asynchronous events may be
**	prepared or set up here.
**
**	A parameter list is passed consisting of a list of function entry
**	points which are to be called.  The list is delineated with an entry
**	counter.  GCcinit calls these routines in the order in which they
**	appear in the parameter list.  Each routine specified in the callback
**	list returns a value of type STATUS and has two arguments, both output,
**	which are pointers to the following types (in sequence):
**
**	STATUS
**
**	CL_ERR_DESC.
**
**	GCcinit must supply to the invoked routine pointers to elements where
**	it may place status values of the specified types.  Thus the invocation
**	of a callback routine is of the following form:
**
**	status = (*call_list->entry_pointer[n])(generic_status, system_status);
**	
**	If any returns a status other than OK, GCcinit will not call any
**	other routines on the list.  It sets its own output calling arguments
**	to the status values returned by the failing callback routine and
**	returns a status of E_DB_ERROR to its caller.
**
**
** Inputs:
**      call_list                       Pointer to a list of initialization 
**					routines to be invoked.
**	    entries			A count of the number of routine entry
**					points which follow in the list.
**	    entry_pointer		A list of zero or more pointers to
**					functions which are to be invoked.  All
**					have two output arguments of type
**					STATUS * and CL_ERR_DESC *, and all
**					return STATUS.
** Outputs:
**      generic_status			Pointer to an element of type STATUS.
**	system_status			Pointer to an element of type
**					CL_ERR_DESC. 
**	
**	Returns:
**	    OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Apr-88 (jbowers)
**          Initial function implementation.
**      18-May-88 (berrow)
**          UNIX/TCP/IP specialization.
*/

STATUS
GCcinit( generic_status, system_status )
STATUS		    *generic_status;
CL_ERR_DESC	    *system_status;
{
	i_EXsetothersig( SIGPIPE, SIG_IGN );

	return OK;
}


/*{
** Name: GCcterm	- CL Communication Server termination function 
**
** Description:
**      This routine is normally called just prior to Communication Server
**	termination.  It is not guaranteed to be called:  There may be server
**	terminations which do not permit an orderly shutdown.  If it is
**	invoked, it is subsequent to GCexec returning to its caller.
**
**	A parameter list is passed consisting of a list of function entry
**	points which are to be called.  The list is delineated with an entry
**	counter.  GCcterm calls these routines in the order in which they
**	appear in the parameter list.  Each routine specified in the callback
**	list returns a value of type STATUS and has two arguments, both output,
**	which are pointers to the following types (in sequence):
**
**	STATUS
**
**	CL_ERR_DESC.
**
**	GCcterm must supply to the invoked routine pointers to elements where
**	it may place status values of the specified types.  Thus the invocation
**	of a callback routine is of the following form:
**
**	status = (*call_list->entry_pointer[n])(generic_status, system_status);
**	
**	If any returns a status other than OK, GCcterm will not call any
**	other routines on the list.  It sets its own output calling arguments
**	to the status values returned by the failing callback routine and
**	returns a status of E_DB_ERROR to its caller.
**
** Inputs:
**      call_list                       List of initialization routines to be
**					invoked.
**	    entries			A count of the number of routine entry
**					points which follow.
**	    entry_pointer		A list of one or more function pointers
**					which are to be invoked.  All have two
**					output arguments of type STATUS * and
**					CL_ERR_DESC *, and all return STATUS.
**
** Outputs:
**      generic_status			Pointer to an element of type STATUS.
**	system_status			Pointer to an element of type
**					CL_ERR_DESC. 
**	
**	Returns:
**	    OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Apr-88 (jbowers)
**          Initial function implementation.
*/
STATUS
GCcterm( generic_status, system_status)
STATUS		    *generic_status;
CL_ERR_DESC	    *system_status;
{
	return OK;
}


/*{
** Name: GCpinit	- Protocol initialization routine
**
** Description:
**      This routine is called during the communication server initialization 
**      process.  It is guaranteed to be called before any protocol driver
**      functions are invoked.  It is responsible for allocating and  
**      initializing the Protocol Control Table (PCT), and for performing any 
**      other initialization which may be required in a specific environment. 
** 
**      The PCT is described in detail in the header file GCCCL.H.  It has an 
**      entry for each supported protocol implementation.  Entries normally, 
**      although not necessarily, correspond one-for-one to the collection of
**	protocol handler routines.
**
** Inputs:
**      none
**
** Outputs:
**      pct				Pointer to an allocated and filled-in
**					Protocol Contro Table (PCT).
**	generic_status			System-independent status (output)
**	system_status			Environment-specific status (output)
**
**	Returns:
**	    STATUS:
**		OK
**		E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Feb-88 (jbowers)
**          Initial function implementation
*/
STATUS
GCpinit(pct, generic_status, system_status)
GCC_PCT		**pct;
STATUS		*generic_status;
CL_ERR_DESC	*system_status;
{
	i4 	i;

	/* 
	** Allow the pct to be terminated by a "" protocol id.
	*/

	for( i = 0; i < IIGCc_gcc_pct.pct_n_entries; i++ )
		if( !IIGCc_gcc_pct.pct_pce[i].pce_pid[0] )
			break;

	IIGCc_gcc_pct.pct_n_entries = i;

	*pct = &IIGCc_gcc_pct;

	return OK;
}


/*{
** Name: GCpterm	- Protocol termination routine
**
** Description:
**	This routine is called during the communication server termination
**	process.  It is guaranteed that no other protocol driver functions will
**	be invoked subsequently.  It is responsible for deallocating the
**	Protocol Control Table (PCT), and for performing any other termination
**	functions associated with protocol handling which may be required in a
**	specific environment, such as closing connections to the local
**	system-provided protocol interface.
**
** Inputs:
**      pct				Pointer to the PCT.
**
** Outputs:
**	generic_status			System-independent status (output)
**	system_status			Environment-specific status (output)
**
**	Returns:
**	    STATUS:
**		OK
**		E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Feb-88 (jbowers)
**          Initial function implementation
*/

STATUS
GCpterm(pct, generic_status, system_status)
GCC_PCT		*pct;
STATUS		*generic_status;
CL_ERR_DESC	*system_status;
{
    	return OK;
}

/*
** Copyright (C) 1994, 2009 Ingres Corporation All Rights Reserved. 
*/

# include    <compat.h>
# include    <gl.h>

# include    <cv.h>
# include    <si.h>
# include    <gc.h>
# include    <gcccl.h>
# include    <me.h>
# include    <mh.h>
# include    <nm.h>
# include    <qu.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>
# include    <pc.h>
# include    <er.h>
# include    <id.h>

# include    <sl.h>
# include    <iicommon.h>
# include    <lo.h>
# include    <cs.h>
# include    <cm.h>

# include    <gca.h>
# include    <gcu.h>
# include    <gcn.h>
# include    <gcc.h>
# include    <gccpci.h>
# include    <gccer.h>
# include    <gccgref.h>
# include    <gccpb.h>
# include    <gcatrace.h>
# include    <gcxdebug.h>
# include    <pm.h>
 
/*
**
** Name: GCCPB.C - GCF Protocol Bridge module
**
** Description:
**      GCCPB.C contains all routines which constitute the Protocol Bridge. 
**
**          gcc_pb - Main entry point for the Protocol Bridge.
**          gcc_pbinit - Protocol Bridge initialization routine
**          gcc_pbterm - Protocol Bridge termination routine
**  	    gcc_pbprot_exit - Exit routine for protocol request completions
**          gcc_pb_evinp - FSM input event evaluator
**          gcc_pbout_exec - FSM output event execution processor
**          gcc_pbactn_exec - FSM action execution processor
**          gcc_psnd - Perform GCC_SEND operation.
**	    gcc_pb_abort - Protocol Bridge internal abort routine
**
**
**  History:
**      30-Apr-90 (cmorris)
**          Initial module creation
**      08-Jul-91 (cmorris)
**	    Fixes to disconnect state transitions
**	12-Jul-91 (cmorris)
**	    Fix transition on receipt of N_RESET
**	12-Jul-91 (cmorris)
**	    Fix transitions to hold onto data received before
**          Connect Confirm.
**	15-Jul-91 (cmorris)
**	    For un-updated protocol drivers, throw away completed
**	    receives/sends after disconnect has been requested.
**	15-Jul-91 (cmorris)
**	    Fixed decrementing connection counts.
**  	18-Dec-91 (cmorris)
**  	    Incorporated recent TL-equivalent changes:
**  	    - Use the right MDE when aborting;
**  	    - Check status of calls to gcc_add_ccb;
**  	    - Added include of nm.h;
**  	    - Don't free CCB on abort when we're already in SBCLD; the
**  	      CCB has already been freed to get to this state;
**  	    - If protocol driver request fails with error, fetch status
**  	      out of parameter list for logging purposes.
**      10-Jan-92 (cmorris)
**  	    Added Listen failure event and associated transitions. Previously
**          this situation shared an event with Open failure; however, the
**  	    two cases require different events/actions to be executed.
**  	10-Apr-92 (cmorris)
**  	    Added support for dynamically allocating mapping table,
**  	    "permanent" protocol bridges, and multiple connections.
**  	04-Jan-93 (cmorris)
**  	    Store/fetch remote network address in CCB instead of MDE.
**  	    Incorporated relevant recent fixes from TL.
**	10-Feb-93 (seiwald)
**	    Removed generic_status and system_status from gcc_xl(),
**	    gcc_xlfsm(), and gcc_xlxxx_exec() calls.
**	11-Feb-93 (seiwald)
**	    Merge the gcc_xlfsm()'s into the gcc_xl()'s.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	16-Feb-93 (seiwald)
**	    Including gcc.h now requires including gca.h.
**	17-Feb-93 (seiwald)
**	    gcc_find_ccb() is no more.
**	22-Mar-93 (seiwald)
**	    Fix to change of 11-feb-93: call QUinsert with a pointer to the
**	    last element on the queue, so that the MDE is inserted at the end.
**	24-mar-93 (smc)
**	    Cast parameter of MEfree to match prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**  	27-Oct-93 (cmorris)
**  	    Don't hand in mde to error logging if error is not related
**  	    to a particular connection.
**	19-jan-94 (edg)
**	    Put include of si.h before gcccl.h.  This change is for NT.
**	    Becuase the Microsoft header, io.h (eventually included by
**	    si.h), redefines open to _open, we need to make sure the
**	    redfinition occurs globally as both gcccl.h and this module
**	    refer to a structure member called open.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	05-Dec-95 (rajus01)
**	    Added support to dynamically route the connections, using
**	    call level interface to Name Server.
**	    Bridge can now be started either using command line 
**	    args  or using symbols defined in config.dat. 
**	    Added gcn_info(),  call back routine for Name server interface.
**	    Added gcc_pbconfig() for reading symbols from config.dat.
**	29-Mar-96 (rajus01)
**	    Removed is_bridge symbol referencing in the code.
**	22-Apr-96 (rajus01)
**	    Added few statistics similar to comm server, for Bridge MIB.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	20-Aug-97 (gordy)
**	    Removed gcb_pb_max and used gcc_ib_max instead.  Other
**	    general cleanup associated with globals.
**      22-Oct-97 (rajus01)
**          Replaced gcc_get_obj(), gcc_del_obj() with
**          gcc_add_mde(), gcc_del_mde() calls respectively.
**	12-Nov-97 (rajus01 )
**	   Fixed PMscan() problem. It was working fine before. It
**	   is still a puzzle to me why it didn't work. Anyway..
**	   Added scan_pmsym().
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for requests to Name Server network database.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	09-jun-1999 (somsa01)
**	    In gcc_pbprot_exit(), on successful startup, write
**	    E_GC2A10_LOAD_CONFIG to the errlog.
**	15-jun-1999 (somsa01)
**	    In gcc_pbprot_exit(), moved printout of E_GC2A10_LOAD_CONFIG to
**	    iigcb.c .
**	02-may-2000 (somsa01)
**	    In gcc_pbinit(), corrected allocated size of pm_vnode.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2000 (somsa01)
**	    In gcc_pbconfig(), make sure we do a PMinit() and a PMload()
**	    before calling PMget().
**      25-Oct-2000 (ashco01) bug #103035               
**          In gcc_pbconfig() added call to CVlower() to convert constructed 
**          parameter name search prefix 'pmsym' to lower case before call 
**          to PMscan().
**	08-mar-2001 (somsa01)
**	    In gcc_pbprot_exit(), when printing out E_GC2815_NTWK_OPEN,
**	    let's print out the normal port id as well as the numeric port
**	    id.
**	06-jun-2001 (somsa01)
**	    We now write out the successful port ids to standard output.
**      25-Jun-2001 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level
**	 6-Jun-03 (gordy)
**	    Remote target address info now stored in trg_addr.
**	16-Jan-04 (gordy)
**	    MDE's may now be 32K.
**	19-feb-2004 (somsa01)
**	    Added stdout_closed static, set to TRUE when we are done writing
**	    out to standard output. Do not run SIstd_write() when this is set.
**	    Otherwise, on Windows, SIstd_write() will run FlushFileBuffers()
**	    which will hang because no one will be reading data from standard
**	    output.
**	24-Jul-2006 (rajus01) - Bug #116394, SD issue#:107656
**	    Moved GCN_INITIATE call to gcc_pbinit() routine, GCN_TERMINATE
**	    call gcc_pbterm() routine. This resolves the GCN initialization
**	    error when the client requests to bridge server are very close
**	    each other.
**	22-Feb-2008 (rajus01) - Bug 119987, SD issue 125582 
**	    Display the actual three letter listen address for which the
**	    network open failed in the error message.
**	08-Jul-2008 (rajus01) SD issue 129333, Bug # 120599
**	    Bridge server fails to listen on the given port ( which is
**	    different from the default port for the given protocol ) 
**	    using unique configuration names.
** 	18-Dec-2008 (rajus01) SD issue 132603, Bug # 121292
**	    Client reported Bridge server's virtual memory size grows
**	    constantly for every connect/disconnect using terminal monitor
**	    eventually hang completely. Purify memory leak reports indicated
**	    that an additional PCB block gets allocated for each connect, but
**	    is never freed after the connection goes away. In this case, the
**	    listen has completed and the input event should be IBNCI instead
** 	    of IBNCIP thus additional action ABLSN is not needed. Also released
**	    CCBs when the connection goes away. The CL layer seems to do STalloc
**	    for the node_id which never gets freed. Took care of this also.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	11-Aug-09 (gordy)
**	    Remove string length restrictions.
*/

static bool	stdout_closed = FALSE;

/*
**  Forward and/or External function references.
*/

static VOID	gcc_pbprot_exit( GCC_MDE *mde );
static u_i4	gcc_pb_evinp( GCC_MDE *mde, STATUS *generic_status );
static STATUS	gcc_pbout_exec( i4  output_id, GCC_MDE *mde, GCC_CCB *ccb );
static STATUS	gcc_pbactn_exec( u_i4 action_code, GCC_MDE *mde,
							GCC_CCB *ccb );
static STATUS	gcc_pbmap( GCC_MDE *mde );
static STATUS	gcc_psnd( GCC_MDE *mde );
static VOID	gcc_pb_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *status, 
			      CL_ERR_DESC *system_status, GCC_ERLIST *erlist );
static bool     gcc_pbconfig( GCC_PCE *pce );
static VOID 	scan_pmsym( char *pmsym );
static i4       gcn_info( i4  msg_type, char *buff, i4 len );
static bool	from_flag = FALSE;
static bool	to_flag = FALSE;
static bool	pm_initialized = FALSE;

GLOBALREF   N_ADDR  	gcb_from_addr;
GLOBALREF   N_ADDR  	gcb_to_addr;
GLOBALREF   GCC_BPMR	gcb_pm_reso; 


/*
** Name: gcc_pb	- Entry point for the Protocol Bridge.
**
** Description:
**	gcc_pb is the entry point for the Protocol Bridge.  It invokes
**	the services of subsidiary routines to perform its requisite functions.
**	It is invoked with a Message Data Element (MDE) as its input parameter.
**	The MDE may come from a peer network connection and is outbound for the
**      network, or from a protocol driver and is inbound for the peer
**      network connection.
**
**	The PB interfaces directly to the protocol handler modules of the CL.
**	These are invoked by generic service requests to perform environment
**	and protocol-specific functions.  This interface is described in detail
**	in GCCCL.C.  The protocol handlers are PB's window on the world, the 
**	connection to the local communication services.
** 
**      There are three phases of PB operation: 
** 
**      - Connection initiation; 
**      - Data transfer;
**      - Connection termination. 
**
**	Each of these is discussed in detail below.  Then the operation of the
**	PB is defined by a Finite State Machine (FSM) specification.  This is
**	the definitive description of PB behavior.  A discussion of this
**	representation is given in detail in GCCAL.C.
**
**	States, input events, output events and actions are symbolized as
**	SBxxx, IBxxx, OBxxx and ABxxx respectively.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.

**
**      Connection initiation is indicated in one of two ways:
**
**	by recipt of a N_CONNECT request sent by the peer network connection;
**
**	or by completion of a LISTEN function invoked through a protocol
**      handler.
**
**      In the first case, we are driven by a peer network connection.
**      A CONNECT service request is invoked from the appropriate protocol 
**      handler to establish contact with the remote communication server.  
**      If the CONNECT request completes without error, the Data Transfer 
**      phase is entered. If not, an N_DISCONNECT request is sent to the peer
**      and the connection is abandoned.
** 
**      In the second case, we are driven by an incoming connection request.
**  	An N_CONNECT request is sent to the peer network connection and the
**      Data Transfer phase is entered.
**
**      The Data Transfer phase is initiated in both cases by invoking 
**      the RECEIVE service on the newly established connection.  Each 
**      received message is packaged into a N_DATA request and send to the
**  	peer connection. Upon receipt of an N_DATA request from the peer, 
**	a SEND is invoked to pass the message to the recipient.
**
**      The Connection Termination phase is entered as a result of
**      the receipt of an N_DISCONNECT indication. This results in 
**  	an N_DISCONNECT request being sent to the peer network connection.
**
**	Following is the list of states, events, and actions defined
**	for the Protocol Bridge, with the state characterizing an individual
**	connection.
**	
**	There are also some input events which are not actually defined in the
**	OSI machine, such as N_DATA confirm.  This is an internally-generated
**	event whose function is to drive some internal action.  This is 
**      simply a convenient way of driving actions which are specific to this
**	implementation, such as discarding MDE's and CCB's.
**

**	States:
**
**	    SBCLD   - Idle (closed), network connection does not exist.
**	    SBWNC   - Waiting for network connection (N_CONNECT confirm)
**	    SBOPN   - Open (Data Transfer)
**	    SBSDP   - Open, completion of previous send pending
**	    SBOPX   - Open (Data Transfer), XOFF received, no rcv outstanding
**	    SBSDX   - Open, completion of previous send pending, XOFF received,
**		      no rcv outstanding
**	    SBCLG   - Closing, waiting for N_DISCONNECT confirm
**
**	Input Events:
**
**	    IBOPN   -  Protocol Bridge Initial Startup
**	    IBNOC   -  N_OPEN confirm (OPEN is complete)
**	    IBNRS   -  N_RESET indication ( OPEN is complete)
**	    IBNCI   -  N_CONNECT indication (LISTEN has completed)
**  	    IBNCIP  -  N_CONNECT indication - permanent bridge
**	    IBNCQ   -  N_CONNECT request (Invoke CONNECT)
**	    IBNCC   -  N_CONNECT confirm (CONNECT has completed)
**	    IBNDR   -  N_DISCONNECT request (Invoke DISCONNECT)
**	    IBNDI   -  N_DISCONNECT indication (Connection has failed)
**	    IBNDC   -  N_DISCONNECT confirm (DISCONNECT has completed)
**	    IBSDN   -  N_DATA request (Invoke SEND)
**          IBDNI   -  N_DATA indication (RECEIVE is complete)
**	    IBDNC -    N_DATA confirm (SEND is complete) & Q is empty
**	    IBDNQ   -  N_DATA confirm (SEND is complete) & Q is not empty
**	    IBABT   -  N_P_ABORT: PB internal execution error
**	    IBXOF   -  N_XOFF request (From peer conn: shut down data flow)
**	    IBXON   -  N_XON request (From peer conn: resume data flow)
**	    IBRDX   -  N_DATA indication (RECEIVE is complete) & XOFF rcvd set
**	    IBSDNQ  -  N_DATA request && sendqueue is full && NOT XOF sent
**	    IBDNX   -  N_DATA confirm && Q is empty && XOF sent
**	    IBNCCQ  -  N_CONNECT confirm && Q is not empty
**          IBNLSF  -  N_LSNCLUP request (network listen failure)
**
**	Output Events:
**
**	    OBNCQ*  -  Send N_CONNECT request to peer
**          OBNDR   -  Send N_DISCONNECT request to peer
**          OBSDN*  -  Send N_DATA request to peer
**	    OBXOF   -  Send N_XOFF indication to peer: shut down data flow
**	    OBXON   -  Send N_XON indication to peer: resume data flow
**
**	Actions:
**
**	    ABCNC*  -  Invoke CONNECT function from protocol handler
**	    ABDCN*  -  Invoke DISCONNECT function from protocol handler
**	    ABMDE*  -  Discard the input MDE
**	    ABCCB   -  Discard the CCB associated with the input MDE
**	    ABLSN   -  Invoke LISTEN function from the protocol handler
**	    ABRVN   -  Invoke RECEIVE for normal flow data
**	    ABSDN*  -  Invoke SEND for normal flow data
**	    ABOPN*  -  Invoke OPEN function from the protocol handler
**	    ABSNQ*  -  Enq an MDE to the SEND queue (a send is pending)
**	    ABSDQ   -  Dequeue a normal or exp. MDE, if any, and send it
**	    ABSXF   -  Set the "XOFF-received" indicator in the CCB
**	    ABRXF   -  Reset the "XOFF-received" indicator in the CCB
**
**	In the FSM definition matrix which follows, the parenthesised integers
**	are the designators for the valid entries in the matrix, andare used in
**	the representation of the FSM in GCCPB.H as the entries in PBFSM_TLB
**	which are the indices into the entry array PBFSM_ENTRY.  See GCCPB.H
**	for further details.

**  Server Control State Table
**  --------------------------
**  ( See gcbalfsm.roc for further information )
**
**  State ||SBACLD|SBASHT|SBAQSC|SBACLG|  State ||SBACLD|SBASHT|SBAQSC|SBACLG|
**	  ||  00  |  01  |  02  |  03     |     ||  00  |  01  |  02  |  03  |
**  ====================================  ====================================
**  Event ||                              Event || 
**  ====================================  ====================================
**  IBALSN||SBACLD|     |     |        |  IBBARQ||      |SBACLG|SBACLG|SBACLG|
**  (00)  ||      |     |     |        |  (03)  ||      |      |      |      |
**	  ||ABALSN|     |     |        |      	||      |ABASHT|ABAQSC|ABAMDE*
**        ||      |     |     |        |       	||      |ABADIS=ABADIS=      |
**        ||      |     |     |        |       	||      |ABAMDE*ABAMDE*      |
**	  || (01) |     |     |        |    	||     	| (07) | (04) | (03) | 
**  ------++-----+-----+---------+-----|  ----------++-----+-----+-----+-----+
**  IBAABQ||      |SBACLG|SBACLG|SBACLG|  IBASHT||SBASHT|      |     |       |
**  (01)  ||      |      |      |      |  (04)  ||      |      |     |       |
**        ||      |ABASHT|ABAQSC|ABAMDE*        ||ABARSP*      |     |       |
**	  ||      |ABADIS=ABADIS=      |        || (05) |      |     |       |
**	  ||      |ABAMDE*ABAMDE*      |  ----------++-----+-----+-----+-----+
**	  ||      |(07)  | (04) |  (03)|  IBAQSC||SBAQSC|      |     |       |
**  ------++-----+-----+-----+-----+----   (05) ||      |      |     |       |
**  IBAACE||      |SBACLG|SBACLG|SBACLG|        ||ABARSP*      |     |       |
**  (02)  ||      |      |      |      |        || (06) |      |     |       |
**  	  ||      |ABASHT|ABAQSC|ABAMDE*  ----------++-----+-----+-----+-----+
**  	  ||      |ABADIS=ABADIS=      |  IBAABT||SBACLD|SBACLG|SBACLG|SBACLD|
**        ||      |ABAMDE*ABAMDE*      |   (06) ||      |      |      |      |
**        ||      | (07) | (04) | (03) |        ||      |ABADIS=ABADIS=ABACCB|
**  ------++-----+-----+-----+-----+----        ||ABAMDE*ABAMDE*ABAMDE*ABAMDE* 
**                                              || (09) | (02) | (02) | (08) | 
**                                        --- ------++-----+-----+-----+-----+
**                                        IBAAFN||      |      |      |SBACLD|
**                                         (07) ||      |      |      |      |
**                                              ||      |      |      |ABACCB|
**                                              ||      |      |      |ABAMDE*
**                                              ||      |      |      | (08) |
**                                        --- ------++-----+-----+-----+-----+
**

**  Server Connection State Table
**  -----------------------------
**	State ||SBCLD|SBWNC|SBOPN|SBSDP|SBOPX|SBSDX|SBCLG|
**	      ||  00 |  01 |  02 |  03 |  04 |  05 |  06 |
**	==================================================
**	Event ||
**	==================================================
**	IBOPN ||SBCLD|     |     |     |     |     |     |
**	(00)  ||     |     |     |     |     |     |     |
**	      ||ABOPN*     |     |     |     |     |     |
**	      || (1) |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNOC ||SBCLD|     |     |     |     |     |     |
**	(01)  ||     |     |     |     |     |     |     |
**	      ||ABLSN|     |     |     |     |     |     |
**	      ||ABCCB|     |     |     |     |     |     |
**	      ||ABMDE*     |     |     |     |     |     |
**	      || (2) |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNRS ||SBCLD|SBCLD|     |     |     |     |     |
**	(02)  ||     |OBNDR|     |     |     |     |     |
**	      ||ABCCB|ABCCB|     |     |     |     |     |
**	      ||ABMDE*ABMDE*     |     |     |     |     |
**	      || (3) | (4) |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNCI ||SBOPN|     |     |     |     |     |     |
**	(03)  ||OBNCQ*     |     |     |     |     |     |
**	      ||ABRVN|     |     |     |     |     |     |
**	      || (5) |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNCQ ||SBWNC|     |     |     |     |     |     |
**	(04)  ||     |     |     |     |     |     |     |
**	      ||ABCNC*     |     |     |     |     |     |
**	      || (6) |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNCC ||     |SBOPN|     |     |     |     |     |
**	(05)  ||     |     |     |     |     |     |     |
**	      ||     |ABRVN|     |     |     |     |     |
**  	      ||     |ABMDE*     |     |     |     |     |
**	      ||     | (7) |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNDR ||SBCLD|SBCLG|SBCLG|SBCLG|SBCLG|SBCLG|     |
**	(06)  ||     |     |     |     |     |     |     |
**	      ||ABMDE*ABDCN*ABDCN*ABDCN*ABDCN*ABDCN*     |
**	      || (8) | (9) | (9) | (9) | (9) | (9) |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNDI ||     |SBCLG|SBCLG|SBCLG|SBCLG|SBCLG|SBCLG|
**	(07)  ||     |OBNDR|OBNDR|OBNDR|OBNDR|OBNDR|     |
**	      ||     |ABDCN*ABDCN*ABDCN*ABDCN*ABDCN*ABMDE*
**	      ||     | (10)| (10)| (10)| (10)| (10)| (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNDC ||     |     |     |     |     |     |SBCLD|
**	(08)  ||     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |     |ABCCB|
**	      ||     |     |     |     |     |     |ABMDE*
**	      ||     |     |     |     |     |     | (3) |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBSDN ||     |SBWNC|SBSDP|SBSDP|SBSDX|SBSDX|     |
**	(09)  ||     |     |     |     |     |     |     |
**	      ||     |ABSNQ*ABSDN*ABSNQ*ABSDN*ABSNQ*     |
**	      ||     | (36)| (11)| (12)| (13)| (14)|     |
**	------++-----+-----+-----+-----+-----+-----+-----+

**	State ||SBCLD|SBWNC|SBOPN|SBSDP|SBOPX|SBSDX|SBCLG|
**	      ||  00 |  01 |  04 |  06 |  07 |  08 |  09 |
**	==================================================
**	Event ||
**	==================================================
**	IBDNI ||     |     |SBOPN|SBSDP|     |     |SBCLG|
**	(10)  ||     |     |OBSDN*OBSDN*     |     |     |
**	      ||     |     |ABRVN|ABRVN|     |     |ABMDE*
**	      ||     |     | (15)| (16)|     |     | (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBDNC ||     |     |     |SBOPN|     |SBOPX|SBCLG|
**	(11)  ||     |     |     |     |     |     |     |
**	      ||     |     |     |ABMDE*     |ABMDE*ABMDE*
**	      ||     |     |     | (17)|     | (18)| (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBDNQ ||     |     |     |SBSDP|     |SBSDX|SBCLG|
**	(12)  ||     |     |     |     |     |     |     |
**	      ||     |     |     |ABSDQ|     |ABSDQ|ABMDE*
**	      ||     |     |     |ABMDE*     |ABMDE*     |
**	      ||     |     |     | (20)|     | (21)| (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBABT ||SBCLD|SBCLG|SBCLG|SBCLG|SBCLG|SBCLG|SBCLD|
**	(13)  ||     |OBNDR|OBNDR|OBNDR|OBNDR|OBNDR|     |
**	      ||     |ABDCN*ABDCN*ABDCN*ABDCN*ABDCN*ABCCB|
**            ||ABMDE*     |     |     |     |     |ABMDE*
**	      || (8) | (10)| (10)| (10)| (10)| (10)| (3) |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBXOF ||     |     |SBOPN|SBSDP|SBOPX|SBSDX|     |
**	(14)  ||     |     |     |     |     |     |     |
**	      ||     |     |ABSXF|ABSXF|ABSXF|ABSXF|     |
**	      ||     |     |ABMDE*ABMDE*ABMDE*ABMDE*     |
**	      ||     |     | (22)| (23)| (24)| (25)|     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBXON ||     |     |SBOPN|SBSDP|SBOPN|SBSDP|     |
**	(15)  ||     |     |     |     |     |     |     |
**	      ||     |     |ABRXF|ABRXF|ABRXF|ABRXF|     |
**	      ||     |     |     |     |ABRVN|ABRVN|     |
**	      ||     |     |ABMDE*ABMDE*ABMDE*ABMDE*     |
**	      ||     |     | (26)| (27)| (28)| (29)|     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**     	IBRDX ||     |     |SBOPX|SBSDX|     |     |     |
**	(16)  ||     |     |OBSDN*OBSDN*     |     |     |
**	      ||     |     | (30)| (31)|     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBSDNQ||     |SBWNC|     |SBSDP|     |SBSDX|     |
**	(17)  ||     |OBXOF|     |OBXOF|     |OBXOF|     |
**	      ||     |ABSNQ*     |ABSNQ*     |ABSNQ*     |
**	      ||     | (37)|     | (32)|     | (33)|     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBDNX ||     |     |     |SBOPN|     |SBOPX|SBCLG|
**	(18)  ||     |     |     |OBXON|     |OBXON|     |
**	      ||     |     |     |ABMDE*     |ABMDE*ABMDE*
**	      ||     |     |     | (34)|     | (35)| (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNCCQ||     |SBSDP|     |     |     |     |     |
**	(19)  ||     |     |     |     |     |     |     |
**	      ||     |ABRVN|     |     |     |     |     |
**	      ||     |ABSDQ|     |     |     |     |     |
**	      ||     |ABMDE|     |     |     |     |     |
**	      ||     | (38)|     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNLSF||SBCLG|     |     |     |     |     |     |
**	(20)  ||     |     |     |     |     |     |     |
**	      ||ABDCN*     |     |     |     |     |     |
**	      || (9) |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+
**	IBNCIP||SBOPN|     |     |     |     |     |     |
**	(21)  ||OBNCQ*     |     |     |     |     |     |
**	      ||ABRVN|     |     |     |     |     |     |
**  	      ||ABLSN|     |     |     |     |     |     |
**	      || (39)|     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+

**
** Inputs:
**      mde			Pointer to Message Data Element
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Apr-90 (cmorris)
**          Initial routine creation.
*/
STATUS
gcc_pb
(
    GCC_MDE 		*mde, 
    STATUS		*generic_status,
    CL_ERR_DESC		*system_status 
)
{
    u_i4	input_event;
    STATUS	status;
    
    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcc_pb_evinp();
    */

    status      = OK;
    input_event = gcc_pb_evinp( mde, &status );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_pb: primitive %s %s event %s size %d\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
		gcx_look(gcx_mde_primitive, mde->primitive_type ),
		gcx_look( gcx_gcc_ipb, input_event ),
		mde->p2 - mde->p1 );
# endif /* GCXDEBUG */

    if( status == OK )
    {
	i4		output_event;
	u_i4           action_code;
	i4		i;
	i4		j;
	GCC_CCB		*ccb = mde->ccb;
	u_i4		old_state;

	/*
	** Determine the state transition for this state and input event.
	*/

	old_state = ccb->ccb_bce.b_state;
	j 	  = pbfsm_tbl[ input_event ][ old_state ];

	/* Make sure we have a legal transition */
	if( j != 0 )
	{
	    ccb->ccb_bce.b_state = pbfsm_entry[ j ].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcc_pbfsm>", mde, "event = %d, state = %d->%d",
			     input_event, old_state, ccb->ccb_bce.b_state);

# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1 && ccb->ccb_bce.b_state != old_state )
		TRdisplay("gcc_pbfsm: new state = %s->%s\n", 
			    gcx_look( gcx_gcc_spb, old_state ), 
			    gcx_look( gcx_gcc_spb, ccb->ccb_bce.b_state ) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event,
	    ** call the output executor.
	    */

	    for( i = 0;
		 i < PB_OUT_MAX && ( output_event =
		 pbfsm_entry[ j ].output[ i ] ) > 0;
		 i++ )
	    {
		if( ( status = gcc_pbout_exec( output_event,
					       mde, ccb ) ) != OK )
		{
		    /*
		    ** Execution of the output event has failed. Aborting the 
		    ** connection will already have been handled so we just get
		    ** out of here.
		    */
		    return( status );
		} /* end if */
	    } /* end for */

	    /*
	    ** For each action in the list for this state and input event,
	    ** call the action executor.
	    */

	    for( i = 0;
		 i < PB_ACTN_MAX && ( action_code =
		    pbfsm_entry[ j ].action[ i ] ) != 0;
		 i++ )
	    {
		if( ( status = gcc_pbactn_exec( action_code, mde,
						ccb ) ) != OK )
		{
		    /*
		    ** Execution of the action has failed. Aborting the 
		    ** connection will already have been handled so we just get
		    ** out of here.
		    */
		    return( status );

		} /* end if */

	    } /* end for */

	} /* end if */
	else
	{
	    GCC_ERLIST		erlist;

	    /*
	    ** This is an illegal transition.  This is a program bug or a
	    ** protocol violation.
	    ** Log the error.  The message contains 2 parms: the input event and
	    ** the current state.
	    */

	    erlist.gcc_parm_cnt 	= 2;
	    erlist.gcc_erparm[0].size   = sizeof( input_event );
	    erlist.gcc_erparm[0].value  = (PTR)&input_event;
	    erlist.gcc_erparm[1].size   = sizeof( old_state );
	    erlist.gcc_erparm[1].value  = (PTR)&old_state;
	    status 			= E_GC2A06_PB_FSM_STATE;

	    /* Abort the connection */

	    gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL, &erlist );
	    status = FAIL;
	}
    }
    else
    {
	/*
	** Evaluation of the input event has failed.  Log the status
	** returned by gcc_pb_evinp, and abort FSM execution.
	*/

	gcc_pb_abort( mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
			(GCC_ERLIST *) NULL );
	status = FAIL;
    }

    return( status );

} /* end gcc_pb */


/*
** Name: gcc_pbinit	- Protocol Bridge initialization routine.
**
** Description:
**      gcc_pbinit is invoked on bridge startup.  It initializes
**      the Protocol Bridge and returns. The following functions are performed: 
**
**	- Invoke GCpinit() to initialize the protocol CL and construct the PCT;
**	
**      - Establish an outstanding GCC_LISTEN on each local network port.
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Apr-90 (cmorris)
**          Initial routine creation
**  	12-Dec-91 (cmorris)
**  	    Only open the two protocols we need
**  	01-Apr-92 (cmorris)
**  	    Dynamically allocate mapping table
**	05-Dec-95 (rajus01)
**	    Added support for reading data from config.dat. 
**	02-may-2000 (somsa01)
**	    Corrected allocated size of pm_vnode.
**	06-jun-2001 (somsa01)
**	    After a successful startup, write "PASS\n" to standard output.
**	    This is to signal the parent that started us that we have no
**	    more information to write out.
**      21-Aug-2001 (wansh01) 
**         for both command line and config file init, call
**         gcc_pbconfig() to validate the protocol status and make sure
**         a vnode(port) is defined for the protocol specified.  
**      04-Apr-2002 (wansh01) 
**         only vnode/port with status 'ON' is opened 
*/

STATUS
gcc_pbinit
(
    STATUS		*generic_status,
    CL_ERR_DESC		*system_status 
)
{
    GCC_CCB		*ccb;
    GCC_PCT		*pct;
    GCC_MDE		*mde; 
    GCC_ERLIST		erlist;
    STATUS		temp_status;
    STATUS		call_status;
    STATUS		status = OK;
    u_i4		i, k, t;
    u_i4               opened = 0;

    *generic_status = OK;

    IIGCc_global->gcc_pb_mt = (GCC_BMT *) MEreqmem( (u_i2)0, 
	                      (u_i4) sizeof( GCC_BMT ),
			      (bool) TRUE, &status );


    if( status != OK )
    {
        *generic_status = status;
        return( FAIL );
    } /* end if */

    /*
    ** Invoke GCpinit() to construct the PCT, then save the PCT.
    */

    if( ( status = GCpinit( &pct, generic_status, system_status ) ) != OK )
	return( FAIL );

    IIGCc_global->gcc_pct = pct;

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_pbinit: pct_n_entries %d\n", pct->pct_n_entries );
# endif /* GCXDEBUG */

    /* Allocate lookup tables, v_node for mapping connections */

    for( t = 0; t < pct->pct_n_entries; t++ )
    {
	IIGCc_global->gcc_pb_mt->bme[ t ] =
			(GCC_BME *) MEreqmem( (u_i2)0,
			(u_i4)( sizeof(GCC_BME) * IIGCc_global->gcc_ib_max ), 
			(bool) TRUE, &status );
        if( status != OK )
        {
	   *generic_status = status;
	   return( FAIL );
        } /* end if */

	gcb_pm_reso.pm_vnode[ t ] = (char *) MEreqmem( (u_i2)0,
				GCC_L_NODE, (bool) TRUE, &status ); 
        if (status != OK)
        {
	   *generic_status = status;
	   return(FAIL);
        } /* end if */

    } /* end for */
     
    /*
    ** Place an OPEN on the source and destination protocol drivers. Invoke 
    ** the PB FSM to do it.  If either attempt fails, return a failed status 
    ** to indicate initialization failure.  If both are successfully requested
    ** return OK.
    **
    ** Allocate a CCB and an MDE for each protocol open. The MDE is set up as
    ** a N_OPEN request, a non-OSI specified SDU.
    */
    

    for( i = 0; i < pct->pct_n_entries; i++ )
    {
	GCC_PCE	*pce = &pct->pct_pce[ i ];

        if( gcc_pbconfig( pce ) == FALSE )
        {
	/* We won't be using this protocol so just disable it */
   	pce->pce_flags = PCT_NO_OUT | PCT_NO_PORT | PCT_OPN_FAIL;
	continue;
        }

    	CVupper( pce->pce_pid );
	 
    	if( ! STcompare( pce->pce_pid, gcb_from_addr.n_sel ) )
	    pce->pce_flags = PCT_NO_OUT;
	else
	  if( ! STcompare( pce->pce_pid, gcb_to_addr.n_sel ) )
	    pce->pce_flags = PCT_NO_PORT;
  	  else
          if (! gcb_pm_reso.cmd_line) 
	  {
	    for( k = 0; k <  gcb_pm_reso.pmrec_count; k++ )
	    {
      	        if( PMnumElem( gcb_pm_reso.pmrec_name[ k ] ) == 7 ) 
		{
	           if ( *( gcb_pm_reso.pm_vnode[ k ]) != EOS )
	           {
	             /*
	   	     ** Set the flags to PCT_NO_OUT for incoming protocol.
	 	     */
		      pce->pce_flags = PCT_NO_OUT;
		   }
		}
		else
	        /*
	         ** Set the flags to PCT_NO_PORT for outgoing protocol.
      	         */
		    pce->pce_flags = PCT_NO_PORT;

	    }
 	   }
	   else
	   { 
	    /* We won't be using this protocol so just disable it */
       	    pce->pce_flags = PCT_NO_OUT | PCT_NO_PORT | PCT_OPN_FAIL;
            continue;
 	   }

	pce->pce_flags |= PCT_OPEN;
	mde = gcc_add_mde( ( GCC_CCB * ) NULL, 0 );
	ccb = gcc_add_ccb();
	if( ( mde != NULL ) & ( ccb != NULL ) )
	{
	    mde->service_primitive = N_OPEN;
	    mde->primitive_type = RQS;
	    mde->ccb = ccb;
	    ccb->ccb_bce.prot_pce = pce;
	    ccb->ccb_bce.b_state = SBCLD;
	    ccb->ccb_bce.table_index  = i;   
	    *generic_status = OK;
	    status = gcc_pb( mde, generic_status, system_status );
	}
	else
	{
	    temp_status = E_GC2004_ALLOCN_FAIL;
	    gcc_er_log( &temp_status,
		        (CL_ERR_DESC *)NULL,
		        (GCC_MDE *)NULL,
		        (GCC_ERLIST *) NULL );
    	    status = FAIL;
	}

        if( status == OK )
	{
	    pce->pce_flags |= PCT_OPEN;
    	    opened++;
	}
	else
	{
	    pce->pce_flags |= PCT_OPN_FAIL;
	}  /* end else */

    } /* end for */

    /* Make sure we requested open on one or more protocols */

    if( opened < 1 )
    {
    	*generic_status = E_GC2A08_NTWK_INIT_FAIL;
	status = FAIL;
    }
    else
	status = OK;

    /* Complete the Name Server initialization here-itself.
    ** Previously it was done for each new client connect 
    ** request which resulted in GCN initialization error when
    ** connection requests come in very close and there was
    ** no chance to call GCN_TERMINATE before calling GCN_INITIATE
    ** thus resulting in DUP_INIT error.
    */

    if( status == OK )
    { 
        status = IIGCn_call( GCN_INITIATE, NULL ); 
        if( status != OK )
        {
            temp_status = E_GC2A0B_PB_GCN_INITIATE;
            gcc_er_log( &temp_status,
       		(CL_ERR_DESC *)NULL,
       		(GCC_MDE *)NULL,
       		(GCC_ERLIST *) NULL );

	    status = FAIL;
        }
    }

    if (!stdout_closed)
    {
	if (status == OK)
	    SIstd_write(SI_STD_OUT, "PASS\n");
	else
	    SIstd_write(SI_STD_OUT, "FAIL\n");

	stdout_closed = TRUE;
    }

    /*
    ** Initialization is complete.  Return to caller.
    */

    return( status );

}  /* end gcc_pbinit */


/*
** Name: gcc_pbterm	- Protocol Bridge termination routine
**
** Description:
**      gcc_pbterm is invoked on bridge shutdown.  It currently
**	performs only the function of calling GCpterm().
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      04-Oct-89 (cmorris)
**          Make function of type STATUS.
**  	10-Apr-92 (cmorris)
**  	    Free up mapping table
**  	05-Dec-95 (rajus01)
**  	    Free up mapping elements and vnodes.
*/
STATUS
gcc_pbterm
(
    STATUS		*generic_status,
    CL_ERR_DESC		*system_status
)
{
    STATUS	    status;
    i4		    t;

    /* Free the mapping table */
    for( t = 0; t < IIGCc_global->gcc_pct->pct_n_entries ; t++ )
    {
	MEfree( (PTR)IIGCc_global->gcc_pb_mt->bme[ t ] );
	MEfree( (PTR)gcb_pm_reso.pm_vnode[ t ] );
    }

    MEfree( (PTR)IIGCc_global->gcc_pb_mt );

    /* Terminate protocol drivers */
    status = GCpterm( IIGCc_global->gcc_pct,
	  	      generic_status,
		      system_status );
   
    /* Disconnect from Name Server */

    status = IIGCn_call( GCN_TERMINATE, NULL );

    if( status != OK )
    {
	STATUS temp_status;

	temp_status = E_GC2A0E_PB_GCN_TERMINATE;
	gcc_er_log( &temp_status,
       		(CL_ERR_DESC *)NULL,
       		(GCC_MDE *)NULL,
       		(GCC_ERLIST *) NULL );
        status = FAIL;
     }

    return( status );

}  /* end gcc_pbterm */


/*
** Name: gcc_pbprot_exit	- Asynchronous protocol service exit
**
** Description:
**	gcc_pbprot_exit is the exit handler invoked by all protocol handler
**	routines, upon completion of asynchronous protocol service requests.
**	It was specified as the completion exit by the original invoker of the
**	protocol handler service request.  It is passed a pointer to the MDE
**	associated with the service request (For details of the protocol
**	handler interface, see the CL specification for GCC).  It invokes
**	gcc_pb, the main protocol bridge entry point, with the MDE.
**	
**	For certain protocol handler service function completions, some special
**	processing is required.
**
** Inputs:
**      mde				The identifier specified at the time
**                                      of the service request.  It is a
**                                      pointer to the MDE for the service 
**					request.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	None
**
** History:
**  	04-Apr-91 (cmorris)
**  	    Culled from gcc_prot_exit
**  	05-Dec-95 (rajus01)
**	    Made changes in GCC_OPEN, GCC_LISTEN cases
**	    for additional functionality.
**  	18-Apr-96 (rajus01)
**	    Added server statistics to GCC_RECEIVE, GCC_SEND cases. 
**	09-jun-1999 (somsa01)
**	    On successful startup, write E_GC2A10_LOAD_CONFIG to the errlog.
**	15-jun-1999 (somsa01)
**	    In gcc_pbprot_exit(), moved printout of E_GC2A10_LOAD_CONFIG to
**	    iigcb.c .
**	08-mar-2001 (somsa01)
**	    When printing out E_GC2815_NTWK_OPEN, let's print out the
**	    normal port id as well as the numeric port id.
**	06-jun-2001 (somsa01)
**	    Write out the successful port ids to standard output.
**	 6-Jun-03 (gordy)
**	    Remote target address info now stored in trg_addr.
**	16-Jan-04 (gordy)
**	    MDE's may now be 32K.
**	04-Aug-09 (rajus01) SD issue 138408, Bug 122402.
**	    On 64bit platforms, the bridge server takes SEGV when the memory
**	    for node_id is freed. Memory was freed here as CL in 2.6 allocated
**	    memory for node_id for each connect and never got freed which lead
**	    slow memory leak problem in GCC and GCB servers. The CL is changed
**	    and it no longer allocates the memory. So no need to free the 
**	    memory here. Bridge is the only GCx server in Ingres 9.x.x that 
**	    still tries to free this memory.
**      02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**          When starting more than one GCC server the symbolic ports are not
**          incremented (Example: II1, II2) in the startup messages, while the
**          actual portnumber is increasing and printed correctly.
**          Log the actual_port_id for a successful network open request.
**	
*/
static VOID
gcc_pbprot_exit( GCC_MDE   *mde )
{
    GCC_P_PLIST		*p_list;
    STATUS		status;
    STATUS		generic_status;
    CL_ERR_DESC		system_status;
    GCC_CCB		*ccb;
    u_i4	    	i;

    generic_status = OK;
    p_list = &mde->mde_act_parms.p_plist;
    ccb = mde->ccb;
    
    /*
    ** Perform the processing appropriate to the completed service request.
    **
    ** It is here that errors resulting from CL level operations are discovered
    ** and logged.  Status from the operation is checked in the parm list for
    ** successful completion.  If OK, deal with the specific situation if any
    ** special processing is required.  If not OK, the error is logged out.
    ** Usually there is mainline level information to be logged out, to provide
    ** context for the CL level information.  Then modify the primitive
    ** indicator in the MDE to indicate an N_DISCONNECT indication (unless this
    ** is already an N_DISCONNECT) or an N_RESET indication, depending on the
    ** particular circumstances.
    **
    ** Note: This is the point at which network status and management
    ** indications might be returned.  This is a future enhancement to deal with
    ** these.
    */

    /* do tracing... */
    GCX_TRACER_2_MACRO("gcc_pbprot_exit>", mde,
		       "primitive = %d, cei = %d",
		       p_list->function_invoked,
		       ccb->ccb_hdr.lcl_cei);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- UP\n" );
	TRdisplay( "gcc_pbprot_exit: primitive %s cei %d\n",
		gcx_look( gcx_gcc_rq, p_list->function_invoked),
                ccb->ccb_hdr.lcl_cei ); 
    }
# endif /* GCXDEBUG */

    switch( p_list->function_invoked )
    {
    	case GCC_LISTEN:
    	{
	    if( p_list->generic_status != OK )
	    {
	    	GCC_ERLIST		erlist;
	    	GCC_PCE			*pce;

   	    	/* 
	    	** A GCC_LISTEN has failed.
	    	** Log the error.  The message contains 2 parms: the protocol
	    	** id and the listen port.
	    	*/

            	mde->service_primitive 	= N_LSNCLUP;
	    	mde->primitive_type    	= RQS;
            	pce 		   	= ccb->ccb_bce.prot_pce;
	    	erlist.gcc_parm_cnt    	= 2;
	    	erlist.gcc_erparm[0].value  = pce->pce_pid;
	    	erlist.gcc_erparm[0].size   = STlength(pce->pce_pid);
            	erlist.gcc_erparm[1].value  = pce->pce_port;
	    	erlist.gcc_erparm[1].size   = STlength(pce->pce_port);
	    	generic_status 		= E_GC2813_NTWK_LSN_FAIL;
	    	gcc_er_log( &generic_status, (CL_ERR_DESC *)NULL, mde, 
              	         &erlist );
	    	gcc_er_log( &p_list->generic_status,
			        &p_list->system_status,
			        mde,
			        (GCC_ERLIST *)NULL );
			
	    } /* end if */
	    else
	    {
	    	GCC_MDE            *lsn_mde;
	    	GCC_CCB	           *lsn_ccb;
	    	STATUS		   generic_status;
	    	CL_ERR_DESC	   system_status;
	    	i4 		   n 	   = ccb->ccb_bce.table_index;
	    	i4		   conns   = 0;
	    	i4		   i	   = 0;
	    	GCC_BME 	   *bme    = IIGCc_global->gcc_pb_mt->bme[ n ];

	    	if( ! ( IIGCc_global->gcc_flags & GCC_TRANSIENT ) )
	    	{
	    	    lsn_mde = gcc_add_mde( (GCC_CCB *) NULL, 0 );
	    	    lsn_ccb = gcc_add_ccb();
	    	    if( ( lsn_mde != NULL ) & ( lsn_ccb != NULL ) )
	    	    {
		    	lsn_mde->service_primitive = N_OPEN;
  	            	lsn_mde->primitive_type = CNF;
		    	lsn_mde->ccb = lsn_ccb;
	            	lsn_ccb->ccb_bce.prot_pce = ccb->ccb_bce.prot_pce;
		    	lsn_ccb->ccb_bce.b_state = SBCLD;
	            	lsn_ccb->ccb_bce.table_index = ccb->ccb_bce.table_index;
		    	lsn_ccb->ccb_bce.prot_pce->pce_flags = PCT_NO_OUT;
		    	generic_status = OK;
	            	gcc_pb( lsn_mde, &generic_status, &system_status );
	            }
	        }	

	    	conns = IIGCc_global->gcc_ib_max + n;
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "gcc_pbprot_exit: table_index  %d conns %d port %s\n",
		    ccb->ccb_bce.table_index, conns,
		    p_list->function_parms.listen.port_id );
    }
# endif /* GCXDEBUG */
    	    	/* Find slot in mapping table */
    	    	for( i = n; i < conns; i++ )
		    if( ! bme[ i ].in_use )
		    	break;
	    	if( i == conns )
	    	{

		    /* We can't support another connection */
		    mde->service_primitive = N_LSNCLUP;
		    mde->primitive_type = RQS;
		    generic_status = E_GC2A09_MAX_PB_CONNS;
		    gcc_er_log( &generic_status, (CL_ERR_DESC *)NULL,
			            (GCC_MDE *)NULL, (GCC_ERLIST *) NULL );

	        }
	        else
	        {
		    bme[ i ].in_use   = TRUE;
		    bme[ i ].from_ccb = ccb;
		    bme[ i ].to_ccb   = (GCC_CCB *) NULL;
	    	}

	        ccb->ccb_bce.pcb = p_list->pcb;
	        ccb->ccb_bce.block_size = TPDU_32K_SZ;

	        /* Increment connection counters */
	        IIGCc_global->gcc_ib_conn_ct++;
	        IIGCc_global->gcc_conn_ct++;
	        ccb->ccb_hdr.flags |= CCB_IB_CONN;

	        /* timestamp the ccb */
	        TMnow( &ccb->ccb_hdr.timestamp );
	        /* 
    	        ** We're running in transient mode, quiesce bridge 
    	        ** to ensure it shuts down 
    	        */
	        if( IIGCc_global->gcc_flags & GCC_TRANSIENT )
	    	    IIGCc_global->gcc_flags |= GCC_QUIESCE;

	        break;
	    } /* end of else */
        } /* end case GCC_LISTEN */
    	case GCC_CONNECT:
    	{
	    if( p_list->generic_status != OK )
	    {
	        GCC_ERLIST		erlist;
	        STATUS		temp_status;

  	        /*
	        ** This is the failure of a GCC_CONNECT.
	        ** Log the error.  The message contains 3 parms: the protocol
	        ** id, the node and the listen port.
	        */

	        erlist.gcc_parm_cnt = 3;
	        erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	        erlist.gcc_erparm[1].value = ccb->ccb_hdr.trg_addr.node_id;
	        erlist.gcc_erparm[2].value = ccb->ccb_hdr.trg_addr.port_id;
	        erlist.gcc_erparm[0].size = 
	            STlength(erlist.gcc_erparm[0].value);
	        erlist.gcc_erparm[1].size = 
	            STlength(erlist.gcc_erparm[1].value);
	        erlist.gcc_erparm[2].size = 
	            STlength(erlist.gcc_erparm[2].value);
	        temp_status = E_GC2809_NTWK_CONNECTION_FAIL;
	        gcc_er_log( &temp_status,
	    	           (CL_ERR_DESC *)NULL,
		           mde,
		           &erlist );
	        gcc_er_log( &p_list->generic_status,
		            &p_list->system_status,
		            mde,
		            (GCC_ERLIST *)NULL );
		if( gcb_pm_reso.cmd_line )
		   IIGCc_global->gcc_flags  |=  GCC_QUIESCE;
	        mde->service_primitive = N_DISCONNECT;
	        mde->primitive_type = IND;

	    } /* end if */

	    ccb->ccb_bce.pcb = p_list->pcb;
            break;
        } /* end case GCC_CONNECT */
        case GCC_DISCONNECT:
        {
	    /* Decrement connection counter. */
	    if( ccb->ccb_hdr.flags & CCB_IB_CONN )
	    {
	    	IIGCc_global->gcc_ib_conn_ct--;
	    	IIGCc_global->gcc_conn_ct--;
	    }
            break;
        } /* end case GCC_DISCONNECT */
    	case GCC_RECEIVE:
    	{
	    if( p_list->generic_status != OK )
	    {
	        mde->service_primitive = N_DISCONNECT;
	        mde->primitive_type = IND;
	    } /* end if */
	    else
	    {
	        /* 
	        ** A RECEIVE for normal flow data has completed.  Save the input
	        ** buffer length.
	        */

		static  gcc_data_in_bits = 0;
	 
		IIGCc_global->gcc_msgs_in++;
		IIGCc_global->gcc_data_in += ( gcc_data_in_bits +
					     p_list->buffer_lng ) /1024;
		gcc_data_in_bits = ( gcc_data_in_bits +
						p_list->buffer_lng ) %1024;
	        mde->p2 = mde->p1 + p_list->buffer_lng;

	    } /* end else */

            break;
        } /* end case GCC_RECEIVE */
        case GCC_SEND:
        {
	    if( p_list->generic_status != OK )
	    {
	        /* Log the error. */

	        gcc_er_log( &p_list->generic_status,
	    	            &p_list->system_status,
		            mde,
		            (GCC_ERLIST *)NULL );
	        mde->service_primitive = N_DISCONNECT;
	        mde->primitive_type = IND;
    	    } /* end if */
	    else
	    {
		/*
		** Keep statistics....... 
		*/
		static  gcc_data_out_bits = 0;
		     
		IIGCc_global->gcc_msgs_out++;
		IIGCc_global->gcc_data_out += ( gcc_data_out_bits +
						p_list->buffer_lng ) /1024;
		gcc_data_out_bits = ( gcc_data_out_bits +
		                                p_list->buffer_lng ) %1024;
	    }
            break;
        } /* end case GCC_SEND */
        case GCC_OPEN:
        {
	    GCC_PCE		*pce = ccb->ccb_bce.prot_pce;
	    GCC_ERLIST	        erlist;

	    if( p_list->generic_status == OK )
	    {
	        STATUS	temp_status;
		char	port[ GCC_L_PORT * 2 + 5 ];
		char	buffer[512];

	        /*
	        ** The attempted network open succeeded.  Log it. 
	        */
		if ( ! STcompare( pce->pce_port,
				  p_list->function_parms.open.lsn_port ) )
		    STcopy( pce->pce_port, port );
                else if( p_list->function_parms.open.actual_port_id != NULL )
		    STprintf( port, "%s (%s)", 
			p_list->function_parms.open.actual_port_id,
			p_list->function_parms.open.lsn_port );
		else
		    STprintf( port, "%s (%s)", 
			p_list->function_parms.listen.port_id,
			p_list->function_parms.open.lsn_port );

	        erlist.gcc_parm_cnt = 2;
		CVupper( pce->pce_pid );
	        erlist.gcc_erparm[0].value = pce->pce_pid;
	        erlist.gcc_erparm[1].value = port;
	        erlist.gcc_erparm[0].size =
				 STlength(erlist.gcc_erparm[0].value);
	        erlist.gcc_erparm[1].size =
				  STlength(erlist.gcc_erparm[1].value);

	        temp_status = E_GC2815_NTWK_OPEN;
	        gcc_er_log( &temp_status,
		           (CL_ERR_DESC *)NULL,
		           (GCC_MDE *)NULL,
		           &erlist );

		CVupper(pce->pce_pid);
		if (!stdout_closed)
		{
		    STprintf(buffer, "    %s port = %s\n", pce->pce_pid, port);
		    SIstd_write(SI_STD_OUT, buffer);
		}

		CVlower ( pce->pce_pid );
	    }
	    else
	    {
	        i4  	opened;
	        i4		i;

	        /*
	        ** Log the error. The message contains 2 parms: the protocol
	        ** id and the listen port.
	        */

	        pce->pce_flags |= PCT_OPN_FAIL;
	        erlist.gcc_parm_cnt = 2;
		CVupper( pce->pce_pid );
	        erlist.gcc_erparm[0].value = pce->pce_pid;
	        erlist.gcc_erparm[1].value = 
				p_list->function_parms.listen.port_id;
	        erlist.gcc_erparm[0].size  =
		  STlength(erlist.gcc_erparm[0].value);
	    	erlist.gcc_erparm[1].size  = 
		      STlength(erlist.gcc_erparm[1].value);
	    	generic_status = E_GC2808_NTWK_OPEN_FAIL;
	        gcc_er_log( &generic_status,
		           (CL_ERR_DESC *)NULL,
		           (GCC_MDE *)NULL,
		           &erlist );
	    	gcc_er_log( &p_list->generic_status,
		            &p_list->system_status,
		            (GCC_MDE *)NULL,
		            (GCC_ERLIST *)NULL );
		CVlower( pce->pce_pid );
	        mde->service_primitive = N_RESET;
	    	mde->primitive_type = IND;

    	    	/*
	        ** See if all attempted protocol opens have failed.  If a
	        ** protocol open has failed, PCT_OPN_FAIL will be set.  If
	        ** the open is not yet complete or did succeed, we count
	        ** it with "opened." Since the completion of protocol
	        ** opens is asynchronous, we check at each open completion
	        ** since we don't know which will be the last one.
	        */

	    	for( i = opened = 0;
		    i < IIGCc_global->gcc_pct->pct_n_entries;
		    i++ )
	        {
		    pce = &IIGCc_global->gcc_pct->pct_pce[i];

		    if( ! ( pce->pce_flags & PCT_OPN_FAIL ) )
		        opened++;
	        }

	    	/*
    	        ** As the bridge needs opens on atleast 2  protocols,
    	        ** we can't carry on if minimum 2 protocols not opened.
	        ** Log an error and terminate Protocol Bridge execution.
	        */

                if( opened < 1 )
                {
	            generic_status = E_GC2A08_NTWK_INIT_FAIL;
	            gcc_er_log( &generic_status,
	   	       	        (CL_ERR_DESC *)NULL,
		                (GCC_MDE *)NULL,
		                (GCC_ERLIST *)NULL );
	       	    GCshut();
	    	}
	    } /* end if */
	break;
        } /* end case GCC_OPEN */
	default:
	{
	    /*
	    ** Unrecognized service primitive.  This is a program bug.  Log
	    ** the problem and abort the connection.
	    */

	    mde->service_primitive = N_ABORT;
	    mde->primitive_type = RQS;
	    generic_status = E_GC2A05_PB_INTERNAL_ERROR;
	    gcc_er_log( &generic_status,
		        (CL_ERR_DESC *) NULL,
		        mde,
		        (GCC_ERLIST *) NULL );
	    break;
	} /* end default */
    }/* end switch */

    (VOID) gcc_pb( mde, &generic_status, &system_status );
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- UP-END\n" );
    }
# endif /* GCXDEBUG */
    return;
} /* end gcc_pbprot_exit */


/*
** Name: gcc_pb_evinp	- PB FSM input event id evaluation routine
**
** Description:
**      gcc_pb_evinp accepts as input an MDE received by the PB.  It  
**      evaluates the input event based on the MDE type and possibly on  
**      some other status information.  It returns as its function value the  
**      ID of the input event to be used by the FSM engine.
**
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
**	system_status		System_specific status code.
**
**	Returns:
**	    u_i4	input_event_id
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      30-Apr-90 (cmorris)
**          Initial function implementation
**      19-Jan-93 (cmorris)
**  	    Got rid of system_status argument:- never used.
*/
static u_i4
gcc_pb_evinp
(
    GCC_MDE 		*mde,
    STATUS 		*generic_status 
)
{
    u_i4               input_event = IBMAX;
    i4			i;
    PTR			pci;

    /*
    ** The input event is determined from the contents of the MDE.  It may be
    ** an SDU from a peer network connection, or an SDU from Network Layer
    ** below.
    */

    GCX_TRACER_3_MACRO("gccpb_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    mde->p2 - mde->p1);
	    
    switch( mde->service_primitive )
    {
	case N_CONNECT:
	{
	    /*
	    ** Both IND and CNF represent nascent connections.  The former is a
	    ** completed GCC_LISTEN and the latter is a completed GCC_CONNECT.
	    ** The RQS is a request from a peer network connection.
	    */
	
	    switch( mde->primitive_type )
	    {
		case RQS:
		{
		    input_event = IBNCQ;
	    	    break;
		}
		case IND:
		{
    	    	    /* Are we in permanent or transient mode? */
    	    	    if (!(IIGCc_global->gcc_flags & GCC_TRANSIENT))
	    		input_event = IBNCI;
    	    	    else
    	    		input_event = IBNCIP;
	    	    break;
		}
		case CNF:
		{
	    	    /* Is the send queue empty? */

	    	    if( mde->ccb->ccb_bce.b_snd_q.q_next ==
					&mde->ccb->ccb_bce.b_snd_q)
	    	    {
			input_event = IBNCC;
	    	    }
	    	    else
	    	    {
	        	input_event = IBNCCQ;
	    	    }
	    	    break;
		}
		default:
	    	{
		    *generic_status = E_GC2A07_PB_FSM_INP;
		    break;
	    	}
	    } /* end switch */
	    break;
        } /* end case N_CONNECT */
	case N_DISCONNECT:
    	{
	    switch( mde->primitive_type )
	    {
		case RQS:
		{
	    	    input_event = IBNDR;
	    	    break;
		}
		case IND:
		{
	    	    input_event = IBNDI;
	    	    break;
		}
		case CNF:
		{
	    	    input_event = IBNDC;
	    	    break;
		}
		default:
	    	{
		    *generic_status = E_GC2A07_PB_FSM_INP;
		    break;
	    	}
	    } /* end switch */
	    break;
        } /* end case N_DISCONNECT */
    	case N_DATA:
    	{
	    switch( mde->primitive_type )
	    {
		case RQS:
		{
	
            	    /* This is a request from the peer connection */

    	    	    /* See if the send queue is full and we haven't sent XOFF */
	
	    	    if( ( mde->ccb->ccb_bce.b_sq_cnt >= GCCPB_SQMAX ) &&
	    		! ( mde->ccb->ccb_bce.b_flags & GCCPB_SXOF) )
	    	    {
	    		input_event = IBSDNQ;
	    	    }
	    	    else
	    	    {
	    		input_event = IBSDN;
	    	    }

	    	    break;
		}	    
		case IND:
		{
	    	    /*
	    	    ** This is a completed GCC_RECEIVE.
	    	    */


	    	    /* Check for previously received XOFF */
	    
	    	    if( mde->ccb->ccb_bce.b_flags & GCCPB_RXOF )
			input_event = IBRDX;
	    	    else
			input_event = IBDNI;
	    	    break;
		} /* end case IND */
		case CNF:
		{
	    	    /* Is the send queue empty? */

	    	    if( mde->ccb->ccb_bce.b_snd_q.q_next ==
					&mde->ccb->ccb_bce.b_snd_q )
	    	    {
			/* Have we sent an XOFF? */

			if( mde->ccb->ccb_bce.b_flags & GCCPB_SXOF )
			{
		    	    input_event = IBDNX;
			}
			else
			{
		    	    input_event = IBDNC;
			}
	    	    }
	    	    else
	    	    {
			input_event = IBDNQ;
	    	    }
	    	    break;
		}
		default:
	    	{
		    *generic_status = E_GC2A07_PB_FSM_INP;
		    break;
	        }
	    } /* end switch */
	    break;
    	} /* end case N_DATA */
    	case N_OPEN:
    	{
	    switch( mde->primitive_type )
	    {
		case RQS:
		{
	    	    input_event = IBOPN;
	    	    break;
		}
		case CNF:
		{
	    	    input_event = IBNOC;
	    	    break;
		}
		default:
	    	{
		    *generic_status = E_GC2A07_PB_FSM_INP;
		    break;
	    	}
	    } /* end switch */
	    break;
        } /* end case N_OPEN */
    	case N_RESET:
    	{
	    input_event = IBNRS;
	    break;
    	} /* end case N_RESET */
    	case N_ABORT:
    	{
	    /* treat this just like a PB abort */
	    input_event = IBABT;
	    break;
	} /* end case N_ABORT */
    	case N_XOFF:
    	{
	    /* N_XOFF has only a request form */
	
	    input_event = IBXOF;
	    break;
    	} /* end case N_XOFF */
    	case N_XON:
    	{
	    /* N_XON has only a request form */
	
	    input_event = IBXON;
	    break;
    	} /* end case N_XON */
    	case N_P_ABORT:
    	{
	    /* N_P_ABORT has only a request form */
	    input_event = IBABT;
	    break;
    	} /* end case N_P_ABORT */
    	case N_LSNCLUP:
    	{
	    /* N_LSNCLUP has only a request form */
	
	    input_event = IBNLSF;
	    break;
        } /* end case N_LSNCLUP */
    	default:
	{
	    *generic_status = E_GC2A07_PB_FSM_INP;
	    break;
	}
    } /* end switch */
    return( input_event );
} /* end gcc_pb_evinp */


/*
** Name: gcc_pbout_exec	- PB FSM output event execution routine
**
** Description:
**      gcc_pbout_exec accepts as input an output event identifier.
**	It executes this output event and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the output event.  Otherwise, a new one is obtained.
** 
**
** Inputs:
**	output_event_id		Identifier of the output event
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
**	system_status		System_specific status code.
**
**	Returns:
**	    OK
**          FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      01-May-90 (cmorris)
**          Initial function implementation
**  	19-Jan-93 (cmorris)
**  	    Got rid of layer id:- everything is passed to peer
**  	    Network Connection.
*/

static STATUS
gcc_pbout_exec
(
    i4  		output_event_id,
    GCC_MDE 		*mde,
    GCC_CCB 		*ccb
)
{
    STATUS	    status = OK;

    GCX_TRACER_1_MACRO("gcc_pbout_exec>", mde,
	    "output_event = %d", output_event_id);

    switch( output_event_id )
    {
    	case OBNCQ:
    	{
	    /*
	    ** Send N_CONNECT request to peer.
            */
	    mde->service_primitive = N_CONNECT;
	    mde->primitive_type = RQS;
	    break;
    	}  /* end case OBNCQ */
    	case OBNDR:
    	{
	    /*
	    ** Send a N_DISCONNECT request to peer.  
	    */

	    if( ! ( mde = gcc_add_mde( ccb, 0 ) ) )
	    {
	    	/* 
            	** Unable to allocate an MDE. Log an error and abort
	    	** the connection.
	    	*/

	    	status = E_GC2004_ALLOCN_FAIL;
	    	gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
				 (GCC_ERLIST *) NULL );
	    	return FAIL;
	    }

	    mde->ccb = ccb;
    	    mde->service_primitive = N_DISCONNECT;
    	    mde->primitive_type = RQS;
	    break;

	} /* end case OBNDR */
	case OBSDN:
    	{
	    /*
	    ** Send an N_DATA request to peer.
     	    */

	    mde->service_primitive = N_DATA;
	    mde->primitive_type = RQS;
	    break;
    	} /* end case OBSDN */
    	case OBXOF:
    	{
    	    GCC_MDE	    *xof_mde;

	    /*
	    ** Send an N_XOFF request to peer.  
	    ** Set the "Sent XOF" indicator in the GCC PB flags.
	    */

	    if( ! ( xof_mde = gcc_add_mde( ccb, 0 ) ) )
	    {
	    	/* 
            	** Unable to allocate an MDE. Log an error and abort
	    	** the connection.
	    	*/

	    	status = E_GC2004_ALLOCN_FAIL;
	    	gcc_pb_abort( xof_mde, ccb, &status, (CL_ERR_DESC *) NULL,
				 (GCC_ERLIST *) NULL );
	    	return FAIL;
	    }

	    xof_mde->ccb = ccb;
	    xof_mde->service_primitive = N_XOFF;
	    xof_mde->primitive_type = IND;
	    ccb->ccb_bce.b_flags |= GCCPB_SXOF;
    	    mde = xof_mde;
	    break;
	} /* end case OBXOF */
    	case OBXON:
    	{
    	    GCC_MDE	    *xon_mde;

	    /*
	    ** Send a N_XON request to peer.
	    ** Reset the "Sent XOF" indicator in the GCC PB flags.
	    */

	    if( ! ( xon_mde = gcc_add_mde( ccb, 0 ) ) )
	    {
	    	/* 
            	** Unable to allocate an MDE. Log an error and abort
	    	** the connection.
	    	*/

	    	status = E_GC2004_ALLOCN_FAIL;
	    	gcc_pb_abort(xon_mde, ccb, &status, (CL_ERR_DESC *) NULL,
	 		     (GCC_ERLIST *) NULL);
	    	return FAIL;
	    }

	    xon_mde->ccb = ccb;
	    xon_mde->service_primitive = N_XON;
	    xon_mde->primitive_type = RQS;
	    ccb->ccb_bce.b_flags &= ~GCCPB_SXOF;
    	    mde = xon_mde;
	    break;
        } /* end case OBXON */
        default:
	{
	    /*
	    ** Unknown event ID.  This is a program bug. Log an error
	    ** and abort the connection.
	    */

	    status = E_GC2A05_PB_INTERNAL_ERROR;
	    gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL );
	    return( FAIL );
	}
    } /* end switch */

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
        TRdisplay( "gcc_pbout_exec: service_primitive %s %s\n",
               gcx_look( gcx_mde_service, mde->service_primitive ),
               gcx_look( gcx_mde_primitive, mde->primitive_type ));
# endif /* GCXDEBUG */

    /* Map our connection into peer's and drive its protocol machine */
    status = gcc_pbmap( mde );

    return( status );

} /* end gcc_pbout_exec */


/*
** Name: gcc_pbactn_exec	- PB FSM action execution routine
**
** Description:
**      gcc_pbactn_exec accepts as input an action identifier.  It
**      executes the action correspoinding to the identifier and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the action.  Otherwise, a new one is obtained.
** 
**      All actions executed by this routine are invocations of services from 
**      the appropriate protocol handler.  The protocol handler is identified 
**      by the index into the PCT contained in the CCB for this connection. 
**
** Inputs:
**	action_code		Identifier of the action to be performed
**      mde			Pointer to dequeued Message Data Element
**	ccb			Pointer to the CCB.
**
**	Returns:
**	    STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      01-May-90 (cmorris)
**          Initial function implementation
**	05-Dec-95 (rajus01)
**	    Made changes in ABOPN, ABCNC, ABCCB, ABLSN case statements
**	    for enhancing functionality of the bridge.
**	09-Feb-96 (rajus01)
**	    Added more DEBUG statements.
** 	23-Feb-96 (rajus01)
**	    When we start bridge using cmd line args, the protcol id
**  	    should be in lower case. Added CVlower( n_sel ).   
**	    If the error is invalid protocol id, quiesce the bridge  
**	    by setting gcc_flags to GCC_QUIESCE..
** 	28-Feb-96 (rajus01)
**	    ABCCB: Don't delete CCB, if the bridge is permanant and if
**	    ccb's are in the mapping table. Deletion of ccb was causing
**	    SIGBUS error, when tracing was ON.
** 	01-Apr-96 (rajus01)
**	    Don't issue GCC_RECEIVE, when there is an error in the protocol
**	    name, or node address on the "to" side of Protocol Bridge.
**	    SunOS 4.1.3 select system call was failing, when we tried to read
**	    from fd which is already closed by GCC_DISCONNECT. But somehow,
**	    Solaris Poll call was ignoring this problem...
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for requests to Name Server network database.
**	 6-Jun-03 (gordy)
**	    Remote target address info now stored in trg_addr.
**	16-Jan-04 (gordy)
**	    MDE's may now be 32K.
**	08-Jul-2008 (rajus01) SD issue 129333, Bug # 120599
**	    Bridge server fails to listen on the given port ( which is
**	    different from the default port for the given protocol ) 
**	    using unique configuration names.
** 	18-Dec-2008 (rajus01) SD issue 132603, Bug # 121292
**	    Release the CCBs when the connections through the bridge server 
**	    goes away.
**	11-Aug-09 (gordy)
**	    Use a more appropriate size for username buffer.
*/

static STATUS
gcc_pbactn_exec
(
    u_i4 	action_code,
    GCC_MDE 	*mde,
    GCC_CCB 	*ccb
)
{
    GCC_P_PLIST		*p_list;
    STATUS		status = OK;
    GCC_PCE		*pce;
    u_i4		i;

    GCX_TRACER_1_MACRO("gcc_pbactn_exec>", mde,
	    "action = %d", action_code);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
        TRdisplay( "gcc_pbactn_exec: action %s\n",
                    gcx_look( gcx_gcc_apb, action_code ) );
# endif /* GCXDEBUG */
    switch( action_code )
    {
	case ABCNC:
    	{
	    i4		       	   n = ccb->ccb_bce.table_index;
	    char 		   *n_sel = ccb->ccb_hdr.trg_addr.n_sel;
	    GCN_CALL_PARMS  	   gcn_parm;
	    STATUS                 status, temp_status;
	    char                   *match = ERx( ",," );
	    char                   uid[ GC_USERNAME_MAX + 1 ];
	    char                   *puid = uid;

	    /*
	    ** Invoke the CONNECT function from the appropriate protocol 
	    ** handler.
	    */
	    if( gcb_pm_reso.cmd_line )
            { 
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
        TRdisplay(
	  "gcc_pbactn_exec: cmd_line args: n_sel %s port_id %s node_id %s \n",
	    ccb->ccb_hdr.trg_addr.n_sel, ccb->ccb_hdr.trg_addr.port_id,
	    ccb->ccb_hdr.trg_addr.node_id );
# endif /* GCXDEBUG */
	    	STcopy( gcb_to_addr.n_sel, ccb->ccb_hdr.trg_addr.n_sel );
            	STcopy( gcb_to_addr.port_id, ccb->ccb_hdr.trg_addr.port_id );
	    	STcopy( gcb_to_addr.node_id, ccb->ccb_hdr.trg_addr.node_id );
	 	CVlower( n_sel );
            }
	    else
	    {
	    	STcopy( NULLSTR, gcb_to_addr.n_sel );
            	STcopy( NULLSTR, gcb_to_addr.port_id );
	    	STcopy( NULLSTR, gcb_to_addr.node_id );

	        IDname( &puid );
	        STzapblank( uid, uid );
				   /* Remote node, protocol, address */
	        /*
	        ** Look for private vnode entries
	        */
	        gcn_parm.gcn_flag = GCN_NET_FLAG;
	        gcn_parm.gcn_response_func = gcn_info;  /* Callback */
	        gcn_parm.gcn_host = NULL;               /* Local Name Server */
	        gcn_parm.gcn_type = GCN_NODE;           /* Node info */
	        gcn_parm.gcn_uid = uid;                 /* Username */
	        gcn_parm.gcn_obj = gcb_pm_reso.pm_vnode[n];  /* VNODE */
	        gcn_parm.gcn_value = match;  /* node, protocol, address */
	        gcn_parm.gcn_install = SystemCfgPrefix;     /* Ignored */
	        gcn_parm.async_id = NULL;               /* Unused */
	        gcn_parm.gcn_buffer = NULL;
	        gcn_parm.gcn_status = OK;
		   
	        status = IIGCn_call( GCN_RET, &gcn_parm );
			    
	        if( status != OK )
		{
	            temp_status = E_GC2A0C_PB_GCN_RET_PRIV;
	            gcc_er_log( &temp_status,
		       		(CL_ERR_DESC *)NULL,
		       		(GCC_MDE *)NULL,
		       		(GCC_ERLIST *) NULL );
	    	    gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL );
		    return( FAIL );
		}
	        /*
	        ** Look for global vnode entries.
	        */
	        gcn_parm.gcn_flag |= GCN_PUB_FLAG;      /* Global entries */
		  
	        status = IIGCn_call( GCN_RET, &gcn_parm );
	        if( status != OK )
		{
	            temp_status = E_GC2A0D_PB_GCN_RET_PUB;
	            gcc_er_log( &temp_status,
		       		(CL_ERR_DESC *)NULL,
		       		(GCC_MDE *)NULL,
		       		(GCC_ERLIST *) NULL );
	    	    gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL );
		    return( FAIL );
		}
       
	    	STcopy( gcb_to_addr.n_sel, ccb->ccb_hdr.trg_addr.n_sel );
            	STcopy( gcb_to_addr.port_id, ccb->ccb_hdr.trg_addr.port_id );
	    	STcopy( gcb_to_addr.node_id, ccb->ccb_hdr.trg_addr.node_id );
	    }
	    /*
	    ** Now, find the entry in the PCT corresponding to the protocol
	    ** specified either on the command line or by default.  Then
            ** construct an N_CONNECT confirm in the MDE.
	    */

	    for( i = 0; i < IIGCc_global->gcc_pct->pct_n_entries; i++ )
	    {
	    	pce = &IIGCc_global->gcc_pct->pct_pce[i];
	    	if( ! STcompare( n_sel, pce->pce_pid ) )
		    break;
	    }

	    if( i == IIGCc_global->gcc_pct->pct_n_entries )
	    {
	    	GCC_ERLIST		erlist;
	    	STATUS			temp_status;

	    	/*
	    	** The specified protocol id was not found in the PCT. 
            	** Generate an N_RESET to indicate this. Log an error
	    	** containing 1 parm: the protocol id.
	    	*/
		/* If it is invalid protocol, just shut down the
		** bridge silently by setting gcc_flgs to GCC_QUIESCE.
		** This is valid only for command line arguments.
		*/
		if( gcb_pm_reso.cmd_line )
		   IIGCc_global->gcc_flags  |=  GCC_QUIESCE;
	    	erlist.gcc_parm_cnt = 1;
	    	erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    	erlist.gcc_erparm[0].size = 
		STlength(erlist.gcc_erparm[0].value);
	    	temp_status = E_GC2803_PROT_ID;
	    	gcc_er_log( &temp_status, (CL_ERR_DESC *)NULL, mde, &erlist );
 	   	mde->service_primitive = N_RESET;
           	mde->primitive_type = IND;
           	status = gcc_pb(mde, NULL, NULL ); 
		
	    } 
	    else if( pce->pce_flags & PCT_OPN_FAIL )
	    {
	    	GCC_ERLIST		erlist;
	    	STATUS		temp_status;
		
	    	/*
	    	** The user attempted an outgoing connection on a protocol
	    	** which failed to open.
            	** Generate an N_RESET to indicate this. Log an error
	    	** containing 1 parm: the protocol id.
	    	*/

	    	erlist.gcc_parm_cnt = 1;
	    	erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    	erlist.gcc_erparm[0].size = 
			STlength(erlist.gcc_erparm[0].value);
	    	temp_status = E_GC2816_NOT_OPEN;
	    	gcc_er_log( &temp_status, (CL_ERR_DESC *)NULL, mde, &erlist );
 	    	mde->service_primitive = N_RESET;
            	mde->primitive_type = IND;
            	status = gcc_pb( mde, NULL, NULL ); 
		
	    }
	    else
	    {
	    	/* We have a winner! Invoke CONNECT from the protocol handler */
	    	ccb->ccb_bce.prot_pce = pce;
    	    	ccb->ccb_bce.block_size = TPDU_32K_SZ;
	    	mde->service_primitive = N_CONNECT;
	    	mde->primitive_type = CNF;
	    	p_list = &mde->mde_act_parms.p_plist;
            	p_list->function_parms.connect.node_id =
		 	ccb->ccb_hdr.trg_addr.node_id;
	    	p_list->function_parms.connect.port_id =
			ccb->ccb_hdr.trg_addr.port_id;
	    	p_list->compl_exit = (void (*)())gcc_pbprot_exit;
	    	p_list->compl_id = (PTR)mde;
	    	p_list->pcb = NULL;
	    	p_list->pce = pce;
	    	p_list->buffer_ptr = mde->p1;
	    	p_list->buffer_lng = mde->p2 - mde->p1;
	    	p_list->options = 0;
	    	p_list->function_invoked = (i4) GCC_CONNECT;
	    	status = ( *pce->pce_protocol_rtne )( GCC_CONNECT, p_list );

	    	if( status != OK )
	    	{
	    	    GCC_ERLIST		erlist;

		
	    	    /*
	            ** Log the error.  The message contains 3 parms:
		    ** the protocol id, the node and the listen port.
	            */

	    	    erlist.gcc_parm_cnt = 3;
	    	    erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    	    erlist.gcc_erparm[1].value = ccb->ccb_hdr.trg_addr.node_id;
	    	    erlist.gcc_erparm[2].value = ccb->ccb_hdr.trg_addr.port_id;
	    	    erlist.gcc_erparm[0].size = 
	    	        STlength(erlist.gcc_erparm[0].value);
	    	    erlist.gcc_erparm[1].size = 
	    	        STlength(erlist.gcc_erparm[1].value);
	    	    erlist.gcc_erparm[2].size = 
	    	        STlength(erlist.gcc_erparm[2].value);
	    	    status = E_GC2809_NTWK_CONNECTION_FAIL;
	            gcc_er_log( &status, (CL_ERR_DESC *)NULL, mde, 
    	    		   &erlist );
	            /* Abort the connection */
	    	    gcc_pb_abort( mde, ccb, &p_list->generic_status, 
    	    	    	     &p_list->system_status, (GCC_ERLIST *) NULL );
	        } /* end if */

	    } /* end else */
	    break;
	} /* end case ABCNC */
        case ABDCN:
        {
	    /*
	    ** Invoke the DISCONNECT function from the appropriate protocol
	    ** handler.
	    */
	    pce = ccb->ccb_bce.prot_pce;
	    mde->service_primitive = N_DISCONNECT;
	    mde->primitive_type = CNF;
	    p_list = &mde->mde_act_parms.p_plist;
	    p_list->compl_exit = (void (*)())gcc_pbprot_exit;
	    p_list->compl_id = (PTR)mde;
	    p_list->pcb = ccb->ccb_bce.pcb;
	    p_list->pce = pce;
	    p_list->buffer_ptr = mde->p1;
	    p_list->buffer_lng = mde->p2 - mde->p1;
	    p_list->options = 0;
	    p_list->function_invoked = (i4) GCC_DISCONNECT;
	    status = ( *pce->pce_protocol_rtne )( GCC_DISCONNECT, p_list );
	    if( status != OK )
	    {
	        gcc_pb_abort( mde, ccb, &p_list->generic_status, 
    	    	    	 &p_list->system_status, (GCC_ERLIST *) NULL );	
	    }
	    break;
        } /* end case ABDCN */
    	case ABMDE:
        {
	    /*
	    ** Discard the MDE.
	    */
	    gcc_del_mde( mde );
	    break;
        } /* end case ABMDE */
	case ABCCB:	/* Return the CCB */
    	{
	    i4        	n     = ccb->ccb_bce.table_index;
	    GCC_BME   	*bme  = IIGCc_global->gcc_pb_mt->bme[n];
	    i4        	conns = n + IIGCc_global->gcc_ib_max;

	    /*
	    ** Discard the CCB pointed to by the input MDE.  Check the send
	    ** queue in the CCB.  If not empty, purge it and release the MDE's.
	    ** Decrement the use count in the CCB.  If it is 0, all users are
	    ** finished with it, and it can be released.  Otherwise, someone
	    ** still using it, and it will be deleted when all others are 
	    ** through with it.  When the CCB is actually deleted, the 
	    ** appropriate global connection counts are decremented.
	    */

	    while( ccb->ccb_bce.b_snd_q.q_next != &ccb->ccb_bce.b_snd_q )
	    {
	        gcc_del_mde((GCC_MDE *)QUremove( ccb->ccb_bce.b_snd_q.q_next));
	    }

	    if( --ccb->ccb_hdr.async_cnt <= 0 )
	    {
    	    	/* Find ccb in mapping table */
    	    	for( i = n; i < conns; i++ )
	    	{
    	    	    if( bme[i].in_use )
		    {
		    	if( bme[i].from_ccb == ccb )
		    	{
			    gcc_del_ccb( ccb );
			    bme[i].from_ccb = (GCC_CCB *) NULL;
			    from_flag = TRUE;
			    break;
		    	}

		    	if( bme[i].to_ccb == ccb )
		    	{
			    gcc_del_ccb( ccb );
			    bme[i].to_ccb = (GCC_CCB *) NULL;
			    to_flag = TRUE;
			    break;
		    	}

	            } /* end if */
                }

		if( ( IIGCc_global->gcc_flags & GCC_TRANSIENT ) || 
		   ( ( ! from_flag ) && ( ! to_flag ) ) )
		{
	        	gcc_del_ccb( ccb );
	        }
    	        /* 
    	        ** Making sure the connection was in the mapping table in the
    	        ** first place, check to see if both ccbs in the mapping
    	        ** have been deleted. If they have, mark the mapping table
    	        ** entry as being free.
    	        */
    	    	if( ( i != conns ) &&
    	    	    ( ! bme[i].from_ccb ) &&
                    ( ! bme[i].to_ccb )
		  )
		{
		     bme[i].in_use = FALSE;
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_pb: bme[ %d ] slot is set to FALSE\n", i );
# endif /* GCXDEBUG */

	        }

	    	/*
	    	** Check to see if this was the last connection: if so, 
    	    	** shut the bridge down.
	    	*/

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_pb: %d inbound connections, %d total connections\n",
		    IIGCc_global->gcc_ib_conn_ct,
		    IIGCc_global->gcc_conn_ct );
# endif /* GCXDEBUG */

	        if( !IIGCc_global->gcc_conn_ct && 
		     IIGCc_global->gcc_flags & GCC_QUIESCE )
                {
		    GCshut();
	        }
	    }
	    from_flag = FALSE;
	    to_flag   = FALSE;
	    break;
	} /* end case ABCCB */
    	case ABLSN:
    	{
	    /*
	    ** Issue LISTEN request to the protocol module.
	    ** Create a new CCB for the potential connection to be established
	    ** with the completion of GCC_LISTEN.  Create an MDE to contain
	    ** the A_ASSOCIATE request which will result.
    	    */

	    GCC_MDE		*lsn_mde;
	    GCC_CCB		*lsn_ccb;
	    GCC_PCE		*pce;
	    i4  		i;

    	    /* If we are in NO_PORT mode for this driver, do nothing */

    	    if( ccb->ccb_bce.prot_pce->pce_flags & PCT_NO_PORT )
    	        break;

	    lsn_ccb = gcc_add_ccb();
	    if( lsn_ccb == NULL )
	    {
	        /* log an error and abort the connection */
	        status = E_GC2004_ALLOCN_FAIL;
	        gcc_pb_abort( mde, lsn_ccb, &status, 
    	    	    	      (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL );
	        status = FAIL;
	        break;
	    } /* end if */
	    lsn_ccb->ccb_hdr.async_cnt +=1;

	    /* Copy listen info out of existing CCB */
	    pce = lsn_ccb->ccb_bce.prot_pce = ccb->ccb_bce.prot_pce;
	    lsn_ccb->ccb_bce.b_state = SBCLD;
	    lsn_ccb->ccb_bce.pcb = NULL; 
	    lsn_ccb->ccb_bce.table_index = ccb->ccb_bce.table_index;
	    lsn_mde = gcc_add_mde( lsn_ccb, 0 );
	    if( lsn_mde == NULL )
	    {
	        /* log an error and abort the connection */
	        status = E_GC2004_ALLOCN_FAIL;
	        gcc_pb_abort( lsn_mde, lsn_ccb, &status,
    	    	    	      (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL );
	        status = FAIL;
	        break;
	    } /* end if */
	    lsn_mde->ccb = lsn_ccb;
	    lsn_mde->service_primitive = N_CONNECT;
	    lsn_mde->primitive_type = IND;
	    lsn_mde->p1 = lsn_mde->p2 = lsn_mde->ptpdu;
	    p_list = &lsn_mde->mde_act_parms.p_plist;
	    p_list->compl_exit = (void (*)())gcc_pbprot_exit;
	    p_list->compl_id = (PTR)lsn_mde;
	    p_list->pcb = NULL;
	    p_list->pce = pce;
	    p_list->options = 0;
	    p_list->buffer_ptr = (PTR)lsn_mde->p1;
	    p_list->buffer_lng = lsn_mde->pend - lsn_mde->p1;
       	    p_list->function_invoked = (i4) GCC_LISTEN;
	    p_list->function_parms.listen.port_id = pce->pce_port;

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_pb: listen port_id %s\n",
		    p_list->function_parms.listen.port_id );
# endif /* GCXDEBUG */

	    status = ( *pce->pce_protocol_rtne )( GCC_LISTEN, p_list );

	    if( status != OK )
	    {
	        /* Log an error and abort the connection */

                GCC_ERLIST          erlist;

                /* Log an error and abort the connection */

                erlist.gcc_parm_cnt = 2;
                erlist.gcc_erparm[0].value = pce->pce_pid;
                erlist.gcc_erparm[0].size = STlength(pce->pce_pid);
                erlist.gcc_erparm[1].value = pce->pce_port;
                erlist.gcc_erparm[1].size = STlength(pce->pce_port);
                status = E_GC2813_NTWK_LSN_FAIL;
	        gcc_er_log( &status, (CL_ERR_DESC *)NULL, mde, &erlist );
	        gcc_pb_abort( mde, ccb, &p_list->generic_status, 
    	        	    	 &p_list->system_status, (GCC_ERLIST *) NULL );
	    }
	    break;
        } /* end case ABLSN */
        case ABRVN:
        {
            GCC_MDE	    *rvn_mde;
	    u_i4	    i;
	
	    /*
	    ** Invoke the RECEIVE function for normal data from the appropriate
	    ** protocol handler.The buffer begins at the start of the PB PCI
	    ** area. This will accommodate the largest PDU which could be sent. 
	    ** When receiving, a new MDE is obtained, since the old one will be 
	    ** used for other purposes.  The PCT index is in the current MDE, in
	    ** the srv_parms.
	    */

	    rvn_mde = gcc_add_mde( ccb, ccb->ccb_bce.block_size );

	    if( rvn_mde == NULL )
	    {
	        /* Unable to allocate an MDE */

	        status = E_GC2004_ALLOCN_FAIL;
	    
	        /* Log an error and abort the connection */
	        gcc_pb_abort( rvn_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			      (GCC_ERLIST *) NULL );
	        status = FAIL;
	        break;
	    }
	    pce = ccb->ccb_bce.prot_pce;
	    rvn_mde->ccb = ccb; /* use the arg instead of mde->ccb */
	    rvn_mde->service_primitive = N_DATA;
	    rvn_mde->primitive_type = IND;
	    rvn_mde->p1 = rvn_mde->p2 = rvn_mde->ptpdu;
	    p_list = &rvn_mde->mde_act_parms.p_plist;
	    p_list->options = 0;
	    p_list->compl_exit = (void (*)())gcc_pbprot_exit;
	    p_list->compl_id = (PTR)rvn_mde;
	    p_list->buffer_ptr = (PTR)rvn_mde->p1;
	    p_list->buffer_lng = rvn_mde->pend - rvn_mde->p1;
	    p_list->pcb = ccb->ccb_bce.pcb;
	    p_list->pce = pce;
	    p_list->function_invoked = (i4) GCC_RECEIVE;
	    if( ccb->ccb_bce.b_state != SBCLD )
	       status = ( *pce->pce_protocol_rtne )( GCC_RECEIVE, p_list );
	    else
		status = OK;
	    if( status != OK )
	    {
	        /* Log an error and abort the connection */
	        gcc_pb_abort( rvn_mde, ccb, &p_list->generic_status, 
    	    	    	 &p_list->system_status, (GCC_ERLIST *) NULL );
	    }
	    break;
        } /* end case ABRVN */
        case ABSDN:
        {
            /*
	    ** Invoke the SEND function for normal data from the appropriate
	    ** protocol handler: call gcc_psnd().
	    */

	    status = gcc_psnd( mde );
	    if (status != OK)
	    {
	        /* Log an error and abort the connection */
    	        p_list = &mde->mde_act_parms.p_plist;
	        gcc_pb_abort( mde, ccb, &p_list->generic_status, 
    	    	    	      &p_list->system_status, (GCC_ERLIST *) NULL );
	    }
	    break;
        } /* end case ABSDN */
        case ABOPN:
        {
            /*
	    ** Issue OPEN request to the protocol module.
	    */

    	    GCC_PCE		*pce;
	    i4  		i;

	    pce = ccb->ccb_bce.prot_pce;
	    mde->service_primitive = N_OPEN;
	    mde->primitive_type = CNF;
	    p_list = &mde->mde_act_parms.p_plist;
	    p_list->compl_exit = (void (*)())gcc_pbprot_exit;
	    p_list->compl_id = (PTR)mde;
	    p_list->pcb = NULL;
	    p_list->pce = pce;
	    p_list->buffer_ptr = NULL;
	    p_list->buffer_lng = 0;
	    p_list->options = 0;
            p_list->function_parms.open.actual_port_id = NULL;
	    p_list->function_invoked = (i4) GCC_OPEN;
	    if( gcc_pbconfig ( pce ) )
	    {
		    /*
		    ** The following check is needed to prevent the server
		    ** from core dumping, if there is only one definition.
		    ** ex., ii.beast.gcb.tcp_ip.port with no value..
		    */
    	    	    if( ( pce->pce_flags & PCT_NO_PORT ) ) 
		    {
	            	p_list->function_parms.open.port_id = pce->pce_port;
               	    	p_list->function_parms.open.lsn_port = pce->pce_port;
		    }
		    else
		    {
			/* Use the listen port for the given configuration 
			** name instead of the default(entries with "*" in 
			** config.dat) listen port.
			*/
			if( gcb_pm_reso.pmrec_count >= 2 )
                           i = gcb_pm_reso.pmrec_count-1;
                        p_list->function_parms.open.port_id =
					gcb_pm_reso.pmrec_value[0];
	                p_list->function_parms.open.lsn_port =
				        gcb_pm_reso.pmrec_value[0];
		    }

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_pb: open port_id %s open listen_port %s\n",
		    p_list->function_parms.open.port_id,
		    p_list->function_parms.open.lsn_port );
# endif /* GCXDEBUG */
	    /*
	    ** Invoke GCC_OPEN service.  
	    */

	    status = ( *pce->pce_protocol_rtne )( GCC_OPEN, p_list );
	    if( status != OK )
	    {
    	    	GCC_ERLIST	    erlist;

	    	/* Log an error and abort the connection */
	    	erlist.gcc_parm_cnt = 2;
	    	erlist.gcc_erparm[0].value = pce->pce_pid;
	    	erlist.gcc_erparm[1].value = pce->pce_port;
	    	erlist.gcc_erparm[0].size = 
		    STlength(erlist.gcc_erparm[0].value);
	    	erlist.gcc_erparm[1].size = 
	 	    STlength(erlist.gcc_erparm[1].value);
	        status = E_GC2808_NTWK_OPEN_FAIL;
	        gcc_er_log( &status, (CL_ERR_DESC *)NULL, 
    	    	            (GCC_MDE *)NULL, &erlist );
	        gcc_pb_abort( mde, ccb, &p_list->generic_status, 
    	    	    	      &p_list->system_status, (GCC_ERLIST *) NULL );
	    }
	    }
	    break;
        } /* end acase ABOPN */
    	case ABSNQ:
    	{
	    /*
	    ** Enqueue the MDE containing a normal send request.  A send is
	    ** currently in progess with completion pending.  This MDE will 
	    ** be sent when its turn comes.
	    */

	    QUinsert( (QUEUE *)mde, ccb->ccb_bce.b_snd_q.q_prev );
	    ccb->ccb_bce.b_sq_cnt +=1;
	    break;
        } /* end case ABSNQ */
        case ABSDQ:
        {
	    GCC_MDE		*sdq_mde;

	    /*
	    ** Dequeue and send an enqueued MDE.
	    */

	    sdq_mde = ( GCC_MDE * )QUremove( ccb->ccb_bce.b_snd_q.q_next );
	    ccb->ccb_bce.b_sq_cnt -=1;
	    sdq_mde->mde_q_link.q_next = NULL;
	    sdq_mde->mde_q_link.q_prev = NULL;

	    /*
	    ** Invoke the SEND function for normal data from the appropriate
	    ** protocol handler: call gcc_psnd().
    	    */

	    status = gcc_psnd( sdq_mde );
	    if( status != OK )
	    {
    	        p_list = &mde->mde_act_parms.p_plist;
	        /* Log an error and abort the connection */
	        gcc_pb_abort( sdq_mde, ccb, &p_list->generic_status, 
    	        	     &p_list->system_status, (GCC_ERLIST *) NULL );
	    }
	    break;
        } /* end case ABSDQ */
        case ABSXF:
        {
    	    /*
	    ** Set the "Received XOF" indicator in the GCC PB flags.
	    */

	    ccb->ccb_bce.b_flags |= GCCPB_RXOF;
	    break;
        } /* end case ABSXF */
        case ABRXF:
        {
	    /*
	    ** Reset the "Received XOF" indicator in the GCC PB flags.
	    */

	    ccb->ccb_bce.b_flags &= ~GCCPB_RXOF;
	    break;
        } /* end case ABRXF */
        default:
    	{
	    status = E_GC2A05_PB_INTERNAL_ERROR;

	    /* Log an error and abort the connection */
	    gcc_pb_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			  (GCC_ERLIST *) NULL );
	    status = FAIL;
	    break;
	}
    } /* end switch */
    return( status );
} /* end gcc_pbactn_exec */



/*
** Name: gcc_pbmap	- Map event onto peer connection
**
** Description:
**      Takes a connection and mde containing an event and maps
**  	the mde onto the connection's peer. Drives the peer connection's
**  	state machine. NOTE: This function is currently a shell that maps
**  	onto the sole other connection. Someday it will be better...
**
** Inputs:
**      mde                             Pointer to an MDE for the message
**
**	Returns:
**	    OK
**          FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-Mar-91 (cmorris)
**          Created
**	12-Jul-91 (cmorris)
**	    Only create ccb in from -> to direction
**	05-Dec-95 (rajus01)
**	    Made changes in mapping the connections.    
**	09-Feb-96 (rajus01)
**	    Added DEBUG statements.
*/
static STATUS
gcc_pbmap( GCC_MDE *mde )
{
    GCC_CCB 		*dest_ccb;
    STATUS      	status = OK;
    u_i4   		i;
    i4       	 	n     = mde->ccb->ccb_bce.table_index;
    GCC_BME  	 	*bme  = IIGCc_global->gcc_pb_mt->bme[n];
    i4	     	 	conns = n + IIGCc_global->gcc_ib_max;

# ifdef GCXDEBUG
    	if( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "gcc_pbmap: GCC_BME table index: %d conns: %d\n",
                        n, conns ); 
# endif /* GCXDEBUG */

    /* Find ccb in mapping table */

    for( i = n; i < conns; i++ )
    {
	if( bme[i].in_use )
	{ 
	    if( bme[i].from_ccb == mde->ccb )
	    {
    	    	dest_ccb = bme[i].to_ccb;

            	/* If destination doesn't exist, create it */
    	    	if( dest_ccb == (GCC_CCB *) NULL )
            	{
	    	    dest_ccb = bme[i].to_ccb = gcc_add_ccb();
            	    if( dest_ccb == (GCC_CCB *) NULL )
	    	    {
	            	/* log an error and abort the connection */
	            	status = E_GC2004_ALLOCN_FAIL;
	            	gcc_pb_abort( mde, dest_ccb, &status, 
    	    	    	    	      (CL_ERR_DESC *) NULL, 
    	    	    	    	      (GCC_ERLIST *) NULL );
    	    	    	status = FAIL;
	            	return( status );
	    	    } /* end if */

    	    	    dest_ccb->ccb_hdr.async_cnt +=1;
	            dest_ccb->ccb_bce.b_state = SBCLD; 
	            dest_ccb->ccb_bce.table_index = n;

            	    /* timestamp the ccb */
	    	    TMnow( &dest_ccb->ccb_hdr.timestamp );

		} /* end if */

    	    	break;
	    } /* end if from_ccb */

	    if( bme[i].to_ccb == mde->ccb )
	    {
    	    	dest_ccb = bme[i].from_ccb;
		break;
	    } /* end if to_ccb */

	} /* end if in_use */
    }
    
    /* 
    ** If we found the ccb in the table, do the mapping. If we didn't,
    ** just return.
    */
    if( i < conns )
    {
    	/* Link up destination ccb to mde */
    	mde->ccb = dest_ccb;

    	/* Now call into peer's state machine */
# ifdef GCXDEBUG
    	if( IIGCc_global->gcc_trace_level >= 1 )
    	{
	    TRdisplay( "----- BRIDGE\n" );
	    TRdisplay( "gcc_pbmap: cei %d\n",
                       mde->ccb->ccb_hdr.lcl_cei ); 
    	}
# endif /* GCXDEBUG */
    	status = gcc_pb( mde, NULL, NULL );
# ifdef GCXDEBUG
    	if( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "----- BRIDGE-END\n" );
# endif /* GCXDEBUG */
    }

    return( status );
}


/*
** Name: gcc_psnd	- SEND a message
**
** Description:
**      Construct the parameter list and invoke the appropriate protocol driver
**	to perform a GCC_SEND operation.  A mismatch between the protocol packed
**	size and the user data length must be accommodated by sending the
**	correct amount of data and setting the MDE up for a subsequent send of
**	the remaining data.
**
** Inputs:
**      mde                             Pointer to an MDE for the message
**
** Outputs:
**      none
**	Returns:
**	    OK
**          FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      04-Jan-89 (jbowers)
**          Created
*/
static STATUS
gcc_psnd( GCC_MDE *mde )
{
    GCC_PCE		*pce;
    GCC_P_PLIST		*p_list;
    STATUS		status;

    /*
    ** Invoke the SEND function for normal data from the appropriate
    ** protocol handler.
    */

    pce = mde->ccb->ccb_bce.prot_pce;
    p_list = &mde->mde_act_parms.p_plist;
    p_list->options = 0;
    p_list->compl_exit = (void (*)())gcc_pbprot_exit;
    p_list->compl_id = (PTR)mde;
    p_list->buffer_ptr = mde->p1;
    p_list->pcb = mde->ccb->ccb_bce.pcb;
    p_list->pce = pce;
    p_list->buffer_lng = mde->p2 - mde->p1;
    p_list->function_invoked = (i4) GCC_SEND;
    mde->service_primitive = N_DATA;
    mde->primitive_type = CNF;
    status = ( *pce->pce_protocol_rtne )( GCC_SEND, p_list );
    return( status );
} /* end gcc_psnd */


/*
** Name: gcc_pb_abort	- Protocol Bridge internal abort
**
** Description:
**	gcc_pb_abort handles the processing of internally generated aborts.
** 
** Inputs:
**      mde		    Pointer to mde
**      ccb		    Pointer to ccb
**	generic_status      Mainline status code
**      system_status       System specfic status code
**      erlist		    Error parameter list
**
** Outputs:
**      none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-Mar-90 (cmorris)
**          Initial routine creation
**  	25-Jan-93 (cmorris)
**  	    Always reset p1, p2 before processing internal abort.
**  	    The pointers could be set to anything when the abort
**  	    occurred.
*/

static VOID
gcc_pb_abort
(
    GCC_MDE 		*mde,
    GCC_CCB 		*ccb,
    STATUS 		*generic_status,
    CL_ERR_DESC 	*system_status,
    GCC_ERLIST 		*erlist 
)
{
    STATUS		temp_generic_status;
    CL_ERR_DESC 	temp_system_status;

    /*
    ** We have to be careful here. It's possible that we
    ** have an "orphaned" MDE:- ie we got an incoming TPDU that did
    ** not refer to an existing connection. Consequently, there's no
    ** CCB associated with the MDE and we can't reenter bridge
    ** an event.
    */
    if (ccb == NULL)
    {
	/* Free up the mde and get out of here */
	gcc_del_mde( mde );
        return;
    }

    if( ! ( ccb->ccb_bce.b_flags & GCCPB_ABT ) )
    {
	/* Log an error */
	gcc_er_log(generic_status, system_status, mde, erlist);

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_bce.b_flags |= GCCPB_ABT;    

        /* Issue the abort */
    	if( mde != NULL )
    	{
	    mde->service_primitive = N_P_ABORT;
	    mde->primitive_type = RQS;
    	    mde->p1 = mde->p2 = mde->ptpdu;
            temp_generic_status = OK;
	    (VOID) gcc_pb( mde, &temp_generic_status, &temp_system_status );
	}
    }

}  /* end gcc_pb_abort */


/*
** Name: gcc_pbconfig	- PB routine to read values from config.dat.
**
** Description:
**	gcc_pbconfig accepts as input a pointer to a 
**	pce. Reads PMsymbols defined for a given protocol id ( pce->pce_pid ) 
**	from config.dat.
**
** Inputs:
**	pce	
**
**	    Pointer to pce.
**
**	Returns:
**
**	    TRUE or FALSE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      05-Dec-95 (rajus01)
**          Initial function implementation.
**	09-Feb-96 (rajus01)
**	    Added DEBUG statements.
**      25-Oct-2000 (ashco01) bug #103035		
**          Added call to CVlower() to convert constructed parameter name
**          search prefix 'pmsym' to lower case before call to PMscan(). 
**	11-sep-2000 (somsa01)
**	    Make sure we do a PMinit() and a PMload() before calling PMget().
**      19-sept-2001 (wansh01) 
**         for the selected protocol, make sure the status is on and  
**         at least a port is defined.  
**      04-Apr-2002 (wansh01)         
**          when a valid vnode/port found, copy the port/vnode info to
**          gcb_pm_reso[0] for futher CONNECT usage.  
*/

static bool
gcc_pbconfig( GCC_PCE *pce )
{
    char 		*pm_val;
    char 		pmsym[128];
    bool 		result = TRUE;
    i4			s = 0;
    i4      		nwords = 80; 
    char 		*p, *name, *words[80];	
    i4			temp_count;

    gcb_pm_reso.pmrec_count = 0;

    if (!pm_initialized)
    {
	PMinit();
	if (PMload(NULL, (PM_ERR_FUNC *)NULL) == OK)
	    pm_initialized = TRUE;
    }
    
    CVlower(pce->pce_pid); 

    if ( ! gcb_pm_reso.cmd_line)
       STprintf( pmsym, "$.$.$.$.%s.port.%%", pce->pce_pid );
    else 
       STprintf( pmsym, "$.$.$.$.%s.port", pce->pce_pid );

    scan_pmsym( pmsym );
    

    temp_count = gcb_pm_reso.pmrec_count;
    for( s = 0; s < gcb_pm_reso.pmrec_count; s++ )
    {
      if (STlength( gcb_pm_reso.pmrec_value[ s ] ) == 0)
      {
       /* no port specified. cannot use this port  */
       temp_count--;
       continue;
      }
      if ( ( ! gcb_pm_reso.cmd_line) && 
           (PMnumElem( gcb_pm_reso.pmrec_name[ s ]) == 7) )
      {
       /* parse name  and get the V_NODE name  */
       for( p = name = STalloc( gcb_pm_reso.pmrec_name[ s ] );
   	         	     *p != EOS;
		       	     CMnext( p ))
       {
        /* replace non-alpha with space for ** STgetwords() */
	if ( *p == '.'  )
	    *p = ' ';
       }
       STgetwords( name, &nwords, words );
       if ( words[6] != NULL )
          STprintf( pmsym, "$.$.$.$.%s.status.%s", pce->pce_pid,words[6]);
       else 
       {
          /*  no vode found    */   
	  temp_count--;
	  continue;
       }
      } 
      else
         /* it is a command line protocol     */  
         STprintf( pmsym, "$.$.$.$.%s.status", pce->pce_pid);


    if ( PMget( pmsym, &pm_val ) == OK )
    {
	if ( STcasecmp( pm_val, "ON" ) != 0 )
         /* status for this protocol vnode is off or NULL  */
         /* cannot use this port   */ 
         temp_count--;
	else 
	{
	   /* this port is ok   */
	  if  ( gcb_pm_reso.cmd_line ) 
            STcopy( gcb_pm_reso.pmrec_value[s], gcb_pm_reso.pmrec_value[ 0 ] );
          else 
	  {
          STcopy( words[ 6 ], gcb_pm_reso.pm_vnode[ 0 ] );
          STcopy( gcb_pm_reso.pmrec_value[s], gcb_pm_reso.pmrec_value[ 0 ] );
          }
        }
    }
    else  
       temp_count--;
    }    /* end for */ 
	 
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
     TRdisplay( "gcc_pbconfig: gcb_pm_reso.pmrec_count = %d,temp_count = %d \n", gcb_pm_reso.pmrec_count,temp_count );
	for( s = 0; s < gcb_pm_reso.pmrec_count; s++ )
	    TRdisplay( "gcc_pbconfig: name[ %d ] = %s value[ %d ] =  %s\n",
			s, gcb_pm_reso.pmrec_name[ s ],
			s, gcb_pm_reso.pmrec_value[ s ] );

# endif /* GCXDEBUG */


    /* make sure there is minimum one port definition in config.dat
    ** for the protocols that you want to use 
    ** if not, then return false 
    */
       
    if ( temp_count  <= 0)
	result =  FALSE ;      

    return( result );
}


/*
** Name: gcn_info - PB call back routine for Name Server.
**
** Description:
**	gcn_info accepts as input a message type 
** 	to Name Server. It also accepts as input a pointer
**	buffer and size of buffer.
**
** Inputs:
**	msg_type		Type of Mesg to Name Server.
**	buff			A character pointer to buff.
**	len			Size of buff.
**
**	Returns:
**	  0 or 1. 
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      05-Dec-95 (rajus01)
**         Culled from vnode.c by Gordy.
**	09-Feb-96 (rajus01)
**	    Added DEBUG statements.
*/

static i4
gcn_info
(
    i4		msg_type,
    char	*buff,
    i4	len
)
{
    GCA_ER_DATA *ptr;
    STATUS		temp_status;
    GCC_ERLIST		erlist;
    char        *type, *uid, *obj, *val;
    char        *pv[5];
    i4          cnt  = 0;
    i4		rows = 0;
    i4		op   = 0;
		 
    switch( msg_type )
    {
	case GCN_RESULT :

	    buff += gcu_get_int( buff, &op );
					      
	    if( op == GCN_RET )
	    {
		buff += gcu_get_int( buff, &rows );
	    
		while( rows-- )
		{
		    buff += gcu_get_str( buff, &type );
		    buff += gcu_get_str( buff, &uid );
		    buff += gcu_get_str( buff, &obj );
		    buff += gcu_get_str( buff, &val );

		    cnt = gcu_words( val, NULL, pv, ',', 3 );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcn_info: Type %s User %s V_node %s\n", type, uid, obj );
# endif /* GCXDEBUG */

		    if( cnt >= 1 )  STcopy( pv[0], gcb_to_addr.node_id );
		    if( cnt >= 2 )  STcopy( pv[1], gcb_to_addr.n_sel );
		    if( cnt >= 3 )  STcopy( pv[2], gcb_to_addr.port_id);
		}
	    }
	    break;
	case GCA_ERROR :

	    ptr  		= (GCA_ER_DATA *)buff;

	    if( ptr->gca_l_e_element > 0 )
	    {
		temp_status         = E_GC2A0A_PB_NO_VNODE;
		erlist.gcc_parm_cnt = 1;
		erlist.gcc_erparm[0].size = sizeof(
				ptr->gca_e_element[0].gca_id_error);
		erlist.gcc_erparm[0].value =
				(PTR)&ptr->gca_e_element[0].gca_id_error;
		gcc_er_log( &temp_status,
		       	    (CL_ERR_DESC *)NULL,
		            (GCC_MDE *)NULL,
		            (GCC_ERLIST *) NULL );
	    }
	break;
    }

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcn_info: node_id %s n_sel %s port_id %s \n",
		    gcb_to_addr.node_id, gcb_to_addr.n_sel,
		    gcb_to_addr.port_id );
# endif /* GCXDEBUG */

    return( op );
}

/*
** Name: scan_pmsym - Scan the given PM resource string.
**
** Description:
**	It scans the PM resource pool for the given string and
**	increment the counter.
**
** Inputs:
**	pmsym		The PM resource string to scan for.
**
**	Returns:
**	  VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      12-Nov-97 (rajus01)
**	    Created.
**
*/
static VOID scan_pmsym( char *pmsym )
{
    PM_SCAN_REC		state;

    /*
    ** Scan the definitions for a given protocol and store it in a
    ** global buffer.
    */
    if( PMscan( PMexpToRegExp( pmsym ) ,&state, NULL,
             &gcb_pm_reso.pmrec_name[gcb_pm_reso.pmrec_count],
              &gcb_pm_reso.pmrec_value[gcb_pm_reso.pmrec_count]
      ) == OK )
	++gcb_pm_reso.pmrec_count;

    while( PMscan( NULL, &state, NULL,
              &gcb_pm_reso.pmrec_name[gcb_pm_reso.pmrec_count],
               &gcb_pm_reso.pmrec_value[gcb_pm_reso.pmrec_count]
            ) == OK )
	++gcb_pm_reso.pmrec_count;

}

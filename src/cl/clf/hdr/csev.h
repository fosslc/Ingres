/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/
#ifndef CSEV_HEADER
#define CSEV_HEADER

#include <pc.h>

/**CL_SPEC
** Name: CSEV.H - CS event handling structures
**
** Description:
**	Definitions used by the internal CL routines of CS and its clients
**	(GC, LK, LG, DI).
**
** Specification:
**  
**  Motivation:
** 
**	CS needs to provide a mechanism by which its clients can perform
**	non-blocking operations on an OS (UNIX) which does not support
**	user-controlled	asynchronous events.
**
**  Description:
**	
**	CS is used to:
**	    o sleep threads, blocked waiting on a particular event.
**	    o wake up threads waiting on events (put to sleep by CSsuspend).
**	    o recognize and handle completion of events.
**	    o put the dbms server to sleep.
**	    o post events to the dbms server (waking it if necessary).
**	    o manage slave processes for for its clients
**	    o request events of slave processes
**
**  Intended Uses:
**
**	CS is used by the UNIX CL modules (CS, GC, DI, LG, and LK) to
**	distribute functions, such as I/O to other processes.  Async
**	operations on UNIX will be implemented by using slave processes
**	to perform the operation.  Communication between the dbms server and 
**	the slave(s) will be through shared memory and semaphores.
**	
**	
**	Actions taken by the DBMS:
**	
**	  o Thread continues until it has to make a blocking procedure call.
**	
**	  o post request to slave process
**	
**	  o either call CSsuspend to sleep thread (DI, LG, LK) or return to 
**	    caller (invoker of GC might call CSsuspend waiting for reactivation
**	    via the call of the exit routine).
**	
**	  o time goes by and eventually the dbms calls CS_find_events to check
**	    to see if events have completed.
**
**	  o if (events have completed)
**		foreach (event that has completed)
**		{
**	    	    call completion handler for that event
**		}
**
**	        o completion handler will finish event's operation
**	          (copy buffers from communication channel,
**		   resume thread via CSresume ...)
**
**	        o event handler :(DI, LK, LG) resume threads
**	               	         (GC) call exit routine for session which
**				  completed event exit routine will most
**				  probably call CSresume which will move
**				  session to run queue.
**
**	    o if (threads runable)
**		schedule a thread
**	      else
**		sleep waiting for an event.
**
**	Actions taken by SLAVE process:
**
**	 o wait for a request from the dbms server to be delivered
**
**	 o while procesing request, other requests may be queued by the server.
**
**	 o block waiting for completion of request (completed I/O) 
**
**	 o Indicate in shared memory and semaphore that request is completed
**
**	 o Post event to dbms server.
**
**	 o Sleep waiting for requests.
**
**
**	Examples processes:
**
**		DI slaves:
**
**		DBMS will fork a "configurable" number of DI slaves.  
**		Each slave communicates with the DBMS through a shared
**		memory segment and is responsible for moving data buffers
**		to/from disk and the shared segments.  These I/O operations
**		will block in the slave.  The slave will
**		notify the server that an operation has completed and will
**		indicate which event was completed.
**		Completion status of the operation is communicated
**		to the server through shared memory, and will eventually be
**		stored a status variable provided by the caller in the server.
**
**		GC slave (socket implementation):
**
**		DBMS will fork a single GC slave.  This slave will maintain
**		a single I/O control block for each connected user to the
**		server in shared memory.  Sockets will be used as the
**		communication channel.  Select will be
**		used to determine if a channel can be read or written in a
**		non-block operation.  The channel will be marked as 
**		non-blocking so that writes will not block the server (a 
**		select may indicate that a channel is writable, but it may not
**		allow the entire block to be written - in that case a partial
**		write will be performed and GC will have to keep track and
**		write the rest of the block when the select indicates it can).
**
**		The GC slave will normally be sleeping on a select.  
**		Whenever the select completes it will process the I/O and post
**		a notice to the server.  The dbms will require a way of
**		interrupting the select to post requests of the GC slave.
**		This can be done by connecting to a socket and sending
**		the message down the socket to the GC slave.
**		
**
**	How to use CS events:
**		
**		To use CS events a process must have a certain amount of
**		shared memory
**		mapped into its address space.  This memory is accessible to 
**		every process using CS within a single installation.
**		This memory is mapped by the CSinitiate() call.  Every user of
**		CS (both servers and slaves) must call CSinitiate() before any
**		other CS routine.
**
**		The requests of CS are driven by "events".
**		Each event is identified by a control block and a slave block
**		which holds locks and handler functions and other information.
**		These handlers are called by the dbms upon event completion.
**	
**		Clients of CS are expected to implement non-blocking CL
**		components through the use of slaves (other unix processes).
**		UNIX DBMS CL implementations will either block using CSsuspend
**		after causeing an event for the slave or will maintain
**		exit routines to be called by event handlers on completion of
**		the request.
**
**		Clients of CS events will resume operation following the
**		completion of an event request by completion handlers either
**		calling CSresume or an appropriate exit handler routine.
**
**		The DBMS will poll for events using CS_find_events() whenever
**		performing session context switches (ie. I/O, lock requests).
**		More polls may be implemented if further research indicates
**		necessity (during long in memory sorts, long optimizations).
**
**		When the DBMS recognizes completion of an event it will call
**		the provided handler, set up by the CS users in CSslaveinit.c.
**
**		Slaves of users of CS post event completion to the DBMS
**		implicitly by the return of handlers in slave processes.
**		This action will later cause CS_find_events in the DBMS to
**		call the handler provided in a call to CSslave_init.
**		Active posting of events to servers are not directly
**		provided yet but appear to be easy.
**
**		Note: The first implementation of this will not garantee that
**		users may use memory other than the event control blocks
**		for communication between slave and server.
**
**    Assumptions:
**	shared memory and counting semaphores between UNIX processes.
**		(Sys V shmem and semaphores)
**
**    Definitions:
**	  
**	  server	- DBMS server process
**
**	  slave		- subprocess of a DBMS server used to perform operations
**			  (reads, writes, ..) that would block.
**
**	  request	- the operation that the slave performs for the DBMS.
**
**	  event		- a communication mechanism through which the 
**			  DBMS informs a slave and is informed by a slave
**			  that a request has completed.
**			  
**
**
**    Concepts:
**
**	    
**	Errors which can be returned:
**
**      
** History:
 * Revision 1.3  88/09/29  21:49:04  jeff
 * increase shared memory per user
 * 
 * Revision 1.2  88/08/29  12:08:17  jeff
 * portability changes
 * 
 * Revision 1.6  88/05/05  12:59:27  anton
 * less server specific shared memory.  5k per user
 * 
 * Revision 1.5  88/04/19  15:38:18  anton
 * fixed cross process semaphores
 * 
 * Revision 1.4  88/03/29  15:37:41  anton
 * server-server events + misc fixes.
 * 
 * Revision 1.3  88/03/21  12:26:07  anton
 * Event system seems solid
 * 
 * Revision 1.2  88/03/03  11:56:42  anton
 * first cut at CS slave driven events
 * 
**      3-15-89 (markd)
**          Changed CS_ACLR macro as CS_relspin for SEQUENT.
**          Atmoically set locks must be unset atomically on Symmetry.
**          CS_relspin is an assembler routine defined in csll.s 
**
**	19-jan-88 (jaa)
**	    Revised and renamed for encapsilation into CS
**
**      Revision 1.2  87/09/29  17:40:21  mikem
**      greg's changes.
**      
**      28-sep-87 (mmm)
**	    Created.
**	31-jan-1989 (roger)
**	    Renamed executable to "iislave".
**	15-feb-1989 (mikem)
**	    Changed algorithm for figuring out size of shared memory to 
**	    allocate CSEV_MSIZE to better reflect the actual size of the
**	    objects being allocated.  The problem was that the DI_SLAVE_CB
**	    contains a CL_ERR_DESC which has grown in size.  For now we will
**	    change the extimate to reflect current reality - a better long
**	    term solution would be to add to the interface a way for the the
**	    event system users to indicate the size of the buffers they will
**	    be using before the shared memory segment has to be allocated.
**	10-Mar-1989 (fredv)
**	    Tset operation of RT is on 2 bytes. Defined CS_ASET as i2 for RT.
**	20-mar-89 (russ)
**	    Changed bzero to MEfill.
**	20-jul-89 (rogerk)
**	    Moved CS_ASET definition to CS.H since it is used in the
**	    CS_SEMAPHORE structure.
**	16-apr-90 (kimman)
**	    Adding ds3_ulx specific code for the context-switch (cs).
**	17-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Force an error if <clconfig.h> is not included;
**		Remove the default case of CS_ACLR, to force the definition
**		of an atomic clear operation;
**		Added specific cases for hp8_us5, sqs_us5, arx_us5, dr6_us5;
**		Remove superfluous "typedef" before struct _CS_SPIN;
**		Add code for new type of semaphores (xCL_078_CVX_SEM).
**	1-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Add machine-specific support for su4_u42.
**	7-june-90 (blaise)
**	    Moved atomic test & set type and atomic test & set op definitions
**	    to <cs.h>. The atomic test & set type definitions for most boxes
**	    were in cs.h anyway.
**      4-sep-90 (jkb)
**	    Define CS_relspin for sqs_ptx
**      30-oct-90 (rog)
**	    Added VOLATILE to the definition of CSEV_CB to prevent code
**	    from being moved when it shouldn't be.
**	03-Oct-90 (anton)
**	    Add declaration of CSdef_resume
**      10-jan-91 (jkb)
**          Increase the value of NSVSEMS from 10 to 30 to allow more
**	    slaves to be forked.
**	26-jun-90 (fls-sequent)
**	    Changed CS atomic locks to SHM atomic locks.
**	22-oct-90 (fls-sequent)
**	    Integrated changes from 63p CL:
**		Add xCL_075 and xCL_078 ifdefs.
**	    Integrated anton's MCT changes:
**		Add support for defered resumes of events.
**      10-jan-91 (jkb)
**          Increase the value of NSVSEMS from 10 to 30
**	30-Jan-91 (anton)
**	    Merge in MCT support
**	08-mar-91 (rudyw)
**	    Add odt_us5 to the list of machines not defining CS_relspin
**	    Reduce number of NSVSEMS to 25 for odt_us5 to ODT system limit.
**	15-nov-1991 (bryanp)
**	    Increase CSEV_MSIZE estimates to allow for growth in DI_SLAVE_CB.
**	06-nov-92 (mikem)
**	    Added su4_us5 to the list of machines which define CS_relspin()
**	    in assembler.
**      12-nov-92 (terjeb)
**	    Add hp8_us5 to the list of machines which needs to avoid
**	    defining CS_relspin at this point.
**	26-jul-1993 (bryanp)
**	    Increase MAXSERVERS to 128. This is the nineties, after all...
**	26-jul-93 (mikem)
**	    Added FUNC_EXTERN's for CSreserve_event(), CScause_event(), 
** 	    CSfree_event(), and CSslave_init().
**	31-jan-94 (mikem)
**	    sir #57671
**	    As part of making the size of server segment configurable by PM
**	    variables changed the CSEV_MSIZE macro to accept 2 arguments:
**	    the number of slave control blocks and the size of the data portion
**	    of the slave control blocks.
**	10-feb-1994 (ajc)
**	    Added hp8_bls specific entries based on hp8*
**	17-jul-1995 (morayf)
**	    Add sos_us5 to the list of machines which needs to avoid
**	    defining CS_relspin at this point.
**	12-dec-1995 (morayf)
**	    Add SNI RMx00 (rmx_us5) to those not using CS_relspin macro.
**	    However, shouldn't need to limit NSVSEMS below 30 on RMx00.
**	02-mar-1996 (morayf)
**	    Made pym_us5 like rmx_us5 except keep NSVSEMS at 25 as this
**	    is default kernel limit.
**	03-jun-1996 (canor01)
**	    Added OS_THREADS_USED define for concurrent threads.
**	28-feb-1997 (hanch04)
**	    changed OS_THREADS_USED
**	07-may-1997 (popri01)
**	    With OS_THREADS_USED, add Unixware (usl_us5) to those not
**	    using relspin
**	29-jul-1997 (walro03)
**	    Updated for Tandem NonStop (ts2_us5).
**	23-May-1997 (bonro01)
**	    Moved cssp_mutex to fix conflicting reference by csll.a
**	    assembly routines.
**	04-feb-1998 (somsa01) X-integration of 432936 from oping12 (somsa01)
**	    Added EV_EXIT to CSEV_CB for alerting the slave to exit.
**      06-Mar-1998 (kosma01)
**          include fdset.h for csev.h's prototype of function
**          CSslave_init. for rs4_us5, ris_us5, and ts2_us5
**	11-Nov-1997 (allmi01)
**	    Added entried for Silicon Graphics (sgi_us5)
**	18-feb-1998 (toumi01)
**	    Add Linux (lnx_us5) to platforms using csll.s CS_relspin.
**	16-Nov-1998 (jenjo02)
**	    Added function prototypes for CS_set_wakeup_block()
**	    CS_cleanup_wakeup_events(), CSMT_get_wakeup_block(),
**	    CSMT_free_wakeup_block(), CSMT_resume_cp_thread().
**	03-mar-1999 (walro03)
**	    Add Sequent (sqs_ptx) to list of platforms that need fdset.h.
**	10-may-1999 (walro03)
**	    Remove obsolete version string bu2_us5, bu3_us5, odt_us5, sqs_u42,
**	    sqs_us5.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	11-Oct-1999 (hweho01)
**	    Included fdset.h file for ris_u64 (AIX 64-bit platform). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	14-jun-2000 (hanje04)
**	    Added ibm_lnx to list of platforms using csll.s CS_relspin.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	10-oct-2002 (devjo01)
**	    Change 'flags' in CSEV_CB from short to i4.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	05-dec-2008 (joea)
**	    Correct prototypes of CS_set_wakeup_block and
**	    CS_cleanup_wakeup_events to use PID instead of i4 for their pid
**	    arguments.
**	11-dec-2008 (joea)
**	    Include pc.h so that 5-dec change doesn't break on Unix.
**      19-dec-2008 (stegr01)
**          Itanium VMS port, add CSEV_HEADER to prevent file being included
**          multiple times.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/


# ifndef CLCONFIG_H_INCLUDED
         error "didn't include clconfig.h before csev.h"
# endif

/*
**  Non-System V machines that use semaphores may have to
**  include <systypes.h> to define semaphore structure.
*/
# ifdef xCL_078_CVX_SEM
#include <systypes.h>
# endif

# if defined (any_aix) || defined(ts2_us5) || defined(sqs_ptx)
#include <fdset.h>
# endif

/*
**  Defines of constants.
*/

# define	MAXSERVERS	128
# define	CSEV_SMID	"CSEV"
# define	NEVUSERS	8

/* estimate of shared memory size needed by the server segment.  The formula is
** basically:
**	nusers * 8K buffer for each disk I/O operation possible +
**	nusers * 1024 bytes for cs and slave overhead (the DI_SLAVE_CB structure
**			is approximately 400 bytes + what ever is needed by cs).
*/
# define	CSEV_MSIZE(numblocks, sizeblocks)	((numblocks) * ((sizeblocks) + 1024l))

#if defined(pym_us5)
#define	NSVSEMS		25
#else
#define	NSVSEMS		30
#endif

/*
**  For non-System V machines that use semaphores and iislaves,
**  total number of slaves will equal to maximum number of semaphores.
*/
#define MAXSLAVES       NSVSEMS

#define	SLAVE		"iislave"
#define	CS_DI_SLAVE	1
#define	CS_GC_SLAVE	0

/*
**  Forward and/or External typedef/struct references.
*/
typedef	struct	_CSEV_SVCB	CSEV_SVCB;
typedef	struct	_CSSL_CB	CSSL_CB;
#ifdef sgi_us5
typedef	struct	_CSEV_CB	CSEV_CB;
#else
typedef	VOLATILE struct	_CSEV_CB	CSEV_CB;
#endif
typedef	struct	_CSEV_CALLS	CSEV_CALLS;
typedef	struct	_CSCP_SEM	CSCP_SEM;
typedef struct	_CS_SPIN	CS_SPIN;

/*
**  Forward function declarations
*/

FUNC_EXTERN VOID	CSdef_resume();

FUNC_EXTERN STATUS CSreserve_event(
    CSSL_CB        *slave_cb,
    CSEV_CB 	   **evcb);

FUNC_EXTERN STATUS CScause_event(
    CSEV_CB        *evcb,
    i4      	   slavenum);

FUNC_EXTERN STATUS CSfree_event(
    CSEV_CB        *evcb);

FUNC_EXTERN STATUS CSslave_init(
    CSSL_CB 	   **slave_cb,
    i4      	   usercode,
    i4      	   nexec,
    i4      	   wrksize,
    i4      	   nwrk,
    STATUS  	   (*evcomp)(),
    fd_set  	   fdmask);

FUNC_EXTERN STATUS CS_set_wakeup_block(
    PID		   pid,
    CS_SID	   sid);

FUNC_EXTERN VOID CS_cleanup_wakeup_events(
    PID	   	   pid,
    i4		   spinlock_held);

#ifdef OS_THREADS_USED
FUNC_EXTERN STATUS CSMT_get_wakeup_block(
    CS_SCB	   *scb);

FUNC_EXTERN VOID CSMT_free_wakeup_block(
    CS_SCB	   *scb);

FUNC_EXTERN VOID CSMT_resume_cp_thread(
    CS_CPID	   *cpid);
#endif /* OS_THREADS_USED */

/*}
** Name: CSSL_CB - Control Block describing a collection of slaves
**
** Description:
**	A collection of these structures are maintained by CS
**	and assigned to slave process groups.  The structure keeps
**	information common to the event control blocks of a slave
**	process group.
**
** History:
**      19-jan-88 (jaa)
**	     Created.
*/
struct	_CSSL_CB {
	i4	flags;		/* flags for slave group */
# define CSSL_MANY	0x01	/* one semaphore per slave */
# define CSSL_WANT	0x02	/* some user is waiting for a evcb */
# define CSSL_SERVER	0x04	/* this is a server event group */
	i4	nslave;		/* number of subslave processes */
	i4	slave_type;	/* index into slave function table */
# ifdef xCL_075_SYS_V_IPC_EXISTS
	int	slsemid;	/* semaphore group for slaves */
# endif
	int	slsembase;	/* base of semaphores for slaves */
/* note: there may be 1 semaphore for a group or 1 semaphore per slave */
	STATUS	(*evcomp)();	/* compleation routine */
	i4	evsize;		/* size of each event cb in bytes */
	i4	nevents;	/* number of event cbs in this pool */
	i4 events;    	/* offset to the pool array */
	i4 evend;	    	/* offset past end of pool array */
};

/*}
** Name: CSEV_CB - An event control block
**
** Description:
**	This structure is kept in shared memory between server and slaves.
**	It provides space for users to communicate requests to slaves and
**	for the server to manage thread scheduleing.
**
** History:
**      19-jan-88 (jaa)
**          Created.
**	15-mar-88 (anton)
**	    New flag group.
**	26-nov-90 (fls-sequent)
**	    Changed CS_ASET to SHM_ATOMIC.
**	03-Oct-90 (anton)
**	    New flag - deferred resume - needed for MCT/Reentrant CL
**	30-Jan-91 (anton)
**	    Change SHM_ATOMIC back to CS_ASET
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	04-feb-1998 (somsa01) X-integration of 432936 from oping12 (somsa01)
**	    Added EV_EXIT to CSEV_CB for alerting the slave to exit.
**	10-oct-2002 (devjo01)
**	    Change 'flags' from short to i4.  On most RISC machines,
**	    (and in particular alpha), operations on short integers
**	    are much less efficient than their int and long counterparts,
**	    and no storage is saved, because an invisible two byte pad
**	    is added after 'flags' to align the next field properly.
*/

struct _CSEV_CB
{
	CS_ASET busy;	/* flag for Test and set instructions */
	i4	flags;		/* other bit word flags */
# define	EV_RSV	0x01	/* evcb is reserved */
# define	EV_INPG	0x02	/* event is in progress */
# define	EV_WAIT	0x04	/* someone is waiting for this to finish */
# define	EV_SLCP	0x08	/* request for slave is done */
# define	EV_BGCM	0x10	/* completion handleing has begun */
# define	EV_PNDF	0x20	/* a free operation is pending */
# define	EV_REST 0x40	/* event restarted in completion handler */
# define	EV_SERV	0x80	/* This is a server event block */
# define	EV_DRES	0x100	/* This evcb has a deferred resume */
# define	EV_EXIT	0x200	/* This evcb has a deferred resume */
# ifdef xCL_075_SYS_V_IPC_EXISTS
	int	retsemid;	/* semaphore group for return event */
# endif
	int	retsemnum;	/* semaphore for return event to server */
	CS_SID	sid;		/* session id of suspended user */
	i4	forslave;	/* for which slave in the group */
	i4	slave_no;	/* id of slave cb in server */
	i4	slave_type;	/* index into slave function table */
	i4	datalen;	/* length of following data */
	char	data[sizeof (i4)];	/* variable length data area */
};

/*}
** Name: CSEV_CALLS	- array of slave functions
**
** Description:
**	This defines an array of functions to be used in slave process.
**
** History:
**      19-jan-88 (anton)
**	    Created.
*/

struct	_CSEV_CALLS
{
	STATUS	(*evinit)();
	STATUS	(*evhandle)();
};

/*}
** Name: CSEV_SVCB - CS event system control block
**
** Description:
**	This structure is kept at the base of the server shared memory
**	segment to define the format of that shared memory segment and
**	allow the event system to communicate between master and slaves.
**
** History:
**      19-jan-88 (anton)
**	    Created.
**	26-jun-90 (fls-sequent)
**	    Changed CS_ASET to SHM_ATOMIC.
**	30-Jan-91 (anton)
**	    back to CS_ASET
*/
struct	_CSEV_SVCB
{
	i4	msize;
	i4	events_off;
	CS_ASET	event_done[NEVUSERS];
	CSSL_CB	slave_cb[NEVUSERS];
};

/*}
** Name: CS_SPIN - Spinlock structure
**
** Description:
**	This structure holds all information assocated with a spin lock
**	Spinlocks are used as a last resort syncronizing method to prevent
**	two processes accessing the same data structure in an improper
**	mannor.  Spinlocks should only be held for VERY short sections
**	of code.
**
**	NOTICE: This structure must match the assembly code in csll.s
**
** History:
**      25-jul-88 (anton)
**          Created.
**	15-mar-89 (markd)
**	    Changed so CS_relspin is not defined as a macro
**	    for SEQUENT.  On the Symmetry CS_relspin must be an
**	    atomic clear, and it is written in assembler in csll.s.
**      01-oct-90 (chsieh)
**	    Changed so CS_relspin is not defined as a macro
**	    for bu2_us5 and bu3_us5. Same as su4_u42, and sqs_us5, sqs_u42 .... 
**	30-Jan-91 (anton)
**	    This is defined in shmlock.h for MCT
**	08-mar-91 (rudyw)
**	    Add odt_us5 to the list of machines which needs to avoid
**	    defining CS_relspin at this point.
**	06-nov-92 (mikem)
**	    Added su4_us5 to the list of machines which define CS_relspin()
**	    in assembler.
**      12-nov-92 (terjeb)
**	    Add hp8_us5 to the list of machines which needs to avoid
**	    defining CS_relspin at this point.
**	9-aug-93 (robf)
**          Add su4_cmw 
**	10-feb-1994 (ajc)
**	    Added hp8_bls specific entries based on hp8*
**	17-jul-1995 (morayf)
**	    Add sos_us5 to the list of machines which needs to avoid
**	    defining CS_relspin at this point.
**	18-feb-1998 (toumi01)
**	    Add Linux (lnx_us5) to platforms using csll.s CS_relspin.
**      28-Jul-1998 (linke01)
**          Adopt morayf's change described above for sui_us5
**	22-Dec-2004 (bonro01)
**	    Solaris a64_sol does not use this CS_relspin(), we use the
**	    one defined in csnormal.h
**	05-Feb-2008 (hanje04)
**	    SIR OSXSI
**	    Don't define CS_relspin() for OSX, defined in csnormal.h
*/
struct _CS_SPIN
{
    CS_ASET	cssp_bit;	/* bit to spin on */
    u_i2	cssp_collide;	/* collision counter */
# ifdef OS_THREADS_USED
    CS_SYNCH	cssp_mutex;	/* spin-lock object */
# endif /* OS_THREADS_USED */
};

/* CS_getspin is in assembler in csll.s */
# if !defined(ds3_ulx) && !defined(su4_u42) && !defined(sqs_ptx) && \
     !defined(sparc_sol) && \
     !defined(any_hpux) && !defined(su4_cmw) && !defined(hp8_bls) && \
     !defined(sos_us5) && !defined(rmx_us5) && !defined(pym_us5) && \
     !defined(usl_us5) && !defined(ts2_us5) && !defined(sgi_us5) && \
     !defined(sui_us5) && !defined(int_lnx) && !defined(rux_us5) && \
     !defined(ibm_lnx) && !defined(a64_sol) && !defined(int_rpl) && \
     !defined(OSX) && !defined(VMS)
#define	CS_relspin(s)	(CS_ACLR(&(s)->cssp_bit))
# endif


/*}
** Name: CSCP_SEM - Cross process semaphore structure
**
** Description:
**	This structure holds all counters and information for cross
**	process semaphores on UNIX.  It must be allocated in shared memory
**	to be used.
**
** History:
**      19-jan-88 (anton)
**	    Created.
**	26-jun-90 (fls-sequent)
**	    Changed CS_ASET to SHM_ATOMIC and CS_SPIN to SHM_SPIN.
**	30-Jan-91 (anton)
**	    back to CS_ASET and CS_SPIN
*/
struct _CSCP_SEM
{
    u_i2	iwant;
    CS_ASET	tasbit;
    CS_SPIN	spinbit;
# ifdef xCL_075_SYS_V_IPC_EXISTS
    int		semid;
# endif
    int		semnum;
#ifdef	OS_THREADS_USED
    i4		collisions;
#endif
};

#endif /* CSEV_HEADER */

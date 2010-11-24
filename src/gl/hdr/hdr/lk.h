/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

/**CL_SPEC
** Name: LK.H - Definition for the LK compatibility library.
**
** Description:
**      This file contains the definitions of types, functions and constants
**      needed to make calls on the compatibility LK functions.
**
**  NOTE: This file is now in the GL! This means that a common version of this
**	  file is shared among ALL systems. Changes to this file must be
**	  submitted to the CL committee for approval.
**
** Specification:
**
**   Description:
**	Locking management services for multiple transactions per process.
**	The ability to manage multiple transaction in the same process is
**	supported by the LK routines, with some assistance from the caller.
**	The LK routines allow the caller to create many transactions in a
**	process, but it is up to the caller to pass transaction identification
**	information to the LK routines when a lock is requested or released.
**	A new transaction is created by calling LKcreate_list.  LKcreate_list
**	returns a lock list identifier.  This lock list identifer is passed
**	to LKrequest and LKrelease to effect locks for a specific transaction.
**
**   Intended Uses:
**	The LK routines are intended for the sole use of DMF.
**
**   Assumptions:
**	It is assumed that the concept of a lock, a list of locks and the
**	possiblity of deadlock between these sets is understood.
**
**	LK is used to set locks on objects for a transaction on behalf of the
**	caller.  It is the responsiblity of the caller to tell the locking 
**	system which transaction is requesting lock services.
**
**	The performance of the LK routines is paramount to the performance of
**	transaction locking, and multiple server synchronization.  Special
**	care should be taken to insure that the LK routines are implemented
**	in a efficent manner.
**
**   Definitions:
**
**(	Lock	    - A lock is the name of a resource and the requested lock 
**		      mode for that resource.
**
**	Lock Id	    - A lock identifier is a number that represents a unique and
**		      similar identifier for a lock.  The resource portion of
**		      a lock identifier is the same for all locks on a resource.
**		      The lock portion of a lock identifier is different for
**		      every current lock.  Lock identifiers can be used in place
**		      of lock keys in certain LK calls, and are usually faster
**		      then using the lock key.
**
**	Lock Key    - A string of bytes that names the resource to be locked.
**
**	Lock Mode   - One of the following modes:
**		    NL	- Null, no lock.
**		    IS	- Intent to Share, implies finer granularity locking.
**		    IX	- Intent to Update, implies finer granularity locking.
**		    S	- Shared access only.
**		    SIX - Shared with intent to update.
**		    U	- Update Mode locking.
**		    X	- Exclusive access.
**
**		    The following table shows which granted mode locks are
**		    compatible with the requested mode.
** 
**                      |    NL   IS   IX   S   SIX   U   X
**                  ----|----------------------------------
**                  NL  |    Y    Y    Y    Y   Y     Y   Y
**                  IS  |    Y    Y    Y    Y   Y     N   N
**                  IX  |    Y    Y    Y    N   N     N   N
**                  S   |    Y    Y    N    Y   N     N   N
**                  SIX |    Y    Y    N    N   N     N   N
**		    U   |    Y    N    N    Y   N     N   N
**                  X   |    Y    N    N    N   N     N   N
**
**		    (See: Notes on Database Operating Systems for an in depth 
**		    explanation.)
**
**	Lock List   - A list of locks held by a transaction.
**
**	Lock Value  - A value that can be stored with a lock, the value is
**		    returned on every lock request and can be set when a
**		    lock is converted or released from SIX or X mode.  The value
**		    also includes the granted mode of the lock, which may be
**		    different then the requested mode of the lock.
**
**	Logical Lock - A lock that is requested and held until end of transaction.
**		    If the lock is requested again in a different mode, the
**		    resulting mode is determined from the following table.
**
**                      |    NL   IS   IX   S   SIX   U   X
**                  ----|----------------------------------
**                  NL  |    NL   IS   IX   S   SIX   U   X
**                  IS  |    IS   IS   IS   S   SIX   U   X
**                  IX  |    IX   IX   IX   SIX SIX   X   SIX
**                  S   |    S    S    SIX  S   SIX   U   SIX
**                  SIX |    SIX  SIX  SIX  SIX SIX   SIX X
**		    U	|    U    U    X    S   SIX   U   X
**                  X   |    X    X    X    X   X     X   X
**
**	Physical Lock - A lock that is held for a short time and released
**		    before end transaction. Physical locks are used to synchro-
**		    nize access to an object that is being logically locked at a
**		    finer granularity.  A physical lock can explicitly be
**		    converted from any mode to any other mode.  A 
**		    lock remains physical as long as all future requests for the
**		    lock specify the physical attribute.  If an existing
**		    physical lock is requested without the physical attribute,
**		    the lock becomes a logical lock.
**
**	Related Lock List - A related lock list in coordination with a normal
**		    lock list, define the set of locks held by a transaction.
**		    The related lock list holds locks that span transactions.
**		    An example would be a database lock.  The lock is held
**		    for many transactions, and only released when the database
**		    is released.
**
**	Transaction - A unit of work that collects locks on objects so that
**		    updates to multiple objects are atomic and serializable.
**)
**   Concepts:
**	LK provides a service to create a lock list. This lock list can be created
**	using  a caller supplied unique identifier, or LK can be requested to
**	generate a unique identifier.  The identifiers supplied by the caller,
**	typically a transaction identifier, can not be in the range of
**	identifiers that LK will generate for identifers that are requested of
**	LK.  The lock list is assigned and lock list identifer that
**	is returned to the caller.  This identifier is used by all LK services
**	to identify the lock list, (i.e. transaction), to operate on.  A lock
**	list can be located using the create list service by unique identifier.
**	This is used for recovery purposes, and documented later.
**	
**	Once a list has been created, lock requests can be performed.  A lock 
**	request involves the following pieces of information: lock key,
**	lock mode, lock list, lock id, timeout and flags.  A typical request
**	sets  a lock on a lock key in a specific mode for a specific transaction
**	that returns a lock identifier if the lock is granted before the timeout
**	interval has elapsed.  Variations on this theme allow the following
**	scenerios:
**
**(	    o	Request a lock on a resource that is already locked by this
**		transaction.  In this case the LK routines will convert the
**		lock to it's lock mode described in the definitions for 
**		logical locks.
**
**	    o	Request a lock and it's associated value.  When the lock
**		is granted the value stored with the lock is also returned.
**
**	    o	Explicit requests for converting physical locks.  A lock can
**		be explicitly converted to a higher or a lower mode.  This allows
**		information to be passed between processes.  A process which
**		wants to set the lock requests it X mode, it then converts
**		the lock to NL and the value passed in on this conversion
**		request is stored with the lock.  Another process can look
**		at the value of the lock by requesting the lock, or converting
**		an instance of the lock to get the new value.
**)
**	Once a lock has been requested it is normally only released when a
**	lock list is released.  This is not true for physical locks.  A
**	physical lock can be released or converted to a higher lock mode or
**	even converted to a lower lock mode.  Physical locks are used to
**	synchronize access to shared resources for short defined intervals
**	while a transaction is manipulating the objects.  This is most often
**	used to increase the concurrency at the expense of the locking
**	overhead.  (An exclusive physical lock can be set on a page in a btree
**	when a split is going to occur, and can be released to allow others
**	access to the btree.)  Physical locks can also be used to send short
**	messages between processes by storing values with the locks and releasing
**	the lock so that another process can aquire that lock and look and/or
**	change the value.  This feature is used to implement distributed
**	object caching, using the value as a timestamp for the object.
**	
**	When resources needed to store locks have been exhausted, the caller
**	has the option of converting from finer granularity locking to
**	coarser granularity locking and releasing the finer granularity locks.
**	For example this happens when a cleint has a table locked in IX mode and
**	is setting X page locks, decides to escalate by converting the
**	table lock to X and releasing all the page locks.  The release routine
**	supports releasing locks that have a similar lock key prefix.  Locks
**	have similar prefixes when the first 16 bytes of the lock key are
**	the same.
**
**	There is also support for requesting the list of locks held by a transaction.
**	This is used for debugging purposes and for the distributed system to log
**	the locks for the first phase of 2-phase commit.  If a transaction has just
**	been returned deadlock, there will be the ability to ask for the transactions
**	involved in the deadlock cycle.  This can then be logged so that the user
**	can diagnose application locking problems.
**
**	The special support for recovery, allows another process to gain
**	access to the transaction lock list of a caller whose process has
**	aborted.  This allows the recovery system to take over a transaction
**	so that it can be aborted and the locks released when this has
**	completed.  The distributed transaction reincarnation support allows
**	the recovery system to write the list of locks held by a distributed
**	transaction to the log for 2-phase commit.  As part of recovery for
**	2-phase transactions, the recovery system will read the lock list
**	from the log and set all the locks again so that the transaction
**	still appears to be active and 	can wait until the outcome of the
**	distributed transaction is known.
**
** History:
**      17-sep-1985 (derek)
**          Created.
**	16-jun-1986 (derek)
**	    Minor cleanup and extentions.
**	30-sep-1988 (rogerk)
**	    Added LK_E_INTERRUPT to LKevent flags
**	06-feb-1989 (mikem)
**	    Added bunches of internal LK error returns.
**	30-feb-1988 (rogerk)
**	    Added lock types for shared buffer manager - LK_BM_LOCK, LK_BM_TABLE
**	    and LK_BM_DATABASE.  Added lock list create options: CROSS_PROCESS
**	    and CONNECT.  Added event flag LK_E_CROSS_PROCESS.  Added show
**	    options LK_S_SLOCK, LK_S_LIST.
**	20-mar-1989 (rogerk)
**	    Added show option LK_S_SRESOURCE.
**	    Added rsb_count field to return the number of holders of a resource.
**	20-apr-1989 (fred)
**	    Added lock type LK_CONTROL for use by logging system to set and
**	    communicate characteristics.  Use here is for communicating the user
**	    defined datatype level.
**	21-feb-1990 (sandyh)
**	    Added LK_S_SUMM flag to LK_SHOW and LK_SUMM struct.
**	12-Jan-1989 (rogerk)
**	    Added error numbers for new LK errors.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove superfluous "typedef" before structures.
**	31-may-90 (blaise)
**	    Integrated changes from termcl:
**		Add new error E_CL1056_LK_SEM_OWNER_DEAD, to be returned
**		when the LK cross process semaphore is held by a process 
**		which has exited.
**	12-jul-1990 (bryanp)
**	    Invented LK_A_INTERRUPT code for LKalter in order to allow DMF
**	    to make a non-interruptible lock list interruptible again.
**      17-jul-1990 (mikem)
**          bug #30833 - lost force aborts
**          Added new error return for LKrequest().
**          It is possible to return from a lock wait and have both been
**          granted the lock, and delivered an interrupt.  In order to not
**          not lose both of these bits of information return a new status
**          for this case (LK_INTR_GRANT) from LKrequest().  It is up to the
**          clients of this routine to take the appropriate actions.
**	16-aug-1991 (ralph)
**	    Added lock type LK_EVCONNECT.  This physical lock is requested
**	    by the event thread when connecting to the event subsystem.
**	    The key consists of the server's pid.  It is used to determine
**	    if the server crashed without disconnecting from the EV subsys.
**	    Before a user thread attempts to queue an event instance to a
**	    server, it attempts to obtain this lock.  If able to
**	    do so, the server associated with the lock has crashed.
**	    This assumes that the lock will (eventually) be released
**	    on behalf of the crashed server.
**	28-jan-1992 (bryanp)
**	    Resolved final incompatibilities between the Unix & VMS versions
**	    Of this file and moved it into the GL. There is now one version
**	    of this file shared among all systems!
**	09-oct-1992 (jnash)
**          6.5 Recovery Project
**           -  Add LK_S_KEY_INFO, LK_ROW
**	20-oct-1992 (markg)
**	    Add new lock type - LK_AUDIT, used by the SXF auditing subsystem.
**	26-apr-1993 (bryanp/andys)
**	    Prototype LK interface.
**	    Add LK_NOSTATUS return code.
**	    Add LK_MASTER_CSP flag for LKcreate_list().
**	    Add new LKalter() codes LK_A_STALL_LOGICAL_LOCKS,
**		LK_A_STALL_NO_LOCKS.
**	24-may-1993 (bryanp)
**	    Added llb_pid to return the Process ID which created this locklist.
**	    Added synch_complete and asynch_complete statistics.
**	    Added csp_msgs_rcvd and csp_wakeups_sent statistics.
**	21-jun-1993 (bryanp)
**	    Added LK_A_CSP_ENTER_DEBUGGER for internal development use.
**	    Added llb_status definition for LLB_INFO_NONPROTECT for lockstat.
**	26-jul-1993 (bryanp)
**	    Added support for showing lock list's event waiting info.
**	31-jan-1994 (rogerk)
**	    Added new LKshow option - LK_S_LIST_LOCKS - for showing all the
**	    locks owned by a particular lock list.
**	31-jan-1994 (mikem)
**	    Bug #58407.
**	    Added CS_SID info to the LK_LLB_INFO structure: llb_sid.  This is 
**	    used by lockstat and internal lockstat type info logging to print 
**	    out session owning locklist. 
**	    Given a pid and session id (usually available from errlog.log 
**	    traces), one can now track down what locks are associated with
**	    that session.
**       19-apr-1995 (nick)
**          Added LK_CKP_TXN as fix for bug #67888.
**	30-jun-1995 (thaju02)
**	    Added LK_OVERRIDE_NIW for ckpdb -max_stall_time=...
**	13-july-1995 (thaju02)
**	    Removed (unnecessary) LK_OVERRIDE_NIW bit. 
**	06-Dec-1995 (jenjo02)
**	    Added LK_S_MUTEX function to LKshow(), LK_MUTEX structure in
**	    which locking system semaphore stats are returned.
**      01-aug-1996 (nanpr01 for ICL keving)                                    
**          Blocking Callbacks Project. 
**          Add LKcallback and LKrequestWithCallback prototypes.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Added lock_key parameter to LKcallback prototype.
**	21-Nov-1996 (jenjo02)
**	    Added 2 new stats for Deadlock Detector thread.
**      27-feb-1997 (stial01)
**          Added LK_S_XSVDB,LK_RECOVER_LLID
**	07-Apr-1997 (jenjo02)
**	    Added LK_S_SLOCKQ to quiet expected errors in DMCM.
**	    Added new stats for callback threads.
**	24-Apr-1997 (jenjo02)
**	    Added LK_S_DIRTY flag to augment LKshow() option. This notifies
**	    LKshow() that locking structures are NOT to be semaphore-protected,
**	    used as a debugging aid.
**      07-may-97 (dilma04)
**          Bug 81895. Add LK_COV_LOCK status return code to notify a caller of 
**          LKrequest() routine that an intend page lock have been converted to 
**          a covering mode. 
**	08-May-1997 (jenjo02)
**	    Added function prototype for LKpropagate().
**	    Added LK_PROPAGATE flag, used internally to calls to LK_request().
**      14-may-97 (dilma04)
**          Add LK_LOG_LOCK status return code to notify a caller of LKrequest()
**          routine that a physical lock have been converted to a logical lock.
**	15-May-1997 (jenjo02)
**	    Added LK_INTERRUPTABLE flag for LKrequest() to enable interruption of
**	    a specific lock request even if the lock list is marked non-interruptable.
**	    Removed LK_ENABLEINTR, which was only being used as a debugging flag.
**	06-Aug-1997 (jenjo02)
**	    Added LK_DEADLOCKED flag to LKrequest() flag values. Used only internally.
**	28-Oct-1997 (jenjo02)
**	    Added LK_NOLOG flag for LKrelease(). Some lock release errors are 
**	    acceptable (like lock not found) and will not be logged if this flag
**	    is set.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter.
**	20-Jul-1998 (jenjo02)
**	    LKevent event type and event value expanded and merged
**	    into LK_EVENT structure.
**	16-Nov-1998 (jenjo02)
**	    Added max_lkb_per_txn (max locks requested per txn) stat.
**      16-feb-1999 (nanpr01)
**          Support Update mode locks.
**	15-Mar-1998 (jenjo02)
**	    Added LK_S_LIST_NAME LKshow() function.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	28-May-1999 (nanpr01)
**	    SIR 91157 : Added LK_QUIET_TOO_MANY flag to stop logging 
**	    DMA00D message.
**	06-Oct-1999 (jenjo02)
**	    Correctly type second parm to LKshow() as LK_LLID.
**	30-Nov-1999 (jenjo02)
**	    Added conditional include <iicommon.h> to pick up
**	    DB_DIS_TRAN_ID type.
**	    Added dis_tran_id parm to LKcreate_list() prototype.
**	    Added llb_sllb_id, llb_s_cnt, llb_dis_id, brought LLB_INFO
**	    defines up to date in LK_LLB_INFO.
**	15-Dec-1999 (jenjo02)
**	    Deleted llb_dis_id, no longer needed.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype,
**	    <iicommon.h> include.
**	24-Aug-2000 (jenjo02)
**	    Added LK_A_REASSOC function to LKalter() for SHARED
**	    WILLING_COMMIT lock lists.
**	08-feb-2001 (devjo01)
**	    Add LK_CSP, LK_CMO, LK_MSG.  Used by CX facility.  
**	    These lk_type's will not show up as regular Ingres locks.
**	04-dec-2001 (devjo01)
**	    Add structures to explicitly lay out the memory structure
**	    for the buffers passed to LKshow for LK_S_LOCKS and
**	    LK_S_RESOURCE.
**	28-Feb-2002 (jenjo02)
**	    Made LK_MULTITHREAD an attribute of a lock list.
**	    Added LK_PHANTOM return code, used internally
**	    by LKrequest(), and accompanying E_CL1058_LK_REQUEST_RETRIED.
**	05-Mar-2002 (jenjo02)
**	    Added LK_SEQUENCE lock type for Sequence Generators.
**	19-Aug-2002 (jenjo02)
**	    Added LK_UBUSY return code to distinguish TIMEOUT=NOWAIT
**	    "busy" induced by user from internal LK_BUSY.
**      23-Aug-2002 (devjo01)
**          Add LK_LOCAL, a flag passed to LKrequest which says lock key
**          scope is limited to current node in clustered ingres.
**      04-sep-2002 (devjo01)
**          Add LKkey_to_string(), to provide a centralized key formatting
**          routine to replace a number of redundant routines.
**      22-oct-2002 (stial01)
**          Added LK_XA_CONTROL
**      28-may-2003 (stial01)
**          Renamed LK_ESCALATE -> LK_IGNORE_LOCK_LIMIT
**	17-Dec-2003 (jenjo02)
**	    Added LKconnect functionality for Partitioned Table,
**	    Parallel Query project.
**      22-sep-2004 (fanch01)
**          Remove meaningless LK_TEMPORARY flag.
**	09-may-2005 (devjo01)
**	    - Add LK_A_ENABLE_DIST_DEADLOCK as LKalter flag.
**	    - Rename certain LKDSM fields for clarity, rename
**	      LKDSM to LK_DSM for consistency.  Add some fields
**	      to provide better deadlock diags.
**	23-nov-2005 (devjo01)
**	    - Add LK_NOTHELD.
**      13-feb-2006 (horda03) Bug 112560/INGSRV2880
**          Add LK_RELEASE_TBL_LOCKS. This signals to LKrelease, that the
**          LK_PARTIAL release is to release all locks held by the
**          transaction on the specified table.
**	17-oct-2006 (abbjo03/jenjo02)
**	    Change members of LK_LKB_INFO to be in agreement with the LKB.
**	19-oct-2006 (abbjo03)
**	     Change o_stamp in LK_DSM to u_i4 to match llb_stamp in LLB.
**	18-jan-2007 (jenjo02/abbjo03)
**	    Add msg_seq to LK_DSM for debugging.
**	28-Feb-2007 (jonj)
**	    Add LK_NOSTALL for LKrequest.
**	21-Mar-2007 (jonj)
**	    Add LK_REDO_REQUEST status.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	04-Apr-2008 (jonj)
**	    Add LK_S_ULOCKS LKshow() function. Returns OK if
**	    LK_U locks are supported.
**      01-may-2008 (horda03) Bug 120349
**          Add LK_LOCK_TYPE_MEANING define
**      16-Jun-2009 (hanal04) Bug 122117
**          - Added LKalter() options LK_A_FORCEABORT and LK_A_NOFORCEABORT.
**          - Added LK_INTR_FA as a lock request return code. Used when a
**            lock request is cancelled/interrupted because the session has
**            been flagged for force abort.
**          - Added E_CL1071_LK_FORCEABORT_ALTER, E_CL1072_LK_FORCEABORT_ALTER.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add LK_RELLOG flag for LKrelease,
**	    LK_CROW lock type.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype LKdeadlock_thread(), LKdist_deadlock_thread()
*/


/*
**  Forward and/or External typedef/struct references.
*/
# ifndef LK_H_INCLUDED
# define LK_H_INCLUDED

typedef struct _LK_LOCK_KEY LK_LOCK_KEY;
typedef struct _LK_VALUE    LK_VALUE;
typedef struct _LK_UNIQUE   LK_UNIQUE;
typedef struct _LK_LLB_INFO LK_LLB_INFO;
typedef struct _LK_LKB_INFO LK_LKB_INFO;
typedef struct _LK_RSB_INFO LK_RSB_INFO;
typedef struct _LK_BLKB_INFO LK_BLKB_INFO;
typedef struct _LK_DSM	    LK_DSM;
typedef struct _LK_STAT	    LK_STAT;
typedef struct _LK_TSTAT    LK_TSTAT;
typedef struct _LK_SUMM	    LK_SUMM;
typedef struct _LK_MUTEX    LK_MUTEX;
typedef struct _LK_EVENT    LK_EVENT;
typedef	struct _LK_S_LOCKS_HEADER LK_S_LOCKS_HEADER;
typedef	struct _LK_S_RESOURCE_HEADER LK_S_RESOURCE_HEADER;

/*
**  Defines of other constants.
*/

/*
**  Constants used for logging locks.
*/

#define			LK_REQUEST 	0x00L
#define                 LK_RELEASE      0x01L

/*
**  Constants for the flag argument of LKcreate_list().
*/

#define                 LK_RECOVER	    0x01L
#define			LK_ASSIGN	    0x02L
#define			LK_NONPROTECT	    0x04L
#define			LK_MASTER	    0x08L
#define			LK_NOINTERRUPT	    0x10L
#define			LK_SHARED	    0x20L
#define			LK_CONNECT	    0x40L
#define			LK_MASTER_CSP	    0x80L
#define                 LK_RECOVER_LLID     0x100L
#define                 LK_MULTITHREAD      0x8000L

/*
**  Constants for the flag argument of LKrequest().
*/

#define			LK_LOGICAL		0x00L
#define			LK_NOWAIT		0x01L
#define			LK_CONVERT		0x02L
#define			LK_PHYSICAL		0x04L
#define			LK_STATUS		0x08L
#define			LK_IGNORE_LOCK_LIMIT	0x10L
#define                 LK_LOCAL                0x20L
#define			LK_NODEADLOCK		0x40L
#define			LK_NOSTALL		0x80L
#define			LK_NONINTR		0x100L
#define			LK_PROPAGATE		0x200L
#define			LK_INTERRUPTABLE	0x400L
#define			LK_DEADLOCKED		0x800L
#define			LK_QUIET_TOO_MANY	0x1000L
#define LK_REQ_FLAG_4CH "NOWT,CONV,PHYS,STAT,ESCT,LOCL,NODK,"\
                        "TEMP,NOIN,PROP,INTR,DEAD,QUTM"

/*
**  Constants for the flag argument of LKrelease().
*/

#define                 LK_SINGLE       ((i4)0x00)
#define			LK_ALL		((i4)0x01)
#define			LK_PARTIAL	((i4)0x02)
#define			LK_RELATED	((i4)0x04)
#define			LK_AL_PHYS	((i4)0x10)
#define			LK_NOLOG	((i4)0x20)
#define			LK_RELEASE_TBL_LOCKS ((i4)0x40)
#define			LK_RELLOG	((i4)0x80)

/*
**  Constants for the lock_mode argument of LKrequest().
*/

#define                 LK_N		((i4)0)
#define			LK_IS		((i4)1)
#define			LK_IX		((i4)2)
#define			LK_S		((i4)3)
#define			LK_SIX		((i4)4)
#define			LK_U		((i4)5)
#define			LK_X		((i4)6)

#define LK_LOCK_MODE_MEANING "N,IS,IX,S,SIX,U,X"

/*
**  Constants for call to LKevent
*/

#define			LK_E_WAIT	    0x01
#define			LK_E_SET	    0x02
#define			LK_E_CLR	    0x04
#define			LK_E_INTERRUPT	    0x08
#define			LK_E_CROSS_PROCESS  0x10

/*
**  LKnode constants.
*/

#define			LK_SELF		1
#define			LK_NODE		2
#define			LK_NRELEASE	3

/*
**  LKshow and LKalter item codes.
*/

#define			LK_I_LLB	     1L
#define			LK_I_BLK	     2L
#define			LK_I_LKH	     3L
#define			LK_I_RSH	     4L
#define			LK_S_ORPHANED	     5L
#define			LK_S_STAT	     6L
#define			LK_A_CSPID	     7L
#define			LK_S_RESOURCE	     8L
#define			LK_S_LOCKS	     9L
#define			LK_A_DLK_HANDLER    10L
#define			LK_A_CLR_DLK_SRCH   11L
#define			LK_I_MAX_LKB	    12L
#define			LK_A_STALL	    13L
#define			LK_A_NOINTERRUPT    14L
#define			LK_S_OWNER	    15L
#define			LK_A_CSID	    16L
#define			LK_S_TRAN_LOCKS	    17L
#define			LK_S_REL_TRAN_LOCKS 18L
#define			LK_S_LLB_INFO	    19L
#define			LK_S_SLOCK	    20L
#define			LK_S_SRESOURCE	    21L
#define			LK_S_LIST	    22L
#define			LK_A_AST_CLR	    23L
#define			LK_S_SUMM	    24L
#define			LK_A_INTERRUPT	    25L
#define 		LK_S_KEY_INFO	    26L
#define			LK_A_STALL_LOGICAL_LOCKS    27L
#define			LK_A_STALL_NO_LOCKS	    28L
#define			LK_I_RSB	    29L
#define			LK_A_CSP_ENTER_DEBUGGER	    30L
#define			LK_S_LIST_LOCKS	    31L
#define			LK_S_MUTEX	    32L
#define                 LK_S_XSVDB          33L
#define			LK_S_SLOCKQ	    34L
#define                 LK_S_OWNER_GRANT    35L
#define                 LK_S_LIST_NAME      36L
#define                 LK_A_SHOW_DEADLOCKS 37L
#define                 LK_A_REASSOC 	    38L
#define                 LK_A_ENABLE_DIST_DEADLOCK   39L
#define			LK_S_ULOCKS	    40L
#define			LK_A_FORCEABORT     41L
#define			LK_A_NOFORCEABORT   42L
#define			LK_S_DIRTY	    0x0100  /* LKshow() dirty-read structures */

/*
**  LK return status codes.
*/

#define                 LK_OK           0L
#define			LK_BUSY		(E_CL_MASK + E_LK_MASK + 0x01)
#define			LK_TIMEOUT	(E_CL_MASK + E_LK_MASK + 0x02)
#define			LK_BADPARAM	(E_CL_MASK + E_LK_MASK + 0x03)
#define			LK_DEADLOCK	(E_CL_MASK + E_LK_MASK + 0x04)
#define			LK_NOLOCKS	(E_CL_MASK + E_LK_MASK + 0x05)
#define			LK_UNEXPECTED	(E_CL_MASK + E_LK_MASK + 0x06)
#define			LK_NEW_LOCK	(E_CL_MASK + E_LK_MASK + 0x07)
#define			LK_VAL_NOTVALID	(E_CL_MASK + E_LK_MASK + 0x08)
/*
** Some unix systems define an "LK_RETRY" in their system-specific header
** files. To make things easier on those systems, we have agreed to this
** "trick" in lk.h which redefines LK_RETRY in such a way that mainline
** code can continue to use LK_RETRY but it will always get the LK_RETRY
** which is defined here (rather than the unix-system-specific one). Some
** compilers complain if you undef a symbol which isn't yet defined, so
** only undef LK_RETRY if it's already defined at this point.
*/
#define			II_LK_RETRY	(E_CL_MASK + E_LK_MASK + 0x09)
#ifdef LK_RETRY
#undef			LK_RETRY
#endif
#define			LK_RETRY	II_LK_RETRY
#define			LK_INTERRUPT	(E_CL_MASK + E_LK_MASK + 0x0a)

/* the following are used internally by the unix implementation of the lk
** system, and may not be defined on all systems.
*/

#define			LK_GRANTED	(E_CL_MASK + E_LK_MASK + 0x0b)
#define			LK_NORMAL	(E_CL_MASK + E_LK_MASK + 0x0c)
#define			LK_CVTNOTGRANT	(E_CL_MASK + E_LK_MASK + 0x0d)
#define			LK_NOTGRANTED	(E_CL_MASK + E_LK_MASK + 0x0e)
#define			LK_CANCEL	(E_CL_MASK + E_LK_MASK + 0x0f)
#define			LK_NOINTRWAIT	(E_CL_MASK + E_LK_MASK + 0x10)
#define			LK_BADMUTEX	(E_CL_MASK + E_LK_MASK + 0x11)

/*
** new generic return indicates lock was granted and an interrupt was
** received. The usual case is a FORCE_ABORT interrupt was delivered.
*/
#define			LK_INTR_GRANT	(E_CL_MASK + E_LK_MASK + 0x12)

/*
** Add LK_NOSTATUS which was present in 6.4
** This status is passed to LK_wakeup to indicate that the current
** status in llb->llb_async_status should not be updated
*/
#define			LK_NOSTATUS	(E_CL_MASK + E_LK_MASK + 0x13)

/*
** LK_PHANTOM is used internally by LK_request() to indicate that
** a lock request that should be waiting (suspended) has been resumed
** prematurely, possibly by an non_LK facility. If detected, the pending
** request is cancelled and retried, which is better thank locking up
** the installation with an unusable LLB.
*/
#define			LK_PHANTOM	(E_CL_MASK + E_LK_MASK + 0x14)

/*
** LK_UBUSY is returned when a lock is blocked and the lock request
** has specified a timeout of DMC_C_NOWAIT (e.g. SET LOCKMODE ...
** TIMEOUT = NOWAIT). It differentiates these kind of "busys" from
** anticipated internal "busys", LK_BUSY.
*/
#define			LK_UBUSY	(E_CL_MASK + E_LK_MASK + 0x15)

/*
** Possible under VMS clusters only.  This remaps a DLM error to a distinct
** code at the LK level for better diagnostics.  In some cases this is
** a legitimate and handled condition.  This occurs when a process 
** attempts to operate of a lock which was allocated by a process which
** is no longer running.
*/
#define			LK_NOTHELD	(E_CL_MASK + E_LK_MASK + 0x16)

/* LKrequest abandoned, must be redone */
#define			LK_REDO_REQUEST	(E_CL_MASK + E_LK_MASK + 0x17)

/* LKrequest abandoned, session has been flagged for FORCE ABORT.
** and may enter into a LOG/LOCK deadlock.
*/
#define			LK_INTR_FA      (E_CL_MASK + E_LK_MASK + 0x18)

#define E_CL1020_LK_BUSY_ALTER         (E_CL_MASK + E_LK_MASK + 0x20)
#define E_CL1021_LK_LLB_ALLOC_FAILED   (E_CL_MASK + E_LK_MASK + 0x21)
#define E_CL1022_LK_LOCK_ALLOC_FAILED  (E_CL_MASK + E_LK_MASK + 0x22)
#define E_CL1023_LK_LKH_ALLOC_FAILED   (E_CL_MASK + E_LK_MASK + 0x23)
#define E_CL1024_LK_RSH_ALLOC_FAILED   (E_CL_MASK + E_LK_MASK + 0x24)
#define E_CL1025_LK_NOINTERRUPT_ALTER  (E_CL_MASK + E_LK_MASK + 0x25)
#define E_CL1026_LK_NOINTERRUPT_ALTER  (E_CL_MASK + E_LK_MASK + 0x26)
#define E_CL1027_LK_BAD_ALTER_REQUEST  (E_CL_MASK + E_LK_MASK + 0x27)
#define E_CL1028_LK_CANCEL_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x28)
#define E_CL1029_LK_CANCEL_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x29)
#define E_CL102A_LK_CREATE_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x2A)
#define E_CL102B_LK_CREATE_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x2B)
#define E_CL102C_LK_CREATE_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x2C)
#define E_CL102D_LK_EXPAND_LIST_FAILED (E_CL_MASK + E_LK_MASK + 0x2D)
#define E_CL102E_LK_EXPAND_LIST_FAILED (E_CL_MASK + E_LK_MASK + 0x2E)
#define E_CL102F_LK_EXPAND_LIST_FAILED (E_CL_MASK + E_LK_MASK + 0x2F)
#define E_CL1030_LK_EXPAND_LIST_FAILED (E_CL_MASK + E_LK_MASK + 0x30)
#define E_CL1031_LK_BAD_UNIQUE_ID      (E_CL_MASK + E_LK_MASK + 0x31)
#define E_CL1032_LK_DUPLICATE_LOCK_ID  (E_CL_MASK + E_LK_MASK + 0x32)
#define E_CL1033_LK_EVENT_BAD_PARAM    (E_CL_MASK + E_LK_MASK + 0x33)
#define E_CL1034_LK_EVENT_BAD_PARAM    (E_CL_MASK + E_LK_MASK + 0x34)
#define E_CL1035_LK_EVENT_BAD_PARAM    (E_CL_MASK + E_LK_MASK + 0x35)
#define E_CL1036_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x36)
#define E_CL1037_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x37)
#define E_CL1038_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x38)
#define E_CL1039_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x39)
#define E_CL103A_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3A)
#define E_CL103B_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3B)
#define E_CL103C_LK_RELEASE_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3C)
#define E_CL103D_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3D)
#define E_CL103E_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3E)
#define E_CL103F_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x3F)
#define E_CL1040_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x40)
#define E_CL1041_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x41)
#define E_CL1042_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x42)
#define E_CL1043_LK_SHOW_BAD_PARAM     (E_CL_MASK + E_LK_MASK + 0x43)
#define E_CL1044_LK_SHOW_BAD_PARAM     (E_CL_MASK + E_LK_MASK + 0x44)
#define E_CL1045_LK_SHOW_BAD_PARAM     (E_CL_MASK + E_LK_MASK + 0x45)
#define E_CL1046_LK_SHOW_BAD_PARAM     (E_CL_MASK + E_LK_MASK + 0x46)
#define E_CL1047_LK_SHOW_BAD_PARAM     (E_CL_MASK + E_LK_MASK + 0x47)
#define E_CL1048_LK_CANCEL_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x48)
#define E_CL1049_LK_CANCEL_BADPARAM    (E_CL_MASK + E_LK_MASK + 0x49)
#define E_CL104A_LK_REQUEST_BAD_PARAM  (E_CL_MASK + E_LK_MASK + 0x4A)
#define E_CL1050_LK_LOCKID_NOTFOUND    (E_CL_MASK + E_LK_MASK + 0x50)
#define E_CL1051_LK_DUPLICATE_LOCK_ID  (E_CL_MASK + E_LK_MASK + 0x51)
#define E_CL1052_LK_RELEASE_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x52)
#define E_CL1053_LK_RELEASE_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x53)
#define E_CL1054_LK_REQUEST_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x54)
#define E_CL1055_LK_REQUEST_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x55)
#define E_CL1056_LK_SEM_OWNER_DEAD     (E_CL_MASK + E_LK_MASK + 0x56)
#define E_CL1058_LK_REQUEST_RETRIED    (E_CL_MASK + E_LK_MASK + 0x58)
#define E_CL1060_LK_CBACK_REG_FAILED   (E_CL_MASK + E_LK_MASK + 0x60)
#define E_CL1061_LK_CBACK_THREAD_DEAD  (E_CL_MASK + E_LK_MASK + 0x61)
#define E_CL1062_LK_CBACK_GET_FAILED   (E_CL_MASK + E_LK_MASK + 0x62)
#define E_CL1063_LK_CBACK_GET_BADPARAM (E_CL_MASK + E_LK_MASK + 0x63)
#define E_CL1064_LK_CBACK_GET_BADCBACK (E_CL_MASK + E_LK_MASK + 0x64)
#define E_CL1065_LK_CBACK_PUT_BADCBT   (E_CL_MASK + E_LK_MASK + 0x65)
#define E_CL1066_LK_CBACK_PUT_FAILED   (E_CL_MASK + E_LK_MASK + 0x66)
#define E_CL1067_LK_CBACK_FIRE_BADRSB  (E_CL_MASK + E_LK_MASK + 0x67)
#define E_CL1068_LK_CBACK_FIRE_FAILED  (E_CL_MASK + E_LK_MASK + 0x68)
#define E_CL1069_LK_CBACK_INIT_FAILED  (E_CL_MASK + E_LK_MASK + 0x69)
#define E_CL106A_LK_CBACK_REG_BADCBT   (E_CL_MASK + E_LK_MASK + 0x6A)

#define LK_COV_LOCK                    (E_CL_MASK + E_LK_MASK + 0x6B)
#define LK_LOG_LOCK                    (E_CL_MASK + E_LK_MASK + 0x6C)

#define E_CL106D_LK_CONNECT_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x6D)
#define E_CL106E_LK_CONNECT_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x6E)
#define E_CL106F_LK_CONNECT_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x6F)
#define E_CL1070_LK_CONNECT_BADPARAM   (E_CL_MASK + E_LK_MASK + 0x70)
#define E_CL1071_LK_FORCEABORT_ALTER   (E_CL_MASK + E_LK_MASK + 0x71)
#define E_CL1072_LK_FORCEABORT_ALTER   (E_CL_MASK + E_LK_MASK + 0x72)

/*}
** Name: LK_LKID - The type of a lock id.
**
** Description:
**	This is the format of a lockid returned by LKrequest.
**
** History:
**      18-oct-1985 (derek)
**          Created.
*/
typedef struct
{
    u_i4		lk_unique;
    u_i4		lk_common;
}   LK_LKID;
/*} 
** Name: LKcallback - Prototype for LK Blocking Callback Functions. 
**  
** Description: 
**      This is the prototype for blocking callbacks.
** 
** History: 
**      01-aug-1996 (nanpr01 for ICL keving)
**          Created.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Added lock_key parm.
*/ 
typedef STATUS (*LKcallback)(i4 block_mode, PTR callback_arg, LK_LKID *lockid,
				LK_LOCK_KEY *lock_key);

/*}
** Name: LK_LLID - A lock list identifier.
**
** Description:
**	This is the format of a lock list identifier returned by
**	the LKcreate_list service.
**
** History:
**      18-oct-1985 (derek)
**          Created.
*/
typedef u_i4	    LK_LLID;

/*}
** Name: LK_LOCK_KEY - Template for a lock key.
**
** Description:
**	This structure defines the format of a key expected by the lock
**	routines.  There is minimal intepretation of the key by the locking
**	routines.
**
** History:
**     17-sep-1985 (derek)
**          Created.
**     21-jan-1989 (EdHsu)
**	    Added LK type LK_CKP_DB to support online backup.
**	16-aug-1991 (ralph)
**	    Added LK type LK_EVCONNECT to support the event subsystem.
**      09-oct-1992 (jnash)
**          Reduced logging project.  Add LK_ROW lock type, at present 
**	    used only for system catalog extend operations.
**	20-oct-1992 (markg)
**	    Add new lock type - LK_AUDIT, used by the SXF auditing subsystem.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency, use LK_KEY_LENGTH instead of
**	    DB_MAXNAME in this LK object
**      19-apr-1995 (nick)
**          Added LK_CKP_TXN to fix deadlock problem in online backup.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add LK_PH_PAGE lock type to protect physical stucture of a page
**          Add LK_VAL_LOCK lock type for phantom protection on hash table.
**	06-feb-2001 (devjo01)
**	    Add LK_CSP, LK_CMO, LK_MSG.
**	16-jan-2004 (thaju02)
**	    Add LK_ONLN_MDFY for online modify.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add LK_CROW (Consistent read ROW),
**	    LK_TBL_MVCC, LK_MAX_TYPE define.
*/
struct _LK_LOCK_KEY
{
    i4		    lk_type;                    /* The type of lock. */
#define			LK_DATABASE		1
#define			LK_TABLE		2
#define			LK_PAGE			3
#define			LK_EXTEND_FILE		4
#define			LK_BM_PAGE		5
#define                 LK_CREATE_TABLE         6
#define                 LK_OWNER_ID             7
#define                 LK_CONFIG               8
#define                 LK_DB_TEMP_ID           9
#define			LK_SV_DATABASE		10
#define			LK_SV_TABLE		11
#define			LK_SS_EVENT		12
#define                 LK_TBL_CONTROL		13
#define			LK_JOURNAL		14
#define                 LK_OPEN_DB		15
#define			LK_CKP_DB		16
#define			LK_CKP_CLUSTER		17
#define                 LK_BM_LOCK		18
#define                 LK_BM_DATABASE		19
#define                 LK_BM_TABLE		20
#define			LK_CONTROL		21
#define			LK_EVCONNECT		22
#define			LK_AUDIT		23
#define			LK_ROW			24
#define                 LK_CKP_TXN              25
#define                 LK_PH_PAGE              26
#define			LK_VAL_LOCK		27
#define			LK_SEQUENCE		28
#define			LK_XA_CONTROL		29
#define			LK_CSP			30
#define			LK_CMO			31
#define			LK_MSG			32
#define			LK_ONLN_MDFY		33
#define			LK_CROW			34
#define			LK_TBL_MVCC		35
/* Kindly maintain... */
#define			LK_MAX_TYPE		35

#define LK_LOCK_TYPE_MEANING \
/*  0 -  10 */ "0,DATABASE,TABLE,PAGE,EXTEND,BM_PAGE,CREATE,OWNER,CONFIG,DB_TEMP_ID,SV_DATABASE," \
/* 11 -  20 */ "SV_TABLE,SS_EVENT,TBL_CONTROL,JOURNAL,OPEN_DB,CKP_DB,CKP_CLUSTER,BM_LOCK,BM_DATABASE,BM_TABLE," \
/* 21 -  30 */ "CONTROL,EVCONNECT,AUDIT,ROW,CKP_TXN,PH_PAGE,VAL_LOCK,SEQUENCE,XA_CONTROL,CSP," \
/* 31 -  40 */ "CMO,MSG,ONLN_MDFY,CROW,TBL_MVCC"

    i4		    lk_key1;
    i4		    lk_key2;
    i4		    lk_key3;
    i4		    lk_key4;
    i4              lk_key5;
    i4		    lk_key6;
#define		    LK_KEY_LENGTH		24
/* length of the above definition 6 * sizeof (i4) */
};

/*}
** Name: LK_UNIQUE - The structure of a unique identifier.
**
** Description:
**      This structure defines what a unique identifier looks like.
**	A caller defined unique identifer can not specify an lk_uhigh
**	value of 0.  This is reserved by the LK routines.
**
** History:
**     18-oct-1985 (derek)
**          Created new for 5.0
*/
struct _LK_UNIQUE
{
    u_i4            lk_uhigh;            /* Most significant part. */
    u_i4	    lk_ulow;	 	 /* Least significant part. */
};

/*}
** Name: LK_VALUE - The structure of a lock value.
**
** Description:
**      This structure defines what a lock value looks like.
**
** History:
**     18-oct-1985 (derek)
**          Created new for 5.0
*/
struct _LK_VALUE
{
    u_i4            lk_high;            /* Most significant part. */
    u_i4	    lk_low;		/* Least significant part. */
    i4		    lk_mode;		/* The granted mode of the lock. */
};

/*}
** Name: LK_EVENT - The structure of a lock event.
**
** Description:
**      This structure defines what a lock event looks like.
**
** History:
**	20-Jul-1998 (jenjo02)
**	    Created.
*/
struct _LK_EVENT
{
    u_i4	    type_high;          /* Most significant type piece */
    u_i4	    type_low;		/* Least significant type piece */
    u_i4	    value;		/* Event value */
};

/*}
** Name: LK_STAT - Locking statistics return structure
**
** Description:
**      This structure is returned by LKshow.
**
** History:
**      05-mar-1986 (Derek)
**          Created.
**	24-may-1993 (bryanp)
**	    Added lots of new statistics for measing locking system behavior.
**	07-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	    Changed all stats to u_i4.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add tstat, stats by lock type.
[@history_template@]...
*/
struct _LK_STAT
{
    u_i4         create;             /* Create lock list. */
    u_i4	    request;		/* New lock request. */
    u_i4	    re_request;		/* Re-request same lock. */
    u_i4	    convert;		/* Explicit conversions. */
    u_i4	    release;		/* Explicit releases. */
    u_i4	    escalate;		/* Partial releases. */
    u_i4	    destroy;		/* Release lock list. */
    u_i4	    wait;		/* Requests that waited. */
    u_i4	    con_wait;		/* Converts that waited. */
    u_i4	    con_search;		/* Convert deadlock search. */
    u_i4	    con_dead;		/* Convert deadlock. */
    u_i4	    dead_search;	/* Deadlock searchs. */
    u_i4	    deadlock;		/* Deadlocks. */
    u_i4	    cancel;		/* cancels. */
    u_i4	    enq;		/* enq requests. */
    u_i4	    deq;		/* deq requests. */
    u_i4	    gdlck_search;	/* # of pending global deadlock 
					** search requests. 
					*/
    u_i4	    gdeadlock;		/* # of global deadlock detected. */
    u_i4	    gdlck_grant;	/* # of global locks grant before the
					** the global deadlock search.
					*/
    u_i4	    totl_gdlck_search;	/* # of global deadlock search requests. */
    u_i4	    gdlck_call;		/* # of global deadlock search calls. */
    u_i4	    gdlck_sent;		/* # of global deadlock messages sent. */
    u_i4	    cnt_gdlck_call;	/* # of continue deadlock search calls. */
    u_i4	    cnt_gdlck_sent;	/* # of continue deadlock message sent. */
    u_i4	    unsent_gdlck_search; /* # of unsent deadlock search requests. */
    u_i4	    sent_all;		/* # of sent all deadlock search request. */
    u_i4	    synch_complete;	/* # of SS$_SYNCH completions. */
    u_i4	    asynch_complete;	/* # of enq_complete completions */
    u_i4	    csp_msgs_rcvd;	/* # of msgs received by CSP so far */
    u_i4	    csp_wakeups_sent;	/* # of LK_wakeup calls made by CSP */
    u_i4	    allocate_cb;	/* # of allocate_cb calls */
    u_i4	    deallocate_cb;	/* # of deallocate_cb calls */
    u_i4	    sbk_highwater;	/* highwater mark of SBK allocation */
    u_i4	    lbk_highwater;	/* highwater mark of LBK allocation */
    u_i4	    rbk_highwater;	/* highwater mark of RBK allocation */
    u_i4	    rsb_highwater;	/* highwater mark of RSB usage */
    u_i4	    lkb_highwater;	/* highwater mark of LKB usage */
    u_i4	    llb_highwater;	/* highwater mark of LLB usage */
    u_i4	    max_lcl_dlk_srch;	/* max length of a deadlocksearch */
    u_i4	    dlock_locks_examined;/* number of lkb's seen by deadlock */
    u_i4	    max_rsrc_chain_len; /* max length of a RSB chain search */
    u_i4	    max_lock_chain_len; /* max length of a LKB chain search */
    u_i4	    dlock_wakeups;	/* number of DeadlockDetector wakeups */
    u_i4	    dlock_max_q_len;	/* max length of deadlock LLB queue */
    u_i4	    cback_wakeups;	/* # of callback thread wakeups */
    u_i4	    cback_cbacks;	/* # of callback invocations */
    u_i4	    cback_stale;	/* # of stale or ignored callbacks */
    u_i4	    max_lkb_per_txn;	/* Max # LKBs in a transaction */
    struct _LK_TSTAT
    {
        u_i4	    request_new;	/* # of request new lock calls */
        u_i4	    request_convert;	/* # of request with implied conversion */
        u_i4	    convert;		/* # of explicit convert requests */
        u_i4	    release;		/* # of single releases */
        u_i4	    wait;		/* # of requests that waited */
        u_i4	    convert_wait;	/* # of converts that waited */
        u_i4	    deadlock;		/* # of requests that deadlocked */
        u_i4	    convert_deadlock;	/* # of converts that deadlocked */
    }		    tstat[LK_MAX_TYPE+1];
};

/*}
** Name: LK_LLB_INFO - Lock list information.
**
** Description:
**      This structure is used to return information about lock lists.
**
** History:
**      16-jun-1987 (Derek)
**          Created.
**      03-Jul-1987 (ac)
**          Added maximum number of locks per lock list support.
**	24-may-1993 (bryanp)
**	    Added llb_pid to return the Process ID which created this locklist.
**	21-jun-1993 (bryanp)
**	    Added llb_status definition for LLB_INFO_NONPROTECT for lockstat.
**	26-jul-1993 (bryanp)
**	    Added event-waiting info: llb_event_type, llb_event_value,
**		llb_evflags. In general, these fields are only interesting if
**		LLB_INFO_EWAIT is on in the llb_status.
**	31-jan-1994 (mikem)
**	    Bug #58407.
**	    Added CS_SID info to the LK_LLB_INFO structure: llb_sid.  This is 
**	    used by lockstat and internal lockstat type info logging to print 
**	    out session owning locklist. 
**	    Given a pid and session id (usually available from errlog.log 
**	    traces), one can now track down what locks are associated with
**	    that session.
**	20-Jul-1998 (jenjo02)
**	    llb_event_type, llb_event_value changed to LK_EVENT structure.
**	30-Nov-1999 (jenjo02)
**	    Added llb_sllb_id, llb_s_cnt, llb_dis_id, brought LLB_INFO
**	    defines up to date.
**	15-Dec-1999 (jenjo02)
**	    Deleted llb_dis_id, no longer needed.
*/
struct _LK_LLB_INFO
{
    i4         	llb_id;             	/* Internal identifier. */
    i4	    	llb_status;		/* Lock list status. */
#define                 LLB_INFO_WAITING	    0x00001L
#define			LLB_INFO_NONPROTECT	    0x00002L
#define			LLB_INFO_ORPHANED	    0x00004L
#define			LLB_INFO_EWAIT		    0x00008L
#define			LLB_INFO_RECOVER	    0x00010L
#define			LLB_INFO_MASTER		    0x00020L
#define			LLB_INFO_ESET		    0x00040L
#define			LLB_INFO_EDONE		    0x00080L
#define			LLB_INFO_NOINTERRUPT	    0x00100L
#define                 LLB_INFO_SHARED		    0x00200L
#define                 LLB_INFO_PARENT_SHARED	    0x00400L
#define			LLB_INFO_ENQWAITING	    0x00800L
#define			LLB_INFO_GDLK_SRCH	    0x01000L
#define			LLB_INFO_ST_ENQ	    	    0x02000L
#define			LLB_INFO_MASTER_CSP    	    0x04000L
#define			LLB_INFO_XSVDB    	    0x20000L
#define			LLB_INFO_XROW    	    0x40000L
    i4	    	llb_key[2];		/* Lock list key. */
    i4	    	llb_r_llb_id;		/* Related lock list id. */
    i4	    	llb_r_cnt;		/* Related lock list count. */
    i4	    	llb_s_llb_id;		/* SHARED lock list id. */
    i4	    	llb_s_cnt;		/* SHARED lock list count. */
    i4	    	llb_lkb_count;		/* Number of locks held. */
    i4	    	llb_llkb_count;		/* Number of logical locks held. */
    i4	    	llb_max_lkb;		/* Maximum number of logical locks 
					** allowed. */
    i4	    	llb_wait_rsb_id;	/* Wait RSB id. */
    i4	    	llb_tick;		/* The current clock tick when making
					** a global deadlock search request.
					*/
    PID		llb_pid;		/* which process created this lock list
					*/
    CS_SID	llb_sid;		/* which session created this lock list
					*/
    LK_EVENT	llb_event;
    i4	    	llb_evflags;
};

/*}
** Name: LK_LKB_INFO - Information about locks.
**
** Description:
**      This structure returns information about a specific lock request.
**
**
** History:
**      16-jun-1987 (Derek)
**          Created.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter.
**	19-jul-2001 (devjo01)
**	    Changed lkb_vms_lkid, and lkb_vms_lkvalue, to lkb_dlm_lkid,
**	    and lkb_dlm_value, and changed types to be conformant with
**	    CX expectations.
**	
[@history_template@]...
*/
struct _LK_LKB_INFO
{
    i4		    lkb_id;             /* Internal identifier. */
    i4		    lkb_rsb_id;		/* Resource identifier. */
    i4		    lkb_llb_id;		/* Lock list identifier. */
    PID	    	    lkb_pid;		/* Process owning this lock */
    CS_SID	    lkb_sid;		/* Session owning this lock */
    i4		    lkb_phys_cnt;	/* Physical lock count. */
    i2		    lkb_grant_mode;	/* Lock grant mode. */
    i2		    lkb_request_mode;	/* Lock convert mode. */
    i4		    lkb_state;		/* Lock state. */
    i4		    lkb_flags;		/* Lock flags. */
    i4		    lkb_key[7];		/* Lock key. */
    LK_UNIQUE	    lkb_dlm_lkid;	/* DLM lock id. */
    i4		    lkb_dlm_lkvalue[4];	/* DLM lock value. */
};

/*}
** Name: LK_RSB_INFO - Information about a resource.
**
** Description:
**      This structure returns information about a lock resource.
**
** History:
**      16-jun-1987 (Derek)
**          Created.
**	20-mar-1989 (rogerk)
**	    Added rsb_count field to return the number of holders of a resource.
**      12-feb-1996 (ICL keving) 
**          Added rsb_cback_count to return the number of LKBs with callbacks on
**          the granted queue. 
[@history_template@]...
*/
struct _LK_RSB_INFO
{
    i4         rsb_id;             /* Internal identifier. */
    i4	    rsb_grant;		/* Group grant mode. */
    i4	    rsb_convert;	/* Convert group mode. */
    i4	    rsb_key[7];		/* Lock key. */
    i4	    rsb_value[2];	/* Lock value. */
    i4	    rsb_invalid;	/* The status of the lock value. */
    i4	    rsb_count;		/* # of granted requests on resource. */
    i4         rsb_cback_count;    /* # of granted requests with callback */
};

/*}
** Name: LK_S_LOCKS_HEADER - LK_S_LOCK buffer layout.
**       
** Description:
**	This structure explicitly lays out the organization of the
**	data returned by an LKshow LK_S_LOCK call.   Depending on
**	the actual size of the buffer passed to LKshow, more or
**	less array elements will be present.
**
** History:
**	04-dec-2001 (devjo01)
**	    Added, so that Tru64 compiler will not be tricked into
**	    performing aligned operations on unaligned memory.
*/
struct _LK_S_LOCKS_HEADER 
{
	LK_LLB_INFO	llbi;		/* Lock list info (1 instance) */
	LK_LKB_INFO	lkbi[1];	/* Lock info (1st element of array */
};
/*}
** Name: LK_S_RESOURCE_HEADER - LK_S_LOCK buffer layout.
**       
** Description:
**	This structure explicitly lays out the organization of the
**	data returned by an LKshow LK_S_RESOURCE call.   Depending on
**	the actual size of the buffer passed to LKshow, more or
**	less array elements will be present.
**
** History:
**	04-dec-2001 (devjo01)
**	    Added, so that Tru64 compiler will not be tricked into
**	    performing aligned operations on unaligned memory.
*/
struct _LK_S_RESOURCE_HEADER 
{
	LK_RSB_INFO	rsbi;		/* Lock resource info (1 instance) */
	LK_LKB_INFO	lkbi[1];	/* Lock info (1st element of array */
};


/*}
** Name: LK_DSM - LK cluster deadlock search message block.
**
** Description:
**      This structure contains LK cluster deadlock search message block. 
**	It is used to transfer deadlock search information to/from CSP
**	for performing global deadlock detection in a cluster environmemt. 
**
** History:
**      24-Jun-1987 (ac)
**          Created.
**	20-Aug-2002 (jenjo02)
**	    Added lkb_id to LKDSM.
**	24-oct-2003 (devjo01)
**	    Promote lockid from u_i4 to LK_UNIQUE.
**	13-may-2005 (devjo01)
**	    - Rename from LKDSM to LK_DSM.
**	    - Rename lkb_id to o_lkb_id, and lockid to o_lockid
**	      to highlight that these are the values from the originating
**	      lock.
**	    - Add o_blk_pid, o_bld_sid to record info about which remote
**	      holder of the original resource started the deadlock cycle.
**	18-jan-2007 (jenjo02/abbjo03)
**	    Add msg_seq for debugging.
**	22-Jan-2007 (jonj)
**	    Deleted unref'd LOCK_CONVERT define.
**	    Add lockid - distributed lockid of blocker.
**	17-May-2007 (jonj)
**	    Deleted lockid, not really useful.
**	    Change LK_UNIQUE tran_id, o_tran_id to u_i4 llb_id, o_llb_id
**	    Shrunk lock_cum_mode, lock_state from i4 to i2.
**	22-May-2007 (jonj)
**	    Add msg_hops.
**	21-Aug-2007 (jonj)
**	    Add o_lock_key, remove llb_id, add msg_search, msg_flags.
**	22-Oct-2007 (jonj)
**	    Add LK_DSM_IN_RETRY flag.
**	04-Jan-2008 (jonj)
**	    Add LK_UNIQUE lockid, the DLM lockid of the blocked
**	    lock.
**	30-Apr-2008 (jonj)
**	    Add msg_node, blk_lockid, deleted unused msg_flags,
*/
struct _LK_DSM
{
	/* Working info re one hop in the search */
	LK_LOCK_KEY	    lock_key;	    /* Blocked resource name. */
	LK_UNIQUE	    lockid;	    /* Blocked dist lockid. */
	i2		    lock_cum_mode;  /* Cummulative request mode
	                                    ** for this request, and all
					    ** requests ahead of it on 
					    ** the resource. */
	i2		    lock_state;	    /* The state of the lock. */
	i4		    msg_node;	    /* Whence this msg came from */
	/* Info about the lock which triggered the search */
	LK_LOCK_KEY	    o_lock_key;	    /* Blocked resource name. */
	u_i4	    	    o_llb_id;	    /* The original waiting lock list  
					    ** that started the deadlock 
					    ** search. */
	u_i4		    o_lkb_id;	    /* LKB id of blocked lock. */
	LK_UNIQUE    	    o_lockid;	    /* Its distributed lockid. */
	u_i4	    	    o_blk_lockid;   /* Initial blocking lockid */
	i4		    o_node;         /* The cluster node id of that 
					    ** lock list.
					    */
	u_i4		    o_stamp;	    /* The request_id of original 
					    ** lock list that started the
					    ** deadlock search. 
					    */
	i4		    o_blk_node;	    /* Ingres node ID of blocker */
	PID		    o_blk_pid;	    /* Process ID of blocker. */
	CS_SID		    o_blk_sid;	    /* Session ID of blocker. */
	u_i4		    o_blk_gtmode;   /* The granted lock mode for 
					    ** the blocker. */
	u_i4	    	    msg_search;	    /* Local deadlock search that
					    ** initiated this message */
	u_i4		    msg_seq;	    /* Message sequence */
	u_i4		    msg_hops;	    /* Number of node-node hops */
};

/*}
** Name: LK_BLKB_INFO - Brief information about locks.
**
** Description:
**      This structure returns information about a specific lock.
**	It's members are known to mainline code so MUST NOT CHANGE.
**
** History:
**      12-Jan-1989 (ac)
**          Created.
*/
struct _LK_BLKB_INFO
{
    i4	    lkb_key[7];		/* Lock key. */
    char	    lkb_grant_mode;	/* Lock grant mode. */
    char	    lkb_attribute;	/* Lock attribute. */
#define			LOCK_PHYSICAL	0x01
};
/*}
** Name: LK_SUMMARY - Locking system summary statistics
**
** Description:
**      This structure is returned by LKshow.
**
** History:
**      21-feb-1990 (Sandyh)
**          Created.
**	24-may-1993 (bryanp)
**	    Added rsb_size now that RSB's and LKB's are managed separately.
[@history_template@]...
*/
struct _LK_SUMM
{
    i4	    lkb_size;		/* Total configured locks. */
    i4	    lkbs_per_xact;	/* Max locks per xact */
    i4	    lkb_hash_size;	/* Size of the lock hash table. */
    i4	    lkbs_inuse;		/* Locks being held. */
    i4	    rsb_hash_size;	/* Size of the resource hash table. */
    i4	    rsbs_inuse;		/* Resources being locked. */
    i4	    llb_size;		/* Max lock lists configured. */
    i4	    llbs_inuse;		/* Lock lists being used. */
    i4	    rsb_size;		/* total configured resource blocks */
};
/*}
** Name: LK_MUTEX - Locking system mutex statistics
**
** Description:
**      This structure is returned by LKshow.
**
** History:
**      06-Dec-1995 (jenjo02)
**          Created.
[@history_template@]...
*/
struct _LK_MUTEX
{
    i4	    count;		/* number of blocks returned */
#define	LK_MUTEX_MAX 16
    CS_SEM_STATS    stats[LK_MUTEX_MAX]; /* stats array */
};

/*
**  Forward and/or External function references, prototypes.
*/

FUNC_EXTERN STATUS  LKalter(
			i4                 flag,
			i4             value,
			CL_ERR_DESC         *sys_err);		
			/* Alter paramters. */

FUNC_EXTERN STATUS  LKcreate_list(
			i4                     flags,
			LK_LLID                 related_lli,
			LK_UNIQUE               *unique_id,
			LK_LLID                 *lock_list_id,
			i4                 count,
			CL_ERR_DESC             *sys_err);    
			/* Create a lock list. */

FUNC_EXTERN STATUS  LKevent(
			i4             flag,
			LK_LLID         lock_list_id,
			LK_EVENT        *event,
			CL_ERR_DESC     *sys_err);
			/* Handle processing of a request service. */

FUNC_EXTERN STATUS  LKinitialize(
			CL_ERR_DESC *sys_err,
			char *lgk_info );
			/* Initialize the LK lock database. */

FUNC_EXTERN STATUS  LKpropagate(
			i4                 flags,
			LK_LLID             lock_list_id,
			LK_LOCK_KEY         *lock_key,
			LK_LOCK_KEY         *new_lock_key,
			CL_ERR_DESC         *sys_err);
			/* Propagate locks to new resource */

FUNC_EXTERN STATUS  LKrelease(
			i4                 flags,
			LK_LLID             lock_list_id,
			LK_LKID             *lockid,
			LK_LOCK_KEY         *lock_key,
			LK_VALUE            *value,
			CL_ERR_DESC         *sys_err);        
			/* Release a lock or a list. */

FUNC_EXTERN STATUS  LKrequest(
			i4                 flags,
			LK_LLID             lock_list_id,
			LK_LOCK_KEY         *lock_key,
			u_i4               lock_mode,
			LK_VALUE            *value,
			LK_LKID             *lockid,
			i4             timeout,
			CL_ERR_DESC         *sys_err);
			/* Request a lock. */

FUNC_EXTERN STATUS  LKrequestWithCallback(
                        i4                  flags,
                        LK_LLID             lock_list_id,
                        LK_LOCK_KEY         *lock_key,
                        u_i4               lock_mode,
                        LK_VALUE            *value,
                        LK_LKID             *lockid,
                        i4             timeout,
                        LKcallback          callback_fn, 
                        PTR                 callback_arg,
                        CL_ERR_DESC         *sys_err);   
                        /* Request a lock. */ 

FUNC_EXTERN STATUS  LKshow(
			i4                 flag,
			LK_LLID             listid,
			LK_LKID             *lockid,
			LK_LOCK_KEY         *lock_key,
			u_i4	            info_size,
			PTR                 info_buffer,
			u_i4	            *info_result,
			u_i4               *context,
			CL_ERR_DESC         *sys_err);		
			/* Show lock information. */

FUNC_EXTERN char * LKkey_to_string(
                        LK_LOCK_KEY *key,
                        char *buffer);
                        /* Standard key to string formatter */

FUNC_EXTERN STATUS  LKnode(
			i4             flag,
			u_i4	    id,
			u_i4	    length,
			PTR		    *key,
			i4		    (*fcn)(),
			CL_ERR_DESC	    *sys_err);
			/* cluster node locking */

FUNC_EXTERN STATUS  LKconnect(
			LK_LLID             clock_list_id,
			LK_LLID             *lock_list_id,
			CL_ERR_DESC	    *sys_err);
			/* Connect to existing lock list */

FUNC_EXTERN STATUS LKdeadlock_thread(
			void		    *dmc_cb);

FUNC_EXTERN STATUS LKdist_deadlock_thread(
			void		    *dmc_cb);
# endif /* LK_H_INCLUDED */

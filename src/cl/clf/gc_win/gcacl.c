/*
** Copyright (c) 1995, 2010 Ingres Corporation
*/

/*
** _WIN32_WINNT may be remove when we no longer support NT 3.51
*/
# define _WIN32_WINNT 0x0400
#include <compat.h>
#include <clconfig.h>
#include <basetsd.h>
#include <er.h>
#include <cs.h>
#include <ex.h>
#include <gc.h>
#include <gcccl.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <id.h>
#include <cv.h>
#include <bsi.h>
#include <meprivate.h> /* for tls */
#include "gclocal.h"
#include "gcclocal.h"
#include "gcaclw95.h"
#include <winbase.h>
#include <mmsystem.h> /* for timeSetEvent */

/*
**  Local prototypes
**
**	- related to external functions
*/
STATUS	GCrequest2(SVC_PARMS *svc_parms);
STATUS	GCrequestSendPeer(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb_in);
VOID WINAPI GCrequestSendPeerComplete(DWORD dwError, DWORD dwNumberXfer, LPOVERLAPPED pOverlapped);
VOID	GClisten2_complete();
/*
**	- "Common" routines
*/
VOID WINAPI GCasyncCompletion(SVC_PARMS *svc_parms);
static void GCsetWaitableTimer(SVC_PARMS *svc_parms);
static void CALLBACK GCrectimeout(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 );
DWORD 	GCcreateSemaphore(ASSOC_IPC * asipc);
VOID	GCdestroySemaphore(ASSOC_IPC * asipc);
VOID	GCrpt_stats();
/*
**	- Named Pipes routines
*/
DWORD	GCconnectToServerPipe(SVC_PARMS *svc_parms);
static void GCqualifiedPipeName( char *p, char *i, char *pn );
/*
**	- Network Protocol Driver routines
*/
GCC_PCE  * GC_get_driver();
DWORD	GCDriverRegister(SVC_PARMS *svc_parms, ASSOC_IPC *asipc, ASYNC_IOCB *iocb);
VOID	GCDriverRequest(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb);
VOID	GCDriverRequest_exit(PTR ptr);
VOID	GCDriverSendPeer(SVC_PARMS *svc_parms, CONNECT *connect_block);
VOID	GCDriverSendPeer_exit(PTR ptr);
DWORD	GCDriverListen(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb);
VOID	GCDriverListen_exit(PTR ptr);
VOID	GCDriverListen2(SVC_PARMS *svc_parms);
VOID	GCDriverListen2_exit(PTR closure);

FUNC_EXTERN VOID GVshobj(char **ObjectPrefix);

static i4		GCinitialized_count = 0;
static char		*LOCALHOST = "localhost";
static USER_LABEL	*ul_root=NULL;

GLOBALREF   BOOL	is_gcn;
GLOBALREF   BOOL	is_comm_svr;
static    char		Asc_std_name[MAXPNAME];
static    i4            NamedPipeServer = FALSE;
static    char		prot_name[MAX_LOC] = "NAMED_PIPE";
static    char 		local_host[GC_HOSTNAME_MAX + 1];
static    char 		gc_user_name[GC_USERNAME_MAX + 1] = "";

/*
**  Flag for allowing/disabling direct connect access from remote machines.
*/
#define		GC_RA_DISABLED          (-1)
#define		GC_RA_UNDEFINED         0
#define		GC_RA_ENABLED           1  /* direct connects allowed */
#define		GC_RA_ALL               2  /* direct connects + II_DBMS_SERVER allowed */
static    i4		remote_access = GC_RA_UNDEFINED;

FACILITYDEF ME_TLS_KEY       gc_compl_key = 0;
static HANDLE           gc_asto_mutex = 0;

/* Event Objects */

GLOBALREF HANDLE    SyncEvent;
GLOBALREF HANDLE    GCshutdownEvent;


/******************************************************************************
**
**  Name: GCACL.C - GCA and GCN private CL routines
**
**  Description:
**
**  The routines in this file are CL since the internals will vary
**  across O.S. environments.
**
**  They provide local IPC between GCA users (clients and servers).
**
**  The routines are PRIVATE to GCA and GCN.
**  They must never be called by other
**  facilities.  Some routines make indirect use of portions of GCA
**  mainline code.
**
**  The following routines are contained in this module:
**
**      GCinitiate- perform CL level initialization
**      GCrequest - Build connection to peer entity
**      GCregister- Establish server identity and prepare for "listen"
**      GClisten  - Make server available for connection to peer entity
**      GCsend    - Send data over local IPC connection
**      GCreceive - Receive data over local IPC connection
**      GCrelease - Release connection to peer entity
**      GCdisconn - Force completion of outstanding requests
**      GCsave    - Save system-dependent data structures
**      GCrestore - Restore system-dependent data structures
**      GCpurge   - Cancel outstanding requests and send purge mark
**      GCterminate - Perform necessary local termination functions
**      GCdetect  - see if a server is still active
**      GCsync    - wait for completion of asynch event(s)
**      GCusrpwd  - authenticate a userid/password pair (no-op on NT)
**
** History:
**	??-???-?? (?????)
**	   Original.
**	17-jun-95 (emmag)
**	   Rewritten to make use of Async read and writes on the named
**	   pipes.
**	25-jun-95 (emmag)
**	   Fixed bug in GCpurge which causes the session to hang.
**	27-jun-95 (emmag)
**	   Maximum number of pipe instances should be dependent on
**	   the connect_max parameter in config.dat and not hard-coded.
**      12-jul-1995 (wonst02)
**         Add the hSignalShutdown Event
**	13-jul-95 (emmag)
**	   Zero byte messages were sometimes being lost on the pipe.
**	   Replaced the zero byte message with a fixed length message
**	   which, if detected, is treated as a chop-mark.  This was done
**	   in both GCreceive and GCCompletionRoutine.
**	   Implemented a timed read in GCreceive.
**	   Fixed up a compiler warning in the previous submission.
**	   GCerror macro put back in place now that the debugging is
**	   complete!!!!!
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**	17-jul-95 (emmag)
**	    The timed read in GCreceive was causing the copy in command
**	    to hang the terminal monitor.  Have removed this code for the
**	    first QA release and will address this issue later.
**	17-jul-1995 (shero03)
**	    If the listen is interrupted, disconnect the pipe so that
**	    the closehandle will remove the server's pipe.
**	18-jul-95 (canor01)
**	    semaphore protect memory allocation routines for MCT
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**          the proper values.
**	21-jul-95 (emmag)
**	    Removed redundant GetThreadID call from the completion routine.
**      21-jul-95 (chech02)
**          Added security level.
**	24-jul-95 (emmag)
**	    Remove the hPurgeEvent.
**	    Removed the calls to FlushFileBuffers which were causing
**	    a hang in processing iibreak manifesting itself as a hang
**	    in processing a help command, or in pressing CTRL-C in the
**	    terminal monitor when retrieving data.
**	    Renamed Roger and Steve's event to hSignalShutdown to make
**	    its purpose clearer.
**	    Any other changes missing from this revision of the file
**	    will be revisited in the near future.  They have been
**	    removed because they caused the server/frontends to hang.
**	25-jul-95 (emmag)
**	    The chopmark that we'll receive from UNIX/VMS etc will
**	    not be our "CHOPMARK", but a zero byte read.  This was
**	    copmletely overlooked in the work I did to alter the
**	    purge message.
**      04-Aug-95 (fanra01)
**          Bug #70280 #70304
**          Replaced GC_ASSOC_FAIL with NP_IO_FAIL. This enables the higher
**          levels to handle an abnormal termination of a connection partner
**          and to free resources.
**          Previously would have left a dbms thread in CSsuspend in CS_setup.
**	10-aug-1995 (canor01)
**	    restore the FlushFileBuffers() call to GCdisconn().
**	22-aug-1995 (canor01)
**	    semaphore protect GClisten() initialization routines (MCT)
**	25-aug-1995 (emmag)
**	    GCpurge must use an overlapped structure to do the WriteFile
**	    since the pipe was created with the FILE_FLAG_OVERLAPPED bit
**	    set.
**      25-aug-1995 (lawst01,chech02)
**          If the association id in the ACB is set to -1 (no active session),
**          don't drive the completion routine.
**	30-aug-95 (emmag)
**	    Change pipe's Read Mode from message to byte.   This is because
**	    we were seeing some messages merging on the pipe in message
**	    mode, causing the system to hang.
**	30-aug-1995 (shero03)
**	    Add Read and Write counts
**	05-sep-95 (emmag)
**	    Rationalise the use of the hCompletion event.
**	06-sep-95 (emmag)
**	    Change read-mode back to message mode, since we still
**	    observe the merged messages regardless of read-mode.
**	    This is generally acknowledged as a problem on NT.
**	    The only case in which this becomes a real problem
**	    is when chop-mark is sent on the pipe.  We rely on
**	    this message not being merged with anything else.
**	    Our workaround for now is to put the CHOPMARK on
**	    the pipe and then do a FlushFileBuffers which will
**	    cause the server to wait until the CHOPMARK is read
**	    by the client and not write anything to the pipe
**	    until this happens.  This means that no other message
**	    *should* merge with the CHOPMARK on the pipe.
**	30-oct-1995 (canor01)
**	    two thread-local storage variables do not need to
**	    use thread-local storage
**	11-nov-95 (chech02)
**	    Added support for windows 95.
**	14-nov-95 (emmag)
**	    Interoperability problems with OpenROAD on the same node
**	    meant changing the architecture to use four simplex pipes
**	    in place of the two duplex pipes we were using.
**	17-nov-95 (canor01)
**	    Cleaned up code.
**	12-dec-1995 (canor01)
**	    While the server is recycling another instance of the
**	    connect pipe, the error returned may be ERROR_PIPE_BUSY
**	    or ERROR_FILE_NOT_FOUND.  Add a loop to allow the next
**	    instance to be created.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	13-dec-1995 (canor01)
**	    FASTSELECT clients expect NO_PEER on a failed GCrecvpeer().
**	21-dec-1995 (canor01)
**	    Restore the hPurgeEvent for NT to synchronize GCpurge()'s
**	    in single-threaded iigcc.
**	28-dec-1995 (emmag)
**	    Increase the wait before retrying on a busy connect pipe.
**	    Decrease the retry count since the two are related..
**	30-dec-1995 (emmag)
**	    Using the hPurgeEvent to synchronize the GCpurge's
**	    means that the call to FlushFileBuffers is no longer
**	    required.    Comms servers on both sides of the connection
**	    and the DBMS server were 'NamedPipeServers' thus causing both
**	    the comms server and the DBMS server to hang waiting for the
**	    pipe to be read.
**	30-dec-1995 (canor01)
**	    Add a SleepEx to the end of GCrequest to give the handshaking
**	    I/O a chance to complete.
**	02-jan-1996 (canor01)
**	    Remove the SleepEx call and change the WriteFileEx() in GCrequest()
**	    to a WriteFile() and wait for its completion.
**	04-jan-1996 (canor01)
**	    Re-add the call to FlushFileBuffers() if we are in server
**	    class "INGRES" or "STAR".
**      05-jan-96 (chech02)
**          Re-add WaitForSingleObject(SyncEvent,..) in GCsync() for windows95.
**	14-jan-96 (tutto01)
**	    Re-add the timed receive code in GCreceive().
**	14-jan-96 (emmag)
**	    Remove Todd's highlights inadvertently left in last submission.
**      23-Jan-96 (fanra01)
**          Modifed calls to CSget_scb to a macro which checks to see if this
**          is a client or server.  The scb is NULL on a client and all
**          subsequent actions to the scb are no-op'ed.
**	02-feb-1996 (canor01)
**	    Add SyncEvent to signal synchronous I/O completion for NT.
**	05-feb-96 (emmag)
**	    Implement username password validation.  This requires
**	    winbase.h and advapi32.lib from the MSDN WIN 32 SDK.
**      06-Feb-96 (fanra01)
**	    Integration of DLL changes.
**	14-Feb-96 (emmag)
**	    Do an async read in GCreceive if the run_sync flag isn't
**	    set.
**	19-feb-96 (emmag)
**	    Another OpenROAD compatability problem when running
**	    standalone.   Need to change our CHOPMARK to match
**	    their IIPRGMSG.
**      29-Feb-96 (fanra01)
**          Added GCF_EMBEDDED_GCC section for w95.
**	29-feb-1996 (canor01)
**	    Change pipe's Read Mode from byte to message.   This is because
**	    we were seeing some messages merging on the pipe in byte
**	    mode, causing the system to hang.
**	01-mar-96 (wooke01)
**	    Added in change to password checking for DBCS
**	    ports.  We haven't got a 3.51 Japanese to test with
**	    so need to make port compatable with 3.5 and 3.51.
**	    Made this possible by not using a different API
**	    to validate the password with.
**	14-mar-1996 (canor01)
**	    Max number of connections may be greater than 255, but
**	    max pipe instances is limited to 255.
**	26-mar-1996 (tutto01)
**	    Initialize OVERLAPPED structure.  Without this, bogus information
**	    was being supplied to a ReadFile call, causing it to fail with
**	    "ERROR_INVALID_PARAMETER".  This resulted in a listen failure by
**	    the communication server.  
**	27-mar-1996 (emmag)
**	    Introduce a delay in GCpurge for the case where we're
**	    not the named pipe server.   This is an ugly hack that is
**	    required to work around a nasty syncronization bug in STAR, 
**	    and will be removed as a matter of urgency.   This hack allows
**	    QA to continue testing STAR on NT which is a priority right now.
**     01-apr-1996 (canor01)
**	   An invalid connection in GCCompletionRoutine() can be 
**	   manifested through an assoc_id of -1 or by a svc_parms->acb 
**	   that is null.
**	03-apr-1996 (canor01)
**	    GCcreateStandardPipes(): Clean up code. Move MEfree's 
**	    to proper place.
**	04-apr-1996 (canor01)
**	    Events and semaphores created in GCinitiate should not be
**	    inherited by children.
**	22-apr-1996 (canor01)
**	    If GCcreateStandardPipes() fails, free the asipc only once
**	    and set the svc_parms->acb->lcb to null so it won't be
**	    freed again on a GCrelease.
**	26-apr-1996 (emmag)
**	    The default should be for logon failure.   This is something
**	    that will be documented in the release notes.   Also cleaned
**	    up the indentation here to make the doublebyte functionality
**	    more readable.
**	26-apr-1996 (canor01)
**	    IIGCC will call GClisten2 on failure of GCcreateStandardPipes,
**	    so we must change svc_parms->acb->lcb there for null as well.
**	07-may-1996 (canor01)
**	    In case of a broken or disconnected pipe in the callback
**	    routine, allow any other asynchronous callbacks to complete
**	    and post their statuses before returning the broken pipe error.
**	    (This avoids "Invalid state transition" errors in iigcc.)
**	15-may-1996 (canor01)
**	    Move most recent changes in NT code into the *_w95 functions.
**	23-may-1996 (canor01)
**	    Limit hPurgeEvent use to single-threaded apps, such as IIGCC.
**	23-may-1996 (canor01)
**	    ERROR_NO_DATA means the pipe is being closed, so return error.
**	07-jun-1996 (emmag)
**	    Windows '95 is incapable of doing a username/password pair
**	    validation, and in many cases there is no current user.
**	    For now, let's leave it up to the server to do the validation.
**	30-jul-96 (fanra01)
**	    Since Windows '95 uses anonymous pipes, remove the pipe
**	    prefix for OpenROAD compatability.
**	23-sep-1996 (canor01)
**	    Move global data definitions to gcdata.c
**      10-Oct-96 (fanra01)
**          Added mutex protection for receive completion in Windows 95 on
**          a per connection basis.  The nameed mutex is based on the pid and
**          the assoc_id is created during request or listen and is destroyed
**          during release.
**      12-Nov-96 (fanra01)
**          Removed the DiconnectNamedPipe calls from GCdisconn.  Diconnect now
**          only closes the handles as in Windows 95 case.  DisconnectNamedPipe
**          causes all pipe handle instances to be invalid even if they have
**          been passed on to children.
**	21-nov-1996 (canor01)
**	    Flush the buffers on the pipe before sending the purge message
**	    to ensure it doesn't get lost on the end of a previous message.
**	02-dec-96 (mcgem01)
**	    Writing beyond the end of the haReceiveMutex caused an access
**	    violation in the STAR SEP tests.   Increase the size of the
**	    array.  Also removed unneeded define.
**	02-dec-1996 (canor01)
**	    Instead of using the server class as the basis of issuing a
**	    FlushFileBuffers(), since this can cause deadlocks with Star,
**	    determine the direction the connection was opened in via the
**	    gca_aux_data.
**	12-dec-96 (mcgem01)
**	    Add a timeout value to GCsync.
**	09-jan-97 (mcgem01)
**	    Remove the PurgeEvent, which is no longer required on NT.
**	18-jan-97 (mcgem01)
**	    Remove the PurgeEvent in it's entirity.
**	20-jan-97 (mcgem01)
**	    The PurgeEvent and all of it's assoicated delays are
**	    required by OpenIngres Deathtrap.
**	22-jan-97 (mcgem01)
**	    There's no need to flush the channel before purging 
**	    on '95. Flushing the channel twice causes a hang in
**	    the disconnect of the iicopy app on Windows '95.
**	30-Jan-97 (kitch01)
**	    Bug 79293 2 concurrent apps will hang during purging if both set 
**	    hPurgeEvent then both waitfor event. This is because the first app
**	    to wake up after the waitfor will reset both hPurgeEvents. A further
**	    problem was found during test, GCC and the DBMS can get out of sync
**	    during GCA_PURGE. This fix cures both problems on NT only.
**	    Change PurgeEvent to a semaphore and associate this with the pid and
**	    assoc_id. The semaphore is created during request or listen and is 
**	    released during GCpurge. The semaphore is reset during GCcompletion.
**	04-feb-97 (mcgem01)
**	    During the purging on Windows '95, the server side should 
**	    flush the pipe before and after sending the purge message
**	    to ensure that it doesn't get appended or prepended to another
**	    message.
**	05-feb-97 (mcgem01 & fanra01)
**	    OpenIngres Desktop: Can't flush an anonymous pipe on 
**	    Windows '95; it returns an Invalid Handle error.  Replace 
**	    the flush  with a delay to allow the write to complete, 
**	    then write a zero byte message in an attempt to flush the 
**	    channel and then follow that by a delay to allow that write 
**	    a chance to complete.  It's nasty, but the only way around 
**	    the merged message problem on '95.
**	06-feb-97 (mcgem01)
**	    Restore NamedPipeServer logic as the OpenIngres Desktop server, 
**	    when run on NT, doesn't fill out gca_aux_data.
**	19-feb-1997 (canor01)
**	    Cross-integrate OI Desktop changes.
**	    Try to do something logical with NamedPipeServer logic that will
**	    work with both Star and Desktop.  Set the asipc->listen_side
**	    flag based on GCrequest vs. GClisten.  This flag is passed
**	    to children via GCsave/GCrestore.
**	05-mar-1997 (canor01)
**	    Do not execute GCA completion routine on ERROR_BROKEN_PIPE.
**	    This can cause AL FSM errors in GCC.
**	12-mar-1997 (canor01)
**	    Remove definition of xDEBUG left from debugging.
**	14-mar-1997 (canor01)
**	    When processing a broken expedited pipe in GCreceive, do not call
**	    the GCA completion routine.
**      17-Mar-97 (fanra01)
**          Modified GCusrpwd from exclusively searching the local account
**          database to including trusted domain account databases.
**      02-apr-97 (mcgem01)
**          Now that double-byte versions of 3.51 and 4.0 are available,
**          we can use the LogonUser API call for username/password
**          validation.
**	21-apr-1997 (canor01)
**	    If the acb has been freed and the memory released, it will
**	    not be NULL, but an invalid pointer.  Use IsBadReadPtr()
**	    to verify it in GCCompletionRoutine().
**	22-apr-1997 (canor01)
**	    Allow GCinitiate() to be called more than once, return at top
**	    of function.
**	22-apr-1997 (canor01)
**	    If II_DBMS_SERVER has been set to a fully qualified name,
**	    i.e. \\SERVER\II\INGRES\nnn, allow the connection to take
**	    place directly, without nameserver or comserver.
**	12-may-97 (mcgem01)
**	    If the connect limit hasn't been specified in config.dat
**	    set the maximum instance count for the named pipes to 
**	    PIPE_UNLIMITED_INSTANCES.
**      12-May-97 (fanra01)
**          Add the setup of rcv_data_length field on successful read.
**          Uninitialised version causing with=## to hang.
**	14-may-97 (mcgem01)
**	    SyncEvent and GCshutdownEvent are defined in gcdata.c and
**	    should have been ref'ed here.
**      02-jun-1997 (canor01)
**          Try to make sure the disconnect message is delivered before
**          pipes are closed.  Make sure all pipe failures are reported
**          back to GCA.
**      25-Jul-97 (fanra01)
**          Modified the architecture for Windows 95.  The implementation now
**          attempts to mimic asynchronous I/O and uses a Windows NT 4.0 and
**          Windows 95 specific function call QueueUserAPC.  This function is
**          only used here in the Windows 95 case and loads the function at
**          runtime.
**          Since the user APC can only be executed when the invoking thread
**          is in an alertable wait the 95 specific code in GCsync was removed.
**
**          Made certain portions of the I/O code common between NT and 95.
**          This made the implementation of the APCs easier.
**      04-Sep-97 (fanra01)
**          Replaced error status reporting to GCA completion.
**          Modified conditionals where return types changed from BOOL to
**          ULONG.
**	28-aug-1997 (canor01)
**	    Allocate connect_block and iocb on the stack for GCrequest
**	    since they are not explicitly freed.
**	07-oct-97 (mcgem01)
**	    Replace LOGON32_LOGON_BATCH with LOGON32_LOGON_NETWORK which is
**	    available only on VC++ 5.0.  This removes the need to grant all
**	    of those darned user rights we normally insist on.  Also closed
**	    off a handle leak.
**      14-Oct-97 (fanra01)
**          Modified the conditions for when a FlushFileBuffer is called.
**          Should isolate the GC level purge message.
**          Scan start of all messages for purge message.
**	15-nov-97 (mcgem01)
**	    Now that we're building with VC++ 5.0, we too can replace
**	    LOGON32_LOGON_BATCH with LOGON32_LOGON_NETWORK to cut down
**	    on the user rights an Ingres/Net user must be granted.
**	19-feb-98 (mcgem01)
**	    Write a zero byte message on the normal and expedited receive
**	    channels prior to closing them.
**	01-Apr-98 (rajus01)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	    Hostname added to service parms for direct network connections.
**      12-may-1998 (canor01)
**          If username/password is invalid, check for the hard-coded Jasmine
**          username/password from jasmonitor and return error code specific
**          to this situation.
**	12-Jun-98 (rajus01)
**	    Ray (fanra01) found that iigcn process takes up all the CPU
**	    due to infinite loop execution in GCnew_assoc_id() and that 
**	    assoc_id_counter references beyond the handle arrays of purge and
**	    receive. Resolved the issue.
**	    Added new semaphore GC_associd_sem for accessing global 
**	    assoc_id_counter in multi-threaded environments.( Is adding
**	    new semaphore OK??? )
**	18-jun-1998 (canor01)
**	    Synchronize with jasmine.
**	26-Jul-1998 (rajus01)
**	    Added TIMEOUT check for GClisten() requests on NT only using
**	    Waitable Timers.
**      07-nov-1998 (canor01)
**          GCsync() can now be awakened by an asynchronous BIO that timed out.
**	    Use LoadLibrary to get entry points for functions that do not
**	    exist on Windows 95.
**	08-feb-1999 (somsa01 for lunbr01)
**	    Simulate a blocking write on W95 on the GCPURGE message to
**	    maintain message boundary; that is, make sure it is not
**	    concatenated with any other messages.  Otherwise, if concat
**	    with other msg, the GCPURGE msg will not be noticed and a
**	    hang will occur (intermittent)...GCPURGE triggered by
**	    GCA_ATTENTION (interrupt). These changes were taken from
**	    OI Desktop changes in March and June 97 by mcgem01 & zhayu01.
**	09-Feb-1999 (somsa01)
**	    In GCpurge(), if is_comm_svr was set, FlushFileBuffers() was
**	    executed to clear out the client read pipe. in other cases, we
**	    just did a wait. However, according to the Micorosft Visual C++
**	    documentation, FlushFileBuffers() will wait until any remaining
**	    data has been read by the client before flushing the pipe.
**	    Therefore, we can use this in ALL situations.
**	15-mar-1999 (somsa01)
**	    Partial x-integration of fix for Jasmine bug 95556; in
**	    GCCompletionRoutine(), do not test for the purge message if
**	    dwNumberXfer is zero.
**	22-mar-1999 (somsa01)
**	    In GCpurge(), replaced the second remaining PCsleep() with
**	    FlushFileBuffers(). It is executed only if there is data left
**	    on the pipe after sending a PURGE message. Also, in GClisten95(),
**	    removed connect_block, as what should be used is asipc->connect.
**	31-mar-1999 (somsa01)
**	    In GCsync(), properly set up the number of events for Windows 95,
**	    as the TIMEOUT event is not valid on that platform.
**	19-apr-1999 (mcgem01)
**	    Close off a semaphore leak in GCrequest by freeing the purge
**	    semaphore in each exit path.
**	19-apr-1999 (somsa01)
**	    In GCregister(), create the pipes after normal initialization.
**	    This way, when a client tries to connect during a server-side
**	    startup, the client will not get back an error but rather wait
**	    for the initialization of the server to complete.
**	06-may-1999 (somsa01)
**	    In GCcloseAllPipes(), for the Server-ended pipes, we must also
**	    perform a DisconnectNamedPipe() before closing the handle.
**	10-may-1999 (somsa01)
**	    Remove the FlushFileBuffers() calls from last change, as they
**	    were causing hangs during the Replicator handoffqa tests.
**	26-May-1999 (rajus01)
**	    GCC was hanging on loopback connections while running replicator
**	    tests: The problem was due to releasing a wrong semaphore object
**	    that actually belonged to a different association. GCnew_assoc_id()
**	    no longer exists. Creation new association id is moved to 
**	    GCcreateSemaphore(). Renamed assoc_id_counter to sem_id_counter.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      09-Sep-1999 (fanra01)
**          Bug 98946
**          Add changes to remove sleeps during pipe creation, connection and
**          dicsonnection to improve performance.  Modified the WaitNamedPipe
**          case for infinite wait which now times out every PIPE_RETRY_SLEEP
**          and reposts.  Retry pipe creation if busy.
**          Free resources during error conditions.
**      01-Oct-1999 (fanra01)
**          Bug 99023
**          Free memory when error encountered.
**      01-Oct-1999 (fanra01)
**          Implementation of change by loera01 for Bug 90952.
**          Add asyncIoPending counter to determine if IOs have completed.
**          When the last IO completes the asyncIoComplete event is signalled
**          allowing GCdisconn to continue.  This removes the need to use
**          SleepEx to make the disconnecting thread alertable.
**      03-Dec-1999 (fanra01)
**          Bug 99392
**          Update the disconnect logic to ensure that all I/Os are completed
**          before the pipe handles are closed.  Also modify the I/O
**          completion to check for zero length expedited messages with
**          ERROR_BROKEN_PIPE status to complete error free.
**      31-Jan-2000 (fanra01)
**          Bug
**	28-feb-2000 (somsa01 for lunbr01) Bug 95163 for W95
**	    Prior fix for this bug was made in NT-specific code. Implemented
**	    same change in W95. That is, remove PCsleeps in the GCpurge code,
**	    thus greatly improving performance in this area. Shared code as
**	    much as possible with NT. Faster code requires checking Purge
**	    Semaphore in all processes (not just appls) to prevent occasional
**	    hangs when GCpurge message recvd before GCA_ATTENTION.
**	28-feb-2000 (somsa01 for lunbr01) Bug 99826 for W95
**	    Occasional hang on W95. GC_PURGE was being received (std chan)
**	    before GCA_ATTENTION (exp chan) in client-side GCC. Removed check
**	    that was preventing GC_PURGE semaphore logic from being executed
**	    in server processes. This hang occurred with and without fix for
**	    bug 95163; fix was made to common NT and W95 code, although bug
**	    only reported on W95.
**	04-may-2000 (somsa01)
**	    In GCrequest(), reworked for loop when attempting to connect to
**	    the named pipe to follow more closely the logic on UNIX. We now
**	    loop for a maximum of the timeout (in PIPE_RETRY_SLEEP chunks) or
**	    PIPE_RETRIES (in the case of an infinite timeout). Also, make
**	    sure that 'svc_parms->gc_cb' is set to NULL if we have to free
**	    up the asipc, so that cleanup routines do not end up freeing
**	    up a piece of memory which has already been freed (thus
**	    causing a SEGV). Also, in GCcreateStandardPipes(), moved
**	    creation of the RcvStdChan to the top to speed up client
**	    connections.
**	11-may-2000 (somsa01)
**	    Removed all statements which freed the asipc/gc_cb. This should
**	    only be freed via GCrelease().
**	12-may-2000 (somsa01)
**	    A constant PMget() on max_assocs in GCcreateStandardPipes()
**	    leaks memory; we now do this once in GCregister() and hold onto
**	    it in a static variable.
**	11-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	25-oct-2000 (somsa01)
**	    Replaced usage of IDname() (which returns the REAL user id)
**	    with GCusername() (which returns the EFFECTIVE user id).
**	07-nov-2000 (somsa01)
**	    In GCusername(), for Embedded Ingres, the user is always ingres.
**	16-mar-2001 (mcgem01)
**	    Set and get the listen address in the user registry so that 
**	    network installations can be supported.
**	21-mar-2001 (mcgem01)
**	    Revert to keeping the name server listen in the symbol table
**	    until we come up with a better way of supporting NFS clients.
**
**      09-Jul-1999 (loera01) Bug 90952
**	   GCcloseAllPipes often returned without allowing the completion
**  	   routines to finish on the four channels.  Added SleepEx calls to
**  	   make sure they had a chance to finish.  Otherwise, a nasty memory
**   	   leak ensues.
**      15-mar-1999 (somsa01)
**          Partial x-integration of fix for Jasmine bug 95556; in
**          GCCompletionRoutine(), do not test for the purge message if
**          dwNumberXfer is zero.
**      25-jan-2000 (lunbr01) Bug 100129
**          Remove calls to DisconnectNamedPipe() during disconnect. This was
**          causing connection attempts to gateways to fail (E_LC0025) because 
**          the gateway listen daemon would disconnect from a connecting
**          session after he spawned a sub-process to handle the connection. 
**          The DisconnectNamedPipe() call had been introduced in fix for bug
**          90952.
**      28-feb-2000 (lunbr01) Bug 95163 for W95
**          Prior fix for this bug was made in NT-specific code. Implemented 
**          same change in W95. That is, remove PCsleeps in the GCpurge code,
**          thus greatly improving performance in this area. Shared code as
**          much as possible with NT. Faster code requires checking Purge
**          Semaphore in all processes (not just appls) to prevent occasional
**          hangs when GCpurge message recvd before GCA_ATTENTION.
**      28-feb-2000 (lunbr01) Bug 99826 for W95
**          Occasional hang on W95. GC_PURGE was being received (std chan)
**          before GCA_ATTENTION (exp chan) in client-side GCC. Removed check
**          that was preventing GC_PURGE semaphore logic from being executed 
**          in server processes. This hang occurred with and without fix for 
**          bug 95163; fix was made to common NT and W95 code, although bug
**          only reported on W95. 
**	01-nov-2000 (rigka01) Bug 100928 
**	    Cross integrate GCusername() from 2.5 change 435518 to 2.0 
**   	    Cross integrate 2.5 change 447967 to 2.0:
**          Replaced usage of IDname() (which returns the REAL user id)
**          with GCusername() (which returns the EFFECTIVE user id).
**      23-feb-2001 (rigka01) 
**	    Cross integrate 2.5 change 448098 to 2.0: 
**          In GCusername(), for Embedded Ingres, the user is always ingres.
**	28-jun-2001 (somsa01)
**	    For OS Version, use GVosvers().
**      04-Aug-2001 (lunbr01)  Bug 105239
**          Listen timeouts not working after the 1st incoming
**          connection.
**      27-sep-2001 (lunbr01) Bug 105902
**          Logic to check for GC purge message is missing in sync receives
**          (only exists in async). Gateways are the only servers using sync;
**          the fix is needed to prevent hangs while processing dbevents.
**      23-Oct-2001 (lunbr01) Bug 105793 & 106119
**          Increase MAX_ASSOCS from 100 to 1000 for W9x and eliminate
**          limit on NT/W2K. Maximum associations will now be limited
**          only by configurable parms such as gcc's inbound_limit or 
**          outbound_limit or dbms connect_limit and not restricted
**          by hard-coded MAX_ASSOCS value.  Moved purge semaphore from
**          predefined static array to ASSOC_IPC per-session ctl blocks.
**          This also fixes bug 106119 (GW hang on GCpurge) because the
**          purge semaphore was not accessible previously from the
**          spawned GW process; may also have been a problem for one
**          application calling another such as OpenROAD to Report Writer.
**      22-Jan-2002 (mofja02) Bug 106890 
**          Added logic to save and restore the Is_server global in GCSave 
**	    and GCrestore. Because Gateways spawn child processes globals 
**	    must besaved and restored and Is_server was not.  This caused  
**	    a hang onsome disconnects.
**      13-Feb-2002 (fanra01)
**          Sir
**          For evaluation releases always return authenticated when OS
**          authentication requested.
**      09-Apr-2002 (ashco01) Bug 107527.
**          W95/98 clients can occasionally recieve a GC purge message
**          appended to the previous message buffer even though the
**          messages have been sent seperately. This would cause the
**          purge message to be missed and a GC receive wait to occur
**          causing the FE application to hang. This fix is a w95/98
**          client-side only change to check for the purge message at
**          the end of the current buffer if not already found at the
**          beginning.
**      25-Jun-2002 (weife01) Bug 108001 
**          In GCsetWaitableTimer(), cast the nanosec to LONGLONG, prevent
**          loosing the precision in setting the timer when the function
**          is called.
**      18-Oct-2002 (fanra01)
**          Bug 106589
**          Added CPU clock timestamps to GCtrace when compiled with xDEBUG.
**          This is processor specific and should work on pentium class
**          Intel processors.
**      12-Dec-2002 (lunbr01) Bug 108292
**          Memory leak of 4-12K per connect/disconnect in comm server on
**          client side.
**	17-mar-2003 (somsa01)
**	    In GCNamePipeSend(), make sure we free the iocb if WriteFileEx()
**	    fails.
**	08-apr-2003 (somsa01)
**	    In GCreceiveCompletion(), ammended lunbr01's fix for Bug 108292
**	    to specifically target the comm server. This was causing loss of
**	    GCA_RELEASE messages in the ICESVR.
**	30-jul-2003 (somsa01)
**	    In GCrestore() and GCsave(), properly address pointers out of
**	    ulptr (since they can be different sizes between 32-bit and
**	    64-bit).
**	07-nov-2003 (somsa01)
**	    Modified last change to ONLY be valid for 64-bit. Apparently,
**	    the new macros are not available in 32-bit with the older compiler.
**	25-nov-2003 (somsa01)
**	    Further cleaned up prior change.
**	26-jan-2004 (somsa01)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**      10-May-2004 (lunbr01) Bug 107527.
**          Addendum to previous fix. Also allow for case where GC purge
**          message is not at the beginning or the end of the buffer, 
**          but is "near" the end and is followed only by a GCA_IACK
**          message, which is typically sent right after the GC purge.
**          Only occurs on W9x systems where we can't seem to force
**          the GC purge message to always travel alone.
**          Another fix was to always check the purge semaphore on W9x;
**          previously was skipped for comm servers, which is biggest
**          need!  Left WNT alone for this part since no problems 
**          have been seen on WNT type systems.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      19-jul-2004 (loera01)
**          For GClisten2(), don't use login name as the user ID.  Instead,
**          get the pipe owner.
**	13-sep-2004 (somsa01)
**	    As an addendum to loera01's last change, we need to lower-case
**	    the userid after getting it from the Pipe.
**      13-dec-2004 (Ralph.Loen@ca.com) Bug 113612
**          In GCrequest(), increase wait time from one second to two minutes.
**      12-apr-2005 (Ralph.Loen@ca.com) Bug 114281 
**          In GCrequest() resolve deadlocks with the GCN to resolve
**          deregistration problem.
**      27-Jul-2005 (drivi01, loera01)
**          Added routine for lowering priority of a Name Server in case
**          if it was set to a higher priority by iirun.exe during startup.
**      01-Jul-2005 (lunbr01) Bug 109115
**          "Direct" connections fail against gateway servers due to disconn
**          logic, which is invoked by gateway daemon. Fix is to not write
**          0-length record to send pipes if gateway daemon process.
**      26-Aug-2005 (lunbr01) Bug 115107 + 109115
**          Out of stack memory error causes GCC to crash.  Eliminate nested
**          calls to async IO completion routines which Microsoft says can
**          chew up memory in the stack.  This condition can occur in 
**          GCdisconn() when triggered by a killed application (failed recv).
**          FYI: This fix eliminates the 0-byte write at disconnect and
**          hence the need for the changes introduced for bug 109115, 
**          so those changes were backed out.
**      10-Oct-2005 (loera01) Bug 115107
**          GCdisconn() now invokes GCcloseAllPipes() in order to drive
**          pending I/O routines to completion via CloseHandle().
**      07-Feb-2006 (Ralph Loen) Bug 115730 
**          In GCrequest(), increase wait time from two minutes to 
**          three minutes when the caller has requested an infinite wait.
**          Invoke SleepEx() only if the caller is a server.
**      19-Jan-2006 (lunbr01) Bug 115622
**          Slow performance for syncronous receives with timeout value (not
**          0 or -1).  Manifests itself in Oracle gateway which uses this
**          to be able to handle DBevents.  
**      27-apr-2006 (Ralph Loen) Bug 116112
**          In GCrequest(), report timeout errors as GC_TIME_OUT rather than
**          GC_NO_PARTNER.
**      21-May-2006 (Ralph Loen) Bug 115730
**          In GCrequest() increased the wait time to four minutes and
**          removed the Is_server conditional for the SleepEx() call.
**      12-Jan-2007 (lunbr01) Bug 117257
**          "Copy in data" is slower after fix for bug 115622.
**      01-May-2007 (Fei Ge) Bug 116825
**          In GCusername(), szUserName shouldn't be static.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    DAS can hang for minutes if multiple concurrent connects.
**	    DBMS is waiting to receive peer info for session from DAS while
**	    DAS is waiting on 2nd session trying to connect to named pipe
**	    instance, which DBMS won't create until he gets peer info for 1st
**	    session.  DAS's send of peerinfo for 1st session can get queued
**	    behind 2nd session connect.  Fix is to make connection processing
**	    asynchronous, which will also significantly improve performance
**	    for all servers when many connections are being requested. 
**	    Add connection statistics to trace output to assist with problem
**	    and performance diagnosis.
**	    Also move GChostname call to init, not for each connect.  Also,
**	    some misc error handling cleanup such as setting GCTRACE level
**	    to 1 (from 3) for various failures such as create purge semaphore
**	    ...all errors should be traced at level 1.
**	04-Aug-2009 (Bruce Lunsford) Bug 122009 and 122406
**	    After previous fix (for bug 122009), direct connect fails with
**	    error messages E_LC0001 GCA protocol service (GCA_REQUEST) failure
**	    plus E_LQ0001.  Prior fix had failed to set status=OK after
**	    async pending I/O did not complete immediately.  Also, the
**	    error handling was enhanced in that area of code.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	    Check for error/warning conditions after successful reads
**	    and writes to the pipe, per recommendation in Microsoft docs;
**	    not encountering as a problem, but at least trace if occurs.
**	    Remove unused CompletionCritSect and clean up compiler warnings.
**	    DUPLEX on pipes is unnecessary and may utilize extra resources;
**	    change to 1-directional depending on usage (remove unnecessary
**	    options, including FILE_ATTRIBUTE_NORMAL).
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime. This also involves using errno instead of
**	    GetLastError() for failures and _endthreadex() at end of thread.
**	21-Aug-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008, clean up warnings.
**	11-Sep-2009 (Bruce Lunsford) Bug 122589 and 122426
**	    After previous fix (for sir 122426), local connections from an
**	    OpenROAD client built on a pre-Ingres 9.3 codebase (eg, OpenROAD
**	    2006 SP1) installed into an Ingres 9.3 installation fail with
**	    error messages E_LC0001 with internal status E_GC0021 (Requested
**	    association partner is unavailable).  Direct connects from
**	    pre-Ingres 9.3 clients fail in the same way.  The cause is that
**	    the pre-Ingres 9.3 clients still expect DUPLEX pipes, a property
**	    that was removed in prior enhancement as "unneeded" (but IS
**	    needed for backward compatibility to earlier clients).  Fix is to
**	    restore DUPLEX property to pipes.
**	10-Dec-2009 (Bruce Lunsford) Bug 123046 (and 122406)
**	    If send of peer info times out (such as over a WAN), then must
**	    cancel the pending IO before proceeding with error; else,
**	    undefined results later including likely ACCVIO in OS function.
**	    Not a problem prior to fix for bug 122406 because time outs
**	    were ignored and treated as success (not good either, but no
**	    ACCVIO and would generally work OK).  Also bumped timeout time
**	    from 100 to 1000 msecs to avoid unnecessary timeout failures
**	    over slow networks (like a WAN).
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers.  For clarity/consistency,
**	    change length of static "local_host" from MAX_LOC(=260) to
**	    GC_HOSTNAME_MAX+1(=257).
**	    Use new format for connect peerinfo messages, which contain
**	    user and hostname; W9x not updated for this as it is no longer
**	    supported.
**	    Also improve performance in GCusername (affects connect requests
**	    primarily) by only checking 1st time in for TNG/embedded Ingres
**	    installation.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    With the orginal change for this sir, when a GC CL peer info
**	    message in the new longname format was received, the userid
**	    from the message (sent by client) was being returned to
**	    the GClisten() caller rather than the "OS-retrieved" pipe owner.
**	    These values "should" be the same anyway, but the pipe owner
**	    is a more trusted/reliable value for identifying the client-side
**	    user.  This is now consistent with pre-longname format behavior.
**	    Also, send peer info as variable length (only what is needed)
**	    rather than full max fixed length message; uses less
**	    (network) overhead and matches Unix behavior.  On the receiving
**	    end, this change requires tightening up the check to determine
**	    the type of incoming peer info message (previously counted on
**	    unique length only).
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add support for net drivers like "tcp_ip" as local IPC.
**	    New functions named GCDriver* were added for calling net
**	    driver(s)--versus the default of executing the pipes protocol
**	    logic, which was modified as little as possible.
**	    Added calls out to gcarw.c functions (ported from Unix)
**	    to implement multiplexing of the normal and expedited channels
**	    for an association over a single network connection (1 per
**	    association -> pipes uses 4 pipe connections per association!).
**	    File gcarw.c has driver functions GCDriver[Receive,Send,Purge,
**	    Disconn]; the others (for connect and listen) are in this file.
**	    NOTE that driver logic is only used if II_GC_PROT environment/
**	    Ingres variable is set to "tcp_ip" or one of the other protocols
**	    listed in the new gcacl PCT (protocol control table).
**	    Status codes returned from the network driver are remapped to
**	    valid gcacl status codes, which are later remapped (in GCA) to
**	    GCA status codes.  Not remapping the driver statuses causes
**	    error message file lookup problems because the converted GCA
**	    status won't be valid (see gc.h and gca.h for valid mappings).
**	    FYI: Unix BS statuses don't get converted to GCF statuses, so
**	    don't experience the same problem.
**	    For readability:
**	    - Reorganize location of functions within file:
**	        External and related, common, named pipe, network protocol
**	        driver, windows 95 anonymous pipes, and DEBUG routines.
**	    - Add long asterisks lines to start and end of function flowerboxes
**	      for those that were missing.
**	    Add svc_parm to fn entry traces in a consistent manner and made
**	    miscellaneous other trace improvements.
**	    With local IPC set to "tcp_ip", direct connects from Unix/Linux
**	    from compatible machines are now supported for Ingres versions
**	    r3/2006 and later.  Due to increased access, added requirement
**	    that direct access from remotes must be configured (II_GC_REMOTE)
**	    in server (same as Unix); this also applies now to remote access
**	    via named pipes from remote Windows machines.
**
**	    Set local_host to "true" local hostname with GetComputerName()
**	    rather than using GChostname(), which can be influenced by
**	    II_HOSTNAME variable.  "local_host" is used in peer info message
**	    processing and is used somewhat to determine if connection is
**	    local or remote and is part of security checking.
**
**	    For pipes, preallocate IOCBs for sends and receives once at
**	    connection start and free at disconnect.  This avoids the
**	    overhead of allocating and freeing on each send and receive.
**	    (Not applicable to the net driver logic, which doesn't use
**	    IOCBs for sends and receives.)
**
**	    Make disconnect safer by not considering IO complete until
**	    the caller's completion routine for the IO has returned, not
**	    just that the physical IO callback has been entered.
**
**	    Change default pipe size from 4096 to 12288 to match Unix and
**	    new tcp_ip packet size (for performance).
**	28-May-2010 (Bruce Lunsford) Bug 123827
**	    Fix for minor problem in last change for sir 122679.
**	    Lowercase the pipe owner since userid is not case-sensitive
**	    on Windows (and to be consistent with GCusername).  Otherwise,
**	    can get a trace WARNING in listen complete when pipe owner
**	    doesn't match the user sent by the client when Ingres run
**	    as a service (SYSTEM vs system).
**	16-Aug-2010 (Bruce Lunsford) Bug 124263
**	    Ingstart.exe may crash or take (up to 30 seconds) longer than it
**	    should to start Ingres if II_GC_PROT=tcp_ip (from sir 122679).
*/

/******************************************************************************
**  Defines of other constants.
******************************************************************************/

#define MAXPNAME             80
#define CONNECT_NORMAL       1
#define CONNECT_DETECT       0
#define DETECT_TIMEOUT       0
#define GC_STACK_SIZE        32768
#define NORMAL               0
#define EXPEDITE             1
#define GC_PURGE_MESSAGE     "IIPRGMSG"
#define GCA_IACK_LEN         28
#define PIPE_RETRIES         10
#define PIPE_RETRY_SLEEP     100

/*
** 4k is probably the optimal buffer size for named pipes, all things
** considered but we will let the user tune this, if desired, through
** the INGRES environment variable II_GCA_PIPESIZE
*/

#define DEFAULT_NP_SIZE     12288
#define MIN_NP_SIZE         1024
#define MAX_NP_SIZE         32768
#define PIPE_PREFIX         "\\\\.\\PIPE\\INGRES"
#define PIPE_QUAL_PREFIX    "\\PIPE\\INGRES"

#define CSGET_SCB(x)        if (Is_server) \
                                CSget_scb((CS_SCB * *)x); \
                            else \
                                * ((CS_SCB * *)(x)) = (CS_SCB *) NULL

/*****************************************************************************
** Debugging and trace macros
******************************************************************************/

GLOBALREF i4  GC_trace;
# ifdef xDEBUG
# define GCTRACE( n )  if ( GC_trace >= n )(void)GCtrace
# else
# define GCTRACE( n )  if ( GC_trace >= n )(void)TRdisplay
# endif

# ifdef xDEBUG
#define GCRESRC( x, y ) \
y; \
GCTRACE(4)("GCresrc: %d %x %s\n", __LINE__, x, #y)

# else
#define GCRESRC( x, y ) y
# endif

#define GCerror(err,stat)	svc_parms->sys_err->errnum = stat; \
				svc_parms->status = (err); \
				(*svc_parms->gca_cl_completion) (svc_parms->closure); \
				return

/******************************************************************************
**  Definition of static variables and forward static functions.
******************************************************************************/

GLOBALREF GCC_PCE	GCdrivers[];	/* Ptr to local IPC PCT table */
static    GCC_PCE	*GCdriver_pce;	/* Ptr to local IPC PCT entry */

/* information about our listener */

static char	  Listen_id[64] = "";
GLOBALREF ULONG   Pipe_buffer_size;
static ASYNC_IOCB ListenIOCB;
static char       Srv2cli_std_pipe_name[MAXPNAME];
static char       Cli2srv_std_pipe_name[MAXPNAME];
static char       Srv2cli_exp_pipe_name[MAXPNAME];
static char       Cli2srv_exp_pipe_name[MAXPNAME];

i4 	Is_w95 = 0;
i4 	Is_server = 0;
PID	Conn_id;
static i4	connect_limit = 0;

/* misc semaphores */
#ifdef MCT
GLOBALREF       CS_SEMAPHORE    GC_misc_sem;
#endif /* MCT */
/* statistics used internally for analysis...traced at termination */

static i4 GCrequest_calls		= 0;
static i4 GCrequest_OK   		= 0;
static i4 GCrequest_delay_OK		= 0;
static i8 GCrequest_delay_OK_msecs	= 0;
static i4 GCrequest_delay_FAIL  	= 0;
static i8 GCrequest_delay_FAIL_msecs	= 0;


/* statistics used by TMperfstat */

GLOBALREF i4 Gc_reads;		/* used by TMperfstat */
GLOBALREF i4 Gc_writes;		/* used by TMperfstat */

/****************************************************************************
** Event Objects:
**
** Three asynchronous event objects: LISTEN, SHUTDOWN and GCC_COMPLETION
******************************************************************************/

GLOBALREF HANDLE hAsyncEvents[NUM_EVENTS];
GLOBALREF HANDLE hSignalShutdown;

/* Function pointer for QueueUserAPC */
static DWORD (FAR WINAPI *pfnQueueUserAPC)(PAPCFUNC pfnAPC,
                                           HANDLE hThread,
                                           DWORD dwData) = NULL;
static HANDLE (FAR WINAPI *pfnCreateWaitableTimer)(
				LPSECURITY_ATTRIBUTES lpTimerAttributes,
  				BOOL bManualReset,  
				LPCTSTR lpTimerName ) = NULL;
static BOOL (FAR WINAPI *pfnCancelWaitableTimer)(HANDLE hTimer) = NULL; 
static BOOL (FAR WINAPI *pfnSetWaitableTimer)(
  				HANDLE hTimer,
  				const LARGE_INTEGER *pDueTime,
  				LONG lPeriod,
  			 	PTIMERAPCROUTINE pfnCompletionRoutine,
  				LPVOID lpArgToCompletionRoutine,
  				BOOL fResume) = NULL;
static BOOL (FAR WINAPI *pfnCancelIo)(HANDLE hFile) = NULL; 

FUNC_EXTERN BOOL GVosvers(char *OSVersionString);


/******************************************************************************
*******************************************************************************
**
**  External and related routines
**
*******************************************************************************
******************************************************************************/


/******************************************************************************
** Name: GCinitiate - Initialize static elements within GC CL.
**
** Description:
**
** This routine is called by gca_initiate to pass certain variables to the CL.
** Currently, these are the addresses of the memory allocation and deallocation
** routines to be used.  Any other system-specific initiation functions may
** be performed here
**
** Inputs:
**
**     alloc   Pointer to memory allocation routine
**     dealloc Pointer to memory deallocation routine
**
** Outputs:
**     none
**
** Returns:
**     none
**
** Exceptions:
**     none
**
** History:
**	26-Jul-1998(rajus01)
**	   Added CreateWaitableTimer() for the new TIMEOUT event for
**	   GClisten() requests.
**      09-Sep-1999 (fanra01)
**          Change the listen event to a manual reset as overlapped events
**          should be manual.
**      02-Sep-2005 (loera01) 115107
**          Locate QueueUserAPC() for all Windows, not just Win95.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Move call of GChostname to here from GCrequest() to be done once
**	    rather than for each connect to improve performance, since it
**	    issues NMgtAt and calls OS function.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.  Remove creation of hCompletion mutex
**	    and CompletionCritSect critical section since they are no longer
**	    needed.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**          Add support for net drivers like "tcp_ip" as local IPC.
**	    Get "true" local hostname with GetComputerName() rather than
**	    using GChostname() which can be influenced by II_HOSTNAME variable.
**	    The "local_host" static field is used in peer info message
**	    processing and is used somewhat to determine if connection is
**	    local or remote and is part of security checking.
**
******************************************************************************/
STATUS
GCinitiate(PTR  (*alloc_ptr)(), VOID (*free_ptr) ())
{
	char *trace;
	char *ptr = NULL;
	SECURITY_ATTRIBUTES sa;
        char VersionString[256];
        HANDLE hDll = NULL;
	STATUS status;
	i4     host_len;

        /*
        ** If GCinitiate has already been called - we can just return here.
        */

	GCinitialized_count++;
        if ( GCinitialized_count > 1 )
            return (OK);
# ifdef xDEBUG
        {
            /*
            ** Create the wakeup thread to allow debugger to break when
            ** application hung.
            */
            DWORD ThreadId = 0;
            GCRESRC( hwakeupthread, hwakeupthread = (HANDLE)_beginthreadex(NULL, 0, wakeup, NULL, 0, &ThreadId));
        }
# endif /* xDEBUG */

	/*
	** Initialize local (real) hostname and effective user id.
	*/
	host_len = sizeof(local_host);
	GetComputerName(local_host, &host_len);
	GCusername( gc_user_name, sizeof( gc_user_name ) );

	iimksec (&sa);
	/* 
	** The events and semaphores created here should 
	** not be inherited by children
	*/
	sa.bInheritHandle = FALSE;

	GVosvers(VersionString);
	Is_w95 = (STstrindex(VersionString, "Microsoft Windows 9",
                      0, FALSE) != NULL) ? TRUE : FALSE;

	/*
	** See if any tracing is to be used.
	*/

	NMgtAt( "II_GC_TRACE", &trace );
	if ( !(trace && *trace) && PMget( "!.gc_trace_level", &trace) != OK )
	{
	    GC_trace = 0;
	}
	else
	{
	    CVan(trace, &GC_trace);
	}

	/*
	 * Get process id
	 */

	PCpid(&Conn_id);


	/*
	 * Create the shutdown event which is used by GCshut to get
	 * GCexec to exit.
	 */

        GCRESRC( hAsyncEvents[SHUTDOWN], hAsyncEvents[SHUTDOWN] = CreateEvent(&sa,
            FALSE,
            FALSE,
            NULL));
        if (hAsyncEvents[SHUTDOWN] == NULL)
	{
		return FAIL;
	}

	/*
	 * ConnectNamedPipe cannot have a completion routine.
	 * It must use an event to mark completion.
	 */

        GCRESRC( hAsyncEvents[LISTEN], hAsyncEvents[LISTEN] = CreateEvent(&sa,
            TRUE,
            FALSE,
            NULL) );
        if (hAsyncEvents[LISTEN] == NULL)
	{
		GCRESRC( hAsyncEvents[SHUTDOWN],
                    CloseHandle(hAsyncEvents[SHUTDOWN]) );
		hAsyncEvents[SHUTDOWN] = NULL;
		return FAIL;
	}

	MEfill(sizeof(ListenIOCB), 0, &ListenIOCB);
	
	/*
	** Events for timeout.
	*/

        if ( !Is_w95 )
        {
            /*
            ** Get a handle to the kernel32 dll and get the proc address for
            ** CreateWaitableTimer
            */
            if ((hDll = LoadLibrary( TEXT("kernel32.dll") )) != NULL)
            {
                pfnCreateWaitableTimer = 
			(HANDLE (FAR WINAPI *)(
                                LPSECURITY_ATTRIBUTES lpTimerAttributes,
                                BOOL bManualReset,
                                LPCTSTR lpTimerName ) )
                                     GetProcAddress( hDll, 
						TEXT("CreateWaitableTimerA"));
                if (pfnCreateWaitableTimer != NULL)
                {
                    pfnCancelWaitableTimer = 
				(BOOL (FAR WINAPI *)(HANDLE hTimer))
					GetProcAddress( hDll, 
						TEXT("CancelWaitableTimer"));
                    pfnSetWaitableTimer = 
				(BOOL (FAR WINAPI *)(
  					HANDLE hTimer,
  					const LARGE_INTEGER *pDueTime,
  					LONG lPeriod,
  			 		PTIMERAPCROUTINE pfnCompletionRoutine,
  					LPVOID lpArgToCompletionRoutine,
  					BOOL fResume))
					    GetProcAddress( hDll, 
						TEXT("SetWaitableTimer"));
                    GCRESRC( hAsyncEvents[TIMEOUT],
                        hAsyncEvents[TIMEOUT] = (*pfnCreateWaitableTimer)(&sa,FALSE, NULL) );
                    if ( pfnCancelWaitableTimer != NULL &&
                         pfnSetWaitableTimer != NULL &&
	                 hAsyncEvents[TIMEOUT] == NULL )
	            {
		        GCRESRC( hAsyncEvents[SHUTDOWN],
                            CloseHandle(hAsyncEvents[SHUTDOWN]));
	                GCRESRC( hAsyncEvents[LISTEN],
                            CloseHandle(hAsyncEvents[LISTEN]));
	                hAsyncEvents[SHUTDOWN] = NULL;
		        hAsyncEvents[LISTEN] = NULL;
		        return FAIL;
	            }
                }
		pfnCancelIo = (BOOL (FAR WINAPI *)(HANDLE hFile)) 
				GetProcAddress( hDll, TEXT("CancelIo") );
		FreeLibrary( hDll );
	    }

	    GCTRACE(3)("GCinitiate: Createwaitable timer success.\n");
        }

	/*
	 * If we're running in the Comm Server, asynchronous events are
	 * signaled by the protocol driver(s) to wake up GCexec and run
 	 * the callback routine.
	 */

        GCRESRC( hAsyncEvents[GCC_COMPLETE],
            hAsyncEvents[GCC_COMPLETE] = CreateEvent(&sa,
                FALSE,
                FALSE,
                NULL) );
        if (hAsyncEvents[GCC_COMPLETE] == NULL)
	{
		GCRESRC( hAsyncEvents[LISTEN],
                    CloseHandle(hAsyncEvents[LISTEN]) );
		GCRESRC( hAsyncEvents[SHUTDOWN],
                    CloseHandle(hAsyncEvents[SHUTDOWN]) );
		hAsyncEvents[SHUTDOWN] = NULL;
		hAsyncEvents[LISTEN] = NULL;
		return FAIL;
	}

        GCRESRC( hSignalShutdown,
            hSignalShutdown = CreateEvent (&sa, FALSE, FALSE, NULL) );
        if (hSignalShutdown == NULL)
        {
            GCRESRC( hAsyncEvents[LISTEN],
                CloseHandle(hAsyncEvents[LISTEN]) );
            GCRESRC( hAsyncEvents[SHUTDOWN],
                CloseHandle(hAsyncEvents[SHUTDOWN]) );
            GCRESRC( hAsyncEvents[GCC_COMPLETE],
                CloseHandle(hAsyncEvents[GCC_COMPLETE]) );
            hAsyncEvents[SHUTDOWN] = NULL;
            hAsyncEvents[LISTEN] = NULL;
            hAsyncEvents[GCC_COMPLETE] = NULL;
            return FAIL;
        }

	/* windows95 */
	if (Is_w95)
	{
            /*
             * SyncEvent is used for synchronous front-ends.
             * GCshutdownEvent is used to signal shutdown to GCexec.
             */
            GCRESRC( SyncEvent, SyncEvent=CreateSemaphore(NULL, 0, 1, NULL) );
            GCRESRC( GCshutdownEvent,
                GCshutdownEvent=CreateEvent(NULL, FALSE, FALSE, NULL) );
            if ((SyncEvent  == NULL) || (GCshutdownEvent == NULL))
                return FAIL;
        }
        else
        {
            GCRESRC( SyncEvent, SyncEvent=CreateSemaphore(NULL, 0, 1, NULL) );
            if (SyncEvent == NULL)
		return FAIL;
	}
        /*
        ** Get a handle to the kernel32 dll and get the proc address for
        ** QueueUserAPC.
        */
        hDll = NULL;
        if ((hDll = LoadLibrary( TEXT("kernel32.dll") )) != NULL)
        {
            pfnQueueUserAPC = (DWORD (FAR WINAPI *)(PAPCFUNC pfnAPC,
                            HANDLE hThread,
                            DWORD dwData))
				GetProcAddress(hDll, TEXT("QueueUserAPC"));
            if (pfnQueueUserAPC == NULL)
            {
                GCRESRC( hAsyncEvents[SHUTDOWN],
                    CloseHandle(hAsyncEvents[SHUTDOWN]) );
                GCRESRC( hAsyncEvents[LISTEN],
                    CloseHandle(hAsyncEvents[LISTEN]) );
                GCRESRC( hAsyncEvents[GCC_COMPLETE],
                    CloseHandle(hAsyncEvents[GCC_COMPLETE]) );
                GCRESRC( hSignalShutdown,
                    CloseHandle(hSignalShutdown) );
                hAsyncEvents[SHUTDOWN] = NULL;
                hAsyncEvents[LISTEN] = NULL;
                hAsyncEvents[GCC_COMPLETE] = NULL;
                hSignalShutdown = NULL;
                TRdisplay ("Function QueueUserAPC not found\n");
                return (FAIL);
            }
            FreeLibrary( hDll );
        }
        else
        {
            TRdisplay ("LoadLibrary kernel32 failed %d\n", GetLastError());
            return (FAIL);
        }
	hAsyncEvents[SYNC_EVENT] = SyncEvent;

	/*
	** Check for alternate LOCAL IPC protocols and call driver init
	*/
	GCdriver_pce = GC_get_driver();

	/*
	** Set the listen completion routine for GCexec(), GCsync(),
	** and CSsuspend()...anyone who waits on the LISTEN event.
	*/

	if (GCdriver_pce)
	{
	    GCevent_func[LISTEN] = GClisten2_complete;
	    GCevent_func[TIMEOUT] = GClisten_timeout;
	}
	else
	if (Is_w95)
	    GCevent_func[LISTEN] = GClisten95_2;
	else
	{
	    GCevent_func[LISTEN] = GClisten2;
	    GCevent_func[TIMEOUT] = GClisten_timeout;
	}

	/*
	** If network protocol driver selected, call its init function
	** if there is one.
	** FIXME...why not move all this code to GC_get_driver()?...
	** keeping this code simpler and more generic.  One prob is
	** that we need to handle an initialization error and do
	** appropriate cleanup.
	*/
	if (( GCdriver_pce ) &&
	    ( ((WS_DRIVER *)GCdriver_pce->pce_driver)->init_func != NULL ))
	{
	    status = (*((WS_DRIVER *)GCdriver_pce->pce_driver)->init_func)( GCdriver_pce );
	    if ( status != OK )
	    {
		TRdisplay ("GCinitiate: '%s' driver initialization failed with status %d\n",
			   GCdriver_pce->pce_pid,
			   status);
                return (FAIL);
	    }
	    GCTRACE(1) ("GCinitiate: '%s' driver selected for local protocol\n",
			GCdriver_pce->pce_pid);
	}

	/*
	** This variable is intended primarily for testing the various
	** options; it may not be needed long term.
	**
	** II_GC_MSG_MODE variable may have one of the following values:
	**   =BLOCK     -> force protocol driver to use "block" mode (even
	**                 if underlying protocol is stream-based).
	**                 (default)
	**   =ADDPREFIX -> tell protocol driver to use normal stream-based
	**                 mode where it prefixes all messages with 2-byte
	**                 length prefix.
	**   =USEPREFIX -> tell protocol driver to use length field from
	**                 passed-in (GC_MTYP) prefix in lieu of adding
	**                 or expecting his own 2-byte prefix.
	**                 (NOT YET SUPPORTED...a "future" if needed)
	** Set flag to tell protocol driver not to prefix messages
	** with 2-byte length prefix since the gcacl.c will supply
	** its own prefix (GC_MTYP) in the multiplexing code.
	** BLOCK mode or USEPREFIX are required for "direct connect"
	** interoperability with Linux/Unix compatible systems, which
	** have a GC_MTYP prefix only for GCA CL messages.  Setting
	** block_mode to TRUE causes the driver to simply read/write w/o
	** the prefix and w/o ensuring full messages, but requires the
	** calling code to reinvoke sends/receives when incomplete,
	** which the GCA CL multiplexing code does.
	*/
	if ( GCdriver_pce )
	{
	  ((THREAD_EVENTS *)GCdriver_pce->pce_dcb)->wsd.block_mode = TRUE;
	  NMgtAt( "II_GC_MSG_MODE", &ptr );
	  if (ptr && *ptr)
  	  {
	    GCTRACE(1) ("GCinitiate: driver msg mode = %s\n", *ptr);
	    if ( STcasecmp( ptr, "BLOCK" ) == 0 )
	    {
		((THREAD_EVENTS *)GCdriver_pce->pce_dcb)->wsd.block_mode = TRUE;
	    }
	    else
	    if ( STcasecmp( ptr, "ADDPREFIX" ) == 0 )
	    {
		((THREAD_EVENTS *)GCdriver_pce->pce_dcb)->wsd.use_gc_mtyp_len = FALSE;
	    }
	    //else    ** future maybe ** 
	    //if ( STcasecmp( ptr, "USEPREFIX" ) == 0 )
	    //{
		//((THREAD_EVENTS *)GCdriver_pce->pce_dcb)->wsd.use_gc_mtyp_len = TRUE;
	    //}
	  }
	}

	GCTRACE(3) ("GCinitate: Complete for (process id) PID=%d.\n",
		    Conn_id);
	return OK;
}

/******************************************************************************
** Name: GCregister - Server start-up notification
**
** Description:
** This routine is called by GCA_REGISTER on server start-up.  It is
** used to notify the GCA CL that a server has been started.
**
** It is in this routine that the invoking server's name is assigned.  This is
** done by a system-specific mechanism that ensures the assignment of a uniquue
** name.  This is the public external name which would be specified to GCrequest
** to initiate an association with the server.  This is returned to the
** invoker for his information.
**
** If appropriate for the particular system-dependent implementation, any
** set-up operations for subsequently establishing an IPC connection with a
** requesting client may be done at this time.  It may be assumed that the
** routine GClisten will be called subsequently.
**
** If IPC setup is done here, the following is applicable.  Otherwise, it
** applies to the GClisten function.
**
** The external specification for GCA allows normal and expedited data flows.
** The exact implementation of these is system dependent.  Where appropriate and
** doable, a separate channel may be established for each flow type.  If this
** is not possible, the same channel may be used for both.
**
** GCregister has no input parameters per-se.  It is passed a pointer to the
** svc_parms structure for this request, which contains the output elements.
**
** The server's name is established by creating a named pipe with a
** specified name.  It is of the form XXXX\pid, where XXXX is a
** server type designator and pid is the local process identifier.
** The only exception is the name server, which is just "IINMSVR".
** Thus, simultaneously, the unique server name is established and
** the endpoint which will subsequently be used for the "listen"
** function is created.  We save the pipe handle created by this
** action for later use.
**
** We set name_server_flag for GCN so that we do *not* make a
** unique name for it.  For other servers, we may have more
** than one going at a time, with disambiguation
** requirements.  With GCN, we will never have more than one,
** and we *really* want to know where it is.
**
** Inputs:
**  svc_parms               Pointer to the service parameter list, to
**                          find the output elements
**
** Outputs:
**  svc_parms->listen_id    The server name assigned to the invoker.
**
**  svc_parms->status       Success or failure indication.
**      OK                  Normal completion
**      GC_LSN_FAIL         Unable to establish listen path
**
**  svc_parms->sys_err      OS-specific status info, returned in
**                  	    a CL_SYS_ERR structure.
**
** Returns:
**     VOID
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	13-nov-95 (emmag)
**	    Modified to use four pipes instead of two.
**	19-apr-1999 (somsa01)
**	    create the pipes after normal initialization. This way, when a
**	    client tries to connect during a server-side startup, the client
**	    will not get back an error but rather wait for the initialization
**	    of the server to complete.
**      01-Oct-1999 (fanra01)
**          Add the creation of the asyncIoComplete event.  This event is
**          created with the inheritance attribute enabled for use in GCsave
**          and GCrestore.
**	12-may-2000 (somsa01)
**	    Obtain the connect limit here and hold onto it.
**	10-Feb-2004 (drivi01)
**	    Removed routine that retrieves connect_limit from config.dat.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove allocation of access_point_identifier area at end of
**	    asipc; it is no longer used.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If net driver selected, call it with GCC_OPEN instead of embedded
**	    named pipes logic. Rearrange code so that pipes-specific code is
**	    together (at bottom of function).
**	    Remove unneeded variable commServerListenPipe.
**	    Preallocate IOCBs for pipe sends and receives (for performance).
**
******************************************************************************/
VOID
GCregister(SVC_PARMS *svc_parms)
{
    char		*env= NULL;
    ULONG		pipesize;
    char		*ii_pipesz;
    char		*out_name;
    char		*inst;
    SECURITY_ATTRIBUTES	sa;
    char		name_server[GL_MAXNAME];
    DWORD		status;
    ASSOC_IPC		*asipc;

    GCTRACE(3)("GCregister: %p Entry point. PID=%d\n", svc_parms, Conn_id);

    svc_parms->status    = OK;

    iimksec (&sa);

    Is_server = TRUE;

    /*
    ** Establish whether or not the running process is a GCN by checking
    ** the name provided from GCA.  Note that this value may change in
    ** the future.
    */
    if ((svc_parms->svr_class) &&
        (!STbcompare(svc_parms->svr_class,6,"NMSVR",5,TRUE)))
        is_gcn = TRUE;

    /*
    ** Check to see if direct connect from remote machines is allowed.
    */
    if ( remote_access == GC_RA_UNDEFINED )
    {
    	NMgtAt( "II_GC_REMOTE", &env );
    	if ( ! env  ||  ! *env )
	    remote_access = GC_RA_DISABLED;
	else if ( ! STcasecmp( env, "ON" )  || 
		  ! STcasecmp( env, "ENABLED" ) )
	    remote_access = GC_RA_ENABLED;
	else if ( ! STcasecmp( env, "FULL" )  || 
		  ! STcasecmp( env, "ALL" ) )
	    remote_access = GC_RA_ALL;
	else
	    remote_access = GC_RA_DISABLED;
    }

    /*
    ** Initialize a structure for listening and set the LISTEN event.
    */

    ListenIOCB.Overlapped.hEvent = hAsyncEvents[LISTEN];
    ListenIOCB.svc_parms = svc_parms;

    /*
    ** Get an ipc block and initialize it.
    **
    */
    asipc = (ASSOC_IPC *)MEreqmem(0, sizeof(ASSOC_IPC), TRUE, NULL);
    if (!asipc)
    {
	GCTRACE(1)("GCregister: Failed to allocate an IPC block.\n");
	GCerror(GC_LSN_FAIL, 0);
    }
    asipc->flags |= GC_IPC_LISTEN;
    ListenIOCB.asipc = asipc;

    /*
    ** If network protocol driver selected, call it with GCC_OPEN.
    */
    if (GCdriver_pce)
    {
	status = GCDriverRegister (svc_parms, asipc, &ListenIOCB);
	return;
    }

    /*
    ** Preallocate IOCBs for pipe send/recv's.  A send and a receive
    ** IOCB is needed for each channel (normal + expedited)...hence,
    ** a total of 4.  For the protocol driver, IOCBs are not used; an
    ** exception is one temporarily used during connection setup which
    ** remains dynamically allocated and freed.
    */
    asipc->iocbs = (ASYNC_IOCB *)MEreqmem(0, sizeof(ASYNC_IOCB) * 4, TRUE, NULL);
    if (!asipc->iocbs)
    {
	GCTRACE(1)("GCregister: Failed to allocate IOCBs (4 for a total size of %d bytes).\n",
		    sizeof(ASYNC_IOCB) * 4);
	GCerror(GC_LSN_FAIL, 0);
    }

    /*
    **  Set up pipes for this server
    */

    svc_parms->listen_id = out_name = Listen_id;

    /*
    ** The installation id preceeds all server names
    */

    NMgtAt("II_INSTALLATION", &inst);
    if (inst && *inst)
    {
	STcopy(inst, out_name);
        out_name += STlength(out_name);
        *out_name++ = '\\';
    }

    /*
    ** Make sure there is a server name
    */

    if (svc_parms->svr_class)
	STlcopy(svc_parms->svr_class, out_name, 8);
    else
	STcopy("UNKNSRVR", out_name);

    /*
    ** Put in server class name
    */
    STcopy(SystemVarPrefix,name_server);
    STcat(name_server,"NMSVR");
    if (!svc_parms->svr_class || MEcmp(svc_parms->svr_class, name_server, 7))
    {
	out_name += STlength(out_name);
	*out_name++ = '\\';
	ultoa(Conn_id, out_name, 16);
    }

    /*
    ** Set pipe size.  Default is 4K, but it is configurable.
    */

    NMgtAt("II_GCA_PIPESIZE", &ii_pipesz);
    if (ii_pipesz && *ii_pipesz)
    {
	CVal(ii_pipesz, &pipesize);
	if (pipesize < MIN_NP_SIZE || pipesize > MAX_NP_SIZE)
	{
	    pipesize = DEFAULT_NP_SIZE;
	}
    }
    else
    {
	pipesize = DEFAULT_NP_SIZE;
    }

    Pipe_buffer_size = pipesize;

    /* set up names for all GCA pipes */

    STprintf(Srv2cli_std_pipe_name,"%s\\S2C_STD\\%s",
	    PIPE_PREFIX,Listen_id);
    STprintf(Cli2srv_std_pipe_name,"%s\\C2S_STD\\%s",
	    PIPE_PREFIX,Listen_id);
    STprintf(Srv2cli_exp_pipe_name,"%s\\S2C_EXP\\%s",
	    PIPE_PREFIX,Listen_id);
    STprintf(Cli2srv_exp_pipe_name,"%s\\C2S_EXP\\%s",
	    PIPE_PREFIX,Listen_id);

    if (Is_w95) /* Windows 95 */
    {
	/* set up names for all connections to server */
	/*
	** Changed the name to use a null string for compatibility with
	** OpenROAD on Windows 95.
	*/
	STprintf(Asc_std_name, "%s-%s", "", Listen_id);
    }

    /*
    ** Now, create the pipes, so that if any requests come in while we are
    ** still initializing, the requests will wait until we are ready to
    ** process them.
    */
    /*
    ** Create the pipes.
    */
    if (Is_w95)
    {
	asipc->SndStdChan = NULL;
	asipc->RcvStdChan = NULL;
	asipc->SndExpChan = NULL;
	asipc->RcvExpChan = NULL;
	asipc->ClientReadHandle = NULL;

	status = CreateAsc(asipc, Asc_std_name);
	if (!status)
	{
	    SETWIN32ERR(svc_parms->sys_err, ~OK , ER_listen);
	    svc_parms->status = GC_LSN_FAIL;
	    SETWIN32ERR(svc_parms->sys_err, status, ER_listen);
	    GCerror(svc_parms->status, status);
	}
    }
    else
    {
	status = GCcreateStandardPipes(asipc);
	if (status != OK)
	{
	    GCerror(GC_LSN_FAIL, status);
	}
    }
    ListenIOCB.flags |= GC_IO_CREATED;
}


/******************************************************************************
** Name: GCrequest - Build connection to peer entity
**
** Description:
**
** This routine is called by gca_request, as a result of a request by a
** client for a connection to a server.  The server must have issued a
** GCA_LISTEN service request in order to have prepared to accept an IPC
** connection.
**
** A local, system-dependent IPC connection is built to the listening server.
** An initial communication takes place with the server, sending a structure
** any system-specific information required by the implementation.
**
** The transmission buffer/segment size is set here.  The
** gca_size_advise parameter is set in the svc_parms parm list to specify
** the optimum buffer size to use if so desired.  A value of 0 indicates
** that the concept of buffer size optimization is irrelevant in this
** environment.
**
** a CL-internal data structure known generically as the Local Control Block
** (LCB) is allocated, and a pointer to it is set in the ACB.  The LCB
** contains system dependent data elements required for subsequent
** communication over the local IPC connection.  Its structure and content
** are unknown to the mainline code above the CL.
**
** Upon completion, the completion exit routine pointed to by the svc_parms
** parameter list is to be called.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** Inputs:
**   svc_parms              Pointer to GCA service parmeter list containing:
**     (*gca_cl_completion)() Routine to call on completion.
**     partner_host           host name of partner to which connection is
**                            requested...may be NULL.
**     partner_name           port name of partner to which connection is
**                            requested.
**     time_out               timeout in ms. 0=poll, -1=no timeout
**
** Outputs:
**  svc_parms->size_advise  Suggested buffer size for communication
**                          optimization.
**
**  svc_parms->gc_cb        Pointer to the LCB.
**
**  svc_parms->status       Success or failure indication.
**      OK                  Normal completion
**      GC_RQ_FAIL          Request failure: connection problem
**      GC_RQ_RESOURCE      Request failure: server resource shortage
**      GC_NO_PARTNER       Requested partner not available
**
**  svc_parms->sys_err      OS-specific status info, returned in
**                          a CL_SYS_ERR structure.
**
** Returns:
**     none
**
** Exceptions:
**     none
**
** Side Effects:
**     Pipe handles are inheritable by child processes.
**
** History:
**      09-Sep-1999 (fanra01)
**          Changed infinite connect to retry every PIPE_RETRY_SLEEP.
**          Prevents the hang when the listening pipe is created but the
**          ConnectNamedPipe is not invoked until after the WaitNamedPipe call.
**          Moved Sleep from wait retry loop as invoked for every iteration
**          even if successful.
**      01-Oct-1999 (fanra01)
**          Bug 99023
**          Free memory when error encountered.
**      01-Oct-1999 (fanra01)
**          Add the creation of the asyncIoComplete event.  This event is
**          created with the inheritance attribute enabled for use in GCsave
**          and GCrestore.
**          Add indication of infinite timeout so that for status is set for
**          real timeout cases.
**	04-may-2000 (somsa01)
**	    Reworked for loop when attempting to connect to the named
**	    pipe to follow more closely the logic on UNIX. We now loop
**	    for a maximum of the timeout (in PIPE_RETRY_SLEEP chunks) or
**	    PIPE_RETRIES (in the case of an infinite timeout). Also, make
**	    sure that 'svc_parms->gc_cb' is set to NULL if we have to free
**	    up the asipc, so that cleanup routines do not end up freeing
**	    up a piece of memory which has already been freed (thus
**	    causing a SEGV).
**      07-feb-2006 (Ralph Loen) Bug 115730 
**          Increase wait time from two minutes to three minutes when the 
**          caller has requested an infinite wait.  Only invoke SleepEx()
**          if called from a server.
**      27-apr-2006 (Ralph Loen) Bug 116112
**          Report timeout errors as GC_TIME_OUT rather than GC_NO_PARTNER.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    DAS can hang for minutes if multiple concurrent connects.
**	    DBMS is waiting to receive peer info for session from DAS while
**	    DAS is waiting on 2nd session trying to connect to named pipe
**	    instance, which DBMS won't create until he gets peer info for 1st
**	    session.  DAS's send of peerinfo for 1st session can get queued
**	    behind 2nd session connect.  Fix is to make connection processing
**	    asynchronous, which will also significantly improve performance
**	    for all servers when many connections are being requested. 
**	    Split sections of GCrequest() out to separate functions
**	    GCrequest2, GCrequestSendPeer, & GCconnectToServerPipe to
**	    better handle async code (more modular).  For instance, if
**	    connect to server pipe instance does not occur immediately
**	    in GCrequest(), it will spawn off a new thread GCrequest2()
**	    to complete connection processing ... thus avoiding blocking
**	    the main thread (for ASYNC GCrequest() calls only).
**	    Also:
**	    - Move call of GChostname to initialization to be done once
**	      rather than for each connect to improve performance, since it
**	      issues NMgtAt and calls OS function.
**	    - asipc->SndStdName was not getting set when pipe connected from
**	      within WaitNamedPipe loop.
**	    - Remove code related to GCN<->GCC deadlock detection since no
**	      longer needed with async connect capability.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove unnecessary options GENERIC_READ, FILE_SHARE_WRITE and
**	    FILE_ATTRIBUTE_NORMAL from sending pipe. Convert CreateThread()
**	    to _beginthreadex() which is recommended when using C runtime.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**          If net protocol driver selected, call it with GCC_CONNECT.
**	16-Aug-2010 (Bruce Lunsford) Bug 124263
**	    Save the (connect_)iocb address in the asipc for driver
**	    connections (eg, tcp_ip) so that it can be freed later in
**	    GCrelease().  Previously, this "save" was only done if
**	    connect processing reached the end of GCDriverRequest_exit()
**	    with intermediate MEfree()s if an error occurred, which
**	    complicated the code and could cause a crash in ingstart.
**
******************************************************************************/
VOID
GCrequest(SVC_PARMS *svc_parms)
{
	STATUS     status = OK;
	bool       cont = TRUE;
	ASSOC_IPC  *asipc;
	char       *szPipeName;
	SECURITY_ATTRIBUTES	pipe_security;
	i4         retry = 0;
	i4         retry_CreateFile = 0;
	char  	   buf[ 80 ];
	bool       infinite_timeout = FALSE;
	bool       sync_flag = svc_parms->flags.run_sync;

	GCTRACE(3)("GCrequest: %p Entry point. target=%s %s timeout=%d %s\n",
		    svc_parms,
		    svc_parms->partner_host ? svc_parms->partner_host : "",
		    svc_parms->partner_name,
		    svc_parms->time_out,
		    sync_flag ? "sync" : "async");

	GCrequest_calls++;	/* For internal connect stats */

	svc_parms->status    = OK;

	buf[0] = '\0';

	/*
	 * Get an ipc block and inialize it
	 */

	asipc = (ASSOC_IPC *)MEreqmem(0, sizeof(ASSOC_IPC), TRUE, NULL);
	if (!asipc)
	{
	    GCTRACE(1)("GCrequest: Failed to allocate an IPC block.\n");
	    GCerror(GC_RQ_FAIL, 0);
	}
	svc_parms->gc_cb = (char *)asipc;

        asipc->flags &= ~GC_IPC_LISTEN;

	/*
	**  If network protocol driver selected, call it with GCC_CONNECT.
	*/
	if (GCdriver_pce)
	{
	    ASYNC_IOCB *iocb = (ASYNC_IOCB *) MEreqmem(0, sizeof(ASYNC_IOCB) + GCA_CL_HDR_SZ + sizeof(CONNECT), FALSE, NULL);
	    if (!iocb)
	    {
		GCTRACE(1)("GCrequest: Failed to allocate ASYNC IOCB block for GCC driver call.\n");
		GCerror(GC_RQ_RESOURCE, ER_alloc);
	    }
	    /*
	    ** Save pointer to iocb so that it can be freed in GCrelease().
	    */
	    asipc->connect_iocb = iocb;
	    iocb->svc_parms = svc_parms;
	    GCDriverRequest (svc_parms, iocb);
	    return;
	}

	/*
	** Bug 79293
	** Create a named semaphore object for synchronising purge messages.
	*/
	if ( GCcreateSemaphore(asipc) == 0 )
	{
	    GCTRACE(1)("GCcreateSemaphore: Failed to create purge semaphore\n");
	    GCerror(GC_RQ_FAIL, 0);
	}

	/*
	** Preallocate IOCBs for pipe send/recv's.
	*/
	asipc->iocbs = (ASYNC_IOCB *)MEreqmem(0, sizeof(ASYNC_IOCB) * 4, TRUE, NULL);
	if (!asipc->iocbs)
	{
	    GCTRACE(1)("GCrequest: Failed to allocate IOCBs (4 for a total size of %d bytes).\n",
		    sizeof(ASYNC_IOCB) * 4);
	    GCerror(GC_RQ_FAIL, 0);
	}

        /* windows95 */
        if (Is_w95)
        {
            GCrequest95(svc_parms);
            return;
        }

        pipe_security.nLength = sizeof(pipe_security);
        pipe_security.lpSecurityDescriptor = NULL;
        pipe_security.bInheritHandle = TRUE;

	/*
	** If a hostname was provided which
	** is different than our current host,
	** combine hostname and address.
	*/
	
	if( svc_parms->partner_host && *svc_parms->partner_host )
	{
	
	    if( STcompare( svc_parms->partner_host, local_host ) )
	    {
		STprintf(buf, "%s%s%s%s","\\\\", svc_parms->partner_host, "\\",
				   svc_parms->partner_name );
	    }
	    else
		STprintf( buf,"%s", svc_parms->partner_name );
	}
	else
	    STprintf( buf,"%s", svc_parms->partner_name );

	/*
	 * Set the names of the pipes - based on partner's name.
	 */
        if (buf[0] == '\\' && buf[1] == '\\')
	{
	    GCqualifiedPipeName( buf, "C2S_STD", asipc->SndStdName );
	    GCqualifiedPipeName( buf, "S2C_STD", asipc->RcvStdName );
	    GCqualifiedPipeName( buf, "C2S_EXP", asipc->SndExpName );
	    GCqualifiedPipeName( buf, "S2C_EXP", asipc->RcvExpName );
	}
	else
	{
            STprintf(asipc->SndStdName, "%s\\C2S_STD\\%s", PIPE_PREFIX, buf );
            STprintf(asipc->RcvStdName, "%s\\S2C_STD\\%s", PIPE_PREFIX, buf);
            STprintf(asipc->SndExpName, "%s\\C2S_EXP\\%s", PIPE_PREFIX, buf);
            STprintf(asipc->RcvExpName, "%s\\S2C_EXP\\%s", PIPE_PREFIX, buf);
	}
	szPipeName = asipc->SndStdName;

	/*
	** Connect to server.
	** If connect does not complete immediately, then spawn a thread
	** to do it in background to avoid tying up the caller's thread...
	** if the request is asynchronous.
	** ERROR_PIPE_BUSY means server is processing another
	** connect request and has not yet created a new instance
	** of the pipe.
	*/
	GCRESRC(asipc->SndStdChan, asipc->SndStdChan =
		CreateFile( szPipeName,
			    GENERIC_WRITE,
			    FILE_SHARE_READ,
			    &pipe_security,
			    OPEN_EXISTING,
			    FILE_FLAG_OVERLAPPED,
			    NULL) );

	if (asipc->SndStdChan == INVALID_HANDLE_VALUE)
	{
	    asipc->SndStdChan = NULL;
	    status = GetLastError();
	    GCTRACE(1)("GCrequest: %p CreateFile INVALID_HANDLE PipeName=%s status=%d\n",
			     svc_parms, szPipeName, status);
	    if (status != ERROR_FILE_NOT_FOUND &&
	        status != ERROR_PIPE_BUSY)
	    {
		GCdestroySemaphore( asipc );
        	GCerror(GC_NO_PARTNER, status);
	    }
	}
	else
	    status = OK;   /* Pipe instance connected to!! */

	/*
	** Decide whether to run rest of the connect directly (call it)
	** or by running it in a separate thread (CreateThread).
	**
	** Call it directly if:
	**   1. GCrequest is SYNC - in that case, it doesn't matter if
	**      latter processing of the connect "blocks" because the caller
	**      doesn't expect to be returned to until the connect is done.
	**   2. GCrequest is ASYNC, but the server pipe was connected to
	**      already (ie, immediately...above).  This means we are already
	**      past the main cause of blocking, so OK to continue in this
	**      thread.
	**
	** Run it under a separate thread if:
	**   1. GCrequest is ASYNC, but the server pipe is not yet available.
	**      The purpose is to avoid blocking this thread; when the
	**      spawned thread is complete, it has the responsibility to
	**      get back in touch with this thread and call the GCrequest
	**      completion routine.
	**
	** The advantage of running directly under this thread is less
	** overhead (ie, better performance), because no need to create
	** and run a separate thread and deal with communication back to
	** this thread when the spawned thread is complete (which is
	** required since the GCrequest completion routine must be invoked
	** from this (the calling) thread).  Since it was found that
	** the above initial attempt to connect is usually successful,
	** even in highly concurrent connect environments, the extra
	** overhead of running in a separate thread is usually avoided.
	*/
	if (sync_flag || asipc->SndStdChan)
	    status = GCrequest2(svc_parms);
	else
	{
	    HANDLE	hThread;
	    DWORD	dwTid;
            asipc->hRequestThread = OpenThread(THREAD_SET_CONTEXT, FALSE,
        		                       GetCurrentThreadId());

	    GCRESRC( hThread, hThread = (HANDLE)_beginthreadex(NULL, GC_STACK_SIZE,
		     (LPTHREAD_START_ROUTINE)GCrequest2,
		     svc_parms, 0, &dwTid) );
            if (hThread != NULL)
            {
                GCRESRC( hThread, CloseHandle (hThread) );
            }
            else
            {
                svc_parms->status = GC_RQ_FAIL;
                status = errno;
		GCTRACE(1)("GCrequest: %p _beginthreadex(GCrequest2) failed. status=%d\n",
			     svc_parms, status);
		GCdestroySemaphore( asipc );
                SETWIN32ERR(svc_parms->sys_err, status, ER_system);
                svc_parms->sys_err->errnum = status;
                (*svc_parms->gca_cl_completion) (svc_parms->closure);
            }
        }

	return;
}


/******************************************************************************
** Name: GCrequest2 - 2nd part of connection processing
**
** Description:
**
** This routine handles the 2nd part of connection processing.
** It may be called directly or run as a separate thread by GCrequest().
**
** The purpose of running under a separate thread is to allow connection
** processing to be run asynchronously from the caller's thread when the
** requested mode is ASYNChronous.  The separate thread is required
** because the pipe connect function is SYNChronous and, hence, will
** block, which is not allowed (bad for performance and could cause hangs).
**
** This routine is called directly if GCrequest mode is SYNC or if the
** initial attempt to connect to the server pipe in GCrequest() completed
** immediately (in which case, initial pipe connection has already occurred).
**
** Whether called directly or in a separate thread, the basic logic is the
** same for completing connection processing.  The difference is how
** the callback of the GCrequest completion routine is handled.
** If called directly, then the completion routine can be called directly.
** If under a different thread, then the initial thread must be notified
** when it is time to run the completion routine.  This is done by
** queuing a User APC to the originating thread with a completion value
** pointing to a routine that will then run the GCrequest completion routine.
**
** Another difference is that the code below (including called functions)
** must be careful not to block if this routine was called directly
** with the GCrequest mode ASYNC.
** 
**
** Inputs:
**   svc_parms              Pointer to GCA service parmeter list containing:
**     gc_cb                  Pointer to the LCB (ASSOC_IPC)
**
** Outputs:
**  svc_parms->size_advise  Suggested buffer size for communication
**                          optimization.
**
**  svc_parms->status       Success or failure indication.
**      OK                  Normal completion
**      GC_RQ_FAIL          Request failure: connection problem
**      GC_RQ_RESOURCE      Request failure: server resource shortage
**
** Returns:
**      OK                  Normal completion
**      non-zero            system status
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Extracted from last part of GCrequest() so that it could be
**	    called directly or run as a separate thread.
**
******************************************************************************/
STATUS
GCrequest2(SVC_PARMS *svc_parms)
{
    STATUS	status = OK;
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    bool	sync_flag = svc_parms->flags.run_sync;

    GCTRACE(3)("GCrequest2: %p Entry point. thread=%p%s %s to server pipe\n",
		svc_parms,
		GetCurrentThreadId(),
		asipc->hRequestThread ? "(spawned)" : "",
		asipc->SndStdChan ? "Already connected" : "Connect");

    svc_parms->status = 0;

    /*
    ** If not already connected, go connect to the server pipe,
    ** which will likely "block".
    */
    if (!asipc->SndStdChan)
    {
	status = GCconnectToServerPipe(svc_parms);
	if (status != OK)
 	    goto GCrequest2_end;
    }

    /*
    ** If all OK, get pipe's buffer size.
    */
    GetNamedPipeInfo(asipc->SndStdChan,
	             NULL,
	             NULL,
	             &svc_parms->size_advise,
	             NULL);

    /*
    ** Connect to remainder of NORMAL and EXPEDITED pipes
    */
    status = GCconnectToStandardPipes(asipc); 
    if (status != OK)
    {
	svc_parms->status = GC_NO_PARTNER;
 	goto GCrequest2_end;
    }

    /*
    ** Send connection peer info data to server
    */
    status = GCrequestSendPeer(svc_parms, NULL);
    if (status != OK)
    {
	svc_parms->status = NP_IO_FAIL;
    }

GCrequest2_end:
    if (status == OK)
	GCrequest_OK++;	/* For internal connect stats */
    else
    {
	GCdestroySemaphore( asipc );
	svc_parms->sys_err->errnum = status;
    }

    /*
    ** Call GCrequest completion routine on thread that called GCrequest.
    ** ...unless GCrequest was ASYNC and still in calling thread, then
    ** just return and the completion will be called by the send peer
    ** completion routine.
    */
    if (asipc->hRequestThread)
    {   
	pfnQueueUserAPC((PAPCFUNC)GCasyncCompletion,
			asipc->hRequestThread, 
                        (DWORD)svc_parms);
	_endthreadex(0);
    }   
    else
    if (sync_flag)
    {   
	(*svc_parms->gca_cl_completion) (svc_parms->closure);
    }   

    return(status);
}



/******************************************************************************
**
** Name: GCrequestSendPeer - Send Connection Peer Info
**
** Description:
**   This routine will send the connection peer info data to the server.
**   If running in a spawned thread or if GCrequest is SYNC, then this
**   routine blocks until the I/O is complete. 
**   Otherwise, when the send completes, sister routine
**   GCrequestSendPeerComplete() is invoked.  This allows the send
**   to be done in a non-blocking manner.
**
** Inputs:
**   svc_parms              Pointer to GCA service parmeter list containing:
**     gc_cb                  Pointer to the LCB (ASSOC_IPC)
**
** Outputs:
**   status                 Success or failure indication:
**      OK                    Normal completion
**      other                 System error from Write to pipe
**
** Returns:
**  status:
**      OK                  Normal completion
**      non-zero            system status
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Extracted from GCrequest() so that it could be called directly
**	    and handle running in the original or a spawned thread.
**	    Also, moved code unrelated to send peer elsewhere.
**	04-Aug-2009 (Bruce Lunsford) Bug 122009 and 122406
**	    After previous fix (for bug 122009), direct connect fails with
**	    error messages E_LC0001 GCA protocol service (GCA_REQUEST) failure
**	    plus E_LQ0001.  Prior fix had failed to set status=OK after
**	    I/O for "sync send" did not complete immediately, which is
**	    normal for direct connects across the network; status was
**	    still set to ERROR_IO_PENDING, even after I/O completed.
**	    Also, enhance error handling to catch all potential errors.
**	    Remove unneeded ResetEvent() since the event handle is closed
**	    when exiting anyway.
**	    Remove unneeded CloseHandle on the send pipe after an error
**	    since it will be closed at end of session anyway (along
**	    with all the other pipes); also, closing it without setting
**	    asipc->SndStdChan to NULL could result in a double close on
**	    the handle.
**	10-Dec-2009 (Bruce Lunsford) Bug 123046 (and 122406)
**	    If send of peer info times out (such as over a WAN), then must
**	    cancel the pending IO before proceeding with error; else,
**	    undefined results later including likely ACCVIO in OS function.
**	    Not a problem prior to fix for bug 122406 because time outs
**	    were ignored and treated as success (not good either, but no
**	    ACCVIO and would generally work OK).
**	    Improve comments for WFSOex of the WriteFile event.
**	    Add #define for SEND_WAIT_TIME_MSECS and replace/remove
**	    misleading SLEEP_TIME_MSECS.  Also bump from 100 to 1000
**	    milliseconds to avoid timeouts over slow WANs; 300 msecs
**	    worked 80% of the time in testing.
**	    Add curly brackets to surround single GCTRACE's within an
**	    else condition since GCTRACE is itself #defined to a multiline
**	    "if x, then y" statement sequence; fixes erroneous matching of
**	    nested if's and else's that follow.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers.  Use/create new format (v2) for
**	    connection peer info (CONNECT) that provides for longer names.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    Send peer info as variable length (only what is needed)
**	    rather than full max fixed length message to make compatible
**	    with Unix and use slightly less (network) overhead.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add prefix filler to connect peer info message area; this is
**	    used by called GC CL lower layers to store GC_MTYP and tcp_ip
**	    2-byte length prefix.
**	    Add optional input parm iocb_in which will be used if not NULL;
**	    needed for driver logic which already has an ASYNC_IOCB.
**	16-Aug-2010 (Bruce Lunsford) Bug 124263
**	    Return OK rather than svc_parms->status after call to
**	    GCDriverSendPeer() since lower layers will handle errors and
**	    call the completion routine appropriately themselves.
**
******************************************************************************/
STATUS
GCrequestSendPeer(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb_in)
{
    STATUS	status = OK;
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    bool	sync_flag = svc_parms->flags.run_sync;
    struct
    {
	char		connect_GCdriver_prefix[GCA_CL_HDR_SZ];
	CONNECT 	connect_block_local;
    } connect_buffer;
    CONNECT	*connect_block;
    GC_RQ_ASSOC2 *connect_block_base;
    char	*out;
    i4  	avail, len;
    ASYNC_IOCB	iocb_local;
    ASYNC_IOCB	*iocb;
    SECURITY_ATTRIBUTES	sa;
    DWORD	bytes_written;

    GCTRACE(3)("GCrequestSendPeer: %p Entry point\n",
		svc_parms);

    /*
    ** If processing an ASYNC GCrequest in the original thread,
    ** then a WriteFileEx() with a completion routine will be invoked
    ** below.  In that case, the message and IOCB must be allocated
    ** rather than obtained from the stack because the stack will be
    ** gone by the time the I/O completion routine runs.  For performance,
    ** the other scenarios continue to get these control blocks from the
    ** local stack.
    */
    if (iocb_in)
    {
	iocb = iocb_in;
	connect_block = (CONNECT *)( (PTR)iocb + sizeof(ASYNC_IOCB) +
			  sizeof(connect_buffer.connect_GCdriver_prefix) );
    }
    else
    if (!sync_flag && !asipc->hRequestThread)
    {
	iocb = (ASYNC_IOCB *) MEreqmem(0, sizeof(ASYNC_IOCB) + sizeof(connect_buffer), FALSE, NULL);
	if (!iocb)
	{
	    GCTRACE(1)("GCrequestSendPeer: Failed to allocate ASYNC IOCB + CONNECT buffer.\n");
	    SETWIN32ERR(svc_parms->sys_err, ~OK, ER_alloc);
	    return(~OK);
	}
	connect_block = (CONNECT *)( (PTR)iocb + sizeof(ASYNC_IOCB) +
			  sizeof(connect_buffer.connect_GCdriver_prefix) );
    }
    else
    {
	iocb = &iocb_local;
	connect_block = &connect_buffer.connect_block_local;
    }

    connect_block_base = &connect_block->v2;

    /*
     * Fill in some connection information
     */

    ZeroMemory( connect_block_base, sizeof(*connect_block_base) );

    connect_block_base->length  = sizeof( *connect_block_base );
    connect_block_base->id      = GC_RQ_ASSOC_ID;
    connect_block_base->version = GC_ASSOC_V2;

    if ( svc_parms->partner_host  &&  *svc_parms->partner_host )
    {
	GCTRACE(2)("GCrequestSendPeer: REMOTE flag set.\n");
	/*
	** Direct remote connection.  Set the REMOTE flag.
	*/
	connect_block_base->flags = GC_RQ_REMOTE;
    }

    /*
    ** connect_block->length is the amount of buffer currently used.
    ** Output position is GC_RQ_ASSOC buffer offset by current used.
    ** The available length is CONNECT2 buffer length less current used.
    */
    out = (char *)connect_block->buffer + connect_block_base->length;
    avail = sizeof( connect_block->buffer ) - connect_block_base->length;

    /*
    ** Save the host name.
    */
    len = STlcopy( local_host, out, avail - 1);
    len++; 				/* include EOS */

    connect_block->v2.length += len;
    connect_block->v2.host_name_len = len;
    out += len;
    avail -= len;

    if ( avail > 0 )
    {
	/* 
	** Who are we ? -- use effective id 
	*/
	GCusername(out, avail);
	len = STlength( out ) + 1;     /* include EOS */

	connect_block->v2.length += len;
	connect_block->v2.user_name_len = len;
	out += len;
	avail -= len;
    }

    if ( avail > 0 )
    {
	/*
	** Save our associated device name.
	*/
	len = STlcopy( "console", out, avail - 1);
	len++; 				/* include EOS */

	connect_block->v2.length += len;
	connect_block->v2.term_name_len = len;
    }

    ZeroMemory( iocb, sizeof(ASYNC_IOCB) );
    iocb->svc_parms = svc_parms;
    iocb->flags &= ~GC_IO_READFILE;

    /*
    ** If the write fails for any reason other than I/O pending, then
    ** we clean up and flag an Association failure.
    */

    /*
    **  If network protocol driver selected, call it with GCDriverSendPeer.
    **  - Return OK here, rather than svc_parms->status, since lower
    **    layers will call the completion routine appropriately
    **    themselves, thus handling errors if they should occur.
    **    In fact, the driver code will likely call GCA back directly
    **    several times before returning (if I/O completes immediately).
    **    Also, if bedchecking the name server, the svc_parms->status
    **    will generally be E_GC0032_NO_PEER and appear to be an error
    **    when it is not.
    */
    if (GCdriver_pce)
    {
	GCDriverSendPeer (svc_parms, connect_block);
	return(OK);
    }

    /*
    ** If processing an ASYNC GCrequest, and if running in the original
    ** thread that called GCrequest(), then the send of the peer info
    ** cannot block.  Hence, issue an async write and schedule a completion
    ** (GCrequestSendPeerComplete()) to run when the IO is complete.
    */
    if (!sync_flag && !asipc->hRequestThread)
    {
	/*
	** Writes with overlapped and expects the completion routine to
	** be called when the I/O completes.
	*/
	if (!WriteFileEx(asipc->SndStdChan,
	                 connect_block,
	                 connect_block->v2.length,
	                 &(iocb->Overlapped),
	                 GCrequestSendPeerComplete))
	{
	    status = GetLastError();
	    if (status == ERROR_IO_PENDING)
		status = OK;
	    else
	    {
		GCTRACE(1)("GCrequestSendPeer: Write (async) failed.\n");
		MEfree((PTR)iocb);
	    }
	}

	return(status);

    }  /* endif async GCrequest in original thread */

    /*
    ** OK to block...GCrequest() is SYNC or running in spawned thread.
    */
    iimksec(&sa);
    GCRESRC( iocb->Overlapped.hEvent,
	     iocb->Overlapped.hEvent = CreateEvent(&sa, TRUE, FALSE, NULL) );

    /*
    ** Write with overlapped, and return to caller when complete with status.
    **
    ** Also note that there is no need to free the iocb in this section
    ** because we are working from a local (in stack) iocb, not an
    ** allocated one here.
    */
    if (!WriteFile(asipc->SndStdChan,
		   connect_block,
		   connect_block->v2.length,
		   &bytes_written,
		   &(iocb->Overlapped)))
    {
	if ((status = GetLastError()) != ERROR_IO_PENDING)
	{
	    GCTRACE(1)("GCrequestSendPeer: Write (sync) failed.\n");
	}
	else
	{
	    #define SEND_WAIT_TIME_MSECS     1000
	    /*
	    ** Wait until the IO event is signaled or we time out.
	    ** Ignore pending async IOs that complete during this time
	    ** and just re-Wait.
	    */
	    while ( (status = WaitForSingleObjectEx(iocb->Overlapped.hEvent,
				          SEND_WAIT_TIME_MSECS, TRUE)) ==
		    WAIT_IO_COMPLETION );
	    if (status == WAIT_OBJECT_0)
	    {
		if (GetOverlappedResult(asipc->SndStdChan, &(iocb->Overlapped),
					&bytes_written, FALSE))
		    status = OK;
		else
		{
		    status = GetLastError();
		    GCTRACE(1)("GCrequestSendPeer: Write (sync) failed with status %d.\n", status);
		}
	    }
	    else if (status == WAIT_TIMEOUT)
	    {
		GCTRACE(1)("GCrequestSendPeer: Write (sync) timed out after %d msecs.\n", SEND_WAIT_TIME_MSECS);
		if ( pfnCancelIo )
                    (*pfnCancelIo)( asipc->SndStdChan );
	    }
	    else
	    {
		GCTRACE(1)("GCrequestSendPeer: Write (sync) wait failed with status %d.\n", status);
	    }
	}
    }
    GCRESRC( iocb->Overlapped.hEvent, CloseHandle(iocb->Overlapped.hEvent) );

    return(status);
}



/******************************************************************************
**
** Name: GCrequestSendPeerComplete - Send Connection Peer Info Completion
**
** Description:
**   This routine is called when the send peer info data has completed.
**   It is invoked by GCrequestSendPeer when implementing non-blocking I/O
**   in the calling thread.
**
** Inputs:
**   dwError                Error code (=0 if I/O successful)
**   dwNumberXfer           Number of bytes sent
**   pOverlapped            Pointer to async I/O Overlapped structure
**
** Outputs:
**   status                 Success or failure indication:
**      OK                    Normal completion
**      other                 System error from Write to pipe
**
** Returns:
**     none
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Created.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Change MEfree(pOverlapped) to MEfree(iocb)...either is correct
**	    since Overlapped is at start of iocb structure, but the latter
**	    is more clear.
**
******************************************************************************/
VOID WINAPI
GCrequestSendPeerComplete(DWORD        dwError,
                          DWORD        dwNumberXfer,
                          LPOVERLAPPED pOverlapped)
{
    ASYNC_IOCB *iocb      = (ASYNC_IOCB *) pOverlapped;
    SVC_PARMS  *svc_parms = (SVC_PARMS *)  iocb->svc_parms;

    GCTRACE(3)("GCrequestSendPeerComplete: %p Entry point\n",
		svc_parms);

    if (dwError)
    {
        svc_parms->status = NP_IO_FAIL;
	svc_parms->sys_err->errnum = dwError;
    }

    /* call the completion routine */
    (*svc_parms->gca_cl_completion) (svc_parms->closure);

    MEfree((PTR)iocb);
    return;
}


/******************************************************************************
**
** Name: GClisten - Establish a server connection with a client
**
** Description:
**
** This routine is, by definition, a server-only function.
**
** This routine establishes a system-specific "read" on the local IPC
** connection previously set up by the GCA_REGISTER service.  This places
** the server in the position of listening for an association request from a
** client.  The completion of the GCA_LISTEN request occurs when a connection
** to the IPC mechanism is made by a client, and a packet of associated data
** is received.  The invoker of GCA_LISTEN is notified of completion by the
** driving of his completion exit, if one was specified and the request was
** asynchronous.  Otherwise, completion is implied by return of control.
**
** It allocates and records in the ACB addressed from the SVC_PARMS structure
** the address of a CL connection control block whose content and structure
** are system-specific.  It is used by this and other CL routines which deal
** with the local IPC connection established by the completion of the
** GCA_LISTEN request.  This data structure is known as the Local Control
** Block (LCB).
**
** In the event of a local failure or resource shortage or other condition
** preventing the successful establishment of the requested connection, there
** should be some mechanism for notifying the client of this so the client does
** not hang on a pending association.  This requires coordination between the
** GClisten and the GCrequest functions.  The exact mechanism is
** system-dependent.
**
** Upon completion, the completion exit routine pointed to by the svc_parms
** parameter list is to be called.  Also, the flag in the ACB called
** acb.starting is set to 0 to indicate that a connection has been established.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** Inputs:
**
**  svc_parms                Pointer to GCA service parmeter list containing:
**   (*gca_cl_completion)()  Routine to call on completion.
**
** Outputs:
**  gc_cb                    CL connection block
**
**  access_point_identifier     A pointer to a character string which
**                              represents the "terminal" through which
**                              the request came.  This is used for
**                              authentication, and is sytem-dependent.
**
**  user_name                   A pointer to a character string which
**                              identifies the client from which
**                              the request came.  This is used for
**                              authentication, and is sytem-dependent.
**
**  size_advise                 Suggested buffer size for communication
**
**  status              Success or failure indication.
**      OK              Normal completion
**      GC_ASSOC_FAIL   A received association request could not
**                      be successfully completed.
**      GC_LSN_FAIL     No GCA register call was done, or some
**                      system-specific failure occurred which
**                      prevented the listen from being
**                      established.
**      GC_LSN_RESOURCE Resource shortage in server
**
**  sys_err             OS-specific status info, returned in
**                      a CL_SYS_ERR structure.
**
** Returns:
**     none
** Exceptions:
**     none
**
** History:
**	19-apr-1999 (somsa01)
**	    Do not create the pipes when GC_IO_CREATED is set.
**      14-Sep-1999 (fanra01)
**          Save the current thread id.  The listen will only complete when
**          called from a GCsync in this thread.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove unused access_point_identifier area at end of asipc used 
**	    to store string version of pid.  Access_point_identifier is
**	    obtained from received peer info (aka, connect) message..
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Preallocate IOCBs for pipe sends and receives (for performance).
**
******************************************************************************/
VOID
GClisten(SVC_PARMS *svc_parms)
{
	DWORD     status;
	ASSOC_IPC *asipc;
	static    bool   init=FALSE;

	GCTRACE(3)("GClisten: %p Entry point.\n",
		svc_parms);

	ListenIOCB.svc_parms = svc_parms;
	ListenIOCB.flags &= ~GC_IO_READFILE;
        ListenIOCB.lstnthread = GetCurrentThreadId();

	if(init==FALSE)
	{
	    PM_SCAN_REC state;
	    char        *regexp, *scan_name, *scan_value;
	    STATUS      status;
	    USER_LABEL  *ul;
	    init=TRUE;

# ifdef MCT
	    gen_Psem(&GC_misc_sem);
# endif /* MCT */
	    (void)PMinit(); /* Make sure PM is initialized */
	    /*
	    ** Initialize list of subject labels.
	    ** We do this here since PM can't currently be called in the
	    ** listen completion routine.
	    */
	    regexp=PMexpToRegExp("ii.$.secure.subject_label.%");
	    for(status=PMscan(regexp, &state, NULL,
			&scan_name, &scan_value); status==OK;
		status=PMscan(NULL, &state, NULL, &scan_name, &scan_value))
	    {
		char *user_name;
		char *PMgetElem(); /* Not in pm.h yet */

		/*
		** Extract user name
		*/
		user_name = PMgetElem(4, scan_name);
		/*
		** Save for later use
		*/
		ul = (USER_LABEL*)MEreqmem(0,sizeof(*ul),0,NULL);
		if(ul == NULL)
		    continue;
		ul->user_name = user_name;
		ul->security_label = scan_value;
		ul->next = ul_root;
		ul_root = ul;
	    }
# ifdef MCT
	    gen_Vsem(&GC_misc_sem);
# endif /* MCT */
	}

	/* If we are already listening, use the
	** asipc from the ListenIOCB. Otherwise
	** create a new asipc.
	*/

	if ((ListenIOCB.flags & GC_IO_LISTEN) ||
	    (ListenIOCB.flags & GC_IO_CREATED) )
	{
	    asipc = ListenIOCB.asipc;
	}
	else
	{
	    /*
	    ** Get an ipc block and inialize it
	    */
	    asipc = (ASSOC_IPC *)MEreqmem( 0, sizeof(ASSOC_IPC), TRUE, NULL);
	    if ( ! asipc )
	    {
		GCTRACE(1)("GClisten: Failed to allocate an IPC block.\n");
		GCerror(GC_LSN_FAIL, 0);
	    }
 
	    asipc->flags |= GC_IPC_LISTEN;
	    ListenIOCB.asipc = asipc;

	    if (!GCdriver_pce)
	    {
		/*
		** Preallocate IOCBs for pipe send/recv's.
		*/
		if (asipc->iocbs)
		{
		}
		asipc->iocbs = (ASYNC_IOCB *)MEreqmem(0, sizeof(ASYNC_IOCB) * 4, TRUE, NULL);
		if (!asipc->iocbs)
		{
		    GCTRACE(1)("GClisten: Failed to allocate IOCBs (4 for a total size of %d bytes).\n",
			    sizeof(ASYNC_IOCB) * 4);
		    GCerror(GC_LSN_FAIL, 0);
		}
	    }
	}

	svc_parms->gc_cb                = (PTR) asipc;

	/* If we are already listening, do not issue a listen.
	** Start a new timer and set the TIMEOUT flag and return.
	*/ 
	if ( ListenIOCB.flags & GC_IO_LISTEN )
	{
	    if ( svc_parms->time_out >= 0 &&
		 ! (ListenIOCB.flags & GC_IO_TIMEOUT) )
	    {
                if ( !Is_w95 )
		    GCsetWaitableTimer( svc_parms );
		ListenIOCB.flags |= GC_IO_TIMEOUT;
	    }
	    return;
	}

	/*
	**  If network protocol driver selected, call it with GCC_LISTEN.
	*/
	if (GCdriver_pce)
	{
	    status = GCDriverListen (svc_parms, &ListenIOCB);
	    return;
	}

        /* windows95 */
        if (Is_w95)
        {
        HANDLE hThread;
        DWORD dwTid;

            GCRESRC( hThread, hThread = (HANDLE)_beginthreadex(NULL, GC_STACK_SIZE,
                (LPTHREAD_START_ROUTINE)GClisten95,
                svc_parms, 0, &dwTid) );
            if (hThread!= NULL)
            {
                GCRESRC( hThread, CloseHandle (hThread) );
            }
            else
            {
                svc_parms->status = GC_LSN_FAIL;
                status = errno;
                SETWIN32ERR(svc_parms->sys_err, status, ER_system);
                svc_parms->sys_err->errnum = status;
                (*svc_parms->gca_cl_completion) (svc_parms->closure);
            }
            return;
        }

        /* Named Pipes */
	{
	    if (!(ListenIOCB.flags & GC_IO_CREATED))
	    {
		status = GCcreateStandardPipes( asipc );
		if ( status != OK )
		{
		    GCerror(GC_LSN_FAIL, status);
		}
		ListenIOCB.flags |= GC_IO_CREATED;
	    }

	    /*
	    ** Server sits waiting for a client to connect.
	    */
	    for(;;)
	    {
		if( ! ConnectNamedPipe( asipc->RcvStdChan, 
					&ListenIOCB.Overlapped ) )
		{
                    status = GetLastError();

		    if (status == ERROR_IO_PENDING)
		    {
			if ( svc_parms->time_out >= 0 &&
			     ! (ListenIOCB.flags & GC_IO_TIMEOUT) )
			{
                            if ( !Is_w95 )
			        GCsetWaitableTimer( svc_parms );
			    ListenIOCB.flags |= GC_IO_TIMEOUT;
			}

			ListenIOCB.flags |= GC_IO_LISTEN;
			break;
		    }

                    if (status == ERROR_PIPE_CONNECTED)
		    {
			SetEvent (hAsyncEvents[LISTEN]);
			break;
		    }

                    /*
                    ** If the named pipe is still connected from
                    ** the previous client, then disconnect it
                    ** and record the error.
                    ** The next connectnamedpipe should then work
                    */
                    if (status == ERROR_NO_DATA) /* pipe being closed */
		    {
			DisconnectNamedPipe(asipc->RcvStdChan);
			continue;
		    }

		    GCerror (GC_LSN_FAIL, status);
		} /* end if ! ConnectNamedPipe() */
            } /* end for loop waiting for client to connect */
	} /* end Named Pipes */
}


/******************************************************************************
**
** Name: GClisten2 - Listen phase 2
**
** Description:
**
** This is phase/part 2 that GClisten uses to connect a server to a client.
** An incoming network connection has been established.  Now issue read
** for connection data.
**
** History:
**      09-Sep-1999 (fanra01)
**          Check that we are in the listening threads context before we
**          continue.
**          Call GCcloseAllPipes on semaphore failure.
**          Changed overlapped event to manual reset. Add manual reset.
**	13-sep-2004 (somsa01)
**	    As an addendum to loera01's last change, lower-case the userid
**	    after obtaining it from the Pipe.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers.  Check for new format (v2) for
**	    connection peer info (CONNECT) that provides for longer names.
**	    Keep support for old format (V0) for backward compatibility with
**	    older clients...mostly needed for direct remote connections.
**	    Added function name, where missing, to start of GCTRACE stmts.
**	    Remove ultoa() of pid to unused access_point_identifier area at
**	    end of asipc; access_point_identifier is obtained from received
**	    peer info (aka, connect) message.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    Pipe owner not returned as user for new long-name peer info fmt.
**	    With prior fix, user_name_len was never set (was still = 0)
**	    if new peer info format received.  This caused 
**	    GetNamedPipeHandleState() call to be a no-op.  Allocate a new
**	    area to store the pipe owner, which may be too long to overlay
**	    into incoming peer info message's user name field.  Added trace
**	    if the userids don't match.
**	    Tighten up check to determine if incoming peer info is in new or
**	    original format since new format is now being sent as variable
**	    length; ie, add check for constant value in new format's id field.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    With drivers, this function is now called directly instead
**	    of due to the setting of the LISTEN event.  In the driver
**	    direct-call scenario, no need to check if this is a listen
**	    thread...will be checked later in driver's LISTEN event_func,
**	    which is GClisten2_complete().
**	    Split out common code at end to a separate function,
**	    GClisten2_complete(), that can also be called/shared
**	    by a driver listen at the end of its processing,
**	    as well as by the inline pipe processing below.
**	    Cleaned up handling of Overlapped.hEvent.
**	28-May-2010 (Bruce Lunsford) Bug 123827
**	    Lowercase the pipe owner since userid is not case-sensitive
**	    on Windows (and to be consistent with GCusername).  Otherwise,
**	    can get a trace WARNING in listen complete when pipe owner
**	    doesn't match the user sent by the client when Ingres run
**	    as a service (SYSTEM vs system).
******************************************************************************/
VOID
GClisten2() 
{
	ASYNC_IOCB *iocb      = &ListenIOCB;
	SVC_PARMS  *svc_parms = iocb->svc_parms;
	ASSOC_IPC  *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
	STATUS     status     = OK;
	OVERLAPPED Overlapped;
	SECURITY_ATTRIBUTES sa;

	GCTRACE(3)("GClisten2: %p Entry point.\n",
		svc_parms);

	/*
	** If we were called due to the LISTEN event being signaled,
	** then verify we're in the correct thread.
	*/
	if (GCevent_func[LISTEN] == GClisten2)
	{
          if (ListenIOCB.lstnthread == GetCurrentThreadId())
          {
            /*
            **  Listen threads context
            */
            ResetEvent( ListenIOCB.Overlapped.hEvent );
            GCTRACE(3)("GClisten2: Listening thread continue\n");
          }
          else
          {
            /*
            ** Woke up in another threads GCsync. GCsync will re-issue the
            ** wait.
            ** Leave the event set so that the listening thread will also
            ** get woken up.
            **
            */
            GCTRACE(3)("GClisten2: Not listening thread\n");
            return;
          }
	}

        iocb->flags &= ~GC_IO_CREATED;
        iocb->flags &= ~GC_IO_LISTEN;

	/*
	** Cancel the TIMEOUT timer if Listen
	** completed successfully.
	*/
	if ( iocb->flags & GC_IO_TIMEOUT )
	{
	    GCTRACE(3)("GClisten2: Cancel on Waitable timer.\n");

            if ( !Is_w95 && pfnCancelWaitableTimer )
	        (*pfnCancelWaitableTimer)( hAsyncEvents[TIMEOUT] );
	    iocb->flags &= ~GC_IO_TIMEOUT;
	}

	/*
	** Check to see if the Server pipe is NULL.  If it is, we're shutting
	** down so just return back to GCexec().
	** Also, comm server can call here after a listen failure.
	*/

	if ( (asipc == NULL) ||
	     (!GCdriver_pce && asipc->RcvStdChan == NULL) )
	    return;

	asipc->connect_recv_numxfer = 0;

	/*
	**  If network protocol driver selected, call it to receive the
	**  connection peer info data.
	*/
	if (GCdriver_pce)
	{
	    GCDriverListen2 (svc_parms);
	    return;
	}

	/*
	** Bug 79293
	** Create a named semaphore object for synchronising purge messages.
	*/
	if ( GCcreateSemaphore(asipc) == 0 )
	{
	    GCTRACE(1)("GCcreateSemaphore: Failed to create purge semaphore\n");
            GCcloseAllPipes( asipc );
	    GCerror(GC_ASSOC_FAIL, GetLastError());
	}


	/****************************
	 * Read the connection data *
	 ****************************/

	iimksec (&sa);

	memset(&Overlapped, 0, sizeof(OVERLAPPED));

	GCRESRC( Overlapped.hEvent,
            Overlapped.hEvent = CreateEvent(&sa, FALSE, FALSE, NULL) );
        if (Overlapped.hEvent == NULL)
	{
	    status = GetLastError();
	    GCcloseAllPipes(asipc);
	    GCTRACE(1)("GClisten2: CreateEvent status = %0x %d\n", status, status);
	    GCerror(GC_LSN_FAIL, status);
	}

	if (!ReadFile(asipc->RcvStdChan,
		          &asipc->connect,
		          sizeof(asipc->connect),
		          &asipc->connect_recv_numxfer,
		          &Overlapped))
	{
	    switch(status = GetLastError())
	    {
		case ERROR_IO_PENDING:
		    WaitForSingleObject(Overlapped.hEvent, INFINITE);
                    GetOverlappedResult( asipc->RcvStdChan, &Overlapped,
                        &asipc->connect_recv_numxfer, TRUE);
		    break;

		default:
	            GCRESRC( Overlapped.hEvent, CloseHandle(Overlapped.hEvent) );
		    GCcloseAllPipes(asipc);
	            GCTRACE(1)("GClisten2: ReadFile status = %0x %d\n", status, status);
	    	    GCerror(GC_LSN_FAIL, status);
	    }
	}

	GCRESRC( Overlapped.hEvent, CloseHandle(Overlapped.hEvent) );

	/*
	** Get the pipe owner name since this is more reliable and
	** trustworthy than the user name sent by the client in the
	** CL GC peer info message.  While the 2 user names
	** will normally be the same, maintain them separately as
	** they may be different and of different lengths.
	** We can no longer overlay directly onto the client-sent 
	** user id field, because the pipe owner name may be longer.
	** Store the pipe owner into a separate fixed-length field
	** guaranteed to be long enough; link it to the asipc since
	** it must be persisted memory because a pointer to it is
	** returned to the caller.
        */
	if ( !asipc->ClientSysUserName )
	{
	    asipc->ClientSysUserName = MEreqmem( 0, GC_USERNAME_MAX + 1, FALSE, NULL);
	    if ( ! asipc->ClientSysUserName )
	    {
		GCTRACE(1)("GClisten2: Failed to allocate memory for client user name (pipe owner).\n");
		GCerror(GC_LSN_FAIL, 0);
	    }
	}

	*asipc->ClientSysUserName = EOS;	/* Init as empty string */
        GetNamedPipeHandleState(asipc->RcvStdChan,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      asipc->ClientSysUserName,
                      GC_USERNAME_MAX + 1);
	_strlwr(asipc->ClientSysUserName);

	/*
	** Advise user of optimum size *
	*/
	svc_parms->size_advise = Pipe_buffer_size;

	GClisten2_complete();
}


/******************************************************************************
**
** Name: GClisten2_complete - Listen phase 2 COMPLETION
**
** Description:
**
** This is the completion routine for GClisten_2.  It is called when the
** connection data from the client has been received or an error occurred
** on the read.  Check for errors; if OK, process the incoming connection
** data, which should be in one of several release-dependent formats.
** Lastly, call the GClisten caller's completion routine to complete
** listen processing.
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created...extracted from last half of GClisten2() so it
**	    could be called by driver and pipes code.  Made this the
**	    event_func[LISTEN] for drivers (instead of GClisten2) to
**	    deal with the way the DBMS in CSsuspend() treats the
**	    signaling of the LISTEN event.  In addition to invoking
**	    the associated function, it signals to the DBMS that the
**	    listen has completed when the associated function returns.
**
**	    Added support for incoming previous Unix/Linux peerinfo
**	    formats V0 and V1 since direct connect is now supported
**	    from those non-Windows platforms if using TCP_IP as the
**	    local IPC.  Due to this "opening up" of direct connect
**	    access, the same protections that exist on Unix servers
**	    was added for Windows.  That is, direct connect from
**	    remote machines must have direct connect enabled in the
**	    DBMS server configuration.
**
**	    Reworked code a bit by adding "for(;;)" wrapper and using
**	    break statements instead of using GCerror() macro.  Helps
**	    a little with the flow (and matches better with Unix).
**
**	    Also reinstated setting of "trusted" flag, modeled after
**	    Unix, and removed corresponding commented-out code that
**	    existed before for Windows.
**	    
******************************************************************************/
VOID
GClisten2_complete() 
{
	ASYNC_IOCB *iocb      = &ListenIOCB;
	SVC_PARMS  *svc_parms = iocb->svc_parms;
	ASSOC_IPC  *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
	ULONG      numxfer    = asipc->connect_recv_numxfer;
	CS_SCB	   *scb;

	GCTRACE(3)("GClisten2_complete: %p Entry point.\n",
		svc_parms);

	/*
	** If we were called due to the LISTEN event being signaled,
	** then verify we're in the correct thread.
	*/
	if (GCevent_func[LISTEN] == GClisten2_complete)
	{
          if (ListenIOCB.lstnthread == GetCurrentThreadId())
          {
            /*
            **  Listen threads context
            */
	    ResetEvent( hAsyncEvents[LISTEN] );
            GCTRACE(3)("GClisten2_complete: Listening thread continue\n");
          }
          else
          {
            /*
            ** Woke up in another threads GCsync. GCsync will re-issue the
            ** wait.
            ** Leave the event set so that the listening thread will also
            ** get woken up.
            **
            */
            GCTRACE(3)("GClisten2_complete: Not listening thread\n");
            return;
          }
	}

	Gc_reads++;
	CSGET_SCB(&scb);
	if (scb)
	    scb->cs_bior++;

	/*
	** Fill in the trusted flag and sec_label
	*/
        svc_parms->sec_label               = (char *)&asipc->sec_label;

	/*
	** Process incoming peer info structure, which may be in one
	** of several formats.  Determine the format and then handle
	** accordingly.
	*/
	GCTRACE(2)( "GClisten2_complete: %p Received peer info of length %d\n",
		    svc_parms, numxfer );

    for(;;)      /* ...allows "break" at error conditions */
    {
	if ( svc_parms->status != OK )
	{
	    svc_parms->status = GC_ASSOC_FAIL;
	    break;
	}

	/*
	** Original peer info is identified by a fixed data length,
	** and the lack of GC_RQ_ASSOC_ID in the id field.
	*/
	if ( (numxfer != sizeof(CONNECT_orig))  ||
	     (asipc->connect.v2.id == GC_RQ_ASSOC_ID) )
	{
	    /*
	    ** The first three peer info fields are identical
	    ** in all peer versions other than the original.
	    ** Identify and validate peer info version.
	    */
	    GC_RQ_ASSOC2	*peer2 = &asipc->connect.v2;
	    char	 	*host, *user, *tty;
	    u_i4		flags = 0;

	    if ( peer2->length > (i4)numxfer )
	    {
		GCTRACE(1)( "GClisten2_complete %p: invalid peer length: %d\n", 
			    svc_parms, peer2->length );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }

	    if ( peer2->id != GC_RQ_ASSOC_ID )
	    {
		GCTRACE(1)( "GClisten2_complete %p: invalid peer info ID: 0x%x\n", 
			    svc_parms, peer2->id );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }

	    GCTRACE(2)( "GClisten2_complete %p: peer info v%d: length %d\n",
	    		svc_parms, peer2->version, peer2->length );

	    /*
	    ** Since named pipes is the only supported local IPC on Windows,
	    ** only the original and the V2 formats can be received.
	    */
	    switch( peer2->version )
	    {
	    case GC_ASSOC_V0 :
	    {
		GC_RQ_ASSOC0	*peer0 = &asipc->connect.v0;

		if ( peer0->length < sizeof( GC_RQ_ASSOC0 ) )
		{
		    GCTRACE(1)( "GClisten2_complete %p: invalid peer v0 len: %d\n",
				svc_parms, peer0->length );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = peer0->host_name;
		user = peer0->user_name;
		tty = peer0->term_name;
	    	break;
	    }
	    case GC_ASSOC_V1 :
	    {
		GC_RQ_ASSOC1	*peer1 = &asipc->connect.v1;

		if ( peer1->rq_assoc0.length < sizeof( GC_RQ_ASSOC1 ) )
		{
		    GCTRACE(1)( "GClisten2_complete %p: invalid peer v1 len: %d\n",
				svc_parms, peer1->rq_assoc0.length );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = peer1->rq_assoc0.host_name;
		user = peer1->rq_assoc0.user_name;
		tty = peer1->rq_assoc0.term_name;
		flags = peer1->flags;
		break;
	    }
	    case GC_ASSOC_V2 :
	    {
		/*
		** Host, user, and terminal names follow, in that
		** order, the fixed portion of peer info structure.
		*/
		i4 length = sizeof( GC_RQ_ASSOC2 ) + peer2->host_name_len +
			    peer2->user_name_len + peer2->term_name_len;

		if ( peer2->length < length )
		{
		    GCTRACE(1)( 
		    "GClisten2_complete %p: invalid peer v2 len: %d (%d,%d,%d)\n",
				svc_parms, peer2->length, peer2->host_name_len,
				peer2->user_name_len, peer2->term_name_len );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = (char *)peer2 + sizeof( GC_RQ_ASSOC2 );
		user = host + peer2->host_name_len;
		tty = user + peer2->user_name_len;
		flags = peer2->flags;
		break;
	    }
	    default :
	    {
		GCTRACE(1)( "GClisten2_complete %p: unsupported peer version: %d\n",
			    svc_parms, peer2->version );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }
	    } /* end switch on peer2_version */

	    if ( svc_parms->status != OK )  break;

	    GCTRACE(2)
		( "GClisten2_complete %p: flags 0x%x, host %s, user %s, tty %s\n",
		  svc_parms, flags, host, user, tty );

	    svc_parms->user_name = user;
	    svc_parms->account_name = user;
	    svc_parms->access_point_identifier = tty;

	    /*
	    ** Clients are only trusted if they are on the
	    ** same host and have the same user ID.
	    **
	    ** If remote, check to see if remote access is
	    ** disabled or restricted.
	    **
	    ** For pipes (!GCdriver_pce), there is no way until Windows
	    ** Vista or Server 2008, when function
	    ** GetNamedPipeClientComputerName() is available,
	    ** to determine the client's computer name from the protocol.
	    ** Instead, just use what was sent in the peer info to
	    ** determine if the client was local or remote.
	    */
	    if ( !GCdriver_pce && STcompare( host, local_host ) )
		asipc->flags |= GC_IPC_IS_REMOTE;

	    if ( ! (asipc->flags & GC_IPC_IS_REMOTE) )  /* if local */
	    {
		if ( STcompare( host, local_host ) )
		{
		    GCTRACE(1)( "GClisten2_complete %p: invalid host: %s!\n", 
				svc_parms, host );
		    svc_parms->status = GC_RMTACCESS_FAIL;
		    break;
		}

		svc_parms->flags.trusted = ! STcompare( user, gc_user_name );
	    }
	    else  if ( remote_access != GC_RA_ALL  && 
		       remote_access != GC_RA_ENABLED )
	    {
		/*
		** Remote access through GC is not allowed.
		*/
		GCTRACE(1)( "GClisten2_complete %p: remote access not enabled!\n", 
			    svc_parms );
		svc_parms->status = GC_RMTACCESS_FAIL;
		break;
	    }
	    else  if ( remote_access == GC_RA_ENABLED  &&
	    	       ! (flags & GC_RQ_REMOTE) )
	    {
		/*
		** Uncontrolled remote access is not allowed.
		*/
		GCTRACE(1)( "GClisten2_complete %p: invalid remote GC access\n", 
			    svc_parms );
		svc_parms->status = GC_RMTACCESS_FAIL;
		break;
	    }
	}
	else
	{
	    /*
	    ** This is an old style peer info packet from the
	    ** Windows GC CL days which is not compatible with
	    ** other platforms, such as Linux.
	    */
	    GCTRACE(2)( "GClisten2_complete %p: peer info v_orig: pid %d\n",
	    		svc_parms, asipc->connect.v_orig.pid);
	    svc_parms->user_name               = asipc->connect.v_orig.userid;
	    svc_parms->account_name            = asipc->connect.v_orig.account;
	    svc_parms->access_point_identifier = asipc->connect.v_orig.access_point;

	    GCTRACE(2)( "GClisten2_complete %p: flag 0x%x, account %s, user %s, term %s\n",
			svc_parms, asipc->connect.v_orig.flag,
			svc_parms->account_name,
			svc_parms->user_name,
			svc_parms->access_point_identifier );
	    /*
	    ** NOTE: No checking of remote access being enabled is done here
	    **       for old style peer info's because we cannot determine if
	    **       the client is local or remote (pipes pre-Windows Vista
	    **       won't tell us and the client's computer name is not passed
	    **       in the peer info message). Also, this maintains backward
	    **       compatibility with older clients; once they upgrade
	    **       though, the server will need II_GC_REMOTE set
	    **       appropriately. This is not a big security exposure
	    **       since old style peerinfo will only come in via named
	    **       pipes from Windows clients in the same Windows domain;
	    **       such is not the case for tcp_ip.
	    */

	}  /* end old-style Windows (pre long identifiers) peer info */

        /*
	** If available, use the client name retrieved from the system
	** instead of the one received in the peer info message.  For the
	** named pipes protocol, the system name will contain the pipe owner.
	** For tcp/ip, no equivalent exists, so must just use the name in the
	** message.
	** The system and message client user names "should" be the same;
	** however, the pipe owner, for instance, is a more reliable
	** source (since it is from the OS) than what the client sent,
	** so provides a bit of extra security.
	** Trace if they happen to be different and, although not rejecting
	** the connection (though perhaps we should), at least turn "trusted"
	** off.
	*/
	if ( asipc->ClientSysUserName )
	{
	    /*
	    ** If tracing on, compare the names and trace if different.
	    */
	    if ( GC_trace &&
		STcompare(svc_parms->user_name, asipc->ClientSysUserName) )
	    {
		GCTRACE(1)( "GClisten2_complete %p: WARNING! Client-sent user(%s) not same as pipe owner(%s)!\n",
		     svc_parms, svc_parms->user_name, asipc->ClientSysUserName);
		svc_parms->flags.trusted = FALSE;
	    }
	    svc_parms->user_name = asipc->ClientSysUserName;
	}

	_strlwr(svc_parms->user_name);

	break;

    }  /* end for(;;) */

    /*
    ** Call completion routine
    */
    GCTRACE(3)( "GClisten2_complete %p: Listen complete. status %x\n",
                svc_parms, svc_parms->status );

    (*svc_parms->gca_cl_completion)( svc_parms->closure );
}

/******************************************************************************
** Name: GCdisconn  - Clear and force completion of outstanding requests.
**
** Description:
**  Force completion of any outstanding send or receive requests on the
**  specified association.  These are "orphan" requests resulting from
**  an unexpected failure of the association, or from normal release of
**  an association which still has requests outstanding.
**  There is no deallocation of association resources.  The LCB pointed to
**  by the ACB contains the system-specific information required to perform
**  the function.
**
** Inputs:
**      svc_parms       pointer
**
** Outputs:
**      none
**  Returns:
**      none
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      12-Nov-96 (fanra01)
**          Removed the DisconnectNamedPipe so that disconnect on NT reflects
**          the actions of Windows 95.
**      02-jun-1997 (canor01)
**          Try to get disconnect message delivered before closing
**          the pipes.
**      17-Sep-1999 (fanra01)
**          Modified the disconnect which ensures that the higher level
**          message is read by the partner before the pipes are diconnected.
**          Tries to minimise the number of calls to SleepEx to improve
**          performance.
**      01-Oct-1999 (fanra01)
**          Add asyncIoPending counter to determine if IOs have completed.
**          When the last IO completes the asyncIoComplete event is signalled
**          allowing GCdisconn to continue.  This removes the need to use
**          SleepEx to make the thread alertable.
**      03-Dec-1999 (fanra01)
**          Close the receive handles if this is a comms server to prevent peer
**          from sending anymore data and to force outstanding I/Os to
**          complete.
**          Modified the function to wait for all outstanding I/Os to complete
**          before continuing with the disconnect.
**          Allow disconnect function to return and call completion
**          asynchronously.
**      31-Jan-2000 (fanra01)
**          Close the receive handles.
**      31-Jan-2000 (fanra01)
**          Bug
**          If the listener is terninating the connection close the receive
**          handles to force the outstanding async I/Os to complete.
**      01-Jul-2005 (lunbr01) Bug 109115
**          "Direct" connections fail against gateway servers due to the
**          disconnect issued by gateway daemon after handing off session
**          to gateway slave process.  The fix is to not write a
**          0 byte message to the send pipe if it is a gateway daemon
**          process.  This was actually implemented more generically->
**          if we are a server (listening) and have GCsave'd our connection
**          (presumably for use by another process, such as a
**          gateway slave), then skip the write.
**          This problem was timing dependent-- ie, didn't always occur.
**          May be a better way to eliminate the failure (note that
**          the application thinks session has been ended if he gets
**          a 0 byte message).
**      26-Aug-2005 (lunbr01) Bug 115107 + 109115
**          Out of stack memory error causes GCC to crash.  Eliminate nested
**          calls to async IO completion routines which Microsoft says can
**          chew up memory in the stack.  This condition can occur in
**          GCdisconn() when triggered by a killed application (failed recv).
**          Removed the WFSOex() loop invoked here, which was used to allow
**          pending IOs to complete. This was removed because we might be
**          executing in the context of an async completion routine (eg, a
**          failed receive).  Instead, just return and let GCasyncDisconn()
**          be invoked later from GCCompletionRoutine() when the session's
**          async IO pending count reaches zero.  The previous fix for
**          "direct connect" (bug 109115) is no longer needed because we
**          no longer need to do the 0-byte write either.
**	10-Dec-2009 (Bruce Lunsford) Bug 123046 (and 122406)
**	    Add some tracing.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If net driver selected, call it with GCDriverDisconn instead of
**	    embedded named pipes logic.
******************************************************************************/

VOID
GCdisconn(SVC_PARMS *svc_parms)
{
    ASSOC_IPC *asipc = (ASSOC_IPC *)svc_parms->gc_cb;

    GCTRACE(3)("GCdisconn: %p Entry point.\n", svc_parms);

    if (asipc != NULL)
    {
        /*
        ** Set flag to indicate that GCdisconn has been called.
        ** Checked in GCcompletionRoutine.
        */
        asipc->flags |= GC_IPC_DISCONN;

        /*
        ** Drive all queued I/O to completion.
        */
        if (is_comm_svr)
            asipc->flags |= GC_IPC_NETCOMPL;

	/*
	**  Disconnect using appropriate protocol.
	*/
	if (GCdriver_pce) /* Network Driver */
	{
	    GCDriverDisconn (svc_parms);
	    return;
	}

        if (!Is_w95)
        {
            GCcloseAllPipes( asipc );
        }
        else
        {
            CloseAsc( asipc );
        }

        GCTRACE(5)("GCdisconn: Pipes closed.  IO pending=%d\n",
		    asipc->asyncIoPending);
        /*
        ** If no outstanding IOs, call completion routine.
        ** Otherwise, it will be called when the last IO completes.
        */
        if (asipc->asyncIoPending == 0)
        {
            GCTRACE(3)("GCdisconn: Disconnect complete.\n");
            GCerror(OK, 0);
        }
       
        /*
        ** Save required info to complete the disconnect later
        ** when all I/Os have completed.
        */
        asipc->hDisconnThread = OpenThread(THREAD_SET_CONTEXT,
            FALSE, GetCurrentThreadId());
        asipc->Disconn_svc_parms = svc_parms;

    }
    GCTRACE(3)( "GCdisconn: Disconnect phase 1 complete.\n" );
    return;
}

/******************************************************************************
** Name: GCrelease - Release connection.
**
** Description:
**
** Clear outstanding requests and the connection for a particular
** association.  Release system specific resources assigned to the
** association, including the LCB.
**
** The number of asynchronous requests still outstanding is contained in
** the ACB.  This count is decremented when a request completes.  It may
** be used as desired to determine whether all requests are complete.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** We now assume that by the time GCdisconn completes (i.e. before
** GCrelease is called) that all async events have been cleared.
** At least as of the time of the writing, GCrelease is called in the
** callback routine of GCdisconn.  Just in case you wondered.
**
** Inputs:
**  gc_cb        CL communication block requests.
** Outputs:
**  none
** Returns:
**     none
** Exceptions:
**     none
** Side Effects:
**     none
** History:
**      03-Dec-1999 (fanra01)
**          Close the asyncIoComplete and all the pipes.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Use local stack variable 'asipc' since it was set.  Also,
**	    reorg'd the code a bit to ensure no ACCVIO if asipc==NULL.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    Free client Pipe owner (user name) if present.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If a driver was selected (ie, if a gcb ptr is in the asipc),
**	    then free the gcb memory and the connect IOCB. Also free memory
**	    for pipe IOCBs which are now preallocated at start of connection.
**	    Close hRequestThread handle if exists.
******************************************************************************/
VOID
GCrelease(SVC_PARMS * svc_parms)
{
    ASSOC_IPC *asipc = (ASSOC_IPC *)svc_parms->gc_cb;

    GCTRACE(3)("GCrelease: %p Entry point.\n", svc_parms);

    svc_parms->status = OK;

    if (!asipc)		/* Remainder of logic depends on asipc present */
	return;

    if (asipc->hRequestThread)
        CloseHandle(asipc->hRequestThread);

    if (asipc->hDisconnThread)
        CloseHandle(asipc->hDisconnThread);

    if ( asipc->ClientSysUserName )
        MEfree((PTR)asipc->ClientSysUserName);

    GCdestroySemaphore(asipc);

    if ( (asipc == ListenIOCB.asipc) &&
         (ListenIOCB.flags & GC_IO_LISTEN || ListenIOCB.flags & GC_IO_CREATED) )
    {
        GCTRACE(3)("GCrelease: %p Reusing IPC %p ListenIOCB.flags=%d\n",
		    svc_parms, asipc, ListenIOCB.flags);
        /* Save control block, will be re-used on next GClisten() */
    }
    else
    {
        GCTRACE(4)("GCrelease: %p Freeing gcb=%p iocbs=%p connect_iocb=%p IPC=%p, ListenIOCB.flags=%d\n",
		    svc_parms, asipc->gca_gcb,
		    asipc->iocbs, asipc->connect_iocb,
		    asipc, ListenIOCB.flags);

	if (asipc->gca_gcb)  /* protocol driver used */
	    MEfree(asipc->gca_gcb);
	else   /* must be pipes */
	{
            if (!Is_w95)
        	GCcloseAllPipes(asipc);
            else
        	CloseAsc(asipc);
	}
	if (asipc->iocbs)
	{
	    MEfree(asipc->iocbs);
	}
	if (asipc->connect_iocb)
	{
	    MEfree(asipc->connect_iocb);
	}
        MEfree((PTR)asipc);
    }
    svc_parms->gc_cb = NULL;
}


/******************************************************************************
** Name: GCsend - Send data on an association
**
** Description:
** Send data to the other party in the connection.  This routine simulates
** byte-stream I/O, and utilizes the system-specific IPC mechanism in whatever
** way is necessary to create this illusion.
**
** The user of this routine has requested svc_parms->svc_send_length bytes
** of data to be sent on the outgoing message stream.  This is to be done in a
** single operation from the caller's point of view; i.e., the operation is
** not complete until the caller's entire buffer has been sent.  The exact
** meaning of "sent" is implementation specific, but the data in the caller's
** buffer must be in a safe harbor, and the caller must be able to re-use the
** buffer.  The data sent as the outgoing message stream is read from the
** address starting at svc_parms->svc_buffer.
**
** This routine must issue the sequence of write requests
** necessary to send the entire buffer, and track any excess outgoing
** data not yet requested or needed.  The breaking down of the
** input data buffer into transmission segment sizes is performed
** here.
**
** The parameter svc_parms->(*gca_cl_completion)() specifies a
** completion exit to be invoked when the request is complete.  This exit is
** called for either synchronous or asynchronous requests.  The invocation of
** this exit implies to the mainline that the send buffer may now be reused.
**
** Possible error conditions are as follows:
** If the connection to the partner is unexpectedly gone, a status of
** GC_ASSOC_FAIL is returned.  If the connection has been terminated locally,
** by the GCdisconn function, a status of GC_ASSN_RLSED is returned.  The
** ability to detect these two modes of disconnection is system dependent.
** If the distinction cannot be made in a particular implementation, a status
** of GC_ASSOC_FAIL is the default.  GC_ASSOC_FAIL normally covers network
** errors from which recovery is impossible.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** Inputs:
**  flags.flow_indicator    Normal/Expedited flow indicator
**                               GC_NORMAL   - Use data channel
**                               GC_PRIORITY - Use interrupt channel
**                                             (Max 64 bytes)
**  svc_send_length         Amount of data to send
**  svc_buffer              Where to get the data to send
**  time_out                requested timeout in ms.
**  gc_cb                Pointer to CL connection block
**  (*gca_cl_completion)()  Routine to call on send completion
**
**
** Outputs:
**
**  status              Success or failure indication
**      OK                  Normal completion
**      NP_IO_FAIL          Communication pipe breakdown.
**      GC_ASSOC_FAIL       Association failure
**      GC_ASSN_RLSED       Association was previously released
**      GC_TIME_OUT         timer expired
**  sys_err             OS-specific status info, returned in
**
** Returns:
**     none
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      25-Jul-97 (fanra01)
**          Modified to make svc_parms setup common for both NT and 95. Split
**          send at the pipe level.
**      03-Dec-1999 (fanra01)
**          Add completion function as a parameter to the pipe writes.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Correct GCTRACE message slightly from "WriteFileEx" to "SendPipe"
**	    to better reflect logic and distinguish from other identical
**	    "WriteFileEx" GCTRACE's elsewhere.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If net driver selected, call it with GCDriverSend instead of
**	    named pipes logic.  Add more info on trace at entry.
**	    Use appropriate pre-allocated ASYNC_IOCB rather than allocating
**	    and freeing one for each send (for performance).
******************************************************************************/
VOID
GCsend(SVC_PARMS *svc_parms)
{
	ASSOC_IPC  *asipc;
	ULONG      status = OK;
	STATUS     rval = OK;
	HANDLE     fd;
	ASYNC_IOCB *iocb;

	GCTRACE(3)("GCsend: %p Entry point. %s timeout=%d %s send_length=%d\n",
		svc_parms,
                (svc_parms->flags.flow_indicator == GC_NORMAL) ? "nor" : "exp",
		svc_parms->time_out,
		svc_parms->flags.run_sync ? "sync" : "async",
		svc_parms->svc_send_length );

	svc_parms->status = OK;

	/*
	** Get local control block for this association
	*/

	asipc = (ASSOC_IPC *) svc_parms->gc_cb;

	if (!asipc)
	{
		GCerror(GC_ASSOC_FAIL, 0);
	}

	if (GCdriver_pce) /* Network Driver */
	{
	    GCDriverSend (svc_parms);
	    return;
	}

	/*
	** Do asynchronous pipes IO *
	*/

	iocb = &asipc->iocbs[svc_parms->flags.flow_indicator * 2 + GC_IOCB_SEND];
	ZeroMemory( iocb, sizeof(*iocb) );
	iocb->svc_parms = svc_parms;
	iocb->flags &= ~GC_IO_READFILE;

	/*
	** Determine the channel for message to go to
	*/
	if (svc_parms->flags.flow_indicator == GC_NORMAL)
	    fd    = asipc->SndStdChan;
	else
	    fd    = asipc->SndExpChan;

	/*
	**  Send using appropriate protocol.
	*/
        if (Is_w95) /* Windows 95 */
        {
            status = GCAnonPipeSend (fd, svc_parms, asipc, iocb,
                GCCompletionRoutine);
        }
        else
        {
            status = GCNamePipeSend (fd, svc_parms, asipc, iocb,
                GCCompletionRoutine);
        }

	if ((status == 0) || (status == ERROR_IO_PENDING))
            InterlockedIncrement( &asipc->asyncIoPending );

        GCTRACE(2)( "GCsend: status %d asyncIoPending=%d\n",
		    status,
		    asipc->asyncIoPending );
	svc_parms->status = rval;
}

/******************************************************************************
** Name: GCreceive - Receive data on an association
**
** Description:
**
** The user of this routine has requested svc_parms->reqd_amount bytes
** of data from the incoming message stream.
** The data read from the incoming messages is placed at the
** address starting at svc_parms->svc_buffer.
**
** The model of data transfer presented to the invoker is that of a byte stream.
** This routine must issue the sequence of system-specific read requests
** necessary to support this and buffer any excess incoming
** data not yet requested.  GCreceive must perform a sequence of recieve
** operations which results in the filling of the caller's buffer, i.e., the
** operation proceeds until sufficient data have been received to satisfy the
** caller's request.  Except under unrecoverable error conditions, the caller
** never receives less than was asked for.  The reconstruction of the final
** output buffer from the segmented transmission buffers is also
** done here.
**
** The GCreceive function must be capable of recoginzing a special "end of file"
** condition on the receive channel.  The implementation of this  condition is
** system-dependent.  It may utilize a system provided capability, or it may
** be implemented in some other way, such as a special data pattern or a
** header.  If EOF is received, no data are returned.
**
** svc_parms contains an indicator flags.new_chop.  This indicator is set TRUE
** if an end-of-file is received.  Otherwise, is set false.
**
** The flags.flow_indicator element of svc_parms specifies whether the receive
** function is to be issued on the normal or expedited flow channel.
**
** A synchronous request may specify a timeout limit.  This means that if
** has been received by the expiration, the returned status is set to
** GC_TIME_OUT and the request is terminated, with system-specific
** being done as required.
**
** The parameter svc_parms->(*gca_cl_completion)(svc_parms) specifies a
** completion exit to be invoked when the request is complete.  This exit is
** called for either synchronous or asynchronous requests.
**
** Possible error conditions are as follows:
** If the connection to the partner is unexpectedly gone, a status of
** GC_ASSOC_FAIL is returned.  If the connection has been terminated locally,
** by the GCdisconn function, a status of GC_ASSN_RLSED is returned.  The
** ability to detect these two modes of disconnection is system dependent.
** If the distinction cannot be made in a particular implementation, a status
** of GC_ASSOC_FAIL is the default.  GC_ASSOC_FAIL normally covers network
** errors from which recovery is impossible.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** Inputs:
**  flags.flow_indicator    GC_NORMAL - Use data channel
**                         !GC_NORMAL - Use interrupt channel (64 bytes, max)
**  reqd_amount             Amount of data to read
**  svc_buffer              Where to put the read data
**  gc_cb                CL connection block condition is legitimate.
**  (*gca_cl_completion)()  Routine to call on read completion
**  time_out                Specifies time to wait for completion  of a
**                          synchronous request.
**
** Outputs:
**
**  rcv_data_length         Length of data actually read
**  flags.new_chop          Indication of receipt of EOF:
**      0 => no EOF
**      1 => EOF received
**  status                  Success or failure indication.
**      OK                  - Normal completion
**      NP_IO_FAIL          - Communication pipe breakdown.
**      GC_ASSOC_FAIL       - Association failure
**      GC_ASSN_RLSED       - Association released by peer
**      GC_TIME_OUT         - A request timed out
**  sys_err                 OS-specific status info, returned in a
**                          CL_SYS_ERR structure.
**
**
** Returns:
**     VOID
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      13-jul-95 (emmag)
**         Implemented a timed read.
**	   Handle the GC_PURGE_MSG as if it were a zero byte read.
**	13-nov-95 (emmag)
**	    Need to revert to using four pipes instead of two.
**	15-jan-95 (tutto01)
**	    Need to set GC_NO_PEER for iinamu.
**	03-may-1996 (canor01)
**	    IIGCC does not use true asynchronous calls all the time,
**	    so before an asynchronous callback to disconnect has been
**	    made, a broken pipe on the expedited channel can be received.
**	    If a broken expedited pipe is detected, give up time slice
**	    and go into alertable state to allow callbacks to complete.
**	14-mar-1997 (canor01)
**	    Also, when processing a broken expedited pipe, do not call
**	    the GCA completion routine.
**      12-May-97 (fanra01)
**          Add the setup of rcv_data_length field on successful read with
**          timeout specified.  An incorrect length causes the higher levels
**          to go scanning through the buffers for more messages eventually
**          re-issuing a receive and resulting in a hang.
**      25-Jul-97 (fanra01)
**          Made svc_parms setup common for both NT and 95.  Split receive
**          at the pipe level.
**      03-Dec-1999 (fanra01)
**          Add completion function as a parameter to the pipe reads.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If net driver selected, call it with GCDriverReceive instead of
**	    embedded pipes logic.  Remove unused SECURITY_ATTRIBUTES and init.
**	    Trace more info at entry.
**
******************************************************************************/
VOID
GCreceive(SVC_PARMS *svc_parms)
{

        ASSOC_IPC  *asipc;
        HANDLE     fd;
        ASYNC_IOCB *iocb;
	BOOL       expedited;

        GCTRACE(3)("GCreceive: %p Entry point. %s timeout=%d %s  buffer size=%d\n",
		svc_parms,
                (svc_parms->flags.flow_indicator == GC_NORMAL) ? "nor" : "exp",
		svc_parms->time_out,
		svc_parms->flags.run_sync ? "sync" : "async",
		svc_parms->reqd_amount );
        svc_parms->status = OK;

        /*
        ** Get local control block for this association.
        */

        asipc = (ASSOC_IPC *) svc_parms->gc_cb;

        if (!asipc)
        {
                GCerror(GC_ASSOC_FAIL, 0);
        }

	if (GCdriver_pce) /* Network Driver */
	{
	    GCDriverReceive (svc_parms);
	    return;
	}

        if (svc_parms->flags.flow_indicator == GC_NORMAL)
	{
		fd = asipc->RcvStdChan;
		expedited = FALSE;
	}
        else
	{
		fd = asipc->RcvExpChan;
		expedited = TRUE;
	}

        svc_parms->flags.new_chop = FALSE;

        /*
        ** Select an OVERLAPPED structure for I/O.
        */

	iocb = &asipc->iocbs[svc_parms->flags.flow_indicator * 2 + GC_IOCB_RECV];
	ZeroMemory( iocb, sizeof(*iocb) );
        iocb->svc_parms = svc_parms;
        iocb->flags |= GC_IO_READFILE;

	/*
	**  Receive using appropriate pipes protocol.
	*/
        if (Is_w95) /* Windows 95 */
        {
            GCAnonPipeReceive (fd, svc_parms, asipc, iocb, expedited,
                GCCompletionRoutine);
        }
        else
        {
            GCNamePipeReceive (fd, svc_parms, asipc, iocb, expedited,
                GCCompletionRoutine);
        }
}

/******************************************************************************
** Name: GCreceiveCompletion - Data has been received on an association
**
** Description:
**
** Process the data just received for GCA CL responsibilities before  
** calling the GCA receive callback routine.
**   1. set chop_mark (if 0-len read)
**   2. check for purge message (strip it out, etc)
**   3. update misc counters
**
**
** Inputs:
**  svc_parms->
**    svc_buffer            Where the read data was put
**  dwNumberXfer            Length of data actually read
**
** Outputs:
**
**  rcv_data_length         Length of data actually read
**  flags.new_chop          Indication of receipt of EOF:
**      0 => no EOF
**      1 => EOF received
**  Gc_reads                Total # of reads done by the process
**  scb->cs_bio++           Total # of IOs done 
**
**
** Returns:
**     VOID
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      26-Dec-2001 (lunbr01) Bug 105902
**          Created.  Copied from GCCompletionRoutine() so that code can 
**          be called from elsewhere (e.g., for sync reads).
**      09-Apr-2002 (ashco01) Bug 107527.
**          W95/98 clients can occasionally recieve a GC purge message
**          appended to the previous message buffer even though the
**          messages have been sent seperately. This would cause the
**          purge message to be missed and a GC receive wait to occur
**          causing the FE application to hang. This fix is a w95/98
**          client-side only change to check for the purge message at
**          the end of the current buffer if not already found at the
**          beginning.
**	12-Dec-2002 (lunbr01) Bug 108292
**	    Memory leak of 4-12K per connect/disconnect in comm server on
**	    client side.
**	08-apr-2003 (somsa01)
**	    In GCreceiveCompletion(), ammended lunbr01's fix for Bug 108292
**	    to specifically target the comm server. This was causing loss of
**	    GCA_RELEASE messages in the ICESVR.
**      10-May-2004 (lunbr01) Bug 107527.
**          Addendum to previous fix. Also allow for case where GC purge
**          message is not at the beginning or the end of the buffer, 
**          but is "near" the end and is followed only by a GCA_IACK
**          message, which is typically sent right after the GC purge.
**          Only occurs on W9x systems where we can't seem to force
**          the GC purge message to always travel alone.  If there are
**          any other combinations, it may be necessary to rework the
**          fix to scan the entire buffer; however, that could 
**          negatively impact performance. 
**          Check purge semaphore on comm servers now on W9x.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Since the asyncIoPending counter is now incremented AFTER
**	    rather than BEFORE calling this function, adjust check on
**	    asyncIoPending by 1 (was 1, now 2) in logic that checks for
**	    expedited receive failure indicating a disconnect in progress
**	    (versus an error).
**
******************************************************************************/
VOID
GCreceiveCompletion(SVC_PARMS *svc_parms,
                    DWORD     *lpError,
                    DWORD     dwNumberXfer)
{
    DWORD      dwError = *lpError;
    STATUS     status;
    CS_SCB     *scb;
    HANDLE     hGCprgSemaphore;
    ASSOC_IPC  *asipc = (ASSOC_IPC *)  svc_parms->gc_cb;
    BOOL       prg_msg = FALSE;
    PTR        prg_pos = NULL;
    PTR        svc_bufend = NULL;

    /*
    ** We MUST set new_chop to TRUE on 0 byte reads!
    */
	
    GCTRACE(3)("GCreceiveCompletion: %p Read %s %d\n",
		svc_parms,
                (svc_parms->flags.flow_indicator == GC_NORMAL) ? "Norm" : "Exped",
                dwNumberXfer);
    svc_parms->flags.new_chop = FALSE;
    if (dwNumberXfer == 0)
    {
        if ((asipc->asyncIoPending <= 2) &&
            (svc_parms->flags.flow_indicator != GC_NORMAL) &&
            (dwError == ERROR_BROKEN_PIPE))
        {
            /*
            ** If at most one outstanding async I/O and this is on the
            ** expedited channel and there is a broken pipe indicated
            ** allow the message to complete with no error without
            ** setting new_chop as we're disconnecting.
            */
            /* dwError = (Is_w95 && is_comm_svr) ? dwError : 0;  */
	    dwError = (is_comm_svr) ? dwError : 0;
            GCTRACE(3)("GCreceiveCompletion: expedited completion during disconnect\n");
        }
        else
        {
	    svc_parms->flags.new_chop = TRUE;
        }
    }
    else if ((CV_C_CONST_MACRO ('I','I','P','R') ==
              *((int *)(svc_parms->svc_buffer))) &&
             (CV_C_CONST_MACRO ('G','M','S','G') ==
              *((int *)(svc_parms->svc_buffer)+1)) )
    {
        /* Purge message found at beginning of buffer */
        svc_bufend = svc_parms->svc_buffer;
        prg_msg = TRUE;
    }

    /* on w95 purge message can sometimes be received at end of
    last message or NEAR the end(before GCA_IACK) - bug #107527 */
    if ((Is_w95) 
     && (dwNumberXfer > sizeof(GC_PURGE_MESSAGE)) 
     && (prg_msg == FALSE))
    {
        /* locate current end of buffer */
        svc_bufend = svc_parms->svc_buffer + dwNumberXfer;
        /* determine correct end of buffer */
        prg_pos = svc_bufend - sizeof (GC_PURGE_MESSAGE);

        if ((CV_C_CONST_MACRO ('I','I','P','R') ==
            *((int *)(prg_pos))) &&
            (CV_C_CONST_MACRO ('G','M','S','G') ==
            *((int *)(prg_pos)+1)) )
        {
            /* set correct buffer endpoint */
            svc_bufend = prg_pos;
            prg_msg = TRUE;
        }
        else
        {
            prg_pos -= GCA_IACK_LEN;  /* back up for possible GCA_IACK */
            if ((prg_pos >= svc_parms->svc_buffer) /* still in buffer?*/
            &&  (CV_C_CONST_MACRO ('I','I','P','R') ==
                *((int *)(prg_pos))) &&
                (CV_C_CONST_MACRO ('G','M','S','G') ==
                *((int *)(prg_pos)+1)) )
            {
                /* set correct buffer endpoint */
                svc_bufend = prg_pos;
                prg_msg = TRUE;
            }
        }
    }

    if (prg_msg)
    {
        GCTRACE(2)("GCreceiveCompletion: Purge message received\n");
	if ((Is_w95) || (!is_comm_svr))
	{
	    if ((hGCprgSemaphore = asipc->hPurgeSemaphore) != NULL)
	    {
		for (;;)
		{
		    GCTRACE(2)("GCreceiveCompletion: WaitFor purge semaphore\n");
		    status = WaitForSingleObjectEx( hGCprgSemaphore,
						    INFINITE, TRUE );
		    if (status == WAIT_OBJECT_0)
		    {
			GCTRACE(2)("GCreceiveCompletion: Got the purge semaphore\n");
			break;
		    }
		}
	    }
	}
	svc_parms->flags.new_chop = TRUE;
        /* ensure remainder of buffer is read */
        dwNumberXfer -= (svc_bufend - svc_parms->svc_buffer)
                        + sizeof(GC_PURGE_MESSAGE);
        if (dwNumberXfer > 0)
        {
            MEcopy (svc_bufend + sizeof (GC_PURGE_MESSAGE),
                    dwNumberXfer,
                    svc_parms->svc_buffer);
        }
    }

    if (asipc->flags & GC_IPC_NETCOMPL)
    {
        dwError = ERROR_BROKEN_PIPE;
        GCTRACE(2)("GCreceiveCompletion: comsvr %s completion\n",
            (svc_parms->flags.flow_indicator == GC_NORMAL) ? "Norm" : "Exped");
    }
    svc_parms->rcv_data_length = dwNumberXfer;
    *lpError = dwError;

    Gc_reads++;
    CSGET_SCB(&scb);
    if (scb)
        scb->cs_bior++;
}

/******************************************************************************
** Name: GCrestore  - Restore data following process switch
**
** Description:
**  This routine recovers data previously saved by GCsave();  The invoker
**  passes a "save name" to identify the saved data.  This identifier was
**  used by GCsave to "tag" the saved data for subsequent recovery.
**  User data, if any, the ACB for an association and the related LCB are
**  recovered.  The LCB pointer in the ACB is set to the recovered LCB, and
**  any system-specific substructures related to the LCB are either
**  recovered or allocated as necessary.
**  Pointers to the user data and the ACB are returned to the invoker.
**
** Inputs:
**      save_name       A unique identifier for this save operation
**
** Outputs:
**      lng_user_data   Length of the user data block.
**      user_data       A poiner to a block of recovered user data.
**      acb             A pointer to a recovered ACB.
**      gc_cb        A pointer to the associated LCB.
**
**  Returns:
**      STATUS           success or failure indication
**      OK               Normal completion
**      GC_BAD_SAVE_NAME Save name is not recognized; no previous
**                       GCsave was done with that name.
**      GC_RESTORE_FAIL  A system-dependent failure occurred,
**                       preventing successful completion.
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**
** History:
**	13-nov-95 (emmag)
**	    Changed to use four pipes instead of two.
**      01-Oct-1999 (fanra01)
**          Add restore of IO completion event handle and the IO pending count.
**          Adjust the array index appropriately.
**      23-Oct-2001 (lunbr01) Bug 105793 & 106119
**          Restore handle for Purge Semaphore, which is now in asipc.
**      22-Jan-2002 (mofja02) Bug 106890 
**          Added logic to save and restore the Is_server global in GCSave 
**	    and GCrestore. Because Gateways spawn child processes globals 
**	    must besaved and restored and Is_server was not.  This caused  
**	    a hang onsome disconnects.
**	30-jul-2003 (somsa01)
**	    Properly address pointers out of ulptr.
**	07-nov-2003 (somsa01)
**	    Prior change is only for LP64.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Preallocate IOCBs for pipe sends and receives (for performance).
**	    If protocol driver being used, call it to allow it to restore
**	    and initialize protocol-specific info.  Also cleaned up the code
**	    to make it a little more generic/maintainable.
**
******************************************************************************/
VOID
GCrestore(SVC_PARMS *svc_parms)
{
	ASSOC_IPC *asipc;
	HANDLE    *ulptr = (HANDLE *) svc_parms->svc_buffer;
        i4         hdrsize = 0;
        i4         ipcsize = 0;
	char      *ipcptr  = NULL;    

	GCTRACE(3)("GCrestore: %p Entry point.\n", svc_parms);

	/*
	 * Allocate the new LCB.
	 */

	asipc = (ASSOC_IPC *)MEreqmem(0, sizeof(ASSOC_IPC), TRUE, NULL);
	svc_parms->gc_cb = (PTR) asipc;

	asipc->SndStdChan = ulptr[0];
	asipc->RcvStdChan = ulptr[1];
	asipc->SndExpChan = ulptr[2];
	asipc->RcvExpChan = ulptr[3];
	asipc->ClientReadHandle = ulptr[4];
        asipc->hDisconnThread = ulptr[5];
#ifdef LP64
        asipc->asyncIoPending  = PtrToInt(ulptr[6]);
#else
        asipc->asyncIoPending  = (u_i4)ulptr[6];
#endif
        asipc->hPurgeSemaphore = ulptr[7];
#ifdef LP64
	Is_server = PtrToInt(ulptr[8]);
#else
	Is_server = (i4)ulptr[8];
#endif

	hdrsize = 9 * sizeof(PTR);
	ipcptr  = svc_parms->svc_buffer + hdrsize;

	/*
	** If a protocol driver is being used, call its restore function
	** to allow it to restore protocol-specific information.
	*/
	if (GCdriver_pce)
	{
	    GCA_GCB	*gcb       = NULL;
	    /*
	    ** Allocate a connection block (GCA_GCB) for gcacl driver use.
	    ** Then, initialize it and link to it from the asipc.
	    */
	    gcb = (GCA_GCB *)MEreqmem( 0, sizeof(GCA_GCB), TRUE, NULL );
	    gcb->GCdriver_pce = (PTR)GCdriver_pce;
	    gcb->buffer_guard = GCB_BUFFER_GUARD;
	    asipc->gca_gcb = gcb;	/* link to it from the asipc */

	    /*
	    ** Restore generic driver info.
	    */
	    MEcopy( ipcptr, sizeof(gcb->id), &gcb->id );
	    ipcsize = sizeof(gcb->id);
	    ipcptr += sizeof(gcb->id);
	    if (((WS_DRIVER *)GCdriver_pce->pce_driver)->restore_func != NULL)
	    {
		STATUS status;
		status = (*((WS_DRIVER *)GCdriver_pce->pce_driver)->restore_func)( ipcptr, GCdriver_pce, &gcb->GCdriver_pcb );
		if( status != OK )
		{
	            GCTRACE(0)( "GCrestore: protocol driver restore failed with status %d\n", status );
	            svc_parms->status = GC_RESTORE_FAIL;
	            return;
		}
	    }
	}  /* endif protocol driver used */
	else
	if (Is_w95) /* Windows 95 */
	{
	    IPCrestore( asipc, (ULONG *)&ulptr[9] );
	}

	if (!GCdriver_pce)
	{
	    /*
	    ** Preallocate IOCBs for pipe send/recv's.
	    */
	    asipc->iocbs = (ASYNC_IOCB *)MEreqmem(0, sizeof(ASYNC_IOCB) * 4, TRUE, NULL);
	}

	svc_parms->status = OK;
}

/******************************************************************************
** Name: GCsave - Save data for process switch
**
** Description:
**  The caller passes a block of user data and an ACB to be saved, and a
**  "save name" identifying this group of saved data for subsequent
**  recovery.  The ACB, the LCB and the user data are saved in a system-
**  dependent way, and are tagged with the save name in a way that will
**  allow GCrestore to recover the data when passed the same save name.
**  GCsave must save any system-dependent substructures associated with
**  the LCB, if required.
**
** Inputs:
**      user_data       A poiner to a block of user data to be saved.
**      lng_user_data   Length of the user data block.
**      acb             A pointer to an ACB to be saved.  The ACB also
**                      points to the associated LCB.
**      save_name       A unique identifier for this save operation
**
** Outputs:
**      none
**
**  Returns:
**      STATUS      	Success or failure indication
**      OK      	Normal completion
**      GC_SAVE_FAIL    A system-dependent failure occurred
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	11-nov-95 (emmag)
**	    Changed to use four pipes instead of two.
**      01-Oct-1999 (fanra01)
**          Add save of IO completion event handle and the IO pending count.
**          Adjust the array index appropriately.
**      23-Oct-2001 (lunbr01) Bug 105793 & 106119
**          Save handle for Purge Semaphore, which is now in asipc.
**      22-Jan-2002 (mofja02) Bug 106890 
**          Added logic to save and restore the Is_server global in GCSave 
**	    and GCrestore. Because Gateways spawn child processes globals 
**	    must besaved and restored and Is_server was not.  This caused  
**	    a hang onsome disconnects.
**	30-jul-2003 (somsa01)
**	    Properly address pointers out of ulptr.
**	07-nov-2003 (somsa01)
**	    Prior change is only for LP64.
**      01-Jul-2005 (lunbr01) Bug 109115
**          Flag connection that it is being GCsave'd.
**      26-Aug-2005 (lunbr01) Bug 115107 + 109115
**          Undo prior fix to flag connection...no longer needed. 
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If protocol driver being used, call it to allow it to save
**	    protocol-specific info.  Also cleaned up the code
**	    to make it a little more generic/maintainable.
**
**
******************************************************************************/
VOID
GCsave(SVC_PARMS * svc_parms)
{
	ASSOC_IPC *asipc = (ASSOC_IPC *) svc_parms->gc_cb;
	HANDLE    *ulptr = (HANDLE *)    svc_parms->svc_buffer;
        i4         hdrsize = 0;
        i4         ipcsize = 0;
	char      *ipcptr  = NULL;    

	GCTRACE(3)("GCsave: %p Entry point.\n", svc_parms);

	/*
	** Store the pipe handles in the svc_buffer.
	*/

	ulptr[0] = asipc->SndStdChan;
	ulptr[1] = asipc->RcvStdChan;
	ulptr[2] = asipc->SndExpChan;
	ulptr[3] = asipc->RcvExpChan;
	ulptr[4] = asipc->ClientReadHandle;
        ulptr[5] = asipc->hDisconnThread;
#ifdef LP64
        ulptr[6] = IntToPtr(asipc->asyncIoPending);
#else
        ulptr[6] = (HANDLE)asipc->asyncIoPending;
#endif
        ulptr[7] = asipc->hPurgeSemaphore;
#ifdef LP64
	ulptr[8] = IntToPtr(Is_server);
#else
	ulptr[8] = (HANDLE)Is_server;
#endif

	hdrsize = 9 * sizeof(PTR);
	ipcptr  = svc_parms->svc_buffer + hdrsize;

	/*
	** If a protocol driver is being used, call its save function
	** to allow it to save protocol-specific information.
	*/
	if (GCdriver_pce)
	{
	    GCA_GCB	*gcb       = asipc->gca_gcb;

	    /* Note idleness of send/receive state machines. */
	    if( gcb->send.state || gcb->recv.state )
	    {
	        GCTRACE(1)("GCsave: line not idle (%d %d)\n",
	                gcb->send.state, gcb->recv.state );
	    }

	    /*
	    ** Save generic driver info.
	    */
	    MEcopy( &gcb->id, sizeof(gcb->id), ipcptr);
	    ipcsize = sizeof(gcb->id);
	    ipcptr += sizeof(gcb->id);

	    /*
	    ** Allow protocol driver to save state.
	    */
	    if (((WS_DRIVER *)GCdriver_pce->pce_driver)->save_func != NULL)
	    {
		STATUS status;
		i4  driversize = svc_parms->reqd_amount - (hdrsize + ipcsize);
		status = (*((WS_DRIVER *)GCdriver_pce->pce_driver)->save_func)( ipcptr, &driversize, gcb->GCdriver_pcb );
		if( status != OK )
		{
	            GCTRACE(0)( "GCsave: protocol driver save failed with status %d\n", status );
	            svc_parms->status = GC_SAVE_FAIL;
	            return;
		}
		ipcsize += driversize;
		ipcptr  += driversize;
	    }
	}  /* endif protocol driver used */
	else
	if (Is_w95) /* Windows 95 */
	{
            ipcsize = IPCsave( asipc, (ULONG *)ipcptr );
	}

	svc_parms->rcv_data_length = hdrsize + ipcsize;
	svc_parms->status = OK;
}

/******************************************************************************
** Name: GCpurge    - Purge normal flow send channel.
**
** Description:
**
**  If possible in the specific environment, any outstanding normal flow
**  send or receive requests are cancelled, and an EOF message is sent
**  on the normal channel.
**  The form of an EOF is system dependent.  It may be a unique data
**  pattern, signal or other special system-provided message which is
**  recognizable by the receiver in GCreceive in a context-free way.
**
** Inputs:
**      gc_cb    Local control block
**
** Outputs:
**      status      OK operation succeeded
**
**  Returns:
**      none
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**     13-jul-95 (emmag)
**         Zero byte messages were sometimes being lost on the pipe.
**	   Replaced this with a fixed length message "CHOPMARK"
**	   Flush the channel after writing the chopmark so that we
**	   will hang until the client reads all buffered data.
**	   Set the purge-event when purging completes.
**     24-aug-95 (emmag)
**         Since we created the pipe with FILE_FLAG_OVERLAPPED set
**         then we must use an Overlapped structure when writing
**         to the pipe.
**	21-nov-1996 (canor01)
**	    Flush the buffers on the pipe before sending the purge message
**	    to ensure it doesn't get lost on the end of a previous message.
**	30-Jan-97 (kitch01)
**		Bug 79293 purge problem on fast pentium machines.
**		Changed hPurgeEvent to hPurgeSemaphore to synchronize the purge
**		messages.
**      14-Oct-97 (fanra01)
**              Modified the conditions for when a FlushFileBuffer is called.
**              Should isolate the GC level purge message.
**	08-Feb-1999 (somsa01)
**	    If is_comm_svr was set, FlushFileBuffers() was executed to
**	    clear out the client read pipe. in other cases, we just did a
**	    wait. However, according to the Micorosft Visual C++
**	    documentation, FlushFileBuffers() will wait until any remaining
**	    data has been read by the client before flushing the pipe.
**	    Therefore, we can use this in ALL situations.
**	22-mar-1999 (somsa01)
**	    Replaced the second remaining PCsleep() with FlushFileBuffers().
**	    It is executed only if there is data left on the pipe after
**	    sending a PURGE message.
**	28-feb-2000 (somsa01 for lunbr01) Bug 95163 for W95
**	    Prior fix for this bug was made in NT-specific code. Implemented
**	    same change in W95. That is, remove PCsleeps in the GCpurge code,
**	    thus greatly improving performance in this area. Shared code as
**	    much as possible with NT.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Don't reissue redundant GetLastError() for input to SETWIN32ERR
**	    after WriteFile() error.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    If net driver selected, call it (GCDriverPurge) instead of
**	    embedded pipes logic.
**
******************************************************************************/
VOID
GCpurge(SVC_PARMS *svc_parms)
{
	ULONG     amount_written = 0;
	ULONG     amount_read = 0;
	ASSOC_IPC *asipc = (ASSOC_IPC *) svc_parms->gc_cb;
        STATUS     err;
        STATUS     status     = OK;
	CS_SCB     *scb = NULL;
        OVERLAPPED Overlapped;
	OVERLAPPED *pOverlapped = NULL;
	SECURITY_ATTRIBUTES sa;
	HANDLE	hGCprgSemaphore;
	HANDLE  hPeekPipe;

	GCTRACE(3)("GCpurge: %p Entry Point\n", svc_parms);

        svc_parms->status = OK;

	if (GCdriver_pce) /* Network Driver */
	{
	    GCDriverPurge (svc_parms);
	    return;
	}

	if (!Is_w95)
	{
	    iimksec (&sa);
	    CSGET_SCB(&scb);

	    /*
	    ** Since we're writing to a named pipe the Offset and OffsetHigh
	    ** members of the Overlapped structure must be NULL.
	    **
	    ** Create an event which we will wait for in case of an
	    ** ERROR_IO_PENDING on the write.
	    */

	    pOverlapped = &Overlapped;
	    Overlapped.Offset = (DWORD) NULL;
	    Overlapped.OffsetHigh = (DWORD) NULL;
	    GCRESRC(Overlapped.hEvent,
		    Overlapped.hEvent = CreateEvent(&sa, TRUE, FALSE, NULL));
	    if (Overlapped.hEvent == NULL)
	    {
		err = GetLastError();
	    }
	} /* End of NT-specific code */

	/*
	** Flush any current messages in pipe. If there is any
	** remaining data in the pipe that hasn't been read,
	** FlushFileBuffers() will wait until it is read before
	** flushing the pipe.
	*/
	FlushFileBuffers(asipc->SndStdChan);

        if (!WriteFile(asipc->SndStdChan,
                       GC_PURGE_MESSAGE,
                       sizeof (GC_PURGE_MESSAGE),
                       &amount_written,
                       pOverlapped))
        {
	    switch(status = GetLastError())
	    {
		case ERROR_IO_PENDING:
		    GCTRACE(1)( "GCPurge: WriteFile() delayed with ERROR_IO_PENDING\n");
		    WaitForSingleObject(Overlapped.hEvent, INFINITE);
		    GetOverlappedResult(asipc->SndStdChan, &Overlapped,
					&amount_written, TRUE);
                    ResetEvent( Overlapped.hEvent );
		    break;

		default:
		    SETWIN32ERR(svc_parms->sys_err, status, ER_write);
		    svc_parms->status = GC_ASSOC_FAIL;
		    GCTRACE(1)( "GCPurge: WriteFile() failed with status=%d\n",
				status );
		    goto GCpurge_end;
	    }
        }

	Gc_writes++;
	if (scb)
	    scb->cs_biow++;

	if (Is_w95)
	{
	    hPeekPipe = asipc->ClientReadHandle;
	}
	else
	{
	    hPeekPipe = asipc->SndStdChan;
	    GCRESRC( Overlapped.hEvent, CloseHandle(Overlapped.hEvent) );
	}

	/*
	** Now make sure the purge message is received by issuing a
	** FlushFileBuffers() only if there is data left on the pipe
	** (which means the client has not read the data yet).
	*/
	amount_read = 0;
	if (!PeekNamedPipe (hPeekPipe, NULL, 0, NULL, &amount_read, NULL))
	{
	    status = GetLastError();
	    GCTRACE(1)( "GCpurge: PeekNamedPipe() failed with status=%d\n",
			status );
	}
	if (amount_read)
	{
	    GCTRACE(2)("GCPurge: FlushFileBuffer on purge message\n", status);
	    FlushFileBuffers(asipc->SndStdChan);
	}

GCpurge_end:
	if ((Is_w95) || (!is_comm_svr))
	{
	    if ((hGCprgSemaphore = asipc->hPurgeSemaphore) != NULL)
	    {
		GCTRACE(2)("GCPurge: Release purge semaphore\n", status);
		ReleaseSemaphore( hGCprgSemaphore, 1, NULL);
	    }
	}

	GCTRACE(3)("GCpurge: Complete\n");
	return;
}

/******************************************************************************
** Name: GCterminate    - Perform local system-dependent termination functions
**
** Description:
**
**  GCterminate is invoked by mainline code as the last operation done as
**  a GCA user is terminating its connection with GCA.  It may be used to
**  perform any final system-dependent functions which may be required,
**  such as cancelling system functions which are still in process, etc.
**
** Inputs:
**  svc_parms       pointer to svc_parms parameter list
**
** Outputs:
**      none
**
**  Returns:
**      none
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Add call to generate statistics report if tracing is on.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add/comment call to shutdown/terminate protocol driver.
**
******************************************************************************/
VOID
GCterminate(SVC_PARMS * svc_parms)
{
	GCTRACE(3)("GCterminate: %p Entry point. PID=%d, use_count=%d\n",
		    svc_parms, Conn_id, GCinitialized_count);
	svc_parms->status = OK;

	GCinitialized_count--;
	if ( GCinitialized_count > 0 )
	    return;

	/*
	**  If network protocol driver selected, call its exit function
	**  if there is one.
	*/
	    if (( GCdriver_pce ) &&
	        ( ((WS_DRIVER *)GCdriver_pce->pce_driver)->exit_func != NULL ))
		svc_parms->status = (*((WS_DRIVER *)GCdriver_pce->pce_driver)->exit_func)(GCdriver_pce);

	if (GC_trace)
	    GCrpt_stats();
}


/******************************************************************************
*******************************************************************************
**
**  "Common" routines
**
*******************************************************************************
******************************************************************************/


/******************************************************************************
** Name:  GCdetect      ... Detect whether a listen id is alive.
**
** Description:
**  This routine is called by the Name Server to detect whether a listen
**  address is still alive.
**
**  Check for an active server by testing for an available named pipe.
**  If a pipe is not available, the server is not there.
**
** Inputs:
**  *listen_id  - the listen address.
**
** Outputs:
**
** CL specs says GCdetect has to return TRUE or FASE:
** Returns:
**  TRUE        - if the listen address is alive
**  FALSE       - if the listen address doesn't exist.
**
** But the name server usage requires a STATUS returned.
** Returns:
**    STATUS:
**      OK              The listen address is alive.
**
** History:
**	13-nov-95 (emmag)
**	    Modified to use 6.4 pipe naming conventions.
**
******************************************************************************/
STATUS
GCdetect(char *listen_id)
{
	char   name[MAXPNAME];
	STATUS status;

	GCTRACE (3)("GCdetect: Entry point\n");

        if (Is_w95) /* Windows 95 */
        {
            STprintf(name, "-%s",  listen_id);
            status = DetectServer(name);
            return ((status == TRUE) ? OK : FAIL);
        }

        STprintf(name, "%s\\C2S_STD\\%s", PIPE_PREFIX, listen_id);
	status = WaitNamedPipe(name, NMPWAIT_USE_DEFAULT_WAIT);
	return (status? OK : FAIL);
}


/******************************************************************************
**
** Name: GCusername
**
** Description:
**	Set the passed in argument 'name' to contain the name of the user 
**	who is executing this process.  Name is a pointer to an array 
**	large enough to hold the user name.  The user name is returned
**	NULL-terminated in lowercase with any trailing whitespace deleted.
**
**	On systems where relevant, this should return the effective rather 
**	than the real user ID. 
**
**	It does the samething as what IDname does.
**
** Input:
**	size		Length of buffer for user name.
**
** Output:
**	name		The effective user name.
**
** Returns:
**	VOID
**
** History:
**	18-Mar-98 (rajus01)
**	    Created.
**	07-nov-2000 (somsa01)
**	    For Embedded Ingres, the user is always ingres.
**      23-feb-2001 (rigka01) 
**	    Cross integrate 2.5 change 448098 to 2.0: 
**          In GCusername(), for Embedded Ingres, the user is always ingres.
**      01-May-2007 (Fei Ge) Bug 116825
**          szUserName shouldn't be static.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers by increasing username buffer
**	    to max size supported by Windows.  
**	    Also improve performance (affects connect requests primarily) by
**	    only checking 1st time in for TNG/embedded Ingres installation.
**
*******************************************************************************/
VOID
GCusername( char *name, i4  size )
{
    char  szUserName[GC_USERNAME_MAX + 1] = "";
    DWORD dwUserNameLen = sizeof(szUserName);
    static i1   TNGInstallation = -1;
    GCTRACE(5)("GCusername: Entered with size %d, TNGInstallation flag=%d\n",
		size, TNGInstallation);

# ifdef EVALUATION_RELEASE
    STlcopy("ingres", name, size - 1);
# else

    /*
    ** On 1st time in only, check  if this is an embedded Ingres installation.
    */
    if (TNGInstallation == -1)  /* If not already set, set it. */
    {
	char        *host;
	char        *value = NULL;
	char        config_string[256];
	PMinit();
	PMload( NULL, (PM_ERR_FUNC *)NULL );
	host = PMhost();
  
	STprintf( config_string, ERx("%s.%s.setup.embed_user"),
		  SystemCfgPrefix, host);
	PMget( config_string, &value );
   
	if (value == NULL || (STbcompare(value, 0, "OFF", 0, TRUE) == 0))
	    TNGInstallation = FALSE;
	else
	    TNGInstallation = TRUE;
    }  /* endif TNGInstallation not set */

    if (TNGInstallation == TRUE)
    {
	STlcopy("ingres", name, size - 1);
	return;
    }

    /*
    ** Normal, non-TNG/embedded, non-Eval Ingres installation...get the
    ** effective userid from the operating system.
    ** NOTE: GetUserName() returns nothing if the target field is too
    **       small to hold the result.  GCusername(), on the other hand,
    **       returns a truncated length if the target field is too small.
    **       This is the reason an intermediate (large) buffer field is
    **       used rather than GetUserName'ing directly into the target.
    **       
    */
    if (GetUserName(szUserName, &dwUserNameLen))
    {
	_strlwr(szUserName);
	STlcopy(szUserName, name, size - 1);
    }

# endif /* EVALUATION_RELEASE */
}

/******************************************************************************
**  Name: GCusrpwd -
**
**  Description:
**  This routine takes two input strings, a username and a password,
**  and checks whether to allow a logon or not.  The strings must be
**  null terminated, and max lengths are 12 for the username and 80
**  for the password.  If a string is too long, it will be truncated.
**
**  A return of OK means the username and password are legal.
**  A return of GC_LOGIN_FAIL means either the username and password are not
**      a valid combination, or that there was an error obtaining
**      the validation info.
**
**  Inputs:
**      username - pointer to a null terminated string
**      password - pointer to a null terminated string
**      sys_err  - for CL failures
**
**  returns:
**      function value returns OK or GC_LOGIN_FAIL.  CL failures are not
**      reported, as such.
**
**  side effects:
**      none
**
**  History:
**	01-03-96 (wooke01)
** 	    For DBCS port security workaround so works on 3.5 and 3.51    
**          NETRESOURCE is a struct that  specifies net connection info
**	    work around security problem by using a share name and validate
**	    the share name for security checks. We have do do this for now in 
**	    DBCS as we haven't got a copy of 3.5.1 Japanese to test DBCS 
**	    functionality Makes sense to support 3.5 and 3.51 for Japanese 
**	    anyway.  However must be aware that the IME is rumoured to 
**	    be buggy in Japanese 3.51 
**      17-Mar-97 (fanra01)
**          Modified LogonUser call to use NULL for domain name.  This causes
**          the local account database to be searched and then the trusted
**          domain account databases until the name is found.
**      02-apr-97 (mcgem01)
**          Now that double-byte versions of 3.51 and 4.0 are available,
**          we can use the LogonUser API call for username/password
**          validation.
**      12-may-1998 (canor01)
**          If username/password is invalid, check for the hard-coded Jasmine
**          username/password from jasmonitor and return error code specific
**          to this situation.
**      13-Feb-2002 (fanra01)
**          Update routine to always return authenticated for evaluation
**          releases (Primarily for Ingres/ICE).
**
******************************************************************************/
STATUS
GCusrpwd(char *username, char *password, CL_ERR_DESC * sys_err)
{
    HANDLE    phToken;
    STATUS    status = OK;

    GCTRACE(3)("GCusrpwd: Entry point.\n");
    CL_CLEAR_ERR(sys_err);

# ifndef EVALUATION_RELEASE
    if (username == NULL || password == NULL)
        return(GC_LOGIN_FAIL);

    if (!Is_w95)
    {
        if (!LogonUser(username, NULL, password, LOGON32_LOGON_NETWORK,
                           LOGON32_PROVIDER_DEFAULT, &phToken))
        {
       	    status = GetLastError();
   	    return (GC_LOGIN_FAIL);
        }
    }

    CloseHandle(phToken);
# endif /* EVALUATION_RELEASE */

    return (OK);
}

/******************************************************************************
** Name: GCsync - wait for sync request to complete
**
** Description:
**  GCsync is called repeatedly while GCA mainline waits for
**  the last posted GCA CL request to complete.  If the
**  implementation requires, GCsync may poll for lower level
**  I/O operations; otherwise, GCsync should block and return
**  when the CL request completes.
**
**  GCsync may return before the last posted request completes,
**  and will be reinvoked by GCA.  For example, it may return
**  after each lower level I/O operation.
**
**  GCsync must also allow keyboard generated interrupts to
**  occur, but not while completions are being driven.
**
**  Since this call is only made by front ends that are doing one thing
**  at a time anyway, it suffices to have a very simple semaphore
**  scheme.
**
** History:
**      29-Feb-96 (fanra01)
**          Increased size of waitarray for extra event handle.
**          Moved initialisation of waitarray to before Is_w95 condition to
**          allows array use in GCF_EMBEDDED_GCC section for multiple wait.
**          Added gcc event for both w95 and NT.
**	12-dec-96 (mcgem01)
**	    GCsync now takes a timeout value.
**      25-Jul-97 (fanra01)
**          Remove win 95 specific code.  No uses the same sync as NT.
**	31-mar-1999 (somsa01)
**	    Properly set up the number of events for Windows 95, as the
**	    TIMEOUT event is not valid on that platform.
**	12-Mar-2004 (inifa01) INGREP154 b111841
**	    repserver hangs using 99% CPU if the event timeout flag is
**	    set (ie -EVTn). 
**	    FIX: If timeout event previously occured and has been canceled
**	    don't load waitarray for current request with the invalid 
**	    canceled handle.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Remove ifdefs for embedded comm server i(GCF_EMBEDDED_GCC)
**	    around GCC_COMPLETE event.  Now always include it in the wait
**	    list since it will be used if a network protocol driver
**	    (like Winsock tcp_ip) is selected with II_GC_PROT.  If not,
**	    it's just a wasted entry in the list but should be OK; it is
**	    always created by this program anyway during initialization.
**
******************************************************************************/

VOID
GCsync(i4 timeout)
{
    STATUS  status;
    ASYNC_IOCB *iocb = &ListenIOCB;
    SVC_PARMS *svc_parms = iocb->svc_parms;
    HANDLE  waitarray[6];
    DWORD   numElements = 3;
    CS_SCB *scb = NULL;
    ASYNC_TIMEOUT *asto = NULL;
    i4  gcc_compl_event = 0;
    i4  to_compl_event = 0;
    i4 index;

    CSGET_SCB(&scb);

    GCTRACE(3)("GCsync: Entry point.\n");

    waitarray[0] = hAsyncEvents[LISTEN];
    waitarray[1] = hSignalShutdown;
    waitarray[2] = SyncEvent;
    index = 2;
    if ( !Is_w95 )
    {
	waitarray[++index] = hAsyncEvents[TIMEOUT];
	numElements++;
    }
    waitarray[++index] = hAsyncEvents[GCC_COMPLETE];
    gcc_compl_event = index;
    numElements++;
    if ( gc_compl_key )
    {
        ME_tls_get( gc_compl_key, (PTR *)&asto, &status );
        if ( asto )
        {
	    /* 
	    ** Prevent infinite loop due to INVALID_HANDLE error
	    ** if gc_compl_key is non zero but event which created
	    ** it has been canceled on timeout recieve. (b111841)
	    */ 
	    if (asto->fd != INVALID_HANDLE_VALUE)
	    {
                waitarray[++index] = asto->event;
                to_compl_event = index;
                numElements++;
	    }
        }
    }

    while (TRUE)
    {
        status=WaitForMultipleObjectsEx(numElements, waitarray, FALSE,
					timeout, TRUE);
        switch(status)
        {
            case WAIT_IO_COMPLETION:
                GCTRACE (3) ("GCsync: WAIT_IO_COMPLETION\n");
                return;

            case WAIT_TIMEOUT:
                GCTRACE (3) ("GCsync: WAIT_TIMEOUT\n");
                return;

            case WAIT_OBJECT_0:
                GCTRACE (3) ("GCsync: WAIT_OBJECT_0\n");
                (*GCevent_func[LISTEN])();
                break;

            case WAIT_OBJECT_0 + 1:
                GCTRACE (3) ("GCsync: WAIT_OBJECT_1\n");
                svc_parms->sys_err->errnum = 0;
                svc_parms->status = (OK);
                (*svc_parms->gca_cl_completion) (svc_parms->closure);
                return;

            case WAIT_OBJECT_0 + 2:
                GCTRACE (3) ("GCsync: WAIT_OBJECT_2\n");
                return;

            case WAIT_OBJECT_0 + 3:
		if (!Is_w95)
		{
		    /* Listen timeout */
		    GCTRACE (3) ("GCsync: WAIT_OBJECT_3\n"); 
		    (*GCevent_func[TIMEOUT])();
		}
		else
		{
		    GCTRACE (3) ("GCsync: WAIT_OBJECT_%d\n", status);
		    if ( status == (STATUS)WAIT_OBJECT_0 + gcc_compl_event )
			(*GCevent_func[GCC_COMPLETE])();
		    else
		    /* async receive timeout */
		    if ( status == (STATUS)WAIT_OBJECT_0 + to_compl_event )
		    {
			if (asto &&
			    asto->fd != 0 && 
			    asto->fd != INVALID_HANDLE_VALUE)
			{
			    if ( pfnCancelIo )
				(*pfnCancelIo)( asto->fd );
			}
		    }
		}
                return;

            case WAIT_OBJECT_0 + 4:
            case WAIT_OBJECT_0 + 5:
                GCTRACE (3) ("GCsync: WAIT_OBJECT_%d\n", status);
                if ( status == (STATUS)WAIT_OBJECT_0 + gcc_compl_event )
                    (*GCevent_func[GCC_COMPLETE])();
                else
                /* async receive timeout */
                if ( (gc_compl_key) &&
		     (status == (STATUS)WAIT_OBJECT_0 + to_compl_event) )
                {
                    if ( asto && 
                         asto->fd != 0 && 
                         asto->fd != INVALID_HANDLE_VALUE )
                    {
			if ( pfnCancelIo )
                            (*pfnCancelIo)( asto->fd );
                    }
                }
                return;

            default:
                status = GetLastError();
                break;
        }
    }
}


/******************************************************************************
** Name: GCnsid - Establish or obtain the identification of the local
**                Name Server
**
** Description:
**
**  GCnsid is invoked to establish and to obtain the identity of the Name
**  Server in a system-dependent way.  This function provides the
**  "bootstrapping" capability required to allow the GCA Connection
**  Manager to establish an association with the Name Server.
**
**  Two operations may be invoked: PUT and GET.  The former is invoked
**  by Name Server to establish its global identity in a system specific
**  way.  This identity is the "listen_id" which it establishes as a
**  normal server to allow clients to request associations.  The latter is
**  invoked by Connection Manager in order to obtain the listen_id which it
**  must use to create an association with Name Server.
**
**  The exact syntax of the identifier may be system dependent, although
**  this is not of concern to GCnsid.  The identifiier consists of a
**  nul-terminated character string.  It is established in a
**  system-dependent way so as to be globally accessible by all GCA
**  Connection Managers in the local node.
**
**
** Inputs:
**  flag            A constant value specifying the operation to be
**                  performed.  The following values are defined:
**                    GCN_SET_LISTENID   Establish a globally accessible
**                                       identifier for the Name Server.
**                    GCN_FIND_LISTENID  Retrieve the identifier of the
**                                       Name Server in the local node.
**
**  gca_listen_id   A pointer to an area containing
**                  the character string specifying
**                  the identity of the Name Server,
**                  if the operation is PUT, or an
**                  area in which to place the character
**                  string identifier if the operation
**                  is GET.
**
**  length          The length of listen_id to be set.
**
** Outputs (GCA_GET_NSID operation only):
**
**  *gca_listen_id          The Name Server identifier.
**
** Returns:
**  STATUS:
**  E_GC0025_NM_SRVR_ID_ERR Unable to obtain NS id.
**
** Exceptions:
**  none
**
** Side Effects:
**  none
** History:
**    23-Sep-98 ( rajus01)
**	 Made it generic. Yanked from Unix code line( gcagcn.c).
**
******************************************************************************/
STATUS
GCnsid(flag, gca_listen_id, length, sys_err)
	i4              flag;
	char           *gca_listen_id;
	i4              length;
	CL_ERR_DESC    *sys_err;
{

	STATUS		CLerror;
	char		mysym[ 200 ];
	char		*inst;
	char		*port;

	NMgtAt( "II_INSTALLATION", &inst );

	STprintf( mysym, "II_GCN%s_PORT", inst ? inst : "" );

	switch( flag )
	{
	case GC_SET_NSID:
		if( ( CLerror = NMstIngAt( mysym, gca_listen_id ) ) != OK )
			return CLerror;
		break;

	case GC_CLEAR_NSID:
		if( ( CLerror = NMstIngAt( mysym, "" ) ) != OK )
			return CLerror;
		break;

	case GC_RESV_NSID:
		break;

	case GC_FIND_NSID:
		NMgtAt( mysym, &port );
		if( !port || !*port )
			return GC_NM_SRVR_ID_ERR;
		STcopy( port, gca_listen_id );
		break;
	default:
		return GC_NM_SRVR_ID_ERR;
	}

	return OK;
} /* end GCnsid */


/******************************************************************************
** Name:  GChostname    ... Request the local host name.
**
** Description:
**  Attempts to find the local host name as appropriate for a given
**  implementation.  The definition of  the local host name is the OS
**  supported name that is used in delineating unique members of an
**  OS level Networked File System or OS Networked Interprocess
**  Communication System. The mechanism for obtaining the local hostname is
**  the one which is the OS vendor supported mechanism.
**  Note that this mechanism may be independent of any user level
**  Network Protocol (eg. VAXCLUSTER does not require DECnet/LAVC).
**
**  If the local host name cannot be found a NULL string is  returned
**  and the return code set to FAIL. This implies that the local
**  node does not participate in any of the above.
**  If the host name string size is too large, a truncated string
**  is returned and the return code is set to FAIL.
**
** Inputs:
**  hostlen     - maximum length of ourhost string
**
** Outputs:
**  *ourhost    - the local hostname buffer string
**
**  Returns:
**      none
**
** Examples:
**    GChostname(ourhost,sozeof(ourhost));
**
** Side Effects:
**      none
**
** History:
**	29-may-1997 (canor01)
**	    Default to using II_HOSTNAME, if it exists, then try for
**	    GetComputerName().
**	08-mar-2007 (smeke01) BUG 115434
**	    The size passed in is equivalent to sizeof(buf), and
**	    includes the space required for EOS marker. Since sizeof() is
**	    a 1-based count of elements we need to decrement by 1 when
**	    passing to any functions that expect a 0-based count of
**	    elements.
**
******************************************************************************/
VOID
GChostname(char *buf,
           i4   size)
{
    char *ptr;
    i4   len = size - 1;

    GCTRACE(3)("GChostname: Entry point.\n");

    NMgtAt( "II_HOSTNAME", &ptr );
    if ( ptr && *ptr )
    {
	STlcopy( ptr, buf, size - 1 );
    }
    else if (GetComputerName(buf, &len) == TRUE)
    {
	CVupper(buf);
    }
    else
    {
	*buf = EOS;
    }
}


/******************************************************************************
**
** Name:	GCCompletionRoutine
**
** Description: Completion routine for Async reads and writes.
**
** History:
**
**     13-jul-95 (emmag)
**         Since zero length messages were sometimes being lost on the pipe -
**	   replace them with a fixed lenght message.  Need to handle receipt
**	   of this message as though it were a zero byte read.
**     25-jul-95 (emmag)
**	   The chopmark coming from UNIX/VMS etc. will be a zero byte read
**	   Overlooked this case in my previous change.
**     29-dec-1995 (canor01)
**	   ERROR_PIPE_NOT_CONNECTED is possible value for dwError, though missing
**	   in documentation.
**     01-apr-1996 (canor01)
**	   An invalid connection can be manifested through an assoc_id
**	   of -1 or by a svc_parms->acb that is null.
**     23-apr-1996 (canor01)
**	   ERROR_BROKEN_PIPE is also possible value for dwError, though missing
**	   in documentation.
**	05-mar-1997 (canor01)
**	    Percolating pipe failures up to GCA seems to be a mistake.
**	    Do not execute GCA completion routine on ERROR_BROKEN_PIPE.
**	    This can cause AL FSM errors in GCC.
**      21-apr-1997 (canor01)
**          If the acb has been freed and the memory released, it will
**          not be NULL, but an invalid pointer.  Use IsBadReadPtr()
**          to verify it.
**      02-jun-1997 (canor01)
**	    Percolating pipe failures up to GCA is definitely the right
**	    thing to do.  Otherwise, GCC will not know to free memory
**	    reserved for the IO.  AL FSM errors avoided by synchronizing
**	    on GCdisconn.
**      04-Sep-97 (fanra01)
**          Replace the error indication to GCA completion removed in
**          integration.
**      14-Oct-97 (fanra01)
**          Modified the conditions for when a FlushFileBuffer is called.
**          Modified the check for a purge message.  Now check every message
**          using an integer eye-catcher compare.
**          If a message is concactinated strip of the internal message and
**          pass up the following message.
**	15-mar-1999 (somsa01)
**	    Partial x-integration of fix for Jasmine bug 95556; do not test
**	    for the purge message if dwNumberXfer is zero.
**      01-Oct-1999 (fanra01)
**          Add decrement of asyncIoPending counter.
**          If the asyncIoPending counter is 0 and GCdisconn flags as being
**          called the asyncIoComplete event is signaled to allow GCdisconn to
**          continue.
**      09-Dec-1999 (fanra01)
**          Use local variables for disconnect fields from asipc.
**	28-feb-2000 (somsa01 for lunbr01) Bug 99826 for W95
**	    Occasional hang on W95. GC_PURGE was being received (std chan)
**	    before GCA_ATTENTION (exp chan) in client-side GCC. Removed check
**	    that was preventing GC_PURGE semaphore logic from being executed
**	    in server processes. This hang occurred with and without fix for
**	    bug 95163; fix was made to common NT and W95 code, although bug
**	    only reported on W95.
**      27-sep-2001 (lunbr01) Bug 105902
**          Move logic to process a received message (such as checking for 
**          a purge message) to a separate function, GCreceiveCompletion().
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Change GCasyncDisconn to GCasyncCompletion since it was renamed.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Remove MEfree(iocb) since the send/recv iocbs are now
**	    preallocated and not freed until disconnect.
**	    Defer decrementing asyncIoPending until after returning
**	    from the GCA completion routine to prevent possible nested
**	    call to GCdisconn and GCrelease thinking it OK to release
**	    resources.
******************************************************************************/

VOID WINAPI
GCCompletionRoutine(DWORD        dwError,
                    DWORD        dwNumberXfer,
                    LPOVERLAPPED pOverlapped)
{
    ASYNC_IOCB *iocb      = (ASYNC_IOCB *) pOverlapped;
    SVC_PARMS  *svc_parms = (SVC_PARMS *)  iocb->svc_parms;
    ASSOC_IPC  *asipc = NULL;
    CS_SCB     *scb;
    HANDLE     hDisconnThread;
    i4         pendingio;
    SVC_PARMS  *disconn_svc_parms = NULL;

    GCTRACE(3)("\nGCCompletionRoutine: %p Entry point. dwError=%d\n",
		svc_parms, dwError);

    if ( svc_parms != NULL &&
	svc_parms->gc_cb != NULL )
    {
	asipc = (ASSOC_IPC *)  svc_parms->gc_cb;

        GCTRACE(3)("GCCompletionRoutine: Cmplt %d\n", asipc->asyncIoPending);

        if ( svc_parms->time_out != -1 && asipc->timer_event != 0 )
        {
            ASYNC_TIMEOUT *asto = asipc->async_timeout;
            WaitForSingleObject(gc_asto_mutex, INFINITE);
            /* cancel our timeout event */
            timeKillEvent(asipc->timer_event);
            asipc->async_timeout = NULL;
            asipc->timer_event = 0;
            asto->fd = INVALID_HANDLE_VALUE;
            ResetEvent(asto->event);
            GCRESRC( asto->event, CloseHandle(asto->event) );
            asto->event = INVALID_HANDLE_VALUE;
            ReleaseMutex(gc_asto_mutex);
        }
	if (iocb->flags & GC_IO_READFILE)
	{
	    GCreceiveCompletion(svc_parms, &dwError, dwNumberXfer);
	}
	else
	{
	    Gc_writes++;
	    CSGET_SCB(&scb);
	    if (scb)
	        scb->cs_biow++;
	}

        if (dwError)
        {
            /* was it canceled due to timeout? */
            if ( dwError == ERROR_OPERATION_ABORTED )
            {
	        svc_parms->status = GC_TIME_OUT;
                svc_parms->sys_err->errnum = dwError;
            }
            else
            {
	        svc_parms->status = GC_ASSOC_FAIL;
                svc_parms->sys_err->errnum = dwError;
            }
        }

        GCTRACE(3)( "GCCompletionRoutine: %p calling gca_service completion. level=%p\n",
		    svc_parms, &svc_parms );
        (*svc_parms->gca_cl_completion) (svc_parms->closure);
        pendingio = InterlockedDecrement( &asipc->asyncIoPending );
        GCTRACE(3)( "GCCompletionRoutine: %p gca_service completed. level=%p\n",
		    svc_parms, &svc_parms );

        hDisconnThread = asipc->hDisconnThread;
        disconn_svc_parms = asipc->Disconn_svc_parms;
        if ((asipc->flags & GC_IPC_DISCONN) && (pendingio == 0) &&
            (hDisconnThread != NULL))
        {
            GCTRACE(3)( "GCCompletionRoutine: %p queue disconnect completion\n",
			disconn_svc_parms );
            pfnQueueUserAPC((PAPCFUNC)GCasyncCompletion, hDisconnThread, 
                            (DWORD)disconn_svc_parms);
        }
    }

    return;
}


/******************************************************************************
**
** Name: GCasyncCompletion
**
** Description:
**	This function calls the completion routine for a request that 
**	completed in a thread other than the original thread of the
**	request.  This function is itself run as a completion routine
**	for a Windows User APC that is queued to the original thread.
**	There are 2 cases that use this:
**        1. ASYNC Connects that entered blocking code and were spawned
**	     off into a separate thread.
**        2. Disconnects after all I/Os have completed.
**      The thread must be in an alertable wait state for this function
**      to be executed.
**	TIP: There might be a temptation to eliminate this function
**	     and queue the UserAPC directly to the actual requester's
**	     completion routine since, as seen below, this routine does
**	     nothing more than turn around and call the requester's
**	     completion routine.  DON'T BOTHER! ... the call mechanism
**	     for this Windows UserAPC routine is WINAPI which is #defined
**	     to __stdcall, which is different from the default call
**	     mechanism which is used by all the Ingres programs.  If that
**	     were to ever get in sync, then this middle-man function could
**	     be eliminated.
**
** Inputs:
**      svc_parms      pointer to svc_parms parameter list
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      03-Dec-1999 (fanra01)
**          Created.
**      26-Aug-2005 (lunbr01) Bug 115107
**          Change this to an APC function rather than an I/O completion
**          routine.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Change name of this function from GCasyncDisconn to
**	    GCasyncCompletion to indicate its more general purpose,
**	    now that it is used for connects in addition to disconnects.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add TIP doc above and eliminate use of asipc below as it was
**	    not really being used. Add extra line break in tracing for
**	    improved readability for what runs between start and finish
**	    of this async execution.
**
******************************************************************************/
VOID WINAPI
GCasyncCompletion( SVC_PARMS *svc_parms)
{
    GCTRACE(3)( "\nGCasyncCompletion: %p Entry point.\n", svc_parms );

    if ((svc_parms != NULL) &&
        (!IsBadReadPtr(svc_parms, sizeof(SVC_PARMS))))
    {
        svc_parms->sys_err->errnum = 0;
        svc_parms->status = OK;
        (*svc_parms->gca_cl_completion) (svc_parms->closure);
        GCTRACE(3)( "GCasyncCompletion: gca_service completed.\n" );
    }
    GCTRACE(3)( "GCasyncCompletion: Exit point.\n\n" );
    return;
}


/******************************************************************************
**
** Name: GCsetWaitableTimer
**
** Description:
**	Calls SetWaitableTimer for GClisten() requests to set the
**	timer to go off 'svc_parms->timeout' seconds after calling the
**	SetWaitableTimer function.
**
** Input:
**	svc_parms		Ptr to service parms.
**
** Output:
**      None.
**
** Returns:
**	VOID
**
** History:
**	26-Jul-98 (rajus01)
**	    Created.
**
*******************************************************************************/
static
void 
GCsetWaitableTimer( SVC_PARMS *svc_parms )
{

    LARGE_INTEGER	li;

    LONGLONG nanosec = (LONGLONG) svc_parms->time_out * 10000;

    GCTRACE(3) ("GCsetWaitableTimer: %p timeout(milli_sec)=%d\n",
		svc_parms, svc_parms->time_out);

    /* Negate the time so that SetWaitableTimer knows
    ** we want relative time instead of absolute time.
    ** This negative value tells the function to go off 
    ** at a time relative to the time when we call the function.
    */
    nanosec=-nanosec;

    /*
    ** Copy the relative time from a __int64 into a
    ** LARGE_INTEGER.
    */

    li.LowPart = (DWORD) (nanosec & 0xFFFFFFFF );
    li.HighPart = (LONG) (nanosec >> 32 );

    svc_parms->time_out = -1;
			
    (*pfnSetWaitableTimer)( hAsyncEvents[TIMEOUT], &li, 0, NULL, NULL, FALSE );

    GCTRACE(3)("Setwaitable timer was called. \n");
}

/******************************************************************************
**
** Name: GClisten_timeout
**
** Description:
**
**	Callback function for (listen) TIMEOUT event.
**
** Input:
**	None	
**
** Output:
**      None.
**
** Returns:
**	VOID
**
** History:
**	27-Jul-98 (rajus01)
**	    Created.
**      04-Aug-2001 (lunbr01)  Bug 105239
**          After a listen timeout, hide the asipc from the current
**          session since we'll reuse it for the new listen.
**
*******************************************************************************/
VOID
GClisten_timeout()
{
    ASYNC_IOCB	*iocb      = &ListenIOCB;
    SVC_PARMS	*svc_parms = iocb->svc_parms;
    CS_SCB	*scb;
    HANDLE pHandle;
    DWORD  priority = 0;
    static bool priorityWasSet = FALSE;
 

    if (!priorityWasSet && is_gcn)
    {
        pHandle = OpenProcess(PROCESS_SET_INFORMATION | 
            PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
        if (pHandle)
        {
            priority = GetPriorityClass(pHandle);
            GCTRACE(3)("GClisten_timeout: process prority is %d\n",priority);
            if (priority != NORMAL_PRIORITY_CLASS && priority != 0 )
                SetPriorityClass(pHandle, NORMAL_PRIORITY_CLASS);
        }
        CloseHandle(pHandle);
    }
    priorityWasSet = TRUE;

    GCTRACE(3)("GClisten_timeout: Entry point.\n");

    CSGET_SCB(&scb);

    iocb->flags &= ~GC_IO_TIMEOUT;
    /*
    ** Since the listen asipc will be reused for the new listen
    ** which will come in by calling the completion routine,
    ** hide it from any further processing on this session by
    ** clearing it in the svc_parms; otherwise, the GCdisconn
    ** that is coming will clear all of the pipe channels.
    */
    svc_parms->gc_cb = NULL; 
    GCerror( GC_TIME_OUT, 0 );
}

/******************************************************************************
**
** Name: GCrectimeout
**
** Description:
**      Timer-expired callback routine for receives.  Sets async I/O timeout
**	event when timer goes off.
**
** Inputs:
**      uID         timer id
**      uMsg        pointer to service parameter structure
**      dwUser      pointer user-supplied callback data
**      dw1         pointer to dw1
**      dw2         pointer to dw2
**
** Outputs:
**      asto->event is set
**
** Returns:
**      None.
**
** History:
**
*******************************************************************************/
static
void
CALLBACK
GCrectimeout(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
    ASYNC_TIMEOUT *asto = (ASYNC_TIMEOUT *)dwUser;

    WaitForSingleObject(gc_asto_mutex, INFINITE);
    if ( asto->event != INVALID_HANDLE_VALUE )
        SetEvent( asto->event );
    ReleaseMutex(gc_asto_mutex);

    return;
}


/******************************************************************************
**
** Name:  GCcreateSemaphore.
**
** Description:
**  Create a syncronization semaphore for the purge event based on the
**  association id and the process id.  This should synchronize the purge send/
**  receive per connection exclusively.
**
** Inputs:
**  Address of ASSOC_IPC control block for the session.
**
** Outputs:
**  Handle of purge semaphore saved in ASSOC_IPC control block.
**
** Returns:
**	0	On error.
**	1  	OK...semaphore created.
**
** History:
**	23-Oct-2001 (lunbr01) Bug 105793 & 106119
**	    Move purge semaphore from global array to the per-session
**          ASIPC control block. This is much simpler and removes the 
**          restriction on max associations imposed by the pre-defined
**          array size. The sequential 1-100 sem_id portion of the
**          purge semaphore is replaced by the address of the ASIPC...
**          which is also unique within process.  The purge semaphore
**          will now be preserved across GCA_SAVE/GCA_RESTORE.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**
*******************************************************************************/
DWORD
GCcreateSemaphore(ASSOC_IPC * asipc)
{
    DWORD   status = 0;
    SECURITY_ATTRIBUTES	sa;
    char		szSemaphoreName[MAX_PATH];
    char		*ObjectPrefix;

    iimksec (&sa);
    sa.bInheritHandle = TRUE;
    GVshobj(&ObjectPrefix);

    if ( asipc->hPurgeSemaphore )
    {
	GCTRACE(2)( "GCcreateSemaphore - semaphore already created!?\n" );
    }
    else
    {
	STprintf(szSemaphoreName, "%sGCprg%x%x", ObjectPrefix,
	     		GetCurrentProcessId(), asipc);

	GCRESRC( asipc->hPurgeSemaphore, asipc->hPurgeSemaphore =
			CreateSemaphore( &sa, 0, 5, szSemaphoreName ) );

	GCTRACE(2)( "GCcreateSemaphore - Created %s [%d,%d]\n", 
		szSemaphoreName, asipc, asipc->hPurgeSemaphore );
        status = TRUE;
    }

    return(status);
}

/******************************************************************************
**
** Name:  GCdestroySemaphore.
**
** Description:
**  Closes the purge semaphore.
**
** Inputs:
**  Address of ASSOC_IPC control block for the session.
**
** Outputs:
**  Clears handle of purge semaphore saved in ASSOC_IPC control block.
**
** Returns:
**  none.
**
** History:
**	23-Oct-2001 (lunbr01) Bug 105793 & 106119
**	    Move purge semaphore from global array to the per-session
**          ASIPC control block.
**
*******************************************************************************/
VOID
GCdestroySemaphore(ASSOC_IPC * asipc)
{
    if ( asipc->hPurgeSemaphore )
    {
        GCRESRC( asipc->hPurgeSemaphore, CloseHandle( asipc->hPurgeSemaphore ) );
        GCTRACE(2)( "GCdestroySemaphore - Destroyed [%d,%d]\n",
                    asipc, asipc->hPurgeSemaphore);
        asipc->hPurgeSemaphore = NULL;
    }
}



/******************************************************************************
** Name: GCrpt_stats    - Report GC statistics
**
** Description:
**
**  Print/trace various internal statistics that can be used for problem
**  diagnosis and performance tuning.  This function is currently called
**  at shutdown from GCterminate() if tracing is on.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Returns:
**      none
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Created.
**
******************************************************************************/
VOID
GCrpt_stats()
{
    i4	GCrequest_delay_calls;
    i4	total, success, failures;

    TRdisplay("\n%31*- GC Statistics %31*-\n\n");

    TRdisplay(" *** Named Pipes (gcacl.c) ***\n");
    TRdisplay("                         Total    Success  Failures\n");
    TRdisplay("                       --------  --------  --------\n");

    failures = GCrequest_calls - GCrequest_OK;
    TRdisplay("  GCrequest (#)     : %9d %9d %9d\n",
	      GCrequest_calls, GCrequest_OK, failures);

    /*
    **  Delayed calls are those that were not immediately completed.
    **  That is, the connect to the server pipe did not return with success
    **  so function GCconnectToServer() was invoked to block/wait until
    **  a server pipe instance became available and could be connected to.
    **
    **  Report the total # of these as well as the average # of milliseconds
    **  that it took for the operation to finally complete (for delayed ones
    **  only...not including immediately connecting sessions).
    **
    **  While the delay time is not the full time it takes to do a connect,
    **  it should be the bulk of it and warrants breaking out as a separate
    **  statistic.
    */
    GCrequest_delay_calls = GCrequest_delay_OK + GCrequest_delay_FAIL;
    TRdisplay("    - delayed (#)   : %9d %9d %9d\n",
	      GCrequest_delay_calls, GCrequest_delay_OK, GCrequest_delay_FAIL);
    if (GCrequest_delay_calls)
    {
        total   = (i4)((GCrequest_delay_OK_msecs + GCrequest_delay_FAIL_msecs) / GCrequest_delay_calls);
        success = GCrequest_delay_OK   ? (i4)(GCrequest_delay_OK_msecs   / GCrequest_delay_OK)   : 0;
        failures= GCrequest_delay_FAIL ? (i4)(GCrequest_delay_FAIL_msecs / GCrequest_delay_FAIL) : 0;
        TRdisplay("         (avg msec) : %9d %9d %9d\n",
		  total, success, failures);
    }

    TRdisplay("\n%77*-\n\n");

    return;
}

/******************************************************************************
*******************************************************************************
**
**  Named Pipe routines
**
*******************************************************************************
******************************************************************************/

/******************************************************************************
**
** Name:	GCcreateStandardPipes
**
** Description: Creates the named pipes.
**
** History:
**	26-jun-95 (emmag)
**	    Max number of connections should be set in config.dat
**	    and not hard-coded.
**	14-mar-1996 (canor01)
**	    Max number of connections may be greater than 255, but
**	    max pipe instances is limited to 255.
**	03-apr-1996 (canor01)
**	    Clean up code. Move MEfree's to proper place.
**	20-apr-1996 (canor01)
**	    If the pipe is busy, retry.
**	22-apr-1996 (canor01)
**	    Max number of pipes is greater than max number of
**	    connections to allow for the listen thread's pipes
**	    to stay active.
**      09-Sep-1999 (fanra01)
**          Moved Sleep from loop to individual retry.
**          Move GetLastError calls before calling GCcloseAllPipes.
**      09-Dec-1999 (fanra01)
**          Only retry pipes which have not previously been created or failed
**          to be created.
**	05-may-2000 (somsa01, lunbr01)
**	    Moved creation of the RcvStdChan to the top to speed up client
**	    connections.
**	12-may-2000 (somsa01)
**	    Moved PMget() of the connect limit to GCregister().
**	10-Feb-2005 (drivi01)
**	    Instead of obtaining connect_limit from config.dat and setting
**	    num_pipes to connect_limit, we now set it to PIPE_UNLIMITED_INSTANCES.
**	    GC should not be trying to limit the number of pipes created for
**	    communication, server should have control over how many connections
**	    can be obtained.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    DUPLEX on pipes is unnecessary...change to 1-directional 
**	    depending on usage.
**	11-Sep-2009 (Bruce Lunsford) Bug 122589 and 122426
**	    Restore DUPLEX to pipes...needed for backward compatibility to
**	    older clients using direct connect or OpenROAD statically-linked
**	    local connections.
**
******************************************************************************/
DWORD
GCcreateStandardPipes(ASSOC_IPC *asipc)
{
    DWORD 	status = OK;
    int		num_pipes = PIPE_UNLIMITED_INSTANCES;
    int		retry;
    SECURITY_ATTRIBUTES sa;

    asipc->SndStdChan = NULL;
    asipc->RcvStdChan = NULL;
    asipc->SndExpChan = NULL;
    asipc->RcvExpChan = NULL;
    asipc->ClientReadHandle = NULL;

    iimksec (&sa);
    sa.bInheritHandle = TRUE;


    GCTRACE(3)("GCcreateStandardPipes: Entry point.\n");

    for (status = ERROR_PIPE_BUSY, retry = PIPE_RETRIES;
         status == ERROR_PIPE_BUSY && retry > 0;
         retry-- )
    {
	if (asipc->RcvStdChan == NULL)
        {
            GCRESRC( asipc->RcvStdChan,
                asipc->RcvStdChan = CreateNamedPipe( Cli2srv_std_pipe_name,
                    PIPE_ACCESS_DUPLEX |
                    FILE_FLAG_OVERLAPPED,
                    PIPE_WAIT |
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE,
                    num_pipes,
                    Pipe_buffer_size,
                    Pipe_buffer_size,
                    0,
                    &sa) );
            if (asipc->RcvStdChan == INVALID_HANDLE_VALUE)
            {
                asipc->RcvStdChan = NULL;
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                    Sleep(PIPE_RETRY_SLEEP);
                continue;
            }
	    else
		STcopy(Cli2srv_std_pipe_name, asipc->RcvStdName);
        }

	if (asipc->SndStdChan == NULL)
	{
            GCRESRC( asipc->SndStdChan,
                asipc->SndStdChan = CreateNamedPipe( Srv2cli_std_pipe_name,
                    PIPE_ACCESS_DUPLEX |
                    FILE_FLAG_OVERLAPPED,
                    PIPE_WAIT |
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE,
                    num_pipes,
                    Pipe_buffer_size,
                    Pipe_buffer_size,
                    0,
                    &sa ) );
            if (asipc->SndStdChan == INVALID_HANDLE_VALUE)
            {
                asipc->SndStdChan = NULL;
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                    Sleep(PIPE_RETRY_SLEEP);
                continue;
            }
	    else
		STcopy(Srv2cli_std_pipe_name, asipc->SndStdName);
	}

	if (asipc->SndExpChan == NULL)
        {
            GCRESRC( asipc->SndExpChan,
                asipc->SndExpChan = CreateNamedPipe( Srv2cli_exp_pipe_name,
                    PIPE_ACCESS_DUPLEX |
                    FILE_FLAG_OVERLAPPED,
                    PIPE_WAIT |
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE,
                    num_pipes,
                    Pipe_buffer_size,
                    Pipe_buffer_size,
                    0,
                    &sa) );
            if (asipc->SndExpChan == INVALID_HANDLE_VALUE)
            {
                asipc->SndExpChan = NULL;
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                    Sleep(PIPE_RETRY_SLEEP);
                continue;
            }
	    else
		STcopy(Srv2cli_exp_pipe_name, asipc->SndExpName);
	}

	if (asipc->RcvExpChan == NULL)
        {
            GCRESRC(asipc->RcvExpChan,
                asipc->RcvExpChan = CreateNamedPipe( Cli2srv_exp_pipe_name,
		PIPE_ACCESS_DUPLEX |
		FILE_FLAG_OVERLAPPED,
		PIPE_WAIT |
		PIPE_TYPE_MESSAGE |
		PIPE_READMODE_MESSAGE,
		num_pipes,
		Pipe_buffer_size,
		Pipe_buffer_size,
		0,
		&sa) );
            if (asipc->RcvExpChan == INVALID_HANDLE_VALUE)
            {
                asipc->RcvExpChan = NULL;
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                Sleep(PIPE_RETRY_SLEEP);
                continue;
            }
	    else
		STcopy(Cli2srv_exp_pipe_name, asipc->RcvExpName);
	}

	status = OK;
    }
    return status;
}


/******************************************************************************
**
** Name: GCconnectToServerPipe - Connect to server pipe (SndStd)
**
** Description:
**
** This routine will spin/wait until it can obtain (ie, connect to) the
** server pipe (client's SndStd pipe) or until it times out.  Note that
** the server pipe is made available when the server listen routine issues
** a CreateNamedPipe on its matching pipe (RcvStd).
**
** When this routine is called, it is assumed that an unsuccessful attempt
** to connect to the pipe has already been made, so the logic begins with
** a WaitNamedPipe() rather than a CreateFile().
**
** Once the initial server pipe is connected to, the caller can continue
** by connecting to the remaining pipes (GCconnectToStandardPipes()).
**
** Inputs:
**   svc_parms              Pointer to GCA service parmeter list containing:
**
** Outputs:
**  svc_parms->size_advise  Suggested buffer size for communication
**                          optimization.
**
**  svc_parms->gc_cb     Pointer to the LCB.
**
**  svc_parms->status       Success or failure indication.
**      OK                  Normal completion
**      GC_RQ_FAIL          Request failure: connection problem
**      GC_RQ_RESOURCE      Request failure: server resource shortage
**
** Returns:
**  STATUS
**
** History:
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**          Split out from GCrequest().
**	    Remove check for GCN<->GCC deadlock, which is no longer needed
**	    with async connects.
**
******************************************************************************/

DWORD
GCconnectToServerPipe(SVC_PARMS *svc_parms)
{
    DWORD	status = OK;
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    char	*szPipeName = asipc->SndStdName;
    SECURITY_ATTRIBUTES pipe_security;
    bool	infinite_timeout = FALSE;
    i4		timeout;
    i4		retry = 0;
    i4		retry_CreateFile = 0;
    i4		init_time = 0;
    i4		start_time = 0;      /* When request to connect started */
    i4		time_to_connect = 0; /* Elapsed msecs to connect or fail*/
    i4		time_to_connect_approx = 0;  /* Approximate Elapsed msecs*/

    GCTRACE(3)("GCconnectToServerPipe: %p Entry point.\n", svc_parms);

    pipe_security.nLength              = sizeof(pipe_security);
    pipe_security.lpSecurityDescriptor = NULL;
    pipe_security.bInheritHandle       = TRUE;

    /*
    ** Determine named pipe timeout.
    */
    switch (svc_parms->time_out)
    {
	case 0:
	    timeout = NMPWAIT_USE_DEFAULT_WAIT;
	    break;

	case -1:
	    timeout = PIPE_RETRY_SLEEP * 2;
	    infinite_timeout = TRUE;
	    break;

	default:
	    timeout = svc_parms->time_out;
	    break;
    }

    /*
    ** Get accurate time to connect/fail if tracing on ... 
    ** else avoid overhead of calling OS function.
    */
    if (GC_trace)
	start_time=GetTickCount();

    /*
    ** Connect to server:
    **   1. Wait for server pipe instance to be available.
    **   2. Then try to connect to it.
    ** Repeat 1 & 2 until connected or timed out.
    **
    ** ERROR_PIPE_BUSY means server is processing another
    ** connect request and has not yet created a new instance
    ** of the pipe.
    */
    for (retry_CreateFile=0;;retry_CreateFile++)
    {
	init_time = GetTickCount();
	GCTRACE(3)("GCconnectToServerPipe: %p while WaitNamedPipe timeout=%d retry=%d status=%d\n",
		     svc_parms, timeout, retry, status);
	while (timeout > 0 && retry < (PIPE_RETRIES * 240))
	{
	    /*
	    ** Since WaitNamedPipe() does not put us into an
	    ** alertable state, any existing I/O's cannot complete.
	    ** Thus, we've added a SleepEx() call to mimic this
	    ** behavior.
	    */
	    SleepEx(0, TRUE);

	    if (!WaitNamedPipe(szPipeName, PIPE_RETRY_SLEEP))
	    {
		status = GetLastError();
		if (status != ERROR_FILE_NOT_FOUND &&
		    status != ERROR_PIPE_BUSY &&
		    status != ERROR_SEM_TIMEOUT)
		{
		    break;
		}
	    }
	    else   /* pipe instance is available */
	    {
		status = OK;
		break;
	    }
	    GCTRACE(3)("GCconnectToServerPipe: %p WaitNamedPipe status=%d  RETRY\n",
		        svc_parms, status);

	    /* Accumulate approx time to connect/fail */
	    if (status == ERROR_SEM_TIMEOUT)
		time_to_connect_approx+=PIPE_RETRY_SLEEP;  

	    if (infinite_timeout)
		retry++;
	    else
		timeout -= PIPE_RETRY_SLEEP;

	} /* end while retry WaitNamedPipe() */
	GCTRACE(3)("GCconnectToServerPipe: %p end while WaitNamedPipe elapsed time(msec)=%d timeout=%d retry=%d status=%d\n",
		     svc_parms,
		     GetTickCount() - init_time, timeout, retry,
		     status);

	if (status == OK)
	{
	    /* Pipe instance now available, so try to connect to it */
	    GCRESRC(asipc->SndStdChan, asipc->SndStdChan =
		    CreateFile( szPipeName,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				&pipe_security,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				NULL) );

	    if (asipc->SndStdChan == INVALID_HANDLE_VALUE)
	    {
		asipc->SndStdChan = NULL;
		status = GetLastError();
		GCTRACE(3)("GCconnectToServerPipe: %p CreateFile INVALID_HANDLE PipeName=%s status=%d\n",
			    svc_parms, szPipeName, status);
		if (status == ERROR_FILE_NOT_FOUND ||
		    status == ERROR_PIPE_BUSY)
		{
		    /*
		    ** Someone else grabbed the pipe instance before we
		    ** could get it...try again for another instance.
		    */
		    continue;  /* loop back to WaitNamedPipe() */
		}
		else /* real error on pipe CreateFile() */
		    break;
	    } /* end if pipe CreateFile() returned invalid handle */

	    status = OK;   /* Pipe instance connected to!! */
	    break;

	} /* end if status == OK (from WaitNamedPipe() loop) */
	else
	    break;    /* Pipe instance never became available!   */

    } /* end "for ;;" loop around WaitNamedPipe()/CreateFile() */

    /*
    ** Get time to connect if tracing on ... else avoid overhead
    ** of calling OS function and just use approx time.
    ** Also use approx time if GetTickCount() wrapped to zero.
    */
    if (GC_trace)
	time_to_connect=(GetTickCount() - start_time);
    if (time_to_connect <= 0)
	time_to_connect = time_to_connect_approx;

    GCTRACE(3)("GCconnectToServerPipe: End for CreateFile time_to_connect=%d approx=%d retry_CreateFile=%d status=%d\n",
		time_to_connect, time_to_connect_approx, retry_CreateFile, status);

    if (status != OK)
    {
	/* For internal connect(failed) stats */
	GCrequest_delay_FAIL++;
	GCrequest_delay_FAIL_msecs+=time_to_connect;

	GCdestroySemaphore( asipc );
	if ((status == ERROR_PIPE_BUSY || status == ERROR_SEM_TIMEOUT)
	&&   timeout <=0 && svc_parms->time_out > 0)
	    svc_parms->status = GC_TIME_OUT;
	else
	    svc_parms->status = GC_NO_PARTNER;
	return(status);
    }

    /* For internal connect(OK) stats */
    GCrequest_delay_OK++;
    GCrequest_delay_OK_msecs+=time_to_connect;

    return(status);
}

/******************************************************************************
**
** Name:  GCconnectToStandardPipes
**
** Description:
**  Open the NORMAL and EXPEDITE pipes.  If the open fails, sleep
**  for a bit and try again.
**
** Inputs:
**  asipc
**
** Outputs:
**  none
**
** Returns:
**  STATUS
**
** History:
**      09-Sep-1999 (fanra01)
**          Add handle checks prior to CreateFile for retry loop.  Don't
**          recreate handles.
**          Add retry on pipe busy and release of resources on error.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Moved logic that built pipe names to GCrequest() so that it
**	    is already done and stored in the asipc; hence, no need to
**	    pass partner_name anymore as a parameter.  This was not a
**	    critical thing to do...just helped with organization of pipe
**	    naming logic.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove unnecessary optons GENERIC_READ & FILE_SHARE_WRITE from
**	    sending pipes, GENERIC_WRITE & FILE_SHARE_READ from receiving
**	    pipes, and FILE_ATTRIBUTE_NORMAL from all pipes.
******************************************************************************/

DWORD
GCconnectToStandardPipes(ASSOC_IPC *asipc)
{
    DWORD status = OK;
    SECURITY_ATTRIBUTES pipe_security;

    GCTRACE(3)("GCconnectToStandardPipes: Entry point.\n");

    pipe_security.nLength              = sizeof(pipe_security);
    pipe_security.lpSecurityDescriptor = NULL;
    pipe_security.bInheritHandle       = TRUE;

    for (;;)
    {
        if ((asipc->RcvStdChan == NULL) ||
            (asipc->RcvStdChan == INVALID_HANDLE_VALUE))
        {
            GCRESRC( asipc->RcvStdChan,
                asipc->RcvStdChan = CreateFile( asipc->RcvStdName,
                    GENERIC_READ,
                    FILE_SHARE_WRITE,
                    &pipe_security,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL) );
            if (asipc->RcvStdChan == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                {
                    Sleep( PIPE_RETRY_SLEEP );
                    status = OK;
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        if ((asipc->RcvExpChan == NULL) ||
            (asipc->RcvExpChan == INVALID_HANDLE_VALUE))
        {
            GCRESRC( asipc->RcvExpChan,
                asipc->RcvExpChan = CreateFile( asipc->RcvExpName,
                    GENERIC_READ,
                    FILE_SHARE_WRITE,
                    &pipe_security,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL) );
            if (asipc->RcvExpChan == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                {
                    Sleep( PIPE_RETRY_SLEEP );
                    status = OK;
                    continue;
                }
                else
                {
                    GCRESRC( asipc->RcvStdChan,
                        CloseHandle( asipc->RcvStdChan ) );
                    asipc->RcvStdChan = NULL;
                    break;
                }
            }
        }

        if ((asipc->SndExpChan == NULL) ||
            (asipc->SndExpChan == INVALID_HANDLE_VALUE))
        {
            GCRESRC( asipc->SndExpChan,
                asipc->SndExpChan = CreateFile( asipc->SndExpName,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    &pipe_security,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL) );
            if(asipc->SndExpChan == INVALID_HANDLE_VALUE)
            {
                status = GetLastError();
                if (status == ERROR_PIPE_BUSY)
                {
                    Sleep( PIPE_RETRY_SLEEP );
                    status = OK;
                    continue;
                }
                else
                {
                    GCRESRC( asipc->RcvStdChan,
                        CloseHandle( asipc->RcvStdChan ) );
                    asipc->RcvStdChan = NULL;
                    GCRESRC( asipc->RcvExpChan,
                        CloseHandle( asipc->RcvExpChan ) );
                    asipc->RcvExpChan = NULL;
                    break;
                }
            }
        }
        break;

    }

    return status;
}

/******************************************************************************
**
** Name: GCqualifiedPipeName   - format a pipe name 
**
** Description:	Format a pipe name, given a fully qualified name
**		that starts with a server name and ends with the
**		connect id.  An infix parameter is inserted between
**	 	to name the particular pipe.
**
** Inputs:
**	partner_name	a string in the format of "\\SERVER"
**
**	infix		a string specifying the particular pipe,
**			such as "C2S_EXP"
**
** Outputs:
**	pipename	a pointer to an area large enough to hold the
**			completed string
**
** History:
**	22-apr-1997 (canor01)
**	    Created.
**
*******************************************************************************/
static
void
GCqualifiedPipeName( char *partner_name, char *infix, char *pipename )
{
    char *machine = partner_name;
    char *cp;

    /* find end of machine name; skip over the "\\\\" */
    if ( (cp = STindex( partner_name + 2, "\\", 0 )) == NULL )
    {
	/* we're lost */
	STcopy( pipename, partner_name );
    }
    else
    {
	*cp = EOS;
	STprintf( pipename, "%s%s\\%s\\%s", machine, PIPE_QUAL_PREFIX,
		  infix, cp+1 );
	*cp = '\\';
    }
    return;
}

/******************************************************************************
**
** Name: GCNamePipeSend
**
** Description:
**      Extracted NT specific write from GCsend.
**      Writes to the pipe and expects the completion routine to be called
**      when the I/O completes.
**
** Inputs:
**      fd          handle to the write pipe
**      svc_parms   pointer to service parameter structure
**      asipc       pointer to association parameter structure
**      iocb        pointer to I/O control block
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**
** Returns:
**      status      0 or ERROR_IO_PENDING success
**                  else failure
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**      03-Dec-1999 (fanra01)
**          Add completion routine function pointer as argument.
**	17-mar-2003 (somsa01)
**	    Make sure we free the iocb if WriteFileEx() fails.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	    Check for error/warning conditions after successful write
**	    to the pipe, per recommendation in Microsoft docs.
**	    Clean up tracing: "GCsend" changed to "GCNamePipeSend" and
**	    more detail added to distinguish previously identical traces.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Move increment of asyncIoPending (on success) and MEfree of
**	    iocb (on failure) up to caller for more consistency across
**	    various types of sends. Requires status to be set correctly
**	    upon return.
*******************************************************************************/
DWORD
GCNamePipeSend( HANDLE fd, SVC_PARMS *svc_parms, ASSOC_IPC *asipc,
    ASYNC_IOCB *iocb, VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) )
{
    DWORD   status = 0;

    if (!WriteFileEx( fd,
        svc_parms->svc_buffer,
        svc_parms->svc_send_length,
        &iocb->Overlapped,
        completion ))
    {
        status = GetLastError();
        GCTRACE(1)( "GCNamePipeSend: WriteFileEx status %d\n", status );
        if ( status != ERROR_IO_PENDING )
        {
	    DWORD errstat;

            if ( status == ERROR_NO_DATA ) /* Pipe being closed */
                errstat = GC_ASSOC_FAIL;
            else
                errstat = NP_IO_FAIL;

            GCTRACE(1)( "GCNamePipeSend: WriteFileEx status %d, errstat=%d\n",
			 status, errstat);
	    svc_parms->sys_err->errnum = status;
	    svc_parms->status = (errstat);
	    (*svc_parms->gca_cl_completion) (svc_parms->closure);
        }
        else
        {
            GCTRACE(3)("GCNamePipeSend: (Write pending)\n");
        }
    }
    else
    {
	/* per MS docs, ck status even if write successful. */
        status = GetLastError();
        if ((status != ERROR_SUCCESS) && (status != ERROR_IO_PENDING))
            GCTRACE(1)( "GCNamePipeSend: WriteFileEx success but status=%d\n", status );
        GCTRACE(3)("GCNamePipeSend: (Write successful)\n");
	status = 0;
    }
    return (status);
}

/******************************************************************************
**
** Name: GCNamePipeReceive
**
** Description:
**      Extracted NT specific read from GCreceive.
**
** Inputs:
**      fd          handle to the write pipe
**      svc_parms   pointer to service parameter structure
**      asipc       pointer to association parameter structure
**      iocb        pointer to I/O control block
**      expeditied  flag indicates read channel
**
** Outputs:
**      svc_parms->rcv_data_length      amount of data read from the pipe
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**      09-Sep-1999 (fanra01)
**          Add GetOverlappedResult call to get read data length after a wait.
**      03-Dec-1999 (fanra01)
**          Add completion routine function pointer as argument.
**      27-sep-2001 (lunbr01) Bug 105902
**          Add call to GCreceiveCompletion() to check for GC purge 
**          message (+misc) in sync receive logic--
**          previously only done for async in GCCompletionRoutine.
**      19-Jan-2006 (lunbr01) Bug 115622
**          Slow performance for syncronous receives with timeout value (not
**          0 or -1).  Prior code used PeekNamedPipe plus 100 msec Sleep
**          in a loop to check for input before issuing read.  The slowness
**          was due to dropping into the Sleep when input was not already
**          present.  Fix removed Peek/Sleep code and replaced with timeout
**          value on WFSO after read plus cancelIO if timeout occurred.
**      12-Jan-2007 (lunbr01) Bug 117257
**          "Copy in data" is slower after fix for bug 115622.  This is
**          due to frequent expedited sync GCA_RECEIVEs with a zero timeout 
**          being issued by "copy in"...aka "copy from file".  Usually,
**          a timeout error is produced because there is nothing to receive
**          from the server.  The performance difference is due to it taking
**          longer in the code now to determine that there is no data.
**          Added code similar to pre-115622 that, in this case, would do a
**          PeekNamedPipe() and bail right away if nothing is there, 
**          instead of scheduling I/O, waiting on it for zero time, and then
**          having to cancel the I/O (in case some came through afterwards).
**          The PeekNamedPipe approach has less system overhead. 
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Check for error/warning conditions after successful read
**	    from the pipe, per recommendation in Microsoft docs.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add trace at entry.  Remove MEfree(iocb)'s since the send/recv
**	    iocbs are now preallocated and not freed until disconnect.
**
*******************************************************************************/
VOID
GCNamePipeReceive (HANDLE fd, SVC_PARMS *svc_parms, ASSOC_IPC *asipc,
    ASYNC_IOCB *iocb, BOOL expedited,
    VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) )
{
ULONG      status;
ULONG      amount_read = 0;
DWORD      dwAvail;
SECURITY_ATTRIBUTES sa;
bool       sync_flag = svc_parms->flags.run_sync;
STATUS     errstat;
DWORD      status_wait = 0;

    GCTRACE(3)("GCNamePipeReceive: %p Entry point.\n", svc_parms);

    iimksec (&sa);

    if (svc_parms->time_out != 0 && (svc_parms->time_out == -1 || !sync_flag))
    {
        if ( svc_parms->time_out > 0 )
        {
            ASYNC_TIMEOUT *asto;

            if ( gc_asto_mutex == 0 )
            {
                GCRESRC( gc_asto_mutex,
                    gc_asto_mutex = CreateMutex(&sa, FALSE, NULL) );
            }
            if ( gc_compl_key == 0 )
            {
                ME_tls_createkey( &gc_compl_key, &status );
            }
            ME_tls_get( gc_compl_key, (PTR *)&asto, &status );
            if ( asto == NULL )
            {
                asto = (ASYNC_TIMEOUT *)MEreqmem( 0, 
                                                  sizeof(ASYNC_TIMEOUT), 
                                                  TRUE, 
                                                  NULL);
                ME_tls_set( gc_compl_key, (PTR)asto, &status );
            }
            if (asto)
            {
                asto->fd = fd;
                GCRESRC( asto->event,
                    asto->event = CreateEvent( &sa, FALSE, FALSE, NULL ) );
                asipc->timer_event = timeSetEvent( svc_parms->time_out, 
                                                   10, 
                                                   (LPTIMECALLBACK)GCrectimeout,
                                                   (DWORD)asto,
                                                   TIME_ONESHOT );
                asipc->async_timeout = asto;
            }
        }

        if (!ReadFileEx(fd,
                        svc_parms->svc_buffer,
                        svc_parms->reqd_amount,
                        &iocb->Overlapped,
                        completion))
        {
            if ((status = GetLastError()) != ERROR_IO_PENDING)
            {
                if ( expedited == TRUE )
                {
                    SleepEx( 0, TRUE );
                    return;
                }
                errstat = NP_IO_FAIL;
                GCerror(errstat, status);
            }
        }
	else
	{
	    /* per MS docs, ck status even if read successful. */
            status = GetLastError();
            if ((status != ERROR_SUCCESS) && (status != ERROR_IO_PENDING))
        	GCTRACE(1)( "GCreceive: ReadFileEx success but status=%d\n", status );
        }
        InterlockedIncrement( &asipc->asyncIoPending );
    }
    else    /* SYNCHRONOUS with timeout value not = -1  */
    {
	/* For performance, just do Peek when caller has 0 timeout. */
        if (svc_parms->time_out == 0)
        {
            if (!PeekNamedPipe(fd, NULL, 0, NULL, &dwAvail, NULL))
            {
                status = GetLastError();
                if ( expedited == TRUE )
                {
                    SleepEx( 0, TRUE );
                    return;
                }
                errstat = NP_IO_FAIL;
                GCerror(errstat, status);
            }
            else
            {
                if (!dwAvail)
                {
                    GCerror(GC_TIME_OUT, 0);
                }
            }
        }  /* endif timeout == 0 */

        GCRESRC( iocb->Overlapped.hEvent,
            iocb->Overlapped.hEvent = CreateEvent(&sa,
                TRUE,
                FALSE,
                NULL) );
        if (!ReadFile(fd,
                      svc_parms->svc_buffer,
                      svc_parms->reqd_amount,
                      &dwAvail,
                      &iocb->Overlapped))
        {
	    switch(status = GetLastError())
            {
                case ERROR_IO_PENDING:
                    GCRESRC( status_wait,
                             status_wait = WaitForSingleObject(
                                 iocb->Overlapped.hEvent,
                                 svc_parms->time_out) );
                    switch (status_wait)
                    {
                        case WAIT_TIMEOUT:
                             /* Timed out before any data received.
                             ** Cancel IO and return with timeout error.
                             */
                             GCTRACE(4)("GCNamePipeReceive: Sync timeout (%d secs) occurred\n", svc_parms->time_out );
		             if ( pfnCancelIo )
                                 (*pfnCancelIo)( fd );
                             CloseHandle(iocb->Overlapped.hEvent);
                             GCerror(GC_TIME_OUT, 0);
                             break;
                        case WAIT_OBJECT_0:
                             /* The event for the IO completed.  */
                             GCTRACE(4)("GCNamePipeReceive: Sync IO event signaled..get data\n" );
			     GetOverlappedResult(fd, &iocb->Overlapped, &dwAvail,
			          TRUE);
			     GCreceiveCompletion(svc_parms, &status, dwAvail);
			     break;
                        default:
                             GCTRACE(4)("GCNamePipeReceive: sync wait default - error %x\n", status_wait );
                             CloseHandle(iocb->Overlapped.hEvent);
                             if ( expedited == TRUE )
                             {
                                 SleepEx( 0, TRUE );
                                 return;
                             }
                             errstat = NP_IO_FAIL;
                             GCerror(errstat, status);
			     break;
                    }
                    break;

                default:
                    CloseHandle(iocb->Overlapped.hEvent);
                    if ( expedited == TRUE )
                    {
                        SleepEx( 0, TRUE );
                        return;
                    }
                    errstat = NP_IO_FAIL;
                    GCerror(errstat, status);
                    break;
            }
        }
        else
        {
            GCreceiveCompletion(svc_parms, &status, dwAvail);
        }
        GCRESRC( iocb->Overlapped.hEvent,
            CloseHandle(iocb->Overlapped.hEvent) );
        ReleaseSemaphore (SyncEvent, 1, NULL);
        GCerror(OK, 0);
    }
    return;
}


/******************************************************************************
**
** Function: GCcloseAllPipes
**
** Description:
**	Close all of the named pipes.
**
** History:
**	13-nov-95 (emmag)
**	    Changed to close four pipes instead of two.
**	06-may-1999 (somsa01)
**	    For the Server-ended pipes, we must also perform a
**	    DisconnectNamedPipe() before closing the handle.
**	10-may-1999 (somsa01)
**	    Remove the FlushFileBuffers() calls from last change, as they
**	    were causing hangs during the Replicator handoffqa tests.
**      01-Oct-1999 (fanra01)
**          Removed 0 length writes on receive pipes as all IO should be
**          completed by the time we get this call.
**
******************************************************************************/
VOID
GCcloseAllPipes(ASSOC_IPC * asipc)
{
        int i = 0;

	if (asipc->SndStdChan)
	{
	    GCRESRC( asipc->SndStdChan, CloseHandle(asipc->SndStdChan) );
	    asipc->SndStdChan  = NULL;
	}
	if (asipc->RcvStdChan)
	{
	    GCRESRC( asipc->RcvStdChan, CloseHandle(asipc->RcvStdChan) );
	    asipc->RcvStdChan  = NULL;
	}
	if (asipc->SndExpChan)
	{
	    GCRESRC( asipc->SndExpChan, CloseHandle(asipc->SndExpChan) );
	    asipc->SndExpChan  = NULL;
	}
	if (asipc->RcvExpChan)
	{
	    GCRESRC( asipc->RcvExpChan, CloseHandle(asipc->RcvExpChan) );
	    asipc->RcvExpChan  = NULL;
	}
	if (asipc->ClientReadHandle)
	{
	    GCRESRC( asipc->ClientReadHandle,
                CloseHandle(asipc->ClientReadHandle) );
	    asipc->ClientReadHandle  = NULL;
	}
	return;
}

/******************************************************************************
*******************************************************************************
**
**  Network Protocol Driver routines
**
*******************************************************************************
******************************************************************************/


/******************************************************************************
** Name: GC_get_driver - Get driver info from local IPC PCT.
**
** Description:
**
** This routine is called by GCinitiate() to look up the driver in the 
** local IPC driver table (aka, local PCT) if Ingres/environment
** symbol II_GC_PROT is defined.  If not defined or no match is found,
** then return NULL (which implies that the embedded, default, named pipes
** logic should be used).
**
** If a GCC-type driver has been requested and is found in the local PCT,
** then initialize various items for the driver, such as the driver
** control block.
**
** Inputs:
**     None in parmlist
**     Requested driver id (such as "tcp_ip") in II_GC_PROT env variable
**     Local IPC PCT (global)
**
** Outputs:
**     None in parmlist
**     Alloc'd and initialized driver control block (dcb) plus related items
**
** Returns:
**     GCC_PCE	Address of local PCT entry for communications driver
**
** Exceptions:
**     none
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.  (modeled after Unix version plus GCpinit() on Windows)
**
******************************************************************************/
GCC_PCE  *
GC_get_driver()
{
	char	*driver = NULL;		/* driver name */
	GCC_PCE	*driver_pce = NULL;	/* Set default driver -> none */
	THREAD_EVENTS	*driver_thread;	/* driver thread (dcb) */
	STATUS		status;
	STATUS		generic_status;
	CL_ERR_DESC	system_status;

	/* Scan for alternate driver */

	NMgtIngAt( "II_GC_PROT", &driver );

	if( driver && *driver )
	{
	    GCC_PCE	*pct = &GCdrivers[0];

	    while( pct->pce_pid && STcasecmp( driver, pct->pce_pid ) )
		    pct++;
	    
	    if( pct->pce_pid )
	    {
		driver_pce = pct;
	    }
	    else
	    {
	        GCTRACE(1) ("GC_get_driver: II_GC_PROT='%s' driver not found--using named pipes instead\n",
			driver);
	    }
	}

	/*
	** If no GCC net driver will be used, we're all done here.
	*/
	if (!driver_pce)
	    return NULL;

	GCTRACE(1) ("GC_get_driver: '%s' driver selected for local protocol\n",
		     driver);

	/*
	** Initialize some GCC objects to simulate a GCC environment
	** for the GCC network protocol driver.  This is basically
	** a subset of what is done by GCpinit() in a true GCC environment.
	** Setting 1st parm (pct) to NULL indicates a full PCT init is not
	** required.
	*/
	if ( (status = GCpinit( NULL, &generic_status, &system_status )) != OK )
	    return NULL;
	
        driver_thread = (THREAD_EVENTS *)MEreqmem( 0,
                                                sizeof(THREAD_EVENTS),
                                                TRUE, NULL );
	if (driver_thread == NULL)
	{
	    GCTRACE(1) ("GC_get_driver: Unable to alloc driver control block (dcb) - size=%d",
		     sizeof(THREAD_EVENTS));
	    return NULL;
	}

	/*
	** Init queues for driver thread.
	*/
	QUinit( &driver_thread->incoming_head );
	QUinit( &driver_thread->process_head );

	/*
	** Assign the driver thread name.
	*/
	driver_thread->thread_name = driver_pce->pce_pid;

	/*
	** Point the PCE's pce_dcb (driver control block) to this
	** thread event structure to enable the driver to find it.
	*/
	driver_pce->pce_dcb = (PTR)driver_thread;

	return driver_pce;
}


/******************************************************************************
**
** Name: GCDriverRegister
**
** Description:
**      Calls network protocol driver to do GCC_OPEN for GCregister
**
**	NOTE that caller of GCregister() assumes a synchronous return
**	in that, unless there is an error, the output values (listen_id)
**	are expected to have been set upon return.  Ie, we can't return
**	now without finishing and then expect to be able to to call the
**	gca_cl_completion later with the results.
**	Fortunately, the driver function GCC_OPEN is synchronous and
**	calls the completion routine (compl_exit) before returning.
**	For that reason, we can take a shortcut and provide a no-op
**	completion routine to the GCC driver and process the results here.
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**      asipc       pointer to association parameter structure
**      iocb        pointer to I/O control block
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**      svc_parms->listen_id
**      iocb->flags
**
** Returns:
**      status      0 success
**                  !0 failure
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
*******************************************************************************/
DWORD
GCDriverRegister(SVC_PARMS *svc_parms, ASSOC_IPC *asipc, ASYNC_IOCB *iocb)
{
    DWORD   status = 0;
    GCC_P_PLIST *gcccl_parmlist = &iocb->gcccl_parmlist;

    GCTRACE(3)("GCDriverRegister: %p Entry point.\n",
		svc_parms);

    MEfill(sizeof(GCC_P_PLIST), 0, gcccl_parmlist);
    gcccl_parmlist->function_invoked = GCC_OPEN;
    gcccl_parmlist->pce        = GCdriver_pce;
    gcccl_parmlist->compl_exit = NULL;
    gcccl_parmlist->compl_id   = NULL;
    gcccl_parmlist->function_parms.open.port_id  = GCdriver_pce->pce_port;
    status = (*GCdriver_pce->pce_protocol_rtne)(
			gcccl_parmlist->function_invoked, gcccl_parmlist);
    if(( status != OK ) || (gcccl_parmlist->generic_status != OK))
    {
	GCTRACE(1)("GCDriverRegister: Local network GCC_OPEN failed for protocol '%s' on port '%s' with status=%d/%x system_status=%d\n",
		GCdriver_pce->pce_pid,
		gcccl_parmlist->function_parms.open.port_id,
		status,
		gcccl_parmlist->generic_status,
		gcccl_parmlist->system_status.callid);
	GCerror(GC_LSN_FAIL, status ? status : gcccl_parmlist->system_status.callid) (status);
    }
    svc_parms->listen_id = gcccl_parmlist->function_parms.open.lsn_port;
    iocb->flags |= GC_IO_CREATED;
    return (status);
}


/******************************************************************************
**
** Name: GCDriverRequest
**
** Description:
**      Calls network protocol driver to do connect for GCrequest
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**        -> partner_host      target host (may be NULL)
**        -> partner_name      target port
**      iocb        pointer to I/O control block
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**
** Returns:
**	VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
*******************************************************************************/
VOID
GCDriverRequest(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb)
{
    DWORD   status = 0;
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    GCC_P_PLIST *gcccl_parmlist = &iocb->gcccl_parmlist;

    GCTRACE(3)("GCDriverRequest: %p Entry point.\n",
		svc_parms);

    MEfill(sizeof(GCC_P_PLIST), 0, gcccl_parmlist);
    gcccl_parmlist->function_invoked = GCC_CONNECT;
    gcccl_parmlist->pce        = GCdriver_pce;
    gcccl_parmlist->compl_exit = (void (*)(PTR))GCDriverRequest_exit;
    gcccl_parmlist->compl_id   = (PTR)iocb;
    if (svc_parms->partner_host == NULL)
        gcccl_parmlist->function_parms.connect.node_id = LOCALHOST;
    else
        gcccl_parmlist->function_parms.connect.node_id = svc_parms->partner_host;
    gcccl_parmlist->function_parms.connect.port_id  = svc_parms->partner_name;

    status = (*GCdriver_pce->pce_protocol_rtne)(
			gcccl_parmlist->function_invoked, gcccl_parmlist);

    if( status != OK )  /* No callback scheduled ... handle now */
	(*gcccl_parmlist->compl_exit)( gcccl_parmlist->compl_id );

    return;
}

/******************************************************************************
**
** Name: GCDriverRequest_exit	- GCC driver completion routine for GCrequest
**
** Description:
** This routine is called by GCC driver when GCC_CONNECT is completed.
** It was specified as the completion exit by GCDriverRequest when it invoked
** the protocol driver.  It is passed a pointer to an IOCB, which has
** pointers to all things relevant, such as the gcacl svc_parms as well
** as the GCC parmlist.
**
** Inputs:
**	IOCB
**
** Outputs:
**	asipc->GCdriver_pcb
**	svc_parms->size_advise  Suggested buffer size for communication
**	                        optimization.
**
** Returns:
**	VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**	16-Aug-2010 (Bruce Lunsford) Bug 124263
**	    Move saving of the iocb address in the asipc to earlier, when
**	    the iocb is first allocated.  Also remove all MEfree(iocb)'s
**	    since it will be freed later in GCrelease(). This eliminates
**	    the chance of freeing the memory twice and fixes an occasional
**	    crash in ingstart.
**
******************************************************************************/
VOID
GCDriverRequest_exit( PTR ptr )
{
    ASYNC_IOCB  *iocb      = (ASYNC_IOCB *)ptr;
    SVC_PARMS   *svc_parms = iocb->svc_parms;
    ASSOC_IPC   *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB	*gcb       = NULL;
    static int	counter	   = -1;  /* const */
    GCC_P_PLIST *gcccl_parmlist = &iocb->gcccl_parmlist;
    STATUS     status = OK;

    GCTRACE(3)("GCDriverRequest_exit: %p Entry point. gcccl_parmlist=0x%p\n",
		svc_parms, gcccl_parmlist);

    /*
    ** If connect failed, return an error.
    */
    if (gcccl_parmlist->generic_status != OK)
    {
	GCTRACE(1)("GCDriverRequest_exit: Local network GCC_CONNECT failed for protocol '%s' to node %s port '%s' with status=%x system_status=%d\n",
		    GCdriver_pce->pce_pid,
		    gcccl_parmlist->function_parms.connect.node_id,
		    gcccl_parmlist->function_parms.connect.port_id,
		    gcccl_parmlist->generic_status,
		    gcccl_parmlist->system_status.callid);
	/*
	** Map GCC CL errors to GCA CL errors
	*/
	switch(gcccl_parmlist->system_status.callid)
	{
	    case ER_alloc:
		status = GC_RQ_RESOURCE;
		break;
	    case ER_connect:
		status = GC_NO_PARTNER;
		break;
	    default:
		status = GC_RQ_FAIL;
	}
	GCerror(status, gcccl_parmlist->system_status.callid);
    }

    /*
    ** Allocate a connection block (GCA_GCB) for gcacl driver use.
    ** Then, initialize it and link to it from the asipc.
    */
    gcb = (GCA_GCB *)MEreqmem( 0, sizeof(GCA_GCB), TRUE, NULL );
    if (gcb == NULL)
    {
	GCTRACE(1) ("GCDriverRequest_exit: Unable to alloc driver connection block (gcb) - size=%d", sizeof(*gcb));
	GCerror(GC_RQ_RESOURCE, ER_alloc);
    }

    gcb->id = ( counter += 2 );
    gcb->GCdriver_pce = (PTR)GCdriver_pce;
    gcb->GCdriver_pcb = gcccl_parmlist->pcb;
    gcb->buffer_guard = GCB_BUFFER_GUARD;
    asipc->gca_gcb = gcb;	/* link to it from the asipc */

    /*
    ** Advise user of optimum size.
    **
    ** Currently based on GC_SIZE_ADVISE which is size of internal
    ** gcacl buffer for drivers (see GCB structure); this also happens
    ** to match what is done on Unix.  An alternative would be to base
    ** it on gcccl_parmlist->pce->pce_pkt_sz (minus sizeof(GC_MTYP)).
    ** Yet another option would be to make it configurable; for instance,
    ** II_GCA_PIPESIZE is used to configure size_advise for named pipes
    ** and this could also be used here, though the name is not ideal.
    ** With named pipes, the server (listen) side sets the size advise and
    ** the client (connecting) side uses the server's setting by checking
    ** an attribute of the pipe.  This capability doesn't currently exist
    ** with the drivers so just make sure both sides set the same value.
    */
    svc_parms->size_advise = GC_SIZE_ADVISE - sizeof(GC_MTYP);

    /*
    ** The following will kick off part 2 of the request (aka, connect)
    ** processing.  This is the same code used by pipes logic once a
    ** connection has been set up for an outgoing connection.
    */

    /*
    ** Send connection peer info data to server
    */
    status = GCrequestSendPeer(svc_parms, iocb);
    if (status != OK)
    {
	GCTRACE(1)("GCDriverRequest_exit: GCrequestSendPeer() failed (%x) for protocol '%s' to node %s port '%s' with status=%x system_status=%d\n",
		    status,
		    GCdriver_pce->pce_pid,
		    gcccl_parmlist->function_parms.connect.node_id,
		    gcccl_parmlist->function_parms.connect.port_id,
		    gcccl_parmlist->generic_status,
		    gcccl_parmlist->system_status.callid);
	svc_parms->status = NP_IO_FAIL;
	svc_parms->sys_err->errnum = status;
	(*svc_parms->gca_cl_completion) (svc_parms->closure);
    }

    return;
}


/******************************************************************************
**
** Name: GCDriverSendPeer
**
** Description:
**      Calls network protocol driver to do send of the CONNECT info
**	(ie, send GC peer info to partner).
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**        -> svc_buffer       pointer to data to be sent
**        -> svc_send_length  length  of data to be sent
**      connect_block   pointer to CONNECT info to be sent
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**
** Returns:
**      VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
*******************************************************************************/
VOID
GCDriverSendPeer(SVC_PARMS *svc_parms, CONNECT *connect_block)
{
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB	*gcb   = (GCA_GCB *)asipc->gca_gcb;

    GCTRACE(3)("GCDriverSendPeer %d: Entry point.\n",
		gcb->id);

    /*
    ** Hijack svc_parms from GCrequest and use for send peer info.
    */
    svc_parms->time_out = -1;   /* Don't timeout subsequent ops */
    svc_parms->flags.flow_indicator = 0;
    svc_parms->svc_buffer = (PTR)connect_block;
    svc_parms->svc_send_length = connect_block->v2.length;

    /*
    ** On Unix/Linux servers, detection of the original (Unix/Linux)
    ** peer info structure relies on the structure length (see
    ** GC_recvpeer_sm() on Unix/Linux).  If the current peer
    ** structure happens to be the same size, add one additional 
    ** (garbage) byte to avoid confusion...in case we are connecting
    ** via "direct connect" to a compatible Unix/Linux server.
    */
    if ( svc_parms->svc_send_length == sizeof( GC_PEER_ORIG ) )  
    	svc_parms->svc_send_length++;

    GCDriverSend( svc_parms );

    /*
    ** Increment connect counter in stats even though there is a
    ** (small) chance that the send could fail in the completion
    ** callback.  It's not critical that the counter be correct
    ** since it is informational only.  Incrementing it here avoids
    ** having to set up a special exit callback for GCDriverSend().
    */
    if( svc_parms->status == OK)
	GCrequest_OK++;

    return;
}


/******************************************************************************
**
** Name: GCDriverListen
**
** Description:
**      Calls network protocol driver to do listen.
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**      iocb        pointer to I/O control block
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**      iocb->flags
**
** Returns:
**      status      0 success
**                  !0 failure
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
*******************************************************************************/
DWORD
GCDriverListen(SVC_PARMS *svc_parms, ASYNC_IOCB *iocb)
{
    DWORD   status = 0;
    GCC_P_PLIST *gcccl_parmlist = &iocb->gcccl_parmlist;

    GCTRACE(3)("GCDriverListen: %p Entry point.\n", svc_parms);

    MEfill(sizeof(GCC_P_PLIST), 0, gcccl_parmlist);
    gcccl_parmlist->function_invoked = GCC_LISTEN;
    gcccl_parmlist->pce        = GCdriver_pce;
    gcccl_parmlist->compl_exit = (void (*)(PTR))GCDriverListen_exit;
    gcccl_parmlist->compl_id   = (PTR)iocb;
    gcccl_parmlist->function_parms.listen.port_id  = GCdriver_pce->pce_port;
    status = (*GCdriver_pce->pce_protocol_rtne)(
			gcccl_parmlist->function_invoked, gcccl_parmlist);

    if( status != OK )  /* No callback scheduled ... handle now */
	(*gcccl_parmlist->compl_exit)( gcccl_parmlist->compl_id );

    if ( svc_parms->time_out >= 0 &&
         ! (iocb->flags & GC_IO_TIMEOUT) )
    {
        if ( !Is_w95 )
	    GCsetWaitableTimer( svc_parms );
	iocb->flags |= GC_IO_TIMEOUT;
    }
    iocb->flags |= GC_IO_LISTEN;
    return (status);
}

/******************************************************************************
**
** Name: GCDriverListen_exit	- GCC driver completion routine for GClisten
**
** Description:
** This routine is called by GCC driver when GCC_LISTEN is completed.
** It was specified as the completion exit by GCDriverListen when it invoked
** the protocol driver.  It is passed a pointer to an IOCB, which has
** pointers to all things relevant, such as the gcacl svc_parms as well
** as the GCC parmlist.
**
** Inputs:
**	IOCB
**
** Outputs:
**	asipc->GCdriver_pcb
**
** Returns:
**	VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
******************************************************************************/
VOID
GCDriverListen_exit( PTR ptr )
{
    ASYNC_IOCB  *iocb      = (ASYNC_IOCB *)ptr;
    SVC_PARMS   *svc_parms = iocb->svc_parms;
    ASSOC_IPC   *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB	*gcb       = NULL;
    static int	counter = 0;    /* const */ /* just for id'ing */
    GCC_P_PLIST *gcccl_parmlist = &iocb->gcccl_parmlist;
    STATUS	status = OK;

    GCTRACE(3)("GCDriverListen_exit: %p Entry point. gcccl_parmlist=0x%p\n",
		svc_parms, gcccl_parmlist);

    /*
    ** If listen failed, return an error.
    */
    if (gcccl_parmlist->generic_status != OK)
    {
	GCTRACE(1)("GCDriverListen_exit: %p Local network GCC_LISTEN failed for protocol '%s' on port '%s' with status=%x system_status=%d\n",
		    svc_parms,
		    GCdriver_pce->pce_pid,
		    gcccl_parmlist->function_parms.listen.port_id,
		    gcccl_parmlist->generic_status,
		    gcccl_parmlist->system_status.callid);
	/*
	** Map GCC CL errors to GCA CL errors
	*/
	switch(gcccl_parmlist->system_status.callid)
	{
	    case ER_alloc:
	    case ER_threadcreate:
		status = GC_LSN_RESOURCE;
		break;
	    case ER_accept:
		status = GC_ASSOC_FAIL;
		break;
	    default:
		status = GC_LSN_FAIL;
	}
	GCerror(status, gcccl_parmlist->system_status.callid);
    }

    /*
    ** Set flag if client is remote.
    */
    if ( !(gcccl_parmlist->options & GCC_LOCAL_CONNECT) )
	asipc->flags |= GC_IPC_IS_REMOTE;
    /*
    ** Allocate a connection block (GCA_GCB) for gcacl driver use.
    ** Then, initialize it and link to it from the asipc.
    */
    gcb = (GCA_GCB *)MEreqmem( 0, sizeof(GCA_GCB), TRUE, NULL );
    if (gcb == NULL)
    {
	GCTRACE(1) ("GCDriverListen_exit: Unable to alloc driver connection block (gcb) - size=%d", sizeof(*gcb));
	GCerror(GC_LSN_RESOURCE, ER_alloc);
    }

    gcb->id = ( counter += 2 );
    gcb->GCdriver_pce = (PTR)GCdriver_pce;
    gcb->GCdriver_pcb = gcccl_parmlist->pcb;
    gcb->buffer_guard = GCB_BUFFER_GUARD;
    asipc->gca_gcb = gcb;	/* link to it from the asipc */

    /*
    ** The following will kick off part 2 of the listen processing.
    ** This is the same as when pipes logic detects and connects to
    ** an incoming connection.
    **
    ** For consistency with pipes logic, originally the LISTEN event
    ** was set here.  This worked for IIGCx servers using GCexec(),
    ** but not for DBMS which uses CSsuspend(); the latter assumes
    ** that LISTEN processing is complete once the event_func[LISTEN]
    ** (=GClisten2()) returns.  Since we still must do an "async" receive
    ** to get the CONNECT block (driver doesn't have a "sync/blocking"
    ** receive) and don't want to hang out here in a wait (prefer to
    ** return to caller's wait environment), we call GClisten2()
    ** directly.  The goal is to not set event[LISTEN} until we
    ** are sure we'll be done when we return; else DBMS gets confused
    ** and issues E_GCFFFF_IN_PROCESS and E_GC0024_DUP_REQUEST messages
    ** and reissues another LISTEN, even though this one is not yet
    ** complete.
    ** There may be a minor performance benefit to doing the call
    ** directly here to GClisten2 (versus setting event, wait for caller
    ** to WFMO on the event and then call it).
    */
    GClisten2();
    return;
}


/******************************************************************************
**
** Name: GCDriverListen2
**
** Description:
**      Calls network protocol driver to receive CONNECT info from remote client
**	This is for part 2 of the Listen processing.  It is called by
**	GClisten2().
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**
** Outputs:
**      svc_parms->sys_err->errnum
**      svc_parms->status
**      iocb->flags
**
** Returns:
**      VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
*******************************************************************************/
VOID
GCDriverListen2(SVC_PARMS *svc_parms)
{
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB	*gcb   = (GCA_GCB *)asipc->gca_gcb;

    GCTRACE(3)("GCDriverListen2 %d: Entry point.\n", gcb->id);

    /*
    ** Receive the GC peer info from partner.  Hijack svc_parms.
    */
    svc_parms->flags.flow_indicator = 0;
    svc_parms->reqd_amount = sizeof( asipc->connect );
    svc_parms->svc_buffer = (char *)&asipc->connect;

    gcb->ccb.save_completion = svc_parms->gca_cl_completion;
    gcb->ccb.save_closure = svc_parms->closure;
    svc_parms->gca_cl_completion = GCDriverListen2_exit;
    svc_parms->closure = (PTR)svc_parms;

    GCDriverReceive( svc_parms );

    return;
}

/******************************************************************************
**
** Name: GCDriverListen2_exit - GCC driver completion rtn for GCDriverListen2
**
** Description:
** This routine is called by GCDriverReceive's completion callback logic when
** GCC_RECEIVE is completed for the CONNECT block (peer info message).
** It was specified as the completion exit by GCDriverListen2 when it invoked
** the protocol driver.  It is passed a pointer to an IOCB, which has
** pointers to all things relevant, such as the gcacl svc_parms as well
** as the GCC parmlist.
**
** Inputs:
**      svc_parms   pointer to service parameter structure
**
** Outputs:
**	svc_parms->size_advise  Suggested buffer size for communication
**	                        optimization.
**
** Returns:
**	VOID
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
**
******************************************************************************/
VOID
GCDriverListen2_exit( PTR closure )
{
    SVC_PARMS	*svc_parms = (SVC_PARMS *)closure;
    ASSOC_IPC   *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB     *gcb       = (GCA_GCB *)asipc->gca_gcb;

    /*
    ** Restore svc_parms
    */
    svc_parms->gca_cl_completion = gcb->ccb.save_completion;
    svc_parms->closure = gcb->ccb.save_closure;

    GCTRACE(2)( "GCDriverListen2_exit %d: Entry point. status %x\n", gcb->id, svc_parms->status );

    if ( svc_parms->status != OK )
    {
	(*svc_parms->gca_cl_completion) (svc_parms->closure);
	return;
    }
    asipc->connect_recv_numxfer = svc_parms->rcv_data_length;

    /*
    ** Advise user of optimum size.
    **
    ** For more info, see discussion where this is set for driver connects
    ** (GCrequest), currently in GCDriverRequest_exit().
    */
    svc_parms->size_advise = GC_SIZE_ADVISE - sizeof(GC_MTYP);

    /*
    ** Triggers final phase of LISTEN processing in gcacl and
    ** lets caller know (if he cares) that the listen phase is
    ** complete (and gca_cl_completion called) once the event_func
    ** for LISTEN completes and returns to the caller.
    */
    SetEvent (hAsyncEvents[LISTEN]);

    return;
}

/******************************************************************************
*******************************************************************************
**
**  windows95 routines - Anonymous Pipes
**
*******************************************************************************
******************************************************************************/

/******************************************************************************
**
** Name: GClisten95
**
** Description:
**      Establishes a listen by creating the shared memory IPC area and
**      waiting for the association event.
**      Once the event is received a connect message is expected on the pipe.
**      This message is read directly into the connect structure within the
**      ASSOC_IPC structure.
**      The listen event is signaled to allow the waiting GCexec thread to
**      complete the listen.
**
** Inputs:
**      svc_parms   pointer to the service parameter structure.
**
** Outputs:
**      svc_parms->connect          updated with connect message from peer.
**      svc_parms->sys_err->errnum
**      svc_parms->status
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**	22-mar-1999 (somsa01)
**	    Removed connect_block, as what should be used is asipc->connect.
**	19-apr-1999 (somsa01)
**	    Do not create the pipes when GC_IO_CREATED is set.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
**
*******************************************************************************/
VOID
GClisten95 (SVC_PARMS * svc_parms)
{
ASSOC_IPC * asipc = (ASSOC_IPC *) svc_parms->gc_cb;
ULONG       status;
ULONG       numxfer;
HANDLE      hReady;

    GCACL_DEBUG_MACRO(( "Entering listen_1\n"));

    /* create all named pipes for the server */
    status = OK;

    if (!(ListenIOCB.flags & GC_IO_CREATED))
    {
	asipc->SndStdChan = NULL;
	asipc->RcvStdChan = NULL;
	asipc->SndExpChan = NULL;
	asipc->RcvExpChan = NULL;
	asipc->ClientReadHandle = NULL;

	if ( !CreateAsc(asipc, Asc_std_name) )
	{
	    SETWIN32ERR(svc_parms->sys_err, ~OK , ER_listen);
	    svc_parms->status = GC_LSN_FAIL;
	    goto end;
	}
	ListenIOCB.flags |= GC_IO_CREATED;
    }

    /* If we are a GCN, then we open the event named "gcn_ready" and signal
    ** the event if it exists. This tells the application that may be trying
    ** to start the gcn that we are ready for a connection.
    */
    if( is_gcn )
    {
        GCRESRC( hReady,
            hReady = OpenEvent( EVENT_ALL_ACCESS, FALSE, "gcn_ready" ) );
        if( hReady )
            SetEvent( hReady );
        is_gcn = FALSE; /* Only needs to be done for first listen. */
        GCRESRC( hReady, CloseHandle( hReady ) );
    }

    /* We loop over the connect, waiting for a "real" connect.  We can get
     * a pseudo-connect from a detect request.  This is seen as a NULL
     * pointer in the DosRead.  This will be ignored.
     */
    do {
	AscWait( asipc, GC_WAIT_FOREVER );

        status = IIReadFile(asipc, asipc->RcvStdChan,&asipc->connect,
                            sizeof(asipc->connect), &numxfer, NULL,
                            (DWORD)GC_WAIT_FOREVER );

        if( status == ~OK )
        {
            status = GetLastError( );
            svc_parms->sys_err->errnum = status;
            svc_parms->status = GC_LSN_FAIL;
            goto end;
        }

    } while(asipc->connect.v_orig.flag == CONNECT_DETECT);

    SetEvent (hAsyncEvents[LISTEN]);
    _endthreadex(0);
    return;
end:
    SETWIN32ERR(svc_parms->sys_err, status, ER_listen);
    GCerror (svc_parms->status, status);
    _endthreadex(status);
}

/******************************************************************************
**
** Name: GClisten95_2
**
** Description:
**      Second stage of connection establishment.  When GCexec receives the
**      listen event it calls this function to set-up parameters from the
**      connect message and to invoke the completion routine.
**
** Inputs:
**      None.
**
** Outputs:
**      Connect service parameters.
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**	19-apr-1999 (somsa01)
**	    Turn off GC_IO_CREATED.
**      01-Oct-1999 (fanra01)
**          Only set event if this is the right thread.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove ultoa() of pid to unused access_point_identifier area at
**	    end of asipc; access_point_identifier is obtained from received
**	    peer info (aka, connect) message.  Qualify connect fields with
**	    v_orig since this is only format currently supported by W95 logic.
**
*******************************************************************************/
VOID
GClisten95_2 ()
{
ASYNC_IOCB *iocb      = &ListenIOCB;
SVC_PARMS  *svc_parms = iocb->svc_parms;
ASSOC_IPC  *asipc     = (ASSOC_IPC *)svc_parms->gc_cb;
STATUS     status     = OK;
char       szUserName[127];
char       *pszUserName = szUserName;
SECURITY_ATTRIBUTES sa;


    GCTRACE(3)("GClisten2: Entry Point\n");

    if (ListenIOCB.lstnthread == GetCurrentThreadId())
    {
        /*
        **  Listen threads context
        */
        ResetEvent( ListenIOCB.Overlapped.hEvent );
        GCTRACE(3)("GClisten2: Listening thread continue\n");
    }
    else
    {
        /*
        ** Woke up in another threads GCsync. GCsync will re-issue the
        ** wait.
        ** Leave the event set so that the listening thread will also
        ** get woken up.
        **
        */
        GCTRACE(3)("GClisten2: Not listening thread\n");
        return;
    }

    iimksec (&sa);

    /*
    ** Check to see if the Server pipe is NULL.  If it is, we're shutting
    ** down so just return back to GCexec().
    ** Also, comm server can call here after a listen failure.
    */

    if ( asipc == NULL || asipc->RcvStdChan == NULL )
        return;

    iocb->flags &= ~GC_IO_CREATED;

    /*
    ** Bug 79293
    ** Create a named semaphore object for synchronising purge messages.
    */
    if ( GCcreateSemaphore(asipc) == 0 )
    {
        GCTRACE(1)("GCcreateSemaphore: Failed to create purge semaphore\n");
        GCerror(GC_ASSOC_FAIL, GetLastError());
    }
    /*
    ** Advise user of optimum size *
    */

    svc_parms->size_advise = Pipe_buffer_size;

    /*
    ** Fill in the trusted flag and sec_label
    */

    svc_parms->sec_label               = (char *)&asipc->sec_label;
    svc_parms->user_name               = asipc->connect.v_orig.userid;
    svc_parms->account_name            = asipc->connect.v_orig.account;
    svc_parms->access_point_identifier = asipc->connect.v_orig.access_point;

    GCusername(pszUserName, sizeof(szUserName));
    svc_parms->flags.trusted = !STcompare(svc_parms->user_name, pszUserName);

    /*
    ** Call completion routine
    */

    GCerror(OK, 0);
}

/******************************************************************************
**
** Name: GCrequest95
**
** Description:
**      Process the request for connection.  Establishes that there is a
**      partner to connect to by opening the shared memory association area.
**      Notifies the partner that a connection is requested by posting the
**      request event.
**      The connect message is then written to the pipe and the completion is
**      called.
**      This function is processed synchronously.
**
** Inputs:
**      svc_parms       pointer to the service parameter structure
**
** Outputs:
**      svc_parms->size_advise
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**          Renamed from request_w95.  Modified to use GCerror which serializes
**          completion.
**      04-Sep-97 (fanra01)
**          Changed IIWriteFile conditional to match modified return type.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Change connect_block from CONNECT to CONNECT_orig structure
**	    since this is only format currently supported by W95 logic.
**
*******************************************************************************/
VOID
GCrequest95(SVC_PARMS *svc_parms)
{

    ULONG       status, numread;
    i4          inbuf_size, outbuf_size, timeout;
    ASSOC_IPC * asipc = (ASSOC_IPC *) svc_parms->gc_cb;
    char        name[MAXPNAME];
    static
    CONNECT_orig     connect_block;
    char        tmp_id[127];
    char        *tmp_id_ptr;

    asipc->SndStdChan = NULL;
    asipc->RcvStdChan = NULL;
    asipc->SndExpChan = NULL;
    asipc->RcvExpChan = NULL;
    asipc->ClientReadHandle = NULL;

    status = OK;

    /* Use the given partner name to get to "listen" pipe */
    /* changed to use null string LISTEN_PFX */
    STprintf(name, "%s-%s", "", svc_parms->partner_name);

    switch (svc_parms->time_out)
    {
        case 0:
            timeout = GC_WAIT_DEFAULT;
            break;
        case -1:
            timeout = GC_WAIT_FOREVER;
            break;
        default:
            timeout = svc_parms->time_out;
            break;
    }


    if ( status = OpenAsc(asipc, name, (DWORD)timeout) )
    {
        svc_parms->status = ( status == ~OK ) ? GC_NO_PARTNER : GC_TIME_OUT;
        goto exit;
    }

    /* Tell the listener that we are not just a detect */
    connect_block.flag = CONNECT_NORMAL;
    connect_block.pid = Conn_id;

    tmp_id_ptr = tmp_id;
    GCusername(tmp_id_ptr, sizeof(tmp_id));
    IDname(&tmp_id_ptr);

    STcopy(tmp_id_ptr, connect_block.userid);
    STcopy(tmp_id_ptr, connect_block.account);
    STcopy("console", connect_block.access_point);

    if (IIWriteFile(asipc, asipc->SndStdChan, &connect_block,
                sizeof(connect_block), &numread, NULL) != OK)
    {
        CloseAsc(asipc);
        svc_parms->status = GC_NO_PARTNER;
        goto exit;
    }

    GetAscInfo(asipc, &outbuf_size, &inbuf_size );

    svc_parms->size_advise = inbuf_size; /* Advise user of optimum size */

exit:
    SETWIN32ERR(svc_parms->sys_err, status, ER_write);
    GCerror (svc_parms->status, status);

}

/******************************************************************************
**
** Name: GCCompletion95
**
** Description:
**      Wrapper function for the GCcompletionRoutine.
**      User APCs can take only one DWORD parameter so the information must be
**      saved into a structure.  This assumes that a DWORD matches a pointer
**      in size.
**
**      This function will be called in the context of the thread identified
**      by the pIoparms->hCmpltThread parameter.
**      This should be the thread which invoked the I/O function and which is
**      also in an alertable wait state.
**
** Inputs:
**      pIoparms    I/O parameters which are required for asynchronous
**                  completion.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**
*******************************************************************************/
VOID APIENTRY
GCCompletion95 (PIOPARMS pIoparms)
{
    pIoparms->completion( pIoparms->status, pIoparms->dwNumberXfer,
        &pIoparms->pIocb->Overlapped );
    GCRESRC( pIoparms->hCmpltThread, CloseHandle (pIoparms->hCmpltThread) );
    MEfree((PTR)pIoparms);
    return;
}

/******************************************************************************
**
** Name: GCreceive95
**
** Description:
**      Thread wrapper for the blocking pipe read so that control is
**      reliquished during I/O.  When the read completes the pIoparms is
**      updated and the GCCompletion95 function is queued for completion in
**      the context of the invoking thread.
**
** Inputs:
**      pIoparms        pointer to I/O parameter structure
**
** Outputs:
**      pIoparms->status        status of read
**      pIoparms->dwNumberXfer  number of bytes read in the message
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**      01-Oct-1999 (fanra01)
**          Add increment of asyncIoPending counter.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
**
*******************************************************************************/
VOID
GCreceive95 (PIOPARMS pIoparms)
{
ULONG       status;
ULONG       amount_read = 0;

    status = IIReadFile ( pIoparms->pAsipc,
        pIoparms->hFd,
        pIoparms->pSvcParms->svc_buffer,
        pIoparms->pSvcParms->reqd_amount,
        &amount_read,
        NULL,
        pIoparms->pSvcParms->time_out);
    pIoparms->status = status;
    pIoparms->dwNumberXfer = amount_read;
    pfnQueueUserAPC ((PAPCFUNC)GCCompletion95, pIoparms->hCmpltThread,
                     (DWORD)pIoparms);
    _endthreadex(0);
    return;
}

/******************************************************************************
**
** Name: GCAnonPipeReceive
**
** Description:
**      Attempt to mimic asynchronous I/O completion in windows 95.
**      Modified the read to use user apc's to execute completion routine in
**      the context of the invoking thread.
**
**      Duplicate the current thread handle giving all access rights.
**      THREAD_SET_CONTEXT is really the only one required.
**
**      Allocate an I/O parameter structure which is used to pass values to
**      the completion routine.
**
**      Create the read thread and return.
**
**      For synchronous reads perform the read and execute gca complete.
**
**      On error the gca completion is executed.
**
** Inputs:
**      fd          handle to the write pipe
**      svc_parms   pointer to service parameter structure
**      asipc       pointer to association parameter structure
**      iocb        pointer to I/O control block
**      expeditied  flag indicates read channel
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created - based on the windows NT implementation.
**      03-Dec-1999 (fanra01)
**          Add completion routine function pointer as argument.
**          Create the receive thread suspended and resume it after
**          the I/O count has been updated.
**      27-sep-2001 (lunbr01) Bug 105902
**          Add call to GCreceiveCompletion() to check for GC purge 
**          message (+misc) in sync receive logic--
**          previously only done for async in GCCompletionRoutine.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Remove MEfree(iocb)'s since the send/recv iocbs are now
**	    preallocated and not freed until disconnect.
**
*******************************************************************************/
VOID
GCAnonPipeReceive( HANDLE fd, SVC_PARMS *svc_parms, ASSOC_IPC *asipc,
    ASYNC_IOCB *iocb, BOOL expedited,
    VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) )
{
    bool        sync_flag = svc_parms->flags.run_sync;
    HANDLE      hProc   = GetCurrentProcess();
    HANDLE      hThread;
    HANDLE      hNewThread;
    PIOPARMS    pIoparms;
    DWORD       status = 0;
    DWORD       dwTid;
    DWORD       amount_read = 0;

    if (svc_parms->time_out == -1 || !sync_flag)
    {
        if (DuplicateHandle (hProc, GetCurrentThread(), hProc, &hNewThread,
                             THREAD_ALL_ACCESS,
                             TRUE,
                             0) == TRUE)
        {
            if ((pIoparms = (PIOPARMS) MEreqmem (0, sizeof(IOPARMS), TRUE,
                                                 &status)) != NULL)
            {
                pIoparms->pSvcParms     = svc_parms;
                pIoparms->pAsipc        = asipc;
                pIoparms->hFd           = fd;
                pIoparms->status        = GC_ASSOC_FAIL;
                pIoparms->hCmpltThread  = hNewThread;
                pIoparms->dwNumberXfer  = 0;
                pIoparms->pIocb         = iocb;
                pIoparms->completion    = completion;
                /*
                ** Create the thread suspended so that the I/O count be
                ** updated before the thread runs.
                */
                GCRESRC( hThread,
                    hThread = (HANDLE)_beginthreadex(NULL, GC_STACK_SIZE,
                        (LPTHREAD_START_ROUTINE)GCreceive95,
                        pIoparms, CREATE_SUSPENDED, &dwTid) );
                if (hThread != NULL)
                {
                    if (asipc->flags & GC_IPC_DISCONN)
                    {
                        TerminateThread( hThread, 0 );
                        svc_parms->status = GC_ASSOC_FAIL;
                        MEfree( (PTR)pIoparms );
                        GCRESRC( hThread, CloseHandle( hThread ) );
                        GCerror( svc_parms->status, ERROR_BROKEN_PIPE );
                    }
                    else
                    {
                        InterlockedIncrement( &asipc->asyncIoPending );
                        /*
                        ** Allow the thread to run
                        */
                        ResumeThread( hThread );
                    }
                    GCRESRC( hThread, CloseHandle( hThread ) );
                }
                else
                {
                    svc_parms->status = GC_ASSOC_FAIL;
                    status = errno;
                    MEfree((PTR)pIoparms);
                    GCerror(svc_parms->status, status);
                }
            }
            else
            {
                TRdisplay ("GC MEreqmem failed error %d\n", status);
            }
        }
        else
        {
            TRdisplay ("GC DuplicateHandle failed error %d\n", GetLastError());
        }
    }
    else
    {
        status = IIReadFile ( asipc,
                              fd,
                              svc_parms->svc_buffer,
                              svc_parms->reqd_amount,
                              &amount_read,
                              NULL,
                              svc_parms->time_out);
	GCreceiveCompletion(svc_parms, &status, amount_read);
        ReleaseSemaphore (SyncEvent, 1, NULL);
        GCerror(status, 0);
    }
    return;
}

/******************************************************************************
**
** Name: GCsend95
**
** Description:
**      Thread wrapper for the blocking pipe write so that control is
**      reliquished during I/O.  When the write completes the pIoparms is
**      updated and the GCCompletion95 function is queued for completion in
**      the context of the invoking thread.
**
** Inputs:
**      pIoparms        pointer to I/O parameter structure
**
** Outputs:
**      pIoparms->status        status of write
**      pIoparms->dwNumberXfer  number of bytes written
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created.
**      01-Oct-1999 (fanra01)
**          Add increment of asyncIoPending counter.
**      03-Dec-1999 (fanra01)
**          If the disconnect flag is set send a zero length message.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Move increment of asyncIoPending (on success) up to caller of
**	    send routines for more consistency across various types of sends.
**
*******************************************************************************/
VOID
GCsend95 (PIOPARMS pIoparms)
{
    ULONG       status;
    ULONG       amount_written = 0;
    PTR         sndbuf = pIoparms->pSvcParms->svc_buffer;
    DWORD       sndlen = pIoparms->pSvcParms->svc_send_length;

    InterlockedIncrement( &pIoparms->pAsipc->asyncIoPending );

    status = IIWriteFile( pIoparms->pAsipc,
        pIoparms->hFd,
        sndbuf,
        sndlen,
        &amount_written,
        NULL);
    pIoparms->status = status;
    pIoparms->dwNumberXfer = amount_written;
    pfnQueueUserAPC ((PAPCFUNC)GCCompletion95, pIoparms->hCmpltThread,
                     (DWORD)pIoparms);
    _endthreadex(0);
    return;
}

/******************************************************************************
**
** Name: GCAnonPipeSend
**
** Description:
**      Attempt to mimic asynchronous I/O completion in windows 95.
**      Modified the write to use user apc's to execute completion routine in
**      the context of the invoking thread.
**
**      Duplicate the current thread handle giving all access rights.
**      THREAD_SET_CONTEXT is really the only one required.
**
**      Allocate an I/O parameter structure which is used to pass values to
**      the completion routine.
**
**      Create the write thread and return.
**
**      All writes are asynchronous.
**
**      On error the gca completion is executed.
**
** Inputs:
**      fd          handle to the write pipe
**      svc_parms   pointer to service parameter structure
**      asipc       pointer to association parameter structure
**      iocb        pointer to I/O control block
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      25-Jul-97 (fanra01)
**	    Created - based on the windows NT implementation.
**      03-Dec-1999 (fanra01)
**          Add completion routine function pointer as argument.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    GCwaitCompletion + GCrestart.  This should improve peformance.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Move and MEfree of iocb (on failure) up to caller for more
**	    consistency across various types of sends. Requires status
**	    to be set correctly upon return.
**
*******************************************************************************/
DWORD
GCAnonPipeSend( HANDLE fd, SVC_PARMS *svc_parms, ASSOC_IPC *asipc,
    ASYNC_IOCB *iocb, VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) )
{
HANDLE      hProc   = GetCurrentProcess();
HANDLE      hThread;
HANDLE      hNewThread;
PIOPARMS    pIoparms;
DWORD       status = 0;
DWORD       dwTid = 0;

    if (DuplicateHandle (hProc, GetCurrentThread(), hProc, &hNewThread,
                         THREAD_ALL_ACCESS,
                         TRUE,
                         0) == TRUE)
    {
        if ((pIoparms = (PIOPARMS) MEreqmem (0, sizeof(IOPARMS), TRUE,
                                             &status)) != NULL)
        {
            pIoparms->pSvcParms     = svc_parms;
            pIoparms->pAsipc        = asipc;
            pIoparms->hFd           = fd;
            pIoparms->status        = GC_ASSOC_FAIL;
            pIoparms->hCmpltThread  = hNewThread;
            pIoparms->dwNumberXfer  = 0;
            pIoparms->pIocb         = iocb;
            pIoparms->completion    = completion;
            GCRESRC( hThread,
                hThread = (HANDLE)_beginthreadex(NULL, GC_STACK_SIZE,
                                   (LPTHREAD_START_ROUTINE)GCsend95,
                                   pIoparms, 0, &dwTid) );
            if (hThread!= NULL)
            {
                status = ERROR_IO_PENDING;
                GCRESRC( hThread, CloseHandle (hThread) );
            }
            else
            {
                svc_parms->status = GC_ASSOC_FAIL;
                status = errno;
                SETWIN32ERR(svc_parms->sys_err, status, ER_system);
                svc_parms->sys_err->errnum = status;
                (*svc_parms->gca_cl_completion) (svc_parms->closure);
            }
        }
        else
        {
            TRdisplay ("GC MEreqmem failed error %d\n", status);
        }
    }
    else
    {
	status = GetLastError();
        TRdisplay ("GC DuplicateHandle failed error %d\n", status);
    }
    return (status);
}

/******************************************************************************
*******************************************************************************
**
**  DEBUG routines
**
*******************************************************************************
******************************************************************************/

# ifdef xDEBUG

void GCtrace(char* fmt, ...);

/*
** A thread that wakes up every 10 seconds.
** Useful in clients which have hung and the debugger cannot break in
** while all threads are in system calls.
*/
static HANDLE hwakeupthread = NULL;

DWORD WINAPI
wakeup(LPVOID parm)
{
    DWORD   status = 0;
    HANDLE  hwakeup = NULL;

    GCRESRC( hwakeup, hwakeup = CreateEvent( NULL, TRUE, FALSE, NULL) );

    for (;hwakeup != NULL;)
    {
        WaitForSingleObject(hwakeup, 10000);
    }

    GCRESRC( hwakeup, CloseHandle(hwakeup) );
    _endthreadex(0);
    return(status);
}


static LARGE_INTEGER freq;
static int bFirst = TRUE;
static unsigned int nMHz = 0;

/******************************************************************************
**
** Name: GCGetCyclecount
**
** Description:
**      Execute instructions to return the cycle count from the processor.
**      This count wraps and is reset when the machine is restarted.
**
** Input:
**      nstart  reference time.
**
** Output:
**      Cycle count from processor register.
**
** Returns:
**      Number of cycles since nStart.
**
** History:
**      18-Oct-2002 (fanra01)
**          Created.
**
*******************************************************************************/
unsigned __int64
GCGetCyclecount(unsigned __int64 nStart)
{
    __int64 now;

    _asm
    {
        _emit 0Fh
        _emit 0a2h
        _emit 0Fh
        _emit 031h
        mov dword ptr now, eax
        mov dword ptr now+4, edx
    }

    return (now - nStart);
}

/******************************************************************************
**
** Name: GCGetCpuMHz
**
** Description:
**      Reads the registry key to get the CPU frequency
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      nMHz    CPU clock frequency
**
** History:
**      18-Oct-2002 (fanra01)
**          Created.
**
*******************************************************************************/
unsigned int
GCGetCpuMHz()
{
    if( bFirst )
    {    /* run this only once. */
        HKEY hKey = 0;

        bFirst = FALSE;

        if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
            KEY_QUERY_VALUE, &hKey) == 0 )
        {
            DWORD dwType = -1;
            unsigned long cbValue = sizeof(nMHz);
            RegQueryValueEx(hKey, "~MHz", 0, &dwType, (unsigned char*)&nMHz,
                &cbValue);

            RegCloseKey(hKey);
        }

        if( nMHz < 1 )
            nMHz = 1;
    }

    return nMHz;
}

/******************************************************************************
**
** Name: GCtimestamp
**
** Description:
**      Gets a marker of the number of CPU cycles that have passed.
**
** Input:
**      now         pointer to 64bit structure for large integers.
**
** Output:
**      .QuadPart   number of CPU cycles that have passed.
**
** Returns:
**      number of CPU cycles that have passed.
**      There is no error return.
**
** History:
**      18-Oct-2002 (fanra01)
**          Created.
**
*******************************************************************************/
static longlong
GCtimestamp( LARGE_INTEGER* now )
{
    if (nMHz == 0)
    {
        int cpuclock = 0;

        cpuclock = GCGetCpuMHz();
        freq.QuadPart = Int32x32To64(cpuclock,1000000);
    }
    now->QuadPart = GCGetCyclecount(0);
    return((longlong)(now->QuadPart));
}

/******************************************************************************
**
** Name: GCtrace
**
** Description:
**      Trace function adds thread id and tick counter into the trace message.
**      Tick count is the machine tick count and provides a rough method
**      of correlating events between trace files.
**
** Inputs:
**      fmt     format string for the trace message
**      ...     variables
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      03-Dec-1999 (fanra01)
**          Created.
**
*******************************************************************************/
void
GCtrace( char* fmt, ... )
{
    char    lbuf[256];
    char    mbuf[512];
    LARGE_INTEGER now;
    
    va_list p;

    va_start( p, fmt );
    GCtimestamp( &now );
    sprintf(lbuf, "%08d %I64x %s", GetCurrentThreadId(), now.QuadPart, fmt);
    vsprintf( mbuf, lbuf, p );
    TRdisplay( mbuf );
    return;
}
# endif  /* xDEBUG */

/*
** Copyright (c) 1987, 2001 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <qu.h>
#include    <cs.h>
#include    <ex.h>
#include    <me.h>
#include    <mu.h>
#include    <tr.h>
#include    <gca.h>
#include    <gc.h>
#include    <gcaint.h>
#include    <gcr.h>
#include    <gcatrace.h>

/**
**
**  Name: gcacall.c
**
**  Description:
**      GCA entry points.  Contains the following functions:
**
**          IIGCa_call		Original entry point for GCA.
**	    IIGCa_cb_call()	Primary entry point for GCA.
**	    gca_resume()	Callback function for GC.
**
**	Callers of GCA originally called IIGCa_call() to request
**	GCA services.  Control Blocks were added to GCA so that
**	multiple GCA users in the same process could perform GCA
**	requests without interference.  A new entry point was
**	added, IIGCa_cb_call(), for the control block interface.  
**
**	The original entry point, IIGCa_call(), is still valid 
**	and provides a default control block so that existing 
**	GCA applications do not need to be changed.
**
**	New GCA applications which need to co-exist with other
**	GCA users in the same application should use IIGCa_cb_call().
**	
**  History:
**      13-apr-87 (jbowers)
**          Initial module creation
**      07-Aug-87 (jbowers)
**          Add enqueing of svc_parms and semaphore protection.
**      07-Aug-87 (jbowers)
**          Add GCA_DISASSOC service.
**      24-Aug-87 (jbowers)
**          changed external name to IIGCa_call
**      22-Dec-87 (jbowers)
**          Added "selective" svc_parms parm list passing: gca_format and
**          gca_interpret are passed parm lists directly, for performance
**          optimization.
**      10-Feb-89 (jbowers)
**          Added GCA_DCONN service to support comm server.
**      25-Mar-89 (jbowers)
**          Changed initialization of svc_parms: clear or initialize only
**	    selected elements instead of zero_filling the whole structure.
**      27-Mar-89 (jbowers)
**          Added proper casting to parm list passed to each service routine.
**      10-Apr-89 (jbowers)
**          Added GCA_ATTRIBS function request: return atributes of the
**	    specified association.
**	15-May-89 (GordonW)
**	    check error return from gca_alloc rouitne.
**	07-jun-89 (pasker)
**	    added tracing to IIGCA_CALL
**      11-Apr-89 (jbowers)
**          Added retrieval of alternate completion exit indication from call
**	    parm "indicators".
**	29-Jul-89 (seiwald)
**	    Zero out connect parameter in svc_parms for GCrequest.
**	    Check new GCA_ALT_EXIT flag from gc.h and set 
**	    svc_parms->flags.alt_exit appropriately.
**	1-Aug-89 (seiwald)
**	    New case for GCA_RQRESP.
**	21-Sep-89 (seiwald)
**	    Reworked to find or add the ACB for each service.  Uses a
**	    dispatch table, indexed by the GCA service code, to determine 
**	    how to handle the ACB and SVC_PARMS, and what function to call.
**	    Code for allocating SVC_PARMS moved to a separate function.
**	27-Sep-89 (seiwald)
**	    Added support for GCA_RESUME indicator for resuming a service
**	    which returned E_GCFFFE_INCOMPLETE.
**	25-Oct-89 (seiwald)
**	    Timeout to IIGCa_call is still in seconds, but time_out to
**	    GCA CL calls is in milliseconds.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	26-Oct-89 (seiwald)
**	    Check parameter_list for NULL.  gca_svcvalid() is dead.
**	26-Oct-89 (seiwald)
**	    Changed calls to gca_qenq, etc. to use QUinsert, etc.
**	10-Nov-89 (seiwald)
**	    Zero out new state flag in newly alloced SVC_PARMS.
**	    This flag is for multistate GCA operations.
**	10-Nov-89 (seiwald)
**	    Moved in gca_resume(), gca_stub() from gcarqst.c.
**	10-Nov-89 (seiwald)
**	    GCA_DCONN is dead.
**	10-Nov-89 (seiwald)
**	    Find SVC_PARMS stashed in ACB, rather than allocating
**	    them from the queue.
**	10-Nov-89 (seiwald)
**	    Reinstitute some checking for GCA_RESUME lobotomized
**	    in SVC_PARMS rearrangement.
**	15-Nov-89 (seiwald)
**	    New GCA_DEBUG_MACRO tracing.
**	16-Nov-89 (seiwald)
**	    Zero out sync_svc_parms when used.  The sys_err field,
**	    in particular, should be emptied.
**	12-Dec-89 (seiwald)
**	    Initialized svc_parms->flags.flow_indicator so that GCA_SEND and
**	    GCA_RECEIVE needn't.
**	12-Dec-89 (seiwald)
**	    Removed antiquated gca_listen_assoc.  I believe this was to cache
**	    the listen ACB so that it could be reused without being release 
**	    with GCA_DISASSOC.  This could never work, since only by calling 
**	    GCA_DISASSOC do you allow the CL to recover its control block.  
**	    Anyway, the GCA interface demands that GCA_DISASSOC be called 
**	    even after a failed listen, so this cached listen ACB would never 
**	    be available.
**	12-Dec-89 (seiwald)
**	    Check for duplicate request so that GCA_SEND and GCA_RECEIVE 
**	    needn't.
**	30-Dec-89 (seiwald)
**	    Async only GCACL: set sync_indicator to 0 (always).  Drive
**	    sync operations to completion after posting operation request.
**	    Protect code below IIGCa_call from interrupts using EX_OFF
**	    and EX_ON.  On interrupt, operations are driven to completion,
**	    not aborted.  Use gca_syncing counter to track number of 
**	    outstanding sync operations (1 normally, 2 on interrupt).
**	    Use nsync_indicator to tell gca_complete whether to drive
**	    client completion routine.
**	31-Dec-89 (seiwald)
**	    Moved API override of GCA_SYNC_FLAG for GCA_REQUEST and
**	    GCA_DISASSOC into IIGCa_call so we can set nsync_indicator
**	    appropriately.  Use nsync_indicator to shortcut gca_resume.
**	01-Jan-90 (seiwald)
**	    Initialize subchan to GCA_NORMAL for operations such as 
** 	    GCA_REQUEST and GCA_LISTEN.
**	28-Jan-90 (seiwald)
**	    Aesthetic tracing changes.
**	31-Jan-90 (seiwald)
**	    Removed now unused pieces of SVC_PARMS.
**	16-Feb-90 (seiwald)
**	    Renamed nsync_indicator to sync_indicator, and removed old
**	    references to sync_indicator.  The CL must now be async.
**	26-Feb-90 (seiwald)
**	    Consolidated error numbers: svc_parms->sys_err now points to 
**	    parmlist gca_os_status, and svc_parms->statusp points to 
**	    parmlist gca_status.
**	26-Feb-90 (seiwald)
**	    Removed gca_stub (old sync support).
**	06-Mar-90 (seiwald)
**	    Sorted out running sync vs. driving completion exits.
**	    The SVC_PARMS flag "run_sync" indicates that the operation
**	    must be driven to completion, and "drive_exit" indicates
**	    that gca_complete should drive the completion routine.
**	27-Apr-90 (jorge)
**	    Use EX_ON, EX_OFF on VMS until EX_DELIVER is supported.
**	01-May-90 (jorge)
**	    Map positive time_out's to -1 for GCA_API_LEVEL_0-1.
**	06-Jun-90 (seiwald)
**	    Made gca_alloc() return the pointer to allocated memory.
**	17-Jun-90 (seiwald)
**	    Use handy pointers set up for IIGCa_call in gca_resume.
**	30-Jun-90 (seiwald)
**	    Don't set svc_parms->flags.in_use until protected from
**	    interrupts: this commits the service from GCA's perspective
**	    and must be atomic with invoking the service.
**	27-Oct-90 (seiwald)
**	    Map async time_out's to -1 for GCA_API_LEVEL_0-1.
**	2-Jan-91 (seiwald)
**	    GCsync() now calls EXinterrupt( EX_DELIVER ).
**	4-Jan-91 (seiwald)
**	    Reverse time_out overriding in GCA_API_LEVEL_0-1: now they
**	    want it.
**	29-Jan-91 (seiwald)
**	    Shuffled GCA tracing levels.
**	    	0: none
**	    	1: GCA_REQUEST calls
**	    	2: all calls & completions
**	    	3: states for REQUEST, LISTEN, DISASSOC, syncing
**	    	4: states for SEND, RECEIVE
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	08-Jul-92 (brucek)
**	    Support for GCA_FASTSELECT service (new with GCA_API_LEVEL_4).
**	09-Sep-92 (brucek)
**	    Support alternate completion exit for GCA_FASTSELECT.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	11-Jan-93 (brucek)
**	    Support for GCA_REG_TRAP and GCA_UNREG_TRAP services (API_LEVEL 4).
**	14-Jan-93 (edg)
**	    Deleted reference to svc_parms->connect.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-Sep-93 (brucek)
**	    Cast async_id to i4 before stuffing into svc_parms.
**	13-Oct-93 (seiwald)
**	    Introduced regular parameter placement into GCA_PARMLIST
**	    so that GCA code doesn't have to hunt for status, syserr,
**	    etc.
**	14-Mar-94 (seiwald)
**	    All async services now provided by gca_service().
**	21-Apr-94 (seiwald)
**	    GCA_REGISTER now through gca_service().
**	    New completion entry in gca_dispatch table.
**	22-Apr-94 (seiwald)
**	    Moved API checking into IIGCa_call and out of gca_resume().
**	16-May-94 (seiwald)
**	    All operations now through gca_service().
**      12-jun-1995 (chech02)
**          Added semaphores to protect critical sections (MCT).
**	21-aug-1995 (canor01)
**	    move MCT semaphores for performance
**	 3-Nov-95 (gordy)
**	    Removed MCT semaphores.  IIGCa_static is allocated once
**	    when GCA_INITIATE called, so does not require semaphore.
**	20-Nov-95 (gordy)
**	    Added prototypes.  Free IIGCa_static when no longer used.
**	21-Nov-95 (gordy)
**	    Added gcr.h for embedded Comm Server configuration.
**	22-Dec-95 (gordy)
**	    Replace MEfree() call with gca_free().
**	12-Jan-96 (gordy)
**	    Adding cast to previous fix to quiet compiler.
**	 3-Sep-96 (gordy)
**	    Rename IIGCa_call() to IIGCa_cb_call() and added control block
**	    parameter.  IIGCa_call() now a cover for IIGCa_cb_call() which
**	    provides a default control block for the original routine.
**	14-Nov-96 (gordy)
**	    GCsync() now takes timeout value.
**	10-Dec-96 (gordy)
**	    The time-out value to IIGCa_cb_call() is now in milli-seconds,
**	    original interface, IIGCa_call(), remains in seconds.
**	16-dec-1996 (donc)
**	   Add OpenROAD wait cursor flag support.
**	16-jul-99 (sarjo01)
**	    Bug 97562: made gca_syncing flag part of ACB to avoid session
**	    collisions.
**      03-jun-1999 (horda03)
**          Set flag to indicate when a channel is already in use. This will
**          disllow any further Send (for the same channel) until the
**          outstanding send operation has completed. This will stop a SEGV
**          within the DBMS server if IPM kills a session that has outstanding
**          messages. Bug 87612. 
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	28-May-97 (thoda04)
**	    Fix to GCA_DEBUG_MACRO call to cast arguments correctly for WIN16.
**	18-Dec-97 (gordy)
**	    Sync request counter made local and reference placed in
**	    ACB to support multi-threaded sync requests.
**	 4-Jun-98 (gordy)
**	    Save timeout value so server connection can be timed-out
**	    even if Name Server connection is successful.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-may-2001 (somsa01)
**	    Changed "svc_parms->flags.in_use" to "svc_parms->in_use".
**	15-Oct-03 (gordy)
**	    Added ACB initialization for REGISTER & FASTSELECT which
**	    marks the ACB for deletion at the end of the request.
**	    FASTSELECT may now be resumed.
**	 1-Mar-06 (gordy)
**	    The NS registration connection established in GCA_REGISTER
**	    is now kept active until GCA is terminated.  The ACB and
**	    registration parms for this connection stored in the
**	    GCA_CB.  To allow operations on this internal connection,
**	    a CALL/CALLRET capability has been added to the GCA state
**	    machine and an alternate call parms added to the SVC_PARMS.
**	    A check is made during a GCA_RESUME request to see if a CALL
**	    operation is active, in which case the alternate SVC_PARMS
**	    is activated.
**	 8-Jun-06 (gordy)
**	    GCA_TERMINATE requires synchronization to close registration
**	    connection.
**/



/*
** Name: gca_dispatch - dispatch table for GCA services
**
** Description:
**	This table tells IIGCa_call what function to call for each
**	service code, and what resources the service needs: a new
**	or an old ACB, no ACB, GCA_SVC_PARMS, or no GCA_SVC_PARMS.
**
** History:
**	21-Sep-89 (seiwald)
**	    Created.
**	15-Oct-03 (gordy)
**	    Register and Fastselect use the ACB strictly for the
**	    duration of the request.
**	 8-Jun-06 (gordy)
**	    Added sync type.  GCA_TERMINATE now uses registration ACB.
*/

# define RCV_SVC	1	/* use read GCA_SVC_PARMS */
# define SND_SVC	2	/* use send GCA_SVC_PARMS */
# define CON_SVC	3	/* use connect GCA_SVC_PARMS */
# define SYNC_SVC	4	/* use locally allocated */

# define NO_ACB		0	/* no acb */
# define NEW_ACB	1	/* alloc a new acb */
# define OWN_ACB	2	/* alloc new acb, free when complete */
# define REG_ACB	3	/* alloc or use registration acb */
# define OLD_ACB	4	/* find old acb */

# define MAY_SYNC	0	/* Request sync/async */
# define IS_SYNC	1	/* Inherently synchronous */
# define DO_SYNC	2	/* Synchronize */
# define NO_SYNC	3	/* Don't syncronize */

static struct gca_dispatch 
{
    bool	complete;	/* does it call completions? */
    u_i1	sync_type;	/* Sync/Async */
    u_i1	acb_type;	/* where to get the ACB */
    u_i1	svc_type;	/* where to get the GCA_SVC_PARMS */ 
    char	*name;		/* service name for tracing */
} gca_dispatch[] = 
{
    FALSE, MAY_SYNC, 0,       0,        "", 
    TRUE,  IS_SYNC,  NO_ACB,  SYNC_SVC, "INITIATE",
    TRUE,  DO_SYNC,  REG_ACB, SYNC_SVC, "TERMINATE",
    TRUE,  MAY_SYNC, NEW_ACB, CON_SVC,  "REQUEST",
    TRUE,  MAY_SYNC, NEW_ACB, CON_SVC,  "LISTEN",
    TRUE,  MAY_SYNC, OLD_ACB, CON_SVC,  "SAVE",
    TRUE,  IS_SYNC,  NO_ACB,  SYNC_SVC, "RESTORE",
    FALSE, IS_SYNC,  NO_ACB,  SYNC_SVC, "FORMAT",
    TRUE,  MAY_SYNC, OLD_ACB, SND_SVC,  "SEND",
    TRUE,  MAY_SYNC, OLD_ACB, RCV_SVC,  "RECEIVE",
    FALSE, IS_SYNC,  NO_ACB,  SYNC_SVC, "INTERPRET",
    TRUE,  MAY_SYNC, REG_ACB, CON_SVC,  "REGISTER",
    TRUE,  MAY_SYNC, OLD_ACB, CON_SVC,  "DISASSOC",
    FALSE, MAY_SYNC, 0,       0,        "",	
    TRUE,  MAY_SYNC, OLD_ACB, SYNC_SVC, "ATTRIBS",
    TRUE,  MAY_SYNC, OLD_ACB, CON_SVC,  "RQRESP",
    TRUE,  MAY_SYNC, OWN_ACB, CON_SVC,  "FASTSELECT",
    FALSE, IS_SYNC,  NO_ACB,  SYNC_SVC, "REG_TRAP",
    FALSE, IS_SYNC,  NO_ACB,  SYNC_SVC, "UNREG_TRAP", 
};

#ifdef NT_GENERIC
static i4	WaitCursorFlag = 0;	/* 1 -> on NT, set a Wait Cursor */
#endif


/*
** Name: gca_default_cb
**
** Description:
**	Pointer to default control block used by 
**	applications calling IIGCa_call().
**
** History:
**	 3-Sep-96 (gordy)
**	    Created.
*/

static PTR	gca_default_cb ZERO_FILL;

/*
** Name: IIGCa_call - Original GCA entry point.
**
** Description:
**	Originally the entry point for GCA requests, this routine is
**	now a cover for IIGCa_cb_call() and provides a default GCA control
**	block for GCA users who are still using the original interface.
**
** History:
**	 3-Sep-96 (gordy)
**	    Rename IIGCa_call() to IIGCa_cb_call() and added control block
**	    parameter.  IIGCa_call() now a cover for IIGCa_cb_call() which
**	    provides a default control block for the original routine.
**	10-Dec-96 (gordy)
**	    Convert time-out in seconds to time-out in milli-seconds
**	    (watch out for special cases).
*/

STATUS
IIGCa_call( i4  service_code, GCA_PARMLIST *parmlist, i4  indicators,
	    PTR async_id, i4 time_out, STATUS *status )
{
    return( IIGCa_cb_call( &gca_default_cb, service_code, parmlist, 
			   indicators, async_id, 
			   (time_out > 0) ? time_out * 1000 : time_out,
			   status ) );
}


/*
** Name: IIGCa_cb_call - GCA external entry function
**
** Description:
**      IIGCa_cb_call() is the top level control module of GCA.  
**	It is called by the GCA user with the service request 
**	specifications and the parameter list for the requested 
**	service.  The following are the provided GCA services:
**        
**      GCA_INITIATE 
**      GCA_TERMINATE 
**      GCA_FORMAT 
**      GCA_INTERPRET 
**      GCA_SEND 
**      GCA_RECEIVE 
**      GCA_SAVE 
**      GCA_RESTORE 
**      GCA_REQUEST 
**      GCA_LISTEN 
**      GCA_REGISTER 
**      GCA_DISASSOC
**      GCA_ATTRIBS
**	GCA_RQRESP
**	GCA_FASTSELECT
**	GCA_REG_TRAP
**	GCA_UNREG_TRAP
**        
**	Errors discovered by IIGCa_cb_call() result in a DB_STATUS of
**	E_DB_xxxx being returned, and an explanatory value being 
**	set in the status parameter of the call parameter list.  
**	Errors discovered subsequently during service request 
**	processing are reflected in the status code in the service 
**	request parameter list.
**
** Inputs:
**	gca_cb_ptr	Address of a pointer which is initially 
**			NULL.  GCA allocates and manages control 
**			blocks through this pointer.
**
**      service_code	Identifier of the requested GCA service.
**
**      parmlist        Pointer to the data structure containing
**                      the parameters required by the requested
**                      service.
**
**      indicators      Flags specifying modifications to the 
**                      requested service.  The following values
**                      are defined:
**                          GCA_SYNC_FLAG: synchronous execution.
**                          GCA_NO_ASYNC_EXIT: don't drive caller's
**				asynchronous exit on completion.
**                          GCA_ALT_EXIT: caller's parm list
**				specifies alternate completion exit.
**
**      async_id        Completion ID pased to asynchronous
**                      completion exit routine, to identify 
**                      the completed service.
**
**      time_out        Maximum suspension time for synchronous
**                      service execution (milli-seconds).
**
**	status		Pointer to users STATUS area used to
**			indicate actual reason that E_DB_ERROR
**			is being returned.
**
** Outputs:
**
**      status                          Result of the service request execution.
**                                      The following values may be returned:
**
**		E_GC0000_OK		Request accepted for execution.
**		E_GC0002_INV_PARAM	Invalid parameter
**		E_GC0003_INV_SVC_CODE	Invalid service code
**		E_GC0004_INV_PLIST_PTR	Invalid parameter list pointer
**		E_GC0006_DUP_INIT	Duplicate GCA_INITIATE request
**		E_GC0007_NO_PREV_INIT	No previous GCA_INITIATE request
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-apr-87 (jbowers)
**          Initial function creation.
**	 6-sep-93 (swm)
**	    Change type of async_id from i4 to PTR (might have to be
**	    PTR) as pointer completion ids. are often passed.
**	12-Jan-96 (gordy)
**	    Adding cast to quiet compiler.
**	 3-Sep-96 (gordy)
**	    Added control block and renamed to IIGCa_cb_call().
**	14-Nov-96 (gordy)
**	    GCsync() now takes timeout value.
**	10-Dec-96 (gordy)
**	    Time-out now in milli-seconds.
**      03-jun-1999 (horda03)
**          Set flag to indicate when a channel is already in use. This will
**          disllow any further Send (for the same channel) until the
**          outstanding send operation has completed. This will stop a SEGV
**          within the DBMS server if IPM kills a session that has outstanding
**          messages. Bug 87612. 
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	18-Dec-97 (gordy)
**	    Sync request counter made local and reference placed in
**	    ACB to support multi-threaded sync requests.
**	6-Jul-99 (rajus01)
**	    Set the remote flag in gc_parms for Embedded Comm Server 
**	    support. 
**	19-may-2001 (somsa01)
**	    Changed "svc_parms->flags.in_use" to "svc_parms->in_use".
**	15-Oct-03 (gordy)
**	    Added OWN_ACB type which allocates ACB and marks it for
**	    deletion when the request completes.  FASTSELECT may now
**	    be resumed.
**	 1-Mar-06 (gordy)
**	    GCA_REGISTER ACB saved in GCA_CB.  Check for active CALL
**	    operation on GCA_RESUME and use alternate SVC_PARMS.  The
**	    in_use flag is set at the start of gca_service().
**	 8-Jun-06 (gordy)
**	    Registration ACB should only be allocated on initial
**	    GCA_REGISTER call.  The registration association is
**	    closed by GCA_TERMINATE.  If no registration ACB, 
**	    GCA_TERMINATE is inherently synchronous and may be run
**	    async. Closing the registration association requires
**	    GCA_TERMINATE to be synchronized to protect the sync
**	    service parms.  Synchronization requires an ACB, so 
**	    the registration ACB is used.  
*/

STATUS
IIGCa_cb_call
( 
    PTR			*gca_cb_ptr,
    i4			service_code,
    GCA_PARMLIST	*parmlist,
    i4			indicators,
    PTR			async_id, 
    i4		time_out,
    STATUS		*status
)
{
    GCA_CB		*gca_cb = NULL;
    GCA_ACB		*acb = NULL;
    GCA_SVC_PARMS	*svc_parms = NULL;
    GCA_SVC_PARMS	sync_svc_parms;
    i4			subchan = GCA_NORMAL;
    struct gca_dispatch *dispatch = &gca_dispatch[ service_code ];
    bool		run_sync = FALSE;
    bool		run_resume = FALSE;

    /* 
    ** Initialize status. 
    */
    *status = OK;

    /*
    ** Validate parameters.  Caller must provide the address of
    ** a pointer where we will store the address of the control 
    ** block.  The control block will be allocated at GCA_INITIATE 
    ** and must be provided (and initialized) for all other calls.  
    ** The block is freed when gca_initialized is cleared (failed 
    ** GCA_INITIATE or GCA_TERMINATE).
    */
    if ( ! gca_cb_ptr  || 
	 (*gca_cb_ptr  &&  ! ((GCA_CB *)*gca_cb_ptr)->gca_initialized) )
    {
	*status = E_GC0002_INV_PARM;
	return( E_DB_ERROR );
    }

    if ( *gca_cb_ptr  &&  service_code == GCA_INITIATE )
    {
	*status = E_GC0006_DUP_INIT;
	return( E_DB_ERROR );
    }

    if ( ! *gca_cb_ptr  &&  service_code != GCA_INITIATE )
    {
	*status = E_GC0007_NO_PREV_INIT;
	return( E_DB_ERROR );
    }

    if ( service_code < 1  ||  service_code > GCA_UNREG_TRAP  || 
	 ! dispatch->svc_type )
    {
	*status = E_GC0003_INV_SVC_CODE;
	return( E_DB_ERROR );
    }

    if ( ! parmlist )
    {
	*status = E_GC0004_INV_PLIST_PTR;
	return( E_DB_ERROR );
    }

    /*
    ** Allocate control block as needed.
    */
    if ( ! *gca_cb_ptr )
    {
	/*
	** We can't call gca_alloc() if this is
	** the first GCA_INITIATE request since
	** the GCA global data area has not been
	** initialized.  If this is the case, we
	** will do the allocation here, making
	** sure to use the same memory management
	** function gca_alloc() will use once 
	** initialized (see gca_init()).
	*/
	if ( IIGCa_global.gca_initialized )
	    *gca_cb_ptr = gca_alloc( sizeof(GCA_CB) );
	else  if ( parmlist->gca_in_parms.gca_alloc_mgr )
	    *status = (*parmlist->gca_in_parms.gca_alloc_mgr)
					    ( 1, sizeof(GCA_CB), gca_cb_ptr );
	else
	    *gca_cb_ptr = MEreqmem( 0, (u_i4) sizeof(GCA_CB), TRUE, status );

	if ( ! *gca_cb_ptr )
	{
	    *status = E_GC0013_ASSFL_MEM;
	    return( E_DB_ERROR );
	}
    }

    gca_cb = (GCA_CB *)*gca_cb_ptr;

    /*
    ** Determine execution mode from service request.
    */
    switch( service_code )
    {
	case GCA_LISTEN:
	    if ( gca_cb->gca_api_version >= GCA_API_LEVEL_4 )
		run_resume = TRUE;
	    break;

	case GCA_REQUEST:
	    if ( gca_cb->gca_api_version < GCA_API_LEVEL_1 )
		indicators |= GCA_SYNC_FLAG;	/* GCA_API_LEVEL_0: sync */
	    else if ( gca_cb->gca_api_version >= GCA_API_LEVEL_2 )
		run_resume = TRUE;
	    break;

	case GCA_FASTSELECT:
	    if ( gca_cb->gca_api_version >= GCA_API_LEVEL_6 )
		run_resume = TRUE;
	    break;

	case GCA_SEND:
	    switch( parmlist->gca_sd_parms.gca_message_type )
	    {
		case GCA_ATTENTION:
		case GCA_NP_INTERRUPT:	
		    subchan = GCA_EXPEDITED; 
		    break;
	    }
	    break;

	case GCA_RECEIVE:
	    subchan = parmlist->gca_rv_parms.gca_flow_type_indicator; 
	    break;

	case GCA_DISASSOC:
	    if ( gca_cb->gca_api_version < GCA_API_LEVEL_2 )
		indicators |= GCA_SYNC_FLAG;	/* GCA_API_LEVEL_0-1: sync */
	    else
		run_resume = TRUE;
	    break;
    }

    /*
    ** Get association control block (acb).
    */
    switch( (indicators & GCA_RESUME) ? OLD_ACB : dispatch->acb_type )
    {
	case NO_ACB:
	    /*
	    ** Doesn't need a GCA_ACB.
	    */
	    acb = NULL;
	    break;

	case NEW_ACB:
	    /*
	    ** Alloc a new GCA_ACB.
	    */
	    if( ! (acb = gca_add_acb()) )
	    {
		*status = E_GC0013_ASSFL_MEM;
		return( E_DB_ERROR );
	    }

	    parmlist->gca_all_parms.gca_assoc_id = acb->assoc_id;
	    break;

	case OWN_ACB:
	    /*
	    ** Alloc a new GCA_ACB.
	    */
	    if( ! (acb = gca_add_acb()) )
	    {
		*status = E_GC0013_ASSFL_MEM;
		return( E_DB_ERROR );
	    }

	    /*
	    ** Flag ACB for deletion when complete.
	    */
	    acb->flags.delete_acb = TRUE;
	    parmlist->gca_all_parms.gca_assoc_id = acb->assoc_id;
	    break;

	case REG_ACB:
	    /*
	    ** Registration ACB is allocated on first GCA_REGISTER
	    ** call.  Subsequent calls re-use the ACB.  No ACB is
	    ** used if GCA_REGISTER has not been called.
	    */
	    if ( 
	         service_code == GCA_REGISTER  &&  
		 ! gca_cb->gca_reg_acb  &&  
	         ! (gca_cb->gca_reg_acb = gca_add_acb()) 
	       )
	    {
		*status = E_GC0013_ASSFL_MEM;
		return( E_DB_ERROR );
	    }

	    acb = gca_cb->gca_reg_acb;
	    break;

	case OLD_ACB:
	    /*
	    ** Fetch old acb using assoc_id from parmlist.
	    */
	    if ( ! (acb = gca_find_acb(parmlist->gca_all_parms.gca_assoc_id)) )
	    {
		*status = E_GC0005_INV_ASSOC_ID;
		return( E_DB_ERROR );
	    }
	    break;
    }

    /*
    ** Is synchronization required/requested?
    */
    switch( dispatch->sync_type )
    {
    case MAY_SYNC :
	/*
	** Service may be sync or async.  Use requested mode.
	**
	** NOTE: service capability is based on highest API
	** level.  Services which have restrictions at lower 
	** API levels set sync flag accordingly above.
	*/
	run_sync = ((indicators & GCA_SYNC_FLAG) != 0);
	break;

    case IS_SYNC :
	/*
	** Service is inherently synchronous. 
	** No need to synchronize, so run async.
	*/
	run_sync = FALSE;
	break;

    case DO_SYNC :
	/*
	** Service should be synchronized.  
	**
	** NOTE: Synchronization is only possible/needed
	** with ACB.  Otherwise, service may be run async.
	*/
	run_sync = (acb != NULL);
	break;

    case NO_SYNC :
	/*
	** Service should not be synchronized.
	*/
	run_sync = FALSE;
	break;
    }

    /*
    ** Set completion routine
    */
    if ( dispatch->complete )
    {
	if ( indicators & (GCA_NO_ASYNC_EXIT | GCA_SYNC_FLAG) )
	{
	    parmlist->gca_all_parms.gca_completion = 0;
	}
	else if ( indicators & GCA_ALT_EXIT )
	{
	    /* leave gca_completion as user set */
	}
	else if ( service_code == GCA_RECEIVE  &&  subchan == GCA_EXPEDITED )
	{
	    parmlist->gca_all_parms.gca_completion = gca_cb->expedited_ce;
	}
	else
	{
	    parmlist->gca_all_parms.gca_completion = gca_cb->normal_ce;
	}

	parmlist->gca_all_parms.gca_closure = async_id;
    }

    /*
    ** Initialize svc_parms.
    */
    switch( dispatch->svc_type )
    {
	case RCV_SVC : 
	    svc_parms = &acb->rcv_parms[ subchan ];
	    break;

	case SND_SVC : 
	    svc_parms = &acb->snd_parms[ subchan ];
	    break;

	case CON_SVC : 
	    svc_parms = &acb->con_parms;
   	    break;

	case SYNC_SVC : 
	default :
	    MEfill( sizeof( sync_svc_parms ), '\0', (PTR)&sync_svc_parms );
	    svc_parms = &sync_svc_parms; 
	    break;
    }

    if ( indicators & GCA_RESUME )
    {
	/*
	** When a CALL operation occurs, the CALL parms in the
	** svc_parms is set to the active parms.  When a RESUME
	** occurs during a CALL, the original svc_parms is loaded 
	** from the ACB.  Load the active CALL parms if present.
	*/
    	if ( svc_parms->call_parms )  svc_parms = svc_parms->call_parms;
    }
    else  if ( svc_parms->in_use )
    {
	/*
	** An active SVC_PARMS on a new request indicates an
	** invalid duplicate request 
	*/
	*status = E_GC0024_DUP_REQUEST;
	return( E_DB_ERROR );
    } 
    else
    {
	/*
	** Initialize the svc_parms, filling every slot we can find.
	*/
	svc_parms->gca_cb = gca_cb;
	svc_parms->acb = acb;
	svc_parms->state = 0;
	svc_parms->service_code = service_code;
	svc_parms->parameter_list = (PTR)parmlist;

	svc_parms->gc_parms.gc_cb = acb ? acb->gc_cb : NULL;
	svc_parms->gc_parms.status = 0;
	svc_parms->gc_parms.sys_err = &parmlist->gca_all_parms.gca_os_status;
	svc_parms->gc_parms.flags.run_sync = run_sync;
	svc_parms->gc_parms.flags.flow_indicator = subchan;
	svc_parms->gc_parms.flags.new_chop = 0;
	if ( acb && acb->flags.remote ) 
	    svc_parms->gc_parms.flags.remote = TRUE;
	svc_parms->gc_parms.closure = (PTR)svc_parms;
	svc_parms->gc_parms.gca_cl_completion = (run_resume  &&  ! run_sync)
						? gca_resume : gca_service;

	/* GCA_API_LEVEL_0-1: no async timeouts */
	if ( gca_cb->gca_api_version < GCA_API_LEVEL_2  &&  ! run_sync )
	    time_out = -1;

	svc_parms->time_out = time_out;
	svc_parms->gc_parms.time_out = time_out;
    }

    /*
    ** Make service call.
    */

    GCA_DEBUG_MACRO(2)( "%04d GCA_%s%s timeout=%d %s\n",
			(i4)(acb ? acb->assoc_id : -1), dispatch->name, 
			(indicators & GCA_RESUME) ? "(RESUME)" : "",
			svc_parms->time_out, run_sync ? "sync" : "async" );

    if ( ! run_sync )
	gca_service( (PTR)svc_parms );		/* Async operation */
    else
    {
	/*
	** Drive a synchronous I/O call.  This usually implies this is a 
	** frontend, and is subject to keyboard interrupts.  We disable
	** interrupts, and GCsync() delivers them only while blocked for 
	** I/O.
	** 
	** We provide a synchronous request counter which is incremented
	** and looped on until decremented by gca_complete().
	**
	** If an interrupt handler invoked during the GCsync() below
	** recursively performs a synchronous IIGCa_cb_call(), that 
	** IIGCa_cb_call() will use our counter and drive this request 
	** to completion as well.  In an interrupt handler, all out-
	** standing I/O operations are now completed (they used to be 
	** aborted).
	**
	** The ACB cannot be referrenced after calling gca_service(),
	** so needed ACB values are cached prior to the call.
	*/
	i4	sync_count = 0;		/* Our sync request counter */
	i4	*syncing = NULL;	/* Current sync request counter */
	i4	assoc_id = -1;

#ifdef  NT_GENERIC
	if (WaitCursorFlag)
	{
	    WaitCursorFlag = 0;
	    SetCursor(LoadCursor(0, IDC_WAIT));
	}
#endif  /* NT_GENERIC */
 
	EXinterrupt( EX_OFF );

	if ( svc_parms->acb )
	{
	    /*
	    ** Set our sync request counter in the 
	    ** ACB (if no other request is active) 
	    ** and increment the counter.
	    */
	    if ( ! svc_parms->acb->syncing )
		svc_parms->acb->syncing = &sync_count;
	    (*svc_parms->acb->syncing)++;

	    /*
	    ** Cache ACB values needed after call to gca_service()
	    ** (ACB may be deleted during call).
	    */
	    assoc_id = svc_parms->acb->assoc_id;
	    syncing = svc_parms->acb->syncing;
	}

	gca_service( (PTR)svc_parms );

	/* 
	** Drive all sync to completion. 
	*/
	while( syncing  &&  *syncing )
	{

	    GCA_DEBUG_MACRO(3)( "%04d     GCA_%s syncing (%d)\n",
				assoc_id, dispatch->name, *syncing );
	    GCsync( -1 );
	}

	EXinterrupt( EX_ON );
    }

    /*
    ** The GCA control block should be freed after
    ** GCA_TERMINATE is called, or if GCA_INITIALIZE
    ** fails.  The gca_initialized flag is only set
    ** for a successful initialization and is reset
    ** at termination.
    */
    if ( ! ((GCA_CB *)*gca_cb_ptr)->gca_initialized )
    {
	gca_free( *gca_cb_ptr );
	*gca_cb_ptr = NULL;
    }

    return( OK );
}


/*
** Name: gca_resume() - CL exit point for resumable async operations
**
** Description:
**	Certain GCA services must always run in the client context, rather
**	than as completion routines.  These services, such as GCA_REQUEST
**	and GCA_DISASSOC, make allocator or semaphore calls after their
**	initial async CL request.  To ensure that these services always run
**	in the client context, all GCA CL functions they invoke drive
**	gca_resume as their completion routines; gca_resume then drives the
**	client completion routine with E_GCFFFE_INCOMPLETE, to indicate
**	that the client should reinvoke the service with the GCA_RESUME
**	indicator.
**
** History:
**	10-Nov-89 (seiwald)
**	    Pulled from GCA_REQUEST.
**	28-Jun-92 (brucek)
**	    GCA_LISTEN can be RESUMEd as of GCA_API_LEVEL_4.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

VOID
gca_resume( PTR closure )
{
    GCA_SVC_PARMS *svc_parms = (GCA_SVC_PARMS *)closure;
    GCA_ALL_PARMS *parmlist = (GCA_ALL_PARMS *)svc_parms->parameter_list;

    parmlist->gca_status = E_GCFFFE_INCOMPLETE;
    (*parmlist->gca_completion)( parmlist->gca_closure );

    return;
}

#ifdef NT_GENERIC 
/*
** Name: IIGCa_setWaitCursor - set the local WAIT cursor flag 
**
** Description:
**    Set the local static variable WaitCursorFlag to the value passed in.
** WaitCursorFlag can be tested to see if a WAIT cursor should be displayed.
**
** History:
**	07-feb-94 (leighb)	Created.
**
*/
VOID
IIGCa_setWaitCursor(flag)
i4	flag;
{
	WaitCursorFlag = flag;
}
#endif /* NT_GENERIC */

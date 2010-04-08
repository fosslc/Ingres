/*
** CHANGE NOTE: When changing data structures used in this file, or the
** meaning/use of those data structures read the "Change Note" in <scev.h>
** (neil).
*/

/* NO_OPTIM = usl_us5 */

/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = usl_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <scf.h>
#include    <sceshare.h>
#include    <scev.h>
#include    <sc0e.h>
#include    <cx.h>
/**
**
**  Name: sceshare.c - Server event subsystem (EV) manager
**
**  Description:
**	This module supports communication of events between servers in
**	a single installation (on a single node - in the VAXcluster case).
**
**	The server event subsystem should not be confused with user events
**	supported as alerts within the DBMS.  The server event subsystem
**	may only be used by DBMS servers.  Those familiar with the processing
**	of alerts in the DBMS (sce.c) will find parallels with the way the
**	event subsystem fields server events.
**
**	This functionality is written on top of the CL shared memory
**	mechanism.  If the shared memory abstraction is not supported in
**	a particular environment, the Event Subsystem cannot be implemented.
**
**	Because this module may be used externally (CSP) this may not include
**	or link in SCF-specific data.
**
**  Defines:
**	sce_einitialize	- Initialize event subsystem for installation
**	sce_econnect	- Connect (a server) to the event subsystem
**	sce_eregister	- Register a server to receive notification of an event
**	sce_ederegister	- Deregister an event no longer of interest
**	sce_esignal	- Server request to broadcast an event
**	sce_efetch	- Server request to fecth it's next instance
**	sce_edisconnect	- Disconnect a server from event subsystem
**	sce_ewinit	- Initialize lock list
**	sce_ewset	- About to wait for the event
**	sce_ewait	- Wait for an LK event to arrive
**	sce_edump	- Dump cross-server event subsystem
**	sce_mlock	- Perform a P operation on a semaphore
**	sce_munlock	- Perform a V operation on a semaphore
**
**  Internal Routines:
**    List Management:
**	sce_erlock	- Find and lock an event registration
**	sce_erunlink	- Unlink registration out of event chain
**	sce_evunlink	- Unlink event out of hash chain 
**	sce_eslock	- Find and lock a server instance
**    Allocation:
**	sce_eallocate	- Allocate a piece of from memory
**	sce_efree	- Free a piece back into memory
**    Tracing:
**	sce_ename   	- Format an event name
**
**  History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	19-mar-90 (rogerk)
**	    Added sce_ewset routine to prepare for sce_ewait call.  Took
**	    LKevent set flag out of sce_ewait - this is done in sce_ewset now.
**	10-jan-91 (neil)
**	    Event instance allocation is now varying length.
**	25-jan-91 (neil)
**	    1. Serialize calls to ME page [de]allocation (MEget_pages & 
**	       MEfree_pages) for multi-threaded server through evc_mem_sem.
**	    2. Allow caller to disconnect another server (for cleanup).
**	16-aug-91 (ralph)
**	    Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_ewinit() grabs a lock (LK_EVCONNECT) during connect processing.
**	    sce_esignal() attempts to get the target server's LK_EVCONNECT
**              lock before queueing an event instance.  If it can get the
**              lock, the target server is disabled.
**	    sce_dump() probes servers' LK_EVCONNECT lock to determine if
**              they are still active.  Corequisite changes are made to
**		the sceadmin utility.
**	    sce_edisconnect() Remove lock list if not external disconnect.
**      16-sep-91 (ralph)
**	    Bug 39791. Don't issue E_SC029D_XSEV_PREVERR when server 
**		already disabled.  Don't return failure status when
**		phantom or disabled server is detected.
**	    Bug 39832. Use LK_MULTITHREAD flag on LK calls on LKrequest()
**		and LKrelease().
**	28-sep-92 (kwatts)
**	    Added calls of CSn_semaphore to give names to semaphores for
**	    tracing.
**	13-jan-93 (mikem)
**	    Lint directed changes.  Fixed "==" vs. "=" bugs in sce_edump().  
**	    Previous to this fix the "(*ERROR*)" and "(LKERROR)" cases were not
**	    formated properly (probably did not happen very often if ever).
**	2-Jul-1993 (daveb)
**	    remove extra comment start.
**	    
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	04-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	30-May-1995 (jenjo02)
**	    Replaced CSp|v_semaphore statements with function calls
**	    to sce_mlock(), sce_munlock().
**	    Defined sce_mlock(), sce_munlock() in sceshare.c for
**	    use by all modules in sce to cut down on the (very)
**	    large number of CSp|v_semaphore macro expansions.
**      06-aug-1997 (canor01)
**          Remove semaphores before freeing memory.
**	24-Oct-1997 (jenjo02)
**	    Don't remove semaphores if the memory segment is not going
**	    to be destroyed; subsequent servers connecting to that
**	    memory expect it to be properly initialized.
**  12-Dec-97 (bonro01)
**      Added cleanup code for cross-process semaphores during
**      MEsmdestroy().
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**      23-jul-1998 (rigka01)
**          in sce_esignal,
**          when event flag indicates that event shouldn't be
**          sent to the originating server, then don't put it on
**          that server's event queue
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**  16-Mar-2000 (linke01)
**      Added NO_OPTIM for usl_us5 to resolve replicator problem, symptom is
**      dd_stop_server can't shutdown replicator server, rows can't be       
**      replicated from one database to another database and dd_distrib_queue
**      didn't get cleaned. 
**	28-Feb-2002 (jenjo02)
**	    Removed LK_MULTITHREAD from LKrequest/release flags. 
**	    MULTITHREAD is now an attribute of evc_locklist.
**	7-oct-2004 (thaju02)
**	    Use SIZE_TYPE for memory related vars.
**	5-apr-2005 (mutma03)
**	    Suppressed the error messages if the function sce_econnect has been 
**	    called by the recovery server as the EV_SEGMENT may not have been
**	    created yet by the dbms during startup.
**      20-Oct-2006 (horda03) Bug 116912
**          E_SC029B_XSEV_REQLOCK could be reported by a server, if it tries
**          to send an event to a server which has connected to the Event Shared
**          memory system but which hasn't yet made an attempt to acquire its
**          LK_EVCONNECT lock. Added a flag to the server instance (EV_SVRINIT),
**          to prevent other server from attempting to send events to the new
**          server until it has made its attempt to get its LK_EVCONNECT lock.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
***/

/* Forward declarations */

static VOID sce_erlock(EV_SCB		*ecb,
		       SCEV_EVENT	*event,
		       i4	    	*ebucket,
		       CS_SEMAPHORE	**eunlock,
		       i4	   	*eoffset,
		       i4	    	*roffset );

static VOID sce_erunlink(EV_SCB *ecb, EV_REVENT *eptr,EV_RSERVER  *rremove );

static VOID sce_evunlink( EV_SCB *ecb,
			 EV_EVHASH *ehash,
			 EV_REVENT *eremove );

static VOID sce_eslock( EV_SCB *ecb,
		       PID pid,
		       i4  *sbucket,
		       CS_SEMAPHORE **sunlock,
		       i4 *soffset );

static DB_STATUS sce_eallocate( EV_SCB *ecb,
			       i4  amount,
			       i4 *alloc_offset );

static VOID sce_efree( EV_SCB *ecb, i4  amount, i4 deall_offset );

static i4  sce_ename( i4  lname, char *name, char *evname );


/*{
** Name: sce_einitialize   - Initialize event subsystem for this installation
**
** Description:
**      This call initializes the event subsystem for this installation.
**	Each server will try to initialize but onlt the first server will
**	succeed.  The first time through the shared memory segment will
**	be allocated and the global data structures will be allocated
**	out of this segment.
**
**	It is assumed that if this is called a second time (after already
**	initialized) then the shared memory allocator will return a status
**	indicating that the segment already exists.  Whether this is the
**	first or later invocation, all servers must "connect" after attempting
**	to initialize (sce_econnect) which gets them all synced up.
**
** 	Algorithm:
**	    try to create EV shared segment;
**	    if it exists return;
**	    allocate EV SCB out of shared segment;
**	    mark event scb as inactive;
**	    (P)initialize/lock multi-process master semaphore (evs_gsemaphore);
**	    initialize internal data structures and offsets;
**	    mark event scb as active;
**	    (V)release master semaphore (evs_gsemaphore);
**
** Inputs:
**	evc			EV request context structure:
**	    .evc_pid		Process ID of initiator - must be set by caller
**				and is an assumed input for all EV calls.
**	    .evc_rbuckets	Number of event registration buckets
**	    .evc_ibuckets	Number of server event instance buckets
**	    .evc_mxre		Maximum number of registrations
**	    .evc_mxei		Maximum number of instances
**	    .evc_errf		Error function - this must be set by the caller
**				and is an assumed input for all EV calls.
**	    .evc_trcf		Trace function - this must be set by the caller
**				and is an assumed input for all EV calls.
** Outputs:
**	evc			EV request context structure.  The "root" field
**				must be left intact for reuse with EV:
**	    .evc_root		Root of shared memory segment (if we create
**				it here), otherwise NULL (on error or if the
**				segment is already created - use sce_econnect).
**	Returns:
**	    DB_STATUS		E_DB_FATAL implies that events cannot be
**				processed by this server.
**	Errors:
**	    Errors found during processing are logged in the error log:
**	    E_SC0289_XSEV_ALLOC		Shared memory segment allocation
**	    E_SC0288_XSEV_SM_DESTROY	Shared memory segment destruction
**	    E_SC0292_XSEV_SEM_INIT	Setup of cross-server semaphores
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	10-jan-91 (neil)
**	    Event instance size is varying length so need to modify allocation.
**	25-jan-91 (neil)
**	    Serialize calls to ME page [de]allocation through evc_mem_sem.
**	28_sep_92 (kwatts)
**	    Add call to CSn_semaphore for EV_SCB, EV_HASH, SVR_HASH, and
**	    ALL_REG_LIST semaphores.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_einitialize( SCEV_CONTEXT *evc )
{
    EV_SCB		*ecb;		/* EV system CB to allocate */
    char		smnm[EV_SEGSIZE+1];/* Shared memory name */
    SIZE_TYPE		alloc;		/* Memory need for all of EV */
    SIZE_TYPE		pages;		/* EV memory is page units */
    SIZE_TYPE		return_pages; 	/* ME returns to user - ignored */
    i4		clflags;	/* For ME */
    EV_EVHASH		*ehash;		/* Event registration hash array */
    EV_SVRHASH		*shash;		/* Server event instance hash array */
    i4			bkt;		/* For scanning hash arrays */
    EV_FREESPACE	*freesp;	/* For initializing free space */
    bool		ev_created;	/* For error recovery - created EV */
    CL_ERR_DESC		clerror;
    STATUS              clstat;

    /*
    ** Fabricate the EV shared memory area name.  There is only one EV area
    ** per installation with the specified name.  ME will guarantee
    ** that this name is unique across servers in an installations.
    */
    MEcopy((PTR)EV_SEGNAME, EV_SEGSIZE, (PTR)smnm);
    smnm[EV_SEGSIZE] = EOS;

    /*
    ** Compute the size of the shared memory segment required:
    **
    **	EV System CB +
    **  # of server event registration buckets +
    **  max # of total allowed event registrations (assume 1 server per event) +
    **  # of server instance buckets +
    **  max # of total allowed event instances (assume 1 instance per server)
    **
    ** We make the (conservative) assumption that there is only one server
    ** registered for each event and one server receives each event (not so
    ** conservative).  This should give us enough memory to support what we
    ** now think is a typical use of the event system.  Of course, the user
    ** may override these defaults by increasing the number of registrations
    ** or instances supported in the server startup options.
    */
    alloc = (sizeof(EV_SCB)) +
	    (evc->evc_rbuckets * sizeof(EV_EVHASH)) +
	    (evc->evc_mxre * (sizeof(EV_REVENT) + sizeof(EV_RSERVER))) +
	    (evc->evc_ibuckets * sizeof(EV_SVRHASH)) +
	    (evc->evc_mxei * (sizeof(EV_SVRENTRY) + EV_INST_ALLOC_MAX));
    pages = (alloc + ME_MPAGESIZE) / ME_MPAGESIZE;	/* Bytes to pages */

    /*
    ** Create the EV shared memory segment.  If it already exists then we allow
    ** our caller to connect to it (sce_econnect).  Make sure to serialize
    ** memory allocation.
    */
    ev_created = FALSE;
    evc->evc_root = NULL;
    evc->evc_locklist = 0;
    clflags = ME_MSHARED_MASK|ME_CREATE_MASK|ME_MZERO_MASK|ME_NOTPERM_MASK;
    sce_mlock(evc->evc_mem_sem);
    clstat = MEget_pages(clflags, pages, smnm, &evc->evc_root,
			 &return_pages, &clerror);
    sce_munlock(evc->evc_mem_sem);

    /*
    ** If shared segment already exists, it may either be valid or it may be
    ** inactive (initialized or being deallocated).  The connection phase
    ** checks this (sce_econnect).
    */
    if (clstat == ME_ALREADY_EXISTS)
    {
	evc->evc_root = NULL;		/* Will be attached in sce_econnect */
	return (E_DB_OK);
    }
    if (clstat != OK) /* Shared memory segment could not be allocated */
    {
	(*evc->evc_errf)(E_SC0289_XSEV_ALLOC, &clerror, 1,
			 sizeof(pages), &pages);
	goto err_exit;
    }
    ev_created = TRUE;

    /* Anchor the EV segment pointer and initialize global data structures */
    ecb = (EV_SCB *)evc->evc_root;

    /*
    ** Set the EVENT subsystem to inactive while we initialize, to prevent
    ** other connecting servers from using EV while we are initializing.
    */
    ecb->evs_status = EV_INACTIVE;

    /* Initialize the synchronization semaphore and lock it asap */
    clstat = CSw_semaphore(&ecb->evs_gsemaphore, CS_SEM_MULTI,
			   "Event (EV_SCB)");
    if (clstat != OK)
    {
	(*evc->evc_errf)(E_SC0292_XSEV_SEM_INIT, 0, 1,
		         sizeof("evs_gsemaphore")-1, "evs_gsemaphore");
	goto err_exit;
    }
    sce_mlock(&ecb->evs_gsemaphore);

    ecb->evs_version = EV_VERSION;
    MEcopy((PTR)smnm, EV_SEGSIZE, (PTR)ecb->evs_segname);
    ecb->evs_memory = pages * ME_MPAGESIZE;	/* Full memory allocated */
    ecb->evs_orig_pid = evc->evc_pid;		/* Save originator */

    /* Initialize the server event registration hash array */
    ecb->evs_rbuckets = evc->evc_rbuckets;
    ecb->evs_rcount = 0;			/* No registrations yet */
    /* Registration space begins right after master SCB */
    ecb->evs_roffset = sizeof(EV_SCB);
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    for (bkt = 0; bkt < ecb->evs_rbuckets; bkt++)
    {
	char sem_name[25];

	clstat = CSw_semaphore(&ehash->ev_ehsemaphore, CS_SEM_MULTI,
			 STprintf(sem_name, "Event (EV_HASH %d)", bkt));
	if (clstat != OK)
	{
	    sce_munlock(&ecb->evs_gsemaphore);
	    (*evc->evc_errf)(E_SC0292_XSEV_SEM_INIT, 0, 1,
		             sizeof("ev_ehsemaphore")-1, "ev_ehsemaphore");
	    goto err_exit;
	}

	ehash->ev_ehoffset = 0;			/* No events registered yet */
	ehash++;
    }

    /* Initialize the server event instance hash array */
    ecb->evs_scount = 0;			/* No servers connected yet */
    ecb->evs_ibuckets = evc->evc_ibuckets;
    ecb->evs_icount = 0;			/* No event instances yet */
    ecb->evs_isuccess = 0;
    ecb->evs_ifail = 0;
    /*
    ** Server instance space follows registration start space plus
    ** the size of the registration hash array.
    */
    ecb->evs_ioffset = ecb->evs_roffset +
		       (ecb->evs_rbuckets * sizeof(EV_EVHASH));
    shash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);
    for (bkt = 0; bkt < ecb->evs_ibuckets; bkt++)
    {
	char sem_name[25];

	clstat = CSw_semaphore(&shash->ev_shsemaphore, CS_SEM_MULTI,
	 		STprintf(sem_name, "Event (SVR_HASH %d)", bkt));
	if (clstat != OK)
	{
	    sce_munlock(&ecb->evs_gsemaphore);
	    (*evc->evc_errf)(E_SC0292_XSEV_SEM_INIT, 0, 1,
			     sizeof("ev_shsemaphore")-1, "ev_shsemaphore");
	    goto err_exit;
	}
	shash->ev_shoffset = 0;			/* No servers yet */
	shash++;
    }

    /* Initialize the all-event-registered server data structures */
    clstat = CSw_semaphore(&ecb->evs_asemaphore, CS_SEM_MULTI,
			    "Event (ALL_REG_LIST)");
    if (clstat != OK)
    {
	sce_munlock(&ecb->evs_gsemaphore);
	(*evc->evc_errf)(E_SC0292_XSEV_SEM_INIT, 0, 1,
			 sizeof("evs_asemaphore")-1, "evs_asemaphore");
	goto err_exit;
    }
    ecb->evs_acount = 0;			/* No "all-reg" servers yet */
    ecb->evs_alloffset = 0;

    /*
    ** Initialize the free list.  The free list space follows the server
    ** instance start space plus the size of the server instance has array.
    */
    ecb->evs_foffset = ecb->evs_ioffset +
		       (ecb->evs_ibuckets * sizeof(EV_SVRHASH));
    /* Now pick a free space node out of the remaining chunk and calculate
    ** the remaining dynamic memory:
    **		Total memory - free list space offset
    ** Note that the single free space node IS available for reuse and
    ** that we do not subtract it from the total available memory.
    */
    freesp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
    ecb->evs_favailable = ecb->evs_memory - ecb->evs_foffset;
    freesp->ev_fsize = ecb->evs_favailable;	/* Amount to deal with */
    freesp->ev_fnextoff = 0;			/* Not fragmented yet */

    /* Looks like we are properly initialized */
    ecb->evs_status = EV_ACTIVE;
    sce_munlock(&ecb->evs_gsemaphore);

    return (E_DB_OK);

err_exit:
    /*
    ** Common error handling code
    */
    if (ev_created)			/* Destroy the shared segment */
    {
	sce_mlock(evc->evc_mem_sem);
	MEfree_pages((PTR)ecb, pages, &clerror);
	CS_cp_sem_cleanup(smnm, &clerror);
	clstat = MEsmdestroy(smnm, &clerror);
	sce_munlock(evc->evc_mem_sem);
	if (clstat != OK)
	{
	    (*evc->evc_errf)(E_SC0288_XSEV_SM_DESTROY, &clerror, 1,
			     STlength(smnm), smnm);
	}
    }
    evc->evc_root = NULL;
    return (E_DB_FATAL);
} /* sce_einitialize */

/*{
** Name: sce_econnect	- Connect a server to the event subsystem
**
** Description:
**	This routine connects a server to the event subsystem.  Note that
**	the server is not necessarily the one that created the shared
**	segment for EV (sce_einitialize) - if it isn't then some
**	smaller initialization steps need to be taken.  When connecting
**	we verify that EV is active - if it isn't then the caller should try
**	again.
**
**	An entry is made into the server instance hash array to indicate the
**	connection.
**
** 	Algorithm:
**	    verify connection request;
**	    (P)lock master semaphore & check if EV is active (evs_gsemaphore);
**	    increment server count;
**	    (V)unlock master semaphore (evs_gsemaphore);
**	    find server hash bucket based on server id;
**	    (P)lock server hash semaphore (ev_shsemaphore);
**	    allocate server entry structure and link to hash bucket;
**	    if all-event-register allocate registration entry;
**	    (V)unlock server hash semaphore (ev_shsemaphore);
**	    if all-event-register then
**		(P)lock all-server registration list (evs_asemaphore);
**		attach new registration, bump count;
**		(V)unlock all-server registration list (evs_asemaphore);
**	    endif
**
** Inputs:
**	evc			EV request context structure:
**	    .evc_pid		Process ID of connecter.
**	    .evc_root		If NULL then this server did NOT create
**				the EV segment and really needs to connect.
**	    .evc_flags		Modifiers to connection.  Currently,
**				SCEV_REG_ALL implies that this server wants
**				to register all events.
** Outputs:
**	evc			EV request context structure. After successful 
**				execution of this routine this data structure
**				must be left intact by caller:
**	    .evc_root		If evc_root was not NULL on entry then will
**				be set (unless there was an error).
**	evactive		If EV is currently inactive then the caller
**				is told this so that they can try again.  This
**				way one server can retry if another server
**				is just initializing. This value (FALSE) is
**				valid if the return status is E_DB_OK.
**	Returns:
**	    DB_STATUS		E_DB_FATAL implies that events cannot be
**				processed by this server.
**	Errors:
**	    Errors found during processing are logged in the error log:
**	    E_SC028B_XSEV_ATTACH	Attaching to shared segment
**	    E_SC0293_XSEV_VERSION	Incompatible segment version
**	    E_SC028C_XSEV_EALLOCATE	Allocating entries for connection.
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Serialize calls to ME page [de]allocation through evc_mem_sem.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	20-apr-1999 (popri01)
**	    Add NO_OPTIM for Unixware 7 (usl_us5) to prevent a problem wherein
**	    registered applications are not notified when events are raised.
**	    The problem appeared consistently throughout the runall dbevents 
**	    test suite as a failure in the "get event" function (event
**	    expected but not received).
**	5-apr-2005 (mutma03)
**	    Suppressed the error messages if this function has been called
**	    by the recovery server as the EV_SEGMENT may not have been created
**	    yet by the dbms during startup.
**      20-Oct-2006 (horda03) Bug 116912
**          Indicate that the server hasn't completed its initialisation - i.e.
**          it hasn't tried to acquire its LK_EVCONNECT lock.
*/

DB_STATUS
sce_econnect( SCEV_CONTEXT *evc, i4  *evactive )
{
    EV_SCB		*ecb;		/* EV system CB to connect to */
    char		smnm[EV_SEGSIZE+1];/* Shared memory name */
    SIZE_TYPE		return_pages; 	/* ME returns to user - ignored */
    i4		clflags;	/* For ME */
    i4			sbucket;	/* Server bucket number */
    EV_SVRHASH		*shash;		/* Server event instance hash array */
    EV_SVRENTRY		*server;	/* Server entry in bucket */
    i4		soffset;	/* Space allocation offset */
    i4		roffset;	/* Registration for ALL events */
    EV_RSERVER		*rptr;		/* Registration entry if for ALL */
    i4			verserr;	/* For version errors */
    i4			allocerr;	/* For allocation errors */
    CL_ERR_DESC		clerror;
    STATUS              clstat;
    DB_STATUS		status;

    *evactive = FALSE; 		/* Assume EV is not active */

    /* Connect this server to the EVENT subsystem (if not already connected) */
    if ((ecb = (EV_SCB *)evc->evc_root) == NULL)
    {
	/* Server not connected to shared memory, do this first */
	MEcopy((PTR)EV_SEGNAME, EV_SEGSIZE, (PTR)smnm);
	smnm[EV_SEGSIZE] = EOS;
	clflags = ME_MSHARED_MASK | ME_NOTPERM_MASK;
	sce_mlock(evc->evc_mem_sem);
	clstat = MEget_pages(clflags, 0, smnm, &evc->evc_root,
			     &return_pages, &clerror);
	sce_munlock(evc->evc_mem_sem);
	if (clstat != OK)		
	{
	    /* filter spurious message, as rcp server comes up first and
              is looking for a segment which is later created by the dbms */

	    if ( !CXconfig_settings(CX_HAS_CSP_ROLE) )
	        (*evc->evc_errf)(E_SC028B_XSEV_ATTACH, &clerror, 1,
			     STlength(smnm), smnm);
	
	    evc->evc_root = NULL;
	    return (E_DB_FATAL);
	}
	ecb = (EV_SCB *)evc->evc_root;
    }

    /* If EV is inactive another server is creating/disposing it - try again */
    if (ecb->evs_status == EV_INACTIVE)
    {
	sce_mlock(evc->evc_mem_sem);
	MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	sce_munlock(evc->evc_mem_sem);
	evc->evc_root = NULL;
	return (E_DB_OK);		/* OK & try again (evactive = FALSE) */
    }

    /*
    ** Lock EV to bump the server count - this is to avoid another server
    ** thinking it can dispose of EV because it is the last server to
    ** disconnect (see sce_edisconnect which relies on the count).
    */
    sce_mlock(&ecb->evs_gsemaphore);
    ecb->evs_scount++;
    sce_munlock(&ecb->evs_gsemaphore);

    /*
    ** Check the active bit again, just in case the "other disposer" slipped
    ** in before we got the semaphore.  In this case decrement the server
    ** count (just in case we got in before the other one acquired the
    ** semaphore - just a consistency step) and assume the caller will recreate.
    */
    if (ecb->evs_status == EV_INACTIVE)
    {
	ecb->evs_scount--;
	sce_mlock(evc->evc_mem_sem);
	MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	sce_munlock(evc->evc_mem_sem);
	evc->evc_root = NULL;
	return (E_DB_OK);		/* OK & try again (evactive = FALSE) */
    }

    /*
    ** Check if EV is the correct version - this is really for development
    ** to allow multiple versions of EV to work in a single installation.
    */
    if (ecb->evs_version != EV_VERSION)
    {
	verserr = EV_VERSION;
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_scount--;
	sce_munlock(&ecb->evs_gsemaphore);
	(*evc->evc_errf)(E_SC0293_XSEV_VERSION, 0, 3,
			 sizeof(EV_SEGNAME)-1, EV_SEGNAME,
			 sizeof(ecb->evs_version), &ecb->evs_version,
			 sizeof(verserr), &verserr);
	sce_mlock(evc->evc_mem_sem);
	MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	sce_munlock(evc->evc_mem_sem);
	evc->evc_root = NULL;
	return (E_DB_FATAL);
    }

    /* Allocate space for a server entry */
    status = sce_eallocate(ecb, sizeof(EV_SVRENTRY), &soffset);
    if (status != E_DB_OK)
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_scount--;
	sce_munlock(&ecb->evs_gsemaphore);
	allocerr = sizeof(EV_SVRENTRY);
	(*evc->evc_errf)(E_SC028C_XSEV_EALLOCATE, 0, 3,
			 sizeof("EV_SVRENTRY")-1, "EV_SVRENTRY",
			 sizeof(allocerr), &allocerr,
			 sizeof(ecb->evs_favailable), &ecb->evs_favailable);
	sce_mlock(evc->evc_mem_sem);
	MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	sce_munlock(evc->evc_mem_sem);
	evc->evc_root = NULL;
	return (status);
    }
    /* Point at server entry through offset and initialize it */
    server = (EV_SVRENTRY *)((PTR)ecb + soffset);
    server->ev_svrnextoff = 0;		/* Nothing yet */
    server->ev_svrinstoff = 0;
    server->ev_svrpid = evc->evc_pid;
    server->ev_svrflags = EV_SVRINIT; /* This server hasn't completed its
				      ** initialisation yet. It hasn't acquired
				      ** the LK_EVCONNECT lock, and won't do this
				      ** until the SCS_SEVENT thread is running.
				      */

    /* If an all-event-registered server then allocate a new registration */
    if (evc->evc_flags & SCEV_REG_ALL)
    {
	status = sce_eallocate(ecb, sizeof(EV_RSERVER), &roffset);
	if (status != E_DB_OK)
	{
	    sce_efree(ecb, sizeof(EV_SVRENTRY), soffset);/* Free server entry */
	    sce_mlock(&ecb->evs_gsemaphore);
	    ecb->evs_scount--;
	    sce_munlock(&ecb->evs_gsemaphore);
	    allocerr = sizeof(EV_RSERVER);
	    (*evc->evc_errf)(E_SC028C_XSEV_EALLOCATE, 0, 3,
			     sizeof("EV_RSERVER")-1, "EV_RSERVER",
			     sizeof(allocerr), &allocerr,
			     sizeof(ecb->evs_favailable),&ecb->evs_favailable);
	    sce_mlock(evc->evc_mem_sem);
	    MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	    sce_munlock(evc->evc_mem_sem);
	    evc->evc_root = NULL;
	    return (status);
	}
	rptr = (EV_RSERVER *)((PTR)ecb + roffset);
	rptr->ev_rspid = evc->evc_pid;
	rptr->ev_rsflags = 0;
	rptr->ev_rsnextoff = 0;
    } /* If this is an all-event-server */

    /*
    ** Add to server instance hash chain.  The hash entry is calculated from
    ** the "instance array offset" plus the "number of hash buckets over".
    */
    sbucket = server->ev_svrpid % ecb->evs_ibuckets;
    shash = (EV_SVRHASH *)((PTR)ecb +
			   ecb->evs_ioffset + (sbucket * sizeof(EV_SVRHASH)));
    /* Lock the chain and stick on the front */
    sce_mlock(&shash->ev_shsemaphore);
    server->ev_svrnextoff = shash->ev_shoffset;
    shash->ev_shoffset = soffset;
    sce_munlock(&shash->ev_shsemaphore);

    /* If all-event link this registration to the "all" chain (in front) */
    if (evc->evc_flags & SCEV_REG_ALL)
    {
	sce_mlock(&ecb->evs_asemaphore);
	rptr->ev_rsnextoff = ecb->evs_alloffset;
	ecb->evs_alloffset = roffset;
	ecb->evs_acount++;
	sce_munlock(&ecb->evs_asemaphore);
    } /* If this is an all-event-server */

    *evactive = TRUE; 	/* It now may be used */

    return (E_DB_OK);
} /* sce_econnect */

/*{
** Name: sce_eregister	- Register to be signaled when an event occurs
**
** Description:
**	This routine allows a server to register with the event subsystem the
**	name of an event for which the server wishes to be signaled (probably
**	on behalf of one or more server threads).  A server that is registered
**	for all events may not register for a specific event.  That server
**	should have connected to EV with the SCEV_REG_ALL flag.
**
**	EV will maintain a single registration for each event for each server.
**	An error is returned if an attempt is made to register an already
**	registered event.  The server will be signaled of the occurrence of
** 	the event by driving LKevent for that server.
**
**	Algorithm:
**	    (P)search for/lock hash chain for registered event(ev_ehsemaphore);
**	    if server already registered then
**		error - server already registered;
**	    else
**		if event not on chain allocate event and link to hash chain;
**		alloc/init server event registration and link to event chain;
**	    endif
**	    (V)release lock on event hash chain(ev_ehsemaphore);
**	    (P)(V)if ok bump registration count under lock (evs_gsemaphore);
**
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of caller
**	  .evc_flags			If this is SCEV_REG_ALL then this
**					call is invalid.
**      event				Description of event to register:
**	  .ev_type			Type of event (diagnostic)
**	  .ev_lname			Length of event name
**	  .ev_name			Event name data
** Outputs:
**	error.err_code			Error is logged and returned to caller:
**	    E_SC028D_XSEV_REGISTERED	Event already [implicitly] registered
**	    E_SC028C_XSEV_EALLOCATE	Event allocation error
**	Returns:
**	    DB_STATUS
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_eregister( SCEV_CONTEXT *evc, SCEV_EVENT  *event, DB_ERROR  *error )
{
    EV_SCB		*ecb;	  /* Root of EV data structures */
    i4			ebucket;  /* Hash bucket corresponding to event */
    EV_EVHASH		*ehash;	  /* Fixed root node in event hash bucket */
    CS_SEMAPHORE	*eunlock; /* To unlock the event reg hash bucket */
    EV_REVENT		*eptr;	  /* New event to add into hash chain */
    i4		eoffset;  /* Offset into free space of new event node */
    EV_RSERVER		*rptr;	  /* New registration node to add to event */
    i4		roffset;  /* Offset into free space of new reg node */
    i4			allocerr;  /* For allocation errors */
    char		evname[EV_PRNAME_MAX];
    DB_STATUS		status;

    /*
    ** Find this event and lock the hash chain.  The finder will always
    ** set ebucket.  If eoffset if set then the actual event was found.
    ** If roffset is set then this server's registration is found too (error).
    */
    ecb = (EV_SCB *)evc->evc_root;
    event->ev_pid = evc->evc_pid;
    sce_erlock(ecb, event, &ebucket, &eunlock, &eoffset, &roffset);

    /* Point to start of hash chain & bump by bucket # to get to our cell */
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    ehash += ebucket;

    for (;;)			/* Break out on error */
    {
    	/* Server already registered for this (or ALL events) */
	if (roffset != 0 || (evc->evc_flags & SCEV_REG_ALL))
	{
	    sce_munlock(eunlock);
	    (*evc->evc_errf)(E_SC028D_XSEV_REGISTERED, 0, 1,
			     sce_ename(event->ev_lname, event->ev_name, evname),
			     evname);
	    error->err_code = E_SC028D_XSEV_REGISTERED;
	    status = E_DB_ERROR;
	    break;
	}

	if (eoffset != 0)	/* Someone's already registered for event */
	{
	    eptr = (EV_REVENT *)((PTR)ecb + eoffset);
	}
	else			/* No one's registered for this event yet */
	{
	    /* Create an event entry for this event and deposit into bucket */
	    status = sce_eallocate(ecb, sizeof(EV_REVENT), &eoffset);
	    if (status != E_DB_OK)
	    {
		sce_munlock(eunlock);
		allocerr = sizeof(EV_REVENT);
		(*evc->evc_errf)(E_SC028C_XSEV_EALLOCATE, 0, 3,
			     sizeof("EV_REVENT")-1, "EV_REVENT",
			     sizeof(allocerr), &allocerr,
			     sizeof(ecb->evs_favailable), &ecb->evs_favailable);
		error->err_code = E_SC028C_XSEV_EALLOCATE;
		status = E_DB_ERROR;
		break;
	    }

	    /* Easier to work in pointers.  Fill in the new event entry */
	    eptr = (EV_REVENT *)((PTR)ecb + eoffset);
	    eptr->ev_retype = event->ev_type;
	    MEcopy((PTR)event->ev_name, event->ev_lname, (PTR)eptr->ev_rename);
	    eptr->ev_relname = event->ev_lname;
	    eptr->ev_resvroff = 0;		/* No registered servers yet */

	    /* Link to the hash bucket node (stack in front) */
	    eptr->ev_renextoff = ehash->ev_ehoffset;
	    ehash->ev_ehoffset = eoffset;
	} /* If we had to create event or not */

	/* Now we have to enter the new server registration */
	status = sce_eallocate(ecb, sizeof(EV_RSERVER), &roffset);
	if (status != E_DB_OK)
	{
	    sce_munlock(eunlock);
	    allocerr = sizeof(EV_RSERVER);
	    (*evc->evc_errf)(E_SC028C_XSEV_EALLOCATE, 0, 3,
			     sizeof("EV_RSERVER")-1, "EV_RSERVER",
			     sizeof(allocerr), &allocerr,
			     sizeof(ecb->evs_favailable), &ecb->evs_favailable);
	    error->err_code = E_SC028C_XSEV_EALLOCATE;
	    status = E_DB_ERROR;
	    break;
	}

	/* Easier to work in pointers.  Fill in with caller values */
	rptr = (EV_RSERVER *)((PTR)ecb + roffset);
	rptr->ev_rspid = event->ev_pid;
	rptr->ev_rsflags = 0;

	/* Link this registration to the event chain (stack in front) */
	rptr->ev_rsnextoff = eptr->ev_resvroff;
	eptr->ev_resvroff = roffset;

	/* Release the registration bucket for this event before returning */
	sce_munlock(eunlock);
	status = E_DB_OK;
	break;
    } /* Dummy for loop */

    if (status == E_DB_OK)	/* Bump global registration count */
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_rcount++;
	sce_munlock(&ecb->evs_gsemaphore);
    }
    return (status);
} /* sce_eregister */

/*{
** Name: sce_ederegister	- Remove a server event registration
**
** Description:
**	Allow a server to remove an event registration. The server will no
**	longer be signalled by the event subsystem if the named event occurs.
**	This routine allows a server to deregister with the event subsystem the
**	name of an event for which the server is already registered.
**
**	An error is returned if an attempt is made to deregister an event
**	that is not registered.
**
**	Algorithm:
**	    (P)search for/lock hash chain for registered event(ev_ehsemaphore);
**	    if server not registered then
**		error - server not registered;
**	    else
**		remove/free server event registration from event chain;
**		if event chain is empty then unlink event from hash chain;
**	    endif
**	    (V)release lock on event hash chain(ev_ehsemaphore);
**	    (P)(V)if ok decrement reg count under lock (evs_gsemaphore);
**
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of caller
**      event				Description of event to deregister:
**	  .ev_type			Type of event (diagnostic)
**	  .ev_lname			Length of event name
**	  .ev_name			Event name data
** Outputs:
**	error.err_code			Error is logged and returned to caller:
**	    E_SC028E_XSEV_NOT_REGISTERED	Event not registered
**	Returns:
**	    DB_STATUS
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_ederegister( SCEV_CONTEXT  *evc, SCEV_EVENT	*event, DB_ERROR  *error )
{
    EV_SCB	*ecb;		/* Root of EV data structures */
    i4		ebucket;	/* Hash bucket corresponding to event */
    EV_EVHASH	*ehash;		/* Fixed root node in event hash bucket */
    CS_SEMAPHORE *eunlock; 	/* To unlock the event reg hash bucket */
    EV_REVENT	*eptr;		/* Event may be removed from hash chain */
    i4	eoffset;	/* Offset into free space of old event node */
    EV_RSERVER	*rptr;		/* Registration node to scan for removal */
    i4	roffset;	/* Offset into free space of old reg node */
    char	evname[EV_PRNAME_MAX];
    DB_STATUS	status;

    /*
    ** Find this event and lock the hash chain.  The finder will always
    ** set ebucket.  If eoffset if set then the actual event was found.
    ** If roffset is set then this server's registration is found too (error).
    */
    ecb = (EV_SCB *)evc->evc_root;
    event->ev_pid = evc->evc_pid;
    sce_erlock(ecb, event, &ebucket, &eunlock, &eoffset, &roffset);

    /* Point to start of hash chain & bump by bucket # to get to our cell */
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    ehash += ebucket;

    for (;;)			/* Break on error */
    {
	if (roffset == 0)	/* Server registration not found */
	{
	    sce_munlock(eunlock);
	    (*evc->evc_errf)(E_SC028E_XSEV_NOT_REGISTERED, 0, 1,
			     sce_ename(event->ev_lname, event->ev_name, evname),
			     evname);
	    error->err_code = E_SC028E_XSEV_NOT_REGISTERED;
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** Remove the server registration entry.  If this is the only
	** registration for this event then remove the event as well.
	*/
	eptr = (EV_REVENT *)((PTR)ecb + eoffset);	/* Work in pointers */
	rptr = (EV_RSERVER *)((PTR)ecb + roffset);
	sce_erunlink(ecb, eptr, rptr);
	if (eptr->ev_resvroff == 0)
	    sce_evunlink(ecb, ehash, eptr);

	/* Release the bucket containing the event before returning */
	sce_munlock(eunlock);
	status = E_DB_OK;
	break;
    } /* End of dummy for loop */

    if (status == E_DB_OK)	/* Decrement global registration count */
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_rcount--;
	sce_munlock(&ecb->evs_gsemaphore);
    }
    return (status);
} /* sce_deregister */

/*{
** Name: sce_esignal	- Signal the occurrence of an event
**
** Description:
**	This routine is called by a server to signal the occurrence of an event
**	to all servers registered for that event.  The event is found on the
**	event hash array and all servers registered for that event have an
**	event instance added to them and then are signaled.  Note that servers
**	registered for all events are also notified.
**
**	Events are signaled by name and an optional data block containing
**	event specific information may be included.  This data block is
**	uninterpreted by the event subsystem.
**
**	The event signal is transmitted to each registered server through
**	LKevent (each server will have an event thread waiting for LKevent).
**	Note that event names must be unique within an installation.
**
**	Within the main loop of this routine we may be holding 2 semaphores.
**	Some special error handling is done so that we do not report errors
**	(logs are I/O operations) while holding semaphores.
**
**	Algorithm:
**	    (P)find/lock event hash chain (evs_ehsemaphore);
**	    if the event was found the
**		for each server registered for the event
**		    (P)find/lock the server instance queue (ev_shsemaphore);
**		    allocate/add an event instance to the server queue;
**		    bump instance count/total;
**		    (V)free the server instance queue (ev_shsemaphore);
**		    call LKevent for that server id;
**		endloop
**	    endif
**	    (P)unlock event hash chain (evs_ehsemaphore);
**
**	    (P)lock all-server registration list (evs_asemaphore);
**	    for all servers on all-event-register list
**		do the same as above;
**	    endloop
**	    (V)unlock all-server registration list (evs_asemaphore);
**
**	    if event not found
**		bump failed count (under ev_gsemaphore lock);
**	    endif
**
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of caller
**      event				Description of event to signal:
**	  .ev_type			Type of event (diagnostic)
**	  .ev_lname			Length of event name
**	  .ev_ldata			Length of event data
**	  .ev_name			Event name data.  May include user data
**					if ev_ldata > 0.
** Outputs:
**	error.err_code			All errors are logged, err_code = first:
**	    E_SC028C_XSEV_EALLOCATE	Allocation of instance failed
**	    E_SC028F_XSEV_NO_SERVER	Event-registered server is not connected
**	    E_SC0290_XSEV_SIGNAL	LKevent could not be signaled
**	    E_SC029B_XSEV_REQLOCK	Detected phantom server. No status ret.
**					Target server is disaabled.
**	    E_SC029E_XSEV_TSTLOCK	Error in sce_edump() during phantom
**					server detection.  No status returned.
**					Target server is disaabled.
**	    E_SC029C_XSEV_RLSLOCK	Error releasing lock during phantom
**					server detection.  No status returned.
**	Returns:
**	    DB_STATUS
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	10-jan-91 (neil)
**	    Event instance allocation is now varying length.
**	16-aug-91 (ralph)
**	    Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_esignal() attempts to get the target server's LK_EVCONNECT
**		lock before queueing an event instance.  If it can get the
**		lock, the target server is disabled.
**	16-sep-91 (ralph)
**	    Don't issue E_SC029D_XSEV_PREVERR when server already disabled.
**		Don't return failure status when phantom or disabled server
**		is detected.
**	2-Jul-1993 (daveb)
**	    remove unused var 'lkflags'.  prototyped.
**      23-jul-1998 (rigka01)
**          When event flag indicates that event shouldn't be
**          sent to the originating server, then don't put it on
**          that server's event queue
**      23-jul-1998 (rigka01)
**	  cross integrate fix for bug 91773 made by hayke01:
**          Modification to last change (fix for bug 90692). We now
**          call sce_efree() to free up the allocated event memory
**          before breaking (when the event flag indicates that event
**          shouldn't be sent to the originating server). This prevents
**          E_SC028C and E_RD018A. This change fixes bug 91773.
**	16-Mar-2006 (wanfr01)
**	    SIR 115849
**	    Add a TRdisplay if sce_eallocate returns a fail code
**      20-Oct-2006 (horda03) Bug 116912
**          Note added to inform that an event should not be sent to a server which
**          hasn't completed its initialisation yet.
**	08-Sep-2008 (jonj)
**	    Fix mis-typed "timeout" param to LKrequest from NULL to 0.
*/

DB_STATUS
sce_esignal( SCEV_CONTEXT *evc, SCEV_EVENT  *event, DB_ERROR  *error )
{
    EV_SCB		*ecb;		/* Root of EV data structures */
    i4		    	ebucket;	/* Bucket in which event was found */
    CS_SEMAPHORE	*eunlock; 	/* To unlock the event hash bucket */
    i4	    	eoffset;	/* Offset of event found */
    EV_REVENT		*eptr;		/* Name of event being signaled */
    EV_RSERVER		*rptr;		/* Ptr to server registered for event */
    i4		    	sbucket;	/* Bucket of registered server */
    CS_SEMAPHORE	*sunlock; 	/* To unlock the server hash bucket */
    i4	    	soffset;	/* Offset of server entry */
    EV_SVRENTRY		*server;	/* To whose queue to add instance */
    EV_INSTANCE   	*instance;	/* Event instance to add */
    i4			inst_alloc;	/* Allocation size for instance */
    i4	    	inst_offset;	/* Offset of new instance */
    EV_INSTANCE   	*iprev;		/* Previous instance to queue onto */
    i4	    	iprev_offset;	/* Offset to modify */
    bool		ifail = TRUE;	/* Instance was actually used? */
    bool		tried_all_reg;	/* Set to indicate tried ALL list */
    bool		was_error = FALSE;
    i4			allocerr = 0;	/* For allocation errors */
#   define		ESIG_MAX_ERR 10	/* Maximum number of errors in here.
					** Stash error info so that we do not
					** have to report errors while holding
					** semaphores.  Reported when we drop
					** out of signalling loop.
					*/
    i4			err;		/* Number of errors reported */
    PID			orphan_err[ESIG_MAX_ERR];	/* For EV_RSORPHAN */
    i4			orphan_nerrs = 0;
    CL_ERR_DESC		lkclerror;
    char		evname[EV_PRNAME_MAX];
    DB_STATUS		status;
    STATUS	    	clstat;

    PID			lk_errpid[ESIG_MAX_ERR];      /* For LK errors */
    i4		lk_errmsg[ESIG_MAX_ERR];
    i4			lk_errcnt = 0;
    LK_LOCK_KEY         lkkey;
    LK_LKID             lkid;
    u_i4               lkmode = LK_S;
    PID                 spid;           /* Current servers id */
    LK_EVENT		lk_event;

    spid = evc->evc_pid;
    lkkey.lk_type = LK_EVCONNECT;
    lkkey.lk_key2 = 0;
    lkkey.lk_key3 = 0;
    lkkey.lk_key4 = 0;
    lkkey.lk_key5 = 0;
    lkkey.lk_key6 = 0;
    MEfill(sizeof(LK_LKID), 0, &lkid);

    /*
    ** Find and lock the event that is being signaled.  By setting the 
    ** registration pointer to NULL we we indicate that we aren't interested
    ** in a particular server registration but all of them.
    */
    ecb = (EV_SCB *)evc->evc_root;
    event->ev_pid = evc->evc_pid;
    sce_erlock(ecb, event, &ebucket, &eunlock, &eoffset, (i4 *)NULL);

    eptr = NULL;
    rptr = NULL;
    if (eoffset != 0)			/* Maybe someone's registered */
    {
        eptr = (EV_REVENT *)((PTR)ecb + eoffset);
	if (eptr->ev_resvroff != 0)	/* We got someone */
	    rptr = (EV_RSERVER *)((PTR)ecb + eptr->ev_resvroff);
    }

    /* If no normal servers registered then try the list of all-event-reg */
    tried_all_reg = FALSE;
    if (rptr == NULL)
    {
	sce_munlock(eunlock);
	tried_all_reg = TRUE;		/* Now try ALL chain */
	sce_mlock(&ecb->evs_asemaphore);
	if (ecb->evs_alloffset != 0)	/* There's someone here */
	    rptr = (EV_RSERVER *)((PTR)ecb + ecb->evs_alloffset);
	else
	    sce_munlock(&ecb->evs_asemaphore);
    }

    /* Calculate varying length size of this instance */
    inst_alloc = sizeof(EV_INSTANCE) + event->ev_lname + event->ev_ldata;

    /*
    ** Loop through each registered server and attach an event instance to
    ** that server's instance queue.  This loop does some special error
    ** stashing so that we do not try to report errors while holding on 
    ** semaphores.
    */
    status = E_DB_OK;
    while (rptr != NULL)
    {
	/* Allocate an event instance entry for this event */
	status = sce_eallocate(ecb, inst_alloc, &inst_offset);
	if (status != E_DB_OK)
	{
	    TRdisplay ("%@  ifail sce_eallocate failure\n");
	    if (!tried_all_reg)
		sce_munlock(eunlock);		     /* Unlock reg event hash */
	    else
		sce_munlock(&ecb->evs_asemaphore);   /* Unlock ALL event hash */
	    if (!was_error)
	    {
		was_error = TRUE;
		error->err_code = E_SC028C_XSEV_EALLOCATE;
	    }
	    allocerr = inst_alloc;
	    break;		/* Useless to try more registered servers */
	}
	instance = (EV_INSTANCE *)((PTR)ecb + inst_offset);

	/* Copy the signaled event to this instance entry */
	instance->ev_inpid = event->ev_pid;	/* With "originator" of event */
	instance->ev_intype = event->ev_type;
	instance->ev_inlname = event->ev_lname;
	MEcopy((PTR)event->ev_name, event->ev_lname, (PTR)instance->ev_inname);
	if ((instance->ev_inldata = event->ev_ldata) > 0)
	{
	    MEcopy((PTR)(event->ev_name + event->ev_lname), event->ev_ldata,
		   (PTR)(instance->ev_inname + instance->ev_inlname));
	}
	instance->ev_innextoff = 0;		/* Disconnect it */

	/*
	** Now find the server entry for this registered server.  This entry
	** was created when the server connected to EV.  If it does not exist
	** then this server disconnected with registrations intact.
	*/
	sce_eslock(ecb, rptr->ev_rspid, &sbucket, &sunlock, &soffset);

	if (soffset == 0)		/* Server not found */
	{
	    /*
	    ** The action will be to remove any registrations for this server.
	    ** The occurrence of this event will be logged but no error will
	    ** be returned. Mark this server as an orphan so we can ignore
	    ** it later.  This registration is cleaned up in sce_edisconnect.
	    */
	    if ((rptr->ev_rsflags & EV_RSORPHAN) == 0)
	    {
		if (orphan_nerrs < ESIG_MAX_ERR)	/* Stash error */
		    orphan_err[orphan_nerrs++] = rptr->ev_rspid;
		rptr->ev_rsflags |= EV_RSORPHAN;
		if (!was_error)
		{
		    was_error = TRUE;
		    error->err_code = E_SC028F_XSEV_NO_SERVER;
		}
	    }
	    /* Free instance so we don't lose it, and continue with loop */
	    sce_efree(ecb, inst_alloc, inst_offset);
	}
	else				/* Server was found */
	for (;;)
	{
	    /*
	    ** Link the event instance to the collection of events awaiting
	    ** processing by this server. Link them in the order they arrived
	    ** so that the server can pick them off from the top of the list.
	    */
	    server = (EV_SVRENTRY *)((PTR)ecb + soffset);

	    /* If sce_dump had problems with this lock, report it here */
	    if (server->ev_svrflags & EV_SVRTSERR)
	    {
		if (lk_errcnt < ESIG_MAX_ERR)   /* Stash eror */
		{
		    lk_errpid[lk_errcnt] = server->ev_svrpid;
			/*
			** Error attempting to release the LK_EVCONNECT
			** lock was previously encountered.  Translate
			** this to EV_SVRRQERR, and issue message
			** E_SC029E_XSEV_TSTLOCK.  This situation
			** may have been caused by probes in sce_edump.
			*/
			server->ev_svrflags ^= EV_SVRTSERR;
			server->ev_svrflags |= EV_SVRRQERR;
			lk_errmsg[lk_errcnt++] = E_SC029E_XSEV_TSTLOCK;
		}
                if (!was_error)
                {
                    was_error = TRUE;
                    error->err_code = E_SC029E_XSEV_TSTLOCK;
		}
	    }

          /* If we've already had problems with this server, skip it.
          ** If the target server is the alert originator and the event
          ** flag indicates that alerts should not be sent when this is
          ** the case, then don't send the alert to the target server.
	  ** If the EV_SVRINIT flag is set, then the server hasn't completed
	  ** its initialisation, so it hasn't requested its LK_EVCONNECT lock
	  ** yet - so don't bother adding the event.
	  */
          if (server->ev_svrflags ||
              ((server->ev_svrpid == evc->evc_pid)
                  && (event->ev_flags & EV_NOORIG)))
          {
              /* Free instance so we don't lose it, and continue */
              sce_efree(ecb, inst_alloc, inst_offset);
              break;
          }

	    /* Ensure the target server is still alive */
	    if (server->ev_svrpid != spid)
	    {
		lkkey.lk_key1 = server->ev_svrpid;
		clstat = LKrequest((LK_PHYSICAL | LK_NOWAIT),
				   evc->evc_locklist, &lkkey, lkmode,
				   NULL, &lkid, 0, &lkclerror);
		if (clstat == OK)
		{
		    /*
		    ** Since we can get the server's LK_EVCONNECT lock,
		    ** the server must have died without disconnecting.
		    */
                    clstat = LKrelease((i4)0, evc->evc_locklist,
				       &lkid, &lkkey, NULL, &lkclerror);
		    if (lk_errcnt < ESIG_MAX_ERR)   /* Stash error */
		    {
			lk_errpid[lk_errcnt] = server->ev_svrpid;
			lk_errmsg[lk_errcnt++] = E_SC029B_XSEV_REQLOCK;
                    }
                    if ((clstat) &&
			(lk_errcnt < ESIG_MAX_ERR))
                    {
			/* We were unable to release the LK_EVCONNECT lock */
                        lk_errpid[lk_errcnt] = server->ev_svrpid;
                        lk_errmsg[lk_errcnt++] = E_SC029C_XSEV_RLSLOCK;
		    }
		    if (!was_error)
		    {
			was_error = TRUE;
			error->err_code = E_SC029B_XSEV_REQLOCK;
		    }
		    server->ev_svrflags |= EV_SVRRQERR;
		    /* Free instance so we don't lose it, and continue */
		    sce_efree(ecb, inst_alloc, inst_offset);
		    break;
		}
	    }

	    /* Link the new event instance to the last one */
	    iprev_offset = server->ev_svrinstoff;
	    iprev = (EV_INSTANCE *)((PTR)ecb + iprev_offset);
	    if (iprev_offset == 0)		/* First on the instance list */
	    {
		/* First instance for this server */
		server->ev_svrinstoff = inst_offset;
	    }
	    else				/* Walk to end of list */
	    {
		while (iprev_offset != 0)
		{
		    iprev_offset = iprev->ev_innextoff;
		    if (iprev_offset != 0)
			iprev = (EV_INSTANCE *)((PTR)ecb + iprev_offset);
		}
		iprev->ev_innextoff = inst_offset;
	    }

	    /*
	    ** Signal the server that an event instance has been queued.
	    ** Global locks are used to signal each server.
	    */
	    lk_event.type_high = 0;
	    lk_event.type_low = 0;
	    lk_event.value = server->ev_svrpid;
	    
	    clstat = LKevent((LK_E_CLR | LK_E_CROSS_PROCESS),
			     evc->evc_locklist, &lk_event, &lkclerror);
	    if (clstat != OK)
	    {
		/*
		** We are unable to signal the indicated server - log it.
		** Mark this server as an LKevent-error recipient.  Maybe
		** the server died and is still attached.
		*/
		if (lk_errcnt < ESIG_MAX_ERR)   /* Stash error */
		{
		    lk_errpid[lk_errcnt] = server->ev_svrpid;
		    lk_errmsg[lk_errcnt++] = E_SC0290_XSEV_SIGNAL;
		}
		if (!was_error)
		{
		    was_error = TRUE;
		    error->err_code = E_SC0290_XSEV_SIGNAL;
		}
		server->ev_svrflags |= EV_SVRLKERR;
		/* Free instance so we don't lose it, and continue with loop */
		sce_efree(ecb, inst_alloc, inst_offset);
	    }
	    else 			/* OK, so bump instance count/totals */
	    {
		sce_mlock(&ecb->evs_gsemaphore);
		ecb->evs_icount++;
		ecb->evs_isuccess++;
		sce_munlock(&ecb->evs_gsemaphore);
		ifail = FALSE;
	    }

	    break;  /* Exit "for" block */

	} /* If server was found or not */
	sce_munlock(sunlock);

	if (rptr->ev_rsnextoff != 0)		/* Next registration */
	{
	    rptr = (EV_RSERVER *)((PTR)ecb + rptr->ev_rsnextoff);
	}
	else					/* Current list is done */
	{
	    /* If at end of normal registration list try ALL list, else done */
	    rptr = NULL;		 /* May need to terminate outer loop */

	    /* Try the chain of all-event-registered servers before ending */
	    if (!tried_all_reg)
	    {
		sce_munlock(eunlock);		/* Unlock event hash chain */
		tried_all_reg = TRUE;		/* Now try ALL chain */
		sce_mlock(&ecb->evs_asemaphore);
		if (ecb->evs_alloffset != 0)	/* There's someone here */
		    rptr = (EV_RSERVER *)((PTR)ecb + ecb->evs_alloffset);
		else
		    sce_munlock(&ecb->evs_asemaphore);
	    }
	    else				/* Done with both chains */
	    {
		sce_munlock(&ecb->evs_asemaphore);
						/* Unlock ALL event hash */
	    }
	}
    } /* While more registered servers */

    /* Error information was stashed earlier */
    if (was_error)
    {
	if (allocerr)
	{
	    status = E_DB_ERROR;    /*@FIX_ME@ - should this abort? */
	    (*evc->evc_errf)(E_SC028C_XSEV_EALLOCATE, 0, 3,
			     sizeof("EV_INSTANCE")-1, "EV_INSTANCE",
			     sizeof(allocerr), &allocerr,
			     sizeof(ecb->evs_favailable), &ecb->evs_favailable);
	}
	for (err = 0; err < orphan_nerrs; err++)
	{
	    status = E_DB_ERROR;    /*@FIX_ME@ - should this abort? */
	    (*evc->evc_errf)(E_SC028F_XSEV_NO_SERVER, 0, 1,
			     sizeof(PID), &orphan_err[err]);
	}
	for (err = 0; err < lk_errcnt; err++)
	{
	    /* Note -- don't set status, as this will abort the stmt */
	    (*evc->evc_errf)(lk_errmsg[err], &lkclerror, 2,
			     sizeof(PID), &lk_errpid[err],
			     sce_ename(event->ev_lname, event->ev_name, evname),
			     evname);
	}
    }
    /* Track events that are "lost in space" - no one to attach to */
    if (ifail)
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_ifail++;
	sce_munlock(&ecb->evs_gsemaphore);
    }
    return (status);
} /* sce_esignal */

/*{
** Name: sce_efetch	- Server request for outstanding instances
**
** Description:
**	When a server is signaled through LK that an event has occurred, this
**	routine is called to obtain the event data being broadcast.
**
**	The server may choose to fetch as many events as are currently queued
**	for the server, and because it may consume events that arrive because
**	of subsequent signals (LKevent), it must also be prepared to deal with
**	an empty list of events.  When no events are oustanding this routine
**	will return an end of list indication.
**
**	The caller must include pointers to event name and event data buffers
**	large enough to hold the largest event data block.
**
**	Algorithm:
**	    (P)find/lock server instance entry (ev_shsemaphore);
**	    if no more entries mark end-of_events;
**	    else pick off first event, copy it and deallocate;
**	    (V)unlock server instance entry (ev_shsemaphore);
**
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of recipient.
**      event				Pointer to event to fetch.
**
** Outputs:
**      event				Event being fetched:
**	  .ev_pid			Server id of "originating" notifier
**	  .ev_type			Type of event
**	  .ev_lname			Length of event name
**	  .ev_ldata			Length of event data (at least the date)
**	  .ev_name			Event name data.  May include user data
**					if ev_ldata > 0.  This data block must
**					be >= SCEV_BLOCK_MAX size.
**	end_of_events			Set to TRUE if there are no more events.
**					Return status is still E_DB_OK.
**	error.err_code
**	    E_SC0291_XSEV_NOT_CONNECT	Server is not connected to EV
**	Returns:
**	    E_DB_OK 			All OK even if no events
**	    E_DB_ERROR			Server should disconnect from EV
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	10-jan-91 (neil)
**	    Event instance allocated memory is now varying length.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_efetch(SCEV_CONTEXT *evc,
	   SCEV_EVENT *event,
	   i4  *end_of_events,
	   DB_ERROR *error )
{
    EV_SCB		*ecb;		/* Root of EV data structures */
    i4		    	sbucket;	/* Bucket of requesting server */
    CS_SEMAPHORE	*sunlock; 	/* To unlock the server hash bucket */
    i4	    	soffset;	/* Offset of server entry */
    EV_SVRENTRY		*server;	/* From whose queue to get instance */
    EV_INSTANCE   	*instance;	/* Event instance to return */
    i4			inst_alloc;	/* Allocation size for instance */
    i4	    	inst_offset;	/* Offset of returned instance */
    DB_STATUS		status;

    /* Find (and lock) the correct server event instance list */
    ecb = (EV_SCB *)evc->evc_root;
    sce_eslock(ecb, evc->evc_pid, &sbucket, &sunlock, &soffset);

    /* Remove the next instance from this server's list and return to caller */
    *end_of_events = FALSE;				/* Assume more events */
    inst_offset = 0;					/* Checked later */
    for (;;)						/* Break on error */
    {
	if (soffset == 0)			/* Server not found */
	{
	    sce_munlock(sunlock);
	    (*evc->evc_errf)(E_SC0291_XSEV_NOT_CONNECT, 0, 2,
			     sizeof(evc->evc_pid), &evc->evc_pid,
			     sizeof("sce_efetch")-1, "sce_efetch");
	    error->err_code = E_SC0291_XSEV_NOT_CONNECT;
	    status = E_DB_ERROR;		/* Caller should disconnect */
	    break;
	}

	server = (EV_SVRENTRY *)((PTR)ecb + soffset);
	if (server->ev_svrinstoff == 0)		/* Server has no events */
	{
	    sce_munlock(sunlock);
	    *end_of_events = TRUE;
	    status = E_DB_OK; 			/* Not an error */
	    break;
	}

	/* Pick off the event and hand back to caller */
	inst_offset = server->ev_svrinstoff;
	instance = (EV_INSTANCE *)((PTR)ecb + inst_offset);

	server->ev_svrinstoff = instance->ev_innextoff;		/* Disconnect */

	/* Copy this event to the user space before deallocating */
	event->ev_pid = instance->ev_inpid;	/* Original notifier */
	event->ev_type = instance->ev_intype;
	event->ev_lname = instance->ev_inlname;
	event->ev_ldata = instance->ev_inldata;
	MEcopy((PTR)instance->ev_inname,
	       instance->ev_inlname + instance->ev_inldata,
	       (PTR)event->ev_name);

	/* Disconnect and deallocate the instance */
	instance->ev_innextoff = 0;
	/* Get varying size of this instance before deallocation */
	inst_alloc = sizeof(EV_INSTANCE) +
		     instance->ev_inlname + instance->ev_inldata;
	sce_efree(ecb, inst_alloc, inst_offset);
	sce_munlock(sunlock);
	status = E_DB_OK;
	break;
    } /* End of dummy for loop */

    if (status == E_DB_OK && inst_offset != 0)
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_icount--;
	sce_munlock(&ecb->evs_gsemaphore);
    }
    return (status);
} /* sce_efetch */

/*{
** Name: sce_edisconnect	- Disconnect server from event subsystem
**
** Description:
**      This call disconnects a particular server from EV.  To disconnect
**	from EV we must remove the server entry (and any dangling instances).
**	Server registrations are also removed by EV, in case they are not
**	removed by the server before it shuts down.  On the disconnection of
**	the last server EV shuts itself down.
**
**	To reduce the possibility of orphaned event registrations appearing
**	we first delete all registrations that apply to the server (if
**	we did this after deleting the server entry then we could end up with
**	orphans because other servers are trying to notify our disconnecting
** 	server).   Orphans are caught and marked in sce_esignal.
**
**	Algorithm:
**	    for each event hash
**		(P)lock event has chain (ev_ehsemaphore);
**		for each registeration in chain
**		    if same server id or registration = orphan
**			remove registration;
**		    endif;
**		endloop;
**		(V)unlock event has chain (ev_ehsemaphore);
**	    endloop;
**
**	    (P)search for/lock hash chain for server entry(ev_shsemaphore);
**	    if server not found then
**		error - server not connected;
**	    else
**	        if disconnecting external server then
**	            send LKevent signal (to clear out it's event thread);
**	        endif;
**		remove/free all event instances;
**		remove/free server entry;
**	    endif
**	    (V)release lock on server hash chain(ev_shsemaphore);
**
**	    if all-event-register then
**		(P)lock all-server registration list (evs_asemaphore);
**		free all-event-server registration, decrement count;
**		(V)unlock all-server registration list (evs_asemaphore);
**	    endif
**
**	    (P)lock global semaphore (ev_gsemaphore);
**	    decrement server & instance counts
**	    if server count = 0 then
**		inactivate EV;
**		(V)unlock global lock (ev_gsemaphore);
**		free shared segment;
**		destroy EV;
**	    else
**		(V)unlock global lock (ev_gsemaphore);
**		free shared segment;
**	    endif;
**
** Inputs:
**	evc			EV request context structure:
**	    .evc_pid		Process ID of disconnecting server
**	    .evc_flags		Modifiers for disconnection:
**				SCEV_REG_ALL implies that this server was
**				an all-event-register server.
**			   	SCEV_DISCON_EXT implies that an external server
**				(i.e. not the caller's process) is being
**				disconnected from EV (useful for cleanup).
** Outputs:
**	evc			Caller event data structures:
**	    .evc_root		Reset to NULL.
**	    .evc_locklist	Reset to NULL.
**	Returns:
**	    DB_STATUS
**	Errors:
**	    Errors found during processing are logged in the error log:
**	    E_SC0291_XSEV_NOT_CONNECT	Server is not connected to EV
**	    E_SC0288_XSEV_SM_DESTROY	Cannot destroy shared EV segment
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	10-jan-91 (neil)
**	    Event instance allocated memory is now varying length.
**	25-jan-91 (neil)
**	    1. Serialize calls to ME page [de]allocation through evc_mem_sem.
**	    2. Allow caller to disconnect another server (for cleanup).
**	18-aug-91 (ralph)
**	    sce_edisconnect() Remove lock list if not external disconnect.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      06-aug-1997 (canor01)
**          Remove semaphores before freeing memory.
**	24-Oct-1997 (jenjo02)
**	    Don't remove semaphores if the memory segment is not going
**	    to be destroyed; subsequent servers connecting to that
**	    memory expect it to be properly initialized.
*/
DB_STATUS
sce_edisconnect( SCEV_CONTEXT	*evc )
{
    EV_SCB		*ecb;		/* Root of EV structures */
    i4			ebucket;	/* Bucket to traverse registrations */
    EV_EVHASH		*ehash;		/* Hash cell in bucket */
    EV_REVENT		*eptr;		/* Event to check for removal */
    i4		eoffset;	/* Offset of current event */
    EV_RSERVER		*rptr;		/* [ALL] registration to check */
    i4		roffset;	/* Offset of current registration */
    EV_RSERVER		*rprev;		/* Previous reg entry (if for ALL) */
    i4		rprev_offset;	/* Offset of previous reg (for ALL) */
    i4			reg_remove;	/* Counter of registrations removed */
    i4			sbucket;	/* Bucket from which to remove server */
    EV_SVRHASH		*shash;		/* Server hash node in bucket */
    CS_SEMAPHORE	*sunlock; 	/* To unlock the server hash bucket */
    EV_SVRENTRY		*server;	/* Server entry to remove */
    i4		soffset;	/* Offset of server entry */
    EV_SVRENTRY		*sprev;		/* Previous server to patch links */
    i4		sprev_offset;	/* Offset of previous-linked server */
    EV_INSTANCE   	*instance;	/* Instance list to walk/free */
    i4			inst_alloc;	/* Allocation size for instance */
    i4	    	inst_offset; 	/* Instance offset for deallocation */
    i4			inst_remove;	/* Counter of instances removed */
    char		smnm[EV_SEGSIZE+1];/* EV shared memory name */
    CL_ERR_DESC		clerror;
    STATUS              clstat;
    bool		lkerr;
    i4                  bkt;
    DB_STATUS		status;
    LK_EVENT		lk_event;

    ecb = (EV_SCB *)evc->evc_root;

    /* Remove all server event registrations and orphans */
    reg_remove = 0;
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    for (ebucket = 0; ebucket < ecb->evs_rbuckets; ebucket++, ehash++)
    {
	if (ehash->ev_ehoffset == 0)			/* No events */
	    continue;

	sce_mlock(&ehash->ev_ehsemaphore);		/* lock event chain */

	eoffset = ehash->ev_ehoffset;
	while (eoffset != 0)			/* Walk all event on chain */
	{
	    eptr = (EV_REVENT *)((PTR)ecb + eoffset);
	    /* Save next offset in case this event is unlinked */
	    eoffset = eptr->ev_renextoff;
	    if (eptr->ev_resvroff != 0)
	    {
		roffset = eptr->ev_resvroff;
		while (roffset != 0)		/* Walk all reg's for event */
		{
		    /* Save next offset in case this registration is unlinked */
		    rptr = (EV_RSERVER *)((PTR)ecb + roffset);
		    roffset = rptr->ev_rsnextoff;
		    /* Remove is our server or an orphan */
		    if (   (rptr->ev_rspid == evc->evc_pid)
			|| (rptr->ev_rsflags & EV_RSORPHAN))
		    {
			sce_erunlink(ecb, eptr, rptr);
			reg_remove++;
		    }
		    rptr = (EV_RSERVER *)((PTR)ecb + roffset);
		}
	    }
	    if (eptr->ev_resvroff == 0)			/* Toss empty event */
		sce_evunlink(ecb, ehash, eptr);
	    eptr = (EV_REVENT *)((PTR)ecb + eoffset);
	}
	sce_munlock(&ehash->ev_ehsemaphore);
    } /* End of event registration tables */
    sce_mlock(&ecb->evs_gsemaphore);
    ecb->evs_rcount -= reg_remove;
    sce_munlock(&ecb->evs_gsemaphore);

    /* Remove all outstanding event instances for this server */
    for (;;)						    /* Break on error */
    {
	sce_eslock(ecb, evc->evc_pid, &sbucket, &sunlock, &soffset);
	if (soffset == 0)		/* No server by that id */
	{
	    sce_munlock(sunlock);
	    (*evc->evc_errf)(E_SC0291_XSEV_NOT_CONNECT, 0, 2,
			     sizeof(evc->evc_pid), &evc->evc_pid,
			     sizeof("sce_edisconnect")-1, "sce_edisconnect");
	    status = E_DB_ERROR;
	    break;
	}
	if (evc->evc_flags & SCEV_DISCON_EXT)	/* External disconnect */
	{
	    /* Allow process to release its event thread from wait state */
	    lk_event.type_high = 0;
	    lk_event.type_low = 0;
	    lk_event.value = evc->evc_pid;

	    clstat = LKevent((LK_E_CLR | LK_E_CROSS_PROCESS),
			     evc->evc_locklist, &lk_event, &clerror);
	    lkerr = (clstat != OK);
	    /* Leave status alone as this is external anyway */
	}

	/* Find the server entry */
	server = (EV_SVRENTRY *)((PTR)ecb + soffset);

	/* If there are instances then remove all of them from front of list */
	inst_remove = 0;
	inst_offset = server->ev_svrinstoff;
	while (inst_offset != 0)
	{
	    /* Point at current instance, disconnect it and deallocate */
	    instance = (EV_INSTANCE *)((PTR)ecb + inst_offset);
	    server->ev_svrinstoff = instance->ev_innextoff;
	    instance->ev_innextoff = 0;
	    /* Get varying size of this instance before deallocation */
	    inst_alloc = sizeof(EV_INSTANCE) +
			 instance->ev_inlname + instance->ev_inldata;
	    sce_efree(ecb, inst_alloc, inst_offset);
	    inst_remove++;
	    inst_offset = server->ev_svrinstoff;	/* Next instance */
	}
	/* Point to start of server hash chain (bump by bkt # to get to node */
	shash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);
	shash += sbucket;
	if (shash->ev_shoffset == soffset)	/* First one on server list */
	{
	    shash->ev_shoffset = server->ev_svrnextoff;
	}
	else
	{
	    /*
	    ** Walk through till we get to our server entry by comparing
	    ** offsets with the different servers.
	    */
	    sprev_offset = shash->ev_shoffset;
	    sprev = (EV_SVRENTRY *)((PTR)ecb + sprev_offset);
	    while (sprev->ev_svrnextoff != soffset)
	    {
		sprev_offset = sprev->ev_svrnextoff;
		sprev = (EV_SVRENTRY *)((PTR)ecb + sprev_offset);
	    }
	    /* Disconnect our node (its offset) from the previous one */
	    sprev->ev_svrnextoff = server->ev_svrnextoff;
	}
	server->ev_svrnextoff = 0;		/* Disconnect & free */
	sce_efree(ecb, sizeof(EV_SVRENTRY), soffset);
	sce_munlock(sunlock);
	status = E_DB_OK;
	break;
    } /* Dummy error loop for removing event instances */

    /*
    ** If this is an all-event-registered server then unlink the pseudo
    ** registration from the "all" chain.
    */
    if (evc->evc_flags & SCEV_REG_ALL)
    {
	sce_mlock(&ecb->evs_asemaphore);		/* Lock ALL chain */
	rprev_offset = 0;
	roffset = ecb->evs_alloffset;
	while (roffset != 0)				/* Search for node */
	{
	    rptr = (EV_RSERVER *)((PTR)ecb + roffset);
	    if (rptr->ev_rspid == evc->evc_pid)		/* Found it */
		break;
	    rprev_offset = roffset;
	    roffset = rptr->ev_rsnextoff;
	}
	if (roffset != 0)				/* Found the ALL reg */
	{
	    if (rprev_offset == 0)			/* Must have been 1st */
	    {
		ecb->evs_alloffset = rptr->ev_rsnextoff;	/* Unlink */
	    }
	    else					/* Not 1st, so bypass */
	    {
		/* Disconnect our node's offset from previous node */
		rprev = (EV_RSERVER *)((PTR)ecb + rprev_offset);
		rprev->ev_rsnextoff = rptr->ev_rsnextoff;
	    }
	    rptr->ev_rsnextoff = 0;			/* Disconnect & free */
	    sce_efree(ecb, sizeof(EV_RSERVER), roffset);
	    ecb->evs_acount--;				/* Track # of ALL */
	    sce_munlock(&ecb->evs_asemaphore);
	}
	else						/* Not found */
	{
	    /* Log error but don't set status as we still want to clean up */
	    sce_munlock(&ecb->evs_asemaphore);
	    (*evc->evc_errf)(E_SC0291_XSEV_NOT_CONNECT, 0, 2,
			     sizeof(evc->evc_pid), &evc->evc_pid,
			     sizeof("sce_edisconnect/SCE_REG_ALL")-1,
			     "sce_edisconnect/SCEV_REG_ALL");
	}
    } /* If this is an all-event-server */

    /* If we really removed a server then adjust counters and/or shut down EV */
    if (status == E_DB_OK)
    {
	sce_mlock(&ecb->evs_gsemaphore);
	ecb->evs_scount--;
	if (ecb->evs_scount == 0)		/* Shutdown EV */
	{
	    /* Logical lock before releasing semaphore */
	    ecb->evs_status = EV_INACTIVE;
	    sce_munlock(&ecb->evs_gsemaphore);
 
            /* remove all semaphores before freeing the memory */
            CSr_semaphore(&ecb->evs_gsemaphore);
 
            ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
            for (bkt = 0; bkt < ecb->evs_rbuckets; bkt++)
            {
                CSr_semaphore(&ehash->ev_ehsemaphore);
                ehash++;
            }
 
            shash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);
            for (bkt = 0; bkt < ecb->evs_ibuckets; bkt++)
            {
                CSr_semaphore(&shash->ev_shsemaphore);
                shash++;
            }
 
            CSr_semaphore(&ecb->evs_asemaphore);
 
	    sce_mlock(evc->evc_mem_sem);
	    MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
	    MEcopy((PTR)EV_SEGNAME, EV_SEGSIZE, (PTR)smnm);
	    smnm[EV_SEGSIZE] = EOS;
		CS_cp_sem_cleanup(smnm, &clerror);
	    clstat = MEsmdestroy(smnm, &clerror);
	    sce_munlock(evc->evc_mem_sem);
	    if (clstat != OK)
	    {
		(*evc->evc_errf)(E_SC0288_XSEV_SM_DESTROY, &clerror, 1,
				 STlength(smnm), smnm);
		status = E_DB_FATAL;
	    }
	}
	else		/* Update instance count & map out shared segment */
	{
	    ecb->evs_icount -= inst_remove;
	    sce_munlock(&ecb->evs_gsemaphore);
	    if ((evc->evc_flags & SCEV_DISCON_EXT) == 0)
	    {
		sce_mlock(evc->evc_mem_sem);
		MEfree_pages((PTR)ecb, ecb->evs_memory/ME_MPAGESIZE, &clerror);
		sce_munlock(evc->evc_mem_sem);
	    }
	}
	evc->evc_root = NULL;
    } /* If everything was ok */

    /* If not external disconnect and we do have a lock list - release it */
    if (evc->evc_flags & SCEV_DISCON_EXT)
    {
	if (lkerr)
	{
	    (*evc->evc_errf)(E_SC0290_XSEV_SIGNAL, &clerror, 2,
			     sizeof(PID), &evc->evc_pid,
			     sizeof("EV-DISCONNECT")-1, "EV-DISCONNECT");
	}
    }
    else if (evc->evc_locklist != (LK_LLID) 0)
    {
	clstat = LKrelease(LK_ALL, evc->evc_locklist,
			   (LK_LKID *)NULL, (LK_LOCK_KEY *)NULL,
			   (LK_VALUE *)NULL, &clerror);
	evc->evc_locklist = 0;
    }

    return (status);
} /* sce_edisconnect */

/*{
** Name: sce_ewinit	- Initialize lock list
**
** Description:
**	This routine is separated out of sce_econnect because, in the case
**	of a DBMS cleint, it must be called by the same thread that will
**	wait for all events.  The routine is just a cover on top of
**	LKcreate_list.
**
** Inputs:
**	evc			EV request context structure
**
** Outputs:
**	evc			EV request context structure.  The "locklist"
**				field must be left intact for EV use:
**	    .evc_locklist	Lock list to use for event notification or NULL
**				on error.
**	Returns:
**	    DB_STATUS		An error implies that events cannot be received
**				by caller.
**	Errors:
**	    Errors found during processing are logged in the error log:
**	    E_SC028A_XSEV_CRLOCKLIST	Creation of server lock list
**	    E_SC029F_XSEV_EVCONNECT	Obtain server's LK_EVCONNECT lock
**
** History:
**	14-mar-90 (neil)
**	    Extracted from sce_econnect to associate with client thread.
**	16-aug-91 (ralph)
**	    Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_ewinit() grabs a lock (LK_EVCONNECT) during connect processing.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      20-Oct-2006 (horda03) Bug 116912
**          Clear the EV_SVRINIT flag once the attempt to acquire the LK_EVCONNECT
**          has been made.
*/

DB_STATUS
sce_ewinit( SCEV_CONTEXT  *evc )
{
    i4		lkflags;	/* For LK */
    CL_ERR_DESC		clerror;
    STATUS              clstatus;
    EV_SCB              *ecb;
    LK_LOCK_KEY		lkkey;
    u_i4		lkmode;
    i4                  sbucket;
    EV_SVRENTRY         *server;
    i4                  soffset;
    CS_SEMAPHORE        *sunlock;

    evc->evc_locklist = 0;
    lkflags = LK_ASSIGN | LK_NONPROTECT | LK_MULTITHREAD;
    clstatus = LKcreate_list(lkflags, (LK_LLID)0, NULL, &evc->evc_locklist,
			     (i4)0, &clerror);
    if (clstatus != OK)
    {
	(*evc->evc_errf)(E_SC028A_XSEV_CRLOCKLIST, &clerror, 0);
	return(E_DB_ERROR);
    }

    /*
    ** Get the LK_EVCONNECT lock for this server.
    **
    ** There is a potential timing conflict is another server
    ** attempts to probe this server while this server is attempting
    ** to get the lock.  This is why we ask for a timeout at 10 seconds.
    */

    lkkey.lk_type = LK_EVCONNECT;
    lkkey.lk_key1 = evc->evc_pid;
    lkkey.lk_key2 = 0;
    lkkey.lk_key3 = 0;
    lkkey.lk_key4 = 0;
    lkkey.lk_key5 = 0;
    lkkey.lk_key6 = 0;
    lkmode = LK_X;
    lkflags = LK_PHYSICAL;

    clstatus = LKrequest(lkflags, evc->evc_locklist, &lkkey, lkmode,
			 NULL, NULL, (i4)10, &clerror);
    /* The LK_EVCONNECT lock has either been obtained or there's been
    ** an error. In either case the server has completed it's initialisation, so
    ** find the server event instance and clear the INIT flag.
    */
    ecb = (EV_SCB *)evc->evc_root;
    sce_eslock(ecb, evc->evc_pid, &sbucket, &sunlock, &soffset);

    if (soffset == 0)
    {
       sce_munlock(sunlock);
#define EVC_NO_SERVER "sce_ewinit"
        (*evc->evc_errf)(E_SC0291_XSEV_NOT_CONNECT, 0, 2,
                             sizeof(evc->evc_pid), &evc->evc_pid,
                             sizeof(EVC_NO_SERVER)-1, EVC_NO_SERVER);

	clstatus = LKrelease(LK_ALL, evc->evc_locklist,
			     (LK_LKID *)NULL, (LK_LOCK_KEY *)NULL,
			     (LK_VALUE *)NULL, &clerror);
	return(E_DB_ERROR);
    }
        

    server = (EV_SVRENTRY *)((PTR)ecb + soffset);
    server->ev_svrflags &= ~(EV_SVRINIT);
    sce_munlock(sunlock);
    
    if (clstatus != OK)
    {
        (*evc->evc_errf)(E_SC029F_XSEV_EVCONNECT, &clerror, 0);
	clstatus = LKrelease(LK_ALL, evc->evc_locklist,
			     (LK_LKID *)NULL, (LK_LOCK_KEY *)NULL,
			     (LK_VALUE *)NULL, &clerror);
	return(E_DB_ERROR);
    }

    return (E_DB_OK);
} /* sce_ewinit */

/*{
** Name: sce_ewset	- Prepare to wait for an event through LKevent
**
** Description:
**	This routine is used to prepare to wait for an event for which
**	the thread is registered in the event system.  It is actually
**	an interface to LKevent.
**
**	This routine must be called prior to calling sce_ewait to actually
**	wait for the event to occur.
**
**	After this routine is called, the event may occur before even
**	making the sce_ewait call.  In this case the sce_ewait call will
**	return immediatly without waiting.
**
**	This routine provides a method of closing the window between when
**	a client of the event system checks for outstanding events and
**	when the client waits for new events to occur.  Without this routine,
**	events which occur in between those actions would be lost.  This
**	routine allows the client to do the following:
**
**	    sce_ewset()				* Prepare to wait *
**	    check for outstanding events
**	    sce_ewait()				* Wait for new events *
**
**	By doing this, the caller assures that no events will be missed, since
**	the sce_ewait call will return if any new events come in after having
**	checked the outstanding event queue.
**	
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of waiting caller
**
** Outputs:
**	Returns:
**	    E_DB_OK 			An LKevent arrived - call sce_efetch.
**	    E_DB_ERROR			LKevent does not like us - close EV
**
** History:
**      19-mar-90 (rogerk)
**	    First written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_ewset( SCEV_CONTEXT	*evc )
{
    i4		lkflags;	/* For LK */
    CL_ERR_DESC	    	clerror;
    STATUS	    	clstatus;
    LK_EVENT		lk_event;

    /* Call LK to prepare for wait call */
    lkflags = (LK_E_SET | LK_E_CROSS_PROCESS | LK_E_INTERRUPT);

    lk_event.type_high = 0;
    lk_event.type_low = 0;
    lk_event.value = evc->evc_pid;

    clstatus = LKevent(lkflags, evc->evc_locklist, &lk_event, &clerror);
    /*
    ** If not OK caller should shut down the event thread.  Caller cannot
    ** wait for events.
    */
    if (clstatus == OK)
	return (E_DB_OK);
    else
	return (E_DB_ERROR);
} /* sce_ewset */

/*{
** Name: sce_ewait	- Wait for an event through LKevent
**
** Description:
**	A server uses EV by establishing an event thread dedicating to waiting
**	on (and processing) events on behalf of other sessions/threads.  This
**	routine acts as an interface to LKevent for the events that were
**	broadcast through sce_esignal.
**
**	Calls to this routine must be preceeded by an sce_ewset call which
**	prepares the caller for the event wait.  See the sce_ewset header for
**	more information on the event wait protocols.
**
** Inputs:
**	evc				EV request context structure:
**	  .evc_pid			Process id of waiting caller
**
** Outputs:
**	Returns:
**	    E_DB_OK 			An LKevent arrived - call sce_efetch.
**	    E_DB_ERROR			LKevent does not like us - close EV
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	19-mar-90 (rogerk)
**	    Took out LK_E_SET parameter to LKevent call and added routine
**	    sce_ewset call to prepare for the wait call.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_ewait( SCEV_CONTEXT *evc )
{
    i4		lkflags;	/* For LK */
    CL_ERR_DESC	    	clerror;
    STATUS	    	clstatus;
    LK_EVENT		lk_event;

    /* Wait for an event to be signalled to this server */
    lkflags = (LK_E_WAIT | LK_E_CROSS_PROCESS | LK_E_INTERRUPT);

    lk_event.type_high = 0;
    lk_event.type_low = 0;
    lk_event.value = evc->evc_pid;

    clstatus = LKevent(lkflags, evc->evc_locklist, &lk_event, &clerror);
    /*
    ** If not OK caller should shut down the event thread - regardless of
    ** value (even if LK_INTERRUPT).
    */
    if (clstatus != OK)
	return (E_DB_ERROR);
    else
	return (E_DB_OK);
} /* sce_ewait */

/*
** List Management Routines
*/

/*{
** Name: sce_erlock	- Find/lock an event registration entry
**
** Description:
**	Search the registered events for the specified event.  There are 3
**	possible results:
**		1. The server registration exists
**		2. The event exists but is not registered for this server
**		3. The event does not exist (return the bucket anyway).
**	Whatever is found the event hash bucket is locked (ev_ehsemaphore).
**	To release it the caller must unlock the passed in semaphore "eunlock".
**
** Inputs:
**	ecb				EV root data structures.
**	event				Event being searched for:
**	   .ev_pid			Server ID. 
**	   .ev_lname			Length of name being searched for.
**	   .ev_name			Name being searched for
**	roffset				Pointer to result registration.  If
**					this is NULL then stop on the event 
**					without searching for a specific server
**					registration - for signalling all
**					servers.
** Outputs:
**	ebucket	    			Bucket # in which the event should be.
**					This is set regardless of whether the
**					actual event is found to allow new
**					events to be entered correctly.
**	eunlock				Pointer to semaphore used to lock
**					bucket.  This is always set.
**	eoffset				Offset of event entry if one was found.
**					Set to 0 if event not found.
**	roffset				If not NULL, then offset of server
**					registration if one was found.  Set to
**					0 if not NULL and not found.
**	Returns:
**	    VOID
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_erlock(EV_SCB *ecb,
	   SCEV_EVENT *event,
	   i4  *ebucket,
	   CS_SEMAPHORE	**eunlock,
	   i4 *eoffset,
	   i4 *roffset )
{
    i4		bkt;		/* Bucket we are looking for */
    i4			i;
    EV_EVHASH		*ehash;		/* Cell into event hash table */
    EV_REVENT		*eptr;		/* Event entry in cell */
    i4		event_offset;	/* To return to caller */
    EV_RSERVER		*rptr;		/* Server registration to look for */
    i4		reg_offset;	/* Registration offset to return */

    /* Hash event name to find the event bucket */
    for (i = 0, bkt = 0; i < event->ev_lname; i++)
	bkt += event->ev_name[i];
    bkt = bkt % ecb->evs_rbuckets;

    /* Point to start of hash chain & bump by bucket # to get to our cell */
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    ehash += bkt;

    sce_mlock(&ehash->ev_ehsemaphore);	/* It's locked */

    *ebucket = bkt;			/* Give to user asap */
    *eunlock = &ehash->ev_ehsemaphore;
    *eoffset = 0;
    if (roffset)
	*roffset = 0;

    if (ehash->ev_ehoffset == 0)	/* If no events in bucket we're done */
	return;

    event_offset = ehash->ev_ehoffset;
    eptr = (EV_REVENT *)((PTR)ecb + event_offset);

    while (eptr != NULL)	/* Walk event looking for a name match */
    {
	if (MEcmp(eptr->ev_rename, event->ev_name, event->ev_lname) == 0)
	{
	    /* This is the correct event entry - set user event offset */
	    *eoffset = event_offset;

	    if (roffset == NULL)		/* All servers can return now */
		return;

	    if ((reg_offset = eptr->ev_resvroff) == 0)
		return;			/* No registrations for this event ? */

	    /* Some registrations exits - lets look for ours */
	    reg_offset = eptr->ev_resvroff;
	    while (reg_offset != 0)
	    {
		rptr = (EV_RSERVER *)((PTR)ecb + reg_offset);
		if (rptr->ev_rspid == event->ev_pid)
		{
		    /* We have match - this server is registered */
		    *roffset = reg_offset;
		    return;
		}
		else			/* Try next registered server */
		{
		    reg_offset = rptr->ev_rsnextoff;
		}
	    }
	    return;			/* Couldn't have found registration */
	}
	else				/* Try next event */
	{
	    if ((event_offset = eptr->ev_renextoff) == 0)
		eptr = NULL;		/* Stop outer loop */
	    else
		eptr = (EV_REVENT *)((PTR)ecb + event_offset);
	} /* If found event or not */
    } /* While events on list */

    /* Got here - couldn't have found matching event */
    return;
} /* sce_erlock */

/*{
** Name: sce_erunlink	- Unlink specified registration out of event list
**
** Description:
**	This routine searches for the already-identified registration in the 
**	event list and unlinks it.  It does not have to worry about state
**	modification by other servers as it requires the caller to already
**	have locked the root hash chain.
**
** Inputs:
**	ecb				EV root data structures.
**	eptr				Event list root
**	rremove				Registration to remove from event list
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**	14-feb-90 (neil)
**	    First written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_erunlink(EV_SCB *ecb, EV_REVENT *eptr,EV_RSERVER  *rremove )
{
    i4	roffset;	/* Offset of registration to remove */
    EV_RSERVER	*rprev;		/* Previous node to removed registration */
    i4	rprev_offset;

    roffset = (PTR)rremove - (PTR)ecb;
    if (eptr->ev_resvroff == roffset)	/* First registration in list */
    {
	/*
	** If registration is the first in event registration list then just
	** adjust the event registration list pointer (offset) and unlink
	** our registration.
	*/
	eptr->ev_resvroff = rremove->ev_rsnextoff;
    }
    else				/* Others in list */
    {
	/*
	** Walk through the list until we get to our registration by
	** comparing the offsets of the different registrations.
	*/
	rprev_offset = eptr->ev_resvroff;
	rprev = (EV_RSERVER *)((PTR)ecb + rprev_offset);
	while (rprev->ev_rsnextoff != roffset)
	{
	    rprev_offset = rprev->ev_rsnextoff;
	    rprev = (EV_RSERVER *)((PTR)ecb + rprev_offset);
	}
	/* Disconnect our node (its offset) from the previous node */
	rprev->ev_rsnextoff = rremove->ev_rsnextoff;
    }
    /* Disconnect and deallocate the registration entry */
    rremove->ev_rsnextoff = 0;
    sce_efree(ecb, sizeof(EV_RSERVER), roffset);
} /* sce_erunlink */

/*{
** Name: sce_evunlink	- Unlink specified event out of event hash chain
**
** Description:
**	This routine searches for the already-identified event in the hash
**	list and unlinks it.  It does not have to worry about state
** 	modification by other servers as it requires the caller to already
**	have locked the current hash chain.
**
** Inputs:
**	ecb				EV root data structures.
**	ehash				Event hash root node
**	eremove				Event node to remove from hash chain
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**	14-feb-90 (neil)
**	    First written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_evunlink( EV_SCB  *ecb, EV_EVHASH  *ehash, EV_REVENT  *eremove )
{
    i4	eoffset;	/* Offset of event node to remove */
    EV_REVENT	*eprev;		/* Previous node to removed event */
    i4	eprev_offset;

    eoffset = (PTR)eremove - (PTR)ecb;
    if (ehash->ev_ehoffset == eoffset)		/* First event in list */
    {
	/*
	** If this event is the first in the hash bucket then adjust the hash
	** bucket event list pointer (offset) and unlink the event.
	*/
	ehash->ev_ehoffset = eremove->ev_renextoff;
    }
    else					/* Other events in hash list */
    {
	/*
	** Walk through the list until we get to our event by comparing the
	** offsets of the different events.
	*/
	eprev_offset = ehash->ev_ehoffset;
	eprev = (EV_REVENT *)((PTR)ecb + eprev_offset);
	while (eprev->ev_renextoff != eoffset)
	{
	    eprev_offset = eprev->ev_renextoff;
	    eprev = (EV_REVENT *)((PTR)ecb + eprev_offset);
	}
	/* Disconnect our node (its offset) from the previous node */
	eprev->ev_renextoff = eremove->ev_renextoff;
    }
    /* Disconnect and deallocate the event entry */
    eremove->ev_renextoff = 0;
    sce_efree(ecb, sizeof(EV_REVENT), eoffset);
} /* sce_evunlink */

/*{
** Name: sce_eslock	- Find/lock a server event instance entry.
**
** Description:
**	Search the server entries for the specified server.  If the entry does
**	not exist the server bucket is returned anyway.  Whatever is returned
**	the server hash bucket is locked (ev_shsemaphore).  To release it the
**	caller must realse the passed in "sunlock" semaphore.
**
** Inputs:
**	ecb				EV root data structures.
**	pid				Server ID being looked for.
**
** Outputs:
**	sbucket	    			Bucket # in which the server should be.
**					This is set regardless of whether the
**					actual server entry is found to allow
**					new servers to be entered correctly.
**	sunlock				Pointer to semaphore used to lock
**					bucket.  This is always set.
**	soffset				Offset of server entry if one was found.
**					Set to 0 if server entry not found.
**	Returns:
**	    VOID
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_eslock( EV_SCB *ecb,
	   PID pid,
	   i4  *sbucket,
	   CS_SEMAPHORE **sunlock,
	   i4 *soffset )
{
    i4	bkt;		/* Bucket in which server entry should be */
    EV_SVRHASH	*shash;		/* Hash cell entry in server bucket */
    EV_SVRENTRY	*server;	/* Server entries in server hash chain */
    i4	svr_offset;	/* Server offset of entry */

    bkt = pid % ecb->evs_ibuckets;
    /* Point to start of hash chain & bump by bucket # to get to our cell */
    shash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);
    shash += bkt;

    sce_mlock(&shash->ev_shsemaphore);		/* It's locked */

    *sbucket = bkt;
    *sunlock = &shash->ev_shsemaphore;
    *soffset = 0;

    if (shash->ev_shoffset == 0)		/* No servers on chain */
	return;

    /* Find the server entry on the hash chain */
    svr_offset = shash->ev_shoffset;
    while (svr_offset != 0)
    {
	server = (EV_SVRENTRY *)((PTR)ecb + svr_offset);
	if (server->ev_svrpid == pid)		/* Found a match */
	{
	    *soffset = svr_offset;
	    return;
	}
	else					/* Try next server */
	{
	    svr_offset = server->ev_svrnextoff;
	}
    }
    /* Didn't find server */
    return;
} /* sce_eslock */

/*
** Free Space Allocation Routines
*/

/*{
** Name: sce_eallocate	- Allocate a block of memory
**
** Description:
**	Allocate a block of memory from internally managed free space and
**	return the offset of the newly allocated block.
**
**	Free space is linked into a chain of free blocks rooted at evs_foffset.
**	Each chuck of free space contains the size of the block and the
**	offset of the next free block.  A zero offset indicates the end of the
**	chain.  We implement a first fit allocation strategy.
**
** Inputs:
**	ecb				EV root data structures.
**	amount				Amount of memory to allocate
**
** Outputs:
**	alloc_offset	    		Offset where the memory starts
**	Returns:
**	    DB_STATUS
**	Errors:
**	    None - caller will log allocation errors.
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static DB_STATUS
sce_eallocate( EV_SCB  *ecb, i4  amount, i4 *alloc_offset )
{
    EV_FREESPACE   	*freesp;	/* Piece to return */
    i4		next_offset;	/* Offset of that piece */
    EV_FREESPACE   	*prevfsp;	/* Keep track of previous piece */
    EV_FREESPACE   	*brkfsp;	/* To break a bigger piece */
    i4			pad;		/* For alignment */
    DB_STATUS		status;

    /*
    ** There is an assumption that no blocks less than the size of
    ** a free space header (EV_FREESPACE) are ever allocated or remaindered
    ** from an allocation.  Make sure to always deal with aligned chunks
    ** so we don't need to worry about aligning pointers.
    */
    if (amount < sizeof(EV_FREESPACE))
	amount = sizeof(EV_FREESPACE);
    pad = amount % sizeof(ALIGN_RESTRICT);
    if (pad != 0)
	amount += sizeof(ALIGN_RESTRICT) - pad;

    sce_mlock(&ecb->evs_gsemaphore);		/* Lock free lists */

    prevfsp = NULL;
    next_offset = ecb->evs_foffset;
    while (next_offset != 0)		/* Stop when all pieces are exhausted */
    {
	freesp = (EV_FREESPACE *)((PTR)ecb + next_offset);
	if (freesp->ev_fsize == amount)		/* Perfectisimo! */
	{
	    if (prevfsp == NULL)		/* Must have been first block */
	        ecb->evs_foffset = freesp->ev_fnextoff;
	    else				/* Skip around the block */
	        prevfsp->ev_fnextoff = freesp->ev_fnextoff;
	    break;
	}
	else if (freesp->ev_fsize >= (amount + sizeof(EV_FREESPACE)))
	{
	    /*
	    ** This block is first fit but not a perfect size.  Break it into
	    ** the requested amount and a new free space block.
	    */
	    if (prevfsp == NULL)		/* Must have been first block */
	    {
		ecb->evs_foffset = ecb->evs_foffset + amount;
		brkfsp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
	    }
	    else				/* Skip around the block */
	    {
		prevfsp->ev_fnextoff = prevfsp->ev_fnextoff + amount;
		brkfsp = (EV_FREESPACE *)((PTR)ecb + prevfsp->ev_fnextoff);
	    }
	    brkfsp->ev_fsize = freesp->ev_fsize - amount;
	    brkfsp->ev_fnextoff = freesp->ev_fnextoff;
	    break;
	}
	else					/* Try next block */
	{
	    prevfsp = freesp;
	    next_offset = freesp->ev_fnextoff;
	}
    } /* While more free space blocks */

    if (next_offset != 0)			/* Block was found */
    {
	*alloc_offset = next_offset;
	freesp->ev_fnextoff = 0;
	ecb->evs_favailable -= amount;
	status = E_DB_OK;
    }
    else
    {
	*alloc_offset = 0;
	status = E_DB_ERROR;
    }

    sce_munlock(&ecb->evs_gsemaphore);		/* Free free lists */

    return (status);
} /* sce_eallocate */

/*{
** Name: sce_efree	- Free a block of memory
**
** Description:
**	Free a block of memory to the internally managed free space.  See
**	sce_eallocate for allocating logic.  Here we implement a coalescing
**	deallocation strategy.
**
** Inputs:
**	ecb				EV root data structures.
**	amount				Amount of memory to free
**	deall_offset 			Offset to free from
**
** Outputs:
**	Returns:
**	    VOID
**	Errors:
**	    None
**
** History:
**      14-oct-89 (paul)
**	    First written to support alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    remove extra comment start.
*/

static VOID
sce_efree( EV_SCB *ecb, i4  amount, i4 deall_offset )
{
    PTR			deall_block;	/* Block being deallocated */
    EV_FREESPACE   	*freesp;	/* Piece in free list */
    EV_FREESPACE   	*prevfsp;	/* Keep track of previous piece */
    EV_FREESPACE   	*mrgfsp;	/* To merge two pieces */
    i4			pad;		/* For alignment */

    deall_block = (PTR)ecb + deall_offset;

    /* Always deal with aligned chunks as returned from sce_eallocate */
    if (amount < sizeof(EV_FREESPACE))
	amount = sizeof(EV_FREESPACE);
    pad = amount % sizeof(ALIGN_RESTRICT);
    if (pad != 0)
	amount += sizeof(ALIGN_RESTRICT) - pad;

    sce_mlock(&ecb->evs_gsemaphore);			/* Lock free lists */

    if (ecb->evs_foffset == 0)				/* All memory is used */
    {
	ecb->evs_foffset = deall_offset;
	freesp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
	freesp->ev_fsize = amount;
	freesp->ev_fnextoff = 0;
    }
    else						/* Put into free list */
    {
	prevfsp = NULL;
	freesp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
	for (;;)					/* Break when found */
	{
	    if (((PTR)freesp + freesp->ev_fsize) == deall_block)
	    {
		/* This block follows a current freespace block - first
		** coalesce the deallocated block.  Then coalesce if the
		** next block is "now" contiguous.  For example, start with
		** ABC, allocate A, then B.  Now deallocate A, we have:
		** A->C.  Now deallocate B.  If we do not coalesce we are left
		** with AB->C.  The extra check merges back to ABC.
		*/
		freesp->ev_fsize += amount;
		if (deall_offset + amount == freesp->ev_fnextoff)
		{
		    mrgfsp =
			 (EV_FREESPACE *)((PTR)ecb + freesp->ev_fnextoff);
		    freesp->ev_fsize += mrgfsp->ev_fsize;
		    freesp->ev_fnextoff = mrgfsp->ev_fnextoff;
		}
		break;
	    }
	    else if ((PTR)freesp == (deall_block + amount))
	    {
		/*
		** Free block immediately follows block we are deallocating.
		** Adjust free block accordingly.
		*/
		mrgfsp = (EV_FREESPACE *)deall_block;
		mrgfsp->ev_fsize = freesp->ev_fsize + amount;
		mrgfsp->ev_fnextoff = freesp->ev_fnextoff;
		if (prevfsp == NULL)
		    ecb->evs_foffset = deall_offset;	/* To head of list */
		else
		    prevfsp->ev_fnextoff = deall_offset;
		break;
	    }
	    else if ((PTR)freesp > deall_block)
	    {
		/* Add a new free block in front of freesp */
		mrgfsp = (EV_FREESPACE *)deall_block;
		mrgfsp->ev_fsize = amount;
		if (prevfsp == NULL)			/* To head of list */
		{
		    mrgfsp->ev_fnextoff = ecb->evs_foffset;
		    ecb->evs_foffset = deall_offset;
		}
		else
		{
		    mrgfsp->ev_fnextoff = prevfsp->ev_fnextoff;
		    prevfsp->ev_fnextoff = deall_offset;
		}
		break;
	    }
	    else
	    {
		if (freesp->ev_fnextoff == 0)
		{
		    /* Out of free blocks, add this new one on the end */
		    freesp->ev_fnextoff = deall_offset;
		    mrgfsp = (EV_FREESPACE *)deall_block;
		    mrgfsp->ev_fsize = amount;
		    mrgfsp->ev_fnextoff = 0;
		    break;
		}
		prevfsp = freesp;
		freesp = (EV_FREESPACE *)((PTR)ecb + freesp->ev_fnextoff);
	    }
	}
    }
    ecb->evs_favailable += amount;

    sce_munlock(&ecb->evs_gsemaphore);
} /* sce_efree */

/*
** Tracing routines
*/

/*{
** Name: sce_ename	- Internal tracing routine to extract event name
**
** Description:
**	Format event name into a string
**
** Inputs:
**	lname		Length of alert/event name
**	name		Event name
**
** Outputs:
**	evname		Buffer to deposit formatted event name (assumed
**			EV_PRMAX_NAME).  If lname was not the length of an
**			alert then this is is just copied (truncated).
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	14-feb-90 (neil)
**	    Written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static i4
sce_ename( i4  lname, char *name, char  *evname )
{
    char		anm[DB_MAXNAME + 1]; 	/* Buffers for name parts */
    char		onm[DB_MAXNAME + 1];
    char		dnm[DB_MAXNAME + 1];
    DB_ALERT_NAME	*alert;

    if (lname == sizeof(DB_ALERT_NAME))	/* Assume well-defined name */
    {
	alert = (DB_ALERT_NAME *)name;
	MEcopy((PTR)&alert->dba_alert, DB_MAXNAME, (PTR)anm);
	anm[DB_MAXNAME] = EOS;
	STtrmwhite(anm);
	MEcopy((PTR)&alert->dba_owner, DB_MAXNAME, (PTR)onm);
	onm[DB_MAXNAME] = EOS;
	STtrmwhite(onm);
	MEcopy((PTR)&alert->dba_dbname, DB_MAXNAME, (PTR)dnm);
	dnm[DB_MAXNAME] = EOS;
	STtrmwhite(dnm);
	STprintf(evname, "(%s, %s, %s)", anm, onm, dnm);
    }
    else				/* Copy/truncate */
    {
	evname[0] = '(';
	lname = min(lname, (EV_PRNAME_MAX-3));
	MEcopy((PTR)name, lname, (PTR)&evname[1]);
	evname[1+lname] = ')';
	evname[2+lname] = EOS;
    }
    return (STlength(evname));
} /* sce_ename */

/*{
** Name: sce_edump	- Dump EV data structures for debugging
**
** Description:
**      This procedure dumps the information strored by EV.  Note that 
**	this routine executes I/O operations while holding multi-process
**	semaphores.  This is normally disallowed but done here as a special
**	case to support EV.
**
**	The caller must have verified that they are using EV before calling.
**
**	NOTE: The layout of trace lines that include SCEV_xYYY strings should
**	      not be modified as they are tracked by sceadmin as well.
**
** Inputs:
**	evc			EV request context structure:
**	    .evc_root		EV root data structures
**	    .evc_pid		Process id of caller
**	    .evc_trcf		Trace function
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**      14-oct-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	16-aug-91 (ralph)
**	    Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_dump() probes servers' LK_EVCONNECT lock to determine if
**		they are still active.
**	13-jan-93 (mikem)
**	    Lint directed changes.  Fixed "==" vs. "=" bugs in sce_edump().  
**	    Previous to this fix the "(*ERROR*)" and "(LKERROR)" cases were not
**	    formated properly (probably did not happen very often if ever).
**	2-Jul-1993 (daveb)
**	    remove unused var 'lkflags'. prototyped.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered (*ecv_trct) calls to NOT assume that ecv_trct is a
**	    varargs function.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to (*ecv_trct).
**	    This works because STprintf is a varargs function in the CL.
**	    Included sc0e.h for SC0E_FMTSIZE definition.
**	16-Mar-2006 (wanfr01)
**	    SIR 115849
**	    Add printing of event names and offsets
**      20-Oct-2006 (horda03) Bug 116912
**          If the server is flagged EV_SVRINIT, then it hasn't yet tried to
**          acquire its LK_EVCONNECT lock, so indicate the server is initialising.
**	08-Sep-2008 (jonj)
**	    Fix mis-typed "timeout" param to LKrequest from NULL to 0.
*/

VOID
sce_edump( SCEV_CONTEXT	*evc )
{
    EV_SCB		*ecb;		/* EV data structures to dump */
    EV_EVHASH		*ehash;		/* Hash cell in event hash list */
    EV_REVENT		*eptr;		/* Event entry in event hash cell */
    EV_RSERVER		*rptr;		/* Server registered for an event */
    EV_SVRHASH		*shash;		/* Hash cell in server hash list */
    EV_SVRENTRY		*sentry;	/* Server entry in server hash cell */
    EV_INSTANCE   	*inst;		/* Instance attached to server */
    EV_FREESPACE   	*freesp;	/* Free space stuff */
    PID			spid;		/* Current servers id */
    VOID		(*etrc)();	/* Trace function to call */
    char		evname[EV_PRNAME_MAX];
    i4			bkt;
    i4			cnt;

    CL_ERR_DESC		clerror;
    STATUS              clstatus;
    LK_LOCK_KEY		lkkey;
    LK_LKID		lkid;
    u_i4		lkmode = LK_S;
    char		*pidlit;
    char		stbuf[SC0E_FMTSIZE]; /* last char for `\n' */

    lkkey.lk_type = LK_EVCONNECT;
    lkkey.lk_key2 = 0;
    lkkey.lk_key3 = 0;
    lkkey.lk_key4 = 0;
    lkkey.lk_key5 = 0;
    lkkey.lk_key6 = 0;
    MEfill(sizeof(LK_LKID), 0, &lkid);

    etrc = evc->evc_trcf;		/* Set by original connecter */

    (*etrc)("SCEV/ Event Subsystem Data Structures:\n");
    (*etrc)(" sch_smemory: SCEV event system control block:\n");

    if ((ecb = (EV_SCB *)evc->evc_root) == NULL)
    {
	(*etrc)("  server is not attached to event subsystem.\n");
	return;
    }
    if (ecb->evs_status == EV_INACTIVE)
    {
	(*etrc)("  event subsystem structures are not currently active.\n");
	return;
    }

    spid = evc->evc_pid;
    (*etrc)(STprintf(stbuf, "  current_server_id = 0x%x\n", spid));
    (*etrc)(STprintf(stbuf,
	    "  ecb_base: 0x%p , evs_version: %d, evs_segname: <%s>\n",
	    (PTR)ecb, ecb->evs_version,
	    ecb->evs_segname));
    (*etrc)(STprintf(stbuf,
	    "  evs_orig_pid = 0x%x\n", (i4)ecb->evs_orig_pid));
    (*etrc)(STprintf(stbuf,
	    "  evs_rbuckets = %d, evs_ibuckets = %d\n", ecb->evs_rbuckets,
	    ecb->evs_ibuckets));

    sce_mlock(&ecb->evs_gsemaphore);
    (*etrc)(STprintf(stbuf,
	    "  evs_scount: %d, evs_rcount = %d, evs_icount = %d\n",
	    ecb->evs_scount, ecb->evs_rcount, ecb->evs_icount));
    (*etrc)(STprintf(stbuf,
	    "  evs_isuccess: %d, evs_ifail = %d\n",
	    ecb->evs_isuccess, ecb->evs_ifail));
    sce_munlock(&ecb->evs_gsemaphore);


    (*etrc)(" *event server registration tables:\n");
    ehash = (EV_EVHASH *)((PTR)ecb + ecb->evs_roffset);
    for (bkt = 0; bkt < ecb->evs_rbuckets; bkt++, ehash++)
    {
      if (ehash->ev_ehoffset == 0)			/* No event */
	continue;

      sce_mlock(&ehash->ev_ehsemaphore);		/* Lock chain */

      (*etrc)(STprintf(stbuf, "   event bucket[%d]\n", bkt));
      eptr = (EV_REVENT *)((PTR)ecb + ehash->ev_ehoffset);
      while ((PTR)eptr != (PTR)ecb)
      {
	sce_ename(eptr->ev_relname, eptr->ev_rename, evname);
	(*etrc)(STprintf(stbuf, "    event_name: %s\n",
		evname));
	(*etrc)(STprintf(stbuf, "    address: 0x%p, type: %d\n", eptr,
		eptr->ev_retype));
	(*etrc)(STprintf(stbuf, "    next(offset): %d, server(offset): %d\n",
	        eptr->ev_renextoff, eptr->ev_resvroff));
	if (eptr->ev_resvroff != 0)
	{
	  cnt = 0;
	  rptr = (EV_RSERVER *)((PTR)ecb + eptr->ev_resvroff);
	  while ((PTR)rptr != (PTR)ecb)
	  {
	    (*etrc)(STprintf(stbuf,
		    "     registered_server[%d] id: 0x%x %s %s\n",
		    cnt++, rptr->ev_rspid,
		    rptr->ev_rspid == spid ? "(CURRENT)" : "",
		    (rptr->ev_rsflags & EV_RSORPHAN) != 0 ? "(ORPHAN)" : ""));
	    (*etrc)(STprintf(stbuf,
		    "     address: 0x%p, next(offset): %d\n", rptr,
		    rptr->ev_rsnextoff));
	    /* On last registration eptr == ecb->evs_ebase */
	    rptr = (EV_RSERVER *)((PTR)ecb + rptr->ev_rsnextoff);
	  }
	}
	else
	{
	  /* Should not happen */
	  (*etrc)("     no registered server for current event (?)\n");
	}
	/* When last bucket is dumped eptr == ecb->evs_ebase */
	eptr = (EV_REVENT *)((PTR)ecb + eptr->ev_renextoff);
      }
      sce_munlock(&ehash->ev_ehsemaphore);
    } /* End of event tables */

    (*etrc)(" *event server instance tables:\n");
    shash = (EV_SVRHASH *)((PTR)ecb + ecb->evs_ioffset);
    for (bkt = 0; bkt < ecb->evs_ibuckets; bkt++, shash++)
    {
      if (shash->ev_shoffset == 0)			/* No server */
	continue;

      sce_mlock(&shash->ev_shsemaphore);		/* Lock chain */

      (*etrc)(STprintf(stbuf, "   server instance bucket[%d]\n", bkt));
      (*etrc)(STprintf(stbuf, "          offset: [%d]\n", shash->ev_shoffset));
      sentry = (EV_SVRENTRY *)((PTR)ecb + shash->ev_shoffset);
      while ((PTR)sentry != (PTR)ecb)
      {
	if (sentry->ev_svrflags & (EV_SVRLKERR | EV_SVRRQERR))
	    pidlit = "(DISABLE)";
	else if (sentry->ev_svrflags & EV_SVRTSERR)
	    pidlit = "(*ERROR*)";
	else if (sentry->ev_svrpid == spid)
	    pidlit = "(CURRENT)";
	else
	{
	    /* If the server hasn't completed its initialisation don't
	    ** bother trying to get the LK_EVCONNECT lock.
	    */
	    if (sentry->ev_svrflags & EV_SVRINIT)
	    {
	       pidlit = "(INITIALISING)";
            }
            else
            {
	       lkkey.lk_key1 = sentry->ev_svrpid;
	       clstatus = LKrequest((LK_PHYSICAL | LK_NOWAIT),
				 evc->evc_locklist, &lkkey, lkmode,
				 NULL, &lkid, 0, &clerror);
	       if (clstatus == OK)
	       {
		   /* The server is no longer alive */
		   pidlit = "(PHANTOM)";
		   clstatus = LKrelease((i4)0, evc->evc_locklist, &lkid,
				     &lkkey, NULL, &clerror);
		   if (clstatus)
		   {
		       /*
		       ** Bad news -- couldn't release the lock!
		       ** we'll mark the error, and log the error
		       ** on the next signal to the server
		       */
		       pidlit = "(LKERROR)";
		       sentry->ev_svrflags |= EV_SVRTSERR;
		   }
	       }
	       else if (clstatus == LK_BUSY)
		   pidlit = "(CONNECT)";
	       else
		   pidlit = "(UNKNOWN)";
            }
	}

	(*etrc)(STprintf(stbuf,
		"    %s: 0x%x %s %s\n", SCEV_xSVR_PID, sentry->ev_svrpid,
	        pidlit,
	        (sentry->ev_svrflags & 
		 (EV_SVRLKERR | EV_SVRRQERR | EV_SVRTSERR)) != 0 ?
		    "(LK ERROR)" : ""));
	(*etrc)(STprintf(stbuf,
		"    address: 0x%p, next(offset): %d, instance(offset): %d\n",
		sentry, sentry->ev_svrnextoff, sentry->ev_svrinstoff));
	if (sentry->ev_svrinstoff != 0)
	{
	  inst = (EV_INSTANCE *)((PTR)ecb + sentry->ev_svrinstoff);
	  while ((PTR)inst != (PTR)ecb)
	  {
	    (*etrc)(STprintf(stbuf, "     server_instance_address: 0x%p\n",
		inst));
	    sce_ename(inst->ev_inlname, inst->ev_inname, evname);
	    (*etrc)(STprintf(stbuf, "     instance_name: %s\n",
		 evname));
	    (*etrc)(STprintf(stbuf, "     datalen: %d (%d for date)\n",
		    inst->ev_inldata, sizeof(DB_DATE)));
	    if (   (inst->ev_intype == DBE_T_ALERT)
	        && (inst->ev_inldata > sizeof(DB_DATE))
	       )
	    {
	      (*etrc)(STprintf(stbuf,
		      "     data: %#s\n", inst->ev_inldata - sizeof(DB_DATE),
		      inst->ev_inname + inst->ev_inlname + sizeof(DB_DATE)));
	    }
	    (*etrc)(STprintf(stbuf, "     next(offset): %d\n",
		inst->ev_innextoff));
	    /* On last instance inst == ecb->evs_ebase */
	    inst = (EV_INSTANCE *)((PTR)ecb + inst->ev_innextoff);
	  }
	}
	else
	{
	  (*etrc)("     no server event instances\n");
	}
	/* When last bucket is dumped eptr == ecb->evs_ebase */
	sentry = (EV_SVRENTRY *)((PTR)ecb + sentry->ev_svrnextoff);
      } /* For all servers in server hash chain */
      sce_munlock(&shash->ev_shsemaphore);
    } /* For all buckets in server hash table */

    /* Now dump the server registrations that are registered for ALL events */
    (*etrc)(" *all-event-server registrations:\n");
    sce_mlock(&ecb->evs_asemaphore);
    (*etrc)(STprintf(stbuf, "   #_all_event_servers = %d\n", ecb->evs_acount));
    rptr = (EV_RSERVER *)((PTR)ecb + ecb->evs_alloffset);
    while ((PTR)rptr != (PTR)ecb)
    {
      (*etrc)(STprintf(stbuf, "    registered_all_server_id: 0x%x %s\n",
	      rptr->ev_rspid, rptr->ev_rspid == spid ? "(CURRENT)" : ""));
      (*etrc)(STprintf(stbuf, "    address: 0x%p, next(offset): %d\n", rptr,
	      rptr->ev_rsnextoff));
      /* On last registration eptr == ecb->evs_ebase */
      rptr = (EV_RSERVER *)((PTR)ecb + rptr->ev_rsnextoff);
    }
    sce_munlock(&ecb->evs_asemaphore);

    /* Now dump the free list */
    (*etrc)(" *event free space management lists:\n");
    sce_mlock(&ecb->evs_gsemaphore);
    (*etrc)(STprintf(stbuf, "   total_memory: %d, available_memory: %d\n",
	    ecb->evs_memory, ecb->evs_favailable));
    (*etrc)(STprintf(stbuf, "   free_space_root(offset): %d\n",
	    ecb->evs_foffset));
    freesp = (EV_FREESPACE *)((PTR)ecb + ecb->evs_foffset);
    cnt = 0;
    while ((PTR)freesp != (PTR)ecb)
    {
      (*etrc)(STprintf(stbuf,
	      "    free[%d]: adrs: 0x%p, offset: %d, len: %d, next(off): %d\n",
	      cnt++, freesp, ((PTR)freesp - (PTR)ecb),
	      freesp->ev_fsize, freesp->ev_fnextoff));
      /* When last free chuck is dumped freesp == ecb->evs_ebase */
      freesp = (EV_FREESPACE *)((PTR)ecb + freesp->ev_fnextoff);
    }
    sce_munlock(&ecb->evs_gsemaphore);
} /* sce_edump */

/*{
** Name: sce_mlock	- Perform a P operation on a semaphore 
**
** Description:
**      Locks a semaphore. All semaphores are acquired exclusively.
**
** Inputs:
**	sem			Pointer to semaphore to lock.
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**      30-May-95 (jenjo02)
**	    First written.
*/

VOID
sce_mlock( CS_SEMAPHORE	*sem )
{
    CSp_semaphore(TRUE, sem);
    return;
} /* sce_mlock */

/*{
** Name: sce_munlock	- Perform a V operation on a semaphore 
**
** Description:
**      Releases a semaphore.
**
** Inputs:
**	sem			Pointer to semaphore to unlock.
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**      30-May-95 (jenjo02)
**	    First written.
*/

VOID
sce_munlock( CS_SEMAPHORE	*sem )
{
    CSv_semaphore(sem);
    return;
} /* sce_munlock */

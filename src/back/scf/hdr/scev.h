/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: scev.h	- Event Handling Subsystem Internal Structures
**
** Naming Note:
** 	Note that the data structures in this file are prefixed with EV for
**	historical reasons (originally this was to be the EV module of the CL).
**	These data structures are now manipulated in sceshare.c.
**
** Description:
**      This file contains the internal structures used by the event subsystem
**	for managing active server events and server event notifications. The
**	structures described in this file are contained in shared memory for
**	use in notifying all servers on this installation of relevant events.
**
**	Vax note: Events raised on one node are transmitted to other nodes in 
**	the cluster through the CSP which cooperates in implementing cluster-
**	wide event notification.
**
** Change Note:
**	Because the data structures described in this file are all allocated
**	out of a single shared segment (shared by multiple servers) make sure
**	to synchronize modifications of the shared segment (ie, any change in
**	size or use of the structures). To avoid incompatible uses of the same
**	segment in a user installation make sure to increase evs_version - this
**	is checked at connect-time.  For developers, who do not want to create
**	a separate installation just to test modifications (and want to allow
**	other test servers to work) you can modify EV_SEGNAME (and bump the
**	suffix).
**
** Structure Description:
**	The basic structures used in the event subsystem are described below.
**	It is important to understand that all the data structures are
**	allocated out of raw memory in the shared event segment and thus
**	are pointed at by offsets (from the root) rather than pointers. 
**	Because of this, when increasing sizes confirm that all sizes are
**	aligned.  To dump the data structures in this file use:
**		SET TRACE POINT SC922		(call sce_edump)
**	Note that it is WRONG to ever store addresses in a shared memory
**	segment, as the addresses may actually be mapped out to individual
**	processes.
**
**	There are two main data structures described here.  The event
**	hash list (for servers registered for a particular event), and server
**	instance hash lists (for servers connected to EV, together with their
**	current event instances):
**
**	* A list of all events that have at least one server registered.
**	  An event may be registered by 1 or more servers.  The events
**	  are anchored to an event hash structure (EV_HASH).
**
**	EV_HASH[bkt]------->EV_REVENT---->EV_REVENT----->
**			        |	      |
**			        V	      V
**			    EV_RSERVER	  EV_RSERVER
**			        |
**			        V
**			    EV_RSERVER
**
**	* A list of currently connected servers (they are not necessarily
**	  registered for any events).  Each server has a list of outstanding
**	  event instances awaiting reception by its event thread.  These are
**	  anchored to a server hash structure (EV_SVRHASH).
**
**	EV_SVRHASH[bkt]---->EV_SVRENTRY--->EV_SVRENTRY---->
**				 |	        |
**				 V		V
**			    EV_INSTANCE	       NULL (no instances)
**			         |
**				 V
**			    EV_INSTANCE
**
**	* A server that is registered for all events (such as the CSP on a 
**	  VAXcluster) is never really physically registered for any events
**	  (through EV_HASH), but is attached to the list of all-event-servers.
**	  Its instance lists follow normal processing.
**
** Locking/Semaphores:
**	Note that all hash chains on both data structures are protected by
**	semaphores.  To protect against semaphore deadlocks, resources must
**	always be claimed in the order EV_HASH followed by EV_SVRHASH.  If
**	a semaphore is currently claimed on EV_SVRHASH, a semaphore may not
**	be claimed on EV_HASH.  The free space/global semaphore may be claimed
**	at any time but no additional semaphores may be acquired while this
**	semaphore is held.  Sempahore hierarchy:
**
**		  Event Hash	   Server Hash	     Global
**		ev_ehsemaphore -> ev_shsemaphore -> evs_gsemaphore
**
**	The all-event-registered server semaphore (evs_asemaphore) behaves
**	like the event registration semaphore (ev_ehsemaphore):
**
**		All/Event Reg	   Server Hash	     Global
**		ev_asemaphore  -> ev_shsemaphore -> evs_gsemaphore
**
**	Because the locks are shared among multiple processes take care NOT
**	to issue I/O calls while holding these locks.  Unlike single
**	process semaphores, multi-process semaphores causes "waiters" to
**	spin while waiting for the semaphore.  For example, error messages
**	should NOT be issued while holding the locks.  Trace messages, however,
**	are OK as their purpose it to debug.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
**	10-jan-91 (neil)
**	    Made instances varying-length so that more fit into default cache.
**	16-aug-91 (ralph)
**	    Added EV_SVRRQERR and EV_SVRTSERR to support phantom
**	    server detection
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Defined offset variables as SIZE_TYPE.
**      20-oct-2006 (horda03) Bug 116912
**          Added EV_SVRINIT flag to prevent a server being sent an event
**          before it is ready.
**/

/*}
** Name: EV_SCB	    - Event subsystem control block
**
** Description:
**	This control block contains the roots for the lists of event
**	registrations and active event instances.  There is exactly one
**	such control block per installation (or per node in a VAXcluster)
**	stored at the beginning of the EV shared memory segment.
**
**	Note that evs_gsemaphore (global semaphore) protects access to all
**	counters and free space lists in this SCB.  These counters are only
**	modified at the time event structures are allocated and deallocated
**	making it possible to use this semaphore without risking semaphore
**	deadlocks.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_SCB
{
			    /* Global information */
    CS_SEMAPHORE    evs_gsemaphore;	/* Global semaphore to control free
					** space and global counters/status
					*/
    i4		    evs_status;		/* Status of the event subsystem.  This
    					** is a psuedo-lock for concurrent
					** initializing servers. On shutdown
					** this is only modified under lock.
					*/
#define	    EV_ACTIVE	    	475	/* Running (some magic number) */
#define	    EV_INACTIVE	    	476	/* Not yet ready to receive requests
					** because the shared segment is either
					** being created or disposed.
					*/

    i4		    evs_version;	/* Current version of EV.  When data
					** structures change a server must be
					** able to process events for old and
					** new servers.  New servers with a
					** modified EV should increase this
					** value.  The segment name can be
					** modified to allow developers to
					** use an exclusive shared data segment
					** w/o requiring a new installation.
					*/
#define	    EV_VERSION		1
#define	    EV_SEGSIZE		12	/* Size of key plus suffix */
    char	    evs_segname[EV_SEGSIZE];
					/* Segment name for identification. See
					** comment at top of file and above
					** for a description of how/why to
					** modify this value.  Must fit in
					** above buffer.
					*/
#define	    EV_SEGNAME		"iievents.001"

    PID		    evs_orig_pid;	/* Server process id of original
					** server that initialized EV.
					*/
    LK_LLID	    evs_locklist;	/* Lock ID for event notification */

			    /* Server Event Registrations */
    i4		    evs_rbuckets;	/* # of registration hash buckets */
    SIZE_TYPE	    evs_roffset;	/* Offset to array of hash bucket
					** pointers for event registrations.
					*/
    i4		    evs_rcount;		/* Current event registration count
					** for tracing (use evs_gsemaphore)
					*/

			    /* Server Instances */
    i4	    evs_scount;		/* Current # of servers connected to EV
					** for tracing (use evs_gsemaphore).
					*/
    i4		    evs_ibuckets;	/* Number of instance hash buckets */
    i4		    evs_icount;		/* Current event instance count
					** for tracing (use evs_gsemaphore)
					*/
    i4		    evs_isuccess;	/* Total # of event instances
					** successfully dispatched for 
					** tracing (use evs_gsemaphore)
					*/
    i4		    evs_ifail;		/* Total # of event instances
					** lost due to no registrations for 
					** tracing (use evs_gsemaphore)
					*/
    SIZE_TYPE	    evs_ioffset;	/* Offset to array of hash bucket
					** pointers for event instances.
					*/

			    /* All-Event-Registered Servers */
    CS_SEMAPHORE    evs_asemaphore;	/* Semaphore to control access to
					** the list and count of all-event
					** servers.
					*/
    i4		    evs_acount;		/* Number of all-event servers in EV
					** (diag-only use evs_asemaphore).
					** Always <= evs_scount.
					*/
    SIZE_TYPE	    evs_alloffset;	/* Offset to first registered server
					** (EV_RSERVER) registered for all
					** events.  Each server on this list 
					** is also connected to the normal
					** server-instance list where it
					** collects its event instances.
					*/

			    /* Free Space Management */
    SIZE_TYPE	    evs_memory;		/* Total shared memory allocated for
					** event management.
					*/
    SIZE_TYPE	    evs_favailable;	/* Unused dynamic event space still
					** available in free space.
					*/
    SIZE_TYPE	    evs_foffset;	/* Offset to first free space block.
					** This may change while the free
					** list is manipulated.
					*/

} EV_SCB;

/*
** Server Event Registration Data Structures
*/

/*}
** Name: EV_HASH    - Define bucket containing a server-registered event.
**
** Description:
**	The events for which there are current server registrations are
**	maintained in a hash structure.  The number of hash buckets is
**	determined when the event subsystem is initialized.  The entries are
**	variable length (DBMS alerts are actually fixed, but this allows
**	for variable length events) and are identified by their offset from
**	the beginning of the event space.  Each entry in a bucket (2 events
**	that hash to the same bucket) is singly linked to its successor.
**
**	The hash chain also contains a semaphore for manipulating the contents
**	of the chain.  The semaphore must be set whenever searching or
**	modifying anything on the chain.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_HASH
{
    CS_SEMAPHORE    ev_ehsemaphore;	/* Semaphore for accessing this chain
					** and anything under the chain.
					*/
    SIZE_TYPE	    ev_ehoffset;	/* Offset to first entry (EV_REVENT)
					** in this hash bucket.
					*/
} EV_EVHASH;


/*}
** Name: EV_REVENT  - Registered event entry for the event subsystem.
**
**	The event hash chain points to a list of registered events.  Each such
**	event is represented by one of these structures.  This registered event
**	points to a list of servers currently registered to receive the
**	named event.
**
**	There are no locks on these structures since concurrency is managed
**	by the lock on the event hash bucket (ev_ehsemaphore).
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_REVENT
{
    SIZE_TYPE	ev_renextoff;		/* Offset to next event (EV_REVENT) on
					** this chain.
					*/
    SIZE_TYPE	ev_resvroff;		/* Offset to first registered server
					** (EV_RSERVER) for this event.
					*/
    i4		ev_retype;		/* Type of event.  Currently only
					** SCEV_ALERT_EVENT is allowed.
					*/
    i4		ev_relname;		/* Length of the event name (ev_rename).
					** Current implementation requires that
					** this be <= SCEV_NAME_MAX.
					*/
    char	ev_rename[SCEV_NAME_MAX]; /* Varying length name of event */
} EV_REVENT;


/*}
** Name: EV_RSERVER 	- Server entry attached to registered event.
**
**Description:
**	Each server currently registered for an event is attached to the
**	list of events.  This list is also protected by the semaphore on the
**	event hash bucket (ev_ehsemaphore).
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_RSERVER
{
    SIZE_TYPE	ev_rsnextoff;		/* Offset to next server registered 
					** for this event (EV_RSERVER)
					*/
    PID		ev_rspid;		/* Process id of this server.  This
					** is highly system dependent.
					*/
    i4		ev_rsflags;		/* To indicate certain status events */
#define		EV_RSORPHAN	0x01	/* This registered server is an orphan
					** in EV and should be cleaned up
					** and/or ignored.
					*/
} EV_RSERVER;

/*
** Server Event Instances Data Structures
*/

/*}
** Name: EV_SVRHASH	- Hash bucket entry for connected server.
**
** Description:
**	Each server that is connected to EV has an entry in a hash bucket.
**	These entries are manipulated when a server needs to be notified of
**	a registered event.  This structure defines a server hash bucket entry.
**
**	A hash bucket may contain multiple server entries (if their ids hash
**	to the same bucket).  Each server may contain 0 or more outstanding
**	event instances to be notified back to their corresponding server
**	processes.
**
**	All entries anywhere under a single hash bucket are locked by the
**	semaphore rooted in the bucket (ev_shsemaphore).
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_SVRHASH
{
    CS_SEMAPHORE    ev_shsemaphore;	/* Semaphore for servers attached
					** to this bucket and outstanding events
					** attached to those servers.
					*/
    SIZE_TYPE	    ev_shoffset;	/* Offset to first server entry
					** (EV_SVRENTRY) on this hash chain.
					*/
} EV_SVRHASH;


/*}
** Name: EV_SVRENTRY	- Description of "connected" server.
**
** Description:
**	Each server connected to EV has an entry in the EV_SVRHASH table.
**	This entry, set up when a server initializes itself with EV
**	(sce_econnect), describes the server (by ID) and includes a list of
**	all pending outstanding events for that server.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
**	16-aug-91 (ralph)
**          Added EV_SVRRQERR and EV_SVRTSERR to support phantom
**	    server detection
*/

typedef struct _EV_SVRENTRY
{
    SIZE_TYPE	ev_svrnextoff;		/* Offset to entry for next server
					** (EV_SVRENTRY)
					*/
    SIZE_TYPE	ev_svrinstoff;		/* Offset to first event instance
					** (EV_INSTANCE) queued for this
					** server - zero if no events.
					*/
    PID	    	ev_svrpid;		/* Id of attached server (sys-dep) */
    i4		ev_svrflags;		/* To indicate certain status events */
#define		EV_SVRLKERR	0x01	/* This registered server got an error
					** when sending an LKevent to it.
                                        ** When this flag is on, the EV
                                        ** subsystem will not queue any
                                        ** more event instances to the
                                        ** server.
					*/
#define		EV_SVRRQERR	0x02	/* This registered server released
					** its LK_EVCONNECT lock.  This
					** should only happen if the server
					** or event thread in the server
					** dies a sudden death without
					** disconnecting from EV.
					** When this flag is on, the EV
					** subsystem will not queue any
					** more event instances to the
					** server.
					*/
#define		EV_SVRTSERR	0x04	/* A failed attempt was made to
					** release this registered server's
					** connect lock by another server.
					** This server effectively died,
					** but the server that detected
					** the death was unable to release
					** the connect lock.
                                        ** When this flag is on, the EV
                                        ** subsystem will not queue any
                                        ** more event instances to the
                                        ** server.
                                        */
#define         EV_SVRINIT      0x08    /* Server has attached to the event
					** subsystem, but hasn't attempted
					** to acquire its LK_EVCONNECT lock.
					** No server should try to send events
					** to this server.
					*/
} EV_SVRENTRY;


/*}
** Name: EV_INSTANCE - An event instance attached to a server entry.
**
** Description:
**	An instance of an event queued for receipt by a server.  When an event
**	is raised, the server raising the event calls EV to generate one of
**	these entries for every server registered to receive the event
**	(sce_esignal).  The receiving server, then dequeues these entries
**	during event processing (sce_efetch).  These entries are linked to
**	the recieving server instance lists (EV_SVRENTRY/ev_svrinstoff).
**
**	The value field is a varying length field.  The maximum size of the
**	an instance (for initial cache allocation) is:
**	    sizeof(EV_INSTANCE) - sizeof(i4) + SCEV_BLOCK_MAX
**	The dynamic size needed for individual instance allocation is:
**	    sizeof(EV_INSTANCE) - sizeof(i4) + ev_inlname + ev_inldata
**	but the extra sizeof(i4) may be ignored as padding.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
**	10-jan-91 (neil)
**	    Made instances varying-length so that more fit into default cache.
**	    and defined EV_INST_ALLOC_MAX.
*/

typedef struct _EV_INSTANCE
{
    SIZE_TYPE	ev_innextoff;		/* Offset to next event instance for
					** the receiving server
					*/
    PID		ev_inpid;		/* Id of server that originally
					** raised this event.  Used for servers
					** that want to broadcast all events
					** though don't want to be called back
					** for their own events (CSP on Vax).
					*/
    i4		ev_intype;		/* Type of event.  Currently only
					** SCEV_ALERT_EVENT is allowed.
					*/
    i4		ev_inlname;		/* Length of event name */
    i4		ev_inldata;		/* Length of event data block.  For
					** alerters will already include the
					** date.
					*/
    char        ev_inname[sizeof(i4)];	/*
					** Varying length event name and data:
					** [0..ev_inlname-1] = name,
					** [ev_inlname..ev_inldata-1] = data
					*/
} EV_INSTANCE;

/* Maximum instance size */
#define	EV_INST_ALLOC_MAX   				\
	    (sizeof(EV_INSTANCE) - sizeof(i4) + SCEV_BLOCK_MAX)

/*
** Free Space Data Structures
*/

/*}
** Name: EV_FREESPACE - Free space nodes.
**
** Description:
**	Structure representing the free-space nodes in EV.  They start out
**	as one big chunk and start fragmenting as allocated and deallocated.
**	See sce_eallocate and sce_edeallocate for usage.
**
**	All free space is controlled by the master semaphore (evs_gsemaphore).
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified structures and completed base functionality.
*/

typedef struct _EV_FREESPACE
{
    SIZE_TYPE	ev_fsize;		/* Size of the next piece */
    SIZE_TYPE	ev_fnextoff;		/* Offset to NEXT free chuck */
} EV_FREESPACE;

/* Max buffer size to print an alert name: (name1, name2, name3)EOS */
#define	EV_PRNAME_MAX	(SCEV_NAME_MAX + 7)

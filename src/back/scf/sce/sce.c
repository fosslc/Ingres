/*
** Copyright (c) 1989, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <me.h>
#include    <pc.h>
#include    <cv.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <csp.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0m.h>
#include    <sc0e.h>
#include    <sceshare.h>
#include    <sce.h>		/* must be after sceshare.h */
#include    <scfcontrol.h>

/**
**
**  Name: sce.c - SCF alert management procedures for a single server
**
**  Description:
**      This module contains the procedures required to manage alerts
**	within SCF for registered sessions.  Currently alerts support the
**	SQL functionality asscociated with events within SCF.  Note that we
**	change terminology when entering SCF from calling these objects
**	'events' in the rest of the DBMS to calling them 'alerts' within SCF.
**	The terminology change is made so that there is less confusion at
**	the point where SCF interfaces to the event subsystem module
**	(SCESHARE).
**
**	The SCESHARE module is repsonsible for managing the propogation
**	of multi-server 'events' where such events may be a superset of the
**	SQL events understood by the rest of the DBMS.
**
**	The SCENOTIFY module is responsible for maintaining session-specific
**	notification and transmission state.
**
**  SCE Trace Points Supported:
**	SC4			Echo REGISTER, RAISE & REMOVE calls.
**	SC5			Trace session notification (in SCENOTIFY).
**	SC6			Trace alert state transitions (in SCENOTIFY).
**	SC921			Dump SCE alert data structures (sce_dump).
**	SC922			Dump EV event data structures (sce_edump).
**	SC923			Interface to sce_alter.  Key tracing is
**				"SC923 3 1" to turn on event thread tracing
**				(attached to a terminal).
**
**  Routines:
**	sce_initialize	- Initialize the SCF alert system
**     	sce_register    - Register a session to receive an alert
**	sce_remove    	- Remove a registration for an alert
**	sce_end_session - Remove all registrations for specified session(s)
**	sce_raise   	- Raise a particular alert
**	sce_shutdown	- Shut down this server's connection to EV
**	sce_thread    	- Server alert processing thread code
**	sce_alter	- Alter event processing (tracing)
**
**  Internal Routines:
**    Notification:
**	sce_qu_instance - Queue alert instance for a registered session
**    List Management:
**	sce_search    	- Search for a a registered alert
**	sce_r_remove    - Remove registration from alert/registration list
**	sce_a_remove    - Remove alert from alert/registration list
**	sce_get_node  	- Allocate new alert/registration/instance node
**	sce_free_node	- Free alert/registration/instance node
**    Tracing:
**	sce_trname   	- Trace an event name
**	sce_dump    	- Dump data structures utility
**
**  History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	19-mar-90 (rogerk)
**	    Added sce_eset calls before calls to sce_ewait to rearm event
**	    thread wait mechanism.
**	25-jan-91 (neil)
**	    1. Some statistics and activity collection for event thread.
**	    2. Pass in server memory semphore for use by EV [de]allocation.
**	    3. Add option to shut down another server connected to EV.
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**	23-Oct-1992 (daveb)
**	    name the semaphores.
**	12-Mar-1993 (daveb)
**	    Add include <me.h>
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	29-Jun-1993 (daveb)
**	    include <tr.h>; remove local extern for TRdisplay.
**	    Also, sc0e_trace and TRdisplay have different returns; can't
**	    easily use a function pointer to both, so do differently.
**	    Correctly cast args to CSget_scb.
**	7-jul-1993 (shailaja)
**	    Cast arguments for prototype compatibility
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	1-Sep-93 (seiwald)
**	    CS option cleanup: csdbms.h is no more.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered sc0e_trace calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to sc0e_trace.
**	    This works because STprintf is a varargs function in the CL.
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
**	19-sep-1995 (nick)
**	    Add call to CSswitch() after processing each event instance ; in
**	    conjunction with the reduced priority of the event thread this
**	    prevents consistency point hangs.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread() prototype.
**      23-jul-1998 (rigka01)
**          In sce_raise,
**          set event flag according to value of alert flag for the
**          purpose of passing on whether or not the event subsystem
**          should send an event to a server that is the originator
**          of the alert
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Improve performance of Events. When adding an event to a
**          session's queue, a pointer is maintained of the last event
**          in the queue (no need to traverse every entry to find the
**          last one). Improved performance of Hashing algorithm.
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    sce_register(), sce_remove(), sce_raise().
**      30-jan-2003 (chash01) In sce_search():
**          Compiler complains that *cp is never more than 8 bits long.
**          Also cp++ in the for (...) statement increment only by one
**          byte since cp is char *.  Change to i4 *ip and rewrite the 
**          loop body.
**	29-mar-2003 (hanch04 for chash01)
**	    above changed caused SEGV.
**      30-may-2002 (devjo01)
**	    Add support for Alerters in clusters.
**	22-Apr-2005 (fanch01)
**	    Add lg.h for LG_LSN type.
**	28-may-2005 (abbjo03)
**	    Correct some CSP_CX_MSG reference.
**	11-nov-2005 (devjo01)
**	    In sce_resignal, set err_code if sce_econnect fails.
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form sc0ePut().
**/

/* External and Forward declarations */
GLOBALREF SC_MAIN_CB	    *Sc_main_cb;	/* Root for SCF data */

/* Notification */
static DB_STATUS sce_qu_instance(DB_ALERT_NAME	*aname,
				 DB_DATE *awhen,
				 i4  user_lvalue,
				 char *user_value,
				 SCE_RSESSION	*reg,
				 DB_ERROR *error );


/* Number of DBevents in session's queue */
static int sce_no_entries_in_q( SCF_SESSION scse_session );

/* List management */

static DB_STATUS sce_qu_instance(DB_ALERT_NAME	*aname,
				 DB_DATE	 *awhen,
				 i4		user_lvalue,
				 char		*user_value,
				 SCE_RSESSION	*reg,
				 DB_ERROR	*error );

static VOID sce_search(SCE_HASH	*sce_hash,
		       DB_ALERT_NAME	*aname,
		       SCF_SESSION	session,
		       i4		*bucket,
		       SCE_ALERT	**alert,
		       SCE_RSESSION	**reg );

static VOID sce_a_remove(SCE_HASH	*sce_hash,
			 i4		bucket,
			 SCE_ALERT	*alert );

static VOID sce_r_remove(SCE_HASH	*sce_hash,
			 SCE_ALERT	*alert,
			 SCE_RSESSION	*reg );

static DB_STATUS sce_get_node(i4		alloc,
			      i4		*freecnt,
			      SC0M_OBJECT	**freelist,
			      i4		objsize,
			      i4		objtype,
			      i4		objtag,
			      SC0M_OBJECT	**obj );



/*
** External SCE routines
*/

/*{
** Name: sce_initialize	- Initialize the SCF alert system.
**
** Description:
**      This routine sets up the data structures required to manage alerts
**	within the server.  Essentially, this requires the hash array for
**	holding outstanding alert registrations to be allocated.  The size of
**	this hash array is function of the number of users and the event
**	system startup parameter.
**
**	Following alert hash-control initialization this routine attaches to
**	the cross-server event subsystem (sceshare).  This "attaching" may
**	actually create the shared resources if this is the first time.
**
**	This routine is expected to be called at server start-up.  Following
**	this routine the caller is expected to startup the event thread.
**
** Inputs/Globals:
**	Sc_main_cb
**	   .sc_nousers			Number of users
**	   .sc_events			Number of event buckets
**
** Outputs/Globals:
**	Sc_main_cb
**	   .sc_alert			Alert hash data structures
**	Returns:
**	    DB_STATUS			A failed alert initialization should
**					never return E_DB_FATAL errors, as
**					this should not be the cause for server
**					shutdown.
**	Errors - all of these are logged:
**	    E_SC0284_BUCKET_INIT_ERROR	Invalid number of buckets specified
**	    E_SC0273_EVENT_RETRY	Connection retry exceeded max
**	    E_SC0285_EVENT_MEMORY_ALLOC	Allocation for local alert memory
**	    E_SC0286_EVENT_SEM_INIT	Error initializing alert semaphore
**	Exceptions:
**	    None
**
** Side Effects:
**	This routines dumps any error messages to the error log rather than
**	expecting the caller to process the error.  This allows more complete
**	diagnostic information to be logged.
**
** History:
**      8-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    1. Initialize statistics.
**	    2. Pass in server memory semphore for use by EV [de]allocation.
**	23-Oct-1992 (daveb)
**	    name the semaphores.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	30-may-2002 (devjo01)
**	    Add flag parameter to distinguish sce_initialize calls
**	    on behalf of an cluster enabled RCP.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Replace evc_errf sc0e_put with sc0ePutAsFcn.
*/

DB_STATUS
sce_initialize( i4 is_an_rcp )
{
    SCE_HASH	    *sce_hash;		/* SCF alert hash CB to allocate */
    i4		    buckets;		/* Real # of buckets to use */
    i4	    hash_size;		/* Size of hash table (fixed part) */
    i4		    bno;		/* Bucket number to initialize */
    i4		    evactive;		/* EV [in]active & can[not] be used */
    i4		    retry;		/* Connection retry count */
#define	SCE_RETRY_MAX	10		/* Maximum attempts */
    DB_STATUS	    status;

    char sem_name[CS_SEM_NAME_LEN];

    /* Break out on error or success */
    for (;;)
    {
	sce_hash = Sc_main_cb->sc_alert = NULL;

	/* Not an error but treat like one */
	if ( !is_an_rcp && Sc_main_cb->sc_events == 0)
	{
	    status = E_DB_WARN;
	    break;
	}

	/* If invalid buckets log error and return - allow server to continue */
	if (Sc_main_cb->sc_events < 0)
	{
	    buckets = Sc_main_cb->sc_events;
	    sc0ePut(NULL, E_SC0284_BUCKET_INIT_ERROR, NULL, 1,
		     sizeof(buckets), (PTR)&buckets);
	    status = E_DB_ERROR;
	    break;
	}

	if ( is_an_rcp )
	{
	    /*
	    ** RCP/CSP will not be registering for alerts,
	    ** so only allocate a minimal SCE_HASH structure. 
	    */
	    buckets = 1;
	}
	else
	    buckets = max(Sc_main_cb->sc_events, Sc_main_cb->sc_nousers);

	/*
	** Allocate and initialize the alert hash array:
	**
	** The hash structure size is the size of the basic control structure
	** plus the number of bucket pointers needed.  We subtract one from the
	** number of buckets since the first pointer is allocated in the static
	** SCE_HASH structure definition.  Make sure to allocate the EV
	** "context" while we're at it.
	*/
	hash_size = sizeof(SCE_HASH) + sizeof(SCE_ALERT *) * (buckets - 1);
	status = sc0m_allocate(SC0M_NOSMPH_MASK,
			       (i4)(hash_size + sizeof(SCEV_CONTEXT)),
			       SE_HASH_TYPE,
			       (PTR) SCD_MEM, SE_HASH_TAG, (PTR *)&sce_hash);
	if (status != E_DB_OK)
	{
	    hash_size += sizeof(SCEV_CONTEXT);
	    sc0ePut(NULL, E_SC0285_EVENT_MEMORY_ALLOC, NULL, 2,
		     sizeof(hash_size), (PTR)&hash_size,
		     sizeof(buckets), (PTR)&buckets);
	    break;
	}

	/*
	** Initialize control structure.  First set the "inactive" flag to
	** avoid user sessions using events while server event thread is
	** initializing itself.
	*/
	sce_hash->sch_flags = SCE_FINACTIVE;

	sce_hash->sch_alert_free = NULL;	/* Free lists */
	sce_hash->sch_nalert_free = 0;
	sce_hash->sch_rsess_free = NULL;
	sce_hash->sch_nrsess_free = 0;
	sce_hash->sch_ainst_free = NULL;
	sce_hash->sch_nainst_free = 0;

	sce_hash->sch_raised = 0;		/* Statistics */
	sce_hash->sch_registered = 0;
	sce_hash->sch_dispatched = 0;
	sce_hash->sch_errors = 0;

	sce_hash->sch_nhashb = buckets;		/* Clear hash buckets */
	for (bno = 0; bno < buckets; bno++)
	    sce_hash->sch_hashb[bno] = NULL;

	/* Initialize semaphore for hash list locking */
	status = CSw_semaphore(&sce_hash->sch_semaphore, CS_SEM_SINGLE,
			STprintf( sem_name, "SCE hash list %p", sce_hash ));
	if (status != OK)
	{
	    sc0ePut(NULL, E_SC0286_EVENT_SEM_INIT, NULL, 1,
		     sizeof("sch_semaphore")-1, (PTR)"sch_semaphore");
	    status = E_DB_ERROR;
	    break;
	}
	

	/* Anchor the EV "context" and intialize it for attaching */
	sce_hash->sch_evc = (SCEV_CONTEXT *)((char *)sce_hash + hash_size);

	/* Clear EV roots as they will be set if we connect to EV */
	sce_hash->sch_evc->evc_root = NULL;
	sce_hash->sch_evc->evc_locklist = (LK_LLID)0;
	sce_hash->sch_evc->evc_mem_sem = &Sc_main_cb->sc_sc0m.sc0m_semaphore;

    	sce_hash->sch_evc->evc_pid = Sc_main_cb->sc_pid;	/* My pid */
    	sce_hash->sch_evc->evc_errf = (void((*)()))sc0e_putAsFcn;/* For errs */
    	sce_hash->sch_evc->evc_trcf = sc0e_trace;		/* & tracing */
	sce_hash->sch_evc->evc_flags = SCEV_DEFAULT;

	if ( !is_an_rcp )
	{
	    /*
	    ** Only try to initialize event (alert) Shared memory
	    ** segment if caller is not the RCP.  Cluster RCP will be
	    ** content to attempt to connect when it gets a request
	    ** to resignal an alert.  If the connection fails, then
	    ** no harm done, since in that case, there are no
	    ** servers registered to received the event anyway.
	    */
	    sce_hash->sch_evc->evc_rbuckets = max(buckets, SCEV_RBKT_MIN);
	    sce_hash->sch_evc->evc_mxre = buckets;
	    sce_hash->sch_evc->evc_ibuckets = SCEV_IBKT_MIN;
	    sce_hash->sch_evc->evc_mxei = max(buckets, SCEV_INST_MIN);

	    /*
	    ** Retry initialization & connection if EV is inactive.  Because
	    ** another server may be creating EV (and tags it inactive during
	    ** the creation), or it may be destroying EV (if it was the last
	    ** server to shutdown - and tags it inactive during shutdown) we try
	    ** to reinitialize and connect a few times.
	    */
	    for (retry = 0; retry < SCE_RETRY_MAX; retry++)
	    {
		/* Create EV if it doesn't exist */
		status = sce_einitialize(sce_hash->sch_evc);
		if (status != E_DB_OK)
		    break;
		/* Attach server to EV */
		status = sce_econnect(sce_hash->sch_evc, &evactive);
		if (status != E_DB_OK || evactive)
		    break;
	    } /* For # of retries */
	    if (retry >= SCE_RETRY_MAX)
	    {
		sc0ePut(NULL, E_SC0273_EVENT_RETRY, NULL, 1,
			 sizeof(retry), (PTR)&retry);
		status = E_DB_ERROR;
	    }
	    if (status != E_DB_OK)
		break;

	    sce_hash->sch_flags = SCE_FACTIVE;	/* We're done */
	}

	status = E_DB_OK;
	break;
    }
    if (status == E_DB_OK)
	Sc_main_cb->sc_alert = sce_hash;
    else if (sce_hash != NULL)
	_VOID_ sc0m_deallocate(0, (PTR *)&sce_hash);
    return (status > E_DB_ERROR ? E_DB_ERROR : status);
} /* sce_initialize */

/*{
** Name: sce_register	- Register a session to receive a named alert
**
** Description:
**      Registers a session to receive the named alert when that alert is
**	raised.  A session may only register for alerts for which it is not
**	currently registered othersise an error is issued.
**
**	SCF does not check to see if the user is authorized to perform
**	this operation. SCF assumes that this checking was done by other
**	facilities within the DBMS before calling SCF.
**
**	If this is the first alert registration in the server then this
**	routine will also register this alert with the event subsystem.
**
**	Algorithm:
**	    (P)lock hash-list & search for alert/session pair (sch_semaphore);
**	    if found error;
**	    if new alert allocate new alert entry;
**	    allocate new registration and add to alert entry;
**	    if was new alert add to event subsystem (sce_eregister);
**	    (V)unlock hash-list (sch_semaphore);
**
** Inputs:
**      scfcb			Request parameters:
**	   .scf_session		Current session id
**	   .scf_alert_parms	Full alert name
**
** Outputs:
**      scfcb.scf_error
**	    E_SC0270_NO_EVENT_MESSAGE	- Registration not allowed due to GCA
**	    E_SC0280_NO_ALERT_INIT	- No event subsystem
**	    E_SC002B_EVENT_REGISTERED	- Event already registered
**	    E_SC0281_ALERT_ALLOCATE_FAIL- Alert allocation failed
**	    E_SC0282_RSES_ALLOCATE_FAIL	- Registration node allocation failed
**	    Errors from the event subsystem
** 	Returns:
**	    DB_STATUS
** 	Exceptions:
**	    None
**
** Side Effects:
**	    May add a new alert registration to the event subsystem.
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Bump # of events registered.
*/

DB_STATUS
sce_register( SCF_CB *scfcb, SCD_SCB *scb )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    i4		    	bucket;		/* Bucket into hash table */
    SCE_ALERT	    	*alert;		/* Alert header in hash table */
    bool		new_alert;	/* New alert entry required */
    SCE_RSESSION    	*reg;		/* Registration of alert */
    SCEV_DCL_MACRO(ev_space, event);	/* Event to pass to EV */
    DB_ALERT_NAME   	*aname;		/* Name of registered alert */
    DB_STATUS		status;

    status = E_DB_OK;
    CLRDBERR(&scfcb->scf_error);

    aname = scfcb->scf_ptr_union.scf_alert_parms->scfa_name;

    if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TALERT, NULL, NULL))
	sce_trname(TRUE, "SCE_REGISTER", aname);

    /* If events not initialized then return error that request is invalid */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	SETDBERR(&scfcb->scf_error, 0, E_SC0280_NO_ALERT_INIT);
	return (E_DB_ERROR);
    }

    /*
    ** If the client has a GCA protocol level of 2 or less, then they are
    ** not expecting GCA_EVENT messages.  If the client is more recent then
    ** they can accept and process these event messages.  Version 0 (zero)
    ** seems to be used for internal development, so ignore it.
    */
    if (   scb->scb_cscb.cscb_version != 0
        && scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2
       )
    {
	SETDBERR(&scfcb->scf_error, 0, E_SC0270_NO_EVENT_MESSAGE);
	return (E_DB_ERROR);
    }

    /* Lock the server alert data structure */
    sce_mlock(&sce_hash->sch_semaphore);

    /* Loop to break if an error occurs */
    for ( ;; )
    {
	/*
	** Search the hash bucket for the named alert.  If found, return an
	** error, else insert into has bucket.
	*/
	sce_search(sce_hash, aname, scfcb->scf_session, &bucket, &alert, &reg);
	if (reg != NULL)
	{
	    /* The session is already registered for this alert */
	    SETDBERR(&scfcb->scf_error, 0, E_SC002B_EVENT_REGISTERED);
	    status = E_DB_ERROR;
	    break;
	}
	if (alert == NULL)	/* Alert doesn't exist - create a new one */
	{
	    new_alert = TRUE;
	    status = sce_get_node(FALSE, &sce_hash->sch_nalert_free,
				  (SC0M_OBJECT **)&sce_hash->sch_alert_free,
				  sizeof(*alert), SE_ALERT_TYPE, SE_ALERT_TAG,
				  (SC0M_OBJECT **)&alert);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&scfcb->scf_error, 0, E_SC0281_ALERT_ALLOCATE_FAIL);
		break;
	    }

	    /* Initialize fields and link into hash chain alert bucket */
	    STRUCT_ASSIGN_MACRO(*aname, alert->scea_name);
	    alert->scea_rsession = NULL;
	    alert->scea_next = sce_hash->sch_hashb[bucket];
	    sce_hash->sch_hashb[bucket] = alert;
	}
	else			/* Alert was found */
	{
	    new_alert = FALSE;
	} /* If needed new alert */
	/* Add the session registration for this alert/session pair */
	status = sce_get_node(FALSE, &sce_hash->sch_nrsess_free,
			      (SC0M_OBJECT **)&sce_hash->sch_rsess_free,
			      sizeof(*reg), SE_RSES_TYPE, SE_RSES_TAG,
			      (SC0M_OBJECT **)&reg);
	if (status != E_DB_OK)
	{
	    SETDBERR(&scfcb->scf_error, 0, E_SC0282_RSES_ALLOCATE_FAIL);
	    if (new_alert)	 /* Deallocate if we just got a new alert */
	    {
		_VOID_ sce_free_node(FALSE, sce_hash->sch_nhashb,
				     &sce_hash->sch_nalert_free,
				     (SC0M_OBJECT **)&sce_hash->sch_alert_free,
				     (SC0M_OBJECT *)alert);
	    }
	    break;
	}
	/* Initialize fields and attach to alert list head */
	reg->scse_session = scfcb->scf_session;
	reg->scse_flags = 0;
	reg->scse_next = alert->scea_rsession;
	alert->scea_rsession = reg;

	/* If this was a new alert then register the event with EV */
	if (new_alert)
	{
	    event->ev_type = DBE_T_ALERT;
	    event->ev_lname = sizeof(*aname);
	    event->ev_ldata = 0;
	    MEcopy((PTR)aname, sizeof(*aname), (PTR)event->ev_name);
	    status = sce_eregister(sce_hash->sch_evc, event, &scfcb->scf_error);
	    /*
	    ** Leave registration alone on failure.  Maybe we're confused
	    ** and we're already registered.
	    */
	    if (status != E_DB_OK)
		break;
	} /* If new alert */

	break;
    }

    /* Collect statistics */
    if (status == E_DB_OK)
	sce_hash->sch_registered++;
    else
	sce_hash->sch_errors++;

    /* Unlock alert lists */
    sce_munlock(&sce_hash->sch_semaphore);

    return (status);
} /* sce_register */

/*{
** Name: sce_remove	- Remove an alert registration
**
** Description:
**      This procedure removes an alert registration for the specified
**	session.   If the session is not registered for this alert then
**	an error is issued.
**
**	If this is the last registration for this alert then the server
**	deregisters the alert with EV.
**
**	Algorithm:
**	    (P)lock hash-list & search for alert/session pair (sch_semaphore);
**	    if not found error;
**	    remove registration from alert entry;
**	    if no registrations left for alert then
**		deregister from event subsystem (sce_ederegister);
**		remove alert from bucket;
**	    endif
**	    (V)unlock hash-list (sch_semaphore);
**
** Inputs:
**      scfcb			Request parameters:
**	   .scf_session		Current session id
**	   .scf_alert_parms	Full alert name
**
** Outputs:
**      scfcb.scf_error
**	    E_SC0280_NO_ALERT_INIT	   - No event subsystem
**	    E_SC002C_EVENT_NOT_REGISTERED  - Event not registered
**	    E_SC028E_XSEV_NOT_REGISTERED   - Cross-server deregistration failed
** 	Returns:
**	    DB_STATUS
** 	Exceptions:
**	    None
**
** Side Effects:
**	    May remove an alert registration from the event subsystem.
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Bump # of errors encountered.
*/

DB_STATUS
sce_remove( SCF_CB *scfcb, SCD_SCB *scb )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    i4		    	bucket;		/* Bucket into hash table */
    SCE_ALERT	    	*alert;		/* Alert header in hash table */
    SCE_RSESSION    	*reg;		/* Registration of alert */
    SCEV_DCL_MACRO(ev_space, event);	/* Event to pass to EV */
    DB_ALERT_NAME   	*aname;		/* Name of registered alert */
    DB_STATUS		status;

    status = E_DB_OK;
    CLRDBERR(&scfcb->scf_error);

    aname = scfcb->scf_ptr_union.scf_alert_parms->scfa_name;

    if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TALERT, NULL, NULL))
	sce_trname(TRUE, "SCE_REMOVE", aname);

    /* If events not initialized then return error that request is invalid */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	SETDBERR(&scfcb->scf_error, 0, E_SC0280_NO_ALERT_INIT);
	return (E_DB_ERROR);
    }

    /* Lock the server alert data strustures */
    sce_mlock(&sce_hash->sch_semaphore);

    /* Loop for breaking on errors */
    for ( ;; )
    {
	/* Search for named alert - if found remove, else return an error */
	sce_search(sce_hash, aname, scfcb->scf_session, &bucket, &alert, &reg);
	if (reg == NULL)
	{
	    /* The session is not registered for this alert */
	    SETDBERR(&scfcb->scf_error, 0, E_SC002C_EVENT_NOT_REGISTERED);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Remove the alert registration.  If this is the last registration,
	** then remove the alert from EV and from the local alert list.
	*/
	sce_r_remove(sce_hash, alert, reg);
	if (alert->scea_rsession == NULL)
	{
	    /* Inform EV of event removal */
	    event->ev_type = DBE_T_ALERT;
	    event->ev_lname = sizeof(DB_ALERT_NAME);
	    event->ev_ldata = 0;
	    MEcopy((PTR)aname, sizeof(*aname), (PTR)event->ev_name);
	    status = sce_ederegister(sce_hash->sch_evc, event,
				     &scfcb->scf_error);
	    if (status != E_DB_OK)
	    {
		/* Still go ahead and remove our alert */
	    }
	    /* Remove alert from bucket */
	    sce_a_remove(sce_hash, bucket, alert);
	}
	break;
    }

    /* Collect statistics */
    if (status != E_DB_OK)
	sce_hash->sch_errors++;

    /* Unlock the alert list */
    sce_munlock(&sce_hash->sch_semaphore);

    return (status);
} /* sce_remove */

/*{
** Name: sce_end_session	- Remove all registrations for ending session
**
** Description:
**      For each alert registration remove the alert.  If the alert is left
**	empty then, like sce_remove, remove the alert and deregister with EV.
**
**	This routine must be called from early on in scs_terminate to avoid
**	cases where registrations remian but the session is gone (for example,
**	because of crashing inside scs_terminate in another facility).  If
**	a registration remains from an invalid session, we may end up adding
**	an instance (sce_qu_instance) to an invalid address.
**
**	This routine may be called from one of a few places:
**	1. From scs_terminate because some thread is being removed:
**	1.1.  A normal user thread is terminated (the obvious one).  In this
**	      case the user session data is passed in.
**	1.2. The event thread is stopped through scs_terminate.  In this case
**	     the event thread session data is passed and we know that the event
**	     thread is being removed because the thread type is SCS_SEVENT.
**	     In this case we just shut down the alert processing instead (see
**	     next).
**	2. From sce_shutdown the whole subsystem is being disconnected:
**	2.1. The obvious case of server shutdown - remove all sessions and
**	     clean up.  This routine is called with a dummy session id
**	     (DB_NOSESSION) to indicate all sessions are to be removed (called
**	     from scd_disconnect).
**	2.2. The event thread was stopped through scs_terminate (see above)
**	     and immediately shuts down the alert processing through
**	     sce_shutdown.
**	2.3. The server alert processing is being shut down because the event
**	     thread failed (from within sce_thread).  In this case we also
**	     come through sce_shutdown.
**
**	Algorithm:
**	    if scs thread type = event then
**		shut down SCE (sce_shutdown)
**		return;
**	    endif
**	    (P)lock hash-list (sch_semaphore);
**	    for all alerts in all buckets
**		for all registrations in alert
**		    if (same session id) or (session = no-session) or (orphan)
**			remove registration;
**			if no registrations left for alert then
**			    deregister from event subsystem (sce_ederegister);
**			    remove alert from bucket;
**	    		endif
**	    	    endif
**		endfor
**	    endfor
**	    if our session = a user session then
**	        (P)lock session-instance-list (sscb_asemaphore)
**	        for each instance in list
**		    remove/free instance;
**	        endfor
**	        set list to null;
**	        (V)unlock session-instance-list
**	    endif
**	    (V)unlock hash-list (sch_semaphore);
**
** Inputs:
**	scb			Session CB or NULL (only if scf_session is
**				no-session):
**	   .sscb_stype		If this thread type is SCS_EVENT then we want
**				to shut down the whole subsystem.
**	   .sscb_alerts		List of alert instances to remove
**	scf_session		Current (about to end) session id
**				If this is no-session (DB_SESSION) then
**				removes registrations for all sessions.
**
** Outputs:
**      scfcb.scf_error
**	    E_SC028E_XSEV_NOT_REGISTERED   - Cross-server deregistration failed
**     	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    May remove an alert registration from the event subsystem.
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Bump # of errors encountered.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      25-Jul-2000 (hanal04) Bug 102108 INGSRV 1230
**          If we never managed to set up the sscb_asemaphore we can't lock
**          it and there shouldn't be any associated alerts.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**         Reset sscb_last_alert and sscb_num_alerts.
*/

DB_STATUS
sce_end_session( SCD_SCB  *scb, SCF_SESSION  scf_session )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    i4		    	bucket;		/* Bucket into hash table */
    SCE_ALERT	    	*alert;		/* Alert header in hash table */
    SCE_ALERT	    	*next_alert;	/* Next alert in list (for removal) */
    SCE_RSESSION    	*reg;		/* Registration of alert */
    SCE_RSESSION    	*next_reg;	/* Next registration (for removal) */
    SCEV_DCL_MACRO(ev_space, event);	/* Event to pass to EV */
    SCS_SSCB	    	*sscb;		/* SCS CB with session instance queue */
    SCE_AINSTANCE   	*inst;		/* Instance to remove */
    SCE_AINSTANCE	*next_inst;	/* Next instance to remove */
    DB_STATUS		status, new_status;
    DB_ERROR		error;

    /*
    ** If server startup failed before the event thread was successfully
    ** initiated (and data structures allocated) just return.  Also do not
    ** access any events if the "noevents" qualifier was used.  Note that
    ** sce_shutdown, which calls use with DB_NOSESSION inactivates the
    ** alert structures so that they can be removed without allowing new
    ** users in.
    */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (   sce_hash->sch_flags == SCE_FINACTIVE
	    && scf_session != DB_NOSESSION)
       )
    {
	return (E_DB_OK);
    }

    /*
    ** If the event thread is being terminated then shut down our connection
    ** to the event subsystem.
    */
    if (   scb != NULL
	&& scf_session != DB_NOSESSION
	&& scb->scb_sscb.sscb_stype == SCS_SEVENT
       )
    {
	return (sce_shutdown());
    }

    status = E_DB_OK;

    /* Lock the alert data structures */
    sce_mlock(&sce_hash->sch_semaphore);

    /* Walk through all buckets/alerts/registrations looking for our session */
    for (bucket = 0; bucket < sce_hash->sch_nhashb; bucket++)
    {
	alert = sce_hash->sch_hashb[bucket];
	while (alert != NULL)
	{
	    reg = alert->scea_rsession;
	    while (reg != NULL)
	    {
		next_reg = reg->scse_next;	/* In case of removal */
		/* Found match, orphan or all of them? */
		if (   reg->scse_session == scf_session
		    || (reg->scse_flags & SCE_RORPHAN)
		    || scf_session == DB_NOSESSION
		   )
		{
		    sce_r_remove(sce_hash, alert, reg);
		}
		reg = next_reg;
	    } /* While more registrations for alert */

	    next_alert = alert->scea_next;	/* In case we remove alert */
	    if (alert->scea_rsession == NULL)	/* Then toss it */
	    {
		/* Inform EV of event removal */
		event->ev_type = DBE_T_ALERT;
		event->ev_lname = sizeof(DB_ALERT_NAME);
		event->ev_ldata = 0;
		MEcopy((PTR)&alert->scea_name, sizeof(alert->scea_name),
		       (PTR)event->ev_name);
		new_status = sce_ederegister(sce_hash->sch_evc, event, &error);
		if (new_status != E_DB_OK)
		{
		    if (new_status > status)
			status = new_status;
		    /* Still continue processing - thread is about to end */
		}
		sce_a_remove(sce_hash, bucket, alert);
	    } /* If alert is left with no registrations */
	    alert = next_alert;
	} /* While more alerts in bucket */
    } /* For all buckets */

    /* If not shutdown then lock alert instance list for this session */
    if ((scf_session != DB_NOSESSION) && (scb->scb_sscb.sscb_aflags & SCS_ASEM))
    {
	sscb = &scb->scb_sscb;
        sce_mlock(&sscb->sscb_asemaphore);

	/* Remove each instance that's still queued */
	if ((inst = (SCE_AINSTANCE *)sscb->sscb_alerts) != NULL)
	{
	    while (inst != NULL)
	    {
		/* Point to next one and free current one */
		next_inst = inst->scin_next;
		_VOID_ sce_free_node(inst->scin_lvalue > 0,
				     sce_hash->sch_nhashb,
				     &sce_hash->sch_nainst_free,
				     (SC0M_OBJECT **)&sce_hash->sch_ainst_free,
				     (SC0M_OBJECT *)inst);
		inst = next_inst;
	    }
	    sscb->sscb_alerts = NULL;
            sscb->sscb_last_alert = NULL;
            sscb->sscb_num_alerts = 0;
	}
	/* Unlock alert instance list for this session */
        sce_munlock(&sscb->sscb_asemaphore);
    } /* If we were ending a user session */

    /* Collect statistics */
    if (status != E_DB_OK)
	sce_hash->sch_errors++;

    /* Unlock server alert data structures */
    sce_munlock(&sce_hash->sch_semaphore);

    return (status);
} /* sce_end_session */

/*{
** Name: sce_raise	- Raise the alert specified
**
** Description:
**      This procedure raises the named alert by either processing it
**	directly (if it is a NOSHARE event) or sending it to the EV subsystem
**	if it is a global event.  EV will then broadcast the event to all
**	servers registered for the alert including the server in which
**	we are currently running (if this server actually has registrations
**	for the alert).
**
**	Algorithm:
**	    if alert is noshare
**	        (P)lock hash-list & search for alert/session (sch_semaphore);
**		if found current registration add instance (sce_qu_instance);
**	        (V)unlock hash-list (sch_semaphore);
**	    else
**		send alert to event subsystem (sce_esignal);
**	    endif
**
** Inputs:
**      scfcb			Request parameters:
**	   .scf_session		Current session id - useful for NOSHARE.
**	   .scf_alert_parms	Full alert specification.
**
** Outputs:
**      scfcb.scf_error
**	    E_SC0280_NO_ALERT_INIT	   - No event subsystem
**	Returns:
**	    DB_STATUS	Note that although errors may occur in raising an
**			event.  These errors are only placed in the error log.
**			They may be dependent on activities in other servers
**			and thus may not be relevant to the current session.
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interaction with other threads and other servers
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Bump # of events raised/dispatched.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      23-jul-1998 (rigka01)
**          set event flag according to value of alert flag for the
**          purpose of passing on whether or not the event subsystem
**          should send an event to a server that is the originator
**          of the alert
*/

DB_STATUS
sce_raise( SCF_CB  *scfcb, SCD_SCB *scb )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    i4		    	bucket;		/* Bucket into hash table */
    SCE_ALERT	    	*alert;		/* Alert header in hash table */
    SCE_RSESSION    	*reg;		/* Registration of alert */
    SCEV_DCL_MACRO(ev_space, event);	/* Event to pass to EV */
    SCF_ALERT	    	*aparm;		/* Alert description */
    DB_ALERT_NAME   	*aname;		/* Name of registered alert */
    DB_STATUS		status;

    status = E_DB_OK;
    CLRDBERR(&scfcb->scf_error);

    aparm = scfcb->scf_ptr_union.scf_alert_parms;
    aname = aparm->scfa_name;

    if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TALERT, NULL, NULL))
	sce_trname(TRUE, "SCE_RAISE", aname);

    /* If events not initialized then return error that request is invalid */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	SETDBERR(&scfcb->scf_error, 0, E_SC0280_NO_ALERT_INIT);
	return (E_DB_ERROR);
    }

    if (aparm->scfa_flags & SCE_NOSHARE)	/* For me only? */
    {
	/* Lock the list & find a registration.  If found then queue instance */
        sce_mlock(&sce_hash->sch_semaphore);
	sce_search(sce_hash, aname, scfcb->scf_session, &bucket, &alert, &reg);
	if (reg != NULL)
	{
	    status = sce_qu_instance(aname, aparm->scfa_when,
				     aparm->scfa_text_length,
				     aparm->scfa_user_text,
				     reg, &scfcb->scf_error);
	}
        sce_munlock(&sce_hash->sch_semaphore);
    }
    else					/* Broadcast to the world */
    {
	/* Set up the event notification data to broadcast */
	event->ev_type = DBE_T_ALERT;
	/* Copy name part */
	event->ev_lname = sizeof(DB_ALERT_NAME);
	MEcopy((PTR)aname, sizeof(DB_ALERT_NAME), (PTR)event->ev_name);
	/* Copy time/date part */
	event->ev_ldata = sizeof(DB_DATE);
	MEcopy((PTR)aparm->scfa_when, sizeof(DB_DATE),
	       (PTR)(event->ev_name + sizeof(DB_ALERT_NAME)));
	/* Add on any user text values */
	if (aparm->scfa_text_length > 0)
	{
	    event->ev_ldata += aparm->scfa_text_length;
	    MEcopy((PTR)aparm->scfa_user_text, aparm->scfa_text_length,
	       (PTR)(event->ev_name + sizeof(DB_ALERT_NAME) + sizeof(DB_DATE)));
	}
        if (aparm->scfa_flags & SCE_NOORIG)
            event->ev_flags=EV_NOORIG;
        else
            event->ev_flags=0;
	if ( CXcluster_enabled() )
	{
	    i4		retval;
	    CSP_CX_MSG	msg;

	    /*
	    ** Squirrel away alert/event text where all CSP's in
	    ** cluster can retrieve it, then send a message with
	    ** coupon returned from CX to wake all CSP's (including
	    ** the one in the current node).  On each node, CSP's
	    ** message processing thread will retrieve message text,
	    ** and then will call back into SCF to issue an sce_esignal
	    ** on behalf of that node.
	    */
	    msg.csp_msg_action = CSP_MSG_6_ALERT;
	    msg.csp_msg_node = (u_i1)CXnode_number(NULL);
	    msg.alert.length = 
	     (PTR)(event->ev_name + event->ev_lname + event->ev_ldata) -
	     (PTR)event;
	    retval = CXmsg_stow( &msg.alert.chit,
	     (PTR)event, msg.alert.length );
	    if ( OK == retval )
	    {
		retval = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&msg );

		(void)CXmsg_release( &msg.alert.chit );
	    }
	    if ( OK != retval )
	    {
		SETDBERR(&scfcb->scf_error, 0, E_SC0276_CX_RAISE_FAIL);
		status = E_DB_ERROR;
	    }
	}
	else
	{
	    status = sce_esignal(sce_hash->sch_evc, event, &scfcb->scf_error);
	}
    }

    /* Collect statistics */
    sce_mlock(&sce_hash->sch_semaphore);
    if (status == E_DB_OK)
    {
	sce_hash->sch_raised++;
	if (aparm->scfa_flags & SCE_NOSHARE)	/* Also dispatched */
	    sce_hash->sch_dispatched++;
    }
    else
    {
	sce_hash->sch_errors++;
    }
    sce_munlock(&sce_hash->sch_semaphore);

    return (status);
} /* sce_raise */

/*{
** Name: sce_resignal	- Propagate an alert.
**
** Description:
**      This procedure is used by clustered Ingres to propagate
**	an alert raised on this, or another node.
**
**	It simply takes a copy of the event block originally  formatted
**	in sce_raise by the originating server, then broadcasted
**	out to all the CSP processes in the cluster, and calls sce_esignal
**	with it.
**
** Inputs:
**      scfcb			Request parameters:
**	   .scf_ptr_union.scf_buffer   Ptr to SCEV structure to be signaled.
**
** Outputs:
**      scfcb.scf_error
**	    E_SC0280_NO_ALERT_INIT	   - No event subsystem
**	Returns:
**	    DB_STATUS	Note that although errors may occur in raising an
**			event.  These errors are only placed in the error log.
**			They may be dependent on activities in other servers
**			and thus may not be relevant to the current session.
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interaction with other threads and other servers
**
** History:
**      30-may-2002 (devjo01)
**	    First written to support Alerters in clusters.
**	11-nov-2005 (devjo01)
**	    Set err_code to E_SC0280_NO_ALERT_INIT if sce_econnect fails.
*/

DB_STATUS
sce_resignal( SCF_CB  *scfcb, SCD_SCB *scb )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    SCEV_EVENT		*event;
    DB_STATUS		status;
    i4			evactive;

    status = E_DB_OK;
    CLRDBERR(&scfcb->scf_error);

    for ( ; /* something to break out of */ ; )
    {
	if ( NULL == (sce_hash = Sc_main_cb->sc_alert) )
	{
	    if ( Sc_main_cb->sc_capabilities & SC_CSP_SERVER )
	    {
		/*
		** Cluster aware CSP, must be able to resignal alerts,
		** but as of Ingres 2.6, CSP does not normally register
		** or signal alerts itself.  We perform a minimal init
		** here, which is sufficient to allow alert to be
		** resignaled.  Most important difference between
		** this init, and that done by a normal server, is
		** that the shared memory segment is neither initialized
		** nor connected to.   This is done, so that 
		** the EV SMS is allowed to be allocatted by a regular
		** DBMS server, and will be sized according to the
		** DBMS CBF parameters.
		*/
		status = sce_initialize(1);
		if (status == E_DB_WARN)
		    status = E_DB_OK;
		if ( status == E_DB_OK )
		    sce_hash = Sc_main_cb->sc_alert;
	    }
	}

	if ( !sce_hash )
	{
	    SETDBERR(&scfcb->scf_error, 0, E_SC0280_NO_ALERT_INIT);
	    status = E_DB_ERROR;
	    break;
	}

	if ( (sce_hash->sch_flags == SCE_FINACTIVE) ||
	     NULL == sce_hash->sch_evc->evc_root )
	{
	    /*
	    ** Need to connect to EV SMS.
	    */
	    status = sce_econnect(sce_hash->sch_evc, &evactive);
	    if (status != E_DB_OK || !evactive)
	    {
		/*
		** This is OK, since it is very possible that
		** an Alert is received by a CSP before any
		** Regular server has initialized the EV SMS.
		*/
		SETDBERR(&scfcb->scf_error, 0, E_SC0280_NO_ALERT_INIT);
		status = E_DB_ERROR;
		break;
	    }
	    /*
	    ** Allocate lock list.  Used only for lock probes.
	    */
	    status = sce_ewinit( sce_hash->sch_evc );
	    if ( E_DB_OK != status )
	    {
		SETDBERR(&scfcb->scf_error, 0, E_SC028A_XSEV_CRLOCKLIST);
		break;
	    }
	    sce_hash->sch_flags = SCE_FACTIVE;	/* We're done */
	}

	event = (SCEV_EVENT *)scfcb->scf_ptr_union.scf_buffer;

	if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TALERT, NULL, NULL))
	    sce_trname(TRUE, "SCE_RESIGNAL", (DB_ALERT_NAME*)(event->ev_name));

	status = sce_esignal(sce_hash->sch_evc, event, &scfcb->scf_error);
	break;
    }
    return (status);
} /* sce_resignal */


/*{
** Name: sce_shutdown	- Shut down this server's connection with EV.
**
** Description:
**	This procedure may be executed as part of the server shutting down
**	or as part of the event thread shutting down.  To avoid user threads
**	from accessing the alert data structures while we are shutting down
**	(in the case of event-thread removal) we first inactivate the
**	alert structures.
**
**	This procedure may be called twice on a normal server shutdown. The
**	first time, when the event thread is terminated and the second
**	when the server is terminated.  Note that we call this when the
**	server is terminated just in case the event thread was kicked out
**	without a chance to clean up.
**
**	Algorithm:
**	    inactivate alert structures;
**	    end all session registrations (using normal sce_end_session);
**	    disconnect from EV (sce_edisconnect);
**	    deallocate main alert hash table;
** Inputs:
**	None
**
** Outputs:
**	Returns:
**	    Any errors are logged.
**	Exceptions:
**	    None
**
** History:
**	14-feb-90 (neil)
**	    Written for alerters.
**	25-jan-91 (neil)
**	    Dump statistics.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_shutdown(void)
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */

    /* Don't bother if no alert processing */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	return (E_DB_OK);
    }
    sce_hash->sch_flags = SCE_FINACTIVE;
    /*
    ** We can only NULL out the sc_alert pointer AFTER calling sce_end_session,
    ** because that routine gets sce_hash in the exact same way as we do.
    */
    _VOID_ sce_end_session((SCD_SCB *)NULL, DB_NOSESSION);  /* All sessions */
    Sc_main_cb->sc_alert = NULL;
    _VOID_ sce_edisconnect(sce_hash->sch_evc);		/* Disconnect from EV */

    /* Dump event thread statistics */
    TRdisplay("%27*- Event Thread Statistics %27*-\n");
    TRdisplay("   Events Registered: %d,  Raised: %d,  Dispatched: %d\n",
	      sce_hash->sch_registered, sce_hash->sch_raised,
	      sce_hash->sch_dispatched);
    TRdisplay("   Event Errors Encountered: %d\n", sce_hash->sch_errors);
    TRdisplay("%79*-\n");

    _VOID_ sc0m_deallocate(0, (PTR *)&sce_hash);
    return (E_DB_OK);					/* Events are over */
} /* sce_shutdown */

/*{
** Name: sce_thread	- Special server thread for processing events
**
** Description:
**	This procedure is executed as a separate thread within the DBMS.
**	Its task is to receive notification from the event subsystem of
**	events occurring for this server.  Once such a notification is
**	received this thread distributes associated notifications to
**	client threads within this server.
**
**	This "thread" only exits (returns) when signalled by the server to
**	shutdown or when there is an internal EV error (and, of course when
**	there is a thread error).  In all cases this thread should never
**	be the cause of server shutdown so we make sure never to return
**	E_DB_FATAL errors - the event thread is a NOT a "SCS vital" task.
**
**	Algorithm:
**	    establish lock list for this thread;
**	    forever loop
**		wait for event indicated from EV (sce_ewait);
**		if (status not ok)		-- maybe shutdown
**		    endloop;
**		while more events to receive from EV (sce_efetch) loop
**		    copy event into local alert;
**		    P(hash-list) to search for alert (sch_semaphore);
**		    if there is such an alert then
**			for all registered sessions
**		    	    attach alert to session-queue (sce_qu_instance);
**			endfor
**		    endif
**		    V(hash-list) to unlock alerts (sch_semaphore);
**		endloop
**	    endloop
**
** Inputs:
**	None
**
** Outputs:
**	Errors are written to log file (status messages):
**	E_SC0271_EVENT_THREAD		For event thread removal.
**	Returns:
**	    DB_STATUS			<= E_DB_ERROR (see comment above)
**	Exceptions:
**	    None
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	19-mar-90 (rogerk)
**	    Added sce_eset routine that must be called before sce_ewait calls.
**	    Made event thread call this routine before fetching events to
**	    make sure that no events are lost due to timing problems.
**	25-jan-91 (neil)
**	    Bump # of events dispatched and track activity.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	19-sep-1995 (nick)
**	    Call CSswitch() within the fetch loop.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Before allowing other sessions to execute, release the SCH_SEMAPHORE
**          as other sessions may require the mutex.
**	13-Feb-2007 (kschendel)
**	    Don't need CSswitch unless internal threaded.
*/

DB_STATUS
sce_thread(void)
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    bool		winit = FALSE;	/* Wait list initialized? */
    i4		    	bucket;		/* Bucket into hash table */
    SCE_ALERT	    	*alert;		/* Alert header in hash table */
    SCE_RSESSION    	*reg;		/* Registration of alert */
    SCEV_DCL_MACRO(ev_space, event);	/* Event to pass to EV */
    DB_ALERT_NAME   	*aname;		/* Name of registered alert */
    DB_DATE		*awhen;		/* Timestamp of raised event */
    i4			user_lvalue;	/* Length of user text */
    char		*user_value;	/* Pointer to user text */
    i4			eoe;		/* End of events */
    SCD_SCB	    	*escb;		/* For activity tracing */
    i4			thread_down = SCE_AREM_THREAD;
    i4			eno;
    DB_STATUS	    	status, evstatus;
    DB_ERROR		error;

    status = E_DB_OK;
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	return  (status);	/* Should not really have been called */
    }

    CSget_sid(&sce_hash->sch_ethreadid);	/* Store thread id */
    CSget_scb((CS_SCB **)&escb);		/* For event thread activity */
    eno = 0;
    for (;;)					/* Spin while thread running */
    {
	if (!winit)
	{
	    /*
	    ** Note that if we can't "wait" we could still raise events,
	    ** but this is not supported as a separate feature.
	    */
	    winit = TRUE;
	    evstatus = sce_ewinit(sce_hash->sch_evc);   /* Create wait list */
	    if (evstatus != E_DB_OK)
		break;
	    /*
	    ** Arm the event wait mechanism.  After this call, any events that
	    ** occur will notify our thread so that sce_ewait will return.  If
	    ** an event occurs before we actually call sce_ewait, then it will
	    ** just return immediately when next called.
	    */
	    evstatus = sce_ewset(sce_hash->sch_evc);
	    if (evstatus != E_DB_OK)
		break;
	}

	/* Track activity */
	MEcopy("Waiting on event lock signal (LKevent)", 38,
	       escb->scb_sscb.sscb_ics.ics_act1);
	escb->scb_sscb.sscb_ics.ics_l_act1 = 38;
	/*
	** Wait for events to be signalled. Event will be terminated if
	** removed (from IIMONITOR) or server shut down.
	*/
	evstatus = sce_ewait(sce_hash->sch_evc);
	if (evstatus != E_DB_OK)
	    break;

	/*
	** Rearm the event wait mechanism so that any new events that come in
	** before we request to wait will still be noticed.  If a new event
	** comes in while we are processing, the next call to sce_ewait will
	** return immediately without waiting.  Note that if a new event
	** comes in before we are done making sce_efetch calls, then we will
	** probably process that new event in this cycle - then we will wake
	** up on the next sce_ewait call and find nothing to do.
	**
	** This rearming of the wait mechanism must be done before the last
	** sce_efetch call to avoid the case where a new event comes in just
	** after we saw there were no more events to process, but before we
	** have set up to be notified of new events.  If that happened we would
	** not notice the event until we were woken up by a 2nd new event.
	*/
	evstatus = sce_ewset(sce_hash->sch_evc);
	if (evstatus != E_DB_OK)
	    break;

	/* Turn on alert dispatch flag to lock out "event thread removers" */
	sce_hash->sch_flags |= SCE_FDISPATCH;

	/* Track activity */
	MEcopy("Dispatching event to registered session(s)", 42,
	       escb->scb_sscb.sscb_ics.ics_act1);
	escb->scb_sscb.sscb_ics.ics_l_act1 = 42;

	for (;;)				/* Loop while more events */
	{
	    /* Fetch the event and process if not end-of-events */
	    evstatus = sce_efetch(sce_hash->sch_evc, event, &eoe, &error);
	    if (evstatus != E_DB_OK || eoe)
		break;

	    eno++;

	    /* Point at the pieces of the event (assume alert) */
	    aname = (DB_ALERT_NAME *)event->ev_name;
	    awhen = (DB_DATE *)&event->ev_name[sizeof(DB_ALERT_NAME)];
	    user_lvalue = event->ev_ldata - sizeof(DB_DATE);
	    user_value =
		     &event->ev_name[sizeof(DB_ALERT_NAME) + sizeof(DB_DATE)];

	    if (sce_hash->sch_flags & SCE_FEVPRINT)	/* Print arrivals */
	    {
		/* Requires an earlier SET TRACE TERMINAL */
		TRdisplay("E> SCE/Event Thread Reception[%d]:\n", eno);
		sce_trname(FALSE, "E>  event_name", aname);
		TRdisplay("E>  event_originator: 0x%x %s\n", event->ev_pid,
			  event->ev_pid==Sc_main_cb->sc_pid ? "(CURRENT)" : "");
		if (user_lvalue > 0 && event->ev_type == DBE_T_ALERT)
		    TRdisplay("E>  event_value: %#s\n", user_lvalue,user_value);
	    } /* If tracing all event reception */

	    /* Look for ALL threads (DB_NOSESSION) registered for this event */
            sce_mlock(&sce_hash->sch_semaphore);

	    sce_search(sce_hash, aname, (SCF_SESSION)DB_NOSESSION,
		       &bucket, &alert, &reg);

	    /* We have the alert list, look for all registered threads */
	    if (alert)
	    {
		reg = alert->scea_rsession;
		while (reg != NULL)
		{
		    /* Don't add instance if we know session is orphan */
		    if ((reg->scse_flags & SCE_RORPHAN) == 0)
		    {
			status = sce_qu_instance(aname, awhen, user_lvalue,
						 user_value, reg, &error);
		    }
		    /* Status ignored because this is not our session */
		    reg = reg->scse_next;
		}
	    }

	    /* Collect statistics */
	    if (status == E_DB_OK)
		sce_hash->sch_dispatched++;
	    else
		sce_hash->sch_errors++;

            /* Release SCH semaphore before allowing other sessions
            ** to run, as they may require the semaphore.
            */
            sce_munlock(&sce_hash->sch_semaphore);

	    /* give another thread a chance */
	    if (! CS_is_mt())
		CSswitch();
	} /* While more events from EV */

	escb->scb_sscb.sscb_ics.ics_l_act1 = 0;

	sce_hash->sch_flags &= ~SCE_FDISPATCH;	/* Done dispatching alerts */

	/* Stop thread if EV found internal error (already logged) */
	if (evstatus != OK)
	    break;
    } /* While thread is running */

    escb->scb_sscb.sscb_ics.ics_l_act1 = 0;

    sc0ePut(NULL, E_SC0271_EVENT_THREAD, NULL, 1,
	     sizeof(thread_down), (PTR)&thread_down);
    _VOID_ sce_shutdown();		/* Shut down server alert processing */
    return (evstatus > E_DB_ERROR ? E_DB_ERROR : status);
} /* sce_thread */

/*{
** Name: sce_alter	- Alter event processing
**
** Description:
**	This interface is currently available through a trace point.  It also
**	may be used for cleanup purposes.  If the SCE alert (or shared
**	subsystem event) data structures get corrupted the event thread can
**	be manually removed and resurrected.
**
**	This routine confirms that a currently-dispatching event thread is
**	not removed and that two event threads are not allowed.
**
**	NOTE: This interface is also used as a means for the admin
**	      file, sceadmin.sc, to access the EV subsystem.  Do not change
**	      this interface and/or operation without reflecting the changes
**	      in sceadmin.sc.
**
** Inputs:
**	op			Trace point value (trace_cb->db_vals[0]):
**				SCE_AREM_THREAD	- Take event thread down
**				SCE_AADD_THREAD	- And bring back up
**				SCE_TMOD_THREAD	- Alter thread (unused)
**				SCE_ATRC_THREAD	- Trace event thread receptions
**				SCE_AREMEX_THREAD - Remove external EV connect
**				1000		- Help for this utility
**	value			Modifier for certain tracing (db_vals[1])
** Outputs:
**	Errors:
**	    All errors are also in the form of trace statements - this is the
**	    result of users setting a trace point:
**	    E_SC0271_EVENT_THREAD		Status message
**	    E_SC0272_EVENT_THREAD_ADD		Error adding event thread
**	Returns:
**	    VOID
**
** History:
**	14-feb-90 (neil)
**	    First written to support Alerters.
**	28-jan-91 (neil)
**	    Add option to shut down another server connected to EV.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

VOID
sce_alter( i4 op, i4	value )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    CL_SYS_ERR	        clerr;		/* Error for CS */
    i4			priority;	/* Priority of the event thread */
    GCA_LS_PARMS	local_crb;	/* To initialize the thread */
    char		*msg;		/* Trace message */
    SCD_SCB	    	*escb;		/* Event thread to remove */
    bool		override;	/* Override original "events" setting */
    SCEV_CONTEXT	ext_evc;	/* External EV context (removal) */
    STATUS		stat;
    char		stbuf[SC0E_FMTSIZE]; /* last char for `\n' */

    sce_hash = Sc_main_cb->sc_alert;
    switch (op)
    {
      case SCE_AREM_THREAD:
	if (sce_hash == NULL || sce_hash->sch_flags == SCE_FINACTIVE)
	{
	    msg = "Alert data structures are not allocated";
	}
	else if (sce_hash->sch_flags & SCE_FDISPATCH)
	{
	    msg = "Event thread is currently dispatching events";
	}
	else
	{
	    if ((escb = (SCD_SCB *)CSfind_scb(sce_hash->sch_ethreadid)) != NULL)
	    {
		CSremove(escb->cs_scb.cs_self);
		msg = "Event thread removed";
	    }
	    else
	    {
		msg = "Error acquiring handle on event thread";
	    }
	}
	sc0e_trace(STprintf(stbuf, "REMOVE Event Thread: %s.\n", msg));
        break;

      case SCE_AADD_THREAD:
	if (sce_hash != NULL)
	{
	    msg = "Alert data structures are already allocated";
	}
	else
	{
	    /* If no events were originally requested - allow them now */
	    if (Sc_main_cb->sc_events == 0)
	    {
		override = TRUE;
		Sc_main_cb->sc_events = SCE_NUM_EVENTS;
	    }
	    else
	    {
		override = FALSE;
	    }
	    if (sce_initialize(0) == E_DB_OK)
	    {
		local_crb.gca_status = 0;
		local_crb.gca_assoc_id = 0;
		local_crb.gca_size_advise = 0;
		local_crb.gca_user_name = " <Event Thread>  ";
		local_crb.gca_account_name = 0;
		local_crb.gca_access_point_identifier = "NONE";
		local_crb.gca_application_id = 0;
		/* Start event thread at higher priority than user sessions */
		priority = Sc_main_cb->sc_norm_priority + 2;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;

		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SEVENT, (CS_SID*)NULL, &clerr);
		if (stat == OK)
		{
		    sc0ePut(NULL, E_SC0271_EVENT_THREAD, NULL, 1,
			     sizeof(op), (PTR)&op);
		    if (override)
		    {
			msg = "Ignoring zero server startup 'events' setting";
			sc0e_trace(STprintf(stbuf, "ADD Event Thread: %s.\n",
			    msg));

		    }
		    msg = "Thread scheduled for execution";
		}
		else
		{
		    sc0ePut(NULL, stat, &clerr, 0);
		    sc0ePut(NULL, E_SC0272_EVENT_THREAD_ADD, NULL, 0);
		    msg = "Error adding thread - check log file";
		}
	    }
	    else
	    {
		msg = "Error initializing SCE - check log file";
	    }
	}
	sc0e_trace(STprintf(stbuf, STprintf(stbuf, "ADD Event Thread: %s.\n",
	    msg)));
        break;

      case SCE_ATRC_THREAD:
	if (sce_hash == NULL || sce_hash->sch_flags == SCE_FINACTIVE)
	{
	    msg = "Alert data structures are not allocated";
	    sc0e_trace(STprintf(stbuf, "TRACE Event Thread: %s.\n", msg));
	}
	else
	{
	    if (value)
		sce_hash->sch_flags |= SCE_FEVPRINT;
	    else
		sce_hash->sch_flags &= ~SCE_FEVPRINT;
	}
        break;

      case SCE_AREMEX_THREAD:
	if (sce_hash == NULL || sce_hash->sch_flags == SCE_FINACTIVE)
	{
	    msg = "No connection to event subsystem";
	}
	else if (sce_hash->sch_flags & SCE_FDISPATCH)
	{
	    msg = "Your event thread is currently dispatching events";
	}
	else if (value == Sc_main_cb->sc_pid)
	{
	    msg = "Invalid attempt to disconnect yourself";
	}
	else
	{
	    /* Fabricate an external EV context CB from local one */
	    STRUCT_ASSIGN_MACRO(*sce_hash->sch_evc, ext_evc);
	    ext_evc.evc_flags |= SCEV_DISCON_EXT;	/* Ext disconnect */
	    ext_evc.evc_pid = value;			/* Ext process id */
	    if (sce_edisconnect(&ext_evc) == E_DB_OK)
		msg = "External EV connection removed";
	    else
		msg = "External EV removal error (check log)";
	}
	sc0e_trace(STprintf(stbuf, "REMOVE EXTERNAL Event Thread: %s.\n", msg));
        break;

      case 999:
	sc0e_trace("Event History: 04-jan-1983 - 15-mar-1991 (Neil Goodman)\n");
	break;
      default:
	sc0e_trace(STprintf(stbuf, "ALTER Event: Invalid operation code %d.\n",
	    op));
      case 1000:
	sc0e_trace("ALTER Event Values:\n");
	sc0e_trace("   0 - Shutdown event thread\n");
	sc0e_trace("   1 - Startup event thread\n");
	sc0e_trace("   2 - Modify event thread characteristics (unused)\n");
	sc0e_trace("   3 - Trace event thread (3  1 - On, 3  0 - Off)\n");
	sc0e_trace("   4 - Cancel external EV connection (4  pid)\n");
	break;
    } /* end switch */
} /* sce_alter */

/*
** Internal Notification routines
*/

/*{
** Name: sce_qu_instance	- Queue an alert instance for a session
**
** Description:
**      Add an alert instance to a session instance queue - the session has
**	been confirmed to be registered.  This procedure may be called by
**	the event thread or in-line by the current thread (if the NOSHARE
**	flag is set).  If the target session is idle make sure to wake it
**	up using CS_NOTIFY_EVENT.
**
**	The routine assumes that the caller has locked the SCE hash control
**	structure (as nodes may be removed from the instance free list).
**	Here we must lock the session CB alert instance queue to add the
**	new instance.
**
**	Note that is it important NOT to try to attach an alert instance
**	to a session that has been disposed.  How could this occur?  A session
**	was disposed and its registrations were left in the alert lists.
**	Later the event thread wakes up with a new alert for which the disposed
**	thread is still registered.  Based on its registration we just try to
**	attach to the session CB.  We can at least test if the session CB is
**	null, but if it isn't we're gonna add some instances to the alert
**	instance queue and we want to be sure this is a valid thread.  Usually,
**	on session clean-up we get rid of all registrations.
**
**      Lock Note:
**	   The session-specific alert semaphore (sscb_asemaphore) must be used
**	   to modify:
**		1. The alert instance list (sscb_alerts)
**		2. The session-waiting state to session-notified (sscb_astate)
**
** Inputs:
**	aname			Alert instance name
**	awhen			Alert instance time-stamp (from "raiser")
**	user_lvalue		Length of user data
**	user_value		Pointer to user data (valid if length > 0)
**	reg			Registered session node:
**	    .scse_session 	Thread id of session we want to attach to.
** Outputs:
**	reg			Registered session node:
**	    .scse_flags		If session is an orphan session then set this
**				to SCE_RORPHAN.
**	error			Errors found while attaching session
**	    E_SC0283_AINST_ALLOC_FAIL	- Allocation failure
**	    E_SC0287_AINST_SESSION_ID 	- Session not valid for alert instance
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          If sscb_last_alert is NULL, then this is the first alert
**          in the queue.
*/

static DB_STATUS
sce_qu_instance(DB_ALERT_NAME	*aname,
		DB_DATE		*awhen,
		i4		user_lvalue,
		char		*user_value,
		SCE_RSESSION	*reg,
		DB_ERROR	*error )
{
    SCE_HASH	    	*sce_hash;	/* Hash table to search */
    SCD_SCB	    	*scb;		/* Real session to attach to */
    SCS_SSCB	    	*sscb;		/* SCS CB with session instance queue */
    SCE_AINSTANCE   	*inst;		/* Current instance to add */
    SCE_AINSTANCE	*ip;		/* To get to end of instance queue */
    DB_STATUS	    	status;

    status = E_DB_ERROR;
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	/* Hash control structure should be in use */
	return (status);
    }

    if ((scb = (SCD_SCB *)CSfind_scb(reg->scse_session)) == NULL)
    {
	/* Log error if not already orphaned */
	SETDBERR(error, 0, E_SC0287_AINST_SESSION_ID);
	if ((reg->scse_flags & SCE_RORPHAN) == 0)
	{
	    sc0ePut(error, 0, NULL, 1,
		     sizeof(reg->scse_session), (PTR)&reg->scse_session);
	    /* Cleaned out when next session ends (sce_end_session) */
	    reg->scse_flags |= SCE_RORPHAN;
	}
	return (status);
    }
    sscb = &scb->scb_sscb;

    /*
    ** This routine places the specified alert on the list of pending alerts
    ** for the named session.  If the session is currently in "waiting (for
    ** user data/event interrupt)" state, CSattn is called to indicate to the
    ** session it has work to do.
    */
    for (;;)				/* Break on error */
    {
	status = sce_get_node(user_lvalue > 0, &sce_hash->sch_nainst_free,
			      (SC0M_OBJECT **)&sce_hash->sch_ainst_free,
			      sizeof(*inst) + user_lvalue,
			      SE_AINST_TYPE, SE_AINST_TAG,
			      (SC0M_OBJECT **)&inst);
	if (status != E_DB_OK)
	{
	    /* Log since this work may not be associated with a session */
	    SETDBERR(error, 0, E_SC0283_AINST_ALLOCATE_FAIL);
	    sc0ePut(error, 0, NULL, 2,
		     sizeof(reg->scse_session), (PTR)&reg->scse_session,
		     sizeof(DB_ALERT_NAME), (PTR)aname);
	    status = E_DB_ERROR;
	    break;
	}

	/* Now fill in the instance structure */
	MEcopy((PTR)aname, sizeof(DB_ALERT_NAME), (PTR)&inst->scin_name);
	MEcopy((PTR)awhen, sizeof(DB_DATE), (PTR)&inst->scin_when);
	inst->scin_flags = 0;
	inst->scin_lvalue = user_lvalue;
	if (user_lvalue > 0)
	    MEcopy((PTR)user_value, user_lvalue, (PTR)inst->scin_value);
	inst->scin_next = NULL;

	/*
	** Link this alert to the SCB for the specified session.
	** First lock the session alert list.  Once locked we *know* that
	** the target thread cannot have already read the alert we just added
	** and cannot have modified the alert state setting.  This locking
	** guarantees that race conditions are handled (actually removed) by
	** making it impossible for this routine to add an entry to the list
	** and not generate an interrupt if one is needed. Note that because
	** we lock each instance (and not the set of them) we also avoid
	** generating too many notifications.
	*/
	sce_mlock(&sscb->sscb_asemaphore);

	/* Add this entry to the alert list */
	if ((ip = (SCE_AINSTANCE *)sscb->sscb_last_alert) == NULL)
	{
	    sscb->sscb_alerts = (PTR)inst;		/* First */
	}
	else
	{
	    ip->scin_next = inst;
	}
        sscb->sscb_last_alert = (PTR) inst;
        sscb->sscb_num_alerts++;

	/*
	** Alert is locked in by us, but do we need to send an interrupt
	** to the session?  Only if they've told us to (via sscb_astate).
	*/
	if (sscb->sscb_astate == SCS_ASWAITING)
	{
	    CSattn(CS_NOTIFY_EVENT, reg->scse_session);
	    SCS_ASTATE_MACRO("event_thread/notify", *sscb, SCS_ASNOTIFIED);
	    /*
	    ** If tracing event thread then indicate if we interrupted or
	    ** not.  This is added to the event reception trace in sce_thread.
	    */
	    if (sce_hash->sch_flags & SCE_FEVPRINT)
		TRdisplay("E>  (notified session: 0x%p)\n", reg->scse_session);
	}

	/* Unlock the list and alert-state flag */
	sce_munlock(&sscb->sscb_asemaphore);
	status = E_DB_OK;
	break;
    }

    return (status);
} /* sce_qu_instance */

/*
** Internal List Managament routines
*/

/*{
** Name: sce_search	- Search for a specific alert registration
**
** Description:
**      This procedure will search the list of alert registrations looking
**	for a specific alert.  There are three possible results:
**	    1. The registration exists
**	    2. The alert exists but is not registered for this session
**	    3. The alert does not exist (but return the bucket anyway)
**
** Inputs:
**	sce_hash		SCF hash structure root
**      aname    		Alert name we are searching for.
**	session			Current SCF session identifier.  If this is
**				DB_NOSESSION then stop on alert without
**				searching sessions - useful for session
**				notification.
** Outputs:
**	bucket		    	Bucket number in which alert should be.  This
**				is set regardless of whether the alert is found
**				to allow new alerts to be entered correctly.
**	alert	    		Pointer to an alert entry or NULL.  Will be set
**				if the specified alert was found, even if the
**				specified session was not registered.
**	reg			Pointer to a registration entry of NULL.  Will
**				be set if a registration was found for alert .
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Use a more efficient algorithm to calculate the alert
**          bucket. Rather than traversing the DB_ALERT_NAME one
**          char at a time, use an integer and shifting. THis
**          reduces the number of iteractions by a factor of 4 and
**          shifting/masking is more efficient. The value returned
**          by the new algorithm is the same as the old if
**          sizeof(DB_ALERT_NAME) is divisible by sizeof(i4).
**      30-jan-2003 (chash01)
**          Compiler complains that *cp is never more than 8 bits long.
**          Also cp++ in the for (...) statement increment only by one
**          byte since cp is char *.  Change to i4 *ip and rewrite the 
**          loop body.
*/

static VOID
sce_search(SCE_HASH	*sce_hash,
	   DB_ALERT_NAME	*aname,
	   SCF_SESSION	session,
	   i4		*bucket,
	   SCE_ALERT	**alert,
	   SCE_RSESSION	**reg )
{
    i4			*ip;		/* Hash backet computer */
    i4		    	tbucket;	/* Bucket corresponding to alert */
    SCE_ALERT	    	*atemp;		/* Alert search index */
    SCE_RSESSION    	*rtemp;		/* Registration search index */
    i4                  i;

    tbucket = 0;
    for( ip = (i4 *)aname, i = 0; i < sizeof(DB_ALERT_NAME) / sizeof(i4);
         i++, ip++ )
    tbucket += (*ip & 0x000000FF) + ( (*ip>>8)& 0x000000FF) +
                       ((*ip>>16) & 0x000000FF) +(*ip>>24) & 0x000000FF;

    tbucket = tbucket % sce_hash->sch_nhashb;


    *bucket = tbucket;			/* Give user the bucket number */
    *alert = NULL;
    *reg = NULL;

    /* Search the bucket for the requested alert (assume NULL termination) */
    atemp = sce_hash->sch_hashb[tbucket];
    while (atemp != NULL)
    {
	if (MEcmp((PTR)&atemp->scea_name, (PTR)aname, sizeof(*aname)) == 0)
	{
	    *alert = atemp;		/* Found a matching alert name */

	    /* First check if we can stop here (no need for session check) */
	    if (session == DB_NOSESSION)
		return;

	    /* Check if the session is registered for this alert */
	    rtemp = atemp->scea_rsession;
	    while (rtemp != NULL)
	    {
		if (rtemp->scse_session == session)
		{
		    /* We have a match, this session is registered */
		    *reg = rtemp;
		    return;
		}
		rtemp = rtemp->scse_next;
	    } /* While more registrations for alert */

	    /* Dropped through - found alert but no session */
	    return;
	}
	atemp = atemp->scea_next;
    } /* While more alerts in bucket */

    /* Got here, couldn't have found the alert either */
    return;
} /* sce_search */

/*{
** Name: sce_r_remove	- Internal routine to remove a registration
**
** Description:
**      Remove a registration and update internal registration lists
**
** Inputs:
**      sce_hash	SCF alert data structures
**	alert		Pointer to alert structure to which registration
**			applies (resolved by caller).
**	reg		Pointer to session registration to remove (resolved
**			by caller).
**
** Outputs:
**	None
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interacts with registration deallocation and free lists.
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_r_remove(SCE_HASH	*sce_hash,
	     SCE_ALERT	*alert,
	     SCE_RSESSION	*reg )
{
    SCE_RSESSION	*rtemp;		/* To search for match */

    /* Find the alert registration by comparing pointers */
    if ((rtemp = alert->scea_rsession) == reg)	/* First on the list */
    {
	alert->scea_rsession = reg->scse_next;	/* Adjust list head */
    }
    else					/* Search for it */
    {
	while (rtemp->scse_next != reg)
	    rtemp = rtemp->scse_next;
	rtemp->scse_next = reg->scse_next;	/* Adjust list entry */
    }
    /* Disconnect the input node and free it */
    reg->scse_next = NULL;
    _VOID_ sce_free_node(FALSE, sce_hash->sch_nhashb,
			 &sce_hash->sch_nrsess_free,
			 (SC0M_OBJECT **)&sce_hash->sch_rsess_free,
			 (SC0M_OBJECT *)reg);
} /* sce_r_remove */

/*{
** Name: sce_a_remove	- Internal routine to remove an alert entry
**
** Description:
**      Remove an alert list entry and update internal alert lists.
**
** Inputs:
**      sce_hash	SCF alert data structures
**	bucket		Bucket number of alert (resolved by caller).
**	alert		Pointer to alert to remove (resolved by caller).
**			The alert should have no registration but we check
**			here anyway to make sure no one slipped in while
**			a semaphore was released.
**
** Outputs:
**	None
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interacts with registration deallocation and free lists.
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

static VOID
sce_a_remove(SCE_HASH	*sce_hash,
	     i4	bucket,
	     SCE_ALERT	*alert )
{
    SCE_ALERT	    *atemp;	/* To search for match */

    /* If there are registered sessions then forget it */
    if (alert->scea_rsession != NULL)
	return;

    /* Remove the alert if there are no sessions registered */
    atemp = sce_hash->sch_hashb[bucket];
    if (atemp == alert)		/* First on list */
    {
	sce_hash->sch_hashb[bucket] = alert->scea_next; /* Adjust list head */
    }
    else			/* Search for it */
    {
	while (atemp->scea_next != alert)
	    atemp = atemp->scea_next;
	/* Should never happen that atemp == NULL, but just in case */
	if (atemp != NULL)
	    atemp->scea_next = alert->scea_next;	/* Adjust list entry */
    }
    /* Disconnect the input node and free it */
    alert->scea_next = NULL;
    _VOID_ sce_free_node(FALSE, sce_hash->sch_nhashb,
			 &sce_hash->sch_nalert_free,
			 (SC0M_OBJECT **)&sce_hash->sch_alert_free,
			 (SC0M_OBJECT *)alert);
} /* sce_a_remove */

/*{
** Name: sce_get_node	- Internal routine to allocate various nodes.
**
** Description:
**      Get a node from the various free lists (passed in by caller) or from
**	memory.  Sometimes the caller will force a memory allocation if
**	they want a varying length structure (alert instances with a value).
**
** Inputs:
**	alloc		If set then callers wants to force an allocation.
**	freecnt		Current number of nodes in free-list.
**	freelist	Free list to look at before allocating.
**	objsize		Size of object to allocate.
**	objtype/objtag	Memory type and tag for sc0m_allocate.
**
** Outputs:
**	freecnt		Decremented if a node was found on free list.
**	freelist	Adjust to point at next entry (if was not empty).
**	obj		Points to newly allocate structure
**	Returns:
**	    DB_STATUS	Allocation problems may cause failure
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interacts with callers free list and may allocate memory
**
** History:
**	14-feb-90 (neil)
**	    Written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/

static DB_STATUS
sce_get_node(i4		alloc,
	     i4		*freecnt,
	     SC0M_OBJECT	**freelist,
	     i4		objsize,
	     i4		objtype,
	     i4		objtag,
	     SC0M_OBJECT	**obj )
{
    DB_STATUS	status;				/* For sc0m_allocate */

    if (!alloc && *freelist != NULL)		/* Use free list */
    {
	*obj = (*freelist);			/* Caller node */
	*freelist = (*freelist)->sco_next;	/* Adjust free list */
	(*freecnt)--;				/* and # if nodes */
	status = E_DB_OK;
	(*obj)->sco_next = NULL;		/* Disconnect caller */
    }
    else				/* Allocate */
    {
	status = sc0m_allocate(0, (i4)objsize, 
				objtype, (PTR)0, objtag, (PTR *)obj);
    }
    return (status);
} /* sce_get_node */

/*{
** Name: sce_free_node	- Internal routine to free various nodes.
**
** Description:
**      Free a node to the various free lists (passed in by caller) or to
**	memory.  Sometimes the caller will force a memory deallocation
**	allocation if they are freeing a varying length structure (alert
**	instances with a value).
**
** Inputs:
**	dealloc		If set then callers wants to force a deallocation.
**	maxfree		Maximum number of free nodes on a list - uses same
**			logical as maxmimum number of hash buckets in server.
**	freecnt		Current number of nodes in free-list.
**	freelist	Free list to add to if not a forced deallocation
**			and the number of free nodes is less that maxfree.
**	obj		Points to object being freed.
**
** Outputs:
**	freecnt		Incremented if a node was added to free list.
**	freelist	Adjust to point at new entry (if fred to list).
**	Returns:
**	    DB_STATUS	Deallocation problems may cause failure
**	Exceptions:
**	    None
**
** Side Effects:
**	    Interacts with callers free list and may deallocate memory
**
** History:
**	14-feb-90 (neil)
**	    Written to support alerters.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_free_node(i4		dealloc,
	      i4		maxfree,
	      i4		*freecnt,
	      SC0M_OBJECT	**freelist,
	      SC0M_OBJECT	*obj )
{
    DB_STATUS	status;				/* For sc0m_allocate */

    if (!dealloc && *freecnt < maxfree)		/* Add to free list */
    {
	obj->sco_next = *freelist;		/* Attach caller to freelist */
	*freelist = obj;
	(*freecnt)++;
	status = E_DB_OK;
    }
    else				/* Really deallocate */
    {
	status = sc0m_deallocate(0, (PTR *)&obj);
    }
    return (status);
} /* sce_free_node */

/*
** Internal Tracing routines
*/

/*{
** Name: sce_trname	- Internal tracing routine to echo event name
**
** Description:
**	Echo interface call and format event name.
**
** Inputs:
**	to_sc0e		To sc0e_trace or TRdisplay.
**	header		SCE request or header in string form.
**	aname		Event name to trace for call.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	    Trace to user/terminal.
**
** History:
**	14-feb-90 (neil)
**	    Written to support alerters.
**	29-Jun-1993 (daveb)
**	    sc0e_trace and TRdisplay have different returns; can't
**	    easily use a function pointer to both, so do differently.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

VOID
sce_trname(i4 to_sc0e, char *header, DB_ALERT_NAME  *aname )
{
    char	anm[DB_MAXNAME + 1]; 	/* Buffers for name parts */
    char	onm[DB_MAXNAME + 1];
    char	dnm[DB_MAXNAME + 1];
    char	stbuf[SC0E_FMTSIZE]; /* last char for `\n' */

    MEcopy((PTR)&aname->dba_alert, DB_MAXNAME, (PTR)anm);
    anm[DB_MAXNAME] = EOS; 
    STtrmwhite(anm);
    MEcopy((PTR)&aname->dba_owner, DB_MAXNAME, (PTR)onm);
    onm[DB_MAXNAME] = EOS; 
    STtrmwhite(onm);
    MEcopy((PTR)&aname->dba_dbname, DB_MAXNAME, (PTR)dnm);
    dnm[DB_MAXNAME] = EOS; 
    STtrmwhite(dnm);
    if( to_sc0e )
    	sc0e_trace(STprintf(stbuf, "%s: (%s, %s, %s)\n",
	    header, anm, onm, dnm));
    else
    	TRdisplay("%s: (%s, %s, %s)\n", header, anm, onm, dnm);
} /* sce_trname */

/*{
** Name: sce_dump	- Dump the alert data structures for debugging
**
** Description:
**      This procedure dumps the information associated with alerts
**	within this server or the event subsystem.
**
** Inputs:
**	sce			TRUE (dump SCE), FALSE (dump EV)
**
** Outputs:
**	None
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**      08-sep-89 (paul)
**	    First written to support Alerters.
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	25-jan-91 (neil)
**	    Dump more statistics.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      28-Dec-2000 (horda03) BUG 103596 INGSRV 1349
**          Indicate the Number of Events in Event Queue.
*/

VOID
sce_dump(i4 sce)
{
    SCE_HASH	    	*sce_hash;	/* Hash table to dump */
    i4		    	bucket;		/* Current bucket number */
    SCE_ALERT	    	*alert;		/* Alert to dump */
    SCE_RSESSION    	*reg;		/* All the alerts registrations */
    SCF_SESSION		sid;		/* Tracers id */
    char		stbuf[SC0E_FMTSIZE]; /* last char for `\n' */

    if ((sce_hash = Sc_main_cb->sc_alert) == NULL)
    {
	sc0e_trace("SCE/ Server event data structures are not allocated.\n");
	return;
    }
    if (sce_hash->sch_flags == SCE_FINACTIVE)
    {
	sc0e_trace("SCE/ Server event data structures are not active.\n");
	return;
    }
    if (!sce)				/* Dump EV */
    {
	if (sce_hash->sch_evc->evc_root == NULL)
	    sc0e_trace("SCEV/ Server not connected to Event Subsystem.\n");
	else
	    sce_edump(sce_hash->sch_evc);
	return;
    }

    CSget_sid(&sid);
    sc0e_trace("SCE/ Local Alert Data Structures:\n");
    sc0e_trace(" sce_hash: SCE alert control block:\n");
    sc0e_trace(STprintf(stbuf,
		"  current_session_id = 0x%p, event_thread_id = 0x%p\n",
	       (PTR)sid, (PTR)sce_hash->sch_ethreadid));
    sce_mlock(&sce_hash->sch_semaphore);
    sc0e_trace(STprintf(stbuf,
		"  #_alert_free = %d, #_rsess_free = %d, #_ainst_free = %d\n",
	       sce_hash->sch_nalert_free, sce_hash->sch_nrsess_free,
	       sce_hash->sch_nainst_free));
    sc0e_trace(STprintf(stbuf,
       "  #_registered = %d, #_raised = %d, #_dispatched = %d, #_errors = %d\n",
       sce_hash->sch_registered, sce_hash->sch_raised,
       sce_hash->sch_dispatched, sce_hash->sch_errors));
    sc0e_trace(STprintf(stbuf, "  #_buckets = %d\n", sce_hash->sch_nhashb));
    for (bucket = 0; bucket < sce_hash->sch_nhashb; bucket++)
    {
	if (sce_hash->sch_hashb[bucket] == NULL)
	    continue;

	sc0e_trace(STprintf(stbuf, "   bucket[%d]\n", bucket));
	alert = sce_hash->sch_hashb[bucket];
	while (alert != NULL)
	{
	    sce_trname(TRUE, "     alert_name", &alert->scea_name);
	    reg = alert->scea_rsession;
	    while (reg != NULL)
	    {
		sc0e_trace(STprintf(stbuf,
			   "       registered_session = 0x%p%s, flags = 0x%x, EventsInQ = %d\n",
			   reg->scse_session,
			   reg->scse_session == sid ? " (CURRENT)" : "",
			   reg->scse_flags,
                           sce_no_entries_in_q( reg->scse_session)));
		reg = reg->scse_next;
	    }
	    alert = alert->scea_next;
	}
    }
    sce_munlock(&sce_hash->sch_semaphore);
} /* sce_dump */


/*{
** Name: sce_no_entries_in_q - Obtain number of Alerts in event queue
**
** Description:
**      Provide the number of entries in an event queue
**
** Inputs:
**      scse_session	(SCF) Session ID of the session being checked.
**
** Outputs:
**      None
**      Returns:
**          Number of alerts in Session's event queue.
**      Exceptions:
**          None
**
** History:
**
**      28-Dec-2000 (horda03)
**         Created.
*/
static int
sce_no_entries_in_q( SCF_SESSION scse_session )
{
   SCD_SCB       *scb;
   SCS_SSCB      *sscb;
   SCE_AINSTANCE *ip;
   DB_STATUS     status;
   int           entries = 0;

   if ((scb = (SCD_SCB *)CSfind_scb(scse_session)) == NULL)
   {
      /* The Session has been removed; so indicate negative count */

      return -1;
   }

   sscb = &scb->scb_sscb;

   return sscb->sscb_num_alerts;
}

/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: sceshare.h - Event Handling Subsystem Interface Structures
**
** Description:
**      This file contains the structures used to communicate with the event
**	handling subsystem. The following structures are defined:
**
** Defines:
**	SCEV_EVENT		- Structure describing a server event.
**	SCEV_DCL_MACRO 		- Macro to declare a varying length event.
**	SCEV_CONTEXT		- Event subsystem context block.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**	10-jan-91 (neil)
**	    Increased default number of instances.
**	25-jan-91 (neil)
**	    1. Pass global server memory [de]allocator semaphore to context CB
**	       to serialize all calls to MEget_pages (et al).
**	    2. Added define of a trace line for sceadmin to track.
**	30-May-95 (jenjo02)
**	    Added function prototypes for sce_mlock, sce_munlock to
**	    consolidate CSp|v_semaphore macros and function calls.
**  19-Sep-97 (merja01)
**      Increased the size of the SCEV_BLOCK_MAX.  Due to the 
**      datatype size differences for 64 bit, local storage was getting
**      overlayed in sce_raise when running with multiple DBMS servers.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      14-May-2010 (stial01)
**          Increase SCEV_BLOCK_MAX for Long IDs
**/

/*
** Maximum length for all event data.  Must include at least event name
** (DB_ALERT_NAME), date (DB_DATE) and varying length user data.  Note that
** the date is only part of alerters and need not be in all-purpose events.
**
** Generally (scfa_text_length) is limited to DB_EVDATA_MAX (256)
** but rdf event data can be bigger:
**    rdfinvalid.c rdf_inv_alert()->scf_call()->sce_raise()
** where scfa_text_length = RDF_SCE_DATASIZE ... sizeof(PID)+sizeof(RDR_RB)
**
** We don't have access to RDF structures here .. but make sure 
** SCEV_BLOCK_MAX includes sizeof big fields in RDR_RB
**
*/
#define	    SCEV_NAME_MAX	sizeof(DB_ALERT_NAME)

#define	    SCEV_BLOCK_MAX	(640 + DB_EVENT_MAXNAME + DB_OWN_MAXNAME + DB_OBJ_MAXNAME) /* max event data */

/*}
** Name: SCEV_EVENT - For communicating with the cross-server event subsystem.
**
** Description:
**	The server event subsystem distributes event notifications to servers
**	registered with it.  Event descriptions are communicated through this
**	data structure. The structure is used by servers to:
**	
**	1. Register a server to be notified when the event occurs.
**	2. Deregister a server event notification.
**	3. Signal the occurrence of an event to other servers.
**	4. Receive a signaled event.
**
**	Note that servers use the event subsystem in much the same way that
**	user sessions may use the server alert hash lists.
**
**	An event is defined by an event name.  An event may also have an
**	optional event data block associated with it.  Note that the ev_ldata
**	is zero for a register event call and can be non-zero, indicating an
**	event data block, for signal and receive calls.  If ev_ldata is zero,
**	there is no data block associated with the event.
**
**	The event subsystem is assumed to be used only by privileged servers
**	within the installation.  The use of this subsystem by any other
**	"processes" will result in security problems since the subsystem
**	does not enforce privileges on server events.  Applications will
**	the alerters mechanism implemented by the DBMS (on top of the event
**	subsystem) to communicate application event occurence.  Note that a
**	'server' requires extra privileges in order to use the event subsystem.
**	This should preclude non-privileged user applications from gaining
**	access to the subsystem.
**
** Event Names:
** 	Event names are variable length strings (not NULL terminated).  The
**	ev_lname field, defining the length of the event name, counts all
**	characters.  Event names must be unique.  The contents of event name is
** 	not interpreted by the EV subsystem and are only compared for byte
**	equality with other event names.  Typically, the event name consists of
**	the database name, event name and event owner (DB_ALERT_NAME).  This
**	combination uniquely identifies an alert in the DBMS.
**
** History:
**      12-aug-89 (paul)
**	    First written to support Alerters
**	14-feb-90 (neil)
**	    Modified interfaces and completed base functionality.
**      23-jul-1998 (rigka01)
**          add ev_flags and EV_NOORIG value for indicating that an event
**          originating with a server should not be sent by the event
**          subsystem to that server
*/

typedef struct _SCEV_EVENT
{
    PID	    ev_pid;			/* Server process id.  Used internally
					** in EV and for finding out the 
					** original event "raiser" in another
					** process.
					*/
    i4	    ev_type;			/* Server event type -- same types as
					** dbe_type of DB_IIEVENT - <dbms.h>.
					** Typically DBE_T_ALERT.
					*/
    i4      ev_flags;                   /* Request modifiers
                                        */
#define EV_NOORIG       0x01            /* Flag copied from SCF_ALERT */
    i4	    ev_lname;			/* Length of "name" data in the varying
					** length block started at "ev_name".
					*/
    i4	    ev_ldata;			/* Length of the length data block
					** inside "ev_name" that follows the
					** "name".  In the case where this is
					** an instance, the first part
					*/
    char    ev_name[1];			/*
					** Varying length data block:
					** [0..ev_lname-1] = event name,
					** [ev_lname..ev_ldata-1] = event data
					*/
} SCEV_EVENT;

/*}
** Name: SCEV_DCL_MACRO - For declaring a varying length event.
**
** Description:
**	Because the event name and data are varying length a macro is 
**	provided.  This will also allow events to grow without recoding users.
**
** History:
**	14-feb-90 (neil)
**	    Written for new alerter interfaces.
*/

#define 	SCEV_DCL_MACRO(evspace, evptr)	\
struct						\
{						\
    SCEV_EVENT	evhold;				\
    char	evdata[SCEV_BLOCK_MAX];		\
} evspace;					\
SCEV_EVENT	*evptr = &evspace.evhold


/*}
** Name: SCEV_CONTEXT	- Context structure for all access to EV.
**
** Description:
**	This control structure is used for all requests into EV.  This
**	structure includes system configuration values (used by EV).  The
**	context (once established by EV) may not be modified by the callers.
**
**	The caller will set the external context fields, such as the
**	caller id (process id), error and trace routines.
**
**	The configuration values will be used initialize EV (when the
**	first server establishes a connection - sce_einitialize).  EV 
**	contains "registrations" (requests from servers to be notified when
**	a particular event occurs) and "instances" (signaled events queued
**	waiting to be received by each server).  The caller configuration
** 	information establishes the number of registrations and instances the
** 	event subsystem can handle.  Registrations and instances are
**	maintained in hash structures.  Fields evc_rbuckets and evc_ibuckets
**	define the number of buckets in each structure.  The optimal values
**	for these parameters are enough buckets to provide separate buckets
**	for the expected number of concurrently outstanding registrations and
**	instances.  Fields evc_maxre and evc_maxei are the maximum number
**	of registrations and instances allowed in the server event subsystem.
**	The configuration values are left in the context for internal use.
**
** History:
**	14-feb-90 (neil)
**	    First written to support Alerters
**	10-jan-91 (neil)
**	    Increased default number of instances.
**	25-jan-91 (neil)
**	    1. Add global server memory [de]allocator semaphore (evc_mem_sem)
**	       to serialize all calls to MEget_pages (et al).
**	    2. Added SCEV_DISCON_EXT to detach external servers from EV.
**	2-Jul-1993 (daveb)
**	    remove useless/incorrect 'typedef' before structure definition.
*/

#define		SE_EVCONTEXT_TAG	/* For conditional compilations */

typedef struct _SCEV_CONTEXT
{
    /*
    ** Root of data structures used for multi-server alert/event notifications.
    ** This data is comprised of the pointer to the shared memory area used
    ** by the multi-server event subsystem and the id of the lock list used
    ** by the servers to signal the occurrence of cross-server events.
    */
    PTR		evc_root;		/* Root of EV shared memory segment */
    LK_LLID	evc_locklist;		/* Lock list used for LKevent.  In the
					** case of a DBMS server this must be
					** established by the same event thread
					** thread that waits for events.
					*/
    CS_SEMAPHORE *evc_mem_sem;		/* Memory semaphore passed in by caller
					** to serialize calls to ME routines.
					*/
    /*
    ** Caller data that must be present for all calls.
    */
    PID		evc_pid;		/* Process id of requestor. EV uses
					** this depending on the request.
					** In the case of "external disconnect"
					** this will point at another process.
					*/
    VOID	(*evc_errf)();		/* Error function that EV can call.
					** Call format must identical to
					** SCF sc0e_put.
					*/
    VOID	(*evc_trcf)();		/* Trace function that EV can call.
					** Call format must identical to
					** SCF sc0e_trace.
					*/
    /*
    ** Data used to initialize EV and left in the context for EV internal use.
    */
    i4		evc_flags;		/* Status flags of request block */
#define		SCEV_DEFAULT	0x00	/* Normal connection - no tricks */
#define		SCEV_REG_ALL	0x01	/* Server requesting to connect &
					** register or disconnect for ALL
					** events.
					*/
#define		SCEV_DISCON_EXT	0x02	/* Request to disconnect an
					** external server.  In this case
					** evc_pid points at another process.
					*/
    i4		evc_rbuckets;		/* # of event registration buckets
					** (for initialization)
					*/
    i4		evc_mxre;		/* Maximum # of registered events
					** (for initialization)
					*/
#define		SCEV_RBKT_MIN	59	/* Minumum # of registered events
					** to allocate (evc_mxre >= RBKT_MIN).
					** Prime number because of modulo
					** hash.
					*/
    i4		evc_ibuckets;		/* # of server event instance buckets
					** (for initialization)
					*/
#define		SCEV_IBKT_MIN	11	/* Minumum # of connected servers to
					** allocate (evc_ibuckets >= IBKT_MIN).
					** Prime number because of modulo
					** hash.
					*/
    i4		evc_mxei;		/* Maximum # of event instances
					** (for initialization)
					*/
#define		SCEV_INST_MIN	200	/* Minumum # of outstanding event
					** instances that a server may not yet
					** have dispatched out of EV:
					** 	evc_mxei >= INST_MIN
					*/
} SCEV_CONTEXT;

/*
** SCEV_xYYY strings - Special trace lines to track in sceadmin.sc
*/
#define	SCEV_xSVR_PID	"server_process_id"

/* FUNC_EXTERNS */

FUNC_EXTERN DB_STATUS sce_econnect( SCEV_CONTEXT *evc, i4  *evactive );

FUNC_EXTERN DB_STATUS sce_eregister( SCEV_CONTEXT *evc,
				    SCEV_EVENT *event,
				    DB_ERROR *error );

FUNC_EXTERN DB_STATUS sce_ederegister(SCEV_CONTEXT  *evc,
				      SCEV_EVENT *event,
				      DB_ERROR  *error );


FUNC_EXTERN DB_STATUS sce_esignal( SCEV_CONTEXT *evc,
				  SCEV_EVENT  *event,
				  DB_ERROR  *error );

FUNC_EXTERN DB_STATUS sce_edisconnect( SCEV_CONTEXT  *evc );

FUNC_EXTERN DB_STATUS sce_ewinit( SCEV_CONTEXT *evc );

FUNC_EXTERN DB_STATUS sce_ewset( SCEV_CONTEXT *evc );

FUNC_EXTERN DB_STATUS sce_ewait( SCEV_CONTEXT *evc );

FUNC_EXTERN DB_STATUS sce_efetch(SCEV_CONTEXT *evc,
				 SCEV_EVENT *event,
				 i4  *end_of_events,
				 DB_ERROR *error );

FUNC_EXTERN VOID sce_edump( SCEV_CONTEXT *evc );

FUNC_EXTERN DB_STATUS sce_einitialize( SCEV_CONTEXT *evc );

FUNC_EXTERN VOID sce_edump( SCEV_CONTEXT  *evc );

FUNC_EXTERN VOID sce_mlock( CS_SEMAPHORE *sem );

FUNC_EXTERN VOID sce_munlock( CS_SEMAPHORE *sem );

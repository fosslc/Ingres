/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: sce.h - Header file for SCF user/session alert management.
**
** Description:
**	This files defines data structures and interface CB's to be
**	used within the SCF session alert management module (sce.c).
**	Refer to other SCE files for installation event subsystem source.
**
**	To dump the data structures described in this file use:
**		SET TRACE POINT SC921 		(call sce_dump).
**
** Defines (in order):
**	SCE_RSESSION	- "Session registered" tag attached to an alert
**	SCE_ALERT	- Server "registered alert" structure
**	SCE_AINSTANCE	- Alert notification instance for a session
**	SCE_HASH	- Global server hash table for alert management.
**
** History:
**	08-sep-89 (paul)
**	    First written for Alerters.
**	14-feb-90 (neil)
**	    Modified for new interfaces and cleanup.
**	25-jan-91 (neil)
**	    1. Added some statistics collecting.
**	    2. Added SCE_AREMEX_THREAD to cancel external EV connection.
**	13-jul-92 (rog)
**	    Added the SCE_FORMAT_* masks to control printing of the
**	    scs_format info.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    sce_register(), sce_remove(), sce_raise().
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SCE_RSESSION SCE_RSESSION;
typedef struct _SCE_ALERT SCE_ALERT;
typedef struct _SCE_AINSTANCE SCE_AINSTANCE;
typedef struct _SCE_HASH SCE_HASH;

/*}
** Name: SCE_RSESSION - Description of session registered for an alert
**
** Description:
**      Each session registered for a specific alert has one of these
**	entries linked to the SCE_ALERT structure describing the alert.
**	For a particular alert there may be a number of sessions registered
**	(requiring a list of SCE_RSESSION nodes).
**
**	This node is attached to the alert structure when a REGISTER EVENT
**	is issued (sce_register) and removed on REMOVE EVENT (sce_remove)
**	or the end of a session.
**
** History:
**	08-sep-89 (paul)
**	    First written for Alerters.
**	14-feb-90 (neil)
**	    Modified for new interfaces and cleanup.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
struct _SCE_RSESSION
{
    /* Fixed header for sc0m and free list memory management */
    SCE_RSESSION    *scse_next;
    SCE_RSESSION    *scse_prev;
    SIZE_TYPE	    scse_length;
    i2		    scse_type;
#define	    SE_RSES_TYPE   0x400	/* Internal ID for structure */
    i2		    scse_s_reserved;
    PTR		    scse_l_reserved;
    PTR		    scse_owner;
    i4		    scse_tag;
#define	    SE_RSES_TAG    CV_C_CONST_MACRO('s','c','e','r')

    SCF_SESSION	    scse_session;	/* Registered session identifier */
    i4		    scse_flags;
#define	    SCE_RORPHAN	    0x01	/* Registration is orphaned from
					** any session.  This flag is set
					** to suppress errors and/or clean up.
					*/
};

/*}
** Name: SCE_ALERT  - Structure containing a registered alert
**
** Description:
**      This structure contains the information associated with an alert
**	event for which at least one session in this server is registered.
**	The alerts are held in a hash structure (SCE_HASH), hashed on the
**	alert name (database, event & owner).  Attached to this alert structure
**	is a list of sessions in the server currently registered to receive
**	this alert.
**
**	This node is attached to the hash table when the first REGISTER EVENT
**	is issued (sce_register) for the alert, and removed when the last
**	REMOVE EVENT (sce_remove) is issued.
**
** History:
**	08-sep-89 (paul)
**	    First written for Alerters.
**	14-feb-90 (neil)
**	    Modified for new interfaces and cleanup.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
struct _SCE_ALERT
{
    /* Fixed header for sc0m and free list memory management */
    SCE_ALERT	    *scea_next;
    SCE_ALERT	    *scea_prev;
    SIZE_TYPE	    scea_length;
    i2		    scea_type;
#define	    SE_ALERT_TYPE   0x401	/* Internal ID for structure */
    i2		    scea_s_reserved;
    PTR		    scea_l_reserved;
    PTR		    scea_owner;
    i4		    scea_tag;
#define	    SE_ALERT_TAG    CV_C_CONST_MACRO('s','c','e','a')

    DB_ALERT_NAME   scea_name;		/* Name of the alert */
    i4		    scea_flags;		/* Alert flags (unused now) */
    SCE_RSESSION    *scea_rsession;	/* List of registered sessions */
};

/*}
** Name: SCE_AINSTANCE - Alert instance for a particular session
**
** Description:
**      This structure describes an alert that has actually occurred and has
**	been put on the alert notification queue of a particular registered
**	session.  It contains the alert name and the user data associated with
**	the alert (time the alert occurred, user specified text, etc.)
**	
** 	Note that the string value is not a pointer because we need to allocate
**	dynamic strings and we don't want to make each dynamic string have
**	the fixed sc0m header.
**
**	This node is attached to a session's alert instance queue when the
**	event is received from the event subsystem.  It is removed from
**	the session's queue when it is later sent to the session's client.
**
** History:
**	08-sep-89 (paul)
**	    First written for Alerters.
**	14-feb-90 (neil)
**	    Modified for new interfaces and cleanup.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
struct _SCE_AINSTANCE
{
    /* Fixed header for sc0m and free list memory management */
    SCE_AINSTANCE   *scin_next;
    SCE_AINSTANCE   *scin_prev;
    SIZE_TYPE	    scin_length;
    i2		    scin_type;
#define	    SE_AINST_TYPE   0x402	/* Internal ID for structure */
    i2		    scin_s_reserved;
    PTR		    scin_l_reserved;
    PTR		    scin_owner;
    i4		    scin_tag;
#define	    SE_AINST_TAG    CV_C_CONST_MACRO('s','c','e','i')

    DB_ALERT_NAME   scin_name;          /* Name of the alert */
    DB_DATE	    scin_when;		/* Time stamp when event was raised */
    i4		    scin_flags;		/* Alert instance flags (unused now) */
    i4		    scin_lvalue;	/* Length of user specified value */
    char	    scin_value[1];	/* String of length scin_lvalue, not
					** null terminated.  If scin_lvalue is
					** zero then this field is not really
					** defined.  If scin_lvalue is not zero
					** then this instance is never put on
					** the instance free list.
					*/
};

/*}
** Name: SCE_HASH - Control structure for hash array of alert registrations
**
** Description:
**      This structure contain the root of the hash array. The actual size
**	of this structure is dependent on the number of hash buckets
**	requested at server initialization.
**
**	This structure also contains free lists for currently unused
**	alert entries, registration entries, and instance entries. This allows
**	us to allocate them as needed but not free them unless we are
**	collecting too many free entries (more than the default number
**	of buckets).  This is done to avoid memory fragmentation for the
**	relatively small nodes on alert lists.
**
**	The "context" of the multi-server event subsystem is also pointed at
**	by this structure so that each server (with the correct locks)
**	can access the shared event subsystem.
**
** History:
**	08-sep-89 (paul)
**	    First written for Alerters.
**	14-feb-90 (neil)
**	    Modified for new interfaces and cleanup.
**	25-jan-91 (neil)
**	    Added some statistics collecting (raised, registered, dispatched &
**	    errors).
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/

#ifndef	    SE_EVCONTEXT_TAG
# define  	SCEV_CONTEXT  i4
#endif

struct _SCE_HASH
{
    /* Fixed header for sc0m memory management */
    SCE_HASH	    *sch_next;
    SCE_HASH	    *sch_prev;
    SIZE_TYPE	    sch_length;
    i2		    sch_type;
#define	    SE_HASH_TYPE   0x403
    i2		    sch_s_reserved;
    PTR		    sch_l_reserved;
    PTR		    sch_owner;
    i4		    sch_tag;
#define	    SE_HASH_TAG	    CV_C_CONST_MACRO('s','c','e','h')

    SCEV_CONTEXT    *sch_evc;		/* EV context & root for all
					** interactions between SCE and EV.
					*/

    CS_SEMAPHORE    sch_semaphore;	/* Controls access to hash lists and
					** global information.
					*/

    /* Free lists for alerts, registrations and instances */
    SCE_ALERT	    *sch_alert_free;	/* Alert entry free list */
    i4		    sch_nalert_free;	/* Number of free alerts */
    SCE_RSESSION    *sch_rsess_free;	/* Alert registration free list */
    i4		    sch_nrsess_free;	/* Number of free registrations */
    SCE_AINSTANCE   *sch_ainst_free;	/* Alert instance free list (only used
					** if event string value is unused)
					*/
    i4		    sch_nainst_free;	/* Number of free instances */

    /* Status and global information */
    i4	    sch_flags;		/* Flag used to indicate context */
#define		     SCE_FINACTIVE	0x00	/* During initialization turn
						** this on because startup is
						** done by a separate thread,
						** and users may not access
						** events while thread is
						** establishing itself.
						*/
#define		     SCE_FACTIVE	0x01	/* After successful startup */
#define		     SCE_FDISPATCH	0x02	/* Event thread is currently
						** dispatching events. Lock
						** out users from removing
						** the thread.
						*/
#define		     SCE_FEVPRINT	0x04	/* Event thread traces all 
						** its event receptions.
						*/
    SCF_SESSION	    sch_ethreadid;	/* Session id of event thread */

    /* Global statistics collection */
    i4		    sch_registered;	/* Total # of events registered */
    i4		    sch_raised;		/* Total # of events raised */
    i4		    sch_dispatched;	/* Total # of events dispatched by
					** event thread.
					*/
    i4		    sch_errors;		/* Total # of errors encountered during
					** event processing.
					*/

    /*
    ** The following array must be the last element in this structure.
    ** When this structure is initially allocated, it's size is determined
    ** dynamically depending on the number of buckets specified.  The
    ** buckets are allocated contiguous following this first bucket.
    */
#define		    SCE_NUM_EVENTS 40	/* Default event buckets */
    i4		    sch_nhashb;		/* Number of alert hash buckets */
    SCE_ALERT	    *sch_hashb[1];	/* Array of alert hash bucket headers
					** the actual length of this array
					** is determined at server startup
					*/
};

/*
** Flags to pass into SCE/sce_alter for thread manipulation.  These are
** trace values.
*/
# define	   SCE_AREM_THREAD	0	/* Take thread down */
# define	   SCE_AADD_THREAD	1	/* Bring thread up */
# define	   SCE_AMOD_THREAD	2	/* Modify thread */
# define	   SCE_ATRC_THREAD	3	/* Trace thread reception */
# define	   SCE_AREMEX_THREAD	4	/* Cancel external connection */

/*
** Flags to control printing of the scs_format info.
*/
# define	   SCE_FORMAT_NEEDNUL	0x0001	/* Need NULL terminator */
# define	   SCE_FORMAT_GCA	0x0002	/* Output goes back via GCA */
# define	   SCE_FORMAT_ERRLOG	0x0004	/* Output goes to logfile */

/* FUNC EXTERNS */

FUNC_EXTERN VOID sce_trname(i4 to_sc0e,
			    char *header,
			    DB_ALERT_NAME *aname );

FUNC_EXTERN DB_STATUS sce_notify( SCD_SCB *scb );

FUNC_EXTERN VOID sce_trstate(char  *title, i4   cur, i4  new );

FUNC_EXTERN DB_STATUS sce_shutdown(void);

FUNC_EXTERN DB_STATUS sce_initialize(i4);

FUNC_EXTERN DB_STATUS sce_end_session( SCD_SCB  *scb, SCF_SESSION  scf_session );

FUNC_EXTERN DB_STATUS sce_thread(void);

FUNC_EXTERN VOID sce_alter( i4 op, i4	value );

FUNC_EXTERN VOID sce_dump(i4 sce);

FUNC_EXTERN VOID sce_chkstate( i4  input_state, SCD_SCB *scb );

FUNC_EXTERN DB_STATUS sce_free_node(i4		dealloc,
				    i4		maxfree,
				    i4		*freecnt,
				    SC0M_OBJECT	**freelist,
				    SC0M_OBJECT	*obj );

FUNC_EXTERN DB_STATUS	    sce_register( SCF_CB *cb,
					  SCD_SCB *scb );
FUNC_EXTERN DB_STATUS	    sce_remove( SCF_CB *cb,
					SCD_SCB *scb );
FUNC_EXTERN DB_STATUS	    sce_raise( SCF_CB *cb,
				       SCD_SCB *scb );


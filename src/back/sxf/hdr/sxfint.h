/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/*
** Name: SXFINT.H - data definitions for SXF internal use
**
** Description:
**	This file contains the typdefs for internal data structures
**	used by SXF.
**	
**	The following control blocks are defined:-
**	    SXF_SVCB -	    SXF sever control block
**	    SXF_SCB -	    SXF session control block
**	    SXF_RSCB -	    SXF record stream control block
**	    SXF_ASCB -	    SXF audit state control block
**	    SXF_DBCB -	    SXF database control block
**
** History:
**	9-july-1992 (markg)
**	    Initial creation.
**	24-sep-1992 (markg)
**	    Add function prototypes for SXF routines.
**	26-nov-1992 (markg)
**	    Added definitions and prototypes for SXAS routines.
**	14-dec-1992 (robf)
**	    Added definitions for statistics gathering.
**	17-dec-1992 (robf)
**	    Define better values for memory allocation.
**	13-apr-1993 (robf)
**	    Add function protoypes for SXM & SXL routines
**	10-may-1993 (robf)
**	    Add SXF_DBCB definitions
**	    Add SXLC_LOCK for SXL cache lock
**	4-oct-93 (stephenb)
**	    Add SXA_EVENT_WAKEUP so that we don't need to use a pointer value
**	    for LKevent() event value parameters.
**	9-feb-94 (robf)
**          Add prototype for sxc_mo_attach()
**	3-june-94 (robf)
**          Increased default SXF memory size from 10K to 30K
**	    we were experiencing memory problems on HP with the default
**	    in C2 mode when running dmfjsp.
**	 1-nov-95 (nick)
**	    Move some definitions to sxf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-feb-2001 (somsa01)
**	    Synchronized sxf_l_reserved and sxf_owner with DM_SVCB.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	7-oct-2004 (thaju02)
**	    Define memory pool size as SIZE_TYPE.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing prototypes.
*/

/*
** Defines of symbolic constants used in SXF
*/
# define	SXF_READ		1
# define	SXF_WRITE		2
# define	SXF_SCAN		1
# define	SXF_BYTES_PER_SERVER    30000	/* Each server needs 30K */
# define 	SXF_BYTES_PER_SES	10000	/* Each session needs 10K */
# define	SXF_LABEL_CACHE_PER_DB	100	/* 100 labels per database */
# define	SXF_LABEL_CACHE_MIN	10	/* At least 10 labels per db*/

/*
** These are state special lock values. Special lock values are used to
** signal states requiring special handling, and must always be less  than
** SXAS_MIN_LKVAL.
*/
# define	SXAS_SUSPEND_LKVAL	10	/* Locking is suspended */
# define	SXAS_STOP_LKVAL		11	/* Locking is stopped */

# define	SXAS_MIN_LKVAL		100	/* Minimum regular low lock 

/*
** The following definition is used to determine the event type for the SXA
** lock events. Currently there seems to be no method for allocation event
** types. The value of this event type does not coflict with any others
** currently in use.
*/
# define	SXF_EVENT_MASK		(DB_SXF_ID << 16)
# define	SXA_EVENT		(SXF_EVENT_MASK + 1)


# define	SXA_EVENT_RESUME	100	/* RESUME audit event */
# define	SXA_EVENT_WAKEUP	101  /* Audit thread wakeup event */


/*
** Name: SXF_DBCB - a database control block used in SXF
**
** Description:
**	This structure defines a database control block for use in SXF.
**	One of these structures will exist for each active database in SXF.
**	It will be built at session startup (SXC_BGN_SESSION), and possibly be 
**	destroyed on ending a session (SXC_END_SESSION). SXF_DBCBs are
**	linked together in a list controlled by the SXF server control 
**	block (SXF_SVCB), the addition or removal of a member from the list
**	will require semaphore protection.
**
** History:
**	10-may-1993 (robf)
**	   Initial creation.
*/
/*
** Name: SXF_DBCB - a database control block used in SXF
**
** Description:
**	This structure defines a database control block for use in SXF.
**	One of these structures will exist for each active database in SXF.
**	It will be built at session startup (SXC_BGN_SESSION), and possibly be 
**	destroyed on ending a session (SXC_END_SESSION). SXF_DBCBs are
**	linked together in a list controlled by the SXF server control 
**	block (SXF_SVCB), the addition or removal of a member from the list
**	will require semaphore protection.
**
** History:
**	10-may-1993 (robf)
**	   Initial creation.
*/
typedef  struct _SXF_DBCB
{
    struct _SXF_DBCB	*sxf_next;	/* Forward pointer in cb list */
    struct _SXF_DBCB 	*sxf_prev;	/* Backward pointer in cb list */
    SIZE_TYPE		sxf_length;	/* Length of this structure */
    i2			sxf_cb_type;	/* Type of this structure */
# define	SXFDB_CB		110
    i2                  sxf_s_reserved;
    PTR                 sxf_l_reserved;
    PTR                 sxf_owner;      /* Structure owner */
    i4                  sxf_ascii_id;   /* So as to see in a dump. */
# define	SXFDBCB_ASCII_ID	CV_C_CONST_MACRO('S', 'X', 'D', 'B')

    i4			sxf_usage_cnt;	/* Number of sessions using the 
					** database
					*/		
    DB_DB_NAME		sxf_db_name;	/* Name of database currently
					** being accessed.
					*/

    /* cid members are for Cache Ids */
    PTR		        sxf_cid_label;  /* Cache ID for labels */

    PTR			sxf_cid_cmp;	/* Cache ID for comparisons */

    /* Lock value for the cache */
    LK_VALUE		sxf_clock_value; /* Lock value */

} SXF_DBCB;
/*
** Name: SXF_SCB - a session control block used in SXF
**
** Description:
**	This structure defines a session control block for use in SXF.
**	One of these structures will exist for each active session in SXF.
**	It will be built at session startup (SXC_BGN_SESSION), and will be 
**	destroyed on ending the session (SXC_END_SESSION). SXF_SCBs are
**	linked together in a list controlled by the SXF server control 
**	block (SXF_SVCB), the addition or removal of a member from the list
**	will require semaphore protection.
**
** History:
**	9-july-1992 (markg)
**	   Initial creation.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	2-apr-94 (robf)
**          Renamed sxf_last_rec sxf_sxap_scb since thats what it really is.
*/
typedef  struct _SXF_SCB
{
    struct _SXF_SCB	*sxf_next;	/* Forward pointer in cb list */
    struct _SXF_SCB 	*sxf_prev;	/* Backward pointer in cb list */
    SIZE_TYPE		sxf_length;	/* Length of this structure */
    i2			sxf_cb_type;	/* Type of this structure */
# define	SXFSCB_CB		106
    i2			sxf_s_reserved;
    PTR			sxf_l_reserved;
    PTR			sxf_owner;	/* Structure owner */
    i4			sxf_ascii_id;	/* So as to see in a dump. */
# define	SXFSCB_ASCII_ID		CV_C_CONST_MACRO('S', 'X', 'S', 'B')
    DB_OWN_NAME		sxf_user;	/* Effective name of the user
					** this control block refers to
					*/
    DB_OWN_NAME		sxf_ruser;	/* Real name of the user this
					** control block refers to.
					*/
    i4		sxf_ustat;	/* Status (privilege) mask of
					** this user.
					*/
    LK_LLID		sxf_lock_list;	/* Identifier of lock list used
					** by this session.
					*/
    PTR			sxf_scb_mem;	/* Memory stream used by this
					** session.
					*/
    PTR			sxf_sxap_scb;	/* SXAP SCB pointer.
					*/
    SXF_DBCB	        *sxf_db_info;	/* Database info */

    i4		sxf_flags_mask; /* Session flags mask */
# define SXF_SESS_ADD_LABEL	0x01	/* Adding a label id */
# define SXF_SESS_CACHE_INIT    0x02

#define			SXS_TBIT_COUNT	    99
#define                 SXS_TVALUE_COUNT    1
    ULT_VECTOR_MACRO(SXS_TBIT_COUNT, SXS_TVALUE_COUNT)
		    sxf_trace;		/* Trace bits */
#define			SXS_TDUMP_STATS	     2	/* Dump current stats */
#define			SXS_TDUMP_SESSION    3  /* Dump session info */
#define                 SXS_TCVTID	     4  /* COnvert id->label */
#define                 SXS_TDUMP_SHM	     5  /* Dump shared memory info */
#define                 SXS_TMAC	    10	/* Trace MAC arbitration */
#define			SXS_TLABEL	    20	/* Trace label operations */
#define			SXS_TLABCACHE	    30  /* Trace label cache ops */
#define			SXS_TLABID	    40  /* Trace label id ops */
#define			SXS_TLABID_DEL	    41  /* Delete a single label id*/
#define			SXS_TDEFLABEL	    50  /* Trace default label ops*/
#define			SXS_TPURGE	    60  /* Purge db cache  */
#define			SXS_TAUD_LOG	    70	/* Audit logical laver*/
#define			SXS_TAUD_PHYS	    80	/* Audit physical layer */
#define			SXS_TAUD_SHM	    85  /* Audit SHM operations */
#define			SXS_TAUD_PLOCK	    86	/* Audit state physical locks */
#define			SXS_TAUD_STATE	    90	/* Audit state tracing */
#define			SXS_THELP	    99  /* Help */

   char			sxf_sessid[SXF_SESSID_LEN]; /* Session id */
}  SXF_SCB;

/*
** Macro to check for session tracing
*/
#define	SXS_SESS_TRACE(flag)   sxf_chk_sess_trace(flag)

/*{
** Name: SXF_RSCB - audit record stream control block
**
** Description:
**	This typedef describes an audit record stream control block. The
**	RSCB is used to identify an instance of an open record stream to the low
**	level security auditing system. This structure is returned to a caller
**	when a SXA_OPEN call is made, all subsequent SXR_READ and SXA_CLOSE 
**	calls must give this identifier.
**	
**	At facility startup time (SXC_STARTUP), a RSCB is built to be used
**	by all sessions for write access to the current audit file. This
**	means that for SXR_WRITE operations the caller should not specify
**	a RSCB identifier.
**
**	RSCBs are arranged in a list controlled by the server control 
**	block (SXF_SVCB). Adding or removing elements of this list
**	require semaphore protection.
**
** History:
**	9-july-1992 (markg)
**	    Initial creation.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef  struct  _SXF_RSCB
{
    struct _SXF_RSCB	*sxf_next;	/* Forward pointer in cb list */
    struct _SXF_RSCB 	*sxf_prev;	/* Backward pointer in cb list */
    SIZE_TYPE		sxf_length;	/* Length of this structure */
    i2			sxf_cb_type;	/* Type of this structure */
# define	SXFRSCB_CB		107
    i2			sxf_s_reserved;
    PTR			sxf_l_reserved;
    PTR			sxf_owner;	/* Structure owner */
    i4			sxf_ascii_id;	/* So as to see in a dump. */
# define	SXRSCB_ASCII_ID 	CV_C_CONST_MACRO('S', 'X', 'R', 'S')
    i4			sxf_rscb_status;
# define	SXRSCB_READ		0x01
# define	SXRSCB_WRITE		0x02
# define	SXRSCB_POSITIONED	0x04
# define	SXRSCB_OPENED		0x08
    SXF_SCB		*sxf_scb_ptr;	/* Pointer to SXF_SCB of session
					** that owns this structure. This
					** may be NULL for the RSCB used
					** by all sessiond for the SXR_WRITE
					** operaion;
					*/
    PTR			sxf_rscb_mem;	/* Memory stream used to allocate
					** storage for this RSCB.
					*/
    PTR			sxf_sxap;	/* Pointer to the SXAP portion of the
					** RSCB.
					*/
}  SXF_RSCB;

/*
** Name: SXF_ASCB - the SXF audit state control block
**
** Description:
**	This typef describes the audit state control block used in SXF. It
**	consists of an array of bytes, one for each audit event type, which
**	shows if the event is enabled or disable. The data held in this
**	control block represents a cached version of the data in the 
**	underlying iidbdb system catalog iisecuritystate. This cached data is
**	protected by a cache locking mechanism such that any changes made to 
**	the cache in one server will cause all other servers in the
**	installation to re-read the data from the iisecuritystate catalog.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
**	7-jul-93 (robf)
**	    Added list of security labels for security level auditing
**	08-sep-93 (swm)
**	    Changed sxf_aud_thread type from i4  to CS_SID to match recent CL
**	    interface revision.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef  struct  _SXF_ASCB
{
    struct _SXF_ASCB	*sxf_next;	/* Not used */
    struct _SXF_ASCB 	*sxf_prev;	/* Not used */
    SIZE_TYPE		sxf_length;	/* Length of this structure */
    i2			sxf_cb_type;	/* Type of this structure */
# define	SXFAS_CB		109
    i2			sxf_s_reserved;
    PTR			sxf_l_reserved;
    PTR			sxf_owner;	/* Structure owner */
    i4			sxf_ascii_id;	/* So as to see in a dump. */
# define	SXFASCB_ASCII_ID	CV_C_CONST_MACRO('S', 'X', 'A', 'S')
    LK_VALUE		sxf_lockval;	/* Value of the lock associated with
					** the audit state control block. Used
					** to invalidate the cached data.
					*/
    char		sxf_event[SXF_E_MAX + 1];
					/* Array of bytes, one for each audit
					** event type. Used to show if 
					** auditing is enabled or disabled 
					** for a particular event.
					*/
    CS_SID		sxf_aud_thread;	/* The session id of the SXF
					** auditing thread.
					*/
   CS_SEMAPHORE		sxf_aud_sem;	/* Semaphore used to serailize
					** access to the ASCB.
					*/
   i4		sxf_aud_nlabel; /* Number of labels in aud_labels*/
   i4		sxf_aud_maxlabel;/* Maximum allowed labels */
# define SXF_AUD_MIN_LAB_ALLOC 30	/* Allocate at least this many  */
} SXF_ASCB;

/*
** Name: SXF_STATS - the SXF server statistics block
**
** Description: This block is used to hold "useful" statistical information
**              about SXF
**
** History:
**	14-dec-1992 (robf)
**		Initial Creation.
**	20-may-1993 (robf)
**	        Added label/id cache stats.
**	30-mar-94 (robf)
**              Added shared buffer stats.
*/
typedef struct _SXF_STATS {
	i4 write_buff;	/* Buffered writes */
	i4 write_queue;	/* Queued writes */
	i4 write_direct;   /* Direct writes */
	i4 write_full;	/* Direct writes, full */
	i4 read_buff;	/* Buffered reads */
	i4 read_direct; 	/* Direct reads */
	i4 open_read; 	/* Open for reading */
	i4 open_write;	/* Open for writing */
	i4 create_count;	/* Create new file */
	i4 close_count;	/* Closes (all) */
	i4 flush_queue;	/* Queue Flushes */
	i4 flush_qempty;	/* Queue Empty Flushes */
	i4 flush_qall;	/* Queue All Flushes */
	i4 flush_qfull;    /* Queue Full flushes */
	i4 flush_count;	/* Flushes */
	i4 flush_empty;	/* Flushes with no work to do */
	i4 flush_predone;	/* Flushes already done */
	i4 logswitch_count;/* Number of log switches */
	i4 extend_count;	/* Number of file extends */
	i4 audit_wakeup;   /* Audit thread wakeups */
	i4 audit_writer_wakeup;   /* Audit writer thread wakeups */
	i4 label_compare;  /* Number of security label comparsions */
	i4 lid_lookup;	/* Number of label id lookups in cache */
	i4 lid_look_fail;	/* Number of lid lookups which failed */
	i4 label_lookup;	/* Number of label lookups in cache */
	i4 label_look_fail;/* Number of label lookups which failed */
	i4 label_cache_add;/* Number of labels added to the cache */
	i4 lid_db_get;	/* Number of lid requests from database */
	i4 label_db_get;	/* Number of labels requested from database */
	i4 lid_db_get_fail;/* Number of lids get fails from database */
	i4 label_db_get_fail;/* Number of labels get fails from database */
	i4	db_build;	/* Database CB initial builds */
	i4	db_purge;	/* Number of database cache purges */
	i4	db_purge_all;	/* Number of full database cache purges */
	i4	label_db_add;	/* Label adds into database */
	i4 mac_access;	/* Number of MAC access requests */
	i4 mac_access_deny;/* Number of denied MAC access requests */
	i4 mac_write;	/* Number of MAC write requests */
	i4 mac_write_deny; /* Number of denied MAC write requests */
	SIZE_TYPE	mem_at_startup; /* Memory allocated at startup */
	SIZE_TYPE	mem_low_avail;  /* Lowest available memeory, approx */
	i4	cmp_lookup;	/* Compare cache lookup */
	i4	cmp_look_fail;	/* Compare cache failed lookups */
	i4	cmp_cache_add;	/* Compare cache adds */
} SXF_STATS;

/*{
** Name: SXF_SVCB - the SXF server control block
**
** Description:
**	This typedef describes the server control block used by SXF. Only
**	one of these structures will exist in the lifetime of the server.
**	It is used to show the state of the SXF facility and additionally
**	is used to manage access to other SXF data structures. It is built
**	and initializes at facility startup time (SXC_STARTUP).
**
** History:
**	9-July-1992 (markg)
**	    Initial creation.
**	7-may-1993 (robf)
**	   Add fields to hold MAC configuration info.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	10-nov-93 (robf)
**         Changed ingres_high/low to be just labels since they span
**	   databases so should never have an id associated with them.
**	22-apr-94 (robf)
**         Add write_fixed, update_float handling
**	10-may-94 (robf)
**	   Add SXF_UF_POLY, update_float should polyinstantiate
*/
typedef  struct  _SXF_SVCB
{
    struct _SXF_SVCB	*sxf_next;	/* Not used */
    struct _SXF_SVCB 	*sxf_prev;	/* Not used */
    SIZE_TYPE		sxf_length;	/* Length of this structure */
    i2			sxf_cb_type;	/* Type of this structure */
# define	SXFSVCB_CB		106
    i2			sxf_s_reserved;
    PTR			sxf_l_reserved;
    PTR			sxf_owner;	/* Structure owner */
    i4			sxf_ascii_id;	/* So as to see in a dump. */
# define	SXFSVCB_ASCII_ID	CV_C_CONST_MACRO('S', 'X', 'S', 'V')
    i4             sxf_svcb_status;
# define        SXF_ACTIVE              0x0001L
# define        SXF_SNGL_USER           0x0002L
# define        SXF_CHECK               0x0004L
# define        SXF_C2_SECURE           0x0008L
# define        SXF_B1_SECURE           0x0010L
# define        SXF_STARTUP             0x0020L
# define        SXF_AUDIT_DISABLED      0x0040L
# define	SXF_NO_IIDBDB		0x0080L
# define 	SXF_AUD_THREAD_UP       0x0100L
    PTR                 sxf_pool_id;    /* Facility memory pool used for
                                        ** allocating all memory streams
                                        ** in the facility
                                        */
    SIZE_TYPE           sxf_pool_sz;    /* Amount of free memory remaining
                                        ** in the SXF memory pool (in bytes).
                                        */
    PTR                 sxf_svcb_mem;   /* Memory stream for server conrol
                                        ** block. Used for allocating memory
                                        ** which will exist for the life of
                                        ** server.
                                        */
    CS_SEMAPHORE	sxf_svcb_sem;	/* Samaphore used to protect access to
					** list structures in the facility
					*/
    SXF_SCB		*sxf_scb_list;	/* Pointer to list of SXF_SCBs */
    SXF_RSCB		*sxf_rscb_list;	/* Pointer to list of SXF_RSCBs */
    i4			sxf_active_ses;	/* Number of active sessions in the
					** facility.
					*/
    i4			sxf_max_ses;	/* Maximum allowable number of 
					** active sessions in the facility
					*/
    SXF_ASCB		*sxf_aud_state;	/* Pointer to the audit state control
					** block.
					*/
    SXF_RSCB		*sxf_write_rscb;/* Pointer to the RSCB used for all
					** SXR_WRITE requests.
					*/
    PTR			sxf_sxap_cb;	/* A pointer to the low level audit
					** control block.
					*/
    LK_LLID		sxf_lock_list;	/* Identifier of lock list used
					** to hold locks that have a duration
					** of the facility life time.
					*/
    DB_STATUS		(*sxf_dmf_cptr)();
					/* A pointer to the dmf_call routine.
					** All calls that SXF makes to DMF
					** must use this pointer. This is
					** to work around a VMS shareable
					** image linking problem.
					*/
    i4			sxf_act_on_err;	/* The action to take when an
					** error occurs in the auditing 
					** system.
					*/
# define	SXF_ERR_SHUTDOWN	1
# define	SXF_ERR_STOPAUDIT	2
    i4		        sxf_delete_down; /* Action to take when a delete down
					 ** MAC request is made
					 */
# define	SXF_DD_ANY		1 /* Always allowed */
# define	SXF_DD_WRITEDOWN   	2 /* Only with writedown */
# define	SXF_DD_NEVER      	3 /* Never allowed */

    i4			sxf_write_fixed;  /* Action to take on write_fixed */
# define	SXF_WF_FLOAT		1 /* Float */
# define	SXF_WF_NOFLOAT	        2 /* Nofloat */
    i4			sxf_update_float;  /* Action to take on update float */
# define	SXF_UF_RECREATE		1 /* Recreate (delete/insert) */
# define	SXF_UF_REPLACE	        2 /* Replace in line */
# define	SXF_UF_POLY		3 /* Polyinstantiate row */
   SXF_STATS		*sxf_stats;	/* SXF statistics block */
   CS_SEMAPHORE		sxf_db_sem;	/* Samaphore used to protect access to
					** db cb structures in the facility
					*/
   SXF_DBCB		*sxf_db_info;	/* SXF Database access info */
   i4		sxf_label_cache_num; /* Label cache size */
   PID			sxf_pid;	     /* Server PID */
}  SXF_SVCB;

/*
** Global reference to the SXF_SVCB used internally by all SXF modules
*/
GLOBALREF	SXF_SVCB	*Sxf_svcb;

/*{
** Name: SXAS_DMF - DMF control information.
**
** Description:
**	This structure is used to hold DMF control information for
**	manipulating tuples in database tables. This structure is only 
**	used to pass data in function calls in SXAS, the SXF audit state
**	management subsystem.
**
** History:
**	26-nov-1992 (markg)
**	    Initial creation.
*/
typedef struct
{
    i4			dm_status;
# define	SX_POSITIONED		1
    PTR			dm_db_id;
    PTR			dm_tran_id;
    PTR			dm_access_id;
    i4			dm_mode;
    char		*dm_tuple;
    i4		dm_tup_size;
} SXAS_DMF;

/*
** Function prototypes of SXF internal routines.
*/
FUNC_EXTERN DB_STATUS sxc_startup(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxc_shutdown(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxc_bgn_session(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxc_end_session(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxc_alter_session(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxac_alter(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxc_build_scb(
    SXF_RCB	*rcb,
    SXF_SCB	**scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxc_destroy_scb(
    SXF_SCB	*scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxac_startup(
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxac_shutdown(
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxac_bgn_session(
    SXF_SCB	*scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxac_end_session(
    SXF_SCB	*scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxac_audit_thread(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxac_audit_writer_thread(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxac_audit_error(void);

FUNC_EXTERN DB_STATUS sxaf_open(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxaf_close(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxar_read(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxar_position(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxar_write(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxar_flush(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxau_get_scb(
    SXF_SCB	**scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxau_build_rscb(
    SXF_RSCB	**rscb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxau_destroy_rscb(
    SXF_RSCB	*rscb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxt_ubegin(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxt_uend(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxas_check(
    SXF_SCB	*scb,
    SXF_EVENT	event_type,
    bool	*result,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxas_cache_init(
    SXF_SCB	*scb,
    i4	*err_code);

FUNC_EXTERN DB_STATUS sxas_show(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxas_alter(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxas_startup(
    i4	*err_code);

FUNC_EXTERN VOID sxf_set_activity(char*);

FUNC_EXTERN VOID sxf_clr_activity(void);

FUNC_EXTERN VOID sxf_incr (i4 *);

FUNC_EXTERN VOID sxc_printstats (void);

FUNC_EXTERN DB_STATUS sxm_access(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxm_write(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxm_deflabel(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxl_find(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxl_delete(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxl_collate(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxl_compare(
    SXF_RCB	*rcb);

FUNC_EXTERN DB_STATUS sxm_arbitrate(
    SXF_RCB     *rcb,
    i4	*err_code);

FUNC_EXTERN SXF_DBCB *sxc_getdbcb (
		SXF_SCB *,
		DB_DB_NAME *, 
		bool,
		i4 *);

FUNC_EXTERN VOID sxc_freedbcb ( 
	SXF_SCB*,
	SXF_DBCB *);

FUNC_EXTERN DB_STATUS sxm_encapsulate (
	SXF_SCB *,
	i4 * );

FUNC_EXTERN DB_STATUS 
sxm_write_audit(
	SXF_RCB	*rcb, 
	SXF_ACCESS access,
	i4 msgid, 
	bool 	result,
	i4 *err_code);

FUNC_EXTERN bool 
sxf_chk_sess_trace(i4 flag);

FUNC_EXTERN VOID 
sxf_scc_trace( char *msg_buffer);

FUNC_EXTERN  DB_STATUS 
sxas_suspend_audit(bool, i4 *);

FUNC_EXTERN  DB_STATUS 
sxas_stop_audit(bool, i4 *);

FUNC_EXTERN  DB_STATUS 
sxas_poll_resume( i4 *);

FUNC_EXTERN DB_STATUS
sxc_mo_attach(void);

FUNC_EXTERN VOID
sxap_shm_dump(void);
/*
** DISPLAY macros (to avoid a variable length arg list)
** Extra casts because many args are i4, shuts up compilers.
** Now that varags is standardized, sxf-display should be varargs.
*/
FUNC_EXTERN VOID
sxf_display(char *format, PTR, PTR, PTR, PTR, PTR);

#define SXF_DISPLAY(format) sxf_display(format, \
		(PTR)0, (PTR)0, (PTR)0, (PTR)0, (PTR)0)

#define SXF_DISPLAY1(format, p1) sxf_display(format, \
		(PTR)(SCALARP)p1, (PTR)0, (PTR)0, (PTR)0, (PTR)0)

#define SXF_DISPLAY2(format, p1, p2) sxf_display(format, \
		(PTR)(SCALARP)p1, (PTR)(SCALARP)p2, (PTR)0, (PTR)0, (PTR)0)

#define SXF_DISPLAY3(format, p1, p2,p3) sxf_display(format, \
		(PTR)(SCALARP)p1, (PTR)(SCALARP)p2, (PTR)(SCALARP)p3, (PTR)0, (PTR)0)

#define SXF_DISPLAY4(format, p1, p2, p3, p4) sxf_display(format, \
		(PTR)(SCALARP)p1, (PTR)(SCALARP)p2, (PTR)(SCALARP)p3, (PTR)(SCALARP)p4, (PTR)0)

#define SXF_DISPLAY5(format, p1, p2, p3, p4, p5) sxf_display(format, \
		(PTR)(SCALARP)p1, (PTR)(SCALARP)p2, (PTR)(SCALARP)p3, (PTR)(SCALARP)p4, (PTR)(SCALARP)p5)

	

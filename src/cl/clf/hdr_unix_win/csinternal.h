/*
** Copyright (c) 1987, 2004 Ingres Corporation
*/

/**
** Name: CSINTERNAL.H - Internal Control System Module definitions
**
** Description:
**      This file contains the definitions of datatypes and constants
**      used within the CS module.
**
** History: $Log-for RCS$
**      29-Oct-1986 (fred)
**          Created.
**	26-sep-1988 (rogerk)
**	    Added thread priority constants.
**	31-oct-1988 (rogerk)
**	    Added CS_ACCNTING_MASK to server block cs_mask.  This specifies
**	    whether or not to write records to the accounting file for each
**	    dbms session.
**	    Added CS_CPUSTAT_MASK to server block cs_mask.  This specifies
**	    that the server should keep track of cpu times on a thread by
**	    thread basis.
**	 7-nov-1988 (rogerk)
**	    Added fields in the CS_SYSTEM control block in which asynchronous
**	    routines can write values without colliding with other threads.
**      28-feb-1989 (rogerk)
**          Added cs_pid field and stats for cross process semaphores to server
**          control block.
**      19-jul-89 (rogerk)
**          Added cs_event_mask to Cs_srv_block for clients of event routines.
**      16-oct-89 (fls-sequent)
**          Modified for MCT.
**	31-Oct-89 (anton)
**	    Change type of quantum in startup message.
**	22-Jan-90 (fredp)
**	    Integrated the following change:  27-jun-89 (daveb)
**	    Added CS_RESERVED_FDS -- a "best guess" at the max. number of 
**	    file descriptors that the server will use internally.
**	23-Jan-90 (anton)
**	    Increased size of cs_crb_buf due to size of GCA_LS_PARMS
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Remove superfluous "typedef"s before structure declarations;
**		Size sc_sname according to GCC_L_PORT and include <gcccl.h>
**		to pick up the GCC_L_PORT length value.
**	21-mar-90 (fls-sequent)
**	    Added CS_GET_MY_SCB macro for MCT.
**	22-oct-90 (fls-sequent)
**	    Integrated changes from 63p CL:
**	        Increased size of cs_crb_buf due to size of GCA_LS_PARMS
**		Remove superfluous "typedef"s before structure declarations;
**		Size sc_sname according to GCC_L_PORT and include <gcccl.h>
**		to pick up the GCC_L_PORT length value.
**	4-jul-92 (daveb)
**		Add SPTREEs to CS_SYSTEM for scb, sem, and conditions.
**	26-jul-1993 (bryanp/keving)
**	    Changed number of options in CS_STARTUP_MSG to CS_OPTMAX.
**	1-Sep-1993 (seiwald)
**	    CS_OPTIONS and CS_STARTUP_MSG gone, as startup parameters no
**	    longer traverse the CL interface.  Added a few flags to the
**	    bottom of CS_SYSTEM to hold parameters once held in the
**	    startup message.  Commented as "defunct" cs_multiple in
**	    CS_SYSTEM, but didn't delete it: every csll.*.s depends on
**	    hardwired offsets into CS_SYSTEM!
**	20-sep-93 (mikem)
**	    Added 3 fields to the CS_SYSTEM segment to keep track of "nap"
**	    calls (nap's are cases when the server sleep's because another
**	    process owns the cross process semaphore it needs).  Also changed
**	    all the statistics counters from u_i4 to u_i4 as these
**	    counters can get very large (especially semaphore spin counts).
**	02-nov-93 (swm)
**	    Bug #56447
**	    Changed type of cs_used from i4  to CS_SID because it can be
**	    used to hold CS_SCB.cs_self.
**      31-jan-94 (mikem)
**          sir #57671
**	    Added 2 fields to CS_SYSTEM keep track of the unix specific 
**	    parameters to control transfer size of non-shared memory I/O 
**	    executed by the unix di slaves: cs_size_io_buf, cs_num_io_buf.
**	20-jul-95 (shero03)
**	    Masked off ADMIN_SCB for NT
**	14-Jun-1995 (jenjo02)
**	    Renamed cs_multiple (obsolete) to cs_hwm_active in which
**	    hwm of #threads on the run-queues is kept.
**      04-oct-1995 (wonst02)
**          Add cs_facility to CS_SYSTEM for the address of scs_facility().
** 	16-Jan-1996 (jenjo02)
**	    Changed cs_sm_* to cs_tm_*. cs_sm_* wasn't being used, so
**	    cs_tm_* fields will be used to account for time-only  
**	    CSsuspend() wait reasons.
**	    Added cs_lge_*, cs_lke_* fields in which to track CSsuspend()
**	    calls on LK/LG events.
**	14-Mch-1996 (prida01)
**	    Add cs_diag to _CS_SYSTEM
**	03-jun-1996 (canor01)
**	    Modified for operating system threads.
**	    Add cs_facility to CS_SYSTEM for Unix as well as NT.
**	    Add synchronization for global splay trees.
**	13-Aug-1996 (jenjo02)
**	    Modified cs_smstatistics in CS_SYSTEM to provide better-fit
**	    statistics for thread semaphores.
**	18-nov-1996 (canor01)
**	    Synchronization primitives in CS_SYSTEM for the splay trees are for
**	    OS-thread systems only.
**	10-dec-96 (hanch04)
**	    moved cs_facility, offset is hardcoded in cs.
**	5-dec-96 (stephenb)
**	    Add definition of CS_PROFILE to CS_SYSTEM for profiling.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          CS_SYSTEM will contain both versions for SMSTAT
**	28-feb-1997 (hanch04)
**	    cs_mt will be part of CS_SYSTEM for threads or no threads
**	21-may-1997 (shero03)
**	    Add definition of CS_DEV_TRCENTRY for debugging
**	03-Jul-97 (radve01)
**	    Several fields added to the end of the server block in order to 
**	    facilitate debugging.
**	07-jul-1997 (canor01)
**	    Add lists of known semaphores.
**      21-jul-1997 (canor01)
**          If we are trying to reserve low file descriptors for stream i/o,
**          increase CS_RESERVED_FDS by 256 to reserve them outside all
**          other uses.
**	26-Aug-1997 (kosma01)
**	    For OS_THREADS_USED, cast special session ids into threads 
**	    friendly types. Also add a NULL session id CS_NULL_ID.
**	03-nov-1997 (canor01)
**	    Add support for using CS_alloc_stack to maintain stacks for
**	    OS threads.
**	01-Dec-1997 (jenjo02)
**	    Added cs_rcp_pid, cs_get_rcp_pid  to CS_SYSTEM to provide the 
**	    identity of the RCP to CSMTp_semaphore() (OS threads only).
**      16-Feb-98 (fanra01)
**          Add attach and detach functions from IMA so that the structures
**          can be registered when the thread is active.
**	23-feb-1998 (canor01)
**	    Moved the fields from the last two changes to after the 
**	    cs_inkernel entry to prevent having to change hard-coded 
**	    offsets in assembler.
**	25-Feb-1998 (jenjo02)
**	    Added cs_cnt_mutex to Orlan's above list.
**	09-Mar-1998 (jenjo02)
**	    Moved CS_MAXPRIORITY, CS_MINPRIORITY, CS_DEFPRIORITY to
**	    csnormal.h to expose them to SCF.
**	24-Sep-1998 (jenjo02)
**	    Eliminate cs_cnt_mutex, hold Cs_known_list_sem while adjusting
**	    session counts instead.
**	16-Nov-1998 (jenjo02)
**	    Added new wait counts/times to CS_SYSTEM for Reads/Writes,
**	    Log I/O. Existing cs_bio/cs_dio stats retained for 
**	    compatibility.
**  02-Dec-1998 (muhpa01)
**	    Moved CS_NULL_ID so it's now defined for all (ie. non-OS_THREADS).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio_time, cs_bio_waits, cs_bio_done, ditto
**	    for cs_dio?. These objects are now computed with (new)
**	    MO methods. Added cs_dio?_kbytes, cs_lio?_kbytes stats
**	    to help track I/O rates in the sampler.
**	05-sep-2002 (devjo01)
**	    Add 'cs_format_lkkey' to CS_SYSTEM.
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads. Defined CS_ALTSTK_SZ
**	12-apr-2004 (somsa010
**	    Updated cs_size to be a SIZE_TYPE in CS_SCB, to correspond
**	    to the changes made to SC0M_OBJECT.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _CS_SYSTEM   CS_SYSTEM;
typedef struct _CS_STK_CB   CS_STK_CB;

/*
**  Defines of other constants.
*/

/*
**      The Base set of special session identifiers
*/
# if defined(OS_THREADS_USED)
#define                 CS_SCB_HDR_ID   (CS_SID)0x0001
#define                 CS_IDLE_ID      (CS_SID)0x0002
#define                 CS_MNTR_ID      (CS_SID)0x0012

#define                 CS_FIRST_ID     (CS_SID)0x0100	    /* first free id */
# else
#define                 CS_SCB_HDR_ID   0x0001
#define                 CS_IDLE_ID      0x0002
#define                 CS_MNTR_ID      0x0012

#define                 CS_FIRST_ID     0x0100	    /* first free id */
# endif   /* OS_THREADS_USED */
#define                 CS_NULL_ID      (CS_SID)0x0         /* null id, not a session anymore */

/*
** Define the maximum number of file descriptors the server will use
** internally.  The value of (CL_NFILE() - CS_RESERVED_FDS) is an
** approximation of the number of file descriptors that will be
** left for other purposes.  With the current implementation of GCA,
** this is the limit on the number of user connections that can be made.
**
** The value here is a "best guess" -- it changes depending on the amount
** of logging and tracing going on.
**
** If we are trying to reserve low file descriptors (0-255) for stream i/o,
** don't count them in the ones we need.
*/

# ifdef xCL_RESERVE_STREAM_FDS
#define                 CS_RESERVED_FDS	(14+256)
# else /* xCL_RESERVE_STREAM_FDS */
#define                 CS_RESERVED_FDS	14
# endif /* xCL_RESERVE_STREAM_FDS */


/*}
** Name: CS_SYSTEM - Overall control structure for the Control System
**
** Description:
**      This structure describes the overall control structure which is used 
**      throughout the CS module, and, therefore, is the central structure 
**      of the entire system in which the CS module is running.  This structure 
**      supplies the anchor points for the ready and wait queues in use in the 
**      system, the anchor for the list of known sessions, the anchor for the
**      connection request block list, and the location for numerous bits of 
**      interesting information about how things are running, etc.
**
**	This structure is referenced and possibly altered by any thread in
**	the system.  Any thread that alters information in the CS_SYSTEM
**	control block must take care that it is protected from changing
**	it at that same time as another thread.  Also, some CS routines
**	are triggered by asynchronous events and these also may alter fields
**	in the CS_SYSTEM control block.  There are three methods used by
**	the CS system to protect itself while changing CS_SYSTEM information.
**
**	    1) Turn off ASTs.  Since CS uses AST's to drive asynchronous
**	       events, no unexpected events can occur while we have AST's
**	       disabled.  This includes quantum based thread switching.  While
**	       ASTs are disabled, the current thread will not be switched out
**	       by the quantum timer - it can only be switched out if it
**	       asks to be.  A session may safely alter CS_SYSTEM control block
**	       fields while ASTs are disabled, but should always take care
**	       enable ASTs when done.
**
**	    2) Turn on cs_inkernel flag. This flag in the CS_SYSTEM control
**	       block specifies that the current session should not be switched
**	       out involuntarily.  Similar to turning off ASTs, when this
**	       flag is on the quantum timer will not switch the current thread
**	       out - the thread can only be switched out on request.  Note,
**	       however, that turning on the cs_inkernel flag does not prevent
**	       asynchronous CS routines from being called.
**
**	       A CS routine that is called directly by a thread (not by way
**	       of an AST) may alter most CS_SYSTEM control block information
**	       safely while the cs_inkernel flag is turned on.  The only
**	       fields that cannot be safely altered are the async field
**	       described below in method (3).  The CS routine should be
**	       careful to turn off the cs_inkernel flag when done, and to
**	       follow the protocol described in below in (3) regarding the
**	       async fields.
**
**	       Some CS routines run in the context of an AST.  These routines
**	       are CSresume, CSattn, and CSquantum.  These routines are
**	       protected from AST's, but since thay may be signalled while
**	       another thread was in the middle of altering CS_SYSTEM
**	       information, they may not change any fields that they want to.
**	       If an AST driven routine sees that the cs_inkernel field is not
**	       on, then it may safely alter any field in the CS control block.
**	       If an AST routine sees that the cs_inkernel field is ON, then
**	       it may alter NO FIELDS except for the async ones described below.
**
**	    3) Use of cs_async fields.  In the CS_SYSTEM control block are
**	       fields which can be used only by atomic routines which are
**	       protected from ASTs.  These fields are:  cs_async, cs_as_list,
**	       cs_scb_async_cnt, and cs_asadd_thread.  When an AST driven CS
**	       routine is called while the cs_inkernel flag is set, that routine
**	       may write only into these special CS_SYSTEM fields.  It must then
**	       make sure that the 'cs_async' field is ON.
**
**	       When a non-AST driven CS routine turns off the cs_inkernel flag,
**	       it must check the cs_async field of the CS_SYSTEM control block.
**	       If that field is set, then the CS routine must turn off AST's
**	       and move all information from the async fields into the regular
**	       portions of the CS_SYSTEM control block.
**
**      NOTE : This structure reference inside CSLL.MAR with hard-wired
**      offsets to some of the fields.  Any changes to the layout of
**      this structure will require changes in the offset definitions in
**      CSLL.MAR as well.
** 
[@comment_line@]...
**
** History:
**      27-Oct-1986 (fred)
**          Initial Definition.
**	 7-nov-1988 (rogerk)
**	    Added fields - cs_async, cs_as_list, cs_scb_async_cnt and
**	    cs_asadd_thread - for asynchronous routines to write information
**	    into.  This is to enable CS to do most operations without having
**	    to enable/disable asts.
**      28-feb-1989 (rogerk)
**          Added cs_pid field and stats for cross process semaphores.
**      19-jul-89 (rogerk)
**          Added cs_event_mask to Cs_srv_block for clients of event routines.
**      16-oct-89 (fls-sequent)
**          Modified for MCT.
**	23-Jan-90 (anton)
**	    Increased size of cs_crb_buf due to size of GCA_LS_PARMS
**	20-sep-93 (mikem)
**	    Added 3 fields to the CS_SYSTEM segment to keep track of "nap"
**	    calls (nap's are cases when the server sleep's because another
**	    process owns the cross process semaphore it needs).  Also changed
**	    all the statistics counters from u_i4 to u_i4 as these
**	    counters can get very large (especially semaphore spin counts).
**      31-jan-94 (mikem)
**          sir #57671
**	    Added 2 fields to CS_SYSTEM keep track of the unix specific 
**	    parameters to control transfer size of non-shared memory I/O 
**	    executed by the unix di slaves: cs_size_io_buf, cs_num_io_buf.
**      25-oct-1995 (thaju02/stoli02)
**          Added wait on CSEVENT counter.
** 	16-Jan-1996 (jenjo02)
**	    Changed cs_sm_* to cs_tm_*. cs_sm_* wasn't being used, so
**	    cs_tm_* fields will be used to account for time-only  
**	    CSsuspend() wait reasons.
**	    Added cs_lge_*, cs_lke_* fields in which to track CSsuspend()
**	    calls on LK/LG events.
**	13-Aug-1996 (jenjo02)
**	    Modified cs_smstatistics in CS_SYSTEM to provide better-fit
**	    statistics for thread semaphores.
**	18-nov-1996 (canor01)
**	    Synchronization primitives for the splay trees are for
**	    OS-thread systems only.
**	03-Jul-97 (radve01)
**	    Several fields added to the end of the server block in order to
**	    facilitate debugging.
**	18-nov-1996 (canor01)
**	    Synchronization primitives for the splay trees are for
**	    OS-thread systems only.
**	01-Dec-1997 (jenjo02)
**	    Added cs_rcp_pid, cs_get_rcp_pid  to CS_SYSTEM to provide the 
**	    identity of the RCP to CSMTp_semaphore() (OS threads only).
**	31-jul-1998 (canor01)
**	    Need to include pc.h for PID definition.
**	24-Sep-1998 (jenjo02)
**	    Eliminate cs_cnt_mutex, hold Cs_known_list_sem while adjusting
**	    session counts instead.
**	16-Nov-1998 (jenjo02)
**	    Added new wait counts/times to CS_SYSTEM for Reads/Writes,
**	    Log I/O. Existing cs_bio/cs_dio stats retained for 
**	    compatibility.
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio_time, cs_bio_waits, cs_bio_done, ditto
**	    for cs_dio?. These objects are now computed with (new)
**	    MO methods. Added cs_dio?_kbytes, cs_lio?_kbytes stats
**	    to help the sampler track I/O rates.
**	13-Feb-2007 (kschendel)
**	    Added wall-clock ticker so that servers have an independently
**	    running clock.  (for OS-threaded only.)
*/

/*
** Need value of GCC_L_PORT from this include file
*/
# include       <gcccl.h>

/*
** Need QUEUE definition
*/
# include	<qu.h>

/*
** Need PID definition
*/
# include	<pc.h>

#ifdef xDEV_TRACE

#define	CS_DEV_TRACE_ENTRIES 5000

typedef struct _CS_DEV_TRCENTRY CS_DEV_TRCENTRY;

    struct      _CS_DEV_TRCENTRY
    {
        CS_SCB		*cs_dev_trc_scb;    	/* The current SCB */
	u_i2		cs_dev_trc_parm;	/* optional parm */
	u_i2		cs_dev_trc_fac;     	/* calling facility */
	int		cs_dev_trc_at;		/* calling from line */
	PTR		cs_dev_trc_opt;		/* optional ptr  */
    };
#endif /*xDEV_TRACE */





struct _CS_SYSTEM
{
    /* 
    ** Offsets to "cs_current" and "cs_inkernel" are hard-coded in
    ** the csll.*.asm modules.
    ** DO NOT MAKE CHANGES IN THIS STRUCTURE from here to 'cs_inkernel'
    ** unless you feel like changing all the asm modules.
    */
    char            cs_header[16];      /* to make easy to find in a dump */
#define                 CS_MARKER       "CS_SYSTEM BLOCK"
    STATUS          (*cs_scballoc)();   /* Routine to get SCB's from */
    STATUS          (*cs_scbdealloc)(); /* Routine thru which to give them back */
    VOID	    (*cs_elog)();       /* Routine with which to log errors */
    STATUS          (*cs_process)();    /* General processing of stuff ... */
    STATUS          (*cs_attn)();	/* Processing of async stuff ... */
    STATUS          (*cs_startup)();    /* To start up the server */
    STATUS          (*cs_shutdown)();   /* To shut the server down */
    STATUS	    (*cs_format)();	/* format scb's */
    STATUS	    (*cs_saddr)();	/* To get session requests */
    STATUS	    (*cs_reject)();	/* To reject session requests */
    VOID	    (*cs_disconnect)();	/* To disconnect sessions */
    STATUS	    (*cs_read)();	/* To read from FE's */
    STATUS	    (*cs_write)();	/* To write to FE's */
    i4              cs_max_sessions;	/* how many sessions are allowed */
    i4              cs_num_sessions;    /* How many are there right now */
    i4              cs_user_sessions;   /* How many user sessions are there */
    i4              cs_max_active;      /* How many threads can be active */
    i4		    cs_hwm_active;	/* High water mark of active threads */
    i4		    cs_cursors;		/* how many active cursors per session */
    i4              cs_num_active;      /* how many threads currently active */
    i4              cs_stksize;         /* how big to make stacks */
    i4              cs_stk_count;       /* How many stacks are allocated */
    i4              cs_ready_mask;      /* Bit mask of priority rdy q */
#define                 CS_PRIORITY_BIT 0x8000
#define			CS_PADMIN	(CS_LIM_PRIORITY - 1)
#define			CS_PIDLE	0
    CS_SCB          *cs_known_list;     /* List of known SCB's */
    CS_SCB	    *cs_wt_list;	/* list of scb's waiting for event */
    CS_SCB	    *cs_to_list;	/* like above but can time out */
    CS_SCB          *cs_rdy_que[CS_LIM_PRIORITY];
					/* The priority ready que */
    CS_SCB          *cs_current;        /* The currently active session */
# if defined(OS_THREADS_USED) && defined(POSIX_THREADS)
    CS_SID          cs_next_id;         /* Next session id available */
# else
    i4              cs_next_id;         /* Next session id available */
# endif 
    STATUS	    cs_error_code;	/* major error in the server */
    i4		    cs_state;		/* state of the system */
#define                 CS_UNINIT       0x00
#define                 CS_INITIALIZING 0x10
#define                 CS_PROCESSING   0x20
#define			CS_SWITCHING	0x25
#define			CS_IDLING	0x30
#define                 CS_CLOSING      0x40
#define                 CS_TERMINATING	0x50
#define			CS_ERROR	0x60
    i4		    cs_mask;		/* special values and states */
#define			CS_NOSLICE_MASK	    0x0001 /* No time slicing */
#define			CS_ADD_MASK	    0x0002
#define			CS_SHUTDOWN_MASK    0x0004 /* Server will shut down
						   ** when all threads exit. */
#define			CS_FINALSHUT_MASK   0x0008 /* Server is shutting down */
#define			CS_ACCNTING_MASK    0x0010 /* Write records for each
						   ** session to accounting 
						   ** file. */
#define			CS_CPUSTAT_MASK     0x0020 /* Collect CPU stats on for
						   ** each session. */
    i4		    cs_inkernel;	/* are we interruptable */
    i4		    cs_async;		/* Set if there was a call to an
					** asynchronous routine while CS was
					** busy (cs_inkernel was set).  This
					** indicates that there is information
					** to be set in the server block when
					** cs_inkernel is reset. */
    char	    cs_aquantum[16];	/* Ascii delta time */
    i4		    cs_aq_length;	/* length of the string above */
    i4		    cs_bquantum[2];	/* VMS binary delta time */
    i4		    cs_toq_cnt;		/* number of scans this time */
    i4		    cs_q_per_sec;	/* number of quantums per second */
    u_i4           cs_quantums;        /* number of quantums gone by */
    u_i4	    cs_idle_time;       /* number of q where task is idle */
    i4	    cs_cpu;		/* cpu time of process so far (10ms) */
    PTR		    cs_svcb;		/* anchor for underlying code */
    CS_STK_CB	    *cs_stk_list;	/* pointer to list of known stacks */
    char	    cs_name[GCC_L_PORT];	/* port name of this server */
    char	    cs_crb_buf[512];	/* buffer for unsolicited CRB's */
    CS_SCB	    *cs_as_list;	/* List of SCB's that became ready
					** while CS was busy (inkernel) and
					** are waiting to be moved to the ready
					** queue. */
    i4		    cs_scb_async_cnt;	/* Number of SCB's with cs_async on */
    i4		    cs_asadd_thread;	/* Received an add-thread request
					** while CS was busy. */
    PID             cs_pid;             /* Process Id */
    i4	    cs_event_mask;	/* Mask of outstanding event waits */
#define			CS_EF_CSEVENT	0x01
    struct  _CS_SMSTAT
    {
        u_i4       cs_smsx_count;  /* collisions on shared vs. exclusive */
        u_i4       cs_smxx_count;  /* exclusive vs. exclusive */
        u_i4       cs_smcx_count;  /* collisions on cross-process */
        u_i4       cs_smx_count;   /* # requests for exclusive */
        u_i4       cs_sms_count;   /* # requests for shared */
        u_i4       cs_smc_count;   /* # requests for cross-process */
        u_i4       cs_smcl_count;  /* # spins waiting for cross-process */
        u_i4       cs_smsp_count;  /* # "single-processor" swusers */
        u_i4       cs_smmp_count;  /* # "multi-processor" swusers */
        u_i4       cs_smnonserver_count;
# ifdef OS_THREADS_USED
        u_i4       cs_smssx_count; /* sp collisions on shared vs. exclusive */
        u_i4       cs_smsxx_count; /* sp exclusive vs. exclusive */
        u_i4       cs_smss_count;  /* sp # requests for shared */
        u_i4       cs_smmsx_count; /* mp collisions on shared vs. exclusive */
        u_i4       cs_smmxx_count; /* mp exclusive vs. exclusive */
        u_i4       cs_smmx_count;  /* mp # requests for exclusive */
        u_i4       cs_smms_count;  /* mp # requests for shared */
# endif /* OS_THREADS_USED */
                                        /* # "non-server" naps */
    }               cs_smstatistics;    /* statistics for semaphores */
    struct  _CS_TMSTAT
    {
	u_i4		cs_bio_idle;	/* bio's while idle */

	u_i4		cs_bior_time;	/* tick count in bio read state */
	u_i4		cs_bior_waits;
	u_i4		cs_bior_done;	/* number done altogether */

	u_i4		cs_biow_time;	/* tick count in bio write state */
	u_i4		cs_biow_waits;
	u_i4		cs_biow_done;	/* number done altogether */

	u_i4		cs_dio_idle;    /* dio's while idle */

	u_i4		cs_dior_time;	/* disk read io */
	u_i4		cs_dior_waits;
	u_i4		cs_dior_done;
	u_i4		cs_dior_kbytes;

	u_i4		cs_diow_time;	/* disk write io */
	u_i4		cs_diow_waits;
	u_i4		cs_diow_done;
	u_i4		cs_diow_kbytes;

	u_i4		cs_lior_time;   /* log read io */
	u_i4		cs_lior_waits;
	u_i4		cs_lior_done;
	u_i4		cs_lior_kbytes;

	u_i4		cs_liow_time;   /* log write io */
	u_i4		cs_liow_waits;
	u_i4		cs_liow_done;
	u_i4		cs_liow_kbytes;

	u_i4		cs_lg_time;
	u_i4		cs_lg_waits;
	u_i4		cs_lg_idle;
	u_i4		cs_lg_done;

	u_i4		cs_lge_time;	/* LGevent waits: */
	u_i4		cs_lge_waits;
	u_i4		cs_lge_done;

	u_i4		cs_lk_time;
	u_i4		cs_lk_waits;
	u_i4		cs_lk_idle;
	u_i4		cs_lk_done;

	u_i4		cs_lke_time;	/* LKevent wait: */
	u_i4		cs_lke_waits;
	u_i4		cs_lke_done;

	u_i4		cs_tm_time;	/* Timer waits:  */
	u_i4		cs_tm_waits;
	u_i4		cs_tm_idle;
	u_i4		cs_tm_done;

        u_i4           cs_event_wait;
        u_i4           cs_event_count;
    }		    cs_wtstatistics;	/* times for various wait states */
#ifdef PERF_TEST
    struct      _CS_PROFILE
    {
        bool            cs_active;      /* profiling currently active for this
                                        ** code */
        i4              cs_calls;       /* no of calls */
        i4              cs_overload;    /* no of overloaded calls (re-called
                                        ** whilst active) */
        f8              cs_start_cpu;   /* start CPU in current active profile*/
        f8              cs_cum_cpu;     /* cumulative CPU used */
    }               cs_profile[CS_MAX_FACILITY + 1];
    f8                cs_cpu_zero;    /* zero point for profiling */
#endif
#ifdef xDEV_TRACE
    CS_DEV_TRCENTRY	*cs_dev_trace_strt;	/* trace area start */
    CS_DEV_TRCENTRY	*cs_dev_trace_end;	/* trace area end */
    CS_DEV_TRCENTRY	*cs_dev_trace_curr;	/* addr(current trace entry) */
#endif /* xDEV_TRACE */
    SPTREE		cs_scb_tree;
    SPTREE		cs_cnd_tree;
    SPTREE		*cs_scb_ptree;
    SPTREE		*cs_cnd_ptree;
# ifdef OS_THREADS_USED
    CS_SYNCH		cs_scb_mutex;
    CS_SYNCH		cs_cnd_mutex;
# endif /* OS_THREADS_USED */
    i4       	cs_quantum;         /* timeslice (units?) */
    i4         	cs_max_ldb_connections;
    i4		cs_size_io_buf;	    /* transfer size for io slave i/o */
    i4		cs_num_io_buf;	    /* number of i/o slave event cb's */
    STATUS	    	(*cs_diag)();	    /* Diagnostics */
    STATUS              (*cs_facility)();   /* return current facility */
    VOID                (*cs_scbattach)();  /* Attach thread control block to MO */
    VOID                (*cs_scbdetach)();  /* Detach thread control block */
    char	       *(*cs_format_lkkey)();/* Fmt LK_LOCK_KEY for display. */
    QUEUE		cs_multi_sem;	    /* list of multi semaphores */
    QUEUE		cs_single_sem;	    /* list of single semaphores */
# ifdef OS_THREADS_USED
    CS_SYNCH		cs_semlist_mutex;   /* protect semaphore lists */
    PID    	    	(*cs_get_rcp_pid)();/* To realize RCP's pid */
    PID             	cs_rcp_pid;         /* Process Id of RCP */
# endif /* OS_THREADS_USED */
    volatile u_i4	cs_ticker_time;	    /* Wall clock timer as maintained
					    ** by the clock-ticker thread (may
					    ** not be used on all platforms).
					    ** In seconds since some epoch.
					    */
    bool	    	cs_stkcache;        /* cache stacks with os-threads */
    bool          	cs_isstar;          /* is this a star server? */
    bool          	cs_define;          /* set II_DBMS_SERVER */
    bool		cs_mt;		    /* use os threads? */
    /*
    ** Useful diagnostic info at the end of the block
    */
    i4     sc_main_sz;   /* Size of SCF Control Block(SC_MAIN_CB,Sc_main_cb) */
    i4     scd_scb_sz;   /* Size of Session Control Block(SCD_SCB) */
    i4     scb_typ_off;  /* Type of the session offset inside session cb. */
    i4     scb_fac_off;  /* The current facility offset inside session cb. */
    i4     scb_usr_off;  /* The user-info off inside session control block */
    i4     scb_qry_off;  /* The query-text offset in session control block */
    i4     scb_act_off;  /* The current activity offset in session cb. */
    i4     scb_ast_off;  /* The alert(event) state offset in session cb. */
volatile PTR srv_blk_end; /* Keep this line et the end of the structure, */
			 /* The variable will be used for debugging purposes */
};

/*}
** Name: CS_STK_CB - Stack region header
**
** Description:
**      This structure is placed in the head of each stack section
**      and contains information about the size, usage, and linkage
**      to other stack sections.
**
** History:
**      10-Nov-1986 (fred)
**          Created.
**	02-nov-93 (swm)
**	    Bug #56447
**	    Changed type of cs_used from i4  to CS_SID because it can be
**	    used to hold CS_SCB.cs_self.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      25-Jun-2003 (hanal04) Bug 110345 INGSRV 2357
**          Added cs_thread_id so that we can successfully reap stacks
**          without refrencing the deallocated CS_SCB.
[@history_template@]...
*/
struct _CS_STK_CB
{
    CS_STK_CB       *cs_next;           /* Standard header */
    CS_STK_CB	    *cs_prev;
    SIZE_TYPE	    cs_size;
    i2		    cs_type;
#define                 CS_STK_TYPE     0x11f0
    i2		    cs_s_reserved;
    PTR		    cs_l_reserved;
    PTR	    	    cs_owner;
    i4		    cs_ascii_id;
#define                 CS_STK_TAG      CV_C_CONST_MACRO('>','S','T','K')
    CS_SID	    cs_used;		    /* owner if used, 0 if not */
# ifdef OS_THREADS_USED
    CS_THREAD_ID    cs_thread_id;           /* actual os thread id */
# endif /* OS_THREADS_USED */
    bool	    cs_reapable;            /* can be reaped */
    PTR		    cs_begin;		    /* first page of stack */
    PTR		    cs_end;		    /* last page of stack */
    PTR		    cs_orig_sp;		    /* starting stack pointer for stk */
};
# ifndef DESKTOP

/*}
** Name: CS_ADMIN_SCB - Session control for the special admin header
**
** Description:
**      This is the special session control block for the admin session.
**      It is different from the normal SCB because it has some additional
**      fields.
**
** History:
**      09-Jan-1987 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _CS_ADMIN_SCB
{
    CS_SCB	    csa_scb;		/* normal session control block stuff */
    i4              csa_mask;           /* what should be done now */
#define                 CSA_ADD_THREAD	0x1
#define                 CSA_DEL_THREAD	0x2
    CS_SCB	    *csa_which;		/* which session to delete */
    CS_SEMAPHORE    csa_sem;		/* semaphore to work with this block */
}   CS_ADMIN_SCB;
typedef struct _CS_MNTR_SCB
{
    CS_SCB	    csm_scb;		/* normal session control block stuff */
    i4		    csm_function;	/* what function are we performing */
    CS_SCB	    *csm_next;		/* next scb to process */
}   CS_MNTR_SCB;
# endif

/*}
** Name: CS_ARGTAB - table describing arguments to server
**
** Description:
**      longer description
**      about the structure 
**
** History:
**      03-jan-89 (anton)
**          Created.
*/
typedef struct _CS_ARGTAB
{
    char	*argname;
    char	argtype;
    PTR		argdata;
    i4		facility;
} CS_ARGTAB;

#ifdef LNX
/*
** NAME: CS_ALTSTK_SZ - Size of alternate stack (Linux Only)
**
** Description:
**	Used by sighandler when dealing with SIGSEGV, SIGILL or SIGBUS
*/
#define CS_ALTSTK_SZ	32384
#endif

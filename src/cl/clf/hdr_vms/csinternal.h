/*
** Copyright (c) 1987, 2009 Ingres Corporation
*/

#include <exhdef.h>
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
**	28-feb-1989 (rogerk)
**	    Added cs_pid field and stats for cross process semaphores to server
**	    control block.
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    21-mar-1990 (fred)
**		Added CPU Governor support.
**	    23-apr-90 (bryanp)
**		Added new statistics to the cs_wtstatistics field in the
**		Cs_srv_block to record information about CS_GWFIO_MASK - type
**		suspensions. These are used by non-SQL gateways.
**	15-jan-91 (ralph)
**          Change extent of CS_STARTUP_MSG.cs_options to CS_OPTMAX
**	4-jul-92 (daveb)
**		Add SPTREEs to CS_SYSTEM for scb, sem, and conditions.
**	1-Sep-1993 (seiwald)
**	    CS_OPTIONS and CS_STARTUP_MSG gone, as startup parameters no
**	    longer traverse the CL interface.  Added a few flags to the
**	    bottom of CS_SYSTEM to hold parameters once held in the
**	    startup message.  Commented as "defunct" cs_multiple in
**	    CS_SYSTEM, but didn't delete it: every csll.*.s depends on
**	    hardwired offsets into CS_SYSTEM!
**	31-jan-1994 (bryanp) B56917
**	    Added FUNC_EXTERN for CSoptions.
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p:
**	  31-may-95 (dougb)
**	    Add "volatile" to CS_SYSTEM and CS_ADMIN_SCB definitions and
**	    force compiler to longword align all examples in an attempt to
**	    avoid CL_INVALID_READY problems.  We want to avoid any code
**	    holding part of these structures in registers when the
**	    asynchronous routine CSresume() happens to run.  Don't want to
**	    resort to compiling the entire system "/noopt" again.
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**	21-dec-95 (dougb)
**	    Undo previous change.
**	22-feb-96 (albany)
**	    Cross-integrated jenjo02's Unix change of 16-jan-1996:
**		Changed cs_sm_* to cs_tm_*. cs_sm_* wasn't being used, so
**		cs_tm_* fields will be used to account for time-only  
**		CSsuspend() wait reasons.
**		Added cs_lge_*, cs_lke_* fields in which to track CSsuspend()
**		calls on LK/LG events.
**	19-aug-96 (kinpa04) incorporated changes of
**		27-jul-95 by jenjo02 of cs_hwm_active
**		25-oct-95 by thaju02/stoli02 wait on CSEVENT counter
**	30-Jul-97 (teresak) incorportated changes from Unix CL to VMS CL for 
**		threads
**		04-oct-1995 (wonst02)
**                Add cs_facility to CS_SYSTEM for the address of scs_facility().
**		03-jun-1996 (canor01)
**                Modified for operating system threads.
**                Add cs_facility to CS_SYSTEM for Unix as well as NT.
**                Add synchronization for global splay trees.
**              13-Aug-1996 (jenjo02)
**                Modified cs_smstatistics in CS_SYSTEM to provide better-fit
**                statistics for thread semaphores.
**              18-nov-1996 (canor01)
**                Synchronization primitives in CS_SYSTEM for the splay trees 
**		  are for OS-thread systems only.
**              10-dec-96 (hanch04)
**                moved cs_facility, offset is hardcoded in cs.
**      	5-dec-96 (stephenb)
**      	  Add definition of CS_PROFILE to CS_SYSTEM for profiling.
**      	18-feb-1997 (hanch04)
**        	  As part of merging of Ingres-threaded and OS-threaded servers,
**        	  CS_SYSTEM will contain both versions for SMSTAT
**      	28-feb-1997 (hanch04)
**        	  cs_mt will be part of CS_SYSTEM for threads or no threads
**      	21-may-1997 (shero03)
**           	  Add definition of CS_DEV_TRCENTRY for debugging
**      	03-Jul-97 (radve01)
**          	  Several fields added to the end of the server block in 
**		  order to facilitate debugging.
**      	26-Aug-1997 (kosma01)
**          	  For OS_THREADS_USED, cast special session ids into threads
**          	  friendly types. Also add a NULL session id CS_NULL_ID.
**		19-may-1998 (kinte01)
**		  Cross integrate Unix change 434579 into AlphaVMS CL
**		  03-Dec-1997 (jenjo02)
**		    Added cs_get_rcp_pid to CS_CB.
**		19-may-1998 (kinte01)
**		  Cross-integrate Unix change 434547
**	          16-Feb-98 (fanra01)
**         	     Add attach and detach functions from IMA so that the 
**		     structures can be registered when the thread is active.
**              20-may-1998 (kinte01)
**                Cross-integrate Unix change 435120
**      	  09-Mar-1998 (jenjo02)
**          	    Moved CS_MAXPRIORITY, CS_MINPRIORITY, CS_DEFPRIORITY to
**                  csnormal.h to expose them to SCF.
**      22-jun-1999 (horda03)
**          Cross-integration of change 440975.
**      29-Jan-99 (hanal04, horda03)
**          Added statistics required as part of re-engineering of
**          CSp_semaphore() and CSv_semaphore().
**      19-Sep-2000 (horda03)
**          Do not use the MEMBER_ALIGNMENT prgmas if AXM_VMS defined.
**          Once Ingres is built solely with /MEMBER_ALIGNMENT the pragmas and
**          the AXM_VMS definition can be removed.
**	25-jul-00 (devjo01)
**	    Increase size of cs_crb_buf from 256 to 512 to allow for
**	    larger size of CL_SYS_ERR structs embedded in GCA structures
**	    introduced by change 444221.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	05-sep-2003 (abbjo03)
**	    In CS_SYSTEM, cs_elog should return void.  
**	23-Oct-03 (devjo01)
**	    hand cross changes for 103715 from Unix CL
**	13-Feb-2007 (kschendel)
**	    Added wall-clock ticker so that servers have an independently
**	    running clock.  (for OS-threaded only.)
**	14-oct-2008 (joea)
**	    Integrate 16-nov-1998 and 29-sep-2000 changes from unix version.
**	    Add new wait/counts/times, for Reads/Writes, Log I/O.  Remove
**	    unsplit time/wait/counts for cs_bio/dio.
**	26-nov-2008 (joea)
**	    Include <exhdef.h> for cs_exhblock member of CS_SYSTEM.  Additional
**	    changes to make CS_SYSTEM and CS_STK_CB more consistent with the
**	    Unix version.
**	06-jan-2009 (joea)
**	    Add cs_size_io_buf, cs_num_io_buf and cs_event_mask members to
**	    CS_SYSTEM.
**      08-mar-2009 (stegr01)
**          Add a couple more fields to align the structures to support MUltithreading
**          remove member alignment macros for Alpha
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef volatile struct _CS_SYSTEM CS_SYSTEM;
typedef struct _CS_STK_CB   CS_STK_CB;
typedef struct _CS_STARTUP_MSG CS_STARTUP_MSG;

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

#define                 CS_FIRST_ID     (CS_SID)0x0100      /* first free id */
#define                 CS_NULL_ID      (CS_SID)0x0         /* null id, not a session anymore */
# else
#define                 CS_SCB_HDR_ID   0x0001
#define                 CS_IDLE_ID      0x0011
#define                 CS_MNTR_ID      0x0012

#define                 CS_FIRST_ID     0x0100	    /* first free id */
# endif   /* OS_THREADS_USED */


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
**	NOTE : This structure reference inside CSLL.MAR with hard-wired
**	offsets to some of the fields.  Any changes to the layout of
**	this structure will require changes in the offset definitions in
**	CSLL.MAR as well.
**
** History:
**      27-Oct-1986 (fred)
**          Initial Definition.
**	 7-nov-1988 (rogerk)
**	    Added fields - cs_async, cs_as_list, cs_scb_async_cnt and
**	    cs_asadd_thread - for asynchronous routines to write information
**	    into.  This is to enable CS to do most operations without having
**	    to enable/disable asts.
**	28-feb-1989 (rogerk)
**	    Added cs_pid field and stats for cross process semaphores.
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added statistics for CS_GWFIO_MASK waits to the wait statistics.
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p:
**	  31-may-95 (dougb)
**	    Force compiler to longword align all examples of this structure
**	    type in an attempt to avoid CL_INVALID_READY problems.  Reduces
**	    the possibility that updates to one field in asynchronous code
**	    (running at AST level in VMS terms) will be lost/invalidated by
**	    a change to an adjacent field in synchronous code.  Also,
**	    shortens the number of instructions during which a field will be
**	    kept in registers.  This is similar to a change markg made to
**	    the CS_SCB structure (on or before 22-feb-95) in ingres63p.
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**	21-dec-95 (dougb)
**	    Undo previous change.
**	22-feb-96 (albany)
**	    Cross-integrated jenjo02's change of 16-jan-1996:
**		Changed cs_sm_* to cs_tm_*. cs_sm_* wasn't being used, so
**		cs_tm_* fields will be used to account for time-only  
**		CSsuspend() wait reasons.
**		Added cs_lge_*, cs_lke_* fields in which to track CSsuspend()
**		calls on LK/LG events.
**      30-Jul-97 (teresak) incorporated changes from Unix CL to VMS CL for
**              threads
**      	13-Aug-1996 (jenjo02)
**                 Modified cs_smstatistics in CS_SYSTEM to provide better-fit
**                 statistics for thread semaphores.
**      	18-nov-1996 (canor01)
**      	   Synchronization primitives for the splay trees are for
**      29-Jan-99 (hanal04, horda03)
**          Added statistics required as part of re-engineering of
**          CSp_semaphore() and CSv_semaphore().
**          	   OS-thread systems only.
**      29-Jan-99 (hanal04, horda03)
**          Added statistics required as part of re-engineering of
**          CSp_semaphore() and CSv_semaphore().
**	23-Oct-03 (devjo01)
**	    hand cross changes for 103715 from Unix CL. Added cs_format_lkkey
*/

/*
** Need value of GCC_L_PORT from this include file
*/
# include       <gcccl.h>

#ifdef xDEV_TRACE

#define CS_DEV_TRACE_ENTRIES 5000

typedef struct _CS_DEV_TRCENTRY CS_DEV_TRCENTRY;

    struct      _CS_DEV_TRCENTRY
    {
        CS_SCB          *cs_dev_trc_scb;        /* The current SCB */
        u_i2            cs_dev_trc_parm;        /* optional parm */
        u_i2            cs_dev_trc_fac;         /* calling facility */
        int             cs_dev_trc_at;          /* calling from line */
        PTR             cs_dev_trc_opt;         /* optional ptr  */
    };
#endif /*xDEV_TRACE */




struct _CS_SYSTEM
{
    char            cs_header[16];      /* to make easy to find in a dump */
#define                 CS_MARKER       "CS_SYSTEM BLOCK"
    STATUS          (*cs_scballoc)();   /* Routine to get SCB's from */
    STATUS          (*cs_scbdealloc)(); /* Routine thru which to give them back */
    void	    (*cs_elog)();	/* Routine with which to log errors */
    STATUS          (*cs_process)();    /* General processing of stuff ... */
    STATUS          (*cs_attn)();	/* Processing of async stuff ... */
    STATUS	    (*cs_startup)();	/* To start up the server */
    STATUS	    (*cs_shutdown)();	/* To shut the server down */
    STATUS	    (*cs_format)();	/* format scb's */
    STATUS	    (*cs_saddr)();	/* To get session requests */
    STATUS	    (*cs_reject)();	/* To reject session requests */
    STATUS	    (*cs_disconnect)();	/* To disconnect sessions */
    STATUS	    (*cs_read)();	/* To read from FE's */
    STATUS	    (*cs_write)();	/* To write to FE's */
    i4              cs_max_sessions;	/* how many sessions are allowed */
    i4              cs_num_sessions;    /* How many are there right now */
    i4              cs_user_sessions;	/* How many user sessions are there */
    i4              cs_max_active;      /* How many threads can be active */
    i4              cs_hwm_active;      /* high water mark of active threads */
    i4		    cs_cursors;		/* how many active cursors per session */
    i4              cs_num_active;      /* how many threads are currently active */
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
# endif /* OS_THREADS_USED OR POSIX_THREADS*/
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
#define                 CS_REPENTING    0x70
    i4		    cs_mask;		/* special values and states */
#define			CS_NOSLICE_MASK	    0x0001 /* No time slicing */
#define			CS_ADD_MASK	    0x0002
#define			CS_SHUTDOWN_MASK    0x0004 /* Server will shut down
						   ** when all threads exit. */
#define			CS_FINALSHUT_MASK   0x0008 /* Server is shutting down */
#define			CS_ACCNTING_MASK    0x0010 /* Write records for each
						   ** session to accounting 
						   ** file. */
#define                 CS_CPUSTAT_MASK     0x0020 /*
                                                   ** Collect CPU stats on for
                                                   ** each session.
                                                   */
#define                 CS_REPENTING_MASK   0x0040 /* Currently repenting */
#define                 CS_LONGQUANTUM_MASK 0x0080 /* Quantum > 1 second */

# ifdef OS_THREADS_USED
    CS_SYNCH        cs_cnt_mutex;       /* mutex for global counts */
# endif /* OS_THREADS_USED */

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
    u_i4            cs_quantums;        /* number of quantums gone by */
    u_i4	    cs_idle_time;       /* number of q where task is idle */
    i4		    cs_cpu;		/* cpu time of process so far */
    PTR		    cs_svcb;		/* anchor for underlying code */
    $EXHDEF	    cs_exhblock;	/* block for VMS exit handler */
    CS_STK_CB	    *cs_stk_list;	/* pointer to list of known stacks */
    char	    cs_name[16];	/* name of this server */
    char	    cs_crb_buf[512];	/* buffer for unsolicited CRB's */
    CS_SCB	    *cs_as_list;	/* List of SCB's that became ready
					** while CS was busy (inkernel) and
					** are waiting to be moved to the ready
					** queue. */
    i4		    cs_scb_async_cnt;	/* Number of SCB's with cs_async on */
    i4		    cs_asadd_thread;	/* Received an add-thread request
					** while CS was busy. */
    PID		    cs_pid;		/* Process Id */
    i4	    	    cs_event_mask;	/* Mask of outstanding event waits */
#define			CS_EF_CSEVENT	0x01
    struct  _CS_SMSTAT
    {
	u_i4	    cs_smsx_count;	/* collisions on shared vs. exclusive */
	u_i4	    cs_smxx_count;	/* exclusive vs. exclusive */
	u_i4	    cs_smcx_count;	/* collisions on cross-process */
	u_i4	    cs_smx_count;	/* # requests for exclusive */
	u_i4	    cs_sms_count;	/* # requests for shared */
	u_i4	    cs_smc_count;	/* # requests for cross-process */
	u_i4	    cs_smcl_count;	/* # spins waiting for cross-process */
	u_i4	    cs_smcs_count;	/* # sleeps waiting for cross-process */
        u_i4        cs_smsp_count;  /* # "single-processor" swusers */
        u_i4        cs_smmp_count;  /* # "multi-processor" swusers */
        u_i4        cs_smnonserver_count;
                                        /* # "non-server" naps */
# ifdef OS_THREADS_USED
        u_i4        cs_smssx_count; /* sp collisions on shared vs. exclusive */
        u_i4        cs_smsxx_count; /* sp exclusive vs. exclusive */
        u_i4        cs_smss_count;  /* sp # requests for shared */
        u_i4        cs_smmsx_count; /* mp collisions on shared vs. exclusive */
        u_i4        cs_smmxx_count; /* mp exclusive vs. exclusive */
        u_i4        cs_smmx_count;  /* mp # requests for exclusive */
        u_i4        cs_smms_count;  /* mp # requests for shared */
# endif /* OS_THREADS_USED */
 
    }		    cs_smstatistics;	/* statistics for semaphores */
    struct  _CS_TMSTAT
    {
	u_i4		cs_bio_idle;	/* bio's while idle */

	u_i4		cs_bior_time;	/* tick count in bio state */
	u_i4		cs_bior_waits;
    u_i4        cs_bior_kbytes;
	u_i4		cs_bior_done;	/* number done altogether */

	u_i4		cs_biow_time;	/* tick count in bio state */
	u_i4		cs_biow_waits;
    u_i4        cs_biow_kbytes;
	u_i4		cs_biow_done;	/* number done altogether */

	u_i4		cs_dio_idle;	/* dio's while idle */

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

	u_i4		cs_event_wait;
	u_i4		cs_event_count;

        u_i4           cs_gwfio_time;  /* statistics for non-SQL Gateway */
        u_i4           cs_gwfio_waits; /* sessions which use the new     */
        u_i4           cs_gwfio_idle;  /* CS_GWFIO_MASK parameter to     */
        u_i4           cs_gwfio_done;  /* CSsuspend().                   */
    }		    cs_wtstatistics;	/* times for various wait states */

    i4              cs_gpercent;        /*
                                        ** Governed percentage -- Server should
                                        ** not use more cpu than this
                                        ** percentage.
                                        */
    i4              cs_gpriority;       /* VMS priority while repenting */
    u_i4            cs_norm_priority;   /* VMS priority normally */
    u_i4              cs_int_time[2];     /*
                                        **  Interval beginning time for
                                        **  governing CPU usage.
                                        */
    i4              cs_int_cpu;         /* CPU time of that interval */
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
    CS_DEV_TRCENTRY     *cs_dev_trace_strt;     /* trace area start */
    CS_DEV_TRCENTRY     *cs_dev_trace_end;      /* trace area end */
    CS_DEV_TRCENTRY     *cs_dev_trace_curr;     /* addr(current trace entry) 
*/
#endif /* xDEV_TRACE */
    SPTREE	    cs_scb_tree;
    SPTREE	    cs_cnd_tree;
    SPTREE	    *cs_scb_ptree;
    SPTREE	    *cs_cnd_ptree;
    bool          	cs_define;          /* set II_DBMS_SERVER */
# ifdef OS_THREADS_USED
    CS_SYNCH        cs_scb_mutex;
    CS_SYNCH        cs_cnd_mutex;
# endif /* OS_THREADS_USED */
    i4       	cs_quantum;         /* timeslice (units?) */
    i4         	cs_max_ldb_connections;
    i4		cs_size_io_buf;	    /* transfer size for io slave i/o */
    i4		cs_num_io_buf;	    /* number of i/o slave event cb's */
    STATUS	    (*cs_diag)();	/* Diagnostics */
    STATUS          (*cs_facility)();   /* return current facility */  
    void            (*cs_scbattach)();  /* Attach thread control block to MO */
    void            (*cs_scbdetach)();  /* Detach thread control block */
    char*	    (*cs_format_lkkey)(); /* Fmt LK_LOCK_KEY for display. */
# ifdef OS_THREADS_USED
    CS_SYNCH        cs_semlist_mutex;   /* protect semaphore lists */
    PID             (*cs_get_rcp_pid)();/* To realize RCP's pid */
    PID             cs_rcp_pid;         /* Process Id of RCP */
# endif /* OS_THREADS_USED */
    volatile u_i4	cs_ticker_time;	    /* Wall clock timer as maintained
					    ** by the clock-ticker thread (may
					    ** not be used on all platforms).
					    ** In seconds since some epoch.
					    */
    bool	    	cs_stkcache;        /* cache stacks with os-threads */
    bool          	cs_isstar;          /* is this a star server? */
    bool                cs_mt;              /* use os threads? */
   /*
    ** Useful diagnostic info at the end of the block
    */
    i4    sc_main_sz;   /* Size of SCF Control Block(SC_MAIN_CB,Sc_main_cb) */
    i4    scd_scb_sz;   /* Size of Session Control Block(SCD_SCB) */
    i4    scb_typ_off;  /* Type of the session offset inside session cb. */
    i4    scb_fac_off;  /* The current facility offset inside session cb. */
    i4    scb_usr_off;  /* The user-info off inside session control block */
    i4    scb_qry_off;  /* The query-text offset in session control block */
    i4    scb_act_off;  /* The current activity offset in session cb. */
    i4    scb_ast_off;  /* The alert(event) state offset in session cb. */
volatile PTR srv_blk_end; /* Keep this line et the end of the structure, */
                         /* The variable will be used for debugging purposes 
*/

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
**	09-Nov-2001 (kinte01)
**	    cs_used needs to be CS_SID
**	    Made l_reserved & owner elements PTR for consistency with DM_SVCB. 
**      25-Jun-2003 (hanal04) Bug 110345 INGSRV 2357
**          Added cs_thread_id so that we can successfully reap stacks
**          without refrencing the deallocated CS_SCB.
*/
struct _CS_STK_CB
{
    CS_STK_CB       *cs_next;           /* Standard header */
    CS_STK_CB	    *cs_prev;
    i4		    cs_size;
    i2		    cs_type;
#define                 CS_STK_TYPE     0x11f0
    i2		    cs_s_reserved;
    PTR		    cs_l_reserved;
    PTR		    cs_owner;
    i4		    cs_ascii_id;
#define                 CS_STK_TAG      '>STK'
    CS_SID	    cs_used;		    /* owner if used, 0 if not */
# ifdef OS_THREADS_USED
    CS_THREAD_ID    cs_thread_id;           /* actual os thread id */
# endif /* OS_THREADS_USED */
    PTR		    cs_begin;		    /* first page of stack */
    PTR		    cs_end;		    /* last page of stack */
    PTR		    cs_orig_sp;		    /* starting stack pointer for stk */
};

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
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p:
**	  31-may-95 (dougb)
**	    Force compiler to longword align all examples of this structure
**	    type in an attempt to avoid CL_INVALID_READY problems.  Done for
**	    same reasons as the CS_SYSTEM change (plus, new compiler
**	    warnings).  Volatile due to single reference in CSresume().
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**	21-dec-95 (dougb)
**	    Undo previous change.
*/

typedef volatile struct _CS_ADMIN_SCB
{
    CS_SCB	    csa_scb;		/* normal session control block stuff */
    i4             csa_mask;           /* what should be done now */
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

/*
** External declarations of CS functions. Functions placed here are those which
** are visible throughout the CL (or throughout the CSmodule), but which are
** NOT visible to the mainline, and hence do NOT belong in CS.H.
*/
FUNC_EXTERN STATUS  CSoptions(CS_SYSTEM *cssb);

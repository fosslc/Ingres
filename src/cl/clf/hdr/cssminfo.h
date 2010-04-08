/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: CSSMINFO.H - System wide information about shared memory segments
**
** Description:
**	The dbms server maintain 3 shared memory segments per server.
**
**	The first shared memory segment "system control block" will maintain
**	information necessary to find all the other shared memory segments.
**	It will also contain fields which are particular to each server in
** 	the installation.  This segment is mapped by every process which
**	uses any sort of shared memory or needs to use the CS event mechanism
**	(ie. dbms server, dbms slaves, acp, and recovery process.)
**	This shared memory segment may be mapped anywhere possible in a process
**	since it will only contain position independant data structures.
**	This shared memory segment is created by an program run at installation
**	startup time.  Programs will map to it using an id derived from the
**	II_INSTALLATION variable.
**
**	The second shared memory segment contains both the logging and locking
**	tables.  It's layout contains first a control block used to manage the
**	segments memory, then the locking control block "LKD" and
**	then the logging control block "LGD".  Following these control 
**	structures is basically heap space maintained by the logging and locking
**	code.  This shared memory segment currently must be mapped to the same 
**	address in every process (this address can is found in the "system 
**	control block").  This shared memory segment is also created by a 
**	program run at installation startup time.  Programs will map to it using
**	an id derived from the II_INSTALLATION variable.  Only users of the
**	logging and locking will have to map this segment (ie. dbms server,
**	acp, and recovery process - dbms slaves will definitiely not have to 
**	map this memory thus making them much smaller).
**
**	The last shared memory segment is allocated one per server.  This 
**	segment contains all shared memory needed by a server.  Information
**	stored in this includes slave/server interaction, event mechanisms,
**	and perhaps eventually dbms buffer manager.  This segment is 
**	dynamically created by the server at server startup time.  Only the
**	dbms server and the dbms slaves should have to map this segment in.
**	This segment should only contain position independent data structures,
**	and so can be mapped to any address in a process.
**
** History: 
**      25-jan-88 (mmm)
**          Created.
**	15-feb-89 (mmm)
**	    bumped CSS_VERSION to 610008.  This was done because the LGK 
**	    control segment changed.
**	21-Jul-89 (anton)
**	    Bumped CSS_VERSION to 700001 for terminator.
**	10-aug-89 (mikem)
**	    Bumped CSS_VERSION to 700002, after changing the size of the
**	    LGK_ACB structure.
**      15-jan-90 (fls-sequent)
**          Added csi_wakeme_pid for MCT server.
**	18-jan-90 (mikem)
**	    Bumped CSS_VERSION to 700003, after changing the size of the
**	    LGD for online checkpoint changes/NSE bug fix integration.
**	23-mar-90 (mikem)
**	    Bumped CSS_VERSION to 700004, after changing the size of the
**	    LGD for added performance stat gathering.
**      17-may-90 (blaise)
**        Integrated changes from 61 and ug:
**            Force error if <clconfig.h> has not been not included;
**            Remove superfluous typdefs before structure declarations;
**	17-may-90 (rogerk)
**	    Bumped CSS_VERSION to 700005, after changing the size of the
**	    lgd_buffer field of LGD.
**      26-jun-90 (fls-sequent)
**	    Changed CS atomic locks to SHM atomic locks.
**      26-jun-90 (fls-sequent)
**	    Bumped CSS_VERSION to 700006 for MCT.
**      22-oct-90 (fls-sequent)
**	    Integrated changes from 63p CL:
**		Force error if <clconfig.h> has not been not included.
**		Remove superfluous typdefs before structure declarations.
**		Add code to handle Convex semaphores (xCL_078_CVX_SEM).
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Bumped up CSS_VERSION to 700005.
**	30-Jan-91 (anton)
**	    Set CSS_VERSION to 630401
**	    Put in MCT support
**	11-apr-91 (gautam)
**	    Changed csi_connect_id to char[64] to allow for large connect ids
**	26-apr-1991 (bryanp)
**	    Updated CSS_VERSION to 630402 to reflect change in csi_connect_id.
**	6-aug-1991 (bryanp)
**	    Updated CSS_VERSION to 630403 to reflect changes in DI_SLAVE_CB.
**	3-march-1992 (jnash)
**          Updated CSS_VERSION to 630404 to reflect changes in DI_SLAVE_CB.
**	20-sep-1993 (mikem)
**	    Updated CSS_VERSION to 650102 to reflect new size of CS_SEMAPHORE
**	    (and thus new size's of all shared memory segments which contain
**	    CS_SEMAPHORE's).
**	31-jan-1994 (mikem)
**	    Updated CSS_VERSION to 650103 to reflect new size of CS_SMCNTRL
**	    which had the css_wakeup field added.
**	20-apr-1994 (mikem)
**	    Updated CSS_VERSION to 650104 to reflect new size of CS_SMCNTRL
**	    which had the css_clock field added.
**      12-Dec-95 (fanra01)
**          Added IMPORT_DLL_DATA section for references by statically built
**          code to data in DLL.
**	03-jun-1996 (canor01)
**	    Change to use operating system thread synchronization objects
**	    when operating system threads are enabled.
**	16-Nov-1998 (jenjo02)
**	    Relocated CS_SM_WAKEUP_CB to cl/hdr/hdr/csnormal.h where it is
**	    now referenced by CS_SCB.
**	30-may-2000 (toumi01)
**	    Add csi_signal_pid field.  This is the pid for sending signals.
**	    If the server is not threaded, or if the OS server model
**	    runs all threads under the same pid, then csi_signal_pid
**	    will be equal to csi_pid.  However, this new field is needed
**	    for servers where the main thread and idle thread pids can
**	    be different because of the 1:1 thread model (e.g. Linux).
**	    In effect, this change undoes the overloading of csi_pid to
**	    mean both the pid of the main server thread and the pid of
**	    the thread that can receive (e.g. SIGUSR2) signals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	29-Sep-2009 (frima01) 121804
**	    Add CS_get_cb_dbg prototype to avoid warnings from gcc 4.3
**/

# ifndef CLCONFIG_H_INCLUDED
        error "didn't include clconfig.h before cssminfo.h"
# endif

# ifdef xCL_NEED_SEM_CLEANUP
# include <lo.h>
# endif
/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _CS_SMCNTRL CS_SMCNTRL;
typedef struct _CS_SERV_INFO CS_SERV_INFO;
typedef struct _CS_SM_DESC CS_SM_DESC;
# ifdef xCL_NEED_SEM_CLEANUP
typedef struct _CS_SHM_IDS CS_SHM_IDS;
# endif

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID CS_get_cb_dbg(PTR     *address);
FUNC_EXTERN STATUS CS_create_sys_segment(i4, i4, CL_ERR_DESC *);    /* create sys cntrl block seg */
FUNC_EXTERN STATUS CS_map_sys_segment();       /* map system cntrl block seg */
FUNC_EXTERN STATUS CS_alloc_serv_segment(SIZE_TYPE, u_i4 *, PTR *, CL_ERR_DESC *);    /* create serv shared mem seg */
FUNC_EXTERN STATUS CS_map_serv_segment(u_i4, PTR *, CL_ERR_DESC *);      /* map serv shared mem seg. */
FUNC_EXTERN STATUS CS_destroy_serv_segment(u_i4, CL_ERR_DESC *);  /* destroy serv shared mem seg */
FUNC_EXTERN bool CS_is_server(i4);
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF CS_SMCNTRL *Cs_sm_cb;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF CS_SMCNTRL *Cs_sm_cb;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */


/*
**  Defines of other constants.
*/
# ifdef xCL_NEED_SEM_CLEANUP
#define			CS_SHMSEG	128
# endif

/*
**	This sets the number of cross process semaphores needed for an
**	installation beyond one per server.
*/

#define			CSCP_NUM	5
#define			MAXSEMS		32

/*}
** Name: CS_SM_DESC - Shared memory segment descriptor
**
** Description:
**	Information describing shared memory segment.
**
** History:
**      08-feb-88 (mmm)
**	    written.
**	07-sep-2004 (thaju02)
**	    Changed cssm_size from u_i4 to SIZE_TYPE.
*/
struct _CS_SM_DESC
{
    i4	    cssm_id;		/* id of segment (used to attach) */
    SIZE_TYPE       cssm_size;          /* size of shared memory segment */
    PTR             cssm_addr;          /* address at which to attach - 
					** only makes sense for pieces attached
					** at specific places - set to -1 for
					** others.
					*/
    u_i4	    cssm_unused;
};
/*}
** Name: CS_SM_WAKEUPS - information to manage array of CS_SM_WAKEUP_CB's
**
** Description:
**	Contains information to manage the dynamically allocated array of
**	CS_SM_WAKEUP_CB's.
**
** History:
**      01-jan-94 (mikem)
**          Created.
*/
typedef struct
{
    i4	    css_numwakeups;	/* max number of wakeup blocks */
    i4	    css_minfree;	/* start search for free block here*/
    i4         css_maxused;        /* largest used wakeup block */
} CS_SM_WAKEUPS;

/*}
** Name: CS_SM_CLOCK - information to manage shared memory clock.
**
** Description:
**      Contains information to manage the UNIX cl shared memory "clock".
**
** History:
**      20-apr-94 (mikem)
**          Created.
**	23-feb-2001 (devjo01)
**	    Add fields 'cscl_calls_since_inc', and 'cscl_numcalls_before_inc'
**	    to throttle down increment of 'cscl_qcount' on fast boxes where
**	    CS_check_time() is called more than 1000 times a second.
**	    Reduce size of 'cscl_calls_since_upd' & 
**	    'cscl_numcalls_before_update' to keep structure size the same.
**	    (b104072/INGSRV1389)
**      28-July2003 (hanal04) Bug 110468 INGSRV 2367
**          Added cscl_calls_since_idle to track cases where the internal
**          clock may be running very slowly because there are no active
**          sessions driving the existing counters towards a system time
**          update.
*/
typedef struct
{
    i4      cscl_secs;     	/* absolute seconds, last update */
    i4      cscl_msecs;        	/* absolute milliseconds, last update */
    i4	    cscl_qcount;	/* pseudo-quantum in ms., last update */
    i4	    cscl_fixed_update;  /* time in ms to bump clock each call */
    u_i2    cscl_calls_since_upd; /* calls since syscall to get time */
    u_i2    cscl_numcalls_before_update; /* # of calls before making syscall */
    u_i2    cscl_calls_since_inc; /* calls since last increment of quantum */
    u_i2    cscl_numcalls_before_inc;  /* # calls which between increments. */ 
    u_i2    cscl_calls_since_idle; /* # calls made since idle thread last ran */
} CS_SM_CLOCK;


/*}
** Name: CS_SERV_INFO - short comment about the structure
**
** Description:
**	The last shared memory segment is allocated one per server.  This 
**	segment contains all shared memory needed by a server.  Information
**	stored in this includes slave/server interaction, event mechanisms,
**	and perhaps eventually dbms buffer manager.  This segment is 
**	dynamically created by the server at server startup time.  Only the
**	dbms server and the dbms slaves should have to map this segment in.
**	This segment should only contain position independent data structures,
**	and so can be mapped to any address in a process.
**
** History:
**      01-jan-88 (mmm)
**          written.
**      15-jan-90 (fls-sequent)
**          Added csi_wakeme_pid for MCT server.
**      26-jun-90 (fls-sequent)
**          Changed CS_ASET to SHM_ATOMIC and CS_SPIN to SHM_SPIN.
**	30-Jan-91 (anton)
**	    back to CS_ASET and CS_SPIN
*/
struct _CS_SERV_INFO
{
    u_i4	    csi_in_use;		/* used by some server */
    CS_SM_DESC	    csi_serv_desc;	/* desc of server shared mem segment */
    i4	            csi_pid;		/* pid of this server */
    i4		    csi_signal_pid;	/* pid for sending SIGUSR2 to server */
    CS_ASET	    csi_event_done[MAXSERVERS];
			/* mask of outstanding events for this server */
    CS_SPIN	    csi_spinlock;	/* spin lock on this structure */
    CS_ASET	    csi_nullev;		/* null events outstanding */
    CS_ASET	    csi_events_outstanding; /* indicate events to complete */
    CS_ASET	    csi_wakeme;		/* if TAS succeeds, wake server */
    CS_ASET	    csi_subwake[NSVSEMS];
					/* if TAS succeeds, wake slave */
    char	    csi_connect_id[64];	/* value to set II_DBMS_SERVER to get
					** to this server.
					*/
    i4	    csi_id_number;	/* array entry of this element in the
					** CS_SERV_INFO array.
					*/
# ifdef xCL_075_SYS_V_IPC_EXISTS
    int		    csi_semid;		/* id of semaphores for this server */
# endif
# ifdef xCL_078_CVX_SEM
    struct semaphore csi_csem[MAXSLAVES]; /* for non-Sys V semaphores */
# endif
    int		    csi_nsems;		/* number of semaphores in group */
    int		    csi_usems;		/* number of used semaphores */
    CSSL_CB	    csi_events;		/* events for this server */
};

/*
** Name: CS_SHM_IDS -
**
** Description:
**
**   On some systems, currently DGUX, the posix thread mutex and condition
**   resources allocate kernel resources in addition to structures within
**   the shared memory area itself.  On these systems, the kernel resources
**   are not de-allocated when the shared memory area is deleted, but must
**   be explicitly destroyed by system library calls.
**   This stucture is defined as part of the system segment and is used
**   to track all calls to CSMTi_semaphore and CSMTr_semaphore.
**   The structure contains the key of the shared memory area, the offset
**   within the shared memory area of the first semaphore, and count of
**   the number of semaphores on the chain.
** 
** History:
**	09-Feb-1998 (bonro01)
**		Created.
** 
*/
# ifdef xCL_NEED_SEM_CLEANUP
struct _CS_SHM_IDS
{
	char	key[MAX_LOC];
	QUEUE	offset;
	int		count;
};
# endif

/*}
** Name: CS_SMCNTRL - Information used to get to all other shared mem info.
**
** Description:
**	
**	The first shared memory segment "system control block" will maintain
**	information necessary to find all the other shared memory segments.
**	It will also contain fields which are particular to each server in
** 	the installation.  This segment is mapped by every process which
**	uses any sort of shared memory or needs to use the CS event mechanism
**	(ie. dbms server, dbms slaves, acp, and recovery process.)
**	This shared memory segment may be mapped anywhere possible in a process
**	since it will only contain position independant data structures.
**	This shared memory segment is created by an program run at installation
**	startup time.  Programs will map to it using an id derived from the
**	II_INSTALLATION variable.
**
** History:
**      25-jan-88 (mmm)
**          Created.
**	10-aug-89 (mikem)
**	    Updated CSS_VERSION to 700002, after changing the size of the
**	    LGK_ACB structure.
**	18-jan-90 (mikem)
**	    Bumped CSS_VERSION to 700003, after changing the size of the
**	    LGD for online checkpoint changes/NSE bug fix integration.
**	23-mar-90 (mikem)
**	    Bumped CSS_VERSION to 700004, after changing the size of the
**	    LGD for added performance stat gathering.
**      4-june-90 (blaise)
**          Integrated changes from termcl code line:
**              Bumped up CSS_VERSION to 700005.
**	17-may-90 (rogerk)
**	    Bumped CSS_VERSION to 700005, after changing the size of the
**	    lgd_buffer field of LGD.
**      26-jun-90 (fls-sequent)
**          Changed CS_SPIN to SHM_SPIN.
**      26-jun-90 (fls-sequent)
**	    Bumped CSS_VERSION to 700006 for MCT.
**	30-Jan-91 (anton)
**	    Set CSS_VERSION to 630401
**	    back to CS_SPIN
**	26-apr-1991 (bryanp)
**	    Updated CSS_VERSION to 630402 to reflect change in csi_connect_id.
**	6-aug-1991 (bryanp)
**	    Updated CSS_VERSION to 630403 to reflect changes in DI_SLAVE_CB.
**	3-jul-1992 (bryanp)
**	    Updated CSS_VERSION to 650101 to reflect new LG/LK system.
**	    Removed css_lglk_desc;
**	20-sep-1993 (mikem)
**	    Updated CSS_VERSION to 650102 to reflect new size of CS_SEMAPHORE
**	    (and thus new size's of all shared memory segments which contain
**	    CS_SEMAPHORE's).
**	31-jan-1994 (mikem)
**	    Updated CSS_VERSION to 650103 to reflect new size of CS_SMCNTRL
**	    which had the css_wakeup field added.
**	25-apr-1994 (mikem)
**	    Updated CSS_VERSION to 650104 to reflect new size of CS_SMCNTRL
**	    which had the css_clock field added.
**	03-jun-1996 (canor01)
**	    Updated CSS_VERSION to 650200 to reflect new size of CS_SMCNTRL
**	    which has new definitions (possibly) for CS_SPIN.
**      28-July2003 (hanal04) Bug 110468 INGSRV 2367
**          Bumped CSS_VERSION to 650201 to reflect changes in CS_SM_CLOCK.
**	5-Apr-2006 (kschendel)
**	    Remove constant version, wasn't being updated and wasn't good
**	    enough.  CS will construct a version from the build-time
**	    version constants, plus the installation ID.
**	    
*/
struct _CS_SMCNTRL
{
    CS_SM_DESC	    css_css_desc;	/* desc of control shared mem seg. */
    i4	    css_version;	/* version number of this segment */
				/* Will be constructed from GV and the
				** installation ID */
    i4	    css_numservers;	/* max number of servers */
    CS_SM_WAKEUPS   css_wakeup;		/* control info for wakeup blocks */
    CS_SM_CLOCK     css_clock;		/* control info for shared mem clock */
# ifdef xCL_075_SYS_V_IPC_EXISTS
    int		    css_semid;		/* Sys V semaphore identifier */
# endif
# ifdef xCL_078_CVX_SEM
    struct semaphore css_csem[MAXSERVERS+CSCP_NUM]; /* non-Sys V semaphores */
# endif
    i4		    css_nsem;		/* number of Sys V semaphores in id */
    i4		    css_usem;		/* number of semaphores initialized */
					/* Note: the first 'numservers'
					   semaphores are the server
					   semaphores */
    CS_SPIN	    css_spinlock;	/* spin lock for this structure */
    i4	    css_qcount;		/* pseudo-quantum counter */
#define	CS_ADD_QUANTUM(c)	Cs_sm_cb->css_qcount += (c)
    i4	    css_events_off;	/* offset to free space for event
					   handling */
    i4	    css_events_end;	/* offset to end of free space for
					   event handling */
    i4	    css_wakeups_off;	/* offset to wakeup control blocks */
# ifdef xCL_NEED_SEM_CLEANUP
	CS_SYNCH	css_shm_mutex;	/* Protect Shm Semaphore list */
	CS_SHM_IDS	css_shm_ids[CS_SHMSEG];	/* Cross Process Semaphores chain */
# endif
    CS_SERV_INFO    css_servinfo[1];	/* Number of entries in this
					** array is given by css_numservers.
					** This member must always be last.
					*/
};

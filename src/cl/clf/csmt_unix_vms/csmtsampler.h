/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/****************************************************************************
**
** Name: cssampler.h - The include file for the monitor's sampler thread.
**
** Description:
**      This file contains the definitions of datatypes and constants
**      used exclusively within the CSsampler program.
**
** History:
**	08-sep-95 (wonst02)
**	    Original.
**	21-sep-1995 (shero03)
**	    Count the mutexes by thread.
**	27-sep-1995 (wonst02)
**	    Add an <invalid> thread type.
**	08-feb-1996 (canor01)
**	    Add LGEVENT_MASK and LKEVENT_MASK and explicit LOG_MASK.  LOG_MASK
**	    is no longer the default.
**	03-jun-1996 (canor01)
**	    Changed HANDLE to CS_SYNCH for Unix.
**	16-sep-1996 (canor01)
**	    Add mutex id to MUTEXES structure.
**	18-sep-1996 (canor01)
**	    Add space in array for REP_QMAN thread.  Add defines for thread
**	    type.
**	24-sep-1996 (canor01)
**	    Increase number of mutexes tracked.
**	21-Nov-1996 (jenjo02)
**	    Increased MAXTHREADS to include LKCALLBACK, LKDEADLOCK.
**	17-Dec-1996 (wonst02)
**	    Add LOCKS structure to report locking information.
**	24-jan-1997 (wonst02)
**	    Add numtlsslots, numtlsprobes, et al, to sampler block.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file and moved all calls to CSMT...
**	09-apr-1997 (kosma01)
**	    MAXTHREADS is defined in sys/thread.h as 512 per process.
**	    Undefined MAXTHREADS so a new value, more meaningful to 
**	    the CS, can be set.
**	11-Mar-1998 (jenjo02)
**	    MAXTHREADS changed to MAXSAMPTHREADS to avoid conflict with
**	    sys/thread.h. Added support for Factotum threads.
**	16-Nov-1998 (jenjo02)
**	    MAXEVENTS upped to 10 to account for new event masks. Added
**	    CS_TMSTAT to sampler block to hold wait time stats.
**
**	04-Apr-2000 (jenjo02)
**	    Added getrusage stats to sampler, when available.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Break out event wait counts by thread type, keep track
**	    of I/O and transaction rates.
**      04-nov-2010 (joea)
**          Add prototype for CSMT_sampler.
******************************************************************************/
# define MAXCONDS	17
# define MAXEVENTS	10	/* DIO_MASK(r/w), BIO_MASK(r/w), LIO_MASK(r/w),
				** LOCK_MASK, LOG_MASK,
				** LGEVENT_MASK, LKEVENT_MASK */

# define MAXMUTEXES	1409    /* Prime number for mutex hash table */
# define MAXLOCKS	701	/* Prime number for lock hash table */

# define MAXSTATES	8   	/* FREE, COMPUTABLE, EVENT_WAIT, MUTEX,
				** STACK_WAIT, UWAIT, CNDWAIT, <invalid> */

# define MAXFACS	17	/* <None>,CLF,ADF,DMF,
				** OPF,PSF,QEF,QSF,
				** RDF,SCF,ULF,DUF,
				** GCF,RQF,TPF,GWF,SXF */

/* 
** Defines (as needed) for cs_thread_type, to match definitions in
** scs.h.
*/
#define                 CS_NORMAL         0       /* Normal user session */
#define                 CS_MONITOR        1       /* Monitor session */
#define                 CS_FAST_COMMIT    2       /* Fast Commit Thread */
#define                 CS_WRITE_BEHIND   3       /* Write Behind Thread */
#define                 CS_SERVER_INIT    4       /* Server initiator     */
#define                 CS_EVENT          5       /* EVENT service thread */
#define                 CS_2PC            6       /* 2PC recovery thread */
#define                 CS_RECOVERY       7       /* Recovery Thread */
#define                 CS_LOGWRITER      8       /* logfile writer thread */
#define                 CS_CHECK_DEAD     9       /* Check-dead thread */
#define                 CS_GROUP_COMMIT   10      /* group commit thread */
#define                 CS_FORCE_ABORT    11      /* force abort thread */
#define                 CS_AUDIT          12      /* audit thread */
#define                 CS_CP_TIMER       13      /* consistency pt timer */
#define                 CS_CHECK_TERM     14      /* terminator thread */
#define                 CS_SECAUD_WRITER  15      /* security audit writer */
#define                 CS_WRITE_ALONG    16      /* Write Along thread */
#define                 CS_READ_AHEAD     17      /* Read Ahead thread */
#define                 CS_REP_QMAN       18      /* replicator queue man */
#define                 CS_LKCALLBACK     19      /* LK callback thread */
#define                 CS_LKDEADLOCK     20      /* LK deadlock thread */
#define                 CS_SAMPLER        21      /* Sampler thread */
#define                 CS_SORT           22      /* Sort thread */
#define                 CS_FACTOTUM       23      /* Factotum thread */
#define			CS_LICENSE	  24	  /* License thread */

			    /* The [-1] thread is CS_INTRNL_THREAD */
# define MAXSAMPTHREADS	26  /* NORMAL, MONITOR, FAST_COMMIT, WRITE_BEHIND,
			    ** SERVER_INIT, EVENT, 2PC, RECOVERY,
			    ** LOGWRITER, CHECK_DEAD, GROUP_COMMIT, FORCE_ABORT,
			    ** AUDIT, CP_TIMER, CHECK_TERM, SECAUD_WRITER,
			    ** WRITE_ALONG, READ_AHEAD, REP_QMAN, LKCALLBACK, 
			    ** LKDEADLOCK, SAMPLER, SORT, FACTOTUM,
			    ** LICENSE, <invalid> */


typedef struct _LOCKS {
    LK_LOCK_KEY	lk_key;			/* lock key (from lk.h) */
    u_i4	count_intrnl;		/* Count for the admin thread */
    u_i4	count[MAXSAMPTHREADS];	/* Count for the other threads */
    } LOCKS;

typedef struct _MUTEXES {
    char	name[CS_SEM_NAME_LEN+1]; /* semaphore name */
    char        *id;                    /* address of mutex ("show mutex") */
    u_i4	count_intrnl;		/* Count for the admin thread */
    u_i4	count[MAXSAMPTHREADS];	/* Count for the other threads */
    } MUTEXES;

typedef struct _THREADS {
    u_i4	numthreadsamples;	/* Number of thread samples */
    u_i4	state[MAXSTATES];	/* Current state of thread */
    u_i4	evwait[5];		/* BIO,DIO,LOGIO,LOG,LOCK event wait */
#define	EV_BIO	0
#define	EV_DIO	1
#define	EV_LIO	2
#define	EV_LOG	3
#define	EV_LOCK	4
    u_i4	facility[MAXFACS];	/* Current facility for thread */
    } THREADS;

typedef struct _CSSAMPLERBLK {
    SYSTIME	starttime;		/* Time of day the sampler started */
    SYSTIME	stoptime;		/* Time of day the sampler stopped */
    u_i4	interval;		/* Sleep time in milliseconds (msec.) */
    u_i4	numsamples;		/* Number of times the sampler sampled*/
    u_i4	totusersamples;		/* Number of "user" thread samples */
    u_i4	totsyssamples;		/* Number of "system" thread samples */

    u_i4	startcpu;		/* Server CPU when sampler started */
    struct	_CS_TMSTAT waits;	/* Wait counts/times when sample started */

    u_i4	bior[2];		/* I/O, Transaction rates */
    u_i4	biow[2];
    u_i4	dior[2];
    u_i4	diork[2];
    u_i4	diow[2];
    u_i4	diowk[2];
    u_i4	lior[2];
    u_i4	liork[2];
    u_i4	liow[2];
    u_i4	liowk[2];
    u_i4	txn[2];
#define  CURR	0
#define  PEAK	1

    u_i4	numtlssamples;		/* Num of CS tls hash table samples */
    u_i4	numtlsslots; 		/* Num of slots in use in tls hash tbl*/
    u_i4	numtlsprobes;		/* Num of probes needed to find entry */
    i4		numtlsreads;		/* Num of reads */
    i4		numtlsdirty;		/* Num of "dirty" reads to find entry */
    i4		numtlswrites;		/* Num of writes */

    u_i4	numusereventsamples;	/* Number of user event samples */
    u_i4	userevent[MAXEVENTS+1];	/* The event types of user threads */

    u_i4	numsyseventsamples;	/* Number of system event samples */
    u_i4	sysevent[MAXEVENTS+1];	/* The event types of system threads */

    u_i4	numcondsamples;		/* Number of cond samples */
    u_i4	cond[MAXCONDS];		/* The cond types */

    u_i4	numlocksamples;		/* Number of lock samples */
    LOCKS	Lock[MAXLOCKS+1];	/* The lock hash table */

    u_i4	nummutexsamples;	/* Number of mutex samples */
    MUTEXES	Mutex[MAXMUTEXES+1];	/* The mutex hash table */
    /*
    ** Note: Save space for the -1 thread type ahead of the Thread[] array. This
    ** allows simpler, faster code in the sampler, because its overhead must be
    ** kept to a minimum.
    */
    THREADS	Thread_Intrnl[1];	/* The [-1] thread is CS_INTRNL_THREAD*/
    THREADS	Thread[MAXSAMPTHREADS];	/* [0]...[MAXSAMPTHREADS] are other types */

#ifdef xCL_003_GETRUSAGE_EXISTS
    struct rusage ruse;
#endif /* xCL_003_GETRUSAGE_EXISTS */

    } CSSAMPLERBLK, *CSSAMPLERBLKPTR;

/* Routines to Lock the Sampler Block */

STATUS LockSamplerBlk(CS_SYNCH *hpLockSem);

STATUS UnlockSamplerBlk(CS_SYNCH *hpLockSem);

FUNC_EXTERN void CSMT_sampler(void);

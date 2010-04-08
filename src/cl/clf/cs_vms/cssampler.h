/******************************************************************************
**
** Copyright (c) 1995, 2008 Ingres Corporation
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
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	14-oct-2008 (joea)
**	    Integrate 16-nov-1998 change from unix version.  MAX_EVENTS
**	    increased to 10.  Use CM_TMSTAT for waits.
**
******************************************************************************/
# define MAXCONDS	17
# define MAXEVENTS	10	/* DIO_MASK(r/w), BIO_MASK(r/w), LIO_MASK(r/w),
				** LOCK_MASK, LOG_MASK,
				** LGEVENT_MASK, LKEVENT_MASK */

# define MAXMUTEXES	40111    /* Prime number for hash table */

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

			    /* The [-1] thread is CS_INTRNL_THREAD */
# define MAXTHREADS	22  /* NORMAL, MONITOR, FAST_COMMIT, WRITE_BEHIND,
			    ** SERVER_INIT, EVENT, 2PC, RECOVERY,
			    ** LOGWRITER, CHECK_DEAD, GROUP_COMMIT, FORCE_ABORT,
			    ** AUDIT, CP_TIMER, CHECK_TERM, SECAUD_WRITER,
			    ** WRITE_ALONG, READ_AHEAD, REP_QMAN, 
			    ** LKCALLBACK, LKDEADLOCK, SAMPLER, <invalid> */


typedef struct _MUTEXES {
    char	name[CS_SEM_NAME_LEN+1]; /* semaphore name */
    char        *id;                    /* address of mutex ("show mutex") */
    i4	count_intrnl;		/* Count for the admin thread */
    i4	count[MAXTHREADS];	/* Count for the other threads */
    } MUTEXES;

typedef struct _THREADS {
    i4	numthreadsamples;	/* Number of thread samples */
    i4	state[MAXSTATES];	/* Current state of thread */
    i4	facility[MAXFACS];	/* Current facility for thread */
    } THREADS;

typedef struct _CSSAMPLERBLK {
    SYSTIME	starttime;		/* Time of day the sampler started */
    SYSTIME	stoptime;		/* Time of day the sampler stopped */
    i4	interval;		/* Sleep time in milliseconds (msec.) */
    i4	numsamples;		/* Number of times the sampler sampled*/

    struct _CS_TMSTAT waits;	/* Wait counts/times when sample started */
    i4	numusereventsamples;	/* Number of user event samples */
    i4	userevent[MAXEVENTS+1];	/* The event types of user threads */

    i4	numsyseventsamples;	/* Number of system event samples */
    i4	sysevent[MAXEVENTS+1];	/* The event types of system threads */

    i4	numcondsamples;		/* Number of cond samples */
    i4	cond[MAXCONDS];		/* The cond types */

    i4	nummutexsamples;	/* Number of mutex samples */
    MUTEXES	Mutex[MAXMUTEXES+1];	/* The mutex hash table */
    /*
    ** Note: Save space for the -1 thread type ahead of the Thread[] array. This
    ** allows simpler, faster code in the sampler, because its overhead must be
    ** kept to a minimum.
    */
    THREADS	Thread_Intrnl[1];	/* The [-1] thread is CS_INTRNL_THREAD*/
    THREADS	Thread[MAXTHREADS];	/* [0]...[MAXTHREADS] are other types */

    } CSSAMPLERBLK, *CSSAMPLERBLKPTR;

/* Routines to Lock the Sampler Block */

# ifdef OS_THREADS_USED
STATUS LockSamplerBlk(CS_SYNCH *hpLockSem);

STATUS UnlockSamplerBlk(CS_SYNCH *hpLockSem);
# endif /* OS_THREADS_USED */

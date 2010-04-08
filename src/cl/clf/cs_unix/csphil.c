/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** csphil.c -- Dining philosophers test of CS

This program simulates the well known "Dining Philosphers" problem (see
for instance, CACM Jan 1988).  It is used to test and demonstrate the
thread and synchronization primitives provided by the INGRES CS facility.

There are N philosphers who spend their lives eating or thinking.  Each
philospher has his own place at a circular table, in the center of which
is a large bowl of rice.  To eat rice requires two chopsticks, but only
N chopsticks are provided, one between each pair of philosphers.  The
only forks a philospher can pick up are those on his immediate right and
left.  Each philospher is identical in structure, alternately eating
then thinking.  The problem is to simulate the behaviour of the
philosphers while avoiding deadlock (the request by a philosopher for a
chopstick can never be granted) and indefinate postponement (the request
by a philospher is continually denied), known colloquially as "starvation."

	"Though the problem of the Dining philosphers appears to have
	greater entertainment value than practical importance, it has
	had huge theoretical influence.  The problem allows the classic
	pitfalls of concurrent programming to be demonstrated in a
	graphical situation.  It is a benchmark of the expressive power
	of new primitives of concurrent programming and stands as a
	challenge to proposers of new languages"
			- G.A. Ringword, CACM Jan 1988.

History:
	8/30/88 (daveb) -- created, with assistance from Jeff Anton.
	8/31/88 (daveb) -- As noted by Fred Carter, SI calls should
			   be protected with a semaphore too.
	10/28/88 (arana) -- Added ming hints for PROGRAM and MALLOCLIB
	05-jul-89 (russ)
		Adding a couple more stubs so that csphil will compile on
		machines that are picky about multiply defined functions.
	14-jul-89 (seng)
		AIX on the PS2 is picky about naming variables and functions
		the same.  Renamed newscb() to newscbf().
	08-aug-89 (seng)
		Adding test to see if System V exceptions will create problems
		for multi-threaded processes.  Compile this in by defining
		EX_DEBUG.
        15-aug-89 (fls-sequent) -- Made following changes for MCT:
            Added calls to CSi_semaphore to intialize semaphores.
            Modified semerr routine to reflect new CS_SEMAPHORE definition.
            Protected semerr SI calls with SIsem.
            Modified CS_map_sys_segment stub to allocate Cs_svinfo structure.
            Removed CVan stub, the real routine is needed.
        16-oct-89 (fls-sequent) -- Added cs_mem_sem support for MCT (from rexl).
	28-Nov-89 (pholman)
		Ensure that functions are defined VOID and not void, so that
		VOID may be redefined in compat.h
	09-feb-90 (swm)
		Added <clconfig.h> include to pickup correct ifdefs in
		<csev.h>.
		Changed include <signal.h> to <clsigs.h>.
	24-apr-90 (kimman)
		Integrating in ingresug line of csphil so that it will 
		load on machines that are picky about multiply defined,
		as per changes made by russ on 05-jul-89.
	4-june-90 (blaise)
		Integrated changes from 61 and ingresug:
		Add setdtablesize call for Sequent.
	29-sep-92 (daveb)
		Straighten out includes so that it will compile again.
		It wasn't including <pc.h> or <csinternal.h>.
	27-oct-92 (daveb)
		Eliminate stubs to avoid duplicate symbol errors.
		This means we'll link in the whole CS universe,
		but what the heck.  Also TRset_file so any underlying
		log messages are visible, and correct call to CS
		routines revealed with prototypes.
	05-Nov-1992 (mikem)
		su4_us5 port.  Changed direct calls to CL_NFILE to calls to new
		function iiCL_get_fd_table_size().  Also pulled ifdef's for
		increasing file descriptor number out and put actual work into
		iiCL_increase_fd_table_size().
	17-Feb-1993 (smc)
		Removed troublesome extern declaration of MEreqmem()
		for prototyped builds, the correct extern declaration
		is now inherited from <me.h>.
	27-apr-1993 (sweeney)
		Now that we use the real CS routines and not stub code,
		it is necessary to run csinstall before csphil will run.
		This breaks the 'cltest' script. This change makes csphil
		idempotent once more.
	30-jun-1993(shailaja)
		Cast to correct prototype mismatch.
	17-aug-1993 (tomm)
		Must include fdset.h since protypes being included reference
		fdset.
	08-sep-93 (swm)
		Changed sid type from i4  to CS_SID in rwnull() to match
		recent CL interface revision.
	1-nov-93 (peterk)
		On hp8_us5 pc.h needs clconfig.h to define 
		xCL_069_UNISTD_H_EXISTS to get proper declaration of getpid()
		so move clconfig.h up in include list
	06-oct-1993 (tad)
		Bug #56449
		Changed %x to %p for pointer values in semerr().
	20-apr-1994 (mikem)
		Bug #59728
		A change to CS caused it to check PM error returns and fail
		to initiate if such errors were detected.  The csphil program
		has been changed to properly initialize the PM system (by
		calling PMload() and PMinit().  Prior to this change csphil
		would fail to start up.
	6-aug-94 (nick)  Cross Integ from 6.4 (ramra01) 8-dec-94
                Definitions of LEFT() and RIGHT() incorrect when running
                less than N philosophers. #65042
	13-Aug-96 (jenjo02)
                Removed reference to CS_SEMAPHORE cs_owner, cs_next, to
                be compatible with threads version.
        19-May-97  (merja01)
		Changed Threads to Start_Threads and End_Threads.  Using POSIX
		threads on axposf the Threads counter would get decremented
		before another thread was started.  Also, added a 1 second
		sleep to the think function for axp_osf running threads.     
		Without the sleep, philosphers run to quickly and do not
		contend for resources.
	29-may-1997 (canor01)
		TRset_file() takes a CL_ERR_DESC*.
        29_aug_97 (musro02)
                In mecl.h, the definition ME_INGRES_SHMEM_ALLOC has changed to
                ME_INGRES_THREAD_ALLOC.  Made that change here
                Removed sqs_ptx from special sqs_us5 processing.
	31-Mar-1998 (jenjo02)
		Added *thread_id parm to CSadd_thread() prototype.
	08-May-1998 (merja01)
		Added initializations of cs_get_rcp_pid, cs_scbattach,
		and cs_scbdetach to correct segmentation violations
		on axp_osf. Also, changed End_Threads to MAX_N instead
		of using hard-coded value of 12.
	12-Nov-1998 (popri01)
		Added initialization of cs_stkcache.
        30-Nov-1998(merja01)
                Fix warning msg by changing cs_scbattach and cs_scbdetach
                from fnull to vnull.
	11-jan-1999 (popri01)
		Allow for an extra wakeup block required by Jon Jensen's
		idlethread alternative in csmtinterf.c (change #438932).
	10-may-1999 (walro03)
		Remove obsolete version string sqs_us5.
	29-feb-2000 (toumi01)
		Added int_lnx to platforms that must sleep 1 second else
		the philosophers do not contend for resources.
        11-may-1999 (hweho01)
                Included stdlib.h header file for ris_u64 platform, because it
                contains the prototype definition of *calloc().  Without the
                explicit declaration, the default int return value for a 
                function will truncate a 64-bit address. 
	15-Nov-1999 (jenjo02)
		Replaced CS_SEMAPHORE cs_sid with cs_thread_id for OS threads.
	15-aug-2000 (somsa01)
		Added ibm_lnx to platforms that must sleep 1 second else
		the philosophers do not contend for resources.
	25-jul-2001 (toumi01)
		Added i64_aix to platforms that must sleep 1 second else
		the philosophers do not contend for resources.
	03-Dec-2001 (hanje04)
	   	Added IA64 Linux to platforms needing sleeping philosophers.
        25-Sep-2002 (hweho01)
                Included stdlib.h header file for rs4_us5 platform.
	03-oct-2002 (devjo01)
	    Set NUMA context.
	08-Oct-2002 (hanje04)
		As part of AMD x86-64 Linux port replace individual Linux 
		defines with a generic LNX define where apropriate.
	21-Mar-2006 (kschendel)
		Do more checking that we've actually done the right thing.
	21-Jun-2006 (bonro01)
		Replace non-portable functions from last change.

PROGRAM  = csphil

MODE = SETUID

NEEDLIBS = COMPAT MALLOCLIB
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jan-2007 (wonca01) BUG 117262
**          Change signature of logger() to reflect the change in CS usage
**          from STATUS* to CL_ERR_DESC*.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# include <compat.h>
# include <gl.h>
# include <clconfig.h>
# include <pc.h>
# include <cs.h>
# include <cx.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <tr.h>
# include <lo.h>
# include <pm.h>

# include <clnfile.h>
# include <fdset.h>
# include <csev.h>
# include <cssminfo.h>
# include <clsigs.h>
# include <csinternal.h>

# include "cslocal.h"

#if defined(any_aix)
# include <stdlib.h>
#endif

GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF       CS_SERV_INFO    *Cs_svinfo;
GLOBALDEF       CS_SEMAPHORE    sc0m_semaphore;

# define N	12		/* number of threads */
# define MAX_N	12		/* max number of philosophers */
# define NBITES	100		/* bites and we are done. */

/* Init to non-zero for verbose version */

i4  Noisy = 1;

/* Gag me.  This is the only place we can keep the type passed
   in by CSadd_thread for 6.1 CS, so we create a whole new type. */

typedef struct
{
	CS_SCB	cs_scb;		/* the "semi-opaque" system scb */
	i4	phil;		/* my data, filled in by newscb( ... type ) */
} MY_SCB;

/* System segment was allocated by me */

bool sys=TRUE;

/* Some concrete names for the ephemeral */

char *Names[MAX_N] =
{
    "Aquinas",
    "Aristotle",
    "Camus",
    "Descartes",
    "Goethe",
    "Kant",
    "Nietzsche",
    "Plato",
    "Rousseu",
    "Sartre",
    "Spinoza",
    "Wittgenstein"
};

/* status of thread (philosopher) */
i4	status[MAX_N];

/* number of threads wanted */
i4	n_phils;

/* Total amount eaten */
volatile i4 n_eaten = 0;

/* Count of the number of "living" philosophers */

i4  Start_Threads = 0;
i4  End_Threads = MAX_N;

/* Protects SI calls */

CS_SEMAPHORE SIsem;

/* Protects Freesticks during updates from access by multiple threads */

CS_SEMAPHORE Freesem;

/* Protects n_eaten during updates from access by multiple threads */

#if !defined(conf_CAS_ENABLED)
CS_SEMAPHORE Eatensem;
#endif

/* Count of sticks available RIGHT and LEFT of each philosopher */

i4  Freesticks[ N ];

/* Array indeces of neighbors in Freesticks */

# define LEFT( n )      ( n ? n - 1 : n_phils - 1 )
# define RIGHT( n )     ( n == (n_phils - 1) ? 0 : n + 1 )

/* function forward refs */

VOID logger();
char *maperr();

/* ---------------- actual application ---------------- */

# ifdef EX_DEBUG
intfunc()
{
	SIfprintf(stderr, "Signal #%d received, continue processing\n", 
		SIGUSR1);
}
# endif /* EX_DEBUG */

/* get sticks for philosopher n */

getsticks( n )
i4  n;
{
    /*
    ** While you can't get both sticks, give control to others who
    ** might eventually free the ones you need
    */
    for( ;; )
    {
	Psem( &Freesem );
	if( Freesticks[ n ] == 2 )
	    break;
	Vsem( &Freesem );
	if (!CS_is_mt())
	    CS_swuser();
    }

    /* ASSERT:	phil n holds Freesem and Freesticks[ n ] == 2. */

    Freesticks[ n ] -= 2;
    --Freesticks[ LEFT( n ) ];
    --Freesticks[ RIGHT( n ) ];

    if (!CS_is_mt())
        CS_swuser();	    /* gratuitous; tests semaphores */
    else
    {
	PCsleep(1);
    }

    Vsem( &Freesem );
}

/* release sticks held by philosopher n */

freesticks( n )
i4  n;
{
    Psem( &Freesem );

    Freesticks[ n ] += 2;
    ++Freesticks[ LEFT( n ) ];
    ++Freesticks[ RIGHT( n ) ];

    if (!CS_is_mt())
        CS_swuser();	    /* gratuitous, tests semaphores */
    else
    {
	PCsleep(1);
    }

    Vsem( &Freesem );
}

/* A philsopher thread function, called out of CSdispatch */

philosopher( mode, scb, next_mode )
i4  mode;
MY_SCB *scb;
i4  *next_mode;
{
    i4  bites = 0;

    switch( mode )
    {
    case CS_INITIATE:	/* A new philsopher is born */

	scb->phil = Start_Threads++;
	status[scb->phil] = mode;
	while( bites < NBITES )
	{
	    getsticks( scb->phil );
	    eats( scb->phil );
	    bites++;
	    freesticks( scb->phil );
	    thinks( scb->phil );
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	break;

    case CS_TERMINATE:
	if( Noisy )
	{
# ifdef EX_DEBUG
	    signal(SIGUSR1, intfunc);
	    if (scb->phil == 1) {
		SIfprintf(stderr, "Send a signal #%d to process #%d\n",
			SIGUSR1, getpid());
		pause();
	    }
# endif /* EX_DEBUG */
	    Psem( &SIsem );
	    SIfprintf(stderr, "%d (%s) dies, RIP.\n",
			scb->phil, Names[ scb->phil ] );
	    if ( status[scb->phil]  == mode )
	    {
		SIfprintf(stderr, "Oops this philosopher is already dead?\n");
		SIfprintf(stderr, "\t\t CS code is non-functional\n");
	    }
	    Vsem( &SIsem );
	}

	*next_mode = CS_TERMINATE;

	/* If no more threads, shutdown */
	status[scb->phil] = mode;

	if( --End_Threads == 0 )
    		CSterminate( CS_KILL, (i4 *)NULL );

	break;
    }
    return( OK );
}

/* Observe that philosopher n is eating */

eats( n )
i4  n;
{
    i4 on, nn;

    if( Noisy )
    {
	Psem( &SIsem );
	SIfprintf(stderr, "%d (%s) eats...\n", n, Names[ n ] );
	Vsem( &SIsem );
    }
#if defined(conf_CAS_ENABLED)
    do {
	on = n_eaten;
	nn = on + 1;
    } while (CScas4(&n_eaten, on, nn) != 0);
#else
	Psem( &Eatensem );
	n_eaten += 1;
	Vsem( &Eatensem );
#endif
    if (!CS_is_mt())
        CS_swuser();
    else
    {
	PCsleep(1);
    }

}

/* Observe that philosopher n is thinking */

thinks( n )
i4  n;
{
    if( Noisy )
    {
	Psem( &SIsem );
	SIfprintf(stderr, "%d (%s) thinks...\n", n, Names[ n ] );
	Vsem( &SIsem );
    }
#if defined(POSIX_THREADS)
#if defined(axp_osf) || defined(LNX)
    sleep(1);
#endif /* axp_osf || Linux */
#endif /* POSIX_THREADS */
    if (!CS_is_mt())
        CS_swuser();
}

/*
**	Setup global variables and add threads for each philosopher.
**	Called by CSinitialize before threads are started
*/
hello( csib )
CS_INFO_CB *csib;
{
    i4  i;
    STATUS stat;
    CL_SYS_ERR	err;

    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);

    /* MCTFIX - don't wipeout pplib's handler
    signal(SIGSEGV, SIG_DFL);
    */
    signal(SIGBUS, SIG_DFL);
 
    CSi_semaphore(&SIsem, CS_SEM_SINGLE);
    CSi_semaphore(&Freesem, CS_SEM_SINGLE);
#if !defined(conf_CAS_ENABLED)
    CSi_semaphore(&Eatensem, CS_SEM_SINGLE);
#endif

    for( i = 0, stat = OK; i < n_phils && stat == OK; i++ )
    {
	Freesticks[ i ] = 2;
	stat = CSadd_thread( 0, (PTR)NULL, i + 1, (CS_SID*)NULL, &err );
    }

    /* no semaphore needed, called before CSdispatch */
    if( Noisy )
	SIfprintf(stderr, "World begins, philosophers wonder why.\n" );

    CSi_semaphore(&sc0m_semaphore, CS_SEM_SINGLE);
 
    csib->cs_mem_sem = &sc0m_semaphore;

    return( stat );
}


/* Called when all thread functions return from the TERMINATE state */

bye()
{
    /* semaphore needed, called when all threads are dead */
    if( Noisy )
	SIfprintf(stderr, "All philosophers are dead.  World ends.\n");

    if (n_eaten != n_phils * NBITES)
	SIfprintf(stderr, "\nERROR: expected %d bites eaten, actual was %d\n",
		n_phils * NBITES, n_eaten);

    /* if we allocated the sytem segment, clear up now. */
    if( sys )
	CS_des_installation();

    return( OK );
}


/* ---------------- book keeping ---------------- */


/* request and wait for semaphore */

Psem( sp )
CS_SEMAPHORE *sp;
{
    i4  rv;

    if( rv = CSp_semaphore( TRUE, sp ) )
	semerr( rv, sp, "Psem" );
    return( rv );
}

/* release a semaphore */

Vsem( sp )
CS_SEMAPHORE *sp;
{
    i4  rv;

    if( rv = CSv_semaphore( sp ) )
	semerr( rv, sp, "Vsem" );
    return( rv );
}

/* decode semaphore error */

semerr( rv, sp, msg )
i4  rv;
CS_SEMAPHORE *sp;
char *msg;
{
    MY_SCB *scb;

    CSget_scb( (CS_SCB **)&scb );
    Psem( &SIsem );
    SIfprintf(stderr, "%s %p returned %d (%s)\n", msg, sp, rv, maperr(rv) );
    SIfprintf(stderr, "\tPhilosopher %d, scb %p\n", scb->phil, scb );
    SIfprintf(stderr, "\t%p->cs_value %d\n", sp, sp->cs_value );
    SIfprintf(stderr, "\t%p->cs_count %d\n", sp, sp->cs_count );
    SIfprintf(stderr, "\t%p->cs_owner %x\n", sp, sp->cs_owner );
    SIfprintf(stderr, "\t%p->cs_next %p\n", sp, sp->cs_next );
# ifdef OS_THREADS_USED
    SIfprintf(stderr, "\t%p->cs_thread_id %d\n", sp, sp->cs_thread_id );
    SIfprintf(stderr, "\t%p->cs_pid %d\n", sp, sp->cs_pid );
# endif /* OS_THREADS_USED */
    Vsem( &SIsem );
}

/* If we get semaphore errors, do simple decoding */

char *
maperr( rv )
i4  rv;
{
    switch( rv )
    {
    case E_CS0017_SMPR_DEADLOCK:
	return "E_CS0017_SMPR_DEADLOCK";

    case E_CS000F_REQUEST_ABORTED:
	return "E_CS000F_REQUEST_ABORTED";

    case E_CS000A_NO_SEMAPHORE:
        return "E_CS000A_NO_SEMAPHORE";

    case OK:
	return "";
    }
}

/* log function, called as the output when needed */

VOID
logger( errnum, arg1, arg2 )
i4  errnum;
CL_ERR_DESC* arg1;
i4  arg2;
{
    char buf[ ER_MAX_LEN ];

    if( ERreport( errnum, buf ) == OK )
    	SIfprintf(stderr, "%s\n", buf);
    else
	SIfprintf(stderr, "ERROR %d (%x), %s %d\n", 
		errnum, errnum, arg1, arg2 );
    if(errnum != E_CS0018_NORMAL_SHUTDOWN)
	PCexit(FAIL);
    PCexit(OK);
}

/*
** Make a new scb, ignoring crb and saving type. Called by CSadd_thread
**
** 'type', passed by 6.1 CS, is ignored here for compatibility with 6.0 CS.
*/
newscbf( newscb, crb, type )
MY_SCB **newscb;
PTR crb;
i4  type;
{
    /* god this is ugly, making us store type here... */

    *newscb = (MY_SCB*) calloc( 1, sizeof( **newscb ) );
    return( NULL == *newscb );
}

/* release an scb */

freescb( oldscb )
MY_SCB *oldscb;
{
    free( oldscb );
}

/* Function used when CS_CB insists on having one and we don't care */

fnull()
{
    return( OK );
}

VOID
vnull()
{
}

/* Function needed for read and write stub */

rwnull( scb, sync )
MY_SCB *scb;
i4  sync;
{
    CS_SID sid;

    CSget_sid( &sid );
    CSresume( sid );
    return( OK );
}


/* Set up philosopher threads and run them.  No args accepted. */

main(argc, argv)
i4	argc;
char	*argv[];
{
    CS_CB 	cb;
    i4  	rv;
    i4  	err_code;
    i4  	f_argc;
    char 	**f_argv,*f_arr[3];
    CL_ERR_DESC syserr;
    STATUS	status;

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    /* test how many philosopher wanted */
    if(argc == 1)
    {
	n_phils = N;
	(void) iiCL_increase_fd_table_size(FALSE, n_phils + CS_RESERVED_FDS);
        if( (iiCL_get_fd_table_size() - CS_RESERVED_FDS) < n_phils  )
                n_phils = iiCL_get_fd_table_size() - CS_RESERVED_FDS;
    }
    else if(argc == 2)
    {
	n_phils = atoi(argv[1]);
	if(n_phils > N || n_phils < 1)
		n_phils = N;
	(void) iiCL_increase_fd_table_size(FALSE, N + CS_RESERVED_FDS);
    }
    else
    {
	SIfprintf(stderr, "usage: %s [num_threads]\n", argv[0]);
	PCexit(FAIL);
    }

    MEadvise( ME_INGRES_ALLOC );

    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &syserr);

    PMinit();

    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);

    /* default some signals */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);

    /* setup big CS parameter block to set stuff up */
    cb.cs_scnt = n_phils;	/* Number of sessions */
    cb.cs_ascnt = 0;		/* nbr of active sessions */
    cb.cs_stksize = (8 * 1024);	/* size of stack in bytes */
    cb.cs_scballoc = newscbf;	/* Routine to allocate SCB's */
    cb.cs_scbdealloc = freescb;	/* Routine to dealloc  SCB's */
    cb.cs_saddr = fnull;	/* Routine to await session requests */
    cb.cs_reject = fnull;	/* how to reject connections */
    cb.cs_disconnect = vnull;	/* how to dis- connections */
    cb.cs_read = rwnull;	/* Routine to do reads */
    cb.cs_write = rwnull;	/* Routine to do writes */
    cb.cs_process = philosopher; /* Routine to do major processing */
    cb.cs_attn = fnull;		/* Routine to process attn calls */
    cb.cs_elog = logger;	/* Routine to log errors */
    cb.cs_startup = hello;	/* startup the server */
    cb.cs_shutdown = bye;	/* shutdown the server */
    cb.cs_format = fnull;	/* format scb's */
    cb.cs_get_rcp_pid = fnull;   /* init to fix SEGV in CSMTp_semaphore*/
    cb.cs_scbattach = vnull;     /* init to fix SEGV in CSMT_setup */
    cb.cs_scbdetach = vnull;     /* init to fix SEGV in CSMT_setup */
    cb.cs_stkcache = FALSE;     /* Match csoptions default - no stack caching */
    cb.cs_format_lkkey = NULL;  /* Not used in this context. */

    /* now fudge argc and argv for CSinitiate */
    f_argc = 2;
    f_arr[0] = argv[0];
    f_arr[1] = "-nopublic";
    f_arr[2] = NULL;
    f_argv = f_arr;

# ifdef OS_THREADS_USED
    /* Additional wakeup block needed by idle thread replacement */
    if(CS_create_sys_segment(1, n_phils + 1, &syserr) != OK)
# else
    if(CS_create_sys_segment(1, n_phils, &syserr) != OK)
# endif /* OS_THREADS_USED */
    {
    SIprintf("Warning: couldn't create system segment.\n");
    sys = FALSE;
    }

    if( rv = CSinitiate( &f_argc, &f_argv, &cb ) )
    {
	SIfprintf(stderr, "CSinitiate returns %d\n", rv );
	PCexit(FAIL);
    }

    /* now do everything that has been set up */
    rv = CSdispatch();

    /* should not get here */
    SIfprintf(stderr, "CSdispatch returns %d (%x)\n", rv, rv );

    PCexit( FAIL );
}

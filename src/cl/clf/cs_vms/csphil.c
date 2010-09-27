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
		Changed sid type from nat to CS_SID in rwnull() to match
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
	12-oct-95 (duursma) AXP VMS port.
	29-nov-95 (dougb) more on bug 71590
	    CS_ssprsm() now takes a single parameter -- just restore context.
	    Use one output channel -- stderr -- only and sem-protect all calls
	    to SIfprintf() to write things in the correct order.  Also, ensure
	    the EXC code doesn't cause multiple executions of the same routine.
	09-sep-97 (kinte01)
	    Cross-integrate change from Unix CL - 430346
	    29-may-1997 (canor01)
	    TRset_file() takes a CL_ERR_DESC*.
        20-may-1998 (kinte01)
            Cross-integrate Unix change 435120
            31-Mar-1998 (jenjo02)
               Added *thread_id output parm to CSadd_thread().
        08-May-1998 (merja01)
	    Added initializations of cs_get_rcp_pid, cs_scbattach,
	    and cs_scbdetach to correct segmentation violations
	    on axp_osf. Also, changed End_Threads to MAX_N instead
	    of using hard-coded value of 12.
 	09-nov-1999 (kinte01)
	   Rename EX_CONTINUE to EX_CONTINUES per AIX 4.3 porting change
	01-dec-2000	(kinte01)
		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
		from VMS CL as the use is no longer allowed
	23-oct-2003 (devjo01)
		Hand cross certain minor 103715 related changes from UNIX CL.
	01-feb-2005 (abbjo03)
	    Do not "play" with exceptions by default.
	29-jun-2009 (joea)
	    Correct EX_CONTINUES to EX_CONTINUE.  Use same constant (EX_DEBUG
	    instead of EXC) and same value for NBITES (100 instead of 3) as
	    on Unix.  Correct function signatures and other tyding up.
        21-jan-2010 (joea)
            Replace CS_ssprsm calls by CSswitch.
        12-aug-2010 (joea)
            Replace VMS exception handling by POSIX signals as done on Unix.
            Disable EX_DEBUG in hello as it causes nasty side effect in
            KPS code.

PROGRAM  = csphil

MODE = SETUID

NEEDLIBS = COMPAT MALLOCLIB
*/

/* 
** EX_DEBUG controls whether we play with exceptions.  The original dining
** philosophers program does not.  If exceptions are desired uncomment
** the following define.
*/

/* # define EX_DEBUG */


# include <compat.h>
# include <gl.h>
# include <clconfig.h>
# include <pc.h>
# include <cs.h>
# include <cx.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <si.h>
# include <tr.h>
# include <lo.h>
# include <pm.h>
#include <vmsclient.h>
# include <clsigs.h>
# include <exhdef.h>
# include <csinternal.h>
#include <stdlib.h>	/* for calloc and free */
#include <unistd.h>	/* for getpid */


FUNC_EXTERN GCsetMode(i2 mode);


# define N	12		/* number of threads */
# define MAX_N	12		/* max number of philosophers */
# define NBITES	100		/* bites and we are done. */

/* Init to non-zero for verbose version */

i4 Noisy = 1;

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
    "Rousseau",
    "Sartre",
    "Spinoza",
    "Wittgenstein"
};

/* status of thread (philosopher) */
i4	status[MAX_N];

/* number of threads wanted */
i4	n_phils;

/* Count of the number of "living" philosophers */

i4 Threads = 0;

/* Protects SI calls */

CS_SEMAPHORE SIsem;

/* Protects Freesticks during updates from access by multiple threads */

CS_SEMAPHORE Freesem;

/* Count of sticks available RIGHT and LEFT of each philosopher */

static i4 Freesticks[ N ];

/* Array indeces of neighbors in Freesticks */

# define LEFT( n )      ( n ? n - 1 : n_phils - 1 )
# define RIGHT( n )     ( n == (n_phils - 1) ? 0 : n + 1 )

/* function forward refs */

void eats(i4 n);
void thinks(i4 n);
void logger(i4 errnum, i4 arg1, i4 arg2);
char *maperr(STATUS rv);
void semerr(i4 rv, CS_SEMAPHORE *sp, char *msg);
STATUS Psem(CS_SEMAPHORE *sp);
STATUS Vsem(CS_SEMAPHORE *sp);

#ifdef EX_DEBUG

static EX free_exc    = 0x0aaabbbb; /* just some values to make them unique */
static EX get_exc     = 0x0aaacccc;
static EX not_handled = 0x0aaadddd;

/*
** CONTINUE from a few specific errors.
*/
STATUS
ex_handler(
EX_ARGS *ex_args)
{
    STATUS stat = EX_DECLARE;

    if (ex_args->exarg_num == free_exc) {
	Psem( &SIsem );
	SIfprintf( stderr,"CONT:freesticks(%d)\n", ex_args->exarg_array[0]);
	Vsem( &SIsem );

	stat = EX_CONTINUE;
    }
    if (ex_args->exarg_num == get_exc) {
	Psem( &SIsem );
	SIfprintf( stderr,"CONT:getsticks(%d)\n", ex_args->exarg_array[0]);
	Vsem( &SIsem );

	stat = EX_CONTINUE;
    }

    return stat;
}

/*
** RESIGNAL a few specific errors.
*/
STATUS
ex_handler2(
EX_ARGS *ex_args)
{
    STATUS stat = EX_DECLARE;

    if (ex_args->exarg_num == free_exc) {
	Psem( &SIsem );
	SIfprintf( stderr,"RESIG:freesticks(%d)\n", ex_args->exarg_array[0]);
	Vsem( &SIsem );

	stat = EX_RESIGNAL;
    }
    if (ex_args->exarg_num == get_exc) {
	Psem( &SIsem );
	SIfprintf( stderr,"RESIG:getsticks(%d)\n", ex_args->exarg_array[0]);
	Vsem( &SIsem );

	stat = EX_RESIGNAL;
    }

    return stat;
}
#endif /* EX_DEBUG */

/* ---------------- actual application ---------------- */

# ifdef EX_DEBUG
void
intfunc(int n)
{
	SIfprintf(stderr, "Signal #%d received, continue processing\n", 
		SIGUSR1);
}
# endif /* EX_DEBUG */

/* get sticks for philosopher n */
void
getsticks(
i4 n)
{
#ifdef EX_DEBUG
    EXsignal(get_exc, 1, n);
#endif

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
	CSswitch();
    }

    /* ASSERT:	phil n holds Freesem and Freesticks[ n ] == 2. */

    Freesticks[ n ] -= 2;
    --Freesticks[ LEFT( n ) ];
    --Freesticks[ RIGHT( n ) ];

    CSswitch();	    /* gratuitous; tests semaphores */

    Vsem( &Freesem );
}

/* release sticks held by philosopher n */
void
freesticks(
i4 n)
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler2, &context) != OK) {
	/* free_exc error should never be EX_DECLARE'd. */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in freesticks()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    Psem( &Freesem );

#ifdef EX_DEBUG
    EXsignal(free_exc, 1, n);
#endif

    Freesticks[ n ] += 2;
    ++Freesticks[ LEFT( n ) ];
    ++Freesticks[ RIGHT( n ) ];

    CSswitch();	    /* gratuitous, tests semaphores */

    Vsem( &Freesem );

#ifdef EX_DEBUG
    EXdelete();
#endif
}


/* A philosopher thread function, called out of CSdispatch */
STATUS
philosopher(
i4 mode,
MY_SCB *scb,
i4 *next_mode)
{
    i4 bites = 0;

#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in philosopher()...");
	Vsem( &SIsem );

	EXdelete();
	return FAIL;
    }
#endif

    switch( mode )
    {
    case CS_INITIATE:	/* A new philsopher is born */

	scb->phil = Threads++;
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
# ifdef EX_SIG_DEBUG
	    signal(SIGUSR1, intfunc);
	    if (scb->phil == 1) {
		SIfprintf(stderr, "Send a signal #%d to process #%d\n",
			SIGUSR1, getpid());
		pause();
	    }
# endif /* EX_SIG_DEBUG */
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

	if( --Threads == 0 ) {
	    /* Everyone else should be dead, no semaphore needed. */
	    SIflush( stderr );

	    CSterminate( CS_KILL, (i4 *)NULL );
	}

	break;
    }

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( OK );
}

/* Observe that philosopher n is eating */
void
eats(
i4 n)
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in eats()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif
 
    if( Noisy )
    {
	Psem( &SIsem );
	SIfprintf(stderr, "%d (%s) eats...\n", n, Names[ n ] );
	Vsem( &SIsem );
    }
    CSswitch();

#ifdef EX_DEBUG
    EXdelete();
#endif
}

/* Observe that philosopher n is thinking */
void
thinks(
i4 n)
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"exception raised while thinking, OK...\n");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    if( Noisy )
    {
	Psem( &SIsem );
	SIfprintf(stderr, "%d (%s) thinks...\n", n, Names[ n ] );
	Vsem( &SIsem );

    }
    CSswitch();

#ifdef EX_DEBUG
    EXsignal(not_handled, 0);

    Psem( &SIsem );
    SIfprintf( stderr,"Error: should not be reached in thinks()...");
    Vsem( &SIsem );

    EXdelete();
#endif
}

/*
**	Setup global variables and add threads for each philosopher.
**	Called by CSinitialize before threads are started
*/
STATUS
hello(
CS_INFO_CB *csib)
{
    i4 i;
    STATUS stat;
    CL_ERR_DESC	err;
#ifdef EX_DEBUG_NO
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in hello()...");
	EXdelete();
	return FAIL;
    }
#endif
#if !defined(axm_vms)
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
#endif

    SIsem.cs_value = 0;
    SIsem.cs_count = 0;
    SIsem.cs_owner = NULL;
    SIsem.cs_list = NULL;
    SIsem.cs_next = NULL;

    Freesem.cs_value = 0;
    Freesem.cs_count = 0;
    Freesem.cs_owner = NULL;
    Freesem.cs_list = NULL;
    Freesem.cs_next = NULL;

    for( i = 0, stat = OK; i < n_phils && stat == OK; i++ )
    {
	Freesticks[ i ] = 2;
	stat = CSadd_thread( 0, (PTR)NULL, i + 1, (CS_SID*)NULL, &err );
    }

    /* no semaphore needed, called before CSdispatch */
    if( Noisy )
	SIfprintf(stderr, "World begins, philosophers wonder why.\n" );

#ifdef EX_DEBUG_NO
    EXdelete();
#endif
    return( stat );
}


/* Called when all thread functions return from the TERMINATE state */
STATUS
bye()
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in bye()...");
	EXdelete();
	return FAIL;
    }
#endif

    /* no semaphore needed, called when all threads are dead */
    if( Noisy )
	SIfprintf(stderr, "All philosophers are dead.  World ends.\n");

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( OK );
}


/* ---------------- book keeping ---------------- */


/* request and wait for semaphore */
STATUS
Psem(
CS_SEMAPHORE *sp)
{
    STATUS rv;
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in Psem()...");
	EXdelete();
	return FAIL;
    }
#endif

    if( rv = CSp_semaphore( TRUE, sp ) )
	semerr( rv, sp, "Psem" );

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( rv );
}

/* release a semaphore */
STATUS
Vsem(
CS_SEMAPHORE *sp)
{
    STATUS rv;
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in Vsem()...");
	EXdelete();
	return FAIL;
    }
#endif

    if( rv = CSv_semaphore( sp ) )
	semerr( rv, sp, "Vsem" );

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( rv );
}

/* decode semaphore error */
void
semerr(
i4 rv,
CS_SEMAPHORE *sp,
char *msg)
{
    MY_SCB *scb;
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in semerr()...");
	EXdelete();
	return;
    }
#endif

    CSget_scb( (CS_SCB **)&scb );
    SIfprintf(stderr, "%s %p returned %d (%s)\n", msg, sp, rv, maperr(rv) );
    SIfprintf(stderr, "\tPhilosopher %d, scb %p\n", scb->phil, scb );
    SIfprintf(stderr, "\t%p->cs_value %d\n", sp, sp->cs_value );
    SIfprintf(stderr, "\t%p->cs_count %d\n", sp, sp->cs_count );
    SIfprintf(stderr, "\t%p->cs_owner %x\n", sp, sp->cs_owner );
    SIfprintf(stderr, "\t%p->cs_list %p\n", sp, sp->cs_list );
    SIfprintf(stderr, "\t%p->cs_next %p\n", sp, sp->cs_next );

#ifdef EX_DEBUG
    EXdelete();
#endif
}

/* If we get semaphore errors, do simple decoding */

char *
maperr(
STATUS rv)
{
    char *ret_val;

#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	SIfprintf( stderr,"Error: unexpected exception in maperr()...");
	EXdelete();
	return "";
    }
#endif

    switch( rv )
    {
    case E_CS0017_SMPR_DEADLOCK:
	ret_val = "E_CS0017_SMPR_DEADLOCK";
	break;

    case E_CS000F_REQUEST_ABORTED:
	ret_val = "E_CS000F_REQUEST_ABORTED";
	break;

    case E_CS000A_NO_SEMAPHORE:
        ret_val = "E_CS000A_NO_SEMAPHORE";
	break;

    case OK:
	ret_val = "";
	break;

    default:
	ret_val = "Unknown exception";
	break;
    }

#ifdef EX_DEBUG
    EXdelete();
#endif
    return ret_val;
}

/* log function, called as the output when needed */

void
logger(
i4 errnum, i4 arg1, i4 arg2)
{
    char buf[ ER_MAX_LEN ];
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in logger()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    Psem( &SIsem );
    if( ERreport( errnum, buf ) == OK )
    	SIfprintf(stderr, "%s\n", buf);
    else
	SIfprintf(stderr, "ERROR %d (%x), %s %d\n", 
		errnum, errnum, arg1, arg2 );
    Vsem( &SIsem );

#ifdef EX_DEBUG
    EXdelete();
#endif
    if(errnum != E_CS0018_NORMAL_SHUTDOWN)
	PCexit(FAIL);
    return OK;
}

/*
** Make a new scb, ignoring crb and saving type. Called by CSadd_thread
**
** 'type', passed by 6.1 CS, is ignored here for compatibility with 6.0 CS.
*/
newscbf( newscb, crb, type )
MY_SCB **newscb;
PTR crb;
i4 type;
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in newscbf()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    /* god this is ugly, making us store type here... */
    *newscb = (MY_SCB*) calloc( 1, sizeof( **newscb ) );

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( NULL == *newscb );
}

/* release an scb */

freescb( oldscb )
MY_SCB *oldscb;
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in freescb()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    free( oldscb );

#ifdef EX_DEBUG
    EXdelete();
#endif
}

/* Function used when CS_CB insists on having one and we don't care */
STATUS
fnull()
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in fnull()...");
	Vsem( &SIsem );
    }
    EXdelete();
#endif

    return( OK );
}

void
vnull()
{
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in vnull()...");
	Vsem( &SIsem );
    }
    EXdelete();
#endif
}

/* Function needed for read and write stub */

rwnull( scb, sync )
MY_SCB *scb;
i4 sync;
{
    CS_SID sid;
#ifdef EX_DEBUG
    EX_CONTEXT context;

    if (EXdeclare(ex_handler, &context) != OK) {
	/* some exception was raised */
	Psem( &SIsem );
	SIfprintf( stderr,"Error: unexpected exception in rwnull()...");
	Vsem( &SIsem );

	EXdelete();
	return;
    }
#endif

    CSget_sid( &sid );
    CSresume( sid );

#ifdef EX_DEBUG
    EXdelete();
#endif
    return( OK );
}


/* Set up philosopher threads and run them.  No args accepted. */

main(argc, argv)
i4	argc;
char	*argv[];
{
    CS_CB 	cb;
    i4 	rv;
    i4 	err_code;
    i4 	f_argc;
    char 	**f_argv,*f_arr[3];
    CL_ERR_DESC syserr;
    STATUS	status;

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    /* test how many philosopher wanted */
    if(argc == 1)
    {
	n_phils = N;
    }
    else if(argc == 2)
    {
	n_phils = atoi(argv[1]);
        if (n_phils > N || n_phils < 1)
	    n_phils = N;
    }
    else
    {
	SIfprintf(stderr, "usage: %s [num_threads]\n", argv[0]);
	PCexit(FAIL);
    }

    MEadvise( ME_INGRES_ALLOC );

    EXsetclient(EX_INGRES_DBMS);

    GCsetMode((i2)GC_SERVER_MODE);
    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &syserr);

    PMinit();

    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);

#if !defined(axm_vms)
    /* default some signals */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
#endif

    /* setup big CS parameter block to set stuff up */
    cb.cs_scnt = n_phils;	/* Number of sessions */
    cb.cs_ascnt = 0;		/* nbr of active sessions */
    cb.cs_stksize = (8 * 1024);	/* size of stack in bytes */
    cb.cs_scballoc = newscbf;	/* Routine to allocate SCB's */
    cb.cs_scbdealloc = freescb;	/* Routine to dealloc  SCB's */
    cb.cs_saddr = fnull;	/* Routine to await session requests */
    cb.cs_reject = fnull;	/* how to reject connections */
    cb.cs_disconnect = fnull;	/* how to dis- connections */
    cb.cs_read = rwnull;	/* Routine to do reads */
    cb.cs_write = rwnull;	/* Routine to do writes */
    cb.cs_process = philosopher; /* Routine to do major processing */
    cb.cs_attn = fnull;		/* Routine to process attn calls */
    cb.cs_elog = logger;	/* Routine to log errors */
    cb.cs_startup = hello;	/* startup the server */
    cb.cs_shutdown = bye;	/* shutdown the server */
    cb.cs_format = fnull;	/* format scb's */
    cb.cs_get_rcp_pid = fnull;
    cb.cs_scbattach = vnull;
    cb.cs_scbdetach = vnull;
    cb.cs_stkcache = FALSE;     /* Match csoptions default - no stack caching */
    cb.cs_format_lkkey = NULL;  /* Not used in this context. */

    /* now fudge argc and argv for CSinitiate */
    f_argc = 2;
    f_arr[0] = argv[0];
    f_arr[1] = "-nopublic";
    f_arr[2] = NULL;
    f_argv = f_arr;

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

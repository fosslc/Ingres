/******************************************************************************
** csphil.c -- Dining philosophers test of CS
**
** This program simulates the well known "Dining Philosphers" problem (see
** for instance, CACM Jan 1988).  It is used to test and demonstrate the
** thread and synchronization primitives provided by the INGRES CS facility.
**
** There are N philosphers who spend their lives eating or thinking.  Each
** philospher has his own place at a circular table, in the center of which
** is a large bowl of rice.  To eat rice requires two chopsticks, but only
** N chopsticks are provided, one between each pair of philosphers.  The
** only forks a philospher can pick up are those on his immediate right and
** left.  Each philospher is identical in structure, alternately eating
** then thinking.  The problem is to simulate the behaviour of the
** philosphers while avoiding deadlock (the request by a philosopher for a
** chopstick can never be granted) and indefinate postponement (the request
** by a philospher is continually denied), known colloquially as "starvation."
**
**     "Though the problem of the Dining philosphers appears to have
**     greater entertainment value than practical importance, it has
**     had huge theoretical influence.  The problem allows the classic
**     pitfalls of concurrent programming to be demonstrated in a
**     graphical situation.  It is a benchmark of the expressive power
**     of new primitives of concurrent programming and stands as a
**     challenge to proposers of new languages"
**             - G.A. Ringword, CACM Jan 1988.
** History:
**	20-Jul-2004 (lakvi01)
**		SIR#112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date.
**  05-Jan-2007 (wonca01) BUG 117262
**      Change signature of logger() to reflect the change in CS usage
**      from STATUS* to CL_ERR_DESC*.
**  11-Aug-2009 (fanra01)
**      BUG 122443
**      Update initialization of CS_CB to include additional elements causing
**      exceptions.
**      Add a name to the CS_INFO_CB.
*****************************************************************************/

# include <compat.h>
# include <sp.h>
# include <cs.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <st.h>
# include <tr.h>
# include <ex.h>
# include <pc.h>
# include <csinternal.h>

# define N		1200		/* number of threads */
# define MAX_N 	1200		/* max number of philosophers */
# define NBITES	3		/* bites and we are done. */

/******************************************************************************
**
** Init to non-zero for verbose version
**
******************************************************************************/

i4              Noisy = 1;

/******************************************************************************
**  Initialize semaphore type for appropriate in-process (CS_SEM_SINGLE) or
**  cross-process (CS_SEM_MULTI) semaphore test.
**  i4              semType = CS_SEM_MULTI;
******************************************************************************/
i4              semType = CS_SEM_SINGLE;

/******************************************************************************
**
** Gag me.  This is the only place we can keep the type passed
** in by CSadd_thread for 6.1 CS, so we create a whole new type.
**
******************************************************************************/

typedef struct _MY_SCB {
	CS_SCB          cs_scb;	/* the "semi-opaque" system scb */
	i4              phil;	/* my data, filled in by newscb( ... type ) */
} MY_SCB;

/******************************************************************************
**
** Some concrete names for the ephemeral
**
******************************************************************************/

char *Names[MAX_N];
/*
	= { "Aquinas",
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
*/

/******************************************************************************
**
** status of thread (philosopher)
**
******************************************************************************/

i4              status[MAX_N];

/******************************************************************************
**
** number of threads wanted
**
******************************************************************************/

i4              n_phils;

/******************************************************************************
**
** Count of the number of "living" philosophers
**
******************************************************************************/

i4              Threads = 0;

/******************************************************************************
**
** Protects SI calls
**
******************************************************************************/

CS_SEMAPHORE    SIsem;

/******************************************************************************
**
** Protect printf
**
******************************************************************************/

CRITICAL_SECTION Pcrit;

/******************************************************************************
**
** Protects Freesticks during updates from access by multiple threads
**
******************************************************************************/

CS_SEMAPHORE    Freesem;

/******************************************************************************
**
** Count of sticks available RIGHT and LEFT of each philosopher
**
******************************************************************************/

i4              Freesticks[N];

/******************************************************************************
**
** Array indeces of neighbors in Freesticks
**
******************************************************************************/

# define LEFT( n )  ( n ? n - 1 : n_phils - 1 )
# define RIGHT( n ) ( n == (n_phils - 1) ? 0 : n + 1 )

/******************************************************************************
**
** function forward refs
**
******************************************************************************/

void serprintf(char *fmt,...);
void eats(long n);
void thinks(long n);
int philosopher(long mode,struct _MY_SCB *scb,long *next_mode);
int hello(struct _CS_INFO_CB *csib);
int bye(void);
long Psem(struct _CS_SEMAPHORE *sp);
long Vsem(struct _CS_SEMAPHORE *sp);
void semerr(long rv,struct _CS_SEMAPHORE *sp,char *msg);
char *maperr(long rv);
void logger(long errnum, CL_ERR_DESC *arg1,long arg2);
long newscbf(struct _MY_SCB **newscb ,char *crb,long type);
long freescb(struct _MY_SCB *oldscb);
long waitnull(void);
long fnull(void);
long vnull(void);
int rwnull(struct _MY_SCB *scb,long sync);

/******************************************************************************
**
**
**
******************************************************************************/

# ifdef EX_DEBUG
intfunc()
{
	printf("Signal #%d received, continue processing \ n ",
	       SIGUSR1);
}
# endif				/* EX_DEBUG */

/******************************************************************************
**
** Serial printf
**
******************************************************************************/
VOID
serprintf(char *fmt,...)
{
	va_list         marker;

	EnterCriticalSection(&Pcrit);

	va_start(marker, fmt);
	vprintf(fmt, marker);
	va_end(marker);
	_flushall();

	LeaveCriticalSection(&Pcrit);
}

/******************************************************************************
**
**
**
******************************************************************************/
VOID
eats(n)
	i4              n;
{
	/*
	 * While you can't get both sticks, give control to others who might
	 * eventually free the ones you need
	 */
	for (;;) {
		Psem(&Freesem);
		if (Freesticks[n] == 2)
			break;
		Vsem(&Freesem);
	}

	/* ASSERT:  phil n holds Freesem and Freesticks[ n ] == 2. */

	Freesticks[n] -= 2;
	--Freesticks[LEFT(n)];
	--Freesticks[RIGHT(n)];
	if (Noisy) {
		serprintf("%d (%s) eats...\n",
			  n,
			  Names[n]);
	}
	Vsem(&Freesem);
}

/******************************************************************************
**
**
**
******************************************************************************/
VOID
thinks(n)
	i4              n;
{
	Psem(&Freesem);

	Freesticks[n] += 2;
	++Freesticks[LEFT(n)];
	++Freesticks[RIGHT(n)];
	if (Noisy) {
		serprintf("%d (%s) thinks...\n",
			  n,
			  Names[n]);
	}
	Vsem(&Freesem);
}

/******************************************************************************
**
**
**
******************************************************************************/
philosopher(mode, scb, next_mode)
	i4              mode;
	MY_SCB         *scb;
	i4             *next_mode;
{
	i4              bites = 0;

	switch (mode) {
	case CS_INITIATE:	/* A new philsopher is born */

		status[scb->phil] = mode;
		while (bites < NBITES) {
			eats(scb->phil);
			bites++;
			thinks(scb->phil);
		}

		/* fall into: */

	default:
		*next_mode = CS_TERMINATE;
		break;

	case CS_TERMINATE:
		if (Noisy) {
# ifdef EX_DEBUG
			signal(SIGUSR1, intfunc);
			if (scb->phil == 1) {
				serprintf("Send a signal #%d to process #%d\n",
					  SIGUSR1, getpid());
				pause();
			}
# endif				/* EX_DEBUG */
			serprintf("%d (%s) dies, RIP.\n",
				  scb->phil,
				  Names[scb->phil]);
			if (status[scb->phil] == mode) {
				serprintf("Oops this philosopher is already dead?\n");
				serprintf("\t\t CS code is non-functional\n");
			}
		}
		*next_mode = CS_TERMINATE;

		/* If no more threads, shutdown */
		status[scb->phil] = mode;

		if (--Threads == 0)
			CSterminate(CS_KILL, (i4  *) NULL);

		break;
	}
	return (OK);
}

/******************************************************************************
**
**  Setup global variables and add threads for each philosopher.
**  Called by CSinitialize before threads are started
**
******************************************************************************/
hello(csib)
	CS_INFO_CB     *csib;
{
	i4              i;
	STATUS          stat;
	CL_SYS_ERR      err;

	InitializeCriticalSection(&Pcrit);
	CSi_semaphore(&Freesem, semType);
	CSn_semaphore(&Freesem, "Freesem");

    STncpy( csib->cs_name, "AlistenString", sizeof(csib->cs_name) );
	for (i = 0, stat = OK; i < n_phils && stat == OK; i++) {
		Freesticks[i] = 2;
		stat = CSadd_thread(0, (PTR) NULL, i + 1, NULL, &err);
		if (stat)
			printf("%d\t0x%lx\n", i, err.errnum);
	}

	/* no semaphore needed, called before CSdispatch */
	if (Noisy)
		serprintf("World begins, philosophers wonder why.\n");

	return (stat);
}

/******************************************************************************
**
** Called when all thread functions return from the TERMINATE state
**
******************************************************************************/
bye()
{
	/* semaphore needed, called when all threads are dead */
	CSr_semaphore(&Freesem);
	if (Noisy)
		serprintf("All philosophers are dead.  World ends.\n");
	return (OK);
}

/******************************************************************************
**
** set a semaphore
**
******************************************************************************/
i4
Psem(sp)
	CS_SEMAPHORE   *sp;
{
	i4  rv;

	if (rv = CSp_semaphore(TRUE, sp))
		semerr(rv, sp, "Psem");
	return (rv);
}

/******************************************************************************
**
** release a semaphore
**
******************************************************************************/
i4
Vsem(sp)
	CS_SEMAPHORE   *sp;
{
	i4  rv;

	if (rv = CSv_semaphore(sp))
		semerr(rv, sp, "Vsem");
	return (rv);
}

/******************************************************************************
**
** decode semaphore error
**
******************************************************************************/
VOID
semerr(rv, sp, msg)
	i4              rv;
	CS_SEMAPHORE   *sp;
	char           *msg;
{
	MY_SCB         *scb;

	CSget_scb((CS_SCB **) (&scb));
	serprintf("%s %x returned %d (%s)\n",
		  msg,
		  sp,
		  rv,
		  maperr(rv));
	serprintf("\tPhilosopher %d, scb %x, mutex %d\n",
		  scb->phil,
		  scb,
		  sp->cs_mutex);
}

/******************************************************************************
**
**
**
******************************************************************************/
char *
maperr(rv)
	i4              rv;
{
	switch (rv) {
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

/******************************************************************************
**
**
**
******************************************************************************/
VOID
logger(errnum, arg1, arg2)
	i4              errnum;
	CL_ERR_DESC     *arg1;
	i4              arg2;
{
/*
	char            buf[ER_MAX_LEN];
	if( ERreport( errnum, buf ) == OK ) {
		serprintf("%s\n",
		       buf);
	} else {
		serprintf("ERROR %d (%x),
		       %s %d\n",
		       errnum,
		       errnum,
		       arg1,
		       arg2 );
	}
*/
	if (errnum != E_CS0018_NORMAL_SHUTDOWN)
		exit(FAIL);
	serprintf("Normal Shutdown\n");
	exit(OK);
}

/******************************************************************************
**
** Make a new scb, ignoring crb and saving type. Called by CSadd_thread
** 'type', passed by 6.1 CS, is ignored here for compatibility with 6.0 CS.
**
******************************************************************************/
STATUS
newscbf(newscb, crb, type)
	MY_SCB        **newscb;
	PTR             crb;
	i4              type;
{
	/* god this is ugly, making us store type here... */

	*newscb = (MY_SCB *) calloc(1, sizeof(**newscb));
	(*newscb)->phil = Threads++;
	return (NULL == *newscb);
}

/******************************************************************************
**
**
**
******************************************************************************/
STATUS
freescb(oldscb)
	MY_SCB         *oldscb;
{
	free(oldscb);
	return OK;
}

/******************************************************************************
**
**
**
******************************************************************************/
STATUS
waitnull()
{
	Sleep(1000L * 100L);

	while(Threads)
		Sleep(1000L * 10L);
	return OK;
}

/******************************************************************************
**
**
**
******************************************************************************/
STATUS
fnull()
{
	return (OK);
}

/******************************************************************************
**
**
**
******************************************************************************/
STATUS
vnull()
{
	return OK;
}

/******************************************************************************
**
**
**
******************************************************************************/
rwnull(scb, sync)
	MY_SCB         *scb;
	i4              sync;
{
	i4              sid;

	CSget_sid(&sid);
	CSresume(sid);
	return (OK);
}

/******************************************************************************
**
**
**
******************************************************************************/
main(argc, argv)
	i4              argc;
	char           *argv[];
{
	CS_CB           cb;
	i4              rv;
	i4              f_argc;
	char          **f_argv, *f_arr[3];
	int				i;

	/* test how many philosopher wanted */
	if (argc == 1) {
		n_phils = N;
	} else if (argc == 2) {
		n_phils = atoi(argv[1]);
		if (n_phils > N || n_phils < 1)
			n_phils = N;
	} else {
		serprintf("usage: %s [num_threads]\n",
			  argv[0]);
		exit(FAIL);
	}

	for (i=0; i<n_phils; i++) {
		Names[i] = (char *) malloc(12);
		sprintf(Names[i], "Phil %4d", i);
	}

	/* setup big CS parameter block to set stuff up */

	cb.cs_scnt       = n_phils;			/* Number of sessions */
	cb.cs_ascnt      = 0;				/* nbr of active sessions */
	cb.cs_stksize    = (8 * 1024);		/* size of stack in bytes */
	cb.cs_scballoc   = newscbf;			/* Routine to allocate SCB's */
	cb.cs_scbdealloc = freescb;			/* Routine to dealloc  SCB's */
	cb.cs_saddr      = fnull;		/* Routine to await session requests */
	cb.cs_reject     = fnull;			/* how to reject connections */
	cb.cs_disconnect = (STATUS(*) ()) vnull;	/* how to disconnections */
	cb.cs_read       = rwnull;			/* Routine to do reads */
	cb.cs_write      = rwnull;			/* Routine to do writes */
	cb.cs_process    = philosopher;		/* Routine to do major processing */
	cb.cs_attn       = fnull;			/* Routine to process attn calls */
	cb.cs_elog       = logger;			/* Routine to log errors */
	cb.cs_startup    = hello;			/* startup the server */
	cb.cs_shutdown   = bye;				/* shutdown the server */
	cb.cs_format     = fnull;			/* format scb's */
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

	if (rv = CSinitiate(&f_argc, &f_argv, &cb)) {
		serprintf("CSinitiate returns %d\n",
			  rv);
		exit(FAIL);
	}
	/* now do everything that has been set up */
	rv = CSdispatch();

	/* should not get here */
	serprintf("CSdispatch returns %d (%x)\n",
		  rv,
		  rv);

	exit(FAIL);
}

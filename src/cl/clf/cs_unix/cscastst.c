/*
** Copyright (c) 2004 Ingres Corporation
**
*/
/*
** CSCASTST.C -- Test CScas functions
**
**	This program spawns a number of threads, each of which repeatedly 
**	increments a global value using either CScas_long,
**	CScas_quad, CScas_ptr, single process
**	mutex protection, or cross process mutex protection (selected with -L,
**	-P, -M, or -X command line arguments).
**
**	If selected increment method is functioning properly, the final
**	sum will be the product of the number of threads run times the
**	number of iterations per thread.
**
**	Multiple instances of this program may be run, although for
**	single process mutexes, or mixed protection methods, test
**	failure should be expected.
**
**	If multiple instances are run simultaneously, only the last
**	instance connected to the shared memory segment will check the
**	results.
**
**	To perform a meaningful test, this should be run using OS threads 
**	on a multiprocessor box.
**
**	Performance metrics can also be obtained using the UNIX 'time'
**	command. (e.g. 'time cscastst -P')
**
**	usage:
**	
**	   cscastst [-L|-P|-M|-X] <number_loops> <number_threads>
**
**	Default is test CScas_long for 1000000 loops
**	and 8 threads.
**
** History:
**	22-jan-2002 (2002)
**	   Original version crafted out of csphil.
**	03-oct-2002 (devjo01)
**	    Set NUMA context.
**	21-nov-2002 (devjo01)
**	    Add CScas_quad testing, mutex testing, and
**	    shared memory segment.
**	02-oct-2003 (devjo01)
**	    Add CScas_ptr testing.
**	17-Nov-2003 (devjo01)
**	    Changes to ease porting.  Functions renamed, CScas_quad
**	    dropped, CScas_long -> CScas4.  "oldval" passed by
**	    value, not by reference, and is not automatically
**	    refreshed.
**	29-dec-2003 (sheco02)
**	    Change CScas_ptr to CScasptr to resolve undefined symbol.
**	23-jan-2004 (devjo01)
**	    Howl, if conf_CAS_ENABLED not defined.
**	13-feb-2004 (devjo01)
**	    If conf_CAS_ENABLED not defined, dummy out CScas references.
**	15-mar-2004 (devjo01)
**	    Only disable CAS tests if conf_CAS_ENABLED not defined.
**	    Allow mutex tests to run.
**	07-oct-2004 (thaju02)
**	    Change pages, pagesgot to SIZE_TYPE.
**	1-Jun-2005 (schka24)
**	    Get pages, not bytes.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes, use i8 not long long.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**
** PROGRAM  = cscastst
**
** NEEDLIBS = COMPAT MALLOCLIB
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
# include <st.h>
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

# include <stdarg.h>

#if defined(ris_u64)
# include <stdlib.h>
#endif

# if !defined(conf_CAS_ENABLED)
# define	CScas4(p,o,n)	0
# define	CScasptr(p,o,n)   0
# endif

GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF       CS_SERV_INFO    *Cs_svinfo;
GLOBALDEF       CS_SEMAPHORE    sc0m_semaphore;
GLOBALDEF       CS_SEMAPHORE    test_semaphore;

# define N_THREADS		       64	/* Max number of threads */
# define N_THREADS_DEFAULT	        8	/* Default number of threads */

# define N_LOOPS	       1000000000	/* Max number of loops */
# define N_LOOPS_DEFAULT	  1000000	/* Default number of loops */

# define MEM_KEY		"cscastst.mem"

/* System segment was allocated by me */

/* status of thread (incrementor) */
/* number of threads wanted */
i4	N_threads;

/* number of iterations wanted */
i4	N_loops;

/* Count of the number of "terminated" incrementors */
i4  End_Threads = 0;

/* Test counter repeatedly incremented by all threads */
struct {
    i8		QuadSum;
    i4	        TargetSum;
    i4	  	LongSum;
    i4  	NextIdent;
    i4		AllocedSysSeg;
    i4		Connects;
    i8		Calls[N_THREADS]; 
    CS_SEMAPHORE	CPsemaphore;
} *Pshm; 

i4	(*Incrementor)(i4, CS_SCB *, i4 *);
i4	AllocedSysSeg;

/* function forward refs */

/* ---------------- actual application ---------------- */

/*
** Adjust number of connections, and return previous connect count.
*/
static i4
adj_connects( i4 adj_amt )
{
    i4	connects;

    CSp_semaphore( TRUE, &sc0m_semaphore );
    connects = Pshm->Connects += adj_amt;
    CSv_semaphore( &sc0m_semaphore );
    return connects;
}

/*
** Adjust TargetSum.
*/
static void
adj_target( i4 adj_amt )
{
    CSp_semaphore( TRUE, &sc0m_semaphore );
    Pshm->TargetSum += adj_amt;
    CSv_semaphore( &sc0m_semaphore );
}

/*
** Get next thread ID.
*/
static i4
next_ident(void)
{
    i4	curident;

    CSp_semaphore( TRUE, &sc0m_semaphore );
    curident = Pshm->NextIdent;
    Pshm->NextIdent++;
    CSv_semaphore( &sc0m_semaphore );
    return curident;
}

/*
** Thread epilogue code.
*/
static void
wrap_up(void)
{
    CL_ERR_DESC syserr;
    i4	newval;

    CSp_semaphore( TRUE, &sc0m_semaphore );
    newval = ++End_Threads;
    CSv_semaphore( &sc0m_semaphore );

    if( newval == N_threads )
    {
	TRset_file(TR_F_CLOSE, TR_OUTPUT, TR_L_OUTPUT, &syserr);
	CSterminate( CS_KILL, (i4 *)NULL );
    }
}


/*
**  FOUR byte integer C&S incrementor thread
*/
static STATUS
test_long( i4 mode, CS_SCB *scb, i4 *next_mode )
{
    i4  iteration, curval, newval, myident;

    switch( mode )
    {
    case CS_INITIATE:	/* A new incrementor is born */

	myident = next_ident();
	if ( myident < N_THREADS )
	{
	    for ( iteration = 0; iteration < N_loops; iteration++ )
	    {
		do 
		{
		    curval = Pshm->LongSum;
		    Pshm->Calls[myident]++;
		    newval = curval + 1;
		} while ( CScas4(&Pshm->LongSum, curval, newval) );
	    }
	}
	else
	{
	    adj_target( -N_loops );
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	break;

    case CS_TERMINATE:
	*next_mode = CS_TERMINATE;
	wrap_up();
	break;
    }
    return( OK );
}

/*
**  Pointer C&S incrementor thread
*/
static STATUS
test_ptr( i4 mode, CS_SCB *scb, i4 *next_mode )
{
    i4 		iteration, myident;
    PTR		curval, newval;	

    switch( mode )
    {
    case CS_INITIATE:	/* A new incrementor is born */

	myident = next_ident();
	if ( myident < N_THREADS )
	{
	    for ( iteration = 0; iteration < N_loops; iteration++ )
	    {
		do 
		{
		    curval = (PTR)(SCALARP)(Pshm->QuadSum);
		    Pshm->Calls[myident]++;
		    newval = (PTR)((SCALARP)curval + 1);
		} while ( CScasptr((PTR*)&Pshm->QuadSum,
			  curval, newval) );
	    }
	}
	else
	{
	    adj_target( -N_loops );
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	break;

    case CS_TERMINATE:
	*next_mode = CS_TERMINATE;
	wrap_up();
	break;
    }
    return( OK );
}
/*
**  EIGHT byte integer incrementor thread using single process mutex protection
*/
static STATUS
test_mutex( i4 mode, CS_SCB *scb, i4 *next_mode )
{
    STATUS	rv;
    i4		iteration, myident;

    switch( mode )
    {
    case CS_INITIATE:	/* A new incrementor is born */

	myident = next_ident();
	if ( myident < N_THREADS )
	{
	    for ( iteration = 0; iteration < N_loops; iteration++ )
	    {
		if( rv = CSp_semaphore( TRUE, &test_semaphore ) )
		{
		    SIfprintf( stderr, "CSp_semaphore ERROR!.  rv=%d (0x%x)\n",
			      rv, rv );
		}
		Pshm->QuadSum++;
		Pshm->Calls[myident] ++; 
		if( rv = CSv_semaphore( &test_semaphore ) )
		{
		    SIfprintf( stderr, "CSv_semaphore ERROR!.  rv=%d (0x%x)\n",
			       rv, rv );
		}
	    }
	}
	else
	{
	    adj_target( -N_loops );
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	Pshm->Calls[myident] += test_semaphore.cs_smstatistics.cs_smxx_count; 
	break;

    case CS_TERMINATE:
	*next_mode = CS_TERMINATE;
	wrap_up();
	break;
    }
    return( OK );
}

/*
**  EIGHT byte integer incrementor thread using cross process mutex protection
*/
static STATUS
test_cpmutex( i4 mode, CS_SCB *scb, i4 *next_mode )
{
    STATUS	rv;
    i4		iteration, myident;
    longlong	curval, newval;	

    switch( mode )
    {
    case CS_INITIATE:	/* A new incrementor is born */

	myident = next_ident();
	if ( myident < N_THREADS )
	{
	    for ( iteration = 0; iteration < N_loops; iteration++ )
	    {
		if( rv = CSp_semaphore( TRUE, &Pshm->CPsemaphore ) )
		{
		    SIfprintf( stderr, "CSp_semaphore ERROR!.  rv=%d (0x%x)\n",
			      rv, rv );
		}
		Pshm->QuadSum++;
		Pshm->Calls[myident] ++; 
		if( rv = CSv_semaphore( &Pshm->CPsemaphore ) )
		{
		    SIfprintf( stderr, "CSv_semaphore ERROR!.  rv=%d (0x%x)\n",
			      rv, rv );
		}
	    }
	}
	else
	{
	    adj_target( -N_loops );
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	Pshm->Calls[myident] += test_semaphore.cs_smstatistics.cs_smxx_count; 
	break;

    case CS_TERMINATE:
	*next_mode = CS_TERMINATE;
	wrap_up();
	break;
    }
    return( OK );
}

/*
**	Setup global variables and add threads for each incrementor.
**	Called by CSinitialize before threads are started
*/
static STATUS
hello( CS_INFO_CB *csib )
{
    i4		i;
    STATUS	stat;
    CL_SYS_ERR	err;

    SIfprintf(stderr,
     "Testing %s for %d iterations and %d threads.\n",
      ( Incrementor == test_long ) ? "CScas4" :
      ( Incrementor == test_ptr ) ? "CScasptr" :
      ( Incrementor == test_mutex ) ? "Single process mutex" :
      "Cross process mutex", N_loops, N_threads);

    CSi_semaphore(&sc0m_semaphore, CS_SEM_SINGLE);
    CSi_semaphore(&test_semaphore, CS_SEM_SINGLE);
    if ( 1 == adj_connects(1) )
    {
	CSi_semaphore(&Pshm->CPsemaphore, CS_SEM_MULTI);
	Pshm->AllocedSysSeg = AllocedSysSeg;
    }
    adj_target( N_loops * N_threads );
 
    csib->cs_mem_sem = &sc0m_semaphore;
    for( i = 0, stat = OK; i < N_threads && stat == OK; i++ )
    {
	stat = CSadd_thread( 0, (PTR)NULL, i + 1, (CS_SID*)NULL, &err );
    }

    return( OK );
}


/* Called when all thread functions return from the TERMINATE state */

static STATUS
bye(void)
{
    STATUS	rv;
    int		i, connects;
    i8		calls;
    CL_SYS_ERR	err;

    connects = adj_connects( -1 );
    if ( connects )
    {
	SIfprintf(stderr, "Process %d ended, %d connection(s) left.\n",
		  getpid(), connects);
	rv = MEdetach( MEM_KEY, (PTR)Pshm, &err );
    }
    else
    {
	Pshm->QuadSum = Pshm->QuadSum + Pshm->LongSum;

	/* no semaphore needed, called when all threads are dead */
	if ( Pshm->QuadSum != Pshm->TargetSum )
	{
	    SIfprintf(stderr, "*** Test Failed! ***  Expected %d, got %ld.\n",
		       Pshm->TargetSum, Pshm->QuadSum );
	}
	else
	{
	    for ( i = 0, calls = 0l; i < Pshm->NextIdent; i++ )
		calls += Pshm->Calls[i];

	    SIfprintf(stderr, "Test Passed with %ld successful increments.",
		      Pshm->QuadSum);
	    SIfprintf(stderr, " %ld contentions detected.\n",
		      calls - Pshm->QuadSum);
	}

	MEsmdestroy( MEM_KEY, &err );

	/* if we allocated the sytem segment, clear up now. */
	if( Pshm->AllocedSysSeg )
	    CS_des_installation();
    }

    return( OK );
}


/* log function, called as the output when needed */

static VOID
logger( i4 errnum, CL_ERR_DESC *dum, i4 numargs, ... )
{
    char buf[ ER_MAX_LEN ];
    va_list args;

    if( ERreport( errnum, buf ) == OK )
    	SIfprintf(stderr, "%s\n", buf);
    else
    {
	char *arg1 = "";
	i4 arg2 = 0;
	if (numargs > 0)
	{
	    va_start(args, numargs);
	    arg1 = va_arg(args, char *);
	    if (numargs > 1)
		arg2 = va_arg(args, i4);
	    va_end(args);
	}
	SIfprintf(stderr, "ERROR %d (%x), %s %d\n", 
		errnum, errnum, arg1, arg2 );
    }
    if(errnum != E_CS0018_NORMAL_SHUTDOWN)
	PCexit(FAIL);
    PCexit(OK);
}

/*
** Make a new scb, ignoring crb and saving type. Called by CSadd_thread
**
** 'type', passed by 6.1 CS, is ignored here for compatibility with 6.0 CS.
*/
static STATUS
newscbf( CS_SCB **newscb, void *crb, i4 type )
{
    /* god this is ugly, making us store type here... */

    *newscb = (CS_SCB*)calloc( 1, sizeof( **newscb ) );
    return( NULL == *newscb );
}

/* release an scb */

static STATUS
freescb( CS_SCB *oldscb )
{
    free( oldscb );
    return (OK);
}

/* Function used when CS_CB insists on having one and we don't care */

static STATUS
fnull1(void *dum1, i4 dum2)
{
    return (OK);
}

static STATUS
fnull2(i4 dum1, CS_SCB *dum2)
{
    return (OK);
}
static STATUS
fnull3(CS_SCB *dum1, char *dum2, i4 dum3, i4 dum4)
{
    return (OK);
}

static VOID
vnull(CS_SCB *dum)
{
}

/* Function needed for read and write stub */

static STATUS
rwnull( CS_SCB *dummy, i4 sync )
{
    CS_SID sid;

    CSget_sid( &sid );
    CSresume( sid );
    return( OK );
}

static i4
pidnull(void)
{
    return (1234);
}


/* Set up incrementor threads and run them.  No args accepted. */

main(argc, argv)
i4	argc;
char	*argv[];
{
    CS_CB 	cb;
    i4  	rv;
    i4  	arg, f_argc;
    char 	**f_argv,*f_arr[3], *argp, dunsel;
    CL_ERR_DESC syserr;
    STATUS	status;
    SIZE_TYPE	pages, pagesgot;
    i4		meflags;

    /* Set default */
    Incrementor = test_long;

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    for ( arg = 1; arg < argc ; arg++ )
    {
	argp = argv[arg];
	if ( 0 == STcmp( argp, "-P" ) )
	{
	    Incrementor = test_ptr;
	    continue;
	}
	if ( 0 == STcmp( argp, "-L" ) )
	{
	    Incrementor = test_long;
	    continue;
	}
	if ( 0 == STcmp( argp, "-M" ) )
	{
	    Incrementor = test_mutex;
	    continue;
	}
	if ( 0 == STcmp( argp, "-X" ) )
	{
	    Incrementor = test_cpmutex;
	    continue;
	}
	if ( 1 == sscanf( argp, "%d%c", &rv, &dunsel ) )
	{
	    if ( 0 == N_loops )
	    {
		N_loops = rv;
		if(N_loops > N_LOOPS || N_loops < 100l)
		{
		    SIfprintf(stderr, "num_loops must be >= 100 and <= %d.\n",
				N_LOOPS);
		    PCexit(FAIL);
		}
		continue;
	    }
	    if ( 0 == N_threads )
	    {
		N_threads = rv;
		if(N_threads > N_THREADS || N_threads < 1)
		{
		    SIfprintf(stderr, "num_threads must be > 1 and <= %d\n",
				N_THREADS);
		    PCexit(FAIL);
		}
		continue;
	    }
	}

		 
	SIfprintf(stderr, "usage: %s [-L|-P|-M|-X] [ num_loops [num_threads]]\n",
		  argv[0]);
	PCexit(FAIL);
    }

# if !defined(conf_CAS_ENABLED)
    if ( !(Incrementor == test_mutex || Incrementor == test_cpmutex) )
    {
	SIfprintf(stderr,
	       "CAS_ENABLED not set in VERS. C&S routines not present!\n" );
	PCexit(FAIL);
    }
# endif

    if ( !N_loops ) N_loops = N_LOOPS_DEFAULT;
    if ( !N_threads ) N_threads = N_THREADS_DEFAULT;

    MEadvise( ME_INGRES_ALLOC );
    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &syserr);

    if( (iiCL_get_fd_table_size() - CS_RESERVED_FDS) < N_threads  )
	(void) iiCL_increase_fd_table_size(FALSE, N_threads + CS_RESERVED_FDS);

    meflags = ME_MSHARED_MASK | ME_IO_MASK |
	ME_CREATE_MASK | ME_NOTPERM_MASK | ME_MZERO_MASK;

    pages = 10;
    status = MEget_pages( meflags, pages, MEM_KEY, (PTR *)&Pshm,
			  &pagesgot, &syserr );
    if ( OK != status )
    {
	if ( ME_ALREADY_EXISTS == status )
	{
	    meflags = ME_MSHARED_MASK | ME_IO_MASK;
	    status = MEget_pages( meflags, pages, MEM_KEY,
				  (PTR *)&Pshm, &pagesgot, &syserr );
	}
	if ( OK != status )
	{
	    SIfprintf( stderr, "MEget_pages ERROR! status=%d (0x%x)\n",
		       status, status );
	    exit(-1);
	}
    }
    else
    {
# ifdef OS_THREADS_USED
	/* Additional wakeup block needed by idle thread replacement */
	if(CS_create_sys_segment(1, N_THREADS + 9, &syserr) == OK)
# else
	if(CS_create_sys_segment(1, N_THREADS + 8, &syserr) == OK)
# endif /* OS_THREADS_USED */
	{
	    AllocedSysSeg = 1;
	}
    }

    PMinit();

    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);

    /* default some signals */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);

    /* setup big CS parameter block to set stuff up */
    cb.cs_scnt = N_threads;	/* Number of sessions */
    cb.cs_ascnt = 0;		/* nbr of active sessions */
    cb.cs_stksize = (8 * 1024);	/* size of stack in bytes */
    cb.cs_scballoc = newscbf;	/* Routine to allocate SCB's */
    cb.cs_scbdealloc = freescb;	/* Routine to dealloc  SCB's */
    cb.cs_saddr = fnull1;	/* Routine to await session requests */
    cb.cs_reject = fnull1;	/* how to reject connections */
    cb.cs_disconnect = vnull;	/* how to dis- connections */
    cb.cs_read = rwnull;	/* Routine to do reads */
    cb.cs_write = rwnull;	/* Routine to do writes */
    cb.cs_process = Incrementor;/* Routine to do major processing */
    cb.cs_attn = fnull2;		/* Routine to process attn calls */
    cb.cs_elog = logger;	/* Routine to log errors */
    cb.cs_startup = hello;	/* startup the server */
    cb.cs_shutdown = bye;	/* shutdown the server */
    cb.cs_format = fnull3;	/* format scb's */
    cb.cs_get_rcp_pid = pidnull;  /* init to fix SEGV in CSMTp_semaphore*/
    cb.cs_scbattach = vnull;    /* init to fix SEGV in CSMT_setup */
    cb.cs_scbdetach = vnull;    /* init to fix SEGV in CSMT_setup */
    cb.cs_stkcache = FALSE;     /* Match csoptions default - no stack caching */
    cb.cs_format_lkkey = NULL;  /* Not used in this context. */

    /* now fudge argc and argv for CSinitiate */
    f_argc = 2;
    f_arr[0] = argv[0];
    f_arr[1] = "-nopublic";
    f_arr[2] = NULL;
    f_argv = f_arr;

    if( status = CSinitiate( &f_argc, &f_argv, &cb ) )
    {
	SIfprintf(stderr, "CSinitiate returns %d\n", status );
	PCexit(FAIL);
    }

    /* now do everything that has been set up */
    status = CSdispatch();

    /* should not get here */
    SIfprintf(stderr, "CSdispatch returns %d (%x)\n", status, status );

    PCexit( FAIL );
}

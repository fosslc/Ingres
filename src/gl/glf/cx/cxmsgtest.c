/*
** Copyright (c) 2004, 2005 Ingres Corporation
**
*/
/*
PROGRAM  = cxmsgtest

NEEDLIBS = COMPAT MALLOCLIB
** Description:
**
**	Program is intended to exercise the CX MSG subsystem.
**
**	If program is invoked with "receive" parameter, it will process
**	messages sent by other programs.  "add", "del" and "mix" send
**	increment, decrement, and a mixture of the two flavors to any
**	receiving programs.  "end" tells receiving programs to exit.
**
** History:
**	06-feb-2001 (devjo01)
**	    Created from csphil.c
**	03-oct-2002 (devjo01)
**	    Set NUMA context.
**	05-Jan-2004 (hanje04)
**	    Replace _SYS_NMLN with SYS_MNLN as _SYS_NMLN is not
**	    defined on later versions of Linux.
**	11-feb-2004 (devjo01)
**	    Set cs_mem_sem in hello().
**      12-apr-2005 (devjo01)
**          Use new CX_MSG struct instead of overloading CX_CMO so as to
**          allow synchronized short messages of 32 bytes.
**	20-apr-2005 (devjo01)
**	    Port to VMS.
**      03-nov-2010 (joea)
**          Declare local functions static.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Fix CS function assignments for new prototyping.
*/


# include <stdarg.h>
# include <sys/utsname.h>
# include <time.h>
# if !defined(VMS) /* _VMS_ */
# include <stdio.h>
# endif /* _VMS_ */

# include <compat.h>
# include <gl.h>
# if !defined(VMS) /* _VMS_ */
# include <clconfig.h>
# endif /* _VMS_ */
# include <pc.h>
# include <cs.h>
# include <lk.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <tr.h>
# include <pm.h>
# include <cv.h>
# include <lo.h>
# include <st.h>
# include <cx.h>
# include <cxprivate.h>

# if !defined(VMS) /* _VMS_ */
/* Externs that should really be in a CS header. */
extern	STATUS CS_des_installation(void);
extern	STATUS CS_create_sys_segment( i4, i4, CL_ERR_DESC * );
# endif /* _VMS_ */

# if defined(VMS) /* _VMS_ */
/*
** FIX-ME. 7.3 RTL does not support vsnprintf, use unsafe alternative
** which is not performing bounds check for a buffer overflow. (Not
** that we expect a buffer overflow, but paranoia is good practice.)
*/
# define vsnprintf(b,n,f,p) vsprintf(b,f,p)
# endif /* _VMS_ */

GLOBALDEF       CS_SEMAPHORE    sc0m_semaphore; /* mem sem expected by SC */

GLOBALREF CX_PROC_CB	 CX_Proc_CB;

# define MAX_INSTANCES	 16	/* Max # of concurrent program invocations */
# define MAX_N		 16
# define THR_EXTRA	  0

/*
**  Test config parameters, with default settings.
*/

/* Init to non-zero for verbose version */
i4  Noisy = 0;

/* number of concurrent instances of this program expected. */
i4	SignedIn = 0;

/* System segment was allocated by me */
bool MySysSeg = TRUE;
bool Is_CSP = TRUE;

char	    *ProgName;

i4	     ThePID;

i4	     Iterations = 1;

char         NodeName[CX_MAX_HOST_NAME_LEN+1] = { '\0' };

time_t       Start_Time, End_Time;

/* status of test thread */
i4	Thread_Status[MAX_N+THR_EXTRA];

CS_SID	SIDs[MAX_N+THR_EXTRA];		/* Session ID for each thread. */

/* Count of the number of "living" threads */
i4  Start_Threads = 0;	/* Monotonicaly increasing counter */
i4  End_Threads = 0;	/* Running count of living threads */

CS_SEMAPHORE Miscsem;
CS_SEMAPHORE SIsem;

typedef struct
{
#define TM_ADD_NODE 0
#define TM_DEL_NODE 1
#define TM_END_NODE 2
    i4	action;
    i4	datum;
}	TEST_MSG;

TEST_MSG	TestMsg;

/* Gag me.  This is the only place we can keep the type passed
   in by CSadd_thread for 6.1 CS, so we create a whole new type. */
typedef struct
{
	CS_SCB	cs_scb;		/* the "semi-opaque" system scb */
	i4	instance;	/* my data */
} MY_SCB;

i4		Receive = 0;
i4		Mixed   = 0;
i4		Delay	= 0;

i4 Psem( CS_SEMAPHORE *sp );
i4 Vsem( CS_SEMAPHORE *sp );

/* ---------- Reporting & Helper Functions ------------ */

#define	BAD_ERROR( id, status ) bad_error( __LINE__, (id), (status) )


/*
** Adjust a counter under mutex protection.
*/
static i4	
adj_counter( i4 *counter, i4 adjustment )
{
    i4		retval;

    (void)Psem( &Miscsem );
    retval = (*counter += adjustment);
    (void)Vsem( &Miscsem );
    return retval;
}	/* adj_counter */

/*
**  Get text eqiv of error code.
*/
static char *
get_err_name( STATUS status, char *buf )
{
    *buf = '\0';
    if ( status )
    {
	/* Temp Kludge */
	(void)sprintf( buf, "E_CL%04.4X", 0x0000ffff & status );
    }
    return buf;
}

/*
**  Free format error formatting routine.
*/
static void
report_error( i4 instance, STATUS status, char *fmt, ... )
{
    time_t	errtime;
    va_list	ap;
    char	scratchbuf[64];
    char	outbuf[256];
    i4		len;

    va_start( ap, fmt );
    
    errtime = time(NULL);
    if ( status )
	(void)sprintf( outbuf, ERx("%-24.24s [%d.%03d] %s : "),
		 ctime(&errtime), ThePID, instance, 
		 get_err_name( status, scratchbuf ) );
    else
	(void)sprintf( outbuf, ERx("%-24.24s [%d.%03d] : "),
		 ctime(&errtime), ThePID, instance );
    len = strlen(outbuf);

    (void)vsnprintf( outbuf + len, sizeof(outbuf) - (len + 1), fmt, ap );
    SIfprintf( stderr, ERx("%s\n"), outbuf );
    SIflush( stderr );
}


/*
**  Empty function for ease in setting break points
**  to catch sever errors when using a debugger.
*/
static void
bad_error( i4 lineno, i4 self, STATUS status )
{
    report_error( self, status, "Unexpected error from line %d", lineno );
}



/* If we get semaphore errors, do simple decoding */

static char *
mapsemerr( i4 rv )
{
    switch( rv )
    {
    case E_CS0017_SMPR_DEADLOCK:
	return ERx("E_CS0017_SMPR_DEADLOCK");

    case E_CS000F_REQUEST_ABORTED:
	return ERx("E_CS000F_REQUEST_ABORTED");

    case E_CS000A_NO_SEMAPHORE:
        return ERx("E_CS000A_NO_SEMAPHORE");

    default:
	break;
    }
    return "";
}


/* Report semaphore error */

static void
semerr( i4 rv, CS_SEMAPHORE *sp, char *msg )
{
    MY_SCB *scb;

    CSget_scb( (CS_SCB **)&scb );
    (void)Psem( &SIsem );
    SIfprintf(stderr, ERx("%s %p returned %d (%s)\n"),
		msg, sp, rv, mapsemerr(rv) );
    SIfprintf(stderr, ERx("\tSCB instance %d, scb %p\n"), scb->instance, scb );
    SIfprintf(stderr, ERx("\t%p->cs_value %d\n"), sp, sp->cs_value );
    SIfprintf(stderr, ERx("\t%p->cs_count %d\n"), sp, sp->cs_count );
    SIfprintf(stderr, ERx("\t%p->cs_owner %x\n"), sp, sp->cs_owner );
    SIfprintf(stderr, ERx("\t%p->cs_next %p\n"), sp, sp->cs_next );
# ifdef OS_THREADS_USED
    SIfprintf(stderr, ERx("\t%p->cs_thread_id %d\n"), sp, sp->cs_thread_id );
    SIfprintf(stderr, ERx("\t%p->cs_pid %d\n"), sp, sp->cs_pid );
# endif /* OS_THREADS_USED */
    SIflush( stderr );
    (void)Vsem( &SIsem );
}

/* request and wait for semaphore */
i4
Psem( CS_SEMAPHORE *sp )
{
    i4  rv;

    if( rv = CSp_semaphore( TRUE, sp ) )
	semerr( rv, sp, ERx("Psem") );
    return( rv );
}

/* release a semaphore */
i4
Vsem( CS_SEMAPHORE *sp )
{
    i4  rv;

    if( rv = CSv_semaphore( sp ) )
	semerr( rv, sp, ERx("Vsem") );
    return( rv );
}


/* ------------- MSG sub-system routines -------------- */

static bool
message_handler( CX_MSG *pmessage, PTR pudata, bool invalid )
{
    bool	 done = TRUE;
    TEST_MSG	*ptmsg = (TEST_MSG*)pmessage;
    time_t	 bintim;

    /*
    ** How do we handle data marked invalid?
    **
    ** Here we don't, but in a real application, we would need logic
    ** to recalculate the message value, IF we are maintaining state
    ** information in the message block.
    */
    (void)time( &bintim );
    switch ( ptmsg->action )
    {
    case TM_ADD_NODE:
	/* Increment psuedo-join count */
	if ( 0 == SignedIn )
	{
	    /* Init value */
	    SignedIn = ptmsg->datum - 1;
	}
	SIfprintf( stderr, ERx("%-24.24s Received ADD. Expected %d, got %d.\n"),
			ctime( &bintim ), (SignedIn + 1), ptmsg->datum );
	if ( (SignedIn + 1) == ptmsg->datum )
	{
	    SignedIn = ptmsg->datum;
	    done = FALSE;
	}
	else
	    BAD_ERROR( 0, -99 );
	break;

    case TM_DEL_NODE:
	/* Decrement psuedo-join count */
	if ( 0 == SignedIn )
	{
	    /* Init value */
	    SignedIn = ptmsg->datum + 1;
	}
	SIfprintf( stderr, ERx("%-24.24s Received DEL. Expected %d, got %d.\n"),
			ctime( &bintim ), (SignedIn - 1), ptmsg->datum );
	if ( (SignedIn - 1) == ptmsg->datum )
	{
	    SignedIn = ptmsg->datum;
	    done = FALSE;
	}
	else
	    BAD_ERROR( 0, -98 );
	break;

    case TM_END_NODE:
	/* End of test */
	SIfprintf( stderr, 
	 ERx("%-24.24s Received END message.\n"), ctime( &bintim ) );
	break;

    default:
	SIfprintf( stderr,
	 ERx("Received unknown message type. Type = %d, data = %d.\n"),
			ptmsg->action, ptmsg->datum );
	BAD_ERROR( 0, -97 );
	break;
    } /* end-switch */

    SIflush( stderr );

    if ( Delay )
	(void)usleep( Delay );

    return done;
}


static STATUS
message_send( CX_MSG *curval, CX_MSG *newval, PTR pudata, bool invalid )
{
    TEST_MSG    *ptmsgi = (TEST_MSG*)curval;
    TEST_MSG    *ptmsgo = (TEST_MSG*)newval;
    TEST_MSG	*ptmsgu = (TEST_MSG*)pudata;
    time_t	 bintim;

    (void)time( &bintim );

    ptmsgo->action = ptmsgu->action;
    switch ( ptmsgu->action )
    {
    case TM_ADD_NODE:
	/* Tell other's I'm joining */
	ptmsgo->datum = ptmsgi->datum + 1;
	SIfprintf( stderr, ERx("%-24.24s Sending ADD message %d.\n"),
	  ctime( &bintim ), ptmsgo->datum );
	break;
    case TM_DEL_NODE:
	/* Tell other's I'm leaving */
	ptmsgo->datum = ptmsgi->datum - 1;
	SIfprintf( stderr, ERx("%-24.24s Sending DEL message.\n"),
	  ctime( &bintim ), ptmsgo->datum );
	break;
    case TM_END_NODE:
	/* Tell other's test is over */
	SIfprintf( stderr, ERx("%-24.24s Sending END message.\n"),
	  ctime( &bintim ) );
	ptmsgo->datum = 0;
	break;
    default:
	BAD_ERROR( 0, -96 );
	break;
    } /* end-switch */
    SIflush( stderr );
    return OK;
}

/* ---------------- actual application ---------------- */

static STATUS
test_startup(void)
{
    STATUS	status;
    PTR		pshm;

    Start_Time = time(NULL);

    SIfprintf( stderr, ERx("%s: Starting run on %s, PID = %d at %s\n"),
	    ProgName, NodeName, ThePID, ctime(&Start_Time));
# if defined (VMS) /* _VMS_ */
    if ( !CXcluster_support() )
    {
	(void)CXalter( CX_A_ONE_NODE_OK, (PTR)1 );
	SIfprintf( stderr,
	    ERx("%s: *** Note %s is NOT part of a hardware cluster ***\n"),
		    ProgName, NodeName);
    }
# endif /* _VMS_ */
    SIflush( stderr );

    {
	CL_ERR_DESC	syserr;
	SIZE_TYPE	pages, pagesgot;
	i4		meflags;

	meflags = ME_MSHARED_MASK | ME_IO_MASK |
		  ME_CREATE_MASK | ME_NOTPERM_MASK | ME_MZERO_MASK;

	pages = (sizeof(CX_NODE_CB) + ME_MPAGESIZE - 1) / ME_MPAGESIZE;
	status = MEget_pages( meflags, pages, ERx("cxmsgtst.mem"), &pshm,
		    &pagesgot, &syserr );
	if ( ME_ALREADY_EXISTS == status )
	{
	    report_error( 0, 0, "Reusing shared memory!" );
	    status = MEget_pages( ME_MSHARED_MASK | ME_IO_MASK, 
				  pages, ERx("cxmsgtst.mem"), &pshm,
				  &pagesgot, &syserr );
	}
	if ( OK != status )
	{
	    report_error( 0, status, "MEget_pages failure.  flags=%0x",
			  meflags );
	    return status;
	}
    }

    /* 1st assume you're a CSP, then try again if role already taken. */
    status = CXinitialize( ERx("XY"), pshm, CX_F_IS_CSP );
    if ( E_CL2C15_CX_E_SECONDCSP == status )
    {
        (VOID)CXalter( CX_A_NEED_CSP, (PTR)1 );
	(void)CXalter( CX_A_ONE_NODE_OK, (PTR)1 );
	status = CXinitialize( ERx("XY"), pshm, 0 );
	Is_CSP = 0;
    }
    if ( status )
    {
	report_error( 0, status,
		      ERx("CXinitialize failure. Is_CSP = %d."), Is_CSP );
    }
    else
    {
	status = CXjoin( 0 );
	if ( status )
	{
	    report_error( 0, status, ERx("CXjoin failure.") );
	}
    }

    return status;
}


static void
test_shutdown(void)
{
    STATUS	status;

    status = CXterminate( 0 );
    if ( status )
    {
	report_error( 0, status,
		      ERx("CXterminate failure. Is_CSP = %d.\n"), Is_CSP );
    }

    End_Time = time(NULL);

    /* Print final report */

    SIfprintf( stderr,
     ERx("%s: Finished at %-24.24s, elapsed time = %d seconds.\n\n"),
     ProgName, ctime(&End_Time), End_Time - Start_Time );
    SIflush( stderr );
}



/*
**	Setup global variables and add each thread.
**	Called by CSinitialize before threads are started
*/
static STATUS
hello( CS_INFO_CB *csib )
{
    STATUS stat;
    i4		i;
    CL_SYS_ERR	err;

# if !defined(VMS) /* _VMS_ */
    (void)signal(SIGSEGV, SIG_DFL);
    (void)signal(SIGBUS, SIG_DFL);

    /* MCTFIX - don't wipeout pplib's handler
    (void)signal(SIGSEGV, SIG_DFL);
    */
    (void)signal(SIGBUS, SIG_DFL);
# endif /* _VMS_ */

    (void)CSi_semaphore(&Miscsem, CS_SEM_SINGLE);
    (void)CSi_semaphore(&SIsem, CS_SEM_SINGLE);
 
    if ( test_startup() )
    {
	(void)CSterminate( CS_KILL, (i4 *)NULL );
    }

    for( i = 0, stat = OK; i < (1 + Receive) && stat == OK; i++ )
    {
	stat = CSadd_thread( 0, (PTR)NULL, i + 1, (CS_SID*)NULL, &err );
    }

    (void)CSi_semaphore(&sc0m_semaphore, CS_SEM_SINGLE);
 
    csib->cs_mem_sem = &sc0m_semaphore;

    return( stat );
}


/* Called when all thread functions return from the TERMINATE state */
static STATUS
bye(void)
{
    test_shutdown();

    /* semaphore needed, called when all threads are dead */
    if( Noisy )
    {
	SIfprintf(stderr, ERx("All threads are dead.  World ends.\n") );
	SIflush( stderr );
    }

# if !defined(VMS) /* _VMS_ */
    /* if we allocated the sytem segment, clear up now. */
    if( MySysSeg )
	(void)CS_des_installation();
# endif /* _VMS_ */

    return( OK );
}


/*
** Generic thread function, called out of CSdispatch 
*/
static STATUS
threadproc( i4 mode, CS_SCB *csscb, i4 *next_mode )
{
    MY_SCB *scb = (MY_SCB *) csscb;
    STATUS status;

    switch( mode )
    {
    case CS_INITIATE:	/* A new thread is born */

	/*
	** Invoke Test code here
	*/
	scb->instance = adj_counter(&Start_Threads,1) - 1;
	(void)adj_counter(&End_Threads,1);
	Thread_Status[scb->instance] = mode;
	CSget_sid( &SIDs[scb->instance] ); 

	if ( Receive )
	{
	    if ( scb->instance )
	    {
		status = CXmsg_connect( 0, message_handler, NULL );
		if ( status )
		{
		    report_error( 0, status,
				  ERx("CXmsg_connect failure.\n") );
		}
	    }
	    else
	    {
		status = CXmsg_monitor();
		if ( status )
		{
		    report_error( 0, status,
				  ERx("CXmsg_monitor failure.\n") );
		}
	    }
	}
	else
	{
	    i4	i;

	    for ( i = 0; i < Iterations; i++ )
	    {
		if ( Mixed )
		    TestMsg.action = (rand() & 0x1) ? TM_ADD_NODE : TM_DEL_NODE;
	        status = CXmsg_send( 0, NULL, message_send, (PTR)&TestMsg );
		if ( OK != status )
		{
		    report_error( 0, status,
				  ERx("CXmsg_send failure.\n") );
		    break;
		}
		if ( Delay )
		    (void)usleep( Delay );
	    }
	}

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	break;

    case CS_TERMINATE:
	*next_mode = CS_TERMINATE;
	(void)CSterminate( CS_KILL, (i4 *)NULL );
	break;
    }
    return( status );
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

    *(MY_SCB **)newscb = (MY_SCB*) calloc( 1, sizeof( **newscb ) );
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

static void
usage( char *badparm )
{
    if ( badparm )
	SIfprintf( stderr, ERx("\n%s: Bad parameter! (%s)\n"), ProgName,
		   badparm );
    else
	SIfprintf( stderr, ERx("\nCX MSG facility stress test program.\n") );
	
    SIfprintf( stderr,
	ERx("\n\t%s [-]add [<i>] | [-]del [<i>] | [-]mix [<i>] |" \
	    "[-]end | [-]receive\n\t\t[ [-]wait <usecs> ]  [-verbose]\n\n"),
	   ProgName );
    SIflush( stderr );
    PCexit(-1);
}

/*
** 'basename' is broken in 7.3 VMS CRTL.  Use clunky LO functions to 
** extract file portion of execuatble name.  Made a separate function
** to keep largish transient buffers off the stack.
*/
static void
set_prog_name( char *fullprogspec )
{
    LOCATION	loc;
    char        dev[LO_DEVNAME_MAX + 1];
    char        path[LO_PATH_MAX + 1];
    char        fprefix[LO_FPREFIX_MAX + 1];
    char        fsuffix[LO_FSUFFIX_MAX + 1];
    char        version[LO_FVERSION_MAX + 1];

    ProgName = ERx("cxtest");
    if ( fullprogspec && *fullprogspec )
    {
	/* Copy to user's space */
	fullprogspec = STalloc(fullprogspec);
	if ( fullprogspec )
	{
	    LOfroms( 0, fullprogspec, &loc );
	    if ( OK == LOdetail(&loc, dev, path, fprefix, fsuffix, version) )
	    {
		STcopy( fprefix, fullprogspec );
		ProgName = fullprogspec;
	    }
	}
    }
}

/* 
** Name: main	- Entry point to cxtest.
**
** Description:
**
**	Parse parameters, and setup for execution of multiple
**	sessions.
**
** Inputs:
**
**	argc 		Count of # of arguments.
**
**	argv		Vector of argument pointers, with argv[0]
**			the program, and argv[ 1 .. n ] the
**			command line parameters.
**
** Usage:
**
**	cxmsgtest [-]add [<i>] | [-]del [<i>] | [-]mix [<i>] | [-]end | \
**		  [-]receive    [ -wait <usec> ]  [-verbose]
**
**	Main options are all mutually exclusive.
**	Option 'receive' will make program catch messages.
**	Options 'add' and 'del' will inc. or dec. message datum.
**	Option 'mix' will randomly choose between 'add' & 'del'.
**	Option 'end' will terminate all receivers.
**	Options 'add', 'del', and 'mix' accept an iteration count
**	between 1 and 1000000 (default = 1).
**	A wait between messages can be specified with 'wait', arg is
**	in milliseconds. (default = 0 micro-seconds).
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- Normal successful completion.
*/
i4
main( i4 argc, char *argv[] )
{
    CS_CB 	cb;
    i4  	rv;
    i4  	f_argc;
    char 	**f_argv,*f_arr[3];
    CL_ERR_DESC	syserr;
    struct utsname nodeinfo;
    STATUS	status;
    char	*argp, **argh;
    i4		verbose = 0;

    set_prog_name( argv[0] );

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    if ( argc < 2 || argc > 5 )
	usage( NULL );

    argh = argv;
    argp = *(++argh);
    if ( '-' == *argp ) argp++;

    switch ( *argp )
    {
    case 'a':
	TestMsg.action = TM_ADD_NODE;
	break;
    case 'd':
	TestMsg.action = TM_DEL_NODE;
	break;
    case 'e':
	TestMsg.action = TM_END_NODE;
	break;
    case 'h':
	usage(NULL);
	break;
    case 'm':
	TestMsg.action = TM_ADD_NODE;
	Mixed = 1;
	break;
    case 'r':
	Receive = 1;
	break;
    default:
	usage( argp );
    }

    argc -= 2;

    if ( argc )
    {
	argp = *(++argh);
	argc --;
	if ( '-' == *argp ) argp++;
	if ( 'w' == *argp )
	{
	    if ( (0 == argc) || 
	         (OK != CVan(*++argh,&Delay)) || 
		 (Delay < 0) || (Delay >= 1000000) )
		usage(argp);
	    argc --;
	}
	else if ( 'v' == *argp )
	{
	    verbose = 1;
	}
	else if ( !Receive && TestMsg.action != TM_END_NODE )
	{
	    if ( OK != CVan( argp, &Iterations ) ||
		 Iterations < 1 || Iterations > 1000000 )
		usage(argp);
	}
	else
	{
	    usage(argp);
	}
    }
	
    if ( argc == 2 )
    {
	argp = *(++argh);
	if ( '-' == *argp ) argp++;
	if ( 'w' == *argp )
	{
	    if (  (OK != CVan(*++argh,&Delay)) || 
		 (Delay < 0) || (Delay >= 1000000) )
		usage(argp);
	    argc -= 2;
	}
    }

    if ( argc != 0 )
	usage(NULL);

    ThePID = getpid();

    (void)uname(&nodeinfo);
    (void)memcpy( NodeName, nodeinfo.nodename, sizeof(NodeName) - 1 );

    (void)MEadvise( ME_INGRES_ALLOC );

    (void)TRset_file(TR_F_OPEN, verbose ? TR_OUTPUT : TR_NULLOUT, 
                                verbose ? TR_L_OUTPUT : TR_L_NULLOUT,
				&syserr);

    (void)PMinit();

    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);

# if !defined(VMS) /* _VMS_ */
    /* default some signals */
    (void)signal(SIGSEGV, SIG_DFL);
    (void)signal(SIGBUS, SIG_DFL);
# endif /* _VMS_ */

    /* setup big CS parameter block to set stuff up */
    cb.cs_scnt = 3;	/* Number of sessions */
    cb.cs_ascnt = 0;		/* nbr of active sessions */
    cb.cs_stksize = (8 * 1024);	/* size of stack in bytes */
    cb.cs_process = threadproc;	/* Routine to do major processing */
    cb.cs_stkcache = FALSE;     /* Match csoptions default - no stack caching */
    cb.cs_scballoc = newscbf;	/* Routine to allocate SCB's */
    cb.cs_scbdealloc = freescb;	/* Routine to dealloc  SCB's */
    cb.cs_saddr = fnull1;	/* Routine to await session requests */
    cb.cs_reject = fnull1;	/* how to reject connections */
    cb.cs_disconnect = vnull;	/* how to dis- connections */
    cb.cs_read = rwnull;	/* Routine to do reads */
    cb.cs_write = rwnull;	/* Routine to do writes */
    cb.cs_attn = fnull2;	/* Routine to process attn calls */
    cb.cs_elog = logger;	/* Routine to log errors */
    cb.cs_startup = hello;	/* startup the server */
    cb.cs_shutdown = bye;	/* shutdown the server */
    cb.cs_format = fnull3;	/* format scb's */
    cb.cs_get_rcp_pid = pidnull; /* init to fix SEGV in CSMTp_semaphore*/
    cb.cs_scbattach = vnull;	/* init to fix SEGV in CSMT_setup */
    cb.cs_scbdetach = vnull;	/* init to fix SEGV in CSMT_setup */
    cb.cs_format_lkkey = NULL;  /* Not used in this context. */

    /* now fudge argc and argv for CSinitiate */
    f_argc = 2;
    f_arr[0] = argv[0];
    f_arr[1] = ERx("-nopublic");
    f_arr[2] = NULL;
    f_argv = f_arr;

# if !defined(VMS) /* _VMS_ */
    /* Additional wakeup block needed by idle thread replacement */
    if(CS_create_sys_segment(32, 32 * 3, &syserr) != OK)
    {
	SIfprintf( stderr, ERx("Warning: couldn't create system segment.\n") );
	MySysSeg = FALSE;
    }
# endif /* _VMS_ */

    if ( status = CSinitiate( &f_argc, &f_argv, &cb ) )
    {
	SIfprintf( stderr, ERx("CSinitiate returns %d\n"), status );
	SIflush( stderr );
	PCexit(FAIL);
    }

    /* now do everything that has been set up */
    rv = CSdispatch();

    /* should not get here */
    SIfprintf( stderr, ERx("CSdispatch returns %d (%x)\n"), rv, rv );
    SIflush( stderr );

    PCexit( FAIL );
}

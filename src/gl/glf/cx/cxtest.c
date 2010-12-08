/*
** Copyright(c) 2001, 2010 Ingres Corporation
**
**
**
PROGRAM  = cxtest

NEEDLIBS = COMPAT MALLOCLIB
** Description:
**
**	Program is intended to exercise the CX DLM, CX CMO, and to
**	a lesser extent the CX MSG subsystems in a way very similar
**	to the way CX is used by the Ingres DBMS.
**	
**	Basic idea is that one or more instances of this program can be run,
**	with each instance running multiple threads.  MSG calls are used to
**	assure runs involving multiple instances are synchronized.  Each
**	instance of the program will have a report thread which coordinates
**	the local test threads, and reports results, a message monitor, and
**	message thread to support the MSG system, and two or more test threads.
**	
**	Test threads perform a zero sum game where the thread debits a resource
**	("steals") from other "gangs" and credits it to its gang.  Current
**	resource counts are held in CMO objects, and the value block for each
**	gang is used to track the number of thefts its committed, and the number
**	of times it has been victimized.   Constant cross checking is done to
**	assure that update atomicity is respected.
**	
**	There are options to control the number of instances, the number
**	of threads, and the number of iterations, as well as the access pattern
**	(round-robin (default), random, or patsy (each gang only steals from one
**	other gang)), and whether locks are acquired in an order guaranteed to
**	avoid deadlock (default), if deadlock is allowed, and if locks
**	are taken in process context as opposed to a group context.
**
**      If "seq" option is specified, cxtest will also exercise a global
**      sequence number counter.
**
**      If "lsn" option is specified, cxtest will also simulate the LSN
**      synchronization logic used by the DBMS. (default)
**
**	"nolsn" will completely suppress LSN and transaction log tests.
**
**	Unless "nolsn" is specified, a virtual transaction log is maintained
**	for each test participant.  During the test run these are kept 
**	in memory, but at the end of the run these files are written to disk
**	with the name(s) $II_SYSTEM/ingres/files/cxtest_<P1><N1>_<P2><N2>.log,
**	where <P1> and <N1> are the PID & Node number of the cxtest
**	instance owning the 1st CSP role, and <P2> & <N2> are the PID
**	and node number of the instance writing the log.  (N.B.) "master"
**	cxtest, has an optimization where it skips this step.
**
**	The Master cxtest then merges all the logs, and confirms there
**	were no ACID violations.  Log files are removed, but if -dumplog
**	is specified, or an error is detected, a plain text version of 
**	the "transaction" log is written out for diagnostic purposes.
**
** History:
**	06-feb-2001 (devjo01)
**	    Created from csphil.c
**	03-oct-2002 (devjo01)
**	    Set NUMA context.
**      05-Jan-2004 (hanje04)
**          Replace _SYS_NMLN with SYS_MNLN as _SYS_NMLN is not
**          defined on later versions of Linux.
**	29-jan-2004 (devjo01)
**	    In test_thread, obtain NL lock on self prior to
**	    suspending pending resume by report thread.  This
**	    guards against a potential race situation in which
**	    one test thread requests a lock on, updates the LVB for,
**	    and releases a lock on another "gang" bfore that gang
**	    had an opportunity to get the lock at all.  This would
**	    lead to a spurious apparent lost update to a LVB.
**	04-feb-2004 (devjo01)
**	  - Always display histogram after intervals.
**	  - Add an CScancelled before final CXmsg_send call to
**	    consume any pending resume from test threads.
**	  - Correct reported program instances calculation
**	    in "Waiting for ... to finish" message.
**	  - Remove LNX conditional code added in last rev.
**	20-feb-2004 (devjo01)
**	    Add support for -proc_context flag.
**	29-feb-2004 (devjo01)
**	    Put in some paranoid code to handle non-synchronous grant
**	    on a down-conversion to NL (which really should always
**	    be granted synchronously).
**	22-mar-2004 (devjo01)
**	    Prevent cxtest from hanging on startup failure by removing
**	    call to CSterminate from 'hello()'.  CSterminate called here
**	    will block on CsMultiTaskSem.  Reorganized code to simply
**	    pass up failure indicator.  CSdispatch will catch failure
**	    and perform shutdown.
**	4-apr-2004 (mutma03)
**	    Added code to signal the thread which is suspended indefinitely
**	    after missing AST due to suspend/resume race condition.
**	2-sep-2004 (mutma03)
**	    Removed code to signal the thread which is suspended indefinitely
**	    after missing AST due to suspend/resume race condition.
**	    The condition would not arise with
**	    CSsuspend_for_AST/CSresume_from_AST.
**      20-sep-2004 (devjo01)
**	    Cumulative changes for Linux beta.
**          - Add code simulating obtaining a synchronized LSN using two
**          different methods.  
**	    - Add psuedo transaction log and checks on same.
**          - Enforce use of consistent parameters if multiple instances
**          cxtest are run concurrently.
**          - No longer need to specifiy "instances" parameter (default = 1).
**	    Non-numeric parameters can be specified alone, or in any order.
**          - If a cxtest is already in progress, subsequent starts get
**          their parameters from whichever cxtest started running
**          first ("master" cxtest).
**          - DO Not allocate "sysseg" ourselves.  csinstall must be run 1st.
**          - Correctly manage "cxtest.mem" shared memory seg if more than
**          one cxtest instance is run.
**	    - Edit sloppy/goofy comments.
**	03-nov-2004 (thaju02)
**	    Change CX_Seg_Pages, pages to SIZE_TYPE.
**	04-mar-2005 (devjo01)
**	    Fix stoopid errors in parameter parsing which caused -help
**	    to be ignored, and bad parameters to pass without comment.
**      12-apr-2005 (devjo01)
**          Use new CX_MSG struct instead of overloading CX_CMO so as to
**          allow synchronized short messages of 32 bytes.
**	20-apr-2005 (devjo01)
**	    Port to VMS.
**	24-jul-2005 (devjo01)
**	    cx_key is now a structure.
**	08-Feb-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**	20-Jan-2010 (hanje04)
**	    SIR 123296
**	    Add support for LSB builds, writable files go to ADMIN and logs
**	    got to LOG instead of everything going to files.
**      03-nov-2010 (joea)
**          Declare local functions static.
**      29-Nov-2010 (frima01) SIR 124685
**          Removed prototypes that reside in gl.h now.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Fix CS assignments for new prototypes.
*/

# include <stdarg.h>
# include <sys/utsname.h>
# include <time.h>
# include <libgen.h>
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
# include <lo.h>
# include <pm.h>
# include <cv.h>
# include <st.h>
# include <nm.h>
# include <cx.h>
# include <cxprivate.h>

/* For dump_all */
# if defined(VMS) /* _VMS_ */
# include <exhdef.h>
# endif /* _VMS_ */
# include <csinternal.h>

# define MAX_INSTANCES	 16	/* Max # of concurrent program invocations */
# define MAX_GANGS	( 2 * ( CX_MAX_CMO - CX_CMO_RESERVED ) )
# define MAX_N		100	/* Max number of test threads */
# define THR_EXTRA	  3	/* Utility threads */
# define CFG_CMO          0     /* CMO used for configuration info */

# ifdef OS_THREADS_USED
# define THR_SLOP	2
# else
# define THR_SLOP	1
# endif

GLOBALDEF       CS_SEMAPHORE    sc0m_semaphore; /* mem sem expected by SC */

extern  VOID CS_swuser(void);

# if defined(VMS) /* _VMS_ */
/*
** FIX-ME. 7.3 RTL does not support vsnprintf, use unsafe alternative
** which is not performing bounds check for a buffer overflow. (Not
** that we expect a buffer overflow, but paranoia is good practice.)
*/
# define vsnprintf(b,n,f,p) vsprintf(b,f,p)
# endif /* _VMS_ */

/* for extended debugging */
GLOBALREF CS_SYSTEM           Cs_srv_block;

GLOBALREF CX_PROC_CB	 CX_Proc_CB;

GLOBALREF CX_STATS CX_Stats;

/*
**  Test config parameters, with default settings.
*/

/* Init to non-zero for verbose version */
i4  Noisy = 0;

/* Identifiers for test group, and individual contributors. */
i4	TestID, MyID;

i4	LocalNode;

/* number of concurrent instances of this program expected. */
i4	Instances = 0;
i4	SignedIn = 0;
bool	StartRun = FALSE;
i4	MyInstance;
i4	KnownInstances[MAX_INSTANCES] = { 0, };

/* number of threads wanted */
i4	Num_Threads = MAX_GANGS;
i4	Num_Gangs = MAX_GANGS;

/* How many times for each session to loop */
i4  Iterations = MAX_GANGS * 1000;

/* Total number of actions this program instance (Iterations * Num_Threads) */
i4  Transactions;

/* Suppress possibility of deadlock? */
i4  NoDeadlocks = 1;

# if defined(LNX) || defined(axp_osf)
    /*
    ** On platforms which support a group lock container
    ** by default get locks in the Group Context
    */
#   define DFLT_LOCK_CONTEXT	0
# else
    /*
    ** On platforms which do not support a group lock
    ** container explicitly request process context.
    */
#   define DFLT_LOCK_CONTEXT	CX_F_PCONTEXT
# endif

i4  LockContextFlag = DFLT_LOCK_CONTEXT;

/* Set if LSN testing enabled */
# define LSN_NONE       0
# define LSN_SEQ        1
# define LSN_SYNCH      2
i4  LSNstyle = LSN_SYNCH;

# define START_LSN   1000

# define LSN_LE(a,b) \
    (((a)->cx_lsn_high < (b)->cx_lsn_high) || \
     (((a)->cx_lsn_high == (b)->cx_lsn_high) && \
     ((a)->cx_lsn_low <= (b)->cx_lsn_low)))

char *LSN_Style_Names[] = { "none", "sequential", "synchronized" };

char *CMO_Style_Names[] = { "none", "DLM", "IMC", "SNC" };

CX_LSN  StartLSN;

i4	LK_LSN_Flag = 0;

/*
**	Log record for psuedo transaction log. 
*/
# define LOG_REC_FORMAT "%04x.%08x %2d %8d %7d %3d %2d %8d %2d %8d %04x.%08x\n"

typedef struct logrec {
    CX_LSN	lsn;	/* This MUST reflect action chronology. Rigorous
    			** Tests are done at test wrap up to assure this
			** is the case.
			*/
    i4		node;	/* Used by log check to assure two nodes don't
			** Generate the same LSN.
			*/
    i4		ident;	/* For diagnostics. "MyId" for record creator. */
    i4		index;	/* For diagnostics. Index of logrec in creators
    			** LogBuffer.
			*/
    i4		thug;	/* Thread which created the record. */
    i4		tgang;	/* Gang thread belongs to. */
    i4		tbal;	/* Thread's balance after logged action. */
    i4		victim; /* Which gang victimized. */
    i4		vbal;	/* Victim gang's balance after logged action. */
    CX_LSN      synclsn;/* For diagnostics. Gsync value visible to this
    			** thread at time log record was created.
			*/
} LOGREC;

FILE	*LogFile;	/* File pointer to write log file to disk */
LOGREC	*LogBuffer;	/* Pointer to in-memory log file */
char	*LogPath;	/* Full path to local log name */
i4	 LogCur;	/* Current index into logBuffer */
i4	 DumpLog = 0;	/* Dump merged log as plain text */

# define	ERR_MSG_MAX_LEN		256
# define	ERR_ABORT_LIMIT	1000 /* Stop checking after this many errors */
# define	ERR_DISP_LIMIT	10   /* Stop displaying errors after this */

char	LogErrMsgBuf[ ((ERR_MSG_MAX_LEN+1) * ERR_DISP_LIMIT) + 1];
char	*LogErrMsgPtr = LogErrMsgBuf;

/* Lock request time out.  0 = no time out. */
i4  Time_Out = 0;

/*
** This is the only place we can keep the type passed in
** by CSadd_thread for 6.1 CS, so we create a whole new type.
*/
typedef struct
{
	CS_SCB	cs_scb;		/* the "semi-opaque" system scb */
	i4	instance;	/* my data */
} MY_SCB;

#define SELF_RCB	0
#define VICT_RCB	1
CX_REQ_CB	ReqCBs[MAX_N][2] = { 0 };

LK_LOCK_KEY	ResKeys[MAX_GANGS] = { 0 };

/* Frequency count for one result code */
typedef struct st_res_stat {
        int     rs_status_code;
        int     rs_occurances;
}               ST_RES_STAT;

/* Basic result statistics */
typedef struct st_res_stats {
        int             rs_inx;
#       define          ST_MAX_STATS    8
#       define          ST_RES_STAT_JUMBLE 9999
        ST_RES_STAT     rs_stats[ST_MAX_STATS];
}               ST_RES_STATS;

#define REQ_STATS	0
#define CONV_STATS	1
#define DOWN_STATS	2
#define WAIT_STATS	3
#define CMPL_STATS	4
#define RLSE_STATS	5
#define NUM_OF_STATS	6

#define ERROR_LIMIT	10	/* # of severe errs a session will tolerate */

char	* ResStatNames[NUM_OF_STATS] = { "Request", "Conversion up", 
		"Conversion down", "Wait", "Completion", "Release" };


/* Variables used to control Reporting thread. */
#define MAX_NUM_BREAKS	10	/* Maximum # of report breaks */

i4	Num_Breaks;			/* # of report breaks */
i4	Next_Break[MAX_NUM_BREAKS+1];	/* Array of breaks in iterations */ 
i4	Completion_Count[MAX_NUM_BREAKS]; /* How many have reached ea. break */
i4	Iterations_Done[MAX_N];		/* Each session, done so far. */
i4	Total_Thefts = 0;		/* Running theft count all thieves. */

CS_SID	SIDs[MAX_N+THR_EXTRA];		/* Session ID for each thread. */
CS_SID *pReportSID;			/* Ptr to SID for report thread. */

/* Session "application" data */

/* Array of result statistics */
ST_RES_STATS ResStats[MAX_N][MAX_NUM_BREAKS][NUM_OF_STATS];
HRSYSTIME    Timings[MAX_N][MAX_NUM_BREAKS+1];


/* Faux shared CX memory segment was allocated by me */
bool Created_CX_Seg = FALSE;

SIZE_TYPE      CX_Seg_Pages;

PTR     CX_SM_Seg = NULL;

/* status of test thread */
i4	Thread_Status[MAX_N+THR_EXTRA];

/* Count of the number of "living" threads */
i4  Start_Threads = 0;	/* Monotonicaly increasing counter */
i4  End_Threads = 0;	/* Running count of living threads */

# define	DEFAULT_TEST		0
# define	PATSY_TEST		1
# define	RANDOM_ORDER_TEST	2
i4  Test_Style = DEFAULT_TEST;

char *Test_Style_Names[] = {
    "round robin (default)",
    "patsy",
    "random order"
};

/* Protects SI calls */
CS_SEMAPHORE SIsem;
CS_SEMAPHORE Miscsem;

static char *OSThreadTypes[] = { "INTERNAL","OS" };

i4	     OSthreaded = -1;

i4	     Interrupted = 0;

i4	     Interactive = 0;

char	    *ProgName;

i4	     ThePID;

char         NodeName[CX_MAX_HOST_NAME_LEN+1] = { '\0' };

time_t       Start_Time, End_Time, Done_Time;

/* Dummy shared memory control block */
CX_NODE_CB	Cx_node_cb = { 0 };

/*
** Forward declarations
*/
static VOID logger( i4 errnum, CL_ERR_DESC *, i4, ... );

i4 Psem( CS_SEMAPHORE *sp );
i4 Vsem( CS_SEMAPHORE *sp );


/* ---------- Reporting & Helper Functions ------------ */

#define	BAD_ERROR( id, status ) bad_error( __LINE__, (id), (status) )

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
    char	scratchbuf[32];
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
    len = STlength(outbuf);
    (void)vsnprintf( outbuf + len, sizeof(outbuf) - (len + 1), fmt, ap );
    if ( instance != 0 ) (void)Psem( &SIsem );
    SIfprintf( stderr, ERx("%s\n"), outbuf );
    SIflush( stderr );
    if ( instance != 0 ) (void)Vsem( &SIsem );
}

/*
**  Specialized error routine for "log" file analysis.
*/
static void
report_log_error( i4 record, CX_LSN *lsn, char *fmt, ... )
{
    va_list	ap;
    i4		len;

    va_start( ap, fmt );
    
    (void)sprintf( LogErrMsgPtr, ERx("LOG ERR: Record %d LSN=%08.8x.%08.8x: "),
		   record, lsn->cx_lsn_high, lsn->cx_lsn_low );
    len = STlength(LogErrMsgPtr);
    (void)vsnprintf( LogErrMsgPtr + len, ERR_MSG_MAX_LEN - (len + 1), fmt, ap );
    SIfprintf( stderr, ERx("%s\n"), LogErrMsgPtr );
    SIflush( stderr );
    LogErrMsgPtr += STlength(LogErrMsgPtr) + 1;
}

/*
**  Empty function for ease in setting break points
**  to catch severe errors when using a debugger.
*/
static void
bad_error( i4 lineno, i4 self, STATUS status )
{
    report_error( self, status, "Unexpected error from line %d", lineno );
}

/*
**  Fill array with 0 .. count-1 in random order.
*/
static void
shuffle( i4 *deck, i4 count )
{
    i4		i, r, t;

    for ( i = 0; i < count; i++ )
	*(deck + i) = i;
    while ( i > 1 )
    {
	r = rand() % i;
	i--;
	t = *(deck + r);
	*(deck + r) = *(deck + i);
	*(deck + i) = t;
    }
}

/*
**  Add multiple results into statistics summary array.
*/
static void
sum_res_stat( ST_RES_STATS * rs, STATUS status, i4 qty )
{
    int     i;

    /* Search for status code in array */
    for ( i = 0; i < rs->rs_inx; i++ )
    {
	if ( rs->rs_stats[i].rs_status_code >= status )
	{
	    if ( rs->rs_stats[i].rs_status_code == status )
	    {
		rs->rs_stats[i].rs_occurances += qty;
		return;
	    }
	    break;
	}
    }

    /* New code, put it in array in status code order. */
    if ( rs->rs_inx < ST_MAX_STATS )
    {
	/* Room for more */
	if ( i < rs->rs_inx )
		(void)memmove( rs->rs_stats + i + 1,
			 rs->rs_stats + i,
			 (rs->rs_inx - i) * sizeof(ST_RES_STAT) );
	rs->rs_stats[i].rs_status_code = status;
	rs->rs_stats[i].rs_occurances = qty;
	rs->rs_inx++;
    }
    else
    {
	/* No room, add to catch all buffer */
	rs->rs_stats[ST_MAX_STATS-1].rs_occurances++;
	if ( rs->rs_stats[ST_MAX_STATS-1].rs_status_code != status )
	    /* Indicate last bucket is a jumble */
	    rs->rs_stats[ST_MAX_STATS-1].rs_status_code =
                                ST_RES_STAT_JUMBLE;
    }
} /* sum_res_stat */


/*
**  Add result into statistics array.
*/
static void
add_res_stat( STATUS status, i4 index, i4 interval, i4 stat )
{
    sum_res_stat( &ResStats[index][interval][stat], status, 1 );
    if ( !OSthreaded )
	CS_swuser();
} /* add_res_stat */


static void
dump_results( char *statdesc, ST_RES_STATS *rs )
{
    ST_RES_STAT     *st;
    char            *rcs;
    int             i, total;
    char	    scratchbuf[80];

    for ( i = total = 0, st = rs->rs_stats;
	  i < rs->rs_inx;
	  i++, st++ )
    {
	if ( 0 == st->rs_status_code )
	    rcs = ERx("Success");
	else if ( st->rs_status_code == ST_RES_STAT_JUMBLE )
	    rcs = ERx("Mixed catchall");
	else
	    rcs = get_err_name( st->rs_status_code, scratchbuf );

	SIfprintf( stderr, ERx("%-24s %7d %s\n"), 
		statdesc, st->rs_occurances, rcs );
	statdesc = "";
	total +=  st->rs_occurances;
    }
    if ( rs->rs_inx > 1 )
	SIfprintf( stderr, ERx("%24s %7d\n"), ERx("total"), total );
}

/* ------------------ Debugging aids ------------------ */

/*
** dump a session queue.
*/
static void
dump_queue( char *name, CS_SCB *start )
{
    CS_SCB	 *scb;
    i4		 i;

    if ( start )
    {
	(void)TRdisplay( ERx(" %s:"), name );
	for ( i = 1, scb = start->cs_rw_q.cs_q_next;
	      scb != start;
	      scb = scb->cs_rw_q.cs_q_next, i++ )
	{
	    if ( 3 == i )
	    {
		(void)TRdisplay( ERx("\n") );
	    i = 0;
	}
	(void)TRdisplay( ERx(" -> %p (%d)"),
	  scb, ((MY_SCB *)scb)->instance );
	}
	(void)TRdisplay( ERx("\n") );
    }
}

static void
dump_parameters( FILE *outfile )
{
    SIfprintf( outfile,
		ERx("%s: %d sessions organized into %d \"gangs\".\n"),
		ProgName, Num_Threads, Num_Gangs );
    SIfprintf( outfile, ERx("%s: Test Style = %s, Deadlocks %sallowed.\n"),
		ProgName, Test_Style_Names[Test_Style],
		NoDeadlocks ? ERx("dis") : "" );
    SIfprintf( outfile, ERx("%s: Locks taken in %s context.\n"),
		ProgName, LockContextFlag ? "Process" : "Group" );
    SIfprintf( outfile, ERx("%s: CMO configuration = %s.\n"),
		ProgName, CMO_Style_Names[CX_Proc_CB.cx_cmo_type] );
    SIfprintf( outfile, ERx("%s: LSN test style = %s.\n"),
		ProgName, LSN_Style_Names[LSNstyle] );
    SIflush( stderr );
}

/*
** dump information for all sessions. (called from debugger)
*/
static void
dump_all(void)
{
#   define	CVT_TO_ID(pscb) \
	((((pscb)==Cs_srv_block.cs_wt_list)|| \
	  ((pscb)==Cs_srv_block.cs_rdy_que[pscb->cs_priority])|| \
          ((pscb)==NULL)) ? -1 : ((MY_SCB *)(pscb))->instance)


    CS_SCB	*scb;
    i4		 thief, gang;

    (void)TRdisplay( ERx("**** CXTEST status summary ****\n") );
    (void)TRdisplay( ERx(" Iterations = %d, Num_Gangs = %d\n"),
     Iterations, Num_Gangs ); 
    (void)TRdisplay( ERx(" cs_state = %x, cs_ready_mask = %x\n"),
     Cs_srv_block.cs_state, Cs_srv_block.cs_ready_mask );
    dump_queue( "wait queue", Cs_srv_block.cs_wt_list );
    dump_queue( "rdy que[8]", Cs_srv_block.cs_rdy_que[8] );
    for ( thief = 0; thief < Num_Threads; thief++ )
    {
	gang = thief % Num_Gangs;
	(void)TRdisplay( ERx("*** SID:%p [ Thief %d in Gang %d ] ***\n"),
	 SIDs[thief], thief, gang );
	(void)TRdisplay( ERx(" Thread_Status = %d; %d iterations completed\n"), 
	 Thread_Status[thief], Iterations_Done[thief] );
	if ( CS_INITIATE == Thread_Status[thief] )
	{
	    scb = CSfind_scb( SIDs[thief] );
	    (void)TRdisplay( 
	     ERx(" cs_state = %w, cs_mask = %v\n prev=%p (%d), next=%p (%d)\n"),
	     cs_state_names, scb->cs_state, cs_mask_names, scb->cs_mask,
	     scb->cs_rw_q.cs_q_prev, CVT_TO_ID(scb->cs_rw_q.cs_q_prev),
	     scb->cs_rw_q.cs_q_next, CVT_TO_ID(scb->cs_rw_q.cs_q_next) );
	}
	if ( CX_NONZERO_ID( &ReqCBs[thief][SELF_RCB].cx_lock_id ) )
	{
	    (void)TRdisplay( ERx("** Lock for self ***\n") );
	    CXdlm_dump_both( NULL, &ReqCBs[thief][SELF_RCB] );
	}
	else
	{
	    (void)TRdisplay( ERx("** NO lock on self! **\n" ) );
	}
	if ( CX_NONZERO_ID( &ReqCBs[thief][VICT_RCB].cx_lock_id ) )
	{
	    (void)TRdisplay( ERx("** Lock on victim ***\n") );
	    CXdlm_dump_both( NULL, &ReqCBs[thief][VICT_RCB] );
	}
	else
	{
	    (void)TRdisplay( ERx("** NO lock on victim **\n" ) );
	}
    }
}


/* ------------- MSG sub-system routines -------------- */

typedef struct
{
#define TM_ADD_NODE 5
#define TM_DEL_NODE 6
    u_i4	action;
    u_i4	datum;
    u_i4	myid;
    u_i4	myinstance;
}	TEST_MSG;


static bool
message_handler( CX_MSG *pmessage, PTR pudata, bool invalid )
{
    static i4	 clueless = 1;

    bool	 done = TRUE;
    TEST_MSG	*ptmsg = (TEST_MSG*)pmessage;

    /*
    ** How do we handle data marked invalid?
    **
    ** Here we don't, but in a real application, we would need logic
    ** to recalculate the message value, IF we are maintaining state
    ** information in the message block.
    */
    if ( ptmsg->datum > MAX_INSTANCES )
    {
	BAD_ERROR( 0, -95 );
    }
    else
    {
	switch ( ptmsg->action )
	{
	case TM_ADD_NODE:
	    /* Increment psuedo-join count */
	    if ( 0 != ptmsg->datum && 
	         ( clueless || ( (SignedIn + 1) == ptmsg->datum ) ) )
	    {
		clueless = 0;
		SignedIn = ptmsg->datum;
		KnownInstances[ptmsg->datum - 1] = ptmsg->myid;
		if ( SignedIn == Instances )
		{
		    StartRun = TRUE;
		}
		done = FALSE;
	    }
	    else
		BAD_ERROR( 0, -99 );
	    break;
	case TM_DEL_NODE:
	    /* Decrement psuedo-join count */
	    if ( clueless || (SignedIn - 1) == ptmsg->datum )
	    {
		KnownInstances[ptmsg->myinstance - 1] = ptmsg->myid;
		clueless = 0;
		SignedIn = ptmsg->datum;
		done = FALSE;
	    }
	    else
		BAD_ERROR( 0, -98 );
	    break;
	default:
	    BAD_ERROR( 0, -97 );
	    break;
	} /* end-switch */
    }
    return done;
}


static STATUS
message_send( CX_MSG *curval, CX_MSG *newval, PTR pudata, bool invalid )
{
    TEST_MSG    *ptmsgi = (TEST_MSG*)curval;
    TEST_MSG    *ptmsgo = (TEST_MSG*)newval;
    TEST_MSG	*ptmsgu = (TEST_MSG*)pudata;

    ptmsgo->action = ptmsgu->action;
    ptmsgo->myid = MyID;

    switch ( ptmsgu->action )
    {
    case TM_ADD_NODE:
	/* Tell other's I'm joining */
	ptmsgo->myinstance = MyInstance = ptmsgi->datum + 1;
	ptmsgo->datum = MyInstance;
	break;
    case TM_DEL_NODE:
	/* Tell other's I'm leaving */
	ptmsgo->myinstance = MyInstance;
	ptmsgo->datum = ptmsgi->datum - 1;
	break;
    default:
	BAD_ERROR( 0, -96 );
	break;
    } /* end-switch */
    return OK;
}

/* -------------------- CMO support ------------------- */

typedef union
{
	CX_CMO		cmo;
	i4		halves[2];
}	TEST_CMO;

typedef struct
{
	i4		half;
	i4		balance;
}	AUX_DATA;

static i4
cmo_read( i4 id, i4 gang )
{
    STATUS	 status;
    i4		 cmo, half, balance;
    TEST_CMO	 accounts;

    cmo = CX_CMO_RESERVED + (gang / 2);
    half = gang % 2;

    status = CXcmo_read( cmo, &accounts.cmo );
    if ( OK != status )
    {
	report_error( id, status, ERx("CMO read error") );
        balance = -1;
    }
    else
	balance = accounts.halves[half];
    return balance;

}

static STATUS
cmo_set_acct_bal( CX_CMO *pold, CX_CMO *pnew, PTR paux, bool invalid )
{
    i4	half = ((AUX_DATA *)paux)->half;
    
    ((TEST_CMO *)pnew)->halves[half] = ((AUX_DATA *)paux)->balance;
    ((TEST_CMO *)pnew)->halves[!half] = ((TEST_CMO *)pold)->halves[!half];
    return OK;
}

static i4
cmo_update( i4 id, i4 gang, i4 newbalance )
{
    STATUS	 status;
    i4		 cmo, balance;
    AUX_DATA	 udata;
    TEST_CMO	 result;

    cmo = CX_CMO_RESERVED + (gang / 2);
    udata.half = gang % 2;
    udata.balance = newbalance;

    status = CXcmo_update( cmo, &result.cmo, cmo_set_acct_bal, (PTR)&udata );
    if ( OK != status )
    {
	report_error( id, status, ERx("CMO update error") );
        balance = -1;
    }
    else
	balance = result.halves[udata.half];
    return balance;
}


static void
confirm_interrupt( void )
{
    static i4	already_prompting = 0;
    i4		doprompt;
    char	ch, *str;

    if ( !Interrupted )
    {
	if ( Interactive )
	{
	    doprompt = 0;
	    (void)Psem( &SIsem );
	    if ( !Interrupted && !already_prompting )
	    {
		already_prompting = doprompt = 1;
	    }
	    (void)Vsem( &SIsem );

	    while ( doprompt )
	    {
		SIfprintf( stderr,
			  "User Interrupt received.  Exit program (y/n)? " );
                SIflush( stderr );
		ch = (char)getchar();

		switch ( ch )
		{
		    case 'Y':
		    case 'y':
			str = "yes";
			Interrupted = 1;
			doprompt = 0;
			break;

		    case EOF:
		    case 'N':
		    case 'n':
			str = "No";
			doprompt = 0;
			break;

		    default:
			str = "type 'Y' or 'N'";
		}
		SIfprintf( stderr, "%s\n", str );
                SIflush( stderr );
	    }
	    already_prompting = 0;
	}
	else
	{
	    SIfprintf( stderr,
		"Breaking due to user interrupt ...\n" );
            SIflush( stderr );
	    Interrupted = 1;
	}
    }
}

/* -------------------- Thread body functions ------------------- */

/*
** test_startup
**
** This code is called by CSdispatch to perform initialization.
**
** History:
**
**	29-jan-2004 (devjo01)
**	    Make resource keys more readable to humans.
**	09-feb-2004 (devjo01)
**	    Print test style and deadlock status.
*/

static i4
test_startup(void)
{
    STATUS	status;
    i4		i, j, k;
    LK_LOCK_KEY *lkkey;
    union {
        CX_CMO      cmo;
        CX_CMO_SEQ  seq;
        CX_LSN      lsn;
    }           lsnbuf;
    union {
        CX_CMO      cmo;
        struct {
            u_i4    iterations;
	    u_i4    testid;
            u_i1    instances;
            u_i1    threads;
            u_i1    teststyle;
            u_i1    lockcontext;
            u_i1    nodeadlocks;
            u_i1    lsnstyle;
        }           cfg;
    }           ConfigCMO;    
    i4          cspflag = CX_F_IS_CSP;

    Start_Time = time(NULL);

    SIfprintf( stderr, ERx("%s: Starting test on %s, PID = %d at %s\n"),
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

    do /* Something to break out of */
    {
        /*
        ** Create shared memory segment if Instances not explicitly
        ** set to '1'.  If completely unset, we may be the 1st "slave"
        ** instance of 'cxtest' on this node, and may be pulling it's
        ** parameters from the 1st CSP.
        */
        if ( Instances != 1 )
        {
            CL_ERR_DESC	syserr;
            SIZE_TYPE	pages;
	    i4		meflags;

            meflags = ME_MSHARED_MASK | ME_IO_MASK |
             ME_CREATE_MASK | ME_NOTPERM_MASK | ME_MZERO_MASK;

            pages = (sizeof(Cx_node_cb) + ME_MPAGESIZE - 1) / ME_MPAGESIZE;
            status = MEget_pages( meflags, pages, ERx("cxtest.mem"), &CX_SM_Seg,
                        &CX_Seg_Pages, &syserr );
            if ( ME_ALREADY_EXISTS == status )
            {
                report_error( 0, 0, "Psuedo CSP reusing shared memory!" );
                status = MEget_pages( ME_MSHARED_MASK | ME_IO_MASK, 
				  pages, ERx("cxtest.mem"), &CX_SM_Seg,
				  &CX_Seg_Pages, &syserr );
            }
            else if ( OK == status )
            {
                Created_CX_Seg = TRUE;
            }

            if ( OK != status )
            {
                report_error( 0, status, "MEget_pages failure.  flags=%0x",
                              meflags );
                break;
            }
        }
        else
        {
            CX_SM_Seg = (PTR)&Cx_node_cb;
        }

	/* 1st assume you're a CSP, then try again if role already taken. */
	status = CXinitialize( ERx("XY"), CX_SM_Seg, cspflag );
	if ( E_CL2C15_CX_E_SECONDCSP == status )
	{
	    (VOID)CXalter( CX_A_NEED_CSP, (PTR)1 );
	    (void)CXalter( CX_A_ONE_NODE_OK, (PTR)1 );
	    status = CXinitialize( ERx("XY"), CX_SM_Seg, 0 );
	    cspflag = 0;
	}
        if ( OK != status )
        {
            report_error( 0, status,
                          ERx("CXinitialize failure. Is_CSP = %d.\n"),
                          cspflag );
            break;
        }
    
        if ( 0 == Instances ) Instances = 1;

	LocalNode = CXnode_number(NULL);
	MyID = (ThePID << 4) | LocalNode;

        if ( CXconfig_settings( CX_HAS_1ST_CSP_ROLE ) )
        {
            SIfprintf( stderr, 
              ERx("Setting parameters for use by other instances\n") );
            SIflush( stderr );

	    TestID = MyID;

            /* Write out parameters used */
            MEfill( sizeof(ConfigCMO.cmo), '\0', (PTR)&ConfigCMO );
            ConfigCMO.cfg.instances = (u_i1)Instances;
            ConfigCMO.cfg.threads = (u_i1)Num_Threads;
            ConfigCMO.cfg.iterations = Iterations;
            ConfigCMO.cfg.lsnstyle = (u_i1)LSNstyle;
            ConfigCMO.cfg.nodeadlocks = (u_i1)NoDeadlocks;
            ConfigCMO.cfg.teststyle = Test_Style;
            ConfigCMO.cfg.lockcontext = (u_i1)LockContextFlag;
	    ConfigCMO.cfg.testid = TestID;

            status = CXcmo_write( CFG_CMO, &ConfigCMO.cmo );
            if ( OK != status )
            {
                report_error( 0, status, ERx("Config CMO write failure.") );
                break;
            }

	    switch( LSNstyle )
	    {
	    case LSN_SEQ:
                MEfill( sizeof(lsnbuf), '\0', (PTR)&lsnbuf );
                lsnbuf.lsn.cx_lsn_low = START_LSN;

                status = CXcmo_write( CX_LSN_CMO, &lsnbuf.cmo );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN init failure.") );
                }
		break;

	    case LSN_SYNCH:
                MEfill( sizeof(lsnbuf), '\0', (PTR)&lsnbuf );
                lsnbuf.lsn.cx_lsn_low = START_LSN;

		status = CXcmo_lsn_lsynch( &lsnbuf.lsn );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN init lsynch failure.") );
		    break;
                }

		status = CXcmo_lsn_gsynch( &StartLSN );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN init gsynch failure.") );
                }
		break;

	    default:
		break;
            }
        }
        else
        {
            SIfprintf( stderr, 
              ERx("Reading parameters set by first instance\n") );
	    SIflush( stderr );

            status = CXcmo_read( CFG_CMO, &ConfigCMO.cmo );
            if ( OK != status )
            {
                report_error( 0, status, ERx("Config CMO read failure.") );
                break;
            }
            if ( ConfigCMO.cfg.instances == 0 ||
                 ConfigCMO.cfg.instances > MAX_INSTANCES ||
                 ConfigCMO.cfg.threads < 2 ||
                 ConfigCMO.cfg.threads > MAX_N ||
                 ConfigCMO.cfg.iterations == 0 ||
                 ConfigCMO.cfg.teststyle >=
                     sizeof(Test_Style_Names)/sizeof(char*) ||
                 ConfigCMO.cfg.lsnstyle >=
                     sizeof(LSN_Style_Names)/sizeof(char*) ||
                 ConfigCMO.cfg.nodeadlocks > '\001' ||
                 (ConfigCMO.cfg.lockcontext != 0 &&
                  ConfigCMO.cfg.lockcontext != CX_F_PCONTEXT) )
            {
                /* Warn user that we're ignoring global parameters. */
                report_error( 0, OK,
                    ERx("Bad CMO config parameters. Using local settings.") );
            }
            else
            {
                Instances = ConfigCMO.cfg.instances;
                Num_Threads = ConfigCMO.cfg.threads;
                Iterations = ConfigCMO.cfg.iterations;
                LSNstyle = ConfigCMO.cfg.lsnstyle;
                NoDeadlocks = ConfigCMO.cfg.nodeadlocks;
                Test_Style = ConfigCMO.cfg.teststyle;
                LockContextFlag = ConfigCMO.cfg.lockcontext;
            }
	    TestID = ConfigCMO.cfg.testid;

	    switch( LSNstyle )
	    {
	    case LSN_SEQ:
                MEfill( sizeof(lsnbuf), '\0', (PTR)&lsnbuf );

                status = CXcmo_read( CX_LSN_CMO, &lsnbuf.cmo );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN init failure.") );
                }
		break;

	    case LSN_SYNCH:
		status = CXcmo_lsn_gsynch( &StartLSN );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN init gsynch failure.") );
                }
		break;

	    default:
		break;
            }
        }

        if ( OK != status )
        {
            break;
	}

	if ( LSN_SYNCH == LSNstyle )
	{
	    LK_LSN_Flag = CX_F_LSN_IN_LVB;
	}

        /*
        ** Group sessions into "gangs".   If there are fewer sessions
        ** than possible gangs, each gang will have one member, else
        ** we attempt to maximize the number of gangs so long as each 
        ** gang has an equal number of members.   If the number of
        ** sessions selected cannot be divided into at least two gangs,
        ** program will complain and a different number of starting
        ** sessions must be selected.
        */
        if ( Num_Threads <= MAX_GANGS )
            Num_Gangs = Num_Threads;
        else
        {
            for ( Num_Gangs = MAX_GANGS; Num_Gangs > 1; Num_Gangs-- )
            {
                if ( Num_Threads == Num_Gangs * ( Num_Threads / Num_Gangs ) )
                    break;
            }
            if ( 1 == Num_Gangs )
            {
                SIfprintf( stderr,
                            ERx("%s: Do not use a prime # of sessions.\n"),
                            ProgName );
                SIflush( stderr );
                status = FAIL;
                break;
            }
        }

        /*
        ** Reporting thread will report progress at even intervals,
        ** constrained that each interval must include an integer
        ** multiple of 'Num_Gangs'.   Algorithim will sacrifice
        ** number of intervals (up to half) to make intervals
        ** even.  If an even division can't be found, the maximum
        ** number of divisions is used and any odd amount is added 
        ** to the last break.
        */
        
        /* Calculate number of report intervals */
        i = ( ( Iterations + Num_Gangs - 1 ) / Num_Gangs );
        Iterations = Num_Gangs * i;
        Num_Breaks = MAX_NUM_BREAKS;
        for ( j = MAX_NUM_BREAKS; j > MAX_NUM_BREAKS / 2; j-- )
        {
            if ( 0 == ( i % j ) )
            {
                Num_Breaks = j;
                break;
            }
        }

        /* Calculate report breaks. */
        Next_Break[Num_Breaks] = Iterations;
        j = Num_Gangs * ( i / Num_Breaks );
        for ( i = 0, k = j; i < Num_Breaks; i++, k += j )
            Next_Break[i] = k;

	Transactions = Iterations * Num_Threads;

	dump_parameters(stderr);

        if ( LSN_NONE != LSNstyle )
        {
	    LOCATION	loc;
	    char	filename[64];
	    char	*logpath;

            SIfprintf( stderr, ERx("%s: Starting LSN = %08.8x.%08.8x (%d)\n"),
             ProgName, StartLSN.cx_lsn_high, StartLSN.cx_lsn_low,
	     StartLSN.cx_lsn_low );

	    /* Open "transaction log" */
	    STprintf( filename, ERx("%s_%d_%d.log"), ProgName, TestID, MyID );

	    /* Prepare log LOCATION */
	    NMloc( LOG, FILENAME, filename, &loc );
	    LOtos( &loc, &logpath );
	    LogPath = STalloc(logpath);

	    SIfprintf( stderr, ERx("%s: Transaction log = %s\n"), ProgName,
	     LogPath );
            SIflush( stderr );
	
# if USE_CL_SI_FACILITY
	    status = SIopen( &loc, "w", &LogFile );
# else
	    LogFile = fopen( LogPath, "wb" );
	    if ( NULL == LogFile ) status = FAIL;
# endif
	    if ( OK != status )
	    {
                report_error( 0, status, ERx("Log open fail for %s."),
		              LogPath );
                break;
	    }

	    /* Allocate "transaction log" buffer */
	    LogBuffer = (LOGREC *)MEreqmem( 0, sizeof(LOGREC) *
			    Transactions, FALSE, &status );
	    if ( NULL == LogBuffer || OK != status )
	    {
                report_error( 0, status, ERx("LogBuffer alloc fail.") );
                break;
	    }
        }
        SIfprintf( stderr,
		    ERx("%s: Progress report every %d of %d iterations.\n"),
                    ProgName, j, Iterations );
        if ( Next_Break[Num_Breaks - 1] != Iterations )
        {
            SIfprintf( stderr, ERx("%s: ( %d in last interval. )\n"),
                    ProgName, Iterations - Next_Break[Num_Breaks - 2] );
            Next_Break[Num_Breaks - 1] = Iterations;
        } 
        SIflush( stderr );

	/* Fill in resource keys */
	for ( i = 0; i < Num_Gangs; i++ )
	{
	    lkkey = &ResKeys[i];
	    lkkey->lk_type = LK_VAL_LOCK;
	    sprintf( (char *)&lkkey->lk_key1, " gang:%d ", i );
	    cmo_update( 0, i, Iterations );
	}

	status = CXjoin( 0 );

	if ( status )
	{
	    report_error( 0, status, ERx("CXjoin failure.") );
            break;
	}

    } while (0);
    return status;
}

/*
**  Name: log_transaction
**
**	Simulate a transaction log.
*/
static void
log_transaction( CX_LSN *lsn, i4 thug, i4 tgang, i4 tbal, i4 victim, i4 vbal,
CX_LSN *synclsn )
{
    i4		 logindex;
    LOGREC	*plogrec;

    logindex = CSadjust_counter( &LogCur, 1 );
    if ( logindex > Transactions )
    {
	report_error( thug, 0, ERx("LogBuffer overflow!") );
    }
    else
    {
        plogrec = LogBuffer + logindex - 1;
	plogrec->lsn = *lsn;
	plogrec->node = LocalNode;
	plogrec->ident = MyID;
	plogrec->index = logindex;
	plogrec->thug = thug;
	plogrec->tgang = tgang;
	plogrec->tbal = tbal;
	plogrec->victim = victim;
	plogrec->vbal = vbal;
	plogrec->synclsn = *synclsn;
    }
}


/*
**  Name: test_thread
**
**  Perform a circular "pickpocket" test.  Each session will repeatedly
**  steal $1 from other "gang" in a symetric fashion so that at the
**  end of a successful run, every gang should have the same amount of 
**  money as when they started.
**
**  Current "account" for each "gang" is kept in one-half of a CMO,
**  and a running count of the number of times gang has stolen, or been
**  victimized is kept in the lock value block.   These values are
**  constantly being cross-checked to detect any lost updates to 
**  CMO, or isolation lapses introduced by the DLM. 
**
**  Varients: 
**
**  If "patsy" flag is set, all thefts are done from neighboring gang
**  with ordinal one less than sessions (wrapping).  This should tend to long
**  deadlock chains, but again everything should be even in the end.
**
**  If "random" flag is set, thefts are done in random order,
**  but each victim is only visited once per cycle, except the "patsy"
**  neighbor who gets hit twice.
**
**  If neither are set, thefts are done in order, which should lead
**  to more contention especially in the beginning.  Again rather
**  than a thread from it's own gang, it will abuse the patsy gang.
**
**  Number of iterations is always bumped up to a multiple of
**  gang count.
**
**  History:
**	29-jan-2004 (devjo01)
**	    Acquire NL lock on self before suspend.
**	29-feb-2004 (devjo01)
**	    Put in some paranoid code to handle non-synchronous grant
**	    on a down-conversion to NL (which really should always
**	    be granted synchronously).
*/

static void
test_thread( MY_SCB *scb )
{
    typedef struct
    {
	i4	lv_thefts;
	i4	lv_loses;
    }	TP_LK_VAL;
    STATUS	status;
    CX_REQ_CB	*pvreq, *psreq;
    TP_LK_VAL	*pslv, *pvlv; 
    i4		i, j;		/* Loop counters */
    i4		self, mygang, patsies, victims; /* index of thieves & victims */
    i4		ourbalance, theirbalance;
    i4		baderrors;	/* Running count of unexpected errors */
    i4		expected;	/* work var. */
    i4		failed, havesl, havevl;
    char	*order;
    i4		deck[MAX_GANGS];/* "deck" of 0 .. MAX_GANGS - 1 */
    LK_UNIQUE	transid;	/* Sham transid for this thief. */
    i4		breaks; 	/* Report break counter */
    union {
        CX_CMO      cmo;
        CX_LSN      lsn;
        CX_CMO_SEQ  seq;
    }           lsnbuf;
    CX_LSN	prevlsn, synclsn;

    self = scb->instance;
    mygang = self % Num_Gangs;

    prevlsn.cx_lsn_low = prevlsn.cx_lsn_high = 0;

    status = CXunique_id( &transid );
    if ( OK != status )
    {
	BAD_ERROR( self, status );
	baderrors = ERROR_LIMIT;
    }
    else
    {
	psreq = &ReqCBs[self][SELF_RCB];
	pvreq = &ReqCBs[self][VICT_RCB];

	breaks = baderrors = 0;
	 
	patsies = ( mygang == 0 ? Num_Gangs : mygang ) - 1;

	/* Get NULL lock on mygang. */
	psreq->cx_new_mode = LK_N;
	psreq->cx_key = ResKeys[mygang];
	psreq->cx_user_func = NULL;
	psreq->cx_user_extra = NULL;
	pslv = (TP_LK_VAL *)psreq->cx_value;
	status = CXdlm_request( CX_F_WAIT | LockContextFlag,
				psreq, &transid );
	if ( OK != status )
	{
	    report_error( self, status, 
		    ERx("Fatal: Could not obtain NULL lock on mygang") );
	    baderrors = ERROR_LIMIT;
	}

	/* Suspend here, pending go-ahead by report thread */
	status = CSsuspend( 0, 0, 0 );
	if ( status )
	    return;

	(void)TMhrnow( &Timings[self][0] );

    }

    for ( i = 0;
          !Interrupted && 
	  baderrors < ERROR_LIMIT &&
	  i < Iterations;
	  i += Num_Gangs )
    {
	if ( i == Next_Break[breaks] )
	{
	    if ( Num_Threads ==
		 CSadjust_counter( &Completion_Count[breaks], 1 ) )
	    {
		/* We're the last one finished */
		CSresume( *pReportSID );
	    }
	    breaks++;
	    (void)TMhrnow( &Timings[self][breaks] );
	}

	for ( j = 0; baderrors < ERROR_LIMIT && j < Num_Gangs; j++ )
	{
	    switch ( Test_Style )
	    {
	    case PATSY_TEST:
		victims = patsies;
		break;
	    case RANDOM_ORDER_TEST:
		if ( j == 0 )
		    shuffle( deck, Num_Gangs );
		victims = deck[j];
		break;
	    default:
		victims = j;
	    }

	    if ( victims == mygang )
	    {
		/* Don't foul own nest, pick "patsy" gangs pocket. */
		victims = patsies;
	    }

	    for ( ; baderrors < ERROR_LIMIT ; )
	    {
		if ( NoDeadlocks && ( mygang > victims ) )
		{
		    /* Reverse order of lock acquisition */
		    order = ERx("CRTrD");
		}
		else
		{
		    order = ERx("RCTDr");
		}

		failed = havesl = havevl = 0;

		/* BEGIN TRANSACTION */
		if ( LSN_SYNCH == LSNstyle )
		{
# if 0
		    /* Simulate Begin Transaction Sync */
		    status = CXcmo_lsn_gsynch( &synclsn );
		    if ( OK != status )
		    {
			report_error( self, 0,
			 ERx("Error in ET CXcmo_lsn_gsynch. " \
			 "status = %x\n"),
			 status );
			break;
		    }
		    if ( !LSN_LE(&prevlsn, &synclsn) )
		    {
			report_error( self, 0,
			 ERx("BT gsync LSN has moved backwards." \
			 "Sync LSN=%8.8x.%8.8x OLD LSN=%8.8x.%8.8x\n"),
			 synclsn.cx_lsn_high, synclsn.cx_lsn_low,
			 prevlsn.cx_lsn_high, prevlsn.cx_lsn_low );
		    }
# endif
		}

		while ( *order )
		{
		    switch ( *order++ )
		    {
		    case 'R':
			if ( failed )
			    break;

			/* Get lock on victim gang */
			pvreq->cx_new_mode = LK_X;
			pvreq->cx_key = ResKeys[victims];
			pvreq->cx_user_func = NULL;
			pvreq->cx_user_extra = NULL;
			pvlv = (TP_LK_VAL *)pvreq->cx_value;

			status = CXdlm_request( LK_LSN_Flag |
					CX_F_STATUS | LockContextFlag,
					pvreq, &transid );
			add_res_stat( status, self, breaks, REQ_STATS );

			if ( OK == status )
			{
			    /* Wait for asynchronous completion */
			    status = CXdlm_wait( CX_F_INTERRUPTOK,
					pvreq, Time_Out );
			    add_res_stat( status, self, breaks, WAIT_STATS );
			    add_res_stat( pvreq->cx_status, self, breaks, 
			      CMPL_STATS );
			    if ( OK == status )
			    {
				/* Successful asynchronous completion */
				;
			    }
			    else if ( E_CL2C04_CX_I_OKINTR == status )
			    {
				/* Successful completion, but user
				** interrupt pending.
				*/
				confirm_interrupt();
				status = OK;
			    }
			    else if ( E_CL2C21_CX_E_DLM_DEADLOCK == status ||
				      E_CL2C0A_CX_W_TIMEOUT == status ||
				      E_CL2C0B_CX_W_INTR == status )
			    {
				/* Non-fatal failure, try again */
				failed = 1;
				break;
			    }
			    else
			    {
				/* Fatal failures. */
				BAD_ERROR( self, status );
				baderrors++;
				failed = 1;
				break;
			    }
			}
			else if ( E_CL2C01_CX_I_OKSYNC == status )
			{
			    /* Got it synchronously */
			    ;
			}
			else if ( E_CL2C04_CX_I_OKINTR == status )
			{
			    /* Successful completion, but user
			    ** interrupt pending.
			    */
			    confirm_interrupt();
			    status = OK;
			}
			else if ( E_CL2C21_CX_E_DLM_DEADLOCK == status )
			{
			    /* Non-fatal failure, try again */
			    failed = 1;
			    break;
			}
			else
			{
			    /* Unexpected error */
			    BAD_ERROR( self, status );
			    baderrors++;
			    failed = 1;
			    break;
			}
			havevl = 1;

			/* Get ready to steal a single from victim gang */
			theirbalance = cmo_read( self, victims );

			expected = ( Iterations + pvlv->lv_thefts - 
			   pvlv->lv_loses );
			if ( theirbalance != expected )
			{
			    /* Accounting mismatch for  victim gang! */
			    report_error( self, 0,
			     ERx("Accounting mismatch for victim " \
			     " = %d, expected %d, saw %d, after\n" \
			     "%d of his thefts and %d of his loses."), 
			      victims, expected, theirbalance, 
			      pvlv->lv_thefts, pvlv->lv_loses );
			    BAD_ERROR( self, -3 );
			    baderrors++;
			    failed = 1;
			    break;
			}

			break;

		    case 'C':
			if ( failed )
			    break;

			/* Get mygang in exclusive mode */
			psreq->cx_new_mode = LK_X;
			status = CXdlm_convert( LK_LSN_Flag | CX_F_STATUS,
			                        psreq );
			add_res_stat( status, self, breaks, CONV_STATS );
			if ( OK == status )
			{
			    /* Wait for asynchronous completion */
			    status = CXdlm_wait( CX_F_INTERRUPTOK, 
					psreq, Time_Out );
			    add_res_stat( status, self, breaks, WAIT_STATS );
			    add_res_stat( psreq->cx_status, self, breaks,
					  CMPL_STATS );
			    if ( OK == status )
			    {
				/* Successfull asynchronous completion */
				;
			    }
			    else if ( E_CL2C04_CX_I_OKINTR == status )
			    {
				/* Successful completion, but user
				** interrupt pending.
				*/
				confirm_interrupt();
				status = OK;
			    }
			    else if ( E_CL2C21_CX_E_DLM_DEADLOCK == status ||
				      E_CL2C0A_CX_W_TIMEOUT == status ||
				      E_CL2C0B_CX_W_INTR == status )
			    {
				/* Non-fatal error, try again */
				failed = 1;
				break;
			    }
			    else
			    {
				/* Unexpected error */
				BAD_ERROR( self, status );
				baderrors++;
				failed = 1;
				break;
			    }
			}
			else if ( E_CL2C01_CX_I_OKSYNC == status )
			{
			    /* Got it synchronously */
			    ;
			}
			else if ( E_CL2C21_CX_E_DLM_DEADLOCK == status )
			{
			    /* Non-fatal failure, try again */
			    failed = 1;
			    break;
			}
			else if ( E_CL2C04_CX_I_OKINTR == status )
			{
			    /* Successful completion, but user
			    ** interrupt pending.
			    */
			    confirm_interrupt();
			    status = OK;
			}
			else
			{
			    /* Unexpected error */
			    BAD_ERROR( self, status );
			    baderrors++;
			    failed = 1;
			    break;
			}

			ourbalance = cmo_read( self, mygang );

			havesl = 1;

			/* Cross check values in lock blocks */
			expected = 
			  ( Iterations +  pslv->lv_thefts - pslv->lv_loses );
			if ( ourbalance != expected )
			{
			    /* Accounting mismatch for mygang! */
			    report_error( self, 0,
			     ERx("Accounting mismatch for mygang. " \
			     "Expected %d, saw %d after %d thefts, %d loses."), 
			     expected, ourbalance,
			     pslv->lv_thefts, pslv->lv_loses );
			    BAD_ERROR( self, -2 );
			    baderrors++;
			    failed = 1;
			    break;
			}

			break;
		    
		    case 'T':
			/* Don't perform update if steps 'R' or 'C' failed. */
			if ( failed )
			    break;

			/*
			** Have covering locks, perform the "transaction"
			*/
			(void)cmo_update( self, victims, theirbalance - 1 );
			(void)cmo_update( self, mygang, ourbalance + 1 );

			/*
			** If testing LSN generation, get an LSN.
			*/
                        switch (LSNstyle)
                        {
                        case LSN_SEQ:
                            status = CXcmo_sequence( CX_LSN_CMO, &lsnbuf.seq );
			    if ( OK != status )
				report_error( self, 0,
				 ERx("Error updating CXcmo_sequence. " \
				 "status = %x, LSN=%8.8x.%8.8x\n"),
				 status, lsnbuf.lsn.cx_lsn_high,
				 lsnbuf.lsn.cx_lsn_low );

			    log_transaction( &lsnbuf.lsn, self, mygang,
			                     ourbalance + 1,
					     victims, theirbalance - 1, 
					     &synclsn );

			    /*
			    ** COMMIT WORK.
			    */

			    /* Nothing to do for this flavor */
                            break;

                        case LSN_SYNCH:
                            /*
                            ** Since we're simulating a loosely coupled
                            ** LSN coordinator, we just adjust the local
                            ** value.  At the end of the transaction,  
			    ** (which is just a few lines down in this
			    ** test), we sync the Local LSN at EOT back 
			    ** to the global LSN.
                            */

			    /* Generate local LSN */
                            status = CXcmo_lsn_next( &lsnbuf.lsn );
			    if ( OK != status )
			    {
				report_error( self, 0,
				 ERx("Error in CXcmo_lsn_next. " \
				 "status = %x\n"),
				 status );
				break;
			    }

			    log_transaction( &lsnbuf.lsn, self, mygang,
					     ourbalance + 1,
					     victims, theirbalance - 1,
					     &synclsn );

			    /*
			    ** COMMIT WORK.
			    */

			    /* Perform EOT synchronization */
                            status = CXcmo_lsn_gsynch( &synclsn );
			    if ( OK != status )
			    {
				report_error( self, 0,
				 ERx("Error in ET CXcmo_lsn_gsynch. " \
				 "status = %x\n"),
				 status );
				break;
			    }
			    
			    if ( !LSN_LE(&lsnbuf.lsn, &synclsn) )
			    {
				report_error( self, 0,
				 ERx("ET gsync LSN has moved backwards." \
				 "Sync LSN=%8.8x.%8.8x OLD LSN=%8.8x.%8.8x\n"),
				 synclsn.cx_lsn_high, synclsn.cx_lsn_low,
				 lsnbuf.lsn.cx_lsn_high,
				 lsnbuf.lsn.cx_lsn_low );
			    }
			    break;

                        default:
                            status = OK;
                            break;
                        }

                        if ( LSN_NONE != LSNstyle )
			{
			    if ( LSN_LE(&lsnbuf.lsn, &prevlsn) )
			    {
				report_error( self, 0,
     ERx("LSN has moved backwards. New LSN=%8.8x.%8.8x OLD LSN=%8.8x.%8.8x\n"),
				    lsnbuf.lsn.cx_lsn_high,
				    lsnbuf.lsn.cx_lsn_low, 
				    prevlsn.cx_lsn_high, prevlsn.cx_lsn_low );
			    }
			    prevlsn = lsnbuf.lsn;
			}

			pslv->lv_thefts++;
			pvlv->lv_loses++;
			break;

		    case 'D':
			if ( !havesl )
			    break;

			/* Downgrade my lock */
			psreq->cx_new_mode = LK_N;
			status = CXdlm_convert( LK_LSN_Flag |
			  CX_F_STATUS | CX_F_OWNSET,
			  psreq );
			add_res_stat( status, self, breaks, DOWN_STATS );
			if ( OK == status )
			{
			    /* Wait for asynchronous completion */
			    status = CXdlm_wait( CX_F_INTERRUPTOK, 
					psreq, Time_Out );
			    if (status == E_CL2C0A_CX_W_TIMEOUT )
			    	    BAD_ERROR( mygang, status );
			}
			else if ( E_CL2C04_CX_I_OKINTR == status )
			{
			    /* Successful completion, but user
			    ** interrupt pending.
			    */
			    confirm_interrupt();
			    status = OK;
			}
			else if ( E_CL2C01_CX_I_OKSYNC != status )
			{
			    /* Should've been granted synchronously */
			    BAD_ERROR( mygang, status );
			}
			break;

		    case 'r':
			if ( !havevl )
			    break;

			/* Release victims lock */
			status = CXdlm_release( 0, pvreq );
			add_res_stat( status, self, breaks, RLSE_STATS );
			if ( OK != status )
			{
			    /* Unexpected error */
			    BAD_ERROR( self, status );
			    baderrors++;
			}
		    } /* switch */
		} /* while */

		if ( !failed )
		    break;
	    } /* for */

	    Iterations_Done[self]++;
	    CSadjust_counter( &Total_Thefts, 1 );
	} /* for one cycle */
    } /* for all iterations */

    if ( ERROR_LIMIT != baderrors )
    {
	(void)TMhrnow( &Timings[self][Num_Breaks] );
    }

    /* Error or not, bump counters for all remaining intervals. */
    {
	i4	dores = 0;

	while ( breaks < Num_Breaks )
	{
	    if ( Num_Threads ==
		 CSadjust_counter( &Completion_Count[breaks], 1 ) )
	    {
		/* We're the last one finished */
		dores = 1;
	    }
	    breaks++;
	}
	if ( dores )
	    CSresume( *pReportSID );
    }
    (void)Psem( &SIsem );
    SIfprintf( stderr, ERx("%s: Thread %d finished\n"), ProgName, self );
    SIflush( stderr );
    (void)Vsem( &SIsem );
}
/*
** Name: report_thread - Test results thread.
**
** This procedure is resonsible for displaying periodic progress
** histograms, punctuated by summary reprts, when each test thread
** has finished a portion of the test run.
**
** It also works with the message thread to coordinate start and
** finish of multiple instances of this test program across the cluster.
**
** History:
**	29-jan-2004 (devjo01)
**	    Add checks that all test threads are ready.
**	04-feb-2004 (devjo01)
**	  - Always display histogram after intervals.
**	  - Add an CScancelled before final CXmsg_send call to
**	    consume any pending resume from test threads.
**	  - Correct reported program instances calculation
**	    in "Waiting for ... to finish" message.
*/
static void
report_thread( MY_SCB *scb )
{
#   define	STALL_LIMIT	3

    ST_RES_STATS	*rs, resstats;
    STATUS		 status;
    i4			 breaksprocessed = 0, stat, i, j, k;
    i4			 stalls, last_total;
    time_t		 histtime;
    char		 deltabuf[16];
    char		 histbuf[4 * MAX_NUM_BREAKS + 16];
    union {
        CX_CMO      cmo;
        i4          seq[4];
    }           lsnbuf;

    pReportSID = &SIDs[scb->instance];

    /* Make sure message channel, and test threads are ready */
    i = 0;
    while ( 1 )
    {
	if ( CXmsg_channel_ready( 0 ) )
	{
	    /*
	    ** Check to see if all test threads have reached
	    ** the starting line.
	    */
	    for ( j = 0; j < Num_Threads; j++ )
	    {
		if ( !CX_NONZERO_ID(&ReqCBs[j][SELF_RCB].cx_lock_id) )
		{
		    /* Not all test_threads ready */
		    break;
		}
	    }
	    if ( j == Num_Threads )
	    {
		/* 
		** All local test threads are ready, breakout so
		** this instance of the program can test the
		** world it is ready to go!
		*/
		break;
	    }
	}

	/* Not all threads are ready, suspend a second, then test again. */
	status = CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
	if ( status )
	    CScancelled( (PTR)0 );
	if ( !(i++ % 5) )
	{
	    /* Report waiting for channel start. */
	    (void)time( &histtime );
	    (void)Psem( &SIsem );
	    SIfprintf( stderr, ERx("Waiting for message channel @%s"),
	       ctime( &histtime ) );
            SIflush( stderr );
	    (void)Vsem( &SIsem );
	}
    }

    /* Tell the world we are ready */
    {
	TEST_MSG	addmsg;

	addmsg.action = TM_ADD_NODE;
	status = CXmsg_send( 0, (CX_MSG*)&addmsg, message_send, (PTR)&addmsg );
    }

    /* Wait until all nodes are ready */
    i = 0;
    while ( !StartRun )
    {
	status = CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
	if ( status )
	    CScancelled( (PTR)0 );
	if ( !(i++ % 5) )
	{
	    /* Report waiting for other instances message. */
	    (void)time( &histtime );
	    (void)Psem( &SIsem );
	    SIfprintf( stderr,
	       ERx("Waiting for %d more instance(s) of %s to start @%s"),
	       Instances - SignedIn, ProgName, ctime( &histtime ) );
            SIflush( stderr );
	    (void)Vsem( &SIsem );
	}
    }

    /* Reset clock for truer TPS calculation when running multiple instances */
    Start_Time = time(NULL);

    (void)Psem(&SIsem);
    SIfprintf( stderr, ERx("%s: Leaving starting gate at %s\n"),
	    ProgName, ctime(&Start_Time));
    SIflush( stderr );
    (void)Vsem(&SIsem);

    /* OK, all nodes are ready, wake the test threads */
    for ( i = 0; i < Num_Threads; i++ )
	CSresume( SIDs[i] );

    /*
    ** Loop, displaying progress histograms between ten second naps.
    ** When all sessions have completed a fraction of the run, give
    ** intermediate summary statistics.  Exit loop when all report
    ** intervals completed.
    */
    stalls = last_total = 0;
    while ( breaksprocessed < Num_Breaks )
    {
	status = CSsuspend( CS_TIMEOUT_MASK, 10, 0 );
	if ( status )
	    CScancelled( (PTR)0 );

	if ( Completion_Count[breaksprocessed] == Num_Threads )
	{
	    while ( Completion_Count[breaksprocessed] == Num_Threads )
	    {
		/* Report results so far */
		(void)time( &histtime );
		(void)Psem( &SIsem );
		SIfprintf( stderr,
		   ERx("\n*** Summary for interval %d/%d @ %-24.24s ***\n"),
		   breaksprocessed + 1, Num_Breaks, ctime( &histtime ) );
		
		/* Consolidate & report session stats */
		for ( stat = 0; stat < NUM_OF_STATS; stat++ )
		{
		    resstats.rs_inx = 0;
		    for ( i = 0; i < Num_Threads; i++ )
		    {
			rs = &ResStats[i][breaksprocessed][stat];
			for ( j = 0; j < rs->rs_inx; j++ )
			    sum_res_stat( &resstats,
			     rs->rs_stats[j].rs_status_code,
			     rs->rs_stats[j].rs_occurances );
		    }

		    dump_results( ResStatNames[stat], &resstats );
		}

                SIflush( stderr );

		(void)Vsem( &SIsem );

		if ( ++breaksprocessed == Num_Breaks )
		    break;
	    }
	}
	
	/* Report progress  for both intervals and time-outs */
	(void)Psem( &Miscsem );
	(void)time( &histtime );
	j = Num_Threads;
	for ( i = 0; i < Num_Breaks; i++ )
	{
	    k = Completion_Count[i];
	    (void)STprintf( histbuf + ( 4 * i ) + i / 2,
	      ERx(" %03d "), j - k );
	    j = k;
	}
	(void)STprintf( histbuf + ( 4 * i ) + i / 2, ERx(": %03d "), k );
	(void)STprintf( deltabuf, ERx("(+%d)"), Total_Thefts - last_total );
	(void)Vsem( &Miscsem );
	(void)Psem( &SIsem );
	SIfprintf( stderr, ERx("\n%-24.24s: Progress histogram:\n"),
	    ctime( &histtime ) );
	SIfprintf( stderr, ERx("%9d %-9.9s [%s]\n"),
	    Total_Thefts, deltabuf, histbuf );
        SIflush( stderr );
	(void)Vsem( &SIsem );

	if ( Completion_Count[breaksprocessed] != Num_Threads )
	{
	    if ( last_total == Total_Thefts )
	    {
		if ( STALL_LIMIT == ++stalls )
		{
		    (void)TRdisplay( ERx("**** STALL DETECTED ****\n") );
		    dump_all();
		}
	    }
	    else
	    {
		last_total = Total_Thefts;
		stalls = 0;
	    }
	}
    }

    End_Time = time(NULL);

    (void)Psem( &SIsem );
    SIfprintf( stderr, ERx("\n%s: Local run complete @%-24.24s\n"),
	ProgName, ctime( &End_Time ) );
    SIflush( stderr );
    (void)Vsem( &SIsem );

    if ( LSN_NONE != LSNstyle )
    {
	LOGREC	*plogrec;

	/* Write out transaction log */

	if ( LogCur != Transactions )
	{
	    report_error( scb->instance, 0,
		ERx("Only %d Transactions completed out of %d anticipated\n"),
		LogCur, Transactions );
	}

	if ( MyInstance != 1 )
	{
	    i4	bytesout;

# if USE_CL_SI_FACILITY
	    status = SIwrite( LogCur * sizeof(LOGREC), (char *)LogBuffer,
			      &bytesout, LogFile );
# else
	    bytesout = fwrite( LogBuffer, sizeof(LOGREC), LogCur, LogFile );
	    if ( bytesout != LogCur ) status = FAIL;
	    bytesout *= sizeof(LOGREC);
# endif
	    if ( OK != status )
	    {
		report_error( scb->instance, status,
		    ERx("Error writing \"transaction\" log %s\n"), LogPath );
	    }
	    else if ( bytesout != LogCur * sizeof(LOGREC) )
	    {
		report_error( scb->instance, 0,
		    ERx("Error writing \"transaction\" log %s\n" \
		        "Only %d of %d bytes written"), LogPath,
			bytesout, LogCur * sizeof(LOGREC) );
	    }
	}

# if USE_CL_SI_FACILITY
	SIclose(LogFile);
# else
	fclose(LogFile);
# endif
	LogFile = NULL;
    }

    /* All local threads finished.  Announce local completion */
    {
	TEST_MSG	donemsg;

	/* Consume any pending resumes left over from test_threads. */
	CScancelled(NULL);

	/* Tell the world this instance of CXtest is done. */
	donemsg.action = TM_DEL_NODE;
	status = CXmsg_send( 0, (CX_MSG*)&donemsg, message_send,
			     (PTR)&donemsg );
	if ( status != OK )
	    BAD_ERROR( 0, status );
    }

    /* Wait until all other nodes are done */
    i = 0;
    while ( SignedIn > 0 )
    {
	status = CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
	if ( status )
	    CScancelled( (PTR)0 );
	if ( !(i++ % 5) )
	{
	    /* Report waiting for other instances message. */
	    (void)time( &histtime );
	    (void)Psem( &SIsem );
	    SIfprintf( stderr,
	       ERx("Waiting for %d more instance(s) of %s to finish @%s"),
	       SignedIn, ProgName, ctime( &histtime ) );
            SIflush( stderr );
	    (void)Vsem( &SIsem );
	}
    }

    Done_Time = time(NULL);

    /* Disconnect from message channel */
    status = CXmsg_disconnect(0);
    if ( OK != status )
	report_error( scb->instance, status, "CXmsg_disconnect" );

    /* Terminate message monitor thread. */
    status = CXmsg_shutdown();
    if ( OK != status )
	report_error( scb->instance, status, "CXmsg_shutdown" );
}


/*
**  monitor_thread
**
**  Body of thread to monitor broadcast messages.
*/
static void
monitor_thread( MY_SCB *scb )
{
    STATUS status;

    /* Connect to message system */
    status = CXmsg_monitor();
    report_error( scb->instance, status, "Message monitor shutdown" );
}


/*
**  message_thread
**
**  Body of thread to receive  broadcast messages.
*/
static void
message_thread( MY_SCB *scb )
{
    static char *msgs[2] =
    {
      "CXmsg_connect failure",
      "Message receive thread shutdown"
    };
    STATUS status;

    /* Connect to message system */
    status = CXmsg_connect( 0, message_handler, NULL );
    report_error( scb->instance, status, msgs[status==OK] );
}


struct _stat_titles {
    char 	*item;
    char        *desc;
}	stat_titles[] = {
    "cx_severe_errors",		"# of severe errors recorded.",
    "cx_msg_mon_poll_interval",	"Msg monitor poll interval (ms)",
    "cx_msg_missed_blocks",	"# Missed blocking notifications",
    "cx_msg_notifies",		"# Message notifications rcvd",
    "cx_dlm_requests",		"# CXdlm_request calls.",
    "cx_dlm_quedreqs",		"# Requests enqueued.",
    "cx_dlm_sync_grants",	"# Synchronous grants.",
    "cx_dlm_converts",		"# CXdlm_convert calls.",
    "cx_dlm_quedcvts",		"# Converts enqueued.",
    "cx_dlm_sync_cvts",		"# Synchronous conversion grants.",
    "cx_dlm_cancels",		"# CXdlm_cancel calls.",
    "cx_dlm_releases",		"# CXdlm_release calls.",
    "cx_dlm_async_waits",	"# Async lock waits.",
    "cx_dlm_async_grants",	"# Async lock grants.",
    "cx_dlm_completions",	"# lock completions.",
    "cx_dlm_blk_notifies",	"# Blocking notifications rcvd.",
    "cx_dlm_deadlocks",		"# Deadlock victims.",
    "cx_cmo_reads",		"# CMO reads.",
    "cx_cmo_writes",		"# CMO writes.",
    "cx_cmo_updates",		"# CMO updates.",
    "cx_cmo_seqreqs",		"# CMO sequence number requests.",
    "cx_cmo_synchreqs",		"# CMO sequence # synchronizations",
    "cx_cmo_lsn_gsynchreqs",	"# Global LSN sequence # synchs",
    "cx_cmo_lsn_lsynchreqs",	"# Local LSN sequence # synchs",
    "cx_cmo_lsn_nextreqs",	"# LSN sequence #'s generated",
    "cx_cmo_lsn_lvblsyncs",	"# LVB LSN local synchs from LVB", 
    "cx_cmo_lsn_lvbrsyncs",	"# LVB LSN local synchs to LVB", 
    "cx_cmo_lsn_lvbgsyncs",	"# LVB LSN global synchs", 
};

/*
** Sort comparator for "transaction" log records.  Sort order is
** by LSN, with NODE as a tie breaker if needed.
*/
static int
log_sort_func( const void *a, const void *b )
{
    const LOGREC	*plra = a, *plrb = b;

    if ( plra->lsn.cx_lsn_high > plrb->lsn.cx_lsn_high )
	return 1;
    if ( plra->lsn.cx_lsn_high < plrb->lsn.cx_lsn_high )
	return -1;
    if ( plra->lsn.cx_lsn_low > plrb->lsn.cx_lsn_low )
	return 1;
    if ( plra->lsn.cx_lsn_low < plrb->lsn.cx_lsn_low )
	return -1;
    if ( plra->node > plrb->node )
	return 1;
    if ( plra->node < plrb->node )
	return -1;
    return 0;
}

/*
**  Routine called when all test threads have completed.
*/
static void
test_shutdown(void)
{
    STATUS	status;
    struct _stat_titles *ptitle;
    i4		*pstat;
    i4		i, balance;
    CL_ERR_DESC	syserr;

    (void)Psem( &SIsem ); 

    /* Print final report */
    SIfprintf( stderr, ERx("%s: Statistics taken from CX_STAT\n\n"),
		ProgName );

    if ( sizeof(stat_titles)/sizeof(stat_titles[0]) !=
	 sizeof(CX_Stats)/sizeof(i4) )
	SIfprintf( stderr,
	 ERx("*** Err.  Stats do not match report titles ***\n" ) );
    else
    {
	for ( i = 0, pstat = (i4 *)&CX_Stats, ptitle = stat_titles;
	      i < sizeof(CX_Stats)/sizeof(i4);
	      i++, pstat++, ptitle++ )
	{
	    SIfprintf( stderr, ERx("%-24.24s %10d   %s\n"), ptitle->item,
			*pstat, ptitle->desc );
	}
    }

    SIfprintf( stderr,
     ERx("\n\n%s: Finished at %-24.24s, elapsed time = %d seconds.\n"),
     ProgName, ctime(&End_Time), End_Time - Start_Time );
    SIfprintf( stderr,
     ERx("%s: Throughput = %d \"transactions\" per second\n\n"),
         ProgName, (100 * Transactions + 50) /
	 ( 100 * ( End_Time - Start_Time ) + 1 ) );

    if ( Instances > 1 )
    {
	SIfprintf( stderr,
	     ERx("%s: All finished at %-24.24s, " \
	     "elapsed time = %d seconds.\n"),
	     ProgName, ctime(&Done_Time), Done_Time - Start_Time );
	SIfprintf( stderr,
	 ERx("%s: Throughput = %d \"transactions\" per second\n\n"),
	     ProgName, (100 * Instances * Transactions + 50) /
	     ( 100 * ( Done_Time - Start_Time ) + 1 ) );
    }
    SIflush( stderr );

    if ( CX_ST_UNINIT != CX_Proc_CB.cx_state )
    {
	for ( i = 0; i < Num_Gangs; i++ )
	{
	    balance = cmo_read( 0, i );
	    if ( balance != Iterations )
	    {
		SIfprintf(stderr,
		 ERx("**** Unexpected total for gang #%d (%d.00)! ****\n"),
		 i,  balance );
                SIflush( stderr );
	    }
	    status = CXdlm_release( 0, &ReqCBs[i][SELF_RCB] );
	}

        if ( LSNstyle != LSN_NONE )
        {
	    /*
	    **	Perform additional verifications if using LSN's.
	    */
	    union {
		CX_LSN		lsn;
		CX_CMO		cmo;
	    }	lsnbuf;
            i4 			expectedlsn;

	    /*
	    **	Read FINAL LSN, and see if it looks reasonable.
	    */
            expectedlsn = START_LSN + Transactions * Instances;

            switch ( LSNstyle )
            {
            case LSN_SEQ:
                status = CXcmo_read( CX_LSN_CMO, &lsnbuf.cmo );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN read final failure.") );
                }
                if ( lsnbuf.lsn.cx_lsn_low != expectedlsn )
                {
                    SIfprintf(stderr,
     ERx("**** Unexpected Final LSN.  Expected %d, got %d! ****\n"),
                          expectedlsn, lsnbuf.lsn.cx_lsn_low );
                }
		else
		{
		     SIfprintf(stderr,
		      ERx("Final LSN value = %08.8x.%08.8x (%d)\n"),
		      lsnbuf.lsn.cx_lsn_high, lsnbuf.lsn.cx_lsn_low,
		      lsnbuf.lsn.cx_lsn_low );
		}
                break;

            case LSN_SYNCH:
		status = CXcmo_lsn_gsynch( &lsnbuf.lsn );
                if ( status )
                {
                    report_error( 0, status, ERx("LSN final gsynch failure.") );
                }

                if ( lsnbuf.lsn.cx_lsn_low > expectedlsn )
                {
                    SIfprintf(stderr,
     ERx("**** Unexpected Final LSN. Expected no more than %d, got %d! ****\n"),
                          expectedlsn, lsnbuf.lsn.cx_lsn_low );
                }
                break;
            }

	    if ( 1 == MyInstance )
	    {
		/*
		** First process in any set of test processes has
		** the responsibility of verifying the "transaction"
		** logs put out by all the processes in this test run.
		*/
		i4		balances[MAX_GANGS];
		LOCATION	loc;
		i4		records, errors, seennodes;
		CX_LSN		prev, lsn;
		i4		node, ident, thug, tgang, tbal, victim, vbal;
		char		*snippoint;
		LOGREC		*sortbuffer, *sortbufend, *plogrec;
		i4		logbufsize, bytesread;

		SIfprintf( stderr,
		    ERx("%s: Analyzing transaction logs, please wait.\n"),
		    ProgName );

		/* Our own file is empty since we didn't write data out */
		LOfroms( PATH & FILENAME, LogPath, &loc );
		status = LOdelete( &loc );
		if ( OK != status )
		{
                    report_error( 0, status, ERx("Fail to delete %s."),
		                  LogPath );
		    status = OK; /* Ignore file delete failure. */
		}

		snippoint = STrchr( LogPath, '_' );
		if ( 1 == Instances )
		{
		    /* Only one instance, piece o'cake. */
		    sortbuffer = LogBuffer;
		    sortbufend = sortbuffer + LogCur;
		}
		else
		{
		    logbufsize = Transactions * sizeof(LOGREC);

		    /* Allocate sort buffer */
		    sortbuffer = (LOGREC *)MEreqmem( 0,
			    sizeof(LOGREC) + Instances * logbufsize,
			    FALSE, &status );
		    if ( OK != status )
		    {
			report_error( 0, status,
			      ERx("Fail to allocate SortBuffer.") );
		    }
		    else
		    {
			MEcopy( (PTR)LogBuffer, LogCur * sizeof(LOGREC),
				(PTR)sortbuffer );
			sortbufend = sortbuffer + LogCur;

			for ( i = 1; i < Instances; i++ )
			{
			    /* Open "transaction" log for other instance */
			    if ( 0 == ( ident = KnownInstances[i] ) )
			    {
				report_error( 0, 0,
				  ERx("Don't know ID for instance %d."), i );
				continue;
			    }
			    STprintf( snippoint, "_%d.log", ident );
			    LOfroms( PATH & FILENAME, LogPath, &loc );

# if USE_CL_SI_FACILITY
			    status = SIopen( &loc, "r", &LogFile);
# else
			    LogFile = fopen( LogPath, "rb" );
			    if ( NULL == LogFile ) status = FAIL;
# endif
			    if ( OK != status )
			    {
				report_error( 0, status,
			 	 ERx("LogFile open fail for %s."), LogPath );
				break;
			    }
# if USE_CL_SI_FACILITY
			    status = SIread( LogFile,
					     logbufsize + sizeof(LOGREC),
					     &bytesread, (char *)sortbufend );
			    if ( ENDFILE == status )
			    {
				/* This is the expected status code. */
				status = OK;
			    }
			    (void)SIclose(LogFile);
# else
			    bytesread = fread( sortbufend, sizeof(LOGREC), Transactions + 1, LogFile );
			    bytesread *= sizeof(LOGREC);
			    if ( !bytesread ) status = FAIL;
			    fclose( LogFile );
# endif
			    (void)LOdelete( &loc );
			    if ( OK != status ) /* SIread status */
			    {
				report_error( 0, status,
			 	 ERx("LogFile READ fail for %s."), LogPath );
				break;
			    }
			    if ( bytesread < logbufsize ) 
			    {
				report_error( 0, status,
	     ERx("LogFile %s unexpectedly small, %d bytes read, %d expected."),
	                         LogPath, bytesread, logbufsize  );
				sortbufend +=
				 (u_i4)(bytesread / sizeof(LOGREC) );
			    }
			    else
			    {
				if ( bytesread > logbufsize )
				{
				    report_error( 0, status,
	     ERx("LogFile %s unexpectedly large, ignoring extra."), LogPath );
				}
				sortbufend += Transactions;
			    }
			}
		    }
		}

		if ( OK == status )
		{
		    /* Sort the buffer */
		    qsort( sortbuffer, sortbufend - sortbuffer, 
		           sizeof(LOGREC), log_sort_func );
		}

		/* Set balances to original values */
		for ( i = 0; i < Num_Gangs; i++ )
		    balances[i] = Iterations;

		/*
		** Read Each value, and make sure delta for "thug" is
		** always plus one, and delta for victim is always
		** minus one.
		*/
		records = errors = seennodes = 0;
		prev.cx_lsn_high = 0; prev.cx_lsn_low = START_LSN;
		plogrec = sortbuffer;
		while ( status == OK && errors < ERR_ABORT_LIMIT )
		{
		    if ( plogrec == sortbufend )
		    {
			status = ENDFILE;
			break;
		    }
		    lsn = plogrec->lsn;
		    node = plogrec->node;
		    ident = plogrec->ident;
		    thug = plogrec->thug;
		    tgang = plogrec->tgang;
		    tbal = plogrec->tbal;
		    victim = plogrec->victim;
		    vbal =  plogrec->vbal;
		    records++;
		    plogrec++;

		    /* LSN sequence numbers should have no gaps */
		    if ( ((prev.cx_lsn_low+1) == lsn.cx_lsn_low &&
			   prev.cx_lsn_high == lsn.cx_lsn_high) ||
			 (((u_i4)(~0)) == prev.cx_lsn_low &&
			   0 == lsn.cx_lsn_low &&
			   (prev.cx_lsn_high+1) == lsn.cx_lsn_high) )
		    {
			/* good new LSN */
			seennodes = 0;
		    }
		    else if ( LSNstyle == LSN_SYNCH &&
			      prev.cx_lsn_low == lsn.cx_lsn_low &&
			      prev.cx_lsn_high == lsn.cx_lsn_high &&
			      !( (1l<<node) & seennodes ) )
		    {
			/*
			** It is permissible to have dups, so
			** long as they are not on the same 
			** node when using the LSN_SYNCH method.
			*/
			seennodes |= (1l<<node);
		    }
		    else if ( (1l<<node) & seennodes )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
			     ERx("LSN collision: Node = %d"), node );
			}
		    }
		    else
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
			     ERx("Bad LSN advance: Prev LSN = %08x.%08x"),
			     prev.cx_lsn_high, prev.cx_lsn_low );
			}
			seennodes = 0;
		    }
		    prev = lsn;

		    /* Check balances */
		    if ( tgang == victim )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
     ERx("Record shows gang %d stealing from itself!\nThis error may cascade"),
				  victim );
			}
			continue;
		    }

		    if ( tgang < 0 || tgang >= Num_Gangs )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
     ERx("Bad \"tgang\" in log: %d.  This error may cascade"),
			      tgang );
			}
		    }
		    else if ( ++balances[tgang] != tbal )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
     ERx("Bad balance for \"tgang\" %d in log: saw %d expected %d.\n" \
         "This error may cascade"), tgang, tbal, balances[tgang] );
			}
		    }
			
		    if ( victim < 0 || victim >= Num_Gangs )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
     ERx("Bad \"victim\" in log: %d.  This error may cascade"),
			      victim );
			}
		    }
		    else if ( --balances[victim] != vbal )
		    {
			if ( errors++ < ERR_DISP_LIMIT )
			{
			    report_log_error( records, &lsn,
     ERx("Bad balance for victim gang %d in log: saw %d expected %d.\n" \
         "This error may cascade"), victim, vbal, balances[victim] );
			}
		    }
		}

		if ( ENDFILE != status )
		{
		    if ( errors > ERR_DISP_LIMIT )
		    {
			report_error( 0, 0,
     ERx("Only first %d errors displayed.\n" \
         "%d error(s) not displayed."),
			 ERR_DISP_LIMIT, errors - ERR_DISP_LIMIT );
		    }
		    if ( errors >= ERR_ABORT_LIMIT )
		    {
			report_error( 0, 0,
     ERx("Log analysis cut short after %d errors"), ERR_ABORT_LIMIT );
		    }
		    else
		    {
			report_error( 0, 0, ERx("Could not check logs") );
		    }
		}
		else if ( records != Instances * Transactions )
		{
		    report_error( 0, 0,
     ERx("Warning: Expected %d transactions, read %d"),
			    Instances * Transactions, records );
		}

		if ( DumpLog )
		{
		    if ( errors > ERR_DISP_LIMIT )
		    {
			SIfprintf( stderr,
     ERx("Only first %d errors displayed. %d error(s) not displayed.\n"),
			 ERR_DISP_LIMIT, errors - ERR_DISP_LIMIT );
		    }

		    STcopy( ERx(".log"), snippoint );
		    SIfprintf( stderr,
     ERx("%s: Writing merged logs in plain text to:\n%s\n"),
			ProgName, LogPath );

		    LOfroms( PATH & FILENAME, LogPath, &loc );
		    status = SIopen( &loc, "w", &LogFile);
		    if ( OK != status )
		    {
			report_error( 0, status,
			 ERx("Diagnostic File open failed.") );
		    }
		    else
		    {
			SIfprintf( LogFile,
			 ERx("=Log-LSN    Node  Rec-ID Rec-Idx Thug TG " \
			     "TBalance Vi Vict-Bal Sync-LSN=\n") );
			for ( plogrec = sortbuffer;
			      plogrec < sortbufend;
			      plogrec++ )
			{
			    SIfprintf( LogFile, LOG_REC_FORMAT,
			     plogrec->lsn.cx_lsn_high, plogrec->lsn.cx_lsn_low,
			     plogrec->node, plogrec->ident, plogrec->index,
			     plogrec->thug, plogrec->tgang, plogrec->tbal,
			     plogrec->victim, plogrec->vbal, 
			     plogrec->synclsn.cx_lsn_high,
			     plogrec->synclsn.cx_lsn_low);
			}
			SIfprintf( LogFile,
			 ERx("====== Params used ======\n") );
			dump_parameters(LogFile);
			SIfprintf( LogFile,
			 ERx("====== Errors Seen ======\n") );
			*LogErrMsgPtr = '\0';
			if ( '\0' == LogErrMsgBuf[0] )
			{
			    SIfprintf( LogFile,
			    ERx("%s: No errors, all is well\n"), ProgName );
			}
			for ( LogErrMsgPtr = LogErrMsgBuf;
			      *LogErrMsgPtr;
			      LogErrMsgPtr += STlength(LogErrMsgPtr) + 1 )
			{
			    SIfprintf( LogFile, ERx("%s\n"), LogErrMsgPtr );
			}
			(void)SIclose(LogFile);
		    }
		}
		else if ( records == Instances * Transactions )
		{
		    SIfprintf( stderr,
			    ERx("%s: ALL IS WELL. No errors found.\n"),
			    ProgName );
		}
		SIfprintf( stderr,
		    ERx("%s: Transaction log analysis complete.\n"),
		    ProgName );
	    }
	    SIflush( stderr );
        }

	status = CXterminate( 0 );
	if ( OK != status )
	{
	    report_error( 0, status, ERx("CXterminate failure.\n") );
	}
    }

    if ( Created_CX_Seg )
    {
        status = MEsmdestroy("cxtest.mem", &syserr);
        if ( OK != status )
	{
	    report_error( 0, status, ERx("MEsmdestroy failure.\n") );
	}
    }

    (void)Vsem( &SIsem );
}


/* ---------------- Generic support routines ---------------- */

/*
** Generic thread function, called out of CSdispatch 
*/
static STATUS
threadproc( i4 mode, CS_SCB *csscb, i4 *next_mode )
{
    MY_SCB *scb = (MY_SCB *) csscb;
    if ( OSthreaded < 0 )
    {
	/*
	** Must be done here, since CS_is_mt() does not return
	** correct status until CSdispatch is called.
	*/
	(void)Psem( &SIsem );
	if ( OSthreaded < 0 )
	{
	    OSthreaded = CS_is_mt() ? 1 : 0;
	    SIfprintf(stderr, ERx("Starting %s with %d %s threads\n"),
			   ProgName,  Num_Threads, OSThreadTypes[OSthreaded] ); 
            SIflush( stderr );
	}
	(void)Vsem( &SIsem );
    }

    switch( mode )
    {
    case CS_INITIATE:	/* A new thread is born */

	scb->instance = CSadjust_counter(&Start_Threads,1) - 1;
	(void)CSadjust_counter(&End_Threads,1);
	Thread_Status[scb->instance] = mode;
	CSget_sid( &SIDs[scb->instance] ); 

	/*
	** Invoke Test code here
	*/
	switch ( scb->instance - Num_Threads )
	{
	case 0:
	    monitor_thread(scb);
	    break;
	case 1:
	    message_thread(scb);
	    break;
	case 2:
	    report_thread(scb);
	    break;
	default:
	    test_thread(scb);
	}
	    

	/* fall into: */

    default:
	*next_mode = CS_TERMINATE;
	break;

    case CS_TERMINATE:
	if( Noisy )
	{
	    (void)Psem( &SIsem );
	    SIfprintf(stderr, ERx("Session #%d dies, RIP.\n"),
			scb->instance );
            SIflush( stderr );
	    (void)Vsem( &SIsem );
	}
	if ( Thread_Status[scb->instance]  == mode )
	{
	    report_error( scb->instance, 0, 
		  ERx("Oops this thread is already dead?\n" \
		  "\t\t CS code is non-functional\n") );
	}

	*next_mode = CS_TERMINATE;

	/* If no more threads, shutdown */
	Thread_Status[scb->instance] = mode;

	if( CSadjust_counter(&End_Threads,-1) == 0 )
    		(void)CSterminate( CS_KILL, (i4 *)NULL );

	break;
    }
    return( OK );
}


/*
**	Setup global variables and add each thread.
**	Called by CSinitialize before threads are started
*/
static STATUS
hello( CS_INFO_CB *csib )
{
    i4  i;
    STATUS stat;
    CL_SYS_ERR	err;

# if !defined(VMS) /* _VMS_ */
    (void)signal(SIGSEGV, SIG_DFL);
    (void)signal(SIGBUS, SIG_DFL);
# endif /* _VMS_ */

    (void)CSi_semaphore(&SIsem, CS_SEM_SINGLE);
    (void)CSi_semaphore(&Miscsem, CS_SEM_SINGLE);

    stat = test_startup();
    if ( OK == stat )
    {
	for( i = 0; i < (Num_Threads + THR_EXTRA) && stat == OK; i++ )
	{
	    stat = CSadd_thread( 0, (PTR)NULL, i + 1, (CS_SID*)NULL, &err );
	}

	/* no semaphore needed, called before CSdispatch */
	if( Noisy )
        {
	    SIfprintf(stderr, ERx("World begins, threads wonder why.\n") );
            SIflush( stderr );
        }

	(void)CSi_semaphore(&sc0m_semaphore, CS_SEM_SINGLE);
     
	csib->cs_mem_sem = &sc0m_semaphore;
    }

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

    return( OK );
}


/* ---------------- book keeping ---------------- */



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
    SIfprintf( stderr,
	ERx("\n\t%s [ <instances> [ <sessions> [ <iterations> ] ] ] \\\n" \
	   "\t\t[ -style patsy | random | default ] \\\n" \
	   "\t\t[ -timeout <secs> ] [ -trace ] \\\n" \
	   "\t\t[ -deadlock ] [ -nolsn | -lsn | -seq ] \\\n"
	   "\t\t[ -dumplog ] [ -verbose ]\n"), ProgName );
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
**	cxtest [ <instances> [ <sessions> [ <iterations> ] ] ]\
**		   [ -style patsy | random | default ]
**		   [ -timeout <secs> ] [ -trace ]
**		   [ -deadlock ] [ -proc_context ]
**		   [ -nolsn | -lsn | -seq ]
**		   [ -dumplog ] [ -verbose ]
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
    i4  	arg, numberarg, number;
    i4		verbose = 0;
    char	*argp;
    char 	**f_argv,*f_arr[3];
    struct utsname nodeinfo;
    CL_ERR_DESC syserr;
	
    STATUS	status;

    set_prog_name( argv[0] );

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    numberarg = arg = 0;
    while ( ++arg < argc )
    {
	argp = argv[arg];
	if ( *argp > '0' &&  *argp <= '9' )
	{
	    if ( OK == CVan( argp, &number ) )
	    {
		switch ( numberarg++ )
		{
		case 0:
		    Instances = number;
		    if ( Instances < 1 || Instances > MAX_INSTANCES )
			usage( argp );
		    break;
		case 1:
		    Num_Threads = number;
		    if ( Num_Threads < 2 || Num_Threads > MAX_N )
			usage( argp );
		    break;
		case 2:
		    Iterations = number;
		    if ( Iterations < 1 || Iterations > 1000000 )
			usage( argp );
		    break;
		default:
		    usage( argp );
		}
	    }
	    else
		usage( argp );
	}
	else if ( '-' == *argp )
	{
	    argp++;
	    if ( 0 == STcompare( argp, ERx("help") ) )
	    {
		usage(NULL);
	    }
	    else if ( 0 == STcompare( argp, ERx("style") ) )
	    {
		if ( arg + 1 == argc )
		    usage( ERx("<missing>") );
		argp = argv[++arg];
		if ( 0 == STcompare( argp, ERx("default") ) )
		    Test_Style = DEFAULT_TEST;
		else if ( 0 == STcompare( argp, ERx("patsy") ) )
		    Test_Style = PATSY_TEST;
		else if ( 0 == STcompare( argp, ERx("random") ) )
		    Test_Style = RANDOM_ORDER_TEST;
		else
		    usage( argp );
	    }
	    else if ( 0 == STcompare( argp, ERx("deadlock") ) )
	    {
		if ( CXconfig_settings( CX_HAS_DEADLOCK_CHECK ) )
		{
		    NoDeadlocks = 0;
		}
		else
		{
		    SIfprintf( stderr,
		     ERx("DLM does not support transactions.\n" \
		         "Ignoring \"deadlock\" parameter.\n" ) );
		}
	    }
	    else if ( 0 == STcompare( argp, ERx("timeout") ) )
	    {
		if ( arg + 1 == argc )
		    usage( ERx("<missing>") );
		argp = argv[++arg];
		if ( OK != CVan( argp, &Time_Out ) )
		    usage( argp );
	    }
	    else if ( 0 == STcompare( argp, ERx("trace") ) )
	    {
		SIfprintf( stderr,
		 ERx("DLM trace ON (IF compiled into CX)\n") ); 
		SIflush( stderr );
		(void)CXalter( CX_A_DLM_TRACE, (PTR)1 );
	    }
	    else if ( 0 == STcompare( argp, ERx("proc_context") ) )
	    {
		LockContextFlag = CX_F_PCONTEXT;
	    }
	    else if ( 0 == STcompare( argp, ERx("nolsn") ) )
	    {
		LSNstyle = LSN_NONE;
            }
	    else if ( 0 == STcompare( argp, ERx("lsn") ) )
	    {
		LSNstyle = LSN_SYNCH;
            }
	    else if ( 0 == STcompare( argp, ERx("seq") ) )
	    {
		LSNstyle = LSN_SEQ;
            }
	    else if ( 0 == STcompare( argp, ERx("dumplog") ) )
	    {
		DumpLog = TRUE;
            }
	    else if ( 0 == STcompare( argp, ERx("verbose") ) )
	    {
		verbose = TRUE;
            }
	    else
	    {
		usage( argp - 1 );
	    }
	}
	else if ( 0 == STcompare( argp, ERx("help") ) )
	{
	    usage( NULL );
	}
	else
	{
	    usage( argp );
	}
    }

# if !defined(VMS) /* _VMS_ */
    (void) iiCL_increase_fd_table_size(FALSE, Num_Threads + THR_EXTRA +
                                       CS_RESERVED_FDS);
    if( (iiCL_get_fd_table_size() - CS_RESERVED_FDS) < Num_Threads  )
	Num_Threads = iiCL_get_fd_table_size() - CS_RESERVED_FDS - THR_EXTRA;
# endif /* _VMS_ */

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
    cb.cs_scnt = Num_Threads + THR_EXTRA; /* Number of sessions */
    cb.cs_ascnt = 0;		/* nbr of active sessions */
    cb.cs_stksize = (16 * 1024);/* size of stack in bytes */
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
    argc = 2;
    f_arr[0] = argv[0];
    f_arr[1] = ERx("-nopublic");
    f_arr[2] = NULL;
    f_argv = f_arr;

    if( rv = CSinitiate( &argc, &f_argv, &cb ) )
    {
	SIfprintf(stderr, ERx("CSinitiate failure! (return=0x%x)\n"), rv );
# if !defined(VMS) /* _VMS_ */
        SIfprintf(stderr, ERx("You must run csinstall brfore running %s\n"),
               ProgName );
# endif /* _VMS_ */
	SIflush( stderr );
	PCexit(FAIL);
    }

    /* now do everything that has been set up */
    rv = CSdispatch();

    /* should not get here */
    SIfprintf(stderr, ERx("CSdispatch returns %d (%x)\n"), rv, rv );
    SIflush( stderr );

    PCexit( FAIL );
}

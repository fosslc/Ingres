/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ep.h>
#include    <ex.h>
#include    <tr.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <cs.h>
#include    <nm.h>
#include    <st.h>
#include    <cm.h>
#include    <si.h>
#include    <er.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <dmfrcp.h>

/**
** 
**  Name: LOGSTAT.C - dumps logging system statistics to 'stdout'
**
**  Description:
**    This file contains:
**	main		the main entry point to this program
**	logstat		cover for call to log_lg_stats
**	log_lg_stats	does actual work
**
**  History:
**      22-aug-1986 (Derek)
**	    Created for Jupiter.
**	 2-AUG-1988 (Edhsu)
**	    Added code for fast commit, fix BCNT, and END Trans count bugs
**	05-oct-1988 (greg)
**	    enhance error message.  Remove ifdefs.
**	31-oct-1988 (rogerk)
**	    Print out free_wait and stall_wait statistics in log_statistics().
**	05-jan-1989 (mmm)
**	    bug #4405.  We must call CSinitiate(0, 0, 0) from any program 
**	    which will connect to the logging system or logging system.
**	    Also add to log_init() error message for unix.
**	27-mar-1989 (rogerk)
**	    Added support for Multi-server fast commit.
**	25-sep-1989 (rogerk)
**	    Added LGD_RCP_RECOVER status.
**	08-dec-1989 (mikem)
**	    Added Dump Window information.
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**	09-jan-1989 (mikem)
**	    Support for new flags added as part of online checkpoint bug fixing.
**	    System status - SONLINE_PURGE.  Database status - ONLINE_PURGE, 
**	    PRG_PEND.
**	18-jan-1990 (fredp)
**	    Changed  log_lg_stats() to pass the structure members of
**	    lxb.tr_dis_id.lg_tran_id to TRdisplay() as this is more
**	    portable.  This prevents SEGV on the Sun4.
**	23-mar-1990 (mikem)
**	    Added code for internal use to make logstat print out stats in
**	    a "vmstat" style format (ie. dump the stats every 5 seconds in
**	    one line).
**	 9-apr-1990 (sandyh)
**	    Added log file percentage calculation and made log split 'waits'.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	25-jul-1990 (bryanp)
**	    If transaction is in "WAIT" state, print out "Status: WAIT".
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**       4-feb-1991 (rogerk)
**          Added lgs_bcp_stall_wait statistic to show the number of
**          log writes stalled by BCP being written. Broke this count
**          out of the generic lgs_stall_wait so that could show REAL stalls.
**          Added new lg stats in logstat output - logfull stalls, BCP stalls.
**      13-jun-1991 (bryanp)
**          Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**      15-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**	15-aug-1991 (jnash) merged 6-Oct-1989 (ac)
**	    Added support for dual logging.
**      15-aug-1991 (jnash) merged 10-Oct-90 (pholman)
**          Disable dual logging for UNIX.
**      15-aug-1991 (jnash) merged 13-may-1991 (bonobo)
**          Added the PARTIAL_LD mode ming hint.
**      26-sep-1991 (jnash) merged 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	30-oct-1991 (bryanp)
**	    Re-enable dual logging for Unix.
**	05-mar-1992 (mikem)
**	    Change the display of logstat dynamic to handle 4 decimal TPS
**	    display (ie. 0-9999).  Also fix logstat dynamic to display the
**	    correct values for the # of LG writes per second.  This value
**	    was being set to the # of timer writes per second.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	22-jul-1992 (bryanp)
**	    Check status from CSinitiate(). New stats & output for new LG/LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	6-oct-1992 (bryanp)
**	    Delete duplicate include of dbms.h.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	2-nov-1992 (bryanp)
**	    Display buffer information.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	15-mar-1993 (rogerk)
**	    Removed EBACKUP, FBACKUP states.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Format the db_sback_lsn field in the LG_DATABASE.
**		Display the CSP's PID if it is non-zero.
**		If xDEBUG is defined, then try to display all the logging system
**		    statistics even if the logging system is not online. This
**		    is primarily useful in "crashed" or "hung" system situations
**		    where any information you can get is nice. Of course, it's
**		    depressingly common that the actual result is that you
**		    coredump within LGshow, which is why this is within xDEBUG.
**	24-may-1993 (bryanp)
**	    It's often nice to be able to get just part of a logstat display,
**		so I added support for "logstat -statistics", "-header",
**		"-processes", "-databases", "-transactions", "-buffers". Good
**		old "logstat" means "all".
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.
**	  - Renamed requests for Logging System JNL and DMP windows to use
**	    new archive window show interfaces.
**	  - Changed logstat output to present "Archive Window" and the
**	    "Previous CP".
**	23-aug-1993 (jnash)
**	    Add command parser to OR-in command line arguments.
**	23-aug-1993 (bryanp)
**	    Added "-user_transactions", "-special_transactions", and
**	    "-all_transactions", for filtering the transactions display.
**	20-sep-1993 (mikem)
**	    Re-enabled the ability to call unsupported "logstat dynamic",
**	    one of the recent changes had disabled this (forcing the logstat
**	    help message when it was attempted).
**	26-oct-1993 (rogerk)
**	    Changed logspace used calculations to use unsigned values.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so dump them.
**	23-may-1994 (bryanp)
**	    Rather than directly calling CSinitiate, make a "dummy" scf_call.
**	    This initializes ALL of SCF, rather than just CS, and in particular
**	    performs some initialization (CSset_sid, etc.) which enables the
**	    use of CSp_semaphore on CS_SEM_SINGLE semaphores. Normally,
**	    logstat doesn't actually need CSp_semaphore on CS_SEM_SINGLE
**	    semaphores, but there are edge cases where it does (for example, if
**	    logstat's rundown processing should discover that an event needs
**	    to be signalled), and it doesn't harm anything for logstat to have
**	    a fully initialized SCF.
**	20-jun-1994 (jnash)
**	    Eliminate unused ONLINE_PURGE state.
**	18-nov-1994 (andyw)
**	    Domestic change to printing active transactions
**	    including printing request block titles
**	22-Apr-1996 (jenjo02)
**	    Added lgs_no_logwriter stat (count of times a logwriter thread
**	    was needed but none were available) to log_lg_stats().
**	28-Jun-1996 (jenjo02)
**	    Added lgs_timer_write_time, lgs_timer_write_idle stats
**	    to show what is driving gcmt thread log writes.
**	10-Jul-1996 (jenjo02)
**	    Added display of buffer utilization histograms.
**	20-aug-96 (nick)
**	    Fix trivial usage message bugs.
**	01-Oct-1996 (jenjo02)
**	    Added display of lgs_max_wq (maximum write queue length),
**	    and lgs_max_wq_count (#times max hit).
**	21-apr-97 (sarjo01)
**	    Bug 81575: added exception handler for stand-alone
**	    (non-IIMERGE) version.
**	14-Nov-1997 (jenjo02)
**	    Added DB_STALL_RW status for databases.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	29-Jan-1998 (jenjo02)
**	    Partitioned log files:
**	    added display of number of partitions to log header.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED log transactions. Updated display
**	    values of lxb_status.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
**	07-jan-2008 (joea)
**	    Discontinue use of cluster nicknames.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _LG_STAT_QUEUE LG_STAT_QUEUE;


/*
**  Defines of other constants.
*/

/*	Constants used for "vmstat" style log stat print out */

/* number of seconds to wait in between sampling */
#define                 SECS_PER_SAMPLE 	5

/* maximum number of data point's collected in the "rolling" average */
#define                 MAX_STAT		(60 / SECS_PER_SAMPLE)

/* # of lines to display before printing header - should be a program arg */
#define                 TERMINAL_LENGTH 	10

# ifndef II_DMF_MERGE
static STATUS ex_handler(
    EX_ARGS     *ex_args);
# endif

static	u_i4	logstat_options;
#define	    LOGSTAT_STATISTICS	    0x0001L
#define	    LOGSTAT_HEADER	    0x0002L
#define	    LOGSTAT_PROCESSES	    0x0004L
#define	    LOGSTAT_DATABASES	    0x0008L
#define	    LOGSTAT_TRANSACTIONS    0x0010L
#define	    LOGSTAT_BUFFERS	    0x0020L
#define	    LOGSTAT_USER_TRANS	    0x0040L
#define	    LOGSTAT_SPECIAL_TRANS   0x0080L
#define	    LOGSTAT_HELP	    0x0100L
#define	    LOGSTAT_DEFAULT	    (LOGSTAT_STATISTICS|LOGSTAT_HEADER|\
				    LOGSTAT_PROCESSES|LOGSTAT_DATABASES|\
				    LOGSTAT_TRANSACTIONS)
#define	    LOGSTAT_ALL		    (LOGSTAT_STATISTICS|LOGSTAT_HEADER|\
				    LOGSTAT_PROCESSES|LOGSTAT_DATABASES|\
				    LOGSTAT_TRANSACTIONS|LOGSTAT_BUFFERS)

/*}
** Name: LG_STAT_QUEUE -  A queue of LG_STATS.
**
** Description:
**	An array implementation of a queue of stats.  Used to keep a running
**	total/average over MAX_STAT data points of various interesting LG_STAT
**	members.  Used to calculate LG system "tps".
**
** History:
**      23-mar-90 (mikem)
**          Created.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _LG_STAT_QUEUE
{
    i4         lgq_head;           /* comment about member of struct */
    i4         lgq_tail;           /* comment about member of struct */
    i4         lgq_count;          /* comment about member of struct */
    LG_STAT	    lgq_stats[MAX_STAT + 1];
					/* array of stats to function as Q */
};


/*
**  Forward and/or External function references.
*/

/* static routine's to support "vmstat" style logstat output */

static STATUS logstat(
    i4	    argc,
    char    *argv[]);

static	VOID	log_lg_stats(VOID);
static VOID	log_dyn_lg_stats(VOID);
static VOID	init_queue(LG_STAT_QUEUE    *stat_queue);

static VOID	stat_insert(
    LG_STAT		*stat,
    LG_STAT_QUEUE	*stat_queue);

static VOID	get_avg(
    LG_STAT_QUEUE	*stat_queue,
    LG_STAT		*avg_stat);


/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	logstat_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*
**	mkming hints
MODE =		SETUID PARTIAL_LD

NEEDLIBS =	SCFLIB DMFLIB ADFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

OWNER =		INGUSER

PROGRAM =	logstat
*/

/*
[@forward_type_references@]
*/

static 	VOID 	logstat_usage(void);

/*
[@function_reference@]...
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definition@]...
[@function_definition@]...
*/


/*{
**
** Name: main	- DUMP log statitics to "stdout"
**
** Description:
**	This program, which may require privilege to run,
**	dumps the logging system statistics to "stdout".
**
** Inputs:
**	None
**
** Outputs:
**	Returns:
**	    SUCCESS/FAILURE through PCexit
**	Exceptions:
**	    none
** Side Effects:
**	    none
**
** History:
**	23-mar-1990 (mikem)
**	    Added code for internal use to make logstat print out stats in
**	    a "vmstat" style format (ie. dump the stats every 5 seconds in
**	    one line).
**      04-jul-1990 (boba)
**          Add spaces in front of "main(" so mkming won't think this
**          is a main routine on UNIX machines where II_DMF_MERGE is
**          always defined (reintegration of 14-mar-1990 change).
**          the beginning of the line.
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**       4-feb-1991 (rogerk)
**          Added lgs_bcp_stall_wait statistic to show the number of
**          log writes stalled by BCP being written. Broke this count
**          out of the generic lgs_stall_wait so that could show REAL stalls.
**          Added new lg stats in logstat output - logfull stalls, BCP stalls.
**      13-jun-1991 (bryanp)
**          Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**      15-aug-91 (jnash) merged 18-dec-1990 (bonobo)
**          Added the PARTIAL_LD mode, and moved "main(argc, argv)" back to
**          the beginning of the line.
**      26-sep-1991 (jnash) merged 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	12-jan-1992 (jnash)
**	    Reduced logging project.  Modify logfull calculation, add log space
**	    reservation output.
**	24-may-1993 (bryanp)
**	    It's often nice to be able to get just part of a logstat display,
**		so I added support for "logstat -statistics", "-header",
**		"-processes", "-databases", "-transactions", "-buffers". Good
**		old "logstat" means "all except -buffers". "logstat -verbose"
**		means "all, all, all".
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	26-jul-1993 (jnash)
**	    Add -help support.
**	23-aug-1993 (jnash)
**	    Add command parser to OR-in command line arguments.
**	23-aug-1993 (bryanp)
**	    Added "-user_transactions", "-special_transactions", and
**	    "-all_transactions", for filtering the transactions display.
**	20-sep-1993 (mikem)
**	    Re-enabled the ability to call unsupported "logstat dynamic",
**	    one of the recent changes had disabled this (forcing the logstat
**	    help message when it was attempted).
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so dump them.
**	23-may-1994 (bryanp)
**	    Rather than directly calling CSinitiate, make a "dummy" scf_call.
**	    This initializes ALL of SCF, rather than just CS, and in particular
**	    performs some initialization (CSset_sid, etc.) which enables the
**	    use of CSp_semaphore on CS_SEM_SINGLE semaphores. Normally,
**	    logstat doesn't actually need CSp_semaphore on CS_SEM_SINGLE
**	    semaphores, but there are edge cases where it does (for example, if
**	    logstat's rundown processing should discover that an event needs
**	    to be signalled), and it doesn't harm anything for logstat to have
**	    a fully initialized SCF.
**	20-aug-96 (nick)
**	    Return FAIL if command line syntax incorrect.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	06-Aug-2009 (wanfr01)
**	    Bug 122418 - Print OS generic LGinitialize error
**	    removed duplicate ule_format
*/

# ifdef	II_DMF_MERGE
VOID MAIN(argc, argv)
# else
VOID 
main(argc, argv)
# endif
i4	argc;
char	*argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(logstat(argc, argv));
}

static STATUS
logstat(
i4	argc,
char	*argv[])
{
    CL_ERR_DESC		sys_err;
    CL_ERR_DESC         cl_err;
    STATUS              status = FAIL;
    STATUS              ret_status;
    i4		err_code;
    char                node_name[CX_MAX_NODE_NAME_LEN];
    i4		i;
    SCF_CB		scf_cb;
    bool		syntax_error;
# ifndef II_DMF_MERGE
    EX_CONTEXT          context;
# endif
    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
			     &sys_err);

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if (status)
    {
#if 0	/* can't TRdisplay if TRset_file fails... */
	TRdisplay("LOGSTAT: failed to open trace file\n");
#endif
        return (FAIL);
    }
# ifndef II_DMF_MERGE
    if (EXdeclare(ex_handler, &context) != OK)
    {
        EXdelete();
        PCexit(FAIL);
    }
# endif
    if (CXcluster_enabled())
    {
	CXnode_name(node_name);
	ule_initiate(node_name, STlength(node_name), "LOGSTAT", 7);
    }    
    else
    {
	ule_initiate(0, 0, "LOGSTAT", 7);
    }

    logstat_options = 0;
    if (argc == 1)
	logstat_options = LOGSTAT_DEFAULT;

    syntax_error = FALSE;
    for (i = 1; i < argc; i++)
    {
	if (MEcmp("-statistics", argv[i], sizeof("-statistics")-1) == 0)
	    logstat_options |= LOGSTAT_STATISTICS;
	else if (MEcmp("-header", argv[i], sizeof("-header")-1) == 0)
	    logstat_options |= LOGSTAT_HEADER;
	else if (MEcmp("-processes", argv[i], sizeof("-processes")-1) == 0)
	    logstat_options |= LOGSTAT_PROCESSES;
	else if (MEcmp("-databases", argv[i], sizeof("-databases")-1) == 0)
	    logstat_options |= LOGSTAT_DATABASES;
	else if (MEcmp("-transactions", argv[i], sizeof("-transactions")-1)==0)
	    logstat_options |= LOGSTAT_TRANSACTIONS;
	else if (MEcmp("-buffers", argv[i], sizeof("-buffers")-1) == 0)
	    logstat_options |= LOGSTAT_BUFFERS;
	else if (MEcmp("-verbose", argv[i], sizeof("-verbose")-1) == 0)
	    logstat_options |= LOGSTAT_ALL;
	else if (MEcmp("-user_transactions", argv[i],
			sizeof("-user_transactions")-1)==0)
	    logstat_options |= LOGSTAT_USER_TRANS;
	else if (MEcmp("-special_transactions", argv[i],
			sizeof("-special_transactions")-1)==0)
	    logstat_options |= LOGSTAT_SPECIAL_TRANS;
	else if (MEcmp("-all_transactions", argv[i],
			sizeof("-all_transactions")-1)==0)
	    logstat_options |= LOGSTAT_TRANSACTIONS;
	else if (MEcmp("-help", argv[i], sizeof("-help")-1) == 0)
	    logstat_options |= LOGSTAT_HELP;
	else if (MEcmp("dynamic",  argv[i], sizeof("dynamic") - 1) == 0)
	    /* do nothing */;
	else
	{
	    syntax_error = TRUE;
	    logstat_options |= LOGSTAT_HELP;
	}
    }

    /*
    ** Service -help request.
    */
    if (logstat_options & LOGSTAT_HELP)
    {
	logstat_usage();
	if (syntax_error)
	    return(FAIL);
	else
	    return(OK);
    }

    /*
    ** Initialize LOGSTAT to be a single user server.
    ** This is done in a completely straightforward and natural way here
    ** by calling scf to allocate memory.  It turns out that this memory
    ** allocation request will result in SCF initializing SCF and CS
    ** processing (In particular: CSinitiate and CSset_sid).  Before this call,
    ** CS semaphore requests will fail.  The memory allocated here is not used.
    */
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = ((42 + SCU_MPAGESIZE - 1)
	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

    status = scf_call(SCU_MALLOC, &scf_cb);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
	return (status);
    }

    if (status = LGinitialize(&sys_err, ERx("logstat")))
    {
	TRdisplay("LOGSTAT: call to LGinitialize failed\n");
	TRdisplay("\tPlease check your installation and permissions\n");

	TRdisplay("\tThis error will occur if this program is run prior\n");
	TRdisplay("to the creation of the system shared memory segments \n");
    }
    else
    {
	/* Set the proper character set for CM */

	ret_status = CMset_charset(&cl_err);
	if ( ret_status != OK)
	{
	    uleFormat(NULL, ret_status, (CL_ERR_DESC *)&cl_err,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("\n Error while processing character set attribute file.\n");
	    return(FAIL);
	}
        
	/*  Write logging statistics. */
	if ((argc == 2) && (STcompare(argv[1], "dynamic") == 0))
	    log_dyn_lg_stats();
	else
	    log_lg_stats();
    }

    return (status);
}

/*
** Name: logstat_usage   - Echo logstat usage format.
**
** Description:
**	 Echo logstat usage format.
**
** History:
**	26-jul-1993 (jnash)
**	    Created.
**	23-aug-1993 (bryanp)
**	    Added -user_transactions, -special_transactions, -all_transactions.
**	20-aug-96 (nick)
**	    Add [-help] !
**      21-Oct-2008 (hanal04) Bug 121105
**          Add [-processes]
*/
static VOID
logstat_usage(void)
{
    TRdisplay(
       "usage: logstat [-buffers] [-databases] [-header] [-statistics]\n");

    TRdisplay(
       "\t[-transactions] [-user_transactions] [-special_transactions]\n");

    TRdisplay(
       "\t[-all_transactions] [-verbose] [-processes] [-help]\n");

    return;
}

/*
** Name: log_lg_stats		    - Call LG to get stats, TRdisplay them.
**
** Description:
**	This routine extracts statistics from LG and formats them for display.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Side Effects:
**	Much TRdisplay is done.
**
** History:
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Format the db_sback_lsn field in the LG_DATABASE.
**		Display the CSP's PID if it is non-zero.
**		If xDEBUG is defined, then try to display all the logging system
**		    statistics even if the logging system is not online. This
**		    is primarily useful in "crashed" or "hung" system situations
**		    where any information you can get is nice. Of course, it's
**		    depressingly common that the actual result is that you
**		    coredump within LGshow, which is why this is within xDEBUG.
**	24-may-1993 (bryanp)
**	    Add a few more database status bits for the database display.
**	26-jul-1993 (rogerk)
**	  - Changed the way journal and dump windows are handled.  They are
**	    now tracked by real Log Addresses rather than the Consistency
**	    points that bound them.  Consolidated the system journal and
**	    dump windows into a single archive window.
**	  - Renamed requests for Logging System JNL and DMP windows to use
**	    new archive window show interfaces.
**	  - Changed logstat output to present "Archive Window" and the
**	    "Previous CP".
**	23-aug-1993 (bryanp)
**	    Added "-user_transactions", "-special_transactions", and
**	    "-all_transactions", for filtering the transactions display.
**	26-oct-1993 (rogerk)
**	    Changed logspace used calculations to use unsigned values.
**	20-jun-1994 (jnash)
**	    Eliminate unused ONLINE_PURGE state.
**	18-nov-1994 (andyw)
**	    Domestic change to printing active transactions
**	    including printing request block titles
**	22-Apr-1996 (jenjo02)
**	    Added lgs_no_logwriter stat (count of times a logwriter thread
**	    was needed but none were available) to log_lg_stats().
**	01-Jul-1997 (shero03)
**	    Added JSwitch state.
**	14-Nov-1997 (jenjo02)
**	    Added DB_STALL_RW status for databases.
**	29-Jan-1998 (jenjo02)
**	    Partitioned log files:
**	    added display of number of partitions to log header.
**	26-Apr-2000 (thaju02)
**	    Removed DB_STALL_RW status. Added DB_CKPLK_STALL. (B101303)
**	02-May-2003 (jenjo02)
**	    Corrected lgd_status bit interpretations.
**	5-Jan-2006 (kschendel)
**	    Total reserved space is now u_i8 for large logs.
**	15-Mar-2006 (jenjo02)
**	    Moved guts to dmdlog 
*/
static VOID    
log_lg_stats(VOID)
{
    i4		dmd_options = 0;

    if (logstat_options & LOGSTAT_STATISTICS)
	dmd_options |= (DMLG_STATISTICS | DMLG_BUFFER_UTIL);
    if (logstat_options & LOGSTAT_HEADER)
	dmd_options |= DMLG_HEADER;
    if (logstat_options & LOGSTAT_PROCESSES)
	dmd_options |= DMLG_PROCESSES;
    if (logstat_options & LOGSTAT_DATABASES)
	dmd_options |= DMLG_DATABASES;
    if (logstat_options & LOGSTAT_TRANSACTIONS)
	dmd_options |= DMLG_TRANSACTIONS;
    if (logstat_options & LOGSTAT_USER_TRANS)
	dmd_options |= DMLG_USER_TRANS;
    if (logstat_options & LOGSTAT_SPECIAL_TRANS)
	dmd_options |= DMLG_SPECIAL_TRANS;
    if (logstat_options & LOGSTAT_BUFFERS)
	dmd_options |= DMLG_BUFFERS;

    dmd_log_info(dmd_options);

    return;
}

/*{
** Name: log_dyn_lg_stats()	- print out dynamic running avg. of lg stats.
**
** Description:
**	A facility to print out a running average of lg statistics.  It 
**	collects lg statistics into a MAX_STAT array, and calculates an
**	average of all the stats in the array.  The interval for collecting
**	the stats is given by SECS_PER_SAMPLE  (which probably should be
**	changed to be user setable).5
**
**	Called by "logstat dynamic".
**
**	Currently does not exit gracefully on UNIX, since all the dmf programs
**	ignore ^C, so you must issue a ^\ to exit it.  Probably needs a ^Y on
**	VMS.
**
**	This interface needs work before it is ever exposed to the user.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID.
**
** History:
**      23-mar-90 (mikem)
**          Created.
**	05-mar-1992 (mikem)
**	    Change the display of logstat dynamic to handle 4 decimal TPS
**	    display (ie. 0-9999).  Also changed the display to support another
**	    decimal place in the #of xacts field.
**	15-Mar-2006 (jenjo02)
**	    Changed to display the more interesting "LASTBUF" and
**	    "FORCED_IO" stats instead of boring stalls.
**	12-Oct-2009 (smeke01) b122682
**	    Added periodic check for interrupts, in particular to enable
**	    Ctrl-C to work on Windows.
*/
static VOID    
log_dyn_lg_stats(VOID)
{
    CL_ERR_DESC	    sys_err; 
    LG_STAT	    stat;
    LG_STAT	    stat_avg;
    LG_STAT_QUEUE   stat_queue;
    i4	    status;
    i4	    length;
    i4		    i, j;
    i4	    err_code;

    init_queue(&stat_queue);
    i = 0;

    for (;;)
    {
	/* Check for interrupts */
	EXinterrupt(EX_DELIVER);

	status = LGshow(LG_S_STAT, (PTR)&stat, sizeof(stat), &length, &sys_err);

	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show logging statistics.\n");
	    return;
	}

	stat_insert(&stat, &stat_queue);

	get_avg(&stat_queue, &stat_avg);

	if (i <= 0)
	{
	    TRdisplay("log_ends   tps    readio writeio LGwrite forces waits lbwait fiwaits group gcount kbpb\n");
	    i = TERMINAL_LENGTH;
	}
	else
	{
	    i--;
	}
	TRdisplay("%9.1f %6.1f %6.1f %7.1f %7.1f %6.1f %5.1f %6.1f %7.1f %5.1f %6.1f %5.1f\n",
	    (((f8) stat.lgs_end)),
	    (((f8) stat_avg.lgs_end) / 100),
	    (((f8) stat_avg.lgs_readio) / 100),
	    (((f8) stat_avg.lgs_writeio) / 100),
	    (((f8) stat_avg.lgs_write) / 100),
	    (((f8) stat_avg.lgs_force) / 100),
	    (((f8) stat_avg.lgs_wait[LG_WAIT_ALL]) / 100),
	    (((f8) stat_avg.lgs_wait[LG_WAIT_LASTBUF]) / 100),
	    (((f8) stat_avg.lgs_wait[LG_WAIT_FORCED_IO]) / 100),
	    (((f8) stat_avg.lgs_group) / 100),
	    (((f8) stat_avg.lgs_group_count) / 100),
	    (stat_avg.lgs_writeio == 0 ? (f8) 0.0 :
	     (((((f8) stat_avg.lgs_kbytes) / 1) / (f8) stat_avg.lgs_writeio))));

	for (j = 0; j < SECS_PER_SAMPLE; j++)
	{
		/* Check for interrupts */
		EXinterrupt(EX_DELIVER);
		PCsleep(1000);
	}

    }
}

/*{
** Name: init_queue()	- initialize the lg stat queue.
**
** Description:
**	Simple "queue" init routine for the array of stats.  Start it out
**	empty.
**
** Inputs:
**	stat_queue			pointer to caller allocated queue.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID.
**
** History:
**      23-mar-90 (mikem)
**          Created.
*/
static VOID
init_queue(
LG_STAT_QUEUE	*stat_queue)
{
    MEfill(sizeof(LG_STAT_QUEUE), 0, stat_queue);
    stat_queue->lgq_head = 0;
    stat_queue->lgq_tail = 0;
    stat_queue->lgq_count = 0;
}

/*{
** Name: stat_insert()	- put a stat on the queue.
**
** Description:
**	Place stat at head of queue, delete oldest stat if queue is full.
**
** Inputs:
**	stat				new stat to put on the queue.
**	stat_queue			the queue of stats.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID.
**
** History:
**      23-mar-90 (mikem)
**          Created.
*/
static VOID
stat_insert(
LG_STAT		*stat,
LG_STAT_QUEUE	*stat_queue)
{
    if (stat_queue->lgq_count > MAX_STAT + 1)
	SIprintf("count to big\n");

    /* place stat at tail of queue, remove oldest stat from head if necessary */

    if (stat_queue->lgq_count == (MAX_STAT + 1))
    {
	/* queue full */

	/* delete top of queue (move head forward) */

	if (stat_queue->lgq_head == MAX_STAT)
	    stat_queue->lgq_head = 0;
	else
	    stat_queue->lgq_head++;
	stat_queue->lgq_count--;
    }

    /* move tail forward to new space for new stat */

    if (stat_queue->lgq_count != 0)
    {
	if (stat_queue->lgq_tail == MAX_STAT)
	    stat_queue->lgq_tail = 0;
	else
	    stat_queue->lgq_tail++;
    }

    /* place the new stat at tail of queue */
    STRUCT_ASSIGN_MACRO(*stat, stat_queue->lgq_stats[stat_queue->lgq_tail]);

    stat_queue->lgq_count++;
}

/*{
** Name: get_avg()	- return avg of all stats in the queue of stats.
**
** Description:
**	Given a pointer to an LG_STAT_QUEUE (a queue of LG_STAT's) return average 
**	of each stat in a single LG_STAT structure also provided by the caller.
**	The averages are returned as integers after their values have been 
**	multiplied by 100 (to give the integer two decimals of precision).  The
**	caller should divide by 100 before displaying the stat.
**
**	Averages are in units of the stat per second.
**
** Inputs:
**	stat_queue			caller provided queue of LG_STAT's
**
** Outputs:
**	avg_stat			caller provided output buf for averages.
**
**	Returns:
**	    VOID.
**
** History:
**      23-mar-90 (mikem)
**          Created.
**	05-mar-1992 (mikem)
**	    Fix logstat dynamic to display the
**	    correct values for the # of LG writes per second.  This value
**	    was being set to the # of timer writes per second.
*/
static VOID
get_avg(
LG_STAT_QUEUE	*stat_queue,
LG_STAT		*avg_stat)
{
    LG_STAT	*begin, *end;
    f4		secs;

    end   = &stat_queue->lgq_stats[stat_queue->lgq_tail];
    begin = &stat_queue->lgq_stats[stat_queue->lgq_head];

    secs = (stat_queue->lgq_count - 1) * SECS_PER_SAMPLE;

    if (stat_queue->lgq_count <= 1)
    {
	/* set them all to 0 and return */
	MEfill(sizeof(LG_STAT), 0, avg_stat);

	return;
    }

    /* calculate end - beginning stats to determine average.  As a hack
    ** we will calculate the avg/sec in a float then multiply it by 100
    ** and store it in a LG_STAT which is struct of i4s.  The display
    ** function will divide the i4 by 100 and display it as a float with
    ** 1 decimal of precision.
    */
    avg_stat->lgs_add = 
        (((f4) (end->lgs_add - begin->lgs_add) / secs) * 100);
    avg_stat->lgs_remove = 
        (((f4) (end->lgs_remove - begin->lgs_remove) / secs) * 100);
    avg_stat->lgs_begin = 
        (((f4) (end->lgs_begin - begin->lgs_begin) / secs) * 100);
    avg_stat->lgs_end = 
        (((f4) (end->lgs_end - begin->lgs_end) / secs) * 100);
    avg_stat->lgs_readio = 
        (((f4) (end->lgs_readio - begin->lgs_readio) / secs) * 100);
    avg_stat->lgs_writeio = 
        (((f4) (end->lgs_writeio - begin->lgs_writeio) / secs) * 100);
    avg_stat->lgs_write = 
        (((f4) (end->lgs_write - begin->lgs_write) / secs) * 100);
    avg_stat->lgs_force = 
        (((f4) (end->lgs_force - begin->lgs_force) / secs) * 100);
    avg_stat->lgs_wait[LG_WAIT_ALL] = 
        (((f4) (end->lgs_wait[LG_WAIT_ALL] - 
		begin->lgs_wait[LG_WAIT_ALL]) / secs) * 100);
    avg_stat->lgs_split = 
        (((f4) (end->lgs_split - begin->lgs_split) / secs) * 100);
    avg_stat->lgs_group = 
        (((f4) (end->lgs_group - begin->lgs_group) / secs) * 100);
    avg_stat->lgs_group_count = 
        (((f4) (end->lgs_group_count - begin->lgs_group_count) / secs) * 100);
    avg_stat->lgs_inconsist_db = 
        (((f4) (end->lgs_inconsist_db - begin->lgs_inconsist_db) / secs) * 100);
    avg_stat->lgs_check_timer = 
        (((f4) (end->lgs_check_timer - begin->lgs_check_timer) / secs) * 100);
    avg_stat->lgs_timer_write = 
        (((f4) (end->lgs_timer_write - begin->lgs_timer_write) / secs) * 100);
    avg_stat->lgs_kbytes = 
        (((f4) (end->lgs_kbytes - begin->lgs_kbytes) / secs) * 100);
    avg_stat->lgs_wait[LG_WAIT_LASTBUF] = 
        (((f4) (end->lgs_wait[LG_WAIT_LASTBUF] - 
		begin->lgs_wait[LG_WAIT_LASTBUF]) / secs) * 100);
    avg_stat->lgs_wait[LG_WAIT_FORCED_IO] = 
        (((f4) (end->lgs_wait[LG_WAIT_FORCED_IO] - 
		begin->lgs_wait[LG_WAIT_FORCED_IO]) / secs) * 100);

    return;
}
# ifndef II_DMF_MERGE
static STATUS
ex_handler(
EX_ARGS     *ex_args)
{
    return (EXDECLARE);
}
# endif

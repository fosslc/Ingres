/*
** Copyright (c) 1986, 2008 Ingres Corporation
**
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <ep.h>
#include    <ex.h>
#include    <tr.h>
#include    <lo.h>
#include    <si.h>
#include    <me.h>
#include    <nm.h>
#include    <cm.h>
#include    <er.h>
#include    <pc.h>
#include    <st.h>
#include    <cv.h>
#include    <si.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <pm.h>
#include    <scf.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lgkparms.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmve.h>
#include    <dmfrcp.h>
#include    <lgclustr.h>
#include    <lgdef.h>
#include    <lgdstat.h>
 
/*
NO_OPTIM = i64_aix
*/

/*
** Definition of all global variables owned by this file.
*/

/* Static function prototype definitions. */

static STATUS _rcpconfig(
    i4		argc,
    char	*argv[]);

static VOID _log_dump_err(
    CL_ERR_DESC  *sys_err,
    i4	*err_code);

static STATUS enable_dual_log(
    i4	action,
    char	*nodename,
    LG_LXID	lx_id);

static STATUS disable_dual_log(i4 action);

static STATUS open_log_as_master(
    LG_LGID		*lg_id,
    char		*nodename,
    i4			nodename_l,
    i4		*bcnt,
    LG_DBID		*db_id,
    i4		*block_size,
    i4			logs_to_erase);

static STATUS rcpconf_parse_command_line(
    i4		argc,
    char		*argv[],
    LG_RECOVER	*recov_struct);

static STATUS rcpconf_action_on_another_node( VOID );

static DB_STATUS parse_lsn(
    char	    *cmd_line,
    LG_LSN	    *lsn);

/**
**  Name: RCPCONFIG.C - DMF Recovery process utility program.
**
**  Description:
**	    This file contains routines to perform various recovery-related
**	    utility operations.
**
**  History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	02-oct-1988 (greg)
**	    add dummy main to get PCexit to work cheaply
**	    "clean-up" messages
**	31-oct-1988 (mmm)
**	    Deleted "SETUID" hint.  The ACP, RCP, and rcpconfig all must be
**	    run by the ingres user.  Making them setuid creates a security
**	    problem in the system.  A better long term solution would be to
**	    have these programs check whether the person running them
**	    is a super-user.  The install script on unix should be changed to
**	    make these programs mode 4700, so that only ingres can run them
**	    and if root happens to run them they will still act as if ingres
**	    started them.
**	05-jan-1989 (mmm)
**	    bug #4405.  We must call CSinitiate(0, 0, 0) from any program 
**	    which will connect to the locking system or logging system.
**	05-jan-1989 (mmm)
**	    Better error message for the "Can't open log file" error message.
**	28-aug-1989 (greg)
**	    add /force_init to usage message
**	31-aug-1989 (sandyh)
**	    bring defaults into line with the installation guides & scripts
**      28-Sep-1989 (ac)
**          Added dual logging support.
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**	18-jan-1989 (greg)
**	    Reapply LOST changes (cms class term_1+ skipped 46,47,48):
**		bug 7785 - /force_init error message		28-aug-1989
**		default values same as I&O guide (sandyh)	31-aug-1989
**	23-mar-1990 (mikem)
**	    On unix we can handle 64k log buffers, and actually may need
** 	    then that big to get enough log I/O througput for > 100tps.
** 	    For now accept 64, but don't tell the user we allow it.
**	8-may-90 (bryanp)
**	    Made a number of changes to improve error-handling: check return
**	    codes from CL functions like NMloc & TRset_file, handle EOF from
**	    TRrequest, and don't leave uninitialized data in rcpconfig.dat
**	    when re-creating it.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	25-sep-1991 (mikem) integrated following change: 12-jun-1991 (bryanp)
**	    Improve msg when force_init is required (35948, 33521).
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**	    - Change read-in character set support to use II_CHARSET symbols.
**	    - Bug# 37166.  Improve the SHUTDOWN message.
**	18-oct-1991 (mikem)
**	    Added ifdef UNIX code around dual logging code.  Dual logging will
**	    be supported on UNIX when bryanp integrates his project.
**	30-oct-1991 (bryanp)
**	    Dual logging changes.
**	    The RCP and RCPCONFIG no longer attempt to keep track of the current
**	    dual logging system state in the rcpconfig.dat file. Instead, we
**	    need only keep track of *configuration* information in this file,
**	    such as whether or not the user has requested dual logging.
**	    Allow the dual logging state to be disabled even if the RCP is down.
**	    Call LGalter(LG_A_COPY) even if the RCP is down.
**	    Added Cluster Dual Logging Support.
**
**	    De-support the VMS-only "/" syntax for parameters. Only "-" now.
**	14-feb-1992 (stevet)
**	    Move read-in character set codes after TRset_file() so that
**	    error message can be printed with correct headings.
**	18-feb-1992 (bryanp)
**	    Bugfix -- missing "&" on an LGalter() call.
**	20-may-1992 (bryanp)
**	    B44355: Pass proper parameters to DM9018.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**      18-jun-1992 (jnash)
**          Append to end of rcpconfig.log log via TR_F_APPEND TRset_file() 
**	    flag (UNIX only).
**	25-aug-1992 (ananth)
**	    Get rid of HP compiler warnings.
**	22-sep-1992 (bryanp)
**	    rcpconfig/patch is dead.
**	    Check status from CSinitiate().
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-1992 (jnash) merged 14-dec-1992 (walt)
**	    Include ex.h for Alpha.
**	18-jan-1993 (bryanp)
**	    Massively rewritten for the new PM-based configuration system.
**	9-feb-1993 (bryanp/andys/keving)
**	    Cluster 6.5 Project:
**		Brought "cant reinitialize" message up to date.
**		Added undocumented SILENT flag to stop trace messages
**		    once args are accepted.
**		Replaced call to dmr_get_lglk_parms with get_lgk_config
**		Added include of lgkparms.h to pick up RCONF definition
**		Change name of get_lgk_config to lgk_get_config.
**	15-may-1993 (rmuth)
**	    Cluster 6.5 Project.
**	        Add support for "-node nodename" option on the initialise
**	        log file options. 
**		- Add a duluxe command line parser
**	24-may-1993 (bryanp)
**	    LGopen now returns block count in 2048 byte units.
**	21-jun-1993 (andys)
**	    Add code to implement "-node nodename" option. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	23-aug-1993 (jnash)
**	     Support command line -help.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	20-sep-1993 (rogerk)
**	    Moved 6.4 changes to 6.5.
**
**	    20-feb-1992 (seg)
**		Truncate rcpconfig.log filename on OS2 since the filesystem
**		does not support filenames longer than 8.3.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called by scf_call(SCF_MALLOC)), 
**	    so dump them.
**	21-feb-1994 (bryanp) B57873
**	    make -[force_]init_{log,dual} be "additive", such that:
**		1) for _log, if the dual is currently enabled, then leave it
**		    enabled, resulting in both logs becoming inuse.
**		2) for _dual, if the primary is currently enabled, then leave
**		    it enabled, resulting in both logs becoming inuse.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	26-Apr-1999 (ahaal01)
**		Added NO_OPTIM for AIX (rs4_us5) to enable the transaction
**		log file header to be written when installing Ingres.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call   
**          in _rcpconfig() routine.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	25-sep-2002 (devjo01)
**	    Establish NUMA context.  Help cleanup.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      05-Nov-2004 (hweho01)
**          Removed rs4_us5 from NO_OPTIM list, it is no longer needed.
**	25-Apr-2005 (mutma03)
**	    Enabled force_init opterations for remote node rcpconfig. 
**	09-jun-2005 (abbjo03)
**	    Use a local buffer after calling LOtos.
**	05-Jan-2007 (bonro01)
**	    Change Getting Started Guide to Installation Guide.
**      26-Apr-2007 (hanal04) SIR 118199
**          Add timestamps to rcpconfig.log entries to help support.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
**	07-jan-2008 (joea)
**	    Discontinue use of cluster nicknames.
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      31-jan-2008 (stial01)
**          Added -rcp_stop_lsn, to set II_DMFRCP_STOP_ON_INCONS_DB
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**      05-may-2009
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      15-jun-2009 (stial01)
**          remove unsupported options from help
**	26-Aug-2009 (kschendel) 121804
**	    Need lgdef.h to satisfy gcc 4.3.  Quickie hack to "build file
**	    name" call args, to conform to lgdef.h and the rest of lg.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	02-Nov-2010 (jonj) SIR 124685
**	    Return int instead of VOID to agree with function prototype.
*/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	rcpconfig_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*
**      mkming hints
**
NEEDLIBS =      DMFLIB SCFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

OWNER =         INGUSER

PROGRAM =       rcpconfig

UNDEFS =        scf_call

MODE =		PARTIAL_LD
**
**/


#define			    RCF_NOTHING_SELECTED    0
#define			    RCF_SHUTDOWN	    1
#define			    RCF_INIT_PRIMARY	    2
#define			    RCF_FORCE_INIT_PRIMARY  3
#define			    RCF_INIT_DUAL	    4
#define			    RCF_FORCE_INIT_DUAL	    5
#define			    RCF_ENABLE_PRIMARY	    6
#define			    RCF_DISABLE_PRIMARY	    7
#define			    RCF_ENABLE_DUAL	    8
#define			    RCF_DISABLE_DUAL	    9
#define			    RCF_INIT		    10
#define			    RCF_FORCE_INIT	    11
#define			    RCF_HELP	    	    12
#define			    RCF_IGNORE_DB	    13
#define			    RCF_IGNORE_TABLE	    14
#define			    RCF_IGNORE_LSN	    15
#define			    RCF_STOP_LSN	    16

static i4 rcf_action = RCF_NOTHING_SELECTED;
static i4 shutdown_action;
static bool    silent = FALSE;
static bool    cluster = FALSE;
static bool    another_node = FALSE;

#define		MAX_NODE_SIZE	(CX_MAX_NODE_NAME_LEN + 1)
static  char	host_nodename[ MAX_NODE_SIZE ];
static  char	req_nodename [ MAX_NODE_SIZE ];
static  char    *log_nodename;

/*
** Define trace file name.
*/
#ifdef OS2
# define RCPLOG_NAME "rcpconfi.log"
#else
# define RCPLOG_NAME "rcpconfig.log"
#endif

static DB_STATUS dual_currently_enabled(void);
static DB_STATUS primary_currently_enabled(void);
static void rcpconfig_usage(i4 msg_index);

/*
** This message is issued when LGerase() returns LG_INITHEADER, which
** indicates that the log file could not be erased because it may contain
** transactions which still require recovery.
**
** Aside: THere are two places in this message where we want a blank line.
** Many current CL implementations do not print a zero-length line, so the
** obvious tricks of ending the preceding line with "\n\n" or just having
** "\n" do not work. So the blank space is indeed required.
*/
static char *force_init_required_msg[] = {
"An error has been encountered in running rcpconfig -init.\n",
"rcpconfig was unable to initialize the log file because there\n",
"are still active transactions in the log file which must be\n",
"recovered. Please:\n",
"    1) Start Ingres without the -init option to perform recovery\n",
"       and clear all transaction information from the Ingres log\n",
"       file.\n",
"    2) Shut Ingres down normally.\n",
"    3) Re-run rcpconfig with the -init flag.\n",
"Alternatively, run rcpconfig with the -force_init option to forcibly\n",
"initialize the log file and erase the active transactions.\n",
" \n",
"   DANGER: This will make inconsistent all databases which were open\n",
"           at the time the system was last running!\n",
" \n",
"For more information, consult the Ingres Database Administrator's\n",
"Guide and the Ingres Installation Guide.\n",
0	/* trailing 0 marks end of the message */
};
/*
** Usage message
*/
static char *rcpconfig_usage_msg[] = {
"rcpconfig: invalid arguments. Acceptable arguments are:\n",
"     rcpconfig%s -disable_dual    [-silent]\n",
"     rcpconfig%s -disable_log     [-silent]\n",
"     rcpconfig%s -enable_dual     [-silent]\n",
"     rcpconfig%s -enable_log      [-silent]\n",
"     rcpconfig%s -force_init      [-silent]\n",
"     rcpconfig%s -force_init_log  [-silent]\n",
"     rcpconfig%s -force_init_dual [-silent]\n",
"     rcpconfig%s -imm_shutdown    [-silent]\n",
"     rcpconfig%s -init            [-silent] [-node nodename]\n",
"     rcpconfig%s -init_log        [-silent] [-node nodename]\n",
"     rcpconfig%s -init_dual       [-silent] [-node nodename]\n",
"     rcpconfig%s -shutdown        [-silent]\n",
0	/* trailing 0 marks end of the message */
};

/*{
** Name: main	- Main program
**
** Description:
**      This is the main entry point of the RCPCONFIG.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    Uses PCexit() to indicate SUCCESS/FAILURE to installation program
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Create/update the II_CONFIG:rcpconfig.dat file.
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	19-Sept-1988 (Edhsu)
**	    Added validation for force abort, logfull and cp value.
**	02-oct-1988 (greg)
**	    add dummy main to get PCexit to work cheaply
**	05-jan-1989 (mmm)
**	    bug #4405.  We must call CSinitiate(0, 0, 0) from any program 
**	    which will connect to the logging system or logging system.
**	05-jan-1989 (mmm)
**	    Better error message for the "Can't open log file" error message.
**      28-Sep-1989 (ac)
**          Added dual logging support.
**	23-mar-1990 (mikem)
**	    On unix we can handle 64k log buffers, and actually may need
** 	    then that big to get enough log I/O througput for > 100tps.
** 	    For now accept 64, but don't tell the user we allow it.
**	8-may-90 (bryanp)
**	    Made a number of changes to improve error-handling: check return
**	    codes from CL functions like NMloc & TRset_file, handle EOF from
**	    TRrequest, and don't leave uninitialized data in rcpconfig.dat
**	    when re-creating it.
**	    When running /config, if the rcpconfig.dat file does not exist,
**	    then we must prompt for a new LDBS value so that we don't leave
**	    un-initialized data in the file. 
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	25-sep-1991 (mikem) integrated following change: 12-jun-1991 (bryanp)
**	    Improve msg when force_init is required (35948, 33521).
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**      25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**          - Change read-in character set support to use II_CHARSET symbols.
**          - Bug# 37166. Improve the SHUTDOWN message.
**	18-oct-1991 (mikem)
**	    Added ifdef UNIX code around dual logging code.  Dual logging will
**	    be supported on UNIX when bryanp integrates his project.
**	30-oct-1991 (bryanp)
**	    The RCP and RCPCONFIG no longer attempt to keep track of the current
**	    dual logging system state in the rcpconfig.dat file. Instead, we
**	    need only keep track of *configuration* information in this file,
**	    such as whether or not the user has requested dual logging.
**	    Don't update rcpconfig.dat on /init or /force_init until the log
**	    file has been erased -- that way if LG_INITHEADER is returned from
**	    LGerase we don't accidentally change rcpconfig.dat.
**	14-feb-1992 (stevet)
**	    Move read-in character set codes after TRset_file() so that
**	    error message can be printed with correct headings.
**	20-may-1992 (bryanp)
**	    B44355: Pass proper parameters to DM9018.
**	18-jun-1992 (jnash)
**	    Append to end of rcpconfig.log log via TR_F_APPEND TRset_file() 
**	    flag.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	3-oct-1992 (bryanp)
**	    rcpconfig/patch is dead.
**	    Check status from CSinitiate().
**	    Make an scf_call() so that SCF & CS can get fully initialized.
**	18-jan-1993 (bryanp)
**	    Massively rewritten for the new PM-based configuration system.
**	9-feb-1993 (keving)
**	    Added undocumented SILENT flag to stop trace messages
**	    once args are accepted.
**	24-may-1993 (bryanp)
**	    LGopen now returns block count in 2048 byte units.
**	21-jun-1993 (andys)
**	    Add code to implement "-node nodename" option. 
**	    Delete some code that was never executed.
**	    Close logfile before we exit.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	23-aug-1993 (jnash)
**	     Support command line -help.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	20-sep-1993 (rogerk)
**	    Moved 6.4 changes to 6.5.
**	    Use new RCPLOG_NAME define instead of direct references to
**	    rcpconfig.log.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called by scf_call(SCF_MALLOC)), 
**	    so dump them.
**	21-feb-1994 (bryanp) B57873
**	    make -[force_]init_{log,dual} be "additive", such that:
**		1) for _log, if the dual is currently enabled, then leave it
**		    enabled, resulting in both logs becoming inuse.
**		2) for _dual, if the primary is currently enabled, then leave
**		    it enabled, resulting in both logs becoming inuse.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update erase call parameters.
*/

# ifdef	II_DMF_MERGE
int MAIN(argc, argv)
# else
int main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
# ifndef II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(_rcpconfig(argc, argv));

    /* NOTREACHED */
    return(FAIL);
}

static STATUS
_rcpconfig(
i4                  argc,
char		    *argv[])
{
    STATUS		status;
    LOCATION		loc;
    CL_ERR_DESC		sys_err;
    CL_ERR_DESC         cl_err;
    char		buffer[12];
    i4			force_flag;
    i4			logs_to_initialize;
    i4		amount_read;
    i4		block_size;
    static FILE		*fp;
    i4		err_code;
    i4		bcnt;
    LG_LGID		lg_id;
    LG_LXID		lx_id;
    LG_DBID		db_id;
    DB_TRAN_ID		tran_id;
    LG_HEADER		header; 
    i4			length;
    char                node_name[LGC_MAX_NODE_NAME_LEN];
    DB_OWN_NAME		user_name;
    bool		bad_para = FALSE;
    bool		data_file_created = FALSE;
    i4			real_arg = 1;
    SCF_CB		scf_cb;
    LG_HDR_ALTER_SHOW	alter_parms;
    i4			open_node_l;	/* length of open_node		*/
    char		*open_node;	/* To open logfile. May be 0	*/
    char		*local_host;
    i4			rad;
    LG_RECOVER	recov_struct;
    LG_RECOVER  recov_save;
    LG_RECOVER  recov_init;

    /*
    ** Set interactive input/output file.
    */
 
    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
			     &sys_err);

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if (CXcluster_enabled())
    {
	cluster = TRUE;

	CXnode_name(host_nodename);
	log_nodename = host_nodename;
	ule_initiate(host_nodename, STlength(host_nodename), "RCPCONFIG", 9);

	STcopy( host_nodename, req_nodename );
    }    
    else
    {
	cluster = FALSE;
	STcopy( "Current", host_nodename);
	STcopy( host_nodename, req_nodename );
	log_nodename = NULL;
	ule_initiate(0, 0, "RCPCONFIG", 9);
    }

    if ( status )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	return (FAIL);
    }

    /* Set the proper character set for CM */

    status = CMset_charset(&cl_err);
    if ( status != OK)
    {
	uleFormat(NULL, E_UL0016_CHAR_INIT, (CL_ERR_DESC *)&cl_err, ULE_LOG , 
	    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(FAIL);
    }

    if ( argc < 2 )
    {
	/* quick exit if no params set */
	rcpconfig_usage(0);
	return(FAIL);
    }

    /* Open II_DBMS_CONFIG!rcpconfig.log */
    {
	char    logname[MAX_LOC+1];
	char	*path;
	i4	l_path;
	i4	flag;

	if ((status = NMloc(UTILITY, FILENAME, RCPLOG_NAME, &loc)) != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	    TRdisplay("%@ Error while building path/filename to %s\n",RCPLOG_NAME);
	    return (FAIL);
	}

	LOtos(&loc, &path);
	STcopy(path, logname);
	l_path = STlength(logname);

	/*
	** Append to the end of existing log file on UNIX, have 
	** VMS versioning to do the equivalent thing.
	*/
#ifdef VMS
	flag = TR_F_OPEN;
#else
	flag = TR_F_APPEND;
#endif

	if ((status = TRset_file(flag, logname, l_path, &sys_err)) != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	    TRdisplay("%@ Error while opening %s\n", RCPLOG_NAME);
	    return (FAIL);
	}
    }
 
    /*
    ** Process the input parameter. The rcpconfig tool supports the following
    ** parameters:
    **
    **	    -init_log
    **	    -force_init_log
    **	    -init_dual
    **	    -force_init_dual
    **	    -enable_log
    **	    -disable_log
    **	    -enable_dual
    **	    -disable_dual
    **	    -shutdown
    **	    -imm_shutdown
    **	    -init
    **	    -force_init
    **	    -node node_name
    **	In addition there may be a -silent flag
    */

    if ( rcpconf_parse_command_line( argc, argv, &recov_struct ) != OK )
    {
	bad_para = TRUE;
    }

    if (bad_para)
    {
	rcpconfig_usage(1);
	return (FAIL);
    }

    /*
    ** Service -help request
    */
    if (rcf_action == RCF_HELP)
    {
	rcpconfig_usage(0);
	return (OK);
    }

    if (silent)
    {
	/* Close TR file to stop TR calls producing anything */
        status = TRset_file(TR_T_CLOSE, TR_OUTPUT, TR_L_OUTPUT,
    			     &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    return (FAIL);
	}


    }


    /*
    ** bug #4405
    ** on unix logstat acts as a "pseudo-server" in that it must connect to
    ** the "event system" when it uses LG.  The CSinitiate call is necessary
    ** to initialize this, and to set up a cleanup routine which will free
    ** a slot in the system shared memory segment when the program exits.
    **
    ** In addition to a simple CSinitiate() call, however, there are a number
    ** of other various CS setup tasks which must be performed. The simplest
    ** way to perform these tasks is to make an scf_call(), which performs all
    ** the various initialization (CSinitiate, CSset_sid, etc.). We don't
    ** actually care about the memory we're allocating, we just want the
    ** side-effects of calling scf_call. If we skip the scf_call, then
    ** CSp_semaphore doesn't work correctly because it can't locate the current
    ** "session".
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
	char	    	error_buffer[ER_MAX_LEN];
	i4 	error_length;
	i4 	count;

	uleFormat(NULL, status, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	    &err_code, 0);
	error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);
	return (FAIL);
    }

    /*
    ** Initialize the Logging system.
    **
    ** Must be done before 'rcpconf_action_on_another_node' is called
    ** in order for CX to be initialized, and any_csp check to work.
    */
    if (LGinitialize(&sys_err, ERx("rcpconfig")))
    {
	_log_dump_err(&sys_err, &err_code);
	return(FAIL);
    }

    /*
    ** If the user has requested action on another node then 
    ** call routine which checks if this is valid.
    */
    if (another_node)
    {
	status = rcpconf_action_on_another_node();
	if (status != OK)
	    return( status );
	open_node = req_nodename;
	open_node_l = STlength(req_nodename);
    }
    else
    {
	open_node = 0;
	open_node_l = 0;
    }

    /*
    ** Now start handling different RCPCONFIG actions. Take care of some of
    ** the simple ones, like shutdown first.
    */

    if (rcf_action == RCF_SHUTDOWN)
    {
	if (status = LGalter(LG_A_SHUTDOWN, (PTR)&shutdown_action,
				sizeof(shutdown_action), &sys_err))
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L,
			(i4 *)NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, LG_A_SHUTDOWN);

	    TRdisplay("%@ Can't Alter the state of the logging system\n\n");
	    return (FAIL);
	}
	TRdisplay("%@ rcpconfig: the %s request has been delivered to \n", argv[real_arg]);
	TRdisplay("%@            the logging system.  Use LOGSTAT to check for success \n");
	TRdisplay("%@            A WILLING_COMMIT transaction will prevent SHUTDOWN.\n");
	return(OK);
    }

    if (rcf_action == RCF_ENABLE_PRIMARY || rcf_action == RCF_ENABLE_DUAL)
    {
	if (enable_dual_log(rcf_action, open_node, 0))
	{
	    TRdisplay("%@ Can't turn on dual logging\n");
	    return (FAIL);
	}
	return(OK);
    }

    if (rcf_action == RCF_DISABLE_PRIMARY || rcf_action == RCF_DISABLE_DUAL)
    {
	if (disable_dual_log(rcf_action))
	{
	    TRdisplay("%@ Can't turn off dual logging\n");
	    return(FAIL);
	}
	return(OK);
    }

    if (rcf_action == RCF_IGNORE_DB
	    || rcf_action == RCF_IGNORE_LSN
	    || rcf_action == RCF_IGNORE_TABLE
	    || rcf_action == RCF_STOP_LSN)
    {
	i4	rcp_flag;

	if (rcf_action == RCF_IGNORE_DB)
	    rcp_flag = LG_A_RCP_IGNORE_DB;
	else if (rcf_action == RCF_IGNORE_TABLE)
	    rcp_flag = LG_A_RCP_IGNORE_TABLE;
	else if (rcf_action == RCF_IGNORE_LSN)
	    rcp_flag = LG_A_RCP_IGNORE_LSN;
	else if (rcf_action == RCF_STOP_LSN)
	    rcp_flag = LG_A_RCP_STOP_LSN;

	/* Zero filled LG_RECOVER struct means it's not initialized */
	MEfill(sizeof(recov_init), 0, (char *)&recov_init);
	
	/* recov_struct is input & output parameter so save input values */
	STRUCT_ASSIGN_MACRO(recov_struct, recov_save);

	if (status = LGalter(rcp_flag, (PTR)&recov_struct,
				sizeof(recov_struct), &sys_err))
	{
	    if (MEcmp(&recov_init, &recov_struct, sizeof(LG_RECOVER)))
	    {
		TRdisplay(
	    "WARNING The RCP encountered an error and is waiting for input.\n");
		TRdisplay("Valid input is:\n");
		TRdisplay("rcpconfig -rcp_continue_ignore_db=%~t\n",
		    DB_DB_MAXNAME, recov_struct.lg_dbname);
		TRdisplay("OR\n");
		TRdisplay("rcpconfig -rcp_continue_ignore_table=%~t\n",
		    DB_TAB_MAXNAME, recov_struct.lg_tabname);
		TRdisplay("OR\n");
		TRdisplay("rcpconfig -rcp_continue_ignore_lsn=%d,%d\n",
		    recov_struct.lg_lsn.lsn_high,
		    recov_struct.lg_lsn.lsn_low);
		TRdisplay("OR\n");
		TRdisplay("rcpconfig -rcp_stop_lsn=%d,%d\n",
		    recov_struct.lg_lsn.lsn_high,
		    recov_struct.lg_lsn.lsn_low);
	    }
	    else
	    {
		TRdisplay("The RCP is not waiting for a RECOVER event.\n");
	    }

	    return (FAIL);
	}
	return(OK);
    }

    /*
    ** If we get to here, then we're initializing or force-initializing one
    ** of the log files
    */
    switch (rcf_action)
    {
	case RCF_INIT:
	    force_flag = 0;
	    logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    break;

	case RCF_INIT_PRIMARY:
	    force_flag = 0;
	    if (dual_currently_enabled() == E_DB_OK)
		logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    else
		logs_to_initialize = LG_ERASE_PRIMARY_ONLY;
	    break;

	case RCF_INIT_DUAL:
	    force_flag = 0;
	    if (primary_currently_enabled() == E_DB_OK)
		logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    else
		logs_to_initialize = LG_ERASE_DUAL_ONLY;
	    break;

	case RCF_FORCE_INIT:
	    force_flag = 1;
	    logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    break;

	case RCF_FORCE_INIT_PRIMARY:
	    force_flag = 1;
	    if (dual_currently_enabled() == E_DB_OK)
		logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    else
		logs_to_initialize = LG_ERASE_PRIMARY_ONLY;
	    break;

	case RCF_FORCE_INIT_DUAL:
	    force_flag = 1;
	    if (primary_currently_enabled() == E_DB_OK)
		logs_to_initialize = LG_ERASE_BOTH_LOGS;
	    else
		logs_to_initialize = LG_ERASE_DUAL_ONLY;
	    break;

	default:
	    TRdisplay("%@ THIS CODE HAS NOT BEEN WRITTEN!\n");
	    return (FAIL);
    }
	/*
	** The first time that the RCP uses a freshly-initialized log file,
	** we want it to write a bunch of records into the beginning of the
	** log file. In particular, by writing 2 consecutive Consistency Points,
	** the RCP ensures that some machine failure recovery algorithms will
	** work properly (dmr_get_cp() will always be able to find a good CP,
	** and header reconstruction will have enough written blocks to be
	** able to determine the blocksize if the header becomes unreadable).
	*/

    /*  Erase the log file. */
     
    if (open_log_as_master(&lg_id, open_node, open_node_l,
			    &bcnt, &db_id, &block_size,
			    logs_to_initialize))
    {
	TRdisplay("%@ Unable to open the logging system.\n");
	return (E_DB_ERROR);
    }

     /*
     ** Begin a transaction so that we can issue LGread for reading
     ** the log file.
     */
     STmove((PTR)DB_RCPCONFIG_INFO, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
     if (status = LGbegin(LG_NOPROTECT, db_id, &tran_id, &lx_id, 
		    sizeof(DB_OWN_NAME), &user_name.db_own_name[0], 
		    (DB_DIS_TRAN_ID*)NULL, &sys_err))
     {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, db_id);
 
	TRdisplay("%@ Can't begin a transaction in the logging system.\n\n");
	return (FAIL);
     }
 

     /*
     ** Initialize the log file header information in order for
     ** LGread to work.
     */
     MEfill(sizeof(header),0,(PTR)&header);
 
     /* bcnt is in unit of the minimum block size, 2kbytes. Actually, this
     ** is truly LGD_MIN_BLK_SIZE from lgdef.h, but we don't really want
     ** to include that file here.
     */

     header.lgh_count = bcnt / (block_size / 2048);
     header.lgh_size = block_size;
 
     alter_parms.lg_hdr_lg_header = header;
     alter_parms.lg_hdr_lg_id = lg_id;
     if (status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			    sizeof(alter_parms), &sys_err))
     {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		0, LG_A_NODELOG_HEADER);
 
	TRdisplay("%@ Can't alter the log header information.\n\n");
	return (FAIL);
     }
 
     /* Erase the log file */
 
     TRdisplay("%@ Erase the log files...\n\n");

     if (status = LGerase(&lg_id, header.lgh_count, block_size,
			    (bool)force_flag, logs_to_initialize,
			    &sys_err))
     {
	if (status == LG_INITHEADER)
	{
	    i4      msg_index;
	
	    for (msg_index=0; force_init_required_msg[msg_index] != 0; 
		 msg_index++)
	    {
		TRdisplay(force_init_required_msg[msg_index]);
	    }
	}
	else
	{
	    _VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM9018_BAD_LOG_ERASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		0, "Check II_LOG_FILE");
 
	    TRdisplay("%@ Can't initialize the log file\n");
	    TRdisplay("%@ \tConsult ii_files:errlog.log for more details.\n");
	}
	return(FAIL);
     }

     status = LGclose(lg_id, &sys_err);
     if (status != OK)
     {
	uleFormat(NULL, status, &sys_err, ULE_LOG,
		NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(FAIL);
     }
     TRdisplay("%@ rcpconfig -init completed successfully.\n"); 
     return(OK);
}

static VOID
_log_dump_err(
CL_ERR_DESC  *sys_err,
i4	    *err_code)
{
    (VOID) uleFormat(NULL, E_DM9011_BAD_LOG_INIT, sys_err, ULE_LOG, NULL,
		      (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
 
    TRdisplay("%@ Error occured initializing the logging system\n");
    TRdisplay("%@ \tPlease insure that the logging system and RCPCONFIG are properly configured\n");
# ifdef UNIX
    TRdisplay("%@ \tOnly the ingres super-user may run this program, any \n");
    TRdisplay("%@ \tother user running this program will get this error. \n");
    TRdisplay("%@ \tThis error may also occur if this program is run prior to\n");
    TRdisplay("%@ \tthe creation of the system shared memory segments (created\n");
    TRdisplay("%@ \tby either the iibuild program or the csinstall utility).\n");
# endif
}

/*{
** Name: Enable_dual_log - Enable dual logging.
**
** Description:
**	This routine handles the request of enabling dual logging. If dual
**	logging is not already enabled, we record the enabling of dual logging
**	in rcpconfig.dat. In addition, if only one of the two log files is
**	currently active, we copy the active log file to the inactive log file.
**
**	We notify the logging system (via LGalter(LG_A_COPY)) before we start
**	the copy and after we complete it. This allows the logging system to
**	know when we are performing a copy. We MUST call LGalter(LG_A_COPY)
**	even if the copy itself fails.
**
** Inputs:
**	    lx_id			    log id of the process that opened
**					    the active file.
**	    nodename			    nodename to show which logfile(s)
**					    to use (cluster). May be NULL to
**					    get default (own) nodename.
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Oct-1989 (ac)
**	    Created.
**	5-jul-1990 (bryanp)
**	    A few enhancements to allow for re-enabling dual logging after a
**	    dynamic disabling has occurred.
**	    Call LGalter(LG_A_COPY) even if the RCP is down.
**	12-dec-1991 (bryanp)
**	    If the RCP is down, and we haven't yet opened the logfile
**	    ourselves, we must call open_log_as_master().
**	16-dec-1991 (bryanp)
**	    Changes from CL committee review:
**	    1) Don't need to pass lx_id to LGcopy().
**	    2) We don't support re-activating dual logging while ONLINE.
**	    3) We don't need LGalter(LG_A_COPY), due to change (2).
**	21-jun-1993 (andys)
**	    Add nodename parameter so that we can pass nodename to LGcopy.
**	    We need to do this to add support for rcpconfig -node XX in
**	    the cluster.
*/
static STATUS
enable_dual_log(
i4		action,
char		*nodename,
LG_LXID		lx_id)
{
    i4	    on = 1;
    i4	    off = 0;
    i4	    lgd_status;
    i4	    length;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    STATUS	    status;
    RCONF	    rconf;
    i4	    bsize;
    i4	    active_log;
    LG_LGID	    lg_id;
    LG_DBID	    db_id;
    i4	    bcnt;

#if 0
    if (rconf.dual_logging)
    {
	TRdisplay("%@ Dual logging is currently enabled\n");
	/*
	** Note that this is NOT an error. They may be re-enabling dual logging
	** after a dynamic log file disable. Just keep on processing...
	*/
    }
#endif

    if (status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
                        &length, &sys_err))
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_LGSTS);
	TRdisplay("%@ Can't show logging system status.\n");
	return(E_DB_ERROR);
    }

    if (lgd_status & 1)
    {
	TRdisplay("%@ Can't enable dual logging while logging system is online\n");
	return(E_DB_OK);
    }

    if (lx_id == 0 && open_log_as_master(&lg_id, 0, 0, &bcnt, &db_id, 
						&bsize, LG_ERASE_BOTH_LOGS))
    {
	TRdisplay("%@ Can't open the logging system.\n");
	return (E_DB_ERROR);
    }
 	
    /* 
    ** Perform the copying and turn on the dual logging status.
    */

    /* Check which log is currently activce. */

    status = LGshow( LG_S_DUAL_LOGGING, (PTR)&active_log, sizeof(active_log),
			&length, &sys_err );

    if (status)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	_VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_DUAL_LOGGING);

	TRdisplay("%@ Can't show the current active log.\n");

	return (E_DB_ERROR);
    }
    if ((active_log == LG_PRIMARY_ACTIVE))
    {
	TRdisplay("%@ Copying II_LOG_FILE to II_DUAL_LOG...\n");
	status = LGcopy(LG_COPY_PRIM_TO_DUAL, nodename, &sys_err);
    }
    else if ((active_log == LG_DUAL_ACTIVE))
    {
	TRdisplay("%@ Copying II_DUAL_LOG to II_LOG_FILE...\n");
	status = LGcopy(LG_COPY_DUAL_TO_PRIM, nodename, &sys_err);
    }
    else if ((active_log == LG_BOTH_LOGS_ACTIVE))
    {
	TRdisplay("%@ Both log files are already active -- no copying needed\n");
#if 0
	if (rconf.dual_logging == 1)
	{
	    status = OK;
	}
	else
	{
	    TRdisplay("%@ Unexpected active log mask (%x)\n", active_log);

	    return (E_DB_ERROR);
	}
#endif
    }
    else
    {
	TRdisplay("%@ Unexpected active log mask (%x)\n", active_log);

	return (E_DB_ERROR);
    }

    if (status)
    {
	(VOID) uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM904E_BAD_LOG_COPY, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	TRdisplay("%@ Can't copy the log file to the dual log file.\n");

	return (E_DB_ERROR);
    }

    if (status = LGalter(LG_A_ENABLE_DUAL_LOGGING, (PTR)&lg_id, sizeof(lg_id),
			    &sys_err))
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	_VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code,
				1, 0, LG_A_ENABLE_DUAL_LOGGING);
 
	TRdisplay("%@ Can't enable dual logging in the logging system.\n");
	return (E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: Disable_dual_log - Disable dual logging.
**
** Description:
**	This routine handles the request of disabling dual logging.
**
**	It may seem curious that we enable dual logging, open the log file,
**	then disable dual logging. This is done because we need to get the
**	logging system to disable dual logging for us and record that fact
**	in the log file header, and the logging system must therefore be
**	instructed to open up both log files (if possible) and then to
**	disable dual logging and record that fact in both log file headers.
**	If we didn't enable dual logging and open both log files before we
**	disabled dual logging, the logging system wouldn't be able to
**	update the log file headers (since the log files wouldn't be open).
**
** Inputs:
**	    action			RCF_DISABLE_PRIMARY/RCF_DISABLE_DUAL
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Oct-1989 (ac)
**	    Created.
**	5-jul-1990 (bryanp)
**	    Allow the dual logging state to be disabled even if the RCP is down.
**	12-dec-1991 (bryanp)
**	    Share logfile opening code in open_log_as_master().
*/
static STATUS
disable_dual_log(i4 action)
{
    i4	    active_log;
    i4	    new_log;
    i4	    lgd_status;
    i4	    length;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    char	    buffer[2];
    i4	    amount_read;
    RCONF	    rconf;
    LG_LGID	    lg_id;
    i4	    bcnt;
    i4	    bsize;
    i4	    size = 64;
    STATUS	    status;
    i4	    alter_args[2];

#if 0
    if (rconf.dual_logging == 0)
    {
	TRdisplay("%@ Dual logging is not currently enabled\n");
	return(E_DB_OK);
    }
#endif
    
    /* Which log file to be disabled ? */

    if (action == RCF_DISABLE_PRIMARY)
	active_log = 0;
    else
	active_log = 1;

    /* Turn off the dual logging status in the logging system. */

    if (status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
                        &length, &sys_err))
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_LGSTS);
	TRdisplay("%@ Can't show logging system status.\n");
	return(E_DB_ERROR);
    }

    if (lgd_status & 1)
    {
	/* Turn off dual logging when the logging system is online. However,
	** refuse to turn off the only active log.
	*/
	if (status = LGshow(LG_S_DUAL_LOGGING, (PTR)&new_log, sizeof(new_log),
			    &length, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    _VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_DUAL_LOGGING);
	    TRdisplay("%@ Can't show the updated active log file information.\n");
	    return (E_DB_ERROR);
	}
	if ((active_log == 0 && new_log == LG_PRIMARY_ACTIVE) ||
	    (active_log == 1 && new_log == LG_DUAL_ACTIVE)     )
	{
	    TRdisplay("%@ Can't disable the only active log file.\n");
	    return (E_DB_ERROR);
	}

	alter_args[0] = 0;
	alter_args[1] = active_log;
	if (status = LGalter(LG_A_DISABLE_DUAL_LOGGING,
			    (PTR)alter_args, sizeof(alter_args),
			    &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
				0, LG_A_DISABLE_DUAL_LOGGING);
 
	    TRdisplay("%@ Can't disable dual logging in the logging system. \n\n");
	    return (E_DB_ERROR);
	}
    }
    else
    {
	if (open_log_as_master(&lg_id, 0, 0, &bcnt, (LG_DBID *)0, &bsize, 
						LG_ERASE_BOTH_LOGS))
	{
	    TRdisplay("%@ Can't open log as master.\n");
	    return (E_DB_ERROR);
	}

	if (status = LGshow(LG_S_DUAL_LOGGING,
			    (PTR)&new_log, sizeof(new_log), &length,
			    &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    _VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_DUAL_LOGGING);
	    TRdisplay("%@ Can't show the updated active log file information.\n");
	    return (E_DB_ERROR);
	}
	if ((active_log == 0 && new_log == LG_PRIMARY_ACTIVE) ||
	    (active_log == 1 && new_log == LG_DUAL_ACTIVE)     )
	{
	    TRdisplay("%@ Can't disable the only active log file.\n");
	    return (E_DB_ERROR);
	}

	alter_args[0] = lg_id;
	alter_args[1] = active_log;
	if (status = LGalter(LG_A_DISABLE_DUAL_LOGGING,
			    (PTR)alter_args, sizeof(alter_args),
			    &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    _VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code,
				1, 0, LG_A_DISABLE_DUAL_LOGGING);
 
	    TRdisplay("%@ Can't disable dual logging in the logging system. \n\n");
	    return (E_DB_ERROR);
	}
    }

#ifdef xDEBUG
    /*
    ** "mutual suspicion": did the logging system really disable the log?
    */
    if (status = LGshow(LG_S_DUAL_LOGGING, (PTR)&new_log, sizeof(new_log),
			&length, &sys_err))
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	_VOID_ uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, 
			LG_S_DUAL_LOGGING);
	TRdisplay("%@ Can't show the updated active log file information.\n");
	return (E_DB_ERROR);
    }
    if (active_log == 0 && new_log != LG_DUAL_ACTIVE)
    {
	TRdisplay("%@ Active log file mask is %x, instead of the expected %x\n",
		    new_log, LG_DUAL_ACTIVE);
	return (E_DB_ERROR);
    }
    if (active_log == 1 && new_log != LG_PRIMARY_ACTIVE)
    {
	TRdisplay("%@ Active log file mask is %x, instead of the expected %x\n",
		    new_log, LG_DUAL_ACTIVE);
	return (E_DB_ERROR);
    }

    TRdisplay("%@ Log file disabled successfully, active log mask is now %x.\n",
		new_log);
#endif

    return (E_DB_OK);
}

/*
** Name: open_log_as_master	- open the logging system using LG_MASTER
**
** Description:
**	For the operations
**	    -init
**	    -force_init
**	    -dual_log
**
**	We need to open the logging system as the master. This does a number
**	of things:
**	    single-threads things so that the RCP can't accidentally start
**	    while we're working.
**
**	    Reads the logfile headers and finds out the current state of the
**	    logfile.
**
**	    Notifies the logging system that we are privileged to perform all
**	    logging system operations.
**
** Inputs:
**	logs_to_erase	    LG_ERASE_PRIMARY_ONLY, LG_ERASE_DUAL_ONLY, or 
**			    LG_ERASE_BOTH_LOGS	    
**
**			    FALSE if we're going to erase the primary log
**	rconf		    Address of the RCONF structure
**
**	nodename	    which logfile to open in a cluster.
**			    NULL if default is to be used.
**
**	nodename_l	    Length of nodename.
**
** Outputs:
**	lg_id		Logging system LG_LGID
**	bcnt		Number of blocks in the log file(s)
**	db_id		Logging system LG_DBID
**	bsize		log file block size
**
** Returns:
**	OK
**	FAIL
**
** History:
**	12-dec-1991 (bryanp)
**	    Moved this code into its own subroutine so that -dual_log could
**	    share it with -init.
**	3-feb-1992 (bryanp)
**	    Bugfix -- missing "&" on an LGalter() call. Also, while I was here,
**	    improved the error handling to log the CL status as well as the
**	    mainline error code.
**	18-jan-1993 (bryanp)
**	    Added block_size parameter, removed rconf parameter.
**	21-jun-1993 (andys)
**	    ule_format error from lgk_get_config.
*/
static STATUS
open_log_as_master(
LG_LGID		*lg_id,
char		*nodename,
i4		nodename_l,
i4		*bcnt,
LG_DBID		*db_id,
i4		*block_size,
i4		logs_to_erase)
{
    i4	size = 64;
    CL_ERR_DESC	sys_err;
    i4	err_code;
    STATUS	status;
    DM0L_ADDDB	add_info;
    i4		l_add_info;
    RCONF		rconf;
    i4	local_log = 0;

    if (status = lgk_get_config(&rconf, &sys_err, TRUE /* verbose */))
    {
	TRdisplay("%@ Unable to retrieve logging and locking parameters.\n");
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return (E_DB_ERROR);
    }
    
    *block_size = rconf.bsize;

    /* Allocate dynamic memory in logging system. */
 
    if ((status = LGalter(LG_A_BLKS, (PTR)&size, sizeof(size), &sys_err)) != OK)
    {
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, LG_A_BLKS);
 
	TRdisplay("%@ Can't allocate dynamic memory in the logging system. Logging system is busy.\n\n");
	return (FAIL);
    }
 
    if ((status = LGalter(LG_A_LDBS, (PTR)&size, sizeof(size), &sys_err)) != OK)
    {
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, LG_A_LDBS);
 
	TRdisplay("%@ Can't allocate dynamic memory in the logging system. Logging system is busy\n\n");
	return (FAIL);
    }


    /*
    ** Open as master so that while erasing the log file, no one can
    ** access the logging system.
    */
 
    if (logs_to_erase == LG_ERASE_BOTH_LOGS)
    {
	/* Decide which log file to open. If the user has requested dual
	** logging, tell the logging system of this fact before trying to
	** open the log file(s).
	*/

	if ((status = LGalter(LG_A_ENABLE_DUAL_LOGGING,
			(PTR)&local_log, 
			sizeof(local_log), &sys_err)) != OK)
	{
	    (VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    _VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &err_code,
				1, 0, LG_A_ENABLE_DUAL_LOGGING);

	    TRdisplay("%@ Can't enable dual logging in the logging system.\n");
	    return (E_DB_ERROR);
	}

	/*
	** Open as master so that we can alter the log file status. Opening
	** the log file with the NOHEADER flag causes the LG code to open only
	** the primary log file. However, we wish to stamp BOTH log headers,
	** so we must open BOTH log files. Therefore, we use the
	** LG_HEADER_ONLY flag.
	*/

	status = LGopen(LG_MASTER | LG_HEADER_ONLY, nodename, nodename_l, 
			lg_id, bcnt, rconf.bsize, &sys_err);
    }
    else if (logs_to_erase == LG_ERASE_PRIMARY_ONLY)
    {
	status = LGopen(LG_MASTER | LG_LOG_INIT | LG_PRIMARY_ERASE, nodename, 
			nodename_l, lg_id, bcnt, rconf.bsize, &sys_err);
    }
    else if (logs_to_erase == LG_ERASE_DUAL_ONLY)
    {
	status = LGopen(LG_MASTER | LG_LOG_INIT | LG_DUAL_ERASE, nodename, 
			nodename_l, lg_id, bcnt, rconf.bsize, &sys_err);
    }
    else
    {
	status = LG_BADPARAM;
    }

    if (status)
    {
	_VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	(VOID) uleFormat(NULL, E_DM9012_BAD_LOG_OPEN, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	TRdisplay("%@ Can't open the log file. This error can be caused by\n");
	TRdisplay("%@ any of the following:\n");

	TRdisplay("%@ \t1) Log file does not exist.\n");
	TRdisplay("%@ \t2) Incorrect permissions or ownership of log file.\n");
	TRdisplay("%@ \t3) Incorrect permissions or ownership of executable.\n");
	TRdisplay("%@ \t4) Logging system busy (ie. acp, rcp, or dbms \n");
	TRdisplay("%@ \t   processes still running\n");

	return (FAIL);
     }

     if (db_id == (LG_DBID *)0)
	return (OK);

     STmove((PTR)DB_RCPCONFIG_INFO, ' ', DB_DB_MAXNAME, 
		(PTR) add_info.ad_dbname.db_db_name);
     MEcopy((PTR)DB_INGRES_NAME, DB_OWN_MAXNAME, 
		(PTR) add_info.ad_dbowner.db_own_name);
     MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
     add_info.ad_dbid = 0;
     add_info.ad_l_root = 4;
     l_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

     if (status = LGadd(*lg_id, 0, (char *)&add_info, l_add_info, db_id,
			&sys_err))
     {
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	(char *)NULL, 0L, (i4 *)NULL, &err_code, 4, 0, *lg_id,
	0, "$rcpconfig", 0, "$ingres", 0, "None");
 
	TRdisplay("%@ Can't add a database to the logging system.\n\n");
	return (FAIL);
     }

    return (OK);
}

/*
** Name: rcpconf_parse_command_line
**
** Description:
**	This routine parses the command line and builds any context
**	required by the operation requested.
**
** Inputs:
**	argc				Number of arguments
**	argv				Array of Ptr to arguments.
**      recov_struct			RESUME offline recovery args
**
** Outputs:
**	FAIL	- If an invalid argument specified
**	OK	- If all args are ok
**
** Side Effects:
**
** History:
**	15-may-1993 (rmuth)
**	    Created.
**	21-jun-1993 (andys)
**	    Use LG_cluster_node to verify nodename.
**	    Convert nodename to upper case to match name from LGcnode_info.
**	    Force shutdown_action to 1 in -shutdown case.
**	    In case 's' don't check if another action has been selected until
**	    after checking for -silent as that may be used in conjunction
**	    with other arguments.
**	23-aug-1993 (jnash)
**	     Support command line -help.
*/
static STATUS
rcpconf_parse_command_line(
i4		argc,
char		*argv[],
LG_RECOVER	*recov_struct)
{
    i4	i;
    char	*pvalue;
    bool	invalid_args = FALSE;
    DB_STATUS	status;

    /*
    ** Check for minimum number of args
    */
    if ( argc < 2 )
    {
	return(FAIL);
    }

    /*
    ** Service -help request.
    */
    if ((argc == 2) && (STcasecmp(&argv[1][0], "-help" ) == 0))
    {
	rcf_action = RCF_HELP;
	return(OK);
    }

    /*
    ** Parse the command line arguments
    */
    for ( i = 1; i < argc; i++)
    {
	if ( argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'd' :

		/*
		** Check no other option selected
		*/
		if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("disable_log", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_DISABLE_PRIMARY;
		}
		else
		if ( STcompare("disable_dual", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_DISABLE_DUAL;
		}
		else
		{
		    invalid_args = TRUE;
		}

		break;

	    case 'e' :

		/*
		** Check no other option selected
		*/
		if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("enable_log", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_ENABLE_PRIMARY;
		}
		else
		if ( STcompare("enable_dual", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_ENABLE_DUAL;
		}
		else
		{
		    invalid_args = TRUE;
		}

		break;

	    case 'f' :

		/*
		** Check no other option selected
		*/
		if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("force_init_log", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_FORCE_INIT_PRIMARY;
		}
		else
		if ( STcompare("force_init_dual", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_FORCE_INIT_DUAL;
		}
		else
		if ( STcompare("force_init", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_FORCE_INIT;
		}
		else
		{
		    invalid_args = TRUE;
		}

		break;

	    case 'i' :

		/*
		** Check no other option selected
		*/
		if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("init_log", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_INIT_PRIMARY;
		}
		else
		if ( STcompare("init_dual", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_INIT_DUAL;
		}
		else
		if ( STcompare("init", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_INIT;
		}
		else
		if ( STcompare("imm_shutdown", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_SHUTDOWN;
		    shutdown_action = 0;
		}
		else
		{
		    invalid_args = TRUE;
		}

		break;

	    case 'n' :
		/*
		** "-node=nodename" or "-node nodename" : 
		*/

		if ( STxcompare("node", 4, &argv[i][1], 0, 0, 0) == 0 )
		{
		    /*
		    ** Only allowed in a cluster environment
		    */
		    if (!cluster)
		    {
			TRdisplay(
	"The -node option is only allowed from an Ingres cluster member\n");
		        invalid_args = TRUE;
			break;
		    }
			
		    if ( '=' == argv[i][5] )
		    {
			pvalue = &argv[i][6];
		    }
		    else if ( ( '\0' == argv[i][5] ) && ( ++i < argc ) )
		    {
			/*
			** Next argument must be the nodename
			*/
			pvalue = &argv[i][0];
		    }
		    else
		    {
			/* 
			** Node argument not supplied so break
			*/
		        invalid_args = TRUE;
			break;
		    }

		    STmove( pvalue, EOS, MAX_NODE_SIZE, req_nodename);

		    /*
		    ** See if requested host node
		    */
		    if (STcompare( host_nodename, req_nodename ) != 0 )
		    {
			another_node = TRUE;
		    }

		    /*
		    ** If another node validate that it is a cluster node
		    */
		    if (another_node)
		    {
			if ( CXnode_number(req_nodename) <= 0 )
			{
			    TRdisplay(
				"Requested node %s is not a cluster member\n",
				req_nodename);
			    invalid_args = TRUE;
			    break;
			}
			log_nodename = req_nodename;
		    }
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'r' :
		/*
		** Check no other option selected
		*/
		if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( MEcmp(&argv[i][1], "rcp_continue_ignore_db=", 23) == 0)
		{
		    rcf_action = RCF_IGNORE_DB;

		    /* Convert the database name to lowercase */
		    CVlower(&argv[i][24]);

		    MEmove(STlength(&argv[i][24]), &argv[i][24], ' ', 
			sizeof(recov_struct->lg_dbname),
			&recov_struct->lg_dbname[0]);
		}
		else if ( MEcmp(&argv[i][1], "rcp_continue_ignore_table=", 26) == 0)
		{
		    rcf_action = RCF_IGNORE_TABLE;

		    /* FIX ME normalize table name */

		    /* Convert the table name to lowercase */
		    CVlower(&argv[i][27]);

		    MEmove(STlength(&argv[i][27]), &argv[i][27], ' ', 
			sizeof(recov_struct->lg_tabname),
			&recov_struct->lg_tabname[0]);
		}
		else if ( MEcmp(&argv[i][1], "rcp_continue_ignore_lsn=", 24) == 0)
		{
		    rcf_action = RCF_IGNORE_LSN;

		    status = parse_lsn(&argv[i][25], &recov_struct->lg_lsn);
		    if (status)
			invalid_args = TRUE;
		}
		else if ( MEcmp(&argv[i][1], "rcp_stop_lsn=", 13) == 0)
		{
		    rcf_action = RCF_STOP_LSN;

		    status = parse_lsn(&argv[i][14], &recov_struct->lg_lsn);
		    if (status)
			invalid_args = TRUE;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 's' :
		if ( STcompare("silent", &argv[i][1]) == 0 )
		{
		    silent = TRUE;
		}
		/*
		** Check no other option selected
		*/
		else if ( rcf_action != RCF_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}
		else if ( STcompare("shutdown", &argv[i][1]) == 0 )
		{
		    rcf_action = RCF_SHUTDOWN;
		    shutdown_action = 1;
		}
		else
		{
		    invalid_args = TRUE;
		}

		break;

	    default:
		invalid_args = TRUE;
		break;
	    }

	}
	else
	{
	    invalid_args = TRUE;
	}

	/*
	** If invalid args have been specfied than bail
	*/
	if ( invalid_args )
	    break;
		
    }

    if ( invalid_args )
	return( FAIL );
    else
	return( OK );
}

/*
** Name: rcpconf_action_on_another_node
**
** Description:
**	This routine will check the desired action on another node is OK.
**	It is only used in an Ingres cluster environment.
**
** Inputs:
**	None
**
** Outputs:
**	Returns OK is action is sensible, otherwise FAIL.
**
** Side Effects:
**
** History:
**	15-may-1993 (rmuth)
**	    Created.
**	21-jun-1993 (andys)
**	    Change to check if action is OK. Action really happens in
**	    the same place as mainline code.
**	25-Apr-2005 (mutma03)
**          Enable force_init options.
**
*/
static STATUS rcpconf_action_on_another_node( VOID )
{

   STATUS	status;

    /*
    ** Check if valid action
    */
    if (( rcf_action != RCF_INIT_PRIMARY )
		    &&
	( rcf_action != RCF_INIT_DUAL )
		   &&
	( rcf_action != RCF_INIT )
		   &&
	( rcf_action != RCF_FORCE_INIT_PRIMARY )
		   &&
	( rcf_action != RCF_FORCE_INIT_DUAL )
		   &&
	( rcf_action != RCF_FORCE_INIT) )
    {
	TRdisplay(
       "Only -init, -init_log, -init_dual, -force_init, -force_init_primary, or -force_init_dual will be allowed with the -node option.\n");
	return( FAIL );
    }

    /*
    ** Make sure that the cluster is OFFLINE
    */
    status = LGc_any_csp_online();
    if ( status == OK )
    {
	TRdisplay(
     "This operation is only allowed when the Ingres cluster is OFFLINE\n");
	return( FAIL );
    }


    return(OK);

}

/*
** Name: primary_currently_enabled
**
** Description:
**	Returns E_DB_OK if the primary log seems to be currently enabled.
**
** History:
**	21-feb-1994 (bryanp) B57873
**	    Created.
*/
static DB_STATUS
primary_currently_enabled(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	lgh_active_logs;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
/* FIXME the hardcoded 128 here matches lgdef.h, and is quicker/easier to
** fix here than all the other places.  Should make it MAX_LOC everywhere.
** (kschendel Aug 2009)
*/
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		num_logs;

    /*
    ** If system is online and active logs include primary, report E_DB_OK.
    ** If system is online and active logs DON'T include primary, E_DB_ERROR
    ** If system is offline, open primary and read log header. If header
    ** checksums correctly and lgh_active_logs & LGH_PRIMARY_LOG_ACTIVE is
    ** true, report E_DB_OK, else report E_DB_ERROR.
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (another_node == 0)
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log,
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_PRIMARY_ACTIVE)
		{
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	/*
	** Logging system is offline. Open primary log and read its header. 
	** If it checksums correctly and lgh_active_logs & 
	** LGH_PRIMARY_LOG_ACTIVE is true, report E_DB_OK, else report E_DB_ERROR.
	*/

	status = LG_build_file_name( LG_PRIMARY_LOG, log_nodename, file,
					    l_file, path, l_path, &num_logs);
	if (status)
	    break;

	if (!silent)
	    TRdisplay( "Opening primary Log file: %t in path: %t\n",
		        l_file[0], file[0], l_path[0], path[0] );

	status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	one_page = 1;
	status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	status = DIclose(&logfile, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
	{
	    lgh_active_logs = ((LG_HEADER *)buffer)->lgh_active_logs;
	    if (lgh_active_logs & LGH_PRIMARY_LOG_ACTIVE)
	    {
		status = E_DB_OK;
		break;
	    }
	    else
	    {
		status = E_DB_ERROR;
		break;
	    }
        }
	else
	{
	    status = E_DB_ERROR;
	    break;
	}

    } while (FALSE);

    if ( status == E_DB_OK )
	return( E_DB_OK );
    else
	return( E_DB_ERROR );
}

/*
** Name: dual_currently_enabled
**
** Description:
**	Returns E_DB_OK if the dual log seems to be currently enabled.
**
** History:
**	21-feb-1994 (bryanp) B57873
**	    Created.
*/
static DB_STATUS
dual_currently_enabled(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	lgh_active_logs;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
/* FIXME the hardcoded 128 here matches lgdef.h, and is quicker/easier to
** fix here than all the other places.  Should make it MAX_LOC everywhere.
** (kschendel Aug 2009)
*/
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		num_logs;

    /*
    ** If system is online and active logs include dual , report E_DB_OK.
    ** If system is online and active logs DON'T include dual, E_DB_ERROR
    ** If system is offline, open dual and read log header. If header
    ** checksums correctly and lgh_active_logs & LGH_DUAL_LOG_ACTIVE is
    ** true, report E_DB_OK, else report E_DB_ERROR.
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (another_node == 0)
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log,
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_DUAL_ACTIVE)
		{
		    status = E_DB_OK;
		    break;
		}
		else
		{
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	/*
	** Logging system is offline. Open dual log and read its header. If it
	** checksums correctly and lgh_active_logs & LGH_DUAL_LOG_ACTIVE is
	** true, report E_DB_OK, else report E_DB_ERROR.
	*/

	status = LG_build_file_name( LG_DUAL_LOG, log_nodename, file,
					    l_file, path, l_path, &num_logs);
	if (status)
	    break;

	if (!silent)
	    TRdisplay( "Opening dual Log file: %t in path: %t\n",
		        l_file[0], file[0], l_path[0], path[0] );

	status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	one_page = 1;
	status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	status = DIclose(&logfile, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	    break;
	}

	if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
	{
	    lgh_active_logs = ((LG_HEADER *)buffer)->lgh_active_logs;
	    if (lgh_active_logs & LGH_DUAL_LOG_ACTIVE)
	    { 
		status = E_DB_OK;
		break;
	    }
	    else
	    { 
		status = E_DB_ERROR;
		break;
	    }
	}
	else
	{
	    status = E_DB_ERROR;
	    break;
	}

    } while (FALSE);

    if ( status == E_DB_OK )
	return( E_DB_OK );
    else
	return( E_DB_ERROR );

}


static
void rcpconfig_usage(i4 msg_index)
{
    char	*radarg;

    if ( CXnuma_cluster_configured() )
	radarg = " -rad radid";
    else
	radarg = "";

    for (msg_index=0; rcpconfig_usage_msg[msg_index] != 0; 
	 msg_index++)
    {
	TRdisplay(rcpconfig_usage_msg[msg_index], radarg);
    }
}



/*{
** Name: parse_lsn		- Parse a command line lsn.
**
** Description:
**
**	Parse an lsn of the form: xx,yy
**
** Inputs:
**      cmd_line		command line where lsn exists
**
** Outputs:
**      lsn			pointer to the output lsn
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**
** Side Effects:
**	    none
**
** History:
**      28-jan-2008 (stial01)
**          Created from parse_lsn in dmfjsp.
*/
static DB_STATUS 
parse_lsn(
char		*cmd_line,
LG_LSN		*lsn)
{
    DB_STATUS	status;
    char      	*tmp_ptr;

    for (;;)
    {
	/*
	** CVal requires a terminating space or null.
	*/
	if (tmp_ptr = STchr(cmd_line, ','))
	    *tmp_ptr++ = EOS;
	else
	    break;

	status = CVal(cmd_line, (i4 *)&lsn->lsn_high);
	if (status != OK)
	    break;

	status = CVal(tmp_ptr, (i4 *)&lsn->lsn_low);
	if (status != OK)
	    break;

        return(E_DB_OK);
    }

    return(E_DB_ERROR);
}

/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gc.h>
#include    <gl.h>
#include    <st.h>
#include    <pc.h>
#include    <cs.h>
#include    <tr.h>
#include    <cm.h>
#include    <er.h>
#include    <si.h>
#include    <lo.h>
#include    <di.h>
#include    <nm.h>
#include    <me.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <lgkparms.h>
#include    <lgdstat.h>
#include    <pm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm0llctx.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dmve.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmfrcp.h>
#include    <dmccb.h>
#include    <csp.h>
#include    <dmfcsp.h>
#include    <dmftrace.h>
#include    <scf.h>
#include    <dmd.h>

/**
**
**  Name: DMFRCP.C - DMF Recovery Control Program.
**
**  Description:
**
**      This file contains the DMF recovery thread routines. The recovery thread
**	has the responsibility of bringing the database into a consistent
**	state following a server crash or machine crash. The recovery process 
**	performs on-line recovery while the system is still running. There is 
**	no need to explicitly request recovery when a failure has been 
**	diagnosed.
**
**	The recovery thread runs in a special server which has been started
**	with the -recovery startup parameter.
**
**	dmr_log_init	- Initialize the logging system.
**	dmr_check_event	- waiting/polling the current event of the logging
**			    system.
**
**	Internal routines:
**
**	rcp_recover	- Recover transactions for the run away servers.
**	rcp_shutdown	- Shutdown the recovrey process.
**	dmr_log_error	- Log the CL error message, convert the error
**			  code to the proper RCP error code.
**	dmr_error_translate	- Convert the DMF error to RCP error code.
**
**  History:
**      22-jul-1986 (Derek)
**          Created for Jupiter.
**	Sep-30-1986 (ac)
**	    written for the real version.
**	17-may-1988 (EdHsu)
**	    added coded for fast commit project.
**	31-oct-1988 (mmm)
**	    Deleted "SETUID" hint.  The ACP, RCP, and rcpconfig all must be
**	    run by the ingres user.  Making them setuid creates a security
**	    problem in the system.  A better long term solution would be to
**	    have these programs check whether the person running them
**	    is a super-user.  The install script on unix should be changed to
**	    make these programs mode 4700, so that only ingres can run them
**	    and if root happens to run them they will still act as if ingres
**	    started them.
**	05-jan-1988 (mmm)
**	    Added error messages that better explain what might be going wrong
**	    on UNIX if log_init() failed.  Also forced program to exit through
**	    PCexit() if log_init() failed, guaranteeing PCatexit() handlers a
**	    chance to run.
**	12-Jan-1989 (ac)
**	    added coded for 2PC support.
**	07-aug-1989 (fred)
**	    Added DYNLIBS MING hint for user defined datatypes.
**	25-sep-1989 (rogerk)
**	    Mark system status LGD_RCP_RECOVER while recovering transactions.
**	    This allows servers to check the system recovery status.
**	26-sep-1989 (cal)
**	    Updated MODE, PROGRAM, and DEST ming hints for user
**	    defined datatypes.
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**				TRdisplay of tran_id
**	29-jan-1990 (walt)
**	    Fix bug 9626 by removing statement in rcp_shutdown() that always
**	    set the log file's status to LGH_RECOVER and caused rcpconfig/init
**	    to not run.
**	12-feb-1990 (rogerk)
**	    Added parameter to dmr_db_known call for duplicate id fix.
**	23-apr-1990 (rogerk)
**	    Fixed fast commit problem in rcp_shutdown.
**	8-may-90 (bryanp)
**	    Check return codes of NMloc and TRset_file calls.
**	5-jun-1990 (bryanp)
**	    Explicitly clear the DCB portion of the DBCB when allocating it.
**	26-jul-1990 (bryanp)
**	    Added a third variant to rcp_shutdown to fix a class of inconsistent
**	    database bugs involving Archiver failures.
**      08-aug-1990 (jonb)
**          Add spaces in front of "main(" so mkming won't think this
**          is a main routine on UNIX machines where II_DMF_MERGE is
**          always defined (reintegration of 14-mar-1990 change).
**	24-aug-1990 (bryanp)
**	    Notify LG of the logfile BOF at the end of dmr_p1(), rather than
**	    waiting for the end of machine failure recovery. This allows LG to
**	    keep more accurate journal windows for journalled databases which
**	    were open at the time of the crash.
**      19-nov-1990 (bryanp)
**          Fix "filename too small" problem noted by Roger Taranto.
**          If dm0l_bcp/dm0l_ecp fails, log the error and take the installation
**          down. It would probably be nice to try another CP if the first one
**          failed, but there's no easy mechanism to do that.
**      18-dec-1990 (bonobo)
**          Added the PARTIAL_LD mode, and moved "main(argc, argv)" back to
**          the beginning of the line.
**      10-jan-1991 (stevet)
**          Added CMset_attr to initialize CM attribute table.
**       4-feb-1991 (rogerk)
**          Declared exception handler at RCP startup so we won't silently
**          exit on access violations.  Added routine ex_handler.
**      13-jun-1991 (bryanp)
**          Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	20-aug-1991 (jnash) merged 10-Oct-1989 (ac)
**	    added dual logging support.
**	20-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	20-aug-1991 (jnash) merged 14-jun-1991 (Derek)
**	    Minor changes for II_DMF_TRACE support.
**      25-sep-1991 (jnash) merged 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**      25-sep-1991 (jnash) merged 16-aug-1991 (rog)
**          Set buffer to be EX_MAX_SYS_REP+15 instead of 256.
**      25-sep-1991 (jnash) merged 19-aug-1991 (rogerk)
**          Set process type in the svcb.  This was done as part of the
**          bugfix for B39017.  This allows low-level DMF code to determine
**          the context in which it is running (Server, RCP, ACP, utility).
**	17-oct-1991 (rogerk)
**	    Fixed coding bug in rcp_shutdown where we were attempting to set
**	    clean_shutdown to FALSE following a bad dm0l_logforce return by
**	    using "clean_shutdown == FALSE".
**      24-oct-1991 (jnash)
**          In rcp_recover, initialize add_status in dm2d_add_db call in 
**	    which it was not previously initialized (caught by LINT).
**	25-oct-1991 (bryanp)
**	    Re-enable dmr_log_statistics call when recovery activity occurs.
**	30-oct-1991 (bryanp)
**	    The dual-logging state is now maintained in the logging system
**	    rather than being recorded in rcpconfig.dat. This simplifies a
**	    number of RCP activities. Basically, the RCP now need only request
**	    dual logging at startup time; the logging system maintains the
**	    dual logging state from that time forward, relying on the RCP only
**	    to perform an LGforce() of the LG_HEADER when dual logging is
**	    disabled.
**	    Removed the rcp_disable_dual_logging() routine as a result.
**	    Added LG_E_EMER_SHUTDOWN for automated testing use.
**	10-apr-1992 (bryanp)
**	    Fix bad arguments to TRdisplay in rcp_commit().
**	10-apr-1992 (rogerk)
**	    Fix call to ERinit().  Added missing args: p_sem_func, v_sem_func.
**	    Even though we do not pass in semaphore routines, we still need
**	    to pass the correct number of arguments.
**	14-apr-1992 (rogerk)
**	    Fixed ule_format call in log_init following LGshow(LG_A_NODEID)
**	    to pass err_code parameter by reference.
**	18-jun-1992 (jnash)
**	    Append to end of RCP log via TR_F_APPEND TRset_file() flag (UNIX
**	    only).
**	18-jun-1992 (jnash)
**	    Change name of RCP log file from II_RCP.LOG to iircp.log.
**	18-jun-1992 (jnash)
**	    If II_DMFRCP_STOP_ON_INCONS_DB set, crash rather than
**	    marking a database inconsistent.
**	7-july-1992 (jnash)
**	    Add DMF prototyping.
**	25-aug_1992 (ananth)
**	    Get rid of HP compiler warnings.
**	22-sep-1992 (bryanp)
**	    We no longer have a DMFRCP process. Instead, we have a distinguished
**	    server called the Recovery Server, which has a distinguished thread
**	    called the Recovery Thread. This means:
**	    a) No more "main"
**	    b) some entry points are called by DML code.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project:
**	      - Changed dmr_recover flags: The RCP and CSP now pass different
**	        startup flag values.
**	      - Changed the altering of the Log File header status before
**	        starting recovery processing.  Prevoiusly we would alter the
**	        log file status to LGH_RECOVER before starting recovery and
**		then change it to LGH_VALID after recovery was complete.  If
**		a crash occurred before the system was brough up, no log scan
**		would be needed to find the CP and EOF (due to the LGH_RECOVER
**		state), but any log records written during UNDO would be lost.
**		Since these UNDO log records are required, the log header is
**		now set to VALID prior to recovery processing rather than after.
**		This means that a crash during recovery will necessitate some
**		extra work during restart to find the EOF.
**	18-jan-1993 (bryanp)
**	    Ensure that dmr_check_event doesn't just fall off the end.
**	    Add parentheses to if statement which calls LGopen.
**	    Use PMget() calls to retrieve logging/locking configuration data.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed ECP to store the LSN of the BCP log record rather than
**		its log address.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**      26-apr-1993 (andys/keving/bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Added forward declaration for release_orphan_locks().
**		At startup, always write two consistency points. This removes
**		    some gross code which used to try to do this ONLY if it
**		    thought the logfile was truly freshly initialized. Instead
**		    of trying to figure that out, we just write two consistency
**		    points unconditionally.
**		Remove static from definition of dmr_log_error which is called
**		    from dmfcsp.
**		Use get_lgk_config from lgk to initialise recovery params
**		Change name of get_lgk_config to lgk_get_config.
**		Include lgkparms.h
**	26-apr-1993 (jnash)
**          Include non-iidbdb open/close msgs in iircp.log.  Also fix
**	    a few compiler warnings.
**	24-may-1993 (bryanp)
**	    Resource blocks are now configured separately from lock blocks.
**	    Removed unused definitions of logging system constants
**		(D_MIN_BLK_SIZE, D_MAX_BLK_SIZE)
**	24-may-1993 (jnash)
**          Fix change of 26-apr to use 32 rather than 24 char iidbdb name.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	26-jul-1993 (jnash)
**	    Support locking the LGK segment via PM.
**	26-jul-1993 (rog)
**          Remove ex_handler() because the one in this file is not used anymore
**	26-jul-1993 (rogerk)
**	    Fixed dmr_complete_recovery call to pass num_nodes instead of
**	    node_id.  Passing node_id caused us to not logdump when database
**	    inconsistencies are found.
**	20-sep-1993 (jnash)
**	    Make cp_p a float, allowing CPs to be a fractional percentage of
**	    the log file (for very large files).
**	18-oct-1993 (jnash)
**	    Fix TRdisplay() text that notes II_CRASH_ON_INCONS_DB enabled.
**	18-oct-1993 (rogerk)
**	    Removed obsolete LG_RCP flag to LGend calls.
**	    Changed rcp_commit to assume ownership of the committing xact and
**	    to write the ET record on its behalf rather than using the context
**	    of the rcp's global transaction.
**	31-jan-1994 (bryanp) B57888
**	    Add err_code argument to rcp_recover; fill it in upon error.
**      15-may-1994 (rmuth)
**          b59248, a server failure in between a LGend and LKrelease on a
**                  READONLY transaction would leave the orphaned lock list
**                  around. This caused by the following..
**          a) The incorrect pid was stored in the rlp, the now defunct
**             "pr_ipid" value was used instead of "pr_pid". Checking for
**             orphan lock lists for this process used this invalid value and
**             hence found nothing.
**          b) The code assumed that there would only ever be one orphaned
**             lock list. The code now continues until LK indicates that
**             there are no more lists.
**      06-Dec-1994 (liibi01)
**          Cross integration from 6.4 (Bug 65068). Fix problem of 'rcpconfig
**          -shutdown' short_circuiting recovery in dmr_recover().
**      20-may-95 (hanch04)
**          Added include dmtcb.h
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	16-sep-1996 (canor01)
**	    Fixed typo in message in rcp_commit().
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmfdata.c.
**	12-Nov-1996 (jenjo02)
**	    Updated TRdisplay() of log events to match new values.
**      27-feb-1997 (stial01)
**          release_orphan_locks() Release ORPHAN XSVDB lock lists
**	28-Mar-1997 (wonst02)
**	    Use II_ACCESS for the type of access: read-write or readonly.
** 	23-may-1997 (wonst02)
** 	    Remove changes for II_ACCESS.
**	28-Oct-1997 (jenjo02)
**	    Added LK_NOLOG flag to LKrelease() call in count_opens to keep
**	    E_CL103B from appearing in the log if we justifiably don't hold
**	    the database lock we're trying to release.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	21-oct-1998 (somsa01)
**	    Removed defines of variables which are not used anymore.
**	08-jun-1999 (somsa01)
**	    In rcp_shutdown(), added printout of server shutdown message to
**	    the errlog.log . Also, properly set up the hostname in
**	    ule_initiate().
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-jun-2001 (devjo01)
**	    Cross of c443261, plus s103715 changes.  (Do not suppress
**	    LG/LK sizing if running within a CSP.  Call dmfcsp_initialize
**	    before operating on logs in RCP context.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**	3-Dec-2003 (schka24)
**	    Use cp_interval_mb rather than old cp_interval.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**	15-Mar-2006 (jenjo02)
**	    Replaced all instances of dmr_log_statistics() with
**	    dmd_log_info().
**	01-Jun-2007 (hweho01)
**	    Added call for GCA_TERMINATE in rcp_shutdown(), avoid 
**          E_GC0155_GCN_FAILURE message in errlog.log.
**      21-Jun-2007 (hweho01)
**          Need to move GCA_TERMINATE call to the main thread of 
**          RCP server, so it will not get error due to context  
**          differences.
**	07-jan-2008 (joea)
**	    Discontinue use of CXnickname_from_node_name.
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**	07-Feb-2008 (hanje04)
**	    SIR 119978
**	    Defining true and false locally causes compiler errors where 
**	    they are already defined. Change the variable names to tru and
**	    fals to avoid this
**	03-jun-2008 (joea)
**	    Show the correct status of dual logging in dmr_log_init.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	    Modified dmr_error_translate to report whence called.
**      11-dec-2008 (stial01)
**          Use define for iidbdb name
**      05-may-2009
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**      15-jun-2009 (stial01)
**          Changes for Redo/undo error handling
**/

/*
**  Defines of the global varibles for logging system.
*/
GLOBALREF   LG_LXID		rcp_lx_id;
GLOBALREF   LG_LGID		rcp_lg_id;
GLOBALREF   LG_DBID		rcp_db_id;
GLOBALREF   DB_TRAN_ID		rcp_tran_id;
GLOBALREF   i4		rcp_node_id;
GLOBALREF   LG_LA		bcp_la;
GLOBALREF   LG_LSN		bcp_lsn;
GLOBALREF   char	Db_stop_on_incons_db;
GLOBALREF   i4		Db_startup_error_action;
GLOBALREF   i4		Db_pass_abort_error_action;
GLOBALREF   i4		Db_recovery_error_action;

/*
** External Variables
*/

 
/*
**  Definition of static variables and forward static functions.
*/
static	DB_STATUS   rcp_recover(DB_ERROR *dberr);

static	VOID	    rcp_shutdown(DB_ERROR *dberr);

static	DB_STATUS   count_opens(i4 open_database);

static  VOID        rcp_commit(VOID);    /* Manually commit a transaction. */
static	VOID	    rcp_get_error_action(VOID);

static	DB_STATUS   dmrErrorTranslateFcn(
				i4		new_error,
				DB_ERROR	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define dmr_error_translate(new_error,dberr) \
	dmrErrorTranslateFcn(new_error,dberr,__FILE__,__LINE__)

static DB_STATUS release_orphan_locks(
				RCP	    *rcp);

static  bool        recovery_needed = FALSE; /* Set by dmr_recover() */
static  bool        recovery_done = FALSE; /* TRUE after dmr_recover() */

#define RCP_LOGFILE_NAME "iircp.log"
#define RCP_FILENAME_LEN ((sizeof(RCP_LOGFILE_NAME))-1)
#define IGNORE_CASE 1

static	RCONF	rconf;

/*{
** Name: dmr_rcp_init	- Initialize the logging system.
**
** Description:
**      This routine initializes the logging system and the locking system.
**
** Inputs:
** Outputs:
**	recovery_log_id	    Set to the RCP's logging system ID.
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
**	18-jun-1992 (jnash)
**	    Append to end of RCP log via TR_F_APPEND TRset_file() flag.
**	18-jun-1992 (jnash)
**	    Change name of RCP log file from II_RCP.LOG to iircp.log.
**	18-jun-1992 (jnash)
**	    If II_DMFRCP_STOP_ON_INCONS_DB set, crash rather than
**	    marking a database inconsistent.
**	16-jun-1992 (bryanp)
**	    Created out of the old "log_init".
**	18-jan-1993 (bryanp)
**	    Add parentheses to if statement which calls LGopen.
**	24-may-1993 (bryanp)
**	    Resource blocks are now configured separately from lock blocks.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	26-jul-1993 (jnash)
**	    Lock the LGK data segment if requested via LGinit_lock().
**	18-oct-1993 (jnash)
**	    Fix TRdisplay() text that notes II_CRASH_ON_INCONS_DB enabled.
**	23-may-1994 (andys)
**	    Change reference to rcpconfig to refer to cbf instead. [bug 55682]
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	26-jun-2001 (devjo01)
**	    Cross of c443261, plus s103715 changes.  (Do not suppress
**	    LG/LK sizing if running within a CSP.  Call dmfcsp_initialize
**	    before operating on logs in RCP context.
**	21-Mar-2006 (jenjo02)
**	    Add LGalter(LG_A_OPTIMWRITE) for optimized log writes.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add LGalter(LG_A_RBBLOCKS), LGalter(LG_A_RFBLOCKS)
**	    for buffered log reads.
*/
DB_STATUS
dmr_rcp_init(LG_LGID	*recovery_log_id)
{
    i4			err_code;
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    LG_HEADER		header;
    i4			length;
    i4		bcnt;
    DM0L_ADDDB		add_info;
    i4			len_add_info;
    i4		l_show;
    DB_OWN_NAME		user_name;
    LOCATION		loc;
    char                *qualname;
    i4                  l_qualname;
    char                node_name[CX_MAX_NODE_NAME_LEN+1];
    char		filename[DI_FILENAME_MAX+1];
    i4			local_log = 0;
    i4			clustered;

    /*	Prepare the RCP log file and the terminal log file. */

    /* Create filename. Non cluster is iircp.log, Cluster
    ** is iircp.log_NODE, where NODE is CX_MAX_NODE_NAME_LEN+1
    ** in length and we add 2
    ** more characters for the "_" and '\0'.
    */

    MEmove(RCP_FILENAME_LEN, RCP_LOGFILE_NAME, ' ', sizeof(filename), filename);
    filename[RCP_FILENAME_LEN] = 0;

    if (clustered = CXcluster_enabled())
    {
	CXnode_name(node_name);
	CXdecorate_file_name(filename, node_name);
    }    
    else
	GChostname(node_name, sizeof(node_name));

    ule_initiate(node_name, STlength(node_name), "II_RCP", 6);


    if( (status = NMloc(UTILITY, FILENAME, filename, &loc)) != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L, 
	    (i4 *)NULL, &err_code, 0);
    }
    else
    {
        LOtos(&loc, &qualname);
        l_qualname = STlength(qualname);
    }

    if (status == OK)
    {
	i4	flag; 

	/*
	** RCP appends to the end of existing log file on UNIX, allows 
	** VMS versioning to do the equivalent thing.
	*/
#ifdef VMS
	flag = TR_F_OPEN;
#else
	flag = TR_F_APPEND;
#endif

	status = TRset_file(flag, qualname, l_qualname, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )0, 0L, (i4 *)NULL, &err_code, 0);
	}
    }

    if (status != OK)
    {
	/*
	** We were unable to open iircp.log, either because the NM symbols
	** have not been correctly set, or because the TR facility isn't
	** working. Since something is SERIOUSLY wrong, we should not go on.
	** Dying with only a message in the error log is not very informative,
	** and may be even worse if ule_format was unable to issue any message
	** (because, e.g., the errlog.log file could not be written to). Make
	** a last ditch effort to display some sort of useful message to
	** the terminal, if possible, before exiting.
	*/
        if ((status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
				&sys_err)) == OK)
	{
	    TRdisplay("Error building path/filename for iircp.log\n");
	    TRdisplay("Please check installation and environment, and\n");
	    TRdisplay("examine the ERRLOG file for additional messages\n");
	}
	PCexit( FAIL );
    }

    /*	Log a startup message. */

    TRdisplay("%@ RCP STARTUP.\n");

    /*
    ** Check if we should do something other than marking a 
    ** database inconsistent if an error occurs
    */
    rcp_get_error_action();


    /* Use Db_startup_error_action DURING startup */
    Db_recovery_error_action = Db_startup_error_action;
    TRdisplay("Current RCP error action %30w\n",
	RCP_ERROR_ACTION, Db_recovery_error_action);

    for (;;)
    {
	status = lgk_get_config(&rconf, &sys_err, TRUE);

	if (status != OK)
	{
	    (VOID) uleFormat( NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
				NULL, 0, NULL, &err_code, 0 );
	    break;
	}

	/* 
	** Initialize the Logging system. 
	** Lock the LGK data segment if requested to do so.
	*/
	if (rconf.lgk_mem_lock)
	    status = LGinit_lock(&sys_err, ERx("recovery thread"));
	else 
	    status = LGinitialize(&sys_err, ERx("recovery thread"));

	if (status)
	{
	    (VOID) uleFormat(NULL, E_DM9011_BAD_LOG_INIT, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	    uleFormat(NULL, E_DM9401_RCP_LOGINIT, 0, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    break;
	}

	/*  Initialize the Locking system. */

	if (status = LKinitialize(&sys_err, ERx("recovery thread")))
	{
	    (VOID) uleFormat(NULL, E_DM901E_BAD_LOCK_INIT, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	    uleFormat(NULL, E_DM9401_RCP_LOGINIT, 0, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    break;
	}

	/* Get the cluster id. */

	status = LGshow(LG_A_NODEID, (PTR)&rcp_node_id, sizeof(rcp_node_id), 
			&l_show, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL,
		    &err_code, 0);
	    TRdisplay("Can't get the cluster id. \n");
	    TRdisplay("%@ RCP SHUTDOWN.\n");
	    return (E_DB_ERROR);
	}

	if (status = LGalter(LG_A_BLKS, (PTR)&rconf.blks,
				sizeof(rconf.blks), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_BLKS, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LG_A_BLKS with the value %d\n", rconf.blks);

	if (status = LGalter(LG_A_LDBS, (PTR)&rconf.ldbs,
				sizeof(rconf.ldbs), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_LDBS, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LG_A_LDBS with the value %d\n", rconf.ldbs);


	/* LK stuff: */

	if (status = LKalter(LK_I_LKH, rconf.lkh, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err, LK_I_LKH, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_LKH with the value %d\n", rconf.lkh);

	if (status = LKalter(LK_I_RSH, rconf.rsh, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err, LK_I_RSH, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_RSH with the value %d\n", rconf.rsh);

	if (status = LKalter(LK_I_BLK, rconf.lkblk, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err, LK_I_BLK, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_BLK with the value %d\n", rconf.lkblk);

	if (status = LKalter(LK_I_RSB, rconf.resources, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err,
			LK_I_RSB, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_RSB with the value %d\n", rconf.resources);

	if (status = LKalter(LK_I_LLB, rconf.lkllb, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err, LK_I_LLB, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_LLB with the value %d\n", rconf.lkllb);

	if (status = LKalter(LK_I_MAX_LKB, rconf.maxlkb, &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9019_BAD_LOCK_ALTER, &sys_err, 
		    LK_I_MAX_LKB, 
		    E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LK_I_MAXLKB with the value %d\n", rconf.maxlkb);

	/* Decide which log file to open. If the user has requested dual
	** logging, tell the logging system of this fact before trying to
	** open the log file(s).
	*/
	if (rconf.dual_logging)
	{
	    if (status = LGalter(LG_A_ENABLE_DUAL_LOGGING, (PTR)&local_log, 
			sizeof(local_log), &sys_err))
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, 
				    LG_A_ENABLE_DUAL_LOGGING, 
				    E_DM9401_RCP_LOGINIT);
		break;
	    }    
	}

	/*  Open as master. */

	if ((status = LGopen(LG_MASTER, 0, 0, &rcp_lg_id, &bcnt,
			    rconf.bsize, &sys_err)) != OK)
	{
	    _VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	    _VOID_ uleFormat(NULL, E_DM9012_BAD_LOG_OPEN, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	    _VOID_ uleFormat(NULL, E_DM9401_RCP_LOGINIT, 0, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
	    break;
	} 
	/*
	** Return logging system ID to caller so that it can be stored in the
	** SVCB.
	*/
	*recovery_log_id = rcp_lg_id;

	/* Add. */

	STmove((PTR)DB_RECOVERY_INFO, ' ', DB_MAXNAME, (PTR) &add_info.ad_dbname);
	MEcopy((PTR)DB_INGRES_NAME, DB_MAXNAME, (PTR) &add_info.ad_dbowner);
	MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
	add_info.ad_dbid = 0;
	add_info.ad_l_root = 4;
	len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;
	if (status = LGadd(rcp_lg_id, LG_NOTDB, (char *)&add_info, len_add_info,
	    &rcp_db_id, &sys_err))
	{
	    (VOID) uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 4, 0, rcp_lg_id,
	    0, "$recovery",
	    0, "$ingres",
	    0, "None");

	    uleFormat(NULL, E_DM9401_RCP_LOGINIT, 0, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    break;
	}

	/*
	** Get the log header so that we can check the status.
	*/

	if (LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length, 
		    &sys_err))
	{
	    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_HEADER, 
				E_DM9401_RCP_LOGINIT);
	    break;
	}

	if (header.lgh_status == LGH_BAD)
	{
	    TRdisplay("\nLog file is bad.\n");
	    TRdisplay("\nRecreate a new log file and initialize the log file using cbf.\n");
	    return(E_DB_ERROR);
	}

	/*
	** Begin a transaction for the recovery process.
	** Note : LGbegin changes the state of the log file header in
	** memory. Don't want to call it until LGshow() has been 
	** called.
	*/
	STmove((PTR)DB_RECOVERY_INFO, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
	if (LGbegin(LG_NOPROTECT, rcp_db_id, &rcp_tran_id, 
		    &rcp_lx_id, sizeof(DB_OWN_NAME),
		    &user_name.db_own_name[0], 
		    (DB_DIS_TRAN_ID*)NULL, &sys_err))
	{
	    status = dmr_log_error(E_DM900C_BAD_LOG_BEGIN, &sys_err, rcp_db_id, 
				E_DM9401_RCP_LOGINIT);
	    break;
	}

	/* 
	** Since LGbegin change the state of the log file header in memory. 
	** Get the memory copy of header again.
	*/

	if (LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length, 
		    &sys_err))
	{
	    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_HEADER, 
				E_DM9401_RCP_LOGINIT);
	    break;
	}

	/* Log header is OK, allocate the log buffers. */

	if (LGalter(LG_A_BCNT, (PTR)&rconf.bcnt, 
			sizeof(rconf.bcnt), &sys_err))
	{
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_BCNT, 
			E_DM9401_RCP_LOGINIT);
	    break;
	} 

	/* Turn optimized writes on or off, as configured */
	if ( LGalter(LG_A_OPTIMWRITE, (PTR)NULL,
			(rconf.optimwrites) ? 1 : 0,
			&sys_err) )
	{
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_OPTIMWRITE, 
			E_DM9401_RCP_LOGINIT);
	    break;
	} 

	/*
	** RBBLOCKS/RFBLOCKS are constrained by the number of blocks in the log file,
	** not known until the log's been opened.
	*/
	if (status = LGalter(LG_A_RBBLOCKS, (PTR)&rconf.rbblocks, 
				sizeof(rconf.rbblocks), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_RBBLOCKS, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LG_A_RBBLOCKS with the value %d\n", rconf.rbblocks);

	if (status = LGalter(LG_A_RFBLOCKS, (PTR)&rconf.rfblocks, 
				sizeof(rconf.rfblocks), &sys_err))
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_RFBLOCKS, 
			E_DM9401_RCP_LOGINIT);
	    break;
	}
	TRdisplay("Alter LG_A_RFBLOCKS with the value %d\n", rconf.rfblocks);

	return(E_DB_OK);
    }

    return(status);
}

/*{
** Name: dmr_log_init	- Initialize the logging system.
**
** Description:
**      This routine initialize the logging system. If not the first time
**	open the log file, check the consistency of the log file header,
**	and perform any necessary recovery work. If the log file header is
**	bad, reconstruct the state of the log file header.
**
** Inputs:
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
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	6-Sept-1988 (Edhsu)
**	    Pass rconf.ldbs to dmf_init routine
**	8-may-90 (bryanp)
**	    Give a more verbose, but more informational, message when
**	    rcpconfig.dat cannot be found or cannot be read. Tell the user
**	    what is wrong, if possible, and how to correct it.
**	24-aug-1990 (bryanp)
**	    Notify LG of the logfile BOF at the end of dmr_p1(), rather than
**	    waiting for the end of machine failure recovery. This allows LG to
**	    keep more accurate journal windows for journalled databases which
**	    were open at the time of the crash.
**	21-aug-1991 (jnash) merged 10-Oct-1989 (ac)
**	    added dual logging support.
**      25-sep-1991 (jnash) merged 19-aug-1991 (rogerk)
**          Set process type in the svcb.  This was done as part of the
**          bugfix for B39017.  This allows low-level DMF code to determine
**          the context in which it is running (Server, RCP, ACP, utility).
**	30-oct-1991 (bryanp)
**	    Updated dual logging support: the rcp no longer tries to keep track
**	    of the dual logging state in the rcpconfig.dat file.
**	16-jun-1992 (bryanp)
**	    Renamed to "dmr_log_init" and changed so that it can be called by
**	    dmc_start_server().
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project:
**	      - Changed dmr_recover flags: The RCP and CSP now pass different
**	        flag values.
**	      - Changed the altering of the Log File header status before
**	        starting recovery processing.  Prevoiusly we would alter the
**	        log file status to LGH_RECOVER before starting recovery and
**		then change it to LGH_VALID after recovery was complete.  If
**		a crash occurred before the system was brough up, no log scan
**		would be needed to find the CP and EOF (due to the LGH_RECOVER
**		state), but any log records written during UNDO would be lost.
**		Since these UNDO log records are required, the log header is
**		now set to VALID prior to recovery processing rather than after.
**		This means that a crash during recovery will necessitate some
**		extra work during restart to find the EOF.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Don't pass header to dmr_recover; it will show the header
**		    itself. dmr_recover also knows how to scan beck to the
**		    last consistency point.
**		Removed the test for a completely empty log file. Now every
**		    run of RCP starts by writing *two* consistency points and
**		    forcing the log.
**	20-sep-1993 (jnash)
**	    Make cp_p a float, allowing fractional percentages for very
**	    large log files.
**	18-oct-1993 (rogerk)
**	    Removed obsolete LG_RCP flag to LGend calls.
**      06-Dec-1994 (liibi01)
**          Cross integration from 6.4(Bug 65068). Force imm_shutdown if
**          dmr_recover() call interrupted by rcpconfig -shutdown.
**	3-Dec-2003 (schka24)
**	    use cp_mb, cp_p is now deprecated.  lgkparms computes cp_mb from
**	    cp_p for anyone not yet using the new parameter.
**	23-Mar-2004 (jenjo02)
**	    Fix TRdisplay of BOF from misleading and nearly always
**	    wrong "machine failure" to either "after recovery" 
**	    or "from log header".
**	04-May-2004 (jenjo02)
**	    Distinguish "recovery_done" from "recovery_needed".
**	31-Mar-2005 (jenjo02)
**	    On a cluster, invoke csp_initialize to perform a
**	    choreographed cluster-wide recovery, which must include
**	    this node's local log.
**	01-Apr-2005 (jenjo02)
**	    Move csp_initialize error reporting to that function
**	    for non-cluster transparency.
*/
DB_STATUS
dmr_log_init(void)
{
    DB_STATUS		err_code;
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    LG_HEADER		header;
    STATUS		cl_status;
    i4			length;
    i4		on = 1;
    LG_HDR_ALTER_SHOW	alter_parms;
    i4			active_log;
    DB_ERROR		dberr;

    CLRDBERR(&dberr);

    for (;;)
    {
        /*
        ** Set process type in the SVCB.
        */
        dmf_svcb->svcb_status |= SVCB_RCP;

	if ( CXcluster_enabled() )
	    status = csp_initialize(&recovery_needed);
	else
	    status = dmr_recover( RCP_R_STARTUP, &recovery_needed );

	if (status != E_DB_OK)
	    break;

        /*
        ** bug 65068
        ** If the rcp was restarted after machine failure and recovery
        ** was required via dmr_recover() above, but was then shutdown
        ** via rcpconfig -shutdown before recovery completed, then
        ** we would write a BCP in rcp_shutdown() causing the next
        ** invocation of the rcp to believe no recovery was required.
        ** Avoid this by using boolean recovery_done to flag to mark
        ** the fact that dmr_recover() hasn't yet completed.
        ** Note that rcpconfig -imm_shutdown didn't cause this problem.
        */
	recovery_done = TRUE;
        

	/* Show the header. */

	if (cl_status = LGshow(LG_A_HEADER,
				(PTR)&header, sizeof(header),
				&length, &sys_err))
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    return(dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_HEADER, 
			    E_DM9407_RCP_RECOVER));
	}

	TRdisplay("%@ RCP: BOF %s: <%d,%d,%d>\n",
		    (recovery_needed) ? "after recovery" : "from log header",
		    header.lgh_begin.la_sequence,
		    header.lgh_begin.la_block,
		    header.lgh_begin.la_offset);

	/* Get the configuration data of various points. */

	header.lgh_l_logfull = (header.lgh_count * rconf.lf_p) / 100;
	header.lgh_l_abort = (header.lgh_count * rconf.abort_p) / 100;
	header.lgh_l_cp = (rconf.cp_mb * 1024 * 1024) / header.lgh_size;
	/* Double check the CP count as the validity of cp_interval_mb
	** can't be guaranteed (as lgkparm doesn't know the true lgh_count).
	** Enforce min of 3 CP's per round trip of the log.
	*/
	if (header.lgh_l_cp > (header.lgh_count/3))
	{
	    TRdisplay("Reduce CP interval from %d blocks to %d\n",
				header.lgh_l_cp, header.lgh_count/3);
	    header.lgh_l_cp = header.lgh_count / 3;
	}
	header.lgh_cpcnt = rconf.cpcnt;
	TRdisplay("Alter the log-full-limit in percentage to %d\n", rconf.lf_p);
	TRdisplay("Alter the consistency point interval block count to %d\n",
	     header.lgh_l_cp);
	TRdisplay("Alter the force-abort-limit in percentage to %d\n", rconf.abort_p);
	TRdisplay("Alter the maximum consistency point interval for invoking archiver to %d\n", rconf.cpcnt);

	/* Log header is valid now, make some changes to the header. */

	/* Change the memory copy of the header. */

	alter_parms.lg_hdr_lg_header = header;
	alter_parms.lg_hdr_lg_id = rcp_lg_id;
	if (cl_status = LGalter(LG_A_NODELOG_HEADER,
				(PTR)&alter_parms, sizeof(alter_parms),
				&sys_err))
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_HEADER, 
				E_DM9401_RCP_LOGINIT);
	    break;
	}

	/* 
	** Write an empty CP record to the log file and update the log header
	** from memory to disk. Actually, write two empty CP record sets.
	**
	** When the RCP starts up, it now writes two consistency points to the
	** log file, instead of just writing one consistency point. Prior to
	** this, the RCP attempted to detect when a log file had just been
	** freshly initialized, and only in that case did it try to write two
	** consistency points. Since writing an extra CP at installation
	** startup time doesn't hurt, and since the old code that tried to
	** deduce when a second CP was needed was buggy and full of holes, it's
	** simplest to just always write two CP's.
	**
	** (For the truly curious, the reason that we need to write two CPs at
	** startup has to do with the fact that the address of a consistency
	** point is actually the address of the record just before the
	** consistency point, and in order to successfully read the log file
	** backwards to reach a consistency point, we must ensure that there is
	** a legal record before the start of the last consistency point. In an
	** empty log file, there is no record before the first consistency
	** point, so we can't allow the first consistency point to also be the
	** last consistency point.  Therefore, we write two consistency
	** points.)
	*/

	if (dm0l_bcp(rcp_lx_id, &bcp_la, &bcp_lsn, &dberr))
	{
	    status = dmr_error_translate(E_DM9401_RCP_LOGINIT, &dberr);
	    break;
	}

	if (dm0l_ecp(rcp_lx_id, &bcp_la, &bcp_lsn, &dberr))
	{
	    status = dmr_error_translate(E_DM9401_RCP_LOGINIT, &dberr);
	    break;
	}

     	if (dm0l_bcp(rcp_lx_id, &bcp_la, &bcp_lsn, &dberr))
     	{
	    status = dmr_error_translate(E_DM9401_RCP_LOGINIT, &dberr);
     	    break;
     	}
     	if (dm0l_ecp(rcp_lx_id, &bcp_la, &bcp_lsn, &dberr))
     	{
	    status = dmr_error_translate(E_DM9401_RCP_LOGINIT, &dberr);
     	    break;
     	}
     	cl_status = LGforce(LG_HDR, rcp_lx_id, 0, 0, &sys_err);
     	if (cl_status != OK)
     	{
     	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
     		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
     	    _VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL,
     		(char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
     		0, rcp_lx_id);
     	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err,
     			    LG_A_HEADER, 
     			    E_DM9401_RCP_LOGINIT);
     	    break;
     	}
     	/* 
     	** Notify logging system of successfully handling machine failure
     	** recovery. 
     	*/
     
     	if (LGend(rcp_lx_id, LG_START, &sys_err))
     	{
     	    status = dmr_log_error(E_DM900E_BAD_LOG_END, &sys_err, 
     				rcp_lx_id, E_DM9401_RCP_LOGINIT);
     	}

	/* Turn logging system online. */

	if (cl_status = LGalter(LG_A_ONLINE, (PTR)&on, sizeof(on), &sys_err))
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_ONLINE, 
				E_DM9401_RCP_LOGINIT);
	    break;
	} 

	/* Log an online message. */

	status = LGshow(LG_S_DUAL_LOGGING, (PTR)&active_log, sizeof(active_log),
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG , NULL, NULL, 0L, NULL,
		       &err_code, 0);
	    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err,
				   LG_S_DUAL_LOGGING, E_DM9401_RCP_LOGINIT);
	    break;
	}

	TRdisplay("\n%@ RCP Dual logging is %w.\n", "NOT enabled,ENABLED",
		  (active_log & LG_DUAL_ACTIVE) ? 1 : 0);
	TRdisplay("%@ RCP LOG IS ONLINE.\n");

	/* Use Db_pass_abort_error_action when ONLINE */
	Db_recovery_error_action = Db_pass_abort_error_action;
	TRdisplay("Current RCP error action %30w\n",
	    RCP_ERROR_ACTION, Db_recovery_error_action);

	dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
		     DMLG_DATABASES | DMLG_TRANSACTIONS);

	return(E_DB_OK);
    }

    return(status);
}

/*{
** Name: rcp_recover - Recover transactions for the run away servers.
**
** Description:
**      This routine recovers transactions for the run away servers.
**
** Inputs:
**	None.
**
** Outputs:
**	err_code				reason for error return.
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
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	10-jun-1988 (edhsu)
**	    Added code for fast commit.
**	30-Aug-1988 (Edhsu)
**	    Add number of redo db to dmr_gen_db to avoid unnecessary redo
**	13-dec-1988 (rogerk)
**	    Added rtx_recover_type field to the RTX struct.  Whenever an rtx
**	    is allocated, its type is set to RTX_UNDO.  If it is moved off of
**	    the abort list and onto the redo list, its type is changed to
**	    RTX_REDO.
**	12-Jan-1989 (ac)
**	    added coded for 2PC support.
**	12-feb-1990 (rogerk)
**	    Added parameter to dmr_db_known call.
**      24-oct-1991 (jnash)
**          Initialize add_status in dm2d_add_db call in which it was
**	    not previously initialized (caught by LINT).
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Completely rewritten.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Fixed dmr_complete_recovery call to pass num_nodes instead of
**	    node_id.  Passing node_id caused us to not logdump when database
**	    inconsistencies are found.
**	31-jan-1994 (bryanp) B57888
**	    Add err_code argument to rcp_recover; fill it in upon error.
*/
static DB_STATUS
rcp_recover(DB_ERROR *dberr)
{
    RCP			*rcp = 0;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    LG_HEADER		header;
    LG_LSN		lsn;
    i4			error;
    i4		length;
    CL_ERR_DESC		sys_err;
    i4		node_id = 0;
    DMP_LCTX		*lctx;

    CLRDBERR(dberr);

    lctx = dmf_svcb->svcb_lctx_ptr;
 
    for (;;)
    {
	/*
	** Read the Logfile Header.  This gives us the current EOF and the
	** address of the last Consistency Point.
	*/
	cl_status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), 
							&length, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_A_HEADER);
	    SETDBERR(dberr, 0, E_DM940E_RCP_RCPRECOVER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Force the log file to the current EOF before starting recovery.
	*/
	status = dm0l_force(rcp_lx_id, (LG_LSN *)0, &lsn, dberr);
	if (status != E_DB_OK)
	    break;

	break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM940E_RCP_RCPRECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM940E_RCP_RCPRECOVER);
	return (E_DB_ERROR);
    }

    /*
    ** 24-feb-1993 (andys)
    ** This next line was:
    **
    ** status = dmr_alloc_rcp( &rcp, RCP_R_ONLINE, node_id, &header );
    **
    ** I'm changing it to have the same form as in dmfrecover.c so as
    ** to void AV'ing
    */    
    status = dmr_alloc_rcp( &rcp, RCP_R_ONLINE, (DMP_LCTX **)0, (i4)0 );
 
    if (status != E_DB_OK)
    {
	uleFormat(NULL, E_DM940E_RCP_RCPRECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM940E_RCP_RCPRECOVER);
	return (E_DB_ERROR);
    }

    /*
    ** Initialize LCTX fields from the Log Header.
    */
    lctx->lctx_eof.la_sequence = header.lgh_end.la_sequence;
    lctx->lctx_eof.la_block    = header.lgh_end.la_block;
    lctx->lctx_eof.la_offset   = header.lgh_end.la_offset;
    lctx->lctx_bof.la_sequence = header.lgh_begin.la_sequence;
    lctx->lctx_bof.la_block    = header.lgh_begin.la_block;
    lctx->lctx_bof.la_offset   = header.lgh_begin.la_offset;
    lctx->lctx_cp.la_sequence  = header.lgh_cp.la_sequence;
    lctx->lctx_cp.la_block     = header.lgh_cp.la_block;
    lctx->lctx_cp.la_offset    = header.lgh_cp.la_offset;


    for (;;)
    {
	status = dmr_online_context(rcp);
	if (status != E_DB_OK)
	    break;

	status = dmr_analysis_pass(rcp, node_id);
	if (status != E_DB_OK)
	    break;

	status = dmr_init_recovery(rcp);
	if (status != E_DB_OK)
	    break;

	status = dmr_redo_pass(rcp, node_id);
	if (status != E_DB_OK)
	    break;

	status = dmr_undo_pass(rcp);
	if (status != E_DB_OK)
	    break;

	status = dmr_complete_recovery(rcp, 1);
	if (status != E_DB_OK)
	    break;

	status = release_orphan_locks(rcp);
	if (status != E_DB_OK)
	    break;

	break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&rcp->rcp_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM940E_RCP_RCPRECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM940E_RCP_RCPRECOVER);
    }

    dmr_cleanup(rcp);

    /*
    ** Deallocate the RCP
    */
    dm0m_deallocate((DM_OBJECT **)&rcp);

    return (status);
}

/*{
** Name: dmr_check_event - waiting/polling the current event of the logging
**		       system.
**
** Description:
**      This routine waits/polls the current event of the logging system.
**
**	The order in which events are processed can be important.  In order
**	to do consistency points correctly, all databases which fail in the
**	middle of a consistency point must be fully recovered before the
**	ECP record can be written.  This means that the RCP must handle all
**	outstanding RECOVER events before it acts on the ECP event.
**
**      If a serious internal error occurs (e.g., we fail to write a Consistency
**      Point in the log file), this routine takes the RCP down. If the RCP
**      isn't able to run properly, DMF can't assure the correctness/integrity
**      of the database and therefore the installation can't be allowed to
**      run.
**
** Inputs:
**	    flag			LG_NOWAIT or 0.
** Outputs:
**	Returns:
**	    E_DB_OK			situation normal
**	    E_DB_INFO			LGevent call was interrupted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	17-may-1988 (EdHsu)
**	    Added code for fast commit project.
**	10-Feb-1989 (ac)
**	    Added 2PC support.
**	25-sep-1989 (rogerk)
**	    Mark system status LGD_RCP_RECOVER while recovering transactions.
**	    This allows servers to check the system recovery status.
**      17-nov-1990 (bryanp)
**          Check error return codes and call PCexit() if a serious internal
**          error occurs.
**	25-oct-1991 (bryanp)
**	    Re-enable dmr_log_statistics call when recovery activity occurs.
**	30-0ct-1991 (bryanp)
**	    The DISABLE_DUAL_LOGGING event now is merely required to LGforce()
**	    the LG_HEADER so that the updated logging system state is saved.
**	    The RCP also will stamp its log if such an event occurs.
**	    Added event LG_E_EMER_SHUTDOWN to simulate a powerfailure for use
**	    by automated testing tools. The RCP obeys this event by shutting
**	    down rapidly (an even more immediate shutdown than IMM_SHUTDOWN).
**	16-jun-1992 (bryanp)
**	    Renamed to dmr_check_event and massaged to be called by the
**	    recovery thread in a recovery server. Changed to return E_DB_INFO
**	    to indicate when an interrupt has occurred.
**	14-dec-1992 (rogerk)
**	    Added Mike Matrigali's (x3241) bugfix to return status correctly
**	    from this routine.
**	14-dec-1992 (rogerk)
**	    Changed error handling; Check return status from rcp_recover.
**	    Added dmd_check type dump during crash failure shutdown mode.
**	18-jan-1993 (rogerk)
**	    Changed to not do Manual Abort or Manual Commit processing when
**	    the LG_NOWAIT flag is passed (meaning that a recovery event is
**	    already in progress).
**	26-apr-1993 (bryanp/keving)
**	    Added lx_id parameter. This must now be passed in explicitly
**	    since the CSP calls this routine when recovering multiple logs.
**	    The RCP passes an lx_id of 0, therefore we check for this and
**	    quietly use rcp.lx_id instead.
**	31-jan-1994 (bryanp) B57888
**	    Add err_code argument to rcp_recover; fill it in upon error.
**	12-Nov-1996 (jenjo02)
**	    Updated TRdisplay() of log events to match new values.
**       5-Dec-2007 (hanal04 Bug 119503 and Bug 118233
**          rcp_shutdown() will not return after a call to CSterminate().
**          Return with E_DB_INFO for an error message free termination
**          of the dmc_recovery thread.
**	07-Feb-2008 (hanje04)
**	    SIR 119978
**	    Defining true and false locally causes compiler errors where 
**	    they are already defined. Change the variable names to tru and
**	    fals to avoid this
*/
DB_STATUS
dmr_check_event(
RCP		*rcp,
i4		flag,
LG_LXID		lx_id)
{
    i4			status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LG_LXID		real_lx_id;
    i4		tru = 1;
    i4		fals = 0;
    i4		mask;
    i4		event = 0;
    i4		error;
    bool	shutdown = FALSE;
    DB_ERROR	local_dberr, *dberr;

    if ( rcp )
        dberr = &rcp->rcp_dberr;
    else
        dberr = &local_dberr;

    CLRDBERR(dberr);

    mask = LG_E_BCP | LG_E_RECOVER | LG_E_ECP | 
	    LG_E_IMM_SHUTDOWN | LG_E_OPENDB | LG_E_CLOSEDB | 
	    LG_E_M_ABORT | LG_E_M_COMMIT | LG_E_DISABLE_DUAL_LOGGING
	    | LG_E_EMER_SHUTDOWN	/* automated testing */
	    ;

    status = E_DB_OK;
    cl_status = OK;
    error = 0;

    for (;;)
    {
	/*
	** The RCP passes an lx_id of 0, in this case we use the global
	** lx_id stored in rcp_lx_id
	*/
	if (lx_id)
	    real_lx_id = lx_id;
	else
	    real_lx_id = rcp_lx_id;

        cl_status = LGevent(flag | LG_INTRPTABLE, real_lx_id, mask,
				    &event, &sys_err);

	if (cl_status == LG_INTERRUPT)
	{
	    TRdisplay("\n%@ Recovery Thread has been interrupted\n");
	    return (E_DB_INFO);
	}

	if (cl_status)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, mask);
	    break;
	}

	/*
	** RCP is only checking for LG_E_RECOVER events from rcpconfig 
	** during startup!
	*/
	if (rcp && rcp->rcp_action == RCP_R_STARTUP && (event & LG_E_RECOVER))
	{
	    TRdisplay("%@ RCP resume signaled.\n");

	    /* 
	    ** Turn off LG_E_RECOVER event to accept another LG_E_RECOVER
	    ** event. 
	    */
	    if (cl_status = LGalter(LG_A_RECOVERDONE,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RECOVERDONE);
	    }
	    break;
	}

	if (flag != LG_NOWAIT && (event & (LG_E_RECOVER | LG_E_IMM_SHUTDOWN)))
	{
	    TRdisplay("\n%@ RCP EVENT MASK (%x) <%v>\n", event,
		"ONLINE,BCP,LOG_FULL,,RECOVER,ARCHIVE,ACP_SHUTDOWN,\
    IMMEDIATE_SHUTDOWN,START_ARCHIVER,PURGEDB,OPENDB,CLOSEDB,,,\
    CPFLUSH,ECP,ECPDONE,CPWAKEUP,,,SBACKUP,BACKUP_FAIL,MAN_ABORT,MAN_COMMIT,\
    RCP_RECOVER,,,,DISABLE_DUAL_LOGGING,EMER_SHUTDOWN", event);

	    dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
			 DMLG_DATABASES | DMLG_TRANSACTIONS);

	}

	if (event & LG_E_EMER_SHUTDOWN)
	{
	    TRdisplay("%@ Emergency Shutdown signaled.\n");
	    PCexit(OK);
	}
	if (event & LG_E_IMM_SHUTDOWN)
	{
	    TRdisplay("%@ Immediate Shutdown signaled.\n");
	    rcp_shutdown(dberr);
            shutdown = TRUE;
            break;
	}

	/*
	** Check for recover event.  We must process the recover event
	** before we process any BCP, ECP events in order to assure correct
	** handling of consistency point protocol.
	*/
	if ((event & LG_E_RECOVER) && flag != LG_NOWAIT)
	{
	    /*
	    ** Turn on LG_E_RCPRECOVER status so servers can tell that the RCP
	    ** is doing work.
	    */
	    if (cl_status = LGalter(LG_A_RCPRECOVER,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RCPRECOVER);
		break;
	    }

	    /* 
	    ** Turn off LG_E_RECOVER event to accept another LG_E_RECOVER
	    ** event. 
	    */
	    if (cl_status = LGalter(LG_A_RECOVERDONE,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RECOVERDONE);
		break;
	    }

	    /*
	    ** Process transactions and databases that require recovery.
	    */
	    status = rcp_recover(dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Turn off LG_E_RCPRECOVER status now that recovery is complete.
	    */
	    if (cl_status = LGalter(LG_A_RCPRECOVER,
				    (PTR)&fals, sizeof(fals), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RCPRECOVER);
		break;
	    }
	}

	if ((event & LG_E_M_ABORT) && flag != LG_NOWAIT)
	{
	    TRdisplay("%@ Manually abort a transaction.\n");

	    /*
	    ** Turn on LG_E_RCPRECOVER status so servers can tell that the RCP
	    ** is doing work.
	    */
	    if (cl_status = LGalter(LG_A_RCPRECOVER,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RCPRECOVER);
		break;
	    }

	    /* 
	    ** Turn off LG_E_M_ABORT event to accept another LG_E_M_ABORT
	    ** event. 
	    */

	    if (cl_status = LGalter(LG_A_M_ABORT,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_M_ABORT);
		break;
	    }

	    status = rcp_recover(dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Turn off LG_E_RCPRECOVER status now that recovery is complete.
	    */
	    if (cl_status = LGalter(LG_A_RCPRECOVER,
				    (PTR)&fals, sizeof(fals), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_RCPRECOVER);
		break;
	    }
	}

	if ((event & LG_E_M_COMMIT) && flag != LG_NOWAIT)
	{
	    TRdisplay("%@ Manually commit a transaction.\n");

	    /* 
	    ** Turn off LG_E_M_COMMIT event to accept another LG_E_M_COMMIT
	    ** event. 
	    */

	    if (cl_status = LGalter(LG_A_M_COMMIT,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_M_COMMIT);
		break;
	    }

	    (VOID)rcp_commit();
	}

	/*
	** If LG has signaled  write a BCP or ECP record then do it.
	** The LG_E_BCP or LG_E_ECP event will be turned off inside the
	** the respective DM0L routine.
	*/
	if ((event & LG_E_BCP) && flag != LG_NOWAIT)
	{
	    status = dm0l_bcp(real_lx_id, &bcp_la, &bcp_lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}
	if ((event & LG_E_ECP) && flag != LG_NOWAIT)
	{
	    status = dm0l_ecp(real_lx_id, &bcp_la, &bcp_lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Last user of database has exited, close the database.
	** This event must be processed after the RECOVER event so that
	** databases marked for closing during recovery are not actually
	** closed till after REDO processing is complete.
	*/
	if (event & LG_E_CLOSEDB)
	{
	    status = count_opens(1);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If new database has been opened, then open it in the RCP.
	** Process this event even if the LG_NOWAIT flag is set.  This
	** means new databases can be opened while recovery is in progress.
	**
	** This event must be handled after the CLOSEDB event so that if
	** a database is undergoing RECOVERY, the closedb at the end of
	** recovery will be executed before a new attempt to open the database.
	*/
	if (event & LG_E_OPENDB)
	{
	    status = count_opens(0);
	    if (status != E_DB_OK)
		break;
	}

	if ((event & LG_E_DISABLE_DUAL_LOGGING) && flag != LG_NOWAIT)
	{
	    /*
	    ** This event is merely acknowledged. The RCP must force the 
	    ** log header to disk to record the current logging system state.
	    */
	    TRdisplay("%@ RCP: Disable dual logging occurred.\n");

	    dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
			 DMLG_DATABASES | DMLG_TRANSACTIONS);

	    if (cl_status = LGforce(LG_HDR, real_lx_id, 0, 0, &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, real_lx_id);
		break;
	    }
	    if (cl_status = LGalter(LG_A_DUAL_LOGGING,
				    (PTR)&tru, sizeof(tru), &sys_err))
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_A_DUAL_LOGGING);
		break;
	    }
	}

	break;
    }

    if (status || cl_status) 
    {
	/*
	** Some serious internal error has occurred. The problem has (hopefully)
	** been reported to the error log. Take the RCP down.
	*/
	TRdisplay("%@ RCP: Crashing due to check_event error (%d).\n",status);
	if (status && dberr->err_code)
	{
	    uleFormat(dberr, 0, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	}
	uleFormat(NULL, E_DM940F_RCP_CHKEVENT, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

	TRdisplay("################## Start Crash Dump ###################\n");
	dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
		     DMLG_DATABASES | DMLG_TRANSACTIONS);
	dmd_dmp_pool(1);
	TRdisplay("################## End Crash Dump #####################\n");

	rcp_shutdown(dberr);
    }

    if(shutdown)
        status = E_DB_INFO;

    return (status);
}

/*{
** Name: rcp_shutdown - Shutdown the recovery process.
**
** Description:
**      This routine handles the shutdown of the recovery process.
**
**	This routine classifies the shutdown into one of several severities:
**	    1)  Completely clean. LGD_START_SHUTDOWN (0x1000) is set in the
**		logging system status, no databases other than the NOT-DB are
**		currently open, and we are the only remaining process. We will
**		write a Consistency Point to record the state of the system
**		in the logfile and will set the log file header to LGH_OK to
**		indicate that nothing in the logfile needs to be scanned the
**		next time we start up.
**	    2)  No recovery work needed, but archiving is needed. If the
**		archiver died without completing its archiving work, then we
**		cannot allow rcpconfig/init to be performed and we must perform
**		the machine-failure activity upon RCP startup in order to
**		preserve the transactions in the log file which need to be
**		archived. This state is recognized because LGD_START_SHUTDOWN
**		is on but at least one real user database is still open. (Since
**		start-shutdown is on, all servers have exited, so the only
**		process that can have the DB open is the ACP). We will write
**		a Consistency Point to record the state of the system in the
**		logfile and will set the log file header to LGH_RECOVER to
**		protect against rcpconfig/init and to trigger machine failure
**		logscans during the next startup.
**	    3)  Hard shutdown. If LGD_START_SHUTDOWN is off, then we are being
**		shut down while transactions and databases are still "inflight".
**		Server buffer caches may not have been flushed, and in-progress
**		transactions will certainly need recovery. In this case, we do
**		NOT write a Consistency Point (to do so would bypass REDO
**		recovery on the next startup), and we set the log file header
**		to LGH_RECOVER to protect against rcpconfig/init and to trigger
**		machine failure recovery at the next startup.
**
** Inputs:
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	29-jan-1990 (walt)
**	    Fix bug 9626 by removing statement in rcp_shutdown() that always
**	    set the log file's status to LGH_RECOVER and caused rcpconfig/init
**	    to not run.
**	23-apr-1990 (rogerk)
**	    Fixed fast commit problem - we can't write BCP/ECP records on
**	    an rcpconfig/imm_shutdown call as it will fool the RCP into
**	    thinking there is no recovery work to do when the system
**	    is brought up again.  Also added some checks for errors during
**	    the shutdown procedure and added a shutdown errlog message.
**	26-jul-1990 (bryanp)
**	    Don't set header to LGH_OK if archiving was not completed. Also
**	    added long comment to function description describing the three
**	    types of RCP shutdown modes.
**	17-oct-1991 (rogerk)
**	    Fixed coding bug where we were attempting to set clean_shutdown
**	    to FALSE following a bad dm0l_logforce return by using
**	    "clean_shutdown == FALSE".
**	30-jun-1992 (bryanp)
**	    dm0l_logforce requires a transaction ID in the new logging system.
**	14-dec-1992 (rogerk)
**	    Changed dm0l_logforce call to dm0l_force.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      06-Dec-1994 (liibi01)
**          Bug 65068, cross integration from 6.4. 
**          Perform imm_shutdown if dmr_recover() not completed.
**	08-jun-1999 (somsa01)
**	    Added printout of server shutdown message to the errlog.log .
**	22-jun-2001 (devjo01)
**	    Integrate CSP shutdown with RCP shutdown.
**	20-May-2005 (jenjo02)
**	    Only dmfcsp_shutdown() if this is a clean shutdown with
**	    no open db's. Other conditions may require node failover
**	    recovery.
**       5-Dec-2007 (hanal04 Bug 119503 and Bug 118233
**          Call CSterminate() and wake-up the admin thread so an iimonitor
**          'set server shut' style termination of the dmfrcp. This will
**          cause all threads to go through their termination routines
**          including the removal of any socket files created in /tmp
**	30-jan-2008 (joea)
**	    Remove CSresume() from 5-dec-2007 change.
**	28-jul-2008 (kschendel)
**	    Fix bug similar to 65068 above, but related to online recovery:
**	    if RCP-RECOVERY flag is still on here, that's *not* a clean
**	    shutdown.  Online recovery probably got an error with stop-on
**	    incons-db set.  We must not write the CP at that point or we
**	    are in deep trouble at next startup.
*/
static
VOID
rcp_shutdown(DB_ERROR *dberr)
{
    CL_ERR_DESC		sys_err;
    LG_HEADER		header;
    STATUS		cl_status;
    DB_STATUS		status = E_DB_OK;
    i4		clean_shutdown = FALSE;
    i4		err_code;
    i4		length;
    i4		online_mode;
    i4		lgd_status;
    i4		num_open_dbs = 0;
    LG_DATABASE		db;
    LG_HDR_ALTER_SHOW	alter_parms;
    i4                scount;

    CLRDBERR(dberr);

    /*
    ** Show the logging system status to determine whether this is a
    ** clean shutdown or an immediate_shutdown.  If the system is being
    ** shutdown cleanly, then the LGD_START_SHUTDOWN bit will be turned
    ** on in the logging system header.
    **
    ** A system that has been shutdown cleanly can be marked as requiring
    ** no recovery actions.  We also write a begin and end consistency
    ** point pair to facilitate future recovery operations.
    **
    ** We must make sure that the system never shuts down cleanly (with
    ** the LGD_START_SHUTDOWN bit turned on) while there are recovery
    ** actions left to perform!
    */
    cl_status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status),
			&length, &sys_err);
    if (cl_status != OK)
    {
	/*
	** Since we can't show the system status, assume hard shutdown.
	*/
	_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_S_LGSTS, 
	    E_DM9410_RCP_SHUTDOWN);
	clean_shutdown = FALSE;
    }

    /*
    ** Check for CLEAN shutdown.  This is indicated by the existence of
    ** the LGD_START_SHUTDOWN bit.  This is turned off if IMM_SHUTDOWN is
    ** signalled by the DBA.
    ** Restart recovery as performed by dmr_recover() must have also
    ** completed - indicated by boolean recovery_done.
    ** Likewise any online recovery (LGD_RCP_RECOVER) must be completed
    */

    if ((cl_status == OK) && (lgd_status & LGD_START_SHUTDOWN)
	&& recovery_done
	&& (lgd_status & LGD_RCP_RECOVER) == 0)
	clean_shutdown = TRUE;
    else
	clean_shutdown = FALSE;

    for (;;)
    {
	/*
	** Force all the dirty log pages to disk to be clean.
	*/
	status = dm0l_force(rcp_lx_id, (LG_LSN *)0, (LG_LSN *)0, dberr);
	if (DB_FAILURE_MACRO(status))
	{
	    status = dmr_error_translate(E_DM9410_RCP_SHUTDOWN, dberr);
	    clean_shutdown = FALSE;
	}

	/*
	** If this is a clean shutdown, then write a Begin/End Consistency
	** Point pair.  This marks the spot from which recovery operations
	** should begin.  We do this because we know there are no transactions
	** active and no Fast Commit work to REDO. THere may still be archiving
	** work to do, however.
	*/
	if (clean_shutdown)
	{
	    status = dm0l_bcp(rcp_lx_id, &bcp_la, &bcp_lsn, dberr);
 
	    if (DB_SUCCESS_MACRO(status))
		status = dm0l_ecp(rcp_lx_id, &bcp_la, &bcp_lsn, dberr);

	    if (DB_FAILURE_MACRO(status))
	    {
		status = dmr_error_translate(E_DM9410_RCP_SHUTDOWN, dberr);
		clean_shutdown = FALSE;
	    }
	}

	/*
	** Determine if there are any databases (other than the NOT-DB) which
	** are still open. If so, then these databases must be open because
	** they have uncompleted archiver work (and the archiver has crashed),
	** so we will need to keep lgh_status set to LGH_RECOVER.
	*/
	if (clean_shutdown)
	{
	    length = 0;	    /* start with first database in system */
	    for (;;)
	    {
		cl_status = LGshow(LG_N_DATABASE, (PTR)&db, sizeof(db),
				    &length, &sys_err);
 
		if (cl_status != OK)
		{
		    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err,
					LG_N_DATABASE, E_DM9410_RCP_SHUTDOWN);
		    clean_shutdown = FALSE;
		    break;
		}
 
		if (length == 0)
		    break;	    /* successful end of DB scan */

		if ((db.db_status & DB_NOTDB) == 0)
		{
		    /*
		    ** Remember that this database was open, and confirm that it
		    ** indeed requires journalling. We check
		    ** this (just in case) to double-check against bugs in LG.
		    */
		    if ((db.db_status & DB_JNL) == 0)
		    {
			clean_shutdown = FALSE;
			TRdisplay("%@ RCP SHUTDOWN DB %~t Status: %x\n",
				    db.db_l_buffer, db.db_buffer, db.db_status);

			/* but keep going, to check for any other DB's */
		    }
		    num_open_dbs++;
		}
	    }
	    if (!clean_shutdown)
	    {
		/*
		** Something went wrong in the DB scan...
		*/
		break;
	    }
	}

	/*
	** Get the log header page so we can update the log file to indicate
	** the recovery status for the next time the system starts up.
	**
	** If we can't show the log header, then we cannot continue shutting
	** down gracefully, we should just exit and let the recovery system
	** do the right thing.
	*/
	cl_status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length,
	    &sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_HEADER, 
		E_DM9410_RCP_SHUTDOWN);
	    clean_shutdown = FALSE;
	    break;
	}

	/*
	** Write the log header with the new status that reflects its recovery
	** mode.  Note that this is not absolutely necessary - the RCP will
	** figure out the correct recovery mode anyway the next time it starts
	** up. That is, it is "safe" to leave the status at LGH_VALID or at
	** LGH_RECOVER -- the only detriment is that rcpconfig/init will be
	** disallowed. If we set the header status to LGH_OK, then NO recovery
	** will be performed at startup, and rcpconfig/init will be allowed,
	** so in order to set this status we must be SURE that no recovery
	** work is needed. If there are still open databases, for example, which
	** need archiving operations performed on them, then we must NOT set
	** LGH_OK.
	*/
	if (clean_shutdown && (num_open_dbs == 0))
	    header.lgh_status = LGH_OK;
	else
	    header.lgh_status = LGH_RECOVER;

	alter_parms.lg_hdr_lg_header = header;
	alter_parms.lg_hdr_lg_id = rcp_lg_id;
	cl_status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			    sizeof(alter_parms),
			    &sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err,
		    LG_A_NODELOG_HEADER, 
		E_DM9410_RCP_SHUTDOWN);
	    clean_shutdown = FALSE;
	    break;
	}

	/*
	** Update the log header from memory to disk.
	*/
	cl_status = LGforce(LG_HDR, rcp_lx_id, 0, 0, &sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9010_BAD_LOG_FORCE, &sys_err, rcp_lx_id, 
		E_DM9410_RCP_SHUTDOWN);
	    clean_shutdown = FALSE;
	    break;
	}

#ifdef xDEBUG
	dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
		     DMLG_DATABASES | DMLG_TRANSACTIONS);
#endif

	/*
	** Turn logging system offline.
	*/
	online_mode = 0;
	cl_status = LGalter(LG_A_ONLINE, (PTR)&online_mode,
			    sizeof(online_mode), &sys_err);
	if (cl_status != OK)
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_ONLINE, 
		E_DM9410_RCP_SHUTDOWN);
	    clean_shutdown = FALSE;
	    break;
	}

	break;
    }

    /*
    ** Log error message and bring down the RCP.  If this is a clean shutdown
    ** then the RCP is the last process to exit.  If this is a hard shutdown
    ** then the rcp's exit will cause the immediate demise of any remaining
    ** servers.
    */
    if (clean_shutdown && (num_open_dbs == 0))
    {
	if ( CXcluster_enabled() )
	{
	    /*
	    ** This CSP will now leave the cluster entirely.  Other
	    ** cluster members will no longer care if this process dies.
	    */
	    dmfcsp_shutdown();
	}

	TRdisplay("%@ RCP LG SHUTDOWN completed normally.\n");
	uleFormat( NULL, E_SC0128_SERVER_DOWN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
    }
    else if (clean_shutdown)
    {
	TRdisplay("%@ RCP LG SHUTDOWN completed.\n");
	TRdisplay("%@ RCP Archiving will be completed after system restart.\n");
	uleFormat( NULL, E_SC0128_SERVER_DOWN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
    }
    else
    {
	uleFormat(NULL, E_DM9424_RCP_HARD_SHUTDOWN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	TRdisplay("%@ RCP Shutdown completed.\n");
	TRdisplay("%@ RCP Recovery will be performed after system restart.\n");
	uleFormat( NULL, E_SC0127_SERVER_TERMINATE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
    }
    
    cl_status = CSterminate(CS_CLOSE, &scount);
    if(cl_status)
    {
        TRdisplay("%@ RCP Shutdown, call to CSterminate() failed.\n");
    }
    else
    {
        TRdisplay("%@ RCP Shutdown, CSterminate() issued. Threads exiting.\n");
    }
}

/*{
** Name: count_opens - Keep track of first open  and last close of database.
**
** Description:
**      This routines updates the configuration file to keep 
**      track of whether or not a database is open when recovery 
**      fails.  This insures that the database will be marked 
**      inconistent by the first openner after the installation 
**      has been reinitialed after a recovery has failed.  This 
**      is a backup in case the log file cannot be accessed or read.
**
** Inputs:
**      flag                            Must be zero for open
**                                      and non-zero for close.
**
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
**      17-jun-1987 (Jennifer)
**          Created for Jupiter.
**	6-Oct-1987 (ac)
**	    Fixed error handling.
**	17-jun-1988 (teg)
**	    threw in check for DB_PRETEND_CONSIST flag if config file status 
**	    is ~DSC_VALID.
**	17-Nov-1988 (ac)
**	    Restruture the code and fix the iidbdb inconsistent problems.
**	5-jun-1990 (bryanp)
**	    Explicitly clear the DCB portion of the DBCB when allocating it.
**	    This allows users of the DCB to test for uninitialized fields by
**	    comparing them with 0.
**	14-dec-1992 (rogerk)
**	    Changed to return an error rather than dmd_check if a database
**	    inconsistency is triggered when Db_incons_crash is set.
**	26-apr-1993 (bryanp)
**	    Changed RCP Mark Open/Close trace statement to be based on xDEBUG
**	    rather than xDEV_TEST so that I could get this trace statement in
**	    my debug compilations. Particularly when monitoring the database
**	    open/close processing that occurs during multi-node cluster restart
**	    recovery, I've found this trace statement to be of use.
**	    Just a comment here to say that I don't really understand the
**	    error handling in this routine. Sometimes we return an error.
**	    Sometimes we "continue". Sometimes we log a message and just plod
**	    along. Sometimes we drop off the end. This could use either some
**	    cleanup or some decent explanation of why it's this way.
**	26-apr-1993 (jnash)
**          Include non-iidbdb open/close msgs in iircp.log.
**	24-may-1993 (jnash)
**          Fix change of 26-apr to use 32 rather than 24 char iidbdb name.
** 	30-may-1997 (wonst02)
** 	    Pass readonly flag when opening .cnf file.
**	28-Oct-1997 (jenjo02)
**	    Added LK_NOLOG flag to LKrelease() call in count_opens to keep
**	    E_CL103B from appearing in the log if we justifiably don't hold
**	    the database lock we're trying to release.
**	06-Sep-2002 (bolke01)
**	    Changed  the hard codes limit of 100 to the actual max db's in
**	    the logging system bug #107757.
**	05-may-2003 (thaju02)
**	    With II_DMFRCP_STOP_ON_INCONS_DB set, if rolldb errs the db is
**	    marked inconsistent. In setting up to rollback open transactions
**	    dmfrcp finds that db is marked inconsistent and performs 
**	    shutdown of rcp along with dbms. Don't shutdown rcp if
**	    db is inconsistent due to rollforwarddb failure. (B111325)
**	 
*/
static DB_STATUS
count_opens(
i4	    open_database)
{
    i4			err_code;
    DB_STATUS		status, local_status;
    CL_ERR_DESC		sys_err;
    i4		length;
    LG_DATABASE		database;
    DM0L_ADDDB		*adddb = (DM0L_ADDDB *) database.db_buffer;
    DM0C_CNF            *cnf;
    DMP_DCB             dcb;
    i4             mask;
    bool		db_valid = TRUE;
    LK_LOCK_KEY		db_key;
    LK_LOCK_KEY		jnl_key;
    LK_VALUE		value;
    STATUS		cl_status;
    i4		runaway_loop_counter;
    i4		open_flag;
    char                *env = 0;
    DB_ERROR		dberr;

    CLRDBERR(&dberr);

    mask = 1 << rcp_node_id;
    for (runaway_loop_counter = 0; runaway_loop_counter < rconf.ldbs;
	runaway_loop_counter++)
    {
	cnf = 0;

	/*  Get next open/close pending database. */

	if (open_database)
	{
	    if (cl_status = LGshow(LG_S_CLOSEDB, (PTR)&database,
				    sizeof(database), &length, &sys_err))
	    {
		_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		return(dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, 
				    LG_S_CLOSEDB, 
				    E_DM9413_RCP_COUNT_OPENS));
	    }
	}	
	else
	{
	    if (cl_status = LGshow(LG_S_OPENDB, (PTR)&database,
				    sizeof(database), &length, &sys_err))
	    {
		_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		return(dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, 
				    LG_S_OPENDB, 
				    E_DM9413_RCP_COUNT_OPENS));
	    }
	}
	if (length == 0)
	    return(E_DB_OK);

	/*	Finish initializing the DCB. */

	MEfill( sizeof(dcb), 0, (PTR)&dcb );
	MEmove(sizeof(adddb->ad_dbname), (PTR)&adddb->ad_dbname, ' ',
			sizeof(dcb.dcb_name), (PTR)&dcb.dcb_name);
	MEmove(sizeof(adddb->ad_dbowner), (PTR)&adddb->ad_dbowner, ' ',
			sizeof(dcb.dcb_db_owner), (PTR)&dcb.dcb_db_owner);
	MEmove(adddb->ad_l_root, (PTR)&adddb->ad_root, ' ',
			sizeof(dcb.dcb_location.physical), 
                        (PTR)&dcb.dcb_location.physical);
	dcb.dcb_id = adddb->ad_dbid;
	dcb.dcb_location.phys_length = adddb->ad_l_root;
	dcb.dcb_access_mode = (database.db_status & DB_READONLY) ?
		DCB_A_READ :
		DCB_A_WRITE;
	dcb.dcb_served = DCB_MULTIPLE;
	dcb.dcb_db_type = DCB_PUBLIC;
	MEcopy((PTR)&dcb.dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);
	db_key.lk_type = LK_OPEN_DB;
	dcb.dcb_status = 0;

	if (open_database == 0)
	{
	    /*  
	    ** Get first time lock in null mode, give up
	    ** after close. 
	    */

	    status = LKrequest(LK_PHYSICAL, dmf_svcb->svcb_lock_list, &db_key,
		    LK_X, (LK_VALUE * )&value,
		    (LK_LKID *)dcb.dcb_odb_lock_id, (i4)0, &sys_err);
	    if (status != OK && status != LK_VAL_NOTVALID)
	    {
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
                        NULL, NULL, 0, NULL,
			&err_code, 2, 0, LK_X, 0, dmf_svcb->svcb_lock_list);

		uleFormat(NULL, E_DM9413_RCP_COUNT_OPENS, NULL, ULE_LOG, 
			NULL, NULL, 0, NULL, 
			&err_code, 0);
		continue;
	    }
	    if ((value.lk_high == 0) && (value.lk_low == 0))
		    value.lk_low++;
	    status = LKrequest(LK_PHYSICAL | LK_CONVERT, 
			dmf_svcb->svcb_lock_list, 
			&db_key, LK_N, (LK_VALUE * )&value,
			(LK_LKID *)dcb.dcb_odb_lock_id, (i4)0, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
                        NULL, NULL, 0, NULL,
			&err_code, 2, 0, LK_N, 0, dmf_svcb->svcb_lock_list);

		uleFormat(NULL, E_DM9413_RCP_COUNT_OPENS, NULL, ULE_LOG, 
			NULL, NULL, 0, NULL, 
			&err_code, 0);
		continue;
	    }
	    if (database.db_status & DB_JNL)
	    {
		/*  Set lock on journals of this database. */

		jnl_key.lk_type = LK_JOURNAL;
		MEcopy((PTR)&dcb.dcb_name, LK_KEY_LENGTH, (PTR)&jnl_key.lk_key1);
		status = LKrequest(LK_NOWAIT | LK_PHYSICAL, 
				dmf_svcb->svcb_lock_list, &jnl_key,
				LK_IX, 0, 0, 0, &sys_err);
		if (status != OK)
		{
		    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
			    NULL, NULL, 0, NULL,
			    &err_code, 2, 0, LK_IX, 0, dmf_svcb->svcb_lock_list);

		    uleFormat(NULL, E_DM9413_RCP_COUNT_OPENS, NULL, ULE_LOG, 
			    NULL, NULL, 0, NULL, 
			    &err_code, 0);

		    status = LKrelease(LK_PHYSICAL, dmf_svcb->svcb_lock_list, 
			(LK_LKID *)0, (LK_LOCK_KEY *)&db_key,
			(LK_VALUE *)0, &sys_err);
		    continue;
		}
	    }
	}

	open_flag = DM0C_PARTIAL;
	if (database.db_status & DB_READONLY)
	    open_flag |= DM0C_READONLY;	

	/*  Open the configuration file. */

	local_status = dm0c_open(&dcb, open_flag, dmf_svcb->svcb_lock_list, 
		    &cnf, &dberr);

	if (local_status == E_DB_OK)
	{
	    if (open_database)
	    {
		cnf->cnf_dsc->dsc_open_count &= ~mask;
	    }
	    else
	    {
		cnf->cnf_dsc->dsc_open_count |= mask;
		if ((cnf->cnf_dsc->dsc_status & DSC_VALID) == 0)
		{
		    if ( (database.db_status & DB_PRETEND_CONSISTENT) == 0)
		    {
			if (cnf->cnf_dsc->dsc_inconst_code != DSC_RFP_FAIL)
			    db_valid = FALSE;
		    }
		}
	    }
		
	    /*  Close the configuration file. */

	    local_status = dm0c_close(cnf, DM0C_UPDATE, &dberr);
	}
	if (local_status != E_DB_OK)
	    status = dmr_error_translate(E_DM9413_RCP_COUNT_OPENS, &dberr);
	if (open_database == 0)
	{
	    if (local_status == E_DB_OK && db_valid == TRUE)
	    {	    
		if (cl_status = LGalter(LG_A_OPENDB, (PTR)&database.db_id, 
			    sizeof(database.db_id), &sys_err))
		{
		    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_OPENDB, 
					E_DM9413_RCP_COUNT_OPENS);
		}
	    }
	    else
	    {
		/* 
		** Got an error trying to update config file. 
		** better mark it inconsistent. 
                */

		/* If so configured, crash rather than mark db inconsistent */
		if ( Db_recovery_error_action == RCP_STOP_ON_INCONS_DB )
		{
		    TRdisplay("\n%@ ** RCP Recovery Failed in COUNT OPENS *\n");
		    uleFormat(NULL, E_DM942A_RCP_DBINCON_FAIL, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 0);
		    return (E_DB_ERROR);
		}

		if (cl_status = LGalter(LG_A_DB, (PTR)&database.db_id, 
					sizeof(database.db_id), &sys_err))
		{
		    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		    status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_DB, 
				E_DM9413_RCP_COUNT_OPENS);
		}

		status = LKrelease(LK_PHYSICAL, dmf_svcb->svcb_lock_list, 
			(LK_LKID *)0, (LK_LOCK_KEY *)&db_key,
			(LK_VALUE *)0, &sys_err);
	    }
	}
	else
        {
	    /* 
	    ** Release first time open lock held in null. Ignore the status
	    ** returned from LKrelease() because we could close the database
	    ** even the open database event is still pending.
	    **
	    ** To prevent the disconcerting E_CL103B message from appearing 
	    ** in the log, pass the LK_NOLOG parm to LKrelease().
	    */

	    status = LKrelease((LK_PHYSICAL | LK_NOLOG), dmf_svcb->svcb_lock_list, 
                    (LK_LKID *)0, (LK_LOCK_KEY *)&db_key,
		    (LK_VALUE *)0, &sys_err);

	    if (database.db_status & DB_JNL)
	    {
		/*  Release lock on journals of this database. */

		jnl_key.lk_type = LK_JOURNAL;
		MEcopy((PTR)&dcb.dcb_name, LK_KEY_LENGTH, (PTR)&jnl_key.lk_key1);
		value.lk_high= 1;
		value.lk_low = 1;
		status = LKrelease(LK_NOLOG, dmf_svcb->svcb_lock_list, 
			 (LK_LKID *)0, (LK_LOCK_KEY *)&jnl_key, 
			 &value, &sys_err);
	    }

	    /* Even if you can't update, say it is closed so 
            ** it will be removed.  Will find out on next 
            ** open that it is inconsistent. 
            */
	    if (cl_status = LGalter(LG_A_CLOSEDB, (PTR)&database.db_id, 
			sizeof(database.db_id), &sys_err))
	    {
		_VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		status = dmr_log_error(E_DM900B_BAD_LOG_ALTER, 
                            &sys_err, LG_A_CLOSEDB, 
			    E_DM9413_RCP_COUNT_OPENS);
	    }
	}

#ifndef xDEBUG
	/*
	** If not the iidbdb, note open/close in rcp log file.
	** (When running xDEBUG always mark open/close operations.)
	*/
	if (MEcmp((char *)adddb->ad_dbname.db_db_name,
	    DB_DBDB_NAME, sizeof(DB_DB_NAME)) != 0)
#endif
	{
	    TRdisplay("%@ RCP Mark %5w %~t ID: %x ROOT: %t\n",
		"open,close", open_database,
		sizeof(adddb->ad_dbname), &adddb->ad_dbname,
		database.db_id, adddb->ad_l_root, &adddb->ad_root);
	}
    }
    if (runaway_loop_counter >= rconf.ldbs)
    {
	TRdisplay("RCP Panic! Processed more than %d opendb requests!\n",rconf.ldbs);
	dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
		     DMLG_DATABASES | DMLG_TRANSACTIONS);
	PCexit(FAIL);
    }
}

/*{
** Name: rcp_commit - Commit a transaction.
**
** Description:
**	This routine handles the request of manually committing a transaction.
**      It writes the commit log record for a transaction, removes the 
**	transaction information from the logging system, and releases all 
**	the locks held by the transaction. Note: All the changes made by
**	the transaction have been flushed to disk at the disconnection time
**	or at the server recovery time.
**
** Inputs:
**	none
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-Feb-1989 (ac)
**	    Created for 2PC support.
**	10-apr-1992 (bryanp)
**	    Fix bad args to TRdisplay (we were passing EID structure by value,
**	    but we need to pass the individual components separately).
**	18-oct-1993 (rogerk)
**	    Added ownership transfer of the transaction by the RCP so that 
**	    log records can be written on behalf of that transaction.
**	    Changed the dm0l_et call to use the commit transaction identifier
**	    rather than the RCP tx to write the ET record.  This allows the
**	    ET record to be correctly linked into the transaction log record
**	    stream.  Removed obsolete LG_RCP flag to LGend calls.
**	16-sep-1996 (canor01)
**	    Fixed typo in message.
**      15-jun-1999 (hweho01)
**          Fixed parameter list in the function call of ule_format().
*/
static
VOID
rcp_commit(VOID)
{
    LG_TRANSACTION	tr;
    i4		length;
    i4		et_flags = DM0L_MASTER;
    DB_TAB_TIMESTAMP    ctime;
    i4		err_code;
    CL_ERR_DESC		sys_err;
    LK_LLID		lock_id;
    DB_STATUS		status;
    STATUS		cl_status;
    DB_ERROR		dberr;

    CLRDBERR(&dberr);

    for (;;)
    {
	if (cl_status = LGshow(LG_S_MAN_COMMIT, (PTR)&tr, sizeof(tr),
				&length, &sys_err))
	{
	    _VOID_ uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    status = dmr_log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, 
				    LG_S_MAN_COMMIT, E_DM9422_RCP_COMMIT);
	    break;
	}
	if (length == 0)
	{
	    TRdisplay("No more transaction need to be manually commmitted.\n");
	    return;
	}

	/*
	** Adopt ownership of the transaction for recovery.
	*/
	tr.tr_pr_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
	CSget_sid(&tr.tr_sess_id);

	cl_status = LGalter(LG_A_LXB_OWNER, (PTR)&tr, sizeof(tr), &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
		NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		0, LG_A_LXB_OWNER);
	    uleFormat(NULL, E_DM9422_RCP_COMMIT, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    break;
	}

	/* Get the lock list of the transaction. */

	if (LKcreate_list((i4)LK_RECOVER, 
		    (LK_LLID)0, 
		    (LK_UNIQUE *)&(tr.tr_eid), 
		    (LK_LLID *)&lock_id, 0,
		    &sys_err))
	{
	    (VOID) uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, 
                    ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	    uleFormat(NULL, E_DM9422_RCP_COMMIT, 0, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
	    break;
	}

	if (tr.tr_status & TR_JOURNAL)
	    et_flags |= DM0L_JOURNAL;

	if (dm0l_et(tr.tr_id, et_flags, DM0L_COMMIT, 
		(DB_TRAN_ID *)&tr.tr_eid, tr.tr_db_id, &ctime, &dberr))
	{
	    status = dmr_error_translate(E_DM9422_RCP_COMMIT, &dberr);
	    break;
	}

	/* 
	** Remove the transacton context in the logging system and release 
	** locks held by the transaction.
	*/

	if (LGend(tr.tr_id, (i4)0, &sys_err))
	{
	    status = dmr_log_error(E_DM900E_BAD_LOG_END, &sys_err, 
			    tr.tr_id, E_DM9422_RCP_COMMIT);
	    break;
	}
    
	if (LKrelease(LK_ALL | LK_RELATED, lock_id, 
			(LK_LKID *)0, 
			(LK_LOCK_KEY *)0, 
			(LK_VALUE *)0, &sys_err))
	{
	    status = dmr_log_error(E_DM901B_BAD_LOCK_RELEASE, &sys_err, 
			    lock_id, E_DM9422_RCP_COMMIT);
	    break;
	}
	/* 
	** Note:: This routine must run successfully. If any error occurs, it
	** means fatal error.
	*/

	TRdisplay("%@: Successfully commit the transaction\n\
\tEID : %x%x   USER: User: <%~t>\n",
	tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran,
	tr.tr_l_user_name, &tr.tr_user_name[0]); 
    }
    TRdisplay("%@: Errors occurred in committing the transaction\n\
\tEID : %x%x   USER: User: <%~t>\n",
	tr.tr_eid.db_high_tran, tr.tr_eid.db_low_tran,
	tr.tr_l_user_name, &tr.tr_user_name[0]); 
}

/*{
** Name: release_orphan_locks - Release orphan locks following online recovery.
**
** Description:
**	This routine is called at the completion of online recovery to
**	release any orphaned lock lists that belonged to transactions which
**	have died but were not processed in the recovery phase (meaning they
**	were read transactions).
**
**	Each locklist left around that is owned by one of the failed processes
**	we just recovered is released.
**
** Inputs:
**	RCP			Recovery context
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-dec-1992 (rogerk)
**	    Reduced logging project: Converted from old online recovery code.
**	15-may-1994 (rmuth)
**	    b59248
**	    - Scanning for orphan lock lists for a process, exited loop 
**	      after first list need to continue looking.
**      27-feb-1997 (stial01)
**          Release ORPHAN XSVDB lock lists
*/
static DB_STATUS
release_orphan_locks(
RCP	    *rcp)
{
    RLP		    *rlp;
    LK_LLID	    lli;
    i4	    context;
    i4	    length;
    i4	    i;
    CL_ERR_DESC	    sys_err;
    DB_STATUS	    status = E_DB_OK;
    STATUS	    cl_status = OK;
    RCP_QUEUE	    *st;
    RDB		    *rdb;
    i4		    error;

    CLRDBERR(&rcp->rcp_dberr);

    /*
    ** Cycle through the process list and ask the Locking System whether there
    ** are any lock lists left around which were owned by thoses processes.
    ** Release any that are found.
    **
    ** Errors encountered here are logged, but not returned.  We continue
    ** processing with the risk that objects may remain locked in the system
    ** and that the system may need to be restarted to release them.
    */

    for (i = 0; i < RCP_LP_HASH; i++)
    {
	for (rlp = rcp->rcp_lph[i]; rlp != NULL; rlp = rlp->rlp_next)
	{

	    /*
	    ** Look for orphaned lock lists which used to be owned by
	    ** the dead process.
	    */
	    context = 0;
	    for (;;)
	    {
	        cl_status = LKshow( LK_S_ORPHANED, 0, 0, 
			            (LK_LOCK_KEY *)&rlp->rlp_pid,
			            sizeof(lli), (PTR)&lli, (u_i4 *)&length,
			    	    (u_i4 *)&context, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, LK_S_ORPHANED);
		    status = E_DB_ERROR;
		    break;
		}

	        /*
		** If no orphaned locks were found, then go to the next process.
		*/
		if (context == 0)
		    break;

		/*
		** Release the orphaned lock list.
		*/
		cl_status = LKrelease(LK_ALL | LK_RELATED, lli, (LK_LKID *)0, 
				(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, 
				ULE_LOG, NULL, 
			        (char *)NULL, (i4)0, (i4 *)NULL, &error, 
				1, 0, lli);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** If encountered an error issue a message and continue
	    */
	    if (status != E_DB_OK)
	    {
		TRdisplay(
		 "Error releasing Orphaned Lock Lists in online recovery\n");
		uleFormat( NULL, E_DM940E_RCP_RCPRECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    }
	}
    }

    /*
    ** If we assumed ownership of XSVDB lock list, release it now.
    **
    ** It is an ORPHAN lock list but it has the RCP pid.
    ** This loops through and releases XSVDB lock lists.
    **
    ** I think we could have also released ORPHAN lock lists with the RCP pid. 
    */
    for (st = rcp->rcp_db_state.stq_next;
	(status == E_DB_OK) && (st != (RCP_QUEUE *)&rcp->rcp_db_state.stq_next);
	st = rdb->rdb_state.stq_next)
    {
	rdb = (RDB *)((char *)st - CL_OFFSETOF(RDB, rdb_state));

	/*
	** If partial redo, make sure we have SV_DATABASE lock
	*/
	if (rdb->rdb_status & RDB_XSVDB_LOCK)
	{
	    /*
	    ** Release the orphaned lock list.
	    */
	    cl_status = LKrelease(LK_ALL | LK_RELATED, rdb->rdb_svdb_ll_id, 
				(LK_LKID *)0, (LK_LOCK_KEY *)0,
			        (LK_VALUE *)0, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, 
			    ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 
			    1, 0, lli);
		status = E_DB_ERROR;
		break;
	    }
	}
    }

    /*
    ** Return OK even if an error was encoutered.
    */
    return (E_DB_OK);
}

/*{
** Name: dmrErrorTranslateFcn	- Convert the DMF error to RCP error code.
**
** Description:
**	This routine converts the DMF error to RCP error code and log both
**	error codes.
**
** Inputs:
**	new_error			    New error number to convert.
**	dberr				    Old error number.
**
** Outputs:
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	25-Nov-2008 (jonj)
**	    Renamed from dmr_error_translate, now invoked with that macro.
**	    In translated error message, report where called from.
*/
static DB_STATUS
dmrErrorTranslateFcn(
i4		new_error,
DB_ERROR	*dberr,
PTR		FileName,
i4		LineNumber)
{
    i4	        local_err_code;
    DB_ERROR	local_dberr;

    /* Report the originating error and source */
    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
    		NULL, 0, NULL, &local_err_code, 0);

    /* translate to new error code */
    dberr->err_code = new_error;

    /* Report whence we were called with translated error */
    local_dberr.err_code = new_error;
    local_dberr.err_data = 0;
    local_dberr.err_file = FileName;
    local_dberr.err_line = LineNumber;

    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL,
    		NULL, 0, NULL, &local_err_code, 0);

    return(E_DB_ERROR);
}

/*{
** Name: dmrLogErrorFcn	- Log the CL error message, convert the error
**			  code to the proper RCP error code.
**
** Description:
**	This routine logs the CL error message and convert the error
**	code to the proper RCP error code and log both error codes.
**
** Inputs:
**      err_code                        The error code to lookup.
**	sys_err				Pointer to associated CL error
**	p				Parameter for the error code.
**	ret_err_code			RCP error code.
**	FileName			Source from where called
**	LineNumber			...and that line number.
** Outputs: 
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Oct-1986 (ac)
**          Created for Jupiter.
*/
DB_STATUS
dmrLogErrorFcn(
DB_STATUS           err_code,
CL_ERR_DESC	    *syserr,
i4		    p,
i4		    ret_err_code,
PTR		    FileName,
i4		    LineNumber)
{
    DB_STATUS	    local_err_code;
    DB_ERROR	    dberr;

    dberr.err_code = err_code;
    dberr.err_data = 0;
    dberr.err_file = FileName;
    dberr.err_line = LineNumber;

    uleFormat(&dberr, 0, syserr, ULE_LOG, NULL,
	(char *)NULL, 0L, (i4 *)NULL, &local_err_code, 1, 0, p);

    /* retain error source information */
    dberr.err_code = ret_err_code;

    uleFormat(&dberr, 0, NULL, ULE_LOG, NULL,
	(char *)NULL, 0L, (i4 *)NULL, &local_err_code, 0);

    return (E_DB_ERROR);
}


/*{
** Name: rcp_get_error_action() set rcp recovery error action
**
** Description:
**      Recovery error action can be specified by setting
**      environment variable II_DMFRCP_STOP_ON_INCONS_DB
**      or
**      cbf parameters ii.$.rcp.offline_error_action,
**      			ii.$.rcp.online_error_action
**      But is is a usage error to set both.
**
**      When both are set, II_DMFRCP_STOP_ON_INCONS_DB will be used.
**
**      This routine checks for the variable/parameters and sets
**      rcp global variables accordingly.
**
** Inputs:
**      None
** Outputs: 
**      None
**
** Side Effects:
**	Sets global variables 
**
** History:
**      20-may-2009 (stial01)
**          Created.
*/
static VOID
rcp_get_error_action(VOID)
{
    char	*pm_str;
    STATUS	status;
    i4		err_code;
    char 	*parm_name;
    char	*env;
    i4		startup_action;
    i4		passabort_action;

    Db_stop_on_incons_db = '\0';
    Db_startup_error_action = RCP_CONTINUE_IGNORE_DB;
    Db_pass_abort_error_action = RCP_CONTINUE_IGNORE_DB;

    NMgtAt( ERx("II_DMFRCP_STOP_ON_INCONS_DB"), &env);
    if ( env && *env)
    {
	Db_stop_on_incons_db = *env;  /* save it */
	if ( CMcmpnocase(env, "Y") == 0)
	{
	    TRdisplay("II_DMFRCP_STOP_ON_INCONS_DB enabled.\n");
	    Db_startup_error_action = RCP_STOP_ON_INCONS_DB;
	    Db_pass_abort_error_action = RCP_STOP_ON_INCONS_DB;
	}
    }

    /* 
    ** For backwards compatibility...
    ** DONT override II_DMFRCP_STOP_ON_INCONS_DB
    **
    ** If the new config params are set with II_DMFRCP_STOP_ON_INCONS_DB
    ** print a usage warning
    */

    startup_action = RCP_UNDEFINED_ACTION;
    parm_name = "!.offline_error_action";
    status = PMget(parm_name, &pm_str);
    if (status)
    {
	if (status != PM_NOT_FOUND)
	{
	    /* Issue error but otherwise ignore it  */
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
    }
    else
    {
	/*
	** Supported offline error actions are
	** CONTINUE_IGNORE_DB, CONTINUE_INGORE_TABLE and STOP
	**
	** If any redo/undo failure occurs the only supported way to
	** recover the data is rollforwarddb (or rollforwarddb -table)
	** 
	** The following offline options are undocumented/unsupported:
	** CONTINUE_IGNORE_PAGE 
	** PROMPT
	*/
	if (STxcompare(pm_str, 0, "continue_ignore_db", 18, 1, 0) == 0)
	    startup_action =   RCP_CONTINUE_IGNORE_DB;
	else if (STxcompare(pm_str, 0, "continue_ignore_table", 21, 1, 0) == 0)
	    startup_action =   RCP_CONTINUE_IGNORE_TABLE;
	else if (STxcompare(pm_str, 0, "stop", 4, 1, 0) == 0 )
	    startup_action =   RCP_STOP_ON_INCONS_DB;
	else if (STxcompare(pm_str, 0, "continue_ignore_page", 20, 1, 0) == 0)
	    startup_action =   RCP_CONTINUE_IGNORE_PAGE; /* unsupported */
	else if (STxcompare(pm_str, 0, "prompt", 6, 1, 0) == 0 )
	    startup_action =  RCP_PROMPT;  /* unsupported */
	else
	    TRdisplay("Ignoring %s Invalid value %s\n",  
		parm_name, pm_str);
    }

    if (startup_action != RCP_UNDEFINED_ACTION)
    {
	if (env && *env)
	    TRdisplay("USAGE error II_DMFRCP_STOP_ON_INCONS_DB with %s %s\n",
		parm_name, pm_str);
	else
	{
	    Db_startup_error_action = startup_action;
	    TRdisplay("Enabled %s %s \n", parm_name, pm_str);
	}
    }

    passabort_action = RCP_UNDEFINED_ACTION;
    parm_name = "!.online_error_action";
    status = PMget(parm_name, &pm_str);
    if (status)
    {
	if (status != PM_NOT_FOUND)
	{
	    /* Issue error but otherwise ignore it  */
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
    }
    else
    {
	/*
	** Supported online error actions are
	** CONTINUE_IGNORE_DB and STOP
	*/
	if (STxcompare(pm_str, 0, "continue_ignore_db", 18, 1, 0) == 0)
	    passabort_action = RCP_CONTINUE_IGNORE_DB;
	else if (STxcompare(pm_str, 0, "stop", 4, 1, 0) == 0 )
	    passabort_action = RCP_STOP_ON_INCONS_DB;
	else
	    TRdisplay("Ignoring %s Invalid value %s\n",  
		parm_name, pm_str);
    }

    if (passabort_action != RCP_UNDEFINED_ACTION)
    {
	if (env && *env)
	    TRdisplay("USAGE error II_DMFRCP_STOP_ON_INCONS_DB with %s %s\n",
		parm_name, pm_str);
	else
	{
	    Db_pass_abort_error_action = passabort_action;
	    TRdisplay("Enabled %s %s \n", parm_name, pm_str);
	}
    }

    TRdisplay("Startup (offline) error action %30w\n",
	RCP_ERROR_ACTION, Db_startup_error_action);

    TRdisplay("Pass abort (online) error action %30w\n",
	RCP_ERROR_ACTION, Db_pass_abort_error_action);

}


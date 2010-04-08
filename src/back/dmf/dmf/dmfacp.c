/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <pc.h>
#include    <tr.h>
#include    <cm.h>
#include    <nm.h>
#include    <lo.h>
#include    <st.h>
#include    <me.h>
#include    <ex.h>
#include    <cs.h>
#include    <er.h>
#include    <jf.h>
#include    <cv.h>
#include    <gc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <ulf.h>
#include    <ulx.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dm0llctx.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0j.h>
#include    <dmftrace.h>
#include    <dmfacp.h>
#include    <lgclustr.h>
#include    <dmftm.h>
#include    <lgdstat.h>
#include    <dmd.h>

/**
**
**  Name: DMFACP.C - DMF Archive Control Program.
**
**  Description:
**      This file contains the DMF archiver program.  The archiver has the
**	respoonsibility of archiving the log file to individual journal files
**	for each journaled database.  The archiver can be running all the time
**	or can be invoked dynamically.
**
**	When the archiver exits, the acp_exit_handler is invoked to record
**	the reason for archiver exit, and to call out to the acpexit command
**	file to allow execution of user-defined actions.
**
**	Any archiver routine which encounters an error which will cause
**	cessation of archive processing (most errors currently fall into this
**	category) must set the acb_error_code field indicating the type
**	of error and format the shutdown error message into the acb_err_text
**	fields.   Valid error codes are defined in DMFACP.H.  If the error
**	is specific to a particular database, the acb_err_database field
**	should also be filled in.
**
**	In addition to filling in the acb_error_code information for archiver
**	shutdown processing, the error handling specific to the particular
**	archiver routine should log enough information about the error just 
**	encountered to allow resolution of the problem.
**
**	As new error checks are added to the archiver, developers must be sure
**	to fill in the above information in the error handling code (unless
**	the error is not one to cause cessation of archiver processing).
**	Failure to do this will cause the archiver to shutdown without any
**	indication of the reason or resolution.
**	
**          main - Main program.
**
**	    Internal routines:
**
**
**
**  History:
**      22-jul-1986 (Derek)
**          Created for Jupiter.
**	 2-Aug-1988 (Edhsu)
**	    Added jobtodo routine to decide whether acp needs to work.
**	31-oct-1988 (mmm)
**	    Deleted "SETUID" hint.  The ACP, RCP, and rcpconfig all must be
**	    run by the ingres user.  Making them setuid creates a security
**	    problem in the system.  A better long term solution would be to
**	    have these programs check whether the person running them
**	    is a super-user.  The install script on unix should be changed to
**	    make these programs mode 4700, so that only ingres can run them
**	    and if root happens to run them they will still act as if ingres
**	    started them.
**	31-oct-1988 (rogerk)
**	    Print out free_wait and stall_wait statistics in log_statistics().
**	05-jan-1989 (mikem)
**	    Fix some error messages for unix, and make sure dmfacp always exits
**	    through PCexit() -- fixes bug #4405.
**	24-jan-1989 (roger)
**	    It is illegal to peer inside the CL error descriptor.  Therefore,
**	    can only PCexit(FAIL) if LGinitialize fails.
**	26-dec-1989 (greg)
**	    Porting changes -- log_statistics -> log_acp_stats
**			       cover for main
**	10-jan-1990 (rogerk)
**	    Added Online Backup changes.
**	    Added Online Purge event - this is a PURGE that can be signaled
**	    while the database is opened.  Added DBCB_ONLINE_PURGE to indicate
**	    that purge is online.
**	    Added DBCB_BACKUP status and sbackup log address to DBCB for
**	    online backup processing.
**	    Added alloc_jnl_context routine to consolidate dbcb allocation
**	    code from get_jnldb and get_purgedb.
**      23-jan-1990 (mikem)
**          Unix compiler fix, attempt was made to STRUCT_ASSIGN
**          a LG_LA to DM_LOG_ADDR and caused a compiler error.  Adding
**          a cast/dereference fixed the problem.
**	30-apr-1990 (sandyh)
**	    bug #21370
**	    In jobtodo() use only oldest transaction that is in ACTIVE,PROTECT
**	    state; otherwise the lxb_cp_lga value will contain a possibly bad
**	    cp address which will cause the scan to proceed to the EOF causing
**	    the ACP to crash.
**	    Changed return status to E_DB_OK if work found, E_DB_INFO otherwise.
**	8-may-90 (bryanp)
**	    Added code to check return codes and handle errors better.  If
**	    We can't open II_ACP.LOG, added code to display a message to
**	    the 'terminal', if possible.
**	5-jun-1990 (bryanp)
**	    Explicitly clear the DCB portion of the DBCB when allocating it.
**	20-jul-1990 (bryanp)
**	    Minor bugfix to alloc_jnl_context() and additional tracing messages.
**	08-aug-1990 (jonb)
**          Add spaces in front of "main(" so mkming won't think this
**          is a main routine on UNIX machines where II_DMF_MERGE is
**          always defined (reintegration of 14-mar-1990 change).
**	19-nov-1990 (bryanp)
**	    Fix a few "used before set" problems noted by Roger Taranto.
**	18-dec-1990 (bonobo)
** 	    Added the PARTIAL_LD mode, and moved "main(argc, argv)" back to
**	    the beginning of the line.
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	 4-feb-1991 (rogerk)
**	    Declared exception handler at ACP startup so we won't silently
**	    exit on access violations.  Added routine ex_handler.
**	25-feb-1991 (rogerk)
**	    Added changes for Archiver Stability project.
**	    Added acb_error_code, acb_err_text, acb_err_database fields to
**	    the archiver control block.  These fields are formatted whenever
**	    an acp-fatal error is encountered.  The error code and message
**	    written from the acp_exit_handler to give final indication of
**	    the archiver shutdown type.
**	    Made changes to much error handling code in DMFACP.C and in
**	    DMFARCHIVE.C to format the above fields for shutdown processing.
**	    Added acp_exit_handler routine to do acp shutdown processing.
**	    Added acpexit command script which is executed (via PCcmdline)
**	    whenever the acp exits.
**	    Added more detailed error information when memory allocation
**	    errors are encountered.
**	17-mar-1991 (bryanp)
**	    B36228: Initialize dbcb_current_lps when allocating DBCB.
**	13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	9-aug-1991 (bryanp)
**	    Log CL error code when LGevent() fails.
**      12-aug-91 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5.  The
**	    only changes made to the 6.5 version of this file were bugfixes
**	    which had already been made in 6.4 as well.
**	10-apr-1992 (rogerk)
**	    Fix call to ERinit().  Added missing args: p_sem_func, v_sem_func.
**	    Even though we do not pass in semaphore routines, we still need
**	    to pass the correct number of arguments.
**	07-jul-1992 (ananth)
**	    Prototyping DMF.
**	25-aug_1992 (ananth)
**	    Get rid of HP compiler warnings.
**	22-sep-1992 (bryanp)
**	    Prototyping LG and LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	30-sep-1992 (jnash)
**	    Change name of ACP log file from II_ACP.LOG to iiacp.log.
**	    Append to end of ACP log file on UNIX.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	21-dec-1992 (jnash & rogerk)
**	    Rewritten for the Reduced Logging Project.  Page 
**	    oriented recovery mechanisms require that database pages
**	    be "moved forward" to their state at the time of the crash,
**	    including pages affected by transaction rollback.  This
**	    is also the case for ROLLFORWARD, hence we now need to
**	    archive aborted transactions.  This in turn allows us to make 
**	    archiving a single pass operation.  As of this point,
**	    the archiver no longer tracks transactions -- just 
**	    databases -- and all multi-pass procedures disappear.  
**	    PURGE and BACKUP events are no longer necessary, and the 
**	    archiver writes to the dump file at the same time as it
**	    write to the journals.  The archiver no longer operates 
**	    between CP boundaries, but instead moves log data from a 
**	    CP to the log file EOF.
**	25-jan-1993 (rogerk)
**	    Restore Bryanp's change to call scf_call instead of CSinitiate
**	    to initialize CS processing.  Without this, any new CSsemaphore
**	    calls added before the first scf_call will fail.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Eliminated EBACKUP signal.  The archiver is now woken for
**		online backup with an LG_E_ARCHIVE signal.
**	16-mar-1993 (rmuth)
**	    Include di.h
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Added lctx parameter to dma_prepare to specify the logfile
**		    context when performing node-specific archiving in the CSP.
**		Respond to some lint suggestions, particularly including
**		    deleting the unused function "error_translate".
**	21-jun-1993 (bryanp)
**	    Add a call to LKinitialize at startup.
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
**	    Changed journal and dump window tracking in the logging system.
**	    Changed to use new archive_windows and database dmp jnl fields.
**      26-jul-1993 (mikem)
**          Initialize MO interface to provide dbms performance stats through
**          queries.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	18-oct-1993 (rogerk)
**	    Added new transaction status values to log_acp_stats.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so ule_format them.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare call.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	12-Nov-1996 (jenjo02)
**	    Update TRdisplay() of log events to match new values.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**      15-may-97 (mcgem01)
**          Clean up compiler error on NT; ERinit takes 4 parameters.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	29-Jan-1998 (jenjo02)
**	    Partitioned log files:
**	    add display of lgh_partitions to log header display.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	08-jun-1999 (somsa01)
**	    Added printout of startup message to errlog.log in dmfacp().
**	    Also, properly set up the hostname in ule_initiate().
**	28-jun-1999 (somsa01)
**	    Changed GLOBALDEF to GLOBALREF for Version[].
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-apr-1999 (hanch04)
**	    Replace STmove with STncpy
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	13-sep-2001 (mcgem01)
**	    Log the Archiver Normal Shutdown message to the errlog.log file
**	    as well as to iiacp.log.
**      26-feb-2003 (stial01)
**          Make sure acpexit gets called if archiver gets SIGTERM, which is
**          how the archiver is shutdown when you do ingstop -dmfacp (b109733)
**      02-apr-2003 (stial01)
**          Removed 26-feb-2003 change for b109733. The fix wasn't generic,
**          and further inspection revealed the reason for the problem was
**          that we need to call EXsetclient before the EXdeclare.
**	02-May-2003 (jenjo02)
**	    Corrected lgd_status bit interpretations.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	03-Oct-2005 (hanje04)
**	    BUG 114921
**	    For now when we receive a SIGHUP, just log it and carry on. Under
**	    certain (as yet unknow) conditions on Linux, the ACP receives
**	    a SIGHUP from the STDIN FD it is polling on.
**	    This fix maybe removed once the cause of the hangup is found.
**	27-Oct-2005 (hanje04)
**	    BUG 114921
**	    Can't reference SIGHUP in generic code, check for EXHANGUP instead.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**/


/*
**	mkming hints

NEEDLIBS =	DMFLIB SCFLIB ADFLIB GWFLIB  ULFLIB GCFLIB COMPATLIB MALLOCLIB  

MODE =		PARTIAL_LD

OWNER =		INGUSER

PROGRAM =	dmfacp

*/

/*
** External variables 
*/
GLOBALREF char  Version[];

/*
**  Definition of static variables and forward static functions.
*/

static i4 acp_sigterm(int sig);

static STATUS dmfacp(VOID);

static STATUS ex_handler(
    EX_ARGS	*ex_args);

static VOID acp_exit_handler(
    i4	exit_code,
    char	*exit_message,
    i4		exit_msg_len,
    char	*dbname);


/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	dmfacp_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif


/*{
** Name: main	- Main program
**
** Description:
**      This is the main entry point of the ACP.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      22-jul-1986 (Derek)
**          Created for Jupiter.
**	09-Nov-1988 (EdHsu)
**	    Added code to tell logging system to notify operator to
**	    restart acp process.
**	8-may-90 (bryanp)
**	    Check return codes of NMloc and TRset_file calls. If we are unable
**	    to open II_ACP.LOG, we cannot continue. If possible, we open
**	    the 'terminal' and issue an informative message there. Regardless,
**	    we refuse to come up if II_ACP.LOG cannot be written to.
**	19-nov-1990 (bryanp)
**	    Fix too-small declaration of 'filename' array noticed by Roger
**	    Taranto.
**      10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	 4-feb-1991 (rogerk)
**	    Declared exception handler at ACP startup so we won't silently
**	    exit on access violations.  Added routine ex_handler.
**	25-feb-1991 (rogerk)
**	    Added changes for Archiver Stability probect.
**	    Added acp_exit_handler routine to do archiver shutdown processing.
**	    This routine should always be called before the archiver exits.
**	13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	9-aug-1991 (bryanp)
**	    Log CL error code when LGevent() fails.
**	    Ensure 'err_code' is always set when breaking out of mainloop with
**	    an error code.
**      12-aug-91 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	10-apr-1992 (rogerk)
**	    Fix call to ERinit().  Added missing args: p_sem_func, v_sem_func.
**	    Even though we do not pass in semaphore routines, we still need
**	    to pass the correct number of arguments.
**	30-sep-1992 (jnash)
**	    Change name of ACP log file from II_ACP.LOG to iiacp.log.
**	    Append to end of ACP log file on UNIX.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	22-dec-1992 (jnash)
**	    Rewritten for the reduced logging project.
**	    The major change is that archiving is now done in a single
**	    pass, rather than the many required previously.  Furthermore, 
**	    the LG_E_ARCHIVE, LG_E_PURGEDB and LG_E_EBACKUP events
**	    are now managed as a single event.  
**	25-jan-1993 (rogerk)
**	    Restore Bryanp's change to call scf_call instead of CSinitiate
**	    to initialize CS processing.  Without this, any new CSsemaphore
**	    calls added before the first scf_call will fail.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Eliminated EBACKUP signal.  The archiver is now woken for
**		online backup with an LG_E_ARCHIVE signal.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Added lctx parameter to dma_prepare to specify the logfile
**		    context when performing node-specific archiving in the CSP.
**	21-jun-1993 (bryanp)
**	    Add a call to LKinitialize at startup.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**      26-jul-1993 (mikem)
**          Initialize MO interface to provide dbms performance stats through
**          queries.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so ule_format them.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare call.
**	12-Nov-1996 (jenjo02)
**	    Update TRdisplay() of log events to match new values.
**      02-apr-1997 (hanch04)
**          move main to front of line
**	15-jan-1999 (nanpr01)
**	    Pass the pointer to pointer to dma_complete function.
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	09-oct-2002 (devjo01)
**	    Fixed stoopid bug introduced by Tru64 optimiser.  'filename'
**	    was passed to NMloc, and its address was retrieved by LOtos
**	    into 'qualname'.  However compiler thought we were done with
**	    memory held by 'filename', so 'qualname' overlay the same
**	    stackspace, trashing the contents of 'filename' when 'filename's
**	    address was read into it!  Fix is to assign 'filename' to 
**	    'qualname', making explicit the relation between them.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	21-Mar-2005 (mutma03)
**	    Fixed the issue with the logfile name for clusters by allocating
**	    right memory for node_name.
**	15-Mar-2006 (jenjo02)
**	    Replaced log_acp_stats() with call to dmd_log_info().
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**
*/
# ifdef	II_DMF_MERGE
VOID MAIN(argc, argv)
# else
VOID 
main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif

    /* make it so that if ACP returns by either a return or by dropping off
    ** the end of the procedure we will still exit through PCexit(), thus
    ** insuring PCatexit() handlers will get run.
    */
    PCexit(dmfacp());
}

static STATUS
dmfacp(VOID)
{
    ACB			*acb;
    ACB			*a;
    DB_STATUS		status;
    STATUS              ret_status;
    CL_ERR_DESC		sys_err;
    EX_CONTEXT		context;
    SCF_CB		scf_cb;
    i4			err_code;
    i4			acp_restart = 0;
    i4			exit_error;
    i4			exit_msg_len;
    char		exit_db[DB_MAXNAME + 1];
    char		exit_text[ER_MAX_LEN];
    DB_ERROR		dberr;

    CLRDBERR(&dberr);

    /*
    ** Declare exception handler.
    ** All exceptions in the archiver are fatal.
    **
    ** When an exception occurs, execution will continue here.  The
    ** exception handler will have logged an error message giving the
    ** exception type and hopefully useful information indicating where
    ** it occurred.
    */
    (void) EXsetclient(EX_INGRES_DBMS);
    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	uleFormat(NULL, E_DM904A_FATAL_EXCEPTION, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code,
	    0);

	/*
	** Format exit error message and call exit handler.
	*/
	exit_error = E_DM9859_ACP_EXCEPTION_EXIT;
	uleFormat(NULL, exit_error, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, exit_text, sizeof(exit_text), &exit_msg_len,
	    &err_code, 0); 
	acp_exit_handler(exit_error, exit_text, exit_msg_len, (char *)NULL);
	PCexit(FAIL);
    }

    /* Set the proper character set for CM */

    ret_status = CMset_charset(&sys_err);

    if ( ret_status != OK)
    {
	uleFormat(NULL, E_UL0016_CHAR_INIT, (CL_ERR_DESC *)&sys_err, ULE_LOG , 
	    NULL, (char * )0, 0L, (i4 *)NULL, &err_code, 0);

	/*
	** Format exit error message and call exit handler.
	*/
	exit_error = E_DM9851_ACP_INITIALIZE_EXIT;
	uleFormat(NULL, exit_error, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, exit_text, sizeof(exit_text),
	    &exit_msg_len, &err_code, 0); 
	acp_exit_handler(exit_error, exit_text, exit_msg_len, (char *)NULL);
	return(FAIL);
    }

    /*
    ** Create filename. Non cluster is DMFACP.LOG, VAXcluster 
    ** is DMFACP.LOG_NODE, where NODE is LGC_MAX_NODE_NAME_LEN in length
    ** and we add 2
    ** more characters for the "_" and '\0'.
    */

#define ACP_LOGFILE_NAME "iiacp.log"
#define ACP_FILENAME_LEN ((sizeof(ACP_LOGFILE_NAME))-1)

    {
	char                filename[ACP_FILENAME_LEN+2+LGC_MAX_NODE_NAME_LEN];
	char                *qualname;
	i4                 l_qualname;
	char                node_name[CX_MAX_NODE_NAME_LEN];
	LOCATION            loc;

	MEmove(ACP_FILENAME_LEN, ACP_LOGFILE_NAME, ' ',
		sizeof(filename), filename);
	filename[ACP_FILENAME_LEN] = 0;
	if (CXcluster_enabled())
	{
	    (void)CXnode_name(node_name);
	    CXdecorate_file_name(filename, node_name);
	}    
	else
	    GChostname(node_name, sizeof(node_name));

	ule_initiate(node_name, STlength(node_name), "II_ACP", 6);

	qualname = filename;
        if( (status = NMloc(LOG, FILENAME, qualname, &loc)) != OK)
        {
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			&err_code, 0);
        }
        else
        {
            LOtos(&loc, &qualname);
            l_qualname = STlength(qualname);
        }

        if (status == OK)
        {
	    i4     flag;

	    /*
	    ** ACP appends to the end of existing log file on UNIX, allows
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
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			&err_code, 0);
	    }
        }

        if (status != OK)
        {
	    /*
	    ** We were unable to open iiacp.log, either because the NM symbols
	    ** have not been correctly set, or because the TR facility isn't
	    ** working. Since something is SERIOUSLY wrong, we should not go on.
	    ** Dying with only a message in the error log is not very
	    ** informative, and may be even worse if ule_format was unable to
	    ** issue any message (because, e.g., the errlog.log file could
	    ** not be written to). Make a last ditch effort to display some
	    ** sort of useful message to the terminal, if possible, before
	    ** exiting.
	    */
	    if ((status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
	    			&sys_err)) == OK)
	    {
		TRdisplay("Error building path/filename for II_ACP.LOG\n");
	        TRdisplay("Please check installation and environment, and\n");
	        TRdisplay("examine the ERRLOG file for additional messages\n");
	    }

	    /*
	    ** Format exit error message and call exit handler.
	    */
	    exit_error = E_DM9851_ACP_INITIALIZE_EXIT;
	    uleFormat(NULL, exit_error, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, exit_text, sizeof(exit_text),
		&exit_msg_len, &err_code, 0);
	    acp_exit_handler(exit_error, exit_text, exit_msg_len, (char *)NULL);
	    return ( FAIL );
        }

    }

    /*	Log a startup message. */

    TRdisplay(" \n%@ ACP STARTUP.\n");
    uleFormat( NULL, E_SC0129_SERVER_UP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &err_code, 2, STlength(Version),
		(PTR)Version, STlength(SystemProductName), SystemProductName );

    /*
    ** Initialize DMFACP to be a single user server.
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
	uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, &err_code, 0);
	return (status);
    }

    ERinit(0, (STATUS (*)()) NULL, (STATUS (*)()) NULL,
			(STATUS (*)()) NULL, (VOID (*)()) NULL);

    /*	Prepare archiver for normal operation. */

    /* Initialize "tmmo" interface to allow gathering of dbms server
    ** performance statistics through MO.
    */
    dmf_tmmo_attach_tmperf();

    status = LKinitialize(&sys_err, ERx("dmfacp"));
    if (status != OK)
    {
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	(VOID) uleFormat(NULL, E_DM901E_BAD_LOCK_INIT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }
    else
    {
	status = LGinitialize(&sys_err, ERx("dmfacp"));

	if (status)
	{
	    (VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM9011_BAD_LOG_INIT, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
    }

    if (status)
    {
	TRdisplay("%@ Error Starting ACP Can't initialize logging.\n");

#ifdef UNIX
	TRdisplay("\tOnly the ingres super-user may run this program, \n");
	TRdisplay("\tany other user running this program will get. \n");
	TRdisplay("\tthis error.\n");
	TRdisplay("\tThis error may also occur if this program is run \n");
	TRdisplay("\tprior to the creation of the system shared \n");
	TRdisplay("\tmemory segments (created by either the iibuild\n");
	TRdisplay("\tprogram or the csinstall utility).\n");
#endif


	/*
	** Format exit error message and call exit handler.
	*/
	exit_error = E_DM9851_ACP_INITIALIZE_EXIT;
	uleFormat(NULL, exit_error, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, exit_text, sizeof(exit_text), &exit_msg_len,
	    &err_code, 0); 
	acp_exit_handler(exit_error, exit_text, exit_msg_len, (char *)NULL);
	return(FAIL);
    }

    /*
    ** Allocate and initialize the acb, init the  Archiver database.
    */
    status = dma_prepare(DMA_ARCHIVER, &acb, (DMP_LCTX *)0, &dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	TRdisplay("%@ Error Starting ACP -- Can't prepare Archiver.\n");
	exit_error = E_DM9851_ACP_INITIALIZE_EXIT;
	uleFormat(NULL, exit_error, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
	    (DB_SQLSTATE *)NULL, exit_text, sizeof(exit_text), &exit_msg_len,
	    &err_code, 0); 
	acp_exit_handler(exit_error, exit_text, exit_msg_len, (char *)NULL);
	return(FAIL);
    }

    a = acb;

    /*	
    ** Perform the mainloop of the archiver until error or shutdown.
    */
    for (;;)
    {
	i4		event;

	CLRDBERR(&a->acb_dberr);
	CLRDBERR(&dberr);

	/*	Wait for any archiver event. */

	status = LGevent(0, a->acb_s_lxid,
	    LG_E_ARCHIVE | LG_E_PURGEDB | LG_E_ACP_SHUTDOWN | LG_E_IMM_SHUTDOWN,
	    &event, &sys_err);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1, 0,
		(LG_E_ARCHIVE | LG_E_PURGEDB | LG_E_ACP_SHUTDOWN |  
		LG_E_IMM_SHUTDOWN));

	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9855_ACP_LG_ACCESS_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &err_code, 0);
	    SETDBERR(&dberr, 0, E_DM9815_ARCH_SHUTDOWN);
	    break;
	}

	/* XXX to change XXX */
	TRdisplay("%78*=\n");
	TRdisplay("%@ ACP: Event <%v>\n",
	    "ONLINE,BCP,LOG_FULL,,RECOVER,ARCHIVE,ACP_SHUTDOWN,IMM_SHUTDOWN,,\
PURGEDB,,,,,,,ECPDONE", event);

	if ( (event & LG_E_ARCHIVE) ||
	     (event & LG_E_PURGEDB) )       
	{
	    /*
	    ** Build archive context by requesting information from 
	    ** the logging system.
	    */
	    status = dma_online_context(a);
	    if (status != E_DB_OK)
	    {
		TRdisplay("%@ ACP DMA_ARCHIVE: Archive Cycle Failed!\n");
		break;
	    }

	    /*
	    ** Get config file information for each database, open journal 
	    ** and dump files, skip over CP and position just past it.
	    */
	    status = dma_soc(a);
	    if (status != E_DB_OK)
	    {
		TRdisplay("%@ ACP DMA_ARCHIVE: Archive Cycle Failed!\n");
		break;
	    }

	    /*
	    ** Copy the log to the journal and dump files.
	    */
	    status = dma_archive(a);
	    if (status != E_DB_OK)
	    {
		TRdisplay("%@ ACP DMA_ARCHIVE: Archive Cycle Failed!\n");
		break;
	    }

	    /*
	    ** Clean up at end of cycle.
	    */
	    status = dma_eoc(a);
	    if (status != E_DB_OK)
	    {
		TRdisplay("%@ ACP DMA_ARCHIVE: Archive Cycle Failed!\n");
		break;
	    }

	    TRdisplay("%@ ACP: Archive Cycle Completed Successfully.\n");
	    continue;
	}

	/*
	** Check for the SHUTDOWN event.
	*/
	if ((event & LG_E_ACP_SHUTDOWN) ||
	    (event & LG_E_IMM_SHUTDOWN))
	{
	    /*
	    ** Fill in exit error information for archiver exit handler.
	    */
	    a->acb_error_code = E_DM9850_ACP_NORMAL_EXIT;
	    uleFormat(NULL, a->acb_error_code, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
		(DB_SQLSTATE *)NULL, a->acb_errtext, sizeof(a->acb_errtext), 
		&a->acb_errltext, &err_code, 0);

	    SETDBERR(&dberr, 0, E_DM9815_ARCH_SHUTDOWN);
	    break;
	}
    }

    /*
    ** Archiver is shutting down.
    **
    ** Save the exit information in local storage for the exit handler as
    ** the archiver control block will be deallocated before the exit
    ** handler is called.
    */
    exit_error = a->acb_error_code;
    exit_msg_len = a->acb_errltext;
    MEcopy((PTR) a->acb_errtext, sizeof(exit_text), (PTR) exit_text);
    STncpy( exit_db, a->acb_errdb.db_db_name, DB_MAXNAME);
    exit_db[ DB_MAXNAME ] = '\0';
    STtrmwhite(exit_db);

    if (dberr.err_code != E_DM9815_ARCH_SHUTDOWN)
	acp_restart = 1;

    if (status != E_DB_OK)
    {
	/*
	** If status is not E_DB_OK, then the archiver is coming down
	** unhappily. acb->acb_dberr contains the reason for the unsuccessful
	** termination, so log that message, then log the archiver
	** statistics.
	*/
	uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);

	dmd_log_info(DMLG_STATISTICS | DMLG_HEADER | DMLG_PROCESSES |
		     DMLG_DATABASES | DMLG_TRANSACTIONS);
    }

    /*	Complete archive processing. */

    status = dma_complete(&a, &dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);

	TRdisplay("%@ Error terminating ACP -- Can't complete Archiver.\n");
    }

    /*	Log a shutdown message. */

    uleFormat(NULL, E_DM9815_ARCH_SHUTDOWN, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4)0, &err_code, 0);

    if (acp_restart &&
	LGalter(LG_A_ACPOFFLINE, (PTR)&acp_restart, sizeof(acp_restart),
		&sys_err))
    {
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, (char *)NULL, 
	    (i4)0, (i4)0, &err_code, 1, 0, LG_A_ACPOFFLINE);
    }

    /*
    ** Call ACP exit handler to process exit type.
    */
    acp_exit_handler(exit_error, exit_text, exit_msg_len, exit_db);

    return (OK);
}

/*{
** Name: ex_handler	- DMF exception handler.
**
** Description:
**	Exception handler for Achiver.  This handler is declared at ACP
**	startup.  All exceptions which come through here will cause the
**	archiver to shutdown.
**
**	An error message including the exception type and PC will be written
**	to the log file before exiting.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	EXDECLARE causes us to return execution to dmfacp routine.
**
** History:
**	 4-feb-1991 (rogerk)
**	    Copied exception handler from DMFCALL.C
**	 16-aug-1991 (rog)
**	    Set buffer to be EX_MAX_SYS_REP+15 instead of 256.
**	26-apr-1993 (bryanp)
**	    Following Lint suggestion, remove unused variable 'ast_status'.
**	07-jul-1993 (rog)
**	    General exception handler cleanup; call ulx_exception to handle
**	    reporting.
**	03-Oct-2005 (hanje04)
**	    BUG 114921
**	    For now when we receive a SIGHUP, just log it and carry on. Under
**	    certain (as yet unknow) conditions on Linux, the ACP receives
**	    a SIGHUP from the STDIN FD it is polling on.
**	    This fix maybe removed once the cause of the hangup is found.
**	27-Oct-2005 (hanje04)
**	    BUG 114921
**	    Can't reference SIGHUP in generic code, check for EXHANGUP instead.
[@history_template@]...
*/
static STATUS
ex_handler(
EX_ARGS	    *ex_args)
{
    i4	    err_code;

    /* We don't want to die if STDIN/OUT etc. get hung up to just
    ** log it and carry on */
    if (ex_args->exarg_num == EXHANGUP )
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
	ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );
	return(EXCONTINUES);
    }
    else if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    ** Note: the fourth argument to ulx_exception must be FALSE so
    ** that it does not try to call scs_avformat() since there is no
    ** session in the ACP.
    */
    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    /*
    ** Returning with EXDECLARE will cause execution to return to the
    ** top of the DMFACP routine.
    */
    return (EXDECLARE);
}

/*{
** Name: acp_exit_handler	- Archiver exit handler.
**
** Description:
**	Exit handler for Achiver.  This routine is called just prior to
**	the archiver exiting.
**
**	This routine is responsible for two functions:
**
**	    - Logging the archiver termination message.
**	    - Exectuting the archiver termination command script.
**
**	Logging the archiver termination message which indicates the reason
**	for the shutdown along with any required/recommended user actions 
**	should the termination not be part of normal shutdown procedures.
**
**	This termination message is passed in by the caller of the exit 
**	handler.  It is formatted by the caller rather than by this routine 
**	to allow error mesage parameters which can give detailed error 
**	information.
**
**
**	The archiver termination command script is a user-modifiable script
**	in the Ingres FILES directory which is run whenever the archiver
**	exits.  It allows site-specific actions to be performed automatically
**	to resolve journaling problems.  
**
**	The name of the termination command script is "acpexit".  It expects
**	one or two arguments:
**
**	    acpexit  error_code  [database_name]
**
**		error_code	- Archiver exit error code.
**		database_name	- Name of database on which error occurred.
**				  This argument is not given if the error
**				  was not specific to a particular database.
**
**	The termination command script is executed via PCcmdline.  On most
**	systems this will cause creation of a subprocess to execute the
**	command script.
**
**	The Archiver Log File (II_ACP.LOG) will be closed by this routine
**	prior to executing the command script to allow the script itself
**	to access the log file.
**
**	In most cases of archiver shutdown, the archiver will have disconnected
**	itself from the Ingres Logging System prior to executing the archiver
**	shutdown procedure.  This allows the command script subprocess to issue
**	the IIarchive command to restart the archiver process (hopefully after
**	addressing the problem which had caused this process' termination)
**	or to use other Ingres command utilities.  Note however, that if
**	errors are encountered during shutdown, the archiver may fail to
**	disconnect itself from the Logging System and any automatic request
**	to restart the ACP from the command script will probably fail.
**
**	Errors encountered during execution of the acp exit handler will
**	be logged to the Ingres Error Log (errlog.log).  Since the Archiver
**	Log File is closed by this routine prior to executing PCcmndline,
**	if that request fails, then the error will not be written to the
**	ACP log (but will still be logged to the errlog.log).
**
** Inputs:
**	exit_code			Error code indicating the reason for
**					archiver termination.
**	exit_message			Formatted error message which describes
**					termination reason and appropriate
**					user actions required.  This message's
**					error number should match the exit_code
**					parameter.
**	exit_msg_len			Length of exit_message.
**	dbname				Name of database which was being
**					processed when archiver error occured
**					which has caused the termination.  May
**					be NULL or an empty string if the error
**					was not associated with a particular
**					database.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	User defined acpexit command script is executed via PCcmndline.
**	Any side effects of that user defined script.
**
** History:
**	25-feb-1991 (rogerk)
**	    Added as part of Archiver Stability changes.
[@history_template@]...
*/
static VOID
acp_exit_handler(
i4	    exit_code,
char	    *exit_message,
i4	    exit_msg_len,
char	    *dbname)
{
    LOCATION	    loc;
    CL_ERR_DESC	    sys_err;
    CL_ERR_DESC	    *cl_err_desc = 0;
    STATUS	    stat = OK;
    i4		    error = 0;
    i4		    local_error;
    i4		    command_length;
    char	    command_buf[256];
    char	    db_buf[DB_MAXNAME + 1];
    char	    num_buf[16];
    char	    *str;

    /*
    ** Log the already formatted termination message to the error log 
    ** and archiver trace file.  
    */
    uleFormat(NULL, 0, (CL_ERR_DESC *)NULL, ULE_MESSAGE, (DB_SQLSTATE *)NULL,
	    exit_message, exit_msg_len, (i4 *)NULL, &local_error, 0);

    for(;;)
    {
	/*
	** Build location to the command script.
	*/
	stat = NMloc(UTILITY, FILENAME, ACP_EXIT_COMFILE, &loc);
	if (stat != OK)
	{
	    break;
	}

	/*
	** Check for existence of the Archiver Exit Command Script.
	** Don't treat non-existence as an error.
	*/
	stat = LOexist(&loc);
	if (stat != OK)
	{
	    stat = OK;
	    break;
	}

	/*
	** Get full path/filename to command script in order to build
	** PCcmdline string.
	**
	** The command string for PCcmdline is an OS specific string
	** which includes the command to run and all arguments.
	*/
	LOtos(&loc, &str);

	/*
	** Get arguments for command line.
	*/
	CVla(exit_code, num_buf);
	MEfill(sizeof(db_buf), '\0', (PTR) db_buf);
	if (dbname)
	{
           STncpy( db_buf, dbname, DB_MAXNAME );
	   db_buf[ DB_MAXNAME ] = '\0';
	}
	

	/*
	** Check that command line buffer is large enough for all the
	** pieces of the command string.
	*/
	command_length = STlength(str);		/* path/filename of script */
	command_length += STlength(num_buf);	/* error number argument */
	command_length += STlength(db_buf);	/* dbname argument */
	command_length += STlength(ACP_PCCOMMAND_PREFIX);  /* command prefix */
	command_length += 2;			/* spaces between args */

	if (command_length >= sizeof(command_buf))
	{
	    TRdisplay("%@ ACP overflow building exit command string:\n");
	    TRdisplay("\n\t%s%s %s %s\n\n", ACP_PCCOMMAND_PREFIX, str, 
		num_buf, db_buf);
	    TRdisplay("\tMaximum size command string: %d characters.\n",
		sizeof(command_buf));

	    error = E_DM983C_ARCH_EXIT_CMD_OVFL;
	    break;
	}

	/*
	** Build command line string to execute.  Includes:
	**
	**	- command string prefix : this is an OS specific string
	**	  which directs the OS command shell to execute the given
	**	  acpexit command script.
	**	- full path/filename to the acpexit command script.
	**	- command script arguments:
	**
	**	    - error number of exit status
	**	    - database name if database error caused exit (optional)
	*/
	STpolycat(6, ACP_PCCOMMAND_PREFIX, str, ERx(" "), num_buf,
					ERx(" "), db_buf, command_buf);

	/*
	** Close the archiver trace file before executing the command
	** script.  This will allow the command script to read the
	** archiver error log.
	**
	** The error return from TRset_file is ignored since there may
	** not actually be a trace file open.
	*/
        (VOID) TRset_file(TR_F_CLOSE, (char *)NULL, 0, &sys_err);

	/*
	** Execute command line.
	*/
	stat = PCcmdline((LOCATION *) NULL, command_buf, (i4) 0,
			    (LOCATION *) NULL, &sys_err);
	if (stat != OK)
	{
	    /*
	    ** If error, set cl_err_desc pointer so error formatted below 
	    ** will include the cl error.  
	    */
	    cl_err_desc = &sys_err;
	    break;
	}
	
	break;
    }

    /*
    ** Check for errors in exit handler processing.
    ** Error status may be incoded in 'stat' if the error came from a CL 
    ** routine; or in 'error' if the error condition was encountered locally.
    **
    ** Note that if the ACP error log has already been closed above then
    ** these ule_format calls will not write the error message there.
    */
    if (stat || error)
    {
	if (stat)
	{
	    uleFormat(NULL, stat, cl_err_desc, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}

	if (error)
	{
	    uleFormat(NULL, error, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}

	uleFormat(NULL, E_DM983B_ARCH_EXIT_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_error,
	    0);
    }
}

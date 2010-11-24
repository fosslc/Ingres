/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <st.h>
#include    <nm.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <me.h>
#include    <cm.h>
#include    <ep.h>
#include    <er.h>
#include    <lo.h>
#include    <si.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0s.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>

/**
** 
**  Name: LOCKSTAT.C - dumps locking system statistics to 'stdout'
**
**	USAGE:
**	    lockstat
**		displays the entire locking system status to stdout
**	    lockstat -summary
**		display just the locking system configuration summary
**	    lockstat -statistics
**		displays the locking system configuration summary as well as
**		    the locking system statistics.
**	    lockstat -lists
**		displays locks by lock list, for all lock lists.
**	    lockstat -user_lists
**		displays locks by lock list, for transaction lock lists only.
**	    lockstat -special_lists
**		displays locks by lock lists, for non-transaction lock lists
**		    only.
**	    lockstat -resources
**		displays locks by resource.
**
**  Description:
**    This file contains:
**	main		the main entry point to this program
**	lockstat	cover for call to log_statistics
**
**  History:
**      22-aug-1986 (Derek)
**	    Created for Jupiter.
**	 2-AUG-1988 (Edhsu)
**	    Added code for fast commit, fix BCNT, and END Trans count bugs
**	05-oct-1988 (greg)
**	    enhance error message.  Remove ifdefs.
**	05-jan-1989 (mmm)
**	    bug #4405.  We must call CSinitiate(0, 0, 0) from any program 
**	    which will connect to the logging system or locking system.
**	    And added to the LKinitialize() TRdisplay's.
**	27-mar-1989 (rogerk)
**	    Added new lock list status values for Shared Buffer Managaer.
**	    Added new lock types for shared buffer manager.
**	25-Apr-1989 (fred)
**	    Added new lock types for user defined abstract datatypes.
**	25-May-1989 (ac)
**	    Added various new lock types and new states.
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**	21-feb-1990 (sandyh)
**	    Added quota section to lock_statictics().
**	13-may-1991 (bonobo)
**	    Added the PARTIAL_LD mode ming hint.
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**	25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**	    Change read-in character set support to use II_CHARSET symbol.
**	25-sep-1991 (mikem) integrated following change: 19-aug-1991 (ralph)
**	    Add support for LK_EVCONNECT
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	25-aug-1992 (ananth)
**	    Get rid of HP compiler warnings.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	09-oct-1992 (jnash)
**	    Reduced logging project.
**	    -   Add LK_ROW lock type.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	14-dec-1992 (jnash)
**	    Add AUDIT lock type in TRdisplay.
**	10-dec-1992 (robf)
**	    Handle AUDIT locks, also ROW locks now correct. 
**	26-apr-1993 (bryanp)
**	    Respond to compiler complaints following prototyping of LK.
**	24-may-1993 (bryanp)
**	    Fixed display of llb's because cluster and non-cluster LLB's now
**		both have the same llb_status definitions.
**	    It's often nice to be able to get just part of a lockstat display,
**		so I added support for "lockstat -summary",
**		"lockstat -statistics", "lockstat -lists",
**		"lockstat -resources", "lockstat -all". Good
**		old "lockstat" means "lockstat -all".
**	    Added llb_pid field to the lock list information structure so that
**		the lock list's creator PID could be displayed. Displayed the
**		PID in hex for VMS, decimal for all other systems.
**	    Added lots of new locking system statistics.
**	21-jun-1993 (bryanp)
**	    Display ALL the lkb_attribute values.
**	    Added "-user_lists" and "-special_lists"
**	12-jul-93 (robf)
**	    Improve formatting for AUDIT locks, now  shows  what they are
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    If a lock list is in LLB_INFO_EWAIT state, display event info.
**	    Added includes too many other files in order to be able to add
**		includes of dm0p.h and dm0s.h to get event info codes.
**	26-jul-1993 (jnash)
**	    Add -help support.
**	22-nov-1993 (jnash)
**	    Move lock_statistics() and format_event_wait_info() into 
**	    dmdlock.c so that it can be used elsewhere. Rename 
**	    lock_statistics() to dmd_lock_info().
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so dump them.
**	20-aug-96 (nick)
**	    Fix 78074 & 78077 - trivial usage message bugs.
**	02-apr-1997 (hanch04)
**	    move main to front of line for mkmingnt
**	24-Apr-1997 (jenjo02)
**	    Added -dirty option which tells LKshow() to skip taking lock resource
**	    semaphores. This is useful while in a debug server and stopped
**	    while a lock resource mutex is held and you just gotta have a
**	    lockstat. This is NOT intended to be an externally documented option!
**	02-feb-1998 (somsa01)  X-integration of 433253 from oping12 (somsa01)
**	    Added exception handler for stand-alone (non-iimerge) version.
**	    (Bug #87480)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in lockstat() routine.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
**	06-feb-2008 (joea)
**	    Add -dlm and -dlmall options.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Use new form of uleFormat.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**	02-Nov-2010 (jonj) SIR 124685
**	    Return int instead of VOID to agree with function prototype.
**/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	lockstat_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*
**	mkming hints
MODE =		SETUID PARTIAL_LD

NEEDLIBS =	DMFLIB ADFLIB ULFLIB GCFLIB SCFLIB DBUTILLIB GWFLIB COMPATLIB MALLOCLIB

OWNER =		INGUSER

PROGRAM =	lockstat
*/

/*
[@forward_type_references@]
*/

/*
**  Forward and/or External function references.
*/

static	STATUS	lockstat(i4 argc, char *argv[]);
static  VOID 	lockstat_usage(void);
# ifndef II_DMF_MERGE
static STATUS ex_handler(EX_ARGS *ex_args);
# endif

/*
[@function_reference@]...
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definition@]...
[@function_definition@]...
*/


/*{
**
** Name: main	- DUMP lock statitics to "stdout"
**
** Description:
**	This program, which may require privilege to run,
**	dumps the locking system statistics to "stdout".
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
**      25-sep-1991 (mikem) integrated following change: 10-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	25-sep-1991 (mikem) integrated following change: 13-jun-1991 (bryanp)
**	    Use TR_OUTPUT and TR_INPUT in TRset_file() calls.
**      25-sep-1991 (mikem) integrated following change: 12-aug-1991 (stevet)
**          Change read-in character set support to use II_CHARSET symbol.
**	5-nov-1992 (bryanp)
**	    Check CSinitiate return code.
**	26-jul-1993 (jnash)
**	    Added -help.  
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so dump them.
**	24-Apr-1997 (jenjo02)
**	    Added -dirty option which tells LKshow() to skip taking lock resource
**	    semaphores. This is useful while in a debug server and stopped
**	    while a lock resource mutex is held and you just gotta have a
**	    lockstat. This is NOT intended to be an externally documented option!
**	02-feb-1998 (somsa01)  X-integration of 433253 from oping12 (somsa01)
**	    Added exception handler for stand-alone (non-iimerge) version.
**	    (Bug #87480)
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	14-Jun-2004 (schka24)
**	    Replace charset handling with (safe) canned function.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      06-Aug-2009 (wanfr01)
**          Bug 122418 - Print OS generic LKinitialize error
**	04-Nov-2010 (bonro01) SIR 124685
**	    Fix windows syntax error introduced by previous change.
*/

# ifdef	II_DMF_MERGE
int MAIN(argc, argv)
# else
int main(argc, argv)
# endif
i4	argc;
char	*argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(lockstat(argc, argv));

    /* NOTREACHED */
    return(FAIL);
}

static STATUS
lockstat(i4 argc, char *argv[])
{
    CL_ERR_DESC		sys_err;
    CL_ERR_DESC         cl_err;
    STATUS		status = FAIL;
    STATUS              ret_status;
    i4			options = 0;
    i4		i;
    bool		syntax_error;
# ifndef II_DMF_MERGE
    EX_CONTEXT		context;
# endif

    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);
    if (status)
	return(FAIL);
# ifndef II_DMF_MERGE
    if (EXdeclare(ex_handler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }
# endif
    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if (argc == 1)
    	options = DMTR_LOCK_ALL;

    syntax_error = FALSE; 
    for (i = 1; i < argc; i++)
    {
	if (MEcmp("-summary", argv[i], sizeof("-summary")-1) == 0)
	    options |= DMTR_LOCK_SUMMARY;
	else if (MEcmp("-statistics", argv[i], sizeof("-statistics")-1) == 0)
	    options |= DMTR_LOCK_STATISTICS;
	else if (MEcmp("-lists", argv[i], sizeof("-lists")-1) == 0)
	    options |= DMTR_LOCK_LISTS;
	else if (MEcmp("-user_lists", argv[i], sizeof("-user_lists")-1) == 0)
	    options |= DMTR_LOCK_USER_LISTS;
	else if (MEcmp("-special_lists", argv[i],
					    sizeof("-special_lists")-1) == 0)
	    options |= DMTR_LOCK_SPECIAL_LISTS;
	else if (MEcmp("-resources", argv[i], sizeof("-resources")-1) == 0)
	    options |= DMTR_LOCK_RESOURCES;
	else if (MEcmp("-help", argv[i], sizeof("-help")-1) == 0)
	    options |= DMTR_LOCK_HELP;
	else if (MEcmp("-dirty", argv[i], sizeof("-dirty")-1) == 0)
	{
	    options |= DMTR_LOCK_DIRTY;
	    /* If -dirty is the only arg, also set DMTR_LOCK_ALL default */
	    if (argc == 2)
		options |= DMTR_LOCK_ALL;
	}
	else if (MEcmp("-dlmall", argv[i], sizeof("-dlmall")-1) == 0)
	    options |= DMTR_LOCK_DLM_ALL;
	else if (MEcmp("-dlm", argv[i], sizeof("-dlm")-1) == 0)
	    options |= DMTR_LOCK_DLM;
	else
	{
	    options |= DMTR_LOCK_HELP;
	    syntax_error = TRUE;
	}
    }

    if (options & DMTR_LOCK_HELP)
    {
	lockstat_usage();
	if (syntax_error)
	    return(FAIL);
	else
	    return(OK);
    }

    /*
    ** bug #4405
    ** on unix logstat acts as a "pseudo-server" in that it must connect to
    ** the "event system" when it uses LG.  The CSinitiate call is necessary
    ** to initialize this, and to set up a cleanup routine which will free
    ** a slot in the system shared memory segment when the program exits.
    */

    status = CSinitiate((i4 *)0, (char ***)0, (CS_CB *)0);
    if (status)
    {
	char		error_buffer[ER_MAX_LEN];
	i4		error_length;
	i4		count;
	i4	    	err_code;

	uleFormat(NULL, status, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	    &err_code, 0);
	error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);
	return (status);
    }

    if (status = LKinitialize(&sys_err, ERx("lockstat")))
    {
	TRdisplay("LOCKSTAT: call to LKinitialize failed\n");
	TRdisplay("\tPlease check your installation and permissions\n");

	TRdisplay("\tThis error will occur if this program is run prior to\n");
	TRdisplay("\tthe creation of the system shared memory segments\n");
    }
    else
    {
	/* Set the proper character set for CM */

	ret_status = CMset_charset(&cl_err);
	if (ret_status != OK)
	{
	    TRdisplay("\n Error while processing character set attribute file.\n");
	    return(FAIL);
	}

	/*      Write locking statistics. */

# ifndef II_DMF_MERGE
	EXinterrupt(EX_DELIVER);
# endif

	dmd_lock_info(options);

    }

    return (status);
}

/*
** Name: lockstat_usage   - Echo lockstat usage format.
**
** Description:
**	 Echo lockstat usage format.
**
** History:
**	26-jul-1993 (jnash)
**	    Created.
**	20-aug-96 (nick)
**	    Missing [-help] from usage. #78074  
**	27-nov-07 (kibro01) b119276
**	    -statistics and -dirty were missing from usage
*/
static VOID
lockstat_usage(void)
{

    TRdisplay(
	"usage: lockstat [-summary] [-lists] [-user_lists] [-statistics]\n");

    TRdisplay(
	"\t[-special_lists] [-resources] [-dirty] [-help]\n");
#if defined(conf_CLUSTER_BUILD)
    TRdisplay("\t[-dlm[all]]\n");
#endif

    return;
}

# ifndef II_DMF_MERGE
static STATUS
ex_handler(EX_ARGS *ex_args)
{
    return (EXDECLARE);
}
# endif

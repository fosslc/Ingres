/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <lo.h>
#include <er.h>
#include <pc.h>
#include <tc.h>
#include <te.h>
#include <termdr.h>
#include <me.h>

#define procmmd_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <septxt.h>
#include <seperrs.h>
#include <termdrvr.h>
#include <fndefs.h>

#ifdef UNIX
#include <sys/wait.h>
#endif
#ifdef NT_GENERIC
#include <Tlhelp32.h>
#endif

#include <ex.h>

/*
** History:
**	16-aug-1989 (boba)
**		Use global, not local, tc.h.  Change path for PEDITOR.
**	17-aug-1989 (eduardo)
**		Don't allow SEP to hang if TM or FE dies after a successful
**		TCopen. Also, generate correct temporary file names.
**	23-aug-1989 (mca)
**		Check the PEDITOR environment variable to find the pathname
**		of the pseudo-editor.
**	29-Aug-1989 (eduardo)
**		Added additional parameter to getLocation call (type of
**	        location being parsed)
**	30-aug-1989 (mca)
**		Modified the algorithm for refreshing the screen during
**		output of results. Per-line refreshing was too slow, so now
**		the screen is refreshed after every screenful.
**	07-Sep-1989 (eduardo)
**		Modified TCgetc calls so they use waitLimit parameter
**		instead of hard coded 180
**	07-Sep-1989 (eduardo)
**		Fixed bug the made SEP decreases testLevel even though
**		it was processing alternate canons
**	15-sep-1989 (mca)
**		Added an argument to SEPspawn call so SEP and Listexec can
**		use the same SEPspawn().
**	25-sep-1989 (mca)
**		Allow output from OS/TCL commands to stderr to be caught on
**		Unix.
**	6-Nov-1989 (Eduardo)
**		Add performance improvement through SEPFILES
**	7-Nov-1989 (Eduardo)
**		Added exception handling
**	4-dec-1989 (Eduardo)
**		Changed declaration of 'thechar' from char to i4  to please
**		porting requirements
**	28-dec-1989 (Eduardo)
**		Added changes to disp_res routine to improve displaying
**		of results in screen
**	11-Jan-1990 (Eduardo)
**		Added support for function keys
**	05-apr-1990 (fredv)(rudyw integrates from ug)
**		On some systems (IBM RT is one example), EOF char is treated as
**		an unsigned one byte integer; thus, it will never be true
**		when compare that to TC_EOF (255 != -1). Use the I1_CHECK
**		macro to do the trick.
**	05-apr-1990 (fredv)(rudyw integrates from ug)
**		As daveb pointed out, TCgetc() returns nat; therefore, thechar
**		and lastchar should be i4  too.
**	05-apr-1990 (fredv)(rudyw integrates from ug)
**		Since TC_TIMEDOUT and TE_TIMEDOUT are defined as (i4) -2, we 
**		must use I1_CHECK_MACRO to convert thechar to correct value in 
**		comparision.
**	24-apr-1990 (bls)
**		Integrated Sep 3.0 code line and NSE (porting) code line.
**	11-jun-1990 (rog)
**		Make sure we use I1_CHECK_MACRO and not just I1_CHECK.
**	18-jun-1990 (rog)
**		Standardized the usage of SEPspawn().
**	27-jun-1990 (rog)
**		Kill spawned VMS process so that we can delete files.
**	09-jul-1990 (rog)
**		Delete COMM files if SEPspawn() fails.
**	13-jul-1990 (rog)
**		Delete I1_CHECK_MACRO and do it right.
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	03-aug-1990 (rog)
**		Improve support for continued lines.
**	08-aug-1990 (rog)
**		More continued line support.
**	10-aug-1990 (rog)
**		Add in_canon to handle '-' in canons correctly.
**	27-aug-90 (rog)
**		Added SEP_ESCAPE to handle SEPENDFILE correctly.
**	29-aug-90 (rog)
**		Fix a field-width parameter in an STprintf() call.
**	25-jun-91 (donj)
**		Add process_if function to handle conditional execution.
**	19-jul-1991 (donj)
**		Changed some typing and pointer inconsistencies in process_if.
**		This feature is not activated yet.
**	23-jul-1991 (donj)
**		Added the handling of environmental variables, EDITOR and
**		VISUAL in similar manner as ING_EDIT for UNIX. These changes
**		were neccessary to work with the new UNIX UTedit function.
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Removed
**	    smatterings of code that had been commented out long ago. Made
**	    the pidstring var a global SEPpidstr. It was getting carried just
**	    about everywhere thru argument lists. It seemed a fine candidate
**	    for a global var. Condensed building blocks for error msgs.
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Centralized the code used for translating "SEPPARAM" environmental
**	    variables and "@FILE(" contructions into SEPPARM.C and GETLOC.C.
**	    Centralized the code for creating canons into create_canon() in
**	    this file. Centralized the code for opening the TC-COMM files.
**	    Added a sep_state variable that's set inside of get_command. It
**	    carries info like "I'm inside of a comment" "I'm inside of a canon",
**	    etc. Finished code for conditional execution of SEP commands.
**	    modules process_if(), Eval_If() added. Added SEPerr_'s to many of
**	    the error traps.
**	27-jan-1992 (donj)
**	    Add tracing support to do sepparam debugging.
**      28-jan-1992 (jefff)
**          Removed 'register' type declaration for numFiles in process_fe,
**          since the & operator was needed on the variable.
**	28-jan-1992 (donj)
**	    Add Changed_Tokens and related code to fix sepparam substitution
**	    inside of process_tm.
**      28-jan-1992 (donj)
**          Fixed UNIX problem in Trans_Tokens chg_tokens was mistyped, was
**	    a i4  should be a bool like Change_Tokens. Also added more trace
**	    lines.
**	29-jan-1992 (donj)
**	    Try fixing bug #42144, leaving files, in_*.sep and out_*.sep behind
**	    on UNIX. (maybe VMS too in certain circumstances).
**	31-jan-1992 (donj)
**	    Fixed bug caused by Changed_Tokens fix above; When there wasn't a
**	    sepparam sep was writing out the wrong buffer.
**	19-feb-1992 (donj)
**	    Fixed a UNIX problem with tokptr[], can't initialize it as an
**	    automatic variable.
**	24-feb-1992 (donj)
**	    Build a tracing for dialog between SEP and it's subprocess, SEPSON.
**	02-apr-1992 (donj)
**	    Removed References not used. Split comments handling away to a
**	    new file, COMMENTS.C. Added code to scan and parse possible options
**	    on the "<<" and ">>" lines. So far all we have is a "-platform"
**	    switch for the open canon line ("<<"). Changed all refs to '\0'
**	    to EOS. Changed more char pointer increments (c++) to CM calls
**	    (CMnext(c)). Changed a "while (*c)" to the standard
**          "while (*c != EOS)". Implement other CM routines as per CM
**	    documentation. Add capability to handle blank lines in SEP tests.
**	    Fix problem with comments inside of TM/FE commands. Implement
**	    skipping canons if they're labelled and don't match the current
**	    platform. Also implement printing all results if verboseLevel is
**	    set high enough.
**	06-apr-1992 (donj)
**	    Added to the Eval_if() function ability to check OS and platform
**	    types. ".if VMS" or ".if UNIX" or ".if vax.vms", etc. work now
**	    in the ".if,.else,.endif" control structure.
**	15-apr-1992 (donj)
**	    Added command array for close_args. Put in code to look for
**	    the keyword ABORT on close_canon line and set abort_if_matched
**	    if found. Had diffing() return new STATUS constant, ABORT_TEST,
**	    if abort_if_matched is set TRUE.
**	05-jul-1992 (donj)
**	    Added command option array for close cannon line to implement an
**	    ABORT-if-Matched function. This function will abort the SEP test
**	    if the canon matches. Function has not been fully implemented. But
**	    this first part is done. Rewrote how process_fe handles keystroke
**	    data to make it more compatible with the cl CM functions. Added a
**	    expression analyzer to determine to diff or skip a particular
**	    cannon. Added cl ME memory tags to use MEtfree() to make memory
**	    handling simpler. Added process dialog tracing. Rewrote how SEP
**	    relates to SEPSON processes. More explicitly handles TC_EOQ,
**	    TC_BOS and TC_EOF chars. Also SEPSON and SEPSON subprocesses.
**	    Example: SEP calls ABF which runs an ABF image. OR SEP call RBF
**	    which runs a REPORT. Modularize the handling of .if,.then,.else
**	    statements inside of the various process handlers, process_tm and
**	    process_fe. Replaced the older .if,.else expression analyzer and
**	    call the newer, more complete version, SEP_Eval_If(). Modularize
**	    encoding of keystroke sequences.
**	05-jul-1992 (donj)
**	    Fix a "code not reached" warning on sunOS.
**	05-jul-1992 (donj)
**	    Define SEP_CMpackwords for sunOS. Also fix cunfusion of sendEOQ
**	    used as a parameter to Read_TCresults()by changing it to
**	    sendEOQ_ptr.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	30-nov-1992 (donj)
**	    Moved strip out c-coded grammar expression stuff into other
**	    files.  Add code to handle allocated memory better. Add tracing
**	    stuff. Implement ABORT command for end-canon qualifier. Take
**	    trans_tokens() out and put in its own file.
**	08-feb-1992 (donj)
**	    Fix a problem with disconnecting from a forms-system frontend
**	    when running SEP in shell mode. SEPtcsubs was not getting
**	    decremented to less than zero. Changed test to look for zero
**	    instead.
**      31-mar-1993 (donj)
**          Add a new field to OPTION_LIST structure, arg_required. Used
**          for telling SEP_Cmd_Line() to look ahead for a missing argument.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**	    Delete an extraneous #endif.
**	25-may-1993 (donj)
**	    Add some more initializations that QA_C complained about.
**	    Use a common msg[] char array.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h. Took out unused
**          arg from SEP_cmd_line.
**      14-aug-1993 (donj)
**          changed open_canon_opt and close_canon_opt from an array of structs
**	    to a linked list of structs. Since these arrays weren't used yet,
**	    they disappeareds. When needed look at listexec.c,executor.c or
**	    septool.c for examples of how to setup the lists.
**	17-aug-1993 (donj)
**	    process_tm was turning on and off exception handling when the
**	    SEP_TCgetc_trace() does it internally. Also removed old code
**	    that was outputing the dialog to the trace file twice. Moved
**	    two constant char defs (TerMon & Con_with) out of process_tm()
**	    Con_with was losing its address and causing an exception. Added
**	    a few tracing statements to find the exception.
**	26-aug-1993 (donj)
**	    Fix a problem with abort_if_matched feature.
**	 7-sep-1993 (donj)
**	    Modularized much of process_fe (and some of process_tm) looking
**	    for a bug in long keystroke sequences for FRS front-ends. If a
**	    long keystroke sequence is broken up into two or more SEP commands,
**	    and there is no output over the TC channel from the front-end for
**	    one of the pieces, then SEP would hang until it timed out from
**	    its waitLimit.
**	15-sep-1993 (donj)
**	    Fixed an backward compatibility problem. SEP was inserting a newline
**	    when it didn't need to into the results from an FRS frontend. This
**	    may cause some problems with new tests created or updated with SEP
**	    3.3 before this change was made but it will make the old tests more
**	    compatible now.
**	23-sep-1993 (donj)
**	    Fixed an unitialized memory read of lastc in Read_TCresults(). Just
**	    to be tight.
**	 7-oct-1993 (donj)
**	    Fix Read_TCresults() so that, if all we read is a TC_EQR, we'll try
**	    again to get dialog fm the fe.
**	 7-oct-1993 (donj)
**	    SEP wasn't reccognizing Front-End Time outs. Also removed ifdefs's
**	    for PROTOTYPING.
**	12-oct-1993 (donj)
**	    process_tm() and process_fe weren't handling ABORT canons correctly.
**	12-oct-1993 (donj)
**	    found another spot where temporary files could be left behind.
**	12-oct-1993 (donj)
**	    set testGrade with the SEPerr_ABORT_canon CONSTANT.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	19-oct-1993 (donj)
**	    Refine Empty query handling to address VMS problems.
**	 3-nov-1993 (donj)
**	    Added @file() support to process_fe() for keystroke commands
**	    from a test file only. NOT FROM SHELLMODE.
**	 2-dec-1993 (donj)
**	    Took out underscores from filenames for hp9_mpe.
**	 5-dec-1993 (donj)
**	    Parameterized the retry_timeout value to test a VMS bug, 57514.
**	14-dec-1993 (donj)
**	    Add GlobalRef for Status_Value to Implement User Variable, Status.
**	    Status makes return status for SEPspawn accessible to User's SEP
**	    test`s conditional expressions.
**      30-dec-1993 (donj)
**          Added GLOBALREF's for screenLine and paging. They're used by the
**          disp_line() macros. Added buffer_1 to parameter list for
**	    assemble_line() so assemble.c wouldn't have any GLOBALREFS.
**	 4-jan-1994 (donj)
**	    Moved del_file() from procmmd.c to utility.c
**	28-jan-1994 (donj)
**	    modularized process_tm() to make less complex. Trying to fix
**	    56175 with command line delimited objects.
**       4-mar-1994 (donj)
**          Had to change exp_bool from type bool to type i4, exp_bool was
**          exp_bool was carrying negative numbers - a no no on AIX. Changed
**          it's name from exp_bool to exp_result. Also had to change the
**          type of SEP_Eval_If(). This meant that Eval_If had to handle the
**	    type change from i4  to bool.
**	22-mar-94 (vijay)
**	    Kill the FE program if it is waiting for input at the end of the
**	    all input sequences.
**	22-mar-1994 (donj)
**	    Was not handling multiple canons when the first canon had a close
**	    canon abort.
**	24-mar-94 (tomm)
**	    Vijay's change kills the FE with a graceful SIGTERM.  I add 
**	    another level of signals.  If the FE doesn't exit with SIGTERM,
**	    we should kill it with SIGKILL.
**	29-mar-94 (vijay)
**	    wait.h is in usr/include/sys.
**	30-mar-94 (donj)
**	    vijay's above change should've been "ifdef'd" so VMS wouldn't try
**	    to find #include <sys/wait.h>
**	31-mar-94 (donj)
**	    Initialization stuff in diffing() was being missed when ignored
**	    canon encountered, causing problems with abort_if_matched flag.
**	    I moved the init'ing ahead of the form_canon() call.
**	01-apr-94 (donj)
**	    <wait.h> doesn't exist on VMS at all. I took it out.
**	13-apr-1994 (donj)
**	    Fixed disp_res() to be a STATUS return type.
**	14-apr-1994 (donj)
**	    Added testcase support.
**	20-apr-1994 (donj)
**	    Simplified tracing slightly with a new macro simple_trace_if().
**	    Modularized more of process_fe().
**	02-map-94 (vijay)
**	    Add a sleep to get over some sync problems with front ends.
**	10-may-1994 (donj)
**	    Stripped out many functions into separate files, Modularizing over
**	    half of the code.
**	11-may-1994 (donj)
**	    Make sure SEP sends a TC_EOQ in all cases.
**	24-may-1994 (donj)
**	    Slight improvement in readability using "() ? m : n;" contruct.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	08-jun-1994 (donj)
**	    Changed an STprintf() to a STcopy().
**	22-jun-1994 (donj)
**	    Use EndOfQuery() function for all TC_EOQ sending. Add a few more
**	    TCflush()'s.
**	27-mar-1995 (holla02)
**	    Modified disconnectTClink() to check that SEPsonpid != 0 before
**	    trying to kill process.  Bug 66012: if SEPPARAMDB was referenced
**	    but not found defined in the environment, SEP exited ungracefully
**	    at "kill(SEPsonpid, 15)" with a 0 value for SEPsonpid.
**	30-mar-1995 (holla02)
**	    Modified PFE_put_keystrokes() to eliminate frontend looping bug
**	    in shell mode (bug #66092).  Change 413341 (11-may-1994 above)
**	    caused side effects because sending TC_EOQ with S_REFRESH (clearing
**	    canon-handling menu) causes Read_fm_FE_not_recording() to read the
**	    TC_EQR returned and proceed down the wrong path at "if (TCempty(
**	    SEPtcoutptr)) { break; }"  I changed the code so that in the shell
**	    mode/not recording case, no TC_EOQ is written.
**
**	19-oct-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**      05-Feb-96 (fanra01)
**          Added conditional for using variables when linking with DLLs.
**	10-nov-98 (kinte01)
**	    rename Warn to Sep_Warn to avoid multiply defined symbol on VMS
**	    when linking with CAlic 
**	05-Mar-1999 (ahaal01)
**	    Moved "include ex.h" after "include sys/wait.h".  The check for
**	    "ifdef EXCONTINUE" in ex.h does no good if sys/context.h
**	    (via sys/wait.h) is included after ex.h.
**	30-jul-1999 (somsa01)
**	    The last change moved the include of ex.h to within the
**	    "#ifdef UNIX" block. However, other platforms still need this
**	    include as well.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	25-jun-2001 (somsa01)
**	    In Get_Input_from_Script(), if we find that we're in a canon,
**	    skip the canon and try again to read a valid command.
**	16-Jun-2005 (bonro01)
**	    Added SEP_CMD_SLEEP to control sleep between sep commands.
**      17-Feb-2009 (horda03) Bug 121667
**          When running a tool via FERUN, ensure the process is running
**          while waiting for FE output.
**	08-May-2009 (drivi01)
**	    Add space after "connect with " string for better output 
**	    message.
**	15-Oct-2010 (drivi01)
**	    Add some additional wait times for the commands that are 
**	    still alive when sep_cmd sleep time has expired.
**	    It is possible the command is just taking longer to run.
**	    If all fails and we have to terminate the command, make
**	    sure all its children are terminated too and are not 
**	    lingering around and causing hangs or preventing
**	    sep from cleaning up after itself or destroying databases.
**	1-Dec-2010 (kschendel)
**	    Compiler warning fixes.
*/

/*
**  defines
*/

#define TEGET_WAIT         1
#define TC_WAIT	           1
#define TC_NOWAIT          0

/* definitons to handle reading of input from terminal testing FEs */

#define KEY_TDOUT          0
#define KEY_OK             1
#define KEY_FK             2

#ifdef UNIX
#define PEDITOR ERx("peditor")
#endif
#ifdef VMS
#ifdef DEBUG
#define PEDITOR ERx("peditor_dbg")
#else
#define PEDITOR ERx("peditor")
#endif
#endif
#ifdef NT_GENERIC
#define PEDITOR ERx("peditor.exe")
#endif

/*
    Reference global variables
*/

GLOBALREF    bool          batchMode ;
GLOBALREF    bool          shellMode ;
GLOBALREF    bool          in_canon ;
GLOBALREF    bool          SEPdisconnect ;
GLOBALREF    bool          updateMode ;
GLOBALREF    bool          ignore_silent ;
GLOBALREF    bool          abort_if_matched ;

GLOBALREF    SEPFILE      *sepResults ;
GLOBALREF    SEPFILE      *sepCanons ;
GLOBALREF    SEPFILE      *sepGCanons ;
GLOBALREF    SEPFILE      *sepDiffer ;
GLOBALREF    TCFILE       *SEPtcinptr ;
GLOBALREF    TCFILE       *SEPtcoutptr ;
GLOBALREF    FILE         *traceptr ;

GLOBALREF    CMMD_NODE    *sepcmmds ;


GLOBALREF    WINDOW       *mainW ;
GLOBALREF    WINDOW       *statusW ;
GLOBALREF    WINDOW       *coverupW ;
# if defined(NT_GENERIC)  /* This is to enable the switching of SEP and FE screens */
# if defined(IMPORT_DLL_DATA)
GLOBALDLLREF    HANDLE        hTEconsole;
GLOBALDLLREF    SECURITY_ATTRIBUTES  TEsa;
# else              /* IMPORT_DLL_DATA */
GLOBALREF    HANDLE        hTEconsole;
GLOBALREF    SECURITY_ATTRIBUTES  TEsa;
# endif             /* IMPORT_DLL_DATA */
GLOBALDEF    HANDLE        hFEconsole; 
#endif              /* NT_GENERIC */

GLOBALREF    i4            fromLastRec ;
GLOBALREF    i4            waitLimit ;
GLOBALREF    i4            diffLevel ;
GLOBALREF    i4            tracing ;
GLOBALREF    i4            SEPtcsubs ;
GLOBALREF    i4            sep_state ;
GLOBALREF    i4            Status_Value ;
GLOBALREF    i4            if_level[] ;
GLOBALREF    i4            if_ptr ;


GLOBALREF    char          holdBuf [] ;
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char          buffer_2 [] ;
GLOBALREF    char         *lineTokens [] ;
GLOBALREF    char          msg [] ;
GLOBALREF    char          SEPpidstr [] ;
GLOBALREF    char          SEPinname [] ;
GLOBALREF    char          SEPoutname [] ;
GLOBALREF    char         *open_args ;
GLOBALREF    char         *close_args ;
GLOBALREF    char         *open_args_array [] ;
GLOBALREF    char         *close_args_array [] ;
GLOBALREF    char         *ErrC ;
GLOBALREF    char         *ErrOpn ;
GLOBALREF    char         *ErrF ;
GLOBALREF    char         *creloc ;
GLOBALREF    char         *lookfor ;
GLOBALREF    char         *rbanner ;
GLOBALREF    char         *rbannerend ;
GLOBALREF    char         *Con_with ;
GLOBALREF    char          kfebuffer [] ;
GLOBALREF    char         *kfe_ptr ;

GLOBALREF    STATUS        SEPtclink ;

GLOBALREF    PID           SEPsonpid ;

GLOBALREF    LOCATION      SEPtcinloc ;
GLOBALREF    LOCATION      SEPtcoutloc ;

GLOBALREF    FUNCKEYS     *fkList ;

GLOBALREF    OPTION_LIST  *open_canon_opt ;
GLOBALREF    OPTION_LIST  *close_canon_opt ;

GLOBALREF    i4            cmd_sleep;

/* Diffing routine */
FUNC_EXTERN  int           QAdiff ( SEPFILE **canFile, SEPFILE **resFile,
				    SEPFILE *diffFile, i4  max_d ) ;

/* Simplify display of error, resetting interrupt and returning. */

#define goto_top_of_results_if(sw,fp) \
 if (sw) { SEPrewind(fp, (sw = FALSE)); }

#define simple_trace_if(sw,str) \
 if (tracing&sw) { SIputrec(ERx(str),traceptr); SIflush(traceptr); }

static char               *peditor_name = NULL ;

char                      *TerMon     = ERx("Terminal Monitor") ;
char                      *Sep_Warn   = ERx("WARNING!! ") ;
char                      *termnoinp  = ERx(" terminated due to no input") ;
char                      *ErrDef     = ERx("ERROR: Could not define ") ;
char                      *ErrDea     = ERx("ERROR: Could not deassign ") ;
char                      *ErrApp     = ERx("ERROR: failed while appending to diff output file") ;
char                      *crefil     = ERx("create file ") ;
char                      *toRes      = ERx(" to RESULT file") ;
char                      *cbanner    = ERx("====  CANON FILE  ====") ;

bool                       Changed_Tokens ;

/*
**  process_ostcl
**
**  Description:
**  subroutine to execute OS/TCL commands.
**  Parameters,
**  testFile	=>  pointer to test script file
**  command	=>  name of OS/TCL command to be executed
**
*/

STATUS
process_ostcl(FILE *testFile,char *command)
{
    char                  *resfname  = NULL ;
    LOCATION              *resLocptr = NULL ;

    FILE                  *resPtr = NULL ;
    char                   resBuff [SCR_LINE] ;
    i4                     ac ;
    PID                    sonpid ;
    STATUS                 syserr ;

    /* tell user what's going on */
    put_message(statusW, STpolycat(3,ERx("Processing `"),command,
			 ERx("' OS/TCL command..."),msg));

    /* name output file name to be generated by OS/TCL command */
    if ((syserr = Create_stf_file(SEP_ME_TAG_MISC, ERx("r1"), &resfname,
				  &resLocptr, msg)) != OK)
    {
	put_message(statusW, msg);
    }
    /* spawn OS/TCL command and wait for it to be finished */
    else if ((syserr = Trans_Tokens(SEP_ME_TAG_PARAM, lineTokens, NULL)) != OK)
    {
	put_message(statusW, STpolycat(3,ErrC, ERx("Translate Tokens"),msg));
    }
    else
    {
	for (ac = 0; lineTokens[ac+1] ; ac++) {} ;

	if ((syserr = SEPspawn(ac, &lineTokens[1], TC_WAIT, NULL, resLocptr,
				   &sonpid, &Status_Value, 2)) != OK)
	{
	    testGrade = SEPerr_Cant_Spawn;
	    disp_line(STpolycat(2,ErrC,ERx("spawn OS/TCL command"),msg),0,0);
	}
	else
	{
	    MEtfree(SEP_ME_TAG_PARAM);
	    /*
	    **  append results to results file 
	    */
	    SEPrewind(sepResults, FALSE);
	    if ((syserr = SIopen(resLocptr, ERx("r"), &resPtr)) != OK)
	    {
		disp_line(STpolycat(3, ErrOpn, ERx("results for appending"),
				       toRes, msg), 0, 0);
	    }
	    else
	    {
		while ((syserr = SIgetrec(resBuff, SCR_LINE, resPtr)) == OK)
		    SEPputrec(resBuff, sepResults);
		if (syserr != ENDFILE)
		{
		    disp_line(STpolycat(3, ErrC, ERx("append all results"),
					   toRes, msg), 0, 0);
		    syserr = FAIL;
		}
		else
		{
		    syserr = OK;
		    SIclose(resPtr);
		    del_floc(resLocptr);
		    SEPputc(SEPENDFILE, sepResults);
		    SEPrewind(sepResults, TRUE);

		    /*
		    **  create canon for command if creating a test, or
		    **  diff if running a test
		    */
    
		    if (shellMode)
		    {
                	create_canon(sepResults);
		    }
		    else
		    {
                	syserr = diffing(testFile,1,0);
		    }
		}
	    }
	}
    }

    if (resfname != NULL)  MEfree(resfname);
    if (resLocptr != NULL) MEfree(resLocptr);

    return(syserr);
}



/*
**	process_tm
**  
**	Description:
**	subroutine to interact with the Terminal Monitor,
**	TM commands will be read from the test script (or ask to
**	the user if working in shell mode), write to a shared file so
**	they can be read by TM, and wait for TM output. After the TM is
**	finished writing the output, the routine reads the output and
**	diff it against the appropiate canons. This cycle continues 'til
**	no more TM commands are present in the test script.
**	Parameters,
**	testFile    =>	pointer to test script
**	command	    =>	name of TM command to be executed
**
*/

STATUS
process_tm(FILE *testFile,char *command)
{
    STATUS                 PTM_init                 ( char *msg ) ;
    STATUS                 PTM_read_fm_TM           ( i4  *thechar ) ;
    STATUS                 Connection_with_Ingres   ( i4  c, char *cmd,
                                                      STATUS *ierr ) ;
    STATUS                 Still_want_have_connection ( char *cmd,
                                                        STATUS *ioerr ) ;
    STATUS                 PTM_get_next_cmd         ( FILE *testFile ) ;
    STATUS                 PTM_write_cmd_to_TM      () ;

    STATUS                 ioerr   ;
    STATUS                 syserr  = OK ;
    STATUS                 get_out = FALSE ;

    bool                   newRes = TRUE ;

    i4                     thechar ;


    if ((ioerr = PTM_init( msg )) == OK)
    {
	/* *********************** **
	** start processing loop:
	** *********************** **
	*/
	do
	{
	    goto_top_of_results_if( newRes, sepResults );
    
	    /* ***************************************************** **
	    ** Read results from TM and check Connection with TM.
	    ** ***************************************************** **
	    */
	    ioerr = PTM_read_fm_TM( &thechar );
	    if (Connection_with_Ingres(thechar, TerMon, &ioerr) != OK)
	    {
		testGrade = SEPerr_TM_timed_out;
		break;
	    }
    
	    /* ***************************************************** **
	    ** Create canon for command if creating a test, or diff
	    ** if running a test.
	    ** ***************************************************** **
	    */
    
	    if (shellMode)
	    {
		create_canon(sepResults);
	    }
	    else
	    if ((syserr = diffing(testFile,1,0)) != OK)
	    {
		break;
	    }
    
	    newRes = TRUE;      /* collect new set of results */
    
	    /* ***************************************************** **
	    ** Check connection, get next command, translate any tokens
	    ** and send it to the terminal monitor.
	    ** ***************************************************** **
	    */
    
	    if ((get_out = Still_want_have_connection( TerMon, &ioerr )) == OK)
	    {
		if (ioerr == OK)
		{
		    ioerr = PTM_get_next_cmd( testFile );
    
		    if ((sep_state&IN_TM_STATE) == 0)
		    {
			fromLastRec = 1;
			STcopy(buffer_1, holdBuf);
			STpolycat(3,Sep_Warn,ERx("TM"),termnoinp,buffer_1);
		    }
		    else if ((syserr = Trans_Tokens(SEP_ME_TAG_PARAM,
						    lineTokens,
						    &Changed_Tokens)) == OK)
		    {   /*
			**  got a 'legal' TM command, write it
			**  to the shared input file.
			*/
			ioerr = PTM_write_cmd_to_TM();
		    }
		}
	    }
	}
	while (((ioerr == OK)&&(syserr == OK)&&(sep_state & IN_TM_STATE))
	       &&(get_out != TRUE));
    
	MEtfree(SEP_ME_TAG_PARAM);
    }
    disconnectTClink(SEPpidstr);
    return ((syserr != OK) ? syserr : ioerr);
}

STATUS
PTM_init(char *msg)
{
    STATUS                 PTM_create_fake_file     ( char *fprefix ) ;
    STATUS                 PTM_spawn_the_TM         ( char *msg ) ;
    STATUS                 PTM_connect_with_TM      () ;
    STATUS                 ret_val ;

    /* ********************************************************* **
    ** Create fake input file and open the real comm files.
    ** Spawn the TM and do the initial connect handshake.
    ** Get out if there's an error.
    ** ********************************************************* **
    */
    if ((ret_val = PTM_create_fake_file(ERx("tm"))) == OK)
    {
	if ((ret_val = open_io_comm_files((bool)TRUE, SEPONBE)) == OK)
	{
	    if ((ret_val = PTM_spawn_the_TM( msg )) == OK)
	    {
		ret_val = PTM_connect_with_TM();
	    }
	}
    }
    return (ret_val);
}

STATUS
PTM_create_fake_file( char *fprefix )
{
    STATUS                 ioerr ;
    LOCATION              *resLoc  = NULL ;
    FILE                  *resPtr ;
    char                  *resFile = NULL ;

    EXinterrupt(EX_OFF);

    if ((ioerr = Create_stf_file(SEP_ME_TAG_PARAM, fprefix, &resFile, &resLoc,
				 msg)) == OK)
    {
	if ((ioerr = SIopen(resLoc, ERx("w"),&resPtr)) == OK)
	{
            SIclose(resPtr);
	}
    }

    if (ioerr != OK)
    {
	disp_line(STpolycat(3,ErrC,crefil,resFile,msg),0,0);
	testGrade = SEPerr_Cant_Opn_ResFile;
    }

    if (resFile != NULL) MEfree(resFile);
    if (resLoc  != NULL) MEfree(resLoc);

    EXinterrupt(EX_ON);
    return (ioerr);
}

STATUS
PTM_spawn_the_TM( char *msg )
{
    STATUS                 syserr = OK ;

    register i4            ac ;
    register i4            ac2 ;

    char                 **nargv = NULL ;
    char                   sepflagbuf [16] ;
    char                  *stdinfile  = NULL ;
    char                  *stdoutfile = NULL ;

    LOCATION              *stdinLoc   = NULL ;
    LOCATION              *stdoutLoc  = NULL ;

    /* spawn terminal monitor */

    put_message(statusW, STpolycat(3,ERx("Starting"),TerMon,ERx("..."),msg));

    nargv = (char **)SEP_MEalloc(SEP_ME_TAG_NODEL, 81*sizeof(char *), TRUE,
                                 (STATUS *)NULL);

    for (ac = 0 ; lineTokens[ac+1] ; ac++)
    {
	nargv[ac]  = (char *) lineTokens[ac+1];
    }

    if (tracing&TRACE_DIALOG)
    {
        for (ac2=0; ac2<ac; ac2++)
	{
            SIfprintf(traceptr,ERx("process_tm1> nargv[%d]> %s\n"),ac2,
                      nargv[ac2]);
	}
	SIflush(traceptr);
    }

    if ((syserr = Trans_Tokens(SEP_ME_TAG_PARAM, nargv, NULL)) == OK)
    {

#ifdef VMS
#define STDINLOC    NULL
#define STDINLPTR   NULL
#define STDOUTLOC   NULL
#define STDOUTLPTR  NULL
#define TM_STR      ERx("<tm")
#define TO_STR      ERx(">to")
#else
#define STDINLOC    stdinLoc
#define STDOUTLOC   stdoutLoc
#define STDINLPTR  &stdinLoc
#define STDOUTLPTR &stdoutLoc
#define TM_STR      ERx("tm")
#define TO_STR      ERx("to")
#endif

	Create_stf_file(SEP_ME_TAG_NODEL, TM_STR, &stdinfile,  STDINLPTR,  msg);
	Create_stf_file(SEP_ME_TAG_NODEL, TO_STR, &stdoutfile, STDOUTLPTR, msg);

#ifdef VMS
	nargv[ac++] = stdinfile;
	nargv[ac++] = stdoutfile;
#endif

	STpolycat(2, ERx("-*"), SEPpidstr, sepflagbuf);
	nargv[ac++] = sepflagbuf;
	nargv[ac] = NULL;

	if (tracing&TRACE_DIALOG)
	{
	    SIfprintf(traceptr,ERx("process_tm2> argc=%d\n"),ac);
	    for (ac2=0; ac2<ac; ac2++)
	    {
		SIfprintf(traceptr,ERx("process_tm3> nargv[%d]=%s\n"),ac2,
			  nargv[ac2]);
	    }
            SIflush(traceptr);
	}

	if ((syserr = SEPspawn(ac, nargv, TC_NOWAIT, STDINLOC, STDOUTLOC,
				   &SEPsonpid, &Status_Value, 1)) != OK)
	{
	    disp_line(STpolycat(2,ERx("Error spawning"),TerMon,msg),0,0);
	    testGrade = SEPerr_Cant_Spawn;
	}

	if (stdinfile  != NULL) MEfree(stdinfile);
	if (stdoutfile != NULL) MEfree(stdoutfile);
	if (stdinLoc   != NULL) MEfree(stdinLoc);
	if (stdoutLoc  != NULL) MEfree(stdoutLoc);
    }


    MEfree(nargv);
    return (syserr);
}

STATUS
PTM_connect_with_TM()
{
    STATUS                 ioerr ;

    /*
    ** try to establish connection with TM
    */

    if (SEP_TCgetc_trace(SEPtcoutptr, waitLimit) != TC_BOS)
    { 
        STpolycat(3,ErrC,ERx("connect with "),TerMon,buffer_1);
        testGrade = SEPerr_TM_Cant_connect;
	ioerr = FAIL;
    }
    else
    {
        EXinterrupt(EX_ON);
        ioerr = OK;
    }

    return (ioerr);
}

STATUS
PTM_read_fm_TM( i4  *thechar )
{
    STATUS                 ioerr = OK ;
    i4                     lastchar = 0 ;

    /*
    ** read results from TM
    */
    simple_trace_if(TRACE_DIALOG,"dialog fm tm> ");

    put_message(statusW, ERx("Collecting results ..."));
    for (;;)
    {
	*thechar = SEP_TCgetc_trace(SEPtcoutptr, waitLimit);
	if ((*thechar == TC_EQR) || (*thechar == TC_TIMEDOUT))
	{
	    break;
	}

	if (*thechar == TC_EOF)
	{
	    ioerr = INGRES_FAIL;
	    break;
	}

	lastchar = *thechar;
	SEPputc(*thechar, sepResults);
    }
    if (lastchar != '\n')
    {
	SEPputc('\n', sepResults);
	simple_trace_if(TRACE_DIALOG,"\n");
    }

    SEPputc(SEPENDFILE, sepResults);
    SEPrewind(sepResults, TRUE);

    return (ioerr);
}

/*
** Name: PTM_get_next_cmd
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
**	30-Dec-2008 (wanfr01)
**	    Bug 121443
**	    Add parameter to inform skip_comment that get_command has printed
**	    the first line of the comment.
*/
STATUS
PTM_get_next_cmd( FILE * testFile )
{
    STATUS                 ioerr ;
    i4                     cmd = 0 ;
    i4                     i ;

    /*
    ** Get next command to be sent to TM, ask it to user if
    ** working in shell mode or take it from test script.
    */

    do
    {
        ioerr = shellMode ? shell_cmmd(TM_PROMPT)
                          : get_command(testFile,NULL,TRUE);

        if (ioerr == FAIL || ioerr == ENDFILE)
        {
            char *tmcmd_in = ERx("TM command in ") ;
            if (ioerr == FAIL)
            {
                STpolycat(3,ErrF,lookfor,tmcmd_in,buffer_1);
            }
            else
            {
                STpolycat(3,ERx("ERROR: EOF while "),lookfor,tmcmd_in,buffer_1);
                ioerr = FAIL;
            }

            if (shellMode)
	    {
                STcat(buffer_1, ERx("input"));
	    }
            else
	    {
                STcat(buffer_1, ERx("test script"));
	    }

            if (tracing&TRACE_DIALOG)
	    {
                SIfprintf(traceptr,ERx("process_tm4> after get_command, %s\n"),
			  buffer_1);
		SIflush(traceptr);
	    }

            break;

        }

        while (sep_state&IN_COMMENT_STATE)
        {
            ioerr = skip_comment(testFile,buffer_1,TRUE);
        }

        if (sep_state&BLANK_LINE_STATE)
        {
            cmd = 1;
            continue;
        }

        cmd = Is_it_If_statement(testFile, &i, (bool)FALSE);
    }
    while (cmd);

    return (ioerr);
}


/*{
** Name: PTM_write_cmd_to_TM - issue a command to terminal monitor
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Created comment block.
**	    Do not print command in this routine, it has already been printed.
*/
STATUS
PTM_write_cmd_to_TM()
{
    STATUS                 ioerr = OK ;
    register i4            ac2 ;
    char                  *goodin = NULL ;

    if (shellMode)
    {
	if (tracing&TRACE_PARM)   
	{ 
	    SIfprintf(traceptr,ERx("process_tm5> buffer_1=%s\n"),buffer_1);
	    SIfprintf(traceptr,ERx("process_tm6> buffer_2=%s\n"),buffer_2);
	    SIflush(traceptr);
	}
	if (Changed_Tokens == TRUE)
	{
	    STcopy(buffer_1, holdBuf);
	    for (ac2 = 0; lineTokens[ac2+1] ; ac2++)
		;
	    assemble_line(++ac2, buffer_1);
	    STpolycat(3, TM_PROMPT, ERx(" "), holdBuf, buffer_2);
	}
	else
	{
	    STpolycat(3, TM_PROMPT, ERx(" "), buffer_1, buffer_2);
	}

	if (tracing&TRACE_PARM)  
	{
	    SIfprintf(traceptr,ERx("process_tm7> buffer_1=%s\n"),buffer_1);
	    SIfprintf(traceptr,ERx("process_tm8> buffer_2=%s\n"),buffer_2);
	    SIflush(traceptr);
	}
	disp_line(buffer_2,0,0);
	append_line(buffer_2,1);
	STcat(buffer_1,ERx("\n"));
	goodin = buffer_1;
    }
    else
    {
	if (tracing&TRACE_PARM)   
	{ 
	    SIfprintf(traceptr,ERx("process_tm9> buffer_1=%s\n"),buffer_1);
	    SIfprintf(traceptr,ERx("process_tm10> buffer_2=%s\n"),buffer_2);
	    SIflush(traceptr);
	}
	if (Changed_Tokens == TRUE)
	{
	    STcopy(buffer_1, holdBuf);
	    for (ac2 = 0; lineTokens[ac2+1] ; ac2++) {} ;
	    assemble_line(++ac2, buffer_1);
	    STpolycat(3, TM_PROMPT, ERx(" "), buffer_1, buffer_2);
	    STcopy(buffer_2, buffer_1);
	    STcat(buffer_1, ERx("\n"));
	}
	if (tracing&TRACE_PARM)   
	{ 
	    SIfprintf(traceptr,ERx("process_tm11> buffer_1=%s\n"),buffer_1);
	    SIfprintf(traceptr,ERx("process_tm12> buffer_2=%s\n"),buffer_2);
	    SIfprintf(traceptr,ERx("process_tm13> holdBuf =%s\n"),holdBuf );
	    SIflush(traceptr);
	}
	if (Changed_Tokens == TRUE)
	{
	    if (!batchMode)
	    {
		disp_line(holdBuf, 0, 0);
	    }
	    append_line(holdBuf, 0);
	    goodin = buffer_1;
	    CMnext(goodin);
	}
	else
	{
	    if (!batchMode)
	    {
		disp_line(buffer_1, 0, 0);
	    }
	    goodin = buffer_1;
	    CMnext(goodin);
	}
    }
    EXinterrupt(EX_OFF);

    if (tracing&TRACE_DIALOG)
    {
	SIfprintf(traceptr,ERx("dialog to tm> %s\n"),goodin); 
	SIflush(traceptr);
    }

    for ( ; *goodin != EOS; CMnext(goodin))
    {
	TCputc(*goodin,SEPtcinptr);
    }

    endOfQuery(SEPtcinptr,ERx("PTM_write"));

    return (ioerr);
}


/*
** Name: process_if
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
*/

STATUS
process_if(FILE *testFile,char *commands[],i4 cmmdID)
{
    STATUS		   ioerr ;
    i4			   cmd ;
    i4			   first_time = TRUE ;

    for ( cmd = cmmdID; cmd; )
    {
	if (!first_time)		/* we need to start off with the dot */
	{				/* for the control command. */
	    append_line(CONTROL_PROMPT,0);
	    append_line(buffer_1,1);
	}
	if (if_ptr == 0)		/* if we're at the top level of the */
	{				/* construct, our job is easier. */
	    if (cmd != IF_CMMD)
	    {
		disp_line(ERx("Error processing IF,ELSE,ENDIF"),0,0);
		return (FAIL);
	    }
	    else
	    {
		if (if_level[if_ptr++] = Eval_If(commands))
		    cmd = 0;		/* Evaluate expression. if true, */
		else			/* get out. else, skip to next   */
		{			/* control command and continue. */
		    ioerr = get_command(testFile,CONTROL_PROMPT,FALSE);
		    cmd = classify_cmmd(sepcmmds,lineTokens[1]);
		}
	    }
	}
	else				/* We're inside an if,else,endif */
	switch (cmd)			/* contruct. */
	{
	    case IF_CMMD:		/* IF commands start new levels. */

		    if (if_level[if_ptr-1] != TRUE)
		    {
			if_level[if_ptr] = -1;
			ioerr = get_command(testFile,CONTROL_PROMPT,FALSE);
			cmd = classify_cmmd(sepcmmds,lineTokens[1]);
		    }
		    else
		    {
			if (if_level[if_ptr] = Eval_If(commands))
			    cmd = 0;
			else
			{
			    ioerr = get_command(testFile,CONTROL_PROMPT,FALSE);
			    cmd = classify_cmmd(sepcmmds,lineTokens[1]);
			}
		    }
		    if_ptr++;

		    break;

	    case ELSE_CMMD:

		    switch (if_level[if_ptr-1])
		    {
			case FALSE:
				if_level[if_ptr-1] = TRUE;
				cmd = 0;
			    break;
			case TRUE:
				if_level[if_ptr-1] = FALSE;
			case -1:
				ioerr = get_command(testFile,CONTROL_PROMPT,FALSE);
				cmd = classify_cmmd(sepcmmds,lineTokens[1]);
			    break;
		    }

		break;

	    case ENDIF_CMMD:

		    if (if_level[--if_ptr] != -1)
			cmd = 0;
		    else
		    {
			ioerr = get_command(testFile,CONTROL_PROMPT,FALSE);
			cmd = classify_cmmd(sepcmmds,lineTokens[1]);
		    }

		    if_level[if_ptr] = 0;

		break;

	}
	first_time = FALSE;
    }
    return (OK);
}

/*
**	process_fe
**
**	Description:
**	subroutine to interact with the frontend modules,
**	keystroke characters will be read from the test script (or type by
**	the user if working in shell mode), write to a shared file so
**	they can be read by the frontend, and wait for frontend output. 
**	After the frontend is finished writing the output, the routine reads
**	the output and diff it against the appropiate canons. This cycle
**	continues 'til no more keystrokes are present in the test script.
**	Parameters,
**	testFile    =>	pointer to test script
**	command	    =>	name of TM command to be executed
*/
STATUS
process_fe(FILE *testFile,char *command)
{
    i4                     Read_fm_FE_not_recording   ( STATUS *ioerr,
						        i4  *recording ) ;
    STATUS                 PFE_init		      ( char *command,
							char *saveit ) ;
    STATUS                 Connection_with_Ingres     ( i4  c, char *cmd,
							STATUS *ioerr ) ;
    STATUS                 Still_want_have_connection ( char *cmd,
							STATUS *ioerr) ;
    STATUS                 Get_key_from_User          ( i4  *in_keys,
							STATUS *s_refresh,
							i4 *record_ing ) ;
    STATUS                 Get_Input_from_Script      ( FILE *testFile ) ;
    STATUS                 PFE_put_keystrokes         ( i4  recording,
							i4 *in_keys,
							bool fromBOS,
							i4 *sendEOQ) ;
    STATUS                 syserr ;
    STATUS                 ioerr ;
    STATUS                 srefresh = FAIL ;

    bool                   fromBOS = FALSE ;
    bool                   newRes = TRUE ;

    i4                     thechar ;
    i4                     recording = 1 ;
    i4                     sendEOQ = 0 ;
    i4                     inkeys = 0 ;

    char                   saved_cmd [TEST_LINE] ;

    if ((ioerr = PFE_init(command, saved_cmd)) == OK)
    {	/*
	** start processing loop: read output / diff it / send input
	*/
	do
	{
	    goto_top_of_results_if( newRes, sepResults );
    
	    /*
	    **  read results generated so far
	    */
	    thechar = (!recording)
		? Read_fm_FE_not_recording(&ioerr, &recording)
		: Read_TCresults(SEPtcoutptr, SEPtcinptr, waitLimit, &fromBOS,
				 &sendEOQ, TRUE, &ioerr);
    
	    if (Connection_with_Ingres(thechar,saved_cmd,&ioerr) != OK)
	    {
		testGrade = SEPerr_FE_timed_out;
		break;
	    }
	    /*
	    **  create canon for command if creating a test, or
	    **  diff if running a test
	    */
	    if (recording)
	    {
		if (!shellMode)
		{
		    if ((syserr = diffing(testFile,0,1)) != OK)
		    {
			disconnectTClink(SEPpidstr);
			return(syserr);
		    }
		}
		else
		{
		    srefresh = create_canon_fe(sepResults);
		    recording = 0;          /* turn off recording */
		    simple_trace_if(TRACE_DIALOG,"dialog (clear recording)>\n");
		}
		newRes = TRUE;      /* time to get new output file */
	    }
    
	    if (Still_want_have_connection(saved_cmd,&ioerr) != OK)
	    {
		break;
	    }
    
#ifdef NT_GENERIC
	    if (!batchMode && !shellMode)
		SetConsoleActiveScreenBuffer(hFEconsole);
#endif
	    /*
	    **  Feed FE module with new keystrokes. Take them from test script
	    **  if running a test, or from the terminal if creating a test
	    */
	    if (shellMode)
	    {
		if (Get_key_from_User(&inkeys, &srefresh, &recording)
		    == KEY_TDOUT)
		    continue;
	    }
	    else
	    {
		ioerr = Get_Input_from_Script(testFile);
    
		if ((sep_state & IN_FE_STATE) == 0)
		{   /*
		    ** If we're not in the FE any longer, save command and get
		    ** out; letting main loop handle this command line.
		    */
		    fromLastRec = 1;
		    STcopy(buffer_1, holdBuf);
		    STpolycat(3,Sep_Warn,ERx("FE"),termnoinp,buffer_1);
		    break;
		}
	    }
	    /*
	    ** put keystrokes in input COMM-file
	    */
	    syserr = PFE_put_keystrokes(recording, &inkeys, fromBOS, &sendEOQ);
    
	}
	while (ioerr == OK);
    }

    disconnectTClink(SEPpidstr);
#ifdef NT_GENERIC
    SetConsoleActiveScreenBuffer(hTEconsole);
    CloseHandle(hFEconsole);
#endif
    return(ioerr);
}

STATUS
PFE_init(char *command, char *saveit)
{
    STATUS                 def_editor_names ( char *buf, i4  *Grade ) ;
    STATUS                 def_frsflags     ( char *buf, i4  *Grade ) ;

    STATUS                 ioerr ;
    register i4            ac ;

    EXinterrupt(EX_OFF);

    /*
    ** Open TC comm files and define ii_frsflags and editor env vars
    */
    ioerr = open_io_comm_files((bool)FALSE, SEPONFE);
    ioerr = ( ioerr == OK ) ? def_frsflags(buffer_1, &testGrade)     : ioerr;
    ioerr = ( ioerr == OK ) ? def_editor_names(buffer_1, &testGrade) : ioerr;
    if (ioerr != OK)
    {
	EXinterrupt(EX_ON);
        return(ioerr);
    }

    /*
    **  create coverup window, so the frontend can't mess up our window.
    */
    if (!batchMode)
    {
        coverupW = TDnewwin(24,80,0,0);
        TDrefresh(coverupW);
#ifdef NT_GENERIC
	hFEconsole = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_WRITE, &TEsa,
                         CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleMode(hFEconsole, ENABLE_PROCESSED_OUTPUT);
#endif
        disp_prompt(STpolycat(3,ERx("Starting FrontEnd module `"),command,
                              ERx("'..."),msg),NULL,NULL);
    }

    if (shellMode)
    {
        hide_cursor();
    }
    
    STcopy(command,saveit);   /* save command value */

    if ((ioerr = Trans_Tokens(SEP_ME_TAG_PARAM, lineTokens, NULL)) == OK)
    {   /*
        ** spawn frontend module
        */
#ifdef NT_GENERIC
	if (!batchMode && !shellMode)
	    SetConsoleActiveScreenBuffer(hFEconsole);
#endif
        for (ac = 0; lineTokens[ac+1]; ac++) {};
    
        if ((ioerr = SEPspawn(ac, &lineTokens[1], TC_NOWAIT, NULL, NULL,
                              &SEPsonpid, &Status_Value, 1)) != OK)
        {
            testGrade = SEPerr_Cant_Spawn;
            STpolycat(2,ErrC,ERx("start up frontend system"),buffer_1);
        }
        else
        {
            /* Rather than waiting for the SEP test timeout, because the
            ** process failed to start, verify that the process still exists
            ** during the wait time.
            */
            i4 waited = 0;
            i4 wait_for = (waitLimit) ? waitLimit : 1;
            i4 res;

            MEtfree(SEP_ME_TAG_PARAM);
            /*
            ** Check for the connection to the Front End.
            */
            simple_trace_if(TRACE_DIALOG,"dialog fm fe (initializing..) >\n");
    
            while (waited < wait_for)
            {
               /* Just wait for a second */
               res = SEP_TCgetc_trace(SEPtcoutptr, 1);

               if ( (res == TC_TIMEDOUT ) && (waited < wait_for) )
               {
                  if (!PCis_alive( SEPsonpid ))
                  {
                     STcopy(STpolycat(2,ErrC,ERx("connect to frontend system"),
                             msg), buffer_1);
                     testGrade = SEPerr_Cant_Connect_to_FE;
                     ioerr = FAIL;
                     break;
                  }
               }
               else if ( res != TC_BOS)
               {
                  STcopy(STpolycat(2,ErrC,ERx("connect to frontend system"),
                          msg), buffer_1);
                   testGrade = SEPerr_Cant_Connect_to_FE;
                   ioerr = FAIL;
                   break;
               }
               else
                  break;

               /* Only need to increment waited if the comamnd has a time limit */
               if (waitLimit) waited++;
            }
        }
    }
    
    EXinterrupt(EX_ON);
    return (ioerr);
}

STATUS
def_editor_names( char *buf, i4  *Grade )
{
    STATUS                 ret_val ;
    char                  *edit_name ;

    if (peditor_name == NULL)
    {
        NMgtAt(ERx("PEDITOR"), &edit_name);
	peditor_name = STtalloc(SEP_ME_TAG_NODEL,
				(edit_name != NULL)&&(*edit_name != EOS)
				? edit_name : PEDITOR );
    }
    edit_name = ERx("ING_EDIT");
    if ((ret_val = NMsetenv(edit_name, peditor_name)) == OK)
    {
#ifdef UNIX
	edit_name = ERx("EDITOR");
	if ((ret_val = NMsetenv(edit_name, peditor_name)) == OK)
	{
	    edit_name = ERx("VISUAL");
	    ret_val = NMsetenv(edit_name, peditor_name);
	}
#endif
    }
    
    if (ret_val != OK)
    {
	STcopy(STpolycat(2,ErrDef,edit_name,msg),buf);
	*Grade = SEPerr_Cant_Set_Peditor;
    }
    
    return (ret_val);
}

STATUS
def_frsflags( char *buf, i4  *Grade )
{
    STATUS                 ret_val ;

    /*
    **  define ii_frsflags symbol
    */
    STpolycat(2,ERx("-*"),SEPpidstr,msg);
    if (batchMode)
    {
        STcat(msg,ERx(",b"));
    }

    if (tracing&TRACE_DIALOG)
    {
        SIfprintf(traceptr,"dialog fe, II_FRSFLAGS>%s\n",msg);
	SIflush(traceptr);
    }

    if ((ret_val = NMsetenv(ERx("II_FRSFLAGS"),msg)) != OK)
    {
	STcopy(STpolycat(2,ErrDef,ERx("ii_frsflags"),msg),buf);
	*Grade = SEPerr_Cant_Set_FRSFlags;
    }
#ifdef NT_GENERIC
    if (!batchMode)  /* set flag to allow SEP to control screens in FE */
    {
      strcpy (msg, "true");
      NMsetenv(ERx("II_SEP_CONTROL"),msg);
    }
#endif
    return (ret_val);
}

i4
Read_fm_FE_not_recording(STATUS *ioerr, i4  *recording)
{
    i4                     thechar  = 0 ;
    i4                     lastchar = 0 ;

    simple_trace_if(TRACE_DIALOG,"Read_fm_FE_not_recording> dialog fm fe>\n");

    /*
    ** collecting partial results (avoiding write pipe blocking)
    */
    *ioerr = OK;
    for (;;)
    {
	EXinterrupt(EX_OFF);
	if (TCempty(SEPtcoutptr))
	{
	    simple_trace_if(TRACE_DIALOG,
	       "Read_fm_FE_not_recording> No data found\n");
	    break;
	}

	if ((thechar = SEP_TCgetc_trace(SEPtcoutptr, 0)) == TC_BOS)
	{
	    endOfQuery(SEPtcinptr,ERx("Read_fm_FE1"));
	    hide_cursor();
#ifdef VMS
	    if (SEPtcsubs < 4)
#endif
		PCsleep(cmd_sleep);
	    continue;
	}
	else if (thechar == TC_EOF)
	{
	    if (SEPtcsubs <= 0)
	    {
		*ioerr = INGRES_FAIL;
	    }
	    else
	    {
		endOfQuery(SEPtcinptr,ERx("Read_fm_FE2"));
		hide_cursor();
#ifdef VMS
		if (SEPtcsubs < 3)
#endif
		    PCsleep(cmd_sleep);
		continue;
	    }
	}

	if (thechar == TC_EQR || thechar == TC_EOF)
	{
	    if (lastchar != '\n')
	    {
		SEPputc('\n', sepResults);
		simple_trace_if(TRACE_DIALOG,"\n");
	    }
	    SEPputc(SEPENDFILE, sepResults);
	    SEPrewind(sepResults, TRUE);
	    *recording = 1;
	    simple_trace_if(TRACE_DIALOG,"dialog (set recording) >\n");
	    Encode_kfe_into_buffer();
	    append_line(buffer_2,1);		
	    break;
	}

	lastchar = thechar;
	SEPputc(thechar, sepResults);
    }
    EXinterrupt(EX_ON);
    simple_trace_if(TRACE_DIALOG,"Read_fm_FE_not_recording> End.\n");

    return (thechar);
}

STATUS
Get_key_from_User(i4 *in_keys, STATUS *s_refresh, i4  *record_ing)
{
    STATUS                 r_key = KEY_OK ;
    i4                     i ;
    i4                     inchar = 0 ;
    char                  *kfe_tmp ;

    simple_trace_if(TRACE_DIALOG,"Get_key_from_User> Start.\n");

    *in_keys = inchar;
    /*
    **	we might need to refresh the screen because a prompt was
    **	displayed
    */
    if (*s_refresh == OK)
    {
	buffer_1[inchar++] = S_REFRESH;
	*s_refresh = FAIL;
    }
    else
    {
        simple_trace_if(TRACE_DIALOG,"Get_key_from_User> Try to get key..\n");
	if ((r_key = getnxtkey(buffer_1, &inchar)) == KEY_TDOUT)
	{
            simple_trace_if(TRACE_DIALOG,
	                   "Get_key_from_User> Timeout, returning..\n");
    	    *in_keys = inchar;
	    return (r_key);
	}
    }
    /*
    **	add new character to set of keys entered so far
    */
    kfe_tmp = buffer_1;
    for (i = 0; i < inchar; i++)
	CMcpyinc(kfe_tmp, kfe_ptr);

    /*
    **	check if the menu key was pressed. This key might be an escape
    **  sequence for some terminal, if that's the case, read the entire
    **  escape sequence. If the menu key was pressed, turn on recording.
    */
    if (r_key == KEY_FK)
    {
	*record_ing = 1;
	simple_trace_if(TRACE_DIALOG,"dialog (set recording,FK pressed) >\n");
    }

    /*
    **	check if reaching limit for key buffer
    */
    if (!(*record_ing) && 
	(((kfe_ptr - kfebuffer) > (TERM_LINE-56) && !CMalpha(kfe_ptr))||
	  (kfe_ptr - kfebuffer) > (TERM_LINE-36)))
    {
	simple_trace_if(TRACE_DIALOG,"dialog (set recording) >\n");
	*record_ing = 1;
    }

    /*
    **	save keys that generated the output to be collected
    **	into new test script
    */
    if (*record_ing)
    {
	hide_cursor();
	Encode_kfe_into_buffer();
	if (tracing&TRACE_DIALOG)
	{
	    SIfprintf(traceptr,"dialog to fe (buffer_2)>%s\n",buffer_2);
	    SIflush(traceptr);
	}

	append_line(buffer_2,1);		
    }
    *in_keys = inchar;
    simple_trace_if(TRACE_DIALOG,"Get_key_from_User> End.\n");

    return (r_key);
}


STATUS
Connection_with_Ingres(i4 c, char *cmd, STATUS *ierr)
{
    register STATUS           serr = OK ;

    /*
    ** check connection with Ingres system
    */

    if (c == TC_TIMEDOUT)
    {
	STpolycat(3,Con_with,cmd,ERx(" timed out"),buffer_1);
	if (ierr != NULL)
	{
	    *ierr = FAIL;
	}
	serr  = FAIL;
    }
    else
    {
	STcopy(ERx("OK"),buffer_1);
    }

    if (tracing&TRACE_DIALOG)
    {
	SIfprintf(traceptr,ERx("Connection_with_Ingres> %s\n"),buffer_1);
	SIflush(traceptr);
    }

    return (serr);
}

/*
**  Encode_kfe_into_buffer
*/

VOID
Encode_kfe_into_buffer()
{
    char         *kfe_tmp1 = kfebuffer ;
    char         *kfe_tmp2 = buffer_2 ;

    *kfe_ptr = EOS;
    CMnext(kfe_tmp1);
    STcopy(ERx("^^ "), kfe_tmp2);
    CMnext(kfe_tmp2); CMnext(kfe_tmp2); CMnext(kfe_tmp2);
    encode_seqs(kfe_tmp1, kfe_tmp2);
}

STATUS
PFE_put_keystrokes( i4  recording, i4  *in_keys, bool fromBOS, i4  *sendEOQ )
{
    STATUS                 ret_val = OK ;
    char                  *goodin  = NULL ;
    char                  *tmp     = NULL ;
    char                  *tmp2    = NULL ;

    bool                   send_an_EOQ = FALSE ;

    /*
    **
    */
    if (shellMode)
    {
	if (recording && !USINGFK)
	{
	    buffer_1[(*in_keys)++] = TC_EOQ;
	}
	else if (recording)
	{
	    send_an_EOQ = TRUE;
	}

	buffer_1[*in_keys] = EOS;
	goodin = buffer_1;
    }
    else
    {
	append_line(buffer_1,0);        /* append info to test log */
	*in_keys = STlength(buffer_1);
	if (*in_keys <= 4)
	{
	    if (fromBOS)
	    {
		*sendEOQ = SEPtcsubs - 1;
	    }
	    return (ret_val);            /*
					 ** This WAS a continue statement when
					 ** this function was part of process_fe
					 */
	}

	if (buffer_1[*in_keys-1] == '\n')
	{
	    buffer_1[*in_keys-1] = EOS;    /* get rid of EOL */
	}

	/* decode input keys coming from test script */

	buffer_2[0] = S_REFRESH;        /* start by refreshing screen */
	decode_seqs(buffer_1 + 3,buffer_2 + 1);
	*in_keys = STlength(buffer_2);
	/*
	** I'm doing this because I suspect timing problems
	** when sending FK to the frontend
	** It's only a gut feeling so far (2/12/90)
	*/
	if (!USINGFK)
	{
	    buffer_2[(*in_keys)++] = TC_EOQ;
	}
	else
	{
	    send_an_EOQ = TRUE;
	}

	buffer_2[*in_keys] = EOS;
	goodin = buffer_2;

	if (IS_FileNotation(buffer_1))
	{
	    tmp = tmp2 = STtalloc(SEP_ME_TAG_NODEL,goodin);
	    Trans_SEPfile(&tmp,NULL,TRUE,SEP_ME_TAG_NODEL);
	    goodin = tmp;
	}
    }

    if (tracing&TRACE_DIALOG)
    {
	SIfprintf(traceptr,"dialog to fe (buffer_1)>%s\n",buffer_1);
	SIflush(traceptr);
    }

    TCflush(SEPtcinptr);

    for (EXinterrupt(EX_ON); *goodin != EOS; CMnext(goodin))
    {
	TCputc(*goodin,SEPtcinptr);
    }
    TCflush(SEPtcinptr);

    /*
    ** I'm doing this because I suspect timing problems
    ** when sending FK to the frontend
    ** It's only a gut feeling so far (2/12/90)
    */

    if (send_an_EOQ)
    {
	endOfQuery(SEPtcinptr,ERx("PFE_put_key"));
    }

    if (tmp != tmp2)
    {
	if (tmp)  MEfree(tmp);
	if (tmp2) MEfree(tmp2);
    }
    else
    {
	if (tmp)  MEfree(tmp);
    }

    return (ret_val);
}

/*
**  endOfQuery
*/

VOID
endOfQuery( TCFILE *tcinptr, char *errprefx )
{
    EXinterrupt(EX_OFF);
    TCputc(TC_EOQ, tcinptr);
    TCflush(tcinptr);
    EXinterrupt(EX_ON);

    if (tracing&TRACE_DIALOG)
    {
	SIfprintf(traceptr,
		  ERx("endOfQuery<%s> dialog to fe (Sending an EOQ)\n"),
		  errprefx);
	SIflush(traceptr);
    }
}

/*
**	disp_res
*/

STATUS
disp_res(SEPFILE *aFile)
{
    STATUS                 ioerr ;

    register i4            i ;
    register char         *cptr = NULL ;

    i4                     linenum = 0 ;

    bool                   lineRefresh = TRUE ;
    char                   tempBuff [MAIN_COLS] ;

                
/*
**  I need to add code to refresh at the end if doing big files or
**  refresh line by line if small files as it used to be.
**  In the mean time I'll refresh always at the end
*/

    SEPrewind(aFile, FALSE);
    while ((ioerr = SEPgetrec(buffer_1, aFile)) == OK)
    {
	cptr = buffer_1;
	while (SEP_CMstlen(cptr,0) >= MAIN_COLS)
	{
	    STlcopy(cptr, tempBuff, MAIN_COLS - 1);
	    disp_line_no_refresh(tempBuff,0,0);
            if (lineRefresh && (!(linenum = (linenum + 1) % MAIN_ROWS) ) )
	    {
		TDrefresh(mainW);
	    }
	    for (i=0; i<(MAIN_COLS - 1); i++)
	    {
	        CMnext(cptr);
	    }
	}
	disp_line_no_refresh(cptr,0,0);
        if (lineRefresh && (!(linenum = (linenum + 1) % MAIN_ROWS) ) )
	{
	    TDrefresh(mainW);
	}
    }
    TDrefresh(mainW);

    if (ioerr == ENDFILE)
    {
	ioerr = OK;
    }
    else
    {
	disp_line(ERx("ERROR while reading output file"),0,0);
	if (ioerr == OK)
	{
	    ioerr = FAIL;
	}
    }

    return (ioerr);
}



/*
** Name: form_canon
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
*/

STATUS
form_canon(FILE *aFile,i4 *canons,char **open_args,char **close_args)
{
    STATUS                 ioerr ;
    STATUS                 ignore = FAIL ;
    i4                     to_canons ;
    char		  *cann  = ERx("canon from test script") ;

    /* create canon file */
    
    to_canons = *canons + 1;
    SEPrewind(sepCanons, FALSE);
    
    /* get canon from test script */

    in_canon = TRUE;
    do
    {
	ioerr = get_command(aFile,NULL,FALSE);
	if (ioerr == FAIL || ioerr == ENDFILE)
	{
	    if (ioerr == FAIL)
		disp_line(STpolycat(3,ErrF,lookfor,cann,msg),0,0);

	    return(FAIL);
	}
    } while ((sep_state&BLANK_LINE_STATE)||(sep_state&IN_ENDING_STATE));

    if ((sep_state & IN_CANON_STATE) == 0)
    {
	fromLastRec = 1;
	STcopy(buffer_1, holdBuf);
	return(FAIL);
    }

    if (sep_state & OPEN_CANON_ARGS)
	*open_args = STtalloc(SEP_ME_TAG_CANONS, buffer_1);
    else
	*open_args = NULL;

    if (updateMode)
    {
	SEPputrec(buffer_1, sepGCanons);
	if (CMcmpcase(SEP_CMlastchar(buffer_1,0), ERx("\n")) != 0)
	    SEPputc('\n', sepGCanons);
    }

    do
    {
	in_canon = TRUE;

	ioerr = get_command(aFile,NULL,FALSE);
	if (ioerr == FAIL || ioerr == ENDFILE)
	{
	    if (ioerr == FAIL)
		STpolycat(3,ErrF,ERx("reading "),cann,msg);
	    else
		STpolycat(3,ErrF,ERx("reading (EOF) "),cann,msg);

	    disp_line(msg,0,0);
	    return(FAIL);
	}

	if (sep_state & IN_CANON_STATE)
	{
	    if (sep_state & (IN_SKIP_STATE|IN_SKIPV_STATE))
		ignore = OK;

	    SEPputrec(buffer_1, sepCanons);

	    if (updateMode)
		SEPputrec(buffer_1, sepGCanons);
	}
    } while (sep_state & IN_CANON_STATE);

    if (sep_state & CLOSE_CANON_ARGS)
	*close_args = STtalloc(SEP_ME_TAG_CANONS, buffer_1);
    else
	*close_args = NULL;

    if (updateMode)
    {
	SEPputrec(buffer_1, sepGCanons);
	if (CMcmpcase(SEP_CMlastchar(buffer_1,0),ERx("\n")) != 0)
	    SEPputc('\n', sepGCanons);
    }
    SEPputc(SEPENDFILE, sepCanons);
    SEPrewind(sepCanons, TRUE);
    (*canons)++;

    return(ignore == OK ? SKIP_CANON : OK);
}




/*
** Name: next_canon
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
*/

STATUS
next_canon(FILE *aFile,bool logit,bool *err_or)
{
    STATUS                 ioerr ;
    char                  *failed_skipping = ERx("ERROR: failed while skipping canons from test script") ;

    *err_or  = FALSE;
    in_canon = TRUE;
    do
    {
	ioerr = get_command(aFile,NULL,FALSE);
	if (ioerr == FAIL || ioerr == ENDFILE)
	{
	    if (ioerr == FAIL)
		disp_line(failed_skipping,0,0);
	    *err_or = TRUE;
	    return(ioerr);
	}
    } while ((sep_state & BLANK_LINE_STATE)||(sep_state & IN_ENDING_STATE));

    if ((sep_state & IN_CANON_STATE) == 0)
    {
	fromLastRec = 1;
	STcopy(buffer_1, holdBuf);
	return (ENDFILE);
    }

    if (logit)
	append_line(buffer_1, 0);

    do
    {
	in_canon = TRUE;
	ioerr = get_command(aFile,NULL,FALSE);
	if (ioerr == FAIL || ioerr == ENDFILE)
	{
	    if (ioerr == FAIL)
		disp_line(failed_skipping,0,0);

	    *err_or = TRUE;
	    return(ioerr);
	}
	if (logit)
	    append_line(buffer_1, 0);

    } while (sep_state & IN_CANON_STATE);

    return(OK);
}

/*
**      skip_canons
*/

STATUS
skip_canons(FILE *aFile,bool logit)
{
    STATUS                 ioerr ;
    bool                   is_error ;

    do
    {
	ioerr = next_canon(aFile,logit,&is_error);
    } while ( ioerr == OK && is_error == FALSE);

    if (is_error)
	return (ioerr);
    else
	return (OK);
}


/*
**	diff_canon
*/

STATUS
diff_canon(i4 *diffound)
{
    STATUS                 ret_val = OK ;
    i4                     diffvalue ;
    static char            diffout [TEST_LINE + 1] ;

    if ((diffvalue = QAdiff(&sepCanons,&sepResults,sepDiffer,diffLevel)))
    {
	/*
	** create DIFF file => diff + canon + result
	*/
	*diffound = diffvalue;

	/*
	** append canon
	*/
	append_obj(cbanner, diffout, sepCanons, NULL);

	/*
	** append results
	*/
	append_obj(rbanner, diffout, sepResults, rbannerend);

	/*
	** mark end of file
	*/
 	SEPputc(SEPENDFILE, sepDiffer);
 	STprintf(diffout, ERx("%*s"), TEST_LINE-2, NULLSTR);

	if (SEPputrec(diffout, sepDiffer) != OK)
	    disp_line(ErrApp,0,0);

	SEPrewind(sepDiffer, TRUE);
	ret_val = FAIL;
    }

    return (ret_val);

}

/*
**  append_obj
*/    

STATUS
append_obj(char *banner,char *diffout,SEPFILE *filptr,char *ebanner)
{
    register i4            i ;
    register char         *cptr = NULL ;
    char                   buffer [SCR_LINE] ;
    STATUS                 ioerr ;
    i4                     count ;

    STprintf(buffer,banner);
    STmove(buffer, ' ', TEST_LINE, diffout);
    diffout[TEST_LINE] = EOS;

    if (SEPputrec(diffout, sepDiffer) != OK)
	disp_line(ErrApp,0,0);

    while ((ioerr = SEPgetrec(buffer, filptr)) == OK)
    {
#ifdef NT_GENERIC  /* get rid of newline character from buffer */
	if (buffer[strlen(buffer)-1] == '\n')
	  buffer[strlen(buffer)-1] = ' ';
#endif
	cptr = buffer;
	while (*cptr != EOS)
	{
	    STmove(cptr, ' ', TEST_LINE, diffout);
	    diffout[TEST_LINE] = EOS;
	    if (SEPputrec(diffout, sepDiffer) != OK)
	    {
		disp_line(ErrApp,0,0);
		break;
	    }
	    if ((count = SEP_CMstlen(cptr,0)) > TEST_LINE)
	    {
		for (i=0; i<TEST_LINE; i++)
		    CMnext(cptr);
	    }
	    else
	    {
		for (i=0; i<count; i++)
		    CMnext(cptr);
	    }
	}
    }
    if (ioerr != ENDFILE)
	disp_line(ERx("ERROR: reading from file (making diff)"),0,0);

    if (ebanner != NULL)
    {
	STprintf(buffer,ebanner);
	STmove(buffer, ' ', TEST_LINE, diffout);
	diffout[TEST_LINE] = EOS;

	if (SEPputrec(diffout, sepDiffer) != OK)
	    disp_line(ErrApp,0,0);
    }

    SEPrewind(filptr, FALSE);
}

/*
**  disconnectTClink
**
**  Description:
**  routine to disconnect SEP from a TC link, it's called upon
**  completion of an INGRES session or when an exception occurs
**	03-Aug-1998 (merja01)
**		Change kill signal from terminate(15) to kill(9).  This      
**		resolves a problem cleaning up processes encountered on
**		axp_osf and solaris.  For some reason, the terminate
**		signal was terminating all process including the parent.
*/

STATUS
disconnectTClink(char *pidstring)
{
    char                   buffer [80] ;
    int		wait_status;
#ifdef NT_GENERIC
    HANDLE	SEPsonhandle;
	HANDLE procs;
	PROCESSENTRY32 pe;
	int bRet;
	typedef struct procid_list {
		PID	pid;
		struct procid_list *prev;
		struct procid_list *next;
	}PROCID_LIST;
	PROCID_LIST		root_pid;
	PROCID_LIST		*plist_head;
	PROCID_LIST		*plist_ptr;
	PROCID_LIST		*plist_tail;	
#endif

    EXinterrupt(EX_OFF);
#ifdef VMS
    SEPkill(SEPsonpid, pidstring);
# else

    if (SEPsonpid != 0)
    {
		int		count = 2;
		bool	bAlive = FALSE;

        PCsleep(cmd_sleep);
#ifdef NT_GENERIC
        SEPsonhandle = OpenProcess (PROCESS_ALL_ACCESS,FALSE,SEPsonpid);
        wait_status = WaitForSingleObject (SEPsonhandle,0); 
#else
        waitpid(SEPsonpid, &wait_status, WNOHANG);
#endif
	/* If command is still alive, it may be that it's just taking a 
	** little longer to complete.  Instead of increasing SEP_CMD_SLEEP
	** add some intelligence to give the command an extra few minutes.
	*/
	while (count--)
	    if (( bAlive = PCis_alive( SEPsonpid )) == TRUE )
		PCsleep(cmd_sleep);
		
	bAlive = PCis_alive(SEPsonpid);
	if (bAlive == TRUE)
        {
	    STcopy(ERx("ERROR: Child did not exit, had to terminate."),buffer);
	    append_line(buffer,1);
#ifdef NT_GENERIC
	    /* Terminating just one process is not enough because that leaves
	    ** commands executed by sepchild lingering around.
	    ** If we're terminating a proces, terminate its children too.
	    */
	    root_pid.pid = SEPsonpid;
	    root_pid.next = NULL;
	    plist_head = plist_tail = plist_ptr = &root_pid;
	    plist_head->prev = plist_head;

	    memset(&pe, 0, sizeof(PROCESSENTRY32));
	    pe.dwSize = sizeof(PROCESSENTRY32);
	    procs = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS/*TH32CS_SNAPALL*/, SEPsonpid);
	    /* Scan all processes and create a linked list of 
	    ** process ids of the children under root process.
	    ** All processes in the tree should be killed.
	    */
	    if (procs)
	    {
		while (plist_ptr != NULL)
		{
		     bRet = Process32First(procs, &pe);
		     while (bRet)
		     {
			if(pe.th32ParentProcessID == plist_ptr->pid)
			{
			     char *tmp = plist_tail;
			     plist_tail = plist_tail->next = SEP_MEalloc(SEP_ME_TAG_PIDS, sizeof(PROCID_LIST), TRUE, NULL);
			     plist_tail->pid = pe.th32ProcessID;
			     plist_tail->next = NULL;
			     plist_tail->prev = tmp;
			     SEPsonhandle = OpenProcess (PROCESS_TERMINATE,FALSE, pe.th32ProcessID); 
			     TerminateProcess(SEPsonhandle, 15);
			     CloseHandle(SEPsonhandle);
			}
			bRet = Process32Next(procs, &pe);
		     }
		     plist_ptr = plist_ptr->next;
		}
	    }
	    /* free all the pids  */
	    plist_ptr = plist_tail;
	    while (plist_ptr != plist_head && plist_ptr != NULL)
	    {
		MEfree(plist_ptr);
		plist_ptr = plist_ptr->prev;
		plist_ptr->next = NULL;
	    }
            SEPsonhandle = OpenProcess (PROCESS_TERMINATE,FALSE,SEPsonpid); 
            if (SEPsonhandle) 
	    {
		TerminateProcess(SEPsonhandle, 15);
		CloseHandle(SEPsonhandle);
	    }
#else
	    kill(SEPsonpid, 9);
#endif /*NT_GENERIC*/
        }

        PCsleep(cmd_sleep);
#ifdef NT_GENERIC
        SEPsonhandle = OpenProcess (PROCESS_ALL_ACCESS,FALSE,SEPsonpid);
        wait_status = WaitForSingleObject (SEPsonhandle,0);
#else
        waitpid(SEPsonpid, &wait_status, WNOHANG);
#endif
        if (PCis_alive(SEPsonpid) == TRUE) 
        {
	    STcopy(ERx("ERROR: Child did not exit/terminate, had to kill."),
	           buffer);
	    append_line(buffer,1);
#ifdef NT_GENERIC
            SEPsonhandle = OpenProcess (PROCESS_TERMINATE,FALSE,SEPsonpid);
			if (SEPsonhandle)
			{
				TerminateProcess(SEPsonhandle, 9);
				CloseHandle(SEPsonhandle);
			}
#else
	    kill(SEPsonpid, 9);
#endif
        }
    }
#endif /* VMS */

    switch(SEPtclink)
    {
	case SEPONBE:

	    /* close I/O COMM-files */

	    TCclose(SEPtcinptr);
	    TCclose(SEPtcoutptr);

	    break;

	case SEPONFE:

	    /* restore cursor */

	    if (shellMode)
		show_cursor();

	    /* close and delete I/O shared files */

	    while (SEPtcsubs-- >= 1)
	    {
		TCputc(TC_EOF, SEPtcinptr);
		TCflush(SEPtcinptr);
#ifdef VMS
		PCsleep(cmd_sleep);
#endif
	    }
	    TCclose(SEPtcinptr);
	    TCclose(SEPtcoutptr);

#ifdef UNIX
	    while (wait(0) != -1)
	    ;
#endif

	    /* deassign environment variable II_FRSFLAGS */

	    if (NMdelenv(ERx("II_FRSFLAGS")) != OK)
		disp_line(STpolycat(2,ErrDea,ERx("ii_frsflags"),msg),0,0);
	    if (NMdelenv(ERx("ING_EDIT")) != OK)
		disp_line(STpolycat(2,ErrDea,ERx("ing_edit"),msg),0,0);
#ifdef UNIX
            if (NMdelenv(ERx("EDITOR")) != OK)
		disp_line(STpolycat(2,ErrDea,ERx("EDITOR"),msg),0,0);
            if (NMdelenv(ERx("VISUAL")) != OK)
		disp_line(STpolycat(2,ErrDea,ERx("VISUAL"),msg),0,0);
#endif

	    /* get rid of coverup window */

	    if (!batchMode)
	    {
		if (SEPdisconnect)
		{
		    PCsleep(3500);
		}
		TDdelwin(coverupW);
		fix_cursor();
		TDclear(curscr);
		TDtouchwin(mainW);
		TDtouchwin(statusW);
		TDrefresh(mainW);
	    }
	    else
	    {
#ifdef hp9_mpe
                STprintf(buffer,ERx("b%s.sep"),  pidstring);
#else
		STprintf(buffer,ERx("b_%s.sep"), pidstring);
#endif
		del_file(buffer);
	    }
	    break;
    }


    /* delete temporary files */
    STprintf(buffer,ERx("tm%s.stf"),pidstring);
    del_file(buffer);
    STprintf(buffer,ERx("to%s.stf"),pidstring);
    del_file(buffer);
#ifdef hp9_mpe
    STprintf(buffer,ERx("in%s.sep"), pidstring);
#else
    STprintf(buffer,ERx("in_%s.sep"),pidstring);
#endif
    del_file(buffer);
#ifdef hp9_mpe
    STprintf(buffer,ERx("out%s.sep"), pidstring);
#else
    STprintf(buffer,ERx("out_%s.sep"),pidstring);
#endif
    del_file(buffer);

    EXinterrupt(EX_ON);

    return(OK);
}

/*
** Name: Get_Input_from_Script
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
**	30-Dec-2008 (wanfr01)
**	    Bug 121443
**	    Add parameter to skip_comment
*/

STATUS
Get_Input_from_Script(FILE *testFile)
{
    STATUS                 serr ;
    i4                     cmd = 0 ;
    i4                     i ;

    simple_trace_if(TRACE_DIALOG,"Get_Input_from_Script> Start.\n");
    do
    {
	serr = get_command(testFile,NULL,FALSE);
	/*
	**  check if problems while reading from test script
	*/
	if (serr == FAIL || serr == ENDFILE)
	{
	    if (serr == FAIL)
		disp_line(STpolycat(3,ErrF,lookfor,ERx("key seqs in script"),
                                    msg),0,0);
	    break;
	}
	if (sep_state&BLANK_LINE_STATE)
	{
	    cmd = 1;
	    continue;
	}
	if (sep_state&IN_ENDING_STATE)
	{
	    cmd = 0;
	    continue;
	}
	if (sep_state & IN_CANON_STATE)
	{
	    /*
	    ** We found a canon when we expected a command. Let's
	    ** skip it until we get to the end of the canon and
	    ** try again.
	    */
	    while (sep_state & IN_CANON_STATE)
	    {
		serr = get_command(testFile,NULL,FALSE);

		/*
		**  check if problems while reading from test script
		*/
		if (serr == FAIL || serr == ENDFILE)
		{
		    if (serr == FAIL)
		    {
			disp_line(STpolycat(3, ErrF, lookfor,
					    ERx("key seqs in script"), msg),
				  0,0);
		    }
		    break;
		}
	    }

	    if (sep_state & IN_CANON_STATE)
		break;

	    cmd = 1;
	    continue;
	}

	/*
	** ************************************************ **
	** make sure keystroke chars were read. It could've **
	** been a ".if,.else,.endif" statement.             **
	** ************************************************ **
	*/
	cmd = Is_it_If_statement(testFile, &i, (bool)TRUE);
    }
    while (cmd);

    while (sep_state&IN_COMMENT_STATE)
	serr = skip_comment(testFile,buffer_1,FALSE);

    simple_trace_if(TRACE_DIALOG,"Get_Input_from_Script> End.\n");
    return (serr);
}

/*
**	getnxtkey()
**
**	Description:
**	get next character from the terminal, it returns
**	KEY_TDOUT	timeout while getting next character
**	KEY_OK		got next character(s)
**	KEY_FK		function key entered or ESC if not using function
**			keys
*/

STATUS
getnxtkey(char *inbuf,i4 *inchars)
{
    char                   keysread [10] ;
    char                   fchar ;
    i4                     achar ;
    i4                     keyscount = 0 ;
    i4                     i ;
    STATUS                 ioerr ;
    STATUS                 flag ;
    FUNCKEYS              *fk = NULL ;

#ifdef VMS
    EXinterrupt(EX_OFF);
#endif
    achar = TEget(TEGET_WAIT);
#ifdef NT_GENERIC          /* if the character is "ESC", must "skip over"    */
    if (achar==27)         /*   the characters "[" and "t" which is put into */
    {                      /*   the keyboard buffer by TEget()               */
      TEget(TEGET_WAIT);
      TEget(TEGET_WAIT);
    }
#endif
#ifdef VMS
    EXinterrupt(EX_ON);
#endif
    if (achar == TE_TIMEDOUT)
	return(KEY_TDOUT);

    ioerr = KEY_OK;
    keysread[keyscount++] = fchar = (char)achar;

    if (USINGFK && !CMalpha(keysread))
    {
	char *fkptr;

	flag = FAIL;
	for (fk = fkList; fk != NULL; fk = fk->next)
	{
	    if (fk->string)
		fkptr = fk->string;
	    else
		continue;
		i = 1;
		achar = fchar;
		for(;;)
		{
		    if (*fkptr++ != (char)achar)
			break;

		    if (*fkptr == EOS)
		    {
			flag = OK;
			break;
		    }
		    if (i < keyscount)
			achar = keysread[i++];
		    else
		    {
#ifdef VMS
			EXinterrupt(EX_OFF);
#endif
			achar = TEget(TEGET_WAIT);
#ifdef NT_GENERIC
			if (achar==27)
			{
			  TEget(TEGET_WAIT);
			  TEget(TEGET_WAIT);
			}
#endif
#ifdef VMS
			EXinterrupt(EX_ON);
#endif
			if (achar == TE_TIMEDOUT)
			    break;
			else
			{
			    keysread[keyscount++] = (char)achar;
			    i++;
			}
		    }
		}
		if (flag == OK)
		{
		    ioerr = KEY_FK;
		    break;
		}
	    }
	}
	else
	{
	    if (!USINGFK && fchar == ESC)
		ioerr = KEY_FK;
	}
	for (i = 0; i < keyscount; i++)
	{
		inbuf[i] = keysread[i];
	}
	*inchars = keyscount;
	return(ioerr);

}

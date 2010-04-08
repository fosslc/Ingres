/*
** septool.c
**
** This file conatins the main routine for SEP, as well as various utility
** routines.
**
** History:
**    03-mar-89 (eduardo)  Created.
**    16-jun-89 (mca)      Added ability to edit test comments while in
**			   test creation mode.
**    21-jun-89 (mca)      Added sep_version string. This is version 0.4c.
**			   Also, made "septerm" mode the default. Now, the -i
**			   command line flag is used to turn this mode off.
**			   Removed some debug output. Added -e flag, which
**			   supresses invokation of the editor on the test
**			   header while in shell mode. Changed the hdrfile
**			   variable from a char * to a char[MAX_LOC], so that
**			   itr can be correctly passed to LOfroms().
**    16-aug-1989 (boba)   Use FTLIB and UGLIB instead of SEPWINLIB.
**                         Fix getstr calls.
**    23-aug-1989 (mca)    Save the value of the ING_EDIT environment var,
**			   since we'll need it for UTedit and it will get
**			   trashed later by process_fe().
**    28-aug-89 (eduardo)  Added support for '-o' flag
**    06-sep-89 (eduardo)  Increased size of following arrays: lineTokens
**			   & testName.
**    07-Sep-89 (eduardo)  Added support for '-w' flag
**    13-Sep-89 (eduardo)  Added decclarations and flag used to communicate 
**			   exit status to listexec in VMS environment
**    6-Nov-1989 (eduardo)
**			   Added performance improvements through SEPFILES
**    6-Nov-1989 (eduardo)
**			   Fixed inconvenience that required user to type
**			   'sep' suffix when executing a test. In order words
**                         if user types 'sep oc001', sep will take it as
**                         'sep oc001.sep'
**    7-Nov-1989 (eduardo)
**			   removed routine close_std since standard files
**			   (stdin, stdout, & stderr) are closed by PCexit
**    7-Nov-1989 (eduardo)
**			   Added exception handling through EX routines
**    7-Nov-1989 (eduardo)
**			   Eliminating moving to TST_SEP directory to read
**			   commands.sep and termcap.sep files
**    30-Nov-1989 (eduardo)
**			   Added capability to ignore case while diffing
**		           as requested by the Gateway Group (J. Powell)
**			   Created 'ignoreCase' variable for this purpose
**    12-dec-89 (mca)	   Set IITCmaster bool for UNIX. This allows SEP and
**			   INGRES to use the same TCclose; only SEP should
**			   unlink sync files.
**    12-Dec-1989 (eduardo)
**			  Added capability to diff white spaces if needed
**    28-Dec-1989 (Eduardo)
**			  Change disp_line routines to display (MAIN_COLS-1)
**			  chacters per line
**    28-Dec-1989 (Eduardo)
**			  Added 'i' value (ignore) to '-a' flag
**    29-Dec-1989 (Eduardo)
**			  Added support for 'automated' update mode,
**		          user can now specify '-mx' where x is either
**			  'm','a','o', or 'i'. This will direct SEP to
**			  append results as main, alternate, only, or 
**		     	  create ignored canons each time there is a
**			  diff. The idea here is to give the user the
**			  capability of updating a test without having
**			  to answer prompt questions each time and the
**			  answer is always the same.
**    29-Dec-1989 (Eduardo)
**			  removed following inconsistency that was 
**			  bothering users:
**			  users were asked to enter 'sep -m testname'
**			  if wanting to work in update mode, note the
**			  space between '-m' flag and the test name.
**			  However, they were asked to enter
**			  'sep -stestname' if wanting to work in 
**			  shellMode, note the no space between the
**			  '-s' flag and the test name. Well, from
**			  now on user will be asked to enter
**			  'sep -s testname' as they do with the -m flag.
**			  Previous syntax is maintained for old timers
**			  sake.
**    15-Jan-1990 (Eduardo)
**			  Added support for function keys
**    16-Jan-1990 (Eduardo)
**			  Cosmetic changes. Move parts of code & comments
**			  to appropiate place
**    27-Feb-1990 (Eduardo)
**                        Moved reading of termcap file by TD routines
**			  before reading SEP termcap data (so we have
**			  access to K0, Ka, etc. variables).
**			  Added TDsetterm call (to read termcap file) if
**			  working in batchMode
**	16-may-1990 (rog)
**		Changed sep_version to include GV_VER.
**	29-may-1990 (rog)
**		Comment out the TDsetterm() call added 27-feb-1990.  This
**		routine does ioctl()'s that turn off echo to the terminal.
**	20-jun-1990 (rog)
**		Put a date in the 'History:' section instead of xx-xxx-1989.
**	27-jun-1990 (rog)
**		Move killing of INGRES process into disconnectTClink().
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	23-jul-1990 (rog)
**		Handle continued lines correctly.
**	01-aug-1990 (rog)
**		Only allow commands to have continued lines.
**	03-aug-1990 (rog)
**		Fix continued keystroke lines.
**	08-aug-1990 (rog)
**		More work on continued lines and some performance improvements.
**	10-aug-1990 (rog)
**		Added in_canon so we do not interpret '-'s that shouldn't be.
**	27-aug-1990 (rog)
**		Last (hopefully) of the '-' changes.
**	18-sep-1990 (rog)
**		Change a temporary buffer from TEST_LINE to TERM_LINE.
**	21-feb-1991 (gillnh2o)
**		Per jonb, added LIBINGRES to NEEDLIBS and removed everything 
**		else that isn't built out of SEP. This may obviate the need
**		to change NEEDLIBS for every new release.
**	19-jul-1991 (donj)
**		Changed i4  if_level and if_ptr to GLOBALDEFs for UNIX cc.
**		VMS cc wasn't so picky.
**	23-jul-1991 (donj)
**		GLOBALDEFs shouldn't be #ifdef'd.
**	30-aug-91 (donj)
**		Add support for "[UN]SEPSET PAGING" in function, disp_line().
**		Add support for Mnemonic Error codes to pass error status back
**		to LISTEXEC in a more coherrent manner.
**		Close trace file on exiting, if tracing still on.
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Added explicit declarations for all submodules called. Removed
**	    smatterings of code that had been commented out long ago. Added
**	    conditional statement support to call process_if. Added function
**	    set_sep_state() in get_command() and shell_cmmd() to set sep_state
**	    variable. Use the sep_state variable to check state rather than
**	    always looking at the line buffer. Change paging var (sepset
**	    command) to default to on. Yet turn it off if we're going into
**	    forms based frontend. Simplified and clarified some control loops.
**	    Added maintenance history upkeep in copy_hdr() routine. Added
**	    SEPerr_'s to many of the error traps.
**	16-jan-1992 (donj)
**	    Problem with change in definition of open and close comment. Old
**	    definition had open comment as the only token on a line to start
**	    a comment and the close comment as the only token on a line to
**	    end a comment. In trying to change this to be moe 'C' like, I
**	    caused a complication. I'm going to temporarily go back to the old
**	    definition.
**	16-jan-1992 (donj)
**	    Add SEPerr_Cant_find_test so that SEP can abort in batchMode when
**	    it can't find the test script. SIprintf() was writing the error in
**	    get_answer() out to the canon file endlessly and filling up all
**	    the free space on VMS. Old method works fine on UNIX. I did add
**	    a "SEPerr_question_in_batch" to get_answer() for other possible
**	    problems.
**	29-jan-1992 (donj)
**	    Added fuzz_factor and "-f" switch for diff_numerics diffing
**	    precision.
**	19-feb-1992 (donj)
**	    Fixed some UNIX problems; eq_sign, d_quote and s_quote, can't
**	    be static inside a function.
**      02-apr-1992 (donj)
**          Removed unneeded references. Changed all refs to '\0' to EOS. Chang
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation. Added definitions for open and close
**	    canon ("<<".">>") argument lists. Added SEP_cmd_line() and
**	    OPTION_LIST structure for more efficient command line parsing.
**	    Add sep_state capability to notice the canon args. Removed from
**	    get_args() the commands handled by SEP_cmd_line().
**      15-apr-1992 (donj)
**	    Got rid of gotuserName; not used anywhere. Moved the SEP_cmd_line()
**	    to utility2.c so it could be used by LISTEXEC and EXECUTOR.
**	05-jul-1992 (donj)
**	    Modularized SEP version code to be used by other images. Replace
**	    references to '"' to more compatible DBL_QUO static array.
**	10-jul-1992 (donj)
**	    Isolating MEreqmem ME_tags.
**	10-jul-1992 (donj)
**	    Switched MEadvise fm ME_INGRES_ALLOC to ME_USER_ALLOC for UNIX.
**	    Didn't work on sun4 (gracie or neon).
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	21-jul-1992 (donj)
**	    Fix a problem with break_line handling double quoted strings that
**	    didn't end with EOS or whitespace (ie '(SEPPARAM == "Y") || ...').
**	    -
**	    Message declaration in get_command() should be an array of chars
**	    not an array of char pointers.
**	07-Oct-1992 (donj)
**	    Implemented a SEP_TIMEOUT environmental var that will modify the
**	    default waitLimit.
**	30-nov-1992 (donj)
**	    Add the "-o" outloc enhancements, grammar_methods, the new
**	    environmental variables, SEP_IF_EXPRESSION, SEP_TRACE, SEP_SET,
**	    SEP_TIMEOUT. Externalized get_string(). Handle allocated memory
**	    better with ME tag, SEP_ME_TAG_ENVCHAIN.
**	18-jan-1993 (donj)
**	    Fixed a problem with tracing, re: opening the trace file. Also
**	    handling the tracing flag.
**      18-jan-1993 (donj)
**          Wrap LOcompose() with SEP_LOcompose() to fix a UNIX bug.
**	04-feb-1993 (donj)
**	    Add GLOBALDEFS for FILLSTACK vars here from sepostcl.c.
**	09-mar-1993 (donj)
**	    Use the new function, SEP_LOwhatisit(), to determine the LOCTYPE
**	    for the '-o' (output) flag. It could be a PATH, PATH&FILENAME or
**	    just a FILNAME.
**	24-mar-1993 (donj)
**	    Alter fuzz factor to the new format.
**	24-mar-1993 (donj)
**	    Add patch to recover from a puzzling problem with qasetuser on VMS.
**      26-mar-1993 (donj)
**          Change exp10() call to pow() call. exp10() doesn't exist on
**          VAX/VMS.
**      26-mar-1993 (donj)
**          Change pow() call to MHpow() call. Use the CL, Use the CL, Use
**	   the CL, Use the CL, Use the CL.
**	30-mar-1993 (donj)
**	    Add (f8) casting to fix a bug on VMS for usage of MHpow().
**      31-mar-1993 (donj)
**          Add a new field to OPTION_LIST structure, arg_required. Used
**          for telling SEP_Cmd_Line() to look ahead for a missing argument.
**	05-apr-1993 (donj)
**	    Fixed outputDir problem. At end of septest when outputDir was
**	    just a directory, septool would try to delete the directory
**	    rather than the logfile.
**	23-apr-1993 (donj)
**	    Fixed somethings that Sun's acc compiler doesn't like.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 5-may-1993 (donj)
**	    getusr() prototype was wrong.
**	 7-may-1993 (donj)
**	    Adding more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**	19-may-1993 (donj)
**	    Fixed SEP_NMgtAt() to not give back *ptr == EOS just ptr == NULL.
**	    No need to check for *ptr == EOS. Changed MEtag in load_commands().
**	20-may-1993 (donj)
**	    Fix some embedded comments. They-re illegal in ansi C.
**	25-may-1993 (donj)
**	    Add a func declaration I missed. Put in a common msg[] char
**	    array. Put in more error checking. Add some more initializations.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h. Took out unused
**          arg from SEP_cmd_line.
**      14-aug-1993 (donj)
**          changed cmdopt from an array of structs to a linked list of structs.
**          added calls to SEP_alloc_Opt() to create linked list.
**      14-aug-1993 (donj)
**	    inadvertantly put GLOBALDEFS for option linked list heads in a VMS
**	    #ifdef.
**      16-aug-1993 (donj)
**          Reflect changes in SEP_Vers_Init(), *sep_version and *sep_platform.
**          Took out some magic numbers from testName,hdrName,testDev,testPath,
**          testPrefix,testSufix,testVersion. Added call to SIeqinit. Moved
**          SEP_alloc_Opt() as an experiment. Simplified the error handling in
**          load_commands(). Shortened some error messages.
**      26-aug-1993 (donj)
**          Fix a problem with abort_if_matched feature.
**	 9-sep-1993 (donj)
**	    Add an undefine_env_var() and fix the undo_env_chain function for
**	    a problem with traversing the list from the wrong direction. Also
**	    allow the same functionality for VMS. There's no danger since the
**	    DCL script handles the SEP_TABLE logical name table correctly.
**	23-sep-1993 (donj)
**	    Replace TDmvwprintw() with SEPprintw() that doesn't use varargs.
**	    might've been causing uninitialized memory reads.
**	11-oct-1993 (donj)
**	    Fix bug 55563. "sep -s vma12.sep" would work but "sep -svma12.sep"
**	    would create a test with the name "newtest.sep". User's still like
**	    to do it this way.
**	12-oct-1993 (donj)
**	    Leave log file if test aborted.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional. Add GLOBALDEFS for TokenID
**	    masking bit masks, A_always_match and A_word_match.
**	14-dec-1993 (donj)
**	    Add GlobalDef of Status_Value to implement Status User Variable.
**	17-dec-1993 (donj)
**	    classify_cmmd() was shifting lineTokens[1] down to lowercase when it
**	    shouldn't've been. Surprised this hasn't been noticed before.
**	30-dec-1993 (donj)
**	    Split out numerous functions to separate files:
**            Disp_Line()              now in displine.c
**            Disp_Line_no_refresh()   now in displine.c
**            get_command()            now in getcmd.c 
**            next_record()            now in nextrec.c
**            set_sep_state()          now in setstate.c
**            shell_cmmd()             now in shellcmd.c
**            assemble_line()          now in assemble.c
**            break_line()             now in breakln.c
**            classify_cmmd()          now in classify.c
**	17-jan-1994 (donj)
**	    Removed unused constant USERNAME_MAX.
**      14-apr-1994 (donj)
**          Added testcase support.
**	20-apr-1994 (donj)
**	    Added Env. Var., SEP_TESTCASE, to control testcase functionality.
**	10-may-1994 (donj)
**	    Implemented Create_stf_file() and del_floc() functions where
**	    appropriate.
**	24-may-1994 (donj)
**	    Modularize main() to make more readable. Take out some unsed vars.
**	25-may-1994 (donj)
**	    Reconcile get_answer(), in sep, and get_answer2(), in listexec,
**	    into one root function SEP_Get_Answer() in file sep/getanswer.c
**	27-may-1994 (donj)
**	    Modularized main() more to improve readability. Split code out
**	    for the following functions:
**
**		SEP_Cre_comment_Huh()		Create_testname()
**		Get_testname()			SEP_dispatch_command()
**		SEP_handle_hdr_file()
**
**	08-jun-1994 (donj)
**	    Modify how SEP handles ferun to eliminate the need for a shell
**	    wrapper. Initially fix is for UNIX. VMS fix implies that SEP
**	    my need to create a DCL symbol to run the application as a
**	    foreign command. (VMSer's know what I'm talking about.).
**	23-jun-1994 (donj)
**	    Fixed a problem in main() with PCerr, wasn't being initialized
**	    correctly.
**	16-oct-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**      05-Feb-96 (fanra01)
**          Added conditional for using variables when linking with DLLs.
**	05-Mar-1999 (ahaal01)
**	    Moved "include ex.h" after "include mh.h".  The check for
**	    "ifdef EXCONTINUE" in ex.h does no good if sys/context.h
**	    (via mh.h) is included after ex.h.
**      13-Jun-2000 (mosjo01)
**          The history_time(timeStr) function had timeStr defined as
**          protected storage (on some platforms) causing SIdofrmt to fail
**          and inturn sep create/maintain would fail.
**          Redefined timeStr as a 12 byte array.
**	24-mar-1999 (mosjo01)
**	    Reposition ex.h further down #include stack to avoid _jmpbuf error,
**	    occuring on AIX rs4_us5.
**      13-Jun-2000 (mosjo01)
**          The history_time(timeStr) function had timeStr defined as
**          protected storage (on some platforms) causing SIdofrmt to fail
**          and inturn sep create/maintain would fail.
**          Redefined timeStr as a 12 byte array.
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	08-sep-2003 (abbjo03)
**	    Change to build with OS headers provided by HP.   
**	21-Jun-2005 (bonro01)
**	    Added SEP_CMD_SLEEP to control sleep between sep commands.
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
**      25-Mar-2009 (drivi01)
**          Require this process to run as elevated.
**          If this tools isn't elevated, exit.
*/

#include <compat.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <mh.h>
#include <ex.h>
#include <ep.h>
#include <nm.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <tc.h>
#include <te.h>
#include <termdr.h>
#include <ex.h>

#ifdef VMS
#undef main
#endif

#define septool_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <seperrs.h>
#include <fndefs.h>
#include <token.h>

/*
**	Global variables
*/
GLOBALDEF    WINDOW       *mainW = NULL ;
GLOBALDEF    WINDOW       *statusW = NULL ;
GLOBALDEF    WINDOW       *coverupW = NULL ;
GLOBALDEF    i4            fromLastRec = 0 ;

GLOBALDEF    char          holdBuf [SCR_LINE] ;
GLOBALDEF    char          buffer_1 [SCR_LINE] ;
GLOBALDEF    char          buffer_2 [SCR_LINE] ;
GLOBALDEF    char         *lineTokens [SCR_LINE] ;
GLOBALDEF    char          msg [TEST_LINE] ;

GLOBALDEF    char         *ErrC       = ERx("ERROR: Could not ") ;
GLOBALDEF    char         *ErrOpn     = ERx("ERROR: Could not open ") ;
GLOBALDEF    char         *ErrF       = ERx("ERROR: failed while ") ;
GLOBALDEF    char         *creloc     = ERx("create location for ") ;
GLOBALDEF    char         *lookfor    = ERx("looking for ") ;
GLOBALDEF    char         *rbanner    = ERx("====  RESULT FILE  ====") ;
GLOBALDEF    char         *rbannerend = ERx("====  RESULT FILE END  ====") ;
GLOBALDEF    char         *Con_with   = ERx("Connection with ") ;

GLOBALDEF    ENV_VARS     *env_chain = NULL ;
GLOBALDEF    i4            sep_state ;
GLOBALDEF    i4            sep_state_old ;
GLOBALDEF    bool          ignore_silent = FALSE ;
GLOBALDEF    bool          abort_if_matched = FALSE ;

GLOBALDEF    FILLSTACK    *fill_stack      = NULL ;
GLOBALDEF    FILLSTACK    *fill_stack_root = NULL ;
GLOBALDEF    FILLSTACK    *fill_temp       = NULL ;


GLOBALDEF    char          kfebuffer [TERM_LINE] ;
GLOBALDEF    char         *kfe_ptr = kfebuffer ;

/*
**	GLOBALDEFS for the GLOBALREFS in sepparam.h
*/

GLOBALDEF    CMMD_NODE    *sepcmmds = NULL ;
GLOBALDEF    bool          shellMode = FALSE ;      /* test building mode     */
GLOBALDEF    bool          batchMode = FALSE ;      /* batch mode	      */
GLOBALDEF    bool          updateMode = FALSE ;     /* maintenance mode	      */
GLOBALDEF    bool          verboseMode = FALSE ;    /* verbose mode	      */
GLOBALDEF    bool          useSepTerm = TRUE ;      /* Use a "basic" terminal
						    ** type		      */
GLOBALDEF    char          terminalType [MAX_LOC] ; /* terminal type	      */
GLOBALDEF    bool          editHeader = TRUE ;      /* Call editor on test head-
						    ** er		      */
GLOBALDEF    bool          ignoreCase = FALSE ;     /* ignore case while diff-
						    ** ing		      */
GLOBALDEF    bool          diff_numerics = FALSE ;  /* diff floats and integers
                                                    ** as numbers rather than
                                                    ** strings.               */
GLOBALDEF    bool          grammar_methods = GRAMMAR_LEX;
						    /* use lex code for token-
						    ** izing grammar.         */
GLOBALDEF    f8            fuzz_factor = 0 ;        /* precision of match for */
GLOBALDEF    int           fuzz_int    = 0 ;        /* diff numerics.         */
GLOBALDEF    int           diffSpaces = 0 ;	    /* diff white space char-
						    ** acters		      */
GLOBALDEF    i4            diffLevel = 40 ;	    /* max. tokens that might
						    ** diffed		      */
GLOBALDEF    i4            testLevel = 3 ;	    /* max. commands that might
						    ** diffed		      */
GLOBALDEF    i4            verboseLevel = 0 ;       /* How much verbosity.    */
GLOBALDEF    i4            testGrade = 0 ;	    /* no. of commands that 
						    ** diffed		      */
GLOBALDEF    LOCATION     *SED_loc = NULL ;         /* pointer for SED file   */
GLOBALDEF    char          canonAns = EOS ;         /* default answer for make
						    ** canon question	      */
GLOBALDEF    char          editCAns = EOS ;         /* default answer for edit
						    ** canon question	      */
GLOBALDEF    char         *outputDir = NULL ;       /* directory for SEP output
						    ** file		      */
GLOBALDEF    LOCTYPE       outputDir_type ;	    /* what type is outputDir */
GLOBALDEF    LOCATION      outLoc ;                 /* output file LOCATION   */
GLOBALDEF    bool          outLoc_exists ;
GLOBALDEF    i4            waitLimit = 300 ;	    /* seconds to wait before
						    ** timing out	      */
GLOBALDEF    char          real_editor [MAX_LOC] ;  /* editor for SEP	      */
GLOBALDEF    char          cont_char[3] = {'-',EOS};/* Continuation character */
GLOBALDEF    bool          in_canon = FALSE ;       /* Are we trying to read a
						    ** canon?		      */

GLOBALDEF    i4            if_level [15] ;	    /* 15 levels shud be more */
GLOBALDEF    i4            if_ptr = 0 ;		    /* than enough.           */

GLOBALDEF    bool          paging = TRUE ;
GLOBALDEF    bool          paging_old = TRUE ;

GLOBALDEF    i4            tracing = 0 ;
GLOBALDEF    i4            trace_nr = 0 ;
GLOBALDEF    FILE         *traceptr = NULL ;
GLOBALDEF    char         *trace_file = NULL ;
GLOBALDEF    LOCATION      traceloc ;

GLOBALDEF    char          userName [20] ;
GLOBALDEF    char          fuzz_str [30] ;

GLOBALDEF    char          old_editor [MAX_LOC] ;   /* old value of EDITOR    */
GLOBALDEF    char          old_visual [MAX_LOC] ;   /* old value of VISUAL    */
GLOBALDEF    char          old_ing_edit [MAX_LOC] ; /* old value of ING_EDIT  */

GLOBALDEF    char         *open_args = NULL ;
GLOBALDEF    char         *close_args = NULL ;
GLOBALDEF    char         *open_args_array [WORD_ARRAY_SIZE] ;
GLOBALDEF    char         *close_args_array [WORD_ARRAY_SIZE] ;

GLOBALDEF    char         *SEP_IF_EXPRESSION = NULL ;
GLOBALDEF    char         *SEP_TRACE = NULL ;
GLOBALDEF    char         *SEP_SET = NULL ;
GLOBALDEF    char         *SEP_TIMEOUT = NULL ;
GLOBALDEF    char         *SEP_TESTCASE = NULL ;

GLOBALDEF    long          A_always_match = A_ALWAYS_MATCH ;
GLOBALDEF    long          A_word_match   = A_WORD_MATCH ;

GLOBALDEF    i4           diff_sleep;
GLOBALDEF    i4           cmd_sleep;

/*
**  handling of TC connections
*/
GLOBALDEF    bool          SEPdisconnect = FALSE ;  /* user requested
						    ** disconnection	      */
GLOBALDEF    i4            SEPtcsubs = 0 ;	    /* number of active INGRES
						    ** systems		      */
GLOBALDEF    STATUS        SEPtclink = SEPOFF ;     /* status of TC connec-
						    ** tion		      */
GLOBALDEF    PID           SEPsonpid ;		    /* PID of INGRES system   */
GLOBALDEF    PID           SEPpid ;                 /* SEP's PID              */
GLOBALDEF    char          SEPpidstr [6] ;          /* SEP's PID as a string. */
GLOBALDEF    i4            Status_Value = 0 ;       /* Return status from SEP-*/
                                                    /* spawn. Used in User Var*/
                                                    /* Status.                */
/*
**  TC files
*/
GLOBALDEF    TCFILE       *SEPtcinptr = NULL ;
GLOBALDEF    TCFILE       *SEPtcoutptr = NULL ;
GLOBALDEF    LOCATION      SEPtcinloc ;
GLOBALDEF    LOCATION      SEPtcoutloc ;
GLOBALDEF    char          SEPinname [MAX_LOC] ;
GLOBALDEF    char          SEPoutname [MAX_LOC] ;

/*
**  handling of temporary files
*/
GLOBALDEF    SEPFILE      *sepResults = NULL ;
GLOBALDEF    SEPFILE      *sepCanons = NULL ;
GLOBALDEF    SEPFILE      *sepGCanons = NULL ;
GLOBALDEF    SEPFILE      *sepDiffer = NULL ;

GLOBALDEF    LOCATION      SEPresloc ;
GLOBALDEF    LOCATION      SEPcanloc ;

GLOBALDEF    char         *cmtFile = NULL ;
GLOBALDEF    LOCATION     *cmtLoc  = NULL ;
GLOBALDEF    char         *cmtName = NULL ;

GLOBALDEF    char          tcName [MAX_LOC] ;


#ifdef VMS

FUNC_EXTERN  char         *getusr () ;
FUNC_EXTERN  i4            SEPexit ( i4 status ) ;
FUNC_EXTERN  i4            setusr ( char *future_user ) ;

GLOBALDEF    bool          fromListexec = FALSE ;
GLOBALDEF    char          listexecKey [MAX_LOC] ;
GLOBALDEF    char         *Original_User = NULL ;
GLOBALDEF    char         *New_User = NULL ;

#define      SEP_ME_ALLOC  ME_INGRES_ALLOC
#define      PCexit        SEPexit
#define      USER_EDITOR   ERx("EDIT")

#else

#define      SEP_ME_ALLOC  ME_USER_ALLOC
#define      USER_EDITOR   ERx("/bin/ed")

#endif /* VMS */

GLOBALDEF    OPTION_LIST  *cmdopt = NULL ;
GLOBALDEF    OPTION_LIST  *open_canon_opt = NULL ;
GLOBALDEF    OPTION_LIST  *close_canon_opt = NULL ;

#if defined(UNIX) || defined(NT_GENERIC)
# if defined(IMPORT_DLL_DATA)
GLOBALDLLREF    bool          IITCmaster ;
# else
GLOBALREF    bool          IITCmaster ;
# endif
#endif /* UNIX || NT_GENERIC */

/*
** SEP version number. This string is displayed when SEP is started in
** non-batch mode.
*/
GLOBALDEF    char         *sep_version = NULL ;
GLOBALDEF    char         *sep_platform = NULL ;

/* char                      *timeStr = ERx("xx-xxx-xxxx"); */
char			   timeStr[12];

/* pointer to legal commands b-tree */
GLOBALDEF    i4            testLine = 0 ;
GLOBALDEF    i4            screenLine = 0 ;

/* boolean to check if TERM_INGRES is to be redefined */
bool changeTIngres = FALSE;

/* routine for handling of exceptions */

static       EX            main_exhandler () ;

static char               *Illegal_param = ERx("ERROR:Illegal parameter = %s") ;
static char               *Illegal_flag  = ERx("ERROR:Illegal flag for parameter = %s") ;
static char               *script_must_be_given = ERx("ERROR: test script name must be given if working %s mode") ;
static char               *eq_sign = ERx("=") ;
static char               *s_quote = ERx("'") ;

/* 
**      ming hints 
** 
PROGRAM = sep

NEEDLIBS = SEPLIB SEPDIFFLIB SEPCLLIB LIBINGRES 

** Note: To use sepwin rather than ft and ug, change FTLIB and UGLIB 
**       above to SEPWINLIB.
*/
#define	    BLANKLINE	-9900
    FILE                  *commentPtr = NULL ;
    char                   testName [MAX_LOC] ;
    char                   tstseppath [MAX_LOC] ;
    bool                   TDwin_open = FALSE ;

    VOID                   Refresh_windows          ( bool *b_mode,
						      WINDOW *main_w,
						      WINDOW *stat_w,
						      bool fixclr ) ;

main(i4 argc,char *argv[])
{
    VOID                   SEP_save_edtr_vars       () ;
    VOID                   SEP_Cre_comment_Huh      ( WINDOW *main_w,
						      WINDOW *stat_w ) ;
    VOID                   Create_testname          ( char *tname,
						      char *tprefix,
						      char *tsuffix ) ;
    STATUS                 Get_testname             ( FILE **tfile,
						      char *tname,
						      char *tprefix,
						      char *tsuffix,
						      i4   *ret_nat ) ;
    STATUS                 SEP_dispatch_command     ( FILE *tfile,
						      i4   cmd,
						      char **tokens ) ;

    i4                     SEP_handle_hdr_file      ( char *hname,
						      LOCATION *hloc,
						      char *hfile,
						      WINDOW *main_w,
						      WINDOW *stat_w,
						      FILE *tfile,
						      char *errmsg ) ;
    i4                     SEP_main_init            ( char *errmsg ) ;
    i4                     SEP_setup_termcap        ( char *ttyType,
						      char *tst_sep,
						      char *errmsg ) ;
    i4                     SEP_copy_header_template ( char **hdr_name,
						      LOCATION **hdr_loc,
						      char *tst_sep,
						      char **hdr_file,
						      char *errmsg ) ;
    i4                     SEP_rpt_PCerr            ( char *errmsg,
						      bool *win_open,
						      i4  err ) ;
    LOCATION              *hdrLoc = NULL ;

    STATUS                 ioerr ;
    STATUS                 ex_val ;
    EX_CONTEXT             ex ;

    FILE                  *testFile = NULL ;

    char                  *cptr = NULL ;
    char                  *hdrName = NULL ;
    char                  *hdrfile = NULL ;
    char                   testPrefix  [LO_FPREFIX_MAX+1] ;
    char                   testSufix   [LO_FSUFFIX_MAX+1] ;

    i4                     cmd_type ;
    i4                     PCerr ;

    /* *************************************************************
    ** Check if we're on Vista and running with a stripped token
    */
    ELEVATION_REQUIRED();

    /* *************************************************************
    **  Do required MEadvise() call before anything else.
    */

    if ((PCerr = MEadvise(SEP_ME_ALLOC)) != OK)
    {
	STprintf(buffer_1, ERx("ME initialization Error <%d>\n"),PCerr);
	PCerr = SEPerr_SEPfileInit_fail;
    }

    /* *********************************************************
    **  open STDIN & STDOUT so we can at least print an error msg.
    */

    if (SIeqinit() != OK)
    {
        PCexit(SEPerr_SEPfileInit_fail);
    }

    /* *********************************************************
    ** If no errors, do a large amount of initialization.
    */

    PCerr = (PCerr != OK) ?  PCerr : SEP_main_init( buffer_1 );

    /* *********************************************************
    ** If no other errors, Get command line arguments.
    */

    *testName = EOS;
    PCerr = (PCerr != OK) ?  PCerr
	  : ( get_args(argc, argv, testName) != OK) ? SEPerr_bad_cmdline : OK ;

    /* *********************************************************
    ** If no errors, set termcap stuff.
    */

    PCerr = (PCerr != OK) ?  PCerr
	  : SEP_setup_termcap( terminalType, tstseppath, buffer_1 );

    /* *********************************************************
    ** If no errors, create the header for a new test.
    */

    PCerr = (PCerr != OK) ?  PCerr
	  : SEP_copy_header_template( &hdrName, &hdrLoc, tstseppath,
				      &hdrfile, buffer_1 );

    /* *********************************************************
    ** If no errors, Open results, canons, and differ files
    */

    PCerr = (PCerr != OK) ?  PCerr
	  : (SEPfilesInit(SEPpidstr, buffer_1) != OK)
	    ? SEPerr_SEPfileInit_fail : OK;

    /* *********************************************************
    ** If there are any errors, print the errmsg and get out.
    */

    if (PCerr != OK)
    {
	PCexit(SEP_rpt_PCerr(buffer_1, &TDwin_open, PCerr));
    }

    (VOID)SEP_save_edtr_vars();

    /* *******************************************************************
    ** Open main window and status window, same approach as INGRES, a main
    ** window to display information, and a status window to interact with
    ** the user through prompts and messages.
    */

    if (!batchMode)
    {
#ifdef UNIX
	initSigHandler();
#endif
	mainW   = TDnewwin(MAIN_ROWS,MAIN_COLS,FIRST_MAIN_ROW,FIRST_MAIN_COL);
	statusW = TDnewwin(STATUS_ROWS+1,STATUS_COLS,FIRST_STAT_ROW,
			   FIRST_MAIN_COL);
	disp_line(sep_version,0,0);
	PCsleep(2000);
    }

    /* *************************************************************
    ** Open test script to be processed. If test name wasn't given in 
    ** command line, a prompt asking for it will be displayed.
    ** If working in shell mode, SEP will assume that is working in a
    ** test named 'newtest.sep'
    */

    if (shellMode)
    {
	/*
	** Create Test name for shell mode.
	*/
	(VOID)Create_testname( testName, testPrefix, testSufix );
    }
    else if (Get_testname( &testFile, testName, testPrefix, testSufix,
			   &PCerr ) != OK)
    {
	SEPfilesTerminate();
	exiting((i4)PCerr, (bool)TRUE);
    }
/*
** *************************************************************
**    Open log file. This file will record the following information:
**    a) results from test run if running a test
**    b) new test if creating a test
**    c) updated test if modifiying an existing test
*/
    if (open_log(testPrefix, testSufix, userName, buffer_1) != OK)
    {
	PCerr = SEPerr_LogOpn_fail;
    }
/*
** *************************************************************
**    fill header file and append it to the log if making a new test
*/
    PCerr = (PCerr != OK) ? PCerr
	  : SEP_handle_hdr_file( hdrName, hdrLoc, hdrfile, mainW, statusW, testFile,
				 buffer_1 );

    if (PCerr)
    {
	PCexit(SEP_rpt_PCerr(buffer_1, &TDwin_open, PCerr));
    }
/*
** *************************************************************
**    Initialize testcase logging
*/
    testcase_startup();
/*
** *************************************************************
**    setup exception handling
*/

    ex_val = OK;
    if (EXdeclare(main_exhandler, &ex) != OK)
	ex_val = FAIL;

    if (ex_val == OK)
    {
      EXinterrupt(EX_RESET);
/*
** *************************************************************
**    Read commands from test script, or prompt for them to the user
**    if working in shell mode.
*/
      do
      {
	/* get next command */

	(VOID)Refresh_windows(&batchMode, mainW, statusW, FALSE);

	switch (ioerr = shellMode ? shell_cmmd(SEP_PROMPT) : 
				    get_command(testFile,SEP_PROMPT,FALSE))
	{
	   case FAIL:
		disp_line(ERx("ERROR: failed while reading test script"),0,0);
		break;

	   case ENDFILE:
		if (!(shellMode || batchMode))
		    disp_line(ERx("EOF reached for test script"),0,0);
		break;

	   case BLANKLINE:  /* capture blank line here rather than in default */
		break;

	   default:	/* check command just read */

		/* set connection flags	*/
		SEPtclink = SEPOFF;
		SEPdisconnect = FALSE;
		SEPtcsubs = 0;

		/* classify command	*/
		if ((cmd_type = classify_cmmd(sepcmmds, lineTokens[1]))
		     == UNKNOWN_CMMD)
		{
		    get_answer(ERx("ERROR: unknown command...[HIT RETURN]"),
				buffer_1);
		    continue;
		}

		/* display command in main screen */
		if(!batchMode)
		{
		    disp_line(SEP_PROMPT,0,0);
		    disp_line(buffer_1,screenLine-1,2);
		}

		/* append command to log file */
		switch (cmd_type)
		{
		    case IF_CMMD:
		    case ELSE_CMMD:
		    case ENDIF_CMMD:
				append_line(CONTROL_PROMPT,0);
			    break;
		    default:
				append_line(SEP_PROMPT,0);
				append_line(ERx(" "),0);
			    break;
		}
		append_line(buffer_1,1);

		/* check if user wants to append a comment */

		SEP_Cre_comment_Huh(mainW, statusW);

		/* Do the command. */

		ioerr = SEP_dispatch_command( testFile, cmd_type, lineTokens );
	}
#ifdef VMS
	New_User = getusr();
        if (STbcompare(New_User,0,Original_User,0,TRUE) != 0)
	    setusr (Original_User);
#endif
      }
      while ((ioerr == OK) && testLevel);
    }

/* *******************************************************************
**			  Close up and say goodbye
** *******************************************************************
*/
    testcase_end(TRUE);

    if (SEP_TESTCASE != NULL)
    {
	testcase_writefile();
    }

    EXinterrupt(EX_OFF);    /* disable exceptions while getting out of here */
    if (!testLevel)	    /* Display Error Msg if tolerance was reached.  */
    {
	STprintf(buffer_1,
	    ERx("ERROR: Test execution stopped. Error tolerance exceeded."));

	if (!batchMode)
	    disp_line(buffer_1, 0,0);

	append_line(buffer_1, 1);
    }
    if (tracing)	    /* Close trace file if opened. */
    {
	if (tracing == TRACE_NONE)
	{
	    LOdelete(&traceloc);
	}
	SIclose(traceptr);
	tracing = FALSE;
    }

    if (!shellMode && (SIclose(testFile) != OK))
	get_answer(ERx("failed while closing test script...[HIT RETURN] "),
		   buffer_1);
    close_log();

    if ((testGrade == 0) &&
	!(verboseMode||shellMode||updateMode||(ioerr == ABORT_TEST)))
	del_log();

    if (!batchMode)
	disp_line(ERx("deleting temporary files ..."),0,0);

    SEPfilesTerminate();
    exiting(testGrade, (bool)FALSE);
    EXdelete();
    PCexit(testGrade);

} /* ************************** End of Main ************************** */

i4
SEP_main_init( char *errmsg )
{
    STATUS                 STSerr ;
    i4                     ret_nat = 0 ;
    char                  *cptr ;
    char                  *sleepptr;

#if defined(UNIX) || defined(NT_GENERIC)
    /* *************************************************************
    **  Work around a Unix INGRES bug in NMgtAt.
    **  It will dump core if II_SYSTEM is not set.
    */
    if (getenv(ERx("II_SYSTEM")) == NULL)
    {
	STcopy(ERx("II_SYSTEM is not defined."), errmsg);
	return (SEPerr_no_IISYS);
    }

    /* *************************************************************
    ** Set IITCmaster to TRUE, so TCclose will remove sync files.
    */
    IITCmaster = TRUE;
#endif

    /* *************************************************************
    **  Setup command line option list.
    */
    SEP_alloc_Opt(&cmdopt, ERx("batch"),
			   NULL,               /* char * loaded here */
			   &batchMode,         /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   FALSE               /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("case_ignore"),
			   NULL,               /* char * loaded here */
			   &ignoreCase,        /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   FALSE               /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("difflevel"),
			   NULL,               /* char * loaded here */
			   NULL,               /* bool   loaded here */
			   &diffLevel,         /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   TRUE                /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("edit_header"),
			   NULL,               /* char * loaded here */
			   &editHeader,        /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   FALSE               /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("fuzz_factor"),
			   fuzz_str,           /* char * loaded here */
			   &diff_numerics,     /* bool   loaded here */
			   &fuzz_int,          /* i4     loaded here */
			   &fuzz_factor,       /* float  loaded here */
			   TRUE                /* is option required */
			   );
#ifdef VMS
    SEP_alloc_Opt(&cmdopt, ERx("listexec"),
			   listexecKey,        /* char * loaded here */
			   &fromListexec,      /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   TRUE                /* is option required */
			   );
#endif
    SEP_alloc_Opt(&cmdopt, ERx("shell"),
			   testName,           /* char * loaded here */
			   &shellMode,         /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   FALSE               /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("testlevel"),
			   NULL,               /* char * loaded here */
			   NULL,               /* bool   loaded here */
			   &testLevel,         /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   TRUE                /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("username"),
			   userName,           /* char * loaded here */
			   NULL,               /* bool   loaded here */
			   NULL,               /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   TRUE                /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("verbose"),
			   NULL,               /* char * loaded here */
			   &verboseMode,       /* bool   loaded here */
			   &verboseLevel,      /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   FALSE               /* is option required */
			   );
    SEP_alloc_Opt(&cmdopt, ERx("wait"),
			   NULL,               /* char * loaded here */
			   NULL,               /* bool   loaded here */
			   &waitLimit,         /* i4     loaded here */
			   NULL,               /* float  loaded here */
			   TRUE                /* is option required */
			   );

    /* *************************************************************
    **    Check for value of environment value TST_SEP
    */

    SEP_NMgtAt(ERx("TST_SEP"), &cptr, SEP_ME_TAG_NODEL);
    if (cptr == NULL)
    {
	STcopy(ERx("ERROR: environment variable TST_SEP is not defined."),
	       errmsg);
	return (SEPerr_No_TST_SEP);
    }
    STcopy(cptr, tstseppath);
    MEfree(cptr);

    /* *************************************************************
    **    get process PID
    */
    PCpid(&SEPpid);
    CVla((SEPpid & 0x0ffff), SEPpidstr);

#ifdef VMS
    /* *************************************************************
    **    get Username
    */
    Original_User = getusr();
#endif

    /* *************************************************************
    **    Check for value of environment variables
    */
    SEP_NMgtAt(ERx("SEP_IF_EXPRESSION"), &SEP_IF_EXPRESSION, SEP_ME_TAG_NODEL);
    SEP_NMgtAt(ERx("SEP_TRACE"),         &SEP_TRACE,         SEP_ME_TAG_NODEL);
    SEP_NMgtAt(ERx("SEP_SET"),           &SEP_SET,           SEP_ME_TAG_NODEL);
    SEP_NMgtAt(ERx("SEP_TIMEOUT"),       &SEP_TIMEOUT,       SEP_ME_TAG_NODEL);
    SEP_NMgtAt(ERx("SEP_TESTCASE"),      &SEP_TESTCASE,      SEP_ME_TAG_NODEL);

    diff_sleep = ( (sleepptr = getenv("SEP_DIFF_SLEEP")) == NULL ) ? 250 : \
                                                atoi(sleepptr);
    cmd_sleep = ( (sleepptr = getenv("SEP_CMD_SLEEP")) == NULL ) ? 250 : \
                                                atoi(sleepptr);

    /* *************************************************************
    **  Check sanity of some of the environment variables.
    ** *************************************************************
    **  Check SEP_TIMEOUT
    */
    if (SEP_TIMEOUT != NULL)
    {
	i4                    timout_ ;

	if ((STSerr = CVan(SEP_TIMEOUT,&timout_)) == OK)
	    waitLimit = timout_;
	else
	{
	    switch(STSerr)
	    {
	       case CV_SYNTAX:

			SIputrec(ERx("ERROR: illegal format "), stderr);
			break;

	       case CV_OVERFLOW:

			SIputrec(ERx("ERROR: overflow "), stderr);
			break;
	       default:
			SIfprintf(stderr, ERx("UNKNOWN ERROR: %d, "), STSerr);
	    }
	    SIputrec(ERx("in SEP_TIMEOUT definition.\n"),stderr);
	    SIputrec(ERx("   leaving SEP Timeout value unchanged.\n"), stderr);
	}

	MEfree(SEP_TIMEOUT);
    }
    /*
    ** *************************************************************
    **  Check SEP_TRACE
    */
    if (SEP_TRACE != NULL)
    {
	if (STbcompare(SEP_TRACE,0,ERx("TRUE"),0,TRUE) == 0)
	{
	    SEP_Trace_Open();
	    SEP_Trace_Set(ERx("ALL"));
	}
	else if (STbcompare(SEP_TRACE,0,ERx("FALSE"),0,TRUE) != 0)
	{
	    SEP_Trace_Open();
	    SEP_Trace_Set(SEP_TRACE);
	}

        MEfree(SEP_TRACE);
    }

    /*
    ** *************************************************************
    **  Check SEP_SET
    */
    if (SEP_SET != NULL)
    {
	SEP_SET_read_envvar(SEP_SET);
	MEfree(SEP_SET);
    }

    (VOID)SEP_Vers_Init(ERx("SEP"), &sep_version, &sep_platform);

    return (ret_nat);
}

i4
SEP_setup_termcap( char *ttyType, char *tst_sep, char *errmsg )
{
    i4                     ret_nat = 0 ;

    char                   termcapDir [MAX_LOC] ;
    char                  *termcapName = NULL ;

    LOCATION               termcapLoc ;

    /* 
    ** Find out terminal type user wants to use. If user wants to 
    ** work in terminal other than the one specified by TERM_INGRES 
    ** redefine the appropiate logicals
    */

    if (useSepTerm)
    {

	/* **************************************************
	** This is a terminal type used only by SEP. We need
	** to redefine both II_TERMCAP_FILE & TERM_INGRES.
	** **************************************************
	*/
	STcopy(ERx("septerm"), ttyType);
	STcopy(tstseppath, termcapDir);
	LOfroms(PATH, termcapDir, &termcapLoc);
	LOfstfile(ERx("termcap.sep"), &termcapLoc);
	LOtos(&termcapLoc, &termcapName);
	define_env_var(ERx("II_TERMCAP_FILE"), termcapName);
	define_env_var(ERx("TERM_INGRES"), ERx("septerm"));
    }
    else if (changeTIngres)
    {
	define_env_var(ERx("TERM_INGRES"), ttyType);
    }

    /*
    ** Read termcap file using TD routines
    */

    if (!batchMode)
    {
	TDinitscr();
	TDwin_open = TRUE;
    }

    /* *********************************************************
    ** load sep commands info and sep termcap info.
    ** *********************************************************
    */
    ret_nat = (load_commands(tst_sep, errmsg) != OK)
	    ? SEPerr_cmd_load_fail : OK;

    ret_nat = (ret_nat != OK) ? ret_nat
	    : (load_termcap(tst_sep, errmsg)  != OK)
	      ? SEPerr_trmcap_load_fail : OK;

    return (ret_nat);
}

i4
SEP_copy_header_template ( char **hdr_name, LOCATION **hdr_loc, char *tst_sep,
			   char **hdr_file, char *errmsg )
{
    STATUS                 STSerr ;
    i4                     ret_nat = 0 ;

    LOCATION               tmp_hloc ;

    FILE                  *hdrPtr = NULL ;

    char                  *cptr ;

    if (shellMode||updateMode)
	history_time(&timeStr);

    if (!shellMode)
	return (ret_nat);

    /* create header file for new test in current directory */

    STSerr = Create_stf_file(SEP_ME_TAG_NODEL,ERx("hdr"),hdr_name,hdr_loc,msg);
    STSerr = ( STSerr == OK ) ? SIopen(*hdr_loc, ERx("w"), &hdrPtr) : STSerr;

    if (STSerr != OK) 
    {
        STprintf(errmsg, ERx("%sheader file for new test."), ErrOpn);
	ret_nat = SEPerr_OpnHdr_fail;
    }
    else
    {
        /* open template in TST_SEP directory */
    
        LOfroms(PATH, tst_sep, &tmp_hloc);
        LOfstfile(ERx("header.txt"), &tmp_hloc);
        LOtos(&tmp_hloc, &cptr);
        *hdr_file = (char *)SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE,
                                        (STATUS *)NULL);
        STcopy(cptr, *hdr_file);
        LOfroms(FILENAME & PATH,*hdr_file,&tmp_hloc);
    
        /* copy template */
    
        if (SIcat(&tmp_hloc,hdrPtr) != OK)
        {
            STprintf(errmsg, ERx("%scopy header file."), ErrC);
            ret_nat = SEPerr_CpyHdr_fail;
        }
        else
        {
            SIfprintf(hdrPtr, ERx(" History: %s\t(Author)\tCreated\n*/\n"),
                      &timeStr);
            SIclose(hdrPtr);
        }
    }

    if (ret_nat)
    {
	TDendwin();
	TEclose();
    }

    return (ret_nat);
}

i4
SEP_rpt_PCerr ( char *errmsg, bool *win_open, i4  err )
{
    if (err != OK)
    {
	SIputrec(errmsg, stdout);
	SIputrec(ERx("\n\n\n"), stdout);
	if (tracing)
	{
	     SIfprintf(traceptr, ERx("SEP_rpt_PCerr> %s\n"), errmsg);
	     SIflush(traceptr);
	     SIclose(traceptr);
	}
	if ((win_open != NULL)&&(*win_open))
	{
	     TDendwin();
	     TEclose();
	     *win_open = FALSE;
	}
    }
    return (err);
}

VOID
SEP_save_edtr_vars ()
{
    char                  *e = NULL ;
    /* 
    ** **********************************************************************
    ** Save the current ING_EDIT, because we'll be modifying it later for the
    ** pseudo-editor.
    */

   NMgtAt(ERx("ING_EDIT"), &e);
   if ((e == NULL) || (*e == EOS))
   {
	e = USER_EDITOR;
#ifdef UNIX
	*old_ing_edit = EOS;
#endif
   }
   STcopy(e, real_editor);
#ifdef UNIX
    STcopy(e, old_ing_edit);
    NMgtAt(ERx("VISUAL"), &e);
    if ((e == NULL) || (*e == EOS))
	*old_visual = EOS;
    else
	STcopy(e, old_visual);
    NMgtAt(ERx("EDITOR"), &e); 
    if ((e == NULL) || (*e == EOS))
        *old_editor = EOS; 
    else 
        STcopy(e, old_editor);
#endif
}

VOID
SEP_Cre_comment_Huh( WINDOW *main_w, WINDOW *stat_w )
{
    if (shellMode && canonAns != EOS)
    {
	char               answer ;

	disp_prompt(ERx("Create comments for this command? (y/n)"), &answer,
		    ERx("YyNn"));

	if (answer == 'y' || answer == 'Y')
	{
	    if (bld_cmtName(msg) != OK)
	    {
		    disp_line(msg,0,0);
	    }
	    else
	    {
		edit_file(cmtName);
		if (SEP_LOexists(cmtLoc))
		{
		    append_line(OPEN_COMMENT, 1);
		    append_file(cmtName);
		    append_line(CLOSE_COMMENT, 1);
		    del_floc(cmtLoc);
		}
	    }
	}
	(VOID)Refresh_windows(&batchMode, main_w, stat_w, TRUE);
    }
}

STATUS
SEP_dispatch_command( FILE *tfile, i4  cmd, char **tokens )
{
    STATUS                 ret_status = OK ;

    switch (cmd)
    {
        case OSTCL_CMMD:

	    ret_status  = process_ostcl(tfile, tokens[1]);
	    sep_state &= (~IN_OS_STATE);
	    break;

        case FORMS_CMMD:
	case FERUN_CMMD:

	    paging_old = paging;
	    paging = FALSE;
	    ret_status = process_fe(tfile,
				    tokens[
#ifdef UNIX
				           (cmd == FORMS_CMMD)?1:2
#else
				           1
#endif
				             ]);
	    paging = paging_old;
	    sep_state &= (~IN_FE_STATE);

	    if (!(batchMode||(ret_status == ABORT_TEST)))
	    {
	        disp_line(buffer_1, 0,0);
	    }

	    if (shellMode)
	    {
	        show_cursor();
	    }

	    break;

        case TM_CMMD:

	    if (((ret_status = process_tm(tfile, tokens[1])) != OK)&&
	        (ret_status != ABORT_TEST))
	    {
	        append_line(buffer_1, 1);
	    }

	    sep_state &= (~IN_TM_STATE);

	    if (!(batchMode||(ret_status == ABORT_TEST)))
	    {
	        disp_line(buffer_1, 0,0);
	    }

	    break;

        case IF_CMMD:
        case ELSE_CMMD:
        case ENDIF_CMMD:

	    ret_status = process_if(tfile, tokens, cmd);

	    if (!batchMode)
	    {
                disp_line(buffer_1, 0,0);
	    }

	    break;

        case TEST_CASE_CMMD:

	    ret_status = testcase_start(&tokens[1]);
	    break;

        case TEST_CASE_END_CMMD:

	    ret_status = testcase_end(NULL);
	    break;

        default:

	    ret_status = sep_ostcl(tfile, &tokens[1], cmd);
	    sep_state &= (~IN_OS_STATE);
	    break;
    }

    return (ret_status);
}

i4
SEP_handle_hdr_file( char *hname, LOCATION *hloc, char *hfile, WINDOW *main_w,
		     WINDOW *stat_w, FILE *tfile, char *errmsg )
{
/*
** *************************************************************
**    fill header file and append it to the log if making a new test
*/
    if (shellMode)
    {
	if (editHeader)
	{
	    edit_file(hname);
	    append_file(hname);
	    (VOID)Refresh_windows(&batchMode, main_w, stat_w, TRUE);
	    del_floc(hloc);
	    MEfree(hfile);
	}
	else
	{
	    append_file(hname);
	    del_floc(hloc);
	}

	MEfree(hname);
	MEfree(hloc);
    }

    return (updateMode ? copy_hdr(tfile, errmsg) : 0);
}

VOID
Refresh_windows( bool *b_mode, WINDOW *main_w, WINDOW *stat_w, bool fixclr )
{
    if ((b_mode != NULL)&&(*b_mode == FALSE))
    {
	if (fixclr)
	{
	    fix_cursor();
	    TDclear(curscr); /* curscr is GLOBALREF'd in termdr.h */
	}

	if (main_w != NULL)
	{
	    TDtouchwin(main_w);
	    TDrefresh(main_w);
	}

	if (stat_w != NULL)
	{
	    TDtouchwin(stat_w);
	    TDrefresh(stat_w);
	}
    }
}

STATUS
Get_testname( FILE **tfile, char *tname, char *tprefix, char *tsuffix, i4  *ret_nat )
{
    STATUS                 ioerr ;
    LOCATION              *tloc ;
    char                  *tdev ;
    char                  *tpath ;
    char                  *tvers ;

    *ret_nat = 0;

    tloc = (LOCATION *)
	   SEP_MEalloc(SEP_ME_TAG_MISC, sizeof(LOCATION), TRUE, (STATUS *)NULL);

    tdev = (char *)
	   SEP_MEalloc(SEP_ME_TAG_MISC, LO_DEVNAME_MAX+1, TRUE, (STATUS *)NULL);

    tpath = (char *)
	    SEP_MEalloc(SEP_ME_TAG_MISC, LO_PATH_MAX+1, TRUE, (STATUS *)NULL);

    tvers = (char *)
	    SEP_MEalloc(SEP_ME_TAG_MISC, LO_FVERSION_MAX+1, TRUE, (STATUS *)NULL);

    if (*tname == EOS)
    {
	get_answer(ERx("Enter name of test script: "), tname);
    }

    while ((ioerr = STbcompare(ERx("exit"), 4, tname,
			       SEP_CMstlen(tname,0), TRUE)) != 0)
    {
	if ((ioerr = LOfroms(FILENAME & PATH,tname,tloc)) == OK)
	{
            LOdetail(tloc, tdev, tpath, tprefix, tsuffix, tvers);
	    if (!SEP_LOexists(tloc))
	    {
		ioerr = FAIL;
		if (tsuffix[0] == EOS && tvers[0] == EOS)
		{
		    STcat(tname, ERx(".sep"));
		    continue;
		}

		if (!batchMode)
		{
			STprintf(buffer_2,
			ERx("test script \"%s\" was not found...[HIT RETURN]"),
				 tname);
			get_answer(buffer_2,buffer_1);
		}
		else
		{
#ifdef UNIX
			SIprintf(ERx("test script \"%s\" was not found..."),
				 tname);
#endif
			*ret_nat = SEPerr_Cant_find_test;
			ioerr = 0;
			break;
		}
	    }
	    else if ((ioerr = SIopen(tloc,ERx("r"),tfile)) != OK)
	    {
		get_answer(ERx("test script couldn't be opened..[HIT RETURN] "),
			   buffer_1);
	    }
	    else
	    {
			ioerr = 1;
			break;
	    }
	}
	else
	{
	    get_answer(
	  ERx("could not create location for test script file...[HIT RETURN] "),
		       buffer_1);
	}
	get_answer(ERx("Enter name of test script: "),tname);
    }

    MEfree(tloc);
    MEfree(tdev);
    MEfree(tpath);
    MEfree(tvers);

    return ((ioerr == 1) ? OK : (ioerr == 0) ? FAIL : ioerr);
}

VOID
Create_testname( char *tname, char *tprefix, char *tsuffix )
{
    if (*tname == EOS)
    {
	STcopy(ERx("newtest.sep"),tname);
	STcopy(ERx("newtest"), tprefix);
	STcopy(ERx("sep"), tsuffix);
    }
    else
    {
	LOCATION          *tloc ;
	char              *tdev ;
	char              *tpath ;
	char              *tvers ;

	tloc = (LOCATION *)
	       SEP_MEalloc(SEP_ME_TAG_MISC, sizeof(LOCATION), TRUE, (STATUS *)NULL);
	tdev = (char *)
	       SEP_MEalloc(SEP_ME_TAG_MISC, LO_DEVNAME_MAX+1, TRUE, (STATUS *)NULL);

	tpath = (char *)
		SEP_MEalloc(SEP_ME_TAG_MISC, LO_PATH_MAX+1, TRUE, (STATUS *)NULL);

	tvers = (char *)
		SEP_MEalloc(SEP_ME_TAG_MISC, LO_FVERSION_MAX+1, TRUE, (STATUS *)NULL);

	tsuffix[0] = EOS;
	LOfroms(FILENAME & PATH, tname, tloc);
	LOdetail(tloc, tdev, tpath, tprefix, tsuffix, tvers);
	if (tsuffix[0] == EOS)
	{
	    STcopy(ERx("sep"), tsuffix);
	}
	MEfree(tloc);
	MEfree(tdev);
	MEfree(tpath);
	MEfree(tvers);
    }
}

/*
**	get_answer
**
**	subroutine to display a question in the status window and
**	grab the answer,
**	parameters:
**	questions   -	message to be displayed in status window,
**	ansbuff	    -	buffer to hold answer entered by user
**
**	Note: this should not be called if working in batch mode
*/
VOID
get_answer(char *question,char *ansbuff)
{
    i4                     ret_nat ;
#ifdef UNIX
    char                  *errmsg = NULL ;
#endif

    ret_nat = SEP_Get_Answer( statusW, PROMPT_ROW, PROMPT_COL, question,
			      ansbuff, &batchMode,
#ifdef UNIX
			      &errmsg);
#else
			      NULL);
#endif

    if (ret_nat)
    {
#ifdef UNIX
	SIputrec(errmsg, stdout);
#endif
	exiting(ret_nat, (bool)TRUE);
    }
}

/*
**	load_commands
**
**	Description:
**	subroutine to load legal SEP commands. It creates a B-tree.
*/

STATUS
load_commands( char *pathname, char *errmsg )
{
    STATUS                 Alloc_cmmd_node ( char *cmdBuf, char *errmsg ) ;

    STATUS                 STSerr ;
    LOCATION               cmmdLoc ;
    FILE                  *cmmdPtr ;

    char                  *cptr ;
    char                  *wrkBuffer = NULL ;
    char                  *filename  = NULL ;
    char                  *cmd_buf   = NULL ;

    bool                   not_finished ;


    wrkBuffer = SEP_MEalloc(SEP_ME_TAG_CMMDS, MAX_LOC+1, TRUE, &STSerr);
    filename  = (STSerr == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_CMMDS, MAX_LOC+1, TRUE, &STSerr) : NULL;
    cmd_buf   = (STSerr == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_CMMDS, TEST_LINE+9,TRUE,&STSerr) : NULL;

    if (STSerr != OK)
    {
	STprintf(errmsg, ERx("%sallocate memory for buffer, err = %d "),
		 ErrC, STSerr);
    }
    else
    {
        STcopy(pathname, wrkBuffer);
    
        if ((STSerr = LOfroms(PATH, wrkBuffer, &cmmdLoc)) != OK)
        {
            STprintf(errmsg,ERx("%smake path out of <%s>, err = %d "), ErrC,
                     wrkBuffer, STSerr);
        }
        else if ((STSerr = LOfstfile(CMMD_FILE, &cmmdLoc)) != OK)
        {
            STprintf(errmsg,ERx("%sappend file to path, err = %d "), ErrC,
                     STSerr);
        }
        else
        {
            LOtos(&cmmdLoc, &cptr);
            STcopy(cptr, filename);
    
            if ((STSerr = LOfroms(FILENAME & PATH, filename, &cmmdLoc)) != OK)
            {
                STprintf(errmsg,
                         ERx("%sget location for commands file. err = %d "),
                         ErrC, STSerr);
            }
            else if ((STSerr = SIopen(&cmmdLoc,ERx("r"),&cmmdPtr)) != OK)
            {
                STprintf(errmsg,ERx("%scommands file, err = %d "), ErrOpn,
                         STSerr);
            }
        }
    }

    for (not_finished = TRUE; not_finished && (STSerr == OK); )
    {
	if ((STSerr = SIgetrec(cmd_buf,TEST_LINE,cmmdPtr)) == OK)
	{
	    STSerr = Alloc_cmmd_node(cmd_buf, errmsg);
	}
	else if (STSerr == ENDFILE)
	{
	    not_finished = FALSE;
	    STSerr = OK;
	}
	else
	{
	    STprintf(errmsg,
		     ERx("ERROR: Reading fm commands.sep, err = %d "), STSerr);
	}
    }

    if (STSerr == OK)
    {
	if ((STSerr = SIclose(cmmdPtr)) != OK)
	{
	    STprintf(errmsg, ERx("%sclose commands file, err = %d "), ErrC,
		     STSerr);
	}
    }

    if (wrkBuffer) MEfree(wrkBuffer);
    if (filename)  MEfree(filename);
    if (cmd_buf)   MEfree(cmd_buf);

    if (tracing &&(STSerr != OK))
    {
	SIfprintf(traceptr, ERx("load_commands> %s\n"), errmsg);
    }

    return(STSerr);
}

STATUS
Alloc_cmmd_node(char *cmdBuf, char *errmsg)
{
    STATUS                 insert_node ( CMMD_NODE **master,
					 CMMD_NODE *anode ) ;

    STATUS                 ret_val ;
    char                  *cptr ;
    char                  *comma ;
    CMMD_NODE             *newnode ;


    cptr  = SEP_CMlastchar(cmdBuf,0);
    *cptr = EOS;	                	 /* get rid of EOL */
    comma = STindex(cmdBuf,ERx(","),0);

    if ((ret_val = (comma == NULL) ? FAIL : OK) == FAIL)
    {
	STcopy(ERx("ERROR: illegal format in commands file"), errmsg);
    }
    else
    {
        *comma = EOS;
        CMnext(comma);
    
        cptr = SEP_MEalloc(SEP_ME_TAG_CMMDS, sizeof(CMMD_NODE),FALSE,&ret_val);
        if (ret_val != OK)
        {
            STprintf(errmsg, ERx("%sallocate memory for CMMD_NODE, err = %d "),
                     ErrC, ret_val);
        }
	else
	{
            newnode = (CMMD_NODE *)cptr;
            newnode->cmmd_nm = STtalloc(SEP_ME_TAG_CMMDS, cmdBuf);
    
            if ((ret_val = CVan(comma,&newnode->cmmd_id)) != OK)
            {
		STcopy(ERx("ERROR: illegal format in commands file"), errmsg);
            }
	    else
	    {
		newnode->left = newnode->right = NULL;
		if ((ret_val = insert_node(&sepcmmds,newnode)) != OK)
		{
		    STcopy(ERx("ERROR: insert_node failed..."), errmsg);
		}
	    }
	}
    }
    return(ret_val);

}

/*
**	insert_node
*/

STATUS
insert_node(CMMD_NODE **master,CMMD_NODE *anode)
{
    STATUS                 ret_val = OK ;
    CMMD_NODE	          *head ;
    i4		           cmp;

    if (*master == NULL)
    {
	*master = anode;
    }
    else
    {
	for (head = *master;;)
	{
	    cmp = STcompare(anode->cmmd_nm, head->cmmd_nm);
	    if (cmp < 0)
	    {
		if (head->left)
		{
		    head = head->left;
		}
		else
		{
		    head->left = anode;
		    break;
		}
	    }
	    else
	    {
		if (head->right)
		{
		    head = head->right;
		}
		else
		{
		    head->right = anode;
		    break;
		}
	    }
	}

    }

    return(ret_val);
}

/*
**	get_args
**
**	Description:
**	subroutine to read flags given in command line
*/

STATUS
get_args(i4 argc,char **argv,char *testname)
{

    char                  *tty = NULL ;
    i4                     args ;
    STATUS                 gottest = FAIL ;
    STATUS                 CVstat ;

    LOINFORMATION          LocInfo ;
    i4                     infoflag ;

    char                   lo_dev  [LO_DEVNAME_MAX+1] ;
    char                   lo_path [LO_PATH_MAX+1] ;
    char                   lo_fpre [LO_FPREFIX_MAX+1] ;
    char                   lo_fsuf [LO_FSUFFIX_MAX+1] ;
    char                   lo_fver [LO_FVERSION_MAX+1] ;

    char                  *pa   = NULL ;
    char                  *npa  = NULL ;
    char                  *op   = NULL ;
    char                  *nop  = NULL ;

    i4                     pal  ;
    i4                     npal ;
    i4                     opl  ;
    i4                     nopl ;

    bool                   sw   ;
    bool                   nsw  ;


    if ((CVstat = SEP_cmd_line(&argc, argv, cmdopt, ERx("-"))) != OK)
    {
	STprintf(buffer_1, "SEP command line is bad!!");
	return(CVstat);
    }

    if (*testname != EOS)
	gottest = OK;

    for (args = 1; args < argc; args++)
    {
	sw = break_arg(argv[args], buffer_1, &pa, &op, &pal, &opl);

	if ((args+1) < argc)
	{
	    STcopy(argv[args+1], buffer_2);
	    nsw  = break_arg(buffer_2, NULL, &npa, &nop, &npal, &nopl);
	}
	else
	{
	    nsw  = FALSE;
	    npa  = NULL;
	    nop  = NULL;
	    npal = 0;
	    nopl = 0;
	}

	if (!sw)
	{
	    if (gottest == OK)
	    {
		STprintf(buffer_1, Illegal_param, buffer_1);
		return(FAIL);
	    }
	    else
	    {
		STcopy(pa, testname);
		gottest = OK;
		continue;
	    }
	}

	if ((opl == 0) && (nsw == FALSE) && (npa != NULL))
	{
	    CVstat = OK;
	    switch (*pa)
	    {
	       case 'I': case 'i':
	       case 'O': case 'o':   break;

	       case 'A': case 'a':
	       case 'M': case 'm':   if (npal != 1) CVstat = FAIL;	break;

	       default:		     CVstat = FAIL;			break;
	    }

	    if (CVstat == OK)
	    {
		op  = npa;
		opl = npal;
		args++;
	    }
	}

	switch (*pa)
	{
           case 'A': case 'a':

		if (op)
		{
		    if (*op == 'R'||*op == 'I'||*op == 'E'||
			*op == 'r'||*op == 'i'||*op == 'e')
			canonAns = *op;
		    else
		    {
			STprintf(buffer_1, Illegal_flag, buffer_1);
			return(FAIL);
		    }
		}
		break;

	   case 'I': case 'i':

		NMgtAt(ERx("TERM_INGRES"), &tty);

		if (tty == NULL || *tty == EOS)
		{
		    tty = ERx("vt100");
		}

		if (op)
		{
		    if (*op == EOS)
			STcopy(tty, terminalType);
		    else
		    {
			STcopy(op, terminalType);
			changeTIngres = (STcompare(op, tty) != 0);
		    }
		}
		useSepTerm = FALSE;
		break;

	   case 'M': case 'm':

		updateMode = TRUE;
		if (op)
		{
		    if (*op == 'M'||*op == 'A'||*op == 'O'||*op == 'I'||
			*op == 'm'||*op == 'a'||*op == 'o'||*op == 'i'||
			*op == EOS)
			editCAns = *op;
		    else
		    {
			STprintf(buffer_1, Illegal_flag, buffer_1);
			return(FAIL);
		    }
		}
		break;

	   case 'O': case 'o':

		if (op)
		{
		    if (*op != EOS)
		    {
			outputDir = STtalloc(SEP_ME_TAG_NODEL, op);
			outputDir_type = SEP_LOwhatisit(outputDir);
			CVstat = LOfroms(outputDir_type,outputDir,&outLoc);
			infoflag = (LO_I_TYPE | LO_I_PERMS);
			outLoc_exists = (LOinfo(&outLoc,&infoflag,&LocInfo) == OK);
			LOwhat(&outLoc,&infoflag);

			if ((infoflag == PATH) && (!outLoc_exists))
			{
			    STprintf(buffer_1,ERx("ERROR: no output dir => %s"),
				     outputDir);
			    return(FAIL);
			}
		    }
                }
		break;

	   default:

		STprintf(buffer_1, Illegal_param, buffer_1);
		return(FAIL);
	}
    } /* end of for loop */

    /* check for conflicting parameters */

    if (shellMode  && batchMode ) batchMode  = FALSE;
    if (shellMode  && updateMode) updateMode = FALSE;
    if (updateMode && batchMode ) updateMode = FALSE;

    if (batchMode && gottest == FAIL)
    {
	STprintf(buffer_1, script_must_be_given, "in batch");
	return(FAIL);
    }
    if (updateMode && gottest == FAIL)
    {
	STprintf(buffer_1, script_must_be_given, "in update");
	return(FAIL);
    }

    if (diff_numerics)
    {
	if (fuzz_int == fuzz_factor)
	{
	    if (fuzz_int > 0)
		fuzz_factor = MHpow((f8)10.0,(f8)(-1)*fuzz_factor);
	    else
		fuzz_factor = MHpow((f8)10.0,fuzz_factor);
	}
    }
   
    return(OK);
}
/*
**  break_arg
*/

bool
break_arg(char *arg,char *savebuf,char **param,char **op_tion,i4 *param_len,i4 *op_tion_len)
{
    char                  *pa = NULL ;
    char                  *op = NULL ;
    char                  *c = NULL ;
    char                  *eq_ptr = NULL ;
    char                  *s_ptr = NULL ;
    char                  *d_ptr = NULL ;
    char                  *tmp_ptr = NULL ;

    i4                     pal ;
    i4                     opl ;

    bool                   sw  ;

    if (savebuf)
	STcopy(arg,savebuf);

    pa  = arg;
    pal = SEP_CMstlen(pa,0);
    op  = NULL;
    opl = 0;

    if (CMcmpcase(pa,ERx("-")) != 0 )
	sw = FALSE;
    else
    {
	sw = TRUE;
	CMnext(pa);
	c  = SEP_CMlastchar(pa,0);

	if ((eq_ptr = STindex(pa, eq_sign, 0)) != NULL)
	{
	    tmp_ptr = eq_ptr;
	    op = CMnext(eq_ptr);
	    *tmp_ptr = EOS;
	}
	else
	{
	    d_ptr = STindex(pa, DBL_QUO, 0);
	    if ((d_ptr != NULL) && (CMcmpcase(c,DBL_QUO) == 0) && (d_ptr < c))
	    {
		tmp_ptr = d_ptr;
		op = CMnext(d_ptr);
		*tmp_ptr = EOS;
		*c = EOS;
	    }
	    else
	    {
		s_ptr = STindex(pa, s_quote, 0);
		if ((s_ptr != NULL) && (CMcmpcase(c,s_quote) == 0) &&
		    (s_ptr < c))
		{
		    tmp_ptr = s_ptr;
		    op = CMnext(s_ptr);
		    *tmp_ptr = EOS;
		    c = EOS;
		}
		else
		{
		    op = pa;
		    CMnext(op);
		}
	    }
	}
	opl = SEP_CMstlen(op,0);
	pal = SEP_CMstlen(pa,0);
    }

    *param	 = pa;
    *op_tion	 = op;
    *param_len	 = pal;
    *op_tion_len = opl;

    return(sw);
}
/*
**  copy_hdr()
*/

i4
copy_hdr( FILE *testFile, char *errmsg )
{
    i4                     ret_nat = 0 ;
    STATUS                 ioerr ;

    bool                   append_it  ;
    bool                   did_history = FALSE ;

    do
    {
        ioerr = get_command(testFile,NULL,FALSE);
	if ((ioerr = (ioerr != OK) ? FAIL : ioerr) != OK)
	{
	    STcopy(ERx("ERROR: failed while copying header to .upd file"),errmsg);
	    ret_nat = SEPerr_CpyHdr_fail;
        }
	else
	{
	    bool           append_it1 ;
	    bool           append_it2 ;

	    append_it1 = ((sep_state == STARTUP_STATE   )||
                      (sep_state == IN_COMMENT_STATE)||
                      (sep_state == BLANK_LINE_STATE)  );

	    append_it2 = (((sep_state&IN_COMMENT_STATE) == 0)&&
                      (sep_state_old&IN_COMMENT_STATE));

	    append_it  = (append_it1||append_it2);

	    if (append_it2 && updateMode && did_history == FALSE)
	    {
		disp_line(ERx(" Creating maintenance history entry... "),0,0);
		PCsleep(2000);

		/* Create History for this update. */

	    	if (bld_cmtName(msg) != OK)
	    	{
		    disp_line(msg,0,0);
	   	}
	    	else
	    	{
		    SIopen(cmtLoc,ERx("w"),&commentPtr);
		    SIfprintf(commentPtr,ERx("          %s\t(Author)\n"),&timeStr);
		    SIclose(commentPtr);

		    edit_file(cmtName);
		    if (SEP_LOexists(cmtLoc))
		    {
			append_file(cmtName);
			del_floc(cmtLoc);
		    }
		    did_history = TRUE;
		}
	    }

	    if (append_it)
	    {
		append_line(buffer_1,0);
	    }
	}
    }
    while ((append_it)&&(ioerr == OK));

    if (ioerr == OK)
    {
	fromLastRec = 1;
	STcopy(buffer_1, holdBuf);
    }

    return(ret_nat);
}



/*
**  define_env_var
**
**  routine to define an environment variable (logical), it also places
**  the logical in a chain of logicals defined. Before, ending execution
**  all these logicals are deassigned and restored to their previous
**  value (if any)
**
*/

STATUS
define_env_var(char *name,char *newval)
{
    STATUS                 STSerr  = OK ;
    char                  *oldval  = NULL ;

    STATUS                 create_chain_element ( char *name,
                                                  char *newval,
                                                  char *oldval ) ;

    if ((name == NULL)||(*name == EOS)||(newval == NULL)||(*newval == EOS))
    {
        STcopy(ERx("ERROR: BAD name or newvalue arg."),buffer_1);
	if (tracing)
	{
	    SIfprintf(traceptr, ERx("define_env_var> %s\n"),buffer_1);
            SIflush(traceptr);
	}
	STSerr = FAIL;
    }
    else
    {	/*
	** look for old value, define new logical, then create env_chain
	** link, if successful.
	*/
	NMgtAt(name, &oldval);
	if ((STSerr = NMsetenv(name, newval)) == OK)
	{
	    STSerr = create_chain_element(name, newval, oldval);
	}
    }

    return (STSerr);
}

STATUS
create_chain_element( char *name, char *newval, char *oldval )
{
    STATUS                 STSerr  = OK ;
    ENV_VARS              *envptr  = NULL ;
    ENV_VARS              *lastmbr = NULL ;

    /* create chain element */

    envptr = (ENV_VARS *)SEP_MEalloc(SEP_ME_TAG_ENVCHAIN, sizeof(ENV_VARS),
                                     TRUE, &STSerr);
    if (STSerr != OK)
    {
        STprintf(buffer_1, ERx("%sallocate memory for ENV_VARS, err= %d "),
                 ErrC, STSerr);
        if (tracing)
	{
            SIfprintf(traceptr,ERx("define_env_var> %s\n"),buffer_1);
	    SIflush(traceptr);
	}
    }
    else
    {
	envptr->next    = NULL;
	envptr->prev    = NULL;
	envptr->name    = STtalloc(SEP_ME_TAG_ENVCHAIN, name);
	envptr->new_val = STtalloc(SEP_ME_TAG_ENVCHAIN, newval);

	if ((oldval != NULL)&&(*oldval != EOS))
	{
	    envptr->old_val = STtalloc(SEP_ME_TAG_ENVCHAIN, oldval);
	}
	else
	{
	    envptr->old_val = NULL;
	}

	/*
	** add element to chain
	*/
	if (env_chain)
	{   /*
	    ** First, get to the end of the list.
	    */
	    for (lastmbr = env_chain; lastmbr->next; lastmbr = lastmbr->next)
		;
	    /*
	    ** Then, plug in the new env var.
	    */
	    lastmbr->next = envptr;
	    envptr->prev = lastmbr;
	}
	else
	{
	    env_chain = envptr;
	}
    }
    return(STSerr);
}

STATUS
undefine_env_var(char *name)
{
    STATUS                 STSerr = OK ;
    ENV_VARS              *envptr = NULL ;
    ENV_VARS              *lastmbr = NULL ;
    bool                   not_found_it ;

    if (env_chain == NULL)
    {
	NMdelenv(name);
    }
    else
    {
	/*
	** First, get to the end of the list.
	*/
	for (lastmbr = env_chain; lastmbr->next; lastmbr = lastmbr->next)
	    ;
	/*
	** Now, work backwards looking for the one we want.
	*/
	for (not_found_it = TRUE; (not_found_it && lastmbr != NULL); )
	{
	    if (STcompare(name,lastmbr->name) == 0)
	    {
		NMdelenv(name);
		if (lastmbr->old_val)
		{
		    NMsetenv(name, lastmbr->old_val);
		    MEfree(lastmbr->old_val);
        	}
		MEfree(lastmbr->new_val);
		MEfree(lastmbr->name);

		if (lastmbr->next)
		{
		    lastmbr->next->prev = lastmbr->prev;
		}

        	if (lastmbr->prev)
		{
		    lastmbr->prev->next = lastmbr->next;
		}

		envptr = lastmbr;
		lastmbr = lastmbr->prev;
		MEfree(envptr);
		not_found_it = FALSE;
	    }
	    else
	    {
		lastmbr = lastmbr->prev;
	    }
	}
	if (not_found_it)
	{
	    STSerr = FAIL;
	}
    }
    return (STSerr);
}

/*
**  undo_env_chain
**
**  subroutine to undo environment chain, it deassigns the logicals in
**  the chain and restore their previous value (if any)
**
*/

STATUS
undo_env_chain()
{
    STATUS                 STSerr = OK ;
    ENV_VARS              *env_ptr ;

    if (env_chain)
    {
	for (env_ptr = env_chain; env_ptr->next; env_ptr = env_ptr->next)
	    ;
	for (; env_ptr; env_ptr = env_ptr->prev)
	{
	    NMdelenv(env_ptr->name);
	    if (env_ptr->old_val)
	    {
		NMsetenv(env_ptr->name, env_ptr->old_val);
	    }
	}
	MEtfree(SEP_ME_TAG_ENVCHAIN);
    }
    return (STSerr);
}

/*
**  main_exhandler
**
**  Description:
**  Exception handler for main routine
**
*/

static EX
main_exhandler(EX_ARGS	*exargs)
{
    EX                     exno ;
    PID                    pid ;
    char                   pidstring [6] ;

    EXinterrupt(EX_OFF);    /* disable more interrupts */
    exno = EXmatch(exargs->exarg_num, 1, EXINTR);

    /* identify incoming interrupt */

    if (exno)
    {
	STcopy(ERx("ERROR: control-c was entered"), buffer_1);
	testGrade = SEPerr_CtrlC_entered;
    }
    else
    {
	STcopy(ERx("ERROR: unknown exception took place"), buffer_1);
	testGrade = SEPerr_Unk_exception;
    }

    /*
    **	try to gracefully disconnect from INGRES system if connected
    */

    if (SEPtclink)
    {
	PCpid(&pid);
	CVla((pid & 0x0ffff),pidstring);
	switch (SEPtclink)
	{
	    case SEPONBE:
	    case SEPONFE:
		disconnectTClink(pidstring);
		break;
	}
    }

    (VOID)Refresh_windows(&batchMode, mainW, statusW, TRUE);
    if (!batchMode)
    {
	disp_line(buffer_1,0,0);
    }
    append_line(buffer_1,1);
    return(EXDECLARE);    
}

/*
**      exiting
*/

VOID
exiting(i4 value,bool getout)
{
    /* deassign logicals defined so far */

    undo_env_chain();
    MEtfree(SEP_ME_TAG_ENVCHAIN);
    /* clean windows */

    if (!batchMode)
    {
        TDendwin();
        TEclose();
    }

    /* restore cursor (just in case) */

    if (shellMode)
    {
        show_cursor();
    }

    /* Remove temp files from fill cmd, if any. */

    if (!(shellMode || (verboseMode && (verboseLevel > 1))))
    {
        del_fillfiles();
    }

    if (getout)
    {
        PCexit(value);
    }
}

/*
** executor.c - called by listexec to run a set of tests.
**
** History:
**
**      ##-jul-1989 (eduardo)
**              Created.
**      ##-sep-1989 (eduardo)
**              Major enhancements.
**      25-sep-1989 (mca)
**              Modify the call to SEPspawn on Unix to reflect the change
**              in SEPspawn's parameters.
**      26-Oct-1989 (fredv)
**              Increased the size of the args array to 10 because the number of
**              arguments we pass can be more than 4. 10 is just an arbitrary
**              pick and should be big enough for any case.
**      06-dec-1989 (mca)
**              In read_confile(), corrected the type of the variable
**              aptr from FILE to (FILE *).
**      18-jun-1990 (rog)
**              Standardize the usage of SEPspawn().
**      25-jun-1990 (rog)
**              Make batchMode a bool in all source files.
**      09-Jul-1990 (owen)
**              Use angle brackets on sep header files in order to search
**              for them in the default include search path.
**      11-Jul-1990 (owen)
**              Remove references to TST_QATEST. We don't need it or use it.
**      14-Nov-1990 (jonb)
**              Use the SEP environment variable, if it's defined, to determine
**              the name of the sep executable.
**      19-oct-1990 (rog)
**              Increase the size of the repLine and logName arrays so we
**              don't overflow them with long pathnames.
**      30-aug-91 (donj)
**              Pretty up a little. Change how var, sonres, is evaluated. Don't
**              rely on it to larger than an arbitrary size if it's an error.
**              Compare it to mnemonic error values defined in SEPERRS.H. Create
**              new local flag, sep_aborted. I believe that SEPDISC, SEPEXCEP
**              and NOSEPANS may be incorrect values. They will be corrected
**              and added to SEPERRS.H as their true values are discovered.
**      14-jan-1992 (donj)
**          Modified all quoted string constants to use the ERx("") macro.
**          Reformatted variable declarations to one per line for clarity.
**          Elaborated on SEPerr_IOinit_fail with SEPerr_CpyHdr_fail,
**          SEPerr_trmcap_load_fail, SEPerr_No_TST_SEP, SEPerr_OpnHdr_fail,
**          SEPerr_SEPfileInit_fail, SEPerr_cmd_load_fail, SEPerr_LogOpn_fail
**          and SEPerr_TDsetterm_fail. Added additonal SEPerrs. Added explicit
**          declarations for all submodules called. Simplified and clarified
**          some control loops. Removed smatterings of code that had been
**          commented out long ago.
**      16-jan-1992 (donj)
**          Added SEPerr_Cant_find_test for VMS when a sep test doesn't exist
**          when SEP is executing in batch mode. It was filling up all free
**          disk space with an endlessly growing canon file trying to write
**          out the error msg with SIprintf() in sep's get_answer (septool.c).
**          STDOUT was going to the canon file. On UNIX it works OK, did add
**          a SEPerr_question_in_batch for other possible problems.
**      24-feb-1992 (donj)
**          Added globalrefs to tracing, trace_nr and traceptr. These tracing
**          vars are used by SEPspawn() in SEPTOOL.EXE. They're here to
**          provide that reference only.
**      23-feb-1993 (donj)
**          OVERLAYSP needed globaldef'ing for VMS
**      02-apr-1992 (donj)
**          Changed all refs to '\0' to EOS. Changed more char pointer
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)"
**      15-apr-1992 (donj)
**          Add SEP_cmd_line() to make command line parsing more uniform
**          amoungst SEPTOOL,LISTEXEC and EXECUTOR. Changed more code to use
**          CM routines. Optimized the use of memory by using MEreqmem().
**          Removed some useless variables. Added code to skip lines beginning
**          with a "#" character (comment line).
**      05-jul-92 (donj)
**          Added SEP version code to allow listexec and executor to know
**          which SEP version it was built against. Listexec and Executor's
**          version will now be the same as the SEP version. Added to sepflags
**          option to pass command line flags to each SEP test. Trimmed compat
**          library out of NEEDLIBS of the ming hints. Not needed. This will
**          allow listexec and executor to be built on UNIX w/o having
**          libcompat.a. Libcompat.a is not delivered with the UNIX tar file.
**      05-jul-92 (donj)
**          Forgot to put LIBINGRES into NEEDLIBS to replace COMPAT.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**      30-nov-1992 (donj)
**          Converted LOfroms() to SEP_LOfroms(). Change local function,
**          break_line() to parse_list_line() so as not to confuse with SEP's
**          break_line function.  Take getLocation() out of here and use the
**          same one SEP uses.
**      04-fed-1993 (donj)
**          MPE can't handle SIopen() for writing SI_TXT files. Creates
**          binary files by default. Changed SIopen for write to SIfopen().
**      15-feb-1993 (donj)
**          Add a ME_tag arg to SEP_LOfroms.
**      09-mar-1993 (donj)
**          Take out SEP_LOrebuild() not needed anymore.
**      31-mar-1993 (donj)
**          Add a new field to OPTION_LIST structure, arg_required. Used
**          for telling SEP_Cmd_Line() to look ahead for a missing argument.
**    26-apr-1993 (donj)
**          Finally found out why I was getting these "warning, trigrah sequence
**          replaced" messages. The question marks people used for unknown
**          dates. (Thanks DaveB).
**       2-jun-1993 (donj)
**          Took out unused arg from SEP_cmd_line.
**      14-aug-1993 (donj)
**          changed cmdopt from an array of structs to a linked list of structs.
**          added calls to SEP_alloc_Opt() to create linked list. Added a call
**          to MEadvise() as per CL requirements.
**      16-aug-1993 (donj)
**          Reflect changes in SEP_Vers_Init(), *sep_version and *sep_platform.
**      21-aug-1993 (donj)
**          NULL out thisTestDir so it won't be tempted to MEfree() unallocated
**          memory the first time around.
**      26-oct-1993 (donj)
**          Patched an array bounds write with thisTestDir.
**       3-nov-1993 (donj)
**          Fixed a problem with testDir and thisTestDir where testDir is
**          specified and there isn't a test path in the listfile, testDir
**          wasn't beeing used.
**       2-dec-1993 (donj)
**          Added testGrade GLOBALDEF here. It's needed by the expression
**          evaluator which resides in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**      14-dec-1993 (donj)
**          Added Status_Value GLOBALDEF here. It's needed by the expression
**          evaluator which resides in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**      30-dec-1993 (donj)
**          Added an include of a dummy.h file that has GLOBALDEFS for things
**          that are referenced in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**       4-jan-1994 (donj)
**          Added define's for the two disp_line functions.
**       4-jan-1994 (donj)
**          batchMode dummy GLOBALDEF was moved to dummy.h.
**      20-apr-1994 (donj)
**          Added initial testcase support.
**	10-may-1994 (donj)
**	    Reorganized GLOBALDEFS and FUNC_EXTERNS. Modularized much.
**	    Have executor allocate more of it's memory structures so that
**	    they can be removed so it can run in a smaller memory space.
**
**	    Fixed executor so that it can run SEP SH Bourne Shell or other
**	    types of scripts. If a list line begins with a "~" character, then
**	    the characters between "~"'s are processed as a command line, i.e.:
**
**		~-other=ksh~SEP,SEP,BUGS,CURSORY::whattest.sep:-w20
**
**	    Use STcopy() rather than STprintf() when no formatting is required.
**
**	    Prototyped any functions that weren't already prototyped. Put all
**	    the prototypes into hdr!fndefs.h.
**
**	    Tightened up many functions. Gave them one exit, with a real value
**	    to return.
**	07-jun-1994 (donj)
**	    Fixed a UNIX bug in retrieving the results of a SEP test. My SEPerr
**	    codes are negative numbers, but UNIX was giving me back the unsigned
**	    equivalents; i.e.: 229 instead of -27, 230 instead of -26. Fixed it
**	    with a one line cast.
**	    Added a SEPerr for ABORT due to missing canon.
**
**	11-sep-1995 (somsa01)
**	    Added defined(NT_GENERIC) to MEadvise.
**	20-Nov-1995 (somsa01)
**	    Added GLOBALDEF SEPpidstr [6] for use with sepspawn.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-Jan-2003 (hanje04)
**	    Replace NULL with 0 in switch statement in EXEC_cat_errmsg() 
**	    to stop comiler barfing because we're switching on an i4.
*/

/*
    Include files
*/

#include <compat.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <tc.h>
#include <tm.h>
#include <termdr.h>

#define executor_c

#include <executor.h>
#include <seperrs.h>
#include <sepdefs.h>

#include <fndefs.h>
#include <dummy.h>

#define disp_line(l,r,c)            Disp_Line(mainW, batchMode, &screenLine, paging, l, r, c)
#define disp_line_no_refresh(l,r,c) Disp_Line_no_refresh(mainW, batchMode, &screenLine, l, r, c)

/*
** *************************************************************************
*/

#define TESTSTYLE_SH       ERx("R")
#define TESTSTYLE_SEP      ERx("S")
#define TESTSTYLE_OTHER    ERx("O")
#define TESTSTYLE_ALL      ERx("*")

#ifdef VMS
#define SH_SYM             ERx("@")
#else
#define SH_SYM             ERx("sh")
#endif

#define SEP_ENV_VAR        ERx("SEP")
#define DEFAULT_SEP_NAME   ERx("sep")

GLOBALDEF    char          testStyle [3] = { '*', EOS, EOS } ;
GLOBALDEF    bool          testStyle_is_SH    = FALSE ;
GLOBALDEF    bool          testStyle_is_SEP   = FALSE ;
GLOBALDEF    bool          testStyle_is_OTHER = FALSE ;
GLOBALDEF    bool          testStyle_is_ALL   = TRUE ;

#define LIST_COMMENT_CHAR  ERx("#")
#define LISTCMD_CH      ERx("~")

GLOBALDEF    bool          test_is_SH       = FALSE ;
GLOBALDEF    bool          test_is_SEP      = FALSE ;
GLOBALDEF    bool          test_is_OTHER    = FALSE ;

GLOBALDEF    char          test_prefix [80] ;
GLOBALDEF    char          test_is_cmd [MAX_LOC] ;
GLOBALDEF    char         *test_is_Tokens [20] ;

/*
** *************************************************************************
*/

GLOBALDEF    bool          Sep_TestCase     = FALSE ;
GLOBALDEF    bool          Sep_TestInfo     = FALSE ;
GLOBALDEF    char         *Sep_TestInfoDB   = NULL ;

/*
** *************************************************************************
**	Global Variables
*/

#ifdef VMS

GLOBALDEF    char          setFile    [MAX_LOC] ;
GLOBALDEF    char          batchQueue [MAX_LOC] ;
GLOBALDEF    char          startTime  [MAX_LOC] ;

#endif

GLOBALDEF    char         *sep_platform = NULL ;
GLOBALDEF    char         *exec_version = NULL ;

GLOBALDEF    char          confFile   [MAX_LOC] ;
GLOBALDEF    char          outDir     [MAX_LOC] ;
GLOBALDEF    char          testList   [MAX_LOC] ;
GLOBALDEF    char          testDir    [MAX_LOC] ;
GLOBALDEF    char          testenvDir [MAX_LOC] ;

GLOBALDEF    PART_NODE    *ing_parts  [MAX_PARTS] ;

GLOBALDEF    PART_NODE    *ing_types  = NULL ;
GLOBALDEF    PART_NODE    *ing_levels = NULL ;

GLOBALDEF    i4            Status_Value = 0 ;

/*
** *************************************************************************
**	Tracing Vars.
*/
GLOBALDEF    i4            tracing = 0 ;
GLOBALDEF    i4            trace_nr = 0 ;
GLOBALDEF    FILE         *traceptr = NULL ;
GLOBALDEF    char         *trace_file ;
GLOBALDEF    LOCATION      traceloc ;
GLOBALDEF    i4            testGrade = 0 ;

/*
** *************************************************************************
**	Command Parameters
*/

GLOBALDEF    char          userName [20] ;
GLOBALDEF    char          SepFlag_str [20] ;
GLOBALDEF    i4            diffLevel = 20 ;
GLOBALDEF    i4            jobLevel  = 5 ;
GLOBALDEF    bool          interactive      = FALSE ;
GLOBALDEF    bool          SepFlags         = FALSE ;

#ifdef VMS

GLOBALDEF    bool          fromListexec = FALSE ;
GLOBALDEF    char          listexecKey [MAX_LOC+1] ;

#endif

GLOBALDEF    OPTION_LIST  *cmdopt = NULL ;
GLOBALDEF    OPTION_LIST  *lisopt = NULL ;

#ifdef NT_GENERIC
GLOBALDEF    char SEPpidstr[6];
#endif

/*
** *************************************************************************
**      ming hints
**
NEEDLIBS = SEPLIB SEPDIFFLIB LISTEXECLIB SEPCLLIB LIBINGRES
*/

/*
** *************************************************************************
**	Module Global Vars.
*/

    char                  *ErrWhileReading
		   = ERx("ERROR: failed while reading in configuration file") ;

#define SEP_ENV_VAR        ERx("SEP")
#define DEFAULT_SEP_NAME   ERx("sep")

    char                  *sepName = NULL ;

#define LINE_PARTS  20

    char                   lastpart  [LINE_PARTS] ;
    char                   lastsub   [LINE_PARTS] ;
    char                   lasttype  [LINE_PARTS] ;
    char                   lastlevel [LINE_PARTS] ;
    char                  *lineParts [LINE_PARTS] ;

    char                  *thisTestDir = NULL ;

    STATUS                 errpart ;
    STATUS                 errsub ;
    STATUS                 errtype ;
    STATUS                 errlevel = OK ;

/*
** *************************************************************************
*/

main(argc,argv)
i4    argc;
char *argv[];
{
    char                   repLine [256] ;
    char                   keyvalue [10] ;

    char                   listFlag [MAX_LOC+1] ;
    char                   logName  [MAX_LOC+1] ;
    char                   buffer   [MAX_LOC+1] ;
    char                   lineRead [MAX_LOC+1] ;
    char                   tempBuf  [MAX_LOC+1] ;
    char                   tempBuf2 [MAX_LOC+1] ;

    register char         *cptr ;
    register char         *cptr2 ;
    register char         *cptr3 ;

    char                  *keychar ;
    char                  *args [10] ;

    LOCATION               outLoc ;
    LOCATION               logLoc ;
    LOCATION               tempLoc ;
    LOCATION               goodTL ;
    LOCATION               badTL ;

    TCFILE                *keyptr ;

    STATUS                 ioerr ;
    STATUS                 goodtest ;

    FILE                  *testPtr ;
    FILE                  *repPtr ;
    FILE                  *goodTP ;
    FILE                  *badTP ;

    i4                     argnum ;
    i4                     i ;
    i4                     count ;
    i4                     partID ;
    i4                     subID ;
    i4                     sonres ;
    i4                     prores ;
    i4                     sep_aborted ;
    i4                     num_tests = 0 ;
    i4                     num_difs  = 0 ;
    i4                     num_abs   = 0 ;
    i4                     num_run   = 0 ;
    i4                     num_btl   = 0 ;

    bool                   printing_comment ;

    PID                    sonpid ;
    char                   sonpidstr [6] ;
    FILE                  *tcPtr ;
    LOCATION               tcLoc ;

    TEST_CASE             *testcase_root = NULL ;
    TEST_CASE             *testcase_old  = NULL ;
    TEST_CASE             *testcase_current = NULL ;
    TEST_CASE             *tc_tmp = NULL ;

    /*
    ** *************************************************************
    **  Do required MEadvise() call before anything else.
    */
#ifdef VMS
    ioerr = MEadvise(ME_INGRES_ALLOC);
#endif

#if defined(UNIX) || defined(NT_GENERIC)
    ioerr = MEadvise(ME_USER_ALLOC);
#endif

#ifdef NT_GENERIC
    sprintf (SEPpidstr, "%d", getpid());
#endif

    /*
    **  open STDIN & STDOUT
    */

    if (SIeqinit() != OK)
	PCexit(-1);

    /*
    **  do general initializations,
    **  getting arguments from command line,
    **  read in configuration file,
    **  check if parts-subs were selected,
    **  check if listexecKey was entered in VMS environment.
    */

    ioerr = EXEC_initialize( buffer );
    ioerr = ( ioerr == OK ) ? EXEC_get_args(argc, argv, buffer)     : ioerr ;
    ioerr = ( ioerr == OK ) ? read_confile(buffer)                : ioerr ;
    ioerr = ( ioerr == OK ) ? EXEC_were_partsubs_selected(buffer)   : ioerr ;
#ifdef VMS
    ioerr = ( ioerr == OK ) ? EXEC_chk_listexecKey(&keyptr, buffer) : ioerr ;
#endif

    if (ioerr != OK)
    {
	SIprintf(ERx("%s\n\n\n"),buffer);
	PCexit(-1);
    }

    /*
    **  go to output directory
    */

    SEP_LOfroms(PATH, outDir, &outLoc, SEP_ME_TAG_NODEL);
    LOchange(&outLoc);

    /*
    **  open list of tests to be executed, test report, good & bad test lists
    */

    if (EXEC_open_files( buffer, &testPtr, &repPtr, &goodTL, &goodTP, &badTL,
		       &badTP ) != OK)
    {
	SIputrec(buffer, stdout);
	SIflush(stdout);
	PCexit(0);
    }

    /*
    **  print report header
    */

    EXEC_print_rpt_header( buffer, repPtr );

    /*
    **  Read tests from list and execute those that meet selection
    **  criteria. For each test executed print part, subsystem, test
    **  name and number of difs found
    */

    printing_comment = FALSE;
    while ((ioerr = SIgetrec(buffer, MAX_LOC, testPtr)) == OK)
    {
	if (CMcmpcase((cptr = buffer),LIST_COMMENT_CHAR) == 0)
	{
	    if (CMcmpcase(CMnext(cptr),LIST_COMMENT_CHAR) != 0)
	    {
		if (!printing_comment)
		{
		    SIprintf(ERx("\n"));
		    printing_comment = TRUE;
		}
		SIputrec(buffer, stdout);
		SIflush(stdout);
	    }
	    continue;
	}
	printing_comment = FALSE;

	num_tests++;

	if (! jobLevel)
	{
	    /*
	    **  reach tolerance level, our only interest now is count
	    **  tests in list
	    */

	    SIputrec(buffer, badTP);
	    num_btl++;
	    continue;
	}

	test_is_SH = test_is_SEP = test_is_OTHER = FALSE;

	if (CMcmpcase((cptr2 = cptr = buffer),LISTCMD_CH) == 0)
	{
	    for (cptr2 = CMnext(cptr);
		 ((cptr2 != NULL)&&(*cptr != EOS)&&CMcmpcase(cptr2,LISTCMD_CH));
		 CMnext(cptr2)) {};

	    if ((cptr2 == NULL)||(*cptr2 == EOS))
	    {
		SIputrec(buffer, badTP);
		num_btl++;
		continue;
	    }
	    else
	    {
		cptr3 = test_is_cmd;
		CMcpychar(ERx("*"), cptr3);
		CMnext(cptr3);
		CMcpychar(ERx(" "), cptr3);
		CMnext(cptr3);
		while (cptr < cptr2)
		{
		    CMcpyinc(cptr,cptr3);
		}
		*cptr3 = EOS;

		i = SEP_CMgetwords(SEP_ME_TAG_NODEL, test_is_cmd,
					    20, test_is_Tokens);

		SEP_cmd_line(&i, test_is_Tokens, lisopt, ERx("-"));

		cptr = buffer;
		for( CMnext(cptr2); (cptr2 != NULL)&&(*cptr2 != EOS); )
		{
		    CMcpyinc( cptr2, cptr );
		}
		*cptr = EOS;
	    }
	}
	else
	{
	    test_is_OTHER = test_is_SH = !(test_is_SEP = TRUE);
	}

	if ((!testStyle_is_ALL)&&
	    ((testStyle_is_SH    && test_is_SEP  )||
	     (testStyle_is_SEP   && test_is_SH   )||
	     (testStyle_is_OTHER && test_is_OTHER)))
	{
	    continue;
	}

	if (!test_is_OTHER)
	{
	    STcopy( test_is_SH ? SH_SYM : sepName, test_prefix);
	}

	STcopy(buffer, lineRead);

	cptr  = SEP_CMlastchar(buffer,0);
	*cptr = EOS;                        /* get rid of nl */

	goodtest = OK;            /* start by assuming is a good test */

	/* finding out directory where this test is                   */
	/* (owen) Remove check for test style for now as we are only  */
	/* interested in sep tests.                                   */

	STcopy((*testDir != EOS ? testDir : testenvDir), thisTestDir);

	/* parse line */

	if (parse_list_line(buffer, lineParts,
		       (*testDir == EOS ? thisTestDir : NULL)) != OK)
	{
	    SIprintf(ERx("ERROR: syntax error in following line:\n\t%s\n\n"),
		     lineRead);
	    SIputrec(lineRead, badTP);
	    num_btl++;
	    continue;
	}

	/* check for legal part */

	CVupper(lineParts[0]);
	if (STcompare(lastpart, lineParts[0]) == 0)
	{
	    goodtest = errpart;
	}
	else
	{
	    for (partID = -1, count = 0;
		 (ing_parts[count])&&(partID < 0); count++)
	    {
		if (STcompare(ing_parts[count]->partname, lineParts[0]) == 0)
		{
		    partID = count;
		}
	    }

	    if (partID < 0)
	    {
		goodtest = FAIL;
	    }
	    STcopy(lineParts[0], lastpart);
	    errpart = goodtest;
	}

	if (goodtest == FAIL)
	{
	    continue;
	}

	/* check for legal subsystem */

	CVupper(lineParts[1]);
	if (STcompare(lastsub, lineParts[1]) == 0)
	{
	    goodtest = errsub;
	}
	else
	{
	    for (count = 0, errsub = FAIL;
		 (ing_parts[partID]->subs[count])&&(errsub == FAIL); count++)
	    {
		if (STcompare(ing_parts[partID]->subs[count], lineParts[1])
		    == 0)
		{
		    subID = count;
		    errsub = OK;
		}
	    }
	    STcopy(lineParts[1], lastsub);
	    goodtest = errsub;
	}
	if (goodtest == FAIL)
	{
	    /* SIputrec(lineRead, badTP); */
	    continue;
	}

	/* check for legal type */

	CVupper(lineParts[2]);
	if (STcompare(lasttype, lineParts[2]) == 0)
	{
	    goodtest = errtype;
	}
	else
	{
	    count = 0;
	    errtype = FAIL;
	    while (ing_types->subs[count])
	    {
		if (STcompare(ing_types->subs[count], lineParts[2]) == 0)
		{
		    errtype = OK;
		    break;
		}
		count++;
	    }
	    STcopy(lineParts[2], lasttype);
	    goodtest = errtype;
	}
	if (goodtest == FAIL)
	{
	    /* SIputrec(lineRead, badTP); */
	    continue;
	}

	/* check for legal level */

	CVupper(lineParts[3]);
	if (STcompare(lastlevel, lineParts[3]) == 0)
	{
	    goodtest = errlevel;
	}
	else
	{
	    count = 0;
	    errlevel = FAIL;
	    while (ing_levels->subs[count])
	    {
		if (STcompare(ing_levels->subs[count], lineParts[3]) == 0)
		{
		    errlevel = OK;
		    break;
		}
		count++;
	    }
	    STcopy(lineParts[3], lastlevel);
	    goodtest = errlevel;
	}
	if (goodtest == FAIL)
	{
	    /* SIputrec(lineRead, badTP); */
	    continue;
	}

	/* update count on number of tests initiated */

	ing_parts[partID]->partnum += 1;
	ing_parts[partID]->subinit[subID] += 1;

	/* create report line (part, subsystem, test name) */

	STmove(lineParts[0], ' ', 80, repLine);
	STmove(lineParts[1], ' ', 80 - hcol2, &repLine[hcol2]);
	STmove(lineParts[4], ' ', 80 - hcol3, &repLine[hcol3]);

	/*  get name of log file */

	STcopy(lineParts[4], logName);
	if (cptr = STindex(logName, ERx("."), 0))
	{
	    *cptr = EOS;
	}

	STcat(logName, ERx(".log"));
	SEP_LOfroms(FILENAME & PATH, logName, &logLoc, SEP_ME_TAG_TOKEN);

	/* execute test */

	args[0] = test_prefix;

	STcopy(thisTestDir, tempBuf);
	LOfroms(PATH, tempBuf, &tempLoc);
	LOfstfile(lineParts[4], &tempLoc);
	LOtos(&tempLoc, &args[1]);
	argnum = 2;

	if (test_is_SEP)
	{
	    STprintf(tempBuf2, ERx("-o%s"), outDir);
	    args[argnum++] = tempBuf2;
#ifdef VMS
	    STprintf(listFlag, ERx("-l%s"), listexecKey);
	    args[argnum++] = listFlag;
#endif
	}

	for (i = 5; lineParts[i]; i++)
	{
	    args[argnum++] = lineParts[i];
	}

	if (test_is_SEP)
	{
	    if (SepFlags)
	    {
		args[argnum++] = SepFlag_str;
	    }

	    if (! interactive)
	    {
		args[argnum++] = ERx("-b");
	    }
	}

	args[argnum] = NULL;

	/* print message in log file */

	SIprintf(ERx("\n=> \""));
	for (i = 0 ; i < argnum ; i++)
	{
	    SIprintf(ERx(" %s"), args[i]);
	}
	cptr = EXEC_TMnow();
	SIprintf(ERx("\"\t(%s)\n"), cptr);
	SIflush(stdout);
	MEfree(cptr);

	prores = sonres = sep_aborted = 0;

	if (SEPspawn(argnum, args, 1, NULL, NULL, &sonpid, &prores, FALSE)
	    != OK)
	{
	    SIprintf(ERx("ERROR: could not spawn command\n"));
	}
	else
	{
	    sonres = prores;

	    if (Sep_TestCase)
	    {
		CVla((sonpid & 0x0ffff), sonpidstr);
		STprintf(tcName,  ERx("tc%s.stf"), sonpidstr);
		LOfroms(FILENAME & PATH,tcName,&tcLoc);
	    }
#ifdef VMS
	    /* get SEP exiting value */

	    if (test_is_SEP)
	    {
		keychar = keyvalue;
		for (;;)
		{
		    *keychar = TCgetc(keyptr, 60);
		    if (*keychar == TC_EOF)
		    {
		        *keychar = EOS;
		        break;
		    }
		    if (*keychar++ == TC_TIMEDOUT)
		    {
		        STcopy(ERx("255"), keyvalue);
		        break;
		    }
		}
		keychar = keyvalue;
		CVan(keychar, &sonres);
	    }
	    else
	    {
		sonres = 0;
	    }
#endif
#ifdef UNIX
	    /* On UNIX it's changing the signed byte into
	    ** an unsigned byte.
	    */

	    sonres = (i4)(char)sonres;
#endif
	    /* add number of difs found to report line */

	    cptr = &repLine[hcol4];
	    CVna(sonres, buffer);
	    STcopy(buffer, cptr);

	    /* update appropiate counters depending on difs found */

	    sep_aborted = EXEC_was_sep_aborted( sonres, diffLevel );

	    if (sonres)
	    {
		SIputrec(lineRead, badTP);
		num_btl++;
		if (sep_aborted)
		{
		    jobLevel--;
		    ing_parts[partID]->subabd[subID] += 1;
		    num_abs++;

		    EXEC_cat_errmsg( sonres, repLine );
		}
		else
		{
		    ing_parts[partID]->subdif[subID] += 1;
		    num_difs++;
		    STcat(repLine, ERx("\t\t\t    DIFS\n"));
		}
	    }
	    else
	    {
		SIputrec(lineRead, goodTP);
		ing_parts[partID]->subrun[subID] += 1;
		num_run++;
		STcat(repLine, ERx("\n"));
	    }
	    SIputrec(repLine, repPtr);
	    if (Sep_TestCase)
	    {
		testcase_old = testcase_root;
		testcase_root = NULL;

		if (SIopen(&tcLoc,ERx("r"),&tcPtr) == OK)
		{
		    i4  i ;
		    for (i=1; SIgetrec(repLine,SCR_LINE,tcPtr) == OK; i++)
		    {
		        if ((i > 3)&&(STlength(repLine) > 1))
		        {
		            testcase_init(&tc_tmp,
		                          ERx("                             "));
		            testcase_froms(repLine,tc_tmp);
		            if (testcase_root == NULL)
		            {
		                testcase_current = testcase_root = tc_tmp;
		            }
		            else
		            {
		                testcase_current->next = tc_tmp;
		                testcase_current = tc_tmp;
		            }
		        }
		    }
		    SIclose(tcPtr);
		    del_floc(&tcLoc);
		    testcase_trace(stdout, testcase_root, ERx("	"));
		    testcase_trace(repPtr, testcase_root,
		                   ERx("			"));
		    testcase_merge(&testcase_old, &testcase_root);
		    testcase_root = testcase_old;
		}
	    }
	    SIprintf(ERx("Number of diff found = %d\n"), sonres);
	}
	SIflush(stdout);

	if (! jobLevel)
	{
	    /* print message indicating that tolerance level was reached */

	    SIputrec(header5, repPtr);
	}

    }

    /* check if we went through all the tests in the list */

    if (ioerr != ENDFILE)
	SIprintf(
	    ERx("\nERROR: could not execute all tests in the input list\n\n"));
    /*
    **  close good & bad test lists
    */

    SIclose(goodTP);
    SIclose(badTP);

    if (num_btl == 0)
    {
	del_floc(&goodTL);
	del_floc(&badTL);
    }

    /*
    **  print report statistics, for each subsystem within a part print:
    **  subsystem, tests initiated, tests without difs, tests w/ difs, and
    **  tests aborted. Then, print the totals for the part.
    **  Finally print the totals for the job
    */

    /* print headers for subsystem totals */

    SIputrec(header3, repPtr);
    SIputrec(header4, repPtr);

    count = 0;
    while (ing_parts[count])
    {
	char    **subs;
	i4     *sinit,*srun,*sdif,*sabd;
	i4     tinit,trun,tdif,tabd;

	if (! ing_parts[count]->partnum)
	{
	    count++;
	    continue;
	}
	tinit = trun = tdif = tabd = 0;
	subs  = ing_parts[count]->subs;
	sinit = ing_parts[count]->subinit;
	srun  = ing_parts[count]->subrun;
	sdif  = ing_parts[count]->subdif;
	sabd  = ing_parts[count]->subabd;
	subID = 0;
	while (subs[subID])
	{
	    if (sinit[subID])
	    {
		/* print subsystem totals */

		STmove((char *)(ing_parts[count]->partname), ' ', 80, repLine);
		cptr = &repLine[h1col2];
		STmove(subs[subID], ' ', 80 - h1col2, cptr);
		cptr = &repLine[h1col3];
		tinit += sinit[subID];
		CVna(sinit[subID], buffer);
		STmove(buffer, ' ', 80 - h1col3, cptr);
		cptr = &repLine[h1col4];
		trun += srun[subID];
		CVna(srun[subID], buffer);
		STmove(buffer, ' ', 80 - h1col4, cptr);
		cptr = &repLine[h1col5];
		tdif += sdif[subID];
		CVna(sdif[subID], buffer);
		STmove(buffer, ' ', 80 - h1col5, cptr);
		cptr = &repLine[h1col6];
		tabd += sabd[subID];
		CVna(sabd[subID], buffer);
		STpolycat(2, buffer, ERx("\n"), cptr);
		SIputrec(repLine, repPtr);
	    }
	    subID++;
	}

	/* print part totals */

	SIputrec(ERx("\n"), repPtr);
	STmove((char *)(ing_parts[count]->partname), ' ', 80, repLine);
	cptr = &repLine[h1col3];
	CVna(tinit, buffer);
	STmove(buffer, ' ', 80 - h1col3, cptr);
	cptr = &repLine[h1col4];
	CVna(trun, buffer);
	STmove(buffer, ' ', 80 - h1col4, cptr);
	cptr = &repLine[h1col5];
	CVna(tdif, buffer);
	STmove(buffer, ' ', 80 - h1col5, cptr);
	cptr = &repLine[h1col6];
	CVna(tabd, buffer);
	STpolycat(2, buffer, ERx("\n\n"), cptr);
	SIputrec(repLine, repPtr);

	count++;
    }

    /*
    **  print statistics for the whole job
    */

    STprintf(buffer, ERx("\nJOB SUMMARY\n"));
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("=== =======\n\n"));
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("Tests in input list          : %d\n"), num_tests);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("Number of tests initiated    : %d\n"),
		(num_difs + num_abs + num_run));
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("Number of tests without difs : %d\n"), num_run);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("Number of tests with difs    : %d\n"), num_difs);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    STprintf(buffer, ERx("Number of tests that aborted : %d\n"), num_abs);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);

    if (Sep_TestCase)
    {
	testcase_trace(stdout, testcase_root, ERx("	"));
	testcase_trace(repPtr, testcase_root, ERx("	"));
    }

    if (Sep_TestInfo)
    {
	testinfo_disconnect();
    }

    SIflush(stdout);
    SIflush(repPtr);

    /*
    **  this is it
    */

#ifdef VMS
    TCclose(keyptr);
#endif

    SIclose(repPtr);
    PCexit(0);
}

STATUS
EXEC_initialize( char *errmsg )
{
    STATUS                 ret_val = OK ;

    char                  *cptr ;

    register i4            i ;

    /*
    ** print executor version
    */

    SEP_Vers_Init(ERx("Test Executor"), &exec_version, &sep_platform);
    SIprintf(ERx("%s\n\n"), exec_version);

    /*
    ** Build the command line options list that SEP_cmd_line() uses
    ** to set command line parameters.
    */

    SEP_alloc_Opt(&cmdopt, ERx("difflevel"),
		           NULL,                /* a char * goes here      */
		           NULL,                /* a bool value goes here  */
		           &diffLevel,          /* a i4  value loaded here */
		           NULL,
		           TRUE                 /* is a value required ?   */
		           );
    SEP_alloc_Opt(&cmdopt, ERx("interactive"),
		           NULL,                /* a char * goes here      */
		           &interactive,        /* a bool value goes here  */
		           NULL,                /* a i4  value loaded here */
		           NULL,
		           FALSE                /* is a value required ?   */
		           );
    SEP_alloc_Opt(&cmdopt, ERx("style"),
		           testStyle,           /* a char * goes here      */
		           NULL,                /* a bool value goes here  */
		           NULL,                /* a i4  value loaded here */
		           NULL,
		           TRUE                 /* is a value required ?   */
		           );
    SEP_alloc_Opt(&cmdopt, ERx("username"),
		           userName,            /* a char * goes here      */
		           NULL,                /* a bool value goes here  */
		           NULL,                /* a i4  value loaded here */
		           NULL,
		           TRUE                 /* is a value required ?   */
		           );
    SEP_alloc_Opt(&cmdopt, ERx("flags"),
		           SepFlag_str,         /* a char * goes here      */
		           &SepFlags,           /* a bool value goes here  */
		           NULL,                /* a i4  value loaded here */
		           NULL,
		           TRUE                 /* is a value required ?   */
		           );
#ifdef VMS
    SEP_alloc_Opt(&cmdopt, ERx("listexec"),
		           NULL,                /* a char * goes here      */
		           &fromListexec,       /* a bool value goes here  */
		           NULL,                /* a i4  value loaded here */
		           NULL,
		           FALSE                /* is a value required ?   */
		           );
#endif
    SEP_alloc_Opt(&cmdopt, ERx("tolerance"),
		           NULL,                /* a char * goes here      */
		           NULL,                /* a bool value goes here  */
		           &jobLevel,           /* a i4  value loaded here */
		           NULL,
		           TRUE                 /* is a value required ?   */
		           );

    SEP_alloc_Opt(&lisopt, ERx("shell"),
			   NULL,                /* a char * goes here      */
			   &test_is_SH,         /* a bool value goes here  */
			   NULL,                /* a i4  value loaded here */
			   NULL,
			   FALSE                /* is a value required ?   */
			   );
    SEP_alloc_Opt(&lisopt, ERx("sep"),
			   NULL,                /* a char * goes here      */
			   &test_is_SEP,        /* a bool value goes here  */
			   NULL,                /* a i4  value loaded here */
			   NULL,
			   FALSE                /* is a value required ?   */
			   );
    SEP_alloc_Opt(&lisopt, ERx("other"),
			   test_prefix,         /* a char * goes here      */
			   &test_is_OTHER,      /* a bool value goes here  */
			   NULL,                /* a i4  value loaded here */
			   NULL,
			   TRUE                 /* is a value required ?   */
			   );

    /*
    **  Check if TST_TESTENV is defined
    */

    NMgtAt(ERx("TST_TESTENV"), &cptr);
    if ((cptr == NULL) || (*cptr == EOS))
    {
	/*
	** Try ING_TST usually they're the same.
	*/
	NMgtAt(ERx("ING_TST"), &cptr);
	if ((cptr == NULL) || (*cptr == EOS))
	{
	    STcopy(
	    ERx("ERROR: neither env vars TST_TESTENV nor ING_TST are defined"),
		   errmsg);
	    ret_val = FAIL;
	}
    }

    if (ret_val == OK)
    {
	STcopy(cptr, testenvDir);

	/*
	** See if SEP_TESTCASE is set.
	*/

	NMgtAt(ERx("SEP_TESTCASE"), &cptr);
	Sep_TestCase = (cptr != NULL)&&(*cptr != EOS) ? TRUE : FALSE ;

	/*
	** See if SEP_TESTINFO is set.
	*/

	NMgtAt(ERx("SEP_TESTINFO"), &cptr);
	if ((cptr != NULL) && (*cptr != EOS))
	{
	    if (testinfo_connect(cptr) == OK)
	    {
		Sep_TestInfoDB = STtalloc(SEP_ME_TAG_NODEL, cptr);
		Sep_TestInfo  = TRUE;
	    }
	}

	/*
	** See if SEP has an environmental variable
	*/

#define SEP_ENV_VAR  ERx("SEP")
#define DEFAULT_SEP_NAME ERx("sep")

	NMgtAt(SEP_ENV_VAR, &cptr);
	sepName = STtalloc(SEP_ME_TAG_NODEL,
		           (cptr == NULL)||(*cptr == EOS) ? DEFAULT_SEP_NAME
							  : cptr );

	*errmsg   = *lastpart  = *lastsub  = EOS;
	*lasttype = *lastlevel = *userName = EOS;

	for (i = 0; i < MAX_PARTS; i++)
	{
	    ing_parts[i] = NULL;
	}
    }

    return (ret_val);
}

char
*EXEC_TMnow()
{
    STATUS                 ret_val = OK ;
    SYSTIME                atime ;
    char                   timestr [40] ;

    TMnow(&atime);
    TMstr(&atime,timestr);

    return (STtalloc(SEP_ME_TAG_NODEL, timestr));
}

STATUS
EXEC_open_files( char *errmsg, FILE **tstPtr, FILE **rptPtr, LOCATION *gtLoc,
	       FILE **gtPtr, LOCATION *bdLoc, FILE **bdPtr )
{
    STATUS                 ret_val = OK ;

    LOCATION              *confLoc ;
    LOCATION              *repLoc ;
    LOCATION              *testLoc ;

    char                  *confBuf ;
    char                  *dummydev ;
    char                  *dummypath ;
    char                  *dummyext ;
    char                  *dummyver ;
    char                  *repName ;
    char                  *goodTests ;
    char                  *badTests ;
    char                  *FormatStr
		                 = ERx("ERROR: could not open %s (%s)\n\n\n");
    char                  *LFile = ERx("list file");
    char                  *RFile = ERx("report file");


    /*
    **  allocate memory for all structures and strings.
    */

    confBuf = STtalloc(SEP_ME_TAG_NODEL, confFile);
    confLoc = (LOCATION *)SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(LOCATION), TRUE,
		                      &ret_val);
    repLoc    = (LOCATION *)
		((ret_val == OK)
		 ? SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(LOCATION), TRUE,
		               &ret_val) : NULL );
    testLoc   = (LOCATION *)
		((ret_val == OK)
		 ? SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(LOCATION), TRUE,
		               &ret_val) : NULL );
    dummydev  = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, LO_DEVNAME_MAX+1, TRUE, &ret_val)
	      : NULL ;
    dummypath = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, LO_PATH_MAX+1, TRUE, &ret_val)
	      : NULL ;
    dummyext  = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, LO_FSUFFIX_MAX+1, TRUE, &ret_val)
	      : NULL ;
    dummyver  = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, LO_FVERSION_MAX+1, TRUE, &ret_val)
	      : NULL ;
    repName   = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, &ret_val)
	      : NULL ;
    goodTests = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, &ret_val)
	      : NULL ;
    badTests  = (ret_val == OK)
	      ? SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, &ret_val)
	      : NULL ;
    thisTestDir = (ret_val == OK)
		? SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC, TRUE, &ret_val)
		: NULL ;

    if (ret_val != OK)
    {
	STcopy(ERx("ERROR: couldn't allocate memory in EXEC_open_files()\n\n\n"),
	       errmsg);
	return (ret_val);
    }

    SEP_LOfroms(PATH & FILENAME, confBuf, confLoc, SEP_ME_TAG_NODEL);

    LOdetail(confLoc, dummydev, dummypath, repName, dummyext, dummyver);
    STcopy(repName, goodTests);
    STcopy(repName, badTests);

    MEfree(confBuf);
    MEfree(confLoc);
    MEfree(dummydev);
    MEfree(dummypath);
    MEfree(dummyext);
    MEfree(dummyver);

    /*
    **  open list of tests to be executed
    */

    SEP_LOfroms(FILENAME & PATH, testList, testLoc, SEP_ME_TAG_NODEL);
    if (SIopen(testLoc, ERx("r"), tstPtr) != OK)
    {
	STprintf(errmsg, FormatStr, LFile, testList);
    }
    else
    {
	/*
	**  open test report, good & bad test lists
	*/

	STcat(repName, ERx(".rpt"));
	SEP_LOfroms(FILENAME & PATH, repName, repLoc, SEP_ME_TAG_NODEL);
	if ((ret_val = SIfopen(repLoc, ERx("w"), SI_TXT, TERM_LINE, rptPtr))
	    != OK)
	{
	    STprintf(errmsg, FormatStr, RFile, repName);
	}
	else
	{
	    STcat(goodTests, ERx(".gtl"));
	    SEP_LOfroms(FILENAME & PATH, goodTests, gtLoc, SEP_ME_TAG_NODEL);
	    if ((ret_val = SIfopen(gtLoc, ERx("w"), SI_TXT, TERM_LINE, gtPtr))
		!= OK)
	    {
		STprintf(errmsg, FormatStr, LFile, goodTests);
	    }
	    else
	    {
		STcat(badTests, ERx(".btl"));
		SEP_LOfroms(FILENAME & PATH, badTests, bdLoc, SEP_ME_TAG_NODEL);
		if ((ret_val = SIfopen(bdLoc, ERx("w"), SI_TXT, TERM_LINE,
		                       bdPtr)) != OK)
		{
		    STprintf(errmsg, FormatStr, LFile, badTests);
		}
	    }
	}
    }

    MEfree(goodTests);
    MEfree(badTests);
    MEfree(repName);
    MEfree(repLoc);
    MEfree(testLoc);

    return (ret_val);
}

STATUS
EXEC_print_rpt_header( char *buffer, FILE *repPtr )
{
    STATUS                 ret_val = OK ;

    char                   currdir [MAX_LOC+1] ;
    char                  *cptr ;

    LOCATION               currLoc ;

    /* print executor version */

    STprintf(buffer, ERx("%s\n\n"),exec_version);
    SIputrec(buffer, repPtr);

    /* print date */

    cptr = EXEC_TMnow();
    STprintf(buffer, ERx("Date       : %s\n"), cptr);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    MEfree(cptr);

    /* print username */

    if (userName[0] == EOS)
    {
	cptr = userName;
	IDname(&cptr);
    }
    CVupper(userName);
    STprintf(buffer, ERx("Username   : %s\n"), userName);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);

    /* print name of current directory */

    LOgt(currdir, &currLoc);
    STprintf(buffer, ERx("Directory  : %s\n"), currdir);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);

#ifdef VMS
    /* print Setup file name */

    STprintf(buffer, ERx("Setup file : %s\n"), setFile);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
#endif

    /* print list of tests */

    STprintf(buffer, ERx("Tests list : %s\n"), testList);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);

    /* print input directory for tests if specified */

    if (STcompare(testDir, ERx("unspecified")) != 0)
    {
	STprintf(buffer, ERx("Tests Dir. : %s\n"), testDir);
	SIprintf(buffer);
	SIputrec(buffer, repPtr);
    }
    else
    {
	*testDir = EOS;
    }

    /* print Dif level */

    STprintf(buffer, ERx("Diff level : %d\n"), diffLevel);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);

    /* print tolerance level */

    STprintf(buffer, ERx("Tolerance  : %d\n\n"), jobLevel);
    SIprintf(buffer);
    SIputrec(buffer, repPtr);
    SIflush(stdout);

    /* print headers for PART I of report */

    SIputrec(header1, repPtr);
    SIputrec(header2, repPtr);

    return (OK);
}

i4
EXEC_was_sep_aborted( i4  sonres, i4  difflevel )
{
    i4                     ret_nat ;

    switch (sonres)
    {
       case 0:
		ret_nat = 0;
		break;

       case SEPerr_ABORT_canon_matched: case SEPerr_cmd_load_fail:
       case SEPerr_Cant_Connect_to_FE:  case SEPerr_CtrlC_entered:
       case SEPerr_question_in_batch:   case SEPerr_Unk_exception:
       case SEPerr_Cant_Set_FRSFlags:   case SEPerr_TM_timed_out:
       case SEPerr_Cant_Opn_Commfile:   case SEPerr_FE_timed_out:
       case SEPerr_trmcap_load_fail:    case SEPerr_bad_cmdline:
       case SEPerr_SEPfileInit_fail:    case SEPerr_OpnHdr_fail:
       case SEPerr_Cant_Trans_Token:    case SEPerr_CpyHdr_fail:
       case SEPerr_Cant_Opn_ResFile:    case SEPerr_LogOpn_fail:
       case SEPerr_Cant_Set_Peditor:    case SEPerr_Cant_Spawn:
       case SEPerr_TM_Cant_connect:     case SEPerr_No_TST_SEP:
       case SEPerr_Cant_find_test:      case SEPerr_no_IISYS:
       case SEPerr_ABORT_canon_missing:

		ret_nat = 1;
		break;

       default:
		ret_nat = (sonres > difflevel)||(sonres < 0) ? 1 : 0 ;
		break;
    }

    return (ret_nat);
}

STATUS
EXEC_cat_errmsg( i4  sonres, char *repLine )
{
    STATUS                 ret_val = OK ;

    switch (sonres)
    {
       case 0:
	    break;
       case SEPerr_no_IISYS:
	    STcat(repLine, ERx("\t\t*** II_SYSTEM UNDEFINED ***\n"));
	    break;
       case SEPerr_bad_cmdline:
	    STcat(repLine, ERx("\t\t*** BAD COMMAND LINE ***\n"));
	    break;
       case SEPerr_cmd_load_fail:
	    STcat(repLine, ERx("\t\t*** COMMANDS LOAD FAILED ***\n"));
	    break;
       case SEPerr_trmcap_load_fail:
	    STcat(repLine, ERx("\t\t*** TERMCAP LOAD FAILED ***\n"));
	    break;
       case SEPerr_OpnHdr_fail:
	    STcat(repLine, ERx("\t\t*** HEADER OPEN FAILED ***\n"));
	    break;
       case SEPerr_LogOpn_fail:
	    STcat(repLine, ERx("\t\t*** LOG OPEN FAILED ***\n"));
	    break;
       case SEPerr_CpyHdr_fail:
	    STcat(repLine, ERx("\t\t*** HEADER COPY FAILED ***\n"));
	    break;
       case SEPerr_No_TST_SEP:
	    STcat(repLine, ERx("\t\t*** TST_SEP UNDEFINED ***\n"));
	    break;
       case SEPerr_SEPfileInit_fail:
	    STcat(repLine, ERx("\t\t*** SEPFILE INIT FAILED ***\n"));
	    break;
       case SEPerr_CtrlC_entered:
	    STcat(repLine, ERx("\t\t*** Control-C was entered. ***\n"));
	    break;
       case SEPerr_Unk_exception:
	    STcat(repLine, ERx("\t\t*** Unknown exception encountered ***\n"));
	    break;
       case SEPerr_TM_timed_out:
	    STcat(repLine, ERx("\t\t*** Terminal Monitor Timed out. ***\n"));
	    break;
       case SEPerr_FE_timed_out:
	    STcat(repLine,
		  ERx("\t\t*** Forms based Front End Timed out. ***\n"));
	    break;
       case SEPerr_Cant_Spawn:
	    STcat(repLine, ERx("\t\t*** Can't Spawn Child Process ***\n"));
	    break;
       case SEPerr_TM_Cant_connect:
	    STcat(repLine,
		  ERx("\t\t*** Terminal Monitor Cannot Connect ***\n"));
	    break;
       case SEPerr_Cant_Trans_Token:
	    STcat(repLine, ERx("\t\t*** Error Translating Token ***\n"));
	    break;
       case SEPerr_Cant_Opn_Commfile:
	    STcat(repLine, ERx("\t\t*** Can't Open I/O COMM-files ***\n"));
	    break;
       case SEPerr_Cant_Opn_ResFile:
	    STcat(repLine, ERx("\t\t*** Can't Open Results File ***\n"));
	    break;
       case SEPerr_Cant_Set_FRSFlags:
	    STcat(repLine, ERx("\t\t*** Can't Set FRS Flags ***\n"));
	    break;
       case SEPerr_Cant_Set_Peditor:
	    STcat(repLine,
		  ERx("\t\t*** Can't Set editor var to Peditor ***\n"));
	    break;
       case SEPerr_Cant_Connect_to_FE:
	    STcat(repLine, ERx("\t\t*** Can't Connect to Front End ***\n"));
	    break;
       case SEPerr_Cant_find_test:
	    STcat(repLine, ERx("\t\t*** Can't Find Test Script ***\n"));
	    break;
       case SEPerr_question_in_batch:
	    STcat(repLine,
		  ERx("\t\t*** Question asked while working in batch.***\n"));
	    break;
       case SEPerr_ABORT_canon_matched:
	    STcat(repLine, ERx("\t\t*** ABORT canon matched. ***\n"));
	    break;
       case SEPerr_ABORT_canon_missing:
	    STcat(repLine, ERx("\t\t*** Canon Missing. ***\n"));
	    break;
       default:
	    STcat(repLine, ERx("\t\t*** ABORTED ***\n"));
	    break;
    }

    return (ret_val);
}

#ifdef VMS
STATUS
EXEC_chk_listexecKey(TCFILE **kptr, char *errmsg)
{
    STATUS                 ret_val ;
    LOCATION               keyloc ;

    /*
    **  check if listexecKey was entered in VMS environment.
    **  if so open TC file
    */

    if (fromListexec)
    {
	SEP_LOfroms(FILENAME & PATH, listexecKey, &keyloc, SEP_ME_TAG_NODEL);
	if ((ret_val = TCopen(&keyloc, ERx("r"), kptr)) != OK)
	{
	    STcopy(ERx("ERROR: could not open TC file\n\n\n"), errmsg);
	}
    }
    else
    {
	STcopy(ERx("ERROR: name of TC file is missing fm cmd line\n\n\n"),
	       errmsg);
	ret_val = FAIL;
    }

    return (ret_val);
}

#endif
STATUS
EXEC_were_partsubs_selected( char *errmsg )
{
    STATUS                 ret_val ;
    i4                     i ;

    /*
    ** check if parts-subs were selected
    */

    for (i = 0; i < MAX_PARTS; i++)
    {
	if (ing_parts[i])
	{
		break;
	}
    }

    if ((ret_val = (i >= MAX_PARTS) ? FAIL : OK) != OK)
    {
	STcopy(ERx("ERROR: no parts/subsystems were selected\n\n\n"), errmsg);
    }

    return (ret_val);
}

/*
**  EXEC_get_args
**
**  routine to get arguments from command line
*/

STATUS
EXEC_get_args( i4  argc, char *argv[], char *message )
{
    PID                    pid ;

    char                   pidstring [6] ;
    char                  *param ;

    i4                     args ;

    STATUS                 gotfile ;
    STATUS                 CVstat ;


    if ((CVstat = SEP_cmd_line(&argc, argv, cmdopt, ERx("-"))) != OK)
    {
	STcopy(ERx("LISTEXEC command line is bad!!"), message);
	return(CVstat);
    }

#ifdef VMS
    if (fromListexec)
    {
	PCpid(&pid);
	CVla((pid & 0x0ffff), pidstring);
	STprintf(listexecKey, ERx("tc%s"), pidstring);
    }
#endif

    testStyle_is_SH    = (CMcmpnocase(testStyle,TESTSTYLE_SH )   == 0);
    testStyle_is_SEP   = (CMcmpnocase(testStyle,TESTSTYLE_SEP)   == 0);
    testStyle_is_ALL   = (CMcmpnocase(testStyle,TESTSTYLE_ALL)   == 0);
    testStyle_is_OTHER = (CMcmpnocase(testStyle,TESTSTYLE_OTHER) == 0);

    if (argc > 1)
    {
	for (gotfile = FAIL, args = 1; (CVstat == OK)&&(args < argc); args++)
	{
	    param = argv[args];

	    if ((CMcmpcase(param,ERx("-")) == 0)||(gotfile == OK))
	    {
		STprintf(message,ERx("ERROR: Illegal parameter = %s"),param);
		CVstat = FAIL;
	    }
	    else
	    {
		STcopy(param, confFile);
		gotfile = OK;
	    }

	} /* end of for loop */

	if ((CVstat == OK)&&(gotfile == FAIL))
	{
	    STcopy(ERx("ERROR: test names file missing from command line"),
		   message);
	    CVstat = FAIL;
	}

    }
    else    /* if argc < 1 */
    {
	STcopy(ERx("ERROR: parameters are missing from command line"),message);
	CVstat = FAIL;
    }

    return(CVstat);
}

/*
**  read_confile
**
**  routine to read in configuration file data,
**  this is the format of the configuration file:
**  output directory
**  setup file
**  batch queue
**  starting time
**  list of tests
**  part name and subsystems to be run from it
**  keyword TYPES
**  type of tests to be executed
**  keyword LEVELS
**  level of tests to be executed
**
*/

STATUS
read_confile( char *message )
{
    PART_NODE             *part ;

    LOCATION               aloc ;

    FILE                  *aptr ;

    char                  *rcBuf ;
    char                  *rcTBuf ;
    char                  *cptr ;
    char                  *ErrAlloc
		   = ERx("ERROR: error allocating memory in read_confile()") ;

    STATUS                 ioerr ;

    bool                   flag ;
    bool                   finished ;

    i4                     count ;
    i4                     part_ID ;

#ifdef VMS
#define MAX_COUNT 6
#else
#define MAX_COUNT 3
#endif

    rcBuf  = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, &ioerr);
    rcTBuf = (ioerr == OK)
	   ? SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, &ioerr) : NULL ;

    if (ioerr != OK)
    {
	STcopy(ErrAlloc, message);
    }
    else
    {
	SEP_LOfroms(FILENAME & PATH, confFile, &aloc, SEP_ME_TAG_NODEL);

	if ((ioerr = SIopen(&aloc, ERx("r"), &aptr)) != OK)
	{
	    STcopy(ERx("ERROR: could not open configuration file"), message);
	}
    }

    /*
    ** start reading info from configuration file
    */

    for (count = 1; (ioerr == OK)&&(count <= MAX_COUNT); count++)
    {
	if ((ioerr = SIgetrec(rcBuf, MAX_LOC, aptr)) != OK)
	{
	    STcopy(ErrWhileReading, message);
	}
	else
	{
	    cptr = SEP_CMlastchar(rcBuf,0);
	    *cptr = EOS;                        /* get rid of nl */

	    if (CMcmpcase(rcBuf,ERx("#")) == 0)
	    {
		continue;
	    }

	    switch(count)
	    {
		case 1:
		    /* get output directory */
		    if ((ioerr = check_outdir(rcBuf, rcTBuf, message)) == OK)
		    {
		        STcopy(rcTBuf, outDir);
		    }
		    break;
#ifdef VMS
		case 2:
		    /* get setup file */
		    if ((ioerr = check_filename(rcBuf, rcTBuf, message, &flag,
		                                TRUE)) == OK)
		    {
		        STcopy(rcTBuf, setFile);
		    }
		    break;
		case 3:
		    /* get name of batch queue */
		    STcopy(rcBuf, batchQueue);
		    break;
		case 4:
		    /* get starting time for batch job */
		    STcopy(rcBuf, startTime);
		    break;
		case 5:
#else
		case 2:
#endif
		    /* get name of list of tests */
		    if ((ioerr = check_filename(rcBuf, rcTBuf, message, &flag,
		                                TRUE)) == OK)
		    {
		        STcopy(rcTBuf, testList);
		    }
		    break;
#ifdef VMS
		case 6:
#else
		case 3:
#endif
		    /* get directory where tests reside (if any) */
		    if (STcompare(rcBuf, ERx("unspecified")) == 0)
		    {
		        STcopy(rcBuf, testDir);
		    }
		    else if ((ioerr = check_outdir(rcBuf, rcTBuf, message))
		             == OK)
		    {
		        STcopy(rcTBuf, testDir);
		    }
		    break;

	    } /* end of switch */
	}
    }

    /* read information about part-subsystems */

    for (part_ID = 0, finished = FALSE; (!finished)&&(ioerr == OK); part_ID++)
    {
	if ((ioerr = SIgetrec(rcBuf, MAX_LOC, aptr)) != OK)
	{
	    STcopy(ErrWhileReading, message);
	    continue;
	}

	cptr = SEP_CMlastchar(rcBuf,0);
	*cptr = EOS;                            /* get rid of nl */
	CVupper(rcBuf);

	if (!(finished = (STcompare(rcBuf, ERx("TYPES")) == 0)))
	{
	    part = (PART_NODE *) SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(*part),
		                             TRUE, &ioerr);
	    if (ioerr != OK)
	    {
		STcopy(ErrAlloc, message);
	    }
	    else
	    {
		part->partname = STtalloc(SEP_ME_TAG_NODEL, rcBuf);
		part->partnum = 0;
		part->subs[0] = NULL;
		ing_parts[part_ID] = part;

		/*
		** read subsystems from PART to be executed
		*/

		ioerr = read_subs(ing_parts[part_ID], message, aptr);
	    }
	}
    }

    if (ioerr == OK)
    {
	/*
	** get legal types
	*/
	part = (PART_NODE *) SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(*part), TRUE,
		                         &ioerr);
	if (ioerr != OK)
	{
	    STcopy(ErrAlloc, message);
	}
	else
	{
	    part->subs[0] = NULL;
	    part->partnum = 0;
	    ing_types = part;

	    ioerr = read_subs(ing_types, message, aptr);
	}
    }

    if (ioerr != OK)
    {
	return (ioerr);
    }

    /*
    ** get legal levels
    */

    if ((ioerr = SIgetrec(rcBuf, MAX_LOC, aptr)) == OK)
    {
	cptr = SEP_CMlastchar(rcBuf,0);
	*cptr = EOS;                    /* get rid of nl */
	CVupper(rcBuf);
    }

    if ((ioerr != OK) || (STcompare(rcBuf, ERx("LEVELS"))))
    {
	STcopy(ErrWhileReading, message);
	ioerr = (ioerr == OK) ? FAIL : ioerr ;
    }
    else
    {
	part  = (PART_NODE *)
		SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(*part), TRUE, &ioerr);

	if (ioerr != OK)
	{
	    STcopy(ErrAlloc, message);
	}
	else
	{
	    part->subs[0] = NULL;
	    part->partnum = 0;
	    ing_levels = part;

	    if ((ioerr = read_subs(ing_levels, message, aptr)) == OK)
	    {
		SIclose(aptr);
	    }
	}
    }

    MEfree(rcBuf);
    MEfree(rcTBuf);
    return(ioerr);
}

STATUS
read_subs ( PART_NODE *part, char *message, FILE *aptr )
{
    char                 **subs = part->subs ;
    i4                    *init = part->subinit ;
    i4                    *run = part->subrun ;
    i4                    *difs = part->subdif ;
    i4                    *abd = part->subabd ;
    i4                     count = -1 ;
    char                  *coma ;
    char                  *cptr ;
    char                   buffer [MAX_LOC] ;
    STATUS                 ioerr ;

    ioerr = SIgetrec(buffer, MAX_LOC, aptr);
    if (ioerr != OK)
    {
	STcopy(ErrWhileReading, message);
    }
    else
    {
        cptr = SEP_CMlastchar(buffer,0);
        *cptr = EOS;                                /* get rid of nl */
        CVupper(buffer);
    
        if (STcompare(buffer, ERx("NONE")) == 0)
        {
            ioerr = FAIL;
            if (part == ing_levels)
            {
		STcopy(ERx("ERROR: no levels were selected"), message);
            }
            else if (part == ing_types)
            {
		STcopy(ERx("ERROR: no types were selected"), message);
            }
            else
            {
                ioerr = OK;
            }
        }
    
        for (cptr = buffer; (ioerr == OK)&&(cptr != NULL)&&(*cptr != EOS);
             cptr = coma)
        {
            coma = STindex(cptr, ERx(","), 0);
            if (coma)
            {
                *coma = EOS;
                 CMnext(coma);
            }

            subs[++count] = STalloc(cptr);
            init[count]   = 0;
            run[count]    = 0;
            difs[count]   = 0;
            abd[count]    = 0;
        }
    }

    if (ioerr == OK)
    {
	subs[++count] = NULL;
    }

    return(ioerr);
}


STATUS
parse_list_line( char *theline, char **theparts, char *dirOfTest )
{
    STATUS                 ret_val = OK ;

    char                  *cptr = theline ;
    char                  *colon ;

    i4                     count = -1 ;
    i4                     partOfLine = 0 ;

    for (;;)
    {
	while ((*cptr != EOS) && CMwhite(cptr))
		CMnext(cptr);

	if (*cptr == EOS)
		break;

	if (colon = STindex(cptr, ERx(":"), 0))
	{
	    *colon = EOS;
	    CMnext(colon);
	}

	switch (++partOfLine)
	{
	   case 1:
		    get_test_class(cptr, theparts, &count);
		    if (count != 3)
		        return(FAIL);

		    break;
	   case 2:
		    if (dirOfTest)
		        if (get_test_dir(cptr, dirOfTest) != OK)
		            return(FAIL);

		    break;
	   case 3:
		    theparts[++count] = cptr;
		    break;
	   case 4:
		    if (get_test_flags(cptr, theparts, &count) != OK)
		        return(FAIL);

		    break;
	}
	if ((cptr = colon) == NULL)
	    break;
    }

    ret_val = (partOfLine >= 3) ? OK : FAIL ;

    if ((ret_val == OK)&&(partOfLine == 3))
    {
	theparts[5] = NULL;
    }

    return (ret_val);
}

STATUS
get_test_class ( char *theline, char **theparts, i4  *count )
{
    STATUS                 ret_val = OK ;

    char                  *cptr = theline ;
    char                  *coma ;

    register i4            i ;

    for (i = *count+1; (cptr != NULL)&&(*cptr != EOS); i++)
    {
	while ((*cptr != EOS) && CMwhite(cptr))
	{
	    CMnext(cptr);
	}

	if (*cptr == EOS)
	{
	    ret_val = FAIL;
	}
	else
	{
            if (coma = STindex(cptr, ERx(","), 0))
            {
                *coma = EOS;
                CMnext(coma);
            }
    
            STtrmwhite(cptr);
            theparts[i] = cptr;
            cptr = coma;
	}
    }
    *count = i-1;

    return (ret_val);
}

STATUS
get_test_dir ( char *theline, char *thedir )
{
    STATUS                 ret_val = OK ;

    char                  *cptr = theline ;
    char                  *coma ;

    LOCATION               aloc ;

    SEP_LOfroms(PATH, thedir, &aloc, SEP_ME_TAG_NODEL);

    while ((cptr != NULL)&&(*cptr != EOS))
    {
	while ((*cptr != EOS) && CMwhite(cptr))
	{
	    CMnext(cptr);
	}

	if (*cptr != EOS)
	{
            if (coma = STindex(cptr, ERx(","), 0))
            {
                *coma = EOS;
                CMnext(coma);
            }
    
            STtrmwhite(cptr);
            LOfaddpath(&aloc, cptr, &aloc);
            cptr = coma;
	}
    }

    LOtos(&aloc, &cptr);
    STcopy(cptr, thedir);

    return (ret_val);
}

STATUS
get_test_flags ( char *theline, char **theparts, i4  *count )
{
    STATUS                 ret_val = OK ;

    char                  *cptr = theline ;
    char                  *coma ;

    register i4            i ;

    for (i = *count+1; (cptr != NULL)&&(*cptr != EOS); i++ )
    {
	while ((*cptr != EOS)&&CMwhite(cptr))
	{
	    CMnext(cptr);
	}

	if (*cptr != EOS)
	{
            if (coma = STindex(cptr, ERx(","), 0))
            {
                *coma = EOS;
                CMnext(coma);
            }
    
            STtrmwhite(cptr);
            theparts[i] = cptr;
            cptr = coma;
	}
    }
    theparts[i] = NULL;
    *count = i;

    return (ret_val);
}

STATUS
check_outdir ( char *dirname, char *newdirname, char *message )
{
    STATUS                 ret_val = OK ;

    LOCATION               aloc ;

    LOCTYPE                typeLoc ;

    i2                     flag ;


    if (getLocation(dirname, newdirname, &typeLoc) != OK)
    {
	STcopy(dirname, newdirname);
    }
    else
    {
	if (typeLoc != PATH)
	{
	    STprintf(message,ERx("ERROR: %s is not a directory"), dirname);
	    ret_val = FAIL;
	}
    }

    if (ret_val == OK)
    {
        SEP_LOfroms(PATH, newdirname, &aloc, SEP_ME_TAG_NODEL);
    
        if (!SEP_LOexists(&aloc))
        {
            STprintf(message,ERx("ERROR: directory %s was not found"),
		     newdirname);
            ret_val = FAIL;
        }
        else
        {
            LOisdir(&aloc, &flag);
    
            if (flag != ISDIR)
            {
                STprintf(message,ERx("ERROR: %s is not a directory"),
			 newdirname);
                ret_val = FAIL;
            }
            else if (LOisfull(&aloc) == FALSE)
            {
                STprintf(message,
			 ERx("ERROR: directory %s is not a full pathname"),
                         newdirname);
                ret_val = FAIL;
            }
        }
    }

    return (ret_val);
}

STATUS
check_filename ( char *filenm, char *newfilenm, char *message, bool *isfull,
		 bool isthere )
{
    STATUS                 ret_val = OK ;

    LOCATION               aloc ;

    LOCTYPE                typeLoc ;

    if (getLocation(filenm, newfilenm, &typeLoc) == FAIL)
    {
	STcopy(filenm, newfilenm);
	SEP_LOfroms(FILENAME & PATH, filenm, &aloc, SEP_ME_TAG_NODEL);
	if (isthere && !SEP_LOexists(&aloc))
	{
	    STprintf(message,ERx("ERROR: file %s was not found"),filenm);
	    ret_val = FAIL;
	}
	else
	{
	    *isfull = LOisfull(&aloc);
	}
    }
    else
    {
	if (typeLoc == PATH)
	{
	    STprintf(message,
		  ERx("ERROR: %s is not a file name"), newfilenm);
	    ret_val = FAIL;
	}
	else
	{
	    SEP_LOfroms(typeLoc, newfilenm, &aloc, SEP_ME_TAG_NODEL);
	    if (isthere && !SEP_LOexists(&aloc))
	    {
		STprintf(message,
			 ERx("ERROR: file %s was not found"), newfilenm);
		ret_val = FAIL;
	    }
	    else
	    {
		*isfull = (typeLoc == (FILENAME & PATH));
	    }
	}
    }

    return (ret_val);
}

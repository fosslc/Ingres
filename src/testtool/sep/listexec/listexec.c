/*
**	History:
**		24-aug-89 (kimman)
**		    Added new libraries to NEEDLIBS list: FTLIB UGLIB
**		27-Oct-1989 (fredv)
**		    Added ming hint not to optimize this file for RT.
**		    Listexec simply dumped core with optimization.
**		06-dec-89 (mca)
**		    In init_parts(), corrected type of variable aptr from
**		    FILE to (FILE *). This error went unnoticed until it
**		    caused the parts file to go unread on Sun4.
**		30-mar-90 (seng)
**		    The filenames for some listexec files were too long,
**		    specifically the "<part>subs.lis" files in $TST_LISTEXEC.
**		    Shorten the names of these files to "<part>.subs".
**		    NOTE TO PV:  This is a change that is NOT BACKWARDS
**		    COMPATIBLE to the "<part>subs.lis" naming convention.
**		24-apr-90 (chieu)
**		    Routine check_selections() returned FAIL when no test has
**		    been selected but did not return a defined status code
**		    when some tests have been selected.  It now returns OK
**		    for the latter case.
**		24-apr-90 (bls)
**		    integrated Sep 3.0 code line with NSE (porting) code line
**		    added ming hints; verifed change to declaration of aptr in
**		    init_parts()
**		11-may-90 (bls)
**		    added recent porting changes to shorten filenames
**		    (from porting group code line).
**		24-may-90 (rog)
**		    Changed SEPWINLIB to FTLIB UGLIB.
**		24-may-90 (rog)
**		    Make sure functions return OK if they're supposed to
**		    return a status and they succeed.
**		14-jun-90 (rog)
**		    Fix typo in error message.
**		18-jun-90 (rog)
**		    Check II_SYSTEM on both UNIX and VMS.
**		25-jun-1990 (rog)
**		    Make batchMode a bool in all source files.
**		09-Jul-1990 (owen)
**		    Use angle brackets on sep header files in order to search
**		    for them in the default include search path.
**		
**		21-feb-1991 (gillnh2o)
**		    Per jonb, added LIBINGRES to NEEDLIBS and remove every-
**		    thing else that isn't built out of SEP. This may obviate
**		    the need to change NEEDLIBS for every new release.
**		14-nov-1991 (lauraw)
**		    Added a "wait for process to finish" flag (-w) so 
**		    listexec can be controlled from a Unix shell script.
**		    (-w is for Unix only.)
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**      25-feb-1992 (donj)
**          Added globalrefs to tracing, trace_nr and traceptr. These tracing
**          vars are used by SEPspawn() in SEPTOOL.EXE. They're here to
**          provide that reference only.
**      02-apr-1992 (donj)
**          Changed all refs to '\0' to EOS. Changed more char pointer
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)"
**	15-apr-1992 (donj)
**	    Add SEP_cmd_line() to make command line parsing more uniform
**	    amoungst SEPTOOL,LISTEXEC and EXECUTOR. Changed more code to use
**	    CM routines. Moved #include's to after header text.
**      05-jul-92 (donj)
**          Added SEP version code to allow listexec and executor to know
**          which SEP version it was built against. Listexec and Executor's
**          version will now be the same as the SEP version. Added to sepflags
**          option to pass command line flags to each SEP test. Trimmed compat
**          library out of NEEDLIBS of the ming hints. Not needed. This will
**          allow listexec and executor to be built on UNIX w/o having
**          libcompat.a. Libcompat.a is not delivered with the UNIX tar file.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**      21-oct-1992 (donj)
**	    Changed put_message() to lput_message() to avoid symbol def clashes.
**	    Added better code for finding the ingresprt.lis, ingreslvl.lis and
**	    ingrestyp.lis files.
**	30-nov-1992 (donj)
**	    Modularize code a bit better. Make Listexec look for the ingresprt.lis,
**	    ingreslvl.lis and ingrestyp.lis in a more appropriate way. Also handle
**	    partial and/or relative directory specifications better. Handle "@file()"
**	    translations better. Don't rewrite the *.cfg file unless something has
**	    changed about it.  Took out local getLocation(), uses SEP's one now.
**	01-dec-1992 (donj)
**	    Sun4 didn't like trying to autoinit the array of char pointers,
**	    dir_path[] in Find_file_locs().
**	23-feb-1993 (donj)
**	    OVERLAYSP needed globaldef'ing for VMS
**	15-feb-1993 (donj)
**	    Add a ME_tag arg to SEP_LOfroms.
**	19-feb-1993 (donj)
**	    VMS LO functions don't like filetypes of NODE & PATH when only PATH.
**	2-mar-1993 (donj)
**	    The config file area is a possible location for listexec parameter
**	    files like, ingresprt.lis, ingrestyp.lis, ingreslvl.lis and the
**	    *.subs files. But we need just the path and not the filename for
**	    SEP_LOffind().
**      31-mar-1993 (donj)
**          Add a new field to OPTION_LIST structure, arg_required. Used
**          for telling SEP_Cmd_Line() to look ahead for a missing argument.
**       2-jun-1993 (donj)
**          Took out unused arg from SEP_cmd_line.
**      14-aug-1993 (donj)
**          changed cmdopt from an array of structs to a linked list of structs.
**          added calls to SEP_alloc_Opt() to create linked list. Added a call
**          to MEadvise() as per CL requirements.
**	16-aug-1993 (donj)
**	    Reflect changes in SEP_Vers_Init(), *sep_version and *sep_platform.
**	14-sep-1993 (dianeh)
**	    Removed NO_OPTIM setting for obsolete PC/RT port (rtp_us5).
**       2-dec-1993 (donj)
**          Added testGrade GLOBALDEF here. It's needed by the expression
**          evaluator which resides in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**	14-dec-1993 (donj)
**	    Added Status_Value GLOBALDEF here. It's needed by the expression
**          evaluator which resides in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**      30-dec-1993 (donj)
**          Added an include of a dummy.h file that has GLOBALDEFS for things
**          that are referenced in a library common to SEP, EXECUTOR and
**          LISTEXEC.
**	 4-jan-1994 (donj)
**	    Moved dummy batchMode bool to dummy.h.
**	31-jan-1994 (kirke)
**	    CMnext() only operates on char *, not char **.  Fixed usage of
**	    hptr accordingly.
**	13-apr-1994 (donj)
**	    Fix return type of run_tests() to STATUS.
**	10-may-1994 (donj)
**	    Added include of fndefs.h, removed any declarations that exist in
**	    fndefs.h.
**
**	    Changed the name of a local function, get_answer(), to get_answer2()
**	    so it wouldn't conflict with another get_answer() that SEP uses.
**	26-may-1994 (donj)
**	    Went further with get_answer2(). Created a common Get_Answer() and
**	    dumped get_answer2().
**	26-may-1994 (donj)
**	    Fix the mangled first comment line.
**
**	04-oct-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
    Include files
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <nm.h>
#include <me.h>
#include <er.h>
#include <cm.h>
#include <tc.h>
#include <te.h>
#include <cv.h>
#include <pc.h>
#include <termdr.h>

#define listexec_c

#include <listexec.h>
#include <sepmenu.h>
#include <sepdefs.h>

#include <fndefs.h>

#include <dummy.h>

/*
    global variables
*/

GLOBALDEF    char          confFile [MAX_LOC] ;
GLOBALDEF    char         *confFile_path = NULL ;
GLOBALDEF    char          outDir [MAX_LOC] ;
GLOBALDEF    char          testList [MAX_LOC] ;
GLOBALDEF    char          testDir [MAX_LOC] ;
GLOBALDEF    char          setFile [MAX_LOC] ;
GLOBALDEF    char          setFile_trans [MAX_LOC] ;
#ifdef VMS
GLOBALDEF    char          batchQueue [MAX_LOC] ;
GLOBALDEF    char          startTime [MAX_LOC] ;
#endif
GLOBALDEF    PART_NODE    *ing_parts [MAX_PARTS] ;
GLOBALDEF    PART_NODE    *ing_types = NULL ;
GLOBALDEF    PART_NODE    *ing_levels = NULL ;

GLOBALDEF    LOCATION      ING_PARTS_LOC ;
GLOBALDEF    LOCATION      ING_TYPES_LOC ;
GLOBALDEF    LOCATION      ING_LEVELS_LOC ;
GLOBALDEF    LOCATION      currLoc ;

GLOBALDEF    OPTION_LIST  *cmdopt ;

GLOBALDEF    char          currBuf [MAX_LOC] ;

char                      *ING_PARTS_BUF ;
char                      *ING_TYPES_BUF ;
char                      *ING_LEVELS_BUF ;

GLOBALDEF    char          listDir [MAX_LOC] ;
GLOBALDEF    bool          Rewrite_Cfg = FALSE ;
/*
** SEP version number. This string is displayed when SEP is started in
** non-batch mode.
*/
GLOBALDEF    char         *sep_platform = NULL ;
GLOBALDEF    char         *le_version = NULL ;

/*
    tracing vars.
*/

GLOBALDEF    i4            tracing = 0 ;
GLOBALDEF    i4            trace_nr = 0 ;
GLOBALDEF    FILE         *traceptr = NULL ;
GLOBALDEF    char         *trace_file ;
GLOBALDEF    LOCATION      traceloc ;
GLOBALDEF    i4            testGrade = 0 ;
GLOBALDEF    i4            Status_Value = 0 ;
/*
    values to be passed to the executor
*/

GLOBALDEF    char          testStyle [3] = { EOS, EOS, EOS } ;
GLOBALDEF    char          userName [20] ;
GLOBALDEF    char          SepFlag_str[20] ;
GLOBALDEF    i4            diffLevel = 0 ;
GLOBALDEF    i4            jobLevel  = 0 ;
GLOBALDEF    bool          interactive = FALSE ;
GLOBALDEF    bool          SepFlags  = FALSE ;
#if defined(UNIX) || defined(NT_GENERIC)
GLOBALDEF    bool          waitFor = FALSE;
#endif

/*
    external functions
*/

FUNC_EXTERN  i4            testparams () ;
FUNC_EXTERN  STATUS        run_tests () ;

/* 
**      ming hints 
** 
NEEDLIBS = SEPLIB SEPDIFFLIB LISTEXECLIB SEPCLLIB LIBINGRES 
*/

char                       newfname [MAX_LOC] ;
LOCATION                   newfnloc ;
char                      *tempPtr ;

main(argc,argv)
i4    argc;
char *argv[];
{
    STATUS                 return_val ;
    WINDOW                *mainW ;
    WINDOW                *promptW ;
    char                   buffer [MAX_LOC] ;
    char                   errmess [MAX_LOC] ;
    char                  *cptr ;
    i4                     get_menu_choice () ;
    i4                     choice ;
    bool                   runthem = FALSE ;
    bool                   is_fullpath ;
    STATUS                 init_parts () ;
    STATUS                 init_subs () ;
    STATUS                 init_typlvl () ;
    STATUS                 chk_fname () ;
    STATUS                 check_outdir () ;
    STATUS                 check_selections () ;
    STATUS                 Find_file_locs () ;
    STATUS                 get_args () ;
    STATUS                 read_confile () ;

    VOID                   display_screen () ;
    VOID                   init_globals () ;
    VOID                   lput_message () ;

    /*
    **	open stdin and stdout
    */

    if (SIeqinit() != OK)
    {
        PCexit(FAIL);
    }

    /*
    *  Work around a Unix INGRES bug in NMgtAt - it will dump core if II_SYSTEM
    *  is not set.  Also, the TD routines need II_SYSTEM to set the terminal
    *  definitions.
    */
    if (getenv(ERx("II_SYSTEM")) == NULL)
    {
	   SIprintf(ERx("II_SYSTEM is not defined.\n"));
	   PCexit(-1);
    }

    /*
    **	get value of TST_LISTEXEC
    */

    NMgtAt(ERx("TST_LISTEXEC"), &cptr);
    if ((cptr == NULL) || (*cptr == EOS))
    {
	SIprintf(ERx("ERROR: environment variable TST_LISTEXEC is not defined\n\n\n"));
	PCexit(FAIL);
    }
    else
    {
	STcopy(cptr, listDir);
    }

    /*
    **	Initializations
    */

/* ************************************************************************************* */
/* ******   option name   ** returned **  returned   ** returned ** returned ** Argument */
/* ******                 **  string  **    bool     ** integer  ** float    ** Required */
/* ************************************************************************************* */
    SEP_alloc_Opt(&cmdopt, ERx("difflevel"),   NULL,        NULL,        &diffLevel, NULL, TRUE );
    SEP_alloc_Opt(&cmdopt, ERx("tolerance"),   NULL,        NULL,        &jobLevel,  NULL, TRUE );
    SEP_alloc_Opt(&cmdopt, ERx("batch"),       NULL,       &batchMode,    NULL,      NULL, FALSE);
    SEP_alloc_Opt(&cmdopt, ERx("interactive"), NULL,       &interactive,  NULL,      NULL, FALSE);
#if defined(UNIX) || defined(NT_GENERIC)
    SEP_alloc_Opt(&cmdopt, ERx("waitfor"),     NULL,       &waitFor,      NULL,      NULL, FALSE);
#endif
    SEP_alloc_Opt(&cmdopt, ERx("style"),       testStyle,   NULL,         NULL,      NULL, TRUE );
    SEP_alloc_Opt(&cmdopt, ERx("username"),    userName,    NULL,         NULL,      NULL, TRUE );
    SEP_alloc_Opt(&cmdopt, ERx("flags"),       SepFlag_str,&SepFlags,     NULL,      NULL, TRUE );

    SEP_Vers_Init(ERx("Listexec"), &le_version, &sep_platform);
    init_globals();

    /*
    **	Getting arguments from command line
    */

    if (get_args(argc,argv,buffer) != OK)
    {
        SIprintf(ERx("%s\n\n\n"),buffer);
        PCexit(0);
    }

    if ((return_val = Find_file_locs()) != OK)
    {
	SIprintf(ERx("ERROR looking for parts,types or levels files.\n\n\n"));
	PCexit (0);
    }

    if (init_parts() == OK)
    {
	choice = 0;
	while (ing_parts[choice])
	{
	    if (init_subs(ing_parts[choice]) == OK)
	    	choice++;
	    else
	    {

		/* merged from porting group codeline 5-11-90, bls */

		SIprintf(ERx("ERROR while accessing %s.subs\n\n\n"),
			ing_parts[choice]->partname);
		PCexit(0);
	    }
	}
    }
    else
    {
	SIprintf(ERx("ERROR while accessing %s\n\n\n"),ING_PARTS);
	PCexit(0);
    }

    if (init_typlvl(ING_TYPES,&ing_types) != OK)
    {
	SIprintf(ERx("ERROR while accessing %s\n\n\n"), ING_TYPES);
	PCexit(0);
    }
    if (init_typlvl(ING_LEVELS,&ing_levels) != OK)
    {
	SIprintf(ERx("ERROR while accessing %s\n\n\n"), ING_LEVELS);
	PCexit(0);
    }

    read_confile(buffer);

    /*
    **	initialize screen and draw main menu
    */

    if (!batchMode)
    {
    	TDinitscr();
    	mainW = TDnewwin(LINES - 2,COLS,0,0);
    	promptW = TDnewwin(2,COLS,LINES - 2,0);
    	display_screen(mainW);
    }

    /*
    **	get choices from main menu and execute them
    **	until QUIT or SUBMIT is selected
    */

    for (;;)
    {
	choice = batchMode ? GO_RUN :
		     get_menu_choice(mainW,promptW,20,5,REN_CONFGN,QUIT_RUN);
	switch (choice)
	{
	    /*
	    **	rename configuration file
	    */
	    case REN_CONFGN:


		SEP_Get_Answer(promptW, 0,0, ERx("Enter file name: "), buffer,
			       NULL, NULL );

		if (chk_fname(buffer, newfname, errmess, &is_fullpath, FALSE,
			      NULL) != OK)
		{
		    lput_message(promptW,errmess);
		}
		else
		{
		    if (is_fullpath)
		    {
			STcopy(newfname, buffer);
			SEP_LOfroms(PATH, buffer, &newfnloc, SEP_ME_TAG_NODEL);
		    }
		    else
		    {
			LOcopy(&currLoc, buffer, &newfnloc);
			LOfstfile(newfname, &newfnloc);
			SEP_LOrebuild(&newfnloc);
		    }
		    LOtos(&newfnloc, &tempPtr);
		    if (STbcompare(tempPtr, 0, confFile, 0, FALSE) != 0)
		    {
			Rewrite_Cfg = TRUE;
			STcopy(tempPtr, confFile);
			if (confFile_path != NULL)
			{
			    MEfree(confFile_path);
			    confFile_path = NULL;
			}
		    }
		}
		display_screen(mainW);
		break;

	    /*
	    **	select output directory
	    */

	    case OUTPUT_DIR:
		SEP_Get_Answer( promptW, 0, 0, ERx("Enter directory name: "),
				buffer, NULL, NULL );

		if (check_outdir(buffer, errmess) == OK)
		{
		    if (STbcompare(buffer, 0, outDir, 0, FALSE) != 0)
		    {
			Rewrite_Cfg = TRUE;
			STcopy(buffer, outDir);
		    }
		}
		else
		{
		    lput_message(promptW,errmess);
		}
		display_screen(mainW);
		break;

#ifdef VMS

	    /*
	    **	enter setup file to be executed before tests run
	    */

	    case SETUP_FILE:
		SEP_Get_Answer(promptW, 0,0, ERx("Enter file name: "), buffer,
			       NULL, NULL );
		if (chk_fname(buffer, newfname, errmess, &is_fullpath,
		    TRUE, NULL) != OK)
		{
		    lput_message(promptW,errmess);
		}
		else
		{
		    if (is_fullpath)
		    {
			SEP_LOfroms(PATH, buffer,&newfnloc, SEP_ME_TAG_NODEL);
		    }
		    else
		    {
			LOcopy(&currLoc, buffer, &newfnloc);
			LOfstfile(newfname, &newfnloc);
			SEP_LOrebuild(&newfnloc);
		    }
		    LOtos(&newfnloc, &tempPtr);
		    if (STbcompare(buffer, 0, setFile, 0, FALSE) != 0)
		    {
			Rewrite_Cfg = TRUE;
			STcopy(buffer,  setFile);
			STcopy(tempPtr, setFile_trans);
		    }
		}
		display_screen(mainW);
		break;

	    /*
	    **	enter name of batch queue
	    */

	    case BATCH_QUEUE:
		SEP_Get_Answer(promptW, 0,0, ERx("Enter queue name: "),
			       batchQueue, NULL, NULL );

		display_screen(mainW);
		break;

	    /*
	    **	enter starting time for batch job
	    */

	    case START_TIME:
		SEP_Get_Answer(promptW, 0,0, ERx("Enter starting time: "),
			       startTime, NULL, NULL );

		display_screen(mainW);
		break;

#endif
	    /*
	    **	enter list of tests to be executed
	    */

	    case TESTS_LIST:
		SEP_Get_Answer(promptW, 0,0, ERx("Enter file name: "), buffer,
			       NULL, NULL );

		if (chk_fname(buffer, newfname, errmess, &is_fullpath,
				   TRUE, NULL) != OK)
		{
		    lput_message(promptW,errmess);
		}
		else
		{
		    if (is_fullpath)
		    {
			SEP_LOfroms(PATH, buffer, &newfnloc, SEP_ME_TAG_NODEL);
		    }
		    else
		    {
			LOcopy(&currLoc, buffer, &newfnloc);
			LOfstfile(newfname, &newfnloc);
			SEP_LOrebuild(&newfnloc);
		    }
		    LOtos(&newfnloc, &tempPtr);
		    if (STbcompare(tempPtr, 0, testList, 0, FALSE) != 0)
		    {
			Rewrite_Cfg = TRUE;
			STcopy(tempPtr, testList);
		    }
		}
		display_screen(mainW);
		break;

	    case TESTS_DIR:
		SEP_Get_Answer(promptW, 0,0, ERx("Enter directory name: "),
			       buffer, NULL, NULL );

		if (check_outdir(buffer, errmess) == OK)
		{
		    if (STbcompare(buffer, 0, testDir, 0, FALSE) != 0)
		    {
			Rewrite_Cfg = TRUE;
			STcopy(buffer, testDir);
		    }
		}
		else
		{
		    lput_message(promptW,errmess);
		}
		display_screen(mainW);
		break;

	    /*
	    **	select tests out of list to be executed
	    */

	    case SELECT_MENU:
		testparams(promptW);
		TDtouchwin(mainW);
		TDrefresh(mainW);
		TDerase(promptW);
		TDrefresh(promptW);
		break;

	    /*
	    **	quit but first check if user wants to create
	    **	a configuration file
	    */

	    case QUIT_RUN:
		if (Rewrite_Cfg)
		{
		    SEP_Get_Answer(promptW, 0,0,
		      ERx("Do you want to create a configuration file (y/n)? "),
				   buffer, NULL, NULL );

		    if ((buffer[0] != 'y') && (buffer[0] != 'Y'))
			break;
		}
		choice++;

	    /*
	    **	submit tests to be executed
	    */

	    case GO_RUN:
		if (STcompare(ERx("unspecified"),outDir) == 0)
		    lput_message(promptW,ERx("Please enter output directory"));
		else if (STcompare(ERx("unspecified"),testList) == 0)
		    lput_message(promptW,ERx("Please enter list of tests"));
		else if (STcompare(ERx("unspecified"),setFile) == 0)
		    lput_message(promptW,ERx("Please enter setup file"));
#ifdef VMS
		else if ((STcompare(ERx("unspecified"),batchQueue) == 0) &&
			 (! interactive))
		    lput_message(promptW,
				 ERx("Please enter name of batch queue"));
#endif
		else if (check_selections(errmess) != OK)
		    lput_message(promptW, errmess);
		else if (--choice == QUIT_RUN)
		    return_val = run_tests(mainW,promptW,&currLoc,FALSE);
		else
		    runthem = TRUE;
		break;
	}

	if ((choice == QUIT_RUN) || runthem || batchMode)
	    break;

    }

    if (runthem)
    {
	/* run tests */

	return_val = run_tests(mainW,promptW,&currLoc,TRUE);
    }

    /*
    **	close the shop
    */

    if (!batchMode)
    {
    	TDdelwin(mainW);
    	TDdelwin(promptW);
    	TDendwin();
    	TEclose();
    }
    PCexit(0);
}


/*
**  routine: display_screen
**
**  use: display main menu
*/
VOID
display_screen(win)
WINDOW	*win;
{
    i4	row = 0;
    i4	choices = 0;

#define   Print_win_row(Menustr,str)    TDmvwprintw(win,++row,5,Menustr,str)

    TDerase(win);
    TDmvwprintw(win, row++, 10, le_version,Header1   );
    TDmvwprintw(win, row++, 10, Header2,   confFile  );

    Print_win_row(Menuline1, ++choices );
    Print_win_row(Menuline2, ++choices );
    Print_win_row(Menuvar,   outDir    );
#ifdef VMS
    Print_win_row(Menuline3, ++choices );
    Print_win_row(Menuvar,   setFile   );
    Print_win_row(Menuline4, ++choices );
    Print_win_row(Menuvar,   batchQueue);
    Print_win_row(Menuline5, ++choices );
    Print_win_row(Menuvar,   startTime );
#endif
    Print_win_row(Menuline6, ++choices );
    Print_win_row(Menuvar,   testList  );
    Print_win_row(Menuline7, ++choices );
    Print_win_row(Menuvar,   testDir   );
    Print_win_row(Menuline8, ++choices );
    row++;
    Print_win_row(Menugo,    ++choices );
    Print_win_row(Menuquit,  ++choices );
    TDrefresh(win);
}


/*
**  routine: get_menu_choice
**
**  use: get selections from given menu
**
**  parameters:
**  win		=>  main window
**  pwin	=>  message window
**  posy, posx	=> row and column for prompt
**  lower,upper	=> boundaries for value to get selected
*/
i4
get_menu_choice(win,pwin,posy,posx,lower,upper)
WINDOW *win;
WINDOW *pwin;
i4      posy;
i4      posx;
i4      lower;
i4      upper;
{
    VOID                   lput_message () ;
    char                   buffer [20] ;
    i4                     choice ;

    for (;;)
    {
	TDmvwprintw(win,posy,posx,ERx("Enter choice ? "));
	TDrefresh(win);
#ifndef NT_GENERIC
	TEwrite(CE,STlength(CE));
#endif
	TEflush();
	getstr(win,buffer);
	if ((STlength(buffer) > 2) || (CVan(buffer,&choice) != OK))
	{
	    lput_message(pwin,ERx("Sorry, that's an illegal entry"));
	    continue;
	}
	else
	{
	    if ((choice < lower) || (choice > upper))
	    {
		lput_message(pwin,ERx("Sorry, that's an illegal entry"));
		continue;
	    }
	    else
		return(choice);
	}
    }
}

/*
**  routine : lput_message
**
**  use : put message in message window
*/
VOID
lput_message(win,string)
WINDOW	*win;
char	*string;
{
    if (batchMode)
	SIprintf(ERx("\n%s\n\n"), string);
    else
    {
    	TDerase(win);
    	TDmvwprintw(win, 0, 0, ERx("%s"), string);
    	TDrefresh(win);
    }
}


extract_modules(part)
PART_NODE *part;
{
    char                   result [256] ;
    char                  *cptr = result ;
    char                 **subs = part->subs ;
    char                  *runkey = part->runkey ;
    i4                     count = 0 ;
    i4                     first = 1 ;

    while (subs[count])
    {
	if (runkey[count] == 'Y')
	{
	    if (first)
	    {
		STcopy(subs[count],cptr);
		first = 0;
	    }
	    else
	    {
		STpolycat(2,ERx(","),subs[count],cptr);
	    }
	    cptr += STlength(cptr);
	}
	MEfree((PTR)subs[count]);
	count++;
    }
    *cptr = EOS;
    if (result[0] != EOS)
	part->substorun= STalloc(result);
}



STATUS
check_module(part)
PART_NODE   *part;
{
    char                 **subs = part->subs ;
    char                  *runkey = part->runkey ;
    i4                     count = 0 ;

    while (subs[count])
    {
	if (runkey[count++] == 'Y')
	{
	    return(OK);
	}
    }
    return(FAIL);
}

STATUS
init_parts()
{
    PART_NODE             *part ;
    FILE                  *aptr ;
    char                   buffer [80] ;
    char                  *cptr ;
    STATUS                 ioerr ;
    STATUS                 meerr = OK ;
    i4                     count = 0 ;
    register i4            i ;

    ing_parts[0] = NULL;
    if (SIopen(&ING_PARTS_LOC, ERx("r"), &aptr) != OK)
    {
	return(FAIL);
    }
    while (((ioerr = SIgetrec(buffer, 80, aptr)) == OK) && (meerr == OK))
    {
	CVupper(buffer);
	cptr  = SEP_CMlastchar(buffer, 0);
	*cptr = EOS;        /* get rid of nl */

	part = (PART_NODE *) SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(*part), TRUE,
					 &meerr);
	if (meerr == OK)
	{
	    part->partname = STalloc(buffer);
	    part->subs[0] = NULL;
	    for (i=0; i < MAX_SUBS; i++)
		part->runkey[i] = 'N';
	    part->substorun = NULL;
	    ing_parts[count++] = part;	    
	}	
    }
    if ((ioerr != ENDFILE) || (meerr != OK))
    {
	SIclose(aptr);
	return(FAIL);
    }
    else
    {
	SIclose(aptr);
	ing_parts[count] = NULL;
	return(OK);
    }

}


STATUS
init_subs(part)
PART_NODE   *part;
{
    FILE                  *aPtr ;
    LOCATION               aLoc ;
    char                   buffer [80] ;
    register char        **hptr = part->subs ;
    char                  *temp ;
    STATUS                 ioerr ;

    LOtos(&ING_PARTS_LOC, &temp);
    STcopy(temp, newfname);
    SEP_LOfroms(FILENAME & PATH, newfname, &aLoc, SEP_ME_TAG_NODEL);
    STpolycat(2, part->partname, ERx(".subs"), buffer);
    CVlower(buffer);
    LOfstfile(buffer, &aLoc);
    if (!SEP_LOexists(&aLoc))
        return(FAIL);

    if (SIopen(&aLoc, ERx("r"), &aPtr) != OK)
    {
        return(FAIL);
    }
    while ((ioerr = SIgetrec(buffer,80,aPtr)) == OK)
    {
	CVupper(buffer);
	temp  = SEP_CMlastchar(buffer,0);
	*temp = EOS;        /* get rid of nl */
        temp  = STalloc(buffer);
        *hptr = temp;
	hptr++;
    }
    if (ioerr != ENDFILE)
    {
	return(FAIL);
    }
    else
    {
    	*hptr = NULL;
    	SIclose(aPtr);
	return(OK);
    }

}

STATUS
init_typlvl(filenm,partptr)
char	    *filenm;
PART_NODE   **partptr;
{
    PART_NODE             *part ;
    STATUS                 ioerr ;
    STATUS                 meerr ;
    LOCATION              *aLoc ;
    FILE                  *aPtr ;
    i4                     i ;
    char                   buffer [80] ;
    char                  *temp ;
    char                 **hptr ;
	    	    
    part = (PART_NODE *) SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(*part), TRUE,
				     &meerr);
    if (meerr != OK)
	return (FAIL);

    if (STcompare(filenm, ING_LEVELS) == 0)
    {
	part->partname = STtalloc(SEP_ME_TAG_NODEL, ERx("LEVEL OF"));
	aLoc = &ING_LEVELS_LOC;
    }
    else
    {
	part->partname = STtalloc(SEP_ME_TAG_NODEL, ERx("TYPE OF"));
	aLoc = &ING_TYPES_LOC;
    }

    part->subs[0] = NULL;
    for (i=0; i < MAX_SUBS; i++)
	part->runkey[i] = 'N';

    part->substorun = NULL;
    *partptr = part;	    

    if (SIopen(aLoc, ERx("r"), &aPtr) != OK)
        return(FAIL);

    hptr = part->subs;
    while ((ioerr = SIgetrec(buffer,80,aPtr)) == OK)
    {	
	CVupper(buffer);
	temp  = SEP_CMlastchar(buffer,0);
	*temp = EOS;        /* get rid of nl */
        temp  = STalloc(buffer);
        *hptr = temp;
	hptr++;
    }
    if (ioerr != ENDFILE)
	return(FAIL);

    *hptr = NULL;
    SIclose(aPtr);
    return(OK);
}

VOID
init_globals()
{
    STcopy(ERx("config.cfg"),confFile);
    STcopy(ERx("unspecified"),outDir);
    STcopy(ERx("unspecified"),testList);
    STcopy(ERx("unspecified"),testDir);
#ifdef VMS
    STcopy(ERx("unspecified"),setFile);
    STcopy(ERx("unspecified"),batchQueue);
    STcopy(ERx("00:00"),startTime);
#endif
    LOgt(currBuf, &currLoc);
}


STATUS
get_args(argc,argv,message)
i4      argc;
char    *argv[];
char    *message;
{
    char                  *param ;
    i4                     args ;
    STATUS                 gotfile = FAIL ;
    STATUS                 CVstat ;


    if ((CVstat = SEP_cmd_line(&argc, argv, cmdopt, ERx("-"))) != OK)
    {
	STprintf(message, "LISTEXEC command line is bad!!");
	return(FAIL);
    }

    if (argc > 1)
    {
        for (args = 1; args < argc; args++)
        {
            param = argv[args];
	    if (CMcmpcase(param,ERx("-")) == 0)
            {
		CMnext(param);
		STprintf(message,ERx("ERROR: Illegal parameter = -%s"), param);
		return(FAIL);
            }

	    if (gotfile == OK)
	    {
		STprintf(message,ERx("ERROR: illegal parameter = %s"),param);
		return(FAIL);
	    }

	    STcopy(param, confFile);
	    gotfile = OK;                   

        } /* end of for loop */
        
	if (batchMode && interactive)
		interactive = FALSE;

    }	/* end of if argc < 1 */
    
    return(OK);
}


STATUS
read_confile(message)
char *message;
{
    LOCATION               aloc ;
    FILE                  *aptr ;
    char                  *local_Buf ;
    char                  *local_Ptr ;
    STATUS                 ioerr ;
    STATUS                 chk_fname () ;
    STATUS                 check_outdir () ;
    bool                   is_fullpath ;
    i4                     count = 0 ;
    i4                     partID ;
    STATUS                 read_subs () ;

    local_Buf = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, NULL);
    STcopy(confFile, local_Buf);

    if (chk_fname(local_Buf, newfname, message, &is_fullpath, FALSE, &aloc)
	!= OK) return(FAIL);

    MEfree(local_Buf);
    LOtos(&aloc, &local_Buf);
    STcopy(local_Buf, confFile);

    if (SIopen(&aloc, ERx("r"), &aptr) != OK)
    {
	STprintf(message,ERx("ERROR: could not open configuration file"));
	return(FAIL);
    }

    /* start reading info from configuration file */

    while((ioerr = SIgetrec(local_Buf, MAX_LOC, aptr)) == OK)
    {
	local_Ptr  = SEP_CMlastchar(local_Buf,0);
	*local_Ptr = EOS;        /* get rid of nl */
	switch(++count)
	{
	    case 1: /* get output directory */

		if (check_outdir(local_Buf, message) != OK)
		{
		    SIclose(aptr);
		    return(FAIL);
		}
		STcopy(local_Buf, outDir);

		break;		
#ifdef VMS
	    case 2: /* get setup file */

		if (chk_fname(local_Buf, newfname, message, &is_fullpath,
				   TRUE, NULL) != OK) 
		{
		    SIclose(aptr);
		    return(FAIL);		    
		}

		if (!is_fullpath)
		{
		    STprintf(message, 
			     ERx("ERROR: incomplete pathname for setup file"));
		    return(FAIL);
		}

		STcopy(local_Buf,  setFile);
		STcopy(newfname, setFile_trans);

		break;

	    case 3: /* get name of batch queue */

		STcopy(local_Buf, batchQueue);

		break;

	    case 4: /* get starting time for batch job */

		STcopy(local_Buf, startTime);

		break;

	    case 5: /* get name of list of tests */
#else
	    case 2: /* get name of list of tests */
#endif
		if (chk_fname(local_Buf, newfname, message, &is_fullpath,
				   TRUE, NULL) != OK)
		{
		    SIclose(aptr);
		    return(FAIL);		    
		}

		if (!is_fullpath)
		{
		    STprintf(message, 
			  ERx("ERROR: incomplete pathname for list of tests"));
		    return(FAIL);
		}
		STcopy(local_Buf, testList);

		break;
#ifdef VMS
	    case 6: /* get directory where tests reside (if any) */
#else
	    case 3: /* get directory where tests reside (if any) */
#endif
		if ((check_outdir(local_Buf, message) != OK) &&
		    (STcompare(local_Buf, ERx("unspecified")) != 0))
		{
		    SIclose(aptr);
		    return(FAIL);
		}

		STcopy(local_Buf, testDir);

		break;		

	} /* end of switch */

#ifdef VMS
	if (count >= 6)
#else
	if (count >= 3)
#endif
		break;

    }        
    if (ioerr != OK)
    {
	STprintf(message,
		 ERx("ERROR: failed while reading in configuration file"));
	return(FAIL);
    }

    /* read information about part-subsystems */

    for (;;)
    {
	ioerr = SIgetrec(local_Buf, MAX_LOC, aptr);
	local_Ptr  = SEP_CMlastchar(local_Buf,0);
	*local_Ptr = EOS;        /* get rid of nl */
	CVupper(local_Buf);
	if ((ioerr != OK) || (STcompare(local_Buf, ERx("TYPES")) == 0))
	{
	    break;
	}
	count = 0;
	partID = -1;
	while (ing_parts[count])
	{
	    if (STcompare(ing_parts[count]->partname, local_Buf) == 0)
	    {
		partID = count;
		break;
	    }
	    count++;
	}
	if (partID < 0)
	{
	    STprintf(message,
		  ERx("ERROR: illegal part name found in configuration file"));
	    return(FAIL);
	}
	if (read_subs(ing_parts[partID],message, aptr) != OK)
	    return(FAIL);
    }
    
    if (ioerr != OK)
    {
	STprintf(message,
	  ERx("ERROR: failed while reading part-info from configuration file"));
	return(FAIL);
    }
    if (read_subs(ing_types, message, aptr) != OK)
    {
	return(FAIL);
    }
    ioerr = SIgetrec(local_Buf, MAX_LOC, aptr);
    local_Ptr  = SEP_CMlastchar(local_Buf,0);
    *local_Ptr = EOS;        /* get rid of nl */
    CVupper(local_Buf);
    if ((ioerr != OK) || (STcompare(local_Buf, ERx("LEVELS"))))
    {
	STprintf(message,
	  ERx("ERROR: error while reading level info from configuration file"));
	return(FAIL);
    }
    if (read_subs(ing_levels, message, aptr) != OK)
	return(FAIL);

    SIclose(aptr);
    MEfree(local_Buf);
    return(OK);
}


STATUS
read_subs(part, message, aptr)
PART_NODE *part;
char      *message;
FILE      *aptr;
{
    char                 **subs = part->subs ;
    char                  *runkey = part->runkey ;
    char                  *comma ;
    char                  *cptr ;
    char                   local_Buf [MAX_LOC+1] ;
    i4                     count ;
    STATUS                 ioerr ;

    ioerr = SIgetrec(local_Buf, MAX_LOC, aptr);
    if (ioerr != OK)
    {
	STprintf(message, ERx("ERROR: error while reading part-subs info from configuration file"));
	return(FAIL);
    }
    cptr  = SEP_CMlastchar(local_Buf,0);
    *cptr = EOS;        /* get rid of nl */
    CVupper(local_Buf);
    cptr  = local_Buf;
    for (;;)
    {
	comma = STindex(cptr, ERx(","), 0);
	if (comma != NULL)
	{
	    *comma = EOS;
	    CMnext(comma);
	}
	count = 0;
	while (subs[count])
	{
	    if (STcompare(subs[count], cptr) == 0)
	    {
		runkey[count] = 'Y';
		break;		
	    }
	    count++;
	}
	cptr = comma;
	if (cptr == NULL)
	{
	    break;
	}
    }

    return(OK);
}


STATUS
check_outdir(dirname, message)
char *dirname;
char *message;
{
    LOCATION               aloc ;
    LOCTYPE                typeLoc ;
    char                   newdirname [MAX_LOC] ;
    i2                     flag ;

    if (getLocation(dirname, newdirname, &typeLoc) != OK)
	STcopy(dirname, newdirname);
    else
    {
	if (typeLoc != PATH)
	{
	    STprintf(message,ERx("ERROR: %s is not a directory"), dirname);
	    return(FAIL);
	}
    }
    SEP_LOfroms(PATH, newdirname, &aloc, SEP_ME_TAG_NODEL);
    if (!SEP_LOexists(&aloc))
    {
	STprintf(message,ERx("ERROR: directory %s was not found"), newdirname);
	return(FAIL);
    }
    else
    {
	LOisdir(&aloc, &flag);
	if (flag == ISDIR)
	{
	    if (LOisfull(&aloc))
	    	return(OK);
	    else
	    {
		STprintf(message, 
		    ERx("ERROR: directory %s is not a full pathname"), 
		    newdirname);
		return(FAIL);
	    }
	}
	else
	{
	    STprintf(message,ERx("ERROR: %s is not a directory"), newdirname);
	    return(FAIL);
	}
    }
}


STATUS
chk_fname(filenm, newfilenm, message, isfull, isthere, aLoc)
char *filenm;
char *newfilenm;
char *message;
bool *isfull;
bool isthere;
LOCATION *aLoc;
{
    LOCATION               aloc ;
    LOCATION              *alocptr ;
    LOCTYPE                typeLoc ;
    char                  *tmpc ;

    if (aLoc == NULL)
	alocptr = &aloc;
    else
	alocptr = aLoc;

    if (getLocation(filenm, newfilenm, &typeLoc) == FAIL)
    {
	STcopy(filenm, newfilenm);
    	SEP_LOfroms(FILENAME & PATH, filenm, alocptr, SEP_ME_TAG_NODEL);
    	if (isthere && !SEP_LOexists(alocptr))
    	{
	    STprintf(message,ERx("ERROR: file %s was not found"),filenm);
	    return(FAIL);
    	}
    	else
    	{
	    LOtos(alocptr, &tmpc);
	    STcopy(tmpc, newfilenm);
	    *isfull = LOisfull(alocptr);
    	}
    }
    else
    {
	if (typeLoc == PATH)
	{
		STprintf(message, 
		  ERx("ERROR: %s is not a file name"), newfilenm);
		return(FAIL);
	}
	SEP_LOfroms(typeLoc, newfilenm, alocptr, SEP_ME_TAG_NODEL);
    	if (isthere && !SEP_LOexists(alocptr))
    	{
		STprintf(message, 
		  ERx("ERROR: file %s was not found"), newfilenm);
		return(FAIL);
    	}
	*isfull = (typeLoc == (FILENAME & PATH));
    }
    return(OK);
}



STATUS
check_selections(message)
char *message;
{
    i4                     i ;

    /*
    **	check if user has selected what tests to execute
    **	level and type
    */

    for (i = 0 ; ing_parts[i] ; i++)
    {
	if (check_module(ing_parts[i]) == OK)
		break;
    }
    if (ing_parts[i] == NULL)
    {
	STprintf(message, 
		ERx("ERROR: no tests have been selected for execution"));
	return(FAIL);
    }
    if (check_module(ing_types) != OK)
    {
	STprintf(message, 
		ERx("ERROR: no test types have been selected for execution"));
	return(FAIL);
    }
    if (check_module(ing_levels) != OK)
    {
	STprintf(message, 
		ERx("ERROR: no test levels have been selected for execution"));
	return(FAIL);
    }
    return(OK);
}

STATUS
Find_file_locs()
{
    STATUS                 r_val ;
    char		  *dir_path [4] ;
    char                  *tmp_dev ;
    char                  *tmp_path ;
    char                  *tmp_fpre ;
    char                  *tmp_fsuf ;
    char                  *tmp_fver ;
    LOCATION              *tmp_loc ;
    i4                     i ;


    i = 0;
    dir_path[i++] = currBuf;
    dir_path[i++] = listDir;

    if (confFile_path == NULL || *confFile_path == EOS)
    {
	tmp_dev  = SEP_MEalloc(SEP_ME_TAG_NODEL, LO_DEVNAME_MAX+1,  TRUE, NULL);
	tmp_path = SEP_MEalloc(SEP_ME_TAG_NODEL, LO_PATH_MAX+1,     TRUE, NULL);
	tmp_fpre = SEP_MEalloc(SEP_ME_TAG_NODEL, LO_FPREFIX_MAX+1,  TRUE, NULL);
	tmp_fsuf = SEP_MEalloc(SEP_ME_TAG_NODEL, LO_FSUFFIX_MAX+1,  TRUE, NULL);
	tmp_fver = SEP_MEalloc(SEP_ME_TAG_NODEL, LO_FVERSION_MAX+1, TRUE, NULL);
	tmp_loc  = (LOCATION *) SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(LOCATION)+1,
					    TRUE,NULL);

	SEP_LOfroms(FILENAME & PATH, confFile, tmp_loc, SEP_ME_TAG_NODEL);
	LOdetail(tmp_loc, tmp_dev,  tmp_path, tmp_fpre, tmp_fsuf, tmp_fver);

#ifdef VMS
	confFile_path = SEP_MEalloc(SEP_ME_TAG_NODEL,
				    STlength(tmp_dev)+STlength(tmp_path)+1,
				    TRUE, NULL);
	STpolycat(2,tmp_dev,tmp_path,confFile_path);
#endif
#if defined(UNIX) || defined(NT_GENERIC)
	confFile_path = STtalloc(SEP_ME_TAG_NODEL,tmp_path);
#endif

	MEfree(tmp_dev);
        MEfree(tmp_path);
        MEfree(tmp_fpre);
        MEfree(tmp_fsuf);
        MEfree(tmp_fver);
	MEfree(tmp_loc);
    }
    if (confFile_path != NULL && *confFile_path != EOS)
	dir_path[i++] = confFile_path;

    dir_path[i] = NULL;

    ING_PARTS_BUF  = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, NULL);
    ING_TYPES_BUF  = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, NULL);
    ING_LEVELS_BUF = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, NULL);

    if ((r_val = SEP_LOffind(dir_path,ING_PARTS,ING_PARTS_BUF,&ING_PARTS_LOC)) != OK)
	return (r_val);
    if ((r_val = SEP_LOffind(dir_path,ING_TYPES,ING_TYPES_BUF,&ING_TYPES_LOC)) != OK)
	return (r_val);

    return (SEP_LOffind(dir_path,ING_LEVELS,ING_LEVELS_BUF,&ING_LEVELS_LOC));
}

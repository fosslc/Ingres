#include <compat.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <pc.h>
#include <te.h>
#include <cv.h>
#include <er.h>
#include <termdr.h>
#include <listexec.h>
#include <params.h>

/*
** runtests.c
**
** History:
**	##-###-1989 (eduardo)
**		Created.
**	25-sep-1989 (mca)
**		Made change to reflect SEPspawn's changed parameter.
**	13-nov-1989 (mca)
**		Filename of log (.out) file is now determined by the
**		user-specified output directory, not the directory of
**		the config file.
**	14-nov-1989 (owen)
**		Listexec will no longer open the config file for write
**		when running in batch mode.
**	24-apr-1990 (bls)
**		integrated Sep 3.0 code line with NSE (porting) code line
**		no open for write on config file when in batch mode,
**	15-jun-1990 (rog)
**		Integrated VMS and UNIX versions.
**	25-jun-1990 (rog)
**		Make batchMode a bool in all source files.
**	09-Jul-1990 (owen)
**		Use angle brackets on sep header files in order to search
**		for them in the default include search path.
**      13-Nov-1990 (jonb)
**              Use EXECUTOR environment variable for the executor name,
**              if it's defined.
**	20-aug-1990 (owen)
**		Pulling in PV's submission (change number 290025) to:
**		Change some misleading messages and make sure the log file
**		goes to the output directory instead of the cfg directory.
**	21-aug-1990 (owen)
**		listexec will now recognize portable file syntax for the
**		output directory.
**	18-jun-1991 (donj)
**		Changes in PCcmdline made filling out the argument list
**		mandatory for VMS. Addition of long cl_err. Will use ifdefs
**		for the time being until I can test on UNIX.
**      14-nov-1991 (lauraw)
**              Waits for SEPspawn to finish if "waitFor" is set (Unix only)
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called.
**	15-apr-1992 (donj)
**	    Changed userName and testStyle to array references rather than
**	    pointer references for easier use with CM functions. Changed
**	    some "if (*ptr)"'s to "if (*ptr != EOS)"'s to fit coding standards.
**	05-jul-1992 (donj)
**	    Added to sepflags option to pass command line flags to each SEP
**	    test.
**	05-jul-1992 (donj)
**	    Fix a unix problem.
**	30-nov-1992 (donj)
**	    Only rewrite *.cfg file if something has changed.
**      15-feb-1993 (donj)
**          Add a ME_tag arg to SEP_LOfroms.
**      15-feb-1993 (donj)
**	    runtests.c doesn't define SE_ME_TAG_NODEL, zero will do.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	13-apr-1994 (donj)
**	    Fix return type for run_tests() to be STATUS. Make sure run_tests()
**	    always returns a STATUS.
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

FUNC_EXTERN  STATUS        extract_modules () ;

#if defined(UNIX) || defined(NT_GENERIC)
FUNC_EXTERN  STATUS        SEPspawn () ;
GLOBALREF    bool          waitFor ;
#endif

#ifdef VMS
FUNC_EXTERN  STATUS        getLocation () ;
#endif

GLOBALREF    char          confFile [] ;
GLOBALREF    char          testStyle [] ;
GLOBALREF    char          userName [] ;
GLOBALREF    char          SepFlag_str [] ;
GLOBALREF    i4            diffLevel ;
GLOBALREF    i4            jobLevel ;
GLOBALREF    bool          interactive ;
GLOBALREF    bool          SepFlags ;
GLOBALREF    bool          batchMode ;
GLOBALREF    bool          Rewrite_Cfg ;

FUNC_EXTERN  bool          SEP_LOexists () ;
FUNC_EXTERN  STATUS        SEP_LOfroms () ;
FUNC_EXTERN  STATUS        SEP_LOrebuild () ;

STATUS
run_tests (mwin, pwin, curLoc, executing)
WINDOW   *mwin;
WINDOW   *pwin;
LOCATION *curLoc;
bool      executing;
{
    register i4            i ;
    register i4            argnum ;
    register i4            row = 0 ;
    char                  *args [8] ;
    char                   confBuf [MAX_LOC] ;
    char                   testBuf [4] ;
    char                   userBuf [24] ;
    char                   diffBuf [12] ;
    char                   jobBuf [12] ;
    char                   logBuf [MAX_LOC] ;
    char                  *dot ;
    static char            executorName [MAX_LOC] = ERx("") ;
    LOCATION               confLoc ;
    LOCATION               logLoc ;
    FILE                  *confF ;
    STATUS                 ioerr = OK ;
    char                   aDev [MAX_LOC] ;
    char                   aPath [MAX_LOC] ;
    char                   aPref [MAX_LOC] ;
    char                   aSuf [5] ;
    char                   aVer [5] ;
    char                  *outPtr ;
    char                   tempBuf [MAX_LOC] ;
#if defined(UNIX) || defined(NT_GENERIC)
    PID                    dummy ;
#endif

#ifdef VMS
    LOCATION               outLoc ;
    FILE                  *logF ;
    long                   cl_err ;
#endif

    if (!batchMode)
    {
    	TDerase(mwin);
    	TDerase(pwin);
    	TDrefresh(pwin);
    	TDmvwprintw(mwin, row++, 0, ERx("Collecting data ..."));
    	TDrefresh(mwin);
    }
    else
    {
	SIprintf(ERx("Collecting data ...\n"));
	SIflush(stdout);
    }

    /*
    **	create configuration file to be passed as a parameter for
    **	the executor
    */

    for (i = 0 ; ing_parts[i] ; i++)
	extract_modules(ing_parts[i]);

    extract_modules(ing_types);
    extract_modules(ing_levels);

    if (!batchMode&&Rewrite_Cfg)
    {
    	TDmvwprintw(mwin, row++, 0, ERx("Writing configuration file ..."));
    	TDrefresh(mwin);
    }
    STcopy(confFile, confBuf);
    SEP_LOfroms(PATH & FILENAME, confBuf, &confLoc, 0);
    LOdetail(&confLoc,aDev,aPath,aPref,aSuf,aVer);

    if (SEP_LOexists(&confLoc))
    {
	if (!batchMode&&Rewrite_Cfg)
	{
		TDmvwprintw(mwin, row++, 0,
			    ERx("Overwriting configuration file %s ..."),
			    confFile);
    		TDrefresh(mwin);
	}
	else if (executing)
	{
		SIprintf(ERx("Using configuration file %s ...\n"), confFile);
		SIflush(stdout);
	}
    }
    else if (!batchMode&&Rewrite_Cfg)
    {
	TDmvwprintw(mwin, row++, 0, ERx("Writing configuration file %s..."),
		    confFile);
	TDrefresh(mwin);
    }

    /*
    ** If in batch mode, don't open the config file for write.
    */


    if (!batchMode && Rewrite_Cfg)
    {
	    if ((ioerr = SIopen(&confLoc, ERx("w"), &confF)) != OK)
	    {
		    TDmvwprintw(mwin,row++, 0, 
		      ERx("ERROR: Could not open conf. file %s"), confFile);
		    TDrefresh(mwin);
		    return(ioerr);
	    }

    /* write output directory, test list, and input directory to file */

	    SIfprintf(confF, ERx("%s\n"), outDir);
#ifdef VMS
	    SIfprintf(confF, ERx("%s\n"), setFile);
	    SIfprintf(confF, ERx("%s\n"), batchQueue);
	    SIfprintf(confF, ERx("%s\n"), startTime);
#endif
	    SIfprintf(confF, ERx("%s\n"), testList);
	    SIfprintf(confF, ERx("%s\n"), testDir);

    /* write part & subsystem of tests to be executed */

	    for (i = 0 ; ing_parts[i] ; i++)
	    if (ing_parts[i]->substorun)
	    {
		    SIfprintf(confF, ERx("%s\n"), ing_parts[i]->partname);
		    SIfprintf(confF, ERx("%s\n"), ing_parts[i]->substorun);
	    }

    /* write type of tests to be executed */

	    SIfprintf(confF, ERx("TYPES\n"));
	    SIfprintf(confF, ERx("%s\n"),
		      (ing_types->substorun) ? ing_types->substorun : ERx("NONE"));

    /* write level of tests to be executed */

	    SIfprintf(confF, ERx("LEVELS\n"));
	    SIfprintf(confF, ERx("%s\n"),
		      (ing_levels->substorun) ? ing_levels->substorun : ERx("NONE"));
	    SIclose(confF);
    }

    /*
    ** this is it if we are not executing
    */

    if (!executing)
	return (ioerr);

    /*
     * Get the executor name from the environment, or use the default.
     */

    if (!*executorName)
    {
        char *p = NULL ;
        NMgtAt(ERx("EXECUTOR"), &p);
        if (!p || !*p)
            p = ERx("executor");
        STcopy(p, executorName);
    }

#ifdef VMS
    /*
    ** write log file
    */

    STprintf(logBuf, ERx("%s.bat"), aPref);
    SEP_LOfroms(FILENAME & PATH, logBuf, &logLoc, 0);
    if ((ioerr = SIopen(&logLoc, ERx("w"), &logF)) != OK)
    {
	if (!batchMode)
	{
		TDmvwprintw(mwin,row++, 0, 
	  	  ERx("ERROR: Could not open log file %s"), logBuf);
		TDrefresh(mwin);
	}
	else
	{
		SIprintf(
		  ERx("ERROR: Could not open log file %s\n"), logBuf);
		SIflush(stdout);
	}
	return(ioerr);
    }
    SIfprintf(logF, ERx("$ write sys$output \"Executing setup file...\"\n"));
    if (getLocation(setFile, aDev, NULL) == OK)
	STcopy(aDev, setFile);
    SIfprintf(logF, ERx("$ @ %s\n"), setFile);
    SIfprintf(logF, ERx("$ write sys$output \"Calling executor ...\"\n"));

    if (SepFlags)
	STprintf(aDev, ERx("$ %s %s -f=%c%s%c -l "), executorName, confFile,
		 '"', SepFlag_str, '"');
    else
	STprintf(aDev, ERx("$ %s %s -l "), executorName, confFile);
#endif

    /*
    **	get ready for calling the executor
    */

#if defined(UNIX) || defined(NT_GENERIC)
    args[0] = executorName;
    args[1] = confFile;
    if (SepFlags)
    {
	STprintf(testBuf, ERx("-f=%c%s%c "),'"',SepFlag_str,'"');
	STcopy(testBuf,SepFlag_str);
	args[2] = SepFlag_str;
	argnum = 3;
    }
    else
    {
	argnum = 2;
    }
#endif

    if (*testStyle != EOS)
    {
	STprintf(testBuf, ERx("-s%s "), testStyle);
#if defined(UNIX) || defined(NT_GENERIC)
	args[argnum++] = testBuf;
#endif
#ifdef VMS
	STcat(aDev, testBuf);
#endif
    }
    if (*userName != EOS)
    {
	STprintf(userBuf, ERx("-u%s "), userName);
#if defined(UNIX) || defined(NT_GENERIC)
	args[argnum++] = userBuf;
#endif
#ifdef VMS
	STcat(aDev, userBuf);
#endif
    }
    if (diffLevel)
    {
	STprintf(diffBuf, ERx("-d%d "), diffLevel);
#if defined(UNIX) || defined(NT_GENERIC)
	args[argnum++] = diffBuf;
#endif
#ifdef VMS
	STcat(aDev, diffBuf);
#endif
    }
    if (jobLevel)
    {
	STprintf(jobBuf, ERx("-t%d "), jobLevel);
#if defined(UNIX) || defined(NT_GENERIC)
	args[argnum++] = jobBuf;
#endif
#ifdef VMS
	STcat(aDev, jobBuf);
#endif
    }
    if (interactive)
    {
	TDclear(curscr);
	TErestore(TE_NORMAL);
#ifdef VMS
	STcat(aDev, ERx("-i "));
#endif
#ifdef UNIX
	args[argnum++] = ERx("-i");
    }
    else
    {
	if (getLocation(outDir, tempBuf, NULL) != OK)
		STcopy(outDir, tempBuf);
	STpolycat(4, tempBuf, ERx("/"), aPref, ERx(".out"), logBuf);
	SEP_LOfroms(PATH & FILENAME, logBuf, &logLoc, 0);
#endif /* UNIX */
#ifdef NT_GENERIC
	args[argnum++] = ERx("-i");
    }
    else
    {
	if (getLocation(outDir, tempBuf, NULL) != OK)
		STcopy(outDir, tempBuf);
	STpolycat(4, tempBuf, ERx("\\"), aPref, ERx(".out"), logBuf);
	SEP_LOfroms(PATH & FILENAME, logBuf, &logLoc, 0);
#endif /* NT_GENERIC */
    }

#if defined(UNIX) || defined(NT_GENERIC)
    args[argnum] = NULL;
#endif

#if defined(UNIX) || defined(NT_GENERIC)
    if (waitFor)
    {
        SIprintf(ERx("Test executor starting\n"));
    }
    ioerr = SEPspawn(argnum, args,
                        (interactive ? interactive : waitFor), NULL,
                        (interactive ? NULL : &logLoc), NULL, NULL, FALSE);

    if (ioerr != OK)
    {
	if (batchMode)
		SIprintf(ERx("Test executor failed to start\n"));
	else
	{
		TDmvwprintw(mwin, row++, 0, ERx("Test executor failed to start."));
		TDrefresh(mwin);
	}
	return (ioerr);
    }
    if (interactive)
    {
	TErestore(TE_FORMS);
	TDclear(curscr);
	TDtouchwin(mwin);
	TDtouchwin(pwin);
	TDrefresh(mwin);
	TDmvwprintw(mwin, row++, 0, ERx("Test executor completed."));
    }
    else
    {
	if (!batchMode)
		TDmvwprintw(mwin, row++, 0, ERx("Test executor started."));
	else
                if (waitFor)
                        SIprintf(ERx("Test executor finished\n"));
                else
                        SIprintf(ERx("Test executor started\n"));
    }

#endif /* UNIX or NT_GENERIC */

#ifdef VMS
    STcat(aDev, ERx("\n"));
    SIputrec(aDev, logF);
    SIclose(logF);

    /*
    **	name log file
    */

    if (getLocation(outDir, aDev, NULL) == OK)
	STcopy(aDev, outDir);
    LOfroms(PATH, outDir, &outLoc);
    STcat(aPref, ERx(".out"));
    LOfstfile(aPref, &outLoc);
    SEP_LOrebuild(&outLoc);
    LOtos(&outLoc, &outPtr);
    STcopy(outPtr, aPref);

    /*
    ** create command line
    */

    if (interactive)
    {
        TDmvwprintw(mwin,row++,0,ERx("Submitting BAT file ... "));
        STprintf(aDev, ERx("@ %s "), logBuf);
    }
    else
    {
	if (batchMode)
	    SIprintf(ERx("Submitting BAT file to batch...\n"));
	else
	    TDmvwprintw(mwin,row++,0,ERx("Submitting BAT file to batch ... "));
        STprintf(aDev, ERx("submit/que=%s/noprint/notify/log=%s/after=%s %s"),
                 batchQueue, aPref, startTime, logBuf);
    }

    /*
    ** finally execute the tests
    */

    if (interactive)
    {
	TDclear(curscr);
    }


#ifdef VMS
    ioerr = PCcmdline(NULL, aDev, PC_WAIT, (LOCATION *)NULL, &cl_err );
#endif
#if defined(UNIX) || defined(NT_GENERIC)
    ioerr = PCcmdline(NULL, aDev);
#endif

    if (interactive)
    {
	TDclear(curscr);
	TDtouchwin(mwin);
	TDtouchwin(pwin);
	TDrefresh(mwin);
    }

    if (ioerr != OK)
    {
	if (batchMode)
	{
	    SIprintf(ERx("ERROR: could not submit BAT file\n"));
	}
	else
	{
	    TDmvwprintw(mwin, row++, 0, ERx("ERROR: could not submit BAT file"));
	    TDrefresh(mwin);
	}
	return (ioerr);
    }
    if (interactive)
    {
	TDclear(curscr);
	TDtouchwin(mwin);
	TDtouchwin(pwin);
	TDrefresh(mwin);
	TDmvwprintw(mwin, row++, 0, ERx("Test executor completed."));
    }
    else
    {
	if (!batchMode)
	{
	    TDmvwprintw(mwin, row++, 0, ERx("BAT file was submitted"));
	}
	else
	{
	    SIprintf(ERx("BAT file was submitted\n"));
	}
    }
#endif /* VMS */

    if (!batchMode)
    {
    	TDrefresh(mwin);
    }
    return (ioerr);
}

/*
    include files
*/
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <ex.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <te.h>
#include <tm.h>
#include <termdr.h>

#define utility_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <termdrvr.h>
#include <septxt.h>
#include <fndefs.h>

/*
** History:
**	16-aug-1989 (mca)
**		Fix getstr call.
**	28-aug-1989 (eduardo)
**		Added support for '-o' flag
**	29-aug-1989 (eduardo)
**		Modified getLocation so it returns type of
**		location being parsed
**	6-Nov-1989 (eduardo)
**		Added performance improvements through SEPFILES
**	8-Nov-1989 (eduardo)
**		Added support for interrupt handling
**	29-Dec-1989 (Eduardo)
**		Added support for new 'edit canon' menu with include
**		menu choice for creating ignored canon
**	29-Dec-1989 (Eduardo)
**		Added support for 'automated' update mode
**	28-Feb-1990 (Eduardo)
**		Added TEinflush in disp_prompt
**	15-Mar-1990 (Eduardo)
**		Added writing of terminal type being used
**	20-Jun-1990 (rog)
**		Put a date in the 'History:' line instead of xx-xxx-1989.
**	26-Jun-1990 (rog)
**		Delete the comment file after we append it to the updated test.
**	10-Jul-1990 (rog)
**		diffBuff is only used inside display_lines, so move it inside
**		that function to avoid name conflicts with the diffBuff defined
**		in sepdiff/output.c and call it something else.
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	06-aug-1990 (rog)
**		append_line() was breaking up long, updated results, and
**		appending "-".
**	08-aug-1990 (rog)
**		Modify append_line() to be more general.
**	08-aug-1990 (rog)
**		Fix some bogus code.
**	08-aug-1990 (rog)
**		Fix an off-by-one error.
**	27-aug-1990 (rog)
**		Fixed SEPENDFILE support by adding calls to SEPcheckEnd().
**	29-aug-1990 (rog)
**		Fix a field-width parameter in an STprintf() call.
**	30-aug-1990 (rog)
**		Back out the above change because it is unnecessary.
**	12-aug-1991 (donj)
**		Send a proper argument to TEget so that VMS will not
**		do a timed qiow. VMS was immediately timing out, beeping
**		the user for out-of-range input and then doing TEget      
**		again, and again, and again.
**	14-jan-1992 (donj)
**	    Took out getLocataion() and started new file, GETLOC.C, which has
**	    additional functions to handle "@FILE(" stuff. Modified all quoted
**	    string constants to use the ERx("") macro. Reformatted variable
**	    declarations to one per line for clarity. Added explicit declar-
**	    ations for all submodules called. Simplified and clarified some
**	    control loops.
**      02-apr-1992 (donj)
**          Removed unneeded references. Changed all refs to '\0' to EOS. Chang
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation. Added SEP_CMstlen() to find char len
**	    rather than byte len. Added SEP_CMlastchar() to find last char in
**	    string when ( string[STlength(string)-1] ) wouldn't do it.
**	10-jun-1992 (donj)
**	    Changed MEreqmem to use a memory block tag and then replaced call
**	    to clear_pages with a call to MEtfree. Deleted clear_pages routine.
**	26-jul-1992 (donj)
**	    Switch MEreqmem() with SEP_MEalloc().
**	30-nov-1992 (donj)
**	    Include code to ask for conditional expression for cannon. Add code
**	    for Abort option in menu choice.
**	01-dec-1992 (donj)
**	    Sun4 needed a FUNC_EXTERN for SEP_CMlastchar().
**	04-feb-1993 (donj)
**	    Changed two local char arrays in open_log() to size, MAX_LOC+1.
**	    They're being used in LO functions and need to be pegged at that
**	    size.
**      05-apr-1993 (donj)
**          Fixed outputDir problem. At end of septest when outputDir was
**          just a directory, septool would try to delete the directory
**          rather than the logfile.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       7-may-1993 (donj)
**	    Got carried away and NULLed out a char ptr that I shouldn't
**	    have in append_line().
**	25-may-1993 (donj)
**	    Added func declaration I missed.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      23-sep-1993 (donj)
**          Replace TDmvwprintw() with SEPprintw() that doesn't use varargs.
**          might've been causing uninitialized memory reads.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**       4-jan-1994 (donj)
**          Moved del_file() from procmmd.c to utility.c
**	15-mar-1994 (ajc)
**	    Added hp8_bls, to overcome multiple declarations of logname.
**	13-apr-1994 (donj)
**	    Fix some return typing. Making sure that all funtions return an
**	    acceptable value in every case.
**	10-may-1994 (donj)
**	    Added function del_floc() and had del_files() call it after it
**	    contructs the location structure. Now, if SEP already has the
**	    location structure, it can go ahead and call del_floc().
**
**	    Changed SEP_cmd_line() and SEP_cmd_fix_disjointed() to take a new
**	    parameter, opt_prefix. In UNIX this is usually a "-" as in "ls -al"
**	    In VMS it is "/" as in "backup/out=output.file". Now it's no longer
**	    hard coded.
**	24-may-1994 (donj)
**	    Fix some casting complaints QA_C and/or HP's c89 compiler had.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**
**	??-sep-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**      05-Feb-96 (fanra01)
**          Added conditional for using variables when linking with DLLs.
**	12-Feb-1996 (somsa01)
**	    When the previous change was made, hTEconsole was made a GLOBALREF
**	    if NT_GENERIC AND IMPORT_DLL_DATA were not defined. This is not
**	    the case. hTEconsole is only defined for NT_GENERIC.
**	05-Jun-1998 (kinpa04)
**	    SIread() now passes back ENDFILE on a partial read of block.
**	31-Aug-1998 (kinpa04)
**	    In append_sepfile function the SEPENDFILE, following the back
**	    slash, may NOT be the first two character of the 82 character
**	    line because of the skewing of canon & result files when
**	    special characters are present. STindex is used to find the
**	    SEPENDFILE anywhere in the line.
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	20-Dec-2000 (toumi01)
**	    Correct function argument mismatch in call to SIftell.
**	21-sep-2004 (drivi01)
**		Added copyright_year(year) function that retrieves current
**		year. Added CATOSL to be appended at the beginning of every 
**		test file.
**	16-May-2005 (hanje04)
**	    GLOBALREF screenLine as it is declared in dummy.h elsewhere and 
**	    Mac OS X doesn't like multiple defns.
**	20-Aug-2007 (bonro01)
**	    SEP uses 0xFF as an internal eof-of-file indicator.  The character
**	    0xFF is not a valid UTF8 character so STindex() will fail to
**	    match if STindex() is used to test for this character.
**	    SEP needs to use a simple byte comparison loop to search for 0xFF
**	    instead of STindex().
**	21-Aug-2007 (bonro01)
**	    Fix non-standard pointer assignment to fix AIX warning.
**	28-Aug-2007 (bonro01)
**	    SEP still fails to detect EOF under some conditions.  Sometimes
**	    the '\' and 0xFF are not at the beginning of the buffer and
**	    sometimes they are split across two different reads.
**	    This change detects when '\' and 0xFF are split across two
**	    reads and also detects EOF if those characters are not located
**	    in the file.
**	04-Oct-2007 (bonro01)
**	    SEP fails to display diffs under Windows because last change
**	    conflicts with windows specific code.
**
*/

struct page_chain {
    OFFSET_TYPE		file_pos;
    struct page_chain *previous;
    struct page_chain *next;
    bool	       final_page;
};

GLOBALREF    WINDOW       *statusW ;

GLOBALREF    bool          shellMode ;
GLOBALREF    bool          updateMode ;
GLOBALREF    bool          SEPdisconnect ;
GLOBALREF    bool          batchMode ;

GLOBALREF    SEPFILE      *sepResults ;
GLOBALREF    SEPFILE      *sepDiffer ;
GLOBALREF    SEPFILE      *sepGCanons ;

GLOBALREF    char          real_editor [] ;
GLOBALREF    char          editCAns ;
GLOBALREF    char         *outputDir ;
GLOBALREF    char          terminalType [] ;
GLOBALREF    char          cont_char[] ;
GLOBALREF    char         *SEP_IF_EXPRESSION ;

GLOBALREF    LOCTYPE       outputDir_type ;
GLOBALREF    LOCATION      outLoc ;

WINDOW                    *diffW = NULL ;
WINDOW                    *diffstW = NULL ;
#if defined(NT_GENERIC)
#if defined(IMPORT_DLL_DATA)
GLOBALDLLREF    HANDLE        hTEconsole;
# else              /* IMPORT_DLL_DATA */
GLOBALREF    HANDLE        hTEconsole;
# endif             /* IMPORT_DLL_DATA */
# endif             /* NT_GENERIC */


GLOBALREF	i4                         screenLine ;
i4                         bottom_page ;
i4                         old_page ;

FILE                      *diffPtr = NULL ;

struct page_chain         *myPages = NULL ;

/*
**  Variables needed to keep track of log file
*/

static FILE               *logptr = NULL ;
# ifdef hp8_bls
/*
 * logname exists already in unistd.h
 */ 
# define logname log_name
# endif
static char                logname [MAX_LOC+1] ;
LOCATION                   logloc ;

/*
    Needed to deal with cursor position on the screen
*/
GLOBALREF    char          SEPpidstr [] ;

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF    i4            lx ;
GLOBALDLLREF    i4            ly ;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF    i4            lx ;
GLOBALREF    i4            ly ;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

GLOBALDEF    char         *conditional_str = NULL ;
GLOBALDEF    char         *conditional_old = NULL ;
GLOBALDEF    char         *conditional_prompt = NULL ;


/* prototypes */
VOID copyright_year(char *);

/*
**	disp_diff
*/

STATUS
disp_diff(STATUS acmmt,i4 canons)
{
    STATUS                 ioerr ;
    STATUS                 up_canon = FAIL ;
    char                   cmmtfile [50] ;
    char                   canon_upd = '\0' ;
    char                   junk ;
    char                   menu_ans ;
    bool                   conditional_exp ;

    /* initialize diff window */

    conditional_exp = FALSE;
#ifndef NT_GENERIC
    TEwrite(CL,STlength(CL));
    TEflush();
#endif
    diffW = TDnewwin(MAIN_ROWS,MAIN_COLS,FIRST_MAIN_ROW,FIRST_MAIN_COL);
    diffstW = TDnewwin(STATUS_ROWS,STATUS_COLS,FIRST_STAT_ROW,FIRST_MAIN_COL);
    TDrefresh(diffstW);
    TDrefresh(diffW);

    screenLine = 0;
    old_page = 0;
    bottom_page = 0;
    myPages = NULL;
    ioerr = OK;

    STprintf(cmmtfile,ERx("ct%s.stf"),SEPpidstr);
    diffPtr = sepDiffer->_fptr;

    /*
    **	display diff
    */

    page_down(0);
    if (updateMode)
	    up_canon = OK;

    for (;;)
    {
	if (updateMode)
	{
	    if (editCAns == '\0')
		disp_prompt(DIFF_MENU_M,&menu_ans,DIFF_MENU_AM);
	    else
		menu_ans = 'e';
	}
	else
	    disp_prompt(DIFF_MENU,&menu_ans,DIFF_MENU_A);

	switch (menu_ans)
	{
	   case 'A': case 'a':
		    ioerr = FAIL;
		    break;
	   case 'P': case 'p':
		    page_up();
		    break;
	   case 'N': case 'n':
		    page_down(0);
		    break;
	   case 'C': case 'c':
		    if (acmmt == FAIL && !updateMode)
			disp_prompt(ERx("no comments attached to this command"),
			    &junk,NULL);
		    else
		    {
			edit_file(cmmtfile);
			TDtouchwin(diffW);
			TDrefresh(diffW);
		    }
		    break;
	   case 'E': case 'e':
		    if (up_canon == OK)
		    {
			if (editCAns == '\0')
			{
			    disp_prompt(UPD_CANON,&canon_upd,UPD_CANON_A);
			}
			else
			{
			    canon_upd = editCAns;
			    menu_ans = 'q';
			}

			if (canon_upd == 'M' || canon_upd == 'A' ||
			    canon_upd == 'm' || canon_upd == 'a' )
			    conditional_exp = ask_for_conditional(diffstW,
								  editCAns);

			up_canon = FAIL;
		    }
		    break;
	}
	if (menu_ans == 'Q' || menu_ans == 'q' ||
	    menu_ans == 'A' || menu_ans == 'a' ||
	    menu_ans == 'D' || menu_ans == 'd')
	{
	    disp_prompt(NULLSTR,NULL,NULL);
	    SEPdisconnect = (menu_ans == 'A' || menu_ans == 'a' ||
			     menu_ans == 'D' || menu_ans == 'd');
	    break;
	}
    }

    /*
    **	update test
    */

    if (updateMode)
    {
	LOCATION	cmmtloc;
	short	count;

	/* add comments */

	LOfroms(FILENAME & PATH,cmmtfile,&cmmtloc);
	if (SEP_LOexists(&cmmtloc))
	{
	    append_line(OPEN_COMMENT,1);
	    append_file(cmmtfile);
	    append_line(CLOSE_COMMENT,1);
	    LOdelete(&cmmtloc);
	}

	/* add canons   */
	    
	switch (canon_upd)
	{
	   case 'I':
	   case 'i':
		    append_line(OPEN_CANON,1);
		    append_line(SKIP_SYMBOL,1);
		    append_line(CLOSE_CANON,1);
		    break;
	   case 'M':
	   case 'm':
		    SEPrewind(sepResults, FALSE);
		    if (conditional_exp == TRUE)
		    {
			append_line(OPEN_CANON,0);
			append_line(conditional_prompt,1);
		    }
 		    else
			append_line(OPEN_CANON,1);

		    append_sepfile(sepResults);
		    append_line(CLOSE_CANON,1);
		    append_sepfile(sepGCanons);
		    break;
	   case 'O':
	   case 'o':
		    SEPrewind(sepResults, FALSE);
		    append_line(OPEN_CANON,1);
		    append_sepfile(sepResults);
		    append_line(CLOSE_CANON,1);
		    break;
	   case '\0':
	   case 'E':
	   case 'e':
		    append_sepfile(sepGCanons);
		    break;
	   case 'A':
	   case 'a':
		    append_sepfile(sepGCanons);
		    SEPrewind(sepResults, FALSE);
		    if (conditional_exp == TRUE)
		    {
			append_line(OPEN_CANON,0);
			append_line(conditional_prompt,1);
		    }
		    else
			append_line(OPEN_CANON,1);

		    append_sepfile(sepResults);
		    append_line(CLOSE_CANON,1);
		    break;
	}
    }
    MEtfree(SEP_ME_TAG_PAGES);
    TDdelwin(diffW);
    TDdelwin(diffstW);
    return(ioerr);
}

/*
**	disp_d_line
*/

VOID
disp_d_line(char *lineptr,int row,int column)
{

    if (row)
    {
	SEPprintw(diffW, row, column, lineptr);
	screenLine = row + 1;
    }
    else
    {
	if (screenLine >= FIRST_MAIN_ROW && screenLine <= LAST_MAIN_ROW)
	{
	    SEPprintw(diffW, screenLine++, FIRST_MAIN_COL, lineptr);
	}
	else
	{
	    TDscroll(diffW);
	    SEPprintw(diffW, LAST_MAIN_ROW, FIRST_MAIN_COL, lineptr);
	}
    }
    TDrefresh(diffW);

}



/*
**  page_down
*/

STATUS
page_down(i4 update)
{
    struct page_chain     *newpage = NULL ;
    char                   junk ;

    if ( (myPages) && (myPages->final_page) )
    {
	disp_prompt(ERx("EOF reached"),&junk,NULL);
	return(OK);
    }
    if ( (myPages == NULL) || (myPages->next == NULL) )
    {
	newpage = (struct page_chain *)
		   SEP_MEalloc(SEP_ME_TAG_PAGES, sizeof(struct page_chain),
			       TRUE, (STATUS *) NULL);

	newpage->file_pos = SIftell(diffPtr );
	newpage->final_page = FALSE;
	newpage->next = NULL;
	if (myPages)
        {
	    newpage->previous = myPages;
	    myPages->next = newpage;
	}
	else
	    newpage->previous = NULL;

	myPages = newpage;
    }
    else
	myPages = myPages->next;

    TDerase(diffW);
    return(display_lines());
} /* page_down */


/*
**	page_up
*/
STATUS
page_up()
{
    STATUS                 ret_val = OK ;
    char                   junk ;

    if (myPages == NULL || myPages->previous == NULL)
	disp_prompt(ERx("TOF reached"),&junk,NULL);
    else
    {
	myPages = myPages->previous;
	SIfseek(diffPtr, myPages->file_pos, SI_P_START);
	display_lines();
    }

    return (ret_val);
}


/*
**	display_lines
*/

STATUS
display_lines()
{
    STATUS                 ret_val ;
    i4                count ;
    char                   dispBuff [TERM_LINE+1] ;
#ifdef NT_GENERIC
    char                  *ptr = NULL;
    i4                     buffCount, i, count2;
    bool                   more_found;
#endif

    for (screenLine = 0;;)
    {
	if ((ret_val = SIread(diffPtr, TEST_LINE, &count, dispBuff)) == FAIL)
	{
	    get_d_answer(ERx("ERROR: read failed on diff file."), dispBuff);
	    return(FAIL);
	}
#ifdef NT_GENERIC
	ptr = dispBuff;
	i = count = 0;
	count2 = TEST_LINE;
	more_found = TRUE;
	while (more_found == TRUE)
	{
	count2 = count2 + count;
	buffCount = 0;
	while (i != count2)
	{
	  if (dispBuff[i] == SEP_ESCAPE)
	  {
	    CMnext(ptr);
	    i++;
	    if (i == count2)
	    {
	      if (diffPtr->_ptr[0] == SEP_ESCAPE)
		buffCount++;
	    }
	    else 
	      if (dispBuff[i] == SEP_ESCAPE)
		buffCount++;
	  }
	  if (i != count2)
	  {
	    i++;
	    CMnext(ptr);
	  }
	}

	if (buffCount > 0)
	{
	  if ((ret_val = SIread(diffPtr, buffCount, &count, ptr)) == FAIL)
	  {
	    get_d_answer(ERx("ERROR: read failed on diff file."), dispBuff);
	    return(FAIL);
	  }
	}
	else
	  more_found = FALSE;
	}
#endif    
 	if (ret_val == ENDFILE || SEPcheckEnd(dispBuff))
	{
	    STprintf(dispBuff, ERx("---  EOF RETURNED  ---"));
	    myPages->final_page = TRUE;
	    disp_d_line(dispBuff, 0, 0);
	    break;
	}
	dispBuff[TEST_LINE + 1] = '\0';
#ifdef NT_GENERIC
	if (dispBuff[TEST_LINE] == -3)
	  dispBuff[TEST_LINE] = '\0';
#endif
 	SEPtranslate(dispBuff, TERM_LINE);
	disp_d_line(dispBuff, 0, 0);
	if (screenLine > LAST_MAIN_ROW)
	    break;
    }
    return(OK);
}  /* display_lines */




/*
**	edit_file
*/

STATUS
edit_file(char *fname)
{
    STATUS                 ret_val = OK ;
    char                   locbuf [MAX_LOC] ;
    LOCATION               loc ;
    WINDOW                *comments = NULL ;

    if (fname != NULL && *fname != '\0')
    {
	comments = TDnewwin(24,80,0,0);
	TDrefresh(comments);
	STcopy(fname, locbuf);
	LOfroms(PATH & FILENAME, locbuf, &loc);
	TErestore(TE_NORMAL);
	UTedit(real_editor, &loc);
	TErestore(TE_FORMS);
	TDdelwin(comments);
    }

    return (ret_val);
}  /* edit_file */
    

/*
**	put_d_message
*/

VOID
put_d_message(char *aMessage)
{

    TDerase(diffstW);
    SEPprintw(diffstW, PROMPT_ROW, PROMPT_COL, aMessage);
    TDrefresh(diffstW);

}

/*
**	get_d_answer
*/

VOID
get_d_answer(char *question,char *ansbuff)
{

    TDerase(diffstW);
    SEPprintw(diffstW, PROMPT_ROW, PROMPT_COL, question);
    getstr(diffstW, ansbuff, FALSE);
    TDerase(diffstW);
    TDrefresh(diffstW);
}



/*
**	open_log
*/

STATUS
open_log(char *testPrefix,char *testSufix,char *username,char *errbuff)
{
    SYSTIME                atime ;
    char                   uname [20] ;
    char                   logFileName [MAX_LOC+1] ;
    char                   timestr [TEST_LINE] ;
    char                   buffer [MAX_LOC+1] ;
    char                  *dot = NULL ;
    char                  *cptr = NULL ;
	char				   year[5];

    if (shellMode)
        STpolycat(3, testPrefix, ERx("."), testSufix, logFileName);
    else if (updateMode)
            STpolycat(2, testPrefix, ERx(".upd"), logFileName);
         else
            STpolycat(2, testPrefix, ERx(".log"), logFileName);

    if (outputDir)
    {
	if (outputDir_type == PATH)
	{
	    STcopy(outputDir, buffer);
	    LOfroms(PATH, buffer, &logloc);
	    LOfstfile(logFileName, &logloc);
	    LOtos(&logloc, &cptr);
	}
	else
	{
	    LOtos(&outLoc,&cptr);
	}
	STcopy(cptr, logname);
    }
    else
        STcopy(logFileName, logname);
    
    
    if (LOfroms(FILENAME & PATH, logname, &logloc) != OK)
    {
	STprintf(errbuff,ERx("ERROR: could not get location for log file"));
	return(FAIL);
    }
    if (SIopen(&logloc,ERx("w"),&logptr) != OK)
    {
	STprintf(errbuff,ERx("ERROR: could not open log file"));
	return(FAIL);
    }

    if (!updateMode)
    {
	append_line(ERx("/*"),1);

	copyright_year(&year);
	STprintf(buffer, ERx("Copyright (c) %s Ingres Corporation"), &year);
	append_line(buffer, 1);
	append_line(ERx(" "), 1);

	STprintf(buffer,ERx("\tTest Name: %s.%s"), testPrefix, testSufix);
	append_line(buffer,1);
	TMnow(&atime);
	TMstr(&atime,timestr);
	STprintf(buffer,ERx("\tTime: %s"),timestr);
	append_line(buffer,1);
	dot = uname;
	if (username == NULL || *username == '\0')
	    IDname(&dot);
	else
	    STcopy(username,uname);
	STprintf(buffer,ERx("\tUser Name: %s"),uname);
	append_line(buffer,1);
	STprintf(buffer, ERx("\tTerminal type: %s"), terminalType);
	append_line(buffer,1);
	append_line(ERx(" "),1);
    }

    return(OK);
}



/*
**	history_time
**		Format a date into the form dd-mmm-yyyy.
*/

VOID
history_time(char *timeStr)
{
    SYSTIME                atime ;
    struct TMhuman         htime ;

    TMnow(&atime);
    TMbreak(&atime, &htime);
    STprintf(timeStr, ERx("%s-%s-%s"), htime.day, htime.month, htime.year);
}

/*
**	copyright_year
**		Retrieves a year from a date.
*/

VOID
copyright_year(char *year)
{
	SYSTIME		atime ;
	struct TMhuman	htime ;

	TMnow(&atime);
	TMbreak(&atime, &htime);
	STprintf(year, ERx("%s"), htime.year);

}

/*
**	append_file
*/

STATUS
append_file(char *filename)
{
    LOCATION               fileloc ;
    FILE                  *fileptr = NULL ;
    char                   buffer [SCR_LINE] ;
    
    if (LOfroms(FILENAME & PATH,filename,&fileloc) == OK &&
	SEP_LOexists(&fileloc) && SIopen(&fileloc,ERx("r"),&fileptr) == OK)
    {
	while (SIgetrec(buffer,SCR_LINE,fileptr) == OK)
	    append_line(buffer,0);
	return(SIclose(fileptr));
    }
    else
	return(FAIL);

}


/*
**	append_sepfile
*/

STATUS
append_sepfile(SEPFILE	*sepptr)
{
    STATUS                 ioerr ;
    char                   buffer [SCR_LINE] ;
    FILE                  *fptr = sepptr->_fptr ;
    char                  *endptr, prev_char=NULL ;
    i4                     count, i ;
#ifdef NT_GENERIC
    char                  *ptr = NULL;
    i4                     buffCount, count2;
    bool                   more_found;
#endif

    SEPrewind(sepptr, FALSE);

/*
**  special case if dealing with diff output because
**  (a) lines are padded with spaces to TEST_LINE
**  (b) do not have `\n'
**  (c) first character of last line is SEPENDFILE
**      because of the skewing of the canon & result files
**      especially when special characters are found in those
**      files, the SEPENDFILE character may NOT appear in the
**      second character of a line after the slash '\'.
*/

    if (sepptr == sepDiffer)
    {
	for (;;)
	{
	    ioerr = SIread(fptr, TEST_LINE, &count, buffer);
#ifdef VMS
	    if (ioerr != OK)
		break;
#endif
#ifdef UNIX
	    if (ioerr != OK && ioerr != ENDFILE)
		break;

	    /* Detect EOF if we miss the trailing '\',0xFF */
	    if( count == 0 )
	    {
		ioerr = ENDFILE;
		break;
	    }

#endif
#ifdef NT_GENERIC
	    if (ioerr != OK && ioerr != ENDFILE)
		break;

	    ptr = buffer;
	    i = count = 0;
	    count2 = TEST_LINE;
	    more_found = TRUE;
	    while ((more_found == TRUE) && (ioerr == OK || ioerr == ENDFILE))
	    {
	      count2 = count2 + count;
	      buffCount = 0;
	      while (i != count2)
	      {
		if (buffer[i] == SEP_ESCAPE)
		{
		  CMnext(ptr);
		  i++;
		  if (i == count2)
		  {
		    if (fptr->_ptr[0] == SEP_ESCAPE)
		      buffCount++;
		  }
		  else
		    if (buffer[i] == SEP_ESCAPE)
		      buffCount++;
		}
		if (i != count2)
		{
		  i++;
		  CMnext(ptr);
		}
	      }

	      if (buffCount > 0)
	        ioerr = SIread(fptr, buffCount, &count, ptr);
	      else
		more_found = FALSE;
	    }
	    if (ioerr != OK && ioerr != ENDFILE)
		break;
#endif

	    /* Detect EOF if '\', 0xFF split across two reads */
	    if( buffer[0] == SEPENDFILE &&  prev_char == '\\' )
	    {
		ioerr = ENDFILE;
		break;
	    }
	    prev_char = buffer[81];
	    /* Detect EOF if '\', 0xFF are in consecutive bytes in the buffer */
	    for(i=0, endptr=buffer; i < TEST_LINE; ++i, ++endptr)
	    {
		if( i > 0 && *endptr == SEPENDFILE && *(endptr-1) == '\\' )
			break;
	    }
	    if( i > 0 && *endptr == SEPENDFILE && *(endptr-1) == '\\' )
	    {
		ioerr = ENDFILE;
		break;
	    }
	    buffer[TEST_LINE] = '\0';
 	    SEPtranslate(buffer, SCR_LINE);
	    STtrmwhite(buffer);
	    append_line(buffer, 1);
	}
    }
    else
    {
	while ((ioerr = SEPgetrec(buffer, sepptr)) == OK)
	    append_line(buffer, 0);
    }
    return(ioerr == ENDFILE ? OK : FAIL);

}


/*
**  append_line
*/

STATUS
append_line(char *aline,i4 newline)
{
    char                   buffer [TEST_LINE+5] ;
    char                  *cp = aline ;

    /*
	line is printed long enough to accomodate SCR output
    */

/*
	This loop breaks up long lines and puts the continuation character
	at the TEST_LINE+2 position.
*/

    while (STlength(cp) > TEST_LINE+3)
    {
	STprintf(buffer, ERx("%.*s%s\n"), TEST_LINE+2, cp, cont_char);
	SIputrec(buffer,logptr);
	cp += TEST_LINE+2;
    }

    if (newline)
    {
	STpolycat(2,cp,ERx("\n"),buffer);
	return(SIputrec(buffer,logptr));
    }
    else
	return(SIputrec(cp,logptr));

}



/*
**  close_log
*/
STATUS
close_log()
{
    STATUS                 ret_val = OK ;
    SYSTIME                atime ;
    char                   buffer [TEST_LINE] ;
    char                   timestr [TEST_LINE] ;

    append_line((char *)ERx("\n"),(i4)1);
    TMnow(&atime);
    TMstr(&atime,timestr);
    STprintf(buffer,ERx("Ending at: %s"),timestr);
    append_line(buffer,1);
    SIclose(logptr);

    return (ret_val);
}



/*
**  del_log
*/
STATUS
del_log()
{
    return (del_file(logname));
}



/*
**  disp_prompt
*/

STATUS
disp_prompt(char *buffer,char *achar,char *range)
{
    STATUS                 ret_val = OK ;
    char                   caracter ;
    char                  *legal = NULL ;
    char                   termBuffer [255] ;
#ifdef NT_GENERIC
    COORD		   coordinate;
    int			   numchars, i;
    char		   tempchar [80];
#endif

    if (achar == NULL)
    {
#ifndef NT_GENERIC
	STprintf(termBuffer,ERx("%s%s%s%s%s"),REG_CHAR,ATTR_OFF,
	    PROMPT_POS,DEL_EOL,buffer);

	TEwrite(termBuffer,STlength(termBuffer));
	TEflush();
#else
	SetConsoleActiveScreenBuffer(hTEconsole);
	coordinate.X=0;
	coordinate.Y=22;
	SetConsoleCursorPosition(hTEconsole, coordinate);
	WriteConsole(hTEconsole, buffer, strlen(buffer), &numchars, NULL);
	coordinate.X=strlen(buffer);
	coordinate.Y=22;
	SetConsoleCursorPosition(hTEconsole, coordinate);
	strcpy (tempchar, " ");
	for (i=2; i<=(80-strlen(buffer)); i++)
	  strcat (tempchar, " ");
	WriteConsole(hTEconsole, tempchar, strlen(tempchar), &numchars, NULL);
#endif
    }
    else
    {
#ifndef NT_GENERIC
	STprintf(termBuffer,ERx("%s%s%s%s%s[ %s ]%s "),REG_CHAR,ATTR_OFF,
	    PROMPT_POS,DEL_EOL,REV_VIDEO,buffer,ATTR_OFF);

	TEwrite(termBuffer,STlength(termBuffer));
	TEflush();
#else
	SetConsoleActiveScreenBuffer(hTEconsole);
	coordinate.X=0;
	coordinate.Y=22;
	SetConsoleCursorPosition(hTEconsole, coordinate);
	SetConsoleTextAttribute(hTEconsole, FOREGROUND_BLUE|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY);
	STpolycat(3, ERx("[ "), buffer, ERx(" ]"), &termBuffer);
	WriteConsole(hTEconsole, termBuffer, strlen(termBuffer), &numchars, NULL);
	coordinate.X=strlen(termBuffer);
	coordinate.Y=22;
	SetConsoleCursorPosition(hTEconsole, coordinate);
	strcpy (tempchar, " ");
	for (i=2; i<=(80-strlen(termBuffer)); i++)
	  strcat (tempchar, " ");
	SetConsoleTextAttribute(hTEconsole, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY|BACKGROUND_BLUE);
	WriteConsole(hTEconsole, tempchar, strlen(tempchar), &numchars, NULL);
#endif
	TEinflush();

	for (;;)
	{
	    EXinterrupt(EX_OFF);
#ifdef VMS
	    sys$setast(0);
#endif
	    caracter = TEget(0);
	    EXinterrupt(EX_ON);
#ifdef VMS
	    sys$setast(1);
#endif
	    if (range == NULL || (legal = STindex(range,&caracter,0)) != NULL)
		break;
	    else
	    {
		TEput(BELL);
		TEflush();
	    }		    
	}
	*achar = caracter;
    }
    return (ret_val);
}



/*
**  fix_cursor
**
**  Description:
**  routine to restore cursor to previous position after
**  something have been hard written to the terminal
*/
STATUS
fix_cursor()
{
    STATUS                 ret_val = OK ;
    char                  *cgp = NULL ;
#ifdef NT_GENERIC
    COORD		   coordinate;

    coordinate.X = lx;
    coordinate.Y = ly;
    SetConsoleCursorPosition(hTEconsole, coordinate);
#else

    cgp = TDtgoto(CM, lx, ly);
    TEwrite(cgp,STlength(cgp));
    TEflush();
#endif
    return (ret_val);
}



/*
**  show_cursor
*/
STATUS
show_cursor()
{
    STATUS                 ret_val = OK ;
    /*
    TEwrite(CURSOR_ON,STlength(CURSOR_ON));
    TEflush();
    */
    return (ret_val);
}

/*
**  hide_cursor
*/
STATUS
hide_cursor()
{
    STATUS                 ret_val = OK ;
    /*
    TEwrite(CURSOR_OFF,STlength(CURSOR_OFF));
    TEflush();
    */
    return (ret_val);
}

bool
ask_for_conditional(WINDOW *askW,char edCAns)
{
    char                   cond_exp ;

    cond_exp  = EOS;
    if (conditional_str == NULL)
	conditional_str = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE+1, TRUE,
				      (STATUS *) NULL);
    if (conditional_old == NULL)
	conditional_old = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE+1, TRUE,
				      (STATUS *) NULL);
    if (conditional_prompt == NULL)
	conditional_prompt = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE+1, TRUE,
					 (STATUS *) NULL);

    if ((*conditional_str == EOS) && (SEP_IF_EXPRESSION != NULL) &&
        (*SEP_IF_EXPRESSION != EOS))
	STcopy(SEP_IF_EXPRESSION, conditional_str);

    STcopy(conditional_str, conditional_old);
#ifdef NT_GENERIC
    STprintf(conditional_prompt,ERx(" IF (%s) >"),conditional_str);
#else
    STprintf(conditional_prompt,ERx("%s%s%s%s%s IF (%s) >%s"),REG_CHAR,
	     ATTR_OFF, PROMPT_POS, DEL_EOL, REV_VIDEO, conditional_str,
	     ATTR_OFF);
#endif

    if (edCAns == EOS)
	get_string(askW, conditional_prompt, conditional_str);

    if ((conditional_str == NULL) || (*conditional_str == EOS))
    {
	if (*conditional_old == EOS)
	{
	    if (edCAns == EOS)
		put_message(askW,
			    ERx("No conditional expression for this Canon."));
	    return (FALSE);
	}
	else
	{
	    STcopy(conditional_old, conditional_str);
	}
    }
    else if (STskipblank(conditional_str, STlength(conditional_str)) == NULL)
    {
	*conditional_str = EOS;
	*conditional_old = EOS;
	if (edCAns == EOS)
	    put_message(askW, ERx("No conditional expression for this Canon."));
	return (FALSE);
    }

    STpolycat(3, ERx(" IF ("), conditional_str, ERx(") "), conditional_prompt);
    return (TRUE);
}

/*
**	put_message
**
**	Description:
**	subroutine to display a message in the status window,
**	parameters,
**	aMessage    -	message to be displayed
**
**	Note: routine does nothing if working in batch mode
*/

VOID
put_message(WINDOW *wndow,char *aMessage)
{

    if (!batchMode)
    {
	TDerase(wndow);
	SEPprintw(wndow, PROMPT_ROW, PROMPT_COL, aMessage);
	TDrefresh(wndow);
    }

}


/*
**	get_string
**
**	Description:
**	subroutine to get commands from the screen. It can take up to
**	255 character line.
**	Parameter:
**	question    -	prompt to be displayed in the screen
**	ansbuff	    -	buffer to hold response entered by user
**
**	Note: cont_char and the end of a line should be used to indicate that
**	more is coming. In other words, cont_char is the continuation character
*/

VOID
get_string(WINDOW *wndow,char *question,char *ansbuff)
{
    char                  *prompt = NULL ;
    char                   tempBuffer [TERM_LINE] ;
    char                  *cp = ansbuff ;
    char                  *cptr = NULL ;
    i4                     clen ;
    register i4            i ;

    prompt = SEP_MEalloc(SEP_ME_TAG_NODEL, STlength(question)+5, TRUE,
			 (STATUS *) NULL);

    STprintf(prompt, ERx("%s "), question);
    put_message(wndow, prompt);    
    getstr(wndow, tempBuffer, FALSE);

    /* check if continuation character was entered */

    cptr = SEP_CMlastchar(tempBuffer,0);

    if (CMcmpcase(cptr,cont_char) == 0)
    {
	*cptr = EOS;
	STcopy(tempBuffer,cp);

	clen = SEP_CMstlen(tempBuffer,0);
	for (i=0; i<clen; i++)
	    CMnext(cp);

	TDerase(wndow);
	STprintf(tempBuffer, ERx("%s%s"), prompt, ansbuff);
	SEPprintw(wndow, PROMPT_ROW - 1, PROMPT_COL, tempBuffer);
	STprintf(prompt, ERx("%s%s"), question, cont_char);
	SEPprintw(wndow, PROMPT_ROW, PROMPT_COL, prompt);
	TDrefresh(wndow);
	getstr(wndow,tempBuffer,FALSE);

	/* check if continuation character was entered */

	cptr = SEP_CMlastchar(tempBuffer,0);

	if (CMcmpcase(cptr,cont_char) == 0)
	{
	    i4	    temp;

	    *cptr = EOS;
	    STprintf(prompt, ERx("%s %s"), question, tempBuffer);
	    SEPprintw(wndow, PROMPT_ROW, PROMPT_COL, prompt);
	    STcopy(tempBuffer, cp);

	    STprintf(prompt, ERx("%s%s"), question, cont_char);
	    SEPprintw(wndow, PROMPT_ROW + 1, PROMPT_COL, prompt);
	    /*
	    **	ugly fix to prevent the screen from scrolling when
	    **	typing to the last line of the screen. It will hopefully
	    **	be changed in the future for a more elegant solution
	    */
	    temp = LINES;
	    LINES++;
	    TDrefresh(wndow);
	    getstr(wndow, tempBuffer, FALSE);
	    disp_prompt((char *)NULLSTR, (char *)NULL, (char *)NULL);
	    LINES = temp;
	}
    }
    MEfree(prompt);
    STcopy(tempBuffer, cp);
    TDerase(wndow);
    TDrefresh(wndow);
}

STATUS
SEPprintw(WINDOW *win, i4  y, i4  x, char *bufr)
{
        if (TDmove(win, y, x) != OK)
                return(FAIL);

        return(TDaddstr(win, bufr));
}

/*
**  del_file
*/

STATUS
del_file(char *afile)
{
    STATUS                 ret_val = OK ;
    LOCATION               fileLoc ;
    char                   fileName [MAX_LOC] ;

    /* This copy is advised to prevent LOfroms from having undefined side
       side effects on the passed-in afile. -- MCA
    */
    STcopy(afile, fileName);
    LOfroms(FILENAME & PATH, fileName, &fileLoc);

    ret_val = del_floc(&fileLoc);

    return (ret_val);
}

/*
** Name: del_floc
**
** Description:
**      Delete a file location.
**
** Inputs:
**      floc    pointer to location structure with file to delete.
**
** Outputs:
**      None:
**
** Return:
**      OK
**
** History:
**      30-Jul-98 (fanra01)
**          Changed while to an if.  Otherwise we may loop here indefinitely
**          waiting for another app to close the file.
**
*/
STATUS
del_floc(LOCATION *floc)
{
    STATUS                 ret_val = OK ;

    /*
    ** This is a "while" rather than an "if" so that we can clean up multiple
    ** versions of a file on OS's that have versioning. Suggested by EGM. --MCA
    */

    if (SEP_LOexists(floc))
    {
	LOdelete(floc);
    }

    return (ret_val);
}

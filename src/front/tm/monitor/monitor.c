# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	<cm.h>
# include	<tm.h>
# include	"monitor.h"
# include	"montab.h"
# include	<qr.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
**  MONITOR
**
**	This routine maintains the logical query buffer in
**	/tmp/INGQxxxx.	It in general just does a copy from input
**	to query buffer, unless it gets a backslash escape character
**	or dollarsign escape character.
**	It recognizes the following escapes:
**
**	\a -- force append mode (no autoclear)
**	\b -- branch (within an include file only)
**	\bell -- turn on beep in tm prompt
**	\nobell -- undo above (default)
**	\c -- reserved for screen clear in geoquel
**	\d -- change working directory
**	\e -- enter editor
**	\f -- format output to 80 or 132 column
**	\g -- "GO": submit query to INGRES
**	\i -- include (switch input to external file)
**	\k -- mark (for \b)
**	\l -- list: print query buffer after macro evaluation
**	\p -- print query buffer (before macro evaluation)
**	\q -- quit ingres
**	\r -- force reset (clear) of query buffer
**	\script -- log the query into the file `script.ing'
**	\s -- call shell
**	\t -- print current time
**	\v -- evaluate macros, but throw away result (for side effects)
**	\w -- write query buffer to external file
**	\macro	-- restore macro processing
**	\nomacro -- turn off macro processing
**	NO -- \echo -- turn on statement echo 
**	NO -- \noecho -- turn off statement echo
**	\quel -- query language is QUEL
**	\sql -- query language is SQL
**	\suppress -- Suppress output of select statements (for qe90 retrieval)
**	\nosuppress -- Disable suppression of selects
**	NO -- \semicolon -- require semicolon terminator
**	NO -- \nosemicolon - don't require semicolon terminator
**	\\ -- produce a single backslash in query buffer
**	$t -- change setting of trace flags
**	$r -- reset system
**      \silent,\sil -- enter 'silent' mode
**      \nosilent,\nosil -- exit 'silent' mode
**      \notitles -- do not output column titles
**      \titles -- output column titles
**      \vdelimiter,\vdelim -- specify vertical delimiter character
**            similar to command line -v. If none specified then
**            revert to default or command line specified value
**
**	Uses trace flag 2
**
**	History:
**	82.11.21 (bml)	-- bug fix #6382 -- in which \quit in an
**		included file was not processed.
**		return special status on \quit so that include()
**		knows what to do; a hack in response to a field
**		emergency.  Should be cleaner...
**	09/29/83 (lichtman) -- added fix of bug #179 that was missed
**		in integration: ignore \ commands inside of comments.
**		Originally fixed by Fred.
**	23-apr-84 (fhc) -- add new feature to put the beep back into
**		your prompt
**	11/1/84 (ded) -- allow user to insert newlines for wide tuples
**	11/06/84 (lichtman) -- added parameter to include() to improve
**		error reporting
**	12/10/84 (lichtman) -- added \macro and \nomacro to turn macro
**		processing on and off
**	85/09/20	 02:02:16  roger
**		Moved Controlwords[] and Syscntrlwords[] to montab.roc
**		(loads in (shared) text segment on Unix).
**	85/10/04	 16:46:52  roger
**		ibs integration.  Note: CMS additions to
**		Controlwords[] are in montab.c.
**	86/01/17  14:30:27  rpm
**		Integrated fix for bug #6382.
**	86/01/20  13:26:37  root
**		Integration of 3.0/23-25 x line changes by RPM .
**	86/07/23  16:50:06  peterk
**		6.0 baseline from dragon:/40/back/rplus
**	86/10/14  14:58:41  peterk
**		working version 2.  eliminated BE include files.
**		added \echo, \noecho TM commands to turn on (off) statement
**		echoing by QR.
**	86/12/29  16:23:05  peterk
**		change over to 6.0 CL headers
**	87/03/18  16:46:52  peterk
**		added implementation of \quel (default), \sql, \semicolon
**		(default), \nosemicolon TM commands.
**	87/03/31  15:45:27  peterk
**		change semantics of \quel and \sql to imply appropriate
**		default settings of \semicolon:  \quel -> \nosemicolon;
**		\sql -> \semicolon.
**	87/04/10  13:01:26  peterk
**		added #include {compat,dbms,fe}.h
**	06/27/88 (dkh) - Changed monitor() to IItmmonitor().
**	11-aug-88 (bruceb)
**		Added ricks' kanji changes.
**	05-oct-88 (kathryn)	Bug with no number - '\' no longer hangs
**		the terminal monitor in SQL.
**	02-nov-88 (bruceb)
**		The use of \macro when running SQL is disallowed.
**	28-nov-88 (bruceb)
**		Accept \r as valid character along with \n and \t (in qputch)
**		for use with DG system.
**	20-apr-89 (kathryn) -- Added "\continue", "\nocontinue" commands.
**		"\continue"   - continue go_block when user error encountered.
**		"\noconintue" - terminate go_block when user error encountered.
**		C_CONTINUE	=	"\co"	=	"\continue"
**		C_NOCONTINUE	=	"\nco"	=	"\nocontinue"
**	25-jan-90 (teresal) - Fix for LOfroms: copy script_file name into a
**		location buffer before calling LOfroms.
**	19-mar-90 (teresal)
**		Added logic so a query buffer truncated because memory is full
**		won't be sent to the server. Bug 9037. 
**	04-apr-92 (kathryn) - Bug 43521
**		Don't attempt to parse second byte of double-byte character.
**		Second byte may be "\" or '"' which should not be interpretted
**		as escape character or quote.
**	12/03/92 (dkh) - Renamed routine getname() so that it does not conflict
**			 with builtin C function of the same name on alpha.
**	3-aug-92 (rogerl) - if processing a quoted string (" or '), then
**		allow quoting of " or ' (respectively) in the input by doubling,
**		(delimited ids)
**	02/03/93 (rogerl) - Remove QBclear() at switch of \CONTINUE and
**		\NOCONTINUE.  The state used for the current 'go' block will
**		be that which is 'last' in the block.  (the entire 'go' block
**		is scanned with switches taking effect at encounter prior to
**		executing any statements) (bug#47609)
**	30-03-93 (rogerl) - SQL quoting must select higher precedence quote
**		character from {",'} and preserve that until the current quote
**		string has been terminated.  Bug 50710.
**	9-6-93 (rogerl) - Treat SQL style comments as C style; allows use of
**		unmatched quotes in the comment text.
**      15-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchin's changes (from 6.4) for axp_osf
**              10/15/93 (kchin)
**              Cast 2nd argument to PTR when calling putprintf(), this
**              is due to the change of datatype of variable parameters
**              in putprintf() routine itself.  The change is made in
**              qputch().
**      25-sep-96 (rodjo04)
**              BUG 77034: Fix given so that terminal monitor can use the
**              enviornment variable II_TM_ON_ERROR to either terminate or
**              continue on error. Added a boolean parameter to IItmmonitor().
**              This new parameter is necessary so that we can tell the
**              difference between stdin, redirected input, and the use
**              of an include file.
**	03-oct-1996 (chech02) bug#78827
**		moved local variables to global heap area for windows 3.1
**		port to prevent 'stack overflow'.
**	15-nov-1996 (bobmart)
**		Touch up fix for 77034.
**	17-Mar-1999 (wanfr01)
**		Add C_SUPPRESS/C_NOSUPPRESS to suppress query output for 
**		large queries to get qe90 information.
**	25-oct-01 (inkdo01)
**		Re-enable macros in SQL.
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Added call to SIclearhistory() in IItmmonitor. 
**	29-mar-2004 (kodse01)
**		SIclearhistory() is called if HistoryRecall is TRUE.
**	01-Feb-2005 (hweho01)
**		Checked the return code after calling include() routine,   
**              so \nocontinue will be honored. Star #13864975. 
**	02-Feb-2005 (hweho01)
**		Removed the semi-colon that was left out from the  
**              previous change.
**	06-May-2005 (toumi01)
**		Return non-zero OS RC if /nocontinue and we get an error.
**	24-Aug-2009 (maspa05) Trac 397, SIR 122324
**		Added commands associated with 'silent' mode - \silent,
**              \nosilent,\titles,\notitles,\vdelimiter, \trim, \notrim
**	29-Sep-2009 (frima01) 122490
**		Add return type for IItmmonitor() to avoid gcc 4.3 warnings.
**	05-May-2010 (hanje04)
**		SIR 123622
**	   	HistoryRecall now enabled for all UNIX.
**      03-Jun-2010 (maspa05) bug 123860
**              \silent resets delimiter to space even if \vdelim was used
**              check whether \vdelim is in force and do not reset if so
**              Also when resetting to default make it space (as documented)
**              rather than \t
*/

FUNC_EXTERN i4		getch();
FUNC_EXTERN STATUS	go();

GLOBALDEF bool	Tm_echo = FALSE;
GLOBALDEF bool	Tm_suppress = FALSE;
/*
** currently QUEL is the default language, thus Tm_semicolon defaults
** to FALSE.  If and when SQL becomes the default language, Tm_semicolon
** should be initialized to TRUE.
*/
GLOBALDEF bool	Tm_semicolon = FALSE;
extern ADF_CB	*Tm_adfscb;
GLOBALREF bool	Qb_memory_full;
GLOBALREF   bool    HistoryRecall;
GLOBALREF      bool TrulySilent ;
GLOBALREF      bool Showtitles ;
GLOBALREF      bool Padding ;
GLOBALREF      bool Uservflag ;
GLOBALREF      char Uservchar[VDELIM_SIZE+1] ;
GLOBALREF       bool    IIMOupcUsePrintChars;

i4
IItmmonitor(include_file)
bool include_file;
{
	char		chr,
			chr1,
			quotechar;
	register i4	controlno;
	char		buffer[100];
	char		*script_file;
#ifdef WIN16 
	static char	*errbuf = NULL;
#else
	char		errbuf[ER_MAX_LEN];
#endif /* WIN16 */
	LOCATION	loc;
	char		locbuf[MAX_LOC+1];
	SYSTIME		time;
	i4		status;
	extern i4	Moninquote;	/* TRUE if inside a f8-quoted string */
	extern i4	Monincomment;	/* TRUE if inside a comment */
	extern i4	Monincomment1;	/* TRUE if inside a -- comment */
	char		*bufalloc();
	char		*getfilenm();

	char		*vdelim_arg;    /* argument to \vdelim */
        static char     vdelim_str[VDELIM_SIZE+1]="|"; /* char to set vertical separator to */
        static char     vdelim_desc[VDELIM_SIZE+1]="|"; /* descriptive version of vdelim, will either
					       be the char or SPACE, TAB or NONE */
	bool		vdelim_warn=TRUE; /* need to warn about excess chars for \vdelim */
	static bool	vdelim_in_force=FALSE; /* a \vdelim is in force */


#ifdef WIN16
	if (errbuf == NULL)
	{	
	  if ((errbuf = bufalloc(ER_MAX_LEN)) == NULL)
		return(FAIL);
	}
#endif

	while (chr = (char)getch())
	{
#ifdef DOUBLEBYTE
		if (CMdbl1st(&chr))
		{
		     qputch(chr);
		     chr = (char)getch();
		     qputch(chr);
		     continue;
		}
#endif
		/* turn on Monincomment if we are entering a comment */
		if ( (chr == SLASH || chr == SQL_COMM_BEG )
		    && !Monincomment && !Moninquote && !Monincomment1 )
		{
			chr1 = (char)getch();
			if (chr1 == STAR)
			{
				qputch(chr);
				qputch(chr1);
				Monincomment = TRUE;
				continue;
			}
			else if (chr1 == SQL_COMM_BEG )
			{
				qputch(chr);
				qputch(chr1);
				Monincomment1 = TRUE;
				continue;
			}
			Peekch = chr1;
		}
		/* turn off Monincomment if we are exiting a comment */
		if (chr == STAR && Monincomment && !Moninquote)
		{
			chr1 = (char)getch();
			if (chr1 == SLASH)
			{
				qputch(chr);
				qputch(chr1);
				Monincomment = FALSE;
				continue;
			}
			Peekch = chr1;
		}
		else if (chr == SQL_COMM_END && Monincomment1 && !Moninquote)
		{
			qputch(chr);
			Monincomment1 = FALSE;
			continue;
		}
		/*
		** Toggle Moninquote if we are entering or leaving a 
		** quoted string; ' or " may be overridding quote in SQL,
		** determined by first encountered.
		** QUEL quote delimiter == " 
		*/

		if ( Tm_adfscb->adf_qlang == DB_QUEL )
			quotechar = ERx('"');

		if ( ( ( Tm_adfscb->adf_qlang == DB_SQL )
		    && ( ( chr == ERx('"') || chr == ERx('\'') )
			&& ( ! Monincomment )
			&& ( ! Monincomment1 ) ) )
		  || ( ( Tm_adfscb->adf_qlang == DB_QUEL )
		    && ( chr == quotechar && ! Monincomment )
		    && ( chr == quotechar && ! Monincomment1 ) ) )
		{
			if ( ! Moninquote )	/* starting a quoted string */
			{
			    quotechar = chr;
			    Moninquote = !Moninquote;
			}
			else			/* might be the ending quote */
			{
			    if (quotechar == chr)
				Moninquote = !Moninquote;

			    /* (quotechar != char) -> no toggle, wrong quote */
			}
		}

		if (chr == '\\' && !Monincomment && !Monincomment1 &&
		    (!Moninquote || Tm_adfscb->adf_qlang == DB_QUEL))
		{
			if (Moninquote)
				qputch(chr);
			chr1 = (char)getch();
			if (chr1 == '\\' || Moninquote)
			{
				qputch(chr1);
				continue;
			}
			else
				Peekch = chr1;

			/* process control sequence */
			if ((controlno = getescape(1)) == 0)
				continue;

			switch (controlno)
			{

			  case C_EDIT:
				edit();
				continue;

			  case C_PRINT:
				print();
				continue;

			  case C_LIST:
				eval((i4)1);
				continue;

			  case C_EVAL:
				eval((i4)0);
				Autoclear = TRUE;
				continue;

			  case C_FORMAT:
				Term_format = TRUE;
				continue;

			  case C_NOFORMAT:
				Term_format = FALSE;
				continue;

			  case C_INCLUDE:
				if(include((LOCATION *)NULL, ERx("")) == -1 )
                                  return (-1); 
				cgprompt();
				continue;

			  case C_WRITE:
				writeout();
				cgprompt();
				continue;

			  case C_CHDIR:
				newdirec();
				cgprompt();
				continue;

			  case C_RESET:
				QBclear(1);
				continue;

			  case C_SUPPRESS:
				Tm_suppress = TRUE;
				continue;

			  case C_NOSUPPRESS:
				Tm_suppress = FALSE;
				continue;

			  case C_GO:
				if (Qb_memory_full)
				    QBclear(1);
				else
                                    if (go()!=OK)
                                        if (include_file)
                                            return(-1);
                                continue;

			  case C_QUIT:
				clrline(1);
				/*
				** (bml) bug fix #6382 -- return special status
				** for \quit so that include() can quit
				*/
				if (quit(OK))
				    return(-1);
				continue;

			  case C_SCRIPT:
				script_file = getfilenm();
				Peekch = '\n';	/* Peekch is trashed in
						   getfilenm() */
				if (Script != NULL)
				{
					if (Nodayfile >= 0)
						putprintf(ERget(F_MO0020_Transcript_log_is_off));

					SIclose(Script);
					Script = NULL;
				}
				else
				{

					if (*script_file == NULL)
						script_file = ERget(F_MO0021_script_ing);

					if (Nodayfile >= 0)
/*	The previous string is for the name of the script file. The default	*/
						putprintf(ERget(F_MO0022_Transcript_log_into_f), script_file);

					STcopy(script_file, locbuf);	
					LOfroms(PATH & FILENAME, locbuf, &loc);

					if (status = SIopen(&loc, ERx("w"), &Script))
					{
						ERreport(status, errbuf);
						putprintf(ERget(F_MO0023_Cant_open_script_f),
							script_file, errbuf);

						Script = NULL;
					}
				}

				prompt((char *)NULL);
				continue;

			  case C_SHELL:
				shell();
				continue;

			  case C_TIME:
				TMnow(&time);
				TMstr(&time,buffer);
				putprintf(ERx("%s\n"), buffer);
				clrline(0);
				continue;

			  case C_APPEND:
				Autoclear = 0;
				clrline(0);
				continue;

			  case C_MARK:
				getfilenm();
				prompt((char *)NULL);
				continue;

			  case C_BRANCH:
				branch();
				prompt((char *)NULL);
				continue;

			  case C_BEEP:
				put_beep();
				continue;

			  case C_NOBEEP:
				take_beep();
				continue;

			  case C_MACRO:
				Savedomacros = Do_macros = TRUE;
				clrline(0);
				continue;

			  case C_NOMACRO:
				Savedomacros = Do_macros = FALSE;
				clrline(0);
				continue;

			  case C_QUEL:
				Tm_adfscb->adf_qlang = DB_QUEL;
				Tm_semicolon = FALSE;
				QBclear(1);
				continue;

			  case C_SQL:
				Tm_adfscb->adf_qlang = DB_SQL;
				Tm_semicolon = TRUE;
				QBclear(1);
				continue;

			  case C_CONTINUE:
				Err_terminate = FALSE;
				continue;
				
			  case C_NOCONTINUE:
				Err_terminate = TRUE;
				continue;

			  /* column padding */
			  case C_PADDING:
				if (!TrulySilent)
					putprintf(ERget(F_MO0050_Padding_on));
				Padding = TRUE;
				continue;

			  /* column padding */
			  case C_NOPADDING:
				if (!TrulySilent)
					putprintf(ERget(F_MO0051_Padding_off));
				Padding = FALSE;
				continue;

			  /* show column titles */
			  case C_TITLES:
				if (!TrulySilent)
					putprintf(ERget(F_MO0044_Titles_on));
				Showtitles = TRUE;
				continue;

			  /* don't show column titles */
			  case C_NOTITLES:
				if (!TrulySilent)
					putprintf(ERget(F_MO0045_Titles_off));
				Showtitles = FALSE;
				continue;

			  /* set vertical separator character delimiter */
			  case C_VDELIM:
				/* read what's to the right of the command,
				** using the getfilenm function used by \script
                                */
				vdelim_arg=getfilenm();

				/* if no argument was given then reset to what 
				** we had before 
				** if -v used on the cmd line (Uservflag set)
				** then reset to the -v char i.e. Uservchar
				**
				** otherwise in silent mode the default is a 
				** space and in non-silent mode it's |
				** note that in non-silent mode we reset to the
				** drawing characters 
				** (IIMOupcUsePrintChars=FALSE)
				** but this is over-riden if you're not writing 
				** to a terminal so need to set vdelim to | 
				*/
				if ( vdelim_arg[0] == EOS )
				{
					if (Uservflag)
						STlcopy(Uservchar,vdelim_str,VDELIM_SIZE);
					else 
						if (TrulySilent)
							STcopy(" ",vdelim_str);
						else
						{
							STcopy("|",vdelim_str);
                                			ITsetvl(vdelim_str);
	                        			IIMOupcUsePrintChars = FALSE;
						}
					if (!TrulySilent)
						putprintf(ERget(F_MO0049_vdelim_reset));
					vdelim_in_force=FALSE;
				}
				else
				{
				/* if there's an argument to \vdelim then
				** check for SPACE, TAB or NONE 
				** otherwise use the first non-blank char
				*/
					STlcopy(vdelim_arg,vdelim_str,VDELIM_SIZE);
					STlcopy(vdelim_arg,vdelim_desc,VDELIM_SIZE);

					if (STbcompare(vdelim_arg,0,ERx("TAB"),0,TRUE) == 0)
					{
						STcopy("\t",vdelim_str);
						STcopy("{TAB}",vdelim_desc);
					}

					if (STbcompare(vdelim_arg,0,ERx("NONE"),0,TRUE) == 0)
					{
						vdelim_str[0]=EOS;
						STcopy("{none}",vdelim_desc);
					}

					if (STbcompare(vdelim_arg,0,ERx("SPACE"),0,TRUE) == 0)
					{
						STcopy(" ",vdelim_str);
						STcopy("{SPACE}",vdelim_desc);
					}

					if (!TrulySilent)
						putprintf(ERget(F_MO0048_vdelim_set),vdelim_desc);


	                        	IIMOupcUsePrintChars = TRUE;

					vdelim_in_force=TRUE;
				}
                                ITsetvl(vdelim_str);
				continue;

			  case C_SILENT:
				/* switch to silent mode */
				if (!TrulySilent)
					putprintf(ERget(F_MO0046_Silent_on));
	                        IIMOupcUsePrintChars = TRUE;
				TrulySilent = TRUE;
				Nodayfile = -1;
				Userdflag = -1;

				/* default delimiter for silent mode is space 
				 * unless -v or \vdelim has been used */
				if (!Uservflag && !vdelim_in_force)
				{
					STlcopy(" ",vdelim_str,VDELIM_SIZE);
                                        ITsetvl(vdelim_str);
				}
				continue;

			  case C_NOSILENT:
				/* revert to non-silent mode and set the
				** vdelim character appropriately
				*/
				putprintf(ERget(F_MO0047_Silent_off));

				/* only change vdelim if a \vdelim is not in
				 * force */
				if (!vdelim_in_force)
				{
				        if (Uservflag)
				        {
	                                     IIMOupcUsePrintChars = TRUE;
					     STlcopy(Uservchar,vdelim_str,
							     VDELIM_SIZE);
                                	     ITsetvl(vdelim_str);
				        }
					else
				        {
                                	     vdelim_str[0] = '|';
                                	     ITsetvl(vdelim_str);
	                        	     IIMOupcUsePrintChars = FALSE;
				        }
				}
				TrulySilent = FALSE;
				Nodayfile = 0;
				Userdflag = 0;
				continue;

			  default:
				/* monitor: bad code %d */
				ipanic(E_MO0053_1500900, controlno);
			}
		}
		else if (chr == '$' && !Monincomment && !Monincomment1
			&& !Moninquote)
		{
			/* process system control sequence */
			if ((controlno = getsyscntrl()) == 0)
				continue;

			switch (controlno)
			{

			  case SC_TRACE:
				trace();
				continue;

			  case SC_RESET:
				reset();
				continue;

			  default:
				/* monitor: bad $ code %d */
				ipanic(E_MO0054_1500901, controlno);
			}
		}


		qputch(chr);
	}
	if (Input == stdin)
	{
		if (Nodayfile >= 0)
			putprintf(ERx("\n"));
	}
	else
	{
		SIclose(Input);
	}
#ifdef UNIX
	if (HistoryRecall)
		SIclearhistory(); /* Clear the history and free the history list */
#endif

	return(OK);		/* normal status return */
}

getescape(copy)
int	copy;
{
	register struct cntrlwd *cw;
	register char		*word;
	FUNC_EXTERN	char			*IIMOgetname();

	word = IIMOgetname();
	for (cw = Controlwords; cw->name; cw++)
	{
		if (STbcompare(cw->name, 0, word, 0, TRUE) == 0)
			return (cw->code);
	}

	/* not found -- pass symbol through and return failure */
	if (copy == 0)
		return (0);
	qputch('\\');
	while (*word != 0)
	{
		qputch(*word++);
	}
	return (0);
}


getsyscntrl()
{
	register struct cntrlwd *scw;
	register char		*word;
	FUNC_EXTERN	char			*IIMOgetname();

	word = IIMOgetname();
	for (scw = Syscntrlwds; scw->name; scw++)
	{
		if (!STcompare(scw->name, word))
			return (scw->code);
	}

	/* not found -- pass symbol through and return failure */

	qputch('$');

	while (*word != 0)
	{
		qputch(*word++);
	}

	return (0);
}

char *
IIMOgetname()
{
	register char	*p;
	static char	buf[42];
	register i4	len;
	char		c;

	p = buf;

	for (len = 0; len < 40; len++)
	{
		*p = (char)getch();

		if (CMalpha(p))
		{
			p++;
		}
		else if (CMdbl1st(p))
		{
			p++;
			len++;
			*p++ = (char)getch();
		}
		else
		{
			Peekch = *p;
			break;
		}
	}

	*p = 0;
	return (buf);
}



qputch(ch)
char	ch;
{
	char	c;

# ifdef	CMS
	char		vmprompt;
# endif
	c = ch;
# ifdef	CMS
	vmprompt = (Newline && (c == '\n'));
# endif

	P_rompt = Newline = (c == '\n');

	if (CMcntrl(&c) && c != '\n' && c != '\t' && c != '\r')
	{
		putprintf(ERget(F_MO0024_Nonprint_char_to_blan), (PTR)c);
		c = ' ';
	}

# ifdef	CMS
	if (vmprompt)  /* like other VM software, identify yourself */
	{
		prompt(ERx("sql"));
	}
	else
# endif
	{
		prompt((char *)NULL);
	}

	if (Autoclear)
		QBclear(0);

	if (q_putc(Qryiop, c))
	{
		return;
	}

	Notnull++;
}


reset()
{
	putprintf(ERget(F_MO0025_reset__does_nothing));
}

trace()
{
	putprintf(ERget(F_MO0026_trace__does_nothing));
}


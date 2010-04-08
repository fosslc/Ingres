/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<st.h>
#include	<te.h>
#ifndef NT_GENERIC
#include	<unistd.h>
#endif
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:	feprompt.c -	Front-End Prompt for User Response Module.
**
** Description:
**	Contains the routine used to prompt for user responses.  Defines:
**
**	IIUGprompt()	prompt for user response.
**	FEprompt()	prompt for user response.
**
** History:
**	Revision 6.4  89/12  wong
**	Added 'IIUGprompt()'.
**
**	Revision 6.0  87/12/03  wong
**	Force prompt only if from terminal.
**
**	Revision 4.0  85/03/13  peter
**	Initial revision.
**
**	09-sep-1987 (rdesmond)
**		removed appendage of "?" to prompt string in FEprompt.
**	31-mar-1988 (marian)
**		removed return OK since sun was giving a warning that
**		the statement is never reached.
**	5/22/90 (elein) 21598 (for 6.4)
**		Do not print prompt string if input is NOT from a terminal
**      18-may-1994 (harpa06)
**              Cross-Integration change by prida01 (bug 49935) -
**		If stdout is redirected to a file we need to turn echo off on
**		stdin to preserver password security.
**      25-sep-96 (mcgem01)
**              Global data moved to ugdata.c
**	16-sep-1997 (donc)
**		Add special OpenROAD prompting for NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

# ifndef FT3270
#define	ENDCHAR '\t'
# else
#define ENDCHAR '\n'
# endif

VOID  IIUGprompt();

GLOBALREF VOID	(*IIugPfa_prompt_func)();

/*{
** Name:	IIUGprompt() -	Prompt for a User Response.
**
** Description:
**	Prompts the user for input from the terminal, but it allows prompt
**	formatting for international support.  See the	'IIUGfmt()'
**	specification for a description of parameter format.
**
** Input:
**	prompt	{char *}  Prompt message to be output.
**	forceit {nat}  Number of times user is to be prompted.
**			1 = once,
**			0 = (practically) infinite.
**	noecho	{bool}  Whether the response is to be echoed.
**	lsize	{nat}  The size of the line buffer for the response.
**	parcount {nat}	Number of parameters to prompt.
**	a1-a10	{PTR}	Arguments (see IIUGfmt)
**
** Output:
**	linebuf	{char *}  Buffer to contain user response.  This must
**			  be of size FE_PROMPTSIZE or less.
**
** History:
**	12/89 (jhw) -- Copied from 'IIUFask()'.
**	03/90 (jhw) -- Added no echo support.
**      18-may-1994 (harpa06)
**			Cross-Integration change by prida01 (bug 49935) - 
**			Need to support echo on stdin incase stdout is
**			redirected to file.
**	18-aug-95 (tutto01)
**		Added code for dialog box support for Windows NT.
**	31-aug-95 (tutto01)
**		Added dialog box support for Report on Windows NT.
*/
static STATUS	teget();
static VOID	teclose();

/*VARARGS6*/
VOID
IIUGprompt ( prompt, forceit, noecho, linebuf, lsize,
			parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
char	*prompt;
i4	forceit;
bool	noecho;
char	linebuf[];
i4	lsize;
i4	parcount;
PTR	a1;
PTR	a2;
PTR	a3;
PTR	a4;
PTR	a5;
PTR	a6;
PTR	a7;
PTR	a8;
PTR	a9;
PTR	a10;
{
	char	*pptr;
	char	prompt_buf[ER_MAX_LEN+1];

	if ( prompt == NULL )
	{ /* no prompt string */
		prompt = ERx("");
	}

	if ( parcount <= 0 || *prompt == EOS )
	{ /* use input prompt string */
		pptr = prompt;
	}
	else
	{ /* format prompt string */
		pptr = IIUGfmt( prompt_buf, sizeof(prompt_buf) - 1,
				prompt, parcount, a1, a2, a3, a4, a5,
						a6, a7, a8, a9, a10
		);
	}

	if ( noecho )
	{
		TEENV_INFO	junk;

		if ( (noecho = ( SIterminal( stdin ) && TEopen(&junk) == OK )) )
		{
			TErestore( TE_FORMS );
                        /* Turn echo off on stdin to avoid redirection stdout*/
                        /* funnies 49935 */
# ifdef UNIX
                        TErestore( TE_ECHOOFF);
# endif
			FEset_exit(teclose);	/* ... on exception */
		}
	}

	for (;;)	/* repeat until you get one */
	{
		if( SIterminal( stdin ) )
		{
#ifdef NT_GENERIC
		    if (GetOpenRoadStyle())
		    {
			SIfprintf((FILE *)NULL, ERx("%s%c"), pptr, ENDCHAR );
		        SIflush((FILE *)NULL);
		    }
		    else
		    {
#endif
			SIfprintf( stderr, ERx("%s%c"), pptr, ENDCHAR );
		        SIflush(stderr);
#ifdef NT_GENERIC
		    }
#endif
		}

		MEfill(lsize, EOS, linebuf);
# ifdef NT_GENERIC
		/* Signal SI to pop up a dialog for user input. */
		if (!STcompare(pptr, "Database"))  STcopy("DBDLG", linebuf);
		if (!STcompare(pptr, "Database?")) STcopy("DBDLG", linebuf);
		if (!STcompare(pptr, "Report or Table")) STcopy("REP_TABLE", linebuf);
# endif /* NT_GENERIC */
		if ( ( noecho ? teget(linebuf, lsize)
				: SIgetrec(linebuf, lsize, stdin) ) != OK )
		{
			if ( noecho )
			{
				FEclr_exit(teclose);
				teclose();
                                /* make sure we echo on stdin in as well */
                                /* in case of errors 49935 */
# ifdef UNIX
                                TErestore(TE_ECHOON);
# endif

			}
			return;
		}

		/* strip off trailing blanks */

		if ( STtrmwhite(linebuf) > 0 || --forceit == 0 )
		{
			if ( noecho )
			{
				/* make sure we echo on stdin in as well */
				/* in case of errors 49935 */
# ifdef UNIX
                                TErestore(TE_ECHOON);
# endif
				FEclr_exit(teclose);
				teclose();
			}

			return;	/* response entered or user not to be forced */
		}
		
		/* break out (kill the program) if input not from terminal */
		if ( !SIterminal(stdin) )
		{
			IIUGerr( E_UG0010_NoPrompt, UG_ERR_FATAL,
					1, prompt != NULL ? prompt : ERx("?")
			);
			/*NOT REACHED */
		}
	}
	/*NOT REACHED */
}

/*
** History:
**	03/90 (jhw) -- Copied from "gcn!netu.c"
*/
static STATUS
teget ( buf, len )
char		*buf;
register i4	len;
{
	register char	*bp = buf;
	register i4	c;

	SIputc('\r', stderr);
	while ( --len > 0 )
	{
		switch ( c = TEget(0) ) 	/* No timeout */
		{
		  case TE_TIMEDOUT:	/* just in case of funny TE CL */
		  case '\n':
		  case '\r':
			SIputc('\n', stderr);
			*buf++ = EOS;
			/* fall through ... */
		  case EOF:
			len = 0;
			break;
		  default:
			*buf++ = c;
			break;
		} /* end switch */
	} /* end while */

	return buf == bp ? ENDFILE : OK;
}

/*
**
** Description:
**	Closes the no echo mode for the terminal.  This will be called
**	as part of execption clean-up or once a parameter has been entered.
**
** History:
**	03/90 (jhw) -- Written.
*/
static VOID
teclose ()
{
	TErestore( TE_NORMAL );
	TEclose();
}

/*{
** Name:	FEprompt() -	Prompt for User Response.
**
** Description:
**
** Input:
**	prompt	{char *}  Prompt message to be output.
**	forceit {bool}  Indicates if user should be reprompted until a value is
**			entered (when TRUE.)
**	bufsize {nat}  Size of response buffer.
**
** Output:
**	linebuf	{char *}  Buffer to contain user response.
**
** Returns:
**	{STATUS}	(FAIL  or  OK)
**
** History:
**	3/13/85 (prs)  written.
**	01/86 (jhw) -- Use 'stderr' for prompt (usually goes to control
**			terminal, unlike 'stdout').
**	01/86 (bab) -- despite forceit being TRUE, only prompt an
**			arbitrary number of times (currently set at 5) before
**			killing the program.  This is so that batch jobs on VMS
**			will not run forever if parameters are missing from the
**			script.  fix for bug 6568.  
**	08-sept-86 (marian)	bug # 9543
**		Check to see if 'prompt' is NULL before checking if empty (EOS).
**	03/87 (jhw) -- Changed to force prompt only if input from terminal.
**	09-sep-1987 (rdesmond)
**		removed appendage of "?" to prompt string.
**	12/89 (jhw) -- Modified to call 'IIUGprompt()'.
*/

STATUS
FEprompt ( prompt, forceit, bufsize, linebuf )
char	*prompt;
bool	forceit;
i4	bufsize;
char	*linebuf;
{
	IIUGprompt(prompt, forceit ? 0 : 1, (bool)FALSE, linebuf, bufsize, 0);

	return OK;
}

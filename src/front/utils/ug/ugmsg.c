/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<st.h>
# include	<cm.h>
# include	<ug.h>

/**
** Name:    ugmsg.c -	Front-End Message Utility Module.
**
** Description:
**	Contains the routine used to output messages.
**
**	IIUGmsg()		message utility.
**
** History:
**	Revision 6.0  87/08/05  peter
**	Initial revision.
**
**	07/01/93 (dkh) - Fixed bug 49695.  Check formatted message
**			 for percent signs (and double them if found
**			 to escape them) if we are going to
**			 display the message via the forms system.
**			 The chosen routine to do this wants to
**			 format the message again since all other
**			 clients have not formatted their message yet.
**			 We can't simply delay formatting the message
**			 since the message uses ER style formatting
**			 commands while the latter routine uses SI
**			 style commands.  This leaves us the solution
**			 that is used here.
**	07/10/93 (dkh) - NULL terminate the copied message that was missing
**			 from previous submission.
**	07-mar-1997 (canor01)
**	    Use TEget instead of SIgetc to wait for the <CR>.  On NT,
**	    stdin on a spawned process can get lost, but TE reads 
**	    directly from the console.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
*/

GLOBALREF i4	(*IIugmfa_msg_function)();

/*	Defines */

#define	MAXMSG		256

/*{
** Name:	IIUGmsg() -	Front-End Message Utility.
**
** Description:
**	Places message on user's screen.  If Forms System is active, the message
**	is placed on the bottom of the user's screen.  Otherwise, the message is
**	printed on the standard output.  The input message string can be
**	in the form of the 'printf()' function.  Message may have "[HIT RETURN]"
**	appended to it and returns once user has hit return.
**
**	The arguments sent to this routine are as described in the 'IIUGfmt()'
**	documentation for international arguments.
**
** Input:
**	str	{char *}  'IIUGfmt()' type format string
**	wait	{bool} whether the routine waits for a <CR> to be typed after
**			printing the message.  In this case, the string
**			"[HIT RETURN]" is appended to the message.
**	parcount {nat}  number of parameters to message.
**	a1-a10	{PTR}  arguments to be formatted into 'str'.
**
** History:
**	04-aug-1987 (peter)
**		Written.
**  	11-may-1988 (neil)
**		Modified non-forms output to go to stdout and not stderr.
**		Programs running w/o the forms system will have all errors
**		and messages going to a consistent place.
**	11/14/89  (wolf)  Add 'SIflush(stdout)' if no forms and wait.
**	16-mar-93 (fraser)
**		Cast argument to (*IIugmfa_msg_function)().
**	07-mar-1997 (canor01)
**	    Use TEget instead of SIgetc to wait for the <CR>.  On NT,
**	    stdin on a spawned process can get lost, but TE reads 
**	    directly from the console.
*/

/* VARARGS3 */
VOID
IIUGmsg (str, wait, parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)	
char	*str;				/* 'IIUGfmt' format */
bool	wait;
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
    char	msgstr[MAXMSG+1];
    char	outstr[MAXMSG+1];
    char	*msgptr;
    char	*hitenter;

# ifdef  FT3270
    if (IIugmfa_msg_function != NULL || !wait ||
		STlength(str) + STlength(hitenter = ERget(FE_HitEnter)) + 2 >=
					sizeof(msgstr))
# else
    if (!wait || STlength(str) + STlength(hitenter = ERget(FE_HitEnter)) + 2 >=
					sizeof(msgstr))
# endif
	msgptr = str;
    else
    	msgptr = STprintf(msgstr, ERx("%s %s"), str, hitenter);

    _VOID_ IIUGfmt(outstr, MAXMSG, msgptr, parcount,
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

    if (IIugmfa_msg_function != NULL)
    {
	char	*ip;
	char	*op;

	/*
	**  Need to double any percent signs (%) remaining in
	**  msgstr since the forms output routine (FTmessage)
	**  will reformat the string using C semantics.  To
	**  avoid any misinterpretation of any percent signs
	**  remaining after IIUGfmt has been called, we need
	**  to double remaining percent signs.
	*/
	ip = outstr;
	op = msgstr;
	for ( ; *ip != EOS; )
	{
		/*
		**  If a percent sign, escape it with another one.
		*/
		if (*ip == '%')
		{
			*op++ = '%';
		}
		CMcpyinc(ip, op);
	}
	*op = EOS;

	(*IIugmfa_msg_function)(msgstr, (bool) FALSE, wait);
    }
    else
    { /* no Forms -- output directly */
	SIfprintf(stdout, outstr);
	if (!wait)
	{
	    SIputc('\n', stdout);
	    SIflush(stdout);
	}
	else
	{
	    char c;

	    SIflush(stdout);
	    while ((c = TEget()) != '\n' && c != '\r')
		;
	}
    }
}

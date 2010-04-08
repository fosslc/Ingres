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

/**
** Name:    femsg.c -	Front-End Message Utility Module.
**
** Description:
**	Contains the routine used to output messages.
**
**	FEmsg()		message utility.
**
** History:
**	Revision 6.0  04-apr-1987  (peter)
**	Changed to use different global routine pointer.
**
**	Revision 4.1  86/02/27  21:43:44  wong
**	Generalized to use forms message routine pointer
**	or print to standard error.
**	01-oct-96 (mcgem01)
**	    extern definition of IIugmfa_msg_function changed to GLOBALREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF i4	(*IIugmfa_msg_function)();

/*{
** Name:    FEmsg() -	Front-End Message Utility.
**
** Description:
**	Places message on user's screen.  If Forms System is active, the message
**	is placed on the bottom of the user's screen.  Otherwise, the message is
**	printed on the standard error output.  The input message string can be
**	in the form of the 'printf()' function.  Message may have "[HIT RETURN]"
**	appended to it and returns once user has hit return.
**
** Input:
**	str	{char *}  'printf()' type format string
**	wait	{bool} whether the routine waits for a <CR> to be typed after
**			printing the message.  In this case, the string
**			"[HIT RETURN]" is appended to the message.
**	args	{nat}  arguments to be formatted into 'str'.
**
** History:
**	??/?? (???) -- Written.
**	02/86 (jhw) -- Generalized to use forms message routine pointer.
**	04-apr-1987 (peter)
**		Use IIugmfa_msg_function instead of FEmsg_routine.
**	10-sep-1993 (kchin)
**		Changed type of arguments: a1-a10 to PTR in FEmsg().  As
**		these arguments will be used to hold pointers.
*/

/*VARARGS2*/
VOID
FEmsg (str, wait, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)	
char	*str;				/* 'printf()' format */
bool	wait;
PTR	a1,a2,a3,a4,a5,a6,a7,a8,a9,a10;	/* arguments */
{
    char	msgstr[256];
    char	*mp;
    char	*hitenter;

#ifdef FT3270
    if (IIugmfa_msg_function != NULL || !wait ||
		STlength(str) + STlength(hitenter = ERget(FE_HitEnter)) + 2 >=
    							sizeof(msgstr))
#else
    if (!wait || STlength(str) + STlength(hitenter = ERget(FE_HitEnter)) + 2 >=
							sizeof(msgstr))
#endif
	mp = str;
    else
	mp = STprintf(msgstr, ERx("%s %s"), str, hitenter);

    if (IIugmfa_msg_function != NULL)
	(*IIugmfa_msg_function)(mp, FALSE, wait,
				a1,a2,a3,a4,a5,a6,a7,a8,a9,a10
	);
    else
    { /* no Forms -- output directly */
	SIfprintf(stderr, mp, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
	if (!wait)
	{
	    SIputc('\n', stderr);
#ifdef VMS
	    SIflush(stderr);
#endif
	}
	else while (SIgetc(stdin) != '\n')
		;
    }
}

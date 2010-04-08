/*
**  Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<rtvars.h>

/*
**  Name:	IIboxerr  - write out an error message to a user.
**
**  Description:
**	This is called by the EQUEL 
**		##	boxerr (short=string, long=string)
**	command, in order to put out an error message.  
**	If the shortstring is not specified or all blanks, control 
**	goes directly to the long string.  If the longstring is not 
**	specified, or all blanks, no help is available when the
**	short string is specified.
**
**  Inputs:
**	shortstring	- string to print for single line.
**			  NULL or all blanks if no single line message.
**	longstring	- string to print for long message.
**			  NULL or all blanks if no window message.
**
**  Outputs:
**	OK		if succeeded
**	STATUS		from FTboxerr.
**
**  History:
**
**	26-mar-1987 (peter)
**		Taken from fterror.c.  Add second parameter and
**		single line message.
**	04/09/88 (dkh) - Changed for VENUS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*	External declarations */

FUNC_EXTERN	STATUS	FTboxerr();

i4
IIboxerr(shortmsg, longmsg)
register char	*shortmsg;
register char	*longmsg;
{
	return ((i4)FTboxerr(shortmsg, longmsg, IIfrscb));
}

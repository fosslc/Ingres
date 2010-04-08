/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>

/**
** Name:	iiprnscr.c	- libq cover for ftprnscr.
**
** Description:
**	This file defines.
**
**	iiprnscr	libq cover for ftprnscr.
**
** History:	$Log - for RCS$
**/

/*{
** Name:	iiprnscr	- libq cover for ftprnscr.
**
** Description:
**	Libq cover routine for the EQUEL statement:
**
**		PRINTSCREEN (FILE=name)
**
**	This call ftprnscr to write the current form to a file.
**
** Inputs:
**	filename	name of file to print, or NULL.
**
** Outputs:
**	Returns:
**		VOID.
**	Exceptions:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	07-aug-85 (peter) written.
**	01-dec-85 (neil)  modified to strip blanks.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/
VOID
IIprnscr(filename)
char	*filename;
{
	char			filebuf[MAX_LOC +1];
	register char		*f;

	if ((f = filename) != (char *)0)
	{
	    STlcopy( f, filebuf, MAX_LOC );
	    STtrmwhite( filebuf );
	    f = filebuf;
	}
	FTprnscr( f );
}

/*
**	recover.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	recover.c - Recover keystrokes typed by users.
**
** Description:
**	This file contains routines to save keystrokes typed
**	by users using the forms system.  Convienent for creating
**	keystrokes files for testing purposes.  Routines defined here:
**	- FDrcvalloc - Open output file to place saved keystrokes.
**	- FDrcvclose - Close keytroke file.
**
** History:
**	02/15/87 (dkh) - Added header.
**	12/19/87 (dkh) - Fixed jup bug 1648.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>


/*{
** Name:	FDrcvalloc - Open keystroke file.
**
** Description:
**	Open a file to save user keystrokes.  The name to use
**	is passed in.
**
** Inputs:
**	fname	Name of file to open for saving keystrokes.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Causes subsequent keystrokes to be saved in the opened file.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDrcvalloc(fname)
char	*fname;
{
	FTsvinput(fname);
}



/*{
** Name:	FDrcvlose - Close the keystroke file.
**
** Description:
**	Closes the keystroke file opened by a previous call to
**	FDrcvalloc().  The file will also be removed if the
**	passed in argument is TRUE.
**
** Inputs:
**	rem	Causes the keystroke file to be removed if this is TRUE.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Subsequent user keystrokes will no longer be saved.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDrcvclose(rem)
bool	rem;
{
	FTsvclose(rem);
}

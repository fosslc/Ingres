/*
**	Copyright (c) 1987 Ingres Corporation
*/

# include	<compat.h>

/**
** Name:	telock.c -	Allow or disallow screen updating by a
**				cooperating process.
**
** Description:
**	This file defines:
**
**	TElock	Allow or disallow screen modification by a cooperating process.
**
** History:
**	14-oct-87 (bab)
**		Finally created.
**/

/*{
** Name:	TElock	- Allow or disallow screen modification by a
**			  cooperating process.
**
** Description:
**	This entry point is a stub for all environments but Data General.
**	It is used to bracket sections of code within which the terminal
**	screen may not be modified by another cooperating process.  For
**	DG, the process being locked out is that used to update the
**	Comprehensive Electronic Office (CEO) status line.
**	
**	Screen modification by the cooperating process is disallowed until
**	this routine is first called to unlock the terminal.  At that time,
**	the current cursor location is also specified.  Multiple attempts
**	to lock the terminal will be accepted, but the first call to unlock
**	the screen will succeed.
**	
**	Coordinate system is zero based with the origin at the upper
**	left corner of the screen with the X coordinate increasing to
**	the right and the Y coordinate increasing towards the bottom.
**	
**	Locking is called only when the forms system is about to update
**	the screen.  Thus the forms system will set the lock when it is
**	about to update the terminal screen and unlock when it is finished.
**	It is assumed that if an INGRES application exits, the cooperating
**	process also exits.  There is also no danger of locking the terminal
**	physically since the locking mechanism described here is only for a
**	parent and cooperating process.  No other processes are affected.
**	
**	Usage is TElock(TRUE, (i4) 0, (i4) 0) to lock the screen,
**	TElock(FALSE, row, column) to re-enable screen updates.
**	
** Inputs:
**	lock	Specify whether the program is to lock out screen updates.
**	row	Specify the cursor's y coordinate; ignored if screen
**		is locked.  Possible values range from 0 to the
**		screen length - 1.
**	col	Specify the cursor's x coordinate; ignored if screen is
**		locked.  Possible values range from 0 to the screen
**		width - 1.
**
** Outputs:
**	None.
**
**	Returns:
**		VOID.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	This is a stub for all non-DG systems.
**
** History:
**	08-sep-87 (bab)	Described
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/
VOID
TElock(
bool	lock,
i4	row,
i4	col)
{

}

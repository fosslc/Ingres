/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>

/**
** Name:	abrtbeep.sc -	ABF Run-Time Beep Function.
**
** Description:
**	Contains the routine used to implement the beep system function.
**	Defines:
**
**	IIARbeep()	beep the terminal.
**
** History:
**	Revision 8.0  89/10  wong
**	Initial revision.
**/

/*{
** Name:	IIARbeep() -	Beep the Terminal.
**
** Description:
**	Outputs a beep or rings the bell on the user's display device.
**
** History:
**	10/89 (jhw) -- Written.
**	06-oct-1993 (mgw)
**		Removed <ug.h> include because of it's new dependancy on
**		<fe.h>. Removed a few other includes because they aren't
**		needed here.
*/

VOID
IIARbeep ()
{
	FTbell();
}

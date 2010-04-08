/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/**
** Name:	iisleep.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIsleep()
**	Private (static) routines defined:
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	void	PCsleep();

/*{
** Name:	IIsleep		-	Sleep the specified no. of seconds
**
** Description:
**	An EQUEL '## SLEEP' statement will generate a call to this routine.
**	It simply calls PCsleep.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	sec		The number of seconds to sleep
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

IIsleep(sec)
i4	sec;
{
	PCsleep((u_i4) (sec * 1000));
}

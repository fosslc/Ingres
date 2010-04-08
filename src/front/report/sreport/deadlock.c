/* &
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<stype.h>
#include	<sglob.h>
# include	"ersr.h"

/*{
** Name:    err_proc() -	Report Specifier EQUEL Error Processor.
**
** Description:
**	ERRPROC - error routine called by ingres to process errors in equel
**		statements. Here deadlock errors are detected by SREPORT
**		and RBF. If a deadlock is detected, a message is printed and
**		a global variable is set.
**
** Returns:
**	0 if the error was a deadlock so no error message is
**		printed by ingres.
**	Otherwise the error number is returned so the error
**	message will be printed.
**
** Called by:
**	Set up by smain and rfrbf through iiseterr.
**
** Side effects:
**	Sets ingerr.
**	Sets ing_deadlock if a deadlock was detected.
**
** History:
**	2/22/84 (gac)	written.
**	26-aug-1992 (rdrane)
**		Move ing_deadlock global to rglob[i].h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	ERR_DEADLOCK	4700
# define	ERR_COPY	5826

static bool	ingerr = FALSE;		/* Ingres error */


i4
errproc(errnum)
i4	*errnum;
{
	ingerr = TRUE;
	if (*errnum == ERR_DEADLOCK)
	{
		if (!St_silent)
		{
			/* Deadlock Message */
			FEmsg(ERget(E_SR0002_Deadlock_occurred), FALSE);
		}
		ing_deadlock = TRUE;
		return(0);
	}
	else if (*errnum == ERR_COPY)
	{
		return(0);
	}

	return(*errnum);

	/* Return the error number to have the error message displayed;
	   to suppress error message printing, return(0) is included. */
}



/*
**	DEADLOCK - This checks if a deadlock has occurred.
**
**	Returns:
**		TRUE	if a deadlocked occurred.
**		FALSE	otherwise.
**
**	Called by:
**		s_app_reports, s_del_old, s_copy_new, rFcatalog.
**
**	Side Effects:
**		Resets ingerr.
**
**	History:
**		2/22/84 (gac)	written.
*/

bool
deadlock()
{
	if (ingerr)
	{
		ingerr = FALSE;
		if (ing_deadlock)
			return(TRUE);
		else
			return(FALSE);
	}
	else
	{
		return(FALSE);
	}
}

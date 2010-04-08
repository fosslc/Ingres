/*
**  DEADLOCK.C
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)deadlock.c	30.1	12/3/84";
**
**  Routines for handling deadlock when writing out
**  forms to the database.
**
**  Borrowed from RBF, first written by gac.
**
**  NOTE - This file does not use IIUGmsg since it does
**  note restore the message line.
**
**  History:
**	10-15-84(dkh)	Borrowed from RBF.
**	08/14/87 (dkh) - ER changes.
**	12/12/87 (dkh) - Added comment on using IIUGmsg.
**      25-sep-96 (mcgem01)
**              Global data moved to vfdata.c
**	20-dec-96 (chech02)
**		Changed vfdlock to GLOBALREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ervf.h"

# define	MST_DEADLOCK	4700
# define	COPY_ERR	5826

static	bool	vfingerr = FALSE;
GLOBALREF	bool	vfdlock;

/*
**  vferrproc
**
**  Vifred equel error routine to call when an equel error occurs.  If
**  routine returns 0 then equel will not print an error message.
**  This routine will set the appropriate variables when a
**  deadlock occurs.
*/

i4
vferrproc(errnum)
i4	*errnum;
{
	vfingerr = TRUE;
	if (*errnum == MST_DEADLOCK)
	{
		winerr(ERget(S_VF0032_Deadlock_occurred_whi));
		vfdlock = TRUE;
		return(0);
	}
	else if (*errnum == COPY_ERR)
	{
		return(0);
	}

	return(*errnum);
}


/*
**  vfdeadlock
**
**  Routine checks to see if a deadlock has occurred.
**  Return TRUE if a deadlock has occurred, FALSE otherwise.
**  Variable vfingerr will be reset if it was set.
*/

bool
vfdeadlock()
{
	if (vfingerr)
	{
		vfingerr = FALSE;
		if (vfdlock)
		{
			return(TRUE);
		}
		else
		{
			return(FALSE);
		}
	}
	else
	{
		return(FALSE);
	}
}

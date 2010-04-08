# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  CHANGE WORKING DIRECTORY
**
**	9/10/80 (djf)	changed chdir() to _setdfdi() for vms.
**	1/25/90 (teresal) Modified to comply with standards for calling
**			  LOfroms routine.
*/

newdirec()
{
	register char	*direc;
	FUNC_EXTERN char	*getfilenm();
	LOCATION	loc;
	char		locbuf[MAX_LOC+1];
	i4		status;
	char		errbuf[ER_MAX_LEN];

	direc = getfilenm();
	/* if the directory is not specified, don't do anything. */
	if (!*direc)
		return;
	STcopy(direc, locbuf);
	if (status = LOfroms(PATH, locbuf, &loc))
	{
		ERreport(status, errbuf);
		putprintf(ERget(F_MO0027_Error_in_path_name), errbuf);
		return;
	}
	if (status = LOchange(&loc))
	{
		ERreport(status, errbuf);
		putprintf(ERget(F_MO0028_Cannot_change_dir), direc, errbuf);
	}
}

/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cv.h>
# include <lo.h>
# include <si.h>
# include <pc.h>
# include <er.h>

/**
** Name:	pidfile.c - process id file
**
** Description:
**	Defines
**		check_pid	- check process id
**		remove_pidfile	- remove pid file
**
** History:
**	16-dec-96 (joea)
**		Created based on pidfile.c in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define PIDFILE_NAME		ERx("rep.pid")


FUNC_EXTERN void RSshutdown(STATUS status);


/*{
** Name:	check_pid - check process id
**
** Description:
**	Verifies if a server is already running.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
**
** Side effects:
**	The file rep.pid is written to the server directory.
*/
void
check_pid()
{
	PID		pid;
	FILE		*pidfile;
	LOCATION	loc;
	char		filename[MAX_LOC+1];
	char		strpid[13];
	STATUS		status;

	STcopy(PIDFILE_NAME, filename);
	LOfroms(FILENAME, filename, &loc);
	if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &pidfile) == OK)
	{
		SIgetrec(strpid, sizeof(strpid), pidfile);
		SIclose(pidfile);
		STtrmwhite(strpid);
		status = CVal(strpid, (i4 *)&pid);

		if (PCis_alive(pid))
		{
			/* Server is already running */
			messageit(1, 78);
			RSshutdown(FAIL);
		}
	}

	/* write the pid to the rep.pid file in the server directory */
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &pidfile) == OK)
	{
		PCpid(&pid);
		SIfprintf(pidfile, ERx("%d\n"), pid);
		SIclose(pidfile);
	}
}


/*{
** Name:	remove_pidfile - removes the pid file
**
** Description:
**	Removes the file "rep.pid" from the server directory.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
remove_pidfile()
{
	LOCATION	loc;
	char		filename[MAX_LOC+1];

	STcopy(PIDFILE_NAME, filename);
	LOfroms(FILENAME, filename, &loc);
	LOdelete(&loc);
}

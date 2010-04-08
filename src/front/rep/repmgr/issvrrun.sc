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
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	issvrrun.sc - is the server running?
**
** Description:
**	Defines
**		is_server_running	- is a server running?
**
** History:
**	16-dec-96 (joea)
**		Created based on issvrrun.sc in replicator library.
**	03-jun-98 (abbjo03)
**		Use RMget_server_dir to prepare server directory.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Use RPdb_error_check
**		instead of INQUIRE_SQL.
**/

FUNC_EXTERN STATUS RMget_server_dir(i2 server_no, LOCATION *loc, char *path);


/*{
** Name:	is_server_running - is a server running?
**
** Description:
**	Determines if a Distribution or Replication server is running.
**
** Inputs:
**	server_no	- the server number
**
** Outputs:
**	none
**
** Returns:
**	0 - the given server is not running
**	1 - the given server is running
*/
bool
is_server_running(
i2	server_no)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	server_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	PID	pid = 0;
	FILE	*pidfile;
	char	file_buf[MAX_LOC+1];
	EXEC SQL BEGIN DECLARE SECTION;
	char	tmp[100];
	EXEC SQL END DECLARE SECTION;
	LOCATION	loc;

	if (RMget_server_dir(server_no, &loc, file_buf) != OK)
		return (FALSE);
	LOfstfile(ERx("rep.pid"), &loc);

	if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &pidfile) == OK)
	{
		SIgetrec(tmp, sizeof(tmp) - 1, pidfile);
		SIclose(pidfile);
		STtrmwhite(tmp);
		CVal(tmp, &pid);
		return (PCis_alive(pid));
	}
	else
	{
		EXEC SQL SELECT pid
			INTO	:tmp
			FROM	dd_servers
			WHERE	server_no = :server_no;
		if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
		{
			IIUGerr(E_RM00E8_Err_rtrv_pid, UG_ERR_ERROR, 0);
			return (FALSE);
		}
		STtrmwhite(tmp);
		CVal(tmp, &pid);
		if (pid == 0)
			return (FALSE);
		return (PCis_alive(pid));
	}
}

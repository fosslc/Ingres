/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <nm.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include "errm.h"

/**
** Name:	svgenlog.c - save replicator generate log
**
** Description:
**	Defines
**		save_rpgen_log	- save code generation log
**
** History:
**	16-dec-96 (joea)
**		Created based on svgenlog.sc in replicator library.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**      21-apr-1999 (hanch04)
**	    Added st.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	save_rpgen_log	- save code generation log
**
** Description:
**	Appends the contents of rpgen.log to rpgen.err and then calls
**	file_display to show the contents of rpgen.log.
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
save_rpgen_log()
{
	FILE	*log;
	FILE	*errlog;
	char	*p;
	char	*logname = ERx("rpgen.log");
	LOCATION	loc;
	char	filename[MAX_LOC+1];
	char	buf[SI_MAX_TXT_REC];

	if (NMloc(TEMP, PATH & FILENAME, logname, &loc) != OK)
		return;

	if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &log) != OK)
	{
		LOtos(&loc, &p);
		STcopy(p, filename);
		IIUGerr(E_RM00F3_Err_open_logfile, UG_ERR_ERROR, 1, filename);
		return;
	}

	if (NMloc(TEMP, PATH & FILENAME, ERx("rpgen.err"), &loc) != OK)
		return;

	if (SIfopen(&loc, ERx("a"), SI_TXT, SI_MAX_TXT_REC, &errlog) != OK)
	{
		LOtos(&loc, &p);
		STcopy(p, filename);
		IIUGerr(E_RM00F2_Err_open_outfile, UG_ERR_ERROR, 1, filename);
		return;
	}

	while (SIgetrec(buf, (i4)sizeof(buf), log) != ENDFILE)
		SIputrec(buf, errlog);

	SIclose(log);
	SIclose(errlog);

	if (NMloc(TEMP, PATH & FILENAME, logname, &loc) != OK)
		return;
	LOtos(&loc, &p);
	STcopy(p, filename);
	file_display(filename);
}

/*
** Copyright (c) 2004 Ingres Corporation
*/
EXEC SQL INCLUDE sqlca;

# include <compat.h>
# include <st.h>
# include <si.h>
# include <lo.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	pathrpt.sc - Data propagation paths report
**
** Description:
**	Defines
**		paths_report - Data propagation paths report
**
** History:
**	16-dec-96 (joea)
**		Created based on pathrpt.sc in replicator library.
**	01-oct-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Replace string literals
**		with ERget calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	paths_report - Data propagation paths report
**
** Description:
**	Create a report from the dd_paths table.
**
** Inputs:
**	filename	- full path and name of the output file
**
** Outputs:
**	none
**
** Returns:
**	OK, E_RM0087_Err_open_report or the sqlca.sqlcode if an error occurs.
**
** Side effects:
**	A report file is created.  A COMMIT is issued after the report is
**	created.
*/
STATUS
paths_report(
char	*filename)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	db[65];
	i2	source_no;
	char	source_db[65];
	i2	cdds_no;
	char	cdds_name[DB_MAXNAME+1];
	i2	local_no;
	char	local_db[65];
	i2	target_no;
	char	target_db[65];
	i2	last_cdds_no;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO errinfo;
	FILE	*report_fp;
	char	report_filename[MAX_LOC+1];
	LOCATION	loc;
	char	tmp[128];	/* strings for the report */
	char	*p;
	bool	first = TRUE;
	u_i4	i;
	STATUS	status;
	char	*line =
		ERx("-------------------------------------------------------------------------------");

	STcopy(filename, report_filename);
	LOfroms(PATH & FILENAME, report_filename, &loc);
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &report_fp) != OK)
	{
		IIUGerr(E_RM0087_Err_open_report, UG_ERR_ERROR, 1,
			report_filename);
		return (E_RM0087_Err_open_report);
	}

	EXEC SQL SELECT TRIM(database_name) + ' ' + VARCHAR(database_no)
		INTO	:db
		FROM	dd_databases
		WHERE	local_db = 1;

	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		IIUGerr(E_RM0030_Err_retrieve_db_name, UG_ERR_ERROR, 0);
		goto rpt_err;
	}

	EXEC SQL SELECT c.cdds_no, s.database_no, l.database_no, t.database_no,
			TRIM(s.vnode_name) + '::' + TRIM(s.database_name),
			TRIM(l.vnode_name) + '::' + TRIM(l.database_name),
			TRIM(t.vnode_name) + '::' + TRIM(t.database_name),
			c.cdds_name
		INTO	:cdds_no, :source_no, :local_no, :target_no,
			:source_db,
			:local_db,
			:target_db,
			:cdds_name
		FROM	dd_databases s, dd_databases l, dd_databases t,
			dd_cdds c, dd_paths p
		WHERE	p.cdds_no = c.cdds_no
		AND	p.sourcedb = s.database_no
		AND	p.localdb = l.database_no
		AND	p.targetdb = t.database_no
		ORDER	BY 1, 2, 3, 4;
	EXEC SQL BEGIN;
		if (first)
			last_cdds_no = cdds_no;
		STtrmwhite(cdds_name);
		if (first || cdds_no != last_cdds_no)
		{
			if (!first)
				SIfprintf(report_fp, ERx("\f\n"));
			date_time(report_fp);
			p = ERget(F_RM00A9_Prod_name);
			i = 40 + STlength(p) / (u_i4)2;
			SIfprintf(report_fp, ERx("%*s\n"), i, p);
			SIfprintf(report_fp, ERx("%s"), db);
			p = ERget(F_RM00B3_Cdds_paths_rpt);
			i = 40 - STlength(db) + STlength(p) / (u_i4)2;
			SIfprintf(report_fp, ERx("%*s\n\n"), i, p);
			SIfprintf(report_fp, ERx("%s\n"), line);
			SIfprintf(report_fp, ERget(F_RM00B4_Cdds_no));
			SIfprintf(report_fp, " %5d  %s\n\n", cdds_no,
				cdds_name);
			SIfprintf(report_fp, ERx("%-25s\n"),
				ERget(F_RM00B5_Orig_db));
			SIfprintf(report_fp, ERx("%5s%-s\n"), ERx(" "),
				ERget(F_RM00B6_Local_db));
			SIfprintf(report_fp, ERx("%10s%-s\n"), ERx(" "),
				ERget(F_RM00B7_Targ_db));
			SIfprintf(report_fp, ERx("%s\n"), line);
			first = FALSE;
		}

		SIfprintf(report_fp, ERx("%5d  "), source_no);
		SIfprintf(report_fp, ERx("%s\n"), source_db);

		SIfprintf(report_fp, ERx("     %5d  "), local_no);
		SIfprintf(report_fp, ERx("%s\n"), local_db);

		SIfprintf(report_fp, ERx("%10s%5d  "), ERx(" "), target_no);
		SIfprintf(report_fp, ERx("%s\n\n"), target_db);
		last_cdds_no = cdds_no;
	EXEC SQL END;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
	{
		IIUGerr(E_RM0121_Err_rtrv_paths, UG_ERR_ERROR, 0);
		goto rpt_err;
	}

	EXEC SQL COMMIT;
	SIclose(report_fp);
	return (OK);

rpt_err:
	EXEC SQL WHENEVER SQLERROR CONTINUE;
	status = -sqlca.sqlcode;
	EXEC SQL ROLLBACK;
	SIclose(report_fp);
	return (status);
}

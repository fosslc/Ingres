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
# include "errm.h"

/**
** Name:	tablerpt.sc - Registered tables report
**
** Description:
**	Defines
**		tables_report - Registered tables report
**		footer - print legend at foot of page
**
** FIXME:
**	Needs to deal with an empty report.
**
** History:
**	16-dec-96 (joea)
**		Created based on tablerpt.sc in replicator library.
**	04-apr-97 (joea)
**		Change description for R - Rules Created to Change Recording
**		Activated.
**	09-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Replace string literals
**		with ERget calls.
**	23-oct-98 (abbjo03)
**		Break up the second line of the column headers which became
**		joined in the 9-sep change. Correct value of Change Recorder
**		Activated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-nov-2009 (joea)
**          Add test for "boolean" in tables_report.
**/

# define	FOOTER_LINE		52


static void footer();


static FILE	*report_fp;
static i4	pagenum;


/*{
** Name:	tables_report - Registered tables report
**
** Description:
**	Create a report from the dd_regist_columns and
**	dd_regist_tables tables.
**
** Inputs:
**	filename	- full path and name of the output file
**
** Outputs:
**	none
**
** Returns:
**	OK, E_RM0087_Err_open_report or sqlca.sqlcode if error occurs.
**
** Side effects:
**	A report file is created. A COMMIT is issued after the report is
**	created.
*/
STATUS
tables_report(
char	*filename)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	localdb[DB_MAXNAME*2+3];
	char	table_name[DB_MAXNAME+1];
	i4	col_sequence;
	char	columns_registered[26];
	char	suppobj_created[26];
	char	rules_created[26];
	i2	cdds_no;
	char	cdds_name[DB_MAXNAME+1];
	char	cdds_lookup_table[DB_MAXNAME+1];
	char	prio_lookup_table[DB_MAXNAME+1];
	char	column_name[DB_MAXNAME+1];
	i4	key_sequence;
	i4	column_sequence;
	char	column_datatype[DB_MAXNAME+1];
	i4	column_length;
	char	column_nulls[2];
	char	column_defaults[2];
	EXEC SQL END DECLARE SECTION;
	char	last_table_name[DB_MAXNAME+1];
	char	datatype[36];
	char	report_filename[MAX_LOC+1];
	LOCATION	loc;
	char	tmp[128];	/* strings for the report */
	u_i4	i;
	STATUS	status;
	char	*p;
	bool	first = TRUE;
	bool	newpage = TRUE;
	i4	linecount;
	char	*line =
		ERx("------------------------------------------------------------------------------");

	STcopy(filename, report_filename);
	LOfroms(PATH & FILENAME, report_filename, &loc);
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &report_fp) != OK)
	{
		IIUGerr(E_RM0087_Err_open_report, UG_ERR_ERROR, 1,
			report_filename);
		return (E_RM0087_Err_open_report);
	}

	EXEC SQL WHENEVER sqlerror GOTO rpt_err;

	EXEC SQL SELECT TRIM(database_name) + ' ' + VARCHAR(database_no)
		INTO	:localdb
		FROM	dd_databases
		WHERE	local_db = 1;

	pagenum = 1;
	linecount = 0;
	EXEC SQL SELECT t.table_name, i.column_sequence,
			TRIM(t.columns_registered), TRIM(t.supp_objs_created),
			TRIM(t.rules_created), t.cdds_no, s.cdds_name,
			t.cdds_lookup_table, t.prio_lookup_table,
			c.column_name, c.key_sequence, c.column_sequence,
			TRIM(LOWERCASE(i.column_datatype)), i.column_length,
			i.column_nulls, i.column_defaults
		INTO	:table_name, :col_sequence,
			:columns_registered, :suppobj_created,
			:rules_created, :cdds_no, :cdds_name,
			:cdds_lookup_table, :prio_lookup_table,
			:column_name, :key_sequence, :column_sequence,
			:column_datatype, :column_length,
			:column_nulls, :column_defaults
		FROM	dd_regist_columns c, dd_regist_tables t, dd_cdds s,
			iicolumns i
		WHERE	t.cdds_no = s.cdds_no
		AND	c.table_no = t.table_no
		AND	t.table_name = i.table_name
		AND	t.table_owner = i.table_owner
		AND	c.column_name = i.column_name
		ORDER	BY 1, 2;
	EXEC SQL BEGIN;
		if (first)
			STcopy(table_name, last_table_name);
		STtrmwhite(cdds_name);
		if (newpage)
		{
			if (!first)
				SIfprintf(report_fp, ERx("\f\n"));
			date_time(report_fp);
			p = ERget(F_RM00A9_Prod_name);
			i = 40 + STlength(p) / (u_i4)2;
			SIfprintf(report_fp, ERx("%*s\n"), i, p);
			p = ERget(F_RM00B8_Regist_tbls_rpt);
			i = 40 + STlength(p) / (u_i4)2;
			SIfprintf(report_fp, ERx("%*s\n"), i, p);
			SIfprintf(report_fp, ERx("%s\n\n"), localdb);
			SIfprintf(report_fp, ERx("%s\n"), line);
			SIfprintf(report_fp, ERx("%-35s"),
				ERget(F_RM00B9_Table));
			SIfprintf(report_fp, ERx("%-10s"),
				ERget(F_RM00BA_C_S_A));
			SIfprintf(report_fp, ERget(F_RM00BB_Prio_lkup_tbl));
			SIfprintf(report_fp, ERx("\n%-45s"),
				ERget(F_RM00BC_Cdds));
			SIfprintf(report_fp, ERget(F_RM00BD_Cdds_lkup_tbl));
			SIfprintf(report_fp, ERx("\n%-35s"),
				ERget(F_RM00BE_Column));
			SIfprintf(report_fp, ERx("%-36s"),
				ERget(F_RM00BF_Datatype));
			SIfprintf(report_fp, ERget(F_RM00C0_D_K_R));
			SIfprintf(report_fp, ERx("\n%s\n"), line);
			newpage = FALSE;
			linecount = 12;
		}
		if (first || !STequal(table_name, last_table_name))
		{
			if (!first)
				SIputc('\n', report_fp);
			if (*columns_registered != EOS)
				STcopy(ERx("C"), columns_registered);
			if (*suppobj_created != EOS)
				STcopy(ERx("S"), suppobj_created);
			if (*rules_created != EOS)
				STcopy(ERx("A"), rules_created);
			SIfprintf(report_fp, ERx("%-35s"), table_name);
			SIfprintf(report_fp, ERx("%-2s"), columns_registered);
			SIfprintf(report_fp, ERx("%-2s"), suppobj_created);
			SIfprintf(report_fp, ERx("%-6s"), rules_created);
			SIfprintf(report_fp, ERx("%s\n"), prio_lookup_table);

			STprintf(tmp, ERx(" %5d %s"), cdds_no, cdds_name);
			SIfprintf(report_fp, ERx("%-45s"), tmp);
			SIfprintf(report_fp, ERx("%s\n"), cdds_lookup_table);
			first = FALSE;
			linecount += 2;
		}

		SIfprintf(report_fp, ERx("  %-33s"), column_name);
		STcopy(column_datatype, datatype);
		if (STequal(datatype, ERx("integer")))
		{
			switch (column_length)
			{
			case 2:
				STcopy(ERx("smallint"), datatype);
				break;

			case 1:
				STcopy(ERx("integer1"), datatype);
				break;

			case 4:
			default:
				break;
			}
		}
		else if (STequal(datatype, ERx("float")))
		{
			if (column_length == 4)
				STcopy(ERx("float4"), datatype);
		}
		else if (!STequal(column_datatype, ERx("date")) &&
			!STequal(column_datatype, ERx("ingresdate")) &&
			!STequal(column_datatype, ERx("ansidate")) &&
			!STequal(column_datatype, ERx("time without time zone")) &&
			!STequal(column_datatype, ERx("timestamp without time zone")) &&
			!STequal(column_datatype, ERx("time with time zone")) &&
			!STequal(column_datatype, ERx("timestamp with time zone")) &&
			!STequal(column_datatype, ERx("time with local time zone")) &&
			!STequal(column_datatype, ERx("timestamp with local time zone")) &&
			!STequal(column_datatype, ERx("interval year to month")) &&
			!STequal(column_datatype, ERx("interval day to second")) &&
                        !STequal(column_datatype, "boolean") &&
			!STequal(column_datatype, ERx("money")))
		{
			i = STlength(datatype);
			STprintf(&datatype[i], ERx("(%d)"), column_length);
		}
		i = STlength(datatype);
		if (*column_nulls == 'Y')
		{
			STprintf(&datatype[i], ERx(" with null"));
		}
		else
		{
			STprintf(&datatype[i], ERx(" not null "));
			i += 10;
			if (*column_defaults == 'Y')
				STprintf(&datatype[i], ERx("with default"));
			else
				STprintf(&datatype[i], ERx("not default"));
		}
		SIfprintf(report_fp, ERx("%-35s"), datatype);
		if (key_sequence > 0)
			SIfprintf(report_fp, ERx("%2d  "), key_sequence);
		else
			SIfprintf(report_fp, ERx("    "));

		SIfprintf(report_fp, ERx("%s  %s\n"),
			key_sequence ? ERget(F_RM00C7_K) : ERx(" "),
			column_sequence ? ERget(F_RM00C8_R) : ERx(" "));
		++linecount;

		if (linecount > FOOTER_LINE)
		{
			footer();
			newpage = TRUE;
		}
		STcopy(table_name, last_table_name);
	EXEC SQL END;
	EXEC SQL COMMIT;
	while (linecount++ != FOOTER_LINE)
		SIputc('\n', report_fp);
	footer();
	SIclose(report_fp);
	return (OK);

rpt_err:
	EXEC SQL WHENEVER sqlerror CONTINUE;
	status = -sqlca.sqlcode;
	EXEC SQL ROLLBACK;
	SIclose(report_fp);
	return (status);
}


static void
footer()
{
	SIputc('\n', report_fp);
	SIfprintf(report_fp, ERx("%-32s"), ERget(F_RM00C1_Cols_regist));
	SIfprintf(report_fp, ERget(F_RM00C2_Tbl_key_seq));
	SIfprintf(report_fp, ERx("\n%-32s"), ERget(F_RM00C3_Supp_objs));
	SIfprintf(report_fp, ERget(F_RM00C4_Rep_key_col));
	SIfprintf(report_fp, ERx("\n%-32s"), ERget(F_RM00C5_Chg_rec_activ));
	SIfprintf(report_fp, ERget(F_RM00C6_Rep_col));
	SIfprintf(report_fp, ERx("\n\n%37s- %d -\n"), ERx(" "), pagenum++);
}

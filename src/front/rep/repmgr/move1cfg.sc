/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <si.h>
# include <cm.h>
# include <nm.h>
# include <er.h>
# include <me.h>
# include <lo.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ug.h>
# include <rpu.h>
# include <tbldef.h>
# include "errm.h"

/**
** Name:	move1cfg.sc - Move configuration information
**
** Description:
**	Defines
**		move_1_config - move configuration to one target database
**
** History:
**	09-jan-97 (joea)
**		Created based on move1cfg.sc in replicator library.
**	28-jan-97 (joea)
**		Add MAXPAGES to modify for dd_last_number to prevent lock
**		escalation. Call trace point DM102 to prevent catalog problems
**		when destroying dd_reg_tbl_idx.
**	17-mar-97 (joea) bug 81128
**		Replace dd_regist_tables.table_owner with the target DBA.
**	02-jul-97 (joea)
**		Use DELETE instead of MODIFY TO TRUNCATED/reMODIFY to initialize**		the catalogs.
**      25-Sep-97 (fanra01)
**              Modified remove_temp function to reload the location structures
**              from the statically stored filenames.  The buffer held in the
**              location structure returned from NMloc is not persistent and
**              can be modified during calls to NMloc.
**              This caused the symptom where the files in the directory
**              defined by II_CONFIG were deleted.
**	14-oct-97 (joea) bug 83765
**		Remove unused argument to dropsupp and drop_supp_objs. After
**		loading dd_regist_tables, reset supp_objs_created. Conversely,
**		after calling tbl_dbprocs, update supp_objs_created.
**	18-may-98 (padni01) bug 89865
**		Use date_gmt instead of date to set supp_objs_created field 
**		of dd_regist_tables.
**	01-oct-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	19-oct-98 (abbjo03) bug 93780
**		Connect to target database requesting exclusive lock.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-Dec-2005 (hanal04) Bug 115562 INGREP182
**          Ensure ERRORTYPE is set to genericerror after connecting to
**          avoid unexpected failures. This is consistent with other 
**          CONNECTs in front/rep/repmgr.
**/

# define DIM(a)		(sizeof(a) / sizeof(a[0]))


GLOBALREF TBLDEF	*RMtbl;


EXEC SQL BEGIN DECLARE SECTION;
static char	filenames[7][MAX_LOC+1];
EXEC SQL END DECLARE SECTION;

static struct cfgtbl
{
	char	*table_name;
	char	*prefix;
	LOCATION	loc;
} cfgtbls[] =
{
	{ ERx("dd_databases"),		ERx("db"), },
	{ ERx("dd_regist_tables"),	ERx("tb"), },
	{ ERx("dd_regist_columns"),	ERx("co"), },
	{ ERx("dd_cdds"),		ERx("cd"), },
	{ ERx("dd_db_cdds"),		ERx("dc"), },
	{ ERx("dd_paths"),		ERx("pa"), },
	{ ERx("dd_last_number"),	ERx("ls"), }
};


static void remove_temp(void);
static STATUS drop_supp_objs(void);


/*{
** Name:	move_1_config - Move configuration to one target database
**
** Description:
**	Copies the Replicator configuration from the local database to the
**	specified target.
**
** Inputs:
**	database_no	- the target database number (unused)
**	vnode_name	- the target nodename
**	dbname		- the target database
**
** Outputs:
**	none
**
** Returns:
**	OK				- success
**	E_RM0082_Error_rtrv_tables	- error retrieving registd table count
**	E_RM0120_Incomplete_cfg		- incomplete configuration
**	E_RM010C_Err_copy_out		- error copying catalog out
**	E_RM0084_Error_connecting	- error connecting to target database
**	E_RM0003_No_rep_catalogs	- catalogs don't exist
**	E_RM010D_Err_clear_cat		- error deleting catalogs
**	E_RM010E_Err_copy_in		- error copying catalog in
**	E_RM010F_Err_adj_owner		- error adjusting DBA table ownership
**	E_RM0110_Err_adj_localdb	- error adjusting local_db flag
**	E_RM0111_Err_rtrv_type		- error retrieving DBMS type
**	E_RM0112_Err_open_tbl_curs	- error opening dd_regist_tables cursor
**	E_RM0113_Err_fetch_tbl_curs	- error fetching from dd_regist_tables
**	Also may return errors from RMtbl_fetch, create_support_tables and
**	tbl_dbprocs.
*/
STATUS
move_1_config(
i2	database_no,
char	*vnode_name,
char	*dbname)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*vnode_name;
char	*dbname;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO errinfo;
	i4	i;
	char	*p;
	STATUS	err;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	old_session;
	i4	cnt;
	char	full_dbname[DB_MAXNAME*2+3];
	i4	other_session = 103;
	char	source_dba[DB_MAXNAME+1];
	char	target_dba[DB_MAXNAME+1];
	char	username[DB_MAXNAME+1];
	i4	table_no;
	char	dbms_type[9];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INQUIRE_SQL (:old_session = SESSION);

	EXEC SQL SELECT COUNT(*) INTO :cnt
		FROM	dd_regist_tables;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		IIUGerr(E_RM0082_Error_rtrv_tables, UG_ERR_ERROR, 0);
		return (E_RM0082_Error_rtrv_tables);
	}
	if (cnt == 0)
	{
		IIUGerr(E_RM0120_Incomplete_cfg, UG_ERR_ERROR, 0);
		return (E_RM0120_Incomplete_cfg);
	}

	if (*vnode_name == EOS)
		STcopy(dbname, full_dbname);
	else
		STprintf(full_dbname, ERx("%s::%s"), vnode_name, dbname);
	SIprintf(ERget(F_RM00D4_Moving_cats), full_dbname);
	SIflush(stdout);

	/* For all tables, copy data to flat files */
	for (i = 0; i < DIM(cfgtbls); ++i)
	{
		/* Build filename */
		if (NMloc(TEMP, PATH, NULL, &cfgtbls[i].loc) != OK)
			return (FAIL);
		if (LOuniq(cfgtbls[i].prefix, ERx("dat"), &cfgtbls[i].loc)
				!= OK)
			return (FAIL);
		LOtos(&cfgtbls[i].loc, &p);
		STcopy(p, filenames[i]);

		switch (i)
		{
		case 0:
			EXEC SQL COPY TABLE dd_databases ()
				INTO	:filenames[i];
			break;

		case 1:
			EXEC SQL COPY TABLE dd_regist_tables ()
				INTO	:filenames[i];
			break;

		case 2:
			EXEC SQL COPY TABLE dd_regist_columns ()
				INTO	:filenames[i];
			break;

		case 3:
			EXEC SQL COPY TABLE dd_cdds ()
				INTO	:filenames[i];
			break;
		case 4:
			EXEC SQL COPY TABLE dd_db_cdds ()
				INTO	:filenames[i];
			break;

		case 5:
			EXEC SQL COPY TABLE dd_paths ()
				INTO	:filenames[i];
			break;

		case 6:
			EXEC SQL COPY TABLE dd_last_number ()
				INTO	:filenames[i];
		}
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			IIUGerr(E_RM010C_Err_copy_out, UG_ERR_ERROR, 1,
				cfgtbls[i].table_name);
			remove_temp();
			return (E_RM010C_Err_copy_out);
		}
	}

	EXEC SQL SELECT DBMSINFO('dba') INTO :source_dba;

	EXEC SQL CONNECT :full_dbname SESSION :other_session
		OPTIONS = '-l';
	if (RPdb_error_check(0, &errinfo))
	{
		IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1,
			full_dbname);
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (E_RM0084_Error_connecting);
	}

	/* See if user is the DBA; if not, reconnect as DBA. */
	EXEC SQL SELECT DBMSINFO('dba') INTO :target_dba;
	EXEC SQL SELECT DBMSINFO('username') INTO :username;
	if (!STequal(target_dba, username))
	{
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL CONNECT :full_dbname SESSION :other_session
			IDENTIFIED BY :target_dba OPTIONS = '-l';
		if (RPdb_error_check(0, &errinfo))
		{
			IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1,
				full_dbname);
			EXEC SQL SET_SQL (SESSION = :old_session);
			remove_temp();
			return (E_RM0084_Error_connecting);
		}
	}

	/* check that Replicator catalogs exist and are up to date */
	if (!RMcheck_cat_level())
	{
		IIUGerr(E_RM0003_No_rep_catalogs, UG_ERR_ERROR, 0);
		EXEC SQL ROLLBACK;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (E_RM0003_No_rep_catalogs);
	}

	/* Turn off replication */
	EXEC SQL SET TRACE POINT DM102;

	EXEC SQL SET_SQL (ERRORTYPE = 'genericerror');

	/* drop old support objects */

	if ((err = drop_supp_objs()) != OK)
	{
		EXEC SQL ROLLBACK;
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (FAIL);
	}

	/* loop to move configuration data */
	for (i = 0; i < DIM(cfgtbls); ++i)
	{
		/* delete existing rows */
		switch (i)
		{
		case 0:
			EXEC SQL DELETE FROM dd_databases;
			break;

		case 1:
			EXEC SQL DELETE FROM dd_regist_tables;
			break;

		case 2:
			EXEC SQL DELETE FROM dd_regist_columns;
			break;

		case 3:
			EXEC SQL DELETE FROM dd_cdds;
			break;

		case 4:
			EXEC SQL DELETE FROM dd_db_cdds;
			break;

		case 5:
			EXEC SQL DELETE FROM dd_paths;
			break;

		case 6:
			EXEC SQL DELETE FROM dd_last_number;
			break;
		}
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			IIUGerr(E_RM010D_Err_clear_cat, UG_ERR_ERROR, 1,
				cfgtbls[i].table_name);
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			remove_temp();
			return (E_RM010D_Err_clear_cat);
		}

		/* Copy data into cfg tables */
		switch (i)
		{
		case 0:
			EXEC SQL COPY TABLE dd_databases ()
				FROM	:filenames[i];
			break;

		case 1:
			EXEC SQL COPY TABLE dd_regist_tables ()
				FROM	:filenames[i];
			EXEC SQL UPDATE dd_regist_tables
				SET	supp_objs_created = '',
					rules_created = '';
			break;

		case 2:
			EXEC SQL COPY TABLE dd_regist_columns ()
				FROM	:filenames[i];
			break;

		case 3:
			EXEC SQL COPY TABLE dd_cdds ()
				FROM	:filenames[i];
			break;

		case 4:
			EXEC SQL COPY TABLE dd_db_cdds ()
				FROM	:filenames[i];
			break;

		case 5:
			EXEC SQL COPY TABLE dd_paths ()
				FROM	:filenames[i];
			break;

		case 6:
			EXEC SQL COPY TABLE dd_last_number ()
				FROM	:filenames[i];
			break;
		}
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			IIUGerr(E_RM010E_Err_copy_in, UG_ERR_ERROR, 1,
				cfgtbls[i].table_name);
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			remove_temp();
			return (E_RM010E_Err_copy_in);
		}

		/* Adjust ownership of DBA tables */
		if (i == 1)
		{
			EXEC SQL UPDATE dd_regist_tables
				SET	table_owner = :target_dba
				WHERE	table_owner = :source_dba;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
			{
				IIUGerr(E_RM010F_Err_adj_owner, UG_ERR_ERROR,
					0);
				EXEC SQL SET NOTRACE POINT DM102;
				EXEC SQL DISCONNECT SESSION :other_session;
				EXEC SQL SET_SQL (SESSION = :old_session);
				remove_temp();
				return (E_RM010F_Err_adj_owner);
			}
		}
	}

	/* Reset the localdb flag */
	EXEC SQL UPDATE dd_databases
		SET	local_db = 0
		WHERE	local_db = 1;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		IIUGerr(E_RM0110_Err_adj_localdb, UG_ERR_ERROR, 0);
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (E_RM0110_Err_adj_localdb);
	}

	EXEC SQL UPDATE dd_databases
		SET	local_db = 1
		WHERE	vnode_name = :vnode_name
		AND	database_name = :dbname;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		IIUGerr(E_RM0110_Err_adj_localdb, UG_ERR_ERROR, 0);
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (E_RM0110_Err_adj_localdb);
	}
	EXEC SQL COMMIT;

	EXEC SQL SELECT TRIM(dbms_type)
		INTO	:dbms_type
		FROM	dd_databases
		WHERE	local_db = 1;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		IIUGerr(E_RM0111_Err_rtrv_type, UG_ERR_ERROR, 0);
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		remove_temp();
		return (E_RM0111_Err_rtrv_type);
	}
	EXEC SQL COMMIT;

	/* if remote database is not Ingres, don't create support objects */
	if (!STequal(dbms_type, ERx("ingres")))
	{
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		EXEC SQL COMMIT;
		return (OK);
	}

	/* Retrieve list of tables from dd_regist_tables */
	EXEC SQL SET_SQL (session = :old_session);
	EXEC SQL DECLARE c1 CURSOR FOR
		SELECT	t.table_no
		FROM	dd_regist_tables t
		WHERE	t.columns_registered != '';

	EXEC SQL OPEN c1 FOR READONLY;
	if (RPdb_error_check(0, &errinfo))
	{
		IIUGerr(E_RM0112_Err_open_tbl_curs, UG_ERR_ERROR, 0);
		EXEC SQL CLOSE c1;
		EXEC SQL ROLLBACK;
		EXEC SQL SET_SQL (SESSION = :other_session);
		EXEC SQL SET NOTRACE POINT DM102;
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL SET_SQL (SESSION = :old_session);
		return (E_RM0112_Err_open_tbl_curs);
	}

	while (TRUE)
	{
		EXEC SQL FETCH c1 INTO :table_no;
		err = RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo);
		if (err)
		{
			IIUGerr(E_RM0113_Err_fetch_tbl_curs, UG_ERR_ERROR, 0);
			EXEC SQL SET_SQL (session = :other_session);
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			EXEC SQL CLOSE c1;
			EXEC SQL ROLLBACK;
			return (E_RM0113_Err_fetch_tbl_curs);
		}
		if (errinfo.rowcount == 0)
			break;

		EXEC SQL SET_SQL (SESSION = :other_session);
		err = RMtbl_fetch(table_no, TRUE);
		if (err)
		{
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			EXEC SQL CLOSE c1;
			EXEC SQL ROLLBACK;
			return (err);
		}

		if (RMtbl->target_type > 2)
		{
			EXEC SQL SET_SQL (SESSION = :old_session);
			continue;
		}

		err = create_support_tables(table_no);
		if (err && err != -1)
		{
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			EXEC SQL CLOSE c1;
			EXEC SQL ROLLBACK;
			return (err);
		}
		err = tbl_dbprocs(table_no);
		if (err && err != -1)
		{
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			EXEC SQL CLOSE c1;
			EXEC SQL ROLLBACK;
			return (err);
		}

		EXEC SQL UPDATE dd_regist_tables
			SET	supp_objs_created = DATE_GMT('now')
			WHERE	table_no = :table_no;
		if ((err = RPdb_error_check(DBEC_SINGLE_ROW, NULL)) != OK)
		{
			EXEC SQL SET NOTRACE POINT DM102;
			EXEC SQL DISCONNECT SESSION :other_session;
			EXEC SQL SET_SQL (SESSION = :old_session);
			EXEC SQL CLOSE c1;
			EXEC SQL ROLLBACK;
			return (err);
		}

		/* Return to local session */
		EXEC SQL SET_SQL (SESSION = :old_session);
	}

	EXEC SQL SET_SQL (SESSION = :other_session);
	/* Turn replication back on */
	EXEC SQL COMMIT;
	EXEC SQL SET NOTRACE POINT DM102;
	EXEC SQL DISCONNECT SESSION :other_session;
	EXEC SQL SET_SQL (SESSION = :old_session);
	EXEC SQL CLOSE c1;
	EXEC SQL COMMIT;
	return (OK);
}

/*{
** Name:	remove_temp - remove temporary files
**
** Description:
**      Removes temporary files
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**      none
**
** History:
**      Add a reload of locations structure from
*/
static void
remove_temp()
{
	i4	i;

	for (i = 0; i < DIM(cfgtbls); ++i)
        {
                LOfroms(PATH & FILENAME, filenames[i], &cfgtbls[i].loc);
		LOdelete(&cfgtbls[i].loc);
        }
}


/*{
** Name:	drop_supp_objs - drops support objects
**
** Description:
**	Drops support tables, procedures and rules.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
drop_supp_objs()
{
	i4	*tbl_nums;
	STATUS	err;
	i4	i;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	num_tables = 0;
	i4	table_no;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT COUNT(*)
		INTO	:num_tables
		FROM	dd_regist_tables;
	if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
		return (FAIL);

	if (num_tables == 0)
		return (OK);

	/* allocate memory for registered table numbers */
	tbl_nums = (i4 *)MEreqmem(0, (u_i4)num_tables * sizeof(i4), TRUE,
		NULL);
	if (tbl_nums == NULL)
		return (FAIL);

	/* get registered table numbers */
	i = 0;
	EXEC SQL SELECT	table_no
		INTO	:table_no
		FROM	dd_regist_tables;
	EXEC SQL BEGIN;
		if (i < num_tables)
			tbl_nums[i] = table_no;
		++i;
	EXEC SQL END;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);
	EXEC SQL COMMIT;

	for (i = 0; i < num_tables; ++i)
	{
		if ((err = dropsupp(tbl_nums[i])) != OK)
			return (err);

		EXEC SQL COMMIT;
	}

	MEfree((PTR)tbl_nums);

	return (OK);
}

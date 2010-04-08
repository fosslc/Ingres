/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <lo.h>
# include <si.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <targtype.h>
# include <tbldef.h>
# include "errm.h"

EXEC SQL INCLUDE sqlda;

/**
** Name:	tblinteg.sc - table integrity report
**
** Description:
**	Defines
**		table_integrity_report	- table integrity report
**		db_get_info		- get database information
**		db_connect		- connect to a database
**		compare_tables		- compare tables
**		end_table_integrity	- close table integrity report
**
** History:
**	09-jan-97 (joea)
**		Created based on tblinteg.sc in replicator library.
**	20-jan-97 (joea)
**		Use defined constants for target types. Add messageit function
**		as parameter to check_unique_keys.
**	13-nov-97 (joea) bug 87203
**		Use ROWCOUNT instead of ENDQUERY in the nested SELECTs.
**		Initialize 'done' before the second loop.  Simplify the empty
**		string tests.
**	12-feb-98 (joea) bug 87203
**		Correct the 13-nov-97 change.  Use ROWCOUNT instead of ENDQUERY
**		in the second nested SELECT.
**	13-may-98 (joea)
**		For blob columns, do a dummy comparison.
**	05-jun-98 (padni01) bug 88184
**		If local table owner is local DBA, assume target table owner 
**		to be target DBA else same as local table owner. Create
**		separate target_select_stmt, target_save_select_stmt and
**		target_base_select_stmt for processing target table.
**	18-jun-98 (abbjo03)
**		Center headings modified by 05-jun change.
**	11-sep-98 (abbjo03) bug 88184
**		Add table information and table owner arguments to print_cols.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Replace string literals
**		with ERget calls. Streamline redundant code by adding
**		db_get_info, db_connect and compare_tables.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-dec-2003 (gupsh01)
**	    Added support for nchar and nvarchar in table_integrity_report.
**      28-Feb-2007 (kibro01) b117788
**          Stop column being added twice if varchar or date
**      06-nov-2009 (joea)
**          Add test for "boolean" in table_integrity.
**/

# define MAX_SELECT_SIZE	(DB_MAX_COLS * (DB_MAXNAME + 10)) + 1000
# define NUM_LOCAL_VARS		3


GLOBALREF TBLDEF *RMtbl;


typedef void MSG_FUNC(i4 msg_level, i4  msg_num, ...);


FUNC_EXTERN void ddba_messageit(i4 msg_level, i4  msg_num, ...);
FUNC_EXTERN void print_cols(FILE *report_fp, TBLDEF *tbl, char *table_owner,
	i2 sourcedb, i4 transaction_id, i4 sequence_no, char *target_wc);
FUNC_EXTERN STATUS check_unique_keys(char *table_name, char *table_owner,
	i4 table_no, char *dbname, MSG_FUNC *msg_func);


static STATUS db_get_info(i2 db_no, i2 cdds_no, char *db_name);
static STATUS db_connect(i4 sess_no, char *db_name, char *user_name, char *dba);
static STATUS compare_tables(char *src_db, char *targ_db, char *src_owner,
	char *targ_owner, i4 src_sess, i4 targ_sess, IISQLDA *sqlda1,
	IISQLDA *sqlda2, char *src_select_stmt, char *targ_select_stmt,
	char *order_clause, i4  *in_src_not_targ, i4  *different);
static void end_table_integrity(IISQLDA *sqlda1, IISQLDA *sqlda2);


static FILE *report_fp;
static i4	column_count;
static i2	database_no1;
static i4	transaction_id1;
static i4	sequence_no1;
static i2	database_no2;
static i4	transaction_id2;
static i4	sequence_no2;

EXEC SQL BEGIN DECLARE SECTION;
static i4	start_session;
static i4	Session1 = 99;
static i4	Session2 = 98;
EXEC SQL END DECLARE SECTION;


/*{
** Name:	table_integrity_report - table integrity report
**
** Description:
**	Compares a table in 2 databases with the specified routing
**	in the specified time period (of replication) and reports on
**	discrepancies.
**
** Inputs:
**	db1		- first database number
**	db2		- second database number
**	table_no	- table number
**	cdds_no		- CDDS number
**	begin_time	- begin date/time
**	end_time	- end date/time
**	order_clause	- order clause
**	filename	- report file name
**
** Outputs:
**	none
**
** Returns:
**	0 - OK
**	otherwise error
*/
STATUS
table_integrity(
i2	db1,
i2	db2,
i4	table_no,
i2	cdds_no,
char	*begin_time,
char	*end_time,
char	*order_clause,
char	*filename)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	localowner[DB_MAXNAME+1];
	char	targetowner[DB_MAXNAME+1];
	char	target_table_owner[DB_MAXNAME+1];
	char	username[DB_MAXNAME+1];
	char	select_clause[MAX_SELECT_SIZE];
	char	from_clause[256];
	char	select_stmt[MAX_SELECT_SIZE];
	char	target_select_stmt[MAX_SELECT_SIZE];
	char	database1[DB_MAXNAME*2+3];
	char	database2[DB_MAXNAME*2+3];
	char	tmp[257];
	char	base_where_clause[1024];
	char	where_clause[1024];
	short	null_ind[DB_MAX_COLS];
	short	null_ind2[DB_MAX_COLS];
	char	report_filename[MAX_LOC+1];
	EXEC SQL END DECLARE SECTION;
	COLDEF	*col_p;
	LOCATION	loc;
	i4	in_1_not_2 = 0;
	i4	in_2_not_1 = 0;
	i4	different = 0;
	i4	no_keys = 1;	/* flag for key columns found */
	u_i4	i;
	IISQLDA	_sqlda1, _sqlda2;
	IISQLDA	*sqlda1 = &_sqlda1;
	IISQLDA	*sqlda2 = &_sqlda2;
	char		*p;
	DBEC_ERR_INFO	errinfo;
	STATUS		status;
	i4		db_num;

	status = RMtbl_fetch(table_no, FALSE);
	if (status != OK)
		return (status);

	STcopy(filename, report_filename);
	LOfroms(PATH & FILENAME, report_filename, &loc);
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &report_fp) != OK)
	{
		IIUGerr(E_RM0087_Err_open_report, UG_ERR_ERROR, 1,
			report_filename);
		return (E_RM0087_Err_open_report);
	}

	EXEC SQL INQUIRE_SQL (:start_session = SESSION);
	status = db_get_info(db1, cdds_no, database1);
	if (status != OK)
	{
		SIclose(report_fp);
		return (status);
	}

	status = db_get_info(db2, cdds_no, database2);
	if (status != OK)
	{
		SIclose(report_fp);
		return (status);
	}

	EXEC SQL SELECT DBMSINFO('username') INTO :username;
	status = db_connect(Session1, database1, username, localowner);
	if (status != OK)
	{
		SIclose(report_fp);
		return (status);
	}

	status = db_connect(Session2, database2, username, targetowner);
	if (status != OK)
	{
		EXEC SQL SET_SQL (session = :Session1);
		EXEC SQL DISCONNECT SESSION :Session1;
		EXEC SQL SET_SQL (session = :start_session);
		SIclose(report_fp);
		return (status);
	}

	if (STequal(RMtbl->table_owner, localowner))
		STcopy(targetowner, target_table_owner);
	else
		STcopy(RMtbl->table_owner, target_table_owner);
	p = ERget(F_RM00A9_Prod_name);
	i = 40 + STlength(p) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, p);
	p = ERget(F_RM00C9_Tbl_integ_rpt);
	i = 40 + STlength(p) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, p);
	p = ERget(F_RM00CA_For_table);
	STprintf(tmp, p, RMtbl->table_owner, RMtbl->table_name, database1);
	i = 40 + STlength(tmp) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, tmp);
	p = ERget(F_RM00CB_And_table);
	STprintf(tmp, p, target_table_owner, RMtbl->table_name, database2);
	i = 40 + STlength(tmp) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, tmp);

	EXEC SQL SET_SQL (session = :Session1);
	if (check_unique_keys(RMtbl->table_name, target_table_owner,
			RMtbl->table_no, database1, ddba_messageit))
		SIfprintf(report_fp, ERget(E_RM00CC_Keys_not_unique),
			database1);
	EXEC SQL SET_SQL (session = :Session2);
	if (check_unique_keys(RMtbl->table_name, RMtbl->table_owner,
			RMtbl->table_no, database2, ddba_messageit))
		SIfprintf(report_fp, ERget(E_RM00CC_Keys_not_unique),
			database2);

	EXEC SQL SET_SQL (session = :Session1);
	STprintf(base_where_clause, ERx("WHERE cdds_no = %d "), (i4)cdds_no);
	STtrmwhite(begin_time);
	STtrmwhite(end_time);
	if (*begin_time != EOS)
	{
		STprintf(tmp, ERx(" AND trans_time >= DATE('%s') "),
			begin_time);
		STcat(base_where_clause, tmp);
	}
	if (*end_time != EOS)
	{
		STprintf(tmp, ERx(" AND trans_time <= DATE('%s') "), end_time);
		STcat(base_where_clause, tmp);
	}

	STprintf(select_clause,
		ERx("SELECT sourcedb, transaction_id, sequence_no"));
	sqlda1->sqln = sqlda2->sqln = 0;
	column_count = NUM_LOCAL_VARS;
	sqlda1->sqlvar[0].sqldata = (char *)&database_no1;
	sqlda1->sqlvar[0].sqlind = &null_ind[0];
	sqlda2->sqlvar[0].sqldata = (char *)&database_no2;
	sqlda2->sqlvar[0].sqlind = &null_ind2[0];
	sqlda1->sqlvar[1].sqldata = (char *)&transaction_id1;
	sqlda1->sqlvar[1].sqlind = &null_ind[1];
	sqlda2->sqlvar[1].sqldata = (char *)&transaction_id2;
	sqlda2->sqlvar[1].sqlind = &null_ind2[1];
	sqlda1->sqlvar[2].sqldata = (char *)&sequence_no1;
	sqlda1->sqlvar[2].sqlind = &null_ind[2];
	sqlda2->sqlvar[2].sqldata = (char *)&sequence_no2;
	sqlda2->sqlvar[2].sqlind = &null_ind2[2];
	for (col_p = RMtbl->col_p; column_count < RMtbl->ncols + NUM_LOCAL_VARS;
		column_count++, *col_p++)
	{
		if (STequal(col_p->column_datatype, ERx("varchar")) ||
			STequal(col_p->column_datatype, ERx("date")) ||
			STequal(col_p->column_datatype, ERx("ingresdate")) ||
			STequal(col_p->column_datatype, ERx("time without time zone")) ||
			STequal(col_p->column_datatype, ERx("time with time zone")) ||
			STequal(col_p->column_datatype, ERx("time with local time zone")) ||
			STequal(col_p->column_datatype, ERx("timestamp without time zone")) ||
			STequal(col_p->column_datatype, ERx("timestamp with time zone")) ||
			STequal(col_p->column_datatype, ERx("timestamp with local time zone")) ||
			STequal(col_p->column_datatype, ERx("interval year to month")) ||
			STequal(col_p->column_datatype, ERx("interval day to second")) ||
			STequal(col_p->column_datatype, ERx("ansidate")))
		{
			STprintf(tmp, ERx(", CHAR(t.%s)"),
				col_p->dlm_column_name);
			STcat(select_clause, tmp);
		}
		else if (STequal(col_p->column_datatype, ERx("nvarchar")) ||
			STequal(col_p->column_datatype, ERx("nchar")))
		{
			STprintf(tmp, ERx(", NVARCHAR(t.%s)"),
				col_p->dlm_column_name);
			STcat(select_clause, tmp);
		}
		else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long byte")))
		{
			p = ERget(F_RM00CC_Unbounded_datatype);
			i = STlength(p);
			STcat(select_clause, ERx(", CHAR('"));
			STcat(select_clause, p);
			STcat(select_clause, ERx("')"));
		}
		else
		{
			STcat(select_clause, ERx(", t."));
			STcat(select_clause, col_p->dlm_column_name);
		}
		if (STequal(col_p->column_datatype, ERx("ingresdate")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 26, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 26, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("ansidate")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 18, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 18, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("time without time zone")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 22, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 22, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("time with time zone")) ||
		     STequal(col_p->column_datatype, ERx("time with local time zone")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 32, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 32, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("timestamp without time zone")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 40, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 40, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("timestamp with time zone")) ||
		     STequal(col_p->column_datatype, ERx("timestamp with local time zone")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 50, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 50, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("interval year to month")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 16, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 16, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("interval day to second")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 46, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 46, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long byte")))
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, i, TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, i, TRUE, (STATUS *)NULL);
		}
		else if (STequal(col_p->column_datatype, "boolean"))
		{
		    sqlda1->sqlvar[column_count].sqldata = (char *)
		        MEreqmem(0, sizeof("FALSE"), TRUE, NULL);
		    sqlda2->sqlvar[column_count].sqldata = (char *)
		        MEreqmem(0, sizeof("FALSE"), TRUE, NULL);
		}
		else
		{
			sqlda1->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 4 + col_p->column_length,
				TRUE, (STATUS *)NULL);
			sqlda2->sqlvar[column_count].sqldata =
				(char *)MEreqmem(0, 4 + col_p->column_length,
				TRUE, (STATUS *)NULL);
		}
		sqlda1->sqlvar[column_count].sqlind = &null_ind[column_count];
		sqlda2->sqlvar[column_count].sqlind = &null_ind2[column_count];
		if (col_p->key_sequence > 0)
		{
			no_keys = 0;
			STprintf(tmp, ERx(" AND t.%s = s.%s"),
				col_p->dlm_column_name, col_p->dlm_column_name);
			STcat(base_where_clause, tmp);
		}
	}
	if (column_count == NUM_LOCAL_VARS)
	{
		IIUGerr(E_RM00CD_No_replic_cols, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		end_table_integrity(sqlda1, sqlda2);
		return (E_RM00CD_No_replic_cols);
	}
	if (no_keys)
	{
		IIUGerr(E_RM00CE_No_key_cols, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		end_table_integrity(sqlda1, sqlda2);
		return (E_RM00CE_No_key_cols);
	}
	sqlda1->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 26, TRUE,
		(STATUS *)NULL);
	sqlda1->sqlvar[column_count].sqlind = &null_ind[column_count];
	sqlda2->sqlvar[column_count].sqldata = (char *)MEreqmem(0, 26, TRUE,
		(STATUS *)NULL);
	sqlda2->sqlvar[column_count].sqlind = &null_ind2[column_count];
	sqlda1->sqln = sqlda2->sqln = column_count + 1;
	STcat(select_clause, ERx(", CHAR(s.trans_time) "));

	STcopy(select_clause, select_stmt);
	STprintf(from_clause, ERx("FROM %s s, %s.%s t "), RMtbl->sha_name,
		RMtbl->dlm_table_owner, RMtbl->dlm_table_name);
	STcat(select_stmt, from_clause);
	STcat(select_stmt, base_where_clause);
	STcopy(select_clause, target_select_stmt);
	STprintf(from_clause, ERx("FROM %s s, %s.%s t "),
		RMtbl->sha_name, target_table_owner, RMtbl->dlm_table_name);
	STcat(target_select_stmt, from_clause);
	STcat(target_select_stmt, base_where_clause);

	status = compare_tables(database1, database2, RMtbl->table_owner,
		target_table_owner, Session1, Session2, sqlda1, sqlda2,
		select_stmt, target_select_stmt, order_clause, &in_1_not_2,
		&different);
	if (status != OK)
	{
		end_table_integrity(sqlda1, sqlda2);
		return (status);
	}
	/* find ones missing from db # 2 */
	EXEC SQL SET_SQL (session = :Session2);
	status = compare_tables(database2, database1, target_table_owner,
		RMtbl->table_owner, Session2, Session1, sqlda1, sqlda2,
		target_select_stmt, select_stmt, order_clause, &in_2_not_1,
		NULL);
	if (status != OK)
	{
		end_table_integrity(sqlda1, sqlda2);
		return (status);
	}

	if (in_1_not_2 + in_2_not_1 + different == 0)
		SIfprintf(report_fp, ERx("\n\n%s\n"), ERget(F_RM00CE_No_diffs));

	end_table_integrity(sqlda1, sqlda2);

	return (OK);
}


/*{
** Name:	db_get_info - get database information
**
** Description:
**	Retrieve database information from catalogs. Also check the CDDS is
**	FP or PRO.
**
** Inputs:
**	db_no	- database number
**	cdds_no	- CDDS number
**
** Outputs:
**	db_name	- vnode and database name
**
** Returns:
**	OK	- no error
*/
static STATUS
db_get_info(
i2	db_no,
i2	cdds_no,
char	*db_name)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	db_no;
i2	cdds_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	dbname[DB_MAXNAME+1];
	char	vnode[DB_MAXNAME+1];
	i2	target_type;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;
	i4		db_num = (i4)db_no;
	i4		cdds_num;

	*dbname = *vnode = EOS;
	EXEC SQL SELECT TRIM(database_name), TRIM(vnode_name)
		INTO	:dbname, :vnode
		FROM	dd_databases
		WHERE	database_no = :db_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		if (errinfo.errorno)
		{
			IIUGerr(E_RM00C8_Err_rtrv_db_name, UG_ERR_ERROR, 1,
				&db_num);
			return (E_RM00C8_Err_rtrv_db_name);
		}
		else	/* rowcount == 0 */
		{
			IIUGerr(E_RM00C9_Db_not_found, UG_ERR_ERROR, 1,
				&db_num);
			return (E_RM00C9_Db_not_found);
		}
	}
	if (*vnode == EOS)
		STprintf(db_name, ERx("%s"), dbname);
	else
		STprintf(db_name, ERx("%s::%s"), vnode, dbname);

	cdds_num = (i4)cdds_no;
	EXEC SQL SELECT target_type
		INTO	:target_type
		FROM	dd_db_cdds
		WHERE	database_no = :db_no
		AND	cdds_no = :cdds_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		IIUGerr(E_RM00CA_Err_rtrv_targ_type, UG_ERR_ERROR, 2, &cdds_num,
			&db_num);
		return (E_RM00CA_Err_rtrv_targ_type);
	}
	if (target_type != TARG_FULL_PEER && target_type != TARG_PROT_READ)
	{
		IIUGerr(E_RM00CB_No_URO_integ_rpt, UG_ERR_ERROR, 2, db_name,
			&cdds_num);
		return (E_RM00CB_No_URO_integ_rpt);
	}
	return (OK);
}


/*{
** Name:	db_connect - connect to a database
**
** Description:
**	Connect to a database and check that the Replicator catalogs exist.
**
** Inputs:
**	sess_no		- session number to use
**	db_name		- database name
**	user_name	- user name
**
** Outputs:
**	dba		- DBA name
**
** Returns:
**	OK	- no error
*/
static STATUS
db_connect(
i4	sess_no,
char	*db_name,
char	*user_name,
char	*dba)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	sess_no;
char	*db_name;
char	*user_name;
char	*dba;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	cnt;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL CONNECT :db_name SESSION :sess_no IDENTIFIED BY :user_name;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1, db_name);
		return (E_RM0084_Error_connecting);
	}

	/* See if user is the DBA; if not, reconnect as DBA. */
	EXEC SQL SELECT DBMSINFO('dba') INTO :dba;
	if (!STequal(dba, user_name))
	{
		EXEC SQL DISCONNECT SESSION :sess_no;
		EXEC SQL CONNECT :db_name SESSION :sess_no
			IDENTIFIED BY :dba;
		if (RPdb_error_check(0, NULL) != OK)
		{
			IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1,
				db_name);
			return (E_RM0084_Error_connecting);
		}
	}

	EXEC SQL SELECT COUNT(*)
		INTO	:cnt
		FROM	iitables
		WHERE	LOWERCASE(table_name) = 'dd_databases'
		AND	table_owner = :dba;
	if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
	{
		IIUGerr(E_RM0092_Err_ck_catalogs, UG_ERR_ERROR, 1, db_name);
		EXEC SQL DISCONNECT SESSION :sess_no;
		return (E_RM0092_Err_ck_catalogs);
	}
	else if (!cnt)
	{
		IIUGerr(E_RM0093_Miss_rep_catalogs, UG_ERR_ERROR, 1, db_name);
		EXEC SQL DISCONNECT SESSION :sess_no;
		return (E_RM0093_Miss_rep_catalogs);
	}

	return (OK);
}


/*{
** Name:	compare_tables - compare tables
**
** Description:
**	Compare rows in source database to rows in target database.
**
** Inputs:
**	src_db			- source vnode and database name
**	targ_db			- target vnode and database name
**	src_owner		- source table owner
**	targ_owner		- target table owner
**	src_sess		- source session number
**	targ_sess		- target session number
**	sqlda1			- first SQLDA
**	sqlda2			- second SQLDA
**	src_select_stmt		- source SELECT statement
**	targ_select_stmt	- target SELECT statement
**	order_clause		- ORDER BY clause
**
** Outputs:
**	in_src_not_targ		- row in source, not in target counter
**	different		- different rows counter
**
** Returns:
**	OK - no error
*/
static STATUS
compare_tables(
char	*src_db,
char	*targ_db,
char	*src_owner,
char	*targ_owner,
i4	src_sess,
i4	targ_sess,
IISQLDA	*sqlda1,
IISQLDA	*sqlda2,
char	*src_select_stmt,
char	*targ_select_stmt,
char	*order_clause,
i4	*in_src_not_targ,
i4	*different)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	src_sess;
i4	targ_sess;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	done = FALSE;
	char	where_clause[1024];
	char	select_stmt[MAX_SELECT_SIZE];
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;
	i4	i;
	i4	db_num;

	STcopy(src_select_stmt, select_stmt);
	if (*order_clause != EOS)
	{
		STcat(select_stmt, ERx(" ORDER BY "));
		STcat(select_stmt, order_clause);
	}
	EXEC SQL PREPARE s1 FROM :select_stmt;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM00CF_Prep_failed, UG_ERR_ERROR, 3, src_owner,
			RMtbl->table_name, src_db);
		return (E_RM00CF_Prep_failed);
	}
	EXEC SQL DESCRIBE s1 INTO :sqlda1;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM00D0_Descr_failed, UG_ERR_ERROR, 3, src_owner,
			RMtbl->table_name, src_db);
		return (E_RM00D0_Descr_failed);
	}
	EXEC SQL DECLARE curs CURSOR FOR s1;
	EXEC SQL OPEN curs FOR READONLY;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM00D1_Err_open_integ_curs, UG_ERR_ERROR, 1, src_db);
		return (E_RM00D1_Err_open_integ_curs);
	}

	IIUGmsg(ERget(F_RM00CD_Comparing), FALSE, 2, src_db, targ_db);
	done = FALSE;
	while (!done)
	{
		EXEC SQL FETCH curs USING DESCRIPTOR :sqlda1;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		{
			IIUGerr(E_RM00D2_Err_fetch_integ_curs, UG_ERR_ERROR, 1,
				src_db);
			return (E_RM00D2_Err_fetch_integ_curs);
		}
		if (!done)
		{
			EXEC SQL SET_SQL (session = :targ_sess);
			STprintf(where_clause, ERx(" AND sourcedb = %d AND \
transaction_id = %d AND sequence_no = %d"),
				database_no1, transaction_id1, sequence_no1);
			STcopy(targ_select_stmt, select_stmt);
			STcat(select_stmt, where_clause);
			EXEC SQL PREPARE s2 FROM :select_stmt;
			if (RPdb_error_check(0, NULL) != OK)
			{
				IIUGerr(E_RM00CF_Prep_failed, UG_ERR_ERROR, 3,
					targ_owner, RMtbl->table_name, targ_db);
				return (E_RM00CF_Prep_failed);
			}
			EXEC SQL DESCRIBE s2 INTO :sqlda2;
			if (RPdb_error_check(0, NULL) != OK)
			{
				IIUGerr(E_RM00D0_Descr_failed, UG_ERR_ERROR, 3,
					targ_owner, RMtbl->table_name, targ_db);
				return (E_RM00D0_Descr_failed);
			}
			EXEC SQL EXECUTE IMMEDIATE :select_stmt USING :sqlda2;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
			{
				IIUGerr(E_RM00D3_Err_exec_immed, UG_ERR_ERROR,
					1, targ_db);
				return (E_RM00D3_Err_exec_immed);
			}
			db_num = (i4)database_no1;
			if (!errinfo.rowcount)
			{
				++*in_src_not_targ;
				SIfprintf(report_fp,
					ERget(E_RM00D4_In_dbx_not_in_dby),
					src_db, targ_db);
				SIfprintf(report_fp,
					ERget(E_RM00D5_Src_txn_seq),
					db_num, transaction_id1, sequence_no1);
				EXEC SQL SET_SQL (session = :src_sess);
				print_cols(report_fp, RMtbl, src_owner,
					database_no1, transaction_id1,
					sequence_no1, (char *)NULL);
			}
			else if (different)
			{
				for (i = 0; i < column_count; i++)
				{
					if (sqlda1->sqlvar[i].sqllen !=
						sqlda2->sqlvar[i].sqllen)
					{
						IIUGerr(E_RM00D6_Tbl_mismatch,
							UG_ERR_ERROR, 2, src_db,
							targ_db);
						return (E_RM00D6_Tbl_mismatch);
					}
					if (MEcmp(sqlda1->sqlvar[i].sqldata,
						sqlda2->sqlvar[i].sqldata,
						sqlda1->sqlvar[i].sqllen) != 0)
					{
						++*different;
						SIfprintf(report_fp,
							ERget(E_RM00D7_Row_diff_dbx_vs_dby),
							src_db, targ_db);
						SIfprintf(report_fp,
							ERget(E_RM00D5_Src_txn_seq),
							db_num, transaction_id1,
							sequence_no1);
						EXEC SQL SET_SQL (session =
							:src_sess);
						print_cols(report_fp, RMtbl,
							src_owner, database_no1,
							transaction_id1,
							sequence_no1,
							(char *)NULL);
						EXEC SQL SET_SQL (session =
							:targ_sess);
						print_cols(report_fp, RMtbl,
							targ_owner,
							database_no1,
							transaction_id1,
							sequence_no1,
							(char *)NULL);
					}
				}
			}
		}
		EXEC SQL SET_SQL (session = :src_sess);
	}
	EXEC SQL CLOSE curs;

	return (OK);
}


/*{
** Name:	end_table_integrity - close table integrity report
**
** Description:
**	Clean up & disconnect at the end of the table_integrity report.
**
** Inputs:
**	sqlda1 -
**	sqlda2 -
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
end_table_integrity(
IISQLDA	*sqlda1,
IISQLDA	*sqlda2)
{
	i4	i;

	SIclose(report_fp);
	for (i = NUM_LOCAL_VARS; i < sqlda1->sqln; ++i)
		MEfree(sqlda1->sqlvar[i].sqldata);
	for (i = NUM_LOCAL_VARS; i < sqlda2->sqln; ++i)
		MEfree(sqlda2->sqlvar[i].sqldata);
	EXEC SQL SET_SQL (session = :Session1);
	EXEC SQL DISCONNECT SESSION :Session1;
	EXEC SQL SET_SQL (session = :Session2);
	EXEC SQL DISCONNECT SESSION :Session2;
	EXEC SQL SET_SQL (session = :start_session);
}

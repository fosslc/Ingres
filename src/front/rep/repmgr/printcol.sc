/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <si.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <adf.h>
# include <rpu.h>
# include <tbldef.h>
# include "errm.h"

/**
** Name:	printcol.sc - print columns
**
** Description:
**	Defines
**		print_cols	- print column names and values
**
** History:
**	09-jan-97 (joea)
**		Created based on prntcols.sc in replicator library.
**	13-may-98 (joea)
**		For LONG VARCHAR and LONG BYTE, print out **Unbounded Data
**		Type** as other character mode front-end tools.
**	09-jun-98 (abbjo03)
**		Remove redundant and incorrect "else if" from 13-may change.
**	11-sep-98 (abbjo03) bug 88184
**		Add table owner argument since the target table may be owned by
**		a differently named user (DBA) than the local user. Add table
**		information argument to limit dependency on global.
**	25-sep-98 (abbjo03)
**		Replace string literals with IIUGerr or ERget calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-dec-2003 (gupsh01)
**	    Added support in print_cols for long nvarchar datatypes. Note The 
**	    nchar and nvarchar unicode datatypes are supported through 
**	    coercion to CHAR types.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**      06-nov-2009 (joea)
**          Add test for "boolean" in print_cols.
**/

GLOBALREF ADF_CB	*RMadf_cb;


/*{
** Name:	print_cols - print column names and values
**
** Description:
**	Print (to file pointer report_fp) the column names and values for the
**	specified entry in the specified table.  Used by at least 2 reports.
**
**	Constructs a where clause with which to identify a set of columns
**	associated with a given replication key.  Format and print to
**	the file handle addressed by report_fp.
**
** Inputs:
**	report_fp	- file handler for output file
**	tbl		- table information
**	table_owner	- table owner
**	sourcedb	- integer database_no
**	transaction_id	- integer transaction_id
**	sequence_no	- integer sequence number within a transaction
**
** Outputs:
**	None
**
** Returns:
**	None
*/
void
print_cols(
FILE	*report_fp,
TBLDEF	*tbl,
char	*table_owner,
i2	sourcedb,
i4	transaction_id,
i4	sequence_no,
char	*target_wc)
{
	DBEC_ERR_INFO errinfo;
	COLDEF	*col_p;
	i4	column_number;
	EXEC SQL BEGIN DECLARE SECTION;
	char	errbuff[256];
	char	tmp[256];
	char	stmt[1024];
	char	tmp_char[256];
	i4	tmp_int = 0;
	f8	tmp_float;
	char	column_val[32];
	char	where_clause[1024];
	short	null_ind;
	EXEC SQL END DECLARE SECTION;

	/* Build a where clause */
	*where_clause = EOS;
	for (column_number = 0, col_p = tbl->col_p;
		column_number < tbl->ncols; column_number++, *col_p++)
	{
		if (col_p->key_sequence > 0)	/* All key columns */
		{
			STprintf(tmp, ERx(" AND t.%s = s.%s "),
				col_p->dlm_column_name, col_p->dlm_column_name);
			STcat(where_clause, tmp);
		}
	}

	for (column_number = 0, col_p = tbl->col_p;
		column_number < tbl->ncols; column_number++, *col_p++)
	{
		if (STequal(col_p->column_datatype, ERx("integer")))
		{
			if (target_wc == NULL)
			{
				STprintf(stmt,
					ERx("SELECT INT4(t.%s) FROM %s.%s t, \
%s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, tbl->sha_name,
					sourcedb, transaction_id, sequence_no);
				STcat(stmt, where_clause);
			}
			else
			{
				STprintf(stmt, ERx("SELECT INT4(t.%s)\
 FROM %s.%s t WHERE %s"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :stmt
				INTO	:tmp_int:null_ind;
			STprintf(column_val, ERx("%d"), tmp_int);
		}
		else if (STequal(col_p->column_datatype, ERx("float"))  ||
			STequal(col_p->column_datatype, ERx("money")))
		{
			if (target_wc == NULL)
			{
				STprintf(stmt,
					ERx("SELECT FLOAT8(t.%s) FROM %s.%s t,\
 %s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, tbl->sha_name,
					sourcedb, transaction_id, sequence_no);
				STcat(stmt, where_clause);
			}
			else
			{
				STprintf(stmt,
					ERx("SELECT FLOAT8(t.%s) FROM %s.%s t \
WHERE %s"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :stmt
				INTO	:tmp_float:null_ind;
			STprintf(column_val, ERx("%.13g"), tmp_float,
				(char)RMadf_cb->adf_decimal.db_decimal);
		}
		else if (STequal(col_p->column_datatype, ERx("long varchar")) ||
			STequal(col_p->column_datatype, ERx("long nvarchar")) ||
			STequal(col_p->column_datatype, ERx("long byte")))
		{
			STcopy(ERget(F_RM00CC_Unbounded_datatype), column_val);
			errinfo.rowcount = 1;
		}
                else if (STequal(col_p->column_datatype, "boolean"))
                {
                    if (target_wc == NULL)
                    {
                        STprintf(stmt, "SELECT TRIM(CHAR(t.%s, 5)) FROM %s.%s\
 t, %s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d",
                                 col_p->dlm_column_name, table_owner,
                                 tbl->dlm_table_name, tbl->sha_name,
                                 sourcedb, transaction_id, sequence_no);
                        STcat(stmt, where_clause);
                    }
                    else
                    {
                        STprintf(stmt, "SELECT TRIM(CHAR(t.%s, 5)) FROM %s.%s t\
 WHERE %s",
                                 col_p->dlm_column_name, table_owner,
                                 tbl->dlm_table_name, target_wc);
                    }
                    EXEC SQL EXECUTE IMMEDIATE :stmt
                        INTO :tmp_char:null_ind;
                    STprintf(column_val, "%s", tmp_char);
                }
		else
		{
			if (target_wc == NULL)
			{
				STprintf(stmt,
					ERx("SELECT TRIM(CHAR(t.%s)) FROM %s.%s\
 t, %s s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, tbl->sha_name,
					sourcedb, transaction_id, sequence_no);
				STcat(stmt, where_clause);
			}
			else
			{
				STprintf(stmt, ERx("SELECT \
TRIM(CHAR(t.%s)) FROM %s.%s t WHERE %s"),
					col_p->dlm_column_name, table_owner,
					tbl->dlm_table_name, target_wc);
			}
			EXEC SQL EXECUTE IMMEDIATE :stmt
				INTO	:tmp_char:null_ind;
			STprintf(column_val, ERx("%s"), tmp_char);
		}
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			if (errinfo.errorno)
			{
				IIUGerr(E_RM00E9_Err_rtrv_col_info,
					UG_ERR_ERROR, 3, col_p->column_name,
					table_owner, tbl->table_name);
				return;
			}
		}
		if (errinfo.rowcount > 0)
		{
			if (null_ind == -1)
				STcopy(ERx("NULL"), column_val);
			SIfprintf(report_fp, ERx("     %s%s:  %s\n"),
				col_p->key_sequence > 0 ? ERx("*") : ERx(" "),
				col_p->column_name, column_val);
		}
		else
		{
			SIfprintf(report_fp, ERget(E_RM00EA_Col_info_miss),
				table_owner, tbl->table_name);
			return;
		}
	}
}

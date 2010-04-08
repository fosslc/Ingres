/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <adf.h>
# include <rpu.h>
# include <wchar.h>

/**
** Name:	crkeycls.sc - create key clauses
**
** Description:
**	Defines
**		create_key_clauses	- create key clauses
**		apostrophe		- double apostrophes in a string
**
** History:
**	30-dec-96 (joea)
**		Created based on crkeycls.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to create_key_clauses.
**	23-jul-98 (abbjo03)
**		Replace DLMNAME_LENGTH by formula using DB_MAXNAME. Replace
**		IISQ_MAX_COLS by DB_MAX_COLS.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	21-jul-99 (abbjo03)
**		Replace formula for maximum length of a delimited identifier by
**		DB_MAX_DELIMID.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-dec-2003 (gupsh01)
**	    Added support for nchar and nvarchar datatypes.
**      06-feb-2009 (stial01)
**          Use DB_IITYPE_LEN for column_datatype (iitypename() result)
**      06-nov-2009 (joea)
**          Add test for "boolean" in create_key_clauses.
**/

typedef void MSG_FUNC(i4 msg_level, i4  msg_num, ...);


static char	*apos = ERx("\'");
static char	decimal_char = EOS;


static void apostrophe(char *string);


FUNC_EXTERN ADF_CB	*FEadfcb(void);


/*{
** Name:	create_key_clauses - create key clauses
**
** Description:
**	Creates clauses of SQL statements for a given transaction (for
**	instance, builds the where clause using the data in the base table that
**	corresponds to the replication key).
**
** Inputs:
**	table_name	-
**	table_owner	-
**	table_no	-
**	sha_name	-
**	database_no	-
**	transaction_id	-
**	sequence_no	-
**	msg_func	- function to display messages
**
** Outputs:
**	where_clause	-
**	name_clause	-
**	insert_value_clause -
**	update_clause	-
**
** Returns:
**	OK or ?
**/
STATUS
create_key_clauses(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
char	*update_clause,
char	*table_name,
char	*table_owner,
i4	table_no,
char	*sha_name,
i2	database_no,
i4	transaction_id,
i4	sequence_no,
MSG_FUNC	*msg_func)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
char	*table_owner;
i4	table_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	i4	column_count;
	i4	i;
	char	dlm_column_name[DB_MAX_DELIMID+1];
	char	column_value[DB_MAX_COLS][32];
	EXEC SQL BEGIN DECLARE SECTION;
	char	column_name[DB_MAX_COLS][DB_MAXNAME+1];
	char	column_datatype[DB_MAX_COLS][DB_IITYPE_LEN];
	i4	column_length[DB_MAX_COLS];
	i4	err;
	i4	rows;
	char	tmp_statement[256];
	i4	tmp_int;
	f8	tmp_float;
	VARCHAR struct
	{
		short	tmp_len;
		char	tmp_char[2048];
	} tmp_vch;
	NVARCHAR struct
	{
		short tmp_len;
		wchar_t tmp_nchar[1024];
	} tmp_nvch;
	short	null_ind;
	EXEC SQL END DECLARE SECTION;

	if (decimal_char == EOS)
		decimal_char = (char)FEadfcb()->adf_decimal.db_decimal;

	if (where_clause)
		*where_clause = EOS;
	if (name_clause)
		*name_clause = EOS;
	if (insert_value_clause)
		*insert_value_clause = EOS;
	if (update_clause)
		*update_clause = EOS;
	column_count = 0;
	EXEC SQL SELECT DISTINCT TRIM(d.column_name),
			TRIM(LOWERCASE(column_datatype)),
			column_length
		INTO	:column_name[column_count],
			:column_datatype[column_count],
			:column_length[column_count]
		FROM	dd_regist_columns d, iicolumns i
		WHERE	i.table_name = :table_name
		AND	i.table_owner = :table_owner
		AND	d.table_no = :table_no
		AND	d.key_sequence > 0
		AND	d.column_name = i.column_name;
	EXEC SQL BEGIN;
		column_count++;
	EXEC SQL END;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* Error %d getting columns for '%s.%s' */
		(*msg_func)(1, 1343, err, table_owner, table_name);
		return (err);
	}
	if (column_count == 0)
	{
		/* No registered columns for '%s.%s'. */
		(*msg_func)(1, 1344, table_owner, table_name);
		return (1344);
	}

	for (i = 0; i < column_count; i++)
	{
		RPedit_name(EDNM_DELIMIT, column_name[i], dlm_column_name);
		if (STequal(column_datatype[i], ERx("integer")))
		{
			STprintf(tmp_statement,
				ERx("SELECT INT4(%s) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_int:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)
			{
				/* integer execute immediate failed */
				(*msg_func)(1, 1349, err);
				return (err);
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
				STprintf(column_value[i], ERx("%d"), tmp_int);
		}
		else if (STequal(column_datatype[i], ERx("float")) ||
			STequal(column_datatype[i], ERx("money")))
		{
			STprintf(tmp_statement,
				ERx("SELECT FLOAT8(%s) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_float:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)
			{
				/* execute immediate failed */
				(*msg_func)(1, 1350, err);
				return (err);
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
				STprintf(column_value[i], ERx("%.13g"),
					tmp_float, decimal_char);
		}
		else if (STequal(column_datatype[i], ERx("nvarchar")) ||
			STequal(column_datatype[i], ERx("nchar")))
		{
			STprintf(tmp_statement,
				ERx("SELECT NVARCHAR(MAX(%s)) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_nvch:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)
			{
				/* char execute immediate failed */
				(*msg_func)(1, 1351, err);
				return (err);
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
			{
				tmp_nvch.tmp_nchar[tmp_nvch.tmp_len] = EOS;
				apostrophe(tmp_nvch.tmp_nchar);
				STprintf(column_value[i], ERx("'%s'"),
					tmp_nvch.tmp_nchar);
			}
		}
                else if (STequal(column_datatype[i], "boolean"))
                {
                    STprintf(tmp_statement,
                        "SELECT %s FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d",
                            dlm_column_name, sha_name, database_no,
                            transaction_id, sequence_no);
                    EXEC SQL EXECUTE IMMEDIATE :tmp_statement
                        INTO :tmp_int:null_ind;
                    EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
                    if (err)
                    {
                        /* execute immediate failed */
                        msg_func(1, 1349, err);
                        return err;
                    }
                    if (null_ind == -1)
                        rows = 0;
                    if (rows > 0)
                        STprintf(column_value[i], "%d", tmp_int);
                }
		else
		{
			STprintf(tmp_statement,
				ERx("SELECT VARCHAR(MAX(%s)) FROM %s WHERE sourcedb = %d AND transaction_id = %d AND sequence_no = %d"),
				dlm_column_name, sha_name, database_no,
				transaction_id, sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :tmp_statement
				INTO	:tmp_vch:null_ind;
			EXEC SQL INQUIRE_SQL (:rows = ROWCOUNT, :err = ERRORNO);
			if (err)
			{
				/* char execute immediate failed */
				(*msg_func)(1, 1351, err);
				return (err);
			}
			if (null_ind == -1)
				rows = 0;
			if (rows > 0)
			{
				tmp_vch.tmp_char[tmp_vch.tmp_len] = EOS;
				apostrophe(tmp_vch.tmp_char);
				STprintf(column_value[i], ERx("'%s'"),
					tmp_vch.tmp_char);
			}
		}
		if (where_clause)
		{
			if (i > 0)
				STcat(where_clause, ERx(" AND "));
			STcat(where_clause, ERx("t."));
			STcat(where_clause, dlm_column_name);
			if (rows == 0)
			{
				STcat(where_clause, ERx(" IS NULL "));
			}
			else
			{
				STcat(where_clause, ERx(" = "));
				STcat(where_clause, column_value[i]);
			}
		}
		if (name_clause)
		{
			if (i > 0)
			{
				STcat(name_clause, ERx(", "));
			}
			STcat(name_clause, dlm_column_name);
		}
		if (insert_value_clause)
		{
			if (i > 0)
			{
				STcat(insert_value_clause, ERx(", "));
			}
			STcat(insert_value_clause, column_value[i]);
		}
		if (update_clause)
		{
			if (i > 0)
			{
				STcat(update_clause, ERx(", "));
			}
			STcat(update_clause, dlm_column_name);
			STcat(update_clause, ERx(" = "));
			STcat(update_clause, column_value[i]);
		}
	}

	return (OK);
}


/*{
** Name:	apostrophe - double apostrophes in a string
**
** Description:
**	Doubles apostrophes in a null-terminated string, except leading
**	and trailing.
**	eg,	'O'Toole, Peter' --> 'O''Toole, Peter'
**		'I don't know how it's done' --> 'I don''t know how it''s done'
**
** Assumptions:
**	The 'string' parameter is expected to be a buffer with enough
**	space to duplicate the apostrophes present in the string,
**	potentially almost twice as long as the original.
**	Also, 'string' is presumed to have leading and trailing
**	apostrophes, since they are not looked at.
**
** Inputs:
**	string - pointer to a null-terminated string, in a buffer that
**		has extra space to expand any apostrophes in the string
**
** Outputs:
**	string - pointer to the same null-terminated string, with all
**		original apostrophes duplicated
**
** Returns:
**	none
**
** Side effects:
**	The size of 'string' is n characters longer than the original,
**	where n is the number of apostrophes in the original.
*/
static void
apostrophe(
char	*string)
{
	char	tmp[1000];
	char	*p;

	for (p = string; *p != EOS; CMnext(p))
	{
		if (CMcmpcase(p, apos) == 0)
		{
			CMnext(p);
			STcopy(p, tmp);
			STcopy(apos, p);
			STcat(p, tmp);
		}
	}
}

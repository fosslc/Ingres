/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include "errm.h"

/*
** Name:	tblregst.sc - default table registration
**
** Description:
**	Defines
**		tbl_register	- default table registration
**		get_unique_index - get unique secondary index
**		check_for_null_keys	- check for null keys
**
** History:
**	16-dec-96 (joea)
**		Created based on tblrgstr.sc in replicator library.
**	18-may-98 (padni01) bug 89865
**		Use date_gmt instead of date to set columns_registered field 
**		of dd_regist_tables.
**	21-sep-98 (abbjo03)
**		Merge cknulkey.sc. Replace ddba_messageit with IIUGerr.
**	06-mar-2000 (abbjo03) bug 92674
**		Change check_for_null_keys not to include dd_regist_tables in
**		its query since the table to be registered hasn't yet been
**		inserted there.
**/

static STATUS get_unique_index(char *table_name, char *table_owner,
	char *index_name);
static STATUS check_for_null_keys(i4 tableno, char *table_owner,
	char *table_name);
STATUS db_config_changed(i2 db_no);


/*{
** Name:	tbl_register - default table registration
**
** Description:
**	Performs default registration for a specific table.
**
** Inputs:
**	table_no	- an existing table number
**	table_name	- table name
**	table_owner	- table owner
**	cdds_no		- CDDS number
**	unique_rule	- does table have unique index
**	table_indexes	- does table have secondary indexes
**
** Outputs:
**	table_no	- the newly assigned table number
**	columns_registered - date/time as of which registration occurred
**	index_used	- the primary or secondary index used for replication
**
** Returns:
**	?
*/
STATUS
tbl_register(
i4	*table_no,
char	*table_name,
char	*table_owner,
i2	cdds_no,
char	*unique_rule,
char	*table_indexes,
char	*columns_registered,
char	*index_used)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	*table_no;
char	*table_name;
char	*table_owner;
i2	cdds_no;
char	*columns_registered;
char	*index_used;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;
	bool	table_registered = FALSE;
	STATUS	retval;

	/* Assure that table number is assigned and unique */
	if (*table_no == 0)
	{
		EXEC SQL UPDATE dd_last_number
			SET	last_number = last_number + 1
			WHERE	column_name = 'table_no';
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00E2_Err_asgn_tbl_no, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00E2_Err_asgn_tbl_no);
		}

		EXEC SQL SELECT last_number INTO :*table_no
			FROM	dd_last_number
			WHERE	column_name = 'table_no';
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00E2_Err_asgn_tbl_no, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00E2_Err_asgn_tbl_no);
		}
	}
	else	/* Eliminate existing column registration */
	{
		EXEC SQL DELETE FROM dd_regist_columns
			WHERE	table_no = :*table_no;
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			IIUGerr(E_RM004E_Error_del_regist_col,
				UG_ERR_ERROR, 0);
			EXEC SQL ROLLBACK;
			return (E_RM004E_Error_del_regist_col);
		}

		table_registered = TRUE;
	}

	/* Do column registration */
	EXEC SQL REPEATED INSERT INTO dd_regist_columns (table_no,
			column_name, column_sequence, key_sequence)
		SELECT	:*table_no,
			column_name, column_sequence, 0
		FROM	iicolumns
		WHERE	table_name = :table_name
		AND	table_owner = :table_owner;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM00DE_Err_add_regist_cols, UG_ERR_ERROR, 2,
			table_owner, table_name);
		return (E_RM00DE_Err_add_regist_cols);
	}

	if (CMcmpcase(unique_rule, ERx("U")) == 0)  /* unique primary index */
	{
		STcopy(table_name, index_used);
		EXEC SQL REPEATED UPDATE dd_regist_columns r
			FROM	iicolumns c
			SET	key_sequence = c.key_sequence
			WHERE	c.table_name = :table_name
			AND	c.table_owner = :table_owner
			AND	c.column_name = r.column_name
			AND	c.key_sequence > 0
			AND	r.table_no = :*table_no;
		if (RPdb_error_check(0, &errinfo) != OK)
		{
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00DF_Err_updt_regist_cols, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00DF_Err_updt_regist_cols);
		}
	}
	else if (CMcmpcase(table_indexes, ERx("Y")))	/* no secondaries */
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM00E0_No_unique_key, UG_ERR_ERROR, 2, table_owner,
			table_name);
		return (E_RM00E0_No_unique_key);
	}
	else
	{
		retval = get_unique_index(table_name, table_owner, index_used);
		switch (retval)
		{
		case 1:		/* unique secondary found */
			EXEC SQL REPEATED UPDATE dd_regist_columns r
				FROM	iiindexes i, iicolumns c
				SET	key_sequence = c.key_sequence
				WHERE	c.table_name = i.index_name
				AND	c.table_owner = i.index_owner
				AND	r.column_name = c.column_name
				AND	r.table_no = :*table_no
				AND	i.index_name = :index_used;
			if (RPdb_error_check(0, &errinfo) != OK)
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM00DF_Err_updt_regist_cols,
					UG_ERR_ERROR, 2, table_owner,
					table_name);
				return (E_RM00DF_Err_updt_regist_cols);
			}
			break;

		case 0:		/* no unique secondary */
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00E0_No_unique_key, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00E0_No_unique_key);

		default:
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00E1_Err_get_index, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00E1_Err_get_index);
		}
	}

	/*
	** As a final precaution, make sure that none of the key columns are
	** nullable
	*/
	retval = check_for_null_keys(*table_no, table_owner, table_name);
	if (retval)	/* key columns are nullable or there was an error */
	{
		EXEC SQL ROLLBACK;
		return (retval);
	}

	/* Insert newly registered table into dd_regist_tables */
	EXEC SQL SELECT DATE_GMT('now') INTO :columns_registered;
	if (!table_registered)
	{
		EXEC SQL REPEATED INSERT INTO dd_regist_tables
				(table_no, table_name, table_owner,
				columns_registered, index_used, cdds_no)
			VALUES	(:*table_no, :table_name, :table_owner,
				:columns_registered, :index_used, :cdds_no);
	}
	else
	{
		EXEC SQL UPDATE dd_regist_tables
			SET	columns_registered = :columns_registered,
				supp_objs_created = '',
				index_used = :index_used,
				cdds_no = :cdds_no
			WHERE	table_no = :*table_no;
	}
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		if (errinfo.errorno)
		{
			EXEC SQL ROLLBACK;
			return (errinfo.errorno);
		}
		else
		{
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM00DF_Err_updt_regist_cols, UG_ERR_ERROR, 2,
				table_owner, table_name);
			return (E_RM00DF_Err_updt_regist_cols);
		}
	}
	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		return (FAIL);
	}

	EXEC SQL COMMIT;

	return (OK);
}


/*
** Name:	get_unique_index - get unique secondary index
**
** Description:
**	returns 1 if unique secondary found 0 if no unique secondary found
**	-1 if error
**
** Inputs:
**	table_name	- which table
**	table_owner	- who owns it
**
** Outputs:
**	index_name	- first unique index found
**
** Returns:
**	?
*/
static STATUS
get_unique_index(
char	*table_name,
char	*table_owner,
char	*index_name)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
char	*table_owner;
char	*index_name;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	short	null_ind = -1;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL REPEATED SELECT MIN(index_name)
		INTO	:index_name:null_ind
		FROM	iiindexes
		WHERE	base_name = :table_name
		AND	base_owner = :table_owner
		AND	unique_rule = 'U';
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		if (errinfo.errorno)
			return (-1);

	if (null_ind == -1)	/* none found */
		return (0);
	else
		return (1);
}


/*{
** Name:	check_for_null_keys - check for null keys
**
** Description:
**	Checks whether the specified table allows null values in its key
**	columns.
**
** Inputs:
**	tableno		- table number
**	table_owner	- table owner
**	table_name	- table name
**
** Outputs:
**	none
**
** Returns:
**	OK, E_RM00DC_Err_ck_null_cols, E_RM00DD_Tbl_null_keys
*/
static STATUS
check_for_null_keys(
i4	tableno,
char	*table_owner,
char	*table_name)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	tableno;
char	*table_owner;
char	*table_name;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	cnt;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT COUNT(*) INTO	:cnt
		FROM	dd_regist_columns c, iicolumns i
		WHERE	c.table_no = :tableno
		AND	i.table_name = :table_name
		AND	i.table_owner = :table_owner
		AND	i.column_nulls = 'Y'
		AND	i.column_name = c.column_name
		AND	c.key_sequence > 0;
	RPdb_error_check(0, &errinfo);
	if (errinfo.errorno)
	{
		IIUGerr(E_RM00DC_Err_ck_null_cols, UG_ERR_ERROR, 2, table_owner,
			table_name);
		return (E_RM00DC_Err_ck_null_cols);
	}

	if (cnt)
	{
		IIUGerr(E_RM00DD_Tbl_null_keys, UG_ERR_ERROR, 2, table_owner,
			table_name);
		return (E_RM00DD_Tbl_null_keys);
	}

	return (OK);
}

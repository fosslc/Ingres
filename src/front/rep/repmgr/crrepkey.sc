/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cv.h>
# include <nm.h>
# include <lo.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <gl.h>
# include <iicommon.h>
# include <adf.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <tblobjs.h>
# include <wchar.h>
# include "errm.h"

EXEC SQL INCLUDE sqlda;
EXEC SQL INCLUDE <tbldef.sh>;
typedef char    DCOLNAME[DB_MAXNAME*2+3]; /* delimited column name */  

/**
** Name:	crrepkey.sc - create replication keys
**
** Description:
**	Defines
**		create_replication_keys - create replication keys
**		free_sqlda		- free the sqlda buffer
**		get_cdds_no		- get the CDDS number
**		get_dd_priority		- get the priority
**
** History:
**	09-jan-97 (joea)
**		Created based on crrepkey.sc in replicator library.
**	29-apr-97 (joea) bug 81857
**		Remove inserts into dd_transaction_id. Determine if temporary
**		table exists before trying to drop it. Correct manipulation of
**		locations for shadow and input queue files.
**	18-nov-97 (padni01) bug 87055
**		Add missing case for text datatype (to be same as that for the
**		varchar datatype) for building values part of insert string.
**	09-mar-98 (padni01) bug 86940
**		In building values part of insert string, prefix wc with 'r.'
**		and not prefix tmp with the 'tab' character but write 'tab' to 
**		file_data before writing tmp to it.
**	22-jul-98 (padni01) bug 90827
**		Add support for decimal datatype
**	26-nov-98 (abbjo03/islsa01) bug 92413
**		This fix prevents converting varchar data to char data while
**		data are copied from source table to shadow table. For example,
**		if the source table contains varchar(30) - data '234\\567' (that
**		is a single backslash). The shadow table should exactly hold
**		same data '234\\567' instead of '234567', which is caused by
**		char conversion from varchar. The local variable len_spec is
**		called to store the length of data value.
**	23-mar-1999 (islsa01) bug 95574
**		Required to change the aliases in the select string. Rather,
**		this bug occurs when the lookup column is NOT the same as the
**		replication key on the base table. Instead of 'r' the alias for
**		the lookup table and 't' the alias for the base table, changes
**		are made to switch the aliases.
**	26-oct-99 (gygro01) bug 99272/pb ingrep 67
**		Createkeys fails if base table keys has datatype char/varchar/
**		date/byte/byte varying/text/c and horizontal partitioning is
**		setup. Problem is due to missing apostrophe around the values.
**	10-nov-99 (abbjo03) sir 99462
**		Count the actual number of rows in the shadow table. If it's
**		empty, drop the index, disable journaling and modify it to heap
**		to allow use of LOAD on the copy. Use a global temporary table
**		and only select the key columns into it. Change DD_TRANS_MAX
**		to II_REP_CREATEKEYS_TRANS_MAX which should hold a number of
**		rows and will now be documented.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-nov-2009 (joea)
**          Add test for "boolean" in create_replication_keys.
**      14-may-2010 (stial01)
**          Don't alloc DB_MAX_COLS delimited column names on stack
**          Use DB_IITYPE_LEN for datatype
**/

# define	MAX_SIZE		6000
# define	TRANS_MAX		10000
# define	MODIFY_TABLE_LIMIT	10000


GLOBALREF ADF_CB	*RMadf_cb;
GLOBALREF
EXEC SQL BEGIN DECLARE SECTION;
	TBLDEF *RMtbl;
EXEC SQL END DECLARE SECTION;


FUNC_EXTERN STATUS RMmodify_input_queue(void);
FUNC_EXTERN STATUS RMmodify_shadow(TBLDEF *tbl, bool recreate_index);


static void free_sqlda(IISQLDA *sqlda, i4  column_count);
static STATUS get_cdds_no(char *wc, i2 *cdds_no);
static STATUS get_dd_priority(char *wc, i2 *dd_priority);


/*{
** Name:	create_replication_keys - create replication keys
**
** Description:
**	Creates the replication keys in the shadow table and, optionally,
**	the input queue table.
**
** Inputs:
**	table_no	- table number
**	queue_flag	- do the input queue too
**
** Outputs:
**	none
**
** Returns:
**
** History
** 19-Aug-1998 (nicph02)
**	Bug 92547: Text for error message 1139 was not displaying the datatype
** 20-Aug-1998 (nicph02)
**	Bug 90716: Added support to 'byte varying'/'byte' datatype
**	03-Dec-2003 (gupsh01)
**	    Added support for nchar/nvarchar datatypes.
**	05-Apr-2004 (gupsh01)
**	    Fixed server crash due to stack overflow on windows.
**	    bug 112097.
**	05-Oct-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	22-Jun-2009 (wanfr01)
**	    Bug 122223 - Use STcopy instead of STcat to avoid truncation 
**	    due to nulls in byte columns, and add length to byte specifier
**	    in copy..from statement.  Call IILQucolinit which is required
**	    to do EXECUTE IMMEDIATE of the copy..from statement, and convert
**	    ucs4 to ucs2 if necessary for successful UTF8 conversion.
*/
STATUS
create_replication_keys(
i4	table_no,
bool	queue_flag)
{
	FILE	*shadow_file;
	FILE	*queue_file;
	i4	exists;
	i4	column_count;
	short	null_ind[DB_MAX_COLS];
	char	file_data[MAX_SIZE];
	char    *file_dataend;
	char	shadow_filename[MAX_LOC+1];
	char	queue_filename[MAX_LOC+1];
	LOCATION	sh_loc;
	LOCATION	iq_loc;
	char	*loc_name_ptr;
	COLDEF	*col_p;
	i4	i, j, count;
	char	exists_where_clause[MAX_SIZE];
	char	tmp[MAX_SIZE];
	char	wc[MAX_SIZE];
	char	key_cols[MAX_SIZE];
	char	tmp_table[DB_MAXNAME+1];
	DCOLNAME *column_name = NULL;
	char	column_datatype[DB_MAX_COLS][DB_IITYPE_LEN+1];
	i4	column_length[DB_MAX_COLS];
	i4	approx_rows;
	i4	rows_processed = 0;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	trans_max = TRANS_MAX;
	char	stmt[256];
	char	copy_stmt[MAX_SIZE];
	char	qcopy_stmt[512];
	i2	sourcedb = 0;
	i4	transaction_id = 0;
	char	timestamp[26];
	i2	cdds_no;
	i2	dd_priority;
	i4	done;
	STATUS	err;
	STATUS	stat;
	char	select_stmt[MAX_SIZE];
	i4	session1;
	i4	session2;
	char	dbname[DB_MAXNAME*2+3];
	char	dba[DB_MAXNAME+1];
	i4	base_rows, shadow_rows, input_rows;

	wchar_t	**nch_val;
	i4 nch_cnt = 0;
	NVARCHAR struct{
	  short len;
	  wchar_t text[256];
	} *nvch_val;
	i4 nvch_cnt = 0;
	char normbuf[10];

	EXEC SQL END DECLARE SECTION;
	char	*env_ptr;
	u_i2    *ucs2val;
	u_i4    *ucs4val;
	IISQLDA	_sqlda;
	IISQLDA	*sqlda = &_sqlda;
	
    for (;;)
	{
		column_name = (DCOLNAME *)MEreqmem (0, 
			(DB_MAX_COLS+1) * sizeof(*column_name), 0, &stat);
		if (stat)
		  break;

		nch_val = (wchar_t **)MEreqmem (0, 
				(sizeof(wchar_t *) * DB_MAX_COLS), 0, &stat);
		if (stat)
		  break;

		for (i=0; i < DB_MAX_COLS; i++)
		{
		  nch_val[i] = (wchar_t *)MEreqmem (0, 
				(sizeof(wchar_t) * 256), 0, &stat); 
		}

		if (stat)
		  break;
		
  		nvch_val =  MEreqmem (0, 
				sizeof(nvch_val) * DB_MAX_COLS, 0, &stat); 
		if (stat)
		  break;

		NMgtAt(ERx("II_REP_CREATEKEYS_TRANS_MAX"), &env_ptr);
		if (env_ptr != NULL && *env_ptr != EOS)
		{
			CVal(env_ptr, &trans_max);
			if (trans_max < TRANS_MAX)
				trans_max = TRANS_MAX;
		}

		err = RMtbl_fetch(table_no, TRUE);
		if (err)
		{
			EXEC SQL ROLLBACK;
			return (err);
		}
		EXEC SQL COMMIT;

		IIUGmsg(ERget(F_RM00DD_Creating_keys), FALSE, 2, RMtbl->table_owner,
			RMtbl->table_name);
		EXEC SQL SET SESSION WITH ON_ERROR = ROLLBACK TRANSACTION;
		EXEC SQL SET_SQL (PREFETCHROWS = :trans_max);
		EXEC SQL SELECT database_no, TRIM(vnode_name) + '::' +
				TRIM(database_name), DBMSINFO('dba')
			INTO	:sourcedb, :dbname, :dba
			FROM	dd_databases
			WHERE	local_db = 1;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0030_Err_retrieve_db_name, UG_ERR_ERROR, 0);
			EXEC SQL ROLLBACK;
			return (E_RM0030_Err_retrieve_db_name);
		}


		err = table_exists(RMtbl->table_name, RMtbl->table_owner, &exists);
		if (err)
		{
			EXEC SQL ROLLBACK;
			return (err);
		}
		if (!exists)
		{
			IIUGerr(E_RM0124_Tbl_not_exist, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			EXEC SQL ROLLBACK;
			return (E_RM0124_Tbl_not_exist);
		}

		err = table_exists(RMtbl->sha_name, dba, &exists);
		if (err)
		{
			EXEC SQL ROLLBACK;
			return (err);
		}
		if (!exists)
		{
			IIUGerr(E_RM0125_Shadow_not_exist, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			EXEC SQL ROLLBACK;
			return (E_RM0125_Shadow_not_exist);
		}

		STprintf(stmt, ERx("SELECT COUNT(*) FROM %s"), RMtbl->sha_name);
		EXEC SQL EXECUTE IMMEDIATE :stmt
			INTO	:shadow_rows;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0126_Err_rtrv_num_rows, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->sha_name);
			return (E_RM0126_Err_rtrv_num_rows);
		}
		EXEC SQL SELECT num_rows
			INTO	:base_rows
			FROM	iitables
			WHERE	table_name = :RMtbl->table_name
			AND	table_owner = :RMtbl->table_owner;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0126_Err_rtrv_num_rows, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			return (E_RM0126_Err_rtrv_num_rows);
		}

		if (!shadow_rows)
		{
			char	idx_name[DB_MAXNAME+1];

			STprintf(stmt, ERx("SET NOJOURNALING ON %s"), RMtbl->sha_name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM0127_Err_set_nojournal, UG_ERR_ERROR, 2,
					RMtbl->table_owner, RMtbl->sha_name);
				EXEC SQL ROLLBACK;
				return (E_RM0127_Err_set_nojournal);
			}
			RPtblobj_name(RMtbl->table_name, table_no, TBLOBJ_SHA_INDEX1,
				idx_name);
			err = table_exists(idx_name, RMtbl->table_owner, &exists);
			if (err != OK)
			{
				EXEC SQL ROLLBACK;
				return (err);
			}
			if (exists)
			{
				STprintf(stmt, ERx("DROP INDEX %s"), idx_name);
				EXEC SQL EXECUTE IMMEDIATE :stmt;
				EXEC SQL INQUIRE_SQL (:err = ERRORNO);
				if (err)
				{
					IIUGerr(E_RM0128_Err_dropping, UG_ERR_ERROR, 1,
						idx_name);
					EXEC SQL ROLLBACK;
					return (E_RM0128_Err_dropping);
				}
			}
			STprintf(stmt, ERx("MODIFY %s TO HEAP"), RMtbl->sha_name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM0129_Err_modifying, UG_ERR_ERROR, 1,
					RMtbl->sha_name);
				EXEC SQL ROLLBACK;
				return (E_RM0129_Err_modifying);
			}
		}
		else
		{
			if (RPtblobj_name(RMtbl->table_name, table_no, TBLOBJ_TMP_TBL,
				tmp_table))
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM00EB_Err_gen_obj_name, UG_ERR_ERROR, 2,
					RMtbl->table_owner, RMtbl->table_name);
				return (E_RM00EB_Err_gen_obj_name);
			}
			*key_cols = EOS;
			for (col_p = RMtbl->col_p; col_p < RMtbl->col_p + RMtbl->ncols;
				++col_p)
			{
				if (!col_p->key_sequence)
					continue;
				if (*key_cols)
					STcat(key_cols, ", ");
				STcat(key_cols, col_p->dlm_column_name);
			}
			STprintf(stmt, ERx("DECLARE GLOBAL TEMPORARY TABLE SESSION.%s \
	AS SELECT %s FROM %s ON COMMIT PRESERVE ROWS WITH NORECOVERY, STRUCTURE = HEAP,\
	NOCOMPRESSION"),
				tmp_table, key_cols, RMtbl->sha_name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM012A_Err_creat_tmp_tbl, UG_ERR_ERROR, 1,
					tmp_table);
				return (E_RM012A_Err_creat_tmp_tbl);
			}
		}

		if (NMloc(TEMP, PATH, NULL, &sh_loc) != OK)
			return (FAIL);
		LOcopy(&sh_loc, shadow_filename, &sh_loc);
		if (LOuniq(ERx("sh"), ERx("dat"), &sh_loc) != OK)
			return (FAIL);
		LOtos(&sh_loc, &loc_name_ptr);
		STcopy(loc_name_ptr, shadow_filename);
		if (SIfopen(&sh_loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &shadow_file)
			!= OK)
		{
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM005E_Error_open_file, UG_ERR_ERROR, 1,
				shadow_filename);
			return (E_RM005E_Error_open_file);
		}

		if (queue_flag)
		{
			EXEC SQL SELECT num_rows
				INTO	:input_rows
				FROM	iitables
				WHERE	table_name = 'dd_input_queue'
				AND	table_owner = :dba;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM0126_Err_rtrv_num_rows, UG_ERR_ERROR, 2,
					dba, ERx("dd_input_queue"));
				return (E_RM0126_Err_rtrv_num_rows);
			}
			if (input_rows < MODIFY_TABLE_LIMIT)
			{
				EXEC SQL MODIFY dd_input_queue TO HEAP;
				EXEC SQL INQUIRE_SQL (:err = ERRORNO);
				if (err)
				{
					IIUGerr(E_RM0129_Err_modifying, UG_ERR_ERROR, 1,
						ERx("dd_input_queue"));
					return (E_RM0129_Err_modifying);
				}
			}

			if (NMloc(TEMP, PATH, NULL, &iq_loc) != OK)
				return (FAIL);
			LOcopy(&iq_loc, queue_filename, &iq_loc);
			if (LOuniq(ERx("iq"), ERx("dat"), &iq_loc) != OK)
				return (FAIL);
			LOtos(&iq_loc, &loc_name_ptr);
			STcopy(loc_name_ptr, queue_filename);
			if (SIfopen(&iq_loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC,
				&queue_file) != OK)
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM005E_Error_open_file, UG_ERR_ERROR, 1,
					queue_filename);
				return (E_RM005E_Error_open_file);
			}
			STprintf(qcopy_stmt, ERx("COPY TABLE dd_input_queue (sourcedb =\
	c0tab, transaction_id = c0tab, sequence_no = c0tab, table_no = c0tab, \
	old_sourcedb = c0tab, old_transaction_id = c0tab, old_sequence_no = c0tab, \
	trans_time = c0tab, trans_type = c0tab, cdds_no = c0tab, dd_priority = c0nl) \
	FROM '%s'"),
				queue_filename);
		}

		STcopy(ERx("SELECT DISTINCT "), select_stmt);
		*exists_where_clause = EOS;
		STprintf(copy_stmt,
			ERx("COPY TABLE %s (sourcedb = c0tab, transaction_id = c0tab, \
	sequence_no = c0tab, trans_type = c0tab, trans_time = c0"),
			RMtbl->sha_name);
		for (column_count = 0, col_p = RMtbl->col_p;
			col_p < RMtbl->col_p + RMtbl->ncols; ++col_p)
		{
			if (!col_p->key_sequence)
				continue;
			if (column_count)
			{
				STcat(select_stmt, ERx(", "));
				STcat(exists_where_clause, ERx(" AND "));
			}
			STprintf(tmp, ERx("s.%s = t.%s"), col_p->dlm_column_name,
				col_p->dlm_column_name);
			STcat(exists_where_clause, tmp);
			if (STequal(col_p->column_datatype, ERx("date")) || 
			    STequal(col_p->column_datatype, ERx("ingresdate")) || 
			    STequal(col_p->column_datatype, ERx("ansidate")) || 
			    STequal(col_p->column_datatype, 
						ERx("time without time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("time with time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("time with local time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("timestamp without time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("timestamp with time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("timestamp with local time zone")) ||
			    STequal(col_p->column_datatype, 
						ERx("interval year to month")) ||
			    STequal(col_p->column_datatype, 
						ERx("interval day to second")))
			{
			    STprintf(tmp, ERx("%s = CHAR(%s)"),
					col_p->dlm_column_name, col_p->dlm_column_name);

			    if (STequal(col_p->column_datatype, ERx("date")) || 
			        STequal(col_p->column_datatype, ERx("ingresdate")))
			      column_length[column_count] = 26;
			    else if (STequal(col_p->column_datatype, ERx("ansidate")))
			      column_length[column_count] = 18;
			    else if (STequal(col_p->column_datatype, 
						ERx("time without time zone")))
			      column_length[column_count] = 22;
			    else if (STequal(col_p->column_datatype, 
						ERx("time with time zone")) ||
			    	     STequal(col_p->column_datatype, 
						ERx("time with local time zone")))
				column_length[column_count] = 32;
			    else if (STequal(col_p->column_datatype, 
						ERx("timestamp without time zone")))
				column_length[column_count] = 40;
			    else if (STequal(col_p->column_datatype, 
						ERx("timestamp with time zone")) ||
			    	     STequal(col_p->column_datatype, 
						ERx("timestamp with local time zone")))
				column_length[column_count] = 50;
			    else if (STequal(col_p->column_datatype, 
						ERx("interval year to month")))
				column_length[column_count] = 16;
			    else if (STequal(col_p->column_datatype, 
						ERx("interval day to second")))
				column_length[column_count] = 46;
			}
			else if (STequal(col_p->column_datatype, ERx("decimal")))
			{
				STprintf(tmp, ERx("%s = CHAR(%s)"),
					col_p->dlm_column_name, col_p->dlm_column_name);
				column_length[column_count] = col_p->column_length + 2;
			}
			else if (STequal(col_p->column_datatype, ERx("money")))
			{
				STprintf(tmp, ERx("%s = FLOAT8(%s)"),
					col_p->dlm_column_name, col_p->dlm_column_name);
				column_length[column_count] = sizeof(double);
			}
			else if (STequal(col_p->column_datatype, ERx("varchar")) ||
				STequal(col_p->column_datatype, ERx("text")) ||
				STequal(col_p->column_datatype, ERx("byte varying")))
			{
				column_length[column_count] = col_p->column_length +
					sizeof(short);
				STcopy(col_p->dlm_column_name, tmp);
			}
			else if (STequal(col_p->column_datatype, ERx("nvarchar")))
			{
				column_length[column_count] =
					col_p->column_length * sizeof(wchar_t) + 
					sizeof (short);
				STcopy(col_p->dlm_column_name, tmp);
			}
			else if (STequal(col_p->column_datatype, ERx("integer")) ||
				STequal(col_p->column_datatype, ERx("float")) ||
				STequal(col_p->column_datatype, ERx("c")) ||
				STequal(col_p->column_datatype, ERx("char")) ||
				STequal(col_p->column_datatype, ERx("byte")))
			{
				STcopy(col_p->dlm_column_name, tmp);
				column_length[column_count] = col_p->column_length;
			}
			else if (STequal(col_p->column_datatype, "boolean"))
			{
				STcopy(col_p->dlm_column_name, tmp);
				column_length[column_count] = 1;
			}
			else if (STequal(col_p->column_datatype, ERx("nchar")))
			{
				column_length[column_count] =
					col_p->column_length * sizeof(wchar_t);
				STcopy(col_p->dlm_column_name, tmp);
			}
			else
			{
				IIUGerr(E_RM012B_Unsupp_datatype, UG_ERR_ERROR, 2,
					col_p->column_datatype, col_p->column_name);
				free_sqlda(sqlda, column_count);
				return (E_RM012B_Unsupp_datatype);
			}
			STcat(select_stmt, tmp);
			sqlda->sqlvar[column_count].sqldata = (char *)MEreqmem(0,
				(u_i4)(column_length[column_count] + 1), TRUE, NULL);
			if (sqlda->sqlvar[column_count].sqldata == NULL)
			{
				IIUGerr(E_RM00FE_Err_alloc_col, UG_ERR_ERROR, 0);
				free_sqlda(sqlda, column_count);
				return (E_RM00FE_Err_alloc_col);
			}
			sqlda->sqlvar[column_count].sqlind = &null_ind[column_count];
			if (STequal(col_p->column_datatype, ERx("varchar")) ||
				STequal(col_p->column_datatype, ERx("text")) ||
				STequal(col_p->column_datatype, ERx("byte varying")))
				STprintf(tmp, ERx("tab, %s = varchar(0)"),
					col_p->dlm_column_name);
			else if (STequal(col_p->column_datatype, ERx("nchar")))
				STprintf(tmp, ERx("tab, %s = nchar(0)"),
					col_p->dlm_column_name);
			else if (STequal(col_p->column_datatype, ERx("nvarchar")))
				STprintf(tmp, ERx("tab, %s = nvarchar(0)"),
					col_p->dlm_column_name);
			else if (STequal(col_p->column_datatype, ERx("byte"))) 
				STprintf(tmp, ERx("tab, %s = byte(%d)"),
					col_p->dlm_column_name, 
					col_p->column_length);
			else
				STprintf(tmp, ERx("tab, %s = c0"),
					col_p->dlm_column_name);
			STcat(copy_stmt, tmp);

			STcopy(col_p->dlm_column_name, column_name[column_count]);
			STcopy(col_p->column_datatype, column_datatype[column_count]);
			++column_count;
		}
		if (err)
		{
			free_sqlda(sqlda, column_count);
			return (err);
		}

		EXEC SQL COMMIT;
		STcat(exists_where_clause, ERx(")"));

		STprintf(tmp, ERx(" FROM %s.%s"), RMtbl->dlm_table_owner,
			RMtbl->dlm_table_name);
		STcat(select_stmt, tmp);
		if (shadow_rows)
		{
			STprintf(tmp, ERx(" t WHERE NOT EXISTS (SELECT * FROM \
	SESSION.%s s WHERE %s"),
				tmp_table, exists_where_clause);
			STcat(select_stmt, tmp);
		}

		STcat(copy_stmt,
			ERx("tab, cdds_no = c0tab, dd_priority = c0nl) FROM '"));
		STcat(copy_stmt, shadow_filename);
		STcat(copy_stmt, ERx("'"));

		sqlda->sqln = column_count;
		EXEC SQL PREPARE s1 FROM :select_stmt;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM012C_Err_prep_stmt, UG_ERR_ERROR, 0);
			EXEC SQL ROLLBACK;
			free_sqlda(sqlda, column_count);
			return (E_RM012C_Err_prep_stmt);
		}
		EXEC SQL DESCRIBE s1 INTO :sqlda;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM012D_Err_descr_stmt, UG_ERR_ERROR, 0);
			EXEC SQL ROLLBACK;
			free_sqlda(sqlda, column_count);
			return (E_RM012D_Err_descr_stmt);
		}
		EXEC SQL DECLARE c1 CURSOR FOR s1;
		EXEC SQL OPEN c1 FOR READONLY;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM012E_Err_open_base_curs, UG_ERR_ERROR, 0);
			EXEC SQL ROLLBACK;
			free_sqlda(sqlda, column_count);
			return (E_RM012E_Err_open_base_curs);
		}

		EXEC SQL INQUIRE_SQL (:session1 = SESSION);
		session2 = session1 + 10;
		EXEC SQL CONNECT :dbname SESSION :session2 IDENTIFIED BY :dba;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1, dbname);
			free_sqlda(sqlda, column_count);
			EXEC SQL SET_SQL (SESSION = :session1);
			return (E_RM0084_Error_connecting);
		}
		EXEC SQL SET SESSION WITH ON_ERROR = ROLLBACK TRANSACTION;

		approx_rows = (i4)(((base_rows - shadow_rows) / 100) + 0.5) * 100;
		for (done = 0; !done && !err;)
		{
			bool	found = FALSE;

			EXEC SQL SET_SQL (SESSION = :session2);
			/* get transaction_id, trans_time */
			EXEC SQL REPEATED SELECT INT4(RIGHT(DBMSINFO('db_tran_id'),
					16)), DATE('now')
				INTO	:transaction_id, :timestamp;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM012F_Err_get_trans_id, UG_ERR_ERROR, 0);
				EXEC SQL ROLLBACK;
				EXEC SQL DISCONNECT;
				EXEC SQL SET_SQL (SESSION = :session1);
				EXEC SQL ROLLBACK;
				free_sqlda(sqlda, column_count);
				return (E_RM012F_Err_get_trans_id);
			}
			STtrmwhite(timestamp);
			for (i = 0; !err && i < trans_max && !done; i++)
			{
				EXEC SQL SET_SQL (SESSION = :session1);
				EXEC SQL FETCH c1 USING DESCRIPTOR :sqlda;
				EXEC SQL INQUIRE_SQL (:err = ERRORNO, :done = ENDQUERY);
				if (err)
				{
					IIUGerr(E_RM0130_Err_fetch_base, UG_ERR_ERROR,
						0);
					EXEC SQL SET_SQL (SESSION = :session2);
					EXEC SQL ROLLBACK;
					EXEC SQL DISCONNECT;
					EXEC SQL SET_SQL (SESSION = :session1);
					EXEC SQL ROLLBACK;
					free_sqlda(sqlda, column_count);
					return (E_RM0130_Err_fetch_base);
				}
				if (!done)
				{
					short	data_length;

					found = TRUE;
					/* build values part of insert string */
					STprintf(file_data, ERx("%d\t%d\t%d\t1\t%s"),
						sourcedb, transaction_id, i, timestamp);
					file_dataend = file_data+STlength(file_data);
					for (j = 0; j < column_count; j++)
					{
						if (j == 0)
							STcopy(ERx("r."), wc);
						else
							STcat(wc, ERx(" AND r."));
						data_length = sqlda->sqlvar[j].sqlname.sqlnamel;
						sqlda->sqlvar[j].sqlname.sqlnamec[data_length] = 0;
						if (STequal(column_datatype[j],
							ERx("integer")))
						{
							if (column_length[j] == 1)
								STprintf(tmp, ERx("%d"),
									*(char *)sqlda->sqlvar[j].sqldata);
							else if (column_length[j] == 2)
								STprintf(tmp, ERx("%d"),
									*(short *)sqlda->sqlvar[j].sqldata);
							else if (column_length[j] == 4)
								STprintf(tmp, ERx("%d"),
									*(i4 *)sqlda->sqlvar[j].sqldata);
						}
						else if (STequal(column_datatype[j],
							ERx("float")) ||
							STequal(column_datatype[j],
							ERx("money")))
						{
							if (column_length[j] == 4)
								STprintf(tmp,
									ERx("%.13g"),
									*(float *)sqlda->sqlvar[j].sqldata,
									(char)RMadf_cb->adf_decimal.db_decimal);
							else if (column_length[j] == 8)
								STprintf(tmp,
									ERx("%.13g"),
									*(double *)sqlda->sqlvar[j].sqldata,
									(char)RMadf_cb->adf_decimal.db_decimal);
						}
						else if (STequal(column_datatype[j],
							ERx("varchar")) ||
							STequal(column_datatype[j],
							ERx("text")) ||
							STequal(column_datatype[j],
							ERx("byte varying")))
						{
							data_length = *(short *)sqlda->sqlvar[j].sqldata;
							sqlda->sqlvar[j].sqldata[data_length
								+ sizeof(short)] = 0;
							STprintf(tmp, ERx("%s"),
								&sqlda->sqlvar[j].sqldata[sizeof(short)]);
						}
						else if (STequal(column_datatype[j],
							ERx("byte")))
						{
							data_length = sqlda->sqlvar[j].sqllen;
							MEcopy(sqlda->sqlvar[j].sqldata,data_length,tmp);
						}
						else if (STequal(column_datatype[j], 
												ERx("nvarchar")) || 
												STequal(column_datatype[j], 
												ERx("nchar")))
											{
						DB_DATA_VALUE unidata;
						DB_DATA_VALUE utf8data;
						STATUS stat = OK;
						if (STequal(column_datatype[j], ERx("nvarchar")))
						{
							data_length = *(short *)sqlda->sqlvar[j].sqldata;
							nvch_val[nvch_cnt].len = 
										*(short *)sqlda->sqlvar[j].sqldata;
							wcsncpy (nvch_val[nvch_cnt].text, 
										(wchar_t *)sqlda->sqlvar[j].sqldata,
										*(short *)sqlda->sqlvar[j].sqldata);
							(nvch_val[nvch_cnt]).text[nvch_val[nvch_cnt].len] 
								= L'\0';
							nvch_cnt++;
						}
						else 
						{
							data_length = sqlda->sqlvar[j].sqllen;
							wcsncpy (nch_val[nch_cnt], 
										(wchar_t *)sqlda->sqlvar[j].sqldata,
										sqlda->sqlvar[j].sqllen);
							nch_val[nch_cnt][sqlda->sqlvar[j].sqllen] = L'\0';
							nch_cnt++;
						}
						/*
						** convert the nvarchar data to utf8 in order to 
						** write to the file.  Convert ucs4 to ucs2 if necessary.
						*/

						unidata.db_length = data_length;
						unidata.db_datatype = sqlda->sqlvar[j].sqltype;
						unidata.db_prec = sizeof(wchar_t);
						if (sizeof(wchar_t) == 2)
						{
						    unidata.db_data = sqlda->sqlvar[j].sqldata;
						}
						else
						{
						    /* adu_nvchr_toutf8 requires ucs2 data - convert ucs4 to ucs2 */
						    unidata.db_data = (char *)MEreqmem(0, 
							(data_length * 2) + sizeof(short), TRUE, NULL);

						    if (STequal(column_datatype[j], ERx("nvarchar")))
						    {
							/* copy the length field */
							ucs2val = (u_i2 *)(unidata.db_data);
							*((u_i2 *)ucs2val) = *(u_i2 *)sqlda->sqlvar[j].sqldata;
							ucs2val = (u_i2 *)(unidata.db_data + DB_CNTSIZE);
						    }
						    else
							ucs2val = (u_i2 *)(unidata.db_data);

						    /*
						    **  need to skip over the length field of the structure.
						    **  note that although length is a short, the next field is  
						    **  sizeof(wchar_t) bytes into the structure, due to auto-alignment
						    **  of structure fields!
						    */
						    if (STequal(column_datatype[j], ERx("nvarchar")))
						    {
						        ucs4val = (u_i4 *)(sqlda->sqlvar[j].sqldata + sizeof(wchar_t));
						    }
						    else
						    {
							ucs4val = (u_i4 *)sqlda->sqlvar[j].sqldata;
						    }
						    for (i = 0; i < data_length; ++i, ucs4val++, ucs2val++)
						    {
							*ucs2val = (u_i2)*ucs4val;
						    }
						}

						/* Allocte space for results UTF8 data can 
						** be 4 times the original unicode data */
						utf8data.db_length = data_length * 4; 
						utf8data.db_data = (char *)MEreqmem(0, 
							utf8data.db_length + sizeof(short), TRUE, NULL);
						utf8data.db_datatype = DB_VCH_TYPE;
						unidata.db_prec = 0;
						stat = adu_nvchr_toutf8 (RMadf_cb, &unidata, 
											&utf8data);
						/* copy UTF8 data*/
						if (STequal(column_datatype[j], ERx("nvarchar")))
							data_length = *(short *)utf8data.db_data; 
						else 
							data_length = utf8data.db_length; 
						MEcopy (utf8data.db_data + sizeof(short),
										data_length, tmp);
						tmp[data_length] = '\0';
						if (utf8data.db_data)
							MEfree (utf8data.db_data);
						}
						else
						{
							STprintf(tmp, ERx("%s"),
								sqlda->sqlvar[j].sqldata);
						}
						STcopy(ERx("	"),file_dataend);
						file_dataend++;
						if (STequal(column_datatype[j],
							ERx("varchar")) ||
							STequal(column_datatype[j],
							ERx("text")) ||
							STequal(column_datatype[j],
							ERx("nvarchar")) ||
							STequal(column_datatype[j],
							ERx("nchar")) ||
							STequal(column_datatype[j],
							ERx("byte varying")))
						{
							char	len_spec[6];
							STprintf(len_spec, "%5d",
								data_length);
							STcopy(len_spec,file_dataend);
							file_dataend+=STlength(len_spec);
						}

						if (STequal(column_datatype[j],
							ERx("nvarchar")) ||
						    STequal(column_datatype[j],
							ERx("nchar")))
						{
							MEcopy(tmp, data_length, file_dataend);
							file_dataend+=data_length;
						} 
						else if (STequal(column_datatype[j], ERx("byte")))
						{
							MEcopy(tmp, sqlda->sqlvar[j].sqllen, file_dataend);
							file_dataend+=sqlda->sqlvar[j].sqllen;
						} 
						else
						{
							STcopy(tmp,file_dataend);
							file_dataend+=STlength(tmp);
						}
						STcat(wc, column_name[j]);
						STcat(wc, ERx(" = "));
						if (STequal(column_datatype[j],
							ERx("varchar")) ||
							STequal(column_datatype[j],
							ERx("text")) ||
							STequal(column_datatype[j],
							ERx("date")) ||
							STequal(column_datatype[j],
							ERx("ingresdate")) ||
							STequal(column_datatype[j],
							ERx("ansidate")) ||
							STequal(column_datatype[j],
							ERx("time without time zone")) ||
							STequal(column_datatype[j],
							ERx("timestamp without time zone")) ||
							STequal(column_datatype[j],
							ERx("time with time zone")) ||
							STequal(column_datatype[j],
							ERx("timestamp with time zone")) ||
							STequal(column_datatype[j],
							ERx("time with local time zone")) ||
							STequal(column_datatype[j],
							ERx("timestamp with local time zone")) ||
							STequal(column_datatype[j],
							ERx("time")) ||
							STequal(column_datatype[j],
							ERx("timestamp")) ||
							STequal(column_datatype[j],
							ERx("interval year to month")) ||
							STequal(column_datatype[j],
							ERx("interval day to second")) ||
							STequal(column_datatype[j],
							ERx("c")) ||
							STequal(column_datatype[j],
							ERx("byte")) ||
							STequal(column_datatype[j],
							ERx("byte varying")) ||
							STequal(column_datatype[j],
							ERx("char")))
						{
							STcat(wc, ERx("'"));
							STcat(wc, tmp);
							STcat(wc, ERx("'"));
						}
						else if (STequal(column_datatype[j],
												ERx("nvarchar")))
											{
												char    vdata[15];
												i4 i = nvch_cnt;
												STprintf(vdata,":nvch_val[%d]", nvch_cnt);
												STcat(wc, vdata);
											}
											else if (STequal(column_datatype[j],
												ERx("nchar")))
											{
												char    vdata[15];
													i4 i = nch_cnt;
													STprintf(vdata,":nch_val[%d]", nch_cnt);
													STcat(wc, vdata);
											}
						else
						{
							STcat(wc, tmp);
						}
					}
					err = get_cdds_no(wc, &cdds_no);
					if (err)
					{
						EXEC SQL SET_SQL (SESSION = :session2);
						EXEC SQL ROLLBACK;
						EXEC SQL DISCONNECT;
						EXEC SQL SET_SQL (SESSION = :session1);
						EXEC SQL ROLLBACK;
						free_sqlda(sqlda, column_count);
						return (err);
					}
					err = get_dd_priority(wc, &dd_priority);
					if (err)
					{
						EXEC SQL SET_SQL (SESSION = :session2);
						EXEC SQL ROLLBACK;
						EXEC SQL DISCONNECT;
						EXEC SQL SET_SQL (SESSION = :session1);
						EXEC SQL ROLLBACK;
						free_sqlda(sqlda, column_count);
						return (err);
					}
					STprintf(tmp, "	%d", cdds_no);
					STcopy(tmp,file_dataend);
					file_dataend+=STlength(tmp);
					STprintf(tmp, "	%d\n", dd_priority);
					STcopy(tmp,file_dataend);
					file_dataend+=STlength(tmp);
					SIwrite(file_dataend-file_data,file_data,&count,shadow_file);
					if (queue_flag)
						SIfprintf(queue_file,
							ERx("%d	%d	%d	%d	%d	%d	%d	%s	%d	%d	%d\n"),
							sourcedb, transaction_id, i,
							RMtbl->table_no, 0, 0, 0,
							timestamp, 1, cdds_no,
							dd_priority);
					++rows_processed;
				}
			}
			if (found)
			{
				SIclose(shadow_file);
				EXEC SQL SET_SQL (SESSION = :session2);

				/* Initialize adf_ucollation to load unicode data */
				EXEC SQL SELECT dbmsinfo('UNICODE_NORMALIZATION') INTO :normbuf;
				if (STlength(normbuf) >= 3)
				{
				    if (STbcompare( normbuf, 3, "NFC",3,0) == 0)
				    {
					IILQucolinit(1);
				    }
				    else if (STbcompare( normbuf, 3, "NFD",3,0) == 0)
				    {
					IILQucolinit(0);
				    }
				}

				EXEC SQL EXECUTE IMMEDIATE :copy_stmt;
				EXEC SQL INQUIRE_SQL (:err = ERRORNO);
				if (err)
				{
					IIUGerr(E_RM0131_Err_copy_from, UG_ERR_ERROR,
						2, RMtbl->sha_name, shadow_filename);
					EXEC SQL ROLLBACK;
					EXEC SQL DISCONNECT;
					EXEC SQL SET_SQL (SESSION = :session1);
					EXEC SQL ROLLBACK;
					free_sqlda(sqlda, column_count);
					return (E_RM0131_Err_copy_from);
				}
				if (SIfopen(&sh_loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC,
					&shadow_file) != OK)
				{
					IIUGerr(E_RM005E_Error_open_file, UG_ERR_ERROR,
						1, shadow_filename);
					EXEC SQL ROLLBACK;
					EXEC SQL DISCONNECT;
					EXEC SQL SET_SQL (SESSION = :session1);
					EXEC SQL ROLLBACK;
					free_sqlda(sqlda, column_count);
					return (E_RM005E_Error_open_file);
				}
				if (queue_flag)
				{
					SIclose(queue_file);
					EXEC SQL EXECUTE IMMEDIATE :qcopy_stmt;
					EXEC SQL INQUIRE_SQL (:err = ERRORNO);
					if (err)
					{
						IIUGerr(E_RM0131_Err_copy_from,
							UG_ERR_ERROR, 2,
							ERx("dd_input_queue"),
							queue_filename);
						EXEC SQL ROLLBACK;
						EXEC SQL DISCONNECT;
						EXEC SQL SET_SQL (SESSION = :session1);
						EXEC SQL ROLLBACK;
						free_sqlda(sqlda, column_count);
						return (E_RM0131_Err_copy_from);
					}
					if (SIfopen(&iq_loc, ERx("w"), SI_TXT,
						SI_MAX_TXT_REC, &queue_file) != OK)
					{
						IIUGerr(E_RM005E_Error_open_file,
							UG_ERR_ERROR, 1,
							queue_filename);
						EXEC SQL ROLLBACK;
						EXEC SQL DISCONNECT;
						EXEC SQL SET_SQL (SESSION = :session1);
						EXEC SQL ROLLBACK;
						free_sqlda(sqlda, column_count);
						return (E_RM005E_Error_open_file);
					}
				}

				IIUGmsg(ERget(F_RM00DE_Rows_completed), FALSE, 2,
					&rows_processed, &approx_rows);
				EXEC SQL COMMIT;
			}
		}
		SIclose(shadow_file);
		LOdelete(&sh_loc);
		if (queue_flag)
		{
			SIclose(queue_file);
			LOdelete(&iq_loc);
		}
		EXEC SQL SET_SQL (SESSION = :session2);
		EXEC SQL DISCONNECT SESSION :session2;
		EXEC SQL SET_SQL (SESSION = :session1);
		EXEC SQL CLOSE c1;
		EXEC SQL COMMIT;
		free_sqlda(sqlda, column_count);

		if (!shadow_rows)
		{
			err = RMmodify_shadow(RMtbl, TRUE);
			if (err)
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM0129_Err_modifying, UG_ERR_ERROR, 1,
					RMtbl->sha_name);
				return (E_RM0129_Err_modifying);
			}
			EXEC SQL COMMIT;
			STprintf(stmt, ERx("SET JOURNALING ON %s"), RMtbl->sha_name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				IIUGerr(E_RM0134_Err_set_journal, UG_ERR_ERROR, 2,
					RMtbl->table_owner, RMtbl->sha_name);
				EXEC SQL ROLLBACK;
				return (E_RM0134_Err_set_journal);
			}
			EXEC SQL COMMIT;
		}
		if (queue_flag && (input_rows < MODIFY_TABLE_LIMIT))
		{
			if (RMmodify_input_queue() != OK)
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM0129_Err_modifying, UG_ERR_ERROR, 1,
					ERx("dd_input_queue"));
				return (E_RM0129_Err_modifying);
			}
			EXEC SQL COMMIT;
		}

		if (shadow_rows)
		{
			STprintf(stmt, ERx("DROP TABLE SESSION.%s"), tmp_table);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			EXEC SQL INQUIRE_SQL (:err = ERRORNO);
			if (err)
			{
				EXEC SQL ROLLBACK;
				IIUGerr(E_RM0128_Err_dropping, UG_ERR_ERROR, 1,
					tmp_table);
				return (E_RM0128_Err_dropping);
			}
			EXEC SQL COMMIT;
		}
		break;
	}
	
	if (stat)
	{
		IIUGerr(E_RM0135_Err_alloc_ucode_key, UG_ERR_ERROR, 1,
					tmp_table);
		return (E_RM0135_Err_alloc_ucode_key);
	}
	
	/* clean up the unicode values if assigned */
	for (i=0; i<DB_MAX_COLS; i++)
	{
	  if (nch_val[i])
	    MEfree (nch_val[i]);
	}

	if (nch_val)
	  MEfree (nch_val);

	if (nvch_val)
	  MEfree (nvch_val);
	
	if (column_name)
	  MEfree (column_name);

	return (OK);
}


/*{
** Name:	free_sqlda - free SQLDA
**
** Description:
**	free allocated space for the sqlda
**
** Inputs:
**	sqlda		- the SQLDA
**	column_count	- the number of columns allocated
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
free_sqlda(
IISQLDA	*sqlda,
i4	column_count)
{
	i4 i;

	for (i = 0; i < column_count; i++)
		MEfree(sqlda->sqlvar[i].sqldata);
}


/*{
** Name:	get_cdds_no - get CDDS number
**
** Description:
**	Lookup a row's cdds_no if there is a cdds_lookup_table.  Else return
**	the default for the table.
**
** Inputs:
**	wc		- where_clause
**
** Outputs:
**	cdds_no		- CDDS number
**
** Returns:
**	?
*/
static STATUS
get_cdds_no(
char	*wc,
i2	*cdds_no)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i2	cdds_num;
	STATUS	err = 0;
	char	select_string[MAX_SIZE];
	char	col_name[DB_MAXNAME+1];
	i4	column_number = 0;
	i4	row_count;
	EXEC SQL END DECLARE SECTION;
	char	expand_string[512];

	*cdds_no = RMtbl->cdds_no;
	*expand_string = EOS;
	if (*RMtbl->cdds_lookup_table != EOS)
	{
		/* build where clause */
		EXEC SQL SELECT TRIM(column_name)
			INTO	:col_name
			FROM	iicolumns
			WHERE	table_name = :RMtbl->cdds_lookup_table
			AND	table_owner = DBMSINFO('dba')
			AND	column_name != 'cdds_no'
			AND	column_name != 'dd_priority';
		EXEC SQL BEGIN;
			if (column_number)
				STcat(expand_string, ERx(" AND "));
			STcat(expand_string, ERx("r."));
			STcat(expand_string, RPedit_name(EDNM_DELIMIT,
				col_name, NULL));
			STcat(expand_string, ERx(" = "));
			STcat(expand_string, ERx("t."));
			STcat(expand_string, RPedit_name(EDNM_DELIMIT,
				col_name, NULL));
			column_number++;
		EXEC SQL END;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0132_Err_rtrv_col_name, UG_ERR_ERROR, 0);
			return (E_RM0132_Err_rtrv_col_name);
		}
		STprintf(select_string, ERx("SELECT t.cdds_no FROM %s t, \
%s.%s r WHERE %s AND %s"),
			RMtbl->cdds_lookup_table, RMtbl->dlm_table_owner,
			RMtbl->dlm_table_name, expand_string, wc);
		EXEC SQL EXECUTE IMMEDIATE :select_string INTO :cdds_num;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO, :row_count = ROWCOUNT);
		if (err)
		{
			IIUGerr(E_RM0133_Err_lookup_value, UG_ERR_ERROR, 2,
				ERx("cdds_no"), RMtbl->cdds_lookup_table);
			return (E_RM0133_Err_lookup_value);
		}
		if (row_count)
			*cdds_no = cdds_num;
	}

	return (OK);
}


/*{
** Name:	get_dd_priority - get priority
**
** Description:
**	get a rows' dd_priority if a priority lookup table is defined.
**
** Inputs:
**	wc - where clause of the row
**
** Outputs:
**	dd_priority - priority of the row
**
** Returns:
**	non-zero - error number, if error occurred
**	0	- if no error encountered
*/
static STATUS
get_dd_priority(
char	*wc,
i2	*dd_priority)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i2	dd_prio;
	STATUS	err = 0;
	char	select_string[MAX_SIZE];
	char	col_name[DB_MAXNAME+1];
	i4	column_number = 0;
	i4	row_count;
	EXEC SQL END DECLARE SECTION;
	char	expand_string[512];

	*dd_priority = 0;
	*expand_string = EOS;
	if (*RMtbl->prio_lookup_table != EOS)
	{
		/* build where clause */
		EXEC SQL SELECT TRIM(column_name)
			INTO	:col_name
			FROM	iicolumns
			WHERE	table_name = :RMtbl->prio_lookup_table
			AND	table_owner = DBMSINFO('dba')
			AND	column_name != 'cdds_no'
			AND	column_name != 'dd_priority';
		EXEC SQL BEGIN;
			if (column_number)
				STcat(expand_string, ERx(" AND "));
			STcat(expand_string, ERx("r."));
			STcat(expand_string, RPedit_name(EDNM_DELIMIT,
				col_name, NULL));
			STcat(expand_string, ERx(" = "));
			STcat(expand_string, ERx("t."));
			STcat(expand_string, RPedit_name(EDNM_DELIMIT,
				col_name, NULL));
			column_number++;
		EXEC SQL END;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			IIUGerr(E_RM0132_Err_rtrv_col_name, UG_ERR_ERROR, 0);
			return (E_RM0132_Err_rtrv_col_name);
		}
		STprintf(select_string, ERx("SELECT r.dd_priority FROM %s r, \
%s.%s t WHERE %s AND %s"),
			RMtbl->prio_lookup_table, RMtbl->dlm_table_owner,
			RMtbl->dlm_table_name, expand_string, wc);
		EXEC SQL EXECUTE IMMEDIATE :select_string INTO :dd_prio;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO, :row_count = ROWCOUNT);
		if (err)
		{
			IIUGerr(E_RM0133_Err_lookup_value, UG_ERR_ERROR, 2,
				ERx("dd_priority"), RMtbl->prio_lookup_table);
			return (E_RM0133_Err_lookup_value);
		}
		if (row_count)
			*dd_priority = dd_prio;
	}

	return (OK);
}

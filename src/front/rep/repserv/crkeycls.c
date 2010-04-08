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
# include <iiapi.h>
# include <sapiw.h>
# include <rpu.h>
# include "conn.h"
# include "tblinfo.h"
# include "distq.h"

/**
** Name:	crkeycls.c - create key clauses
**
** Description:
**	Defines
**		RScreate_key_clauses	- create key clauses
**		apostrophe		- double apostrophes in a string
**
** History:
**	30-dec-96 (joea)
**		Created based on crkeycls.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to create_key_clauses.
**	27-may-98 (joea)
**		Convert to OpenAPI for RepServer use only.
**	08-jul-98 (abbjo03)
**		Remove sha_name argument and use shadow_table member. Add new
**		arguments to IIsw_selectSingleton.
**	22-jul-98 (abbjo03)
**		Replace dlm_col_name by col_name.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	01-nov-2002 (abbjo03)
**	    Initialize column_value and cdata.dv_null.
**	29-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw functions.
**      06-nov-2009 (joea)
**          Add handling of IIAPI_BOOL_TYPE in RScreate_key_clauses.
**/

static char	*apos = ERx("\'");
static char	decimal_char = EOS;
static char	*common_where_clause = ERx(" WHERE sourcedb = ~V AND \
	transaction_id = ~V AND sequence_no = ~V");


static void apostrophe(char *string);


FUNC_EXTERN ADF_CB	*FEadfcb(void);


/*{
** Name:	RScreate_key_clauses - create key clauses
**
** Description:
**	Creates clauses of SQL statements for a given transaction (for
**	instance, builds the where clause using the data in the base table that
**	corresponds to the replication key).
**
** Inputs:
**	conn		- connection descriptor
**	tbl		- table descriptor
**	trans		- transaction key
**
** Outputs:
**	where_clause		-
**	name_clause		-
**	insert_value_clause	-
**	update_clause		-
**
** Returns:
**	OK or ?
**/
STATUS
RScreate_key_clauses(
RS_CONN		*conn,
RS_TBLDESC	*tbl,
RS_TRANS_KEY	*trans,
char		*where_clause,
char		*name_clause,
char		*insert_value_clause,
char		*update_clause)
{
	char			column_value[2000];
	i4			rows;
	char			stmt[512];
	i4			int_val;
	f8			flt_val;
	struct
	{
		short	len;
		char	text[2000];
	} vch_val;
	RS_COLDESC		*col;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	if (decimal_char == EOS)
		decimal_char = (char)FEadfcb()->adf_decimal.db_decimal;

	*column_value = EOS;
	if (where_clause)
		*where_clause = EOS;
	if (name_clause)
		*name_clause = EOS;
	if (insert_value_clause)
		*insert_value_clause = EOS;
	if (update_clause)
		*update_clause = EOS;

	SW_QRYDESC_INIT(pdesc[0], IIAPI_INT_TYPE, sizeof(trans->src_db), FALSE);
	SW_QRYDESC_INIT(pdesc[1], IIAPI_INT_TYPE, sizeof(trans->trans_id),
		FALSE);
	SW_QRYDESC_INIT(pdesc[2], IIAPI_INT_TYPE, sizeof(trans->seq_no), FALSE);
	SW_PARMDATA_INIT(pdata[0], &trans->src_db, sizeof(trans->src_db),
		FALSE);
	SW_PARMDATA_INIT(pdata[1], &trans->trans_id, sizeof(trans->trans_id),
		FALSE);
	SW_PARMDATA_INIT(pdata[2], &trans->seq_no, sizeof(trans->seq_no),
		FALSE);
	for (col = tbl->cols; col < tbl->cols + tbl->num_regist_cols; ++col)
	{
		if (!col->key_sequence)
			continue;
		cdata.dv_null = FALSE;
		switch (col->coldesc.ds_dataType)
		{
		case IIAPI_INT_TYPE:
			STprintf(stmt, ERx("SELECT INT4(%s) FROM %s"),
				col->col_name, tbl->shadow_table);
			STcat(stmt, common_where_clause);
			SW_COLDATA_INIT(cdata, int_val);
			if (IIsw_selectSingleton(conn->connHandle,
				&conn->tranHandle, stmt, 3, pdesc, pdata, 1,
				&cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
				&errParm) != IIAPI_ST_SUCCESS)
			{
				/* integer execute immediate failed */
				messageit(1, 1349, errParm.ge_errorCode);
				IIsw_close(stmtHandle, &errParm);
				return (errParm.ge_errorCode);
			}
			rows = cdata.dv_null ? 0 : IIsw_getRowCount(&getQinfoParm);
			if (rows > 0)
				STprintf(column_value, ERx("%d"), int_val);
			break;

		case IIAPI_FLT_TYPE:
		case IIAPI_MNY_TYPE:
			STprintf(stmt, ERx("SELECT FLOAT8(%s) FROM %s"),
				col->col_name, tbl->shadow_table);
			STcat(stmt, common_where_clause);
			SW_COLDATA_INIT(cdata, flt_val);
			if (IIsw_selectSingleton(conn->connHandle,
				&conn->tranHandle, stmt, 3, pdesc, pdata, 1,
				&cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
				&errParm) != IIAPI_ST_SUCCESS)
			{
				/* execute immediate failed */
				messageit(1, 1350, errParm.ge_errorCode);
				IIsw_close(stmtHandle, &errParm);
				return (errParm.ge_errorCode);
			}
			rows = cdata.dv_null ? 0 : IIsw_getRowCount(&getQinfoParm);
			if (rows > 0)
				STprintf(column_value, ERx("%.13g"), flt_val,
					decimal_char);
			break;

		default:
			STprintf(stmt, ERx("SELECT VARCHAR(%s) FROM %s"),
				col->col_name, tbl->shadow_table);
			STcat(stmt, common_where_clause);
			SW_COLDATA_INIT(cdata, vch_val);
			if (IIsw_selectSingleton(conn->connHandle,
				&conn->tranHandle, stmt, 3, pdesc, pdata, 1,
				&cdata, NULL, NULL, &stmtHandle, &getQinfoParm,
				&errParm) != IIAPI_ST_SUCCESS)
			{
				/* char execute immediate failed */
				messageit(1, 1351, errParm.ge_errorCode);
				IIsw_close(stmtHandle, &errParm);
				return (errParm.ge_errorCode);
			}
			rows = cdata.dv_null ? 0 : IIsw_getRowCount(&getQinfoParm);
			if (rows > 0)
			{
                            if (col->coldesc.ds_dataType == IIAPI_BOOL_TYPE)
                            {
                                SW_VCH_TERM(&vch_val);
                                STprintf(column_value, "%s", vch_val.text);
                            }
                            else
                            {
				SW_VCH_TERM(&vch_val);
				apostrophe(vch_val.text);
				STprintf(column_value, ERx("'%s'"),
					vch_val.text);
                            }
			}
		}
		IIsw_close(stmtHandle, &errParm);
		if (where_clause)
		{
			if (*where_clause != EOS)
				STcat(where_clause, ERx(" AND "));
			STcat(where_clause, ERx("t."));
			STcat(where_clause, col->col_name);
			if (!rows)
			{
				STcat(where_clause, ERx(" IS NULL "));
			}
			else
			{
				STcat(where_clause, ERx(" = "));
				STcat(where_clause, column_value);
			}
		}
		if (name_clause)
		{
			if (*name_clause != EOS)
			{
				STcat(name_clause, ERx(", "));
			}
			STcat(name_clause, col->col_name);
		}
		if (insert_value_clause)
		{
			if (*insert_value_clause != EOS)
			{
				STcat(insert_value_clause, ERx(", "));
			}
			STcat(insert_value_clause, column_value);
		}
		if (update_clause)
		{
			if (*update_clause != EOS)
			{
				STcat(update_clause, ERx(", "));
			}
			STcat(update_clause, col->col_name);
			STcat(update_clause, ERx(" = "));
			STcat(update_clause, column_value);
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

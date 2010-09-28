/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <si.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <rpu.h>
# include "distq.h"
# include "tblinfo.h"
# include "repserv.h"

/**
** Name:	xmitsupp.c - Replicator Server transmit support routines
**
** Description:
**	Defines
**		RSpdb_PrepDescBuffers	- prepare OpenAPI buffers
**		RSfdb_FreeDescBuffers	- free OpenAPI buffers
**		RSpdp_PrepDbprocParams	- prepare dbproc parameters
**		sdp_SetDbprocParam	- sets a procedure parameter
**		RSiiq_InsertInputQueue	- insert into the input queue
**		RSist_InsertShadowTable	- insert into the shadow table
**
** History:
**	09-dec-97 (joea)
**		Created based on rsu.sc in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	09-jul-98 (abbjo03)
**		Add RSfdb_FreeDescBuffers, RSiiq_InsertInputQueue,
**		RSist_InsertShadowTable. Add two new arguments to IIsw_query.
**	22-jul-98 (abbjo03)
**		Allow for delimited identifiers in paramNames parameter to
**		RSpdp_PrepDbprocParams.
**	30-jul-98 (abbjo03)
**		Change RSiiq_InsertInputQueue to a repeated query.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	21-apr-1999 (hanch04)
**	    Replace STrindex with STrchr
**	21-jul-99 (abbjo03)
**		Replace formula for maximum length of a delimited identifier by
**		DB_MAX_DELIMID.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-oct-2002 (padni01) bug 106498
**		Allocate temporary buffer stmt in RSist_InsertShadowTable(),
**		based on the size of col_list and val_list
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw functions.
**	05-mar-2008 (gupsh01)
**	    Initialize fields in RSist_InsertShadowTable to avoid corrupt 
**	    data in UTF8 instance.
**      10-mar-2008 (stial01)
**          Init ttime fields to avoid unorm errors if UTF8 (b121769)
**      17-Aug-2010 (thich01)
**          Add geospatial types to IIAPI types.  Make changes to treat spatial
**          types like LBYTEs.
**/

static II_PTR iiq_qryHandle;


static void sdp_SetDbprocParam(IIAPI_DESCRIPTOR *col, IIAPI_DATAVALUE *dv,
	bool old_data, char *paramName, IIAPI_DESCRIPTOR *paramDesc,
	IIAPI_DATAVALUE *paramValue);


/*{
** Name:	RSpdb_PrepDescBuffers - prepare OpenAPI buffers
**
** Description:
**	Prepares the OpenAPI buffers of a transaction row by allocating memory
**	for them and copying the table/column cache information into them.
**
** Inputs:
**	row	- transaction row
**
** Outputs:
**	row	- transaction row
**
** Returns:
**	OK
**	RS_MEM_ERR	- memory allocation error
*/
STATUS
RSpdb_PrepDescBuffers(
RS_TRANS_ROW	*row)
{
	RS_COLDESC		*col;
	IIAPI_DESCRIPTOR	*col_desc;
	IIAPI_DESCRIPTOR	*key_desc;

	row->row_desc = (IIAPI_DESCRIPTOR *)MEreqmem(0,
		row->tbl_desc->num_regist_cols * sizeof(IIAPI_DESCRIPTOR), TRUE,
		NULL);
	if (row->row_desc == NULL)
		return (RS_MEM_ERR);
	row->key_desc = (IIAPI_DESCRIPTOR *)MEreqmem(0,
		row->tbl_desc->num_key_cols * sizeof(IIAPI_DESCRIPTOR), TRUE,
		NULL);
	if (row->key_desc == NULL)
		return (RS_MEM_ERR);

	if (row->data_len == 0)
		row->data_len = row->tbl_desc->num_regist_cols *
			sizeof(IIAPI_DATAVALUE) + RS_ROW_BUF_LEN_DFLT;
	if (row->key_len == 0)
		row->key_len = row->tbl_desc->num_key_cols *
			sizeof(IIAPI_DATAVALUE) + RS_ROW_BUF_LEN_DFLT;
	row->row_data = (IIAPI_DATAVALUE *)MEreqmem(0, row->data_len, TRUE,
		NULL);
	if (row->row_data == NULL)
		return (RS_MEM_ERR);
	row->key_data = (IIAPI_DATAVALUE *)MEreqmem(0, row->key_len, TRUE,
		NULL);
	if (row->key_data == NULL)
		return (RS_MEM_ERR);

	col_desc = row->row_desc;
	key_desc = row->key_desc;
	for (col = row->tbl_desc->cols; col < row->tbl_desc->cols +
		row->tbl_desc->num_regist_cols; ++col)
	{
		MEcopy((PTR)&col->coldesc, (u_i2)sizeof(IIAPI_DESCRIPTOR),
			col_desc++);
		if (col->key_sequence)
			MEcopy((PTR)&col->coldesc,
				(u_i2)sizeof(IIAPI_DESCRIPTOR), key_desc++);
	}

	return (OK);
}


/*{
** Name:	RSfdb_FreeDescBuffers - free OpenAPI buffers
**
** Description:
**	Frees the OpenAPI buffers allocated by RSpdb_PrepDescBuffers.
**
** Inputs:
**	row	- transaction row
**
** Outputs:
**	row	- transaction row
**
** Returns:
**	OK
*/
STATUS
RSfdb_FreeDescBuffers(
RS_TRANS_ROW	*row)
{
	if (row->key_data)
		MEfree((PTR)row->key_data);	
	if (row->row_data && row->tbl_desc->has_blob)
	{
		RS_COLDESC	*col;
		IIAPI_DATAVALUE	*dv = row->row_data;

		for (col = row->tbl_desc->cols; col < row->tbl_desc->cols +
			row->tbl_desc->num_regist_cols; ++col, ++dv)
		{
			if (col->coldesc.ds_dataType == IIAPI_LVCH_TYPE ||
				col->coldesc.ds_dataType == IIAPI_GEOM_TYPE ||
				col->coldesc.ds_dataType == IIAPI_POINT_TYPE ||
				col->coldesc.ds_dataType == IIAPI_MPOINT_TYPE ||
				col->coldesc.ds_dataType == IIAPI_LINE_TYPE ||
				col->coldesc.ds_dataType == IIAPI_MLINE_TYPE ||
				col->coldesc.ds_dataType == IIAPI_POLY_TYPE ||
				col->coldesc.ds_dataType == IIAPI_MPOLY_TYPE ||
				col->coldesc.ds_dataType == IIAPI_GEOMC_TYPE ||
				col->coldesc.ds_dataType == IIAPI_LBYTE_TYPE &&
					dv->dv_value)
			{
				MEfree((PTR)dv->dv_value);
				dv->dv_value = NULL;
			}
		}
	}
	if (row->row_data)
		MEfree((PTR)row->row_data);	
	if (row->key_desc)
		MEfree((PTR)row->key_desc);	
	if (row->row_desc)
		MEfree((PTR)row->row_desc);	

	row->row_data = row->key_data = NULL;
	row->row_desc = row->key_desc = NULL;
	row->data_len = row->key_len = 0;
	return (OK);
}


/*{
** Name:	RSpdp_PrepDbprocParams - prepare dbproc parameters
**
** Description:
**	Prepares an procedure parameters for a remote insert, update, or delete
**	execute procedure call.
**
** Inputs:
**	proc_name	- procedure name
**	target		- target database and CDDS
**	tbl		- the table description
**	row		- the distribution queue row data record
**
** Outputs:
**	paramName	- pointer to array of parameter names
**	paramDesc	- pointer to IIAPI_DESCRIPTOR array
**	paramValue	- pointer to IIAPI_DATAVALUE array
**
** Returns:
**	OK
**	FAIL
*/
STATUS
RSpdp_PrepDbprocParams(
char			*proc_name,
RS_TARGET		*target,
RS_TBLDESC		*tbl,
RS_TRANS_ROW		*row,
char			(*paramName)[DB_MAX_DELIMID+1],
II_INT2			*paramCount,
IIAPI_DESCRIPTOR	*paramDesc,
IIAPI_DATAVALUE		*paramValue)
{
	IIAPI_DESCRIPTOR	*col;
	IIAPI_DATAVALUE		*dv;
	i2			i;

	if (row->trans_type < RS_INSERT || row->trans_type > RS_DELETE)
	{
		messageit(1, 1080, row->trans_type, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		return (RS_INTERNAL_ERR);
	}

	i = 0;
	paramDesc[i].ds_dataType = IIAPI_CHA_TYPE;
	paramDesc[i].ds_nullable = paramValue[i].dv_null = FALSE;
	paramDesc[i].ds_length = paramValue[i].dv_length = STlength(proc_name);
	paramDesc[i].ds_precision = paramDesc[i].ds_scale = 0;
	paramDesc[i].ds_columnType = IIAPI_COL_SVCPARM;
	paramDesc[i].ds_columnName = NULL;
	paramValue[i++].dv_value = proc_name;

	if (row->trans_type == RS_INSERT || row->trans_type == RS_UPDATE)
	{
		for (col = row->row_desc, dv = row->row_data;
				col < row->row_desc + tbl->num_regist_cols;
				++col, ++dv, ++i)
			sdp_SetDbprocParam(col, dv, FALSE, paramName[i],
				&paramDesc[i], &paramValue[i]);
	}

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->rep_key.src_db,
		ERx("sourcedb"));
	SW_PROCDATA_INIT(paramValue[i], row->rep_key.src_db);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->rep_key.trans_id,
		ERx("transaction_id"));
	SW_PROCDATA_INIT(paramValue[i], row->rep_key.trans_id);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->rep_key.seq_no,
		ERx("sequence_no"));
	SW_PROCDATA_INIT(paramValue[i], row->rep_key.seq_no);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->table_no,
		ERx("table_no"));
	SW_PROCDATA_INIT(paramValue[i], row->table_no);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE,
		row->old_rep_key.src_db, ERx("old_sourcedb"));
	SW_PROCDATA_INIT(paramValue[i], row->old_rep_key.src_db);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE,
		row->old_rep_key.trans_id, ERx("old_transaction_id"));
	SW_PROCDATA_INIT(paramValue[i], row->old_rep_key.trans_id);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE,
		row->old_rep_key.seq_no, ERx("old_sequence_no"));
	SW_PROCDATA_INIT(paramValue[i], row->old_rep_key.seq_no);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, target->cdds_no,
		ERx("cdds_no"));
	SW_PROCDATA_INIT(paramValue[i], target->cdds_no);
	++i;

	if (row->trans_type == RS_INSERT || row->trans_type == RS_UPDATE)
	{
		SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->priority,
			ERx("dd_priority"));
		SW_PROCDATA_INIT(paramValue[i], row->priority);
		++i;
	}

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_CHA_TYPE, row->trans_time,
		ERx("trans_time"));
	SW_PROCDATA_INIT(paramValue[i], row->trans_time);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->trans_type,
		ERx("trans_type"));
	SW_PROCDATA_INIT(paramValue[i], row->trans_type);
	++i;

	SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, target->type,
		ERx("target_type"));
	SW_PROCDATA_INIT(paramValue[i], target->type);
	++i;

	if (row->trans_type == RS_UPDATE || row->trans_type == RS_DELETE)
	{
		SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE,
			row->old_cdds_no, ERx("s_cdds_no"));
		SW_PROCDATA_INIT(paramValue[i], row->old_cdds_no);
		++i;

		SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE,
			row->old_priority, ERx("s_dd_priority"));
		SW_PROCDATA_INIT(paramValue[i], row->old_priority);
		++i;

		SW_PROCDESC_INIT(paramDesc[i], IIAPI_CHA_TYPE,
			row->old_trans_time, ERx("s_trans_time"));
		SW_PROCDATA_INIT(paramValue[i], row->old_trans_time);
		++i;

		SW_PROCDESC_INIT(paramDesc[i], IIAPI_INT_TYPE, row->new_key,
			ERx("s_new_key"));
		SW_PROCDATA_INIT(paramValue[i], row->new_key);
		++i;

		for (col = row->key_desc, dv = row->key_data;
				col < row->key_desc + tbl->num_key_cols;
				++col, ++dv, ++i)
			sdp_SetDbprocParam(col, dv, TRUE, paramName[i],
				&paramDesc[i], &paramValue[i]);
	}

	*paramCount = i;
	return (OK);
}


/*{
** Name:	sdp_SetDbprocParam - sets a procedure parameter
**
** Description:
**	Sets the IIAPI_DESCRIPTOR and IIAPI_DATAVALUE for a database procedure.
**
** Inputs:
**	col		- pointer to column descriptor
**	dv		- pointer to data value descriptor
**	old_data	- indicates whether old data should be set up
**
** Outputs:
**	paramName	- pointer to parameter name
**	paramDesc	- pointer to parameter IIAPI_DESCRIPTOR
**	paramValue	- pointer to parameter IIAPI_DATAVALUE
**
** Returns:
**	none
*/
static void
sdp_SetDbprocParam(
IIAPI_DESCRIPTOR	*col,
IIAPI_DATAVALUE		*dv,
bool			old_data,
char			*paramName,
IIAPI_DESCRIPTOR	*paramDesc,
IIAPI_DATAVALUE		*paramValue)
{
	char	*s;

	*paramDesc = *col;	/* struct */
	paramDesc->ds_columnType = IIAPI_COL_PROCPARM;
	if (old_data)
	{
		STcopy(ERx("_ii_"), paramName);
		s = col->ds_columnName;
		CMnext(s);	/* skip the leading quote character */
		STcat(paramName, s);
		s = STrchr(paramName, '\"');
		if (s)
			*s = EOS;	/* get rid of trailing quote */
		RPedit_name(EDNM_DELIMIT, paramName, paramName);
	}
	else
	{
		STcopy(col->ds_columnName, paramName);
	}
	paramDesc->ds_columnName = paramName;
	*paramValue = *dv;	/* struct */
}


/*{
** Name:	RSiiq_InsertInputQueue - insert into the input queue
**
** Description:
**	Inserts a row into the input queue.
**
** Inputs:
**	conn		- connection descriptor
**	tbl		- table descriptor
**	trans_type	- transaction type
**	rep_key		- replicated transaction key
**	old_rep_key	- previous replicated transaction key
**	cdds_no		- CDDS number
**	priority	- priority
**	trans_time	- transaction time
**	check_collision	- check for collision?
**
** Outputs:
**	none
**
** Returns:
**	status from RSerror_check
*/
STATUS
RSiiq_InsertInputQueue(
RS_CONN		*conn,
RS_TBLDESC	*tbl,
i2		trans_type,
RS_TRANS_KEY	*rep_key,
RS_TRANS_KEY	*old_rep_key,
i2		cdds_no,
i2		priority,
char		*trans_time,
bool		check_collision)
{
	i4			i;
	char 			ttime[26];
	IIAPI_DESCRIPTOR	pdesc[11];
	IIAPI_DATAVALUE		pdata[11];
	II_PTR			stmtHandle;
	STATUS			status;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("RSiiq_InsertInputQueue") };

	/* Null out the ttime values to avoid 
	** errors possible due to corrupt data in UTF8 server.
	*/
	MEfill (sizeof(ttime), 0, &ttime);

	STcopy(trans_time, ttime);
	i = 0;
	TRANS_KEY_DESC_INIT(&pdesc[i], &pdata[i], *rep_key);
	i += 3;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, trans_type);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		tbl->table_no);
	++i;
	TRANS_KEY_DESC_INIT(&pdesc[i], &pdata[i], *old_rep_key);
	i += 3;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_CHA_TYPE, ttime);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, cdds_no);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, priority);
	status = IIsw_queryRepeated(conn->connHandle, &conn->tranHandle,
		ERx("INSERT INTO dd_input_queue (sourcedb, \
transaction_id, sequence_no, trans_type, table_no, old_sourcedb, \
old_transaction_id, old_sequence_no, trans_time, cdds_no, dd_priority) VALUES \
( $0 = ~V , $1 = ~V , $2 = ~V , $3 = ~V , $4 = ~V , $5 = ~V , $6 = ~V , \
$7 = ~V , $8 = ~V , $9 = ~V , $10 = ~V )"),
		11, pdesc, pdata, &qryId, &iiq_qryHandle, &stmtHandle,
		 &getQinfoParm, &errParm);
	status = RSerror_check(1230, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL, rep_key->src_db, rep_key->trans_id, 
		rep_key->seq_no, conn->db_no, tbl->table_name);
	if (check_collision && STequal(errParm.ge_SQLSTATE,
			II_SS23000_CONSTR_VIOLATION))
		status = OK;
	return (status);
}


/*{
** Name:	RSist_InsertShadowTable - insert into the shadow table
**
** Description:
**	Inserts a row into the shadow table.  The arrays of column and data
**	descriptors should be pre-filled with key column information.
**
** Inputs:
**	conn		- connection descriptor
**	tbl		- table descriptor
**	trans_type	- transaction type
**	rep_key		- replicated transaction key
**	old_rep_key	- previous replicated transaction key
**	cdds_no		- CDDS number
**	priority	- priority
**	in_archive	- is transaction archived?
**	new_key		- new primary key?
**	trans_time	- transaction time
**	distrib_time	- distribution time
**	col_list	- column list
**	val_list	- value list
**	pdesc		- array of column descriptors
**	pdata		- array of data descriptors
**
** Outputs:
**	none
**
** Returns:
**	status from RSerror_check
*/
STATUS
RSist_InsertShadowTable(
RS_CONN			*conn,
RS_TBLDESC		*tbl,
i2			trans_type,
RS_TRANS_KEY		*rep_key,
RS_TRANS_KEY		*old_rep_key,
i2			cdds_no,
i2			priority,
i1			in_archive,
i2			new_key,
char			*trans_time,
char			*distrib_time,
char			*col_list,
char			*val_list,
IIAPI_DESCRIPTOR	pdesc[],
IIAPI_DATAVALUE		pdata[])
{
	i4			i;
	char 			*stmt;
	RS_TRANS_KEY		old_key;
	char 			ttime[26];
	char 			dtime[26];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	IIAPI_GETQINFOPARM	getQinfoParm;

	/* Null out the dtime and ttime values to avoid 
	** errors possible due to corrupt data in UTF8 server.
	*/
	MEfill (sizeof(ttime), 0, &ttime);
	MEfill (sizeof(dtime), 0, &dtime);

	stmt = (char *)MEreqmem((u_i4)0, (u_i4)(STlength(col_list) + STlength(val_list) + 512), TRUE, (STATUS *)NULL);
	if (stmt == NULL)
	{
		/* Error allocating stmt */
		messageit(1, 1900);
		return (FAIL);
	}

	if (old_rep_key)
		old_key = *old_rep_key;		/* struct */
	else
		old_key.src_db = (i2)(old_key.trans_id = old_key.seq_no = 0);
	STcopy(trans_time, ttime);
	if (distrib_time)
		STcopy(distrib_time, dtime);
	else
		STcopy(ERx("now"), dtime);
	i = tbl->num_key_cols;
	TRANS_KEY_DESC_INIT(&pdesc[i], &pdata[i], *rep_key);
	i += 3;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_CHA_TYPE, ttime);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_CHA_TYPE, dtime);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, in_archive);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, cdds_no);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, trans_type);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, priority);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE, new_key);
	++i;
	TRANS_KEY_DESC_INIT(&pdesc[i], &pdata[i], old_key);
	STprintf(stmt, ERx("INSERT INTO %s (%s, sourcedb, transaction_id, \
sequence_no, trans_time, distribution_time, in_archive, cdds_no, trans_type, \
dd_priority, new_key, old_sourcedb, old_transaction_id, old_sequence_no) \
VALUES (%s, ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V , ~V )"),
		tbl->shadow_table, col_list, val_list);
	IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
		(II_INT2)(tbl->num_key_cols + 13), pdesc, pdata, NULL, NULL,
		&stmtHandle,  &getQinfoParm, &errParm);
	status = RSerror_check(1229, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm,
		&errParm, NULL, rep_key->src_db, rep_key->trans_id, 
		rep_key->seq_no, conn->db_no, tbl->shadow_table);
	MEfree((PTR)stmt);
	return (status);
}

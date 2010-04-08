/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cv.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <fe.h>
# include <ui.h>
# include <tblobjs.h>
# include <targtype.h>
# include "conn.h"
# include "tblinfo.h"
# include "distq.h"
# include "repserv.h"
# include "rsstats.h"
# include "repevent.h"

/**
** Name:	rsupdate.c - propagate an UPDATE
**
** Description:
**	Defines
**		RSupdate	- propagate an UPDATE row
**
** History:
**	11-dec-97 (joea)
**		Created based on dupdate.sc in replicator library and on
**		ddpquery.c and qryfuncs.tlp in ingres library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	09-jul-98 (abbjo03)
**		Add support for blob datatypes.
**	22-jul-98 (abbjo03)
**		If the FP dbproc returns non-zero, return an error. Use remote
**		owner name when accessing remote table. Allow for delimited
**		identifiers in pnames.
**	05-nov-98 (abbjo03)
**		Add call to update statistics.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	26-jan-99 (abbjo03)
**		Add test for URO databases before calling dbprocs.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	06-may-99 (abbjo03)
**		Add special code to allow existing customers in non-SQL92
**		installations to propagate to uppercase Gateway databases.
**	11-may-99 (abbjo03)
**		Supplemental changes to 6-may code for non-SQL92 propagation to
**		uppercase Gateways.
**	21-jul-99 (abbjo03)
**		Replace formula for maximum length of a delimited identifier by
**		DB_MAX_DELIMID.
**	06-Nov-2003 (inifa01) INGREP 108 bug 106498
**		Changed from allocating default MAX SIZE for temp buffers col_list, 
**		where_list, stmt and val_list which was insufficient for large tables 
**		to calling RSmem_allocate() to allocate MAX possible size based on 
**		the replicated tables row_width and number of replicated columns.
**		Call RSmem_free() to free multiple temp buffers.
**	30-mar-2004 (gupsh01)
**		Added getQinfoParm as input parameter to II_sw... functions.
**/

/*{
** Name:	RSupdate	- propagate an UPDATE row
**
** Description:
**	Propagates an UPDATE transaction row, either by calling a remote
**	database procedure or by executing a remote UPDATE.
**
** Inputs:
**	target	- target database and CDDS
**	row	- row of data
**
** Outputs:
**	none
**
** Returns:
**	OK
**	else	- error
*/
STATUS
RSupdate(
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	char 			*stmt=NULL;
	char			*col_list=NULL;
	char			*val_list=NULL;
	char			*where_list=NULL;
	char			*proc_name;
	STATUS			status;
	bool			collision_processed = FALSE;
	RS_TBLDESC		*tbl = row->tbl_desc;
	II_INT2			nparams;
	IIAPI_DESCRIPTOR	pdesc[DB_MAX_COLS*2+1];
	IIAPI_DATAVALUE		pdata[DB_MAX_COLS*2+1];
	char			pnames[DB_MAX_COLS+1][DB_MAX_DELIMID+1];
	RS_CONN			*conn = &RSconns[target->conn_no];
	II_LONG			procRet;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	II_LONG			rowCount;
	IIAPI_DESCRIPTOR 	*col;
	IIAPI_DESCRIPTOR	*cds;
	IIAPI_DATAVALUE		*cdv;
	IIAPI_DATAVALUE		*kdv;
	IISW_CALLBACK		cb = NULL;
	i4			num_cols = tbl->num_regist_cols;
	i4			row_width = tbl->row_width;
	IIAPI_GETQINFOPARM	getQinfoParm;

	messageit(5, 1260, row->rep_key.src_db, row->rep_key.trans_id,
		row->rep_key.seq_no, target->db_no);
	status = RScollision(target, row, tbl, &collision_processed);
	if (status != OK || collision_processed)
		return (status);

	if (tbl->has_blob)
		cb = RSblob_ppCallback;
	col_list = RSmem_allocate(0,num_cols,DB_MAXNAME+3,0); 
	if (col_list == NULL)
		return (FAIL);

	val_list = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,0);
	if (val_list == NULL)
	{
		MEfree((PTR)col_list);
		return (FAIL);
	}
	stmt = RSmem_allocate(2*row_width,num_cols,DB_MAXNAME*2+14,2048);
	if (stmt == NULL)
	{
		RSmem_free(col_list,val_list,NULL,NULL); 
		return (FAIL);
	}
	where_list = RSmem_allocate(row_width,tbl->num_key_cols,2*DB_MAXNAME+14,0); 
	if (where_list == NULL)
	{
		RSmem_free(col_list,val_list,stmt,NULL); 
		return (FAIL);
	}
	/*
	** For FP and PRO targets and tables with blob columns, if the previous
	** transaction had a new key insert a new key row into the shadow.
	*/
	if (target->type != TARG_UNPROT_READ && tbl->has_blob && row->new_key)
	{
		/*
		** Iterate through key_desc to create the column and value
		** lists and prepare the column and value descriptors.
		*/
		*col_list = *val_list = EOS;
		for (col = row->key_desc, cds = pdesc, cdv = pdata, kdv =
			row->key_data; col < row->key_desc + tbl->num_key_cols;
			++col, ++cds, ++kdv)
		{
			if (col != row->key_desc)
			{
				STcat(col_list, ERx(", "));
				STcat(val_list, ERx(","));
			}
			STcat(col_list, col->ds_columnName);
			STcat(val_list, ERx(" ~V "));
			*cds = *col;	/* struct */
			cds->ds_columnType = IIAPI_COL_QPARM;
			*cdv = *kdv;	/* struct */
		}
		/* Add a dummy INSERT to the shadow table */
		status = RSist_InsertShadowTable(conn, tbl, RS_INSERT,
			&row->old_rep_key, NULL, row->old_cdds_no,
			row->old_priority, (i1)0, (i2)1, row->old_trans_time,
			row->old_trans_time, col_list, val_list, pdesc, pdata);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (status);
		}
	}
	/*
	** For FP targets and tables with blobs, archive the previous
	** transaction from the base table.
	*/
	if (tbl->has_blob && target->type == TARG_FULL_PEER)
	{
		/*
		** Iterate through row_desc and prepare the INSERT column list
		** and the SELECT column (value) list.
		*/
		*col_list = *val_list = *where_list = EOS;
		for (col = row->row_desc; col < row->row_desc +
			tbl->num_regist_cols; ++col)
		{
			if (col != row->row_desc)
			{
				STcat(col_list, ERx(", "));
				STcat(val_list, ERx(", "));
			}
			STcat(col_list, col->ds_columnName);
			STcat(val_list, ERx("t."));
			STcat(val_list, col->ds_columnName);
		}
		/*
		** Iterate through key_desc and prepare the WHERE list to join
		** the base and the shadow.
		*/
		for (col = row->key_desc; col < row->key_desc +
			tbl->num_key_cols; ++col)
		{
			if (col != row->key_desc)
				STcat(where_list, ERx(" AND "));
			STcat(where_list, ERx("t."));
			STcat(where_list, col->ds_columnName);
			STcat(where_list, ERx(" = s."));
			STcat(where_list, col->ds_columnName);
		}
		TRANS_KEY_DESC_INIT(pdesc, pdata, row->old_rep_key);
		STprintf(stmt, ERx("INSERT INTO %s (%s, sourcedb, \
transaction_id, sequence_no) SELECT %s, s.sourcedb, s.transaction_id, \
s.sequence_no FROM %s.%s t, %s s WHERE %s AND s.sourcedb = ~V AND \
s.transaction_id = ~V AND s.sequence_no = ~V AND in_archive = 0"),
			tbl->archive_table, col_list, val_list,
			tbl->rem_table_owner, tbl->dlm_table_name,
			tbl->shadow_table, where_list);
		status = IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
			3, pdesc, pdata, NULL, NULL, &stmtHandle, &getQinfoParm,
			&errParm);
		status = RSerror_check(1226, ROWS_SINGLE_ROW, stmtHandle,
			 &getQinfoParm, &errParm, NULL, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no,
			target->db_no, tbl->table_name);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (status);
		}
	}
	/*
	** For URO targets or if the table has blob columns, we update the base
	** row of the remote table.
	*/
	if (target->type == TARG_UNPROT_READ || tbl->has_blob)
	{
		char	objname[DB_MAXNAME*2+3];

		/* updating table %s in database %s. */
		messageit(5, 1148, tbl->table_name, row->rep_key.src_db);
		/*
		** Iterate through row_desc to create the list of SET value
		** assignments and prepare the column and data descriptors. 
		*/
		*val_list = *where_list = EOS;
		for (col = row->row_desc, cds = pdesc, cdv = pdata,
			kdv = row->row_data; col < row->row_desc +
			tbl->num_regist_cols; ++col, ++cds, ++cdv, ++kdv)
		{
			if (col != row->row_desc)
				STcat(val_list, ERx(","));
			/*
			** The following is special case code to allow existing
			** customers in non-SQL92 installations to propagate to
			** Gateway databases that use uppercase table and
			** column names.
			*/
			if (target->type == TARG_UNPROT_READ &&
				conn->name_case == UI_UPPERCASE)
			{
				STcopy(col->ds_columnName, objname);
				CVupper(objname);
				STcat(val_list, objname);
			}
			else
			{
				STcat(val_list, col->ds_columnName);
			}
			STcat(val_list, ERx(" = ~V "));
			*cds = *col;	/* struct */
			cds->ds_columnType = IIAPI_COL_QPARM;
			*cdv = *kdv;	/* struct */
		}
		/*
		** Iterate through key_desc to create the key colum WHERE
		** clause and prepare the column and data descriptors. The data
		** will come directly from key_data.
		*/
		for (col = row->key_desc, kdv = row->key_data;
			col < row->key_desc + tbl->num_key_cols; ++col, ++cds,
			++cdv, ++kdv)
		{
			if (col != row->key_desc)
				STcat(where_list, ERx(" AND "));
			/*
			** Special case for non-SQL92 to uppercase GW databases
			** (see above).
			*/
			if (target->type == TARG_UNPROT_READ &&
				conn->name_case == UI_UPPERCASE)
			{
				STcopy(col->ds_columnName, objname);
				CVupper(objname);
				STcat(where_list, objname);
			}
			else
			{
				STcat(where_list, col->ds_columnName);
			}
			STcat(where_list, ERx(" = ~V"));
			*cds = *col;	/* struct */
			cds->ds_columnType = IIAPI_COL_QPARM;
			*cdv = *kdv;	/* struct */
		}
		STcopy(tbl->dlm_table_name, objname);
		if (target->type == TARG_UNPROT_READ &&
				conn->name_case == UI_UPPERCASE)
			CVupper(objname);
		STprintf(stmt, ERx("UPDATE %s.%s SET %s WHERE %s"),
			tbl->rem_table_owner, objname, val_list, where_list);
		IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
			(II_INT2)(tbl->num_regist_cols + tbl->num_key_cols),
			pdesc, pdata, cb, NULL, &stmtHandle,  &getQinfoParm,
			&errParm);
		status = RSerror_check(1227, ROWS_SINGLE_ROW, stmtHandle,
			 &getQinfoParm, &errParm, &rowCount, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no,
			target->db_no, tbl->table_name);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (status);
		}
		/*
		** For URO targets, if the UPDATE fails, turn it into an INSERT.
		*/
		if (target->type == TARG_UNPROT_READ && !rowCount)
		{
			/*
			** Iterate through row_desc and create the INSERT
			** column and value lists and prepare the column
			** descriptors. The data will come directly from
			** row_data.
			*/
			*col_list = *val_list = EOS;
			for (col = row->row_desc, cds = pdesc;
				col < row->row_desc + tbl->num_regist_cols;
				++col, ++cds)
			{
				if (col != row->row_desc)
				{
					STcat(col_list, ERx(", "));
					STcat(val_list, ERx(","));
				}
				if (target->type == TARG_UNPROT_READ &&
					conn->name_case == UI_UPPERCASE)
				{
					STcopy(col->ds_columnName, objname);
					CVupper(objname);
					STcat(col_list, objname);
				}
				else
				{
					STcat(col_list, col->ds_columnName);
				}
				STcat(val_list, ERx(" ~V "));
				*cds = *col;	/* struct */
				cds->ds_columnType = IIAPI_COL_QPARM;
			}	
			STcopy(tbl->dlm_table_name, objname);
			if (target->type == TARG_UNPROT_READ &&
					conn->name_case == UI_UPPERCASE)
				CVupper(objname);
			STprintf(stmt,
				ERx("INSERT INTO %s.%s (%s) VALUES (%s)"),
				tbl->rem_table_owner, objname, col_list,
				val_list);
			IIsw_query(conn->connHandle, &conn->tranHandle,
				stmt, tbl->num_regist_cols, pdesc,
				row->row_data, cb, NULL, &stmtHandle, 
				&getQinfoParm, &errParm);
			status = RSerror_check(1560, ROWS_SINGLE_ROW,
				stmtHandle,  &getQinfoParm, &errParm, NULL, 
				tbl->table_name);
			if (status != OK)
			{
				RSmem_free(col_list,val_list,stmt,where_list); 
				return (status);
			}
		}
	}
	/*
	** For FP and PRO targets and tables with blobs, update the shadow and
	** the input queue.
	*/
	if (target->type != TARG_UNPROT_READ && tbl->has_blob)
	{
		IIAPI_DATAVALUE	*kdv;

		/*
		** Update the previous shadow row to indicate it has been
		** archived.
		*/
		TRANS_KEY_DESC_INIT(pdesc, pdata, row->old_rep_key);
		STprintf(stmt, ERx("UPDATE %s SET in_archive = 1 WHERE \
in_archive = 0 AND sourcedb = ~V AND transaction_id = ~V AND sequence_no = ~V"),
			tbl->shadow_table);
		status = IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
			3, pdesc, pdata, NULL, NULL, &stmtHandle,  &getQinfoParm,
			&errParm);
		status = RSerror_check(1228, ROWS_SINGLE_ROW, stmtHandle,
			 &getQinfoParm, &errParm, NULL, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no,
			target->db_no, tbl->table_name);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (status);
		}
		/*
		** Iterate through key_desc and create the column and value
		** lists and prepare the column and data descriptors.
		*/
		*col_list = *val_list = EOS;
		for (col = row->key_desc, cds = pdesc, cdv = pdata,
			kdv = row->key_data; col < row->key_desc +
			tbl->num_key_cols; ++col, ++cds, ++cdv, ++kdv)
		{
			if (col != row->key_desc)
			{
				STcat(col_list, ERx(", "));
				STcat(val_list, ERx(","));
			}
			STcat(col_list, col->ds_columnName);
			STcat(val_list, ERx(" ~V "));
			*cds = *col;	/* struct */
			cds->ds_columnType = IIAPI_COL_QPARM;
			*cdv = *kdv;	/* struct */
		}
		/* Insert into the shadow table */
		status = RSist_InsertShadowTable(conn, tbl, row->trans_type,
			&row->rep_key, &row->old_rep_key, target->cdds_no,
			row->priority, (i1)0, (i2)0, row->trans_time, NULL,
			col_list, val_list, pdesc, pdata);
		if (status != OK)
		{	
			RSmem_free(col_list,val_list,stmt,where_list);
			return (status);
		}
		/*
		** For FP targets, insert into the input queue for cascade
		** propagation.
		*/
		if (target->type == TARG_FULL_PEER)
		{
			status = RSiiq_InsertInputQueue(conn, tbl,
				row->trans_type, &row->rep_key,
				&row->old_rep_key, target->cdds_no,
				row->priority, row->trans_time, FALSE);
			if (status != OK)
			{
				RSmem_free(col_list,val_list,stmt,where_list); 
				return (status);
			}
		}
	}
	/*
	** For FP and URO targets without blob columns, call the remote
	** database procedure.
	*/
	else if (target->type != TARG_UNPROT_READ)
	{
		proc_name = pnames[0];
		if (RPtblobj_name(tbl->table_name, row->table_no,
			TBLOBJ_REM_UPD_PROC, proc_name) != OK)
		{
			messageit(1, 1816, ERx("RSupdate"), tbl->table_name);
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (RS_INTERNAL_ERR);
		}
		if (RSpdp_PrepDbprocParams(proc_name, target, tbl, row, pnames,
				&nparams, pdesc, pdata) != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (RS_INTERNAL_ERR);
		}
		status = IIsw_execProcedure(conn->connHandle,
			&conn->tranHandle, nparams, pdesc, pdata, &procRet,
			&stmtHandle,  &getQinfoParm, &errParm);
		status = RSerror_check(1572, ROWS_DONT_CARE, stmtHandle,
			 &getQinfoParm, &errParm, NULL, proc_name);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (status);
		}
		if (procRet)
		{
			messageit(1, procRet, row->rep_key.src_db,
				row->rep_key.trans_id, row->rep_key.seq_no,
				target->db_no, tbl->table_name);
			RSmem_free(col_list,val_list,stmt,where_list); 
			return (procRet);
		}
	}

	RSmem_free(col_list,val_list,stmt,where_list); 

	RSstats_update(target->db_no, tbl->table_no, RS_UPDATE);
	if (target->type != TARG_UNPROT_READ)
		RSmonitor_notify(&RSconns[target->conn_no], RS_INC_UPDATE,
			RSlocal_conn.db_no, tbl->table_name, tbl->table_owner);
	RSmonitor_notify(&RSlocal_conn, RS_OUT_UPDATE, target->db_no,
		tbl->table_name, tbl->table_owner);

	return (OK);
}

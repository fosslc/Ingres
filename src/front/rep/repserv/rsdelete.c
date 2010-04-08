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
# include <targtype.h>
# include <tblobjs.h>
# include <targtype.h>
# include "conn.h"
# include "tblinfo.h"
# include "distq.h"
# include "repserv.h"
# include "rsstats.h"
# include "repevent.h"

/**
** Name:	rsdelete.c - propagate a DELETE
**
** Description:
**	Defines
**		RSdelete	- propagate a DELETE row
**
** History:
**	11-dec-97 (joea)
**		Created based on ddelete.sc in replicator library and on
**		ddpquery.c and qryfuncs.tlp in ingres library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	09-jul-98 (abbjo03)
**		Add support for blob datatypes.
**	22-jul-98 (abbjo03)
**		If the FP dbproc returns non-zero, return an error. Use remote
**		owner name when deleting from remote table. Allow for delimited
**		identifiers in pnames.
**	05-nov-98 (abbjo03)
**		Add call to update statistics.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
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
**		Replace number of columns by DB_MAX_COLS. Replace formula for
**		maximum length of a delimited identifier by DB_MAX_DELIMID.
**      06-Nov-2003 (inifa01) INGREP 108 bug 106498
**              Changed from allocating default MAX SIZE for stmt & where_list 
**              which was insufficient for large tables to calling
**              RSmem_allocate() to allocate MAX possible size based on the
**              replicated tables row_width and number of replicated columns.
**	30-mar-2004 (gupsh01)
**		Added getQinfoParm as input parameter to II_sw... functions.
**/

/*{
** Name:	RSdelete	- propagate a DELETE row
**
** Description:
**	Propagates a DELETE transaction row, either by calling a remote
**	database procedure or by executing a remote DELETE.
**
** Inputs:
**	row		- row of data
**
** Outputs:
**	none
**
** Returns:
**	OK
**	else	- error
*/
STATUS
RSdelete(
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	char 			*stmt=NULL;
	char			*proc_name;
	STATUS			err;
	bool			collision_processed = FALSE;
	RS_TBLDESC		*tbl = row->tbl_desc;
	II_INT2			nparams;
	IIAPI_DESCRIPTOR	pdesc[DB_MAX_COLS+1];
	IIAPI_DATAVALUE		pdata[DB_MAX_COLS+1];
	char			pnames[DB_MAX_COLS+1][DB_MAX_DELIMID+1];
	RS_CONN			*conn = &RSconns[target->conn_no];
	II_LONG			procRet;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	IIAPI_DESCRIPTOR 	*col;
	IIAPI_DESCRIPTOR 	*pds;
	i4			num_cols=tbl->num_regist_cols;
	i4			row_width=tbl->row_width;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+8,128); 
	if (stmt == NULL)
	   return (FAIL);

	messageit(5, 1268, row->rep_key.src_db, row->rep_key.trans_id,
		row->rep_key.seq_no, target->db_no);

	err = RScollision(target, row, tbl, &collision_processed);
	if (err || collision_processed)
	{
		MEfree((PTR)stmt);
		return (err);
	}

	/* For URO targets, delete the base row. */
	if (target->type == TARG_UNPROT_READ)
	{
		char	*where_list=NULL;
		char	objname[DB_MAXNAME*2+3];

		where_list = RSmem_allocate(row_width,tbl->num_key_cols,DB_MAXNAME+8,0); 
		if (where_list == NULL)
		{
			MEfree((PTR)stmt);
			return (FAIL);
		}
		/*
		** Iterate through key_desc and create the WHERE list and
		** prepare the column descriptors. The data will come directly
		** from key_data.
		*/
		*where_list = EOS;
		for (col = row->key_desc, pds = pdesc;
			col < row->key_desc + tbl->num_key_cols; ++col, ++pds)
		{
			if (col != row->key_desc)
				STcat(where_list, ERx(" AND "));
			/*
			** The following is special case code to allow existing
			** customers in non-SQL92 installations to propagate to
			** Gateway databases that use uppercase table and
			** column names.
			*/
			if (conn->name_case == UI_UPPERCASE)
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
			*pds = *col;	/* struct */
			pds->ds_columnType = IIAPI_COL_QPARM;
		}
		STcopy(tbl->dlm_table_name, objname);
		if (target->type == TARG_UNPROT_READ &&
				conn->name_case == UI_UPPERCASE)
			CVupper(objname);
		STprintf(stmt, ERx("DELETE FROM %s.%s WHERE %s"),
			tbl->rem_table_owner, objname, where_list);
		status = IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
			tbl->num_key_cols, pdesc, row->key_data, NULL, NULL,
			&stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1556, ROWS_SINGLE_ROW, stmtHandle,
			&getQinfoParm, &errParm, NULL, tbl->table_name);
		MEfree((PTR)where_list);
		if (status != OK)
		{
			MEfree((PTR)stmt);
			return (status);
		}
	}
	/* For FP and PRO targets, call the remote database procedure. */
	else
	{
		proc_name = pnames[0];
		if (RPtblobj_name(tbl->table_name, row->table_no,
			TBLOBJ_REM_DEL_PROC, proc_name) != OK)
		{
			messageit(1, 1816, ERx("RSdelete"), tbl->table_name);
			MEfree((PTR)stmt);
			return (RS_INTERNAL_ERR);
		}

		if (RSpdp_PrepDbprocParams(proc_name, target, tbl, row, pnames,
				&nparams, pdesc, pdata) != OK)
		{
			MEfree((PTR)stmt);
			return (RS_INTERNAL_ERR);
		}
		status = IIsw_execProcedure(conn->connHandle,
			&conn->tranHandle, nparams, pdesc, pdata, &procRet,
			&stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1573, ROWS_DONT_CARE, stmtHandle,
			&getQinfoParm, &errParm, NULL, proc_name);
		if (status != OK)
		{
			MEfree((PTR)stmt);
			return (status);
		}
		if (procRet)
		{
			messageit(1, procRet, row->rep_key.src_db,
				row->rep_key.trans_id, row->rep_key.seq_no,
				target->db_no, tbl->table_name);
			MEfree((PTR)stmt);
			return (procRet);
		}
	}

	RSstats_update(target->db_no, tbl->table_no, RS_DELETE);
	if (target->type != TARG_UNPROT_READ)
		RSmonitor_notify(&RSconns[target->conn_no], RS_INC_DELETE,
			RSlocal_conn.db_no, tbl->table_name, tbl->table_owner);
	RSmonitor_notify(&RSlocal_conn, RS_OUT_DELETE, target->db_no,
		tbl->table_name, tbl->table_owner);

	MEfree((PTR)stmt);
	return (OK);
}

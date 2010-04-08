/*
** Copyright (c) 1997, 2009 Ingres Corporation
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
#include <tbldef.h>
# include <targtype.h>
# include "rsconst.h"
# include "conn.h"
# include "tblinfo.h"
# include "distq.h"
# include "repserv.h"
# include "rsstats.h"
# include "repevent.h"

/**
** Name:	rsinsert.c - propagate an INSERT
**
** Description:
**	Defines
**		RSinsert	- propagate an INSERT row
**
** History:
**	11-dec-97 (joea)
**		Created based on dinsert.sc in replicator library and on
**		ddpquery.c and qryfuncs.tlp in ingres library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	09-jul-98 (abbjo03)
**		Add support for blob datatypes.
**	22-jul-98 (abbjo03)
**		If the FP dbproc returns non-zero, return an error. Use remote
**		owner name when inserting into the remote table. Allow for
**		delimited identifiers in pnames.
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
**	11-may-99 (abbjo03)
**		Add special code to allow existing customers in non-SQL92
**		installations to propagate to uppercase Gateway databases.
**	21-jul-99 (abbjo03)
**		Replace formula for maximum length of a delimited identifier by
**		DB_MAX_DELIMID.
**	12-oct-99 (nanpr01/abbjo03)
**		Implement optimistic collision resolution for Priority and Last
**		Write Wins. Do not call RScollision on entry. Call it if the
**		procedure returns an indication of duplicate key on insert.
**	24-nov-99 (abbjo03)
**		Rename RSoptim_coll_resol as RSonerror_norollback_target so
**		that it can be used in IIRSrlt_ReleaseTrans.
**	03-Apr-2002 (inifa01) INGREP 110 bug 107490
**		With cdds collision mode 4 or 3 and error mode 1(SKIP ROW), if a 
**		collision is encountered the replicator server terminates and dumps  
**		core after reporting errors; 114 D_INSERT: Duplicate error inserting,
**		record 1351/1349 Char execute immediate failed with error 13172739,  
**		1212 PING: DBMS error 13172739 raising event. The requested operation 
**		cannot be performed with active transactions.
**		Fix was to re-instate the distributed transaction handle via call to
**		RSdtrans_id_get_last() only if a rollback occured in RSerror_check()
**		due to error.
**      06-Nov-2003 (inifa01) INGREP 108 bug 106498
**              Changed from allocating default MAX SIZE temp buffers for col_list, 
**              stmt and val_list which was insufficient for large tables to
**              calling RSmem_allocate() to allocate MAX possible size based on the
**              replicated tables row_width and number of replicated columns.
**		Call RSmem_free() to free multiple temp buffers.
**	30-Mar-2004 (gupsh01)
**		Added getQinfoParm as input parameter to II_sw... functions.
**/

GLOBALREF bool	RSonerror_norollback_target;
GLOBALREF RS_TARGET  *RStarget;

/*{
** Name:	RSinsert	- propagate an INSERT row
**
** Description:
**	Propagates an INSERT transaction row, either by calling a remote
**	database procedure or by executing a remote INSERT.
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
RSinsert(
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	char 			*stmt=NULL;
	char			*col_list=NULL;
	char			*val_list=NULL;
	char			*proc_name;
	STATUS			status;
	bool			collision_processed = FALSE;
	RS_TBLDESC		*tbl = row->tbl_desc;
	RS_COLDESC              *colp;
	II_INT2			nparams;
	IIAPI_DESCRIPTOR	pdesc[DB_MAX_COLS+13];
	IIAPI_DATAVALUE		pdata[DB_MAX_COLS+13];
	char			pnames[DB_MAX_COLS+1][DB_MAX_DELIMID+1];
	RS_CONN			*conn = &RSconns[target->conn_no];
	II_LONG			procRet;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_DESCRIPTOR	*col;
	IIAPI_DESCRIPTOR	*kcol;
	IIAPI_DESCRIPTOR	*cds;
	IIAPI_DATAVALUE		*cdv;
	i4			row_width = tbl->row_width;
	i4			num_cols = tbl->num_regist_cols;
	IIAPI_GETQINFOPARM	getQinfoParm;
	char                    over_ident[24];

	col_list = RSmem_allocate(0,num_cols,DB_MAXNAME+2,0); 
	if (col_list == NULL)
	   return (FAIL);

	val_list = RSmem_allocate(row_width,num_cols,4,0); 
	if (val_list == NULL)
	{
		MEfree((PTR)col_list);
		return (FAIL);
	}

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+6,128); 
	if (stmt == NULL)
	{
		RSmem_free(col_list,val_list,NULL,NULL);
		return (FAIL);
	}
	messageit(5, 1258, row->rep_key.src_db, row->rep_key.trans_id,
		row->rep_key.seq_no, target->db_no);
	if (target->collision_mode == COLLMODE_ACTIVE ||
		target->collision_mode == COLLMODE_BENIGN)
	{
		status = RScollision(target, row, tbl, &collision_processed);
		if (collision_processed)
		{
			RSstats_update(target->db_no, tbl->table_no, RS_INSERT);
			if (target->type != TARG_UNPROT_READ)
				RSmonitor_notify(conn, RS_INC_INSERT,
					RSlocal_conn.db_no, tbl->table_name,
					tbl->table_owner);
			RSmonitor_notify(&RSlocal_conn, RS_OUT_INSERT,
				target->db_no, tbl->table_name,
				tbl->table_owner);
		}
		if (status != OK || collision_processed)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (status);
		}
	}

	/*
	** For URO targets or if the table has blob columns, we first insert
	** the base row into the remote table.
	*/
	if (target->type == TARG_UNPROT_READ || tbl->has_blob)
	{
		IISW_CALLBACK	cb;
		char		objname[DB_MAXNAME*2+3];

		messageit(5, 1142, tbl->table_name);
		/*
		** Iterate through row_desc to create the INSERT column and
		** value lists and prepare the column descriptors. The data
		** will come directly from row_data.
		*/
		*col_list = *val_list = EOS;
		for (col = row->row_desc, cds = pdesc; col < row->row_desc +
			tbl->num_regist_cols; ++col, ++cds)
		{
			if (col != row->row_desc)
			{
				STcat(col_list, ERx(", "));
				STcat(val_list, ERx(","));
			}
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
		*over_ident = EOS;
		for (colp = tbl->cols;
		     colp < tbl->cols + tbl->num_regist_cols; ++colp)
                {
		    if (colp->col_ident == COL_IDENT_ALWAYS)
		    {
		        STcopy("OVERRIDING SYSTEM VALUE", over_ident);
		        break;
		    }
		    else if (colp->col_ident == COL_IDENT_BYDEFAULT)
		    {
		        STcopy("OVERRIDING USER VALUE", over_ident);
		        break;
		    }
		}
		STprintf(stmt, "INSERT INTO %s.%s (%s) %s VALUES (%s)",
			 tbl->rem_table_owner, objname, col_list, over_ident,
			 val_list);
		if (tbl->has_blob)
			cb = RSblob_ppCallback;
		else
			cb = NULL;
		IIsw_query(conn->connHandle, &conn->tranHandle, stmt,
			tbl->num_regist_cols, pdesc, row->row_data, cb, NULL,
			&stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1134, ROWS_SINGLE_ROW, stmtHandle,
			 &getQinfoParm, &errParm, NULL, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no,
			target->db_no, tbl->table_name);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (status);
		}
	}
	/*
	** For FP and PRO targets and tables with blobs, update the shadow and
	** input queue.
	*/
	if (target->type != TARG_UNPROT_READ && tbl->has_blob)
	{
		IIAPI_DATAVALUE	*kdv;

		/*
		** Iterate through key_desc and prepare the column and data
		** descriptors. Since for an INSERT key_data is empty we need
		** to prepare the data descriptors from row_data by searching
		** through row_desc. Also create the key column and value lists
		** for the INSERT into the shadow table.
		*/
		*col_list = *val_list = EOS;
		for (kcol = row->key_desc, cds = pdesc, cdv = pdata; kcol <
			row->key_desc + tbl->num_key_cols; ++kcol, ++cds, ++cdv)
		{
			if (kcol != row->key_desc)
			{
				STcat(col_list, ERx(", "));
				STcat(val_list, ERx(","));
			}
			STcat(col_list, kcol->ds_columnName);
			STcat(val_list, ERx(" ~V "));
			*cds = *kcol;	/* struct */
			cds->ds_columnType = IIAPI_COL_QPARM;
			for (col = row->row_desc, kdv = row->row_data;
				col < row->row_desc + tbl->num_regist_cols;
				++col, ++kdv)
			{
				if (STequal(kcol->ds_columnName,
					col->ds_columnName))
				{
					*cdv = *kdv;	/* struct */
					break;
				}
			}
		}
		/*
		** For FP targets, if the previous transaction was a DELETE
		** update the shadow row to indicate it has been archived.
		*/
		if (target->type == TARG_FULL_PEER)
		{
			char	*where_list;

			where_list = RSmem_allocate(row_width,tbl->num_key_cols,DB_MAXNAME+8,0); 
			if (where_list == NULL)
			{
				RSmem_free(col_list,val_list,stmt,NULL); 
				return (FAIL);
			}
			/*
			** Iterate through key_desc and prepare the where list.
			** The UPDATE will use the column and data descriptors
			** prepared above.
			*/
			*where_list = EOS;
			for (kcol = row->key_desc; kcol < row->key_desc +
				tbl->num_key_cols; ++kcol)
			{
				if (kcol != row->key_desc)
					STcat(where_list, ERx(" AND "));
				STcat(where_list, kcol->ds_columnName);
				STcat(where_list, ERx(" = ~V"));
			}
			STprintf(stmt, ERx("UPDATE %s SET in_archive = 1 \
WHERE %s AND trans_type = %d AND in_archive = 0"), 
				tbl->shadow_table, where_list, RS_DELETE);
			status = IIsw_query(conn->connHandle,
				&conn->tranHandle, stmt, tbl->num_key_cols,
				pdesc, pdata, NULL, NULL, &stmtHandle,
				&getQinfoParm, &errParm);
			/*
			** We don't care if any rows were updated since normally
			** there won't be a previous DELETE.
			*/
			status = RSerror_check(1657, ROWS_DONT_CARE, stmtHandle,
				 &getQinfoParm, &errParm, NULL, row->rep_key.src_db,
				row->rep_key.trans_id, row->rep_key.seq_no,
				target->db_no, tbl->table_name);
			MEfree((PTR)where_list);
			if (status != OK)
			{
				RSmem_free(col_list,val_list,stmt,NULL); 
				return (status);
			}
		}
		/* Insert into the shadow table */
		status = RSist_InsertShadowTable(conn, tbl, row->trans_type,
			&row->rep_key, NULL, target->cdds_no, row->priority,
			(i1)0, (i2)0, row->trans_time, NULL, col_list, val_list,
			pdesc, pdata);
		if (status != OK)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
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
				RSmem_free(col_list,val_list,stmt,NULL); 
				return (status);
			}
		}
	}
	/*
	** For FP and PRO targets without blob columns, call the remote
	** database procedure.
	*/
	else if (target->type != TARG_UNPROT_READ)
	{
		proc_name = pnames[0];
		if (RPtblobj_name(tbl->table_name, row->table_no,
			TBLOBJ_REM_INS_PROC, proc_name) != OK)
		{
			messageit(1, 1816, ERx("RSinsert"), tbl->table_name);
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (RS_INTERNAL_ERR);
		}
		if (RSpdp_PrepDbprocParams(proc_name, target, tbl, row,
				pnames, &nparams, pdesc, pdata) != OK)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (RS_INTERNAL_ERR);
		}

		status = IIsw_execProcedure(conn->connHandle,
			&conn->tranHandle, nparams, pdesc, pdata, &procRet,
			&stmtHandle, &getQinfoParm, &errParm);
		RSonerror_norollback_target = TRUE;
		status = RSerror_check(1571, ROWS_DONT_CARE, stmtHandle,
			 &getQinfoParm, &errParm, NULL, proc_name);
		RSonerror_norollback_target = FALSE;
		if (status != OK && target->collision_mode != COLLMODE_PRIORITY
				&& target->collision_mode != COLLMODE_TIME)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (status);
		}
		if (status != OK && procRet != 114)
		{
			RSmem_free(col_list,val_list,stmt,NULL); 
			return (status);
		}
		if (procRet)
		{
		    messageit(3, procRet, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no,
			target->db_no, tbl->table_name);
		    if ((target->collision_mode == COLLMODE_PRIORITY ||
		    	target->collision_mode == COLLMODE_TIME) &&
			procRet == 114) 
		    {
			/* [inifa01 b107490] We only need to re-instate the transactn
			   handle here if it has been freed above by RSerror_check().
			   If the target error mode is SKIP ROW then we've still got
			   our handle, no rollback in RSerror_check() in this mode, 
			   so lets keep using what we've got. 
			*/
			if (RStarget->error_mode != EM_SKIP_ROW)
			    RSlocal_conn.tranHandle = RSdtrans_id_get_last(); 

			status = RScollision(target, row, tbl, 
				&collision_processed);
			if (status == OK && !collision_processed)
			{
				status = RSinsert(target, row);
				RSmem_free(col_list,val_list,stmt,NULL); 
				return (status);
			}
			else
			{
			    RSstats_update(target->db_no, tbl->table_no, 
				   RS_INSERT);
			    if (target->type != TARG_UNPROT_READ)
				RSmonitor_notify(conn, RS_INC_INSERT,
					RSlocal_conn.db_no, tbl->table_name,
					tbl->table_owner);
			    RSmonitor_notify(&RSlocal_conn, RS_OUT_INSERT,
				target->db_no, tbl->table_name,
				tbl->table_owner);
			}
			if (status != OK || collision_processed)
			{
				RSmem_free(col_list,val_list,stmt,NULL); 
			    	return (status);
			}
		    }
		    else 
			{
				RSmem_free(col_list,val_list,stmt,NULL); 
				return (procRet);
			}
		}
	}

	RSstats_update(target->db_no, tbl->table_no, RS_INSERT);
	if (target->type != TARG_UNPROT_READ)
		RSmonitor_notify(conn, RS_INC_INSERT, RSlocal_conn.db_no,
			tbl->table_name, tbl->table_owner);
	RSmonitor_notify(&RSlocal_conn, RS_OUT_INSERT, target->db_no,
		tbl->table_name, tbl->table_owner);

	RSmem_free(col_list,val_list,stmt,NULL); 
	return (OK);
}

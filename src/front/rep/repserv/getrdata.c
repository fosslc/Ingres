/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <tblobjs.h>
# include <rpu.h>
# include <targtype.h>
# include "conn.h"
# include "distq.h"
# include "tblinfo.h"
# include "repserv.h"
# include "erusf.h"

/**
** Name:	getrdata.c - get row data
**
** Description:
**	Defines
**		IIRSgrd_GetRowData	- get row data
**		get_prev_trans_info	- get previous transaction info
**		set_dataval		- set IIAPI_DATAVALUE structure
**
** History:
**	09-dec-97 (joea)
**		Created based on distsupp.sc in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	08-may-98 (joea)
**		Add support for C, DECIMAL, BYTE and BYTE VARYING datatypes in
**		set_dataval.
**	09-jul-98 (abbjo03)
**		Add support for LONG VARCHAR and LONG BYTE. Use RSerror_check
**		to check number of rows affected.  Add support for non-DBA
**		tables and delimited table names.
**	12-aug-98 (abbjo03)
**		Change the selects to repeated where possible.
**	21-aug-98 (abbjo03)
**		Remove dead test code left from 12-aug-98 change.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	23-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Nov-2003 (padni01/inifa01) INGREP 108 bug 106498
**		Allocate temporary buffers col_list and stmt in 
**		IIRSgrd_GetRowData(), based on the number of columns
**		registered for replication.
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm to the input parameter list
**	    of II_sw.. function calls.
**    27-Oct-2004 (inifa01) bug 113336 INGREP166
**	    Replicated transactions get processed out of order if an update
**	    transaction fails due to lock timeout on the shadow table.
**	    FIX: Changed to return actual error if get_prev_trans_info()
**	    fails, instead of returning RS_DBMS_ERROR.  On return to transmit()
**	    will check if queue should be re-read.
**	31-mar-2005 (gupsh01)
**	    Added support for i8 in set_dataval.
**	23-Aug-2005 (inifa01)INGREP174/B114410
**	    The RepServer takes shared locks on base, shadow and archive tables
**	    when Replicating FP -> URO.  This is not necessary and it causes lock
**	    waits and deadlocks with subsequent user transactions. 
**	    - The resolution to this problem adds flag -NLR and opens a separate 
**	    session running with readlock=nolock if this flag is set.  This 
**	    separate session will read the shadow and archive tables with nolock
**	    and set shared locks on the base table with an immediate commit once
**	    the shadow/archive/base tables have been read.
**	15-Mar-2006 (hanje04)
**	    Add IIAPI_GETQINFOPAR parameters to IIsw calls after X-integ of
**	    fix for bug 114410. Also define where apropriate.
**	16-Mar-2006 (hanje04)
**	    Add missing argument from get_prev_trans_info() call in 
**	    IIRSgrd_GetRowData.
**	2-Oct-2006 (kibro01) bug 116030
**	    Return correct error code in the event of timeout error in
**	    IIRSgrd_GetRowData so we can always loop round correctly and 
**	    try again.
**      28-Apr-2009 (coomi01) b121984
**          Add IIAPI_DATE_TYPE to support ANSI Dates.
**      24-May-2010 (stial01)
**          Added buf5 param to RSmem_free()
**      17-Aug-2010 (thich01)
**          Add geospatial types to IIAPI types.  Make changes to treat spatial
**          types like LBYTEs or NBR type as BYTE.
**/

# define ALRES_BYTES	sizeof(ALIGN_RESTRICT)
# define ALRES(s)	((s) + ALRES_BYTES - 1) / ALRES_BYTES


static int get_prev_trans_info(RS_TBLDESC *tbl, RS_TRANS_ROW *row, RS_TARGET *target);
static STATUS set_dataval(IIAPI_DESCRIPTOR *col, PTR *pbuf,
	IIAPI_DATAVALUE *dv);


/*{
** Name:	IIRSgrd_GetRowData	- get row data
**
** Description:
**	Fills the row_data and key_data buffers from the base and archive
**	tables.
**
** Inputs:
**	row		- row of data
**
** Outputs:
**	row		- row of data
**
** Returns:
**	OK
*/
STATUS
IIRSgrd_GetRowData(
i2		server_no,
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	RS_TBLDESC		*tbl = row->tbl_desc;
	RS_COLDESC		*col;
	char			*stmt=NULL;
	char			*col_list=NULL;
	char			where_tblkey[2048];
	char			key_cols[2048];
	char			where_col[DB_MAXNAME+16];
	i4			i;
	II_INT2			nparms;
	II_INT2			nkeys;
	i4			msg_num;
	char			in_archive;
	PTR			pd;	/* pointer into data buffer */
	PTR			pk;	/* pointer into key buffer */
	ALIGN_RESTRICT		prev_key_info[ALRES(4096)];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DESCRIPTOR	kdesc[DB_MAX_COLS];
	IIAPI_DATAVALUE		keydv[DB_MAX_COLS];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	IIAPI_DESCRIPTOR	*kds;
	IIAPI_DATAVALUE		*cdv;
	IIAPI_DATAVALUE		*kdv;
	char			qry_char_id[DB_MAXNAME+16];
	SW_REPEAT_QUERY_ID	qryId;
	i4			num_cols=tbl->num_regist_cols;

	if (row->trans_type != RS_INSERT)
	{
		status = get_prev_trans_info(tbl, row, target);
		if (status != OK)
		{
			/* Timeout is not an error - keep it different */
			if (status == E_US125E_4702)
				return (RS_REREAD);
			return (RS_DBMS_ERR);
		}
	}

	if (row->trans_type == RS_DELETE)
		return (OK);

	col_list = RSmem_allocate(0,num_cols,DB_MAXNAME+3,0); 
	if (col_list == NULL)
	   return (FAIL);

	stmt = RSmem_allocate(0,num_cols,DB_MAXNAME+3,2048); 
	if (stmt == NULL)
	{
		MEfree((PTR)col_list);
		return (FAIL);
	}

        if (target->type == TARG_UNPROT_READ && RSnolock_read)
        {
		/* Issue a commit so that a shared lock can be taken on the base
		** table, the session is running with readlock=nolock.
		*/
                IIsw_commit(&RSlocal_conn_nlr.tranHandle, &errParm);
                STprintf(stmt, ERx("SET LOCKMODE  ON %s.%s WHERE READLOCK = SHARED"),
                tbl->dlm_table_owner, tbl->dlm_table_name);

                if (IIsw_query(RSlocal_conn_nlr.connHandle, &RSlocal_conn_nlr.tranHandle, stmt,
                    0, NULL, NULL,NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
                {/* Error setting shared lock on table */
                        messageit(1, 1901, errParm.ge_errorCode, tbl->dlm_table_owner,
			tbl->dlm_table_name,errParm.ge_message);
                        IIsw_close(stmtHandle, &errParm);
                        return (FAIL);
                }
 		IIsw_close(stmtHandle, &errParm);
	}

	/* specify the fixed fields first */
	SW_COLDATA_INIT(keydv[0], in_archive);

	*col_list = *key_cols = EOS; 
	pd = (char *)row->row_data + sizeof(IIAPI_DATAVALUE) *
		tbl->num_regist_cols;
	kdv = &keydv[1];
	pk = (PTR)prev_key_info;
	MEfill((u_i2)sizeof(prev_key_info), (u_char)0, pk);

	for (col = tbl->cols, cdv = row->row_data, nkeys = 1;
		col < tbl->cols + tbl->num_regist_cols; ++col, ++cdv)
	{
		if (*col_list != EOS)
			STcat(col_list, ERx(", "));
		STcat(col_list, col->col_name);
		if (set_dataval(&col->coldesc, &pd, cdv) != OK)
		{
			RSmem_free(col_list,stmt,NULL,NULL,NULL); 
			return (RS_MEM_ERR);
		}
		if (col->key_sequence)
		{
			if (*key_cols != EOS)
				STcat(key_cols, ERx(", "));
			STcat(key_cols, col->col_name);
			set_dataval(&col->coldesc, &pk, kdv);
			++kdv;
			++nkeys;
		}
	}

	i = 0;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->rep_key.src_db);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->rep_key.trans_id);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->rep_key.seq_no);
	STprintf(stmt, ERx("SELECT in_archive, %s FROM %s WHERE sourcedb = $0 \
= ~V AND transaction_id = $1 = ~V AND sequence_no = $2 = ~V"),
		key_cols, tbl->shadow_table);
	qryId.int_id[0] = tbl->table_no;
	qryId.int_id[1] = 0;
	qryId.char_id = qry_char_id;
	STprintf(qry_char_id, ERx("RSgrd1_%-.25s"), tbl->table_name);

	if (target->type == TARG_UNPROT_READ && RSnolock_read)
	{
		/* Read the shadow table in separate session with readlock=nolock */
	    	status = IIsw_selectSingletonRepeated(RSlocal_conn_nlr.connHandle,
		         &RSlocal_conn_nlr.tranHandle, stmt, 3, pdesc, pdata, &qryId,
		         &tbl->grd1_qryHandle, nkeys, keydv, &stmtHandle, &getQinfoParm, &errParm);
       	    	status = RSerror_check(1126, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
                         NULL, row->rep_key.src_db, row->rep_key.trans_id,
                         row->rep_key.seq_no, row->tbl_desc->table_name);

            	if (status != OK)
                {
                	RSmem_free(col_list,stmt,NULL,NULL,NULL);
                	return (status);
            	}

	}
	else
	{
		status = IIsw_selectSingletonRepeated(RSlocal_conn.connHandle,
		         &RSlocal_conn.tranHandle, stmt, 3, pdesc, pdata, &qryId,
		         &tbl->grd1_qryHandle, nkeys, keydv, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1126, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
		         NULL, row->rep_key.src_db, row->rep_key.trans_id,
		         row->rep_key.seq_no, row->tbl_desc->table_name);
		if (status != OK)
		{
			RSmem_free(col_list,stmt,NULL,NULL,NULL); 
			return (status);
		}
	}
	if (in_archive)
	{
		STprintf(stmt, ERx("SELECT %s FROM %s WHERE "), col_list,
			tbl->archive_table);
		if (tbl->has_blob)
			STcat(stmt, ERx("sourcedb = ~V AND transaction_id = ~V \
AND sequence_no = ~V"));
		else
			STcat(stmt, ERx("sourcedb = $0 = ~V AND transaction_id \
= $1 = ~V AND sequence_no = $2 = ~V"));
		STprintf(qry_char_id, ERx("RSgrd2_%-.25s"), tbl->table_name);
		nparms = 3;
		for (i = 0; i < nparms; ++i)
		{
			kdesc[i] = pdesc[i];		/* struct */
			keydv[i + 1] = pdata[i];	/* struct */
		}
		msg_num = 1132;
	}
	else
	{
		STprintf(stmt, ERx("SELECT %s FROM %s.%s WHERE "), col_list,
			tbl->dlm_table_owner, tbl->dlm_table_name);
		STprintf(qry_char_id, ERx("RSgrd3_%-.25s"), tbl->table_name);
		*where_tblkey = EOS;
		i = 0;
		for (col = tbl->cols, kds = kdesc; col < tbl->cols +
			tbl->num_regist_cols; ++col)
		{
			if (col->key_sequence)
			{
				*kds = col->coldesc;	/* struct */
				kds->ds_columnType = IIAPI_COL_QPARM;
				if (*where_tblkey != EOS)
					STcat(where_tblkey, ERx(" AND "));
				if (tbl->has_blob)
					STprintf(where_col, ERx("%s = ~V"),
						col->col_name);
				else
					STprintf(where_col,
						ERx("%s = $%d = ~V"),
						col->col_name, i++);
				STcat(where_tblkey, where_col);
				++kds;
			}
		}
		STcat(stmt, where_tblkey);
		nparms = tbl->num_key_cols;
		msg_num = 1128;
	}
	if (target->type == TARG_UNPROT_READ && RSnolock_read)
	{
		/* Read the base/archive table in separate session. Archive table 
		** read with readlock=nolock. base table with shared lock 
		*/
		if (tbl->has_blob)
		    status = IIsw_selectSingleton(RSlocal_conn_nlr.connHandle,
			    &RSlocal_conn_nlr.tranHandle, stmt, nparms, kdesc,
			    &keydv[1], tbl->num_regist_cols, row->row_data,
		            RSblob_gcCallback, NULL, &stmtHandle, &getQinfoParm, &errParm);
		else
		    status = IIsw_selectSingletonRepeated(RSlocal_conn_nlr.connHandle,
			    &RSlocal_conn_nlr.tranHandle, stmt, nparms, kdesc,
			    &keydv[1], &qryId, in_archive ? &tbl->grd2_qryHandle :
			    &tbl->grd3_qryHandle, tbl->num_regist_cols,
			    row->row_data, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(msg_num, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
		     	 NULL, row->rep_key.src_db, row->rep_key.trans_id,
		         row->rep_key.seq_no, row->tbl_desc->table_name);
		
		/* commit transaction on session running with readlock=nolock  
		** immediately to release shared lock on base table 
		*/
		IIsw_commit(&RSlocal_conn_nlr.tranHandle, &errParm);
		RSmem_free(col_list,stmt,NULL,NULL,NULL); 
	}
	else
        {
        	if (tbl->has_blob)
                    status = IIsw_selectSingleton(RSlocal_conn.connHandle,
                            &RSlocal_conn.tranHandle, stmt, nparms, kdesc,
                            &keydv[1], tbl->num_regist_cols, row->row_data,
                            RSblob_gcCallback, NULL, &stmtHandle, &getQinfoParm, &errParm);
        	else
                    status = IIsw_selectSingletonRepeated(RSlocal_conn.connHandle,
                            &RSlocal_conn.tranHandle, stmt, nparms, kdesc,
                            &keydv[1], &qryId, in_archive ? &tbl->grd2_qryHandle :
                            &tbl->grd3_qryHandle, tbl->num_regist_cols,
                            row->row_data, &stmtHandle, &getQinfoParm, &errParm);
        	status = RSerror_check(msg_num, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
                         NULL, row->rep_key.src_db, row->rep_key.trans_id,
                         row->rep_key.seq_no, row->tbl_desc->table_name);
        	RSmem_free(col_list,stmt,NULL,NULL,NULL);
        }
	return (status);
}


/*{
** Name:	get_prev_trans_info - get previous transaction information
**
** Description:
**	SELECTs key and other information from shadow table for transaction
**	that affected row before the current one.
**
** Inputs:
**	tbl		- pointer to table cache
**	row		- row of data
**
** Outputs:
**	row		- row of data
**
** Returns:
**	OK
**	RS_INTERNAL_ERR or RS_DBMS_ERR
*/
static int
get_prev_trans_info(
RS_TBLDESC	*tbl,
RS_TRANS_ROW	*row,
RS_TARGET	*target)
{
	char			stmt[4096];
	RS_COLDESC		*col;
	char			key_cols[2048];
	i4			i;
	PTR			pk;	/* pointer into key buffer */
	II_INT2			nkeys;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		keydv[DB_MAX_COLS];
	IIAPI_DATAVALUE		*kdv;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	char			qry_char_id[DB_MAXNAME+1];
	SW_REPEAT_QUERY_ID	qryId;

	/* specify the fixed fields first */
	SW_COLDATA_INIT(keydv[0], row->old_cdds_no);
	SW_COLDATA_INIT(keydv[1], row->old_priority);
	SW_COLDATA_INIT(keydv[2], row->old_trans_time);
	SW_COLDATA_INIT(keydv[3], row->new_key);

	*key_cols = EOS;
	kdv = &keydv[4];
	pk = (char *)row->key_data + sizeof(IIAPI_DATAVALUE) *
		tbl->num_regist_cols;

	for (col = tbl->cols, i = 0, nkeys = 4;
		col < tbl->cols + tbl->num_regist_cols; ++col)
	{
		if (col->key_sequence)
		{
			if (*key_cols != EOS)
				STcat(key_cols, ERx(", "));
			STcat(key_cols, col->col_name);
			set_dataval(&col->coldesc, &pk, kdv);
			++kdv;
			++nkeys;
		}
	}

	STprintf(stmt, ERx("SELECT cdds_no, dd_priority, DATE_GMT(trans_time) \
AS trans_time, new_key, %s FROM %s WHERE sourcedb = $0 = ~V AND transaction_id \
= $1 = ~V AND sequence_no = $2 = ~V"),
		key_cols, tbl->shadow_table);
	if (row->trans_type == RS_DELETE)
		STcat(stmt, ERx(" AND in_archive = 1"));
	i = 0;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->old_rep_key.src_db);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->old_rep_key.trans_id);
	++i;
	SW_PARM_DESC_DATA_INIT(pdesc[i], pdata[i], IIAPI_INT_TYPE,
		row->old_rep_key.seq_no);
	qryId.int_id[0] = tbl->table_no;
	qryId.int_id[1] = 0;
	qryId.char_id = qry_char_id;
	STprintf(qry_char_id, ERx("RSgrd4_%-.25s"), tbl->table_name);

	if (target->type == TARG_UNPROT_READ && RSnolock_read)
	{
		/* Read the shadow table in separate session with readlock=nolock */
        	status = IIsw_selectSingletonRepeated(RSlocal_conn_nlr.connHandle,
		         &RSlocal_conn_nlr.tranHandle, stmt, 3, pdesc, pdata, &qryId,
		         &tbl->grd4_qryHandle, nkeys, keydv, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1528, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
		         NULL, row->tbl_desc->table_name);
	}
	else
	{
		status = IIsw_selectSingletonRepeated(RSlocal_conn.connHandle,
		         &RSlocal_conn.tranHandle, stmt, 3, pdesc, pdata, &qryId,
		         &tbl->grd4_qryHandle, nkeys, keydv, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1528, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, &errParm,
		         NULL, row->tbl_desc->table_name);
	}
	if (status != OK)
		return (status);

	SW_CHA_TERM(row->old_trans_time, keydv[2]);
	for (i = 0; i < nkeys - 4; ++i)
		row->key_data[i] = keydv[i + 4];	/* struct */

	return (OK);
}


/*{
** Name:	set_dataval - set IIAPI_DATAVALUE structure
**
** Description:
**	Sets up the IIAPI_DATAVALUE structure from the column info cache.
**
** Inputs:
**	col	- pointer to IIAPI_DESCRIPTOR
**	pbuf	- pointer to address of RS_TRANS_ROW column or key data
**
** Outputs:
**	pbuf	- pointer to address of RS_TRANS_ROW column or key data
**	dv	- pointer to IIAPI_DATAVALUE structure to be set up
**
** Returns:
**	OK		- success
**	RS_MEM_ERR	- LVCH/LBYTE memory allocation failed
**
** Side effects:
**	pbuf is advanced to point beyond the value just set up.
*/
static STATUS
set_dataval(
IIAPI_DESCRIPTOR	*col,
PTR			*pbuf,
IIAPI_DATAVALUE		*dv)
{
	dv->dv_length = col->ds_length;

	switch (col->ds_dataType)
	{
	case IIAPI_INT_TYPE:
		switch (dv->dv_length)
		{
		case 1:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf,
				sizeof(i1));
			*pbuf = (PTR)dv->dv_value + sizeof(i1);
			break;
		case 2:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(i2));
			*pbuf = (PTR)dv->dv_value + sizeof(i2);
			break;
		case 4:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(i4));
			*pbuf = (PTR)dv->dv_value + sizeof(i4);
		case 8:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(i8));
			*pbuf = (PTR)dv->dv_value + sizeof(i8);
		}
		break;

	case IIAPI_MNY_TYPE:
	case IIAPI_FLT_TYPE:
		switch (dv->dv_length)
		{
		case 4:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(f4));
			*pbuf = (PTR)dv->dv_value + sizeof(f4);
			break;
		case 8:
			dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(f8));
			*pbuf = (PTR)dv->dv_value + sizeof(f8);
		}
		break;

	case IIAPI_TXT_TYPE:
	case IIAPI_VCH_TYPE:
	case IIAPI_VBYTE_TYPE:
		dv->dv_value = (char *)ME_ALIGN_MACRO(*pbuf, sizeof(i2));
		*pbuf = (PTR)dv->dv_value + dv->dv_length;
		break;

	case IIAPI_LVCH_TYPE:
	case IIAPI_LBYTE_TYPE:
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_GEOMC_TYPE :
		dv->dv_length = sizeof(RS_BLOB);
		dv->dv_value = (char *)MEreqmem(0, dv->dv_length, TRUE, NULL);
		if (dv->dv_value == NULL)
			return (RS_MEM_ERR);
		break;

	case IIAPI_CHA_TYPE:
	case IIAPI_CHR_TYPE:
	case IIAPI_DEC_TYPE:
	case IIAPI_DTE_TYPE:
	case IIAPI_BYTE_TYPE:
        case IIAPI_NBR_TYPE:
	case IIAPI_DATE_TYPE:
	default:
		dv->dv_value = (char *)*pbuf;
		*pbuf += dv->dv_length;
	}

	return (OK);
}

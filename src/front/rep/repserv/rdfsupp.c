/*
** Copyright (c) 1997, 2009 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <rpu.h>
# include <tblobjs.h>
#include <tbldef.h>
# include "conn.h"
# include "tblinfo.h"


/**
** Name:	rdfsupp.c - RDF support routines
**
** Description:
**	Defines
**		RSnrt_NumRepTbls	- number of replicated tables
**		RSnrc_NumRepCols	- number of replicated columns
**		RSitc_InitTblCache	- initialize table cache
**		RSicc_InitColCache	- initialize column cache
**		RSaro_AdjRemoteOwner	- adjust remote owner
**
** History:
**	21-jan-97 (joea)
**		Created from rdfsupp.sc in replicator library.
**	24-nov-97 (joea)
**		Change function names to avoid problems later in MVS. Use
**		IIAPI_DESCRIPTOR in RS_COLDESC.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	08-may-98 (joea)
**		Add length indicator for BYTE VARYING.
**	20-may-98 (joea)
**		Initialize dba_table member of RS_TBLDESC.
**	08-jul-98 (abbjo03)
**		Add has_blob, dlm_table_name, dlm_table_owner, shadow_table and
**		archive_table members to RS_TBLDESC. Change arguments to
**		RSicc_InitColCache. Add new arguments to IIsw_selectSingleton.
**	22-jul-98 (abbjo03)
**		In RSicc_InitColCache initialize col_name as a delimited
**		identifier. Initialize rem_table_owner in RSitc_InitTblCache.
**		Add RSaro_AdjRemoteOwner.
**	03-dec-98 (abbjo03)
**		Eliminate SIprintf error messaging. In RSnrt_NumRepTbls and
**		RSnrc_NumRepCols return 0 if there is an error SELECTing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	04-may-99 (abbjo03)
**		Correct the length specification for DECIMAL datatypes.
**	21-jul-99 (abbjo03)
**		Replace length of a date datatype by DB_DTE_LEN.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-oct-2002 (padni01) bug 106498 INGREP 108
**          Fetch row_width from iicolumns in RSitc_InitTblCache()
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw.. functions.
**      22-apr-2009 (joea)
**          In RSicc_InitColCache, retrieve column_always_ident and
**          column_bydefault_ident to support identity columns.
**      05-nov-2009 (joea)
**          Add test for BOOLEAN in RSicc_InitColCache.
**/

/*{
** Name:	RSnrt_NumRepTbls - number of replicated tables
**
** Description:
**	Returns the number of tables registered for replication in the
**	local database.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	The count of rows in dd_regist_tables.
*/
i4
RSnrt_NumRepTbls()
{
	i4			num_tbls;
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_GETEINFOPARM	errParm;

	SW_COLDATA_INIT(cdata, num_tbls);
	if (IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle,
		ERx("SELECT COUNT(*) FROM dd_regist_tables r, iitables s WHERE \
		r.table_name = s.table_name and r.table_owner = s.table_owner"), 
		0, NULL, NULL, 1, &cdata, NULL, NULL, &stmtHandle, &getQinfoParm,
		&errParm)
		!= IIAPI_ST_SUCCESS)
	{
		IIsw_close(stmtHandle, &errParm);
		return (0);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);

	return ((i4)num_tbls);
}


/*{
** Name:	RSnrc_NumRepCols - number of replicated columns
**
** Description:
**	Returns the number of columns registered for replication in the
**	local database.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	The count of rows in dd_regist_columns that are replicated.
*/
i4
RSnrc_NumRepCols()
{
	i4			num_cols;
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;

	SW_COLDATA_INIT(cdata, num_cols);
	if (IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle,
		ERx("SELECT COUNT(*) FROM dd_regist_columns WHERE \
column_sequence != 0"),
		0, NULL, NULL, 1, &cdata, NULL, NULL, &stmtHandle, 
		&getQinfoParm, &errParm)
		!= IIAPI_ST_SUCCESS)
	{
		IIsw_close(stmtHandle, &errParm);
		return (0);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);

	return ((i4)num_cols);
}


/*{
** Name:	RSitc_InitTblCache - initialize table cache
**
** Description:
**	Read the replicated table information from dd_regist_tables into
**	the RepServer RDF cache.
**
** Inputs:
**	t		- the base of the table cache array
**	num_tbls	- the number of tables to be held in the array
**
** Outputs:
**	none
**
** Returns:
**	FAIL if the actual number of tables retrieved exceeds num_tbls
**	or if there is an error in the select.
*/
STATUS
RSitc_InitTblCache(
RS_TBLDESC	*t,
i4		num_tbls)
{
	i4			i;
	IIAPI_DATAVALUE		cdata[7];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	stmtHandle = NULL;
	for (i = 0; i <= num_tbls; ++i)
	{
		SW_COLDATA_INIT(cdata[0], t->table_no);
		SW_COLDATA_INIT(cdata[1], t->table_name);
		SW_COLDATA_INIT(cdata[2], t->table_owner);
		SW_COLDATA_INIT(cdata[3], t->cdds_no);
		SW_COLDATA_INIT(cdata[4], t->cdds_lookup_table);
		SW_COLDATA_INIT(cdata[5], t->prio_lookup_table);
		SW_COLDATA_INIT(cdata[6], t->row_width);
		status = IIsw_selectLoop(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("SELECT r.table_no, r.table_name, r.table_owner, \
r.cdds_no, r.cdds_lookup_table, r.prio_lookup_table, s.row_width FROM \
dd_regist_tables r, iitables s where r.table_name = s.table_name AND \
r.table_owner = s.table_owner ORDER BY 1"),
			0, NULL, NULL, 7, cdata, &stmtHandle, 
			&getQinfoParm, &errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		STtrmwhite(t->table_name);
		STtrmwhite(t->table_owner);
		if (STequal(t->table_owner, RSlocal_conn.owner_name))
			t->dba_table = TRUE;
		else
			t->dba_table = FALSE;
		t->has_blob = FALSE;
		STtrmwhite(t->cdds_lookup_table);
		STtrmwhite(t->prio_lookup_table);
		RPedit_name(EDNM_DELIMIT, t->table_name, t->dlm_table_name);
		RPedit_name(EDNM_DELIMIT, t->table_owner, t->dlm_table_owner);
		STcopy(t->dlm_table_owner, t->rem_table_owner);
		if (RPtblobj_name(t->table_name, t->table_no, TBLOBJ_SHA_TBL,
			t->shadow_table) != OK || RPtblobj_name(t->table_name,
			t->table_no, TBLOBJ_ARC_TBL, t->archive_table) != OK)
		{
			messageit(1, 1816, ERx("RSitc_InitTblCache"),
				t->table_name);
			IIsw_close(stmtHandle, &errParm);
			IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
			return (FAIL);
		}
		++t;
	}
	if (status == IIAPI_ST_NO_DATA && i != num_tbls && status !=
		IIAPI_ST_SUCCESS)
	{
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);

	return (OK);
}


/*{
** Name:	RSicc_InitColCache - initialize column cache
**
** Description:
**	Read the replicated column information for a given table from
**	dd_regist_columns into the RepServer RDF cache.
**
** Inputs:
**	t		- the table descriptor
**	c		- the base of this table columns' cache array
**
** Outputs:
**	num_cols	- the number of columns held in the array
**
** Returns:
**	FAIL if there is an error retrieving from dd_registered_columns
**	or if there are no registered columns.
** History:
**	25-Nov-2003 (gupsh01)
**	    Added cases for NCHAR, NVARCHAR and LONG NVARCHAR to support
**	    Unicode datatypes.
**	19-Aug-2006 (gupsh01)
**	    Added case for ANSI datetime data types.
**      10-Oct-2006(hweho01)
**          Correct the ds_dataType and ds_legnth settings for DATE type.
**	08-Mar-2007 (gupsh01)
**	    For nchar and nvarchar data type restrict the maximum allowed
**	    Length is DB_MAXSTRING.
*/
STATUS
RSicc_InitColCache(
RS_TBLDESC	*t,
RS_COLDESC	*c,
short		*num_cols)
{
	char			datatype[DB_MAXNAME+1];
	char			col_name[DB_MAXNAME+1];
	char			nullable[2];
	char			always_ident[2];
	char			bydefault_ident[2];
	i4			col_length;
	i4			col_scale;
	short			i;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata[9];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	SW_QRYDESC_INIT(pdesc[0], IIAPI_CHA_TYPE, DB_MAXNAME, FALSE);
	SW_QRYDESC_INIT(pdesc[1], IIAPI_CHA_TYPE, DB_MAXNAME, FALSE);
	SW_QRYDESC_INIT(pdesc[2], IIAPI_INT_TYPE, 4, FALSE);
	SW_PARMDATA_INIT(pdata[0], t->table_name, STlength(t->table_name),
		FALSE);
	SW_PARMDATA_INIT(pdata[1], t->table_owner, STlength(t->table_owner),
		FALSE);
	SW_PARMDATA_INIT(pdata[2], &t->table_no, 4, FALSE);
	SW_COLDATA_INIT(cdata[1], col_name);
	SW_COLDATA_INIT(cdata[3], datatype);
	SW_COLDATA_INIT(cdata[4], col_length);
	SW_COLDATA_INIT(cdata[5], col_scale);
	SW_COLDATA_INIT(cdata[6], nullable);
	SW_COLDATA_INIT(cdata[7], always_ident);
	SW_COLDATA_INIT(cdata[8], bydefault_ident);
	stmtHandle = NULL;
	for (i = 0; ; ++i)
	{
		SW_COLDATA_INIT(cdata[0], c->col_sequence);
		SW_COLDATA_INIT(cdata[2], c->key_sequence);
		status = IIsw_selectLoop(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("SELECT d.column_sequence, d.column_name, \
d.key_sequence, column_datatype, column_length, column_scale, column_nulls, \
column_always_ident, column_bydefault_ident \
FROM iicolumns i, dd_regist_columns d WHERE i.table_name = ~V AND \
i.table_owner = ~V AND d.table_no = ~V AND i.column_name = d.column_name AND \
d.column_sequence != 0 ORDER BY 1"),
			3, pdesc, pdata, 9, cdata, &stmtHandle, 
			&getQinfoParm, &errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(col_name, cdata[1]);
		RPedit_name(EDNM_DELIMIT, col_name, c->col_name);
		c->coldesc.ds_columnName = c->col_name;
		c->coldesc.ds_length = (II_INT2)col_length;
		SW_CHA_TERM(datatype, cdata[3]);
		if (STequal(datatype, ERx("INTEGER")))
		{
			c->coldesc.ds_dataType = DB_INT_TYPE;
		}
		else if (STequal(datatype, ERx("FLOAT")))
		{
			c->coldesc.ds_dataType = DB_FLT_TYPE;
		}
		else if (STequal(datatype, ERx("CHAR")))
		{
			c->coldesc.ds_dataType = DB_CHA_TYPE;
		}
		else if (STequal(datatype, ERx("VARCHAR")))
		{
			c->coldesc.ds_dataType = DB_VCH_TYPE;
			c->coldesc.ds_length += sizeof(i2);
		}
		else if (STequal(datatype, ERx("DATE")))
		{
			c->coldesc.ds_dataType = DB_DTE_TYPE;
			c->coldesc.ds_length = (II_LONG)DB_DTE_LEN;
		}
		else if (STequal(datatype, ERx("ANSIDATE")))
		{
			c->coldesc.ds_dataType = DB_ADTE_TYPE;
			c->coldesc.ds_length = (II_LONG)4;
		}
		else if (STequal(datatype, ERx("INGRESDATE")))
		{
			c->coldesc.ds_dataType = DB_DTE_TYPE;
			c->coldesc.ds_length = (II_LONG)DB_DTE_LEN;
		}
		else if (STequal(datatype, ERx("TIME WITHOUT TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TMWO_TYPE;
			c->coldesc.ds_length = (II_LONG)10;
		}
		else if (STequal(datatype, ERx("TIME WITH TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TMW_TYPE;
			c->coldesc.ds_length = (II_LONG)10;
		}
		else if (STequal(datatype, ERx("TIME WITH LOCAL TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TME_TYPE;
			c->coldesc.ds_length = (II_LONG)10;
		}
		else if (STequal(datatype, ERx("TIMESTAMP WITHOUT TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TSWO_TYPE;
			c->coldesc.ds_length = (II_LONG)14;
		}
		else if (STequal(datatype, ERx("TIMESTAMP WITH TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TSW_TYPE;
			c->coldesc.ds_length = (II_LONG)14;
		}
		else if (STequal(datatype, ERx("TIMESTAMP WITH LOCAL TIME ZONE")))
		{
			c->coldesc.ds_dataType = DB_TSTMP_TYPE;
			c->coldesc.ds_length = (II_LONG)14;
		}
		else if (STequal(datatype, ERx("INTERVAL DAY TO SECOND")))
		{
			c->coldesc.ds_dataType = DB_INDS_TYPE;
			c->coldesc.ds_length = (II_LONG)12;
		}
		else if (STequal(datatype, ERx("INTERVAL YEAR TO MONTH")))
		{
			c->coldesc.ds_dataType = DB_INYM_TYPE;
			c->coldesc.ds_length = (II_LONG)3;
		}
                else if (STequal(datatype, "BOOLEAN"))
                {
                    c->coldesc.ds_dataType = DB_BOO_TYPE;
                    c->coldesc.ds_length = DB_BOO_LEN;
                }

		/*
		** Note:  OBJECT_KEY and TABLE_KEY are mapped to CHAR
		** for iicolumns.column_datatype, but appear as such
		** in iicolumns.column_internal_datatype.
		** Similarly, SECURITY_LABEL is mapped to CHAR for
		** iicolumns.column_datatype, but appears as such in
		** iicolumns.column_internal_datatype.
		*/

		/*
		** DECIMAL is supported by OpenIngres, but not yet by
		** the gateways.
		*/
		else if (STequal(datatype, ERx("DECIMAL")))
		{
			c->coldesc.ds_dataType = DB_DEC_TYPE;
			c->coldesc.ds_precision = c->coldesc.ds_length;
			c->coldesc.ds_scale = (II_INT2)col_scale;
			c->coldesc.ds_length =
				DB_PREC_TO_LEN_MACRO(c->coldesc.ds_length);
		}

		/* MONEY is not OpenSQL */
		else if (STequal(datatype, ERx("MONEY")))
		{
			c->coldesc.ds_dataType = DB_MNY_TYPE;
			c->coldesc.ds_length = (II_LONG)8;
		}

		/* the following two are QUEL legacy datatypes */
		else if (STequal(datatype, ERx("C")))
		{
			c->coldesc.ds_dataType = DB_CHR_TYPE;
		}
		else if (STequal(datatype, ERx("TEXT")))
		{
			c->coldesc.ds_dataType = DB_TXT_TYPE;
			c->coldesc.ds_length += sizeof(i2);
		}

		/* the following four are only in OpenIngres */
		else if (STequal(datatype, ERx("BYTE")))
		{
			c->coldesc.ds_dataType = DB_BYTE_TYPE;
		}
		else if (STequal(datatype, ERx("BYTE VARYING")))
		{
			c->coldesc.ds_dataType = DB_VBYTE_TYPE;
			c->coldesc.ds_length += sizeof(i2);
		}
		else if (STequal(datatype, ERx("LONG BYTE")))
		{
			c->coldesc.ds_dataType = DB_LBYTE_TYPE;
			t->has_blob = TRUE;
		}
		else if (STequal(datatype, ERx("LONG VARCHAR")))
		{
			c->coldesc.ds_dataType = DB_LVCH_TYPE;
			t->has_blob = TRUE;
		}
		else if (STequal(datatype, ERx("NCHAR")))
		{
			c->coldesc.ds_dataType = DB_NCHR_TYPE;
			c->coldesc.ds_length = 
			  c->coldesc.ds_length * sizeof(wchar_t);
			if (c->coldesc.ds_length > DB_MAXSTRING)
			  c->coldesc.ds_length = DB_MAXSTRING;
		}
		else if (STequal(datatype, ERx("NVARCHAR")))
		{
			c->coldesc.ds_dataType = DB_NVCHR_TYPE;
			c->coldesc.ds_length = sizeof (i2) +
			  c->coldesc.ds_length * sizeof(wchar_t);
			if (c->coldesc.ds_length > DB_MAXSTRING)
			  c->coldesc.ds_length = DB_MAXSTRING;
		}
		else if (STequal(datatype, ERx("LONG NVARCHAR")))
		{
			c->coldesc.ds_dataType = DB_LNVCHR_TYPE;
			t->has_blob = TRUE;
		}
		else
		{
			c->coldesc.ds_dataType = DB_NODT;
		}

		nullable[1] = EOS;
		if (CMcmpcase(nullable, ERx("Y")) == 0)
			c->coldesc.ds_nullable = TRUE;
		else
			c->coldesc.ds_nullable = FALSE;
		always_ident[1] = EOS;
		if (CMcmpcase(always_ident, "Y") == 0)
	            c->col_ident = COL_IDENT_ALWAYS;
	        else if (CMcmpcase(bydefault_ident, "Y") == 0)
                    c->col_ident = COL_IDENT_BYDEFAULT;
		else
		    c->col_ident = COL_IDENT_NONE;
		++c;
	}
	if (status != IIAPI_ST_NO_DATA && status != IIAPI_ST_SUCCESS)
	{
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	*num_cols = i;

	return (OK);
}


/*{
** Name:	RSaro_AdjRemoteOwner	- adjust remote owner
**
** Description:
**	Adjusts the rem_table_owner member of RS_TBLDESC based on the local and
**	target DBAs.
**	
** Inputs:
**      tbl	- table descriptor
**
** Outputs:
**	tbl	- adjusted table descriptor
**
** Returns:
**	none
*/
void
RSaro_AdjRemoteOwner(
RS_TBLDESC	*tbl,
char		*target_dba)
{
	if (tbl->dba_table && !STequal(tbl->table_owner, target_dba))
		RPedit_name(EDNM_DELIMIT, target_dba, tbl->rem_table_owner);
	else
		STcopy(tbl->dlm_table_owner, tbl->rem_table_owner);
}

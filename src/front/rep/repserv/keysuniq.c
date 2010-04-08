/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "rdfint.h"
# include "tblinfo.h"

/**
** Name:	keysuniq.c - check for unique keys
**
** Description:
**	Defines
**		RSkeys_check_unique	- check for unique keys on all tables
**		check_unique_keys	- check for unique keys
**		check_indexes		- check for unique indexes
**
** History:
**	16-dec-96 (joea)
**		Created based on alluniqk.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to check_unique_keys.
**	20-jun-97 (joea)
**		Remove trailing blanks from table_name and table_owner.
**	21-apr-98 (joea)
**		Temporarily comment out body of function while converting
**		remainder of RepServ to OpenAPI.
**	21-may-98 (joea)
**		Convert to use OpenAPI.
**	08-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton.
**	30-jul-98 (abbjo03)
**		Change queries to repeated.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm to the parameter list of II_sw.. calls.
**/

# define NO_UNIQUE_KEYS		(-1)

# define MAX_UNIQUE_INDEXES	16


static II_PTR qryHandle[5];


static STATUS check_unique_keys(RS_CONN *conn, RS_TBLDESC *tbl);
static STATUS check_indexes(RS_CONN *conn, RS_TBLDESC *tbl);


/*{
** Name:	RSkeys_check_unique	- check for unique keys on all tables
**
** Description:
**	Checks all tables registered for the db_no and server_no to see if keys
**	or indexes force the columns marked as replication keys to be unique.
**
** Inputs:
**	dbname		- the name of the database to be checked
**	db_no		- the database number
**	server_no	- the server number
**
** Outputs:
**	none
**
** Returns:
**	status from check_unique_keys
*/
STATUS
RSkeys_check_unique(
RS_CONN	*conn)
{
	i4		i;
	STATUS		status;
	RS_TBLDESC	*tbl;
	IIAPI_GETEINFOPARM	errParm;

	/*
	** RSkeys_check_unique can be called against multiple databases, but
	** only once at the start of each connection. So we need to initialize
	** the query handles for each target.
	*/
	for (i = 0; i < sizeof(qryHandle) / sizeof(qryHandle[0]); ++i)
		qryHandle[i] = NULL;
	/*
	** FIXME:  In REP 1.0, we used to attempt to verify all tables. In
	** REP 1.1, this was changed to only look at tables which supposedly
	** were in a FP or PRO CDDS serviced by the current server number.
	** However, because of horizontal partitioning, some tables could be
	** bypassed.  This goes back to checking all tables, but not issuing
	** a warning or error if a table doesn't exist.
	*/
	tbl = RSrdf_svcb.tbl_info;
	for (i = 0; i < RSrdf_svcb.num_tbls; ++i, ++tbl)
	{
		status = check_unique_keys(conn, tbl);
		if (status == NO_UNIQUE_KEYS)
			/* Table does not have unique keys enforced */
			messageit(1, 1536, tbl->table_owner, tbl->table_name,
				conn->db_name);
		IIsw_commit(&conn->tranHandle, &errParm);
	}

	return (status);
}


/*{
** Name:	check_unique_keys - check for unique keys
**
** Description:
**	Checks to see if keys or indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	conn	- connection information block
**	tbl	- table description block
**
** Outputs:
**	none
**
** Returns:
**	OK	- table has unique keys
**	-1	- no unique keys
**	1523	- INGRES error looking for table
**	1524	- INGRES error looking up columns
**	Other error returns from check_indexes (see below)
*/
static STATUS
check_unique_keys(
RS_CONN		*conn,
RS_TBLDESC	*tbl)
{
	char			unique_rule[2];
	char			table_indexes[2];
	i4			cnt1;
	i4			cnt2;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata[2];
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("RScheck_unique_keys1") };

	SW_QRYDESC_INIT(pdesc[0], IIAPI_CHA_TYPE, sizeof(tbl->table_name),
		FALSE);
	SW_QRYDESC_INIT(pdesc[1], IIAPI_CHA_TYPE, sizeof(tbl->table_owner),
		FALSE);
	SW_PARMDATA_INIT(pdata[0], tbl->table_name, STlength(tbl->table_name),
		FALSE);
	SW_PARMDATA_INIT(pdata[1], tbl->table_owner, STlength(tbl->table_owner),
		FALSE);
	SW_COLDATA_INIT(cdata[0], unique_rule);
	SW_COLDATA_INIT(cdata[1], table_indexes);
	if (IIsw_selectSingletonRepeated(conn->connHandle, &conn->tranHandle,
		ERx("SELECT unique_rule, table_indexes FROM iitables WHERE \
table_name = $0 = ~V AND table_owner = $1 = ~V"),
		2, pdesc, pdata, &qryId, &qryHandle[0], 2, cdata, &stmtHandle,
		&getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1523, errParm.ge_errorCode, tbl->table_owner,
			tbl->table_name, conn->db_name);
		IIsw_close(stmtHandle, &errParm);
		return (1523);
	}
	if (!IIsw_getRowCount(&getQinfoParm))
	{
		IIsw_close(stmtHandle, &errParm);
		/* table doesn't exist */
		return (OK);
	}
	IIsw_close(stmtHandle, &errParm);
	SW_CHA_TERM(unique_rule, cdata[0]);
	SW_CHA_TERM(table_indexes, cdata[1]);

	if (CMcmpcase(unique_rule, ERx("U")) == 0)
	{
		cnt1 = cnt2 = 0;
		SW_COLDATA_INIT(cdata[0], cnt1);
		qryId.char_id = ERx("RScheck_unique_keys2");
		if (IIsw_selectSingletonRepeated(conn->connHandle,
			&conn->tranHandle, ERx("SELECT COUNT(*) FROM iicolumns \
WHERE table_name = $0 = ~V AND key_sequence > 0 AND table_owner = $1 = ~V"),
			2, pdesc, pdata, &qryId, &qryHandle[1], 1, cdata,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1524, errParm.ge_errorCode,
				tbl->table_owner, tbl->table_name,
				conn->db_name);
			IIsw_close(stmtHandle, &errParm);
			return (1524);
		}
		IIsw_close(stmtHandle, &errParm);

		SW_QRYDESC_INIT(pdesc[2], IIAPI_INT_TYPE, sizeof(tbl->table_no),
			FALSE);
		SW_PARMDATA_INIT(pdata[2], &tbl->table_no,
			sizeof(tbl->table_no), FALSE);
		SW_COLDATA_INIT(cdata[0], cnt2);
		qryId.char_id = ERx("RScheck_unique_keys3");
		if (IIsw_selectSingletonRepeated(conn->connHandle,
			&conn->tranHandle, ERx("SELECT COUNT(*) FROM iicolumns \
i, dd_regist_columns d WHERE i.column_name = d.column_name AND i.table_name = \
$0 = ~V AND i.table_owner = $1 = ~V AND table_no = $2 = ~V AND i.key_sequence \
> 0 AND d.key_sequence > 0"),
			3, pdesc, pdata, &qryId, &qryHandle[2], 1, cdata,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1524, errParm.ge_errorCode,
				tbl->table_owner, tbl->table_name,
				conn->db_name);
			IIsw_close(stmtHandle, &errParm);
			return (1524);
		}
		IIsw_close(stmtHandle, &errParm);
		if (cnt1 && cnt1 == cnt2)	/* base table keys unique */
			return (OK);
	}

	if (CMcmpcase(table_indexes, ERx("Y")) == 0)
		return (check_indexes(conn, tbl));

	return (NO_UNIQUE_KEYS);
}


/*{
** Name:	check_indexes - check for unique indexes
**
** Description:
**	Checks to see if indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	conn	- connection information block
**	tbl	- table description block
**
** Outputs:
**	none
**
** Returns:
**	OK	- the table has a unique index
**	1527	- too many unique indexes
**	1525	- retrieval error
**	-1	- table does not have unique indexes
**/
static STATUS
check_indexes(
RS_CONN		*conn,
RS_TBLDESC	*tbl)
{
	struct
	{
		char	name[DB_MAXNAME+1];
		char	owner[DB_MAXNAME+1];
	} idx[MAX_UNIQUE_INDEXES];
	i4			cnt1;
	i4			cnt2;
	i4			i;
	i4			num_idxs;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata[2];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("check_indexes1") };

	SW_QRYDESC_INIT(pdesc[0], IIAPI_CHA_TYPE, sizeof(tbl->table_name),
		FALSE);
	SW_QRYDESC_INIT(pdesc[1], IIAPI_CHA_TYPE, sizeof(tbl->table_owner),
		FALSE);
	SW_PARMDATA_INIT(pdata[0], tbl->table_name, STlength(tbl->table_name),
		FALSE);
	SW_PARMDATA_INIT(pdata[1], tbl->table_owner, STlength(tbl->table_owner),
		FALSE);
	stmtHandle = NULL;
	for (i = 0; ; ++i)
	{
		if (i > MAX_UNIQUE_INDEXES)
		{
			messageit(1, 1527, tbl->table_owner, tbl->table_name);
			IIsw_close(stmtHandle, &errParm);
			return (1527);
		}
		SW_COLDATA_INIT(cdata[0], idx[i].name);
		SW_COLDATA_INIT(cdata[1], idx[i].owner);
		status = IIsw_selectLoop(conn->connHandle, &conn->tranHandle,
			ERx("SELECT index_name, index_owner FROM iiindexes \
WHERE base_name = ~V AND base_owner = ~V AND unique_rule = 'U'"),
			2, pdesc, pdata, 2, cdata, &stmtHandle, &getQinfoParm, 
			&errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(idx[i].name, cdata[0]);
		SW_CHA_TERM(idx[i].owner, cdata[1]);
	}
	if (status != IIAPI_ST_NO_DATA && status != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1525, tbl->table_owner, tbl->table_name,
			errParm.ge_errorCode);
		IIsw_close(stmtHandle, &errParm);
		return (1525);
	}
	IIsw_close(stmtHandle, &errParm);
	num_idxs = i;

	SW_QRYDESC_INIT(pdesc[2], IIAPI_INT_TYPE, sizeof(tbl->table_no), FALSE);
	SW_PARMDATA_INIT(pdata[2], &tbl->table_no, sizeof(tbl->table_no),
		FALSE);
	for (i = 0; i < num_idxs; ++i)
	{
		cnt1 = cnt2 = 0;
		SW_QRYDESC_INIT(pdesc[0], IIAPI_CHA_TYPE, sizeof(idx[i].name),
			FALSE);
		SW_QRYDESC_INIT(pdesc[1], IIAPI_CHA_TYPE, sizeof(idx[i].owner),
			FALSE);
		SW_PARMDATA_INIT(pdata[0], idx[i].name, STlength(idx[i].name),
			FALSE);
		SW_PARMDATA_INIT(pdata[1], idx[i].owner, STlength(idx[i].owner),
			FALSE);
		SW_COLDATA_INIT(cdata[0], cnt1);
		qryId.char_id = ERx("RScheck_indexes1");
		if (IIsw_selectSingletonRepeated(conn->connHandle,
			&conn->tranHandle, ERx("SELECT COUNT(*) FROM \
iiindex_columns WHERE index_name = $0 = ~V AND index_owner = $1 = ~V"),
			2, pdesc, pdata, &qryId, &qryHandle[3], 1, cdata,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1525, tbl->table_owner, tbl->table_name,
				errParm.ge_errorCode);
			IIsw_close(stmtHandle, &errParm);
			return (1525);
		}
		IIsw_close(stmtHandle, &errParm);

		SW_COLDATA_INIT(cdata[0], cnt2);
		if (IIsw_selectSingletonRepeated(conn->connHandle,
			&conn->tranHandle, ERx("SELECT COUNT(*) FROM \
iiindex_columns i, dd_regist_columns d WHERE index_name = $0 = ~V AND \
index_owner = $1 = ~V AND i.column_name = d.column_name AND table_no = $2 = ~V \
AND d.key_sequence > 0"),
			3, pdesc, pdata, &qryId, &qryHandle[4], 1, cdata,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1525, tbl->table_owner, tbl->table_name,
				errParm.ge_errorCode);
			IIsw_close(stmtHandle, &errParm);
			return (1525);
		}
		IIsw_close(stmtHandle, &errParm);
		/* this index is unique and only uses key replicator columns */
		if (cnt1 && cnt1 == cnt2)
			return (OK);
	}

	return (NO_UNIQUE_KEYS);	/* no unique indexes */
}

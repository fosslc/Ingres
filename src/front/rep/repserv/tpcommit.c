/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <si.h>
# include <tm.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "recover.h"
# include "tblinfo.h"
# include "distq.h"
# include "repserv.h"

/**
** Name:	tpcommit.c - two-phase commit processing
**
** Description:
**	Defines
**		RScommit		- two-phase commit processing
**		RSdtrans_id_register	- register distributed transaction ID
**		mktimestamp		- make time stamp
**
** History:
**	03-jan-97 (joea)
**		Created based on commitit.sc in replicator library.
**	03-feb-97 (joea)
**		Do not write the shadow table name to the commit log. Use a
**		single format string for all the writes. Rollback on negative
**		return from get_t_no. Add the code from gettno.sc. Rename
**		commitit to RScommit. Replace parts of RLIST by REP_TARGET. Add
**		DQ_ROW and RS_TBLDESC arguments to RScommit.
**	26-mar-97 (joea)
**		Include distq.h before records.h.
**	22-aug-97 (joea)
**		Replace DQ_ROW by RS_TRANS_ROW.
**	30-oct-97 (joea)
**		Remove redundant messages 1284 and 1285.
**	11-dec-97 (joea)
**		Eliminate RS_TBLDESC parameter, use the row->tbl_desc instead.
**		Add RS_TARGET argument and eliminate RStarget global.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	19-may-98 (joea)
**		Use defines for recovery entry numbers.
**	08-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	31-aug-99 (abbjo03) bug 98604
**		Add RSdtrans_id_reregister to reinstate a distributed
**		transaction ID.
**	27-sep-99 (abbjo03) bugs 98604, 99005
**		Replace RSdtrans_id_reregister by RSdtrans_id_get_last. Add
**		calls to IIsw_releaseXID to free the trans ID handle.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw... functions.
**/

# define MAX_TMWORDS	6


GLOBALDEF bool	RStwo_phase = TRUE;


static i4	high_limit = 0;
static i4	low = 0;
static II_PTR	dtrans_id_handle = NULL;
static bool	first = TRUE;

static char	*log_format = ERx("%d!%d!%d!%d!%s!%s!%d!%d!%d!%s!%s\n");


static void mktimestamp(char *timestr, char *timestamp);


/*{
** Name:	RScommit - two-phase commit processing
**
** Description:
**	Prepares to commit locally, commits remotely, commits locally.
**	Logs each step of the commit so that a reasonable attempt can
**	be made to recover if it does not complete.
**
** Inputs:
**	target	- target database, CDDS and connection number
**	row	- distribution queue row
**
** Outputs:
**	none
**
** Returns:
**	OK or Ingres error
**
** Side effects:
**	A distributed transaction is committed using incomplete two-phase
**	commit (a prepare is only done on the local database, so the target
**	database is not aware it is part of a distributed transaction).
*/
STATUS
RScommit(
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	i4			high = (i4)RSlocal_conn.db_no;
	SYSTIME			now;
	char			timestamp[21];
	char			timestr[27];
	i4			start_entry_no;
	i4			prepare_entry_no;
	i4			remote_entry_no;
	i4			complete_entry_no;
	RS_TBLDESC		*tbl = row->tbl_desc;
	RS_CONN			*local_conn = &RSlocal_conn;
	RS_CONN			*target_conn = &RSconns[target->conn_no];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;

	if (RStwo_phase)	/* two-phase commit is on */
	{
		start_entry_no = RS_2PC_BEGIN;
		prepare_entry_no = RS_PREP_COMMIT;
		remote_entry_no = RS_REMOTE_COMMIT;
		complete_entry_no = RS_2PC_END;
	}
	else			/* two-phase commit is off */
	{
		start_entry_no = RS_NPC_BEGIN;
		remote_entry_no = RS_NPC_REM_COMMIT;
		complete_entry_no = RS_NPC_END;
	}

	TMnow(&now);
	TMstr(&now, timestr);
	mktimestamp(timestr, timestamp);

	SIfprintf(RScommit_fp, log_format, start_entry_no, high, low,
		(i4)target->db_no, tbl->table_owner, tbl->table_name,
		tbl->table_no, row->rep_key.trans_id, row->rep_key.seq_no,
		timestamp, "Start of commit");
	SIflush(RScommit_fp);

	if (RStwo_phase)	/* only prepare to commit if two-phase is on */
	{
		/*
		** Prepare to commit. The high value is %d, The low value is %d
		*/
		messageit(5, 1277, high, low);

		status = IIsw_prepareCommit(&local_conn->tranHandle, &errParm);
		if (status != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1214);
			IIsw_rollback(&target_conn->tranHandle, &errParm);
			IIsw_rollback(&local_conn->tranHandle, &errParm);
			IIsw_releaseXID(&dtrans_id_handle);
			return (status);
		}

		SIfprintf(RScommit_fp, log_format, prepare_entry_no, high, low,
			(i4)target->db_no, tbl->table_owner, tbl->table_name,
			tbl->table_no, row->rep_key.trans_id,
			row->rep_key.seq_no, timestamp, "Prepare to commit");
		SIflush(RScommit_fp);
	}

	status = IIsw_commit(&target_conn->tranHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
	{
		IIsw_rollback(&target_conn->tranHandle, &errParm);
		if (IIsw_rollback(&local_conn->tranHandle, &errParm) !=
			IIAPI_ST_SUCCESS)
		{
			RSdo_recover = TRUE;
			messageit(1, 1215);
			messageit(1, 1216);
		}
		else
		{
			messageit(1, 1215);
		}
		IIsw_releaseXID(&dtrans_id_handle);

		SIfprintf(RScommit_fp, log_format, remote_entry_no, high,
			low, (i4)target->db_no, tbl->table_owner,
			tbl->table_name, tbl->table_no, row->rep_key.trans_id,
			row->rep_key.seq_no, timestamp, "Local rollback");
		SIfprintf(RScommit_fp, log_format, complete_entry_no, high,
			low, (i4)target->db_no, tbl->table_owner,
			tbl->table_name, tbl->table_no, row->rep_key.trans_id,
			row->rep_key.seq_no, timestamp, "Commit complete");
		SIflush(RScommit_fp);
		return (status);
	}

	SIfprintf(RScommit_fp, log_format, remote_entry_no, high, low,
		(i4)target->db_no, tbl->table_owner, tbl->table_name,
		tbl->table_no, row->rep_key.trans_id, row->rep_key.seq_no,
		timestamp, "Remote commit");
	SIflush(RScommit_fp);

	status = IIsw_commit(&local_conn->tranHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
	{
		RSdo_recover = TRUE;
		IIsw_releaseXID(&dtrans_id_handle);
		return (status);
	}
	IIsw_releaseXID(&dtrans_id_handle);

	SIfprintf(RScommit_fp, log_format, complete_entry_no, high, low,
		(i4)target->db_no, tbl->table_owner, tbl->table_name,
		tbl->table_no, row->rep_key.trans_id, row->rep_key.seq_no,
		timestamp, "Commit complete");
	SIflush(RScommit_fp);

	return (OK);
}


/*{
** Name:	mktimestamp - make time stamp
**
** Description:
**	Convert the system specific time string (from TMstr) into a
**	Replicator timestamp of the form dd-mmm-yyy hh:mm:ss.
**	mktimestamp() is only intended as a temporary fix to the problem
**	regarding availablity of TMbreak().
**
** Inputs:
**	timestr
**
** Outputs:
**	timestamp
**
** Returns:
**	none
*/
static void
mktimestamp(
char *timestr,
char *timestamp)
{
	char	*tmwords[MAX_TMWORDS];
	char	*dom;
	char	*mon;
	char	*yr;
	char	*hms;
	i4	count;

	*timestamp = EOS;
	count = MAX_TMWORDS;

# ifdef VMS
	if CMspace(timestr)
		CMnext(timestr);
	RPgetwords(ERx(" -"), timestr, &count, tmwords);
	if (count < 3)
		return;
	dom = tmwords[0];
	mon = tmwords[1];
	yr = tmwords[2];
	hms = tmwords[3];
# else
	STgetwords(timestr, &count, tmwords);
	if (count < 5)
		return;
	mon = tmwords[1];
	dom = tmwords[2];
	hms = tmwords[3];
	yr = tmwords[4];
# endif

	STprintf(timestamp, ERx("%.2s-%.3s-%.4s %.8s"), dom, mon, yr, hms);
}


/*{
** Name:	RSdtrans_id_register - register distributed transaction ID
**
** Description:
**	Register a distributed transaction ID.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
STATUS
RSdtrans_id_register(
RS_TARGET	*target)
{
	RS_CONN			*conn = &RSlocal_conn;
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	IIAPI_GETQINFOPARM	getQinfoParm;

	if (first || low == high_limit)
	{
		status = IIsw_query(conn->connHandle, &conn->tranHandle,
			ERx("UPDATE dd_last_number SET last_number = \
last_number + 100 WHERE column_name = 'next_transaction_id'"),
			0, NULL, NULL, NULL, NULL, &stmtHandle, &getQinfoParm,
			&errParm);
		if (RSerror_check(1234, ROWS_SINGLE_ROW, stmtHandle, 
			 &getQinfoParm, &errParm, NULL) != OK)
		{
			IIsw_rollback(&conn->tranHandle, &errParm);
			return (FAIL);
		}

		SW_COLDATA_INIT(cdata, high_limit);
		status = IIsw_selectSingleton(conn->connHandle,
			&conn->tranHandle,
			ERx("SELECT last_number FROM dd_last_number WHERE \
column_name = 'next_transaction_id'"),
			0, NULL, NULL, 1, &cdata, NULL, NULL, &stmtHandle,
			 &getQinfoParm, &errParm);
		if (RSerror_check(1234, ROWS_SINGLE_ROW, stmtHandle, 
			 &getQinfoParm, &errParm, NULL) != OK)
		{
			IIsw_rollback(&conn->tranHandle, &errParm);
			return (FAIL);
		}
		first = FALSE;
		low = high_limit - 100;
	}
	++low;
	IIsw_commit(&conn->tranHandle, &errParm);

	status = IIsw_registerXID((II_UINT4)conn->db_no, (II_UINT4)low, ERx(""),
		&conn->tranHandle);
	dtrans_id_handle = conn->tranHandle;
	return (status);
}


II_PTR
RSdtrans_id_get_last()
{
	return (dtrans_id_handle);
}

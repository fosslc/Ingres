/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <targtype.h>
# include <tblobjs.h>
# include "conn.h"
# include "recover.h"

/**
** Name:	recovlog.c - recover from log
**
** Description:
**	Defines
**		recover_from_log	- recover from log
**		do_local_commit		- commit a pending transaction locally
**		do_local_rollback	- rollback a pending transaction locally
**		check_remote_db		- check remote database
**
** FIXME:
**	Use TM instead of going to SELECT to find the time difference.
**
** History:
**	16-dec-96 (joea)
**		Created based on recovlog.sc and ckremote.sc in replicator
**		library.
**	22-jan-97 (joea)
**		Recreate the shadow table name since it is no longer written to
**		the commit log. Use consistent naming for table_name.
**	21-apr-98 (joea)
**		Temporary changes to allow conversion of remainder of RepServ
**		to OpenAPI.
**	19-may-98 (joea)
**		Convert from ESQL to OpenAPI.
**	08-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	31-mar-2000 (abbjo03) bug 101128
**		Deal with recovery against a URO target.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-dec-2002 (somsa01)
**	    Moved and changed declaration of "entries" to static. The
**	    original declaration caused the stack size of a process started
**	    from the service control manager on Windows to be exceeded.
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw.. functions.
*/

static STATUS do_local_commit(COMMIT_ENTRY *entry);
static STATUS do_local_rollback(COMMIT_ENTRY *entry);
static STATUS check_remote_db(COMMIT_ENTRY *entry);

static COMMIT_ENTRY	entries[MAX_ENTRIES];

/*{
** Name:	recover_from_log - recover from log
**
** Description:
**	Reads two phase commit log file & finds any / all in progress
**	commits & commits/rolls back the local db or calls
**	check_remote_db to see whether the transaction made it to the
**	remote db, & then commits or rolls back appropriately.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or INGRES error
**
** History:
**	11-dec-2002 (somsa01)
**	    Moved and changed declaration of "entries" to static. The
**	    original declaration caused the stack size of a process started
**	    from the service control manager on Windows to be exceeded.
*/
STATUS
recover_from_log()
{
	i4		last;
	STATUS		status;
	i4		i;
	i4		delta_seconds;
	char		stmt[256];
	IIAPI_DATAVALUE	cdata;
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;

	status = get_commit_log_entries(entries, &last);
	if (status)
	{
		/* Log file inconsistent -- skipping */
		messageit(1, 1370);
		return (status);
	}

	for (i = 0; i < last; i++)
	{
		if (entries[i].entry_no != RS_2PC_BEGIN &&
			entries[i].entry_no != RS_NPC_END &&
			entries[i].entry_no != 0)
		{
			STprintf(stmt,
				ERx("SELECT INT4(INTERVAL('seconds', \
DATE('now') - DATE('%s')))"),
				entries[i].timestamp);
			SW_COLDATA_INIT(cdata, delta_seconds);
			status = IIsw_selectSingleton(RSlocal_conn.connHandle,
				&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL,
				1, &cdata, NULL, NULL, &stmtHandle, &getQinfoParm,
				&errParm);
			IIsw_close(stmtHandle, &errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
			if (delta_seconds > 0)
			{
				switch (entries[i].entry_no)
				{
				case RS_2PC_BEGIN:
					status = do_local_rollback(&entries[i]);
					break;

				case RS_PREP_COMMIT:
					status = check_remote_db(&entries[i]);
					break;

				case RS_REMOTE_COMMIT:
					status = do_local_commit(&entries[i]);
					break;

				case RS_NPC_BEGIN:
				case RS_NPC_REM_COMMIT:
				/* no way to recover, use error message no */
					status = 1705;
					break;
				}
				if (status)
				{	/* Error recovering log, case %d */
					messageit(1, 1845, entries[i].entry_no);
					return (status);
				}
			}
		}
	}

	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	return (OK);
}


static STATUS
do_local_commit(
COMMIT_ENTRY	*entry)
{
	STATUS	status;
	IIAPI_GETEINFOPARM	errParm;

	status = RSconn_open_recovery(LOCAL_CONN, (i4)entry->source_db_no,
		entry->dist_trans_id);
	if (status)
	{
		/* Error opening recovery connection */
		messageit(1, 1846);
		return (status);
	}

	status = IIsw_commit(&RSconns[RECOVER_CONN].tranHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
		/* Error doing commit */
		messageit(1, 1847, status);

	RSconn_close(RECOVER_CONN, CONN_CLOSED);

	return (OK);
}


static STATUS
do_local_rollback(
COMMIT_ENTRY	*entry)
{
	STATUS	status;
	IIAPI_GETEINFOPARM	errParm;

	status = RSconn_open_recovery(LOCAL_CONN, (i4)entry->source_db_no,
		entry->dist_trans_id);
	if (status)
	{
		/* Error opening recovery connection */
		messageit(1, 1848);
		return (status);
	}

	status = IIsw_rollback(&RSconns[RECOVER_CONN].tranHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
		/* Error doing rollback */
		messageit(1, 1849, status);

	RSconn_close(RECOVER_CONN, CONN_CLOSED);

	return (OK);
}


/*{
** Name:	check_remote_db - check remote database
**
** Description:
**	In recovering an incomplete commit, this routine commits or
**	rolls back the local transaction depending on whether the data
**	is in the remote db.
**
** Inputs:
**	entry -
**
** Outputs:
**	none
**
** Returns:
**	OK or error from called functions
**
** Side effects:
**	A pending (WILLING_TO_COMMIT) distributed transaction is committed
**	or rolled back.
*/
static STATUS
check_remote_db(
COMMIT_ENTRY	*entry)
{
	i4			target_conn_no;
	char			stmt[256];
	char			shadow_name[DB_MAXNAME+1];
	i4			found;
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	RS_CONN			*conn;
	STATUS			status;

	target_conn_no = RSconn_lookup(entry->target_db_no);
	if (target_conn_no == UNDEFINED_CONN)
	{
		messageit(1, 1850);
		return (1850);
	}

	if (RSconns[target_conn_no].target_type == TARG_UNPROT_READ)
	{
		/*
		** If the remote is unprotected readonly, rollback the willing
		** to commit transaction. At worse, we'll have a benign
		** collision when we retry.
		*/
		status = RSconn_open_recovery(LOCAL_CONN,
			(i4)entry->source_db_no, entry->dist_trans_id);
		status = IIsw_rollback(&RSconns[RECOVER_CONN].tranHandle,
			&errParm);
		return (status);	
	}

	status = RSconn_open_recovery(target_conn_no, 0, 0);
	if (status)
	{
		/* Error opening remote recovery connection */
		messageit(1, 1852);
		return (status);
	}

	if (status = RPtblobj_name(entry->table_name, entry->table_no,
		TBLOBJ_SHA_TBL, shadow_name))
	{
		/* Error generating object name */
		messageit(1, 1816, ERx("CHECK_REMOTE_DB"), entry->table_name);
		return (status);
	}

	conn = &RSconns[RECOVER_CONN];
	STprintf(stmt,
		ERx("SELECT COUNT(*) FROM %s WHERE transaction_id = %d AND \
sequence_no = %d"),
		shadow_name, entry->trans_id, entry->seq_no);
	SW_COLDATA_INIT(cdata, found);
        if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle, stmt, 0,
		NULL, NULL, 1, &cdata, NULL, NULL, &stmtHandle,
		&getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
        {
		/* Error executing query */
		messageit(1, 1854, errParm.ge_errorCode);
		RSconn_close(RECOVER_CONN, CONN_CLOSED);
		return (errParm.ge_errorCode);
        }
        IIsw_close(stmtHandle, &errParm);
        IIsw_commit(&conn->tranHandle, &errParm);	
	RSconn_close(RECOVER_CONN, CONN_CLOSED);

	status = RSconn_open_recovery(LOCAL_CONN, (i4)entry->source_db_no,
		entry->dist_trans_id);
	if (status == OK)
	{
		/* nothing we can do if it can't find the transaction */
		if (found)
			status = IIsw_commit(&conn->tranHandle, &errParm);
		else
			status = IIsw_rollback(&conn->tranHandle, &errParm);
		if (status)
			/* Error executing commit or rollback */
			messageit(1, 1855, status);
		RSconn_close(RECOVER_CONN, CONN_CLOSED);
	}

	return (status);
}

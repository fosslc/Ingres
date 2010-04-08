/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <cv.h>
# include <me.h>
# include <tm.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include <fe.h>
# include <ui.h>
# include <targtype.h>
# include "rsconst.h"
# include "conn.h"
# include "repserv.h"

/**
** Name:	conn.c - Replicator Server connections
**
** Description:
**	Defines
**		RSconns_check_failed	- checks failed target connections
**		RSconns_check_idle	- checks target connections
**		RSconn_close		- close the specified connection
**		RSconns_close		- close all connections
**		RSconns_init		- allocates and loads RSconns array
**		RSconn_lookup		- look up a database in RSconns array
**		RSconn_open		- open the specified connection
**		RSconn_get_name_case	- retrieve DB_NAME_CASE value
**		RSconn_open_recovery	- open the recovery connection
**
** History:
**	16-dec-96 (joea)
**		Created based on openconn.sc in replicator library.
**	14-jan-97 (joea)
**		Use defined constants for target types.
**	24-jan-97 (joea)
**		Remove logic from check_connections to re-open connections with
**		transactions on queue. Make open_conn external.
**	22-jun-97 (joea)
**		Discontinue using the ingrep role.
**	02-oct-97 (joea)
**		Undo the 22-jun change since some customers want to not trigger
**		their own rules if Replicator is doing the updates.
**	29-oct-97 (joea)
**		Correct parameters to message 64.
**	04-nov-97 (joea)
**		When connecting to a gateway, add "/dbms_type" to the vnode/
**		dbname string.
**	05-dec-97 (padni01) bug 87635
**		In check_failed_connections, save the value of RScurr_conn in
**		the beginning and restore session back to it in the end.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL (except for
**		open_recover_conn whose body has been temporarily commented
**		out).
**	04-may-98 (joea)
**		Use IIsw_dbConnect if reconnecting as the DBA. 
**	15-may-98 (joea)
**		Convert 2PC recovery connection to OpenAPI.
**	20-may-98 (joea)
**		Change arguments to RSkeys_check_unique (and rename it).
**	09-jul-98 (abbjo03)
**		Add prefix to USER_NAME to avoid conflict with define in nmcl.h.
**		Add new arguments to IIsw_selectSingleton.
**	14-aug-98 (abbjo03)
**		If there is an error in checking keys, return the correct
**		status. After connecting, copy the real DBA to the owner name.
**	29-oct-98 (abbjo03)
**		Correct size of data values array in RSconns_init.
**	04-nov-98 (abbjo03)
**		Report statistics when closing a connection.
**	02-dec-98 (abbjo03)
**		Uppercase the Replicator role based on the value of name case
**		in the local database.
**	15-jan-99 (abbjo03)
**		Open a dbevents connection in RSconns_init and close it in
**		RSconns_close.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	05-may-99 (abbjo03)
**		Add RSconn_get_name_case and call it in RSconn_open.
**	23-aug-99 (abbjo03) sir 98493
**		Add support for row level locking and repeatable read isolation
**		level.
**	31-aug-99 (abbjo03) sir 98493
**		Need use keep readlock = shared for data integrity.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-nov-2002 (gygro01) b109062/ingrep127
**          In a CONN_FAIL situation disconnect properly in RSconn_close
**          in order to release the connection handler. The connection
**          handler will be set in any case, before the connection will be
**          opened again.
**	03-dec-2003 (gupsh01)
**	    Added support for IIAPI_VERSION_3, initialize connHandle to
**	    Gloval environment RSenv in RSconns_init and RSconn_open_recovery.
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as the infput parameter to II_sw functions.
**      23-Aug-2005 (inifa01)INGREP174/B114410
**          The RepServer takes shared locks on base, shadow and archive tables
**          when Replicating FP -> URO causing deadlocks and lock waits.  
**          - Added flag -NLR and open a separate session running with 
**          readlock=nolock if this flag is set.  RSconns_close() updated to 
**	    close separate session.
**	12-Dec-2006 (kibro01) b117311
**	    When a remote connection requires a change of user for the DBA
**	    in the remote server (e.g. the remote database is not owned by
**	    $ingres) then we disconnect and reconnect.  The reconnection lost
**	    the default settings which are set everywhere else by setting
**	    connHandle to RSenv (see RSconns_init for this being done).
**	    If these settings are lost, the API level defaults to IIAPI_LEVEL_1
**	    which does not support datatypes such as nvarchar.
**	22-Nov-2007 (kibro01) b119245
**	    Add a flag to requiet a server if it was previously quiet
**	    but has been unquieted for a connection retry.
**	
**/

# define REP_ROLE	ERx("ingrep")	/* Replicator role */

# define E_US1267_4711_DIS_TRAN_UNKNOWN		4711


GLOBALDEF RS_CONN	*RSconns = NULL;	/* connections array */
GLOBALDEF i4	RSnum_conns = 0;		/* size of array */
GLOBALDEF i4	RScurr_conn = UNDEFINED_CONN;
GLOBALDEF i4	RSconn_timeout = CONN_NOTIMEOUT;
GLOBALDEF i4	RSopen_retry = CONN_NORETRY;
GLOBALDEF bool	RSquiet = FALSE;
GLOBALDEF bool	RSrequiet = FALSE;
GLOBALDEF i2	RSserver_no = -1;
GLOBALDEF bool	RSsingle_pass = FALSE;
GLOBALDEF bool	RSskip_check_rules = FALSE;
GLOBALDEF i4	RStime_out = QRY_TIMEOUT_DFLT;

GLOBALREF II_PTR	RSenv; /* environment */


static i4	mid = UNDEFINED_CONN;
static i2	last_dbno = -1;		/* last database no */


/*{
** Name:	RSconns_check_idle - checks target connections
**
** Description:
**	Checks the target connections closing those that have timed out.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSconns_check_idle()
{
	i4	i;

	/* Close expired connections. */
	for (i = FIXED_CONNS;
		RSconn_timeout != CONN_NOTIMEOUT && i < RSnum_conns; ++i)
	{
		if (RSconns[i].status == CONN_OPEN &&
			TMsecs() - RSconns[i].last_needed >= RSconn_timeout)
		{
			messageit(3, 1791, RSconns[i].vnode_name,
				RSconns[i].db_name);
			RSconn_close(i, CONN_CLOSED);
		}
	}
}


/*{
** Name:	RSconns_check_failed - checks failed target connections
**
** Description:
**	Checks the failed target connections, opening ones that qualify based
**	on open_retry count.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSconns_check_failed()
{
	i4	i;

	/* Open failed connections, if appropriate. */
	for (i = FIXED_CONNS; i < RSnum_conns; ++i)
	{
		if (RSconns[i].status == CONN_FAIL)
		{
			/* If the retry limit has been reached, try to open. */
			if (++RSconns[i].open_retry >= RSopen_retry)
			{
				if (RSconn_open(i))
					RSconn_close(i, CONN_FAIL);
			}
		}
	}
}


/*{
** Name:	RSconn_close - close the specified connection
**
** Description:
**	Closes the specified connection.
**
** Inputs:
**	conn_no	- the connection number
**	status	- status of the connection, usually CONN_CLOSED or CONN_FAIL
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSconn_close(
i4	conn_no,
i4	status)
{
	IIAPI_GETEINFOPARM	errParm;
	RS_CONN		*conn;

	if (!RSnum_conns)
	{
		messageit(1, 1793, conn_no);
		return;
	}

	conn = &RSconns[conn_no];
	/* if the connection is believed to be open, attempt to rollback */
	if (conn->status == CONN_OPEN)
	{
		IIsw_rollback(&conn->tranHandle, &errParm);
		IIsw_disconnect(&conn->connHandle, &errParm);
	}

	/* If the closed status is CONN_FAIL, quiet the target */
	if (status == CONN_FAIL && conn_no >= FIXED_CONNS)
		quiet_db(conn->db_no, SERVER_QUIET);

	/* b109062 release connection handler */
	IIsw_disconnect(&conn->connHandle, &errParm);

	/* Reset to use defaults, such as the API level (b117311) kibro01 */
	if (conn->connHandle == NULL)
		conn->connHandle = RSenv;

	if (conn_no >= FIXED_CONNS)
	{
		i4	num_txns;
		i4	conn_mins;

		RSstats_report(conn->db_no, &num_txns, &conn_mins);
		messageit(2, 106, conn->db_no, conn->vnode_name, conn->db_name,
			num_txns, conn_mins / 60, conn_mins % 60);
	}
	conn->status = status;
	conn->open_retry = 0;
}


/*{
** Name:	RSconns_close - close connections
**
** Description:
**	Rolls back transactions in all connected databases, disconnects all,
**	and exits.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSconns_close()
{
	i4			i;
	IIAPI_GETEINFOPARM	errParm;

	for (i = FIXED_CONNS; i < RSnum_conns; ++i)
	{
		if (RSconns[i].status == CONN_OPEN)
			RSconn_close(i, CONN_CLOSED);
	}

	if (RSconns && RSconns[NOTIFY_CONN].status == CONN_OPEN)
	{
		IIsw_disconnect(&RSconns[NOTIFY_CONN].connHandle, &errParm);
		RSconns[NOTIFY_CONN].status = CONN_CLOSED;
	}

	if (RSlocal_conn.status == CONN_OPEN)
		IIsw_disconnect(&RSlocal_conn.connHandle, &errParm);
	if (RSlocal_conn_nlr.status == CONN_OPEN)
		IIsw_disconnect(&RSlocal_conn_nlr.connHandle, &errParm);
	RSlocal_conn.status = CONN_CLOSED;
	RSlocal_conn_nlr.status = CONN_CLOSED;
	RSstats_terminate();
}


/*{
** Name:	RSconns_init - allocates and loads RSconns array
**
** Description:
**	Allocates the RSconns array and initializes the array with the
**	target database information.
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
RSconns_init()
{
	i2			gateway;
	i2			min_type;
	i4			i;
	RS_CONN			conn;
	char			*func = ERx("CONNECTIONS_INIT");
	char			db_name[DB_MAXNAME*2+3];
	char			stmt[512];
	IIAPI_DESCRIPTOR        pdesc;
	IIAPI_DATAVALUE         pdata;
        IIAPI_DATAVALUE         cdata[6];
	II_PTR			stmtHandle;
        IIAPI_GETEINFOPARM      errParm;
        IIAPI_GETQINFOPARM      getQinfoParm;
	IIAPI_STATUS		status;

	if (RSnum_conns)
		return (FAIL);

        SW_COLDATA_INIT(cdata[0], RSnum_conns);
	STprintf(stmt, ERx("SELECT COUNT(DISTINCT targetdb) FROM dd_db_cdds c, \
dd_paths p, dd_databases d WHERE c.cdds_no = p.cdds_no AND c.database_no = \
p.targetdb AND c.server_no = %d AND c.database_no = d.database_no AND \
p.localdb = %d AND LOWERCASE(d.vnode_name) != 'mobile'"),
		(i4)RSserver_no, (i4)RSlocal_conn.db_no);
        if (IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL, 1, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
        {
		messageit(1, 1794, func, errParm.ge_errorCode,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	RSnum_conns += FIXED_CONNS;
	RSconns = (RS_CONN *)MEreqmem(0, (u_i4)RSnum_conns * sizeof(RS_CONN),
		TRUE, (STATUS *)NULL);
	if (RSconns == NULL)
	{
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		messageit(1, 1795, func);
		return (FAIL);
	}

	SW_COLDATA_INIT(cdata[0], conn.db_no);
	SW_COLDATA_INIT(cdata[1], conn.vnode_name);
	SW_COLDATA_INIT(cdata[2], conn.db_name);
	SW_COLDATA_INIT(cdata[3], conn.owner_name);
	SW_COLDATA_INIT(cdata[4], conn.dbms_type);
	SW_COLDATA_INIT(cdata[5], gateway);
	stmtHandle = NULL;
	STprintf(stmt, ERx("SELECT DISTINCT c.database_no, vnode_name, \
database_name, database_owner, d.dbms_type, gateway FROM dd_paths p, \
dd_db_cdds c, dd_databases d, dd_dbms_types t WHERE c.cdds_no = p.cdds_no AND \
c.database_no = p.targetdb AND c.server_no = %d AND c.database_no = \
d.database_no AND p.localdb = %d AND d.dbms_type = t.dbms_type AND \
LOWERCASE(d.vnode_name) != 'mobile' ORDER BY database_no"),
		(i4)RSserver_no, (i4)RSlocal_conn.db_no);
	for (i = FIXED_CONNS; i <= RSnum_conns; ++i)
	{
		status = IIsw_selectLoop(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL, 6, cdata,
			&stmtHandle, &getQinfoParm, &errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(conn.vnode_name, cdata[1]);
		SW_CHA_TERM(conn.db_name, cdata[2]);
		SW_CHA_TERM(conn.owner_name, cdata[3]);
		SW_CHA_TERM(conn.dbms_type, cdata[4]);
		RSconns[i] = conn;	/* struct */
		RSconns[i].status = CONN_CLOSED;
		RSconns[i].gateway = (bool)gateway;
		RSconns[i].open_retry = RSconns[i].last_needed = 0;
		RSconns[i].tables_valid = FALSE;
		RSconns[i].target_type = TARG_UNPROT_READ;	/* default */
		RSconns[i].connHandle = RSenv; 
		RSconns[i].tranHandle = NULL;
	}
	if (status == IIAPI_ST_NO_DATA && i != RSnum_conns && status !=
		IIAPI_ST_SUCCESS)
	{
		/* Error selecting connection information */
		messageit(1, 1796, func, errParm.ge_errorCode,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	/* get RSconns[].target_type */
	SW_QRYDESC_INIT(pdesc, IIAPI_INT_TYPE, 2, FALSE);
	SW_COLDATA_INIT(cdata[0], min_type);
	for (i = FIXED_CONNS; i < RSnum_conns; i++)
	{
		SW_PARMDATA_INIT(pdata, &RSconns[i].db_no, 2, FALSE);

		status = IIsw_selectSingleton(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("SELECT MIN(target_type) FROM dd_db_cdds WHERE \
database_no = ~V"),
			1, &pdesc, &pdata, 1, cdata, NULL, NULL, &stmtHandle,
			 &getQinfoParm, &errParm);
		if (status != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1796, func, errParm.ge_errorCode,
				errParm.ge_message);
			IIsw_close(stmtHandle, &errParm);
		}
		else
		{
			IIsw_close(stmtHandle, &errParm);
			RSconns[i].target_type = min_type;
		}
	}

	if (IIsw_commit(&RSlocal_conn.tranHandle, &errParm) != IIAPI_ST_SUCCESS)
	{
		/* Error committing */
		messageit(1, 1798, func, errParm.ge_errorCode,
			errParm.ge_message);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}

	/* open a new local session for receiving dbevents */
	RSconns[NOTIFY_CONN].connHandle = RSenv;
	STprintf(db_name, ERx("%s::%s"), RSlocal_conn.vnode_name,
		RSlocal_conn.db_name);
	if (IIsw_dbConnect(db_name, RSlocal_conn.owner_name, NULL,
		&RSconns[NOTIFY_CONN].connHandle, NULL, &errParm) !=
		IIAPI_ST_SUCCESS)
	{
		/* DBMS error opening database */
		messageit(1, 3, errParm.ge_errorCode, db_name,
			errParm.ge_message);
		return (FAIL);
	}
	RSconns[NOTIFY_CONN].status = CONN_OPEN;

	return (OK);
}


/*{
** Name:	RSconn_lookup - look up a database in the RSconns array
**
** Description:
**	Looks up a database number in the Conenctions array and returns the
**	index.  A binary search is used.
**
** Inputs:
**	db_no	- the database number
**
** Outputs:
**	none
**
** Returns:
**	UNDEFINED_CONN - not found
**	otherwise - the index of the found database in the RSconns array
*/
i4
RSconn_lookup(
i2	db_no)
{
	i4	low, high, i;			/* binary search indices */

	if (!RSnum_conns)
	{
		messageit(1, 1799, db_no);
		return (UNDEFINED_CONN);
	}

	/* check cache */
	if (db_no == last_dbno)
		return (mid);
	last_dbno = db_no;

	/* initialize the binary search indices */
	low = FIXED_CONNS;
	high = RSnum_conns - 1;

	/* perform the look-up */
	while (low <= high)
	{
		mid = (low + high) / 2;
		i = db_no - RSconns[mid].db_no;

		if (i < 0)
			high = mid - 1;		/* too big */
		else if (i > 0)
			low = mid + 1;		/* too small */
		else
			return (mid);		/* found */
	}

	mid = UNDEFINED_CONN;

	return (mid);
}

/*{
** Name:	RSconn_open - open the specified connection
**
** Description:
**	Opens the specified connection.
**
** Inputs:
**	conn_no	- the connection number
**
** Outputs:
**	none
**
** Returns:
**	0 - OK
**	otherwise error
*/
STATUS
RSconn_open(
i4	conn_no)
{
	char			nodedb[DB_MAXNAME*2+12];
	RS_USER_NAME		dba;
	RS_USER_NAME		user;
	char			stmt[512];
	RS_CONN			*conn;
	IIAPI_DATAVALUE		cdata[2];
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;

	if (!RSnum_conns)
	{
		messageit(1, 1800, conn_no);
		return (FAIL);
	}

	conn = &RSconns[conn_no];
	if (conn->status != CONN_CLOSED)
		RSconn_close(conn_no, CONN_CLOSED);

	if (!conn->gateway)
	{
		char	rep_role[7];

		STcopy(REP_ROLE, rep_role);
		if (RSlocal_conn.name_case == UI_UPPERCASE)
			CVupper(rep_role);
		STprintf(nodedb, ERx("%s::%s"), conn->vnode_name,
			conn->db_name);
		status = IIsw_setConnParam(&conn->connHandle, IIAPI_CP_APP_ID,
			rep_role, &errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
	}
	else
	{
		STprintf(nodedb, ERx("%s::%s/%s"), conn->vnode_name,
			conn->db_name, conn->dbms_type);
	}
	if (IIsw_connect(nodedb, NULL, NULL, &conn->connHandle, NULL, &errParm)
		!= IIAPI_ST_SUCCESS)
	{
		messageit(1, 1801, errParm.ge_errorCode, nodedb, conn_no,
			errParm.ge_message);
		return (errParm.ge_errorCode);
	}

	/* See if user is the DBA; if not, reconnect as DBA */
	conn->tranHandle = NULL;
	SW_COLDATA_INIT(cdata[0], dba);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
		ERx("SELECT DBMSINFO('DBA')"), 0, NULL, NULL, 1, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d selecting DBA name */
		messageit(1, 96, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	SW_COLDATA_INIT(cdata[0], user);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
		ERx("SELECT DBMSINFO('USERNAME')"), 0, NULL, NULL, 1, cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d selecting DBA name */
		messageit(1, 97, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	if (IIsw_commit(&conn->tranHandle, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1798, ERx("RSconn_open"), errParm.ge_errorCode,
			errParm.ge_message);
		return (FAIL);
	}

	SW_VCH_TERM(&dba);
	SW_VCH_TERM(&user);
	if (STcompare(dba.name, user.name))
	{
		IIsw_disconnect(&conn->connHandle, &errParm);

		/* Reset to use defaults, e.g. API level (b117311) kibro01 */
		if (conn->connHandle == NULL)
			conn->connHandle = RSenv;

		if (!conn->gateway)
		{
			char	rep_role[7];

			STcopy(REP_ROLE, rep_role);
			if (RSlocal_conn.name_case == UI_UPPERCASE)
				CVupper(rep_role);
			status = IIsw_setConnParam(&conn->connHandle,
				IIAPI_CP_APP_ID, rep_role, &errParm);
		}
		if (IIsw_dbConnect(nodedb, dba.name, NULL, &conn->connHandle,
			NULL, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1803, errParm.ge_errorCode, nodedb,
				dba.name, conn_no, errParm.ge_message);
			return (errParm.ge_errorCode);
		}
	}

	/* adjust the target DBA to the real DBA */
	STcopy(dba.name, conn->owner_name);

	if (IIsw_query(conn->connHandle, &conn->tranHandle,
		ERx("SET SESSION ISOLATION LEVEL REPEATABLE READ"), 0, NULL,
		NULL, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1807, errParm.ge_errorCode, nodedb, dba, conn_no,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (errParm.ge_errorCode);
	}
	IIsw_close(stmtHandle, &errParm);

	STprintf(stmt, ERx("SET LOCKMODE SESSION WHERE READLOCK = SHARED, \
LEVEL = ROW, TIMEOUT = %d"),
		RStime_out);
	if (IIsw_query(conn->connHandle, &conn->tranHandle, stmt, 0, NULL, NULL,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1807, errParm.ge_errorCode, nodedb, dba, conn_no,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (errParm.ge_errorCode);
	}
	IIsw_close(stmtHandle, &errParm);

	if (!conn->gateway)
	{
		if (IIsw_query(conn->connHandle, &conn->tranHandle,
			ERx("SET TRACE POINT DM32"), 0, NULL, NULL, NULL, NULL,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			messageit(1, 112, errParm.ge_errorCode, nodedb,
				errParm.ge_message);
			IIsw_close(stmtHandle, &errParm);
			return (errParm.ge_errorCode);
		}
		IIsw_close(stmtHandle, &errParm);
	}
	if (RSconn_get_name_case(conn) != OK)
	{
		messageit(1, 113);
		return (FAIL);
	}
	if (IIsw_commit(&conn->tranHandle, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1798, ERx("RSconn_open"), errParm.ge_errorCode,
			errParm.ge_message);
		return (FAIL);
	}

	status = OK;
	if (!RSskip_check_rules &&
		RSconns[conn_no].target_type < TARG_UNPROT_READ &&
		RSconns[conn_no].tables_valid == FALSE)
	{
		if (RSkeys_check_unique(&RSconns[conn_no]) != OK)
		{
			quiet_db(RSconns[conn_no].db_no, SERVER_QUIET);
			status = FAIL;
		}
		else
		{
			RSconns[conn_no].tables_valid = TRUE;
		}
	}

	if (status == OK)
	{
		/* unquiet this connection for this server */
		unquiet_db(RSconns[conn_no].db_no, SERVER_QUIET);
		RSconns[conn_no].status = CONN_OPEN;
		/* Database %s is open */
		messageit(4, 1073, nodedb);
	}

	return (status);
}


/*{
** Name:	RSconn_get_name_case - retrieve DB_NAME_CASE value
**
** Description:
**	Retrieve value of DB_NAME_CASE from iidbcapabilities.
**
** Inputs:
**	conn	- connection block
**
** Outputs:
**	conn->name_case	- name case for connection
**
** Returns:
**	OK or FAIL
*/
STATUS
RSconn_get_name_case(
RS_CONN	*conn)
{
	char			name_case[DB_MAXNAME+1];
	IIAPI_DATAVALUE		cdata;
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	SW_COLDATA_INIT(cdata, name_case);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
		ERx("SELECT cap_value FROM iidbcapabilities WHERE \
cap_capability = 'DB_NAME_CASE'"),
		0, NULL, NULL, 1, &cdata, NULL, NULL, &stmtHandle, 
		&getQinfoParm, &errParm)
		!= IIAPI_ST_SUCCESS)
	{
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	if (CMcmpcase(name_case, ERx("U")) == 0)
		conn->name_case = UI_UPPERCASE;
	else if (CMcmpcase(name_case, ERx("L")) == 0)
		conn->name_case = UI_LOWERCASE;
	else if (CMcmpcase(name_case, ERx("M")) == 0)
		conn->name_case = UI_MIXEDCASE;
	else
		conn->name_case = UI_UNDEFCASE;
	return (OK);
}


/*{
** Name:	RSconn_open_recovery - open the recovery connection
**
** Description:
**	Opens the recovery connection.
**
** Inputs:
**	conn_no		- connection number
**	highid		- the high transaction ID, or NULL
**	lowid		- the low transaction ID, or NULL
**
** Outputs:
**	none
**
** Returns:
**	0 - OK
**	otherwise error
*/
STATUS
RSconn_open_recovery(
i4	conn_no,
i4	highid,
i4	lowid)
{
	char			nodedb[DB_MAXNAME*2+3];
	RS_CONN			*conn = &RSconns[RECOVER_CONN];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;

	if (!RSnum_conns)
	{
		messageit(1, 1809);
		return (FAIL);
	}

	if (conn_no == LOCAL_CONN)
		*conn = RSlocal_conn;		/* struct */
	else
		*conn = RSconns[conn_no];	/* struct */
	conn->status = CONN_CLOSED;
	conn->connHandle = RSenv;
	conn->tranHandle = NULL;
	STprintf(nodedb, ERx("%s::%s"), conn->vnode_name, conn->db_name);

	if (highid && lowid)
	{
		status = IIsw_registerXID(highid, lowid, ERx(""),
			&conn->tranHandle);
		if (status != IIAPI_ST_SUCCESS)
		{
			messageit(1, 1810, status, nodedb, RECOVER_CONN,
				ERx(""));
			return (status);
		}
	}

	if (IIsw_dbConnect(nodedb, conn->owner_name, NULL, &conn->connHandle,
		&conn->tranHandle, &errParm) != IIAPI_ST_SUCCESS)
	{
		if (errParm.ge_errorCode == E_US1267_4711_DIS_TRAN_UNKNOWN)
		{
			conn->connHandle = RSenv;
			conn->tranHandle = NULL;
			if (IIsw_dbConnect(nodedb, conn->owner_name, NULL,
				&conn->connHandle, &conn->tranHandle,
				&errParm) != IIAPI_ST_SUCCESS)
			{
				messageit(1, 1810, errParm.ge_errorCode, nodedb,
					RECOVER_CONN, errParm.ge_message);
				return (errParm.ge_errorCode);
			}
		}
		else
		{
			messageit(1, 1810, errParm.ge_errorCode, nodedb,
				RECOVER_CONN, errParm.ge_message);
			return (errParm.ge_errorCode);
		}
	}

	conn->status = CONN_OPEN;
	/* Database %s is open */
	messageit(4, 1073, nodedb);

	return (OK);
}

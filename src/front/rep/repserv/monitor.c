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
# include "conn.h"
# include "repevent.h"

/**
** Name:	monitor.c - monitor server status and activity
**
** Description:
**	Defines
**		RSmonitor_notify	- raise monitor dbevents
**		RSping			- report server status
**
** History:
**	16-dec-96 (joea)
**		Created based on dmonitor.sc in replicator library.
**	14-jan-97 (joea)
**		Use defined constants for target types.
**	27-jan-97 (joea)
**		Replace parts of RLIST by REP_TARGET.
**	11-dec-97 (joea)
**		Eliminate REP_TARGET. d_monitor will only be called if target
**		type is not URO.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	21-oct-98 (abbjo03)
**		Change RSmonitor (-MON flag) to default to TRUE.
**	21-jan-99 (abbjo03)
**		Use notification connection to raise dd_server event.
**	02-feb-99 (abbjo03)
**		Undo the 21-jan change.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALDEF bool	RSmonitor = TRUE;


static i4	ping_cnt = 0;

static char *mon_evts[] =
{
	ERx("dd_insert"),
	ERx("dd_update"),
	ERx("dd_delete"),
	ERx("dd_transaction"),
	ERx("dd_insert2"),
	ERx("dd_update2"),
	ERx("dd_delete2"),
	ERx("dd_transaction2"),
};


/*{
** Name:	RSmonitor_notify - raise monitor dbevents
**
** Description:
**	Raise dbevents to alert the monitor process.
**
** Inputs:
**	conn		- connection to use to raise dbevent
**	mon_type	- type of monitoring event
**	db_no		- database number
**	table_name	- name of table being monitored, or "Transactions"
**	table_owner	- table owner, or NULL
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSmonitor_notify(
RS_CONN	*conn,
i4	mon_type,
i2	db_no,
char	*table_name,
char	*table_owner)
{
	char			node_dbname[DB_MAXNAME*2+3];
	char			table_ownname[DB_MAXNAME*2+2];
	char			evtText[256];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;

	if (!RSmonitor)
		return;

	if (table_owner != NULL && *table_owner != EOS)
		STprintf(table_ownname, ERx("''%s.%s''"), table_owner,
			table_name);
	else
		STcopy(table_name, table_ownname);

	if (db_no != RSlocal_conn.db_no)
	{
		i4	conn_no = RSconn_lookup(db_no);

		if (conn_no == UNDEFINED_CONN)
		{
			messageit(1, 1789, (i4)db_no);
			STprintf(node_dbname, ERx("%d %s::%s"), (i4)db_no,
				ERx("undefined"), ERx("undefined"));
			STcopy(ERx("Unexpected database"), table_ownname);
		}
		else
		{
			STprintf(node_dbname, ERx("%d %s::%s"), (i4)db_no,
				RSconns[conn_no].vnode_name,
				RSconns[conn_no].db_name);
		}
	}
	else
	{
		STprintf(node_dbname, ERx("%d %s::%s"), (i4)RSlocal_conn.db_no,
			RSlocal_conn.vnode_name, RSlocal_conn.db_name);
	}

	STprintf(evtText, ERx("%-35s %s"), node_dbname, table_ownname);

	/* Alerting Monitor Type %d on %s */
	messageit(5, 1281, mon_type, evtText);

	if (mon_type < 1 || mon_type > 8)
	{
		/* Unknown Monitor Type %d */
		messageit(3, 1282, mon_type);
		return;
	}

	if (IIsw_raiseEvent(conn->connHandle, &conn->tranHandle,
		mon_evts[mon_type - 1], evtText, &stmtHandle, &errParm) !=
			IIAPI_ST_SUCCESS)
		messageit(1, 1212, ERx("D_MONITOR"), errParm.ge_errorCode,
			errParm.ge_message);
	IIsw_close(stmtHandle, &errParm);
}


/*{
** Name:	RSping - report server status
**
** Description:
**	Raises a database event reporting the status of this server.
**
** Inputs:
**	override - indicates whether to override the NMO/RSmonitor flag
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSping(
bool	override)
{
	char			evtName[512];
	char			evtText[80];
	RS_CONN			*conn = &RSlocal_conn;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;

	if (!RSmonitor && !override)
		return;

	++ping_cnt;

	/* The Server is Active/Quiet, Error Count %d, Ping %d */
	if (RSquiet)
	{
		messageit(5, 66, RSerr_cnt, ping_cnt);
		messageit_text(evtText, 68, RSerr_cnt, ping_cnt, conn->db_no,
			conn->vnode_name, conn->db_name);
	}
	else
	{
		messageit(5, 67, RSerr_cnt, ping_cnt);
		messageit_text(evtText, 69, RSerr_cnt, ping_cnt, conn->db_no,
			conn->vnode_name, conn->db_name);
	}

	STprintf(evtName, ERx("dd_server%d"), (i4)RSserver_no);
	if (IIsw_raiseEvent(conn->connHandle, &conn->tranHandle, evtName,
			evtText, &stmtHandle, &errParm) != IIAPI_ST_SUCCESS)
		/* DBMS error %d raising event */
		messageit(1, 1212, ERx("PING"), errParm.ge_errorCode,
			errParm.ge_message);
	IIsw_close(stmtHandle, &errParm);
}

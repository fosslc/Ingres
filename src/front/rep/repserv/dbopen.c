/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <pc.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "rsconst.h"
# include "conn.h"

/**
** Name:	dbopen.c - open local database
**
** Description:
**	Defines
**		RSlocal_db_open	- open local database
**		RSlocal_db_open_nlr - open local database with nolock reads
**		RSshutdown	- close databases & shutdown the server
**
** History:
**	16-dec-96 (joea)
**		Created based on opendb.sc in replicator library.
**	25-feb-97 (joea)
**		In RSshutdown, set RSdb_open to FALSE after closing connections,
**		otherwise messageit tries to switch to the closed sessions and
**		we never shut cleanly. Also, record the session switch in the
**		RScurr_conn global and replace two remaining uses of
**		error_check() by RPdb_error_check().
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	04-may-98 (joea)
**		Use IIsw_dbConnect to connect to the database.
**	11-may-98 (joea)
**		Add tranHandle parameter to IIsw_connect.
**	20-may-1998 (joea)
**		Move all_unique_keys and unquiet_all to RepServer's main().
**	09-jul-98 (abbjo03)
**		Add prefix to USER_NAME to avoid conflict with define in nmcl.h.
**		Add new arguments to IIsw_selectSingleton.
**	28-oct-98 (abbjo03)
**		Correct message number in RSshutdown.
**	02-dec-98 (abbjo03)
**		Retrieve case of regular DBMS object names.
**	15-jan-99 (abbjo03)
**		Call IIsw_getErrorInfo after error in raising dbevent in
**		RSshutdown. Use dbevent connection to raise the dbevent.
**	02-feb-99 (abbjo03)
**		Partially undo change of 15-jan (using dbevent connection to
**		raise dbevent).
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	22-apr-99 (abbjo03)
**		Add envHandle parameter to IIsw_terminate.
**	05-may-99 (abbjo03)
**		Call RSconn_get_name_case instead of retrieving DB_NAME_CASE.
**	23-aug-99 (abbjo03) sir 98493
**		Add support for row level locking and repeatable read isolation
**		level.
**	31-aug-99 (abbjo03) sir 98493
**		Need to keep readlock = shared for data integrity.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-dec-2003 (gupsh01)
**	    Added API version 3 support, now pass the global environment variable, 
**	    RSenv to RSlocal_db_open and RSshutdown.
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw functions.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**      23-Aug-2005 (inifa01)INGREP174/B114410
**          The RepServer takes shared locks on base, shadow and archive tables
**          when Replicating FP -> URO.  causing deadlock and lock waits.
**          - The resolution to this problem adds flag -NLR and opens a separate
**          session running with readlock=nolock if this flag is set.
**	    Added RSlocal_db_open_nlr() for opening separate session.
**	15-Mar-2006 (hanje04)
**	    Add IIAPI_GETQINFOPAR parameters to IIsw calls after X-integ of
**	    fix for bug 114410. Also define where apropriate.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

GLOBALDEF RS_CONN	RSlocal_conn ZERO_FILL;
GLOBALDEF RS_CONN	RSlocal_conn_nlr ZERO_FILL;
GLOBALREF II_PTR	RSenv;


static bool	shutting_down = FALSE;


/*{
** Name:	RSlocal_db_open - open local database
**
** Description:
**	Opens connection to the local database.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	FAIL	- an error occurred in connecting to the local database
**	OK	- connected successfully
*/
STATUS
RSlocal_db_open()
{
        char                    database_name[DB_DB_MAXNAME+DB_NODE_MAXNAME+3];
					/* needs to hold vnode:dbname */
	char			dba[DB_OWN_MAXNAME+1];
	char			user[DB_OWN_MAXNAME+1];
	char			stmt[1024 + DB_MAXNAME];
	PID			pid;
	RS_CONN			*conn = &RSlocal_conn;
	IIAPI_DATAVALUE		cdata[2];
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	if (*conn->vnode_name != EOS)
		STprintf(database_name, ERx("%s::%s"), conn->vnode_name,
			conn->db_name);
	else
		STcopy(conn->db_name, database_name);

	if (conn->connHandle == NULL)
	  conn->connHandle = RSenv;

	if (IIsw_dbConnect(database_name, *conn->owner_name == EOS ? NULL :
		conn->owner_name, NULL, &conn->connHandle, NULL, &errParm) !=
		IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d opening database '%s' */
		messageit(1, 3, errParm.ge_errorCode, database_name,
			errParm.ge_message);
		return (FAIL);
	}

	conn->tranHandle = NULL;
	if (IIsw_query(conn->connHandle, &conn->tranHandle,
		ERx("SET SESSION ISOLATION LEVEL REPEATABLE READ"), 0, NULL,
		NULL, NULL, NULL, &stmtHandle, &getQinfoParm,
		&errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 3, errParm.ge_errorCode, database_name,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	/*
	** The local session must be running with read locks to maintain data
	** integrity.
	*/
	STprintf(stmt, ERx("SET LOCKMODE SESSION WHERE READLOCK = SHARED, \
LEVEL = ROW, TIMEOUT = %d"),
		RStime_out);
	if (IIsw_query(conn->connHandle, &conn->tranHandle, stmt, 0, NULL, NULL,
		NULL, NULL, &stmtHandle, &getQinfoParm, 
		&errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 3, errParm.ge_errorCode, database_name,
			errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	/* confirm that we are connected as the DBA */
	SW_COLDATA_INIT(cdata[0], dba);
	SW_COLDATA_INIT(cdata[1], user);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
		ERx("SELECT CHAR(dba_name), CHAR(user_name) FROM \
iidbconstants"),
		0, NULL, NULL, 2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
		&errParm) != IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d selecting DBA name */
		messageit(1, 96, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	SW_CHA_TERM(dba, cdata[0]);
	SW_CHA_TERM(user, cdata[1]);
	if (STcompare(dba, user))
	{
		/*
		** User '%s' is not the owner of database '%s'.  Use the -OWN
		** startup flag to specify user '%s' as the correct owner.
		*/
		messageit(1, 98, user, database_name, dba);
		return (FAIL);
	}
	STcopy(dba, conn->owner_name);

	if (RSconn_get_name_case(conn) != OK)
	{
		messageit(1, 113);
		IIsw_rollback(&conn->tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_commit(&conn->tranHandle, &errParm);

	/* Selecting local database number */
	messageit(5, 1069);
	/* FIXME:  this overwrites the node info provided with the -IDB flag */
	STprintf(stmt, ERx("SELECT database_no, vnode_name FROM dd_databases \
WHERE database_name = '%s' AND local_db = 1"),
		conn->db_name);
	SW_COLDATA_INIT(cdata[0], conn->db_no);
	SW_COLDATA_INIT(cdata[1], conn->vnode_name);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle, stmt, 0,
		NULL, NULL, 2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
		&errParm) != IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d selecting local database name */
		messageit(1, 1074, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&conn->tranHandle, &errParm);
		return (FAIL);
	}
	if (!IIsw_getRowCount(&getQinfoParm))
	{
		/* Local database information not found */
		messageit(1, 1077);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&conn->tranHandle, &errParm);
	STtrmwhite(conn->vnode_name);

	/* Local database '%s' is open */
	conn->status = CONN_OPEN;
	messageit(4, 1064, database_name);

	/* update this server instance in the catalogs */
	PCpid(&pid);
	STprintf(stmt,
		ERx("UPDATE dd_servers SET pid = '%d' WHERE server_no = %d"),
		(i4)pid, (i4)RSserver_no);
	if (IIsw_query(conn->connHandle, &conn->tranHandle, stmt, 0, NULL, NULL,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS ||
		IIsw_getRowCount(&getQinfoParm) != 1)
	{
		/* Error updating pid in dd_servers table */
		messageit(1, 1640);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&conn->tranHandle, &errParm);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&conn->tranHandle, &errParm);

	if (RSnolock_read)
	{
		STlcopy(RSlocal_conn.db_name,RSlocal_conn_nlr.db_name,DB_DB_MAXNAME);
		STlcopy(RSlocal_conn.vnode_name,RSlocal_conn_nlr.vnode_name,DB_MAXNAME);
		STlcopy(RSlocal_conn.owner_name,RSlocal_conn_nlr.owner_name,DB_OWN_MAXNAME);

	    	if (RSlocal_db_open_nlr() != OK)
		    return(FAIL);
	}

	return (OK);
}

/*{
** Name:        RSlocal_db_open_nlr - open local database with nolock reads 
**
** Description:
**      Opens connection to the local database for reading the base table
**	shadow table and archive table with readlock=nolock if -NLR flag
**	is set.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Returns:
**      FAIL    - an error occurred in connecting to the local database
**      OK      - connected successfully
*/
STATUS
RSlocal_db_open_nlr()
{
        char                    database_name[DB_DB_MAXNAME+DB_NODE_MAXNAME+3];
					/* needs to hold vnode:dbname */
        char                    dba[DB_OWN_MAXNAME+1];
        char                    user[DB_OWN_MAXNAME+1];
        char                    stmt[1024 + DB_MAXNAME];
        PID                     pid;
        RS_CONN                 *conn = &RSlocal_conn_nlr;
        IIAPI_DATAVALUE         cdata[2];
        IIAPI_GETEINFOPARM      errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
        II_PTR                  stmtHandle;

        if (*conn->vnode_name != EOS)
                STprintf(database_name, ERx("%s::%s"), conn->vnode_name,
                        conn->db_name);
        else
                STcopy(conn->db_name, database_name);

        if (conn->connHandle == NULL)
          conn->connHandle = RSenv;

        if (IIsw_dbConnect(database_name, *conn->owner_name == EOS ? NULL :
                conn->owner_name, NULL, &conn->connHandle, NULL, &errParm) !=
                IIAPI_ST_SUCCESS)
        {
                /* DBMS error %d opening database '%s' */
                messageit(1, 3, errParm.ge_errorCode, database_name,
                        errParm.ge_message);
                return (FAIL);
        }

        conn->tranHandle = NULL;
        if (IIsw_query(conn->connHandle, &conn->tranHandle,
                ERx("SET SESSION ISOLATION LEVEL REPEATABLE READ"), 0, NULL,
                NULL, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
        {
                messageit(1, 3, errParm.ge_errorCode, database_name,
                        errParm.ge_message);
                IIsw_close(stmtHandle, &errParm);
                return (FAIL);
        }
        IIsw_close(stmtHandle, &errParm);

        /*
        ** The local session must be running with read locks to maintain data
        ** integrity.
        */
        STprintf(stmt, ERx("SET LOCKMODE SESSION WHERE READLOCK = NOLOCK, \
LEVEL = ROW, TIMEOUT = %d"),
                RStime_out);
        if (IIsw_query(conn->connHandle, &conn->tranHandle, stmt, 0, NULL, NULL,
                NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
        {
                messageit(1, 3, errParm.ge_errorCode, database_name,
                        errParm.ge_message);
                IIsw_close(stmtHandle, &errParm);
                return (FAIL);
        }
        IIsw_close(stmtHandle, &errParm);

        /* confirm that we are connected as the DBA */
        SW_COLDATA_INIT(cdata[0], dba);
        SW_COLDATA_INIT(cdata[1], user);
        if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
                ERx("SELECT CHAR(dba_name), CHAR(user_name) FROM \
iidbconstants"),
                0, NULL, NULL, 2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) !=
                IIAPI_ST_SUCCESS)
        {
                /* DBMS error %d selecting DBA name */
                messageit(1, 96, errParm.ge_errorCode, errParm.ge_message);
                IIsw_close(stmtHandle, &errParm);
                return (FAIL);
        }
        IIsw_close(stmtHandle, &errParm);

        SW_CHA_TERM(dba, cdata[0]);
        SW_CHA_TERM(user, cdata[1]);
        if (STcompare(dba, user))
        {
                /*
                ** User '%s' is not the owner of database '%s'.  Use the -OWN
                ** startup flag to specify user '%s' as the correct owner.
                */
                messageit(1, 98, user, database_name, dba);
                return (FAIL);
        }
        STcopy(dba, conn->owner_name);

        if (RSconn_get_name_case(conn) != OK)
        {
                messageit(1, 113);
                IIsw_rollback(&conn->tranHandle, &errParm);
                return (FAIL);
        }
        IIsw_commit(&conn->tranHandle, &errParm);

        /* Selecting local database number */
        messageit(5, 1069);
        /* FIXME:  this overwrites the node info provided with the -IDB flag */
        STprintf(stmt, ERx("SELECT database_no, vnode_name FROM dd_databases \
WHERE database_name = '%s' AND local_db = 1"),
                conn->db_name);
        SW_COLDATA_INIT(cdata[0], conn->db_no);
        SW_COLDATA_INIT(cdata[1], conn->vnode_name);
        if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle, stmt, 0,
                NULL, NULL, 2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) !=
                IIAPI_ST_SUCCESS)
        {
                /* DBMS error %d selecting local database name */
                messageit(1, 1074, errParm.ge_errorCode, errParm.ge_message);
                IIsw_close(stmtHandle, &errParm);
                IIsw_rollback(&conn->tranHandle, &errParm);
                return (FAIL);
        }
        if (!IIsw_getRowCount(stmtHandle))
        {
                /* Local database information not found */
                messageit(1, 1077);
                return (FAIL);
        }
        IIsw_close(stmtHandle, &errParm);
        IIsw_commit(&conn->tranHandle, &errParm);
        STtrmwhite(conn->vnode_name);

        /* Local database '%s' is open */
        conn->status = CONN_OPEN;

        return (OK);
}

/*{
** Name:	RSshutdown - close databases & shutdown the Replicator server
**
** Description:
**	Shuts down the Replicator Server with the specified shutdown status.
**
** Inputs:
**	status	- exit status
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSshutdown(
STATUS	status)
{
	char			evtName[DB_MAXNAME+1];
	char			stmt[512];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;

	if (shutting_down)
		PCexit(FAIL);	/* ensure we don't recurse */
	else
		shutting_down = TRUE;

	/* The Server has shutdown */
	messageit(5, 1270);
	if (RSlocal_conn.status == CONN_OPEN)
	{
		STprintf(evtName, ERx("dd_server%d"), (i4)RSserver_no);
		if (IIsw_raiseEvent(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle, evtName,
			"The server has shutdown", &stmtHandle, &errParm) !=
			IIAPI_ST_SUCCESS)
		{
			/* DBMS error raising event */
			messageit(1, 1212, ERx("SHUTDOWN"),
				errParm.ge_errorCode, errParm.ge_message);
		}
		IIsw_close(stmtHandle, &errParm);

		/* update this server instance in the catalogs */
		STprintf(stmt, ERx("UPDATE dd_servers SET pid = '' "
			"WHERE server_no = %d"), RSserver_no);
		if (IIsw_query(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL, NULL,
			NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS ||
			IIsw_getRowCount(&getQinfoParm) != 1)
		{
			/* Error updating pid in dd_servers table */
			messageit(1, 1640);
			IIsw_close(stmtHandle, &errParm);
			IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		}
		IIsw_close(stmtHandle, &errParm);
		IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	}

	RSconns_close();
	remove_pidfile();
	if (status == OK)
		/* Replicator Server -- Normal shutdown */
		messageit(2, 44);
	else
		/* Replicator Server -- Shutdown due to error */
		messageit(2, 94);

	RSerrlog_close();

	IIsw_terminate(&RSenv);
	PCexit(status);
}

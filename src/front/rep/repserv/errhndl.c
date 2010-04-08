/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include "rsconst.h"
# include "conn.h"
# include "distq.h"
# include "repevent.h"
# include "repserv.h"

/**
** Name:	errhndl.c - handle error
**
** Description:
**	Defines
**		RSerror_handle	- handle error
**
** History:
**	16-dec-96 (joea)
**		Created based on derror.sc in replicator library.
**	07-feb-97 (joea)
**		Replace parts of RLIST by REP_TARGET.  Really correct SkipRow
**		error mode by always returning EM_NEXT_ROW.
**	05-sep-97 (joea)
**		Remove target/local rollbacks on deadlock.  This will be done
**		in IIRSabt_AbortTrans.  Remove unused local variables.
**	11-dec-97 (joea)
**		Add RS_TARGET argument and eliminate RStarget global.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	21-jan-99 (abbjo03)
**		Call IIsw_rollback if there is a deadlock.
**	23-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Dec-2004 (inifa01) INGREP169/Bug 113637
**	    With an error mode setting of Quiet Server, when an error occurs
**	    the server continues and could replicate transactions out of order
**	    if the transactions to be processed exceed the QBT setting.
**	    FIX:  If an error occurs and the error mode setting is Quiet server
**	    then turn of RSgo_again. 
**      2-Oct-2006 (kibro01) bug 116030
**          Return correct error code in the event of timeout error in
**          IIRSgrd_GetRowData so we can always loop round correctly and
**          try again.
**	24-May-2007 (kibro01) b118195
**	    If we lose the connection, quiet the target database but not the
**	    server (if connection retry is in effect) so we've got a chance of
**	    opening it again later.
**      22-Nov-2007 (kibro01) b119245
**          Add a flag to requiet a server if it was previously quiet
**          but has been unquieted for a connection retry.
**      20-Apr-2009 (coomi01) b121963
**          Promote test for REREAD on a timeout error, bug 116030, to 
**          caller RStransmit().
**/

/*{
** Name:	RSerror_handle - handle error
**
** Description:
**	Checks for DBMS errors, such as deadlock.  Reacts appropriately
**	depending on the error mode.
**
** Inputs:
**	target		- target database, CDDS and connection number
**	dberror		- DBMS error number
**
** Outputs:
**	none
**
** Returns:
**	ERR_NEXT_TRANS	- continue processing the link but skip this transaction
**	ERR_WAIT_EVENT	- go wait for an event
**	ERR_REREAD	- reload the linklist
**	ERR_NEXT_ROW	- skip this row
**
** Side effects:
**	May rollback transactions and raise dbevents.
*/
STATUS
RSerror_handle(
RS_TARGET	*target,
i4		dberror)
{
	char		sqlstate[DB_SQLSTATE_STRING_LEN+1];
	char		msg_buf[ER_MAX_LEN];
	STATUS		status;
	i4		len;
	CL_ERR_DESC	err_code;
	IIAPI_GETEINFOPARM	errParm;

	status = ERslookup(dberror, (CL_ERR_DESC *)NULL, ER_NAMEONLY, sqlstate,
		msg_buf, sizeof(msg_buf), (i4)-1, &len, &err_code, 0,
		(ER_ARGUMENT *)NULL);
	sqlstate[DB_SQLSTATE_STRING_LEN] = EOS;
	if (STequal(sqlstate, II_SS08006_CONNECTION_FAILURE) ||
		STequal(sqlstate, II_SS50005_GW_ERROR) ||
		STequal(sqlstate, II_SS50006_FATAL_ERROR))
	{
		if (target->conn_no != LOCAL_CONN)
		{
			/* Error communicating with database */
			messageit(1, 1655, RSconns[target->conn_no].db_name);
			if (RSconns[target->conn_no].status == CONN_OPEN)
			{
				RSconn_close(target->conn_no, CONN_FAIL);
			}
			return (ERR_REREAD);
		}
		else
		{
			/* Lost connection to local database */
			messageit(1, 64);
			RSshutdown(FAIL);
		}
	}

	if ( (dberror == E_AP0001_CONNECTION_ABORTED ||
	      dberror == E_AP0006_INVALID_SEQUENCE) &&
	     target->conn_no != LOCAL_CONN &&
	     RSconns[target->conn_no].status == CONN_OPEN &&
	     RSopen_retry > 0)
	{
		/* We received a connection abort, (or invalid-sequence, which
		** implies the same thing) which means we will already have
		** received an SC0206 which will have quietened the server,
		** so we have to unquiet the server but quiet the target 
		** database, so when we next get an event we can try to
		** reconnect. (kibro01) b118195
		*/
		RSconn_close(target->conn_no, CONN_FAIL);
		quiet_db(RSconns[target->conn_no].db_no, SERVER_QUIET);
		RSgo_again = FALSE;
		/* If this server is or was  -QIT, set it to return to that 
		** state after the connection retry (kibro01) b119245
		*/
		if (RSquiet)
			RSrequiet = TRUE;
		RSquiet = FALSE;
		return (ERR_REREAD);
	}

	/* timeout or deadlock -- reload and try again */
	if (STequal(sqlstate, II_SS40001_SERIALIZATION_FAIL) ||
			STequal(sqlstate, II_SS5000P_TIMEOUT_ON_LOCK_REQUEST))
	{
		IIsw_rollback(&RSconns[target->conn_no].tranHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (ERR_REREAD);
	}

	switch (target->error_mode)
	{
	case EM_SKIP_TRANS:
		break;

	case EM_SKIP_ROW:
		return (ERR_NEXT_ROW);

	case EM_QUIET_SERVER:
		RSquiet = TRUE;
		RSgo_again=FALSE;
		set_flags("-QIT");
		RSping(FALSE);
		return (ERR_WAIT_EVENT);

	case EM_QUIET_DB:
		if (target->db_no)
			quiet_db(target->db_no, SERVER_QUIET);
		return (ERR_REREAD);

	case EM_QUIET_CDDS:
		if (target->db_no)
			quiet_cdds(&target->db_no, target->cdds_no,
				SERVER_QUIET);
		return (ERR_REREAD);

	default:
		break;
	}

	return (ERR_NEXT_TRANS);
}

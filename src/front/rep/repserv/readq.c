/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <me.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "rsconst.h"
# include "conn.h"
# include "repserv.h"
# include "distq.h"

/**
** Name:	readq.c - read distribution queue
**
** Description:
**	Defines
**		read_q	- read distribution queue
**
** History:
**	16-dec-96 (joea)
**		Created based on readq.sc in replicator library.
**	03-feb-97 (joea)
**		RSqep should be GLOBALDEFd here. Remove CDDS lookup (now done
**		in transmit). Remove unnecessary null_trans_time.
**	25-mar-97 (joea)
**		Do not lookup tables here since discarding a row breaks
**		transaction integrity.  Call new_node() earlier in the SELECT
**		loop and eliminate need from trim_node(). Include distq.sh
**		instead of records.sh. Use DQ_ROW nested in RLIST.
**	04-sep-97 (joea)
**		Change global 'marker' to a local static.  Accept server number
**		as parameter instead of using global.  Rework return values to
**		provide more information and allow them to be returned by
**		IIRSgnt_GetNextTrans.
**	25-nov-97 (joea)
**		Merge target fields into RLIST. Rename QBM/QBT globals to avoid
**		name problems on MVS.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	22-jul-98 (abbjo03)
**		Remove GLOBALDEFs so they won't interfere with DistQ API.
**	31-jul-98 (abbjo03)
**		Add missing IIsw_getErrorInfo call. Change select to a repeated
**		query.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	08-mar-99 (abbjo03)
**		If there is a deadlock, first close the OpenAPI statement then
**		rollback.
**	23-aug-99 (abbjo03) sir 98493
**		Add support for row level locking and repeatable read isolation
**		level. Remove QEP flag since it's no longer used.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-Oct-2001 (inifa01) bug 105555 INGREP 97.
**		When the replicator server starts up, if the number of
**		transactions in the distribution queue is large
**		(exceeds QBM value) the server loops.
**		In read_q() when count > RSread_q_must_break and more than
**		one transaction has been read into the linked list we were
**		not properly traversing the list to find the end of the last
**		transaction.
**		Call new function 'pop_list_bottom()', to traverse and pop
**		the linked list from 'list_bottom'. [next_node() traverses
**		the list from 'list_top'.] Records from the current transaction
**		that cannot be completely loaded are removed from the list to
**		be processed on the next go.
**	23-jul-2002 (abbjo03)
**	    Add a test for RSgo_again to exit the SELECT loop (in 2.0 the breaks
**	    within the loop were ESQL ENDSELECTs).
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw... function.
**	27-Jul-2004 (inifa01) bug 112022 INGREP 156
**	    Changed go_again_dbno definition from static to GLOBALDEF
**/

static RLIST	*list_p;

static II_PTR	rdq_qryHandle = NULL;

GLOBALDEF i2	go_again_dbno = 0;
static i4	go_again_xid = 0;


/*{
** Name:	read_q - read distribution queue
**
** Description:
**	reads the distribution queue into a linked list. Only reads items
**	not intended for databases marked quiet.
**
** Inputs:
**	server_no	- server number
**
** Outputs:
**	none
**
** Returns:
**	OK		- one or more rows read
**	RS_NO_TRANS	- zero rows read
**	RS_REREAD	- deadlock or other recoverable error
**	RS_DBMS_ERR	- other DBMS error
**	RS_MEM_ERR	- transaction too big for QBM limit
**
** Side effects:
**	The linked list is populated with rows from the dd_distrib_queue.
**
**	16-Apr-2008 (kibro01) b120258
**	    If we've picked up some rows, even if the query is now at the
**	    end (which can happen if we've exceeded QBT but finished the
**	    query in the same batch of rows) then we do want to go ahead
**	    and replicate them.
*/
STATUS
read_q(
i2	server_no)
{
	RLIST			list_row;
	i4			count = 0;
	i2			last_sourcedb = -1;
	i4			last_trans_id = 0;
	bool			trans_too_big = FALSE;
	RS_CONN			*conn = &RSlocal_conn;
	IIAPI_DESCRIPTOR	pdesc;
	IIAPI_DATAVALUE		pdata;
	IIAPI_DATAVALUE		cdv[13];
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;
	II_LONG			rowCount;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("RSread_q") };

	messageit(5, 1034, ERx("read_q"));
	RSgo_again = FALSE;
	init_list();

	IIsw_query(conn->connHandle, &conn->tranHandle,
		ERx("SET TRANSACTION ISOLATION LEVEL READ COMMITTED"),
		0, NULL, NULL, NULL, NULL, &stmtHandle,
		&getQinfoParm, &errParm);
	IIsw_close(stmtHandle, &errParm);

	stmtHandle = NULL;
	SW_COLDATA_INIT(cdv[0], list_row.row.rep_key.src_db);
	SW_COLDATA_INIT(cdv[1], list_row.row.rep_key.trans_id);
	SW_COLDATA_INIT(cdv[2], list_row.row.rep_key.seq_no);
	SW_COLDATA_INIT(cdv[3], list_row.row.table_no);
	SW_COLDATA_INIT(cdv[4], list_row.row.old_rep_key.src_db);
	SW_COLDATA_INIT(cdv[5], list_row.row.old_rep_key.trans_id);
	SW_COLDATA_INIT(cdv[6], list_row.row.old_rep_key.seq_no);
	SW_COLDATA_INIT(cdv[7], list_row.row.trans_time);
	SW_COLDATA_INIT(cdv[8], list_row.row.trans_type);
	SW_COLDATA_INIT(cdv[9], list_row.target.cdds_no);
	SW_COLDATA_INIT(cdv[10], list_row.row.priority);
	SW_COLDATA_INIT(cdv[11], list_row.target.type);
	SW_COLDATA_INIT(cdv[12], list_row.target.db_no);
	SW_PARM_DESC_DATA_INIT(pdesc, pdata, IIAPI_INT_TYPE, server_no);
	while (TRUE)
	{
		status = IIsw_selectLoopRepeated(conn->connHandle,
			&conn->tranHandle,
			ERx("SELECT sourcedb, transaction_id, sequence_no, \
table_no, old_sourcedb, old_transaction_id, old_sequence_no, \
DATE_GMT(trans_time) AS trans_time, trans_type, q.cdds_no, dd_priority, \
c.target_type, targetdb FROM dd_distrib_queue q, dd_db_cdds c, dd_databases d \
WHERE q.cdds_no = c.cdds_no AND targetdb = c.database_no AND c.server_no = $0 \
= ~V AND c.is_quiet = 0 AND c.database_no = d.database_no AND \
LOWERCASE(d.vnode_name) != 'mobile' ORDER BY trans_time, targetdb, sourcedb, \
transaction_id, cdds_no, sequence_no"),
			1, &pdesc, &pdata, &qryId, &rdq_qryHandle, 13, cdv,
			&stmtHandle, &getQinfoParm, &errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(list_row.row.trans_time, cdv[7]);
		if (go_again_dbno)
		{
			if (list_row.row.rep_key.src_db != go_again_dbno ||
				list_row.row.rep_key.trans_id != go_again_xid)
			{
				continue;
			}
			else
			{
				go_again_dbno = 0;
				go_again_xid = 0;
			}
		}
		if (++count > RSqbt_read_try_to_break)
		{
			if (list_row.row.rep_key.src_db != last_sourcedb ||
				list_row.row.rep_key.trans_id != last_trans_id)
			{
				--count;
				RSgo_again = TRUE;
				go_again_dbno = list_row.row.rep_key.src_db;
				go_again_xid = list_row.row.rep_key.trans_id;
				break;
			}

			if (count > RSqbm_read_must_break)
			{
				list_p = list_top();
				/* quick check: is this the only transaction? */
				if (list_p->row.rep_key.src_db == last_sourcedb
					&& list_p->row.rep_key.trans_id ==
					last_trans_id)
				{
					init_list();
					trans_too_big = TRUE;
					break;
				}

				/*
				** go to end of list and traverse up to find
				** end of last transaction.
				*/
				list_p = list_bottom();
				while (list_p  != NULL)
				{
					if (list_p->row.rep_key.src_db !=
						last_sourcedb ||
						list_p->row.rep_key.trans_id !=
						last_trans_id)
					{
						RSgo_again = TRUE;
						go_again_dbno= last_sourcedb;
						go_again_xid= last_trans_id;
						/* Done to set 'current' to
						** 'top' instead of 'bottom',
						** necessary for next_row()
						** call when processing returns
						** to RStransmit().
						*/
                                                (void)list_top();
						break;
					}
					/* Traverse list and remove 'bottom' entry. */
					list_p = pop_list_bottom();
					--count;
				}
			}
		}

		if (RSgo_again)
		    break;
		/* copy all fields of list row but preserve link & blink */
		list_p = new_node();
		MEcopy(&list_row, CL_OFFSETOF(RLIST, link), list_p);

		last_sourcedb = list_p->row.rep_key.src_db;
		last_trans_id = list_p->row.rep_key.trans_id;
	}

	if (status != IIAPI_ST_NO_DATA && status != IIAPI_ST_SUCCESS)
	{
		if (STequal(errParm.ge_SQLSTATE, II_SS08006_CONNECTION_FAILURE)
			|| STequal(errParm.ge_SQLSTATE, II_SS50005_GW_ERROR) ||
			STequal(errParm.ge_SQLSTATE, II_SS50006_FATAL_ERROR))
			/* fatal error */
		{
			/* Lost connection to local database */
			messageit(1, 1868);
			IIsw_close(stmtHandle, &errParm);
			RSshutdown(FAIL);
		}

		/* timeout or deadlock -- reload and try again */
		if (STequal(errParm.ge_SQLSTATE, II_SS40001_SERIALIZATION_FAIL)
			|| STequal(errParm.ge_SQLSTATE,
			II_SS5000P_TIMEOUT_ON_LOCK_REQUEST))
		{
			IIsw_close(stmtHandle, &errParm);
			IIsw_rollback(&conn->tranHandle, &errParm);
			RSgo_again = TRUE;
			return (RS_REREAD);
		}

		IIsw_close(stmtHandle, &errParm);
		messageit(1, 1652, errParm.ge_errorCode, errParm.ge_message);
		return (RS_DBMS_ERR);
	}
	rowCount = IIsw_getRowCount(&getQinfoParm);
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&conn->tranHandle, &errParm);

	if (!rowCount && !count)
	{
		/* Nothing in distribution queue */
		messageit(3, 1275);
		return (RS_NO_TRANS);
	}

	if (trans_too_big)
	{
		messageit(1, 1695, last_sourcedb, last_trans_id);
		return (RS_MEM_ERR);
	}

	/* %d records read from dd_distrib_queue */
	messageit(4, 1276, count);
	return (OK);
}

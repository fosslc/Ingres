/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "distq.h"
# include "repserv.h"

/**
** Name:	distqapi.c - distribution queue API
**
** Description:
**	Defines
**		IIRSgnt_GetNextTrans	- get next transaction
**		IIRSgnr_GetNextRowInfo	- get next row information
**		IIRSrlt_ReleaseTrans	- remove transaction from queue
**		IIRSabt_AbortTrans	- abort current transaction
**		IIRSclq_CloseQueue	- close distribution queue
**
** History:
**	28-aug-97 (joea)
**		Created based on distq.c in replicator library.
**	18-nov-97 (joea)
**		Add missing test for target database in IIRSgnr_GetNextRow.
**	11-dec-97 (joea)
**		Change IIRSgnr_GetNextRow to IIRSgnr_GetNextRowInfo. Return a
**		status from IIRSabt_AbortTrans. Added RS_TARGET to RLIST.
**		Eliminate RStarget global.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.  Merge
**		IIRSrlt_ReleaseTrans from clearq.sc.
**	11-jun-98 (abbjo03) bug 84031
**		Retry MAX_RETRIES times to delete from dd_distrib_queue. Also,
**		correct the logic for testing for a serialization or timeout
**		error.
**	09-jul-98 (abbjo03)
**		Add two new arguments to IIsw_query.
**	28-jul-98 (abbjo03)
**		Use repeated query in IIRSrlt_ReleaseTrans.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	31-aug-99 (abbjo03) bug 98604
**		In IIRSrlt_ReleaseTrans, before retrying call RSdtrans_id_re-
**		register to reinstate the distributed transaction ID.
**	27-sep-99 (abbjo03) bug 98604
**		Correct 31-aug change. Instead of reregistering the distrib
**		tran ID, restore it by calling RSdtrans_id_get_last.
**	24-nov-99 (abbjo03) bug 84031
**		Prevent RSerror_check from rolling back the remote transaction
**		if a serialization error occurs in IIRSrlt_ReleaseTrans.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw function calls.
**/

# define MAX_RETRIES		3


GLOBALREF bool	RSonerror_norollback_target;


static RLIST	*curr_row;
static bool	first_row = TRUE;
static bool	queue_open = FALSE;
static II_PTR	rlt_qryHandle = NULL;


/*{
** Name:	IIRSgnt_GetNextTrans	- get next transaction
**
** Description:
**	Gets the next transaction from the distribution queue for a given
**	server, either by calling read_q or by advancing through the in-memory
**	list.  Actually IIRSgnr_GetNextRowIinfo always does the advancing.
**
** Inputs:
**	server_no	- server number
**
** Outputs:
**	target		- target database and CDDS
**	trans		- source database and transaction identifier
**
** Returns:
**	OK
**	RS_NO_TRANS	- no transactions in distribution queue
**	RS_REREAD	- deadlock or other recoverable error
**	RS_DBMS_ERR	- DBMS non-recoverable error
**	RS_MEM_ERR	- transaction too big (QBM exceeded)
**
** Side effects:
**	If list is empty, read_q is called and curr_row is set to the
**	head of the list.  First_row is set to true so that
**	IIRSgnr_GetNextRowIinfo won't advance.
*/
STATUS
IIRSgnt_GetNextTrans(
i2		server_no,
RS_TARGET	*target,
RS_TRANS_KEY	*trans)
{
	STATUS	status;

	if (curr_row == NULL)
	{
		if (queue_open)
			return (RS_NO_TRANS);

		status = read_q(server_no);
		if (status != OK)
			return (status);
		curr_row = list_top();
		if (curr_row == NULL)
			return (RS_NO_TRANS);
		queue_open = TRUE;
	}

	target->db_no = curr_row->target.db_no;
	target->cdds_no = curr_row->target.cdds_no;
	target->type = curr_row->target.type;
	trans->src_db = curr_row->row.rep_key.src_db;
	trans->trans_id = curr_row->row.rep_key.trans_id;
	first_row = TRUE;

	return (OK);
}


/*{
** Name:	IIRSgnr_GetNextRowInfo	- get next row information
**
** Description:
**	Gets the next row in the distribution queue for a given transaction.
**	This currently assumes that read_q will order the transactions in
**	the in-memory linked list in the right order.
**
** Inputs:
**	server_no	- server number
**	target		- target database and CDDS
**	row		- transaction row
**
** Outputs:
**	row		- transaction row
**
** Returns:
**	OK
**	RS_NO_ROWS	- no more rows
**
** Side effects:
**	Curr_row is advanced to the next item in the linked list, unless
**	this is the first row in a transaction (in which case first_row
**	is negated).
*/
STATUS
IIRSgnr_GetNextRowInfo(
i2		server_no,
RS_TARGET	*target,
RS_TRANS_ROW	*row)
{
	if (!first_row)
		curr_row = next_node();
	else
		first_row = FALSE;

	if (curr_row == NULL)
		return (RS_NO_ROWS);

	if (curr_row->row.rep_key.src_db != row->rep_key.src_db ||
		curr_row->row.rep_key.trans_id != row->rep_key.trans_id ||
			curr_row->target.cdds_no != target->cdds_no ||
			curr_row->target.db_no != target->db_no)
		return (RS_NO_ROWS);

	MEcopy((PTR)&curr_row->row, CL_OFFSETOF(RS_TRANS_ROW, data_len),
		(PTR)row);

	return (OK);
}


/*{
** Name:	IIRSrlt_ReleaseTrans - remove transaction from queue
**
** Description:
**	Remove the specified transaction from the distribution queue.
**
** Inputs:
**	server_no	- server number (not used)
**	target		- the target database and CDDS
**	trans		- the originating transaction
**
** Outputs:
**	none
**
** Returns:
**	OK or the ge_errorCode from the IIsw_query DELETE call.
*/
STATUS
IIRSrlt_ReleaseTrans(
i2		server_no,
RS_TARGET	*target,
RS_TRANS_KEY	*trans)
{
	i4			retries;
	IIAPI_DESCRIPTOR	pdesc[4];
	IIAPI_DATAVALUE		pdv[4];
	II_PTR			stmtHandle;
	IIAPI_STATUS            status;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("IIRSrlt_ReleaseTrans") };

	SW_PARM_DESC_DATA_INIT(pdesc[0], pdv[0], IIAPI_INT_TYPE, trans->src_db);
	SW_PARM_DESC_DATA_INIT(pdesc[1], pdv[1], IIAPI_INT_TYPE,
		trans->trans_id);
	SW_PARM_DESC_DATA_INIT(pdesc[2], pdv[2], IIAPI_INT_TYPE, target->db_no);
	SW_PARM_DESC_DATA_INIT(pdesc[3], pdv[3], IIAPI_INT_TYPE,
		target->cdds_no);
	for (retries = 0; retries < MAX_RETRIES; ++retries)
	{
		status = IIsw_queryRepeated(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("DELETE FROM dd_distrib_queue WHERE sourcedb = $0 \
= ~V AND transaction_id = $1 = ~V AND targetdb = $2 = ~V AND cdds_no = $3 = \
~V"),
			4, pdesc, pdv, &qryId, &rlt_qryHandle, &stmtHandle,
			&getQinfoParm, &errParm);
		RSonerror_norollback_target = TRUE;
		status = RSerror_check(1021, ROWS_ONE_OR_MORE, stmtHandle,
			 &getQinfoParm, &errParm, NULL, trans->src_db, trans->trans_id,
			target->db_no);
		RSonerror_norollback_target = FALSE;
		if (status == OK)
			break;
		if (!(STequal(errParm.ge_SQLSTATE,
				II_SS40001_SERIALIZATION_FAIL) ||
				STequal(errParm.ge_SQLSTATE,
				II_SS5000P_TIMEOUT_ON_LOCK_REQUEST)))
			break;
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		RSlocal_conn.tranHandle = RSdtrans_id_get_last();
	}
	
	return (status);
}


/*{
** Name:	IIRSabt_AbortTrans - abort current transaction
**
** Description:
**	Abort the current transaction.
**
** Inputs:
**	server_no	- server number (not used)
**	target		- the target database and CDDS
**	trans		- source database and transaction identifier
**
** Outputs:
**	none
**
** Returns:
**	OK		- transaction aborted
**	RS_DBMS_ERR	- couldn't rollback
*/
STATUS
IIRSabt_AbortTrans(
i2		server_no,
RS_TARGET	*target,
RS_TRANS_KEY	*trans)
{
	IIAPI_GETEINFOPARM	errParm;

	if (target->conn_no != UNDEFINED_CONN)
		if (IIsw_rollback(&RSconns[target->conn_no].tranHandle,
				&errParm) != IIAPI_ST_SUCCESS)
			return (RS_DBMS_ERR);

	if (IIsw_rollback(&RSlocal_conn.tranHandle, &errParm) !=
			IIAPI_ST_SUCCESS)
		return (RS_DBMS_ERR);

	while (curr_row != NULL &&
			curr_row->row.rep_key.src_db == trans->src_db &&
			curr_row->row.rep_key.trans_id == trans->trans_id &&
			curr_row->target.db_no == target->db_no &&
			curr_row->target.cdds_no == target->cdds_no)
		curr_row = next_node();

	return (OK);
}


/*{
** Name:	IIRSclq_CloseQueue	- close distribution queue
**
** Description:
**	Closes the distribution queue.
**
** Inputs:
**	server_no	- server number
**
** Outputs:
**	none
**
** Returns:
**	OK
*/
STATUS
IIRSclq_CloseQueue(
i2	server_no)
{
	curr_row = NULL;
	first_row = TRUE;
	queue_open = FALSE;
	return (OK);
}

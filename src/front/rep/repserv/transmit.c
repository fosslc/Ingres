/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <tm.h>
# include <me.h>
# include <iiapi.h>
# include <sapiw.h>
# include <targtype.h>
# include "rsconst.h"
# include "conn.h"
# include "cdds.h"
# include "distq.h"
# include "repevent.h"
# include "repserv.h"
# include "rsstats.h"
# include "tblinfo.h"

/**
** Name:	transmit.c - transmit
**
** Description:
**	Defines
**		RStransmit	- transmit replicated transactions
**
** History:
**	16-dec-96 (joea)
**		Created based on transmit.sc in replicator library.
**	03-feb-97 (joea)
**		Replace clear_q with IIRSrlt_ReleaseTrans. Rename transmit()
**		to RStransmit().  Call read_q() and check_connections() at the
**		start of RStransmit().  Add missing parameter to messageit 1081.
**		Replace parts of RLIST by REP_TARGET. Add GLOBALDEFs for marker
**		and RStarget. Call RSrdf_gdesc and add RS_TBLDESC arguments to
**		d_insert, d_update, d_delete and RScommit. Replace rest of
**		RLIST by DQ_ROW.
**	26-mar-97 (joea)
**		Look up table info (removed from read_q()). Call d_error() if
**		table not registered. Temporarily use TBL_ENTRY instead of
**		RS_TBLDESC (latter will be used for generic RepServer) when
**		calling d_insert, d_update and d_delete.
**	28-apr-97 (joea) bug 81363
**              Test first for known CDDS.  If not found, use SkipTransaction
**              (default) error mode.  Then test for known table.  If not found,
**              use SkipTransaction/SkipRow error modes, as applicable.  Rename
**              err_flag to next_trans for clarity.
**	21-jun-97 (joea)
**		Only do the collision/error mode assignments if we aren't
**		skipping to the next transaction.
**	24-jun-97 (joea)
**		Add cdds_no to call to IIRSrlt_ReleaseTrans.
**	05-sep-97 (joea)
**		Implement the Distribution Queue API.
**	24-oct-97 (joea)
**		Commit after raising the transaction dbevents in order not to
**		leave an open transaction.
**	27-oct-97 (joea) bug 86723
**		If d_error returns ERR_REREAD, set result to ERR_REREAD to
**		force another call into RStransmit.
**	14-nov-97 (joea)
**		Capture the return value of IIRSgnt_GetNextTrans and shutdown
**		the RepServer if necessary.
**	05-dec-97 (padni01) bug 87507
**		Move call to lkup_cdds outside and before the if condition 
**		to ensure that it is executed.
**	11-dec-97 (joea)
**		Implement rev 2 of Replicator API. Replace d_insert/update/
**		delete by RSinsert/update/delete. Eliminate TBL_ENTRY,
**		RStbl_funcs, and REP_TARGET.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	07-may-98 (joea) bug 90775
**		If we cannot connect to the target, set RStarget.conn_no to
**		UNDEFINED_CONN.
**	02-jun-98 (abbjo03)
**		Reset result at bottom of GetNextTrans loop.
**	29-jun-98 (abbjo03)
**		Call RSfdb_FreeDescBuffers on exit from GetNextRow loop.
**	23-jul-98 (abbjo03)
**		Call RSaro_AdjRemoteOwner to deal with the remote DBA being
**		different from the local DBA.
**	27-oct-98 (abbjo03)
**		Support VDBA manual collision resolution. If dd_priority is
**		negative, change collision mode to priority resolution.
**	03-nov-98 (abbjo03)
**		Add call to update statistics.
**	03-dec-98 (abbjo03)
**		Replace message numbers that had temporarily been assigned
**		message number 1739.
**	19-jan-99 (abbjo03)
**		After IIRSgrd_GetRowData, call RSerror_handle to determine if we
**		need to reread the queue.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Jul-2004 (inifa01) bug 112022 INGREP 156
**	    Transactions can be replicated out of order if the records in
**	    the queue exceed the QBT/QBM settings and a timeout or deadlock
**	    error occurs.
**	    FIX: unset go_again_dbno if re-reading of the distribution queue
**	    will occur due to ERR_REREAD.
**	12-Jul-2006 (kibro01) bugs 116030 and 116031
**	    If a transaction fails, record any dependencies of the transaction
**	    and skip them further down the transaction list.
**	    Also mark that we will loop round the replication cycle if there
**	    were new rejected transactions so we can redo deadlock ones now
**	    rather than waiting for the next cycle
**	16-Aug-2007 (drivi01) BUG 119307
**	    Upon declaration RS_TRANS_ROW structure sometimes is not 
**	    initialized properly resulting in a SEGV when next_trans 
**	    is set to TRUE. Added code to initialize fields in row 
**	    structure to NULL right after declaration.
**	19-Mar-2008 (kibro01) b120125
**	    If a parent TX ID is the same as this TX ID (such as when two
**	    changes are made to the same record in the same TX) don't check
**	    that TX in the rejected list - otherwise since it was there from
**	    the last run through it will flag up as rejected immediately again.
**/

GLOBALDEF bool	RSin_transmit = FALSE;
GLOBALREF i2       go_again_dbno ;
/* Note: only error_get_string uses this global */
GLOBALDEF RS_TARGET	*RStarget;

static i4 *rej_trans_list=NULL;
static i4 rej_trans_list_size=0;
static i4 rej_trans_list_count=0;
static bool added_rej_trans=0;
#define REJ_TRANS_LIST_BLOCK	1000

static STATUS
add_rej_trans(
	i4 trans_id);

static bool
found_rej_trans(
	i4	trans_id);

static void
drop_rej_trans(
	i4 trans_id);


/*{
** Name:	RStransmit - transmit replicated transactions
**
** Description:
**	Calls funtions that actually perform inserts, updates, and
**	deletes in target database.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	0		- shutdown the server
**	ERR_WAIT_EVENT	- go wait for events
**	ERR_REREAD	- call RStransmit again without waiting for events
** History:
**
**      20-Apr-2009 (coomi01) b121963
**          Translate error code RS_REREAD on timeout from IIRSgrd_GetRowData()
**          to ERR_REREAD here rather than call the error handler to do it
**          for us.
*/
STATUS
RStransmit()
{
	STATUS		err;
	STATUS		err_trans;
	STATUS		result = ERR_WAIT_EVENT;
	bool		next_trans;
	RS_CONN		*conn;
	RS_TARGET	target;
	RS_TRANS_ROW	row;
	IIAPI_GETEINFOPARM	errParm;

	/*initialize row structure */
	row.key_data = NULL;
	row.row_data = NULL;
	row.key_desc = NULL;
	row.row_desc = NULL;

	RSin_transmit = TRUE;
	while ((err = IIRSgnt_GetNextTrans(RSserver_no, &target, &row.rep_key))
		== OK)
	{
		RS_CDDS		*cdds;

		next_trans = FALSE;
		RStarget = &target;
		target.conn_no = RSconn_lookup(target.db_no);
		conn = &RSconns[target.conn_no];
		if (conn->status == CONN_CLOSED)
		{
			if (RSconn_open(target.conn_no) != OK)
			{
				RSconn_close(target.conn_no, CONN_FAIL);
				target.conn_no = UNDEFINED_CONN;
			}
		}
		if (conn->status != CONN_OPEN)
		{
			IIRSabt_AbortTrans(RSserver_no, &target, &row.rep_key);
			continue;
		}
		conn->last_needed = TMsecs();

		if (RScdds_lookup(target.cdds_no, &cdds) != OK)
		{
			messageit(1, 1740, target.cdds_no);
			IIRSabt_AbortTrans(RSserver_no, &target, &row.rep_key);
			continue;
		}
		target.collision_mode = cdds->collision_mode;
		target.error_mode = cdds->error_mode;

		if (RSdtrans_id_register(&target) != OK)
			RSshutdown(FAIL);
		row.data_len = row.key_len = 0;
		err_trans = OK;
		while ((err = IIRSgnr_GetNextRowInfo(RSserver_no, &target,
			&row)) == OK)
		{

			/* Check to see if the parent of this transaction
			** is in our rejected list.  If so, this is a
			** dependent transaction and must also be skipped
			** Add check (b120125) for Parent TX != This TX
			*/
			if (!next_trans &&
			    row.old_rep_key.trans_id != row.rep_key.trans_id &&
			    found_rej_trans(row.old_rep_key.trans_id))
			{
				next_trans = TRUE;
			}
			if (next_trans)
			{
				/* If we are ignoring this because its parent
				** was rejected, we must note down each 
				** transaction ID within it as rejected too
				*/
				if (row.old_rep_key.trans_id!=0)
				{
					if (add_rej_trans(row.rep_key.trans_id)
						!=OK)
					{
						result = 0;
						break;
					}
				}
				RSfdb_FreeDescBuffers(&row);
				continue;
			}
		
			if (RSrdf_gdesc(row.table_no, &row.tbl_desc) != OK)
			{
				messageit(1, 1739, row.table_no);
				if (target.error_mode != EM_SKIP_ROW)
					next_trans = TRUE;
			}

			if (!next_trans && RSpdb_PrepDescBuffers(&row) != OK)
			{
				messageit(1, 110, row.rep_key.src_db,
					row.rep_key.trans_id,
					row.rep_key.seq_no);
				next_trans = TRUE;
			}

			if (!next_trans)
			{
				if ((err = IIRSgrd_GetRowData(RSserver_no,
					&target, &row)) != OK)
				{
				    /*
				    ** Translate a RS_REREAD here rather than call
				    ** RSerror_handle() to do it for us.
				    */	
				    if ( err == RS_REREAD )
				    {
					err_trans = ERR_REREAD;
				    }
				    else
				    {
					err_trans = RSerror_handle(&target, err);
				    }

				    if (err_trans == ERR_NEXT_ROW)
					continue;
				    messageit(3, 111, row.rep_key.src_db,
					row.rep_key.trans_id,
					row.rep_key.seq_no);
				    RSfdb_FreeDescBuffers(&row);
				    next_trans = TRUE;
				    /* Remember that this transaction
				    ** has been rejected so its children 
				    ** cannot be applied
				    */
				    if (add_rej_trans(row.rep_key.trans_id)!=OK)
				    {
					result = 0;
					break;
				    }
				    /* Continue instead of break so we
				    ** can note down the rest of this
				    ** transaction in the rejected list
				    */
				    continue;
				}
			}

			RSaro_AdjRemoteOwner(row.tbl_desc, conn->owner_name);
			if (row.priority < 0)
				target.collision_mode = COLLMODE_PRIORITY;
			switch (row.trans_type)
			{
			case RS_INSERT:
				err = RSinsert(&target, &row);
				break;

			case RS_UPDATE:
				err = RSupdate(&target, &row);
				break;

			case RS_DELETE:
				err = RSdelete(&target, &row);
				break;

			default:
				/*
				** Unknown transaction type %d with database
				** %d, transaction id %d, sequence %d
				*/
				messageit(1, 1080, row.trans_type,
					row.rep_key.src_db,
					row.rep_key.trans_id,
					row.rep_key.seq_no);
				err = 0;
				next_trans = TRUE;
			}

			RSfdb_FreeDescBuffers(&row);
			if (err)
			{
				err_trans = RSerror_handle(&target, err);
				if (err_trans == ERR_NEXT_ROW)
					continue;
				next_trans = TRUE;
				/* Remember that this transaction
				** has been rejected so its children 
				** cannot be applied
				*/
				if (add_rej_trans(row.rep_key.trans_id)!=OK)
				{
					result = 0;
					break;
				}
			}
		}	/* while GetNextRow */

		if (next_trans)
		{
			IIRSabt_AbortTrans(RSserver_no, &target, &row.rep_key);
			if (err_trans == ERR_NEXT_TRANS)
				continue;
			if (err_trans == ERR_REREAD)
				result = ERR_REREAD;
			/* ERR_WAIT_EVENT or ERR_REREAD */
			break;
		}
		else
		{
			if (err == RS_DBMS_ERR || err == RS_MEM_ERR)
			{
				result = 0;
				break;
			}
			else if (err == RS_REREAD)
			{
				result = ERR_REREAD;
				break;
			}

			/* Make sure this isn't in our rejected trans list */
			drop_rej_trans(row.rep_key.trans_id);

			/* Commiting transaction_id %d */
			messageit(5, 1081, row.rep_key.trans_id);
			/* Clear out distrib queue for this trans */
			err = IIRSrlt_ReleaseTrans(RSserver_no, &target,
				&row.rep_key);
			if (err)
			{
				result = RSerror_handle(&target, err);
				break;
			}
			err = RScommit(&target, &row);
			if (err)
			{
				result = RSerror_handle(&target, err);
				break;
			}
			RSstats_update(target.db_no, (i4)0, (i4)0);
			if (RSmonitor)
			{
				RSmonitor_notify(&RSlocal_conn, RS_OUT_TRANS,
					target.db_no, "Transactions", NULL);
				IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
				if (target.type != TARG_UNPROT_READ)
				{
					RSmonitor_notify(conn, RS_INC_TRANS,
						RSlocal_conn.db_no,
						"Transactions", NULL);
					IIsw_commit(&conn->tranHandle,
						&errParm);
				}
			}

			if ((result = RSdbevent_get_nowait()) == 0)
				break;		/* shutdown server */
		}
		result = ERR_WAIT_EVENT;
	}	/* while GetNextTrans */

	if (err == RS_DBMS_ERR || err == RS_MEM_ERR || err == RS_INTERNAL_ERR)
		result = 0;

	if (IIRSclq_CloseQueue(RSserver_no) != OK)
	{
		messageit(1, 103, RSserver_no);
		result = 0;
	}

	if (result == ERR_REREAD)
		/*
		** Reading the dist q due to both 'REREAD' and 'RSgo_again' 
		** is contradictory.  If returning ERR_REREAD then unset
		** 'go_again_dbno'.  (b112022)
		*/
	    	go_again_dbno = 0;

	RSin_transmit = FALSE;

	/* If we have got a rejected transaction now and didn't before,
	** (with deadlock, for example) then it's probable that it will
	** go in if we just try again, so flag up that we should loop
	*/
	if (go_again_dbno == 0 && added_rej_trans)
	{
		added_rej_trans = FALSE;
		RSgo_again = TRUE;
	}

	return (result);
}

/*{
** Name:	found_rej_trans - Check if transaction is in rejected list
**
** Description:
**	Checks to see if this transaction ID was not performed earlier in
**	the cycle.
**
** Inputs:
**	trans_id: Transaction ID to check
**
** Outputs:
**	none
**
** Returns:
**	TRUE		- Transaction ID found in list
**	FALSE		- Transaction ID not found in list
**
** History:
**	12-Jul-2006 (kibro01)
**		Created function to manage parent transactions
*/
static bool
found_rej_trans(
	i4	trans_id)
{
	i4 loop;
	i4 counter;

	if (rej_trans_list == NULL || rej_trans_list_count == 0)
		return (FALSE);

	counter = rej_trans_list_count;
	for (loop = 0; loop < rej_trans_list_size && counter > 0; loop++)
	{
		if (rej_trans_list[loop] != 0 &&
		    rej_trans_list[loop] == trans_id)
		{
			return (TRUE);
		}
		if (rej_trans_list[loop] != 0)
		{
			counter--;
		}
	}
	return (FALSE);
}

/*{
** Name:	add_rej_trans - Add rejected transaction to list
**
** Description:
**	Add rejected transaction ID to list of rejections
**
** Inputs:
**	trans_id - Transaction ID which failed
**
** Outputs:
**	none
**
** Returns:
**	OK		- Everything OK
**	other		- Error from MEreqmem
**
** History:
**	12-Jul-2006 (kibro01)
**		Created function to manage parent transactions
*/
static STATUS
add_rej_trans(
	i4 trans_id)
{
	i4 loop = 0;
	i4 *new_rej_trans_list;
	i4 new_rej_trans_list_size;
	STATUS ret;

	if (found_rej_trans(trans_id))
		return (OK);

	if (rej_trans_list == NULL)
	{
		rej_trans_list=(i4*)MEreqmem(0,
			sizeof(i4) * REJ_TRANS_LIST_BLOCK, TRUE, &ret);
		if (ret != OK)
			return (ret);
		rej_trans_list_size = REJ_TRANS_LIST_BLOCK;
	}

	for (loop = 0; loop < rej_trans_list_size; loop++)
	{
		if (rej_trans_list[loop] == 0)
		{
			rej_trans_list[loop] = trans_id;
			added_rej_trans = TRUE;
			rej_trans_list_count++;
			return (OK);
		}
	}

	new_rej_trans_list_size = rej_trans_list_size + REJ_TRANS_LIST_BLOCK;

	new_rej_trans_list = (i4*)MEreqmem(0,
			new_rej_trans_list_size * sizeof(i4), TRUE, &ret);

	if (ret != OK)
		return (ret);

	MEcopy(rej_trans_list, sizeof(i4) * rej_trans_list_size,
			new_rej_trans_list);
	MEfree((PTR)rej_trans_list);

	rej_trans_list = new_rej_trans_list;
	rej_trans_list[rej_trans_list_size] = trans_id;
	rej_trans_list_size = new_rej_trans_list_size;
	added_rej_trans = TRUE;
	rej_trans_list_count++;
	return (OK);
}

/*{
** Name:	drop_rej_trans - Mark a transaction as successful
**
** Description:
**	Check if the transaction ID given was in the failed list,
**	and if so remove it since it has now been successfully handled
**
** Inputs:
**	trans_id - Successful transaction ID
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	12-Jul-2006 (kibro01)
**		Created function to manage parent transactions
*/
static void
drop_rej_trans(
	i4 trans_id)
{
	if (found_rej_trans(trans_id))
	{
		i4 loop = 0;
		for (loop = 0; loop < rej_trans_list_size; loop++)
		{
			if (rej_trans_list[loop] != 0 &&
			    rej_trans_list[loop] == trans_id)
			{
				rej_trans_list[loop] = 0;
				rej_trans_list_count--;
			}
		}
	}
	return;
}

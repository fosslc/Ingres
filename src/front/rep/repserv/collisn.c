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
# include <rpu.h>
# include <targtype.h>
# include <tblobjs.h>
# include "rsconst.h"
# include "conn.h"
# include "tblinfo.h"
# include "repserv.h"
# include "distq.h"

/**
** Name:	collisn.c - collision handling
**
** Description:
**	Defines
**		RScollision			- collision
**		pre_clean			- pre-clean the queue
**		resolve				- resolve a collision
**		resolve_insert_by_time		- resolve an insert by time
**		resolve_insert_by_priority	- resolve an insert by priority
**		resolve_insert			- resolve an insert
**		resolve_update_by_time		- resolve an update by time
**		resolve_update_by_priority	- resolve an update by priority
**		resolve_missing_update		- resolve a missing update
**		resolve_update			- resolve an update
**		resolve_delete_by_time		- resolve a delete by time
**		resolve_delete_by_priority	- resolve a delete by priority
**		resolve_missing_delete		- resolve a missing delete
**		resolve_delete			- resolve an delete
**		delete_remote_record		- delete a remote record
**		get_local_trans_time		- get local transaction time
**		calc_time_diffs			- calculate time differences
**		calc_prio_diffs			- calculate priority differences
**		old_trans_archived		- old transaction archived?
**		old_trans_new_key		- old transaction has new key?
**		base_row_exists			- does base row exist?
**
** History:
**	16-dec-96 (joea)
**		Created based on collisn.sc in replicator library.
**	20-jan-97 (joea)
**		Use defined constants for target types. Add msg_func parameter
**		to create_key_clauses.
**	10-feb-97 (joea)
**		Replace parts of RLIST by REP_TARGET. Rename collision(), the
**		only public entry point, to RScollision(). Add RS_TBLDESC and
**		DQ_ROW arguments to RScollision and create statics tbl,
**		shadow_name and row for access by the other functions. Revert
**		pre_clean queries to dynamic SQL.
**	24-feb-97 (joea) bug 79468
**		In resolve(), for INSERTs test for a zero row count in the
**		GROUP BY select.  For UPDATEs and DELETEs, ensure the row to be
**		accessed is still present in the base table.
**	26-mar-97 (joea)
**		Temporarily use TBL_ENTRY instead of RS_TBLDESC (latter will be
**		used for generic RepServer).
**	28-aug-97 (joea)
**		Replace DQ_ROW by RS_TRANS_ROW.
**	01-oct-97 (joea) bug 83900, 85749, 85752
**		Partially undo the changes of 24-feb-97 for bug 79468.  For
**		UPDATEs and DELETEs, don't check for base table row existence
**		after checking if the old transaction is archived.
**	09-dec-97 (joea)
**		Replace d_insert by RSinsert, TBL_ENTRY by RS_TBLDESC. Add
**		RStarget argument.
**	05-mar-98 (joea) bug 89399
**		Reverse the sense of the tests in old_trans_archived.
**	21-apr-98 (joea)
**		Temporarily comment out body of RScollision while converting
**		remainder of RepServ to OpenAPI.
**	28-may-98 (joea)
**		Convert to use OpenAPI instead of ESQL (interim submission).
**	09-jun-98 (abbjo03)
**		Complete conversion to OpenAPI.
**	09-jul-98 (abbjo03)
**		Replace TRANS_KEY_INIT macro by TRANS_KEY_DESC_INIT macro in
**		sapiw.h.  Eliminate Dlm_Table_Name, Dlm_Table_Owner and
**		shadow_name static globals since they have been added to
**		RS_TBLDESC.  Eliminate shadow_name argument to
**		RScreate_key_clauses. Add new arguments to IIsw_selectSingleton.
**	17-jul-98 (abbjo03)
**		In delete_remote_record, adjust the DELETE shadow trans_time so
**		that resolve_missing_update will only retrieve one row. In
**		resolve_update_by_priority, add error checking after returning
**		from resolve_missing_update.
**	22-jul-98 (abbjo03)
**		Use rem_table_owner when accessing the remote table. Replace
**		dlm_col_name by col_name.
**	31-aug-98 (abbjo03) bug 79468
**		In resolve(), for UPDATE and DELETE if the previous transaction
**		hasn't been archived and we are dealing with a PRO target, check
**		if the base row is still there. In resolve_insert() and
**		delete_remote_record() add differentation between FP and PRO
**		targets. Add calc_time_diffs and calc_prio_diffs to consolidate
**		redundant code.
**	20-jan-99 (abbjo03)
**		Remove IIsw_close call after RSerror_check call. Add errParm and
**		rowCount to RSerror_check.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	09-apr-99 (abbjo03
**		In resolve_missing_delete, properly terminate trans_time before
**		using it in the second query. After returning from
**		resolve_missing_delete, if return is not OK, return immediately.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-08-01 (padni01) bug 103776 x-integ of change 451689 from 2.0
**		In resolve (), for updates, call RScreate_key_clauses with 
**		&row->old_rep_key instead of &row->rep_key. This will ensure 
**		that the where_clause and insert_values_clause will contain 
**		the row's primary key before the update.
**      16-jul-02 (padni01) bug 103776 Additional change; x-integ from 2.0
**              In resolve_missing_update, create where_clause for the
**              new updated key on source database.
**	11-dec-2002 (inifa01) x-int bug 109269 by padni01
**		In resolve_missing_update(), add new_key=0 condition to 
**		select old_sourcedb, old_transaction_id, old_sequence_no 
**		from shadow table.
**      06-Nov-2003 (inifa01) INGREP 108 bug 106498
**		Changed from allocating default MAX SIZE for temp buffers  
**		name_clause, stmt, where_clause, insert_value_clause, select_stmt,   
**		old_wc, wc and tmp which was insufficient for large tables to
**		calling RSmem_allocate() to allocate MAX possible size based on 
**		the replicated tables row_width and/or number of replicated 
**		columns as appropriate. 
**		Call RSmem_free() to free multiple temp buffers.
**	26-mar-2004 (gupsh01)
**		Added getQinfoParm as input parameter to II_sw functions.
**      09-Mar-2005 (inifa01) INGREP172 Bug 113823
**      	In a FP<->FP replication scheme with collision mode 
**		LastWriteWins, collision resolution fails with error 1590 if
**		the source wins and a record with the same key has in_archive=0
**		in the shadow table. delete_remote_record() expects to find
**		only the colliding record with in_archive == 0 in the shadow
**		table.
**      	Also discovered that delete_remote_record() enters the wrong
**		key value into the shadow table for the remote record being
**		deleted. FIX: in delete_remote_record() restrict determination
**		from remote shadow table of record being deleted to transaction
**		with the latest time stamp
**      	Also in resolve_missing_update() 'insert_value_clause' is now
**		passed in as an argument so that the new key
**		insert_value_clause can be created for the remote db.
**	16-May-2005 (inifa01) INGREP176 b114521
**		Some update collisions can remain unresolved if multiple operations on a 
**		key value In the shadow table have the same 'trans_time'. Errors reported
**		are 1392 followed by 1614.
**		FIX: Modify resolve_missing_update() to retrieve 'old_wc' based on 
**		row->old_rep_key instead of building 'old_key' from searching the shadow
**		table since the information is already available.  Also now use 
**		row->old_rep_key to retrieve the target 'new_key' instead of 'old_key'.
**	04-Aug-2005 (sheco02)
**		Fixed x-integration 478013 problem by adding getQinfoParm as 
**	        input parameter to IIsw_selectSingleton and RSerror_check calls..
**/

# define LONG_AGO	ERx("01-jan-1980")
# define MAX_SIZE	10000


static RS_TRANS_KEY	last_trans = { 0, -1, 0 };
static RS_TARGET	*target;
static RS_TRANS_ROW	*row;
static RS_CONN		*target_conn;
static RS_TBLDESC	*tbl;


static STATUS pre_clean(bool *collision_found, bool *collision_processed,
	bool process_collision);
static STATUS resolve(bool *collision_processed);
static STATUS resolve_insert_by_time(char *rem_trans_time, i2 rem_src_db,
	i4 *delta_time, i4  *delta_db);
static STATUS resolve_insert_by_priority(i2 rem_priority, i2 rem_src_db,
	i4 *delta_prio, i4  *delta_db);
static STATUS resolve_insert(i4 db_flag, i4  delta_db, char *where_clause,
	char *name_clause, char *insert_value_clause,
	bool *collision_processed);
static STATUS resolve_update_by_time(char *where_clause, char *name_clause,
	char *insert_value_clause, i4  *delta_time, i4  *delta_db,
	bool *prior_delete, bool *collision_processed);
static STATUS resolve_update_by_priority(char *where_clause, char *name_clause,
	char *insert_value_clause, i4  *delta_prio, i4  *delta_db,
	bool *prior_delete, bool *collision_processed);
static STATUS resolve_missing_update(char *where_clause, char* insert_value_clause,
	char *rem_trans_time,
	i2 *rem_priority, i2 *remote_source_db, bool *prior_delete,
	bool *collision_processed);
static STATUS resolve_update(i4 db_flag, i4  delta_db, char *where_clause,
	char *name_clause, char *insert_value_clause, bool prior_delete,
	bool *collision_processed);
static STATUS resolve_delete_by_time(char *where_clause, char *name_clause,
	char *insert_value_clause, i4  *delta_time, i4  *delta_db,
	bool *collision_processed);
static STATUS resolve_delete_by_priority(char *where_clause, char *name_clause,
	char *insert_value_clause, i4  *delta_prio, i4  *delta_db,
	bool *collision_processed);
static STATUS resolve_missing_delete(char *where_clause, char *rem_trans_time,
	i2 *rem_priority, i2 *remote_source_db, bool *collision_processed);
static STATUS resolve_delete(i4 db_flag, i4  delta_db, char *where_clause,
	char *name_clause, char *insert_value_clause,
	bool *collision_processed);
static STATUS delete_remote_record(char *where_clause, char *name_clause,
	char *insert_value_clause);
static STATUS get_local_trans_time(char *loc_trans_time, i2 *db_no);
static STATUS calc_time_diffs(char *rem_trans_time, i2 rem_src_db,
	i4 *delta_time, i4  *delta_db);
static STATUS calc_prio_diffs(i2 rem_priority, i2 rem_src_db, i4  *delta_time,
	i4 *delta_db);
static bool old_trans_archived(void);
static bool old_trans_new_key(void);
static bool base_row_exists(char *where_clause);


/*{
** Name:	RScollision - handle a collision
**
** Description:
**	Pre-cleans the queue or pre-cleans and resolves collisions
**	depending on the collision flag.
**
** Inputs:
**	targ			- target descriptor
**	table			- table descriptor
**	trans_row		- current transaction row
**
** Outputs:
**	collision_processed	- was a collision processed
**
** Returns:
**	OK	- no collisions detected
*/
STATUS
RScollision(
RS_TARGET	*targ,
RS_TRANS_ROW	*trans_row,
RS_TBLDESC	*table,
bool		*collision_processed)
{
	STATUS	err = OK;
	bool	collision_found = FALSE;
	/* currently, only set in pre_clean() and not maintained by resolve() */

	target = targ;
	target_conn = &RSconns[target->conn_no];
	row = trans_row;
	tbl = table;

	if (target->collision_mode != COLLMODE_PASSIVE &&
		target->type == TARG_UNPROT_READ)
	{
		/* cannot resolve collisions for target type 3 */
		messageit(4, 1559, target->db_no);
		return (OK);
	}

	switch (target->collision_mode)
	{
	case COLLMODE_PASSIVE:	/* detect simple coll.; no resolve */
		break;

	case COLLMODE_ACTIVE:	/* detect benign coll.; no resolve */
		err = pre_clean(&collision_found, collision_processed,
			FALSE);
		break;

	case COLLMODE_BENIGN:	/* detect benign coll.; resolve */
		err = pre_clean(&collision_found, collision_processed,
			TRUE);
		break;

	case COLLMODE_TIME:	/* resolve by last timestamp */
	case COLLMODE_PRIORITY:	/* resolve by priority lookup */
		err = pre_clean(&collision_found, collision_processed,
			TRUE);
		if (!err && !*collision_processed)
			err = resolve(collision_processed);
		break;

	default:
		messageit(1, 1788, target->collision_mode, tbl->table_owner,
			tbl->table_name, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		/*
		** Since neither err nor collision_processed are set, this case
		** is treated like COLLMODE_PASSIVE.
		*/
		break;
	}

	if (!err && collision_found && !*collision_processed)
		err = 1;	/* do not transmit, leave on queue */

	return (err);
}


/**
** Name:	pre_clean - pre-clean the distribution queue
**
** Description:
**	Pre-cleans the queue for the current command in a transaction.
**
** Inputs:
**	process_collision	- collision should be resolved
**
** Outputs:
**	collision_found		- a benign collision was found
**	collision_processed	- was a collision processed
**
** Returns:
**	OK	- no collisions detected
**/
static STATUS
pre_clean(
bool	*collision_found,
bool	*collision_processed,
bool	process_collision)
{
	i4			cnt;
	char			stmt[1024];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;

	switch (row->trans_type)
	{
	case RS_INSERT:
	case RS_UPDATE:
		STprintf(stmt, "SELECT COUNT(*) FROM %s WHERE sourcedb = ~V \
AND transaction_id = ~V AND sequence_no = ~V AND in_archive = 0",
			tbl->shadow_table);
		break;

	case RS_DELETE:
		STprintf(stmt, "SELECT COUNT(*) FROM %s WHERE sourcedb = ~V \
AND transaction_id = ~V AND sequence_no = ~V AND trans_type = %d",
			tbl->shadow_table, RS_DELETE);
		break;

	default:
		/* Invalid trans type %d for table '%s.%s' */
		messageit(1, 1379, row->trans_type, tbl->table_owner,
			tbl->table_name, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		return (1379);
	}

	TRANS_KEY_DESC_INIT(pdesc, pdata, row->rep_key);
	SW_COLDATA_INIT(cdata, cnt);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 3, pdesc, pdata, 1, &cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	/* Error checking for benign collision for table '%s.%s' */
	status = RSerror_check(1377, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm,
		&errParm, NULL, tbl->table_owner, tbl->table_name, 
		row->trans_type, row->rep_key.src_db, row->rep_key.trans_id,
		row->rep_key.seq_no);
	if (status != OK)
		return (status);

	if (cnt)
	{
		*collision_found = TRUE;

		if (process_collision)
		{
			/* Removing benign collision for table '%s.%s' */
			messageit(3, 1378, tbl->table_owner, tbl->table_name,
				row->trans_type, row->rep_key.src_db,
				row->rep_key.trans_id, row->rep_key.seq_no);
			*collision_processed = TRUE;
		}
		else
		{
			/* Benign collision detected for table '%s.%s' */
			messageit(3, 1381, tbl->table_owner, tbl->table_name,
				row->trans_type, row->rep_key.src_db,
				row->rep_key.trans_id, row->rep_key.seq_no);
		}
	}

	return (OK);
}


/**
** Name:	resolve - resolve collisions
**
** Description:
**	Detects & resolves collisions remaining after the queue has been
**	pre-cleaned.
**
** Inputs:
**	none
**
** Outputs:
**	collision_processed	- was a collision processed
**
** Returns:
**	OK	- no collisions detected
**/
static STATUS
resolve(
bool	*collision_processed)
{
	char		rem_trans_time[26];
	i2		rem_src_db;
	i2		rem_priority;
	II_LONG		row_count;
	char		*where_clause=NULL;
	char		*name_clause=NULL;
	char		*insert_value_clause=NULL;
	char		*stmt=NULL;
	STATUS		err;
	i4		db_flag;
	i4		delta_db;
	bool		prior_delete;
	IIAPI_DATAVALUE	cdata[3];
	II_PTR		stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS	status;
	i4		num_cols=tbl->num_regist_cols;
	i4		row_width=tbl->row_width;

	name_clause = RSmem_allocate(0,num_cols,DB_MAXNAME+2,0);
	if (name_clause == NULL)
	   return (FAIL);

	insert_value_clause = RSmem_allocate(row_width,num_cols,2,0); 
	if (insert_value_clause == NULL)
	{
		MEfree((PTR)name_clause);
		return (FAIL);
	}
	
	where_clause = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,0); 
	if (where_clause == NULL)
	{
		RSmem_free(name_clause,insert_value_clause,NULL,NULL);
		return (FAIL);
	}
	
	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256);
	if (stmt == NULL)
	{
		RSmem_free(name_clause,insert_value_clause,where_clause,NULL); 
		return (FAIL);
	}
	messageit(5, 1034, "resolve");
	switch (row->trans_type)
	{
	case RS_INSERT:
		err = RScreate_key_clauses(&RSlocal_conn, tbl, &row->rep_key,
			where_clause, name_clause, insert_value_clause, NULL);
		if (err)
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (err);
		}
		if (!base_row_exists(where_clause))
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (OK);
		}
		STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)), dd_priority, \
MIN(sourcedb) FROM %s t WHERE %s GROUP BY dd_priority"),
			tbl->shadow_table, where_clause);
		SW_COLDATA_INIT(cdata[0], rem_trans_time);
		SW_COLDATA_INIT(cdata[1], rem_priority);
		SW_COLDATA_INIT(cdata[2], rem_src_db);
		status = IIsw_selectSingleton(target_conn->connHandle,
			&target_conn->tranHandle, stmt, 0, NULL, NULL, 3, cdata,
			NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle,
			&getQinfoParm, &errParm, &row_count);
		if (status != OK)
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (status);
		}
		SW_CHA_TERM(rem_trans_time, cdata[0]);
		if (cdata[0].dv_null || !row_count)
			STcopy(LONG_AGO, rem_trans_time);
		if (cdata[1].dv_null || !row_count)
			rem_priority = -999;
		if (cdata[2].dv_null || !row_count)
			rem_src_db = -1;

		/* collision found */
		messageit(1, 1392, ERx("INSERT"), tbl->table_owner,
			tbl->table_name, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		switch (target->collision_mode)
		{
		case COLLMODE_TIME:
			err = resolve_insert_by_time(rem_trans_time,
				rem_src_db, &db_flag, &delta_db);
			break;

		case COLLMODE_PRIORITY:
			err = resolve_insert_by_priority(rem_priority,
				rem_src_db, &db_flag, &delta_db);
			break;

		default:
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt);
			return (OK);
		}

		if (!err)
			err = resolve_insert(db_flag, delta_db, where_clause,
				name_clause, insert_value_clause,
				collision_processed);
		break;

	case RS_UPDATE:
		err = RScreate_key_clauses(&RSlocal_conn, tbl, &row->old_rep_key,
			where_clause, name_clause, insert_value_clause, NULL);
		if (err)
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (err);
		}
		if (!old_trans_archived())
		{
			if (target->type == TARG_FULL_PEER ||
					(target->type == TARG_PROT_READ &&
					base_row_exists(where_clause)))
			{
				RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
				return (OK);
			}
		}

		/*
		** possible collision.  We need to check the new_key
		** flag in the source shadow table -- if it's set, then
		** we don't actually have a collision
		*/
		if (old_trans_new_key() && base_row_exists(where_clause))
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (OK);
		}

		/* collision found */
		messageit(1, 1392, ERx("UPDATE"), tbl->table_owner,
			tbl->table_name, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		switch (target->collision_mode)
		{
		case COLLMODE_TIME:
			err = resolve_update_by_time(where_clause, name_clause,
				insert_value_clause, &db_flag, &delta_db,
				&prior_delete, collision_processed);
			break;

		case COLLMODE_PRIORITY:
			err = resolve_update_by_priority(where_clause,
				name_clause, insert_value_clause, &db_flag,
				&delta_db, &prior_delete, collision_processed);
			break;

		default:
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (OK);
		}

		if (*collision_processed)
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt);
			return (err);
		}

		if (!err)
			err = resolve_update(db_flag, delta_db, where_clause,
				name_clause, insert_value_clause, prior_delete,
				collision_processed);
		break;

	case RS_DELETE:
		/*
		** we have a delete collision if the old replication key (db,
		** tran, seq) is in the shadow table with in archive = 1.  We
		** also have a collision if the base key is not in the base
		** table & the old rep key was not in the shadow table.  In the
		** first case, we compare transaction times or priorities &
		** resolve it accordingly.  In the latter, we just delete the
		** delete command off the queue.  The actual logic for this
		** should look a lot like the update logic.
		*/
		err = RScreate_key_clauses(&RSlocal_conn, tbl, &row->rep_key,
			where_clause, name_clause, insert_value_clause, NULL);
		if (err)
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (err);
		}
		if (!old_trans_archived())
		{
			if (target->type == TARG_FULL_PEER ||
					(target->type == TARG_PROT_READ &&
					base_row_exists(where_clause)))
			{
				RSmem_free(name_clause,insert_value_clause,where_clause,stmt);
				return (OK);
			}
		}

		/*
		** possible collision.  We need to check the new_key
		** flag in the source shadow table -- if it's set, then
		** we don't actually have a collision
		*/
		if (old_trans_new_key() && base_row_exists(where_clause))
		{
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt);
			return (OK);
		}

		/* collision found */
		messageit(1, 1392, ERx("DELETE"), tbl->table_owner,
			tbl->table_name, row->rep_key.src_db,
			row->rep_key.trans_id, row->rep_key.seq_no);
		switch (target->collision_mode)
		{
		case COLLMODE_TIME:
			err = resolve_delete_by_time(where_clause, name_clause,
				insert_value_clause, &db_flag, &delta_db,
				collision_processed);
			break;

		case COLLMODE_PRIORITY:
			err = resolve_delete_by_priority(where_clause,
				name_clause, insert_value_clause, &db_flag,
				&delta_db, collision_processed);
			break;

		default:
			RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
			return (OK);
		}

		if (!err)
			err = resolve_delete(db_flag, delta_db, where_clause,
				name_clause, insert_value_clause,
				collision_processed);
		break;
	}

	RSmem_free(name_clause,insert_value_clause,where_clause,stmt); 
	return (err);
}


/**
** Name:	resolve_insert_by_time
**
** Description:
**	Determines whether the local or remote database came second in an
**	insert collision.
**/
static STATUS
resolve_insert_by_time(
char	*rem_trans_time,
i2	rem_src_db,
i4	*delta_time,
i4	*delta_db)
{
	return (calc_time_diffs(rem_trans_time, rem_src_db, delta_time,
		delta_db));
}


/**
** Name:	resolve_insert_by_priority
**
** Description:
**	Determines whether the local or remote database has the higher
**	priority in an insert collision.
**/
static STATUS
resolve_insert_by_priority(
i2	rem_priority,
i2	rem_src_db,
i4	*delta_prio,
i4	*delta_db)
{
	return (calc_prio_diffs(rem_priority, rem_src_db, delta_prio,
		delta_db));
}


/**
** Name:	resolve_insert
**
** Description:
**	Resolves an insert collision.
**/
static STATUS
resolve_insert(
i4	db_flag,
i4	delta_db,
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
bool	*collision_processed)
{
	char			rem_trans_time[26];
	char			loc_trans_time[26];
	i2			loc_src_db;
	STATUS			err;
	char			stmt[256];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM      getQinfoParm;
	IIAPI_STATUS		status;

	if (db_flag == 0)
	{
		if (delta_db > 0)
			db_flag = 1;
		else
			db_flag = -1;
	}
	if (db_flag >= 0)	/* remote database wins */
	{
		*collision_processed = TRUE;
		return (OK);
	}

	err = get_local_trans_time(loc_trans_time, &loc_src_db);
	if (err)
		return (err);
	/* delete the record */
	err = delete_remote_record(where_clause, name_clause,
		insert_value_clause);
	if (err)
		return (err);

	if (target->type == TARG_FULL_PEER)
	{
		/* get new trans_time */
		STprintf(stmt, ERx("SELECT CHAR(DATE('%s') - '1 sec')"),
			loc_trans_time);
		SW_COLDATA_INIT(cdata, rem_trans_time);
		status = IIsw_selectSingleton(target_conn->connHandle,
			&target_conn->tranHandle, stmt, 0, NULL, NULL, 1,
			&cdata, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1389, ROWS_SINGLE_ROW, stmtHandle,
			&getQinfoParm, &errParm, NULL);
		if (status != OK)
			return (status);
		SW_CHA_TERM(rem_trans_time, cdata);

		/*
		** get sequence number:  use negative numbers to avoid
		** duplicating possible sequence numbers in the transaction.
		** If same trans as last time through, decrement sequence
		** number. If first collision in this transaction, use sequence
		** number -1. Since, logically, the delete comes before the
		** insert, this should work fine.
		*/
		if (last_trans.trans_id != row->rep_key.trans_id)
		{
			last_trans.seq_no = -1;
			last_trans.trans_id = row->rep_key.trans_id;
		}
		else
		{
			--last_trans.seq_no;
		}
		last_trans.src_db = row->rep_key.src_db;
		/* insert DELETE command to remote input queue*/
		status = RSiiq_InsertInputQueue(target_conn, tbl, RS_DELETE,
			&last_trans, &row->old_rep_key, target->cdds_no, (i2)0,
			rem_trans_time, TRUE);
		if (status != OK)
			return (status);
	}
	*collision_processed = FALSE;

	return (OK);
}


/**
** Name:	resolve_update_by_time
**
** Description:
**	Determines whether the local or remote database came second in an
**	update collision.
**/
static STATUS
resolve_update_by_time(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
i4	*delta_time,
i4	*delta_db,
bool	*prior_delete,
bool	*collision_processed)
{
	char		rem_trans_time[26];
	i2		rem_priority;
	i2		rem_src_db;
	STATUS		err;
	char		*stmt=NULL;
	IIAPI_DATAVALUE	cdata[2];
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM      getQinfoParm;
	IIAPI_STATUS	status;
	i4		row_width=tbl->row_width;
	i4		num_cols=tbl->num_regist_cols;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256);
	if (stmt == NULL)
	   return (FAIL);

	*prior_delete = FALSE;
	STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)), MIN(sourcedb) \
FROM %s t WHERE %s AND in_archive = 0"),
		tbl->shadow_table, where_clause);
	SW_COLDATA_INIT(cdata[0], rem_trans_time);
	SW_COLDATA_INIT(cdata[1], rem_src_db);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 2, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, NULL);
	if (status != OK)
	{
		MEfree((PTR)stmt);
		return (status);
	}
	SW_CHA_TERM(rem_trans_time, cdata[0]);
	if (cdata[0].dv_null)
		STcopy(LONG_AGO, rem_trans_time);
	if (cdata[1].dv_null)
		rem_src_db = -1;
	if (!base_row_exists(where_clause))
	{
		/* No row to update, make sure not deleted */
		if (cdata[0].dv_null)
		{
			err = resolve_missing_update(where_clause,insert_value_clause
				,rem_trans_time, &rem_priority, &rem_src_db,
				prior_delete, collision_processed);
			if (err)
			{
				MEfree((PTR)stmt);
				return (err);
			}
			if (*collision_processed)
			{
				MEfree((PTR)stmt);
				return (err);
			}
		}
	}
	else
	{
		if (cdata[0].dv_null)	/* still need remote trans time */
		{
			STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)), \
MIN(sourcedb) FROM %s t WHERE %s"),
				tbl->shadow_table, where_clause);
			SW_COLDATA_INIT(cdata[0], rem_trans_time);
			SW_COLDATA_INIT(cdata[1], rem_src_db);
			status = IIsw_selectSingleton(target_conn->connHandle,
				&target_conn->tranHandle, stmt, 0, NULL, NULL,
				2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
				&errParm);
			status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, 
				&getQinfoParm, &errParm, NULL);
			if (status != OK)
			{
				MEfree((PTR)stmt);
				return (status);
			}
			SW_CHA_TERM(rem_trans_time, cdata[0]);
			if (cdata[0].dv_null)
				STcopy(LONG_AGO, rem_trans_time);
			if (cdata[1].dv_null)
				rem_src_db = -1;
		}
	}
	MEfree((PTR)stmt);
	/* See which is later */
	return (calc_time_diffs(rem_trans_time, rem_src_db, delta_time,
		delta_db));
}


/**
** Name:	resolve_update_by_priority
**
** Description:
**	Determines whether the local or remote database has higher priority
**	in an update collision.
**/
static STATUS
resolve_update_by_priority(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
i4	*delta_prio,
i4	*delta_db,
bool	*prior_delete,
bool	*collision_processed)
{
	i2			rem_priority;
	i2			rem_src_db;
	char			rem_trans_time[26];
	II_LONG			row_count;
	STATUS			err;
	char			*stmt=NULL;
	IIAPI_DATAVALUE		cdata[3];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	i4			row_width=tbl->row_width;
	i4			num_cols=tbl->num_regist_cols;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256);
	if (stmt == NULL)
	   return (FAIL);

	*prior_delete = FALSE;
	/*
	** get trans time while we're here in case we need to do
	** resolve_missing_update
	*/
	STprintf(stmt, ERx("SELECT CHAR(trans_time), dd_priority, sourcedb \
FROM %s t WHERE %s AND in_archive = 0"),
		tbl->shadow_table, where_clause);
	SW_COLDATA_INIT(cdata[0], rem_trans_time);
	SW_COLDATA_INIT(cdata[1], rem_priority);
	SW_COLDATA_INIT(cdata[2], rem_src_db);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 3, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, &row_count);
	MEfree((PTR)stmt);
	if (status != OK)
		return (status);
	SW_CHA_TERM(rem_trans_time, cdata[0]);
	if (cdata[0].dv_null)
		STcopy(LONG_AGO, rem_trans_time);
	if (!row_count)
	{
		rem_priority = -999;
		rem_src_db = -1;
	}
	if (!base_row_exists(where_clause))
	{
		/* no record to update -- make sure not del'd */
		if (!row_count)
		{
			rem_priority = -999;
			rem_src_db = -1;
			err = resolve_missing_update(where_clause,insert_value_clause
				,rem_trans_time, &rem_priority, &rem_src_db,
				prior_delete, collision_processed);
			if (err)
				return (err);
			if (*collision_processed)
				return (err);
		}
	}
	return (calc_prio_diffs(rem_priority, rem_src_db, delta_prio,
		delta_db));
}


/*{
** Name:	resolve_missing_update - resolve missing update
**
*/
static STATUS
resolve_missing_update(
char	*where_clause,
char	*insert_value_clause,
char	*rem_trans_time,
i2	*rem_priority,
i2	*rem_src_db,
bool	*prior_delete,
bool	*collision_processed)
{
	RS_TRANS_KEY		old_key;
	RS_TRANS_KEY		new_key;
	i2			dd_priority;
	i4			cnt;
	i2			trans_type;
	STATUS			err;
	char			trans_time[26];
	char			*old_wc=NULL;
	char			*stmt=NULL;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata[5];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	i4			row_width=tbl->row_width;
	i4			num_cols=tbl->num_regist_cols;
	i4			seq_no, trans_id;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+10,256);
	if (stmt == NULL)
	   return (FAIL);

	old_wc = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,0);
	if (old_wc == NULL)
	{
		MEfree((PTR)stmt);
		return (FAIL);
	}
	err = RScreate_key_clauses(&RSlocal_conn, tbl, &row->old_rep_key, old_wc, 
		NULL, NULL, NULL);
	if (err)
	{
		RSmem_free(stmt,old_wc,NULL,NULL); 
		return (err);
	}
	STprintf(stmt, ERx("SELECT COUNT(*) FROM %s t WHERE %s"),
		tbl->shadow_table, old_wc);
	SW_COLDATA_INIT(cdata[0], cnt);
	status = IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL, 1, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1615, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL, tbl->shadow_table, old_wc);
	if (cnt)
	{
		STprintf(stmt,
			ERx("SELECT CHAR(MAX(trans_time)) FROM %s t WHERE %s"),
			tbl->shadow_table, old_wc);
		cdata[0].dv_length = 26;
		cdata[0].dv_value = rem_trans_time;
		status = IIsw_selectSingleton(target_conn->connHandle,
			&target_conn->tranHandle, stmt, 0, NULL, NULL, 1, cdata,
			NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1616, ROWS_SINGLE_ROW, stmtHandle, 
			&getQinfoParm, &errParm, NULL, tbl->shadow_table, old_wc);
		if (status != OK)
		{
			RSmem_free(stmt,old_wc,NULL,NULL); 
			return (status);
		}
		SW_CHA_TERM(rem_trans_time, cdata[0]);
		if (cdata[0].dv_null)
		{
			row->trans_type = RS_INSERT;
			err = RSinsert(target, row);
			*collision_processed = TRUE;
			RSmem_free(stmt,old_wc,NULL,NULL); 
			return (err);
		}
                STprintf(stmt,
                         ERx("SELECT MAX(transaction_id) FROM %s t WHERE %s \
                         AND trans_time = '%s' "),
                         tbl->shadow_table, old_wc, rem_trans_time);
                SW_COLDATA_INIT(cdata[0], trans_id);
                status = IIsw_selectSingleton(target_conn->connHandle,
                        &target_conn->tranHandle, stmt, 0, NULL, NULL, 1, cdata,
                        NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
                status = RSerror_check(1616, ROWS_SINGLE_ROW, stmtHandle,
                        &getQinfoParm, &errParm, NULL, tbl->shadow_table, old_wc);
                if (status != OK)
                {
                        RSmem_free(stmt,old_wc,NULL,NULL);
                        return (status);
                }
                STprintf(stmt,
                         ERx("SELECT MAX(sequence_no) FROM %s t WHERE %s \
                         AND trans_time = '%s' AND transaction_id = %d"),
                         tbl->shadow_table, old_wc, rem_trans_time, trans_id);
                SW_COLDATA_INIT(cdata[0], seq_no);
                status = IIsw_selectSingleton(target_conn->connHandle,
                        &target_conn->tranHandle, stmt, 0, NULL, NULL, 1, cdata,
                        NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
                status = RSerror_check(1616, ROWS_SINGLE_ROW, stmtHandle,
                        &getQinfoParm, &errParm, NULL, tbl->shadow_table, old_wc);
                if (status != OK)
                {
                        RSmem_free(stmt,old_wc,NULL,NULL);
                        return (status);
                }
		STprintf(stmt, ERx("SELECT trans_type, dd_priority, sourcedb, \
transaction_id, sequence_no FROM %s t WHERE trans_time = '%s' AND %s AND \
sequence_no = %d AND transaction_id = %d "),
		tbl->shadow_table, rem_trans_time, old_wc, seq_no, trans_id);
		SW_COLDATA_INIT(cdata[0], trans_type);
		SW_COLDATA_INIT(cdata[1], dd_priority);
		SW_COLDATA_INIT(cdata[2], new_key.src_db);
		SW_COLDATA_INIT(cdata[3], new_key.trans_id);
		SW_COLDATA_INIT(cdata[4], new_key.seq_no);
		status = IIsw_selectSingleton(target_conn->connHandle,
			&target_conn->tranHandle, stmt, 0, NULL, NULL, 5, cdata,
			NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1617, ROWS_SINGLE_ROW, stmtHandle,
			&getQinfoParm, &errParm, NULL, tbl->shadow_table, old_wc);
		if (status != OK)
		{
			RSmem_free(stmt,old_wc,NULL,NULL); 
			return (status);
		}
		if (trans_type == RS_DELETE)	/* delete */
		{
			*prior_delete = TRUE;
		}
		else
		{
			/* get new key where clause in remote db */

			STprintf(stmt, ERx("SELECT CHAR(trans_time), \
dd_priority, sourcedb, transaction_id, sequence_no FROM %s WHERE old_sourcedb \
= ~V AND old_transaction_id = ~V AND old_sequence_no = ~V"),
				tbl->shadow_table);
			TRANS_KEY_DESC_INIT(pdesc, pdata, row->old_rep_key);
			cdata[0].dv_length = 26;
			cdata[0].dv_value = rem_trans_time;
			SW_COLDATA_INIT(cdata[1], dd_priority);
			SW_COLDATA_INIT(cdata[2], new_key.src_db);
			SW_COLDATA_INIT(cdata[3], new_key.trans_id);
			SW_COLDATA_INIT(cdata[4], new_key.seq_no);
			status = IIsw_selectSingleton(target_conn->connHandle,
				&target_conn->tranHandle, stmt, 3, pdesc, pdata,
				5, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
				&errParm);
			status = RSerror_check(1618, ROWS_SINGLE_ROW,
				stmtHandle, &getQinfoParm, &errParm, NULL, 
				tbl->shadow_table, old_key.src_db, old_key.trans_id,
				old_key.seq_no);
			if (status != OK)
			{
				RSmem_free(stmt,old_wc,NULL,NULL); 
				return (status);
			}
			SW_CHA_TERM(rem_trans_time, cdata[0]);
		}
		*rem_priority = dd_priority;
		*rem_src_db = new_key.src_db;
		/*
		** Modified to also create new key insert_value_clause
		** for remote db.  b113823
		*/
		err = RScreate_key_clauses(target_conn, tbl, &new_key, 
			where_clause, NULL, insert_value_clause, NULL);
		if (err)
		{
			RSmem_free(stmt,old_wc,NULL,NULL); 
			return (err);
		}
	}
	else
	{
		row->trans_type = RS_INSERT;
		err = RSinsert(target, row);
		*collision_processed = TRUE;
		RSmem_free(stmt,old_wc,NULL,NULL); 
		return (err);
	}

	RSmem_free(stmt,old_wc,NULL,NULL);
	return (OK);
}


/**
** Name:	resolve_update
**
** Description:
**	resolves update collisions.
**/
static STATUS
resolve_update(
i4	db_flag,		/* < 0 -- local, >= 0 -- remote */
i4	delta_db,		/* loc_src_db - rem_src_db */
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
bool	prior_delete,
bool	*collision_processed)
{
	STATUS	err = OK;

	if (db_flag == 0)
	{
		if (delta_db > 0)
			db_flag = 1;
		else
			db_flag = -1;
	}

	if (db_flag < 0)	/* local database wins */
	{
		/* if necessary, delete the remote record */
		if (!prior_delete)
		{
			err = delete_remote_record(where_clause, name_clause,
				insert_value_clause);
			if (err)
				return (err);
		}
		/* insert the current record */
		row->trans_type = RS_INSERT;
		err = RSinsert(target, row);
	}
	*collision_processed = TRUE;

	return (err);
}


/**
** Name:	resolve_delete_by_time
**
** Description:
**	Determines whether the local or remote database came second in an
**	delete collision.
**/
static STATUS
resolve_delete_by_time(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
i4	*delta_time,
i4	*delta_db,
bool	*collision_processed)
{
	char		rem_trans_time[26];
	i2		rem_priority;
	i2		remote_source_db;
	i2		rem_src_db;
	STATUS		err;
	char		*stmt=NULL;
	IIAPI_DATAVALUE	cdata[2];
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS	status;
	i4		row_width=tbl->row_width;
	i4		num_cols=tbl->num_regist_cols;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256);
	if (stmt == NULL)
	   return (FAIL);

	STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)), MIN(sourcedb) \
FROM %s t WHERE %s AND in_archive = 0"),
		tbl->shadow_table, where_clause);
	SW_COLDATA_INIT(cdata[0], rem_trans_time);
	SW_COLDATA_INIT(cdata[1], rem_src_db);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 2, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		 &errParm, NULL);
	if (status != OK)
	{
		MEfree((PTR)stmt);
		return (status);
	}
	SW_CHA_TERM(rem_trans_time, cdata[0]);
	if (cdata[0].dv_null)
		STcopy(LONG_AGO, rem_trans_time);
	if (cdata[1].dv_null)
		rem_src_db = -1;
	if (!base_row_exists(where_clause))
	{
		/* No row to delete, make sure key not updated */
		if (cdata[0].dv_null)
		{
			err = resolve_missing_delete(where_clause,
				rem_trans_time, &rem_priority,
				&remote_source_db, collision_processed);
			if (*collision_processed || err != OK)
			{
				MEfree((PTR)stmt);
				return (err);
			}
			rem_src_db = remote_source_db;
		}
	}
	else
	{
		if (cdata[0].dv_null)	/* still need remote trans time */
		{
			STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)), \
MIN(sourcedb) FROM %s t WHERE %s"),
				tbl->shadow_table, where_clause);
			SW_COLDATA_INIT(cdata[0], rem_trans_time);
			SW_COLDATA_INIT(cdata[1], rem_src_db);
			status = IIsw_selectSingleton(target_conn->connHandle,
				&target_conn->tranHandle, stmt, 0, NULL, NULL,
				2, cdata, NULL, NULL, &stmtHandle, &getQinfoParm, 
				&errParm);
			status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle,
				&getQinfoParm, &errParm, NULL);
			if (status != OK)
			{
				MEfree((PTR)stmt);
				return (status);
			}
			SW_CHA_TERM(rem_trans_time, cdata[0]);
			if (cdata[0].dv_null)
				STcopy(LONG_AGO, rem_trans_time);
			if (cdata[1].dv_null)
				rem_src_db = -1;
		}
	}
	MEfree((PTR)stmt);
	/* See which is later */
	return (calc_time_diffs(rem_trans_time, rem_src_db, delta_time,
		delta_db));
}


/**
** Name:	resolve_delete_by_priority
**
** Description:
**	Determines whether the local or remote database has higher priority
**	in an delete collision.
**/
static STATUS
resolve_delete_by_priority(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
i4	*delta_prio,
i4	*delta_db,
bool	*collision_processed)
{
	i2			rem_priority = -999;
	i2			remote_source_db;
	i2			rem_src_db;
	char			rem_trans_time[26];
	II_LONG			row_count;
	STATUS			err;
	char			*stmt=NULL;
	IIAPI_DATAVALUE		cdata[2];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	i4			row_width=tbl->row_width;
	i4			num_cols=tbl->num_regist_cols;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256);
	if (stmt == NULL)
	   return (FAIL);

	/* get the remote priority and database number */
	STprintf(stmt, ERx("SELECT dd_priority, sourcedb FROM %s t WHERE %s \
AND in_archive = 0"),
		tbl->shadow_table, where_clause);
	SW_COLDATA_INIT(cdata[0], rem_priority);
	SW_COLDATA_INIT(cdata[1], rem_src_db);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 2, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, &row_count);
	MEfree((PTR)stmt);
	if (status != OK)
		return (status);
	if (!row_count)
	{
		rem_priority = -999;
		rem_src_db = -1;
	}
	if (!base_row_exists(where_clause))
	{
		/* no record to delete -- make sure key not updated */
		if (!row_count)
		{
			err = resolve_missing_delete(where_clause,
				rem_trans_time, &rem_priority,
				&remote_source_db, collision_processed);
			if (*collision_processed || err != OK)
				return (err);
			rem_src_db = remote_source_db;
		}
	}
	return (calc_prio_diffs(rem_priority, rem_src_db, delta_prio,
		delta_db));
}


/*{
** Name:	resolve_missing_delete - resolve missing delete
**
** Description:
**	If a collision is detected on a delete and the row to be deleted on the
**	target is missing, either 1) the collision is with another delete or 2)
**	the collision is with a primary key update.  This function determines
**	which is the case and either resolves the collision by cleaning the
**	distribution queue or seeks out the new key value and returns its
**	transaction time and priority for resolution in resolve_delete().
**
** FIXME:
**	This apparently has never had any error checking since it was written.
*/
static STATUS
resolve_missing_delete(
char	*where_clause,
char	*rem_trans_time,
i2	*rem_priority,
i2	*remote_source_db,
bool	*collision_processed)
{
	RS_TRANS_KEY		old_key;
	RS_TRANS_KEY		new_key;
	char			trans_time[26];
	i2			dd_priority;
	i2			rem_src;
	i2			trans_type;
	STATUS			err = OK;
	char			*stmt=NULL;
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata[3];
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;
	i4			row_width=tbl->row_width;
	i4			num_cols=tbl->num_regist_cols;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256); 
	if (stmt == NULL)
	   return (FAIL);
	/*
	** At this point, we don't know whether the key has been deleted or
	** updated.
	** The following two queries will get a handle on the last operation
	** performed on the ORIGINAL key. First, we get the latest trans_time
	** based on the original where_clause.
	*/
	STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)) FROM %s t WHERE %s "),
		tbl->shadow_table, where_clause);
	SW_COLDATA_INIT(cdata[0], trans_time);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 1, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	IIsw_close(stmtHandle, &errParm);
	if (cdata[0].dv_null)
	{
		*collision_processed = TRUE;
		MEfree((PTR)stmt);
		return (OK);
	}
	SW_CHA_TERM(trans_time, cdata[0]);

	/* Then we use the trans_time to extract the old replication key */
	STprintf(stmt, ERx("SELECT sourcedb, transaction_id, sequence_no FROM \
%s t WHERE %s AND trans_time = '%s'"),
		tbl->shadow_table, where_clause, trans_time);
	SW_COLDATA_INIT(cdata[0], old_key.src_db);
	SW_COLDATA_INIT(cdata[1], old_key.trans_id);
	SW_COLDATA_INIT(cdata[2], old_key.seq_no);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 3, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1614, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL, tbl->table_owner, tbl->table_name, where_clause);
	if (status != OK)
	{
		MEfree((PTR)stmt);
		return (status);
	}

	/* get the new replication key */
	STprintf(stmt,
		ERx("SELECT sourcedb, transaction_id, sequence_no FROM %s \
WHERE old_sourcedb = ~V AND old_transaction_id = ~V AND old_sequence_no = ~V"),
		tbl->shadow_table);
	TRANS_KEY_DESC_INIT(pdesc, pdata, old_key);
	SW_COLDATA_INIT(cdata[0], new_key.src_db);
	SW_COLDATA_INIT(cdata[1], new_key.trans_id);
	SW_COLDATA_INIT(cdata[2], new_key.seq_no);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 3, pdesc, pdata, 3, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1383, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, NULL);
	if (status != OK)
	{
		MEfree((PTR)stmt);
		return (status);
	}

	/*
	** RScreate_key_clauses will build a new key string with the new values
	** of the key columns.  This lets us get the latest operation on the
	** NEW primary key
	*/
	err = RScreate_key_clauses(target_conn, tbl, &new_key, where_clause, 
		NULL, NULL, NULL);
	if (err)
	{
		MEfree((PTR)stmt);
		return (err);
	}
	/* This will give us the time_stamp we need */
	STprintf(stmt, ERx("SELECT CHAR(MAX(trans_time)) FROM %s t WHERE %s"),
		tbl->shadow_table, where_clause);
	cdata[0].dv_length = 26;
	cdata[0].dv_value = rem_trans_time;
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 1, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	IIsw_close(stmtHandle, &errParm);
	if (cdata[0].dv_null)
		STcopy(LONG_AGO, rem_trans_time);
	SW_CHA_TERM(rem_trans_time, cdata[0]);

	/* Now we get the trans_type of that latest transaction. */
	STprintf(stmt, ERx("SELECT trans_type, sourcedb, dd_priority FROM %s t \
WHERE trans_time = '%s' AND %s"),
		tbl->shadow_table, rem_trans_time, where_clause);
	SW_COLDATA_INIT(cdata[0], trans_type);
	SW_COLDATA_INIT(cdata[1], rem_src);
	SW_COLDATA_INIT(cdata[2], dd_priority);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 3, cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	IIsw_close(stmtHandle, &errParm);

	MEfree((PTR)stmt);
	/* If IT is a delete, then we toss our delete */
	if (trans_type == RS_DELETE)
	{
		*collision_processed = TRUE;
		return (OK);
	}
	*rem_priority = dd_priority;
	*remote_source_db = rem_src;

	/*
	** else we pass back the db_flag and delta_db and let resolve_delete()
	** decide what to do.  But we need to pass the right things to
	** delete_remote_record().  Since where_clause is a char *, the
	** where_clause passed to delete_remote_record will contain the new key
	** values.
	*/

	return (OK);
}


/**
** Name:	resolve_delete
**
** Description:
**	resolves delete collisions.
**/
static STATUS
resolve_delete(
i4	db_flag,		/* < 0 -- local, >= 0 -- remote */
i4	delta_db,		/* loc_src_db - rem_src_db */
char	*where_clause,
char	*name_clause,
char	*insert_value_clause,
bool	*collision_processed)
{
	STATUS	err = OK;

	if (db_flag == 0)
	{
		if (delta_db > 0)
			db_flag = 1;
		else
			db_flag = -1;
	}

	if (db_flag < 0)	/* local database wins */
	{
		/* delete the remote record */
		err = delete_remote_record(where_clause, name_clause,
			insert_value_clause);
		if (err)
			return (err);

	}
	*collision_processed = TRUE;

	return (err);
}


/*{
** Name:	delete_remote_record - delete remote record
*/
static STATUS
delete_remote_record(
char	*where_clause,
char	*name_clause,
char	*insert_value_clause)
{
	char		*select_stmt=NULL;
	char		*stmt1=NULL;
	char		*stmt2=NULL;
	char		*wc=NULL;
	char		*tmp=NULL;
	RS_COLDESC	*col;
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS	status;
	i4		row_width=tbl->row_width;
	i4		num_cols=tbl->num_regist_cols;
	char		trans_time[26];
	i2		src_db;
	IIAPI_DATAVALUE  cdata[2];

	wc = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,2302); 
	if (wc == NULL)
	   return (FAIL);

	select_stmt = RSmem_allocate(0,num_cols,DB_MAXNAME+6,512); 
	if (select_stmt == NULL)
	{
		MEfree((PTR)wc);
		return (FAIL);
	}

	stmt1 = RSmem_allocate(row_width,num_cols,3*DB_MAXNAME + 22,3070);
	if (stmt1 == NULL)
	{
		RSmem_free(wc,select_stmt,NULL,NULL); 
		return (FAIL);
	}
	stmt2 = RSmem_allocate(row_width,num_cols,3*DB_MAXNAME + 22,3070);
        if (stmt2 == NULL)
        {
                RSmem_free(wc,select_stmt,stmt1,NULL);
                return (FAIL);
        }
	tmp = RSmem_allocate(row_width,num_cols*2,1,1024);
	if (tmp == NULL)
	{
		RSmem_free(wc,select_stmt,stmt1,stmt2); 
		return (FAIL);
	}
	/*
	** The Change Recorder won't become active (since we're RepServer), so
	** we need to adjust the shadow & archive tables.
	*/
	if (target->type == TARG_FULL_PEER)	/* has archive table */
	{
		/* build insert statement & select clauses. Need
		** to select MAX(trans_time) for row being inserted
		** into archive table in case more than one with 
		** in_archve == 0 exists.
		*/
 
		*trans_time = EOS;	
                STcopy(ERx("SELECT CHAR(MAX(s.trans_time)), MIN(s.sourcedb)\
"),stmt1);
		STprintf(stmt2, ERx("INSERT INTO %s (sourcedb, transaction_id, \
sequence_no"),
			tbl->archive_table);
		STcopy(ERx("SELECT s.sourcedb, s.transaction_id, \
s.sequence_no"), 
			select_stmt);
		STprintf(wc, ERx(" FROM %s.%s t, %s s WHERE %s "),
			tbl->rem_table_owner, tbl->dlm_table_name,
			tbl->shadow_table, where_clause);
		for (col = tbl->cols; col < tbl->cols + tbl->num_regist_cols;
			++col)
		{
			STpolycat(3, stmt2, ERx(", "), col->col_name, stmt2);
			STpolycat(3, select_stmt, ERx(", t."), col->col_name,
				select_stmt);
			if (col->key_sequence > 0)
			{
				STprintf(tmp, ERx(" AND t.%s = s.%s"),
					col->col_name, col->col_name);
				STcat(wc, tmp);
			}
		}
		STpolycat(3, stmt1, wc, ERx(" AND s.in_archive = 0 AND trans_type != 3"), stmt1);

                SW_COLDATA_INIT(cdata[0], trans_time);
                SW_COLDATA_INIT(cdata[1], src_db);
                status = IIsw_selectSingleton(target_conn->connHandle,
                        &target_conn->tranHandle, stmt1, 0, NULL, NULL, 2,
			cdata,NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1590, ROWS_ZERO_OR_ONE, stmtHandle,
        		 &getQinfoParm, &errParm, NULL);
                if (status != OK)
                {
                        RSmem_free(wc,select_stmt,stmt1,tmp);
                        MEfree((PTR)stmt2);
                        return (status);

                }
                SW_CHA_TERM(trans_time,cdata[0]);
		STpolycat(5, stmt2, ERx(") "), select_stmt, wc,
			ERx(" AND s.in_archive = 0 AND trans_type != 3"), stmt2);
		if (*trans_time != EOS) 
		{
		    STprintf(stmt1, ERx(" AND CHAR(s.trans_time) = '%s' AND \
		    s.sourcedb= %d "),trans_time, src_db);
		    STcat(stmt2, stmt1);
		}
 
		status = IIsw_query(target_conn->connHandle,
			&target_conn->tranHandle, stmt2, 0, NULL, NULL, NULL,
			NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1590, ROWS_ZERO_OR_ONE, stmtHandle,
			&getQinfoParm, &errParm, NULL);
		if (status != OK)
		{
			RSmem_free(wc,select_stmt,stmt1,tmp);
			MEfree((PTR)stmt2);
			return (status);

		}
		STprintf(stmt1, ERx("UPDATE %s t SET in_archive = 1 WHERE \
in_archive = 0 AND trans_type != 3 AND CHAR(trans_time) = '%s'  AND sourcedb= %d AND %s ") ,
 tbl->shadow_table, trans_time,src_db, where_clause);
		status = IIsw_query(target_conn->connHandle,
			&target_conn->tranHandle, stmt1, 0, NULL, NULL, NULL,
			NULL, &stmtHandle, &getQinfoParm, &errParm);
		status = RSerror_check(1588, ROWS_DONT_CARE, stmtHandle,
			&getQinfoParm, &errParm, NULL, tbl->table_owner, tbl->table_name);
		if (status != OK)
		{
			RSmem_free(wc,select_stmt,stmt1,tmp); 
			MEfree((PTR)stmt2);
			return (status);
		}
	}

	STprintf(stmt1, ERx("INSERT INTO %s (%s, sourcedb, transaction_id, \
sequence_no, trans_time, distribution_time, in_archive, cdds_no, trans_type, \
dd_priority, new_key, old_sourcedb, old_transaction_id, old_sequence_no) "),
		tbl->shadow_table, name_clause);
	/* Subtract 1 from the sequence_no to avoid duplicates */
	STprintf(tmp, ERx("VALUES (%s, %d, %d, %d, '%s' - DATE('1 sec'), \
DATE('now'), 0, %d, %d, %d, 0, %d, %d, %d)"),
		insert_value_clause, row->rep_key.src_db,
		row->rep_key.trans_id, row->rep_key.seq_no - 1,
		row->trans_time, target->cdds_no, RS_DELETE, row->priority,
		row->old_rep_key.src_db, row->old_rep_key.trans_id,
		row->old_rep_key.seq_no);
	STcat(stmt1, tmp);
	status = IIsw_query(target_conn->connHandle, &target_conn->tranHandle,
		stmt1, 0, NULL, NULL, NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1588, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL, tbl->table_owner, tbl->table_name);
	if (status != OK)
	{
		RSmem_free(wc,select_stmt,stmt1,tmp); 
		MEfree((PTR)stmt2);
		return (status);
	}

	row->old_rep_key.src_db = row->rep_key.src_db;
	row->old_rep_key.trans_id = row->rep_key.trans_id;
	row->old_rep_key.seq_no = row->rep_key.seq_no - 1;

	STprintf(stmt1, ERx("DELETE FROM %s.%s t WHERE %s"),
		tbl->rem_table_owner, tbl->dlm_table_name, where_clause);
	status = IIsw_query(target_conn->connHandle, &target_conn->tranHandle,
		stmt1, 0, NULL, NULL, NULL, NULL, &stmtHandle, &getQinfoParm, 
		&errParm);
	status = RSerror_check(1386, ROWS_ZERO_OR_ONE, stmtHandle, &getQinfoParm,
		&errParm, NULL, tbl->table_owner, tbl->table_name);
	
	RSmem_free(wc,select_stmt,stmt1,tmp); 
	MEfree((PTR)stmt2);
	return (status);
}


/**
** Name:	get_local_trans_time
**
** Description:
**	Get the transaction time from the local database.
**/
static STATUS
get_local_trans_time(
char	*loc_trans_time,
i2	*db_no)
{
	i2			tmp_db_no;
	II_LONG			row_count;
	char			stmt[1024];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;

	STprintf(stmt, ERx("SELECT CHAR(trans_time) FROM %s WHERE sourcedb = \
~V AND transaction_id = ~V AND sequence_no = ~V"),
		tbl->shadow_table);
	TRANS_KEY_DESC_INIT(pdesc, pdata, row->rep_key);
	cdata.dv_length = 26;
	cdata.dv_value = loc_trans_time;
	status = IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 3, pdesc, pdata, 1, &cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1384, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, &row_count);
	SW_CHA_TERM(loc_trans_time, cdata);
	if (cdata.dv_null)
		STcopy(LONG_AGO, loc_trans_time);
	tmp_db_no = row->rep_key.src_db;
	if (!row_count)
	{
		STcopy(LONG_AGO, loc_trans_time);
		tmp_db_no = -1;
	}
	*db_no = tmp_db_no;

	return (status);
}


/**
** Name:	calc_time_diffs	- calculate time and database differences
**
** Description:
**	Calculate the difference between the local and remote transaction times
**	and between the local and remote source database numbers.
**
** Input:
**	rem_trans_time	- remote transaction time
**	rem_src_db	- remote source database number
**
** Output:
**	delta_time	- result of rem_trans_time minus loc_trans_time
**	delta_db	- result of rem_src_db minus loc_src_db
**
** Returns:
**	OK or error status from get_local_trans_time or RSerror_check
**/
static STATUS
calc_time_diffs(
char	*rem_trans_time,
i2	rem_src_db,
i4	*delta_time,
i4	*delta_db)
{
	i4		delta_seconds;
	char		loc_trans_time[26];
	i2		loc_src_db;
	char		stmt[256];
	STATUS		err;
	IIAPI_DATAVALUE	cdata;
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS	status;

	err = get_local_trans_time(loc_trans_time, &loc_src_db);
	if (err)
		return (err);
	STprintf(stmt, ERx("SELECT INT4(INTERVAL('seconds', DATE('%s') - \
DATE('%s')))"),
		rem_trans_time, loc_trans_time);
	SW_COLDATA_INIT(cdata, delta_seconds);
	status = IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL, 1, &cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1385, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, 
		&errParm, NULL);
	if (status != OK)
		return (status);
	*delta_time = delta_seconds;
	/*
	** if rem_src_db was not set, then force loc_src_db to be the lowest
	** number
	*/
	if (rem_src_db == -1)
		*delta_db = -1;
	else
		*delta_db = loc_src_db - rem_src_db;
	return (OK);
}


/**
** Name:	calc_prio_diffs	- calculate priority differences
**
** Description:
**	Retrieve the local priority and source database from the shadow table.
**
** Input:
**	rem_priority	- remote priority
**	rem_src_db	- remote source database number
**
** Output:
**	delta_prio	- result of rem_priority minus loc_priority
**	delta_db	- result of rem_src_db minus loc_src_db
**
** Returns:
**	OK or error status from RSerror_check
**/
static STATUS
calc_prio_diffs(
i2	rem_priority,
i2	rem_src_db,
i4	*delta_prio,
i4	*delta_db)
{
	i2			loc_priority;
	i2			loc_src_db;
	II_LONG			row_count;
	char			stmt[256];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;

	STprintf(stmt, ERx("SELECT dd_priority FROM %s WHERE sourcedb = ~V AND \
transaction_id = ~V AND sequence_no = ~V"),
		tbl->shadow_table);
	TRANS_KEY_DESC_INIT(pdesc, pdata, row->rep_key);
	SW_COLDATA_INIT(cdata, loc_priority);
	status = IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 3, pdesc, pdata, 1, &cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1384, ROWS_DONT_CARE, stmtHandle, &getQinfoParm, &errParm,
		&row_count);
	if (status != OK)
		return (status);
	loc_src_db = row->rep_key.src_db;
	if (!row_count)
	{
		loc_priority = -999;
		loc_src_db = -1;
	}
	*delta_prio = rem_priority - loc_priority;
	/*
	** if rem_src_db was not set, then force loc_src_db to be the lowest
	** number
	*/
	if (rem_src_db == -1)
		*delta_db = -1;
	else
		*delta_db = loc_src_db - rem_src_db;
	return (OK);
}


/*{
** Name:	old_trans_archived - old transaction archived?
**
** Description:
**	Determines if the previous transaction has been archived by querying
**	the target shadow table.
**
** Input:
**	none
**
** Output:
**	none
**
** Returns:
**	TRUE if old transaction has been archived, FALSE otherwise
*/
static bool
old_trans_archived()
{
	i4			cnt;
	char			stmt[256];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;

	STprintf(stmt, ERx("SELECT COUNT(*) FROM %s WHERE sourcedb = ~V AND \
transaction_id = ~V AND sequence_no = ~V AND in_archive = 0"),
		tbl->shadow_table);
	TRANS_KEY_DESC_INIT(pdesc, pdata, row->old_rep_key);
	SW_COLDATA_INIT(cdata, cnt);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 3, pdesc, pdata, 1, &cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1391, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm,
		 &errParm, NULL, tbl->table_owner, tbl->table_name, 
		row->rep_key.src_db, row->rep_key.trans_id, row->rep_key.seq_no);
	if (status != OK)
		return (status);
	return (cnt ? FALSE : TRUE);
}


/*{
** Name:	old_trans_new_key - does old transaction have new key?
**
** Description:
**	Determines if the previous transaction has the new_key column set.
**
** Input:
**	none
**
** Output:
**	none
**
** Returns:
**	TRUE if old transaction has new_key = 1, FALSE otherwise
*/
static bool
old_trans_new_key()
{
	i4			cnt;
	char			stmt[256];
	IIAPI_DESCRIPTOR	pdesc[3];
	IIAPI_DATAVALUE		pdata[3];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS		status;

	STprintf(stmt, ERx("SELECT COUNT(*) FROM %s WHERE sourcedb = ~V AND \
transaction_id = ~V AND sequence_no = ~V AND new_key = 1"),
		tbl->shadow_table);
	TRANS_KEY_DESC_INIT(pdesc, pdata, row->old_rep_key);
	SW_COLDATA_INIT(cdata, cnt);
	status = IIsw_selectSingleton(RSlocal_conn.connHandle,
		&RSlocal_conn.tranHandle, stmt, 3, pdesc, pdata, 1, &cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm);
	status = RSerror_check(1391, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL, tbl->table_owner, tbl->table_name, 
		row->rep_key.src_db, row->rep_key.trans_id, row->rep_key.seq_no);
	if (status != OK)
		return (status);
	return ((cnt == 0) ? FALSE : TRUE);
}


/*{
** Name:	base_row_exists - does base row exist?
**
** Description:
**	Determines if a row to be inserted/updated/deleted exists in the base
**	table in the target database.
**
** Input:
**	where_clause	- where clause identifying primary key
**
** Output:
**	none
**
** Returns:
**	TRUE if base row exists, FALSE otherwise
*/
static bool
base_row_exists(
char	*where_clause)
{
	i4		cnt;
	char		*stmt=NULL;
	IIAPI_DATAVALUE	cdata;
	II_PTR		stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_STATUS	status;
	i4		num_cols=tbl->num_regist_cols;
	i4		row_width=tbl->row_width;

	stmt = RSmem_allocate(row_width,num_cols,DB_MAXNAME+11,256); 
	if (stmt == NULL)
		return (FAIL);
	STprintf(stmt, "SELECT COUNT(*) FROM %s.%s t WHERE %s",
		tbl->rem_table_owner, tbl->dlm_table_name, where_clause);
	SW_COLDATA_INIT(cdata, cnt);
	status = IIsw_selectSingleton(target_conn->connHandle,
		&target_conn->tranHandle, stmt, 0, NULL, NULL, 1, &cdata, NULL,
		NULL, &stmtHandle, &getQinfoParm, &errParm);
	/* Error checking for benign collision for table '%s.%s' */
	status = RSerror_check(1383, ROWS_SINGLE_ROW, stmtHandle, &getQinfoParm, 
		&errParm, NULL);
	MEfree((PTR)stmt);
	if (status != OK)
		return (status);
	return ((cnt == 0) ? FALSE : TRUE);
}

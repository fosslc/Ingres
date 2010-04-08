/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <me.h>
# include <tm.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include "rdfint.h"
# include "conn.h"
# include "tblinfo.h"
# include "rsstats.h"

/**
** Name:	rsstats.c - RepServer statistics
**
** Description:
**	Defines
**		RSstats_init	- initialize monitor statistics
**		RSstats_update	- update statistics
**		RSstats_report	- report statistics
**		RSstats_terminate	- terminate statistics
**
** History:
**	04-nov-98 (abbjo03)
**		Created.
**	03-dec-98 (abbjo03)
**		If MEget_pages fails because the shared memory already exists,
**		call MEsmdestroy and try again.
**	15-dec-98 (abbjo03)
**		Implement PM parameter to control shared memory, detailed
**		statistics.
**	15-jan-99 (abbjo03)
**		Define TARG_BASE based on FIXED_CONNS.
**	18-feb-99 (abbjo03)
**		Add process ID, database, vnode, owner and table names.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

# define TARG_BASE	FIXED_CONNS


static RS_MONITOR	*mon_stats;
static bool		shared_stats = FALSE;
static u_i4	targ_size;
static u_i4	tbl_size;
static i4		num_pages;
static char		sm_key[13];


/*{
** Name:	RSstats_init - initialize monitor statistics
**
** Description:
**	Initializes the Replicator Server statistics memory segment.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK	Function completed normally.
*/
STATUS
RSstats_init()
{
	STATUS		status;
	char		*val;
	i4		num_targs;
	SIZE_TYPE	pages_alloc;
	u_i4	stats_size;
	RS_TARGET_STATS	*targ;
	RS_TARGET_STATS	*targ_end;
	RS_TABLE_STATS	*tbl;
	RS_CONN		*conn;
	RS_TBLDESC	*tbl_info;
	CL_SYS_ERR	sys_err;
	char		server_num[4];

	PMsetDefault(1, PMhost());
	STprintf(server_num, ERx("%d"), (i4)RSserver_no);
	PMsetDefault(3, server_num);
	status = PMget(ERx("II.$.REPSERV.$.SHARED_STATISTICS"), &val);
	if (status == OK && STbcompare(val, 0, ERx("ON"), 0, TRUE) == 0)
		shared_stats = TRUE;
	num_targs = RSnum_conns - TARG_BASE;
	stats_size = sizeof(RS_MONITOR) +
		sizeof(RS_MONITOR) % sizeof(ALIGN_RESTRICT);
	targ_size = sizeof(RS_TARGET_STATS) +
		sizeof(RS_TARGET_STATS) % sizeof(ALIGN_RESTRICT);
	if (shared_stats)
		tbl_size = sizeof(RS_TABLE_STATS) + sizeof(RS_TABLE_STATS) %
			sizeof(ALIGN_RESTRICT);
	else
		tbl_size = 0;
	num_pages = (stats_size + num_targs * (targ_size + RSrdf_svcb.num_tbls *
		tbl_size)) / ME_MPAGESIZE + 1;
	if (shared_stats)
	{
		STprintf(sm_key, ERx("%s.%03d"), RS_STATS_FILE, RSserver_no);
		status = MEget_pages(ME_SSHARED_MASK | ME_CREATE_MASK |
			ME_MZERO_MASK | ME_NOTPERM_MASK, num_pages, sm_key,
			(PTR *)&mon_stats, &pages_alloc, &sys_err);
		if (status == ME_ALREADY_EXISTS)
		{
			status = MEsmdestroy(sm_key, &sys_err);
			if (status == OK)
				status = MEget_pages(ME_SSHARED_MASK |
					ME_CREATE_MASK | ME_MZERO_MASK |
					ME_NOTPERM_MASK, num_pages, sm_key,
					(PTR *)&mon_stats, &pages_alloc,
					&sys_err);
		}
	}
	else
	{
		mon_stats = (RS_MONITOR *)MEreqmem(0, num_pages * ME_MPAGESIZE,
			TRUE, &status);
	}
	if (status != OK)
		return (status);
	mon_stats->server_no = RSserver_no;
	mon_stats->local_db_no = RSlocal_conn.db_no;
	PCpid(&mon_stats->pid);
	mon_stats->startup_time = TMsecs();
	mon_stats->num_targets = num_targs;
	mon_stats->num_tables = RSrdf_svcb.num_tbls;
	STcopy(RSlocal_conn.vnode_name, mon_stats->vnode_name);
	STcopy(RSlocal_conn.db_name, mon_stats->db_name);
	mon_stats->target_stats = (RS_TARGET_STATS *)((PTR)mon_stats +
		stats_size);
	targ_end = (RS_TARGET_STATS *)((PTR)mon_stats->target_stats +
		mon_stats->num_targets * (targ_size + tbl_size *
		mon_stats->num_tables));
	for (conn = &RSconns[TARG_BASE], targ = mon_stats->target_stats;
		targ < targ_end; ++conn, targ = (RS_TARGET_STATS *)((PTR)targ +
		targ_size + tbl_size * mon_stats->num_tables))
	{
		targ->db_no = conn->db_no;
		STcopy(conn->vnode_name, targ->vnode_name);
		STcopy(conn->db_name, targ->db_name);
		if (shared_stats)
		{
			targ->table_stats = (RS_TABLE_STATS *)((PTR)targ +
				targ_size);
			for (tbl_info = RSrdf_svcb.tbl_info,
				tbl = targ->table_stats;
				tbl < targ->table_stats + mon_stats->num_tables;
				++tbl, ++tbl_info)
			{
				tbl->table_no = tbl_info->table_no;
				STcopy(tbl_info->table_owner, tbl->table_owner);
				STcopy(tbl_info->table_name, tbl->table_name);
			}
		}
	}

	return (OK);
}


/*{
** Name:	RSstats_update - update statistics
**
** Description:
**	Updates the Replicator Server statistics.
**
** Inputs:
**	target_db	- target database number
**	table_no	- table number
**	txn_type	- transaction type
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
RSstats_update(
i2	target_db,
i4	table_no,
i4	txn_type)
{
	RS_TARGET_STATS	*targ;
	RS_TABLE_STATS	*tbl;
	RS_TARGET_STATS	*targ_end;

	if (!mon_stats)
		return;
	targ_end = (RS_TARGET_STATS *)((PTR)mon_stats->target_stats +
		mon_stats->num_targets * (targ_size + tbl_size *
		mon_stats->num_tables));
	for (targ = mon_stats->target_stats; targ < targ_end; targ =
		(RS_TARGET_STATS *)((PTR)targ + targ_size + tbl_size *
		mon_stats->num_tables))
	{
		if (target_db == targ->db_no)
		{
			if (table_no == 0)
			{
				if (!targ->first_txn_time)
					targ->first_txn_time = TMsecs();
				++targ->num_txns;
				targ->last_txn_time = TMsecs();
				break;
			}
			if (!shared_stats)
				break;
			for (tbl = targ->table_stats; tbl < targ->table_stats +
				mon_stats->num_tables; ++tbl)
			{
				if (table_no == tbl->table_no)
				{
					++tbl->num_rows[txn_type - 1];
					break;
				}
			}
		}
	}
}


/*{
** Name:	RSstats_report - report statistics
**
** Description:
**	Reports Replicator Server statistics for a target database.
**
** Inputs:
**	target_db	- target database number
**
** Outputs:
**	num_txns	- number of transactions
**	conn_mins	- connection time in minutes
**
** Returns:
**	none
*/
void
RSstats_report(
i2	target_db,
i4	*num_txns,
i4	*conn_mins)
{
	RS_TARGET_STATS	*targ;
	RS_TARGET_STATS	*targ_end;

	if (!mon_stats)
		return;
	*num_txns = *conn_mins = 0;
	targ_end = (RS_TARGET_STATS *)((PTR)mon_stats->target_stats +
		mon_stats->num_targets * (targ_size + tbl_size *
		mon_stats->num_tables));
	for (targ = mon_stats->target_stats; targ < targ_end; targ =
		(RS_TARGET_STATS *)((PTR)targ + targ_size + tbl_size *
		mon_stats->num_tables))
	{
		if (target_db == targ->db_no)
		{
			*num_txns = targ->num_txns;
			*conn_mins = (targ->last_txn_time -
				targ->first_txn_time) / 60;
			break;
		}
	}
}


/*{
** Name:	RSstats_terminate - terminate statistics
**
** Description:
**	Frees up the shared memory for RepServer statistics.
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
RSstats_terminate()
{
	CL_SYS_ERR	sys_err;

	if (!mon_stats)
		return;
	if (shared_stats)
	{
		MEfree_pages((PTR)mon_stats, num_pages, &sys_err);
		MEsmdestroy(sm_key, &sys_err);
	}
}

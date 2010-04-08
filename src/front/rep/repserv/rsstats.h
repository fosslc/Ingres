/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef RSSTATS_H_INCLUDED
# define RSSTATS_H_INCLUDED
# include <compat.h>
# include <er.h>
# include <pc.h>
# include <gl.h>
# include <iicommon.h>

/**
** Name:	rsstats.h - Replicator Server statistics
**
** Description:
**	Include file for Replicator Server statistics and monitoring via a
**	shared memory segment.
**
** History:
**	05-nov-98 (abbjo03)
**		Created.
**	12-feb-99 (abbjo03)
**		Add database and table names to structures. Add process ID to
**		server structure. Change nats and i4s to i4.
**	24-feb-99 (abbjo03)
**		Add prototype for RSstats_terminate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define RS_STATS_FILE		ERx("rpsvrmon")


/* table statistics */
typedef struct tagRS_TABLE_STATS
{
	i4	table_no;
	i4	num_rows[3];	/* counters by txn type */
	char	table_owner[DB_MAXNAME+1];
	char	table_name[DB_MAXNAME+1];
} RS_TABLE_STATS;


/* target database statistics */
typedef struct tagRS_TARGET_STATS
{
	i2		db_no;
	i4		first_txn_time;
	i4		num_txns;
	i4		last_txn_time;
	char		vnode_name[DB_MAXNAME+1];
	char		db_name[DB_MAXNAME+1];
	RS_TABLE_STATS	*table_stats;
} RS_TARGET_STATS;

/* server statistics */
typedef struct tagRS_MONITOR
{
	i2		server_no;
	i2		local_db_no;
	PID		pid;
	i4		startup_time;
	i4		num_targets;
	i4		num_tables;
	char		vnode_name[DB_MAXNAME+1];
	char		db_name[DB_MAXNAME+1];
	RS_TARGET_STATS	*target_stats;
} RS_MONITOR;


/* function prototypes */
FUNC_EXTERN STATUS RSstats_init(void);

FUNC_EXTERN void RSstats_update(i2 target_db, i4 table_no, i4 txn_type);

FUNC_EXTERN void RSstats_report(i2 target_db, i4 *num_txns, i4 *conn_mins);

FUNC_EXTERN void RSstats_terminate(void);

# endif

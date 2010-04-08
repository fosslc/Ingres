/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <lo.h>
# include <si.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <tbldef.h>
# include "collisn.h"
# include "errm.h"

/**
** Name:	collrpt.sc - Queue collision report
**
**	Defines
**		queue_collision	- queue collision report
**
** History:
**	09-jan-97 (joea)
**		Created based on collrpt.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to create_key_clauses.
**	11-sep-98 (abbjo03)
**		Add table information and owner arguments to print_cols.
**	30-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

struct _varchar
{
	short	length;
	char	data[256];
};

typedef void MSG_FUNC(i4 msg_level, i4  msg_num, ...);


GLOBALREF TBLDEF		*RMtbl;
GLOBALREF COLLISION_FOUND	collision_list[];


static FILE	*report_fp;

EXEC SQL BEGIN DECLARE SECTION;
static i4	initial_session;
static i2	remotedb;
EXEC SQL END DECLARE SECTION;


FUNC_EXTERN void ddba_messageit(i4 msg_level, i4  msg_num, ...);
FUNC_EXTERN void print_cols(FILE *report_fp, TBLDEF *tbl, char *table_owner,
	i2 sourcedb, i4 transaction_id, i4 sequence_no, char *target_wc);
FUNC_EXTERN STATUS get_collisions(char *localdbname, char *vnode_name,
	i2 localdb, char * local_owner, i4  *collision_count, i4  *records);
FUNC_EXTERN STATUS close_sessions(void);
FUNC_EXTERN STATUS RMtbl_fetch(i4 table_no, bool force);
FUNC_EXTERN STATUS create_key_clauses(char *where_clause, char *name_clause,
	char *insert_value_clause, char *update_clause, char *table_name,
	char *table_owner, i4 table_no, char *sha_name, i2 database_no,
	i4 transaction_id, i4 sequence_no, MSG_FUNC *msg_func);


/*{
** Name:	queue_collision - Queue collision report
**
** Description:
**	Check the records in the dd_distrib_queue table for
**	collisions in their target database. Creates a report with
**	information about any detected collisions.
**
** Inputs:
**	localdb		- name of the database to which the caller is connected
**	filename	- full path and name of the output file
**
** Outputs:
**	none
**
** Returns:
**	?
*/
STATUS
queue_collision(
i2	localdb,
char	*filename)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	localdb;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;
	LOCATION	loc;
	EXEC SQL BEGIN DECLARE SECTION;
	char	local_owner[DB_MAXNAME+1];
	char	target_owner[DB_MAXNAME+1];
	char	sourcedbname[DB_MAXNAME+1];
	char	remote_dbname[DB_MAXNAME+1];
	char	vnode_name[DB_MAXNAME+1];
	char	remote_vnode_name[DB_MAXNAME+1];
	i2	sourcedb;
	i4	transaction_id;
	i4	sequence_no;
	i4	err;
	char	select_statement[1024];
	char	where_clause[1024];
	char	tmp[128];	/* strings for the report */
	char	report_filename[MAX_LOC+1];
	EXEC SQL END DECLARE SECTION;
	i4	collision_count = 0;
	i4	records = 0;
	u_i4	i;
	bool	dba_table;
	char	*p;
	i4	remdb;

	STcopy(filename, report_filename);
	LOfroms(PATH & FILENAME, report_filename, &loc);
	if (SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC, &report_fp) != OK)
	{
		IIUGerr(E_RM0087_Err_open_report, UG_ERR_ERROR, 1,
			report_filename);
		return (E_RM0087_Err_open_report);
	}
	EXEC SQL INQUIRE_SQL (:initial_session = SESSION);
	EXEC SQL SELECT database_name , vnode_name
		INTO	:sourcedbname, :vnode_name
		FROM	dd_databases
		WHERE	database_no = :localdb;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		IIUGerr(E_RM0088_Err_rtrv_db_name, UG_ERR_ERROR, 0);
		EXEC SQL ROLLBACK;
		return (E_RM0088_Err_rtrv_db_name);
	}

	STtrmwhite(vnode_name);
	STtrmwhite(sourcedbname);
	p = ERget(F_RM00A9_Prod_name);
	i = 40 + STlength(p) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, p);
	p = ERget(F_RM00D5_Queue_coll_rpt);
	i = 40 + STlength(p) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, p);
	STprintf(tmp, ERx("%s:  %d  %s"), ERget(F_RM00B6_Local_db), localdb,
		sourcedbname);
	i = 40 + STlength(tmp) / (u_i4)2;
	SIfprintf(report_fp, ERx("%*s\n"), i, tmp);

	EXEC SQL SELECT DBMSINFO('DBA') INTO :local_owner;
	err = get_collisions(sourcedbname, vnode_name, localdb, local_owner,
		&collision_count, &records);
	EXEC SQL SET_SQL (SESSION = :initial_session);
	if (err)
	{
		close_sessions();
		EXEC SQL ROLLBACK;
		return (err);
	}

	for (i = 0; i < collision_count; i++)
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		(void)RMtbl_fetch(collision_list[i].db1.table_no, FALSE);
		dba_table = STequal(RMtbl->table_owner, local_owner);
		switch (collision_list[i].type)
		{
		case TX_INSERT:
			SIfprintf(report_fp, ERx("\n%s '%s.%s'\n"),
				ERget(F_RM00D6_Insert_coll), RMtbl->table_owner,
				RMtbl->table_name);
			SIfprintf(report_fp, ERx("%s\n"),
				ERget(F_RM00B6_Local_db));
			SIfprintf(report_fp, ERget(F_RM00D7_Srcdb_txn_seq),
				collision_list[i].db1.sourcedb,
				collision_list[i].db1.transaction_id,
				collision_list[i].db1.sequence_no);
			EXEC SQL SET_SQL (SESSION = :initial_session);
			print_cols(report_fp, RMtbl, RMtbl->table_owner,
				collision_list[i].db1.sourcedb,
				collision_list[i].db1.transaction_id,
				collision_list[i].db1.sequence_no,
				(char *)NULL);

			/* Get the remote vnode and database name */
			remotedb = collision_list[i].remote_db;
			remdb = (i4)remotedb;
			EXEC SQL REPEATED SELECT database_name, vnode_name
				INTO	:remote_dbname, :remote_vnode_name
				FROM	dd_databases
				WHERE	database_no = :remotedb;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
			{
				IIUGerr(E_RM0114_Err_rtrv_rem_db, UG_ERR_ERROR,
					1, &remdb);
				EXEC SQL SET_SQL (SESSION = :initial_session);
				close_sessions();
				EXEC SQL ROLLBACK;
				return (E_RM0114_Err_rtrv_rem_db);
			}
			else if (errinfo.rowcount != 1)
			{
				STcopy(ERget(F_RM00D8_Not_found),
					remote_dbname);
				STcopy(ERget(F_RM00D8_Not_found),
					remote_vnode_name);
			}
			STtrmwhite(remote_vnode_name);
			STtrmwhite(remote_dbname);
			SIfprintf(report_fp, ERget(F_RM00D9_Remote_db),
				remdb, remote_vnode_name, remote_dbname);

			err = create_key_clauses(where_clause, NULL, NULL,
				NULL, RMtbl->table_name, RMtbl->table_owner,
				RMtbl->table_no, RMtbl->sha_name,
				collision_list[i].db1.sourcedb,
				collision_list[i].db1.transaction_id,
				collision_list[i].db1.sequence_no,
				ddba_messageit);
			if (err)
			{
				EXEC SQL SET_SQL (SESSION = :initial_session);
				close_sessions();
				EXEC SQL ROLLBACK;
				return (err);
			}

			remotedb = collision_list[i].remote_db;
			EXEC SQL SET_SQL (SESSION = :remotedb);
			EXEC SQL SELECT DBMSINFO('DBA') INTO :target_owner;
			print_cols(report_fp, RMtbl, dba_table ? target_owner :
				RMtbl->table_owner,
				collision_list[i].db2.sourcedb,
				collision_list[i].db2.transaction_id,
				collision_list[i].db2.sequence_no,
				where_clause);
			break;

		case TX_UPDATE:
		case TX_DELETE:
			SIfprintf(report_fp, ERx("\n%s '%s.%s'\n"),
				collision_list[i].type == TX_UPDATE ?
				ERget(F_RM00DA_Update_coll) :
				ERget(F_RM00DB_Delete_coll), RMtbl->table_owner,
				RMtbl->table_name);
			SIfprintf(report_fp, ERx("%s\n"),
				ERget(F_RM00B6_Local_db));
			SIfprintf(report_fp, ERget(F_RM00D7_Srcdb_txn_seq),
				collision_list[i].db1.sourcedb,
				collision_list[i].db1.transaction_id,
				collision_list[i].db1.sequence_no);
			EXEC SQL SET_SQL (SESSION = :initial_session);
			STprintf(select_statement, ERx("SELECT sourcedb, \
transaction_id, sequence_no FROM %s WHERE sourcedb= %d AND transaction_id = \
%d AND sequence_no = %d AND in_archive = 0"),
				RMtbl->sha_name,
				collision_list[i].db1.sourcedb,
				collision_list[i].db1.transaction_id,
				collision_list[i].db1.sequence_no);
			EXEC SQL EXECUTE IMMEDIATE :select_statement
				INTO	:sourcedb, :transaction_id,
					:sequence_no;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
			{
				IIUGerr(E_RM0115_Err_exec_immed, UG_ERR_ERROR,
					1, RMtbl->sha_name);
				EXEC SQL SET_SQL (SESSION = :initial_session);
				close_sessions();
				EXEC SQL ROLLBACK;
				return (E_RM0115_Err_exec_immed);
			}
			err = create_key_clauses(where_clause, NULL, NULL,
				NULL, RMtbl->table_name, RMtbl->table_owner,
				RMtbl->table_no, RMtbl->sha_name, sourcedb,
				transaction_id, sequence_no, ddba_messageit);
			if (err)
			{
				EXEC SQL SET_SQL (SESSION = :initial_session);
				close_sessions();
				EXEC SQL ROLLBACK;
				return (err);
			}
			EXEC SQL SET_SQL (SESSION = :initial_session);
			print_cols(report_fp, RMtbl, RMtbl->table_owner,
				sourcedb, transaction_id, sequence_no,
				(char *)NULL);
			/* Get the remotedb database_name and vnode_name */
			remotedb = collision_list[i].remote_db;
			remdb = (i4)remotedb;
			EXEC SQL REPEATED SELECT database_name,
					vnode_name
				INTO	:remote_dbname,
					:remote_vnode_name
				FROM	dd_databases
				WHERE	database_no = :remotedb;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
			{
				IIUGerr(E_RM0114_Err_rtrv_rem_db, UG_ERR_ERROR,
					1, &remdb);
				EXEC SQL SET_SQL (SESSION = :initial_session);
				close_sessions();
				EXEC SQL ROLLBACK;
				return (E_RM0114_Err_rtrv_rem_db);
			}
			else if (errinfo.rowcount != 1)
			{
				STcopy(ERget(F_RM00D8_Not_found),
					remote_dbname);
				STcopy(ERget(F_RM00D8_Not_found),
					remote_vnode_name);
			}
			STtrmwhite(remote_vnode_name);
			STtrmwhite(remote_dbname);
			SIfprintf(report_fp, ERget(F_RM00D9_Remote_db),
				remdb, remote_vnode_name, remote_dbname);
			EXEC SQL SET_SQL (SESSION = :remotedb);
			EXEC SQL SELECT DBMSINFO('DBA') INTO :target_owner;
			print_cols(report_fp, RMtbl, dba_table ? target_owner :
				RMtbl->table_owner, sourcedb, transaction_id,
				sequence_no, where_clause);
			break;
		}
	}
	SIfprintf(report_fp, ERget(F_RM00DC_Colls_found), collision_count);
	SIclose(report_fp);
	EXEC SQL SET_SQL (SESSION = :initial_session);
	close_sessions();
	EXEC SQL commit;

	return (OK);
}

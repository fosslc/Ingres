/*
** Copyright (c) 2004 Ingres Corporation
*/
EXEC SQL INCLUDE sqlca;

# include <compat.h>

/**
** Name:	moddict.sc - Modify Replicator DD tables
**
** Description:
**	Defines
**		modify_tables	- Remodify DD tables
**
** History:
**	16-dec-96 (joea)
**		Created based on moddict.sc in replicator library.
**	14-jan-97 (joea)
**		Remove dd_target_types.
**	24-jan-97 (joea)
**		Add MAXPAGES to dd_last_number to prevent lock escalation.
**	22-apr-97 (joea)
**		Modify dd_input_queue only on dd_transaction_id.
**	16-jun-98 (abbjo03)
**		Add indexes argument to create persistent indexes if called
**		from repcat and convrep, but not from repmod.
**	03-sep-99 (abbjo03) sir 98645
**		Modify input queue to hash with suitable minpages. Modify
**		distribution queue to get better performance on deletes and on
**		read joins.
**	28-oct-99 (abbjo03)
**		Split out input queue modification so it can be called from
**		create_replication_keys.
**/

STATUS RMmodify_input_queue(void);


/*{
** Name:	modify_tables - Remodify Replicator DD tables
**
** Description:
**	Remodifies the Replicator data dictionary tables to their
**	optimal storage structures.
**
** FIXME:
**	This could take a parameter to avoid remodifying read-only
**	tables like dd_events.
**
** Inputs:
**	indexes	- create indexes?
**
** Outputs:
**	none
**
** Returns:
**	OK on success, FAIL if there is a database error.
*/
STATUS
modify_tables(
bool	indexes)
{
	EXEC SQL WHENEVER SQLERROR GOTO mod_err;
	EXEC SQL WHENEVER SQLWARNING CALL sqlprint;

	EXEC SQL MODIFY dd_nodes TO BTREE UNIQUE ON vnode_name;

	EXEC SQL MODIFY dd_databases TO BTREE UNIQUE ON database_no;

	EXEC SQL MODIFY dd_paths TO BTREE UNIQUE
		ON cdds_no, sourcedb, localdb, targetdb;

	EXEC SQL MODIFY dd_cdds TO BTREE UNIQUE ON cdds_no;

	EXEC SQL MODIFY dd_db_cdds TO BTREE UNIQUE ON cdds_no, database_no;

	EXEC SQL MODIFY dd_regist_tables TO BTREE UNIQUE on table_no;

	EXEC SQL MODIFY dd_regist_columns TO BTREE UNIQUE
		ON table_no, column_name;

	if (RMmodify_input_queue() != OK)
		return (FAIL);

	EXEC SQL MODIFY dd_distrib_queue TO BTREE
		ON targetdb, cdds_no, sourcedb, transaction_id;

	EXEC SQL MODIFY dd_mail_alert TO BTREE UNIQUE ON mail_username;

	EXEC SQL MODIFY dd_servers TO BTREE UNIQUE ON server_no;

	EXEC SQL MODIFY dd_support_tables TO BTREE UNIQUE ON table_name;

	EXEC SQL MODIFY dd_last_number TO HASH UNIQUE ON column_name
		WITH MAXPAGES = 7;

	/* read-only tables */

	EXEC SQL MODIFY dd_events TO ISAM UNIQUE ON dbevent
		WITH FILLFACTOR = 100;

	EXEC SQL MODIFY dd_server_flags TO ISAM UNIQUE ON flag_name
		WITH FILLFACTOR = 100;

	EXEC SQL MODIFY dd_option_values TO ISAM UNIQUE
		ON option_type, option_name, numeric_value
		WITH FILLFACTOR = 100;

	EXEC SQL MODIFY dd_flag_values TO ISAM ON startup_flag;

	if (indexes)
	{
		EXEC SQL CREATE UNIQUE INDEX dd_db_name_idx
			ON dd_databases (vnode_name, database_name)
			WITH PERSISTENCE;

		EXEC SQL CREATE UNIQUE INDEX dd_cdds_name_idx
			ON dd_cdds(cdds_name)
			WITH PERSISTENCE;

		EXEC SQL CREATE UNIQUE INDEX dd_reg_tbl_idx
			ON dd_regist_tables (table_name, table_owner)
			WITH PERSISTENCE;
	}

	return (OK);

mod_err:
	return (FAIL);
}

STATUS
RMmodify_input_queue()
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	err;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL WHENEVER SQLERROR CONTINUE;
	EXEC SQL MODIFY dd_input_queue TO HASH ON transaction_id
		WITH MINPAGES = 1024;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	return ((STATUS)err);
}

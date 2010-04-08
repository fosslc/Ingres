/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <rpu.h>

/**
** Name:	dbdelete.sc - delete a database
**
** Description:
**	Defines
**		db_delete	- delete a database
**
** History:
**	16-dec-96 (joea)
**		Created based on dbdelete.sc in replicator library.
**/

STATUS db_config_changed(i2 db_no);


/*{
** Name:	db_delete - delete a database
**
** Description:
**	Delete a replicated database object from the dd_databases and related
**	catalogs.
**
** Inputs:
**	database_no	- database number
**
** Outputs:
**	none
**
** Returns:
**	OK, FAIL or DBMS error
*/
STATUS
database_delete(
i2	database_no)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	database_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;

	EXEC SQL REPEATED DELETE FROM dd_databases
		WHERE	database_no = :database_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	EXEC SQL DELETE FROM dd_db_cdds
		WHERE	database_no = :database_no;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	EXEC SQL REPEATED DELETE FROM dd_paths
		WHERE	localdb = :database_no
		OR	sourcedb = :database_no
		OR	targetdb = :database_no;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		return (FAIL);
	}

	EXEC SQL COMMIT;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	return (OK);
}

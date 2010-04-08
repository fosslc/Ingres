/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <rpu.h>

/*
** Name:	cddsdel.sc
**
** Description:
**	Delete a CDDS from the dd_cdds table and deassign tables in the
**	deleted CDDS.
**
** History:
**	16-dec-96 (joea)
**		Created based on cddsdel.sc in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN STATUS db_config_changed(i2 db_no);


STATUS
cdds_delete(
i2	cdds_no,
i4	RegTableCount)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	cdds_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;

	EXEC SQL REPEATED DELETE FROM dd_cdds
		WHERE	cdds_no = :cdds_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	EXEC SQL REPEATED DELETE FROM dd_paths
		WHERE	cdds_no = :cdds_no;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	EXEC SQL DELETE FROM dd_db_cdds
		WHERE	cdds_no = :cdds_no;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		return (errinfo.errorno);
	}

	if (RegTableCount > 0)
	{
		EXEC SQL UPDATE dd_regist_tables
			SET	cdds_no = 0
			WHERE	cdds_no = :cdds_no;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
		{
			EXEC SQL ROLLBACK;
			return (errinfo.errorno);
		}
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

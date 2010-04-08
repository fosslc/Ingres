/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <tblobjs.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	deregtbl.sc - De-register replicated table
**
** Description:
**	Defines
**		deregister	- de-register a replicated table
**
** History:
**	16-dec-96 (joea)
**		Created based on deregtbl.sc in replicator library.
**	10-oct-97 (joea)
**		Remove unused argument to dropsupp and deregister.
**      16-Aug-98 (islsa01)
**              Call RMtcb_flush() to toss the tcb after deregisterring a table
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit and string literals with IIUGerr calls.
**/

FUNC_EXTERN STATUS db_config_changed(i2 db_no);
FUNC_EXTERN STATUS RMtcb_flush(void);

/*{
** Name:	deregister - de-register a replicated table
**
** Description:
**	Removes registration information, rules, procedures, and
**	support tables for the specified table.
**
** Inputs:
**	table_no	- numeric identifier for the table
**
** Outputs:
**	none
**
** Returns:
**	0 for success
**	E_RM00ED_Cant_dereg_tbl, E_RM00EC_Err_del_reg_tbl
**	Also returns statuses from the "drop" routines
*/
STATUS
deregister(
i4	*table_no)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	*table_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO errinfo;
	STATUS	err;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	NumQueue;	/* count of row in queues. */
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT COUNT(*)
		INTO	:NumQueue
		FROM	dd_distrib_queue
		WHERE	table_no = :*table_no;
	if (RPdb_error_check(0, &errinfo))
		return (FAIL);

	if (NumQueue > 0)
	{
		IIUGerr(E_RM00ED_Cant_dereg_tbl, UG_ERR_ERROR, 0);
		return (E_RM00ED_Cant_dereg_tbl);
	}

	EXEC SQL SELECT COUNT(*)
		INTO	:NumQueue
		FROM	dd_input_queue
		WHERE	table_no = :*table_no;
	if (RPdb_error_check(0, &errinfo))
		return (FAIL);

	if (NumQueue > 0)
	{
		IIUGerr(E_RM00ED_Cant_dereg_tbl, UG_ERR_ERROR, 0);
		return (E_RM00ED_Cant_dereg_tbl);
	}

	/* Drop all support objects */
	err = dropsupp(*table_no);
	if (err)
		return (err);

	/* Now delete from registration tables */
	EXEC SQL DELETE FROM dd_regist_columns
		WHERE	table_no = :*table_no;
	if (RPdb_error_check(0, &errinfo))
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM00EC_Err_del_reg_tbl, UG_ERR_ERROR, table_no);
		return (E_RM00EC_Err_del_reg_tbl);
	}

	EXEC SQL DELETE FROM dd_regist_tables
		WHERE	table_no = :*table_no;
	if (RPdb_error_check(0, &errinfo))
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM00EC_Err_del_reg_tbl, UG_ERR_ERROR, table_no);
		return (E_RM00EC_Err_del_reg_tbl);
	}

	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		return (FAIL);
	}

	EXEC SQL COMMIT;
        *table_no = 0;
        return (RMtcb_flush());
}

/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	dbcfgchg.sc - set dd_databases.config_changed
**
** Defines:
**	db_config_changed - set dd_databases.config_changed
**
** History:
**	16-dec-96 (joea)
**		Created based on dbcfgchg.sc in replicator library.
**	24-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**/

/*{
** Name:	db_config_changed - set dd_databases.config_changed
**
** Description:
**	Sets dd_databases.config_changed to current date/time.
**	The db_no parameter is currently disregarded.  In the future, it
**	would allow selective changing of dd_databases rows.
**
** Inputs:
**	db_no	- database number
**
** Outputs:
**	none
**
** Returns:
**	OK or E_RM00E3_Err_updt_cfg_chg
*/
STATUS
db_config_changed(
i2	db_no)
{
	EXEC SQL UPDATE dd_databases
		SET	config_changed = DATE('now');
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM00E3_Err_updt_cfg_chg, UG_ERR_ERROR, 0);
		return (E_RM00E3_Err_updt_cfg_chg);
	}

	return (OK);
}

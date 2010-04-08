/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <rpu.h>
# include <iiapi.h>
# include <sapiw.h>
# include "rsconst.h"
# include "conn.h"

/**
** Name:	dbquiet.c - quieting routines
**
** Description:
**	Defines
**		quiet_db	- quiet a database
**		unquiet_db	- unquiet a database
**		quiet_cdds	- quiet a cdds
**		unquiet_cdds	- unquiet a cdds
**		unquiet_all	- unquiet all databases and cdds's
**		quiet_update	- update dd_db_cdds is_quiet
**
** History:
**	16-dec-96 (joea)
**		Created based on quietdb.sc in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw functions.
**/

static IIAPI_STATUS quiet_update(char *stmt);


/*{
** Name:	quiet_db - quiet a database
**
** Description:
**	Updates the database to mark the specified database number
**	as quiet.
**
** Inputs:
**	db_no		- database number to be quieted
**	quiet_type	- used to specify a USER_QUIET or SERVER_QUIET state
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
quiet_db(
i2	db_no,
i4	quiet_type)
{
	char	stmt[512];

	/* Setting target database %d to quiet */
	messageit(2, 1718, db_no);
	STprintf(stmt,
		ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE server_no = %d \
AND database_no = %d AND is_quiet < %d"),
		quiet_type, (i4)RSserver_no, (i4)db_no, quiet_type);

	if (quiet_update(stmt) != IIAPI_ST_SUCCESS)
		RSshutdown(FAIL);
}


/*{
** Name:	unquiet_db - unquiet a database
**
** Description:
**	Updates the database to mark the specified database number
**	as unquiet.
**
** Inputs:
**	db_no		- database number to be unquieted
**	quiet_type	- used to specify a USER_QUIET or SERVER_QUIET state
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
unquiet_db(
i2	db_no,
i4	quiet_type)
{
	char	stmt[512];

	/* Unquieting target database %d */
	messageit(2, 1722, db_no);

	STprintf(stmt,
		ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE server_no = %d \
AND database_no = %d AND is_quiet != %d AND is_quiet <= %d"),
		NOT_QUIET, (i4)RSserver_no, (i4)db_no, NOT_QUIET, quiet_type);
	if (quiet_update(stmt) != IIAPI_ST_SUCCESS)
		RSshutdown(FAIL);
}


/*{
** Name:	quiet_cdds - quiet a cdds
**
** Description:
**	Updates the database to mark the specified cdds number
**	as quiet.
**
** Inputs:
**	dbno_ptr	- database be quieted, NULL for all
**	cdds_no		- cdds number to be quieted
**	quiet_type	- used to specify a USER_QUIET or SERVER_QUIET state
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
quiet_cdds(
i2	*dbno_ptr,
i2	cdds_no,
i4	quiet_type)
{
	char	stmt[512];

	if (dbno_ptr == NULL)
	{
		/* Setting CDDS %d to quiet */
		messageit(2, 1726, cdds_no);

		STprintf(stmt,
			ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE \
server_no = %d AND cdds_no = %d AND is_quiet < %d"),
			quiet_type, (i4)RSserver_no, (i4)cdds_no, quiet_type);
	}
	else
	{
		/* Setting database %d CDDS %d to quiet */
		messageit(2, 1727, *dbno_ptr, cdds_no);

		STprintf(stmt,
			ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE \
server_no = %d AND database_no = %d AND cdds_no = %d AND is_quiet < %d"),
			quiet_type, (i4)RSserver_no, (i4)*dbno_ptr,
			(i4)cdds_no, quiet_type);
	}
	if (quiet_update(stmt) != IIAPI_ST_SUCCESS)
		RSshutdown(FAIL);
}


/*{
** Name:	unquiet_cdds - unquiet a cdds
**
** Description:
**	Updates the database to mark the specified cdds number
**	as unquiet.
**
** Inputs:
**	dbno_ptr	- database be quieted, NULL for all
**	cdds_no		- cdds number to be unquieted
**	quiet_type	- used to specify a USER_QUIET or SERVER_QUIET state
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
unquiet_cdds(
i2	*dbno_ptr,
i2	cdds_no,
i4	quiet_type)
{
	char	stmt[512];

	if (dbno_ptr == NULL)
	{
		/* Unquieting CDDS %d */
		messageit(2, 1731, cdds_no);

		STprintf(stmt,
			ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE \
server_no = %d AND cdds_no = %d AND is_quiet != %d AND is_quiet <= %d"),
			NOT_QUIET, (i4)RSserver_no, (i4)cdds_no, NOT_QUIET,
			quiet_type);
	}
	else
	{
		/* Unquieting database %d CDDS %d */
		messageit(2, 1732, *dbno_ptr, cdds_no);

		STprintf(stmt,
			ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE \
server_no = %d AND database_no = %d AND cdds_no = %d AND is_quiet != %d AND \
is_quiet <= %d"),
			NOT_QUIET, (i4)RSserver_no, (i4)*dbno_ptr,
			(i4)cdds_no, NOT_QUIET, quiet_type);
	}
	if (quiet_update(stmt) != IIAPI_ST_SUCCESS)
		RSshutdown(FAIL);
}


/*{
** Name:	unquiet_all - unquiet all databases and cdds's
**
** Description:
**	Updates the database to mark all of the database numbers and cdds
**	numbers as unquiet.
**
** Inputs:
**	quiet_type	- used to specify a USER_QUIET or SERVER_QUIET state
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
unquiet_all(
i4	quiet_type)
{
	char	stmt[512];

	/* Unquieting all databases and CDDSs */
	messageit(2, 1736);

	STprintf(stmt,
		ERx("UPDATE dd_db_cdds SET is_quiet = %d WHERE server_no = %d AND is_quiet != %d AND is_quiet <= %d"),
		NOT_QUIET, (i4)RSserver_no, NOT_QUIET, quiet_type);

	if (quiet_update(stmt) != IIAPI_ST_SUCCESS)
		RSshutdown(FAIL);
}


/*{
** Name:	quiet_update - update dd_db_cdds is_quiet
**
** Description:
**	Updates the database to mark all of the database numbers and cdds
**	numbers as unquiet.
**
** Inputs:
**	stmt	- UPDATE statement
**
** Outputs:
**	none
**
** Returns:
**	status from IIsw_queryNP or IIsw_commit
*/
static IIAPI_STATUS
quiet_update(
char	*stmt)
{
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_STATUS		status;
	IIAPI_GETQINFOPARM	getQinfoParm;

	if (IIsw_queryNP(RSlocal_conn.connHandle, &RSlocal_conn.tranHandle,
		stmt, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1719, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (errParm.ge_errorCode);
	}
	IIsw_close(stmtHandle, &errParm);

	status = IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
	{
		messageit(1, 1798, ERx("quiet_update"), errParm.ge_errorCode,
			errParm.ge_message);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
	}

	return (status);
}

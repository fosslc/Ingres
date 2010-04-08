/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <si.h>
# include <me.h>
# include <er.h>
# include <te.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <tblobjs.h>
# include <tbldef.h>
# include "errm.h"

/*
** Name:	cddsacti.sc - CDDS activate
**
** Description:
**	Defines
**		turn_recorder	- turn Change Recorder on or off
**		cdds_activate - creates activation rules for a CDDS
**		tbl_deactivate - drops change recorder rules for a table
**
** History:
**	09-jan-97 (joea)
**		Created based on cddsacti.sc in replicator library.
**	26-mar-97 (joea)
**		Call RMtcb_flush after deactivating a table.
**	19-may-97 (joea) bug 82149
**		Deal with tbl_rules() returning -1 (no need to activate).
**	25-sep-97 (joea)
**		Remove unused second argument to tbl_deactivate.
**	09-oct-97 (joea)
**		Remove unused argument to tbl_rules.
**	04-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. In cdds_activate, retrieve
**		the number of tables in the CDDS and then allocate memory for
**		the table numbers instead of having an arbitrary limit.
**	10-dec-98 (abbjo03)
**		Change action/tbl_activate parameters to a bool. Eliminate
**		dbname parameter to cdds_activate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF TBLDEF	*RMtbl;

STATUS db_config_changed(i2 db_no);
FUNC_EXTERN STATUS RMtcb_flush(void);
STATUS tbl_deactivate(i4 table_no);

# ifdef NT_GENERIC

STATUS std_turn_recorder(char *dbname, i2 cdds_no, bool activate);

STATUS
turn_recorder(
char	*dbname,
i2	cdds_no,
bool	activate)
{
	STATUS	status;

	TErestore(TE_NORMAL);
	status = std_turn_recorder(dbname, cdds_no, activate);
	TErestore(TE_FORMS);
	return (status);
}

# define turn_recorder		std_turn_recorder
# endif


/*{
** Name:	cdds_activate
**
** Description:
**	Creates or drops the change recorder rules for a CDDS.
**
** Inputs:
**	cdds_no		- integer dataset number.
**	tbl_activate	- activate or deactivate
**
** Outputs:
**	none
**
** Returns
**	0	- action successful
**	-1	- no tables in CDDS
**	err	- database error number
*/
STATUS
cdds_activate(
i2	cdds_no,
bool	tbl_activate)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	cdds_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	i4	i;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	num_tables;
	i4	*table_nos;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;
	STATUS	err = 0;

	EXEC SQL SELECT COUNT(*)
		INTO	:num_tables
		FROM	dd_regist_tables
		WHERE	cdds_no = :cdds_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM0082_Error_rtrv_tables, UG_ERR_ERROR, 0);
		return (errinfo.errorno);
	}
	if (!num_tables)
	{
		EXEC SQL ROLLBACK;
		return (-1);
	}

	table_nos = (i4 *)MEreqmem(0, (u_i4)(num_tables * sizeof(i4 *)), TRUE,
		NULL);
	if (!table_nos)
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM0083_Error_alloc_tables, UG_ERR_ERROR, 0);
		return (E_RM0083_Error_alloc_tables);
	}
		
	i = 0;
	EXEC SQL SELECT table_no
		INTO	:table_nos[i]
		FROM	dd_regist_tables
		WHERE	cdds_no = :cdds_no;
	EXEC SQL BEGIN;
		if (i >= num_tables)
			EXEC SQL ENDSELECT;
		++i;
	EXEC SQL END;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		if (errinfo.errorno != 0)
		{
			IIUGerr(E_RM0082_Error_rtrv_tables, UG_ERR_ERROR, 0);
			return (errinfo.errorno);
		}
	}
	EXEC SQL COMMIT;

	if (tbl_activate)
	{
		/* Create the rules */
		for (i = 0; i < num_tables; i++)
		{
			err = tbl_rules(table_nos[i]);
			if (err != OK && err != -1)
				return (err);
		}
		EXEC SQL COMMIT;
	}
	else	/* Drop the rules */
	{
		for (i = 0; i < num_tables; i++)
		{
			err = tbl_deactivate(table_nos[i]);
			if (err)
				return (err);
		}
		EXEC SQL COMMIT;
	}

	return (err);
}


/*{
** Name:	tbl_deactivate
**
** Description:
**	Drops change recorder rules and updates dd_regist_tables
**	to clear timestamps.
*/
STATUS
tbl_deactivate(
i4	table_no)
{
	DBEC_ERR_INFO	errinfo;
	STATUS	err;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	t_no;
	EXEC SQL END DECLARE SECTION;

	err = RMtbl_fetch(table_no, TRUE);
	if (err)
		return (err);

	t_no = RMtbl->table_no;

	EXEC SQL UPDATE dd_regist_tables
		SET	rules_created = ''
		WHERE	table_no = :t_no;
	if (RPdb_error_check(0, &errinfo))
		return(errinfo.errorno);

	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		return (FAIL);
	}

	return (RMtcb_flush());
}


/*{
** Name:	turn_recorder - turn change recorder on or off
**
** Description:
**	Turn change recorders on or off, i.e. create or drop
**	rules for tables in the CDDS.
**
** Inputs:
**	dbname	- target db name
**	cdds_no	- cdds no
**	activate	- activate or deactivate
**
** Outputs:
**	none
**
** Returns:
**	OK	- successful
**	-1	- no tables
**	others	- error
*/
STATUS
turn_recorder(
char	*dbname,
i2	cdds_no,
bool	activate)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*dbname;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	char	dba[DB_MAXNAME+1];
	char	username[DB_MAXNAME+1];
	i4	other_session = 200;
	i4	old_session;
	EXEC SQL END DECLARE SECTION;
	STATUS	retval;

	EXEC SQL INQUIRE_SQL (:old_session = SESSION);

	if (!activate)
		SIprintf("Deactivate CDDS '%d' in database '%s'. . .\n\r",
			cdds_no, dbname);
	else
		SIprintf("Activate CDDS '%d' in database '%s'. . .\n\r",
			cdds_no, dbname);
	SIflush(stdout);

	EXEC SQL CONNECT :dbname SESSION :other_session;
	if (RPdb_error_check(0, &errinfo))
	{
		EXEC SQL SET_SQL (session = :old_session);
		IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1, dbname);
		return (E_RM0084_Error_connecting);
	}

	/* See if user is the DBA; if not, reconnect as DBA. */
	EXEC SQL SELECT DBMSINFO('dba'), DBMSINFO('username')
		INTO	:dba, :username;
	if (STcompare(dba, username))
	{
		EXEC SQL DISCONNECT SESSION :other_session;
		EXEC SQL CONNECT :dbname SESSION :other_session
			IDENTIFIED BY :dba;
		if (RPdb_error_check(0, &errinfo))
		{
			EXEC SQL SET_SQL (session = :old_session);
			IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR, 1,
				dbname);
			return (E_RM0084_Error_connecting);
		}
	}

	retval = cdds_activate(cdds_no, activate);

	if (retval == 0 || retval == -1)
		EXEC SQL COMMIT;
	else
		EXEC SQL ROLLBACK;

	EXEC SQL DISCONNECT SESSION :other_session;
	EXEC SQL SET_SQL (SESSION = :old_session);

	return (retval);
}

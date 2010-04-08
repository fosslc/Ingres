/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <te.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <targtype.h>
# include <tblobjs.h>
# include "errm.h"

EXEC SQL INCLUDE <tbldef.sh>;

/*
** Name:	tblrules.sc - create table rules
**
** Description:
**	Defines
**		tbl_rules	- create table rules
**		RMtcb_flush	- force a TCB flush
**
** History:
**	16-dec-96 (joea)
**		Created based on tblrules.sc in replicator library.
**	26-mar-97 (joea)
**		Add RMtcb_flush to force a TCB flush in the DBMS.
**	19-may-97 (joea) bug 82149
**		Do not activate if target type is not Full Peer and return -1.
**	09-oct-97 (joea)
**		Remove unused argument to tbl_rules.
**	18-may-98 (padni01) bug 89865
**		Use date_gmt instead of date to set rules_created field 
**		of dd_regist_tables.
**	02-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Remove NT_GENERIC
**		workaround.
**/

GLOBALREF
EXEC SQL BEGIN DECLARE SECTION;
	TBLDEF	*RMtbl;
EXEC SQL END DECLARE SECTION;


FUNC_EXTERN STATUS db_config_changed(i2 db_no);


STATUS RMtcb_flush(void);


/*{
** Name:	tbl_rules - create table rules
**
** Description:
**	Creates the activation rules for a table
**
** Inputs:
**	table_no	- table number
**
** Outputs:
**	none
**
** Returns:
**	-1	No need to activate
**	E_RM0085_No_cols_registered
**	E_RM0086_No_supp_objs
**	FAIL	error
**	SQL error number
*/
STATUS
tbl_rules(
i4	table_no)
{
	DBEC_ERR_INFO	errinfo;

	/*
	** Check to see if columns have been registered and support tables
	** built
	*/
	(void)RMtbl_fetch(table_no, TRUE);

	if (RMtbl->target_type != TARG_FULL_PEER)
		return (-1);

	if (*RMtbl->columns_registered == EOS)	/* no columns */
	{
		IIUGerr(E_RM0085_No_cols_registered, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM0085_No_cols_registered);
	}

	if (*RMtbl->supp_objs_created == EOS)
	{
		IIUGerr(E_RM0086_No_supp_objs, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM0086_No_supp_objs);
	}

	EXEC SQL UPDATE dd_regist_tables
		SET	rules_created = DATE_GMT('now')
		WHERE	table_no = :RMtbl->table_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		if (errinfo.errorno != 0)
			return (errinfo.errorno);
		else	/* rowcount == 0 */
			return (FAIL);
	}
	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		return (FAIL);
	}
	EXEC SQL COMMIT;

	return (RMtcb_flush());
}


/*{
** Name:	RMtcb_flush - force a TCB flush
**
** Description:
**	Force a TCB flush so the DBMS will be aware that replication was
**	enabled or disabled.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK, FAIL or SQL errorno
*/
STATUS
RMtcb_flush()
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	table_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT reltid
		INTO	:table_id
		FROM	iirelation
		WHERE	relid = :RMtbl->table_name
		AND	relowner = :RMtbl->table_owner;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		EXEC SQL ROLLBACK;
		if (errinfo.errorno)
			return (errinfo.errorno);
		else
			return (FAIL);
	}
	EXEC SQL SET TRACE POINT dm713 :table_id;

	EXEC SQL COMMIT;
	return (OK);
}

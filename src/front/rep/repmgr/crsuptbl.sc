/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <si.h>
# include <te.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <generr.h>
# include <rpu.h>
# include <targtype.h>
# include <tblobjs.h>
# include "errm.h"

EXEC SQL INCLUDE <tbldef.sh>;

/*
** Name:	crsuptbl.sc - create support tables
**
** Description:
**	Defines
**		create_support_tables	- create support tables
**		grant_sup_to_tblOwner	- grant privs on support tables
**
** History:
**	09-jan-97 (joea)
**		Created based on crsuptbl.sc in replicator library.
**	14-jan-97 (joea)
**		Use defined constants for target types.
**	01-may-97 (joea) bug 75327
**		Return OK at end of grant_sup_to_tblOwner.  Only call it, if
**		the owner is not the DBA.
**	18-sep-97 (joea)
**		Remove calls to save_rpgen_log(), since sql_gen() does not use
**		rpgensql to create the support tables.
**	30-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	09-nov-99 (abbjo03)
**		Split out RMmodify_shadow so that it can be called from
**		create_replication_keys.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF
EXEC SQL BEGIN DECLARE SECTION;
	TBLDEF *RMtbl;
EXEC SQL END DECLARE SECTION;


STATUS RMmodify_shadow(TBLDEF *tbl, bool recreate_index);


static STATUS grant_sup_to_tblOwner(void);


# ifdef NT_GENERIC
STATUS
create_support_tables(
i4	table_no)
{
	STATUS	status;

	TErestore(TE_NORMAL);
	status = std_create_support_tables(table_no);
	TErestore(TE_FORMS);
	return (status);
}

# define create_support_tables	std_create_support_tables
# endif


/*{
** Name:	create_support_tables - create support tables
**
** Description:
**	Creates the shadow and archive tables.
**
** Inputs:
**	table_no	- table number
**
** Outputs:
**	none
**
** Returns:
**	?
*/
STATUS
create_support_tables(
i4	table_no)
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	err = 0;
	i4	rows = 0;
	i4	is_stale;
	char	dba[DB_MAXNAME+1];
	EXEC SQL END DECLARE SECTION;
	i4	exists = 0;

	(void)RMtbl_fetch(table_no, FALSE);

	SIprintf(ERget(F_RM00D2_Proc_tbl), RMtbl->table_owner,
		RMtbl->table_name);
	SIflush(stdout);

	EXEC SQL SELECT DBMSINFO('DBA') INTO :dba;

	err = table_exists(RMtbl->table_name, RMtbl->table_owner, &exists);
	if (err)
		return (err);
	if (!exists)
	{
		IIUGerr(E_RM0100_Tbl_not_found, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM0100_Tbl_not_found);
	}

	EXEC SQL SELECT INT4(INTERVAL('seconds',
			DATE(:RMtbl->columns_registered) - DATE(create_date)))
		INTO	:is_stale
		FROM	iitables
		WHERE	table_name = :RMtbl->sha_name;
	err = RPdb_error_check(DBEC_SINGLE_ROW, &errinfo);
	if (err)
	{
		if (errinfo.rowcount == 0)
		{
			is_stale = TRUE;	/* table does not exist */
		}
		else
		{
			IIUGerr(E_RM0101_Err_rtrv_shad_info, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			return (err);
		}
	}

	if (is_stale < 0)
	{
		SIprintf(ERget(E_RM010B_Sup_up_to_date), RMtbl->table_owner,
			RMtbl->table_name);
		SIflush(stdout);
		return (-1);	/* Denotes no need to proceed */
	}

	SIprintf(ERget(F_RM00D3_Gen_sup_tbl), RMtbl->table_owner,
		RMtbl->table_name);
	SIflush(stdout);

	err = dropobj(RMtbl->sha_name, ERx("TABLE"));
	if (err)
		return (err);

	/*
	** sql_gen() now executes the procedure immediately,
	** and checks for errors.  If GE_NO_RESOURCE is returned,
	** the query is written to a file and executed via TM
	** in which case we need to perform the select to determine
	** success of sql_gen()
	*/

	/* Create shadow table */
	err = sql_gen(RMtbl->table_name, RMtbl->table_owner, RMtbl->table_no,
		ERx("crshadow.tlp"), ERx("c_s"));
	switch (err)
	{
	case 0:			/* OK */
		break;

	case GE_NO_PRIVILEGE:	/* Insufficient privilege */
		IIUGerr(E_RM00F7_No_grant_permit, UG_ERR_ERROR, 0);
		return (E_RM00F7_No_grant_permit);

	case GE_NO_RESOURCE:	/* Insufficient resources */
		err = table_exists(RMtbl->sha_name, dba, &exists);
		if (err || !exists)
		{
			IIUGerr(E_RM0102_Tbl_create_fail, UG_ERR_ERROR, 1,
				RMtbl->sha_name);
			return (E_RM0102_Tbl_create_fail);
		}
		break;

	default:
		IIUGerr(E_RM0103_Err_create_shad, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM0103_Err_create_shad);
	}

	/* Modify shadow table and create its index */
	err = RMmodify_shadow(RMtbl, TRUE);
	if (err)
	{
		dropobj(RMtbl->sha_name, ERx("TABLE"));
		return (err);
	}

	EXEC SQL INSERT INTO dd_support_tables (table_name)
		VALUES	(:RMtbl->sha_name);
	err = RPdb_error_check(DBEC_SINGLE_ROW, &errinfo);
	if (err)
	{
		IIUGerr(E_RM0106_Err_ins_sha_sup, UG_ERR_ERROR, 2,
			RMtbl->sha_name);
		return (err);
	}
	EXEC SQL COMMIT;

	/* Generate for full peer database */
	if (RMtbl->target_type == TARG_FULL_PEER)
	{
		if (err = table_exists(RMtbl->arc_name, dba, &exists))
			return (err);
		if (exists)
		{
			err = dropobj(RMtbl->arc_name, ERx("TABLE"));
			if (err)
				return (err);
		}
		err = sql_gen(RMtbl->table_name, RMtbl->table_owner,
			RMtbl->table_no, ERx("crarchiv.tlp"), ERx("c_a"));
		if (err && err != GE_NO_RESOURCE)
		{
			dropobj(RMtbl->arc_name, ERx("TABLE"));
			dropobj(RMtbl->sha_name, ERx("TABLE"));
			if (err == GE_NO_PRIVILEGE)
			{
				IIUGerr(E_RM00F7_No_grant_permit, UG_ERR_ERROR,
					0);
				return (E_RM00F7_No_grant_permit);
			}
			IIUGerr(E_RM0107_Err_create_arch, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			return (E_RM0107_Err_create_arch);
		}
		else
		{
			if (err == GE_NO_RESOURCE)
			{
				err = table_exists(RMtbl->arc_name, dba,
					&exists);
				if (err || !exists)
				{
					IIUGerr(E_RM0102_Tbl_create_fail,
						UG_ERR_ERROR, 1,
						RMtbl->sha_name);
					dropobj(RMtbl->sha_name, ERx("TABLE"));
					return (E_RM0102_Tbl_create_fail);
				}
			}
		}
		err = sql_gen(RMtbl->table_name, RMtbl->table_owner,
			RMtbl->table_no, ERx("mdarchiv.tlp"), ERx("m_a"));
		if (err && err != GE_NO_RESOURCE)
		{
			dropobj(RMtbl->sha_name, ERx("TABLE"));
			dropobj(RMtbl->arc_name, ERx("TABLE"));
			if (err == GE_NO_PRIVILEGE)
			{
				IIUGerr(E_RM00F7_No_grant_permit, UG_ERR_ERROR,
					0);
				return (E_RM00F7_No_grant_permit);
			}
			IIUGerr(E_RM0108_Err_modify_arch, UG_ERR_ERROR, 1,
				RMtbl->sha_name);
			return (E_RM0108_Err_modify_arch);
		}

		EXEC SQL INSERT INTO dd_support_tables (table_name)
			VALUES	(:RMtbl->arc_name);
		err = RPdb_error_check(DBEC_SINGLE_ROW, &errinfo);
		if (err)
		{
			IIUGerr(E_RM0109_Err_ins_arc_sup, UG_ERR_ERROR, 2,
				RMtbl->arc_name);
			return (err);
		}
	}

	if (!STequal(dba, RMtbl->table_owner))
	{
		err = grant_sup_to_tblOwner();
		if (err)
			return (err);
	}
	EXEC SQL COMMIT;

	return (OK);
}


static STATUS
grant_sup_to_tblOwner()
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	sql_stmt[128];
	EXEC SQL END DECLARE SECTION;

	STprintf(sql_stmt, ERx("GRANT INSERT ON dd_input_queue TO %s"),
		RMtbl->dlm_table_owner);
	EXEC SQL EXECUTE IMMEDIATE :sql_stmt;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM010A_Err_grant_tbl, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM010A_Err_grant_tbl);
	}

	STprintf(sql_stmt, ERx("GRANT UPDATE, SELECT, INSERT ON %s TO %s"),
		RMtbl->sha_name, RMtbl->dlm_table_owner);
	EXEC SQL EXECUTE IMMEDIATE :sql_stmt;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM010A_Err_grant_tbl, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		return (E_RM010A_Err_grant_tbl);
	}

	if (RMtbl->target_type == TARG_FULL_PEER)
	{
		STprintf(sql_stmt, ERx("GRANT INSERT ON %s TO %s"),
			RMtbl->arc_name, RMtbl->dlm_table_owner);
		EXEC SQL EXECUTE IMMEDIATE :sql_stmt;
		if (RPdb_error_check(0, NULL) != OK)
		{
			IIUGerr(E_RM010A_Err_grant_tbl, UG_ERR_ERROR, 2,
				RMtbl->table_owner, RMtbl->table_name);
			return (E_RM010A_Err_grant_tbl);
		}
	}

	return (OK);
}


STATUS
RMmodify_shadow(
TBLDEF	*tbl,
bool	recreate_index)
{
	STATUS	err;

	err = sql_gen(tbl->table_name, tbl->table_owner, tbl->table_no,
		ERx("mdshadow.tlp"), ERx("m_s"));
	if (err && err != GE_NO_RESOURCE)
	{
		if (err == GE_NO_PRIVILEGE)
		{
			IIUGerr(E_RM00F7_No_grant_permit, UG_ERR_ERROR, 0);
			return (E_RM00F7_No_grant_permit);
		}
		IIUGerr(E_RM0104_Err_modify_shad, UG_ERR_ERROR, 1,
			tbl->sha_name);
		return (E_RM0104_Err_modify_shad);
	}

	if (!recreate_index)
		return (OK);

	err = sql_gen(tbl->table_name, tbl->table_owner, tbl->table_no,
		ERx("ixshadow.tlp"), ERx("i_s"));
	if (err && err != GE_NO_RESOURCE)
	{
		if (err == GE_NO_PRIVILEGE)
		{
			IIUGerr(E_RM00F7_No_grant_permit, UG_ERR_ERROR, 0);
			return (E_RM00F7_No_grant_permit);
		}
		IIUGerr(E_RM0105_Err_index_shad, UG_ERR_ERROR, 1,
			tbl->sha_name);
		return (E_RM0105_Err_index_shad);
	}

	return (OK);
}

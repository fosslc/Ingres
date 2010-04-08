/*
** Copyright (c) 1997, 2009 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <targtype.h>
# include <tblobjs.h>
# include "errm.h"

EXEC SQL INCLUDE <tbldef.sh>;
EXEC SQL INCLUDE <ui.sh>;

/*
** Name:	tblfetch.sc - fetch table information
**
** Description:
**	Defines
**		RMtbl_fetch	- fetch table information
**
** History:
**	09-jan-97 (joea)
**		Created based on tblfetch.sc in replicator library.
**	14-jan-97 (joea)
**		Use defined constants for target types.
**	18-sep-97 (joea)
**		Retrieve base table page size.
**	10-oct-97 (joea) bug 83765
**		If a table exists but the CDDS/database relationship has not
**		been specified, warn the user and mark the table as URO.
**		Initialize RMtbl to NULL and only set it to point to the cached
**		structure if there are no errors.
**	04-may-98 (joea)
**		When checking for table existence, allow zero rows to be
**		treated as no error.
**	27-aug-98 (abbjo03) bug 92717
**		Since SECURITY_LABEL is reported as CHAR in iicolumns.
**		column_datatype, retrieve column_internal_datatype instead.
**	29-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	21-apr-2009 (joea)
**	    Retrieve info from iicolumns to support identity columns.
**      31-dec-2009 (coomi01) b123089
**          Adjust above change to work with older iicolumn catalog.
**/

EXEC SQL BEGIN DECLARE SECTION;
static TBLDEF	tbl;
EXEC SQL END DECLARE SECTION;


GLOBALDEF TBLDEF	*RMtbl = NULL;


/*{
** Name:	RMtbl_fetch - fetch table information
**
** Description:
**	Retrieve information on a table and construct the names of all related
**	objects
**
** Inputs:
**	table_no	- table number
**	force		- force the information to be refreshed
**
** Outputs:
**	none
**
** Returns:
**	OK
**	E_RM00FA_Tbl_not_found
**	E_RM00FD_No_regist_cols
**	E_RM00FE_Err_alloc_col
**	E_RM00FF_Err_col_count
**
** Side effects:
**	The global RMtbl is set to point to the 'tbl' cache.
*/
STATUS
RMtbl_fetch(
i4	table_no,
bool	force)
{
	DBEC_ERR_INFO	errinfo;
	STATUS	err;
	EXEC SQL BEGIN DECLARE SECTION;
	COLDEF	*col_p;
	short	null_ind = 0;
	char	stmt[2048];
	i4	cdds_tbl_exists;
	char	col_always_ident[2];
	char	col_bydefault_ident[2];
	char    cap_value[32];
	EXEC SQL END DECLARE SECTION;

	if (tbl.table_no == table_no && !force)
	{
		RMtbl = &tbl;
		return (OK);
	}

	if (tbl.ncols > 0 && tbl.col_p != NULL)
	{
		MEfree((char *)tbl.col_p);
		tbl.ncols = 0;
	}

	tbl.table_no = table_no;

	/* Populate tbldef based on table_no */
	EXEC SQL REPEATED SELECT TRIM(table_name), TRIM(table_owner),
			cdds_no, columns_registered,
			supp_objs_created, rules_created,
			TRIM(cdds_lookup_table), TRIM(prio_lookup_table),
			TRIM(index_used)
		INTO	:tbl.table_name, :tbl.table_owner,
			:tbl.cdds_no, :tbl.columns_registered,
			:tbl.supp_objs_created, :tbl.rules_created,
			:tbl.cdds_lookup_table, :tbl.prio_lookup_table,
			:tbl.index_used
		FROM	dd_regist_tables
		WHERE	table_no = :tbl.table_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		if (errinfo.errorno)
		{
			return (errinfo.errorno);
		}
		else	/* If table is not found, then return an error */
		{
			IIUGerr(E_RM00FA_Tbl_not_found, UG_ERR_ERROR, 1,
				&tbl.table_no);
			return (E_RM00FA_Tbl_not_found);
		}
	}

	RPedit_name(EDNM_DELIMIT, tbl.table_name, tbl.dlm_table_name);
	RPedit_name(EDNM_DELIMIT, tbl.table_owner, tbl.dlm_table_owner);
	STtrmwhite(tbl.columns_registered);
	STtrmwhite(tbl.supp_objs_created);
	STtrmwhite(tbl.rules_created);

	EXEC SQL SELECT table_pagesize
		INTO	:tbl.page_size
		FROM	iitables
		WHERE	table_name = :tbl.table_name
		AND	table_owner = :tbl.table_owner;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
		return (errinfo.errorno);

	/*
	** If the base table doesn't exist, we're presumably being called from
	** MoveConfig (which still moves everything in sight).  Set target_type
	** to URO and return.
	*/
	if (!errinfo.rowcount)
	{
		tbl.target_type = TARG_UNPROT_READ;
		tbl.ncols = 0;
		tbl.col_p = col_p = NULL;
		RMtbl = &tbl;
		return (OK);
	}

	cdds_tbl_exists = FALSE;
	if (*tbl.cdds_lookup_table != EOS)
	{
		EXEC SQL SELECT COUNT(*)
			INTO	:cdds_tbl_exists
			FROM	iitables
			WHERE	table_name = :tbl.cdds_lookup_table
			AND	table_owner = :tbl.table_owner;
		if (RPdb_error_check(0, &errinfo) != OK)
			return (errinfo.errorno);
	}

	if (cdds_tbl_exists)
	{
		STprintf(stmt,
			ERx("SELECT MIN(target_type) FROM dd_db_cdds \
c, dd_databases d, %s l WHERE c.cdds_no = l.cdds_no AND c.database_no = \
d.database_no AND d.local_db = 1"),
			tbl.cdds_lookup_table);
		EXEC SQL EXECUTE IMMEDIATE :stmt
			INTO	:tbl.target_type:null_ind;
	}
	else
	{
		EXEC SQL SELECT c.target_type INTO :tbl.target_type
			FROM	dd_db_cdds c, dd_databases d
			WHERE	c.cdds_no = :tbl.cdds_no
			AND	c.database_no = d.database_no
			AND	d.local_db = 1;
	}
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
	{
		return (errinfo.errorno);
	}
	else if (errinfo.rowcount == 0 || null_ind == -1)
	{
		/*
		** The table happens to exist in this database but the
		** dd_db_cdds catalog or lookup table don't expect it to
		** be here.  Warn the user and specify the CDDS as URO
		** so we won't create support objects.
		*/
		if (cdds_tbl_exists)
		{
			IIUGerr(E_RM00FB_No_valid_cdds, UG_ERR_ERROR, 2,
				tbl.table_owner, tbl.table_name);
		}
		else
		{
			i4	cdds_no = (i4)tbl.cdds_no;

			IIUGerr(E_RM00FC_Miss_cdds_defn, UG_ERR_ERROR, 1,
				&cdds_no);
		}

		tbl.target_type = TARG_UNPROT_READ;
		RMtbl = &tbl;
		return (OK);
	}

	EXEC SQL SELECT COUNT(*) INTO :tbl.ncols
		FROM	dd_regist_columns
		WHERE	table_no = :tbl.table_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		return (errinfo.errorno);

	if (tbl.ncols == 0)
	{
		IIUGerr(E_RM00FD_No_regist_cols, UG_ERR_ERROR, 2,
			tbl.table_owner, tbl.table_name);
		return (E_RM00FD_No_regist_cols);
	}

	if (err = RPtblobj_name(tbl.table_name, tbl.table_no, TBLOBJ_ARC_TBL,
			tbl.arc_name))
		return (err);

	if (err = RPtblobj_name(tbl.table_name, tbl.table_no, TBLOBJ_SHA_TBL,
			tbl.sha_name))
		return (err);

	if (err = RPtblobj_name(tbl.table_name, tbl.table_no,
			TBLOBJ_REM_DEL_PROC, tbl.rem_del_proc))
		return (err);

	if (err = RPtblobj_name(tbl.table_name, tbl.table_no,
			TBLOBJ_REM_UPD_PROC, tbl.rem_upd_proc))
		return (err);

	if (err = RPtblobj_name(tbl.table_name, tbl.table_no,
			TBLOBJ_REM_INS_PROC, tbl.rem_ins_proc))
		return (err);

	col_p = (COLDEF *)MEreqmem(0, (u_i4)tbl.ncols * sizeof(COLDEF),
		TRUE, (STATUS *)NULL);
	if (col_p == NULL)
	{
		IIUGerr(E_RM00FE_Err_alloc_col, UG_ERR_ERROR, 0);
		return (E_RM00FE_Err_alloc_col);
	}

	tbl.col_p = col_p;

	/*
	** b123089
	** First decide if we will need extra detail from iicolumns 
	*/
	if ( STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_930) >= 0 )
	{
	    /*
	    ** The newer iicolumns catalog have extra details that we would like 
	    */
	    EXEC SQL REPEATED SELECT TRIM(c.column_name), 
		LOWERCASE(TRIM(c.column_internal_datatype)), 
		c.column_length,       c.column_scale, 
		d.column_sequence,     d.key_sequence, 
		c.column_nulls,        c.column_defaults, 
		c.column_always_ident, c.column_bydefault_ident  /** Two Extra Details **/
	    INTO
		:col_p->column_name,
		:col_p->column_datatype,
		:col_p->column_length,   :col_p->column_scale,
		:col_p->column_sequence, :col_p->key_sequence,
		:col_p->column_nulls,    :col_p->column_defaults,
		:col_always_ident,       :col_bydefault_ident
	    FROM   iicolumns c, dd_regist_columns d 
	    WHERE  c.table_name  = :tbl.table_name
	    AND    c.table_owner = :tbl.table_owner
	    AND    c.column_name = d.column_name 
	    AND	   d.table_no    = :tbl.table_no
	    ORDER  BY d.column_sequence;

	    EXEC SQL BEGIN;
	    {
		RPedit_name(EDNM_DELIMIT, col_p->column_name,
			    col_p->dlm_column_name);

		if (*col_always_ident == 'Y')
		    col_p->column_ident = COL_IDENT_ALWAYS;
		else if (*col_bydefault_ident == 'Y')
		    col_p->column_ident = COL_IDENT_BYDEFAULT;
		else
		    col_p->column_ident = COL_IDENT_NONE;
		col_p++;
		
		if ((col_p - tbl.col_p) >= tbl.ncols)
		    EXEC SQL ENDSELECT;
	    }
	    EXEC SQL END;
	}
	else
	{
	    /*
	    ** This SQL is for the older iicolumns catalog, without extra detail
	    */
	    EXEC SQL REPEATED SELECT TRIM(c.column_name), 
		LOWERCASE(TRIM(c.column_internal_datatype)), 
		c.column_length,   c.column_scale, 
		d.column_sequence, d.key_sequence, 
		c.column_nulls,    c.column_defaults
	    INTO
		:col_p->column_name,
		:col_p->column_datatype,
		:col_p->column_length,   :col_p->column_scale,
		:col_p->column_sequence, :col_p->key_sequence,
		:col_p->column_nulls,    :col_p->column_defaults
	    FROM   iicolumns c, dd_regist_columns d 
	    WHERE  c.table_name  = :tbl.table_name
	    AND    c.table_owner =  :tbl.table_owner
	    AND    c.column_name = d.column_name 
	    AND	   d.table_no    = :tbl.table_no
	    ORDER  BY d.column_sequence;

	    EXEC SQL BEGIN;
	    {
		RPedit_name(EDNM_DELIMIT, col_p->column_name,
			    col_p->dlm_column_name);

		/** Set a default value */
		col_p->column_ident = COL_IDENT_NONE;

		col_p++;

		if ((col_p - tbl.col_p) >= tbl.ncols)
		    EXEC SQL ENDSELECT;
	    }
	    EXEC SQL END;
	}

	RMtbl = &tbl;
	return (OK);
}

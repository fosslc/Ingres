/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <pc.h>
# include <ci.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <uigdata.h>
# include <rpu.h>
# include <tbldef.h>
# include "errm.h"

/**
** Name:	ddgensup.sc - create Replicator support objects
**
** Description:
**	Defines
**		main		- build Replicator support objects
**		error_exit	- error exit routine
**
** History:
**	09-jan-97 (joea)
**		Created based on ddgensup.sc in replicator library.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	18-may-98 (padni01) bug 89865
**		Use date_gmt instead of date to set supp_objs_created field 
**		of dd_regist_tables.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Replace string literals
**		with ERget calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries.
**/

/*
PROGRAM =	ddgensup

NEEDLIBS =	REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

GLOBALREF TBLDEF *RMtbl;


static	char	*db_name = NULL;
EXEC SQL BEGIN DECLARE SECTION;
static	i4	tbl_no = 0;
EXEC SQL END DECLARE SECTION;
static	char	*user_name = ERx("");

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	{ERx("table_no"), DB_INT_TYPE, FARG_PROMPT, (PTR)&tbl_no},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	NULL
};


static void error_exit(void);


FUNC_EXTERN STATUS db_config_changed(i2 db_no);
FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);


/*{
** Name:	main - build Replicator support objects
**
** Description:
**	Builds the Replicator shadow and archive tables by calling
**	create_support_tables and database procedures by calling
**	tbl_dbprocs.  This is currently only intended to be called by
**	the remote command utility of VDBA.
**
** Inputs:
**	argc	- the number of arguments on the command line
**	argv	- the command line arguments
**		argv[1]	- the database to connect to
**		argv[2]	- the table for support object creation
**		argv[3] - -uusername, where username is the DBA name
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
i4
main(
i4	argc,
char	*argv[])
{
	STATUS	status;
	EX_CONTEXT	context;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("ddgensup"), args) != OK)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	/* Open the database */
	if (FEningres(NULL, (i4)0, db_name, user_name, NULL) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}
	EXEC SQL SET AUTOCOMMIT OFF;
	EXEC SQL SET_SQL (ERRORTYPE = 'genericerror');

	if (!IIuiIsDBA)
	{
		EXdelete();
		IIUGerr(E_RM0002_Must_be_DBA, UG_ERR_FATAL, 0);
	}

	if (!RMcheck_cat_level())
	{
		EXdelete();
		IIUGerr(E_RM0003_No_rep_catalogs, UG_ERR_FATAL, 0);
	}

	if (RMtbl_fetch(tbl_no, TRUE) != OK)
		error_exit();

	if (*RMtbl->columns_registered == EOS)
	{
		IIUGerr(E_RM0033_Must_reg_before_supp, UG_ERR_ERROR, 0);
		error_exit();
	}

	status = create_support_tables(tbl_no);
	if (status != OK && status != -1)
	{
		IIUGerr(E_RM00E4_Supp_obj_create_fail, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		error_exit();
	}

	status = tbl_dbprocs(tbl_no);
	if (status == -1)
	{
		IIUGerr(E_RM00E5_Proc_up_to_date, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
	}
	else if (status)
	{
		IIUGerr(E_RM00E6_Dbproc_create_fail, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);
		error_exit();
	}

	EXEC SQL UPDATE dd_regist_tables
		SET	supp_objs_created = DATE_GMT('now')
		WHERE	table_no = :tbl_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM0050_Error_updt_regist_tbl, UG_ERR_ERROR, 0);
		error_exit();
	}

	if (db_config_changed(0) != OK)
	{
		EXEC SQL ROLLBACK;
		error_exit();
	}

	EXEC SQL COMMIT;
	FEing_exit();

	IIUGmsg(ERget(F_RM00CF_Supp_obj_create_succ), FALSE, 2,
		RMtbl->table_owner, RMtbl->table_name);

	EXdelete();
	PCexit(OK);
}


static void
error_exit()
{
	FEing_exit();
	EXdelete();
	PCexit(FAIL);
}

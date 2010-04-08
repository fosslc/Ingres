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
# include "errm.h"

# include <si.h>

/**
** Name:	ddmovcfg.sc - move Replicator configuration
**
** Description:
**	Defines
**		main	- move Replicator configuration
**
** History:
**	04-dec-96 (joea)
**		Created based on ddmovcfg.sc in replicator library.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
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
PROGRAM =	ddmovcfg

NEEDLIBS =	REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/


static	char	*db_name = NULL;
EXEC SQL BEGIN DECLARE SECTION;
static	i2	target_db_no;
EXEC SQL END DECLARE SECTION;
static	i4	target_db_nat = 0;
static	char	*user_name = ERx("");

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	{ERx("target_db"), DB_INT_TYPE, FARG_PROMPT, (PTR)&target_db_nat},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	NULL
};


static void error_exit(void);


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);


/*{
** Name:	main - move the Replicator configuration
**
** Description:
**	Moves the configuration by calling move_1_cfg(). This is currently
**	only intended to be called by the remote command utility of VDBA.
**
** Inputs:
**	argc	- the number of arguments on the command line
**	argv	- the command line arguments
**		argv[1]	- the source database name.
**		argv[2]	- the target database number.
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
	EXEC SQL BEGIN DECLARE SECTION;
	char	target_vnode[DB_MAXNAME+1];
	char	target_db_name[DB_MAXNAME+1];
	EXEC SQL END DECLARE SECTION;
	EX_CONTEXT	context;
	DBEC_ERR_INFO	errinfo;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("ddmovcfg"), args) != OK)
		PCexit(FAIL);
	target_db_no = (i2)target_db_nat;

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

	EXEC SQL SELECT vnode_name, database_name
		INTO	:target_vnode, :target_db_name
		FROM	dd_databases
		WHERE	database_no = :target_db_no;
	(void)RPdb_error_check(0, &errinfo);
	if (errinfo.errorno)
	{
		SIprintf("Error %d SELECTing from dd_databases\n",
			errinfo.errorno);
		error_exit();
	}
	if (errinfo.rowcount == 0)
	{
		SIprintf("Invalid database number\n");
		error_exit();
	}
	EXEC SQL COMMIT;

	STtrmwhite(target_vnode);
	STtrmwhite(target_db_name);
	if (move_1_config(target_db_no, target_vnode, target_db_name) != OK)
	{
		SIprintf("Move configuration to '%s::%s' failed\n",
			target_vnode, target_db_name);
		error_exit();
	}

	EXEC SQL COMMIT;
	FEing_exit();

	SIprintf("Configuration moved to '%s::%s'\n", target_vnode,
		target_db_name);

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

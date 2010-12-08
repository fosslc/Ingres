/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
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

/**
** Name:	repcat.sc - load Replicator catalog tables
**
** Description:
**	Defines
**		main		- load Replicator catalog tables
**		error_exit	- exit on error
**
** History:
**	04-dec-96 (joea)
**		Created based on repcat.sc in replicator library.
**	10-jul-97 (joea)
**		Use a single session and SET SESSION AUTHORIZATION instead of
**		dual sessions.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	16-jun-98 (abbjo03)
**		Call individual functions load_tables, modify_tables and
**		create_events.
**	19-oct-98 (abbjo03) bug 93780
**		Take exclusive lock on database.  Accept +w/-w flags. Check if
**		catalogs already exist.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	16-dec-1999 (somsa01)
**	    Changed "wait" to "waitflag" to avoid compilation problems on
**	    platforms such as HP.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries.
**      30-Nov-2010 (horda03) b 124757
**          If "set session with on_error=rollback transaction" is set
**          trying to delete a DBEVENT which doesn't exist will "silently"
**          rollback the transaction. So set the required ON_ERROR handling.
**/

/*
PROGRAM =	repcat

NEEDLIBS =	 REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB ;

UNDEFS =	II_copyright
*/


static	char	*db_name;
static	char	*user_name = ERx("");
static	bool	waitflag = FALSE;
static	bool	nowait = TRUE;

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	{ERx("wait"), DB_BOO_TYPE, FARG_FAIL, (PTR) &waitflag},
	{ERx("nowait"), DB_BOO_TYPE, FARG_FAIL, (PTR) &nowait},
	NULL
};


static void error_exit(void);


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);
FUNC_EXTERN bool RMcheck_cat_level(void);
FUNC_EXTERN STATUS load_tables(void);
FUNC_EXTERN STATUS modify_tables(bool indexes);
FUNC_EXTERN STATUS create_events(void);


/*{
** Name:	main - load Replicator catalog tables
**
** Description:
**	Creates the Replicator catalog tables and dbevents.
**
** Inputs:
**	argc	- the number of arguments on the command line
**	argv	- the command line arguments
**		argv[1]	- the database to connect to
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
	EX_CONTEXT	context;
	char		*dowait;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("repcat"), args) != OK)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	if (waitflag)
		dowait = ERx("+w");
	else
		dowait = ERx("-w");
	/* Open the database */
	if (FEningres(NULL, (i4)0, db_name, user_name, ERx("-l"), dowait,
		NULL) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}
	EXEC SQL SET AUTOCOMMIT OFF;
	EXEC SQL SET_SQL (ERRORTYPE = 'genericerror');
        EXEC SQL SET SESSION WITH ON_ERROR = ROLLBACK STATEMENT;

	if (!IIuiIsDBA)
	{
		EXdelete();
		IIUGerr(E_RM0002_Must_be_DBA, UG_ERR_FATAL, 0);
	}
	if (RMcheck_cat_level())
	{
		EXdelete();
		IIUGerr(E_RM0122_Catalogs_exist, UG_ERR_FATAL, 0);
	}

	IIUGmsg(ERget(S_RM0078_Create_rep_cat_db), FALSE, 1, db_name);
	if (load_tables() != OK)
		error_exit();
	if (modify_tables(TRUE) != OK)
		error_exit();
	if (create_events() != OK)
		error_exit();
	EXEC SQL COMMIT;
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();

	EXEC SQL SET SESSION AUTHORIZATION '$ingres';
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();

	if (update_cat_level() != OK)
		error_exit();

	EXEC SQL COMMIT;
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();

	FEing_exit();

	IIUGmsg(ERget(S_RM007A_Rep_cat_success), FALSE, 1, db_name);
	EXdelete();
	PCexit(OK);
}


/*{
** Name:	error_exit - exit on error
**
** Description:
**	Rollback, disconnect and exit on error.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
error_exit()
{
	EXEC SQL ROLLBACK;
	FEing_exit();

	IIUGerr(E_RM0079_Error_rep_cat_db, UG_ERR_ERROR, 1, db_name);
	EXdelete();
	PCexit(FAIL);
}

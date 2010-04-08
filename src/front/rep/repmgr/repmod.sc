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
** Name:	repmod.sc - modify Replicator catalog tables
**
** Description:
**	Defines
**		main - modify Replicator catalog tables
**
** History:
**	16-dec-96 (joea)
**		Created based on repmod.sc in replicator library.
**      15-jan-97 (hanch04)
**              Added missing NEEDLIBS
**	02-jul-97 (joea)
**		Take exclusive lock on database.  Accept +w/-w flags.  Issue
**		trace DM102 to turn off any replication side effects while
**		modifying the catalogs.
**	10-jul-97 (joea)
**		Remove 2-jul trace messages inadvertently left in.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	16-jun-98 (abbjo03)
**		Add indexes argument to modify_tables.
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
**/

/*
PROGRAM =	repmod

NEEDLIBS =      REPMGRLIB REPCOMNLIB SHFRAMELIB	SHQLIB SHCOMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

EXEC SQL BEGIN DECLARE SECTION;
static	char	*db_name;
EXEC SQL END DECLARE SECTION;
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


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);
FUNC_EXTERN STATUS modify_tables(bool indexes);


/*{
** Name:	main - modify Replicator catalog tables
**
** Description:
**	Remodifies the Replicator catalog tables.
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
	if (IIUGuapParse(argc, argv, ERx("repmod"), args) != OK)
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

	EXEC SQL SET TRACE POINT DM102;
	IIUGmsg(ERget(S_RM007B_Modify_rep_cat_db), FALSE, 1, db_name);
	if (modify_tables(FALSE) != OK)
	{
		EXEC SQL ROLLBACK;
		EXEC SQL SET NOTRACE POINT DM102;
		FEing_exit();

		IIUGerr(E_RM007C_Error_mod_cat_db, UG_ERR_ERROR, 1, db_name);
		EXdelete();
		PCexit(FAIL);
	}

	EXEC SQL COMMIT;
	EXEC SQL SET NOTRACE POINT DM102;
	FEing_exit();

	IIUGmsg(ERget(S_RM007D_Rep_mod_success), FALSE, 1, db_name);
	EXdelete();
	PCexit(OK);
}

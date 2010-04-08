/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<dictutil.h>

/**
** Name:	connect.c -	Connect to DB for dict util programs.
**
** Description:
**	Connect to DB for dictutil upgrade/conversion programs.
**
** History:
**	4/90 (bobm) written.
**	7/91 (pete) Added support for +c flag; WITH clause for Gateways.
**		Bug 38706.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIDDdbcDataBaseConnect.
**
** Description:
**	Performs database connection for dictutil executables.  Note that
**	this is the only use these utilities need to make of their command
**	lines.
**
** History:
**	4/90 (bobm) written.
**	7/90 (mike s) initialize connect args
*/

STATUS
IIDDdbcDataBaseConnect(argc, argv)
i4	argc;
char	**argv;
{
	char	*db;
	char	*xflag = ERx("");
	char	*uflag = ERx("");
	char    *connect = ERx("");	/* +c flag: WITH clause for Gateways */
	ARGRET	rarg;
	i4	pos;

	/*
	** Read the command line arguments.
	*/
	if (FEutaopen(argc, argv, ERx("dictupgrade")) != OK)
		return (FAIL);

	if (FEutaget(ERx("database"), 0, FARG_FAIL, &rarg, &pos) != OK)
		return (FAIL);

	db = rarg.dat.name;

	if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
		uflag = rarg.dat.name;

	if (FEutaget(ERx("equel"), 0, FARG_FAIL, &rarg, &pos) == OK)
		xflag = rarg.dat.name;

	if (FEutaget(ERx("connect"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
		connect = rarg.dat.name;
		IIUIswc_SetWithClause(connect);
	}

	FEutaclose();

	return FEningres((char *) NULL, 0, db, xflag, uflag, (char *) NULL);
}

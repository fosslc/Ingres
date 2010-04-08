/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <lo.h>
# include <er.h>

/**
** Name:	optexist.c - check for existence of runrepl.opt file
**
** Description:
**	Defines
**		RMopt_exists - check for existence of runrepl.opt file
**
** History:
**	16-dec-96 (joea)
**		Created based on optexist.c in replicator library.
**	03-jun-98 (abbjo03)
**		Use RMget_server_dir to prepare server directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN STATUS RMget_server_dir(i2 server_no, LOCATION *loc, char *path);


/*{
** Name:	RMopt_exists - check for existence of runrepl.opt file
**
** Description:
**	Verifies that runrepl.opt file exists for the server passed
**	as an argument.  Returns TRUE if it is there, FALSE otherwise.
**
** Inputs:
**	server_no	- integer id for a server
**
** Outputs:
**	none
**
** Returns:
**	TRUE	- file exists
**	FALSE	- file does not exist or cannot be opened.
**/
bool
RMopt_exists(
i2	server_no)
{
	char		filename[MAX_LOC+1];
	LOCATION	loc;
	LOINFORMATION	loinfo;
	i4		flagword;

	if (RMget_server_dir(server_no, &loc, filename) != OK)
		return (FALSE);
	LOfstfile(ERx("runrepl.opt"), &loc);

	flagword = LO_I_TYPE;
	if (LOinfo(&loc, &flagword, &loinfo) == OK)
		if (loinfo.li_type == LO_IS_FILE)
			return (TRUE);

	return (FALSE);
}

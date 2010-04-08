/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include <er.h>

/**
** Name:	gtsvrdir.c - get server directory
**
** Description:
**	Defines
**		RMget_server_dir	- get server directory
**
** History:
**	30-dec-96 (joea)
**		Created based on getbin.c in replicator library.
**	03-jun-98 (abbjo03)
**		Expand to return a specific server directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safe env variable handling.
**/

static char rservers_dir[MAX_LOC+1] ZERO_FILL;


/*{
** Name:	RMget_server_dir - get server directory
**
** Description:
**	Read the DD_RSERVERS environment (logical) variable and set the
**	RMserver_dir global variable.
**
** Inputs:
**	server_no	- server number
**
** Outputs:
**	loc		- pointer to location descriptor
**	path		- buffer for directory (should be MAX_LOC+1 long)
**
** Returns:
**	OK	- variable/logical defined
*/
STATUS
RMget_server_dir(
i2		server_no,
LOCATION	*loc,
char		*path)
{
	char		*p;
	char		buff[16];
	LOCATION	ingloc;

	if (*rservers_dir == EOS)	/* first time around */
	{
		NMgtAt(ERx("DD_RSERVERS"), &p);
		if (p != NULL && *p != EOS)
		{
			STlcopy(p, rservers_dir, sizeof(rservers_dir)-1);
		}
		else
		{
			/* lookup II_SYSTEM/ingres/rep */
			NMloc(SUBDIR, PATH, ERx("rep"), &ingloc);
			LOtos(&ingloc, &p);
			STcopy(p, rservers_dir);
		}
	}
	STcopy(rservers_dir, path);
	LOfroms(PATH, path, loc);
	LOfaddpath(loc, ERx("servers"), loc);
	STprintf(buff, ERx("server%d"), (i4)server_no);
	LOfaddpath(loc, buff, loc);

	return (OK);
}

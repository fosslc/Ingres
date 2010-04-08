/*
**	Copyright (c) 2004 Ingres Corporation
*/

/* # include's */
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ertu.h"

/**
** Name:	FEspnvar -	Interface for accessing feawn's variables.
**
** Description:
**	Since FEspawn needs to know about the database name, and the -u flag,
**	routines are necessary to set and retrieve these variables, stored
**	here as statics. Callers of FEspawn will need to know about the -e
**	flag, as well. This file defines:
**
**	FEdbname()	returns a pointer to the database name.
**	FEecats()	returns a bool: TRUE if -e is used, FALSE otherwise.
**	FEsetdbname()	sets the database name static.
**	FEsetecats()	sets the empty catalogs (-e flag) static.
**
** History:
**	26-mar-1987	(daver) First Written.
**/

/* # define's */
/* extern's */
/* static's */

static	char	fedbname[256] = ERx("");
static	bool	feecats = FALSE;

/*{
** Name:	FEdbname	- return database name in use
**
** Description:
**	The simply returns a pointer to the static fedbname,
**	where the current database name is kept.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	char * to fedbname.
**
** History:
**	26-mar-1987	(daver) First Written.
*/
char *
FEdbname()
{
	return (fedbname);
}

/*{
** Name:	FEsetdbname	-	sets the current database name value
**
** Description:
**	This copies the string passed in into the static fedbname
**	for safekeeping.
**
** Inputs:
**	dbname		current database in use.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	26-mar-1987	(daver) First Written.
*/
VOID
FEsetdbname(dbname)
char	*dbname;
{
	STcopy(dbname,fedbname);
}

/*{
** Name:	FEecats -	return TRUE if -e flag is used
**
** Description:
**	The simply returns the value of the bool static feecats,
**	which is TRUE if the -e flag is in use and FALSE otherwise.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	boolean value of feecats.
**
** History:
**	27-mar-1987	(daver) First Written.
*/
bool
FEecats()
{
	return (feecats);
}

/*{
** Name:	FEsetecats	-	sets the feecats static for -e flag.
**
** Description:
**	This sets the bool static feecats to the boolean value passed in.
**
** Inputs:
**	eflag		TRUE if -e is used; FALSE otherwise.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	26-mar-1987	(daver) First Written.
*/
VOID
FEsetecats(eflag)
bool	eflag;
{
	feecats = eflag;
}


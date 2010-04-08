/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abclass.h>

/**
** Name:	abfapp.c -	Application Object Routines.
**
** Description:
**	Routines which deal with application structures
**
**	Defines
**		abappprint(app)
*/

/*
**	abappprint
**		Print a application as part of a trace.
**
**	Parameters:
**		app		{APPL *}
**			The application to print.
**
**	Returns:
**		NOTHING
**
**	Called by:
**		various
**
**	Side Effects:
**		NONE
**
**	Trace Flags:
**		NONE
**
**	Error Messages:
**		NONE
**
**	History:
**		Written (jrc)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
abappprint ( app )
register APPL	*app;
{
	abtrcprint(ERx("APPLICATION %s"), app->name, (char *) NULL);
	abtrcprint(ERx("\tstate = 0%o"), *(i4 *)&app->data, (char *) NULL);
	abtrcprint(ERx("\tcreator = %s"), app->owner, (char *)NULL);
	abtrcprint(ERx("\tcreate date = %s"), app->create_date, (char *) NULL);
	abtrcprint(ERx("\tmodify date = %s"), app->alter_date, (char *) NULL);
}

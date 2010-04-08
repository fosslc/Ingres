# include	<compat.h>
# include 	<si.h>
# include 	<equel.h>
# include	<equtils.h>

/*
+* Filename:	sydebug.c
** Purpose:	Define the debugging variable for debug.h and the debugging
**	  	print routine, if desired
**
** Defines:
**	syPrintf	- Like SIprintf, but goes to the dump file.
-*	_db_debug	- Holds the current debug flag bits.
** Notes:
**
** History:
**	11-Feb-85	- Initial version. (mrw)
**	29-jun-87 (bab)	Code cleanup.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


GLOBALREF i4	_db_debug;

/*
+* Procedure:	syPrintf
** Purpose:	Like SIprintf, but goes to the dump file.
** Parameters:
**	fmt	- char *	- Printf format string.
**	a - j	- char *	- Up to 10 arguments to the format string.
** Return Values:
-*	Nothing.
** Notes:
**	Takes a maximum of 10 args (not counting the format string).
**
** Imports modified:
**	<what do we touch that we don't really own>
*/

/* VARARGS */
void
syPrintf( fmt, a, b, c, d, e, f, g, h, i, j )
char	*fmt, *a, *b, *c, *d, *e, *f, *g, *h, *i, *j;
{
    register FILE
	*dp = eq->eq_dumpfile;

    SIfprintf( dp, fmt, a, b, c, d, e, f, g, h, i, j );
    SIflush( dp );
}

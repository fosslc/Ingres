/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	*Sccsid = "@(#)abftypes.h	1.4  2/1/84"; */

/*
** History:
**	23-mar-1992 (mgw)
**		Delete obsolete function definitions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** extern declarations for procedures
** in order to declare their return type, and/or just
** to let routines know they are procedures
*/

i4		abrtscall();
LOCATION	*abfilsrc();

/* VARARGS */
i4		abexeprog();
/* VARARGS */
i4		abproerr();

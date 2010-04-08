/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>

/*
**
+* Filename:	eqcmain.c
** Purpose:	Main program for EQUEL/C preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
** History:
**		12-mar-1996 (thoda04)
**			Added function prototype for eq_main to get rid of
**			compiler warning.  Added a return for main().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-mar-2001 (somsa01)
**	    Updated MING hint for program name to account for different
**	    product builds.
**	29-Sep-2004 (drivi01)
**	    Removed extern before eq_main function, it looks like it was
**	    added there by mistake.
*/

/*

PROGRAM = (PROG0PRFX)eqc

NEEDLIBS = CLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

UNDEFS = tok_keytab yylex yyparse

*/

VOID eq_main(i4 argc, char *argv[]);

i4
main( argc, argv )
i4	argc;
char	*argv[];
{
    eq_main( argc, argv );
    return(0);
}

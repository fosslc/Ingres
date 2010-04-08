/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>

/*
**
+* Filename:	cobsqmain.c
** Purpose:	Main program for ESQL/COBOL preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
**
** History:
**              8/07/92 (larrym) -- Changed UNDEF tok_keytab to tok_optab.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-mar-2001 (somsa01)
**	    Updated MING hint for program name to account for different
**	    product builds.
*/


/*

PROGRAM = (PROG0PRFX)esqlcbl

NEEDLIBS = COBOLSQLIB COBOLLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

UNDEFS = tok_optab yylex yyparse

*/


i4
main( argc, argv )
i4	argc;
char	*argv[];
{
    i4		eq_main();

    return	eq_main( argc, argv );
}

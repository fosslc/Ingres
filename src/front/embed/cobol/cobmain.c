/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>

/*
**
+* Filename:	cobmain.c
** Purpose:	Main program for EQUEL/COBOL preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
**
** History:
**	19-mar-2001 (somsa01)
**	    Updated MING hint for program name to account for different
**	    product builds.
*/


/*

PROGRAM = (PROG0PRFX)eqcbl

NEEDLIBS = COBOLLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

UNDEFS = tok_keytab yylex yyparse

*/


i4
main( argc, argv )
i4	argc;
char	*argv[];
{
    i4		eq_main();

    return	eq_main( argc, argv );
}

# include	<compat.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
+* Filename:	adamain.c
** Purpose:	Main program for EQUEL/ADA preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
*/


/*

PROGRAM = eqa

NEEDLIBS = ADALIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

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

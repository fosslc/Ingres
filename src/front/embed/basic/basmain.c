# include	<compat.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
**
+* Filename:	eqcmain.c
** Purpose:	Main program for EQUEL/C preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
**
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/


/*

PROGRAM = eqb

NEEDLIBS = BASICLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

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

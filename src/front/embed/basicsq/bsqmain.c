# include	<compat.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
**
+* Filename:	bassqmain.c
** Purpose:	Main program for ESQL/BASIC preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
**
** History:
**              8/07/92 (larrym) -- Changed UNDEF tok_keytab to tok_optab.
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/


/*

PROGRAM = esqlb

NEEDLIBS = BASICSQLIB BASICLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

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

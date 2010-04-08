/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<me.h>

/*
**
+* Filename:	eqfmain.c
** Purpose:	Main program for EQUEL/FORTRAN preprocessor.
**
-* Defines:	main	- Entry point (see equel/eqmain.c for description).
**
** History:
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
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

PROGRAM = (PROG0PRFX)eqf

NEEDLIBS = FORTRANLIB EQUELLIB UGLIB FMTLIB AFELIB ADFLIB COMPATLIB

UNDEFS = tok_keytab yylex yyparse

*/


i4
main( argc, argv )
i4	argc;
char	*argv[];
{
    i4		eq_main();

    /* Use the ingres allocator instead of malloc/free default (daveb) */
/*    MEadvise( ME_INGRES_ALLOC ); */	/* Need Unix integration, first */

    return	eq_main( argc, argv );
}

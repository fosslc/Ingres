# ifndef VMS
/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: iigetenv.c -- contains main() of a program which displays the
**	value of a named INGRES environment symbol.
**
**  Usage:
**	iigetenv symbol	
**
**  Description:
**	Calls NMgtAt() to look up the value of a symbol.
**
**  Exit Status:
**	OK	the symbol's value was displayed.
**	FAIL	the symbol is undefined.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	21-oct-96 (mcgem01)
**		Modify usage message so that this executable may be
**		used by Jasmine.
**	26-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of pc.h to eliminate gcc 4.3 warnings.
*/

/*
PROGRAM =	(PROG1PRFX)getenv
**
NEEDLIBS =	COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include	<compat.h>
# include	<nm.h>
# include	<me.h>
# include	<si.h>
# include	<lo.h>
# include	<pc.h>

GLOBALREF LOCATION NMSymloc;
 
main( i4  argc, char **argv )
{
	char *value;

	MEadvise( ME_INGRES_ALLOC );

	if( argc != 2 )
	{
		SIprintf( "\nUsage: %s symbol\n\n", argv[0] );
		PCexit( FAIL );
	}
 
	NMgtAt( argv[ 1 ], &value );

	if( value != NULL && *value != EOS )
		SIprintf( "%s\n", value );
	else
		PCexit( FAIL );
 
	PCexit( OK );
}
# endif /* VMS */

/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <st.h>
# include <me.h>
# include <gv.h>
# include <pc.h>

/*
**  Name: iivertst -- program used by ingbuild to determine whether Unix
**	$PATH includes $II_SYSTEM/ingres/{bin,utility} before attempting
**	to run setup programs. 
**
**  Usage:
**	iivertst release_id
**
**  History:
**	22-feb-94 (tyler)
**		Created to address SIR 58920.
**	26-apr-2000 (somsa01)
**		Appended MING hint to account for different product builds.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
PROGRAM =	(PROG4PRFX)vertst
**
NEEDLIBS =	COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

GLOBALREF char Version[];

main( i4  argc, char **argv )
{
	MEadvise( ME_INGRES_ALLOC );

	if( argc != 2 )
		PCexit( FAIL );

	if( !STequal( Version, argv[ 1 ] ) )
		PCexit( FAIL );

	PCexit( OK );
}

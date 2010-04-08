/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: iisulock.c -- locks the config.dat file during setup.
**
**  Usage:
**	iisulock [ application ]
**
**  Description:
**	Calls lock_config_data() to create a lock for the named application. 
**
**  Exit Status:
**	OK	config.dat was locked successfully
**	FAIL	config.dat is already locked
**
**  History:
**	26-jul-93 (tyler)
**		Created.
**	25-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
*/

/*
PROGRAM =	(PROG1PRFX)sulock
**
NEEDLIBS =	UTILLIB COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <me.h>
# include <pc.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <util.h>

main( argc, argv )
int	argc;
char	*argv[];
{
	char *application = NULL;

	MEadvise( ME_INGRES_ALLOC );

	switch( argc )
	{
		case 1:
			break;

		case 2:
			application = argv[ 1 ];
			break;
		default:
			SIprintf( ERx( "\nUsage: iisulock [ application ]\n\n" ) );
			PCexit( FAIL );
	}

	if( lock_config_data( application ) != OK )
		PCexit( FAIL ); 

	PCexit( OK );
}

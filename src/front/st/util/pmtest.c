/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: pmtest.c -- simple test program for (some) PM functions.
**
**  Description:
**
**	Usage: pmtest [ pmfile [ restriction ] ]
**
**	pmtest loads the contents of the named PM data file, optionally
**	restricts the name space being loaded, and recognizes the
**	following commands:	
**
**	command		description
**	-------		-----------
**	C		clear a default name component
**	D		delete a resource
**	E		erase all resources and reinitialize
**	F		set a default name component
**	G		display a resource value
**	H		display help text	
**	I		insert a resource
**	L		load a resource file	
**	N		display the number of components in a resource name
**	P		display all resources that match a PM pattern
**	Q		quit this program
**	R		display all resources that match egrep regexp
**	W		write all resources to a resource file 
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	19-jan-93 (tyler)
**		Put UTILLIB in NEEDLIBS before COMPATLIB to pick up
**		development version of pm.c (being built in UTILLIB).
**	11-feb-93 (tyler)
**		Fixed on-line documentation for PM expression test
**		command.
**	21-jun-93 (tyler)
**		Changed STlcopy() call to STcopy() since the former was
**		not intended and the missing argument to STlcopy() caused 
**		pmtest not to work on VMS (and other systems surely).
**		Made pmfile argument optional (config.dat gets loaded
**		if not specified) and added optional load restriction
**		argument to test PMrestrict().
**	23-jul-93 (tyler)
**		Fixed error reporting.  Moved display_error() function
**		to util.c (renamed to PMerror()). 
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	23-jan-98 (mcgem01)
**		Changed STlcopy() call to STcopy() since the former was
**		not intended and the missing argument to STlcopy() caused
**		pmtest not to compile.
**	02-jul-99 (toumi01)
**		Change getline to iigetline to avoid conflict with C lib
**		definition in Linux glibc 2.1.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include <compat.h>
# include <me.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <pc.h>
# include <er.h>
# include <cm.h>
# include <cv.h>
# include <pm.h>
# include <util.h>
# include <pmutil.h>

/*
PROGRAM = 	pmtest
**
NEEDLIBS = 	UTILLIB COMPATLIB MALLOCLIB 
**
UNDEFS =	II_copyright
*/

# define F_PRINT( F_MSG, ARG )	SIprintf( ERx( F_MSG ), ARG );

# define PRINT( MSG ) SIprintf( ERx( MSG ) ); 

static void
show_help( void )
{
	PRINT( "\n" );
	PRINT( "C - Clear a default name component\n" );
	PRINT( "D - Delete a resource\n" );
	PRINT( "E - Erase all resources and reinitialize\n" );
	PRINT( "F - set a deFault name component\n" );
	PRINT( "G - Get (and display) the value of a resource\n" );
	PRINT( "H - display this Help text\n" );
	PRINT( "I - Insert a new resource\n" );
	PRINT( "L - Load a resource file\n" );
	PRINT( "N - count the Number of components in a name\n" );
	PRINT( "P - display resources which match a PM expression\n" );
	PRINT( "R - display resources which match a Regular expression\n" );
	PRINT( "Q - Quit this program\n" );
	PRINT( "W - Write a resource file\n" );
}

static void
iigetline( char *buf, i4  len )
{
	SIgetrec( buf, len, stdin );
	buf[ STlength( buf ) - 1 ] = EOS;
}

main( argc, argv )
int argc;
char *argv[];
{
	char cmd[ BIG_ENOUGH ];
	char param[ BIG_ENOUGH ];
	char *rptr, *cptr, *name, *value, *pmfile, *restriction;
	LOCATION pm_loc;
	char loc_buf[ MAX_LOC + 1 ];
	STATUS status;

	MEadvise( ME_INGRES_ALLOC );

	PMinit();

	PMlowerOn();

	switch( argc )
	{
		case 1:
			pmfile = NULL;
			restriction = NULL;
			break;

		case 2:
			pmfile = argv[ 1 ];
			restriction = NULL;
			break;

		case 3:
			pmfile = argv[ 1 ];
			restriction = PMexpToRegExp( argv[ 2 ] );
			break;

		default:
			PRINT( "\nUsage: pmtest [ pmfile [ restriction ] ]\n\n" );
			PCexit( FAIL );
	}

	if( PMrestrict( restriction ) != OK )
	{
		PRINT( "\nBad restriction.\n\n" );
		PCexit( FAIL );
	}

	if( pmfile != NULL )
	{
		STcopy( pmfile, loc_buf );
		LOfroms( FILENAME, loc_buf, &pm_loc );
		status = PMload( &pm_loc, PMerror );
	}
	else
		status = PMload( (LOCATION *) NULL, PMerror );

	switch( status )
	{
		case OK:
			break;

		default:
			PCexit( FAIL );
	}

	PRINT( "\nWelcome to the PM test program.\n" );

	show_help();

	for( ;; )
	{
		PRINT( "\nPM> ");
		iigetline( cmd, sizeof( cmd ) );
		PRINT( "\n" );
		for( cptr = &cmd[ 1 ]; *cptr != EOS && CMwhite( cptr );
			CMnext( cptr ) );
		CMtolower( cmd, cmd );
		switch( *cmd )
		{

		case 'C':
		case 'c':
			{
			i4 idx;

			/* clear a default component name */
			if( *cptr == EOS )
			{
				PRINT( "Enter index of component default to clear: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			CVan( cptr, &idx );
			PMsetDefault( idx, NULL );
			}
			break;

		case 'E':
		case 'e':
			PMfree();
			PMinit();
			PMlowerOn();
			PRINT("All resources erased.\n");
			break;

		case 'D':
		case 'd':
			/* delete a resource from the set stored in memory */
			if( *cptr == EOS )
			{
				PRINT("Delete parameter: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			if( PMdelete( cptr ) != OK )
				PRINT("Unable to delete named parameter.\n");
			break;

		case 'F':
		case 'f':
			/* set a default */
			{
			i4 idx;
			char *dflt;

			/* get the value of a resource */
			if( *cptr == EOS )
			{
				PRINT( "Enter index of component: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			CVan( cptr, &idx );
			F_PRINT( "Enter default value for component %d: ",
				idx );
			iigetline( dflt = param, sizeof( param ) );

			if( *dflt == EOS )
				PMsetDefault( idx, NULL );
			else
				PMsetDefault( idx, dflt );	
			}
			break;

		case 'G': 
		case 'g': 
			/* get the value of a resource */
			if( *cptr == EOS )
			{
				PRINT( "Get parameter: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			if( PMget( cptr, &value ) == OK )
			{
				F_PRINT( "%s: ", cptr );
				F_PRINT( "%s\n", value );
			}
			else
				F_PRINT( "Not found: %s\n", cptr );
			break;

		case 'H':
		case 'h':
			show_help();
			break;

		case 'I':
		case 'i':
			/* insert resource to the set stored in memory */
			if( *cptr == EOS )
			{
				PRINT( "Insert parameter: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			PRINT("Value: ");
			iigetline( param, sizeof( param ) );
			PRINT( "\n" );
			if( PMinsert( cptr, param ) != OK ) 
				PRINT( "Unable to add parameter.\n");
			break;

		case 'L':
		case 'l':
			/* load resource file */ 
			if( *cptr == EOS )
			{
				PRINT( "Load PM file: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			STcopy( cptr, loc_buf );
			LOfroms( FILENAME, loc_buf, &pm_loc );
			if( PMload( &pm_loc, PMerror ) != OK )
				PRINT("Unable to load resource file.\n");
			PRINT( "PM file loaded.\n");
			break;

		case 'N':
		case 'n':
			/* count the number of elements in a resource name */
			PRINT("Count elements in symbol: ");
			iigetline( cmd, sizeof( cmd ) );
			PRINT( "\n" );
			F_PRINT( "number of elements=%d\n", PMnumElem( cmd ) );

		case 'P': 
		case 'p': 
			/* display resources which match a PM expression */
		{
			PM_SCAN_REC state;

			if( *cptr == EOS )
			{
				PRINT( "PM expression: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			cptr = PMexpToRegExp( cptr );
			if( PMscan( cptr, &state, NULL, &name, &value )
				== OK
			) { 
				F_PRINT( "%s	", name );
				F_PRINT( "%s\n", value );
			}
			while( PMscan( NULL, &state, NULL, &name, &value )
				== OK
			) {
				F_PRINT( "%s	", name );
				F_PRINT( "%s\n", value );
			}
			break;
		}

		case 'Q':
		case 'q':
			/* quit test program */
			PCexit( OK );

		case 'R': 
		case 'r': 
			/* display resources which match a regular expression */
		{
			PM_SCAN_REC state;

			if( *cptr == EOS )
			{
				PRINT( "Regular expression: ");
				iigetline( cptr = cmd, sizeof( cmd ) );
				PRINT( "\n" );
			}
			if( PMscan( cptr, &state, NULL, &name, &value )
				== OK
			) {
				F_PRINT( "%s	", name );
				F_PRINT( "%s\n", value );
			}
			while( PMscan( NULL, &state, NULL, &name, &value )
				== OK
			) {
				F_PRINT( "%s	", name );
				F_PRINT( "%s\n", value );
			}
			break;
		}

		case 'W':
		case 'w':
			/* write data file */
			if( pmfile == NULL )
				PMwrite( (LOCATION *) NULL );
			else
				PMwrite( &pm_loc ); 
			PRINT( "Wrote parameter file.\n" );
			break;

		default:
			break;
		}
	}
}


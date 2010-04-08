/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Name: iivalres.c -- contains main() for a program which checks 
**	a configuration resource value for rule system constraint
**	violation. 
**
**  Usage:
**	iivalres [ -v ] resource value [ rule_map ]
**
**  Description:
**	iivalres checks for rules constraint violations by calling 
**	CRvalidate();
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	24-may-93 (tyler)
**		Added argument to PMmInit() call.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.  Changed fourth argument in CRvalidate() call
**		to a file pointer.
**	23-jul-93 (tyler)
**		PMmInit() interface changed.  PMerror() is now passed
**		passed to PMmLoad() for better error reporting.  Replaced
**		embedded strings with message lookups.  Call to
**		PMmLowerOn() removed. 
**	19-oct-93 (tyler)
**		CR_NO_RULES_ERR status (returned by CRload()) is now
**		handled.  Modified other error status symbols.
**      07-mar-1996 (canor01)
**              Return null function to pathname verification routine.
**      21-oct-96 (mcgem01)
**              Modify usage message so that executable may be renamed
**      21-apr-1999 (hanch04)
**          Added st.h
**	21-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.  Removed MALLOCLIB from NEEDLIBS
*/

/*
PROGRAM =	(PROG1PRFX)valres
**
NEEDLIBS =	CONFIGLIB SHEMBEDLIB UTILLIB COMPATLIB 
**
UNDEFS =	II_copyright
**
DEST = 		utility
*/

# include <compat.h>
# include <me.h>
# include <pc.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <pmutil.h>
# include <si.h>
# include <st.h>
# include <simacro.h>
# include <util.h>
# include <erst.h>
# include "cr.h"


main( int argc, char **argv )
{
	PM_CONTEXT *pm_id;
	i4 info, i;
	char *rulemap, *name, *value, *constraint;
	bool usage = TRUE;
	FILE *output = (FILE *) NULL;

	MEadvise( ME_INGRES_ALLOC );

	if( argc == 3 )
	{
		usage = FALSE;
		name = argv[ 1 ];
		value = argv[ 2 ];
		rulemap = NULL;
	}
	else if( argc == 4 )
	{
		usage = FALSE;
		if( STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 )
		{
			output = stdout;
			name = argv[ 2 ];
			value = argv[ 3 ];
			rulemap = NULL;
		}
		else	
		{
			rulemap = NULL;
			name = argv[ 1 ];
			value = argv[ 2 ];
			rulemap = argv[ 3 ];
		}
	}
	else if( argc == 5 && STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 )
	{
		output = stdout;
		usage = FALSE;
		name = argv[ 2 ];
		value = argv[ 3 ];
		rulemap = argv[ 4 ];
	}
	if( usage )
	{
		SIprintf(ERx( "\nUsage: %s [ -v ] name value [ rule_map ]\n\n"),
				argv[0] );
		PCexit( FAIL );
	}

	(void) PMmInit( &pm_id );

	if( PMmLoad( pm_id, NULL, PMerror ) != OK )
		PCexit( FAIL );

	CRinit( pm_id );

	switch( CRload( rulemap, &info ) )
	{
		case OK:
			break;

		case CR_NO_RULES_ERR:
			SIfprintf( stderr, ERget( S_ST0418_no_rules_found ) );
			PCexit( FAIL );

		case CR_RULE_LIMIT_ERR:
			SIfprintf( stderr,
				ERget( S_ST0416_rule_limit_exceeded ), info );
			PCexit( FAIL );

		case CR_RECURSION_ERR:
			SIfprintf( stderr,
				ERget( S_ST0417_recursive_definition ),
				CRsymbol( info ) );
			PCexit( FAIL );
	}
	if( CRvalidate( name, value, &constraint, output ) != OK )
		PCexit( FAIL );

	PCexit( OK );
}

/* Return null function to pathname verification routine. */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
        return NULL;
}

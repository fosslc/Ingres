/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
**
**  Name: iiremres.c -- contains main() for a program which deletes 
**	a configuration resource and recalculates derived resources. 
**
**  Usage:
**	iiremres [ -v ] resource 
**
**  Description:
**	iiremres deletes the named resource and recalculates the value
**	of all derived resources by calling CRsetPMval().
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	24-may-93 (tyler)
**		Added argument to PMmInit() call.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.  Changed third argument in CRsetPMval() call
**		to a file pointer.
**	23-jul-93 (tyler)
**		Added argument to CRsetPMval() call.  PMmInit()
**		interface changed.  PMerror() is now passed to
**		PMmLoad() for better error reporting.  Replace embedded
**		strings with message lookups.  Resources are now
**		written to config.dat in lower case on all platforms. 
**	19-oct-93 (tyler)
**		CR_NO_RULES_ERR status (returned by CRload()) is now
**		handled.  Modified other error status symbols.
**	01-dec-93 (tyler)
**		Report an error if unable to write config.dat.
**      07-mar-1996 (canor01)
**              Return null function to pathname verification routine.
**	21-oct-96 (mcgem01)
**		Modify usage message so that this executable may be
**		used by Jasmine.
**      21-apr-1999 (hanch04)
**	    Added st.h
**	21-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-apr-2003 (hanch04)
**          Pass the config.log to CRsetPMval and new flag to pass
**          for printing to stdout if verbose is set.
**          This change fixes bug 100206.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries. Removed MALLOCLIB from NEEDLIBS
**	25-Jul-2007 (drivi01)
**	    Added additional routine to print additional error messaging
**	    on Vista when this program fails to open config.log.
**	    Also added a check to avoid potential SEGV, do not close
**	    file pointer if it wasn't open.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
PROGRAM =	(PROG1PRFX)remres
**
NEEDLIBS =	CONFIGLIB UTILLIB COMPATLIB SHEMBEDLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <me.h>
# include <pc.h>
# include <ep.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <pmutil.h>
# include <si.h>
# include <st.h>
# include <nm.h>
# include <simacro.h>
# include <util.h>
# include <erst.h>
# include "cr.h"

GLOBALDEF char		change_log_buf[ MAX_LOC + 1 ];	
GLOBALDEF FILE          *change_log_fp;
GLOBALDEF LOCATION      change_log_file;


main( int argc, char **argv )
{
	PM_CONTEXT *pm_id;
	i4 info;
	char *name;
	bool usage = TRUE;
	bool verbose = FALSE;
	FILE *output = (FILE *) NULL;
	bool tee_stdout = FALSE;


	MEadvise( ME_INGRES_ALLOC );

	if( argc == 2 )
	{
		usage = FALSE;
		name = argv[ 1 ];
	}
	else if ( argc == 3 && STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 )
	{
		verbose = TRUE;
		tee_stdout = TRUE;
		usage = FALSE;
		name = argv[ 2 ];
	}
	if( usage )
	{
		SIprintf( ERx( "\nUsage: %s [ -v ] name\n\n" ), argv[0] );
		PCexit( FAIL );
	}

	(void) PMmInit( &pm_id );

	PMmLowerOn( pm_id );

	if( PMmLoad( pm_id, NULL, PMerror ) != OK )
		PCexit( FAIL );

	CRinit( pm_id );

	switch( CRload( NULL, &info ) )
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
        /* prepare LOCATION for config.log */
        NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file );

        /* open change log file for appending */
        if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
                &change_log_fp ) != OK )
        {
	    /* 
	    ** Check if this is Vista, and if user has sufficient privileges 
	    ** to execute.
	    */
	    ELEVATION_REQUIRED_WARNING();
            SIfprintf( stderr,
            ERget( S_ST0323_unable_to_open), change_log_buf );
        }

	CRsetPMval( name, NULL, change_log_fp, tee_stdout, TRUE );

        /* close change log after appending */
	if (change_log_fp)
        (void) SIclose( change_log_fp );

	if( PMmWrite( pm_id, NULL ) != OK )
	{
		SIfprintf( stderr, ERx( "\n%s\n\n" ),
			ERget( S_ST0314_bad_config_write ) );
		PCexit( FAIL );
	}
	PCexit( OK );
}

/* Return null function to pathname verification routine. */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
	return NULL;
}

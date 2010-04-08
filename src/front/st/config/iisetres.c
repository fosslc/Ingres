/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
**
**  Name: iisetres.c -- contains main() for a program which sets 
**	a configuration resource and recalculates derived resources. 
**
**  Usage:
**	iisetres [ -v ] name value
**
**  Description:
**	iisetres sets the named resource and recalculates the value of
**	all derived resources by calling CRsetPMval().
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-93 (tyler)
**		Added -v option to allow default mode to be silent.
**	24-may-93 (tyler)
**		Added argument to PMmInit() call.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.  Changed third argument in CRsetPMval() call
**		a file pointer.
**	23-jul-93 (tyler)
**		Added parameter to CRsetPMval() call.  PMmInit() interface
**		changed.  PMerror() is now passed to PMmLoad() for
**		better error reporting.  Replaced embedded string with
**		message lookups.  Resources are now written to config.dat
**		in lower case on all platforms. 
**	19-oct-93 (tyler)
**		CR_NO_RULES_ERR status (returned by CRload()) is now
**		handled.  Modified other error status symbols.
**	01-dec-93 (tyler)
**		Report an error if unable to write config.dat.
**      07-mar-1996 (canor01)
**              Return null function to pathname verification routine.
**	21-oct-96 (mcgem01)
**		Modify usage message so that executable may be renamed
**		for Jasmine.
**      29-Aug-97 (hanal04)
**              Read/Write the config.dat file in II_CONFIG_LOCAL if       
**              II_CONFIG_LOCAL is set (Re: b78563).
**      21-Jul-97 (hanal04)
**              Reworked nfs client fix in iisunet. Undo the previous
**              change to this file. b91480.
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
**      15-apr-2003 (hanch04)
**          Pass the config.log to CRsetPMval and new flag to pass
**          for printing to stdout if verbose is set.
**          This change fixes bug 100206.
**	22-may-2003 (drivi01)
**	    Implementation of a DAR #110244, added -p/+p functionality to 
**	    iisetres command.
**	25-jun-2003 (drivi01)
**	    Adjustment to the above implementation of a DAR #110244,
**	    when iisetres is ran with no parameters on Unix it throws
**	    SEGV error.
**	04-Aug-2003 (drivi01)
**	    Formatted comments.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries. Removed MALLOCLIB from NEEDLIBS
**	25-Jul-2007 (drivi01)
**	    Added additional routine to print additional error messaging
**	    on Vista when this program fails to open config.log.
**	    Also, added a check to avoid potential SEGV, do not close
**	    file pointer if it wasn't open.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
PROGRAM =	(PROG1PRFX)setres
**
NEEDLIBS =	CONFIGLIB UTILLIB COMPATLIB SHEMBEDLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <me.h>
# include <ep.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <pmutil.h>
# include <pc.h>
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

static LOCATION		protect_file;
static PM_CONTEXT	*protect_data;


main( int argc, char **argv )
{
	PM_CONTEXT *pm_id;
	i4 info, i;
	char *name, *value;
	char *host, *rulemap;
	bool usage = TRUE;
	FILE *output = (FILE *) NULL;
	bool tee_stdout = FALSE;
	char protecting[4]={'\0'};
	char protect_buf[MAX_LOC + 1];
	char *orig_val;
	STATUS config_status=-1;
	MEadvise( ME_INGRES_ALLOC );

	if( argc == 3 && STcompare( argv[ 1 ], ERx( "-v" ) ) != 0 && 
		STcompare( argv[ 1 ], ERx( "-p" ) ) != 0 && 
		STcompare( argv[ 1 ], ERx( "+p" ) ) != 0)
	{
		usage = FALSE;
		name = argv[ 1 ];
		value = argv[ 2 ];
	}
	else if (argc == 3 && (STcompare( argv[ 1 ], ERx( "-p" ) ) == 0 || 
			STcompare( argv[ 1 ], ERx( "+p" ) ) == 0))
	{
		usage = FALSE;
		if (STcompare( argv[ 1 ], ERx( "-p" ) ) == 0)
			STprintf(protecting,"%s", ERget( S_ST0331_no ));
		else
			STprintf(protecting,"%s", ERget( S_ST0330_yes ));
		name = argv[ 2 ];
		value = NULL;
	}
	else if (argc == 4 && (STcompare( argv[1], ERx( "-p" ) )==0 || 
			STcompare( argv[1], ERx( "+p" ) )==0) &&
			STcompare( argv[2], ERx( "-v" ) )!=0)
	{

		if (STcompare( argv[ 1 ], ERx( "-p" ) ) == 0)
			STprintf(protecting,"%s", ERget( S_ST0331_no ));
		else
			STprintf(protecting,"%s", ERget( S_ST0330_yes ));
		usage = FALSE;
		name = argv[2];
		value = argv[3];
		
	}
	else if (argc == 4 && (STcompare( argv[2], ERx( "-p" )) ==0 ||
			STcompare( argv[2], ERx( "+p" ) )==0) &&
			STcompare( argv[1], ERx( "-v") )==0)
	{
		if (STcompare( argv[ 2 ], ERx( "-p" ) ) == 0)
			STprintf(protecting,"%s", ERget( S_ST0331_no ));
		else
			STprintf(protecting,"%s", ERget( S_ST0330_yes ));
		tee_stdout = TRUE;
		usage = FALSE;
		name = argv[3];
		value = NULL;

	}
	else if ( argc == 4 && STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 
			&& (STcompare( argv[ 2 ], ERx( "-p" ) ) != 0 &&
			STcompare( argv[ 2 ], ERx( "+p" ) ) != 0 ) )
	{
		tee_stdout = TRUE;
		usage = FALSE;
		name = argv[ 2 ];
		value = argv[ 3 ];
	}
	else if (argc == 4 && (STcompare( argv[2], ERx( "-p" )) ==0 ||
			STcompare( argv[2], ERx( "+p" ) )==0) &&
			STcompare( argv[1], ERx( "-v") )==0)
	{
		if (STcompare( argv[ 1 ], ERx( "-p" ) ) == 0)
			STprintf(protecting,"%s", ERget( S_ST0331_no ));
		else
			STprintf(protecting,"%s", ERget( S_ST0330_yes ));
		tee_stdout = TRUE;
		usage = FALSE;
		name = argv[3];
		value = NULL;

	}
	else if ( argc == 5 && (STcompare( argv[ 2 ], ERx( "-p" ) ) == 0 || 
			STcompare( argv[ 2 ], ERx( "+p" ) ) == 0) && 
			STcompare( argv[ 1 ], ERx( "-v" ) ) == 0)
	{
		if (STcompare( argv[ 2 ], ERx( "-p" ) ) == 0)
			STprintf(protecting,"%s", ERget(S_ST0331_no ));
		else
			STprintf(protecting,"%s", ERget( S_ST0330_yes ));
		tee_stdout = TRUE;
		usage = FALSE;
		name = argv[3];
		value = argv[4];
	}
	if( usage )
	{
		SIprintf( ERx( "\nUsage: %s [ -v ] [-p|+p] name [value]\n\n" ), argv[0]);
		PCexit( FAIL );
	}

	/*
	** Implementation of -p/+p flag which (un)protects parameter
	** from automatic adjustments.
	*/
	if (*protecting != NULL)
	{
		
		NMloc(ADMIN, FILENAME, ERx ( "protect.dat" ), &protect_file);
		LOcopy( &protect_file, protect_buf, &protect_file);
		PMmInit(&protect_data);
		PMmLowerOn(protect_data);
		PMmLoad(protect_data, &protect_file, NULL);
		if (STcompare( protecting, ERget( S_ST0330_yes ) ) == 0)
		{
			if (PMmGet(protect_data, name, &orig_val) == OK)
				PMmDelete(protect_data, name);

			/*
			** Open config.dat to get the value of the variable.
			*/
			(void) PMmInit(&pm_id);
			PMmLowerOn( pm_id );
			if (PMmLoad( pm_id, NULL, PMerror ) !=OK)
				PCexit(FAIL);
			if (PMmGet(pm_id, name, &orig_val) !=OK)
				orig_val = ERx( "0" );


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
			config_status=NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file );


			/* 
			** Check if the parameter is derived or not because only 
			** derived parameters can be protected.
			*/
			if (CRderived(name))
			{
				if (PMmInsert(protect_data, name, orig_val)==OK)
				{
					char temp[BIG_ENOUGH];
					PMmWrite(protect_data, &protect_file);
					if (tee_stdout)
						SIfprintf(stdout, ERx("\n%s is now protected\n\n"), name);
				}
			}
			else
				if (tee_stdout)
					SIfprintf(stderr, ERget( S_ST0682_not_derived_resource ), name );


		}
		else
		{
			if (PMmGet(protect_data, name, &orig_val)==OK)
				if (PMmDelete(protect_data, name)==OK && tee_stdout)
					SIfprintf(stdout, ERx("\n%s is NOT protected\n\n"), name);

			PMmWrite(protect_data, &protect_file);
		}

	}

	/*
	** If iisetres is used to protect a parameter only value doesn't 
	** have to be set and the rest of the code doesn't need to be executed.
	*/
	if (value != NULL)
	{
		/* If one of the parameters has been protected
		** chances are we don't need to load config.dat
		** b/c it had been already done.
		*/
		if (config_status != OK)
		{
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
		}
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

		CRsetPMval( name, value, change_log_fp, tee_stdout, TRUE );

        /* close change log after appending */
	if (change_log_fp)
        (void) SIclose( change_log_fp );

        if( PMmWrite( pm_id, NULL ) != OK )
          {
           SIfprintf( stderr, ERx( "\n%s\n\n" ),
           ERget( S_ST0314_bad_config_write ) );
           PCexit( FAIL );
          }
	}
	PCexit( OK );
}

/* Return null function to pathname verification routine. */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
	return NULL;
}

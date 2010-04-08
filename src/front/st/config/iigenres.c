/*
**  Copyright (c) 1992, 2010 Ingres Corporation
*/

/*
**  Name: iigenres.c -- contains main() for a program which generates
**	a default configuration for an INGRES installation.
**
**  Usage:
**	iigenres [ -v ] host | rule_map | host rule_map
**
**  Description:
**	If a hostname is passed, the configuration generated will be
**	for that host.
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
**		functions.
**	23-jul-93 (tyler)
**		PMmInit() interface changed.  PMerror() is now passed
**		to PMmLoad() for better error reporting.  Replaced
**		embedded strings with message lookups.  Resources 
**		are now written to config.dat in lower case on all
**		platforms. 
**	25-aug-93 (fredv)
**		Included <st.h>
**	19-oct-93 (tyler)
**		CR_NO_RULES_ERR status (returned by CRload()) is now
**		handled.  Modified other error status symbols.  Force
**		recalculation of derived resources.  Changed "dmf" to
**		"dbms" in dbms cache resources names and added minor
**		optimization to resources generation.
**	22-nov-93 (tyler)
**		Fixed bug which caused generated gcc protocol resources
**		to have the instance name "private".  Fixed previous
**		fix to force recalculation of derived resources.
**	07-mar-1996 (canor01)
**		Return null function to pathname verification routine.
**	06-jun-1996 (thaju02)
**	  	Add buffer cache parameters to default configuration for
**		variable page size support.
**	19-oct-96 (mcgem01)
**		Modify usage message so that the exe may be renamed
**		for Jasmine.
**      29-Aug-97 (hanal04)
**              Read/Write the config.dat file in II_CONFIG_LOCAL if     
**              II_CONFIG_LOCAL is set (Re: b78563).
**      21-Jul-97 (hanal04)
**              Reworked nfs client fix in iisunet. Undo the previous
**              change to this file. b91480.
**	21-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-nov-02 (chash01)
**              qualify RMS gateway for private caches
**      15-apr-2003 (hanch04)
**	    Pass the config.log to CRsetPMval and new flag to pass
**	    for printing to stdout if verbose is set.
**          This change fixes bug 100206.
**      11-sep-2003 (chash01)
**          add "private" into rms gateway parameters if they are 
**          ii.nodename.rms.*.*.dmfxxxxx
**	19-feb-2004 (somsa01)
**	    Updated syntax for command.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHEMBEDLIB to replace
**	    its static libraries.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
PROGRAM =	(PROG1PRFX)genres
**
NEEDLIBS =	CONFIGLIB UTILLIB SHEMBEDLIB COMPATLIB 
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <me.h>
# include <er.h>
# include <pc.h>
# include <si.h>
# include <st.h>
# include <lo.h>
# include <pm.h>
# include <nm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include <erst.h>
# include "cr.h"

bool verbose = FALSE;

GLOBALDEF char		change_log_buf[ MAX_LOC + 1 ];	
GLOBALDEF FILE          *change_log_fp;
GLOBALDEF LOCATION      change_log_file;

#define CRF_FPRINT( FP, F_MSG, ARG ) \
        { \
		F_FPRINT( FP, F_MSG, ARG ); \
		if( verbose ) \
                    F_FPRINT( stdout, F_MSG, ARG ); \
        }
#define CRFPRINT( FP, MSG ) \
        { \
                FPRINT( FP, MSG, ); \
		if( verbose ) \
                    FPRINT( stdout, MSG, ); \
        }
        

main( int argc, char **argv )
{
	i4 *resources, n, i, info;
	STATUS status;
	PM_CONTEXT *pm_id;
	char *host, *rulemap;
	bool usage = TRUE;

	MEadvise( ME_INGRES_ALLOC );
#ifdef DONTKNOW
/* does this make sense?  i.e. has II_CHARSET been installed yet? */
	/* Add call to IIUGinit to initialize character set table and others */
	if (IIUGinit() != OK)
	{
		PCexit(FAIL);
	}
#endif


	if( argc == 2 && STcompare( argv[ 1 ], ERx( "-v" ) ) != 0 )
	{
		usage = FALSE;
		host = argv[ 1 ];
		rulemap = NULL;
	}
	else if( argc == 3 )
	{
		usage = FALSE;
		if( STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 )
		{
			verbose = TRUE;
			host = argv[ 2 ];
			rulemap = NULL;
		}
		else	
		{
			host = argv[ 1 ];
			rulemap = argv[ 2 ];
		}
	}
	else if( argc == 4 && STcompare( argv[ 1 ], ERx( "-v" ) ) == 0 )
	{
		usage = FALSE;
		verbose = TRUE;
		host = argv[ 2 ];
		rulemap = argv[ 3 ];
	}
	if( usage )
	{
		SIprintf(
		  ERx("\nUsage: %s [ -v ] host | rule_map | host rule_map\n\n"),
			argv[0] );
		PCexit( FAIL );
	}

	(void) PMmInit( &pm_id );

	PMmLowerOn( pm_id );

        if( PMmLoad( pm_id, NULL, PMerror ) != OK )
	     PCexit( FAIL );

	PMmSetDefault( pm_id, HOST_PM_IDX, host );

        /* prepare LOCATION for config.log */
        NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file );

        /* open change log file for appending */
        if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
                &change_log_fp ) != OK )
        {
	    SIfprintf( stderr,
	    ERget( S_ST0323_unable_to_open), change_log_buf );
        }

	CRinit( pm_id );

	status = CRload( rulemap, &info );

	switch( status )
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

	if( verbose )
		F_PRINT( ERget( S_ST0411_generating_defaults ), host );

	resources = (i4 *) MEreqmem( 0, info * sizeof( i4  ), FALSE, NULL );

	for( i = 0; i < info; i++ )
		resources[ i ] = i;

	if( verbose )
		PRINT( ERget( S_ST0412_sorting_resources ) );

	CRsort( resources, info );

	if( verbose )
		PRINT( ERget( S_ST0413_computing_resources ) );

	/* pause for slow readers */
	PCsleep( 1 );

	for( i = 0; i < info; i++ )
	{
		char *symbol, *full_name, *elem2, *value, *temp;
		i4 len;

		temp = NULL;

		symbol = CRsymbol( resources[ i ] );

 		len = PMmNumElem( pm_id, symbol ); 

		elem2 = PMmGetElem( pm_id, 2, symbol );

		full_name = PMmExpandRequest( pm_id, symbol );
		if( len == 6 
                    && ( STequal( PMmGetElem( pm_id, 2, full_name ),
                                  ERx( "dbms" ) )
                         || STequal( PMmGetElem( pm_id, 2, full_name ),
			          ERx( "rms" ) )
                       ) 
                  )
#if 0
		if( len == 6 
                    && STequal( PMmGetElem( pm_id, 2, full_name ),
                                  ERx( "dbms" ) )
                  )
#endif
		{
			/*  dmf cache case */
			temp = PMmGetDefault( pm_id, 3 );
			PMmSetDefault( pm_id, 3, ERx( "private" ) );
			full_name = PMmExpandRequest( pm_id, symbol );
		}

		if (len == 7 && 
                     ( STequal(PMmGetElem(pm_id, 2, full_name), ERx("dbms")) ||
                       STequal(PMmGetElem(pm_id, 2, full_name), ERx("rms"))
                     ) && 
		    (STequal(PMmGetElem(pm_id, 5, full_name), ERx("p2k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("p4k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("p8k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("p16k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("p32k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("p64k")) ||
		     STequal(PMmGetElem(pm_id, 5, full_name), ERx("cache"))))
		{
                        /*  dmf cache case */
                        temp = PMmGetDefault( pm_id, 3 );
                        PMmSetDefault( pm_id, 3, ERx( "private" ) );
                        full_name = PMmExpandRequest( pm_id, symbol );
		}

		/* derived resources must be recomputed */
		if( CRderived( CRsymbol( resources[ i ] ) ) )
			PMmDelete( pm_id, full_name ); 

		if( CRcompute( resources[ i ], &value ) != OK )
		{
			SIfprintf( stderr,
				ERget( S_ST0414_undefined_resource ),
				full_name );
                        /* close change log after appending */
                        SIclose( change_log_fp );
			PCexit( FAIL );
		}

		if( value == NULL )
		{
			PMmSetDefault( pm_id, 3, temp );
			continue;
		}

		CRF_FPRINT( change_log_fp, ERx( "%s: " ), full_name );
		CRF_FPRINT( change_log_fp, "(%s)\n", value );

		PMmInsert( pm_id, CRsymbol( resources[ i ] ), value );

		PMmSetDefault( pm_id, 3, temp );
	}

        if( verbose )
		F_PRINT( ERget( S_ST0415_defaults_generated ), host );

        PMmWrite( pm_id, NULL );

         /* close change log after appending */
        (void) SIclose( change_log_fp );

	PCexit( OK );
}

/* return null function to pathname verification routines */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
	return NULL;
}

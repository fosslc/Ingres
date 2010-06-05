/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
**
**  Name: iiinitres.c -- contains main() for a program which creates
**	 the specified configuration entries for an INGRES installation.
**
**  Usage:
**	iiinitres [ -v ] [-keep] [-keephigher]  resource  [rule-map]
**
**  Description:
**      Insert the specified resource into the configuration file for
**      all positions where the resource can be added.
**      The utility will replace the default elements of the template
**	for the specified resource with appropriate values obtained
**	from the PM data file. For each entry added, any resources
**      dependant on the inserted resource will be recalculated.
**
**	-keep = If a resource with the specified name exists, keep
**		its current value.  (Used during upgrades.)  Without the
**		-keep option, any old value is overwritten, as would be
**		appropriate for brand new resources.
**	-keephigher = If a resource with the specified name exists, is
**		      a numeric and is greater than the specified value,
		      keep the original.
**
**	-v = Verbose; display old/new values as well as all recalcs
**
**	The optional rule-map file names a rule map for the rule system
**	loader to use.  The normal default is "default.rfm", ie, all the
**	standard rule files.  The optional rule-map exists for upgrading,
**	for special situations like deprecating a resource in favor of a
**	new, improved one.  While the resource may have some rule default
**	X, for upgrading we may wish to set its value to the deprecated
**	resource value.  Since iiinitres takes care of ALL possible
**	occurrences (all hosts, all configurations, etc), it can do this
**	much better than a complicated series of hand-coded searches,
**	get-values, and set-values.  All we need to do is to temporarily
**	modify the rule, and that's where the rule-map comes in.  The
**	upgrade would build a temporary rule-map which points to a
**	modified version of the rule for the new resource, that sets
**	the resource from the old one.
**
**  History:
**	19-Jan-2000 (horda03)
**		Created.
**	21-May-2001 (hanje04)
**		Replace nat with i4's
**      20-oct-2002 (horda03)
**              There was a problem updating values which existed in the
**		config.dat file. The CRcompute() function defaults to the
**		value in the PM cache, so must delete the value before
**		calling CRcompute(). THis meant that the old value is
**		lost, so now storing it in local memory.
**		Not all entries were being calculated if the value we used
**		as a template didn't exist elsewhere. Now, scan through
**		all matches, but only set the value once for each matching
**		resource.
**       4-Oct-2002 (hanal04) Bug 109219.
**              Correct above change to resolve undefined symbol error
**              when linking iiinitres.
**      15-apr-2003 (hanch04)
**          Pass the config.log to CRsetPMval and new flag to pass
**          for printing to stdout if verbose is set.
**          This change fixes bug 100206.
**	4-Dec-2003 (schka24)
**	    Add -keep option and optional rule-map, both for use in
**	    upgrading when new or replacement resources are introduced.
**	24-Sep-2004 (schka24)
**	    Be more careful about avoiding junk in orig-value for printing.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHEMBEDLIB to replace
**	    its static libraries. Removed MALLOCLIB from NEEDLIBS
**	20-Apr-2005 (hweho01)
**	    Replace MEcopy with STcopy when the check_str buffer 
**	    is initialized, since it needs to be terminated by the 
**	    null character. Bug 114359. 
**	20-June-2007 (hanje04)
**	    SIR 118543
**	    Add -keephigher option to protect existing values from being
**	    trampled without preventing them from being increased.
**	25-Jul-2007 (drivi01)
**	    Added a routine for Vista port, to provide additional 
**	    error message when this application is unable to open 
**	    config.log.  Fixed a potential bug, check if the file 
**	    is indeed open before attempting to close it.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of cv.h to eliminate gcc 4.3 warnings.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

/*
PROGRAM =	iiinitres
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
# include <nm.h>
# include <pm.h>
# include <cm.h>
# include <pmutil.h>
# include <util.h>
# include <ep.h>
# include <simacro.h>
# include <erst.h>
# include <cv.h>
# include "cr.h"
# include <cv.h>

GLOBALDEF char		change_log_buf[ MAX_LOC + 1 ];	
GLOBALDEF FILE          *change_log_fp;
GLOBALDEF LOCATION      change_log_file;


main( int argc, char **argv )
{
	i4		*resources, i, j, info, len;
	STATUS		pm_status;
	STATUS 		status;
	PM_CONTEXT 	*pm_id;
	char 		*resource;
	char		*rule_map;
	bool 		usage = FALSE;
	bool 		verbose = FALSE;
	bool 		resource_spec = FALSE;
	bool		resource_found = FALSE;
	bool		keep_existing;
	bool		keep_higher;
	i4 		*wild_pos = NULL;
	i4 		no_wild;
        PM_SCAN_REC 	state;
	char		*symbol, *full_name, *value, *orig;
	char 		check_str [1024];
	char 		last_str [1024];
	char		*orig_val = 0;
	i4		orig_size = 0;
	i4		orig_len;
	char		*res_place, *entry, *sea_res, *ch;

	MEadvise( ME_INGRES_ALLOC );

	rule_map = NULL;
	keep_existing = FALSE;
	keep_higher = FALSE;
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] == '-')
		{
			if( STcompare( argv[ i ], ERx( "-v" ) ) == 0 )
			{
				verbose = TRUE;
			}
			else if( STcompare( argv[ i ], ERx( "-keep" ) ) == 0 )
			{
				keep_existing = TRUE;
			}
			else if( STcompare( argv[ i ],
					    ERx( "-keephigher" ) ) == 0 )
			{
				keep_higher = TRUE;
			}
			else
			{
				usage = TRUE;
				break;
			}
		}
		else if (!resource_spec)
		{
			resource_spec = TRUE;

			resource = argv [i];
		}
		else if (rule_map == NULL)
		{
			rule_map = argv[i];
		}
		else
		{
			usage = TRUE;
			break;
		}
	}

	if( (!resource_spec) || usage )
	{
		SIprintf( ERx("\nUsage: %s [-v] [-keep] [-keephigher] resource [rule-map]\n\n"),
			argv[0] );
		PCexit( FAIL );
	}

	(void) PMmInit( &pm_id );

	PMmLowerOn( pm_id );

        if( PMmLoad( pm_id, NULL, PMerror ) != OK )
	     PCexit( FAIL );

        /* prepare LOCATION for config.log */
        NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file );

        /* open change log file for appending */
        if( SIfopen( &change_log_file , ERx( "a" ), 
	    SI_TXT, SI_MAX_TXT_REC, &change_log_fp ) != OK )
        {
    	    /* 
	    ** Check if this is Vista, and if user has sufficient privileges 
	    ** to execute.
	    */
	    ELEVATION_REQUIRED_WARNING();
	    SIfprintf( stderr, ERget( S_ST0323_unable_to_open),
	    change_log_buf );
        }

	CRinit( pm_id );

	status = CRload( rule_map, &info );

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
		F_PRINT( ERget( S_ST044A_init_resource ), resource);

	resources = (i4 *) MEreqmem( 0, info * sizeof( i4 ), FALSE, NULL );

	for( i = 0; i < info; i++ )
		resources[ i ] = i;

	if( verbose )
		PRINT( ERget( S_ST0412_sorting_resources ) );

	CRsort( resources, info );

	if( verbose )
		PRINT( ERget( S_ST0413_computing_resources ) );

	/* pause for slow readers */
	PCsleep( 1 );

	orig_size = 100;
	orig_val = MEreqmem(0, orig_size, TRUE, NULL);

	if (!orig_val)
	{
		SIprintf( "Memory allocation error. Utility aborted\n\n");

		PCexit( FAIL );
	}

	for( i = 0; i < info; i++ )
	{
		symbol = CRsymbol( resources[ i ] );

 		len = PMmNumElem( pm_id, symbol );

		value = PMmGetElem( pm_id, len - 1, symbol);

		if (STcompare(value, resource) != 0) continue;

		resource_found = TRUE;

                /* Now that we've found the resource format, we need to
                ** add an entry to ALL positions where it belongs.
                */

		/* First determine the elements which are Default - denoted by '$'.
                ** Need to find all the "Default" entries for the resoure being
		** added.
		*/

		if (wild_pos) MEfree( (char *)wild_pos);

		wild_pos = (int *) MEreqmem(0, len * sizeof( i4 *), FALSE, NULL);
		no_wild = 0;

		for (j = 0; j < len; j++)
		{
			if (STcompare(PMmGetElem( pm_id, j, symbol), ERx("$")) == 0)
			{
				wild_pos [no_wild++] = j;
			}
		}

                /* Convert the Default elements to wildcard elements for the search */

                STcopy( symbol, check_str );

                for( ch = check_str; *ch; ch++)
		{
                   if (*ch == '$') *ch = '%';
		}

                /* Change last element to a wildcard, we want to find all resources
		** resource that has the same format as the entry we wish to add.
		** The relevant "Default" elements wil be extracted from all
		** entries of this found resource.
		*/

		res_place = STrindex( check_str, ERx("."), STlength( check_str ));

                *(++res_place)   = '%';
                *(res_place + 1) = '\0';

                sea_res = PMmExpToRegExp( pm_id, check_str);

                len = 0;

                MEfill(sizeof(last_str), 0, last_str);

                for (status = PMmScan( pm_id, sea_res, &state, NULL, &entry, &value);
		     status == OK;
		     status = PMmScan( pm_id, NULL, &state, NULL, &entry, &value) )
		{
			/* If this entry matches previously found string, then move on
                        ** to next entry.
                        */
			if ( !STbcompare(last_str, len, entry, len, FALSE)) continue;

			res_place = STrindex( entry, ERx("."), STlength( entry ));

                        len = (i4) ( (PTR) res_place - (PTR) entry);

                        MEcopy(entry, len, last_str);

			/* Set default and add symbol. Recalculate derived symbols. */

			for (j = 0; j < no_wild; j++)
			{
				PMmSetDefault(  pm_id, wild_pos [j],
						PMmGetElem( pm_id, wild_pos [j], entry));
			}

			full_name = PMmExpandRequest( pm_id, symbol );

			/* The original value is needed for -v
			** (verbose) and -k (keep existing value).
			** There is something of a glitch here, there's
			** no way to tell PM to NOT search wildcards!
			** If e.g. ii.host.dbms.*.foo exists and is 30,
			** PMmGet will find a value of 30 for
			** ii.host.dbms.noname.foo.  That's why -k has
			** to delete and (re-)insert everything;  otherwise
			** it might think that noname.foo is already there.
			** The (unfortunate) side effect is that in the
			** example, noname.foo gets inserted as 30 instead
			** of as the the rules default.  "It's a feature."
			*/
			if (verbose || keep_existing || keep_higher)
			{
			    pm_status = PMmGet( pm_id, full_name, &orig);
			    if (pm_status == OK)
			    {
				if ( (orig_len = STlength(orig)) >= orig_size)
				{
				    MEfree( orig_val );
				    /* Allow for small increases */
				    orig_size = orig_len + 100;
				    orig_val = MEreqmem(0, orig_size, FALSE, NULL);
				}
				MEcopy(orig, orig_len + 1, orig_val);
			    }
			    else
			    {
				orig_val[0] = EOS;
			    }
			}

			PMmDelete( pm_id, full_name );

			if( CRcompute( resources[ i ], &value ) != OK )
			{
				SIfprintf( stderr,
					   ERget( S_ST0414_undefined_resource ),
					   full_name );
				PCexit( FAIL );
			}

			if (value == NULL) continue;

			if ( keep_higher && pm_status == OK )
			{
			    /*
			    ** if current value is numeric and >
			    ** the specified value, keep it.
			    */
			    char	*ptr = orig_val ;
			    bool isnum = TRUE ;
			    i8	orig_i8, new_i8;

			    CVal8( orig_val, &orig_i8 );
			    CVal8( value, &new_i8 );

			    /*
			    ** check special cases first. 0 and -ve
			    ** numbers mean unlimited or max. Never
			    ** overwrite a 0 or -ve value.
			    ** Otherwise, always apply new 0 or -ve
			    */
			    if ( orig_i8 < (i8)1 )
				keep_existing = TRUE;
			    else if ( new_i8 < (i8)1 )
				keep_existing = FALSE;
			    else
			    {
				while ( *ptr != EOS )
				{
				    if ( ! CMdigit( ptr ) )
				    {
					isnum = FALSE;
					break;
				    }
				    ptr++ ;
				}

				if ( isnum && ( orig_i8 > new_i8 ) )
				    keep_existing = TRUE ;
  
			    }
			}

			if (keep_existing && pm_status == OK)
				value = orig_val;

			SIprintf( "CHANGE %s: (%s)...(%s)\n",
			    full_name, orig_val, value);

			CRsetPMval( full_name, value, change_log_fp, 
			    verbose, TRUE);
			CRresetcache();
			orig_val[0] = EOS ;
		}

		if (! len )
		{
			/* There is no entry that can be used to fill in the
			** Defaults of the new resource, so add the resource using
			** Wildcaards as the Default elements.
			*/

			for (j = 0; j < no_wild; j++)
			{
				PMmSetDefault( pm_id, wild_pos [j], ERx("*"));
			}

                        if( CRcompute( resources[ i ], &value ) != OK )
                        {
                                SIfprintf( stderr,
                                        ERget( S_ST0414_undefined_resource ),
                                        full_name );
                                PCexit( FAIL );
                        }

			if (value == NULL) continue;

                        CRsetPMval( symbol, value, change_log_fp, 
			    verbose, verbose);
			continue;
		}

	}

	if (resource_found)
	{
		if( verbose )
			F_PRINT( ERget( S_ST044B_added_resource ), resource );

        	PMmWrite( pm_id, NULL );

        	/* close change log after appending */
		if (change_log_fp)
	        	(void) SIclose( change_log_fp );

		PCexit( OK );
	}

	if( verbose )
		F_PRINT( ERget( S_ST0414_undefined_resource ), resource );

        /* close change log after appending */
        (void) SIclose( change_log_fp );

	PCexit( FAIL );
}

/* return null function to pathname verification routines */
BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
{
	return NULL;
}

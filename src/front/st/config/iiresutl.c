/*
** Copyright (c) 2004 Ingres Corporation
**
*/
/*
**
**  Name: iiresutl.c -- contains main() for a program which computes 
**	complex aggregate functions on data stored in config.dat in
**	the II_CONFIG directory.
**
**  Usage:
**	iiresutl -dbms_sum | -dbms_max | -dmf_max_seg | -dmf_seg_num |
**		 -net_sum | -net_max | -dbms_mem_all[=dbms type] |
**               -max_dbms_mem | -max_dbms_stack | -dbms_swap_mem [ outfile hostname ] |
**		-logwriter_sum
**
**  Description:
**	iiresutl optionally computes aggregate functions for data
**	associated with the named host and in that case sends it output
**	to a named temporary file.  iiresutl is intended for use as an
**	external configuration rules system procedure.  The aggregate
**	functions it computes are described below:
**
**	-dbms_sum	Returns ( ii.$.dbms.foo.connect_limit *
**			ii.$.ingstart.foo.dbms ) for all values of foo.
**
**	-dbms_max	Returns largest value of ii.$.dbms.foo.connect_limit 
**			where ii.$.ingstart.foo.dbms > 0.
**
**	-net_sum	Returns ( ( ii.$.gcc.foo.inbound_limit +
**			ii.$.gcc.foo.outbound_limit ) *
**			ii.$.ingstart.foo.gcc ) for all values of foo.
**
**	-net_max	Returns largest value of ( ii.$.gcc.foo.inbound_limit 
**			+ ii.$.gcc.foo.outbound_limit ) where
**			ii.$.ingstart.foo.gcc > 0.
**
**	-star_sum	Returns ( ii.$.star.foo.connect_limit *
**			ii.$.ingstart.foo.star ) for all values of foo.
**
**	-star_max	Returns largest value of ii.$.star.foo.connect_limit 
**			where ii.$.ingstart.foo.star > 0.
**
**	-dmf_max_seg	Returns largest value of ii.$.dbms.shared.foo.dmf_memory
**			where ii.$.ingstart.foo.dbms > 0. 
**
**	-dmf_seg_num	Returns total number of shared dmf caches configured
**			for startup.
**
**      -dbms_mem_all[=dbms type]
**   			Returns the amount of memory required by all the DBMS
**                      servers, where ii.$.ingstart.foo.dbms > 0. You may
**                      specify the memory used by a specific DBMS type. The
**                      memory requirements take into consideration, all memory
**                      regions created by each of the server started.
**
**      -max_dbms_mem	Returns the maximum memory used by a single DBMS server
**			where ii.$.ingstart.foo.dbms > 0.
**
**      -max_dbms_stack	Returns the maximum stack space than can be allocated by a
**                      single DBMS server where ii.$.ingstart.foo.dbms > 0.
**
**      -dbms_swap_mem	Returns the amount of Swap memory used by DBMS servers
**                      configured to start.
**
**	-logwriter_sum	Returns FOO_SUM(ii.$.dbms.foo.log_writer *
**			 ii.$.ingstart.foo.dbms) + ii.$.recovery.%.log_writer
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-93 (tyler)
**		Added support for -star_max option.
**	23-jul-93 (tyler)
**		Change ii.$.dbms.$.cache_share to ii.$.dbms.$.cache_sharing.
**		Fixed bug in -dmf_max_seg calculation.  PMerror() is now
**		passed to PMload() for better error reporting.  Call to
**		PMlowerOn() removed. 
**	25-aug-93 (fredv)
**		Included <st.h>
**	19-oct-93 (tyler)
**		Call PMhost() instead of GChostname().  Changed "dmf"
**		to "dbms" in dbms cache resources names.
**	05-nov-93 (tyler)
**		PCexit() with OK if output is successfully written to a
**		temporary file.
**	22-feb-94 (tyler)
**		Added -net_max option needed to fix BUG 59512.  Also
**		added -net_sum and fixed other unreported BUGS found
**		while editing this file.
**	25-feb-94 (tyler)
**		Fixed unreported BUG introduced in the previous revision
**		and added -star_sum option for completeness.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**	21-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Mar-2003 (hanch04)
**	    The -dmf_max_seg should only return the largest single cache
**	    that is needed.  If dmf_separate is on for any caches, that
**	    should not be included in the total amount of memory needed.
**	    The -dmf_seg_num should return the total number of segments.
**	    If dmf_separate is on, then that cache is its on segment.
**	28-Nov-2003 (schka24)
**	    add logwriter_sum
**	    include QSF memory in -max_dbms_mem and friends
**	17-aug-2004 (thaju02)
**	    Changed dbms_swap_mem, max_dbms_stack and associated local 
**	    variables to i8.  Use CVal8.
**	30-aug-2004 (sheco02)
**	    Back out change r23 to fix the x-integration problem.
**      06-Jul-2009 (coomi01) b122263
**          It may be possible to configure a single logwriter thread
**          where the dbms has 0 writers recovery has 1.
**      29-Mar-2010 (ashco01) B122768 
**          Include QEF memory in -max_dbms_mem calculation.
*/

/*
PROGRAM =	(PROG1PRFX)resutl
**
NEEDLIBS =	UTILLIB UGLIB COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <gl.h>
# include <cv.h>
# include <me.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <pc.h>
# include <pmutil.h>
# include <util.h>
# include <si.h>
# include <st.h>
# include <simacro.h>
# include <erst.h>

char *dmf_memory[]  = { "", "p4k", "p8k", "p16k", "p32k", "p64k" };

/* Structure to store defined cache names */

typedef struct _CACHE_LIST {
                        char *name;
                        struct _CACHE_LIST *next;
                }
                CACHE_LIST;

int
num_threads( char *dbms_name )
{
   u_i4 sum = 10; /* Idle, Admin, TimeBandit, Dead Process Detector, Force Abort, Group Commit
                  ** Consistency Pt, Event, Terminator, License
                  */
   char expbuf[ 1024 ];
   char *value;
   i4   val;

   STprintf( expbuf, ERx("%s.$.dbms.%s.connect_limit"), SystemCfgPrefix,
                              dbms_name );

   if( PMget( expbuf, &value ) == OK )
   {
      CVan( value, &val);
      sum += val;
   }

   STprintf( expbuf, ERx("%s.$.dbms.%s.log_writer"), SystemCfgPrefix,
                              dbms_name );

   if( PMget( expbuf, &value ) == OK )
   {
      CVan( value, &val );
      sum += val;
   }

   STprintf( expbuf, ERx("%s.$.dbms.%s.fast_commit"), SystemCfgPrefix,
                              dbms_name );

   if( PMget( expbuf, &value ) == OK )
   {
      if ( ! ValueIsBoolTrue( value ) )
      {
         STprintf( expbuf, ERx("%s.$.dbms.%s.write_behind"), SystemCfgPrefix,
                              dbms_name );

         if( PMget( expbuf, &value ) == OK )
         {
            CVan( value, &val );
            sum += val;
         }
      }
   }

   STprintf( expbuf, ERx("%s.$.dbms.%s.rep_qman_threads"), SystemCfgPrefix,
                              dbms_name );

   if( PMget( expbuf, &value ) == OK )
   {
      CVan( value, &val );
      sum += val;
   }

   return sum;
}

i8
dmf_cache_size( char *cache_type, char *cache_name )
{
   i8 sum = 0;
   i8 tmp;
   i4   i;
   char expbuf[ 1024 ];
   char *value;
   

   for ( i = 0; i < sizeof(dmf_memory)/sizeof(char *); i++)
   {
      if (*dmf_memory [i] == '\0')
      {
         STprintf( expbuf, ERx("%s.$.dbms.%s.%s.dmf_memory"), SystemCfgPrefix,
                              cache_type, cache_name );
      }
      else
      {
         /* Check that Cache is "ON". */

         STprintf( expbuf, ERx("%s.$.dbms.%s.%s.cache.%s_status"), SystemCfgPrefix,
                              cache_type, cache_name, dmf_memory[i]);

         if ( PMget( expbuf, &value ) != OK )  continue;

         if ( ! ValueIsBoolTrue( value ) )  continue;

         STprintf( expbuf, ERx("%s.$.dbms.%s.%s.%s.dmf_memory"), SystemCfgPrefix,
                              cache_type, cache_name, dmf_memory[i]);
      }

      if( PMget( expbuf, &value ) == OK )
      {
	 CVal8( value, &tmp );
         sum += tmp;
      }
   }

   return sum;
}

i8
dmf_max_segment_size( char *cache_type, char *cache_name )
{
    i8 sum = 0;
    i8 max_separate_seg = 0;
 
    i4   i;
    i8   tmp;
    char expbuf[ 1024 ];
    char *value;

    for ( i = 0; i < sizeof(dmf_memory)/sizeof(char *); i++)
    {
	if (*dmf_memory [i] == '\0')
	{
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.dmf_memory"),
		SystemCfgPrefix, cache_type, cache_name );
	}
	else
	{
	    /* Check that Cache is "ON". */
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.cache.%s_status"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	    if ( PMget( expbuf, &value ) != OK )  continue;
	    if ( ! ValueIsBoolTrue( value ) )  continue;
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.%s.dmf_memory"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	}

	if( PMget( expbuf, &value ) == OK )
	{
	    CVal8( value, &tmp );
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.%s.dmf_separate"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	    if ( PMget( expbuf, &value ) == OK )
	    {
		if ( ValueIsBoolTrue( value ) )
		{ 
		    if( tmp > max_separate_seg )
			max_separate_seg = tmp;
		    continue;
		}
	    }
	    sum += tmp;
	}
    }
    if( sum > max_separate_seg )
    {
        return sum;
    }
    else
    {
        return max_separate_seg;
    }
}

int
dmf_num_segments( char *cache_type, char *cache_name )
{
    i4   i, tmp;
    int num_segments = 1;
    char expbuf[ 1024 ];
    char *value;

    for ( i = 0; i < sizeof(dmf_memory)/sizeof(char *); i++)
    {
	if (*dmf_memory [i] == '\0')
	{
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.dmf_memory"),
		SystemCfgPrefix, cache_type, cache_name );
	}
	else
	{
	    /* Check that Cache is "ON". */
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.cache.%s_status"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	    if ( PMget( expbuf, &value ) != OK )  continue;
	    if ( ! ValueIsBoolTrue( value ) )  continue;
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.%s.dmf_memory"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	}

	if( PMget( expbuf, &value ) == OK )
	{
	    CVan( value, &tmp );
	    STprintf( expbuf, ERx("%s.$.dbms.%s.%s.%s.dmf_separate"),
		SystemCfgPrefix, cache_type, cache_name, dmf_memory[i]);
	    if ( PMget( expbuf, &value ) == OK )
	    {
		if ( ValueIsBoolTrue( value ) )
		{ 
		    num_segments++;
		}
	    }
	}
    }
    return num_segments;
}

void
main( int argc, char **argv )
{
	LOCATION fileloc;
	STATUS status;
	PM_SCAN_REC state1, state2;
	char *host;                    
	char locbuf[ MAX_LOC + 1 ];
	char result[ 1024 ];
	char expbuf[ 1024 ];
	char tmpbuf[ 1024 ];
	char *exp, *name, *value, *startup;
	FILE *fp;
        char *memory_area[] = { "opf_memory", "psf_memory", "qef_memory", 
                                "qsf_memory", "rdf_memory" };

	MEadvise( ME_INGRES_ALLOC );

	if( argc < 2 || argc > 4 )
		ERROR( "bad command line." );

	PMinit();

	if( PMload( NULL, PMerror ) != OK )
		PCexit( FAIL );

	if( argc > 2 )
		PMsetDefault( HOST_PM_IDX, argv[ argc - 1 ] );
	else
	{
		host = PMhost();
		PMsetDefault( HOST_PM_IDX, host );
	}

	if( STcompare( argv[ 1 ], ERx( "-dbms_max" ) ) == 0 )
	{
		i4 n, max_val = 0;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );
		STcopy( ERx( "0" ), result );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			CVan( startup, &n );

			if( n < 1 )
				continue;

			STprintf( expbuf, ERx( "%s.$.dbms.%s.connect_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf ); 

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK ) 
			{
				CVan( value, &n );
				if( n > max_val )
				{
					max_val = n;
					STcopy( value, result );
				}
			}
		}
	}
	else if( STcompare( argv[ 1 ], ERx( "-star_sum" ) ) == 0 )
	{
		i4 n, sum = 0;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.star"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			i4 n;

			CVan( startup, &n ); 

			if( n < 1 )
				continue;
		
			STprintf( expbuf, ERx( "%s.$.star.%s.connect_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf ); 

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK )
			{
				i4 tmp; 

				CVan( value, &tmp );
				sum += n * tmp;
			}
		}
		STprintf( result, ERx( "%d" ), sum );
	}
	else if( STcompare( argv[ 1 ], ERx( "-star_max" ) ) == 0 )
	{
		i4 n, max_val = 0;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.star"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );

		STcopy( ERx( "0" ), result );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			i4 n;

			CVan( startup, &n );

			if( n < 1 )
				continue;

			STprintf( expbuf, ERx( "%s.$.star.%s.connect_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf );

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK ) 
			{
				CVan( value, &n );
				if( n > max_val )
				{
					max_val = n;
					STcopy( value, result );
				}
			}
		}
	}
	else if( STcompare( argv[ 1 ], ERx( "-dbms_sum" ) ) == 0 )
	{
		i4 n, sum = 0;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			i4 n;

			CVan( startup, &n ); 

			if( n < 1 )
				continue;
		
			STprintf( expbuf, ERx( "%s.$.dbms.%s.connect_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf ); 

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK )
			{
				i4 tmp; 

				CVan( value, &tmp );
				sum += n * tmp;
			}
		}
		STprintf( result, ERx( "%d" ), sum );
	}
	else if( STcompare( argv[ 1 ], ERx( "-dmf_max_seg" ) ) == 0 )
	{
		/* figure size of largest dmf shared memory segment. */

		char buf[ 1024 ];
		i8 calc_seg;
		i8 max_seg = 0;
                char *shared_cache;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
                        i4 n;

                        CVan( startup, &n ); 

                        if( n < 1 )
                                continue;

			PMsetDefault( 3, PMgetElem( 3, name ) );

			/* look up ii.$.dbms.$.cache_sharing */
			STprintf( tmpbuf, ERx( "%s.$.dbms.$.cache_sharing" ),
				  SystemCfgPrefix );
			if( PMget( tmpbuf, &value ) != OK)
				continue;

			if( ! ValueIsBoolTrue( value ) )
				continue;
			/* if reached here, cache is shared */

			STprintf( tmpbuf, ERx( "%s.$.dbms.$.cache_name" ), 
				  SystemCfgPrefix );
			if( PMget( tmpbuf, &shared_cache) != OK )
				continue;

			/* if reached here, found shared cache name */

                        calc_seg = dmf_max_segment_size( "shared", shared_cache);
			if( calc_seg > max_seg )
				max_seg = calc_seg;
		}
		STprintf( result, ERx( "%lld" ), max_seg );
	}
	else if( STcompare( argv[ 1 ], ERx( "-dmf_seg_num" ) ) == 0 )
	{	
		/* count number of dmf shared memory segments. */
		char buf[ 1024 ];
		i4 n;
		i4 num_segments = 0;
		CACHE_LIST *head, *cur;	

		head = NULL;

		STprintf( tmpbuf, ERx( "%s.$.ingstart.%%.dbms" ),
			  SystemCfgPrefix );
		exp = PMexpToRegExp( tmpbuf );
		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			PMsetDefault( 3, PMgetElem( 3, name ) );

			/* look up ii.$.dbms.$.cache_sharing */
			STprintf( tmpbuf, ERx( "%s.$.dbms.$.cache_sharing" ), 
				  SystemCfgPrefix );
			if( PMget( tmpbuf, &value )
				!= OK
			)
				continue;
			/* if here, found cache_sharing value */

			if( ! ValueIsBoolTrue( value ) )
				continue;
			/* if reached here, cache is shared */

			STprintf( tmpbuf, ERx( "%s.$.dbms.$.cache_name" ),
				  SystemCfgPrefix );
			if( PMget( tmpbuf, &value ) != OK )
				continue;
			/* if reached here, found shared cache name */

			/* add it to the list if it's not already there */
			for( cur = head; cur != NULL; cur = cur->next )
				if( STcompare( cur->name, value ) == 0 )
					break;
			if( cur == NULL )
			{
				/* found another shared cache */
				cur = (CACHE_LIST *)  MEreqmem( 0,
					sizeof( CACHE_LIST), FALSE,
					NULL ); 
				if( cur == NULL )
					ERROR( "MEreqmem() failed." );
				cur->name = value;
				cur->next = head;
				head = cur;
			}
		}
		/* count records in list */
		for( cur = head, n = 0; cur != NULL; cur = cur->next, n++ )
		{
		    num_segments += dmf_num_segments( "shared", cur->name);
		}

		STprintf( result, ERx( "%d" ), num_segments );
	}
	else if( STcompare( argv[ 1 ], ERx( "-net_max" ) ) == 0 )
	{
		i4 n, max_val = 0;

		STprintf( tmpbuf, ERx( "%s.$.ingstart.%%.gcc" ),
			  SystemCfgPrefix );
		exp = PMexpToRegExp( tmpbuf );

		STcopy( ERx( "0" ), result );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			CVan( startup, &n );

			if( n < 1 )
				continue;

			STprintf( expbuf, ERx( "%s.$.gcc.%s.inbound_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf );

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK ) 
			{
				CVan( value, &n );
			}

			STprintf( expbuf, ERx( "%s.$.gcc.%s.outbound_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf );

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK ) 
			{
				i4 tmp;

				CVan( value, &tmp );
				n += tmp;
			}

			if( n > max_val )
			{
				max_val = n;
				STprintf( result, ERx( "%d" ), max_val );
			}
		}
	}
	else if( STcompare( argv[ 1 ], ERx( "-net_sum" ) ) == 0 )
	{
		i4 n, sum = 0;

		STprintf( tmpbuf, ERx( "%s.$.ingstart.%%.gcc" ), 
			  SystemCfgPrefix );
		exp = PMexpToRegExp( tmpbuf );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name,
			&startup ) )
		{
			i4 n, tmp;

			CVan( startup, &n ); 

			if( n < 1 )
				continue;

			STprintf( expbuf, ERx( "%s.$.gcc.%s.inbound_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf );

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK )
			{
				CVan( value, &tmp );
				sum += n * tmp;
			}

			STprintf( expbuf, ERx( "%s.$.gcc.%s.outbound_limit" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf );

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK )
			{
				CVan( value, &tmp );
				sum += n * tmp;
			}
		}
		STprintf( result, ERx( "%d" ), sum );
	}
	else if( STscompare( argv[ 1 ], 13, ERx( "-dbms_mem_all"), 13) == 0 )
	{
           u_i4 i, sum = 0;
           i4 n, tmp;
           char *dbms_name;

           dbms_name = argv[ 1 ] + 9;

           if (*(dbms_name++) == '=')
           {
              /* Only obtain information regarding the specified server type */

              STprintf(tmpbuf, ERx("%s.$.ingstart.%s.dbms"), SystemCfgPrefix, dbms_name);
           }
           else
           {
              STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
           }
           exp = PMexpToRegExp( tmpbuf );

           for(
                        status = PMscan( exp, &state1, NULL, &name, &startup );
                        status == OK;
                        status = PMscan( NULL, &state1, NULL, &name,
                        &startup ) )
           {
              char *cache_type;
              char *cache_name;
              char search_str [500];

              CVan( startup, &n );
               
              /* Only interested in Servers which are started by Ingstart */

              if( n < 1 ) continue;

              dbms_name = PMgetElem( 3, name );


              /* Determine whether the server is using private or shared cache */

              STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_sharing" ),
                                  SystemCfgPrefix, dbms_name );

              if( PMget( expbuf, &value ) != OK )  continue;

              if ( ! ValueIsBoolTrue( value ) )
              {
                 cache_type = "private";
                 cache_name = dbms_name;
              }
              else
              {
                 cache_type = "shared";


                 /* Get the shared cache name */

                 STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_name" ),
                           SystemCfgPrefix, dbms_name );

                 if( PMget( expbuf, &cache_name) != OK ) continue;
              }

              /* Get the memory allocation for each memory segment */

              for ( i = 0; i < sizeof(memory_area)/sizeof(char *); i++)
              {
                 STprintf( expbuf, ERx("%s.$.dbms.%s.%s"), SystemCfgPrefix,
                           dbms_name, memory_area [i]);

                 if( PMget( expbuf, &value ) == OK )
                 {
                    CVan( value, &tmp );
                    sum += n * tmp;
                 }
              }

              /* Get the memory allocation for each DMF area enabled */

              sum += n * dmf_cache_size(cache_type, cache_name);
           }
           STprintf( result, ERx( "%d" ), sum );
        }
	else if( STcompare( argv[ 1 ], ERx( "-max_dbms_mem")) == 0 )
	{
           u_i4 i, sum, max_dbms_mem = 0;
           i4 n, tmp;
           char *dbms_name;

           STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
           exp = PMexpToRegExp( tmpbuf );

           for(
                        status = PMscan( exp, &state1, NULL, &name, &startup );
                        status == OK;
                        status = PMscan( NULL, &state1, NULL, &name,
                        &startup ) )
           {
              char *cache_type;
              char *cache_name;
              char search_str [500];

              CVan( startup, &n );
               
              /* Only interested in Servers which are started by Ingstart */

              if( n < 1 ) continue;

              sum = 0;

              dbms_name = PMgetElem( 3, name );


              /* Determine whether the server is using private or shared cache */

              STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_sharing" ),
                                  SystemCfgPrefix, dbms_name );

              if( PMget( expbuf, &value ) != OK )  continue;

              if ( ! ValueIsBoolTrue( value ) )
              {
                 cache_type = "private";
                 cache_name = dbms_name;
              }
              else
              {
                 cache_type = "shared";


                 /* Get the shared cache name */

                 STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_name" ),
                           SystemCfgPrefix, dbms_name );

                 if( PMget( expbuf, &cache_name) != OK ) continue;
              }

              /* Get the memory allocation for each memory segment */

              for ( i = 0; i < sizeof(memory_area)/sizeof(char *); i++)
              {
                 STprintf( expbuf, ERx("%s.$.dbms.%s.%s"), SystemCfgPrefix,
                           dbms_name, memory_area [i]);

                 if( PMget( expbuf, &value ) == OK )
                 {
                    CVan( value, &tmp );
                    sum += tmp;
                 }
              }

              /* Get the memory allocation for each DMF area enabled */

              sum += dmf_cache_size(cache_type, cache_name);

              if (sum > max_dbms_mem) max_dbms_mem = sum;
           }
           STprintf( result, ERx( "%d" ), max_dbms_mem );
        }
	else if( STcompare( argv[ 1 ], ERx( "-max_dbms_stack")) == 0 )
	{
           i8 sum, max_dbms_stack = 0;
           i4 n;
	   i8 tmp;
           char *dbms_name;

           STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
           exp = PMexpToRegExp( tmpbuf );

           for(
                        status = PMscan( exp, &state1, NULL, &name, &startup );
                        status == OK;
                        status = PMscan( NULL, &state1, NULL, &name,
                        &startup ) )
           {
              CVan( startup, &n );
               
              /* Only interested in Servers which are started by Ingstart */

              if( n < 1 ) continue;

              dbms_name = PMgetElem( 3, name );

              sum = num_threads( dbms_name );

              STprintf( expbuf, ERx( "%s.$.dbms.%s.stack_size" ),
                                  SystemCfgPrefix, dbms_name );

              if( PMget( expbuf, &value ) != OK )  continue;

              /* Now, multiple number of sessions by stack size */

              CVal8( value, &tmp );
              sum *= tmp;

              if (sum > max_dbms_stack) max_dbms_stack = sum;
           }
           STprintf( result, ERx( "%lld" ), max_dbms_stack );
	}
        else if ( STcompare( argv[ 1 ], ERx( "-dbms_swap_mem")) == 0 )
        {     
           u_i4 i; 
	   i8 dbms_swap_mem = 0;
           i4   n;
	   i8 tmp;
           char *dbms_name;
           CACHE_LIST *head, *cur;

           head = NULL;

           /* DBMS Swap mem is calculated as follows.
           **
           **   For each server type
           **               Add Stack_Size * Number of Sessions * Number of server type started
           **               Add "memory_areas" * Number of server type started
           **   For each server type using Private DMF cache
           **               Add "ON" cache area sizes * Number of server type started
           **   For each server using Shared cache.
           **               Add "ON" cache areas ONCE per cache name.
           */

           STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
           exp = PMexpToRegExp( tmpbuf );

           for(
                        status = PMscan( exp, &state1, NULL, &name, &startup );
                        status == OK;
                        status = PMscan( NULL, &state1, NULL, &name,
                        &startup ) )
           {
              char *cache_type;
              char *cache_name;
              char search_str [500];
              bool include_cache;
              i4   num_caches;

              CVan( startup, &n );
               
              /* Only interested in Servers which are started by Ingstart */

              if( n < 1 ) continue;

              dbms_name = PMgetElem( 3, name );


              STprintf( expbuf, ERx( "%s.$.dbms.%s.stack_size" ),
                                  SystemCfgPrefix, dbms_name );

              if( PMget( expbuf, &value ) != OK )  continue;

              /* Now, multiple number of sessions by stack size and number of
              ** servers started */

	      CVal8( value, &tmp );
              dbms_swap_mem += num_threads(dbms_name) * tmp * n;


              /* Determine whether the server is using private or shared cache */

              STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_sharing" ),
                                  SystemCfgPrefix, dbms_name );

              if( PMget( expbuf, &value ) != OK )  continue;

              if ( ! ValueIsBoolTrue( value ) )
              {
                 cache_type    = "private";
                 cache_name    = dbms_name;
                 include_cache = TRUE;
                 num_caches    = n;
              }
              else
              {
                 cache_type   = "shared";

                 /* Get the shared cache name */

                 STprintf( expbuf, ERx( "%s.$.dbms.%s.cache_name" ),
                           SystemCfgPrefix, dbms_name );

                 if( PMget( expbuf, &cache_name) != OK ) continue;

                 /* Have we included this cache already ? */

                 for( cur = head; cur != NULL; cur = cur->next )
                    if( STcompare( cur->name, cache_name) == 0 )
                                        break;
                 if ( cur )
                 {
                    include_cache = FALSE;
                 }
                 else
                 {
                    include_cache = TRUE;
                    num_caches    = 1;

                    cur = (CACHE_LIST *)  MEreqmem( 0,
                                        sizeof( CACHE_LIST), FALSE,
                                        NULL ); 

                    if( cur == NULL ) ERROR( "MEreqmem() failed." );

                    cur->name = value;
                    cur->next = head;
                    head = cur;
                 }
              }

              /* Get the memory allocation for each memory segment */

              for ( i = 0; i < sizeof(memory_area)/sizeof(char *); i++)
              {
                 STprintf( expbuf, ERx("%s.$.dbms.%s.%s"), SystemCfgPrefix,
                           dbms_name, memory_area [i]);

                 if( PMget( expbuf, &value ) == OK )
                 {
		    CVal8( value, &tmp );
                    dbms_swap_mem += tmp * n;
                 }
              }

              if (! include_cache ) continue;

              /* Get the memory allocation for each DMF area enabled */

              dbms_swap_mem += dmf_cache_size(cache_type, cache_name) * num_caches;
           }
           STprintf( result, ERx( "%lld" ), dbms_swap_mem);
        }
	else if( STcompare( argv[ 1 ], ERx( "-logwriter_sum" ) ) == 0 )
	{
		i4 n, sum = 0;

		STprintf(tmpbuf, ERx("%s.$.ingstart.%%.dbms"), SystemCfgPrefix);
		exp = PMexpToRegExp( tmpbuf );

		for(
			status = PMscan( exp, &state1, NULL, &name, &startup );
			status == OK;
			status = PMscan( NULL, &state1, NULL, &name, &startup ) )
		{
			i4 n;

			CVan( startup, &n ); 

			if( n < 1 )
				continue;
		
			STprintf( expbuf, ERx( "%s.$.dbms.%s.log_writer" ),
				  SystemCfgPrefix, PMgetElem( 3, name ) );

			exp = PMexpToRegExp( expbuf ); 

			if( PMscan( exp, &state2, NULL, &name, &value ) == OK
				|| PMget( expbuf, &value ) == OK )
			{
				i4 tmp; 

				CVan( value, &tmp );
				sum += n * tmp;
			}
		}

		/*
		** Bug 122263
		**  Also check recovery logwriter(s) 
		**  - which may be stand alone 
		*/
		STprintf(expbuf, ERx("%s.$.recovery.$.log_writer"),
			 SystemCfgPrefix);

		if (PMget(expbuf, &value) == OK)
		{
		    i4 tmp;
		    CVan(value, &tmp);
		    sum = sum + tmp;
		}
		STprintf( result, ERx( "%d" ), sum );
	}
       	else
		PCexit( FAIL );

	if( argc < 4 )
	{
		/* send result to standard output */
		F_PRINT( ERx( "%s\n" ), result );
		PCexit( OK );
	}
	/* otherwise, send to temporary file passed in argv[ 2 ] */
	STlcopy( argv[ 2 ], locbuf, sizeof( locbuf ) );
	LOfroms( PATH & FILENAME, locbuf, &fileloc );
	if( SIfopen( &fileloc , ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp )
		!= OK )
	{
		PCexit( FAIL );	
	}
	SIputrec( result, fp );
	SIclose( fp );
	PCexit( OK );
}

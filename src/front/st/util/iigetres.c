/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: iigetres.c -- contains main() of a program which displays the
**	value of a named INGRES configuration resource. 
**
**  Usage:
**	iigetres resource [ vms_logical ]
**
**  Description:
**	Looks up a value in the default PM configuration file (config.dat)	
**	and prints it to standard output.  On VMS, if program is called with
**	second argument, the value is assigned to a VMS process-level logical.
**
**  Exit Status:
**	OK	the value of the resource was displayed.
**	FAIL	the resource is undefined.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	18-jan-92 (tyler)
**		Added code to optionally return a value by creating
**		a supervisor mode process level logical on VMS - by 
**		calling lib$set_logical().
**	1-mar-93 (tyler)
**		Replaced call to lib$set_logical() with call to
**		sys$crelnm() in order to create an executive mode
**		(rather than supervisor mode) logical.
**	22-mar-93 (tyler)
**		Added error message on PMload() failure and changed
**		exit status returned when PMget() fails to OK.
**	23-jul-93 (tyler)
**		Added MALLOCLIB to NEEDLIBS list.  PMerror() is now
**		passed to PMload() for better error reporting.
**		<util.h> is now included.
**      08-30-93 (huffman)
**              add <st.h>
**	21-oct-96 (mcgem01)
**		Modify usage message so that this exe can be used
**		by Jasmine.
**	21-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	11-dec-2009 (whiro01)
**		Calling PMsetDefault (0) and (1) to set the prefix and host
**		defaults so that certain common wildcards will work.
**		Implemented "*" as the name to enumerate all values (except
**		on VMS when 3 args are given (doesn't make sense).
**		Calling "PMscan" for arguments that have wildcards.
**      04-Jan-2010 (horda03)
**              Fix VMS build failure introduced on 11-dec-2009.
*/

/*
PROGRAM =	(PROG1PRFX)getres
**
NEEDLIBS =	UTILLIB COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <cm.h>
# include <gl.h>
# include <me.h>
# include <pc.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <si.h>
# include <st.h>
# include <simacro.h>
# include <util.h>

# ifdef VMS
# include <ssdef.h>
# include <descrip.h>
# include <libdef.h>
# endif /* VMS */


main(argc, argv)
int	argc;
char	*argv[];
{
	char *value;
	bool usage = TRUE;
	bool allwild = FALSE;
# ifdef VMS
	struct dsc$descriptor lnm_table;
	struct dsc$descriptor lnm_name;
	struct dsc$descriptor lnm_value;
	STATUS status;
# endif /* VMS */

	MEadvise( ME_INGRES_ALLOC );

	if( argc == 2 )
		usage = FALSE;
# ifdef VMS
	else if( argc == 3 )
	{
		usage = FALSE;

		lnm_table.dsc$w_length = (unsigned short)
			STlength( ERx( "LNM$PROCESS" ) );
		lnm_table.dsc$a_pointer = ERx( "LNM$PROCESS" ); 
		lnm_table.dsc$b_dtype = 0;
		lnm_table.dsc$b_class = 0;

		lnm_name.dsc$w_length = (unsigned short)
			STlength( argv[ 2 ] );
		lnm_name.dsc$a_pointer = argv[ 2 ]; 
		lnm_name.dsc$b_dtype = 0;
		lnm_name.dsc$b_class = 0;
	}
# endif /* VMS */

	if( usage )
	{
# ifdef VMS
		SIprintf( ERx( "\nUsage: %s name [ logical ]\n\n" ), argv[0] );
# else /* VMS */
		SIprintf( ERx( "\nUsage: %s name\n\n" ), argv[0] );
# endif /* VMS */
		PCexit( FAIL );
	}

	PMinit();
	if( PMload( NULL, PMerror ) != OK )
		PCexit( FAIL );

	/* Set common default prefixes so that certain wildcards will work:  */
	/* "$.$.privileges.user.xxx" or "!.privileges.user.xxx" for instance */
	PMsetDefault( 0, SystemCfgPrefix );
	PMsetDefault( 1, PMhost() );

	/* Check for total wildcard argument ("*") to list all values */
	/* or for individual wildcard '%' segments                    */
	
	if ( STchr( argv[ 1 ], '%' ) != NULL ||
	     ( allwild = STcompare( argv[ 1 ], ERx( "*" ) ) == 0 ? TRUE : FALSE ) )
	{
		PM_SCAN_REC state;
		char *regexp, *p;
		char pmexp[ 10 ];
		STATUS status;
		char *name, *value;
#ifdef VMS
		if ( argc == 3 )
		{
			SIprintf( ERx( "\nUsage: %s name [ logical ]\n"), argv[0] );
			SIprintf( ERx( "  Cannot specify logical name with wildcard argument: '%s'!\n"), argv[ 1 ] );
			PCexit( FAIL );
		}
#endif
		MEfill( sizeof( PM_SCAN_REC ), 0, &state );
		if( allwild )
		{
			STprintf( pmexp, "%s.*", SystemCfgPrefix );
			for( p = pmexp; *p != EOS; p++ )
				CMtoupper( p, p );
			regexp = pmexp;
		}
		else
			regexp = PMexpToRegExp( argv[ 1 ] );
		p = regexp;
		while( ( status = PMscan( p, &state, NULL, &name, &value )) == OK )
		{
			SIprintf( "%s=%s\n", name, value );
			p = NULL;
		}
		SIflush( stdout );
	}		
	else if( PMget( argv[ 1 ], &value ) == OK )
	{
# ifdef VMS
		if( argc == 3 )
		{
			lnm_value.dsc$w_length = (unsigned short)
				STlength( value );
			lnm_value.dsc$a_pointer = value;
			lnm_value.dsc$b_dtype = 0;
			lnm_value.dsc$b_class = 0;

			status = lib$set_logical( &lnm_name, &lnm_value,
				&lnm_table, 0, 0 );

			if( status != SS$_NORMAL && status != SS$_SUPERSEDE )
				F_ERROR( "Unable to set logical \"%s\"", argv[ 2 ] );

			PCexit( OK );
		}
# endif /* VMS */
		F_PRINT( "%s\n", value );
		PCexit( OK );
	}

# ifdef VMS
	if( argc == 3 )
	{
		status = lib$delete_logical( &lnm_name, &lnm_table );

		if( status != SS$_NORMAL && status != SS$_NOLOGNAM ) 
			F_ERROR( "Unable to delete logical \"%s\"", argv[ 2 ] );
	}
# endif /* VMS */

	PCexit( OK );
}

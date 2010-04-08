/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <pc.h>
#include    <cs.h>
#include    <si.h>
#include    <st.h>
#include    <cx.h>
#include    <cv.h>

/**
**
**  Name: IINUMAINF - Get NUMA information about current box
**
**  Description:
**      This program fills the need (lacking in Tru64 5.1), for a
**	command line interface for querying basic NUMA information
**	required for setting up Ingres.
**
**	Currently used in 'iisudbms' shell script to see if user
**	should be prompted for NUMA specific options.
**
**          main() - The iinumainfo main program
**
**
**  History:
**	19-sep-2002 (devjo01)
**	    Initial version.
**	10-dec-2004 (abbjo03)
**	    Include si.h.
**/

/*
PROGRAM =	iinumainfo
**
NEEDLIBS =	COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/


static void usage( char * );

static void dump_rad_info( i4 );

/*{
** Name: main()	- iinumainfo main program.
**
** Description:
**	Echo back aspects of OS NUMA configuration.
**
** Inputs:
**	argc		argument count
**	argv		argument string vector
**
** Outputs:
**      none
**
**	Returns:
**	    via PCexit
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-sep-2002 (devjo01)
**	    Initial version.
**	29-Sep-2009 (frima01) 122490
**	    Add include of cv.h and st.h to avoid gcc 4.3 warnings.
*/

main(argc, argv)
int	argc;
char	*argv[];
{
    STATUS	status = OK;
    CL_ERR_DESC err_code;
    i4		rad, numrads;
    char	*argp;

    if ( argc > 3 )
	usage( argv[3] );

    if ( argc > 1 )
    {
	argp = argv[1];

	if ( '-' != *argp++ )
	    usage( --argp );

	if ( 0 == STcompare( argp, "radcount" ) )
	{
	    /*
	    ** Echo # of Supported RADs, or "0" if no support.
	    */
	    if ( argc > 2 )
		 usage( argv[2] );
	    SIprintf("%d\n", CXnuma_rad_count());
	}
	else if ( 0 == STcompare( argp, "radinfo" ) )
	{
	    /*
	    ** Echo information for one RAD, or all rads.
	    */
	    numrads = CXnuma_rad_count();
	    if ( argc > 2 )
	    {
		if ( OK == CVan( argv[2], &rad ) &&
		     rad > 0 &&
		     rad <= numrads )
		{
		    dump_rad_info(rad);
		}
	    }
	    else
	    {
		for ( rad = 1; rad <= numrads; rad++ )
		{
		    dump_rad_info(rad);
		}
	    }
	}
	else
	{
	    usage( --argp );
	}
    }
    else
    {
	usage( NULL );
    }

    PCexit(OK);
}

/*
** Complain about bad parameter, display correct usage, & scram.
*/
static void
usage( char *badparam )
{
    if ( badparam )
    {
	SIfprintf( stderr,  "Bad parameter! (%s)\n\n", badparam );
    }

    SIfprintf( stderr,  "Usage: iinumainfo -radcount | -radinfo [ <rad#> ]\n\n" );
    PCexit(FAIL);
}

/*
** Dump information about one or more RADS to stdout.
*/
static void
dump_rad_info( i4 rad )
{
    STATUS		status;
    CX_RAD_INFO		rad_info;
    CX_RAD_DEV_INFO	*prdi;
    char		buf[16];
    i4			i;

    status = CXnuma_get_info(rad, &rad_info);
    if ( OK != status )
    {
	SIfprintf( stderr, "Fail to get RAD info for RAD#%d\n", rad);
	return;
    }
    SIprintf( "RAD%3d: OSid = %-16.16s  %3d CPUs %12dK memory\n",
      rad, rad_info.cx_radi_os_id_decoder(rad_info.cx_radi_os_id,buf,16),
      rad_info.cx_radi_cpus, rad_info.cx_radi_physmem );
    if ( rad_info.cx_radi_numdevs )
    {
	prdi = rad_info.cx_radi_devs;
	for ( i = 0; i < rad_info.cx_radi_numdevs; i++ )
	{
	    SIprintf( "  %s\n", prdi->cx_rdi_name );
	}
    }
}

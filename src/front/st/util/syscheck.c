/*
**  Copyright (c) 1992, 2004 Ingres Corporation
**
**  Name: syscheck.c -- contains main() for a program which replaces the
**	Bourne shell utility "syscheck" to check whether the local host
**	has sufficient resources to run Ingres as configured.
**
**  Usage:
**	syscheck [ -v ] [ -f ] [ -o outfile ]
**
**  Description:
**	Rather than parsing output from the (now defunct) rcheck utility,
**	this implementation of syscheck makes direct calls to the RLcheck()
**	function to obtain current limits on system resources.
**
**  Exit Status:
**	OK	The operating system has sufficient resources to run INGRES
**		as currently configured.	
**	FAIL	The operating system does not have sufficient resources to
**		run INGRES.		
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		The names of missing (PM) resources are now expanded from
**		their contracted "request" forms before display.
**	23-jul-93 (tyler)
**		PMerror() is now passed to PMload() for better error
**		reporting. 
**	04-aug-93 (tyler)
**		Improved output format.
**	20-sep-93 (tyler)
**		More output format changes.
**	19-oct-93 (tyler)
**		More output format changes.  Call PMhost() instead of
**		GChostname().  Added -v option to display all required
**		and available resources.  Fixed bug which caused
**		resource requirement not to be displayed when soft
**		limit was exceeded and hard limit supported but not
**		exceeded.  Added support for writing output to a
**		specified file.
**	22-nov-93 (tyler)
**		Don't always exit with failure status if -v flag
**		specified.
**	16-dec-93 (tyler)
**		Diplay value returned by GChostname() rather than
**		PMhost() in "Checking ..." message. 
**	31-jan-94 (tyler)
**		Switched from "INGRES" to "OpenINGRES".  Also fixed
**		error in shared memory tests.
**	31-aug-95 (tutto01)
**		Changed string to OpenIngres.
**	10-dec-96 (reijo01)
**		Change ii and OpenIngres literals to use generic 
**		SYSTEM_CFG_PREFIX and SYSTEM_PRODUCT_NAME after adding gl.h.
**		Also call SI functions directly bypassing those silly macros.
**	07-apr-97 (hanch04)
**		Added dummy main for nt
**              Delete space at the end of ming hint 
**	26-sep-1997 (mosjo01)
**		SCO OpenServer restricts RLcheck (nlist) from reading
**		/stand/unix (must be root). nlist has been bypassed in
**		rl.c (find_symbol), thus 0 values for limit returned.
**		Game of charades to make syscheck run with OK results.
**		If we did not do this, ingbuild would be run in two parts
**		to allow user to set suid and owner=root for syscheck.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	01-jul-1998 (kosma01)
**		Syscheck does not work on SGI yet. This must be fixed.
**		It reports the amount of shared memory available is zero,
**		even tho there is some available. Correct this even if
**		the desicion is to accept zero as a valid value for SHMEM.
**	09-jul-98 (i4jo01)
**		Do not fail on DYNIX/ptx for low shmmni values. Versions
**		after 4.2 changed the structure of shminfo. Allow syscheck
**		to not report failure for insufficient number of shared
**		memory identifiers.
**	03-nov-1998 (somsa01)
**		Backed out change #434627.  (Bug #91887)
**	03-Dec-1998 (kosma01)
**		Syscheck on SGI now works, find_symbols64 in rl.c was using
**		32 bit locate_kmem calls, now it uses 64 bit locate_kmem 
**		calls if necessary.
**	23-Jul-1999 (merja01)
**              Changed echo_reqd to volatile to prevent data corruption on
**              axp_osf.  
**	11-may-2000 (somsa01)
**		Added changes required for other product builds.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-Jun-2000 (hweho01)
**          Changed limit from 4-byte to 8-byte for ris_u64, since the 
**          size of maximum shared memory segment has been increased to 
**          2GB in AIX 4.3.1.
**      06-Nov-2000 (hweho01)
**          Changed limit from 4-byte to 8-byte for axp_osf, since  
**          the size of shmmax in shminfo structure has been increased. 
**          
**      11-Nov-2000 (ahaal01)
**          Changed limit from 4-byte to 8-byte for rs4_us5, since the 
**          size of maximum shared memory segment has been increased to 
**          2GB in AIX 4.3.1.
**	25-oct-2001 (somsa01)
**	    Changed "int_wnt" to "NT_GENERIC".
**	10-Sep-2002 (hanch04)
**	    If we are building the 32/64-bit (conf_ADD_ON64), then exec the 64-bit syscheck
**	    for Solaris.
**	23-Sep-2002 (hanch04)
**	    Added BitsInKernel() fuction to show weather a 32 or 64-bit 
**	    kernel is running.  Use syscheck -kernel_bits to return
**	    just the bit size of the kernel(32 or 64).
**	    Need to be platform specific.  
**	26-Sep-2002 (somsa01)
**	    On HP-UX, include unistd.h to satisfy the sysconf() function.
**	    Also, fixed typo in printout from BitsInKernel().
**	28-Oct-2002 (hanch04)
**	    Don't exit on failure in PCdoexeclp64().
**      30-Oct-2002 (hweho01)
**          On AIX, include sys/systecfg.h to enable the __KERNEL_64()  
**          call which is used to determine the bit size of running 
**          kernel. 
**	14-Nov-2002 (hanch04)
**	    Don't exit on failure after PCexeclp64(),
**	    continue on and run the 32-bit version.
**	07-May-2003 (hanch04)
**	    syscheck will not exit FAIL if it thinks that the system does not
**	    have sufficient resources to run Ingres.  It will exit OK.
**	    If II_DISABLE_SYSCHECK is set, then it immediately exits OK.
**	    The [ -f ] flag will force syscheck to revert to the old behavior 
**	    and return fail if the system does not have sufficient resources 
**	    to run Ingres. 
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	13-oct-2003 (hayke02)
**	    Remove ADD_ON64 from around the call to BitsInKernel() so that
**	    we soft fail when running 32 bit syscheck on 64 bit Solaris, but 
**	    still allow -kernel_bits to run - see new bool bits32_64.
**	    This change fixes bug 111071.
**	02-Jan-2003 (bonro01)
**	    Fix BitsInKernel() for sgi.
**	08-Mar-2004 (hanje04)
**	    SIR 107679
**	    Improve syscheck functionality for Linux:
**	    Define more descriptive error codes so that ingstart can make a 
**	    more informed decision about whether to continue or not. (generic)
**	    Extend linux functionality to check shared memory values.
**	    Add -c flag and the ability to write out kernel parameters needed
**	    by the system in order to run Ingres.
**	23-mar-2004 (penga03)
**	    Moved "# include <sys/shm.h>" between #ifdef LNX and #endif.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      06-Aug-2004 (zhahu02)
**          Updated for INGINS133/b108541 for Solaris9 no segments.
**	07-Oct-2004 (drivi01)
**	    Changed "rl.h" to <rl.h> reference and moved header into front/st/hdr.
**	08-Nov-2004 (bonro01)
**	    Moved II_DISABLE_SYSCHECK test earlier in the program so that it
**	    checks this flag before trying to run the 64-bit version.
**	10-Nov-2004 (bonro01)
**	    Fix syscheck to support 8-byte integer values to prevent syscheck
**	    from reporting that there are not enough resources available.
**	24-Dec-2004 (shaha03)
**	    SIR #113754 Added condition for i64_hpu to invoke the 64-bit 
**	    instance incase of hybrid-build. 
**	15-Mar-2005 (bonro01)
**	    Add 64-bit kernel check for a64_sol.
**	28-Mar-2005 (shaha03)
**	   Removed i64_hpu from the condition to invoke 64-bit instance for 
**	   reverse hrid, to revert back the change done for hybrid build.
**	15-May-2005 (bonro01)
**	   Support for a64_sol on Solaris 10
**	   Use i8 values for all Platforms.
**	14-Nov-2006 (bonro01)
**	    shminfo values on Linux may be 32-bit or 64-bit depending on
**	    the kernel and a 32-bit syscheck would have difficulty retrieving
**	    a 64-bit kernel value.  shmmax and shmmni can be retrieved from
**	    /proc/sys/kernel regardless of kernel or executable type.
**	8-Aug-2007 (bonro01)
**	    Fix BitsInKernel() for Linux to detect 64bit Kernels.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

/*
PROGRAM =	(PROG0PRFX)syscheck
**
NEEDLIBS =	UTILLIB COMPATLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <si.h>
# include <lo.h>
# include <pm.h>
# include <pc.h>
# include <gc.h>
# include <nm.h>
# include <cv.h>
# include <st.h>
# include <gcccl.h>
# include <util.h>
# include <simacro.h>

# if defined(sparc_sol) || defined(a64_sol)
# include <sys/systeminfo.h>
# endif /* solaris */

# if defined(thr_hpux) || defined(sgi_us5)
# include <unistd.h>
# endif

# if defined(any_aix)
# include <sys/systemcfg.h>
# endif /* aix */

GLOBALREF STATUS exitvalue;  

# ifdef LNX
# include <sys/utsname.h>

GLOBALREF struct _shminfo shminfo;
# endif

# ifndef VMS
# if defined( NT_GENERIC )

main()
{
        printf("Ingres SYSCHECK\n");
        exit(0);
}
#else /* NT_GENERIC */

# include <rl.h>

static i4 BitsInKernel();
static VOID gen_config( i4 rl_limit, i8 value, FILE *outfp );


main( argc, argv )
int argc;
char **argv;
{
	char buf[ BIG_ENOUGH ];
        i8 limit;
	bool fail = FALSE;
	bool supported[ RL_END_OF_LIST ];
	i4 i;
	i8 maxreq = 0; /* maximum value required for arbitrary limit */
	bool verbose = FALSE;
	bool usage = TRUE;
	bool bitsin = FALSE;
	bool genconf = FALSE;
#if defined(sparc_sol) && !defined(LP64) && !defined(conf_BUILD_ARCH_32_64)
	bool bits32_64 = FALSE;
#endif /* pure 32 bit */
	LOCATION outloc;
	char outbuf[ MAX_LOC + 1 ];
	FILE *outfp = stdout;
	char host[ GCC_L_NODE + 1 ];
	char *disabled = NULL;

	NMgtAt( ERx( "II_DISABLE_SYSCHECK" ), &disabled );
	if( disabled != NULL && *disabled != EOS)
	{
	    PCexit( OK );
	}

#if defined(sparc_sol) && !defined(LP64)
	if(BitsInKernel() == 64 )
        {
#if defined(conf_BUILD_ARCH_32_64)
		PCdoexeclp64(argc, argv, FALSE);
#else
		bits32_64 = TRUE;
#endif /* hybrid */
        }
#endif /* sparc && !LP64 */

	MEadvise( ME_INGRES_ALLOC );

	if( argc == 1 )
		usage = FALSE;
	else
	{
		i4 argi = 1;

		if( argi < argc && 
		    STequal( argv[ argi ], ERx( "-f" ) ) )
		{
			exitvalue = FAIL;
			usage = FALSE;
			++argi;
		}

		if( argi < argc && 
		   STequal( argv[ argi ], ERx( "-v" ) ) )
		{
			verbose = TRUE;
			usage = FALSE;
			++argi;
		}

		if( argi < argc && 
		   STequal( argv[ argi ], ERx( "-c" ) ) )
		{
			verbose = FALSE;
			usage = FALSE;
			genconf = TRUE;
			++argi;
		}
		if( argi < argc && 
		    STequal( argv[ argi ], ERx( "-kernel_bits" ) ) )
		{
			bitsin = TRUE;
			usage = FALSE;
			++argi;
		}


		if( argi < argc - 1 && STequal( argv[ argi ], ERx( "-o" ) ) )
		{
			/* send output to temporary file name */
			usage = FALSE;
			STlcopy( argv[ argi + 1 ], outbuf, sizeof( outbuf ) );
			LOfroms( PATH & FILENAME, outbuf, &outloc );
			if( SIfopen( &outloc , ERx("w"), SI_TXT,
				SI_MAX_TXT_REC, &outfp ) != OK )
			{
				PCexit( FAIL );	
			}
		}
		else if( argi < argc - 1 )
			usage = TRUE;
	}

	if( usage )
		ERROR( "Usage: syscheck [ -f ] [ -v ] [ -c ] [ -kernel_bits ] [ -o outfile ] " );

	if( bitsin )
	{
	    SIfprintf( outfp, "%d\n", BitsInKernel());
	    SIclose( outfp );
	    PCexit( OK );
	}
#if defined(sparc_sol) && !defined(LP64) && !defined(conf_BUILD_ARCH_32_64)
	else
	{
	    if (bits32_64)
	    {
		SIprintf( "Unable to open a 64 bit kernel memory file /dev/kmem\n" );
		SIprintf( "\nAll shared memory resource checking has been disabled.\n\n" );
		return(FALSE);
	    }
	}
#endif /* su4_us5  && !LP64 && !hybrid */

	PMinit();

	PMlowerOn();

	if( PMload( NULL, PMerror ) != OK )
	{
		SIclose( outfp );
		PCexit( FAIL );
	}

	PMsetDefault( 0, ERx( SystemCfgPrefix ) );
	PMsetDefault( 1, PMhost() );
	PMsetDefault( 2, ERx( "syscheck" ) );

	GChostname( host, sizeof( host ) );
	if ( !genconf )
	SIfprintf( outfp, "\nChecking host \"%s\" for system resources required to run %s...\n\n", host, SystemProductName);

# ifdef sqs_ptx
	if ( verbose )
	    SIfprintf( outfp, "Note: Shared memory information may be inaccurate for DYNIX/ptx, use 'sysdef' for actual values.\n\n");
# endif
	if ( verbose )
	    SIfprintf( outfp, "%d-bit kernel running.\n\n", BitsInKernel());

	SIflush(outfp);

	for( i = 0; i < RL_END_OF_LIST; i++ )
		supported[ i ] = TRUE;

	/*
	** The following loop assumes that hard limits will always be
	** processed before corresponding soft limits, if at all. 
	*/
	for( i = 0; i < RL_END_OF_LIST; i++ )
	{
#if defined(sparc_sol)
        char isabuf1[1024];                                 
        sysinfo(SI_RELEASE,isabuf1, 1024);                  
        if(STrstrindex( isabuf1, "5.9", 1024, 1 ) != NULL )
        { 
          if ((i == RL_SYS_SHARED_NUM_SOFT)|| (i == RL_SYS_SHARED_NUM_HARD)) 
          continue;
        }
#endif /* sparc_sol */
                MEfill( (u_i2)sizeof(buf), '\0', buf );
		/* if the limit isn't supported, skip it */
		if( RLcheck( i, buf, 0, NULL ) != OK )
		{
			supported[ i ] = FALSE;
			continue;
		}
                CVal8( buf, &limit );
		/*
		** If limit returned is < 0, assume that it can't be
		** checked reliably.
		*/
		if( limit < 0 )
			continue;

		/* at this point, we know the OS supports the limit */
		switch( i )
		{
			i8 required;
			char *val;
			volatile bool echo_reqd; 

		case RL_PROC_FILE_DESC_HARD:

			echo_reqd = TRUE;
			if( PMget( ERx( "!.file_limit" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.file_limit" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				echo_reqd = FALSE;
				F_FPRINT( outfp, "%s file descriptors per-process required.\n", val );
				F_FPRINT( outfp, "%s is the current system hard limit.\n", buf );
				if( !supported[ RL_PROC_FILE_DESC_SOFT ] )
					FPRINT( outfp, ERx( "\n" ) );
			}
			if( required > limit )
			{
				fail = TRUE;
				exitvalue = RL_FILE_DESC_FAIL;
			}
			break;

		case RL_PROC_FILE_DESC_SOFT:

			if( PMget( ERx( "!.file_limit" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.file_limit" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				if( !supported[ RL_PROC_FILE_DESC_HARD ] ||
					echo_reqd )
				{
					F_FPRINT( outfp, "%s file descriptors per-process required.\n", val );
				}
				F_FPRINT( outfp, "%s is the current system soft limit.\n\n", buf );
			}
			if( required > limit &&
				! supported[ RL_PROC_FILE_DESC_HARD ] )
				fail = TRUE;
			break;

		case RL_PROC_FILE_SIZE_HARD:
			break;

		case RL_PROC_FILE_SIZE_SOFT:
			break;

		case RL_PROC_RES_SET_HARD:
			break;

		case RL_PROC_RES_SET_SOFT:
			break;

		case RL_PROC_CPU_TIME_HARD:
			break;

		case RL_PROC_CPU_TIME_SOFT:
			break;

		case RL_PROC_DATA_SEG_HARD:
			break;

		case RL_PROC_DATA_SEG_SOFT:
			break;

		case RL_PROC_STACK_SEG_HARD:
			break;

		case RL_PROC_STACK_SEG_SOFT:
			break;

		case RL_SYS_SHARED_SIZE_HARD:
		{
			bool show_limit = FALSE;
			maxreq = 0;

			if( PMget( ERx( "!.rcp_segment" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.rcp_segment" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
			    if ( genconf )
			    {
				maxreq = ( required > maxreq ? required: maxreq );
			    }
			    else
			    {
				F_FPRINT( outfp, "%s byte shared memory segment required by LG/LK sub-systems.\n", val );
				show_limit = TRUE;	
				if( required > limit )
                                {
                                    fail = TRUE;
                                    exitvalue = RL_SHARED_SIZE_FAIL;
                                }
			    }

			}
			if( PMget( ERx( "!.dbms_segment" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.dbms_segment" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
			    if ( genconf )
			    {
				maxreq = ( required > maxreq ? required: maxreq );
			    }
			    else
			    {
				F_FPRINT( outfp, "%s byte shared memory segment required by DBMS server(s).\n", val );
				show_limit = TRUE;	
				if( required > limit )
                                {
                                        fail = TRUE;
                                        exitvalue = RL_SHARED_SIZE_FAIL;
                                }
			    }

			}

			if( PMget( ERx( "!.dmf_segment" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.dmf_segment" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
			    if ( genconf )
			    {
				maxreq = ( required > maxreq ? required: maxreq );
			    }
			    else
			    {
				F_FPRINT( outfp, "%s byte shared memory segment required by shared DBMS cache(s).\n", val );
				show_limit = TRUE;	
				if( required > limit )
				{
					fail = TRUE;
					exitvalue = RL_SHARED_SIZE_FAIL;
				}
			    }
			}
			if( show_limit )
			{
				F_FPRINT( outfp, "%s bytes is the maximum shared memory segment size.\n\n", buf );
			}
			if ( genconf )
			    gen_config( RL_SYS_SHARED_SIZE_HARD, maxreq, outfp );

			break;
		}

		case RL_SYS_SHARED_SIZE_SOFT:
			break;

		case RL_SYS_SHARED_NUM_HARD:

			echo_reqd = TRUE;
			if( PMget( ERx( "!.segments" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.segments" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
# if defined (sqs_ptx)
	limit = required;
# endif
			if( verbose || required > limit )
			{
				echo_reqd = FALSE;
				F_FPRINT( outfp, "%s shared memory segments required.\n", val );
				F_FPRINT( outfp, "%s is the total number of shared memory segments allocated by the system.\n", buf );
				if( !supported[ RL_SYS_SHARED_NUM_SOFT ] )
					FPRINT( outfp, ERx( "\n" ) );
			}
			if( required > limit )
			{
				fail = TRUE;
				exitvalue=RL_SHARED_NUM_FAIL;
			}
			break;

		case RL_SYS_SHARED_NUM_SOFT:

			if( PMget( ERx( "!.segments" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.segments" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
# if defined (sqs_ptx)
	limit = required;
# endif
			if( verbose || required > limit )
			{
				if( !supported[ RL_SYS_SHARED_NUM_HARD ] ||
					echo_reqd )
				{
					F_FPRINT( outfp, "%s shared memory segments required.\n", val );
				}
				F_FPRINT( outfp, "%s shared memory segments are currently available.\n\n", buf );
			}
			if( required > limit )
				fail = TRUE;
			break;

		case RL_SYS_SEMAPHORES_HARD:

			echo_reqd = TRUE;
			maxreq = 0;
			if( PMget( ERx( "!.semaphores" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.semaphores" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				echo_reqd = FALSE;
				F_FPRINT( outfp, "%s semaphores required.\n", val );
				F_FPRINT( outfp, "%s is the total number of semaphores allocated by the system.\n", buf );
				if( !supported[ RL_SYS_SEMAPHORES_SOFT ] )
					FPRINT( outfp, ERx( "\n" ) );
			}
/*
			if ( genconf )
			{
			    maxreq = ( required > maxreq ? required: maxreq );
			}
			else
			{
			    F_FPRINT( outfp, "%s byte shared memory segment required by LG/LK sub-systems.\n", val );
			    show_limit = TRUE;	
*/
			    if( required > limit )
			    {
				fail = TRUE;
				exitvalue=RL_SEMAPHORES_FAIL;
			    }
/*
			}

			if ( genconf )
			    gen_config( RL_SYS_SEMAPHORES_HARD, maxreq, outfp );
*/
			break;

		case RL_SYS_SEMAPHORES_SOFT:

			if( PMget( ERx( "!.semaphores" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.semaphores" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				if( !supported[ RL_SYS_SEMAPHORES_HARD ] ||
					echo_reqd )
				{
					F_FPRINT( outfp, "%s semaphores required.\n",
						val );
				}
				F_FPRINT( outfp, "%s semaphores are currently available.\n\n", buf );
			}
			if( required > limit )
				fail = TRUE;
			break;

		case RL_SYS_SEM_SETS_HARD:

			echo_reqd = TRUE;
			if( PMget( ERx( "!.semaphore_sets" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.semaphore_sets" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				echo_reqd = FALSE;
				F_FPRINT( outfp, "%s semaphore sets required.\n", val );
				F_FPRINT( outfp, "%s is the total number of semaphore sets allocated by the system.\n", buf );
				if( !supported[ RL_SYS_SEM_SETS_SOFT ] )
					FPRINT( outfp, ERx( "\n" ) );
			}
			if( required > limit )
			{
				fail = TRUE;
				exitvalue = RL_SEM_SETS_FAIL;
			}
			break;

		case RL_SYS_SEM_SETS_SOFT:

			if( PMget( ERx( "!.semaphore_sets" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.semaphore_sets" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				if( !supported[ RL_SYS_SEM_SETS_HARD ] ||
					echo_reqd )
				{
					F_FPRINT( outfp, "\n%s semaphore sets required.\n",
						val );
				}
				F_FPRINT( outfp, "%s semaphore sets are currently available.\n\n", buf );
			}
			if( required > limit )
				fail = TRUE;
			break;

		case RL_SYS_SEMS_PER_ID_HARD:
			break;

		case RL_SYS_SEMS_PER_ID_SOFT:
			break;

		case RL_SYS_SWAP_SPACE_HARD:

			echo_reqd = TRUE;
			if( PMget( ERx( "!.swap_space_kb" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.swap_space_kb" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				echo_reqd = FALSE;
				F_FPRINT( outfp, "%sk bytes of swap space required.\n", val );
				F_FPRINT( outfp, "%sk bytes is total swap space allocated by the system.\n", buf );
				if( !supported[ RL_SYS_SWAP_SPACE_SOFT ] )
					FPRINT( outfp, ERx( "\n" ) );
			}
			if( required > limit )
			{
				fail = TRUE;
				exitvalue = RL_SWAP_SPACE_FAIL;
			}
			break;

		case RL_SYS_SWAP_SPACE_SOFT:

			if( PMget( ERx( "!.swap_space_kb" ), &val ) != OK )
			{
				STprintf( buf, "%s not found in configuration resource file.",
					PMexpandRequest( ERx( "!.swap_space_kb" ) ) );
				ERROR( buf );
			}
			CVal8( val, &required );
			if( verbose || required > limit )
			{
				if( !supported[ RL_SYS_SWAP_SPACE_HARD ] ||
					echo_reqd )
				{
					F_FPRINT( outfp, "%sk bytes of swap space required.\n", val );
				}
				F_FPRINT( outfp, "%sk bytes is the total swap space currently available.\n\n", buf );
			}
			if( required > limit )
				fail = TRUE;
			break;
		}
	}	
	if( fail )
	{
		SIfprintf( outfp, "Your system may not have sufficient resources to run %s as configured.\n\n", SystemProductName);
		SIclose( outfp );
	 	/*
		** OK   - default
		** FAIL - [ -f ] was passed
		*/
		PCexit( exitvalue );
	}
	if ( !genconf )
	SIfprintf( outfp, "Your system has sufficient resources to run %s.\n\n", SystemProductName );
	SIclose( outfp );
	PCexit( OK );
}
static i4
BitsInKernel()
{
    i4             bitsin = 32; /* default to 32 */

#if defined(sparc_sol)
    /*
    ** Check to see if the kernel is 32 or 64 bit.  We cannot read a
    ** 64 bit kernel in a 32 bit build.  Found this command running
    ** "truss isainfo -b"
    **  "sparcv9+vis sparcv9 sparcv8plus+vis sparcv8plus sparcv8 sparcv8-fsmuld sparcv7 sparc"
    */
    char isabuf[1024];
    sysinfo(SI_ISALIST,isabuf, 1024);
    if(STrstrindex( isabuf, "sparcv9", 1024, 1 ) != NULL )
    {
      bitsin = 64;
    }
    else
    {
      bitsin = 32;
    }
#endif
#if defined(a64_sol)
    /*
    ** Check to see if the kernel is 32 or 64 bit.  We cannot read a
    ** 64 bit kernel in a 32 bit build.
    */
    char isabuf[1024];
    sysinfo(SI_ISALIST,isabuf, 1024);
    if(STrstrindex( isabuf, "amd64", 1024, 1 ) != NULL )
    {
      bitsin = 64;
    }
    else
    {
      bitsin = 32;
    }
#endif
#if defined(axp_osf) || defined(i64_win) || \
        defined(i64_hpu) ||  defined(axp_lnx) || \
	defined(a64_win)
    /* platforms that were ALWAYS lp64 */
    bitsin = 64;
#endif
#if defined(sgi_us5)
    if( (i4)sysconf(_SC_KERN_POINTERS) == 64 )
        bitsin = 64;
    else
        bitsin = 32;
#endif
#if defined(thr_hpux)
    bitsin = (i4)sysconf(_SC_KERNEL_BITS);
#endif
#if defined(any_aix)
    if ( __KERNEL_64() )
       bitsin = 64 ;
    else  
       bitsin = 32 ;
#endif  /* aix */
#if defined(LNX)
    struct utsname uname_buf;

    uname(&uname_buf);
    if( strstr( uname_buf.machine, "x86_64" ) != NULL ||
        strstr( uname_buf.machine, "ia64" ) != NULL )
      bitsin = 64;
    else
      bitsin = 32;
#endif /* LNX */

    return( bitsin );
}

/*
** gen_config
**
** Prints out kernel parameters the values need to start up ingres.
** Obviously heavily platform dependent but function should attempt
** to use same format as kernel config files for that platform.
*/

# ifdef LNX
static VOID
gen_config( i4 rl_limit, i8 value, FILE *outfp )
{
    /* dump out kernel parameter based on rl_limit */
    switch ( rl_limit )
    {
    case RL_PROC_FILE_DESC_HARD:
    break;

    case RL_PROC_FILE_SIZE_HARD:
    break;

    case RL_PROC_RES_SET_HARD:
    break;

    case RL_PROC_CPU_TIME_HARD:
    break;

    case RL_PROC_DATA_SEG_HARD:
    break;

    case RL_PROC_STACK_SEG_HARD:
    break;

    case RL_SYS_SHARED_SIZE_HARD:
	/* print apropriate value for maximum shared memory segment */
	if ( shminfo.shmmax < value )
	    F_FPRINT( outfp, "kernel.shmmax=%lld\n",value );
	
	/* if system shared memory limit is less than 5 times the segment
	   limit print apropriate value */
	if ( (shminfo.shmall * 1024) < (5 *  value  ) ) 
		F_FPRINT( outfp, "kernel.shmall=%lld\n", value / ( 1024/5 ) ); 
    break;

    case RL_SYS_SHARED_NUM_HARD:
    break;

    case RL_SYS_SEMAPHORES_HARD:
    break;

    case RL_SYS_SEM_SETS_HARD:
    break;

    case RL_SYS_SEMS_PER_ID_HARD:
    break;

    case RL_SYS_SWAP_SPACE_HARD:
    break;
    }
}

# else /* Linux */
static VOID
gen_config( i4 rl_limit, i8 value, FILE *outfp )
{
}

# endif


# endif /* NT_GENERIC */
# endif /* VMS */

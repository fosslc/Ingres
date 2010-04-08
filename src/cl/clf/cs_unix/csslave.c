/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <clconfig.h>
# include    <systypes.h>
# include    <cs.h>
# include    <fdset.h>
# include    <csev.h>
# include    <er.h>
# include    <me.h>
# include    <pc.h>
# include    <st.h>
# include    <cx.h>

/**
**
**  Name: CSSLAVE.C - Slave process for dbms server on UNIX
**
**  Description:
**	This file builds a slave process for the dbms server by linking
**	various CL routines into a separate executable.
**
**          main() - Slave process main procedure
**
**
**  History:
 * Revision 1.4  89/01/29  18:21:26  jeff
 * New GC doesn't have a GC slave
 * 
 * Revision 1.3  88/08/25  17:10:07  mikem
 * make the executable be "slave"
 * 
 * Revision 1.2  88/08/24  11:30:14  jeff
 * cs up to 61 needs
 * 
 * Revision 1.2  88/04/18  13:06:37  anton
 * multi-user gca is ready
 * 
 * Revision 1.1  88/03/29  15:55:02  anton
 * Initial revision
 * 
**      03-mar-88 (anton)
**	    Created;
**	31-jan-1989 (roger
**	    Renamed executable to "iislave".
**	27-apr-1989 (boba)
**	    Add DEST ming hint to build executable in bin directory
**	    (default is now utility for cs).
**	5-june-90 (blaise)
**	    Integrated changes from ingresug:
**		Add include <clconfig.h> to pick up correct ifdefs in <csev.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      23-may-1994 (markm)
**          Make call to iiCL_increase_fd_table_size() to make sure we aquire
**          maximum number of file descriptors available to the slave.
**	16-aug-2001 (toumi01)
**	    move CSEV_CALLS from csslave.c to cssl.c so that it will be
**	    available in libcompat.a
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	23-sep-2002 (devjo01)
**	    Establish NUMA context.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**/


/*
**  Forward and/or External typedef/struct references.
*/


/*
**  Forward and/or External function references.
*/



/*
**  Defines of other constants.
*/
# define	CS_CX_SLAVE_RAD_ARG	5

/* Change following to undef to supress this for release. */
#undef  II_SLAVE_SPIN

#ifdef II_SLAVE_SPIN

i4	awoken = 0;

#endif


/* typedefs go here */

/*
** Definition of all global variables owned by this file.
*/


GLOBALREF	CSEV_CALLS	CScalls[];

/*
**  Definition of static variables and forward static functions.
*/

/*
PROGRAM =	iislave

NEEDLIBS =	COMPATLIB MALLOCLIB

DEST = 		bin
*/

main(argc, argv)
int	argc;
char	**argv;
{
    char        *envp;
    i4		 arg, rad;

    MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    {
	char	*lp64enabled;

	/*
	** Try to exec the 64-bit version
	*/
	NMgtAt("II_LP64_ENABLED", &lp64enabled);
	if ( (lp64enabled && *lp64enabled) &&
	   ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
	     !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
	{
	    PCexeclp64(argc, argv);
	}
    }
#endif  /* hybrid */

# ifdef II_SLAVE_SPIN
    NMgtAt(ERx( "II_SLAVE_SPIN" ), &envp);
    if ( envp && '1' == *envp )
    {
	PCpid(&rad);
	SIfprintf(stderr, ERx( "II_SLAVE_SPIN = %d" ), rad);
	while ( !awoken )
	{
	    sleep(1);
	}
    }
# endif

    if ( argc > CS_CX_SLAVE_RAD_ARG &&
	 (OK == CVan(argv[CS_CX_SLAVE_RAD_ARG], &rad)) &&
	 (rad > 0) &&
	 (OK == CXset_context(NULL, rad)) )
    {
	/*
	** Running NUMA clusters, now that we've established our RAD
	** context, eat the extra parameter.
	*/
	argc--;
	for ( arg = CS_CX_SLAVE_RAD_ARG; arg < argc; arg++ )
	    argv[arg] = argv[arg+1];
	argv[arg] = NULL;
	
    }
    iiCL_increase_fd_table_size(TRUE,0); 
    CS_init_slave(argc, argv);
    exit(1);
}

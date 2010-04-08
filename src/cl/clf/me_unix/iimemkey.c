/*
** Copyright (c) 1987, 2002 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <pc.h>
# include   <st.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <meprivate.h>
# include   <me.h>
# include   <cs.h>
# include   <cx.h>

# ifdef SYS_V_SHMEM
/* For SEQUENT Dynix 3 bsd, use specially imported shmops */
/*# include   <sys/ipc.h>*/
# include   <clipc.h>
# include   <sys/shm.h>
# endif

/**
**  Name: IIMKKEY.C - Prints the memory key for SYS 5 shared mem systems.
**
**  Description:
**	This module contains the code which implements IIMKKEY
**
**		Usage: ipcrm -M `iimemkey memoryfile`
**
**  History:
**	24-apr-1995 (canor01)
**	    Created to solve shared memory cleanup problems.
**	03-jun-1997 (canor01)
**	    Memory keys need to be 8 hex digits with leading zeros.
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_u42.
**	25-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	28-sep-2002 (devjo01)
**	    Add CXget_context, so NUMA cluster context can be established.
*/

/*
** mkming hints
PROGRAM = (PROG1PRFX)memkey

DEST = utility

NEEDLIBS =	COMPATLIB MALLOCLIB
*/

/*{
** Name: main			- print the shared memory key
**
** Description:
**	Used by build and install tools to clean up shared memory.
**
** Inputs:
**	None
**
** Outputs:
**
**	Returns:
**	    OK	- success
**	    !OK - failure (CS*() routine failure, segment not mapped, ...)
**	
**  History:
**	24-apr-1995 (canor01)
**	    Created to solve shared memory cleanup problems.
**	03-jun-1997 (canor01)
**	    Memory keys need to be 8 hex digits with leading zeros.
**      01-sep-1997 (rigka01) Bug #85043
**	    Memory keys need to be 8 hex digits with leading zeros.
**	06-mar-1998 (toumi01)
**	    Linux ipcs and ipcrm commands use a logical key, not the hex
**	    memory key used by most Unix systems.  Return that key from
**	    here and use it in a (modified) ipcclean script to issue
**	    commands like "ipcrm shm 4".
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	14-jun-2000 (hanje04)
**	    OS/390 Linux ipcs and ipcrm commands also use a logical key instead
**	    of a hex mem key.
**	    Added ibm_lnx to platforms that do. (i.e. int_lnx).
**	04-Dec-2001 (hanje04)
**	    Add IA64 Linux to logical keys change.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Don't need OSX specific output
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
int
main(int argc, char *argv[])
{
    key_t	key;

    MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    {
	char        *lp64enabled;
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

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
	PCexit(FAIL);

    if ( argc < 2 )
    {
        SIprintf( "Usage: %s MEMORYFILE\n", argv[0] );
        PCexit(FAIL);
    }

#if defined(LNX)
    {
	int	shmid;
	key = ME_getkey(argv[1]);
	if ((key = ME_getkey(argv[1])) != (key_t)-1)
	    if ((shmid = shmget(key, 0, 004)) != -1)
		SIprintf("%d\n", shmid);
    } 
#else
    if ((key = ME_getkey(argv[1])) != (key_t)-1)
	SIprintf("0x%08x\n", (i4)key);
#endif /* Linux */

    PCexit(OK);
}

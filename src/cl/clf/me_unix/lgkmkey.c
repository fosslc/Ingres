/*
**Copyright (c) 2004 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <meprivate.h>
# include   <me.h>
# include   <pc.h>
# include   <st.h>
# include   <cs.h>
# include   <cx.h>

# ifdef SYS_V_SHMEM
/* For SEQUENT Dynix 3 bsd, use specially imported shmops */
/*# include   <sys/ipc.h>*/
# include   <clipc.h>
# include   <sys/shm.h>
# endif

/**
**  Name: LGKMKEY.C - Prints the LGK memory key for SYS 5 shared mem systems.
**
**  Description:
**	This module contains the code which implements LGKMKEY
**
**		Usage: ipcrm -M `lgkmkey`
**
**  History:
**	21-nov-1992 (bryanp)
**	    Created to solve LG/LK memory cleanup problems.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      01-sep-1997 (rigka01) bug #85043
**          memory key needs to be 8 hex digits with leading zeros  
**      03-sep-1997 (rigka01) bug #85144
**          revert to previous revision due to the use of
**          lgkmkey in ipcclean being unnecessary 
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
**      28-sep-2002 (devjo01)
**          Add CXget_context, so NUMA cluster context can be established.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

/*
** mkming hints
PROGRAM = lgkmkey

DEST = utility

NEEDLIBS =	COMPATLIB MALLOCLIB
*/

/*{
** Name: main			- print the LG/LK shared memory key
**
** Description:
**	Used by build and install tools to clean up the LG/LK shared memory.
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
**	21-nov-1992 (bryanp)
**	    Created to solve LG/LK memory cleanup problems.
**	15-may-1995 (thoro01)
**	    Added NO_OPTIM  hp8_us5 to file.
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_u42.
**	28-sep-2002 (devjo01)
**	    Add CXget_context, so NUMA cluster context can be established.
*/
int
main(int argc, char *argv[])
{
    key_t	key;

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

    if ((key = ME_getkey("lglkdata.mem")) != (key_t)-1)
	SIprintf("0x%x\n", (i4)key);

    PCexit(OK);
}

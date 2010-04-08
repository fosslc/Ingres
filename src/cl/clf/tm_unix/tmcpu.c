/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include	"tmlocal.h"

/**
**
**  Name: TMCPU.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMcpu	- return cpu time used by process
**
**
** History:
 * Revision 1.2  88/08/10  14:25:20  jeff
 * added daveb's fixs for non-bsd
 * 
 * Revision 1.1  88/08/05  13:46:48  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:52:52  anton
**      use getrusage
**      
**      Revision 1.2  87/11/10  16:03:25  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Remove superfluous redeclaration of times() function.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: TMcpu	- CPU time used by process
**
** Description:
**      get CPU time used by process
**
** Inputs:
**
** Outputs:
**	Returns:
**	    milli secs used by process so far
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
*/
i4
TMcpu()
{
 
# ifdef xCL_003_GETRUSAGE_EXISTS
 
	struct	rusage	pstat;
	i4		cpu;

	getrusage(RUSAGE_SELF, &pstat);

	cpu = (pstat.ru_utime.tv_usec + pstat.ru_stime.tv_usec)
	    		/ MICRO_PER_MILLI +
	    (pstat.ru_utime.tv_sec + pstat.ru_stime.tv_sec) * MILLI_PER_SEC;

	return(cpu);
 
# else
 
	struct tms	tms;
	time_t		ticks;
 
	(void)times( &tms );
	ticks = tms.tms_utime + tms.tms_stime;
	return( ticks * MILLI_PER_SEC / HZ );
 
# endif
}

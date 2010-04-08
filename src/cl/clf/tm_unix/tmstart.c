/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include 	"tmlocal.h"

/**
** Name: TMSTART.C - Start system measurements
**
** Description:
**      This file contains the following tm routines:
**    
**      TMstart()        -  Start system measurements.
**
** History:
 * Revision 1.2  88/08/10  14:25:32  jeff
 * added daveb's fixs for non-bsd
 * 
 * Revision 1.1  88/08/05  13:46:56  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:54:03  anton
**      use getrusage
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/



/*{
** Name: TMstart	- Start system measurements
**
** Description:
**	Set timer for start information
**
** Inputs:
**
** Outputs:
**      time                            start time info
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
*/

VOID
TMstart(tm)
register TIMER          *tm;
{
	SYSTIME		stime;

# ifdef xCL_003_GETRUSAGE_EXISTS
 
	struct rusage	pstat;

	getrusage(RUSAGE_SELF, &pstat);
	TMnow( &stime );

	tm->timer_start.stat_usr_cpu	=
	    pstat.ru_utime.tv_usec / MICRO_PER_MILLI +
		pstat.ru_utime.tv_sec * MILLI_PER_SEC;

	tm->timer_start.stat_cpu	= tm->timer_start.stat_usr_cpu +
	    pstat.ru_stime.tv_usec / MICRO_PER_MILLI +
		pstat.ru_stime.tv_sec * MILLI_PER_SEC;

	tm->timer_start.stat_dio	= pstat.ru_inblock + pstat.ru_oublock;
	tm->timer_start.stat_bio	= pstat.ru_msgsnd + pstat.ru_msgrcv;
	tm->timer_start.stat_pgflts	= pstat.ru_majflt;
 
# else
 
	struct tms	tms;
 
	(void) times( &tms );
	TMnow( &stime );
 
	tm->timer_start.stat_usr_cpu	= tms.tms_utime * MILLI_PER_SEC / HZ;
	tm->timer_start.stat_cpu	= tms.tms_stime * MILLI_PER_SEC / HZ;
 
	/* no way to get these values.  5.0 kept a DI i/o count internally,
	   but there is no way to pass that back from a slave to a server */
	
	tm->timer_start.stat_dio	= 0;
	tm->timer_start.stat_bio	= 0;
	tm->timer_start.stat_pgflts	= 0;
 
# endif

	tm->timer_start.stat_wc		= stime.TM_secs;
}

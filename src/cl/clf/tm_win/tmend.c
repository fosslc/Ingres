/*
** Copyright (c) 1987, 1997 Ingres Corporation
*/

# include	<compat.h>
# include       <cs.h>
# include       <pc.h>
# include       <csinternal.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include 	"tmlocal.h"

/**
**
**  Name: TMEND.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMend	- end timer for end information.
**
**
** History:
 * Revision 1.2  88/08/10  14:25:24  jeff
 * added daveb's fixs for non-bsd
 * 
 * Revision 1.1  88/08/05  13:46:49  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:53:42  anton
**      use getrusage
**      
**      Revision 1.2  87/11/10  16:03:34  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-apr-95 (emmag)
**	    Desktop porting changes.
**      15-nov-1997 (canor01)
**          Strip out Unix-specific code, return better numbers.
**/

GLOBALREF CS_SYSTEM        Cs_srv_block;


/*{
** Name: TMend	- End system measurements
**
** Description:
**	Set timer for end information
**
** Inputs:
**      tm                              ptr to time info
**
** Outputs:
**      tm                              ptr to time info
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
**      06-jul-1987 (mmm)
**          Updated for jupiter unix cl.
*/
VOID
TMend(tm)
TIMER              *tm;
{
    register TIMERSTAT	    *start,
			    *end,
			    *value;
    SYSTIME		    stime;

    CSstatistics( &tm->timer_end, TRUE );

    TMnow( &stime );
    tm->timer_end.stat_wc = stime.TM_secs;

    start				= &tm->timer_start;
    end					= &tm->timer_end;
    value				= &tm->timer_value;

    value->stat_cpu			= end->stat_cpu - start->stat_cpu;
    value->stat_usr_cpu			= end->stat_usr_cpu -
					  start->stat_usr_cpu;
    value->stat_dio			= end->stat_dio - start->stat_dio;
    value->stat_bio			= end->stat_bio - start->stat_bio;
    value->stat_pgflts			= end->stat_pgflts - start->stat_pgflts;
    value->stat_wc			= end->stat_wc - start->stat_wc;
}

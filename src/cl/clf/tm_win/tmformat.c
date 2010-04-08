/*
** Copyright (c) 1987, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <tm.h>

/**
**
**  Name: TMFORMAT.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMformat	- format system measurements
**
**
** History:
 * Revision 1.1  88/08/05  13:46:50  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  16:03:49  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**/

/*{
** Name: TMformat	- format system measurements
**
** Description:
**      Format the information of the TIMER struct to human readable form
**      in buf.  The value of mode determines whether the information
**      formatted is for the start (TIMER_START), for the end (TIMER_END) 
**      or an intermediate (TIMER_VALUE) duration (TIMER_VALUE is the default
**      action).  ident is the user supplied identifier for the TIMER.
**      Buf is assumed to be large enough to hold it.
**
** Inputs:
**      tm                              ptr to time info to format
**      mode                            TIMER_START - start time
**                                      TIMER_END - end time
**                                      TIMER_VALUE - intermediate duration
**      ident                           user supplied identifier for timer
**      decchar                         decimal character to use
**
** Outputs:
**      buf                             formatted system measurements
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
TMformat(tm, mode, ident, decchar, buf)
TIMER              *tm;
i4                 mode;
char               *ident;
char		   decchar;
char		   *buf;
{
	TIMERSTAT	*stat;
	i4		tot_cpu,
			usr_cpu;
	i4		tot_min,
			tot_sec,
			tot_ms,
			tot_hrs;
	i4		usr_min,
			usr_sec,
			usr_ms,
			usr_hrs;


	switch(mode)
	{
		case TIMER_START:
			stat = &tm->timer_start;
			break;
		case TIMER_END:
			stat = &tm->timer_end;
			break;
		case TIMER_VALUE:
		default:
			stat = &tm->timer_value;
			break;
	}

	tot_cpu	= stat->stat_cpu;
	tot_ms	= (tot_cpu % 1000);
	tot_sec	= (tot_cpu /= 1000) % 60;
	tot_min	= (tot_cpu /= 60) % 60;
	tot_hrs	= (tot_cpu / 60);
	usr_cpu	= stat->stat_usr_cpu;
	usr_ms	= (usr_cpu % 1000);
	usr_sec	= (usr_cpu /= 1000) % 60;
	usr_min	= (usr_cpu /= 60) % 60;
	usr_hrs	= (usr_cpu / 60);

	/* 2 calls to STprintf() due to pyramid stack restriction */

 	STprintf(buf,
	    "TIMER-%-12s cpu = %01d%02d:%02d.%03d; usr = %01d%02d:%02d.%03d;\n",
	    ident,
	    tot_hrs, tot_min,  tot_sec, tot_ms,
	    usr_hrs, usr_min,  usr_sec, usr_ms);
	STprintf(buf+STlength(buf),
	    "                   dio = %-6d; pf = %-6d et = %-10.2f\n",
	    stat->stat_dio,
	    stat->stat_pgflts, stat->stat_wc / 60., decchar);
}

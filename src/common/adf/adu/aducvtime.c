/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <tm.h>
#include    "adutime.h"
#include    "adumonth.h"

/**
**
**  Name: ADUCVTIME.C - contains routines for making a time vector structure.
**
**  Description:
**        This file contains routines for converting time represented in 
**	seconds	into a timevect structure.
**
**          adu_cvtime() -	Convert time represented in seconds into a timevect
**			struct.
**
**  Function prototype defines in ADUTIME.H
**
**  History:    
**      30-apr-86 (ericj)
**	    Converted for Jupiter.  Made adu_cvtime() reference Adu_ii_dmsize instead
**	    of defining its own updatable(non-reentrant) month info array.
**	    Added parameter of a pointer to a timevect rather than returning
**	    a pointer to a static timevect.
**      31-dec-1992 (stevet)
**          Added function prototypes.
{@history_line@}...
**/


/*
** Name: adu_cvtime() - convert time(secs) to a structure.
**
** Description:
**	  Converts the standard INGRES time into a vector defining the
**	the components of the time and date. The struture returned
**	is defined in "timevect.h".
**
** Inputs:
**      time                            time to convert(secs. since Jan 1, 1970)
**	time_nsec_part			the nano second portion of the time for
**					high resolution time support.
**	t				Ptr to a timevect to be initialized.
** Outputs:
**	t
**	    .tm_sec			normalized seconds in the day.
**	    .tm_min			normalized minutes in the day.
**	    .tm_hour			normalized hours in the day.
**	    .tm_mon			normalized month in the year,
**					0 <= tm_mon <= 11
**	    .tm_year			normalized year, number of years since
**					1900?
**	    .tm_wday			day of week (modulo 7).
**	    .tm_yday			number of days in current year.
**	    
**  Returns:
**      VOID
**
**  Side effects:
**      none
**
**  History:
**      12/26/80 (peb)      -- first written for VMS
**	30-apr-86 (ericj)
**	    Converted for Jupiter.  Removed updatable array msize(non-reentrant)
**	    and reference Adu_ii_dmsize instead.  Added parameter, a pointer to
**	    a timevect struct to be updated, rather than returning a pointer
**	    to a static timevect struct.
**	13-jan-1987 (daved)
**	    avoid loop in calculating year
**	26-jul-2006 (gupsh01)
**	    Added time_nsec_part in the input parameters.
*/

VOID
adu_cvtime(
register i4	    time,
register i4	    time_nsec_part,
register struct timevect    *t)
{
    register i4     mo,
		    yr;
    i4		    mdays,
		    ydays;
    i4	    noyears;
    i4	    nodays;
    i4	    i;

    /* get seconds, minutes, hours */
    t->tm_sec   = time % 60;
    time        /= 60;
    t->tm_min    = time % 60;
    time        /= 60;
    t->tm_hour   = time % 24;
    time        /= 24;

    t->tm_nsec   = time_nsec_part;

    /* get day of week */
    t->tm_wday  = (time + 4) % 7;

    /* at this point, time is number of days since 1-jan-1970 */

    /* get the year. first, add enough days to get us to 1/1/1601.
    ** this date is special in that it is after a leapyear where
    ** the year is divisible by 400 and 100. The formula is good till
    ** the year 3012 or so.
    */
#define DAYS_SINCE_1601 134774		/* 1-1-1970 - 1-1-1601 */
    /* number of days since 1601 */
    i = time + DAYS_SINCE_1601;  
    /* number of years since 1601 */
    noyears = (i-i/1460+i/36500-i/146000)/365;	
    /* number of days to first day of current year */
    nodays = noyears*365 + noyears/4 - noyears/100 + noyears/400 
	- DAYS_SINCE_1601;

    time -= nodays;

    yr		= noyears + 1601;	/* xxxx year */
    t->tm_year  = yr - 1900;		/* offset from 1900 . why?? */
    t->tm_yday  = time;

    /* get month */
    TMyrsize(yr, &ydays);
    for (mo = 0; mo < 12; mo++)
    {
	if ((mo == 1) && (ydays == LEAPYEAR))
	    mdays = 29;
	else
	    mdays = Adu_ii_dmsize[mo];
	
	if (time >= mdays)
	    time -= mdays;
	else
	    break;
    }

    if (mo > 11)
	mo = 11;

    t->tm_mon   = mo;
    t->tm_mday  = time + 1;

    return;
}

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
#include    <tmtz.h>
#include    <cv.h>
#include    "tmlocal.h"

/**
**
**  Name: TM.C - Time routines.
**
**  Description:
**      This module contains the UNIX internal TM cl routines:
**
**	    TM_parse_timestamp	- take a timestamp string and returs secs and ms
**	    TM_lookup_month	- Search string table to find number of month.
**	    TM_monthsize	- how many days are in a given month.
** 	    TM_date_to_secs     - Convert year,month,day,minutes,secs to secs.
**
** History:
 * Revision 1.1  88/08/05  13:46:53  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**      25-sep-1992 (stevet)
**          Replaced the use of TMzone with TMtz routines for enhanced timezone
**          supports.
**      12-feb-1993 (smc)
**          Added forward declarations.
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**      21-oct-1996 (canor01)
**          Make TM_lookup_month() visible (for Jasmine).
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/
/* # defines */
/* typedefs */

/* forward references */
static STATUS TM_date_to_secs();

/* externs */
/* statics */


/*{
** Name: TM_parse_timestamp - parse a user input string that is a timestamp
**
** Description:
**    Parse a user input string which is a timestamp of the form:
**    (dd-mmm-yyyy{' '|':'}hh:mm:ss.cc).  Any part following the year
**    may be left off and defaults to 0.  Returns the number of seconds
**    since jan 1, 1970.
**
**
** Inputs:
**	input_string			    User input time string
**
** Output:
**	ret_seconds			    Return # of secs since jan 1, 1970
**	ret_usecs			    Return # usecs indicated by string
**
**      Returns:
**	    OK				    Successful parse.
**	    non zero			    Syntax error.
**
** History:
**      09-jul-87 (mmm)
**          Created.
*/
STATUS
TM_parse_timestamp(input_string, ret_seconds, ret_usecs)
char	*input_string;
i4     	*ret_seconds;
i4     	*ret_usecs;
{
    i4 		day	= 0;
    i4 		month	= 0;
    i4 		year	= 0;
    i4 		hour	= 0;
    i4 		minute	= 0;
    i4 		second	= 0;
    i4 		usecs	= 0;
    STATUS	ret_val = FAIL;
    char	tmp_buf[20];
    char	string_buf[TM_SIZE_STAMP + 1];
    char	*string = string_buf;
    i4     	secs_since_1970;

    for (;;)
    {
	/* break on errors */
	ret_val = FAIL;

	/* copy string to buffer we can manipulate */
	if (STlength(input_string) > TM_SIZE_STAMP)
	    break;
	else
	    STcopy(input_string, string);

	STzapblank(string, string);

	/* Now parse format correctly "dd-mmm-yyyy hh:mm:ss.cc" */

	/* dd- */
	tmp_buf[0] = *string++;
	tmp_buf[1] = *string++;
	tmp_buf[2] = '\0';
	if (*string++ != '-')
	    break;

	if (CVan(tmp_buf, &day))
	    break;

	/* mmm- */
	tmp_buf[0] = *string++;
	tmp_buf[1] = *string++;
	tmp_buf[2] = *string++;
	tmp_buf[3] = '\0';
	if (*string++ != '-')
	    break;

	if (TM_lookup_month(tmp_buf, &month))
	    break;

	/* yyyy{:} */
	tmp_buf[0] = *string++;
	tmp_buf[1] = *string++;
	tmp_buf[2] = *string++;
	tmp_buf[3] = *string++;
	tmp_buf[4] = '\0';
	if (*string == ':')
	    string++;

	if (CVan(tmp_buf, &year))
	    break;

	/* from this point on input is optional (and defaults to 0) */

	/* hh: */ 
	if (*string == '\0')
	{
	    /* end of input */
	    hour = 0;
	    ret_val = OK;
	    break;
	}
	else
	{
	    tmp_buf[0] = *string++;
	    tmp_buf[1] = *string++;
	    tmp_buf[2] = '\0';
	    if (*string == ':')
	    	string++;
	    if (CVan(tmp_buf, &hour))
		break;
	}

	/* mm: */ 
	if (*string == '\0')
	{
	    /* end of input */
	    minute = 0;
	    ret_val = OK;
	    break;
	}
	else
	{
	    tmp_buf[0] = *string++;
	    tmp_buf[1] = *string++;
	    tmp_buf[2] = '\0';
	    if (*string == ':')
		string++;
	    if (CVan(tmp_buf, &minute))
		break;
	}

	/* ss. */ 
	if (*string == '\0')
	{
	    /* end of input */
	    second = 0;
	    ret_val = OK;
	    break;
	}
	else
	{
	    tmp_buf[0] = *string++;
	    tmp_buf[1] = *string++;
	    tmp_buf[2] = '\0';
	    if (*string == '.')
		string++;
	    if (CVan(tmp_buf, &second))
		break;
	}

	/* cc */
	if (*string == '\0')
	{
	    /* end of input */
	    usecs = 0;
	    ret_val = OK;
	    break;
	}
	else
	{
	    tmp_buf[0] = *string++;
	    tmp_buf[1] = *string++;
	    tmp_buf[2] = '\0';
	    if (*string++ != '\0')
		break;
	    if (CVan(tmp_buf, &usecs))
		break;
	    else
		usecs = (usecs * 10) * MICRO_PER_MILLI;
	}

	ret_val = OK;
	break;
    }

    /* now do input validation */

    if (TM_date_to_secs(year, month, day, hour, minute, 
			second, usecs, &secs_since_1970))
    {
	ret_val = FAIL;
    }
    else
    {
	/* create the TM_TIMESTAMP */
	*ret_seconds	= secs_since_1970;
	*ret_usecs	= usecs;
    }

    return(ret_val);
}


/*{
** Name: TM_lookup_month - Find a month number give it's name.
**
** Description:
**    Given an imput month of the form ("jan", "feb", ...) find the
**    the month number.  On success the month_num field will be set
**    to the month number (1-12), and status will be OK.  If no match
**    is found FAIL is returned.
**
** Inputs:
**	month_str			    Month string ("jan", "feb", ...)
**
** Output:
**	month_num			    On success set to the month number.
**
**      Returns:
**	    OK				    A match was found.
**	    FAIL			    No match was found.
**
** History:
**      07-jul-87 (mmm)
**          Created (copied and modified from iutil).
*/
STATUS
TM_lookup_month(month_str, month_num)
char	*month_str;
i4 	*month_num;
{
    TM_MONTHTAB *month_index;
    i4 		which_month;
    STATUS	ret_val;

    ret_val = FAIL;

    CVlower(month_str);

    /* find the month in the table */

    for (month_index = TM_Monthtab; month_index->tmm_code; month_index++)
    {
	if ((month_str[0] == month_index->tmm_code[0])	&&
	    (month_str[1] == month_index->tmm_code[1])	&&
	    (month_str[2] == month_index->tmm_code[2])	&&
	    (month_str[3] == '\0'))
	{
	    which_month = month_index->tmm_month;
	    ret_val = OK;
	    break;
	}
    }

    *month_num = which_month;
    return(ret_val);
}


/*{
** Name: TM_monthsize - return number of days in a given month.
**
** Description:
**    Returns number of days in a given month.  Month is input as a
**    number between 1 and 12.  This function takes into account leapyears.
**    Probably only works for recent history (ie. for whatever TMyrsize()
**    supports.
**
** Inputs:
**	month				    number of the month.
**	year				    year of month - of form 19XX.
**
** Output:
**	none (see return)
**
**      Returns:
**	    number of days in the month in question.
**
** History:
**      07-jul-87 (mmm)
**          Created (copied and modified from iutil).
*/
static i4 
TM_monthsize(month, year)
i4 	month;
i4 	year;
{
    i4 		days_in_month;
    i4 		days_in_year;


    days_in_month = TM_Dmsize[month - 1];

    /* can only fail if year is bad and we have already checked */
    (void) TMyrsize(year, &days_in_year);

    if (month == 2 && days_in_year == LEAPYEAR)
    {
	/* This is February of a leap year */
	days_in_month++;
    }

    return (days_in_month);
}


/*{
** Name: TM_date_to_secs - Convert year,month,day,minutes,secs to secs.
**
** Description:
**    Convert a date with years, months, days, minutes, secs to secs.
**    All parameter checking is done in this routine.  See below for Legal 
**    values for the parameters.
**
**    On success return OK and set output_secs to the sum of seconds since
**    jan 1, 1970.  Return FAIL if any validation fails.
**
** Inputs:
**      year
**	year  		 year (1970 <= year <= 2035)
**	month 		 month (1 <= month <= 12)
**	day   		 day (1 <= day <= 31)
**	hour  		 hour (0 <= hour <= 23)
**	min   		 minutes (0 <= min <= 59)
**	sec   		 seconds (0 <= sec <= 59)
**
** Output:
**	output_sec	 number of seconds since jan 1, 1970.
**
**      Returns:
**	    OK				    All parameters valid.
**	    FAIL			    An invalid parameter passed in.
**
** History:
**      07-jul-87 (mmm)
**          Created.
**      25-sep-1992 (stevet)
**          Replaced the use of TMzone with TMtz routines for enhanced timezone
**          supports.
*/
static STATUS
TM_date_to_secs(year, month, day, hour, mins, sec, usecs, output_sec)
i4 	year;
i4 	month;
i4 	day;
i4 	hour;
i4 	mins;
i4 	sec;
i4 	usecs;
i4      *output_sec;
{
    i4 		i;
    i4     	time;
    i4 		dy_per_yr;
    i4		time_check;
    PTR         tm_cb;
    STATUS	ret_val = OK, status;

    if ((year < 1970) 				|| 
	(year > 2035)				||
	(month <= 0)				||
	(month > 12)				||
	(day <= 0)				||
	(day > TM_monthsize(month, year))	||
	(hour < 0)				||
	(hour >= 24)				||
	(mins < 0)				||
	(mins >= 60)				||
	(sec < 0)				||
	(sec >= 60)				||
	(usecs < 0))
    {
	/* bad input */
	ret_val = FAIL;
    }
    else
    {
	/* convert date */

	/* "date" will be # of days from 1970 for a while */
	time = 0;

	/* do year conversion */

	for (i = 1970; i < year; i++)
	{
	    if (TMyrsize(i, &dy_per_yr))
	    {
		ret_val = FAIL;
		break;
	    }

	    time += dy_per_yr;
	}

	if (!ret_val)
	{
	    /* do month conversion */
	    for (i = 1; i < month; i++)
		time += TM_monthsize(i, year);

	    /* do day conversion */
	    time += day - 1;

	    /* we now convert date to be the # of hours since 1970 */
	    time *= 24;

	    /* add in hours */
	    time += hour;
	    time *= 60;

	    /* add in minutes */
	    time += mins;
	    time *= 60;

	    /* add in seconds */
	    time += sec;

	    /* 
	    ** time value should never overflow becuase of the 
	    ** date has to be between 1970 and 2035 
	    */
	    time_check = (i4) time;

	    status = TMtz_init(&tm_cb);
	    if( status == OK)
		time -= TMtz_search(tm_cb, TM_TIMETYPE_LOCAL, time_check);

	    *output_sec = time;
	}
    }

    return(ret_val);
}

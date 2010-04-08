/*
**    Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <efndef.h>
#include    <iledef.h>
#include    <iosbdef.h>
#include    <jpidef.h>
#include    <tm.h>
#include    <st.h>
#include    <me.h>
#include    <mu.h>
#include    <nm.h>
#include    <cv.h>
#include    <tmtz.h>
#include    <ssdef.h>
#include    <starlet.h>
#include    <lib$routines.h>

/**
**
**  Name: TM.C - Time library
**
**  Description:
**	Time Compatibility Library for VMS.
**
**          [@func_to_be_defined_name@] - [@comment_text@]
**          [@func_to_be_defined_name@] - [@comment_text@]
**
**
**  History:    $Log-for RCS$
** 	2-26-83 (dkh) Created 
** 	8-31-83 (dd)  Modified for VMS
**	07/06/84 (lichtman) - added TMsubtr - needed for timing statistics
**					in callfn() in ctlmod.
**	17-jan-85 (fhc) -- sys$waitfr sys$getjpi for vms 4.0 support
**	8/2/85 (peb)	Updated for multinational formatting
**	12/11/85 (ericj) Updated for 4.0 release.  Added TMinit to initialize
**			variable timezone at run-time.
**	9/12/85	(ac)	-- bug 6255. TMcvtsecs should use timezone
**				   for the day of the week.
**	2/11/85 (ericj) Make TMinit() an entry point into the CL so that when
**			the environment variable, II_TIMEZONE, is not defined, the 
**			error can be handled nicely.
**	02/21/86 (mmm)  -- timer struct changed in mercury to support
**				   user cpu time.
**      15-sep-86 (seputis)
**          modified to coding standards
**	25-sep-92 (stevet)
**	    changed TMstr and TMcvtsec to use TMtz routines for
**	    timezone adjustments.
**	30-oct-92 (markg)
**	    Fixed string size bug in TMbreak.
**	30-oct-92 (markg)
**	    Fixed string size bug in TMbreak.
**	14-dec-92 (donj)
**	    Fixed a problem with TMcvtsecs(). See History under that function.
**	19-jul-93 (walt)
**	    In TMend, nolonger use event flag zero.  Call lib$get_ef for an
**	    event flag instead.
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    added missing includes
**	16-mar-98 (kinte01)
**	    rename variable timezone to tm_timezone to avoid conflict with
**	    the DEC C symbol timezone. By default all DEC C symbols are 
**	    prefixed and so the local variable timezone was being turned
**	    into DECC$GL_TIMEZONE causing duplicate symbol warnings when
**	    linking. Once the problem was narrowed down, the easiest solution
**	    was to just rename this variable in this module.
**	04-sep-98 (kinte01)
**	    The output of TMstr & TMcvtstr was reversed according to the
**	    CL spec. This was causing the inserts, updates, deletes on a
**	    table activated for Replication to fail with E_QE0018 Illegal 
**	    parameter in control block.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	04-nov-2002 (abbjo03)
**	    Undo the change of 4-sep-98 and adjust TMbreak not to depend on
**	    TMcvtsecs.
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	28-apr-2005 (abbjo03)
**	    Remove TMsecs() in order to use Unix implementation.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      20-mar-2009 (stegr01)
**          Add TMet function and OS_THREADS support for Itanium
**      05-feb-2010 (joea)
**          In TMcvtsecs, return FAIL if TMbreak doesn't return OK.
**/



FUNC_EXTERN void TMgettm();
FUNC_EXTERN i4 TMtostr(i4 curr_time, char *buf);

/*
**  Defines of other constants.
*/

/*
**      Time library constants
*/

/* a valid value for timezone must be a multiple of 60 */
#define                 INVALID_TIME    (-1)

/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF i4                  tm_timezone = INVALID_TIME;


/*
**  Definition of static variables and forward static functions.
*/

static  struct 
	{
	    i4        tim_cpu;            /* [@comment_text@] */
	    i4        tim_dio;            /* [@comment_text@] */
	    i4        tim_bio;            /* [@comment_text@] */
	    i4        tim_pf;             /* [@comment_text@] */
	    i4        tim_usr_cpu;        /* [@comment_text@] */
	    i4        tim_ws;             /* [@comment_text@] */
	    i4        tim_phys;           /* [@comment_text@] */
	}   timer_get;
static	ILE3	itmlst[7] = {
			{ 4, JPI$_CPUTIM, (PTR)&timer_get.tim_cpu, 0 },
			{ 4, JPI$_DIRIO, (PTR)&timer_get.tim_dio, 0 },
			{ 4, JPI$_BUFIO, (PTR)&timer_get.tim_bio, 0 },
			{ 4, JPI$_PAGEFLTS, (PTR)&timer_get.tim_pf, 0 },
			/*! BEGIN BUG - working set and physical pages originally not implemented */
			{ 4, JPI$_PPGCNT, (PTR)&timer_get.tim_ws, 0},
			{ 4, JPI$_FREP0VA, (PTR)&timer_get.tim_phys, 0},
			/*! END BUG */
			{ 0, 0, 0, 0}
		};
static 	char	*days[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

# ifdef OS_THREADS_USED
static MU_SEMAPHORE     TMet_sem;
static bool TMet_sem_init;
# endif /* OS_THREADS_USED */


/*{
** Name: TMyrsize	- days in year
**
** Description:
**	Routine returns the number of days in a normal year (365)
**	if "year" is not a leap year.  366 is returned for a leap
**      year.  This routine assumes the English calendar;  before
**      1752, the Julian scheme was followed, meaning that every
**      4th year was a leap year.  After 1752, century years that
**      are not divisible by 400 are NOT leap years.
**      This scheme is extended backwards to year 1, however dubious
**      that might be.
**
**      Note that we DO NOT return the actual number of days in 1752.
**      It was a leap year, and we return 366.
**
**	Will signal a BAD_YEAR exception if year is less than zero (0).
**
** Inputs:
**      year                            year (larger than 0)
**
** Outputs:
**      days                            number of days in year
**	Returns:
**	    OK or FAIL if year < 0
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**      29-feb-2008 (bolke01) Bug 114958
**          Add the same chaneg that Karl Scchendel made in tmyrsize.c
**          in the UNIX CL to address VMS problem.
*/
STATUS
TMyrsize(year, daysinyear)
i4                year;
i4                *daysinyear;
{
    if (year < 0)
    {
	*daysinyear = 0;
	return(FAIL);
    }
    if ((year & 3) == 0
       && (year <= 1752 || ((year % 100) != 0 || (year % 400) == 0)) )
    {
	*daysinyear = LEAPYEAR;
	return(OK);
    }
    else
    {
	*daysinyear = NORMYEAR;
	return(OK);
    }
}

/*{
** Name: TMnow	- Current time
**
** Description:
**	This routine simply returns the number of seconds since
**	00:00:00 GMT, Jan. 1, 1970.  
**
** Inputs:
**
** Outputs:
**      stime                           current UCT in seconds since 1/1/70
**	Returns:
**	    OK
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
void
TMnow(stime)
SYSTIME            *stime;
{
    TMSTRUCT	tp;

    TMgettm(&tp);
    stime->TM_secs	= tp.time;
    stime->TM_msecs	= tp.millitm;
}

/*{
** Name: TMstr	- Convert SYSTIME to native string
**
** Description:
**	This routine sets "timestr" to point to a character string
**	giving the 1) Day of the week; 2) Month;
**	3) Date; 4) Hour, minutes, seconds and 5) year of "timeval"
**	in a system dependent character format.
**(
**	Note:
**	    Every time this routine is called the previous data
**	    is lost since the buffer is overwritten.
**
**	    I blindly assume that timeval is non-negative and
**	    that timestr is a good pointer.
**)
** Inputs:
**      timeval                         time value to convert to string
**
** Outputs:
**      timestr                         character representation of timeval
**	Returns:
**	    STATUS of operation
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	25-sep-1992 (stevet)
**	    Use TMtz routines to calculate timezone adjustments.
**	04-nov-2002 (abbjo03)
**	    Revert to native string output as per the CL spec.
*/
void
TMstr(
SYSTIME            *timeval,
char               *timestr)
{
    i4	    current_time;
    PTR	    tz_cb;
    STATUS  status;

    current_time = timeval->TM_secs;
    if (TMtz_init(&tz_cb) == OK)
        current_time += TMtz_search(tz_cb, TM_TIMETYPE_GMT, current_time);

    TMtostr(current_time, timestr);
}

/*{
** Name: TMcmp	- Compare times
**
** Description:
**	Compare time1 with time2.  Returns positive if time1 greater than time2;
**	zero if time1 equals time2; and negative if time1 less than time2.
**
** Inputs:
**      time1                           first time
**      time2                           second time to compare
**
** Outputs:
**	Returns:
**	    negative if time1 < time2
**	    zero if time1 == time2
**	    positive if time1 > time2
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
TMcmp(time1, time2)
SYSTIME            *time1;
SYSTIME            *time2;
{
    if (time1->TM_secs == time2->TM_secs)
    {
	return(time1->TM_msecs - time2->TM_msecs);
    }

    return(time1->TM_secs - time2->TM_secs);
}

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
**	    void
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
void
TMstart(tm)
TIMER              *tm;
{
    TMSTRUCT	tp;
    IOSB	iosb;
    
    sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
    TMgettm(&tp);

    tm->timer_start.stat_cpu	= timer_get.tim_cpu * 10;
    tm->timer_start.stat_dio	= timer_get.tim_dio;
    tm->timer_start.stat_bio	= timer_get.tim_bio;
    tm->timer_start.stat_pgflts	= timer_get.tim_pf;
    /*! BEGIN BUG - working set and physical pages originally not implemented */
    tm->timer_start.stat_ws		= timer_get.tim_ws;
    tm->timer_start.stat_phys	= timer_get.tim_phys;
    /*! END BUG */
    tm->timer_start.stat_wc		= tp.time;
}

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
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	15-jul-93 (walt)
**	    Use sys$getjpiw rather than sys$getjpi/sys$waitfor.  We're hanging
**	    here on Alpha sometimes, and it might be a synchronization problem
**	    of some kind since event flag zero is being waited on.
**	19-jul-93 (walt)
**	    That didn't solve the problem.  Nolonger use event flag zero; call
**	    lib$get_ef for an event flag instead.
*/
void
TMend(tm)
TIMER              *tm;
{
    TMSTRUCT	tp;
    TIMERSTAT	*start, *end, *value;
    i4		status;
    IOSB	iosb;
    
    sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
    TMgettm(&tp);

    tm->timer_end.stat_cpu		= timer_get.tim_cpu * 10;
    tm->timer_end.stat_dio		= timer_get.tim_dio;
    tm->timer_end.stat_bio		= timer_get.tim_bio;
    tm->timer_end.stat_pgflts	= timer_get.tim_pf;
    /*! BEGIN BUG - working set and physical pages originally not implemented */
    tm->timer_end.stat_ws		= timer_get.tim_ws;
    tm->timer_end.stat_phys		= timer_get.tim_phys;
    /*! END BUG */
    tm->timer_end.stat_wc		= tp.time;
    start				= &tm->timer_start;
    end					= &tm->timer_end;
    value				= &tm->timer_value;
    value->stat_cpu			= end->stat_cpu - start->stat_cpu;
    value->stat_dio			= end->stat_dio - start->stat_dio;
    value->stat_bio			= end->stat_bio - start->stat_bio;
    value->stat_pgflts			= end->stat_pgflts - start->stat_pgflts;
    value->stat_wc			= end->stat_wc - start->stat_wc;
    /*! BEGIN BUG - working set and physical pages originally not implemented */
    value->stat_ws			= end->stat_ws;
    value->stat_phys			= end->stat_phys;
    /*! END BUG */
}

/*{
** Name: TMsubstr	- difference of the TIMER structs
**
** Description:
**	TMsubtr - take the difference of two TIMER structs.  Intended for use with
**		nested (TMstart, TMend) pairs.  It should be used like this:
**(
**			TIMER	outer;
**			TIMER	inner;
**			TIMER	result;
**
**			TMstart(&outer);
**				.
**				.
**			TMstart(&inner);
**				.
**				.
**			TMend(&inner);
**				.
**				.
**			TMend(&outer);
**			TMsubtr(&outer, &inner, &result);
**)
**		This will fill result with the statistics from outer minus the statistics
**		from inner.  The starting statistics in result will be those from outer.
**		The ending statistics will be set to the ending statistics for outer minus
**		the statistics accumulated between the TMstart(&inner) and the TMend(&inner).
**		The difference statistics will be set to the difference statistics for outer
**		minus the statistics accumulated for inner.
**
** Inputs:
**      outer                           outer statistics
**      inner                           inner statistics
**
** Outputs:
**      result                          TIMER struct for difference
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07/06/84 (lichtman) - written
**      16-sep-86 (seputis)
**          initial creation
*/
void
TMsubstr(outer, inner, result)
TIMER		   *outer;
TIMER		   *inner;
TIMER		   *result;
{
    register TIMERSTAT	*restart	= &result->timer_start,
			*resend		= &result->timer_end,
			*resval		= &result->timer_value,
			*outend		= &outer->timer_end,
			*inval		= &inner->timer_value;

    MEcopy((PTR)&outer->timer_start, sizeof(*restart), (PTR)restart); /* starting stats 
						** for result = starting stats
                                                ** for outer */
    resend->stat_cpu	= outend->stat_cpu - inval->stat_cpu;
    resend->stat_dio	= outend->stat_dio - inval->stat_dio;
    resend->stat_bio	= outend->stat_bio - inval->stat_bio;
    resend->stat_wc	= outend->stat_wc - inval->stat_wc;
    resend->stat_pgflts	= outend->stat_pgflts - inval->stat_pgflts;
    resend->stat_ws	= outend->stat_ws;	/* doesn't make any sense to 
                                                ** subtract the working set or
						** physical memory size.
						*/
    resend->stat_phys	= outend->stat_phys;
    resval->stat_cpu	= resend->stat_cpu - restart->stat_cpu;
    resval->stat_dio	= resend->stat_dio - restart->stat_dio;
    resval->stat_bio	= resend->stat_bio - restart->stat_bio;
    resval->stat_wc	= resend->stat_wc - restart->stat_wc;
    resval->stat_pgflts	= resend->stat_pgflts - restart->stat_pgflts;
    resval->stat_ws	= resend->stat_ws;
    resval->stat_phys	= resend->stat_phys;
}

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
**	    void
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
void
TMformat(
TIMER              *tm,
i4                mode,
char               *ident,
char               decchar,
char               *buf)
{
    TIMERSTAT	*stat;
    i4		time;
    i4		hrs, mins, sec, ms;


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

    time	= stat->stat_cpu;
    ms	= (time % 1000);
    sec	= (time /= 1000) % 60;
    mins	= (time /= 60) % 60;
    hrs	= (time / 60);

    STprintf(buf,
"TIMER-%-12s cpu = %1d:%02d:%02d.%03d; dio = %-6d; bio = %-6d; pf = %-6d et = %-10.2f; ws = %-6d; phys = %-6d\n",
	    ident, hrs, mins,  sec, ms, 
	    stat->stat_dio,    stat->stat_bio,
	    stat->stat_pgflts, stat->stat_wc / 60., decchar,
	    stat->stat_ws,     stat->stat_phys);
}

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
    IOSB iosb;

    sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
    return(timer_get.tim_cpu * 10);
}

/*{
** Name: TMbreak	- break char string for date into pieces
**
** Description:
**      Break the string created by TMstr() into component pieces.  This 
**      routine fills in the following fields of the struct (actually 
**      the pointer to the struct) passed to it: the day-of-the-week,
**      month, day, year, hours, minutes, and seconds.
**
** Inputs:
**      tm                              time to break up
**
** Outputs:
**      human                           components of time
**	Returns:
**	    FAIL - if tm is invalid
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**
**	30-oct-92 (markg)
**	    Fixed string length problem, cp was declared as char  cp[24], this
**	    was too short and was causing corruption of 'ptr'.
**	04-nov-2002 (abbjo03)
**	    Remove dependency on TMcvtsecs and instead use output from TMtostr.
*/
STATUS
TMbreak(
SYSTIME            *tm,
struct TMhuman     *human)
{
    char        tbuf[TM_SIZE_STAMP + 1];
    char        *ptr = tbuf;
    i4		day;
    i4		current_time;
    PTR		tz_cb;

    if (tm->TM_secs > MAXI4)
	current_time = MAXI4;
    else if (tm->TM_secs < MINI4)
	current_time = MINI4;
    else
	current_time = tm->TM_secs;

    /* Timezone adjustment */
    if (TMtz_init(&tz_cb) == OK)
        current_time += TMtz_search(tz_cb, TM_TIMETYPE_GMT, current_time);

    if (TMtostr(current_time, tbuf) == 0)
	return FAIL;

    /* bug 6255, should use current_time instead of secs
    ** for the day of the week
    */
    day = ((current_time/86400) + 4) % 7;
    STcopy(days[day], human->wday);
    STlcopy(ptr, human->day, 2);
    ptr += 3;
    STlcopy(ptr, human->month, 3);
    ptr += 4;
    STlcopy(ptr, human->year, 4);
    ptr += 5;
    STlcopy(ptr, human->hour, 2);
    ptr += 3;
    STlcopy(ptr, human->mins, 2);
    ptr += 3;
    STlcopy(ptr, human->sec, 2);
    return (OK);
}

/*{
** Name: TMcvtsecs	- Convert seconds to a string
**
** Description:
**      This routine performs the same functions as TMstr but takes a 
**      number of seconds since Jan 1,1970 as the first argument rather
**      than a pointer to a SYSTIME.
**
**	convert seconds to a character string.  the number of seconds
**	passed in is assumed to be the number of seconds since
**	Jan. 1, 1970.  you must provide the buffer to put the string
**	in and it is assumed the buffer is big enough.  format of
**	the string may vary between systems.
**
** Inputs:
**      secs                            number of seconds
**
** Outputs:
**      buf                             buffer to store the string in
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10/3/83 (dd) VMS CL
**      15-sep-86 (seputis)
**          initial creation
**	25-sep-1992 (stevet)
**	    Use TMtz routines to calculate timezone adjustments.
**	14-dec-92 (donj)
**	    The first copy of the week day fm the days[] array is done with
**	    STcopy(), TMcvtsecs() assumes that STcopy() returns the length of
**	    the copied string like STlcopy().  This was the case with the
**	    MACRO version, although it was undocumented. But, with the new C
**	    coded version it doesn't return a value. This corrupts the string
**	    that TMcvtsecs() is trying to build.
**	5-aug-93 (ed)
**	    STcopy does not return length
**	04-nov-2002 (abbjo03)
**	    Rewrite as per UNIX/NT implementation.
*/
STATUS
TMcvtsecs(
i4            secs,
char          *buf)
{
    struct TMhuman	comptime;
    SYSTIME		tm;

    tm.TM_secs = secs;
    if (TMbreak(&tm, &comptime) != OK)
        return FAIL;
    STpolycat(11, comptime.day, "-", comptime.month, "-", comptime.year, " ",
	comptime.hour, ":", comptime.mins, ":", comptime.sec, buf);
    return(OK);
}

/*{
** Name: TMzone	- Return minutes to Greenwich
**
** Description:
**	Local time zone (in minutes of time westward from Greenwich).
**
** Inputs:
**
** Outputs:
**      zone                            local time zone in minutes
**	    				westward from Greenwich.
**	Returns:
**	    
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      15-sep-86 (seputis)
**          initial creation
*/
void
TMzone(zone)
i4                *zone;
{
    if (tm_timezone == INVALID_TIME)
	    TMinit();
    *zone = -tm_timezone / 60;	
}

/*{
** Name: TMinit	- initialize time zone
**
** Description:
**	    Initialize the TM variable, timezone, for operating systems
**	that don't implement the concept.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    OK		if the varible is properly initialized.
**	    TM_NOTIMEZONE	if the environment variable, II_TIMEZONE, is
**			not defined.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-Feb-86 (ericj) -- made a CL entry point and returns status.
**      15-sep-86 (seputis)
**          initial creation
*/
STATUS
TMinit()
{
    PTR	gmt_str;
    i4	gmt_hrs;

    NMgtAt("II_TIMEZONE", &gmt_str);
    if (!gmt_str || !(*gmt_str))
    {
	    tm_timezone = 0;
	    return(TM_NOTIMEZONE);	/* flag some kind of error condition */
    }
    CVal(gmt_str, &gmt_hrs);
    tm_timezone = -1 * gmt_hrs * 60 * 60;
    return(OK);
}

/*{
** Name: TMet   - Current time (really)
**
** Description:
**      This routine simply returns the number of seconds since
**      00:00:00 GMT, Jan. 1, 1970.
**
**      Return seconds and milliseconds since 00:00:00 GMT, 1/1/1970.
**      On sysV machines, milliseconds field is always 0.
**
**      This function is not an approved interface to the CL.  It is only to
**      be used internally by the UNIX CL for debugging purposes (currently
**      used for benchmark debugging - testing exactly elapsed time during
**      sub-second pauses in the dbms, rcp, ...).  It is used instead of
**      TMnow() because of daveb's change to TMnow which salts the time with
**      the pid to workaround a misuse of TMnow() by the dbms.
**
** Inputs:
**
** Outputs:
**      stime                           current UCT in seconds since 1/1/70
**
**      Returns:
**          VOID
**
** Description:
**      This routine simply returns the number of seconds since
**      00:00:00 GMT, Jan. 1, 1970.
**
**      Return seconds and milliseconds since 00:00:00 GMT, 1/1/1970.
**      On sysV machines, milliseconds field is always 0.
**
**      This function is not an approved interface to the CL.  It is only to
**      be used internally by the UNIX CL for debugging purposes (currently
**      used for benchmark debugging - testing exactly elapsed time during
**      sub-second pauses in the dbms, rcp, ...).  It is used instead of
**      TMnow() because of daveb's change to TMnow which salts the time with
**      the pid to workaround a misuse of TMnow() by the dbms.
**
** Inputs:
**
** Outputs:
**      stime                           current UCT in seconds since 1/1/70
**
**      Returns:
**          VOID
** History:
**      02-feb-90 (mikem)
**          created.
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in TMet() to describe 2
**          versions of gettimeofday() available.
**      16-Nov-1998 (jenjo02)
**          TMet() now called by CSMTsuspend for timed waits. Faults
**          in code was causing 2 gettimeofday()'s to be executed,
**          TM_msecs wasn't being set correctly.
**      04-Apr-2001 (bonro01)
**          Correct millisecond time increment for code that does not
**          use gettimeofday() function.  There was some confusion
**          about whether this routine returned microseconds or milliseconds.
*/

VOID
TMet(stime)
SYSTIME *stime;
{

#   ifdef xCL_005_GETTIMEOFDAY_EXISTS
    {
        struct timeval  t;
        struct timezone tz;
# if defined(dg8_us5) || defined(dgi_us5)
        int gettimeofday ( struct timeval *, struct timezone * ) ;
# else
# if defined(xCL_GETTIMEOFDAY_TIMEONLY_V)
    int gettimeofday ( struct timeval *, ...) ;
# else
        int             gettimeofday();
# endif /* xCL_GETTIMEOFDAY_TIMEONLY_V */
# endif

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
        gettimeofday(&t);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
        gettimeofday(&t, &tz);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
        gettimeofday(&t, &tz);
# endif
        stime->TM_secs = t.tv_sec;
        stime->TM_msecs = t.tv_usec / 1000;
    }
#   else
    {

        static i4      seconds = 0;
        static i4      msecs   = 0;

        stime->TM_secs = time(0);


        /* DMF code expects timestamps to be of finer granularity than
        ** a second.  In order to fake this on UNIX's with this granularity
        ** we will make the timestamps monatomically increasing within
        ** a process.
        */
#ifdef OS_THREADS_USED
        if ( !TMet_sem_init )
            MUw_semaphore( &TMet_sem, "TM et sem" );
        MUp_semaphore( &TMet_sem );
#endif /* OS_THREADS_USED */
        if (stime->TM_secs == seconds)
        {
            if(++msecs >= 1000)
            {
                stime->TM_secs = ++seconds;
                msecs=0;
            }

        }
        else
        {
            seconds = stime->TM_secs;
            msecs = 0;
        }
        stime->TM_msecs = msecs;
#ifdef OS_THREADS_USED
        MUv_semaphore( &TMet_sem );
#endif /* OS_THREADS_USED */
    }
#   endif /* xCL_005_GETTIMEOFDAY_EXISTS */

    return;
}

/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <cs.h>
#include    <mu.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <cv.h>
#include    "tmlocal.h"

/**
**
**  Name: TMTIMESTAMP.C - Time routines.
**
**  Description:
**      This module contains the TM cl routines.
**
**	    TMcmp_stamp - Compare timestamps.
**          TMget_stamp - Get timestamp.
**	    TMstamp_str - Convert stamp to string.
**	    TMstr_stamp - Convert string to stamp.
**	    TMsecs_to_stamp - Convert seconds since 1-Jan-1970 to TM_STAMP
**
**
** History:
** Revision 1.3  87/07/24  15:59:06  mikem
**      Update files to meet jupiter coding standards.
 * Revision 1.1  88/08/05  13:46:59  roger
 *	UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * Revision 1.2  88/08/10  14:25:35  jeff
 * 	added daveb's fixes for non-bsd
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	1-sep-89 (daveb)
**		Enhance the uniqueness of the stamp by salting in the low
**		8 bits of the pid.
**      21-jun-93 (mikem)
**	    Only call getpid() once per process.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-apr-94 (mikem)
**	    Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**	    xCL_GETTIMEOFDAY_TIME_AND_TZ) in TMget_stamp() to describe 2 
**	    versions of gettimeofday() available.  
**	31-Jan-1995 (canor01)
**	    Implement medium-term fix to continue to allow TMtime_stamp
**	    to be used for both time and for unique stamp.  Save 
**	    previous timestamp for comparison, plus salt in portion of
**	    pid to try to get uniqueness across processes. (B63625)
**      30-Nov-94 (nive)
**              For dg8_us5, chnaged the prototype definition for
**              gettimeofday to gettimeofday ( struct timeval *, struct
**              timezone * )
**      23-May-95 (walro03)
**              Reversed previous change (30-Nov-94).  A better way is to use
**              the correct prototype based on xCL_GETTIMEOFDAY_TIMEONLY vs.
**              xCL_GETTIMEOFDAY_TIME_AND_TZ.
**      06-jun-1995 (canor01)
**              added semaphore protection for statics
**      15-jun-95 (popri01)
**          Add support for additional, variable parm list, GETTIME prototype
**	03-jun-1996 (canor01)
**	    Clean up usage of semaphores to protect static variables used
**	    with operating-system threads.
**  20-feb-97 (muhpa01)
**      Removed local declaration for gettimeofday() for hp8_us5 to fix
**      "Inconsistent parameter list declaration" error.
**	27-May-1997 (jenjo02)
**	    Added missing line of code to set TMstamp_sem_init to TRUE once
**	    semaphore has been initialized.
**      28-may-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
**      14-Feb-2000 (linke01)
**          Remove local declaration for gettimeofdady() for usl_us5. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-nov-2003 (abbjo03)
**	    Add TMsecs_to_stamp.
**/

/* defines */

#define DAYS_BTW_BASEDATES		40587UL
#define SECS_BTW_BASEDATES		(DAYS_BTW_BASEDATES * 86400UL)
#define HUNDRED_NANOSEC_ADJ		10000000L

/* External variables */

# ifdef OS_THREADS_USED
static MU_SEMAPHORE TMstamp_sem;
static bool TMstamp_sem_init = FALSE;
# endif /* OS_THREADS_USED */


/*{
** Name: TMcmp_stamp	- Compare two timestamps.
**
** Description:
**      Compare two time stamps.  Returns the logical subtraction
**	of the time stamps.  
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      time1                           First timestamp.
**	time2				Second timestamp.
**
** Outputs:
**	Returns:
**	    -1			    time1 < time2
**	    0			    time1 == time2
**	    +1			    time1 > time2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
**      06-jul-1987 (mmm)
**          Initial Jupiter unix cl.
*/
i4
TMcmp_stamp(
TM_STAMP	*time1,
TM_STAMP	*time2)
{
    if (time1->tms_sec != time2->tms_sec)
    {
	if (time1->tms_sec < time2->tms_sec)
	    return(-1);
	else
	    return(1);
    }
    else
    {
	if (time1->tms_usec < time2->tms_usec)
	    return(-1);
	else if (time1->tms_usec > time2->tms_usec)
	    return(1);
	else
	    return(0);
    }
}

/*{
** Name: TMget_stamp	- Return timestamp.
**
** Description:
**      Return a TM timestamp.
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**
** Outputs:
**      time                            Pointer to location to return stamp.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1986 (Derek)
**	    Created.
**      06-jul-1987 (mmm)
**          Initial Jupiter unix cl.
**	28-jul-1988 (daveb)
**	Can't call input parameter "time" because that confilcts with the
**		"time" system call you need on System V.
**	1-sep-89 (daveb)
**		Enhance the uniqueness of the stamp by salting in the low
**		8 bits of the pid.
**      21-jun-93 (mikem)
**	    Only call getpid() once per process.
**	20-apr-94 (mikem)
**	    Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**	    xCL_GETTIMEOFDAY_TIME_AND_TZ) in TMget_stamp() to describe 2 
**	    versions of gettimeofday() available.  
**	27-May-1997 (jenjo02)
**	    Added missing line of code to set TMstamp_sem_init to TRUE once
**	    semaphore has been initialized.
**	15-Feb-2008 (hanje04)
**	    SIR S119978
**	    Remove local definitions of system functions.
*/
void
TMget_stamp(
TM_STAMP	*stamp)
{
    static int	pid = 0;
    static TM_STAMP   last_stamp = { 0, 0 };
# ifdef xCL_005_GETTIMEOFDAY_EXISTS
    struct timeval  t;
    struct timezone tz;
# else /* xCL_005_GETTIMEOFDAY_EXISTS */
    static i4	seconds = 0;
    static i4	usecs   = 0;
 
# endif /* xCL_005_GETTIMEOFDAY_EXISTS */
 
# ifdef OS_THREADS_USED
    if ( !TMstamp_sem_init )
    {
	MUw_semaphore( &TMstamp_sem, "TM stamp sem" );
	TMstamp_sem_init = TRUE;
    }
# endif /* OS_THREADS_USED */

# ifdef xCL_005_GETTIMEOFDAY_EXISTS
 
# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
    gettimeofday(&t);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    gettimeofday(&t, &tz);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
    gettimeofday(&t, &tz);
# endif
    stamp->tms_sec = t.tv_sec;
    stamp->tms_usec = t.tv_usec;
# else
 
    stamp->tms_sec = time(0);
 
    /* DMF code expects timestamps to be of finer granularity than
    ** a second.  In order to fake this on UNIX's with this granularity
    ** we will make the timestamps monatomically increasing within
    ** a process.  We increase 1000 at a time because the timestamp that
    ** actually gets printed is only in 100th's of a second.
    */
# ifdef OS_THREADS_USED
    MUp_semaphore( &TMstamp_sem );
# endif /* OS_THREADS_USED */
    if (stamp->tms_sec == seconds)
    {
	usecs += 1000;
    }
    else
    {
	seconds = stamp->tms_sec;
	usecs = 0;
    }
    stamp->tms_usec = usecs;
# ifdef OS_THREADS_USED
    MUv_semaphore( &TMstamp_sem );
# endif /* OS_THREADS_USED */
 
# endif

    /* To enhance the uniqueness of the returned value as a stamp,
    ** salt in the low 8 bits worth of the pid (daveb) 
    */
# ifdef OS_THREADS_USED
    MUp_semaphore( &TMstamp_sem );
# endif /* OS_THREADS_USED */
    if (!pid)
    {
	pid = getpid();

        /* B63625 (canor01)
        ** Since the low-order bits of the pid could potentially
        ** affect the above calculations, as a medium-term fix
        ** (suggested by sweeney) left shift the pid by two bits
        ** before or'ing it in.  Really, though, we need two 
        ** functions--one for unique stamp and one for time.
        */
    	pid <<= 2;
    }


    /* B63625: To further enhance the uniqueness of the returned value
    ** as a stamp in the case of *very* fast machines, make sure 
    ** returns on consecutive calls are different. (canor01) 
    */
    if ( stamp->tms_sec == last_stamp.tms_sec &&
	 stamp->tms_usec <= last_stamp.tms_usec )
        stamp->tms_usec = ++last_stamp.tms_usec;
    last_stamp.tms_sec  = stamp->tms_sec;
    last_stamp.tms_usec = stamp->tms_usec;
# ifdef OS_THREADS_USED
    MUv_semaphore( &TMstamp_sem );
# endif /* OS_THREADS_USED */

    stamp->tms_usec |= (pid & 0xff);

}

/*{
** Name: TMstamp_str	- Convert a stamp to a string.
**
** Description:
**      Convert a stamp to a string of the form:
**	    dd-mmm-yyyy hh:mm:ss.cc
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      time                            Pointer to timestamp.
**
** Outputs:
**      string                          Pointer to output string.
**					Must be TM_SIZE_STAMP bytes long.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
*/
void
TMstamp_str(
TM_STAMP	*time,
char		*string)
{
    struct TMhuman	human;
    SYSTIME		tm;
    i4			csecs;
    char		csecs_str[10];

    /* initialize the SYSTIME */

    tm.TM_secs	= time->tms_sec;
    tm.TM_msecs	= time->tms_usec / MICRO_PER_MILLI;

    if (TMbreak(&tm, &human) == OK)
    {
	/* Now format correctly "dd-mmm-yyyy hh:mm:ss.cc" */

	/* dd- */
	if (human.day[0] == ' ')
	    *string++ = '0';
	else
	    *string++ = human.day[0];
	*string++ = human.day[1];
	*string++ = '-';

	/* mmm- */
	*string++ = human.month[0];
	*string++ = human.month[1];
	*string++ = human.month[2];
	*string++ = '-';

	/* yyyy-  */
	*string++ = human.year[0];
	*string++ = human.year[1];
	*string++ = human.year[2];
	*string++ = human.year[3];
	*string++ = ' ';

	/* hh: */ 
	*string++ = human.hour[0];
	*string++ = human.hour[1];
	*string++ = ':';

	/* mm: */ 
	*string++ = human.mins[0];
	*string++ = human.mins[1];
	*string++ = ':';

	/* ss. */ 
	*string++ = human.sec[0];
	*string++ = human.sec[1];
	*string++ = '.';

	/* cc */

	/* print out 100th of second from internally stored microsecond */
	csecs = (time->tms_usec / 10000);
    	CVna(csecs, csecs_str);

	if (csecs <= 9)
	{
	    *string++ = '0';
	    *string++ = csecs_str[0];
	}
	else
	{
	    *string++ = csecs_str[0];
	    *string++ = csecs_str[1];
	}
	*string++ = '\0';
    }
    else
    {
	/* For some reason one of the conversions failed.  Since this
	** routine is currently spec'd as returning void - the best we
	** can do is return a bogus string.
	*/
	STcopy("dd-mmm-yyyy hh:mm:ss.cc", string);
    }

    return;
}

/*{
** Name: TMstr_stamp	- Convert string to stamp.
**
** Description:
**      Convert a string of the form:
**	    dd-mmm-yyyy:hh:mm:ss.cc 
**	to a timestamp.  Any partial suffix after the yyyy is legal.
**	(I.E  dd-mmm-yyyy means dd-mmm-yyy 00:00:00.00)
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      string                          String to convert.
**
** Outputs:
**      time                            Pointer to timestamp.
**	Returns:
**	    OK
**	    TM_SYNTAX
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
*/
STATUS
TMstr_stamp(
char		*string,
TM_STAMP	*time)
{
    i4	seconds;
    i4	usecs;
    STATUS	ret_val = OK;
    STATUS	TM_parse_timestamp();

    if (TM_parse_timestamp(string, &seconds, &usecs))
    {
	time->tms_sec	= 0;
	time->tms_usec	= 0;
	ret_val = TM_SYNTAX;
    }
    else
    {
	time->tms_sec	= seconds;
	time->tms_usec	= usecs;
    }

    return(ret_val);
}

/*{
** Name: TMsecs_to_stamp - Convert seconds since 1-Jan-1970 to TM_STAMP
**
** Description:
**	Converts the number of seconds since 00:00:00 UTC, Jan. 1, 1970, to
**	a TM_STAMP timestamp. Note, this routine is for the sole use of DMF
**	for auditing and rollforward.
**
** Inputs:
**	secs		seconds to convert
**
** Outputs:
**	stamp		pointer to TM_STAMP structure
**
** Returns:
**	void
**
** Side Effects:
**	none
**
** History:
**	18-nov-2003 (abbjo03)
**	    Created.
*/
void
TMsecs_to_stamp(
i4		secs,
TM_STAMP	*stamp)
{
    stamp->tms_sec = secs;
    stamp->tms_usec = 0;
}

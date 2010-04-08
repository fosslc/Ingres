/*
**  Copyright (c) 2004, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <adudate.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>
#include    "adutime.h"
#include    "adu1dates.h"


/**
** Name:  ADULLDATES.C -    Low level date routines which primarily support ADF
**			    interface.
**
** Description:
**	  This file defines low level date routines which primarily support
**	the ADF interface.  Other routines are used for the normalization
**	of dates.
**
**	This file defines:
**
**	    adu_1normldate()   - Normalizes an absolute date.
**	    adu_2normldint()   - Normalizes a internal interval date.
**          adu_4date_cmp()    - Internal routine used for comparing dates.
**          adu_dlenchk()      - Required function for checking create length
**                                 and precision
**          adu_3day_of_date() - Compute absolute number of days since epoch
**                                 for the given ABSOLUTE date.
**          adu_5time_of_date()- Calculate the number of second since 1/1/70.
**
**  Function prototypes defined in ADUDATE.H except adu_4date_cmp, which is
**  defined in ADUINT.H file.
** 
**  History:
**	21-jun-85 (wong)
**	    Used 'I1_CHECK_MACRO' macro to hide unsigned character problems
**	    with AD_NEWDTNTRNL member 'dn_highday'.
**	11-apr-86 (ericj)
**	    Assimilated into Jupiter from 4.0/02 code.
**	03-feb-87 (thurston)
**	    Upgraded the adu_4date_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for dates.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	16-aug-89 (jrb)
**	    Put adu_prelim_norml() back in and re-wrote for performance.
**	24-jan-90 (jrb)
**	    Fixed bug 9859.  date('60 mins') was returning "2 hours".
**	06-sep-90 (jrb)
**	    Use adf_timezone (session specific) rather than call TMzone.
**      29-aug-92 (stevet)
**          Added adu_5time_of_date to calculate the number of second
**          since 1/1/70 for a given Ingres date.  Modified the timezone
**          adjustment method in adu_4date_cmp().
**      04-jan-1993 (stevet)
**          Added function prototypes and moved adu_prelim_norml to adudates.c
**          since this routine only used in adudates.c file.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	10-feb-1998 (hanch04)
**	    changed PRE_EPOCH_DAYS when calculating days to support
**	    years 0001 - 9999, EPOCH will be calculated as 14-sep-1752
**	    dates between, but not including  02-sep-1752 and 14-sep-1752
**	    do not exist.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**	19-Feb-2008 (kiria01) b119943
**	    Include dn_nseconds with dn_seconds in checks for setting
**	    dn_status);
**	03-Jun-2008 (kiria01) b112144
**	    Correct TZ type flags as passed to TMtz_search to that
**	    TM_TIMETYPE_LOCAL not used against adjusted NEWDTNTRNL form & v.v. 
**      17-Apr-2009 (horda03) Bug 121949
**          AD_DN2_ADTE_TZ must be set for all Ingres Date's that are ABSOLUTE
**          but don't have TIMESPEC. Old Ingres versions may insert Ingres Date's
**          with some flags not set (e.g. back in 2.6 the date('today') function
**          would insert a date represented by '01xxxxxx' (only ABSOLUTE flag set).
**      17-jul-2009 (horda03) b122337
**          For an ABSOLUTE Ingresdate check to see if it need to be
**          normalised. In olden days, a date( '10:10:10') could have
**          a negative Num seconds, or num seconds > MAX SECS PER DAY
**          depending on the timezone of the session entering the
**          value.
**      11-Nov-2009 (horda03) Bug 122871
**          Set DATE Status flags based on the DATE type being converted
**          not on the value of the DATE part. If a timestamp with 0 time
**          is converted to an IngresDate, the Ingres date will be a DATE-ONLY
**          value; it should be a DATE+TIME value.
**/

FUNC_EXTERN DB_STATUS
ad0_1cur_init(
ADF_CB		    *adf_scb,
struct	timevect    *tv,
bool                now,
bool		    highprec,
i4		    *tzoff);

static DB_STATUS
ad0_tz_offset(
ADF_CB              *adf_scb,
AD_NEWDTNTRNL       *base,
TM_TIMETYPE	    src_type,
i4                  *tzsoffset);


/*  Static variable references	*/

static	i4	    dum1;   /* Dummy to give to ult_check_macro() */
static	i4	    dum2;   /* Dummy to give to ult_check_macro() */

static const int daysBefore[] = {
	0,	/* notused, dn_month is 1 origin */
	0,	/* Jan */
	31,	/* Feb */
	59,	/* Mar */
	90,	/* Apr */
	120,	/* May */
	151,	/* Jun */
	181,	/* Jul */
	212,	/* Aug */
	243,	/* Sep */
	273,	/* Oct */
	304,	/* Nov */
	334	/* Dec */
};

static const i4 nano_mult[] = {
    1000000000,  /* 0 */
     100000000,  /* 1 */
      10000000,  /* 2 */
       1000000,  /* 3 */
        100000,  /* 4 */
         10000,  /* 5 */
          1000,  /* 6 */
           100,  /* 7 */
            10,  /* 8 */
             1,  /* 9 */
};


/*
**  Name:   adu_1normldate() - normalize an internal date.
**
**  Description:
**	Normalizes an absolute time as follows:
**	    1 <= month <= 12
**	    1 <= day <= adu_2monthsize(month, year)
**	    0 <= time <= 24 * 3600000
**
**	normldate works for both positive dates and negative
**      dates (for subtraction).
**
**  Inputs:
**      d   - Pointer to date to normalize.
**
**  Outputs:
**	*d  - normalized date.
**
**  Returns:
**      (none)
**
**  History:
**      03/25/85 (lichtman) -- fixed bug #5281: timezone of 0 allows
**              24:00:00 to be stored.  Fixed normalization
**              of time part of date to check for >= AD_5DTE_MSPERDAY
**              instead of > AD_5DTE_MSPERDAY.
**	10-apr-86 (ericj) - snatched from 4.0 code and modified for Jupiter.
**	13-jan-87 (daved) - performance enhancements
**	10-Dec-2004 (schka24)
**	    More (minor) speed tweaks.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_1normldate for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	01-aug-2006 (gupsh01)
**	    Added support for nanosecond precision level.
**	18-Feb-2008 (kiria01) b120004
**	    Use standard approach to normalising numbers.
*/

#   define  xDEBUG 1
VOID
adu_1normldate(
AD_NEWDTNTRNL   *d)
{
    register i4	    days;
    register i4     t;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
    {
        TRdisplay("normldate:abs.date input:year=%d, month=%d, day=%d\n",
            d->dn_year, d->dn_month, days);
        TRdisplay("normldate:and time=%d.%09d, status=%d\n",
            d->dn_seconds, d->dn_nsecond, d->dn_status);
    }
#   endif

    d->dn_status2 &= ~AD_DN2_NORM_PEND;

    if ((u_i4)d->dn_nsecond >= AD_49DTE_INSPERS)
    {
        t = d->dn_nsecond / AD_49DTE_INSPERS;
        d->dn_seconds += t;
	d->dn_nsecond -= t * AD_49DTE_INSPERS;
	if (d->dn_nsecond < 0)
	{
	    d->dn_nsecond += AD_49DTE_INSPERS;
	    d->dn_seconds--; /* Borrow a second */
	}
    }

    days = d->dn_day;
    if ((u_i4)d->dn_seconds >= AD_41DTE_ISECPERDAY)
    {
	t = d->dn_seconds / AD_41DTE_ISECPERDAY;
	days += t;
	d->dn_seconds -= t * AD_41DTE_ISECPERDAY;
	if (d->dn_seconds < 0)
	{
	    d->dn_seconds += AD_41DTE_ISECPERDAY;
	    days--; /* Borrow a day */
	}
    }


    /* normalize months so we can handle days correctly */

    d->dn_month--; /* zero bias temporarily */
    if ((u_i4)d->dn_month >= 12 && d->dn_year > 0)
    {
	t = d->dn_month / 12;
	d->dn_year += t;
	d->dn_month -= t * 12;
	if (d->dn_month < 0)
	{
	    d->dn_month += 12;
	    d->dn_year--;
	}
    }
    d->dn_month++;


    /* now do days remembering to handle months on overflow/underflow.
    ** Don't bother if # of days is ok for any month.
    */

    if (days > 28)
    {
	while (days > (t = adu_2monthsize((i4) d->dn_month, (i4) d->dn_year)))
	{
	    days -= t;
	    d->dn_month++;

	    if (d->dn_month > 12)
	    {
		d->dn_year++;
		d->dn_month -= 12;
	    }
	}
    }

    while ((days < 1) && (d->dn_month > 0))
    {
        d->dn_month--;

        if (d->dn_month < 1)
        {
            d->dn_year--;
            d->dn_month += 12;
        }

        days += adu_2monthsize((i4) d->dn_month, (i4) d->dn_year);
    }

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
    {
        TRdisplay("normldate:abs.date output:year=%d, month=%d, day=%d\n",
            d->dn_year, d->dn_month, days);
        TRdisplay("normldate:and time=%d, status=%d\n",
            d->dn_seconds, d->dn_status);
    }
#   endif

    d->dn_day    = days;
}


/*
**  Name:   adu_2normldint() - Internal routine used to normalize interval dates
**
**  Description:
**      Normalizes an interval date as follows:
**      0 <= time  <= 24 * 3600000
**      0 <= month <  12
**
**  NOTE: days and times are not normalized to months and years
**      since these are inexact transformations.
**
**  Returns number of days in interval, used for comparisons.
**  Computed using 365 days/year and 30.5 days/month
**
**      11/02/84 (lin) - if time interval = 0, reset DAYSPEC status bit instead
**           of the original YEARSPEC.
**
**  12/9/85 (ac) - bug # 6399. Rounding error in normalizing the dn_time in
**             mini second.
**	17-jan-02 (inkdo01)
**	    All these years, and MONTHSPEC hasn't been set for date values.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_2normldint for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	28-jul-2006 (gupsh01)
**	    Added support for nanoseconds.
**	16-oct-2006 (gupsh01)
**	    Fix day calculation.
**	13-feb-2006 (gupsh01)
**	    Another fix to days calculation.
**	4-sep-2007 (dougi)
**	    Fix normalization involving nsecs. 3 unit, multi-radix, mixed sign
**	    normalization ain't very easy.
**	18-Feb-2008 (kiria01) b120004
**	    Use standard approach to normalising numbers and remove some of
**	    the unnecessary float processing. Documented previous change to
**	    isolate some normalisation errors due to combinations not being
**	    handled.
*/

i4
adu_2normldint(
AD_NEWDTNTRNL   *d)
{
    i1	status = 0;
    i4  nsec = d->dn_nsecond;
    i4  secs = d->dn_seconds;
    i4  days = d->dn_day;
    i2  months = 0;
    i2  years = 0;
    f8  month_days = 0.0;
    f8  flt_days = 0.0;
    f8  flt_years = 0.0;
    f8  flt_months = 0.0;
    f8  flt_temp = 0.0;

    /* normalize days and time */
    d->dn_status2 &= ~AD_DN2_NORM_PEND;

    /* Does nsec have (or owe) more than a whole second? */
    if ((u_i4)nsec >= AD_49DTE_INSPERS)
    {
	i4 tmp = nsec / AD_49DTE_INSPERS;
        secs += tmp;
        nsec -= tmp * AD_49DTE_INSPERS;
    }

    /* Does secs have (or owe) more than a whole day? */
    if ((u_i4)secs >= AD_40DTE_SECPERDAY)
    {
	i4 tmp = secs / AD_40DTE_SECPERDAY;
        days += tmp;
        secs -= tmp * AD_40DTE_SECPERDAY;
    }

    /* We now have both nsec and secs in valid ranges BUT might be -ve */

    /* We need to normalize across all 3 units (days, seconds, nanoseconds)
    ** and we aim to get all three components to have the same sign (or 0)
    **
    ** Switch on encoding (ternary number, in which leftmost digit is
    ** for days units, middle is time units and rightmost is nanoseconds,
    ** and 2 means value is > 0, 1 means value is < 0 and 0 means value
    ** is 0). All combinations of signs of the 3 units are represented,
    ** many of which don't require any wor. E.g. all values with 
    ** nanosecond value 0 are ignored, because normalization of days/
    ** seconds has already been done. And all combinations of days and
    ** seconds with different, non-zero signs will have been normalized
    ** to the same sign.
    ** This code may look a little unconventional, but it handles the
    ** problems of multi-radix normalization. If anyone can think of
    ** a better way of doing it, be my guest.
    */
#define N_eq_0 (0 * 1)
#define N_lt_0 (1 * 1)
#define N_gt_0 (2 * 1)
#define S_eq_0 (0 * 3)
#define S_lt_0 (1 * 3)
#define S_gt_0 (2 * 3)
#define D_eq_0 (0 * 9)
#define D_lt_0 (1 * 9)
#define D_gt_0 (2 * 9)
    switch ((nsec > 0 ? N_gt_0 : (nsec < 0 ? N_lt_0 : N_eq_0)) +
	    (secs > 0 ? S_gt_0 : (secs < 0 ? S_lt_0 : S_eq_0)) +
	    (days > 0 ? D_gt_0 : (days < 0 ? D_lt_0 : D_eq_0)))
    {
    /*  0  D_eq_0 S_eq_0 N_eq_0	    =0  */
    /*  1  D_eq_0 S_eq_0 N_lt_0	    <0 */
    /*  2  D_eq_0 S_eq_0 N_gt_0	    >0 */
    /*  3  D_eq_0 S_lt_0 N_eq_0     <0 */
    /*  4  D_eq_0 S_lt_0 N_lt_0	    <0 */
    /*  5  D_eq_0 S_lt_0 N_gt_0  ** <0 */
    /*  6  D_eq_0 S_gt_0 N_eq_0     >0 */
    /*  7  D_eq_0 S_gt_0 N_lt_0  ** >0 */
    /*  8  D_eq_0 S_gt_0 N_gt_0     >0 */
    /*  9  D_lt_0 S_eq_0 N_eq_0     <0 */
    /* 10  D_lt_0 S_eq_0 N_lt_0     <0 */
    /* 11  D_lt_0 S_eq_0 N_gt_0 *** <0 */
    /* 12  D_lt_0 S_lt_0 N_eq_0     <0 */
    /* 13  D_lt_0 S_lt_0 N_lt_0     <0 */
    /* 14  D_lt_0 S_lt_0 N_gt_0  ** <0 */
    /* 15  D_lt_0 S_gt_0 N_eq_0  ** <0 */
    /* 16  D_lt_0 S_gt_0 N_lt_0  ** <0 */
    /* 17  D_lt_0 S_gt_0 N_gt_0 *** <0 */
    /* 18  D_gt_0 S_eq_0 N_eq_0     >0 */
    /* 19  D_gt_0 S_eq_0 N_lt_0  ** >0 */
    /* 20  D_gt_0 S_eq_0 N_gt_0     >0 */
    /* 21  D_gt_0 S_lt_0 N_eq_0  ** >0 */
    /* 22  D_gt_0 S_lt_0 N_lt_0 *** >0 */
    /* 23  D_gt_0 S_lt_0 N_gt_0  ** >0 */
    /* 24  D_gt_0 S_gt_0 N_eq_0     >0 */
    /* 25  D_gt_0 S_gt_0 N_lt_0  ** >0 */
    /* 26  D_gt_0 S_gt_0 N_gt_0     >0 */

    case D_eq_0 + S_lt_0 + N_gt_0:
    case D_lt_0 + S_lt_0 + N_gt_0:
	nsec -= AD_42DTE_INSUPPERLIMIT;		/* remove a second */
	secs++;					/* add a second */
	break;

    case D_lt_0 + S_eq_0 + N_gt_0:
    case D_lt_0 + S_gt_0 + N_gt_0:
	nsec -= AD_42DTE_INSUPPERLIMIT;		/* remove a second */
	secs++;					/* add a second */
	/*  FALLTHROUGH to remove+add a day */

    case D_lt_0 + S_gt_0 + N_eq_0:
    case D_lt_0 + S_gt_0 + N_lt_0:
	secs -= AD_40DTE_SECPERDAY;		/* remove a day */
	days++;					/* add a day */
	break;

    case D_eq_0 + S_gt_0 + N_lt_0:
    case D_gt_0 + S_gt_0 + N_lt_0:
	nsec += AD_42DTE_INSUPPERLIMIT;		/* add a second */
	secs--;					/* remove a second */
	break;

    case D_gt_0 + S_eq_0 + N_lt_0:
    case D_gt_0 + S_lt_0 + N_lt_0:
	nsec += AD_42DTE_INSUPPERLIMIT;		/* add a second */
	secs--;					/* remove a second */
	/*  FALLTHROUGH to add+remove a day */
	
    case D_gt_0 + S_lt_0 + N_eq_0:
    case D_gt_0 + S_lt_0 + N_gt_0:
	secs += AD_40DTE_SECPERDAY;		/* add a day */
	days--;					/* remove a day */
	break;

    }

    d->dn_day = days;
    d->dn_seconds = secs;
    d->dn_nsecond = nsec;

    /* normalize years and months */
    flt_days     = days + secs / AD_40DTE_SECPERDAY;
    flt_years    = (i4)d->dn_year;
    flt_months   = (i4)d->dn_month;
    flt_years   += flt_months / 12;
    d->dn_year   = years = flt_years;

    /*
        Since conversion from double to long truncates the double,
            round here to hide any floating point accuracy
            problems.
        Bug 1344 peb 6/25/83.
    */

    flt_temp = (flt_years - d->dn_year) * 12;
    d->dn_month = months = flt_temp + (flt_temp >= 0.0 ? 0.5 : -0.5);

    /* calculate number of days in interval */
    month_days   = flt_years * AD_14DTE_DAYPERYEAR + flt_days;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_006_NORMDATE, &dum1, &dum2))
        TRdisplay("normldint: interval converted to days = %d\n", (i4)month_days); 
#   endif

    /* now normalize status bits according to LRC-approved rules */

    status = d->dn_status;

    if (years != 0)
    {
	status |= AD_DN_YEARSPEC;
	
	if (months == 0)
	    status &= ~AD_DN_MONTHSPEC;
	else status |= AD_DN_MONTHSPEC;
    }
    else
    {
	if (months != 0)
	{
	    status &= ~AD_DN_YEARSPEC;
	    status |= AD_DN_MONTHSPEC;
	}
	else
	{
	    if (status & AD_DN_YEARSPEC)
		status &= ~AD_DN_MONTHSPEC;
	}
    }

    if (days != 0)
    {
	status |= AD_DN_DAYSPEC;
	
	if (secs == 0)
	    status &= ~AD_DN_TIMESPEC;
    }
    else
    {
	if (secs != 0)
	{
	    status &= ~AD_DN_DAYSPEC;
	    status |= AD_DN_TIMESPEC;
	}
	else
	{
	    if (status & AD_DN_DAYSPEC)
		status &= ~AD_DN_TIMESPEC;
	}
    }

    d->dn_status    = status;
    d->dn_day    = days;

    return((i4)month_days);
}


/*
**  Name:  adu_4date_cmp() - Internal routine used for comparing dates
**
**  Parameters:
**	adf_scb -- ADF session control block
**      dv1     -- first date
**      dv2     -- second date
**	cmp     -- Result of comparison, as follows:
**			<0  if  dv1 <  dv2
**			>0  if  dv1 >  dv2
**			=0  if  dv1 == dv2
**
**  Since intervals are always < absolute dates, the status of the dates
**  are first checked. If one date is absolute and the other an interval
**  the return code is set (absolute > interval) without looking at the
**  actual dates.
**
**  Empty dates are not compared to either intervals or absolute dates.
**  The empty date is considered to be less than all other dates values,
**  but equal to another empty date.
**
**  Returns:
**	E_DB_OK
**
**  History:
**	03-feb-87 (thurston)
**	    Upgraded the adu_4date_cmp() routine to be able to be a function
**	    instance routine, as well as the adc_compare() routine for dates.
**	06-sep-90 (jrb)
**	    Use adf_timezone (session specific) rather than call TMzone.
**      29-aug-92 (stevet) 
**          Modified timezone adjustment method by call TMtz_search()
**          to calcuate correct timezone offset based on a given time
**          value.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_4date_cmp for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	25-sep-2006 (gupsh01)
**	    Added support for normalizing time to UTC for time/timestamp
**	    with time zone types.
**	07-nov-2006 (gupsh01)
**	    Fixed the second calculations.
**	02-jan-2007 (gupsh01)
**	    Account for the nanosecond difference if the absolute dates/time
**	    values are same upto nanoseconds.
**	04-feb-2007 (gupsh01)
**	    Fixed the sorting of the time values.
**	18-Feb-2008 (kiria01) b120004
**	    Remove timezone code from compare as dealing with UTC times
**	11-May-2008 (kiria01) b120004
**	    Apply any defered application of time/timezone if needed.
**      24-Mar-2009 (coomi01) b121830
**          Test for normalisation pending flag, and normalise before
**          comparing if required.
*/

/*ARGSUSED*/
DB_STATUS
adu_4date_cmp(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
i4		*cmp)
{
    DB_STATUS db_stat = E_DB_OK;
    i4	    days1;
    i4	    days2;
    i4	    tmp_val;
    AD_NEWDTNTRNL    *d1;
    AD_NEWDTNTRNL    *d2;
    AD_NEWDTNTRNL    tempd1;
    AD_NEWDTNTRNL    tempd2;     /* aligned storage */

    
    /*
    **  MEcopy()'s were put in for BYTE_ALIGN but are needed even
    **      on VAX as we can't do comparisions in place since
    **      we normalize the values in place.
    **  Think what happens when the sorter does these comparisons
    **      (effectively changing the internal representation
    **      to the normalized representation) and then returns
    **      these to the query processor to be stuffed into a
    **      temporary (or a new relation in modify!!!).
    **
    **  Change 8/24/83 by peb.
    */

    if (db_stat = adu_6to_dtntrnl (adf_scb, dv1, &tempd1))
	return (db_stat);
    if (db_stat = adu_6to_dtntrnl (adf_scb, dv2, &tempd2))
	return (db_stat);

    d1 = &tempd1;
    d2 = &tempd2;

    if     (     d1->dn_status == AD_DN_NULL && d2->dn_status == AD_DN_NULL)
        tmp_val = 0;
    else if(     d1->dn_status == AD_DN_NULL && d2->dn_status != AD_DN_NULL)
        tmp_val = -1;
    else if(     d1->dn_status != AD_DN_NULL && d2->dn_status == AD_DN_NULL)
        tmp_val = 1;
    else if(!(d1->dn_status & AD_DN_ABSOLUTE && d2->dn_status & AD_DN_ABSOLUTE))
    {
        if (d1->dn_status & AD_DN_ABSOLUTE)
        {
            /* d1 is absolute and d2 an interval; d1 > d2 */

            tmp_val = 1;
        }
        else if (d2->dn_status & AD_DN_ABSOLUTE)
        {
            /* d1 is an interval and d2 absolute; d1 < d2 */

            tmp_val = -1;
        }
        else        /* both dates are intervals */
        {
            days1   = adu_2normldint(d1);
            days2   = adu_2normldint(d2);

            if (!(tmp_val = days1 - days2))
                tmp_val = d1->dn_seconds - d2->dn_seconds;
        }
    }
    else
    {
	if ((d1->dn_status2 ^ d2->dn_status2) & AD_DN2_ADTE_TZ)
	{
	    if (db_stat = adu_dtntrnl_pend_time(adf_scb, d1))
		return (db_stat);
	    if (db_stat = adu_dtntrnl_pend_time(adf_scb, d2))
		return (db_stat);
	}
	if ((d1->dn_status2 ^ d2->dn_status2) & AD_DN2_NO_DATE)
	{
	    if (db_stat = adu_dtntrnl_pend_date(adf_scb, d1))
		return (db_stat);
	    if (db_stat = adu_dtntrnl_pend_date(adf_scb, d2))
		return (db_stat);
	}

	/* 
	** Finally, check for normalisation pending flag 
	*/
	if (d1->dn_status2 & AD_DN2_NORM_PEND)
	{
	    adu_1normldate(d1);
	}
	if (d2->dn_status2 & AD_DN2_NORM_PEND)
	{
	    adu_1normldate(d2);
	}

        /* both dates are absolute */
        if (!(tmp_val = d1->dn_year - d2->dn_year) &&
            !(tmp_val = d1->dn_month - d2->dn_month) &&
            !(tmp_val = d1->dn_day - d2->dn_day))
            tmp_val = d1->dn_seconds - d2->dn_seconds;
    }

    if (tmp_val == 0)
    {
	/* check the nanosecond diff */
        tmp_val = d1->dn_nsecond - d2->dn_nsecond;
    }

    if (tmp_val < 0)
	*cmp = -1;
    else if (tmp_val > 0)
	*cmp = 1;
    else
	*cmp = 0;

    return(E_DB_OK);
}


/*
**  Name:  adu_dlenchk() - Required function for checking create length
**                         and precision
**
**  Parameters:
**      length  -- length in create specification
**      prec    -- precision in create specification
**
**  Returns:
**      true    -- valid size and precision
**      false   -- invalid size or precision
**
*/
bool
adu_dlenchk(
i4  *length,
i4  prec)
{
    return(*length == ADF_DTE_LEN && prec == 0);
}



/*
**  Name:  adu_3day_of_date() - Compute absolute number of days since epoch
**                              for the given ABSOLUTE date.
**
**  Description:
**	This is used for computing intervals in date subtraction and for
**	computing the day of week for a given date.
**
**	NOTE: The epoch is Jan 1, 1582 for all date operations.
**
**  Parameters:
**      date    -- internal date
**
**  Returns:
**      days since epoch for this date.
**
**  History
**	13-jan-87 (daved)
**	    avoid loop in computing days in year.
**	10-feb-1998 (hanch04)
**	    changed PRE_EPOCH_DAYS when calculating days to support
**	    years 0001 - 9999, EPOCH will be calculated as 14-sep-1752
**	    dates between, but not including  02-sep-1752 and 14-sep-1752
**	    do not exist.
**	19-May-2004 (schka24)
**	    Eliminate time consuming loop calculating days-before-
**	    this-month, use table lookup.
**	1-Jul-2004 (schka24)
**	    Back out year optimization, wrong because of integer divide.
**	    Century dates prior to 1752 were in fact leap years.
**	    Note that we're assuming the English calendar here.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_3day_of_date for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
*/

i4
adu_3day_of_date(
register AD_NEWDTNTRNL  *d)
{
    register i4 i;
    register i4 days;
    i4		yr;

    days = 0;

    /* Compute number of days in previous years , 14-sep-1752 */
#define PRE_EPOCH_DAYS 639810

    yr = d->dn_year - 1;
    /* 365 days a year plus one on leap year, except every hundred years
    ** not divisible by 400.
    ** Can't do y/4 - y/100 + y/400 = 97y/400, because the individual
    ** divisions truncate differently than the combined.
    ** 1800 was the first century year to not be a leap year, after which
    ** 2000 was the first quadricentury year to be a leap year.
    ** 1700 WAS a leap year, hence the -1 (adding it back in), and
    ** setting i=1 for early dates makes it all work out...
    */
    i = 1;
    if (yr >= 1800) i = (yr-1600) / 100;
    days = 365 * yr + (yr >> 2) - (i-1) + (i >> 2);

    /* Compute number of days in previous months */

    days += daysBefore[d->dn_month];

    /* Check for leap year, minimizing divides on modern day hardware.
    ** According to the Julian calendar, there was no century leap year
    ** adjustment.  The English calendar switched in 1752.
    ** Very old dates assume a backwards extension of the Julian calendar,
    ** but of course that's strictly a convenience and becomes increasingly
    ** bogus as you go backwards.
    */

    i = d->dn_year % 400;
    if (d->dn_month >= 3 && ((d->dn_year & 3) == 0)
      && (days < PRE_EPOCH_DAYS || (i != 100 && i != 200 && i != 300)) )
	++ days;

    /* For absolute date, high order day byte is always zero */
    /* Add in number of days in current month and return */

    days += d->dn_day;
    if (days < PRE_EPOCH_DAYS)
    {
	if (days > (PRE_EPOCH_DAYS - 11))
	    days = (PRE_EPOCH_DAYS - 12);
    }
    else
	days -= 11;

    return ( days ); 
              
}   



/*
**  Name:  adu_5time_of_date() - Compute the number of seconds since 1/1/70
**                               00:00. 
**
**  Description:
**	This internal routine is used for calculating number of seconds since
**      1/1/70 00:00.
**
**      The number of seconds is returned in an I4 and capped and MIN/MAX I4.
**      This means that the range is circa +/- 68 years
**
**  Parameters:
**      d    -- internal date
**
**  Returns:
**      seconds since 1/1/70 00:00.
**
**  History
**	25-sep-1992 (stevet)
**	    Initial creation.
**	17-jul-2006 (gupsh01)
**	    Added support in adu_5time_of_date for new internal format 
**	    AD_NEWDTNTRNL for ANSI datetime support.
**	07-nov-2006 (gupsh01)
**	    Fixed the seconds calculations.
**      05-Nov-2009 (horda03) Bug 122859
**          If the date is not normalized, then the seconds could be
**          negative. Modified the way that the boundary conditions
**          are detected.
**      12-Nov-2009 (horda03) b122859
**          Fix typo.
*/
i4
adu_5time_of_date(
register AD_NEWDTNTRNL  *d)
{
    register i4  i;
    register i4  days;
    register i4  yr;
    i4          pos_days;

    /* Compute number of days in previous years */
#define DAYS_SINCE_1970  719163
#define MAX_DAYS          24855   /* Maximum number of days that can be converted to
                                  ** seconds and held in an I4
                                  ** 1 DAY = 86400 seconds
                                  ** MAXI4 / 86400 = 24855
                                  */
#define MAX_LAST_DAY_SECS 11647   /* MAXI4 - (MAXI4 / 86400 * 86400) */

    yr = d->dn_year - 1;
    /* 365 days a year plus one on leap year, except every hundred years
    ** not divisible by 400.
    **
    ** This doesn't handle the lost days due to the switch to Gregorian
    ** Calendar. But this occured beyond the range of seconds that can be stored in
    ** an I4 from 1-Jan-1970; so not an issue.
    */
    days = 365 * yr - DAYS_SINCE_1970 + yr/4 - yr/100 + yr/400;

    /* Compute number of days in previous months */

    for (i = 1; i < d->dn_month; i++)
        days += adu_2monthsize((i4) i, (i4) d->dn_year);

    /* Always absolute date, do not need to look at high day */

    days += d->dn_day;

    pos_days = max(days, -days);

    if (pos_days > MAX_DAYS)
    {
       return (days > 0) ? MAXI4 : MINI4;
    }
    else if ( pos_days == MAX_DAYS)
    {
       /* We're close to the boundary, so need to check the seconds. */

       if ( (days < 0) && (d->dn_seconds < -MAX_LAST_DAY_SECS) )
       {
          return MINI4;
       }
       else if ( (days > 0) && (d->dn_seconds > MAX_LAST_DAY_SECS) )
       {
          return MAXI4;
       }
    }

    /* To reach here means that the Seconds since 1-Jan-1970 can be stored
    ** in an I4.
    */
    return (days * AD_41DTE_ISECPERDAY) + d->dn_seconds;
}   


/*
**  Name:  ad0_tz_offset	- Return appropriate tz offset in seconds
**
**  Description:
**
**  Parameters:
**      adf_scb		- ptr to SCB
**	base		- ptr to AD_NEWDTNTRNL which is to be used as the
**			  date off which to bias the TZ. If passed as NULL
**			  the bias date will be taken from current time.
**	src_type	- TM_TIMETYPE of TM_TIMETYPE_LOCAL or TM_TIMETYPE_GMT
**	offset		- ptr to i4 to receive the TZ offset value in seconds.
**
**  Returns:
**      DB_STATUS
**
**  History
**	18-Feb-2008 (kiria01) b120004
**	    Created to factor out timezone calculating.
**	01-May-2008 (kiria01) b120004
**	    Catch calls where base is passed but with no
**	    date present (ie. from TME,TMW or TMWO)
*/
static DB_STATUS
ad0_tz_offset(
ADF_CB              *adf_scb,
AD_NEWDTNTRNL	    *base,
TM_TIMETYPE	    src_type,
i4		    *tzoffset)
{
    STATUS	     db_stat = E_DB_OK;

    if (!base || base->dn_status2 & AD_DN2_NO_DATE)
	/* Just get the TZ cheaply */
	return ad0_1cur_init(adf_scb, NULL, TRUE, FALSE, tzoffset);

    /* Record the Session time zone displacement */
    *tzoffset = TMtz_search (adf_scb->adf_tzcb, src_type, 
				adu_5time_of_date(base));
    return (db_stat);
}

/*
**  Name:  adu_6to_dtntrnl	- map a value of one of the supported types
**	to the ADF internal format
**
**  Description:
**
**  Parameters:
**      indv		- ptr to DB_DATA_VALUE for input value
**	outval		- ptr to AD_NEWDTNTRNL into which value is written
**
**  Returns:
**      none
**
**  NOTE: This routine MUST initialise ALL fields of the outval structure
**
**  History
**	21-apr-06 (dougi)
**	    Written for date/time enhancements.
**	29-jun-2006 (gupsh01)
**	    Modified setting up the status bit and verifying.
**	19-jul-2006 (gupsh01)
**	    Modified setting up the vacant fields.
**	01-aug-2006 (gupsh01)
**	    Fixed memory alignment for ANSI date/time internal
**	    data.
**	02-aug-2006 (gupsh01)
**	    Fixed the copying of Timestamp without timezone 
**	    datatypes.
**	22-sep-06 (gupsh01)
**	    Even for empty values of ANSI date/times set the 
**	    flags so 0 is printed out.
**	16-sep-06 (gupsh01)
**	    Make sure that we check for absolute data type value.
**	20-oct-06 (gupsh01)
**	    Protect this routine from segv if the input and output
**	    pointers are uninitialized.
**	22-nov-06 (gupsh01)
**	    Modified so that the status flag is set only for non 
**	    zero values of date and time.
**	13-nov-06 (gupsh01)
**	    Fixed the time with time zone case and timestamp with
**	    local time zone.
**	18-Feb-2008 (kiria01) b120004
**	    Reworked to do the brunt of conversion work and added
**	    handling.
**	10-Apr-2008 (kiria01) b120004
**	    Ensure that zero timestamp is a valid Absolute and not
**	    treated as null date.
**	17-Apr-2008 (hweho01)
**	    Avoid SIGBUS on some plastforms, copy caller's data  
**	    to local buffer, so the fields are aligned. 
**	07-May-2008 (hweho01)
**	    Modified the previous change on 16-Apr-2008 :
**	    1) only copy the data to local buffer if it is not aligned,
**	    2) check the length of data to be copied, prevent buffer 
**	       overrun. 
**	11-May-2008 (kiria01) b120004
**	    Defered application of time/timezone from pure dates.
**	20-May-2008 (kiria01) b120004
**	    When converting from pure date to TSWO or TMWO, leave the
**	    time as 00:00.
**	26-Feb-2009 (kiria01) b121734
**	    Don't calculate the status bits for TSTMP, TSW and TSWO
**	    after TZ adjustment as this will lose empty dates with
**	    non-GMT timezones.
**      17-Apr-2009 (horda03) b121949
**          Set AD_DN2_ADTE_TZ for an ABSOLUTE IngresDate with no
**          AD_DN_TIMESPEC.
**      17-jul-2009 (horda03) b122337
**          For an ABSOLUTE Ingresdate check to see if it need to be
**          normalised. In olden days, a date( '10:10:10') could have
**          a negative Num seconds, or num seconds > MAX SECS PER DAY
**          depending on the timezone of the session entering the
**          value.
**      11-nov-2009 (horda03) b122871
**          Set dn_status flags based on the DATE type being converted.
*/
DB_STATUS
adu_6to_dtntrnl(
ADF_CB         *adf_scb,
DB_DATA_VALUE	*indv,
AD_NEWDTNTRNL	*outval)
{
    DB_STATUS   db_stat = E_DB_OK;
    AD_DTUNION	*dp, date_buff;
    i4		offset;
    if (indv && outval && (dp = (AD_DTUNION*)indv->db_data))
    {
	bool need_tz = FALSE;

	/* NOTE - All AD_NEWDTNTRNL fields must be initialised
	** this routine
	*/
	outval->dn_dttype = abs(indv->db_datatype);
	AD_TZ_SETNEW(outval, 0);
	outval->dn_status = 0;
	outval->dn_status2 = 0;

	if(ME_ALIGN_MACRO(indv->db_data, DB_ALIGN_SZ) != (PTR) indv->db_data) 
        { 
	  /* Copy data to buffer, so the fields are aligned correctly. */
	  MEcopy((PTR)indv->db_data, (indv->db_length > sizeof(date_buff))
	         ? sizeof(date_buff) : indv->db_length, (PTR)&date_buff );
	  dp = &date_buff;
        }

	/* Just switch to the relevant conversion code and do it. */
	switch(outval->dn_dttype)
	{
	case DB_DTE_TYPE:		/* old Ingres DATE */
	    outval->dn_year = dp->ing_date.dn_year;
	    outval->dn_month = dp->ing_date.dn_month;
	    outval->dn_day = dp->ing_date.dn_lowday |
		    (dp->ing_date.dn_highday << AD_21DTE_LOWDAYSIZE);
	    outval->dn_seconds = dp->ing_date.dn_time/AD_6DTE_IMSPERSEC;
	    outval->dn_nsecond = (dp->ing_date.dn_time % AD_6DTE_IMSPERSEC) *
							AD_38DTE_NSPERMS;
	    outval->dn_status = dp->ing_date.dn_status;
	    if (outval->dn_status & AD_DN_TIMESPEC &&
		    outval->dn_status & AD_DN_ABSOLUTE)
            {
		need_tz = TRUE;

                if ( (u_i4)outval->dn_seconds >= AD_41DTE_ISECPERDAY)
                {
                   /* An ABSOLUTE date shouldn't have -ive nor >AD_41DTE_ISECPERDAY
                   ** so need to normalise.
                   */
                   outval->dn_status2 |= AD_DN2_NORM_PEND;
                }
            }
	    else if (outval->dn_status & AD_DN_ABSOLUTE)
	    {
		outval->dn_status2 |= AD_DN2_ADTE_TZ;
	    }
	    break;

	case DB_ADTE_TYPE:	/* new standard DATE */
	    outval->dn_year = dp->adate.dn_year;
	    outval->dn_month = dp->adate.dn_month;
	    outval->dn_day = dp->adate.dn_day;
	    outval->dn_seconds = 0;
	    outval->dn_nsecond = 0;

            outval->dn_status = (AD_DN_ABSOLUTE  | AD_DN_YEARSPEC | 
                                 AD_DN_MONTHSPEC | AD_DN_DAYSPEC);
	    outval->dn_status2 |= AD_DN2_ADTE_TZ;
	    break;

	case DB_TME_TYPE:	/* Ingres TIME (WITH LOCAL TIME ZONE) */
	    need_tz = 1;
	    /*FALLTHROUGH*/
	case DB_TMW_TYPE:	/* TIME WITH TIME ZONE */
	case DB_TMWO_TYPE:	/* TIME WITHOUT TIME ZONE */
	    outval->dn_year = 0;
	    outval->dn_month = 0;
	    outval->dn_day = 0;
	    outval->dn_seconds = dp->atime.dn_seconds;
	    outval->dn_nsecond = dp->atime.dn_nsecond;
	    if (outval->dn_dttype == DB_TMW_TYPE)
	    {
		AD_TZ_SETNEW(outval, AD_TZ_OFFSET(&dp->atime));
		if (AD_TZ_ISLEGACY(&dp->atime))
		{
		    /* Might need to normalise if legacy */
		    outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		    outval->dn_status2 |= AD_DN2_NORM_PEND;
		}
		outval->dn_status2 |= AD_DN2_TZ_OFF_LCL;
	    }
	    else if (outval->dn_dttype == DB_TMWO_TYPE)
	    {
		if (!adf_scb)
		    offset = 0;
		else if (db_stat = ad0_tz_offset(adf_scb, NULL,
						TM_TIMETYPE_LOCAL, &offset))
		    return (db_stat);
		AD_TZ_SETNEW(outval, offset);
		outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		outval->dn_status2 |= AD_DN2_NORM_PEND;
	    }
            outval->dn_status = (AD_DN_ABSOLUTE | AD_DN_TIMESPEC);
	    outval->dn_status2 |= AD_DN2_NO_DATE;
	    /* Set the following if TME/W/WO are to work with intervals
	    ** which is accepted by 9.1.0 but is probably wrong */
	    /* outval->dn_status2 |= AD_DN2_ABS_OR_INT;*/
	    break;

	case DB_TSTMP_TYPE:	/* Ingres TIMESTAMP (WITH LOCAL TIME ZONE) */
	    need_tz = 1;
	    /*FALLTHROUGH*/
	case DB_TSW_TYPE:		/* TIMESTAMP WITH TIME ZONE */
	case DB_TSWO_TYPE:	/* TIMESTAMP WITHOUT TIME ZONE */
            outval->dn_year = dp->atimestamp.dn_year;
            outval->dn_month = dp->atimestamp.dn_month;
            outval->dn_day = dp->atimestamp.dn_day;
            outval->dn_seconds = dp->atimestamp.dn_seconds;
            outval->dn_nsecond = dp->atimestamp.dn_nsecond;
            outval->dn_status = (AD_DN_ABSOLUTE  | AD_DN_YEARSPEC |
                                 AD_DN_MONTHSPEC | AD_DN_DAYSPEC  |
                                 AD_DN_TIMESPEC);
	    if (outval->dn_dttype == DB_TSW_TYPE)
	    {
		AD_TZ_SETNEW(outval, AD_TZ_OFFSET(&dp->atimestamp));
		if (AD_TZ_ISLEGACY(&dp->atimestamp))
		{
		    /* Might need to normalise if legacy */
		    outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		    outval->dn_status2 |= AD_DN2_NORM_PEND;
		}
		outval->dn_status2 |= AD_DN2_TZ_OFF_LCL;
	    }
	    else if (outval->dn_dttype == DB_TSWO_TYPE)
	    {
		if (!adf_scb)
		    offset = 0;
		else if (db_stat = ad0_tz_offset(adf_scb, outval,
						TM_TIMETYPE_LOCAL, &offset))
		    return (db_stat);
		AD_TZ_SETNEW(outval, offset);
		outval->dn_seconds -= AD_TZ_OFFSETNEW(outval);
		outval->dn_status2 |= AD_DN2_NORM_PEND;
	    }
	
	    /* Set the following if TS/W/WO are to work with intervals
	    ** which is accepted by 9.1.0 but is probably wrong */
	    /* outval->dn_status2 |= AD_DN2_ABS_OR_INT; */
	    break;

	case DB_INYM_TYPE:	/* INTERVAL YEAR TO MONTH */
	    outval->dn_year = dp->aintym.dn_years;
	    outval->dn_month = dp->aintym.dn_months;
	    outval->dn_day = 0;
	    outval->dn_seconds = 0;
	    outval->dn_nsecond = 0;
	    outval->dn_status = (AD_DN_LENGTH | AD_DN_YEARSPEC | 
                                 AD_DN_MONTHSPEC); 
	    break;

	case DB_INDS_TYPE:	/* INTERVAL DAY TO SECOND */
	    outval->dn_year = 0;
	    outval->dn_month = 0;
	    outval->dn_day = dp->aintds.dn_days;
	    outval->dn_seconds = dp->aintds.dn_seconds;
	    outval->dn_nsecond = dp->aintds.dn_nseconds;
	    outval->dn_status = (AD_DN_LENGTH | AD_DN_DAYSPEC |
                                 AD_DN_TIMESPEC); 
	    break;
	
	default:
	    if (adf_scb)
		return(adu_error(adf_scb, E_AD1001_BAD_DATE, 1,
					outval->dn_dttype));
	}
	if (need_tz)
	{
	    if (!adf_scb)
		offset = 0;
	    else if (db_stat = ad0_tz_offset(adf_scb, outval,
					TM_TIMETYPE_GMT, &offset))
		return (db_stat);
	    AD_TZ_SETNEW(outval, offset);
	    outval->dn_status2 |= AD_DN2_TZ_OFF_LCL;
	}
    }
    else if (adf_scb)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    else
	db_stat = E_AD9999_INTERNAL_ERROR;
    return (db_stat);
}


/*
**  Name:  adu_dtntrnl_pend_date - set date if needed
**
**  Description:
**
**  Parameters:
**	inval		- ptr to AD_NEWDTNTRNL from which value is copied
**
**  Returns:
**      none
**
**  History
**	28-Mar-2008 (kiria01) b120004
**	    Created to allow pending date to be set.
*/
DB_STATUS
adu_dtntrnl_pend_date (
ADF_CB         *adf_scb,
AD_NEWDTNTRNL	*inval)
{
    DB_STATUS   db_stat = E_DB_OK;

    if (inval->dn_status2 & AD_DN2_NO_DATE)
    {
	struct timevect cur_tv;
	i4 tzoff;

	/* Initialize current time vector */ 
	if (db_stat = ad0_1cur_init(adf_scb, &cur_tv, FALSE, FALSE, &tzoff))
	    return (db_stat);

	/* No date component provided and we need one */
	inval->dn_year	= cur_tv.tm_year + AD_TV_NORMYR_BASE;
	inval->dn_month = cur_tv.tm_mon + AD_TV_NORMMON;
	inval->dn_day	= cur_tv.tm_mday;
	inval->dn_status |= AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			    AD_DN_DAYSPEC; 
	inval->dn_status2 &= ~AD_DN2_NO_DATE;
    }
    return (db_stat);
}


/*
**  Name:  adu_dtntrnl_pend_time - set time to midnight GMT if needed
**
**  Description:
**
**  Parameters:
**	inval		- ptr to AD_NEWDTNTRNL from which value is copied
**
**  Returns:
**      none
**
**  History
**	11-May-2008 (kiria01) b120004
**	    Created to allow pending time to be set.
*/
DB_STATUS
adu_dtntrnl_pend_time (
ADF_CB         *adf_scb,
AD_NEWDTNTRNL	*inval)
{
    DB_STATUS   db_stat = E_DB_OK;

    if (inval->dn_status2 & AD_DN2_ADTE_TZ)
    {
	i4 offset = 0;
	if (adf_scb && (db_stat = ad0_tz_offset(adf_scb, inval,
					TM_TIMETYPE_LOCAL, &offset)))
	    return db_stat;
	AD_TZ_SETNEW(inval, offset);
	/* We adjust for gmt and note that tz is saved */
	inval->dn_seconds = -offset;
	inval->dn_status |= AD_DN_TIMESPEC;
	inval->dn_status2 &= ~AD_DN2_ADTE_TZ;
	inval->dn_status2 |= (AD_DN2_TZ_OFF_LCL|AD_DN2_NORM_PEND);
    }
    return (db_stat);
}


/*
**  Name:  adu_7from_dtntrnl	- map a value of one of the supported types
**	from the ADF internal format to the storage format
**
**  Description:
**
**  Parameters:
**      outdv		- ptr to DB_DATA_VALUE for output value
**	inval		- ptr to AD_NEWDTNTRNL from which value is copied
**
**  Returns:
**      none
**
**  History
**	21-apr-06 (dougi)
**	    Written for date/time enhancements.
**	17-jun-06 (gupsh01)
**	    Fixed the DB_DTE_TYPE low/high day calculation.
**	01-aug-06 (gupsh01)
**	    Fixed time calculation for DB_DTE_TYPE considering
**	    the internal structure now holds nanoseconds.
**	08-sep-06 (gupsh01)
**	    Fixed the sigbus error calling this function.
**	16-sep-06 (gupsh01)
**	    Make sure that we check for absolute data type value.
**	20-oct-06 (gupsh01)
**	    Protect this routine from segv if the input and output
**	    pointers are uninitialized.
**	5-Nov-07 (kibro01) b119412
**	    Apply timezone and normalise since AD_DATENTRNL doesn't
**	    have a timezone, so it would get lost.
**	18-Feb-2008 (kiria01) b120004
**	    Reworked to do the brunt of conversion work and added
**	    handling.
**	11-May-2008 (kiria01) b120004
**	    Apply any defered application of time/timezone if needed.
**	20-May-2008 (kiria01) b120004
**	    When converting from pure date to TSWO or TMWO, leave the
**	    time as 00:00. Otherwise, when converting to TSWO or TMWO
**	    denormalise any TZ specified. Correct INTDS rounding error.
**	26-Nov-2008 (kibro01) b121250
**	    Adjust output value for a DTE type based on the required
**	    output precision so we don't end up with extraneous nanoseconds.
**	12-Dec-2008 (kiria01) b121366
**	    Correct potential abend on outdv
*/
DB_STATUS
adu_7from_dtntrnl(
ADF_CB         *adf_scb,
DB_DATA_VALUE	*outdv,
AD_NEWDTNTRNL	*inval)
{
    DB_STATUS   db_stat = E_DB_OK;
    AD_DTUNION	*dp, _dp;
    i4		nsecs;

    /* Switch on target data type and copy the fields over. */
    if (outdv && inval && (dp = (AD_DTUNION*)outdv->db_data))
    {
	if (ME_ALIGN_MACRO(dp, sizeof(i4)) != (PTR)dp)
	    dp = &_dp;
	switch (abs(outdv->db_datatype))
	{
	case DB_TME_TYPE:
	case DB_TMW_TYPE:
	    if (inval->dn_status2 & AD_DN2_ADTE_TZ)
	    {
		if (db_stat = adu_dtntrnl_pend_time(adf_scb, inval))
		    return db_stat;
	    }
	    /*FALLTHROUGH*/
	case DB_TMWO_TYPE: /* Leave WO form with 00:00 from pure date */
	    /* Don't need the date */
	    break;
	case DB_INYM_TYPE:
	case DB_INDS_TYPE:
	    /* None of these types will use the date */
	    break;
	case DB_TSTMP_TYPE:
	case DB_TSW_TYPE:
	    if (inval->dn_status2 & AD_DN2_ADTE_TZ)
	    {
		if (db_stat = adu_dtntrnl_pend_time(adf_scb, inval))
		    return db_stat;
		/* We have the date already */
		break;
	    }
	    /*FALLTHROUGH*/
	case DB_TSWO_TYPE: /* Leave WO form with 00:00 from pure date */
	default:
	    /* Getdate assigned if delayed */ 
	    if (db_stat = adu_dtntrnl_pend_date(adf_scb, inval))
		return (db_stat);
	}
	switch (abs(outdv->db_datatype))
	{
	case DB_DTE_TYPE:
	    if (inval->dn_status2 & AD_DN2_NORM_PEND)
	    {
		if (inval->dn_status & AD_DN_ABSOLUTE)
		    adu_1normldate(inval);
		else
	    	    adu_2normldint(inval);
	    }

	    nsecs = inval->dn_nsecond;

	    /* Now adjust the nanoseconds based on the output value precision
	    ** if putting result in an absolute date. (kibro01) b121250
	    */
	    if (inval->dn_status & AD_DN_ABSOLUTE)
	    {
        	if (outdv->db_prec <= 0 || outdv->db_prec > 9)
                    nsecs = 0;
        	else
        	{
                    nsecs /= nano_mult[outdv->db_prec];
                    nsecs *= nano_mult[outdv->db_prec];
        	}
	    }

	    dp->ing_date.dn_year = inval->dn_year;
	    dp->ing_date.dn_month = inval->dn_month;
	    dp->ing_date.dn_highday = (inval->dn_day >> AD_21DTE_LOWDAYSIZE);
	    dp->ing_date.dn_lowday = (inval->dn_day & AD_22DTE_LOWDAYMASK);
	    dp->ing_date.dn_time = inval->dn_seconds * AD_6DTE_IMSPERSEC + 
				    nsecs/AD_29DTE_NSPERMS;
	    dp->ing_date.dn_status = inval->dn_status;
    
	    /* Make sure that the status field is 
	    ** consistent This will help fix up conversion to string 
	    ** If the date value may change due to various operations. 
	    */
	    if (dp->ing_date.dn_time)
		dp->ing_date.dn_status |= AD_DN_TIMESPEC;
	    /* If no date bits were set and we're absolute then treat
	    ** this as a time and don't update bits*/
	    if ((inval->dn_status2 & AD_DN2_DATE_DEFAULT) &&
		(dp->ing_date.dn_status & (
			AD_DN_YEARSPEC|AD_DN_MONTHSPEC|
			AD_DN_DAYSPEC|AD_DN_ABSOLUTE)) == AD_DN_ABSOLUTE)
		break;
	    if (dp->ing_date.dn_year)
		dp->ing_date.dn_status |= AD_DN_YEARSPEC;
	    if (dp->ing_date.dn_month)
		dp->ing_date.dn_status |= AD_DN_MONTHSPEC;
	    if (dp->ing_date.dn_lowday)
		dp->ing_date.dn_status |= AD_DN_DAYSPEC;
	    break;

        case DB_ADTE_TYPE:
	    if (inval->dn_status & AD_DN_TIMESPEC)
	    {
		if (inval->dn_status2 & AD_DN2_TZ_OFF_LCL)
		{
		    inval->dn_status2 &= ~AD_DN2_TZ_OFF_LCL;
		    inval->dn_status2 |= AD_DN2_NORM_PEND;
		    inval->dn_seconds += AD_TZ_OFFSETNEW(inval);
		}
		else
		{
		    i4 offset = 0;
		    if (adf_scb && (db_stat = ad0_tz_offset(adf_scb, inval,
							TM_TIMETYPE_GMT, &offset)))
			return (db_stat);
		    inval->dn_seconds += offset;
		    inval->dn_status2 |= AD_DN2_NORM_PEND;
		}
	    }
	    if (inval->dn_status2 & AD_DN2_NORM_PEND)
		adu_1normldate(inval);
	    dp->adate.dn_year = inval->dn_year;
	    dp->adate.dn_month = inval->dn_month;
	    dp->adate.dn_day = inval->dn_day;
	    break;

	case DB_TMW_TYPE:	/* TIME WITH TIME ZONE */
	    AD_TZ_SET(&dp->atime, AD_TZ_OFFSETNEW(inval));
	    goto TM_common;

	case DB_TMWO_TYPE:	/* TIME WITHOUT TIME ZONE */
	    /* Denormalize TZ if present */
	    inval->dn_seconds += AD_TZ_OFFSETNEW(inval);
	    /*FALLTHROUGH*/

	case DB_TME_TYPE:	/* Ingres TIME (WITH LOCAL TIME ZONE) */
	    /* Clear TZ in structure */
	    AD_TZ_SET(&dp->atime, 0);

TM_common:  if (inval->dn_status2 & AD_DN2_NORM_PEND)
		adu_1normldate(inval);
	    dp->atime.dn_seconds = inval->dn_seconds;
	    dp->atime.dn_nsecond = inval->dn_nsecond;
	    break;

        case DB_TSW_TYPE:	/* TIMESTAMP WITH TIME ZONE */
	    AD_TZ_SET(&dp->atimestamp, AD_TZ_OFFSETNEW(inval));
	    goto TS_common;

	case DB_TSWO_TYPE:	/* TIMESTAMP WITHOUT TIME ZONE */
	    /* Denormalize TZ if present */
	    inval->dn_seconds += AD_TZ_OFFSETNEW(inval);
	    /*FALLTHROUGH*/

	case DB_TSTMP_TYPE:	/* Ingres TIMESTAMP (WITH LOCAL TIME ZONE) */
	    /* Clear TZ in structure */
	    AD_TZ_SET(&dp->atime, 0);

TS_common:  if (inval->dn_status2 & AD_DN2_NORM_PEND)
		adu_1normldate(inval);
	    dp->atimestamp.dn_year = inval->dn_year;
	    dp->atimestamp.dn_month = inval->dn_month;
	    dp->atimestamp.dn_day = inval->dn_day;
	    dp->atimestamp.dn_seconds = inval->dn_seconds;
	    dp->atimestamp.dn_nsecond = inval->dn_nsecond;
	    break;

	case DB_INYM_TYPE:	/* INTERVAL YEAR TO MONTH */
	    adu_2normldint(inval);
	    dp->aintym.dn_years	= inval->dn_year;
	    dp->aintym.dn_months = inval->dn_month;
	    /* We have a oddity in the ANSI handling with TMW(O)*/
	    if (adf_scb &&
		    (inval->dn_status & AD_DN_ABSOLUTE) &&
		    !(inval->dn_status2 & AD_DN2_ABS_OR_INT))
		return(adu_error(adf_scb, E_AD505E_NOABSDATES, 0));
	    break;

	case DB_INDS_TYPE:	/* INTERVAL DAY TO SECOND */
	    adu_2normldint(inval);
	    dp->aintds.dn_days	= inval->dn_day;
	    if (inval->dn_month | inval->dn_year)
	    {
		f8 days = inval->dn_month * AD_13DTE_DAYPERMONTH +
			    inval->dn_year * AD_14DTE_DAYPERYEAR;
		if (days < 0)
		    days -= 0.5;
		else
		    days += 0.5;
		dp->aintds.dn_days += (i4)days;
	    }
	    dp->aintds.dn_seconds = inval->dn_seconds;
	    dp->aintds.dn_nseconds = inval->dn_nsecond;
	    if (adf_scb &&
		    (inval->dn_status & AD_DN_ABSOLUTE) &&
		    !(inval->dn_status2 & AD_DN2_ABS_OR_INT))
		return(adu_error(adf_scb, E_AD505E_NOABSDATES, 0));
	    break;

	default:
	    if (adf_scb)
		return(adu_error(adf_scb, E_AD1001_BAD_DATE, 1,
					abs(outdv->db_datatype)));
	}
	if (dp == &_dp)
	    MEcopy((PTR)dp, outdv->db_length, outdv->db_data);
    }
    else if (adf_scb)
	return (adu_error(adf_scb, (i4) E_AD9999_INTERNAL_ERROR, 0));
    else
	db_stat = E_AD9999_INTERNAL_ERROR;
    return (db_stat);
}

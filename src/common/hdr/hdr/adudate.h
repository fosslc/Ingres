/*
** Copyright (c) 2004,2008 Ingres Corporation
*/
#include <iicommon.h>

/**
** Name: ADUADTDATE.H - Date datatype definitions.
**
** Description:
**	Defines the INGRES internal representation of dates, MACROS, and
**  constants associated with dates.
**
**	The following typedefs are defined here:
**	    AD_DATENTRNL - Defines the INGRES internal representation of dates.
**
** History: $Log-for RCS$
**	Revision 3.22.177.0  85/06/21  13:32:05  wong
**	    Removed unsigned character hack that declared
**	    a different internal date structure for INGRES.
**	03-dec-85 (seputis)
**	    Added constant definitions.
**	26-nov-86 (thurston)
**	    Renamed all constants, typedefs, etc. to conform to Jupiter
**	    coding standards.
**	02-apr-87 (thurston)
**	    Added constants:  AD_23DTE_MIN_ABS_YR, AD_24DTE_MAX_ABS_YR,
**	    AD_25DTE_MIN_INT_YR, and AD_26DTE_MAX_INT_YR.
**	28-may-87 (thurston)
**	    Changed the value of AD_13DTE_DAYPERMONTH from 30.44 to the more
**	    accurate value of 30.4375.  Also changed the value of
**	    AD_14DTE_DAYPERYEAR from the erroneous 365.44 to the correct value
**	    of 365.25.
**	10-nov-88 (thurston)
**	    Spiffied up the look of this file so that it looks more presentable.
**	31-jan-92 (bonobo)
**	    Replaced multiple question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**      04-jan-92 (stevet)
**          Added function prototypes for internal date functions.
**	23-feb-1998 (hanch04)
**	    added AD_DN_AFTER_EPOCH, AD_DN_BEFORE_EPOCH, ADDT_EQ_EPOCH,
**	    ADDT_GTE_EPOCH, ADDT_LTE_EPOCH, ADDT_GT_EPOCH, ADDT_LT_EPOCH
**	    to support years 0001 - 9999,
**	    dates between, but not including 02-sep-1752 and 14-sep-1752
**	    do not exist.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	5-jan-01 (inkdo01)
**	    Added AD_AGGDATE to define sum/avg workspace for date datatype.
**      23-apr-2001 (horda03) Bug 104518 INGCBT 348
**          ADCVALCHK.C requires a list of valid DATE status bits. Originally,
**          it created its own list, but when date support for years 0001 to
**          9999 was added, this list got missed - which could lead to COPY IN
**          failing with binary dates. To ensure this doesn't happen again,
**          created AD_DN_VALID_DATE_STATES.
**	21-apr-06 (dougi)
**	    Add new structure definitions for ANSI date/time types and a 
**	    new internal structure to hold values of all types for ADF
**	    processing.
**	2-Jun-2006 (bonro01)
**	   Include iicommon.h for DB_DT_ID added by last change.
**	19-jun-2006 (gupsh01)
**	    Added new constants and macros for ANSI datetime support.
**	01-aug-2006 (gupsh01)
**	    Fixed lengths of new ANSI date/time types. Added new constants to  
**	    support seconds in dn_seconds.
**	11-dec-2006 (dougi)
**	    Added AD_45DTE_MAXSPREC to define max precision of time/timestamp/
**	    interval day to second seconds precision.
**	12-dec-2006 (gupsh01)
**	    Added AD_47DTE_MIN_TZ_SECS and AD_48DTE_MAX_TZ_SECS.
**      23-Jul-2007 (hanal04) Bug 118788
**          Change AD_47DTE_MIN_TZ_SECS to AD_47DTE_MIN_TZ_MN and
**          AD_48DTE_MAX_TZ_SECS to AD_48DTE_MAX_TZ_MN. The defines
**          are the min and max minutes for a TZ.
**	19-Sep-2007 (kibro01) b119154
**	    Correct external length of ansidate to 10 characters
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling
**	12-Feb-2009 (frima01) b120004
**	    Changed AD_TZ_HOURSNEW to use abs() since it didn't
**	    work properly with negative values on Unixware.
**	13-jun-2010 (stephenb)
**	    Add ADU_BIGGEST_MACRO to support expanding date lengths
**	    in aducalclen
**      24-Jun-2010 (horda03) B123987
**          Added AD_11DTE_INTRN_OUTLENGTH for the length of an Ingredate
**          Interval (when coverted to a string. Also added
**          AD_11_MAX_STRING_LEN which is set to the maximum length
**          a string for a DATE (ANSI or INGRESDATE) will take.
**/


/*}
** Name: AD_DATENTRNL - Defines the INGRES internal representation of dates.
**
** Description:
**	This typedef describes the INGRES internal format of a date.
**
**  Note: Internally, *ALL* dates/times are stored as GMT.
**
**  Starting with 3.0, we use the same date structure everywhere,
**  even if we have unsigned characters.  This means we need to be
**  careful with .dn_status and .dn_highday, which need to hold i1's.
**  (Should they be declared as i1's?)
**
** History:
**	Revision 3.22.177.0  85/06/21  13:32:05  wong
**	    Removed unsigned character hack that declared
**	    a different internal date structure for INGRES.
**	10-nov-88 (thurston)
**	    Spiffied up the look of this typedef to look more presentable.
**	02-feb-1998 (walro03)
**	    Changed dn_highday from char to i1.  Char fields are unsigned on
**	    some platforms, giving incorrect results for sql/quel interval
**	    function when result was to be a negative number of days.
**      23-apr-2001 (horda03) Bug 104518 INGCBT 348
**          Added AD_DN_VALID_DATE_STATES
**  aug-2009 (stephenb)
**  	prototype various functions that are called outside ADF
*/

typedef struct _AD_DATENTRNL
{
    char	    dn_status;		/* Status bits, as follows:
					*/
#define			AD_DN_NULL	   0x00    /* Date is the empty date */
#define			AD_DN_ABSOLUTE	   0x01    /* Absolute point in time */
#define			AD_DN_LENGTH	   0x02    /* Date is a time interval */
#define			AD_DN_YEARSPEC	   0x04    /* Year specified */
#define			AD_DN_MONTHSPEC	   0x08    /* Month specified */
#define			AD_DN_DAYSPEC	   0x10    /* Day specified */
#define			AD_DN_TIMESPEC	   0x20    /* Time specified */
#define			AD_DN_AFTER_EPOCH  0x40    /* after  02-sep-1752 */
#define			AD_DN_BEFORE_EPOCH 0x80    /* before 02-sep-1752 */

/* horda03 - note. AD_DN_AFTER_EPOCH and AD_DN_BEFORE_EPOCH (or any other new
**                 flag) should not be stored in the date's status filed as
**                 this will invalidate the use of FE tools (such as QBF) from
**                 updating DATE columns in older versions of Ingres via
**                 Ingres/Net.
**
**    Left these as valid DATE states here to ensure that existing installations
**    will not become invalid.
*/ 

#define			AD_DN_VALID_DATE_STATES \
	( AD_DN_ABSOLUTE | AD_DN_LENGTH | AD_DN_YEARSPEC | AD_DN_MONTHSPEC | \
	  AD_DN_DAYSPEC | AD_DN_TIMESPEC | AD_DN_AFTER_EPOCH | \
	  AD_DN_BEFORE_EPOCH )

    i1		    dn_highday;		/* High order bits of day
					*/
    i2		    dn_year;		/* Year (# years for intervals)
					*/
    i2		    dn_month;		/* Month (# months for intervals)
					*/
    u_i2	    dn_lowday;		/* Day (# days for intervals)
					** ***NOTE*** See also .dn_highday for
					** the `high order bits' of the day, if
					** more than will fit into an u_i2.
					*/
    i4		    dn_time;		/* Time in milliseconds from 00:00
					*/
}   AD_DATENTRNL;
typedef AD_DATENTRNL	AD_OLDDTNTRNL;


/*}
** Name: AD_ADATE	- defines the layout of an ANSI DATE value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard DATE data type as it appears in the data base.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
*/

typedef	struct _AD_ADATE
{
    i2		dn_year;		/* year component (1-9999) */
    i1		dn_month;		/* month component (1-12) */
    i1		dn_day;			/* day component (1-31) */
}   AD_ADATE;


/*}
** Name: AD_TIME	- defines the layout of an ANSI TIME value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard TIME data type as it appears in the data base. NOTE,
**	the hours and minutes components of the time are converted to seconds
**	and added to the seconds of the time to produce the seconds 
**	component (0-86400).
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
*/

typedef	struct _AD_TIME
{
    i4		dn_seconds;		/* seconds since midnight */
    i4		dn_nsecond;		/* nanoseconds since last second */
    i1		dn_tzhour;		/* timezone hour component (-12 to 14)*/
    i1		dn_tzminute;		/* timezone minute component (0-59) */
}   AD_TIME;


/*}
** Name: AD_TIMESTAMP	- defines the layout of an ANSI TIMESTAMP value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard TIMESTAMP data type as it appears in the data base.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
*/

typedef	struct _AD_TIMESTAMP
{
    i2		dn_year;		/* year component (1-9999) */
    i1		dn_month;		/* month component (1-12) */
    i1		dn_day;			/* day component (1-31) */
    i4		dn_seconds;		/* seconds since midnight */
    i4		dn_nsecond;		/* nanoseconds since last second */
    i1		dn_tzhour;		/* timezone hour component (-12 to 14)*/
    i1		dn_tzminute;		/* timezone minute component (0-59) */
}   AD_TIMESTAMP;


/*}
** Name: AD_INTYM	- defines the layout of an ANSI YEAR TO MONTH 
**	INTERVAL value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard YEAR TO MONTH INTERVAL data type as it appears in the 
**	data base.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
*/

typedef	struct _AD_INTYM
{
    i2		dn_years;		/* years component (-9999 to 9999) */
    i1		dn_months;		/* months component (0-11) */
}   AD_INTYM;


/*}
** Name: AD_INTDS	- defines the layout of an ANSI DAY TO SECOND 
**	INTERVAL value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard DAY TO SECOND INTERVAL data type as it appears in the 
**	data base.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
*/

typedef	struct _AD_INTDS
{
    i4		dn_days;		/* days component */
    i4		dn_seconds;		/* time component in seconds 
					** (hours*3600 + minutes*60 + 
					** seconds) */
    i4		dn_nseconds;		/* nanoseconds component */
}   AD_INTDS;


/*}
** Name: AD_NEWDTNTRNL	- defines the layout of date/time values for 
**	ADF processing
**
** Description:
**	This typedef describes the fields used to define the coalesced
**	structure used by ADF to manipulate all date/time values (including
**	the "old" Ingres DATE type). Because of the coalescence, the fields
**	must be big enough to hold component values from all of the date/
**	time types.
**
** History:
**	21-apr-06 (dougi)
**	    Written for the date/time enhancements.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL. Added definitions for int & float
**	    nanoseconds per second: AD_49DTE_INSPERS & AD_50DTE_NSPERS.
**	16-Mar-2008 (kiria01) b120004
**	    As part of cleanip timezone handling the internal date
**	    structure has had added the field dn_status2 to allow
**	    deferring tz adjustment.
*/

typedef	struct _AD_NEWDTNTRNL
{
    i2		dn_year;		/* year component (1-9999) */
    i2		dn_month;		/* month component (1-12) */
    i4		dn_day;			/* day component (1-31) */
    i4		dn_seconds;		/* seconds since midnight */
    i4		dn_nsecond;		/* nanoseconds since last second */
    i2		dn_tzoffset;		/* timezone offset in miniutes needed to
					**     denormalise dn_seconds
					*/
    char	dn_status;		/* status byte from old date */
    char	dn_status2;		/* extended status */
#define	AD_DN2_TZ_OFF_LCL	0x01    /* Src date was adjusted to gmt
					** using dn_tzoffset the local
					** offset */
#define	AD_DN2_NORM_PEND	0x02    /* Normalise pending */
#define AD_DN2_ADTE_TZ		0x04	/* ADTE input applied a tz shift
					** and retained the tzoffset
					*/
#define AD_DN2_NO_DATE		0x08	/* Src time has no date
					*/
#define AD_DN2_ABS_OR_INT	0x10	/* Oddly TMW and TMWO are allowed
					** to coerce to INDS & INYM
					*/
#define AD_DN2_DATE_DEFAULT	0x20	/* No date was present in literal
					** str. Used to allow legacy flag
					** setting for DTE.
					*/
#define AD_DN2_NOT_GMT		0x40	/* Info: time has been de-normalised
					** and is nolonger gmt
					*/
    DB_DT_ID	dn_dttype;		/* type code for source value */
}   AD_NEWDTNTRNL;


/*}
** Name: AD_DTUNION	- collected date forms
**
** Description:
**	This typedef defines a union type suitable for use when casting the DBV
**	data value in conjunction with the data type id.
**
** History:
**	20-Feb-2008 (kiria01) b119885
**	    Defined to ease typesafe access to the date values.
*/
typedef union _AD_DTUNION {
    AD_DATENTRNL	ing_date;
    AD_ADATE		adate;
    AD_TIME		atime;
    AD_TIMESTAMP	atimestamp;
    AD_INTYM		aintym;
    AD_INTDS		aintds;
} AD_DTUNION;

/*}
** Name: AD_AGGDATE	- Defines workspace used to compute sum/avg aggregates
**	on date datatypes.
**
** Description:
**	This typedef describes the fields used to accumulate the sum of a
**	series of dates for computing the average.
**
** History:
**	5-jan-01 (inkdo01)
**	    Written.
**	11-sep-06 (gupsh01)
**	    Added nanoseconds.
**	19-Feb-2008 (kiria01) b119943
**	    Changed tot_nsecs to i4 as no point it being a float
**	    as it can't accurately handle the the nanosecon range.
*/

typedef struct _AD_AGGDATE
{
    f8		tot_secs;		/* total of seconds */
    i4		tot_nsecs;		/* total of nano seconds */
    i4		tot_status;		/* accumulation of statuses of inputs */
    i4		tot_months;		/* total of months from year/month 
					** fields of dates */
    char	tot_empty;		/* 0 - indicates no null dates
					** 1 - indicates at least one null date */
}   AD_AGGDATE;



/*
** Macro definitions used to compare dates. Please note that we do
** NOT cast the caller's arguments to (AD_DATENTRNL *) pointers. 
** This means that the caller is REQUIRED to pass the correct types to
** these macros, or the caller will fail to compile. 
*/
#define ADDT_EQ_EPOCH(addt1)\
  (((addt1)->dn_year == 1752) &&\
   ((addt1)->dn_month == 9) &&\
   (((addt1)->dn_day > 2) && ((addt1)->dn_day < 14)))

#define ADDT_GTE_EPOCH(addt1)\
    (((addt1)->dn_year > 1752) ||\
	(((addt1)->dn_year == 1752) &&\
	 ((addt1)->dn_month > 9)) ||\
	    (((addt1)->dn_year == 1752) &&\
	     ((addt1)->dn_month == 9) &&\
	     ((addt1)->dn_day > 2)))

#define ADDT_LTE_EPOCH(addt1)\
    (((addt1)->dn_year < 1752) ||\
	(((addt1)->dn_year == 1752) &&\
	 ((addt1)->dn_month < 9)) ||\
	    (((addt1)->dn_year == 1752) &&\
	     ((addt1)->dn_month == 9) &&\
	     ((addt1)->dn_day < 14)))

#define ADDT_GT_EPOCH(addt1)\
    (((addt1)->dn_year > 1752) ||\
	(((addt1)->dn_year == 1752) &&\
	 ((addt1)->dn_month > 9)) ||\
	    (((addt1)->dn_year == 1752) &&\
	     ((addt1)->dn_month == 9) &&\
	     ((addt1)->dn_day > 13)))

#define ADDT_LT_EPOCH(addt1)\
    (((addt1)->dn_year < 1752) ||\
	(((addt1)->dn_year == 1752) &&\
	 ((addt1)->dn_month < 9)) ||\
	    (((addt1)->dn_year == 1752) &&\
	     ((addt1)->dn_month == 9) &&\
	     ((addt1)->dn_day < 2)))


/*
**  MACRO to tell how many days in a given year ... leap year calculation:
*/
#define ADU_DYSIZE_MACRO(x) (((x)%4==0 && (x)%100!=0 || (x)%400==0) ? 366 : 365)
/*
** MACRO for biggest of 3 values, used to calculate output length
** in adicalclen
*/
#define ADU_BIGGEST_MACRO(a, b, c) \
(a > b && a > c) ? a : ((b > a && b > c) ? b : c)


/*
**  Defines of other constants:
*/

#define	    ADF_DTE_LEN		sizeof(AD_DATENTRNL)	/* obvious */
#define	    ADF_ADATE_LEN	       4
#define	    ADF_TIME_LEN	      10	
#define	    ADF_TSTMP_LEN	      14
#define	    ADF_INTYM_LEN              3	
#define	    ADF_INTDS_LEN	      12	
#define	    AD_1DTE_OUTLENGTH	      25    /* string length of date output */
#define     AD_2ADTE_OUTLENGTH        10    /* string length of ADATE output */
#define     AD_3TMWO_OUTLENGTH        21    /* string length of TMWO output */
#define     AD_4TMW_OUTLENGTH         31    /* string length of TMW output */
#define     AD_5TME_OUTLENGTH         31    /* string length of TME output */
      /* Max string '24:59:59.999999999 am gmt10half' */
#define     AD_6TSWO_OUTLENGTH        39    /* string length of TMWO output */
#define     AD_7TSW_OUTLENGTH         49    /* string length of TSW output */
#define     AD_8TSTMP_OUTLENGTH       49    /* string length of TSTMP output */
      /* Max string '31-September-2007 24:59:59.999999999 am gmt10half' */
#define     AD_9INYM_OUTLENGTH        15    /* string length of INYM output */
					    /* Max string '9999 yrs 12 mos' */
#define     AD_10INDS_OUTLENGTH       45    /* string length of INDS output */
      /* Max string '3649635 days 23 hrs 59 mins 59.999999999 secs' */
#define     AD_11DTE_INTRV_OUTLENGTH  57    /* string length of an ingresdate interval */
      /* Max string '-9999 yrs -11 mos -3600000 days -19 hrs -39 mins -30 secs' */
#define     AD_11_MAX_STRING_LEN      AD_11DTE_INTRV_OUTLENGTH
                                            /* Maximum string length of
                                            ** any date type.
                                            */


#define	    AD_2DTE_MSPERSEC	    1000.0  /* # msecs in a second (as flt) */
#define	    AD_3DTE_MSPERMIN	   60000.0  /* # msecs in a minute (as flt) */
#define	    AD_4DTE_MSPERHOUR	 3600000.0  /* # msecs in an hour (as flt) */
#define	    AD_5DTE_MSPERDAY	86400000.0  /* # msecs in a day (as flt) */
#define	    AD_6DTE_IMSPERSEC	    1000L   /* # msecs in a second (as int) */
#define	    AD_7DTE_IMSPERMIN	   60000L   /* # msecs in a minute (as int) */
#define	    AD_8DTE_IMSPERHOUR	 3600000L   /* # msecs in an hour (as int) */
#define	    AD_9DTE_IMSPERDAY	86400000L   /* # msecs in a day (as int) */

#define	    AD_10DTE_ISECPERMIN	      60    /* # seconds in a minute (as int) */
#define	    AD_11DTE_IMINPERHOUR      60    /* # minutes in an hour (as int) */

#define	    AD_12DTE_DAYPERWEEK	       7.0  /* # days in a week (as flt) */
#define	    AD_13DTE_DAYPERMONTH      30.436875	/* # days per month (as flt) */
#define	    AD_14DTE_DAYPERYEAR	     365.2425   /* # days in a year (as flt) */
#define	    AD_15DTE_IDAYPERWEEK       7    /* # days in a week (as int) */
#define	    AD_16DTE_IDAYPERYEAR     365    /* # days in a year (as int) */

#define	    AD_17DTE_MONPERQUAR        3.0  /* # months in a quarter (as flt) */
#define	    AD_18DTE_IMONPERQUAR       3    /* # months in a quarter (as int) */
#define	    AD_19DTE_QUARPERYEAR       4.0  /* # quarters in a year */

#define	    AD_20DTE_WEEKLENGTH	       7    /* # days in a week */

#define	    AD_21DTE_LOWDAYSIZE	      16    /* # bits in the "low" day field */
#define	    AD_22DTE_LOWDAYMASK	  0xFFFF    /* Mask to use for "low" day */

#define	    AD_23DTE_MIN_ABS_YR	       1    /* Min yr for absolute dates */
#define	    AD_24DTE_MAX_ABS_YR	    9999    /* Max yr for absolute dates */
#define	    AD_25DTE_MIN_INT_YR	   (-9999)  /* Min # yrs in time interval */
#define	    AD_26DTE_MAX_INT_YR	     9999   /* Max # yrs in time interval */
#define	    AD_27DTE_MIN_INT_DAY  (-3652047)/* Min # days in time interval */
#define	    AD_28DTE_MAX_INT_DAY    3652047 /* Max # days in time interval */

#define	    AD_29DTE_NSPERMS      1000000   /* # nsecs per msec */
#define     AD_30DTE_MIN_SEC            0   /* Min # seconds */
#define     AD_31DTE_MAX_SEC           60   /* Max # seconds */
#define     AD_32DTE_MIN_NSEC           0   /* Min # nsecs in time */
#define     AD_33DTE_MAX_NSEC   999999999   /* Max # nsecs in time */
#define     AD_34DTE_MIN_TZ_HR       (-12)   /* Min # hours in time zone */
#define     AD_35DTE_MAX_TZ_HR         14   /* Max # hours in time zone */
#define     AD_36DTE_MIN_TZ_MN          0   /* Min # minutes in time zone */
#define     AD_37DTE_MAX_TZ_MN         59   /* Max # minutes in time zone */
#define	    AD_38DTE_NSPERMS	 1000000.0  /* # nsecs in a millisecond */

#define     AD_39DTE_ISECPERHOUR     3600   /* Max number of seconds in an hour */

#define     AD_40DTE_SECPERDAY     	86400.00   /* Max number of seconds in a day in float form, */
#define     AD_41DTE_ISECPERDAY    	86400   /* Max number of seconds in a day */
#define     AD_42DTE_INSUPPERLIMIT	1000000000   /* Max number of nseconds in a day */
#define     AD_43DTE_SECPERMIN     	60.00   /* Max number of seconds in a day in float form, */
#define     AD_44DTE_SECPERHOUR     	3600.00   /* Max number of seconds in a day in float form, */
#define	    AD_45DTE_MAXSPREC		9	  /* max seconds precision */
#define	    AD_47DTE_MIN_TZ_MN          (-779)    /* min timezone mins */
#define	    AD_48DTE_MAX_TZ_MN  	840       /* max timezone mins */
#define     AD_49DTE_INSPERS	1000000000	  /* Nanosecs in a second */
#define     AD_50DTE_NSPERS		1.0e9	  /* Nanosecs in a second */

/*
**  Forward and/or External function references.
*/

/* Normalizes an absolute date. */
FUNC_EXTERN VOID adu_1normldate(AD_NEWDTNTRNL  *d);

/* Normalizes a internal interval date. */
FUNC_EXTERN i4 adu_2normldint(AD_NEWDTNTRNL  *d);

/* Check create length and precision of date. */
FUNC_EXTERN bool adu_dlenchk(i4  *length, i4  prec);

/* Compute absolute number of days since epoch. */
FUNC_EXTERN i4 adu_3day_of_date(AD_NEWDTNTRNL  *d);

/* Compute the number of seconds since epoch */
FUNC_EXTERN i4 adu_5time_of_date(AD_NEWDTNTRNL  *d);

FUNC_EXTERN DB_STATUS adu_14strtodate(); /* Routine to coerce a string data
                                            ** value into a date data value.
                                            */
FUNC_EXTERN DB_STATUS adu_21ansi_strtodt(); /* Routine to coerce a 
					    ** ANSI string data
                                            ** value into a date data value.
                                            */
FUNC_EXTERN DB_STATUS adu_2datetodate(); /* This routine converts a date 
                                            ** value into a date value.
                                            ** This is used for copying a date
                                            ** data value into a tuple buffer.
                                            */
FUNC_EXTERN DB_STATUS adu_6datetostr();  /* Routine to coerce a date datatype
                                            ** into a string datatype.
                                            */
/*
** Time zone access macros for AD_TIME & AD_TIMESTAMP
**
** Although originally stored as dn_tzhour and dn_tzminute, time zone
** is now stored in database as a biased i2 from these fields concatenated.
**
** Concatenating the two fields into an i2, the legacy values must all be
** in the range -14*256 and +12*256 based on the potential stored hours.
** As long as we bias the new form so that its range does not overlap,
** we can have both side by side.
** _AD_TZ_BIAS, the bias value, we choose as 256*60 to allow for larger
** timezone offsets but mostly so that the HEX for GMT will be 3C00.
** -128..-25 is unused, -24..24 for legacy 24..60..96 is the new range.
** _AD_TZ_BIAS marks the GMT point.
**
** History:
**	26-Feb-2008 (kiria01) b120004
**	    Created to support normalising of timezone supporting datatypes
*/
#define _AD_MIN_LGY_HR (-24)
#define _AD_MAX_LGY_HR (24)
#define _AD_TZ_BIAS_HR (60) /* Must be multiple of 60 for AD_TZ_MINUTES */
#define _AD_TZ_BIAS (_AD_TZ_BIAS_HR<<8)
/*
** Thus there are now three forms that time zone is stored in:
** - internal form - an i2 offset in minutes
** - storage form - a biased i2 offset in minutes (always negative)
** - legacy storage form - two i1 fields
*/
/* _AD_TZ - Method of concatenation == short on bigendian */
#define _AD_TZ(d) (((i2)(d)->dn_tzhour<<8)|((i2)(d)->dn_tzminute&0xFF))

/* _AD_TZ_LEGACY - convert legacy to internal TZ offset in mins */
#define _AD_TZ_LEGACY(d) ((d)->dn_tzhour*(i2)60+(d)->dn_tzminute)

/* AD_TZ_ISLEGACY test for legacy form */
#define AD_TZ_ISLEGACY(d) ((d)->dn_tzhour<_AD_MAX_LGY_HR)

/* AD_TZ_OFFSET extract the tz offset in seconds from storage form */
#define AD_TZ_OFFSET(d) ((AD_TZ_ISLEGACY(d)\
			    ?_AD_TZ_LEGACY(d)\
			    :_AD_TZ(d)-_AD_TZ_BIAS)*AD_10DTE_ISECPERMIN)

/* AD_TZ_SET assign offset in secs to storage form */
#define AD_TZ_SET(d, val) ((d)->dn_tzhour=((val/AD_10DTE_ISECPERMIN+_AD_TZ_BIAS)>>8)&0xFF),\
			((d)->dn_tzminute=(val/AD_10DTE_ISECPERMIN+_AD_TZ_BIAS)&0xFF)

/* AD_TZ_MINUTES extract minutes from storage form */
#define AD_TZ_MINUTES(d) (AD_TZ_ISLEGACY(d)\
			    ?(d)->dn_tzminute\
			    :((d)->dn_tzhour>=_AD_TZ_BIAS_HR\
				?_AD_TZ(d)-_AD_TZ_BIAS\
				:_AD_TZ_BIAS-_AD_TZ(d))%60)

/* AD_TZ_HOURS extract hours from storage form */
#define AD_TZ_HOURS(d) (AD_TZ_ISLEGACY(d)\
			    ?(d)->dn_tzhour\
			    :((_AD_TZ(d)-_AD_TZ_BIAS)/60))

/* AD_TZ_OFFSETNEW extract the tz offset in seconds from AD_NEWDTNTRNL */
#define AD_TZ_OFFSETNEW(d) ((d)->dn_tzoffset*AD_10DTE_ISECPERMIN)

/* AD_TZ_SETNEW assign offset in secs to AD_NEWDTNTRNL */
#define AD_TZ_SETNEW(d, val) ((d)->dn_tzoffset=(val)/AD_10DTE_ISECPERMIN)

/* AD_TZ_SIGNNEW extract sign of offset from AD_NEWDTNTRNL */
#define AD_TZ_SIGNNEW(d) ((d)->dn_tzoffset<0?-1:1)

/* AD_TZ_HOURSNEW extract hours from AD_NEWDTNTRNL */
#define AD_TZ_HOURSNEW(d) (abs((d)->dn_tzoffset)/60)

/* AD_TZ_MINUTESNEW extract minutes from AD_NEWDTNTRNL */
#define AD_TZ_MINUTESNEW(d) ((d)->dn_tzoffset>=0\
			    ?(d)->dn_tzoffset-((d)->dn_tzoffset/60)*60\
			    :-(d)->dn_tzoffset-((d)->dn_tzoffset/-60)*60)

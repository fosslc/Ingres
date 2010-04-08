#include <adudate.h>
/*
**  Copyright (c) 2004 Ingres Corporation
**
**  DATES.H -- defines the internal representation for dates in the
**			INGRES date abstract data type
**
**	Note: all times are stored as GMT.
**
**	History:
**	
**	03-dec-85 (seputis) - added constant definitions
**	11/20/89 (elein) integrated 2/27/90
**		- Added SECS_ definitions formerly in utils!src!fmt!format.h
**		- Including adudate.h -- THE DEFINITIVE data definition
**		  EVERYTHING in the FE MUST be in terms of values in adudate.h
**		- Changed DN_ constants to be in terms of AD constants
**		- Changed WEEKLENGHT, LOWDAY constants to be in terms of AD
**		  constants
**		- removed unused DATENTRNL definition and redefined it
**		  as AD_DATENTRNL
**		- removed unused MSPER definitions and dysize macro.
**		- Months per year is still local--there is no equivalent
**		  in adudate.h
**		- removed some RCS history
**	11/08/92 (dkh) - Fixed acc trigraph complaint.
**	11/09/06 (gupsh01) - Added ANSI date/time types.
*/

# define	DATEOUTLENGTH		25    /* string length of DATE */
# define	ANSIDATE_OUTLENGTH	17    /* string length of ADATE */
# define	ANSITMWO_OUTLENGTH	21    /* string length of TMWO */
# define	ANSITMW_OUTLENGTH	31    /* string length of TMW */
# define	ANSITME_OUTLENGTH       31    /* string length of TME */
# define	ANSITSWO_OUTLENGTH	39    /* string length of TMWO */
# define	ANSITSW_OUTLENGTH	49    /* string length of TSW */
# define	ANSITSTMP_OUTLENGTH	49    /* string length of TSTMP */
# define	ANSIINYM_OUTLENGTH	15    /* string length of INYM */
# define	ANSIINDS_OUTLENGTH	45    /* string length of INDS */


# define	DN_NULL		AD_DN_NULL	/* Null date field */
# define	DN_ABSOLUTE	AD_DN_ABSOLUTE	/* Absolute point in time */
# define	DN_LENGTH	AD_DN_LENGTH	/* Length of time */
# define	DN_YEARSPEC	AD_DN_YEARSPEC	/* Year specified */
# define	DN_MONTHSPEC	AD_DN_MONTHSPEC	/* Month specified */
# define	DN_DAYSPEC	AD_DN_DAYSPEC	/* Day specified */
# define	DN_TIMESPEC	AD_DN_TIMESPEC	/* Time specified */

# define	SECS_MIN	AD_10DTE_ISECPERMIN			/* # secs in a min */
# define	SECS_HR		(AD_8DTE_IMSPERHOUR/AD_6DTE_IMSPERSEC)	/* # secs in an hour */
# define	SECS_DAY	(AD_9DTE_IMSPERDAY/AD_6DTE_IMSPERSEC)	/* # secs in an day */
# define	SECS_MON	(AD_13DTE_DAYPERMONTH*SECS_DAY)		/* # secs in an month */
# define	SECS_YR		(AD_14DTE_DAYPERYEAR*SECS_DAY)		/* # secs in an year */
# define	MONS_YR		12.0					/* # months in a year */
# define	WEEKLENGTH	AD_20DTE_WEEKLENGTH			/* # days in a week */

# define	DATENTRNL	AD_DATENTRNL			/* The Real date structure */
# define	DATE_LENGTH	sizeof (AD_DATENTRNL)
# define	ADATE_LENGTH	4	
# define	ATIME_LENGTH	10	
# define	ATSTMP_LENGTH	14
# define	AINTYM_LENGTH   3
# define	AINTDS_LENGTH	12	
# define	LOWDAYSIZE		AD_21DTE_LOWDAYSIZE
# define	LOWDAYMASK		AD_22DTE_LOWDAYMASK

/*
** Starting with 3.0, we use the same date structure everywhere,
** even if we have unsigned characters.  This means we need to be
** careful with dn_status and dn_highday, which need to hold i1's.
** (should they be declared as i1's?)
** (elein) definition removed: the official one is in adudate.h
*/

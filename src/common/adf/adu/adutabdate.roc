/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    "adu2date.h"
#include    "iicommon.h"

/**
**
**  Name: ADUTABDATE.ROC - defines read-only data associated with dates.
**
**  Description:
**        This file defines and initializes read-only global data used in the
**	interpretation of dates.  This is put in a ROC, read-only-C, file so
**	that it can be loaded as a shared text segment on Unix.
**
**  History:    
**      03-dec-85 (seputis) added quarters and weeks
**      25-may-86   (ericj)    
**          Converter for Jupiter.
**	26-may-86 (ericj)
**	    Changed the four read-only templates supporting dates templates
**	    so that each one would have all of the possible date formats.
**	    These will be used directly to interpret dates instead of the
**	    global updateble table, Datetmplt[], which has been deleted.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	30-jan-87 (thurston)
**	    Changed the spec letter for day from 'G' to 'D'.
**	21-jan-88 (thurston)
**	    Added the `1988_01_21' date format to all of the international
**	    date format tables.  This is produced by the INGRES date_gmt()
**	    function (recent change; used to use `1988.01.21'), and is depended
**	    on by the standard catalog interface.  I also spiffed up the
**	    comments a bit, where I could.
**	06-nov-88 (thurston)
**	    Temporarily commented out the READONLYs on various tables.
**	15-jun-89 (jrb)
**	    Made all tables GLOBALCONSTDEF instead of just GLOBALDEF (this is
**	    for MVS).
**      06-may-92 (stevet)
**          Added Adu_5ymd[], Adu_6mdy[] and Adu_7dmy[] tables for
**          YMD, MDY and DMY new date formats repectively.
**	13-mar-1996 (angusm)
**		bug 48458: MULTINATIONAL4 added:  as MULTINATIONAL with
**		4-digit year.
**	09-feb-1998 (kinte01)
**	   Made adu_dt_size GLOBALCONSTDEF instead of GLOBALDEF READONLY
**	   This corrects problem with DEC C compiler complaints regarding
**	   the incompatibility between the definition here and the one in
**	   adu2date.h.
**	07-Jun-1999 (shero03)
**	    Add ISO-6801 week in dextract
**	24-Jun-1999 (shero03)
**	    Add ISO4.
**	15-jan-1999 (kitch01)
**	   Bug 94791. Added Canadian timezones and GMT zones currently 
**	   defined in iiworld.tz, plus gmt-3.5. For those zones that have
**	   half hours the convention is to omit the decimal point, thus
**	   GMT-3.5 becomes GMT-35. Also changed the values to be the 
**	   offset from GMT in minutes rather than hours, again to facilitate
**	   zones with half hours.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	1-Jul-2004 (schka24)
**	    1-Jan-0001 (day 1 in our reckoning) was Saturday, not Friday.
**	    Fix the weekday table now that day numbers are being properly
**	    computed.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
**	31-Jul-2006 (gupsh01)
**	    Added new format adu_10ansi.
**	19-oct-2006 (dougi)
**	    Added microsecond and nanosecond.
**	02-feb-2007 (gupsh01)
**	    Added timezone_hour and timezone_minute.
**	18-Oct-2007 (kibro01) b119318
**	    Use a common function in adu to determine valid date formats
**	01-Dec-2008 (kiria01) SIR 121297
**	    Add extra flags to aid date processing in combined parser.
**	11-Dec-2008 (kiria01) SIR 121297
**	    Don't need to check ymd dates of n-n-n format as they are in
**	    ANSI form anyway.
**	28-Jan-2009 (kiria01) SIR 121297
**	    Reinstate the check of ymd dates of n-n-n format as the check
**	    does still need making.
**	28-Feb-2009 (kiria01) b121728
**	    Corrected comments relating to ISO & ISO4 date formats.
**	12-Mar-2010 (kschendel) SIR 123275
**	    Move all the date format table stuff to a perfect-hash lookup.
**	    The actual data is now private to adudates.c.
**	    Similarly, move all month names (and now and today) to another
**	    perfect-hash lookup, also in adudates.c.
**/

/* Jam hints
**
LIBRARY = IMPADFLIBDATA
**
*/


/*
** Definition of all global variables owned by this file.
*/

/*
**  Define `Adu_weekdays[]', the integer code as used by the day-of-week
**  function, dow(), and the capitalized abbreviation for a weekday.
*/

GLOBALCONSTDEF   ADU_WEEKDAYS Adu_weekdays[]=
{
    0,	    "Fri",
    1,	    "Sat",
    2,	    "Sun",
    3,	    "Mon",
    4,	    "Tue",
    5,	    "Wed",
    6,	    "Thu"
};


/*
**  Define `Adu_datename[]', used to define valid parts of an INGRES date.
**  This also defines values associated with these parts where appropriate.
**  
**  NOTE: this table is binary-searched, so all entries MUST be alphabetic.
**
**  Any date-input part that is not exclusively a time part is excluded.
**  In other words, anything that can be part of a date without time is
**  excluded.  That means month names, today, and now are excluded.
**  What's left are timezone names, am/pm, and date function specifiers
**  for e.g. date_part, etc.
**
**  A d_value of exactly -1 is used to flag something that can only
**  occur in an Ingres-style interval date.
*/

GLOBALCONSTDEF ADU_DATENAME Adu_datename[] =
{
    "adt",      't',    -180,
    "am",       'a',    0,
    "ast",      't',	-240,

    "bst",	't',	60,

    "cdt",      't',    -300,
    "cst",      't',    -360,

    "day",      'D',    -1,
    "days",     'D',    -1,

    "edt",      't',    -240,
    "est",      't',    -300,

    "gmt",      't',    0,
    "gmt-1",    't',	-60,
    "gmt-10",	't',	-600,
    "gmt-11",	't',	-660,
    "gmt-12",	't',	-720,
    "gmt-2",	't',	-120,
    "gmt-3",	't',	-180,
    "gmt-35",	't',	-210,
    "gmt-4",	't',	-240,
    "gmt-5",	't',	-300,
    "gmt-6",	't',	-360,
    "gmt-7",	't',	-420,
    "gmt-8",	't',	-480,
    "gmt-9",	't',	-540,
    "gmt1",	't',	60,
    "gmt10",	't',	600,
    "gmt105",	't',	630,
    "gmt11",	't',	660,
    "gmt12",	't',	720,
    "gmt13",	't',	780,
    "gmt2",	't',	120,
    "gmt3",	't',	180,
    "gmt35",	't',	210,
    "gmt4",	't',	240,
    "gmt5",	't',	300,
    "gmt55",	't',	330,
    "gmt6",	't',	360,
    "gmt7",	't',	420,
    "gmt8",	't',	480,
    "gmt9",	't',	540,
    "gmt95",	't',	570,


    "hour",     'H',    -1,
    "hours",    'H',    -1,
    "hr",       'H',    -1,
    "hrs",      'H',    -1,

    "iso-week", 'w',    -1,
    "iso-wk",   'w',    -1,
    "iso-wks",  'w',    -1,

    "mdt",      't',    -360,
    "microsecond", 'x', -1,
    "min",      'I',    -1,
    "mins",     'I',    -1,
    "minute",   'I',    -1,
    "minutes",  'I',    -1,
    "mo",       'M',    -1,
    "month",    'M',    -1,
    "months",   'M',    -1,
    "mos",      'M',    -1,
    "mst",      't',    -420,

    "nanosecond", 'X',  -1,
    "ndt",	't',	-150,
    "nst",	't',	-210,

    "pdt",      't',    -420,
    "pm",       'a',    12,
    "pst",      't',    -480,

    "qtr",      'Q',    -1,
    "qtrs",     'Q',    -1,  
    "quarter",  'Q',    -1,
    "quarters", 'Q',    -1,

    "sec",      'S',    -1,
    "second",   'S',    -1,
    "seconds",  'S',    -1,
    "secs",     'S',    -1,

    "timezone_hour",    'R',    -1,
    "timezone_minute",    'T',    -1,

    "week",     'W',    -1,
    "weeks",    'W',    -1,
    "wk",       'W',    -1,
    "wks",      'W',    -1,

    "ydt",	't',	-420,
    "year",     'Y',    -1,
    "years",    'Y',    -1,
    "yr",       'Y',    -1,
    "yrs",      'Y',    -1,
    "yst",	't',	-480,
};

GLOBALCONSTDEF i4  Adu_dt_size = 
		(sizeof(Adu_datename) / sizeof(ADU_DATENAME));


/* Translation from date_format name to internal format code. */

GLOBALCONSTDEF   DATEFMTLIST Adu_date_format_list[] = 
{
	{	"us",			DB_US_DFMT	},
	{	"multinational",	DB_MLTI_DFMT	},
	{	"finland",		DB_FIN_DFMT	},
	{	"sweden",		DB_FIN_DFMT	},
	{	"iso",			DB_ISO_DFMT	},
	{	"german",		DB_GERM_DFMT	},
	{	"ymd",			DB_YMD_DFMT	},
	{	"mdy",			DB_MDY_DFMT	},
	{	"dmy",			DB_DMY_DFMT	},
	{	"multinational4",	DB_MLT4_DFMT	},
	{	"iso4",			DB_ISO4_DFMT	},
	{	"ansi",			DB_ANSI_DFMT	},
	{	NULL,			0		}
};


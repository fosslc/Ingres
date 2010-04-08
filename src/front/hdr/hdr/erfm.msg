/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erfm.msg -	Message file for class FM
**
** Description:
**	This file defines the messages for class FM.
**
** History:
**	 7-aug-1987 (grant)
**		Created.
**	08/31/87 (dkh) - Added E_FM600D_BAD_LEN message id.
**	12-may-88 (sylviap)
**		Added E_FM2016_INVALID_TEMPLATE.
**	02-oct-89 (sylviap)
**		Added E_FM2017_NO_JUSTIF and E_FM2018_EXPECT_ZERO for the
**		Q format.
*/

# define	_FM_CLASS	33
/*
**	dateutil.c
*/
/*
**	The following strings are for the specification and formatting of
**	dates using the date template.  For example, a date format
**	specification of d"SUNDAY, Feb. 3, 1901 at 4:05 pm" would format
**	a date value of "14-aug-1987 10:23:07" as
**	"FRIDAY, Aug. 14, 1987 at 10:23 am".
**
**	All strings must be in lower-case.
*/
/*
**	The 3-letter abbreviations for months of the year follow.
*/
F_FM0001_jan	"jan"
F_FM0002_feb	"feb"
F_FM0003_mar	"mar"
F_FM0004_apr	"apr"
F_FM0005_may	"may"
F_FM0006_jun	"jun"
F_FM0007_jul	"jul"
F_FM0008_aug	"aug"
F_FM0009_sep	"sep"
F_FM000A_oct	"oct"
F_FM000B_nov	"nov"
F_FM000C_dec	"dec"
/*
**	The full-length names of the months of the year follow.
*/
F_FM000D_january	"january"
F_FM000E_february	"february"
F_FM000F_march	"march"
F_FM0010_april	"april"
F_FM0011_may	"may"
F_FM0012_june	"june"
F_FM0013_july	"july"
F_FM0014_august	"august"
F_FM0015_september	"september"
F_FM0016_october	"october"
F_FM0017_november	"november"
F_FM0018_december	"december"
/*
**	The ordinal suffixes for the numbers 0 through 99 follow.
*/
/*
**	The ordinal suffix for 0 is "th" as in "0th" or "zeroth".
*/
F_FM0019_th	"th"
/*
**	The ordinal suffix for 1 is "st" as in "1st" or "first".
*/
F_FM001A_st	"st"
/*
**	The ordinal suffix for 2 is "nd" as in "2nd" or "second".
*/
F_FM001B_nd	"nd"
/*
**	The ordinal suffix for 3 is "rd" as in "3rd" or "third".
*/
F_FM001C_rd	"rd"
/*
**	The ordinal suffix for 4 is "th" as in "4th" or "fourth".
*/
F_FM001D_th	"th"
/*
**	the ordinal suffix for 5 is "th" as in "5th" or "fifth".
*/
F_FM001E_th	"th"
/*
**	6th or sixth
*/
F_FM001F_th	"th"
/*
**	7th or seventh
*/
F_FM0020_th	"th"
/*
**	eighth
*/
F_FM0021_th	"th"
/*
**	ninth
*/
F_FM0022_th	"th"
/*
**	tenth
*/
F_FM0023_th	"th"
/*
**	eleventh
*/
F_FM0024_th	"th"
/*
**	twelfth
*/
F_FM0025_th	"th"
/*
**	thirteenth
*/
F_FM0026_th	"th"
/*
**	fourteenth
*/
F_FM0027_th	"th"
/*
**	fifteenth
*/
F_FM0028_th	"th"
/*
**	sixteenth
*/
F_FM0029_th	"th"
/*
**	seventeenth
*/
F_FM002A_th	"th"
/*
**	eighteenth
*/
F_FM002B_th	"th"
/*
**	nineteenth
*/
F_FM002C_th	"th"
/*
**	twentieth
*/
F_FM002D_th	"th"
/*
**	twenty-first
*/
F_FM002E_st	"st"
/*
**	twenty-second
*/
F_FM002F_nd	"nd"
/*
**	twenty-third
*/
F_FM0030_rd	"rd"
/*
**	twenty-fourth
*/
F_FM0031_th	"th"
/*
**	twenty-fifth
*/
F_FM0032_th	"th"
/*
**	twenty-sixth
*/
F_FM0033_th	"th"
/*
**	twenty-seventh
*/
F_FM0034_th	"th"
/*
**	twenty-eighth
*/
F_FM0035_th	"th"
/*
**	twenty-ninth
*/
F_FM0036_th	"th"
/*
**	thirtieth
*/
F_FM0037_th	"th"
F_FM0038_st	"st"
F_FM0039_nd	"nd"
F_FM003A_rd	"rd"
F_FM003B_th	"th"
F_FM003C_th	"th"
F_FM003D_th	"th"
F_FM003E_th	"th"
F_FM003F_th	"th"
F_FM0040_th	"th"
/*
**	fortieth
*/
F_FM0041_th	"th"
F_FM0042_st	"st"
F_FM0043_nd	"nd"
F_FM0044_rd	"rd"
F_FM0045_th	"th"
F_FM0046_th	"th"
F_FM0047_th	"th"
F_FM0048_th	"th"
F_FM0049_th	"th"
F_FM004A_th	"th"
/*
**	fiftieth
*/
F_FM004B_th	"th"
F_FM004C_st	"st"
F_FM004D_nd	"nd"
F_FM004E_rd	"rd"
F_FM004F_th	"th"
F_FM0050_th	"th"
F_FM0051_th	"th"
F_FM0052_th	"th"
F_FM0053_th	"th"
F_FM0054_th	"th"
/*
**	sixtieth
*/
F_FM0055_th	"th"
F_FM0056_st	"st"
F_FM0057_nd	"nd"
F_FM0058_rd	"rd"
F_FM0059_th	"th"
F_FM005A_th	"th"
F_FM005B_th	"th"
F_FM005C_th	"th"
F_FM005D_th	"th"
F_FM005E_th	"th"
/*
**	seventieth
*/
F_FM005F_th	"th"
F_FM0060_st	"st"
F_FM0061_nd	"nd"
F_FM0062_rd	"rd"
F_FM0063_th	"th"
F_FM0064_th	"th"
F_FM0065_th	"th"
F_FM0066_th	"th"
F_FM0067_th	"th"
F_FM0068_th	"th"
/*
**	eightieth
*/
F_FM0069_th	"th"
F_FM006A_st	"st"
F_FM006B_nd	"nd"
F_FM006C_rd	"rd"
F_FM006D_th	"th"
F_FM006E_th	"th"
F_FM006F_th	"th"
F_FM0070_th	"th"
F_FM0071_th	"th"
F_FM0072_th	"th"
/*
**	ninetieth
*/
F_FM0073_th	"th"
F_FM0074_st	"st"
F_FM0075_nd	"nd"
F_FM0076_rd	"rd"
F_FM0077_th	"th"
F_FM0078_th	"th"
F_FM0079_th	"th"
F_FM007A_th	"th"
F_FM007B_th	"th"
F_FM007C_th	"th"
/*
**	The 2-letter abbreviations for the days of the week follow,
**	starting with Thursday.
*/
F_FM007D_th	"th"
F_FM007E_fr	"fr"
F_FM007F_sa	"sa"
F_FM0080_su	"su"
F_FM0081_mo	"mo"
F_FM0082_tu	"tu"
F_FM0083_we	"we"
/*
**	The 3-letter abbreviations for the days of the week follow,
**	starting with Thursday.
*/
F_FM0084_thu	"thu"
F_FM0085_fri	"fri"
F_FM0086_sat	"sat"
F_FM0087_sun	"sun"
F_FM0088_mon	"mon"
F_FM0089_tue	"tue"
F_FM008A_wed	"wed"
/*
**	The full-length names for the days of the week follow.
*/
F_FM008B_thursday	"thursday"
F_FM008C_friday	"friday"
F_FM008D_saturday	"saturday"
F_FM008E_sunday	"sunday"
F_FM008F_monday	"monday"
F_FM0090_tuesday	"tuesday"
F_FM0091_wednesday	"wednesday"
/*
**	The initial letter of "ante meridian" follows.  For example,
**	this follows the time of day as in 10:30 a.m.
*/
F_FM0092_a	"a"
/*
**	This is the initial letter of "post meridian".  For example,
**	this follows the time of day as in 4:15 p.m.
*/
F_FM0093_p	"p"
/*
**	This is an abbreviation for ante meridian as in 10:30 am.
*/
F_FM0094_am	"am"
/*
**	This is the abbreviation for post meridian as in 4:15 pm.
*/
F_FM0095_pm	"pm"
/*
**	The singular names of time interval units follow.  For example,
**	1 year 1 month 1 day 1 hour 1 minute 1 second.
*/
F_FM0096_year	"year"
F_FM0097_month	"month"
F_FM0098_day	"day"
F_FM0099_hour	"hour"
F_FM009A_minute	"minute"
F_FM009B_second	"second"
/*
**	The plural names for time interval units follows.  For example,
**	5 years 3 months 8 days 15 hours 22 minutes 9 seconds.
*/
F_FM009C_years	"years"
F_FM009D_months	"months"
F_FM009E_days	"days"
F_FM009F_hours	"hours"
F_FM00A0_minutes	"minutes"
F_FM00A1_seconds	"seconds"
/*
**	ffmtdate.c
*/
/*
**	ffmtdate.c
*/
/*
**	ffmtstr.c
*/
/*
**	fmareasz.c
*/
/*
**	fmdeffmt.c
*/
/*
**	fmsetfmt.c
*/
/*
**	fmtutil.c
*/
/*
**	fmtutil.c
*/
/*
**	strutil.c
*/
/*
**	fsetfmt.c
*/
/*
**	findate.c
*/
/*
**	finnt.c
*/
/*
**	fsetfmt.c
*/
/*
**	The following string is used as the 2nd argument to the FMT
**	error E_FM200D and as the only argument to FMT error E_FM200B.
*/
S_FM0001_day_of_week	"day of week"


/*
**	Error messages
*/

E_FM0000_OK
"FMT successful return status."
E_FM1000_FORMAT_OVERFLOW
"Format is not wide enough to accommodate entire number.\n
Either the number is too large or the width of the format should be increased."
E_FM2000_EXPECTED_CHAR_INSTEAD
"Expected character '%0c' instead of character '%1c'."
E_FM2001_EXPECTED_NUMBER
"Expected a number starting at '%0c'."
E_FM2002_EXPECTED_WORD
"Expected the word '%0c' in the date entered."
E_FM2003_WRONG_WORD
"Wrong word '%0c' used in the date entered."
E_FM2004_DATE_COMP_NOT_ENTERED
"The %0c component of the date was not entered."
E_FM2005_DAY_OF_YEAR_TOO_LARGE
"Day %0d of the year %1d is too large in the date entered."
E_FM2006_BAD_CHAR
"Illegal character '%0c' found."
E_FM2007_BAD_FORMAT_CHAR
"Illegal character '%0c' specified for format type."
E_FM2008_NO_ESCAPED_CHAR
"No character follows the backslash ('\') character in the format template."
E_FM2009_CHAR_SHOULD_FOLLOW
"Character '%0c' must follow character '%1c' in the numeric template.\n
Either 'db' or 'cr' may be specified in a numeric template format
specification."
E_FM200A_FORMAT_TOO_LONG
"Width of format template, %0d, exceeds maximum allowed (255)."
E_FM200B_COMP_ALREADY_SPEC
"The %0c component specified more than once in the date template."
E_FM200C_ABS_COMP_SPEC_FOR_INT
"Absolute date component '%0c' cannot be specified in a time interval format."
E_FM200D_WRONG_COMP_VALUE_SPEC
"Wrong value '%0c' specified for the %1c component.\n
February specifies the month and Sunday specifies the day of the week
in a date template format specification."
E_FM200E_WRONG_ORD_VALUE
"Wrong ordinal suffix '%0c' follows the number %1c in the date template.\n
The ordinal suffix 'st' should follow 1, 'nd' should follow 2, 'rd' should
follow 3, and 'th' should follow 4, 5, and 6."
E_FM200F_ORD_MUST_FOLLOW_NUM
"Ordinal suffix '%0c' must follow a number."
E_FM2010_PM_AND_24_HR_SPEC
"PM and 24-hour time cannot be specified together.\n
Using '16' for the hour instead of '4' cannot be specified with 'pm' in a
date template format specification."
E_FM2011_I_FMT_CANT_HAVE_PREC
"The integer format 'I' cannot have numeric precision.\n
The integer format should be specified as Iw, NOT Iw.p, where w and p are
numbers for the width and precision of the format, respectively."
E_FM2012_PREC_GTR_THAN_WIDTH
"The format precision, %0d, must be at least 2 less than the format width,
which is %1d.  This is to allow room for the decimal point and a sign."
E_FM2013_EXPECTED_WORD_INSTEAD
"Expected word '%0c' instead of word '%1c' in date entered."
E_FM2014_JUSTIF_ALREADY_SPEC
"Format justification ('-', '*' or '+') specified more than once."
E_FM2015_NO_DATE_COMP_SPEC
"No date component was specified in the date template format specification.\n
The date components are: 1 for year, 2 or February for month, 3 for day,
4 or 16 for hour, 5 for minute, 6 for second, and Sunday for the day of week."
E_FM2016_INVALID_TEMPLATE
"'%0c' is an invalid template."
E_FM2017_NO_JUSTIF
"The Q format is used for non-printable character sequences.\n
You may not justify non-printable characters.\n
The correct syntax is 'Q0' or 'q0'."
E_FM2018_EXPECT_ZERO
"A zero, '0', was expected.\n
The correct syntax is 'Q0' or 'q0'."

/* f_stparse */
E_FM3000_MASKSIZE
"The string template given would create a mask larger than %0d characters.
This is not allowed."
E_FM3001_BAD_USER_CLASS
"The character '%0c' cannot be used as a user defined character class.
Only the letters 'i', 'j', 'k', 'l', 'm' can be used for this purpose."
E_FM3002_NO_CC_TERM
"There is an unterminated character class definition in the template.
All character class definitions must be terminated with a ']' character."
E_FM3003_DUP_UPR
"A character class definition in the template has multiple '/U' uppercase
specifications. Only one such specification is allowed per character class."
E_FM3004_DUP_LWR
"A character class definition in the template has multiple '/L' lowercase
specifications. Only one such specification is allowed per character class."
E_FM3005_BOTH_UPR_LWR
"A character class definition in the template has both a '/U' uppercase,
and a '/L' lowercase specification. Only one such specification is allowed
per character class."
E_FM3006_DUP_DEF
"A character class definition in the template has multiple '//x' default 
specifications. Only one such specification is allowed per character class."
E_FM3007_BAD_DEF_CHAR
"A character class definition in the template specifies a control or
double byte character as the default character. Control and double byte
characters cannot be specified as the default character."
E_FM3008_BAD_SLASH_CHAR
"A character class definition in the template contains a '/' specifcation
but an illegal character follows the '/'. The only characters that can follow
the '/' are: 'U' (for uppercase) 'L' (for lowercase) and '/'
(default character)."
E_FM3009_CTL_CHAR
"A character class definition in the template contains a control character.
Control characters are not allowed in character class definitions."
E_FM300A_MUST_ESCAPE
"A character class definition in the template contains an illegal character.
Any character other than alphanumeric or space must be escaped with a '\\'
character."
E_FM300B_BAD_RANGE
"A character class definition in the template contains an illegal range
specification (such as [Z-A]). Ranges must be specified with the lower valued
letter first and the higher valued letter second."
E_FM300C_BAD_CLASS
"There is a bad character class definition in the template. All character
class definitions must contain at least one specification of a good
character."
E_FM300D_MAX_CCIDX
"The format template contains too many character class definitions.
Only %0d character class definitions are allowed in a format template."
E_FM300E_USR_CLAS_UNDEF
"The user defined character class '%0c' is not yet defined. Before using the
letter '%0c' the character class must be defined (such as [%0c=ABC])."
E_FM300F_BAD_ST_CHAR
"The template contains a '%0c', which is an illegal character. If you would
like this character to be a constant character in the template it must be
preceded by a backslash (as in \%0c)."
E_FM3010_NO_DEF
"The template contains no user entry positions, only constant characters.
Format templates must contains user entry positions."



E_FM6000_NULL_ARGUMENT
"FMT ERROR: One of the pointer parameters to the procedure is NULL."
E_FM6001_NO_TEMPLATE
"FMT ERROR: There is no template for a format requiring a template."
E_FM6002_ZERO_WIDTH
"FMT ERROR: Width of format is zero."
E_FM6003_DISPLAY_NOT_LONGTEXT
"FMT ERROR: Datatype of display parameter, %0d, is not LONGTEXT."
E_FM6004_DISPLAY_LEN_TOO_SHORT
"FMT ERROR: Length of display parameter, %0d, is too short for format width %1d."
E_FM6005_DATATYPE_LEN_INVALID
"FMT ERROR: The length, %0d, of datatype %1d is illegal."
E_FM6006_BAD_DATATYPE
"FMT ERROR: The datatype %0d does not exist."
E_FM6007_BAD_FORMAT
"FMT ERROR: Format type %0d does not exist."
E_FM6008_NOT_SINGLE_LINE_FMT
"FMT ERROR: Format type %0d, width %1d and precision %2d is not a single-line
  format."
E_FM6009_NOT_MULTI_LINE_FMT
"FMT ERROR: Format type %0d, width %1d and precision %2d is not a multi-line
  format."
E_FM600A_CORRUPTED_TEMPLATE
"FMT ERROR: Template contains an illegal character that should have been
  caught by fmt_setfmt."
E_FM600B_FMT_DATATYPE_INVALID
"FMT ERROR: Format %0d is not valid for datatype %1d."
E_FM600C_B_FMT_IMPLIES_NO_TYPE
"FMT ERROR: B format does not imply any datatype."
E_FM600D_BAD_LEN
"FMT ERROR: Length `%0d' is invalid for datatype `%1c'."



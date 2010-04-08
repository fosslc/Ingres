/*
**
**  INTL.H -- definitions for multinational options for backend
**	      and frontends.
**
**  Defines typedef containing multinational options. The options are:
**
**	DATES:
**		US style dates (mm/dd/yy and dd-mm-yy)
**		Multinational  (dd/mm/yy and yy-mm-dd)
**
**	MONEY:
**		currency symbol to use (max 3 characters)
**		placement of currency symbol (leading or trailing), with
**			or without separating blank (the blank is added to
**			the currency symbol stored in the multinationa struct.
**		precision (number of fractional digits two display). We allow
**			a maximum of two fractional digits (default) specified
**			by entering the value two. Negative values cause the
**			low order whole digits to be suppressed. Note that
**			internal computations are always done to two decimal
**			places. Examples...
**
**				Specification	Number		Display
**					1	14234.56	14234.6
**					-3	14234.56	14
**
**	DECIMAL INDICATOR:
**		A single character used to indicate the fractional part of
**		a floating point number.
**
**  History:
 * Revision 1.2  86/04/25  11:15:46  daveb
 * Remove comment cruft, add dependency warning.
 * 
 * Revision 1.2  85/11/05  10:38:44  joe
 * Added a comment about the report writer's dependency on
 * the order of the date comments.
 * 
**	6/29/86 (peb) -- first written for 4.0
**	8/04/85 (jrc) -- Changed to intl.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** WARNING:
**	if the values assigned to these symbolics change, the routine
**	setdateformat() in adt/dates.c must be updated. It assumes the
**	values and ordering specified below.
**
**	Also, report/ritdfmt.c in the front ends depends on the ordering
**	of these constants.
*/
# define	M_US		0	/* US style dates (default) */
# define	M_MULTINATIONAL 1	/* European style dates */
# define	M_FINLAND	2	/* Finnish and Swedish conventions */
# define	M_ISO		3	/* Peter Madam's ISO date */
/* END WARNING */

# define	M_MNY_LEADING	0	/* Currency symbol precedes value */
# define	M_MNY_TRAILING	1	/* Currency symbol after value */

/*
** WARNING:
**	if the value of M_MNY_MAX changes, the structure initialization
**	in dbu/set.c must be adjusted to correspond.
*/
# define	M_MNY_MAX	4	/* Currency symbol size */

typedef struct multi
{
	i4	m_date_format;		/* Date Format
					**	M_US
					**	M_MULTINATIONAL
					*/
	i4	m_mny_format;		/* M_MNY_LEADING
					** M_MNY_TRAILING
					*/
	i4	m_mny_prec;		/* Significant money digits to display*/
	char	m_mny_symbol[M_MNY_MAX+1];
					/* money symbol */
	char	m_decimal;		/* decimal point */
	char	m_thousand;		/* Thousands separator */
} MULTINATIONAL;

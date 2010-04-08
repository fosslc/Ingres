/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<bt.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<fmt.h>
# include	<cm.h>
# include	<tm.h>
# include	<dateadt.h>
# include	<adf.h>
# include	<afe.h>
# include	"format.h"

/*
**	F_FMTDATE - write a date into a string using a date template.
**
**	Parameters:
**		cb	- control block for ADF routine call.
**		date - date value to fit to date template.
**		fmt - FMT structure containing template.
**		result - address of string to contain formatted date.
**				Must be wide enough for template width + 1.
**
**	Returns:
**		OK
**
**	History:
**		2/22/83 (gac)	- written.
**		2/3/84 (gac)	date changed to internal date structure.
**		7/5/84	(gac)	fixed bug 3591 -- null date should be formatted
**				as a blank string with length of template.
**		1/20/85 (rlm)	added ifdef for CS environment (no dates)
**		10-jun-86 (mgw) Bug # 9551
**				fmt->fmt_width is set to be the max possible
**				date in the date case in f_setfmt() and must be
**				used in r_trunc() in rnxtrow.qc to determine how
**				much space to allocate. Don't reset it here by
**				data item length. Rather use a tmp variable to
**				hold the width for f_strlft() and f_strrgt(),
**				not fmt->fmt_width. Use fwidth since we're out
**				of the for loop where it's used previously.
**		04-nov-1986 (yamamoto)
**			changed CH macros to CM.
**	18-dec-86 (grant)	changed parameters to DB_DATA_VALUEs and display
**				is a text string instead of a null-terminated
**				one.
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**	19-dec-89 (cmr) -	UNIX porting changes for 6.3
**				Use if/then/else instead of conditional
**				expressions (i.e. (a ? b : c)) when 
**				calculating time intervals.  Some compilers 
**				couldn't cope w/ the cond exprs.
**	3/8/91 (elein) unreported bug--shows up internally on PC
**		If the input date is nullable, then the result of the
**		timezone arithmetic is nullable.  However, we were declaring
**		the result dbv as regular not nullable. Solution is to 
**		"pretend" the date dbv is not nullable since it will not
**		be null at this point. (fm_format screens out null)  This
**		is the same technique used in afcvinto.c. We put the nullability
**		back on return so as not to corrupt the dbv.
**	9-sep-1992 (mgw) Bug #46546
**		Fixed a problem with integer overflow when calculating an
**		f8 value by forcing the integer operands to float. Also
**		changed the nat's involved in these calculations to i4s
**		so these values are guaranteed to at least be 32 bit ints.
**    23-oct-1991 (mgw) Bug 40071
**            Pass the correct buffer widths down to f_strrgt(), f_strlft(),
**            and f_strctr() to get the correct justifications.
**	11/07/92 (dkh) - Removed dependency on use of TMzone().  This also
**			 leads to correct display of time values that cross
**			 PDT/PST and similar changes.
**	11/19/92 (dkh) - Fixed f_fmtdate() to correctly use the passed in
**			 datevalue whenever it is called
**    21-apr-1994 (donc)
**		Unjustified formats were displaying fmt_width number of 
**		db_t_text characters even if the actual data to be displayed 
**		contained less characters. This caused date data to be 
**		displayed with trailing junk when the actual number of "good" 
**		characters was less the fmt_width. 
**	04/25/93 (dkh) - Rolled back previous change since some clients
**			 expect the formatted data to be the same length
**			 as specified in the format.  The correct fix is
**			 to to blank fill any unused space so that old
**			 data does not show through.  This is consistent
**			 with how unjustified formats are handled for
**			 strings and numerics.
**	06/27/93 (dkh) - Fixed bug 51451.  Put back changes the 11/07/92
**			 and 11/19/92 changes that got lost in r63p to
**			 r65 integrations.
**	10/17/93 (dkh) - Removed include of anytype.h since it serves
**			 no purpose.
**	08/01/94 (connie) Bug #55539
**		Put back Donc's fix since dkh's change causes a regression
**		problem from 6.4/03.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  06-sep-2006 (wonca01) BUG 116592
**	    Remove check for absolute for case('3') in the switch(*template)
**	    statement.  Since day of month can not be an absolute point in time
**	    like a timestamp with Hours:minutes:seconds, the check is not
**	    necessary.
**	19-oct-2006 (gupsh01)
**	    Bug 116916 : Fixed previous change for BUG 116592 which removed 
**	    code needed for some date templates.
*/

/*
** set up DB_DATA_VALUEs and functions for adjusting absolute date by timezone
** only once
*/
static	bool		Tmz_init = FALSE;
static	DB_DATA_VALUE	datestr;
static	DATENTRNL	Tmzdate;
static	DB_DATA_VALUE	*Darr[2];
static	AFE_OPERS	Ops;
static	ADI_FI_ID	Func_id;
static	DB_DATA_VALUE	Diffdbv;
static	DATENTRNL	Diffdate;


STATUS
f_fmtdate(cb, datevalue, fmt, display)
ADF_CB		*cb;
DB_DATA_VALUE	*datevalue;
FMT		*fmt;
DB_DATA_VALUE	*display;
{
/*
**	ifdef knocks out entire routine
*/

#ifdef DATEFMT
	/* internal declarations */

	bool	absolute;
	char	word[100];
	char	class;
	i4	value;
	i4	length;
	i4	year = 0;	/* These should be  */
	i4	month = 0;	/* i4, not i4  */
	i4	day = 0;	/* in case we're    */
	i4	hour = 0;	/* on a machine     */
	i4	minute = 0;	/* where i4  is     */
	i4	second = 0;	/* 16 bits.         */
	i4	unit;
	f8	interval;
	i4	dif = 0;
	i4	fwidth = 0;
	i4	digit;
	bool	prevnumexists;
	i4	previousnumber;
	i4	previouscomp;
	i4	time_zone;
	i4	i;
	char	*t;
	char	*template;
	char	*r;
	DATENTRNL	*date;
	char		*result;
	DB_TEXT_STRING	*disp;
	char	tmzstr[AFE_DATESIZE + 1];
	DB_DATA_VALUE	dbvtmzstr;
	ADI_OP_ID	opid;
	ADI_FI_DESC	fdesc;
	STATUS		status;
	bool		nulledit = FALSE;
	DB_DATE_FMT	orig_dfmt;
	char		*cp;

	/* start of routine */

	f_initlen();	/* initialize global lengths for month & day of week */


	date = (DATENTRNL *) datevalue->db_data;
	disp = (DB_TEXT_STRING *) display->db_data;
	result = (char *) disp->db_t_text;

	if (fmt == NULL || fmt->fmt_type != F_DT ||
	    (BTtest(FD_ABS, (char *)&(fmt->fmt_prec)) &&
	     (date->dn_status & DN_LENGTH)) ||
	    (BTtest(FD_INT, (char *)&(fmt->fmt_prec)) &&
	     (date->dn_status & DN_ABSOLUTE)))
	{
		return afe_cvinto(cb, datevalue, display);
	}
	else if (date->dn_status == DN_NULL)
	{
		MEfill((u_i2) fmt->fmt_width, ' ', result);
		disp->db_t_count = fmt->fmt_width;
		return OK;
	}

	r = result;

	if( AFE_NULLABLE(datevalue->db_datatype))
	{
		AFE_UNNULL(datevalue);
		nulledit = TRUE;
	}
	absolute = date->dn_status & DN_ABSOLUTE;
	if (absolute && (date->dn_status & DN_TIMESPEC))
	{
		/* adjust date for time zone */

		if (!Tmz_init)
		{
			Ops.afe_opcnt = 1;
			Ops.afe_ops = Darr;
			Darr[0] = datevalue;

			status = afe_opid(cb, ERx("c"), &opid);
			if (status != OK)
			{
				return status;
			}

			datestr.db_datatype = DB_CHR_TYPE;
			datestr.db_length = AFE_DATESIZE;

			status = afe_fdesc(cb, opid, &Ops, NULL, &datestr,
				&fdesc);
			if (status != OK)
			{
				return status;
			}

			Func_id = fdesc.adi_finstid;

			Tmz_init = TRUE;
		}

		datestr.db_data = (PTR) tmzstr;
		Darr[0] = datevalue;

		orig_dfmt = cb->adf_dfmt;
		/* Use Finland format to get four digits for the year */
		cb->adf_dfmt = DB_FIN_DFMT;

		status = afe_clinstd(cb, Func_id, &Ops, &datestr);
		if (status != OK)
		{
			cb->adf_dfmt = orig_dfmt;
			return status;
		}

		cb->adf_dfmt = orig_dfmt;

		cp = tmzstr;

		/*
		**  The code below assumes the date is formatted as
		**  yyyy-mm-dd hh:mm:ss.  If this changes, the code
		**  below has to change.
		*/

		/* Get the year value */
		*(cp + 4) = EOS;
		CVan(cp, &year);

		/* Get the month value */
		cp +=5;
		*(cp + 2) = EOS;
		CVan(cp, &month);

		/* Get the day value */
		cp +=3;
		*(cp + 2) = EOS;
		CVan(cp, &day);

		/* Get the hour value */
		cp +=3;
		*(cp + 2) = EOS;
		CVan(cp, &hour);

		/* Get the minute value */
		cp +=3;
		*(cp + 2) = EOS;
		CVan(cp, &minute);

		/* Get the second value */
		cp +=3;
		*(cp + 2) = EOS;
		CVan(cp, &second);
	}
	else
	{
		year = date->dn_year;
		month = date->dn_month;
		day = (I1_CHECK_MACRO(date->dn_highday) << LOWDAYSIZE) +
	      	date->dn_lowday;
		second = date->dn_time / 1000;
	}

	if (!absolute)
	{
		/*
		** calculate components of interval depending on date template
		**
		** Bug 46546 - cast integer calculations to f8 to avoid integer
		** overflow.
		*/

		interval = SECS_YR * (f8)year + SECS_MON * (f8)month +
			   SECS_DAY * (f8)day + SECS_HR * (f8)hour +
			   SECS_MIN * (f8)minute + second;

		switch(BThigh((char *)&(fmt->fmt_prec), FD_LEN)) /* round up */
		{
		case(FD_SECS):
			break;
		/*
		** The following four cases have been changed to use
		** if/then/else rather than a conditional expression,
		** i.e. (a ? b : c), because some compilers couldn't 
		** cope with them. (cmr)
		*/
		case(FD_MINS):
			if ( interval > 0 )
				interval += SECS_MIN/2;
			else
				interval -= SECS_MIN/2;
			break;
		case(FD_HRS):
			if ( interval > 0 )
				interval += SECS_HR/2;
			else
				interval -= SECS_HR/2;
			break;
		case(FD_DAYS):
			if ( interval > 0 )
				interval += SECS_DAY/2;
			else
				interval -= SECS_DAY/2;
			break;
		case(FD_MONS):
			if ( interval > 0 )
				interval += SECS_MON/2;
			else
				interval -= SECS_MON/2;
			break;
		case(FD_YRS):
			if ( interval > 0 )
				interval += SECS_YR/2;
			else
				interval -= SECS_YR/2;
			break;
		}

		for (unit = FD_YRS-1; (unit = BTnext(unit, (char *)&(fmt->fmt_prec), FD_LEN)) >= 0; )
		{	/*
			** Bug 46546 - Since SECS_MIN, SECS_HR, and SECS_DAY
			** are integer values, the integer operands in the
			** multiplications involving these values were cast
			** to f8 to avoid overrunning an integer buffer.
			** Note that there could still be problems for large
			** interval values as described below. Also that i4's
			** are still being used in adu. Perhaps in a future
			** release the second, minute, hour, day, month, and
			** year variables should be changed to accomodate
			** larger values. 64 bit ints would be ideal...
			*/

			switch(unit)
			{
			case(FD_SECS):
				/*
				** Note that since second is a i4, this
				** will fail if i4 is a 32 bit int and
				** interval is greater than a MAX_I4, i.e.
				** about 66 years worth of seconds.
				*/
				second = interval;
				break;
			case(FD_MINS):
				/*
				** Note that since minute is a i4, this
				** will fail if i4 is a 32 bit int and
				** interval is greater than 60 * MAX_I4,
				** i.e. about 3840 years worth of minutes.
				*/
				minute = interval/SECS_MIN;
				interval -= SECS_MIN * (f8)minute;
				break;
			case(FD_HRS):
				/*
				** Note that since hour is a i4, this
				** will fail if i4 is a 32 bit int and
				** interval is greater than 3600 * MAX_I4
				** seconds, i.e. about 224,000 years worth
				** of hours.
				*/
				hour = interval/SECS_HR;
				interval -= SECS_HR * (f8)hour;
				break;
			case(FD_DAYS):
				/*
				** Note that since day is a i4, this
				** will fail if i4 is a 32 bit int and
				** interval is greater than 86400 * MAX_I4
				** seconds, i.e. SECS_DAY * MAX_I4
				** ( > 5 million) years worth of days.
				*/
				day = interval/SECS_DAY;
				interval -= SECS_DAY * (f8)day;
				break;
			case(FD_MONS):
				/*
				** Note that since SECS_MON is a large float
				** value (2,629,746.00) there shouldn't be
				** any problems here.
				*/
				month = interval/SECS_MON;
				interval -= month*SECS_MON;
				break;
			case(FD_YRS):
				/*
				** Note that since SECS_YEAR is a large float
				** value (31,556,952.00) there shouldn't be
				** any problems here.
				*/
				year = interval/SECS_YR;
				interval -= year*SECS_YR;
				break;
			}
		}
	}

	/* output date */

	for (template = fmt->fmt_var.fmt_template; *template != EOS;)
	{
		switch(*template)
		{
		case('\\'):
			template++;
			CMcpyinc(template, r);
			fwidth = 0;
			prevnumexists = FALSE;
			break;
		case('|'):
			template++;
			for (; dif > 0; dif--)
			{
				*r++ = ' ';
			}
			fwidth = 0;
			break;
		case('0'):
			*r++ = *template++;
			fwidth++;
			break;
		case('1'):
			if (*(template+1) == '6')
			{
				template += 2;
				*r++ = '0';
				f_formnumber(hour, 2, &r);
				prevnumexists = TRUE;
				previousnumber = hour;
			}
			else if (*(template+1) == '9' &&
				 *(template+2) == '0' &&
				 *(template+3) == '1')
			{
				template += 4;
				r += 3;
				f_formnumber(year, 4, &r);
				prevnumexists = TRUE;
				previousnumber = year;
			}
			else
			{
				template++;
				length = f_formnumber((absolute ? (year % 100) : year), ++fwidth, &r);
				dif += MAXSETWIDTH - max(fwidth,length);
				prevnumexists = TRUE;
				previousnumber = year;
				previouscomp = 1;
			}
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		case('2'):
			template++;
			length = f_formnumber(month, ++fwidth, &r);
			dif += MAXSETWIDTH - max(fwidth,length);
			prevnumexists = TRUE;
			previousnumber = month;
			previouscomp = 2;
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		case('3'):
			template++;
			if (absolute)
			{
				if (BTtest(FD_MONS, (char *)&(fmt->fmt_prec)))
				{
					length = f_formnumber(day, ++fwidth, &r);
					prevnumexists = TRUE;
					previousnumber = day;
					i = MAXSETWIDTH;
				}
				else if (BTtest(FD_YRS, (char *)&(fmt->fmt_prec)))
				{
					prevnumexists = TRUE;
					previousnumber = f_dayofyear(year, month, day);
					length = f_formnumber(previousnumber, ++fwidth, &r);
					i = 3;
				}
				else
				{
					prevnumexists = TRUE;
					previousnumber = day;
					length = f_formnumber(previousnumber, ++fwidth, &r);
					i = MAXSETWIDTH;
				}
				dif += i - max(fwidth,length);
			}
			else
			{
				f_formnumber(day, ++fwidth, &r);
				prevnumexists = TRUE;
				previousnumber = day;
				previouscomp = 3;
			}
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		case('4'):
			template++;
			length = f_formnumber((absolute ? (hour%12 == 0 ? 12 : hour%12) : hour), ++fwidth, &r);
			dif += MAXSETWIDTH - max(fwidth,length);
			prevnumexists = TRUE;
			previousnumber = hour;
			previouscomp = 4;
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		case('5'):
			template++;
			length = f_formnumber(minute, ++fwidth, &r);
			dif += MAXSETWIDTH - max(fwidth,length);
			prevnumexists = TRUE;
			previousnumber = minute;
			previouscomp = 5;
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		case('6'):
			template++;
			length = f_formnumber(second, ++fwidth, &r);
			dif += MAXSETWIDTH - max(fwidth,length);
			prevnumexists = TRUE;
			previousnumber = second;
			previouscomp = 6;
			fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding number */
			break;
		default:
			if (CMspace(template))
			{
				CMbyteinc(fwidth, template);
				CMcpyinc(template, r);
				break;
			}

			if (CMalpha(template))
			{
				length = f_getword(&template, 0, word);
				if (f_dkeyword(word, &class, &value))
					switch(class)
					{
					case(F_MON):
						if (length == F_monlen)
						{
							length = f_formword(F_Fullmon, month-1, word, &r);
							dif += F_monmax - length;
						}
						else
						{
							f_formword(F_Abbrmon, month-1, word, &r);
						}
						break;

					case(F_DOW):
						i = f_daysince1582(year,month,day) % WEEKLENGTH;
						if (length == F_dowlen)
						{
							length = f_formword(F_Fulldow, i, word, &r);
							dif += F_dowmax - length;
						}
						else
						{
							f_formword((length==2 ? F_Abdow : F_Abbrdow), i, word, &r);
						}
						break;

					case(F_PM):
						f_formword((length==F_pmlen ? F_Fullpm : F_Abbrpm),
							   hour/12, word, &r);
						break;

					case(F_ORD):
						f_formword(F_Ordinal, (previousnumber % 100), word, &r);
						break;

					case(F_TIUNIT):
						if (!absolute && prevnumexists && previouscomp == ((value % 6) + 1))
						{
							/* its a date interval, we have an interval unit, and it's preceded
							** by a number which matches it in type */
							if (previousnumber == 1)
							{
								if (value >= 6)
								{
									/* unit specified is plural but previous number is one,
									** so make the unit singular. */
									f_formword(F_Fullunit, value-6, word, &r);
								}
								else
								{
									f_copyword(word, &r);
								}
							}
							else
							{
								if (value < 6)
								{
									/* unit specified is singular but previous number is not one,
									** so make the unit plural. */
									f_formword(F_Fullunit, value+6, word, &r);
								}
								else
								{
									f_copyword(word, &r);
								}
							}
						}
						else
						{
							f_copyword(word, &r);
						}
						break;
					}
				else
				{
					f_copyword(word, &r);
				}

				fwidth = (CMspace(template)) ? -1 : 0;	/* Numbers do not eat space separating preceding word */
			}
			else
			{
				CMcpyinc(template, r);
				fwidth = 0;
			}
			prevnumexists = FALSE;
			break;
		}
	}

	/* BUG 9551 - fmt->fmt_width is set to max possible date string
	**	      in f_setfmt(), don't reset it here. Rather, use fwidth
	**	      since we're out of the for loop where it's previous
	**	      value was useful to store the value used by f_strlft()
	**	      and f_strrgt() below. - mgw
	*/

	fwidth = r - result;

        /*
        ** BUG 40071 - Had to make an adjustment to this previous fix to
        **             get correct justification. Use fmt->fmt_width for
        **             disp->db_t_count and for the output buffer width of
        **             these justification routines, not fwidth.
        */
        disp->db_t_count = fmt->fmt_width;

	switch(fmt->fmt_sign)
	{
	case(FS_MINUS):
		f_strlft(disp->db_t_text, fwidth,
			 disp->db_t_text, fmt->fmt_width);
		break;

	case(FS_PLUS):
		f_strrgt(disp->db_t_text, fwidth,
			 disp->db_t_text, fmt->fmt_width);
		break;

	case(FS_STAR):
		f_strctr(disp->db_t_text, fwidth,
			 disp->db_t_text, fmt->fmt_width);
		break;

	case(FS_NONE):
		/*
		** DonC: The following test and assignment was
		** added to accomodate W4GL and other products
		** that do not assume a default (like left justification).
		** Prior to this modification, this case had disp->db_t_count
		** arbitrarily set to fmt->fmt_width. This caused the resulting
		** data display to contain junk characters were at the end
		** display text to be printed when the fmt->fmt_width exceeded
		** the actual width of the date data (fwidth).
		*/
		if ( fwidth < fmt->fmt_width )
			disp->db_t_count = fwidth;
		break;
	}

#else
	disp->db_t_count = 0;
#endif
	if(nulledit)
	{
		AFE_MKNULL(datevalue);
	}
	return OK;
}

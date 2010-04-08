/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**	History:
**		1/20/85 (rlm)	split routines which did not use data
**				structure defined herein into separate
**				source file so that data structure is
**				not linked in when date formats aren't
**				being used
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**      25-sep-96 (mcgem01)
**              Global data moved to fmtdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	 <fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<dateadt.h>
# include	"format.h"
# include	<st.h>

# ifdef DATEFMT
# define	daysinyear(yr)		((yr % 4 == 0 && yr % 100 != 0 || yr % 400 == 0) ? 366 : 365)
# define	daysinmonth(mon, yr)	((mon == 2 && daysinyear(yr) == 366) ? monsize[1]+1 : monsize[mon-1])

GLOBALREF i4	monsize[12];
GLOBALREF KYW F_Keywords[]; 
GLOBALREF KYW	*F_Abbrmon;
GLOBALREF KYW	*F_Fullmon;
GLOBALREF KYW	*F_Ordinal;
GLOBALREF KYW	*F_Abdow;
GLOBALREF KYW	*F_Abbrdow;
GLOBALREF KYW	*F_Fulldow;
GLOBALREF KYW	*F_Abbrpm;
GLOBALREF KYW	*F_Fullpm;
GLOBALREF KYW	*F_Fullunit;
GLOBALREF KYW	*F_SFullunit;

GLOBALREF i4	F_monlen;	/* length of 2nd month name
				** (i.e. "February") */
GLOBALREF i4	F_monmax;	/* length of longest month name
				** (i.e. "September") */
GLOBALREF i4	F_dowlen;	/* length of 1st day of week name
				** (i.e. "Sunday") */
GLOBALREF i4	F_dowmax;	/* length of longest day of week name
				** (i.e. "Wednesday") */
GLOBALREF i4	F_pmlen;	/* length of "pm" */

# endif

/*{
** Name:	f_initlen	initializes the date template keyword table.
**
** Description:
**	This routine initializes the date template keyword table.  It must
**	be called before doing any date template processing, either in
**	fmt_setfmt or fmt_format.  It only needs to be called once but
**	can be called more than once since it does nothing if it has already
**	been called.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** History:
**	6-aug-87 (grant)	created.
*/

f_initlen()
{
static	bool	Init = FALSE;	/* TRUE if this routine was already called */

	i4	i;
	i4	len;
	KYW	*fk;

	if (!Init)
	{
		/* initialize the date template keyword table */

		fk = F_Keywords;

		/*	F_Abbrmon: */
		(fk++)->k_name = ERget(F_FM0001_jan);
		(fk++)->k_name = ERget(F_FM0002_feb);
		(fk++)->k_name = ERget(F_FM0003_mar);
		(fk++)->k_name = ERget(F_FM0004_apr);
		(fk++)->k_name = ERget(F_FM0005_may);
		(fk++)->k_name = ERget(F_FM0006_jun);
		(fk++)->k_name = ERget(F_FM0007_jul);
		(fk++)->k_name = ERget(F_FM0008_aug);
		(fk++)->k_name = ERget(F_FM0009_sep);
		(fk++)->k_name = ERget(F_FM000A_oct);
		(fk++)->k_name = ERget(F_FM000B_nov);
		(fk++)->k_name = ERget(F_FM000C_dec);
		/*	F_Fullmon: */
		(fk++)->k_name = ERget(F_FM000D_january);
		(fk++)->k_name = ERget(F_FM000E_february);
		(fk++)->k_name = ERget(F_FM000F_march);
		(fk++)->k_name = ERget(F_FM0010_april);
		(fk++)->k_name = ERget(F_FM0011_may);
		(fk++)->k_name = ERget(F_FM0012_june);
		(fk++)->k_name = ERget(F_FM0013_july);
		(fk++)->k_name = ERget(F_FM0014_august);
		(fk++)->k_name = ERget(F_FM0015_september);
		(fk++)->k_name = ERget(F_FM0016_october);
		(fk++)->k_name = ERget(F_FM0017_november);
		(fk++)->k_name = ERget(F_FM0018_december);
		/*	F_Ordinal: */
		(fk++)->k_name = ERget(F_FM0019_th);  /* 0 */
		(fk++)->k_name = ERget(F_FM001A_st);
		(fk++)->k_name = ERget(F_FM001B_nd);
		(fk++)->k_name = ERget(F_FM001C_rd);
		(fk++)->k_name = ERget(F_FM001D_th);
		(fk++)->k_name = ERget(F_FM001E_th);
		(fk++)->k_name = ERget(F_FM001F_th);
		(fk++)->k_name = ERget(F_FM0020_th);
		(fk++)->k_name = ERget(F_FM0021_th);
		(fk++)->k_name = ERget(F_FM0022_th);
		(fk++)->k_name = ERget(F_FM0023_th);  /* 10 */
		(fk++)->k_name = ERget(F_FM0024_th);
		(fk++)->k_name = ERget(F_FM0025_th);
		(fk++)->k_name = ERget(F_FM0026_th);
		(fk++)->k_name = ERget(F_FM0027_th);
		(fk++)->k_name = ERget(F_FM0028_th);
		(fk++)->k_name = ERget(F_FM0029_th);
		(fk++)->k_name = ERget(F_FM002A_th);
		(fk++)->k_name = ERget(F_FM002B_th);
		(fk++)->k_name = ERget(F_FM002C_th);
		(fk++)->k_name = ERget(F_FM002D_th);  /* 20 */
		(fk++)->k_name = ERget(F_FM002E_st);
		(fk++)->k_name = ERget(F_FM002F_nd);
		(fk++)->k_name = ERget(F_FM0030_rd);
		(fk++)->k_name = ERget(F_FM0031_th);
		(fk++)->k_name = ERget(F_FM0032_th);
		(fk++)->k_name = ERget(F_FM0033_th);
		(fk++)->k_name = ERget(F_FM0034_th);
		(fk++)->k_name = ERget(F_FM0035_th);
		(fk++)->k_name = ERget(F_FM0036_th);
		(fk++)->k_name = ERget(F_FM0037_th);  /* 30 */
		(fk++)->k_name = ERget(F_FM0038_st);
		(fk++)->k_name = ERget(F_FM0039_nd);
		(fk++)->k_name = ERget(F_FM003A_rd);
		(fk++)->k_name = ERget(F_FM003B_th);
		(fk++)->k_name = ERget(F_FM003C_th);
		(fk++)->k_name = ERget(F_FM003D_th);
		(fk++)->k_name = ERget(F_FM003E_th);
		(fk++)->k_name = ERget(F_FM003F_th);
		(fk++)->k_name = ERget(F_FM0040_th);
		(fk++)->k_name = ERget(F_FM0041_th);  /* 40 */
		(fk++)->k_name = ERget(F_FM0042_st);
		(fk++)->k_name = ERget(F_FM0043_nd);
		(fk++)->k_name = ERget(F_FM0044_rd);
		(fk++)->k_name = ERget(F_FM0045_th);
		(fk++)->k_name = ERget(F_FM0046_th);
		(fk++)->k_name = ERget(F_FM0047_th);
		(fk++)->k_name = ERget(F_FM0048_th);
		(fk++)->k_name = ERget(F_FM0049_th);
		(fk++)->k_name = ERget(F_FM004A_th);
		(fk++)->k_name = ERget(F_FM004B_th);  /* 50 */
		(fk++)->k_name = ERget(F_FM004C_st);
		(fk++)->k_name = ERget(F_FM004D_nd);
		(fk++)->k_name = ERget(F_FM004E_rd);
		(fk++)->k_name = ERget(F_FM004F_th);
		(fk++)->k_name = ERget(F_FM0050_th);
		(fk++)->k_name = ERget(F_FM0051_th);
		(fk++)->k_name = ERget(F_FM0052_th);
		(fk++)->k_name = ERget(F_FM0053_th);
		(fk++)->k_name = ERget(F_FM0054_th);
		(fk++)->k_name = ERget(F_FM0055_th);  /* 60 */
		(fk++)->k_name = ERget(F_FM0056_st);
		(fk++)->k_name = ERget(F_FM0057_nd);
		(fk++)->k_name = ERget(F_FM0058_rd);
		(fk++)->k_name = ERget(F_FM0059_th);
		(fk++)->k_name = ERget(F_FM005A_th);
		(fk++)->k_name = ERget(F_FM005B_th);
		(fk++)->k_name = ERget(F_FM005C_th);
		(fk++)->k_name = ERget(F_FM005D_th);
		(fk++)->k_name = ERget(F_FM005E_th);
		(fk++)->k_name = ERget(F_FM005F_th);  /* 70 */
		(fk++)->k_name = ERget(F_FM0060_st);
		(fk++)->k_name = ERget(F_FM0061_nd);
		(fk++)->k_name = ERget(F_FM0062_rd);
		(fk++)->k_name = ERget(F_FM0063_th);
		(fk++)->k_name = ERget(F_FM0064_th);
		(fk++)->k_name = ERget(F_FM0065_th);
		(fk++)->k_name = ERget(F_FM0066_th);
		(fk++)->k_name = ERget(F_FM0067_th);
		(fk++)->k_name = ERget(F_FM0068_th);
		(fk++)->k_name = ERget(F_FM0069_th);  /* 80 */
		(fk++)->k_name = ERget(F_FM006A_st);
		(fk++)->k_name = ERget(F_FM006B_nd);
		(fk++)->k_name = ERget(F_FM006C_rd);
		(fk++)->k_name = ERget(F_FM006D_th);
		(fk++)->k_name = ERget(F_FM006E_th);
		(fk++)->k_name = ERget(F_FM006F_th);
		(fk++)->k_name = ERget(F_FM0070_th);
		(fk++)->k_name = ERget(F_FM0071_th);
		(fk++)->k_name = ERget(F_FM0072_th);
		(fk++)->k_name = ERget(F_FM0073_th);  /* 90 */
		(fk++)->k_name = ERget(F_FM0074_st);
		(fk++)->k_name = ERget(F_FM0075_nd);
		(fk++)->k_name = ERget(F_FM0076_rd);
		(fk++)->k_name = ERget(F_FM0077_th);
		(fk++)->k_name = ERget(F_FM0078_th);
		(fk++)->k_name = ERget(F_FM0079_th);
		(fk++)->k_name = ERget(F_FM007A_th);
		(fk++)->k_name = ERget(F_FM007B_th);
		(fk++)->k_name = ERget(F_FM007C_th);
		/*	F_Abdow: */
		(fk++)->k_name = ERget(F_FM007D_th);
		(fk++)->k_name = ERget(F_FM007E_fr);
		(fk++)->k_name = ERget(F_FM007F_sa);
		(fk++)->k_name = ERget(F_FM0080_su);
		(fk++)->k_name = ERget(F_FM0081_mo);
		(fk++)->k_name = ERget(F_FM0082_tu);
		(fk++)->k_name = ERget(F_FM0083_we);
		/*	F_Abbrdow: */
		(fk++)->k_name = ERget(F_FM0084_thu);
		(fk++)->k_name = ERget(F_FM0085_fri);
		(fk++)->k_name = ERget(F_FM0086_sat);
		(fk++)->k_name = ERget(F_FM0087_sun);
		(fk++)->k_name = ERget(F_FM0088_mon);
		(fk++)->k_name = ERget(F_FM0089_tue);
		(fk++)->k_name = ERget(F_FM008A_wed);
		/*	F_Fulldow: */
		(fk++)->k_name = ERget(F_FM008B_thursday);
		(fk++)->k_name = ERget(F_FM008C_friday);
		(fk++)->k_name = ERget(F_FM008D_saturday);
		(fk++)->k_name = ERget(F_FM008E_sunday);
		(fk++)->k_name = ERget(F_FM008F_monday);
		(fk++)->k_name = ERget(F_FM0090_tuesday);
		(fk++)->k_name = ERget(F_FM0091_wednesday);
		/*	F_Abbrpm: */
		(fk++)->k_name = ERget(F_FM0092_a);
		(fk++)->k_name = ERget(F_FM0093_p);
		/*	F_Fullpm: */
		(fk++)->k_name = ERget(F_FM0094_am);
		(fk++)->k_name = ERget(F_FM0095_pm);
		/*	F_Fullunit: */
		(fk++)->k_name = ERget(F_FM0096_year);
		(fk++)->k_name = ERget(F_FM0097_month);
		(fk++)->k_name = ERget(F_FM0098_day);
		(fk++)->k_name = ERget(F_FM0099_hour);
		(fk++)->k_name = ERget(F_FM009A_minute);
		(fk++)->k_name = ERget(F_FM009B_second);
		/*	F_SFullunit: */
		(fk++)->k_name = ERget(F_FM009C_years);
		(fk++)->k_name = ERget(F_FM009D_months);
		(fk++)->k_name = ERget(F_FM009E_days);
		(fk++)->k_name = ERget(F_FM009F_hours);
		(fk++)->k_name = ERget(F_FM00A0_minutes);
		(fk++)->k_name = ERget(F_FM00A1_seconds);

		/* determine length of date specification month name */
		F_monlen = STlength(F_Fullmon[F_MONVALUE-1].k_name);

		/* determine the length of the longest month name */
		for (i = 0; i < MONS_YR; i++)
		{
			len = STlength(F_Fullmon[i].k_name);
			if (F_monmax < len)
			{
				F_monmax = len;
			}
		}

		/* determine length of date specification day of week name */
		F_dowlen = STlength(F_Fulldow[F_DOWVALUE].k_name);

		/* determine the length of the longest day of week name */
		for (i = 0; i < WEEKLENGTH; i++)
		{
			len = STlength(F_Fulldow[i].k_name);
			if (F_dowmax < len)
			{
				F_dowmax = len;
			}
		}

		/* determine length of "pm" */
		F_pmlen = STlength(F_Fullpm[1].k_name);

		Init = TRUE;
	}
}


bool
f_dkeyword(word,class,value)
register char	*word;
char		*class;
i4		*value;
	/* return true if 'word' is a keyword; also returns keyword 'class' and
	   'value' */
{
	register i4	i;
	char		tword[100];

#ifdef DATEFMT
	STcopy(word, tword);
	CVlower(tword);
	for (i = 0; F_Keywords[i].k_name; i++)
		if (STcompare(F_Keywords[i].k_name, tword) == 0)
		{
			*class = F_Keywords[i].k_class;
			*value = F_Keywords[i].k_value;
			return(TRUE);
		}
#endif
	return(FALSE);
}

# ifdef DATEFMT
i4
f_dayofyear(year,month,day)
i4	year;
i4	month;
i4	day;
		/* Determine the day of the year for the date specified by the
		   year (1582-????), month (1-12), and day (1-31). */
{
	register i4	totaldays = 0;
	register i4	i;

	for (i=1; i < month; i++)
		totaldays += daysinmonth(i, year);
	totaldays += day;
	return(totaldays);
}

i4
f_daysince1582(year,month,day)
i4	year;
i4	month;
i4	day;
	/* Returns the number of days since 1-jan-1582 for the date specified
	   by the year (1582-????), month (1-12), and day (1-31).
	   For example, 1-jan-1582 returns 1. */
{
	register i4	totaldays = 0;
	register i4	i;

	for (i=1582; i < year; i++)
		totaldays += daysinyear(i);
	totaldays += f_dayofyear(year, month, day);
	return(totaldays);
}

STATUS
f_monthdayofmonth(dayofyear, year, month, dayofmonth)
i4	dayofyear;
i4	year;
i4	*month;
i4	*dayofmonth;
	/* Returns the month (1-12) and day of month (1-31), given the day
	   of year (1-366) and year (1582-2382).
	   If the day of year exceeds the number of days in the year,
	   a FAIL status is returned. */
{
	register i4	i;
	i4		dim;

	if (dayofyear > daysinyear(year))
	{
		return FAIL;
	}

	for (i = 1; dayofyear > (dim = daysinmonth(i, year)); i++)
	{
		dayofyear -= dim;
	}

	*dayofmonth = dayofyear;
	*month = i;

	return OK;
}

f_yearmonthdayofmonth(daysince1582, year, month, dayofmonth)
i4	daysince1582;
i4	*year;
i4	*month;
i4	*dayofmonth;
	/* Returns the year (1582-2382), month (1-12) and day (1-31), given
	   the number of days since 1-jan-1582. */
{
	register i4	i;
	i4		diy;

	for (i = 1582; daysince1582 > (diy = daysinyear(i)); i++)
	{
		daysince1582 -= diy;
	}

	*year = i;
	(VOID) f_monthdayofmonth(daysince1582, i, month, dayofmonth);
}

i4
f_getcurryear(cb)
ADF_CB	*cb;
	/* Returns the current year */
{
    DB_DATA_VALUE		from;
    DB_DATA_VALUE		into;
    char			*today = ERx("today");
    DATENTRNL			date;

    from.db_datatype = DB_CHR_TYPE;
    from.db_length = STlength(today);
    from.db_data = (PTR) today;

    into.db_datatype = DB_DTE_TYPE;
    into.db_length = sizeof(date);
    into.db_data = (PTR) &date;

    afe_cvinto(cb, &from, &into);

    return date.dn_year;
}
# endif


/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<bt.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<cm.h>
# include	"format.h"
# include	<st.h>

/**
** Name:	findate.c	This file contains the routine for converting
**				strings formatted with a date template
**				into an internal date value.
**
** Description:
**	This file defines:
**
**	f_indate	Convert a long text string into a date
**			using a date template format as its guide.
**
** History:
**	6-jan-87 (grant)	created.
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**	08-07-91 (tom) - fixed bug 39066 (incorrect interpretation of
**			times around midnight and noon).
**	09/20/92 (dkh) - Fixed input for dates to match the number of characters
**			 specified in the date template.  This removes
**			 ambiguity wrt to interpreting user input that omits
**			 the leading zero on months/days that are less than 10.
**			 Resolves SIR 40938.
**	07-sep-93 (essi)
**		Fixed bug 39148 by making sure the year part is made up of
**		digits only.
**	10/07/93 (rudyw)
**		Consolidate code for SIR 40938, code for bug fixes 39148 and
** 		bug 52280 into a single code section which detects when a user
**		entering data in an all numeric date template has entered 
**		less/more than the number of chars specified or has entered
**		non-numeric data.
**	11/01/93 (dkh) - Fixed up prototype mismatch complaint.
**      11/15/93 (rudyw)
**		Modify submission of 10/7/93 to account for the fact that user
**		data is not terminated with EOS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

# define	NOT_SET		-32767	/* must be < -800 since this may be a
					** legal year interval
					*/

/* extern's */
/* static's */

/*{
** Name:	f_indate	Converts a long text string into a date
**				using a date template format.
**
** Description:
**	This routine converts a long text string into a date value using
**	a date template format as a guide.
**
** Inputs:
**	cb		Control block for ADF routine call.
**	fmt		Pointer to FMT structure which is a date template.
**
**	display		Points to the long text string.
**
** Outputs:
**	value		Points to the converted date.
**
**	Returns:
**		E_DB_OK
**		E_DB_ERROR
**
** History:
**	6-jan-87 (grant)	stubbed.
**	9-sep-1992 (mgw) Bug #46546
**		Changed f_getnumber()'s third arg to i4 to ensure at
**		least 32 bit int.
**	16-mar-1994 (rdrane)
**		If input consists soley of a day-of-the-week supplied,
**		then this constitutes an error (b60455).
**      7-aug-1995 (kch)
**              Return from f_indate() if template word and date word are not
**              equal. This avoids problems converting days/months/years to
**              hours. This change fixes bug 69810.
**	27-nov-1995 (canor01/kch)
**		Move fix for bug 69810 - return from f_indate() if
**		template keyword class is F_TIUNIT (time interval unit).
**	06-Nov-2006 (kiria01) b117042
**		Conform CM macro calls to relaxed bool form
*/

STATUS
f_indate(cb, fmt, display, value)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*display;
DB_DATA_VALUE	*value;
{
    STATUS		status;
    char		*t;
    char		date[100];
    char		*dt;
    DB_DATA_VALUE	dvfrom;
    DB_TEXT_STRING	*disp;
    u_char		*d;
    u_char		*dstart;
    u_char		*dend;
    i4		year;
    i4		month;
    i4		day;
    i4		hour = 0;
    i4		minute = 0;
    i4		second = 0;
    i4			fwidth = 0;
    char		tword[100];
    char		tclass;
    i4			tvalue;
    i4			tlength;
    char		dword[100];
    char		dclass;
    i4			dvalue;
    i4			dlength;
    i4			len;
    bool		dkw;
    i4			pmhours = 0;
    bool		is_am = FALSE;
    char		*spec_comps;	/* specified components in template */
    DB_DATE_FMT		curr_dfmt;
    i4			next1;
    i4			next2;
    i4		*ptrcomp;
    char		*comp;
    bool                all_numeric;
    i4                  idx;

    /* start of routine */

    t = fmt->fmt_var.fmt_template;
    if (t == NULL)
    {
	return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
    }

    spec_comps = (char *) &(fmt->fmt_prec);

    if (BTtest(FD_INT, spec_comps))
    {	 /* date interval */
	year = 0;
	month = 0;
	day = 0;
    }
    else /*absolute date */
    {
	year = NOT_SET;
	month = NOT_SET;
	day = NOT_SET;
    }

    disp = (DB_TEXT_STRING *) display->db_data;
    dstart = disp->db_t_text;
    dend = dstart + disp->db_t_count;

    /*
    ** The following two sections of code are a consolidation of
    **    1) Implementation of SIR 40938 introduced in version 6.4/03/00.
    **    2) fix for BUG 52280 (a side effect of the SIR implementation)
    **    3) Implementation of BUG 39148 (patch generated)
    ** The intent carried forward from 40938 is to force users entering data in
    ** a templated date field to enter the exact number of characters as exist
    ** in the template to avoid any ambiguities resulting from having less than
    ** the required number of characters (eg 109 against d"0203" template).
    ** The refinement is to make this apply only to templates which are all
    ** numeric under the assumption that when field separators exist, there 
    ** are no amiguities to resolve. Failure at this point does not result
    ** in a direct error, but only causes the data to be passed back to the
    ** ADF routines which will process the input according to the valid date
    ** input formats it knows about (documented in the SQL Reference Manual).
    ** 
    ** Section 1 - Determine if the date template is completely numeric
    */
    for ( idx=0, all_numeric=TRUE; *(t+idx)!=EOS ; idx++)
    {
        if ( !CMdigit(t+idx) )
        {
            all_numeric = FALSE;
            break;
        }
    }

    /* 
    ** Section 2 - Test the data entered if the template is numeric
    */
    if ( all_numeric == TRUE )
    {
        /* Ensure that input data length matches the template length */
        if ( STlength(t) != dend-dstart )
        {
	    return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
                             dend-dstart, (PTR)dstart);
        }

        /* Ensure that input data is all numeric */
        for ( idx=0; idx < dend-dstart; idx++)
        {
            if ( !CMdigit(dstart+idx) )
            {
	        return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
                                 dend-dstart, (PTR)dstart);
            }
        }
    }

    /* scan template and date string, picking off components of date */

    for (d = dstart; *t != EOS && d < dend;)
    {
	for (; CMwhite(t); CMnext(t))
	    ;

	for (; d < dend && CMwhite(d); CMnext(d))
	    ;

	switch (*t)
	{
	case '\\':
	    CMnext(t);
	    if (CMcmpnocase(t, d) != 0)
	    {
		return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
				 CMbytecnt(t), (PTR) t, CMbytecnt(d), (PTR) d);
	    }
	    CMnext(t);
	    CMnext(d);
	    fwidth = 0;
	    break;

	case '|':
	    CMnext(t);
	    break;

	case '0':
	    fwidth++;
	    CMnext(t);
	    break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	    switch (*t)
	    {
	    case '1':
		if (*(t+1) == '6')
		{
		    CMnext(t);
		    fwidth++;
		    ptrcomp = &hour;
		}
		else
		{
		    if (*(t+1) == '9' &&
			*(t+2) == '0' &&
			*(t+3) == '1')
		    {
			t += 3;
			fwidth = 3;
		    }
		    ptrcomp = &year;
		}
		break;

	    case '2':
		ptrcomp = &month;
		break;

	    case '3':
		ptrcomp = &day;
		break;

	    case '4':
		ptrcomp = &hour;
		break;

	    case '5':
		ptrcomp = &minute;
		break;

	    case '6':
		ptrcomp = &second;
		break;
	    }

	    CMnext(t);
	    if (++fwidth == 1)
	    {
		fwidth = dend-d;
	    }
	    else
	    {
		fwidth = min(fwidth, dend-d);
	    }

	    len = f_getnumber(&d, fwidth, ptrcomp);
	    if (len == 0)
	    {
		return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
				 dend-d, (PTR) d);
	    }

	    fwidth = 0;
	    break;

	default:
	    if (CMalpha(t))
	    {
		tlength = f_getword(&t, 0, tword);
		dlength = f_getword(&d, dend-d, dword);
		if (dlength == 0)
		{
		    return afe_error(cb, E_FM2002_EXPECTED_WORD, 2,
				     tlength, (PTR) tword);
		}

		dkw = f_dkeyword(dword, &dclass, &dvalue);
		if (f_dkeyword(tword, &tclass, &tvalue))
		{
		    if (!dkw || tclass != dclass)
		    {
			return afe_error(cb, E_FM2003_WRONG_WORD, 2,
					 dlength, (PTR) dword);
		    }

		    switch (tclass)
		    {
		    case F_MON:
			month = dvalue;
			break;

		    case F_PM:
			pmhours = dvalue;

			/* we must remember if we explictly saw am indicator */
			if (pmhours == 0)
				is_am = TRUE;
			break;

		    /* Bug 72518 - move fix for bug 69810 here - return from */
		    /* f_indate() if template keyword class is F_TIUNIT      */
		    /* (time interval unit).                                 */
		    case F_TIUNIT:
			return afe_error(cb, E_FM2003_WRONG_WORD, 2,
			                 dlength, (PTR) dword);
		    }
		}
		else
		{
		    len = min(tlength, dlength);
		    if (STbcompare(tword, len, dword, len, TRUE) != 0)
		    {
			return afe_error(cb, E_FM2013_EXPECTED_WORD_INSTEAD, 4,
					 tlength, (PTR) tword,
					 dlength, (PTR) dword);
		    }
		}
	    }
	    else if (CMcmpcase(t, d) != 0)
	    {
		return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
				 CMbytecnt(t), (PTR) t, CMbytecnt(d), (PTR) d);
	    }
	    else
	    {
		CMnext(t);
		CMnext(d);
	    }
	    break;
	}
    }

    /*
    ** check that specified components have been entered and
    ** adjust for missing components.
    */

    if (!BTtest(FD_INT, spec_comps))
    {	 /* absolute date */

	/* if it's not 12 then we must add 12 hours if it's pm */
	if (hour != 12)
	{
	    hour += pmhours;
	}
	else	/* if it's 12 am we represent as hour 0 */ 
	{
	    if (is_am == TRUE)
	    {
		hour = 0;
	    }
	}
	if (BTtest(FD_DAYS, spec_comps))
	{    /* day specified */
	    if (day == NOT_SET)
	    {	 /* day not entered */
		comp = F_SFullunit[2].k_name;
		return afe_error(cb, E_FM2004_DATE_COMP_NOT_ENTERED, 2,
				 STlength(comp), (PTR)comp);
	    }

	    if (BTtest(FD_MONS, spec_comps))
	    {	 /* month specified */
		if (month == NOT_SET)
		{    /* month not entered */
		    comp = F_SFullunit[1].k_name;
		    return afe_error(cb, E_FM2004_DATE_COMP_NOT_ENTERED, 2,
				     STlength(comp), (PTR)comp);
		}

		if (year == NOT_SET)
		{    /* month,day specified & entered;
		     ** year not entered (specified or not) */
		    year = f_getcurryear(cb);
		}
	    }
	    else
	    {	 /* no month specified */
		if (BTtest(FD_YRS, spec_comps))
		{    /* year specified */
		    if (year == NOT_SET)
		    {	 /* year not entered */
			comp = F_SFullunit[0].k_name;
			return afe_error(cb, E_FM2004_DATE_COMP_NOT_ENTERED, 2,
					 STlength(comp), (PTR)comp);
		    }

		    if (f_monthdayofmonth(day, year, &month, &day) != OK)
		    {
			return afe_error(cb, E_FM2005_DAY_OF_YEAR_TOO_LARGE, 4,
					 sizeof(day), (PTR) &day,
					 sizeof(year), (PTR) &year);
		    }
		}
		else
		{    /* no year specified */
		    f_yearmonthdayofmonth(day, &year, &month, &day);
		}
	    }
	}
	else	/* no day specified */
	{
	    if (BTtest(FD_MONS, spec_comps))
	    {	 /* month specified */
		if (month == NOT_SET)
		{
		    comp = F_SFullunit[1].k_name;
		    return afe_error(cb, E_FM2004_DATE_COMP_NOT_ENTERED, 2,
				     STlength(comp), (PTR)comp);
		}

		day = 1;
		if (year == NOT_SET)
		{    /* year not entered (specified or not) */
		    year = f_getcurryear(cb);
		}
	    }
	    else
	    {	 /* no month specified */
		if (BTtest(FD_YRS, spec_comps))
		{	/* year specified */
		    if (year == NOT_SET)
		    {
			comp = F_SFullunit[0].k_name;
			return afe_error(cb, E_FM2004_DATE_COMP_NOT_ENTERED, 2,
					 STlength(comp), (PTR)comp);
		    }

		    day = 1;
		    month = 1;
		}
	    }
	}
    }

    /* construct a standard date from the components */

    dt = date;
    STcopy(ERx(""), date);

    next1 = BTnext(FD_YRS-1, spec_comps, FD_LEN);
    next2 = BTnext(FD_HRS-1, spec_comps, FD_LEN);

    if (BTtest(FD_INT, spec_comps))
    {		/* date interval */
	if (next1 >= FD_YRS && next1 <= FD_DAYS)
	{    /* years, months, or days specified */
	    STprintf(dt, ERx("%d yrs %d mos %d days "), year, month, day);
	    dt += STlength(dt);
	}

	if (next2 >= FD_HRS && next2 <= FD_SECS)
	{    /* hours, minutes, or seconds specified */
	    STprintf(dt, ERx("%d hrs %d mins %d secs"), hour, minute, second);
	    dt += STlength(dt);
	}
    }
    else	/* absolute date */
    {
	if (next1 >= FD_YRS && next1 <= FD_DAYS)
	{    /* year, month, or day was specified */
	    STprintf(dt, ERx("%d/%d/%d "), month, day, year);
	    dt += STlength(dt);
	}

	if (next2 >= FD_HRS && next2 <= FD_SECS)
	{    /* hour, minute, or second was specified */
	    STprintf(dt, ERx("%d:%d:%d"), hour, minute, second);
	    dt += STlength(dt);
	}
    }

    /*
    ** At this point, we should have an absolute date.  Any day-of-the-week
    ** supplied with the input is really an error, but we ignore the string.
    **  If the resultant value is an empty string and we got here, then only
    ** the day-of-the-week was supplied, and this constitutes an error (b60455).
    */
    if  (STlength(date) == 0)
    {
	return(E_DB_ERROR);
    }

    /* save current system environment date format and set it to US */

    curr_dfmt = cb->adf_dfmt;
    cb->adf_dfmt = DB_US_DFMT;

    /* finally convert from string to the internal date type */

    dvfrom.db_datatype = DB_CHR_TYPE;
    dvfrom.db_length = STlength(date);
    dvfrom.db_data = (PTR) date;

    status = afe_cvinto(cb, &dvfrom, value);

    /* change system environment back to previous date format */

    cb->adf_dfmt = curr_dfmt;

    return status;
}

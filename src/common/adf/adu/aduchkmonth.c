/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <st.h>
#include    <tm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    "adumonth.h"
#include    "adu2date.h"

/**
**  Name:   ADUCHKMONTH.C - Get information about months.
**
**  Description:
**	  This file contains routines necessary to determine if a month is
**	valid and return information about a month such as its size in days
**	and its calendar sequential number.
**
**	This file defines:
**
**	    adu_1monthchk() -    determine if a given month is valid.
**	    adu_2monthsize() -   get the number of days in the month.
**
**  Function prototypes define in ADUMONTH.H
**
**  History:	
**	14-apr-86 (ericj)
**	    Modified from 4.0/02 code for Jupiter.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define    MONTHS_PER_YR   12


/*{
** Name: adu_1monthchk() -  determine if a given month is valid and get its
**			    sequence number.
**
** Description:
**        This routine determines if a given month if valid.  The month can
**	be represented as either an integer (between 1 and 12 inclusive) or
**	as a string (spelled out or abbreviated).  If the month is valid,
**	its calendar sequential number is returned as an i4.
**
** Inputs:
**      input                           string containing month either as an
**					integer, spelled out, or abbreviated.
**      output				pointer to i4 to return months sequence
**					number (e.g. a number 1 thru 12).
**
** Outputs:
**      *output				the month's corresponding number.
**
**	Returns:
**	    0				if month was valid.
**	    1				otherwise.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-apr-86 (ericj)
**	    modified 4.0/02 code for Jupiter.
**	17-mar-87 (thurston)
**	    Added cast to pacify Whitesmith compiler's READONLY stuff.
**	15-Mar-2010 (kschendel) SIR 123275
**	    Use month lookup, months separated out now.
*/

bool
adu_1monthcheck(
char    *input,
i4	*output)
{
    i4			    month;

    /* month can be an integer, or an alphanumeric code */

    if (CVal(input, &month) == OK)
    {
        *output = month;
        return (month < 1 || month > MONTHS_PER_YR);
    }
    else
    {
	const ADU_DATENAME *d;

        _VOID_ CVlower(input);

	d = adu_monthname_lookup(input,STlength(input));
	if (d == NULL)
	{
	    *output = 0;
	    return 1;
	}

	*output = d->d_value; 
	return (d->d_class != 'm'); /* TWISTED LOGIC! */
    }
}


/*{
** Name: adu_2monthsize()	- determine the size of a month in days.
**
** Description:
**        This routine returns the numbers of days in a given month for a
**	specific year.
**
** Inputs:
**      month                           an i4 designating the month.
**	year				the year we're interested in.
**
** Outputs:
**
**	Returns:
**	    size			the number of days in this month for
**					this year.
**
** History:
**	14-apr-86 (ericj)
**	    Modified from 4.0/02 code for Jupiter
**	25-Jan-2005 (jenjo02)
**	    TMyrsize() iff month == 2.
*/

i4
adu_2monthsize(
i4  month,
i4  year)
{
    register i4	    size;
	     i4     yrDays;	    /* WECO compilers can't pass address
				    ** of register variable
				    */

    size = Adu_ii_dmsize[month - 1];

    if ( month == 2 )
    {
	TMyrsize((i4) year, &yrDays);

	if ( yrDays == LEAPYEAR )
	    size++;     /* This is February of a leap year */
    }

    return (size);
}

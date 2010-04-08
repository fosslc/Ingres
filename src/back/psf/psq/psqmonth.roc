/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <sqlstate.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psqmonth.h>

/**
**
**  Name: PSQMONTH.C - Functions for handling months for the "save" command
**
**  Description:
**      This file contains the functions for handling months for the "save"
**	command.
**
**          psq_month - Get the month number from a string
**	    psq_monsize - Get the month size from a month and a year
**
**
**  History:    $Log-for RCS$
**      29-jul-86 (jeff)
**          Adapted from monthchk.c in 4.0.
**	15-jan-92 (barbara)
**	    Included ddb.h, qefrcb.h.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	29-jun-93 (andre)
**	    #included CV.H and TM.H
**      16-sep-93 (swm)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-Sep-2008 (jonj)
**	    Add include of pshparse.h to pick up psf_error() prototype,
**	    necessitating a whole slew of other includes.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/


/*}
** Name: PSQ_MONTHTAB - Table of month names versus numbers
**
** Description:
**      This structure is used for looking up month numbers given their
**	names.
**
** History:
**      29-jul-86 (jeff)
**          Adapted from monthtab.roc in 4.0.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
typedef struct _PSQ_MONTHTAB
{
    char            *code;              /* Month name */
    i4              month;              /* Month number */
} PSQ_MONTHTAB;

/*
**  Definition of static variables and forward static functions.
*/

static  const PSQ_MONTHTAB Monthtab[] =   /* Table of month names vs nums */
{
    "jan",	    1,
    "feb",	    2,
    "mar",	    3,
    "apr",	    4,
    "may",	    5,
    "jun",	    6,
    "jul",	    7,
    "aug",	    8,
    "sep",	    9,
    "oct",	    10,
    "nov",	    11,
    "dec",	    12,
    "january",	    1,
    "february",	    2,
    "march",	    3,
    "april",	    4,
    "june",	    6,
    "july",	    7,
    "august",	    8,
    "september",    9,
    "october",	    10,
    "november",	    11,
    "december",	    12,
    NULL
};

static  const i4  Dmsize[] =          /* Number of days in each month */
{
    31,	    /* January */
    28,	    /* February, non-leap year */
    31,	    /* March */
    30,	    /* April */
    31,	    /* May */
    30,	    /* June */
    31,	    /* July */
    31,	    /* August */
    30,	    /* September */
    31,	    /* October */
    30,	    /* November */
    31	    /* December */
};

/*{
** Name: psq_month	- Get a month number given its name
**
** Description:
**      This function takes a null-terminated string representing a month,
**	and returns a number representing the month, where January is 1
**	and December is 12.
**
** Inputs:
**      monthname                       Month string
**	monthnum			Place to put month number (a nat)
**	err_blk				Filled in if an error happens
**
** Outputs:
**      monthnum                        Filled in with the month number
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Illegal month name
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (jeff)
**          Adapted from 4.0 monthchk.c
*/
DB_STATUS
psq_month(
	char               *monthname,
	i4		   *monthnum,
	DB_ERROR	   *err_blk)
{
    register PSQ_MONTHTAB *p;
    i4                  month;
    DB_STATUS		ret_val;
    i4		err_code;

    CVlower(monthname);

    for (p = (PSQ_MONTHTAB*) Monthtab; p->code != NULL; p++)
    {
	if (STcompare(monthname, p->code) == 0)
	{
	    *monthnum = p->month;
	    return (E_DB_OK);
	}
    }

    (VOID) psf_error(5601L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	STlength(monthname), monthname);
    return (E_DB_ERROR);
}

/*{
** Name: psq_monsize	- Get number of days in month given year
**
** Description:
**      This function takes a month number and a year number and returns
**	the number of days in that month and year.
**
** Inputs:
**      month                           Month number
**	year				Year number
**
** Outputs:
**	Returns:
**	    Number of days in that month and year
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (jeff)
**          Adapted from 4.0 monthsize() function
*/
i4
psq_monsize(
	i4     month,
	i4	year)
{
    i4      size;
    i4	    yrdays;

    size = Dmsize[month - 1];

    TMyrsize(year, &yrdays);

    if (month == 2 && yrdays == 366)
	size++;	    /* This is February of a leap year */

    return (size);
}

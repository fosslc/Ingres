/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    "adumonth.h"

/*
**
**  Name:   ADUMONTAB.ROC -  define global read-only data structures to define
**			     months.
**  Description:
**	  This file defines global read-only data structures which are used to
**	define the parameters of a month.  It is used by the internal date
**	routines.
**
**  History: 
**	14-apr-86 (ericj)
**	    Adapted from 4.0/02 code for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	06-nov-88 (thurston)
**	    Temporarily commented out the READONLY on the month table.
**	15-jun-89 (jrb)
**	    Changed GLOBALDEF to GLOBALCONSTDEF for month table.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Fix above so that it works on VMS and it agrees with the header.
*/

/* Jam hints
**
LIBRARY = IMPADFLIBDATA
**
*/



GLOBALCONSTDEF i4	    Adu_ii_dmsize[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

GLOBALCONSTDEF ADU_MONTHTAB Adu_monthtab[] =
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
    0
};

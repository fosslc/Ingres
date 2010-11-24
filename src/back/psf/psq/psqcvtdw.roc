/*
** Copyright (c) 1986, 2008, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <st.h>
#include    <psqcvtdw.h>

/**
**
**  Name: PSQCVTDW.ROC - Convert day of week string to day of week number
**
**  Description:
**      This file contains the functions for converting strings standing for
**	days of the week into numbers.
**
**          psq_cvtdow - Convert string to day of week
**
**
**  History:    $Log-for RCS$
**      20-aug-86 (jeff)
**          Adapted from cvtdow() in 4.0 dprot.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_cvtdow(
	char *sdow);

/*}
** Name: PSQ_DOWNAME - Day of week versus day number
**
** Description:
**      This table is used for converting days of the week to day numbers
**
** History:
**      20-aug-86 (jeff)
**          adapted from struct downame in 4.0 dprot.c
*/
typedef struct _PSQ_DOWNAME
{
    char            *dow_name;          /* Day of week name */
    i4              dow_num;            /* Day of week number */
} PSQ_DOWNAME;

/*
**  Definition of static variables and forward static functions.
*/

static  const PSQ_DOWNAME Psq_dowlist[] = 	/* List of days of week */
{
    {"sun",	    0},
    {"sunday",	    0},
    {"mon",	    1},
    {"monday",	    1},
    {"tue",	    2},
    {"tues",	    2},
    {"tuesday",	    2},
    {"wed",	    3},
    {"wednesday",   3},
    {"thu",	    4},
    {"thurs",	    4},
    {"thursday",    4},
    {"fri",	    5},
    {"friday",	    5},
    {"sat",	    6},
    {"saturday",    6},
    {NULL,	    0}
};


/*{
** Name: psq_cvtdow	- Convert day of week string to day of week number
**
** Description:
**      This function converts a string standing for a day of the week to a
**	number standing for the same thing.  Days are numbered consecutively,
**	starting with 0 for Sunday, and ending with 6 for Saturday.  If it
**	can't decipher the string, it returns -1.
**
** Inputs:
**      sdow                            String standing for day of week
**
** Outputs:
**	Returns:
**	    0-6 for Sunday-Saturday, or -1 for unknown
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-aug-86 (jeff)
**          written
*/
i4
psq_cvtdow(
	char               *sdow)
{
    register PSQ_DOWNAME *d;
    register char       *s;

    s = sdow;

    for (d = (PSQ_DOWNAME*) Psq_dowlist; d->dow_name != NULL; d++)
	if (!STbcompare(s, 0, d->dow_name, 0, TRUE))
	    return (d->dow_num);

    return (-1);
}

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: IINAP.H - prototype's for export by handy!iinap.c
**
** Description:
**	Export prototypes from handy!iinap.c.  Includes prototypes for:
**	    II_nap()
**	    II_yield_cpu_time_slice()
**
** History:
**      25-apr-94 (mikem)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Forward and/or External function references.
*/

/* declaration of external proc */

FUNC_EXTERN i4 II_nap(
    i4	usecs);
FUNC_EXTERN VOID II_yield_cpu_time_slice(
    i4	pid,
    i4	timeout);

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADUTIMEVECT.H - Define vector for time components.
**
** Description:
**        This file defines a timevect struct for holding components of time.
**
** History:
**      1-may-86 (ericj)
**          Converted for Jupiter.
**	24-oct-89
**	    Added some #defs.  The timevect definition should really be a
**	    typedef; I'll put it on my list.
**      31-dec-1992 
**          Added function prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Jul-2006 (gupsh01)
**	    Added nano seconds.
**/

/*
**   Define constants for tranforming a timevect absolute year spec into an
** INGRES internal date absolute year spec and for transforming a timevect
** absolute month spec into an INGRES internal date absolute month spec.
*/
#define	    AD_TV_NORMYR_BASE		1900
#define	    AD_TV_NORMMON		   1

struct timevect
{
    i4    tm_sec;
    i4    tm_nsec;
    i4    tm_min;
    i4    tm_hour;
    i4    tm_mday;
    i4    tm_mon;
    i4    tm_year;
    i4    tm_wday;
    i4    tm_yday;
    i4    tm_isdst;
};

FUNC_EXTERN VOID adu_cvtime(i4  time,
			    i4  time_nsec_part,
			    struct   timevect    *t);

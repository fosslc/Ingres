/*
** Copyright (c) 1986, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <starlet.h>
#include    <time.h>

/**
**
**  Name: TMGETTM.C - get time
**
**  Description:
**      get time
**
{@func_list@}...
**
**
**  History:    $Log-for RCS$
**      15-sep-86 (seputis)
**          initial creation
[@history_template@]...
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes &
**	   external function references
**      22-Feb-2010 (horda03)
**         Get UTC time (gettimeofday) without reference to Ingres timezone, just in case
**         the VMS and Ingres timezone details aren't equal.
**/


FUNC_EXTERN long int TMconv();


/*{
** Name: TMgettm	- Return the current time in a 'timeb' structure.
**
** Description:
**
**	Use the VMS function to get the time and then stuff 
** 	the appropriate parts of it into the appropriate parts
**	of the 'timeb' struct
**
** Inputs:
**
** Outputs:
**      tm                              timeb structure
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10/4/83 (dd) -- Written for VMS CL
**	12/21/84 (kooi) -- changed timeadr from i4 to unsigned i4
**			since we need unsigned arithmetic when computing
**			millitm.
**      15-sep-86 (seputis)
**          initial creation
**	25-sep-92 (stevet)
**	    Add timezone adjustment using new TMtz routines.  TMconv
**	    no longer returns time in GMT (with timezone adjusted).
[@history_template@]...
*/
VOID
TMgettm(tm)
TMSTRUCT           *tm;
{
    struct timeval tv;

    gettimeofday( &tv, 0);

    tm->time = tv.tv_sec;

    /* get milliseconds since last second.  gettimeofday returns seconds and
    ** microseconds. 1000 microseconds = 1 millisecond.
    */
    tm->millitm = tv.tv_usec / 1000;
    tm->timezone = 0;
    tm->dstflag = 0;
}


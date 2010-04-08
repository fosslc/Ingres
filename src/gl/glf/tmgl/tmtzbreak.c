/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <systypes.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>

/**
**
**  Name: TMTZBREAK.C - Local Timezone Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMtz_break	- break char string for date into pieces
**
**
** History:
**	15-aug-1997 (canor01)
**	    Created from tmbreak.c.
**	19-aug-1997 (canor01)
**	    Correct parameters to TMtz_str. Add whitespace.
**	27-aug-1997 (canor01)
**	    Added st.h.
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**/

/*{
** Name: TMtz_break	- break char string for date into pieces
**
** Description:
**      Break the string created by TMtz_str() into component pieces.  This 
**      routine fills in the following fields of the struct (actually 
**      the pointer to the struct) passed to it: the day-of-the-week,
**      month, day, year, hours, minutes, and seconds.
**
** Inputs:
**      tm                              time to break up
**	tz_cb				pointer to TM_TZ_CB
**
** Outputs:
**      human                           components of time
**	Returns:
**	    FAIL - if tm is invalid
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-1997 (canor01)
**	    Created from tmbreak.c.
**	19-aug-1997 (canor01)
**	    Correct parameters to TMtz_str. Add whitespace.
*/
STATUS
TMtz_break( SYSTIME *tm, PTR tz_cb, struct TMhuman *human )
{
        char    buf[TM_SIZE_STAMP+1];
	char	*cp = buf;

	TMtz_str( tm, tz_cb, buf );

	STncpy( human->wday, cp, 3 );
	human->wday[3]= EOS;
	cp	= cp + 4;
	STncpy( human->month, cp, 3 );
	human->month[3]= EOS;
	cp	= cp + 4;
	STncpy( human->day, cp, 2 );
	human->day[2]= EOS;
	cp	= cp + 3;
	STncpy( human->hour, cp, 2 );
	human->hour[2]= EOS;
	cp	= cp + 3;
	STncpy( human->mins, cp, 2 );
	human->mins[2]= EOS;
	cp	= cp + 3;
	STncpy( human->sec, cp, 2 );
	human->sec[2]= EOS;
	cp	= cp + 3;
	STncpy( human->year, cp, 4 );
	human->year[4]= EOS;

	return ( OK );
}

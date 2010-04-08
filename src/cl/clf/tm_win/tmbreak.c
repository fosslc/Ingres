/*
** Copyright (c) 1987, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <tm.h>
#include    <st.h>

/**
**
**  Name: TMBREAK.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMbreak	- break char string for date into pieces
**
**
** History:
 * Revision 1.1  88/08/05  13:46:46  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:51:52  anton
**      define ctime
**      
**      Revision 1.2  87/11/10  16:03:13  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/*{
** Name: TMbreak	- break char string for date into pieces
**
** Description:
**      Break the string created by TMstr() into component pieces.  This 
**      routine fills in the following fields of the struct (actually 
**      the pointer to the struct) passed to it: the day-of-the-week,
**      month, day, year, hours, minutes, and seconds.
**
** Inputs:
**      tm                              time to break up
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
**      15-sep-86 (seputis)
**	06-jul-87 (mmm)
**	    initial jupiter unix cl.
**      22-sep-92 (stevet)
**          Call TMstr to get the local time in 'DDD MMM YY HH MM SS YYYY'
**          format and pick up the new timezone support.
*/
STATUS
TMbreak(tm, human)
SYSTIME            *tm;
struct TMhuman     *human;
{
        char    buf[TM_SIZE_STAMP+1];
	char	*cp=buf;

	TMstr(tm, buf);
	STlcopy(cp,human->wday,3);
	cp	= cp + 4;
	STlcopy(cp,human->month,3);
	cp	= cp + 4;
	STlcopy(cp,human->day,2);
	cp	= cp + 3;
	STlcopy(cp,human->hour,2);
	cp	= cp + 3;
	STlcopy(cp,human->mins,2);
	cp	= cp + 3;
	STlcopy(cp,human->sec,2);
	cp	= cp + 3;
	STlcopy(cp,human->year,4);

	return(OK);
}

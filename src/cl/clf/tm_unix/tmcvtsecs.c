/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <tm.h>

/**
**
**  Name: TMCVTSECS.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMcvtsecs	- convert seconds to a character string.
**
**
** History:
 * Revision 1.1  88/08/05  13:46:48  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/12/21  12:58:22  mikem
**      return status
**      
**      Revision 1.2  87/11/10  16:03:29  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: TMcvtsecs	- Convert seconds to a string
**
** Description:
**      This routine performs the same functions as TMstr but takes a 
**      number of seconds since Jan 1,1970 as the first argument rather
**      than a pointer to a SYSTIME.
**
**	convert seconds to a character string.  the number of seconds
**	passed in is assumed to be the number of seconds since
**	Jan. 1, 1970.  you must provide the buffer to put the string
**	in and it is assumed the buffer is big enough.  format of
**	the string may vary between systems.
**
** Inputs:
**      secs                            number of seconds
**
** Outputs:
**      buf                             buffer to store the string in
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10/3/83 (dd) VMS CL
**      15-sep-86 (seputis)
**          initial creation
*/
STATUS
TMcvtsecs(secs, buf)
i4            secs;
char               *buf;
{
	struct TMhuman	comptime;
	SYSTIME		tm;

	tm.TM_secs = secs;
	TMbreak(&tm, &comptime);

	/* This STpolycat is in two pieces due to current PYRAMID limit	*/
	/* of 12 parameters for variable argument calls.		*/
	STpolycat(6,	comptime.day,	"-", 
		  	comptime.month,	"-", 
		  	comptime.year,	" ", 
			buf);
	STpolycat(6,	buf,
			comptime.hour,	":", 
		  	comptime.mins,	":", 
		  	comptime.sec,
			buf);

	return(OK);
}

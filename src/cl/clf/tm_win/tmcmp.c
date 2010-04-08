/*
** Copyright (c) 1987, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <tm.h>

/**
**
**  Name: TMCMP.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMcmp	- compare times
**
**
** History:
 * Revision 1.1  88/08/05  13:46:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  16:03:20  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      06-jul-1987 (mmm)
**          Updated for jupiter unix cl.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/*{
** Name: TMcmp	- Compare times
**
** Description:
**	Compare time1 with time2.  Returns positive if time1 greater than time2;
**	zero if time1 equals time2; and negative if time1 less than time2.
**
** Inputs:
**      time1                           first time
**      time2                           second time to compare
**
** Outputs:
**	Returns:
**	    negative if time1 < time2
**	    zero if time1 == time2
**	    positive if time1 > time2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
**	    initial jupiter unix cl.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/
i4
TMcmp(time1,time2)
SYSTIME	*time1;
SYSTIME	*time2;
{
	if (time1->TM_secs == time2->TM_secs)
	{
		return(time1->TM_msecs - time2->TM_msecs);
	}

	return(time1->TM_secs - time2->TM_secs);
}

/*
** Copyright (c) 1997, Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<tm.h>			/* hdr file for TM stuff */

/**
**
**  Name: TMCPU.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMcpu	- return cpu time used by process
**
**
** History:
**	19-apr-95 (emmag)
**	    Desktop porting changes.
**	24-apr-1997 (canor01)
**	    Updated to use GetProcessTimes().  Removed Unix-specific
**	    code.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**/


/*{
** Name: TMcpu	- CPU time used by process
**
** Description:
**      get CPU time used by process
**
** Inputs:
**
** Outputs:
**	Returns:
**	    milli secs used by process so far
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-apr-1997 (canor01)
**	    Updated to use GetProcessTimes().  Removed Unix-specific
**	    code.
**	10-Mar-2005 (drivi01)
**	    Updated millisec calculation to add 2 64-bit numbers instead
**	    of only dwLowDateTime fields to reflect correct process time.
*/
i4
TMcpu()
{
    FILETIME	CreationTime;
    FILETIME	ExitTime;
    FILETIME    KernelTime;
    FILETIME    UserTime;
    bool	retval;
    STATUS	status;
    u_i4        millisec;
    ULARGE_INTEGER kT, uT;

    retval = GetProcessTimes( GetCurrentProcess(),
			      &CreationTime,
			      &ExitTime,
			      &KernelTime,
			      &UserTime );
    if ( retval == 0 )
    {
	status = GetLastError();
	return(0);
    }
 
    /*
    ** Interval measured in 64-bit value of 100-nanoseconds 
    ** 
    */

    kT.LowPart = KernelTime.dwLowDateTime;
    kT.HighPart = KernelTime.dwHighDateTime;
    uT.LowPart = UserTime.dwLowDateTime;
    uT.HighPart = UserTime.dwHighDateTime;
    millisec = (kT.QuadPart + uT.QuadPart) / HUNDREDNANO_PER_MILLI ;

    return ( millisec );
 
}

/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**
**/



#include <unistd.h>

#include <compat.h>
#include <iinap.h>

/*
** Name: II_nap() - sub-second sleep
**
** Description:
**      II_nap() sleeps the specified number of microseconds.  It
**      returns 0 on normal termination and -1 if the function fails
**      to sleep for the specified time.
**
**      Implementation of II_nap() may not be able to provide micro-second
**      resolution.  Implementations using the poll() system call will
**      round micro-second timeouts to the next highest milli-second increment.
**
**      Most OS implementations of select() will also not provide
**      micro-second resolution.  For instance on sun4c machines a
**      1 micro second timeout always results in a minimum of a 10 millisecond
**      sleep.
**
** Inputs:
**      usecs - Number of microseconds to sleep.
**
** Outputs:
**      none
**
**      Returns:
**          -1 on error, 0 otherwise
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-dec-2007 (stegr01)
**          Function initially created for VMS.
**
*/

i4
II_nap(i4 usecs)       /* micro seconds */
{
    i4 sts;

    /* use usleep library function for this */

    sts = usleep ((unsigned int)usecs); /* returns 0 or -1 if error */
    return (sts);
}


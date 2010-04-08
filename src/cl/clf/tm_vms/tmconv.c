/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**
**/

#include <compat.h>

#include <ssdef.h>
#include <stsdef.h>
#include <ints.h>
#include <gen64def.h>

#include <lib$routines>

/*
** unix_epoch below is January 1st 1970 expressed as a VMS time
*/

static int64 unix_epoch = 0x007C95674BEB4000L;

/*
** Name:  TMconv
**
** Description:
**    Convert a quadword VMS time to seconds since Unix epoch
**
** Inputs:
**    i8* vms_time - pointer to quadword vms time
**
** Outputs:
**     None
**
** Returns:
**     Seconds since Unix epoch
**
** Exceptions:
**     None
**
** Side Effects:
**     None
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
TMconv (i8* vms_time)
{
    i4 sts;
    i4 secs;
    i4 remainder;
    i8 rslt_time;

    /* VMS time is expressed in 100 nSec units - so divide by 10**7 to convert to seconds */

    sts = lib$subx ((u_i8 *)vms_time, (u_i8 *)&unix_epoch, (u_i8 *)&rslt_time);
    if (!(sts & STS$M_SUCCESS))
        return (0);
    sts = lib$ediv(&10000000, &rslt_time, &secs, &remainder);
    if (!(sts & STS$M_SUCCESS))
        return (0);;
    return (secs);
}

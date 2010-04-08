/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**      05-feb-2010 (joea)
**          Correct return value to length of string returned by sys$asctim.
**
**/

#include <compat.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <descrip.h>
#include <ssdef.h>
#include <stsdef.h>
#include <ints.h>
#include <gen64def.h>

#include <lib$routines.h>
#include <starlet.h>


/*
** unix_epoch below is January 1st 1970 expressed as a VMS time
*/

static i8 unix_epoch = 0x007C95674BEB4000L;

/*
** Name:  TMtostr
**
** Description:
**    Convert integer time in seconds to a string
**
** Inputs:
**    i4  secs - time in seconds;
**    char* op_str - buffer to receive converted time
**
** Outputs:
**     None
**
** Returns:
**     status
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
TMtostr (i4 secs, char* op_str)
{
    i4 j;
    i4 sts;
    GENERIC_64 vms_time;
    struct dsc$descriptor_s timdsc = {20, DSC$K_DTYPE_T, DSC$K_CLASS_S, op_str};

    /*
    ** clear output string in case of errors
    ** you will note that there is an implicit assumption that the
    ** the caller provided an output  buffer of at least 21 characters
    */

    op_str[0] = 0;

    /* convert time in secs to VMS 100nSec units */


    sts = lib$emul(&secs, &10000000, &0, (int64 *)&vms_time);
    if (!(sts & STS$M_SUCCESS))
       return 0;
    sts = lib$addx ((u_i8 *)&vms_time, (u_i8 *)&unix_epoch, (u_i8 *)&vms_time);
    if (!(sts & STS$M_SUCCESS))
       return 0;

    /*
    ** restrict the o/p buffer length to a max of 20 chars
    ** i.e. lose the trailing ".nn" millisecs of the string
    */

    sts = sys$asctim (&timdsc.dsc$w_length, &timdsc, &vms_time, 0);
    j = (sts & STS$M_SUCCESS) ? timdsc.dsc$w_length : 0;
    op_str[j] = 0;
    return ((sts & STS$M_SUCCESS) ? j : 0);
}

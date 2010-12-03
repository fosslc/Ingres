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
#include <syidef.h>
#include <stsdef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <efndef.h>
#include <gl.h>

#include <lib$routines.h>         /* librtl prototypes         */
#include <starlet.h>              /* system service prototypes */


/*
** Name:  iiCL_increase_fd_table_size
**
** Description:
**    There are no File Descriptors in VMS - so just return
**    the process channelcnt. Since we can't actually change
**    the sysgen parameter channelcnt (it's not a dynamic
**    parameter) the args are irrelevant
**
** Inputs:
**    i4  maximum - N/A
**    i4 increase_fds -  N/A
**
** Outputs:
**     None
**
** Returns:
**     channelcnt
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
**    29-Nov-2010 (frima01) SIR 124685
**	   Added include of gl.h for prototype.
*/


i4
iiCL_increase_fd_table_size(i4 maximum, i4 increase_fds)
{
    i4 sts;
    i4 channelcnt = 0;


    ILE3 itmlst[2];
    IOSB iosb;

    itmlst[0].ile3$w_length       = sizeof(channelcnt);
    itmlst[0].ile3$w_code         = SYI$_CHANNELCNT;
    itmlst[0].ile3$ps_bufaddr     = &channelcnt;
    itmlst[0].ile3$ps_retlen_addr = NULL;
    itmlst[1].ile3$w_length       = 0;
    itmlst[1].ile3$w_code         = 0;

    sts = sys$getsyiw (EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, 0);
    if (sts & STS$M_SUCCESS) sts = iosb.iosb$w_status;
    if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
    return (channelcnt);
}

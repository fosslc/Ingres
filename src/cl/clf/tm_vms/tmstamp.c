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
#include <st.h>

#include <string.h>

#include <ssdef.h>
#include <stsdef.h>
#include <ints.h>
#include <descrip.h>
#include <gen64def.h>

#include <lib$routines.h>
#include <starlet.h>

/*
** Name:  TMcmpstamp
**
** Description:
**    Compare 2 timestamps
**
** Inputs:
**    u_i8* t1 - first timestamp
**    u_i8* t2 - second timestamp
**
** Outputs:
**     None
**
** Returns:
**     -1 - t2 <  t1
**      0 - t2 == t1
**     +1 - t2 >  t1
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
TMcmpstamp(u_i8* t1, u_i8* t2)
{
    i4 sts;
    i8 result;

    sts = lib$sub_times (t2, t1, (u_i8 *)&result); /* t2 - t1 */

    /*
    ** see documentation for lib$sub_times for why equal times
    ** return a result of 1 100nSec tic
    */

    return ((result  < 0) ? -1 :
            (result == 1) ?  0 : 1);
}


/*
** Name:  TMstoa
**
** Description:
**    convert timestamp to ascii string
**
** Inputs:
**    i8* time - quadword VMS time
**
** Outputs:
**    char* op_str - output buffer for ascii time
**
** Returns:
**     None
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


void
TMstoa (i8* time, char* op_str)
{
    i4 sts;
    i4 j;

    /*
    ** there is an implicit assumption here that the
    ** caller provided a buffer of at least 24 chars
    */

    struct dsc$descriptor_s timdsc = {23,
                                      DSC$K_DTYPE_T,
                                      DSC$K_CLASS_S,
                                      op_str};
    sts = sys$asctim (&timdsc.dsc$w_length, &timdsc, (GENERIC_64 *)time, 0);
    j = (sts & STS$M_SUCCESS) ? timdsc.dsc$w_length : 0;
    op_str[j] = 0;
}


/*
** Name:  TMatos
**
** Description:
**    convert string to quadword VMS timestamp
**
** Inputs:
**    char* str - input buffer for ascii time
**
** Outputs:
**    i8* time - quadword VMS time
**
** Returns:
**     None
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
TMatos (char *str, i8* time)
{
    i4 sts;
    struct dsc$descriptor_s timdsc = {STlength(str),
                                      DSC$K_DTYPE_T,
                                      DSC$K_CLASS_S,
                                      str};
    sts = sys$bintim (&timdsc, (GENERIC_64 *)time);
    return (sts);
}


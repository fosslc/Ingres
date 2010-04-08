/*
**
**  Name: CSSPRSM.C - dummy C function for Itanium on VMS
**
**  Description:
**      This file is a dummy function to satisfy the jam
**      build that uses a macro64 file on Alpha
**
**  History:
**      20-mar-2009 (stegr01)
**          Created.
*/

#include <compat.h>

#include <stdbool.h>
#include <starlet.h>

i4 CS_ssprsm (bool save_regs)
{
    /*
    ** KPS stuff should go here for I64 and Alpha
    ** instead of using our own stack switching
    */

    return  (1);
}

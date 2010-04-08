/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**      N.B. no effort is made to include <compat.h>
**      Otherwise we'll finish up redefining "main"
**      Similarly we'll use the compiler defined ints.
**
**/

#include <string.h>

#include <descrip.h>
#include <ssdef.h>

extern void PCexit(int code);
extern void IIcompatmain(struct dsc$descriptor_s* imgdsc, void (*func)());
extern void compatmain();

#ifndef FAIL
#define FAIL 1
#endif

/*
** Frontmain always appears as the first compilation unit in the linker list
** Unlike the Macro variant it will also have the DECC setup done, since the
** compiler, when it sees the signature of main() will insert __main() as the
** first entry point - so __main() becomes the image start address
** __main() will first call decc$main() and then our own main().
**
** frontmain calls IIcompatmain with 2 arguments
**
** [1] the image file name (by descriptor)
** [2] the function compatmain()
**
** IIcompatmain gets the arg list via a lib$get_foreign() call and then calls
** compatmain with this arglist. Callers who use this facilty will include compat.h
** in their own main module - compat.h redefines main as compatmain
*/



/*
** Name:  main
**
** Description:
**    Main entry point for images
**
** Inputs:
**    int   argcnt - argument count;
**    char* argv[] - argument list
**
** Outputs:
**     None
**
** Returns:
**     SS$_ABORT if non-normal exit
**
** Exceptions:
**     None
**
** Side Effects:
**      Calls IIcompatmain then exits
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

int main (int argc, char* argv[])
{
    /*
    ** the first arg provided by sys$imgsta() is always the image file descriptor
    */

    struct dsc$descriptor_s imgdsc = {strlen(argv[0]), DSC$K_DTYPE_T, DSC$K_CLASS_S, argv[0]};

    IIcompatmain(&imgdsc, compatmain);

    /*
    ** IIcompatmain should never return.  If it does, we'll call PCexit(FAIL)
    */

    PCexit (FAIL);
    return (SS$_ABORT);
}

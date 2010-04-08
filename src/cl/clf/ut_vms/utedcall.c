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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <varargs.h>

#include <ssdef.h>
#include <stsdef.h>
#include <chfdef.h>
#include <descrip.h>
#include <libwaitdef.h>

#include <lib$routines.h>
#include <starlet.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


typedef struct dsc$descriptor_s DESCRIP;


static i4 UTed_handler(struct  chf$signal_array *sig, struct  chf$mech_array *mech);


/*
** Name:  UTedcall
**
** Description:
**    Call an editor
**
** Inputs:
**    i4 (*func)() - editor function;
**    u_i4 arglst[] - functions arg list
**
** Outputs:
**     None
**
** Returns:
**     i4 sts
**
** Exceptions:
**     None
**
** Side Effects:
**      Calls some editor
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

i4
UTedcall (i4 (*func)(), u_i4 arglist[])
{
    bool trapping_ctrlc;
    i4   sts;

    /*
    ** establish an exception handler - if the editor
    ** signals an error then this handler will unwind
    ** to us with the appropriate condition value
    */

    lib$establish(UTed_handler);

    /*
    ** if we're currently trapping ^C then disallow it temporarily
    */

    trapping_ctrlc = IIctrlc_is_trapped();
    if (trapping_ctrlc) IInotrap_ctrlc();

    /* call the editor */

    sts = lib$callg (arglist, func);
    if (trapping_ctrlc) IItrap_ctrlc();

    /*
    ** if there was an error then the editor probably didn't
    ** output an error message - so we'll do it anyway
    */

    if (!(sts & STS$M_SUCCESS))
    {
        /*
        ** hi  order word = flags (0xF)
        ** low order word = argcnt (1)
        */

        u_i4 msgvec[2] = {0x000F0001, sts};

        sys$putmsg(msgvec, 0, 0, 0);
        lib$wait (&2.0F, &LIB$K_IEEE_S);
    }

    return (sts);
}


/*
** Name:  UTed_handler
**
** Description:
**    Exception handler for editor function
**
** Inputs:
**    struct  chf$signal_array *sig - signal array
**    struct  chf$mech_array   *mech - mechanism array
**
** Outputs:
**     None
**
** Returns:
**     i4 sts
**
** Exceptions:
**     None
**
** Side Effects:
**      Calls some editor
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static i4
UTed_handler (struct  chf$signal_array *sig, struct  chf$mech_array *mech)
{
    i4 sts;
    i4 sts_unwind = SS$_UNWIND;
    if (lib$match_cond ((u_i4 *)&sig->chf$is_sig_name, (u_i4 *)&sts_unwind)) return (SS$_RESIGNAL);
    mech->chf$q_mch_savr0 = sig->chf$is_sig_name;

    mech->chf$q_mch_args |= 0x000F0000L;
    sys$putmsg(mech, 0, 0, 0);
    lib$wait (&2.0F, &LIB$K_IEEE_S);

    /*
    ** unwind the stack to the call frame of the procedure that called the procedure
    ** that established the condition handler that is calling the $UNWIND service
    */

    sts = sys$unwind (NULL, NULL);
    return (sts); /* should be SS$_NORMAL */
}


/*
** Name:  UTfind_image_symbol
**
** Description:
**    Locate function address in RTL
**
** Inputs:
**    struct dsc$descriptor_s fildsc - RTL file name
**    struct dsc$descriptor_s symdsc - symbol name
**    i4*                            - address to return symbol value
**    struct dsc$descriptor_s imgdsc - imagename descriptor
**    i4                             - flags
**
** Outputs:
**     None
**
** Returns:
**     i4 sts
**
** Exceptions:
**     None
**
** Side Effects:
**      Calls some editor
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
UTfind_image_symbol (va_alist)
va_dcl
{
    i4 arg_cnt = 0;
    DESCRIP* fildsc;
    DESCRIP* symdsc;
    DESCRIP* imgdsc;
    i4 *     symval;
    i4       flags;




    va_count(arg_cnt);

    va_list ap;
    va_start(ap);


    fildsc = va_arg(ap, DESCRIP*);
    symdsc = va_arg(ap, DESCRIP*);
    symval = va_arg(ap, i4*);

    imgdsc = (arg_cnt >= 4) ? va_arg(ap, DESCRIP*) : NULL;
    flags  = (arg_cnt >= 5) ? va_arg(ap, i4)       : 0;


    va_end(ap);

    /*
    ** lib$find_image_symbol may signal errors as well as returning them
    ** so convert the signals to a return sts
    */

    lib$establish (lib$sig_to_ret);

    return (lib$find_image_symbol (fildsc, symdsc, symval, imgdsc, flags));
}


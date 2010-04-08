/*
** Copyright (c) 2008 Ingres Corporation
**
** EXGETCTX.C -- Replacement for .mar and .m64 variants.
**
** 10-Dec-2008 (stegr01)
**    Created as a replacement for assembler files for Itanium.
**
*/

#include <compat.h>

#include <ssdef.h>
#include <stsdef.h>
#include <builtins.h>
#include <libicb.h>
#include <lib$routines.h>

/*
** Name:  EXgetctx
**
** Description:
**    Set the return address as the PC for the caller
**    in the Invocation context block
**
** Inputs:
**    INVO_CONTEXT_BLK* icb - Address of the Invocation context block
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

#define INVO_FAILURE 0

i4
EXgetctx (INVO_CONTEXT_BLK* icb)
{
    i4   sts;
    u_i8 ret_pc;

#if defined(axm_vms)

    /* get the current context */
    
    lib$get_curr_invo_context(icb);

    /* and next the caller's context */

    sts = lib$get_prev_invo_context(icb);
    if (sts == INVO_FAILURE)
        return (FAIL);

    /* and set the caller's PC to that frame's PC */

    icb->libicb$q_program_counter = __RETURN_ADDRESS();

#elif defined(i64_vms)

    /* Itanium requires octaword alignment for ICB */

    if (((i4)icb & 0xFFFFFFF0) != (i4)icb)
       lib$signal (SS$_BUFNOTALIGN);

    /* get the caller's PC */

    ret_pc = __RETURN_ADDRESS();

    /* The ICB requires initialising for IA64, before we can use it */ 

    sts = lib$i64_init_invo_context(icb, LIBICB$K_INVO_CONTEXT_VERSION, 0);
    if (sts == INVO_FAILURE)
        return (FAIL);

    /* get this frame's Invocation context */

    lib$i64_get_curr_invo_context(icb);
    if (icb->libicb$l_alert_code != LIBICB$K_AC_OK)
        return (FAIL);

    /* and now the caller's context */

    sts = lib$i64_get_prev_invo_context(icb);
    if (sts == INVO_FAILURE)
        return (FAIL);

    /* and set the caller's PC in the context block */

    lib$i64_set_pc(icb, &ret_pc);
#endif

    return (OK);
}

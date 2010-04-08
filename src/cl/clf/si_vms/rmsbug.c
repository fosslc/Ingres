/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**      24-nov-2009 (stegr01)
**      Change maximum DECC version from 7.2 to 7.3
**
**/


/*
** none of this will work if we allow the compiler to optimise stuff away
*/

#pragma optimize level=0

/*
** The minimum version of the DECC compiler for Itanium must be 7.2
** And we should check that the compiler is using R45 to save the
** address of the ctrlc_seen stack local variable
** draw this requirement to the user's attention for newer versions
** maintainers should then adjust the version check
**
*/

#ifdef __ia64
#if __DECC_VER   < 70290001 /* Minimum DECC version is 7.2 */
#pragma message save
#pragma message fatal (ALL)
#pragma message ("Minimum DECC compiler version for Itanium's is 7.2 ")
#pragma message restore
#elif __DECC_VER > 70390018 /* Maximum DECC version is 7.3 */
#pragma message save
#pragma message fatal (ALL)
#pragma message ("Maximum DECC compiler version for Itanium's is 7.3 ")
#pragma message restore
#endif
#endif

#pragma message enable (UNUSED)

#ifndef __NEW_STARLET
#define __NEW_STARLET
#endif


#include <compat.h>

#include <stdbool>
#include <stdlib>
#include <string>
#include <stdio>
#include <assert>

#include <ssdef>
#include <stsdef>
#include <rms>
#include <chfdef>
#include <libicb>
#include <ints>

#if defined(__ia64)
#include <fdscdef>
#elif defined(__alpha)
#include <pdscdef>
#endif

#include <lib$routines>
#include <starlet>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifdef __alpha
#define FP_REG_NUM 29
#define SP_REG_NUM 30
#endif


/*
** force the compiler to preserve our known argument registers
** then we can fish out the args when we're walking the stack
*/

#if defined(__ia64)
#pragma     linkage_ia64 rms_common_lnk = (preserved (r4, r5, r6), parameters (r4,r5,r6))
#elif defined(__alpha)
#pragma     linkage      rms_common_lnk = (preserved (r16,r17,r18))
#endif
#pragma use_linkage rms_common_lnk (rms_common)

typedef i4 (*RMS_FUNC)(struct RAB *);


i4 rms_get (struct RAB *rab);
i4 rms_put (struct RAB *rab);

static i4 rms_common (RMS_FUNC rms_func, struct RAB *rab, bool *ctrlc_seen);
static i4 ctrlc_handler(struct  chf$signal_array *sig, struct  chf$mech_array *mech);

/*
** for Itaniums we need to walk the stack and identify which
** frame holds the reference to the local stack variable ctrlc_seen
** however, unlike the Alpha, we can't get the procedure descriptor
** from the ICB - only the return address to the procedure
** so we save the Function descriptors here so we can identify
** the PC range
*/

#ifdef __ia64
static FDSCDEF* fdsc_rms_get = (FDSCDEF *)rms_get;
static FDSCDEF* fdsc_rms_put = (FDSCDEF *)rms_put;
static FDSCDEF* fdsc_rms_cmn = (FDSCDEF *)rms_common;
#endif

/*
** the following 3 functions must remain in this order ...
**
** [1] rms_get()
** [2] rms_put()
** [3] rms_common()
**
** The address of the local stack variable ctrlc_seen
** is passed to rms_common solely so we can find it as a
** saved argument when we walk the stack in the controlc_handler
**
** A combination of no-optimisation and linkages allows us to
** find the address of the variable reliably
**
*/

i4
rms_get (struct RAB *rabaddr)
{
    i4 sts;
    bool ctrlc_seen = FALSE;
    sts = rms_common((RMS_FUNC)sys$get, rabaddr, &ctrlc_seen);
    if (ctrlc_seen) lib$signal(SS$_CONTROLC);
    return (sts);
}

i4
rms_put (struct RAB *rabaddr)
{
    i4 sts;
    bool ctrlc_seen = FALSE;
    sts = rms_common((RMS_FUNC)sys$put, rabaddr, &ctrlc_seen);
    if (ctrlc_seen) lib$signal(SS$_CONTROLC);
    return (sts);
}


i4
rms_common (RMS_FUNC rms_func, struct RAB *rab, bool *ctrlc_seen)
{
    i4 sts = RMS$_SUC;
    lib$establish (ctrlc_handler);

#ifdef TEST_ME
    signal_ctrlc();
#else
    sts = (*rms_func)(rab);
#endif
    return (sts);
}

i4
ctrlc_handler(struct  chf$signal_array *sig, struct  chf$mech_array *mech)
{
    i4 sts = SS$_RESIGNAL;
    i4 sts_ctrlc = SS$_CONTROLC;
    if (lib$match_cond ((u_i4 *)&sig->chf$is_sig_name, (u_i4 *)&sts_ctrlc))
    {
        INVO_CONTEXT_BLK icb;
        bool* ctrlc_seen = NULL;

#if defined(__ia64)

        /* make absolutely sure we can find the rms_common procedure reliably */

        assert (fdsc_rms_get->fdsc$q_entry < fdsc_rms_put->fdsc$q_entry);
        assert (fdsc_rms_put->fdsc$q_entry < fdsc_rms_cmn->fdsc$q_entry);
        sts = lib$i64_init_invo_context (&icb, LIBICB$K_INVO_CONTEXT_VERSION, 0);
        if (!sts) lib$signal (SS$_BADPARAM);
        lib$i64_get_curr_invo_context (&icb);
#elif defined(__alpha)
        lib$get_curr_invo_context (&icb);
#endif

        do
        {
#if defined(__ia64)
            void* ret_pc;
            sts = lib$i64_get_prev_invo_context(&icb);
            ret_pc = (void *)icb.libicb$ih_return_pc;
            if (ret_pc > fdsc_rms_cmn->fdsc$l_entry) continue;
            if (ret_pc < fdsc_rms_get->fdsc$l_entry) continue;

            /*
            ** found the stack frame for rms_common
            ** its ret_pc will lie between the entry point for
            ** rms_get and the start PC for rms_common
            ** args 1-3 are stored in stacked registers R43-R45
            ** and we want the third arg (R45)
            ** (R40-R47 are defined as stacked local registers where
            ** the compiler can save preserved arguments)
            **
            ** don't ask why it's R45 - it just so happens that that is
            ** how the compiler decided to save them - at least it's
            ** consistent about it - as long as you don't change the
            ** code in rms_common()!
            */

            ctrlc_seen = (bool *)icb.libicb$ih_ireg[45];
            break;
#elif defined (__alpha)
            PDSCDEF* pdsc;
            char* fp;
            char* sp;
            char* base = NULL;
            char* rsa  = NULL;
            u_i8* pReg;
            i4    rn;

            sts = lib$get_prev_invo_context(&icb);
            /* returns 0 = bottom of chain, 1 = ok */

            /*
            ** Life is a bit simpler on Alphas - we can identify
            ** the ICB for the rms_common call frame directly
            ** From there we can look into the Register Save Area
            ** and recover the address of the stack local variable
            ** ctrlc_seen as the 3rd saved argument
            ** Note that we only cater for stack frame procedures
            **
            */

            pdsc = (PDSCDEF *)icb.libicb$ph_procedure_descriptor;
            if (pdsc != (PDSCDEF *)rms_common) continue;
            fp = (char *)icb.libicb$q_ireg[FP_REG_NUM];
            sp = (char *)icb.libicb$q_ireg[SP_REG_NUM];

            /*
            ** the layout of the RSA (in quadwords) is
            **
            ** +-----------------------+
            ** !  saved_return (R26)   ! : rsa + 0
            ** +-----------------------+
            ** !  1st saved int reg    ! : rsa + 8
            ** +-----------------------+
            ** !                       !
            ** >                       >
            ** <                       <
            ** !                       !
            ** +-----------------------+
            ** !  Nth saved int reg    !
            ** +-----------------------+
            **
            ** Alpha i/p args are in R16-R21
            ** We expect our argument in R18 since it's the 3rd one
            */

            if (!(pdsc->pdsc$v_kind == PDSC$K_KIND_FP_STACK)) break;
            base = (pdsc->pdsc$v_base_reg_is_fp) ? fp : sp;
            rsa = base + pdsc->pdsc$w_rsa_offset;
            pReg = (u_i8 *)rsa;
            for (rn=0; rn<=30; rn++)
            {
                if (pdsc->pdsc$l_ireg_mask & (1<<rn)) pReg++;
                if (rn != 18) continue;
                ctrlc_seen = *(bool **)pReg;
                break;
            }
            break;
#endif
        } while (!icb.libicb$v_bottom_of_stack);

        if (ctrlc_seen) *ctrlc_seen = TRUE;

    }
    return (sts);
}


#ifdef TEST_ME
static i4 signal_ctrlc (void);
i4 main (i4 argc, char* argv[])
{
    i4 sts;
    char* filnam = "sys$scratch:test.tmp";
    struct FAB fab = cc$rms_fab;
    struct RAB rab = cc$rms_rab;

    fab.fab$b_fns = strlen(filnam);
    fab.fab$l_fna = filnam;
    fab.fab$b_org = FAB$C_SEQ;
    fab.fab$l_fop = FAB$M_SQO;
    fab.fab$b_fac = FAB$M_PUT;
    fab.fab$b_rfm = FAB$C_VAR;
    fab.fab$b_rat = FAB$M_CR;
    fab.fab$l_fop = FAB$M_CIF;
    fab.fab$w_mrs = 132;

    rab.rab$l_rbf = filnam;
    rab.rab$w_rsz = strlen(filnam);
    rab.rab$b_rac = RAB$C_SEQ;
    rab.rab$l_fab = &fab;

    sts = sys$create(&fab);
    if (!(sts & STS$M_SUCCESS)) lib$signal(sts);
    sts = sys$connect(&rab);
    if (!(sts & STS$M_SUCCESS)) lib$signal(sts);
    sts = rms_put (&rab);
    if (!(sts & STS$M_SUCCESS)) lib$signal(sts);
    sts = sys$close(&fab);
}


static i4 signal_ctrlc ()
{
    lib$signal (SS$_CONTROLC);
    return (SS$_NORMAL);
}
#endif

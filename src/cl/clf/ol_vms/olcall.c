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


#include <ints.h>
#include <assert.h>
#include <psigdef.h>

#include <lib$routines.h>


#pragma message disable(ELLIPSEARG)
#ifdef i64_vms
#pragma message disable(SHOWMAPLINKAGE)
#endif

/*
** It is essential that this code is not optimised
** otherwise populating the floating point registers
** will be optimised away by the compiler
** And just to be sure we don't rely on a command line
** switch to do this - we'll use the pragma instead
*/

#pragma optimize level=0

/* The AI register number may not be defined in old headers */

#ifndef AI$K_REGNO
#define AI$K_REGNO 25
#endif


typedef struct _ARG_ENTRY {
union {
    f8       d;
    i8       i;
    i4*      pi;
    void*    pv;
    f8*      pd;
  } r_overlay;
} ARG_ENTRY;


#define i_   r_overlay.i
#define d_   r_overlay.d
#define pi_  r_overlay.pi
#define pv_  r_overlay.pv
#define pd_  r_overlay.pd


/*
** Alphas   park the first six   args in registers
** Itaniums park the first eight args in registers
*/



#ifdef i64_vms
#define MAX_ARG_REGISTERS 8
#elif  axm_vms
#define MAX_ARG_REGISTERS 6
#endif


static i8 getAI (i8);
static f8 getF0 (f8);
static f8 setF1 (f8);
static f8 setF2 (f8);
static f8 setF3 (f8);
static f8 setF4 (f8);
static f8 setF5 (f8);
static f8 setF6 (f8);
static f8 setF7 (f8);
static f8 setF8 (f8);
static i8 setAI (i8);

#pragma     linkage getf0_lnk    = (result(f0))
#pragma     linkage getai_lnk    = (parameters (r25), result(r0),  notneeded(ai))
#pragma     linkage setai_lnk    = (parameters (r16), result(r25), nopreserve(r25))

/*
** Floating args 1-6 should appear in Alpha registers f16-f21 (6)
*/

#ifdef axm_vms
#pragma     linkage setf1_lnk   = (parameters (f16), result(f16))
#pragma     linkage setf2_lnk   = (parameters (f16), result(f17))
#pragma     linkage setf3_lnk   = (parameters (f16), result(f18))
#pragma     linkage setf4_lnk   = (parameters (f16), result(f19))
#pragma     linkage setf5_lnk   = (parameters (f16), result(f20))
#pragma     linkage setf6_lnk   = (parameters (f16), result(f21))
#pragma     linkage setf7_lnk   = (parameters (f16), result(f22)) /* unused on alpha */
#pragma     linkage setf8_lnk   = (parameters (f16), result(f23)) /* unused on alpha */
#endif

/*
** Floating args 1-8 should appear in Itanium registers f8-f15 (8)
*/

#ifdef i64_vms
#pragma     linkage_ia64 setf1_lnk   = (parameters (f8), result(f8))
#pragma     linkage_ia64 setf2_lnk   = (parameters (f8), result(f9))
#pragma     linkage_ia64 setf3_lnk   = (parameters (f8), result(f10))
#pragma     linkage_ia64 setf4_lnk   = (parameters (f8), result(f11))
#pragma     linkage_ia64 setf5_lnk   = (parameters (f8), result(f12))
#pragma     linkage_ia64 setf6_lnk   = (parameters (f8), result(f13))
#pragma     linkage_ia64 setf7_lnk   = (parameters (f8), result(f14))
#pragma     linkage_ia64 setf8_lnk   = (parameters (f8), result(f15))
#endif


#pragma use_linkage getf0_lnk    (getF0)
#pragma use_linkage getai_lnk    (getAI)
#pragma use_linkage setai_lnk    (setAI)

#pragma use_linkage setf1_lnk    (setF1)
#pragma use_linkage setf2_lnk    (setF2)
#pragma use_linkage setf3_lnk    (setF3)
#pragma use_linkage setf4_lnk    (setF4)
#pragma use_linkage setf5_lnk    (setF5)
#pragma use_linkage setf6_lnk    (setF6)
#pragma use_linkage setf7_lnk    (setF7)
#pragma use_linkage setf8_lnk    (setF8)

#pragma inline (setF1,setF2,setF3,setF4,setF5,setF6,setF7,setF8)

/*
** Name:  OLcall
**
** Description:
**    Call user defined functions
**
** Inputs:
**     func      - ptr to extern function to be called
**     olcnt     - length of arglist
**     olclass[] - type of each arg
**                 1 = double
**                 0 = anything else
**     arglist[] - argument list for func()
**                 doubles are passed in 2 longwords
**                 everything else in 1
**     oltype    - type of return value
**                 2 = double
**                 1 = int, by ref, desc addr etc
**                 0 = void
**     retptr    - ptr to where to put the return value
**
** Outputs:
**     None
**
** Returns:
**     None
**
** Exceptions:
**     None
**
** Side Effects:
**      None
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/



void
OLcall (void (*func)(), i4 olcnt, i4 olclass[], i4 arglist[], i4 oltype, void* retptr)
{
    i4        i,j,ri;
    ARG_ENTRY al[256] = {0};
    i4*       pArgs = arglist;
    AIDEF     ai = {0};


    f8 (*f8callg)()  = (f8 (*)())lib$callg_64;
    i4 (*i4callg)()  = (i4 (*)())lib$callg_64;



    /*
    ** Both Alpha and Itanium architectures pass the first 6 (alpha) or 8 (Itanium)
    ** arguments in registers.
    ** Ints/pointers etc. in R16-R21 (Alpha) or R32-R39 (itanium).
    ** floats/doubles etc.in F16-F21 (Alpha) or F8-F15  (itanium).
    ** (note that we don't handle complex types that would require 2 registers)
    ** However we can't ask the compiler to pass an argument in via a float register
    ** without specifying a function prototype. This would lead to having to define
    ** 8! (~40,000) prototypes to cover all arrangements of the int/float args.
    ** Instead we'll exploit the compilers linkage pragmas to preload the appropriate
    ** floating registers before we call the user function. We know that these float
    ** registers will never be touched between our calls. It will then be up to the user
    ** function to interpret them appropriately. Note that no attempt is made to setup
    ** the AI register to indicate the argument type. This doesn't seem to be necessary
    ** since the target function must know which register to fetch its arguments from.
    ** In this case we don't require any function prototypes and we don't care how long
    ** the arg list is since args 7 (alpha) or 9 (itanium) -> 255 will be in memory (on the stack)
    **
    ** IMPORTANT WARNING !!! - this only works if the code has not been optimised
    ** This is enforced by using #prgama optimise level=0 in this compilation unit
    **
    ** We can't exploit the PDSC's version of argument types since some user functions
    ** may be using stdarg variable length arg lists.
    **
    ** We shouldn't really bother setting up the AI register since ...
    **
    ** [1] the only language that we support for passing int/floats by value is C
    ** [2] lib$callg(_64) treats the arglist as ints/pointers and sets up the AI register
    **     accordingly  - so there's no point in us setting it up if it's going to get
    **     trampled on by lib$callg
    **
    ** we only need to setup the (float) registers for the first 6 (alpha)
    ** or 8 (itanium) arguments
    **
    */

    ai.ai$b_arg_count = (u_char)al[0].i_;
    assert (AI$K_AR_I64 == 0);
    assert (AI$K_REGNO  == 25);

    al[0].i_ = olcnt;

    for (i=0; i<olcnt; i++)
    {
        if (olclass[i]) /* 1 = double, 0 = anything else */
        {
            al[i+1].d_ = *(f8 *)pArgs++;
        }
        else
        {
            al[i+1].i_ = *pArgs;
        }
        pArgs++;
    }

    j = min((i4)al[0].i_, MAX_ARG_REGISTERS);
    for (i=1; i<=j; i++)
    {
        if (olclass[i-1]) /* 1 = double, 0 = anything else */
        {
            switch (i)
            {
                case 1: setF1(al[i].d_);   break;
                case 2: setF2(al[i].d_);   break;
                case 3: setF3(al[i].d_);   break;
                case 4: setF4(al[i].d_);   break;
                case 5: setF5(al[i].d_);   break;
                case 6: setF6(al[i].d_);   break;
                case 7: setF7(al[i].d_);   break;
                case 8: setF8(al[i].d_);   break;
            }
        }

        ri = (olclass[i-1]) ? AI$K_AR_FS : AI$K_AR_I64;
        ai.ai$v_arg_reg_info |= ri<<(i-1)*3;
    }



    /* now call the user supplied function */


    switch (oltype)
    {
        case 0:   /* void */      (* i4callg)((u_i8 *)al, func);   break;
        case 1 :  *(i4 *)retptr = (* i4callg)((u_i8 *)al, func);   break;
        case 2:   *(f8 *)retptr = (* f8callg)((u_i8 *)al, func);   break;
    }

}


static f8 getF0 (f8 p0) {return (p0);}
static i8 getAI (i8 ai) {return (ai);}
static f8 setF1 (f8 p1) {return (p1);} /* alpha and Itanium */
static f8 setF2 (f8 p2) {return (p2);} /* alpha and Itanium */
static f8 setF3 (f8 p3) {return (p3);} /* alpha and Itanium */
static f8 setF4 (f8 p4) {return (p4);} /* alpha and Itanium */
static f8 setF5 (f8 p5) {return (p5);} /* alpha and Itanium */
static f8 setF6 (f8 p6) {return (p6);} /* alpha and Itanium */
static f8 setF7 (f8 p7) {return (p7);} /* Itaniun only */
static f8 setF8 (f8 p8) {return (p8);} /* Itanium only */
static i8 setAI (i8 ai) {return (ai);}


/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: astjacket.c - AST management within a pthread environment.
**
** Description:
**   astjacket.h redefines all system services capable of issuing
**   ASTs to ingres jacket functions that intercept the AST.
**   e.g. sys$setimr is refined to ii_sys$setimr. This module
**   contains all the ii_sys$xxxxxx functions
**
**
** History:
**  18-Jun-2007 (stegr01)
*/

#include <compat.h>

#ifdef OS_THREADS_USED

#include <stdlib.h>
#include <stdio.h>
#include <types.h>


#include <astmgmt.h>

#include <ssdef.h>
#include <stsdef.h>
#include <iledef.h>
#include <jpidef.h>
#include <efndef.h>
#include <ints.h>
#include <gen64def.h>
#include <iosadef.h>
#include <iosbdef.h>
#include <utcblkdef.h>
#include <acmedef.h>
#include <pthread.h>
#include <builtins.h>
#include <starlet.h>


/*
** don't bother with a full prototype since we don't want to pollute
** this file's namespace with all the CL header files definitions (yet!)
*/

extern void IICSMTget_ast_ctx();

static AST_CTX* initAstCtx (ui2 ssrv, AST_RTN astadr, AST_PRM astprm, u_i4 ret_pc);
static void     init_jacket(void);


static i4  jkt_lock          = 0;
static i4  jkt_multithreaded = FALSE;
static i4  jkt_initialised   = FALSE;

/*
** the default state of the jacket is don't diddle
** around with ASTs - just call the system service
** directly
*/

static i4  use_ast           = TRUE;

/*
** the default state of AST handling is to
** execute them on the AST dispatcher thread
** rather than queue them to the issuing thread's
** SCB for execution in CSMTsuspend
*/
static i4  use_scb           = FALSE;

#pragma nostandard



/* These prototypes lifted directly from starlet.h */


/*
** Name:  ii_sys$xxxxxx
**
** Description:
**    Replacment calls for asynchronous system services that support ASTs
**    (redefined in astjacket.h)
**    N.B. this applies only to the asynch version
**    e.g. sys$getjpi (NOT sys$getjpiw)
**    we don't really expect any rational persion to specify an AST for
**    for a synchronous service (with the exception of sys$enqw using
**    a blocking AST)
**
** Inputs:
**    As per the original system service
**
** Outputs:
**    As per the original system service
**
**  Returns:
**    System service status
**
**  Exceptions:
**    None.
**
** Side Effects:
**    If the user specifies an AST address and parameter, this will be
**    substituted by the function IIMISC_ast_jacket() whose argument is the
**    address of an ASTCTX block. This block will hold (among other
**    things) the user's original AST and parameter.
**    The user's AST will either be executed on the AST dispatcher thread
**    if there is no SCB or executed on the issuer's thread, by queueing
**    the context block to the SCB and issuing a resume to the target thread
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
ii_sys$wake(ui4 *pidadr, void *prcnam)
{
    i4  sts;
    sts = sys$wake(pidadr, prcnam);
    return (sts);
}

i4
ii_sys$hiber()
{
    i4  sts;
    sts = sys$hiber();
    return (sts);
}

i4
ii_sys$schdwk(ui4 *pidadr, void *prcnam, GENERIC_64* daytim, GENERIC_64* reptim)
{
    i4  sts;
    sts = sys$schdwk(pidadr, prcnam, daytim, reptim);
    return (sts);
}


i4
ii_sys$brkthru(u_i4  efn,
                   void *msgbuf,
                   void *sendto,
                   u_i4  sndtyp,
                   struct _iosb *iosb,
                   u_i4  carcon,
                   u_i4  flags,
                   u_i4  reqid,
                   u_i4  timout,
                   void (*astadr)(__unknown_params),
                   i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_BRKTHRU, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$brkthru(efn,msgbuf, sendto, sndtyp, iosb, carcon, flags, reqid, timout, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);


    return (sts);
}

i4
ii_sys$check_privilege(u_i4  efn,
                           struct _generic_64 *prvadr,
                           struct _generic_64 *altprv,
                           u_i4  flags,
                           void *itmlst,
                           u_i4  *audsts,
                           void (*astadr)(__unknown_params),
                           i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_CHECK_PRIVILEGE, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$check_privilege(efn, prvadr, altprv, flags, itmlst, audsts, _astadr, _astprm);
    return (sts);
}


i4
ii_sys$dclast(void (*astadr)(__unknown_params),
                  unsigned __int64 astprm,
                  u_i4  acmode)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;


    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_DCLAST, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$dclast(_astadr, _astprm, acmode);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$enq(u_i4  efn,
               u_i4  lkmode,
               void *lksb,
               u_i4  flags,
               void *resnam,
               u_i4  parid,
               void (*astadr)(__unknown_params),
               unsigned __int64 astprm,
               void (*blkast)(__unknown_params),
               u_i4  acmode,
               u_i4  rsdm_id,
              __optional_params)
{
    i4  sts;

    AST_RTN _blkast = (AST_RTN)blkast;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx1 = NULL;
    AST_CTX* ctx2 = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx1 = initAstCtx(CTX$C_SYS_ENQ, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx1 == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx1;
    }

    if ((blkast != NULL) && !use_ast)
    {
        ctx2 = initAstCtx(CTX$C_SYS_ENQ, (AST_RTN)blkast, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx2 == NULL) return (SS$_INSFMEM);
        _blkast = (AST_RTN)IIMISC_ast_jacket;
    }


    sts = sys$enq(efn, lkmode, lksb, flags, resnam,  parid, _astadr, _astprm, _blkast, acmode, rsdm_id);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx1);
    if (!(sts & STS$M_SUCCESS) && blkast && !use_ast) IIMISC_freeAstCtx(ctx2);
    return (sts);
}

i4
ii_sys$enqw(u_i4  efn,
                u_i4  lkmode,
                void *lksb,
                u_i4  flags,
                void *resnam,
                u_i4  parid,
                void (*astadr)(__unknown_params),
                unsigned __int64 astprm,
                void (*blkast)(__unknown_params),
                u_i4  acmode,
                u_i4  rsdm_id,
               __optional_params)
{
    i4  sts;
    AST_RTN _blkast = (AST_RTN)blkast;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx1 = NULL;
    AST_CTX* ctx2 = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx1 = initAstCtx(CTX$C_SYS_ENQW, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx1 == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx1;
    }

    if ((blkast != NULL) && !use_ast)
    {
        ctx2 = initAstCtx(CTX$C_SYS_ENQ, (AST_RTN)blkast, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx2 == NULL) return (SS$_INSFMEM);
        _blkast = (AST_RTN)IIMISC_ast_jacket;
    }


    sts = sys$enqw(efn, lkmode, lksb, flags, resnam,  parid, _astadr, _astprm, _blkast, acmode, rsdm_id);
    return (sts);
}

i4
ii_sys$getdvi(u_i4  efn,
                  unsigned i2  chan,
                  void *devnam,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm,
                  struct _generic_64 *nullarg,
                  __optional_params)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETDVI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getdvi(efn, chan, devnam, itmlst, iosb, _astadr, _astprm, nullarg);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);

    return (sts);
}

i4
ii_sys$getlki(u_i4  efn,
                  u_i4  *lkidadr,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm,
                  u_i4  reserved)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETLKI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getlki(efn, lkidadr, itmlst, iosb, _astadr, _astprm, reserved);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$getjpi(u_i4  efn,
                  u_i4  *pidadr,
                  void *prcnam,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETJPI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getjpi(efn, pidadr, prcnam, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$getqui(u_i4  efn,
                  unsigned i2  func,
                  u_i4  *context,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETQUI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getqui(efn, func, context, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$getrmi(u_i4  efn,
                  u_i4  nullarg1,
                  u_i4  nullarg2,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETRMI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getrmi(efn, nullarg1, nullarg2, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$getsyi(u_i4  efn,
                  u_i4  *csidadr,
                  void *nodename,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETSYI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getsyi(efn, csidadr, nodename, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$qio(u_i4  efn,
               unsigned i2  chan,
               u_i4  func,
               struct _iosb *iosb,
               void (*astadr)(__unknown_params),
               __int64 astprm,
               void *p1,
               __int64 p2,
               __int64 p3,
               __int64 p4,
               __int64 p5,
               __int64 p6)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_QIO, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr   = (AST_RTN)IIMISC_ast_jacket;
        _astprm   = (AST_PRM)ctx;


        /*
        ** if P6 was specified then this qio has a timer associated with it
        ** and P6 is the reqidt of the timer. The AST dispatcher thread will
        ** issue a cancel for this timer before requeueing the "ast" to the
        ** issuer's thread
        */

        if ((u_i4)p6)
        {
            ctx->flags.ctx$v_cantim = TRUE;
            ctx->qio_reqidt = (u_i4)p6;
            p6 = 0;
        }
    }
    sts = sys$qio(efn, chan, func, iosb, _astadr, _astprm, p1, p2, p3, p4, p5, p6);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$setast(char enbflg)
{
    i4  sts = 0;

    if (!use_ast)
    {
        sts = (enbflg == 0) ? SS$_WASSET : SS$_WASCLR;
    }
    else
    {
        sts = sys$setast(enbflg);
    }

    return (sts);

}

i4
ii_sys$setimr(u_i4  efn,
                  struct _generic_64 *daytim,
                  void (*astadr)(__unknown_params),
                  unsigned __int64 reqidt,
                  u_i4  flags)
{
    i4  sts;

    /* copy the callers args */

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _reqidt = (AST_PRM)reqidt;
    AST_CTX* ctx = NULL;

    if (astadr && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETIMR, (AST_RTN)astadr, (AST_PRM)reqidt, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);

        /* kludge to allow caller to target the ast to the AST dispatcher thread */

        if (flags > 1)
        {
            flags = 0;
            ctx->flags.ctx$v_requeue = FALSE;

        }

        /*
        ** store the association bertween the ctx address and the
        ** original reqidt, since the new reqidt has been usurped by the ctx address.
        ** The call to $cantim will similarly retrieve the ctxadr (reqidt) so we
        ** can cancel the timer
        */

        IIMISC_addTimerEntry (ctx);

        /*
        ** reset the AST to our jacket function
        ** when this ast is delivered it will
        ** queue the context block to the ast dispatcher thread
        ** and wake the thread. The user's AST will then be called
        ** with the reqidt as the parameter
        */

        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _reqidt = (__int64)ctx;
    }

    sts = sys$setimr(efn, daytim, _astadr, _reqidt, flags);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}



i4
ii_sys$cantim(unsigned __int64 reqidt,
                  u_i4  acmode)
{

    /*
    ** if the reqidt is non-zero then we'll have to find the timer
    ** find all tqeQueue entries (whose reqidt is the ctx address for the original $setimr call)
    ** we then use the ctxadr as the timer entry. The ctx is marked as
    ** cancelled so that if it had been queued to the AST dispatch thread
    ** but not yet serviced, we'll ignore it
    **
    ** N.B. IIMISC_findTimerEntry will remove the entry from the queue and mark
    **      the ctx as cancelled. Note that the ctx block can only be on
    **      one of two queues at any one time -
    **      the tqeQueue and the astdeliveryQueue
    */

    AST_PRM  _reqidt = (AST_PRM)reqidt;
    i4       sts     = SS$_WASCLR;

    if (!use_ast)
    {
        AST_CTX* ctx  = NULL;
        while (ctx = IIMISC_findTimerEntry(reqidt))
        {
            _reqidt = (unsigned __int64)ctx;
            sts = sys$cantim (_reqidt, acmode);
            IIMISC_freeAstCtx(ctx);
            sts = SS$_WASSET;
        }
    }
    else
    {
        sts = sys$cantim (_reqidt, acmode);
    }

    return (sts);
 }

i4
ii_sys$sndjbc(u_i4  efn,
                  unsigned i2  func,
                  u_i4  nullarg,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SNDJBC, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$sndjbc(efn, func, nullarg, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}


/* These  system services are unhandled */

i4
ii_sys$acm(u_i4  efn,
               u_i4  func,
               i4  *contxt,
               void *itmlst,
               struct _acmesb *acmsb,
               void (*astadr)(__unknown_params),
               unsigned __int64 astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_ACM, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$acm(efn, func, contxt, itmlst, acmsb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$audit_event(u_i4  efn,
                       u_i4  flags,
                       void *itmlst,
                       u_i4  *audsts,
                       void (*astadr)(__unknown_params),
                       i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_AUDIT_EVENT, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$audit_event(efn, flags, itmlst, audsts, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$cpu_transition(i4  tran_code,
                          i4  cpu_id,
                          void *nodename,
                          i4  node_id,
                          u_i4  flags,
                          u_i4  efn,
                          struct _iosb *iosb,
                          void (*astadr)(__unknown_params),
                          unsigned __int64 astprm,
                          u_i4  timout)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_CPU_TRANSITION, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$cpu_transition(tran_code, cpu_id, nodename, node_id, flags, efn, iosb, _astadr, _astprm, timout);
    return (sts);
}

i4
ii_sys$dns(u_i4  efn,
               u_i4  func,
               void *itmlst,
               struct _iosb *dnsb,
               void (*astadr)(__unknown_params),
               i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_DNS, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$dns(efn, func, itmlst, dnsb, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$files_64(u_i4  efn,
                    unsigned i2  func,
                    void *fsb,
                    void (*astadr)(__unknown_params),
                    i4  astprm)
{
    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_FILES_64, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$files_64(efn, func, fsb, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$getdti(u_i4  efn,
                  u_i4  flags,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm,
                  u_i4  log_id [4],
                  u_i4  *contxt,
                  void *search,
                  void *itmlst)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_GETDTI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$getdti(efn, flags, iosb, _astadr, _astprm, log_id, contxt, search, itmlst);
    return (sts);
}

i4
ii_sys$io_fastpath(u_i4  efn,
                       u_i4  cpu_mask,
                       u_i4  func,
                       struct _iosb *iosb,
                       void (*astadr)(__unknown_params),
                       unsigned __int64 astprm,
                       __optional_params)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_IO_FASTPATH, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$io_fastpath(efn, cpu_mask, func, iosb, _astadr, _astprm);
    return (sts);
}


i4
ii_sys$io_setup(u_i4  func,
                    struct _generic_64 *bufobj,
                    struct _generic_64 *iosobj,
                    void (*astadr)(struct _iosa *),
                    u_i4  flags,
                    unsigned __int64 *return_fandle)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_IO_SETUP, (AST_RTN)astadr, (AST_PRM)0, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
    }
    sts = sys$io_setup(func, bufobj, iosobj, _astadr, flags, return_fandle);
    return (sts);
}

i4
ii_sys$ipc(u_i4  efn,
               unsigned i2  func,
               void *ipcb,
               void (*astadr)(__unknown_params),
               i4  astprm,
               __optional_params)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_IPC, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$ipc(efn, func, ipcb, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$lfs(u_i4  efn,
               unsigned i2  func,
               void *fsb,
               void (*astadr)(__unknown_params),
               i4  astprm)
{

    i4  sts;

    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_LFS, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$lfs(efn, func, fsb, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$setcluevt(u_i4  event,
                     void (*astadr)(__unknown_params),
                     i4  astprm,
                     u_i4  acmode,
#ifdef __INITIAL_POINTER_SIZE                    /* Defined whenever ptr size pragmas supported */
#pragma __required_pointer_size __long           /* And set ptr size default to 64-bit pointers */
                     void *handle)
#else
                     unsigned __int64 handle)
#endif
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETCLUEVT, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$setcluevt(event, _astadr, _astprm, acmode, handle);
    return (sts);
}

i4
ii_sys$set_system_event(u_i4  event,
                            void (*astadr)(__unknown_params),
                            unsigned __int64 astprm,
                            u_i4  acmode,
                            u_i4  flags,
                            unsigned __int64 *handle)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SET_SYSTEM_EVENT, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$set_system_event(event, _astadr, _astprm, acmode, flags, handle);
    return (sts);
}

i4
ii_sys$setevtast(void *evtnam,
                     i4  (*evtadr)(__unknown_params),
                     u_i4  *evtfac,
                     u_i4  acmode,
                     u_i4  flags,
                     u_i4  reqid,
                     u_i4  *evtid,
                     u_i4  evtcrd,
                     u_i4  efn,
                     struct _iosb *iosb,
                     void (*astadr)(__unknown_params),
                     i4  astprm,
                     u_i4  nullarg)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETEVTAST, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$setevtast(evtnam, evtadr, evtfac, acmode, flags, reqid, evtid, evtcrd, efn, iosb, _astadr, _astprm, nullarg);
    return (sts);
}

i4
ii_sys$setpfm(u_i4  pfmflg,
                  i4  (*astadr)(__unknown_params),
                  i4  astprm,
                  u_i4  acmode,
                  i4  *bufcntadr)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETPFM, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$setpfm(pfmflg, (i4  (*)())_astadr, _astprm, acmode, bufcntadr);
    return (sts);
}

i4
ii_sys$setpra(i4  (*astadr)(__unknown_params),
                  u_i4  acmode)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETPRA, (AST_RTN)astadr, (AST_PRM)0, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
    }
    sts = sys$setpra((i4  (*)())_astadr, acmode);
    return (sts);
}

i4
ii_sys$setdti(u_i4  efn,
                  u_i4  flags,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm,
                  u_i4  *contxt,
                  unsigned i2  func,
                  void *itmlst)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETDTI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$setdti(efn, flags, iosb, _astadr, _astprm, contxt, func, itmlst);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$setuai(u_i4  efn,
                  u_i4  *contxt,
                  void *usrnam,
                  void *itmlst,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_SETUAI, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$setuai(efn, contxt, usrnam, itmlst, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$updsec(struct _va_range *inadr,
                  struct _va_range *retadr,
                  u_i4  acmode,
                  char updflg,
                  u_i4  efn,
                  struct _iosb *iosb,
                  void (*astadr)(__unknown_params),
                  i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_UPDSEC, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$updsec(inadr, retadr, acmode, updflg, efn, iosb, _astadr, _astprm);
    if (!(sts & STS$M_SUCCESS) && astadr && !use_ast) IIMISC_freeAstCtx(ctx);
    return (sts);
}

i4
ii_sys$xfs_client(u_i4  efn,
                      unsigned i2  func,
                      void *fsb,
                      void (*astadr)(__unknown_params),
                      i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_XFS_CLIENT, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$xfs_client(efn, func, fsb, _astadr, _astprm);
    return (sts);
}

i4
ii_sys$xfs_server(u_i4  efn,
                      unsigned i2  func,
                      void *fsb,
                      void (*astadr)(__unknown_params),
                      i4  astprm)
{
    i4  sts;
    AST_RTN _astadr = (AST_RTN)astadr;
    AST_PRM _astprm = (AST_PRM)astprm;
    AST_CTX* ctx = NULL;

    if ((astadr != NULL) && !use_ast)
    {
        ctx = initAstCtx(CTX$C_SYS_XFS_SERVER, (AST_RTN)astadr, (AST_PRM)astprm, __RETURN_ADDRESS());
        if (ctx == NULL) return (SS$_INSFMEM);
        _astadr = (AST_RTN)IIMISC_ast_jacket;
        _astprm = (AST_PRM)ctx;
    }
    sts = sys$xfs_server(efn, func, fsb, _astadr, _astprm);
    return (sts);
}


/*
** Name:  initAstCtx
**
** Description:
**    Fetch an AST context block and initialise it
**
** Inputs:
**    u_i2 ssrv - system service code (mostly for debug)
**    AST_RTN astadr - user's original AST address
**    AST_PRM astprm - user's original AST parameter
**    u_i4  ret_pc - caller's PC (mostly for debug)
**
** Outputs:
**    None
**
**  Returns:
**    Address of AST context block
**
**  Exceptions:
**    None.
**
** Side Effects:
**    May start the AST High priority dispatcher thread if it hasn't
**    already been started
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static AST_CTX*
initAstCtx (u_i2 ssrv, AST_RTN astadr, AST_PRM astprm, u_i4  ret_pc)
{

    AST_CTX* ctx;

    ctx = IIMISC_getAstCtx();
    if (ctx != NULL)
    {
        ctx->flags.ctx$v_ssrv    = ssrv;
        ctx->flags.ctx$v_queued  = TRUE;
        ctx->ret_pc              = ret_pc;

        /*
        ** if we have a context defined (i.e. a target thread's sid)
        ** then allow the ast to be requeued to the issuer's thread
        ** (This may not be the case (e.g. iigcc is entirely AST driven
        **  and has no SCBs))
        */

        if (use_scb)
        {
            IICSMTget_ast_ctx(ssrv, ret_pc,
                              &ctx->scb,
                              &ctx->evt_cv,
                              &ctx->ast_qhdr,
                              &ctx->ast_pending);
            if (ctx->scb) ctx->flags.ctx$v_requeue = TRUE;
        }
        else
        {
            ctx->flags.ctx$v_noscb = TRUE;
        }

        /*
        ** save the real ast address and ast param
        */

        ctx->astadr = astadr;
        ctx->astprm = astprm;
    }
    return (ctx);
}

/*
** Name:  IIMISC_initialise_jacket
**
** Description:
**    Initialise the AST jacket code - check the multithread
**    argument to see if the executable is a single-threaded
**    or multi-threaded server.
**    If it is multi-threaded we can use the SCB as the target
**    of AST.
**    If it is single-threaded the AST must be executed on the
**    Ast dispatcher thread
**    In any event we must change the jacket state from the default
**    (just call the system service with the user specified AST)
**    to jacketing the ASTs.
**
** Inputs:
**    int multi_threaded
**
** Outputs:
**    None
**
**  Returns:
**    None
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

void
IIMISC_initialise_jacket (int multi_threaded)
{
    use_ast  = FALSE;
    use_scb  = multi_threaded;
}


/*
** Name:  init_jacket
**
** Description:
**    Initialise the AST jacket code - check the multithread
**    property to see if we need to start the AST dispatcher
**    thread. If this image is not multithreaded then
**    the jacket functions simply call the system services
**    directly (without substituting the ASTADR/ASTPRM  args)
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    None
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


static void
init_jacket(void)
{
    i4  sts;
    IOSB iosb;
    ILE3 itmlst[2];
    i4  multithread = 0;

    /*
    ** if we're using ASTs as normal, then we won't need
    ** an AST dispatcher thread
    */

    if (use_ast) return;

    /*
    ** protect ourselves from multiple threads
    ** atempting to initialise all at once
    */

    __LOCK_LONG(&jkt_lock);
    if (!jkt_initialised)
    {
        itmlst[0].ile3$w_length       = sizeof(multithread);
        itmlst[0].ile3$w_code         = JPI$_MULTITHREAD;
        itmlst[0].ile3$ps_bufaddr     = &multithread;
        itmlst[0].ile3$ps_retlen_addr = NULL;
        itmlst[1].ile3$w_length       = 0;
        itmlst[1].ile3$w_code         = 0;
        itmlst[1].ile3$ps_bufaddr     = NULL;
        itmlst[1].ile3$ps_retlen_addr = NULL;

        sts = sys$getjpiw (EFN$C_ENF, 0, 0, itmlst, &iosb, NULL, 0);
        if (sts & STS$M_SUCCESS) sts = iosb.iosb$w_status;
        if (!(sts & STS$M_SUCCESS)) lib$signal(sts);

        /*
        ** Any image linked /THREADS_ENABLE[=(MULTIPLE_KERNEL_THREADS,UPCALLS)]
        ** will have multithread >= 2 (CTL$GL_MULTITHREAD)
        ** N.B. multithread will be 0 if /NOTHREADS (default) (CTL$GQ_TM_CALLBACKS == 0 - no thread manager)
        **                          0    /THREADS=(MULTIPLE)  (CTL$GQ_TM_CALLBACKS == 0 - no thread manager)
        **                          1    /THREADS=(UPCALLS)   (Thread manager available)
        **                          2    /THREADS=(MULTIPLE,UPCALLS)
        **                          2    /THREADS
        **
        ** If the pthread$rtl has not been mapped (i.e. there are no pthread calls
        ** present in the code) then multithread will always be 0, regardless of the
        ** state of the /thread_enable switch for the linker
        **
        */

        if (multithread >= 2) jkt_multithreaded = TRUE;

        //if (jkt_multithreaded) IIMISC_initialise_astmgmt(NULL);
        jkt_initialised = TRUE;
        __MB();
    }
    __UNLOCK_LONG(&jkt_lock);
}


#pragma standard

# else /* OS_THREADS_USED */

   static int dummy = 1; /* Declaration to avoid compiler warnings */

#endif /* OS_THREADS_USED */

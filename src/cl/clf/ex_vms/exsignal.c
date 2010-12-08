/*
**    Copyright (c) 1993, 2000 Ingres Corporation
*/
# include	<compat.h>
#include <gl.h>
#if defined(axm_vms)
# include	<excl.h>
# include	<tr.h>
# include	<lib$routines.h>
#else
#include <clconfig.h>
#include <clsigs.h>
#include <ex.h>
#include <er.h>
#include <pc.h>
#include <si.h>
#include <evset.h>
#include <exinternal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#endif

/*
** exsignal.c
**
**      History:
**
**           27-may-93 (walt)
**               Add a case for zero arguments.
**           25-jun-1993 (huffman)
**               Change include file references from xx.h to xxcl.h.
**	     16-jul-1993 (walt)
**		Remove check for bad arg_count before the switch statement.
**		Let the default case print a message and exit.
**	     07-nov-1994 (markg)
**		Changed call to lib$signal to not pass arg_count. This is not
**		needed as it clutters up the signal array and makes
**		signal argument handling difficult. lib$signal is documented
**		incorrectly by DEC.
**	     31-mar-1995 (dougb)
**		Integrate Mark's change from the 6.4 codeline.  Note: the
**		real problem here is in what's passed to a handler routine
**		such as EXcatch() -- we don't want the arg count twice.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      22-dec-2008 (stegr01)
**          Itanium VMS port
**      12-aug-2010 (joea)
**          Replace VMS exception handling by POSIX signals as done on Unix.
*/

#if defined(axm_vms)
VOID
EXsignal(EX ex, i4 arg_count, i4 arg1, i4 arg2, i4 arg3, 
		 i4 arg4, i4 arg5, i4 arg6)
{

	switch (arg_count)
	{

	case 0:
		lib$signal(ex);
		break;

	case 1:
		lib$signal(ex, arg1);
		break;

	case 2:
		lib$signal(ex, arg1, arg2);
		break;

	case 3:
		lib$signal(ex, arg1, arg2, arg3);
		break;

	case 4:
		lib$signal(ex, arg1, arg2, arg3, arg4);
		break;

	case 5:
		lib$signal(ex, arg1, arg2, arg3, arg4, arg5);
		break;

	case 6:
		lib$signal(ex, arg1, arg2, arg3, arg4, arg5, arg6);
		break;

 	default:
		TRdisplay("EXsignal:  arg_count is %d (should be 0-6). \
Nothing will be signaled.\n", arg_count);
		break;
	}

}
#else

#define i_EXreturn(x, y)  longjmp((x)->iijmpbuf, (y))

/* On some machines special code can be run to turn an integer divide by zero
** into a recoverable signal event.
*/

#if defined(xCL_OVERRIDE_IDIV_HARD)
#undef xCL_046_IDIV_HARD
#endif

/*
** Remap exceptions from which we may not return to be "hard" cases.
*/
#ifndef xCL_011_USE_SIGVEC
#       if defined(xCL_046_IDIV_HARD) || defined(xCL_049_FDIV_HARD) \
                || defined(xCL_052_IOVF_HARD) || defined(xCL_058_FUNF_HARD)
#               undef EXINTDIV
#               undef EXFLTDIV
#               undef EXINTOVF
#               undef EXFLTOVF
#               undef EXFLTUND
#               define EXINTDIV EXHINTDIV
#               define EXFLTDIV EXHFLTDIV
#               define EXINTOVF EXHINTOVF
#               define EXFLTOVF EXHFLTOVF
#               define EXFLTUND EXHFLTUND
#       endif
#else
#       ifdef xCL_046_IDIV_HARD
#               undef EXINTDIV
#               define EXINTDIV EXHINTDIV
#       endif
#       ifdef xCL_049_FDIV_HARD
#               undef EXFLTDIV
#               define EXFLTDIV EXHFLTDIV
#       endif
#       ifdef xCL_052_IOVF_HARD
#               undef EXINTOVF
#               define EXINTOVF EXHINTOVF
#       endif
#       ifdef xCL_055_FOVF_HARD
#               undef EXFLTOVF
#               define EXFLTOVF EXHFLTOVF
#       endif
#       ifdef xCL_058_FUNF_HARD
#               undef EXFLTUND
#               define EXFLTUND EXHFLTUND
#       endif
#endif /* SIGVEC */

#ifdef MAXSIG
#undef MAXSIG
#endif
#define MAXSIG          NSIG    /* must be >= highest signal number web
                                ** catch, + 1; be sure it's always big
                                ** enough!
                                */


FUNC_EXTERN void EXdump(u_i4 error, u_i4 *stack);


typedef TYPESIG (*SIGHANDLER_TYPE)(int);

/*
** Keep track of what signal actions are already in place.
** If an exception occurs that we do not handle (no handler in
** place, or last handler returns EXRESIGNAL), take this action.
*/
SIGHANDLER_TYPE orig_handler[MAXSIG];

/*
** These are the signals we are interested in mapping to exceptions.
** You should catch ALL signals that would terminate the program except
** SIGIOT (SIGABORT).
**
** You should NOT catch signals whose SIG_DFL action is harmless.
**
** There had better be a case in i_EXcatch for each of these, and a 
** decode case in exsysrep.c.
*/ 
int sigs_to_catch[] = 
{
        SIGHUP,
        SIGINT,
        SIGQUIT,
        SIGILL,
#ifdef SIGDANGER
        SIGDANGER,
#endif
#ifdef SIGEMT
        SIGEMT,
#endif
        SIGALRM,
        SIGBUS,
        SIGFPE,
#ifdef SIGLOST
#if    SIGLOST != SIGABRT
        SIGLOST,
#endif
#endif
        SIGPIPE,
#ifdef SIGPROF
        SIGPROF,
#endif
#ifdef SIGPWR
        SIGPWR,
#endif
        SIGSEGV,
#ifdef SIGSYS
        SIGSYS,
#endif
#ifdef SIGSYSV
        SIGSYSV,
#endif
        SIGTERM,
#ifdef SIGTRAP
        SIGTRAP,
#endif
#ifdef SIGUSR1
        SIGUSR1,
#endif
#ifdef SIGUSR2
        SIGUSR2,
#endif
#ifdef SIGVPROF
        SIGVPROF,
#endif
#ifdef SIGVTALRM
        SIGVTALRM,
#endif
#ifdef SIGXCPU
        SIGXCPU,
#endif
#ifdef SIGXFSZ
        SIGXFSZ,
#endif
      -1                /* terminates the list */
};

/*
** Counts of "queued" signals while EXinterrupt is blocking them.
*/
extern i4 EXsigints;
extern i4 EXsigquits;

/*
** Flags that we are already processing these signals, to avoid recursion
*/
static i4 Inhup = 0;
static i4 Inint = 0;
static i4 Inquit = 0;

/*
**  Set the default client type to be an user application.
*/
static i4 client_type = EX_USER_APPLICATION;
static bool ex_initialized = FALSE;

/*
** Forward reference
*/
TYPESIG i_EXcatch(int signum, siginfo_t *code, EX_SIGCONTEXT *scp);

/*
** Name: EXsetsig -
**
** Description:
**      sets up a handler for a signal, using eithe sigvec or signal
**      as appropriate.
**
** Inputs:
**      signum                  The signal of interest
**      handler                 A pointer to function of TYPESIG
**                              to be used as the new handler.
**
** Returns:
**      A pointer to the  TYPESIG handler previously installed.
**
** Side Effects:
**      Changes the process's signal state.
*/
SIGHANDLER_TYPE
EXsetsig(int signum, SIGHANDLER_TYPE handler)
{
#ifdef xCL_011_USE_SIGVEC
    struct sigvec new, old;

    new.sv_handler = handler;
    new.sv_mask = new.sv_onstack = 0;

#ifdef SV_ONSTACK
#  if defined (xCL_077_BSD_MMAP) || defined (ris_u64) || defined(rs4_us5)
#   if defined(su4_u42) || defined (ris_u64) || defined(rs4_us5)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
        new.sv_onstack |= SV_ONSTACK;
#   endif /* su4_u42 */
#  endif /* ifdef xCL_077_BSD_MMAP */
#endif /* SV_ONSTACK */

    (void)sigvec(signum, &new, &old);
    return old.sv_handler;
#else
#ifdef xCL_068_USE_SIGACTION
    struct sigaction new, old;

#if defined(hpb_us5) || defined(LNX) || defined(mg5_osx)
    new.sa_sigaction = handler;
#else
    new.sa_handler = handler;
#endif /* hpb_us5 */
    sigemptyset(&new.sa_mask);

    new.sa_flags = 0;

#ifdef SA_SIGINFO
    /*
    ** give us three args to the signal handler to provide context
    ** information ( we already expect three args for all signal suites )
    */
    new.sa_flags |= SA_SIGINFO;
#endif /* SA_SIGINFO */

#ifdef SA_ONSTACK
#if defined (xCL_077_BSD_MMAP) || defined(rs4_us5) || defined(ris_u64)
    /*
    ** catch some signals on an alternate stack
    */
#   if defined(su4_us5) || defined(axp_osf) || defined(hpb_us5) || \
        defined(ris_u64) || defined(rs4_us5)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
        new.sa_flags |= SA_ONSTACK;
#   elif defined(LNX) || defined(mg5_osx)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
        new.sa_flags =  new.sa_flags | SA_ONSTACK | SA_RESTART | SA_NODEFER;
#   endif /* su4_us5 axp_osf */
#  endif /* ifdef xCL_077_BSD_MMAP */
#endif /* SA_ONSTACK */

    (void)sigaction(signum, &new, &old);
#if defined(hpb_us5) || defined(LNX) || defined(mg5_osx)
    return( old.sa_sigaction );
#else
    return( old.sa_handler );
#endif /* hpb_us5 */
#else
    return(signal(signum,handler));
#endif /* xCL_068_USE_SIGACTION */
#endif /* xCL_011_USE_SIGVEC */
}

/*
** Name:
**      EXestablish()
**
** Function:
**      Set catches for signals from UNIX. This is called
**      from the i_EXsetup the first time that is called
**      out of the EXdeclare() macro.
**
** Side Effects:
**      Changes for signals set up, saves the original handlers.
*/
void
i_EXestablish()
{
    char *segv_dump;    /* Don't-catch env var */
    i4  i;              /* array index */
    i4  s;              /* signal of interest */

    if (!ex_initialized)
    {
        ex_initialized = TRUE;
        if( client_type == EX_INGRES_DBMS)
            EXsetsig(SIGINT, SIG_IGN);
    }

    /* See if don't-catch var is defined (env var only, not ing var) */
    segv_dump = getenv("II_SEGV_COREDUMP");

    /* loop through the array of signals we want to map to exceptions */

    for (i = 0; (s = sigs_to_catch[i]) > 0 ; i++)
    {
        /* Don't catch segv, bus error if trying to get coredump */
        if (segv_dump != NULL && (s == SIGBUS || s == SIGSEGV))
            continue;

        /*
        **  Since SIGBUS, SIGSEGV, SIGFPE ans SIGPIPE are
        **  always trapped by everyone, we don't have to deal with them
        **  specially below.
        */
        if (client_type != EX_INGRES_DBMS)
        {
            switch (s)
            {
            case SIGHUP:
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                /*
                **  If the client is a tool, then break out
                **  so it can be trapped.  Otherwise, skip it
                **  for user apps.
                */
                if (client_type == EX_INGRES_TOOL)
                    break;
                else
                    continue;

            case SIGILL:
#ifdef SIGDANGER
            case SIGDANGER:
#endif
#ifdef SIGEMT
            case SIGEMT:
#endif
            case SIGALRM:
#ifdef SIGLOST
            case SIGLOST:
#endif
#ifdef SIGPROF
            case SIGPROF:
#endif
#ifdef SIGPWR
            case SIGPWR:
#endif
#ifdef SIGSYS
            case SIGSYS:
#endif
#ifdef SIGSYSV
            case SIGSYSV:
#endif
#ifdef SIGTRAP
            case SIGTRAP:
#endif
#ifdef SIGUSR1
            case SIGUSR1:
#endif
#ifdef SIGUSR2
            case SIGUSR2:
#endif
#ifdef SIGVPROF
            case SIGVPROF:
#endif
#ifdef SIGVTALRM
            case SIGVTALRM:
#endif
#ifdef SIGXCPU
            case SIGXCPU:
#endif
#ifdef SIGXFSZ
            case SIGXFSZ:
#endif
                continue;
            }
        }

        if ((orig_handler[s] = EXsetsig(s, (SIGHANDLER_TYPE)i_EXcatch))
            == SIG_IGN)
        {
            /* If we were ignoring it, continue to do so for these.
            ** This is so background jobs don't suddenly start to
            ** pickup keyboard generated signals that were nohupped.
            */
            switch (s)
            {
            case SIGINT:
            case SIGHUP:
            case SIGQUIT:
                (void)EXsetsig(s, SIG_IGN);
            }
        }
    }
}

void EXsignal(int ex, int arg_count, ...);

/*
**      Name:
**              i_EXcatch()
**
**      Function:
**              The function installed by EX to handle the UNIX signals of
**              interest contained in sigs_to_catch[].  Signals mentioned
**              here, but not in sigs_to_catch will never be caught.
**
**      Inputs:
**              signum                  The signal of interest
**              code                    The sigvec code
**              scp                     the sigvec context pointer
**
**              Code and scp will not exist on non-sigvec systems.
**
**      Returns:
**              none.
**
**      Side Effects:
**              Drives the exception system, or leaves flags for
**              interrupts and quits of interest to EXinterrupt.
*/
TYPESIG
i_EXcatch(int signum, siginfo_t *code, EX_SIGCONTEXT *scp)
{
    EX e;

#ifdef xCL_011_USE_SIGVEC
    /*
    ** Must restore the original signal mask.  The kernel blocked
    ** the current signal on entry to the handler; but if we _longjmp
    ** out of the handler, the kernel will not get a chance to restore
    ** the original mask for us.
    */

#if defined(ris_u64)
    /* sc_mask is a sigset_t, which is a structure of two groups of 32-bit
    ** integers for AIX 4.1.  we just need the lower 32 for this function.
    */
    (void)sigsetmask(scp->sc_mask.losigs);
#else
    (void)sigsetmask(scp->sc_mask);
#endif  /* rs4_us5 */
#else
#if defined(xCL_068_USE_SIGACTION)
     /*
     ** restore original signal mask (see above comment for sigvec)
     */
#if defined(sqs_ptx) || defined(axp_osf) || defined(sos_us5) || \
    defined(hp8_us5) || defined(i64_aix) || defined(LNX) || defined(VMS) || \
    defined(rs4_us5) || defined (dgi_us5) || defined(mg5_osx)
    {
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask,signum);
        sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
    }
#else
    sigprocmask(SIG_SETMASK, &(scp->uc_sigmask), (sigset_t *)0);
#endif /* sqs_ptx */

#else
    /* Notorious race condition here! If same signal happens before
       we replant ourselves, we can get blown out of the water. */
    (void)EXsetsig(signum, (SIGHANDLER_TYPE)i_EXcatch);

    /* provide some value to exception handler */
    code = scp = signum;
#endif /* xCL_068_USE_SIGACTION */
#endif  /* xCL_011_USE_SIGVEC */

    switch (signum)
    {
        case SIGHUP:
            if (Inhup)              /* avoid recursion */
                return;
            e = EXHANGUP;
            break;

        case SIGINT:
            /*
            ** If SIGINT and SIGQUIT and keyboard interrupts are currently
            ** masked (EXintr_count > 0), just count the signal.
            ** The exception will be raised when keyboard interrupts
            ** are unmasked with EX_ON, EX_DELIVER or EX_RESET.
            */
            if (EXintr_count > 0)         /* log ints while blocked */
            {
                 EXsigints++;
                return;
            }
            else if (Inint)               /* avoid recursion */
            {
                return;
            }
            e = EXINTR;
            break;

        case SIGQUIT:
            if (EXintr_count > 0)         /* log quits while blocked */
            {
                EXsigquits++;
                return;
            }
            else if (Inquit)              /* avoid recursion */
            {
                return;
            }
            e = EXQUIT;
            break;

        /*
        ** We don't map anything to EXRESVOP or EXBUSERR, because many mainline
        ** handlers only call EXsysreport when the exception is EXSEGVIO.  So
        ** We dump all the fatal signals there instead.  Were we really going
        ** to do something different for these exceptions than for a SEGV?
        **      e = EXRESVOP;
        **      break;
        **      e = EXBUSERR;
        **      break;
        */

        case SIGFPE:
            /*
            ** On machines where possible and needed, adjust the signal return
            ** pc so we don't land on the same instruction and loop.  If
            ** you can do this, be sure not to turn on the "hard" versions
            ** of the exceptions.
            */
#ifdef  xCL_011_USE_SIGVEC
            switch (code)
            {
#ifdef FPE_INTDIV_TRAP
                case FPE_INTDIV_TRAP:
                    e = EXINTDIV;
                    break;
#endif
#ifdef FPE_INTDIV_EXC
                case FPE_INTDIV_EXC:
                    e = EXINTDIV;
                    break;
#endif
#ifdef FPE_INTOVF_TRAP
                case FPE_INTOVF_TRAP:
                    e = EXINTOVF;
                    break;
#endif
#ifdef FPE_INTOVF_EXC
                case FPE_INTOVF_EXC:
                    e = EXINTOVF;
                    break;
#endif
#ifdef FPE_FLTDIV_TRAP
                case FPE_FLTDIV_TRAP:
                    e = EXFLTDIV;
                    break;
#endif
#ifdef FPE_FLTDIV_EXC
                case FPE_FLTDIV_EXC:
                    e = EXFLTDIV;
                    break;
#endif
#ifdef FPE_FLTUND_TRAP
                case FPE_FLTUND_TRAP:
                    e = EXFLTUND;
                    break;
#endif
#ifdef FPE_FLTUND_EXC
                case FPE_FLTUND_EXC:
                    e = EXFLTUND;
                    break;
#endif
        /* These default to EXFLTOVF */
#ifdef FPE_FLTOVF_TRAP
                case FPE_FLTOVF_TRAP:
#endif
#ifdef FPE_FLTOVF_EXC
                case FPE_FLTOVF_EXC:
#endif
#ifdef FPE_FLTNAN_TRAP
                case FPE_FLTNAN_TRAP:
#endif
#ifdef FPE_FLTINVALID_EXC
                case FPE_FLTINVALID_EXC:
#endif
#ifdef FPE_FLTINEXACT_EXC
                case FPE_FLTINEXACT_EXC:
#endif
                default:
                    e = EXFLTOVF;
            }
#else   /* xCL_011_USE_SIGVEC */
            e = EXFLTOVF;           /* Should be EXFLOAT, but mainline
                                       handlers don't know about that one. */
#endif  /* xCL_011_USE_SIGVEC */
            break;

#ifdef SIGTRAP
        case SIGTRAP:
            e = EXTRACE;
#ifdef axp_osf
            e = EXINTDIV;
#endif
            break;
#endif /* SIGTRAP */

        /*
        ** Map any fatal but otherwise uninteresting signals to EXSEGVIO,
        ** so naive mainline code that only calls EXsysreport on that exception
        ** will pick it up and log a meaningful message.
        */
        case SIGBUS:
#ifdef SIGDANGER
            /*
            ** Sigdanger is not fatal on most systems. If it is so on yours,
            ** ifdef it for your box. Else, we ignore it.
            */
#endif
#ifdef SIGEMT
        case SIGEMT:
#endif
        case SIGILL:
# ifndef ris_us5
#ifdef SIGLOST
        case SIGLOST:
#endif
#endif /* ris_us5 */
#ifdef SIGPROF
        case SIGPROF:
#endif
#ifdef SIGPWR
        case SIGPWR:
#endif
        case SIGSEGV:
#ifdef SIGSYS
        case SIGSYS:
#endif
#ifdef SIGSYSV
        case SIGSYSV:
#endif
#ifdef SIGUSR1
        case SIGUSR1:
#endif
#ifdef SIGUSR2
        case SIGUSR2:
#endif
#ifdef SIGVTALRM
        case SIGVTALRM:
#endif
#ifdef SIGXCPU
        case SIGXCPU:
#endif
#ifdef SIGXFSZ
        case SIGXFSZ:
#endif
            e = EXSEGVIO;
            break;

        case SIGALRM:
            e = EXTIMEOUT;
            break;

        case SIGPIPE:
            e = EXCOMMFAIL;
            break;

        case SIGTERM:
            e = EXKILL;
            break;

        default:  /* not a signal we handle */
            return;
    }

    /* EXsignal and EXsys_report count on there being 3 args if this is
       an exception raised by a signal.  This distinguishes them from ones
       raised by software, which will have no arguments */

    EXsignal(e, 3, (long)signum, (long)code, (PTR)scp);
}

/*
** Name:  EXsignal -
**
** Inputs:
**      ex              exception to raise
**      N               number of arguments following.
**      args ...        variable number of arguments to the exception.
**
** Example:
**      EXsignal(ex, N, arg1, arg2, ... ,argN)
**              EX      ex;
**              i4      N;
**              i4      arg1,arg2,...argN;
**
**      Raises the exception ex with the N arguments arg1..argN.
**
** Returns:
**      Nothing.
*/

# define        ERR_EXIT        -1

void
EXsignal(EX ex, i4 N, ...)
{
    long        *arp;
    EX_CONTEXT  *env,
                *exc,
                **excp;
    long        argarr[EXMAXARG];
    EX_ARGS     exarg;
    char        errmsg[ER_MAX_LEN];
    STATUS      handle_ret;
    i4          sig;
    va_list     args;

    exarg.exarg_count = N;
    exarg.exarg_num = ex;
    exarg.exarg_array = argarr;

    /* Don't allow recursive interrupts.  It's OK if an Inxxx
    ** variable goes > 1, since we will forcibly set it to zero before
    ** we are done processing the one that matters.  We must be sure
    ** to reset them, or we will lock out subsequent interrupts!
    */
    if ((ex == EXINTR && Inint++) || (ex == EXQUIT && Inquit++)
        || (ex == EXHANGUP && Inhup++))
        return;

    /*
    ** We first collect up the arguments into the standard
    ** exception argument stack.  This is declared locally
    ** because exceptions can be signaled during an exception.
    ** 3 args is a special case indicating a system exception
    ** has occured, the data is then written into additional 
    ** elements of the EX_ARGS structure
    */
    arp = argarr;
    va_start(args, N);
    if (N < 3)
        while (N--)
                *arp++ = va_arg(args, long);
    else
    {
        exarg.signal = va_arg(args, long );
        exarg.code = va_arg(args, long);
        exarg.scp = va_arg(args, PTR);
    }
    va_end(args);

    /*
    ** We are now ready to begin running handlers.
    ** This loop does not pop exceptions off the stack.
    */
    excp = i_EXtop();
    env = excp ? *(excp) : NULL;
    for ( ; env; env = i_EXnext(env))
    {
        if (env->handler_address == 0)
            continue;

        handle_ret = (*(env->handler_address))(&exarg);

        switch (handle_ret)
        {
            case EXRESIGNAL:
                /* run higher level handlers */
                continue;

            case EXDECLARE:
                if (ex != EX_PSF_LJMP)
                    EXdump(EV_DUMP_ON_FATAL, 0);
                /* long jump back to the declaration ("unwind") */
                break;

            case EXCONTINUES:
                /* return running no more handlers */
                Inint = Inquit = Inhup = 0;
                return;

            default:
                SIfprintf(stderr, "invalid handler return--%x\n",
                          handle_ret);
                PCexit(ERR_EXIT);
        }
        break;
    }

    /*
    ** If no handler and it is EXEXIT, bail out immediately with
    ** status 0.  This doesn't call PCexit to run PCatexit queued
    ** routines.  Maybe it should.
    **
    ** If no handler and it is a UNIX signal, reinstall the user's
    ** original handler and resignal.  Exit if it returns.
    */
    if (!env)
    {
        /*
        ** No more handlers.  Take the original handler's action, if any.
        */
        switch (ex)
        {
            case EXEXIT:
                exit(0);        /* Not PCexit? */
                break;

            case EXHANGUP:
            case EXINTR:
            case EXQUIT:
            case EXRESVOP:
            /* case EXBUSERR: see EXSEGVIO */
            case EXINTDIV:
            case EXFLTDIV:
            case EXFLTOVF:
            case EXSEGVIO:
            case EXTIMEOUT:
            case EXCOMMFAIL:
            case EXUNKNOWN:
            case EXBREAKQUERY:
            case EXCHLDDIED:
                if (exarg.exarg_count == 3)
                {
                    /*
                    ** This exception was generated by a UNIX signal.
                    ** The single argument is the signal number that
                    ** caused the exception.
                    */
                    sig = exarg.signal;
                    (void)EXsetsig(sig, orig_handler[sig]);
                    (void)kill(getpid(), sig);
                    (void)EXsetsig(sig, (SIGHANDLER_TYPE)i_EXcatch);
                    break;
                }
                /* else fall through ... */

            default:
                EXsignal( EXEXIT, 0, 0 );
                abort();        /* "Can't get here" */
        }
        Inint = Inquit = Inhup = 0;
        return;
    }

    /*
    ** If we get here then an unwind is in progress.
    **
    ** We have to step down the runtime stack popping
    ** each scope we encounter.  Before the scope is popped
    ** we must call any exception bound to the stack
    **
    ** Each exception on the except stack must be <= to
    ** current runtime scope because of the loop
    ** done at the start of the routine.
    */
    exarg.exarg_num = EX_UNWIND;
    exarg.exarg_count = 0;

    for (exc = *(excp = i_EXtop()); env != exc; exc = i_EXnext(exc))
    {
        if (exc->handler_address)
        {
            (void)(*(exc->handler_address))(&exarg);

            i_EXpop(excp);
        }
    }

    /*
    ** The handlers have been run.  Time to do the return to
    ** the appropriate scope.
    */
    Inint = Inquit = 0;
#ifdef xCL_077_BSD_MMAP
#if defined(LNX)
    {
        stack_t exstack;

        if (sigaltstack( (stack_t *)0, &exstack) == 0)
        {
            if (exstack.ss_flags & SA_ONSTACK)
            {
                sigset_t mask;
        
                sigemptyset(&mask);
                sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
                exstack.ss_flags &= ~SA_ONSTACK;
                sigaltstack(&exstack, (stack_t *)0);
            }
        }
    }
#endif
#if defined(su4_u42) || defined(axp_osf)
    {
        struct sigstack exstack;

#if defined(axp_osf)
        if (sigstack(&exstack, &exstack) == 0)
#else
        if (sigstack((struct sigstack *)0, &exstack) == 0)
#endif
        if (exstack.ss_onstack)
        {
#if defined(axp_osf)
            sigset_t mask;

            sigemptyset(&mask);
            sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
#endif
            exstack.ss_onstack = 0;
            (void)sigstack(&exstack, (struct sigstack *)0);
        }
    }
#endif /* su4_u42 axp_osf */
#endif /* ifdef xCL_077_BSD_MMAP */
    i_EXreturn(env, EXDECLARE);

    /* "should never get here" -- i_EXreturn shouldn't return */
}

/*{
** Name: EXsetclient - Set client information for the EX subsystem
**
** Description:
**      This routine informs the EX subsystem as to the type of client
**      that it is dealing with.  The client type information is
**      important in that it allows EX to modify its behavior dependent
**      on the client
**
**      For user applications, EX will only intercept access violation and
**      floating point exceptions.  This becomes the default behavior for EX.
**      It is up to the user application to deal with any other exceptions.
**
**      If the client is the Ingres DBMS, then EX will need to intercept
**      any exceptions that will cause a process to exit (current behavior).
**
**      In the case of Ingres Tools, EX will also intercept user generated
**      exit exceptions (such as interrupt) in addition to the access
**      violation and floating point exceptions.
**
** Inputs:
**      EX_INGRES_DBMS          Client is the Ingres DBMS or any application
**                              that desires the trap all behavior.
**
**      EX_INGRES_TOOL          Client is one of the Ingres Tools.
**
**      EX_USER_APPLICATION     Client is a user written application.  This
**                              is actually unnecessary since this is the
**                              default.  It is defined here just for
**                              completeness.
**
** Outputs:
**      Returns:
**              EX_OK                   If everything succeeded.
**              EXSETCLIENT_LATE        If the EX subsystem has already
**                                      been initialized when this is called.
**              EXSETCLIENT_BADCLEINT   If an unknown client was passed in.
**
**      Exceptions:
**              None.
**
** Side Effects:
**      None.
*/
STATUS
EXsetclient(i4 client)
{
#if !defined(axm_vms)
    if (!ex_initialized)
    {
        switch (client)
       {
           case EX_INGRES_DBMS:
           case EX_INGRES_TOOL:
           case EX_USER_APPLICATION:
               client_type = client;
               break;

         default:
              return EXSETCLIENT_BADCLIENT;
              break;
       }
   }
   else
   {
       return EXSETCLIENT_LATE;
   }
#endif
   return EX_OK1;
}
#endif

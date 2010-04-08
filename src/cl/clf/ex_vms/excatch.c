/*
** Copyright (c) 1993, 2009 Ingres Corporation
*/
# include	<compat.h>
# include	<excl.h>
# include	<mhcl.h>
# include	<ssdef.h>
# include	<mthdef.h>
# include       <er.h>
# include       <si.h>
# include       <st.h>
# include	<tr.h>
# include       <ints.h>
# include	<chfdef.h>
# include       <stsdef.h>
#if defined(i64_vms)
# include       <libicb.h>
#endif
# include	<lib$routines.h>
# include	<starlet.h>
# include       "exi.h"

#include <setjmp.h>

#ifdef OS_THREADS_USED
#include <pthread.h>
#include <cma$def.h>
#endif

/*
** EXCATCH.C -		EXdeclare establishes EXcatch as the generic condition
**			handler to the operating system for all conditions
**			(exceptions).  EXcatch calls the user-provided handler,
**			looks at the status returned, then returns an appropriate
**			return code to the operating system.
**      History:
**
**          25-jun-1993 (huffman)
**              Change include file references from xx.h to xxcl.h.
**	    02-aug-1993 (walt)
**		Log received exceptions to help track looping server.
**	    02-aug-1993 (walt)
**		Special case:  if exargs.exarg_num is 0x10A32 (68146.) then
**		set exargs.exarg_count to zero.  This wasn't necessary on VAX
**		because we didn't copy the arguments to a separate place for
**		the user handler like we do here.
**	    06-aug-1993 (walt)
**		If we're called during an unwind, delete our record of the
**		user's handler because the frame is going to be deleted.
**	    10-aug-1993 (walt)
**		Turn the generic Alpha exception SS$_HPARITH into a matching
**		CL error so it doesn't just get passed over as an unrecognized
**		exception.  Also set exarg_count to zero for exceptions where
**		there's no arguments to copy to the user (signal array argument
**		count <= 3).
**	    11-aug-1993 (walt)
**		Make invo_value an i8 rather than just an int.  sys$goto_unwind
**		takes 64 bits from the address of invo_value and moves them
**		into R0 during a nonlocal goto.  If any of the high bits of
**		invo_value become set somehow, then a statement of the form
**		"if (EXdeclare( ...) == EXDECLARE" fails because the internal
**		return value isn't *exactly* EXDECLARE in all 64 bits.
**	    07-nov-1994 (markg)
**		Fixed the method of passing arguments to the user handler.
**		The problem was caused by incorrect values being passed to 
**		lib$signal in EXsignal(). Also backed out Walt's change of
**		02-aug-1993 as this is no longer required.
**	    31-mar-1995 (dougb)
**		Integrate Mark's change from the 6.4 codeline.  Note: the
**		real problem here is in what's passed to a handler routine
**		such as EXcatch() -- we don't want the arg count twice.  I
**		also restored logic which ensures the argument count doesn't
**		go negative (just in case...), which isn't in the 6.4 version.
**	    3-apr-1995 (dougb)
**		Make sure registers are restored as of the
**		lib$get_current_invo_context() call in the EXdeclare() macro.
**	    26-jul-1995 (dougb)
**		Do *not* continue after a problem we can't fix up.  In
**		particular, arithmetic and MTH$ routine errors must cause
**		EX_CONTINUE from the user handler to be ignored.  We chose
**		to resignal instead.
**		There is no way on this pipelined machine to determine
**		exactly which instruction caused an arithmetic fault and to
**		correct the offending operand (there's no lib$fixup_flt()
**		routine).  While a MTH$ routine signalling a problem may be
**		on the stack above us, it is not a simple matter to
**		determine where its return value will be placed to fix it
**		up.  Both of these types of fixups would be performed on
**		Vax/VMS.  Some Unix boxes must be similar to Alpha VMS --
**		what do they do?
**		??? Should spend some time finding a way to fix up MTH$
**		??? return values.  That at least sounds possible.
**	    23-aug-1995 (duursma)
**		Two changes:
**		(1) With the reimplementation of MH.C (calling C runtimes instead
**		of MTH$ and OTS$ routines), MTH$ exceptions should never occur
**		again.  If they do, TRdisplay() an error message.  And as before,
**		don't allow EX_CONTINUE from such an exception.
**		(2) Don't restore scratch registers (R22-R24).  They were probably
**		not saved to begin with
**	    10-oct-1995 (duursma/dougb)
**		Performance improvements:
**		    Avoid all calls to lib$get_curr_invo_context() and
**		lib$get_prev_invo_context().  Move the call to
**		lib$get_invo_handle() out of the stack-walking loop.  Replace
**		the call to EXfind_handler() with the new i_EXtop() and
**		i_EXnext() routines.
** 	    03-jun-1997 (teresak/schang)
**		Bug 81931: Certain queries will cause the DBMS server to
**		access violate inside an OpenVMS 7.x system call because
**	        a data structure that originated from OpenVMS system calls 
**		was modifed and therefore the data returned to other system 
**		calls was invalid.
**	    09-nov-1999 (kinte01)
**	        Rename EX_CONTINUE to EX_CONTINUES per AIX 4.3 porting change
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes and
**	    external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	18-May-2001 (hanje04)
**	    When crossing 447895 from oping20 to main changed iijmpbuf
**	    back to jmpbuf. Now change it back to iijmpbuf again
**      15-Jan-2007 (horda03) Bug 117486
**          Catch IMGDMP signals. There occur when the SET PROC/DUMP
**          command is executed against a process. If we catch this
**          signal, so a stack dump and continue normal processing.
**	18-jun-2008 (joea)
**	    Correct EX_CONTINUES to EX_CONTINUE as done on Unix and Windows.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**	05-jun-2009 (joea)
**	    Conditionally reinstate previous Alpha version. Make #ifdefs
**	    depend on release identifier rather than other constants. Reformat
**	    some lines for readability.
**      17-jun-2009 (stegr01)
**          replace sys$goto_unwind_64 with longjmp for Itanium
**          The C RTL setjmp/longjmp code contains a call to Itanium assembler 
**          to correctly restore scratch registers which apparently the sys$goto_unwind
**          fails to do
**          As a result of this surplus code has been removed
**	30-jun-2009 (joea)
**	    Re-merge Unix function signatures.
**      17-Aug-2009 (horda03) B122474
**          Trying to assign a FLOAT to a too large a value causes a SS$_HPARITH
**          exception, so once detected what the exception is for, convert to the
**          exception value Ingres expects.
**      17-nov-2009 (stegr01)
**          move a couple of variables to the top of a for loop to avoid 
**          compiler complaints
*/



int
EXcatch(struct chf$signal_array *sigs, struct chf$mech_array *mechs)
{
#if defined(axm_vms)
	int		establishers_handle;
	int64		*jmpbuf;
#define EXPOP_PARAM 
#else
        int64           establishers_handle;
        int64           establishers_invo_handle;
        jmp_buf*        jmpbuf;
#define EXPOP_PARAM excp
#endif
	i4		(*handler)();
	int64		invo_context_fp;
	int64		establishers_fp;
	unsigned int floovemat = MTH$_FLOOVEMAT;	/* lib$match_cond wants */
	unsigned int floundmat = MTH$_FLOUNDMAT;	/* these by reference. */
	int64		invo_value = EX_DECLARE;
	int		status;
	int		handler_ret;
	unsigned int	tst_cond;

	EX_ARGS		exargs;
	int		*sig_args_ptr;
	int		except_summary;

	uint64		invo_mask;			/* lib$put_invo_... */
	EX_CONTEXT      *env = NULL;
        EX_CONTEXT      **excp;

        extern STATUS cs_handler();

	/*
	** Mask indicating which integer registers we want to restore.  It
	** includes all of the conventional saved (R2-15), but none of the
	** scratch (R22-24) registers.
	*/
	const uint32	int_mask = 0x0FFFCul;

	/*
	** Mask indicating which floating-point registers we want to restore.
	** It includes only the conventional saved registers (F2-9).
	*/
	const uint32	fp_mask = 0X03FCul;

	/*
	** Have we seen a problem from which we should not continue?
	*/
	i4		dont_continue = FALSE;

	i4	save_sigs;

#ifdef OS_THREADS_USED

        /* Ignore EXIT_THREAD signals */

        if (lib$match_cond(&sigs->chf$l_sig_name, &CMA$_EXIT_THREAD))
	    return (SS$_RESIGNAL);

#endif

	/*  get the frame pointer of our establisher */
#if defined(axm_vms)
	establishers_fp = mechs->chf$q_mch_frame & 0xffffffff;

	/*  get the user handler declared for us */

	excp = i_EXtop();
	for (env = *excp; env ; env = i_EXnext(env))
	{
	    jmpbuf = env->iijmpbuf;
	    invo_context_fp = jmpbuf[33] & 0xffffffff;
	    if (invo_context_fp == establishers_fp)
		break;
	}
#else /* axm_vms */
        establishers_invo_handle = mechs->chf$q_mch_invo_handle;

        excp = i_EXtop();
        env = excp ? *(excp) : NULL;
        for (; env; env = i_EXnext(env))
        {
           int64 invo_handle = 0;
           __int64 *aligned_env;
           INVO_CONTEXT_BLK* icb;
           int sts;

           jmpbuf = &env->iijmpbuf;

           /* 
           ** This ghastly kludge is due to the fact that the VMS setjmp and longjmp
           ** attempts to octaword align the jmp_buf (also known as an INVO_CONTEXT_BLK)
           ** in order to cope with TIE related ICBS that may not be aligned.
           ** So if we wish to examine the jmp_buf externally as an INVO_CONTEXT_BLK
           ** we're going to have to do the same ...
           **
           */

           aligned_env = (__int64*)((char*)(env) + sizeof(__int64));
           aligned_env = (__int64*)((__int64)((char*)(aligned_env) + 15) & ~15);
           icb = (struct invo_context_blk *) aligned_env;
           sts = lib$i64_get_invo_handle(icb, &invo_handle);
           if (!(sts & STS$M_SUCCESS)) /* returns 0 == fail, 1==success */
           {
              /* if we carry on after this we'll just get an accvio ... so resignal */
              return (SS$_RESIGNAL);
           }
           if (invo_handle == establishers_invo_handle) break;
        }

        if (env == NULL)
        {
           if (sigs->chf$l_sig_name == SS$_UNWIND)
           {
              return (SS$_RESIGNAL);
           }
           else
           {
              lib$signal (SS$_DATALOST);
           }
        }
#endif

	handler = env->handler_address;

	/*
	**  If we're called for an UNWIND, call the user's handler with
	**  SS$_UNWIND and a zero argument count.
	*/

	tst_cond = SS$_UNWIND;
	if (lib$match_cond(&sigs->chf$l_sig_name, &tst_cond))
	{
	    exargs.exarg_num = EX_UNWIND;
	    exargs.exarg_count = 0;
	    handler_ret = (*handler)(&exargs);

	    i_EXpop(excp);
	    return ( SS$_CONTINUE );    /* return value ignored during unwind */
	}

        /* Have we been signalled to dump the current stack ? */

	tst_cond = SS$_IMGDMP;
	if (lib$match_cond(&sigs->chf$l_sig_name, &tst_cond))
	{
	    save_sigs = sigs->chf$l_sig_args;
	    if ( 3 >= sigs->chf$l_sig_args )
	       sigs->chf$l_sig_args = 0;
	    else
	       sigs->chf$l_sig_args -= 3;

            /* Use the cs_handler to dump the stack and display the error messages
            ** as this will correctly identify the process.
            */
            cs_handler( (EX_ARGS*)sigs );

	    return ( SS$_CONTINUE );
	}

	/*
	**  If this condition is from the MTH$ facility change the condition to
	**  a CL error before calling the user handler.
	*/

	if (EXTRACT_FACILITY(sigs->chf$l_sig_name) == MTH$_FACILITY)
	{
	    TRdisplay("EXcatch: MTH$ Facility caused unexpected exception\n");

	    if (lib$match_cond(&sigs->chf$l_sig_name, &floovemat))
		sigs->chf$l_sig_name = SS$_FLTOVF;
	    else if (lib$match_cond(&sigs->chf$l_sig_name, &floundmat))
		sigs->chf$l_sig_name = SS$_FLTUND;
	    else
		sigs->chf$l_sig_name = MH_BADARG;

	    /*
	    ** Do not continue from this error.  We shouldn't EVER get here,
	    ** and if we do, we won't be able to determine where the return 
	    ** value has to be placed,
	    */
	    dont_continue = TRUE;
	}

	/*
	**  If the condition is SS$_HPARITH, look at which bit is set in the
	**  signal array's Exception Summary word and make the condition a
	**  corresponding CL condition.  Reset the signal array argument count
	**  to 1, saying that there's only one meaningful entry - there are no
	**  arguments to copy for the user's handler.
	**
	**  (For the inexact rounding condition, just return SS$_CONTINUE and
	**  go on.)
	*/

	tst_cond = SS$_HPARITH;
	if (lib$match_cond(&sigs->chf$l_sig_name, &tst_cond))
	{
	    sigs->chf$l_sig_args = 1;
	    sig_args_ptr = &sigs->chf$l_sig_arg1;
	    except_summary = *(sig_args_ptr + 2);

            /* Use the Standard Exception value for the detected
            ** HPARITH exception
            */

	    if (except_summary & HPARITH_BADARG)
		sigs->chf$l_sig_name = MH_BADARG;
	    else if (except_summary & HPARITH_FLTDIV)
		sigs->chf$l_sig_name = EXFLTDIV;
	    else if (except_summary & HPARITH_FLTOVF)
		sigs->chf$l_sig_name = EXFLTOVF;
	    else if (except_summary & HPARITH_FLTUND)
		sigs->chf$l_sig_name = EXFLTUND;
	    else if (except_summary & HPARITH_NOTEXACT)
		return (SS$_CONTINUE);	    /* Not an error to handle */
	    else if (except_summary & HPARITH_INTOVF)
		sigs->chf$l_sig_name = EXINTOVF;

	    /*
	    ** Since we can't determine the offending instruction or fixup
	    ** any operands, make sure no attempt is made to continue.
	    */
	    dont_continue = TRUE;
	}


	/*
	** The chf$l_sig_args field in the signal argument vector contains
	** the number of longwords in the vector not including the
	** chf$l_sig_args field. This count includes the condition value
	** PSL and PC which are contained in the vector. In order to
	** transform this into a EX_ARGS structure simply subtract 3 (the 
	** number of arguments not needed) from the chf$l_sig_args field and 
	** cast it to the correct type.
	**
	** When changing a data structure that is originated from OpenVMS 
	** system calls and the data is returned to other system calls, the 
	** data should not be modified unless we know that there will be no 
	** adverse effect. Beginning with OpenVMS 7.0, not resetting the 
	** value of sigs->chf$l_sig_args results in access violations in 
	** system routines (contsignal). 
	*/

	save_sigs = sigs->chf$l_sig_args;
	if ( 3 >= sigs->chf$l_sig_args )
	    sigs->chf$l_sig_args = 0;
	else
	    sigs->chf$l_sig_args -= 3;
	handler_ret = (*handler)((EX_ARGS*)sigs);
        sigs->chf$l_sig_args = save_sigs;
	/*
	**  Return to the operating system, passing a return code based on what
	**  the user's handler returned to us.
	*/

	switch(handler_ret)
	{
	case EX_CONTINUE:
	    if ( dont_continue )
		TRdisplay( "EXcatch(): ignoring EX_CONTINUE return from user\
 handler, will resignal.\n" );
	    else
		return ( SS$_CONTINUE );
	    /* May fall through... */

	case EX_RESIGNAL:
	    return ( SS$_RESIGNAL );

	case EX_DECLARE:
	    /*  get the handle of our establisher */
#if defined(axm_vms)
	    establishers_handle = lib$get_invo_handle( jmpbuf );

	    /*
	    ** Reset the registers in our target function to the values they
	    ** had when lib$get_current_invo_context() was called.  We want
	    ** to restore only the interesting registers.  Tend to corrupt
	    ** the stack (badly) if we restore too much.
	    */
	    invo_mask = int_mask | ((uint64)fp_mask << (BITS_IN(uint64) / 2));

	    status = lib$put_invo_registers( establishers_handle, jmpbuf,
					    &invo_mask );

	    /*  Unwind to our establisher, make a non-zero return */
	    sys$goto_unwind(&establishers_handle, jmpbuf+2, &invo_value, 0);
#else /* axm_vms */
            longjmp(jmpbuf, EX_DECLARE);
#endif
	    break;

	default:
	    TRdisplay("EXcatch:  unknown return from user handler = 0x%x, \
resignaling ...\n", handler_ret);
	    return ( SS$_RESIGNAL );
	}
}

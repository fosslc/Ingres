
/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<gl.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<math.h>		/* header file for math package */
# include	<errno.h>		/* header file for error checking */
# include	<ex.h>			/* header file for exceptions */
# include	<mh.h>
# include	"mhlocal.h"

/*
NO_OPTIM = sgi_us5
*/

/**
** Name: MHEXP.C - Find the exponential of x (i.e., e**x).
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
**      This file contains the following external routines:
**    
**	MHexp()		   -  Find the exponential of x (i.e., e**x).
**
** History:
 * Revision 1.1  90/03/09  09:15:53  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:47  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:11  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:15:55  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:56  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:07:29  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:42  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:27  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:04  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-jul-97 (hweho01) 
**	    Determine the type of floating point number by 
**	    calling finite() on UnixWare(usl_us5).
**	15-oct-1998 (walro03)
**	    Detect overflow condition by calling finite() on AIX (rs4_us5).
**	20-oct-98 (muhpa01) 
**          Determine if result overflowed by calling isfinite() on hpb_us5
**          (HP-UX 11.00).
**	12-may-99 (hweho01) 
**          Determine if result overflowed by calling finite() on ris_u64
**          (AIX 64-bit).
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	07-Aug-2001 (hanje04)
**          Determine if result overflowed by calling finite() on Linux.
**          (Intel, Alpha and s390).
**	04-Dec-2001 (hanje04)
**	    IA64 Linux to use finite().
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	22-Oct-2003 (bonro01)
**	    Unoptimize for sgi to eliminate float errors in MHexp
**	    where it would miss overflow errors.
**	20-Apr-2005 (hanje04)
**	     As most platforms now seem to need to use the finte(result) 
**	     call, use xCL_FINITE_EXISTS to see if we use it.
**	04-May-2005 (lakvi01)
**	    Unoptimize MHexp() for a64_win since it is unable to catch
**	    overflow errors.
**	01-Nov-2007 (hanje04)
**	    SIR 114907
**	    OSX defines xCL_FINITE_EXISTS but should use isfinite()
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	10-Aug-2009 (drivi01)
**	    Add pragma to remove optimization from this file 
**          for all Windows platforms not just 64-bit.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */

/*{
** Name: MHexp() - Exponential.
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
** Inputs:
**      x                               Number to find exponential for.
**
** Outputs:
**      none
**
**	Returns:
**	    exp(x)
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**      21-jul-87 (mmm)
**          Upgraded UNIX cl to Jupiter coding standards.
**      21-may-2002 (huazh01 for hayke02)
**           For nc4_us5 declare errno locally. 
**           This change fixes bug 103611.
**	20-Apr-2005 (hanje04)
**	     As most platforms now seem to need to use the finte(result) 
**	     call, use xCL_FINITE_EXISTS to see if we use it.
**	04-May-2005 (lakvi01)
**	    Unoptimize MHexp() for a64_win since it is unable to catch
**	    overflow errors.
*/
#ifdef NT_GENERIC
#pragma optimize ("",off)
#endif
f8
MHexp(x)
f8	x;
{
	f8	result;
	f8	ret_val	= 0.0;

#ifdef nc4_us5
        i4      errno = NOERR;
#else
	errno  = NOERR;
#endif

	result = exp(x);

	switch (errno)
	{
		case NOERR:
#if defined(thr_hpux) || defined(OSX)
               		if (!isfinite(result)) 
			    EXsignal(EXFLTOVF, 0);
#elif defined(xCL_FINITE_EXISTS)
			if (!finite(result)) 
                    	    EXsignal(EXFLTOVF, 0);
#endif
			ret_val = result;
			break;
		case EDOM:
			EXsignal(MH_BADARG, 0);
			break;
		case ERANGE:
			EXsignal(EXFLTOVF, 0);
			break;
		default:
			EXsignal(MH_INTERNERR, 0);
			break;
	}

	return(ret_val);
}

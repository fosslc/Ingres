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

/**
** Name: MHIPOW.C - Integer power.
**
** Description:
**	Find x**i.
**
**      This file contains the following external routines:
**    
**	MHipow()	   -  Integer power.
**
** History:
 * Revision 1.1  90/03/09  09:15:54  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:00  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:19  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:03  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:57  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.3  88/11/02  14:07:09  jeff
 * now accept a negative exponent
 * 
 * Revision 1.2  88/10/31  11:07:52  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:49  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:55  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:39  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:10  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-jul-97 (hweho01)
**	    Detect the overflow condition by calling
**	    finite() on UnixWare(usl_us5).
**	15-oct-1998 (walro03)
**	    Detect overflow condition by calling finite() on AIX (rs4_us5).
**	20-oct-98 (muhpa01)
**          Determine if result overflowed by calling isfinite() on hpb_us5
**          (HP-UX 11.00).
**      12-may-99 (hweho01)
**          Determine if result overflowed by calling finite() on ris_u64
**          (AIX 64-bit).
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      28-Aug-2001 (hanje04)
**          Determine if result overflowed by calling finite() on Linux
**          (Intel, Alpha and S390).
**	04-Dec-2001 (hanje04)
**	    IA64 Linux will use finite().
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**      09-May-2005 (hanal04) Bug 114470
**          Submission of open files on nc4_us5 build. errno must be declared.
**	01-Nov-2007 (hanje04)
**	    SIR 114907
**	    OSX defines xCL_FINITE_EXISTS but should use isfinite()
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */

/*{
** Name: MHipow() - Integer power.
**
** Description:
**	Find x**i.
**
** 	UNIX math package will not handle case where y is negative and 
**	non-integral.  It returns 0 and set errno to EDOM.
**
** Inputs:
**      x                               Number to raise to a power.
**      i                               Integer power to raise it to.
**
** Outputs:
**      none
**
**	Returns:
**	    x**i
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
**      21-jun-2002 (huazh01 for hayke02)
**          For nc4_us5 decalre errno locally. 
**          This change fixes bug 103611.
*/
f8
MHipow(x, i)
f8	x;
i4	i;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

	if (i >= 0)
	{
	    result = pow(x, (f8) i);
	}
	else
	{
	    result = 1.0 / pow(x, (f8) -i);
	}
	switch(errno)
	{
		case NOERR:
# ifdef	SUN_FINITE
			if (!finite(result))
			    EXsignal(EXFLTOVF, 0);
# endif

#if defined(usl_us5) || defined(any_aix) || defined(LNX)
			if (!finite(result))
			    EXsignal(EXFLTOVF, 0);
#endif /* usl_us5 aix lnx */

#if defined(thr_hpux) || defined(OSX)
           if (!isfinite(result))
              EXsignal(EXFLTOVF, 0);
#endif
			return(result);
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
	return(0.0);
}

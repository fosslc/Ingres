/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<gl.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>
# include	<math.h>		/* header file for math package */
# include	<errno.h>		/* header file for error checking */
# include	<clsigs.h>		/* header file for signal definitions */
# include	<ex.h>			/* header file for exceptions */
# include	<mh.h>
# include	"mhlocal.h"

/**
** Name: MHPOW.C - raise number to a power.
**
** Description:
**
** 	raise number to a power.  Find x**y.
**
**      This file contains the following external routines:
**    
**	MHpow()		   -  Find x**y.
**
** History:
 * Revision 1.2  90/05/24  15:00:32  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:55  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:13  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:26  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:11  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:58  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:08:32  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:50  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:52:07  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:51  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:15  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      24-may-90 (blaise)
**          Integrated changes from ingresug:
**              Change include <signal.h> to <clsigs.h>;
**		Added include <config.h>.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.
**    30-dec-92 (mikem)
**        su4_us5 port.  Fixed cast of 1st argument to setjmp() - it should
**        be a jmp_buf not a jmp_buf *.  This generated a compiler warning
**        on su4_us5.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-may-97 (mcgem01)
**	    Clean up compiler warning.
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
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      28-Aug-2001 (hanje04)
**          Determine if result overflowed by calling finite() on Linux
**          (Intel, Alpha and s390).
**	04-Dec-2001 (hanje04)
**	    IA64 Linux will use finite().
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	20-Apr-2005 (hanje04)
**	     As most platforms now seem to need to use the finte(result) 
**	     call, use xCL_FINITE_EXISTS to see if we use it.
**      09-May-2005 (hanal04) Bug 114470
**          Submission of open files on nc4_us5 build. errno must be declared.
**	20-Oct-2005 (bonro01)
**	    Overflow detection on hp2.us5 and i64.hpu was broken by a
**	    syntax error in the previous change.
**	01-Nov-2007 (hanje04)
**	    SIR 114907
**	    OSX defines xCL_FINITE_EXISTS but should use isfinite()
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	29-Nov-2010 (frima01) SIR 124685
**	    Add forward declaration of powsig.
**/

/* # defines */
/* typedefs */
/* forward references */
i4 powsig(i4 signal );

/* externs */
/* statics */

/* return from floating point exception */

static jmp_buf powjmp;


/*{
** Name: MHpow() - raise number to a power.
**
** Description:
**	Find x**y.
**
** 	UNIX math package will not handle case where y is negative and 
**	non-integral.  It returns 0 and set errno to EDOM.
**
** Inputs:
**      x                               Number to raise to a power.
**      y                               Power to raise it to.
**
** Outputs:
**      none
**
**	Returns:
**	    x**y
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
**          Upgraded UNIX CL to Jupiter coding standards.
**      21-jun-2002 (huazh01 for hayke02)
**          For nc4_us5 decalre errno locally.
**          This change fixes bug 103611. 
*/

f8
MHpow(x, y)
f8	x;
f8	y;
{
	f8	result;
	i4	powsig();
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

	if (y >= 0.0)
	{
	    result = pow(x, y);
	}
	else
	{
	    result = 1.0 / pow(x, -y);
	}

	switch(errno)
	{
		case NOERR:
#if defined(thr_hpux) || defined(OSX)
               		if (!isfinite(result)) 
			    EXsignal(EXFLTOVF, 0);
#elif defined(xCL_FINIT_EXISTS)
			if (!finite(result)) 
                    	    EXsignal(EXFLTOVF, 0);
#endif
			return(result);
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

/* catch the floating point exception and raise flag */

i4
powsig( signal )
i4  signal;
{
	longjmp( powjmp, 1 );
}

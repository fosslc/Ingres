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
** Name: MHSQRT.C - Square root.
**
** Description:
**	Find the square root of x.
**
**      This file contains the following external routines:
**    
**	MHceil()	   -  Find the square root of x.
**
** History:
 * Revision 1.1  90/03/09  09:15:56  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:26  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:34  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:21  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:59  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:08:45  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:53  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.4  88/02/12  10:56:16  anton
**      removed MH_SQRNEG exception
**      
**      Revision 1.3  87/11/13  13:52:26  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:56:04  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:23  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> for AIX 3.1
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      09-May-2005 (hanal04) Bug 114470
**          Submission of open files on nc4_us5 build. errno must be declared.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */

/*{
** Name: MHsqrt() - Square root.
**
** Description:
**	Find the square root of x.
**	Only works if x is non-negative.
**
** Inputs:
**      x                               Number to find square root of.
**
** Outputs:
**      none
**
**	Returns:
**	    Square root of x.
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
**	12-feb-88 (anton)
**	    Removed MH_SQRNEG execption
**      21-jun-2002 (huazh01 for hayke02)
**          For nc4_us5 declare errno locally.
**          This change fixes bug 103611. 
*/
f8
MHsqrt(x)
f8	x;
{
	f8	result;
	f8	ret_val = 0.0;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

#ifdef nc4_us5
        i4      errno = NOERR;
#else
        errno = NOERR;
#endif

	if (x >= 0.0)
	{
	    result = sqrt(x);
	}
	else
	{
	    errno = EDOM;
	}
	switch (errno)
	{
	case NOERR:
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

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
** Name: MHCOS.C - Cosine
**
** Description:
**	Find the cosine of x, where x is in radians.
**
**      This file contains the following external routines:
**    
**	MHcos()		   -  Find the cos of a number.
**
** History:
 * Revision 1.1  90/03/09  09:15:53  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:43  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:09  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:15:52  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:55  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:07:17  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:46  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:36  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:23  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:01  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
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
** Name: MHcos() - Cosine.
**
** Description:
**	Find the cosine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take cosine of.
**
** Outputs:
**	none
**
**	Returns:
**	    Cosine of x radians.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**	    MH_MATHERR
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
**          For nc4_us5 declare errno locally. 
**          This change fixes bug 103611.
*/
f8
MHcos(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

	result = cos(x);
	switch (errno)
	{
		case NOERR:
# ifdef	notdef
			if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
			{
				EXsignal(MH_MATHERR, 0);
			}
# endif
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

/*{
** Name: MHacos() - Arccosine.
**
** Description:
**	Find the arccosine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take arccosine of.
**
** Outputs:
**	none
**
**	Returns:
**	    Arccosine of x radians.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**	    MH_MATHERR
**
** Side Effects:
**	    none
**
** History:
**	2-may-2007 (dougi)
**	    Cloned from MHcos().
*/
f8
MHacos(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

	result = acos(x);
	switch (errno)
	{
		case NOERR:
# ifdef	notdef
			if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
			{
				EXsignal(MH_MATHERR, 0);
			}
# endif
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

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
** Name: MHSIN.C - Sine.
**
** Description:
**	Find the sine of x, where x is in radians.
**
**      This file contains the following external routines:
**    
**	MHsin()	   	-  Sine.
**
** History:
 * Revision 1.1  90/03/09  09:15:56  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:22  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:31  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:16  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:59  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:08:39  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:52  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:52:15  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:59  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:21  mikem
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
** Name: MHsin() - Sine.
**
** Description:
**	Find the sine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take sine of.
**
** Outputs:
**	none
**
**	Returns:
**	    sine of x radians.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**      21-jun-2002 (huazh01)
**          For nc4_us5 declare errno locally.
**          This change fixes bug 103611.
*/
f8
MHsin(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif
	result = sin(x);
	switch(errno)
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
** Name: MHasin() - Arcsine.
**
** Description:
**	Find the arcsine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take arcsine of.
**
** Outputs:
**	none
**
**	Returns:
**	    arcsine of x radians.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-may-2007 (dougi)
**	    Cloned from MHsin().
*/
f8
MHasin(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif
	result = asin(x);
	switch(errno)
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

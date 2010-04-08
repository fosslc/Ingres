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
** Name: MHATAN.C - Find the arctangent of x.  
**
** Description:
** 	Find the arctangent of x.  Returned result is in radians.
**
**      This file contains the following external routines:
**    
**	MHatan()	   -  Find the arctangent of x.
**	MHatan2()	   -  Find the arctangent of x and y.
**	MHtan()		   -  Find the tangent of x.
**
** History:
 * Revision 1.1  90/03/09  09:15:53  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:35  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:04  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:15:47  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:54  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:04:35  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:45  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:50:50  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:12  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:30:56  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: MHatan() - Arctangent.
**
** Description:
**	Find the arctangent of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Number to find the arctangent of.
**
** Outputs:
**      none
**
**	Returns:
**	    Returned result is in radians.
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
**          For nc4_us5 declare errno locally. 
**          This change fixes bug 103611.
**      09-May-2005 (hanal04) Bug 114470
**          Submission of open files on nc4_us5 build. errno must be declared.
*/
f8
MHatan(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif
	result = atan(x);
	switch (errno)
	{
		case NOERR:
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
	return(0);
}

/*{
** Name: MHatan2() - Arctangent.
**
** Description:
**	Find the arctangent of x and y.
**	Returned result is in radians.
**
** Inputs:
**      x                    X-coordinate to find the arctangent of.
**      y                    y-coordinate to find the arctangent of.
**
** Outputs:
**      none
**
**	Returns:
**	    Returned result is in radians.
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
**	3-may-2007 (dougi)
**	    Cloned from MHatan().
*/
f8
MHatan2(x, y)
f8	x;
f8	y;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif
	result = atan2(x, y);
	switch (errno)
	{
		case NOERR:
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
	return(0);
}

/*{
** Name: MHtan() - Tangent.
**
** Description:
**	Find the tangent of x.
**	Argument is in radians.
**
** Inputs:
**      x                               Number to find the tangent of.
**
** Outputs:
**      none
**
**	Returns:
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
**	2-may-2007 (dougi)
**	    Cloned from MHatan().
*/
f8
MHtan(x)
f8	x;
{
	f8	result;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif
	result = tan(x);
	switch (errno)
	{
		case NOERR:
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
	return(0);
}

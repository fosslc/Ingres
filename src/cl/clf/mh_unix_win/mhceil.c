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
** Name: MHCEIL.C - returns the smallest integer >= the argument passed to it.
**
** Description:
**	returns the smallest integer not less than the
**	argument passed to it.
**
**      This file contains the following external routines:
**    
**	MHceil()	   -  Find the ceil of a number.
**
** History:
 * Revision 1.1  90/03/09  09:15:53  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:39  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:06  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:15:49  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:55  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/10/31  11:05:58  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:46  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:30  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:19  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:30:59  mikem
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
** Name: MHceil() - Ceiling.
**
** Description:
**	Returns the smallest integer not less than the
**	argument passed to it.  The return value, although
**	an integer, is returned as an f8.
**
** Inputs:
**      x                               Number to find ceiling for.
**
** Outputs:
**	none
**
**	Returns:
**	    The smallest integer (as an f8) not less than x.
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
*/
f8
MHceil(x)
f8	x;
{
	return(ceil(x));
}

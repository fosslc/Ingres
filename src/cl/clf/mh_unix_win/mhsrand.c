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
** Name: MHSRAND.C - Set random number generator.
**
** Description:
**	Initialize random number generator to a particular
**	starting point.  This should just plain work with no problems.
**
**      This file contains the following external routines:
**    
**	MHsrand()	   -  Set random number generator.
**
** History:
 * Revision 1.1  90/03/09  09:15:56  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:30  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:36  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:23  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:00  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:42:53  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:52:30  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:56:10  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:27  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	09-apr-91 (seng)
**	    Add <systypes.h> to clear problems with <sys/types.h> included
**	    in <math.h>
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */

/*{
** Name: MHsrand() - Set random number generator.
**
** Description:
**	Initialize random number generator to a particular
**	starting point.  This should just plain work with no problems.
**
**	Note:   This routine sets a static variable, which is normally
**		discouraged in a server environment.  However, due to the
**		nature of the random number generator, it will probably not
**		effect the functioning of the MHrand() routine if many sessions
**		are actively sharing this variable.  In fact, it may indeed
**		give an added randomness to the routine!
**		
** Inputs:
**      seed                            Just any ol' number.
**
** Outputs:
**      none
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets a static variable.  (See note in description above.)
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter coding standards.
*/
STATUS
MHsrand(seed)
i4	seed;
{
	srand(seed);
	return(OK);
}

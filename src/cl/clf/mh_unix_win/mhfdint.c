
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
** Name: MHFDINT.C - Take the integer part of the passed in double
**
** Description:
**      Take the integer part of the passed in
**	double and return it as a double.
**
**      This file contains the following external routines:
**    
**	MHfdint()	   -  Find the ceil of a number.
**
** History:
 * Revision 1.1  90/03/09  09:15:54  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:56  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:16  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:01  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:56  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:42:48  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:51  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:35  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:08  mikem
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
** Name: MHfdint() - Take integer part of a double.
**
** Description:
**      Take the integer part of the passed in
**	double and return it as a double.
**
** Inputs:
**      x                               The double to take the integer part of.
**
** Outputs:
**      none
**
**	Returns:
**	    Integer part of x (as an f8).
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
**      11-feb-93 (smc)
**          Commented out text after endifs.
*/

/* lint doesn't know about asm code so thinks val is unused */
/* ARGSUSED */

f8
MHfdint(val)
f8	val;
{
# define	FDINT_DEFAULT

# if defined(VAX) && defined(BSD)
	asm("emodd 4(ap), $0, $0f1.0, r0, r0");
	asm("subd3 r0, 4(ap), r0");

# undef		FDINT_DEFAULT
# endif /* VAX and BSD */

# ifdef	FDINT_DEFAULT
	return ((val >= 0.0 ) ? floor(val) : ceil(val));
# endif	/* FDINT_DEFAULT */
}

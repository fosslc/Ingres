/*
**	Copyright (c) 1987, 1999 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<gl.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>		/* for TYPESIG */
# include	<math.h>		/* header file for math package */
# include	<errno.h>		/* header file for error checking */
# include	<ex.h>			/* header file for exceptions */
# include    	<me.h>
# include    	<meprivate.h>
# include	<mh.h>
# include	<cs.h>
# include	"mhlocal.h"

/**
** Name: MHRAND.C - Random number.
**
** Description:
**	Return the next random number.
**
**      This file contains the following external routines:
**    
**	MHrand()	   -  Return the next random number.
**
** History:
 * Revision 1.1  90/03/09  09:15:55  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:44:18  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:29  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:16:13  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:58  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:42:51  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:52:11  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:55  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:18  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> for AIX 3.1
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	15-Jan-1999 (shero03)
**	    Add MHrand2
**	01-Mar-1999 (shero03)
**	    Use TLS seed when available
**	05-May-1999 (popri01)
**	    Extend shero03's TLS seed logic to MHrand for
**	    Unixware (usl_us5), whose reentrant version of 
**	    rand also requires a seed.
**/

/* # defines */
/* typedefs */

typedef void FP_Callback(CS_SCB **scb);
/* forward references */
/* externs */
FUNC_EXTERN void FP_get_scb(CS_SCB **scb);
/* statics */

/* jseed must be loaded with an initial value of some sort. */
static u_i4 jseed = 734221605;
static ME_TLS_KEY  MHrandomkey = 0;

static u_i4 *FindSeed(void);

/*{
** Name: MHrand() - Random number.
**
** Description:
**	Return the next random number.
**	This also should just plain work with no problems.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    A random number as an f8.
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
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter coding standards.
**	05-may-99 (popri01)
**	    Use reentrant version of rand, which requires use
**	    of seed logic as in MHrand2.
*/
f8
MHrand()
{
# ifdef usl_us5
    u_i4	*seedptr;
    CS_SCB      *scb;

    FP_get_scb(&scb);			/* see if we have sessions */
    if (scb)
    {	
    	seedptr = &scb->cs_random_seed;	/* yes, and its non 0 use it */
	if (*seedptr == 0)
	   seedptr = &jseed;
    }
    else
    	seedptr = &jseed;
    return ( rand_r(seedptr) % MAXI2 ) / (f8) MAXI2;
# else
	return( ( rand() % MAXI2 ) / (f8) MAXI2 ); 
# endif

}

/*{
** Name: MHrand2() - Random number.
**
** Description:
**	Return the next random integer.
**	This routine improves the standard ANSI algorithm by using more
**	high order bits.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    A random number as an u_i4.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jan-1999 (shero03)
**          Created.
**	01-Mar-1999 (shero03)
**	    Use TLS seed when available
*/
u_i4
MHrand2()
{
    u_i4	*seedptr;
    CS_SCB 	*scb;
    
    FP_get_scb(&scb);			/* see if we have sessions */
    if (scb)
    {	
    	seedptr = &scb->cs_random_seed;	/* yes, and its non 0 use it */
	if (*seedptr == 0)
	   seedptr = &jseed;
    }
    else
    	seedptr = &jseed;
    *seedptr = *seedptr * 1103515245 + 12345;
    return( (*seedptr >> 7) & MH_MAXRAND2INT );
}

/*{
** Name: MHsrand2() - Set random number generator for algorithm 2.
**
** Description:
**	Initialize random number generator 2 to a particular
**	starting point. 
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
**      19-Jan-1999 (shero03)
**          Created.
**	01-Mar-1999 (shero03)
**	    Use TLS seed when available
*/
STATUS
MHsrand2(seed)
i4	seed;
{
    CS_SCB 	*scb;

    FP_get_scb(&scb);			/* see if we have sessions */
    if (scb)
	scb->cs_random_seed = seed;
    else
    	jseed = seed;
    return(OK);
}

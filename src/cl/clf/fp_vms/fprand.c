/*
**    Copyright (c) 1993, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <cs.h>
#include    <fp.h>

/*
**
**  Name: FP.C - Floating Point functions for the CL.
**
**  Description:
**      This file contains the random number generation parts of the floating
**	point functions defined by the CL spec as being part of the FP module.
**
**          FPsrand() - Set random number generator.
**          FPrand() - Random number.
**
**
**  History: 
**      08-apr-93 (rog)
**          Created from mh.c.
**	01-Mar-1999 (shero03)
**	    Use TLS seed when available
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	    clean up compiler warnings
*/

typedef void FP_Callback(CS_SCB **scb);
FUNC_EXTERN void FP_set_callback( FP_Callback fun);
FUNC_EXTERN void FP_get_scb(CS_SCB **scb);

/* gseed must be loaded with an initial value of some sort. */
static u_i4 gseed = 734221605;
static FP_Callback *localcallback = NULL;


/*{
** Name: FPsrand() - Set random number generator.
**
** Description:
**	Initialize random number generator to a particular
**	starting point.  This should just plain work with no problems.
**
**(	Note:   This routine sets a static variable, which is normally
**		discouraged in a server environment.  However, due to the
**		nature of the random number generator, it will probably not
**		effect the functioning of the FPrand() routine if many sessions
**		are actively sharing this variable.  In fact, it may indeed
**		give an added randomness to the routine!
**)		
** Inputs:
**      seed                            Just any ol' number.
**
** Outputs:
**      None
**
** Returns:
**	None
**
** Exceptions:
**	None
**
** Side Effects:
**	Sets a static variable.  (See note in description above.)
**
** History:
**      22-apr-1992 (rog)
**          Created from MHsrand().
**      01-Mar-1999 (shero03)
**          Use TLS seed when available
*/


VOID
FPsrand(seed)
i4	seed;
{
    CS_SCB      *scb;

    FP_get_scb(&scb);                   /* see if we have sessions */

    if (scb)
        scb->cs_random_seed = seed;
    else
         gseed = seed;
}


/*{
** Name: FPrand() - Random number.
**
** Description:
**	Return the next random number.
**	This also should just plain work with no problems.
**
** Inputs:
**      None
**
** Outputs:
**	None
**
** Returns:
**	A random number as an double.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      08-apr-1993 (rog)
**          Created from Knuth's writings on random number generation.
**      01-Mar-1999 (shero03)
**          Use TLS seed when available
*/

double
FPrand()
{
    double t;
    double      dret;
    u_i4        *seedptr;
    CS_SCB      *scb;

    /*
    ** Note: Knuth specifically states that randomly playing with
    ** M, A, and, C won't help you get even better random numbers.
    */

    /* M should be >= 2^30; this is 2^31 */
#define M 2147483648.0

    /*
    ** "A" should satisfy: A mod 8 = 5.  "A" should also be between
    ** 0.01M and 0.99M.  This number is 0.01M + 1.0, which satifies
    ** both requirements.  "A" should also have fairly random digits.
    */
#define	A 21474837.0

    /*
    ** Either go read Knuth, or just leave C alone.
    */
#define C 1.0

    /*
    ** X = ((A * X) + C) % M;
    ** (Can't use the % operator on doubles.)
    */

    FP_get_scb(&scb);                   /* see if we have sessions */
    if (scb)
    {
        seedptr = (u_i4 *)&scb->cs_random_seed; /* yes, and its non 0 use it */
        if (*seedptr == 0)
           seedptr = &gseed;
    }
    else
        seedptr = &gseed;

    t = ((A * *seedptr) + C) / M;
    *seedptr = M * (t - FPfdint(&t));
    dret = *seedptr/M;

    return (dret);
}

/*{
** Name: FP_get_scb - Retrieve the SCB (if one exists ).
**
** Description:
**      Return the address of the SCB, if running sessions,
**      else return NULL.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      Addr(SCB) if sessions, NULL otherwise
**
** Exceptions:
**      None
**
** History:
**      11-Mar-1999 (shero03)
**          Created from Orlan's notes.
*/

void
FP_get_scb(CS_SCB **scb)
{
    if (localcallback)
        (*localcallback)(scb);
    else
        *scb = (CS_SCB *)NULL;
    return;
}

/*{
** Name: FP_set_callback - Set the address of the callback routine.
**
** Description:
**      Establish an address for the local callback function.
**      If this is part of a server, it is called from CS_Dispatch
**      so that FP_get_scb uses CSget_SCB.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      Addr(SCB) if sessions, NULL otherwise
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      11-Mar-1999 (shero03)
**          Created from Orlan's notes.
*/

void
FP_set_callback( FP_Callback fun)
{
        localcallback = fun;
}


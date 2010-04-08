/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <sp.h>
# include <mo.h>
# include <mu.h>
# include "moint.h"

/**
** Name:	momutex.c	- mutual exclusion for MO
**
** Description:
**	This file defines:
**
**	MO_mutex	- obtain MO lock
**	MO_unmutex	- release MO lock
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	1-oct-92 (daveb)
**	    changed to use MU sems.
**	13-Jan-1993 (daveb)
**	    Change to handle deadlock w/counting sem.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF MU_SEMAPHORE MO_sem;
GLOBALREF i4  MO_semcnt;


/*
** Name:  MO_mutex - obtain MO lock
**
**	Gain control of the MO mutual exclusion lock, blocking other
**	calls until a matching MO_unmutex is called.  It is a logic
**	error that may (or may not) be detected to call this from a
**	process/thread that already holds the lock.
**
** Inputs:
**	None.
**
** Outputs:
**	err	filled with detailed error status if return != OK,
**		otherwise left untouched.
**
** Returns:
**
**	OK if the mutex was acquired.  other system specific error
**	status, possibly including deadlock with self if detectable.
**
**  History:
**	24-sep-92 (daveb)
**	    documented
**	13-Jan-1993 (daveb)
**	    Handle deadlock/counting semaphore case.  All errors
**	    are treated as deadlock if the semaphore is held.
**	    This isn't stricly correct, but is all we can do since
**	    we don't know all the E_???_DEADLOCK codes that might
**	    be returned.
**      21-jan-1999 (canor01)
**          Remove erglf.h.
*/
STATUS
MO_mutex(void)
{
    STATUS stat = OK;

    if( !MO_disabled )
    {
	stat = MUp_semaphore( &MO_sem );

	/* Assume any error when sem held is deadlock, which we count */
	   
	if( stat != OK && MO_sem.mu_value == 1 )
	    stat = OK;

	if( stat == OK )
	{
	    MO_semcnt++;
	    MO_nmutex++;
	}
    }
    return ( stat );
}


/*
** Name:  MO_unmutex - release MO lock
**
**	release control of the MO mutual exclusion lock, unblocking other
**	calls until a matching MO_unmutex is called.  It is a logic
**	error that may (or may not) be detected to call this from a
**	process/thread that doesn't already hold the lock.
**
** Inputs:
**	None.
**
** Outputs:
**	none.
**
** Returns:
**
**	OK if the mutex was released.  other system specific error
**	status.
**
**  History:
**	24-sep-92 (daveb)
**	    documented
**	13-Jan-1993 (daveb)
**	    Handle deadlock/counting semaphore case.
*/

STATUS
MO_unmutex(void)
{
    STATUS stat = OK;

    if( !MO_disabled )
    {
	MO_nunmutex++;
	if( --MO_semcnt == 0 )
	    stat = MUv_semaphore( &MO_sem );
    }
    return ( stat );
}

/* end of momutex.c */

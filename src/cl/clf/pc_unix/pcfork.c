# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cs.h>
# include	<pc.h>
# include	<qu.h>
# include	<ex.h>

# include	<clconfig.h>

# include	"pclocal.h"
# include	"pcpidq.h"
# include	<PCerr.h>

/*
** Copyright (c) 1985, 2004 Ingres Corporation
**
**	Name:
**		PCfork.c
**
**	Function:
**		PCfork	-- internal CL use only.
**
**	Arguments:
**		Pointer to a STATUS variable that will be filled in.
**
**	Result:
**		tries to execute a fork.  Returns -1 on error, with
**		status possibly filled in.
**
**	Side Effects:
**		Creates a new process, adds a Pidqueue entry so PCwait
**		can be used to reap it.
**
**	History:
**	31-Oct-1988 (daveb)
**		Created.
**	31-may-89 (daveb)
**		Correctly set exit status.
**	26-aug-89 (rexl)
**		Added calls to protect stream allocator.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	05-jun-1995 (canor01)
**	    semaphore protect memory allocation routine
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	26-mar-1997 (canor01)
**	    If using System V threads, call fork1() instead of fork().
**	17-dec-1997 (canor01)
**	    Initialize mutex for synchronization of OS threads.
**	19-dec-1997 (saeid)
**	    Add definition of "Pidq_mutex" and test the above modification.
**	23-jan-1998 (canor01)
**	    Clean up compile problems.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      07-jun-2000 (stial01)
**          Removed Pidq_mutex. (b101767, Startrak 9027036)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-nov-2010 (stephenb)
**	    Include ex.h for prototyping.
*/

# if defined(OS_THREADS_USED) && defined(SYS_V_THREADS)
# define FORKCMD	fork1
# else /* OS_THREADS_USED && SYS_V_THREADS */
# define FORKCMD	fork
# endif /* OS_THREADS_USED && SYS_V_THREADS */




i4
PCfork( status )
STATUS *status;
{
    register PIDQUE	*pp;		/* new descriptor for process */
    register int	pid = -1;	/* default is error value */
    
    /* allocate a Pid queue entry for this possible process. */

    pp = (PIDQUE *) MEreqmem(0, sizeof(*pp), TRUE, status);
    if( pp != NULL )
    {
	/* if the Pidq doesn't exist yet, make it now */
	if(Pidq_init == FALSE)
	{
	    QUinit(&Pidq);
	    Pidq_init = TRUE;
	}
    
	if( (pid = FORKCMD()) < 0 ) 	/* failure, free mem, set status */
	{
	    *status = PCno_fork();	/* parent, fork failed */
	    MEfree( (PTR)pp );	
	}
	else if( pid > 0 )		/* parent keeps track */
	{
	    pp->pid = pid;
	    pp->stat = PC_WT_EXEC;
	    (void)QUinsert( (QUEUE *)pp, &Pidq );
	}
	/* else in child, don't do anything */
    }
    return( pid );
}



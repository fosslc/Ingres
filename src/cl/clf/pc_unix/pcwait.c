# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<systypes.h>
# include       <clconfig.h>
# include       <clsigs.h>
# include	<ex.h>
# include	<cs.h>
# include	<pc.h>
# include	<PCerr.h>
# include	"pclocal.h"
# include	<errno.h>
# include	<qu.h>
# include	"pcpidq.h"

GLOBALDEF QUEUE	Pidq;
GLOBALDEF int Pidq_init = FALSE;

/*
** Copyright (c) 1985, 2004 Ingres Corporation
**
**	Name:
**		PCwait.c
**
**	Function:
**		PCwait
**
**	Arguments:
**		PID	proc;
**
**	Result:
**		wait for specified subprocess to terminate.
**
**		Returns: child's exit status
**				OK		-- success
**				PC_WT_BAD	-- child didn't execute, e.g bad parameters, bad or inacessible date, etc.
**				PC_WT_NONE	-- no children to wait for
**				PC_WT_TERM	-- child we were waiting for returned bad termination status
**
**	Side Effects:
**		None
**
**	History:
 * 
 * Revision 1.2  87/03/14  11:30:39  daveb
 * Take JAS's changes to allow user equel signal handling.
 * 
**Revision 30.1  85/08/14  19:16:55  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/29  12:16:14  wong
**		Corrected WECO code to return correct status
**		depending on status from the child.
**		
**		03/83	--	(gb)
**			written
**		06/87	--	(lin & daveb)
**			added a loop for wait() if the pid picked up is not the
**			one we want to wait for. Also changed the proc to the
**			the input pid instead of the original pid address 
**			containing the returned pid.
**	25-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add include for <clconfig.h>;
**		Changed include <signal.h> to <clsigs.h>;
**		Silence warnings by using TYPESIG;
**		Put the initial check of the pid queue inside the for(;;) loop -
**		if someone has SIGCHLD trapped, the pid may be added to the
**		queue effectively during the wait().
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.
**	21-may-91 (seng)
**		Add int declaration in front of Pidq_init.
**		RS/6000 compiler chokes on statement.
**	26-apr-1993 (fredv)
**	    Moved <clsigs.h> before <ex.h> to avoid redefine of EXCONTINUE 
**		in the system header sys/context.h of AIX.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	05-jun-1995 (canor01)
**	    semaphore protect memory allocation routines
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	06-oct-1998 (somsa01)
**	    Instead of returning PC_WT_BAD, return the error received from
**	    the child.
**	27-oct-1997 (canor01)
**	    If process completed, but returned a bad status, return that
**	    to the caller.
**	02-dec-1997 (Saeid)
**	    Use waitpid, instead of wait. This allow you to wait for specific
**          child, otherwise problem of concurrency with compileProcedure (Jasmine).
**	17-dec-1997 (canor01)
**	    Synchronize access to global Pidq when using OS threads.
**	19-dec-1997 (Saeid)
**	    Condition wait or waitpid with "xCL_WAITPID_EXISTS", for portability.
**          "xCL_WAITPID_EXISTS" is defined in "clsecrt.h", because right
**	    now, the only UNIX plateform for Jasmine is Solaris. But it
**	    should be implemented in "mksecret.sh".
**	09-oct-1998 (somsa01)
**	    In returning the exit status from the child, wait_status should
**	    be shifted by 8, not by 4.
**      07-jun-2000 (stial01)
**          Removed Pidq_mutex. (b101767, Startrak 9027036)
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
*/



STATUS
PCwait(proc)
PID	proc;
{
	i4	wait_status;
    PIDQUE	*qp;		/* q element of the process of interest */
    PIDQUE	*sqp;		/* search queue pointer */
    STATUS	status;
    bool	pid_found = FALSE;
    PID		pid;
    STATUS	ret_val;
    TYPESIG	(*inthandler)();
    TYPESIG	(*quithandler)();

    /* No queue means nothing has been started */
    if(Pidq_init == FALSE)
	return(PC_WT_NONE);

    /*
    ** Wait for a process to die.
    ** If error, map as needed and proceed.
    ** If it's the one we are looking for, remove it from the chain
    **   and exit the loop.
    ** If it's not the one of interest, save it's status in it's
    **   entry on the queue and wait again.
    */
    for( ; ; )
    {
        /* search for the PID in the pid queue */
        for (qp = (PIDQUE *) Pidq.q_next; 
	        qp != (PIDQUE *) &Pidq;
	        qp = (PIDQUE *) qp->pidq.q_next
	     )
	        if(qp->pid == proc)
	        {
		        pid_found = TRUE;
		        status = qp->stat;
		        break;
	        }
 
        /* if we found the PID and the status is not executing, then return
        ** it's status
        */
        if( pid_found )
        {
	    if(status != (STATUS) PC_WT_EXEC)
	    {
	        QUremove( (QUEUE*)qp );
	        MEfree( (char *)qp);
	    }
        }
        else
	    status = PC_WT_NONE;

        if ( pid_found)
	{
            if (status != (STATUS) PC_WT_EXEC)
	    	return (status);
	}
	else    return (status);
 
	/*
	** Disable the SIGINT and SIGQUIT, and save their old actions.
	** EXinterrupt(EX_OFF) now latches incoming keyboard interrupts,
	** rather than discarding them; we need to actually turn the
	** signal handling off.
	*/
	inthandler = signal(SIGINT, SIG_IGN);
	quithandler = signal(SIGQUIT, SIG_IGN);

# if defined(OS_THREADS_USED) && defined(xCL_WAITPID_EXISTS)
	pid  = waitpid(proc, &wait_status, 0);
# else /* OS_THREADS_USED && xCL_WAITPID_EXISTS */
	pid  = wait(&wait_status);
# endif /* OS_THREADS_USED && xCL_WAITPID_EXISTS */

	/* reset interrupt and quit signals */

	(void) signal(SIGINT, inthandler);
	(void) signal(SIGQUIT, quithandler);
 
	/*  
	** Map the status of the dead child to a PC_WT_code.
	**
	**	if no specific child died
	**	 	return PC_WT_NONE
	**	else if wait_status is abnormal
	**		if termination status is normal
	**			got a bad return code from child process
	*/
 
	ret_val = (wait_status == 0) ? OK :
			 ((wait_status & 0377) != 0 ? 
				PC_WT_TERM : wait_status >> 8);
 
	if (pid == SYS_NO_CHILDREN)
	{
		switch (errno)
		{
			case ECHILD:
		    /* no children to wait for */
		    return(PC_WT_NONE);

	    case EFAULT:
		    /* exit to avoid loop */
		    return(PC_WT_BAD);
 
			case EINTR:
		    /* continue looping in the wait */
				break;

			default:
		    /* should never occur */
				ret_val = PC_WT_TERM;
				break;
		}
	}

	/* If this is the process of interest,
	** dequeue it's entry and return it's mapped status.
	*/
	if(pid == proc)
	{
	    (void)QUremove( (QUEUE*)qp);
	    MEfree( (char*)qp );
	    break;
	}
 
	/* Search for the PID in the pid queue 
	** Save the status of the dead process it if is
	** in the queue, otherwise it falls on the floor.
	*/
	for (sqp = (PIDQUE *) Pidq.q_next; 
		sqp != (PIDQUE *) &Pidq;
		sqp = (PIDQUE *) sqp->pidq.q_next
	 )
	    if(sqp->pid == pid) 
	    {
		    sqp->stat = ret_val;
		    break;
	    }
    }
	return(ret_val);
}

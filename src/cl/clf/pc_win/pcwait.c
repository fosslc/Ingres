# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include       <clconfig.h>
# include       <clsigs.h>
# include	<ex.h>
# include	<me.h>
# include	<cs.h>
# include	<pc.h>
# include	<PCerr.h>
# include	"pclocal.h"
# include	<errno.h>
# include	<qu.h>
# include	"pcpidq.h"

/*
** Copyright (c) 1985, 1999 Ingres Corporation
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
**	16-aug-96 (emmag)
**	    Stub required to build NT for now.  Will revisit.
**	09-nov-1999 (somsa01)
**	    Defined PCwait() for NT.
*/



STATUS
PCwait(proc)
PID	proc;
{
    PIDQUE	*qp;		/* q element of the process of interest */
    STATUS	status;
    bool	pid_found = FALSE;
    STATUS	ret_val;
    HANDLE	hPid;

    /* No queue means nothing has been started */
    if (!Pidq_init)
	return(PC_WT_NONE);

    CS_synch_lock(&Pidq_mutex);

    /* Search for the PID in the pid queue. */
    for (qp = (PIDQUE *)Pidq.q_next;
	 qp != (PIDQUE *)&Pidq;
	 qp = (PIDQUE *)qp->pidq.q_next)
    {
	if ((qp->pid == proc) && qp->hPid)
	{
	    pid_found = TRUE;
	    hPid = qp->hPid;
	    status = qp->stat;
	    break;
	}
    }

    /*
    ** If we found the PID and the status is not executing, then
    ** return its status.
    */
    if (pid_found)
    {
	if (status != PC_WT_EXEC)
	{
	    QUremove((QUEUE*)qp);
	    MEfree((PTR)qp);
	}
    }
    else
	status = PC_WT_NONE;

    CS_synch_unlock(&Pidq_mutex);

    if (pid_found)
    {
	if (status != PC_WT_EXEC)
	    return(status);
    }
    else
	return(status);

    
    if (WaitForSingleObject(hPid, INFINITE) == WAIT_FAILED)
	return(PC_WT_BAD);
    GetExitCodeThread(hPid, &ret_val);

    /*
    ** Dequeue the thread's entry and return its mapped status.
    */
    CS_synch_lock(&Pidq_mutex);
    QUremove((QUEUE*)qp);
    CS_synch_unlock(&Pidq_mutex);
    MEfree((PTR)qp);
    CloseHandle(hPid);

    return(ret_val);
}

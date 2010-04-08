/*
**	Copyright (c) 1999 Ingres Corporation
*/

# include	<compat.h>
# include	<cs.h>
# include	<pc.h>
# include	<qu.h>
# include	<clconfig.h>
# include	"pclocal.h"
# include	"pcpidq.h"

/*
**
**  Name: PCisthread_alive - is a thread alive?
**
**  Description:
**	This function grabs a thread id's handle off of the PID queue and
**	tests to see if it is alive. If it cannot be found, then it
**	executes PCis_alive().
**
** History:
**      10-nov-1999 (somsa01)
**	    Created.
*/
bool
PCisthread_alive(PID pid)
{
    PIDQUE	*qp;
    bool	pid_found = FALSE;
    HANDLE	hPid;

    /* No queue means nothing has been started */
    if (!Pidq_init)
	return(PCis_alive(pid));

    /*
    ** Find the thread id in the PID queue.
    */
    CS_synch_lock(&Pidq_mutex);
    for (qp = (PIDQUE *)Pidq.q_next;
	 qp != (PIDQUE *)&Pidq;
	 qp = (PIDQUE *)qp->pidq.q_next)
    {
	if ((qp->pid == pid) && qp->hPid)
	{
	    pid_found = TRUE;
	    hPid = qp->hPid;
	    break;
	}
    }
    CS_synch_unlock(&Pidq_mutex);

    if (!pid_found)
	return(PCis_alive(pid));
    else
    {
	DWORD	status;

	/*
	** Let's see if this thread is still alive.
	*/
	GetExitCodeThread(hPid, &status);
	if (status == STILL_ACTIVE)
	    return(TRUE);
	else
	{
	    /*
	    ** Set the thread's exit status in the queue.
	    */
	    CS_synch_lock(&Pidq_mutex);
	    qp->stat = status;
	    qp->hPid = NULL;
	    CS_synch_unlock(&Pidq_mutex);
	    CloseHandle(hPid);

	    return(FALSE);
	}
    }
}

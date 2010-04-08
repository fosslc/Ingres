/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cs.h>
# include	<er.h>
# include	<pc.h>
# include	<qu.h>
# include	<st.h>

# include	<clconfig.h>

# include	"pclocal.h"
# include	"pcpidq.h"
# include	<PCerr.h>

static i4	PCfork_thread(LPVOID thread_arg);

/*
**
**  Name: PCfork - "fork" a process/thread on NT.
**
**  Description:
**	This function spawns a process with no waiting via threads. The
**	thread id is then stored on the PID queue.
**
**  Inputs:
**	
**
**  History:
**	16-Aug-1996 (mcgem01)
**		The fork() functionality is not available on desktop 
**		platforms, so return fail for now.  This issue will be 
**		addressed for the parallel checkpoint functionality.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	    The fork() functionality is not available on desktop 
**	    platforms, so return fail for now.  This issue will be 
**	    addressed for the parallel checkpoint functionality.
**	08-nov-1999 (somsa01)
**	    Defined the meaning of PCfork() on NT.
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
*/

PID
PCfork(
LPTHREAD_START_ROUTINE	thread_func,
LPVOID			func_arg,
STATUS			*status)
{
    register PIDQUE		*pp;
    LPTHREAD_START_ROUTINE	thrd_func;
    SECURITY_ATTRIBUTES		sa;
    HANDLE			hThread;
    PID				pid = -1;

    if (!func_arg)
	return(pid);

    /* Allocate a PID queue entry for this possible thread. */
    pp = (PIDQUE *)MEreqmem(0, sizeof(*pp), TRUE, status);
    if (pp != NULL)
    {
	/* If the PID queue doesn't exist yet, make it now. */
	if (!Pidq_init)
	{
	    QUinit(&Pidq);
	    Pidq_init = TRUE;
	    CS_synch_init(&Pidq_mutex);
	}

	iimksec(&sa);

	if (thread_func)
	    thrd_func = thread_func;
	else
	    thrd_func = (LPTHREAD_START_ROUTINE)PCfork_thread;

	if (!(hThread = CreateThread(&sa, 0, thrd_func, func_arg, 0, &pid)))
	{
	    *status = GetLastError();
	    MEfree((PTR)pp);
	    return(-1);
	}

	/*
	** Add the entry into the PID queue.
	*/
	pp->pid = pid;
	pp->stat = PC_WT_EXEC;
	pp->hPid = hThread;

	CS_synch_lock(&Pidq_mutex);
	QUinsert((QUEUE *)pp, &Pidq);
	CS_synch_unlock(&Pidq_mutex);
    }

    return(pid);
}

static i4
PCfork_thread(
LPVOID	thread_arg)
{
    char	*command = (char *)thread_arg;
    i4		os_ret;
    CL_ERR_DESC	err_code;

    if ((os_ret=PCcmdline((LOCATION *)NULL, command, PC_WAIT, NULL,
			  &err_code)))
    {
	SETCLERR(&err_code, 0, ER_system);
	if (!err_code.errnum)
	{
	    err_code.moreinfo[0].size = sizeof(os_ret);
	    err_code.moreinfo[0].data._longnat = os_ret;

	    /*
	    ** Let's make sure that we do not overflow what we have left
	    ** to work with in the moreinfo structure. Thus, since we have
	    ** (CLE_INFO_ITEMS-1) unions left to use, the max would be
	    ** CLE_INFO_MAX*(CLE_INFO_ITEMS-1).
	    */
	    err_code.moreinfo[1].size = min((u_i2)STlength(command),
		(u_i2)(CLE_INFO_MAX*(CLE_INFO_ITEMS-1)));
	    STlcopy(command, err_code.moreinfo[1].data.string,
		err_code.moreinfo[1].size);
	    /* ule_format */
	}
    }

    return(os_ret);
}


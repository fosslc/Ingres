/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <clconfig.h>
#include    <cs.h>
#include    <csev.h>

#include    <csinternal.h>
#include    <cssminfo.h>

/*
** Name: CSCPRES.C	- Cross-process resume support
**
** Description:
**	This file contains the routines which implement cross-process resume.
**
**	Cross-process resume is used by the Logging and Locking components of
**	DMF.
**
**	    CScp_resume	    - Resume (a thread in) a process
**
**	Implementation notes for Unix:
**
**	    Cross-process resume support is built upon the CS event system's
**	    "null server-server event" support.
**
**	    The installation system segment now contains an array of "wakeup
**	    blocks"; the size of this array can be set through CSinstall, and
**	    defaults to 2000 elements (32,000 bytes of system shared memory).
**
**	    A wakeup block is used to record the PID and SID of a thread which
**	    has been CScp_resume'd.
**
**	    Each process in the CS system
**	    defines a server event handler. For pseudo-servers, since there is
**	    only one thread in the process, the server event handler is simply
**	    CSresume. For multi-threaded servers, the server event handler
**	    looks through the wakeup blocks and calls CSresume for each block
**	    which applies to this server.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-march-1993 (jnash)
**	    In CScp_resume, silently return if CS_find_servernum finds that 
**	    the server is not alive.  This can occur when the RCP performs
**	    server failure rundown processing, and perhaps at other times.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-Nov-1998 (jenjo02)
**	    Changed CScp_resume argument to (new) CS_CPID* instead of
**	    pid, sid.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need cssminfo.h to satisfy gcc 4.3.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, get from csinternal.
*/

/*
** Name: CScp_resume	- Resume (a thread in) a process
**
** Description:
**	This routine resumes the indicated thread in the indicated process.
**
**	If the indicated process is this process, then this is a simple
**	CSresume operation. If the indicated process is another process, then
**	that process must be notified that it should CSresume the indicated
**	thread.
**
** Inputs:
**	cpid		- pointer to thread identity.
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	15-march-1993 (jnash)
**	    Silently return if CS_find_servernum finds that the server
**	    is not alive.  This can occur in RCP rundown code and perhaps 
**	    at other times.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	16-Nov-1998 (jenjo02)
**	    Changed CScp_resume argument to (new) CS_CPID* instead of
**	    pid, sid.
**	    When OS threads are in use, call CSMT_resume_cp_thread() to
**	    directly signal the thread instead of using the IdleThread
**	    event mechanism.
*/
VOID
CScp_resume(
CS_CPID	*cpid)
{

    if (cpid->pid == Cs_srv_block.cs_pid)
    {
#ifdef OS_THREADS_USED
	if (Cs_srv_block.cs_mt)
	    CSMTresume(cpid->sid);
	else
#endif /* OS_THREADS_USED */
	    CSresume(cpid->sid);
    }
    else
    {
#ifdef OS_THREADS_USED
#ifndef USE_IDLE_THREAD
	if (Cs_srv_block.cs_mt)
	    /* Direct wakeup by signalling cross-process condition */
	    CSMT_resume_cp_thread(cpid);
	else
#endif /* USE_IDLE_THREAD */
#endif /* OS_THREADS_USED */
	{
	    i4		server_num;

	    if (CS_find_servernum(cpid->pid, &server_num))
		return;

	    if (CS_is_server(server_num))
	    {
		if (CS_set_wakeup_block(cpid->pid, cpid->sid))
		    PCexit(FAIL);
	    }
	    CScause_event(0, server_num);
	}
    }

    return;
}

/*
** Name: CS_cpres_event_handler		- handle any cross process resumes
**
** Description:
**	This routine is called by the CS event system to handle any
**	cross-process resumes which are sent to this process from other
**	processes. It in turn calls back to the event system to do most of
**	the work.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK
**
** History:
**	3-jul-1992 (bryanp)
**	    created.
*/
STATUS
CS_cpres_event_handler(CSEV_CB *notused)
{
    CS_handle_wakeup_events(Cs_srv_block.cs_pid);

    return (OK);
}

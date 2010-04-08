/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = dr6_us5 i64_aix
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <errno.h>
# include   <clipc.h>
# include   <fdset.h>
# include   <clsigs.h>
# include   <cs.h>
# include   <pc.h>
# include   <csev.h>
# include   <csinternal.h>
# include   <cssminfo.h>
# include   <lo.h>
# include   <nm.h>
# include   <me.h>
# include   <st.h>
# include   <tr.h>
# include   <clpoll.h>

/**
** Name: CSEV.C - Functions used to implement CS events in the server
**
** Description:
**      This file contains the following external routines:
**
**	CS_event_init()    -  Initialize the event system.
**      CSslave_init()     -  Slave initialization routine.
**      CSreserve_event()  -  Reserve an event control block.
**      CScause_event()    -  Send a request to a slave.
**      CSfree_event()     -  Free a reserved event control block.
**	CS_find_events()   -  Find and handle return events.
**	CSdef_resume()	   -  Resume an evcb during completion code
**	CSawait_event()    -  Wait for a specific event to complete.
**
 * Revision 1.12  89/03/07  15:19:08  roger
 * CL_SYS_ERR -> CL_ERR_DESC.
 * 
 * Revision 1.11  89/02/16  14:42:18  jeff
 * Timeing related bug fix - wrong slave could have serviced an evcb
 * 
 * Revision 1.10  89/01/29  18:19:58  jeff
 * New select driven event system
 * 
 * Revision 1.9  89/01/18  17:26:44  jeff
 * pipe & signal wakeups & CS_find_events will never block
 * 
 * Revision 1.8  89/01/05  15:15:01  jeff
 * moved header file
 * 
 * Revision 1.7  88/12/27  10:45:33  jeff
 * inititalize event system on first request (slave_init)
 * 
 * Revision 1.6  88/12/14  13:55:30  jeff
 * event system optimizations
 * 
 * Revision 1.5  88/10/17  15:17:56  jeff
 * fixed union argument passing
 * 
 * Revision 1.4  88/08/31  17:17:15  roger
 * Changes needed to build r
 * 
 * Revision 1.3  88/08/30  11:47:01  jeff
 * fix for rare server hang problem
 * 
 * Revision 1.2  88/08/24  11:29:26  jeff
 * cs up to 61 needs
 * 
 * Revision 1.11  88/05/31  11:24:54  anton
 * server-server event fix and added spin locks
 * 
 * Revision 1.10  88/04/22  15:19:33  anton
 * made event system startup more flexible
 * 
 * Revision 1.9  88/04/21  12:07:02  anton
 * added some spin locks and a cheep event wakeup for LG LK issues
 * 
 * Revision 1.8  88/04/18  13:00:43  anton
 * code cleanup & keep track of slave process ids
 * 
 * Revision 1.7  88/03/29  15:35:59  anton
 * server-server events + misc fixes
 * 
 * Revision 1.6  88/03/21  12:22:49  anton
 * Event system appears solid
 * 
 * Revision 1.5  88/03/14  14:42:59  anton
 * added II_SLAVE symbol lookup
 * 
 * Revision 1.4  88/03/14  13:44:28  anton
 * added more failure exits
 * 
 * Revision 1.3  88/03/07  16:31:35  anton
 * added exception declaration and argument list change
 * 
 * Revision 1.2  88/03/03  11:56:03  anton
 * first cut at CS slave driven events
 * 
**	19-jan-88 (jaa)
**	    revised and renamed for CS events
**
**      Revision 1.2  87/09/29  17:40:07  mikem
**      greg's changes.
**      
**      Revision 1.1  87/09/29  09:51:30  mikem
**      Initial revision
**      
**      28-sep-87 (mmm)
**	    written
**	Integrated changes daveb and GordonWright made to beta1 code into
**	beta3 code (fredv)
**	o   Use new ifdefs, reorder includes, define SIGCLD where needed,
**	    delete unused variable.  
**	o   Replace vfork with PCfork, more header cleanup.
**	o   PCfork requires a *status as argv.
**	20-jul-89 (rogerk)
**	    When run out of event control blocks, call cs_ef_wait to wait for
**	    one to be freed.  When freeing control blocks, check for any
**	    sessions waiting for them and call cs_ef_wake.
**      01-nov-1989 (fls-sequent)
**          Modified for MCT.  Get Cs_srv_block spin lock before accessing
**	    cs_event_mask for CS_EF_CSEVENT.
**	6-Dec-89 (anton)
**	    Added code to retry forking slaves
**	    and fixed problem where the maximum number of slaves
**	    would not work. (CSslave_init)
**	16-jan-90 (fls-sequent)
**	    For MCT, added code to signal the process that is sleeping
**	    not the "main" process during wakeup in CScause_event.
**	18-Jan-90 (anton)
**	    Added timeout parameter to iiCLfdreg.
**	22-Jan-90 (fredp)
**	   Integrated the following UNIX porting group changes:
**         25-may89 (russ) 
**	          Added TRdisplay error message on failure of shmget.
**         31-jul89 (russ) 
**	          Moved creation of CSpipe to CSslave_init.
**	          In CSslave_init, return after CS_event_init if no slaves.
**	    Integrated anton's MCT changes: 
**		Removed CSef code, it is no longer used.
**		Add support for defered resumes of events.
**      5-june-90 (blaise)
**          Integrated changes from 61 and ingresug:
**    7-june-90 (blaise)
**        Added missing declaration for CSsigchld().
**    24-Aug-90 (jkb)
**        Remove code to create pipes between server and slaves use
**        signals rather than pipes to alert server
**    30-oct-1991 (bryanp)
**        Changes to CSslave_init to allow the RCP to have slaves, which it
**        uses for the log file.
**	26-jun-90 (fls-sequent)
**	    Changed CS atomic locks to SHM atomic locks.
**	24-Aug-90 (jkb)
**	    Remove code to create pipes between server and slaves use
**	    signals rather than pipes to alert server
**	18-oct-90 (fls-sequent)
**	    Integrated changes from 63p CL:
**              In CS_event_init, rather than just i_EXsetsig for SIGCHLD,
**		set a signal trap for CSsigchld, which will deal with the
**		SIGCHLD correctly;
**		Add call to CS_ACLR when initialising the events for a new
**		slave in CSslave_init;
**		Add xCL_075 around SYSV shared memory calls;
**		Add trace statement on failed iiCLpoll call in CSawait_event();
**		Add xCL_078_CVX_SEM to include Convex semaphores.
**	7-june-90 (blaise)
**	    Added missing declaration for CSsigchld().
**	24-Aug-90 (jkb)
**	    Remove code to create pipes between server and slaves use
**	    signals rather than pipes to alert server
**	30-oct-1991 (bryanp)
**	    Changes to CSslave_init to allow the RCP to have slaves, which it
**	    uses for the log file.
**	02-jul-92 (swm)
**	    Merge TRdisplays so correct errno is displayed; otherwise errno
**	    is trampled on by the first of our (two) TRdisplays.
**	    Added dr6_uv1 to NO_OPTIM because dr6_us5 has optimisation
**	    switched off; both dr6_us5 and dr6_uv1 are DRS6000s running SVR4
**	    but dr6_uv1 has secure Unix extensions.
**	6-aug-1992 (bryanp)
**	    Added CS_set_wakeup_block for CScp_resume support.
**	05-nov-1992 (mikem)
**	    su4_us5 port.  Added NO_OPTIM.  Using the same compiler as su4_u42.
**	26-jul-1993 (bryanp)
**	    If iislave is entirely missing, then the child may die before the
**	    parent even returns from PCfork(). If this occurs, CSsigchld doesn't
**	    know that this is a slave PID that has just dies, so it ignores the
**	    child death and eventually we hang in CSsuspend. To avoid this,
**	    re-check each of the slave PIDs at the end of CSslave_init() before
**	    returning; if any are no longer alive, complain bitterly and force
**	    the server down.
**	    We don't really need NSVSEMS per server, we only need as many as
**	    	we have slaves.
**	    css_semid is no longer used, since we no longer have any need for
**		any system-wide semaphores.
**	 9-aug-93 (robf)
**	    Add su4_cmw to NO_OPTIM
**	23-aug-1993 (mikem)
**	    If server starts with no slaves then do not attempt to get any
**	    semaphores.  semget() will fail if you ask for 0 semaphores, so
**	    you must special case the 0 semaphore case.
**	23-aug-1993 (andys)
**	    Created CS_cleanup_wakeup_events to clean up old wakeup events.
**	    This is analogous to the fix in ingres63p for bug # 52761.
**	18-oct-1993 (bryanp)
**	    Added argument to CS_cleanup_wakeup_events to indicate whether
**		it needs to take the system segment spinlock or not.
**      31-jan-94 (mikem)
**          sir #57671
**          As part of making the size of server segment configurable by PM
**          variables changed the CSEV_MSIZE macro to accept 2 arguments:
**          the number of slave control blocks and the size of the data portion
**          of the slave control blocks.
**	31-jan-94 (mikem)
**	    bug #58298
**	    Changed CS_handle_wakeup_events() to maintain a high-water mark
**	    for events in use to limit the scanning necessary to find 
**	    cross process events outstanding.  Previous to this each call to 
**	    the routine would scan the entire array, which in the default 
**	    configuration was 4k elements long.
**	02-nov-93 (swm)
**	    Bug #56447
**	    Changed type of automatic variable, resid, from i4 to CS_SID
**	    in CS_handle_group() to eliminate possible pointer truncation.
**	21-feb-1994 (bryanp)
**	    Ignore SIGCHLD once CS_event_shutdown begins shutting slaves down.
**	13-mar-1995 (smiba01)
**	    Added dr6_ues to NO_OPTIM list (see 02-jul-92)
**	25-oct-1995 (stoli02/thaju02)
**	    Added event waits and count counters.
**    01-may-1995 (amo ICL)
**        Added anb's async io amendments
**    03-jun-1996 (canor01)
**        Remove unneeded MCT semaphores.
**      24-jul-1996 (canor01)
**          integrate changes from cs.
**    22-jan-1997 (canor01)
**        Protect change in server control block's csi_events_outstanding
**        flag so changes do not get lost in concurrent access.
**    18-feb-1997 (hanch04)
**        As part of merging of Ingres-threaded and OS-threaded servers,
**        use csmt version of this file.
**	24-mar-1997 (canor01)
**	    Allow for different default slave name based on SystemExecName
**	    (especially for Jasmine).
**  15-oct-97 (devjo01)
**      Moved set of 'forslave' to before CSget_sid() function call
**      to prevent AXP optimizing compiler from switching order of
**      assignment with 'flags' (*5679933.1).
**	04-feb-1998 (somsa01) X-integration of 432936 from oping12 (somsa01)
**	    On some machines, the slaves may not die when the semaphores are
**	    removed. Therefore, set up a new signal to alert the slaves to
**	    exit themselves before the semaphores are removed.
**	20-may-1997 (canor01)
**	    If II_PCCMDPROC is set to a process name, use it as a command
**	    process slave.  PCcmdline() will pass all commands to it.
**	15-oct-97 (devjo01)
**	    Moved set of 'forslave' to before CSget_sid() function call
**	    to prevent AXP optimizing compiler from switching order of
**	    assignment with 'flags' (*5679933.1).
**	16-Nov-1998 (jenjo02)
**	    Added support for direct signalling of cross-process conditions
**	    instead of reliance on IdleThread/SIGUSR2 when OS threads are
**	    in use.
**	    Added new external functions CSMT_resume_cp_thread(),
**	    CSMT_get_wakeup_block(), CSMT_free_wakeup_block() for OS threads.
**	27-jan-98 (schte01)
**	    Corrected OS_THREADS_IN_USE to OS_THREADS_USED. Added ifndef/endif
**       in order to prevent inclusion of CSMT_resume_cp_thread if using
**       idle threads. Corrected cs_event_cond, cs_event_sem to event_cond, 
**       event_sem.
**	03-Mar-1999 (jenjo02)
**	    Fixed typos, wakeup_block[i] to &wakeup_blocks[i]
**	    in CS_cleanup_wakeup_events().
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dr6_uv1.
**	30-may-2000 (toumi01)
**	    Send signals to new csi_signal_pid (vs. csi_pid).
**	15-Aug-2001(horda03) Bug 10531 INGSRV 1502
**	    Add new function CS_clear_wakeup_block() to clear a Cross Process
**	    resume for a session.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	23-sep-2002 (devjo01)
**	    Pass NUMA context to slave if running NUMA clusters.
**	10-oct-2002 (devjo01)
**	    Added logic to conditionally check for a missed event.
**	14-Nov-2002 (hanch04)
**	    Removed NO_OPTIM for Solaris
**	07-Sep-2004 (mutma03)
**         If xCL_SUSPEND_USING_SEM_OPS defined, initialize 
**         event_wait_sem in CSMT_get_wakeup_block.
**/

/* externs */
GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF	CS_SERV_INFO	*Cs_svinfo;
GLOBALREF	CS_SYSTEM	Cs_srv_block;
GLOBALREF	CSSL_CB		*Di_slave;
# ifdef OS_THREADS_USED
GLOBALREF     bool            Di_async_io;
# endif /* OS_THREADS_USED */

/* statics */
PTR		CSev_base;
u_i4		CSservernum;
i4		CSslavepids[NSVSEMS] ZERO_FILL;
# ifdef JASMINE
i4		CScmdprocpid ZERO_FILL;
# endif /* JASMINE */
CSEV_CB		*CSwaiting ZERO_FILL;

FUNC_EXTERN	VOID	CS_event_shutdown();
FUNC_EXTERN	VOID	iiCLintrp();
FUNC_EXTERN     VOID    CSsigchld();
FUNC_EXTERN	i4	CXnuma_cluster_rad(void);

static		i4	minfree = 0;
static		i4 maxused = 0;



/*{
1** Name: CS_event_init - Initialize the event system.
**
** Description:
**    Perform all necessary initialization. These 
**    actions may include mapping shared memory into the current address 
**    space and initializing tables local to CS.
**
** Inputs:
**	none.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          non-OK status                   Function completed abnormally.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      20-sep-87 (mmm)
**          Created.
**	03-mar-88 (anton)
**	    Functional.
**	18-Jan-90 (anton)
**	    Added timeout parameter to iiCLfdreg.
**	22-Jan-90 (fredp)
**	    Moved creation of CSpipe to CSslave_init().
**	26-jun-90 (fls-sequent)
**	    Changed CS_ACLR to CS_ACLR.
**	5-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Rather than just i_EXsetsig for SIGCHLD, set a signal trap
**		for CSsigchld, which will deal with the SIGCHLD correctly.
**	30-oct-1991 (bryanp)
**	    Allow CSevent_init to be called twice. In particular, only
**	    install the signal and exit handlers once.
**	02-jul-92 (swm)
**	    Merge TRdisplays so correct errno is displayed; otherwise errno
**	    is trampled on by the first of our (two) TRdisplays.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (bryanp)
**	    We don't really need NSVSEMS per server, we only need as many as
**	    	we have slaves. This means our caller must tell us how many
**		slaves we'll have.
**	23-aug-1993 (mikem)
**	    If server starts with no slaves then do not attempt to get any
**	    semaphores.  semget() will fail if you ask for 0 semaphores, so
**	    you must special case the 0 semaphore case.
**	18-oct-1993 (bryanp)
**	    Defer the registration of CS_event_shutdown as an atexit handler
**	    until we have actually connected to the system segment. Otherwise,
**	    if we fail to connect to the system segment, but then call
**	    CSevent_shutdown, it will erroneously destroy server segment 0.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
*/
STATUS
CS_event_init(nusers, nslaves)
i4	nusers;
i4	nslaves;
{
	int		pid;
	i4		status;
	i4		i;
	i4		msize;
	PTR		addr;
	register CSEV_SVCB	*svcb;
	CL_ERR_DESC	err_code;
	static          bool    first_time = TRUE;
	i4		size_of_serv_segment;

	if (first_time)
	{
	    i_EXsetothersig(SIGCHLD, CSsigchld);
	    i_EXsetothersig(SIGUSR2, iiCLintrp);
	    i_EXsetothersig(SIGPIPE, SIG_IGN);
	}	
	if (nusers == 0)
	{
	    /* this is a pseudo-server */
	    status = CS_init_pseudo_server(&CSservernum);
	}
	else
	{
	    if (Cs_srv_block.cs_size_io_buf == 0)
		Cs_srv_block.cs_size_io_buf = 8192;
	    /* Override user if we're not going to use slaves */
	    if (nslaves == 0)
		Cs_srv_block.cs_size_io_buf = 4096;

	    /* Forget about messing with num I/O bufs, needs more
	    ** fixes in slave-init, maybe elsewhere.
	    */
	    /* ** if (Cs_srv_block.cs_num_io_buf == 0) ** */
		Cs_srv_block.cs_num_io_buf = nusers;

	    size_of_serv_segment = 
	    CSEV_MSIZE(Cs_srv_block.cs_num_io_buf, Cs_srv_block.cs_size_io_buf);

	    status = CS_alloc_serv_segment(size_of_serv_segment, &CSservernum,
				   &CSev_base, &err_code);
	}

	if (status)
	    return (status);

	/*
	** We have deferred the registration of CS_event_shutdown as an
	** atexit handler until we had successfully connected to the
	** shared memory system segment. Now that that has worked, install
	** the exit handler. Then, if we're a pseudo server, we're all done.
	*/

	if (first_time)
	{
	    first_time = FALSE;
	    PCatexit(CS_event_shutdown);
	}

	if (nusers == 0)
	    return (OK);

	svcb = (CSEV_SVCB *)CSev_base;

	/* allocate shared memory for nuser size system */
	svcb->events_off = sizeof(*svcb);
	svcb->msize = size_of_serv_segment;

	/* get bunch of sys V semaphores */
# ifdef xCL_075_SYS_V_IPC_EXISTS
	if (nslaves > 0)
	{
	    Cs_svinfo->csi_semid = 
	      semget(Cs_svinfo->csi_serv_desc.cssm_id, nslaves, 0600|IPC_CREAT);
	}
# endif
	if (Cs_svinfo->csi_semid == -1)
	{
            TRdisplay("semget of %d semaphores failed\nerrno = %d\n",
		nslaves, errno);
	    return(FAIL);
	}
	Cs_svinfo->csi_nsems = nslaves;
	Cs_svinfo->csi_usems = 0;
	for (i = 0; i < NEVUSERS; i++)
	{
	    CS_ACLR(&svcb->event_done[i]);
	}

	return(OK);
}

/*{
** Name: CS_event_shutdown()	- deallocate system resources for CSevents
**
** Description:
**	Terminate child processes and deallocate system shared memory and
**	semaphores.  Should only be called before the server exists and
**	must be called.
**
**	Once the event system begins to shut down the slave processes, the
**	SIGCHLD signal is no longer interesting and we ignore it.
**
** Inputs:
**	none
**
** Outputs:
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-mar-88 (anton)
**	    Created.
**	21-feb-1994 (bryanp)
**	    Ignore SIGCHLD once CS_event_shutdown begins shutting slaves down.
**	    This change fixes some server shutdown behaviors in which the server
**		might begin to shut down, then call CS_event_shutdown, which
**		would shut down the slaves, which would cause SIGCHLD signals
**		to be generated, which would cause CSsigchld() to call
**		CSterminate, and CSterminate would encounter problems because
**		the server was already far advanced in its shutdown processing.
**		To avoid having shutdown code interrupted by signal-generated
**		shutdown code, we ignore the SIGCHLD signal once we begin
**		intentionally shutting down the slaves.
**	29-may-1997 (canor01)
**	    Changed return type to void, since no status can safely be
**	    returned.
**	04-feb-1998 (somsa01) X-integration of 432936 from oping12 (somsa01)
**	    On some machines, the slaves may not die when the semaphores are
**	    removed. Therefore, set up a new signal to alert the slaves to
**	    exit themselves before the semaphores are removed.
**	16-Nov-1998 (jenjo02)
**	    Added call to CS_cleanup_wakeup_event() to release wakeup blocks
**	    left around by this process.
*/
VOID
CS_event_shutdown()
{
    CL_ERR_DESC	errcode;
    CSEV_SVCB	*svcb;
	CSEV_CB		*evcb;
	i4			j;

    /* kill child processes - they are designed to die when semaphores
     * are removed from the system */
    if (svcb = (CSEV_SVCB *)CSev_base)
    {
	/* We no longer care to know when the slaves die: */
	i_EXsetothersig(SIGCHLD, SIG_IGN);

	/*
	** On some machines, child processes may not die when the
	** semaphores are removed. Therefore, signal the slaves to exit,
	** then remove the semaphores.
	*/
	if (Di_slave)
	{
		CSreserve_event(Di_slave, &evcb);
		for (j=0; j < NSVSEMS; j++)
		if (CSslavepids [j])
		{
			/*
			** Since this is the last event the slaves will ever get,
			** take off all possible flags and just leave on the exit
			** flag (so as to not clutter this event).
			*/
			evcb->flags &= ~(EV_INPG|EV_WAIT|EV_SLCP|EV_BGCM|EV_PNDF|
							 EV_REST| EV_SERV|EV_DRES);
			evcb->flags |= EV_EXIT;

			CScause_event(evcb, j);
	    }
		/*
		** Since we are handling the cleanup, take off the possible
		** remaining flags to allow us to properly free the evcb.
		*/
		evcb->flags &= ~(EV_INPG|EV_BGCM);
		CSfree_event(evcb);
	}

# ifdef xCL_075_SYS_V_IPC_EXISTS
	semctl(Cs_svinfo->csi_semid, 0, IPC_RMID, 0);
# endif
# ifdef xCL_078_CVX_SEM
        int i;

        for (i=0; i < NSVSEMS; i++)
            if (CSslavepids [i])
                kill (CSslavepids[i],SIGKILL);
# endif
    }
    /* Cleanup wakeup event blocks left over from this process */
    CS_cleanup_wakeup_events(Cs_srv_block.cs_pid, (i4)0);
    CS_destroy_serv_segment(CSservernum, &errcode);
    CSev_base = NULL;
    return;
}

/*{
** Name: CSslave_init()	- Fork a slave process group
**
** Description:
**	This function is called by a CL module to begin a
**	collection of slave processes accessed by the CS event system.
**	Event control blocks are allocated and assigned here.
**    If the event system has not yet been initialized, then determine if
**    we are a multi-threaded server or not. If we are, then initialize a
**    server segment of "appropriate" size. If we are NOT a multi-threaded
**    server, then we must be a 'pseudo server', such as createdb or the RCP.
**    In the pseudo-server case, then we need a server segment ONLY IF we are
**    initializing for LG slaves. (pseudo servers do not use DI slaves).
**
**
** Inputs:
**	usercode	Code identifier for user.
**	nexec		number of slave processes
**	wrksize		size of data area in event control blocks
**			For slaves this does not include the I/O buffer.
**	nwrk		number of event control blocks (>= nexec)
**	evcomp		a event completion routine
**	fdmask		a bitmap of file descriptors to preserve in slave
**			process
**
** Outputs:
**	slave_cb	pointer set to the slave control block
**
**	Returns:
**	    E_DB_OK	everything worked
**	    !OK		something failed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Child UNIX processes are created.  Procedure may take a long
**	    time and should only be called during initialization.
**
** History:
**	10-feb-89 (arana)
**	    On Pyramid use fork instead of vfork from thread
**	    initiated with chgstack system call
**	 1-dec-88 (anton)
**	    fix for use of vfork
**      19-jan-88 (anton)
**          Created
**	20-mar-89 (russ)
**	    Changed an invokation of rindex to STrindex().
**	6-Dec-89 (anton)
**	    Added code to retry forking slaves
**	    and fixed problem where the maximum number of slaves
**	    would not work.
**	22-Jan-90 (fredp)
**	    Return after CS_event_init if no slaves.  Create CSpipe
**	    here instead of in CS_event_init.
**      5-june-90 (blaise)
**          Integrated changes from 61 and ingresug:
**              Add call to CS_ACLR when initialising the events for a new
**              slave.
**	13-nov-1991 (bryanp)
**	    The RCP now has slaves, so this routine must allow for event system
**	    initialization even if Cs_srv_block.cs_max_active is 0.
**	7-Jul-1993 (daveb)
**	    Remove extern of rindex() function.  We don't use it, and our
**	    delcaration conflicited with a prototyped one in system headers.
**	26-jul-1993 (bryanp)
**	    If iislave is entirely missing, then the child may die before the
**	    parent even returns from PCfork(). If this occurs, CSsigchld doesn't
**	    know that this is a slave PID that has just dies, so it ignores the
**	    child death and eventually we hang in CSsuspend. To avoid this,
**	    re-check each of the slave PIDs at the end of CSslave_init() before
**	    returning; if any are no longer alive, complain bitterly and force
**	    the server down.
**	24-mar-1997 (canor01)
**	    Allow for different default slave name based on SystemExecName
**	    (especially for Jasmine).
**	20-may-1997 (canor01)
**	    If II_PCCMDPROC is set to a process name, use it as a command
**	    process slave.  PCcmdline() will pass all commands to it.
**	15-Jun-2004 (schka24)
**	    Eliminate II_SLAVE env variable, security hole and there's
**	    no need for anyone but developers to be messing with slaves.
**	    One can always rename files, use wrappers, etc.
**	19-Jul-2004 (schka24)
**	    The size_io_buf parameter didn't do anything except crash the
**	    server, fix it.
*/
STATUS
CSslave_init(slave_cb, usercode, nexec, wrksize, nwrk, evcomp, fdmask)
CSSL_CB	**slave_cb;
i4	usercode;
i4	nexec;
i4	wrksize;
i4	nwrk;
STATUS	(*evcomp)();
fd_set	fdmask;
{
    register CSEV_SVCB	*svcb;
    register CSSL_CB	*slcb;
    register CSEV_CB	*evcb;
    register i4	i;
    char		usservno[16];
    char		usstr[16];
    char		usslno[16];
    char		ussubslv[16];
    char		radbuf[16];
    char		fdstr[FD_SETSIZE/4+1];
    char		path[200], *ppos;
    char		slavename[200];
    LOCATION		executable;
    char		*str, *name;
    i4			j;
    i4			k;	/* vfork critical variable */
    i4			nretrys;
    i4			rad;
    STATUS			status = OK;
    
    if (!(svcb = (CSEV_SVCB *)CSev_base))
    {
	if (Cs_srv_block.cs_max_active > 0)
	{
	    status = CS_event_init(Cs_srv_block.cs_max_active, nexec);
	}
	else
	{
	    status = CS_event_init(nexec, nexec);
	}

	if (!status && !(svcb = (CSEV_SVCB *)CSev_base))
	{
	    status = FAIL;
	}
	if (status)
	{
	    return(status);
	}
    }
    
    /* return now if the number of slaves is zero */
    if (nexec > 0)
    {

	/* Since we don't know cs_io_buf_size until we init, the caller
	** passes the bare DI_SLAVE_CB size and we adjust for the I/O buffer.
	*/
	if (usercode == CS_DI_SLAVE)
	    wrksize = wrksize - sizeof(ALIGN_RESTRICT) + Cs_srv_block.cs_size_io_buf;

        /* allocate slave cb */
        for (i = 0; i < NEVUSERS; i++)
        {
	    if (!svcb->slave_cb[i].nslave)
	        break;
        }
        if (i == NEVUSERS ||
	    (Cs_svinfo->csi_usems + nexec) > Cs_svinfo->csi_nsems ||
	    nwrk*(wrksize+sizeof(CSEV_CB)) > (svcb->msize - svcb->events_off))
        {
	    return(FAIL);
        }
        
        slcb = &svcb->slave_cb[i];
        slcb->nslave = nexec;
        slcb->slave_type = usercode;
        slcb->flags = CSSL_MANY;
# ifdef xCL_075_SYS_V_IPC_EXISTS
        slcb->slsemid = Cs_svinfo->csi_semid;
# endif
        slcb->slsembase = Cs_svinfo->csi_usems;
        Cs_svinfo->csi_usems += nexec;
        slcb->evcomp = evcomp;
        slcb->evsize = wrksize + sizeof(CSEV_CB);
        slcb->nevents = nwrk;
        slcb->events = svcb->events_off;
        slcb->evend = svcb->events_off += nwrk * slcb->evsize;
        
        /* allocate event cbs */
        for (evcb = (CSEV_CB *)(CSev_base + slcb->events);
	     evcb < (CSEV_CB *)(CSev_base + slcb->evend);
	     evcb = (CSEV_CB *)((char *)evcb + slcb->evsize))
        {
	    evcb->retsemnum = CSservernum;
	    evcb->slave_no = i;
	    evcb->slave_type = usercode;
	    evcb->datalen = wrksize + sizeof(i4);
	    CS_ACLR(&evcb->busy);
        }
        /* fork slaves */
        CVna(i, usslno);
        CVna(CSservernum, usservno);
        CVna(usercode, usstr);
        MEfill(FD_SETSIZE/4, '@', (PTR)fdstr);
        str = fdstr;
        for (i = 0; i < FD_SETSIZE; i += 4)
        {
	    if (FD_ISSET(i, &fdmask))
	    {
	        *str |= 1;
	    }
	    if (FD_ISSET(i+1, &fdmask))
	    {
	        *str |= 2;
	    }
	    if (FD_ISSET(i+2, &fdmask))
	    {
	        *str |= 4;
	    }
	    if (FD_ISSET(i+3, &fdmask))
	    {
	        *str |= 8;
	    }
	    str++;
        }
        *str = EOS;

	if (STcompare(SystemExecName, "ing") == 0)
	    STcopy(SLAVE, slavename);
	else
	    STpolycat(2, SystemExecName, "slave", slavename);

        if (slavename[0] != '/')
        {
	    NMloc(BIN, FILENAME, slavename, &executable);
	    if (LOexist(&executable) != OK)
	        return (FAIL);
	    LOtos(&executable, &str);
        }
        for (j = 0; j < NSVSEMS; j++)
        {
	    if (!CSslavepids[j])
	    {
	        break;
	    }
        }
        if (nexec + j > NSVSEMS)
        {
	    return(FAIL);
        }
        STcopy(str, path);
        if (name = STrchr(path, '/'))
        {	
	    name++;
        }
        else
        {
	    name = path;
        }
        NMgtAt("II_SLAVE_DEBUG", &str);
        ppos = path + STlength(path);
        nretrys = 10;
        i = 0;
	if ( 0 != (rad = CXnuma_cluster_rad()) )
	    CVna(rad, radbuf);

        while (i < nexec)
        {
	    CVna(i, ussubslv);
	    if (str && *str)
	    {
	        STcopy(ussubslv, ppos);
	    }
	    switch (CSslavepids[j] = PCfork(&status))
	    {
	    case 0:	/* in child */
	        close(0);
	        for (k = 1; k < FD_SETSIZE; k++)
	        {
		    if (!FD_ISSET(k, &fdmask))
		    {
		        close(k);
		    }
	        }
		if (rad)
		    execl(path, name, usstr, usslno, usservno, ussubslv, 
			  radbuf, fdstr, 0);
		else
		    execl(path, name, usstr, usslno, usservno, ussubslv,
			  fdstr, 0);
	        _exit(1);
	    case -1:
	        TRdisplay("Slave fork failed with status %d errno %d\n",
		          status, errno);
	        if (--nretrys)
	        {
		    sleep(2);
		    continue;
	        }
	        return(FAIL);
	    default:
	        ;
	    }
	    j++;	/* vfork critical - do not move into switch */
	    i++;
        }
        *slave_cb = slcb;
        for (i = 0; i < NSVSEMS; i++)
        {
	    if (CSslavepids[i] == 0)
	        break;
	    if (PCis_alive(CSslavepids[i]) == FALSE)
	        return (FAIL);
        }
    }

    return(OK);
}

/*{
** Name: CSreserve_event - reserve an event control block
**
** Description:
**    
**	Mark an idle event control block assigned to a slave group
**	as busy and return it to user.
**
** Inputs:
**	slave_cb			pointer to the slave control block
**
**
** Output:
**	evcb				set to point to event control block
**
**      Returns:
**          OK                         	    Function completed normally. 
**          non-OK status                   Function completed abnormally.
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	Under adverse conditions this routine may suspend execution of
**	the user until resources (evcb's) are available.
**
** History:
**      19-jan-88 (anton)
**          Created.
**	15-mar-88 (anton)
**	    New flags.
**	20-jul-89 (rogerk)
**	    When run out of event control blocks, call CS_ef_wait to wait
**	    for an event block to become free.
**	26-jun-90 (fls-sequent)
**	    Changed CS_TAS to CS_TAS.
**	25-oct-1995 (thaju02/stoli02)
**	    Added event waits and count counters.
*/
STATUS
CSreserve_event(slave_cb, evcb)
register CSSL_CB	*slave_cb;
CSEV_CB	**evcb;
{
    register CSEV_CB	*pevcb;
    register char	*ev_base;

    ev_base = CSev_base;
    if (slave_cb->flags & CSSL_SERVER)
    {
	ev_base = (char *)Cs_sm_cb;
    }
    for (;;)
    {
	for (pevcb = (CSEV_CB *)(ev_base + slave_cb->events);
	     pevcb < (CSEV_CB *)(ev_base + slave_cb->evend);
	     pevcb = (CSEV_CB *)((char *)pevcb + slave_cb->evsize))
	{
	    if (CS_TAS(&pevcb->busy))
	    {
		break;
	    }
	}

	Cs_srv_block.cs_wtstatistics.cs_event_count++;

	if (pevcb == (CSEV_CB *)(ev_base + slave_cb->evend))
	{
	  Cs_srv_block.cs_wtstatistics.cs_event_wait++;
          /*
          ** Wait for an event block to be freed.
          ** Mark that we are waiting for a free event block so that
          ** CSfree_event will resume us.
          */
          Cs_srv_block.cs_event_mask |= CS_EF_CSEVENT;
          CS_ef_set(CS_EF_CSEVENT);
          CS_ef_wait();
	}
	else
	{
	    break;
	}
    }
    pevcb->flags = EV_RSV;
    if (slave_cb->flags & CSSL_SERVER)
    {
	pevcb->flags = EV_RSV|EV_SERV;
    }
    *evcb = pevcb;
    return(OK);
}

/*{
** Name: CScause_event()	- Cause an event to begin
**
** Description:
**	Inform a slave that a request for event processing has
**	begun.  May notify one specific slave or a group depending
**	on initialization.
**
** Inputs:
**	evcb			Reserved event control block
**
**	slavenum		which slave in group to wake
**
** Outputs:
**
**	Returns:
**	    E_DB_OK or
**	    !OK
**
**	Exceptions:
**
** Side Effects:
**
** History:
**      19-jan-88 (anton)
**	    Created.
**	15-mar-88 (anton)
**	    New flags.
**	16-feb-89 (anton)
**	    Timeing related bug fix.  Be sure to set forslave before
**	    setting flags to allow a slave to service the evcb
**	16-jan-90 (fls-sequent)
**	    For MCT, added code to signal the process that is sleeping
**	    not the "main" process during wakeup.
**	21-mar-90 (fls-sequent)
**	    For MCT, set sid in evcb before causing the event to happen.
**	    Previously, the sid was set in CSsuspend.  But in MCT,
**	    events were completeing before the sid was set.
**	26-jun-90 (fls-sequent)
**	    Changed CS_TAS to CS_TAS and set only CS_TAS to CS_TAS.
**      22-jan-1997 (canor01)
**          Protect change in server control block's csi_events_outstanding
**          flag so changes do not get lost in concurrent access.
**  15-oct-97 (devjo01)
**      Moved set of 'forslave' to before CSget_sid() function call
**      to prevent AXP optimizing compiler from switching order of
**      assignment with 'flags' (*5679933.1).
*/
STATUS
CScause_event(evcb, slavenum)
register CSEV_CB	*evcb;
i4	slavenum;
{
    CSSL_CB		*slcb;
    
    /* first, null server-server events */
    if (!evcb)
    {
# ifdef OS_THREADS_USED
	CS_getspin(&Cs_sm_cb->css_servinfo[slavenum].csi_spinlock);
# endif /* OS_THREADS_USED */
	CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_nullev);
	CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_events_outstanding);
	if (CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_wakeme))
	{
	    kill((int)Cs_sm_cb->css_servinfo[slavenum].csi_signal_pid, SIGUSR2);
	}
# ifdef OS_THREADS_USED
	CS_relspin(&Cs_sm_cb->css_servinfo[slavenum].csi_spinlock);
# endif /* OS_THREADS_USED */

	return(OK);
    }
    /* normal events */
    if ((evcb->flags & (EV_INPG|EV_BGCM)) == EV_INPG)
    {
	return(FAIL);
    }
	 /* Putting slavenum assignment here to discourage optimizing
	    compilers from deciding to assign 'flags' first */
    evcb->forslave = slavenum;
    /* get sid for current thread */
    CSget_sid((CS_SID*)&evcb->sid);
    /* NOTE: The target slave must be set up before the flags bits are
       set to hand the evcb to the slave or a very very slim chance exists
       that the wrong slave will handle the operation */
    evcb->flags |= EV_INPG;
    if (evcb->flags & EV_BGCM)
    {
	evcb->flags |= EV_REST;
	evcb->flags &= ~EV_SLCP;
    }
    if (evcb->flags & EV_SERV)
    {
# ifdef OS_THREADS_USED
	CS_getspin(&Cs_sm_cb->css_servinfo[slavenum].csi_spinlock);
# endif /* OS_THREADS_USED */

	evcb->flags |= EV_SLCP;
	if (slavenum == -1)
	{
	    slavenum = evcb->retsemnum;
	}
	/* how to get back to us, this server */
	evcb->retsemnum = CSservernum;
	/* wake up right server */
	CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_event_done[evcb->slave_no]);
	CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_events_outstanding);
	if (CS_TAS(&Cs_sm_cb->css_servinfo[slavenum].csi_wakeme))
	{
	    kill((int)Cs_sm_cb->css_servinfo[slavenum].csi_signal_pid, SIGUSR2);
	}
# ifdef OS_THREADS_USED
	CS_relspin(&Cs_sm_cb->css_servinfo[slavenum].csi_spinlock);
# endif /* OS_THREADS_USED */
    }
    else
    {
	/* wake appropos slave */
	slcb = &((CSEV_SVCB *)CSev_base)->slave_cb[evcb->slave_no];
	if (CS_TAS(&Cs_svinfo->csi_subwake[slcb->slsembase + slavenum]))
	{
# ifdef xCL_075_SYS_V_IPC_EXISTS
	    CS_wakeup(slcb->slsemid, slcb->slsembase + slavenum);
# endif
# ifdef xCL_078_CVX_SEM
            CS_wakeup(&Cs_svinfo->csi_csem[slcb->slsembase + slavenum]);
# endif
	}
    }
    return(OK);
}

/*{
** Name: CSfree_event()	- Free a reserved event control block
**
** Description:
**	Release an event control block for use by other users.
**
** Inputs:
**	evcb				Reserved event control block to free
**
** Outputs:
**
**	Returns:
**	    E_DB_OK
**	    FAIL		Event can't be freed
**
**	Exceptions:
**
** Side Effects:
**	May resume execution of user waiting for an evcb
**
** History:
**      19-jan-88 (anton)
**          Created.
**	 1-dec-88 (anton)
**	    New restrictions on when CSfree_event is legal.
**	20-jul-89 (rogerk)
**	    If someone is waiting for a free control block, call CS_ef_wake.
**	21-mar-90 (fls-sequent)
**	    For MCT, to be safe clear sid in evcb when freeing an event.
**	26-jun-90 (fls-sequent)
**	    Changed CS_ACLR to CS_ACLR.
**	6-dec-95 (stephenb)
**	    After freeing an event control block we must search for threads
**	    waiting for event control blocks to become free and wake them.
*/
STATUS
CSfree_event(evcb)
register CSEV_CB	*evcb;
{
    if (evcb->flags & EV_INPG)
    {
	return(FAIL);
    }
    if (evcb->flags & EV_BGCM)
    {
	evcb->flags |= EV_PNDF;	/* free is pending */
    }
    else
    {
	evcb->flags = 0;
	evcb->sid = 0;
	CS_ACLR(&evcb->busy);
	/* wake threads waiting for an evcb to become free */
	if (Cs_srv_block.cs_event_mask & CS_EF_CSEVENT)
	{
	    Cs_srv_block.cs_event_mask &= ~CS_EF_CSEVENT;
	    CS_ef_wake(CS_EF_CSEVENT);
	}
    }
    return(OK);
}

/*{
** Name: CS_find_events - check for outstanding events and handle all.
**
** Description:
**    
**    CS_find_events is called by the dbms server to handle
**    any currently outstanding events.  This function calls the
**    event handler routines setup for each of the outstanding events.
**
**    When a disk operation is completed by DI an event will be raised 
**    indicating completion of that operation.  The DI handler for that event
**    will complete any local work necessary to complete the operation (status
**    information, buffer copies, ...) and then return the 
**    session id of the session whose I/O has completed.  CSresume will be
**    called for this session.
**
**    CS_find_event returns the number of events handled by setting 
**    "events_handled."
**
** Inputs:
**	reset_wakeme			OS threads only.
**					If TRUE and there are no 
**					events, reset csi_wakeme
**					while holding csi_spinlock,
**					implying the next action
**					by the caller will be to
**					enter some wakeable state,
**					i.e., CLpoll.
**
** Output:
**	num_of_events			number of different events
**					handled.
**
**      Returns:
**          OK status                   Function completed normally.
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	none
**
** History:
**      19-jan-88 (jaa)
**          Created.
**      26-jun-90 (fls-sequent)
**          Changed CS_ASET to SHM_ATOMIC, CS_ACLR to CS_ACLR, and
**	    CS_ISSET to CS_ISSET.
**	03-Oct-90 (anton)
**	    Turn off BGCM state for deferred CSfree_event
**	    Add support for deferred resume
**      01-may-95 (amo ICL)
**          Added ANB's gather write amendments
**      22-jan-1997 (canor01)
**          Protect change in server control block's csi_events_outstanding
**          flag so changes do not get lost in concurrent access.
**	10-oct-2002 (devjo01)
**	    Added logic to conditionally check for a missed event.
**	01-Aug-2005 (jenjo02)
**	    Reuse obsolete "timeout" as "reset_wakeme" to stop
**	    missed signals encountered by the mg5_osx port.
**	    When TRUE and there are no events, the caller is
**	    notifying CS_find_events that it will next enter
**	    a "wakeable" state (CLpoll) and that csi_wakeme should
**	    be cleared. We'll do that while holding the csi_spinlock.
**	    Before this change, the csi_spinlock would be released,
**	    then csi_wakeme cleared, allowing another server to 
**	    acquire csi_spinlock, fail CS_TAS(csi_wakeme), and 
**	    neglect the signal.
*/
STATUS
CS_find_events(reset_wakeme, num_of_events)
i4	*reset_wakeme;
i4	*num_of_events;
{
    static	i4	 counter = 0;
    static	i4	 threshhold = -1;

    CS_ASET	*ap;
    register i4	i;
    CSEV_SVCB		*svcb;
    
    *num_of_events = 0;
# ifdef OS_THREADS_USED
    if( Di_async_io)
    {
      DI_chk_aio_list();
    }
    
    CS_getspin(&Cs_svinfo->csi_spinlock);
# endif /* OS_THREADS_USED */
    if (!CS_ISSET(&Cs_svinfo->csi_events_outstanding))
    {
# ifdef OS_THREADS_USED
	if ( *reset_wakeme )
	    CS_ACLR(&Cs_svinfo->csi_wakeme);
	CS_relspin(&Cs_svinfo->csi_spinlock);
# endif /* OS_THREADS_USED */
	return(OK);
    }

    if ( threshhold < 0 )
    {
	char	*evp;

	NMgtAt( "II_FIND_EVENT_PARANOIA_CHECK_PERIOD", &evp );
	if ( NULL == evp || '\0' == *evp ||
	     OK != CVan( evp, &threshhold ) || threshhold < 0 )
	    threshhold = 0;
    }
    
    CS_ACLR(&Cs_svinfo->csi_events_outstanding);
    
    if (CS_ISSET(&Cs_svinfo->csi_nullev))
    {

      /* Because of the hp 800 compiler, we need to pass in a param
       * to the routine called by (*Cs_svinfo->csi_events.evcomp)().
       * Previous, NULL was passed and it caused rcpconfig much grief.
       * Note that the "tmp" param passed in is junk and is not expected
       * to be used.  It was only meant to satisfy the optimizer.
       */
      CSEV_CB tmp;  
      CS_ACLR(&Cs_svinfo->csi_nullev);
      (*Cs_svinfo->csi_events.evcomp)(tmp); 
      ++*num_of_events;
    }
    
    if (svcb = (CSEV_SVCB *)CSev_base)
    {
	for (ap = svcb->event_done, i = 0;
	     i < NEVUSERS;
	     i++, ap++)
	{
	    if (CS_ISSET(ap))
	    {
		CS_ACLR(ap);
		CS_handle_group(&svcb->slave_cb[i], num_of_events);
	    }
	}

	if ( threshhold != 0 && ++counter >= threshhold )
	{
	    i4	numevents;

	    for (ap = svcb->event_done, i = 0;
		 i < NEVUSERS;
		 i++, ap++)
	    {
		if ( !CS_ISSET(ap) )
		{
		    numevents = *num_of_events;
		    CS_handle_group(&svcb->slave_cb[i], num_of_events);
		    if ( !CS_ISSET(ap) && numevents != *num_of_events )
		    {
			TRdisplay( "%@ FIND_EVENT_PARANOIA vindicated\n" );
		    }
		}
	    }
	}
    }
    
#ifdef	notyet
    for (ap = Cs_svinfo->csi_event_done, i = 0;
	 i < Cs_sm_cb->css_numservers;
	 ap++, i++)
    {
	if (CS_ISSET(ap))
	{
	    CS_ACLR(ap);
	    CS_handle_group(&Cs_sm_cb->css_servinfo[i].csi_events,
			    num_of_events);
	}
    }
#endif
# ifdef OS_THREADS_USED
    if ( *num_of_events == 0 && *reset_wakeme )
	CS_ACLR(&Cs_svinfo->csi_wakeme);
    CS_relspin(&Cs_svinfo->csi_spinlock);
# endif /* OS_THREADS_USED */

    return(OK);
}

CS_handle_group(slcb, numevents)
register CSSL_CB	*slcb;
i4	*numevents;
{
    register CSEV_CB	*evcb;
    register char	*ev_base;
    CS_SID		resid = (CS_SID)0;

    ev_base = CSev_base;
    if (slcb->flags & CSSL_SERVER)
    {
	ev_base = (char *)Cs_sm_cb;
    }
    for (evcb = (CSEV_CB *)(ev_base + slcb->events);
	 evcb < (CSEV_CB *)(ev_base + slcb->evend);
	 evcb = (CSEV_CB *)((char *)evcb + slcb->evsize))
    {
	if (CS_ISSET(&evcb->busy)
	    && (evcb->flags & (EV_SLCP|EV_INPG)) == (EV_SLCP|EV_INPG)
	    && (!(evcb->flags & EV_SERV) || evcb->forslave == CSservernum))
	{
	    evcb->flags &= ~EV_INPG;
	    evcb->flags |= EV_BGCM;
	    ++*numevents;
	    (*slcb->evcomp)(evcb);
	    if (CSwaiting == evcb)
	    {
		CSwaiting = NULL;
	    }
	    if (evcb->flags & EV_DRES)
	    {
		resid = evcb->sid;
		evcb->flags &= ~EV_DRES;
	    }
	    if (evcb->flags & EV_REST)
	    {
		evcb->flags &= ~(EV_BGCM|EV_REST);
	    }
	    else
	    {
		if (evcb->flags & EV_PNDF)
		{
		    evcb->flags &= ~(EV_PNDF|EV_BGCM);
		    CSfree_event(evcb);
		}
		else
		{
		    evcb->flags = EV_RSV|(evcb->flags & EV_SERV);
		}
	    }
	    if (resid)
	    {
		CSresume(resid);
		resid = 0;
	    }
	}
    }
}

/*{
** Name: CSdef_resume()	- Resume an evcb durring completion code
**
** Description:
**	CSresume of an evcb during a completion routine is not allowed.
**	CSdef_resume will mark the evcb for deferred resume which will
**	inform the event system to resume the session in question
**	once the completion code has finished.
**
**	This solves some race conditions when completion routines
**	can run at any time such as with MCT.
**
** Inputs:
**	evcb			Event control block to resume.
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK		if ok
**	    !E_DB_OK		if timeout expired.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Session suspended will be resumed after the event completion
**	finishes.
**
** History:
**      03-oct-90 (anton)
**          Created.
*/
VOID
CSdef_resume(evcb)
CSEV_CB	*evcb;
{
	evcb->flags |= EV_DRES;
}

/*{
** Name: CSawait_event()	- Wait syncronously for a specific event.
**
** Description:
**	Wait for a specific event control block to be called by its
**	completion handler.  This is intended to allow syncronous calls
**	to be simply built from asyncronous calls.  While waiting,
**	other events may complete via their completion handlers.
**	If the event control block passed to CSawait_event is used to
**	call CScause_event within its own completion handler, CSawait_event
**	may return after the first completion handler returns and
**	possiby before the second completion handler call.
**
** Inputs:
**	timeout			timeout flag (0 for poll, -1 for block,
**					positive for timeout (not done))
**					if timeout was enabled,
**					time remaining is deposited
**	evcb			Event control block to wait for.
**
** Outputs:
**	timeout			if timeout was enabled,
**					time remaining is deposited
**
**	Returns:
**	    E_DB_OK		if ok
**	    !E_DB_OK		if timeout expired.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Other threads managed by CS do not run.
**	Other compleation handers will run.
**
** History:
**      03-feb-88 (anton)
**          Created.
*/
STATUS
CSawait_event(timeout, evcb)
i4	*timeout;
register CSEV_CB	*evcb;
{
    i4		nevents;
    STATUS	status;

    CSwaiting = evcb;
    status = OK;
    while (status == OK && CSwaiting == evcb && *timeout)
    {
	status = CS_find_events(timeout, &nevents);
	if(iiCLpoll(timeout) == E_FAILED)
                TRdisplay("CSawait_event: iiCLPoll failed... serious\n");
    }
    if (CSwaiting == evcb)
    {
	status = FAIL;
    }
    return(status);
}

/*{
** Name: CSinstall_server_event() - Set up server-server events.
**
** Description:
**	This call is similar to CSslave_init except that it sets up a
**	server or peudo-server for passing events (messages) to other
**	servers.  The paradigm is to allocate control blocks in installation
**	wide shared memory and inform other servers to look for events for
**	them in this servers event pool.
**
**	Currently, only one server event may be installed.
**
** Inputs:
**	num_events			number of events this server may
**					reserve at once.
**	size_events			size of user area of each event
**					block for this server.
**
** Outputs:
**	slave_cb			set to point to control block for
**					this server's events.
**
**	Returns:
**	    OK
**	    !OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    installation wide shared memory is reduced.
**
** History:
**      22-mar-88 (anton)
**          Created.
**	20-mar-89 (russ)
**	    Changed invokation of bzero() to MEfill().
**	26-jun-90 (fls-sequent)
**	    Changed CS_getspin to SHM_GETSPIN and CS_relspin to CS_relspin.
*/
STATUS
CSinstall_server_event(slave_cb, num_events, size_events, evcomp)
CSSL_CB	**slave_cb;
i4	num_events;
i4	size_events;
STATUS	(*evcomp)();
{
    register CSSL_CB	*slcb;
    register CSEV_CB	*evcb;
    register i4	i;
    STATUS		status;
    i4			j;
    
    status = CS_find_server_slot(&j);
    if (status)
    {
	return(status);
    }
    
    /* allocate slave cb */
    slcb = &Cs_sm_cb->css_servinfo[j].csi_events;
    if (slcb->nslave)
    {
	return(FAIL);
    }
    
    CS_getspin(&Cs_sm_cb->css_spinlock);
    
    slcb->slave_type = 0;	/* unused */
    slcb->flags = CSSL_SERVER;
# ifdef xCL_075_SYS_V_IPC_EXISTS
    slcb->slsembase = j;
# endif
# ifdef xCL_078_CVX_SEM
    slcb->slsembase = 0;
# endif
    slcb->evcomp = evcomp;
    slcb->evsize = size_events + sizeof(CSEV_CB);
    slcb->nevents = num_events;
    slcb->events = Cs_sm_cb->css_events_off;
    if (slcb->events + num_events * slcb->evsize >=
	Cs_sm_cb->css_events_end)
    {
	status = FAIL;
    }
    
    if (!status)
    {
	MEfill(num_events * slcb->evsize,'\0',(char *)Cs_sm_cb + slcb->events);
	slcb->evend = Cs_sm_cb->css_events_off += num_events * slcb->evsize;
	
	/* allocate event cbs */
	for (evcb = (CSEV_CB *)((char *)Cs_sm_cb + slcb->events);
	     evcb < (CSEV_CB *)((char *)Cs_sm_cb + slcb->evend);
	     evcb = (CSEV_CB *)((char *)evcb + slcb->evsize))
	{
	    evcb->retsemnum = j;
	    evcb->slave_no = j;
	    evcb->slave_type = 0;
	    evcb->datalen = size_events + sizeof(i4);
	}
	slcb->nslave = 1;	/* just to indicate that this is allocated */
	*slave_cb = slcb;
    }
    
    CS_relspin(&Cs_sm_cb->css_spinlock);
    return(status);
}

/*{
** Name: CSget_serverid() - return a number identifing this server
**
** Description:
**	Returns an identifier of this server that can be used for
**	server-server event processing.  The number is only unique to
**	an installation.
**
** Inputs:
**
** Outputs:
**      servid			server id
**
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-mar-88 (anton)
**          Created.
*/
STATUS
CSget_serverid(servid)
i4	*servid;
{
    return(CS_find_server_slot(servid));
}

/*
** Name: CS_set_wakeup_block		- record a pending cross-process resume
**
** Description:
**	This routine finds a free wakeup block and records the pid & sid of the
**	thread to be awoken. If there are no free wakeup blocks, it returns
**	FAIL.
**
** Inputs:
**	pid				- process ID which owns the thread
**	sid				- session ID to be awoken.
**
** Outputs:
**	none
**
** Returns:
**	OK				- wakeup recorded
**	!OK				- no free wakeup blocks
**
** History:
**	3-jul-1992 (bryanp)
**	    Created.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	31-jan-94 (mikem)
**	    bug #58298
**	    Changed CS_handle_wakeup_events() to maintain a high-water mark
**	    for events in use to limit the scanning necessary to find 
**	    cross process events outstanding.  Previous to this each call to 
**	    the routine would scan the entire array, which in the default 
**	    configuration was 4k elements long.
**	22-Mar-1999 (bonro01)
**	    SEGV occured in csphil.
**	    Replace hardcoded value with proper value for the number
**	    of wakeup control blocks.
*/
STATUS
CS_set_wakeup_block(PID	pid, CS_SID sid)
{
    CS_SM_WAKEUP_CB	*wakeup_blocks;
    CS_SM_WAKEUPS	*css_wakeup;
    i4			i;

    if (Cs_sm_cb)
    {
	CS_getspin(&Cs_sm_cb->css_spinlock);

	wakeup_blocks = (CS_SM_WAKEUP_CB *)
			    ((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);

	css_wakeup = &Cs_sm_cb->css_wakeup;

	for (i = css_wakeup->css_minfree; i < css_wakeup->css_numwakeups; i++)
	{
	    if (wakeup_blocks[i].inuse == 0)
	    {
		wakeup_blocks[i].pid = pid;
		wakeup_blocks[i].sid = sid;
		wakeup_blocks[i].inuse = 1;
		css_wakeup->css_minfree = i + 1;
		if (i > Cs_sm_cb->css_wakeup.css_maxused)
		    css_wakeup->css_maxused = i;
		CS_relspin(&Cs_sm_cb->css_spinlock);
		return(OK);
	    }
	}

	TRdisplay("CS: Examined %d wakeup blocks; all were in use!\n", i);
	for (i = 0; i < css_wakeup->css_numwakeups; i++)
	{
	    TRdisplay("Wakeup block %d: inuse %d pid %d sid %p\n",
			i, wakeup_blocks[i].inuse, wakeup_blocks[i].pid,
			wakeup_blocks[i].sid);
	}

	CS_relspin(&Cs_sm_cb->css_spinlock);
    }

    return (FAIL);
}

/*
** Name: CS_handle_wakeup_events 	- perform any needed thread wakeups
**
** Description:
**	This routine wakes up any threads which have scheduled wakeups that
**	were sent by other processes.
**
** Inputs:
**	pid				- this process's pid
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** Side Effects:
**	Some threads may be awoken.
**
** History:
**	3-jul-1992 (bryanp)
**	    created.
**	31-jan-94 (mikem)
**	    bug #58298
**	    Changed CS_handle_wakeup_events() to maintain a high-water mark
**	    for events in use to limit the scanning necessary to find 
**	    cross process events outstanding.  Previous to this each call to 
**	    the routine would scan the entire array, which in the default 
**	    configuration was 4k elements long.
**	5-Apr-2006 (kschendel)
**	    Preventive:  if running regular OS-threaded, shouldn't get
**	    here, just return (or could panic) if we do.
**	11-May-2009 (kschendel) b122041
**	    Remove ; after #endif, cpp complains.
*/
VOID
CS_handle_wakeup_events( PID pid )
{
    CS_SM_WAKEUP_CB	*wakeup_blocks;
    CS_SM_WAKEUPS	*css_wakeup;
    i4		local_minfree;
    i4			i;

/* If we're running MT, shouldn't get here unless we have to simulate
** cross process events using the idle thread.
** Perhaps this should be a trdisplay and panic.
*/
#ifndef USE_IDLE_THREAD
    if (Cs_srv_block.cs_mt)
	return;
#endif
    CS_getspin(&Cs_sm_cb->css_spinlock);

    wakeup_blocks = (CS_SM_WAKEUP_CB *)
			((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);
    css_wakeup = &Cs_sm_cb->css_wakeup;
    local_minfree = css_wakeup->css_minfree;

    /* starting at maximum used wakeup slot search backward for wakeups that
    ** apply to this server.
    */
    for (i = css_wakeup->css_maxused; i >= 0; i--)
    {
	if (wakeup_blocks[i].inuse == 1 &&
	    wakeup_blocks[i].pid == pid)
	{
	    CSresume(wakeup_blocks[i].sid);
	    wakeup_blocks[i].inuse = 0;
	    local_minfree = i;
	}
    }

    if (local_minfree < css_wakeup->css_minfree)
    {
	css_wakeup->css_minfree = local_minfree;
    }

    CS_relspin(&Cs_sm_cb->css_spinlock);

    return;
}
/*
** Name: CS_cleanup_wakeup_events 	- cleanup wakeup events.
**
** Description:
**	This routine steps down the array of wakeup blocks marking invalid
**	any with the given pid.
**	This routine is expected to be used to cleanup the wakeup blocks
**	at intialization to prevent bugs caused by reuse of pids, such as
**	bug # 52761.
**
** Inputs:
**	pid				- process id.
**	spinlock_held			- true if caller already holds spinlock
**
** Outputs:
**	none.
**
** Returns:
**	VOID
**
** Side Effects:
**	none.
**
** History:
**	23-aug-1993 (andys)
**	    created.
**	18-oct-1993 (bryanp)
**	    Added argument to CS_cleanup_wakeup_events to indicate whether
**		it needs to take the system segment spinlock or not.
**	31-jan-94 (mikem)
**	    bug #58298
**	    Changed CS_handle_wakeup_events() to maintain a high-water mark
**	    for events in use to limit the scanning necessary to find 
**	    cross process events outstanding.  Previous to this each call to 
**	    the routine would scan the entire array, which in the default 
**	    configuration was 4k elements long.
**	16-Nov-1998 (jenjo02)
**	    Added support for direct signalling of cross-process conditions
**	    instead of reliance on IdleThread/SIGUSR2 when OS threads are
**	    in use.
**	    Destroy CP mutex and condition when freeing wakeup blocks.
**	03-Mar-1999 (jenjo02)
**	    Fixed typos, wakeup_block[i] to &wakeup_blocks[i]
*/
VOID
CS_cleanup_wakeup_events( PID pid, i4  spinlock_held )
{
    CS_SM_WAKEUP_CB	*wakeup_blocks;
    i4			i;
    CS_SM_WAKEUPS	*css_wakeup;
    i4		local_minfree;

    if (Cs_sm_cb)
    {
	if (spinlock_held == 0)
	    CS_getspin(&Cs_sm_cb->css_spinlock);

	wakeup_blocks = (CS_SM_WAKEUP_CB *)
			    ((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);

	css_wakeup = &Cs_sm_cb->css_wakeup;
	local_minfree = css_wakeup->css_minfree;

	/* starting at maximum used wakeup slot search backward for wakeups that
	** apply to this server.
	*/

	for (i = css_wakeup->css_maxused; i >= 0; i--)
	{
	    if (wakeup_blocks[i].pid == pid)
	    {
#ifdef xDEBUG
		TRdisplay("CS_cleanup_wakeup_events %@ pid %d i %d\n", pid, i);
#endif
#ifdef OS_THREADS_USED
#ifndef USE_IDLE_THREAD
		if (wakeup_blocks[i].inuse && Cs_srv_block.cs_mt)
		{
		    CS_cp_synch_destroy(&wakeup_blocks[i].event_sem);
		    CS_cp_cond_destroy(&wakeup_blocks[i].event_cond);
		}
#endif /* USE_IDLE_THREAD */
#endif /* OS_THREADS_USED */
		wakeup_blocks[i].inuse = 0;
		local_minfree = i;
	    }
	}

	if (local_minfree < css_wakeup->css_minfree)
	{
	    css_wakeup->css_minfree = local_minfree;
	}

	if (spinlock_held == 0)
	    CS_relspin(&Cs_sm_cb->css_spinlock);
    }

    return;
}


#ifdef OS_THREADS_USED
#ifndef USE_IDLE_THREAD
/*
** Name: CSMT_resume_cp_thread 	- Resume a thread in another process.
**
** Description:
**	This function is for OS threads with cross-process condition/mutex
**	ability. It is called from CScp_resume() to resume a thread in 
**	another process.
**
** Inputs:
**	cpid				- pointer to the CP identity of
**					  the thread to be resumed.
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** Side Effects:
**	A thread in another process may be resumed.
**
** History:
**	15-Oct-1998 (jenjo02)
**	    created.
**	13-Feb-2007 (kiria01) b117581
**	    Give event_state its own set of mask values.
*/
VOID
CSMT_resume_cp_thread( 
CS_CPID		*cpid)
{
    CS_SM_WAKEUP_CB	*wakeup_block;

    if ( cpid && cpid->wakeup > 0 )
    {
        wakeup_block = (CS_SM_WAKEUP_CB *)
			((char *)Cs_sm_cb + cpid->wakeup);

	if (wakeup_block->inuse == 1)
	{
	    CS_cp_synch_lock( &wakeup_block->event_sem );

	    /* Validate the thread info */
	    if (wakeup_block->inuse == 1 &&
		wakeup_block->pid == cpid->pid &&
		wakeup_block->sid == cpid->sid)
	    {
		/* 
		** Only signal the condition if the thread is
		** waiting on the condition. This resume may happen 
		** before the thread has an opportunity to enter a 
		** suspended state. CSMTsuspend() checks for this by 
		** testing CS_EDONE_MASK before anything else.
		*/
		if (wakeup_block->event_state == EV_WAIT_MASK)
		    CS_cp_cond_signal( &wakeup_block->event_cond );
		wakeup_block->event_state |= EV_EDONE_MASK;
	    }

	    CS_cp_synch_unlock( &wakeup_block->event_sem );
	}
    }

    return;
}
#endif /* USE_IDLE_THREAD */

/*
** Name: CSMT_get_wakeup_block		- assign a wakeup block to an SCB
**
** Description:
**	This routine finds a free wakeup block and records the pid & sid of the
**	thread to be awoken. If there are no free wakeup blocks, it returns
**	FAIL.
**
** Inputs:
**	scb				CS_SCB of session
**
** Outputs:
**	none
**
** Returns:
**	OK				- wakeup assigned
**	!OK				- no free wakeup blocks
**
** History:
**	16-Nov-1998 (jenjo02)
**	    created.
**	27-Jan_1999 (schte01)
**	    Corrected cs_event_cond, cs_event_sem to event_cond, event_sem.
**	22-Mar-1999 (bonro01)
**	    SEGV occured in csphil.
**	    Replace hardcoded value with proper value for the number
**	    of wakeup control blocks.
*/
STATUS
CSMT_get_wakeup_block(CS_SCB *scb)
{
    CS_SM_WAKEUP_CB	*wakeup_blocks;
    CS_SM_WAKEUPS	*css_wakeup;
    i4			i;
    STATUS		status;

#ifdef USE_IDLE_THREAD
    /* Use local wakeup block embedded in SCB */
    scb->cs_evcb = &scb->cs_local_wakeup;
    CS_cond_init( &scb->cs_evcb->event_cond );
    CS_synch_init( &scb->cs_evcb->event_sem );
#if defined(xCL_SUSPEND_USING_SEM_OPS)
     /*
     ** USE_IDLE_THREAD defined implied cross process operations are
     ** not supported, so 2nd parameter must be 0 (private semaphore).
     */
     sem_init( &scb->cs_evcb->event_wait_sem, 0, 0 );

#endif
    scb->cs_evcb->inuse = 0;
    scb->cs_evcb->pid = 0;
    scb->cs_evcb->sid = (CS_SID)0;
    scb->cs_evcb->event_state = 0;
    return(OK);
#else
    if (Cs_sm_cb)
    {
	CS_getspin(&Cs_sm_cb->css_spinlock);

	wakeup_blocks = (CS_SM_WAKEUP_CB *)
			    ((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);

	css_wakeup = &Cs_sm_cb->css_wakeup;

	for (i = css_wakeup->css_minfree; i < css_wakeup->css_numwakeups; i++)
	{
	    if (wakeup_blocks[i].inuse == 0)
	    {
		wakeup_blocks[i].pid = scb->cs_pid;
		wakeup_blocks[i].sid = scb->cs_self;
		wakeup_blocks[i].inuse = 1;
		wakeup_blocks[i].event_state = 0;

		css_wakeup->css_minfree = i + 1;
		if (i > Cs_sm_cb->css_wakeup.css_maxused)
		    css_wakeup->css_maxused = i;

		CS_relspin(&Cs_sm_cb->css_spinlock);

		scb->cs_evcb = &wakeup_blocks[i];

		CS_cp_synch_init(&wakeup_blocks[i].event_sem, &status);
		CS_cp_cond_init(&wakeup_blocks[i].event_cond, &status);
		return(OK);
	    }
	}

	TRdisplay("CS: Examined %d wakeup blocks; all were in use!\n", i);
	for (i = 0; i < css_wakeup->css_numwakeups; i++)
	{
	    TRdisplay("Wakeup block %d: inuse %d pid %d sid %p\n",
			i, wakeup_blocks[i].inuse, wakeup_blocks[i].pid,
			wakeup_blocks[i].sid);
	}

	CS_relspin(&Cs_sm_cb->css_spinlock);
    }

    scb->cs_evcb = (CS_SM_WAKEUP_CB *)0;

    return (FAIL);
#endif /* USE_IDLE_THREAD */
}

/*
** Name: CSMT_free_wakeup_block 	- Release a wakeup event cb at thread end.
**
** Description:
**	This function is for OS threads with cross-process condition/mutex
**	ability. It is called at thread termination to release the wakeup   
**	block for the thread.
**
** Inputs:
**	scb				  the thread's CS_SCB.
**	    cs_evcb			  pointer to thread's wakeup block.
**
** Outputs:
**	scb->cs_evcb wakeup block pointer is voided
**
** Returns:
**	VOID
**
** Side Effects:
**	A wakeup block is made available.
**
** History:
**	15-Oct-1998 (jenjo02)
**	    created.
**	5-Apr-2006 (kschendel)
**	    Fix minfree goof, don't divide by sizeof, C does it already.
*/
VOID
CSMT_free_wakeup_block( 
CS_SCB 		*scb)
{
    CS_SM_WAKEUP_CB	*wakeup_block;
    CS_SM_WAKEUP_CB	*wakeup_blocks;
    CS_SM_WAKEUPS	*css_wakeup;

    if (wakeup_block = scb->cs_evcb)
    {
#ifdef USE_IDLE_THREAD
	if (wakeup_block->inuse == 1)
	{
	    CS_synch_destroy( &wakeup_block->event_sem );
	    CS_cond_destroy( &wakeup_block->event_cond );
	}
#else
	wakeup_blocks = (CS_SM_WAKEUP_CB *)
			    ((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);

	css_wakeup = &Cs_sm_cb->css_wakeup;

	CS_getspin(&Cs_sm_cb->css_spinlock);

	if (wakeup_block->inuse == 1 &&
	    wakeup_block->pid == scb->cs_pid &&
	    wakeup_block->sid == scb->cs_self)
	{
	    wakeup_block->inuse = 0;
	    CS_cp_synch_destroy(&wakeup_block->event_sem);
	    CS_cp_cond_destroy(&wakeup_block->event_cond);

	    /* New minfree? */
	    if ((wakeup_block - wakeup_blocks) < css_wakeup->css_minfree)
	    {
		css_wakeup->css_minfree = wakeup_block - wakeup_blocks;
	    }
	}

	CS_relspin(&Cs_sm_cb->css_spinlock);
#endif /* USE_IDLE_THREAD */
	scb->cs_evcb = (CS_SM_WAKEUP_CB *)NULL;
    }

    return;
}
#endif /* OS_THREADS_USED */

/*
** Name: CS_clear_wakeup_block            - Clear a pending cross-process resume
**
** Description:
**      This routine searches the wakeup block for a record corresponding to
**      pid & sid of the thread to be cleared. If an entry is found the entry
**      is cleared.
**
** Inputs:
**      pid                             - process ID which owns the thread
**      sid                             - session ID to be cleared.
**
** Outputs:
**      none
**
** Returns:
**      none
**
** History:
**      15-Aug-2001 (horda03)
**          Created.
*/
VOID
CS_clear_wakeup_block(PID pid, CS_SID sid)
{
    CS_SM_WAKEUP_CB     *wakeup_blocks;
    CS_SM_WAKEUPS       *css_wakeup;
    i4                 i;

    CS_getspin(&Cs_sm_cb->css_spinlock);

    wakeup_blocks = (CS_SM_WAKEUP_CB *)
                        ((char *)Cs_sm_cb + Cs_sm_cb->css_wakeups_off);

    css_wakeup = &Cs_sm_cb->css_wakeup;

    for (i = css_wakeup->css_maxused; i >= 0; i--)
    {
        if ((wakeup_blocks[i].inuse == 1) &&
            (wakeup_blocks[i].pid == pid) &&
            (wakeup_blocks[i].sid == sid) )
        {
            wakeup_blocks[i].inuse = 0;
            if (i < css_wakeup->css_minfree)
                css_wakeup->css_minfree = i;

            TRdisplay( "CS_clear_wakeup_block Removed a Pending CP resume for %08x\n",
                        sid);

            break;
        }
    }

    CS_relspin(&Cs_sm_cb->css_spinlock);
}

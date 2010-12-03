/*
** Copyright (c) 1985, 2003 Ingres Corporation
**
**	Name:
**		pcreap.c
**
**      Function:
**              Examine child process list looking for exit code(s) 
*/

# include 	<compat.h>	/* compatability library header */
# include 	<gl.h>	/* compatability library header */
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<ex.h>
# include	<cs.h>
# include	<pc.h>
# include	<qu.h>

# if defined (any_aix) || defined(OSX)
# include       <sys/wait.h>
# elif defined (rmx_us5)
# include </usr/ucbinclude/sys/wait.h>
# elif !defined(thr_hpux)
# include	<wait.h>
# endif

# include	<errno.h>
# include	<PCerr.h>
# include	"pclocal.h"
# include	"pcpidq.h"

# ifdef xCL_001_WAIT_UNION
# include	<sys/wait.h>
# if defined(any_aix)
# include <sys/m_wait.h>
# endif
# endif

/* External variables */
GLOBALREF QUEUE		Pidq;

/* 
**	Function:
**		PCreap_withThisPid -- wait for dead process, usually after SIGCLD.
**
**	Arguments:
**		none.
**
**	Result:
**		Looks to see for a process has died
**
**	Returns:
**		Pid of the process reaped, or -1 if not reaped.
**
**	Side Effects:
**		The status of the reaped process is saved in the Pidqueue,
**		for examination by PCwait.
**
**	History:
**	31-Oct-1988 (daveb)
**		created.
**	26-may-89 (daveb)
**		silence warnings by declaring handles TYPESIG instead of int.
**	25-mar-91 (kirke)
**		Added #include <systypes.h> because HP-UX's signal.h includes
**		sys/types.h.
**	21-mar-91 (seng)
**		wait union struct declared in <sys/m_wait.h> on AIX 3.1.
**	26-apr-1993 (fredv)
**	    Moved <clconfig.h> and <clsigs.h> before <ex.h> to avoid
**		redefine of EXCONTINUE in the system header sys/context.h
**		of AIX.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      10-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      05-jun-1995 (canor01)
**          semaphore protect global Pidq
**	03-jun-1996 (canor01)
**	    remove above semaphore since code is not used multi-threaded.
**	05-Dec-1997 (allmi01)
**	    Added special #defines and #undefs fro sgi_us5 to get he wait
**	    struct to be included from sys/wait.h.
**	10-Dec-1998 (kosma01)
**	    sqs_ptx ran afoul of CA Licensing on a multiprocessor box.
**	    The DBMS server would loop during startup or stop responding later.
**	    The sever was catching a SIGCHLD signal from CA Licensing, but had
**	    only iislaves to wait() for. Changing just PCreap() to use waitpid() 
**	    instead of wait() fixed the failing server on DYNIX/ptx 4.4 but
**	    broke the working server on DYNIX/ptx 4.2. I then stopped CSsigchld()
**	    from reestablishing itself via i_EXsetothersig(), because the OS does
**	    it for sigaction established signal handlers. Someday, a brave soul 
**	    should check if the CSsigchld() change by it self is sufficient, the
**	    waitpid() approach is slow.
**	23-jan-1998 (canor01)
**	    restore the protection on the Pidq.
**	23-Mar-1999 (kosma01)
**	    Remove special #defines and #undefs for sgi_us5, they are no
**	    longer needed to get a clean compile and execution.
**      01-Oct-1999 (linke01)
**          Structure not found in <wait.h> or <sys/wait.h>. Included
**          </usr/ucbinclude/sys/wait.h> for rmx_us5.
**	13-sep-2000 (hayke02)
**	    The sqs_ptx only fix for 94452 (CA licensing problems on
**	    multiprocessor boxes - dmfrcp or iidbms waiting for iislave
**	    to die during ingstart) has been extended to all platforms
**	    (even platforms supporting OS threading can run iislaves
**	    if II_THREAD_TYPE is set to internal). This change fixes
**	    bug 98804, problem INGSRV 892.
**	01-Jan-2000 (bonro01)
**	    Enable (kosma01) change from 10-Dec-1998 for dgi.
**	12-Apr-2000 (hweho01)
**	    Added ris_u64 to the list of platforms that need <sys/m_wait.h>.
**      07-jun-2000 (stial01)
**          Removed Pidq_mutex. (b101767, Startrak 9027036)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-May-2001 (hanje04)
**	    Cleaned up after bad X-integ.
**	24-May-2001 (hanje04)
**	    Extend sqs_ptx inclusion of wait.h to all platforms
**	    to ensure WNOHANG is defined.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      01-Oct-2002 (hanal04) Bug 108818
**          Do not include wait.h if we have included sys/wait.h
**          This resolves build errors found during mark 2232 update.
**	28-feb-2003 (somsa01)
**	    Do not include wait.h on HP-UX.
**	02-May-2003 (hweho01)
**	    The wait.h is located in include/sys directory on AIX.
**	28-Apr-2005 (hanje04)
**	    wait.h is located in include/sys on Mac OS X.
**	10-Oct-2006 (hanje04)
**	    BUG: 116919
**	    Use wait() macros (WIEXITED, WEXITSTATUS etc.) to determine
**	    the exit status of child processes correctly. This was not
**	    being achieved on Linux and probably other platforms.
**	09-Oct-2006 (hanje04)
**	    BUG: 117445
**	    SD: 113106
**	    Make sure PCreap stores the correct status when a child process
**	    receives a signal. Only if the process has terminated should we
**	    store PC_WT_TERM in sqp->stat. Otherwise, PC_WT_INTR is stored.
**	12-Feb-2006 (hanje04)
**	    BUG: 117445
**	    SD:113106
**	    Remove "if WIFCONTINUED()" case from waitpid loop as it is not
**	    defined in earlier versions of GLIBC. It shouldn't be needed
**	    anyway as waitpid() is called without WCONTINUED, meaning it
**	    should be reported.
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**      29-Dec-2008 (coomi01) b121587
**          Introduce targetPid parameter. Renamed function from PCreap()
**          to maintain acustomed interface by adding new PCreap() below.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/



i4
PCreap_withThisPid(int targetPid)
{
#ifdef xCL_001_WAIT_UNION
    union wait wait_status;
#else
    int	wait_status;
#endif

    i4	proc_stat;
    i4  sys_stat;

    register PIDQUE	*sqp;		/* search queue pointer */
    register PID	pid;
    register STATUS	rval;

    TYPESIG (*inthandler)();
    TYPESIG (*quithandler)();

    /* 
    ** If the Pidq doesn't exist, no point waiting for the child 
    ** since we don't care anyway.
    */
    if(Pidq_init == FALSE)
	return( -1 );	

    /*
    ** Disable the SIGINT and SIGQUIT, and save their old actions.
    ** EXinterrupt(EX_OFF) now latches incoming keyboard interrupts,
    ** rather than discarding them; we need to actually turn the
    ** signal handling off.
    */
    inthandler = signal(SIGINT, SIG_IGN);
    quithandler = signal(SIGQUIT, SIG_IGN);

    /*
    ** Set a default result-code
    ** - As above return code, meaning "Child does not exist"
    */
    pid = -1;

    for (sqp = (PIDQUE *) Pidq.q_next;
                sqp != (PIDQUE *) &Pidq;
                sqp = (PIDQUE *) sqp->pidq.q_next )
    {
        /*
        ** Are we looking for a specific child to checkup on ?
        */
        if (targetPid != -1)
        {
            /*
            ** Pick out that child by stepping right over the others.
            */
            if (targetPid != sqp->pid)
                continue; /** the for loop **/

            /*
            ** The child does exist, so change the default result-code.
            ** - Now means "Child state changed from executable"
            */
            pid = targetPid;
        }

        /*
        ** Still executing this child ?
        */
        if ( sqp->stat == PC_WT_EXEC )
        {
           /*
           ** Loop until no error (-1) attempting to read status of this pid
           ** - A zero result here now means "pid reports no state change"
           ** - WNOHANG is important, this call will not block if child
           **   still running ... so we may use routine a a polling agent.
           */
           while (((pid = waitpid ( sqp->pid, (int *)&wait_status, WNOHANG)) == -1) && (errno == EINTR));

            if ( pid == sqp->pid )
            {
                /* error pid needs no special handling; it falls on the floor */

# ifdef xCL_001_WAIT_UNION
               proc_stat = wait_status.w_retcode;
               sys_stat = wait_status.w_status & ~(proc_stat);
# else
# ifdef xCL_USE_WAIT_MACROS
		if ( WIFEXITED( wait_status ) )
		{
		   /* process exited, set PC status based on exit value */
		   proc_stat = WEXITSTATUS( wait_status );
 		   sqp->stat = proc_stat == 0 ?
				OK : PC_WT_BAD ;
		   break;
		}
		else if ( WIFSIGNALED( wait_status ) )
		{
		    /* process terminated by signal */
		    sqp->stat = PC_WT_TERM;
		    break;
		}
		else if ( WIFSTOPPED( wait_status ) )
		{
		    /* process interrupted by signal */
		    sqp->stat = PC_WT_INTR;
		    break;
		}
		else
# endif
		{
		    proc_stat = wait_status & 0377;
		    sys_stat = wait_status & ~(proc_stat);
		}
# endif
    /*
    ** Map the status of the dead child to a PC_WT_code.
    */

               sqp->stat = proc_stat == 0 ? OK : sys_stat == 0 ? PC_WT_TERM : PC_WT_BAD;
               break;
            }
        }

        /*
        ** Were we were targetting a particular pid ?
        */
        if (targetPid != -1)
        {
	    /* 
	    ** Break  out of for-loop 
	    */
            break;
        }
    }

    /* reset interrupt and quit signals */

    (void) signal(SIGINT, inthandler);
    (void) signal(SIGQUIT, quithandler);

    return( pid );
}

/*
** Copyright (c) 1985, 2003 Ingres Corporation
**
**    Name:
**            pcreap.c
**
**    Function:
**            PCreap -- wait for any dead process, usually after SIGCLD.
**
**    Arguments:
**            none.
**
**    Result:
**            waits for a processes to die
**
**    Returns:
**            Pid of process reaped, or -1 if none reaped.
**
**      29-Dec-2008 (coomi01) b121587
**         Create stub routine to maintain traditional interface to PCreap(),
**         calling new interface with new target pid parameter.
**      15-nov-2010 (stephenb)
**          Correctlty define PCreap() for prototyping.
*/
i4
PCreap(void)
{
    /*
    ** Call renamed PCreap() function with default parameter
    */
    return PCreap_withThisPid(-1);
}


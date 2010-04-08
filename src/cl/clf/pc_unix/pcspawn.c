/*
**	Copyright (c) 1983, 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<pc.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<er.h>

# include	<clconfig.h>
# include	<clsigs.h>
# include       <ex.h>
# include	<errno.h>
# include	<unistd.h>

# include	<PCerr.h>
# include	"pclocal.h"

/*
 *	Name:
 *		PCspawn.c
 *			routines defined
 *				PCspawn()
 *				no_fork()
 *				no_exec()
 */




/*
 *	Name:	PCspawn.c
 *
 *	Function:
 *		PCspawn
 *
 *	Arguments:
 *		i4		argc;
 *		char		**argv;
 *		bool		wait;
 *		LOCATION	*in_name;
 *		LOCATION	*out_name;
 *		PID		*pid;
 *
 *	Result:
 *		Spawn a subprocess.
 *
 *		create a new process with it's standard input and output 
 *		linked to LOCATION's named by the caller.  Null LOCATIONS 
 *		means use current stdin and stdout.
 *
 *		Returns:
 *			OK		-- success
 *			FAIL		-- general failure
 *			PC_SP_CALL	-- arguments incorrect
 *			PC_SP_EXEC	-- cannot execute command specified, 
 *					   bad magic number
 *			PC_SP_MEM	-- system temporarily out of core or 
 *					   process requires too many 
 *					   segmentation registers.
 *			PC_SP_OWNER	-- must be owner or super-user to 
 *					   execute this command
 *			PC_SP_PATH	-- part of specified path didn't exist
 *			PC_SP_PERM	-- didn't have permission to execute
 *			PC_SP_PROC	-- system process table is temporarily 
 *					   full
 *			PC_SP_REOPEN	-- couldn't associate argument 
 *					   [in, out]_name with std[in, out]
 *			PC_SP_SUCH	-- command doesn't exist
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb) written
 *		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
 *		10-oct-88 (daveb)
 *			reformatted, include clconfig.h, rename PC.h to
 *			pclocal.h.
 *
 *			Eliminate use of BSD define by always checking for
 *			access permision.  We don't really know if we are
 *			using vfork or fork, and it doesn't hurt.
 *
 *			Simplify tortuous ?: operations on reopen.
 *
 *			We have a problem in that we want everyone to use
 *			PCwait for child accounting, but we use MEreqmem,
 *			that some people don't want.  We may need to rethink
 *			whether dynamic allocation is such a good idea.
 *	3-Dec-1988 (daveb)
 *			Bury fork and additions to the pid queue by using
 *			a new internal routing, PCfork(), to be used
 *			throughout the CL.
 *	31-may-89 (daveb)
 *			PCfork takes an argument.  Wasn't providing one.
 *	19-mar-90 (Mike S)
 *		  	Add support for appended output files and redirected
 *			stderr.
**	25-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Flush the TR buffers to output;
**		Set up the default handler for SIGC(H)LD to keep sub-shells
**		happy.
**	29-may-90 (blaise)
**	    Changed SIGCLD to SIGCHLD. SIGCLD is the sysV equivalent of 
**	    SIGCHLD, and it seems that, while sysV boxes know about
**	    both of these, some bsd boxes do not.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	12-apr-95 (abowler)
**	    Bug 52338 - renamed setsig to EXsetsig.
**	11-nov-1997 (canor01)
**	    When PC_NO_WAIT is specified, when the spawned process exits,
**	    it will become "defunct", waiting for its parent to reap it.
**	    Instead, fork an intermediate process to be its parent that
**	    will immediately exit. Caller can reap this one.
**	20-nov-1997 (walro03)
**	    Place clconfig.h, clsigs.h, and ex.h in proper order to avoid
**	    compile error redefining EXCONTINUE on AIX.
**      02-sep-1998 (canor01)
**	    Change PCexit() to _exit() when terminating the intermediate
**	    process created in PC_NO_WAIT.  This will prevent it from closing
**	    files held open by the parent.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (hweho01)
**          1) In PCdospawn(), Child process also calls PCfork() if the BSD
**              version is used.
**          2) Changed to compare with -1 when perform the error check for   
**             BSD's setpgrp() function.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Include unistd.h to quite compiler warnings for system functions.
*/


STATUS
PCspawn(argc, argv, wait, in_name, out_name, pid)
i4		argc;
char		**argv;
bool		wait;
LOCATION	*in_name;
LOCATION	*out_name;
PID		*pid;
{
    STATUS PCdospawn();

    /* 
    ** Someday the added functionality of appended output and redirected
    ** stderr may be part of PCspawn.
    */
    return PCdospawn(argc, argv, wait, in_name, out_name, FALSE, FALSE, pid);
}

STATUS PCdospawn(argc, argv, wait, in_name, out_name, append, rederr, pid)
i4              argc;
char            **argv;
bool            wait;
LOCATION        *in_name;
LOCATION        *out_name;
i4		append;		/* non-zero to append, rather than recreate,
				** output file */
i4		rederr;		/* Non-zero to redirect stderr to err_log */
PID		*pid;
{
    STATUS		no_exec();
    char		buf[64];
    char		*in_fname;
    char		*out_fname;
    STATUS		PCwait();
    TYPESIG             (*old_handler)(), (*EXsetsig())();
    int			flags = 0;
    STATUS		status;

    /* Flush the output buffers to make sure all child output
    ** follows the parent output.  The streams may be buffered
    ** if, for example, they were redirected to a file.
    */
    SIflush(stdout);
    SIflush(stderr);
    TRflush();

   /* Don't want to mess up a sub-shell's handling of this by leaving
    ** it something odd.  This might be a problem if something is
    ** run out of a DBMS server and an iislave happens to die while the
    ** subprocess spawned here is running.  The symptom of this not working
    ** right is ckpdb failing, because the shell's handling of child death
    ** was messed up by our redirected handling of SIGCHLD.  Frankly, that
    ** smells odd to me, but what the heck... (daveb).
    */
    old_handler = EXsetsig(SIGCHLD, SIG_DFL);

    if ( (argc < 1) || (argv[0] == NULL))
    {
	PCstatus = PC_SP_CALL;
    }

    /*
    ** If we are using fork() instead of vfork(), we must check
    ** to see if argument is executable now, because
    ** PCstatus from child process won't be available
    ** to parent.  We don't know if we are using fork, so
    ** always check.
    */

    else if (access(argv[0], 01) == BAD_ACCESS)
    {
	switch (errno)
	{
	case EACCES:
	      /* error occurred because path was inaccessable. */
	      PCstatus = PC_SP_PERM;
	      break;

	case EPERM:
	      PCstatus = PC_SP_OWNER;
	      break;

	case ENOTDIR:
	      /* error occurred path didn't exist */
	      PCstatus = PC_SP_PATH;
	      break;

	case ENOENT:
	      /* error occurred path didn't exist */
	      PCstatus = PC_SP_SUCH;
	      break;
	}
    }
    else if ( (*pid = (PID)PCfork( &PCstatus )) > 0 )	/* parent */
    {
	/*	
	** (v)fork returns control to parent after exec.
	** Calling process can ask PCspawn() to wait for
	** the child, wait itself or choose not to wait.
	*/
	/*
	** in the case of PC_NO_WAIT, we are only waiting for
	** the intermediate process, which will also do fork/exec
	*/
	PCstatus = PCwait(*pid);
    }
    else if ( *pid == 0 )				/* child */
    {
	if ( !wait )
	{
#if !defined xCL_086_SETPGRP_0_ARGS
            /* BSD flavour setpgrp */
            if (setpgrp(0, getpid()) == -1 ) 
            {
                status = errno;
		SIprintf("Can't change process group for spawned process\n");
		PCexit(status);
            }

# ifdef TIOCNOTTY
            /* say goodbye to our control terminal */
            if ( (fd = open("/dev/tty", O_RDWR) ) >= 0 )
            {
		ioctl(fd, TIOCNOTTY, (char *) NULL);
		close(fd);
            }
# endif /* TIOCNOTTY */

#else
            /* SYSV flavour setpgrp */
            if(setpgrp() == -1) /* create new pgrp, lose control terminal */
            {
		status = errno;
		SIprintf("Can't change process group for spawned process\n");
		PCexit(status);
            }

#endif

            /* fork again, so we can't reacquire a control terminal */

            if ( (*pid = PCfork(&status)) < 0 )
            {
		status = errno;
		SIprintf("Can't fork again for spawned process\n");
		PCexit(status);
            }

            else if (*pid > 0)
            {
		/* intermediate parent */
		_exit(OK); 
            }

            /* reset all the signal handlers to default */
            EXsetsig( SIGCHLD, SIG_DFL );

	}
	if (in_name != NULL)
	{
	    LOtos(in_name, &in_fname);

	    if ( *in_fname && freopen(in_fname, "r", stdin) != stdin )
		PCstatus =  PC_SP_REOPEN;
	}

	if (PCstatus == OK)
	{
	    if (out_name != NULL)
	    {
		char *mode = (append != 0) ? ERx("a") : ERx("w");
		LOtos(out_name, &out_fname);

		if ( *out_fname )
		{
		    if (freopen(out_fname, mode, stdout) != stdout)
		    {
		    	PCstatus = PC_SP_REOPEN;
		    }
		    else if (rederr != 0)
		    {
			close(2);
			dup(1);
		    }
		}
	    }

	    if (PCstatus == OK)
	    {
		STcopy(argv[0], buf);
		argv[argc] = '\0';	/* insure argv set up properly */

		execvp(buf, argv);

		/* should never reach here, because we checked first */

		no_exec(buf);
	    }
	}

	/* Should only reach here we had a PC_SP_REOPEN failure */

	_exit( FAIL );
    }

    (void) EXsetsig(SIGCHLD, old_handler);
    return( PCstatus );
}



/*
 *
 *	Name:
 *		PCspawn.c
 *
 *	Function:
 *		no_fork
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		called by PCspawn and PCsspawn when call to [v]fork() fails.
 *
 *	Returns:
 *		PC_SP_PROC	-- system process table is temporarily full
 *		PC_SP_MEM	-- system temporarily out of core or 
 *					process requires too many
 *					segmentation registers.
 *		FAIL		-- general failure
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
 */
 

STATUS
no_fork()
{
	switch (errno)
	{
	case EAGAIN:
		PCstatus = PC_SP_PROC;
		break;

	case ENOMEM:
		PCstatus = PC_SP_MEM;
		break;

	default:
		PCstatus = FAIL;
		break;
	}

	return(PCstatus);
}



/*
 *
 *	Name:
 *		no_exec
 *
 *	Function:
 *		no_exec
 *
 *	Arguments:
 *		char	*command;
 *
 *	Result:
 *		called by PCspawn() or PCsspawn() when a call to exec fails.
 *			Returns reason for failure and cleans up environment.
 *
 *		Returns:
 *		    FAIL	-- general failure
 *		    PC_SP_EXEC	-- cannot execute command specified, bad 
 *					magic number
 *		    PC_SP_PERM	-- didn't have permission to execute
 *		    PC_SP_OWNER	-- must be owner or super-user to execute
 *					this command
 *		    PC_SP_PATH	-- part of specified path didn't exist
 *		    PC_SP_SUCH	-- command doesn't exist
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
 */
 

STATUS
no_exec(command)
char	*command;
{
	PCstatus = FAIL;

	if (errno == ENOEXEC)
	{
		PCstatus = PC_SP_EXEC;
	}
	else if (access(command, EXECUTE) == BAD_ACCESS)
	{
		switch (errno)
		{
		  case EACCES:
			/* error occurred because path was inaccessable */
			PCstatus = PC_SP_PERM;
			break;

		  case EPERM:
			PCstatus = PC_SP_OWNER;
			break;

		  case ENOTDIR:
			/* error occurred path didn't exist */
			PCstatus = PC_SP_PATH;
			break;

		  case ENOENT:
			/* error occurred path didn't exist */
			PCstatus = PC_SP_SUCH;
			break;
		}
	}

	(void)_exit( FAIL );	
	/*NOTREACHED*/
}

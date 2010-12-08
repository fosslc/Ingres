# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	<systypes.h>
# include	<clconfig.h>

# include	<ex.h>
# include	<pc.h>
# include	<si.h>

# include	<errno.h>
# include	<clnfile.h>

# include	<PCerr.h>
# include	"pclocal.h"

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		PCsspawn.c
**
**	Function:
**		PCsspawn
**
**	Arguments:
**		i4	argc;
**		char	**argv;
**		bool	wait;
**		PIPE	*c_stdin;	* PIPE pointer (file descriptor) to be
**					  associated with child's stdin *
**		PIPE	*c_stdout;	* PIPE pointer (file descriptor) to be
**					  associated with child's stdout *
**		PID	*pid;		* process id of spawned child *
**
**	Result:
**		Spawn slave subprocess.
**
**		Create a new process with its standard inpout and output linked
**			to file descriptors (c_stdin and c_stdout) supplied by
**			the caller.
**
**		Returns:
**			OK		-- success
**			FAIL		-- general failure
**			PC_SP_CALL	-- arguments incorrect
**			PC_SP_EXEC	-- cannot execute command specified,
**						bad magic number
**			PC_SP_MEM	-- system temporarily out of core or
**						process requires too
**						many segmentation registers.
**			PC_SP_OWNER	-- must be owner or super-user to
**						execute this command
**			PC_SP_PATH	-- part of specified path didn't exist
**			PC_SP_PERM	-- didn't have permission to execute
**			PC_SP_PROC	-- system process table is temporarily
**						full
**			PC_SP_SUCH	-- command doesn't exist
**			PC_SP_CLOSE	-- had trouble closing unused sides of
**						pipe
**			PC_SP_DUP	-- had trouble associating, dup()'ing,
**						the pipes with the child's
**						std[in, out]
**			PC_SP_PIPE	-- couldn't establish communication
**						pipes
**
**	Side Effects:
**		None
**
**	History:
**	Log:	pcsspawn.c,v $
**		Revision 1.2  86/04/29  10:11:13  daveb
**		remove unused variabld "fd"
**
**		Revision 1.2  86/03/10  01:45:05  perform
**		New pyramid compiler can't handle declaration of static's
**		within scope of use.
**
**		03/83	--	(gb) written
**		6/12/86 --	(ljm) Bug #9626:  integrate Annie's fix for bug
**				#7822 from 3.0:
*	+(1/28/86 (ac) -- bug # 7822 fix. setuid(geteuid()) after
*	+		fork() but before exec() backend in child's
*	+		process address space. In this way, only
*	+		the real uid of the child process is set
*	+		to its effective uid before spawns backend
*	+		and the frontend program still has its
*	+		original real uid.)
*
*		9/25/86 --	(daveb) Remove the setuid stuff for 
*			7822 and 9626.  This was a botch.  The original
*			problem (accessdb) has long since been fixed by
*			correct methods without the side-effects of setuid.
**			This code was not portable to System V, and was
*			incorrect, causing a segmentation violation on Sun.
*		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
*
*		10-oct-88 (daveb)
*			reformatted, include clconfig.h, rename PC.h to
*			pclocal.h.
*
*			Eliminate use of BSD define by always checking for
*			access permision.  We don't really know if we are
*			using vfork or fork, and it doesn't hurt.
*
*	3-Dec-1988 (daveb)
*			Bury fork and queue manipulation in new routine, 
*			PCfork(), to be used throughout the CL.
*	8-may-89 (daveb)
*			Do fd set size with CL_NFILE() macro instead of
*			using _NFILE or getdtablesize directly.
* 	31-may-89 (daveb)
* 			PCfork takes an argument.  We weren't.  Give it one.
**	25-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Flush the TR buffers to output;
**		Remove EX_NO_FD stuff left over from 5.0.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's unistd.h (included
**	    by clnfile.h) includes sys/types.h.
**      05-Nov-1992 (mikem)
**          su4_us5 port.  Changed direct calls to CL_NFILE to calls to new
**          function iiCL_get_fd_table_size().
**	7-Jul-1993 (daveb)
**	    include <clconfig.h> before <pc.h>, so it can correctly
**	    set stuff so we get the right extern for getpid() from
**	    system header files, or wherever.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-nov-2010 (stephenb)
**	    Correctlty define static functions for prototyping.
**	1-Dec-2010 (kschendel)
**	    Modernize declaration style for Sun compiler.
*/

static i4	pipe_open(PIPE *);
static STATUS	swap_pipes(PIPE *, PIPE *, char **);



STATUS
PCsspawn(i4 argc, char **argv, bool wait,
	PIPE *c_stdin, PIPE *c_stdout, PID *pid)
{
    PIPE	pipe1[2];
    PIPE	pipe2[2];
    
    STATUS	PCwait();
    char	*index;
    
# define	DUP_ERROR	-1
# define	READ_SIDE	 0
# define	WRITE_SIDE	 1
# define	CHILD		 0

    /* Flush the output buffers to make sure all child output
    ** follows the parent output.  The streams may be buffered
    ** if, for example, they were redirected to a file.
    */
    SIflush(stdout);
    SIflush(stderr);
    TRflush();

    if ((argc < 1) || (argv[0] == NULL))
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
    
    else if (access(argv[0],01) == BAD_ACCESS)
    {
	switch (errno)
	{
	case EACCES:
	      /* error occurred as path was inaccessable */
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
    else if (pipe_open(pipe1) || pipe_open(pipe2))
    {
	    PCstatus = PC_SP_PIPE;
    }
    else if ( (*pid = (PID)PCfork( &PCstatus )) > 0 )
    {
	/*
	**	PCstatus could have been changed in the
	**	child if we actually executed a vfork,
	**		as PCstatus is a global.
	*/

	if (PCstatus == OK)
	{
	    /*
	    **	close the sides of the pipes the child uses.
	    **
	    **	set user passed in file descriptor
	    **	c_std[in, out] to opposite end of pipe
	    **	the child process will use for
	    **	it's std[in, out].
	    */
	    
	    if (close(pipe1[READ_SIDE]) || close(pipe2[WRITE_SIDE]))
	    {
		PCstatus = PC_SP_CLOSE;
	    }
	    else
	    {
		*c_stdin  = pipe1[WRITE_SIDE];
		*c_stdout = pipe2[READ_SIDE];
	    }
    
	    if (wait)
		PCstatus = PCwait(*pid);
	}
    }
    else if ( *pid == 0 )				/* child */
    {
	    /*
	    **	close the sides of the pipes the parent uses.
	    **
	    **	associate "read" pipe with child's stdin and "write"
	    **	pipe with child's stdout.
	    **
	    **	NOTE: dup() associates it's argument with the lowest
	    **		available file descriptor, hence by preceding
	    **		the dup of pipe1[READ_SIDE] with a close of
	    **		STDIN the first fd in the file table is
	    **		identical (similiarly with pipe2[WRITE_SIDE]
	    **		and STDOUT).
	    */
    
	    if (close(pipe1[WRITE_SIDE]) || close(pipe2[READ_SIDE]))
	    {
		PCstatus = PC_SP_CLOSE;
	    }
	    else if ((PCstatus = swap_pipes(pipe1, pipe2, argv)) == OK)
	    {
		argv[argc] = '\0';    /* insure correct argv */
		execvp(argv[0], argv);
    
		/* should never reach here...	*/
    
		PCno_exec(argv[0]); /*  find out why execv[p] failed */
	    }
	    (void)exit( FAIL );
	    /*NOTREACHED*/
    }
    
    return ( PCstatus );
}

/*
** PIPE_OPEN - open pipe, closing di files if necessary
*/

static	i4
pipe_open(pipe_struct)
PIPE	pipe_struct[];
{
    i4	ret_val = OK;

    if (pipe(pipe_struct) < 0)
                    ret_val = FAIL;

    return(ret_val);
}

/*	SWAP_PIPES -	Put the given pipes in a known location
**
**	Parameters -
**		pipe1, pipe2 -	The pipes
**		argv -		The argument vector of what is being 
**				started up.
**
**	Returns -
**		status
**
**	History:
**
**      05-Nov-1992 (mikem)
**          su4_us5 port.  Changed direct calls to CL_NFILE to calls to new
**          function iiCL_get_fd_table_size().
**	15-Jun-2004 (schka24)
**	    Safe env var handling.
*/
static STATUS
swap_pipes(pipe1, pipe2, argv)
PIPE	*pipe1, *pipe2;
char	*argv[];
{
    i4	fd, first_free_file;
    PIPE	dup_ret;
    char	*cp;
    char	debug[20];
    char	wordbuf[30];
    char	*word = wordbuf;
    i4	ws = 1;
    char	tty[30];
    i4	tty_fnum;
    extern  i4  in_dbestart;

    /* Lets look at the env var II_BE_DEBUG and figure out what debugger
    ** we might be starting up.
    */

    *debug = '\0';
    if(in_dbestart)
	    NMgtAt("II_DBE_DEBUG", &cp);
    else
    NMgtAt("II_BE_DEBUG", &cp);

    if (cp != 0 && *cp != 0)
    {
	STlcopy(cp, debug, sizeof(debug)-1);	/* Trim... */
	STgetwords(debug, &ws, &word);
    }

    /* if we are using a debugger */
    if (*debug && STcompare(argv[0], debug) == 0)
    {
	/* we are going to leave STDIN and STDOUT open for the debugger,
	** so we'll put the pipes in a different place for the backend
	** to find. The backend will know to look their because it will
	** look at the env var II_BE_DEBUG.
	*/
	if (pipe1[READ_SIDE] != STDIN2)
	{
	    dup_ret = dup(STDIN2);

	    if (pipe2[WRITE_SIDE] == STDIN2)
		    pipe2[WRITE_SIDE] = dup_ret;

	    if (dup2(pipe1[READ_SIDE], STDIN2) == DUP_ERROR ||
		    close(pipe1[READ_SIDE]))
	    {
		    return (PC_SP_DUP);
	    }
	}

	if (pipe2[WRITE_SIDE] != STDOUT2)
	{
	    if (dup2(pipe2[WRITE_SIDE], STDOUT2) == DUP_ERROR ||
		    close(pipe2[WRITE_SIDE]))
	    {
		    return (PC_SP_DUP);
	    }
	}

	/* lets find out if they want to direct the input and output
	** to/from the debugger to another terminal. If so, we will
	** open the other terminal and swap it with STDIN and STDOUT.
	*/
	NMgtAt("II_BE_DEBUG_IO", &cp);

	if (cp != 0 && *cp != 0)
	{
	    STlpolycat(2, sizeof(tty)-1, "/dev/", cp, &tty[0]);

	    if ((tty_fnum = open(tty, 2)) >= 0)
	    {
		close(STDIN);
		dup2(tty_fnum, STDIN);
		close(STDOUT);
		dup2(STDIN, STDOUT);
		close(tty_fnum);
	    }
	    else
	    {
		SIfprintf(stderr,
"PCsspawn: II_BE_DEBUG_IO = %s, but %s couldn't be opened. err = %d\n",
			  cp, tty, errno);
		SIfprintf(stderr,
"\tThe current STDIN and STDOUT will be used for %s instead.\n",
			  debug);
	    }
	}

	SIprintf("Your debugger is: %s\n", debug);
	first_free_file = 5;
    }
    else
    {
	if ((close(STDIN))				||
		 (dup(pipe1[READ_SIDE]) == DUP_ERROR)	||
		 (close(pipe1[READ_SIDE])))
	{
		return (PC_SP_DUP);
	}
	else if ((close(STDOUT))			||
		 (dup(pipe2[WRITE_SIDE]) == DUP_ERROR)	||
		 (close(pipe2[WRITE_SIDE])))
	{
		return (PC_SP_DUP);
	}

	first_free_file = 3;
    }

    /*
    ** Close all file descriptors other than STDIN,
    ** STDOUT and STDERR.
    **
    ** NOTE: we are not checking the return status
    ** of close as we're indiscriminately closing
    ** all possibly open files.
    */

    for (fd = first_free_file; fd < iiCL_get_fd_table_size(); fd++)
    {
	    close(fd);
    }

    return(OK);
}


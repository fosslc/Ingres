/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PCfspawn.c
 *
 *	Function:
 *		PCfspawn
 *
 *	Arguments:
 *		i4	argc;
 *		char	**argv;
 *		bool	wait;			* wait/don't wait for child *
 *		FILE	*c_stdin;		* descriptor for slave's stdin *
 *		FILE	*c_stdout;		* descriptor for slave's stdout *
 *		PID	*pid;			* process id of slave, returned to caller *
 *
 *	Result:
 *		Spawn a slave process.
 *
 *		Create a new process with its standard input and output linked to FILE pointers
 *			supplied by the caller.
 *
 *		Returns:
 *			OK		-- success
 *			FAIL		-- general failure
 *			PC_SP_CALL	-- arguments incorrect
 *			PC_SP_EXEC	-- cannot execute command specified, bad magic number
 *			PC_SP_MEM	-- system temporarily out of core or process requires too
 *						many segmentation registers.
 *			PC_SP_OWNER	-- must be owner or super-user to execute this command
 *			PC_SP_PATH	-- part of specified path didn't exist
 *			PC_SP_PERM	-- didn't have permission to execute
 *			PC_SP_PROC	-- system process table is temporarily full
 *			PC_SP_SUCH	-- command doesn't exist
 *			PC_SP_CLOSE	-- had trouble closing unused sides of pipe
 *			PC_SP_DUP	-- had trouble associating, dup()'ing, the pipes
 *						with the child's std[in, out]
 *			PC_SP_PIPE	-- couldn't establish communication pipes
 *			PC_SP_OPEN	-- had trouble associating child's std[in, out] with passed in FILE pointers
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
**	25-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add include for <clconfig.h>.
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	15-nov-2010 (stephenb)
**	    include ex.h for prototyping.
 */


/* static char	*Sccsid = "@(#)PCfspawn.c	3.1  9/30/83"; */


# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<pc.h>
# include	<ex.h>
# include	"pclocal.h"
# include	<PCerr.h>
# include	<si.h>

#include <stdio.h>

STATUS
PCfspawn(i4 argc, char **argv, bool wait, FILE **c_stdin, FILE **c_stdout, PID *pid)
{

	PIPE		t_stdin;
	PIPE		t_stdout;


	/*
		call PCsspawn() to spawn slave and set file descriptors t_stdin and t_stdout
			to slave's stdin and stdout.

		call fdopen to associate the passed in FILE pointers with t_stdin and t_stdout.
	*/

	if (!(PCstatus = PCsspawn(argc, argv, wait, &t_stdin, &t_stdout, pid)))
	{
		*c_stdin  = fdopen(t_stdin,  "a");
		*c_stdout = fdopen(t_stdout, "r");

		if ((*c_stdin == NULL) || (*c_stdout == NULL))
			PCstatus = PC_SP_OPEN;
	}

	return(PCstatus);
}

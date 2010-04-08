/*
** Copyright (c) 1983, 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	<st.h>
# include       <me.h>
# include	"pclocal.h"
# include	<PCerr.h>

/**
** Name:	pccmdline.c
**
**	Function:
**		PCcmdline
**
**	Arguments:
**		LOCATION	*interpreter;
**		char		*cmdline;
**		bool		wait;
**		LOCATION	*err_log;
**		CL_ERR_DESC     *err_code;
**
**	Result:
**		Execute command line.
**
**		Invoke the specified interpreter to execute the command line.
**			A NULL argument means use the default.
**
**		Returns:
**			OK		-- success
**			FAIL		-- general failure
**			PC_CM_CALL	-- arguments incorrect
**			PC_CM_EXEC	-- cannot execute command specified, 
**						bad magic number
**			PC_CM_MEM	-- system temporarily out of core or 
**						process requires too
**						many segmentation registers.
**			PC_CM_OWNER	-- must be owner or super-user to
**						execute this command
**			PC_CM_PATH	-- part of specified path didn't exist
**			PC_CM_PERM	-- didn't have permission to execute
**			PC_CM_PROC	-- system process table is temporarily 
**						full
**			PC_CM_REOPEN	-- couldn't associate argument 
**						[in, out]_name with std[in, out]
**			PC_CM_SUCH	-- command doesn't exist
**			PC_CM_BAD	-- child didn't execute, e.g bad 
**						parameters, bad or inacessible
**						date, etc.
**			PC_CM_TERM	-- child we were waiting for returned 
**						bad termination status
** History:
**	Revision 6.4  90/03/07  sylviap
**	05-jan-90 (sylviap)
**		Added two more parameters to allow spawning a detached process.
**	07-mar-90 (sylviap)
**		Changed err_log to a LOCATION. Changed wait to be a nat.
**		Added (CL_ERR_DESC*) err_code as a parameter.
**
**	19-mar-90 (Mike S)
**		Add support for appended output files and redirected
**		stderr.
**
**	25-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add include for <clconfig.h>.
**
**	Revision 6.3  90/03  wong
**	Add support for using $SHELL command interpreter if command-line is
**	empty, which means starts up a shell interactively.  JupBug #10016.
**
**	Revision 3.1  85/07/12  21:14:45  greg
**	Don't use ING_SHELL--AT&T request
**		
**	Revision 3.0  85/05/29  16:27:56  wong
**	Changed to ignore exit status of interpreter
**	when called interactively.
**		
**	03/83	--	(gb)	written
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	 8-sep-95 (allst01/smiba01)
**	    Do not write to err_code if it is NULL !
**	    This breaks qasetuser amongst others and
**	    causes havoc in sep tests.
**	06-jul-98 (kosma01)
**	    The picky SGI C compiler requires a cast from
**	    const char [] to char *;
**      20-may-1997 (canor01)
**          If II_PCCMDPROC is set to a process name, use it as a command
**          process slave.  PCcmdline() will pass all commands to it.
**	04-jun-1997 (canor01)
**	    Only use the command process slave if the shared memory has
**	    already been created (by server or by slave).
**	06-oct-1998 (somsa01)
**	    In PCdocmdline(), instead of returning PC_FAIL, return the error
**	    received from the child.
**	27-oct-1997 (canor01)
**	    Pass CL error back up to caller.  Translate PCspawn/PCwait
**	    errors into the PCcmdline equivalent.
**	27-jan-1998 (canor01)
**	    Use of the command process slave is specific to Jasmine.
**	11-may-1999 (cucjo01)
**	    Do not use CLSETERR if err_code is NULL
**	    This breaks qasetuser amongst others and
**	    causes havoc in sep tests.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Mar-2002 (hanje04)
**	    BUG 107298
**	    In PCdoccmdline(), when invoking /bin/sh on Linux use -p, to stop 
**	    problems with bash v2.x squashing the SUID bit for non ingres users.
**	    Also increased size of *argv[] to handle extra parameter.
**	16-May-2002 (hanje04)
**	    Added other Linux's to list of platforms using -p to invoke bash.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	15-Jun-2004 (schka24)
**	    Watch out for env variable buffer overrun.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	07-Jun-2005 (hanje04)
**	    Mac OSX also uses bash so make sure we call it with -p
**	30-May-2007 (hanje04)
**	    BUG 119214
**	    Make def_interpreter[] /bin/bash on Linux to prevent problems
**	    on Ubuntu with /bin/sh being linked to /bin/dash.
**	    Also use def_interpreter[] instead of "/bin/sh" explicitly
**	    when determining if we need to add a -p.
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	17-Nov-2008 (smeke01) b121233
**	    Remove redundant adb feature.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/


# ifdef LNX
static const char	def_interpreter[] =	"/bin/bash";
# else
static const char	def_interpreter[] =	"/bin/sh";
# endif

STATUS
PCcmdline ( interpreter, cmdline, wait, err_log, err_code )
LOCATION	*interpreter;
char		*cmdline;
i4		wait;
LOCATION	*err_log;
CL_ERR_DESC     *err_code;
{
    STATUS PCdocmdline();

    /* 
    ** Someday the added functionality of appended output files may be part of
    ** PCcmdline.
    */
    return PCdocmdline(interpreter, cmdline, wait, FALSE, err_log, 
			err_code);
}

STATUS
PCdocmdline(interpreter, cmdline, wait, append, err_log, err_code )
LOCATION        *interpreter;
char            *cmdline;
i4              wait;
i4		append;		/* non-zero to append, rather than recreate,
				** output file */
LOCATION        *err_log;
CL_ERR_DESC     *err_code;
{
    register char	**ap;
    i4			argc;
    char		*argv[5];
    char		interp[MAX_LOC+1];
    PID			pid;
    STATUS		retval;
    char		*cp;

    static char	*shell = NULL;

    if(err_code)
	CL_CLEAR_ERR( err_code );

    {
	char	*u_interp;

	/*
	**	if interpreter
	**		set u_interp to LO buffer
	**	else if no command line and $SHELL
	**		set u_interp to $SHELL
	**	else
	**		use default
	*/

	if ( interpreter != (LOCATION *)NULL )
	{
	    LOtos(interpreter, &u_interp);
	}
	else if ( ( cmdline == (char *)NULL || *cmdline == EOS )
		&& ( ( shell != (char *)NULL && *shell != EOS )
			|| ( NMgtAt("SHELL", &shell),
				( shell != (char *)NULL && *shell != EOS ) )
		) )
	{
	    u_interp = shell;
	}
	else
	{
	    u_interp = (char *)def_interpreter;
	}
	(VOID)STlcopy(u_interp, interp, sizeof(interp)-1);
    }
		
    /*
    **	set up argc, argv for call to PCspawn()
    */

    ap    = argv;
    *ap++ = interp;

    if ( cmdline == (char *)NULL || *cmdline == EOS )
	argc	= 1;
    else
    {
#if defined(LNX) || defined(OSX)
	if ( STcompare(interp, def_interpreter) == 0 )
	{	
	    *ap++ = "-p"; /*If we're using the default shell it must use -p  */
	    *ap++ = "-c";
	    argc  = 4;
	}
	else
	{
	    *ap++ = "-c";
	    argc = 3;
	}
#else
	*ap++ = "-c";
	argc  = 3;
#endif
	*ap++ = cmdline;
    }

    *ap   = NULL;

    /*
    ** Ignore exit status from shell if no command line was given
    ** (argc == 1).  (That is, an interactive shell was executed.)
    */

    retval =
	PCdospawn(argc, argv, (bool)(wait != PC_NO_WAIT), (LOCATION*)NULL, 
		err_log, append, TRUE, &pid);

    if ( retval != OK && err_code )
        SETCLERR(err_code, 0, 0);

    /*
    ** Replace PCspawn/PCwait errors with equivalent PCcmdline ones.
    */
    switch ( retval )
    {
	case PC_SP_MEM:  	retval = PC_CM_MEM; break;
	case PC_SP_EXEC:	retval = PC_CM_EXEC; break;
	case PC_SP_CALL:	retval = PC_CM_CALL; break;
	case PC_SP_OWNER:	retval = PC_CM_OWNER; break;
	case PC_SP_PATH:	retval = PC_CM_PATH; break;
	case PC_SP_PERM:	retval = PC_CM_PERM; break;
	case PC_SP_PROC:	retval = PC_CM_PROC; break;
	case PC_SP_SUCH:	retval = PC_CM_SUCH; break;
	case PC_SP_REOPEN:	retval = PC_CM_REOPEN; break;
	case PC_WT_TERM:	retval = PC_CM_TERM; break;
    }

    return (retval == OK || (retval == PC_WT_BAD && argc == 1)) ? OK : retval;
}

# include	<compat.h>  
# include	<gl.h>
# include	<pc.h>  
# include	"pclocal.h"
# include	"pcerr.h"
# include 	<starlet.h>

# define	EXenable()
# define	EXdisable()

/*
 *	Copyright (c) 1983, 2000 Ingres Corporation
 *
 *	Name:
 *		PCwait.c
 *
 *	Function:
 *		PCwait
 *
 *	Arguments:
 *		PID	*proc;
 *
 *	Result:
 *		wait for specified subprocess to terminate.
 *
 *		Returns: child's exit status
 *				OK		-- success
 *				PC_WT_BAD	-- child didn't execute, e.g bad parameters, bad or inacessible date, etc.
 *				PC_WT_NONE	-- no children to wait for
 *				PC_WT_TERM	-- child we were waiting for returned bad termination status
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
**      16-jul-93 (ed)
**	    added gl.h
**      18-jun-98 (kinte01)
**          return status PCstatus
**      01-oct-98 (chash01)
**          we should wait on proc (or pid) instead of eventflg for that
**          if multiple subprocesses are spawned, they all share the same
**          event flag.  Any one subprocess terminates will set the eventflg
**          permanently, thus gives the false sense that all subprocesses
**          have terminated.  What we should do is to wiat for a period
**          (1 sec) then call PCis_alive to verify the termination of the
**          subprocess.  Also change argument from *proc to proc.  My search
**          indicates that PCwait() arg is a pid value, not an address.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	27-jul-2004 (abbjo03)
**	    Rename pc.h to pclocal.h to avoid conflict with CL hdr!hdr file.
*/


STATUS
PCwait(proc)
PID	proc;
{

	/*
		disable the signals interrupt and quit and
			save their old actions.
	*/

	EXdisable();

# if 0
	PCstatus = sys$waitfr(PCeventflg);
# endif

	for (;;)
	{
	    PCsleep(1000);
	    if ( !(PCstatus = PCis_alive( proc )) )
	    break;
	}
	/*
		reset interrupt and quit signals
	*/

	EXenable();

        return (PCstatus);

}

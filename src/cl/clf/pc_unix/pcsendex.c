/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PCsendex.c
 *
 *	Function:
 *		PCsendex
 *
 *	Arguments:
 *		EX		except;
 *		PID		process;
 *
 *	Result:
 *		Send specified process the specified exception.
 *
 *		Returns:
 *			OK		-- success
 *			PC_SD_CALL	-- invalid exception value
 *			PC_SD_NONE	-- No process corresponding to the pid (process)
 *						can be found.
 *			PC_SD_PERM	-- Can't signal specified process.  Effective uid
 *						of caller isn't "root" and doesn't match real
 *						uid of receiving process.
 *			FAIL		-- Undetermined reason for failure.
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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	15-nov-2010 (stephenb)
**	    move include of ex.h above pclocal.h to pull in required defines
**	    for prototypes now in pclocal.h.
 */
 
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<pc.h>
# include	<ex.h>
# include	"pclocal.h"
# include	<PCerr.h>
# include	<errno.h>

STATUS
PCsendex(except, process)
EX		except;
PID		process;
{
	i4	kill_ret;

# define	KILL_FAIL	-1


	PCstatus = OK;

	kill_ret = kill(process, except);

	if (kill_ret == KILL_FAIL)
	{
		switch (errno)
		{
			case EINVAL :
				PCstatus = PC_SD_CALL;
				break;

			case ESRCH:
				PCstatus = PC_SD_NONE;
				break;

			case EPERM:
				PCstatus = PC_SD_PERM;
				break;

			default:
				PCstatus = FAIL;
				break;
		}
	}

	return(PCstatus);
}

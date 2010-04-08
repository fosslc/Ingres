/*
 *	Copyright (c) 1983, 2000 Ingres Corporation
 *
 *	Name:
 *		PCendpipe.c
 *
 *	Function:
 *		PCendpipe
 *
 *	Arguments:
 *		PIPE	pipe;
 *
 *	Result:
 *		End communication on a pipe.
 *
 *		No more communication will take place on this pipe,
 *			and it is deallocated.
 *
 *		Returns:
 *			OK		-- success
 *			BAD_PIPE	-- no such PIPE exists
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *		09/83 (wood) -- adapted to VMS world.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
 *
 */


# include	<compat.h>  
# include	<gl.h>
# include	<pc.h>
# include	<starlet.h>  
# include	"pcerr.h"


/* taken from u$dev:[back.newback.src.gutil20]mailbox.c */

PCendpipe(mbx)
int	mbx;
{
	register i4	ret;

	ret = sys$delmbx(mbx);
	if ((ret & 1) == 0)
		return (ret);
	ret = sys$dassgn(mbx);
	if ((ret & 1) == 0)
		return (ret);
	return (0);
}


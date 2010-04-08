/*
**Copyright (c) 2004 Ingres Corporation
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
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
 */


# include	<compat.h>
# include	<gl.h>
# include	<pc.h>
# include	<PCerr.h>

STATUS
PCendpipe(pipe)
PIPE	pipe;
{
	return((close(pipe) == -1) ? PC_END_PIPE : OK);
}

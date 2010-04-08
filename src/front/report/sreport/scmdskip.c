/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)scmdskip.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <cm.h> 

/*
**   S_CMD_SKIP - skip the input until a command is found.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		11/22/83 (gac)	written.
**		24-mar-1987 (yamamoto)
**			Modified for double byte characters.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
s_cmd_skip()
{
	i4	type;
	i4     rtn_char;               /* dummy variable for sgskip */

	type = s_g_skip(FALSE, &rtn_char);
	while (!((type == TK_PERIOD && CMalpha(Tokchar+CMbytecnt(Tokchar))) ||
		 type == TK_ENDFILE))
	{
		type = s_g_skip(TRUE, &rtn_char);
	}
}

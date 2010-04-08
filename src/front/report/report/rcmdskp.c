/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rcmdskp.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_CMD_SKP - skip RTEXT until next command is encountered.
**
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
**		12/19/83 (gac)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_cmd_skp()
{

	do
	{
		Tokchar += STlength(Tokchar);
	} while (r_g_eskip() != TK_ENDSTRING);

	return;
}

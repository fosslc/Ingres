/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rparclip.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
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
**   R_PAR_CLIP - clip a parameter name to less than maximum length
**		for a parameter and lowercase it.
**
**	Parameters:
**		name	parameter name to clip.
**
**	Returns:
**		none.
**
**	Called by:
**		r_par_get.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		7/1/81 (ps)	written.
**		12/23/83 (gac)	rewrote for expressions.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_par_clip(name)
register char	*name;
{
	/* start of routine */

	if (STlength(name) > MAXPNAME)
	{
		*(name+MAXPNAME) = '\0';
	}

	CVlower(name);

}

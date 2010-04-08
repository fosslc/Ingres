/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_GT_WCODE - return A_GROUP code if the name given is "within_name".
**
**	Parameters:
**		name  - name to check. May contain upper or lower
**			case characters.
**
**	Returns:
**		Code of the constant, if it exists.  Returns -1 if
**		the command is not in the list.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		11/29/83 (gac)	written.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4	
r_gt_wcode(name)
char	*name;

{
	CVlower(name);

	if (STequal(name,ERx("w_column")) ||
	    STequal(name,ERx("within_column")) ||
	    STequal(name,ERx("wi_column")))
	{
		return(A_GROUP);
	}

	return(A_ERROR);
}

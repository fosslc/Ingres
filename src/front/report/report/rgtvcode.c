/*
** Copyright (c) 2004 Ingres Corporation
*/


/* static char	Sccsid[] = "@(#)rgtvcode.c	30.1	11/14/84"; */

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
**   R_GT_VCODE - return the program variable code for the name given.
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
**		11/29/83 (gac)	wriiten.
**		17-may-90 (sylviap)
**			Added page_width as a user variable. (#958)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4	
r_gt_vcode(name)
char	*name;
{


	if (STbcompare(name, 0, V_PAGENUM, 0, TRUE) == 0)
	{
		return(VC_PAGENUM);
	}
	else if (STbcompare(name, 0, V_LINENUM, 0, TRUE) == 0)
	{
		return(VC_LINENUM);
	}
	else if (STbcompare(name, 0, V_POSNUM, 0, TRUE) == 0)
	{
		return(VC_POSNUM);
	}
	else if (STbcompare(name, 0, V_LEFTMAR, 0, TRUE) == 0)
	{
		return(VC_LEFTMAR);
	}
	else if (STbcompare(name, 0, V_RIGHTMAR, 0, TRUE) == 0)
	{
		return(VC_RIGHTMAR);
	}
	else if (STbcompare(name, 0,  V_PAGELEN, 0, TRUE) == 0)
	{
		return(VC_PAGELEN);
	}
	else if (STbcompare(name, 0,  V_PAGEWDTH, 0, TRUE) == 0)
	{
		return(VC_PAGEWDTH);
	}

	return(VC_NONE);
}

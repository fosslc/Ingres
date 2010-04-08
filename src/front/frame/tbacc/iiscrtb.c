/*
**	iiscrtb.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 

/**
** Name:	iiscrtb.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIscr_tb()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	12/23/87 (dkh) - Performance changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIscr_tb	-	Scroll table field
**
** Description:
**	Scroll the passed tablefield in the direction given.  If
**	a pending EQUEL 'in' list is waiting, update the necessary
**	vars.
**
**	Using FDcopytab, execute a sub-scroll on the display and
**	update current row numbers, etc.  If no 'in' list is
**	coming in then this routine does nothing.
**
** Inputs:
**	<param name>	<input value description>
**	tb		Ptr to tablefield descriptor
**	direc		Direction to scroll in
**	tofill		Flag - EQUEL 'IN' list is waiting
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**		04-mar-1983 	- written (ncg)
**		30-apr-1984	- Improved interface to FD routines (ncg)
**
*/

i4
IIscr_tb(TBSTRUCT *tb, i2 direc, i4 tofill)
{
	if (tb->tb_display == 0)
	{
		/* TRUE, because we do not want err mssgs on bare tables */
		return (TRUE);
	}
	if (tofill)
	{
		switch (direc)
		{
		  case sclUP:
			/* scroll up display of table, clear last row */
			FDcopytab(tb->tb_fld,
				(i4) fdtfTOP, (i4) (tb->tb_display - 1));
			FDclrrow(tb->tb_fld, tb->tb_display - 1);
			tb->tb_rnum = tb->tb_display; /* nxt row for IItsetcol*/
			break;
	 
		  case sclDOWN:
			/* scroll down display rep of table, clear row # 1 */
			FDcopytab(tb->tb_fld,
				(i4) (tb->tb_display - 1), (i4) fdtfTOP);
			FDclrrow(tb->tb_fld, fdtfTOP);
			tb->tb_rnum = 1;	/* next row for IItsetcol() */
			break;

		  default:
			return (FALSE);
		}
	}
	/* else, do nothing */
	return (TRUE);
}

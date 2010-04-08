/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rxtab.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_X_TAB - move the current line position forward or backward in the
**	line, or to a specified column position.
**
**	Parameters:
**		position position to tab to, or column number.
**		type	type of tabbing--absolute,relative,default,or column.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update info in the St_cline flag.
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		208, 209.
**
**	Trace Flags:
**		5.0, 5.3.
**
**	History:
**		4/8/81 (ps) - written.
**		11/19/83 (gac)	added position_number.
**		23-mar-90 (sylviap)
**			If .TAB goes beyond En_lmax, ignore command. #724
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
r_x_tab(position,type)
register i4	position;
register i2	type;
{
	/* internal declarations */

	register LN	*ln;			/* fast LN ptr */
	i4	 	curpos;

	/* start of routine */


	ln = St_cline;
	curpos = ln->ln_curpos;

	switch(type)
	{
		case(B_ABSOLUTE):
			curpos = position;
			break;

		case(B_RELATIVE):
			curpos += position;
			break;

		case(B_DEFAULT):
			/* default is start of line */
			curpos = St_left_margin;
			break;

		case(B_COLUMN):
			curpos = (r_gt_att(position))->att_position;
			break;

	}

	if (curpos < 0)
	{
		r_error(208, NONFATAL, Cact_tname, Cact_attribute, NULL);
		ln->ln_curpos = 0;
	}
	else
	{
		if (curpos > En_lmax)
			/* 
			** If too large, don't reset ln->ln_curpos.  Ignore
			** the .TAB command
			*/
			r_error(209, NONFATAL, Cact_tname, Cact_attribute, NULL);
		else
			ln->ln_curpos = curpos;
	}

	St_pos_number = ln->ln_curpos;


	return;
}

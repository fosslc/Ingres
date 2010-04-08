/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_M_PBOT - set up the page bottom in a default report.
**	This will contain the current page number, centered.
**
**	Parameters:
**		labove	number of lines above the footing.
**		lbelow	number of lines below the footing.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_m_columns, r_m_wrap.
**
**	Trace Flags:
**		50, 59.
**
**	History:
**		3/30/82 (ps)	written.
**		9-nov-1992 (rdrane)
**			Force generation of single quoted string constants
**			for 6.5 delimited identifier support.
**		22-dec-1993 (mgw) Bug #58122
**			Make labove and lbelow nat's, not i2's to prevent
**			stack overflow when called from r_m_block(). This
**			routine is only ever called with integer constant
**			args so i4  is appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2005 (lakvi01)
**          Properly prototype function.
*/


VOID
r_m_pbot(labove,lbelow)
i4	labove;
i4	lbelow;
{
	/* internal declarations */

	char	ibuf[20];		/* hold result of itoa */

	/* start of routine */



	r_m_action(ERx("begin"), ERx("rbfpbot"), NULL);
	if (labove > 0)
	{	/* lines above the footing */
		CVna(labove,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}

	r_m_action(ERx("center"), NULL);
	r_m_action(ERx("print"), ERx("\'-\', page_number(f4), \' -\'"), NULL);
	r_m_action(ERx("endprint"), NULL);

	if (lbelow > 0)
	{	/* lines below the footing */
		CVna(lbelow,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}

	r_m_action(ERx("end"), ERx("rbfpbot"), NULL);

	return;
}

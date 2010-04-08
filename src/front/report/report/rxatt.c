/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxatt.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_X_ATT - evaluate the data value for an attribute.
**	This checks to see if it is being referenced in the HEADER or FOOTER
**	for an attribute. If in header, return the CURRENT value, and
**	if in footer, return the previous.  In page header and page footer, 
**	check to see what section of the stack is in use, and return
**	current or previous according to that.
**
**	Parameters:
**		ordinal - attribute ordinal of data att to print.
**		val	- pointer to the value of the attribute.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_e_item.
**
**	Trace Flags:
**		none.
**
**	History:
**		4/10/81 (ps) - written.
**		5/11/83 (nl) - Put in changes made since code was
**			       ported to UNIX.  Pull out parameter telling
**			       current or previous to figute it out 
**			       according to the current environment (to fix
**			       bug 937).
**		1/6/84 (gac)	changed for expressions.
**		2/1/84 (gac)	added date type.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
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
r_x_att(ordinal,val)
ATTRIB		ordinal;
DB_DATA_VALUE	*val;
{
	/* internal declarations */

	ATT	*attribute;	/* fast ptr to ATT structure */
	i4	which;		/* either D_CURRENT or D_PREVIOUS to
				** indicate which data value to return */
	DB_DATA_VALUE	*attval;

	/* start of routine */

	attribute = r_gt_att(ordinal);

	/* Figure out whether to use the current or previous data
	** value.  If in header, use current.  If footer, use previous.
	** If in page header or footer, check to status of the
	** section on the page stack. */

	which = STequal(Cact_tname,NAM_FOOTER)  ? D_PREVIOUS : D_CURRENT;
	if (STequal(Cact_attribute,NAM_PAGE) )
	{
		/* in page break, use current break info for which */
		if (STequal(Cact_tname,NAM_FOOTER) )
		{	
			/* in page footer.  Only switch if data written */
			if (((STequal(St_h_tname,NAM_HEADER) ) &&
					St_h_written) || 
				(STequal(St_h_tname,NAM_DETAIL) ))
			{
				/* writing the next header */
				which = D_CURRENT;
			}
		}
		else
		{	
			/* in page header */
			if (STequal(St_h_tname,NAM_FOOTER) )
			{
				/* in a footer */
				which = D_PREVIOUS;
			}
		}
	}

	if (which == D_CURRENT)
	{
	    attval = &(attribute->att_value);
	}
	else
	{
	    attval = &(attribute->att_prev_val);
	}

	MEcopy((PTR)attval, (u_i2)sizeof(DB_DATA_VALUE), (PTR)val);
}

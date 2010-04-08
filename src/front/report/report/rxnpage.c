/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
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
**   R_X_NPAGE - go to the top of the next page, and optionally
**	give it a new page number.  This simply calls r_x_newline
**	to write out the lines.
**
**	If this is called before anything is written to the report,
**	don't write out the footer for the page.  
**
**	Parameters:
**		pagenum	new page number.
**		type	type of change to page number--absolute,relative,default.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update St_nxt_page.
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		210.
**
**	Trace Flags:
**		5.0, 5.7.
**
**	History:
**		4/20/81 (ps) - written.
**		6/12/84	(gac)	bug 2328 fixed -- ff instead of newlines if .FF
**						  and no page footer.
**		2/21/89 (kathryn) - bug 3902 --  Do not print .FF here this
**			is now done in r_do_page() to ensure formfeed is
**			not printed until after the footer.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_npage(pagenum,type)
register i2	pagenum;
register i2	type;
{
	/* start of routine */



	switch(type)
	{
		case(B_ABSOLUTE):
			St_nxt_page = pagenum;
			break;

		case(B_RELATIVE):
			St_nxt_page = St_p_number + pagenum;
			break;

		case(B_DEFAULT):
			if (St_no_write)
			{
				St_nxt_page = St_p_number;
			}
			else
			{
				St_nxt_page = St_p_number + 1;
			}
	}

	if (St_nxt_page < 0)
	{
		r_error(210, NONFATAL, Cact_tname, Cact_attribute, NULL);
	}


	/* now call r_x_newline or r_do_page to write out the new page */

	if (St_no_write)
	{
		r_do_page();
	}
	else if (St_p_length > 0 && St_ff_on && !St_to_term && 
		 !St_in_pbreak && Ptr_pag_brk->brk_footer == NULL &&
		 !(St_c_lntop->ln_started &&
		   St_l_number+(St_c_lntop->ln_has_ul && St_ulchar != '_') >=
								St_p_length))
	{
		if (St_c_lntop->ln_started)
		{
			r_x_newline(1);
		}
		r_do_page();
	}
	else
	{
		r_x_newline(St_p_length); /* assured of getting a new page */
	}

	return;
}

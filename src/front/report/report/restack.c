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
# include	<errw.h>
# include	<er.h>
# include	<ug.h>

/*
**   R_PUSH_ENV and R_POP_ENV - routines to push and pop onto the
**	environment stack, which holds information about the current
**	margins, and line buffers being used.
*/



/*
**   R_PUSH_ENV - push the current environment onto the ESTK.
**
**	Parameters:
**		none.
**
**	Returns:
**		address of pushed ESTK.
**
**	Called by:
**		r_do_page, r_x_adjust, r_stp_adjust.
**
**	Side Effects:
**		Updates St_c_estk.  May initialize Ptr_estk_top.
**
**	Trace Flags:
**		2.0, 2.6.
**
**	Error Messages:
**		none.
**
**	History:
**		7/20/81 (ps)	written.
**
**		5/10/83 (nl) - put in changes since code was ported
**			       to UNIX.
**			       Get rid of tname and attribute from
**			       stack to fix bug 937.
**		1/8/85 (rlm) - tagged memory allocation.
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

ESTK	*
r_push_env()
{
	/* internal declarations */

	register ESTK	*estk;		/* fast ptr to an ESTK */
	ESTK		*fetch;		/* need address for alloc */

	/* start of routine */


	if (Ptr_estk_top == NULL)
	{	/* start the stack */
                if ((fetch = (ESTK *)FEreqmem ((u_i4)Rst4_tag,
			(u_i4)(sizeof(ESTK)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_push_env"));
		}

		estk = fetch;
		Ptr_estk_top = estk;
	}
	else if (St_c_estk == NULL)
	{	/* nothing in stack now */
		estk = Ptr_estk_top;
	}
	else if (St_c_estk->estk_below == NULL)
	{	/* must add to stack */
                if ((fetch = (ESTK *)FEreqmem ((u_i4)Rst4_tag,
			(u_i4)(sizeof(ESTK)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_push_env"));
		}
		estk = fetch;
		estk->estk_above = St_c_estk;
		St_c_estk->estk_below = estk;
	}
	else
	{	/* already have one, use it */
		estk = St_c_estk->estk_below;
	}

	/* now add the current info to the ESTK structure */

	estk->estk_lm = St_left_margin;
	estk->estk_rm = St_right_margin;
	estk->estk_ul = St_underline;
	estk->estk_cline = St_cline;
	estk->estk_c_lntop = St_c_lntop;
	/*
	**	Removed to fix bug #937
	**estk->estk_tname = Cact_tname;
	**estk->estk_attribute = Cact_attribute;
	*/

	St_c_estk = estk;


	return(estk);
}



/*
**   R_POP_ENV - routine to pop an ESTK structure.
**
**	Parameters:
**		none.
**
**	Returns:
**		address of popped structure.
**
**	Called by:
**		r_do_page, r_stp_adjust.
**
**	Side Effects:
**		Changes margins, error ptrs and current line buffers.
**
**	Trace Flags:
**		2.0, 2.6.
**
**	Error Messages:
**		Syserr:No more in stack.
**
**	History:
**		7/21/81 (ps)	written.
**
**		5/10/83 (nl) -	Put in changes since code was
**				ported to UNIX.
**				Get rid of tname and attribute
**				from stack to fix bug 937.
**		11/19/83 (gac)	added position_number.
**		26-feb-2009 (gupsh01)
**		    Reset utf8 adjustment from ln_utf8_adj.
*/

ESTK	*
r_pop_env()
{
	/* start of routine */


	if (St_c_estk == NULL)
	{
		r_syserr(E_RW0022_r_pop_env_No_environm);
	}

	St_right_margin = St_c_estk->estk_rm;
	St_left_margin = St_c_estk->estk_lm;
	St_underline = St_c_estk->estk_ul;
	St_cline = St_c_estk->estk_cline;
	St_pos_number = St_cline->ln_curpos;
	St_c_lntop = St_c_estk->estk_c_lntop;
	if (En_need_utf8_adj)
	  En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;

	/*
	** Removed to fix bug 937.
	**Cact_tname = St_c_estk->estk_tname;
	**Cact_attribute = St_c_estk->estk_attribute;
	*/

	St_c_estk = St_c_estk->estk_above;


	return(St_c_estk);
}


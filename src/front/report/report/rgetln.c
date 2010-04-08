/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rgetln.c	30.1	11/14/84"; */

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
# include	<ug.h>

/*
**   R_GET_LN - set up a LN structure, and clear it out.
**		This also checks to see that there are not more lines
**		in the linked list than En_wmax (to watch for runaways).
**		This also sets the link in the previous structure if a
**		new structure is created. If the link already exists,
**		simply return that structure, cleared out.
**
**	Parameters:
**		oln	LN address of previous structure (for link).
**			NULL if first in list.
**
**	Returns:
**		address of new LN structure.
**		If En_wmax is exceeded, this returns oln.
**
**	Called by:
**		r_rep_set, r_x_print, r_stp_adjst.
**
**	Side Effects:
**		May set St_err_on.
**
**	Trace Flags:
**		2.0, 2.3.
**
**	Error Messages:
**		301.
**
**	History:
**		7/20/81 (ps)	written.
**		6/20/84	(gac)	bug 2213 fixed -- error msg 301 given only once.
**		1/8/84 (rlm)	tagged memory allocation
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		23-feb-1993 (rdrane)
**			Explicitly initialize new LN field ln_ctrl to
**			(CTRL *)NULL (46393 and 46713).
**			Customized IIUGbmaBadMemoryAllocation() text to
**			disambiguate which may have occured.
**			Unconditionally call r_clear_line() - it could never
**			be NOT called.
**		17-mar-1993 (rdrane)
**			Conditionalize issuance of "max lines exceeded" msg
**			to put out specific .BLOCK/non-.BLOCK version (b49749).
**			Effect renaming of "error_301" stuph to be more
**			meaningful.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-nov-2008 (gupsh01)
**	    Added support for -noutf8align flag.
**	26-feb-2009 (gupsh01)
**	    initialize new LN field ln_utf8_adj.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


static	bool	rgtln_max_lines = FALSE;	/*
						** Already issued error for
						** max lines in .BLOCK or
						** max lines without explicit
						** line break.
						*/

LN	*
r_gt_ln(oln)
register LN	*oln;
{
	/* internal declarations */

	register LN	*ln;			/* temp holder of address */
	LN		*fetch;			/* must pass address for alloc */
	char		s_buffer[20];		/* used in writing error */

	/* start of routine */



	if (oln != (LN *)NULL)
	{	/* if there is another there already, use it */
		if (oln->ln_below != (LN *)NULL)
		{	/* return it */
			if ((oln->ln_below)->ln_started == LN_NONE)
			{
				r_clear_line(oln->ln_below);
			}
			return (oln->ln_below);
		}
		/* need a new structure.  Add it if possible */
		if ((oln->ln_sequence + 1) > En_wmax)
		{	/* no more lines can fit.  Return old value */
			if (!rgtln_max_lines)
			{
				CVna(En_wmax,s_buffer);
				if  (St_c_cblk > 0)
				{
					r_error(0x12D,NONFATAL,s_buffer,NULL);
				}
				else
				{
					r_error(0x12E,NONFATAL,s_buffer,NULL);
				}
				St_err_on = FALSE;
				rgtln_max_lines = TRUE;
			}
			return(oln);
		}
		/* fall through if not returned by now */
	}

	/* get a new structure */

        if ((fetch = (LN *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(LN)), TRUE, (STATUS *)NULL)) == (LN *)NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_gt_ln - LN"));
	}
											 
	if (En_need_utf8_adj)
	{	
	    if (En_lmax != En_lmax_orig)
	        En_utf8_lmax = 2 * En_lmax;

            if ((fetch->ln_buf = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(En_utf8_lmax + 1), TRUE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("r_gt_ln - ln_buf"));
	    }
            if ((fetch->ln_ulbuf = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(En_utf8_lmax + 1), TRUE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("r_gt_ln - ln_ulbuf"));
	    }
	}
	else
	{
            if ((fetch->ln_buf = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(En_lmax+1), TRUE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("r_gt_ln - ln_buf"));
	    }
            if ((fetch->ln_ulbuf = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(En_lmax+1), TRUE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("r_gt_ln - ln_ulbuf"));
	    }
	}
	ln = fetch;
	ln->ln_started = LN_NONE;
	ln->ln_curpos = 0;
	ln->ln_has_ul = FALSE;
	ln->ln_ctrl = (CTRL *)NULL;
	ln->ln_below = (LN *)NULL;
	ln->ln_utf8_adjcnt = 0;

	/* Set the buffers to blanks	*/
	r_clear_line(ln);

	/* add to old LN */

	if (oln == (LN *)NULL)
	{
		ln->ln_sequence = 1;
	}
	else
	{
		oln->ln_below = ln;
		ln->ln_sequence = oln->ln_sequence + 1;
	}


	return(ln);
}

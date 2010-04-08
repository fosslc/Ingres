/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rdopage.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<st.h>		 
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_DO_PAGE - print out the page footers for this page, and the page
**	headers for the next page, if the St_do_phead flag is set.
**	The next page number is set to St_nxt_page (if non-negative) or
**	incremented by 1 (if St_nxt_page < 0).
**
**	Remember to use the page margins during the printing of
**	the header and footer for the page.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Updates St_p_number, St_l_number, St_pos_number, St_in_pbreak.
**		and St_nxt_page.
**		Changes, temporarily, the value of St_right_margin,
**		St_left_margin, St_cline and St_c_lntop.
**
**	Called by:
**		r_x_newline, r_x_npage..
**
**	Trace Flags:
**		4.0, 4.5.
**
**	History:
**		4/14/81 (ps) - written.
**		11/19/83 (gac)	changed St_l_number to St_l_number+1.
**		6/5/84	(gac)	fixed bug 2217 -- .NP 0 now starts at page 0.
**		2/21/89 (kathryn) Bug# 3902 -- If .FF has been specified
**			then write formfeed after footer prints.
**		02-aug-89 (sylviap)
**			Took out 'Enter C, S or RETURN...'  prompt.  RW now does
**			scrollable output.
**		3/21/90 (elein) performance
**                      Only call r_stp_adjst iff in adjusting
**		4/22/91 (elein) 36642
**			Pass /f through for terminals if it is set
**    6/10/91 (dchan)
**       The first parameter passed to 
**       r_x_print() was incorrectly passed by value 
**       rather than by reference.
**		8/1/91 (elein) 36642, take two
**			Add & in STcopy. Works ok on unix, not vms
**		8/2/91 (elein 36642, take three
**			& generated compiler warnings on sun.  Should
**			have used PTR cast.
**		22-oct-1992 (rdrane)
**			Set precision in ffdbv DB_DATA_VALUE just to be
**			safe.
**		19-may-1993 (rdrane)
**			Re-parameterize calls to r_wrt() to include explicit
**			length of line to be written, and ensure that we
**			compute that length here.  This is to ensure that
**			a control sequence containing an embedded ASCII NUL
**			does not prematurely end the line.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_do_page()
{

	/* internal declarations */

	bool		no_write = FALSE;	/* true if St_no_write TRUE
						** on entry.  */
	DB_DATA_VALUE	ffdbv;
	DB_TEXT_STRING	*fftext;
	char 		ffbuf[20];


	/* start of routine */



	if (St_p_length <=0)
	{	/* never do page breaks on 0 length page */
		return;
	}

	St_in_pbreak = TRUE;

	if (St_no_write)
	{	/* nothing written yet */
		
		r_set_dflt();
		no_write = TRUE;		/* need to know later */
	}	/* note: r_set_dflt sets St_no_write to FALSE */

	/* temporarily save the current environment */

	r_switch();

	r_ln_clr(Ptr_pln_top);		/* clear out page buffers */
	r_x_tab(St_p_lm,B_ABSOLUTE);

	/* set up error flag switch */

	Cact_tname = NAM_FOOTER;

	if (!no_write)
	{


		r_x_tcmd((Ptr_pag_brk)->brk_footer);

		if ((St_c_cblk>0) || (St_c_lntop->ln_started!=LN_NONE))
		{	/* something left in the line buffers.  Write out */
			St_c_cblk = 0;
			r_x_newline(1);
		}
		/*  Bug #  3902  */
		if (St_ff_on && !St_to_term )
			r_wrt(En_rf_unit, ERx("\f"), 1);
		else if( St_to_term &&  St_ff_on) 
		{
			ffdbv.db_data     = (PTR) ffbuf;
			ffdbv.db_length   = 1;
			ffdbv.db_prec   = 0;
			ffdbv.db_datatype = DB_LTXT_TYPE;
			fftext = (DB_TEXT_STRING *) ffbuf;
			STcopy(ERx("\f"), (PTR)fftext->db_t_text);
			fftext->db_t_count = 1;
			r_x_print( &ffdbv, ffdbv.db_length, FALSE);
		}
	}


	/* determine the next page number */

	if (St_nxt_page >= 0)
	{
		St_p_number = St_nxt_page;
		St_nxt_page = -1;
	}
	else if (!no_write)
	{
		St_p_number++;
	}

	St_l_number = 1;
	St_pos_number = 1;

	/* if the St_do_phead is set to FALSE (at the very end of the
	** report, don't put out a header for the next page */

	if (St_do_phead == TRUE)
	{
		Cact_tname =NAM_HEADER;

		r_x_tcmd((Ptr_pag_brk)->brk_header);
	}

	St_in_pbreak = FALSE;

	if( St_adjusting)
	{
		r_stp_adjust();			/* in case anything is there */
	}

	if ((St_c_cblk>0) || (St_c_lntop->ln_started!=LN_NONE))
	{	/* something left in the line buffers.  Write out */
		St_c_cblk = 0;
		r_x_newline(1);
	}

	/* get back the old environment */

	r_switch();

	r_x_tab(St_left_margin,B_ABSOLUTE);	/* go to report margin */


	return;

}

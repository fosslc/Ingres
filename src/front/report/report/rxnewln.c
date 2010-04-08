/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>
# include	<st.h>		/* 6-x_PC_80x86 */
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
# include	<errw.h>

/*
**   R_X_NEWLINE - write out the current output line, and optionally
**	output blank lines as well.
**
**	If RW is currently in .BLOCK mode (the .BLOCK command is
**	in effect), then don't write out the line buffers, but rather
**	drop down to the next line in the line buffers, and tab to
**	the start of the line.	If .BLOCK is not in effect, write out
**	the lines.
**
**	If underlining has been specified for the line and output is
**	to the terminal, then ignore the specification.  Changing to
**	multi-line underscoring will trash pagination, the prior method
**	of using backspaces resulted in the underling being erased and
**	lost (destructive backspace interpretation by the terminal),
**	and using the carriage return method of explicit file/printer
**	produces a corrupted terminal display.
**
**	If underlining has been specified for the line, output it as
**	carriage return/underscore strings on the same line.	If it
**	has been specified as any other character (usually a -), output
**	it as a separate line.
**
**	However, if underlining output is to the terminal, then ignore
**	the specification.  Changing to multi-line underscoring will
**	trash pagination, the prior method of using backspaces resulted
**	in the underling being erased and lost (destructive backspace
**	interpretation by the terminal), and using the carriage return
**	method of explicit file/printer produces a corrupted terminal display.
**
**
**	Parameters:
**		nlines - number of newlines to write.  If 1, only the
**			current output line is written.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Only indirectly.
**
**	Called by:
**		r_x_tcmd, r_rep_do, r_x_newpage, r_x_eblock.
**
**	Trace Flags:
**		5.0, 5.6.
**
**	History:
**		4/8/81 (ps) - written.
**		11/19/83 (gac)	changed St_l_number to St_l_number+1.
**		6/11/84 (gac)	fixed bug 2057 -- FF char at beginning of line.
**		2/21/89 (kathryn) - Bug#3902 -- No longer print .FF here.
**			.FF will be printed by r_do_page() to ensure it is
**			not written before footer.
**		3/21/90 (elein) performance
**			Check St_no_write before calling r_set_dflt
**		8/17/90 (elein) b32202
**			If underlining is going to take up two lines
**			ensure that they are together.
**		10/13/90 (elein) b33872
**			whoops.  never do a page break in a page break.
**			bug caused by fix to 32202
**		24-feb-1993 (rdrane)
**			Re-parameterize calls to r_wrt_eol() to accept
**			LN structure.  We now track q0 sequences on a per-line
**			basis (46393 and 46713).
**			Note that the code sequence
**				if ((ln->ln_below == NULL)
**				   || ((ln->ln_below!=NULL)
**				   && (ln->ln_below->ln_started==LN_NONE)))
**			is equivalent to
**				if  ((ln->ln_below == (LN *)NULL) ||
**					 (ln->ln_below->ln_started==LN_NONE))
**			Since it occurs multiply and is executed for virtually
**			every line in the report, use the briefer, more
**			efficient latter form.
**		16-mar-1993 (rdrane)
**			Get rid of the Bs_buf and all it stands for and with.
**			The FORMS system doesn't handle backspaces like a
**			printer anyway.  For ULC = '_' (underscore), ignore
**			the specification if going to a terminal - there's no
**			way from within UF to make it work in both the display
**			case as well as the file/print case.  While this does
**			effectively classify bug 49991 as a feature, it also
**			cleans up the logic and eliminates the annoying
**			"flashing" of the underscores on the display menu line
**			when the report runs to completion.  No one's sure how
**			this ever worked ...
**		23-apr-1993 (rdrane)
**			Order text/UL strings in buffer as UL/text, not text/UL
**			(how it was prior to fix for 49991).  This was causing
**			SEP tests to recognize non-significant differences.
**		26-Aug-1993 (fredv)
**			Included <me.h>.
**		03-Nov-1994 (athda01)
**			Fix for bug 12905.  Remove STtrmwhite calls in order to
**			ensure that lines are written with required number of
**			trailing blanks to allow fixed length file records to be
**			created.
**		24-Jan-1996 (abowler)
**			Backed out above change - though strictly correct, it
**			was causing 70653 which is more serious, and more
**			often reported. 12905 has been changed to a SIR.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-feb-2009 (gupsh01)
**	    Reset the En_utf8_adj_cnt for multiline output.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**	    
*/

			/*
			** Pointer to buffer for concatenating underlining
			** (.ULC = '_') underscore buffer, carriage return,
			** and text line.
			*/
static	char	*ul_dsply_buf = NULL;

VOID
r_x_newline(nlines)
register i2	nlines;
{
	/* internal declarations */

	register LN	*ln;		/* fast ptr to ln */

	/* start of routine */


	if( St_no_write)
	{
		r_set_dflt();
	}

	/* if line buffers are empty, prime the first one */

	if (St_c_lntop->ln_started == LN_NONE)
	{
		r_ln_clr(St_c_lntop);
		St_c_lntop->ln_started = LN_WRAP;
	}

	/* check if in .BLOCK mode.  If so simply move down the buffers */

	if (St_c_cblk > 0)
	{	/* in block mode */
		while (nlines-- > 0)
		{
			St_cline->ln_started = LN_NEW;
			St_l_number++;
			if (St_cline->ln_has_ul && (St_ulchar != '_'))
			{
				St_l_number++;
			}
			St_cline = r_gt_ln(St_cline);
			if (En_need_utf8_adj)
			    En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;
			r_x_tab(St_left_margin,B_ABSOLUTE);
		}
		return;
	}

	/*
	** Not in block mode.  Establish and initialize the
	** underlining buffer, then proceed to write the line
	** buffers out to the target device.
	*/
	if  (ul_dsply_buf == NULL)
	{
		if  ((ul_dsply_buf = (char *)MEreqmem((u_i4)0,
					      (u_i4)((2 * En_lmax) + 2),
					      TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx(
						"r_x_newline - ul_dsply_buf"));
		}
	}
	*ul_dsply_buf = EOS;

	while(nlines-- > 0)
	{
		for(ln = St_c_lntop;
		    (ln != (LN *)NULL) && (ln->ln_started != LN_NONE);
		    ln = ln->ln_below)
		{
			if ((ln->ln_has_ul) && (St_ulchar == '_') &&
			    (!St_to_term))
			{	/*
				** Underlining on the same line.
				** Ignore the specification if going to
				** the terminal.  Otherwise, assemble the
				** line now with a carriage  return, and put
				** everything out with the text line in one
				** write.
				*/
				STtrmwhite(ln->ln_ulbuf);
				STtrmwhite(ln->ln_buf);
				STpolycat(3,
					  ln->ln_ulbuf,ERx("\r"),ln->ln_buf,
					  ul_dsply_buf);
			}
			else if ((ln->ln_has_ul) && (St_ulchar != '_') &&
				 (!St_in_pbreak) && (St_p_length > 0) &&
				 ((St_l_number + En_lc_bottom +2) >
							 St_p_length))
			{
				/*
				** If this underlining business is going to
				** take two lines, do page if necessary now so
				** they will be together.  Checking +2 'cause
				** we haven't printed anything yet.  We pass a
				** NULL LN structure because we don't want to
				** prematurely clear any control sequences.
				** Note the implication that q0 and explicit
				** underlining are mutually exclusive (underline
				** using the q0 format is the way to go!)
				*/
				if (r_wrt_eol(En_rf_unit,ERx(""),(LN *)NULL)
									 < 0)
				{
					r_syserr(E_RW004D_r_x_newline_Write_err);
				}
				St_l_number++;
				r_do_page();
				if  ((ln->ln_below == (LN *)NULL) ||
					 (ln->ln_below->ln_started==LN_NONE))
				{	/* end of text and end of page.
					** don't do extra blank lines.*/
					nlines = 0;
				}
			}


			/*
			** Now put out the text line.  Note that text
			** may not be at end of line, and that we may
			** be putting out an underlined text line.
			*/
			if  (*ul_dsply_buf != EOS)
			{
				if (r_wrt_eol(En_rf_unit,ul_dsply_buf,
					      ln) < 0)
				{
					r_syserr(E_RW004D_r_x_newline_Write_err);
				}
				*ul_dsply_buf = EOS;
			}
			else
			{
				STtrmwhite(ln->ln_buf);
				if (r_wrt_eol(En_rf_unit,ln->ln_buf,ln) < 0)
				{
					r_syserr(E_RW004D_r_x_newline_Write_err);
				}
			}
			St_l_number++;

			/* now check for end of page */

			if (!St_in_pbreak && (St_p_length>0))
			{
				if ( (St_l_number+En_lc_bottom) > St_p_length)
				{
					r_do_page();
					if  ((ln->ln_below == (LN *)NULL) ||
						 (ln->ln_below->ln_started ==
								LN_NONE))
					{	/* end of text and end of page.
						** don't do extra blank lines.*/
						nlines = 0;
					}
				}
			}

			if (ln->ln_has_ul && (St_ulchar!='_'))
			{	/* underlining on a new line */
			 	STtrmwhite(ln->ln_ulbuf);
				/*
				** We pass a NULL LN structure because we've
				** already cleared any control sequences when
				** we printed the text.
				*/
				if (r_wrt_eol(En_rf_unit,ln->ln_ulbuf,
					      (LN *)NULL) < 0)
				{
					r_syserr(E_RW004D_r_x_newline_Write_err);
				}
				St_l_number++;

				/* check for end of page */

				if (!St_in_pbreak && (St_p_length>0))
				{
					if ((St_l_number+En_lc_bottom) >
								 St_p_length)
					{
						r_do_page();
						if  ((ln->ln_below ==
								 (LN *)NULL) ||
						     (ln->ln_below->ln_started==
								 LN_NONE))
						{
							/*
							** End of text and end
							** of page.  Don't do
							** extra blank lines.
							*/
							nlines = 0;
						}
					}
				}
			}
		}

		r_ln_clr(St_c_lntop);
		if (nlines > 0)
		{
			/* force the line */
			St_c_lntop->ln_started = LN_WRAP;
		}
		else
		{
			r_x_tab(St_left_margin,B_ABSOLUTE);
		}
	}

	return;

}


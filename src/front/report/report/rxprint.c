/*
** Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)rxprint.c	30.1	11/14/84";	*/

# include	<compat.h>
# include	<me.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h> 
# include	<er.h> 
# include	<ug.h> 
# include	<rtype.h> 
# include	<rglob.h> 

/*
**   R_X_PRINT - add a string to the current output line at the current
**	position, using a maximum length.
**
**	Parameters:
**		string - string to add.
**		len - length of string to add.  If zero, use the terminator
**			to the string to end it.
**			If not, maximum length to write.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update info in St_cline.
**		Will set the St_l_started flag.
**
**	Error messages:
**		204.
**
**	History:
**		4/8/81 (ps) -written.
**		5/11/83 (nl)-Put in changes made since code was ported
**			     to UNIX. Add reference to Cact_written to 
**			     fix bug 937.
**		11/19/83 (gac)	added position_number.
**		04-nov-1986 (yamamoto)
**			Modified for double byte characters.
**		02-oct-89 (sylviap)
**			Added new parameter, printable, to state if string is
**			a control sequence, Q format (FALSE).  Creates a linked
**			list of control sequences.
**		3/21/90 (elein) performance
**			Check St_no_write before catlling r_set_dflt
**		27-sep-90 (sylviap)
**			Allows Q0 format to be printed to the screen.
**		23-feb-1993 (rdrane)
**			Re-work allocation and initialization of CTRL
**			structures.  We now track q0 sequences on a per-line
**			basis (46393 and 46713).  Remove declaration of
**			IIRWgct_get_ctrl() and IIRWgcs_get_cstring() - these
**			are now done directly.
**		19-may-1993 (rdrane)
**			Use MEcopy() to manipulate q0 strings.  STlcopy() was
**			terminating prematurely if the string contained
**			embedded ASCII NUL's.
**		6-aug-1993 (rdrane)
**			Ensure we set ln_started in case a line contains only
**			q0 format strings (b54025).
**      08-30-93 (huffman)
**              add <me.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2009 (gupsh01)
**	    Reset the En_utf8_adj_cnt field for multiline output.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_x_print(string,len,printable)
DB_DATA_VALUE	*string;
register i2	len;
bool		printable;	/* FALSE = control sequence, non-printable */
{
	register LN	*cline;
	register u_char	*str;
	u_char		*strend;
	DB_TEXT_STRING	*textstring;
	CTRL		*c_blk;
	TAGID		ctrl_tag;

	/* start of routine */

	textstring = (DB_TEXT_STRING *)(string->db_data);
	str = textstring->db_t_text;
	strend = str + textstring->db_t_count;
	if (str == strend)
	{
		return;
	}


	if( St_no_write)
	{
		r_set_dflt();
	}


	cline = St_cline;

	if (len == 0)
	{
		len = textstring->db_t_count;
	}
	if (!printable)
	{
		/*
		** String using the Q format.  Allocate all q0 CTRL structures
		** and buffers for a given line with the same memory tag.  If
		** this is the first q0 control sequence, allocate a fresh tag.
		** Otherwise, obtain the currently associated tag from the
		** implied chain head.
		*/
		if  (cline->ln_ctrl == (CTRL *)NULL)
		{
			ctrl_tag = FEgettag();
		}
		else
		{
			ctrl_tag = cline->ln_ctrl->cr_tag;
		}
 		if ((c_blk = (CTRL *)FEreqmem(ctrl_tag,(u_i4)(sizeof(CTRL)),
					      TRUE,(STATUS *)NULL)) ==
								(CTRL*)NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_x_print - q0 block"));
		}
 		if ((c_blk->cr_buf = FEreqmem(ctrl_tag,(u_i4)(len + 1),
					      TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_x_print - q0 buf"));
		}

		/*
		** Establish the position (implied ordinal), length, and
		** contents of this control string, and then add it to the
		** current chain.
		*/
		c_blk->cr_number = cline->ln_curpos;
		c_blk->cr_tag = ctrl_tag;
		c_blk->cr_len = len;
		MEcopy((PTR)str,(u_i2)len,(PTR)c_blk->cr_buf);
		IIRWpct_put_ctrl(c_blk,cline);
		/*
		** Ensure that we flag the line as containing something
		*/
		cline->ln_started = LN_WRAP;
	}
	else
	{

		cline->ln_started = LN_WRAP;

		while (len > 0)
		{	/* more in the string */
			if (cline->ln_curpos < St_right_margin)
			{
			   u_char	*cstr = str;

			   /* fits on this line */
			   if (str < strend)
			   {	/* more chars */
			      if (!CMspace(str))
			      {
				 CMcpychar(str,
				    (cline->ln_buf+cline->ln_curpos));
				 if (St_underline && CMnmchar(str))
				 {
				    *(cline->ln_ulbuf +
				       cline->ln_curpos) = St_ulchar;
				    if (CMdbl1st(str)) *(cline->ln_ulbuf + 
				       cline->ln_curpos + 1) = St_ulchar;
						cline->ln_has_ul = TRUE;
				 }
			      }
			      CMnext(str);
			   }
			   CMbyteinc(cline->ln_curpos, cstr);
			   CMbytedec(len, cstr);
			}
			else
			{	/* doesn't fit.  Wrap around */

			   cline = St_cline = r_gt_ln(cline);
			   if (En_need_utf8_adj)
			       En_utf8_adj_cnt = 0;

			   r_x_tab(St_left_margin,B_ABSOLUTE);
			   cline->ln_started = LN_WRAP;
			}
		}

		St_pos_number = cline->ln_curpos;
	}
	Cact_written = TRUE;

	return;
}

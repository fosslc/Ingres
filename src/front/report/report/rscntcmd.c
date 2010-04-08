/*	
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>

/*  NO_OPTIM=dgi_us5 */
/*{
** Name:	r_scn_tcmd() -		Scan Text Action Command Linked List.
**
** Description:
**   R_SCN_TCMD - scan a tcmd linked list to help set up the default
**	formats and positions for attributes and the default left
**	and right column. This keeps an old right margin and left
**	margin and goes through the TCMD linked list, processing
**	the commands to keep track of the positions of 
**	printing the column values.  The most detailed
**	point of printing a value determines the default position
**	and format.
**
**	Parameters:
**		tcmd	start of linked list of TCMD structures
**			which are to be processed.
**
**	Returns:
**		none.
**
**	Called by:
**		r_set_dflt.
**
**	Trace Flags:
**		140, 145, 147.
**
**	Error Messages:
**		none.
**
**	History:
**		9/13/81 (ps)	written.
**		4/2/83 (ps)	change ref to P_ATT to fix bug 937.
**		6/27/83 (gac)	add date templates.
**		12/12/83 (gac)	add if statement.
**		1/18/84 (gac)	add print expressions.
**		6/5/84	(gac)	fixed bug 2269 -- makes sure that right margin
**				<= number from -l (132 by default).
**		6/7/84	(gac)	fixed bug 582 & 2260 -- adjusting position
**				before printing (i.e. .LEFT, .CENTER, .RIGHT)
**				now sets the default right margin properly.
**		7/5/84	(gac)	fixed bug 3599 -- do not evaluate expressions
**				when scanning.
**		8/2/89 (elein) B7333
**			Added paramenter to r_e_itm to NOT evaluate 
**			expressions when we are only trying to find out
**			length.
**		01-Nov-95 (allmi01)
**			Added NO_OPTIM ming hint to correct a compiler abort
**			when this module is compiled at level 2 optimization.
**		14-Mar-96 (toumi01)
**			Added axp_osf to NO_OPTIM list to correct report
**			right_column format error; problem showed up only
**			in iiweb inventory report testing.
**		15-Jan-99 (schte01)
**			Remove axp_osf from NO_OPTIM list.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jul-2003 (hanal04) Bug 109896 INGCBT 454
**          In scan_loop() check for a a return of 0 cols from fmt_size().
**          This may happen in formats such as (c0) and (t0). If this
**          is the case we should use the dflt_width() of the data item
**          when adjusting curpos.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

/*	external variables to these routines */

	static i4	curpos = 0;		/* current position */
	static bool	linestart = FALSE;	/* TRUE when line started */
	static i4	oldpos = 0;		/* location of start of line */
	static i4	ocurpos = 0;		/* used in adjusting */
	static f4	apos = 0;		/* position in adjusting */
	static bool	in_adjust = FALSE;	/* TRUE when in adjust */
	static bool	nlfound = FALSE;	/* TRUE if .NL found */
	static i4	blklevel = 0;		/* level of .BLK nesting */
	static i4	templm = 0;		/* temp lm */
	static i4	temprm = 0; 		/* temporary lm and rm */
	static i4	curlmaxx = 0;		/* current max x in line */
	static i4	ocurlmaxx = 0;		/* save it for a while */

	static i4	 dflt_width();

VOID
r_scn_tcmd(tcmd)
register TCMD	*tcmd;
{
	/* start of routine */



	/* note that the current position always start at 0 (or LM if set) */

	curpos = St_lm_set ? St_r_lm : 0;
	curlmaxx = curpos;

	Cact_tcmd = tcmd;

	scan_loop((TCMD *)NULL);

	/* Assume that this is the end of the line */

	if (!St_rm_set)
	{
		if (curpos > St_right_margin)
		{
			St_right_margin = min(curpos, En_lmax);
		}
	}


	return;
}

/*   SCAN_LOOP - main loop of R_SCN_TCMD called recursively for IF statement
**	evaluation of alternative branches.
**
**	Parameters:
**		end_tcmd	Pointer to last TCMD to scan. If in an IF
**				statement, this will be the TCMD following
**				the entire IF statement which all lists
**				of TCMDS in the IF statement eventually point
**				to.
*/
 
VOID
scan_loop(end_tcmd)
TCMD	*end_tcmd;
{
	/* internal declarations */

	register TCMD	*tcmd;
	ATTRIB		ordinal;		/* ordinal of attribute */
	ADI_FI_ID	fid;
	register VP	*vp;			/* fast ptr to VP struct */
	IFS		*ifs;
	PEL		*pel;
	f4		end_adjust();
	STATUS		status;
	i4		rows;
	i4		cols;

	/* external values previous to IF statement */

	i4		p_St_left_margin;
	i4		p_St_right_margin;
	i4		p_curpos;
	bool		p_linestart;
	i4		p_oldpos;
	i4		p_ocurpos;
	f4		p_apos;
	bool		p_in_adjust;
	bool		p_nlfound;
	i4		p_blklevel;
	i4		p_templm,p_temprm;
	i4		p_curlmaxx;
	i4		p_ocurlmaxx;

	/* external values for left branch of IF statement */

	i4		l_St_left_margin;
	i4		l_St_right_margin;
	i4		l_curpos;
	bool		l_linestart;
	i4		l_oldpos;
	i4		l_ocurpos;
	f4		l_apos;
	bool		l_in_adjust;
	bool		l_nlfound;
	i4		l_blklevel;
	i4		l_templm,l_temprm;
	i4		l_curlmaxx;
	i4		l_ocurlmaxx;

	while (Cact_tcmd != end_tcmd)
	{	/* go through the text */

		tcmd = Cact_tcmd;			/* faster pointer */
		Cact_tcmd = tcmd->tcmd_below;		/* next action */

		oldpos = -1;	


		switch(tcmd->tcmd_code)
		{
		case(P_PRINT):
			strt_line();

			pel = tcmd->tcmd_val.t_v_pel;
			switch (pel->pel_item.item_type)
			{
			case(I_ATT):
			case(I_ACC):
			case(I_CUM):
				apos = end_adjust();

				switch (pel->pel_item.item_type)
				{
				case(I_ATT):
					ordinal = pel->pel_item.item_val.i_v_att;
					fid = 0;
					break;

				case(I_ACC):
					ordinal = pel->pel_item.item_val.i_v_acc->acc_attribute;
					fid = pel->pel_item.item_val.i_v_acc->acc_ags.adf_agfi;
					break;

				case(I_CUM):
					ordinal = pel->pel_item.item_val.i_v_cum->cum_attribute;
					fid = pel->pel_item.item_val.i_v_cum->cum_ags.adf_agfi;
					break;
				}
	
				curpos += r_st_adflt(ordinal, (i4)apos,
						     pel->pel_fmt, fid);
				break;
	
			default:
				if (pel->pel_fmt != NULL)
				{
				    status = fmt_size(Adf_scb, pel->pel_fmt,
						      NULL, &rows, &cols);
				    if (status != OK)
				    {
					FEafeerr(Adf_scb);
				    }

				    /* Formats such as c0 and t0 will 
				    ** return zero columns. If this happens
				    ** use the dflt_width().
				    */
				    if (cols)
				    {
				        curpos += cols;
				    }
				    else
				    {
					curpos += dflt_width(&pel->pel_item);
				    }
				}
				else
				{
					curpos += dflt_width(&pel->pel_item);
				}
				break;
			}

			curlmaxx = max(curlmaxx, curpos);
			break;

		case(P_TAB):
			vp = tcmd->tcmd_val.t_v_vp;
			switch(vp->vp_type)
			{
				case(B_ABSOLUTE):
					curpos = vp->vp_value;
					break;

				case(B_RELATIVE):
					curpos += vp->vp_value;
					break;

				case(B_DEFAULT):
					curpos = St_left_margin;
					break;

				case(B_COLUMN):
					curpos = (r_gt_att((ATTRIB)vp->vp_value))->att_position;
					if (curpos < 0)
					{
						curpos = 0;
					}

					break;
			}
			if ((!St_lm_set) && (curpos<St_left_margin))
			{
				oldpos = curpos;
			}
			break;

		case(P_WITHIN):
			r_x_within(tcmd);
			curpos = St_left_margin;
			templm = St_left_margin;
			temprm = St_right_margin;
			ocurlmaxx = curlmaxx;
			curlmaxx = curpos;
			break;

		case(P_RIGHT):
		case(P_CENTER):
		case(P_LEFT):
			strt_line();

			/* curpos at end of print determine the
			** actual next current position */

			ocurpos = curpos;
			curpos = 0;
			in_adjust = TRUE;
			St_a_tcmd = tcmd;
			break;

		case(P_END):
			switch(tcmd->tcmd_val.t_v_long)
			{
			case(P_PRINT):
				/* if adjusting in effect, fix it up */
				if (in_adjust)
				{
					apos = r_eval_pos(St_a_tcmd,ocurpos,FALSE);
					switch(St_a_tcmd->tcmd_code)
					{  /* figure out new position */
					case(P_LEFT):
						apos += curpos;
						break;
					case(P_CENTER):
						apos += curpos/2.0;
						break;
					case(P_RIGHT):
						break;
					}
					curpos = (apos>curpos) ? apos : curpos;
					in_adjust = FALSE;
				}
				break;

			case(P_WITHIN):
				/* end .WITHIN loop */
				curpos = St_right_margin;
				templm = min(templm,St_left_margin);
				temprm = max(temprm,St_right_margin);
				r_x_ewithin();		/* restores old margins */
				if (St_cwithin == NULL)
				{	/* all done with .WITHIN loop */
					St_left_margin = (!St_lm_set ? min(templm,St_left_margin) : St_r_lm);
					St_right_margin = (!St_rm_set ? max(temprm,St_right_margin) : St_r_rm);
					curlmaxx = ocurlmaxx;
				}
				else
				{	/* more in within loop */
					curpos = St_left_margin;
					curlmaxx = curpos;
				}
				break;

			case(P_BLOCK):
				/* end of block.  Same as a newline */
				if(--blklevel<=0 && nlfound)
				{
					curpos = (St_lm_set ? St_r_lm : 0);
					curlmaxx = curpos;
				}
				break;

			default:
				/* other .ENDs fall through */
				break;
			}
			break;

		case(P_BLOCK):
			nlfound = FALSE;
			blklevel++;
			break;

		case(P_NEWLINE):
		case(P_NPAGE):
			/* set temporary right margin */

			if (!St_rm_set)
			{
				if (curpos > St_right_margin)
				{
					St_right_margin = min(curpos, En_lmax);
				}
			}

			linestart = FALSE;
			if (St_cwithin != NULL)
			{	/* in a within loop */
				curpos = St_left_margin;
			}
			else
			{
				curpos = (St_lm_set ? St_r_lm : 0);
			}
			curlmaxx = curpos;
			nlfound = TRUE;
			break;

		case(P_LINEEND):
			curpos = curlmaxx;
			break;

		case(P_IF):

			/* save previous external values */
			p_St_left_margin = St_left_margin;
			p_St_right_margin = St_right_margin;
			p_curpos = curpos;
			p_linestart = linestart;
			p_oldpos = oldpos;
			p_ocurpos = ocurpos;
			p_apos = apos;
			p_in_adjust = in_adjust;
			p_nlfound = nlfound;
			p_blklevel = blklevel;
			p_templm = templm;
			p_temprm = temprm;
			p_curlmaxx = curlmaxx;
			p_ocurlmaxx = ocurlmaxx;

			ifs = tcmd->tcmd_val.t_v_ifs;
			Cact_tcmd = ifs->ifs_then;
			scan_loop(tcmd->tcmd_below);

			/* save external values for left ("then") list of actions */
			l_St_left_margin = St_left_margin;
			l_St_right_margin = St_right_margin;
			l_curpos = curpos;
			l_linestart = linestart;
			l_oldpos = oldpos;
			l_ocurpos = ocurpos;
			l_apos = apos;
			l_in_adjust = in_adjust;
			l_nlfound = nlfound;
			l_blklevel = blklevel;
			l_templm = templm;
			l_temprm = temprm;
			l_curlmaxx = curlmaxx;
			l_ocurlmaxx = ocurlmaxx;

			/* restore external values previous to IF statement */
			St_left_margin = p_St_left_margin;
			St_right_margin = p_St_right_margin;
			curpos = p_curpos;
			linestart = p_linestart;
			oldpos = p_oldpos;
			ocurpos = p_ocurpos;
			apos = p_apos;
			in_adjust = p_in_adjust;
			nlfound = p_nlfound;
			blklevel = p_blklevel;
			templm = p_templm;
			temprm = p_temprm;
			curlmaxx = p_curlmaxx;
			ocurlmaxx = p_ocurlmaxx;

			Cact_tcmd = ifs->ifs_else;
			scan_loop(tcmd->tcmd_below);

			/* compare and adjust external values */
			St_left_margin = min(St_left_margin, l_St_left_margin);
			St_right_margin = max(St_right_margin, l_St_right_margin);
			curpos = max(curpos, l_curpos);
			linestart = linestart && l_linestart;
			oldpos = max(oldpos, l_oldpos);
			ocurpos = max(ocurpos, l_ocurpos);
			apos = (apos>l_apos) ? apos : l_apos;
			in_adjust = in_adjust && l_in_adjust;
			nlfound = nlfound && l_nlfound;
			blklevel = min(blklevel, l_blklevel);
			templm = min(templm, l_templm);
			temprm = max(temprm, l_temprm);
			curlmaxx = max(curlmaxx, l_curlmaxx);
			ocurlmaxx = max(ocurlmaxx, l_ocurlmaxx);

			Cact_tcmd = tcmd->tcmd_below;
			break;

		default:
			break;

		}

		/* if line started, maybe set left margin */


		if ((oldpos >= 0) && (!St_lm_set)
				  && (oldpos < St_left_margin))
		{
			St_left_margin = oldpos;
		}
	}

	return;
}

/*
**   STRT_LINE - start a new line by setting linestart to TRUE and changing oldpos.
**	This is called by each of the printing commands in r_scn_tcmd.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Uses:
**		oldpos, curpos, linestart.
*/

VOID
strt_line()
{
	if (!linestart)
	{
		oldpos = curpos;
		linestart = TRUE;
	}
	return;
}


/*
**   END_ADJUST - if adjusting in effect, look at the old adjusting command, and
**	find the correct position associated with the command.  this is used by
**	the commands which may set the position of an attribute.
**
**	Parameters:
**		none.
**
**	Returns:
**		value of apos.
*/

f4
end_adjust()
{
	if (!in_adjust)
	{
		return (f4)curpos;
	}

	apos = r_eval_pos(St_a_tcmd,ocurpos,FALSE);
	switch(St_a_tcmd->tcmd_val.t_v_vp->vp_type)
	{
		case(P_CENTER):
			apos -= curpos/2.0;
			break;

		case(P_LEFT):
			break;

		case(P_RIGHT):
			apos -= curpos;
			break;
	}

	return apos = (apos < 0 ? 0 : apos);
}


/* DFLT_WIDTH  -- returns the default width for an item */
/* History 
* 	8/1/89 elein b7333
	Pass additional parameter to reitem.
*/
	
i4
dflt_width(item)
ITEM	*item;
{
	DB_DATA_VALUE	value;
	i4	length;

	r_ges_reset();
	r_e_item(item, &value, FALSE);
	afe_dplen(Adf_scb, &value, &length);
	return(length - DB_CNTSIZE);
}

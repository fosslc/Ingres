/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	*Sccsid = "@(#)rnxtrow.qc	1.4  3/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>		 
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<afe.h>

/*
**   R_NXT_ROW - process the next row read in the report relation.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_do.
**
**	Trace Flags:
**		4.1.
**
**	History:
**		11/30/82 (ps)	written.
**		11/2/83 (gac)	added truncation of floating point columns
**				with F or numeric template formats for +t.
**		2/1/84 (gac)	changed r_deblank to r_fix_data.
**		6/13/85	(ac)	bug# 5599 fixes.  Set St_before_report to FALSE
**				after calling r_x_tcmd. Since r_e_belexpr()
**				called by r_x_tcmd relies on the value of 
**				St_before_report to be TRUE to flag the first 
**				row of data read.
**		7/26/85 (gac)	Break on formatted dates.
**
**		3/6/86	(ac)	Bug # 8323 fix. Make the rounding correct for
**				the negative number.
**		10-jun-86 (mgw) Bug # 9551
**				reordered arguments to FEtalloc() so that the
**				number of elements and the number of bytes per
**				element are in the correct order. This wasn't
**				a problem since they just get multiplied with
**				each other, but it was sloppy.
**		22-jan-88 (rdesmond)
**			changed variable 'roundup' to 'r_roundup' since SUN
**			has a macro by the same name
**		07/20/89 (martym) Bug #6527	
**			Took absolute value of data type, because it could 
**			be nullable.
**		2/21/90 (elein) b9354
**			Ensure no warning msgs from fmt_format
**      	03-dec-90 (steveh)      
**			Fixed bug 33558. This bug caused the report
**                      writer to incorrectly break when fields were
**                      formatted using the C0 format.  This was caused
**                      by junk left over at the end of the data value
**                      after formatting by f_string.  f_string now
**                      pads the formatted data string with blanks.
**		25-jun-96 (abowler)
**			B74106: Don't try to 'round' null float values.
**			This caused an accvio on AXP VMS.
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
r_nxt_row()
{

	/* start of routine */

	St_in_retrieve = TRUE;


	/* process any breaks */

	if (St_before_report)
	{
		St_break = Ptr_brk_top;

		/* set up error report flags */

		Cact_attribute = NAM_REPORT;
		Cact_tname = NAM_HEADER;
		r_do_header(St_break);
		r_trunc();
		r_brkdisp();
		St_before_report = FALSE;
	}
	else
	{
		r_trunc();
		r_brkdisp();
		if ((St_break = r_find_brk()) != NULL)
		{
			Cact_tname = NAM_FOOTER;
			r_do_footer(St_break);
			Cact_tname = NAM_HEADER;
			r_do_header(St_break);
		}
	}

	
	/* process the detail text */

	Cact_attribute = Cact_tname = NAM_DETAIL;
	r_x_tcmd(Ptr_det_brk->brk_header);

	r_mov_dat();			/* move data to previous
					** spot */

	return;
}

VOID
r_trunc()	/* Truncate floating point columns with F or numeric template
		   format if truncate flag set */
{
	/* external declarations */

	NUMBER		MHfdint();
	NUMBER		MHipow();

	/* internal declarations */

	ATTRIB		i;
	ATT		*att;
	FMT		*fmt;
	NUMBER		num;
	NUMBER		ten = 10.0;
	NUMBER		powerof10;
	NUMBER		s_roundup = 0.5;

	if (St_truncate || (!St_tflag_set && St_compatible))
	{
		/*
		** In 5.0, +t was the default.
		** In 6.0, neither +t nor -t is the default so for compatibility
		** have +t be the default.
		** In this routine +t implies that floats are truncated
		** to the precision of their format.
		*/
		for (i = 1; i <= En_n_attribs; i++)
		{
			att = r_gt_att(i);
			fmt = att->att_format;
			if (att->att_dformatted)
			{
				if (abs(att->att_value.db_datatype) == DB_FLT_TYPE &&
				    (!AFE_ISNULL(&(att->att_value))) &&
				    (fmt->fmt_type==2  /* F_F */ ||
				     fmt->fmt_type==10 /* F_NT */))
				{
					powerof10 = MHipow(ten, (i4)fmt->fmt_prec);
					if (att->att_value.db_length <= 5)
					{
						num = *((f4 *)(att->att_value.db_data));
					}
					else
					{
						num = *((f8 *)(att->att_value.db_data));
					}

					num *= powerof10;
					num = num + (num >= 0.0 ? s_roundup : -s_roundup);
					num = MHfdint(num) / powerof10;
					if (att->att_value.db_length == 4)
					{
						*((f4 *)(att->att_value.db_data)) = num;
					}
					else
					{
						*((f8 *)(att->att_value.db_data)) = num;
					}
				}
			}
		}
	}
}

/*{
** Name:	r_brkdisp	Formats all break attributes into their
**				displayed value.
**
** Description:
**	This routine formats all break attributes into their displayed value.
**	This is done so that the formatted value can be compared with the
**	previous one to determine breaks.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** History:
**	18-feb-87 (grant)	implemented.
**	23-oct-1992 (rdrane)
**		Ensure set/propagate precision in DB_DATA_VALUE
**		structures.
*/

VOID
r_brkdisp()
{
    /* external declarations */
    ATT		*r_gt_att();

    /* internal declarations */

    register ATT	*att;	/* ptr to ATT struct associated with break */
    register BRK	*brk;	/* ptr to found BRK structure */
    STATUS		status;
    DB_DATA_VALUE	display;
    i4			fmtkind;

    /* start of routine */

    /* start checking at the second BRK structure, because the first
    ** is always for the overall report.
    */

    for (brk = Ptr_brk_top->brk_below; brk != NULL; brk = brk->brk_below)
    {
	att = r_gt_att(brk->brk_attribute);

	if (att->att_dformatted)
	{
	    display.db_datatype = DB_LTXT_TYPE;
	    display.db_length = att->att_dispwidth;
	    display.db_prec = 0;
	    display.db_data = (PTR)(att->att_cdisp);

	    /* MEfill-ing the work buffer with blanks fixes
	       bug 33558.  This overwrites left-over junk
	       in the buffer with blanks and allows for a
	       correct comparison between the current and
	       previous records. (steveh)                  */
	    MEfill(att->att_dispwidth, ' ', display.db_data);

	    status = fmt_format(Adf_scb, att->att_format, &(att->att_value),
				&display, FALSE);
	    if (status != OK && status != E_DB_WARN)
	    {
		FEafeerr(Adf_scb);
	    }
	}
    }
}

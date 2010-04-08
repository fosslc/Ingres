/*
** Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include    <adf.h>
# include    <fmt.h>
# include    <rtype.h> 
# include    <rglob.h> 
# include    <cm.h> 
# include    <si.h>
# include    <er.h>
# include    <errw.h>

# define    CWIDTH    26

/*
**   R_M_LABELS - set up a LABELS default report.
**
**    Parameters:
**        none.
**
**    Returns:
**        none.
**
**    Called by:
**        r_m_rprt()
**
**    Side Effects:
**        May change format widths for numeric fields.
**
**    Trace Flags:
**
**    History:
**        12/2/89 (martym)  written for report and REPORT/RBF.
**	  2/7/90 (martym) Coding standards cleanup.
**        4/5/90 (elein) performance
**                Since fetching attribute call needs no range
**                check use &Ptr_att_top[] directly instead of
**                calling r_gt_att. We don't expect brk_attribute
**                to be A_REPORT, A_GROUP, or A_PAGE
**	  5/3/90 (martym)
**		 Now adding additional NEWLINE's to compensate 
**		 for wrapping fields in the code that's generated 
**		 for the default label reports.
**	17-aug-90 (sylviap)
**		Sets St_right_margin to En_lmax_orig so the report title 
**		for a label report will be centered. (#31446)
**	12/09/92 (dkh) - Added Choose Columns support for rbf.
**	31-dec-1992 (rdrane)
**		Use the absolute value of the datatype when determining
**		format coercion for ints and floats - NULLablilty is not
**		a deciding factor (bug 43060).  Add DECIMAL to this
**		coercion.  Note that the default assumption is probably
**		too small for current limits, and for true consistency
**		we are going to coerce floats as well (the whole idea here
**		seems to be to make all numeric formats the same width as
**		that for float - whose assumed default width of of 10 may
**		change!).
**	15-jan-1993 (rdrane)
**		Generate the attribute name as unnormalized.
**		Remove declaration of r_gt_att() since already declared
**		in included hdr files.
**	2-feb-1992 (rdrane)
**		Bypass columns which are unsupported datatypes as well
**		as pre-deleted.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and move r_m_InitLabs to rglob.h
**	    to eliminate gcc 4.3 warnings.
*/



VOID
r_m_labels()
{

    register ATT *att;        
    register ATTRIB i;
    i4  j = 0;    
    i4  curx=0;
    i4  nextx=0;
    i4  cury=1;
    i2 width=0;
    i4  maxx=0;
    i4   tmp_r_margin;

    i4  rows = 0;
    i4  cols = 0;

    bool is_num;

    char ibuf[50];
    char newname [FE_MAXNAME+1];
    char fmtstr [MAX_FMTSTR];
    char tmp_buf1[(FE_UNRML_MAXNAME + 1)];

    /*
    ** Initialize the label's dimensions:
    */
    LabelInfo. LineCnt = LabelInfo. MaxFldSiz = LabelInfo. MaxPerLin = 0;
    LabelInfo. VerticalSpace = LABST_VERT_DEFLT;
    LabelInfo. HorizSpace    = LABST_HORZ_DEFLT;

    /*
    ** FIXME: Incldue the following code when ready to implement 
    ** block style reports in the ReportWriter:
    **
    ** Declare the variables needed:
    _VOID_ DeclareVar(ERx(LABST_LINECNT), ERx("int"));
    _VOID_ DeclareVar(ERx(LABST_MAXFLDSIZ), ERx("int"));
    _VOID_ DeclareVar(ERx(LABST_MAXPERLIN), ERx("int"));
    _VOID_ r_dcl_set(FALSE);
    */

    /* 
    ** set up the detail lines:
    */
    Cact_attribute = NAM_DETAIL;
    Cact_type = B_HEADER;

    curx = St_left_margin;
    nextx = St_left_margin;
    cury = 1;

    /* 
    ** First fill in the default locations etc. for each column:
    */
    for (i = 1; i <= En_n_attribs; i ++)
    {
        att = &(Ptr_att_top[i-1]);

	if  ((att->pre_deleted) ||
	     (att->att_value.db_data == (PTR)NULL))
	{
		continue;
	}

        /* 
        ** ensure format width of numeric fields is 10
        */
        switch(abs(att->att_value.db_datatype))
	{
	case DB_INT_TYPE:
	    is_num = TRUE;
	    fmt_setfmt(Adf_scb, ERx("f10"), att->att_format,
		       &(att->att_format), NULL);
	    break;
	case DB_FLT_TYPE:
	    is_num = TRUE;
	    fmt_setfmt(Adf_scb, ERx("n10.3"), att->att_format,
		       &(att->att_format), NULL);
	    break;
	case DB_DEC_TYPE:
	    is_num = TRUE;
            if  (DB_S_DECODE_MACRO(att->att_value.db_prec))
	    {
		fmt_setfmt(Adf_scb, ERx("f10.3"), att->att_format,
			   &(att->att_format), NULL);
	    }
	    else
	    {
		fmt_setfmt(Adf_scb, ERx("f10"), att->att_format,
			   &(att->att_format), NULL);
	    }
	    break;
        default:
	    is_num = FALSE;
	    break;
	}

	if  (is_num)
	{
            att->att_dformatted = FALSE;
            r_x_sformat(i, att->att_format);
            width = 24;   /* 10 + title */
        }
	else
	{
            width = r_ret_wid(i);
	}

        att->att_width = width;
        if (LabelInfo. MaxFldSiz < width)
            LabelInfo. MaxFldSiz = width;

        /* 
        ** Go to next line:
        */
        cury ++;
        maxx=max(maxx,curx);
        curx = St_left_margin;
        nextx = curx;
        att->att_position = nextx;
        att->att_line = cury;
        nextx += (CWIDTH * ((width/CWIDTH)+1));
        curx += width;
    }

    LabelInfo. MaxFldSiz += LabelInfo. HorizSpace;

    /*
    ** Never divide by zero:
    */
    if (LabelInfo. MaxFldSiz != 0)
        LabelInfo. MaxPerLin = (i4)En_lmax / (i4)LabelInfo. MaxFldSiz;


    /*
    ** set En_lmax equal to the size of the largest field so that 
    ** when we return we can catch the error where we have exceeded 
    ** En_lmax:
    */
    En_lmax = (i4)LabelInfo. MaxFldSiz;
    if (LabelInfo. MaxPerLin == 0)
	LabelInfo. MaxPerLin = En_lmax;

    maxx = max(maxx, curx);
    St_right_margin = maxx;


    /* 
    ** keep on one page 
    if (cury > 1)
    {    
        CVna(cury,ibuf);
        r_m_action(ERx("need"), ibuf, NULL);
    }
    */

    r_m_action(ERx("top"), NULL);

    /* 
    ** Now put out the commands for a wrapped detail line:
    */

    cury = 1;
    for (i = 1; i <= En_n_attribs; i ++)
    {    
        att = &(Ptr_att_top[i-1]);

	if  ((att->pre_deleted) ||
	     (att->att_value.db_data == (PTR)NULL))
	{
		continue;
	}

	/*
        r_m_action(ERx("tab"), ERx("("), ERx("$"), ERx(LABST_LINECNT),
            ERx("*"), ERx("$"), ERx(LABST_MAXFLDSIZ), ERx(")"), NULL);
	*/
	_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
	r_m_action(ERx("print"), &tmp_buf1[0], NULL);
	fmt_size(Adf_scb, att->att_format, NULL, &rows, &cols);
	if (rows == 0)	
		rows = 2;
        CVna(rows,ibuf);
        r_m_action(ERx("newline"), ibuf, NULL);
    }

    /*
    ** FIXME: Incldue the following code when ready to implement 
    ** block style reports in the ReportWriter:
    **
    r_m_action(ERx("let"), ERx(LABST_LINECNT), ERx("="),
        ERx("$"), ERx(LABST_LINECNT), ERx("+1"), 0);
    r_m_action(ERx("endlet"), 0);
    r_m_action(ERx("if"), ERx("$"), ERx(LABST_LINECNT), ERx("="),
    	ERx("$"), ERx(LABST_MAXPERLIN), 0);
    r_m_action(ERx("then"), 0);
    r_m_action(ERx("let"), ERx(LABST_LINECNT), ERx("=0"), 0);
    r_m_action(ERx("endlet"), 0);
    r_m_action(ERx("newline"), 0);
    r_m_action(ERx("newline"), 0);
    r_m_action(ERx("end"), ERx("block"), 0);
    r_m_action(ERx("begin"), ERx("block"), 0);
    r_m_action(ERx("endif"), 0);
    */

    /* 
    ** Set up the report header:
    */
    Cact_attribute = NAM_REPORT;
    Cact_type = B_HEADER;
    r_m_rhead();
    r_m_ptop(0,0,1);

    /* 
    ** Save the old right margin.  Need to reset to the width of the report 
    ** so the report title will be centered.  (#31446)
    */
    tmp_r_margin = St_right_margin;
    St_right_margin = En_lmax_orig;

    r_m_rtitle(1,2);

    St_right_margin = tmp_r_margin; 	/* reset back */

    /* 
    ** Set up the page header:
    */
    Cact_attribute = NAM_PAGE;
    Cact_type = B_HEADER;
    r_m_ptop(1,0,1);
    r_m_action(ERx("newline"), NULL);

    /* 
    ** Set up the page footer:
    */
    Cact_attribute = NAM_PAGE;
    Cact_type = B_FOOTER;
    r_m_pbot(1,1);

    return;

}



/*
**   R_M_INITLABS - Initialize a labels style report's dimensions
**
**    Parameters:
**        none.
**
**    Returns:
**        none.
**
**    Called by:
**        r_m_labels().
**	      rFscan_commands().
**
**    Side Effects:
**        Global struct LabelInfo gets initialized.
**
**    Trace Flags:
**
**    History:
**        12/12/89 (martym)  written for report and REPORT/RBF.
*/



VOID r_m_InitLabs()
{

    /*
    ** Initialize the label's dimensions:
    */
    LabelInfo. LineCnt = LabelInfo. MaxFldSiz = LabelInfo. MaxPerLin = 0;
    LabelInfo. VerticalSpace = LABST_VERT_DEFLT;
    LabelInfo. HorizSpace    = LABST_HORZ_DEFLT;

    return;

}

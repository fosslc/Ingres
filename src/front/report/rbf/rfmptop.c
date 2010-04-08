/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmptop.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_PTOP - set up the page top headings in a RBFed report.
**	This will contain the date, time and report name in different
**	combinations, depending on the report width.
**
**	Parameters:
**		type = 0 if the page top for the report.  This will
**			not contain the report name.
**		     = 1 if the page top for the page break.  This will
**			contain the report name.
**		labove	number of lines above the page heading.
**		lbelow	number of lines below the page heading.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		rFsave.
**
**	Trace Flags:
**		180, 184.
**
**	History:
**		4/4/82 (ps)	written.
**		1/26/83 (gac)	does not generate .TFORMAT for non-break 
**				attributes.
**		1/29/87 (rld)	replaced call to f_fmtstr with reference 
**				to ATT.fmtstr
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		16-oct-89 (cmr)	remove refs to copt_whenprint.  
**	28-jan-90 (cmr) give title a more reasonable size based on 
**			FE_MAXNAME.  N.B. if the size of F_RF0021_Report_
**			changes then the size of title needs to be updated.
**	2/20/90 (martym) Changed to return STATUS passed back by the 
**		     SREPORTLIB routines.
**	04-jun-90 (cmr) Put back in support for TFORMAT.
**	29-aug-90 (sylviap) 
**		Added right_margin as a parameter. #32701
**      2-nov-1992 (rdrane)
**      	Remove all FUNCT_EXTERN's since they're already
**		declared in included hdr files.  Ensure that we generate
**		string constants with single, not double quotes.  Ensure
**		that we ignore unsupported datatypes.  Generate the atrribute
**		name for any .TFORMAT as unnormalized.
**	2-jul-1993 (rdrane)
**		Added support for user override of hdr/ftr date/time
**		and page # formats.  Output the report variables with
**		reference to the declared formats, assuming that the
**		report wants to include them, and that they'll fit.
**		Removed wdate and wtime, since these are now variable -
**		use the widths saved in the Opt instead.  Remove FEadfcb()
**		call and use global Adf_scb.
**	12-jul-1993 (rdrane)
**		Always use the "current_time" RW variable.  The added support
**		for user override of hdr date/time can result in specification
**		of a format requiring the time in either one, and
**		"current_date" only contains a fixed "midnight" time component.
**	13-jul-1993 (rdrane)
**		Eliminate space between current_time and its open format
**		parens.  This is consistent w/ SREPORT formatting, and
**		eliminates DIFF's between RBF archive and COPYREP/SREPORT
**		results.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/



STATUS rFm_ptop(type,labove,lbelow, right_margin)
i4	type;
i4	labove;
i4	lbelow;
i4	right_margin;
{
	/* internal declarations */

	i4		wdate;		/* width of date timestamp */
	i4		wtime;		/* width of time timestamp */
	i4		wname;		/* width of title */
	i4		wreport;	/* width of the report */
	i4		midrep;		/* midpoint of the report */
	char		title[FE_MAXNAME + 9];	/* holds "Report: " */
						/* plus En_report  */
	char		ibuf[4];	/* holds return from CVna */
	i4		ord;
	ATT		*att;
	COPT		*copt;
	char		fmtstr[MAX_FMTSTR];
	STATUS 		stat = OK;
	char		tmp_buf1[(FE_UNRML_MAXNAME + 1)];


	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0) || TRgettrace(184,0))
	{
		SIprintf(ERx("rFm_ptop: entry.\r\n"));
	}
#	endif

	RF_STAT_CHK(s_w_action(ERx("begin"), ERx("rbfptop"), (char *)0));

	if (labove > 0)
	{	/* put some lines above the page top */
		CVna(labove,ibuf);
		RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
	}

	STcopy(ERget(F_RF0021_Report_), &tmp_buf1[0]);
	STcat(&tmp_buf1[0],En_report);

	wdate = 0;
	wtime = 0;
	if  (Opt.rdate_inc_fmt == ERx('y'))
	{
		wdate = Opt.rdate_w_fmt;
	}
	if  (Opt.rtime_inc_fmt == ERx('y'))
	{
		wtime = Opt.rtime_w_fmt;
	}
	wname = (type==0) ? 0 : STlength(&tmp_buf1[0]);
	wreport = right_margin - St_left_margin;
	midrep = wreport/2;

	if (((midrep-(wname/2)+1)>(St_left_margin+wdate+1))
		&& ((midrep+(wname/2)+1)<(right_margin-wtime)))
	{	/* all of them fit.  Print them all conditionally */

		if  (Opt.rdate_inc_fmt == ERx('y'))
		{
			RF_STAT_CHK(s_w_action(ERx("left"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("print"),
				    ERx("current_time($"),
				    RBF_NAM_DATE_FMT,ERx(")"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
		}
		if (type != 0)
		{
			RF_STAT_CHK(s_w_action(ERx("center"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("print"), ERx("\'"), 
					&tmp_buf1[0], ERx("\'"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
		}

		if  (Opt.rtime_inc_fmt == ERx('y'))
		{
			RF_STAT_CHK(s_w_action(ERx("right"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("print"),
				    ERx("current_time($"),
				    RBF_NAM_TIME_FMT,ERx(")"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
		}
	}
	else if ((type!=0) && ((wdate+wname+1) <= wreport))
	{	/* at least date and name fit */

		if  (Opt.rdate_inc_fmt == ERx('y'))
		{
			RF_STAT_CHK(s_w_action(ERx("left"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("print"),
				    ERx("current_time($"),
				    RBF_NAM_DATE_FMT,ERx(")"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
		}

		RF_STAT_CHK(s_w_action(ERx("right"),(char *)0));
		RF_STAT_CHK(s_w_action(ERx("print"), ERx("\'"), 
				&tmp_buf1[0], ERx("\'"), (char *)0));
		RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
	}
	else if (type!=0)
	{	/* only the name will fit.  Use that */
		RF_STAT_CHK(s_w_action(ERx("center"),(char *)0));
		RF_STAT_CHK(s_w_action(ERx("print"), ERx("\'"), 
			&tmp_buf1[0], ERx("\'"), (char *)0));
		RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
	}
	else
	{	/* only the date will fit.  Print it */
		if  (Opt.rdate_inc_fmt == ERx('y'))
		{
			RF_STAT_CHK(s_w_action(ERx("center"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("print"),
				    ERx("current_time($"),
				    RBF_NAM_DATE_FMT,ERx(")"),(char *)0));
			RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));
		}
	}

	if (lbelow > 0)
	{	/* number of lines below the heading */
		CVna(lbelow,ibuf);
		RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
	}

	/* If any "whenprint" break fields are set, add "tformat" commands */

	for (ord=1; ord<=En_n_attribs; ord++)
	{
		copt = rFgt_copt(ord);
		if (copt->copt_break=='y' &&
		    ((copt->copt_whenprint=='t') || (copt->copt_whenprint=='p')))
		{	/* needs a 'tformat' */
			att = r_gt_att(ord);
			/*
			** Test is for fix to bug #1020. att->att_format
			** could be a null pointer if the field was deleted.
			*/
			if  ((att != (ATT *)NULL) && (att->att_format != NULL))
			{
				if (fmt_fmtstr(Adf_scb, att->att_format,
				    fmtstr) != OK)
				{
					IIUGerr(E_RF0037_rFm_ptop_Can_t_get_fo, 
					    UG_ERR_FATAL, 0);
				}
				_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
				RF_STAT_CHK(s_w_action(ERx("tformat"), 
					&tmp_buf1[0], ERx("("), fmtstr, 
					ERx(")"),(char *)0));
			}
		}
	}

	RF_STAT_CHK(s_w_action(ERx("end"), ERx("rbfptop"), (char *)0));

	return(OK);
}

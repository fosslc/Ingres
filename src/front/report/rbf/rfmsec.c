/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmsec.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_SEC - set up the commands for one section of the report.
**	This reads all the trims and fields, either associated
**	or unassociated, and converts them into report writer
**	commands.  Each omnilinear part of the section is
**	put in a .BLOCK/.ENDBLOCK group so that they will be
**	written out together.  Also, if underlining is in effect,
**	the line with the last trim or field for each attribute
**	is underlined, as well as the bottom unassociated trim or
**	field.
**	Aggregates:  Aggregates (derived fields) are treated like 
**	ordinary fields except that no .within is needed.  The 
**	printing of an aggregate is surrounded by a begin/end 
**	RBFAGGS.  The validation string for the field contains the
**	text to be written (e.g. "count(empno)" or "avg(salary)").
**
**	Note that this routine assumes that the trims and fields
**	are sorted in order from the top left corner of the screen
**	down toward the bottom right (as a Y,X sort).
**
**	Parameters:
**		tline	top line of section.
**		bline	bottom line of section.
**		utype	type of underlining.
**			ULS_NONE - none.
**			ULS_LAST - last line only.
**			ULS_ALL - all.
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
**		180, 186.
**
**	History:
**		5/4/82 (ps)	written.
**		1/26/83 (gac)	does not generate .TFORMAT for non-break attributes.
**		3/1/83 (gac)	handles quotes and backslashes in trim.
**		8/11/83 (gac)	fix bug 1314 -- for b format use width of 
**				format, not attribute.
**		22-aug-86 (mgw) Bug #9785
**				Print only the number of blanks in the field, 
**				not the column, for recurring values in break 
**				columns in .within's
**		1/29/87 (rld)	use fmt_size instead of f_fmtwid
**		16-sep-88 (sylviap) 
**				Added TITAN changes (4K).
**		07-oct-88 (sylviap) 
**				Use fdTRIMLEN instead of literal 256.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		01-sep-89 (cmr)
**			Changes to reflect new Sections data structure.
**		16-oct-89 (cmr)
**			Get rid of references to copt_whenprint.  Also
**			changed the argument list for rFm_sec().
**		22-nov-89 (cmr)
**			Support for editable aggregates.  Use Cs_length
**			rather than En_n_attribs when traversing the Cs_top
**			array so that aggs get processed too.  Also, look
**			at the field's flags to see if it is an agg
**			(fdDERIVED).  For aggs, use validation string for
**			rcommand text.
**		03-dec-90 (cmr)
**			Dynamically allocate arrays -- now that we support
**			aggregates the number of cols is unknown until runtime.
**		  2/20/90 (martym)   
**			Changed to return STATUS passed back by the 
**			SREPORTLIB routines.
**		07-apr-90 (cmr)
**			Save the aggregate # with the .begin rbfaggs text so
**			RBF can determine if heading and agg are associated.
**			Also write out a .top for aggs so that block-like aggs
**			get printed together.
**              17-apr-90 (sylviap)
**                      Changed parameter list so returns the maximum report 
**			width. Needed for .PAGEWIDTH support. (#129, #588)
**		04-jun-90 (cmr)
**			Put back in support for the B format.
**			Added MapToAggNum() to calculate the correct rbf agg #;
**			used to index into the Cs_top array on edit.
**		21-jul-90 (cmr) fix for bug 31731
**			Make underlining work properly.  Make maxline a single
**			i4 (there's no need for an array).  Also, minline
**			removed - not needed.
**              29-aug-90 (sylviap)
**			Got rid of maxwidth parameter. #32701.
**              08-oct-90 (sylviap)
**			Does not generate any .WITHIN's for label reports.
**			Label reports do not have any associtated (column 
**			headings) trim.  #33772.
**		07-aug-91 (steveh)
**			Changed comma operator to && operator in MapToAggNum.
**			Registerized several variables for speed.
**			(bug 39013)
**		30-oct-1992 (rdrane)
**			Remove all FUNCT_EXTERN's since they're already
**			declared in included hdr files.
**			Ensure that we ignore unsupported datatypes (of which
**			there shouldn't be any).  Ensure attribute names passed
**			to s_w_action in unnormalized form.  Ensure that all
**			string constants are generated with single, not double
**			quotes.
**		10-nov-1992 (rdrane)
**			Ensure that all attribute names are put into RW
**			statements in unnormalized form.
**		15-jan-1993 (rdrane)
**			Ensure that all attribute names are put into RW
**			statements in unnormalized form.  Hopefully we've
**			found all the occurances this time ...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/


GLOBALREF i4	*ag_array;

TAGID	rfmsec_tag = 0;
i4	maxline = 0;		/* largest line in section for each att */
LIST	**curf = NULL;		/* current field for each section */
LIST	**curt = NULL;		/* current trim for each section */

STATUS rFm_sec(tline,bline,utype,indetail)
i4	tline;
i4	bline;
char	utype;
bool	indetail;
{
	/* internal declarations */

	i4		i, ord, len;
	i4		cury, curtop, curbottom;
	Sec_node	*n;
	register LIST	*list;			/* fast ptr to a list */
	register ATT	*att;			/* ATT structure for att */
	FIELD		*f;			/* field */
	FLDHDR		*FDgethdr();
	TRIM		*t;			/* trim ptr */
	bool		firstone;		/* TRUE when .WITHIN started */
	bool		firstagg;		/* TRUE when RBFAGGS started */
	bool		aggfield;		/* TRUE if aggregate field */
	bool		online;			/* TRUE if line written on */
	i4		nskip;			/* number of lines to skip */
	char		trim[2*fdTRIMLEN + 1];		
						/* trim with quotes and 
						** backslashes preceded by */
	char		ibuf[20];
	char		agindex[16];
	i4		aggn;
	static		old_length = 0;
	bool		do_b_fmt;
	COPT		*copt;
	DB_DATA_VALUE	 adbv;
	i4		fmt_numrows, fmt_numcols, status;
	STATUS 		stat = OK;
	char		tmp_buf1[(FE_UNRML_MAXNAME + 1)];
	
/* start of routine */

#	ifdef	xRTR1
	if(TRgettrace(180,0) || TRgettrace(186,0))
	{
		SIprintf(ERx("rFm_sec: entry.\r\n"));
	}
#	endif

	if ((tline+1) >= bline)
	{	/* No section specified */

#		ifdef	xRTR2
		if (TRgettrace(186,0))
		{
			SIprintf(ERx("	No section specified.\r\n"));
		}
#		endif

		return(OK);
	}

	/*
	** Allocate arrays; can't use static arrays because we don't
	** know how many fields + aggregates there might be.
	*/
	if ( !rfmsec_tag || Cs_length > old_length )
	{
		if ( rfmsec_tag )
			FEfree( rfmsec_tag );
		rfmsec_tag = FEgettag();
		if ( 
			( (curf = (LIST**)FEreqmem( (u_i4)rfmsec_tag, 
				(Cs_length+1)*sizeof(LIST *), TRUE, 
					(STATUS *)NULL)) == NULL) ||
			( (curt = (LIST**)FEreqmem( (u_i4)rfmsec_tag,
				(Cs_length+1)*sizeof(LIST *), TRUE, 
					(STATUS *)NULL)) == NULL)
	   	)
		{
			IIUGbmaBadMemoryAllocation(ERx("rFm_sec"));
		}
		else
			old_length = Cs_length;
	}
	else
	{
		/* zero out the arrays */
		MEfill(	(old_length+1)*sizeof(LIST *),  '\0', curf );
		MEfill(	(old_length+1)*sizeof(LIST *),  '\0', curt );
	}

	/* First find the first trim and field and the
	** ending lines for each attribute and
	** for the unassociated trim in this section */

	rFfld_rng(&maxline, curf, curt, tline, bline);


	if ((bline-tline)>2)
	{
		CVna((bline-tline-1),ibuf);
		RF_STAT_CHK(s_w_action(ERx("need"),ibuf,(char *)0));
	}

	if (utype == ULS_ALL)
	{	/* underline everything */
		RF_STAT_CHK(s_w_action(ERx("underline"),(char *)0));
	}

	/* For each omnilinear section, go through the appropriate trim,
	** and the appropriate fields to convert to RW commands */

	curtop = tline + 1;
	nskip = 0;
	while(curtop < bline)
	{
		curbottom = omnilin(curtop);

#		ifdef	xRTR3
		if (TRgettrace(186,0))
		{
			SIprintf(ERx("	In omnilinear section.\r\n"));
			SIprintf(ERx("		Curtop: %d\r\n"),curtop);
			SIprintf(ERx("		Curbottom: %d\r\n"),curbottom);
			SIprintf(ERx("		nskip: %d\r\n"),nskip);
		}
#		endif

		if ((curbottom == curtop) || (curbottom == 0))
		{	/* simply a blank line */
			nskip++;
			curtop++;		/* go down one */
			continue;
		}

		/* Something on the line */

		curbottom--;			/* last line of section */
		if (nskip > 0)
		{	/* write out the newline commands */
			CVna(nskip,ibuf);
			RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
			nskip = 0;
		}

		RF_STAT_CHK(s_w_action(ERx("begin"), ERx("block"), (char *)0));

		/* Go through the CS linked lists, both associated
		** and unassociated, in this omnilinear block. */

		for (i=0; i<=Cs_length; i++)
		{	/* Next attribute.  (if 0, unassociated) */
			firstone = TRUE;
			firstagg = TRUE;
			aggfield = FALSE;
			cury = curtop;
			online = FALSE;

			for(list=curt[i]; list!=NULL; list=list->lt_next)
			{	/* scan the trim structures. */
				t = list->lt_trim;
				len = STlength (t->trmstr);

				if (t->trmy > curbottom)
				{	/* no more in this list */
					if (t->trmy > bline)
					{	/* no more in section */
						curt[i] = NULL;
					}
					curt[i] = list;
					break;
				}
				if (i > 0)
				{	/* associated fields/trim */
					f = Cs_top[i - 1].cs_flist->lt_field;
					if ((FDgethdr(f))->fhd2flags & fdDERIVED )
						aggfield = TRUE;
					if (!aggfield && firstone) 
					{
						ord = r_mtch_att(f->fldname);
						att = r_gt_att(ord);
						/* 
						** LABELS cannot have associated
						** fields/trim 
						*/
						if ((att != (ATT *)NULL) &&
						    (St_style != RS_LABELS))
						{
							_VOID_ IIUGxri_id(
							    att->att_name,
							    &tmp_buf1[0]);
							RF_STAT_CHK(s_w_action
							   (ERx("within"), 
							    &tmp_buf1[0], (char *)0));
							RF_STAT_CHK(s_w_action
							   (ERx("top"),(char *)0));
						}
						firstone = FALSE;
					}
				}
				if (t->trmy > cury)
				{	/* go to a new line */
					CVna((t->trmy-cury),ibuf);
					RF_STAT_CHK(s_w_action(ERx("newline"),
						ibuf,(char *)0));
					cury = t->trmy;
					online = FALSE;
				}
				if ((i <= 0) || aggfield)
				{	/* unassociated trim */
					if ( aggfield && firstagg )
					{
						STcopy(ERx("rbfaggs "), agindex);
						/*
						** Map i to the agg #; used to
						** index Cs_top array on edit.
						*/
						aggn = MapToAggNum(i);
						CVna(aggn,ibuf);
						STcat(agindex, ibuf);
						RF_STAT_CHK(s_w_action(
							ERx("begin"), 
							agindex, 
							(char *)0));
						RF_STAT_CHK(s_w_action(
							ERx("top"),(char *)0));
						firstagg = FALSE;
					}
					CVna(t->trmx, ibuf);
					RF_STAT_CHK(s_w_action(ERx("tab"),
						ibuf,(char *)0));
				}
				else
				{	/* associated.	Relative tab */
					RF_STAT_CHK(s_w_action(
						ERx("linestart"),(char *)0));
					if (t->trmx > att->att_position)
					{	/* not at left of column */
						CVna
						  (t->trmx-att->att_position,
						  ibuf);
						RF_STAT_CHK(s_w_action(
							ERx("tab"), ERx("+"),
						  	ibuf,(char *)0));
					}
				}

				if ((utype==ULS_LAST) && (cury>=maxline))
				{
					RF_STAT_CHK(s_w_action(
						ERx("underline"),(char *)0));
				}

				rF_bstrcpy(trim, t->trmstr);
				RF_STAT_CHK(s_w_action(ERx("print"), 
					ERx("\'"), trim,
				        ERx("\'"), (char *)0));
				RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));

				if ((utype==ULS_LAST) && (cury>=maxline))
				{
					RF_STAT_CHK(s_w_action(
						ERx("nounderline"),(char *)0));
				}

				online = TRUE;
			}
			if (list==NULL)
			{	/* no more in this section */
				curt[i] = NULL;
			}
			if (online)
			{	/* put out a newline for the last line */
				RF_STAT_CHK(s_w_action(ERx("newline"),(char *)0));
			}

			if (!firstone)
			{
				RF_STAT_CHK(s_w_action(ERx("top"),(char *)0));
			}

			/* Now go through the fields */

			cury = curtop;
			online = FALSE;

			for(list=curf[i]; list!=NULL; list=list->lt_next)
			{	/* scan the field structures. */
				f = list->lt_field;

				if ((FDgethdr(f))->fhd2flags & fdDERIVED)
					aggfield = TRUE;
				if (f->flposy > curbottom)
				{	/* no more in this list */
					if (f->flposy > bline)
					{	/* no more in section */
						curf[i] = NULL;
					}
					curf[i] = list;
					break;
				}
				if (i > 0) 
				{
					/* associated fields/trim */
					if (!aggfield && firstone) 
					{
						ord = r_mtch_att(f->fldname);
						att = r_gt_att(ord);
						/*
						** LABELS cannot have associated
						** fields/trim 
						*/
						if ((att != (ATT *)NULL) &&
						    (St_style != RS_LABELS))
						{
							_VOID_ IIUGxri_id(
							    att->att_name,
							    &tmp_buf1[0]);
							RF_STAT_CHK(s_w_action(
							   ERx("within"),
							   &tmp_buf1[0],(char *)0));
							RF_STAT_CHK(s_w_action(
							   ERx("top"),(char *)0));
						}
						firstone = FALSE;
					}
				}
				if (f->flposy > cury)
				{	/* go to a new line */
					CVna((f->flposy-cury),ibuf);
					RF_STAT_CHK(s_w_action(ERx("newline"),
						ibuf,(char *)0));
					cury = f->flposy;
					online = FALSE;
				}
				if (i <= 0 || aggfield)
				{	/* unassociated field */
					if ( aggfield && firstagg )
					{
						STcopy(ERx("rbfaggs "), agindex);
						/*
						** Map i to the agg #; used to
						** index Cs_top array on edit.
						*/
						aggn = MapToAggNum(i);
						CVna(aggn,ibuf);
						STcat(agindex, ibuf);
						RF_STAT_CHK(s_w_action(
							ERx("begin"), 
							agindex, 
							(char *)0));
						firstagg= FALSE;
					}
					if ( aggfield )
					{
						RF_STAT_CHK(s_w_action(
							ERx("top"),(char *)0));
					}
					CVna(f->flposx, ibuf);
					RF_STAT_CHK(s_w_action(ERx("tab"),
						ibuf,(char *)0));
				}
				else
				{	/* associated.	Relative tab */
					RF_STAT_CHK(s_w_action(
						ERx("linestart"),(char *)0));
					if (f->flposx > att->att_position)
					{	/* not at left of column */
						CVna
						  (f->flposx-att->att_position,
						  ibuf);
						RF_STAT_CHK(s_w_action(
							ERx("tab"), ERx("+"),
						  	ibuf,(char *)0));
					}
				}

				if ((utype==ULS_LAST) && (cury>=maxline))
				{
					RF_STAT_CHK(s_w_action(
						ERx("underline"),(char *)0));
				}

				if ( aggfield )
				{
					RF_STAT_CHK(s_w_action(
						ERx("print"), f->flvalstr, 
						ERx("("),f->flfmtstr, ERx(")"),
						(char *)0));
				}
				else
				{
				/* If in detail section, print the fieldname
				** with a "b" format if the fieldname is an
				** attribute, and the copt->copt_whenprint
				** is set to something other than 'a' */

					do_b_fmt = FALSE;
					ord = r_mtch_att(f->fldname);
					if (ord > 0 && indetail)
					{
						/* got one */
						copt = rFgt_copt(ord);
						if (copt->copt_break=='y' &&
						copt->copt_whenprint!='a')
							do_b_fmt = TRUE;
					}
					if (do_b_fmt)
					{
						att = r_gt_att(ord);
						if ((att != (ATT *)NULL) &&
						    (att->att_format != NULL))
						{
							adbv = att->att_value;
							/* get format width */
							status =  fmt_size(
								FEadfcb(),
								att->att_format,
								&adbv, 
								&fmt_numrows, 
								&fmt_numcols
								);
							if (status != OK)
						    		IIUGerr(
								E_RF0039_rFafield__Can_t_get_f, 
									UG_ERR_FATAL, 0);
							/* Begin fix for 9785 */
							CVna(fmt_numcols, ibuf);
							IIUGxri_id(
							    f->fldname,
							    &tmp_buf1[0]);
							RF_STAT_CHK(s_w_action(
								ERx("print"),
							   	&tmp_buf1[0],
								ERx("(b"), 
								ibuf, ERx(")"),
								(char *)0));
						}
					}
					else
					{
						IIUGxri_id(f->fldname,
							   &tmp_buf1[0]);
						RF_STAT_CHK(s_w_action( 
							ERx("print"),
							&tmp_buf1[0],(char *)0));
					}
				}
				RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));

				if ((utype==ULS_LAST) && (cury>=maxline))
				{
					RF_STAT_CHK(s_w_action(
						ERx("nounderline"),(char *)0));
				}

				online = TRUE;
			}
			if (list==NULL)
			{	/* no more in this section */
				curf[i] = NULL;
			}
			if (online)
			{	/* put out a newline for the last line */
				RF_STAT_CHK(s_w_action(ERx("newline"),(char *)0));
			}

			/* LABELS cannot have associated fields/trim */
			if ((i > 0) && (St_style != RS_LABELS) && !firstone) 
			{
				RF_STAT_CHK(s_w_action(ERx("end"), 
					ERx("within"), (char *)0));
			}
			if ((i>0) && !firstagg)
			{
				RF_STAT_CHK(s_w_action(ERx("end"), 
					ERx("rbfaggs"), (char *)0));
			}
		}
		
		RF_STAT_CHK(s_w_action(ERx("end"), ERx("block"), (char *)0));
		curtop = curbottom+1;
	}

	if (nskip > 0)
	{	/* write out any buffered newline commands */
		CVna(nskip,ibuf);
		RF_STAT_CHK(s_w_action(ERx("newline"),ibuf,(char *)0));
	}

	if (utype == ULS_ALL)
	{
		RF_STAT_CHK(s_w_action(ERx("end"), ERx("underline"), (char *)0));
	}

	return(OK);
}

/*
** MapToAggNum() - This routine takes an index, which is the index into the
**		   compressed Cs_top array (compressed by rfvifred/vftorbf()),
**		   and converts it into an index that can be used when the
**		   report is read back in, at which time the Cs_top array
**		   will be uncompressed.
*/
i4
MapToAggNum(index)
register i4	index;
{
	register i4  i;

	for (i = 0; i < Agg_count && ag_array[i] != 0; i++)
	{
		if (ag_array[i] == index)
			return(i+1);
	}
	ag_array[i] = index;
	return(i+1);
}

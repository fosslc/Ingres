/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfscncol.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"
#include	<ug.h>

/*
**   RFSCN_COL - scan the edited linked trim and fields associated with
**	each column and set the default position, width, format
**	and location.  The position will be set to the minimum
**	x, the width set to the maximum x - minimum x, the format
**	to the format in the first field found in the detail
**	section.  This also sets the St_right_margin and St_left_margin.
**	Skip aggregate fields.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		rFsave.
**
**	Side Effects:
**		Change values in the ATT structures.
**		Sets St_right_margin, St_left_margin.
**
**	Trace Flags:
**		180, 181.
**
**	Error Messages:
**		none.
**
**	History:
**		4/4/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**		changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	01-aug-89 (cmr)	Changed to reflect new Sections data struct.
**	22-nov-89 (cmr)	Only setup an ATT for regular fields not aggregates.
**			Use Cs_length not En_n_attribs when scanning the
**			Cs_top array; we need to look at all CS structs 
**			to calculate margins.
**	12/19/89 (dkh) - VMS shared lib changes.
**	24-jan-90 (cmr) Reset all att structs initially and rewrote main loop
**			to take into consideration that the Cs_top array is
**			compressed.
**	30-oct-1992 (rdrane)
**		Ensure that unsupported datatype fields are ignored.
**		Remove declaration of r_gt_att() since its already
**		already declared in included hdr files.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN	VOID	FTterminfo();

VOID
rFscn_col()
{
	/* internal declarations */

 	i4		COLS;
	i4		minx;		/* minimum x position */
	i4		maxx;		/* maximum x position */
	i4		attord;
	register ATT	*att;		/* fast ptr to ATT */
	CS		*cs;		/* ptr to CS for att */
	register LIST	*list;		/* fast ptr to list element */
	register i4	i;		/* counter */
	TRIM		*t;		/* pointer to trim */
	FIELD		*f;		/* pointer to field */
	bool		agg_flag;
	FLDHDR		*FDgethdr();
	FT_TERMINFO	terminfo;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(180,0) || TRgettrace(181,0))
	{
		SIprintf(ERx("rFscn_col: entry.\r\n"));
	}
#	endif

	FTterminfo(&terminfo);

 	COLS = terminfo.cols;

	St_left_margin = 0;
	St_right_margin = 0;		/* recalculate */

	for ( i = 1; i <= En_n_attribs; i++ )
	{
		att = r_gt_att(i);
		if  (att != (ATT *)NULL)
		{
			/* reset the defaults for this column */
			att->att_position = 0;
			att->att_width = 0;
			att->att_format = NULL;
			att->att_line = 0;
			att->att_brk_ordinal = 0;
		}
	}

	for (i=0; i < Cs_length; i++)
	{	/* check next attribute */
		minx = COLS;
		maxx= 0;
		agg_flag = FALSE;
		cs = &(Cs_top[i]);
#		ifdef	xRTR2
		if (TRgettrace(181,0))
		{
			SIprintf(ERx("	Next att %d in rRscn_col.\r\n"),i);
			r_pr_att(i);
		}
#		endif

		/* first scan the linked fields */

		for (list=cs->cs_flist; list!=NULL; list=list->lt_next)
		{
			if ((f=list->lt_field) == NULL)
			{
				continue;
			}
			if ((FDgethdr(f))->fhd2flags & fdDERIVED )
			{
				agg_flag = TRUE;
			}
			else
			{
				attord = r_mtch_att(f->fldname);
				att = r_gt_att( attord );
				if  (att == (ATT *)NULL)
				{
					continue;
				}
				if (att->att_format==NULL)
				{
					att->att_format = f->flfmt;
				}
			}
			minx = min(minx,(f->flposx+f->fldatax));
			maxx = max(maxx,(f->flposx+f->fldatax+f->fldataln));
		}

		/* Next scan the linked trim */

		for (list=cs->cs_tlist; list!=NULL; list=list->lt_next)
		{
			if ((t=list->lt_trim) == NULL)
			{
				continue;
			}
			minx = min(minx,t->trmx);
			maxx = max(maxx,(t->trmx+STlength(t->trmstr)));
		}

		if ( !agg_flag )
		{
			att->att_position = minx;
			att->att_width = maxx - minx;
		}
		St_right_margin = max(St_right_margin, maxx);

#		ifdef	xRTR3
		if (TRgettrace(181,0))
		{
			SIprintf(ERx("	End of loop in rFscn_col.\r\n"));
			r_pr_att(i);
		}
#		endif

	}

	/* Now scan the Other lists for right margin */

	for (list=Other.cs_flist; list!=NULL; list=list->lt_next)
	{
		f = list->lt_field;
		St_right_margin = max(St_right_margin,
				      (f->flposx+f->fldatax+f->fldataln-1));
	}

	/* Next scan the linked trim */

	for (list=Other.cs_tlist; list!=NULL; list=list->lt_next)
	{
		t = list->lt_trim;
		St_right_margin = max(St_right_margin,
				      (t->trmx+STlength(t->trmstr)-1));
	}

#	ifdef	xRTR3
	if (TRgettrace(181,0))
	{
		SIprintf(ERx("	At end of rFscn_col.  RM:%d\n"),
			 St_right_margin);
	}
#	endif

	return;
}

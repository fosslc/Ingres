/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<vt.h>
# include	<fe.h>
# include	<frame.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**	History:
**		1/85	(ac)	Created from codes in rfmlayout.qc for
**				modularization.
**		1/21/85 (rlk) - Changed ME calls to FE calls.
**		1/24/85 (rlk) - Integrated FT code.
**		10-sep-87 (bruceb)
**			Removed define of ERR; not needed.
**		08/18/88 (tom) - Fixed bug causing default reports
**				larger than the screen to overlay the right
**				margin by one char.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		22-nov-89 (cmr)	index into the Cs_top array instead of using
**				rFget_cs().  We need to handle aggregates too
**				and rFget_cs() is only for regular fields.
**	12/19/89 (dkh) - VMS shared lib changes.
**              2/13/90 (martym) 
**                      Fixed JB #6505, by not adding 1 whenever the  
**                      value of room is calculated.
**	17-apr-90 (bruceb)
**		Removed first argument in calls on FDwflalloc().  (lint)
**	17-aug-90 (sylviap)
**		Sets the right margin at the En_lmax setting for label style
**		reports.  This allows centering of the report title for label
**		reports.  (#31446)
**	29-aug-90 (sylviap)
**		If editing a report, then set the right margin to the page size.
**		#32701
**	19-sep-90 (sylviap)
**		Need to increment frmaxx by 2 because for FT3270, (rf)vifred is
**		allowing two bytes for the end of report marker (right margin).
**	 2-jan-91 (steveh)
**		Fixed Bug 34842 in which margins were not remembered across 
**		edits of a report.  This was fixed by priming cols with the 
**		width of the report.
**    20-may-90 (steveh)
**		Modified fix to bug 36997. This also fixes bugs 37574 and 
**		37671.
**		Added return type to rfframe().
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Aug-2009 (kschendel) 121804
**	    Need vt.h to satisfy gcc 4.3.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN	VOID	FTterminfo();

i4
rfframe(ntrims, nfields)
i4	ntrims;
i4	nfields;
{
	CS		*cs;		/* ptr to cs */
	i4		i;		/* ptr */
	register LIST	*l;		/* fast ptr to linked lists */
	register FIELD	**f;		/* fast ptr */
	register TRIM	**t;		/* fast prt */
	i4		posx;
	i4		endx;
	i4		cols;
	i4		room;
	i4		len;

	FT_TERMINFO	terminfo;

	FRAME		*FDnewfrm();	/* GAC -- added cast */

	FTterminfo(&terminfo);

	/* see note below about adding 1 to cols.. we don't add it
	   here.. but it will be added later */
	cols = terminfo.cols;

	/*	Start a tag region for the dynamically allocated frame */

#	ifdef CFEVMSTAT
	FEgetvm(ERx("rFmlayout:  Before Layout frame allocation"));
#	endif

	Rbfrm_tag = FEbegintag();


	/* Allocate the parts of the FRAME structure we know */

	Rbf_frame = FDnewfrm(En_report);
	Rbf_frame->frfldno = nfields;
	Rbf_frame->frtrimno = ntrims;
	Rbf_frame->frmaxx = cols;
	Rbf_frame->frmaxy = Cury;
	Rbf_frame->frposx = Rbf_frame->frposy = 0;

/*
**  Using VT interface. (dkh)
**
**	if(FDfralloc(Rbf_frame) == FALSE || vfundoalloc(Rbf_frame) == FALSE)
*/

	if (FDfralloc(Rbf_frame) == FALSE)
	{
#		ifdef	xRTR1
		if (TRgettrace(221,0))
		{
			SIprintf(ERx("	rFmlayout: Can't allocate Rbf_frame.\r\n"));
			SIprintf(ERx("		Rbf_frame add:%p\r\n"),Rbf_frame);
		}
#		endif

		FEendtag();
		return(LAYOUTERR);
	}

	f = Rbf_frame->frfld;
	t = Rbf_frame->frtrim;

#	ifdef	xRTR3
	if (TRgettrace(221,0))
	{
		SIprintf(ERx("	In rFmlayout. After first FDfralloc.\r\n"));
		FDfrmdmp(Rbf_frame);
	}
#	endif

	/* Now add in the TRIM and FIELD pointers */

	for (l=Other.cs_tlist; l!=NULL; l=l->lt_next,t++)
	{	
		/* add one of the TRIM's to the FRAME */

		*t = l->lt_trim;
 		room = cols - (*t)->trmx;
		len = STlength((*t)->trmstr);
 		if (len > room)
		{
			cols += (len - room);
		}
	}

	for (l=Other.cs_flist; l!=NULL; l=l->lt_next)
	{
		posx = l->lt_field->flposx;
		endx = posx + l->lt_field->flmaxx;
		if (endx > cols)
		{
			cols = endx;
		}
		*f++ = l->lt_field;
		if (FDwflalloc(l->lt_field, NOWIN) == FALSE)
		{
#			ifdef	xRTR1
			if (TRgettrace(221,0))
			{
				SIprintf(ERx("	rFmlayout: Can't allocate unassoc field.\r\n"));
				SIprintf(ERx("		field add: %p\r\n"), l->lt_field);
			}
#			endif

			FEendtag();
			return(LAYOUTERR);
		}
	}

	for (i=1; i<=Cs_length; i++)
	{	/* go through CS structures */

		cs = &(Cs_top[i - 1]);
		if (cs == NULL)
			continue;

		for (l=cs->cs_flist; l!=NULL; l=l->lt_next)
		{
			posx = l->lt_field->flposx;
			endx = posx + l->lt_field->flmaxx;
			if (endx > cols)
			{
				cols = endx;
			}
			*f++ = l->lt_field;
			if (FDwflalloc(l->lt_field,NOWIN) == FALSE)
			{
#				ifdef	xRTR1
				if (TRgettrace(221,0))
				{
					SIprintf(ERx("	rFmlayout: Can't allocate assoc field.\r\n"));
					SIprintf(ERx("		field add: %p\r\n"), l->lt_field);
				}
#				endif

				FEendtag();
				return(LAYOUTERR);
			}
		}

		for (l=cs->cs_tlist; l!=NULL; l=l->lt_next, t++)
		{
			*t = l->lt_trim;
 			room = cols - (*t)->trmx;
			len = STlength((*t)->trmstr);
 			if (len > room)
			{
				cols += (len - room);
			}
		}
	}

	/* 
	** The addition of 2 below compensates for the fact that 
	** you need to make space for the right margin (END line)
	** For FT3270, it needs to save off two bytes, one for the
	** line, the other for an attribute byte (not sure what that is tho!)
	** RBF needs to be consistent w/VIFRED.
	*/

	/* 
	** if editing an existing 6.4 report, set the right margin to the 
	** page size. Pre 6.4 reports will have En_pg_width set to NULL.
	** #32701 (steveh: additional change fixes bugs 36997, 37574, and 
	** 37671).
	*/
	if ((*En_pg_width != EOS) &&
		((En_SrcTyp == NoRepSrc) ||
		(Rbf_ortype == OT_EDIT)	 ||
		(Rbf_ortype == OT_DUPLICATE)))
	{
		Rbf_frame->frmaxx = En_lmax + 2;
	}
	else
	{
		if (St_style == RS_LABELS)
			Rbf_frame->frmaxx = En_lmax_orig + 2;
		else
			Rbf_frame->frmaxx = cols + 2;
	}

	if (!VTnewfrm(Rbf_frame))
 	{
		FEendtag();
 		return(LAYOUTERR);
 	}

#	ifdef	xRTR3
	if (TRgettrace(221,0))
	{
		SIprintf(ERx("	In rFmlayout after all fields.\r\n"));
		FDfrmdmp(Rbf_frame);
	}
#	endif

	/*	End the tag region for the dynamically allocated frame */

	FEendtag();

#	ifdef CFEVMSTAT
	FEgetvm(ERx("rFmlayout:  After Layout frame allocation"));
#	endif

	return(LAYOUTOK);
}

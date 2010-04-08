/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfatrim.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFATRIM - add a TRIM structure to the frame.  This uses the
**	information in Curtop, Curx, Cury, Curatt to find out
**	where the TRIM is to go.  It then adds a pointer to 
**	the TRIM structure into the CS array or the Other list.
**	If this trim is associated with an aggregate field then use
**	Curaggregate to calculate the index into the CS array.
**
**	Parameters:
**		string		trim to add.  A copy will be made.
**		agg_flag	TRUE if this trim is associated with an
**				aggregate field.
**
**	Returns:
**		address of TRIM structure created.  NULL on error.
**
**	Trace Flags:
**		200, 211.
**
**	Error Messages:
**		none.
**
**	Side Effects:
**		Updates the Cs structure or the Otrim list.
**
**	History:
**		2/14/82 (ps)	written.
**		1/85	(ac)	Initialized lt_utag to DS_PTRIM for
**				DS routines.
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**		22-nov-89 (cmr)
**			Added bool flag to indicate whether this trim
**			is associated with an aggregate or not.
**		14-sep-90 (sylviap)
**			If the trim is in the detail section of a label
**			report, make it unassociated trim, not a column
**			heading. Added in_detail. (#32744)
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



TRIM	*
rFatrim(string, agg_flag, in_detail)
char	*string;
bool	agg_flag;
bool	in_detail;		/* TRUE = processing in detail section */
{
	/* internal declarations */

	register TRIM	*trim;			/* fast trim ptr */
	char		*tstring;		/* trim string */
	register LIST	*list;			/* ptr to list to contain TRIM */
	CS		*cs;			/* CS structure, while .WITHIN */
	register LIST	*olist;			/* used in linking list */
	TRIM		*FDnewtrim();		/* GAC -- added cast */
	i4		index;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0) || TRgettrace(211,0))
	{
		SIprintf(ERx("rFatrim: entry.\n"));
	}
#	endif

	if (string == NULL)
	{
		return(NULL);
	}

#	ifdef	xRTR2
	if (TRgettrace(211,0))
	{
		SIprintf(ERx("	string:%s:\n"),string);
		SIprintf(ERx("	Curx:%d\n"),Curx);
		SIprintf(ERx("	Cury:%d\n"),Cury);
		SIprintf(ERx("	Curordinal:%d\n"),Curordinal);
	}
#	endif

	/* allocate the TRIM string */

	tstring = (char *) STalloc(string);
	STtrmwhite(tstring);
	if (STlength(tstring)<1)
	{
		return(NULL);
	}

	trim = FDnewtrim(tstring, Cury, Curx);

#	ifdef	xRTR2
	if (TRgettrace(211,0))
	{
		SIprintf(ERx("	In rFatrim before linking.\n"));
		FDtrmdmp(trim);
	}
#	endif

	list = lalloc(LTRIM);
	list->lt_trim = trim;
	list->lt_utag = DS_PTRIM;

	/* find out what it is associated with */

	olist = NULL;
	if ( agg_flag )
	{
		index = En_n_attribs + Curaggregate;
		cs = &(Cs_top[index - 1]);
	}
	else
		/* 
		** Checking for column headings.  If processing trim in the
		** detail section of a label report, then ALL trim is 
		** unassociated. (#32744) 
		*/
		if ((Curordinal>0)&&(Curordinal<=En_n_attribs) &&
		   ((!in_detail) || (St_style != RS_LABELS)))
		{	/* associated with a column */
			cs = &(Cs_top[Curordinal-1]);
		}
		else
		{	/* not associated with a column */
			cs = &Other;
		}

	if((olist=cs->cs_tlist) == NULL)
	{
		cs->cs_tlist = list;
	}
	/* if a list exists, add to the end of the existing one */

	if (olist!=NULL)
	{
		while (olist->lt_next != NULL)
		{	/* get to end of list */
			olist = olist->lt_next;
		}
		olist->lt_next = list;
	}

#	ifdef	xRTR2
	if (TRgettrace(211,0))
	{
		SIprintf(ERx("	At end of rFatrim.\n"));
		SIprintf(ERx("		Previous list add:%p\n"),olist);
		if (olist!=NULL)
		{
			SIprintf(ERx("			lt->next:%p\n"),olist->lt_next);
			rFpr_list(olist,LTRIM,FALSE);
		}
		SIprintf(ERx("		New list add:%p\n"),list);
		if (list!=NULL)
		{
			SIprintf(ERx("			lt->next:%p\n"),list->lt_next);
			rFpr_list(list,LTRIM,FALSE);
		}
	}
#	endif

	return(trim);
}

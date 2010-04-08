/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rffldrng.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFFLD_RNG - find the range of locations for the fields and
**	trim within a given section of the report.  This goes
**	through the list of trims and fields (regular and aggregate)
**	and figures out the minimum and maximum lines on which the 
**	components lie.  It also finds the first TRIM and FIELD 
**	structures within that section.
**
**	All of the arrays which are passed to this routine are
**	filled in by this routine.  Each are big enough to
**	hold all the fields (Cs_length).  Element 0 in 
**	each array is reserved for unassociated trim and fields.
**
**	Parameters:
**				will contain the minimum line
**				in this section which will contain
**		maxline		last line in section with a fld or trim
**		minfld		list of *FIELDs which will contain
**				pointers to the first FIELD encountered
**				in this section (important because they
**				are ordered).
**		mintrm		list of *TRIMs which will contain pointers
**				to the first trims encountered.
**		tline		top line in section.
**		bline		bottom line in section.
**		
**	Returns:
**		fills in arrays minfld, mintrm.  Sets maxline.
**
**	Called by:
**		rFm_sec.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		186.
**
**	History:
**		10/16/82 (ps)	written.
**		10/21/83 (nml)  Put in GAC's fix for bug #1632 - now deleting
**				columns doesn't cause A.V.
**      	9/22/89 (elein)	UG changes ingresug change #90045
**				changed <rbftype.h> & <rbfglob.h> to "rbftype.h" 
**				& "rbfglob.h"
**		22-nov-89 (cmr)	Use Cs_length instead of En_n_attribs when
**				scanning the Cs_top array so that aggregates
**				get included too.
**		21-jul-90 (cmr)	fix for bug 31731
**				Make underlining work properly - set maxline
**				correctly.  Also removed minline - not needed.
**		04-dec-90 (steveh) corrected computation of *maxline so as to
**				fix bug 34298. *maxline must be the maximum over
**				*all* fields.  Bug 34298 was caused by the
**				fact that *maxline was incorrectly computed as
**				the maximum on the *last* field.
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
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
rFfld_rng(maxline,minfld,mintrm,tline,bline)
i4	*maxline;
LIST	**minfld;
LIST	**mintrm;
i4	tline;
i4	bline;
{
	/* internal declarations */

	register i4	i;		/* fast counter */
	register FIELD	*f;		/* faster ptr to field */
	TRIM		*t;		/* faster ptr to trim */
	register LIST	*list;		/* faster ptr to linked list */
	CS		*cs;		/* pointer to CS for att */

	/* start of routine */

	/* prime *maxline */
	*maxline = tline;

	for (i=0; i<=Cs_length; i++, minfld++, mintrm++)
	{	/* next attribute */

		/* *maxline must be computed as the maximum 
		   over *all* fields. Bug 34298 (steveh)       */
		*maxline = *maxline > tline ? *maxline : tline;

		*minfld = NULL;
		*mintrm = NULL;

		if (i > 0)
		{
			cs = &(Cs_top[i - 1]);
			if (cs == NULL)
				continue;		/* no fields or trim */
		}
		else
		{
			cs = &Other;		/* the unassociated */
		}

		/* First go through the list of trim */

		for (list=cs->cs_tlist; list!=NULL; list=list->lt_next)
		{	/* check next associated trim */
			t = list->lt_trim;
			if (t->trmy <= tline)
			{	/* in previous section */
				continue;
			}
			if (t->trmy >= bline)
			{	/* in subsequent section */
				break;
			}
			if (t->trmy > *maxline)
			{	/* new maximum line */
				*maxline = t->trmy;
			}
			if (*mintrm == NULL)
			{	/* first trim encountered */
				*mintrm = list;
			}
		}

		/* Next check the associated fields */

		for (list=cs->cs_flist; list!=NULL; list=list->lt_next)
		{	/* check next associated trim */
			f = list->lt_field;
			if (f->flposy <= tline)
			{	/* in previous section */
				continue;
			}
			if (f->flposy >= bline)
			{	/* in subsequent section */
				break;
			}
			if (f->flposy > *maxline)
			{	/* new maximum line */
				*maxline = f->flposy;
			}
			if (*minfld == NULL)
			{	/* first field encountered */
				*minfld = list;
			}
		}

#		ifdef	xRTR3
		if (TRgettrace(186,0))
		{
			SIprintf(ERx("	After finding maxes in rFfld_rng: %d\r\n"),i);
			SIprintf(ERx("		Maxline: %d\r\n"),*maxline);
			SIprintf(ERx("		Min trim: %p\r\n"),*mintrm);
			SIprintf(ERx("		Min field: %p\r\n"),*minfld);
		}
#		endif
	}

	return;
}

/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfmstruct.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFMSTRUCT - Build the SRT array from the COPT structure.
**
**	This checks out the SRT array to fill in the COPT structures
**	for current sort attributes.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called By:
**		rFdisplay.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		200, 203, 205.
**
**	Error Messages:
**		none.
**
**	History:
**		5/27/82 (ps)	written.
**		2/7/83 (gac)	added validation of sequence, order, and break fields.
**		3/14/83 (gac)	changed structure.h to program.
**		11/27/84 (rlk) - Re-wrote using tablefields.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		16-oct-89 (cmr)	alloc COPT structs here. Also removed
**				references to copt_whenprint & copt_skip.
**		08-jan-90 (cmr) set defaults for all copts.
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
rFmstruct()
{
	/* internal declarations */

	i2	i;		/* used as counter */
	SORT	*sort;		/* ptr into SORT array for attribute */
	COPT	*copt;		/* ptr to COPT for sort attribute */

#	ifdef	xRTR1
	if (TRgettrace(200,0) || TRgettrace(203,0) || TRgettrace(205,0))
	{
		SIprintf(ERx("rFmstruct: entry.\r\n"));
	}
#	endif

	Copt_tag = FEbegintag();
	if ((Copt_top = (COPT *)FEreqmem ((u_i4)0, 
		(u_i4)(En_n_attribs * (sizeof(COPT))), TRUE, 
		(STATUS *)NULL)) == NULL)
        {
		IIUGbmaBadMemoryAllocation(ERx("rFmstruct"));
	}
	FEendtag();

	for ( i = 1; i <= En_n_attribs; i++ )
	{
		copt = rFgt_copt( i );
		rFclr_copt( copt );
	}

	/* Now fill in the fields for the current Sort attributes */

	for(i=1; i<=En_n_sorted; i++)
	{	/* fill in the COPT structure for sort attributes */
		sort = &Ptr_sort_top[i-1];
		copt = rFgt_copt(sort->sort_ordinal);
		copt->copt_sequence = i;
		copt->copt_order = (*(sort->sort_direct)=='d') ? 'd' : 'a';
		copt->copt_break = (*(sort->sort_break)=='y') ? 'y' : 'n';
	}

	return;
}

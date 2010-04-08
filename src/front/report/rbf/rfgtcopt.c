/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfgetcopt.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFGET_COPT - given an ordinal of an attribute from the data relation,
**	return the COPT structre.
**
**	Parameters:
**		ordinal		ordinal of attribute.
**
**	Returns:
**		COPT structure of the attribute with the given ordinal.
**		If the ordinal is bad, return NULL.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		200, 210.
**
**	History:
**		8/27/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



COPT  *
rFgt_copt(ordinal)
i4	ordinal;
{
	/* internal declarations */

	COPT		*copt;			/* ptr to return struct */

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0) || TRgettrace(210,0))
	{
		SIprintf(ERx("rFgt_copt:entry.\n"));
	}
#	endif

#	ifdef	xRTR2
	if (TRgettrace(210,0))
	{
		SIprintf(ERx("	ordinal:%d\n"),ordinal);
	}
#	endif

	if((ordinal<=0) || (ordinal>En_n_attribs))
	{
		copt = NULL;
	}
	else
	{
		copt = &(Copt_top[ordinal-1]);
	}

#	ifdef	xRTR2
	if (TRgettrace(210,0))
	{
		SIprintf(ERx("	rFgt_copt return add:%p\n"),copt);
	}
#	endif

	return (copt);
}

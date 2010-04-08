/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfgetcs.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFGET_CS - given an ordinal of an attribute from the data relation,
**	return the CS structre.
**
**	Parameters:
**		ordinal		ordinal of attribute.
**
**	Returns:
**		CS structure of the attribute with the given ordinal.
**		If the ordinal is bad or has none, return NULL.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		200, 210.
**
**	History:
**		2/17/82 (ps)	written.
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



CS  *
rFgt_cs(ordinal)
i4	ordinal;
{
	/* internal declarations */

	register CS	*cs;			/* ptr to return struct */
	register LIST	*list;			/* ptr to list in it */
	i4		i;			/* counter */
	i4		attord;	
	register FIELD	*f;			/* ptr to field */

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0) || TRgettrace(210,0))
	{
		SIprintf(ERx("rFgt_cs:entry.\n"));
	}
#	endif

	if((ordinal<=0) || (ordinal>En_n_attribs))
	{
		cs = NULL;
	}
	else
	{	/* check through the list to find the attribute name */
		for (i=0,cs=Cs_top; i<Cs_length; i++,cs++)
		{	/* check next CS structure */
			cs = &(Cs_top[i]);

#			ifdef	xRTR3
			if (TRgettrace(210,0))
			{
				SIprintf(ERx("	Checking CS ord:%d: add:%p.\r\n"),
					i,cs);
				rFpr_cs(cs);
			}
#			endif

			for (list=cs->cs_flist; list!=NULL; list=list->lt_next)
			{	/* check next field to find attribute name */
				if ((f=list->lt_field) == NULL)
					continue;
				attord = r_mtch_att(f->fldname);
				if (attord == ordinal)
					return(cs);
			}
		}
		cs = NULL;
	}

#	ifdef	xRTR2
	if (TRgettrace(210,0))
	{
		SIprintf(ERx("	rFgt_cs return add:%p\n"),cs);
	}
#	endif

	return (cs);
}

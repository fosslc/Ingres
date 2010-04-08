/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfcmpcs.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFCMP_CS - compress the CS array, taking out any that have no
**	fields in them.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Compresses the CS array and maybe changes Cs_length.
**
**	Called by:
**		rfdisplay.
**
**	History:
**		9/15/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
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
rFcmp_cs()
{
	/* internal declarations */

	register CS	*cs,*cstemp;		/* fast ptrs */
	i4		ncs,ncstemp;

	/* start of routine */

	for(ncs=0; ncs<Cs_length; ncs++)
	{	/* note that Cs_length may change in loop */
		if (Cs_top[ncs].cs_flist == NULL)
		{	/* doesn't have anything.  Get rid of it */
			for (ncstemp=ncs+1; ncstemp<Cs_length; ncstemp++)
			{	/* move CS one to the left */
				cs = &Cs_top[ncstemp-1];
				cstemp = &Cs_top[ncstemp];
				cs->cs_flist = cstemp->cs_flist;
				cs->cs_tlist = cstemp->cs_tlist;
				cs->cs_name = cstemp->cs_name;
			}
			Cs_length--;
			/*
			** BUG 967
			*/
			ncs--;
		}
	}
}
				

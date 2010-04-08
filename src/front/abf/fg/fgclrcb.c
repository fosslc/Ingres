/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include       <abclass.h>
# include       <metafrm.h>

/**
** Name:	fgclrcb.c	- Clear COL_TYPECHANGE bits
**
** Description:
**	This file defines:
**
**	IIFGccbClearChangeBits - Clear COL_TYPECHANGE bits
**
** History:
**	8/10/89 (Mike S)        Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN STATUS      IIAMmuMetaUpdate();

/* static's */
/*{
** Name:	IIFGccbClearChangeBits - Clear COL_TYPECHANGE bits
**
** Description:
**	This routine is called after we've successfullly generated a form,
**	either brand-new or fixed-up.  We clear all the COL_TYPECHANGE bits
**	for the metaframe, and save it to the database, if any bits
**	were set in the first place.
**
** Inputs:
**	metaframe	METAFRAME *	metaframe for current frame
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**	The Visual Query may be written to the database.
**
** History:
**	8/10/89 (Mike S)	Initial version.
*/
VOID
IIFGccbClearChangeBits(metaframe)
METAFRAME *metaframe;
{
	register MFTAB 	*tbl;	/* Table pointer */
	register MFCOL	*col;	/* Column pointer */
	i4 changes = 0;	/* Number of bits cleared */
	i4 i, j;

	/* Clear all the COL_TYPECHANGE bits */
	for ( i = 0; i < metaframe->numtabs; i++)
	{
		tbl = metaframe->tabs[i];
		for (j = 0; j < tbl->numcols; j++)
		{
			col = tbl->cols[j];
			if ((col->flags & (COL_TYPECHANGE)) != 0)
			{
				col->flags &= ~(COL_TYPECHANGE);
				changes++;	
			}
		}
	}

	/* If any were cleared, save the VQ to the database */
	if (changes != 0)
		_VOID_ IIAMmuMetaUpdate(metaframe, MF_P_VQ);
}

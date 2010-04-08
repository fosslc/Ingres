/*
**	newtrim.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	newtrim.c - Allocate a new trim structure.
**
** Description:
**	Allocate a new trim structure and the trim string as well.
**
** History:
**	02/04/85 (???) - Version 3.0 work.
**	02/15/87 (dkh) - Added header.
**	25-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h"
# include	<er.h>



/*{
** Name:	FDnewtrim - Allocate new trim structure.
**
** Description:
**	This routine allocates the structure for trim to be placed
**	on the frame. It then fills this structure with the
**	information necessary for placing it on the frame.
**	A trim can only consist of one line of 50 characters maximum.
**	If you wish to place more than one line on a frame, you
**	should use more than one trim string.
**	This routine may be called by VIFRED/QBF directly for default
**	form building, etc.
**
** Inputs:
**	trmstr	String to place on the frame.
**	y	line to place string.
**	x	column to place string.
**
** Outputs:
**	Returns:
**		ptr	Point to the allocated TRIM structure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	JEN  -  1 Nov 1981  (written)
**	12-20-84 (rlk) - Replace {ME|ST}*alloc calls the FE{s}*alloc.
**	02/15/87 (dkh) - Added procedure header.
*/
TRIM *
FDnewtrim(trmstr, y, x)			/* FDNEWTRIM: */
reg char	*trmstr;			/* trim string */
reg i4		y, x;				/* trim coordinates */
{
    TRIM	*trmall;
    reg TRIM	*trim;				/* pointer to trim struct */

    FUNC_EXTERN	char	*FEsalloc();

						/* allocate trim struct */
    if ((trmall = (TRIM *)FEreqmem((u_i4)0, (u_i4)(sizeof(TRIM)), TRUE,
	(STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("FDnewtrim-1"));
	return(NULL);
    }
    trim = trmall;
						/* allocate trim string */
    if((trim->trmstr = FEsalloc(trmstr)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("FDnewtrim-2"));
	return(NULL);
    }

    trim->trmy = y;				/* set line number */
    trim->trmx = x;				/* set column number */
    return(trim);
}

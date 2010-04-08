/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>


/**
** Name:	vtdelfrm.c -	Clean up memory connected with a frame.
**
** Description:
**	Routines defined are:
**		IIVTdfDelFrame		Free memory associated with a frame
**
** History:
**	31-may-89 (bruceb)	Written
**/

/*{
** Name:	IIVTdfDelFrame	- Free memory associated with a frame
**
** Description:
**	Free up memory associated with the passed in frame.  Also NULLs
**	the frame's pointer.
**
**	Currently frees up the frscr and untwo windows for the frame.
**	Requires that calling routines handle freeing the frame itself
**	since this is frequently FEreqmem'd so must be tag free'd.
**
** Inputs:
**	frm	Pointer to FRAME pointer to be free'd.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	31-may-89 (bab)	Written.
*/

VOID
IIVTdfDelFrame(frm)
FRAME	**frm;
{
    if (*frm != NULL)
    {
	TDdelwin((*frm)->frscr);
	TDdelwin((*frm)->untwo);
	*frm = NULL;
    }
}

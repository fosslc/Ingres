/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<gtdef.h>

extern bool G_idraw;
extern i4  (*G_mfunc)();

/*
**	these routines allow things outside GT to bracket several
**	GT drawing calls to avoid mode resets.
**
**  History:
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GTxactbeg ()
{
	(*G_mfunc)(GRAPHMODE);
	G_idraw = TRUE;
}

GTxactend ()
{
	(*G_mfunc)(ALPHAMODE);
	G_idraw = FALSE;
}

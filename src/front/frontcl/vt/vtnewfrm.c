/*
**  vtnewfrm.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	01-oct-87 (bab)
**		Allow for operation when top rows of the terminal
**		made unavailable for use by the forms system.
**	26-may-92 (rogerf) DeskTop Porting Change:
**		Added scrollbar processing inside SCROLLBARS ifdef.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"
# include	<si.h>
# include	<er.h>
# ifdef SCROLLBARS
# include	<wn.h>
# endif

bool
VTnewfrm(frm)
FRAME	*frm;
{
	i4	i;
	TRIM	**frtrim;
	TRIM	*trim;

	if ((frm->frscr = TDnewwin(frm->frmaxy, frm->frmaxx, (i4)0,
		(i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTnewfrm-1"));
		return(FALSE);
	}

# ifndef	WSC
	/* leave window coordinates (see TERMDR) */
	frm->frscr->lvcursor = FALSE;

	/* Do not clear screen first time through */
	frm->frscr->_clear = FALSE;

	frm->frscr->_cury = 0;
	frm->frscr->_curx = 0;
	frm->frscr->_starty = IITDflu_firstlineused;
# else
	((struct _win_st *)(frm->frscr))->_clear = FALSE;
	((struct _win_st *)(frm->frscr))->lvcursor = FALSE;
	((struct _win_st *)(frm->frscr))->_clear = FALSE;
	((struct _win_st *)(frm->frscr))->_cury = 0;
	((struct _win_st *)(frm->frscr))->_curx = 0;
	((struct _win_st *)(frm->frscr))->_starty = IITDflu_firstlineused;
# endif
	if ((frm->untwo = TDnewwin(frm->frmaxy, frm->frmaxx, (i4)0,
		(i4)0)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTnewfrm-2"));
		return(FALSE);
	}
# ifndef	WSC
	frm->untwo->lvcursor = TRUE;
	frm->untwo->_starty = IITDflu_firstlineused;
# else
	((struct _win_st *)(frm->untwo))->lvcursor = TRUE;
	((struct _win_st *)(frm->untwo))->_starty = IITDflu_firstlineused;
# endif
	for (i = 0, frtrim = frm->frtrim; i < frm->frtrimno; i++)
	{
		trim = *frtrim++;
		TDmvstrwadd(frm->frscr, trim->trmy, trim->trmx, trim->trmstr);
	}

#ifdef SCROLLBARS
	WNFbarCreate(FALSE, 		   /* not on "real" form */
			0,                 /* current row */
        		frm->frmaxy,       /* max form rows */
			0,                 /* current column */
			frm->frmaxx);      /* max form columns */
#endif /* SCROLLBARS */

    return(TRUE);
}


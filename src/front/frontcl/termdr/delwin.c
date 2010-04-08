# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<me.h>

/*
**  This routine deletes a window and releases it back to the system.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  delwin.c
**
**  History:
**	25-jun-87 (bab)	Code cleanup.
**	03/27/90 (fraser) - TDdelwin now uses functions IITD1dDelwin()
**			    and IITD2dDelwin().
**	05/04/90 (dkh) - Integrated changes from PC into 6.4 code line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





/*{
** Name:	IITD1dDelwin - Delete (free) memory for a window - step 1.
**
** Description:
**	This routine simply deletes the various arrays used
**	in the window structure.
**
** Inputs:
**	win	Pointer to window structure to free memory from.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	03/27/90 (fraser) - Initial version.
**	05/02/90 (dkh) - Moved to this file.
*/
VOID
IITD1dDelwin(win)
WINDOW	*win;
{
	MEfree((PTR) win->_y);
}



/*{
** Name:	IITD2Delwin - Delete (free) memory for a window - step 2.
**
** Description:
**	This routine deletes the character cell memory allocated
**	for a window.
**
** Inputs:
**	win	Pointer to a window structure to free memory from.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	03/27/90 (fraser) - Initial version.
**	05/02/90 (dkh) - Tuned up code and moved to this file.
*/
VOID
IITD2dDelwin(win)
WINDOW	*win;
{
	reg	i4	i;
	reg	i4	nlines;
	reg	PTR	p;
	reg	char	**freep;

	nlines = win->_maxy;
	freep = win->_freeptr;
	for (i = 0; i < nlines; i++)
	{
		if ((p = (PTR) *freep++) != NULL)
		{
			MEfree(p);
		}
	}
}
		


/*{
** Name:	TDdelwin - Delete a window by freeing memory used by window.
**
** Description:
**	This routine deletes a window structure by freeing all memory
**	associated with the window.
**
** Inputs:
**	win	Pointer to window to free.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/20/90 (dkh) - Changed to used IITD1dDelwin() and IITD2dDelwin().
*/
VOID
TDdelwin(awin)
WINDOW	*awin;
{
	reg	WINDOW	*win = awin;

	if (!(win->_flags & _SUBWIN))
	{
		/*
		**  Free up memory associated with the
		**  character array.
		*/
		IITD2dDelwin(win);
	}

	/*
	**  Free up memory associated with the arrays for
	**  the window.
	*/
	IITD1dDelwin(win);

	/*
	**  Free up memory for the window itself.
	*/
	MEfree((PTR)win);
}

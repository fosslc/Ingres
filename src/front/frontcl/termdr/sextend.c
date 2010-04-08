/*
**  sextend.c  -  Extend the bottom of a SubWindow
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	xx/xx/xx	-- Created.
**	12/03/86(KY) -- Added for handling Double Bytes attributes (_dx)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
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
# include	<termdr.h>
# include	<me.h>


WINDOW *
TDsextend(awin, nolines)
WINDOW	*awin;
i4	nolines;
{
	reg	WINDOW	*win = awin;
		char	**y;
		char	**da;
		char	**dx;
	reg	i4	i;
		i4	ln;
		i4	ysize;
		i4	chgsize;
		i4	*lastch;
		i4	*firstch;
		i4	*space;
	reg	i4	size;

	if (win == NULL)
		return(NULL);

	if (nolines <= 0)
		return(win);

	ln = win->_maxy + nolines;

	/*
	**  Note:  Calculations below assume that nats and char *s are
	**  the same size (in this case 4 bytes).  If this is not
	**  true, code has to be tuned to take into account the
	**  different sizes when incrementing the pointer "space".
	**  (dkh)
	*/

	chgsize = ln * sizeof (i4);
	ysize = ln * sizeof(char *);

	/* (_firstch, _lastch)+(_y,_da,_dx)*/ 
	size = (chgsize * 2) + (ysize * 3);

	if ((space = (i4 *)MEreqmem((u_i4)0, (u_i4)size, FALSE,
	    (STATUS *)NULL)) == NULL)
	{
		return(NULL);
	}

	y = (char **) space;

	space += ln;

	firstch = (i4 *) space;

	space += ln;

	lastch = (i4 *) space;

	space += ln;

	da = (char **) space;

	space += ln;

	dx = (char **) space;

	for (i = 0; i < ln; i++)
	{
		firstch[i] = lastch[i] = _NOCHANGE;
	}

	win->_firstch = firstch;
	win->_lastch = lastch;
	win->_y = y;
	win->_da = da;
	win->_dx = dx;
	win->_maxy += nolines;

	return(win);
}

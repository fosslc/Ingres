/*
**      Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cm.h>
# include	<me.h>
# include	<termdr.h> 
# include	<tdkeys.h> 

WINDOW		*TDnewwin();

/*
**  TDTMPUTIL.C -- utilities for handling of temporary window.
**
**	Defines:
**		TDtmpwin() - create temporary window structure for TDinsch().
**		TDtwincpy() - copy one line from source window to dest window. 
**
**	History:
**		01/27/87 (KY)  -- Created for inserting Double Bytes chracters.
**	12/23/87 (dkh) - Performance changes.
**	28-sep-89 (bruceb)
**		Properly assign _maxx, _maxy with nc, nl.
**	23-sep-93 (vijay)
**		Move include <termdr.h> down so that <sys/types.h> will get
**		included first (by the cl headers). types.h on AIX has "reg"
**		as a variable name. reg is defined as register in termdr.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**	TDtmpwin -- create a new tempraly window.
**
**	Parameters:
**		twin -- WINDOW structure for temporaly use.
**		nl -- number of max lines on twin
**		nc -- number of max chracters in line on twin
**
**	Returns:
**		WINDOW *  -- pointer of created window
**
**	History:
*/

static	i4	twinxmax = 0;
static	i4	twinymax = 0;


WINDOW *
TDtmpwin(twin, nl, nc)
reg	WINDOW	*twin;
reg	i4	nl;
reg	i4	nc;
{
	if (twin != NULL && nl <= twinymax && nc <= twinxmax)
	{
		twin->_maxx = nc;
		twin->_maxy = nl;
		return(twin);
	}
	else
	{
		if (twin != NULL)
		{
			TDdelwin(twin);
		}
		twinymax = nl;
		twinxmax = nc;
		return(TDnewwin(nl, nc, 0, 0));
	}
}


/*
**	TDtwincpy -- Copy data and attributes from win to twin.
**
**	copy data and attributes from current window to tempraly window
**	for shifting a text to right.
**
**	Parameters:
**		win -- WINDOW structure for source data.
**		twin -- WINDOW structure for destination data.
**		y  -- beginning line to copy.
**		nl -- number of lines to copy.
**
**	Returns:
**		NONE
**
**	History:
*/

TDtwincpy(win, twin, y, nl)
reg	WINDOW	*win;
reg	WINDOW	*twin;
reg	i4	y;
reg	i4	nl;
{
	if (y+nl > win->_maxy)
		nl = win->_maxy - y;

	while (nl--)
	{
		MEcopy(win->_y[y], win->_maxx, twin->_y[y]);
		MEcopy(win->_dx[y], win->_maxx, twin->_dx[y]);
		y++;
	}

	return(OK);
}


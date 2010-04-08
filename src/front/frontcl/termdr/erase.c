# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<cm.h>
# include	<termdr.h>
# include	<st.h>

/*
**  This routine erases everything on the window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  erase.c
**
**  TDershdlr -- This routine erases everything on the window.
**
**  History:
**	mm/dd/yy (RTI) -- created for 5.0.
**	11/06/86 (KY)  -- added erase attributes of Double Bytes characters
**			    and Phantom Spaces.
**	19-jun-87 (bab)	Code cleanup.
**	10/14/87 (dkh) - Removed unnecessary ifdefs.
**	10/14/87 (dkh) - Changed to use STskipblank instead of STskip.
**	12/23/87 (dkh) - Performance changes.
**	10/22/88 (dkh) - More perf changes.
**	11/10/88 (dkh) - Temporarily turn off DOUBLEBYTE.
**	02/21/89 (dkh) - Added include of cm.h and cleaned up ifdefs.
**	03/22/89 (dkh) - Added code to also clear out dx values in
**			 optimized case.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	25-Aug-1993 (fredv)
**		Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	char	*STskipblank();

# define	ERASE_ALL		(i4) 0
# define	ERASE_CH		(i4) 1


static
TDershdlr(win, erstype)
reg	WINDOW	*win;
reg	i4	erstype;
{
	reg	i4	y;
	reg	i4	maxx;
	reg	i4	maxy;
	reg	char	blank = ' ';
	reg	char	**_y;
	reg	char	**_da;
	reg	char	**_dx;
	reg	char	*dx;
	reg	char	*sp;
	reg	char	*start;
	reg	i4	*_lastch;
	reg	i4	*_firstch;
	reg	i4	all_erase = 0;
	reg	i4	count = 0;

	maxx = win->_maxx;
	maxy = win->_maxy;
	_y = win->_y;
	_da = win->_da;
	_dx = win->_dx;
	_lastch = win->_lastch;
	_firstch = win->_firstch;
	if (erstype == ERASE_ALL)
	{
		_da = win->_da;
		all_erase = 1;
	}
	for (y = 0; y < maxy; y++)
	{
		start = *_y++;
		dx    = *_dx++;
		if (maxx < 30)
		{
			count = maxx;
			while (count--)
			{
				*start++ = blank;
				*dx++ = EOS;
			}
		}
		else
		{
			if ((sp = STskipblank(start, maxx)) != NULL)
			{
				u_i2	erasecnt = start + maxx - sp;
			
				MEfill(erasecnt, (u_char) blank, sp);

				/*
				** erase attributes of Double Bytes chracters
				** and Phantom Spaces too.
				*/
# ifdef DOUBLEBYTE
				dx += maxx - erasecnt;
				MEfill(erasecnt, (u_char) '\0', dx);
# endif /* DOUBLEBYTE */
			}
		}
		*_firstch++ = 0;
		*_lastch++ = maxx - 1;
		if (all_erase)
		{
			MEfill((u_i2) maxx, (u_char) '\0', *_da++);
		}
	}
	win->_curx = win->_cury = 0;
}

TDersda(win)
WINDOW	*win;
{
	i4	maxy;
	i4	maxx;
	i4	y;
	char	**_da;

	maxy = win->_maxy;
	maxx = win->_maxx;
	_da = win->_da;
	for (y = 0; y < maxy; y++)
	{
		MEfill((u_i2) maxx, (u_char) '\0', _da[y]);
	}
}


TDerschars(win)
WINDOW	*win;
{
	TDershdlr(win, ERASE_CH);
}


TDerase(win)
WINDOW	*win;
{
	TDershdlr(win, ERASE_ALL);
}

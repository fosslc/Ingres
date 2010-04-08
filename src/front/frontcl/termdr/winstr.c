# include	<compat.h>
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

/*
**  WINSTR.C
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**	08/19/85 - DKH
**		Added argument "trim" that indicates if we want
**		to trim blanks at the end of the string.
**	10/18/86 (KY)
**		Copy characters in the window to 'string' except phantom spaces.
**	22-apr-87 (bab)
**		Added TDwin_rstr to extract strings from a right-to-left
**		field.  This routine works for single-byte characters.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Added IITDwin_attr() to return 1st attribute character in
**		the given window.
**	18-mar-92 (leighb) DeskTop Porting Change:
**		make sure pointer 'win' is not NULL in TDwin_str.
**	 3-may-93 (rudyw)
**		make sure pointer 'win' is not NULL in TDwin_rstr. A NULL value
**		set in TMwmvtab signifying an invisible column may end up here.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

VOID
TDwin_str(win, string, trim)
WINDOW	*win;
char	*string;
bool	trim;
{
		i4	i;
	reg	u_char	*cur;
	reg	u_char	*end;
	reg	u_char	*ptr;
	reg	u_char	*dx;

	if (win == NULL)			 
	{
		*string = '\0';
		return;
	}
	ptr = (u_char *) string;
	for (i = 0; i < win->_maxy; i++)
	{
		cur = (u_char *) &win->_y[i][0];
		dx  = (u_char *) &win->_dx[i][0];
		end = (u_char *) &win->_y[i][win->_maxx - 1];
		while (cur <= end)
		{
			if (*dx++ & _PS)
			{
				cur++;
			}
			else
			{
				*ptr++ = *cur++;
			}
		}
	}
	*ptr = '\0';
	if (trim)
	{
		STtrmwhite(string);
	}
}

VOID
TDwin_rstr(win, string, trim)
WINDOW	*win;
char	*string;
bool	trim;
{
		i4	i;
	reg	u_char	*cur;
	reg	u_char	*end;
	reg	u_char	*ptr;

	if (win == NULL)			 
	{
		*string = '\0';
		return;
	}

	ptr = (u_char *) string;
	for (i = 0; i < win->_maxy; i++)
	{
		cur = (u_char *) &win->_y[i][win->_maxx - 1];
		end = (u_char *) &win->_y[i][0];
		while (cur >= end)
		{
			*ptr++ = *cur--;
		}
	}
	*ptr = '\0';
	if (trim)
	{
		STtrmwhite(string);
	}
}

u_char
IITDwin_attr(win)					 
WINDOW	*win;
{
	return(**(win->_da));
}							 

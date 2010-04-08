

/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<cm.h>


/**
** Name:	tdfplus.c  - function to support flashing plus sign
**
** Description:
**	When the user activates the "Create" menuitem in vifred a flashing
**	plus sign is displayed at the cursor position. This file
**	supports that plus sign. Note the plus sign is supported 
**	as a one deep stack.
**



/*{
** Name:	TDflashplus	- turn on/off the flashing plus sign
**
** Description:
**	We must save the screen's contents temporarily 
**	and display a flashing plus sign. (vifred support)
**	This routine assumes it is going to be called once with
**	state == 1 .. so that it can save the screen location
**	and put up a flashing plus sign.. and then latter
**	it will be called again with state == 0 at which
**	time it will restore the (temporarily obscured) screen
**	position.
**
** Inputs:
**	WINDOW *win;	- window struct ptr
**	i4	y,x;	- coordinates
**	i4	state;	- state to set on/off
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04-04-88  (tom) - created
**	14-mar-1995 (peeje01) Cross Integration from 6500db_su4_us42
**		Original comments appear below:
** **      06-aug-92 (kirke)
** **          Added code to handle case when the '+' overlays a DB character.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
TDflashplus(win, y, x, state)
WINDOW	*win; 
i4	y; 
i4	x;
i4	state;
{
	static i4   lin;
	static i4   col;
	static char ch;
	static char ch2;
	static char da;
	
	if (state == 0)		/* turn off flashing plus, restore old char */
	{
		win->_y[lin][col] = ch;			
		if (CMdbl1st(&ch))
			win->_y[lin][col + 1] = ch2;
		win->_da[lin][col] = da;			
	}
	else			/* save old char/attr, turn on flashing plus */
	{
		ch = win->_y[lin = y][col = x];
		da = win->_da[y][x];
		if (CMdbl1st(&ch))
		{
			ch2 = win->_y[y][x + 1];
			win->_y[y][x + 1] = OVERLAYSP;
		}
		win->_y[y][x] = '+';
		win->_da[y][x] = _RVVID | _BLINK;
		;
	}
}


# include	<compat.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<termdr.h>


/*
**  This routine draws a box around the given window with "vert"
**  as the vertical delimiting char, and "hor", as the horizontal one.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	19-jun-87 (bab)	Code cleanup.
**	07/13/88 (dkh) - Initialized ld_chr with non-line drawing
**			 set of characters.
**	10/30/88 (dkh) - Performance changes.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**      24-sep-96 (mcgem01)
**              Global data moved to gtdata.c
**	18-apr-01 (hayke02)
**		Required change to avoid conflict with reserved word restrict.
**		Identified after latest compiler OSFCMPLRS440 installed on
**		axp.osf which includes support for this based on the C9X
**		review draft.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-may-2001 (somsa01)
**	    Changed 'restrict' to 'iirestrict' from last cross-integration.
**	28-Mar-2205 (lakvi01)
**	    Corrected function prototypes.
*/


GLOBALREF i4	tdprform;


/*
** ld_chr - This table contains the line drawing character to use
**		given a particular set of directions of line extensions.
**
**	This array is set up by TDboxsetup called at
**	initialization time.
*/
static char	ld_chr[16] = { ' ', '|', '-', '+',
			       '|', '|', '+', '+',
			       '-', '+', '-', '+',
			       '+', '+', '+', '+'
			     }; 
/*
** ld_base - This contains the base number with which to offset the
** 		spoke representation of the box characters.. It is
**		chosen at initialization time so that it does not conflict
**		with any of the terminal's line drawing characters.
*/
static i4	ld_base;  


/* the directions of the spokes emanating from the center of a box 
   drawing character position */
# define LD_NORTH 0x01
# define LD_EAST  0x02
# define LD_SOUTH 0x04
# define LD_WEST  0x08

/*{
** Name:	TDboxsetup	- setup box/line drawing characters 
**
** Description:
**	Setup box/line drawing character data structures. This assumes
**	that we have now initialized the terminal and have termcap info
**	about line drawing.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		none	
**
**	Exceptions:
**		none
**
** Side Effects:
**		The box character data structures are setup.
**
** History:
**	03/30/88 (tom) - written	
*/
VOID
TDboxsetup()
{
	i4 	i;

	/* if the terminal supports line drawing */
	if (LD != NULL)
	{
		ld_chr[ LD_NORTH | LD_WEST ] 				= *QA;
		ld_chr[ LD_SOUTH | LD_WEST ] 				= *QB;
		ld_chr[ LD_SOUTH | LD_EAST ] 				= *QC;
		ld_chr[ LD_NORTH | LD_EAST ] 				= *QD;
		ld_chr[ LD_NORTH | LD_WEST  | LD_SOUTH | LD_EAST ] 	= *QE;
		ld_chr[ LD_WEST ]					= *QF;
		ld_chr[ LD_EAST ]					= *QF;
		ld_chr[ LD_WEST  | LD_EAST ]				= *QF;
		ld_chr[ LD_NORTH | LD_SOUTH | LD_EAST ]			= *QG;
		ld_chr[ LD_NORTH | LD_SOUTH | LD_WEST ]			= *QH;
		ld_chr[ LD_NORTH | LD_WEST  | LD_EAST ]			= *QI;
		ld_chr[ LD_WEST  | LD_SOUTH | LD_EAST ] 		= *QJ;
		ld_chr[ LD_NORTH ] 					= *QK;
		ld_chr[ LD_SOUTH ] 					= *QK;
		ld_chr[ LD_NORTH | LD_SOUTH ] 				= *QK;
	}
    /*
	else
	{
		ld_chr[ LD_NORTH | LD_WEST ] 				= '+';
		ld_chr[ LD_SOUTH | LD_WEST ] 				= '+';
		ld_chr[ LD_SOUTH | LD_EAST ] 				= '+';
		ld_chr[ LD_NORTH | LD_EAST ] 				= '+';
		ld_chr[ LD_NORTH | LD_WEST  | LD_SOUTH | LD_EAST ] 	= '+';
		ld_chr[ LD_WEST ]					= '-';
		ld_chr[ LD_EAST ]					= '-';
		ld_chr[ LD_WEST  | LD_EAST ]				= '-';
		ld_chr[ LD_NORTH | LD_SOUTH | LD_EAST ]			= '+';
		ld_chr[ LD_NORTH | LD_SOUTH | LD_WEST ]			= '+';
		ld_chr[ LD_NORTH | LD_WEST  | LD_EAST ]			= '+';
		ld_chr[ LD_WEST  | LD_SOUTH | LD_EAST ] 		= '+';
		ld_chr[ LD_NORTH ] 					= '|';
		ld_chr[ LD_SOUTH ] 					= '|';
		ld_chr[ LD_NORTH | LD_SOUTH ] 				= '|';
	}
    */

	/* we must choose ld_base such that none of the terminal's line
	   drawing characters fall within the range ld_base .. ld_base + 15
	   inclusive note that there is no choice of line drawing characters
	   that the terminal vendor could use such that there would not
	   be room */
	for (ld_base = 0; ; ld_base++)
	{
		for (i = 0; i < 16; i++)
		{
			if (	ld_chr[i] >= ld_base 
			   &&   ld_chr[i] <= ld_base + 15
			   )
				goto KEEP_TRYING;
		}
		break;	/* no conflict found */
    KEEP_TRYING:
		continue;
	}
} 
/*{ 
** Name:	TDboxput	- put box chars to the virtual screen.
**
** Description:
**	This routine is responsible for the placement of box/line drawing
**	characters to the virtual screen. This routine will fixup 
**	lines that meet into T's and Crosses as needed based on what
**	is already on the virtual screen. This function is not
**	called if line drawing is not supported by the terminal.
**
** Inputs:
**	char	*scr;	- screen image pointer
**	char	*atr;	- screen attribute pointer
**	i4	count;	- count of characters to post
**	i4	mask;	- bit mask indicating line extension directions
**			  for the character to post LD_NORTH ....
**	i4	rmask;	- restriction mask.. this is a bit mask indicating
**			  that the spokes extending in the indicated directions
**			  are to be surpressed. This allows us to keep
**			  boxes and lines from encroaching into the 
**			  boxed fields and table field boxes.
**	i4	attr;	- attribute byte to post
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/30/88  (tom) - created
*/
VOID
TDboxput(chr, atr, count, mask, rmask, attr)
reg	char	*chr;
reg	char	*atr;
reg	i4	count;
reg	i4	mask;
reg	i4	rmask;
reg	i4	attr;
{
	reg	i4	c;

	while (count--)
	{
		c = *chr - ld_base;
		if (	(*atr & _LINEDRAW)
		   &&	c > 0 
		   &&	c < 16
		   )
		{
			/* turn off any restricted spokes */
			c &= ~rmask;
			/* turn on any new spokes */
			*chr++ = (mask | c) + ld_base;
		}
		else
		{
			*chr++ = mask + ld_base;
		}
		*atr++ = attr | _LINEDRAW;
	}
}

/*{
** Name:	TDboxfix	- fixup box characters in virtual screen 
**
** Description:
**	When box characters are posted into the virtual screen, the lower
**	four bits of the character byte indicate in which direction
**	the spokes emanate (after subtracting out the ld_base). 
**	In addition a bit in the _flag member of the WINDOW structure is set.
**	When refresh is performed the TDboxfix
**	routine is called, which looks at this bit. If it is set
**	then TDboxfix will go through the whole window translating the
**	line drawing characters from their spoke representation to the
**	actual display characters.
**
** Inputs:
**	WINDOW	*win;	- ptr to window structure to check and maybe fixup
**
** Outputs:
**	Returns:
**		nothing 
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	04/26/88 (tom)  - first written
*/
VOID
TDboxfix(win)
WINDOW	*win;
{
	reg	char	*chr;
	reg	char	*atr;
	reg	i4	cols;
	reg	i4	lin;
	
	if (win->_flags & _BOXFIX)
	{
		for (lin = 0; lin < win->_maxy; lin++)
		{
			chr = win->_y[lin];
			atr = win->_da[lin];
			for (cols = win->_maxx; cols--; chr++)
			{
				if (	(*atr++ & _LINEDRAW)
				   && 	*chr >= ld_base 
				   && 	*chr < ld_base + 16
				   )
				{
					*chr = ld_chr[*chr - ld_base];
				}
			}
		}
		win->_flags &= ~_BOXFIX;
	}
}
/* 
** Routine sets the corners for boxes.
*/

VOID
TDsetcorners(fchrs, lchrs, fatrs, latrs, bx, endx, corner, attr)
char	*fchrs;
char	*lchrs;
char	*fatrs;
char	*latrs;
reg	i4	bx;
reg	i4	endx;
i4	corner;
i4	attr;
{
	if (LD == NULL || tdprform)
	{
		fchrs[bx] = fchrs[endx] = lchrs[bx] = lchrs[endx] = corner;
		fatrs[bx] = fatrs[endx] = latrs[bx] = latrs[endx] = attr; 
	}
	else
	{
		TDboxput(&fchrs[bx], &fatrs[bx], 1, 
						LD_SOUTH | LD_EAST, 0, attr);
		TDboxput(&fchrs[endx], &fatrs[endx], 1, 
						LD_SOUTH | LD_WEST, 0, attr);
		TDboxput(&lchrs[bx], &latrs[bx], 1, 
						LD_NORTH | LD_EAST, 0, attr);
		TDboxput(&lchrs[endx], &latrs[endx], 1, 
						LD_NORTH | LD_WEST, 0, attr);
	}
}


/*
**  Laydown horizontal and vertical bars for box/table fields.
*/

VOID
TDhv(win, by, endy, bx, endx, vert, hor, attr, iirestrict)
reg	WINDOW	*win;
i4		by;
i4		endy;
i4		bx; 
i4		endx;
reg	char	vert;
char		hor;
reg	i4	attr;
	i4	iirestrict;
{
	reg	char	*fchrs;
	reg	char	*lchrs;
	reg	char	*fatrs;
	reg	char	*latrs;
	reg	i4	i;
	i4		linedraw;

	/* if we have line drawing and we are not printing the form.. */
	linedraw = LD != NULL && !tdprform;

	if (linedraw)
	{
		/* we must post that box characters must be fixed up
		   before we attempt to refresh the real screen */
		win->_flags |= _BOXFIX;
	}

	i = endx - bx - 2;
	fchrs = win->_y[by];
	lchrs = win->_y[endy];
	fatrs = win->_da[by];
	latrs = win->_da[endy];

	/* Laydown horizontal separators.  */

	if (linedraw)
	{
		TDboxput(fchrs + bx + 1, fatrs + bx + 1, i, 
			LD_EAST | LD_WEST, iirestrict ? LD_SOUTH : 0, attr);
		TDboxput(lchrs + bx + 1, latrs + bx + 1, i, 
			LD_EAST | LD_WEST, iirestrict ? LD_NORTH : 0, attr);
	}
	else
	{
		MEfill((u_i2)i, hor, fchrs + bx + 1); 
		MEfill((u_i2)i, hor, lchrs + bx + 1); 
		MEfill((u_i2) i, attr, fatrs + bx + 1);
		MEfill((u_i2) i, attr, latrs + bx + 1);
	}


	/*  Laydown vertical separators.  */

	endx -= bx + 1; /* make endx relative to start of the line */

	if (linedraw)
	{
		for (i = by + 1; i < endy; i++) 
		{
			fchrs = win->_y[i] + bx;
			fatrs = win->_da[i] + bx;
			TDboxput(fchrs, fatrs, 1, 
				LD_NORTH | LD_SOUTH,
				iirestrict ? LD_EAST : 0, attr);
			TDboxput(fchrs + endx, fatrs + endx, 1, 
				LD_NORTH | LD_SOUTH,
				iirestrict ? LD_WEST : 0, attr);
		}
	}
	else
	{
		for (i = by + 1; i < endy; i++) 
		{
			fchrs = win->_y[i] + bx;
			fchrs[0] = vert;
			fchrs[endx] = vert;

			fatrs = win->_da[i] + bx;
			fatrs[0] = attr;
			fatrs[endx] = attr;
		}
	}
}


/*
**  TDboxtopless
**
**  Draw a box without the top line.
*/

VOID
TDboxtopless(awin, vert, hor, corner)
WINDOW	*awin;
char	vert;
char	hor;
char	corner;
{
	u_char	val;
	i4		maxx;

	TDbox(awin, vert, hor, corner);

	/*
	**  Now blank out top line of box.
	*/

	maxx = awin->_maxx;
	MEfill((u_i2) maxx, ' ', awin->_y[0]);
	MEfill((u_i2) maxx, '\0', awin->_da[0]);
	MEfill((u_i2) maxx, '\0', awin->_dx[0]);
	if (LD != NULL && !tdprform)
	{
		val = *QK;
		awin->_da[0][0] |= _LINEDRAW;
		awin->_da[0][maxx - 1] |= _LINEDRAW;
	}
	else
	{
		val = '|';
	}
	awin->_y[0][0] =  val;
	awin->_y[0][maxx - 1] = val;
}


VOID
TDbox(awin, vert, hor, corner)
WINDOW		*awin;
char		vert;
char		hor;
char		corner;
{
	reg	WINDOW	*win = awin;
	reg	i4	endy;
	reg	i4	endx;
	reg	char	*fchrs;
	reg	char	*lchrs;
	reg	char	*fatrs;
	reg	char	*latrs;

	endx = win->_maxx;
	endy = win->_maxy - 1;
	fchrs = win->_y[0];
	lchrs = win->_y[endy];
	fatrs = win->_da[0];
	latrs = win->_da[endy];

	TDhv(win, 0, endy, 0, endx, vert, hor, 0, TRUE);
	endx--;

	if (!win->_scroll && (win->_flags & _SCROLLWIN))
		fchrs[0] = fchrs[endx] = lchrs[0] = lchrs[endx] = ' ';
	else
	{
			TDsetcorners(fchrs, lchrs, fatrs, latrs, 0, endx, 
				corner, 0);
	}
	TDtouchwin(win);
}

VOID
TDboxhdlr(win, y, x, type)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
reg	i4	type;
{
	reg	char	*cp;

	cp = &(win->_y[y][x]);

	if (LD == NULL || tdprform)
	{
		if (type == TD_TOPT || type == TD_BOTTOMT || type == TD_XLINES)
		{
			*cp = '+';
		}
		else
		{
			*cp = '|';
		}
	}
	else
	{
		switch (type)
		{
		case TD_TOPT:
			type = LD_EAST  | LD_WEST  | LD_SOUTH;
			break;

		case TD_BOTTOMT:
			type = LD_EAST  | LD_WEST  | LD_NORTH;
			break;

		case TD_RIGHTT:
			type = LD_SOUTH  | LD_WEST  | LD_NORTH;
			break;

		case TD_LEFTT:
			type = LD_EAST  | LD_SOUTH  | LD_NORTH;
			break;

		default:
			type = LD_EAST  | LD_WEST  | LD_NORTH  | LD_SOUTH;
			break;
		}

		TDboxput(cp, &win->_da[y][x], 1, type, 0, 0);
		/* request box character fixup */
		win->_flags |= _BOXFIX;
	}
}

VOID
TDtfdraw(win, y, x, ch)
WINDOW		*win;
reg	i4	y;
reg	i4	x;
reg	char	ch;
{
	char	*p, *atp;

	p = &win->_y[y][x];
	atp = &win->_da[y][x];

	if (LD == NULL || tdprform)
	{
		*p = ch;
		*atp = 0;
	}
	else
	{
		TDboxput(p, atp, 1,
			(ch == '-' || ch == '=')
				?  (LD_EAST  | LD_WEST)
				:  (LD_NORTH | LD_SOUTH) ,
				0, 0);
		win->_flags |= _BOXFIX;
	}
	TDmove(win, y, x);
}

VOID
TDcorners(win, attr)
WINDOW	*win;
i4	attr;
{
	i4	endy = win->_maxy - 1;
	i4	endx = win->_maxx - 1;

	/* if there is line drawing.. then TDsetcorners will be painting 
	   box characters. So we must post for refresh time fixup */
	if (LD != NULL && !tdprform)
		win->_flags |= _BOXFIX;

	TDsetcorners( win->_y[0], win->_y[endy], win->_da[0], win->_da[endy], 
		0, endx, '+', attr);
}

/*{
** Name:	TDhline	- draw horizontal line in window
**
** Description:
**	Called from TDsbox.
**	Draws a horizontal line at the given position, for a given length.
**	
**
** Inputs:
**	WINDOW *win	- window to draw in
**	i4 y;		- line to start with
**	i4 x;		- col to start with
**	i4 len;	- length of the line 
**	i4 attr;	- attribute of line 
**	i4 hiattr;	- hilight attribute of line end
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**

** Side Effects: **
** History:
**	04/18/88  (tom) - first written
*/
i4
TDhline(win, y, x, len, attr, hiattr)
WINDOW	*win;
i4	y;
i4	x;
i4	len;
i4	attr;
i4  	hiattr;
{
	char	*pch, *pda;
	i4		linedraw;

	/* if we have line drawing and we are not printing the form.. */
	linedraw = LD != NULL && !tdprform;

	pch = win->_y[y] + x;
	pda = win->_da[y] + x;

	if (linedraw)
	{
		TDboxput(pch, pda, 1, LD_EAST, 0, hiattr);

		if (len > 2)
		{
			TDboxput(pch + 1, pda + 1, len - 2, 
				LD_EAST | LD_WEST, 0, attr);
		}

		TDboxput(pch + len - 1, pda + len - 1, 1, LD_WEST, 0, hiattr);

		/* request fixup of box characters at refresh time */
		win->_flags |= _BOXFIX;
	}
	else
	{
		pch[0] = '-';
		pda[0] = hiattr;
		if (len > 2)
		{
			MEfill(len - 2, '-', pch + 1);
			MEfill(len - 2, attr, pda + 1);
		}
		pch[len-1] = '-';
		pda[len-1] = hiattr;
	}
}
/*{
** Name:	TDvline	- draw vertical line in window
**
** Description:
**	Called from TDsbox.
**	Draws a vertical line at the given position, for a given length.
**	
**
** Inputs:
**	WINDOW *win	- window to draw in
**	i4 y;		- line to start with
**	i4 x;		- col to start with
**	i4 len;	- length of the line 
**	i4 attr;	- attribute of line 
**	i4 hiattr;	- hilight attribute of line end 
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	04/18/88  (tom) - first written
*/
i4
TDvline(win, y, x, len, attr, hiattr)
reg WINDOW	*win;
reg i4	y;
reg i4	x;
i4	len;
i4	attr;
i4	hiattr;
{
	reg	char	*pch, *pda;
	i4		linedraw;

	/* if we have line drawing and we are not printing the form.. */
	linedraw = LD != NULL && !tdprform;

	if (linedraw)
	{
		pch = win->_y[y] + x;
		pda = win->_da[y] + x;

		TDboxput(pch, pda, 1, LD_SOUTH, 0, hiattr);

		while (len-- > 2)
		{
			pch = win->_y[++y] + x;
			pda = win->_da[y] + x;
			TDboxput(pch, pda, 1, LD_NORTH | LD_SOUTH, 0, attr);
		}

		pch = win->_y[++y] + x;
		pda = win->_da[y] + x;
		TDboxput(pch, pda, 1, LD_NORTH, 0, hiattr);

		/* request fixup of box characters at refresh time */
		win->_flags |= _BOXFIX;
	}
	else
	{
		win->_y[y][x] = '|';
		win->_da[y][x] = hiattr;

		while (len-- > 2)
		{
			win->_y[++y][x] = '|';
			win->_da[y][x] = attr;
		}

		win->_y[++y][x] = '|';
		win->_da[y][x] = hiattr;

	}
}

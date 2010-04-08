/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
	pointers of range mode buffers are ...

    <head buffer>
	+------------------------------+
	|abK1cdK2efgK3K4K5  --> add    |
	+----------------*-------------+
	 ^               ^            ^
	 |               |            |
        hbuf            hcur         hend

				+--------------------+
				|		     |
				|       WINDOW       |
				|		     |
				+--------------------+

				    <tail buffer>
					+------------------------------+
					|     add <--      K3fedK2cbaK1|
					+------------------*-----------+
					 ^                 ^          ^
				         |                 |          |
					tbuf              tcur       tend
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"ftframe.h"
# include	"ftrange.h"
# include	<cm.h>

/*
**  FTDBLUTIL.C -- utilities of Double Byte character handling.
**
**	Defines:
**		FTrdelchar()	    delete a character on the current position
**		FTrmvleft()	    move cursor left in range mode
**		FTrmvright()	    move cursor right in range mode
**		FTrinschar()	    insert a character into the current position
**		FTraddchar()	    overwrite a char on the current position
**
**	Routines below are called only by routines above.
**
**		IIFTdsrDmySftR()    return how many characters have to put
**					out into tail buffer.
**		IIFTdslDmySftL()    check whether up to 2 characters can get
**					into window from tail buffer or not.
**		IIFTslShiftLeft()	    shift screen data to left until 2 characters
**					in the tail buffer can get into window.
**		IIFTtaTailAddstr()  add string to tail buffer.
**		IIFTatAddToTail()   put 'nbytes' bytes from end of window to
**					tail buffer.
**		IIFTahAddtoHead()   put 'nchars' characters to head buffer from
**					beginning of window.
**		FTgetfromtl()	    get characters into window from tail buffer
**					as many as posible.
**		FTlastpos()	    check whether current position is last
**					position or not.
**
**	History:
**		02/17/87 (KY)  -- Created for handling Double Byte characters.
**		28-apr-87 (bab)
**			Added code for handling of reversed, right-to-left
**			fields.  Such fields do NOT work with double byte
**			characters.
**		04-06-87 (bab)
**			Declare return types of various functions.
**	06/19/87 (dkh) - Code cleanup.
**	07/22/87 (dkh) - Fixed jup bug 479.
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**	10/02/87 (dkh) - Various changes for check7.
**	22-oct-87 (bab)
**		changed from rgpt->TCUR to rgptr->tcur; wouldn't
**		have compiled as a Kanji version.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

VOID	FTrdelchar();
i4	FTrmvleft();
i4	FTrmvright();
VOID	FTrinschar();
VOID	FTraddchar();
i4	IIFTdsrDmySftR();
i4	IIFTdslDmySftL();
i4	FTchksftL();
VOID	IIFTslShiftLeft();
VOID	IIFTtaTailAddstr();
i4	IIFTatAddToTail();
i4	IIFTahAddtoHead();
VOID	FTgetfromtl();


/*
**	FTrdelchar -- delete a character on the current position
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		NONE
**
**	History:
*/

VOID
FTrdelchar(win, rgptr, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
bool	reverse;
{
	i4	lasty, lastx, y, x;
	i4	lastmoved = FAIL;

	if (reverse)
	{
		TDrdelch(win);
	}
	else
	{
		if (rgptr->tcur != NULL)
		{
			lastmoved = IIFTdslDmySftL(win, 1, &lasty, &lastx);
		}
		TDdelch(win);
	}
	if (rgptr->tcur != NULL
	    && (reverse || lastmoved == OK))
	{
		y = win->_cury;
		x = win->_curx;
		if (reverse)
		{
			win->_cury = win->_maxy - 1;
			win->_curx = 0;
		}
		else
		{
			win->_cury = lasty;
			win->_curx = lastx;
		}
		FTgetfromtl(win, rgptr, reverse);
		win->_cury = y;
		win->_curx = x;
	}
}


/*
**	FTrmvleft -- move cursor left in range mode
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		OK -- could move left or, if not reversed, could get character
**			from head buffer.
**		FAIL -- if not reversed, go outside of window and head buffer
**			has no characters.
**
**	History:
*/

i4
FTrmvleft(win, rgptr, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
bool	reverse;
{
	u_char	*cp = (u_char *) rgptr->hcur;
	i4	nbytes;
	i4	retval;

	if (reverse)
	{
		if (win->_cury == win->_maxy - 1 && win->_curx == 0)
		{
			IIFTahAddtoHead(win, rgptr, (i4)1, reverse);
			win->_cury = 0;
			win->_curx = win->_maxx - 1;
			TDrdelch(win);
			win->_cury = win->_maxy - 1;
			win->_curx = 0;
			FTgetfromtl(win, rgptr, reverse);

			retval = OK;
		}
		else
		{
			retval = TDrmvleft(win, 1);
		}
	}
	else
	{
		if (win->_cury == 0 && 	win->_curx == 0 && rgptr->hcur != NULL)
		{
			cp++;
			CMprev(cp, rgptr->hbuf);
			nbytes = IIFTdsrDmySftR(win, CMbytecnt(cp));
			if (nbytes)
			{
				IIFTatAddToTail(win, rgptr, nbytes);
			}
			*(cp + CMbytecnt(cp)) = '\0';
			TDinsstr(win, cp);
			rgptr->hcur = (char *) cp;
			if (rgptr->hcur == rgptr->hbuf)
			{
				rgptr->hcur = NULL;
			}
			else
			{
				(rgptr->hcur)--;
			}
			win->_cury = win->_curx = 0;
	
			retval = OK;
		}
		else
		{
			retval = TDmvleft(win, 1);
		}
	}
	return(retval);
}


/*
**	FTrmvright -- move cursor right in range mode
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		OK -- could move right or, if reversed, could get character
**			from head buffer.
**		FAIL -- if reversed, go outside of window and head buffer
**			has no characters.
**
**	History:
*/

i4
FTrmvright(win, rgptr, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
bool	reverse;
{
	u_char	tempch[2];
	i4	nbytes;
	i4	retval;

	if (reverse)
	{
		if (win->_cury == 0 && win->_curx == win->_maxx - 1
							&& rgptr->hcur != NULL)
		{
			tempch[0] = win->_y[win->_cury][win->_curx];
			tempch[1] = EOS;
			IIFTtaTailAddstr(rgptr, tempch);
			TDrinsstr(win, rgptr->hcur); /* single byte char */
			if (rgptr->hcur == rgptr->hbuf)
				rgptr->hcur = NULL;
			else
				(rgptr->hcur)--;

			retval = OK;
		}
		else
		{
			retval = TDrmvright(win, 1);
		}
	}
	else
	{
		if (win->_cury == win->_maxy - 1
					&& FTlastpos(win, fdopRIGHT, 0, NULL))
		{
			nbytes = win->_maxx - win->_curx;
			IIFTatAddToTail(win, rgptr, nbytes);
			IIFTslShiftLeft(win, rgptr, reverse);
			FTgetfromtl(win, rgptr, reverse);
			/*
			**  Only increment win->_curx if we can.
			**  Ability to move should already have been
			**  determined by calls above.
			*/
			if (win->_curx < win->_maxx - 1)
			{
				CMbyteinc(win->_curx,
					&(win->_y[win->_cury][win->_curx]));
			}

			retval = OK;
		}
		else
		{
			retval = TDmvright(win, 1);
		}
	}
	return (retval);
}


/*
**	FTrinschar -- insert a character into the current position
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**		cp	-- character pointer for inserting.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		NONE
**
**	History:
*/

VOID
FTrinschar(win, rgptr, cp, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
u_char	*cp;
bool	reverse;
{
	u_char	tempch[2];
	i4	nbytes;

	if (reverse)
	{
		if (win->_cury == win->_maxy - 1 && win->_curx == 0)
		{
			IIFTahAddtoHead(win, rgptr, (i4)1, reverse);
			win->_cury = 0;
			win->_curx = win->_maxx - 1;
			TDrdelch(win);
			win->_cury = win->_maxy - 1;
			win->_curx = 0;
		}
		else
		{
			tempch[0] = win->_y[win->_maxy - 1][0];
			tempch[1] = EOS;
			IIFTtaTailAddstr(rgptr, tempch);
		}
		TDrinsstr(win, cp);
	}
	else
	{
		if (win->_cury == win->_maxy - 1
					&& FTlastpos(win, fdopERR, (1), cp))
		{
			nbytes = win->_maxx - win->_curx;
			IIFTatAddToTail(win, rgptr, nbytes);
			IIFTtaTailAddstr(rgptr, cp);
			IIFTslShiftLeft(win, rgptr, reverse);
			FTgetfromtl(win, rgptr, reverse);
			CMbyteinc(win->_curx,
					&(win->_y[win->_cury][win->_curx]));
			return;
		}
		else
		{
			nbytes = CMbytecnt(cp);
			if (win->_curx + nbytes > win->_maxx)
			{
				nbytes++;
			}
			nbytes = IIFTdsrDmySftR(win, nbytes);
			if (nbytes)
			{
				IIFTatAddToTail(win, rgptr, nbytes);
			}
		}
		TDinsstr(win, cp);
	}
}


/*
**	FTraddchar -- overwrite a character on the current position
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**		cp	-- character pointer for adding.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		NONE
**
**	History:
*/

VOID
FTraddchar(win, rgptr, cp, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
u_char	*cp;
bool	reverse;
{
	if (!reverse && win->_cury == win->_maxy - 1
					&& FTlastpos(win, fdopERR, (0), cp))
	{
		/*
		** If you overstrike a Double Byte character on the last
		** position of window the head byte of tail buffer should
		** be deleted.
		** And if the head character is a Double Byte character,
		** then 2nd byte has to be changed to space.
		**
		**    ----------+				----------+
		**            ab|K1cd	---> overstrike 'K?'	     abK? |cd
		**    ---------*+				----------+
		**           [aK|? cd]
		*/
		if (rgptr->tcur != NULL && win->_curx == win->_maxx-1
								&& CMdbl1st(cp))
		{
			if (CMdbl1st(rgptr->tcur++))
			{
				*(rgptr->tcur) = ' ';
			}
		}
		IIFTtaTailAddstr(rgptr, cp);
		IIFTslShiftLeft(win, rgptr, reverse);
		FTgetfromtl(win, rgptr, reverse);
		/*
		**  Only increment win->_curx if we can.
		**  Ability to move should already have been
		**  determined by calls above.
		*/
		if (win->_curx < win->_maxx - 1)
		{
			CMbyteinc(win->_curx,
				&(win->_y[win->_cury][win->_curx]));
		}
	}
	else
	{
		TDaddchar(win, cp, reverse);
	}
}


/*
**	IIFTdsrDmySftR -- return how many characters have to put out
**			  to tail buffer.
**
**	Parameters:
**		win	-- WINDOW structure.
**		nbytes	-- number of bytes to move right from current cursor
**			   position.
**
**	Returns:
**		How many characters have to put out into tail buffer.
**		remain current cursor position of 'win' structure.
**
**	History:
*/

i4
IIFTdsrDmySftR(win, nbytes)
WINDOW	*win;
i4	nbytes;
{
	i4	y=win->_cury;
	i4	x=win->_curx;
	i4	toy, tox;
	u_char	*dx;
	i4	restbytes;

	dx   = (u_char *) &(win->_dx[y][x]);
	toy = y;
	tox = x + nbytes;
	if (tox >= win->_maxx)
	{
		toy++;
		tox -= win->_maxx;
	}	

	while (toy < win->_maxy)
	{
		if (y == toy && x == tox)
		{
			return(0);
		}
		while (tox < win->_maxx)
		{
			if (x <= tox)
			{
				x += (win->_maxx - tox);
				tox = win->_maxx;   /* break condition */
				dx = (u_char *) &(win->_dx[y][x]);
				if (*dx & _PS || x >= win->_maxx)
				{
					y++;
					x = 0;
				}
				else if (*dx & _DBL2ND)
				{
					x--;
				}
					
			}
			else
			{
				dx = (u_char *) &(win->_dx[y][win->_maxx-1]);
				tox += (win->_maxx - x);
				y++;
				x = 0;
				if (*dx & _PS)
				{
					tox--;
				}
			}
		}
		toy++;
		tox = 0;
	}
	restbytes = win->_maxx * (win->_maxy - y - 1) + (win->_maxx - x);
	return(restbytes);
}


/*
**	IIFTdslDmySftL -- check where the cursor move to after it had moved
**			from the position at current position + nchars
**			to the current cursor position.
**
**	Parameters:
**		win	-- WINDOW structure.
**		nbytes	-- number of bytes to move right from current cursor
**			   position.
**		lasty, lastx -- the moved end of screen text.
**
**	Returns:
**		remain current cursor position.
**
**	History:
*/

i4
IIFTdslDmySftL(win, nchars, lasty, lastx)
WINDOW	*win;
i4	nchars;
i4	*lasty;
i4	*lastx;
{
	i4	y, x, toy, tox;
	u_char	*tmp, *dx;
	i4	movech;

	y = toy = win->_cury;
	x = tox = win->_curx;
	tmp = (u_char *) &(win->_y[y][x]);
	dx = (u_char *) &(win->_dx[y][x]);

	/*
	** move a pointer to the beginning position of shifting.
	*/
	while(nchars--)
	{
		if (*dx & _PS)
		{
			x = win->_maxx;
		}
		if (x >= win->_maxx)
		{
			y++;
			x = 0;
			tmp = (u_char *) &(win->_y[y][x]);
			dx  = (u_char *) &(win->_dx[y][x]);
		}
		/*
		** This routine is currently getting called with
		** nchars == 1, so the following code is acceptable.
		** Specifically, if routine is called when y==win->_maxy-1
		** and x==win->_maxx-1, and current location contains
		** a phantom space, then y gets set to win->_maxy, and
		** tmp gets set to &(win->_y[win->_maxy][0]), which
		** is out of range.  (bab)
		*/
		CMbyteinc(x, tmp);
		CMnext(tmp);
	}

	while (y < win->_maxy)
	{
		if (y == toy && x == tox)
		{
			return(FAIL);
		}
		while (x < win->_maxx)
		{
			if (x < tox)
			{
				movech = win->_maxx - tox;
				x += movech;
				dx = (u_char *) &(win->_dx[y][x]);
				if ((tox = tox+movech) >= win->_maxx)
				{
					toy++;
					tox = 0;
				}
				if (*dx & _DBL2ND)
				{
					x--;
				}
			}
			else /* (x >= tox) */
			{
				movech = win->_maxx - x;
				x += movech;
				tox += movech;
				if (win->_dx[y][x-1] & _PS)
				{
					tox--;
				}
				if (tox >= win->_maxx)
				{
					toy++;
					tox = 0;
				}
			}
		}
		y++;
		x = 0;
	}

	*lasty = toy;
	*lastx = tox;
	return(OK);
}


/*
**	use only in IIFTslShiftLeft()
**
**	FTchksftL -- Check whether up to 2 characters can get into window from
**			tail buffer or not.
**
**	Parameters:
**		win    -- WINDOW structure.
**		rfptr  -- RGRFLD structure.
**		nbytes -- number of bytes to move right from current cursor
**			  position.
**
**	Returns:
**		OK  -- if up to 2 characters (depends on how many characters
**			are there in tail buffer) can get.
**		FAIL -- there is NOT enough room on window.
**		changed current cursor position to where is the current
**		position after it had moved to left.
**
**	History:
*/

i4
FTchksftL(win, rgptr, nchars)
WINDOW	*win;
RGRFLD	*rgptr;
i4	nchars;
{
	i4	y, x, toy, tox;
	u_char	*tmp, *dx, *cp;
	i4	movech, restch;
	i4	nbytes;

	y = toy = 0;
	x = tox = 0;
	tmp = (u_char *) win->_y[y];
	dx = (u_char *) win->_dx[y];

	/*
	** move a pointer to the beginning position of shifting.
	*/
	while(nchars--)
	{
		if (*dx & _PS)
		{
			x = win->_maxx;
		}
		if (x >= win->_maxx)
		{
			y++;
			x = 0;
			tmp = (u_char *) &(win->_y[y][x]);
			dx  = (u_char *) &(win->_dx[y][x]);
		}
		CMbyteinc(x, tmp);
		CMnext(tmp);
	}

	while (y < win->_maxy && y <= win->_cury)
	{
		if (y == toy && x == tox)
		{
			return(FAIL);
		}
		while (x < win->_maxx)
		{
			if (x < tox)
			{
				movech = win->_maxx - tox;
				if (y == win->_cury)
				{
					restch = win->_curx - x;
					if (movech > restch)
					{
						tox += restch;
						break;	/* go out */
					}
				}
				x += movech;
				dx = (u_char *) &(win->_dx[y][x]);
				if ((tox = tox+movech) >= win->_maxx)
				{
					toy++;
					tox = 0;
				}
				if (*dx & _DBL2ND)
				{
					x--;
				}
			}
			else /* (x >= tox) */
			{
				movech = win->_maxx - x;
				if (y == win->_cury)
				{
					movech = win->_curx - x;
				}
				x += movech;
				tox += movech;
				if (y == win->_cury)
				{
					x = win->_maxx;
					break;
				}
				else
				{
					if (win->_dx[y][x-1] & _PS)
					{
						tox--;
					}
					if (tox >= win->_maxx)
					{
						toy++;
						tox = 0;
					}
				}
			}
		}
		y++;
		x = 0;
	}

	y = toy;
	x = tox;

	if (rgptr->tcur == NULL)
	{
		rgptr->tcur = rgptr->tend;
		*(rgptr->tcur) = ' ';
	}
	/* 'rgptr->tcur' should not be NULL at this point. */
	cp = (u_char *) rgptr->tcur;

	/*
	** is there enough space to input an input character on the end of line
	** a input character has already been saved into head of tail buffer.
	*/
	nbytes = CMbytecnt(cp);
	    /*
	    ** if are going to insert a Double Byte character at the last
	    ** position on a line (maxx-1, y), you have to insert a phantom
	    ** space into that position instead of the character.
	    */
	if (x + nbytes > win->_maxx)
	{
		if (y >= win->_maxy - 1)
		{
			return(FAIL);
		}
		y++;
		x = 0;
	}
	x += nbytes;
	    /*
	    ** check whether current line is full or not
	    */
	if (x > win->_maxx - 1)
	{
		if (y >= win->_maxy - 1)
		{
			return(FAIL);
		}
		y++;
		x = 0;
	}
	CMnext(cp);

	/*
	** check there is enough space to input a character which was on
	** cursor position. 
	** that character has already been saved into tail buffer.
	*/
	if (cp == (u_char *) rgptr->tend)
	{
		nbytes = 1;
	}
	else
	{
		nbytes = CMbytecnt(cp);
	}
	if (x + nbytes > win->_maxx)
	{
		if (y >= win->_maxy - 1)
		{
			return(FAIL);
		}
		y++;
		x = 0;
	}
	if (x + nbytes > win->_maxx)
	{
		return(FAIL);
	}

	win->_cury = toy;
	win->_curx = tox;
	return(OK);
}


/*
**	use only in IIFTslShiftLeft()
**
**	FTtltohd  --
**
**	Parameters:
**		win	-- WINDOW structure.
**		rfptr	-- RGRFLD structure.
**
**	Returns:
**
**		changed current cursor position to where's the cursor
**		    after move text.
**
**	History:
*/

VOID
FTtltohd(win, rgptr)
WINDOW	*win;
RGRFLD	*rgptr;
{
	i4	room;
	i4	bytecnt;
	u_char	*tcur = (u_char *) rgptr->tcur;
	u_char	*hcur = (u_char *) rgptr->hcur;

	room = win->_maxx * win->_maxy;
	if (rgptr->tcur == NULL)
	{
	    rgptr->tcur = rgptr->tend;
	    *(rgptr->tcur) = ' ';
	}
	bytecnt = CMbytecnt(rgptr->tcur);

	do
	{
		while (bytecnt--)
		{
			if (rgptr->hcur == NULL)
			{
				rgptr->hcur = rgptr->hbuf;
			}
			else if (rgptr->hcur == rgptr->hend)
			{
				FTrngxpand(rgptr, RG_H_ADD);
				(rgptr->hcur)++;
			}
			else
			{
				(rgptr->hcur)++;
			}
			*(rgptr->hcur) = *(rgptr->tcur)++;
		}
		if (rgptr->tcur > rgptr->tend)
		{
			rgptr->tcur = NULL;
			break;
		}
		bytecnt = CMbytecnt(rgptr->tcur);
	}
	while (rgptr->tcur != NULL && room < bytecnt);
}


/*
**	IIFTslShiftLeft -- shift screen data to left until 2 characters in the
**			tail buffer can get into window.
**
**	Parameters:
**		win -- WINDOW structure.
**		rgptr -- RGRFLD structure.
**
**	Returns:
**		NONE.
**		put characters into head buffer and shift screen data to left.
**		changed current cursor position to where is the current
**		position after it had moved to left.
**
**	History:
*/

VOID
IIFTslShiftLeft(win, rgptr, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
bool	reverse;
{
	i4	svy, svx;
	i4	nchars;
	i4	maxx, maxy, maxxy;
	i4	bytecnt;

	maxy = win->_maxy;
	maxx = win->_maxx;
	maxxy = maxx * maxy;
	svy = win->_cury;
	svx = win->_curx;

	/*
	** if the window has not enough room for input character and cursor.
	** before call this routine a input character has to put into
	** tail buffer.
	*/
	bytecnt = rgptr->tcur == NULL ? 1 : CMbytecnt(rgptr->tcur);
	if ( maxxy < bytecnt + 1)
	{
		FTtltohd(win, rgptr);
		nchars = 1;
	}
	else
	/*
	** if the window has more than one byte.
	*/
	{
		for (nchars = 1; FTchksftL(win, rgptr, nchars) != OK; nchars++)
		{
			/* noop */ ;
		}
		svy = win->_cury;
		svx = win->_curx;
		IIFTahAddtoHead(win, rgptr, nchars, reverse);
	}
	win->_cury = 0;
	win->_curx = 0;
	while(nchars--)
	{
		TDdelch(win);
	}
	win->_cury = svy;
	win->_curx = svx;
}


/*
**	IIFTtaTailAddstr -- add string to tail buffer.
**
**	Parameters:
**		rgptr -- RGRFLD structure.
**		str   -- string for adding.
**
**	Returns:
**		NONE.
**
**	History:
*/

/*
**  Add a string to the tail buffer.
**  Make sure to check that if character to add is a blank and
**  we have not added anything, then just return.
*/

VOID
IIFTtaTailAddstr(rgptr, str)
RGRFLD	*rgptr;
u_char	*str;
{
	u_char	    *startp;
	u_char	    *cp;

	startp = str;
	cp = startp + STlength((char *) str);
	if (rgptr->tcur == NULL)
	{
		for (CMprev(cp, startp); CMspace(cp) && cp > startp;
							CMprev(cp, startp))
		{
		}
		if (CMspace(cp))
		{
			return;
		}
		CMnext(cp);
	}
	/* put the pointer on last position of the string before the null */
	cp--;
	while (cp >= startp)
	{
		if (rgptr->tcur == NULL)
		{
			rgptr->tcur = rgptr->tend;
		}
		else if (rgptr->tcur == rgptr->tbuf)
		{
			FTrngxpand(rgptr, RG_T_ADD);
			(rgptr->tcur)--;
		}
		else
		{
			(rgptr->tcur)--;
		}
		*(rgptr->tcur) = *(cp--);
	}
}


/*
**	IIFTatAddToTail -- put 'nbytes' bytes from end of window to tail buffer.
**
**	Parameters:
**		win -- WINDOW structure.
**		rgptr  -- RGRFLD structure.
**		nbytes -- number of bytes to put into tail buffer
**				(includes count of phantom spaces).
**
**	Returns:
**		OK -- at least one character put into tail buffer.
**		FAIL -- NO characters put into tail buffer.
**
**	History:
*/

i4
IIFTatAddToTail(win, rgptr, nbytes)
WINDOW	*win;
RGRFLD	*rgptr;
i4	nbytes;
{
	i4	y, x;
	u_char	*tmp, *dx, *startp;

	y = win->_maxy;
	x = -1;
	while (nbytes > 0)
	{
		if (x < 0)
		{
		    /*
		    ** it should had checked before calling this routine
		    ** that there is enough room in the window 
		    ** for pushing 'nbytes' out into tail buffer.
		    */
			y--;
			x = win->_maxx - 1;
			startp = (u_char *) win->_y[y];
			tmp = (u_char *) &(win->_y[y][x]);
			tmp++;
			CMprev(tmp, startp);
			dx  = (u_char *) &(win->_dx[y][x]);
			if (*dx & _PS)
			{
				nbytes--;
				x--;
				CMprev(tmp, startp);
				if (rgptr->tcur != NULL)
				{
					break;
				}
				continue;
			}
			if (rgptr->tcur != NULL)
			{
				break;
			}
		}
		if (!CMspace(tmp) || nbytes <= 0)
		{
			break;
		}
		CMbytedec(nbytes, tmp);
		CMbytedec(x, tmp);
		CMprev(tmp, startp);
	}
	if (nbytes <= 0)
	{
		return(FAIL);
	}

	tmp = (u_char *) &(win->_y[y][x]);
	while(nbytes > 0)
	{
		if (x < 0)
		{
			if (--y < 0)
			{
				return(FAIL);
			}
			else
			{
				x = win->_maxx - 1;
				tmp = (u_char *) &(win->_y[y][x]);
				dx  = (u_char *) &(win->_dx[y][x]);
				if (*dx & _PS)
				{
					nbytes--;
					tmp--;
					x--;
				}
			}
		}
		if (rgptr->tcur == NULL)
		{
			rgptr->tcur = rgptr->tend;
		}
		else if (rgptr->tcur == rgptr->tbuf)
		{
			FTrngxpand(rgptr, RG_T_ADD);
			(rgptr->tcur)--;
		}
		else
		{
			(rgptr->tcur)--;
		}
		*(rgptr->tcur) = *tmp;
		nbytes--;
		x--;
		tmp--;
	}

	return(OK);
}


/*
**	IIFTahAddtoHead -- put 'nchars' characters to head buffer
**			   from beginning of window.
**
**	Parameters:
**		win	-- WINDOW structure.
**		rgptr	-- RGRFLD structure.
**		nchars	-- number of characters to put into head buffer
**				(exclude count of phantom spaces).
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		OK -- at least one character put into head buffer.
**		FAIL -- NO characters put into head buffer.
**
**	History:
**		28-apr-87 (bab)
**			The 'reverse' case of IIFTahAddtoHead() is really
**			just making use of the code that modifies
**			rgptr->hcur (nchars == 1).  The rest of the
**			code is pretty much ignored when reverse is TRUE.
*/

i4
IIFTahAddtoHead(win, rgptr, nchars, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
i4	nchars;
bool	reverse;
{
	i4	y = 0;
	i4	x = 0;
	i4	nbytes;
	u_char	*tmp, *dx;

	if (reverse)
		x = win->_maxx - 1;

	tmp = (u_char *) &(win->_y[0][x]);
	dx  = (u_char *) &(win->_dx[0][x]);
	while (nchars--)
	{
		if (x >= win->_maxx || *dx & _PS)
		{
			if (y + 1 >= win->_maxx)
			{
				return(FAIL);
			}
			y++;
			x = 0;
			tmp = (u_char *) &(win->_y[y][x]);
			dx  = (u_char *) &(win->_dx[y][x]);
		}
		nbytes = CMbytecnt(tmp);
		while (nbytes--)
		{
			if (rgptr->hcur == NULL)
			{
				rgptr->hcur = rgptr->hbuf;
			}
			else if (rgptr->hcur == rgptr->hend)
			{
				FTrngxpand(rgptr, RG_H_ADD);
				(rgptr->hcur)++;
			}
			else
			{
				(rgptr->hcur)++;
			}
			*(rgptr->hcur) = *tmp++;
			dx++;
			x++;
		}
	}
	return(OK);
}


/*
**	FTgetfromtl -- get as many characters as possible into window from
**			tail buffer.
**
**	Parameters:
**		win	-- WINDOW structure.
**		rgptr	-- RGRFLD structure.
**		reverse	-- indicate if field is right-to-left.
**
**	Returns:
**		NONE.
**		remain current cursor position of 'win' structure.
**
**	History:
**		28-apr-87 (bab)
**			The 'reverse' case of FTgetfromtl() is really
**			just making use of the code that modifies
**			rgptr->tcur.  The rest of the code is pretty
**			much ignored when reverse is TRUE.
*/

VOID
FTgetfromtl(win, rgptr, reverse)
WINDOW	*win;
RGRFLD	*rgptr;
bool	reverse;
{
	i4	y = win->_cury;
	i4	x = win->_curx;
	i4	nbytes;
	u_char	*tmp, *dx, *bufp;

	tmp = (u_char *) &(win->_y[y][x]);
	dx  = (u_char *) &(win->_dx[y][x]);
	if ((bufp = (u_char *) rgptr->tcur) == NULL)
	{
		return;
	}

	if (reverse)
		nbytes = 1;
	else
		nbytes = win->_maxx * (win->_maxy - y - 1) + (win->_maxx - x);

	while (nbytes)
	{
		if (x > win->_maxx - 1)
		{
			if (y >= win->_maxy - 1)
			{
				break;
			}
			y++;
			x = 0;
			tmp = (u_char *) win->_y[y];
			dx = (u_char *) win->_dx[y];
		}
		if (x + CMbytecnt(bufp) > win->_maxx)
		{
			*tmp = PSP;
			*dx = _PS;
			if (y >= win->_maxy - 1)
			{
				break;
			}
			y++;
			x = 0;
			tmp = (u_char *) win->_y[y];
			dx = (u_char *) win->_dx[y];
		}
		CMcpychar(bufp, tmp);
		CMbyteinc(x, bufp);
		*dx = '\0';
		if (CMdbl1st(tmp))
		{
			*(dx+1) = _DBL2ND;
		}
		CMbyteinc(tmp, bufp);
		CMbyteinc(dx, bufp);
		CMbyteinc(rgptr->tcur, bufp);
		CMbytedec(nbytes, bufp);
		CMnext(bufp);
		if (bufp > (u_char *) rgptr->tend)
		{
			rgptr->tcur = NULL;
			break;
		}
	}
}


/*
**	FTlastpos -- check whether current position is last position or not.
**
**	Parameters:
**		win	-- WINDOW structure.
**		op	-- operation code.
**		insmode	-- flag for checking insert mode or not.
**			    (this parameter needed for 'op'=fdopERR)
**		insp	-- character pointer to insert.
**			    (this parameter needed for 'op'=fdopERR)
**
**	Returns:
**		TRUE  -- current position is the last position.
**		FALSE -- current position is NOT the last position.
**
**	History:
*/

STATUS
FTlastpos(win, op, insmode, insp)
WINDOW	*win;
i4	op;
i4	insmode;
u_char	*insp;
{
	i4	y = win->_cury;
	i4	x = win->_curx;
	i4	maxy = win->_maxy;
	i4	maxx = win->_maxx;
	u_char	*curp, lastdx;

	curp = (u_char *) &(win->_y[y][x]);
	lastdx = win->_dx[maxy-1][maxx-1];

	switch(op)
	{
		case fdopRIGHT:
			return((x == maxx - 1)
				|| (x == maxx - 2 && CMdbl1st(curp))
				|| (x == maxx - 2 && lastdx & _PS)
				|| (x == maxx - 3 && CMdbl1st(curp)
						  && lastdx & _PS) );
		case fdopERR:
			if (insmode)
			{				
				return((x == maxx - 1)
					|| (x == maxx - 2 && CMdbl1st(insp))
					|| (x == maxx - 2 && CMdbl1st(curp))
					|| (x == maxx - 3 && CMdbl1st(insp)
							  && CMdbl1st(curp)
							  && lastdx & _PS) );
			}
			else
			{				
				return((x == maxx - 1)
					|| (x == maxx - 2 && CMdbl1st(insp))
					|| (x == maxx - 2 && lastdx & _PS)
					|| (x == maxx - 3 && CMdbl1st(insp)
							  && CMdbl1st(curp)
							  && lastdx & _PS) );
			}
	}
	return(FALSE);
}

/*
**  tddblutil.c
**
**  Copyright (c) 2004 Ingres Corporation
*/

/*
	 A.) The Rules to Handle the Double Bytes Chracters
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1.   General Rules

1.1.   For 8-bits characters
      * No change will be happend to manipulate characters so handle
	characters one by one. (cf. Fig. 1)

1.2.   For Kanji characters
      * The cursor never stop on K2 and PS. (cf. Fig. 2,3)

      * Treat a Kanji character as if it's an ASCII character
	except on overwriting mode.
	If in overwiting mode, overwite a character on current cursor position
	without relation to current displayed character.
	After overwriting a character, only 2nd byte of a character was leave
	then replace it by space.
						(cf. Fig. 7,8,9,10)

      * A PS will be displayed only on the right edge of line. (cf. Fig. 7,8)

2.   Rules for Cursor Movement

2.1.   For 8-bits characters
      * No change will be happend to manipulate characters so handle
	characters one by one. (cf. Fig. 1)

2.2.   For Kanji characters
      * The cursor go forward and is on K2 or PS
	then skip to next chracter.
					(cf. Fig. 2)

      * The cursor go backward and is on 
	   K2 then skip to previous K1 character.
	   PS then skip to previous character. 
					(cf. Fig. 3)

      * The cursor go upward or go downward and is on 
	   K2 then move to previous K1 character.
	   PS then move previous character.
						(cf. Fig. 4,5,6)

3.   Rules for Manipulating characters

3.1.   For 8-bits characters
      * No change will be happend to manipulate characters so handle
	characters one by one. (cf. Fig. 1)

3.2.   For Kanji characters
      * No Kanji half character will be displayed, so wrapping within lines,
	put a PS on the last column to force the Kanji character on the next
	line or in the tail buffer on range mode.
						(cf. Fig. 7,11,12)

      * After manipulating, insert, overwrite, delete and etc., a character,
	if the cursor move out of window, shift the window data until there
	are enough room to appere the character which is stopped by the cursor.
						(cf. Fig. 12)

4.   Rules for horizontal scrolling of the terminal screen

4.1.   For 8-bits characters
      * No change will be happend to manipulate characters so handle
	characters one by one. (cf. Fig. 1)

4.2.   For Kanji characters
      * The partial window will be invisible by horizontal scrolling of
	the terminal screen, so when a fragment of Kanji character remained
	on the edge of terminal screen, a space chracter put on it temporarily
	instead of a fragment of Kanji character until terminal screen changed.
						(cf. Fig. 13,14)



Kanji character			16 bit character
ASCII character			 8 bit character
K1				1st byte of a Kanji character
K2				2nd byte of a Kanji character
PS				phantom space

Fig.#
~~~~~

1       +--*--+					+---*-+
        |abcde|  input '?'			|ab?cd|        
        |fghij|                                 |efghi|j        
	+-----+					+-----+
	     

2       +--*--+					+-----+
        |abK1^|  move cursor -> (right)         |abK1^|        
        |K2K3c|                                 |K2K3c|         
	+-----+					+*----+
	     

3       +-----+					+--*--+
        |abK1^|  move cursor <- (left)          |abK1^|        
        |K2K3c|                                 |K2K3c|         
	+*----+					+-----+
	     

4       +-----+		     ^                  +--*--+
        |abK1^|  move cursor |  (up)            |abK1^|        
        |K2cK3|                                 |K2cK3|         
	+----*+					+-----+
	     


6       +-*---+					+-----+
        |abK1^|  move cursor |  (down)          |abK1^|        
        |K2cK3|              V                  |K2cK3|         
	+-----+					+*----+


7       +--*--+					+---*-+
        |abcK1|  input '?'                      |ab?c^|        
        |K2K3d|                                 |K1K2^|K3d      
	+-----+					+-----+
	     

8       +--*--+					+---*-+
        |abK1^|  overwrite by '?'               |ab? ^|
        |K2K3c|                                 |K2K3c|         
	+-----+					+-----+
	     

9       +-*---+					+---*-+
        |abK1^|  overwrite by 'K?'              |aK? ^|        
        |K2K3c|                                 |K2k3c|         
	+-----+					+-----+
	
     
10      +-----+					+-----+
        |abK1^|  overwrite by '?'               |abK1?|        
        |K2K3c|                                 |K2k3c|         
	+*----+					+*----+
	
     
11      +--*--+					+-*---+
        |abK1^|  delete a character             |aK1K2|
        |K2K3c|dK4        before cursor         |K3cd^|K4       
	+-----+					+-----+

	     
12      +-----+					+-----+
        |abK1^|  delete a character            a|bK1K2|        
        |K2K3c|K4K5       on cursor             |K3K4^|K5         
	+----*+					+---*-+
	     

13   +--------------+                             +--------------+
     |  +-----+	    |			       +""|---+          |
     |  |aK1K2|     |                          :aK| K2|          |
     |  |K3K4b|     |                          :K3|K4b|          |
     |  +----*+     |                          +""|--*+          |
     |              +                             |              |
     +--------------+                             +--------------+

	     
14                                                +--------------+
  +--------------------+                       +""|--------------|""""+
  |aK1K2K3K4K5K6K7K8K9^|                       :aK| K2K3K4K5K6K7 |8K9^:
  |K0K1K2K3K4K5K6K7K8K9|                       :K0|K1K2K3K4K5K6K7|K8K9:
  +--------------------+                       +""|--------------|""""+
                                                  |              |
                                                  +--------------+

*		current cursor position
^		phantom space
K1,K2,...,K?    Double byte characters
a,b,c,...,?	Single byte characters

**
*/



/*
		 B.) The KEYSTRUCT structure
		~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For getting a Double Bytes character from terminal, I modified
    routines for getting characters in
				 routine which returned KEYSTRUCT
	    [FT]FTGETCH.C	    FTgetc*,FTgetch,FTTDgetch,FTget_char,
				    FTwstrip,FTfstrip
	    [FT]FTKFE.C		    KFcntrl,KFplay
	    [TERMDR]GETCH.C	    TDgetch
	    [TERMDR]ITIN.C.	    ITnxtch*,ITin
	    [TERMDR]KEYS.C	    FKgetc*,FKnxtch*,FKexpmac,FKgetinput*
	    [TERMDR]TDDBLUTI.C	    TDgetKS*,TDsetKS.
				    (routines with '*' call TDgetKS)

	These routines return KEYSTRUCT pointer insted of 'nat'. Because of 
    handling a Double Bytes chracter as same as a single byte chracter.
    KEYSTRUCT structure was defined in [FRONT.HDR]TERMDR.H and this structure is

	    struct	_key_struct
	    {
		    i4		ks_fk;
		    u_char	ks_ch[3];
	    }
	    #define	KEYSTRUCT	struct	_key_struct.

	'ks_ch' contain character codes of both a Double bytes character and 
    a single byte character and finished by '\0' to be able to treat ks_ch 
    as string.
	'ks_fk' contains a code of function key. A function code is set when
    a function key was entered, otherwise 'ks_fk' is set by '\0'.
	The function to get this KEYSTRUCT structure is TDgetKS().
	In this function the substance of KEYSTRUCT structure is defined
    by static ...
	    static	KEYSTRUCT   KS;.
    And pointer of KEYSTRUCT passes by this function to upper functions.





	     C.) The Double Attributes Flag definition
	    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For data attributes of Double Bytes characters '_dx' was added into
    WINDOW structure. Now it uses for indicating a 2nd byte of a Double Bytes
    character and a phantom space. It defined in [FRONT.HDR]TERMDR.H like below

	    struct  _win_st
	    {
		    ========
		    char    **_dx;
	    }.
    And bit pattern is
	    #define	_PS	    01	    for phantom spaces
	    #define	_DBL2ND	    02	    for 2nd byte 
						of Double Byte characters
	    #define	_DBLMASK    03	    for masking bit pattern, it uses
						to recognise that a character
						has _PS or _DBL2ND attribute.

*/



# include	<compat.h>
# include	<kst.h>
# include	<tdkeys.h> 
# include	<cm.h>
# include	<si.h>	/* include for using 'EOF' */
# include	<termdr.h> 

/*
**  TDDBLUTIL.C -- utilities of Double Dytes characters handling.
**
**	Defines:
**		TDxinc()	    return number of bytes to go forward.
**		TDxdec()	    return number of bytes to go backward.
**		TDfixpos()	    return number of bytes to go legal position.
**				    (never stop 2nd-byte and phantom space.)
**				    ps. this function was used only in the
**				    TDmvcrsr().
**		TDmvcrsr()	    move cursor up, down, left and right on
**				    the window.
**		TDaddchar()	    overwrite a character.
**		TDgetKS()	    return pointer of KEYSTRUCT structure.
**		TDsetKS()	    set a value of input into KEYSTRUCT
**				    structure.
**
**	History:
**		10/02/86 (KY)  -- Created for Double Byte characters.
**		22-apr-87 (bab)
**			Added reverse field-direction code.
**		19-jun-87 (bab)	Code cleanup.
**		10-sep-87 (bab)
**			Changed from use of ERR to FAIL (for DG.)
**	10/22/88 (dkh) - Performance changes.
**	11/10/88 (dkh) - Temporarily turn off DOUBLEBYTE.
**	02/21/89 (dkh) - Added include of cm.h and cleaned up ifdefs.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**	TDxinc -- change to number of bytes from number of characters to
**		  go forward.
**
**	Parameters:
**		win -- WINDOW structure.
**		y,x -- start position to move.
**		repts -- number of characters to go forward.
**
**	Returns:
**		a number of bytes to forward the cursor.
**		if the position after moving is over the maximum number of x
**		then return the maximum number of x.
**
**	History:
*/

i4
TDxinc(win, y, x, repts)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
reg	i4	repts;
{
	i4	cntx;
	u_char	*cp = (u_char *) &(win->_y[y][x]);

	for (cntx = 0; repts > 0 && x < win->_maxx; --repts)
	{
		CMbyteinc(cntx, cp);
		CMbyteinc(x, cp);
		CMnext(cp);
	}

	if (x == win->_maxx - 1 && (win->_dx[y][x] & _PS))
		cntx++;

	return	cntx;
}


/*
**	TDxdec -- change to number of bytes from number of characters to
**		  go backward.
**
**	Parameters:
**		win -- WINDOW structure.
**		y,x -- start position to move.
**		repts -- number of chracters to go backward.
**
**	Returns:
**		a number of bytes to backward the cursor.
**		if the position after moving is before start position
**		then return a negative value.
**
**	History:
*/

i4
TDxdec(win, y, x, repts)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
reg	i4	repts;
{
	i4	cntx;
	u_char	*dx = (u_char *) &(win->_dx[y][x]);

	for (--dx, cntx = 0; repts > 0 && x >= 0; --dx, ++cntx, --x, --repts)
	{
		if (*dx & _DBL2ND)
		{
			--dx; ++cntx; --x;
		}
	}

	if (x == win->_maxx - 1 && (win->_dx[y][x] & _PS))
		--cntx;

	return	cntx;
}


/*
**	TDfixpos -- get a number of bytes to put the cursor on legal position.
**		    this function was used only in the TDmvcrsr().
**
**		If the cursor move to a 2nd byte of Double Byte character or
**		a Phantom Space, then it has to move more left.
**		Caller has to make sure that 'x' is in legal range.
**
**	Parameters:
**		win -- WINDOW structure.
**		y,x -- current cursor position.
**
**	Returns:
**		number of bytes the cursor has to move left.
**		0 -- cursor is on legal character (1st byte of kanji or
**			single byte of character).
**		1 -- cursor is on 2nd byte of character or phantom space
**			which followed by non double byte character.
**		2 -- cursor is on phantom space which followed by double
**			byte character.
**
**	History:
*/

i4
TDfixpos(win, y, x)
reg	WINDOW	*win;
reg	i4	y;
reg	i4	x;
{
	i4	cntx = 0;
	u_char	*dx = (u_char *) &(win->_dx[y][x]);

	if (y >= win->_maxy || y < 0)
		return cntx;
    
	if (*dx & _PS)
	{
		--dx; ++cntx;
	}

	if (*dx & _DBL2ND)
		++cntx;

	return cntx;
}


/*
**	TDmvcrsr -- move the cursor in window
**
**	Parameters:
**		op  -- direction of movement.
**		win -- WINDOW structure to handle.
**		repts -- number of characters (8-bits or 16-bits)/lines to move.
**		reverse -- indicate if movement is to occur in a standard,
**			   left-to-right, or in a reversed (right-to-left)
**			   field.  Only used for left/right movement.
**
**	Returns:
**		FAIL -- go outside of window.
**		OK   -- move to specified position.
**
**	History:
**		11/04/86 (KY)  -- Created for Double Byte characters.
**		22-apr-87 (bab)
**			added reverse arg and its use for TDmove calls
**			to support right-to-left fields.
**		10/02/87 (dkh) - Changed TDmvcursor to TDmvcrsr for check7.
*/

i4
TDmvcrsr(op, win, repts, reverse)
reg	i4	op;
reg	WINDOW	*win;
reg	i4	repts;
bool		reverse;
{
	i4	y = win->_cury;
	i4	x = win->_curx;
	i4	nbytes;
	
	switch (op)
	{
	    case TDmvLEFT:
		/*
		** move cursor left in field
		*/
		    nbytes = TDxdec(win, y, x, repts);
		    if (reverse)
		    {
			if (TDrmove(win, y, x - nbytes) == FAIL)
			{
				return(FAIL);
			}
		    }
		    else
		    {
			if (TDmove(win, y, x - nbytes) == FAIL)
			{
				return(FAIL);
			}
		    }
		    /*
		    ** if current position is on Phantom Space or 2nd Byte of
		    ** a Double Byte character then the cursor has to move
		    ** further left
		    */
		    if (win->_dx[win->_cury][win->_curx] & _DBLMASK)
		    {
			    nbytes = TDfixpos(win, win->_cury, win->_curx);
			    TDmove(win, win->_cury, win->_curx - nbytes);
		    }
		    break;

	    case TDmvRIGHT:
		 /*
		 ** move cursor right in field
		 */
		    nbytes = TDxinc(win, y, x, repts);
		    if (reverse)
		    {
			if (TDrmove(win, y, x + nbytes) == FAIL)
			{
			       return(FAIL);
			}
		    }
		    else
		    {
			if (TDmove(win, y, x + nbytes) == FAIL)
			{
			       return(FAIL);
			}
		    }
		    break;

	    case TDmvUP:
		 /*
		 ** move cursor up in field
		 */
		    if (TDmove(win, y - repts, x) == FAIL)
		    {
			    return(FAIL);
		    }
		    /*
		    ** if current position is on Phantom Space or 2nd Byte of
		    ** a Double Byte character then the cursor has to move
		    ** more left
		    */
		    if (win->_dx[win->_cury][win->_curx] & _DBLMASK)
		    {
			    nbytes = TDfixpos(win, win->_cury, win->_curx);
			    TDmove(win, win->_cury, win->_curx - nbytes);
		    }
		    break;

	    case TDmvDOWN:
		 /*
		 ** move cursor down in field
		 */
		    if (TDmove(win, y + repts, x) == FAIL)
		    {
			    return(FAIL);
		    }
		    /*
		    ** if current position is on Phantom Space or 2nd Byte of
		    ** a Double Byte character then the cursor has to move
		    ** more left
		    */
		    if (win->_dx[win->_cury][win->_curx] & _DBLMASK)
		    {
			    nbytes = TDfixpos(win, win->_cury, win->_curx);
			    TDmove(win, win->_cury, win->_curx - nbytes);
		    }
		    break;

	    default:
		/*
		**  return FAIL becouse 'op' operation code is wrong
		**	and do nothing.
		*/
		    return(FAIL);
	}

	return(OK);
}


/*
**	TDaddchar -- overstrike a character at current cursor position
**
**	Parameters:
**		win -- WINDOW structure to handle.
**		cp  -- a character pointer to be overstruck.
**		reverse -- indicate if field is right-to-left.
**
**	Returns:
**		NONE
**
**	History:
**		01/29/87 (KY)  -- Created for Double Byte characters.
**		27-apr-87 (bab)
**			Added 'reverse' parameter.  When reverse'd,
**			passes through directly to TDraddstr (same
**			effect as would happen if the 'if' statement only
**			surrounded the TDaddstr/TDraddstr, but faster).
*/

i4
TDaddchar(win, cp, reverse)
WINDOW	*win;
u_char	*cp;
bool	reverse;
{
	/*
	** overstrike a character to window buffer
	** without taking care of what character is on the current cursor 
	** position.
	** after that when only 2nd byte of a Double Byte character is left
	** on next, replace it by space.
	**
	**	    +--*-----+				+---*----+
	**	    |K1K2K3ab|	overstrike with '?'	|K1? K3ab|
	**	    +--------+				+--------+
	**						     |---|
	**						       |
	**						       V
	**						  NOT changed
	*/

	i4	x, y;
	i4	lastch;

	if (reverse)
	{
		lastch = TDraddstr(win, cp);
	}
	else
	{
		y = win->_cury;
		x = win->_curx;
	
		/*
		** check whether a Double Byte character was input at last
		** position of window (_maxx - 1, _maxy - 1) or not
		*/
		if (x + CMbytecnt(cp) > win->_maxx)
		{
			if (y + 1 >= win->_maxy)
			{
				return(FAIL);
			}
			win->_y[y][x] = PSP;
			win->_dx[y][x] = _PS;
			if (win->_firstch[y] == _NOCHANGE)
			{
				win->_firstch[y]= win->_lastch[y] = x;
			}
			else if (x < win->_firstch[y])
			{
				win->_firstch[y] = x;
			}
			else if (x > win->_lastch[y])
			{
				win->_lastch[y] = x;
			}
			y++;
			x = 0;
		}
	
		win->_cury = y;
		win->_curx = x;
		/*
		**  if a single character overstruck a Double Byte character
		**  and there is a phantom space on end of upper line, then
		**  overwrite a phantom space with a single character and
		**  leave cursor on current position.
		**
		**	+--------+			    +--------+
		**	|abcdefg^|	overstrike 	    |abcdefg?|
		**	|K1      |	  '?' -->	    |K1      |
		**	+*-------+			    +*-------+
		*/
		if (CMbytecnt(cp) == 1 && x == 0 && y > 0
				&& win->_dx[y-1][win->_maxx-1] & _PS)
		{
			win->_cury -= 1;
			win->_curx = win->_maxx - 1;
			win->_dx[win->_cury][win->_curx] = '\0';
		}
		/* add a character */
		if ((lastch = TDaddstr(win, cp)) == FAIL)
		{
			return(FAIL);
		}
	
		y = win->_cury;
		x = win->_curx;
		if (win->_dx[y][x] & _DBL2ND)
		{
			win->_y[y][x] = ' ';
			win->_dx[y][x] = '\0';
			if (win->_firstch[y] == _NOCHANGE)
			{
				win->_firstch[y]= win->_lastch[y] = x;
			}
			else if (x < win->_firstch[y])
			{
				win->_firstch[y] = x;
			}
			else if (x > win->_lastch[y])
			{
				win->_lastch[y] = x;
			}
		}
		if (win->_dx[y][x] & _PS)
		{
			if (y >= win->_maxy-1)
			{
				return(FAIL);
			}
			win->_cury += 1;
			win->_curx = 0;
		}
	}
	return(lastch);
}


/*
**	TDgetKS -- Get a KEYSTRUCT and return its pointer.
**		    A 'KEYSTRUCT' structure 'KS' is declared by "static" type
**		    actually then use this entity at all other places.
**
**	Parameters:
**
**	Returns:
**		pointer of KEYSTRUCT structure.
**		
**	History:
*/

KEYSTRUCT *
TDgetKS()
{
	static	    KEYSTRUCT	kstrct;

	kstrct.ks_fk = 0;
	kstrct.ks_ch[0] = '\0';
	return(&kstrct);
}


/*
**	TDsetKS -- Set a value of 'key' into 'ks'.
**
**	Parameters:
**		ks  -- KEYSTRUCT structure.
**		key -- valiable to be set into structure.
**		pos -- 0 : for initializing 'ks'
**		       1 : for setting 'key' to 'ks->ks_ch[0]'
**		       2 : for setting 'key' to 'ks->ks_ch[1]'
**			    as 2nd byte of a Double byte character.
**
**	Returns:
**		pointer to structure KEYSTRUCT
**		
**	History:
*/

KEYSTRUCT *
TDsetKS(ks, key, pos)
KEYSTRUCT   *ks;
i4	    key;
i4	    pos;
{
	if (pos == 1 || pos == 0)
	{
		ks->ks_ch[0] = '\0';
		ks->ks_fk = 0;
		if (pos == 1)
		{
			if (key >= KEYOFFSET)
			{
				ks->ks_fk = key;
			}
			else
			{
				ks->ks_ch[0] = (u_char)key;
				ks->ks_ch[1] = '\0';
			}
		}
	}
# ifdef DOUBLEBYTE
	else if (pos == 2)
	{
		ks->ks_ch[1] = (u_char)key;
		ks->ks_ch[2] = '\0';
	}
# endif /* DOUBLEBYTE */
	return(ks);
}

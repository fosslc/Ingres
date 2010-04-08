/*
**      Copyright (c) 1990 Ingres Corporation
*/

# include       <compat.h>
# include       <ex.h>
# include       <te.h>
# include       <si.h>
# include       <er.h>
# include       <nm.h>
# include       "telocal.h"

# define  _NOCHANGE	-1

GLOBALREF FILE *IIte_fpout;

/*	TEbrefresh - begin termdr refresh()
**
**	Set TEok2write switch so that no actual writes to the screen take
**	place while inside termdr refresh()
**
**	History:
**	    08-aug-1999 (mcgem01)
**		Replace nat with i4.
*/

void
TEbrefresh(void)
{
	++TEok2write;	      /* Increments to account for recursion */
}

/*	TEerefresh - end termdr refresh()
**
**	Clear TEok2write switch so writes to the screen again take place.
**	Then write the entire curscr WINDOW to the console buffer
**	and position the cursor.
**
** History:
**	06-oct-1995 (canor01)
**	    Call SetConsoleActiveScreenBuffer unconditionally, since the
**	    current screen buffer may belong to another process (as when
**	    running SEP).
**	18-oct-1995 (canor01)
**	    If stdout has been redirected (IIte_fpout != NULL), then
**	    don't write to Windows console either.
**      18-Oct-95 (tutto01)
**          Modified the repeated calls to console functions which cause OS
**          to repaint the console window and cause flicker.
**          Added SEP sections to add repeated calls to
**          SetConsoleActiveScreenBuffer so output is correct when SEP takes
**          control.
*/

static	short	restRemt(int i, unsigned char * cp, unsigned char * ap);

STATICDEF	unsigned char *	TEdispaddr = 0;		/* TE line display buffer */
STATICDEF	unsigned char	last_c = ' ';		/* char at TEcols * TErows */
STATICDEF	unsigned char	last_a = '\0';		/* attrib at TEcols * TErows */
# ifdef SEP
STATICDEF       i2              SepConsoleControl = 0;
# endif             /* SEP */
void
TEerefresh(
i4	line,			/* line number (0 relative) to position cursor on */
i4	col,			/* colm number (0 relative) to position cursor on */
char **	_y,			/* ptrs to ptrs to chars on each real screen line */
char **	_da,		/* ptrs to ptrs to display attribs for each line  */
i4 *	_firstch)	/* _firstch[n] is col of 1st chg on n-th line	  */
{
	register int	i, j;
	unsigned int	i1;	/* counter to 1st char to write in TE line buffer */
	unsigned char *	d1;	/* pts to 1st char to write in TE line buffer */
	unsigned char *	d;	/* pts into TE line display buffer */
	unsigned char *	cp;	/* pts to next char to display	 */
	unsigned char *	ap;	/* pts to next attr to display	 */
	unsigned char   ca;	/* current attribute			 */
	unsigned char   pa;	/* previous attribute			 */

	if	(--TEok2write == 0 && IIte_fpout == NULL)
	{
		if (!TEdispaddr)
		{
			TEdispaddr = GlobalLock(GlobalAlloc(GMEM_MOVEABLE, TEcols + 2));
			memset(TEdispaddr, ' ', TEcols + 2);    /* initialize to blanks */
		}
		if (bNewTEconsole)
		{
		COORD	coordChar;
                char    * sepctrl = NULL;

			bNewTEconsole = FALSE;
			coordChar.X = TEcols;
			coordChar.Y = TErows;
			SetConsoleScreenBufferSize(hTEconsole, coordChar);
			coordChar.X = 0;
			coordChar.Y = 0;
			FillConsoleOutputCharacter(hTEconsole, (TCHAR) ' ',
					TEcols * TErows, coordChar, &i);
                        SetConsoleActiveScreenBuffer(hTEconsole);
# ifdef SEP
                        NMgtAt(ERx("II_SEP_CONTROL"),&sepctrl);
                        if (sepctrl && *sepctrl)
                            SepConsoleControl = 1;
# endif             /* SEP */
		}
# ifdef SEP
                if(SepConsoleControl)
                    SetConsoleActiveScreenBuffer(hTEconsole);
# endif             /* SEP */
		TEcci.bVisible = FALSE;
		SetConsoleCursorInfo(hTEconsole, &TEcci);
		for	(j = -1; ++j < TErows; )
		{
			d = TEdispaddr + 1;
			if	(TEwriteall)
				i = 0;
			else	
			{
				if	(_firstch[j] >= TEcols)
				{
					continue;
				}
				ap = _da[j];						/*US*/
				for	(i = -1; ++i < _firstch[j]; )	/*US+*/
				{									/*US*/
					if	(*(ap+i) & _UNLN)			/*US*/
						break;						/*US*/
				}									/*US*/
			}
			/* i now set to 1st underlined char or to _firstch[j] */
			d += i;
			cp = _y[j] + i;
			ap = _da[j] + i;
			*(d-1) = ' ';
			SetConsoleTextAttribute(hTEconsole, pa = TExlate[*ap]);
			d1 = d;
			TEposcur(j, i1 = i);
			while	(i++ < TEcols)
			{
				if ((*ap & _UNLN) && (*cp == ' ')	/*US*/
				 && ((i==1) || ((*(cp-1) == ' ')	/*US*/
						&& (*(d-1) == '_'))			/*US*/
					    || !(*(ap-1) & _UNLN) 		/*US*/
					    || restRemt(i,cp+1,ap+1)))	/*US*/
				{									/*US*/
					cp++;							/*US*/
					*d++ = '_';						/*US*/
				}									/*US*/
				else	*d++ = *cp++;
				if ((ca = TExlate[*ap++]) != pa)
				{
					WriteConsole(hTEconsole, d1, i - i1 - 1, &i1, NULL);
					SetConsoleTextAttribute(hTEconsole, pa = ca);
					d1 = d - 1;
					TEposcur(j, i1 = i - 1);
				}
			}
			if ((j == TErows - 1) && (i > TEcols))
			{
				if ((last_c == *(d-1)) && (last_a == ca))
				{
					--i;
				}
				else
				{
					last_c = *(d-1);
					last_a = ca;
				}
			}
			WriteConsole(hTEconsole, d1, i - i1 - 1, &i1, NULL);
		}
		TEposcur(line, col);
		TEcci.bVisible = TRUE;
		SetConsoleCursorInfo(hTEconsole, &TEcci);
		TEwriteall = 0;
	}
}

static
short
restRemt(			/* REST of field chars aRe EMpTy */	/*US*/
int		i,
unsigned char * cp,
unsigned char * ap)
{
	while	(i++ < TEcols)
	{
		if	(!(*ap++ & _UNLN))
			break;
		if	(*cp++ != ' ')
			return(0);
	}
	return(1);
}

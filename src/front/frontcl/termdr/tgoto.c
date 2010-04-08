/*
**      Copyright (c) 2004 Ingres Corporation
*/
/*
**  TGOTO.c - Create goto string for terminal
**
**  Routine to perform cursor addressing.
**  Cursormotion is a string containing printf type escapes to allow
**  cursor addressing.  We start out ready to print the destination
**  line, and switch each time we print row or column.
**  The following escapes are defined for substituting row/column:
**
**	%d	as in printf
**	%2	like %2d
**	%3	like %3d
**	%.	gives %c hacking special case characters
**	%+x	like %c but adding x first
**
**	The codes below affect the state but don't use up a value.
**
**	%>xy	if value > x add y
**	%*x	Multiply current parameter by x
**	%r	reverses row/column
**	%s	subtract one from line count (new ann arbor, 6/84 - dkh)
**	%i	increments row/column (for one origin indexing)
**	%n	exclusive-or all parameters with 0140 (Datamedia 2500)
**	%%	gives %
**	%B	BCD (2 decimal digits encoded in one byte)
**	%D	Delta Data (backwards bcd)
**	%O	Omron  (added 25 Jan 82 - jen)
**
**  all other characters are ``self-inserting''.
*/

/*
**  tgoto.c
**
**  History:
**	25-jun-87 (bab)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	10/03/87 (dkh) - Added "%*x" for vt340 color support.
**	10/22/88 (dkh) - Performance changes.
**	11/01/88 (dkh) - More performance changes.
**	04/29/92 (dkh) - Changed the CTRL macro to make ANSI C happy.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<er.h>

#define		CTRL(c)	(c & 037)

GLOBALREF	i4	IITDgotolen;

char *
TDtgoto(cursormotion, destcol, destline)
char	*cursormotion;
i4	destcol;
i4	destline;
{
	static	char	result[40];
	static	char	added[40];
		char	*cp = cursormotion;
	reg	char	*dp = result;
	reg	char	c;
		i4	oncol = 0;
	reg	i4	which = destline;

	IITDgotolen = 0;

	if (cp == 0)
	{
	  toohard:
		return(ERx("OOPS"));
	}

	added[0] = 0;
	while (c = *cp++)
	{
		if (c != '%')
		{
			*dp++ = c;
			continue;
		}
		switch (c = *cp++)
		{

			case 'i':
				destcol++;
				destline++;
				which++;
				continue;

			case '2':
				*dp++ = (char) which / 10 | '0';
				*dp++ = (char) which % 10 | '0';
				oncol = 1 - oncol;
				which = oncol ? destcol : destline;
				continue;

#ifdef CM_N
			case 'n':
				destcol ^= 0140;
				destline ^= 0140;
				goto setwhich;
#endif

			case 'd':
				if (which < 10)
					goto one;
				if (which < 100)
					goto two;
				/* fall into... */

			case '3':
				*dp++ = (char) (which / 100) | '0';
				which %= 100;
				/* fall into... */

		/*	case '2':	*/
two:	
				*dp++ = (char) which / 10 | '0';
one:
				*dp++ = (char) which % 10 | '0';
swap:
				oncol = 1 - oncol;
setwhich:
				which = oncol ? destcol : destline;
				continue;

#ifdef CM_GT
			case '>':
				if (which > *cp++)
					which += *cp++;
				else
					cp++;
				continue;
#endif

			case '*':
				which *= *cp++;
				continue;

			case '+':
				which += *cp++;
				/* fall into... */

			case '.':
/*casedot:*/
				/*
				**  This code is worth scratching your head at for a
				**  while.  The idea is that various weird things can
				**  happen to nulls, EOT's, tabs, and newlines by the
				**  tty driver, arpanet, and so on, so we don't send
				**  them if we can help it.
				**
				**  Tab is taken out to get Ann Arbors to work, otherwise
				**  when they go to column 9 we increment which is wrong
				**  because bcd isn't continuous.  We should take out
				**  the rest too, or run the thing through more than
				**  once until it doesn't make any of these, but that
				**  would make termlib (and hence pdp-11 ex) bigger,
				**  and also somewhat slower.  This requires all
				**  programs which use termlib to stty tabs so they
				**  don't get expanded.  They should do this anyway
				**  because some terminals use ^I for other things,
				**  like nondestructive space.
				*/
				if (which == 0 || which == CTRL('d') || which == '\t' || which == '\n')
				{
					if (oncol || UP) /* Assumption: backspace works */
						/*
					 	* Loop needed because newline happens
					 	* to be the successor of tab.
					 	*/
						do
						{
							STcat(added, oncol ? (BC ? BC : ERx("\b")) : UP);
							which++;
						} while (which == '\n');
				}
				*dp++ = (char) which;
				goto swap;

			case 'r':
				oncol = 1;
				goto setwhich;

			case 's':
				destline--;
			/*	which--;	*/
				continue;

			case '%':
				*dp++ = c;
				continue;

#ifdef CM_B
			case 'B':
				which = (which/10 << 4) + which%10;
				continue;
#endif

#ifdef CM_D
			case 'D':
				which = which - 2 * (which%16);
				continue;
#endif

			case 'O':
				*dp++ = (char) ((which >> 4) & 0xf) | 0x40;
				*dp++ = (char) ( which & 0xf ) | 0x40;
				oncol = 1 - oncol;
				which = oncol ? destcol : destline;
				continue;

			default:
				goto toohard;
		}
	}
	if (*added)
	{
		STcopy(added, dp);
	}
	else
	{
		*dp = EOS;
		IITDgotolen = dp - result;
	}
	return(result);
}

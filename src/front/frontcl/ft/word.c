/*
**	word.c - Code to move to next/previous word.
**
**	Copyright (c) 2004 Ingres Corporation
**
**	FDWORD -
**		move forward in a window to the beginning 
**		of the first word found.  Word is considered
**		any set of alphanumerics together or non-
**		alphanumerics together.  The current set of 
**		coordinates is set to the forward of the
**		word.
**
**	Parameters:
**		win - window to seek beginning of the word
**
**	Side effects:
**		coordinates of the window changed
**
**	Error messages:
**		none
**
**	Calls:
**		TDmove
**
**  History:
**	08/19/85 (DKH) -- Added support for sideway scrolling in a field.
**	10/20/86 (KY)  -- Changed CH.h to CM.h and added macro define CURPTR
**	07-may-87 (bab)
**		Added code for moving a word in an RL field.  Done for
**		Hebrew project.
**	06/19/87 (dkh) - Code cleanup.
**	2-sep-87 (mgw) Bug # 13142
**		prevent AV by not referencing through frm->frfld if it
**		doesn't exist.
**	10-sep-87 (bab)
**		Changed from use of ERR to FAIL (for DG.)
**	09-nov-87 (bab)
**		Added code to handle movement in a scrolling field
**		buffer.
**	09-feb-88 (bruceb)
**		Scrolling field flag now in fhd2flags; can't use most
**		significant bit of fhdflags.
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
# include	"ftframe.h"
# include	"ftrange.h"
# include	<cm.h>
# include	<scroll.h>

# define	CURCHAR(win)	((win)->_y[(win)->_cury][(win)->_curx])
# define	CURPTR(win)	(&((win)->_y[(win)->_cury][(win)->_curx]))
# define	FORE		1
# define	AFT		-1

# define	SKIP1		0
# define	SKIP2		1


FUNC_EXTERN	RGRFLD	*FTgetrg();

/*
**  Have reached boundary on window need to check if the window has
**  scrolled and do the appropriate thing.
*/

VOID
FTrgskip(frm, fldno, win, dir, phase, reverse)
FRAME	*frm;
i4	fldno;
WINDOW	*win;
i4	dir;
i4	phase;
bool	reverse;
{
	char	*hcur;
	char	*hbuf;
	char	*tcur;
	char	*tend;
	i4	i;
	RGRFLD	*rgptr;

	rgptr = FTgetrg(frm, fldno);

	hcur = rgptr->hcur;
	hbuf = rgptr->hbuf;
	tcur = rgptr->tcur;
	tend = rgptr->tend;
	if (dir == AFT)
	{
		/*
		**  Have not scrolled foward yet, just return.
		*/

		if (hcur == NULL)
		{
			return;
		}
		i = -1;
		if (phase == SKIP1)
		{
			/*
			**  Skip spaces.
			*/

			for (; hcur >= hbuf;
				CMprev(hcur, hbuf), CMbytedec(i, hcur))
			{
				if (!CMwhite(hcur))
				{
					break;
				}
			}
		}

		/*
		**  Skip non-spaces.
		*/

		for (; hcur >= hbuf; CMprev(hcur, hbuf), CMbytedec(i, hcur))
		{
			if (CMwhite(hcur))
			{
				break;
			}
		}
		FTrngshft(rgptr, ++i, win, reverse);

		return;
	}
	else	/* dir == FORE */
	{
		/*
		**  Have not scrolled backward yet, just return.
		*/

		if (tcur == NULL)
		{
			return;
		}
		i = 1;
		if (phase == SKIP1)
		{
			/*
			**  Skip non-spaces.
			*/

			for (; tcur <= tend; CMbyteinc(i, tcur), CMnext(tcur))
			{
				if (CMwhite(tcur))
				{
					break;
				}
			}
		}

		/*
		**  Skip spaces.
		*/

		for (; tcur <= tend; CMbyteinc(i, tcur), CMnext(tcur))
		{
			if (!CMwhite(tcur))
			{
				break;
			}
		}

		if (tcur > tend)
		{
			/*
			**  No real word out in the tail buffer,
			**  just return.
			*/

			return;
		}

		FTrngshft(rgptr, i, win, reverse);
	}
}

/*
**  Skip over spaces or non-spaces in window in direction "dir" starting
**  at the current window position.  If "what" is TRUE, then skip
**  spaces.  FALSE means to skip non-spaces.
*/

i4
FTwskip(win, dir, what, reverse)
WINDOW	*win;
i4	dir;
bool	what;
bool	reverse;
{
	if (reverse)
	{
		do
		{
			if (((dir == FORE) ?
				TDrmvleft(win, 1) : TDrmvright(win, 1)) == FAIL)
			{
				return(FAIL);
			}
		} while (what ? CMwhite(CURPTR(win)) : !CMwhite(CURPTR(win)) );
	}
	else
	{
		do
		{
			if (((dir == FORE) ?
				TDmvright(win, 1) : TDmvleft(win, 1)) == FAIL)
			{
				return(FAIL);
			}
		} while (what ? CMwhite(CURPTR(win)) : !CMwhite(CURPTR(win)) );
	}

	return(OK);
}


VOID
FTword(frm, fldno, win, dir)
FRAME	*frm;
i4	fldno;
WINDOW	*win;
i4	dir;
{
	bool		inrngmd = FALSE;
	bool		reverse = FALSE;
	SCROLL_DATA	*scroll_fld = NULL;
	FIELD		*fld;
	FLDHDR		*hdr;

	if (frm->frmflags & fdRNGMD)
	{
		inrngmd = TRUE;
	}

	if (frm->frfld != NULL)	/* Begin fix for 13142 */
	{
		/*
		** 13142 - currently only need to check for hebrew reverse
		** handling if you're on a data field (frm->frfld != NULL)
		** since hebrew reverse handling is only implemented there.
		** If hebrew reverse handling ever becomes available on trim
		** fields or title parts of fields, then the fact that
		** frm->frfld is NULL in the VIFRED edit case must be corrected.
		*/
		fld = frm->frfld[fldno];
		hdr = (*FTgethdr)(fld);
		if (hdr->fhdflags & fdREVDIR)
		{
			reverse = TRUE;
		}			/* End fix for 13142 */
		if (hdr->fhd2flags & fdSCRLFD)
		{
			scroll_fld = IIFTgssGetScrollStruct(fld);
		}
	}

	if (dir == AFT)
	{
		/* Going backwards. */
		if (scroll_fld)
		{
			IIFTpwsPrevWordScroll(win, scroll_fld, reverse);
		}
		else	/* all others */
		{
			/* First skip spaces.  */
			if (FTwskip(win, dir, TRUE, reverse) == FAIL)
			{
				if (inrngmd)
				{
					FTrgskip(frm, fldno, win, dir, SKIP1,
						reverse);
				}
				/* Can't move any further in the window */
			}
			else
			{
				/* Now skip over non-spaces. */
				if (FTwskip(win, dir, FALSE, reverse) == FAIL)
				{
					if (inrngmd)
					{
						FTrgskip(frm, fldno, win, dir,
							SKIP2, reverse);
					}
					/* At edge of the window */
				}
				else
				{
					/*
					**  Move forward to first character in
					**  word.
					*/
					if (reverse)
					{
						TDrmove(win, win->_cury,
							win->_curx - 1);
					}
					else
					{
						TDmove(win, win->_cury,
							win->_curx + 1);
					}
				}
			}
		}
	}
	else	/* dir == FORE */
	{
		/* Going forward. */
		if (scroll_fld)
		{
			IIFTnwsNextWordScroll(win, scroll_fld, reverse);
		}
		else
		{
			/* Skip non-spaces if current char is not a space. */
			if (!CMwhite(CURPTR(win)))
			{
				if (FTwskip(win, dir, FALSE, reverse) == FAIL)
				{
					if (inrngmd)
					{
						FTrgskip(frm, fldno, win, dir,
							SKIP1, reverse);
					}
					return;
				}
			}
	
			/*
			**  Skip spaces to get to next word.  Current
			**  position should be it.
			*/
	
			if (FTwskip(win, dir, TRUE, reverse) == FAIL)
			{
				if (inrngmd)
				{
					FTrgskip(frm, fldno, win, dir, SKIP2,
						reverse);
				}
			}
		}
	}
}

/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** GETCH.c
**
** The read character routines for the frame driver.
** These are included here so that a recovery file can be written as
** the characters are read.
**
** DEFINES
**	FTgetc(file)
**				get a character from a file
**
**	FTgetch()		reads a character from curscr
**
**	FTTDgetch(win)		reads a character from the window
**	    WINDOW  *win
**
**	FTget_char()		reads a character from stdin
**
**	FTe_getch(win)		reads a character from a window
**	    WINDOW	*win	Throws out controls
**
** HISTORY
** 
**    	stolen 9/13/82 (jrc) from vifred
**	modified 10/9/85 (dpr) adding keystroke file editor calls
**	12/08/86(KY) -- changed return value from 'nat' to 'KEYSTRUCT'
**			for Double Bytes characters handling.
**			and added CM.h for calling CMdbl1st().
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	06/18/88 (dkh) - Integrated MVS changes.
**	12/27/89 (dkh) - Changed keystroke saving for window-view commands.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"ftframe.h"
# include	<tdkeys.h> 
# include	<te.h> 
# include	<si.h>
# include	<cm.h>
# include	<uigdata.h>

#ifdef SEP

# include	<tc.h>

GLOBALREF TCFILE    *IIFTcommfile;

#endif /* SEP */

GLOBALREF bool	fdrecover;


FUNC_EXTERN KEYSTRUCT	*TDgetch();

KEYSTRUCT   *
FTgetc(file)
FILE	*file;
{
    reg	KEYSTRUCT   *ks;

    if (file != stdin)
    {
	ks = TDgetKS();
	TDsetKS(ks, SIgetc(file), 1);
	if (CMdbl1st(ks->ks_ch))
	{
		TDsetKS(ks, SIgetc(file), 2);
	}
	if (fdrecover)
	{
		if (!ks->ks_fk)
		{
			TDrcvstr(ks->ks_ch);
		}
	}
    }
    else
    {
	ks = (*in_func)(stdin);
    }
    if (!KY && fdrecover)
    {
	if (!ks->ks_fk)
	{
		TDrcvstr(ks->ks_ch);
	}
    }
    return(ks);
}

/*
** History
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
*/
KEYSTRUCT *
FTgetch()
{
	reg KEYSTRUCT	*ks;
	KEYSTRUCT   *FTwstrip();

	ks = TDgetch(stdscr);
	if (!KY && fdrecover)
	{
		if (!ks->ks_fk)
		{
			TDrcvstr(ks->ks_ch);
		}
	}
	if (!IIUIfedata()->testing)
		return(ks);

#ifdef SEP
	if (IIFTcommfile != NULL && ks->ks_ch[0] == TC_EOQ)
	{
		TCputc(TC_EQR,IIFTcommfile);
		TCflush(IIFTcommfile);
		return(FTgetch());
	}
#endif /* SEP */

	if (ks->ks_ch[0] != LEFT_COMMENT)
		return(ks);
	/* We are reading a comment from a keystroke file */
	ks = FTwstrip(stdscr);
	return(ks);
}

/*
** History
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
*/
KEYSTRUCT *
FTTDgetch(win)
WINDOW	*win;
{
	KEYSTRUCT   *ks;
	KEYSTRUCT   *FTwstrip();

# ifdef DATAVIEW
	if (IIMWimIsMws())
	{
		if (IIMWduiDoUsrInput(&ks) == FAIL)
			return((KEYSTRUCT *) NULL);
		else
			return(ks);
	}
# endif	/* DATAVIEW */

	ks = TDgetch(win);
	if (!KY && fdrecover)
	{
		if (!ks->ks_fk)
		{
			TDrcvstr(ks->ks_ch);
		}
	}
	if (!IIUIfedata()->testing)
		return(ks);

#ifdef SEP
	if (IIFTcommfile != NULL && ks->ks_ch[0] == TC_EOQ)
	{
		TCputc(TC_EQR,IIFTcommfile);
		TCflush(IIFTcommfile);
		return(FTTDgetch(win));
	}
#endif /*  SEP */

	if (ks->ks_ch[0] != LEFT_COMMENT)
		return(ks);
	/* We are reading a comment from a keystroke file */
	ks = FTwstrip(win);
	return(ks);
}


/*
**  FTwstrip - strip comment from window
*/

KEYSTRUCT   *
FTwstrip(window)
WINDOW	*window;
{
	KEYSTRUCT   *ks;

	for (;;)
	{
		do
		{
		ks = TDgetch(window);
			if (!KY && fdrecover)
			{
				TDrcvstr(ks->ks_ch);
			}
		}
		while (ks->ks_ch[0] != RIGHT_COMMENT);
		ks = TDgetch(window);
		if (!KY && fdrecover)
		{
			if (!ks->ks_fk)
			{
				TDrcvstr(ks->ks_ch);
			}
		}
		if (ks->ks_ch[0] != LEFT_COMMENT)
		{
			return(ks);
		}
	}
}

KEYSTRUCT   *
FTfstrip()
{
	KEYSTRUCT   *ks;

	for (;;)
	{
		do
		{
			ks = (*in_func)(stdin);
			if (!KY && fdrecover)
			{
				TDrcvstr(ks->ks_ch);
			}
		} while (ks->ks_ch[0] != RIGHT_COMMENT);
		ks = (*in_func)(stdin);
		if (!KY && fdrecover)
		{
			if (!ks->ks_fk)
			{
				TDrcvstr(ks->ks_ch);
			}
		}
		if (ks->ks_ch[0] != LEFT_COMMENT)
		{
			return(ks);
		}
	}
}

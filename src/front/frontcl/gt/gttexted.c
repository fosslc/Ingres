/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<te.h>
# include	<cm.h>
# include	<termdr.h>
# include	<graf.h>
# include	<ft.h>
# include	<kst.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"
# include	<ug.h>

/**
** Name:	gttexted.c -	Graphics System Text Edit Module.
**
** Description:
**	This module handles editing of text in the Graphics system.  It defines:
**
**	GTtexted
**
** History:
**	85/09/30  15:09:43  bobm
**		Initial revision.
**
**	86/01/13  16:38:13  wong
**		Replaced reference of 'G_mrow' with 'G_lines' ("G_MROW").
**
**	86/01/28  10:45:22  bobm
**		remove unused declaration.
**
**	86/02/04  11:45:39  bobm
**		move some routines to gthchar.c - fix escape key
**		truncation problem.
**
**	86/02/05  16:53:35  bobm
**		ifdef out edit on screen code (TEXTONSCREEN)
**
**	86/03/12  14:56:23  wong
**		Name change.
**
**	86/03/24  18:01:30  bobm
**		GTstat -> GTstl
**
**	86/04/15  12:02:36  bobm
**		Restrict text string to 78 characters
**
**	6/4/86 (bruceb)	- use printability check and \r, \n, instead
**			  of hardwired ascii numbers (still have
**			  \010 (^H) in the code).
**			  Add new arg to GTtexted() to specify
**			  maximum length of typed string.
**			  (fix for bug 9335).
**	23-oct-86 (bruceb)
**		remove spurious extern of G_frame.
**	9-feb-89  (Mike S.)
**			- Add check for non-standard backspace character.
**	17-feb-89  (Mike S.)
**			- Pass proper character to CMwhite.
**	25-mar-89 (Mike S)
**			- Add GTlock calls for DG.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	28-mar-90 (bruceb)
**		Added locator support.  Added to the ifdef'd-out TEXTONSCREEN
**		waited messages and made all locations illegal when user is
**		entering text.
**      21-jan-93 (pghosh)
**              Modified COORD to IICOORD. This change was supposed to be 
**              done by leighb when the ftdefs.h file was modified to have 
**              IICOORD.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for mode_msg() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/*
** MAX_TXT_LEN == 78 (arbitrarily) since GKS limitation on message
** lengths is 80
*/
# define	MAX_TXT_LEN	78

extern char	*G_tc_cm, *G_tc_ce, *G_tc_dc, *G_tc_nd, *G_tc_bc;
FUNC_EXTERN i4  GTchout();
extern i4	G_lines;
extern i4	G_cols;

static i4	mode_msg();
static bool	Insert = FALSE; /* insert mode */

# ifdef TEXTONSCREEN
static bool Termwarn = TRUE;	/* bad terminal warning on first call */
# endif

/*
**	We always return a newly stored string when we're done
**	editting.  This makes life simpler for "undo" operations
**	in the editor than if we wrote over an old copy of the string
**	if shorter.  We use a little bit more storage this way, which
**	will get reclaimed at GRtxtfree ()
**
**	if ext = NULL, we intend to edit on the menu line.
*/
char *
GTtexted (str, ext, maxilength)
char	*str;
EXTENT	*ext;
i4	maxilength; /* maximum length of the typed text; if 0, use default */
{
	i4	row, col, lmax, maxlen, idx, i, len;
	i4	code, kval;
	bool	kflag;
	char	bufr[81], *TDtgoto(), *GRtxtstore();
	FRS_EVCB	evcb;
	KEYSTRUCT	*key;
	KEYSTRUCT	*GTgetch();
	u_char	c;
	IICOORD	done;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	idx = 0;
/*
**	we always edit on the menu line now.  TEXTONSCREEN ifdef's in
**	the capability to edit in the box on the screen.
*/
# ifdef TEXTONSCREEN
	if (ext != NULL)
	{
		lmax = GThchstr(str, ext, &row, &col);

		if (lmax < 8)
		{
			done.begy = done.endy = G_MROW;
			done.begx = 0;
			done.begx = COLS - 1;
			IITDpciPrtCoordInfo(ERx("GTtexted"),
			    ERx("done"), &done);
			GTmsg (ERget(F_GT0015_Box_too_small));
			for (;;)
			{
			    key = GTgetch();
			    if (key->ks_ch[0] == '\r' || key->ks_ch[0] == '\n')
			    {
				break;
			    }
			    if (key->ks_fk == KS_LOCATOR)
			    {
				if (key->ks_p1 == done.begy)
				{
				    IITDposPrtObjSelected(ERx("done"));
				    break;
				}
				else
				{
				    FTbell();
				}
			    }
			}
			lmax = 0;
		}
		if (lmax < 0 && Termwarn)
		{
			done.begy = done.endy = G_MROW;
			done.begx = 0;
			done.begx = COLS - 1;
			IITDpciPrtCoordInfo(ERx("GTtexted"),
			    ERx("done"), &done);
			GTmsg (ERget(F_GT0016_Bad_Alpha));
			for (;;)
			{
			    key = GTgetch();
			    if (key->ks_ch[0] == '\r' || key->ks_ch[0] == '\n')
			    {
				break;
			    }
			    if (key->ks_fk == KS_LOCATOR)
			    {
				if (key->ks_p1 == done.begy)
				{
				    IITDposPrtObjSelected(ERx("done"));
				    break;
				}
				else
				{
				    FTbell();
				}
			    }
			}
			Termwarn = FALSE;
		}
		if (lmax <= 0)
		{
			GTmsg (ERx(""));
			row = G_MROW;
			col = 0;
			lmax = G_cols - 2;
		}
		else
		{
			GTflbox(ext,0);
			GTmsg(ERx(""));
		}
	}
	else
	{
		GTmsg (ERx(""));
		row = G_MROW;
		col = 0;
		lmax = G_cols - 2;
	}
#else
	GTmsg (ERx(""));
	row = G_MROW;
	col = 0;
	lmax = G_cols - 2;
#endif

	if (maxilength == 0)
		maxilength = MAX_TXT_LEN;

	if (lmax > maxilength)
		lmax = maxilength;

	mode_msg (row);
	STcopy (str, bufr);
	TDtputs(TDtgoto(G_tc_cm, col, row), 1, GTchout);
	TEwrite(bufr, (i4) (len = maxlen = STlength(bufr)));
	TDtputs(TDtgoto(G_tc_cm, col, row), 1, GTchout);

	if (len > lmax)
	{
		bufr[lmax] = '\0';
		len = maxlen = lmax;
	}

	idx = 0;
	for (i=len; i < lmax; ++i)
		bufr[i] = ' ';

	for (;;)
	{
		if (len > maxlen)
			maxlen = len;

		key = GTgetch();

		/* if a printable character, handle it */
		if (key->ks_fk == 0 && ! CMcntrl(key->ks_ch))
		{
			if (idx >= len)
				len = idx + 1;
			if (Insert)
			{
				for (i=len; i > idx; --i)
					bufr[i] = bufr[i-1];
				bufr[idx] = (key->ks_ch)[0];
				if (len < lmax)
					++len;
				TEwrite (bufr+idx, (i4) (len-idx));
				if (idx < (lmax-1))
					++idx;
				TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
				continue;
			}
			bufr[idx] = (key->ks_ch)[0];
			TEput (bufr[idx]);
			if (idx < (lmax-1))
			{
				++idx;
			}
			else
			{
				if (G_tc_bc == (char *)NULL || *G_tc_bc == EOS)
					TEput ('\010');
				else
					TDtputs( G_tc_bc, 1, GTchout );
			}
			continue;
		}

		if (FKmapkey(key, NULL, &code, &kval, &kflag, &c, &evcb) != OK)
		{
			FTbell();
			continue;
		}

		switch (code)
		{
		case fdopRUB:
			if (idx == 0)
			{
				FTbell();
				break;
			}
			--idx;
			if (G_tc_bc == (char *)NULL || *G_tc_bc == EOS)
				TEput ('\010');
			else
				TDtputs( G_tc_bc, 1, GTchout );
			/* fall through */
		case fdopDELF:
			if (idx >= len)
				break;
			bufr[len] = ' ';
			for (i=idx; i < len; ++i)
				bufr[i] = bufr[i+1];
			TEwrite(bufr+idx, (i4) (len-idx));
			--len;
			TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			break;
		case fdopMODE:
			if (Insert)
				Insert = FALSE;
			else
				Insert = TRUE;
			mode_msg(row);
			TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			break;
		case fdopLEFT:
			if (idx <= 0)
			{
				FTbell();
				break;
			}
			--idx;
			if (G_tc_bc == (char *)NULL || *G_tc_bc == EOS)
				TEput ('\010');
			else
				TDtputs( G_tc_bc, 1, GTchout );
			break;
		case fdopRIGHT:
			if (idx >= (lmax-1))
			{
				FTbell();
				break;
			}
			++idx;
			if (G_tc_nd != NULL && *G_tc_nd != '\0')
				TDtputs(G_tc_nd, 1, GTchout);
			else
				TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			break;
		case fdopBKWORD:
			if (idx <= 0)
			{
				FTbell();
				break;
			}
			i = idx;
			--idx;
			while (CMwhite(bufr + idx) && idx > 0)
				--idx;
			while (!CMwhite(bufr + idx) && idx > 0)
				--idx;
			if (CMwhite(bufr + idx))
				++idx;
			if (idx == i)
			{
				FTbell();
				break;
			}
			TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			break;
		case fdopWORD:
			if (idx >= len)
			{
				FTbell();
				break;
			}
			while (!CMwhite(bufr + idx) && idx < len)
				++idx;
			while (CMwhite(bufr + idx) && idx < len)
				++idx;
			TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			break;
		case fdopCLRF:
			for (i=0; i < len; ++i)
				bufr[i] = ' ';
			TDtputs(TDtgoto(G_tc_cm, col, row), 1, GTchout);
			TEwrite (bufr, (i4) len);
			TDtputs(TDtgoto(G_tc_cm, col, row), 1, GTchout);
			idx = len = 0;
			break;
		case fdopRET:
			len = idx;	/* fall through to menu key case */
		case fdopMENU:
			bufr[len] = '\0';
			len = STtrmwhite(bufr);
			if (row != G_MROW)
				GThchclr (row, col, maxlen);
			str = GRtxtstore(bufr);
			GTstlcat(ERx(""));
# ifdef DGC_AOS
			GTcsunl();
# else
			GTcursync();
# endif
			return (str);
		default:
			TDtputs(TDtgoto(G_tc_cm, col+idx, row), 1, GTchout);
			FTbell();
			break;
		}
	}
}

static mode_msg(row)
i4  row;
{
	char *mode,bufr[FE_PROMPTSIZE];

	if (Insert)
		mode = ERget(F_GT0017_Insert);
	else
		mode = ERget(F_GT0018_Overstrike);

	if (row == G_MROW)
		IIUGfmt(bufr, FE_PROMPTSIZE,
				ERget(F_GT0019_on_menu_line), 1, mode);
	else
		IIUGfmt(bufr, FE_PROMPTSIZE,
				ERget(F_GT001A_Mode_format), 1, mode);
	GTstlcat (bufr);
}

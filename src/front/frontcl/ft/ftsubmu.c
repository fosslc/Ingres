/*
**  ftsubmu.c
**
**  Routines to display submenus as a pop-up style menu.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**
**  7/2/85 -
**    Created. (dkh)
**              07/10/86 (a.dea) -- Initialize MUswin & MUsdwin.
**	06/19/87 (dkh) - Code cleanup.
**	10-nov-88 (bruceb)
**		AS THIS CODE CURRENTLY EXISTS, IT WILL NOT WORK WITH TIMEOUT.
**		Since it is currently unused, this is not a problem.
**	09/23/89 (dkh) - Porting changes integration.
**	22-mar-90 (bruceb)
**		AS THIS CODE CURRENTLY EXISTS, NO LOCATOR SUPPORT PROVIDED.
**		Since it is currently unused, this is not a problem.
**	04-dec-95 (chimi03)
**		Removed optimization for hp8_us5 due to internal error from
**		the compiler.  The compiler error was "internal error 6590".
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	19-jan-1999 (muhpa01)
**		Removed NO_OPTIM for hp8_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<ctrl.h>
# include	<menu.h>
# include	<termdr.h>
# include	<tdkeys.h>
# include	<kst.h>

FUNC_EXTERN	KEYSTRUCT	*FTTDgetch(); 

GLOBALREF       WINDOW  *MUswin;
GLOBALREF       WINDOW  *MUsdwin;


i4
FTsmuget(win)
WINDOW	*win;
{
	i4		row = 0;
	i4		maxy;
	KEYSTRUCT	*keystr;
	i4		ch;

	TDtouchwin(win);
	TDrowhilite(win, row, (u_char) _RVVID);
	TDrefresh(win);
	maxy = win->_maxy - 1;
	for (;;)
	{
		keystr = FTTDgetch(win);  /* get a character from the window */
		ch = keystr->ks_ch[0];

		if (ch == MENUCHAR)
		{
			return((i4) -1);
		}

		switch (ch)		/* switch on that character */
		{
			case UPKEY:
			case ctrlK:
			case 'k':
			case 'K':
				if (row > 0)
				{
					TDrowhilite(win, row, (u_char) 0);
					row--;
					TDrowhilite(win, row, (u_char) _RVVID);
					TDmove(win, (i4) row, (i4) 0);
					TDrefresh(win);
				}
				break;

			case DOWNKEY:
			case ctrlJ:
			case 'j':
			case 'J':
				if (row < maxy)
				{
					TDrowhilite(win, row, (u_char) 0);
					row++;
					TDrowhilite(win, row, (u_char) _RVVID);
					TDmove(win, (i4) row, (i4) 0);
					TDrefresh(win);
				}
				break;

			case '\r':
			/*
				TDrowhilite(win, row, (u_char) 0);
			*/
				return(row);

			default:
				break;		  
		}
	}
}



u_char *
FTsubmenuinp(mu, width)
MENU	mu;
i4	width;
{
	i4		maxx;
	i4		maxy;
	i4		i;
	i4		muitem;
	u_char	*retval;
	struct	com_tab	*ct;

	maxy = mu->mu_last;
	maxx = width;
	MUswin->_begy = LINES - 6 - maxy;
	MUswin->_begx = ((COLS - maxx - 4)/2) - 1;
	MUswin->_maxx = maxx + 4;
	MUswin->_maxy = maxy + 4;
	TDerase(MUswin);
	if (TDsubwin(MUswin, MUswin->_maxy - 2, MUswin->_maxx - 2,
		MUswin->_begy + 1, MUswin->_begx + 1, MUsdwin) == NULL)
	{
		return(NULL);
	}
	TDbox(MUsdwin, '|', '-', '+');
	if (TDsubwin(MUswin, MUswin->_maxy - 4, MUswin->_maxx - 4,
		MUswin->_begy + 2, MUswin->_begx + 2, MUsdwin) == NULL)
	{
		return(NULL);
	}

	ct = mu->mu_coms;
	for (i = 0; i < maxy; i++, ct++)
	{
		MEcopy(ct->ct_name, (u_i2) STlength(ct->ct_name), MUsdwin->_y[i]);
	}

	TDtouchwin(MUswin);
	TDrefresh(MUswin);

	muitem = FTsmuget(MUsdwin);
	if (muitem < 0)
	{
		retval = NULL;
	}
	else
	{
	/*
		STcopy(mu->mu_coms[muitem].ct_name, FTsmubuf);
		retval = (u_char *) FTsmubuf;
	*/
		retval = (u_char *) mu->mu_coms[muitem].ct_name;
	}
	TDerase(MUswin);
	TDrefresh(MUswin);
	return(retval);
}

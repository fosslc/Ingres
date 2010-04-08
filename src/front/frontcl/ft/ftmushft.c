/*
**  ftmushift.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  9/21/84 -
**    Stolen from original menudr for FT (dkh)
**  4/22/85 -
**    Added calls to FDdmpmsg for enhanced .scr output (drh)
**  5/8/85 -
**    Added support for long menus. (dkh)
**  5/10/85 -
**    Added support for lotus style of menus. (dkh)
**  5/31/85 -
**    Added support for backward compatibility with old style menu. (dkh)
**    Do not have to update menu dumping stuff since our products
**    will always be using the new stuff.  However, if a customer
**    re-linked and knows about the -D flag, then he has problems.
**  6/1/85 -
**    Spruced up uerror messages. (dkh)
**  6/21/85 -
**    Added support for VIFRED/RBF type of menus. (dkh)
**  7/3/85 -
**    Added support for special submenu display. (dkh)
**  7/12/85 -
**    Support for shifting long menus and lotus style input placed here. (dkh)
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**  09-nov-88 (bruceb)
**	WHEN THIS FILE IS UPDATED TO WORK IN 6.X, THEN NEED TO ADD
**	TIMEOUT SUPPORT.
**  06-may-92 (leighb) DeskTop Porting Change: added mubutton
**  23-sep-96 (mcgem01)
**      Global data moved to ftdata.c
**  28-Mar-2005 (lakvi01)
**	Corrected function prototypes.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<ctrl.h>
# include	<menu.h> 
# include	<omenu.h>
# include	<termdr.h> 
# include	<tdkeys.h>
# include	<cm.h> 
# include	<si.h>
# include	<st.h>
# include	<te.h>
# include	<er.h>

GLOBALREF	i4	mubutton;
GLOBALREF	i4	mutype;
GLOBALREF	i4	mu_where;
GLOBALREF	bool	mu_statline;
GLOBALREF	bool	mu_popup;

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	u_char	*FTuserinp();
extern	bool		Rcv_disabled;
extern	FILE		*FTdumpfile;
FUNC_EXTERN	u_char	*FTsubmenuinp();


# define	DISP_FIRST	(i4) 0
# define	DISP_LAST	(i4) 1
# define	NO_DISP		(i4) 2

# define	SET_MOVE	(i4) 0
# define	TAB_MOVE	(i4) 1

# define	NO_ITEMS	(i4) 0
# define	ONE_ITEM	(i4) 1
# define	DUP_ITEMS	(i4) 2



# ifdef LOTUSINP
i4
FTltssearch(mu, start, str, newitem)
MENU	mu;
i4	start;
char	*str;
i4	*newitem;
{
	struct com_tab	*comtab;
	i4		i;
	i4		length;
	i4		last;
	i4		end;
	i4		matchitem = -1;
	i4		retval = NO_ITEMS;
	char		*cp;
	bool		firsttime;

	if (*str != '\0')
	{
		firsttime = FALSE;
		last = mu->mu_last;
		if (start == 0)
		{
			end = last;
			firsttime = TRUE;
		}
		else
		{
			end = start;
		}
		comtab = mu->mu_coms;
		length = STlength(str);
		for (i = start; (!firsttime || i != end); i++)
		{
			if (i == end)
			{
				firsttime = TRUE;
			}
			if (i == last)
			{
				i = 0;
			}
			cp = comtab[i].ct_name;
			if (STbcompare(str, length, cp, STlength(cp), TRUE) == 0)
			{
				if (matchitem == -1)
				{
					matchitem = i;
					retval = ONE_ITEM;
				}
				else
				{
					retval = DUP_ITEMS;
					break;
				}
			}
		}
	}
	*newitem = matchitem;
	return(retval);
}


VOID
FTltsfset(mu, start, curitem)
MENU	mu;
i4	start;
i4	*curitem;
{
	struct com_tab	*comtab;
	i4		first;
	i4		second;
	i4		last;
	i4		i;
	i4		ofirst;

	comtab = mu->mu_coms;
	last = mu->mu_last;
	ofirst = mu->mu_dfirst;
	if (comtab[start].ct_flags & CT_BOUNDARY)
	{
		mu->mu_dfirst = i = start;
	}
	else
	{
		for (i = start; i > 0; i--)
		{
			if (comtab[i].ct_flags & CT_BOUNDARY)
			{
				break;
			}
		}
		mu->mu_dfirst = i;
	}
	for (i++; i < last; i++)
	{
		if (comtab[i].ct_flags & CT_BOUNDARY)
		{
			break;
		}
	}
	mu->mu_dsecond = i;
	if (ofirst != mu->mu_dfirst)
	{
		FTdispmenu(mu);
		FTltsbld(mu, NO_DISP, SET_MOVE);
	}
	*curitem = start;
}


VOID
FTltsbld(mu, disp_which, move_type)
MENU	mu;
i4	disp_which;
i4	move_type;
{
	struct com_tab	*comtab;
	WINDOW		*win;
	i4		i;
	i4		j;
	i4		pos = 2;

	if ((j = mu->mu_dsecond) != mu->mu_last)
	{
		j++;
	}
	if ((i = mu->mu_dfirst) != 0)
	{
		pos++;
	}
	for (comtab = mu->mu_coms; i < j; i++)
	{
		comtab[i].ct_begpos = pos;
		pos += STlength(comtab[i].ct_name);
		comtab[i].ct_endpos = pos - 1;
		pos += 2;
	}
	if (disp_which == NO_DISP)
	{
		return;
	}
	if (disp_which == DISP_FIRST)
	{
		if ((i = mu->mu_dfirst) != 0 && move_type == TAB_MOVE)
		{
			i++;
		}
	}
	else
	{
		if (((i = mu->mu_dsecond) == mu->mu_last) || (move_type == TAB_MOVE))
		{
			i--;
		}
	}
	win = mu->mu_window;
	TDhilite(win, comtab[i].ct_begpos, comtab[i].ct_endpos, (u_char) _RVVID);
	TDtouchwin(win);
	TDmove(win, 0, comtab[i].ct_begpos);
	TDrefresh(win);
}

u_char *
FTlotusinp(mu)
MENU	mu;
{
	u_char	*cp;
	u_char	*in;
	struct com_tab	*comtab;
	WINDOW		*win;
	i4		firstitem;
	i4		curitem;
	i4		lastitem;
	i4		newitem;
	i4		hilited = 0;
	i4		search_stat;
	u_char	buf[256];
	bool		selected = FALSE;
	bool		short_menu = FALSE;

	if (mu->mu_dsecond == 0)
	{
		mu->mu_dsecond = mu->mu_last;
		short_menu = TRUE;
	}
	FTltsbld(mu, DISP_FIRST, SET_MOVE);
	win = mu->mu_window;
	curitem = firstitem = mu->mu_dfirst;
	lastitem = mu->mu_dsecond;
	comtab = mu->mu_coms;
	in = buf;
	for (;;)
	{
		*in = FTTDgetch(win);	/* get a character from the window */
		if (*in == MENUCHAR)
		{
			TDhilite(win, comtab[curitem].ct_begpos, comtab[curitem].ct_endpos, (u_char) 0);
			TDtouchwin(win);
			TDrefresh(win);
			if (short_menu)
			{
				mu->mu_dsecond = 0;
			}
			return(NULL);
		}
		switch (*in)		/* switch on that character */
		{
		/*
			case MENUCHAR:
				break;
		*/

			case '>':
				if (short_menu)
				{
					break;
				}
				FTshiftmenu(mu, *in);
				FTltsbld(mu, DISP_FIRST, SET_MOVE);
				curitem = firstitem = mu->mu_dfirst;
				lastitem = mu->mu_dsecond;
				in = buf;
				*in = '\0';
				break;

			case '<':
				if (short_menu)
				{
					break;
				}
				FTshiftmenu(mu, *in);
				FTltsbld(mu, DISP_FIRST, SET_MOVE);
				curitem = firstitem = mu->mu_dfirst;
				lastitem = mu->mu_dsecond;
				in = buf;
				*in = '\0';
				break;

			case ctrlW:
			case ctrlR:
				TDrefresh(curscr);
				break;

			case ctrlI:
				if (!short_menu && (curitem == mu->mu_last - 1 || curitem == lastitem))
				{
					FTshiftmenu(mu, '>');
					FTltsbld(mu, DISP_FIRST, TAB_MOVE);
					curitem = firstitem = mu->mu_dfirst;
					if (curitem != 0)
					{
						curitem++;
					}
					lastitem = mu->mu_dsecond;
					in = buf;
					*in = '\0';
					break;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) 0);
				if (short_menu && (curitem == mu->mu_last - 1))
				{
					curitem = 0;
				}
				else
				{
					curitem++;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) _RVVID);
				TDtouchwin(win);
				TDmove(win, 0, comtab[curitem].ct_begpos);
				TDrefresh(win);
				in = buf;
				*in = '\0';
				break;

			case ctrlP:
				if (!short_menu && curitem == firstitem)
				{
					FTshiftmenu(mu, '<');
					FTltsbld(mu, DISP_LAST, TAB_MOVE);
					firstitem = mu->mu_dfirst;
					curitem = lastitem = mu->mu_dsecond;
					curitem--;
					in = buf;
					*in = '\0';
					break;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) 0);
				if (short_menu && curitem == 0)
				{
					curitem = mu->mu_last - 1;
				}
				else
				{
					curitem--;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) _RVVID);
				TDtouchwin(win);
				TDmove(win, 0, comtab[curitem].ct_begpos);
				TDrefresh(win);
				in = buf;
				*in = '\0';
				break;

			case '\r':
				if (FTdumpfile)
				{
					TDdumpwin(curscr, FTdumpfile, TRUE);
				}
				selected = TRUE;
				hilited = curitem;
				break;

			default:
				if (CMcntrl(in))
				{
					TEput(BELL);
					TEflush();
					break;		  
				}
				*++in = '\0';
				search_stat = FTltssearch(mu, curitem, buf, &newitem);
				if (search_stat == NO_ITEMS)
				{
					in--;
					TEput(BELL);
					TEflush();
					break;
				}
				else if (search_stat == ONE_ITEM)
				{
					hilited = curitem;
					curitem = newitem;
					selected = TRUE;
					break;
				}
				if (newitem == curitem)
				{
					break;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) 0);
				if (mu->mu_dsecond == 0)
				{
					curitem = newitem;
				}
				else
				{
					FTltsfset(mu, newitem, &curitem);
					firstitem = mu->mu_dfirst;
					lastitem = mu->mu_dsecond;
				}
				TDhilite(win, comtab[curitem].ct_begpos,
					comtab[curitem].ct_endpos, (u_char) _RVVID);
				TDtouchwin(win);
				TDmove(win, 0, comtab[curitem].ct_begpos);
				TDrefresh(win);
				break;
		}
		if (selected)
		{
			break;
		}
	/*
		TDrefresh(win);
	*/
	}
	if (short_menu)
	{
		mu->mu_dsecond = 0;
	}
	TDhilite(win, comtab[hilited].ct_begpos, comtab[curitem].ct_endpos, (u_char) 0);
	TDtouchwin(win);
	TDmove(win, 0, comtab[hilited].ct_begpos);
	TDrefresh(win);
	cp = (u_char *) comtab[curitem].ct_name;
	return(cp);
}

# else

u_char *
FTlotusinp(mu)
MENU	mu;
{
	return((u_char *)ERx(""));
}

# endif /* LOTUSINP */

VOID
FTnxtset(mu)
MENU	mu;
{
	i4		i;
	i4		j;
	struct com_tab	*comtab;

	comtab = mu->mu_coms;
	i = mu->mu_dfirst;
	if (!(comtab[i].ct_flags & CT_ALONE))
	{
		for (i++, j = mu->mu_last; i < j; i++)
		{
			if (comtab[i].ct_flags & CT_BOUNDARY)
			{
				break;
			}
		}
		if (comtab[i].ct_flags & CT_ALONE)
		{
			i--;
		}
	}
	mu->mu_dsecond = i;
}


i4
FTprvset(mu)
MENU	mu;
{
	i4		i;
	struct com_tab	*comtab;

	comtab = mu->mu_coms;
	i = mu->mu_dsecond;
	if (mu->mu_dsecond == mu->mu_last || !(comtab[i].ct_flags & CT_ALONE))
	{
		for (i--; i > 0; i--)
		{
			if (comtab[i].ct_flags & CT_BOUNDARY)
			{
				break;
			}
		}
		if (comtab[i].ct_flags & CT_ALONE)
		{
			i++;
		}
	}
	mu->mu_dfirst = i;
}


VOID
FTshiftmenu(mu, direction)
MENU	mu;
u_char	direction;
{
	if (!(mu->mu_flags & MU_LONG))
	{
		return;
	}

	if (direction == '>')
	{
		if (mu->mu_dfirst == mu->mu_dsecond)
		{
			if (mu->mu_dsecond == mu->mu_last - 1)
			{
				mu->mu_dfirst = 0;
			}
			else
			{
				mu->mu_dfirst = mu->mu_dsecond + 1;
			}
		}
		else
		{
			if (mu->mu_dsecond == mu->mu_last)
			{
				mu->mu_dfirst = 0;
			}
			else
			{
				if (mu->mu_coms[mu->mu_dsecond + 1].ct_flags & CT_ALONE)
				{
					mu->mu_dfirst = mu->mu_dsecond + 1;
				}
				else
				{
					mu->mu_dfirst = mu->mu_dsecond;
				}
			}
		}
		FTnxtset(mu);
	}
	else
	{
		if (mu->mu_dfirst == 0)
		{
			if (mu->mu_coms[mu->mu_last - 1].ct_flags & CT_ALONE)
			{
				mu->mu_dfirst = mu->mu_dsecond = mu->mu_last - 1;
				FTdispmenu(mu);
				return;
			}
			else
			{
				mu->mu_dsecond = mu->mu_last;
			}
		}
		else
		{
			if (mu->mu_dfirst == mu->mu_dsecond)
			{
			/*
				if (mu->mu_coms[mu->mu_dfirst - 1].ct_flags & CT_ALONE)
				{
					mu->mu_dsecond = mu->mu_dfirst - 1;
				}
				else
				{
					mu->mu_dsecond = mu->mu_dfirst;
				}
			*/
				mu->mu_dsecond = mu->mu_dfirst - 1;
			}
			else
			{
				if (mu->mu_coms[mu->mu_dfirst - 1].ct_flags & CT_ALONE)
				{
					mu->mu_dsecond = mu->mu_dfirst - 1;
				}
				else
				{
					mu->mu_dsecond = mu->mu_dfirst;
				}
			}
		}
		FTprvset(mu);
	}
	FTdispmenu(mu);
}


/*
VOID
FTbeginmenu(mu)
MENU	mu;
{
	if (mu->mu_setptr != 0)
	{
		FTmovemenu(mu, 0);
	}
}
*/

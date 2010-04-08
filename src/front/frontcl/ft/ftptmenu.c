/*
**  FTputmenu
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**
**  9/21/84 -
**    Stolen for FT. (dkh)
**  5/8/85 -
**    Added support for long menu.s (dkh)
**  5/31/85 -
**    Added support for backward compatibility with old style menus. (dkh)
**  7/12/85 -
**    Added support for simple pop-up style menus. (dkh)
**  7/15/85 -
**    Added support for control/function key mapping to a menu position. (dkh)
**	03/06/87 (dkh) - Added support for ADTs.
**  11/25/86 (bruceb)	Fix for bug 10890
**    Reset curmenu to NULL in FTputmenu if menu is empty.  Used by
**    (new routine) FTvismenu() to determine if current form has a
**    visible menu.
**  10-dec-86 (dkh,bruceb) Fix for bug 10983
**    Change FTgtrefmu to save contents of menu window (MUwin), clear the
**    window, and replace the contents, rather than calling FTdispmenu to
**    reconstruct the menuline--which was a problem since call occured in
**    a state that the forms system wasn't designed for.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changes for 8000 series error numbers.
**	09/03/88 (dkh) - Fixed jup/venus bug 3246.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	09/22/89 (dkh) - Porting changes integration.
**	10/01/89 (dkh) - Code cleanup.
**	10/02/89 (dkh) - Changed IIFTscSizeChg() to IIFTcsChgSize().
**	11/24/89 (dkh) - Put in support for menu fast path from wview.
**	01/10/90 (dkh) - Put in menu truncation if it is too long to display.
**	26-mar-90 (bruceb)
**		Added locator support for menus.  Register the locations of
**		the menuitems and the '<' and '>' sideways scroll indicators.
**		IIFTmldMenuLocDecode() added to determine effect of a click.
**		IIFTsmShiftMenu() added to scroll a menu left or right.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	11-march-91 (blaise)
**	    Integrated desktop changes.
**	03/24/91 (dkh) - Fixed above change from PC group since global
**			 names must begin with II.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	08/21/91 (dkh) - Fixed previous integration so things can link.
**	15-nov-91 (leighb) DeskTop Porting Change: remove PMFE ifdefs
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	10-feb-92 (rogerf) added call to FTputgui inside GUIMENU ifdef.
**	10-mar-92 (leighb) Check for MU_GUIDIVIDER character in menu item
**		name and replace with a '\0' if not using GUI menus.
**		Save name of last Menu Item Selected in IIFTlmisLstMuItemSel.
**	06/26/92 (dkh) - Added support for enabling/disabling menuitems.
**	01/26/93 (dkh) - Added ifdefs around "tools for windows" specific
**			 changes.
**	3-29-93 (fraser)
**		Removed certain ifdefs on GUIMENU.  Certain code related
**		to "GUI" menus needs to be executed on all platforms; 
**		otherwise, applications developed under Windows cannot
**		be ported to other platforms without changing them.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	12/22/93 (donc) Bug 55103
**		With the advent of activating/deactivating menuitems,
**		the handling of how long menulines failed to take into
**		account that between refreshes of the menuline, some
**		menutitems may become visible while others disappear.
**		I modified the setting of mu->mu_dsecond to account for
**		this.
**	01/18/94 (dkh) - Rolled back above change as it causes a crash
**			 when the menu line is split due to a long
**			 length.  A different change has been added
**			 to fix the original bug.
**	04/23/96 (chech02) - fixed compiler complaint for windows 3.1 port.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Removed prototype for FTshiftmenu().
**	2-Mar-2007 (kibro01) b117829
**	    Added FTmuvisible() to count current visible menu items
**	19-Mar-2007 (kibro01) b117829
**	    Corrected fix for bug 117829 to initialise comtab correctly
**	    and only to count user-defined menu items in the visible count
**	4-Jun-2007 (kibro01) b118422
**	    Keys did not work in vifred, because the menu to be counted
**	    needs to include all non-user items, not just user-defined ones.
**	4-Sep-2007 (kibro01) b117829/b118718
**	    Add in FTmukeyactive to determine finally how to process the
**	    disabled flag of a key - it can't be determined from inside
**	    mapkey.c.
**	27-Nov-2007 (kibro01) b119331
**	    Make FTmykeyactive return TRUE (the key is OK to press) if it
**	    isn't listed on the menu - ctrl-F for search works like this
**	    when within a tablefield.
**	16-Oct-2008 (kibro01) b121033
**	    When all items are invisible on a menu, the IIFT menu is null,
**	    which means that items don't have names, can't be turned off by
**	    name (obviously) and therefore the activation on the fn key itself
**	    must be valid, so return TRUE from FTmukeyactive in that case.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<menu.h>
# include	<omenu.h>
# include	<termdr.h>
# include	<mapping.h>
# include	<er.h>
# include	<erft.h>

FUNC_EXTERN	bool	FTputring();
FUNC_EXTERN	i4	TDrefresh();
GLOBALREF	bool	mu_popup;
GLOBALREF	bool	ftshmumap;

GLOBALREF MENU	IIFTcurmenu;

/* The following definition of mu_guidivider must not be ifdef-ed */
static	char	mu_guidivider[2] = {MU_GUIDIVIDER, '\0'};     

static	WINDOW	*MUwin ZERO_FILL;
static	WINDOW	*MUinwin ZERO_FILL;
GLOBALREF	WINDOW	*MUswin;
GLOBALREF	WINDOW	*MUsdwin;
GLOBALREF	i4	mutype;
GLOBALREF	i4	mu_where;       /* Either MU_TOP or MU_BOTTOM       */
#ifdef	GUIMENU
GLOBALREF	char	IIFTlmisLstMuItemSel[];
#endif

FUNC_EXTERN	WINDOW	*TDnewwin();
FUNC_EXTERN	WINDOW	*TDmakenew();
FUNC_EXTERN	char	*FTflfrsmu();
FUNC_EXTERN	VOID	IIFTcsChgSize();
FUNC_EXTERN	VOID	IITDptiPrtTraceInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	VOID	IIFTfldFrmLocDecode();
FUNC_EXTERN	i4	mu_trace();

GLOBALREF	i4	IITDAllocSize;
GLOBALREF	i4	YN;
GLOBALREF	i4	YO;
GLOBALREF	bool	IITDlsLocSupport;
GLOBALREF	FRAME	*FTfrm;
GLOBALREF       VOID    (*FTdmpmsg)();

/*
**  FTmuinit
**
**  Initialize menu windows.
**
**  Written - 1/16/85 (dkh)
*/

STATUS
FTmuinit()
{
	if (mutype == MU_RING)
	{
		return(FTinring());
	}

	if ((MUwin = TDnewwin((i4) 1, IITDAllocSize - 1,
		LINES - 1, (i4) 0)) == NULL)
	{
		return(FAIL);
	}
	MUwin->_relative = FALSE;

	if ((MUinwin = TDmakenew((i4) 1, IITDAllocSize - 1, LINES - 1,
		(i4) 0)) == NULL)
	{
		return(FAIL);
	}

	if ((MUswin = TDnewwin((i4) LINES - 1, IITDAllocSize, (i4) 0,
		(i4) 0)) == NULL)
	{
		return(FAIL);
	}

	if ((MUsdwin = TDmakenew((i4) LINES - 1, IITDAllocSize, (i4) 0,
		(i4) 0)) == NULL)
	{
		return(FAIL);
	}

# ifdef	DATAVIEW
	IIMWmiMenuInit(FTflfrsmu, menumap);
# endif	/* DATAVIEW */

	MUsdwin->_flags |= _SUBWIN;
	MUsdwin->_parent = MUswin;
	MUsdwin->_relative = FALSE;
	MUsdwin->lvcursor = FALSE;
	MUinwin->_flags |= _SUBWIN;
	MUinwin->_parent = MUwin;
	MUinwin->_relative = FALSE;
	MUinwin->lvcursor = FALSE;
	return(OK);
}



i4
FTmulimits(mu)
MENU	mu;
{
	i4	menulen;
	i4	i;
	i4	visible;
	i4	prev;
	i4	first = 0;
	i4	mucount;
	i4	namelength;
	char	*p;
	char	*label;
	COMTAB	*comtab;

	menulen = 2;
	mucount = mu->mu_last;
	comtab = mu->mu_coms;
	mu->mu_dfirst = 0;
	mu->mu_dsecond = 0;
	mu->mu_flags &= ~MU_LONG;

	for (i = 0; i < mucount; i++)
	{
		comtab[i].ct_flags &= ~(CT_BOUNDARY | CT_ALONE | CT_TRUNC);
	}

	prev = 0;
	visible = 0;
	for (i = 0; i < mucount; i++)
	{
		if (comtab[i].ct_flags & CT_DISABLED)
		{
			continue;
		}
		namelength = STlength(comtab[i].ct_name);

	    /*  The following if statment must not be ifdef-ed. */
		if (p = STindex(comtab[i].ct_name, mu_guidivider, namelength))		 
		{
			*p = EOS;
			namelength = STlength(comtab[i].ct_name);
		}						 
		if (ftshmumap)
		{
			if ((label = FTflfrsmu(comtab[i].ct_enum)) ||
				(label = menumap[visible]))
			{
				namelength += STlength(label) + 2;
			}
		}

		menulen += namelength;

		if (namelength > MAXMULEN - 2)
		{
			/*
			**  If we are not dealing with the first
			**  menuitem, then set boundary flag
			**  for previous menuitem and first
			**  display set.
			*/
			if (i - 1 >= 0)
			{
				comtab[i - 1].ct_flags |= CT_BOUNDARY;
				if (!first)
				{
					first = 1;
					mu->mu_dsecond = i - 1;
				}
			}
			if (prev == i - 1)
			{
				/*
				**  Set single item display flag
				**  if there was only one item before.
				*/
				comtab[i - 1].ct_flags |= CT_ALONE;
			}
			comtab[i].ct_flags |= (CT_BOUNDARY | CT_ALONE | CT_TRUNC);
			menulen = 0;
			prev = i;
			mu->mu_flags |= MU_LONG;
			if (!first)
			{
				first = 1;
				mu->mu_dsecond = i;
			}
		/*
			(*FTgeterr)(E_FT0038_RTMULEN, (i4)0);
			return (FALSE);
		*/
		}


		else if (menulen == MAXMULEN)
		{
			if (i + 1 < mucount)
			{
				comtab[i].ct_flags |= CT_BOUNDARY;

				if (!first)
				{
					first = 1;
					mu->mu_dsecond = i;
				}

				if (prev == (i - 1))
				{
					menulen = 0;
					comtab[i].ct_flags |= CT_ALONE;
				}
				else
				{
					menulen = 2 + namelength;
				}

				prev = i;

				mu->mu_flags |= MU_LONG;
			}
		}
		else if (menulen > MAXMULEN)
		{
			comtab[i - 1].ct_flags |= CT_BOUNDARY;

			if (prev == (i - 1))
			{
				prev = i;
				comtab[i - 1].ct_flags |= CT_ALONE;
				menulen = namelength + 2;
			}
			else
			{
				prev = i - 1;
				menulen = 4 + namelength + STlength(comtab[i - 1].ct_name);
				if (ftshmumap)
				{
					if ((label = FTflfrsmu(comtab[i - 1].ct_enum)) || (label = menumap[visible - 1]))
					{
						menulen += STlength(label) + 2;
					}
				}
				if (menulen > MAXMULEN)
				{
					prev = i;
					comtab[i].ct_flags |= (CT_ALONE | CT_BOUNDARY);
					menulen = 0;
				}
			}

			if (!first)
			{
				first = 1;
				mu->mu_dsecond = i - 1;
			}


			mu->mu_flags |= MU_LONG;
		}
		menulen += 2;
		visible++;

	}
	return(TRUE);
}


/*
**  FTputmenu
**  display the passed menu on the bottom of the screen
**
**  Written
**	(9/24/81) jrc
**  Stolen for FT
**	(9/20/84) dkh
*/
i4
FTputmenu(mu)
MENU	mu;
{
	OMENU	omu;		/* old menu structure. */
	COMTAB	*ct;
	i4	width;
	i4	termwidth;
	i4	maxwidth;
	bool	submenu = FALSE;
	char	*p;

	IIFTcsChgSize();

# ifdef DATAVIEW
	if (IIMWpmPutMenu(mu) != OK)
		return(FALSE);
# endif	/* DATAVIEW */

	if (mu == NULL)
	{
		TDerase(stdmsg);
		TDrefresh(stdmsg);
		IIFTcurmenu = NULL; /* set up for use by FTvismenu() */
		return(TRUE);
	}

	if (mutype == MU_RING)
	{
		return(FTputring(mu));	/* Do ring menu */
	}
# ifdef	GUIMENU
	else if (mutype == MU_GUI)
	{
            return (FTputgui(mu, (i2) -1, (i2) 0));  /* do Graphical User Interface menu */
	}
# endif	/* GUIMENU */
	/*
	**  The RUNTIME routines that created the menu structures
	**  have zero'ed out mu_dfirst.	 This must be true else
	**  things fall apart.
	*/

	MUwin->_cury = 0;
	MUwin->_curx = 0;


	if (mu->mu_prline != NULL && mu_popup && (mu->mu_flags & MU_SUBMENU) &&
		mu->mu_last <= MAX_SUBMUCNT)
	{
		submenu = TRUE;
		termwidth = COLS - SUBMU_SPACE;
		ct = mu->mu_coms;
		maxwidth = 0;
		while (ct->ct_name != NULL)
		{
			width = STlength(ct->ct_name);
		    /*  The following if statement must not be ifdef-ed.  */
			if (p = STindex(ct->ct_name, mu_guidivider, width))
			{
				*p = EOS;
				width = STlength(ct->ct_name);
			}						 
			if (width > termwidth)
			{
				submenu = FALSE;
				break;
			}
			if (width > maxwidth)
			{
				maxwidth = width;
			}
			ct++;
		}
	}
	if (submenu)
	{
		return(TRUE);
	}

	if (mu->mu_prline == NULL)
	{
		/*
		**  New style of menu structure. (dkh)
		*/

		mu->mu_window = MUwin;
		mu->mu_prompt = MUinwin;
		if (mu->mu_flags & (MU_NEW | MU_REFORMAT))
		{
			mu->mu_flags &= ~(MU_NEW | MU_REFORMAT);
			FTmulimits(mu);
		}
	}
	else
	{
		/*
		**  Old style of menu structure. (dkh)
		*/

		omu = (OMENU) mu;
		omu->mu_window = MUwin;
		omu->mu_prompt = MUinwin;
	}

	IIFTcurmenu = mu;

	return(FTdispmenu(mu));
}


FTdispmenu(mu)
MENU	mu;
{
	i4	i;
	i4	mulen;
	i4	start;
	i4	last;
	i4	cplen;
	i4	milen;
	i4	midisplen;
	i4	lblen;
	i4	lbdisplen;
	char	*cp;
	char	*sp;
	char	*lb;
	char	*p;
	WINDOW	*win;
	WINDOW	*inwin;
	COMTAB	*comtab;
	char	buf[MAX_TERM_SIZE + 1];
	bool	old_menu = FALSE;
	char	tbuf[100];
	COMTAB	*ctab;
	i4	visible;


	if (mu->mu_prline != NULL)
	{
		/*
		**  We have been passed an old style menu structure.
		**  Deal with it. (dkh)
		*/

		cp = mu->mu_prline;
		old_menu = TRUE;
	}

        /*
        ** brett, New pull down menu windex command string
        ** Use the 3 windex interfaces to send all the menu items in
        ** pull down form to the window manager.  This avoids using
        ** a buffer locally.
        */
        comtab = mu->mu_coms;
        IITDnwpdmenu();
	visible = 0;
        for (i=0; i<mu->mu_last; i++)
	{
		if (comtab[i].ct_flags & CT_DISABLED)
		{
			continue;
		}
	    /*  The following if statement must not be ifed-ed.  */
		if (p = STindex(mu->mu_coms[i].ct_name, 
				    mu_guidivider, 
					STlength(mu->mu_coms[i].ct_name)))
		{
			*p = EOS;
		}						 
                if ((sp = FTflfrsmu(comtab[i].ct_enum)) || (sp = menumap[i]))
		{
                        IITDpdmenuitem(comtab[i].ct_name, sp);
		}
                else
		{
                        /* The NULL is redundent but readable */
                        IITDpdmenuitem(comtab[i].ct_name, NULL);
		}

		/*
		**  Determine the starting visible menu item
		**  so we can match up the function key mappings
		**  correctly when the menu line needs to be
		**  scrolled.
		*/
		if (i < mu->mu_dfirst)
		{
			visible++;
		}
        }
        IITDedpdmenu();

	if (!old_menu)
	{
		/*
		**  Place check for submenu here
		**  if pop-up submenus are desired.
		*/

		comtab = mu->mu_coms;
		cp = buf;

		start = mu->mu_dfirst;
		last = mu->mu_dsecond;

		IITDptiPrtTraceInfo(ERx(""));	/* Space out the trace info. */

		if (start != 0)
		{
			*cp++ = '<';
			mu->mu_back = 0;
			IITDptiPrtTraceInfo(ERx("Menu back scroll at 0"));
		}
		else
		{
			mu->mu_back = -1;
		}
		*cp++ = ' ';
		*cp++ = ' ';

		if (mu->mu_flags & MU_LONG)
		{
			/*
			**  If we are displaying only one menu item.
			*/

			if (last != mu->mu_last)
			{
				last++;
			}
		}
		else
		{
			last = mu->mu_last;
		}

		for (i = start; i < last; i++)
		{
			ctab = &(comtab[i]);

			if (ctab->ct_flags & CT_DISABLED)
			{
				/*
				**  Set position info to be something
				**  that is not possible so that it
				**  can't be used for cursor selection
				**  of menuitem via built-in windowview.
				*/
				ctab->ct_begpos = -1;
				ctab->ct_endpos = -1;
				continue;
			}
			sp = ctab->ct_name;

			if (ctab->ct_flags & CT_TRUNC)
			{
				midisplen = (MAXMULEN - 2) /2;
				lbdisplen = MAXMULEN - 2 - midisplen - 2;

				if (ftshmumap)
				{
					if ((lb=FTflfrsmu(ctab->ct_enum)) ||
						(lb = menumap[visible]))
					{
						lblen = STlength(lb);
						milen = STlength(sp);

						/*
						**  If label length is shorter
						**  than allocated, then
						**  give some back to menuitem.
						*/
						if (lblen < lbdisplen)
						{
							midisplen += lbdisplen -
								lblen;
							lbdisplen = lblen;
						}
						else
						{
						    /*
						    **  See if we can
						    **  take some away
						    **  from menuitem
						    **  and give to label.
						    */
						    if (milen < midisplen)
						    {
							lbdisplen += midisplen -
							    milen;
							midisplen = milen;
						    }
						}
					}
					else
					{
						midisplen = MAXMULEN - 2;
					}
				}
				else
				{
					midisplen = MAXMULEN - 2;
				}

				ctab->ct_begpos = cp -  buf;
				for (cplen = 0; cplen < midisplen && *sp;
					cplen++)
				{
					*cp++ = *sp++;
				}
				if (ftshmumap && lb)
				{
					*cp++ = '(';
					while (*lb)
					{
						*cp++ = *lb++;
					}
					*cp++ = ')';
				}
				ctab->ct_endpos = cp -  buf - 1;
			}
			else
			{
				ctab->ct_begpos = cp -  buf;
				while (*sp)
				{
					*cp++ = *sp++;
				}
				if (ftshmumap)
				{
					if ((sp=FTflfrsmu(ctab->ct_enum)) ||
						(sp = menumap[visible]))
					{
						*cp++ = '(';
						while (*sp)
						{
							*cp++ = *sp++;
						}
						*cp++ = ')';
					}
				}
				ctab->ct_endpos = cp -  buf - 1;
			}
			*cp++ = ' ';
			*cp++ = ' ';

			if (IITDlsLocSupport)
			{
			    STprintf(tbuf,
				ERx("Menuitem '%s' (%d), coords are %d - %d"),
				ctab->ct_name, ctab->ct_enum,
				ctab->ct_begpos, ctab->ct_endpos);
			    IITDptiPrtTraceInfo(tbuf);
			}

			visible++;
		}
		if (last < mu->mu_last)
		{
			mu->mu_forward = cp - buf;
			STprintf(tbuf, ERx("Menu forward scroll at %d"),
			    mu->mu_forward);
			IITDptiPrtTraceInfo(tbuf);
			*cp++ = '>';
			*cp++ = ' ';
		}
		else
		{
			mu->mu_forward = -1;
		}
		*cp = '\0';
		cp = buf;
	}

	mulen = STlength(cp);

	win = mu->mu_window;
	inwin = mu->mu_prompt;
	if (TDsubwin(win, (i4)1, (COLS-1) - mulen, LINES - 1,
		mulen, inwin) == NULL)
	{
		return(FALSE);
	}
	TDerase(win);
	TDaddstr(win, cp);
        /*
        ** brett, Menu line windex command string
        ** Note: The origin for the menu line is (0,0)
        */
        IITDmnline(0, LINES-1, cp);

	/*
	**  Add menu list to standard message
	**	10-20-82  (jen)
	*/

	TDerase(stdmsg);
	TDaddstr(stdmsg, cp);
	TDtouchwin(stdmsg);

	TDerase(inwin);
	win->lvcursor = TRUE;
	TDtouchwin(win);
	TDrefresh(win);
	win->lvcursor = FALSE;
	return(TRUE);
}



FTvisclumu()
{
    if ((mutype == MU_RING) || (mutype == MU_GUI))
    {
		return;
	}

	TDmove(MUwin, 0, 0);
	TDrefresh(MUwin);
}


/*
**  FTgtrefmu -- called by Vigraph to refresh the menu line at a
**		 point when the forms system thinks the line is
**		 visible, but in reality a screen clear has occurred
**		 in the graphics code.
*/
FTgtrefmu()
{
	char	buf[MAX_TERM_SIZE + 1];

	TDwin_str(MUwin, buf, (bool) TRUE);
	TDclear(MUwin);
	TDaddstr(MUwin, buf);
	TDrefresh(MUwin);
}


/*
** FTvismenu -- does the current form have a visible menu
**
** called by FKismapped().
*/
bool
FTvismenu()
{
	if (IIFTcurmenu == NULL)
		return(FALSE);
	else
		return(TRUE);
}


VOID
FTdmpmu()
{
	if (IIFTcurmenu != NULL)
	{
		mu_trace(IIFTcurmenu);
	}
}


/*{
** Name:	IIFTmscMenuSizeChg - Change size of menu window.
**
** Description:
**	Changes the size of the windows that the menu is displayed in.
**
** Inputs:
**	iswide		Change to logical wide size if TRUE.  Change to
**			logical narrow size if FALSE.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/04/89 (dkh) - Initial version.
*/
VOID
IIFTmscMenuSizeChg(iswide)
i4	iswide;
{
	i4	newsize;

	if (iswide)
	{
		newsize = YO;
	}
	else
	{
		newsize = YN;
	}

	/*
	**  Only set the main window for the menu line.
	**  No need to set submenu used for input.  They
	**  will be set properly when menu is displayed.
	MUwin->_maxx = MUinwin->_maxx = newsize - 1;
	MUswin->_maxx = MUsdwin->_maxx =  newsize;
	*/
	MUwin->_maxx = newsize - 1;
	MUswin->_maxx = newsize;
}


i4
IIFTmiMenuInx()
{
	return(IIFTcurmenu->mu_dfirst);
}


/*{
** Name:	IIFTmldMenuLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Needs to recognize menuitems, '<', '>' and ':'.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopMUITEM, fdopSCRRT,
**			fdopSCRLT or fdopRET if legal.
**	    .mf_num	Menuitem number if location is a menuitem.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	27-mar-90 (bruceb)	Written.
*/
VOID
IIFTmldMenuLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    bool	found = FALSE;
    i4		row;
    i4		col;
    i4		i;
    i4		j;
    i4		last;
    COMTAB	*ctab;
    char	buf[80];

    evcb->event = fdNOCODE;

    if (IIFTcurmenu != NULL)
    {
	row = key->ks_p1;
	col = key->ks_p2;

	if (row != ((LINES - 1) * (mu_where == MU_BOTTOM)))
	{
	    /* User has clicked off the menu line.  Legal? */
	    if ((IIFTcurmenu->mu_flags & MU_FRSFRM) && FTfrm
		&& (FTfrm->frmflags & fdTRHSPOT))
	    {
		/* Possible click on a hotspot. */
		IIFTfldFrmLocDecode(key, evcb);
		if ((evcb->event != fdNOCODE) && (evcb->event != fdopHSPOT))
		{
		    /* User 'selected' a field, tblfld cell or scroll point. */
		    evcb->event = fdNOCODE;
		    IITDposPrtObjSelected(ERx("NOT on the menu"));
		}
	    }
	}
	else
	{
	    /* User has clicked on the menu line. */
	    last = IIFTcurmenu->mu_dsecond;
	    if (IIFTcurmenu->mu_flags & MU_LONG)
	    {
		if (last != IIFTcurmenu->mu_last)
		{
		    last++;
		}
	    }
	    else
	    {
		last = IIFTcurmenu->mu_last;
	    }

	    ctab = IIFTcurmenu->mu_coms;
	    /* Scan visible menuitems for a match. */
	    for (i = IIFTcurmenu->mu_dfirst; i < last; i++)
	    {
		if ((ctab[i].ct_begpos <= col) && (col <= ctab[i].ct_endpos))
		{
		    evcb->event = fdopMUITEM;
		    evcb->mf_num = i;
		    found = TRUE;

		    /*
		    **  In order to handle disabled menuitems, we need
		    **  to change (reduce) the value in evcb->mf_num so
		    **  that the decoding in runtime is the same no matter
		    **  how a menuitem was selected.
		    */
		    for (j = 0; j < i; j++)
		    {
			if (ctab[j].ct_flags & CT_DISABLED)
			{
				evcb->mf_num--;
			}
		    }

		    STprintf(buf, ERx("menuitem (%s)"), ctab[i].ct_name);
		    IITDposPrtObjSelected(buf);
#ifdef	GUIMENU
		    STcopy(ctab[i].ct_name, IIFTlmisLstMuItemSel);
#endif
		    break;
		}
	    }

	    if (!found)
	    {
		if (col == IIFTcurmenu->mu_back)
		{
		    evcb->event = fdopSCRRT;
		    IITDposPrtObjSelected(ERx("<"));
		}
		else if (col == IIFTcurmenu->mu_forward)
		{
		    evcb->event = fdopSCRLT;
		    IITDposPrtObjSelected(ERx(">"));
		}
		else if (col == IIFTcurmenu->mu_colon)
		{
		    evcb->event = fdopRET;
		    IITDposPrtObjSelected(ERx(":"));
		}
	    }
	}
    }
}


/*{
** Name:	IIFTsmShiftMenu	- Shift menu left or right.
**
** Description:
**	Calls FTshiftmenu() to shift IIFTcurmenu left or right.  Assumes that
**	the menu can shift since this occurs only via locator click.
**
** Inputs:
**	direction	fdopSCRLT or fdopSCRRT.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	29-mar-90 (bruceb)	Written.
*/
VOID
IIFTsmShiftMenu(direction)
i4	direction;
{
    /*
    ** If IIFTcurmenu isn't set, shouldn't have been called,but recover by doing
    ** nothing here.  Calling routine will just return up with an fdopMENU
    ** event.
    */
    if (IIFTcurmenu)
    {
	if (direction == fdopSCRRT)
	{
	    if (mutype == MU_RING)
	    {
		_VOID_ FTringShift(direction);
	    }
# ifdef	GUIMENU
	    else if (mutype == MU_GUI)
	    {
		_VOID_ FTguiShift(direction);
	    }
# endif
	    else
	    {
		_VOID_ FTshiftmenu(IIFTcurmenu, '<');
	    }
	    (*FTdmpmsg)(ERx("Shifting menu items in from the RIGHT\n"));
	}
	else
	{
	    if (mutype == MU_RING)
	    {
		_VOID_ FTringShift(direction);
	    }
# ifdef	GUIMENU
	    else if (mutype == MU_GUI)
	    {
		_VOID_ FTguiShift(direction);
	    }
# endif
	    else
	    {
		_VOID_ FTshiftmenu(IIFTcurmenu, '>');
	    }
	    (*FTdmpmsg)(ERx("Shifting menu items in from the LEFT\n"));
	}
	_VOID_ mu_trace(IIFTcurmenu);
    }
}

i4
FTmuvisible(count_all)
bool	count_all;
{
	i4	i;
	i4	count = 0;
	COMTAB	*comtab;
	MENU	mu;

	mu = IIFTcurmenu;
	if (!mu)
		return 0;

	comtab = mu->mu_coms;

	for (i = 0; i < mu->mu_last; i++)
	{
		/* Only count User-defined menu items for disabled-checking */
		if (comtab[i].ct_flags & CT_DISABLED ||
		    (!(comtab[i].ct_flags & CT_ACTUF) && !count_all))
			continue;
		count++;
	}
	return count;
}


bool
FTmukeyactive(intr_val)
i4	intr_val;
{
	COMTAB  *comtab;
	MENU    mu;
	i4	flags = (CT_DISABLED | CT_ACTUF);
	i4	i;

	mu = IIFTcurmenu;
	/* no current menu means all items are invisible, in which case
	** the function keys cannot be switched off by name in the menu,
	** so must be valid (kibro01) b121033
	*/
	if (!mu)
		return (TRUE);

	comtab = mu->mu_coms;

	for (i=0; i<mu->mu_last; i++)
	{
		if (comtab[i].ct_enum == intr_val)
		{
			if ((comtab[i].ct_flags & flags) == flags)
				return (FALSE);
			return (TRUE);
		}
	}
	return (TRUE);
}

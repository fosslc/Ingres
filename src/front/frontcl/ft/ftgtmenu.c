/*
**  FTgetmenu
**
**  Copyright (c) 2004 Ingres Corporation
**
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
**    Splitted up file and place routine that handle menu shifting
**    and lotus style input into ftmushift.c. (dkh)
**  8/6/85 -
**    Added support for validation when a menuitem is selected. (dkh)
**  8/13/85 -
**    Added calls to KFprintmark for Keystroke File Editor (dpr)
**  8/21/85 -
**	Added call to KFcountout for Keystroke File Editor (dpr)
**  1-oct-1985 (joe)
**	Added the special debug comment.
**  10/18/85 -
**	Added more diagnostics for running tests. (dkh)
**  8/13/85 -
**    Added calls to KFprintmark for Keystroke File Editor (dpr)
**  8/21/85 -
**	Added call to KFcountout for Keystroke File Editor (dpr)
**  2/27/87 (garyb) -- ifdef'd Keystroke file editor related calls.
**  12/24/86 (dkh) - Added support for new activations.
**  10/20/86 (KY)  -- Changed CH.h to CM.h.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	05/22/87 (dkh) - Deleted duplicate calls to mu_trace.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/25/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	07/23/88 (dkh) - Added exec sql immediate hook.
**	09/23/88 (dkh) - Fixed jup bug 1852.
**	10-nov-88 (bruceb)
**		If FTuserinp() returns because of timeout, than return
**		fdopTMOUT, distinguishable from all legal menu-ids.
**	09/22/89 (dkh) - Porting changes integration.
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.
**	02/20/90 (dkh) - Fixed bug 9771.
**	02/23/90 (dkh) - Fixed bug 9961.
**	27-mar-90 (bruceb)
**		Added locator support for menus.  Addition here is that the
**		coords of the ':' are set/cleared before/after FTuserinp().
**		VIFRED menus no longer need to reset on exit.  Treat them
**		like every other menu.  -- Zapped unused popup menu code.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	31-may-90 (rog)
**		Included <me.h> so MEfill() is defined correctly.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	15-nov-91 (leighb) DeskTop Porting Change: remove PMFE ifdefs
**	17-dec-91 (leighb) DeskTop Porting Change:
**		change FTgetring() to IIFTgetring() to make dkh happy.
**	06/26/92 (dkh) - Added support for enabling/disabling menuitems.
**	10-feb-92 (rogerf) - Added call to IIFTgetgui if GUIMENU is defined.
**      24-sep-92 (fraser) - Changed FTgetmenu to fix RingMenu bug (Bug #46718)
**      23-apr-96 (chech02) - fixed compiler complaint for windows 3.1 port.
**	06-aug-97 (dacri01) - When an frskey is pressed, check to see if it is 
**                             disabled. 
**  15-mar-2000 (rodjo04)  Bug 100900
**                   Reworked above fix so that we now search the index for
**                   the right key position.
**	28-Mar-2005 (lakvi01)
**	     Updated function prototypes.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


# include	<compat.h>
# include	<cv.h>
# include	<pc.h>
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
# include	<frsctblk.h>
# include	<cm.h>
# include	<si.h>
# include	<st.h>
# include	<te.h>
# include	<me.h>
# include	<ug.h>
# include	<er.h>
# include	<erft.h>
# include	<uigdata.h>

GLOBALREF	bool	fdrecover;
GLOBALREF	i4	mu_where;
GLOBALREF	bool	mu_statline;
GLOBALREF	bool	ftvalanymenu;
GLOBALREF	bool	mu_popup;
GLOBALREF	bool	Rcv_disabled;
GLOBALREF	char	*IIFTmpbMenuPrintBuf;
GLOBALREF	i4	mutype;

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	u_char	*FTuserinp();
FUNC_EXTERN	u_char	*FTsubmenuinp();
FUNC_EXTERN	u_char	*FTlotusinp();
FUNC_EXTERN	VOID	IIFTofOnlyFrskeys();
FUNC_EXTERN	VOID	IITDptiPrtTraceInfo();


/*
** These three defines were added to handle two menuitems
** with the same prefix (e.g. you have "msg" and "msgc").
** Fix for BUG 831 (dkh)
*/

# define	NO	1
# define	YES	2
# define	EXACT	3


i4		prefix();


/*
** These are for the new special comment $_debug
*/
static bool	FTmudbg = FALSE;		/* Whether debugging is on */
static bool	IIFTmagic = FALSE;		/* Magic hook for tracing */
static i4	(*FTdbgfunc)() = NULL;		/* The function to call */

STATUS
FTmumap(mu, intrval, evcb)
MENU	mu;
i4	*intrval;
FRS_EVCB *evcb;
{
	WINDOW	*win;
	i4	omf_num;
	COMTAB	*ct;
	long	munum;
	i4	i;
	i4	visible;

	win = mu->mu_window;
	if (evcb->mf_num >= mu->mu_last)
	{
		/*
		**  This assumes that FTboxerr has implemented
		**  save unders and will put back what is overwrote.
		*/
		munum = evcb->mf_num + 1;
		IIUGerr(E_FT0008_Menuitem___d__does_no, UG_ERR_ERROR,1, &munum);
		TDtouchwin(win);
		TDrefresh(win);
		return(FAIL);
	}

	omf_num = evcb->mf_num;

	munum = mu->mu_last;
	visible = 0;
	ct = mu->mu_coms;
	for (i = 0; i < munum; i++, ct++)
	{
		if (ct->ct_flags & CT_DISABLED)
		{
			continue;
		}
		if (visible == omf_num)
		{
			break;
		}
		visible++;
	}

	ct = &(mu->mu_coms[i]);
	if (ct->ct_func != NULL)
	{
		(*(ct->ct_func))();
	}
	*intrval = ct->ct_enum;
	evcb->mf_num = omf_num;
	return(OK);
}

/*
** History:
**	 28-aug-1990 (Joe)
**	     Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
**      24-sep-92 (fraser)  Bug #46718
**          Don't call IIFTgetring if mu == NULL.
**          Note: At Leigh's request I similarly qualified the call
**          to IIFTgetgui; am not sure, however, whether this is the
**          correct fix for GUI menus.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
i4
FTgetmenu(mu, evcb)
MENU	mu;
FRS_EVCB *evcb;
{
	u_char		buf[MAX_TERM_SIZE + 1];
	register u_char *cp = buf;
	register struct com_tab *ct;
	char			*found = NULL;
	OMENU			omu;
	struct ocom_tab		*oct;
	FRS_EVCB		levcb;
	WINDOW			*inwin;
	i4			status;
	u_char			*savecp;
	char			*menustrs[MAX_MENUITEMS + 1];
	char			**str = menustrs;
	i4			flags;
	i4			mucounter = 0;
	i4			muval = 0;
	i4			ct_enum = 0;
	i4			j;
	bool			exact_match;
	bool			duplicate;
	bool			old_menu = FALSE;
	char			tbuf[20];
	long	     munum;
        int          i;

	if (evcb == NULL)
	{
		MEfill((u_i2) sizeof(levcb), (u_char) '\0', (PTR) &levcb);
		evcb = &levcb;
	}

        if (mutype == MU_RING && mu != NULL)
	{
		return(IIFTgetring(mu, evcb));
	}
# ifdef GUIMENU
        else if (mutype == MU_GUI && mu != NULL)
        {
                return(IIFTgetgui(mu, evcb));
        }
# endif /* GUIMENU */
	if ((FTgtflg & FTGTEDIT) != 0)
	{
		if (iigtsupdfunc)
		{
			(*iigtsupdfunc)(FALSE);
		}
		if (iigtmreffunc && (*iigtmreffunc)())
			FTgtrefmu();
	}

	evcb->event = 0;

	/*
	**  If there are no menuitems, then we
	**  can only accept frskey input.
	*/
	if (mu == NULL)
	{
		TDerase(stdmsg);
		IIFTofOnlyFrskeys(stdmsg, evcb);
		if (evcb->event == fdopTMOUT)
		{
			return(fdopTMOUT);
		}
		else if (evcb->event == fdopFRSK)
		{
			/*
			**  A frskey was pressed.
			*/
			ct_enum = evcb->intr_val;
			(*FTdmpmsg)(ERx("A FRSKEY WAS PRESSED WITH VALUE %d\n"), ct_enum);
			return(ct_enum);
		}
		else
		{
			/*
			**  User must have hit return, just
			**  return up.
			*/
			return(fdNOCODE);
		}
	}
	mu_trace(mu);
	if (mu->mu_prline != NULL)
	{
		old_menu = TRUE;
		omu = (OMENU) mu;
		inwin = omu->mu_prompt;
		flags = omu->mu_flags;
		oct = omu->mu_coms;
		while (oct->ct_name != NULL)
		{
			*str++ = oct->ct_name;
			oct++;
		}
	}
	else
	{
		inwin = mu->mu_prompt;
		flags = mu->mu_flags;
		ct = mu->mu_coms;
		while (ct->ct_name != NULL)
		{
			if (ct->ct_flags & CT_DISABLED)
			{
				*str++ =  ERx("");
			}
			else
			{
				*str++ = ct->ct_name;
			}
			ct++;
		}
	}
	*str = NULL;

	exact_match = FALSE;
	for (;;)
	{
		str = menustrs;
		if (!KY)
		{
			Rcv_disabled = TRUE;
		}
		if (!old_menu)
		{
		    mu->mu_colon = inwin->_begx;
		    STprintf(tbuf, ERx("Menu colon at %d"),
			mu->mu_colon);
		    IITDptiPrtTraceInfo(tbuf);
		    cp = FTuserinp(inwin, ERx(": "), FT_MENUIN, TRUE, evcb);
		    mu->mu_colon = -1;
		}
		else
		{
		    cp = FTuserinp(inwin, ERx(": "), FT_MENUIN, TRUE, evcb);
		}

# ifdef DATAVIEW
		if (IIMWimIsMws() && (cp == NULL))
			return(fdNOCODE);
# endif	/* DATAVIEW */

		if (evcb->event == fdopTMOUT)
		{
			return(fdopTMOUT);
		}
		else if (evcb->event == fdopMUITEM)
		{
			if (FTmumap(mu, &ct_enum, evcb) == OK)
			{
				/*
				**  Fix for BUG 8410. (dkh)
				*/
				if (!KY)
				{
					Rcv_disabled = FALSE;
				}
				return(ct_enum);
			}
			else
			{
				continue;
			}
		}
		else if (evcb->event == fdopFRSK)
		{
			/*
			**  A frskey was pressed.
			*/
			ct_enum = evcb->intr_val;
			(*FTdmpmsg)(ERx("A FRSKEY WAS PRESSED WITH VALUE %d\n"), ct_enum);
			/* 
                         * If the key is disabled, ignore it.
                         */
            munum = mu->mu_last;
            ct = mu->mu_coms;
            for (i = 0; i < munum; i++)
            {
                if (ct[i].ct_enum == ct_enum)
                {
                    if (ct[i].ct_flags & CT_DISABLED)
                        {
                             FTbell();
                             return(fdNOCODE);
                        }
                    break;
                }
            }
            return(ct_enum);
		}
		if (*cp == '<' || *cp == '>')
		{
			if (!old_menu)
			{
				FTshiftmenu(mu, *cp);
				if (*cp == '>')
				{
					(*FTdmpmsg)(ERx("Shifting menu items in from the RIGHT\n"));
				}
				else
				{
					(*FTdmpmsg)(ERx("Shifting menu items in from the LEFT\n"));
				}
				mu_trace(mu);
			}
			continue;
		}
		savecp = cp;
		if (!KY)
		{
			Rcv_disabled = FALSE;
		}
		while (CMspace(cp))
			CMnext(cp);
		STcopy((char *) cp, (char *) buf);
		if (buf[0] == '\0')
		{
			mu_outmenu(savecp);
			if (flags & NULLRET)
			{
				return(fdNOCODE);
			}
			/*
			**  This should really not occur.
			*/
			uerror(mu, S_FT0009_Null_string_not_legal,  NULL);
			continue;
		}

		/*
		**  Check for "HIDDEN" comment option.
		**  First check to make sure -I flag is
		**  set.  Fix for BUG 5349. (dkh)
		**
		** If ftmudbg set then allow it also (joe).
		*/

		if (STequal((char *) buf, ERx("\\xyzzyx")))
		{
			IIFTmagic = TRUE;
		}
		if ((fdrecover || IIUIfedata()->testing || FTmudbg || IIFTmagic)
		   && !old_menu && buf[0] == '\\')
		{
			mu_outmenu(buf);
			mu_comment(mu, buf);
			continue;
		}

		duplicate = FALSE;

		while (*str != NULL)
		{
			status = prefix(buf, *str);

			/* This next if statement was added to
			** handle an exact match for a menuitem
			** so that "msg" and "msgc" menuitems
			** are handled properly. BUG 831 (dkh)
			*/

			if (status == EXACT)
			{
				exact_match = TRUE;
				found = *str;
				muval = mucounter;
				break;
			}
			else if (status == YES)
			{
				if (found != NULL)
				{
					/*
					**  Fix for BUG 7548. (dkh)
					*/
					mu_outmenu(savecp);
					duplicate = TRUE;
					uerror(mu, E_FT003F_is_ambiguous, buf);
					break;
				}
				else
				{
					found = *str;
					muval = mucounter;
				}
			}
			mucounter++;
			str++;
		}

		/*
		**  Write entire menu item to recovery file.
		*/

		/*
		**  Fix for BUG 7548. (dkh)
		*/
		if (found == NULL)
		{
			mu_outmenu(savecp);
		}
		else
		{
			if (!duplicate)
			{
				mu_outmenu(found);
			}
		}

		/* This if statement modified to handle exact match
		** situations.	BUG 831 (dkh)
		*/

		if (found != NULL && (*str == NULL || exact_match))
		{
			/*
			**  If we have a menu, then move cursor
			**  to beginning of prompt menu to
			**  indicate a menu was accepted.
			**  Useful on slow systems.
			*/

			if (!old_menu)
			{
			    TDmove(mu->mu_prompt, (i4) 0, (i4) 0);
			    TDrefresh(mu->mu_prompt);
			}

			if (old_menu)
			{
				if (omu->mu_coms[muval].ct_func != NULL)
				{
					(*(omu->mu_coms[muval].ct_func))();
				}
			}
			else
			{
				if (mu->mu_coms[muval].ct_func != NULL)
				{
					(*(mu->mu_coms[muval].ct_func))();
				}
			}
			if (old_menu)
			{
				ct_enum = omu->mu_coms[muval].ct_enum;
			}
			else
			{
				ct_enum = mu->mu_coms[muval].ct_enum;
			}
			evcb->event = fdopMUITEM;
			evcb->intr_val = ct_enum;
			evcb->mf_num = muval;

			/*
			**  In order to handle disabled menuitems, we need
			**  to change (reduce) the value in evcb->mf_num so
			**  that the decoding in runtime is the same no matter
			**  how a menuitem was selected.
			*/
			if (!old_menu)
			{
				ct = mu->mu_coms;
				for (j = 0; j < muval; j++)
				{
					if (ct[j].ct_flags & CT_DISABLED)
					{
						evcb->mf_num--;
					}
				}
			}

			return(ct_enum);
		}

		/*
		** if gets here then not correct prefix
		** print error and try again
		*/

		if (found == NULL)
		{
			uerror(mu, E_FT0040_is_not_prefix, buf);
		}
		cp = buf;
		found = NULL;
		mucounter = 0;
		muval = 0;
	}
}

/*
**  prefix(str1, str2)
**
**  see if first argument is a prefix of the second
**
**  Written
**	(9/25/81) jrc
*/

i4
prefix(str1, str2)
char	*str1;
char	*str2;
{
	register char	*s1 = str1,
			*s2 = str2;

	for(; *s1 && *s2; CMnext(s1), CMnext(s2))
		if (CMcmpnocase(s1, s2))
		{
			return(NO); /* Not a prefix */
		}
	if (!*s2 && *s1)
	{
		return(NO); /* Not a prefix */
	}

	/* This statement checks for an exact match
	** Fix for BUG 831 (dkh)
	*/

	if (*s1 == '\0' && *s2 == '\0')
	{
		return(EXACT);
	}
	return(YES); /* Is a prefix */
}


/*
**  uerror
**
**  This routine assumes the error message is in "s1" and
**  the one argument is in "s2".  Also, s1 and s2 must
**  be less than 200 characters in total length.
*/

VOID
uerror(mu, s1, s2)
MENU		mu;
ER_MSGID	s1;
char		*s2;
{
	WINDOW		*win;
	OMENU		omu;

        /*
	** brett, windex command string
	** Disable menu selection in window interface.
	*/
        IITDdsmenus();

	/*
	**  This assumes that menus use only 1 line and are less
	**  than 200 in length.
	*/

	if (mu->mu_prline != NULL)
	{
		omu = (OMENU) mu;
		win = omu->mu_window;
	}
	else
	{
		win = mu->mu_window;
	}

	/*
	**  This assumes that FTboxerr will put back whatever
	**  it overwrote.
	*/
	IIUGerr(s1, UG_ERR_ERROR, 1, s2);

	TDtouchwin(win);
	TDrefresh(win);

        /*
	** brett, windex command string
	** Enable menu selection in window interface.
	*/
        IITDenmenus();
}

/*
**  mu_comment:
**	Prompt user for a comment so it can be added to key stroke file.
*/

STATUS
mu_comment(mu, buf)
MENU	mu;
char	*buf;
{
	i4	secs;
	u_char	sleep[256];
	u_char	comment[256];

	comment[0] = '\0';
	TDrcvwrite(LEFT_COMMENT);
	TDrcvwrite('\n');
	TDrcvwrite(RIGHT_COMMENT);
	Rcv_disabled = TRUE;
	FTprmthdlr(ERx("comment: "), (u_char *) comment, (i4) TRUE, NULL);
	Rcv_disabled = FALSE;
	(*FTdmpmsg)(ERx("COMMENT: %s"), comment);
	mu_outmenu(comment);
	TDrcvwrite(LEFT_COMMENT);
	TDrcvwrite('\n');
	TDrcvwrite(RIGHT_COMMENT);
	if (FTmudbg && STcompare((char *) comment, ERx("$_debug")) == 0)
	{
		if (FTdbgfunc != NULL)
			(*FTdbgfunc)();
	}
	else if (STcompare((char *) comment, ERx("$_sleep")) == 0)
	{
		sleep[0] = '\0';
		FTprmthdlr(ERx("Seconds to sleep? "), (u_char *) sleep,
			(i4) TRUE, NULL);
		if (CVan((char *) sleep, &secs) == OK)
		{
			secs *= 1000;
			PCsleep(secs);
		}
	}
	else if (STcompare((char *) comment, ERx("$_sql")) == 0)
	{
		sleep[0] = EOS;
		FTprmthdlr(ERx("SQL Statement? "), (u_char *) sleep,
			TRUE, NULL);
		(*FTsqlexec)(sleep);
	}
	TDtouchwin(mu->mu_window);
	TDrefresh(mu->mu_window);
	return(OK);
}

/*
**  mu_trace:
**	Write out trace information to the recovery file.
*/

i4
mu_trace(mu)
MENU	mu;
{
	char	buf[MAX_TERM_SIZE + 1];

	TDwin_str(mu->mu_window, buf, TRUE);
	STcopy(buf, IIFTmpbMenuPrintBuf);
	TDrcvwrite(LEFT_COMMENT);
	TDrcvwrite('\n');
	TDrcvstr(buf);
	TDrcvwrite('\n');
	TDrcvwrite(RIGHT_COMMENT);
	return(OK);
}

/*
**  mu_outmenu:
**	Output menu item.
**
**	Added call to KFcountout to count these characters going into Keystroke
**	File. This is the full menu item name rather than just the first letter
**	of the menu item, which is what is often typed in. Done to synch up
**	the marks in the Keystroke file with the number of actual keystrokes
**	entered. See KFE.c for more details. 8/21/85, dpr
*/

VOID
mu_outmenu(buffer)
char	*buffer;
{

	(*FTdmpmsg)(ERx("MENU RESPONSE: %s"), buffer );

	if (KY)
	{
		TDrcvwrite(LEFT_COMMENT);
		TDrcvwrite('\n');
		TDrcvwrite('\n');
		TDrcvstr(ERx("MENU \""));
		TDrcvstr(buffer);
		TDrcvstr(ERx("\" just selected "));
		TDrcvwrite('\n');
		TDrcvwrite('\n');
		TDrcvwrite(RIGHT_COMMENT);
	}
	else
	{
		TDrcvstr(buffer);
		TDrcvwrite('\r');
	}
}

/*{
** Name:	FTdbginit	-	Set the debugging function.
**
** Description:
**	This function sets the debugging function to be called if the
**	special comment $_debug is entered in a comment.  It also
**	sets the flag FTmudbg so that getmenu will not ignore the
**	comment character.
**	If the function is NULL then FTmudbg is turned off.
**
** Inputs:
**		func		- pointer to the function to call.
**
** History:
**	1-oct-1985 (joe)
**		Written
*/
VOID
FTdbginit(func)
i4	(*func)();
{
	FTdbgfunc = func;
	FTmudbg = TRUE;
	if (func == NULL)
		FTmudbg = FALSE;
}

/*
**  mapkey.c
**
**  Decode a return value returned from calling any of
**  the get character routines.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	Created - 07/25/85 (dkh)
**	86/11/21  joe - Lowercased FT*.h
**	86/11/18  dave - Initial60 edit of files
**	86/01/10  dave - Fix for BUG 7551. (dkh)
**	85/11/12  dave - Fixed FTfrskreset() to not reset ftwfrsk. (dkh)
**	85/11/05  dave - Added FTwasfrsk() for fix to display
**			 submenu problem. (dkh)
**	85/10/17  dave - Changed to move cursor to beginning of
**			 menu line to provide a visual clue that a
**			 menu item was selected. (dkh)
**	85/09/18  john - Changed i4=>nat, CVla=>CVna, CVal=>CVan,
**			 unsigned char=>u_char
**	85/08/23  dave - Initial version in 4.0 rplus path. (dkh)
**	85/08/09  dave - Initial revision
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	85/10/17  18:15:13  dave
**		Changed to move cursor to beginning of menu line to provide a
**		visual clue that a menu item was selected. (dkh)
**	85/11/05  15:53:12  dave
**		Added FTwasfrsk() for fix to display submenu problem. (dkh)
**	85/11/12  19:07:38  dave
**		Fixed FTfrskreset() to not reset ftwfrsk. (dkh)
**	86/01/10  19:28:57  dave
**		Fix for BUG 7551. (dkh)
**	08/29/86 (a.dea) -- Kludge up index assignment for
**                       EBCDIC systems for control keys.
**	25-nov-86 (bruceb)	Fix for bug 10890
**		In FKismapped, if mapped to menu item, but no visible
**		menu, return FAIL.
**	12/24/86 (dkh) - Added support for new activations.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	24-apr-89 (bruceb)
**		In FKmapkey(), if fdopSHELL key, and shell_enabled is not
**		TRUE, then return 'bad key'.
**	08/28/89 (dkh) - Added support for inquiring on frskey that
**			 caused activation.
**	27-sep-89 (bruceb)
**		No longer use 'shell_enabled', but same intent exists.
**		Same logic now applies also to the editor function key.
**	11/24/89 (dkh) - Added support for goto's from wview.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	16-mar-90 (bruceb)
**		Add support for locator events.  New function pointer passed
**		into FKmapkey() for use in determining what has occurred
**		when KS_LOCATOR input is detected.  The functions are
**		responsible for any necessary modfications to the evcb.
**	27-apr-90 (bruceb)
**		Generate FTvisclumu() when clicking on a menuitem.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	15-nov-91 (leighb) DeskTop Porting Change:
**		CTRLU (^U) is a section marker, not a control character
**		on the IBM/PC.
**  15-feb-1996(angusm)
**      Change FKmapkey() to prevent SEGV if menuline is blank
**      (ie frskeys only). bug 48319.
**      02-may-1997 (cohmi01)
**          Add fdopSCRTO for MWS scroll bar button positioning.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-Jan-2007 (kibro01) b117462
**	    If a key is deactivated, don't let it get pressed.  Beep instead.
**	2-Mar-2007 (kibro01) b117829
**	    Extend dectivation check (b117462) to confirm the item which is
**	    deactivated has an intr_val which maps to a menu item directly 
**	    (i.e. it is <= the number of visible menu items in this menu_).  If
**	    it doesn't, then it's a standard function key like F3-End, and 
**	    doesn't get deactivated, so also doesn't have an active flag set.
**	1-May-2007 (kibro01) b118038
**	    If there were 5 items in a menu, 5 function keys will be given
**	    mappings to items in that menu.  If one is then deactivated the
**	    function key still has its mapping, but doesn't do anything, so
**	    the frame drops out instantly.  It should beep when it has no
**	    function.
**	4-Jun-2007 (kibro01) b118422
**	    Keys did not work in vifred, because the menu to be counted
**	    needs to include all non-user items, not just user-defined ones.
**	4-Sep-2007 (kibro01) b117829/b118718
**	    Add in FTmukeyactive to determine finally how to process the
**	    disabled flag of a key - it can't be determined from inside
**	    this module.
**	25-Sep-2008 (kibro01) b120797
**	    Ensure that the fixes for bugs 117462/117829/118038/118422/119331
**	    and 118718 all work for vt100 terminals, where the function key
**	    numeric mapping works differently, and puts the values beyond the
**	    visible list.  Note that the change to FTmukeyactive resolved the
**	    requirement to perform the < FTmuvisible() check here.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	"ftframe.h"
# include	<ctrl.h>
# include	<tdkeys.h>
# include	<mapping.h>
# include	<frsctblk.h>
# include	<cm.h>
# include	<er.h>

FUNC_EXTERN	bool	FTvismenu();
FUNC_EXTERN	bool	FTmukeyactive();
FUNC_EXTERN	i4	IIFTmiMenuInx();
FUNC_EXTERN	i4	FTmuvisible(bool);

GLOBALREF	FRS_GLCB	*IIFTglcb;


STATUS
FKismapped(code, flg, val, evcb)
i4	*code;
i4	*flg;
bool	*val;
FRS_EVCB *evcb;
{
	i4	lcode;
	i4	index;
	i4	num;
	i1	lval;

	evcb->event = fdNOCODE;
	lcode = *code;
	if (lcode == ftKEYLOCKED || lcode == ftNOTDEF)
	{
		return(FAIL);
	}
	if (lcode >= ftMUOFFSET && lcode < ftFRSOFFSET)
	{
		/* if no visible menu, return FAIL to cause 'beep' */
		if (!FTvismenu())
			return(FAIL);

		/* If it is beyond the visible menu items (because one
		** was switched off, for example, then beep (kibro01) b118038
		*/
		if (lcode - ftMUOFFSET >= FTmuvisible(TRUE))
			return(FAIL);

		/*
		**  A menu item, handle it.
		*/
		*code = fdopMENU;
		*flg = fdcmINBR;
		*val = FALSE;

		/*
		**  Log menu item selection into event block
		**  Don't forget to set the validate and
		**  activate booleans.
		*/
		num = lcode - ftMUOFFSET;
		evcb->event = fdopMUITEM;
		evcb->mf_num = num;

		FTdmpmu();

		FTvisclumu();
	}
	else if (lcode >= ftFRSOFFSET && lcode <= ftMAXCMD)
	{
		/*
		**  A FRS key mapping.  Check for validation
		**  on it.
		*/
		index = lcode - ftFRSOFFSET;
		lcode = frsoper[index].intrval;
		if (lcode == fdopERR)
		{
			return(FAIL);
		}
		*code = lcode;
		*flg = fdcmINBR;
		lval = frsoper[index].val;
		*val = lval;

		/*
		**  Log frs key selection into event block.
		**  Don't forget to set the validate and
		**  activate booleans.
		*/
		evcb->event = fdopFRSK;
		evcb->mf_num = index;
		evcb->val_event = lval;
		evcb->act_event = frsoper[index].act;
		evcb->intr_val = lcode;
		
		/* If the key is deactivated, don't let it get pressed
		** (b117462) kibro01 - Call FTmukeyactive to check properly
		** since it can't be checked here (kibro01)
		*/
		if (!FTmukeyactive(lcode))
		{
			return(FAIL);
		}

		(*FTdmpmsg)(ERx("A FRS KEY WAS PRESSED WITH VALUE %d\n"),
			lcode);

		FTvisclumu();
	}
	else
	{
		return(FAIL);
	}
	return(OK);
}


STATUS
FKmapkey(key, loc_decode, code, flg, val, cp, evcb)
KEYSTRUCT	*key;
VOID		(*loc_decode)();
i4	*code;
i4	*flg;
bool	*val;
u_char	*cp;
FRS_EVCB *evcb;
{
	i4		lcode;
	i4		lflg;
	i4		index;
	struct	mapping	*mptr;
	STATUS		retval = FAIL;
	bool		lval = FALSE;
	i4		event;

	/*
	**  Fix for BUG 7551 which only appears on SPHINX. (dkh)
	*/
	*code = fdopERR;
	*flg = fdcmNULL;
	*val = FALSE;

	if (key->ks_fk == KS_LOCATOR)
	{
	    /*
	    ** No locator routine indicates that all locator events are
	    ** illegal.
	    */
	    if (loc_decode == NULL)
	    {
		evcb->event = fdNOCODE;
	    }
	    else
	    {
		(*loc_decode)(key, evcb);
		event = evcb->event;
		if ((event == fdopMUITEM) || (event == fdopRET)
		    || (event == fdopSCRLT) || (event == fdopSCRRT))
		{
		    if (event == fdopMUITEM)
		    {
			FTdmpmu();
			FTvisclumu();
		    }
		    *code = fdopMENU;
		    *flg = fdcmINBR;
		    retval = OK;
		}
		else if (event != fdNOCODE)
		{
		    /* e.g. fdopGOTO, fdopHSPOT and fdopSCRUP. */
		    *code = event;
		    *flg = fdcmINBR;
		    retval = OK;
		}
	    }
	}
	else if (key->ks_fk == KS_GOTO || key->ks_fk == KS_SCRTO)
	{
		/*
		**  We have a goto command from wview, or a 
		**  scroll-to cmd from mws.
		*/
		if (key->ks_fk == KS_GOTO)
		    *code = fdopGOTO;
		else
		    *code = fdopSCRTO;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = *code;
		evcb->gotofld = key->ks_p1;
		evcb->gotorow = key->ks_p2;
		evcb->gotocol = key->ks_p3;
	}
/*
** bug 48319
*/
	else if ( (key->ks_fk == KS_MFAST) && (FTvismenu() == TRUE))
	{
		/*
		**  We have a menu fast path command form wview.
		*/
		*code = fdopMENU;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopMUITEM;

		/*
		**  Menuitem position numbers are zero indexed.
		*/
		evcb->mf_num = key->ks_p1;
		if (key->ks_p2)
		{
			evcb->mf_num += IIFTmiMenuInx();
		}

		FTdmpmu();
		FTvisclumu();
	}
	else if (key->ks_fk == KS_HOTSPOT)
	{
		*code = fdopHSPOT;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopHSPOT;
		evcb->mf_num = key->ks_p1;
	}
# ifdef DATAVIEW
	else if (key->ks_fk == KS_SCRUP)
	{
		*code = fdopSCRUP;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopSCRUP;
	}
	else if (key->ks_fk == KS_SCRDN)
	{
		*code = fdopSCRDN;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopSCRDN;
	}
	else if (key->ks_fk == KS_UP)
	{
		*code = fdopUP;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopUP;
	}
	else if (key->ks_fk == KS_DN)
	{
		*code = fdopDOWN;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopDOWN;
	}
	else if (key->ks_fk == KS_NXITEM)
	{
		*code = fdopNXITEM;
		*val = FALSE;
		*flg = fdcmINBR;
		retval = OK;

		evcb->event = fdopNXITEM;
	}
# endif	/* DATAVIEW */
	else if (key->ks_fk != 0)
	{
		/*
		**  Activates on FRS keys should work correctly
		**  since cmdval for it should have been setup
		**  already from runtime.
		**  If a key has no mapping or has been locked,
		**  just return FAIL and calling routine should
		**  output a beep or whatever is appropriate.
		*/
		index = key->ks_fk - KEYOFFSET + MAX_CTRL_KEYS;
		mptr = &keymap[index];
		lcode = mptr->cmdval;
		if (lcode < fdopERR)
		{
			/*
			**  A regular forms system command.
			*/

			*code = lcode;
			*val = FALSE;
			*flg = mptr->cmdmode;
			retval = OK;

			evcb->event = lcode;
		}
		else
		{
			retval = FKismapped(&lcode, &lflg, &lval,
				evcb);
			if (retval == OK)
			{
				*code = lcode;
				*flg = lflg;
				*val = lval;
			}
		}
	}
	else
	{
		/*
		**  All regular characters (including 8 bit ones)
		**  come here.
		*/
		lcode = froperation[key->ks_ch[0]&0377];
		lflg = fropflg[key->ks_ch[0]&0377];
		if (lcode == fdopERR)
		{
			/*
			**  No old style activates.  Must check to
			**  see if char is a control char we need
			**  to worry about.
			*/
			index = -1;
			if (key->ks_ch[0] == ctrlESC)
			{
				index = CTRL_ESC;
			}
			else if (key->ks_ch[0] == ctrlDEL)
			{
				index = CTRL_DEL;
			}
# ifndef EBCDIC
			else if (key->ks_ch[0] <= ctrlZ &&	 
# ifdef	PMFE							 
				key->ks_ch[0] != ctrlU &&	 
# endif								 
				key->ks_ch[0] >= ctrlA)		 
			{
				index = key->ks_ch[0] - ctrlA;
			}
# else
                        /*
                        **  For EBCDIC, you must kludge up the matching
                        **  of control A-Z with the proper index into
                        **  the 'keymap' table.
                        */
                        else switch(key->ks_ch[0])
			{
                           case 0x01:          /* ctrlA */
                                   index = 0;
                                   break;
                           case 0x02:          /* ctrlB */
                                   index = 1;
                                   break;
                           case 0x03:          /* ctrlC */
                                   index = 2;
                                   break;
                           case 0x37:          /* ctrlD */
                                   index = 3;
                                   break;
                           case 0x2d:          /* ctrlE */
                                   index = 4;
                                   break;
                           case 0x2e:          /* ctrlF */
                                   index = 5;
                                   break;
                           case 0x2f:          /* ctrlG */
                                   index = 6;
                                   break;
                           case 0x16:          /* ctrlH */
                                   index = 7;
                                   break;
                           case 0x05:          /* ctrlI */
                                   index = 8;
                                   break;
                           case 0x25:          /* ctrlJ */
                                   index = 9;
                                   break;
                           case 0x0b:          /* ctrlK */
                                   index = 10;
                                   break;
                           case 0x0c:          /* ctrlL */
                                   index = 11;
                                   break;
                           case 0x0d:          /* ctrlM */
                                   index = 12;
                                   break;
                           case 0x0e:          /* ctrlN */
                                   index = 13;
                                   break;
                           case 0x0f:          /* ctrlO */
                                   index = 14;
                                   break;
                           case 0x10:          /* ctrlP */
                                   index = 15;
                                   break;
                           case 0x11:          /* ctrlQ */
                                   index = 16;
                                   break;
                           case 0x12:          /* ctrlR */
                                   index = 17;
                                   break;
                           case 0x13:          /* ctrlS */
                                   index = 18;
                                   break;
                           case 0x3c:          /* ctrlT */
                                   index = 19;
                                   break;
                           case 0x3d:          /* ctrlU */
                                   index = 20;
                                   break;
                           case 0x32:          /* ctrlV */
                                   index = 21;
                                   break;
                           case 0x26:          /* ctrlW */
                                   index = 22;
                                   break;
                           case 0x18:          /* ctrlX */
                                   index = 23;
                                   break;
                           case 0x19:          /* ctrlY */
                                   index = 24;
                                   break;
                           case 0x3f:          /* ctrlZ */
                                   index = 25;
                                   break;

			}
# endif /* EBCDIC */
 
			/*
			**  If is a control char we have to worry about
			*/
			if (index >= 0)
			{
				mptr = &keymap[index];
				lcode = mptr->cmdval;
				lflg = mptr->cmdmode;
				if (lcode < fdopERR)
				{
					/*
					**  A regular forms system command.
					*/
					*code = lcode;
					*val = FALSE;
					*flg = lflg;
					retval = OK;

					evcb->event = lcode;
				}
				else
				{
					retval = FKismapped(&lcode, &lflg,
						&lval, evcb);
					if (retval == OK)
					{
						*code = lcode;
						*flg = lflg;
						*val = lval;
					}
				}
			}
			else
			{
				/*
				**  Just a normal char.
				*/
				*code = lcode;
				*flg = lflg;
				*val = FALSE;
				retval = OK;

				evcb->event = fdNOCODE;
			}
		}
		else
		{
			*code = lcode;
			*flg = lflg;
			*val = FALSE;
			retval = OK;

			evcb->event = fdNOCODE;
		}
	}

	/*
	** If the shell key, but the shell key is not enabled for the
	** application, pretend that this is a non-mapped key.
	** True also for the editor function key.
	*/
	if ((*code == fdopSHELL && !(IIFTglcb->enabled & SHELL_FN))
		|| (*code == fdopVERF && !(IIFTglcb->enabled & EDITOR_FN)))
	{
		*code = fdopERR;
		retval = FAIL;
	}
	return(retval);
}

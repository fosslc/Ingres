/*
**  FTrunnof
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**	24 Apr 90 - (leighb) Created
**	02/11/93 (dkh) - Added forward declaration of FTTDgetch() to
**			 take care of compiler complaint.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# include   <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include   <fe.h>
# include   "ftframe.h"
# include   <menu.h>
# include   <mapping.h>
# include   <kst.h>
# include   <frsctblk.h>
# include   <cm.h>
# include   <st.h>
# include   <te.h>
# include   <er.h>
# include   <st.h>
# include   <nm.h>

GLOBALREF   i4		mutype;		/* Menu type (RING or CLASSIC)	*/
GLOBALREF   MENU        IIFTcurmenu;    /* pointer to current menu      */
GLOBALREF   FRS_GLCB *	IIFTglcb;	/* FRS Global Control Block	*/

FUNC_EXTERN	VOID		IIFTmldMenuLocDecode();
FUNC_EXTERN     KEYSTRUCT       *FTTDgetch();
FUNC_EXTERN	bool		IIFTtoTimeOut();

/*
**  Name:   FTrunNoFld - run a form with no fields.
**
**  Description:
**	Run a form with no fields.  Since this cannot
**	actually be done, just get the user's input
**	and return it.  This routine is called rather than
**	FTgetmenu because if Ring Menus are being used, the arrow
**	keys are "eaten" by FTgetring and they may be needed by the form
**	with no fields (for example in the Frame Flow Diagram in Vision).
**
**  Inputs:
**
**      mu          A pointer to a menu structure.
**      evcb        A pointer to an FRS_EVCB (event control block)
**
**  Outputs:
**
**      evcb    FTrunNoFld sets the following members of the event
**              control block:
**
**                  evcb->event
**                  evcb->intr_val
**                  evcb->mf_num
**
**  Returns:
**
**      The value returned depends on what event triggers the
**      return.  The are several possibilities:
**
**          Event                       Value returned
**          -----                       --------------
**
**          User selects a menu item    ct_enum from the com_tab structure
**                                      for the item
**
**          User presses an FRS key     fdopFRSK
**
**          A timeout occurs            fdopTMOUT
**
**	    User press the Menu Key	fdopMENU
**
**  Exceptions:
**
**  Side Effects:
**
**  History:
**
**      24-apr-91 (leighb) First written.
**	21-dec-92 (leighb) Notify Windows CL of running form with 0 flds.
**	3-feb-93 (andrewc)
**		Changed declaration of "fptr" to conform to K&R C.
*/

i4
FTrunNoFld(mu, evcb)
MENU        mu;
FRS_EVCB *  evcb;
{
	i4         ret;            /* The value to return		*/
	i4         code;           /* Parameter for FKmapkey		*/
	i4         flg;            /* Parameter for FKmapkey		*/
	bool        val;            /* Parameter for FKmapkey		*/
	u_char      dummy;          /* Parameter for FKmapkey		*/
	bool        done;           /* TRUE when done			*/
	COMTAB *    ct;
	KEYSTRUCT * ks;             /* Holds the most recent keystroke  */
	i4         (*fptr)();
	i4	    (*kyint)();	    /* Keystroke intercept routine	*/
	VOID	    (*locdecode)(); /* Mouse location decode routine	*/
	i4	    event;	    /* Save evcb->event for kyint	*/
	i4	    keytype;	    /* Type of key for kyint		*/
	i4	    keyaction;	    /* kyint returns this		*/
# ifdef DATAVIEW
	bool	    do_here = TRUE;
# endif	/* DATAVIEW */

	/*
	**  Only do key intercept for ring menu and gui menu input.
	*/
	kyint = IIFTglcb->intr_frskey0 ? IIFTglcb->key_intcp : NULL;

	if ((kyint == NULL) || (mutype == MU_TYPEIN) || (mutype == MU_LOTUS))
	{
		evcb->event = fdopMENU;
		return(fdopMENU);
	}

	IIFTcurmenu = mu;
	mu->mu_flags |= MU_FRSFRM;
	TDmove(mu->mu_window,mu->mu_window->_maxx-1,mu->mu_window->_maxy-1);
	TDrefresh(mu->mu_window);
	done = FALSE;
	evcb->event = 0;
	IIFTstsSetTmoutSecs(evcb);  /* Set timeout value */
	IIFTsnfSetNbrFields(1);
	locdecode = IIFTmldMenuLocDecode;
#ifdef	PMFEWIN3
	WNrunNoFld((i2)1);
#endif
	do      /* loop until done */
	{
		ks = FTTDgetch(mu->mu_window);      /* Get keystroke */

		if (!IIFTtoTimeOut(evcb))           /* Map key if not timeout*/
			FKmapkey(ks,locdecode,&code,&flg,&val,&dummy,evcb);

		event = evcb->event;	/* save evcb->event for kyint	*/
		ks->ks_ch[2] = EOS;

		if (ks->ks_fk != 0)
		{
			if (evcb->event == fdopHSPOT)
			{
				keytype = evcb->mf_num;
			}
			else
			{
				keytype = FUNCTION_KEY;
				if (code == fdopMENU)
				{
				    /*
				    ** Special casing for locators.
				    ** Not changing evcb->event, just
				    ** that value used by (*kyint)().
				    */
				    event = fdopMENU;
				}
			}
		}
		else keytype = CMcntrl(ks->ks_ch) ? CONTROL_KEY : NORM_KEY;

		/*
		**  Pass TRUE for on_menu parameter since keystroke
		**  interception is only enabled for menu input.
		*/
		keyaction = (*kyint)(event, keytype, ks->ks_ch, (bool) TRUE);
		if (keyaction == FRS_IGNORE)
		{
# ifdef DATAVIEW
			if ((IIMWdlkDoLastKey(TRUE, &do_here) == FAIL)
                	 && (IIMWimIsMws()))
			{
	                	ret = fdNOCODE;
				break;
			}
# endif	/* DATAVIEW */
			continue;
		}
		else if (keyaction == FRS_RETURN)
		{
			evcb->event = fdopFRSK;
			evcb->val_event = FRS_NO;
			evcb->act_event = FRS_NO;
			ret = evcb->intr_val = IIFTglcb->intr_frskey0;
			break;
		}

		/*
		**  Otherwise, drop through for normal processing.
		*/

		switch (evcb->event)
		{

		case fdopTMOUT:             /* timeout */
			ret = fdopTMOUT;
			done = TRUE;
			break;

		case fdopMUITEM:            /* menu item selected */
			if (evcb->mf_num < mu->mu_last)
			{
				ct = &(mu->mu_coms[evcb->mf_num]);
				ret = evcb->intr_val = ct->ct_enum;
				if ((fptr = mu->mu_coms[evcb->mf_num].ct_func)
						  != NULL)
					(fptr)();
				done = TRUE;
			}
			break;

		case fdopFRSK:              /* FRS key */
			ret = evcb->intr_val;
			done = TRUE;
			break;

		case fdopPRSCR:             /* Print current screen */
			FTprnscr(NULL);
			break;

		case fdopSCRLT:		    /* Scroll Menu Left */
		case fdopSCRRT:		    /* Scroll Menu Right */
			IIFTsmShiftMenu(evcb->event);
			evcb->event = fdopMENU;
		case fdopMENU:              /* Menu key - switch to Menu */
			ret = evcb->event;
			done = TRUE;
			break;

		case fdopERR:               /* a "regular" character */
		case fdNOCODE:
		default:
			break;
		}

		ks->ks_ch[2] = EOS;

	} while (!done);

#if defined(PMFEWIN3)
	WNrunNoFld((i2)0);
#endif
	return ret;
}

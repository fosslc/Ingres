/*
**  FTrun
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)ftrun.c	1.2	1/23/85";
**
**  History:
**	09/20/84 (dkh) - Created for FT split up.
**	12/24/86 (dkh) - Added support for new activations.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	05/02/87 (dkh) - Added support for tf cursor tracking.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	11/12/87 (dkh) - Changed to log current field for row highlighting.
**	12/12/87 (dkh) - FTwinerr --> IIUGerr.
**	05/06/88 (dkh) - Venus changes.
**	21-nov-88 (bruceb)
**		Set the number of seconds until timeout if it has changed.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	09/22/89 (dkh) - Porting changes integration.
**	10/02/89 (dkh) - Changed IIFTscSizeChg() to IIFTcsChgSize().
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.
**	20-mar-90 (bruceb)
**		Added locator support.  New IIFTfldFrmLocDecode() routine
**		added for determining effect of mouse button click on a form.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	26-may-92 (rogerf) DeskTop Porting Change:
**		Added scrollbar processing inside SCROLLBARS ifdef.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	26-jan-93 (leighb) DeskTop Porting Change:
**		Correct tablefield scrollbar for Windows.
**	21-mar-93 (fraser)
**		Added ifdef around reference to IIFTdsrDSRecords.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
# include	<erft.h>
# include	<ug.h>
# include	<menu.h>
# include	<er.h>
# ifdef		SCROLLBARS
# include	<wn.h>
# endif

static	i4	FTcurmode = 0;
GLOBALREF	i4	FTfldno;

GLOBALREF	FRAME	*FTfrm;
GLOBALREF	i4	mu_where;       /* Either MU_TOP or MU_BOTTOM       */
GLOBALREF	i4	IITDflu_firstlineused;

FUNC_EXTERN	VOID	FTrcrtrk();
FUNC_EXTERN	VOID	FTscrtrk();
FUNC_EXTERN	VOID	IIFTsccSetCurCoord();
FUNC_EXTERN	VOID	IIFTstsSetTmoutSecs();
FUNC_EXTERN	VOID	IIFTdcDispChk();
FUNC_EXTERN	VOID	IIFTcsChgSize();
FUNC_EXTERN	i4	IITDlioLocInObj();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	WINDOW	*IIFTwffWindowForForm();
FUNC_EXTERN	VOID	IIFTmldMenuLocDecode();


GLOBALREF   void        (*IIFTdsrDSRecords)();


i4
FTcurdisp()
{ 
	return(FTcurmode);
}


i4
FTrun(frm, mode, startfld, frscb)
FRAME	*frm;
i4	mode;
i4	startfld;
FRS_CB	*frscb;
{
	i4	code;
	FRAME	*ofrm;
	FIELD   *fd;
	TBLFLD  *tbl;
	FLDHDR  *thdr;
	i4	nbrrecds, currecd;
	i4	x_adj, y_adj;

	if ((FTgtflg & FTGTREF) != 0)
	{
		if (iigtreffunc)
		{
			(*iigtreffunc)();
		}
		if ((FTgtflg & FTGTEDIT) != 0)
		{
			if (iigtsupdfunc)
			{
				(*iigtsupdfunc)(TRUE);
			}
			if (iigtslimfunc)
			{
				(*iigtslimfunc)(-1);
			}
		}
		FTgtflg &= ~FTGTREF;
	}
	if (iigtmreffunc && (*iigtmreffunc)())
		FTgtrefmu();

	IIFTcsChgSize();

	IIFTdcDispChk();

	IIFTstsSetTmoutSecs(frscb->frs_event);

	if (mode == fdmdBROWSE)
	{
		FTcurmode = fdmdUPD;
	}
	else
	{
		FTcurmode = mode;
	}
	ofrm = frm;
	FTfrm = frm;
	FTfldno = frm->frcurfld;
        /*
        ** brett, Give mode windex command string.
        ** Give cursor mode to window interface.
        */
        IITDgvmode(FTcurmode);

	/*
	**  Check to see if current field for form is
	**  the same as when we exited from FT.	 If not,
	**  need to do any necessary cursor tracking
	**  clean up on old field.
	*/
	if (ofrm == frm && frm->frnumber != FTfldno)
	{
		FTrcrtrk(frm, frm->frnumber);
#ifdef	SCROLLBARS
	        WNSbarDestroy();
#endif
	}

	/*
	**  Do any necessary cursor tracking details for
	**  current field, even if it is the same field
	**  as before.	This takes care of a change in
	**  the current row.
	*/
	FTscrtrk(frm, FTfldno);

#ifdef	SCROLLBARS
	fd = frm->frfld[FTfldno];
	tbl = fd->fld_var.fltblfld;
	thdr = &(tbl->tfhdr);
	(*IIFTdsrDSRecords)(tbl, &nbrrecds, &currecd);
	x_adj = frm->frposx ? frm->frposx - 1 : 0;
	y_adj = frm->frposy ? frm->frposy - 1 : 0;
	if (frm->frmflags & fdISPOPUP)
        	++y_adj;      /* column header */
	if (tbl->tfrows > 0 && nbrrecds > tbl->tfrows)
	{
	        WNSbarCreate(thdr->fhposx + x_adj,  /* x coordinate */
        	             thdr->fhposy + y_adj,  /* y coordinate */
                	     thdr->fhmaxx,	    /* width  */
	                     thdr->fhmaxy,          /* height */
        	             currecd,		    /* current tbl fld row  */
                	     tbl->tfrows,           /* visible tbl fld rows */
	                     nbrrecds);		    /* dataset record count */
	}
#endif
	for(;;)				/* for ever (until code returned) */
	{
		if ((FTgtflg & FTGTEDIT) != 0)
			mode = fdcmGREDIT;
		else
			mode = FTforcebrowse(frm, frm->frcurfld) ? fdcmBRWS : frm->frmode;

		switch (mode)		/* switch on frame driver mode */
		{
			case fdcmBRWS:	/* run browse mode */
				code = FTbrowse(frm, FTcurmode, frscb);
				if (code == fdNOCODE ||	 code == fdopBRWSGO)
					break;
				frm->frnumber = frm->frcurfld;

				/*
				**  Set current screen coordinates
				**  before retruning out of FT layer.
				*/
				IIFTsccSetCurCoord();

				return(code);

			case fdcmEDIT:
			case fdcmINSRT:	/* run INSERT mode */
				code = FTinsert(frm, FTcurmode, frscb);
				switch(code)
				{
					case fdopBRWSGO:
						break;

					case fdopFEXIT:
						code = 0;

					default:
						frm->frnumber = frm->frcurfld;

						/*
						**  Set current screen
						**  coordinates before
						**  retruning out of FT layer.
						*/
						IIFTsccSetCurCoord();

						return(code);
				}
				break;

			case fdcmGREDIT:

				if (iigteditfunc)
				{
					code = (*iigteditfunc)(frscb);
				}
				if (code == fdopREFR)
				{
					(*iigtclrfunc)();
					TDrefresh (curscr);
					(*iigtreffunc)();
					(*iigtsupdfunc)(TRUE);
					(*iigtslimfunc)(-1);
					break;
				}

				/*
				**  Set current screen coordinates
				**  before retruning out of FT layer.
				*/
				IIFTsccSetCurCoord();

				return (code);

			default:
				IIUGerr(E_FT0013_ERROR__bad_editing_mo,
					UG_ERR_ERROR, 0);

				/*
				**  Set current screen coordinates
				**  before retruning out of FT layer.
				*/
				IIFTsccSetCurCoord();

				return (fdopERR);
		}
	}
}


/*{
** Name:	IIFTfldFrmLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Needs to recognize fields and hotspot trim.
**
**	Note that this routine pays no attention to the mode of the form or
**	field.  fdopGOTO code is relied upon for determining the legality of
**	landing upon any given field, and for beeping when necessary.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopGOTO, fdopHSPOT,
**			fdopSCRUP and fdopSCRDN if legal.
**	    .gotofld	Field number if legal field location.
**	    .gotorow	Table field row number if legal location.
**	    .gotocol	Table field col number if legal location.
**	    .mf_num	Trim number if location is a hotspot.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	20-mar-90 (bruceb)	Written.
*/
VOID
IIFTfldFrmLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    i4		row;
    i4		col;
    i4		seq;
    LOC		*curloc = NULL;
    IICOORD	*curobj;
    LOC		*found = NULL;
    WINDOW	*win;
    IICOORD	frmobj;
    char	buf[80];

    /*
    ** If no form exists, than illegal location.  Must have been called by
    ** IIFTmldMenuLocDecode() for an initial submenu.
    */
    if (!FTfrm)
    {
	evcb->event = fdNOCODE;
	return;
    }

    row = key->ks_p1;
    col = key->ks_p2;

    /* If user has clicked on the menu line, call menu decode routine. */
    if (row == ((LINES - 1) * (mu_where == MU_BOTTOM)))
    {
	IIFTmldMenuLocDecode(key, evcb);
	return;
    }

    /*
    ** Modify row and col values to account for screen scrolling, then
    ** check that location is within the form.
    */
    if (FTfrm->frmflags & fdISPOPUP)
    {
	row -= curscr->_starty;
	col -= curscr->_startx;

	if ((win = IIFTwffWindowForForm(FTfrm, TRUE)) == NULL)
	{
	    /* If can't find the window for this form, nothing is legal. */
	    evcb->event = fdNOCODE;
	    IITDposPrtObjSelected(ERx("a lost popup window"));
	    return;
	}

	/*
	** Using the window rather than the frame information since no way
	** of telling whether a fixed or floating popup, and hence no way
	** to determine if frame is 1 or 0's-based.
	** Use of _start[yx] is indicated by ftsyndsp.c code.
	*/
	frmobj.begy = win->_starty;
	frmobj.begx = win->_startx;
	frmobj.endy = win->_starty + win->_maxy - 1;
	frmobj.endx = win->_startx + win->_maxx - 1;
    }
    else
    {
	row -= IITDflu_firstlineused;
	row -= (curscr->_begy + curscr->_starty);
	col -= (curscr->_begx + curscr->_startx);

	frmobj.begy = FTfrm->frposy;
	frmobj.begx = FTfrm->frposx;
	frmobj.endy = FTfrm->frposy + FTfrm->frmaxy - 1;
	frmobj.endx = FTfrm->frposx + FTfrm->frmaxx - 1;
    }

    if (IITDlioLocInObj(&frmobj, row, col))
    {
	/*
	** Now, adjust the row and col values to reflect the size of the form.
	*/
	row -= frmobj.begy;
	col -= frmobj.begx;
	curloc = ((LOC **)(FTfrm->frres2->coords))[row];

	STprintf(buf, ERx("at form row #%d, col #%d"), row, col);
	IITDposPrtObjSelected(buf);
    }


    for (; curloc; curloc = curloc->next)
    {
	curobj = curloc->obj;
	if (IITDlioLocInObj(curobj, row, col))
	{
	    found = curloc;
	    break;
	}
    }

    if (found)
    {
	seq = found->sequence;
	switch (found->objtype)
	{
	  case FREGULAR:
		evcb->event = fdopGOTO;
		evcb->gotofld = seq;
		STprintf(buf, ERx("Fld #%d"), seq);
		break;

	  case FTABLE:
		evcb->event = fdopGOTO;
		evcb->gotofld = seq;
		row = evcb->gotorow = found->row_number;
		col = evcb->gotocol = found->col_number;
		STprintf(buf, ERx("TFld #%d, row #%d, col #%d"), seq, row, col);
		break;

	  case FHOTSPOT:
		evcb->event = fdopHSPOT;
		evcb->mf_num = seq;
		STprintf(buf, ERx("Trim #%d"), seq);
		break;

	  case FSCRUP:
	  case FSCRDN:
		if (FTfrm->frcurfld == seq)
		{
		    if (found->objtype == FSCRUP)
			evcb->event = fdopSCRUP;
		    else
			evcb->event = fdopSCRDN;
		    STprintf(buf, ERx("scroll of Fld #%d"), seq);
		}
		else
		{
		    evcb->event = fdNOCODE;
		    STprintf(buf, ERx("scroll on wrong field"));
		}
		break;

	  default:
		evcb->event = fdNOCODE;
		STprintf(buf, ERx("an unknown type"));
		break;
	}
	IITDposPrtObjSelected(buf);
    }
    else
    {
	evcb->event = fdNOCODE;
    }
}

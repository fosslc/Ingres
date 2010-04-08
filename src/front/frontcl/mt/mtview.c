/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"mtloc.h"
# include	<termdr.h>
# include	<fstm.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<ctrl.h>
# include	<frsctblk.h>
# include	<kst.h> /* include for KEYSTRUCT structure */
# include	<er.h>
# include	"ermt.h"
# ifdef SCROLLBARS
# include	<wn.h>
# endif
# include	<termdr.h>


GLOBALREF	WINDOW	*statscr;
GLOBALREF	WINDOW	*dispscr;
GLOBALREF	WINDOW	*bordscr;
GLOBALREF	bool	(*MTfsmorefunc)();
GLOBALREF	VOID	(*MTdmpmsgfunc)();
GLOBALREF	i4	(*MTputmufunc)();
GLOBALREF	VOID	(*MTdiagfunc)();

FUNC_EXTERN	KEYSTRUCT	*FTgetch();
FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	i4	IITDlioLocInObj();
FUNC_EXTERN	VOID	IIFTmldMenuLocDecode();
FUNC_EXTERN	VOID	IIFTsmShiftMenu();
FUNC_EXTERN	WINDOW	*IIMTgl();

VOID	IIMTfldFrmLocDecode();
VOID	IIMTcgCursorGoto();

static	i4	refresh = FALSE;
static	IICOORD	*dispwin = NULL;

/*
**	 MTview	 --  Drive the Output form
**
**	 This is a machine dependent routine which is analogous to
**	 FTrun and VTcursor; it drives a scrollable output form
**	 and reads input from the terminal, waiting for menu items
**	 to be selected.
**
**	 When valid (non-internal) menu item is selected FSview returns
**	 to its caller, who is expected to call FTgetmenu in order to
**	 get the menu item selected.
**
**	 Returns: nothing
**
**	History:
**		8/85 (stevel)	written.
**		10/85 (john)	Ported to UNIX/VMS.
**		11/85 (peter)	Add support for PrintScreen key to map
**				to the Save function.
**		6/30/86 (bruceb)	- added calls to FDdmpmsg() for display
**				of query output during testing.
**		2/24/87 (peterk) - added FRS_EVCB arg to call of FKmapkey.
**		24-sep-87 (bruceb)
**			Added call on FTshell for fdopSHELL key.
**	11/12/87 (dkh) - Moved to FT directory and renamed MTview.
**	11/21/87 (dkh) - Eliminated any possible back references.
**	11/23/87 (dkh) - Fixed jup bug 1390.
**	07-aug-89 (sylviap)
**		Added PRTKEY to allow output to be printed.
**	10/06/89 (kenl) - Refresh when save key is hit for DG.
**	17-oct-89 (sylviap)
**		Changed MTfsmorefunc to be a bool function.  Added VOID when
**		it's called.
**	01/10/90 (dkh) - Changed behavior of up/down key when it reaches
**			 window boundary.  They now cause viewport tos scroll.
**	09-feb-90 (sylviap)
**		Added new scrolling (paging) method.  up/down arrows scroll
**		one row at a time.
**	09-feb-90 (sylviap)
**		Added new scrolling (paging) method.  up/down arrows scroll
**		one row at a time.
**	22-mar-90 (bruceb)
**		Added locator support.  Can click anywhere within dispscr
**		window, or on the menu.  FKmapkey() is now called with a
**		function pointer used to determine legal locator regions on
**		the screen and the effect of clicking on those regions.
**	26-apr-90 (sylviap)
**		Added more parameters so MTview passes back current x and y
**		positions.
**	06/05/90 (dkh) - Put in changes to handle very wide output
**			 where the browse buffer can't hold two
**			 screens of data.
**	08/18/90 (dkh) - Fixed bug 31360.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	20-nov-92 (leighb) DeskTop Porting Change:
**		Added scrollbar processing inside SCROLLBARS ifdef.
**	20-mar-2000 (kitch01)
**		Bug 99136. Changes to allow correct useage of the browse
**		spill file if one has been created during record retrieval.
**  17-nov-2000 (kitch01)
**      Amend above fix. It is possible to be using the browse buffer for
**      display and then spill to disk during the call to the more output
**      function. In this case we need to setup to use the spill file for
**      all future display activities.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-mar-2002 (horda03) Bug 107271.
**         To improve performance of Browse Buffers, and to prevent the use
**         of the browse file filling memory when a large number of rows
**         have been selected, one vbfidx record will reference the position
**         in the browse file of consequative records of the same length.
**	25-feb-2004 (somsa01)
**	    Removed typecasting ncur_rcb to a long.
**	21-feb-2006 (thaju02)
**	    Update column/line indicators with cursor's current 
**	    position. (B112031)
**	14-Jun-2006 (thaju02)
**	    Update ncur_rec accordingly. (B116234)
**      11-Jul-2007 (hanal04) bug 118646
**          Ensure ncur_rec is in sync with the current record even after
**          Page Up/Down, Top, Bottom events. Also prevent scroll up/down
**          from going past the last record and hitting SEGVs.
**	26-Aug-2009 (kschendel) 121804
**	    Need termdr.h to satisfy gcc 4.3.
*/

i4
MTview(bcb, menuptr, qrb, cur_line, xpos, ypos)
BCB	*bcb;
MENU	menuptr;
PTR	qrb;
bool	cur_line;
i4	*xpos;
i4	*ypos;
{
	register WINDOW *win;
	register i4	i;
	i4		ncur_rec;
	i4		ncur_col;
	i4		csrrow;
	i4		csrcol;
	RCB		*ncur_rcb;
	i4		nmdr;		/* number of data rows */
	i4		m_select;
	i4		key;
	KEYSTRUCT	*inp;
	i4		flg;
	bool		validate;
	u_char		ch;
	FRS_EVCB	evcb;
	struct com_tab	*ct;
	i4		rows_to_scrl;
	i4		temp;
	i4		oscrollrows;
	char		msgline[MAX_TERM_SIZE + 1];
	BFPOS	*ncur_bfidx; /* Current browse file index position */
        bool		redraw = 0;
	i4		csrmove = 0;

	(*MTdiagfunc)(ERx("---- Entering FSview ----\n"));

# ifdef DGC_AOS
	if (refresh)
	{
		TDrefresh(curscr);
	}
# endif
	refresh = FALSE;

	m_select = -1;
	key = MENUKEY;
	win = dispscr;

	if (!bcb->bf_in_use)
	{
	   ncur_rec = bcb->vrec;
	   ncur_col = bcb->vcol;
	   ncur_rcb = bcb->vrcb;
	   ncur_bfidx = NULL;
	}
	else
	{
	   /* We are using the browse file to view the returned records.
	   ** The BCB first visible record number must be reset to ensure
	   ** correct operation of subsequent scrolls and calls to MTdraw.
	   ** ncur_rcb is not used if the browse file is in use.
	   */
	   ncur_rec = bcb->vrec = bcb->vbf_rec;
	   ncur_col = bcb->vcol;
	   ncur_rcb = NULL;
	   ncur_bfidx = bcb->vbfidx;
	}

	if ((*xpos == 0) && (*ypos == 0))
	{
		csrrow = 0;
		csrcol = 0;
	}
	else
	{
		/* put cursor back to previous position */
		csrrow = *ypos - win->_begy - 1;
		csrcol = *xpos - win->_begx - 1;

                /* Adjust current record details based on cursor adjustment */
                if (!bcb->bf_in_use)
                {
                    ncur_rec = bcb->vrec = bcb->vrec + csrrow;
                }        
                else
                {
                    ncur_rec = bcb->vrec = bcb->vbf_rec = bcb->vbf_rec + csrrow;
                }
	}
	(*MTdiagfunc)(ERx("FSview: ncur_rec=%d, ncur_col=%d, ncur_rcb=%x\n"),
		ncur_rec, ncur_col, ncur_rcb);

	nmdr = bcb->mx_rows - 2;

	MTdraw(bcb, menuptr, csrrow, csrcol);

	if (!dispwin)
	{
	    if ((dispwin = (IICOORD *)FEreqmem((u_i4)0, (u_i4)sizeof(IICOORD),
		FALSE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("MTview"));
	    }
	    dispwin->begy = dispscr->_begy;
	    dispwin->begx = dispscr->_begx;
	    dispwin->endy = dispscr->_begy + dispscr->_maxy - 1;
	    dispwin->endx = dispscr->_begx + dispscr->_maxx - 1;
	}
	IITDpciPrtCoordInfo(ERx("MTview"), ERx("dispwin"), dispwin);

	/*
	**   Continue to Write and Read the screen until the user
	**   selects a command key.
	*/
	while (m_select < 0)
	{
		m_select = -1;

		/*
		**	 Redraw the entire screen if it is required.
		*/
		if ( (ncur_rec != bcb->vrec) ||
		     (ncur_col != bcb->vcol) ||
		     redraw)
		{
			bcb->vrcb = ncur_rcb;
			bcb->vrec = ncur_rec;
			bcb->vcol = ncur_col;
            if (bcb->bf_in_use)
            {
               bcb->vbfidx = ncur_bfidx;
               bcb->vbf_rec = ncur_rec;
            }
			MTdraw(bcb, menuptr, csrrow, csrcol);
			redraw = 0;
		}

#ifdef SCROLLBARS
	        WNFbarCreate(FALSE, 			/* not on "real" form */
		     ncur_rec+win->_cury-1,		/* current row */
		     bcb->req_complete ? bcb->lrec 	/* max form rows */
				       : max(bcb->lrec,LINES+1),
               	     ncur_col+win->_curx-1,		/* current column */
                     bcb->mxcol);			/* max form columns */
#endif /* SCROLLBARS */

		/*
		**   Check for menu selections; set m_select to
		**   point to the corresponding menu item.
		*/

		inp = FTgetch();
		if (FKmapkey(inp, IIMTfldFrmLocDecode, &m_select, &flg,
		    &validate, &ch, &evcb) != OK)
		{
			FTbell();
			continue;
		}

		if (evcb.event == fdopFRSK)
		{
			m_select = evcb.intr_val;
		}
		else if (evcb.event == fdopMUITEM)
		{
			ct = &(menuptr->mu_coms[evcb.mf_num]);
			m_select = ct->ct_enum;
		}

		/*
		**   Take action if an 'internal' menu item was selected
		*/
		switch (m_select)
		{
		  case SVEKEY:
			refresh = TRUE;		/* fall through */
		  case HELPKEY:
		  case PRTKEY:
		  case ENDKEY:
		  case TOPKEY:
		  case BOTKEY:
		  case MENUKEY:
		  case fdopMENU:
			key = m_select;
			if (key == fdopMENU)
			{
			    if (evcb.event != fdopMENU)
			    {
				/* locator click on '<', '>' */
				IIFTsmShiftMenu(evcb.event);
			    }
			    key = MENUKEY;
			}
			m_select = 1;
			break;

		  case fdopPRSCR:
			FTprvfwin(IIMTgl());
			TDsaveLast(msgline);
			FTprnscr(NULL);
			TDwinmsg(msgline);
			TDrefresh(win);
			m_select = -1;
			break;

		  case fdopGOTO:
			IIMTcgCursorGoto(&evcb);
			csrrow = win->_cury;
			csrcol = win->_curx;
			TDrefresh(win);
			m_select = -1;
			break;

		  case fdopLEFT:
		  case fdopRIGHT:
			if (m_select == fdopRIGHT)
			{
			    if ((win->_curx >= (win->_maxx - 1)) &&
				(bcb->mxcol > win->_maxx))
			    {
				ncur_col = bcb->vcol + csrcol + 1;
				csrcol = 0;    
			    }
			    else if ((bcb->vcol + csrcol) < bcb->mxcol)
			    {
				MTscursor(win, m_select);
				csrrow = win->_cury;
				csrcol = win->_curx;
				TDrefresh(win);
				redraw = 1;
			    }
			}
			else if (bcb->vcol + csrcol)
			{
			    if (csrcol)
			    {
				MTscursor(win, m_select);
				csrrow = win->_cury;
				csrcol = win->_curx;
				TDrefresh(win);
				redraw = 1;
			    }
			    else if (bcb->vcol > bcb->mx_cols)
			    {
				ncur_col -= bcb->mx_cols;
				csrcol = win->_maxx - 1;
			    }
			}
			m_select = -1;
			break;

		  case fdopREFR:
			TDrefresh(curscr);
			m_select = -1;
			break;

		  /*
		  **	Not used since FTword now requires a frame
		  **	pointer and field number.
		  case fdopWORD:
			FTword(win, 1);
			TDrefresh(win);
			csrrow = win->_cury;
			csrcol = win->_curx;
			m_select = -1;
			break;

		  case fdopBKWORD:
			FTword(win, -1);
			TDrefresh(win);
			csrrow = win->_cury;
			csrcol = win->_curx;
			m_select = -1;
			break;
		  */

		  case fdopCLRF:
			bcb->vrec = -1;
			m_select = -1;
			break;

		  /*
		  **	Additional keys we may want to use eventually.
		  **
		  case fdopHOME:
			ncur_rec = bcb->frec;
			ncur_rcb = bcb->frcb;
			csrrow = 0;
			csrcol = 0;
			m_select = -1;
			break;
		  */

		  case fdopUP:
		  case fdopDOWN:
			if ((m_select == fdopDOWN &&
				win->_cury < (win->_maxy - 1)) ||
				(m_select == fdopUP &&
				win->_cury > 0))
			{
				MTscursor(win, m_select);
				csrrow = win->_cury;
				csrcol = win->_curx;
				TDrefresh(win);

                                if (m_select == fdopUP)
                                    ncur_rec--;
                                else 
				    ncur_rec++;
				m_select = -1;
				break;
			}
			/*
			**  Otherwise, we fall through to do scrolling
			**  since we tried to move beyond the window
			**  boundary.
			*/
		/*
		**   If the user hit a scroll key then we must adjust
		**   the frame offset values.
		*/
		  case fdopPREV:
		  case fdopNEXT:
		  case fdopNROW:
		  case fdopSCRUP:
		  case fdopSCRDN:

                        csrmove = 0;

			/*
			**   If the user hit 'down' then we may need
			**   to ask for more backend output.
			*/
			if (m_select==fdopNEXT
			    || m_select==fdopNROW
			    || m_select==fdopSCRUP
			    || m_select == fdopDOWN)
			{
				if ((!cur_line) && (m_select==fdopSCRUP))
				{
					/*
					** do not keep the current line so
					** scroll one full page
					*/
					rows_to_scrl = win->_maxy;
				}
				else if (m_select==fdopDOWN)
				{
					/*
					** if down arrow, then only scroll down
					** one row
					*/
					rows_to_scrl = 1;
				}
				else
				{
					/*
					** otherwise, scroll enough rows to
					** bring the bottom line up to the
					** top of the screen.  move cursor
					** to the top of the page.
					*/
					rows_to_scrl = win->_maxy-1;
                                        csrmove = csrrow;
                                        if(csrmove == (win->_maxy-1))
                                        {
                                            redraw = 1;
                                        }
					csrrow = 0;
				}
				/*
				**   If we are about to scroll beyond
				**   the end of the in-memory buffer
				**   then call FSmore to get
				**   additional records.
				*/
				oscrollrows = 0;
				if (ncur_rec+rows_to_scrl+nmdr > bcb->lrec) 
				{
					/*
					**  Don't automatically ask
					**  for the max number of rows.
					**  This may cause some of the
					**  rows we want to see to be
					**  scrolled to the spill file.
					**  This can happen if we don't
					**  have room for two screens of
					**  data and we only need a few
					**  more rows for the next screen.
					*/
					temp = ncur_rec + rows_to_scrl+nmdr -
						bcb->lrec;
					if (temp < rows_to_scrl)
					{
						oscrollrows = rows_to_scrl;
						rows_to_scrl = temp + 1;
					}
				       _VOID_ (*MTfsmorefunc)(bcb, rows_to_scrl,
						qrb);

                    /* After getting more records if the browse file index
                    ** is null and the browse file is now in use, then we
                    ** need to setup for reading the spill file. We must also
                    ** role the record position link list forward to the correct
                    ** position which is 1 less than the current record number
                    */
                    if ((ncur_bfidx == NULL) && bcb->bf_in_use)
                    {
                       ncur_col = bcb->vcol;
                       ncur_rcb = NULL;
                       ncur_bfidx = bcb->vbfidx;
                       for (;ncur_bfidx->recnum + ncur_bfidx->num_recs < ncur_rec; ncur_bfidx = ncur_bfidx->nbfpos);
                    }

					if (oscrollrows)
					{
						rows_to_scrl = oscrollrows;
					}
				}
    
				/*
				**   If after doing an FSmore we find
				**   that the 'current' RCB is no
				**   longer in memory we will have
				**   to special case it to account
				**   for rows that have been scrolled
				**   out by FSmore.
				*/
				if ((ncur_rec < bcb->frec) && (!bcb->bf_in_use))
				{
					/* This is not a problem when the browse file is
					** being used
					*/
					int	to_scroll;

(*MTdiagfunc)(ERx("FSview: *Error* Browse buffer too small; "));
(*MTdiagfunc)(ERx("old current record no longer in memory\n"));
(*MTdiagfunc)(ERx("\told cur_rec = %d	new first rec = %d, scrlrows = %d\n"),
	  ncur_rec,bcb->frec, rows_to_scrl);
					to_scroll = rows_to_scrl -
						(bcb->frec - ncur_rec);
					ncur_rec = bcb->frec;
					ncur_rcb = bcb->frcb;
					for (i = 0; i < to_scroll; i++)
					{
						if (ncur_rec + nmdr - 1
						    > bcb->lrec)
							break;
						ncur_rec++;
						ncur_rcb
						    = ncur_rcb->nrcb;
					}
				}
				else if (!bcb->bf_in_use)
				{
					/* Scroll using the browse buffer */
					for (i = 0; i < rows_to_scrl; i++)
					{
						if (ncur_rec >= bcb->lrec)
							break;
						ncur_rec++;
						ncur_rcb
						    = ncur_rcb->nrcb;
					}
				}
				else
				{
					/* Scroll using the browse file */
					if (rows_to_scrl + ncur_rec + nmdr - 1 > bcb->nrecs)
                                        {
                                           rows_to_scrl = bcb->nrecs + 1 - ncur_rec - nmdr;

                                           if (rows_to_scrl < 0) rows_to_scrl = 0;
                                        }

                                        
                                        ncur_rec += rows_to_scrl;

                                        while (ncur_bfidx->recnum + ncur_bfidx->num_recs < ncur_rec)
                                        {
                                           ncur_bfidx = ncur_bfidx->nbfpos;
                                        }

				}

                                ncur_rec -= csrmove;
			}
			/*
			**   If the user hit 'up' (and there more than
			**   a screenful of records) then we must move
			**   the 'new current record' marker backwards.
			*/
			else if ((bcb->nrows > nmdr) || (bcb->bf_in_use))
			{
				if ((!cur_line) && (m_select==fdopSCRDN))
				{
					/* Don't keep the current line so
					** scroll one full page
					*/
					rows_to_scrl = win->_maxy;
				}
				else if (m_select==fdopUP)
				{
					/*
					** if down arrow, then only scroll up
					** one row
					*/
					rows_to_scrl = 1;
				}
				else
				{
					/*
					** otherwise, scroll up enough rows to
					** bring the top line to the bottom.
					** move cursor to the bottom of the
					** page.
					*/
					rows_to_scrl = win->_maxy - 1;
                                        csrmove = rows_to_scrl - csrrow;
                                        if(csrrow == 0)
                                        {
                                            /* the current record will be
                                            ** the same after the scroll
                                            ** so force a redraw
                                            */
                                            redraw = 1;
                                        }
					csrrow = win->_maxy-1;
				}
				if (!bcb->bf_in_use)
				{
				   /* Scroll using the browse buffer */
				   for (i=0 ; i < rows_to_scrl; i++)
				   {
				 	   if (ncur_rcb->prcb == NULL)
						   break;
					   ncur_rec--;
					   ncur_rcb = ncur_rcb->prcb;
				   }
				}
				else
				{
				   /* Scroll using the browse file */

                                   if (rows_to_scrl >= ncur_rec)
                                   {
                                      ncur_rec = 1;

                                      ncur_bfidx = bcb->fbfidx;
                                   }
                                   else
                                   {
                                      ncur_rec -= rows_to_scrl;

                                      while (ncur_bfidx->recnum > ncur_rec)
                                      {
                                         ncur_bfidx = ncur_bfidx->pbfpos;
                                      }
                                   }
				}

                                ncur_rec += csrmove;
			}
			(*MTdmpmsgfunc)(ERget(S_MT0001_A_FRSKEY_WAS_PRESSED),
				m_select);
			m_select = -1;
			break;

		  case fdopSCRLT:	/* Left */
		  case fdopSCRRT:	/* Right */
			/*
			**   If the user hit a scroll key then we must
			**   adjust the offset values.
			*/
			if (bcb->mxcol > win->_maxx)
			{
				if ( m_select == fdopSCRRT )
				{
					ncur_col = bcb->vcol - (win->_maxx - 2 - csrcol);
				}
				else
				{
					if (csrcol == 0)
						csrcol = win->_maxx - 2;
					ncur_col = bcb->vcol + csrcol;
				}

				csrcol = 0;
				if (ncur_col < 1) ncur_col = 1;
				else if (ncur_col + win->_maxx - 1 > bcb->mxcol)
					ncur_col = bcb->mxcol - win->_maxx + 2;
			}
			(*MTdmpmsgfunc)(ERget(S_MT0001_A_FRSKEY_WAS_PRESSED),
				m_select);
			m_select = -1;

			break;

		  case fdopSHELL:
			FTshell();
			break;

		  default:
			FTbell();
			(*MTputmufunc)(menuptr);
			TDrefresh(win);
			m_select = -1;
			break;
		}
	}
	/* Return what the current cusor positions are */
	*ypos = win->_cury + win->_begy + 1;
	*xpos = win->_curx + win->_begx + 1;

	/*
	**   Tell caller that the user has selected a "menu" item.
	**   The number of the actual menu item selected has been
	**   squirreled away in m_select.
	*/
	(*MTdiagfunc)(ERx("---- Leaving FSview ----\n"));
	return (key);
}


MTscursor(win, dir)
WINDOW	*win;
int	dir;
{
	switch(dir)
	{
	  case fdopLEFT:
		TDmvleft(win, 1);
		break;

	  case fdopRIGHT:
		TDmvright(win, 1);
		break;

	  case fdopUP:
		TDmvup(win, 1);
		break;

	  case fdopDOWN:
		TDmvdown(win, 1);
		break;
	}
}


/*{
** Name:	IIMTcgCursorGoto	- Move the cursor under the locator.
**
** Description:
**	Move the cursor to the (already validated) position of the locator.
**
** Inputs:
**	evcb
**	    .gotorow	Row on the screen.
**	    .gotocol	Col on the screen.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**	Modifies the _cury, _curx values of dispscr to match the new location.
**
** History:
**	22-mar-90 (bruceb)	Written.
*/
VOID
IIMTcgCursorGoto(evcb)
FRS_EVCB	*evcb;
{
    char	buf[50];

    dispscr->_cury = evcb->gotorow
	- (dispscr->_begy + curscr->_begy + curscr->_starty);
    dispscr->_curx = evcb->gotocol
	- (dispscr->_begx + curscr->_begx + curscr->_startx);

    STprintf(buf, ERx("dispscr's window coords (%d,%d)"),
	dispscr->_cury, dispscr->_curx);
    IITDposPrtObjSelected(buf);
}


/*{
** Name:	IIMTfldFrmLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Legal locations are any region on the screen that fall in the output
**	window.
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopGOTO if legal.
**	    .gotorow	Row on the screen if legal location.
**	    .gotocol	Col on the screen if legal location.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	22-mar-90 (bruceb)      Written.
*/
VOID
IIMTfldFrmLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    i4		row;
    i4		col;
    char	buf[50];

    row = key->ks_p1;
    col = key->ks_p2;

    /* If user has clicked on the menu line, call menu decode routine. */
    if (row == LINES - 1)
    {
	IIFTmldMenuLocDecode(key, evcb);
	return;
    }

    if (!IITDlioLocInObj(dispwin, row, col))
    {
	evcb->event = fdNOCODE;
    }
    else
    {
	evcb->event = fdopGOTO;
	evcb->gotorow = row;
	evcb->gotocol = col;

	STprintf(buf, ERx("dispscr's screen coords (%d,%d)"), row, col);
	IITDposPrtObjSelected(buf);
    }
}

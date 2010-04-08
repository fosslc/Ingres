/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"mtloc.h"
# include	<termdr.h>
# include	<menu.h>
# include	<fstm.h>
# include	<cm.h>
# include	<er.h>
# include	"ermt.h"
# include	<uf.h>
# include	<ug.h>

GLOBALREF	WINDOW	*titlscr;
GLOBALREF	WINDOW	*statscr;
GLOBALREF	WINDOW	*dispscr;
GLOBALREF	WINDOW	*bordscr;
GLOBALREF	i4	(*MTputmufunc)();
GLOBALREF	VOID	(*MTdmpcurfunc)();
GLOBALREF	VOID	(*MTdiagfunc)();

/*
**   MTdraw  --	 Draw the appropriate section of the output form on the
**		 screen. (This routine is machine dependent)
**
**   FTdraw will write a full screen of output to the terminal,
**   consisting of 2 header lines, some properly scrolled output data,
**   a forms system menu, and a correctly positioned cursor.
**
**   History:
**	6/30/86 (bruceb)
**		Added FDdmpcur call to yield output for testing purposes.
**	11/12/87 (dkh) - Moved to FT directory and renamed MTdraw.
**	11/21/87 (dkh) - Eliminated any possible back references.
**	11/24/87 (peterk) - fixed to handle multi-line strings output thru
**		ITlinewindow.
**	02-nov-88 (bruceb)
**		Leave blank lines in the window if text on this line ends
**		before start of window.  Maintains proper vertical spacing.
**	04-jan-90 (sylviap)
**		Displays the title window if exists.
**	09-feb-90 (sylviap)
**		Fixed bug that would not scroll a blank line correctly.
**	08/15/90 (dkh) - Fixed bug 21670.
**	20-mar-2000 (kitch01)
**		Bug 99136. Changes to allow correct useage of browse spill
**		file if one has been created.
**      20-jun-2001 (bespi01)
**              Changes to handle varying line lengths in the isql spill file
**              meant that the meaning of the final parameter to getnextrcb()
**              changed.
**              Bug 104175 Problem INGCBT 335
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-feb-2006 (thaju02)
**	    Update column indicator with cursor's current column 
**	    position. (B112031)
**      11-Jul-2007 (hanal04) Bug 118646
**          Don't adjust the cursor position when completing the request.
**          MTview() needs to be able to be able to synchronise the
**          cursor position with ncur_rec.
*/

VOID
MTdraw(bcb, menuptr, csrrow, csrcol)
BCB	*bcb;
MENU	menuptr;
i4	csrrow;
i4	csrcol;
{
	register WINDOW *win;
	register i4	i;
	i4		tlen;
	RCB		*rcb;
	char		tmp[MAX_TERM_SIZE + 1];
	STATUS  rc;

	(*MTdiagfunc)(ERx("---- Entering FTdraw ----\n"));

	/*
	**   Start the data stream by clearing the screen to nulls
	*/
	win = dispscr;
	TDerase(win);
	TDerase(statscr);

	/*
	**   Generate header area
	**
	**   'End of Output' warning
	*/
	if ( (bcb->vrec + win->_maxy-1 >= max(bcb->nrecs, bcb->nrows))
	    && (bcb->req_complete) )
	{
		TDaddstr(statscr, ERget(F_MT0001_End_of_Output));
	}
	/*
	**   'Start of Output' warning
	*/
	else if (bcb->vrec == 1)
	{
		TDaddstr(statscr, ERget(F_MT0002_Start_of_Output));
	}
	/*
	**   'Bottom of Buffer' warning
	*/
	else if (bcb->vrec + win->_maxy-1 >= bcb->nrows)
	{
		TDaddstr(statscr, ERget(F_MT0003_Scroll_down_for_more));
	}
	/*
	**   'Top of Buffer' warning
	*/
	else if (bcb->vrec == bcb->frec)
	{
		TDaddstr(statscr, ERget(F_MT0004_Start_of_in_memory_ou));
	}

	/*
	**   Row and Column Indicators
	*/
	tmp[0] = '\0';
	if (bcb->mxcol > win->_maxx-1)
	{
		STprintf(tmp, ERget(F_MT0005_Column_header_info),
		    bcb->vcol + csrcol, bcb->mxcol);
	}

	i = STlength(tmp);

	if (bcb->req_complete)
	{
		STprintf(&tmp[i], ERget(F_MT0006_Line_header_info), bcb->vrec,
			max(bcb->nrecs, bcb->nrows));
	}
	else
	{
		STprintf(&tmp[i], ERget(F_MT0007_Line_info_inc), bcb->vrec);
	}

	i = STlength(tmp);
	i = statscr->_maxx - i - 1;

	TDmove(statscr, 0, i);
	TDaddstr(statscr, tmp);

	/*
	**   Generate display area
	*/

	if (!bcb->bf_in_use)
	   rcb = bcb->vrcb;
	else
	{
	   if( (rcb = (RCB *)MEreqmem( 0, RCBLEN+bcb->mxcol+2, TRUE, NULL )) == NULL )
	   {
	      /* By calling iiufBrExit() we guarentee to clear up before 
	      ** termination
	      */
	      iiufBrExit( bcb, E_UG0009_Bad_Memory_Allocation, 1, "MTdraw");
	      return;
	   }
	   rc = IIUFgnr_getnextrcb(bcb, &rcb, 0);
	}

	for (i = 0; (i < win->_maxy) && rcb ; i++)
	{
		/*
		** this used to take the min of the length of the string,
		** or win->_maxx -1, but the -1 was removed; if the output
		** turned out to be exactly 80 chars wide (like the help 
		** output), the last char (rcb->data[79]) would get wiped
		** out with '\0', and the FSTM wouldn't want to scroll 
		** right because it didn't think anything was over there.
		*/
		tlen = min(rcb->len - bcb->vcol + 1, win->_maxx);

		if (tlen>0)
		{
			u_char	*begpos = (u_char *)(rcb->data);
			u_char	*svpos;
			u_char	*curpos = (u_char *)(&rcb->data[bcb->vcol-1]);
			u_char	*endpos =
				   (u_char *)(&rcb->data[bcb->vcol-1 + tlen-1]);

			MEcopy(&rcb->data[bcb->vcol-1], tlen, tmp);

			/*
			** Check if a 2nd byte of Double Bytes character
			** is on first position of screen.
			*/
			svpos  = curpos;
			curpos++;
			CMprev(curpos, begpos);
			if (curpos != svpos)
			{
				tmp[0] = '@';
			}
			/*
			** Check if a 1st byte of Double Bytes character
			** is on last position of screen.
			*/
			svpos  = endpos;
			endpos++;
			CMprev(endpos, begpos);
			if (endpos == svpos && CMdbl1st(endpos))
			{
				tmp[tlen-1] = '%';
			}
			tmp[tlen] = '\0';
			TDmove(win, i, 0);
			ITlinewindow(win, tmp);
		}
		else
		{
			/*
			** If this line ends before the current start of
			** screen than leave a blank line as a spacer.
			*/
			win->_cury++;
		}

		if (!bcb->bf_in_use)
		   rcb = rcb->nrcb;
		else
		{
		   MEfill(sizeof(*rcb->data), 0, &rcb->data);
		   rc = IIUFgnr_getnextrcb(bcb, &rcb, i + 1);
		   if ((rc != OK) && (rc != ENDFILE))
		   {
			  TDmove(win, i+1, 0);
			  ITlinewindow(win, "End of output - Unexpected error reading browse file");
		      break;
		   }
		}
	}
	if (titlscr != NULL)
	{
		/* Display title if there is one */
		TDrowhilite(titlscr, 0, _RVVID);
		TDtouchwin(titlscr);
		TDrefresh(titlscr);
	}
	TDrowhilite(statscr, 0, _RVVID);
	TDtouchwin(statscr);
	TDrefresh(statscr);
	TDtouchwin(bordscr);
	TDrefresh(bordscr);
	(*MTputmufunc)(menuptr);
	TDmove(win, csrrow, csrcol);
	TDrefresh(win);


	if (bcb->req_complete && !bcb->eor_warning)
	{
		bcb->eor_warning = TRUE;
	}
		
	if (bcb->bf_in_use)
	{
	   MEfree((PTR)rcb);
	   if (SIclose(bcb->bffd) != OK)
	   {
	      /* This is not a grave error here. We may attempt to use the
	      ** file again later and if the problem persists the error will
	      ** be reported elsewhere
	      */
	      (*MTdiagfunc)(ERx("-- MTgetnextrcb spill file close failure --\n"));
	   }
	   else
	      bcb->bffd = NULL;
	}

	(*MTdmpcurfunc)();	/* put current screen to test output file */
	(*MTdiagfunc)(ERx("---- Leaving FTdraw ----\n"));
}

/*
**	iitbscrll.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<frserrno.h>
# include	<cm.h>
# include	<er.h>
# include	"ertb.h"
# include	<rtvars.h>

/**
** Name:	iitbscrll.c	-	set scroll modes
**
** Description:
**
**	Public (extern) routines defined:
**		IItbsmode()	Set scroll mode
**		IItscroll()	Scroll startup routine
**	Private (static) routines defined:
**
**	NOTE - This file does not use IIUGmsg since it does
**	not restore the message line.
**
** History:
**	04-mar-1983	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	12/12/87 (dkh) - Added comment on using IIUGmsg.
**	08/28/89 (dkh) - Changed code to prevent scrolling when form is in
**			 READ mode.  This applies even if table field
**			 is in FILL or QUERY mode.
**	04/25/96 (chech02)
**			- Added function type i4  to IItscroll() for
**			  windows 3.1 port.
**	04/15/97 (cohmi01)
**	    Change IItscroll() to call new FDrsbRefreshScrollButton() func
**	    if a scroll was done to a tablefield.	(bug 81574)
**	06/18/97 (hayke02)
**	    Return from IItscroll() after call to IIscr_scan() or
**	    IIscr_tb().
**	07/09/97 (kithc01)
**		backout above change but ensure ds is initialized to prevent
**		SIGBUS.
**	23/12/97 (kitch01)
**	    Bugs 87709, 87808, 87812, 87841.
**		Removed function IItput_done as it was only required by MWS and is now
**		superseded by IItscroll_init.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

GLOBALREF	TBSTRUCT	*IIcurtbl;

static i2	scrmd = sclNONE;		/* bad scroll mode used */


/*{
** Name:	IItbsmode	-	Set scroll mode
**
** Description:
**	Set the scroll mode for the current table.
**	 Based on the actual mode, set the current row for any
**	 following OUT lists.  These lists may be used by a following
**	 IIretcol(), which needs to know the current row/record.
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**	modestr		String containing the mode to set
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## scroll "" tbl2 up
**
**	if ( IItbsetio(1,"","tbl2",-3) != 0 )
**	{
**		IItbsmode("up");
**		...
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

i4
IItbsmode(modestr)
char	*modestr;
{
	register DATSET		*ds;
	register TBSTRUCT	*tb;
	char			*mode;
	char			m_buf[MAXFRSNAME+1];
	u_char			tmp_char[3];	/* temp buf for case convert */

	tb = IIcurtbl;
	mode = IIstrconv(II_CONV, modestr, m_buf, (i4)MAXFRSNAME);

	ds = tb->dataset;

	CMtolower(mode, tmp_char);
	switch (*tmp_char)
	{
	  case 'u':
		/* scroll up -- outgoing row is first */
		scrmd = sclUP;
		tb->tb_rnum = 1;	/* for OUT list, if exists */
		if (ds != NULL)
			ds->crow = ds->distop;
		break;

	  case 'd':
		/* scroll down -- outgoing row is last */
		scrmd = sclDOWN;
		tb->tb_rnum = tb->tb_display;	/* for OUT list, if exists */
		if (ds != NULL)
			ds->crow = ds->disbot;
		break;

	  case 't':
		scrmd = sclTO;
		break;

	  default:
		scrmd = sclNONE;
		/* bad mode given on "## scroll" */
		IIFDerror(TBBADSCR, 2, (char *) mode, tb->tb_name);
		return (FALSE);
		break;
	}
	return (TRUE);
}


/*{
** Name:	IItscroll	-	Scroll tablefield
**
** Description:
**	Set up scroll, and call actual scrolling routines.
**
**	If no data set is linked then just call the scroll routine.
**	If there is a data set then if info can be scrolled in from
**	data set records then execute the scroll. Otherwise the scroll
**	is out of data and return FALSE.  Note that if the table is an
**	APPEND type table then an attempt to scroll UP (ie: move the
**	cursor DOWN) will add a "fake" row to the screen to be filled
**	by the user.
**	No IN list may used for a table linked to a data set.
**
**	When the user hits the bottom of the table field display with
**	the cursor this causes a scroll UP -- bring UP data from below.
**	When hitting the top it causes a scroll DOWN -- bring DOWN data
**	from above.
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**	tofill		The EQUEL 'IN' list
**	recnum		Record number to scroll to
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## scroll "" tbl2 up out (param(list,addr)) in (col1=buf)
**
**	if (IItbsetio(1,"","tbl2",-3) != 0 )
**	{
**		IItbsmode("up");
**		IItrc_param(list,addr,(int *) 0);
**		if (IItscroll(0,-3) != 0 )
**		{
**			IItcolset("col1",1,32,0,buf);
**		}
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04/15/97 (cohmi01)
**	    If tablefield was scrolled, call FDrsbRefreshScrollButton() so
**	    GUI front ends can update button position. (bug 81574)
**	06/18/97 (hayke02)
**	    Return from IItscroll() after call to IIscr_scan() or
**	    IIscr_tb(). This prevents SIGBUS when uninitialized ds is
**	    tested and used in call to FDrsbRefreshScrollButton() after
**	    the call to IIscr_scan().
**	07/09/97 (kitch01)
**		The above fix prevents FDrsbRefreshScrollButton() from being called
**		to allow us to notify GUI front ends of the changed scrollbar
**		button position. Backed out the change and added code to ensure
**		ds is initialized before call to FDrsbRefreshScrollButton().
**
*/

i4
IItscroll(i4 tofill, i4 recnum)
{
    register DATSET	*ds;		/* current data set */
    register TBSTRUCT	*tb;		/* current table field */
    i4			retcode = TRUE;	

    tb = IIcurtbl;
    if (scrmd == sclTO)
    {
	/* scanning mode */
	retcode = IIscr_scan(tb, recnum);
    }
    else if ((ds = tb->dataset) == NULL)
    {
	/* scroll of regular table field */
	retcode = IIscr_tb(tb, scrmd, tofill);
    }
    else if (tofill)
    {
	/* IN list with data set */
	IIFDerror(TBSCINLIST, 1, (char *) IIcurtbl->tb_name);
	retcode = FALSE;
    }
    else
    {
	/* current table is linked to a data set */
	switch (scrmd)
	{
	    case sclUP:
		/* bottom  of data set is last row of display */
		if (ds->bot == ds->disbot)
		{
			/* user tried cursor-scroll UP */
			if (tb->tb_state == tbSCINTRP)
			{
				/*
				**  If form is in read mode, don't
				**  allow user to open new rows.
				**  Using IIstkfrm since user caused
				**  scrolling can only occur in
				**  the current form.
				*/
				if (IIstkfrm->fdrunfrm->frmode == fdcmBRWS)
				{
					retcode = FALSE;
					break;
				}

				/* user can append to display */
				if (tb->tb_mode == fdtfAPPEND ||
					tb->tb_mode == fdtfQUERY)
				{
					TBLFLD	*tbl;

				/*
				**  There is no 3rd argument is actual
				**  declaration of IIscr_fake. (dkh)
				**	IIscr_fake(tb, ds, IIstkfrm->fdrunfrm);
				*/

					IIscr_fake(tb, ds);

					/*
					**  If we are doing mulitple scrolls
					**  in updatable table fields, then
					**  we only do a scroll fake for
					**  one row.
					*/

					tbl = tb->tb_fld;
					if (tbl->tfcurrow > tbl->tflastrow)
					{
						tbl->tfcurrow = tbl->tflastrow;
						tbl->tfstate = tfNORMAL;
					}
					retcode = TRUE;
					break;
				}
				retcode = FALSE; /* cannot append */
				break;
			}
			/* FDerror(TBEMPTSCR, 1, tb->tb_name); */
			winerr(ERget(F_TB0005_Out_of_data_below));
			IIsetferr(TBEMPTSCR);
			retcode = FALSE;
			break;
		}
		IIscr_ds(tb, sclUP);		/* do actual scroll */
		break;

	  case sclDOWN:
		/* top of data set is first row of display */
		if (ds->top == ds->distop)
		{
			/* give error message only if an Equel scroll */
			if (tb->tb_state != tbSCINTRP)
			{
				/* FDerror(TBEMPTSCR, 1, tb->tb_name); */
				winerr(ERget(F_TB0006_Out_of_data_above));
				IIsetferr(TBEMPTSCR);
			}
			retcode = FALSE;
			break;
		}
		IIscr_ds(tb, sclDOWN);
		break;

	  default:
		retcode = FALSE;
		break;
	}
    }

    /* 
    ** If we have a dataset, and retcode indicates that a scroll was 
    ** done, update Scroll Bar button position.
    */
    if (((ds = tb->dataset) != NULL) && (retcode == TRUE))
	   FDrsbRefreshScrollButton(tb->tb_fld, ds->distopix, ds->ds_records);

    return (retcode);
}


/*{
** Name:	IItscroll_init	- Initialize any tablefield scrollbars
**
** Description:
**	Using the currently displayed form, send scrollbar button
**	messages for all tablefields on the form. 
**	Currently needed only by MWS.
**
** Inputs:
**	None. Uses global variable IIfrmio.
**
** Outputs:
**
** Returns:
**	None
**
** Exceptions:
**	none
**
** Side Effects:
**	Messages sent to MWS client.
**
** History:
**	23-dec-1997 (kitch01)
**	    Added for bugs 87709, 87808, 87812, 87841.
**
*/
VOID
IItscroll_init()
{
	TBSTRUCT *tbstruct;
	DATSET	*ds;

	if ((IIfrmio == NULL) || (IIfrmio->fdruntb == NULL))
	 	return;

	for (tbstruct = IIfrmio->fdruntb; tbstruct != NULL;
			   tbstruct = tbstruct->tb_nxttb)
	{
		if ((ds = tbstruct->dataset) != NULL)
			FDrsbRefreshScrollButton(tbstruct->tb_fld, ds->distopix, ds->ds_records);			
	}

	return;
}

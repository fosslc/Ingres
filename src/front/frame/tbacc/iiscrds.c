/*
**	iiscrds.c
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
# include	<rtvars.h>

/**
** Name:	iiscrds.c
**
** Description:
**	Scroll a tablefield dataset
**
**	Public (extern) routines defined:
**		IIscr_ds()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	18-aug-89 (bruceb)
**		When NEWEMPT rows are removed from the dataset, invalidate
**		the aggregates and re-process them.
**	02/05/92 (dkh) - Updated to handle interface change to IIdisprow().
**	03/16/97 (cohmi01)
**	    Rewrite to support block mode scrolling - for bug 73587.
**	    As a result tfcurrow is incr/decr here, causing fewer calls
**	    to this routine from tbchkscr/tbophdlr logic.
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();
FUNC_EXTERN	VOID	IIFDpadProcessAggDeps();


/*{
** Name:	IIscr_ds	-	Scroll tablefield dataset
**
** Description:
**	Scroll a tablefield one or more rows at a time.  Before a row is
**	scrolled off the screen, call IIretrow to update the
**	dataset with any changes.  Then call FDcopytab() to
**	copy the displayed data up or down.  Finally, call
**	IIdisprow to display the new row scrolled in.
**
**	A scroll UP means get data records from BELOW,
**	A scroll DOWN means get data records from ABOVE.
**	The amount of rows to scroll is determined by the difference
**	between the tfcurrow  and  tflastrow values for the tablefield.
**
** Inputs:
**	tb		Ptr to the tablefield to scroll
**	direc		Flag - scroll up or down
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates the pointers in the dataset structure to reflect the
**	rows that are currently displayed.
**	The  fields tfcurrow is incr/decr based on size of scroll done.
**
** History:
**		04-mar-1983 	- written (ncg)
**		30-apr-1984	- Improved interface to FD routines (ncg)
**		13-jun-1985	- Remove "fake-row" if scrolled out under the
**				  data set. (ncg)
**		18-feb-1987	- Changed rp_rec reference to rp_data (drh)
**	03/16/97 (cohmi01)
**	    Rewrite to support block mode scrolling - for bug 73587.
**	    As a result tfcurrow is incr/decr here, causing fewer calls
**	    to this routine from tbchkscr/tbophdlr logic.
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**  30-july-1997 (rodjo04)
**      If num_new is zero or less, then the cursor position is 
**      still within the table limits. Do the scroll, but do not
**      adjust the cursor position.
**  9-Mar-2009 (kibro01) b121685
**	Add special case when the row being deleted is the only row on the
**	screen, in which case we are also deleting the current top row, so we
**	have to remember where to go next.
*/

IIscr_ds(tb, direc)
register TBSTRUCT	*tb;
i2			direc;
{
	register DATSET	*ds;
	bool		newaggval = FALSE;
	i4		num_scroll;
	i4		num_new;
	i4		num_left;
	i4		cur_scroll;
	TBROW		*cur_data;
	bool		save_pos = FALSE;
	TBROW		*jump_to_row = NULL;

	ds = tb->dataset;

	switch (direc)
	{
	  case sclUP:
		/*
		** determine boundaries of scroll and new data window:
		** num_new = # of new records to add to window from dataset.
		** num_scroll = number of rows to be shifted
		*/
		num_new = tb->tb_fld->tfcurrow - tb->tb_fld->tflastrow;

	        /*
		** if num_new is less than or equal to zero then 
		** the current cursor position is still within 
		** the table field limits. A scroll was requested 
		** so scroll the field ignoring cursor position.
		*/

		if (num_new <= 0)
		{        
		    num_new = 1;
		    save_pos = TRUE;
		}
        
		/* see if we really have that many new recs, else cap it */
		for (num_left = 0, cur_data = ds->disbot->nxtr;
		    (num_left < num_new) && cur_data; 
		    num_left++, cur_data = cur_data->nxtr)
		    ;
		if (num_left < num_new)
		    num_new = num_left;

		num_scroll = tb->tb_fld->tfrows - num_new;

		/*
		** scroll top rows off of screen. This is done for as many 
		** new rows as will be scrolled in (num_new). Operation
		** consists of saving rows back to the dataset list in case
		** they changed, and advancing the 'display top' pointer.
		*/
		for (cur_scroll = 0; cur_scroll < num_new; cur_scroll++)
		{
		    IIretrow(tb, cur_scroll, ds->distop); /* save to ds */
		    ds->distop = ds->distop->nxtr;	
		    ds->distopix++;	/* keep track of ordinal position */
		}
		
		/* scroll existing data up on screen */
		/*** here is where MWS could use X131 request ***/
		FDmoverows(tb->tb_fld, num_new -1, tb->tb_fld->tfrows - 1, 
		    num_new -1);

		/* scroll new records into window from dataset */
		cur_scroll = num_scroll;
		for (; num_new; cur_scroll++, num_new--)
		{
		    ds->disbot = ds->disbot->nxtr;	
		    IIdisprow(tb, cur_scroll, ds->disbot);
		}
		 
		/* adjust tfcurrow to reflect this scroll, which may have */
		/* only been a portion of the scroll requested	    	  */
		if(!save_pos)    
			tb->tb_fld->tfcurrow -= (num_left -1);

		break;

	case sclDOWN:
		/*
		** determine boundaries of scroll and new data window:
		** num_new = # of new records to add to window from dataset.
		** num_scroll = number of rows to be shifted
		*/
		num_new = -tb->tb_fld->tfcurrow; 

		/*
		** if num_new is less than or equal to zero then 
		** the current cursor position is still within 
		** the table field limits. A scroll was requested 
		** so scroll the field ignoring cursor position.
		*/

		if (num_new <= 0)
		{        
		    num_new = 1;
		    save_pos = TRUE;
		}

		/* see if we really have that many new recs, else cap it */
		for (num_left = 0, cur_data = ds->distop;
		    (num_left < num_new) && cur_data->prevr; 
		    num_left++, cur_data = cur_data->prevr)
		    ;
		if (num_left < num_new)
		    num_new = num_left;

		num_scroll = tb->tb_fld->tfrows - num_new;

		/*
		** scroll bottom rows off of screen. This is done for as many 
		** new rows as will be scrolled in (num_new). Operation
		** consists of saving rows back to the dataset list in case
		** they changed, and moving back the 'display bottom' pointer.
		*/
		for (cur_scroll = tb->tb_fld->tfrows -1; 
		    cur_scroll >= num_scroll; cur_scroll--)
		{
		    /* save bottom row before scrolling */
		    IIretrow(tb, cur_scroll, ds->disbot);
		    ds->disbot = ds->disbot->prevr;	
		    /* 
		    ** delete a "fake-row" if it has been scrolled out and is 
		    ** the last on the data set. The program should not access
		    ** this record after it is freed (see order of an OUT list).
		    */
		    if (ds->disbot->nxtr == ds->bot 
			&& ds->bot->rp_state == stNEWEMPT)
		    {
			/* Special case when the row being deleted is the
			** only row on the screen, in which case we are also
			** deleting the current top row, so we have to remember
			** where to go next (kibro01) b121685
			*/
			if (ds->bot == ds->distop)
			    jump_to_row = ds->bot->prevr;

			IIt_rfree(ds, ds->bot);
			ds->bot = ds->disbot;
			ds->bot->nxtr = NULL;
			ds->ds_records--;

			if (tb->tb_fld->tfhdr.fhd2flags & fdAGGSRC)
			{
			    IIFDiaaInvalidAllAggs(IIfrmio->fdrunfrm, 
				tb->tb_fld);
			    if (IIfrscb->frs_event->eval_aggs)
			    {
				/*
				** If eval_aggs == TRUE, this was called because
				** of an arrow up in FT.  If FALSE, than because
				** of a ##scroll, and thus will be processed 
				** later on in IIrunform().
				*/
				newaggval = TRUE;
			    }
			}
		    }
		}

		/* scroll existing data down on screen  */
		/*** here is where MWS could use X131 request ***/
		FDmoverows(tb->tb_fld, num_scroll, 0, 
		    num_new -1);

		cur_scroll = num_scroll;
		
		while (num_new--)
		{
		    if (jump_to_row)
		    {
			ds->distop = jump_to_row;
		        ds->distopix--;	/* keep track of ordinal position */
		    }
		    else
		    {
		        ds->distop = ds->distop->prevr;	
		        ds->distopix--;	/* keep track of ordinal position */
		    }
		    IIdisprow(tb, num_new, ds->distop);
		}
		if(!save_pos)    
			tb->tb_fld->tfcurrow = 0;

		if (newaggval)
		{
		    IIFDpadProcessAggDeps(IIfrmio->fdrunfrm);
		}
		break;
	
	default:
		return (FALSE);
		break;
	}
	return (TRUE);
}

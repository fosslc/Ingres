
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	"decls.h"


/**
** Name:	vfdisp.c -	Display routines for vifred.
**
** Description:
**	This file contains functions to display features on the virtual
**	screen. It was created during the addition of box/line character
**	support.
**
**	vfDispAll	display all features of the form up to given line #
**	vfBoxCount	return count of boxes in line table
**	vfDispBoxes	display all boxes on the form 
**	reDisplay	redisplay one feature of the form
**	vfTrigUpd	post the need to update the whole form
**	vfPostUpd	post start and end of region which must be 
**			fixed up latter by vfUpdate.
**	vfUpdate	fix up posted update region and update real screen
**
** History:
**	03/26/88  (tom) - created.
**	07-sep-89 (bruceb)
**		Call VTputstring with trim attributes now, instead of 0.
**	25-jan-90 (cmr)
**		For RBF call SectionsUpdate() to ensure the y coords
**		get updated for the special section lines.  This
**		routine supersedes incremental calls to newLimits().
**      04/10/90 (esd) - added extra parm FALSE to call to VTdispbox
**                       to tell VT3270 *not* to clear the positions
**                       under the box before drawing the box
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**      06/12/90 (esd) - Add extra parm to VTput[v]string to indicate
**                       whether form reserves space for attribute bytes
**	06/15/92 (dkh) - Added support for rulers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
GLOBALREF	char	*vfvtrim;
GLOBALREF	i4	vfvattrtrim;
/* extern's */
/* static's */

static i4  posted_upd = 1;

static	bool	update_suspended = FALSE;	/* suspend updates */


/*{
** Name:	vfDispAll	- redisplay all features
**
** Description:
**	Redisplay all features of the form up to a given line number.
**	This assumes that the virtual screen has just been erased.
**
** Inputs:
**	i4 last	- last line we are to consider. 
**
** Outputs:
**
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/26/87  (tom) - created
*/
VOID
vfDispAll(last)
i4	last;	
{
	register i4  i;
	POS	*ps;
	VFNODE	**nodep;
	VFNODE	*node;


	/* first we display the box/lines of the form because they are
	   conceptualized as being on a display plane below the display
	   plane of the fields/trim.. */

	vfDispBoxes(frame, last);

	/* then we display non-boxes, these will overwrite the boxes */

	for (nodep = line, i = 0; i <= last; i++, nodep++)
	{
		for (node = *nodep; node != NNIL; node = vflnNext(node, i))
		{
			ps = node->nd_pos;
			if (ps->ps_name != PBOX)
			{
				if (i == ps->ps_begy)
				{
					if (ps->ps_mask == PNORMAL)
						reDisplay(ps);
					else
						newFeature(ps);
				}
			}
		}
	}

	posted_upd = 0; /* we have just updated the whole form 
		    	   so it is not necessary to do it again 
			   until another update is vfTrigUpd'ed */
}


/*{
** Name:	vfDispBoxes	- redisplay all box features
**
** Description:
**	Redisplay all box features of the form up to a given line number.
**	This assumes that the virtual screen has just been erased.
**	Note that this routine needs the FRAME * argument because
**	it is called from the sequence build code, in addition to
**	the call from DispAll. 
**
** Inputs:
**	FRAME *frm;	- frame to paint boxes on
**	i4 last	- last line we are to consider. 
**
** Outputs:
**
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/27/87  (tom) - created
*/
VOID
vfDispBoxes(frm, last)
FRAME	*frm;
i4	last;	
{
	register i4	i;
	register i4	y;
	register i4	x;
	register POS	*ps;
	VFNODE	**nodep;
	VFNODE	*node;

	for (nodep = line, i = 0; i <= last; i++, nodep++)
	{
		for (node = *nodep; node != NNIL; node = vflnNext(node, i))
		{
			ps = node->nd_pos;
			if (ps->ps_name == PBOX)
			{
				if (i == ps->ps_begy)
				{
					y = ps->ps_begy; 
					x = ps->ps_begx, 
					VTdispbox(frm, y, x, 
						ps->ps_endy - y + 1, 
						ps->ps_endx - x + 1, 
						*((i4 *)ps->ps_feat), 
						FALSE, FALSE, FALSE);
				}
			}
		}
	}
}

/*{
** Name:	vfBoxCount	- return count of boxes in line table 
**
** Description:
**		Loop through the line table and count all of the boxes.
**
** Inputs:
**	none
**
** Outputs:
**
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	05/11/87  (tom) - created
*/
i4
vfBoxCount()
{
	register i4  i, count;
	POS	*ps;
	VFNODE	**nodep;
	VFNODE	*node;

	for (nodep = line, count = i = 0; i <= endFrame; i++, nodep++)
	{
		for (node = *nodep; node != NNIL; node = vflnNext(node, i))
		{
			ps = node->nd_pos;
			if (ps->ps_name == PBOX && i == ps->ps_begy &&
				ps->ps_attr != IS_STRAIGHT_EDGE)
			{
				count++;
			}
		}
	}
	return (count);
}



/*{
** Name:	reDisplay	- redisplay one feature 
**
** Description:
**	Given a POS structure ptr redraw the feature on the 
**	virtual screen.
**
** Inputs:
**	POS *ps		the position struct
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	03/26/88  (tom) - moved from delete.c
*/
VOID
reDisplay(ps)
register POS	*ps;
{
	register TRIM	*tr;
	register FIELD	*fd;

	switch (ps->ps_name)
	{
	case PSPECIAL:
	case PTRIM:
		/* NOSTRICT */
		tr = (TRIM *) ps->ps_feat;

		VTputstring(frame, tr->trmy, tr->trmx, tr->trmstr,
			    tr->trmflags, FALSE, IIVFsfaSpaceForAttr);
		break;

	case PFIELD:
		/* NOSTRICT */
		fd = (FIELD *) ps->ps_feat;

		vfFlddisp(fd);
		break;

# ifndef FORRBF
	case PTABLE:
		/* NOSTRICT */
		fd = (FIELD *) ps->ps_feat;

		vfTabdisp(fd->fld_var.fltblfld, FALSE);
		break;
# endif

	case PBOX:
		vfBoxDisp(ps, FALSE);
		break;
	}
}

/*{
** Name:	vfTrigUpd - trigger update of whole form when vfUpdate's called
**
** Description:
**	This routine is called when the miscelaneous display of features
**	may have messed up some box/line character join fixups. When this
**	function is called we post that the entire screen needs updating.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	04/29/88  (tom) - created
*/
VOID
vfTrigUpd()
{
	posted_upd = 1;
}



/*{
** Name:	vfUpdate	- update the real screen
**
** Description:
**	Fixup any possibly exposed box characters in the posted update
**	region, then redisplay the features which cover them.
**
** Inputs:
**	i4 lin,col;	line and column to position the cursor on.
**	i4 updflg;	flag to say if we actually update the screen.
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none 
**
** Side Effects:
**
** History:
**	03/26/88  (tom) - created
*/
VOID
vfUpdate(lin, col, updflg)
i4	lin;
i4	col;
i4	updflg;
{
	static old_endFrame = -1;
	static old_endxFrm = -1;

	/* has update fixup been requested */
	if (posted_upd)
	{
		VTerase(frame); 

#ifdef FORRBF
		/*
		** At this point we need to update the Sections list so
		** that the y coordinates for the special section lines
		** lines are correct (new lines may have been added/deleted).
		** This needs to be done before calling spcRbld().
		** [No more calls to newLimits() sprinkled about]
		*/
		SectionsUpdate();
# endif
		/* if the dimensions have changed since the last update
		   then we must redo special trim */
		if (  endFrame != old_endFrame
		   || endxFrm != old_endxFrm
		   )
		{
			old_endFrame = endFrame;
			old_endxFrm = endxFrm;
			spcRbld();
			spcVTrim();
		}
		else	/* otherwise just output the verticle string */
		{
			VTputvstr(frame, 0,
			    endxFrm + 1 + IIVFsfaSpaceForAttr,
			    vfvtrim, vfvattrtrim, FALSE,
			    IIVFsfaSpaceForAttr);
		}
		vfDispAll(endFrame); 
	}
	if (updflg == TRUE && !update_suspended)
		VTxydraw(frame, lin, col);	/* update real screen */
	posted_upd = 0; 			/* reset update flag */
}


/*{
** Name:	IIVFsuSuspendUpdate - Suspend screen updates
**
** Description:
**	This routine sets a static variable (from passed parameter) that
**	tells other routines in this file whether screen updates should be
**	performed.  This is necessary so that group moves don't cause
**	the screen to jump around if the selected fields are bigger than
**	the terminal screen.
**
** Inputs:
**	suspend		Valud to indicate whether screen update should be
**			suspended.
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
**	06/15/92 (dkh) - Initial version.
*/
void
IIVFsuSuspendUpdate(suspend)
bool	suspend;
{
	update_suspended = suspend;
}

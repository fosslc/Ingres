/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	movecomp.c - moving components in vifred/rbf
**
** Description:
**	This file contains routines to support moving components in
**	vifred/rbf.  In particular, group move support was added for 6.5.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	09/01/92 (dkh) - Added support for group moves for 6.5.
**	06/17/93 (dkh) - Fixed bugs 51322 and 51324.  The collision
**			 code in IIVFgmcGroupMoveCollide() was calculating
**			 the collision points incorrectly.  The code now
**			 detects collisions correctly no matter where the
**			 new anchor point is.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
[@history_template@]...
**/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<frsctblk.h>
# include	<vt.h>
# include	<er.h>
# include	"vffiles.h"
# include	"ervf.h"
# include	"vfhlp.h"
# include	<ug.h>

FUNC_EXTERN	i4	VFfeature();
FUNC_EXTERN	void	IIVFsuSuspendUpdate();

static	LIST	*comps_to_move = NULL;


/*
** a component of a window has been moved
** fix up the real component
*/
VOID
moveComp(ps, fd)
reg	POS	*ps;
reg	SMLFLD	*fd;
{
	reg	FIELD	*ft;
		i4	y,
			x;
		FLDHDR	*hdr;
		FLDTYPE	*type;
		FLDVAL	*val;
	
	ft = (FIELD *)ps->ps_feat;
	hdr = FDgethdr(ft);
	type = FDgettype(ft);
	val = FDgetval(ft);
	x = hdr->fhposx = ps->ps_begx;
	y = hdr->fhposy = ps->ps_begy;
	hdr->fhmaxx = ps->ps_endx - ps->ps_begx + 1;
	hdr->fhmaxy = ps->ps_endy - ps->ps_begy + 1;
	hdr->fhtitx = fd->tx - x;
	hdr->fhtity = fd->ty - y;
	type->ftdatax = fd->dx - x;
	val->fvdatay = fd->dy - y;
	vfFlddisp(ft);
}


# ifndef FORRBF
/*{
** Name:	IIVFgmcGroupMoveCollide - Check for collisions in a group move.
**
** Description:
**	This routine determines if the desired destinattion for a
**	group move will collied with any existing components.
**
** Inputs:
**	oldy	Y coordiante of old anchor
**	oldx	X coordiante of old anchor
**	endy	Y coordiante of old endpoint
**	endx	X coordiante of old endpoint
**	newy	Y coordiante of new anchor
**	newx	X coordiante of new anchor
**
** Outputs:
**
**	Returns:
**		TRUE	If there is a collision.
**		FALSE	If there is no collision.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/23/92 (dkh) - Initial version.
*/
i4
IIVFgmcGroupMoveCollide(oldy, oldx, endy, endx, newy, newx)
i4	oldy;
i4	oldx;
i4	endy;
i4	endx;
i4	newy;
i4	newx;
{
	VFNODE	*nd;
	POS	*ps;
	i4	i;
	i4	starty;
	i4	startx;
	i4	numcol;
	i4	numrow;
	i4	nendy;
	i4	nendx;

	starty = newy;
	startx = newx;
	numrow = endy - oldy;
	numcol = endx - oldx;
	nendy = starty + numrow;
	nendx = startx + numcol;

	/*
	**  Make sure we don't check the bottom margin or go beyond
	**  the bottom margin since there can't be any components on the
	**  bottom margin or beyond.  This is an optimization and ensures
	**  that we don't crash.
	*/
	if (nendy >= endFrame)
	{
		nendy = endFrame - 1;
	}

	for (i = starty; i <= nendy; i++)
	{
		for (nd = line[i]; nd != NULL; nd = vflnNext(nd, i))
		{
			ps = nd->nd_pos;
			if (ps->ps_begy == i)
			{
				/*
				**  If we see a box or one of the system
				**  maintained components, ignore them.
				**  This is OK since boxes don't cause
				**  a collision problem and system maintained
				**  stuff should not get in the way.
				*/
				if (ps->ps_name == PBOX ||
					ps->ps_attr == IS_IN_GROUP_MOVE || 
					ps->ps_name == PSPECIAL)
				{
					continue;
				}

				/*
				**  If either the starting x position or
				**  the ending x position for the
				**  component is within the desired
				**  area, then we have a collision.
				**  In this case, we just return
				**  TRUE to indicate that some sort
				**  collision was found.
				*/
				if ((ps->ps_begx <= startx &&
					startx <= ps->ps_endx) ||
					(ps->ps_begx <= nendx &&
					nendx <= ps->ps_endx))
				{
					return(TRUE);
				}
			}
		}
	}

	/*
	**  No collisions found.
	*/
	return(FALSE);
}


/*{
** Name:	IIVFfcbFindBoxedComps - Find components for group move.
**
** Description:
**	Find the form components that are enclosed in the bounding box
**	described by the passed in coordinates.
**
** Inputs:
**	starty	Y coordinate of bounding box anchor point
**	startx	X coordinate of bounding box anchor point
**	endy	Y coordinate of bounding box ending position
**	endx	X coordinate of bounding box ending position
**
** Outputs:
**	sendy	Y coordinate of shrunken ending position
**	sendx	X coordinate of shrunken ending position
**
**	Returns:
**		Number of components in the bounding box.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/22/92 (dkh) - Initial version
*/
i4
IIVFfcbFindBoxedComps(starty, startx, endy, endx, sendy, sendx)
i4	starty;
i4	startx;
i4	endy;
i4	endx;
i4	*sendy;
i4	*sendx;
{
	VFNODE	*nd;
	POS	*ps;
	i4	i;
	i4	within_count = 0;
	i4	tendy = 0;
	i4	tendx = 0;
	LIST	*lt;

	comps_to_move = NULL;

	for (i = starty; i <= endy; i++)
	{
		for (nd = line[i]; nd != NULL; nd = vflnNext(nd, i))
		{
			ps = nd->nd_pos;

			/*
			**  Skip the cross hairs or the RIGHT and BOTTOM
			**  margin markers in the selected area.
			*/
			if (ps->ps_attr == IS_STRAIGHT_EDGE ||
				ps->ps_name == PSPECIAL)
			{
				continue;
			}

			/*
			**  If component starts on the line, check
			**  to see if it fits completely within
			**  the box.  Note that components that
			**  have a common boundary with the bounding
			**  box is not considered to be "within" the
			**  box.
			*/
			if (ps->ps_begy == i)
			{
				if (ps->ps_begy >= starty &&
					ps->ps_begx >= startx &&
					ps->ps_endy <= endy &&
					ps->ps_endx <= endx)
				{
					within_count++;
					/*
					**  We now gather up the selected
					**  components into a list.
					**
					**  Will unlink the components
					**  below since we don't want
					**  to trample the structures
					**  that we are traversing to
					**  find things.
					**
					**  We don't want to erase them from
					**  the screen so users can still
					**  see what they've selected.
					*/
					lt = lalloc();
					lt->lt_var.lt_vpos = ps;
					lt->lt_next = comps_to_move;
					comps_to_move = lt;

					/*
					**  Now gather info on the rightmost
					**  and bottommost position of the
					**  selected position.
					*/
					if (tendy < ps->ps_endy)
					{
						tendy = ps->ps_endy;
					}
					if (tendx < ps->ps_endx)
					{
						tendx = ps->ps_endx;
					}
				}
			}
		}
	}

	if (within_count)
	{
		/*
		**  If we found something, unlink them here.  Can't
		**  do it above since that will change the structures
		**  that we are trying to traverse.
		*/
		for (lt = comps_to_move; lt != NULL; lt = lt->lt_next)
		{
			vfdisplay(lt->lt_var.lt_vpos,
				lt->lt_var.lt_vpos->ps_name, TRUE);
			lt->lt_var.lt_vpos->ps_attr = IS_IN_GROUP_MOVE;
		/*
			unLinkPos(lt->lt_var.lt_vpos);
		*/
		}
		*sendy = tendy;
		*sendx = tendx;
	}

	return(within_count);
	
}


/*{
** Name:	IIVFgmrGroupMoveRelocate - Relocate selected components for
**					   a group move.
**
** Description:
**	This routine does the actual work of moving the selected components
**	for a group move command.
**
** Inputs:
**	oldy	Y coordinate of old anchor point.
**	oldx	X coordinate of old anchor point.
**	newy	Y coordinate of new anchor point.
**	newx	X coordinate of new anchor point.
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
**	08/25/92 (dkh) - Initial version.
*/
void
IIVFgmrGroupMoveRelocate(oldy, oldx, newy, newx)
i4	oldy;
i4	oldx;
i4	newy;
i4	newx;
{
	LIST	*lt;
	POS	*ps;
	i4	diffy;
	i4	diffx;
	i4	numrow;
	i4	numcol;
	i4	compy;
	i4	compx;
	SMLFLD	sfd;

	/*
	**  Change old anchor points so that when we calculate the
	**  offsets for a component, the components will be have
	**  a zero offset for the new anchor spot.
	oldy++;
	oldx++;
	*/

	/*
	**  Suspend updates to screen so that things all appear
	**  at once and there is no screen bounce if the updates
	**  are bigger than the terminal screen.
	*/
	IIVFsuSuspendUpdate((bool) TRUE);

	/*
	**  We must first unlink the selected objects so that we
	**  can move them with no problems later on.
	*/
	for (lt = comps_to_move; lt != NULL; lt = lt->lt_next)
	{
		ps = lt->lt_var.lt_vpos;
		ps->ps_attr = 0;
		unLinkPos(ps);
	}

	for (lt = comps_to_move; lt != NULL; lt = lt->lt_next)
	{
		ps = lt->lt_var.lt_vpos;

		/*
		**  Updated coordinate to affect the move.
		*/
		diffy = ps->ps_begy - oldy;
		diffx = ps->ps_begx - oldx;
	/*
		numrow = ps->ps_endy - ps->ps_begy;
		numcol = ps->ps_endx - ps->ps_begx;
	*/

		/*
		**  Delete image at old location.
		vfersPos(ps);
		*/

	/*
		ps->ps_begy = newy + diffy;
		ps->ps_begx = newx + diffx;
		ps->ps_endy = ps->ps_begy + numrow;
		ps->ps_endx = ps->ps_begx + numcol;
	*/
		compy = newy + diffy;
		compx = newx + diffx;

		switch (ps->ps_name)
		{
			case PTRIM:
				(void) placeTrim(ps, compy, compx, FALSE);
				break;

			case PBOX:
				(void) placeBox(ps, compy, compx, FALSE);
				break;

			case PFIELD:
				FDToSmlfd(&sfd, ps);
				(void)placeField(ps, &sfd, compy, compx, FALSE);
				break;

			case PTABLE:
				(void) placeTable(ps, compy, compx, FALSE);
				break;

			default:
				break;
		}

		/*
		**  Now put the component back.  No need
		**  to check for overlap since that should
		**  have been taken care of already by a
		**  previous call to IIVFgmcGroupMoveCollide()
		insPos(ps, FALSE);
		*/
	}

	/*
	**  Let screen updates occur again.
	*/
	IIVFsuSuspendUpdate((bool) FALSE);

	vfTrigUpd();
	vfUpdate(globy, globx, TRUE);
}


/*{
** Name:	IIVFgmpGroupMovePutback - Put back components that
**					  were selected for a group move.
**
** Description:
**	This routine puts back the components that were selected for
**	a group move.  They were unlinked by IIVFfcbFindBoxedComps()
**	and not have to be put back in case collisions occur or
**	the user did not select a new anchor.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Variable comps_to_move gets set to NULL.
**
** History:
**	08/24/92 (dkh) - Initial version.
*/
void
IIVFgmpGroupMovePutback()
{
	LIST	*lt;
	LIST	*old;

	for (lt = comps_to_move; lt != NULL; )
	{
	/*
		insPos(lt->lt_var.lt_vpos, FALSE);
	*/
		lt->lt_var.lt_vpos->ps_attr = 0;
		old = lt;
		lt = lt->lt_next;
		lfree(old);
	}
	comps_to_move = NULL;
}



/*{
** Name:	IIVFgmGroupMove - Move a group of components together.
**
** Description:
**	Allow user to define a bounding box to "lasso" a set of components
**	that can be moved together.  This only works in Vifred for now
**	due to columns in RBF that get in the way.
**
** Inputs:
**	None.
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
**	08/20/92 (dkh) - Initial version.
**	6-Jul-2009 (kschendel) SIR 122138
**	    SPARC solaris compiled 64-bit flunks undo test, fix by properly
**	    declaring exChange (for pointers) and exi4Change (for i4's).
*/
void
IIVFgmGroupMove()
{
	i4		startx;
	i4		starty;
	i4		endx;
	i4		endy;
	i4		newx;
	i4		newy;
	i4		sendy;
	i4		sendx;
	i4		poscount;
	i4		numcol;
	i4		numrow;
	FLD_POS		*posarray = NULL;
	FRS_EVCB	evcb;
	POS		tboxpos;
	i4		*np;
	i4		junk = 0;
	i4		diff;
	i4		within_count;
	i4		bail_out;
	i4		place_selected;
	i4		attr = 0;
	LIST		*lt;
	LIST		*old;
	MENU		placemu;
	char		buf[200];
	i4		menu_val;

	VTflashplus(frame, globy, globx, 1);

	scrmsg(ERget(F_VF0047_Pos_opp_corner));

	startx = globx;
	starty = globy;

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);
	evcb.event = fdNOCODE;

	FTclrfrsk();

	vfposarray(&poscount, &posarray);

	while (VTcursor(frame, &globy, &globx, endFrame, endxFrm + 1, poscount,
		posarray, &evcb, VFfeature, RBF, FALSE, FALSE, FALSE,
		TRUE) != fdopMENU)
	{
		;
	}

	/*
	**  If user did not move cursor, user could not have selected
	**  anything.  So we cancel the move.
	*/
	if (starty == globy && startx == globx)
	{
		VTflashplus(frame, 0, 0, 0);
	/*
		IIUGerr(E_VF016C_NO_COMPS, UG_ERR_ERROR, 0);
	*/
		return;
	}

	/*
	**  Now figure out if there are any objects that are completely
	**  surrounded by the box.  If so, then put up box so user
	**  can see what is selected and then allow user to position
	**  cursor to new anchor point.
	**
	**  If there are no components completely surrounded by the selection,
	**  let user know and exit out.
	*/

	endy = globy;
	endx = globx;

	/*
	**  Determine the true upper left corner for the bounding box.
	*/
	if (starty > endy)
	{
		exi4Change(&starty, &endy);
	}

	if (startx > endx)
	{
		exi4Change(&startx, &endx);
	}

	within_count = IIVFfcbFindBoxedComps(starty, startx, endy, endx,
		&sendy, &sendx);

	/*
	**  If we found components in the bounding box, go to next step.
	*/
	if (within_count)
	{
		/*
		**  Put flashing symbols at the 4 corners.
		**  Can't use VTflashplus() since it forces
		**  a redraw on each call and can cause screen
		**  to bounce around if box is bigger than screen.
		*/

		attr = (fdBLINK | fdRVVID);
		VTputstring(frame, starty, startx, ERx("+"), attr,
			(bool) FALSE, (bool) FALSE);
		VTputstring(frame, endy, endx, ERx("+"), attr,
			(bool) FALSE, (bool) FALSE);
		VTputstring(frame, starty, endx, ERx("+"), attr,
			(bool) FALSE, (bool) FALSE);
		VTputstring(frame, endy, startx, ERx("+"), attr,
			(bool) FALSE, (bool) FALSE);

		VTxydraw(frame, globy, globx);

		/*
		**  Allow user to set new anchor point.
		*/
		placemu = selbox_m;
		placemu->mu_flags |= MU_NEW;

		place_selected = FALSE;
		bail_out = FALSE;
		for (;;)
		{
			evcb.event = fdNOCODE;
			FTclrfrsk();
			FTaddfrs(1, HELP, NULL, 0, 0);
			FTaddfrs(3, UNDO, NULL, 0, 0);
			vfmu_put(placemu);


			(void) VTcursor(frame, &globy, &globx, endFrame,
				endxFrm + 1, poscount, posarray, &evcb,
				VFfeature, RBF, FALSE, FALSE, FALSE, FALSE);

			VTdumpset(vfdfile);
			if (evcb.event == fdopFRSK)
			{
				menu_val = evcb.intr_val;
			}
			else if (evcb.event == fdopMUITEM)
			{
				if (IImumatch(placemu, &evcb, &menu_val) != OK)
				{
					continue;
				}
			}
			else
			{
				menu_val = FTgetmenu(placemu, &evcb);
				if (evcb.event == fdopFRSK)
				{
					menu_val = evcb.intr_val;
				}
			}
			VTdumpset(NULL);
			switch(menu_val)
			{
			  case fdNOCODE:
			  default:
				continue;

			  case HELP:
				vfhelpsub(VFH_GRPMV, ERget(S_VF016E_GROUPMOVE),
					placemu);
				continue;

			  case UNDO:
				bail_out = TRUE;
				break;

			  case PLACE:
				if (starty == globy && startx == globx)
				{
					place_selected = TRUE;
				}
				else
				{
					if (IIVFgmcGroupMoveCollide(starty,
						starty, sendy, sendx, globy,
						globx))
					{
						IIUGerr(E_VF016D_GM_COLLISION,
							UG_ERR_ERROR, 0);
					}
					else
					{
						place_selected = TRUE;
					}
				}
				break;
			}
			if (bail_out || place_selected)
			{
				break;
			}
		}
		/*
		**  If new anchor point is same as old one, don't do
		**  anything.
		*/
		if (bail_out || (starty == globy && startx == globx))
		{

			IIVFgmpGroupMovePutback();
		}
		else
		{
			/*
			**  Check for overlaps, if none, then check to see
			**  if form has to expand, if so, then do it.  Since
			**  the form is expanding, no need to worry about
			**  cross hairs.
			**
			**  If overlap, just tell user and exit.
			**
			*/
			/*
			**  All we need to do now is to move the
			**  selected components and to set
			**  noChanges to FALSE to let us know that
			**  changes were made.
			**
			**  Need to determine each component's offset
			**  from starting point and then use that in
			**  calculating new positions.
			**
			**  We also need to determine if the new
			**  location will require the form to be
			**  expanded, if so, do that first.
			**
			**  Note that our calculations are based
			**  on the size of the bounding box being
			**  moved and not just the right most
			**  and bottom most positions occupied by
			**  the the selected components.  We can do
			**  the latter if we do more work.
			*/
			numcol = sendx - startx + 1;
			numrow = sendy - starty + 1;
			if ((diff = globx + numcol - endxFrm - 1) > 0)
			{
				vfwider(frame, diff);
			}
			if (globy + numrow >= endFrame)
			{
				vfMoveBottomDown(globy +
					numrow - endFrame);
			}

			IIVFgmrGroupMoveRelocate(starty, startx,
				globy, globx);

			noChanges = FALSE;
		}

		/*
		**  Note that there is no need to throw away the box since
		**  the screen image gets rebuilt by the controlling
		**  menu loop in cursor.qsc.
		*/
		setGlobUn(unNONE, (POS *) NULL, (i4 *) NULL);
	}
	else
	{
		IIUGerr(E_VF016C_NO_COMPS, UG_ERR_ERROR, 0);
	}
}
# endif /* FORRBF */

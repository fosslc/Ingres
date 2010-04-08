/*
** movedef.c
** code to do default movement
**
** Copyright (c) 2004 Ingres Corporation
**
** History
**	08/14/87 (dkh) - ER changes.
**	08-feb-88 (bruceb)
**		In newFrame(), check for NULL pointer before calling MEfree.
**	16-oct-89 (cmr)	 moveDown(): if frame is adjusted, call newLimits()
**			 after the adjustment not before.
**	24-jan-90 (cmr)	 Get rid of calls to newLimits(); superseded by
**			 one call to SectionsUpdate() by vfUpdate().
**	04/13/90 (dkh) - Fixed bug 8630.  Checking for sideways movement
**			 (due to a fitPos() call) will now ignore the position
**			 that was passed to fitPos() if it encounters it
**			 during column checking for RBF.
**			 This can happen in following setup:
**			  heading-a    heading-b    heading-c
**			               field-b      field-c     field-a
**			 If heading-a is edited and runs into headings to
**			 right, then code will look for heading-a when
**			 it checks field-a for sideways movement.  But
**			 heading-a was unlinked when it was edited which
**			 will cause RBF to die.  This change makes things
**			 work.
**	18-apr-90 (bruceb)
**		Lint cleanup; removed 'oldcode' arg from moveBack and
**		movePosBack.
**	4/20/90 (dkh) -  Put in fix to solve a variant of bug 8630.  Default
**			 movement will now handle the case where heading-b
**			 is edited and overlaps heading-c.  RBF will now
**			 move heading-c out of the way correctly instead
**			 of putting out an overlap error message and exiting.
**      06/09/90 (esd) - Check IIVFsfaSpaceForAttr instead of
**                       calling FTspace, so that whether or not
**                       to leave room for attribute bytes can be
**                       controlled on a form-by-form basis.
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**      06/12/90 (esd) - Modify inRegion and vfdistBe to allow
**                       two table fields or boxed regular fields
**                       to abut, even on 3270s.
**	07/12/90 (dkh) - Put in NULL pointer check for pos_tofit since
**			 moving a column needs to bypass what pos_tofit is
**			 used for.
**	10/31/90 (dkh) - Initialized struct in compOver() so that
**			 overlapped calculations don't use garbage values.
**	26-may-92 (rogerf) DeskTop Porting Change:
**			 Added scrollbar processing inside SCROLLBARS ifdef.
**	06/10/92 (dkh) - Changes for rulers support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jul-2009 (kschendel) SIR 122138
**	    SPARC solaris compiled 64-bit flunks undo test, fix by properly
**	    declaring exChange (for pointers) and exi4Change (for i4's).
**	aug-2009 (stephenb)
**		Prototyping front-ends
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"
# ifdef SCROLLBARS
# include	<wn.h>
# endif

/*
** movedef has a global state variable which corresponds to the
** level of the current move.
** A level is a unique id for a composite move, that is a move which
** is composed of several smaller moves.
** A move's level is used when column moves are being done.
** Each position in the column is moved with a different level number.
** All position's with mask > 0 have been moved, the level is used
** to determine if the position has been moved in this level only.
*/

static	POS	*pos_tofit = NULL;  /* Position being fitted by fitPos() */

static i4	moveLevel = 1;
i4	vfshftval();
i4	vfdistBe();

static bool chkSide(VFNODE *nd, i4 x, bool doColumn, i4 shift);

/*
** set level sets the move level
*/
VOID
nextMoveLevel()
{
	moveLevel++;
}

VOID
resetMoveLevel()
{
	moveLevel = 1;
}

i4
getMoveLevel()
{
	return moveLevel;
}

/*
** Default movements occur when a feature is edited and changes
** size or position (e.g. A move or edit).
** Vifred first tries to move positions which overlap sideways,
** and then moves then down.
**
** Sideways movement in vifred is broken into two classes,
** Shift move and Close fit moves.
**
** A shift move causes
** all overlapping positions to be shifted to the right.  Any
** positions to the right of the overlapping are moved an equal
** distance, maintaining their horizontal positioning.
**
** Close fit moves shift overlapping positions only as far right
** as necessary to clear them of the overlap.
** Any positions in the way are only moved as far right as possible.
*/

# define	MSHIFT	1
# define	CLOSE	2

static i4	Sidemode = CLOSE;	/* default mode */

/*
** changes the sideways mode to shift
*/
VOID
vfshiftmode()
{
	Sidemode = MSHIFT;
}

VOID
vfclosemode()
{
	Sidemode = CLOSE;
}

/*
** fit the passed position on the frame
** allocate bigger frame if necessary
*/
VOID
fitPos(ps, noUndo)
register POS	*ps;
bool		noUndo;
{
	i4		num;
	POS		over;

	pos_tofit = ps;

	vfTrigUpd();
	if (ps->ps_name != PBOX && overLap(ps) != NULL)
	{
		compOver(ps, &over);


		if (sideWays(ps, noUndo) == FALSE)
			moveDown(ps, &over, noUndo);

		pos_tofit = NULL;
		return;
	}


	if (noUndo)
		setGlMove(ps);
	/*
	** Change test to be "<=" rather than "<".
	** This will prevent newFrame from being called with 0.
	** frame->frscr will get erased during an undo of an undo.
	** Undo of an undo does not display screen
	** correctly if you have just entered a new
	** field which pushes other fields down.
	** BUG 1587 (dkh)
	*/
	if (ps->ps_endy <= endFrame)
	{
		pos_tofit = NULL;
		return;
	}
	/*
	** at this point, ps doesn't overlap but is off the
	** frame. Allocate a new frame and move everything
	*/
	num = ps->ps_endy - (endFrame);
	vfnewLine(num);
	newFrame(num);

	pos_tofit = NULL;
}

/*
** movenum is a static which counts traversals.
*/
static i4	movenum = 0;

/*
** move the position using the default processing
** try sideways first, and then move down.
*/

bool
sideWays(ps, noUndo)
POS	*ps;
bool	noUndo;
{
	VFNODE	*nd;
	i4	num;
	i4	shift;		/* only used for shift */

	nd = vfgrfbuild(ps);
	/*
	** Test to see if sideways move is possible before
	** effecting state of the move
	*/
	if (Sidemode == MSHIFT)
	{
		shift = vfshftval(nd, ps);
	}
	if (testSide(nd, ps, shift) == FALSE)
	{
		return (FALSE);
	}

	moveSide(nd, ps, shift);
	num = ps->ps_endy - endFrame;
	if (num < 0)
		num = 0;
	if (noUndo)
		setGlMove(ps);
	vfnewLine(num);
	newFrame(num);
	return (TRUE);
}

i4
vfshftval(nd, ps)
VFNODE	*nd;
POS	*ps;
{
	i4	minx;
	LIST	*adj;
	VFNODE	*overnd;

	minx = endxFrm + 1;
	for (adj = nd->nd_adj; adj != NULL; adj = adj->lt_next)
	{
		if (adj->lt_node == NULL)
			continue;
		overnd = adj->lt_node;
		if (overnd->nd_pos->ps_begx < minx)
			minx = overnd->nd_pos->ps_begx;
	}
	return (ps->ps_endx - minx + 2);
}

/*
** travlevel is the level marker used in the node nd_mask
** field to control traversal. It is only used with the
** shift operation.
**
** travlevel is constantly incremented without ever being
** reset.
*/
static i2	travlevel = 0;

bool
testSide(nd, ps, shift)
VFNODE	*nd;
POS	*ps;
i4	shift;		/* only used for shifts */
{
	LIST	*adj;
	VFNODE	*overnd;
	POS	*overps;
	i4	y;

	/*
	**  Code not changed under the assumption that attribute
	**  bytes only take up one space on the display.  Code
	**  now allows for this so no change neccessary for now. (dkh)
	*/

	movenum = 0;
	travlevel++;
	for (adj = nd->nd_adj, y = ps->ps_begy; adj != NULL;
				y++, adj = adj->lt_next)
	{
		for (overnd = adj->lt_node; ; overnd = vflnNext(overnd, y))
		{
			if (overnd == NULL)
				break;

			overps = overnd->nd_pos;

			/* step through all boxes on this line */
			if (overps->ps_name != PBOX)
				break;
		}
		if (overnd == NULL)
			continue;

		if (  Sidemode == CLOSE
		   && ps->ps_endx + 2 < overps->ps_begx
		   )
		{
			continue;
		}
		if (! chkSide(overnd, ps->ps_endx+2, TRUE, shift) )
			return (FALSE);
	}
	return (TRUE);
}


/*
** check to see if the position at this node can be moved
** to the location given
*/
static bool
chkSide(VFNODE *nd, i4 x, bool doColumn, i4 shift)
{
	i4	howfar;
	POS	*ps;
	LIST	*adj;
	VFNODE	*overnd;
	POS	*overps;
	i4	y;
	/*
	**  Code not changed under the assumption that attribute
	**  bytes only take up one space on the display.  Code
	**  now allows for this so no change neccessary for now. (dkh)
	*/


	if (Sidemode == MSHIFT && nd->nd_mask == travlevel)
		return (TRUE);
	movenum++;
	nd->nd_mask = travlevel;

	ps = nd->nd_pos;

	howfar = Sidemode == MSHIFT ? shift : x - ps->ps_begx;

	if (ps->ps_endx + howfar >= endxFrm + 1)
		return (FALSE);

	for (adj = nd->nd_adj, y = ps->ps_begy; adj != NULL;
					y++, adj = adj->lt_next)
	{
		for (overnd = adj->lt_node; ; overnd = vflnNext(overnd, y))
		{
			if (overnd == NULL)
				break;

			overps = overnd->nd_pos;

			/* step through all boxes on this line */
			if (overps->ps_name != PBOX)
				break;
		}
		if (overnd == NULL)
			continue;

		if (  (  (   Sidemode == CLOSE
		      	 && howfar > vfdistBe(overnd, ps)
		      	 )
		      || Sidemode == MSHIFT
		      )
		      && ! chkSide(overnd, ps->ps_endx + howfar + 2,
				TRUE, shift)
		   )
		{
			return (FALSE);
		}
	}
	if (doColumn && !testCol(ps, howfar))
		return (FALSE);
	return (TRUE);
}

/*
** see if a column can move
*/
bool
testCol(ps, howfar)
POS	*ps;
i4	howfar;
{
	VFNODE	*nd;
	POS	*list;

	if (ps->ps_column == NULL)
		return (TRUE);
	list = ps->ps_column;
	while (list != ps)
	{
		/*
		** This test must guard againest columns which are on
		** the same line.  If a column is on the same line, then
		** we don't want to move this part of the column if
		** it comes before the present column marker.  (ps)
		**
		** The situation is this:
		**
		**		[1st column]	 [2nd column]
		**
		**  if ps is 2nd piece then we don't want to move
		**  1st piece because an infinite loop could result.
		**  vfVertReg checks for vertical overlap.
		**  if rbf is in block mode then we don't want
		**  to call ndonPos since the 1st piece was unlinked
		**  and ndonPos will not find it.  Fix for bug #865.
		*/
		if (vfVertReg(ps, list))
		{
			return (TRUE);	/* was FALSE (dkh) */
		}

		/*
		**  If the position is the same as the one being
		**  fitted, then we just skip it since we will
		**  not move the position that we are trying to
		**  fit in.
		*/
		if (list == pos_tofit)
		{
			list = list->ps_column;
			continue;
		}

		nd = ndonPos(list);
		if (! chkSide(nd, list->ps_begx + howfar, FALSE,howfar) )
			return (FALSE);

		/*
		**  If one of the linked components is to the left
		**  of the component being fitted, then just give
		**  up and return FALSE.  This prevents overlap
		**  errors when insPos() is executted.
		**
		**  Note that we don't need to use "<=" since two
		**  components can't start at the same location.
		*/
		if (pos_tofit != NULL && list->ps_begy == pos_tofit->ps_begy &&
			list->ps_begx < pos_tofit->ps_begx)
		{
			return(FALSE);
		}
		list = list->ps_column;
	}
	return (TRUE);
}

/*
** move all position sideways from here
*/
VOID
moveSide(nd, ps, shift)
VFNODE	*nd;
POS	*ps;
i4	shift;		/* only used for shift */
{
	LIST	*adj;
	VFNODE	*overnd;
	i4	y;

	/*
	**  Code not changed under the assumption that attribute
	**  bytes only take up one space on the display.  Code
	**  now allows for this so no change neccessary for now. (dkh)
	*/

	movenum = 0;
	travlevel++;

	y = ps->ps_begy;

	for (adj = nd->nd_adj; adj != NULL; adj = adj->lt_next, y++)
	{
		for (overnd = adj->lt_node; ; overnd = vflnNext(overnd, y))
		{
			/* skip through all boxes on this line */
			if (overnd == NULL || overnd->nd_pos->ps_name != PBOX)
				break;
		}

		if (overnd == NULL)
			continue;

		if (  Sidemode == CLOSE
		   && ps->ps_endx + 2 < overnd->nd_pos->ps_begx
		   )
		{
			continue;
		}
		mvSide(overnd, ps->ps_endx+2, TRUE, shift);
	}
}

/*
** recursive function to move nodes to the right on the same line:
**     - find out how much this node must move
**     - recurse to move following nodes over so as to clear this node
**	 (slightly different for boxes because they do not
**	 have to be moved because overlaps are ok)
**     - actually move this node
*/
VOID
mvSide(nd, x, doColumns, shift)
VFNODE	*nd;
i4	x;
bool	doColumns;
i4	shift;
{
	i4	howfar;
	VFNODE	*overnd;
	POS	*ps;
	LIST	*adj;
	i4	y;

	/*
	**  Code not changed under the assumption that attribute
	**  bytes only take up one space on the display.  Code
	**  now allows for this so no change neccessary for now. (dkh)
	*/

	if (  Sidemode == MSHIFT
	   && nd->nd_mask == travlevel
	   )
	{
		return;
	}

	movenum++;
	nd->nd_mask = travlevel;
	ps = nd->nd_pos;

	howfar = Sidemode == MSHIFT
			? shift
			: x - ps->ps_begx;

	y = ps->ps_begy;

	for (adj = nd->nd_adj; adj != NULL; adj = adj->lt_next, y++)
	{
		for (overnd = adj->lt_node; ; overnd = vflnNext(overnd, y))
		{
			/* skip through all boxes on this line */
			if (overnd == NULL || overnd->nd_pos->ps_name != PBOX)
				break;
		}

		if (overnd == NULL)
			continue;

		if (  Sidemode == CLOSE
		   && howfar <= vfdistBe(overnd, ps)
		   )
			continue;

		mvSide(overnd, ps->ps_endx + howfar + 2, TRUE, shift);
	}

	if (doColumns)
		mvCol(ps, howfar);

	if (ps->ps_mask == PNORMAL)
		savePos(ps);

	ps->ps_begx += howfar;
	ps->ps_endx += howfar;
	ps->ps_mask = getMoveLevel();
}

/*
** see if a column can move
*/
VOID
mvCol(ps, howfar)
POS	*ps;
i4	howfar;
{
	VFNODE	*nd;
	POS	*list;

	if (ps->ps_column == NULL)
		return;
	list = ps->ps_column;
	while (list != ps)
	{
		/*
		** This test must guard againest columns which are on
		** the same line.  If a column is on the same line, then
		** we don't want to move this part of the column if
		** it comes before the present column marker.  (ps)
		**
		** The situation is this:
		**
		**		[1st column]	 [2nd column]
		**
		**  if ps is 2nd piece then we don't want to move
		**  1st piece because an infinite loop could result.
		**  Call to ndonPos moved down here for two reasons.
		**  One, to make code look like what's in function
		**  "testCol" (after fix for bug #865) and two
		**  to eliminate unnecessary calls
		**  to ndonPos.
		**
		**  We now also don't move if the position is
		**  the same as the position being fitted by fitPos().
		*/
		if (vfVertReg(ps, list) == FALSE && list != pos_tofit)
		{
			nd = ndonPos(list);
			mvSide(nd, list->ps_begx + howfar, FALSE, howfar);
		}
		list = list->ps_column;
	}
}

/*
** move those features which over lap down
*/
VOID
moveDown(ps, over, noUndo)
POS	*ps;
POS	*over;
bool	noUndo;
{
	register i4	i;
	i4		num;
	i4		oldend;
	POS		*eps;
	register VFNODE **nlp;		/* pointer to new line table */

	oldend = endFrame;
	movenum = 0;
	/*
	** add an extra space for a blank line
	*/
	num = ps->ps_endy - over->ps_begy + 2;
	if (noUndo)
		setGlMove(ps);
	vfnewLine(num);
	nlp = &(line[endFrame]);
	for (i = endFrame; i > over->ps_endy; i--, nlp--)
		adjPosVert(*nlp, num, i);
	for (i = over->ps_endy; i >= over->ps_begy; i--, nlp--)
		adjOverPos(*nlp, num, i, over);

	/* call a variant of newFrame which does not check if
	   we just expanded form beyond what would be allowed as a popup,
	   the code below will attempt to move the bottom up to it's old
	   position if it can.. so the test must be done latter */
	vf_newFrame(num);

	/* since we may have added lines needlessly (in the case where
	   the features at the bottom of the form have some space
	   beneath them) */
	while (  endFrame != oldend
	      && endFrame > ps->ps_endy + 1
	      && line[endFrame - 1] == NNIL
	      )
	{
		eps = line[endFrame]->nd_pos;
		eps->ps_begy -= 1;
		eps->ps_endy -= 1;
		line[endFrame - 1] = line[endFrame];
		line[endFrame] = NNIL;
		endFrame--;
	}
	/* now test if the form can still work as a popup */
	vfChkPopup();
}

/*
** calculate the distance between a position, and the position
** a node points to.
*/

i4
vfdistBe(nd, ps)
VFNODE	*nd;
POS	*ps;
{
	POS	*nps;

	nps = nd->nd_pos;
	return (nps->ps_begx - ps->ps_endx - 1);
}

static i4	nextExtent = 2;
/*
** allocate a new line table
*/
VOID
vfnewLine(num)
i4	num;
{
	reg	VFNODE	**lp,
			**nlp;
	VFNODE	**rval;
	i4	size;
	i4	i;

	if (endFrame + num < frame->frmaxy)
		return;
	size = frame->frmaxy + num + nextExtent;
	if ((rval = (VFNODE **)FEreqmem((u_i4)0,
	    (u_i4)(size * sizeof(VFNODE *)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vfnewLine"));
	}
	for (lp = line, nlp = rval, i = 0; i <= endFrame; i++, lp++, nlp++)
		*nlp = *lp;
	line = rval;
}

/*
** have to allocate a new frame with num more lines
*/
VOID
newFrame(num)
i4	num;
{
	vf_newFrame(num);
	vfChkPopup();
}

VOID
vf_newFrame(num)
i4	num;
{
	i4		size;
	extern	bool	vfinundo;

	if (endFrame + num >= frame->frmaxy)
	{
		size = num + nextExtent;
		VTextend(frame, size);
		/* nextExtent += 2; */
		frame->frmaxy += size;
	}

	endFrame += num;
	vfTrigUpd();

#ifdef SCROLLBARS
	WNFbarCreate(FALSE, 	   /* not on "real" form */
		-1,                /* current row */
                frame->frmaxy,     /* max form rows */
                -1,                /* current column */
                frame->frmaxx);    /* max form columns */
#endif /* SCROLLBARS */

}

void
adjPosVert(lt, num, i)
register VFNODE *lt;
i4		num,
		i;
{
	POS	*ps;

	while (lt != NULL)
	{
		ps = lt->nd_pos;
		lt = vflnNext(lt, i);

		if (	ps->ps_name == PBOX
		   ||	ps->ps_mask == getMoveLevel()
		   ||	ps->ps_begy != i
		   )
			continue;

		movenum++;
		unLinkPos(ps);
		if (ps->ps_mask == PNORMAL)
			savePos(ps);
		ps->ps_begy += num;
		ps->ps_endy += num;
		ps->ps_mask = getMoveLevel();
		insPos(ps, FALSE);
	}
}

void
adjOverPos(lt, num, i, over)
register VFNODE *lt;
i4		num,
		i;
POS		*over;
{
	POS	*lps;

	while (lt != NNIL)
	{
		lps = lt->nd_pos;
		lt = vflnNext(lt, i);

		if (    lps->ps_name == PBOX
		   ||	lps->ps_mask == getMoveLevel()
		   ||	lps->ps_begy != i
		   ||	!inRegion(lps, over)
		   )
			continue;

		movenum++;
		unLinkPos(lps);
		if (lps->ps_mask == PNORMAL)
			savePos(lps);
		lps->ps_begy += num;
		lps->ps_endy += num;
		lps->ps_mask = getMoveLevel();
		insPos(lps, FALSE);
	}
}

/*
** adjust the lists of positions by adding num to their y's
*/
void
adjustPos(lt, num, i)
VFNODE	*lt;
i4	num;
i4	i;
{
	reg	POS	*ps;

	for (; lt != NNIL; lt = vflnNext(lt, i))
	{
		ps = lt->nd_pos;

		if ( ps->ps_mask == getMoveLevel())
			continue;

		if (ps->ps_mask == PNORMAL)
			savePos(ps);
		ps->ps_begy += num;
		ps->ps_endy += num;
		ps->ps_mask = getMoveLevel();
	}
}

/*
** move the line table back to a consistent state
** code is either -1 or 1.
** They are used to set the position mask of those position which have
** changed.
*/
VOID
moveBack(code, beg)
i4	code;
i4	beg;
{
	register VFNODE **lp;
	register i4	i;
	extern	i4	vfoldundo;

	lp = &line[beg];
	for (i = beg; i <= endFrame; i++, lp++)
	{
		movePosBack(*lp, code, i);
	}
	/*
	** Code used to display up to endFrame.
	** But information about fields not being updated if the above
	** calls to movePosBack move fields below endFrame.
	** This occurs if we create a form that forces other fields
	** down, then we an undo.  Another undo at this point will
	** cause field information to be lost through the undo algorithm.
	** This fixes bug 1588 (dkh).
	*/
	vfDispAll(vfoldundo);
}

VOID
movePosBack(lt, code, i)
register VFNODE *lt;
i4		code;
i4		i;
{
	reg	POS	*ps;

	/*
	** go through list unlinking those that need
	** to be moved
	** have to be careful when we increment since
	** unlink will change the list structure
	*/
	while (lt != NULL)
	{
		ps = lt->nd_pos;
		lt = vflnNext(lt, i);
		/*
		** has already been moved, or was never moved
		*/
		if (ps->ps_mask == code || ps->ps_mask == PNORMAL)
			continue;
		unLinkPos(ps);
		ps->ps_endy = ps->ps_oldy + (ps->ps_endy - ps->ps_begy);
		ps->ps_endx = ps->ps_oldx + (ps->ps_endx - ps->ps_begx);
		exi4Change(&ps->ps_oldx, &ps->ps_begx);
		exi4Change(&ps->ps_oldy, &ps->ps_begy);
		ps->ps_mask = code;
		insPos(ps, FALSE);
	}
}

void
compOver(POS *ps, POS *over)
{
	VFNODE		**lp;
	register POS	*lps;
	register POS	*cps;
	register VFNODE *lt;
	i4		i;
	i4		starty;
	i4		endy;
	i4		saveby = 0;
	i4		savebx = 0;
	i4		saveey = 0;
	i4		saveex = 0;

	MEfill((u_i2) sizeof(POS), (u_char) '\0', (PTR) over);
	over->ps_begy = ps->ps_endy;
	over->ps_endy = min(ps->ps_endy, endFrame);
	over->ps_begx = ps->ps_endx;
	over->ps_endx = ps->ps_endx;
	starty = ps->ps_begy;
	endy = ps->ps_endy;
	cps = ps;
	for (;;)
	{
		/*
		**  Code changed to keep searching until there
		**  are no changes.  This takes into account
		**  the fact that we are creating a rectangle
		**  were components overlap, but does not
		**  account for the growing rectangle may
		**  grow to encompass a component on a line
		**  above (or below) where we are looking.
		**  Fix for BUG 5330. (dkh)
		*/
		for (i = starty, lp = &line[i]; i <= endy && i <= endFrame;
			i++, lp++)
		{
			for (lt = *lp; lt != NNIL; lt = vflnNext(lt, i))
			{
				lps = lt->nd_pos;

				if (lps->ps_name == PBOX)
					continue;

				if (inRegion(lps, cps))
				{
					over->ps_endy =
						max(lps->ps_endy,over->ps_endy);
					over->ps_begy =
						min(lps->ps_begy,over->ps_begy);
					over->ps_begx =
						min(lps->ps_begx,over->ps_begx);
					over->ps_endx =
						max(lps->ps_endx,over->ps_endx);
				}
			}
		}
		if (saveby != over->ps_begy || savebx != over->ps_begx ||
			saveey != over->ps_endy || saveex != over->ps_endx)
		{
			starty = over->ps_begy;
			endy = over->ps_endy;
			saveby = over->ps_begy;
			savebx = over->ps_begx;
			saveey = over->ps_endy;
			saveex = over->ps_endx;
			cps = over;
		}
		else
		{
			break;
		}
	}
}

bool
inRegion(ps, over)
register POS	*ps;
register POS	*over;
{
	return !((ps->ps_endx < over->ps_begx) ||
		(over->ps_endx < ps->ps_begx));
}

/*
** determine if two positions overlap vertically.
*/
bool
vfVertReg(ps, over)
register POS	*ps,
		*over;
{
	return (vertin(ps->ps_begy, over) || vertin(over->ps_begy, ps));
}

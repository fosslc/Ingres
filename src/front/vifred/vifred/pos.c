/*
** pos.c
** contains all routines which deal with position
** structures
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	12/21/84 DRH - Allocate line table with FEcalloc.  Allocate a pos
**		       in palloc() using the line table tag.
**	01/16/85 DRH - Allocate in palloc() using FEtcalloc.
**	1-25-85 (peterk) - separated routines dealing with building
**		line table into another file lines.c.
**	2/6/85 (peterk) - removed commented-out lines
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**      06/12/90 (esd) - Test for overlapping features by calling
**                       inRegion instead of invoking the 'within' macro
**                       (inRegion has been modified to allow two
**                       table fields or boxed regular fields to abut,
**                       even on 3270s).
**      06/12/90 (esd) - Remove code that sets spacesize from FTspace,
**                       since the 'within' macro no longer uses it.
**                       Note that the change to the 'within' macro
**                       fixes a bug (in onPos) that caused a feature
**                       to be selected on a 3270 when the cursor was
**                       positioned just to the right of the feature,
**                       even when there was an edge of a box directly
**                       under the cursor.
**      06/12/90 (esd) - Support new parm 'attr_reqd' to palloc.
**	06/10/92 (dkh) - Changes for rulers support.
**	04/16/98 (tlk) - Renamed unlink to pos_unlink as it conflicted 
**			 with a DEC C function by the same name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<si.h>
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"

/*
** monkey around with positions
*/

/*
** return the feature which is on the position
** passed
*/
POS *
onPos(y, x)
i4	y,
	x;
{
	register VFNODE *nd;
	register POS	*ps;

	/* loop through the nodes on the line looking first for
	   any non-boxes. */

	for (nd = line[y]; nd != NULL; nd = vflnNext(nd, y))
	{
		ps = nd->nd_pos;
		if (ps->ps_name != PBOX && within(x, ps))
			return (ps);
	}

	/* next loop again, this time considering only boxes */

	for (nd = line[y]; nd != NULL; nd = vflnNext(nd, y))
	{
		ps = nd->nd_pos;
		if (ps->ps_name == PBOX && onEdge(y, x, ps) == TRUE)
			return (ps);
	}
	return (NULL);
}

/*{
** Name:	onEdge	- 	is feature on edge of given box
**
** Description:
**	Check if an x,y position is on the edge of a box 
**	feature.. 
**
** Inputs:
**	i4  y,x;	- position to test
**	POS *ps;	- box structure to test against
**
** Outputs:
**
**	Returns:
**	i4 		TRUE - if it lies on the edge
**	 		FALSE - if it doesn't
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04/05/88  (tom) - created
*/
i4
onEdge(y, x, ps)
reg	i4	y;
reg	i4	x;
reg	POS	*ps;
{

	/* if point is on either of the sides .. */
	if (x == ps->ps_begx || x == ps->ps_endx)
		if (y >= ps->ps_begy &&	y <= ps->ps_endy)
			return TRUE;

	/* else if point is on top or bottom line .. */
	if (y == ps->ps_begy || y == ps->ps_endy)
		if (x >= ps->ps_begx &&	x <= ps->ps_endx)
			return TRUE;

	return FALSE;
}



/*
** return the node of the feature which is on the position
** passed
*/
VFNODE *
ndonPos(ps)
POS	*ps;
{
	i4	y,
		x;
	register VFNODE *nd;

	y = ps->ps_begy;
	x = ps->ps_begx;
	for (nd = line[y]; nd != NULL; nd = vflnNext(nd, y))
		if (within(x, nd->nd_pos))
			return nd;
	syserr(ERget(S_VF0083_ndonPos__can_t_find_n), ps);
	/*NOTREACHED*/
}

/*
** return the pos of the feature(if any) which overlaps
** the passed feature
*/
POS *
overLap(ps)
POS	*ps;
{
	i4	i;
	POS	*ht;

	for (i = ps->ps_begy; i <= ps->ps_endy && i <= endFrame; i++)
		if ((ht = horOverLap(i, ps)) != NULL)
			return (ht);
	return (NULL);
}

/*
** return the feature if there is a horizontal overlap
*/
POS *
horOverLap(y, ps)
i4		y;
register POS	*ps;
{
	reg	VFNODE	*nd;

	if (y > endFrame)
		return (NULL);
	for (nd = line[y]; nd != NULL; nd = vflnNext(nd, y))
	{
		if (nd->nd_pos->ps_name == PBOX)
			continue;
		if (nd->nd_pos->ps_feat != ps->ps_feat &&
		    inRegion(ps, nd->nd_pos))
		{
			return (nd->nd_pos);
		}
	}
	return (NULL);
}

/*
** unlink the passed position from all its entries in the line table
*/
VOID
unLinkPos(ps)
register POS	*ps;
{
	register i4	i;
	register VFNODE **lt;
	VFNODE		*node;
	VFNODE		*pos_unLink();

	for (i = ps->ps_begy, lt = &line[i]; i <= ps->ps_endy; i++, lt++)
		node = pos_unLink(lt, ps, i);
	ndfree(node);
}

/*
** unLink the passed position from the line table
*/
VFNODE *
pos_unLink(nd, ps, i)
VFNODE	**nd;
POS	*ps;
i4	i;
{
	register VFNODE *node;

	node = *nd;
	for (; *nd != NULL && (*nd)->nd_pos != ps; nd = vflnAdrNext(*nd, i))
		;
	if (*nd == NULL)
		syserr(ERget(S_VF0084_unLink__can_t_find_po), ps, node);
	node = *nd;
	*nd = vflnNext(node, i);
	return (node);
}

/*
** allocate a position structure
*/
POS *
palloc(tag, y, x, ylen, xlen, feat, attr_reqd)
i4	tag,
	y,
	x,
	ylen,
	xlen;
i4	*feat;
bool	attr_reqd;
{
	POS	*ps;

	if ((ps = (POS *)FEreqmem((u_i4)vflinetag, (u_i4)sizeof(*ps), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("palloc"));
	}
	ps->ps_begx = x;
	ps->ps_begy = y;
	ps->ps_endx = x + xlen - 1;
	ps->ps_endy = y + ylen - 1;
	ps->ps_feat = feat;
	ps->ps_name = tag;
	ps->ps_attr = 0;
	return (ps);
}

VOID
savePos(ps)
reg POS *ps;
{
	ps->ps_oldx = ps->ps_begx;
	ps->ps_oldy = ps->ps_begy;
	ps->ps_mask = PMOVE;
}
/* 
**  erase the feature pointed to by POS by painting spaces on the screen
*/
VOID
vfersPos(ps)
register POS	*ps;
{
	register i4	i;
	reg	i4	len;

	len = ps->ps_endx - ps->ps_begx + 1;
	for (i = ps->ps_begy; i <= ps->ps_endy; i++)
	{
		VTdelstring(frame, i, ps->ps_begx, len);
	}
	vfTrigUpd();
}

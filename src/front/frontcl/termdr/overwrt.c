# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>

# define	minval(a,b)	((a) < (b) ? (a) : (b))

#ifdef i64_win
#pragma function(memcpy)	/* Turn off usage of memcpy() intrinsic. */
#endif

/*
**  This routine writes win1 on win2 destructively.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	05/04/88 (dkh) - Added TDoverlay() for Venus.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-feb-2004 (somsa01)
**	    Added turing off the memcpy() intrinsic for i64_win to prevent
**	    ABF from SEGV'ing in test fgl92.sep.
*/


TDoverwrite(win1, win2)
WINDOW	*win1;
WINDOW	*win2;
{
	reg	char	**lines1;
	reg	char	**lines2;
	reg	char	**da1;
	reg	char	**da2;
	reg	char	**dx1;
	reg	char	**dx2;
	reg	i4	minx;
	reg	i4	miny;


	if (win1->_maxy < win2->_maxy)
	{
		miny = win1->_maxy;
	}
	else
	{
		miny = win2->_maxy;
	}

	if (win1->_maxx < win2->_maxx)
	{
		minx = win1->_maxx;
	}
	else
	{
		minx = win2->_maxx;
	}



	lines1 = win1->_y;
	lines2 = win2->_y;
	da1 = win1->_da;
	da2 = win2->_da;
	dx1 = win1->_dx;
	dx2 = win2->_dx;
	for (; --miny >= 0;)
	{
		MEcopy(*lines1++, (u_i2)minx, *lines2++);
		MEcopy(*da1++, (u_i2)minx, *da2++);
		MEcopy(*dx1++, (u_i2)minx, *dx2++);
	}
}



/*{
** Name:	TDoverlay - Overlay a window on top of another one.
**
** Description:
**	This routine is used to overlay a source window (or parts
**	of the source window) into a destination window at a
**	specific starting location.
**
** Inputs:
**	srcwin	Source window.
**	srcsy	Y coordinate to copy from in source window.
**	srcsx	X coordinate to copy form in source window.
**	dstwin	Destinationn window.
**	dsty	Y coordinate to copy into in destination window.
**	dstx	X coordinate to copy into in destination window.
**
** Outputs:
**	None.
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
**	05/04/88 (dkh) - Initial version.
*/
VOID
TDoverlay(srcwin, srcsy, srcsx, dstwin, dstsy, dstsx)
WINDOW	*srcwin;
i4	srcsy;
i4	srcsx;
WINDOW	*dstwin;
i4	dstsy;
i4	dstsx;
{
	i4	miny;
	i4	minx;
	i4	i;
reg	char	*srcln;
reg	char	*dstln;
reg	char	*srcda;
reg	char	*dstda;
reg	char	*srcdx;
reg	char	*dstdx;

	if (srcwin->_maxx > dstwin->_maxx)
	{
		minx = dstwin->_maxx;
	}
	else
	{
		minx = srcwin->_maxx;
	}

	if (srcwin->_maxy > dstwin->_maxy)
	{
		miny = dstwin->_maxy;
	}
	else
	{
		miny = srcwin->_maxy;
	}

	/*
	**  Do line by line copy of the three planes of info.
	*/
	for (i = 0; i < miny; i++)
	{
		srcln = &(srcwin->_y[srcsy][srcsx]);
		dstln = &(dstwin->_y[dstsy][dstsx]);
		srcda = &(srcwin->_da[srcsy][srcsx]);
		dstda = &(dstwin->_da[dstsy][dstsx]);
		srcdx = &(srcwin->_dx[srcsy++][srcsx]);
		dstdx = &(dstwin->_dx[dstsy++][dstsx]);

		MEcopy(srcln, minx, dstln);
		MEcopy(srcda, minx, dstda);
		MEcopy(srcdx, minx, dstdx);
	}
}

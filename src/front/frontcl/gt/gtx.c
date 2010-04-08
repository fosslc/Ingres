/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <graf.h>

extern GR_FRAME *G_frame;

/*
**	conversion routines for floating point - char position, and
**	drawing coordinates
*/

/*
**	translate to drawing.
*/
GTxtodrw (x,y,tx,ty)
float x,y;	/* normalized coordinates */
float *tx,*ty;	/* drawing coordinates */
{
	*tx = G_frame->xm * x + G_frame->xb;
	*ty = G_frame->ym * y + G_frame->yb;
}

/*
**	translate from drawing.
*/
GTxfrdrw (x,y,tx,ty)
float x,y;	/* drawing coordinates */
float *tx,*ty;	/* normalized coordinates */
{
	*tx =(x - G_frame->xb) / G_frame->xm;
	*ty =(y - G_frame->yb) / G_frame->ym;
}

/*
**	translate an extent to drawing
*/
GTxetodrw (exfr,exto)
EXTENT *exfr,*exto;
{
	GTxtodrw (exfr->xlow,exfr->ylow,&(exto->xlow),&(exto->ylow));
	GTxtodrw (exfr->xhi,exfr->yhi,&(exto->xhi),&(exto->yhi));
}

/*
**	alpha cursor to normalized floating point
**	returns center of cursor.
*/
GTxatof (row,col,x,y)
i4  row,col;
float *x,*y;
{
	*x = col - G_frame->lcol;
	*x *= G_frame->csx;
	*x += .5 * G_frame->csx;

	/* rows go from top down, graphics coordinates bottom up */
	*y = row - G_frame->lrow;
	*y *= G_frame->csy;
	*y += .5 * G_frame->csy;
	*y = 1.0 - *y;
}

/*
**	normalized floating point to alpha cursor.  Returns cursor
**	position which contains point.  The "if" conditions take care
**	of coordinates sufficiently close to edges for round off error
**	to push them out of the window.
*/
GTxftoa (x,y,row,col)
float x,y;
i4  *row,*col;
{
	x /= G_frame->csx;
	*col = x;
	*col += G_frame->lcol;
	if (*col > G_frame->hcol)
		*col = G_frame->hcol;

	/* rows go from top down, graphics coordinates bottom up */
	y = 1.0 - y;
	y /= G_frame->csy;
	*row = y;
	*row += G_frame->lrow;
	if (*row > G_frame->hrow)
		*row = G_frame->hrow;
}

/*
**	0.0 - 1.0 on screen to 0.0 - 1.0 within extent.
**	and vice-versa
*/
GTxtoext (ext,x,y,xt,yt)
EXTENT *ext;
float x,y;
float *xt,*yt;
{
	float delx,dely;

	dely = ext->yhi - ext->ylow;
	delx = ext->xhi - ext->xlow;

	if (dely < 0.00001)
		dely = 0.00001;
	if (delx < 0.00001)
		delx = 0.00001;

	*yt = (y - ext->ylow)/dely;
	*xt = (x - ext->xlow)/delx;
}

GTxfrext (ext,x,y,xt,yt)
EXTENT *ext;
float x,y;
float *xt,*yt;
{
	float delx,dely;

	dely = ext->yhi - ext->ylow;
	delx = ext->xhi - ext->xlow;

	*yt = y * dely + ext->ylow;
	*xt = x * delx + ext->xlow;
}

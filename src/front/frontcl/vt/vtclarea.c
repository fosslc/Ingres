
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<termdr.h>
# include	"vtframe.h"	


/**
** Name:	vtclarea.c - Clear an area of a form. 
**
** Description:
**		Clear an area of a form to spaces with 0 attribute. This
**		is done so as to insure overwritting of possible box 
**		characters which may have been placed under table fields
**		and mulitline fields (and the space between the title
**		and the field. It is called during vifred's update of 
**		the features of the form.
** Name:	 -	<short comment>.
**
**	This file defines:
**
**	VTClearArea	- clear area of the screen
**
** History:
**	06/18/88 (tom) - created for 6.1 release.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */


/*{
** Name:	VTClearArea	- clear area of a form 
**
** Description:
**		Clear an area of a form to spaces with 0 attribute. This
**		is done so as to insure overwritting of possible box 
**		characters which may have been placed under table fields
**		and mulitline fields (and the space between the title
**		and the field. It is called during vifred's update of 
**		the features of the form. Since with the addition of
**		Box/line features it is necessary to update the form
**		more, this function should be able to run fast.
**		
**
** Inputs:
**	FRAME   *frm;	- form to clear on
**	i4	y,x;	- start point
**	i4	ny,nx;	- number of lines and columns to clear
**
** Outputs:
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/18/88 (tom) - written for 6.1
*/
VOID
VTClearArea(frm, y, x, ny, nx)
FRAME	*frm;
register i4  y;
register i4  x;
register i4  ny;
register i4  nx;
{

	register i4  i;
	register WINDOW *win;

	win = frm->frscr;

	for (i = 0; i < ny; i++)
	{
		MEfill((u_i2)nx, 0, win->_da[i + y] + x);  
		MEfill((u_i2)nx, ' ', win->_y[i + y] + x);  
	}
}


/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"	


/**
** Name:	vtfplus  - function to support flashing plus sign
**
** Description:
**	When the user activates the "Create" menuitem a flashing
**	plus sign is displayed at the cursor position. This file
**	supports that plus sign. Note the plus sign is supported 
**	as a one deep stack.
**
*/



/*{
** Name:	VTflashplus	- turn on/off the flashing plus sign
**
** Description:
**	This routine provides the VT interface function to 
**	support vifred's desire to put a flashing '+' sign
**	on the screen prior to creating a new form feature.
**
** Inputs:
**	FRAME *frm;	- current frame.
**	i4 y,x;	- x and y position to flash
**	i4 state;	- state to set
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04-04-88  (tom) - created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
VTflashplus(frm, y, x, state)
FRAME	*frm;
i4	y;
i4	x;
i4	state;
{

	
	TDflashplus(frm->frscr, y, x, state);
	if (state)
		VTxydraw(frm, y, x);
}


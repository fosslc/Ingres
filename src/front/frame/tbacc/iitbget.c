/*	
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include 	<runtime.h>
# include	<me.h>
# include	<er.h>

/**
** Name:	iitbget.c
**
** Description:
**
**	Public (extern) routines defined:
**		IItget()
**	Private (static) routines defined:
**
** History:
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	05-jun-89 (bruceb)
**		Change to use FE tagged memory.
**	12/11/92 (dkh) - Added IITBatsAdjustTfSize() so that we can
**			 adjust the size of a table field completely.
**	04/15/93 (dkh) - Fixed bug 50688.  Made sure that tflaastrow
**			 is also updated when the number of displayed
**			 rows in a table field is changed.  This eliminates
**			 erroneous updates to the screen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IItget		-	Allocate and init tf display
**
** Description:
**	Allocate the display portion of a tablefield, and initialize
**	all of it's components.
**
** Inputs:
**	tbfrm		Ptr to the frame
**	fld		Ptr to the tablefield in the frame	
**
** Outputs:
**
** Returns:
**	Ptr to the new tablefield
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	13-jan-1987 (peter)	Changed IIbadmem call.
*/

TBSTRUCT *
IItget(tbfrm, fld)
FRAME	*tbfrm;
FIELD	*fld;
{
	TBSTRUCT		*tb;			/* table to return */
	register TBLFLD		*tfld;			/* real table field */
	char			namestr[50];

	tfld = fld->fld_var.fltblfld;
	if ((tb = (TBSTRUCT *)FEreqmem((u_i4)(tbfrm->frtag),
	    (u_i4)(sizeof(TBSTRUCT)), TRUE, (STATUS *)NULL)) == NULL)
	{
		/* never returns */
		IIUGbmaBadMemoryAllocation(ERx("IItget"));
	}

	STcopy(FDGFName(fld), namestr);
	CVlower(namestr);
	tb->tb_name = FEtsalloc((u_i2)(tbfrm->frtag), namestr);
	tb->tb_fld = tfld;		/* parent table field */
	tb->tb_numrows = tfld->tfrows;	/* max number of rows */
	tb->dataset = NULL;		/* no data set till IItinit() */
	tb->scrintrp[0] = 0;		/* clear scroll up interrupt values  */
	tb->scrintrp[1] = 0;		/* 		down		     */
	tb->tb_display = 0;  		/* no row limits yet  */
	tb->tb_rnum = 0;
	tb->tb_mode = fdtfUPDATE;	/* may be changed in IItinit() */
	tb->tb_state = tbUNDEF;		/* so far nothing going */

	/* initialize fields in frame's representation of table */

	/*
	**  Added to set up frame info for table field traversal.
	**  Due to FT. (dkh)
	*/

	FDFTsetiofrm(tbfrm);

	/* FDtblput() ALWAYS returns TRUE */
	FDtblput(tb->tb_fld, tb->tb_mode, fdopSCRUP, fdopSCRDN,
		(i4) fdNOCODE, (i4) fdNOCODE);

	return (tb);
}


/*{
** Name:	IIUFatsAdjustTfSize - Adjust size of a table field and its
**				      parent form.
**
** Description:
**	Adjust the size of the table field and the form that contains it
**	based on the passed in size information.  The number of rows
**	that is desired is passed in.  This assumes that the caller
**	has already made sure that the value is not greater than
**	the maximum that the form was defined with originally.
**
** Inputs:
**	frm	Pointer to FRAME structure.
**	tb	Pointer to TBSTRUCT structure.
**	tf	Pointer to TBLFLD structure.
**	rows	Desired number of rows.
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
**	12/05/92 (dkh) - Initial version.
*/
void
IITBatsAdjustTfSize(frm, tb, tf, rows)
FRAME		*frm;
TBSTRUCT	*tb;
TBLFLD		*tf;
i4		rows;
{
	/*
	**  Size of form is starting position of table field
	**  plus the number of rows plus one for the bottom
	**  border of the table field plus one for zero offset
	**  for the starting position of the table field.
	*/
	frm->frmaxy = tf->tfhdr.fhposy + rows + 1 + 1;
	tf->tfhdr.fhmaxy = rows + 2;
	tf->tfrows = rows;

	/*
	**  Update last displayed row to match new size.  This
	**  is a zero based value.
	*/
	tf->tflastrow = rows - 1;

	tb->tb_numrows = rows;
}

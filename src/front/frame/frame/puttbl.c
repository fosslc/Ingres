/*
**	puttbl.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	puttbl.c - Initialize a table field for running.
**
** Description:
**	Contains routine(s) to initialize and fill a table field
**	with data.
**	Routines defined:
**	- FDtblput - Initialize a table field.
**
** History:
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	02/17/87 (dkh) - Added header.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>

/*
** FDTBLPUT
** 	initialize a table field with data
** 	the user passes in values which correspond to
**      values to return when a state change occurs
**
** RETURNS
**	TRUE - if everything is okay
**	FALSE - if tblfld is not in frame or not a table field
*/

/*{
** Name:	FDtblput - Initialize a table field for running.
**
** Description:
**	Initialize a table field for use by clearing the display
**	buffer and setting system default values for each row/column
**	in the table field.  In addition, the mode of the table
**	field as well as the scrll{up|down} activation values
**	are set.
**
** Inputs:
**	tbl	Table field to initialize.
**	mode	Mode to set on the table field.
**	scrup	Forms command to return for data scrollup (fdopSCRUP).
**	scrdown	Forms command to return for data scrolldown (fdopSCRDN).
**	delete	Forms command to return on a row delete - unused.
**	insert	Forms command to return on a row insert - unused.
**
** Outputs:
**	Returns:
**		Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**	09/16/87 (dkh) - Integrated table field change bit.
*/
i4
FDtblput(tbl, mode, scrup, scrdown, delete, insert)
TBLFLD	*tbl;
i4	mode;
i4	scrup;
i4	scrdown;
i4	delete;
i4	insert;
{
	FUNC_EXTERN	i4	FDclr();
	FUNC_EXTERN	i4	FDdat();
	FUNC_EXTERN	i4	IIFDccb();

	/* 
	** Mask out old display mode masks,
	** and assign table field new mask.
	*/
	FDtbltrv(tbl, FDclr, FDALLROW);
	FDstbltrv(tbl, FDdat, FDALLROW);
	FDtbltrv(tbl, IIFDccb, FDALLROW);
	tbl->tfhdr.fhdflags &= ~fdtfMASK;
	tbl->tfhdr.fhdflags |= mode;
	tbl->tfscrup = scrup;
	tbl->tfscrdown = scrdown;
	tbl->tfdelete = delete;
	tbl->tfinsert = insert;
    	tbl->tfstate = tfFILL;
    	tbl->tfputrow = FALSE;
	tbl->tflastrow = 0;
	tbl->tfcurcol = 0;
	tbl->tfcurrow = 0;
	return (TRUE);
}

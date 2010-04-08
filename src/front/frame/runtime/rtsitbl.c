/*
**	rtsitbl.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtsitbl.c - Support set/inquire_frs table statements.
**
** Description:
**	This file contains routines to support the inquire_frs table
**	statements.  There are no set_frs table statements.
**	Routines in this file are:
**	 - RTinqtbl() - Entry point for all the inquire_frs table statements.
**
** History:
**	02/05/85 (jen) - Initial version.
**	02/14/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	12/14/92 (dkh) - Added INTERNAL USE ONLY option for setting the
**			 number of rows for a table field.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# include	<compat.h>
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
# include	<runtime.h> 
# include	<er.h> 

GLOBALREF	FRAME	*IIsifrm;

/*{
** Name:	RTsettbl - Set attributes of a table field.
**
** Description:
**	This routine sets the attributes of a table field.
**	The only settable attribute at the moment is the number
**	of rows which is AN INTERNAL USE ONLY feature.
**
**	This routine was created so we don't have to add another entry
**	point to the system to support another internal use feature.
**
** Inputs:
**	tbl	Pointer to a TBSTRUCT structure.
**	tblfld	Pointer to a TBLFLD structure.
**	frsflg	Inquiry option.
**	data	Value to set.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE	If things worked.
**		FALSE	If things didn't work.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/14/92 (dkh) - Initial version.
*/
i4
RTsettbl(TBSTRUCT *tbl, TBLFLD *tblfld, i4 frsflg, i4 *data)
{
	i4	retval = TRUE;

	switch (frsflg)
	{
	  case frsROWMAX:
		IITBatsAdjustTfSize(IIsifrm, tbl, tblfld, *data);
		break;

	  default:
		retval = FALSE;
		break;
	}

	return(retval);
}



/*{
** Name:	RTinqtbl - Entry point for inquire_frs table statements.
**
** Description:
**	Main entry point to support the inquire_frs table statements.
**	There are no set_frs table statements currently.
**	Supported options are:
**	 - Name of the table field. (character)
**	 - The current row number (one based). (integer)
**	 - The number of physical rows in the table field. (integer)
**	 - Last physical row that has data in it (one based). (integer)
**	 - Number of (non-deleted) rows in the dataset. (integer)
**	 - Number of physical columns in the table field. (integer)
**	 - Name of current column. (character)
**	 - Mode of table field (read, update, fill or query). (character)
**
** Inputs:
**	tbl	Pointer to a TBSTRUCT structure.
**	tblfld	Pointer to a TBLFLD structure.
**	frsflg	Inquiry option.
**
** Outputs:
**	data	Data space where result is to be placed.  If result is
**		a character string, the space is assumed to be large
**		enough for the result and the null terminator.
**	Returns:
**		TRUE	If inquiry was completed successfully.
**		FALSE	If an unknown option was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/05/85 (jen) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTinqtbl(TBSTRUCT *tbl, TBLFLD *tblfld, i4 frsflg, i4 *data)
{
	FLDHDR	*hdr;
	FLDCOL	*col;
	i4	row;

	hdr = &tblfld->tfhdr;
	switch (frsflg)
	{
	  case frsTABLE:
	  case frsFLDNM:
		STcopy(hdr->fhdname, (char *) data);
		break;

	  case frsROWNO:
		/* 
		** Check if table is in middle of a scroll.
		** If scrolling down then tfcurrow may be < 0.
		** If scrolling up then tfcurrow may be > lastrow.
		** See fixstate.c for details.
		*/
		if ((row = tblfld->tfcurrow) < 0)
			row = 0;
		else if (row > tblfld->tflastrow)
			row = tblfld->tflastrow;
		*data = row +1;
		break;

	  case frsROWMAX:
		*data = tblfld->tfrows;
		break;

	  case frsLASTROW:
		/*
		** We cannot just return tf->tflastrow to the user as this 
		** has the value 0 when there is one row there or zero rows.  
		*/
		*data = tbl->tb_display;
		break;

	  case frsDATAROWS:
		*data = (tbl->dataset != NULL) ? tbl->dataset->ds_records : tbl->tb_display;
		break;
			
	  case frsCOLMAX:
		*data = tblfld->tfcols;
		break;

	  case frsCOLUMN:
		col = tblfld->tfflds[tblfld->tfcurcol];
		STcopy(col->flhdr.fhdname, (char *) data);
		break;

	  case frsFLDMODE:
		switch (tbl->tb_mode)
		{
		  case fdtfREADONLY:
			STcopy(ERx("READ"), (char *) data);
			break;

		  case fdtfUPDATE:
			STcopy(ERx("UPDATE"), (char *) data);
			break;

		  case fdtfAPPEND:
			STcopy(ERx("FILL"), (char *) data);
			break;

		  case fdtfQUERY:
			STcopy(ERx("QUERY"), (char *) data);
			break;
		}

	  default:
		return (FALSE);			/* preprocessor bug */
	}
	return (TRUE);
}

GLOBALREF RTATTLIST	IIatttbl[] ;

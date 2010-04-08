/*
**	rtsicol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
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
# include	"setinq.h"

/**
** Name:	rtsicol.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTinqcol()
**		RTsetcol()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	07/20/92 (dkh) - Added support for inquiring on existence of
**			 a column.
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

FUNC_EXTERN	i4	RTinqfld();

GLOBALREF	FIELD	*IIsifld;

/*{
** Name:	RTinqcol	-	inquire on column attributes
**
** Description:
**	Handle inquiry about a column.  Call RTinqfld to do the work.
**
** Inputs:
**	tblfld		Ptr to the tablfield
**	col		Ptr to the column structure
**	frsflg		Flag - type of object being inquired about 
**	data		Ptr to a buffer to update with the information
**
** Outputs:
**	data		Buffer is updated with result of inquiry
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

i4
RTinqcol(TBLFLD *tblfld, FLDCOL *col, i4 frsflg, i4 *data)
{
	FIELD	fld;

	switch(frsflg)
	{
	case frsSROW:
	case frsSCOL:
	case frsHEIGHT:
	case frsWIDTH:
 		RTinqfld(IIsifld, (REGFLD *) col, frsflg, data);
		break;

	case frsEXISTS:
		if (col != NULL)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	default:
		fld.fltag = FCOLUMN;

		/* NOSTRICT */
		fld.fld_var.flregfld = (REGFLD *)col;

 		RTinqfld(&fld, (REGFLD *) col, frsflg, data);
		break;
	}
}

/*{
** Name:	RTsetcol	-	Set column attributes
**
** Description:
**	Set column attributes.  Call RTsetfld to do the work.
**
** Inputs:
**	tblfld		Ptr to tablefield descriptor
**	col		Ptr to column descriptor
**	frsflg		Type of attribute being set
**	data		Ptr to buffer containing value to set
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

i4
RTsetcol(TBLFLD *tblfld, FLDCOL *col, i4 frsflg, i4 *data)
{
	FIELD	fld;

	fld.fltag = FCOLUMN;

	/* NOSTRICT */
	fld.fld_var.flregfld = (REGFLD *)col;

 	RTsetfld(&fld, (REGFLD *) col, frsflg, data);
}

GLOBALREF RTATTLIST	IIattcol[] ;


/*	iiqbfraw.c
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

/**
** Name:	iiqbfraw.c	-	QBF support routines
**
** Description:
**	Routines used by QBF to allow it to get the 'raw' string
**	from a field.  This is the string in the forms display
**	buffer.  It's used for QBF's sorting capability, and should
**	NOT be called by anything other than QBF.
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**	Created - 07/15/85 (dkh)
**	11-sep-87 (bab)
**		Changed cast in call to FDfind from char** to nat**.
**	12/31/88 (dkh) - Perf changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	FLDHDR	*IIFDgfhGetFldHdr();

/*{
** Name:	IIqbfgetraw	-	get 'raw' field data
**
** Description:
**	Cover routine for call to IIqbfraw.
**
** Inputs:
**	frame		Name of the frame
**	fieldname	Name of the field to get
**	column		No. of the column to get
**	buffer		Ptr to buffer to put the field data into
**
** Outputs:
**	buffer		Will be updated with raw field data
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

void
IIqbfgetraw(char *frame, char *fieldname, i4 column, char *buffer)
{
	IIqbfraw(frame, fieldname, column, buffer, QBFRAW_GET);
}

/*{
** Name:	IIqbfputraw	-	put 'raw' field data
**
** Description:
**	Cover routine for call to IIqbfraw.
**
** Inputs:
**	frame		Name of the frame
**	fieldname	Name of the field to put
**	column		No. of the column to put
**	buffer		Ptr to buffer containing data to put
**
** Outputs:
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
void
IIqbfputraw(char *frame, char *fieldname, i4 column, char *buffer)
{
	IIqbfraw(frame, fieldname, column, buffer, QBFRAW_PUT);
}

/*{
** Name:	IIqbfraw	-	QBF's interface to raw field data
**
** Description:
**	Validates that the field and form exist, then calls
**	FDqbfraw to do the real work.
**
** Inputs:
**	frame		Name of the form
**	fieldname	Name of the field
**	column		No. of the column
**	buffer		Place to put the raw data
**	which		get or put flag
**
** Outputs:
**	If get, updates buffer with the raw data
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

void
IIqbfraw(char *frame, char *fieldname, i4 colunm, char *buffer, i4 which)
{
	RUNFRM	*runf;
	FRAME	*frm;
	FIELD	*fld;

	if ((runf = RTfindfrm(frame)) == NULL)
	{
		*buffer = '\0';
	}
	else
	{
		frm = runf->fdrunfrm;
		if ((fld = FDfind((i4 **) frm->frfld, fieldname,
			frm->frfldno, IIFDgfhGetFldHdr)) == NULL)
		{
			*buffer = '\0';
		}
		else
		{
			FDqbfraw(frm, fld, colunm, buffer, which);
		}
	}
}

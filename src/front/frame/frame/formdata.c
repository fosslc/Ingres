/*
**	formdata.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	formdata.c - Support routines for getting form runtime data.
**
** Description:
**	Routines to support getting current runtime information are
**	defined here.  These routines are the precursors to the
**	set/inquire statements.
**
**	Table fields are always on the frfld[] list (the one which
**	not display only.  The reason for this is that if they could be on the 
**	frnsfld[] list then the user could never get the cursor on them, and
**	would not be able to manipulate or cursor TDscroll them.  
**	However, the check is done for both type of fields, for the later
**	possibility of editing table fields in Vifred.
**
**	All these routines should be phased out with the new Set/Inquire of
**	Equel.  The only one that may be required is FDfldnm().
**
**	Routines defined here are:
**	- FDfldnm - Field name of the current field.
**	- FDfldtype - Data type of current field.
**	- FDfldlen - Data length of current field.
**	- FDfrmchg - Was there a change made to the form by the user.
**	- FDfldval - Validation string for current field.
**	- FDfldfmt - Format string for current field.
**
** History:
**	JEN  - 26 Feb 1982 (written)
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
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
# include	<er.h>



/*{
** Name:	FDfldnm - Return name of current field.
**
** Description:
**	Given a form, return the name of the current field in the form.
**	Works for both regular and table fields.  Returns an empty
**	string if the form pointer is NULL or the current field number
**	has been trashed.
**
** Inputs:
**	frm	Form to obtain current field from.
**
** Outputs:
**	Returns:
**		Name	Name of current field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
char	*
FDfldnm(frm)
reg FRAME	*frm;
{
	reg i4		fldno;

	if (frm == NULL)
		return (ERx(""));
	fldno = frm->frcurfld;
	if (fldno >= frm->frfldno)
	{
		return (ERx(""));
	}
	else if (fldno < 0)
	{
		if (-fldno >= frm->frnsno+1)
			return (ERx(""));
		/*
		** The "fldname" macro definition works for table fields too
		** because of the union part of "FIELD"
		** The _field request is for the current field name; for 
		** current column name use _column.
		*/
		return (frm->frnsfld[(-fldno)-1]->fldname);
	}		
	else
	{
		return (frm->frfld[fldno]->fldname);
	}
}




/*{
** Name:	FDfldtype - Return data type for current field.
**
** Description:
**	Given a form, returne the data type of the current field in
**	the form.  Is this still needed when ADTs are in place??
**	This should only work for regular fields but no explicit
**	checks for this are done below.
**
** Inputs:
**	frm	Form containing current field.
**
** Outputs:
**	Returns:
**		0	If bad form pointer found.
**		-1	If bad field number found.
**		type	Data type of current field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
i4
FDfldtype(frm)
FRAME	*frm;
{
	reg i4		fldno;

	if (frm == NULL)
		return (0);
	fldno = frm->frcurfld;
	if (fldno >= frm->frfldno)
	{
		return ((i4) -1);
	}
	else if (fldno < 0)
	{
		if (-fldno >= frm->frnsno+1)
			return ((i4) -1);
		else
			return (frm->frnsfld[(-fldno)-1]->fldatatype);
	}		
	else
	{
		return (frm->frfld[fldno]->fldatatype);
	}
}




/*{
** Name:	FDfldlen - Get data length for current field.
**
** Description:
**	Given a form, return the data length for the current field.
**	Only works for regular fields.  Is this still needed for ADTs?
**
** Inputs:
**	frm	Form to get current field from.
**
** Outputs:
**	Returns:
**		0	If bad form pointer found.
**		-1	If bad current field number found.
**		len	Data length of current field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDfldlen(frm)
FRAME	*frm;
{
	reg i4		fldno;

	if (frm == NULL)
		return ((i4) -1);
	fldno = frm->frcurfld;
	if (fldno >= frm->frfldno)
	{
		return ((i4) -1);
	}
	else if (fldno < 0)
	{
		if (-fldno >= frm->frnsno+1)
			return ((i4) -1);
		else
			return (frm->frnsfld[(-fldno)-1]->fllength);
	}		
	else
	{
		return (frm->frfld[fldno]->fllength);
	}
}




/*{
** Name:	FDfrmchg - Did form change.
**
** Description:
**	Return information on whether the form was changed (edited)
**	by a user.  Anything typed into a field will cause the bit
**	to be set.
**
** Inputs:
**	frm	Form to check for change bit.
**
** Outputs:
**	Returns:
**		FALSE	If bad form pointer found.
**		0	If no changes.
**		1	If changes were made by user.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDfrmchg(frm)
FRAME	*frm;
{

	if (frm == NULL)
		return ((i4) FALSE);
	return (frm->frchange);
}



/*{
** Name:	FDfldval - Get validation string for current field.
**
** Description:
**	Given a form, return a pointer to the validation string
**	for the current field.  Only works for regular fields.
**	This interface may get changed due to ADTs.
**
** Inputs:
**	frm	Form to find current field from.
**
** Outputs:
**	Returns:
**		str	Pointer to validation string.  This is an empty
**			string if a bad form pointer or bad current
**			field number is found or the field has no
**			validation string.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
char	*
FDfldval(frm)
reg FRAME	*frm;
{
	reg i4		fldno;
	reg char	*valstr;

	if (frm == NULL)
		return (ERx(""));
	fldno = frm->frcurfld;
	if (fldno >= frm->frfldno)
	{
		return (ERx(""));
	}
	else if (fldno < 0)
	{
		if (-fldno >= frm->frnsno+1)
			return (ERx(""));
		else
			valstr = (frm->frnsfld[(-fldno)-1]->flvalstr);
	}		
	else
	{
		valstr = (frm->frfld[fldno]->flvalstr);
	}
	if (valstr == NULL)
		return (ERx(""));
	else
		return (valstr);
}




/*{
** Name:	FDfldfmt - Return format string for current field.
**
** Description:
**	Given a form, get the format string for the current field.
**	Only works for regular fields.
**
** Inputs:
**	frm	Form to get current field from.
**
** Outputs:
**	Returns:
**		str	Format string for current field.  It is empty
**			if a bad form pointer or invalid current field
**			number is found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
**      24-sep-96 (hanch04)
**              Global data moved to framdata.c
*/
char	*
FDfldfmt(frm)
reg FRAME	*frm;
{
	reg i4		fldno;
	reg char	*fmtstr;

	if (frm == NULL)
		return (ERx(""));
	fldno = frm->frcurfld;
	if (fldno >= frm->frfldno)
	{
		return (ERx(""));
	}
	else if (fldno < 0)
	{
		if (-fldno >= frm->frnsno+1)
			return (ERx(""));
		else
			fmtstr = (frm->frnsfld[(-fldno)-1]->flfmtstr);
	}		
	else
	{
		fmtstr = (frm->frfld[fldno]->flfmtstr);
	}
	if (fmtstr == NULL)
		return (ERx(""));
	else
		return (fmtstr);
}


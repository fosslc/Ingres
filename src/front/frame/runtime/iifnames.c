/*
**	iifnames.c
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
# include	<rtvars.h>

/**
** Name:	iifnames.c	-	FORMDATA loop service routine
**
** Description:
**
**	Public (extern) routines defined:
**		IIfnames()
**
**	Private (static) routines defined:
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	 9-mar-1983  -  Fixed bug #775 (jen)
**	29-apr-1983  -	Deleted all unnessessary data transfers, now 
**			that they are executed inside IIfnames() loop.
**			Cont. of bug #775 (ncg)
**	15-may-1984  -	Now scans form with no regular fields. (ncg)
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIfnames	-	Get fld names for FORMDATA
**
** Description:
**	This routine is the main call in the FORMDATA loop.  It is set up
**	the same way as a display statement, with IIactfrm().  It then sets
**	all of the FRS variables requested.
**
**	Manipulates the 'current field' variable in the frame structure
**	for the current frame on the stack frame.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	TRUE	When the 'next' field is pointed to successfully
**		FALSE	When all fields have been done
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## formdata f1
**	## {
**	## }
**
**	if (IIdispfrm("f","f") == 0) goto IIfdE1;
**	while (IIfnames() != 0) {
**	} 
**  IIfdE1:
**	IIendfrm();
**
** Side Effects:
**	Changes IIstkfrm->fdruncur to point to the 'next' field in
**	the form, to support the FORMDATA loop
**
** History:
**	
*/

IIfnames()
{
	register FRAME	*frm;

	/*
	**  Indicates that we are running in FORMDATA loop.
	**  Allows us to handle things correctly for one
	**  window project.  (dkh)
	*/

	IIfromfnames = 1;

	/* 
	** The equivalent of RTchkfrs(IIstkfrm) is done via IIactfrm() 
	** We need this check here, just in case someone did a 'goto' out
	** of the loop, and then came back in after setting some other form
	** state outside. (ncg)
	*/
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	frm = IIstkfrm->fdrunfrm;

	/*
	**	If all sequenced fields and non-sequenced fields have
	**	been exhausted, then return and end the FORMDATA loop.
	*/
	if (frm->frfldno == 0)		/* No regular fields */
	{
		if (frm->frnsno == 0)	/* No fields at all */
		{
			return (FALSE);
		}
		if (IIstkfrm->fdruncur == 0)	/* First non-seq field */
		{
			IIstkfrm->fdruncur = -1;
		}
	}
	if (IIstkfrm->fdruncur >= frm->frfldno
	    || -(IIstkfrm->fdruncur) > frm->frnsno)
	{
		return (FALSE);
	}
	/*
	**	Save the current field number and set the field number
	**	for the frame driver.  This number is negative if we are
	**	looking at non-sequenced fields.
	*/

	frm->frcurfld = IIstkfrm->fdruncur;

	/*
	**	Get the requested run-time information
	**	The following code is not required any more.
	**
	**	Deleted (ncg)
	**
	*/

	/*
	**	Reset the current field.  If non-sequenced field, decrement.
	**	If sequenced field, increment.  If out of sequenced fields,
	**	begin non-sequenced fields if any.
	*/
	if (IIstkfrm->fdruncur < 0)
	{
		IIstkfrm->fdruncur--;
	}
	else
	{
		IIstkfrm->fdruncur++;
		if (IIstkfrm->fdruncur >= frm->frfldno)
			IIstkfrm->fdruncur = -1;
	}
	IIsetferr((ER_MSGID) 0); /* clear FRS error flag each time in loop */
	return (TRUE);
}

/*
**	error.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	error.c - Generic error message facility for frs.
**
** Description:
**	This routine is the entry point for the forms system to
**	output numbered error messages to the screen.  Calls
**	IIUGerr to do the work.
**
** History:
**	Written  9/1/82 (jrc)
**	02/15/87 (dkh) - Added procedure header.
**	08/26/87 (dkh) - Changes for ER and 8000 series error numbers.
**	09/05/87 (dkh) - Error processing for forms system now goes
**			 through IIFDerror().
**	09/08/87 (dkh) - Added IIerror(), IIp_err() and IIx_err()
**			 so FDerror will work on a temporary basis.
**			 Also extracted a version of FDerror for QBF to use.
**	09/11/87 (dkh) - Moved FDerror and associated routines to its own file.
**	09/12/87 (dkh) - Got rid of unnecessary references.
**	10/21/87 (peter) - Change inquire to return error in same
**			format as LIBQ.
**	01/11/90 (dkh) - Force screen redisplay before putting out
**			 error message.
**      12/18/92 (donc) - Fix for bug 45209 (VMS). If FDerror is called by
**			 the IIFVadfxhdlr after the "unavailable frame" screen,
**			 the FDredisp in FDerror would get called causing yet
**			 another exception, and so on and so on. Finally the
**			 image would page fault and grow so large that VMS would
**			 kick it out.
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
# include	<si.h>
# include	<ug.h>

FUNC_EXTERN	ER_MSGID	IILQfrserror();

GLOBALREF	FRAME		*frcurfrm;

# define	ERR_MASK	(i4) 0x0000ffff

/*
**  This is the variable holding the forms system error number.
*/
static	ER_MSGID	frserrno;

/*
**  This varaible holds the error text for the current forms system error.
*/
static	char	error_text[2*ER_MAX_LEN + 1];


/*{
** Name:	IIFDerror - Generic forms system error display routine.
**
** Description:
**	This is the generic error display routine used by the various
**	parts of the forms system.  This routine only handles errors
**	in the 8000 range (and thus are trappable and inquireable
**	by the user.  To allow user applications to trap these error
**	numbers, this routine will first call IIfrs_seterr().  If
**	a zero is returned, then this routine returns immediately.
**	If any other value is returned by IIfrs_seterr(), this routine
**	calls IIUGerr to display the error message associated with the
**	passed in error number.
**
**	This routine also sets a local variable with the passed in
**	error number so that an application can inquire on it.
**
** Inputs:
**	error_id	The error number.
**	parcount	Count of the number of parameters to display
**			with error message.
**	a1-a10		The ten possible arguments.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Sets local varaible "frserrno" to passed in error number.
**
** History:
**	08/26/87 (dkh) - Initial version.
*/
VOID
IIFDerror(error_id, parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
ER_MSGID	error_id;
i4		parcount;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{
	char		errtext[ER_MAX_LEN + 1];

	frserrno = error_id & ERR_MASK;

	/*
	**  Format the error message so that user can inquire on it
	**  in the error catching routine set by a call to IIseterr().
	*/
	IIUGber(errtext, ER_MAX_LEN, error_id, UG_ERR_ERROR, parcount,
		a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	_VOID_ IIUGfemFormatErrorMessage (error_text, 2*ER_MAX_LEN,
		errtext, FALSE);

	/*
	**  Allow user to catch error number, if necessary.  If
	**  a zero is returned, just return.  Any other value
	**  causes the error message to be displayed.
	*/
	if (!IILQfrserror(frserrno))
	{
		return;
	}

	/*
	**  Force a redisplay to sync up screen with current
	**  display loop.  I believe that IIendfrm() will
	**  correctly set frcurfrm to to current display
	**  loop's form.
	*/
	if (frcurfrm && frcurfrm->frmflags & fdDISPRBLD)
	{
		/* DonC: Fix for bug 45209 */
		frcurfrm->frmflags ^= fdDISPRBLD;
		FDredisp(frcurfrm);
	}

	IIUGeppErrorPrettyPrint (stdout, errtext, FALSE);
}


/*{
** Name:	IIFDsterr - Set forms system error number
**
** Description:
**	This routine sets the local variable "frserrno" to the
**	passed in error number.
**
** Inputs:
**	error_id	Error number to set.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	The error text buffer (error_text) is cleared if the
**	error id is zero.
**
** History:
**	08/26/87 (dkh) - Initial version.
*/
VOID
IIFDsterr(error_id)
ER_MSGID	error_id;
{
	/*
	**  If settting error number to zero, clear out
	**  the error text buffer as well.
	*/
	if ((frserrno = (error_id & ERR_MASK)) == 0)
	{
		error_text[0] = EOS;
	}
}


/*{
** Name:	IIFDgterr - Get the forms system error number.
**
** Description:
**	Return the value in local variable "frserrno" to the caller.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		error_id	The value in "frserrno".
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/26/87 (dkh) - Initial version.
*/
i4
IIFDgterr()
{
	/*
	**  Force to a i4  since the error number that the
	**  user gets can always fit in two bytes.
	*/
	return((i4) frserrno);
}


/*{
** Name:	IIFDgetGetErrorText - Get error text for last frs error.
**
** Description:
**	Returns a pointer to the buffer holding the error text of the
**	last forms system error.  If there is no error, the buffer holds
**	an empty string.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		Pointer to beginning of error text.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/87 (dkh) - Initial version.
*/
char *
IIFDgetGetErrorText()
{
	return(error_text);
}

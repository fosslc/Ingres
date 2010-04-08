/*
**	iiformerr.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>

/**
** Name:	iiformerr.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIsetferr()
**		IIFRerr()
**	Private (static) routines defined:
**
** History:
**	4-mar-1983 	- written (ncg)
**	08/26/87 (dkh) - Changed for 6.0 and ER.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/05/87 (dkh) - Changed IIfderr to return a i4  instead of ER_MSGID.
**	10/02/87 (dkh) - Changed IIfderr to IIFRerr for check7.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	VOID	IIFDsterr();
FUNC_EXTERN	i4	IIFDgterr();

/*{
** Name:	IIsetferr	-	Set FRS error flag
**
** Description:
**	 Any error coming through IIerror() from the front or back end will
**	 use IIerrdisp (if IIform_on is set -- see IIforms.c where IIwin_err
**	 is set). IIerrdisp will set the error number to the error detected.
**	 Set from IIerrdisp(), IItbug() , TDscroll errors in IItscroll()
**	 and IItsetio() and IItact() for validation errors.
**	 Reset from IIrunfrm(), IIstartfrm(), IIactfrm(), IIendfrm() and
**	 IIrunfrm().
**
**	Will set the error number provided as an input parameter in 
**	IIerrflag, and in the local static errno.
**
** Inputs:
**	err		Error to set
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**	Sets IIerrflag to err.
**
** History:
**	
*/

void
IIsetferr(ER_MSGID err)
{
	IIFDsterr(err);
}


/*{
** Name:	IIFRerr	-	Return forms error no.
**
** Description:
**	Retunrs an error number to the caller.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	The current value of errno.
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
IIFRerr()
{
	return(IIFDgterr());
}

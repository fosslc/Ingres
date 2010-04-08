/*
**	rtgetfrm.c
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
# include	<frserrno.h>
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	rtgetfrm.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTgetfrm()
**	Private (static) routines defined:
**
** History:
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	07/21/88 (dkh) - Added routine RTispopup() to aid the
**			 "help" facility.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	RTgetfrm	-	Get pointer to frame descriptor
**
** Description:
**	Given the name of a frame, look through the list of initialized
**	frames and return a pointer to it's descriptor.
**
** Inputs:
**	nm		Name of the frame to locate
**
** Outputs:
**
** Returns:
**	Ptr to the RUNFRM structure for the frame.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**		21-jul-1983  -  written (jen)
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

RUNFRM	*
RTgetfrm(nm)
reg	char	*nm;
{
	if (nm == NULL || *nm == EOS)
	{
		if (IIstkfrm == NULL)
			IIFDerror(RTFRACT, 0, NULL);
		return (IIstkfrm);
	}

	/* search for the frame on the list of initialized frames */
	return (RTfindfrm(nm));
}



/*{
** Name:	RTispopup - Determine if form is a popup or not.
**
** Description:
**	Determine if form (based on passed in name) is a popup or not.  Use 
**	current form is empty string passed in.
**
** Inputs:
**	Name	Form name.
**
** Outputs:
**	Returns:
**		TRUE	If form is a popup.
**		FALSE	If form is not a popup.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/22/88 (dkh) - Initial version.
*/
i4
RTispopup(name)
char	*name;
{
	i4	retval = FALSE;
	RUNFRM	*rfrm;

	if ((rfrm = RTgetfrm(name)) != NULL &&
		rfrm->fdrunfrm->frmflags & fdISPOPUP)
	{
		retval = TRUE;
	}

	return(retval);
}

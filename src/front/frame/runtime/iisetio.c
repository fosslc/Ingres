/*
**	static	char	*Sccsid = "@(#)iisetio.c	1.2	2/5/85";
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<frserrno.h>
# include	<rtvars.h>
# include	<lqgdata.h>

/*
**	IIsetio.c  -  Set a frame for GETFORM/GETOPER/PUTFORM
**
**	Defines: IIfsetio( formname )
**
**	This routine sets up a frame for the forms runtime system
**	to interact with the fields in that form.  If a name is passed
**	it looks in the list of initialized forms to send information
**	to that form.  If it is NULL or blank, it uses the currently
**	displayed form.
**
**  History:
**	16-feb-1983  -	Extracted from original runtime.c (jen)
**	22-may-1984  -	Added "most recently used" test.  (ncg)
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/11/87 (dkh) - Added fix to make sure FDFTiofrm is in sync.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**
**	Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/


IIfsetio(nm)
char	*nm;
{
	reg	RUNFRM	*runf;
		char	*name;
	char		fbuf[MAXFRSNAME+1];

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return (FALSE);
	}

	/*
	**	If the name is NULL set the current form.
	*/
	if (nm == NULL)
	{
		if ((IIfrmio = IIstkfrm) != NULL)
		{
			/*
			**  Set up frame info for possible table field
			**  traversal.	Necessitated by FT. (dkh)
			*/
			FDFTsetiofrm(IIfrmio->fdrunfrm);
			return (TRUE);
		}
		else
		{
			IIFDerror(RTFRIN, 1, ERx("NULL"));
			return (FALSE);
		}
	}

	name = IIstrconv(II_CONV, nm, fbuf, (i4)MAXFRSNAME);

	/*
	**	Search the list of initialized forms.  Fisrt test against
	**	most recently used (IIfrmio).
	*/
	if (IIfrmio != NULL)
	{
		if (STcompare(name, IIfrmio->fdfrmnm) == 0)
		{
			FDFTsetiofrm(IIfrmio->fdrunfrm);
			return (TRUE);
		}
	}
	runf = RTfindfrm(name);
	if (runf == NULL)
	{
		IIFDerror(RTFRIN, 1, name);
		return (FALSE);
	}

	/*
	**	Set the form I/O pointer
	*/
	IIfrmio = runf;

	/*
	**  Set up frame info for possible table field
	**  traversal.	Necessitated by FT. (dkh)
	*/

	FDFTsetiofrm(IIfrmio->fdrunfrm);

	return (TRUE);
}

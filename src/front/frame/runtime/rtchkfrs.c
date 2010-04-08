/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	static	char	*Sccsid = "@(#)rtchkfrs.c	1.2	2/5/85";
*/


/*
**  RTCHKFRS  -  Make the checks necessary for the FRS system
**
**  History:
**	21-jul-1983  -  Written (jen)
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
# include	<lqgdata.h>
# include	<si.h>
# include       <er.h>

i4
RTchkfrs(frmptr)
reg RUNFRM	*frmptr;
{
	if(!IILQgdata()->form_on)
	{
		/* no ## forms statement issued yet */
		IIFDerror(RTNOFRM, 0, NULL);
		return (FALSE);
	}

	if (frmptr == NULL)
	{
		/* no form is currently active in Equel/Forms */
		IIFDerror(RTFRACT, 0, NULL);
		return (FALSE);
	}

	return (TRUE);
}

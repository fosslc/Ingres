/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

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
# include	<frserrno.h>
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iifldclr.c
**
** Description:
**	Clear a field in the frame.
**
**	Public (extern) routines defined:
**		IIfldclear()
**	Private (static) routines defined:
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	09-mar-1983  -	Modified to clear table fields. (ncg)
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>
**	16-jun-89 (bruceb)
**		Call on FDfldclr changed to match new interface.
**		As result, no need to call FDFTsetiofrm and FDFTsmode.
**	08/11/91 (dkh) - Passed FRAME pointer to IITBtclr() so that clearing
**			 a table field will clear out dataset attributes
**			 from the table field cells holding area as well.
**	03/01/92 (dkh) - Renamed IItclr() to IITBtclr() to avoid name
**			 space conflicts with eqf generated symbols.
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
** Name:	IIfldclear	-	Clear a field
**
** Description:
**	Clear the named field.  It may be a simple field or a tablefield.
**	Tablefields are cleared by calling IITBtclr, simple fields by 
**	calling FDfldclr.
**	
**	Using the frame on top of the frame stack, looks through it's
**	list of fields to find a match on name.  When found clears it.
**	If not found looks through the list of non-sequenced fields for 
**	the name, and, if found, clears the field.  
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	strvar		name of the field to clear
**
** Outputs:
**
** Returns:
**	i4	TRUE	Field was cleared successfully
**		FALSE	Field could not be cleared
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## clear field a, b
**
**	IIfldclear("a");
**	IIfldclear("b");
**
** Side Effects:
**
** History:
**	
*/

IIfldclear(strvar)
char	*strvar;
{
	FRAME	*frm;
	FIELD	*fld;
	i4	i;
	char	*name;
	char		fbuf[MAXFRSNAME+1];

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (strvar == NULL)
		return (FALSE);
	
	name = IIstrconv(II_CONV, strvar, fbuf, (i4)MAXFRSNAME);
	
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	frm = IIstkfrm->fdrunfrm;
	for (i = 0; i < frm->frfldno; i++)
	{
		fld = frm->frfld[i];
		if (STcompare(fld->fldname, name) == 0)
		{
			if (fld->fltag == FTABLE)
				IITBtclr(IItfind(IIstkfrm, name),
					IIstkfrm->fdrunfrm);
			else
				FDfldclr(frm, FT_UPDATE, i);
			return (TRUE);
		}
	}
	for (i = 0; i < frm->frnsno; i++)
	{
		fld = frm->frnsfld[i];
		if (STcompare(fld->fldname, name) == 0)
		{
			FDfldclr(frm, FT_DISPONLY, i);
			return (TRUE);
		}
	}

	IIFDerror(RTCLFL, 2, IIstkfrm->fdfrmnm, name);
	return (FALSE);
}

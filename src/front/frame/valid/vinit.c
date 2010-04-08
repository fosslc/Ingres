/*
**	Copyright (c) 2004 Ingres Corporation
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
# include	<valid.h>
# include	<er.h>


/*
**	The yyvinit() routine sets the validation
**	string of a field as the input for the parser.
**
**	History: 5 Dec 1984  -	moved from lex.c (bab)
**	08/14/87 (dkh) - ER changes.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	06/23/89 (dkh) - Changes for derived field support.  Changed
**			 name of routine from yyvinit to IIFVVinit so
**			 two parsers can exist in the forms system.
**	09/25/92 (dkh) - Added support for owner.table.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
*/

FUNC_EXTERN	void	IIFVsdsScanDelimString();

GLOBALREF char		*vl_buffer ;
GLOBALREF FLDHDR	*IIFVflhdr ;
GLOBALREF bool		*IIFVxref ;

IIFVvinit(frame, hdr, type)
FRAME	*frame;
FLDHDR	*hdr;
FLDTYPE *type;
{
	if ((vl_curfrm = frame) == NULL
	||  (vl_curhdr = hdr) == NULL
	||  (vl_curtype = type) == NULL)
	{
		vl_buffer = ERx("");
		return(FALSE);
	}
	vl_buffer = type->ftvalstr;
	IIFVflhdr = hdr;
	IIFVxref = FALSE;
	IIFVsdsScanDelimString(FALSE);
	return(TRUE);
}

/*
**	dinit.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/* # include's */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"derive.h"
# include	<afe.h>
# include	<cm.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	"erfv.h"

/**
**  Name:	dinit.c - Init stuff for derived fields.
**
**  Description:
**	Contains routine to initialize the derivation parser
**	for parsing a new derivation formula.
**
**	This file defines:
**
**	IIFVdinit	Initialize derivation parser.
**
**  History:
**	06/05/89 (dkh) - Initial version.
**	19-jan-90 (bruceb)
**		IIFVdfnDrvFrmName() now returns the form name in quotes
**		so that the error messages (in erfi.msg) don't contain
**		the quote marks.  This is so that "being saved" will be
**		displayed in those error messages without quote marks.
**	19-oct-1990 (mgw)
**		Fixed #include of local erfv.h and derive.h to use ""
**		instead of <>
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* # define's */

/* Field being parse for derivation formula */
GLOBALREF	FIELD	*IIFVcfCurFld ;

/* DeriveFld debug/print flag */
GLOBALREF	bool	IIFVddpDebugPrint ;

/* Flag indicating if derived formula was ALL constants */
GLOBALREF	bool	IIFVocOnlyCnst ;



/* extern's */
GLOBALREF	char	*vl_buffer;
GLOBALREF	FRAME	*vl_curfrm;
GLOBALREF	FLDHDR	*vl_curhdr;
GLOBALREF	FLDTYPE	*vl_curtype;
GLOBALREF	IIFVdstype IIFVdlval;
GLOBALREF	bool	 vl_syntax;
GLOBALREF	i4	IIFVorcOprCnt;
GLOBALREF	i4	IIFVodcOpdCnt;

FUNC_EXTERN	VOID	IIFVdtDrvTrace();


/* static's */
static	char	f_name[FE_MAXNAME + 2 + 1];	/* Size for `name' and EOS. */


/*{
** Name:	IIFVdinit - Initialze derivation parser.
**
** Description:
**	Initializes the derivation parser for parsing a new
**	derivation formula.
**
** Inputs:
**	frm		Pointer to form containing field to be parsed.
**	fld		The field to be parsed.
**	hdr		The FLDHDR structure of the field/column.
**	type		The FLDTYPE structure of the field/column.
**
** Outputs:
**
**	Returns:
**		TRUE	If initialization was successful.
**		FALSE	If initialization failed.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	Various variables are intialized in preparation for
**	parsing a derivation formula.
**
** History:
**	06/05/89 (dkh) - Initial version.
*/
i4
IIFVdinit(frm, fld, hdr, type)
FRAME	*frm;
FIELD	*fld;
FLDHDR	*hdr;
FLDTYPE	*type;
{
	IIFVdtDrvTrace(frm, fld, hdr, type);
	if ((vl_curfrm = frm) == NULL ||
		(vl_curhdr = hdr) == NULL ||
		(vl_curtype = type) == NULL ||
		(IIFVcfCurFld = fld) == NULL)
	{
		vl_buffer = ERx("");
		return(FALSE);
	}
	vl_buffer = type->ftvalstr;
	IIFVocOnlyCnst = TRUE;
	IIFVorcOprCnt = 0;
	IIFVodcOpdCnt = 0;
	return(TRUE);
}




/*{
** Name:	IIFVddfDeriveDbgFlg - Set debug flag of derivation parser.
**
** Description:
**	Variable IIFVddpDebugPrint is set to passed in value.
**	This variable controls whether debug information is
**	printed about the derivation tree that is built by
**	the parser.
**
** Inputs:
**	val		Value to set IIFVddpDebugPrint to.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/05/89 (dkh) - Initial version.
*/
VOID
IIFVddfDeriveDbgFlg(val)
bool	val;
{
	IIFVddpDebugPrint = val;
}


/*{
** Name:	IIFVdfnDrvFrmName - Return name of form containing field being parsed.
**
** Description:
**	Routine simply returns the name of the form that contains the
**	field being parsed by the derivation parser.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		name	Name of form that contains field being parsed
**			or the generic "being saved" if saving from
**			vifred at the moment.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/05/89 (dkh) - Initial version.
*/
char *
IIFVdfnDrvFrmName()
{
	if (vl_syntax)
	{
		return(ERget(F_FV0002_being_saved));
	}
	else
	{
		STprintf(f_name, ERx("`%s'"), vl_curfrm->frname);
		return(f_name);
	}
}

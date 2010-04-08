/*
**	fdfndfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdfndfld.c - Find a field.
**
** Description:
**	File contains routine to find a field in a form.
**
** History:
**	29-nov-1984  -  pulled from RTgetfld() (bab)
**	05-jan-1987 (peter)	Moved from RTfndfld.
**	02/15/87 (dkh) - Added header.
**	11-sep-87 (bab)
**		Changed cast in call on FDfind from char** to nat**.
**	12/31/88 (dkh) - Perf changes.
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
# include	<frserrno.h>


FUNC_EXTERN	FLDHDR	*IIFDgfhGetFldHdr();


/*{
** Name:	FDfndfld - Find a field descriptor.
**
** Description:
**	Given a form and field, find the field in the form and
**	returned the field descriptor.  Also return display only
**	indicator in the passed paramter.
**
** Inputs:
**	frm	Form to look for field in.
**	ns	Name of field to look for.
**
** Outputs:
**	ns	If field is display only, set to TRUE, FALSE otherwise.
**	Returns:
**		ptr	Field descriptor of found field.  NULL otherwise.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	29-nov-1984  -  pulled from RTgetfld() (bab)
**	05-jan-1987 (peter)	Moved from RTfndfld.
**	02/15/87 (dkh) - Added procedure header.
*/
FIELD	*
FDfndfld(frm, nm, ns)
reg	FRAME	*frm;
reg	char	*nm;
bool		*ns;	/* return TRUE if the fld is non-sequence, else FALSE */
{
	reg	FIELD	*fld;
	bool		nonseq;

	nonseq = FALSE;
	/*
	**	Search the list of fields.
	*/
	if ((fld = FDfind((i4 **)frm->frfld, nm, frm->frfldno,
		IIFDgfhGetFldHdr)) == NULL)
	{
		nonseq = TRUE;
		fld = FDfind((i4 **)frm->frnsfld, nm, frm->frnsno,
			IIFDgfhGetFldHdr);
	}

	*ns = nonseq;
	return (fld);	/* fld == NULL if not a non-sequence field either */
}

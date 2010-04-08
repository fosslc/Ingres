/*
**	gtfldno.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	gtfldno.c - Get a field sequence number.
**
** Description:
**	Return field sequence number for a field.
**	Routine defined:
**	- FDgtfldno - Get a field sequence number.
**
** History:
**	bab - 15 Feb 1985  (pulled from other files, including
**			chkfld.c, getfld.c, qryop.c, ...)
**	02/15/87 (dkh) - Added header.
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





/*{
** Name:	FDgtfldno - Get a field sequence number.
**
** Description:
**	Get the field sequence number.  It is no big deal for
**	updateable fields.  However, for display only fields,
**	we must return the index into the display only chain
**	for the field.
**
** Inputs:
**	mode	Whether to look on updateable (FT_UPDATE) or display
**		only (FT_DISPONLY) chain of fields.
**	frm	Form that contains field.
**	fd	Pointer to the field.
**
** Outputs:
**	regfld	The REGFLD pointer of the field is assigned here.
**	Returns:
**		seqno	The sequence number for the field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDgtfldno(mode, frm, fd, regfld)
i4		mode;
register FRAME	*frm;
register FIELD	*fd;
REGFLD		**regfld;
{
	register i4	fldno;		/* current field pointer */
	i4		i;

	*regfld = fd->fld_var.flregfld;
	if (mode == FT_DISPONLY)
	{
		for (i = 0; i < frm->frnsno; i++)
		{
			if (fd == frm->frnsfld[i])
			{
				fldno = i;
				break;
			}
		}
	}
	else
	{
		fldno = (*regfld)->flhdr.fhseq;
	}

	return (fldno);
}

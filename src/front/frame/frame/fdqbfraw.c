/*
**	fdqbfraw.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdqbfraw.c - Raw (string) field I/O for QBF.
**
** Description:
**	Support for QBF to do RAW (string) field I/O.
**	Routines defined:
**	- FDqbfraw - Raw I/O for QBF.
**
** History:
**	Created - 07/15/85 (dkh)
**	85/07/19  11:01:31  dave
**		Initial revision
**	85/07/23  15:23:12  dave
**		Initial version in 40 rplus path. (dkh)
**	85/09/18  16:21:55  john
**		Changed i4=>nat, CVla=>CVna, CVal=>CVan, unsigned char=>u_char
**	85/11/05  20:45:45  dave
**		extern to FUNC_EXTERN for routines. (dkh)
**	86/11/18  21:03:28  dave
**		Initial60 edit of files
**	02/17/87 (dkh) - Added header.
**	18-may-88 (bruceb)
**		Updated for reverse fields.
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

FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDVAL	*FDgetval();




/*{
** Name:	FDqbfraw - Raw (string) I/O for QBF.
**
** Description:
**	Special routine for QBF to get the string in a field without
**	any other processing.  This routine allows QBF to get/put
**	character strings to any field in a form, regardless of
**	data type.  This is needed to support the "sort/order"
**	feature in QBF.  Access to table fields is restricted to
**	the first physical row.
**
** Inputs:
**	frm	Form containing field for I/O.
**	fld	The field itself.
**	column	Column number if field is a table field.
**	buffer	String to place in field if doing a put.
**	which	Whether to do a get (QBFRAW_GET) or put
**		(QBFRAW_PUT) for the field.
**
** Outputs:
**	buffer	Buffer space to place string from field if doing a get.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Routine is bypassing normal field processing to do RAW like
**	I/O.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
VOID
FDqbfraw(frm, fld, column, buffer, which)
FRAME	*frm;
FIELD	*fld;
i4	column;
char	*buffer;
i4	which;
{
	DB_DATA_VALUE	*ddbv;
	DB_TEXT_STRING	*text;
	i4		count;
	i4		i;
	u_char		*ptr;
	u_char		*cp;
	TBLFLD		*tbl;
	FLDVAL		*val;
	FLDHDR		*hdr;
	i4		ocol;
	i4		orow;
	i4		row = 0;
	i4		fldtype = FT_REGFLD;
	i4		width;
	i4		reverse;

	if (fld->fltag == FTABLE)
	{
		fldtype = FT_TBLFLD;
		tbl = fld->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = column;
		tbl->tfcurrow = 0;
		val = FDgetval(fld);
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}
	else
	{
		hdr = FDgethdr(fld);
		val = FDgetval(fld);
	}

	reverse = (hdr->fhdflags & fdREVDIR ? TRUE : FALSE);

	ddbv = val->fvdsdbv;
	text = (DB_TEXT_STRING *) ddbv->db_data;
	ptr = text->db_t_text;
	cp = (u_char *) buffer;
	if (which == QBFRAW_GET)
	{
		count = text->db_t_count;
		if (reverse)
		{
			ptr += (count - 1); /* point to right end of buffer */
			for (i = 0; i < count; i++)
			{
				*cp++ = *ptr--;
			}
		}
		else
		{
			for (i = 0; i < count; i++)
			{
				*cp++ = *ptr++;
			}
		}
		*cp = EOS;
	}
	else
	{
		count = 0;
		if (reverse)
		{
			width = ddbv->db_length - DB_CNTSIZE;
			ptr += (width - 1); /* point to right end of buffer */
			while (*cp)
			{
				*ptr-- = *cp++;
				count++;
			}
			/* blank pad the leading spaces in the buffer */
			while (count < width)
			{
				*ptr-- = ' ';
				count++;
			}
		}
		else
		{
			while (*cp)
			{
				*ptr++ = *cp++;
				count++;
			}
		}
		text->db_t_count = count;
		FTfldupd(frm, hdr->fhseq, FT_UPDATE, fldtype, column, row);
	}
}

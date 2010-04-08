/*
**	nmretrv.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	nmretrv.c - Retrieve names of fields/columns.
**
** Description:
**	This file contains routines to retrieve names of fields/columns.
**	Routines defined in this file are:
**	- FDGFName - Retrieve name of a field.
**	- FDGCName - Retrieve name of a column.
**
** History:
**	02/15/87 (dkh) - Added header.
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




/*{
** Name:	FDGFName - Get name of a field.
**
** Description:
**	Given a field, return the name associated with the it.
**
** Inputs:
**	fd	Field to get name from.
**
** Outputs:
**	Returns:
**		name	Name of field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
char *
FDGFName(fd)
reg FIELD	*fd;
{
	reg FLDHDR	*hdr;

	if (fd->fltag == FTABLE)
		hdr = &(fd->fld_var.fltblfld->tfhdr);
	else
		hdr = &(fd->fld_var.flregfld->flhdr);

	return (hdr->fhdname);
}




/*{
** Name:	FDGCName - Get name of a column.
**
** Description:
**	Given a column, return the name associated with it.
**
** Inputs:
**	col	Column to get name from.
**
** Outputs:
**	Returns:
**		name	Name of column.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
char *
FDGCName(col)
reg FLDCOL	*col;
{
	return (col->flhdr.fhdname);
}




/*
** Name:	IIFDgfhGetFldHdr - Get the FLDHDR for a field.
**
** Description:
**	Return a pointer to the FLDHDR structure for a field.
**
** Inputs:
**	fld	Pointer to a REGFLD structure.
**
** Outputs:
**	Returns:
**		hdr	Pointer to a FLDHDR structure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/31/88 (dkh) - Initial version.
*/
FLDHDR *
IIFDgfhGetFldHdr(fld)
FIELD	*fld;
{
	FLDHDR	*hdr;

	if (fld->fltag == FTABLE)
	{
		hdr = &(fld->fld_var.fltblfld->tfhdr);
	}
	else
	{
		hdr = &(fld->fld_var.flregfld->flhdr);
	}

	return(hdr);
}




/*{
** Name:	IIFDgchGetColHdr - Get the FLDHDR for a table field column.
**
** Description:
**	Return a pointer to the FLDHDR structure for a table field column.
**
** Inputs:
**	col	Pointer to a FLDCOL structure.
**
** Outputs:
**	Returns:
**		hdr	Pointer to a FLDHDR structure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/31/88 (dkh) - Initial version.
*/
FLDHDR *
IIFDgchGetColHdr(col)
FLDCOL	*col;
{
	return(&(col->flhdr));
}




/*{
** Name:	IIFDcncCreateNameChecksum - Create checksum for a name.
**
** Description:
**	Given a string compute a checksum for the string.
**
** Inputs:
**	name	String to compute checksum on.
**
** Outputs:
**	Returns:
**		chksum	Checksum for passed in string.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/31/88 (dkh) - Initial version.
*/
i4
IIFDcncCreateNameChecksum(name)
char	*name;
{
	char	*ptr;
	u_char	checksum;

	ptr = name;
	checksum = (u_char) *ptr++;
	while (*ptr)
	{
		checksum ^= (u_char) *ptr++;
	}

	return((i4) checksum);
}

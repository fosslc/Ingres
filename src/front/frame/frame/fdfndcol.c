/*
**	fdfndcol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdfndcol.c - Find a column.
**
** Description:
**	File contains routines to find a column in a table field.
**	- FDfndcol - Find column in a table field.
**
** History:
**	29-nov-1984  -  pulled from RTgetcol() (bab)
**	5-jan-1987 (peter)	Moved from RTfndcol.c.
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

FUNC_EXTERN	FLDHDR	*IIFDgchGetColHdr();


/*{
** Name:	FDfndcol - Find a column in a table field.
**
** Description:
**	Given a table field and the name of a column, find
**	the column in the table field and return the column
**	descriptor.
**
** Inputs:
**	tbl	Table field to look in.
**	name	Name of column to look for.
**
** Outputs:
**	Returns:
**		ptr	Column descriptor.  NULL if column does not
**			exist in the table field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	29-nov-1984  -  pulled from RTgetcol() (bab)
**	5-jan-1987 (peter)	Moved from RTfndcol.c.
**	02/15/87 (dkh) - Added procedure header.
*/
FLDCOL	*
FDfndcol(tbl, name)
reg	TBLFLD	*tbl;
reg	char	*name;
{
	/* NOSTRICT */
	return ((FLDCOL *)FDfind((i4 **)tbl->tfflds, name,
		tbl->tfcols, IIFDgchGetColHdr));
}

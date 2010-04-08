/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
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

/**
** Name:	rtgetcol	-	Get a column descriptor
**
** Description:
**
**	Public (extern) routines defined:
**		RTgetcol()
**	Private (static) routines defined:
**
** History:
**/

FUNC_EXTERN	FLDCOL	*FDfndcol();

/*{
** Name:	RTgetcol	-	Find a column descriptor
**
** Description:
**	Call FDfndcol to locate a column descriptor
**
** Inputs:
**	nm		Name of the column to locate
**
** Outputs:
**
** Returns:
**	Ptr to the FLDCOL for the column
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	21-jul-1983  -  written (jen)
**	05-jan-1987 (peter)	Changed RTfndcol to FDfndcol.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	
*/


FLDCOL	*
RTgetcol(tbl, nm)
reg	TBLFLD	*tbl;
reg	char	*nm;
{
	if (nm == NULL || *nm == EOS)
	{
		return (tbl->tfflds[tbl->tfcurcol]);
	}
	/*
	**	Search the list of fields.
	*/

	return (FDfndcol(tbl, nm));
}

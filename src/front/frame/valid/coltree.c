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
# include	<afe.h>

/*
**	COLTREE.c  -  return a FLDCOL pointer to the specified
**			field and column.
**	
**	Arguments:  column - the column name.
**		    fd	   - pointer to the tablefield.
**	
**	History:  29 Nov   1984 - written (bab)
**	04/03/87 (dkh) -Changed for 6.0.
**	11-sep-87 (bab)
**		Changed cast in call to FDfind from char** to nat**.
**	12/31/88 (dkh) - Perf changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


FUNC_EXTERN	FIELD	*FDfind();
FUNC_EXTERN	FLDHDR	*IIFDgchGetColHdr();



FLDCOL	*
vl_coltree(column, fd)
reg char	*column;		/* column name */
reg FIELD	*fd;			/* pointer to the alledged tablefield */
{
    	TBLFLD	*tbl;
    	FLDCOL	*col;

	tbl = fd->fld_var.fltblfld;

	col = (FLDCOL *) FDfind((i4 **) tbl->tfflds, column,
		tbl->tfcols, IIFDgchGetColHdr);

	return (col);
}

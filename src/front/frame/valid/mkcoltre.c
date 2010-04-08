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
# include	<ds.h>
# include	<feds.h>
# include	<st.h>
# include	<me.h>
# include	<ex.h>
# include	<er.h>
# include	"erfv.h"

/*
**	MKCOLTREE.c  -	Make a node in the validation tree
**			for a tablefield column.
**
**	Arguments:  table     -	 the tblfld name.
**		    column    -	 the column name.
**
**	History:  29 Nov   1984 - written (bruceb)
**	5-jan-1987 (peter)	Changed calls to IIbadmem.
**	04/07/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bruceb) Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	11-sep-87 (bruceb)
**		Changed cast in call to FDfind from char** to nat**.
**	19-apr-88 (bruceb)
**		When user error, not parser error, is responsible,
**		don't call vl_par_error().
**	06-jul-88 (sylviap)
**		Added calls to vl_nopar_error() if a user error.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	12/31/88 (dkh) - Perf changes.
**	17-may-89 (sylviap)
**		Changed parameter list to vl_nopar_error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF	FLDHDR	*IIFVflhdr;
GLOBALREF	bool	IIFVxref;

FUNC_EXTERN	FIELD	*FDfind();
FUNC_EXTERN	VTREE	*vl_mkattrtree();
FUNC_EXTERN	FLDHDR	*IIFDgchGetColHdr();

VTREE *
vl_mkcoltree(table, column)	/* MAKE TREE: */
reg char	*table;		/* table name */
reg char	*column;	/* column name */
{
	VTREE	*result;		/* result to pass back */
	VTREE	*left;			/* the table node */
	FIELD	*fd;
	TBLFLD	*tbl;
	FLDCOL	*col;
	FLDHDR	*hdr;

	left = vl_mkattrtree(table, vCOLSEL, &fd);

	left->v_data.v_fld = fd;

	if (fd->fltag != FTABLE)
	{
		vl_nopar_error(E_FV001A_Can_only_use, 0);
	}

	/* create a node */
	if ((result = (VTREE *)MEreqmem((u_i4)0, (u_i4)(sizeof(VTREE)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("MKCOLTREE"));
	}

	tbl = fd->fld_var.fltblfld;

	if ((col = (FLDCOL *) FDfind((i4 **) tbl->tfflds, column,
		tbl->tfcols, IIFDgchGetColHdr)) == NULL)
	{
		vl_nopar_error(E_FV001B_Bad_column_name, 2, 
			column, table);
	}

	hdr = &(col->flhdr);
	if (hdr != IIFVflhdr)
	{
		IIFVxref = TRUE;
	}

	result->v_data.v_oper = col->flhdr.fhseq;

	result->v_nodetype = vCOLSEL;		/* set the node type */
	result->v_left = left;	/* and assign the left and right branches */
	result->v_right.v_treright = NULL;

	return (result);
}

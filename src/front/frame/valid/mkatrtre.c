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
# include	<ex.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>
# include	<frserrno.h>

/*
**	MKATTRTREE.c -  Make a node in the validation tree
**			for an attribute.
**	
**	Arguments:  table    -	the tblfld name.
**		    nodetype -	vATTR if called from yyparse(),
**				else vCOLSEL if from mkcoltree().
**		    fld	     -	place to return pointer to field.
**	
**	History:  29 Nov   1984 - written (bab)
**		5-jan-1987 (peter)	Changed calls to IIbadmem.
**		6-jan-1987 (peter)	Change calls from RTfnd* to FDfnd*.
**	04/07/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	09/05/87 (dkh) - Changes for 8000 series error numbers.
**	09/12/87 (dkh) - Fixed jup bug 919.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	19-apr-88 (bruceb)
**		Removed an unnecessary call on IIUGfmt().
**	06-jul-88 (sylviap)
**		Added call to vl_nopar_error so error message will appear
**		on the validation correction frame.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF	FLDHDR	*IIFVflhdr;
GLOBALREF	bool	IIFVxref;

FUNC_EXTERN	FIELD	*FDfndfld();

VTREE *
vl_mkattrtree(table, nodetype, fld)	/* MAKE TREE: */
reg char	*table;		/* table name */
i4		nodetype;
FIELD		**fld;	/* used to return pointer to field "table" */
{
	VTREE	*result;		/* result to pass back */
	FIELD	*fd;
	FLDHDR	*hdr;
	bool	junk;

	/* create a node */
	if ((result = (VTREE *)MEreqmem((u_i4)0, (u_i4)(sizeof(VTREE)), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vl_mkattrtre"));
	}
	
	if ((fd = FDfndfld(vl_curfrm, table, &junk)) == NULL)
	{
		vl_nopar_error(VALNOFLD, 2, vl_curfrm->frname, table);
	}

	if (nodetype == vATTR)
	{
		result->v_data.v_fld = fd;
		hdr = &(fd->fld_var.flregfld->flhdr);
		if (hdr != IIFVflhdr)
		{
			IIFVxref = TRUE;
		}
	}
	else	/* called from v_mkcoltree */
	{
		*fld = fd;	/* pass back the field pointer */
	}

	result->v_nodetype = vATTR;		/* set the node type */
	result->v_left = NULL;	/* and assign the left and right branches */
	result->v_right.v_treright = NULL;

	return (result);
}

/*
**	chgfmt.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	chgfmt.c - Change format/datatype for a field.
**
** Description:
**	Given a field and a format string, change the format
**	and data type for the field to that given by the
**	passed in format string.  Needs to be changed for 6.0.
**	Routines defined in this file:
**	- FDchgfmt - Change format/datatype of field.
**
** History:
**	02/15/87 (dkh) - Added header.
**	06/24/87 (dkh) - Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/


# include	<compat.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 
# include	<st.h>
# include	"fdfuncs.h"
# include	<me.h>
# include	<er.h>


FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();



/*{
** Name:	FDchgfmt - Change format/datatype of field.
**
** Description:
**	Given the name of a field and format string, change the
**	format of the field to that specified in the format string.
**	The data type for the field may also changed as a result
**	of the format change.
**	This routine should only be used by GBF.  If used by
**	anyone else, the results are not guaranteed and AVs
**	can result.
**
** Inputs:
**	frm	Form that field is in.
**	name	Name of field to change format/datatype for.
**	fmtstr	Format string to use for the change.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	The datatype for the field is also changed at this point,
**	along with new allocation for data storage.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
**	03/04/87 (dkh) - Added support for ADTs.
*/
bool
FDchgfmt(frm, name, fmtstr)
FRAME	*frm;
char	*name;
char	*fmtstr;
{
	ADF_CB	*ladfcb;
	i4	fmtsize;
	i4	fmtlen;
	PTR	blk;
	FIELD	*fld;
	FLDTYPE	*type;
	FLDVAL	*val;
	i4	x;
	i4	y;
	bool	junk;

	fld = FDfndfld(frm, name, &junk);

	if (fld == NULL)
		return (FALSE);

	type = FDgettype(fld);
	val = FDgetval(fld);

	ladfcb = FEadfcb();

	if ((fmt_areasize(ladfcb, fmtstr, &fmtsize) == OK) &&
		((blk = MEreqmem((u_i4)0, (u_i4)fmtsize, TRUE,
		    (STATUS *)NULL)) != NULL) &&
		(fmt_setfmt(ladfcb, fmtstr, blk, &(type->ftfmt),
			&fmtlen) == OK))
	{
		;
	}
	else
	{
		return(FALSE);
	}

	if (fmt_ftot(ladfcb, type->ftfmt, val->fvdbv) != OK)
	{
		return(FALSE);
	}

	fld->flfmtstr = STalloc(fmtstr);

	if (fmt_size(ladfcb, type->ftfmt, val->fvdbv, &y, &x) != OK)
	{
		return(FALSE);
	}

	/*
	**  Allocate the storage space for the data buffers.
	*/
	if ((val->fvdbv->db_data = MEreqmem((u_i4)0,
	    (u_i4)val->fvdbv->db_length, TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("FDchgfmt"));
		return(FALSE);
	}


	/*
	**  This routine is very specific for running with GBF
	**  and should not be used by anyone else.  If it is
	**  the results are not guaranteed.  AVs may even occur.
	*/
	fld->flwidth = x;
	fld->fldataln = x;

	return (TRUE);
}

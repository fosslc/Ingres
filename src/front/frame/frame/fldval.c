# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 

/*
**  fldval.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "%W%	%G%";
**
**  History:	19-nov-85 (mgw) bug # 6599
**			This routine is called with 6 args, so I added disponly
**			as per "The FD and FT Interface" design spec in spite of
**			the fact that Marian and Dave H. already fixed this bug
**			by disallowing ^A's (auto dup) on table fields in
**			FTinsert().
**	03/05/87 (dkh) - Added support for ADTs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
i4
FDfieldval(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	return (FDputdata(frm, fldno, disponly, fldtype, col, row));
}

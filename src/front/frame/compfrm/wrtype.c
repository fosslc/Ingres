# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

/*
**  wrtype.c
**  write out a FLDTYPE
**  the heading has already been written
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  static char	Sccsid[] = "@(#)wrtype.c	30.2	2/4/85";
**	03/03/87 (dkh) - Added support for ADTs.
**	06/01/87 (dkh) - Sync up with frame changes.
**	11/12/87 (dkh) - Code cleanup.
**	21-jan-1998 (fucch01)
**	    Casted middle arg of DSwrite as type PTR to match paramater
**	    type, get rid of compile errors on sgi_us5.
*/


fcwrType(type)
register FLDTYPE	*type;
{
	DSwrite(DSD_SPTR, (PTR)type->ftdefault, 0);
	DSwrite(DSD_I4, (PTR)type->ftlength, 0);
	DSwrite(DSD_I4, (PTR)type->ftprec, 0);
	DSwrite(DSD_I4, (PTR)type->ftwidth, 0);
	DSwrite(DSD_I4, (PTR)type->ftdatax, 0);
	DSwrite(DSD_I4, (PTR)type->ftdataln, 0);
	DSwrite(DSD_I4, (PTR)type->ftdatatype, 0);
	DSwrite(DSD_I4, (PTR)type->ftoper, 0);
	DSwrite(DSD_SPTR, (PTR)type->ftvalstr, 0);
	DSwrite(DSD_SPTR, (PTR)type->ftvalmsg, 0);

	/*
	**  For 6.0, don't compile the validation tree.
	*/
	DSwrite(DSD_I4, 0, 0);				/* ftvalchk */
	DSwrite(DSD_SPTR, (PTR)type->ftfmtstr, 0);
	DSwrite(DSD_I4, 0, 0);				/* ftfmt */
	DSwrite(DSD_I4, 0, 0);				/* ftfu */
}

/*
**  wrval.c
**  write out a FLDVAL
**  the heading has already been written
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  static char	Sccsid[] = "@(#)wrval.c	30.2	2/4/85";
**	03/03/87 (dkh) - Added support for ADTs.
**	21-jan-1998 (fucch01)
**	    Casted val->fvdatay as type PTR to match paramater type.
**	    Gets rid of compile error on sgi_us5.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

fcwrVal(val)
register FLDVAL	*val;
{
	DSwrite(DSD_I4, 0, 0);				/* fvdbv */
	DSwrite(DSD_I4, 0, 0);				/* fvdsdbv */
	DSwrite(DSD_I4, 0, 0);				/* fvbufr */
	DSwrite(DSD_I4, 0, 0);				/* fvdatawin */
	DSwrite(DSD_I4, (PTR)val->fvdatay, 0);
}

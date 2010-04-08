# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  wrreg.c
**  write out a regular field's internals
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)wrreg.c	30.2	2/4/85";
**
**  History:
**	05/01/87 (dkh) - Deleted call to fcwrVtree. (dkh)
**	08/14/87 (dkh) - ER changes.
**	11/12/87 (dkh) - Code cleanup.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
*/


FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();


fcwrReg(fd, name)
register FIELD	*fd;
char		*name;
{
	FLDTYPE	*type;

	type = FDgettype(fd);
# ifdef IBM370
	DSinit(fcoutfp, fclang, DSA_C, name, DSV_LOCAL, ERx("const REGFLD"));
# else
	DSinit(fcoutfp, fclang, DSA_C, name, DSV_LOCAL, ERx("REGFLD"));
# endif /* IBM370 */
	fcwrHdr(FDgethdr(fd));
	fcwrType(type);
	fcwrVal(FDgetval(fd));
	DSfinal();
}

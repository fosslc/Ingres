/*
**  FTiofrm.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)ftsetio.c	1.1	1/23/85";
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

FTsetiofrm(frm)
FRAME	*frm;
{
	FTiofrm = frm;
}

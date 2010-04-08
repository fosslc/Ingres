/*
**  VTaddcomm
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)vtaddcomm.c	30.1	12/3/84";
**
**  9/21/84 -
**    Stolen from addcomm for FT. (dkh)
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"

VOID
VTaddcomm(command, operation, opflag)
u_char	command;
i4	operation;
i4	opflag;
{
    froperation[command] = (i2) operation;
    fropflg[command] = (i1) opflag;
}

/*
**  FTnewfrm
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)ftnewfrm.c	1.2	1/23/85";
**
**  History:
**	01/07/87 (dkh) - Deleted output of NULL ptr message as is should never
**			 happen.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	 <si.h>

STATUS
FTnewfrm(frm)
FRAME	*frm;
{
	if (frm == NULL)
	{
		return(FAIL);
	}

	return(OK);
}

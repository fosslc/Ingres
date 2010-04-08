/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	static	char	Sccsid[] = "@(#)vfexit.c	30.2	1/28/85";
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	 <ex.h> 

# ifndef FORRBF

GLOBALDEF	bool	vftkljmp = FALSE;

i4
vfexit(status)
i4	status;
{
	vftestclose();
	if (Vfeqlcall)
	{
		vftkljmp = TRUE;
		EXsignal(EXVFLJMP,0);
	}
	PCexit(status);
}
# endif

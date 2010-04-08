/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	static	char	Sccsid[] = "@(#)vfexit.c	30.2	1/28/85";
**
**	History:
**      25-sep-96 (mcgem01)
**              Global data moved to vfdata.c
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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

GLOBALREF	bool	vftkljmp;

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

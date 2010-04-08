/*
**  FTpanic
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <si.h> 


FTpanic(msg)
char	*msg;
{
	FTclose(NULL);
	SIfprintf(stdout, msg);
	PCexit(-1);
}

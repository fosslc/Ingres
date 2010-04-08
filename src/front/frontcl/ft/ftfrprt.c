/*
**  FTfrprint
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**  History:
**	01-oct-96 (mcgem01)
**	    extern changed to GLOBALREF to quieten compiler.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <termdr.h> 

GLOBALREF	i4	tdprform;

FTfrprint(val)
i4	val;
{
	tdprform = val;
}

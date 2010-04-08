/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h> 
# include	<ex.h> 

/*
**	static	char	Sccsid[] = "%W%	%G%";
**      24-sep-96 (hanch04)
**          Global data moved to data.c
*/

GLOBALREF i4	vl_help ;

STATUS
vl_except(exargs)
EX_ARGS	*exargs;
{
	if (exargs->exarg_num == EXVALJMP)
	{
		vl_help = FALSE;
		return (EXDECLARE);
	}
	return (EXRESIGNAL);
}

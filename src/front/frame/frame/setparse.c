/*
**	setparse.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	setparse.c - Set validation parsing mode.
**
** Description:
**	Control validation parsing/building.
**	Routines defined:
**	- FDsetparse - Set/reset validation building.
**
** History:
**	02/17/87 (dkh) - Added header.
**	25-jul-90 (bruceb)
**		Return the old value of vl_syntax.
**      24-sep-96 (hanch04)
**              Global data moved to framdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 



/*
** sets the parse to syntax only or actual build
*/

GLOBALREF	i4	FDparscol;
GLOBALREF	bool	vl_syntax;



/*{
** Name:	FDsetparse - Set/reset validation building.
**
** Description:
**	Routines sets a global (vl_syntax) that controls whether
**	validation strings should be parsed only or build a
**	validation execution tree as well.  A value of TRUE
**	means parsing only.
**
** Inputs:
**	val	Value to assign to "vl_syntax".
**
** Outputs:
**	Returns:
**		Previous value of vl_syntax.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
bool
FDsetparse(val)
bool	val;
{
	bool	oval = vl_syntax;

	vl_syntax = val;

	return(oval);
}

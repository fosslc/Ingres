/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
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
# include	<menu.h>
# include	<runtime.h>
# include	<rtvars.h>

/**
** Name:	iiretval.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIretval()
**	Private (static) routines defined:
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIretval	-	Return frame driver interrupt code
**
** Description:
**	Return the return value of IIrunfrm().  This is used in
**	figuring out which activate code to run.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	The current value in IIresult.
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## display f
**	## activate field f1 { code_1 }
**	## activate menuitem m1 { code_2 }
**	## activate scrollup t1 { code_3 }
**	## finalize
**
**	if (IIdispfrm("f","f") == 0) goto IIfdE1;
**	goto IImuI1;
**    IIfdB1:
**	while (IIrunform() != 0) {
**	  if (IIretval() == 1) { code_1 }
**	  else if (IIretval() == 2) { code_2 }
**	  else if (IIretval() == 3) { code_3 }
**	  else {
**  	    goto IIfdE1;
**	  } 
**	}
**   IImuI1:
**	add field f1 with value 1
**	add menu m1 with value 2
**	add scrollup t1 with value 3
**
** Side Effects:
**
** History:
**	
*/

i4
IIretval()
{
	return(IIresult);
}

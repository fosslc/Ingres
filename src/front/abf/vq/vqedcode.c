
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abclass.h>
# include       "ervq.h"


/**
** Name:	vqedcode.c - edit generated code
**
** Description:
**	This file defines:
**		IIVQegcEditGenCode  - implement user edit of gen'ed code.
**
** History:
**	10/01/89 (tom)	- created
**/

FUNC_EXTERN bool IIVQgcGenCheck();

FUNC_EXTERN VOID iiabFileEdit(); 

/*{
** Name:	IIVQegcEditGenCode	- Edit generated code
**
** Description:
**	This function invokes the edit of code associated with
**	frames and procedures. For the metaframe types.. we
**	will generate the code if it doesn't exist yet.. the
**	other types will just invoke vifred and do the default 
**	thing.
**
** Inputs:
**	OO_OBJECT *ao;	- application object that has the code attached
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	10/04/89 (tom) - created
*/
VOID
IIVQegcEditGenCode(ao)
OO_OBJECT *ao;
{

	switch (ao->class)
	{
	case OC_MUFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_BRWFRAME:
		if (IIVQgcGenCheck( (USER_FRAME*) ao) == FALSE)
		{
			return;
		}
		break;
	}

	iiabFileEdit( ((APPL_COMP*)ao)->appl, ao, ((USER_FRAME*)ao)->source);
}

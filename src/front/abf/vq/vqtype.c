
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>

# include	<metafrm.h>


/**
** Name:	vqtype.c -	return the type of a frame
**
** Description:
**	This file has routines to dynamically create the frame flow
**	diagram frame.
**
**	This file defines:
**
**	IIVQmffMakeFfFrame	make initial frame flow diagram frame
**
** History:
**	03/26/89 (tom) - created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIVQotObjectType - return the object type of the given object
**
** Description:
**	This function returns the type of the current metaframe.
**	It does this by decoding the class.
**
** Inputs:
**	OO_OBJECT *ao;
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04/13/89 (tom) - created
*/
i4
IIVQotObjectType(ao)
register OO_OBJECT *ao;
{
	register i4  type;

	switch(ao->class)
	{
				/* emerald project frames */
	case OC_MUFRAME:
		type = MF_MENU;
		break;

	case OC_APPFRAME:
		type = MF_APPEND;
		break;
	 
	case OC_UPDFRAME:
		type = MF_UPDATE;
		break;
	 
	case OC_BRWFRAME:
		type = MF_BROWSE;
		break;
	 
				/* traditional abf frames */
	default:
	case OC_OSLFRAME:
	case OC_OLDOSLFRAME:
		type = MF_USER;
		break;
	 
	case OC_RWFRAME:
		type = MF_REPORT;
		break;
	 
	case OC_QBFFRAME:
		type = MF_QBF;
		break;
	 
	case OC_GRFRAME:
	case OC_GBFFRAME:
		type = MF_GRAPH;
		break;
	 
	case OC_HLPROC:
		type = MF_HLPROC;
		break;

	case OC_OSLPROC:
		type = MF_OSLPROC;
		break;
	 
	case OC_DBPROC:
		type = MF_DBPROC; 
	}

	return (type);
}

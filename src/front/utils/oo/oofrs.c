/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<oodefine.h>
#include	<oosymbol.h>
#include	"ooldef.h"
#include	<ooproc.h>

/**
** Name:	oofrs.c -	Object Module Set-up Interactive Support Module.
**
** Description:
**	Contains the routine used to set-up interactive support for the object
**	module.  This simply replaces some methods with their interactive, FRS
**	based versions, so that non-interactive programs can be linked without
**	linking in the FRS.  (Of course, in a true OO environment this would
**	have been supported by sub-classing.)  Defines:
**
**	iiooInteractive()	set-up OO interactive support.
**
** History:
**	Revision 6.4  90/01  wong
**	Initial revision.
**	25-Aug-2009 (kschendel) 121804
**	    Need ooproc.h to satisfy gcc 4.3.
**/


static bool	_First = TRUE;

VOID
iiooInteractive ()
{
	if ( _First )
	{
		register OO_METHOD	*meth;
		OOID			methid;

		/* look up method starting at class */
		if ( (methid = iiooClook(O1, _confirmName, FALSE)) == nil )
		{ /* ran out of supers (i.e. at OC_OBJECT) */
			OOsndSelf(O1, _noMethod, _confirmName);
			return;
		}
		meth = (OO_METHOD*)OOp(methid);
		meth->entrypt = iiobConfirmName;

		_First = FALSE;
	}
}

/*
**	Copyright (c) 1987, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ilerror.h>
#include	"iamint.h"

/**
** Name:	iamabort.sc - transaction abort
**
** Description:
**	abort transaction for AOM.  Includes shutting off catalog writing.
**
** History:
**	Revision 6.2  89/03  wong
**	Changed to call 'IIUIabortXaction()'.
**
**	Revision 6.0  87/12  bobm
**	Initial revision.
*/

/*{
** Name:	abort_it - abort AOM transaction
**
** Description:
**	Performs cleanup of AOM transactions, returning failure status.
**	Internal AOM use only
**
** Return:
**	{STATUS}	ILE_FAIL	DB failure
**
** History:
**	12/87 (bobm)	written.  extracted from individual code in other
**			routines, added error catch for failed abort.
**	18-aug-88 (kenl)
**		Changed QUEL abort to SQL ROLLBACK.
**	03/89 (jhw)  Changed to call 'IIUIabortXaction()'.
*/

STATUS
abort_it ()
{
	IIUIabortXaction();

	iiuicw0_CatWriteOff();

	return ILE_FAIL;
}

/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/
#include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include "irstd.h"

/**
** Name:        irfreeil.c -      ILRF Free IL from memory
**
** Description:
**      Retrieves code for a frame.  Defines:
**
**      IIORfcFixCurrent()	fix current frame (obsolete)
**	IIORucUnfixCurrent()	unfix current frame (obsolete)
**	IIORfiFreeIL()		free frame (not called)
**	IIORfaFreeAllframes()	free frame (called from irfget.c)
**
** History:
**      01-jun-92 (davel)
**              added history section; disallowed freeing the current frame
**		in IIORfaFreeAllframes() - fix for bug 44444.  Obsolesce
**		the code that conditionally handled allowing the current
**		frame to be freed (this is a bad bad idea).
*/

static	i4 IL_trycurrent	ZERO_FILL;

/*
**  the following three functions are no longer needed, and will be removed
**  entirely soon
*/
IIORfcFixCurrent()
{
	IL_trycurrent = 0;
}

IIORucUnfixCurrent()
{
	IL_trycurrent = 1;
}

IIORfiFreeIL(irblk)
IRBLK *irblk;
{
	if (IL_trycurrent)
		/* try non-current, try current if only one left */
		return (IIORfaFreeAllframes(irblk));
	else
		/* try non-current only */
		return (IIORmfMemFree(irblk, IORM_SINGLE, NULL));
}		

/*
** IIORfaFreeAllframes() - used by irfget.c to free memory when the frames-in-
** memory limit is reached.  No longer try to free the current frame ever.
** Return error when no more to free.
*/
STATUS
IIORfaFreeAllframes(irblk)
IRBLK *irblk;
{
	if (IIORmfMemFree(irblk, IORM_SINGLE, NULL) == OK)
		return (OK);

	/* we REALLY don't have room */
	return (ILE_NOMEM);
}

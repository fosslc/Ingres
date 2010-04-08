/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/
#include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include "irstd.h"

/*
** unlink a frame from its list. This routine returns the frame to
** the free pool, making it invalid for further use.
*/

GLOBALREF FREELIST *IIOR_Pool;	/* allocation pool */

list_unlink(frm)
IR_F_CTL *frm;
{
	QUremove(&frm->fi_queue);
	FElpret(IIOR_Pool, frm);
}

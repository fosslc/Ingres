/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <graf.h>

/**
** Name:	gtrestrict.c -	Graphics Restrict.
**
** Description:
**      <comments>.  This file defines:
**
**	GTrestrict()
**
** History:
**		Revision 40.102  86/04/09  13:21:58  bobm
**		LINT changes
**		
**		Revision 40.101  86/02/27  10:24:33  wong
**		Changed to use 'G_restrict'.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern GR_FRAME	*G_frame;	/* Graphics frame structure */

extern bool	G_restrict;	/* Graphics restrict locator flag */

extern i4  G_reflevel;

VOID
GTrestrict (flag, level, ext)
bool flag;		/* turn restriction on/off */
i4  level;		/* preview level */
EXTENT *ext;		/* screen limit (FALSE only) */
{
	G_reflevel = G_frame->preview;
	G_frame->preview = level;

	if (flag)
	{
		G_frame->lim = &((G_frame->cur)->ext);
		G_restrict = TRUE;
		GTrsbox(TRUE,'+');
	}
	else
	{
		GTrsbox(FALSE);
		G_frame->lim = ext;
		G_restrict = FALSE;
	}
}

/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"

/**
** Name:    <filename>    - <short comment>
**
** Description:
**      <comments>.  This file defines:
**
**	GTlocator()	set use of graphics locator device.
**	GTcolors()	return number of available device colors.
**	GTmref()	toggle menu reference.
**
** History:
**	85/09/30  15:05:03  bobm
**	Initial revision.
**		
**	86/02/27  10:54:08  wong
**	Changed locator function to set 'G_locate'.
**		
**	86/04/09  13:19:37  bobm
**	LINT changes
**
**	23-oct-86 (bab)
**		Remove spurious extern of G_frame.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern Gdefloc	*G_dloc;	/* GKS locator device */

extern bool	G_locate;	/* Graphics use locator flag */
extern bool G_mref;
extern char *G_tc_cm;
FUNC_EXTERN i4  GTchout();

bool
GTlocator (use_locator)
bool	use_locator;
{
	G_locate = G_dloc != NULL && use_locator;

	return G_dloc != NULL;
}

i4
GTcolors ()
{
	return (G_cnum);
}

bool
GTmref ()
{
	if (G_mref)
	{
		G_mref = FALSE;
		return (TRUE);
	}
	return (FALSE);
}

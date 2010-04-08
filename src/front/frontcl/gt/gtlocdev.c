/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"

/**
** Name:    gtlocdev.c - locator device
**
** Description:
**	locator device (mouse) handler
**
** History:
**		Changed record input to ginlc to be valid. 1/89 Mike S
**
**		Revision 40.101	 86/04/09  13:20:04  bobm
**		LINT changes
**
**		Revision 40.100	 86/02/26  17:45:24  sandyd
**		BETA-1 CHECKPOINT
**
**		Revision 40.7  86/02/03	 12:12:01  bobm
**		reorder gkscomp.h / cchart.h / gks.h interactions for VMS
**
**		Revision 40.6  86/01/17	 14:34:18  bobm
**		fix to work with new drivers.
**
**		Revision 40.5  86/01/06	 23:59:52  wong
**		Put back external declarations of 'G_ws' and 'G_dloc'.
**
**		Revision 40.4  86/01/06	 20:08:40  wong
**		Changed to use "GT_DEVICE G_dev".
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern Gws	*G_ws;		/* GKS workstation pointer (referenced) */
extern Gdefloc	*G_dloc;	/* GKS locator device (referenced) */

extern i4	(*G_mfunc)();	/* Mode switch function pointer */

extern bool	G_mref;		/* menu refresh flag */

GTlocdev (x, y, ext, mflag)
float *x,*y;
EXTENT *ext;
bool mflag;
{
	Gqloc *loc;
	Gqloc * grqlc();
	Gloc loc1;
	i4 rc;

	if (G_dloc == NULL)
		return (-1);

	if (mflag)
		GTmsg (ERget(F_GT0012_Alpha_Cursor));

	GTxtodrw (*x,*y,x,y);
	loc1.transform = 1;
	loc1.position.x = *x;
	loc1.position.y = *y * G_aspect;

	(*G_mfunc)(LOCONMODE);
	ginlc (G_ws, 1, &loc1, 1, &(G_dloc->e_area), &(G_dloc->record) );
	gslcm (G_ws,1,REQUEST,ECHO);
	for(;;)
	{
		loc =  grqlc(G_ws,1);
		if (loc->status == NONE)
		{
			rc = 1;
			break;
		}

		loc->loc.position.y /= G_aspect;
		GTxfrdrw ((float) loc->loc.position.x,
					(float) loc->loc.position.y,x,y);

		/* check for "menu" area */
		if (*y < 0.0 && mflag)
		{
			rc = 1;
			break;
		}

		if (*x >= ext->xlow && *x <= ext->xhi && *y >= ext->ylow && *y <= ext->yhi)
		{
			rc = 0;
			break;
		}

		(*G_mfunc)(LOCOFFMODE);
		GTmsg (ERget(F_GT0013_No_point));
		(*G_mfunc)(LOCONMODE);
		gslcm (G_ws,1,REQUEST,ECHO);
	}
	(*G_mfunc) (LOCOFFMODE);
	if (mflag)
	{
		FTgtrefmu();
		G_mref = 0;
	}
	return (rc);
}

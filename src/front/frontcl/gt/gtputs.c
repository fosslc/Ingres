/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"

/**
** Name:	gtputs.c -	Graphics System GKS Output String Routine.
**
** Description:
**	Contains a routine used to output strings from the Graphics System to
**	the GKS device.
**
**	GTputs()	output string to GKS device.
**
** History:
**		Revision 40.0  86/04/15  14:21:18  wong
**		Initial revision.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern Gws	*G_ws;		/* GKS workstation pointer */

/*{
** Name:	GTputs() -	Output String to GKS Device.
**
** Description:
**	Graphics System interface to VE GKS escape to output string to GKS
**	device.  This routine exists for those routines that cannot intermix
**	GKS calls with SI calls.
**
** Inputs:
**	str		string to output.
**
** History:
**	04/86 (jhw) -- Written.
*/

void
GTputs (str)
char	*str;
{
	Gescstring	gks_dev;

	i4	Vstring();

	gks_dev.ws = G_ws;
	gks_dev.s = str;
	gesc(Vstring, &gks_dev);
}

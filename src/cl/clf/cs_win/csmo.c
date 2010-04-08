/*
** Copyright (c) 1987, 2004 Ingres Corporation
*/

# include <compat.h>
# include <sys\types.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <tr.h>

# include "csmgmt.h"

/**
** Name:	csmo.c	- main entry for UNIX CS MO stuff
**
** Description:
**	Defines the main classdef routine for the UNIX CSMO system.
**
**	CS_mo_init	- define CSMO classes.
**
** History:
**	29-Oct-1992 (daveb)
**	    documented
**	18-Nov-1992 (daveb)
**	    log init failures.
**	13-Jan-1993 (daveb)
**	    init mon objects.
**	14-may-97 (mcgem01)
**	    Remove class.h and add a GLOBALREF for CS_classes here
**	    to facilitate the Jasmine build.
** 	14-dec-1998 (wonst02)
** 	    Add definitions for cssampler objects.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Remove MOclassdef() call for CS_sem_classes as we've
**	    not kept objects for semaphores for a long time.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**/

GLOBALREF MO_CLASS_DEF    CS_classes[];

/*{
** Name:	CS_mo_init	- register all CSMO classes.
**
** Description:
**	Defines all the CSMO classes.  To be called once out of
**	CSinitiate.  At present does not define the
**	untested CS_int_classes and CS_mon_classes.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none.
**
** History:
**	29-Oct-1992 (daveb)
**	    documented
**	18-Nov-1992 (daveb)
**	    log init failures.
**	13-Jan-1993 (daveb)
**	    init mon objects.
**	10-Aug-1995 (shero03)
**	    fix up TRdisplay typos
**	24-jul-2000 (somsa01 for jenjo02)
**	    Remove MOclassdef() call for CS_sem_classes as we've
**	    not kept objects for semaphores for a long time.
*/

VOID
CS_mo_init(void)
{
	STATUS          cl_stat;
	cl_stat = MOclassdef(MAXI2, CS_scb_classes);
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x on CS_scb_classes\n", cl_stat, cl_stat);

	cl_stat = MOclassdef(MAXI2, CS_cnd_classes);
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x on CS_cnd_classes\n", cl_stat, cl_stat);
	cl_stat = MOclassdef(MAXI2, CS_classes);
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x on CS_classes\n", cl_stat, cl_stat);
	cl_stat = MOclassdef(MAXI2, CS_mon_classes);
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x on CS_mon_classes\n", cl_stat, cl_stat);
	cl_stat = MOclassdef(MAXI2, CS_samp_classes );
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x from CS_samp_classes\n", cl_stat, cl_stat);
	cl_stat = MOclassdef(MAXI2, CS_samp_threads );
	if (cl_stat != OK)
		TRdisplay("cl_stat %d %x from CS_samp_threads\n", cl_stat, cl_stat);
}

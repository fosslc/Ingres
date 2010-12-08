/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>

# include <csinternal.h>
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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	09-apr-1997 (kosma01)
**	    correct names of classes on TRdisplays
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**/

/* data */

MO_CLASS_DEF CS_classes[] =
{
    /* Nothing here... */
    { 0 }
};


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
**	15-Nov-1999 (jenjo02)
**	    Remove MOclassdef() call for CS_sem_classes as we've
**	    not kept objects for semaphores for a long time.
*/

VOID
CS_mo_init(void)
{
    STATUS cl_stat;
    cl_stat = MOclassdef( MAXI2, CS_scb_classes );
    if( cl_stat != OK )
	TRdisplay( "cl_stat %d %x on CS_scb_classes\n", cl_stat, cl_stat );
    cl_stat = MOclassdef( MAXI2, CS_cnd_classes );
    if( cl_stat != OK )
	TRdisplay( "cl_stat %d %x on CS_cnd_classes\n", cl_stat, cl_stat );
    cl_stat = MOclassdef( MAXI2, CS_classes );
    if( cl_stat != OK )
	TRdisplay( "cl_stat %d %x on CS_classes\n", cl_stat, cl_stat );
    cl_stat = MOclassdef( MAXI2, CS_mon_classes );
    if( cl_stat != OK )
	TRdisplay( "cl_stat %d %x on CS_mon_classes\n", cl_stat, cl_stat );
}



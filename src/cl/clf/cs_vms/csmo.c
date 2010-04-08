/*
** Copyright (c) 1987, 1992 Ingres Corporation
*/

# include <compat.h>
#include    <gl.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>

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
**      16-jul-93 (ed)
**	    added gl.h
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**/

/* data */

static MO_CLASS_DEF CS_classes[] =
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
**	    log init failures.
**	13-Jan-1993 (daveb)
**	    init mon objects.
**	18-Jan-1993 (daveb)
**	    something's wrong with CS_mon_classes; don't do for now.
*/
VOID
CS_mo_init(void)
{
    (void) MOclassdef( MAXI2, CS_scb_classes );
    (void) MOclassdef( MAXI2, CS_sem_classes );
    (void) MOclassdef( MAXI2, CS_cnd_classes );
    (void) MOclassdef( MAXI2, CS_classes );
/*    (void) MOclassdef( MAXI2, CS_mon_classes ); */
}



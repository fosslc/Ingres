/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

/**
**
**  Name: LKSIZE.C - Utility routines for use by iishowmem
**
**  Description:
**      Utility routines for use by iishowmem, the utility program which prints
**	out the lg/lgk memory requirements.  iishowmem is used by the 
**	install support scripts to configure the machine and logging/locking
**	parameters.
**
**	This file has been created to only contain modules not needed by the
**	"mainline" product.  This module is currently not linked into the 
**	mainline product (so be careful when adding modules so as to not require
**	this extra code in the mainline deliverables).
**
**          LK_size() - Return "sizeof" given "lk" objects.
**
**  History:
**      15-feb-90 (mikem)
**	    created.
**	29-sep-1992 (bryanp)
**	    Ported to the new LG/LK system.
**	24-may-1993 (bryanp)
**	    Resource blocks are now separate from lock blocks.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**      23-Oct-2002 (hanch04)
**          Changed memory size variables to use a SIZE_TYPE to allow
**          memory > 2 gig
**/


/*{
** Name: LK_size()	- Return "sizeof" given "lk" objects.
**
** Description:
** 	Return "sizeof" given "lk" objects.  Given the lgkshare.h defined
**      identifier for the object, this routine will return the size of that
**	object.
**	
**	This routine would not be needed if the LG/LK maintained data structures
**	were not local to their respective modules.
**
** Inputs:
**	object				identifier for the object:
**						LGK_OBJ0_LK_LLB
**						LGK_OBJ1_LK_LLB_TAB
**						LGK_OBJ2_LK_BLK
**						LGK_OBJ3_LK_BLK_TAB
**						LGK_OBJ4_LK_LKH
**						LGK_OBJ5_LK_RSH
**						LGK_OBJB_LK_RSB
**						LGK_OBJC_LK_RSB_TAB
**						LGK_OBJF_LK_MISC
**
**
** Outputs:
**      size                        	size of the object, if one of the 
**					know objects, else it return 0 size.
**
**	Returns:
**	    VOID;
**
** History:
**      18-feb-90 (mikem)
**	    created.
**	29-sep-1992 (bryanp)
**	    Ported to the new LG/LK system.
**	24-may-1993 (bryanp)
**	    Resource blocks are now separate from lock blocks.
**	21-Mar-2000 (jenjo02)
**	    Added LGK_OBJF_LK_MISC
**	25-Aug-2006 (jonj)
**	    *_TAB sizes are SIZE_TYPE offsets, not i4.
*/
VOID
LK_size(i4 object, SIZE_TYPE *size)
{
    *size = 0;

    switch (object)
    {
	case LGK_OBJ0_LK_LLB:
	    *size = sizeof(LLB);
	    break;

	case LGK_OBJ1_LK_LLB_TAB:
	    *size = sizeof(SIZE_TYPE);
	    break;

	case LGK_OBJ2_LK_BLK:
	    *size = sizeof(LKB);
	    break;

	case LGK_OBJ3_LK_BLK_TAB:
	    *size = sizeof(SIZE_TYPE);
	    break;

	case LGK_OBJ4_LK_LKH:
	    *size = sizeof(LKH);
	    break;

	case LGK_OBJ5_LK_RSH:
	    *size = sizeof(RSH);
	    break;

	case LGK_OBJB_LK_RSB:
	    *size = sizeof(RSB);
	    break;

	case LGK_OBJC_LK_RSB_TAB:
	    *size = sizeof(SIZE_TYPE);
	    break;

	case LGK_OBJF_LK_MISC:
	    *size = sizeof(LKD);
	    break;

	default:
	    *size = 0;
	    break;
    }

    return;
}

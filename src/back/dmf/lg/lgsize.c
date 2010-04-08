/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGSIZE.C - Utility routines for use by iishowmem
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
**          LG_size() - Return "sizeof" given "lg" objects.
**
**  History:
**      15-feb-90 (mikem)
**	    created.
**	29-sep-1992 (bryanp)
**	    Ported to the new LG/LK system.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-Mar-2000 (jenjo02)
**	    Corrected size of LXB_TAB and LDB_TAB, added
**	    LGK_OBJD_LG_LBB, LGK_OBJE_LG_MISC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Oct-2002 (hanch04)
**          Changed LG_size to use a SIZE_TYPE to allow
**          memory > 2 gig
**/


/*{
** Name: LG_size()	- Return "sizeof" given "lg" objects.
**
** Description:
** 	Return "sizeof" given "lg" objects.  Given the lgkshare.h defined
**      identifier for the object, this routine will return the size of that
**	object.
**	
**	This routine would not be needed if the LG/LK maintained data structures
**	were not local to their respective modules.
**
** Inputs:
**	object				identifier for the object:
**					    LGK_OBJ6_LG_BUFFER
**					    LGK_OBJ7_LG_LXB
**					    LGK_OBJ8_LG_LXB_TAB
**					    LGK_OBJ9_LG_LDB
**					    LGK_OBJA_LG_LDB_TAB
**					    LGK_OBJD_LG_LBB
**					    LGK_OBJE_LG_MISC
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
**	21-Mar-2000 (jenjo02)
**	    Corrected size of LXB_TAB and LDB_TAB, added
**	    LGK_OBJD_LG_LBB, LGK_OBJE_LG_MISC.
**	25-Aug-2006 (jonj)
**	    *_TAB sizes are SIZE_TYPE offsets, not i4.
**	    Add another to LXB_TAB for (new) LFD type.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add a u_i4 to LG_LXB_TAB
**	    to alloc memory for the ldb_xid_array.
*/
VOID
LG_size(i4 object, SIZE_TYPE *size)
{
    *size = 0;

    switch (object)
    {
	case LGK_OBJ7_LG_LXB:
	    *size = sizeof(LXB);
	    break;

	/*
	** Each "LXB table" includes an equal number of
	** slots for LXB, LPB, LPD, LFD, a LXB hash bucket,
	** and a u_i4 for the MVCC xid_array.
	*/
	case LGK_OBJ8_LG_LXB_TAB:
	    *size = (4 * sizeof(SIZE_TYPE)) + sizeof(LXBQ)
	    		+ sizeof(u_i4);
	    break;

	case LGK_OBJ9_LG_LDB:
	    *size = sizeof(LDB);
	    break;

	/*
	** Each "LDB table" includes an equal number of
	** slots for LFB and a LDB.
	*/
	case LGK_OBJA_LG_LDB_TAB:
	    *size = 2 * sizeof(SIZE_TYPE);
	    break;

	case LGK_OBJD_LG_LBB:
	    *size = sizeof(LBB);
	    break;

	case LGK_OBJE_LG_MISC:
	    *size = sizeof(LGD) + sizeof(LFB);
	    break;

	default:
	    *size = 0;
	    break;
    }

    return;
}

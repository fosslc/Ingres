/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPDMFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <cs.h>
#include    <er.h>
#include    <tr.h>
#include    <pm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <lgkdef.h>

/*
** Name:	lgkdata.c
**
** Description:	Global data for lgk facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	18-apr-2001 (devjo01)
**	    Added LGK_my_PID (s103715).
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
*/

/* lgkm.c */

GLOBALDEF LGK_BASE  LGK_base	ZERO_FILL;   /* information about LG 
                                             ** and LK memory pool.
                                             */  
GLOBALDEF PID	    LGK_my_pid;  	     /* "self" PID used by LG/LK */

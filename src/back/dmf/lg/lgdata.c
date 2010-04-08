/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPDMFLIBDATA
**
*/

#include    <compat.h>
#include    <cs.h>
#include    <gl.h>
#include    <di.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <lg.h>
#include    <lgdef.h>

/*
** Name:	lgdata.c
**
** Description:	Global data for lg facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	18-Dec-1997 (jenjo02)
**	    Added LG_my_pid.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Changed Lg_di_cbs type from DI_IO to LG_IO.
**	24-Aug-1999 (jenjo02)
**	    Added LG_is_mt.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-aug-2001 (kinte01)
**	    add cs.h to resolve missing type definitions(CS_SID,etc) on VMS
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
*/

/* lgopen.c */
GLOBALDEF    LG_IO   Lg_di_cbs[2];
GLOBALDEF    u_i4    LG_logs_opened = 0;
GLOBALDEF    PID     LG_my_pid	ZERO_FILL;
GLOBALDEF    bool    LG_is_mt = FALSE;

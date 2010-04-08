/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPLIBQLIBDATA
**
*/

# include <compat.h>
# include <lo.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>

/*
** Name:        uidata.c
**
** Description: Global data for ui facility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPLIBQLIBDATA
**	    in the Jamfile.
*/

GLOBALDEF PTR   clienttable;
GLOBALDEF PTR   moduletable;
GLOBALDEF char  DD_dev[MAX_LOC];
GLOBALDEF char  DD_path[MAX_LOC];

/* uisavept.qsc */
GLOBALDEF       char    stmt_buffer[FE_MAXNAME + 20];

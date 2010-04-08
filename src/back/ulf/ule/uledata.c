/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <uleint.h>

/*
** Name:	uledata.c
**
** Description:	Global data for ule facility.
**
** History:
**	17-oct-2000 (somsa01)
**	    Created.
**	19-oct-2000 (hanch04)
**	    Removed unneed header files
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPULFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPULFLIBDATA
**
*/

/* uleformat.c */
GLOBALDEF ULE_MHDR Ule_mhdr;	/* `dm_cname_max::[server_name, session_id]:' */
GLOBALDEF i4	   Ule_started = 0;	 /* did we fill in the above */


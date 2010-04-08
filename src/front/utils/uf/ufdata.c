/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
/*
** Name:        ufdata.c
**
** Description: Global data for uf facility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

GLOBALDEF VOID  (*iiuftfdDiagFunc)();

/*
** Copyright (c) 2004 Ingres Corporation
*/
 
# include       <compat.h>

/*
** Name:        data.c
**
** Description: Global data for  facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

GLOBALDEF bool  IITBccvChgdColVals = FALSE;

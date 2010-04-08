/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
/*
** Name:        tbldata.c
**
** Description: Global data for tblutil facility.
**
** History:
**
**      26-sep-96 (mcgem01)
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

GLOBALDEF       bool    IituMainform = TRUE ;

/* util.qsc */

GLOBALDEF       bool    second_pass_emode = FALSE;

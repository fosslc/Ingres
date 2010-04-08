/*
** Copyright (c) 2004 Ingres Corporation
*/
 
# include	<compat.h>

/*
** Name:        cgcdata.c
**
** Description: Global data for  cgc facility.
**
** History:
**
**      09-jan-98 (mcgem01)
**          Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
*/

/*
**
LIBRARY = IMPEMBEDLIBDATA
**
LIBOBJECT = cgcdata.c
**
*/

GLOBALDEF PTR   IIgca_cb = NULL;

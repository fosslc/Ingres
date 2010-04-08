/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include       <compat.h>
# include       "gkscomp.h"

/*
** Name:	gtdata.c
**
** Description:	Global data for gt facility.
**
** History:
**
**	24-sep-96 (mcgem01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* gtdatset.c */

GLOBALDEF Gint  GTmap_axis[4] = { GLEFT, GRIGHT, GUP, GDOWN };


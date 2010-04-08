/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include       <compat.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       "vtframe.h"

/*
** Name:	vtdata.c
**
** Description:	Global data for vt facility.
**
** History:
**
**	24-sep-96 (mcgem01)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* vtcursor.c */
GLOBALDEF       i4      VTgloby = 0;
GLOBALDEF       i4      VTglobx = 0;

/* vtundo.c */
GLOBALDEF       WINDOW  *unLast  ZERO_FILL;
GLOBALDEF       WINDOW  *unImage ZERO_FILL;

/* vtxydraw.c */
GLOBALDEF       i4      IIVTcbegy = 0;
GLOBALDEF       i4      IIVTcbegx = 0;

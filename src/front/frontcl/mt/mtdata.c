/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <termdr.h>

/*
** Name:	mtdata.c
**
** Description:	Global data for mt facility.
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

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/* mtinit.c */

GLOBALDEF       WINDOW  *titlscr = NULL;
GLOBALDEF       WINDOW  *dispscr = NULL;
GLOBALDEF       WINDOW  *statscr = NULL;
GLOBALDEF       WINDOW  *bordscr = NULL;
GLOBALDEF       i4      (*MTputmufunc)() = NULL;
GLOBALDEF       VOID    (*MTdmpmsgfunc)() = NULL;
GLOBALDEF       bool    (*MTfsmorefunc)() = NULL;
GLOBALDEF       VOID    (*MTdmpcurfunc)() = NULL;
GLOBALDEF       VOID    (*MTdiagfunc)() = NULL;

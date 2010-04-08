/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	abut.h	- definitions needed by users of abut.c
**
** Description:
**	Miscellaneous things used by IIUT routines
**
** History:
**	7/90 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define E_ObjNoSrc      -2	/* Object is present, but no source */
#define E_NoComp      	-3	/* Compilation is undefined for this component*/
#define E_BadComp      	-4	/* User chose not to recompile a "bad" frame */

/* Arguments for IIUT routines */
typedef struct {
                char    *cmd_line;
                i4      arg_cnt;
                PTR     args[10];
} UTARGS;



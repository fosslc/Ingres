/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	curtc.h	  -	Structure defintions for rectype to curecord
**
** Description:
**	This file contains the typedef for the structure
**	CURTC which is used to go from a rectype to a CURECORD
**
** History:
**	3-jul-1987 (Joe)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	CURTC		- rectype to CURECORD structure.
**
** Description:
**	This structure is used to associate a rectype with a CURECORD.
**
** History:
**	3-jul-1987 (Joe)
**		Initial Version
*/
typedef struct
{
    i4		curectype;		/* The rectype */
    CURECORD	*curec;
} CURTC;

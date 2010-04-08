/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	cuntr.h	  -	Structure defintions for name to rectype.
**
** Description:
**	This file contains the typedef for the structure
**	CUNTR which is used to go from a name to a rectype.
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
** Name:	CUNTR		- Name to rectype structure.
**
** Description:
**	This structure is used to associate a name with a rectype.
**
** History:
**	3-jul-1987 (Joe)
**		Initial Version
*/
typedef struct
{
    i4		curectype;		/* The rectype */
    char	*cuname;		/* The name for this rectype in files*/
    i4		cunmlen;		/* The length of the name. */
    ER_MSGID	cunmid;			/* Id of name of object.(always fast) */
} CUNTR;

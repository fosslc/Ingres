/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	trimtxt.h	- define TRIMTXT typedef
**
** History:
**	8/25/89 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	TRIMTXT	- linked list of trim
**
** Description:
**	Linked list of trim
**
** History:
**	8/25/89 (Mike S) Initial version
*/
typedef struct _trimtxt
{
	i4	row;		/* Row number */
	i4	column;		/* Column number */
	char	*string;	/* Trim string */
	struct _trimtxt *next;	/* Forward pointer */
} TRIMTXT;

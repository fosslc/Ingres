/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vqaclist.h - Define strucutre for application component list
**
** History:
**	12/13/89 (Mike S)	Initial version
**/

/*}
** Name:	AC_LIST - Reconciliation frames
**
** Description:
**	Each component has an indicator of whether to use it.
**
** History:
**	12/13/89 (Mike S)       Initial version	
*/
typedef struct
{
	APPL_COMP	*comp;		/* Application component */
	bool		used;		/* whether to use it */
	PTR		utility;	/* Utility pointer */
} AC_LIST;

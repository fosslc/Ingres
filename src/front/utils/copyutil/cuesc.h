/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:	cuesc.h		- Structure definition for extended system
**								catalogs
**
** Description:
**	This file contains the definition of the structure CUESC which holds
**	details of extended system catalogs.
**
** History:
**	14-Jun-91 (blaise)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name:	CUESC		- Extended system catalogs structure.
**
** Description:
**	This structure is used to hold the names of extended system catalogs.
**	Since these catalogs are optional copyapp should only give a warning,
**	not an error, if it is copying an application from a database which
**	has them to a database which does not.
*/

typedef struct
{
	i4		cuescid;		/* Id of catalog */
	char		*cuescname;		/* Name of catalog */
	ER_MSGID	cuescmsg;		/* Warning message */
} CUESC;

/*
** System catalog ids
*/

/* The following are all VISION-ONLY system catalogs */

# define	CUT_FRAMEVARS		0x1		/* ii_framevars */
# define	CUT_MENUARGS		0x2		/* ii_menuargs */
# define	CUT_VQJOINS		0x4		/* ii_vqjoins */
# define	CUT_VQTABCOLS		0x8		/* ii_vqtabcols */
# define	CUT_VQTABLES		0x10		/* ii_vqtables */

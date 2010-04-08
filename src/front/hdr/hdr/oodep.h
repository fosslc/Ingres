/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	oodep.h -	ABF Dependency Types.
**
** Description:
**	Defines the object dependency types used by ABF (in the
**	"ii_abfdependencies" catalog.)
**
** History:
**	Revision 6.0  88/01/01  bobm
**	Initial revision (moved from "ooclass.qsh.")
*/

# define	OC_DTMEMBER	3501	/* Dependency Type: member of */
# define	OC_DTDBREF	3502	/* Dependency Type: DB reference */
# define	OC_DTCALL	3503	/* Dependency Type: call */
# define	OC_DTRCALL	3504	/* Dependency Type: rcall */
# define	OC_DTMREF	3505	/* Dependency Type: menu reference */
# define	OC_DTGLOB	3506	/* Dependency Type: global var */
# define	OC_DTTYPE	3507	/* Dependency Type: type definition */
# define	OC_DTTABLE_TYPE	3508	/* type definition, table-based */
# define	OC_DTFORM_TYPE	3509	/* type definition, form-based */
# define	OC_DTTFLD_TYPE	3510	/* type definition, tablefield-based */


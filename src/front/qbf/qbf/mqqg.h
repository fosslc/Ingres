/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	mqqg.h     -    Defines a structure to use with qg.
**
** Description:
**	Defines the data structure MQQG_GEN.
**	This structure is pointed to by a QS_VALGEN QRY_SPEC.
**	It is used by bldwhere.  It contains a pointer to
**	the QDEF, and an indication of whether the query is
**	being built for the master or the detail query.
**
** History:
**	13-may-1987 (Joe)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	MQQG_GEN	- Structure for QS_VALGEN QRY_SPEC.
**
** Description:
**	This structure is pointed to by a QS_VALGEN QRY_SPEC.
**	The structure will get passed to bldwhere by IIQG_generate
**	when it needs to build the where clause for the query.
**	bldwhere uses the MQQDEF pointed to by this structure to
**	build its query.  It also needs to know which where clause,
**	the master or the detail, is being built.
**
** History:
**	13-may-1987 (Joe)
**		Initial Version
*/
typedef struct
{
	i4		mqqg_where;		/* Which where clause */
#	define MQQG_MASTER	1
#	define MQQG_DETAIL	2
	MQQDEF		*mqqg_qdef;		/* The QDEF */
} MQQG_GEN;

/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef RFDINT_H_INCLUDED
# define RFDINT_H_INCLUDED
# include <compat.h>
# include "tblinfo.h"

/**
** Name:	rdfint.h - RepServer RDF internal structures
**
** Description:
**	Defines a RepServer RDF-like Server Control Block.
**
** History:
**	14-jan-97 (joea)
**		Created based on rdfint.h in replicator library.
**	20-may-98 (joea)
**		Add GLOBALREF for RSrdf_svcb.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct tagRS_RDI_SVCB
{
	i4	num_tbls;		/* number of registered tables */
	i4	num_cols;		/* number of registered columns */
	RS_TBLDESC	*tbl_info;	/* array of table descriptors */
	RS_COLDESC	*col_info;	/* array of column descriptors */
} RS_RDI_SVCB;


GLOBALREF RS_RDI_SVCB RSrdf_svcb;


# endif

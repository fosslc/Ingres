/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include "rdfint.h"
# include "tblinfo.h"

/**
** Name:	rdfgdesc.c - request description of a table
**
** Description:
**	This file contains the routines for requesting relation, attributes
**	and index information for a table.
**
**		RSrdf_gdesc	- request description of a table
**
**  History:
**	29-jan-97 (joea)
**		Created based on rdfgdesc.c in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF RS_RDI_SVCB RSrdf_svcb;


/*{
** Name:	RSrdf_gdesc	- request description of a table
**
** Description:
**	This routine is named after the DBMS's RDF routine but is much
**	simpler.  It searches the "RDF" control block for a table
**	matching the name passed as an argument.  Currently it uses a
**	linear search, which can later be changed to a binary search.
**	
** Inputs:
**      table_no	- table number
**
** Outputs:
**	table_ptr	- pointer to a RS_TBLDESC structure, NULL if not found
**
** Returns:
**	OK	Function completed normally.
**	FAIL	No such table was found.
*/
STATUS
RSrdf_gdesc(
i4		table_no,
RS_TBLDESC	**table_ptr)
{
	RS_RDI_SVCB	*svcb;
	STATUS		status;
	i4		i;

	svcb = &RSrdf_svcb;
	*table_ptr = svcb->tbl_info;
	for (i = 0; i < svcb->num_tbls; ++i, ++*table_ptr)
	{
		if ((*table_ptr)->table_no == table_no)
			return (OK);
	}

	*table_ptr = (RS_TBLDESC *)NULL;
	return (FAIL);
}

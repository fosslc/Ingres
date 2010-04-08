/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include "rdfint.h"
# include "tblinfo.h"

/**
** Name:	rdfstart.c - start up RDF for the RepServer
**
** Description:
**	Defines
**		RSrdf_startup	- start up RDF for the RepServer
**		rdf_init_cache	- initialize the RepServer cache
**
**  History:
**	21-jan-97 (joea)
**		Created based on rdfstart.c in replicator library.
**	24-nov-97 (joea)
**		Change RSrdf_xxx function names to avoid problems later in MVS.
**	15-jun-98 (abbjo03)
**		Change arguments to RSicc_InitColCache.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALDEF RS_RDI_SVCB	RSrdf_svcb;

static STATUS rdf_init_cache();


/*{
** Name:	RSrdf_startup - start up RDF for the RepServer
**
** Description:
**	This routine is named after the DBMS's RDF routine but is much
**	simpler.  It initializes the "RDF" control block with the
**	number of tables and columns registered for replication.  It
**	then allocates a cache for keeping the table and column
**	information and sets up the pointers in the control block.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
** Returns:
**	OK	Function completed normally.
**	FAIL	No tables or columns registered for replication.
*/
STATUS
RSrdf_startup()
{
	RS_RDI_SVCB	*svcb;
	u_i4		cache_size, t_size, c_size;
	STATUS		status;

	/* get the number of registered tables and registered columns */
	svcb = &RSrdf_svcb;
	svcb->num_tbls = RSnrt_NumRepTbls();
	if (svcb->num_tbls == 0)
		return (FAIL);
	svcb->num_cols = RSnrc_NumRepCols();
	if (svcb->num_cols == 0)
		return (FAIL);

	/* request cache memory and initialize pointers */
	t_size = sizeof(RS_TBLDESC) +
		sizeof(RS_TBLDESC) % sizeof(ALIGN_RESTRICT);
	c_size = sizeof(RS_COLDESC) +
		sizeof(RS_COLDESC) % sizeof(ALIGN_RESTRICT);
	cache_size = svcb->num_tbls * t_size + svcb->num_cols * c_size;
	svcb->tbl_info = (RS_TBLDESC *)MEreqmem(0, cache_size, TRUE, &status);
	if (svcb->tbl_info == (RS_TBLDESC *)NULL)
		return (status);
	svcb->col_info = (RS_COLDESC *)((PTR)svcb->tbl_info +
		svcb->num_tbls * t_size);

	return (rdf_init_cache());
}


/*{
** Name:	rdf_init_cache - initialize the caches
**
** Description:
**	This initializes the tables and columns caches.  Currently it
**	is called once at startup because we can't have two sessions.
**	Later on this can go away and the caches will be initialized
**	on demand.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
** Returns:
**	OK	Function completed normally.
**	FAIL	No tables or columns registered for replication.
*/
static STATUS
rdf_init_cache()
{
	RS_RDI_SVCB	*svcb;
	RS_TBLDESC	*t;
	RS_COLDESC	*c;
	i4		i, j;
	STATUS		status;

	/* initialize the tables cache */
	svcb = &RSrdf_svcb;
	status = RSitc_InitTblCache(svcb->tbl_info, svcb->num_tbls);
	if (status != OK)
		return (status);

	/* initialize the columns cache */
	c = svcb->col_info;
	for (i = 0, t = svcb->tbl_info; i < svcb->num_tbls; ++i, ++t)
	{
		t->cols = c;
		RSicc_InitColCache(t, t->cols, &t->num_regist_cols);
		for (j = 0; j < t->num_regist_cols; ++j)
		{
			if (c[j].key_sequence)
				++t->num_key_cols;
		}
		c += t->num_regist_cols;
	}
	return (OK);
}

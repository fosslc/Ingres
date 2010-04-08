/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h> 
# include	<si.h> 
# include	<qg.h> 
# include	"qgi.h"

/**
** Name:    qgosl.c -	Query Generator Module ABF Run-time Support Interface.
**
** Description:
**	Contains the special support interface for the ABF run-time system.
**	Defines:
**
**	IIQG_inquery()	return active state of query.
**	IIQG_free()	free internal query state structure.
**
** History:
**	Revision 6.0  87/04  wong
**	Removed 'QG_trmwhite()' for Version 1.
*/

/*{
** Name:    IIQG_inquery() -	Return Active State of Query.
**
** Description:
**	Returns TRUE if specified query is currently active, FALSE
**	otherwise.
**
** Input:
**	qdesc	{QDESC *}  The query description.
**
** Returns:
**	{bool}  Whether the query is active.
**
** Called by:
**	abrtsgen()
*/

bool
IIQG_inquery (qdesc)
QDESC	*qdesc;
{
    return (bool)(qdesc->qg_internal != NULL &&
	(qdesc->qg_internal->qg_state & QS_IN) != 0);
}

/*
** Name:    IIQG_free() -	Free Internal Query State Structure.
**
** Description:
**	Free the internal query state structure and any mapping structures
**	for a query.  Note:  This is also called by QBF.
**
** Input:
**	qry	{QDESC *}  The query description.
**
** History:
**	8/9/85 (mgw) -- Added to fix bug #6187.
**	04/87 (jhw) -- Modified for Version 1.
*/

VOID
IIQG_free (qry)
register QDESC	*qry;
{
    while (qry != NULL)
    {
	register QSTRUCT    *qsp;

	if ((qsp = qry->qg_internal) == NULL)
	    break;

	if ( (qsp->qg_state & QS_QBF) == 0 )
	{ /* not QBF */
		if (qsp->qg_buffer != NULL)
			MEfree(qsp->qg_buffer);
		if (qry->qg_internal->qg_margv != NULL)
			MEfree(qsp->qg_margv);
	}
	if (qsp->qg_lbuf != NULL)
	    MEfree(qsp->qg_lbuf);
	MEfree(qry->qg_internal);
	qry->qg_internal = NULL;

	qry = qry->qg_child;
   }
}

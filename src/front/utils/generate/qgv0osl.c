/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h> 
# include	<si.h> 
# include	<qg.h> 
# include	<qgv0.h>
# include	"qgi.h"

/**
** Name:    qgv0osl.c -	Query Generator Module OSL Support, Version 0.
**
** Description:
**	QG is the module for the query generators used in MQBF and OSL.
**	It runs the query that is passed in in a QDV0.  The important
**	feature of a generator is that it produces one tuple at a time
**	each time it is called.  This means that a query does not have
**	to be run in a loop.
**
**	QG_inquery()	return active state of query.
**	QG_trmwhite()	trim white space from query retrieved strings.
**	QG_free()	free internal query state structure.
**
** History:
*/

/*{
** Name:    QG_inquery() -	Return Active State of Query.
**
**	Returns TRUE if currently within the specified query, FALSE if
**	the all tuples for the query have already been displayed.
**
** Parameters:
**	qdesc	{QDV0 *}  The query.
**
** Called by:
**	abrtsgen()
*/
bool
QG_inquery(qdesc)
QDV0	*qdesc;
{
	if (qdesc->qg_internal == NULL)
		return FALSE;
	if (qdesc->qg_internal->qg_state & QS_IN)
		return TRUE;
	else
		return FALSE;
}

/*{
** Name:    QG_trmwhite() -	Trim White Space From Query Retrieved Strings.
**
** Description:
**	Trim white space from all strings retrieve through
**	a QDV0 structure.  This is called by ABF to make sure all
**	strings get trimed.
*/
STATUS
QG_trmwhite(qry)
QDV0	*qry;
{
	QSTRUCT		*qstr;
	char		**argv;
	char		*cp;

	if ((qstr = qry->qg_internal) == NULL)
		return QE_NOINTERNAL;
	if ((qstr->qg_state & QS_MAP))
		argv = qstr->qg_margv;
	else
		argv = qry->qg_argv;
	for (cp = qry->qg_param; *cp; cp++)
	{
		if (*cp != '%')
			continue;
		switch (*++cp)
		{
		  case 'c':
			STtrmwhite(*argv);
			argv++;
			break;

		  case 'i':
		  case 'f':
			argv++;
			break;
		}
	}
	return OK;
}


/*{
** Name:    QG_free() -	Free Internal Query State Structure.
**
** Descripttion:
**	Free's a Qdesc structure
**
**	added 8/9/85 to fix bug 6187 - mgw
*/
VOID
QG_free(qry)
QDV0	*qry;
{
	while (qry != NULL)
	{
		if (qry->qg_internal == NULL)	/* if this is true, then we */
		{				/* know nothing about the   */
			return;			/* rest of the structure    */
		}
		if ((qry->qg_argv != NULL) && (qry->qg_internal->qg_state & QS_MAP))
		{
			MEfree(qry->qg_argv);
			qry->qg_argv = NULL;
		}
		if (qry->qg_internal->qg_buffer != NULL)
			MEfree(qry->qg_internal->qg_buffer);
		if (qry->qg_internal->qg_msize != NULL)
			MEfree(qry->qg_internal->qg_msize);
		if (qry->qg_internal->qg_lbuf != NULL)
			MEfree(qry->qg_internal->qg_lbuf);
		MEfree(qry->qg_internal);
		qry->qg_internal = NULL;
		qry = qry->qg_child;
	}
}

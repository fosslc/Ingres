/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include	<qg.h>
#include	<si.h>
#include	<lo.h>
#include	"qgi.h"

/**
** Name:    qgqbf.c -	Query Generator Module QBF Support Interface.
**
** Description:
**	Contains a special support interface for QBF.  Defines:
**
**	IIQG_qbf()	set-up query for QBF.
**
** History:
**	Revision 6.0  87/04  wong
**	Initial revision (moved from "generate/qg.qc".)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:    IIQG_qbf() -	Set-Up Query For QBF.
**
** Description:
**	This is a special interface for QBF.  It makes QG use the buffer
**	that QBF has already allocated so the QG does not create its own.
**
** Input:
**	qry	{QDESC *}  The Query that QBF is running.
**
**	buf	{char *}  The buffer for the tuples of the query.
**
**	size	{nat}  The size of the buffer.
**
** Returns:
**	{STATUS}  OK
**		  QE_MEM	If the QDESC is invalid.
** History:
**	03/84 (jrc) -- Written.
*/
STATUS
IIQG_qbf(qry, buf, size)
register QDESC	*qry;
char		*buf;
i4		size;
{
    register QSTRUCT	*qint;

    if (qry->qg_internal == NULL)
    {
	if ((qry->qg_internal = IIQG_alloc()) == NULL)
	    return QE_MEM;
    }
    qint = qry->qg_internal;
    qint->qg_state |= QS_QBF;
    qint->qg_buffer = buf;
    qint->qg_bsize = size;

    return OK;
}

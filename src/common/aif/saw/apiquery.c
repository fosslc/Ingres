/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <iiapi.h>
# include <sapiw.h>
# include "sapiwi.h"

/**
** Name:	apiquery.c - synchronous API wrapper query functions
**
** Description:
**	Defines
**		IIsw_query			- simple query
**		IIsw_queryRepeated		- repeated query
**		IIsw_selectSingleton		- singleton SELECT
**		IIsw_selectSingletonRepeated	- repeated singleton SELECT
**		IIsw_selectLoop			- SELECT loop
**		IIsw_selectLoopRepeated		- repeated SELECT loop
**		IIsw_queryQry			- generic query call
**
** History:
**	16-apr-98 (joea)
**		Created.
**	21-may-98 (joea)
**		Test for no data after IIsw_getColumns in IIsw_selectSingleton.
**	09-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton and IIsw_query and
**		add support blob datatypes. Eliminate IIsw_queryNP which has
**		been replaced by a macro call on IIsw_query.
**	11-aug-98 (abbjo03)
**		Add support for repeated queries. Remove IIsw_queryPrepExec.
**	21-jan-99 (abbjo03)
**		In IIsw_queryRepeated and IIsw_selectSingletonRepeated, return
**		if the second IIsw_queryQry call is not successful. In
**		IIsw_selectSingletonRepeated and IIsw_selectLoopRepeated, use
**		IIsw_getQueryInfo to check for invalid repeat query ID.
**	22-jan-99 (abbjo03)
**		Use a local error handle in IIsw_selectSingletonRepeated when
**		calling IIsw_getQueryInfo.
**	25-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	15-mar-99 (abbjo03)
**		Only call IIsw_getErrorInfo if the error handle is not null.
**	01-nov-1999 (somsa01)
**		In IIsw_selectSingletonRepeated(), if qryId is NULL, then the
**		repeat query id will be a run-time id rather than a
**		compile-time id.
**	28-dec-1999 (abbjo03) bug 99861
**		Deal with the case of the first replicated column being a long
**		varchar or long byte.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_swSelect functions.
**      05-nov-2009 (joea)
**          Initialize qy_flags parameter in IIsw_queryQry.
**/

/*{
** Name:	IIsw_query	- simple query
**
** Description:
**	Issues a simple non-SELECT query.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	ppCallback	- pointer to callback for IIsw_putParms
**	ppClosure	- closure argument to callback for IIsw_putParms
**
** Outputs:
**	tranHandle	- transaction handle
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	Status from call to IIsw_getQueryInfo.
*/
IIAPI_STATUS
IIsw_query(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
IISW_CALLBACK		ppCallback,
II_PTR			ppClosure,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	i4			i;
	IIAPI_STATUS		status;
	II_INT2			baseCol;

	status = IIsw_queryQry(connHandle, tranHandle, IIAPI_QT_QUERY,
		queryText, parmCount ? TRUE : FALSE, stmtHandle, 
		qInfoParm, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);
	if (parmCount)
	{
		status = IIsw_setDescriptor(*stmtHandle, parmCount, parmDesc,
			errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
		baseCol = 0;
		for (i = 0; i < parmCount; ++i)
		{
			/* skip over non-blob columns */
			if (parmDesc[i].ds_dataType == IIAPI_LVCH_TYPE ||
				parmDesc[i].ds_dataType == IIAPI_LBYTE_TYPE)
			{
				if (i - baseCol)
				{
					/* put the non-blob columns */
					status = IIsw_putParms(*stmtHandle,
						(II_INT2)(i - baseCol),
						parmData + baseCol, NULL, NULL,
						errParm);
					if (status != IIAPI_ST_SUCCESS)
						return (status);
				}
				/* put all blob segments */
				do
				{
					status = IIsw_putParms(*stmtHandle, 1,
						parmData + i, ppCallback,
						ppClosure, errParm);
					if (status != IISW_MORE_SEGMENTS &&
						status != IIAPI_ST_SUCCESS)
						return (status);
				} while (status == IISW_MORE_SEGMENTS);
				baseCol = i + 1;
			}
		}
		/*
		** If there were no blob columns or if non-blob columns remain
		** after the last blob column, put all the columns or the rest,
		** respectively.
		*/
		if (baseCol != i)
		{
			status = IIsw_putParms(*stmtHandle, (II_INT2)(i -
				baseCol), parmData + baseCol, NULL, NULL,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
		}
	}
	
	return (IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm));
}


/*{
** Name:	IIsw_queryRepeated - repeated query
**
** Description:
**	Issues a repeated query.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	qryId		- repeat query ID
**	qryHandle	- repeat query handle
**
** Outputs:
**	tranHandle	- transaction handle
**	qryHandle	- repeat query handle
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	Status from call to IIsw_getQueryInfo.
*/
IIAPI_STATUS
IIsw_queryRepeated(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
SW_REPEAT_QUERY_ID	*qryId,
II_PTR			*qryHandle,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_STATUS		status;

	while (TRUE)
	{
		if (*qryHandle == NULL)
		{
			status = IIsw_queryQry(connHandle, tranHandle,
				IIAPI_QT_DEF_REPEAT_QUERY, queryText, TRUE,
				stmtHandle, qInfoParm, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			status = IIsw_putParmsRepeated(*stmtHandle, TRUE,
				parmCount, parmDesc, parmData, qryId, qryHandle,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			status = IIsw_getQueryInfo(*stmtHandle, qInfoParm,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*qryHandle = qInfoParm->gq_repeatQueryHandle;
			status = IIsw_close(*stmtHandle, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*stmtHandle = NULL;
		}
		status = IIsw_queryQry(connHandle, tranHandle,
			IIAPI_QT_EXEC_REPEAT_QUERY, ERx(""), TRUE, stmtHandle,
			qInfoParm, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		status = IIsw_putParmsRepeated(*stmtHandle, FALSE, parmCount,
			parmDesc, parmData, qryId, qryHandle, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		status = IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm);
		if (!(qInfoParm->gq_flags & IIAPI_GQF_UNKNOWN_REPEAT_QUERY))
			break;
		*qryHandle = NULL;
		status = IIsw_close(*stmtHandle, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		*stmtHandle = NULL;
	}

	return (status);
}


/*{
** Name:	IIsw_selectSingleton	- singleton SELECT
**
** Description:
**	Issues a singleton SELECT.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	columnCount	- number of columns
**	gcCallback	- pointer to callback for IIapi_getColumns
**	gcClosure	- closure argument to callback for IIapi_getColumns
**
** Outputs:
**	tranHandle	- transaction handle
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_selectSingleton(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
II_INT2			columnCount,
IIAPI_DATAVALUE		*columnData,
IISW_CALLBACK		gcCallback,
II_PTR			gcClosure,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	i4			i;
	IIAPI_STATUS		status;
	IIAPI_GETDESCRPARM	gdescParm;
	II_INT2			rowCount;
	II_INT2			baseCol;

	status = IIsw_queryQry(connHandle, tranHandle,
		IIAPI_QT_SELECT_SINGLETON, queryText, parmCount ? TRUE : FALSE,
		stmtHandle, qInfoParm, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);

	if (parmCount)
	{
		status = IIsw_setDescriptor(*stmtHandle, parmCount, parmDesc,
			errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
		status = IIsw_putParms(*stmtHandle, parmCount, parmData, NULL,
			NULL, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
	}

	status = IIsw_getDescriptor(*stmtHandle, &gdescParm, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);

	if (gdescParm.gd_descriptorCount != columnCount)
	{
		IIsw_getErrorInfo(&IISW_internalError, errParm);
		return (E_SW0001_INTERNAL_ERROR);
	}
	for (i = 0; i < columnCount; ++i)
		if (gdescParm.gd_descriptor[i].ds_length >
			columnData[i].dv_length)
		{
			IIsw_getErrorInfo(&IISW_internalError, errParm);
			return (E_SW0001_INTERNAL_ERROR);
		}

	rowCount = 1;
	baseCol = 0;
	for (i = 0; i < columnCount; ++i)
	{
		/* skip over non-blob columns */
		if (gdescParm.gd_descriptor[i].ds_dataType == IIAPI_LVCH_TYPE
			|| gdescParm.gd_descriptor[i].ds_dataType ==
			IIAPI_LBYTE_TYPE)
		{
			if (i - baseCol)
			{
				/* get the non-blob columns */
				status = IIsw_getColumns(*stmtHandle, &rowCount,
					(II_INT2)(i - baseCol), columnData +
					baseCol, NULL, NULL, errParm);
				if (status != IIAPI_ST_SUCCESS && status !=
						IIAPI_ST_NO_DATA)
					return (status);
			}
			/* fetch all blob segments */
			do
			{
				status = IIsw_getColumns(*stmtHandle, &rowCount,
					1, columnData + i, gcCallback,
					gcClosure, errParm);
				if (status != IISW_MORE_SEGMENTS && status !=
						IIAPI_ST_SUCCESS && status !=
						IIAPI_ST_NO_DATA)
					return (status);
			} while (status == IISW_MORE_SEGMENTS);
			baseCol = i + 1;
		}
	}
	/*
	** If there were no blob columns or if non-blob columns remain after
	** the last blob column, fetch all the columns or the rest,
	** respectively.
	*/
	if (baseCol != i)
	{
		status = IIsw_getColumns(*stmtHandle, &rowCount, (II_INT2)(i -
			baseCol), columnData + baseCol, NULL, NULL, errParm);
		if (status != IIAPI_ST_SUCCESS && status != IIAPI_ST_NO_DATA)
			return (status);
	}

	return (IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm));
}


/*{
** Name:	IIsw_selectSingletonRepeated - repeated singleton SELECT
**
** Description:
**	Issues a repeated singleton SELECT.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	qryId		- repeat query ID
**	qryHandle	- repeat query handle
**	columnCount	- number of columns
**
** Outputs:
**	tranHandle	- transaction handle
**	qryHandle	- repeat query handle
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_selectSingletonRepeated(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
SW_REPEAT_QUERY_ID	*qryId,
II_PTR			*qryHandle,
II_INT2			columnCount,
IIAPI_DATAVALUE		*columnData,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	i4			i;
	IIAPI_STATUS		status;
	II_INT2			rowCount;
	IIAPI_GETDESCRPARM	gdescParm;

	while (TRUE)
	{
		if (*qryHandle == NULL)
		{
		    if (qryId)
		    {
			status = IIsw_queryQry(connHandle, tranHandle,
				IIAPI_QT_DEF_REPEAT_QUERY, queryText, TRUE,
				stmtHandle, qInfoParm, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			status = IIsw_putParmsRepeated(*stmtHandle, TRUE,
				parmCount, parmDesc, parmData, qryId, qryHandle,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
		    }
		    else
		    {
			status = IIsw_queryQry(connHandle, tranHandle,
				IIAPI_QT_DEF_REPEAT_QUERY, queryText, FALSE,
				stmtHandle, qInfoParm, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
		    }

			status = IIsw_getQueryInfo(*stmtHandle, qInfoParm,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*qryHandle = qInfoParm->gq_repeatQueryHandle;
			status = IIsw_close(*stmtHandle, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*stmtHandle = NULL;
		}
		status = IIsw_queryQry(connHandle, tranHandle,
			IIAPI_QT_EXEC_REPEAT_QUERY, ERx(""), TRUE, stmtHandle,
			qInfoParm, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		status = IIsw_putParmsRepeated(*stmtHandle, FALSE, parmCount,
			parmDesc, parmData, qryId, qryHandle, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;

		status = IIsw_getDescriptor(*stmtHandle, &gdescParm, errParm);
		if (status == IIAPI_ST_ERROR)
		{
			if (errParm->ge_errorCode == E_AP0014_INVALID_REPEAT_ID)
			{
				*qryHandle = NULL;
				status = IIsw_close(*stmtHandle, errParm);
				if (status != IIAPI_ST_SUCCESS)
					break;
				*stmtHandle = NULL;
				continue;
			}
			return (status);
		}
		if (status != IIAPI_ST_SUCCESS)
			break;

		if (gdescParm.gd_descriptorCount != columnCount)
		{
			IIsw_getErrorInfo(&IISW_internalError, errParm);
			return (E_SW0001_INTERNAL_ERROR);
		}
		for (i = 0; i < columnCount; ++i)
			if (gdescParm.gd_descriptor[i].ds_length >
				columnData[i].dv_length)
			{
				IIsw_getErrorInfo(&IISW_internalError, errParm);
				return (E_SW0001_INTERNAL_ERROR);
			}

		rowCount = 1;
		status = IIsw_getColumns(*stmtHandle, &rowCount, columnCount,
			columnData, NULL, NULL, errParm);
		if (status != IIAPI_ST_SUCCESS && status != IIAPI_ST_NO_DATA)
			break;

		status = IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm);
		if (!(qInfoParm->gq_flags & IIAPI_GQF_UNKNOWN_REPEAT_QUERY))
			break;
		*qryHandle = NULL;
		status = IIsw_close(*stmtHandle, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		*stmtHandle = NULL;
	}

	return (status);
}


/*{
** Name:	IIsw_selectLoop	- SELECT loop
**
** Description:
**	Initiate or continue a SELECT loop.  To initiate stmtHandle should be
**	NULL.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	columnCount	- number of columns
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**
** Outputs:
**	tranHandle	- transaction handle
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	IIAPI_ST_SUCCESS	- successfully retrieved row data
**	IIAPI_ST_NO_DATA	- no more row data
**	anything else		- error
*/
IIAPI_STATUS
IIsw_selectLoop(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
II_INT2			columnCount,
IIAPI_DATAVALUE		*columnData,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	i4			i;
	IIAPI_STATUS		status;
	II_INT2			rowCount;
	IIAPI_GETDESCRPARM	gdescParm;

	if (*stmtHandle == NULL)
	{
		status = IIsw_queryQry(connHandle, tranHandle,
			IIAPI_QT_SELECT, queryText, parmCount ? TRUE : FALSE,
			stmtHandle, qInfoParm, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);

		if (parmCount)
		{
			status = IIsw_setDescriptor(*stmtHandle, parmCount,
				parmDesc, errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
			status = IIsw_putParms(*stmtHandle, parmCount, parmData,
				NULL, NULL, errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
		}

		status = IIsw_getDescriptor(*stmtHandle, &gdescParm, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);

		if (gdescParm.gd_descriptorCount != columnCount)
		{
			IIsw_getErrorInfo(&IISW_internalError, errParm);
			return (E_SW0001_INTERNAL_ERROR);
		}
		for (i = 0; i < columnCount; ++i)
			if (gdescParm.gd_descriptor[i].ds_length >
					columnData[i].dv_length)
			{
				IIsw_getErrorInfo(&IISW_internalError, errParm);
				return (E_SW0001_INTERNAL_ERROR);
			}
	}

	rowCount = 1;
	status = IIsw_getColumns(*stmtHandle, &rowCount, columnCount,
		columnData, NULL, NULL, errParm);
	if (status != IIAPI_ST_SUCCESS && status != IIAPI_ST_NO_DATA)
		return (status);
	else if (rowCount)
		return (IIAPI_ST_SUCCESS);
	status = IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm);
	if (status != IIAPI_ST_SUCCESS)
		return (status);

	return (IIAPI_ST_NO_DATA);
}


/*{
** Name:	IIsw_selectLoopRepeated	- repeated SELECT loop
**
** Description:
**	Initiate or continue a repeated SELECT loop.  To initiate stmtHandle
**	should be NULL.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryText	- query statement
**	parmCount	- number of parameters
**	parmDesc	- array of parameter descriptors
**	parmData	- array of parameter data value descriptors
**	qryId		- repeat query ID
**	qryHandle	- repeat query handle
**	columnCount	- number of columns
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**
** Outputs:
**	tranHandle	- transaction handle
**	columnData	- array of column data value descriptors
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	IIAPI_ST_SUCCESS	- successfully retrieved row data
**	IIAPI_ST_NO_DATA	- no more row data
**	anything else		- error
*/
IIAPI_STATUS
IIsw_selectLoopRepeated(
II_PTR			connHandle,
II_PTR			*tranHandle,
II_CHAR			*queryText,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
SW_REPEAT_QUERY_ID	*qryId,
II_PTR			*qryHandle,
II_INT2			columnCount,
IIAPI_DATAVALUE		*columnData,
II_PTR			*stmtHandle,
IIAPI_GETQINFOPARM	*qInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	i4			i;
	IIAPI_STATUS		status;
	II_INT2			rowCount;
	IIAPI_GETDESCRPARM	gdescParm;

	while (TRUE)
	{
		if (*stmtHandle == NULL && *qryHandle == NULL)
		{
			status = IIsw_queryQry(connHandle, tranHandle,
				IIAPI_QT_DEF_REPEAT_QUERY, queryText, TRUE,
				stmtHandle, qInfoParm, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			status = IIsw_putParmsRepeated(*stmtHandle, TRUE,
				parmCount, parmDesc, parmData, qryId, qryHandle,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			status = IIsw_getQueryInfo(*stmtHandle, qInfoParm,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*qryHandle = qInfoParm->gq_repeatQueryHandle;
			status = IIsw_close(*stmtHandle, errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;
			*stmtHandle = NULL;
		}
		if (*stmtHandle == NULL)
		{
			status = IIsw_queryQry(connHandle, tranHandle,
				IIAPI_QT_EXEC_REPEAT_QUERY, ERx(""), TRUE,
				stmtHandle, qInfoParm, errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
			status = IIsw_putParmsRepeated(*stmtHandle, FALSE,
				parmCount, parmDesc, parmData, qryId, qryHandle,
				errParm);
			if (status != IIAPI_ST_SUCCESS)
				break;

			status = IIsw_getDescriptor(*stmtHandle, &gdescParm,
				errParm);
			if (status == IIAPI_ST_ERROR)
			{
				if (errParm->ge_errorCode ==
					E_AP0014_INVALID_REPEAT_ID)
				{
					*qryHandle = NULL;
					status = IIsw_close(*stmtHandle,
						errParm);
					if (status != IIAPI_ST_SUCCESS)
						break;
					*stmtHandle = NULL;
					continue;
				}
				return (status);
			}
			if (status != IIAPI_ST_SUCCESS)
				break;

			if (gdescParm.gd_descriptorCount != columnCount)
			{
				IIsw_getErrorInfo(&IISW_internalError, errParm);
				return (E_SW0001_INTERNAL_ERROR);
			}
			for (i = 0; i < columnCount; ++i)
				if (gdescParm.gd_descriptor[i].ds_length >
						columnData[i].dv_length)
				{
					IIsw_getErrorInfo(&IISW_internalError,
						errParm);
					return (E_SW0001_INTERNAL_ERROR);
				}
		}

		rowCount = 1;
		status = IIsw_getColumns(*stmtHandle, &rowCount, columnCount,
			columnData, NULL, NULL, errParm);
		if (status != IIAPI_ST_SUCCESS && status != IIAPI_ST_NO_DATA)
			break;
		else if (rowCount)
			return (IIAPI_ST_SUCCESS);
		status = IIsw_getQueryInfo(*stmtHandle, qInfoParm, errParm);
		if (!(qInfoParm->gq_flags & IIAPI_GQF_UNKNOWN_REPEAT_QUERY))
			return (IIAPI_ST_NO_DATA);
		*qryHandle = NULL;
		status = IIsw_close(*stmtHandle, errParm);
		if (status != IIAPI_ST_SUCCESS)
			break;
		*stmtHandle = NULL;
	}

	return (status);
}


/*{
** Name:	IIsw_queryQry	- generic query call
**
** Description:
**	Calls IIapi_query and then waits for it to complete.
**
** Inputs:
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**	queryType	- query type
**	queryText	- query statement
**	params		- will parameters be sent?
**
** Outputs:
**	tranHandle	- transaction handle
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**
** Returns:
**	none
*/
IIAPI_STATUS
IIsw_queryQry(
II_PTR		connHandle,
II_PTR		*tranHandle,
IIAPI_QUERYTYPE	queryType,
II_CHAR		*queryText,
II_BOOL		params,
II_PTR		*stmtHandle,
IIAPI_GETQINFOPARM *qryInfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_QUERYPARM	qryParm;

	qryParm.qy_genParm.gp_callback = NULL;
	qryParm.qy_genParm.gp_closure = NULL;
	qryParm.qy_connHandle = connHandle;
	qryParm.qy_queryType = queryType;
	qryParm.qy_queryText = queryText;
	qryParm.qy_parameters = params;
	qryParm.qy_tranHandle = *tranHandle;
        qryParm.qy_flags = 0;
	IIapi_query(&qryParm);
	*tranHandle = qryParm.qy_tranHandle;
	*stmtHandle = qryParm.qy_stmtHandle;
	IIsw_wait(&qryParm.qy_genParm);
	if (qryParm.qy_genParm.gp_errorHandle)
		IIsw_getErrorInfo(qryParm.qy_genParm.gp_errorHandle, errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (qryParm.qy_genParm.gp_status);
}

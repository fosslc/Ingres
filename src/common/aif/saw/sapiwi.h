/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef SAPIWI_H_INCLUDED
# define SAPIWI_H_INCLUDED
# include <compat.h>
# include <iiapi.h>
# include <sapiw.h>

/**
** Name:	sapiwi.h - synchronous API wrapper internal functions
**
** Description:
**	Include file for internal functions to interface with the OpenIngres
**	API.
**
** History:
**	15-apr-98 (joea)
**		Created.
**	06-may-98 (shero03)
**	     Moved getColumns to sapiw.h
**	09-jul-98 (abbjo03)
**		Add callback and closure arguments to IIsw_putParms.
**	30-jul-98 (abbjo03)
**		Add IIsw_putParmsRepeated to support repeated queries.
**	23-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	26-mar-2004 (gupsh01)
**		Added getQinfoParm as input parameter to II_sw function calls.
**/

# define E_SW0001_INTERNAL_ERROR	0xEA0001


GLOBALREF II_PTR IISW_internalError;


FUNC_EXTERN IIAPI_STATUS IIsw_getDescriptor(II_PTR stmtHandle,
	IIAPI_GETDESCRPARM *gdescParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_putParms(II_PTR stmtHandle, II_INT2 parmCount,
	IIAPI_DATAVALUE *parmData, IISW_CALLBACK callback, II_PTR closure,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_putParmsRepeated(II_PTR stmtHandle, bool define,
	II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_DATAVALUE *parmData, SW_REPEAT_QUERY_ID *qryId, II_PTR *qryHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_queryQry(II_PTR connHandle, II_PTR *tranHandle,
	IIAPI_QUERYTYPE queryType, II_CHAR *queryText, II_BOOL params,
	II_PTR *stmtHandle, IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_setDescriptor(II_PTR stmtHandle,
	II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN II_VOID IIsw_wait(IIAPI_GENPARM *genParm);

# endif

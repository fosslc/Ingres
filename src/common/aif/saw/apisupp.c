/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "sapiwi.h"

/**
** Name:	apisupp.c - synchronous API wrapper support functions
**
** Description:
**	Defines
**		IIsw_initialize		- initialize the API
**		IIsw_terminate		- terminate the API
**		IIsw_getQueryInfo	- get query information
**		IIsw_getRowCount	- get row count
**		IIsw_getDescriptor	- get data descriptors
**		IIsw_setDescriptor	- set data descriptors
**		IIsw_getColumns		- get columns
**		IIsw_putParms		- put parameters
**		IIsw_putParmsRepeated	- put repeated parameters
**		IIsw_close		- close a query or dbevent retrieval
**		IIsw_wait		- wait for an API call to complete
**		IIsw_getErrorInfo	- get error information
**
** History:
**	15-apr-98 (joea)
**		Created.
**	09-jul-98 (abbjo03)
**		Add callback and closure arguments to IIsw_getColumns and
**		IIsw_putParms to support blob datatypes.
**	30-jul-98 (abbjo03)
**		Add IIsw_putParmsRepeated to support repeated queries.
**	20-aug-98 (abbjo03)
**		Correct the assignment of the repeated query character string
**		in IIsw_putParmsRepeated.
**	25-feb-99 (abbjo03)
**		Add envHandle to IIsw_initialize to support API version 2.
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	15-mar-99 (abbjo03)
**		Only call IIsw_getErrorInfo if the error handle is not null.
**	22-apr-99 (abbjo03)
**		Add envHandle to IIsw_terminate to support API version 2.
**		IIsw_terminate will call IIapi_releaseEnv if a non-NULL
**		envHandle is passed. IIsw_initialize will use version 1 or 2
**		depending on the envHandle pointer..
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_swSelect functions.
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from MEcopy calls.
**      05-nov-2009 (joea)
**          Add support for IIAPI_VERSION_7 in IIsw_initialize.
**      30-aug-2010 (thich0)
**          Add support for IIAPI_VERSION_8 in IIsw_initialize.
**/

# define E_SW0001_INTERNAL_ERROR	0xEA0001


GLOBALDEF II_PTR IISW_internalError;


static char *msg = "Internal OpenIngres API related error";


/*{
** Name:	IIsw_initialize - initialize the API
**
** Description:
**	Calls IIapi_initialize. If envHandle is not NULL, initializes at
**	version 2 and returns an environment handle.
**
** Inputs:
** 	envHandle	- environment handle pointer.	
**	version		- version information for api,
**			  valid values are 
** 			 IIAPI_VERSION_1, IIAPI_VERSION_2,  
**			 and IIAPI_VERSION_3 and IIAPI_VERSION_4 
**	
**
** Outputs:
**	envHandle	- environment handle
**
** Returns:
**	The status from the initialize parameter block.
** History:
**	25-nov-2003 (gupsh01)
**	    Added version an an input parameter.
**	08-apr-2005 (gupsh01)
**	    Added support for IIAPI_VERSION_4 (bug 114266).
**      28-Apr-2009 (coomi01) b121984
**          Added support for IIAPI_VERSION_5 
*/
IIAPI_STATUS
IIsw_initialize(
II_PTR	*envHandle, 
i4	version)
{
	IIAPI_INITPARM	init_parm;
	IIAPI_GETEINFOPARM      *errParm;
	
	init_parm.in_timeout = -1;
	if (version == 0)
	  init_parm.in_version = (envHandle ? IIAPI_VERSION_2 : IIAPI_VERSION_1);
	else 
	{
            if (version >= IIAPI_VERSION_1 && version <= IIAPI_VERSION_8)
		init_parm.in_version = version;
	  else 
            return (E_SW0001_INTERNAL_ERROR);
	}

	IIapi_initialize(&init_parm);
	if (envHandle && init_parm.in_status == IIAPI_ST_SUCCESS)
		*envHandle = init_parm.in_envHandle;
	return (init_parm.in_status);
}


/*{
** Name:	IIsw_terminate - terminate the API
**
** Description:
**	If envHandle is not NULL, calls IIapi_releaseEnv. Then calls
**	IIapi_terminate.
**
** Inputs:
**	envHandle	- environment handle
**
** Outputs:
**	none
**
** Returns:
**	The status from the terminate parameter block.
*/
IIAPI_STATUS
IIsw_terminate(
II_PTR	envHandle)
{
	IIAPI_RELENVPARM	relenv_parm;
	IIAPI_TERMPARM		term_parm;

	relenv_parm.re_status = IIAPI_ST_SUCCESS;
	if (envHandle)
	{
		relenv_parm.re_envHandle = envHandle;
		IIapi_releaseEnv(&relenv_parm);
	}
	IIapi_terminate(&term_parm);
	return (relenv_parm.re_status == IIAPI_ST_SUCCESS ? term_parm.tm_status
		: relenv_parm.re_status);
}


/*{
** Name:	IIsw_getQueryInfo	- get query information
**
** Description:
**	Calls IIapi_getQueryInfo.
**
** Inputs:
**	stmtHandle	- statement handle
**
** Outputs:
**	qinfoParm	- query information parameters block
**	errParm		- error parameters block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_getQueryInfo(
II_PTR			stmtHandle,
IIAPI_GETQINFOPARM	*qinfoParm,
IIAPI_GETEINFOPARM	*errParm)
{
	qinfoParm->gq_genParm.gp_callback = NULL;
	qinfoParm->gq_genParm.gp_closure = NULL;
	qinfoParm->gq_stmtHandle = stmtHandle;
	IIapi_getQueryInfo(qinfoParm);
	IIsw_wait(&qinfoParm->gq_genParm);
	if (qinfoParm->gq_genParm.gp_errorHandle)
		IIsw_getErrorInfo(qinfoParm->gq_genParm.gp_errorHandle,
			errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (qinfoParm->gq_genParm.gp_status);
}


/*{
** Name:	IIsw_getRowCount	- get row count
**
** Description:
**	Calls IIapi_getQueryInfo and returns row count.
**
** Inputs:
**	stmtHandle	- statement handle
**
** Outputs:
**	none
**
** Returns:
**	gq_rowCount or -1.
*/
II_LONG
IIsw_getRowCount(
IIAPI_GETQINFOPARM	*qinfoParm)
{
	if (qinfoParm &&
	    qinfoParm->gq_genParm.gp_status == IIAPI_ST_SUCCESS &&
	    qinfoParm->gq_mask & IIAPI_GQ_ROW_COUNT)
		return (qinfoParm->gq_rowCount);
	else
		return ((II_LONG)-1);
}


/*{
** Name:	IIsw_getDescriptor	- get data descriptors
**
** Description:
**	Calls IIapi_getDescriptor.
**
** Inputs:
**	stmtHandle	- statement handle
**
** Outputs:
**	gdescParm	- descriptor parameters block
**	errParm		- error parameters block
**
** Returns:
**	Status from call to IIsw_wait.
*/
IIAPI_STATUS
IIsw_getDescriptor(
II_PTR			stmtHandle,
IIAPI_GETDESCRPARM	*gdescParm,
IIAPI_GETEINFOPARM	*errParm)
{
	gdescParm->gd_genParm.gp_callback = NULL;
	gdescParm->gd_genParm.gp_closure = NULL;
	gdescParm->gd_stmtHandle = stmtHandle;
	IIapi_getDescriptor(gdescParm);
	IIsw_wait(&gdescParm->gd_genParm);
	if (gdescParm->gd_genParm.gp_errorHandle)
		IIsw_getErrorInfo(gdescParm->gd_genParm.gp_errorHandle,
			errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (gdescParm->gd_genParm.gp_status);
}


/*{
** Name:	IIsw_setDescriptor	- set data descriptors
**
** Description:
**	Calls IIapi_setDescriptor.
**
** Inputs:
**	stmtHandle	- statement handle
**	parmCount	- number of parameter descriptors
**	parmDesc	- parameter descriptors array
**
** Outputs:
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_setDescriptor(
II_PTR			stmtHandle,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_SETDESCRPARM	sdescParm;

	sdescParm.sd_genParm.gp_callback = NULL;
	sdescParm.sd_genParm.gp_closure = NULL;
	sdescParm.sd_stmtHandle = stmtHandle;
	sdescParm.sd_descriptorCount = parmCount;
	sdescParm.sd_descriptor = parmDesc;
	IIapi_setDescriptor(&sdescParm);
	IIsw_wait(&sdescParm.sd_genParm);
	if (sdescParm.sd_genParm.gp_errorHandle)
		IIsw_getErrorInfo(sdescParm.sd_genParm.gp_errorHandle, errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (sdescParm.sd_genParm.gp_status);
}


/*{
** Name:	IIsw_getColumns	- get columns
**
** Description:
**	Calls IIapi_getColumns.
**
** Inputs:
**	stmtHandle	- statement handle
**	rowCount	- number of rows to fetch
**	colCount	- number of columns to retrieve
**	callback	- pointer to callback function
**	closure		- closure argument to callback function
**
** Outputs:
**	rowCount	- number of rows returned
**	colData		- column/row array
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_getColumns(
II_PTR		stmtHandle,
II_INT2		*rowCount,
II_INT2		colCount,
IIAPI_DATAVALUE	*colData,
IISW_CALLBACK	callback,
II_PTR		closure,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_GETCOLPARM	gcolParm;

	gcolParm.gc_genParm.gp_callback = callback;
	gcolParm.gc_genParm.gp_closure = closure;
	gcolParm.gc_stmtHandle = stmtHandle;
	gcolParm.gc_rowCount = *rowCount;
	gcolParm.gc_columnCount = colCount;
	gcolParm.gc_columnData = colData;
	IIapi_getColumns(&gcolParm);
	IIsw_wait(&gcolParm.gc_genParm);
	*rowCount = gcolParm.gc_rowsReturned;
	errParm->ge_errorHandle = NULL;
	if (gcolParm.gc_genParm.gp_status == IIAPI_ST_SUCCESS &&
			gcolParm.gc_moreSegments)
		return (IISW_MORE_SEGMENTS);
	if (gcolParm.gc_genParm.gp_errorHandle)
		IIsw_getErrorInfo(gcolParm.gc_genParm.gp_errorHandle, errParm);
	return (gcolParm.gc_genParm.gp_status);
}


/*{
** Name:	IIsw_putParms	- put parameters
**
** Description:
**	Calls IIapi_putParms.
**
** Inputs:
**	stmtHandle	- statement handle
**	parmCount	- number of parameter
**	parmData	- parameter array
**	callback	- pointer to callback function
**	closure		- closure argument to callback function
**
** Outputs:
**	errParm		- error parameters block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_putParms(
II_PTR		stmtHandle,
II_INT2		parmCount,
IIAPI_DATAVALUE	*parmData,
IISW_CALLBACK	callback,
II_PTR		closure,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_PUTPARMPARM	putParm;

	putParm.pp_genParm.gp_callback = NULL;
	putParm.pp_genParm.gp_closure = NULL;
	putParm.pp_stmtHandle = stmtHandle;
	putParm.pp_parmCount = parmCount;
	putParm.pp_parmData = parmData;
	putParm.pp_moreSegments = FALSE;
	/* the callback should adjust moreSegments as needed */
	if (callback)
		(*callback)(closure, (II_PTR)&putParm);
	IIapi_putParms(&putParm);
	IIsw_wait(&putParm.pp_genParm);
	errParm->ge_errorHandle = NULL;
	if (putParm.pp_genParm.gp_status == IIAPI_ST_SUCCESS &&
			putParm.pp_moreSegments)
		return (IISW_MORE_SEGMENTS);
	if (putParm.pp_genParm.gp_errorHandle)
		IIsw_getErrorInfo(putParm.pp_genParm.gp_errorHandle, errParm);
	return (putParm.pp_genParm.gp_status);
}


/*{
** Name:	IIsw_putParmsRepeated	- set data descriptors
**
** Description:
**	Calls IIapi_setDescriptor and IIapi_putParms for repeated queries.
**
** Inputs:
**	stmtHandle	- statement handle
**	define		- repeat query is being defined
**	parmCount	- number of parameter
**	parmData	- parameter array
**	qryId		- repeat query ID
**	qryHandle	- repeat query handle
**
** Outputs:
**	errParm		- error parameters block
**
** Returns:
**	IIAPI_ST_OUT_OF_MEMORY or status from IIsw_setDescriptor or
**	IIsw_putParms.
*/
IIAPI_STATUS
IIsw_putParmsRepeated(
II_PTR			stmtHandle,
bool			define,
II_INT2			parmCount,
IIAPI_DESCRIPTOR	*parmDesc,
IIAPI_DATAVALUE		*parmData,
SW_REPEAT_QUERY_ID      *qryId,
II_PTR                  *qryHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	II_INT2			extra_parms;
	i4			len;
	IIAPI_DESCRIPTOR        *ds;
        IIAPI_DATAVALUE         *dv;
	IIAPI_STATUS		status;

	if (define)
		extra_parms = 3;
	else
		extra_parms = 1;

	ds = (IIAPI_DESCRIPTOR *)MEreqmem(0, (u_i4)(parmCount + extra_parms) *
		sizeof(IIAPI_DESCRIPTOR), TRUE, (STATUS *)NULL);
	if (!ds)
	{
		IIsw_getErrorInfo(&IISW_internalError, errParm);
		return (IIAPI_ST_OUT_OF_MEMORY);
	}
	if (define)
	{
		ds->ds_dataType = (ds + 1)->ds_dataType = IIAPI_INT_TYPE;
		(ds + 2)->ds_dataType = IIAPI_CHA_TYPE;
		ds->ds_nullable = (ds + 1)->ds_nullable =
			(ds + 2)->ds_nullable = FALSE;
		ds->ds_length = (ds + 1)->ds_length = sizeof(qryId->int_id[0]);
		(ds + 2)->ds_length = (II_INT2)(len = STlength(qryId->char_id));
		ds->ds_precision = (ds + 1)->ds_precision =
			(ds + 2)->ds_precision = ds->ds_scale =
			(ds + 1)->ds_scale = (ds + 2)->ds_scale = 0;
		ds->ds_columnType = (ds + 1)->ds_columnType = 
			(ds + 2)->ds_columnType = IIAPI_COL_SVCPARM;
		ds->ds_columnName = (ds + 1)->ds_columnName =
			(ds + 2)->ds_columnName = ERx("");
	}
	else
	{
		ds->ds_dataType = IIAPI_HNDL_TYPE;
		ds->ds_nullable = FALSE;
		ds->ds_length = sizeof(II_PTR);
		ds->ds_precision = ds->ds_scale = 0;
		ds->ds_columnType = IIAPI_COL_SVCPARM;
		ds->ds_columnName = ERx("");
	}
	if (parmCount)
		MEcopy(parmDesc, parmCount * sizeof(IIAPI_DESCRIPTOR),
			ds + extra_parms);
	status = IIsw_setDescriptor(stmtHandle, (II_INT2)(parmCount +
		extra_parms), ds, errParm);
	MEfree((PTR)ds);
	if (status != IIAPI_ST_SUCCESS)
		return (status);

	dv = (IIAPI_DATAVALUE *)MEreqmem(0, (u_i4)(parmCount + extra_parms) *
		sizeof(IIAPI_DATAVALUE), TRUE, (STATUS *)NULL);
	if (!dv)
	{
		IIsw_getErrorInfo(&IISW_internalError, errParm);
		return (IIAPI_ST_OUT_OF_MEMORY);
	}
	if (define)
	{
		dv->dv_null = (dv + 1)->dv_null = (dv + 2)->dv_null = FALSE;
		dv->dv_length = (dv + 1)->dv_length = sizeof(qryId->int_id[0]);
		(dv + 2)->dv_length = (II_INT2)len;
		dv->dv_value = &(qryId->int_id[0]);
		(dv + 1)->dv_value = &(qryId->int_id[1]);
		(dv + 2)->dv_value = qryId->char_id;
	}
	else
	{
		dv->dv_null = FALSE;
		dv->dv_length = sizeof(II_PTR);
		dv->dv_value = qryHandle;
	}
	if (parmCount)
		MEcopy(parmData, parmCount * sizeof(IIAPI_DATAVALUE),
			dv + extra_parms);
	status = IIsw_putParms(stmtHandle, (II_INT2)(parmCount + extra_parms),
		dv, NULL, NULL, errParm);
	MEfree((PTR)dv);
	return (status);
}


/*{
** Name:	IIsw_close	- close a query or dbevent retrieval
**
** Description:
**	Calls IIapi_close.
**
** Inputs:
**	stmtHandle	- statement handle
**
** Outputs:
**	errParm		- error parameters block
**
** Returns:
**	If previous API call succeeded and IIapi_close fails, return the
**	IIsw_wait status, otherwise return the input status.
*/
IIAPI_STATUS
IIsw_close(
II_PTR		stmtHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_CLOSEPARM	closeParm;

	closeParm.cl_genParm.gp_callback = NULL;
	closeParm.cl_genParm.gp_closure = NULL;
	closeParm.cl_stmtHandle = stmtHandle;
	IIapi_close(&closeParm);
	IIsw_wait(&closeParm.cl_genParm);
	if (closeParm.cl_genParm.gp_errorHandle)
		IIsw_getErrorInfo(closeParm.cl_genParm.gp_errorHandle, errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (closeParm.cl_genParm.gp_status);
}


/*{
** Name:	IIsw_wait - wait for an API call to complete
**
** Description:
**	Waits for an API call to complete.
**
** Inputs:
**	genParm		- generic parameters block
**
** Outputs:
**	genParm		- updated generic parameters block
**
** Returns:
**	none
*/
II_VOID
IIsw_wait(
IIAPI_GENPARM	*genParm)
{
	IIAPI_WAITPARM	waitParm;

	waitParm.wt_timeout = -1;
	while (!genParm->gp_completed)
		IIapi_wait(&waitParm);
}


/*{
** Name:	IIsw_getErrorInfo - get error information
**
** Description:
**	Calls IIapi_getErrorInfo.
**
** Inputs:
**	errHandle	- error handle
**
** Outputs:
**	errParm		- error parameters block
**
** Returns:
**	none
*/
II_VOID
IIsw_getErrorInfo(
II_PTR			errHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	if (errHandle == &IISW_internalError)
	{
		errParm->ge_type = IIAPI_GE_ERROR;
		STncpy(errParm->ge_SQLSTATE, II_SS5000B_INTERNAL_ERROR,
			II_SQLSTATE_LEN );
		errParm->ge_SQLSTATE[ II_SQLSTATE_LEN ] = EOS;
		errParm->ge_errorCode = E_SW0001_INTERNAL_ERROR;
		errParm->ge_message = msg;
		errParm->ge_serverInfoAvail = FALSE;
	}
	else if (errHandle)
	{
		errParm->ge_errorHandle = errHandle;
		IIapi_getErrorInfo(errParm);
	}
}

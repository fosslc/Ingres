/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <iiapi.h>
# include <sapiw.h>
# include "sapiwi.h"

/**
** Name:	apitran.c - synchronous API wrapper transaction functions
**
** Description:
**	Defines
**		IIsw_commit		- commit a transaction
**		IIsw_rollback		- roll back a transaction
**		IIsw_registerXID	- register 2PC transaction ID
**		IIsw_prepareCommit	- prepare to commit
**		IIsw_releaseXID		- release 2PC transaction ID
**
** History:
**	15-apr-98 (joea)
**		Created.
**	25-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	15-mar-99 (abbjo03)
**		Only call IIsw_getErrorInfo if the error handle is not null.
**	24-sep-99 (abbjo03) bug 99005
**		Add IIsw_releaseXID.
**/

/*{
** Name:	IIsw_commit - commit a transaction
**
** Description:
**	Commits a given transaction and free the transaction handle.
**
** Inputs:
**	tranHandle	- transaction handle
**
** Outputs:
**	tranHandle	- transaction handle
**	errParm		- error parameter block
**
** Returns:
**	IIAPI_ST_SUCCESS or status from call to IIsw_wait.
*/
IIAPI_STATUS
IIsw_commit(
II_PTR	*tranHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_COMMITPARM	commitParm;

	/* if bad parameter, do a benign return */
	if (*tranHandle == NULL)
		return (IIAPI_ST_SUCCESS);

	commitParm.cm_genParm.gp_callback = NULL;
	commitParm.cm_genParm.gp_closure = NULL;
	commitParm.cm_tranHandle = *tranHandle;

	IIapi_commit(&commitParm);
	IIsw_wait(&commitParm.cm_genParm);
	if (commitParm.cm_genParm.gp_errorHandle)
	{
		IIsw_getErrorInfo(commitParm.cm_genParm.gp_errorHandle,
			errParm);
	}
	else
	{
		/* reset transaction handle */
		*tranHandle = NULL;
		errParm->ge_errorHandle = NULL;
	}
	return (commitParm.cm_genParm.gp_status);
}


/*{
** Name:	IIsw_rollback - rollback a transaction
**
** Description:
**	Rolls back a given transaction.
**
** Inputs:
**	tranHandle	- transaction handle
**
** Outputs:
**	tranHandle	- transaction handle
**	errParm		- error parameter block
**
** Returns:
**	IIAPI_ST_SUCCESS or status from call to IIsw_wait.
*/
IIAPI_STATUS
IIsw_rollback(
II_PTR	*tranHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_ROLLBACKPARM	rollbackParm;

	/* if bad parameter, do a benign return */
	if (*tranHandle == NULL)
		return (IIAPI_ST_SUCCESS);

	rollbackParm.rb_genParm.gp_callback = NULL;
	rollbackParm.rb_genParm.gp_closure = NULL;
	rollbackParm.rb_tranHandle = *tranHandle;
	rollbackParm.rb_savePointHandle = NULL;

	IIapi_rollback(&rollbackParm);
	IIsw_wait(&rollbackParm.rb_genParm);
	if (rollbackParm.rb_genParm.gp_errorHandle)
	{
		IIsw_getErrorInfo(rollbackParm.rb_genParm.gp_errorHandle,
			errParm);
	}
	else
	{
		/* reset transaction handle */
		*tranHandle = NULL;
		errParm->ge_errorHandle = NULL;
	}
	return (rollbackParm.rb_genParm.gp_status);
}


/*{
** Name:	IIsw_registerXID - register 2PC transaction ID
**
** Description:
**	Registers a two phase commit transaction ID.
**
** Inputs:
**	lowTran		- low order bytes of the transaction ID
**	highTran	- high order bytes of the transaction ID
**	tranName	- unique transaction name
**
** Outputs:
**	tranHandle	- transaction handle
**
** Returns:
**	The status from the call to IIapi_registerXID.
*/
IIAPI_STATUS
IIsw_registerXID(
II_UINT4		lowTran,
II_UINT4		highTran,
II_CHAR			*tranName,
II_PTR			*tranHandle)
{
	IIAPI_REGXIDPARM	regxidParm;

	regxidParm.rg_tranID.ti_type = IIAPI_TI_IIXID;
	regxidParm.rg_tranID.ti_value.iiXID.ii_tranID.it_highTran = highTran;
	regxidParm.rg_tranID.ti_value.iiXID.ii_tranID.it_lowTran = lowTran;
	STcopy(tranName, regxidParm.rg_tranID.ti_value.iiXID.ii_tranName);

	IIapi_registerXID(&regxidParm);
	if (regxidParm.rg_status == IIAPI_ST_SUCCESS)
		*tranHandle = regxidParm.rg_tranIdHandle;
	else
		*tranHandle = NULL;

	return (regxidParm.rg_status);
}


/*{
** Name:	IIsw_prepareCommit - prepare to commit
**
** Description:
**	Begins a two phase commit transaction.
**
** Inputs:
**	tranHandle	- transaction handle
**
** Outputs:
**	tranHandle	- transaction handle
**	errParm		- error parameter block
**
** Returns:
**	IIAPI_ST_SUCCESS or status from call to IIsw_wait.
*/
IIAPI_STATUS
IIsw_prepareCommit(
II_PTR	*tranHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_PREPCMTPARM	prepParm;

	prepParm.pr_genParm.gp_callback = NULL;
	prepParm.pr_genParm.gp_closure = NULL;
	prepParm.pr_tranHandle = *tranHandle;
	IIapi_prepareCommit(&prepParm);
	IIsw_wait(&prepParm.pr_genParm);
	if (prepParm.pr_genParm.gp_errorHandle)
	{
		IIsw_getErrorInfo(prepParm.pr_genParm.gp_errorHandle, errParm);
	}
	else
	{
		*tranHandle = prepParm.pr_tranHandle;
		errParm->ge_errorHandle = NULL;
	}
	return (prepParm.pr_genParm.gp_status);
}


/*{
** Name:	IIsw_releaseXID - release 2PC transaction ID
**
** Description:
**	Releases a two phase commit transaction ID.
**
** Inputs:
**	tranHandle	- transaction ID handle
**
** Outputs:
**	tranHandle	- transaction handle
**
** Returns:
**	The status from the call to IIapi_releaseXID.
*/
IIAPI_STATUS
IIsw_releaseXID(
II_PTR	*tranHandle)
{
	IIAPI_RELXIDPARM	relxidParm;

	relxidParm.rl_tranIdHandle = *tranHandle;
	IIapi_releaseXID(&relxidParm);
	if (relxidParm.rl_status == IIAPI_ST_SUCCESS)
		*tranHandle = NULL;

	return (relxidParm.rl_status);
}

/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <iiapi.h>
# include <sapiw.h>
# include "sapiwi.h"

/**
** Name:	apiconn.c - synchronous API wrapper connection functions
**
** Description:
**	Defines
**		IIsw_connect		- connect to a database
**		IIsw_dbConnect		- connect to database as given user
**		IIsw_disconnect		- disconnect from a database
**		IIsw_setConnParam	- set connection parameter
**
** History:
**	10-apr-98 (joea)
**		Created.
**	04-may-98 (joea)
**		Add IIsw_dbConnect.
**	15-may-98 (joea)
**		Add tranHandle parameter to IIsw_connect and IIsw_dbConnect.
**		Correct connHandle assignment in IIsw_disconnect.
**	25-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	15-mar-99 (abbjo03)
**		Only call IIsw_getErrorInfo if the error handle is not null.
**	20-apr-99 (abbjo03)
**		Add co_type initialization in IIsw_connect.
**/

/*{
** Name:	IIsw_connect - connect to a database
**
** Description:
**	Opens connection to a specified database.
**
** Inputs:
**	target		- vnode name, database name and DBMS type
**	username	- operating system user name
**	password	- operating system user password
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**
** Outputs:
**	connHandle	- connection handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_connect(
II_CHAR	*target,
II_CHAR	*username,
II_CHAR	*password,
II_PTR	*connHandle,
II_PTR	*tranHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_CONNPARM	connParm;

	connParm.co_genParm.gp_callback = NULL;
	connParm.co_genParm.gp_closure = NULL;
	connParm.co_target = target;
	connParm.co_username = username;
	connParm.co_password = password;
	connParm.co_timeout = -1;
	connParm.co_connHandle = *connHandle;
	connParm.co_tranHandle = (tranHandle ? *tranHandle : NULL);
	connParm.co_type = IIAPI_CT_SQL;

	IIapi_connect(&connParm);
	IIsw_wait(&connParm.co_genParm);
	if (connParm.co_genParm.gp_errorHandle)
	{
		IIsw_getErrorInfo(connParm.co_genParm.gp_errorHandle, errParm);
	}
	else
	{
		*connHandle = connParm.co_connHandle;
		if (tranHandle)
			*tranHandle = connParm.co_tranHandle;
		errParm->ge_errorHandle = NULL;
	}
	return (connParm.co_genParm.gp_status);
}


/*{
** Name:	IIsw_dbConnect - connect to database as given user
**
** Description:
**	Opens connection to a specified database as a specified database user.
**
** Inputs:
**	target		- vnode name, database name and DBMS type
**	dbUser		- database user name
**	dbPassword	- database user password
**	connHandle	- connection handle
**	tranHandle	- transaction handle
**
** Outputs:
**	connHandle	- connection handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_dbConnect(
II_CHAR	*target,
II_CHAR	*dbUser,
II_CHAR	*dbPassword,
II_PTR	*connHandle,
II_PTR	*tranHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_STATUS	status;

	if (dbUser && *dbUser != EOS)
	{
		status = IIsw_setConnParam(connHandle, IIAPI_CP_EFFECTIVE_USER,
			dbUser, errParm);
		if (status != IIAPI_ST_SUCCESS)
			return (status);
		if (dbPassword && *dbPassword != EOS)
		{
			status = IIsw_setConnParam(connHandle,
				IIAPI_CP_DBMS_PASSWORD, dbPassword, errParm);
			if (status != IIAPI_ST_SUCCESS)
				return (status);
		}
	}
	return (IIsw_connect(target, NULL, NULL, connHandle, tranHandle,
		errParm));
}


/*{
** Name:	IIsw_disconnect - disconnect from a database
**
** Description:
**	Disconnects from a database associated with a certain connection.
**
** Inputs:
**	connHandle	- connection handle to be closed
**
** Outputs:
**	connHandle	- connection handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_disconnect(
II_PTR	*connHandle,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_DISCONNPARM	disconnParm;

	disconnParm.dc_genParm.gp_callback = NULL;
	disconnParm.dc_genParm.gp_closure = NULL;
	disconnParm.dc_connHandle = *connHandle;

	IIapi_disconnect(&disconnParm);
	IIsw_wait(&disconnParm.dc_genParm);

	/* Reset connection handle */
	*connHandle = NULL;
	if (disconnParm.dc_genParm.gp_errorHandle)
		IIsw_getErrorInfo(disconnParm.dc_genParm.gp_errorHandle,
			errParm);
	else
		errParm->ge_errorHandle = NULL;
	return (disconnParm.dc_genParm.gp_status);
}


/*{
** Name:	IIsw_setConnParam - set miscellaneous connection parameter
**
** Description:
**	Sets an II_CHAR miscellaneous connection parameter.
**
** Inputs:
**	connHandle	- connection handle to be closed
**	paramID		- parameter ID
**	paramValue	- parameter value
**
** Outputs:
**	connHandle	- connection handle
**	errParm		- error parameter block
**
** Returns:
**	Status from genParm.gp_status.
*/
IIAPI_STATUS
IIsw_setConnParam(
II_PTR	*connHandle,
II_LONG	paramID,
II_CHAR	*paramValue,
IIAPI_GETEINFOPARM	*errParm)
{
	IIAPI_SETCONPRMPARM	setconnParm;

	setconnParm.sc_genParm.gp_callback = NULL;
	setconnParm.sc_genParm.gp_closure = NULL;
	setconnParm.sc_connHandle = *connHandle;
	setconnParm.sc_paramID = paramID;
	setconnParm.sc_paramValue = paramValue;
	IIapi_setConnectParam(&setconnParm);
	IIsw_wait(&setconnParm.sc_genParm);
	if (setconnParm.sc_genParm.gp_errorHandle)
	{
		IIsw_getErrorInfo(setconnParm.sc_genParm.gp_errorHandle,
			errParm);
	}
	else
	{
		/* save connection handle */
		*connHandle = setconnParm.sc_connHandle;
		errParm->ge_errorHandle = NULL;
	}
	return (setconnParm.sc_genParm.gp_status);
}

/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apimisc.h>
# include <apienhnd.h>
# include <apitrace.h>

/*
** Name: apiseten.c
**
** Description:
**	This file contains the IIapi_setEnvParam() function interface.
**
**	IIapi_setEnvParam		Set environment parameters.
**
** History:
**	04-Aug-98 (rajus01)
**	    Created.
*/

/*
** Name: IIapi_setEnvParam
**
** Description:
**	This function provide an interface for the frontend application
**	to provide flexibility to modify environmental parameters to be
**	passed to a DBMS server.
**	Please refer to the API user specification for function description.
**
**	setEnvParm	input and output parameters for IIapi_setEnvParam()
**
** Return value:
**	none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	04-Aug-98 (rajus01)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_setEnvParam( IIAPI_SETENVPRMPARM II_FAR *setEnvParm )
{
    IIAPI_ENVHNDL	*envHndl = NULL;
    
    IIAPI_TRACE( IIAPI_TR_TRACE ) 
	( "IIapi_setEnvParam: set environment parameter\n" );

    /*
    ** Validate Input parameters
    */
    if ( ! setEnvParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setEnvParam: null function parameters\n" );
	return;
    }

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setEnvParam: API is not initialized\n" );
	setEnvParm->se_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }

    envHndl = (IIAPI_ENVHNDL *)setEnvParm->se_envHandle;

    if ( ! IIapi_isEnvHndl( envHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setEnvParam: invalid environment handle\n" );
	setEnvParm->se_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_setEnvParam: envHndl = %p\n",  envHndl);

    if( setEnvParm->se_paramID < IIAPI_EP_BASE  || 
	setEnvParm->se_paramID > IIAPI_EP_TOP )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	( "IIapi_setEnvParam: invalid param ID %d\n", setEnvParm->se_paramID );
	setEnvParm->se_status = IIAPI_ST_FAILURE;
	return;

    }

    setEnvParm->se_status = IIapi_setEnvironParm( setEnvParm );

    return;
}

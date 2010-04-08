/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiautil.c 
**
** Description:
**	OpenAPI asynchronous demo utility module.
**
**	This module provides routine to perform common API calls
**	synchronously.
**
**	The following functions are provided:  
**	
**	IIdemo_conn()		Call IIapi_connect().
**	IIdemo_disconn()	Call IIapi_disconnect().
**	IIdemo_abort() 		Call IIapi_abort().
**	IIdemo_autocommit()	Call IIapi_autocommit().
**	IIdemo_query()		Call IIapi_{query, getQueryInfo, close}().
**	IIdemo_checkError()	Print API call status and error information.
*/

# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>
# include "apiautil.h"


/*
** Local functions.
*/

static	void	IIdemo_wait( IIAPI_GENPARM *, II_BOOL );


    
/*
** Name: IIdemo_conn
**
** Description:
**	Establish database connection using IIapi_connect().
*/
    
IIAPI_STATUS 
IIdemo_conn( II_PTR env, char *dbname, II_PTR *connHandle, II_BOOL verbose )
{
    IIAPI_CONNPARM	connParm;
    
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_type = IIAPI_CT_SQL;
    connParm.co_target =  dbname;
    connParm.co_connHandle = env;
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    /*
    ** Make sync request
    */
    if ( verbose )  printf( "\t\tCalling IIapi_connect()\n" );
    IIapi_connect( &connParm );
    IIdemo_wait( &connParm.co_genParm, verbose );

    /*
    ** Check and return results
    **
    ** If an error occurs, the connection handle returned must
    ** be freed by aborting the failed connection.
    */
    IIdemo_checkError( "IIapi_connect()", &connParm.co_genParm ); 

    if ( connParm.co_genParm.gp_status == IIAPI_ST_SUCCESS )
	*connHandle = connParm.co_connHandle;
    else  if ( connParm.co_connHandle )
	IIdemo_abort( &connParm.co_connHandle, verbose );

    return( connParm.co_genParm.gp_status );
}


/*
** Name: IIdemo_disconn
**
** Description:
**	Drop database connection using IIapi_disconnect().
*/
    
IIAPI_STATUS
IIdemo_disconn( II_PTR *connHandle, II_BOOL verbose )
{
    IIAPI_DISCONNPARM	disconnParm;
    
    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = *connHandle;
    *connHandle = NULL;
    
    /*
    ** Make sync request.
    */
    if ( verbose )  printf( "\t\tCalling IIapi_disconnect()\n" );
    IIapi_disconnect( &disconnParm );
    IIdemo_wait( &disconnParm.dc_genParm, verbose );
    
    /*
    ** Check results.
    */
    IIdemo_checkError( "IIapi_disconnect()", &disconnParm.dc_genParm ); 
    
    return( disconnParm.dc_genParm.gp_status );
}

 
/*
** Name: IIdemo_abort
**
** Description:
**	Abort database connection using IIapi_abort().
*/

IIAPI_STATUS 
IIdemo_abort( II_PTR * connHandle, II_BOOL verbose )
{
    IIAPI_ABORTPARM	abortParm;
    
    abortParm.ab_genParm.gp_callback = NULL;
    abortParm.ab_genParm.gp_closure  = NULL;
    abortParm.ab_connHandle = *connHandle;
    *connHandle = NULL;

    /*
    ** Make sync request.
    */
    if ( verbose )  printf( "\t\tCalling IIapi_abort()\n" );
    IIapi_abort( &abortParm );
    IIdemo_wait( &abortParm.ab_genParm, verbose );

    /*
    ** Check results.
    */
    IIdemo_checkError( "IIapi_abort()", &abortParm.ab_genParm );

    return( abortParm.ab_genParm.gp_status );
}


/*
** Name: IIdemo_autocommit
**
** Description
**	Enable/Disable autocommit using IIapi_autocommit().
*/

IIAPI_STATUS
IIdemo_autocommit( II_PTR *connHandle, II_PTR  *tranHandle, II_BOOL verbose )
{
    IIAPI_AUTOPARM	autoParm;
    II_PTR		nullHandle = NULL;

    autoParm.ac_genParm.gp_callback = NULL;
    autoParm.ac_genParm.gp_closure = NULL;
    autoParm.ac_connHandle = *connHandle;
    autoParm.ac_tranHandle = *tranHandle;

    /*
    ** Make sync request.
    */
    if ( verbose )  printf( "\t\tCalling IIapi_autocommit()\n" );
    IIapi_autocommit( &autoParm );
    IIdemo_wait( &autoParm.ac_genParm, verbose );

    /*
    ** Check and return results.  
    **
    ** If an error occurs enabling autocommit, the transaction
    ** handle returned must be freed by disabling autocommit.
    ** This is done with a recursive call to this routine.
    */
    IIdemo_checkError( "IIapi_autocommit()", &autoParm.ac_genParm);

    if ( autoParm.ac_genParm.gp_status == IIAPI_ST_SUCCESS )
	*tranHandle = autoParm.ac_tranHandle;
    else  if ( ! *tranHandle  &&  autoParm.ac_tranHandle )
	IIdemo_autocommit( &nullHandle, &autoParm.ac_tranHandle, verbose );

    return( autoParm.ac_genParm.gp_status );
}


/*
** Name: IIdemo_query
**
** Description:
**	Executes a basic query using the API functions
**	IIapi_query(), IIapi_getQueryInfo(), IIapi_close().
*/

IIAPI_STATUS
IIdemo_query( II_PTR *connHandle, II_PTR *tranHandle, 
	      char *queryText, II_BOOL verbose )
{
    IIAPI_QUERYPARM	queryParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_STATUS        return_status; 

    /*
    ** Call IIapi_query().
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = *connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (char *)queryText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = *tranHandle;
    queryParm.qy_stmtHandle = ( II_PTR )NULL;

    /*
    ** Make sync request.
    */
    if ( verbose )  printf( "\t\tCalling IIapi_query()\n" );
    IIapi_query( &queryParm );
    IIdemo_wait( &queryParm.qy_genParm, verbose );

    /*
    ** Check and return results.
    **
    ** The transaction handle is always returned (but may not change).
    ** If an error occurs, IIapi_close() must still be called if a
    ** statement handle is returned.
    */
    IIdemo_checkError( "IIapi_query()", &queryParm.qy_genParm);
    *tranHandle = queryParm.qy_tranHandle;
    return_status = queryParm.qy_genParm.gp_status;

    if ( queryParm.qy_genParm.gp_status == IIAPI_ST_SUCCESS )
    {
	IIAPI_GETQINFOPARM	getQInfoParm;

	/*
	** Call IIapi_getQueryInfo().
	*/
	getQInfoParm.gq_genParm.gp_callback = NULL;
	getQInfoParm.gq_genParm.gp_closure = NULL;
	getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;
    
	/*
	** Make sync call.
	*/
	if ( verbose )  printf( "\t\tCalling IIapi_getQueryInfo()\n" );
	IIapi_getQueryInfo( &getQInfoParm );
	IIdemo_wait( &getQInfoParm.gq_genParm, verbose );
   
	/*
	** Check and return results.
	*/
	IIdemo_checkError( "IIapi_getQueryInfo()", &getQInfoParm.gq_genParm );
	return_status = getQInfoParm.gq_genParm.gp_status;
    }

    if ( queryParm.qy_stmtHandle )
    {
	/* 
	** Call IIapi_close().
	*/
	closeParm.cl_genParm.gp_callback = NULL;
	closeParm.cl_genParm.gp_closure = NULL;
	closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

	/*
	** Make sync call.
	*/
	if ( verbose )  printf( "\t\tCalling IIapi_close()\n" );
	IIapi_close( &closeParm );
	IIdemo_wait( &closeParm.cl_genParm, verbose );

	/*
	** Check and return results.
	**
	** Only return the most significant status.
	*/
	IIdemo_checkError( "IIapi_close()", &closeParm.cl_genParm );
	if ( closeParm.cl_genParm.gp_status > return_status )
	    return_status = closeParm.cl_genParm.gp_status;
    }

    return( return_status );
}


/*
** Name: IIdemo_checkError
**
** Description:
**	Display status and error information resulting from
**	an API request.
*/

void 
IIdemo_checkError( char *func, IIAPI_GENPARM *genParm )
{
    /*
    ** Print (bad) status.
    */
    if ( genParm->gp_status >= IIAPI_ST_ERROR )
    {
	char *ptr, val[33];

	switch( genParm->gp_status )
	{
	case IIAPI_ST_ERROR : 		ptr = "IIAPI_ST_ERROR"; break;
	case IIAPI_ST_FAILURE :		ptr = "IIAPI_ST_FAILURE"; break;
	case IIAPI_ST_NOT_INITIALIZED :	ptr = "IIAPI_ST_NOT_INITIALIZED"; break;
	case IIAPI_ST_INVALID_HANDLE :	ptr = "IIAPI_ST_INVALID_HANDLE"; break;
	case IIAPI_ST_OUT_OF_MEMORY :	ptr = "IIAPI_ST_OUT_OF_MEMORY"; break;
	default : 
	    ptr = val; 
	    sprintf( val, "%d", genParm->gp_status ); 
	    break;
	}

        printf( "\t%s status = %s\n", func, ptr );
    }

    /*
    ** Print error messages (if any).
    */
    if ( genParm->gp_errorHandle )
    {
	IIAPI_GETEINFOPARM	getErrParm; 
	char			*ptr;

	/*
	** Provide initial error handle.
	*/
	getErrParm.ge_errorHandle = genParm->gp_errorHandle;

	/*
	** Call IIapi_getErrorInfo() in loop until no data.
	*/
	do
	{ 
	    IIapi_getErrorInfo( &getErrParm );
	    if ( getErrParm.ge_status != IIAPI_ST_SUCCESS )  break;

	    /*
	    ** Print error message info.
	    */
	    switch( getErrParm.ge_type )
	    {
	    case IIAPI_GE_ERROR	:	ptr = "ERROR"; break;
	    case IIAPI_GE_WARNING :	ptr = "WARNING"; break;
	    case IIAPI_GE_MESSAGE :	ptr = "USER MESSAGE"; break;
	    default :			ptr = "?"; break;
	    }

	    printf( "\tInfo: %s '%s' 0x%x: '%s'\n",
		   ptr, getErrParm.ge_SQLSTATE, getErrParm.ge_errorCode,
		   getErrParm.ge_message ? getErrParm.ge_message : "NULL" );

	} while( 1 ); /* next message */
    }

    return;
}



/*
** Name: IIdemo_wait
**
** Description:
**	Calls IIapi_wait() until an API request has completed.
*/

static void
IIdemo_wait( IIAPI_GENPARM *genParm, II_BOOL verbose )
{
    IIAPI_WAITPARM waitParm;

    while( genParm->gp_completed == FALSE )
    {
	if ( verbose )  printf( "\t\tCalling IIapi_wait()\n" );

	waitParm.wt_timeout = -1;
	IIapi_wait( &waitParm );

	if ( waitParm.wt_status != IIAPI_ST_SUCCESS )
	{
	    genParm->gp_status = waitParm.wt_status;
	    break;
	}
    }

    return;
}


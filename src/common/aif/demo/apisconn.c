/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisconn.c  
**
** Description:
**	Demonstrates using IIapi_connect(),IIapi_setConnectParam(), 
**	IIapi_disconnect() and IIapi_abort()
**
**  Following actions are demonstrate in the main()
**	Connect (no conn parms)
**	Disconnect
**	Set Connection Parameters
**	Connect (with conn parms)
**	Abort Connection
**
** Command syntax: apisconn database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>

static  void IIdemo_init( II_PTR *);
static	void IIdemo_term( II_PTR *);



/*
** Name: main - main loop
**
** Description:
**	This function is the main loop of the sample code.
*/

int
main( int argc, char** argv ) 
{
    II_PTR		connHandle = (II_PTR)NULL;
    II_PTR		tranHandle = (II_PTR)NULL;
    II_PTR		envHandle = (II_PTR)NULL;
    IIAPI_SETCONPRMPARM	setConPrmParm;
    IIAPI_CONNPARM	connParm;
    IIAPI_DISCONNPARM	disconnParm;
    IIAPI_ABORTPARM	abortParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    II_LONG		longvalue; 

    if ( argc != 2 )
    {
	printf( "usage: apisconn [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(&envHandle);
			 
    /*
    **  Connect with no connection parameters.
    */
    printf( "apisconn: establishing connection\n" );

    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  argv[1];
    connParm.co_connHandle = envHandle;
    connParm.co_tranHandle = NULL;
    connParm.co_type = IIAPI_CT_SQL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    while( connParm.co_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    connHandle = connParm.co_connHandle;
    tranHandle = connParm.co_tranHandle;
    
    /*
    **  Disconnect
    */
    printf( "apisconn: releasing connection\n");

    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = connHandle;
    
    IIapi_disconnect( &disconnParm );
    
    while( disconnParm.dc_genParm.gp_completed == FALSE )
         IIapi_wait( &waitParm );

    /*
    **  Set connection parameter.
    */
    printf( "apisconn: set connection parameter\n" );

    setConPrmParm.sc_genParm.gp_callback = NULL;
    setConPrmParm.sc_connHandle = envHandle;
    setConPrmParm.sc_paramID = IIAPI_CP_DATE_FORMAT;
    setConPrmParm.sc_paramValue =(II_PTR)&longvalue;
    longvalue = IIAPI_CPV_DFRMT_YMD;

    IIapi_setConnectParam( &setConPrmParm );

    while( setConPrmParm.sc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
	      
    connHandle = setConPrmParm.sc_connHandle; 
			 
    /*
    **  Connect with connection parameter.
    */
    printf( "apisconn: establishing connection\n" );

    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  argv[1];
    connParm.co_connHandle = connHandle;
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    while( connParm.co_genParm.gp_completed == FALSE )
         IIapi_wait( &waitParm );

    connHandle = connParm.co_connHandle;
    tranHandle = connParm.co_tranHandle;
    
    /*
    **  Abort the connection.
    */
    printf( "apisconn: abort connection\n");
    
    abortParm.ab_genParm.gp_callback = NULL;
    abortParm.ab_genParm.gp_closure = NULL;
    abortParm.ab_connHandle = connHandle;
    
    IIapi_abort( &abortParm );
    
    while( abortParm.ab_genParm.gp_completed == FALSE )
         IIapi_wait( &waitParm );

    IIdemo_term(&envHandle);

    return( 0 );
}


/*
** Name: IIdemo_init
**
** Description:
**	Initialize API access.
**
** Input:
**	None.
**
** Output:
**	envHandle	Environment handle.
**
** Return value:
**	None.
*/

static void
IIdemo_init( II_PTR *envHandle )
{
    IIAPI_INITPARM  initParm;

    printf( "IIdemo_init: initializing API\n" );
    initParm.in_version = IIAPI_VERSION_2; 
    initParm.in_timeout = -1;
    IIapi_initialize( &initParm );

    *envHandle = initParm.in_envHandle; 
    return;
}


/*
** Name: IIdemo_term
**
** Description:
**	Terminate API access.
**
** Input:
**	envHandle	Environment handle.
**
** Output:
**	envHandle	Environment handle.
**
** Return value:
**	None.
*/

static void
IIdemo_term( II_PTR *envHandle )
{
    IIAPI_RELENVPARM	relEnvParm;
    IIAPI_TERMPARM	termParm;

    printf( "IIdemo_term: releasing environment resources\n" );
    relEnvParm.re_envHandle = *envHandle;
    IIapi_releaseEnv(&relEnvParm);

    printf( "IIdemo_term: shutting down API\n" );
    IIapi_terminate( &termParm );

    *envHandle = NULL;
    return;
}


/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisroll.c 
**
** Description:
**	Demonstrates using IIapi_savePoint() and IIapi_rollback()
**
** Following actions are demonstrated in the main()
**	Create savepoint
**	Rollback to savepoint 
**	Rollback transaction.
**
** Command syntax: apisroll database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init();
static	void IIdemo_term();
static	void IIdemo_conn( char *, II_PTR * );
static	void IIdemo_disconn( II_PTR * );
static	void IIdemo_query( II_PTR *, II_PTR *, char *, char * );

static	char queryText[] = "create table api_demo_roll( c1 integer )";



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
    II_PTR		savePtHandle = (II_PTR)NULL;
    IIAPI_SAVEPTPARM	savePtParm;
    IIAPI_ROLLBACKPARM	rollbackParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    char		savePtName[] = "save_point";

    if ( argc != 2 )
    {
	printf( "usage: apisroll [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(); 
    IIdemo_conn(argv[1],&connHandle);
    IIdemo_query(&connHandle,&tranHandle,"create table",queryText);

    /*
    **  Create savepoint.
    */
    printf( "apisroll: creating save point\n");

    savePtParm.sp_genParm.gp_callback = NULL;
    savePtParm.sp_genParm.gp_closure = NULL;
    savePtParm.sp_tranHandle = tranHandle;
    savePtParm.sp_savePoint = (char * ) savePtName; 

    IIapi_savePoint( &savePtParm );

    while(savePtParm.sp_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    savePtHandle = savePtParm.sp_savePointHandle; 

    /*
    **  Rollback to save point.
    */
    printf( "apisroll: rolling back to save point\n");

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = tranHandle;
    rollbackParm.rb_savePointHandle = savePtHandle; 

    IIapi_rollback( &rollbackParm );

    while(rollbackParm.rb_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    /*
    **  Rollback transaction.
    */
    printf( "apisroll: rolling back transaction\n");

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = tranHandle;
    rollbackParm.rb_savePointHandle = NULL; 

    IIapi_rollback( &rollbackParm );

    while(rollbackParm.rb_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    IIdemo_disconn(&connHandle);
    IIdemo_term();

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
**	None.
**
** Return value:
**	None.
*/

static void
IIdemo_init()
{
    IIAPI_INITPARM  initParm;

    printf( "IIdemo_init: initializing API\n" );
    initParm.in_version = IIAPI_VERSION_1; 
    initParm.in_timeout = -1;
    IIapi_initialize( &initParm );

    return;
}


/*
** Name: IIdemo_term
**
** Description:
**	Terminate API access.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Return value:
**	None.
*/

static void
IIdemo_term()
{
    IIAPI_TERMPARM  termParm;

    printf( "IIdemo_term: shutting down API\n" );
    IIapi_terminate( &termParm );

    return;
}


/*
** Name: IIdemo_conn
**
** Description:
**	Open connection with target Database.
**
** Input:
**	dbname		Database name.
**
** Output:
**	connHandle	Connection handle.
**
** Return value:
**	None.
*/
    
static void
IIdemo_conn( char *dbname, II_PTR *connHandle )
{
    IIAPI_CONNPARM	connParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    
    printf( "IIdemo_conn: establishing connection\n" );

    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  dbname;
    connParm.co_connHandle = NULL;
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    while( connParm.co_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    *connHandle = connParm.co_connHandle;
    return;
}


/*
** Name: IIdemo_disconn
**
** Description:
**	Release DBMS connection.
**
** Input:
**	connHandle	Connection handle.
**
** Output:
**	connHandle	Connection handle.
**
** Return value:
**	None.
*/
    
static void
IIdemo_disconn( II_PTR *connHandle )
{
    IIAPI_DISCONNPARM	disconnParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    
    printf( "IIdemo_disconn: releasing connection\n" );

    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = *connHandle;
    
    IIapi_disconnect( &disconnParm );
    
    while( disconnParm.dc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    
    *connHandle = NULL;
    return;
}


/*
** Name: IIdemo_query
**
** Description:
**	Execute SQL statement taking no parameters and returning no rows.
**
** Input:
**	connHandle	Connection handle.
**	tranHandle	Transaction handle.
**	desc		Query description.
**	queryText	SQL query text.
**
** Output:
**	connHandle	Connection handle.
**	tranHandle	Transaction handle.
**
** Return value:
**	None.
*/

static void
IIdemo_query( II_PTR *connHandle, II_PTR *tranHandle, 
	      char *desc, char *queryText )
{
    IIAPI_QUERYPARM	queryParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    printf( "IIdemo_query: %s\n", desc );

    /*
    ** Call IIapi_query to execute statement.
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = *connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = queryText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = *tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );
  
    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    /*
    ** Return transaction handle.
    */
    *tranHandle = queryParm.qy_tranHandle;

    /*
    ** Call IIapi_getQueryInfo() to get results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    /*
    ** Call IIapi_close() to release resources.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    return;
}


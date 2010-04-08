/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apissell.c 
**
** Description:
**	Demonstrates using IIapi_query(), IIapi_getDescriptor(),
**	IIapi_getColumns(), IIapi_cancel() and IIapi_close().
**
**	SELECT data using a select loop.
** 
** Following actions are demonstrated in the main()
**	Issue query
**	Get query result descriptors
**	Get query results
**	Cancel query processing
**	Free query resources.
**
** Command syntax: apissell database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init();
static	void IIdemo_term();
static	void IIdemo_conn( char *, II_PTR * );
static	void IIdemo_disconn( II_PTR * );
static	void IIdemo_rollback( II_PTR  * );

static	char queryText[] = "select table_name from iitables";



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
    II_PTR		stmtHandle = (II_PTR)NULL;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_GETDESCRPARM	getDescrParm;
    IIAPI_GETCOLPARM	getColParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_CANCELPARM	cancelParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    IIAPI_DATAVALUE	DataBuffer[ 1 ];
    int			i;
    char		var1[33];

    if ( argc != 2 )
    {
	printf( "usage: apissell [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(); 
    IIdemo_conn(argv[1],&connHandle);

    /*
    **  Issue query
    */
    printf( "apissell: select table names\n" );

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = queryText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );
  
    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    tranHandle = queryParm.qy_tranHandle;
    stmtHandle = queryParm.qy_stmtHandle;

    /*
    **  Get query result descriptors
    */
    printf( "apissell: get descriptors\n" );

    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    IIapi_getDescriptor( &getDescrParm );
    
    while( getDescrParm.gd_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    /*
    **  Get query results
    */
    printf( "apissell: get results\n" );

    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 1;
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_columnData =  DataBuffer;
    getColParm.gc_columnData[0].dv_value = var1; 
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0;

    for( i = 0; i < 5; i++ )
    {
	IIapi_getColumns( &getColParm );
        
        while( getColParm.gc_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	if ( getColParm.gc_genParm.gp_status >= IIAPI_ST_NO_DATA )
	    break; 

	var1[ DataBuffer[0].dv_length ] = '\0';
	printf( "\t%s = %s  \n", 
	    getDescrParm.gd_descriptor[0].ds_columnName, var1 );

    };

    /*
    **  Cancel query processing.
    */
    printf( "apissell: cancel query\n" );

    cancelParm.cn_genParm.gp_callback = NULL;
    cancelParm.cn_genParm.gp_closure = NULL;
    cancelParm.cn_stmtHandle = stmtHandle;

    IIapi_cancel(&cancelParm );

    while( cancelParm.cn_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm);

    /*
    **  Free query resources.
    */
    printf( "apissell: free query resources\n" );

    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    IIdemo_rollback(&tranHandle); 
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
** Name: IIdemo_rollback
**
** Description:
**	Invokes IIapi_rollback() to rollback current transaction
**	and resets the transaction handle.
**
** Input:
**	tranHandle	Handle of transaction.
**
** Output:
**	tranHandle	Updated handle.
**
** Return value:
**      None.
*/
	
static void
IIdemo_rollback( II_PTR *tranHandle )
{
    IIAPI_ROLLBACKPARM	rollbackParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    printf( "IIdemo_rollback: rolling back transaction\n" );

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = *tranHandle;
    rollbackParm.rb_savePointHandle = NULL; 

    IIapi_rollback( &rollbackParm );

    while( rollbackParm.rb_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    *tranHandle = NULL;
    return;
}


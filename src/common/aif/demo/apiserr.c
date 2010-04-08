/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiserr.c 
**
** Description:
**	Demonstrates using IIapi_getQueryInfo() and IIapi_getErrorInfo().
**	This demo program can be taken as an example of a generic error 
**	handler function for handling OpenAPI errors returned to the 
**	application. Additionally please refer to the OpenAPI documentation,
**	section "Generic Parameters" to understand the meaning of each of 
**	IIAPI_ST_* error codes returned in "gp_status" o/p parameter of 
**	IIAPI_GENPARM structure.
**
** Following actions are demonstrated in the main()
**	Check status of statement with no rows.
**	Check status of insert statement.
**	Check status of invalid statement.
**
** Command syntax: apiserr database_name
**
** History:
**      30-Sep-2009 (rajus01) SD issue 133239; Bug # 122747
**	    It was not clear to a new OpenAPI user that this demo
**	    program demonstrates the OpenAPI generic error handling of all 
**	    OpenAPI functions in an OpenAPI application. Added additional 
**	    comments to the description section above.
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init();
static	void IIdemo_term();
static	void IIdemo_conn( char *, II_PTR * );
static	void IIdemo_disconn( II_PTR * );
static	void IIdemo_query( II_PTR *, II_PTR *, char *, char * );
static	void IIdemo_rollback( II_PTR  * );
static	void IIdemo_checkError( IIAPI_GENPARM * );
static	void IIdemo_checkQInfo( IIAPI_GETQINFOPARM  * );

static  char        createTBLText[] =
     "create table api_demo_err( name char(20) not null, age i4 not null )";
static  char    insertText[] = "insert into api_demo_err values('John' , 30)";
static  char    invalidText[] = "select badcolumn from api_demo_err";

    

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
    II_PTR		stmtHandle;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_SETDESCRPARM	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_GETQINFOPARM	getQInfoParm; 
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    if ( argc != 2 )
    {
	printf( "usage: apiserr [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(); 
    IIdemo_conn(argv[1],&connHandle);   

    /* 
    **  Valid query: no row count 
    */
    IIdemo_query(&connHandle, &tranHandle,"create table",createTBLText);

    /* 
    **  Valid query: row count 
    */
    IIdemo_query(&connHandle, &tranHandle,"insert",insertText);

    /* 
    **  Invalid query 
    */
    IIdemo_query(&connHandle, &tranHandle,"invalid column",invalidText);

    IIdemo_rollback(&tranHandle);
    IIdemo_disconn(&connHandle);
    IIdemo_term();

    return( 0 );
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

    if ( queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS)
	IIdemo_checkError(& queryParm.qy_genParm);

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

    if ( getQInfoParm.gq_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getQInfoParm.gq_genParm );
    else 
	IIdemo_checkQInfo( &getQInfoParm ); 

    /*
    ** Call IIapi_close() to release resources.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    if ( closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError(&closeParm.cl_genParm);

    return;
}


/*
** Name: IIdemo_checkError
**
** Description:
**	Check the status of an API function call 
**	and process error information.
**
** Input:
**	genParm		API generic parameters.
**
** Output:
**	None.
**
** Return value:
**	None.
*/

static void
IIdemo_checkError( IIAPI_GENPARM        *genParm )
{
    IIAPI_GETEINFOPARM	getErrParm; 
    char		type[33];

    /*
    ** Check API call status.
    */
    printf( "\tgp_status = %s\n",
               (genParm->gp_status == IIAPI_ST_SUCCESS) ?  
			"IIAPI_ST_SUCCESS" :
               (genParm->gp_status == IIAPI_ST_MESSAGE) ?  
			"IIAPI_ST_MESSAGE" :
               (genParm->gp_status == IIAPI_ST_WARNING) ?  
			"IIAPI_ST_WARNING" :
               (genParm->gp_status == IIAPI_ST_NO_DATA) ?  
			"IIAPI_ST_NO_DATA" :
               (genParm->gp_status == IIAPI_ST_ERROR)   ?  
			"IIAPI_ST_ERROR"   :
               (genParm->gp_status == IIAPI_ST_FAILURE) ? 
			"IIAPI_ST_FAILURE" :
               (genParm->gp_status == IIAPI_ST_NOT_INITIALIZED) ?
			"IIAPI_ST_NOT_INITIALIZED" :
               (genParm->gp_status == IIAPI_ST_INVALID_HANDLE) ?
			"IIAPI_ST_INVALID_HANDLE"  :
               (genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY) ?
			"IIAPI_ST_OUT_OF_MEMORY"   :
              "(unknown status)" );

    /*
    ** Check for error information.
    */
    if ( ! genParm->gp_errorHandle )  return;
    getErrParm.ge_errorHandle = genParm->gp_errorHandle;

    do
    { 
	/*
	** Invoke API function call.
 	*/
    	IIapi_getErrorInfo( &getErrParm );

 	/*
	** Break out of the loop if no data or failed.
	*/
    	if ( getErrParm.ge_status != IIAPI_ST_SUCCESS )
	    break;

	/*
	** Process result.
	*/
	switch( getErrParm.ge_type )
	{
	    case IIAPI_GE_ERROR	 : 
		strcpy( type, "ERROR" ); 	break;

	    case IIAPI_GE_WARNING :
		strcpy( type, "WARNING" ); 	break;

	    case IIAPI_GE_MESSAGE :
		strcpy(type, "USER MESSAGE");	break;

	    default:
		sprintf( type, "unknown error type: %d", getErrParm.ge_type);
		break;
	}

	printf( "\tError Info: %s '%s' 0x%x: %s\n",
		   type, getErrParm.ge_SQLSTATE, getErrParm.ge_errorCode,
		   getErrParm.ge_message ? getErrParm.ge_message : "NULL" );

    } while( 1 );

    return;
}


/*
** Name: IIdemo_checkQInfo
**
** Description:
**	Processes the information returned by IIapi_getQInfo().
**
** Input:
**	getQInfoParm	Parameter block from IIapi_getQInfo().
**
** Output:
**	None.
**
** Return value:
**	None.
*/

static void
IIdemo_checkQInfo( IIAPI_GETQINFOPARM *getQInfoParm )
{
    /*
    ** Check query result flags.
    */
    if ( getQInfoParm->gq_flags & IIAPI_GQF_FAIL )
        printf( "\tflag = IIAPI_GQF_FAIL\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_ALL_UPDATED )
        printf( "\tflag = IIAPI_GQF_ALL_UPDATED\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_NULLS_REMOVED )
        printf( "\tflag = IIAPI_GQF_NULLS_REMOVED\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_UNKNOWN_REPEAT_QUERY )
        printf( "\tflag = IIAPI_GQF_UNKNOWN_REPEAT_QUERY\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_END_OF_DATA )
        printf( "\tflag = IIAPI_GQF_END_OF_DATA\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_CONTINUE )
        printf( "\tflag = IIAPI_GQF_CONTINUE\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_INVALID_STATEMENT )
        printf( "\tflag = IIAPI_GQF_INVALID_STATEMENT\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_TRANSACTION_INACTIVE )
        printf( "\tflag = IIAPI_GQF_TRANSACTION_INACTIVE\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_OBJECT_KEY )
        printf( "\tflag = IIAPI_GQF_OBJECT_KEY\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_TABLE_KEY )
        printf( "\tflag = IIAPI_GQF_TABLE_KEY\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_NEW_EFFECTIVE_USER )
        printf( "\tflag = IIAPI_GQF_NEW_EFFECTIVE_USER\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_FLUSH_QUERY_ID )
        printf( "\tflag = IIAPI_GQF_FLUSH_QUERY_ID\n" );

    if ( getQInfoParm->gq_flags & IIAPI_GQF_ILLEGAL_XACT_STMT )
        printf( "\tflag = IIAPI_GQF_ILLEGAL_XACT_STMT\n" );

    /*
    ** Check query result values.
    */
    if ( getQInfoParm->gq_mask & IIAPI_GQ_ROW_COUNT )
        printf( "\trow count = %d\n", getQInfoParm->gq_rowCount );

    if ( getQInfoParm->gq_mask & IIAPI_GQ_CURSOR )
        printf( "\treadonly = TRUE\n" );

    if ( getQInfoParm->gq_mask & IIAPI_GQ_PROCEDURE_RET )
        printf( "\tprocedure return = %d\n", getQInfoParm->gq_procedureReturn );

    if ( getQInfoParm->gq_mask & IIAPI_GQ_PROCEDURE_ID )
        printf("\tprocedure handle = 0x%x\n",getQInfoParm->gq_procedureHandle);

    if ( getQInfoParm->gq_mask & IIAPI_GQ_REPEAT_QUERY_ID )
        printf("\trepeat query ID = 0x%x\n",getQInfoParm->gq_repeatQueryHandle);

    if ( getQInfoParm->gq_mask & IIAPI_GQ_TABLE_KEY )
        printf( "\tReceived table key\n" );

    if ( getQInfoParm->gq_mask & IIAPI_GQ_OBJECT_KEY )
        printf( "\treceived object key\n" );

    return;
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


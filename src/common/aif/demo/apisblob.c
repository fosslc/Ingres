/*
** Copyright (c) 2004 Ingres Corporation
*/



/*
**
** Name: apisblob.c 
**
** Description:
**      Demonstrates using IIapi_setEnvParam(), IIapi_query(), 
**	IIapi_getDescriptor(), IIapi_getColumns(), IIapi_getQueryInfo(),
**	and IIapi_close().
**
**      SELECT row with blob data.
**
** Following actions are demonstrated in the main()
**	Set EnvParam for segment length
**      Issue query
**      Get query result descriptors
**      Get query results
**      Close query processing
**      Free query resources.
**
** Command syntax:
**     apisblob database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static  void IIdemo_init( II_PTR * );
static  void IIdemo_term( II_PTR * );
static	void IIdemo_conn( char *, II_PTR *, II_PTR * );
static	void IIdemo_disconn( II_PTR * );
static  void IIdemo_rollback( II_PTR  * );
static  void IIdemo_query( II_PTR *, II_PTR *, char * );
static  void IIdemo_insert( II_PTR *, II_PTR * );
static  void IIdemo_checkError( IIAPI_GENPARM *, char * );

static char createTBLText[] =
	 "create table api_demo ( name char(20) not null, \
				blobdata long varchar, \
				comment char(20))";

static char insertText[] =
 	"insert into api_demo values (  ~V ,  ~V ,  ~V )";

static char selectText[] = "select * from api_demo";


static char blobdata2[4100]; 
static char blobdata3[4100]; 
static char blobdata[1000] =
"test blob data segment 11111111111111111111111111111111111111111111111\
222222222222222222222222222222222222222222222222222222222222222222222\
333333333333333333333333333333333333333333333333333333333333333333333\
444444444444444444444444444444444444444444444444444444444444444444444444\
55555555555555555555555555555555555555555555555555555555555555555555555\
6666666666666666666666666666666666666666666666666666666666666666666666\
7777777777777777777777777777777777777777777777777777777777777777777777\
88888888888888888888888888888888888888888888888888888888888888888888888\
999999999999999999999999999999999999999999999999999999999999999999999\
end of blobdata segment ";

#define DEMO_TABLE_SIZE 5 

static struct
{
    char    *name;
    char    *comment;
} insTBLInfo[DEMO_TABLE_SIZE] =
{
    {"Abrham, Barbara T.",    "comment field" },
    { "Haskins, Jill G.",     "comment field" },
    { "Poon, Jennifer C.",    "comment field" },
    { "Thurman, Roberta F.",  "comment field" },
    { "Wilson, Frank N.",     "comment field" }
};



/*
** Name: main - main loop
**
** Description:
**	This function is the main loop of the sample code.
**
*/

int
main( int argc, char** argv )
{
    II_PTR		envHandle  = (II_PTR)NULL;
    II_PTR		connHandle = (II_PTR)NULL;
    II_PTR		tranHandle = (II_PTR)NULL;
    II_PTR		stmtHandle = (II_PTR)NULL;
    IIAPI_SETENVPRMPARM setEnvPrmParm;
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_GETCOLPARM    getColParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_WAITPARM      waitParm = { -1 };
    IIAPI_DATAVALUE    	DataBuffer[ 3 ];
    II_LONG		paramvalue; 
    char		var1[21],var2[21];
    char		*ptr;
    short		len;


    if ( argc != 2 )
    {
	printf( "usage: apisblob [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }


    IIdemo_init( &envHandle );
    IIdemo_conn( argv[1], &envHandle, &connHandle );

    printf( " apisblob: create table \n" );
    IIdemo_query( &connHandle, &tranHandle, createTBLText );

    printf( " apisblob: insert 1 row \n" );
    IIdemo_insert( &connHandle, &tranHandle );

    /*
    **  set EnvParam for segment length of 4K 
    */
    printf( "apisblob: setEnvParam seg len to 4K \n" );
    setEnvPrmParm.se_envHandle = envHandle;
    setEnvPrmParm.se_paramID = IIAPI_EP_MAX_SEGMENT_LEN; 
    setEnvPrmParm.se_paramValue = (II_LONG *)( &paramvalue );
    paramvalue = 4096; 

    IIapi_setEnvParam( &setEnvPrmParm );
    if ( setEnvPrmParm.se_status != IIAPI_ST_SUCCESS )
	printf( " apisblob: setEnvParm error \n" );

 
    printf( " apisblob: select 1 row \n" );
     
    /*
    ** Provide parameters for IIapi_query().
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = ( char * )selectText;
    queryParm.qy_parameters = FALSE; 
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = ( II_PTR )NULL;

    IIapi_query( &queryParm );

    tranHandle = queryParm.qy_tranHandle;
    stmtHandle = queryParm.qy_stmtHandle;
    
    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &queryParm.qy_genParm, "select Text " );

    /*
    **  Get query result descriptors
    */
    printf( "apisblob: get descriptors\n" );
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL; 

    IIapi_getDescriptor( &getDescrParm );
    
    while( getDescrParm.gd_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( getDescrParm.gd_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getDescrParm.gd_genParm, "get Descr " );

    /*
    **  Get query results
    */
    printf( "apisblob: get results\n" );
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 1; 
    getColParm.gc_columnData = &DataBuffer[0];
    getColParm.gc_columnData[0].dv_value = &var1; 
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0;

    IIapi_getColumns( &getColParm );
        
    while( getColParm.gc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    if ( getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getColParm.gc_genParm, "getCol first " );

    var1[ getColParm.gc_columnData[0].dv_length ] = '\0';
    printf( " apisblob: first column username= %s\n", var1 );

    /*
    ** Call IIapi_getColumns() in loop.
    ** until all segments for the blob column are retrived. 
    */
    do
    {
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 1; 
    getColParm.gc_columnData =  &DataBuffer[1];
    getColParm.gc_columnData[0].dv_value = &blobdata2; 
    getColParm.gc_stmtHandle = stmtHandle;

    IIapi_getColumns( &getColParm );
    while( getColParm.gc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getColParm.gc_genParm,"getCol loop " );

    memcpy( (char *)&len, blobdata2, 2 );
    ptr = (char *)blobdata2 + 2;
    memcpy( blobdata3, ptr, len );
    blobdata3[len] = '\0';

    printf( " apisblob: blob segment length: %d (%s)\n", len, 
	getColParm.gc_moreSegments ? "more segments" : "end of segments");

    }while ( getColParm.gc_moreSegments );

    /*
    ** Call IIapi_getColumns() to retrieve last column
    */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 1; 
    getColParm.gc_columnData =  &DataBuffer[2];
    getColParm.gc_columnData[0].dv_value = &var2; 
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0; 

    IIapi_getColumns( &getColParm );
        
    while( getColParm.gc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getColParm.gc_genParm,"getCol last " );

    var2[ getColParm.gc_columnData[0].dv_length ] = '\0';
    printf( " apisblob: last column comment=  %s\n", var2 );

    /*
    ** Call IIapi_getQueryInfo().
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( getQInfoParm.gq_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getQInfoParm.gq_genParm, "get Info " );
    /*
    ** close the query 
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &closeParm.cl_genParm, "close " );

    IIdemo_rollback( &tranHandle ); 
    IIdemo_disconn( &connHandle );
    IIdemo_term( &envHandle );

    return( 0 );
}

/*
** Name: IIdemo_init
**
** Description:
**      Initialize API access.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Return value:
**      None.
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
**      Terminate API access.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Return value:
**      None.
*/


static void
IIdemo_term( II_PTR *envHandle )
{

    IIAPI_RELENVPARM   relEnvParm;
    IIAPI_TERMPARM     termParm;

    printf( "IIdemo_term: releasing envHandle\n" );
    relEnvParm.re_envHandle = *envHandle;
    IIapi_releaseEnv( &relEnvParm );

    printf( "IIdemo_term: shutting down API\n" );
    IIapi_terminate( &termParm );

    return;

}


/*
** Name: IIdemo_conn
**
** Description:
**      Open connection with target Database.
**
** Input:
**      dbname          Database name.
**
** Output:
**      connHandle      Connection handle.
**
** Return value:
**      None.
*/
    
static void
IIdemo_conn( char *dbname, II_PTR *envHandle, II_PTR *connHandle )
{
    IIAPI_CONNPARM    	connParm;
    IIAPI_WAITPARM      waitParm = { -1 };
    
    printf( "IIdemo_conn: establishing connection\n" );
    
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  dbname;
    connParm.co_connHandle = *envHandle; 
    connParm.co_type = IIAPI_CT_SQL;
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
**      Release DBMS connection.
**
** Input:
**      connHandle      Connection handle.
**
** Output:
**      connHandle      Connection handle.
**
** Return value:
**      None.
*/

static void
IIdemo_disconn( II_PTR *connHandle )
{
    IIAPI_DISCONNPARM disconnParm;
    IIAPI_WAITPARM    waitParm = { -1 };
    
    printf( "IIdemo_disconn: releasing connection\n");
    
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
**      Invokes IIapi_rollback() to rollback current transaction
**      and resets the transaction handle.
**
** Input:
**      tranHandle      Handle of transaction.
**
** Output:
**      tranHandle      Updated handle.
**
** Return value:
**      None.
*/


static void
IIdemo_rollback( II_PTR *tranHandle )
{
    IIAPI_ROLLBACKPARM  rollbackParm;
    IIAPI_WAITPARM      waitParm = { -1 };

    printf( "IIdemo_rollback: rolling back transaction\n" );

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = *tranHandle;
    rollbackParm.rb_savePointHandle = NULL; 

    IIapi_rollback( &rollbackParm );

    while(rollbackParm.rb_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );
    return;
}


/*
** Name: IIdemo_insert
**
** Description:
**      Insert rows into table.
**
** Input:
**      connHandle      Connection handle.
**      tranHandle      Transaction handle.
**
** Output:
**      connHandle      Connection handle.
**      tranHandle      Transaction handle.
**
** Return value:
**      None.
*/

static 	void
IIdemo_insert ( II_PTR *connHandle, II_PTR *tranHandle )
{
    IIAPI_QUERYPARM		queryParm;
    IIAPI_SETDESCRPARM		setDescrParm;
    IIAPI_PUTPARMPARM		putParmParm;
    IIAPI_GETQINFOPARM		getQInfoParm;
    IIAPI_CLOSEPARM             closeParm;
    IIAPI_WAITPARM              waitParm = { -1 };
    IIAPI_DESCRIPTOR    	DescrBuffer[ 3 ];
    IIAPI_DATAVALUE    		DataBuffer[ 3 ];
    short			len, total;


    /*
    ** Insert row.
    */
    printf( "IIdemo_insert: execute insert\n" );
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = *connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = ( char * )insertText;
    queryParm.qy_parameters = TRUE;
    queryParm.qy_tranHandle = *tranHandle;
    queryParm.qy_stmtHandle = ( II_PTR )NULL;

    IIapi_query( &queryParm );

    *tranHandle = queryParm.qy_tranHandle;
    
    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &queryParm.qy_genParm, "insert Text " );

    /*
    **  Describe query parameters.
    */
    setDescrParm.sd_genParm.gp_callback = NULL;
    setDescrParm.sd_genParm.gp_closure = NULL;
    setDescrParm.sd_stmtHandle = queryParm.qy_stmtHandle;
    setDescrParm.sd_descriptorCount = 3;
    setDescrParm.sd_descriptor = ( IIAPI_DESCRIPTOR * )( &DescrBuffer );
 
    setDescrParm.sd_descriptor[0].ds_dataType = IIAPI_CHA_TYPE;
    setDescrParm.sd_descriptor[0].ds_nullable = FALSE;
    setDescrParm.sd_descriptor[0].ds_length = 20;
    setDescrParm.sd_descriptor[0].ds_precision = 0;
    setDescrParm.sd_descriptor[0].ds_scale = 0;
    setDescrParm.sd_descriptor[0].ds_columnType = IIAPI_COL_QPARM;
    setDescrParm.sd_descriptor[0].ds_columnName = NULL;

    setDescrParm.sd_descriptor[1].ds_dataType = IIAPI_LVCH_TYPE;
    setDescrParm.sd_descriptor[1].ds_nullable = TRUE; 
    setDescrParm.sd_descriptor[1].ds_length = 3000;
    setDescrParm.sd_descriptor[1].ds_precision = 0;
    setDescrParm.sd_descriptor[1].ds_scale = 0;
    setDescrParm.sd_descriptor[1].ds_columnType = IIAPI_COL_QPARM;
    setDescrParm.sd_descriptor[1].ds_columnName = NULL;

    setDescrParm.sd_descriptor[2].ds_dataType = IIAPI_CHA_TYPE;
    setDescrParm.sd_descriptor[2].ds_nullable = FALSE;
    setDescrParm.sd_descriptor[2].ds_length = 20;
    setDescrParm.sd_descriptor[2].ds_precision = 0;
    setDescrParm.sd_descriptor[2].ds_scale = 0;
    setDescrParm.sd_descriptor[2].ds_columnType = IIAPI_COL_QPARM;
    setDescrParm.sd_descriptor[2].ds_columnName = NULL;

    IIapi_setDescriptor( &setDescrParm );
	
    while( setDescrParm.sd_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( setDescrParm.sd_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &setDescrParm.sd_genParm, "set Desc" );

    /*
    **  Send query parameters.
    */
    putParmParm.pp_genParm.gp_callback = NULL;
    putParmParm.pp_genParm.gp_closure = NULL;
    putParmParm.pp_stmtHandle = queryParm.qy_stmtHandle;
    putParmParm.pp_parmCount = 1;

    putParmParm.pp_parmData = &DataBuffer[0];
    putParmParm.pp_parmData[0].dv_null = FALSE;
    putParmParm.pp_parmData[0].dv_length = strlen( insTBLInfo[0].name ); 
    putParmParm.pp_parmData[0].dv_value = insTBLInfo[0].name;

    putParmParm.pp_moreSegments = 0;

    IIapi_putParms( &putParmParm );

    while( putParmParm.pp_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( putParmParm.pp_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &putParmParm.pp_genParm, "putParm 1" );

    len = strlen( blobdata );
    memcpy( blobdata2, (char *)&len, 2 );
    memcpy( blobdata2 + 2, blobdata, len );

    for( total = 0; total < 5000; )
    {
	putParmParm.pp_genParm.gp_callback = NULL;
	putParmParm.pp_genParm.gp_closure = NULL;
	putParmParm.pp_stmtHandle = queryParm.qy_stmtHandle;
	putParmParm.pp_parmCount = 1; 

	putParmParm.pp_parmData = &DataBuffer[1];
	putParmParm.pp_parmData[0].dv_null = FALSE; 
	putParmParm.pp_parmData[0].dv_length = len + 2; 
	putParmParm.pp_parmData[0].dv_value = blobdata2;
	total += len;
	putParmParm.pp_moreSegments = total < 5000 ? TRUE : FALSE; 
	printf( " apisblob: blob segment length: %d (%s)\n", len, 
	    putParmParm.pp_moreSegments ? "more segments" : "end of segments");

	IIapi_putParms( &putParmParm );

	while( putParmParm.pp_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	if ( putParmParm.pp_genParm.gp_status != IIAPI_ST_SUCCESS )
	    IIdemo_checkError( &putParmParm.pp_genParm, "put blob" );
    }

    putParmParm.pp_genParm.gp_callback = NULL;
    putParmParm.pp_genParm.gp_closure = NULL;
    putParmParm.pp_stmtHandle = queryParm.qy_stmtHandle;
    putParmParm.pp_parmCount = 1;

    putParmParm.pp_parmData = &DataBuffer[2];
    putParmParm.pp_parmData[0].dv_null = FALSE;
    putParmParm.pp_parmData[0].dv_length = strlen(insTBLInfo[0].comment); 
    putParmParm.pp_parmData[0].dv_value = insTBLInfo[0].comment;
    putParmParm.pp_moreSegments = 0;

    IIapi_putParms( &putParmParm );

    while( putParmParm.pp_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( putParmParm.pp_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &putParmParm.pp_genParm, "put last" );

    /*
    **  Get insert results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( getQInfoParm.gq_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &getQInfoParm.gq_genParm, "get Info " );
		
   /*
   **  Free statement resources.
   */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS )
	IIdemo_checkError( &closeParm.cl_genParm, "close " );
    
    return;
}


/*
** Name: IIdemo_query
**
** Description:
**      Execute SQL statement taking no parameters and returning no rows.
**
** Input:
**      connHandle      Connection handle.
**      tranHandle      Transaction handle.
**      queryText       SQL query text.
**
** Output:
**      connHandle      Connection handle.
**      tranHandle      Transaction handle.
**
** Return value:
**      None.
*/

static void
IIdemo_query( II_PTR *connHandle, II_PTR *tranHandle,
	      char *queryText )
{
    IIAPI_QUERYPARM	queryParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM      waitParm = { -1 };

    printf( "IIdemo_query: Executing Query\n");

    /*
    ** call IIapi_query to execute statement.
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = *connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (char * )queryText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = *tranHandle;
    queryParm.qy_stmtHandle = ( II_PTR )NULL;

    IIapi_query( &queryParm );
  
    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    *tranHandle = queryParm.qy_tranHandle;

    /*
    ** call IIapi_getQueryInfo() to get results.
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

/*
** Name: IIdemo_checkError
**
** Description:
**      Check the status of an API function call
**      and process error information.
**
** Input:
**      genParm         API generic parameters.
**	text	 	calling rtn description. 	  		
**
** Output:
**      None.
**
** Return value:
**      None.
*/

static void
IIdemo_checkError( IIAPI_GENPARM *genParm, char *text )
{
    IIAPI_GETEINFOPARM  getErrParm; 
    char                type[33];

    printf("IIdemo_checkError: calling rtn  = %s", text);


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
	    case IIAPI_GE_ERROR		: 
		strcpy( type, "ERRROR" ); 	break;

	    case IIAPI_GE_WARNING	:
		strcpy( type, "WARNING" ); 	break;

	    case IIAPI_GE_MESSAGE	:
		strcpy( type, "USER MESSAGE" );	break;

	    default:
	    sprintf( type, "unknown error type ( %d )", getErrParm.ge_type);
	}

	printf( "\t Error Info: %s '%s' 0x%x: %s\n",
		   type, getErrParm.ge_SQLSTATE, getErrParm.ge_errorCode,
		   getErrParm.ge_message ? getErrParm.ge_message : "NULL" );

    } while( 1 );
    
    return;
}

/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiscupd.c
**
** Description:
**	Demonstrates using IIapi_query(), IIapi_setDescriptor() and
**	IIapi_putParams() to do a cursor positioned update.
** 
** Following actions are demonstrated in the main()
**	Open cursor
**	Position cursor on row
**
**	Issue cursor update statement
**	Describe parameters
**	Send parameter values
**	Get update statement results
**	Free update statement resources
**
**	Get cursor results
**	Free cursor resources
**
** Command syntax: apiscupd database_name
*/

# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>

static  void IIdemo_init();
static  void IIdemo_term();
static  void IIdemo_conn( char *, II_PTR * );
static  void IIdemo_disconn( II_PTR * );
static  void IIdemo_query( II_PTR *, II_PTR *, char *, char * );
static	void IIdemo_rollback( II_PTR  * );
static  void IIdemo_insert( II_PTR *, II_PTR *);


static	char	createTBLText[] =
	"create table api_demo_cupd( name char(20) not null, age i4 not null )";
static	char	prepText[] =
	"prepare s0 from insert into api_demo_cupd values( ?, ? )";
static	char	execText[] = "execute s0";
static	char	openText[] = "select * from api_demo_cupd for update";
static	char	updateText[] = "update api_demo_cupd set age = -1";


# define        DEMO_TABLE_SIZE                 5

static struct
{
    char        *name;
    int         age;
} insTBLInfo[DEMO_TABLE_SIZE] =
{
    { "Abrham, Barbara T.",  35 },
    { "Haskins, Jill G.",    56 },
    { "Poon, Jennifer C.",   50 },
    { "Thurman, Roberta F.", 32 },
    { "Wilson, Frank N.",    24 }
};



/*
** Name: main - main loop
**
** Description:
**	This function is the main loop of the sample code.
*/

int
main( int argc, char** argv ) 
{
    II_PTR          	connHandle = (II_PTR)NULL;
    II_PTR        	tranHandle = (II_PTR)NULL;
    II_PTR        	stmtHandle = (II_PTR)NULL;
    II_PTR        	cursorID;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_SETDESCRPARM 	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_GETDESCRPARM	getDescrParm;
    IIAPI_GETCOLPARM	getColParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_WAITPARM      waitParm = { -1 };
    IIAPI_DESCRIPTOR	DescrBuffer[ 1 ];
    IIAPI_DATAVALUE	DataBuffer[ 2 ];
    IIAPI_DATAVALUE	CursorBuffer[ 1 ];
    char                var1[33];  
    int			var2;

    if ( argc != 2 )
    {
	printf( "usage: apiscupd [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init();
    IIdemo_conn(argv[1],&connHandle);
    IIdemo_query(&connHandle, &tranHandle,"create table",createTBLText);
    IIdemo_query(&connHandle, &tranHandle,"prepare insert",prepText);
    IIdemo_insert(&connHandle,&tranHandle);

    /*
    **  Open cursor.
    */
    printf( "apiscupd: open cursor\n");

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_OPEN;
    queryParm.qy_queryText = openText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );

    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    tranHandle = queryParm.qy_tranHandle;
    stmtHandle = queryParm.qy_stmtHandle;

    /*
    **  Get cursor row descriptor.
    */
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    IIapi_getDescriptor( &getDescrParm );
    
    while( getDescrParm.gd_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    /*
    **  Position cursor on row.
    */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = getDescrParm.gd_descriptorCount;
    getColParm.gc_columnData = DataBuffer;

    getColParm.gc_columnData[0].dv_value = var1;
    getColParm.gc_columnData[1].dv_value = &var2;
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0;

    do
    {
	printf( "apiscupd: next row\n" );
	IIapi_getColumns( &getColParm );
        
        while( getColParm.gc_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	if ( getColParm.gc_genParm.gp_status >= IIAPI_ST_NO_DATA )
	    break;

	/*
	**  Update using cursor.
	*/
	printf( "apiscupd: update row\n" );

	queryParm.qy_genParm.gp_callback = NULL;
	queryParm.qy_genParm.gp_closure = NULL;
	queryParm.qy_connHandle = connHandle;
	queryParm.qy_queryType = IIAPI_QT_CURSOR_UPDATE;
	queryParm.qy_queryText = updateText;
	queryParm.qy_parameters = TRUE;
	queryParm.qy_tranHandle = tranHandle;
	queryParm.qy_stmtHandle = NULL;

	IIapi_query( &queryParm );

	while( queryParm.qy_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Describe query parameters - cursor handle.
	*/
	setDescrParm.sd_genParm.gp_callback = NULL;
	setDescrParm.sd_genParm.gp_closure = NULL;
	setDescrParm.sd_stmtHandle = queryParm.qy_stmtHandle;
	setDescrParm.sd_descriptorCount = 1;
	setDescrParm.sd_descriptor = DescrBuffer;

	setDescrParm.sd_descriptor[0].ds_dataType = IIAPI_HNDL_TYPE;
	setDescrParm.sd_descriptor[0].ds_nullable = FALSE;
	setDescrParm.sd_descriptor[0].ds_length = sizeof( II_PTR );
	setDescrParm.sd_descriptor[0].ds_precision = 0;
	setDescrParm.sd_descriptor[0].ds_scale = 0;
	setDescrParm.sd_descriptor[0].ds_columnType = IIAPI_COL_SVCPARM;
	setDescrParm.sd_descriptor[0].ds_columnName = NULL;

	IIapi_setDescriptor( &setDescrParm );

	while( setDescrParm.sd_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Send query parameters - cursor handle.
	*/
	putParmParm.pp_genParm.gp_callback = NULL;
	putParmParm.pp_genParm.gp_closure = NULL;
	putParmParm.pp_stmtHandle = queryParm.qy_stmtHandle;
	putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;
	putParmParm.pp_parmData = CursorBuffer; 
	putParmParm.pp_moreSegments = 0;

	putParmParm.pp_parmData[0].dv_null = FALSE;
	putParmParm.pp_parmData[0].dv_length = sizeof( II_PTR );
	putParmParm.pp_parmData[0].dv_value = (II_PTR)&cursorID;
	cursorID = stmtHandle;

	IIapi_putParms( &putParmParm );

	while( putParmParm.pp_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Get update statement results.
	*/
	getQInfoParm.gq_genParm.gp_callback = NULL;
	getQInfoParm.gq_genParm.gp_closure = NULL;
	getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

	IIapi_getQueryInfo( &getQInfoParm );

	while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Free update statement resources.
	*/
	closeParm.cl_genParm.gp_callback = NULL;
	closeParm.cl_genParm.gp_closure = NULL;
	closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

	IIapi_close( &closeParm );

	while( closeParm.cl_genParm.gp_completed == FALSE )
	   IIapi_wait( &waitParm );

    } while( 1 );

    /*
    **  Get cursor fetch results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
         IIapi_wait( &waitParm );

    /*
    **  Free cursor resources.
    */
    printf( "apiscupd: close cursor\n" );

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
** Name: IIdemo_insert
**
** Description:
**	Insert rows into table.
**
** Input:
**	connHandle	Connection handle.
**      tranHandle	Transaction handle.
**
** Output:
**	connHandle	Connection handle.
**	tranHandle	Transaction handle.
**
** Return value:
**      None.
*/

static 	void
IIdemo_insert( II_PTR *connHandle, II_PTR *tranHandle )
{
    IIAPI_QUERYPARM	queryParm;
    IIAPI_SETDESCRPARM	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    IIAPI_DESCRIPTOR	DescrBuffer[ 2 ];
    IIAPI_DATAVALUE	DataBuffer[ 2 ];
    int			row;
    int			i;

    for( row = 0; row < DEMO_TABLE_SIZE; row++ )
    {
	/*
	**  Insert row.
	*/
	printf( "IIdemo_insert: execute insert\n" ); 

	queryParm.qy_connHandle = *connHandle;
	queryParm.qy_queryType = IIAPI_QT_EXEC;
	queryParm.qy_queryText = execText;
	queryParm.qy_parameters = TRUE;
	queryParm.qy_tranHandle = *tranHandle;
	queryParm.qy_stmtHandle = NULL;

	IIapi_query( &queryParm );

        while( queryParm.qy_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	*tranHandle =  queryParm.qy_tranHandle;

	/*
	**  Describe query parameters.
	*/
        setDescrParm.sd_genParm.gp_callback = NULL;
      	setDescrParm.sd_genParm.gp_closure = NULL;
  	setDescrParm.sd_stmtHandle = queryParm.qy_stmtHandle;
	setDescrParm.sd_descriptorCount = 2;

        setDescrParm.sd_descriptor = DescrBuffer;
	setDescrParm.sd_descriptor[0].ds_dataType = IIAPI_CHA_TYPE;
	setDescrParm.sd_descriptor[0].ds_nullable = FALSE;
	setDescrParm.sd_descriptor[0].ds_length =
					strlen( insTBLInfo[row].name );
	setDescrParm.sd_descriptor[0].ds_precision = 0;
	setDescrParm.sd_descriptor[0].ds_scale = 0;
	setDescrParm.sd_descriptor[0].ds_columnType = IIAPI_COL_QPARM;
	setDescrParm.sd_descriptor[0].ds_columnName = NULL;

        setDescrParm.sd_descriptor[1].ds_dataType = IIAPI_INT_TYPE;
	setDescrParm.sd_descriptor[1].ds_nullable = FALSE;
        setDescrParm.sd_descriptor[1].ds_length = sizeof( II_INT4 );
        setDescrParm.sd_descriptor[1].ds_precision = 0;
        setDescrParm.sd_descriptor[1].ds_scale = 0;
        setDescrParm.sd_descriptor[1].ds_columnType = IIAPI_COL_QPARM;
        setDescrParm.sd_descriptor[1].ds_columnName = NULL;

 	IIapi_setDescriptor( &setDescrParm );
	
        while( setDescrParm.sd_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Send query parameters.
	*/
	putParmParm.pp_genParm.gp_callback = NULL;
	putParmParm.pp_genParm.gp_closure = NULL;
	putParmParm.pp_stmtHandle = queryParm.qy_stmtHandle;
	putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;
	putParmParm.pp_moreSegments = 0;

        putParmParm.pp_parmData = DataBuffer;
	putParmParm.pp_parmData[0].dv_null = FALSE;
	putParmParm.pp_parmData[0].dv_length = 
					strlen( insTBLInfo[row].name );
	putParmParm.pp_parmData[0].dv_value = insTBLInfo[row].name;

        putParmParm.pp_parmData[1].dv_null = FALSE;
        putParmParm.pp_parmData[1].dv_length = sizeof( II_INT4 );
        putParmParm.pp_parmData[1].dv_value = &insTBLInfo[row].age;

	IIapi_putParms( &putParmParm );

        while( putParmParm.pp_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Get insert results.
	*/
	getQInfoParm.gq_genParm.gp_callback = NULL;
	getQInfoParm.gq_genParm.gp_closure = NULL;
	getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

	IIapi_getQueryInfo( &getQInfoParm );

	while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	/*
	**  Free statement resources.
	*/
	closeParm.cl_genParm.gp_callback = NULL;
	closeParm.cl_genParm.gp_closure = NULL;
	closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

	IIapi_close( &closeParm );

	while( closeParm.cl_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );
    }
    
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


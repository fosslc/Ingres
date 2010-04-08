/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisprbr.c
**
** Description:
**	Demonstrates using IIapi_query(), IIapi_setDescriptor(),
**	IIapi_putParams(), IIapi_getDescriptor() and IIapi_getColumns()
**	to execute a database procedure with BYREF parameters and
**	retrieve the parameter result and procedure return values.
** 
** Following actions are demonstrated in the main()
**	Create procedure
**	Execute procedure
**	Retrieve BYREF parameter results
**	Get procedure results
**
** Command syntax: apisprbr database_name
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


static	char	procName[] = "api_demo_prbr"; 
static	char	procText[] =
"create procedure api_demo_prbr( \
    table_name varchar(32) with null, \
    column_count integer with null ) \
as declare \
    table_count integer not null; \
begin \
    if ( :table_name is null ) then \
	select table_name into :table_name from iitables; \
    endif; \
    select count(*) into :column_count from iicolumns \
	where table_name = :table_name; \
    select count(*) into :table_count from iitables; \
    return :table_count; \
end";

typedef struct 
{
    short	length;
    char	value[ 33 ];
} varchar;



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
    IIAPI_QUERYPARM	queryParm;
    IIAPI_SETDESCRPARM  setDescrParm;
    IIAPI_PUTPARMPARM   putParmParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_GETCOLPARM    getColParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_COMMITPARM    commitParm;
    IIAPI_WAITPARM      waitParm = { -1 };
    IIAPI_DESCRIPTOR	DescrBuffer[ 3 ];
    IIAPI_DATAVALUE	DataBuffer[ 3 ];
    varchar		name;
    int			count;
    char                parmName[] = "table_name";
    char                parmCount[] = "column_count";

    if ( argc != 2 )
    {
	printf( "usage: apisprbr [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init();
    IIdemo_conn(argv[1],&connHandle);
    IIdemo_query(&connHandle, &tranHandle,"create procedure",procText);
 
    /*
    **  Execute procedure
    */
    printf( "apisprbr: execute procedure\n" );

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_EXEC_PROCEDURE;
    queryParm.qy_queryText = NULL;
    queryParm.qy_parameters = TRUE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );

    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    tranHandle = queryParm.qy_tranHandle;
    stmtHandle = queryParm.qy_stmtHandle;

    /*  
    **  Describe procedure parameters
    */
    setDescrParm.sd_genParm.gp_callback = NULL;
    setDescrParm.sd_genParm.gp_closure = NULL;
    setDescrParm.sd_stmtHandle = stmtHandle;
    setDescrParm.sd_descriptorCount = 3;
    setDescrParm.sd_descriptor = DescrBuffer;
    setDescrParm.sd_descriptor[0].ds_dataType = IIAPI_CHA_TYPE;
    setDescrParm.sd_descriptor[0].ds_length = strlen(procName);

    setDescrParm.sd_descriptor[0].ds_nullable = FALSE;
    setDescrParm.sd_descriptor[0].ds_precision = 0;
    setDescrParm.sd_descriptor[0].ds_scale = 0;
    setDescrParm.sd_descriptor[0].ds_columnType = IIAPI_COL_SVCPARM;
    setDescrParm.sd_descriptor[0].ds_columnName = NULL;

    setDescrParm.sd_descriptor[1].ds_dataType = IIAPI_CHA_TYPE;
    setDescrParm.sd_descriptor[1].ds_nullable = TRUE;
    setDescrParm.sd_descriptor[1].ds_length = 32;
    setDescrParm.sd_descriptor[1].ds_precision = 0;
    setDescrParm.sd_descriptor[1].ds_scale = 0;
    setDescrParm.sd_descriptor[1].ds_columnType = IIAPI_COL_PROCBYREFPARM;
    setDescrParm.sd_descriptor[1].ds_columnName = parmName; 

    setDescrParm.sd_descriptor[2].ds_dataType = IIAPI_INT_TYPE;
    setDescrParm.sd_descriptor[2].ds_nullable = TRUE;
    setDescrParm.sd_descriptor[2].ds_length = sizeof( II_INT4 );
    setDescrParm.sd_descriptor[2].ds_precision = 0;
    setDescrParm.sd_descriptor[2].ds_scale = 0;
    setDescrParm.sd_descriptor[2].ds_columnType = IIAPI_COL_PROCBYREFPARM;
    setDescrParm.sd_descriptor[2].ds_columnName = parmCount; 

    IIapi_setDescriptor( &setDescrParm );

    while( setDescrParm.sd_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

    /*
    **  Send procedure parameters
    */
    putParmParm.pp_genParm.gp_callback = NULL;
    putParmParm.pp_genParm.gp_closure = NULL;
    putParmParm.pp_stmtHandle = stmtHandle;
    putParmParm.pp_parmCount = setDescrParm.sd_descriptorCount;
    putParmParm.pp_parmData = DataBuffer;
    putParmParm.pp_moreSegments = 0;

    putParmParm.pp_parmData[1].dv_null = FALSE;
    putParmParm.pp_parmData[0].dv_length = strlen( procName );
    putParmParm.pp_parmData[0].dv_value = procName;

    putParmParm.pp_parmData[1].dv_null = TRUE;
    putParmParm.pp_parmData[1].dv_length = 0;
    putParmParm.pp_parmData[1].dv_value = NULL;

    putParmParm.pp_parmData[2].dv_null = TRUE;
    putParmParm.pp_parmData[2].dv_length = 0;
    putParmParm.pp_parmData[2].dv_value = NULL;

    IIapi_putParms( &putParmParm );
   
    while( putParmParm.pp_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

    /*
    **  Get BYREF parameter descriptors
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
    **  Get BYREF parameter results.
    */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 2;
    getColParm.gc_columnData = DataBuffer;
    getColParm.gc_columnData[0].dv_value = &name;
    getColParm.gc_columnData[1].dv_value = &count;
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0;

    IIapi_getColumns( &getColParm );
        
    while( getColParm.gc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    if ( getColParm.gc_genParm.gp_status < IIAPI_ST_NO_DATA )
    {
	name.value[ name.length ] = '\0';
	printf( "\ttable %s has %d columns\n", name.value, count );
    }

    /*
    **  Get procedure results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    if ( getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_RET )
	printf( "\tthere are %d tables\n", getQInfoParm.gq_procedureReturn );

    /*
    **  Free resources.
    */
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


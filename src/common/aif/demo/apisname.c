/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisname.c  
**
** Description:
**	Demonstrates using IIapi_connect(), IIapi_autocommit() and
**	IIapi_query() to access the Name Server database.
**
**  Following actions are demonstrate in the main()
**	Connect to local Name Server
**	Enable Autocommit
**	Query connection info  
**	Disable autocommit
**
** Command syntax: apisname 
*/

# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static  void IIdemo_init( II_PTR * );
static	void IIdemo_term( II_PTR * );
static	void IIdemo_disconn( II_PTR   * );

static  char showText[] = "show global connection * * * * ";



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
    II_PTR		envHandle = (II_PTR)NULL;
    IIAPI_CONNPARM	connParm;
    IIAPI_AUTOPARM	autoparm;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_GETDESCRPARM	getDescrParm;
    IIAPI_GETCOLPARM	getColParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    IIAPI_DESCRIPTOR	DescrBuffer[ 5 ];
    IIAPI_DATAVALUE	DataBuffer[ 5 ];
    char		var[5][129];
    short		i, len;

    IIdemo_init(&envHandle);

    /*
    **  Connect to local Name Server
    */
    printf( "apisname: establishing connection to Name Server\n" );

    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  NULL;   
    connParm.co_type   =  IIAPI_CT_NS;   
    connParm.co_connHandle = envHandle; 
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
    **  Enable autocommit
    */
    printf( "apisauto: enable autocommit\n" );

    autoparm.ac_genParm.gp_callback = NULL;
    autoparm.ac_genParm.gp_closure  = NULL;
    autoparm.ac_connHandle = connHandle;
    autoparm.ac_tranHandle = NULL;

    IIapi_autocommit( &autoparm );

    while( autoparm.ac_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    tranHandle = autoparm.ac_tranHandle;

    /*
    **  Execute 'show' statement.
    */
    printf( "apisname: retrieving VNODE connection info\n");

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = showText;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );
  
    while( queryParm.qy_genParm.gp_completed == FALSE )
      IIapi_wait( &waitParm );

    stmtHandle = queryParm.qy_stmtHandle;

    /*
    **  Get result row descriptors.
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
    **  Retrieve result rows.
    */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = getDescrParm.gd_descriptorCount;
    getColParm.gc_columnData = DataBuffer;
    getColParm.gc_stmtHandle = stmtHandle;
    getColParm.gc_moreSegments = 0;

    for( i = 0; i < getDescrParm.gd_descriptorCount; i++ )
	getColParm.gc_columnData[i].dv_value = var[i]; 

    do 
    {
	IIapi_getColumns( &getColParm );
        
        while( getColParm.gc_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );

	if ( getColParm.gc_genParm.gp_status >= IIAPI_ST_NO_DATA )
	    break; 

        for( i = 0; i < getDescrParm.gd_descriptorCount; i++ ) 
        {
	    if ( getDescrParm.gd_descriptor[i].ds_dataType == IIAPI_VCH_TYPE )
	    {
		memcpy( (char *)&len, var[i], 2 );
		var[i][ len + 2 ] = '\0';
		strcpy( var[i], &var[i][2] );
	    }
	    else
	    {
		var[i][ getColParm.gc_columnData[ i ].dv_length ] = '\0';
	    }

        }

	printf( "\tG/P = %s vnode = %s host = %s prot = %s addr = %s\n", 
		var[0],var[1],var[2],var[3],var[4]);

    } while (1); 

    /*
    **  Get query results.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
          
    /*
    **  Close query.
    */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    /*
    **  Disable autocommit.
    */
    printf( "apisname: disable autocommit\n" );

    autoparm.ac_connHandle = NULL;
    autoparm.ac_tranHandle = tranHandle;

    IIapi_autocommit( &autoparm );
    
    while( autoparm.ac_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    IIdemo_disconn(&connHandle);
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


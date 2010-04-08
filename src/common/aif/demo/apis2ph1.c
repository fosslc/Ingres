/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apis2ph1.c 
**
** Description:
**	Demonstrates using IIapi_registerXID(), IIapi_prepareCommit().
**
**	First part of the two phase commit demo.  Begin a distributed 
**	transaction and exit without disconnecting.  
**
**	Run apis2ph2 to finish processing of distributed transaction.
**
** Following actions are demonstrated in the main()
**     Register distributed transaction ID.
**     Prepare distributed transaction to be committed.
**
** Command syntax: apis2ph1 database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init();
static	void IIdemo_conn( char *dbname, II_PTR *connHandle );
static	void IIdemo_query( II_PTR *, II_PTR *, char *, char * );

static	char queryText[] =
	"create table api_demo_2pc( name char(20) not null, age i4 not null )";



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
    II_PTR		tranIDHandle = (II_PTR)NULL;
    IIAPI_CONNPARM	connParm;
    IIAPI_REGXIDPARM	regXIDParm;
    IIAPI_RELXIDPARM	relXIDParm;
    IIAPI_PREPCMTPARM	prepCmtParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    II_UINT4		lowID = 1111, highID = 9999;

    if ( argc != 2 )
    {
	printf( "usage: apis2phs [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(); 
    IIdemo_conn( argv[1], &connHandle );

    /*
    **  Register distributed transaction ID.
    */
    printf( "apis2ph1: register XID\n" );

    regXIDParm.rg_tranID.ti_type = IIAPI_TI_IIXID;
    regXIDParm.rg_tranID.ti_value.iiXID.ii_tranID.it_highTran = highID;
    regXIDParm.rg_tranID.ti_value.iiXID.ii_tranID.it_lowTran = lowID;

    IIapi_registerXID( &regXIDParm );
    
    tranIDHandle = regXIDParm.rg_tranIdHandle;
    
    /*
    **  Perform operations in distributed transaction.
    */
    tranHandle = tranIDHandle; 
    IIdemo_query(&connHandle,&tranHandle,"create table",queryText); 

    /*
    **  Prepare distributed transaction.
    */
    printf( "apis2ph1: prepare to commit \n" );

    prepCmtParm.pr_genParm.gp_callback = NULL;
    prepCmtParm.pr_genParm.gp_closure = NULL;
    prepCmtParm.pr_tranHandle = tranHandle;
    
    IIapi_prepareCommit( &prepCmtParm ); 

    /*
    ** Program terminates without ending the transaction,
    ** closing the connection, or shutting down OpenAPI.
    ** The distributed transaction will still exist in
    ** the DBMS since it was prepared.  Running the demo
    ** apis2ph2 will establish a connection which can
    ** end the distributed transaction.
    */
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


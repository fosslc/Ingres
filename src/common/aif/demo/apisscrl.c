/*
** Copyright (c) 2007 Ingres Corporation
*/


/*
** Name: apisscrl.c 
**
** Description:
**	Demonstrates using IIapi_scroll() and IIapi_position().
**
**	Scroll and position in a result set.
** 
** Following actions are demonstrated in the main()
**	Issue query
**	Get query result descriptors
**	Scroll/position cursor
**	Get query results
**	Free query resources
**
** Command syntax:
**     apisscrl database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init( II_PTR * );
static	void IIdemo_term( II_PTR * );
static	void IIdemo_conn( char *, II_PTR * );
static	void IIdemo_disconn( II_PTR * );
static	void IIdemo_rollback( II_PTR  * );
static	void IIdemo_scroll( II_PTR *, II_UINT2, II_INT4 );
static	void IIdemo_position( II_PTR *, II_UINT2, II_INT4, II_INT2 );
static  void IIdemo_get( II_PTR *, int );

static	char queryText[] = "select table_name from iitables";

#define	ROW_COUNT	10

static	IIAPI_DATAVALUE	DataBuffer[ROW_COUNT];
static	int  resultSize = 0;


/*
** Name: main - main loop
**
** Description:
**	This function is the main loop of the sample code.
*/

int
main( int argc, char** argv ) 
{
    II_PTR		envHandle = (II_PTR)NULL;
    II_PTR		connHandle = (II_PTR)NULL;
    II_PTR		tranHandle = (II_PTR)NULL;
    II_PTR		stmtHandle = (II_PTR)NULL;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_SETDESCRPARM	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
    IIAPI_GETDESCRPARM	getDescrParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    char		varvalue[ROW_COUNT][33];
    int			i;

    if ( argc != 2 )
    {
	printf( "usage: apisscrl [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init( &envHandle ); 
    connHandle = envHandle;
    IIdemo_conn(argv[1],&connHandle);

    /*
    **  Open scrollable cursor
    */	
    printf( "apisscrl: opening cursor\n" );

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_OPEN;
    queryParm.qy_queryText =  queryText;
    queryParm.qy_flags = IIAPI_QF_SCROLL;
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;

    IIapi_query( &queryParm );

    while( queryParm.qy_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    tranHandle = queryParm.qy_tranHandle;
    stmtHandle = queryParm.qy_stmtHandle;

    /*
    **  Get query result descriptors.
    */
    printf( "apisscrl: get descriptors\n" );

    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    IIapi_getDescriptor( &getDescrParm );
    
    while( getDescrParm.gd_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    for( i = 0; i < ROW_COUNT; i++ )
	DataBuffer[i].dv_value = varvalue[i];

    /*
    **  Scroll to last row to determine result set size.
    */
    printf( "apisscrl: scroll last\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_LAST, 0 );
    IIdemo_get( &stmtHandle, 0 );
    
    /*
    **  Scroll to first row.
    */
    printf( "apisscrl: scroll first\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_FIRST, 0 );
    IIdemo_get( &stmtHandle, 1 );
    
    /*
    **  Scroll after last row.
    */
    printf( "apisscrl: scroll after\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_AFTER, 0 );
    IIdemo_get( &stmtHandle, 0 );
    
    /*
    **  Scroll previous to last row.
    */
    printf( "apisscrl: scroll prior\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_PRIOR, 0 );
    IIdemo_get( &stmtHandle, 1 );
    
    /*
    **  Scroll before first row.
    */
    printf( "apisscrl: scroll before\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_BEFORE, 0 );
    IIdemo_get( &stmtHandle, 0 );
    
    /*
    ** Scroll next to first row.
    */
    printf( "apisscrl: scroll next\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_NEXT, 0 );
    IIdemo_get( &stmtHandle, 1 );
    
    /*
    ** Scroll to specific row.
    */
    printf( "apisscrl: scroll absolute\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_ABSOLUTE, resultSize / 2 );
    IIdemo_get( &stmtHandle, 1 );
    
    /*
    ** Refresh current row.
    */
    printf( "apisscrl: scroll current\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_CURRENT, 0 );
    IIdemo_get( &stmtHandle, 1 );
    
    /*
    ** Scroll relative to current position.
    */
    printf( "apisscrl: scroll relative\n" );
    IIdemo_scroll( &stmtHandle, IIAPI_SCROLL_RELATIVE, -ROW_COUNT );
    IIdemo_get( &stmtHandle, 1 );

   
    /*
    ** Position to first block of rows.
    */
    printf( "apisscrl: position first block\n" );
    IIdemo_position( &stmtHandle, IIAPI_POS_BEGIN, 1, ROW_COUNT );
    IIdemo_get( &stmtHandle, ROW_COUNT );
    
    /*
    ** Position to next block of rows.
    */
    printf( "apisscrl: position second block\n" );
    IIdemo_position( &stmtHandle, IIAPI_POS_CURRENT, 1, ROW_COUNT );
    IIdemo_get( &stmtHandle, ROW_COUNT );
    
    /*
    ** Position to last block of rows.
    */
    printf( "apisscrl: position last block\n" );
    IIdemo_position( &stmtHandle, IIAPI_POS_END, -ROW_COUNT, ROW_COUNT );
    IIdemo_get( &stmtHandle, ROW_COUNT );

    /*
    **  Free query resources.
    */
    printf( "apisscrl: close cursor\n" );

    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
	     
    IIdemo_rollback(&tranHandle); 
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
    initParm.in_version = IIAPI_VERSION_6; 
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
    IIAPI_RELENVPARM	relParm;
    IIAPI_TERMPARM	termParm;

    printf( "IIdemo_term: shutting down API\n" );
    relParm.re_envHandle = *envHandle;
    IIapi_releaseEnv( &relParm );
    IIapi_terminate( &termParm );

    *envHandle = NULL;
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
**	connHandle	Environment handle.
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
    connParm.co_type = IIAPI_CT_SQL;
    connParm.co_connHandle = *connHandle;
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


/*
** Name: IIdemo_scroll
**
** Description:
**	Invokes IIapi_scroll() to scroll cursor in result set.
**
** Input:
**	stmtHandle	Statement handle.
**	orientation	Scroll orientation.
**	offset		Scroll offset.
**
** Output:
**	None.
**
** Return value:
**      None.
*/
	
static void
IIdemo_scroll( II_PTR *stmtHandle, II_UINT2 orientation, II_INT4 offset )
{
    IIAPI_SCROLLPARM	scrollParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    scrollParm.sl_genParm.gp_callback = NULL;
    scrollParm.sl_genParm.gp_closure = NULL;
    scrollParm.sl_stmtHandle = *stmtHandle;
    scrollParm.sl_orientation = orientation;
    scrollParm.sl_offset = offset;

    IIapi_scroll( &scrollParm );

    while( scrollParm.sl_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    return;
}


/*
** Name: IIdemo_position
**
** Description:
**	Invokes IIapi_position() to position cursor in result set.
**
** Input:
**	stmtHandle	Statement handle.
**	ref		Position reference
**	offset		Reference offset.
**	rows		Number of rows.
**
** Output:
**	None.
**
** Return value:
**      None.
*/
	
static void
IIdemo_position(II_PTR *stmtHandle, II_UINT2 ref, II_INT4 offset, II_INT2 rows)
{
    IIAPI_POSPARM	posParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    posParm.po_genParm.gp_callback = NULL;
    posParm.po_genParm.gp_closure = NULL;
    posParm.po_stmtHandle = *stmtHandle;
    posParm.po_reference = ref;
    posParm.po_offset = offset;
    posParm.po_rowCount = rows;

    IIapi_position( &posParm );

    while( posParm.po_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    return;
}


/*
** Name: IIdemo_get
**
** Description:
**	Invokes IIapi_getColumns(), if rows requested > 0, to
**	receive the query results, then invokes IIapi_getQueryInfo()
*	to check query status.
**
** Input:
**	stmtHandle	Statement handle.
**	rows		Number of rows requested.
**
** Output:
**	None.
**
** Return value:
**      None.
*/
	
static void
IIdemo_get( II_PTR *stmtHandle, int rows )
{
    IIAPI_GETCOLPARM	getColParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_WAITPARM	waitParm = { -1 };

    if ( rows > 0 )
    {
	getColParm.gc_genParm.gp_callback = NULL;
	getColParm.gc_genParm.gp_closure = NULL;
	getColParm.gc_stmtHandle = *stmtHandle;
	getColParm.gc_rowCount = rows;
	getColParm.gc_columnCount = 1;
	getColParm.gc_columnData = DataBuffer;
	getColParm.gc_moreSegments = 0;

	IIapi_getColumns( &getColParm );

	while( getColParm.gc_genParm.gp_completed == FALSE )
	    IIapi_wait( &waitParm );
    }

    /*
    **  Get fetch result info.
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = *stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while( getQInfoParm.gq_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    if ( getQInfoParm.gq_mask & IIAPI_GQ_ROW_STATUS )
    {
	if ( getQInfoParm.gq_rowStatus & IIAPI_ROW_BEFORE )
	    if ( getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT )
		printf( "\tCursor BEFORE (position %d, rows %d)\n",
			getQInfoParm.gq_rowPosition, getQInfoParm.gq_rowCount );
	    else
		printf( "\tCursor BEFORE (position %d)\n",
			getQInfoParm.gq_rowPosition );
	else if ( getQInfoParm.gq_rowStatus & IIAPI_ROW_AFTER )
	    if ( getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT )
		printf( "\tCursor AFTER (position %d, rows %d)\n",
			getQInfoParm.gq_rowPosition, getQInfoParm.gq_rowCount );
	    else
		printf( "\tCursor AFTER (position %d)\n",
			getQInfoParm.gq_rowPosition );
	else
	{
	    if ( getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT )
		printf( "\tCursor @ %d, received %d row%s\n",
			getQInfoParm.gq_rowPosition, getQInfoParm.gq_rowCount,
			(getQInfoParm.gq_rowCount != 1) ? "s" : "" );
	    else
		printf( "\tCursor @ %d\n", getQInfoParm.gq_rowPosition );

	    if ( getQInfoParm.gq_rowStatus & IIAPI_ROW_LAST )
	    	resultSize = getQInfoParm.gq_rowPosition;
	}
    }
    else  if ( getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT )
    {
	printf( "\treceived %d row%s\n", getQInfoParm.gq_rowCount,
		(getQInfoParm.gq_rowCount != 1) ? "s" : "" );
    }

    return;
}


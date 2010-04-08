/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apis2ph2.c 
**
** Description:
**	Demonstrates using IIapi_registerXID(), IIapi_connect() with
**	transaction handle, IIapi_releaseXID().
**
**	Second part of two phase commit demo.  Run apis2ph1 to begin
**	distributed transaction.  Reconnects to distribute transaction
**	and rolls back the transaction.
**
** Following actions are demonstrated in the main()
**	Register distributed transaction ID.
**	Establish connection with distributed transaction.
**	Rollback distributed transaction.
**	Release distributed transaction ID.
**
** Command syntax: apis2ph2 database_name
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init();
static	void IIdemo_term();
static	void IIdemo_disconn( II_PTR *);



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
    IIAPI_ROLLBACKPARM	rollbackParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    II_UINT4		lowID = 1111, highID = 9999;

    if ( argc != 2 )
    {
	printf( "usage: apis2phs [vnode::]dbname[/server_class]\n" );
	exit( 0 );
    }

    IIdemo_init(); 

    /*
    **  Register distributed transaction ID.
    */
    printf( "apis2ph2: register XID\n" );

    regXIDParm.rg_tranID.ti_type = IIAPI_TI_IIXID;
    regXIDParm.rg_tranID.ti_value.iiXID.ii_tranID.it_highTran = highID;
    regXIDParm.rg_tranID.ti_value.iiXID.ii_tranID.it_lowTran = lowID;

    IIapi_registerXID( &regXIDParm );
    
    tranIDHandle = regXIDParm.rg_tranIdHandle;

    /*
    **  Establish connection which is associated 
    **  with the distributed transaction.
    */
    printf( "apis2ph2: establishing connection\n" );
    
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  argv[1];;
    connParm.co_connHandle = NULL;
    connParm.co_tranHandle = tranIDHandle;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    while( connParm.co_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    connHandle = connParm.co_connHandle;
    tranHandle = connParm.co_tranHandle;

    /*
    **  Rolback the distributed transaction.
    */
    printf( "apis2ph2: rollback \n");

    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = tranHandle;
    rollbackParm.rb_savePointHandle = NULL; 

    IIapi_rollback( &rollbackParm );

    while(rollbackParm.rb_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    /*
    **  Release distributed transaction ID.
    */
    printf( "apis2ph2: release XID\n" );

    relXIDParm.rl_tranIdHandle   = tranIDHandle;

    IIapi_releaseXID( &relXIDParm );

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


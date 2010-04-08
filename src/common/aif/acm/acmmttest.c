/*
** Copyright (C) 1998, 2004 Ingres Corporation
*/
#ifndef NT_GENERIC
#include <stdio.h>
main()
{
	printf("This program only runs under Windows NT.\n");
}
#else	/* NT_GENERIC */		  

# include <stdio.h>
# include <stdlib.h>
# include <windows.h>

# include   <compat.h>
# include   <si.h>
# include   <cs.h>
# include   <tm.h>
# include   <iiapi.h>
# include   <sapiw.h>
# include   <sqlstate.h>
# include   <ccm.h>

static void *task_func( void * );

int		TaskCount;
int		Iterations;

static bool yield = TRUE;

/*
** Name: acmmttest.c - Connection Manager Multi-threaded tester
**
** Description:
**	This file contains the main routine to run ACM multi-threaded tester.
**
**	Arguments:	<iterations> <threads>  
**	    iterations - number of query cycles.  (Default is 2)
**	    threads - number of threads of query cycles.  (Default is 5)
**
**  History:
**  26-May-1998 (shero03)
**      Created.
**	04-Mar-1999 (shero03)
**	    changed err to be pointer to API Error Info Block      
**	26-Apr-1999 (shero03)
**	    Support environment handle on SW init and term.
**      29-nov-2004 (rigka01) bug 111394, problem INGSRV2624
**          Cross the part of change 466624 from main to ingres26 that
**          applies to bug 111394 (change 466196).
**	30-mar-2004 (somsa01)
**	    Added IIAPI_GETQINFOPARM argument to IIsw_selectLoop().
**	29-Sep-2004 (drivi01)
**	    Added NEEDLIBS hints for generating Jamfile.
*/

/*
**
NEEDLIBS = ACMLIB SAWLIB APILIB CUFLIB GCFLIB ADFLIB \
	   COMPATLIB 
**
*/

static STATUS RunQuery(char *dbname,
                       char *userid,
                       char *flags);

static void PrintErrorMessage(IIAPI_GETEINFOPARM * err);

/*
** Name: main() - the main routine of Connection Manager multi-threaded tester
**
** Description:
**	This function is the main routine to run Connection Manager multi-threaded tester.
**
** Inputs:
**	argc		Number of command line parameters
**	argv		Array of command line parameter strings
**
** Outputs:
**	None.
**
** Returns:
**	int		Status code
**
** History:
**  26-May-1998 (shero03)
**      Created.
**	26-jan-2004 (penga03)
**      Added an argument for IIsw_initialize.
*/

int
main( int argc, char **argv )
{
	int		iterations = 2;
	int		threads = 5;
	int		i;
	int		tid;
    	II_PTR		envptr;
    	IIAPI_GETEINFOPARM err;

	if (argc > 1)
		iterations = atoi(argv[1]);
	if (argc > 2)
		threads = atoi(argv[2]);

	if ((iterations < 1) ||
		(threads < 1) )
	{
		printf("\nInvalid Parameter\n");
		printf("Usage: <iterations> <threads>\n\n");
		exit(0);
	}		  

    TaskCount = threads;
    Iterations = iterations;

    if (IIsw_initialize(&envptr,0) == IIAPI_ST_SUCCESS) /* Initialize api, cm */
    {

    	/* Initialize the Connection Manager */
    	IIcm_Initialize((PTR)&err);

    	for( i = 0; i < threads; i++ )
    	{
	      CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) task_func, 
				0, 0, &tid );
	      printf( "[%d] Created task %d\n", GetCurrentThreadId(), tid );
	      if ( yield )  Sleep(0);
    	}

    	while (TaskCount > 0)
	      Sleep (1000);

    	printf( "[%d] Exiting\n", GetCurrentThreadId() );

        /* Terminate the Connection Manager */
        IIcm_Terminate((PTR)&err);

        /* Terminate saw */
        IIsw_terminate(envptr);
    }

    return( 0 );
}		/* procedure main */

static void *
task_func( void *arg )
{
    int		i;
    int		tuple = 0;
    STATUS	ret;
    char    	dbname[] = "iidbdb";
    char    	*userid = NULL;
    char    	*flags = NULL;
    SYSTIME	now;

    printf( "[%d] Starting\n", GetCurrentThreadId() );

    for (i = 0; i < Iterations; i++)
    {
    	if ( yield  && (i > 0)) 
        	Sleep(now.TM_msecs / 500);

       	/* FIXME Get the flags from input */
       	ret = RunQuery(dbname, userid, flags);
	TMnow(&now);
    }

    printf( "[%d] Exiting\n", GetCurrentThreadId() );

    InterlockedDecrement (&TaskCount);
    return( NULL );
}

/*
**  Name: RunQuery - Run the canned query
**
**  Description:
**      Connect to the database using the Connection Manager
**      Send the select statement to the Database server
**      Retrieve the tuples from the Database server
**      Committ the transaction
**      Drop the connection from the Connection Manager
**
**  Inputs:
**      field		- binary field to convert
**
**  Outputs:
**
**  Returns:
**      Hash index
**
**  History:
**      26-Feb-98 (shero03)
**          Created.
**      31-Mar-04 (penga03)
**          Added an argument &qinfoParm for IIsw_selectLoop.
**	30-mar-2004 (somsa01)
**	    Added IIAPI_GETQINFOPARM argument to IIsw_selectLoop().
**	05-Apr-04 (penga03)
**	    Removed the redefinition of IIsw_selectLoop.
*/
static STATUS RunQuery(char *dbname, char *userid, char *flags)
{

    PTR      		hConn = NULL;
    PTR			hStmt = NULL;
    PTR			hTran = NULL;
    IIAPI_DATAVALUE  	cdv[2];
    char		query[] = "select relid, reltid from iirelation where reltid < 10";
    IIAPI_GETQINFOPARM	qinfoParm;
    IIAPI_GETEINFOPARM	err;
    II_LONG     	lreltid;
    char        	lrelid[33];
    II_LONG		cRows;
    STATUS		ret;

    /* Connect to the database */
    ret = IIcm_GetAPIConnection(dbname, userid, flags, &hConn, (PTR)&err);

    if (ret != IIAPI_ST_SUCCESS)
    {	
       PrintErrorMessage(&err);
       return IIAPI_ST_ERROR;
    }
    	
    printf("[%d] Connection %x\n", GetCurrentThreadId(), hConn);
    if ( yield )  Sleep(0);

    /* Retrieve a Row */
    SW_COLDATA_INIT(cdv[0], lrelid);
    SW_COLDATA_INIT(cdv[1], lreltid);

    while (ret == IIAPI_ST_SUCCESS)
    {
    	ret = IIsw_selectLoop (hConn, &hTran, query,
    			 0, NULL, NULL, 2, cdv, &hStmt, &qinfoParm, &err);
    	if (ret == IIAPI_ST_SUCCESS)
    	{
    	   /*
      	   ** SW_CHA_TERM(lrelid, cdv[0]);
	   ** printf(" %32s  %4d\n", lrelid, lreltid);
	   */
    	   if ( yield )  Sleep(0);
	}
        else if (ret == IIAPI_ST_NO_DATA)
    	{
	   /* Retrieve the query result */
    	   cRows = IIsw_getRowCount(hStmt);
    	   printf( "[%d] read %d rows \n", GetCurrentThreadId(), cRows );
	   break;
	}
	else
	{	
            PrintErrorMessage(&err);
	    hStmt = NULL;
	    break;
	 }
    }

    /* Close the statement */
    if (hStmt)
    {
    	ret = IIsw_close(hStmt, &err);
    	if (ret != IIAPI_ST_SUCCESS)
    	    PrintErrorMessage(&err);
    	else if ( yield )  Sleep(0);
    }

    /* Commit the transaction */
    if (hTran)
    {
    	ret = IIsw_commit(&hTran, &err);
    	if (ret != IIAPI_ST_SUCCESS)
            PrintErrorMessage(&err);
	else if ( yield )  Sleep(0);
    }

    /* Drop the connection */
    if (hConn)
    {
        ret = IIcm_DropAPIConnection(hConn, TRUE, (PTR)&err);
        if (ret != IIAPI_ST_SUCCESS)
            PrintErrorMessage(&err);
    	else if ( yield )  Sleep(0);
			
        hConn = NULL;
    }

    return ret;
}       /* proc RunQuery */

/*
**  Name: PrintErrorMessage
**
**  Description:
**      Prints the error message from the last statement
**
**  Inputs:
**      hErr       error handle
**
**  Outputs:
**
**  Returns:
**
** History:
**  26-May-1998 (shero03)
**          Created
*/
static void PrintErrorMessage(IIAPI_GETEINFOPARM * err)
{
    int		i;
    char 	sqlstate[ 6 ], type[ 33 ];

    do
    {
	if ( err->ge_status != IIAPI_ST_SUCCESS )
	   break;
	    
	 switch( err->ge_type )
	{
	    case IIAPI_GE_ERROR		: strcpy( type, "ERROR" );	break;
	    case IIAPI_GE_WARNING	: strcpy( type, "WARNING" );	break;
	    case IIAPI_GE_MESSAGE	: strcpy( type, "USER" );	break;
	    default : sprintf( type, "????? (%d)", err->ge_type );	break;
	 }

	 printf( "[%d] Info: %s '%s' %d: %s\n", GetCurrentThreadId(),
			type, err->ge_SQLSTATE, err->ge_errorCode,
			err->ge_message ? err->ge_message : "NULL" );

	 if ( err->ge_serverInfoAvail )
	 {
	     ss_decode( sqlstate, err->ge_serverInfo->svr_id_error );
	     printf( "[%d]     : %s (0x%x) %d (0x%x) %d %d 0x%x\n",
	     	GetCurrentThreadId(), sqlstate, 
	       	err->ge_serverInfo->svr_id_error,
	       	err->ge_serverInfo->svr_local_error,
	       	err->ge_serverInfo->svr_local_error,
	       	err->ge_serverInfo->svr_id_server,
	       	err->ge_serverInfo->svr_server_type,
	       	err->ge_serverInfo->svr_severity );

	     for( i = 0; i < err->ge_serverInfo->svr_parmCount; i++ )
	     {	
	     	printf( "[%d]     : ", GetCurrentThreadId() );

		if ( err->ge_serverInfo->svr_parmDescr[i].ds_columnName && 
		     *err->ge_serverInfo->svr_parmDescr[i].ds_columnName)
		    printf("%s = ", 
		     err->ge_serverInfo->svr_parmDescr[i].ds_columnName );

		    if ( err->ge_serverInfo->svr_parmDescr[i].ds_nullable  &&  
		     	err->ge_serverInfo->svr_parmValue[i].dv_null )
		       	printf( "NULL" );
		    else
		    {
		    	switch( err->ge_serverInfo->svr_parmDescr[i].ds_dataType )
		    	{
			    case IIAPI_CHA_TYPE :
			      printf( "'%*.*s'", 
			      err->ge_serverInfo->svr_parmValue[i].dv_length,
			      err->ge_serverInfo->svr_parmValue[i].dv_length,
			      err->ge_serverInfo->svr_parmValue[i].dv_value );
			      break;

			    default : 
			      printf( "Datatype %d not displayed",
			      err->ge_serverInfo->svr_parmDescr[i].ds_dataType );
			      break;
		    	}
		     }

		     printf( "\n" );
	    	}
	 }
    } while( 1 );
}

#endif	/* NT_GENERIC */

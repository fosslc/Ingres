/*
** Copyright (C) 1998, 2004 Ingres Corporation
*/
# include   <compat.h>
# include   <si.h>
# include   <cs.h>
# include   <iiapi.h>
# include   <sapiw.h>
# include   <ccm.h>

/*
NEEDLIBS = ACMLIB SAWLIB APILIB CUFLIB GCFLIB ADFLIB
** Name: acmtest.c - Connection Manager tester
**
** Description:
**	This file contains the main routine to run ACM tester.
**
**	Arguments:	<iterations>
**	    iterations - number of query cycles.  (Default is 2)
**
**  History:
**  11-Mar-1998 (shero03)
**      Created.
**	21-May-1998 (shero03)
**	    Add iterations parameter	
**	04-Mar-1999 (shero03)
**	    changed err to be pointer to API Error Info Block      
**	26-Apr-1999 (shero03)
**	    Support environment handle on SW init and term.
**      29-nov-2004 (rigka01) bug 111394, problem INGSRV2624
**          Cross the part of change 466624 from main to ingres26 that
**          applies to bug 111394 (change 466196).
**	30-mar-2004 (somsa01)
**	    Added getQinfoParm as input parameter to IIsw_ functions.
**	14-Jul-2004 (hanje04)
**	    Added NEEDLIBS to aid correct generation of Jamfile
*/
static STATUS RunQuery(char *dbname,
                       char *userid,
                       char *flags);

static void PrintErrorMessage(IIAPI_GETEINFOPARM * err);

/*
** Name: main() - the main routine of Connection Manager tester
**
** Description:
**	This function is the main routine to run Connectioni Manager tester.
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
**	11-Mar-1998 (shero03)
**	    Created.
**	21-May-1998 (shero03)
**	    Add iterations parameter	
**	26-jan-2004 (penga03)
**	    Added an argument for IIsw_initialize.    
*/

int
main( int argc, char **argv )
{
    int		iterations = 2;
    int		i;
    char    	dbname[] = "iidbdb";
    char    	*userid = NULL;
    char    	*flags = NULL;
    STATUS	ret;
    II_PTR	envptr;
    IIAPI_GETEINFOPARM err;

	if (argc > 1)
		iterations = atoi(argv[1]);
	if (iterations < 1)
	{
		printf("\nInvalid Parameter\n");
		printf("Usage: <iterations>\n\n");
		exit(0);
	}		  

    if (IIsw_initialize(&envptr,0) == IIAPI_ST_SUCCESS) /* Initialize api, cm */
    {

    	/* Initialize the Connection Manager */
    	IIcm_Initialize((PTR)&err);

		for (i = 0; i < iterations; i++)
		{
        	/* FIXME Get the flags from input */
        	ret = RunQuery(dbname, userid, flags);
		}

        /* Terminate the Connection Manager */
        IIcm_Terminate((PTR)&err);

        /* Terminate saw */
        IIsw_terminate(envptr);
    }

    return( 0 );
}		/* procedure main */

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
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes, use II_PTR not PTR.
*/
static STATUS RunQuery(char *dbname, char *userid, char *flags)
{

    PTR      		hConn = NULL;
    II_PTR		hStmt = NULL;
    II_PTR		hTran = NULL;
    IIAPI_DATAVALUE  	cdv[2];
    char		query[] = "select relid, reltid from iirelation where reltid < 50";
    II_LONG     	lreltid;
    char        	lrelid[33];
    II_LONG		cRows;
    IIAPI_GETQINFOPARM  qinfoParm;
    IIAPI_GETEINFOPARM	err;
    STATUS		ret;

    /* Connect to the database */
    ret = IIcm_GetAPIConnection(dbname, userid, flags, &hConn, (PTR)&err);

    if (ret != IIAPI_ST_SUCCESS)
    {	
       PrintErrorMessage(&err);
       return IIAPI_ST_ERROR;
    }

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
	}
        else if (ret == IIAPI_ST_NO_DATA)
    	{
		/* Retrieve the query result */
    		cRows = IIsw_getRowCount(hStmt);
    		printf( " %d rows \n", cRows);
		break;
	}
	else
	{	
            PrintErrorMessage(&err);
	    break;
	}
    }

    /* Close the statement */
    ret = IIsw_close(hStmt, &err);
    if (ret != IIAPI_ST_SUCCESS)
        PrintErrorMessage(&err);

    /* Commit the transaction */
    ret = IIsw_commit(&hTran, &err);
    if (ret != IIAPI_ST_SUCCESS)
        PrintErrorMessage(&err);

    /* Drop the connection */
    if (hConn != NULL)
    {
        ret = IIcm_DropAPIConnection(hConn, TRUE, (PTR)&err);
        if (ret != IIAPI_ST_SUCCESS)
            PrintErrorMessage(&err);
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
**      22-Mar-1998 (shero03)
**          Created
*/
static void PrintErrorMessage(IIAPI_GETEINFOPARM * err)
{
    if (err->ge_message)
    	printf("%s/d", err->ge_message);
}

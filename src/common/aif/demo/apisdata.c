/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisdata.c 
**
** Description:
**	Demonstrates using IIapi_formatData() and IIapi_convertData()
**
** Following actions are demonstrated in the main()
**     Set environment parameter
**     Convert string to DATE using environment date format.
**     Convert DATE to string using installation date format.
**
** Command syntax: apisdata
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>


static	void IIdemo_init( II_PTR *);
static	void IIdemo_term( II_PTR *);



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
    II_PTR		envHandle = (II_PTR)NULL;
    IIAPI_SETENVPRMPARM	setEnvPrmParm;
    IIAPI_FORMATPARM	formatParm;
    IIAPI_CONVERTPARM	convertParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    char		isodate[] = "20020304 10:30:00 pm";
    char		date[12];
    char		date_str[26];
    II_LONG		paramvalue;

    IIdemo_init(&envHandle); 

    /* 
    **  Set date format environment parameter.
    */
    printf( "apisdata: set environment parameter\n" );

    setEnvPrmParm.se_envHandle = envHandle;
    setEnvPrmParm.se_paramID = IIAPI_EP_DATE_FORMAT;
    setEnvPrmParm.se_paramValue = (II_PTR)&paramvalue;
    paramvalue = IIAPI_EPV_DFRMT_ISO; 

    IIapi_setEnvParam( &setEnvPrmParm );

    /* 
    **  Convert ISO date string to Ingres internal date  
    */
    printf( "\tISO date = %s\n", isodate);
    printf( "apisdata: formatting date\n" );

    formatParm.fd_envHandle = envHandle; 
    formatParm.fd_srcDesc.ds_dataType  = IIAPI_CHA_TYPE;  
    formatParm.fd_srcDesc.ds_nullable  = FALSE;  
    formatParm.fd_srcDesc.ds_length  = sizeof(isodate);  
    formatParm.fd_srcDesc.ds_precision = 0;
    formatParm.fd_srcDesc.ds_scale = 0;
    formatParm.fd_srcDesc.ds_columnType = IIAPI_COL_QPARM;
    formatParm.fd_srcDesc.ds_columnName = NULL;
    
    formatParm.fd_srcValue.dv_null = FALSE;
    formatParm.fd_srcValue.dv_length = sizeof(isodate);
    formatParm.fd_srcValue.dv_value = isodate;

    formatParm.fd_dstDesc.ds_dataType = IIAPI_DTE_TYPE;
    formatParm.fd_dstDesc.ds_nullable = FALSE;
    formatParm.fd_dstDesc.ds_length = 12;
    formatParm.fd_dstDesc.ds_precision = 0;
    formatParm.fd_dstDesc.ds_scale = 0;
    formatParm.fd_dstDesc.ds_columnType = IIAPI_COL_QPARM;
    formatParm.fd_dstDesc.ds_columnName = NULL;

    formatParm.fd_dstValue.dv_null = FALSE;
    formatParm.fd_dstValue.dv_length = sizeof(date);
    formatParm.fd_dstValue.dv_value = &date;

    IIapi_formatData( &formatParm );

    /*
    **  Convert Ingres internal date to string
    **  using installation date format.
    */
    printf( "apisdata: converting date\n" );

    convertParm.cv_srcDesc.ds_dataType  = IIAPI_DTE_TYPE;  
    convertParm.cv_srcDesc.ds_nullable  = FALSE;  
    convertParm.cv_srcDesc.ds_length  = 12;  
    convertParm.cv_srcDesc.ds_precision = 0;
    convertParm.cv_srcDesc.ds_scale = 0;
    convertParm.cv_srcDesc.ds_columnType = IIAPI_COL_QPARM;
    convertParm.cv_srcDesc.ds_columnName = NULL;
    
    convertParm.cv_srcValue.dv_null = FALSE;
    convertParm.cv_srcValue.dv_length = sizeof(date);
    convertParm.cv_srcValue.dv_value = &date;

    convertParm.cv_dstDesc.ds_dataType = IIAPI_CHA_TYPE;
    convertParm.cv_dstDesc.ds_nullable = FALSE;
    convertParm.cv_dstDesc.ds_length = sizeof(date_str) - 1;
    convertParm.cv_dstDesc.ds_precision = 0;
    convertParm.cv_dstDesc.ds_scale = 0;
    convertParm.cv_dstDesc.ds_columnType = IIAPI_COL_QPARM;
    convertParm.cv_dstDesc.ds_columnName = NULL;

    convertParm.cv_dstValue.dv_null = FALSE;
    convertParm.cv_dstValue.dv_length = sizeof(date_str) - 1;
    convertParm.cv_dstValue.dv_value = date_str;

    IIapi_convertData( &convertParm );

    date_str[ convertParm.cv_dstValue.dv_length ] = '\0';
    printf( "\tdate = %s\n", date_str );
     
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


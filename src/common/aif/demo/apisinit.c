/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apisinit.c  
**
** Description:
**	Demonstrates using IIapi_init(),IIapi_setEnvParam(), 
**	IIapi_releaseEnv() and IIapi_term()
**
** Following actions are demonstrate in the main()
**	Initialize at level 1
**	Terminate API
**	Initialize at level 2
**	Set environment parameter
**	Release environmment resources
**	Terminate API
**
** Command syntax: apisinit
*/


# include <stdio.h>
# include <stdlib.h>
# include <iiapi.h>



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
    IIAPI_INITPARM	initParm;
    IIAPI_SETENVPRMPARM	setEnvPrmParm;
    IIAPI_RELENVPARM	relEnvParm;
    IIAPI_TERMPARM	termParm;
    II_LONG		longvalue; 

    /*
    **  Initialize API at level 1 
    */
    printf( "apisinit: initializing API at level 1 \n" );
    initParm.in_version = IIAPI_VERSION_1;
    initParm.in_timeout = -1;

    IIapi_initialize( &initParm );

    /*
    **  Terminate API 
    */
    printf( "apisinit: shutting down API\n" );

    IIapi_terminate( &termParm );

    /*
    **  Initialize API at level 2.  Save environment handle.
    */
    printf( "apisinit: initializing API at level 2 \n" );
    initParm.in_version = IIAPI_VERSION_2;
    initParm.in_timeout = -1;

    IIapi_initialize( &initParm );

    envHandle = initParm.in_envHandle; 
    
    /*
    **  Set an environment parameter
    */
    printf( "apisinit: set environment parameter\n" );
    setEnvPrmParm.se_envHandle = envHandle;
    setEnvPrmParm.se_paramID = IIAPI_EP_DATE_FORMAT;
    setEnvPrmParm.se_paramValue = (II_PTR)&longvalue;
    longvalue = IIAPI_EPV_DFRMT_YMD;

    IIapi_setEnvParam( &setEnvPrmParm );

    /*
    **  Release environment resources.
    */
    printf( "apisinit: releasing environment handle\n" );
    relEnvParm.re_envHandle = envHandle;

    IIapi_releaseEnv(&relEnvParm);

    /*
    **  Terminate API.
    */
    printf( "apisinit: shutting down API\n" );

    IIapi_terminate( &termParm );

    return( 0 );
}


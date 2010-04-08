/******************************************************************************
**
** Copyright (c) 1999, 2002 Ingres Corporation
**
******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include "tngapi.h"

/******************************************************************************
**
** Name:        test.c
**
** Description:
**	A very basic test harness for the TNG API.
**
** History:
**	05-may-1999 (mcgem01)
**	    Created.
**	02-aug-2000 (mcgem01)
**	    Add a test for the II_ServiceStarted function.
**	17-aug-2000 (mcgem01)
**	    Add the service startup and shutdown functions.
**	    Also added a function which gets the value of an 
**	    Ingres environment variable.
**	27-dec-2001 (somsa01)
**	    Added usage of II_IngresServiceName().
**	27-dec-2001 (somsa01)
**	    Added usage of II_IngresVersion().
**	11-jan-2002 (somsa01)
**	    Added usage of II_StartServiceSync().
**	14-jan-2002 (somsa01)
**	    Added usage of II_IngresVersionEx();
**	15-mar-2002 (somsa01)
**	    Added usage of II_GetIngresInstallSize().
**	17-apr-2002 (abbjo03)
**	    Add test for II_PrecheckInstallation.
**	30-Dec-2004 (wansh01) 
**	    Changed TNGAPImodule name to iilibutil.dll.  
**
******************************************************************************/

HINSTANCE   hTNGAPIModule=NULL;

FARPROC     lpfnII_PingGCN;
FARPROC     lpfnII_IngresVersion;
FARPROC     lpfnII_IngresVersionEx;
FARPROC     lpfnII_TNG_Version;
FARPROC     lpfnII_ServiceStarted;
FARPROC     lpfnII_StartService;
FARPROC     lpfnII_StartServiceSync;
FARPROC     lpfnII_StopService;
FARPROC     lpfnII_IngresServiceName;
FARPROC     lpfnII_GetEnvVariable;
FARPROC     lpfnII_GetIngresInstallSize;
FARPROC     lpfnII_PrecheckInstallation;

#define II_PingGCN		(*lpfnII_PingGCN)
#define II_IngresVersion	(*lpfnII_IngresVersion)
#define II_IngresVersionEx	(*lpfnII_IngresVersionEx)
#define II_TNG_Version		(*lpfnII_TNG_Version)
#define II_ServiceStarted	(*lpfnII_ServiceStarted)
#define II_StartService		(*lpfnII_StartService)
#define II_StartServiceSync	(*lpfnII_StartServiceSync)
#define II_StopService		(*lpfnII_StopService)
#define II_IngresServiceName	(*lpfnII_IngresServiceName)
#define II_GetEnvVariable	(*lpfnII_GetEnvVariable)
#define II_GetIngresInstallSize (*lpfnII_GetIngresInstallSize)
#define II_PrecheckInstallation	(*lpfnII_PrecheckInstallation)

int
main(int argc, char *argv[])
{
    DWORD	dwError;
    char	*value;
    int		testres;
    char	ServiceName[32];
    char	VersionString[32];
    double	size;
    char	CurrentDirectory[512];
    char	err_msg[1024];

    hTNGAPIModule=LoadLibrary("iilibutil.dll");

    if (!hTNGAPIModule)
    {
	dwError = GetLastError();

	switch (dwError)
	{
		case ERROR_INVALID_DLL:
		    printf("OIUTIL.DLL is not a valid 32-bit DLL.\n");
		    break;

		case ERROR_MOD_NOT_FOUND:
		case ERROR_DLL_NOT_FOUND:
		    printf("OIUTIL.DLL could not be found in the path.\n");
		    break;

		default:
		    printf ("LoadLibrary failed with error %d.\n", dwError);
		    break;
	}
	exit (2);
    }

    lpfnII_PingGCN = GetProcAddress(hTNGAPIModule,"II_PingGCN");
    lpfnII_IngresVersion = GetProcAddress(hTNGAPIModule,"II_IngresVersion");
    lpfnII_IngresVersionEx = GetProcAddress(hTNGAPIModule,"II_IngresVersionEx");
    lpfnII_TNG_Version = GetProcAddress(hTNGAPIModule,"II_TNG_Version");
    lpfnII_ServiceStarted = GetProcAddress(hTNGAPIModule,"II_ServiceStarted");
    lpfnII_StartService = GetProcAddress(hTNGAPIModule,"II_StartService");
    lpfnII_StartServiceSync = GetProcAddress(hTNGAPIModule,
					     "II_StartServiceSync");
    lpfnII_StopService = GetProcAddress(hTNGAPIModule,"II_StopService");
    lpfnII_IngresServiceName = GetProcAddress(hTNGAPIModule,
					      "II_IngresServiceName");
    lpfnII_GetEnvVariable = GetProcAddress(hTNGAPIModule,"II_GetEnvVariable");
    lpfnII_GetIngresInstallSize = GetProcAddress(hTNGAPIModule,
						 "II_GetIngresInstallSize");
    lpfnII_PrecheckInstallation = GetProcAddress(hTNGAPIModule,
						"II_PrecheckInstallation");

    if (!lpfnII_PingGCN || !lpfnII_TNG_Version || !lpfnII_ServiceStarted
	|| !lpfnII_StartService || !lpfnII_StopService
	|| !lpfnII_IngresServiceName || !lpfnII_GetEnvVariable
	|| !lpfnII_StartServiceSync || !lpfnII_IngresVersionEx
	|| !lpfnII_GetIngresInstallSize || !lpfnII_PrecheckInstallation)
    {
	dwError = GetLastError();
	printf ("Unable to locate API functions in the DLL.Error: %d\n",
		dwError);
	exit (dwError);
    }
#ifdef NOT_DEFINED
    II_GetEnvVariable ("II_INSTALLATION", &value);

    printf ("The value of II_INSTALLATION is %s\n\n", value);

    testres =  II_PingGCN ();

    if (testres)
    {
    	testres =  II_ServiceStarted ();
        if (testres)
	{
	    printf ("Ingres was started as a service.\n");
	    printf ("Stopping the Ingres Service.\n");
    	    testres = II_StopService ();
	    if (testres)
		printf ("Service Failed to Stop.  Error was %d\n", testres);
	}
        else
	    printf ("Ingres was started from the command line.\n");
    }
    else
    {
	printf ("Ingres installation has not been started.\n");
	printf ("Starting the Ingres Service.\n");
	testres = II_StartServiceSync ();
	if (testres)
	    printf ("Service Failed to Start.  Error was %d\n", testres);
    }

    II_IngresServiceName(ServiceName);
    printf("The Ingres Service name is '%s'.\n", ServiceName);

    testres =  II_TNG_Version ();
    if (testres)
	printf ("This is a TNG aware version of Ingres.\n");
    else
	printf ("I wouldn't try it!\n");
#endif
    testres = II_IngresVersion();
    printf("Status of II_IngresVersion() is %d.\n", testres);

    testres = II_IngresVersionEx(VersionString);
    printf("Status of II_IngresVersionEx() is %d, version is '%s'.\n",
	   testres, VersionString);

    if (GetCurrentDirectory(sizeof(CurrentDirectory), CurrentDirectory))
    {
	strcat(CurrentDirectory, "\\sample.rsp");
	if (GetFileAttributes(CurrentDirectory) != -1)
	{
	    II_GetIngresInstallSize(CurrentDirectory, &size);
	    printf( "Ingres install size, using sample.rsp, is %.3f MB\n",
		    size );
	}
    }

    if (argc > 1)
    {
	testres = II_PrecheckInstallation(argv[1], err_msg);
	printf("II_PrecheckInstallation returned '%s'\n", err_msg);
    }

    if (hTNGAPIModule)
    {
	FreeLibrary (hTNGAPIModule);
	hTNGAPIModule = NULL;
    }

    return 0;
}

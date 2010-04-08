/*
**  Copyright (c) 1995, 2001 Ingres Corporation
**
**  Name: clntproc.c
**
**  Description:
**
**	Used by the NT Service Manager to start Ingres client using
**	ingstart -client and to stop Ingres using ingstop.exe
**
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <compat.h>
#include "clntproc.h"


/*
** Name: main
**
** Description:
**	Called when the Ingres client is started by the NT Service
**	Manager.  Report error in the EventLog if it fails to start.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**	None.
**
** History:
**	19-oct-95 (emmag)
**	    Created.
**	16-dec-95 (emmag)
**	    Append bin as well as utility to the path.  Set the II_SYSTEM
**	    environment variable before setting the path.
**	14-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	10-apr-98 (mcgem01)
**	    Product name change to Ingres.
**	12-feb-2001 (somsa01)
**	    Cleaned up compiler warnings.
*/

VOID	
main()
{
	SERVICE_TABLE_ENTRY	lpServerServiceTable[] = 
	{
	    { TEXT ("Ingres_Client"), 
	    ( LPSERVICE_MAIN_FUNCTION) OpenIngresService},
	    { NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher( lpServerServiceTable ))
	{
	    StopService("Failed to start Ingres Client");
	}
	return;
}


/*
** Name: OpenIngresService	
**
** Description:
**	Register the control handling function and start the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
*/

VOID
OpenIngresService(DWORD dwArgc, LPSTR *lpszArgv)
{
	DWORD			dwWFS;

	/* 
	** Register contorl handling function with the control dispatcher.
	*/

	sshStatusHandle = RegisterServiceCtrlHandler(
					TEXT("Ingres_Client"),
					IngresCtrlHandler);

	if ( !sshStatusHandle )
		goto cleanup;

	ssServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	ssServiceStatus.dwServiceSpecificExitCode = 0;

	/* 
	** Update status of SCM to SERVICE_START_PENDING	
	*/
	if (!ReportStatusToSCMgr(
		SERVICE_START_PENDING,
		NO_ERROR,
		2,
		3000000))
		goto cleanup;

	/* 
	** Create an event that will handle termination.	
	*/
	hServDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

	if ( hServDoneEvent == NULL )
	    goto cleanup;
	
	dbstatus = StartIngresServer();

	if (!dbstatus)
		goto cleanup;

	/* 
	** Update status of SCM to SERVICE_RUNNING		
	*/
	if (!ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0, 0))
		goto cleanup;

	/* 
	** Wait for terminating event.				
	*/
	dwWFS = WaitForSingleObject( hServDoneEvent, INFINITE	);

	if ( dwWFS == 0 )
		goto cleanup;

	/* No return is really expected here.	*/

	return;

cleanup:
	if (hServDoneEvent != NULL)
		CloseHandle(hServDoneEvent);
	if (sshStatusHandle != 0)
		(VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
	return;
}


/*
** Name: IngresCtrlHandler
**
** Description:
**     Handles control requests for OpenIngres Service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**
*/

WINAPI 
IngresCtrlHandler(DWORD dwCtrlCode)
{
	DWORD	nret;

	switch(dwCtrlCode) 
	{
	    case SERVICE_CONTROL_PAUSE:
		break;

	    case SERVICE_CONTROL_CONTINUE:
		break;

	    case SERVICE_CONTROL_STOP:
		ReportStatusToSCMgr(
			SERVICE_STOP_PENDING,
			NO_ERROR,
			1,
			100000);
		nret = StopServer();
		SetEvent(hServDoneEvent);
		break;

	    case SERVICE_CONTROL_INTERROGATE:
		break;

	    default:
		break;
	}
	return nret;
}


/*
** Name: StartIngresServer
**
** Description:
**	Runs ingstart.exe which starts all the ingres processes 
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
**	07-24-97 (natjo01)
**		Changed name of key to CA-OpenIngres_Client.
*/

int
StartIngresServer()
{
	LPTSTR	lpszOldValue;
	TCHAR	tchBuf[ _MAX_PATH];
	TCHAR	tchServiceImage[ _MAX_PATH];
	TCHAR	tchCommand[ _MAX_PATH];
	DWORD	nret;
	DWORD	dwType;
	DWORD	cbData = _MAX_PATH;
	DWORD	dwWait;
	HKEY	hKey1;
	HKEY	hKey2;
	SIZE_TYPE     len = 0;
	
	
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.lpReserved = NULL;
	siStartInfo.lpDesktop = NULL;
	siStartInfo.lpTitle = NULL;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.cbReserved2 = 0;
	siStartInfo.dwFlags = 0;
    
	lpszOldValue = ((GetEnvironmentVariable("PATH", tchBuf, _MAX_PATH) > 0)
					 ? tchBuf : NULL);

	nret = RegOpenKeyEx (HKEY_CLASSES_ROOT,
			"Ingres_Client",
			0,
			KEY_QUERY_VALUE,
			&hKey1);

	nret = RegOpenKeyEx ( hKey1,
			"shell",
			0,
			KEY_QUERY_VALUE,
			&hKey2);

	nret = RegQueryValueEx( hKey2,
			"II_SYSTEM",
			NULL,
			&dwType,		
			tchII_SYSTEM,
			&cbData);

	CloseHandle(hKey1);
	CloseHandle(hKey2);
	

	/* 
	** Strip II_SYSTEM of any \'s that may have been tagged onto 
	** the end of it.					     
	*/

	len = strlen (tchII_SYSTEM);

	if (tchII_SYSTEM [len-1] == '\\')
	    tchII_SYSTEM [len-1] = '\0';

	/* 
	** Append ";%II_SYSTEM%\inges\utility" to the path and set the PATH
	** environment variable. This is applicable only to this process.
	*/

	strcat( tchBuf, ";" );
	strcat( tchBuf, tchII_SYSTEM);	
	strcat( tchBuf, "\\ingres\\utility;");
	strcat( tchBuf, tchII_SYSTEM);	
	strcat( tchBuf, "\\ingres\\bin;");

	nret = SetEnvironmentVariable("II_SYSTEM", tchII_SYSTEM);
	nret = SetEnvironmentVariable("PATH", tchBuf);

	strcpy( tchServiceImage, tchII_SYSTEM);
	strcat( tchServiceImage, "\\ingres\\utility\\ingstart.exe");

	strcpy( tchCommand, "ingstart.exe -client" );

	isstarted = CreateProcess(
		tchServiceImage, 
		tchCommand,	
		NULL,		
		NULL,		
		TRUE,		
		HIGH_PRIORITY_CLASS,		
		NULL,	
		NULL,		
		&siStartInfo,
		&piProcInfo);

	dwWait = WaitForSingleObject( piProcInfo.hProcess, INFINITE);

	if (!isstarted)
		goto cleanup;
	
	return isstarted;

cleanup:
	if (hServDoneEvent != NULL)
		CloseHandle(hServDoneEvent);

	if (sshStatusHandle != 0)
		(VOID)ReportStatusToSCMgr(
			SERVICE_STOPPED,
			dwGlobalErr,
			0,
			0);
	return 0;
}



/*
** Name: StopServer
**
** Description:
**      Issue the ingstop command to stop the OpenIngres server.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

BOOL
StopServer()
{
	char	cmdline [256];
	DWORD	dwWait;

	strcpy( cmdline, tchII_SYSTEM);
	strcat( cmdline, "\\ingres\\utility\\ingstop.exe -kill");

	isstarted = CreateProcess(NULL,
				cmdline,
				NULL,
				NULL,
				TRUE,
				HIGH_PRIORITY_CLASS,
				NULL,
				NULL,
				&siStartInfo,
				&piProcInfo2);

	dwWait = WaitForSingleObject( piProcInfo2.hProcess, INFINITE);
	return (isstarted);
}



/*
** Name: ReportIngresEvents
**
** Description:
**      Write events to the event log.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**
**	07-24-97 (natjo01)
**		Changed name of key to CA-OpenIngres_Client.
*/

VOID
ReportIngresEvents(LPTSTR lpszMsg)
{
	HANDLE	hEventSource;
	LPTSTR	lpszStrings[2];
	CHAR	chMsg[256];

	dwGlobalErr = GetLastError();

	hEventSource=RegisterEventSource(NULL,TEXT("Ingres_Client"));

	sprintf(chMsg, "Ingres error: %d %d", dwGlobalErr, dwGlobalStatus);

	lpszStrings[0] = chMsg;
	lpszStrings[1] = lpszMsg;

	if (hEventSource != NULL) 
	{
		ReportEvent(
			hEventSource, 
			EVENTLOG_ERROR_TYPE,
			0,
			0,
			NULL,
			2,
			0,
			lpszStrings,
			NULL);
	}
	return;
}


/*
** Name: StopService
**
** Description:
**      Stop the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
**	07-24-97 (natjo01)
**		Changed key name to CA-OpenIngres_Client.
*/

VOID
StopService(LPTSTR lpszMsg)
{
	HANDLE	hEventSource;
	LPTSTR	lpszStrings[2];

	dwGlobalErr = GetLastError();

	hEventSource=RegisterEventSource( NULL, TEXT("Ingres_Client"));

	sprintf(chMsg, "Ingres error: %d", dwGlobalErr);
	lpszStrings[0] = chMsg;
	lpszStrings[1] = lpszMsg;

	if (hEventSource != NULL) 
	{
		ReportEvent(hEventSource, 
			EVENTLOG_ERROR_TYPE,
			0,
			0,
			NULL,
			2,
			0,
			lpszStrings,
			NULL);
		(VOID) DeregisterEventSource(hEventSource);
	}
	SetEvent(hServDoneEvent);
}


/*
** Name: ReportStatusToSCMgr
**
** Description:
**     Report the status to the service control manager.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

BOOL
ReportStatusToSCMgr(DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwCheckPoint,
		DWORD dwWaitHint)

{
	BOOL fResult;

	if (dwCurrentState == SERVICE_START_PENDING)
		lpssServiceStatus->dwControlsAccepted = 0;
	else
		lpssServiceStatus->dwControlsAccepted = SERVICE_ACCEPT_STOP;

	ssServiceStatus.dwCurrentState  = dwCurrentState;
	ssServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ssServiceStatus.dwCheckPoint    = dwCheckPoint;
	ssServiceStatus.dwWaitHint      = dwWaitHint;

	if (!(fResult = SetServiceStatus( sshStatusHandle, lpssServiceStatus)))
	{
		StopService("SetServiceStatus");
	}
	
	return fResult;
}

/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : regsvice.cpp, Implementation file 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Check the Service (Installation, Running, Set Password)
**
** History:
**
** xx-May-1999 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 04-Feb-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, change nTotalSecods data type
**    to LONGLONG to clean up the warnings.
**/


#include "stdafx.h"
#include "usrexcep.h"
#include <winsvc.h>
#include "regsvice.h"

BOOL IsServiceInstalled (LPCTSTR lpszServiceName)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;

	//
	// Connect to service control manager on the local machine and
	// open the ServicesActive database
	schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (schSCManager == NULL )
	{
		CString strMsg;
		LONG err = GetLastError();
		
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		case ERROR_DATABASE_DOES_NOT_EXIST:
		case ERROR_INVALID_PARAMETER:
			throw CeException (E_SERVICE_CONNECT);
			break;
		}
		return FALSE;
	}

	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	schService = OpenService (schSCManager, lpszServiceName, GENERIC_READ);
	if (schService != NULL)
		return TRUE;
	return FALSE;
}

BOOL IsServiceRunning (LPCTSTR lpszServiceName)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, sizeof(svrstatus), 0);
	//
	// Connect to service control manager on the local machine and 
	// open the ServicesActive database
	schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (schSCManager == NULL )
	{
		CString strMsg;
		LONG err = GetLastError();
		
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		case ERROR_DATABASE_DOES_NOT_EXIST:
		case ERROR_INVALID_PARAMETER:
			throw CeException (E_SERVICE_CONNECT);
			break;
		}
		return FALSE;
	}
	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	schService = OpenService (schSCManager, lpszServiceName, GENERIC_READ);
	if (schService == NULL)
		return FALSE;
	// 
	// Check if the Service 'lpszServiceName' is Running
	if (!QueryServiceStatus (schService, &svrstatus))
	{
		throw CeException (E_SERVICE_STATUS);
		return FALSE;
	}
	if (svrstatus.dwCurrentState == SERVICE_RUNNING)
		return TRUE;
	return FALSE;
}



BOOL SetServicePassword (LPCTSTR lpszServiceName, const CaServiceConfig& cf)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	TCHAR      tchszDisplayName [256];
	DWORD      dwpcchBuffer  = sizeof(tchszDisplayName);

	//
	// Connect to service control manager on the local machine and 
	// open the ServicesActive database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL )
	{
		CString strMsg;
		LONG err = GetLastError();
		
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		case ERROR_DATABASE_DOES_NOT_EXIST:
		case ERROR_INVALID_PARAMETER:
			throw CeException (E_SERVICE_CONNECT);
			break;
		}
		return FALSE;
	}
	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	schService = OpenService (schSCManager, lpszServiceName, SERVICE_ALL_ACCESS);
	if (schService == NULL)
		return FALSE;
#if defined (MAINWIN)
	return FALSE;
#else
	if (!GetServiceDisplayName(schSCManager, lpszServiceName, tchszDisplayName, &dwpcchBuffer))
	{
		throw CeException (E_SERVICE_NAME);
		return FALSE;
	}
#endif
	//
	// Change the service config (password account).
	BOOL bOK = ChangeServiceConfig (
		schService,           // Handle of service
		SERVICE_NO_CHANGE,    // Type of service
		SERVICE_NO_CHANGE,    // When to start service
		SERVICE_NO_CHANGE,    // Severity if service fails to start
		NULL,                 // Address of service binary file name (Not Change)
		NULL,                 // Address of load ordering group name.(Not Change)
		NULL,                 // Pointer to variable to get tag identifier. (Not Change)
		NULL,                 // Address of array of dependency names. (Not Change)
		cf.m_strAccount,      // Address of account name of service
		cf.m_strPassword,     // Address of password for service account
		tchszDisplayName);    // Address of display name
	if (!bOK)
	{
		throw CeException (E_SERVICE_CONFIG);
		return FALSE;
	}
	return TRUE;
}


BOOL RunService (LPCTSTR lpszServiceName, DWORD& dwError)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, sizeof(svrstatus), 0);

	//
	// Connect to service control manager on the local machine and 
	// open the ServicesActive database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL )
	{
		CString strMsg;
		dwError = GetLastError();
		
		switch (dwError)
		{
		case ERROR_ACCESS_DENIED:
		case ERROR_DATABASE_DOES_NOT_EXIST:
		case ERROR_INVALID_PARAMETER:
			throw CeException (E_SERVICE_CONNECT);
			break;
		}
		return FALSE;
	}
	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	schService = OpenService (schSCManager, lpszServiceName, SERVICE_ALL_ACCESS);
	if (schService == NULL)
		return FALSE;

	if (!StartService (schService, 0, NULL))
	{
		dwError = GetLastError();
		return FALSE;
	}

	CTime tStarted = CTime::GetCurrentTime();
	CTime t = CTime::GetCurrentTime();
	CTimeSpan tdiff  = t - tStarted;
	LONGLONG   nTotalSecods = 0;
	// 
	// Check if the Service 'lpszServiceName' is Running
	if (!QueryServiceStatus (schService, &svrstatus))
	{
		throw CeException (E_SERVICE_STATUS);
		dwError = GetLastError();
		return FALSE;
	}
	//
	// Maximum time 20 seconds waiting for the service finishs starting.
	while (svrstatus.dwCurrentState == SERVICE_START_PENDING && nTotalSecods < 20)
	{
		Sleep (200);
		QueryServiceStatus (schService, &svrstatus);
		if (svrstatus.dwCurrentState == SERVICE_RUNNING)
			return TRUE;
		t = CTime::GetCurrentTime();
		tdiff  = t - tStarted;
		nTotalSecods = tdiff.GetTotalSeconds();
	}
	QueryServiceStatus (schService, &svrstatus);
	if (svrstatus.dwCurrentState =! SERVICE_RUNNING)
	{
		dwError = GetLastError();
		return FALSE;
	}
	else
		return TRUE;
}




/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : regsvice.cpp, Implementation file 
** Project  : Ingres II/Vdba.
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Check the Service (Installation, Running, Set Password)
**
**  History:
**	xx-May-1998 (uk$so01)
**	    Created
**	10-Sep-1999 (uk$so01)
**	    Replace include files resmfc.h & mainmfc.h by rcdepend.h for
**	    more reusable code
**	    Replace the VDBA_MfcResourceString by using LoadString to be
**	    independent from VDBA.
**	01-Feb-2000 (noifr01)
**	    (SIR 100238) added the HasAdmistratorRights() function (code
**	    taken from the components.cpp source from front!st_enterprise_inst
**	04-Jul-2000 (uk$so01)
**	    (BUG # 102010) 
**	    Add the CloseServiceHandle to eache successfully opened handle
**	    when it is no used.
**	28-Jul-2001 (schph01)
**	    (BUG # 105359)
**	    Change the type of access in function OpenSCManager() by
**	    GENERIC_READ instead of SC_MANAGER_ALL_ACCESS for the
**	    IsServiceInstalled() and IsServiceRunning() functions.
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**  17-Oct-2002 (noifr01)
**      restored } character that had been lost in previous propagation
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
*/

#include "stdafx.h"
#include <winsvc.h>
#include "rcdepend.h"
#include "regsvice.h"
#include "xdlgacco.h"

BOOL IVM_IsServiceInstalled (LPCTSTR lpszServiceName)
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
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
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
	{
		CloseServiceHandle (schSCManager);
		return FALSE;
	}

	CloseServiceHandle (schService);
	CloseServiceHandle (schSCManager);
	return TRUE;
}

BOOL IVM_IsServiceRunning (LPCTSTR lpszServiceName)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, 0, sizeof(svrstatus));
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
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
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
	{
		CloseServiceHandle (schSCManager);
		return FALSE;
	}
	// 
	// Check if the Service 'lpszServiceName' is Running
	BOOL bRet = TRUE;
	if (!QueryServiceStatus (schService, &svrstatus))
	{
		CString strMsg;
		if (!strMsg.LoadString(IDS_E_SERVICE_STATUS))
			strMsg = _T("Cannot query the service status information");
		AfxMessageBox (strMsg);
		bRet = FALSE;
	}

	CloseServiceHandle (schService);
	CloseServiceHandle (schSCManager);
	if (bRet && svrstatus.dwCurrentState == SERVICE_RUNNING)
		return TRUE;
	return FALSE;
}



BOOL IVM_SetServicePassword (LPCTSTR lpszServiceName, const CaServiceConfig& cf)
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
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
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
	{
		CloseServiceHandle (schSCManager);
		return FALSE;
	}
#if defined (MAINWIN)
	return FALSE;
#else
	if (!GetServiceDisplayName(schSCManager, lpszServiceName, tchszDisplayName, &dwpcchBuffer))
	{
		CString strMsg;
		if (!strMsg.LoadString(IDS_E_GET_SERVICE_NAME))
			strMsg = _T("Cannot get the service's display name");
		AfxMessageBox (strMsg);
		CloseServiceHandle (schSCManager);
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
		CString strMsg;
		if (!strMsg.LoadString(IDS_E_CHANGE_SERVICE_CONF))
			strMsg = _T("Cannot change the service configuration, \nfail to set password");
		AfxMessageBox (strMsg);
		return FALSE;
	}
	return TRUE;
}


BOOL IVM_RunService (LPCTSTR lpszServiceName, DWORD& dwError)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, 0, sizeof(svrstatus));

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
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
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
	{
		CloseServiceHandle (schSCManager);
		return FALSE;
	}

	BOOL bRet = TRUE;
	if (!StartService (schService, 0, NULL))
	{
		dwError = GetLastError();
		bRet = FALSE;
	}

	int   nTotalSecods = 0;
	// 
	// Check if the Service 'lpszServiceName' is Running
	if (bRet && !QueryServiceStatus (schService, &svrstatus))
	{
		CString strMsg;
		if (!strMsg.LoadString(IDS_E_SERVICE_STATUS))
			strMsg = _T("Cannot query the service status information");
		AfxMessageBox (strMsg);
		dwError = GetLastError();
		bRet = FALSE;
	}
	//
	// Maximum time 60 Minutes waiting for the service finishs starting.
	int nMaxWait = 60*60*1000;
	while (bRet && svrstatus.dwCurrentState == SERVICE_START_PENDING && nTotalSecods < nMaxWait)
	{
		Sleep (500);
		QueryServiceStatus (schService, &svrstatus);
		if (svrstatus.dwCurrentState == SERVICE_RUNNING)
		{
			bRet = TRUE;
			break;
		}
		nTotalSecods += 500;
	}

	QueryServiceStatus (schService, &svrstatus);
	if (bRet && svrstatus.dwCurrentState != SERVICE_RUNNING)
	{
		dwError = GetLastError();
		bRet = FALSE;
	}
	else
		bRet = TRUE;

	CloseServiceHandle (schService);
	CloseServiceHandle (schSCManager);
	return bRet;
}



BOOL IVM_FillServiceConfig (CaServiceConfig& cf)
{
	CxDlgAccountPassword dlg;
	if (dlg.DoModal() == IDOK)
	{
		cf.m_strAccount  = dlg.m_strAccount;
		cf.m_strPassword = dlg.m_strPassword;
		return TRUE;
	}
	return FALSE;
}

BOOL IVM_StopService (LPCTSTR lpszServiceName, DWORD& dwError)
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, 0, sizeof(svrstatus));

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
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
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
	{
		CloseServiceHandle (schSCManager);
		return FALSE;
	}

	BOOL bRet = TRUE;
	if (!ControlService (schService, SERVICE_CONTROL_STOP, &svrstatus))
	{
		dwError = GetLastError();
		bRet = FALSE;
	}

	int   nTotalSecods = 0;
	// 
	// Check if the Service 'lpszServiceName' is Running
	if (bRet && !QueryServiceStatus (schService, &svrstatus))
	{
		CString strMsg;
		if (!strMsg.LoadString(IDS_E_SERVICE_STATUS))
			strMsg = _T("Cannot query the service status information");
		AfxMessageBox (strMsg);
		dwError = GetLastError();
		bRet = FALSE;
	}
	//
	// Maximum time 1H Minutes waiting for the service finishs stopping.
	int nMaxWait = 60*60*1000;
	while (bRet && svrstatus.dwCurrentState == SERVICE_STOP_PENDING && nTotalSecods < nMaxWait)
	{
		Sleep (500);
		QueryServiceStatus (schService, &svrstatus);
		if (svrstatus.dwCurrentState == SERVICE_STOPPED)
		{
			bRet = TRUE;
			break;
		}
		nTotalSecods += 500;
	}
	QueryServiceStatus (schService, &svrstatus);
	if (bRet && svrstatus.dwCurrentState != SERVICE_STOPPED)
	{
		dwError = GetLastError();
		bRet = FALSE;
	}

	CloseServiceHandle (schService);
	CloseServiceHandle (schSCManager);
	return bRet;
}


CString IVM_ServiceErrorMessage (DWORD dwError)
{
	CString strMsg;
	strMsg.Format (_T("Service: unknown error = %d !"), dwError);
	switch (dwError)
	{
	case ERROR_DEPENDENT_SERVICES_RUNNING:
		strMsg = _T("A stop control has been sent to a service which other running services are dependent on.");
		break;
	case ERROR_INVALID_SERVICE_CONTROL:
		strMsg = _T("The requested control is not valid for this service");
		break;
	case ERROR_SERVICE_REQUEST_TIMEOUT:
		strMsg = _T("The service did not respond to the start or control request in a timely fashion");
		break;
	case ERROR_SERVICE_NO_THREAD:
		strMsg = _T("A thread could not be created for the service.");
		break;
	case ERROR_SERVICE_DATABASE_LOCKED:
		strMsg = _T("The service database is locked.");
		break;
	case ERROR_SERVICE_ALREADY_RUNNING:
		strMsg = _T("An instance of the service is already running.");
		break;
	case ERROR_INVALID_SERVICE_ACCOUNT:
		strMsg = _T("The account name is invalid or does not exist.");
		break;
	case ERROR_SERVICE_DISABLED:
		strMsg = _T("The specified service is disabled and cannot be started.");
		break;
	case ERROR_CIRCULAR_DEPENDENCY:
		strMsg = _T("Circular service dependency was specified.");
		break;
	case ERROR_SERVICE_DOES_NOT_EXIST:
		strMsg = _T("The specified service does not exist as an installed service.");
		break;
	case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
		strMsg = _T("The service cannot accept control messages at this time.");
		break;
	case ERROR_SERVICE_NOT_ACTIVE:
		strMsg = _T("The service has not been started..");
		break;
	case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
		strMsg = _T("The service process could not connect to the service controller.");
		break;
	case ERROR_EXCEPTION_IN_SERVICE:
		strMsg = _T("An exception occurred in the service when handling the control request.");
		break;
	case ERROR_DATABASE_DOES_NOT_EXIST:
		strMsg = _T("The database specified does not exist.");
		break;
	case ERROR_SERVICE_SPECIFIC_ERROR:
		strMsg = _T("The service has returned a service-specific error code.");
		break;
	case ERROR_PROCESS_ABORTED:
		strMsg = _T("The process terminated unexpectedly.");
		break;
	case ERROR_SERVICE_DEPENDENCY_FAIL:
		strMsg = _T("The dependency service or group failed to start.");
		break;
	case ERROR_SERVICE_LOGON_FAILED:
		strMsg = _T("The service did not start due to a logon failure.");
		break;
	case ERROR_SERVICE_START_HANG:
		strMsg = _T("After starting, the service hung in a start-pending state.");
		break;
	case ERROR_INVALID_SERVICE_LOCK:
		strMsg = _T("The specified service database lock is invalid.");
		break;
	case ERROR_SERVICE_MARKED_FOR_DELETE:
		strMsg = _T("The specified service has been marked for deletion.");
		break;
	case ERROR_SERVICE_EXISTS:
		strMsg = _T("The specified service already exists.");
		break;
	case ERROR_ALREADY_RUNNING_LKG:
		strMsg = _T("The system is currently running with the last-known-good configuration.");
		break;
	case ERROR_SERVICE_DEPENDENCY_DELETED:
		strMsg = _T("The dependency service does not exist or has been marked for deletion.");
		break;
	case ERROR_BOOT_ALREADY_ACCEPTED:
		strMsg = _T("The current boot has already been accepted for use as the last-known-good control set.");
		break;
	case ERROR_SERVICE_NEVER_STARTED:
		strMsg = _T("No attempts to start the service have been made since the last boot.");
		break;
	case ERROR_DUPLICATE_SERVICE_NAME:
		strMsg = _T("The name is already in use as either a service name or a service display name.");
		break;
	case ERROR_DIFFERENT_SERVICE_ACCOUNT:
		strMsg = _T("The account specified for this service is different from the account\nspecified for other services running in the same process. ");
		break;
	default:
		break;
	}
	return strMsg;
}

BOOL IVM_HasAdmistratorRights()
{
#ifdef MAINWIN
	// SHOULD NOT BE CALLED
	return FALSE;
#endif
	OSVERSIONINFO vsinfo;

	vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx (&vsinfo))
		return FALSE;
	if (vsinfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
		return FALSE;

	BOOL fAdmin;
	HANDLE hThread;
	TOKEN_GROUPS *ptg=NULL;
	DWORD cbTokenGroups;
	DWORD dwGroup;
	PSID psidAdmin;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority=SECURITY_NT_AUTHORITY;
	
	if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
	{
		if ( GetLastError() == ERROR_NO_TOKEN)
		{
		/*
		** If the thread does not have an access token, we'll examine the
		** access token associated with the process.
		*/
			if ( !OpenProcessToken( GetCurrentProcess(),TOKEN_QUERY,&hThread) )
				return (FALSE);
		}
		else 
			return (FALSE);
	}
	
	/*
	** Then we must query the size of the group information associated with
	** the token. Note that we expect a FALSE result from GetTokenInformation
	** because we've given it a NULL buffer. On exit cbTokenGroups will tell
	** the size of the group information.
	*/
	
	if ( GetTokenInformation (hThread, TokenGroups, NULL, 0, &cbTokenGroups) )
		return (FALSE);
	
		/*
		** Here we verify that GetTokenInformation failed for lack of a large
		** enough buffer.
		*/
	if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		return (FALSE);
	
		/*
		** Now we allocate a buffer for the group information.
		** Since _alloca allocates on the stack, we don't have
		** to explicitly deallocate it. That happens automatically
		** when we exit this function.
		*/
	if ( !( ptg = (struct _TOKEN_GROUPS *)malloc( cbTokenGroups)) ) 
		return (FALSE);
	
		/*
		** Now we ask for the group information again.
		** This may fail if an administrator has added this account
		** to an additional group between our first call to
		** GetTokenInformation and this one.
		*/
	if ( !GetTokenInformation (hThread, TokenGroups, ptg, cbTokenGroups,
		&cbTokenGroups) )
		return (FALSE);
	
	/* Now we must create a System Identifier for the Admin group. */
	if ( !AllocateAndInitializeSid (&SystemSidAuthority, 2, 
		SECURITY_BUILTIN_DOMAIN_RID, 
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &psidAdmin) )
		return ( FALSE);
	
	/*
	** Finally we'll iterate through the list of groups for this access
	** token looking for a match against the SID we created above.
	*/
	fAdmin= FALSE;
	
	for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
	{
		if ( EqualSid (ptg->Groups[dwGroup].Sid, psidAdmin) )
		{
			fAdmin = TRUE;
			break;
		}
	}
	
	/* Before we exit we must explicity deallocate the SID we created. */
	FreeSid (psidAdmin);
	return (fAdmin);

}

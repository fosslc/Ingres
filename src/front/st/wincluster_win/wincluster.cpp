/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: wincluster.cpp - Windows cluster High Availability Option setup wizard.
**
** Description:
**	Defines the class behaviors for the Ingres Windows cluster setup application,
**	aka, High Availability Option for Windows. Specifically, it:
**		- Initializes the application
**		- Ensures only one instance is running
**		- Creates and executes the property sheet that implements the setup wizard.
**	In addition, it contains other functions, called elsewhere in the wizard, to:
**		- Set up the High Availability Option
**		- Perform other ancillary functions.
**
**  The following steps are performed to set up the High Availability Option 
**  for Windows Clustering:
**  
**  1. Create the Ingres Service via the Service Control Manager.
**  2. Register Ingres with the Windows Cluster service.
**  3. Modify the Ingres config.dat to use the Virtual Cluster name instead of 
**     machine name.
**  4. Modify the Ingres "ingsetenv" variables to use the Virtual Cluster name.
**  5. Modify the "IIxxxx_MACHINENAME" files in ingres\files\name.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
**	22-jun-2004 (wonst02)
**		Remove all "Advantage" qualifiers and use "Ingres" alone.
** 	19-jul-2004 (wonst02)
** 		Create the Ingres Service via the Service Control Manager.
** 	03-aug-2004 (wonst02)
** 		Add comments.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove the HELP command.
**	02-dec-2004 (wonst02) Bug 113522
**	    Add Ingres service with current userid and password.
*/

#include "stdafx.h"
#include <winsvc.h>
#include <security.h>			// For GetUserNameEx()
#include "wincluster.h"
#include "configdat.h"
#include "setenv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HPALETTE hSystemPalette=0;

static BOOL IsAdmin(void);
static BOOL FileExists(LPCSTR s);

/*
**  Description:
** 	Display message in a pop-up message box.
** 
**	History:
*/
static UINT MyMessageBox(UINT uiStrID,UINT uiFlags,LPCSTR param=0)
{
	UINT ret;
	CString title;
	CString text;
	
	title.LoadString(IDS_TITLE);
	if (param)
		text.Format(uiStrID,param);
	else
		text.LoadString(uiStrID);

	HWND hwnd=property ? property->m_hWnd : 0;
	if(!hwnd)
	 uiFlags|=MB_APPLMODAL;

	ret=::MessageBox(hwnd, text,title,uiFlags);
	return ret;
}

void MyError(UINT uiStrID,LPCSTR param)
{
	MyMessageBox(uiStrID,MB_OK|MB_ICONEXCLAMATION,param);
}

// Display a system error message in a message box
void SysError(
		UINT stringId,		// A string identifier from the local resource file
		LPCSTR insertChr,	// A string to be inserted into the message text
		DWORD lastError)	// A system error code to be looked up and inserted
{
	CString	Message, Title;
	CHAR	outmsg[512];

	Message.LoadString(stringId);
	Title.LoadString(IDS_TITLE);
	LPVOID lpMsgBuf = NULL;
	if (lastError)
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL );
	if (insertChr && lastError)
		sprintf(outmsg, Message, insertChr, (LPCTSTR)lpMsgBuf);
	else if (insertChr)
		sprintf(outmsg, Message, insertChr, "");
	else if (lastError)
		sprintf(outmsg, Message, (LPCTSTR)lpMsgBuf, "");
	else
		sprintf(outmsg, Message, "", "");
	
	HWND hwnd=property ? property->m_hWnd : 0;

	MessageBox(hwnd, outmsg, Title, MB_ICONEXCLAMATION | MB_OK);

	// Free the buffer.
	if (lpMsgBuf)
		LocalFree( lpMsgBuf );
}

BOOL AskUserYN(UINT uiStrID,LPCSTR param)
{
	return (MyMessageBox(uiStrID,MB_YESNO|MB_ICONQUESTION,param)==IDYES);
}

static BOOL IsGoodOSVersionAndRights()
{
	OSVERSIONINFOEX osver; 
	
	memset((char *) &osver,0,sizeof(osver)); 
	osver.dwOSVersionInfoSize=sizeof(osver); 
	GetVersionEx((LPOSVERSIONINFO) &osver);
	
	if(	(osver.dwPlatformId==VER_PLATFORM_WIN32_NT && osver.dwMajorVersion >= 5) &&
		(osver.dwMinorVersion == 0 /*Win 2000*/ || osver.dwMinorVersion == 2 /*2003 Server*/) &&
		(osver.wSuiteMask & VER_SUITE_ENTERPRISE) )
	{
		// Windows 2000 Advanced Server or Windows Server 2003, Enterprise Edition is installed.
		return TRUE;
	}
	else
	{
		MyError(IDS_SUPPORTVERSION);
		return FALSE;
	}
}

static BOOL IsAdmin()
{
    /* Determine whether the current process is running under a 
       local administrator account. */

    HANDLE hAccessToken;
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize;
    BOOL bSuccess;
    SID_IDENTIFIER_AUTHORITY sidNtAuthority=SECURITY_NT_AUTHORITY;
    PSID psidAdministrators;
    PTOKEN_GROUPS ptgGroups=(PTOKEN_GROUPS)InfoBuffer;
    UINT x;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken))
		return FALSE;
    bSuccess=GetTokenInformation(hAccessToken, TokenGroups, InfoBuffer, 1024, &dwInfoBufferSize);
    CloseHandle(hAccessToken);
    if (!bSuccess)
		return FALSE;
    if (!AllocateAndInitializeSid(&sidNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators))
    {
		return FALSE;
    }
    
    bSuccess=FALSE;
    for (x=0; x<ptgGroups->GroupCount; x++)
    {
		if (EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid))
		{
			bSuccess=TRUE;
			break;
		}
    }
    FreeSid(psidAdministrators);
    return bSuccess;
}


static BOOL AlreadyRunning()
{
	CString s; 
	
	s.LoadString(IDS_TITLE);
	if (FindWindow(NULL,s)!=0)
	{
		MyError(IDS_ALREADYRUNNING);
		return TRUE;
	}
	return FALSE;
}

static BOOL
FileExists(LPCSTR s)
{
    HANDLE File = CreateFile(s,0,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);

    if (File == INVALID_HANDLE_VALUE)
	return FALSE;

    CloseHandle(File);
    return TRUE;
}

//
//	Create the Ingres Service via the Service Control Manager.
//
BOOL CreateIngSvc()
{
	CHAR	lpServiceName[] = "Ingres_Database_II";
	CHAR	lpDisplayName[] = "Ingres Intelligent Database [II]";
	CHAR	lpBinaryPathName[1024] = "";
	ULONG	nSize = 512;
	CString	csNameBuffer(" ", nSize);
	LPTSTR	lpServiceStartName = csNameBuffer.GetBuffer();
	BOOLEAN	bRet = GetUserNameEx( NameSamCompatible, lpServiceStartName, &nSize );

	if (!bRet && (GetLastError() == ERROR_MORE_DATA))
	{
		csNameBuffer.ReleaseBufferSetLength( nSize );
		lpServiceStartName = csNameBuffer.GetBuffer();
		bRet = GetUserNameEx( NameSamCompatible, lpServiceStartName, &nSize );
	}
	if (!bRet)
	{
		SysError( IDS_GETUSERNAME_ERR, NULL, GetLastError() );
		return FALSE;
	}

	SC_HANDLE hSCManager = OpenSCManager(
		NULL,	// this machine
		NULL,	// the SERVICES_ACTIVE_DATABASE 
		SC_MANAGER_CREATE_SERVICE );
	if (! hSCManager)
	{
		DWORD dErr = GetLastError();
		switch (dErr)
		{
			case IDS_ERROR_SCM_ACCESS_DENIED:
				SysError( IDS_ERROR_SCM_ACCESS_DENIED, lpServiceName, dErr );
				break;
			default:
				SysError( IDS_ERROR_SCM, lpServiceName, dErr );
				break;
		}

		return FALSE;
	}

	// Substitute the actual 2-letter install code into the names.
	strncpy(lpServiceName + sizeof(lpServiceName) - 3, thePreSetup.m_InstallCode.GetBuffer(2), 2);
	strncpy(lpDisplayName + sizeof(lpDisplayName) - 4, thePreSetup.m_InstallCode.GetBuffer(2), 2);
	sprintf(lpBinaryPathName, "\"%s\\ingres\\bin\\servproc.exe\"", thePreSetup.m_II_System.GetBuffer());

	SC_HANDLE hIngSvc = CreateService(
		hSCManager,
		lpServiceName,
		lpDisplayName,
		SC_MANAGER_CREATE_SERVICE,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		lpBinaryPathName,
		NULL,	// does not belong to a group
		NULL,	// no tag
		NULL,	// no dependencies
		lpServiceStartName,
		thePreSetup.m_ServicePassword.GetBuffer() );

	BOOL bReturn = TRUE;
	if (hIngSvc)
	{
		CloseServiceHandle(hIngSvc);
	}
	else
	{
		DWORD dErr = GetLastError();
		switch (dErr)
		{
			case ERROR_SERVICE_EXISTS:
				break;
			case ERROR_ACCESS_DENIED:
				SysError( IDS_ERROR_ACCESS_DENIED, lpServiceName, dErr );
				bReturn = FALSE;
				break;
			case ERROR_DUPLICATE_SERVICE_NAME:
				SysError( IDS_ERROR_DUPLICATE_SERVICE_NAME, lpDisplayName, dErr );
				bReturn = FALSE;
				break;
			case ERROR_INVALID_NAME:
				SysError( IDS_ERROR_INVALID_NAME, lpServiceName, dErr );
				bReturn = FALSE;
				break;
			case ERROR_INVALID_SERVICE_ACCOUNT:
				SysError( IDS_ERROR_INVALID_SERVICE_ACCOUNT, lpServiceStartName, dErr );
				bReturn = FALSE;
				break;
			default:
				SysError( IDS_SERVICEERROR, lpServiceName, dErr );
				bReturn = FALSE;
				break;
		}
	}

	CloseServiceHandle(hSCManager);
	return bReturn;
}

//
// Create a property list for the cluster resource
//
BOOL DefineProperties(HRESOURCE hResource, LPTSTR lpszResourceName)
{
	DWORD dRet = ERROR_SUCCESS;

	// Define the service properties:
	//	cluster resource "Ingres Service" /properties PendingTimeout=300000:dword

	WCHAR szPendingTimeout[] = L"PendingTimeout"; 

	struct _INGRES_PROPERTIES
	{
		DWORD nPropertyCount;
		// PendingTimeout=300000:dword
		CLUSPROP_PROPERTY_NAME_DECLARE(PendingTimeout, sizeof(szPendingTimeout) / sizeof(WCHAR));
		CLUSPROP_DWORD PendingTimeoutValue;
		CLUSPROP_SYNTAX EndPendingTimeout;
	} PropList;

	PropList.nPropertyCount = 1;
	// PendingTimeout=300000:dword
	PropList.PendingTimeout.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.PendingTimeout.cbLength = sizeof(szPendingTimeout);
	lstrcpyW(PropList.PendingTimeout.sz, szPendingTimeout);
	PropList.PendingTimeoutValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_DWORD;
	PropList.PendingTimeoutValue.cbLength = sizeof(DWORD);
	PropList.PendingTimeoutValue.dw = 300000;
	PropList.EndPendingTimeout.dw = CLUSPROP_SYNTAX_ENDMARK;

	LPVOID	lpInBuffer = &PropList;
	DWORD	cbInBufferSize = sizeof(PropList);
	
	dRet = ClusterResourceControl( 
			hResource,                               // resource handle          
			NULL,                                    // optional host node          
			CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES,  // this control code          
			lpInBuffer,                              // input buffer: property list          
			cbInBufferSize,                          // allocated buffer size (bytes)          
			NULL,                                    // output buffer: (not used)          
			0,                                       // output buffer size (not used)          
			NULL );                                  // actual size of resulting data (not used)          

	if (! (dRet == ERROR_SUCCESS || dRet == ERROR_RESOURCE_PROPERTIES_STORED) )
	{
		SysError( IDS_RESOURCEPROPERTY_ERR, lpszResourceName, dRet );
		return FALSE;
	}

	return TRUE;
}

//
// Create a "private" property list for the cluster resource
//
BOOL DefineServiceParameters(HRESOURCE hResource, LPTSTR lpszResourceName)
{
	USES_CONVERSION;
	DWORD dRet = ERROR_SUCCESS;
	// cluster resource "Ingres Service" /privproperties 
	//	ServiceName="Ingres_Database_II":string
	//	StartupParameters="HAScluster":string 
	//	UseNetworkName=1:dword

	WCHAR szServiceName[] = L"ServiceName";
	WCHAR szServiceNameValue[] = L"Ingres_Database_II";
	WCHAR szStartupParameters[] = L"StartupParameters";
	WCHAR szStartupParametersValue[] = L"HAScluster";
	WCHAR szUseNetworkName[] = L"UseNetworkName";

	struct _INGRES_PROPERTIES
	{
		DWORD nPropertyCount;
		// ServiceName="Ingres_Database_II":string
        CLUSPROP_PROPERTY_NAME_DECLARE(ServiceName, sizeof(szServiceName) / sizeof(WCHAR));
		CLUSPROP_SZ_DECLARE(ServiceNameValue, sizeof(szServiceNameValue) / sizeof(WCHAR));
		CLUSPROP_SYNTAX EndServiceName;
		// StartupParameters="HAScluster":string 
        CLUSPROP_PROPERTY_NAME_DECLARE(StartupParameters, sizeof(szStartupParameters) / sizeof(WCHAR));
		CLUSPROP_SZ_DECLARE(StartupParametersValue, sizeof(szStartupParametersValue) / sizeof(WCHAR));
		CLUSPROP_SYNTAX EndStartupParameters;
		// UseNetworkName=1:dword
		CLUSPROP_PROPERTY_NAME_DECLARE(UseNetworkName, sizeof(szUseNetworkName) / sizeof(WCHAR));
		CLUSPROP_DWORD UseNetworkNameValue;
		CLUSPROP_SYNTAX EndUseNetworkName;
	} PropList;

	PropList.nPropertyCount = 3;
	// ServiceName="Ingres_Database_II":string
	lstrcpyW(szServiceNameValue, L"Ingres_Database_");
	wcscat(szServiceNameValue, T2CW( thePreSetup.m_InstallCode.GetBuffer() ));

	PropList.ServiceName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.ServiceName.cbLength = sizeof(szServiceName);
	lstrcpyW(PropList.ServiceName.sz, szServiceName);
	PropList.ServiceNameValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
	PropList.ServiceNameValue.cbLength = sizeof(szServiceNameValue);
	lstrcpyW(PropList.ServiceNameValue.sz, szServiceNameValue);
	PropList.EndServiceName.dw = CLUSPROP_SYNTAX_ENDMARK;
	// StartupParameters="HAScluster":string 
	PropList.StartupParameters.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.StartupParameters.cbLength = sizeof(szStartupParameters);
	lstrcpyW(PropList.StartupParameters.sz, szStartupParameters);
	PropList.StartupParametersValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
	PropList.StartupParametersValue.cbLength = sizeof(szStartupParametersValue);
	lstrcpyW(PropList.StartupParametersValue.sz, szStartupParametersValue);
	PropList.EndStartupParameters.dw = CLUSPROP_SYNTAX_ENDMARK;
	// UseNetworkName=1:dword
	PropList.UseNetworkName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.UseNetworkName.cbLength = sizeof(szUseNetworkName);
	lstrcpyW(PropList.UseNetworkName.sz, szUseNetworkName);
	PropList.UseNetworkNameValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_DWORD;
	PropList.UseNetworkNameValue.cbLength = sizeof(DWORD);
	PropList.UseNetworkNameValue.dw = 1;
	PropList.EndUseNetworkName.dw = CLUSPROP_SYNTAX_ENDMARK;

	LPVOID	lpInBuffer = &PropList;
	DWORD	cbInBufferSize = sizeof(PropList);
	
	dRet = ClusterResourceControl( 
			hResource,                               // resource handle          
			NULL,                                    // optional host node          
			CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, // this control code          
			lpInBuffer,                              // input buffer: property list          
			cbInBufferSize,                          // allocated buffer size (bytes)          
			NULL,                                    // output buffer: (not used)          
			0,                                       // output buffer size (not used)          
			NULL );                                  // actual size of resulting data (not used)          

	if (! (dRet == ERROR_SUCCESS || dRet == ERROR_RESOURCE_PROPERTIES_STORED) )
	{
		if (dRet == ERROR_DEPENDENCY_NOT_FOUND)
			SysError( IDS_RESOURCEDEPENDENCYINVALID_ERR, lpszResourceName, dRet );
		else
			SysError( IDS_RESOURCEPRIVPROPERTY_ERR, lpszResourceName, dRet );
		return FALSE;
	}

	return TRUE;
}

//
// Define the Registry Replication keys
//
BOOL DefineRegistryKeys(HRESOURCE hResource, LPTSTR lpszResourceName)
{
	USES_CONVERSION;
	DWORD dRet = ERROR_SUCCESS;
    char  SubKey[128];

	//
	// Define the Registry Replication keys
	// 

	struct {
		char *szRegistryKey;
	} RegistryKeys[] = {
		"SOFTWARE\\Classes\\Ingres_Database_%s",
		"SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation"
	};
	WCHAR szRegistryKey[128];
	LPVOID lpInBuffer = NULL;
	DWORD cbInBufferSize = 0;


	for (int i=0; i<sizeof(RegistryKeys)/sizeof(RegistryKeys[0]); i++)
	{
		sprintf(SubKey, RegistryKeys[i].szRegistryKey, 
				thePreSetup.m_InstallCode.GetBuffer());
		lstrcpyW(szRegistryKey, T2CW(SubKey));
		lpInBuffer = (LPVOID)szRegistryKey;
		cbInBufferSize = ( wcslen(szRegistryKey) + 1 ) * sizeof(WCHAR);
		dRet = ClusterResourceControl( 
				hResource,                               // resource handle          
				NULL,                                    // optional host node          
				CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,// this control code          
				lpInBuffer,                              // input buffer: property list          
				cbInBufferSize,                          // allocated buffer size (bytes)          
				NULL,                                    // output buffer: (not used)          
				0,                                       // output buffer size (not used)          
				NULL );                                  // actual size of resulting data (not used)          

		if (! (dRet == ERROR_SUCCESS || dRet == ERROR_ALREADY_EXISTS) )
		{
			SysError( IDS_RESOURCEREGISTRY_ERR, lpszResourceName, dRet );
			return FALSE;
		}
	}

	return TRUE;
}


// Register Ingres with the Windows Cluster service.
BOOL RegisterClusterService()
{
	USES_CONVERSION;

	BOOL bResult = FALSE;

	if (thePreSetup.m_hCluster == NULL)
		return FALSE;

	HGROUP hGroup = NULL;
	HRESOURCE hResource = NULL;
	HRESOURCE hDependsOn = NULL;

	//
    // Define the Ingres cluster group to the Windows Cluster service.
	//
	LPTSTR lpszGroupName = thePreSetup.m_GroupName.GetBuffer();
	LPCWSTR lpwszGroupName = T2CW(lpszGroupName);
	hGroup = OpenClusterGroup(thePreSetup.m_hCluster, lpwszGroupName);
	if (hGroup == NULL)
	{
		hGroup = CreateClusterGroup(thePreSetup.m_hCluster, lpwszGroupName);
		if (hGroup == NULL)
		{
			SysError(IDS_CREATECLUSTER_ERR, lpszGroupName, GetLastError());
			goto endf;
		}
	}

	//
	// Define the Ingres resource to the Windows Cluster service.
	//
	LPTSTR lpszResourceName = thePreSetup.m_ResourceName.GetBuffer();
	LPCWSTR lpwszResourceName = T2CW(lpszResourceName);
	hResource = OpenClusterResource(thePreSetup.m_hCluster, lpwszResourceName);
	if (hResource == NULL)
	{
		hResource = CreateClusterResource( hGroup, lpwszResourceName, 
				L"Generic Service", 0 );
		if (hResource == NULL)
		{
			SysError(IDS_CREATERESOURCE_ERR, lpszResourceName, GetLastError());
			goto endf;
		}
	}

	WcClusterResource *resource = NULL;
	LPTSTR lpszDependsOn = NULL;
	LPCWSTR lpwszDependsOn = NULL;
	DWORD dRet = ERROR_SUCCESS;

	//
	// Define the Ingres resource dependencies to the Windows Cluster service.
	//
	for (int i=0; i<thePreSetup.m_Dependencies.GetCount(); i++)
	{
		resource=(WcClusterResource *) thePreSetup.m_Dependencies.GetAt(i);
		lpszDependsOn = resource->m_name.GetBuffer();
		lpwszDependsOn = T2CW(lpszDependsOn);
		hDependsOn = OpenClusterResource(thePreSetup.m_hCluster, lpwszDependsOn);
		if (hDependsOn == NULL)
		{
			SysError( IDS_OPENRESOURCE_ERR, lpszDependsOn, GetLastError() );
			goto endf;
		}
		dRet = AddClusterResourceDependency(hResource, hDependsOn);
		if (dRet == ERROR_INVALID_PARAMETER)
		{
			SysError( IDS_RESOURCEDEPENDENCYINVALID_ERR, lpszDependsOn, dRet );
			goto endf;
		}
        else if (! (dRet == ERROR_SUCCESS || dRet == ERROR_DEPENDENCY_ALREADY_EXISTS) )
		{
			SysError( IDS_RESOURCEDEPENDENCY_ERR, lpszDependsOn, dRet );
			goto endf;
		}
		CloseClusterResource(hDependsOn);
		hDependsOn = NULL;
	}

	//
	// Define the service properties:
	//	cluster resource "Ingres Service" /properties PendingTimeout=300000:dword
	//
	if ( !DefineProperties(hResource, lpszResourceName) )
		goto endf;
    
	//
	// Define the Ingres service name, Ingres_Database_II, and startup parameter.
	//
	if ( !DefineServiceParameters(hResource, lpszResourceName) )
		goto endf;
    
	//
	// Define the Registry Replication keys
	//
	if ( !DefineRegistryKeys(hResource, lpszResourceName) )
		goto endf;

	// If all went well, return TRUE:
	bResult = TRUE;

endf:

	if (hDependsOn)
		CloseClusterResource(hDependsOn);
	if (hResource)
		CloseClusterResource(hResource);
	return bResult;
}

// Modify the Ingres config.dat to use the virtual Cluster name instead of machine name.
//	ii.*.config.server_host:                <clustername>
//	ii.<clustername>.gcn.local_vnode:        <clustername>
//	ii.<clustername>.icesvr.*.dictionary_node: <clustername>
BOOL ModifyConfigDat()
{
	CString szPattern, szFrom, szTo, s;
    TCHAR szName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD nSize=sizeof(szName);
	WcConfigDat config;
	if (! config.Init())
		return false;

	BOOL bRet = TRUE;

	GetComputerName(szName, &nSize);

	szPattern.Format( "\\.%s\\.", (LPCSTR)szName );
	szPattern.MakeLower();
	szFrom.Format( ".%s.", (LPCSTR)szName );
	szFrom.MakeLower();
	szTo.Format( ".%s.", (LPCSTR)thePreSetup.m_ClusterName );
	szTo.MakeLower();
	bRet = config.ScanResources( szPattern.GetBuffer(), szFrom, szTo );

	if (bRet)
	{
		bRet = config.SetResource( "ii.*.config.server_host", thePreSetup.m_ClusterName.GetBuffer() );
	}

	if (bRet)
	{
		s.Format( "ii.%s.gcn.local_vnode", (LPCSTR)thePreSetup.m_ClusterName );
		s.MakeLower();
		bRet = config.SetResource( s.GetBuffer(), thePreSetup.m_ClusterName.GetBuffer() );
	}

	config.Term();
	return bRet;
}

// Modify the Ingres "ingsetenv" variables to use the Virtual Cluster name.
// 	II_GCNPP_LCL_VNODE=CLUSTER
// 	II_HOSTNAME=CLUSTER
BOOL ModifyVariables()
{
	BOOL bRet=TRUE;

	WcSetEnv setenv;
	
	CString sCmd, sArgs;

	bRet = setenv.Set("II_GCNPP_LCL_VNODE", thePreSetup.m_ClusterName.GetBuffer());
	   
	if (bRet)
	{
		bRet = setenv.Set("II_HOSTNAME", thePreSetup.m_ClusterName.GetBuffer());
	}   
	           	   
	return bRet;
}

// Rename the "IIxxxx_MACHINENAME" files in ingres\files\name.
//	IIATTR_      IIIUSVR_     IIRMCMD_
//	IICOMSVR_    IILOGIN_     IIRTICKET_
//	IIDASVR_     IILTICKET_   IISERVERS_
//	IIINGRES_    IINODE_      
BOOL ModifyFiles()
{
	CString sOldName, sNewName;
    char szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD nSize=sizeof(szComputerName);
	struct {
		char *szName;
	} File[] = {"IIATTR_", "IICOMSVR_", "IIDASVR_", "IIINGRES_",
		"IIIUSVR_", "IILOGIN_", "IILTICKET_", "IINODE_", 
		"IIRMCMD_", "IIRTICKET_", "IISERVERS_"};
	
	GetComputerName(szComputerName, &nSize);

	for (int i=0; i<sizeof(File)/sizeof(File[0]); i++)
	{
		sOldName.Format( "%s\\ingres\\files\\name\\%s%s", thePreSetup.m_II_System.GetBuffer(), 
			(LPCSTR)File[i].szName, (LPCSTR)szComputerName );
		sNewName.Format( "%s\\ingres\\files\\name\\%s%s", thePreSetup.m_II_System.GetBuffer(), 
			(LPCSTR)File[i].szName, (LPCSTR)thePreSetup.m_ClusterName );
		
		if (! MoveFile( sOldName.GetBuffer(), sNewName.GetBuffer() ))
		{
			DWORD err = GetLastError();
			if (err != ERROR_FILE_NOT_FOUND &&	// File may already have been renamed
				err != ERROR_ALREADY_EXISTS)	// New file already exists
			{
				SysError( IDS_RENAMEERROR, (LPCSTR)sOldName, err );
				return FALSE;
			}
		}
	}

	return TRUE;
}


//
//  Set up the High Availability Option on this cluster node.
//
//	The following steps are performed to set up the High Availability Option 
// 	for Windows Clustering:
//	1. Create the Ingres Service via the Service Control Manager.
//	2. Register Ingres with the Windows Cluster service.
//	3. Modify the Ingres config.dat to use the Virtual Cluster name instead of 
// 	   machine name.
//	4. Modify the Ingres "ingsetenv" variables to use the Virtual Cluster name.
//	5. Modify the "IIxxxx_MACHINENAME" files in ingres\files\name.
//
BOOL SetupHighAvailability()
{
//	1. Create the Ingres Service via the Service Control Manager.
	if (! CreateIngSvc())
		return FALSE;

//	2. Register Ingres with the Windows Cluster service.
	if (! RegisterClusterService())
		return FALSE;

//	3. Modify the Ingres config.dat to use the Virtual Cluster name instead of machine name.
	if (! ModifyConfigDat())
		return FALSE;

//	4. Modify the Ingres "ingsetenv" variables to use the Virtual Cluster name.
	if (! ModifyVariables())
		return FALSE;

//	5. Modify the "xxx_MACHINENAME" files in ingres\files\name.
	if (! ModifyFiles())
		return FALSE;

    return TRUE;
}

/*
**	Name: Local_NMgtIngAt
**
**	Description:
**	A revised version of NMgtIngAt independent of Ingres CL functions, 
**	used to retrieve Ingres "ingsetenv" values from symbol.tbl.
**
*/
BOOL 
Local_NMgtIngAt(CString strKey, CString ii_system, CString &strRetValue)
{
	char szFileName[MAX_PATH+1];
	CStdioFile theInputFile;
	int iPos;
	CString strInputString;

	if(strKey.IsEmpty() || ii_system.IsEmpty())
		return FALSE;

	sprintf(szFileName, "%s\\ingres\\files\\symbol.tbl", ii_system.GetBuffer());
	if(!theInputFile.Open(szFileName, CFile::modeRead | CFile::typeText, 0))
		return FALSE;
	while(theInputFile.ReadString(strInputString) != FALSE)
	{
		strInputString.TrimRight();
		iPos=strInputString.Find(strKey);
		if(iPos>=0)
		{
			if( (strInputString.GetAt(iPos+strKey.GetLength()) == '\t') || 
				(strInputString.GetAt(iPos+strKey.GetLength()) == ' ') )
			{ 
				strInputString = strInputString.Right(strInputString.GetLength()-strKey.GetLength());
				strInputString.TrimLeft();
				strRetValue=strInputString;
				theInputFile.Close();
				return TRUE;
			}
		}
	}
	theInputFile.Close();
	return FALSE;
}

/*
**	Name: Local_PMget
**
**	Description:
**	A revised version of PMget independent of Ingres CL functions, 
**	used to retrieve Ingres resource values from config.dat.
**
*/
BOOL 
Local_PMget(CString strKey, CString ii_system, CString &strRetValue)
{
	char szFileName[MAX_PATH+1];
	CStdioFile theInputFile;
	int iPos;
	CString strInputString;

	if(strKey.IsEmpty() || ii_system.IsEmpty())
		return FALSE;

	sprintf(szFileName, "%s\\ingres\\files\\config.dat", ii_system.GetBuffer((ii_system.GetLength()+1)));
	if(!theInputFile.Open(szFileName, CFile::modeRead | CFile::typeText, 0))
		return FALSE;
	while(theInputFile.ReadString(strInputString) != FALSE)
	{
		strInputString.TrimRight();
		iPos=strInputString.Find(strKey);
		if(iPos>=0)
		{
			if( (strInputString.GetAt(iPos+strKey.GetLength()) == ':') || 
				(strInputString.GetAt(iPos+strKey.GetLength()) == ' ') )
			{ 
				strInputString = strInputString.Right(strInputString.GetLength()-strKey.GetLength()-1);
				strInputString.TrimLeft();
				strRetValue=strInputString;
				theInputFile.Close();
				return TRUE;
			}
		}
	}
	theInputFile.Close();
	return FALSE;
}

/*
**  Name: IngresAlreadyRunning
**
**  Description:
**	Check if Ingres identified by II_SYSTEM is already running.
**
**  Returns:
**	-1	config.dat doesn't exist
**		(Ingres identified by II_SYSTEM is not installed)
**	 0	config.dat exists but Ingres not running
**		(Ingres indentified by II_SYSTEM is installed but not running)
**	 1	iigcn is running
**		(Ingres indentified by II_SYSTEM is installed and running)
**
**  History:	
*/
int
IngresAlreadyRunning()
{
    CString	csIigcn, csTemp, csConfig;
    char	SubKey[128];
    HKEY	hkSubKey;
    DWORD	type, size;

    sprintf(SubKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation",
	    thePreSetup.m_InstallCode);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_QUERY_VALUE,
		     &hkSubKey) == ERROR_SUCCESS)
    {
		size = sizeof(ii_installpath);
		RegQueryValueEx(hkSubKey, "II_SYSTEM", 0, &type,
				(BYTE *)ii_installpath, &size);
		RegCloseKey(hkSubKey);
    }
    else
		return -1;

    csConfig.Format("%s\\ingres\\files\\config.dat", ii_installpath);
    if (GetFileAttributes(csConfig) != -1)
    {
		/*
		** We have an installation. Now we have to see if it is running...
		*/
		csIigcn.Format("%s\\ingres\\bin\\iigcn.exe", ii_installpath);
		if (GetFileAttributes(csIigcn) != -1)
		{
			/*
			** We have to use a hack to see if the installation is
			** running. This is by trying to delete the iigcn.exe
			** executable. We must use the hack so that we do not
			** have to link with Ingres libraries (which then
			** causes the install.exe to use the Ingres DLLs) to
			** utilize ping_iigcn().
			*/
			csTemp.Format("%s\\ingres\\bin\\temp.exe", ii_installpath);
			CopyFile(csIigcn, csTemp, FALSE);
			if (DeleteFile(csIigcn))
			{
			/* iigcn is not running */
			CopyFile(csTemp, csIigcn, FALSE);
			DeleteFile(csTemp);
			return 0;
			}
			else
			{
			/* iigcn is running */
			DeleteFile(csTemp);
			return 1;
			}
		}
		else
		{
			/*
			** iigcn.exe does not exist. Therefore, the installation
			** cannot possibly be running.
			*/
			return 0;
		}
    }
    else
		return 0;
}

/*
**	Name: WinstartRunning
**
**	Description:
**	Check if Winstart is running.
**
**	History:	
*/
BOOL
WinstartRunning()
{
	CString title;
	title.Format("Ingres - Service Manager [%s]", thePreSetup.m_InstallCode);
	if(FindWindow(NULL, title) != 0)
		return TRUE;

	title=CString("Ingres - Service Manager");
	if(FindWindow(NULL, title) != 0)
		return TRUE;

	title=CString("Ingres - IngStart");
	if(FindWindow(NULL, title) != 0)
		return TRUE;

	return FALSE;
}

/*
**  Name: ExecuteEx
**
**  Description:
**	Create a new process to execute an executable file.
**
**  History:
*/
BOOL 
ExecuteEx(LPCSTR lpCmdLine, BOOL bWait/*=TRUE*/, BOOL bWindow/*=FALSE*/)
{
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;
    DWORD dwCreationFlags=CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS;

    memset((char*)&pi, 0, sizeof(pi)); 
    memset((char*)&si, 0, sizeof(si)); 
    si.cb = sizeof(si);

    if (bWindow)
    {
	si.dwFlags=STARTF_USESHOWWINDOW;
	si.wShowWindow=SW_MINIMIZE;
	dwCreationFlags=NORMAL_PRIORITY_CLASS;
    }

    if (CreateProcess(NULL, (LPSTR)lpCmdLine, NULL, NULL, TRUE, 
                      dwCreationFlags, NULL, NULL, &si, &pi))
    {
	if (bWait)
	{
	    DWORD   dw;

	    WaitForSingleObject(pi.hProcess, INFINITE);
	    if (GetExitCodeProcess(pi.hProcess, &dw))
		return (dw == 0);
	}
	else
	    return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// WcWinClusterApp

BEGIN_MESSAGE_MAP(WcWinClusterApp, CWinApp)
//{{AFX_MSG_MAP(WcWinClusterApp)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WcWinClusterApp construction

WcWinClusterApp::WcWinClusterApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only WcWinClusterApp object

WcWinClusterApp theApp;
WcPreSetup thePreSetup;
HICON theIcon=0;
WcPropSheet *property=0;
char ii_installpath[MAX_PATH+1];

/////////////////////////////////////////////////////////////////////////////
// WcWinClusterApp initialization

/*
**	
*/
BOOL WcWinClusterApp::InitInstance()
{
	// Standard initialization
	hSystemPalette=0;
	theIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	if( !IsGoodOSVersionAndRights() )
	{
		return FALSE;
	}
	if ( !IsAdmin() )
	{
		MyError(IDS_NOT_ADMIN);
		return FALSE;
	}
	if(m_lpCmdLine)
	{
		CString strBuffer;
		int iPos;
		char ach[MAX_PATH];

		strBuffer=m_lpCmdLine;
		strBuffer.MakeLower();
		
		iPos=strBuffer.Find("/?");
		if(iPos>=0)
		{
			MyError(IDS_INSTALLERUSAGE);
			return FALSE;
		}
		
		iPos=strBuffer.Find("/embed");
		if(iPos>=0)
			thePreSetup.m_EmbeddedRelease ="1";
					
		iPos=strBuffer.Find("/c");
		if(iPos>=0)
		{
			thePreSetup.m_CreateResponseFile ="1";
			MyError(IDS_CREATINGRSPFILE);
		}

		iPos=strBuffer.Find("/l");
		if(iPos>=0)
		{
			if (GetTempPath(sizeof(ach), ach))
			thePreSetup.m_MSILog.Format("%s\\ingmsi.log", ach);
			else
			thePreSetup.m_MSILog ="C:\\Temp\\ingmsi.log";
		}
	}/* if(m_lpCmdLine) */
	
	/* Check if Wizard is already running */
	if(!AlreadyRunning())
	{
		property=new WcPropSheet(IDS_TITLE);

		m_pMainWnd=property;
		INT_PTR response=property->DoModal();
		if (response == ID_WIZFINISH)
		{
		}
		delete property;
	}
	
	return FALSE;	// Return FALSE - do not start a main windows loop
}

BOOL Execute(LPCSTR lpCmdLine, BOOL bWait /*=TRUE*/)
{
	PROCESS_INFORMATION pi; 
	memset((char*)&pi,0,sizeof(pi)); 
	STARTUPINFO  si; 
	memset((char*)&si,0,sizeof(si)); 
	si.cb=sizeof(si);  
	if (CreateProcess(NULL,(LPSTR)lpCmdLine,NULL,NULL,FALSE,DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi))
	{
		if (bWait)
		{
			DWORD dw;
			WaitForSingleObject(pi.hProcess,INFINITE);
			if (GetExitCodeProcess(pi.hProcess,&dw))
				return (dw==0);
		}
		else
			return TRUE;
	}
	return FALSE;
}

/*
**  Description:
** 	Execute a command using a Windows command line.
** 
**	History:
*/
BOOL Execute(LPCSTR lpCmdLine, LPCSTR par /* =0 */, BOOL bWait /* =TRUE */)
{
	CString cmdLine(lpCmdLine);

	if(par)
	{
		cmdLine += " ";
		cmdLine += par;
	}
	return Execute(cmdLine, bWait);
}


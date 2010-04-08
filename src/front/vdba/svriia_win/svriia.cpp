/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : svriia.cpp : Defines the initialization routines for the DLL.
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main dll
**
** History:
**
** 11-Dec-2000 (uk$so01)
**    Created
** 18-Jun-2001 (noifr01) 
**    changed help file from svriia.hlp to iia.hlp
** 19-Sep-2001 (uk$so01)
**    BUG #105759 (Win98 only). Exit iia in the Step 2, the Disconnect
**    session did not return.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 25-Oct-2004 (noifr01)
**    (bug 113319) set the locale correctly for ensuring functions such
**    as _istleadbyte() work correctly
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale,
**    for sorting object name in Rigth pane list.
**/

#include "stdafx.h"
#include "svriia.h"
#include "fryeiass.h"
#include "libguids.h"
#include "taskanim.h"
#include "ingobdml.h"
#include <htmlhelp.h>
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL SetRegKeyValue(LPTSTR pszKey, LPTSTR pszSubkey, LPCTSTR pszValue);
static BOOL AddRegNamedValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValueName, LPCTSTR pszValue);


//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CSvriiaApp

BEGIN_MESSAGE_MAP(CSvriiaApp, CWinApp)
	//{{AFX_MSG_MAP(CSvriiaApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSvriiaApp construction

CSvriiaApp::CSvriiaApp()
{
	lstrcpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	m_strHelpFile = _T("iia.hlp");
	m_bHelpFileAvailable = TRUE;
	m_strInstallationID = _T("");
	m_pParamStruct = NULL;
	m_nSequence = 0;
	m_strNullNA = _T("n/a");
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSvriiaApp object
CSvriiaApp theApp;

BOOL CSvriiaApp::InitInstance() 
{
	m_strInstallationID = INGRESII_QueryInstallationID();
	m_pServer = new CaComServer();
	m_pServer->m_hInstServer = m_hInstance;
	_tsetlocale(LC_CTYPE ,"");	
	_tsetlocale(LC_COLLATE ,"");	
	BOOL bOk = CWinApp::InitInstance();
	if (bOk)
	{
		bOk = TaskAnimateInitialize();
		if (!bOk)
		{
			//
			// Failed to load the dll %1.
			CString strMsg;
			AfxFormatString1(strMsg, IDS_FAIL_TO_LOAD_DLL, _T("<TkAnimat.dll>"));
			AfxMessageBox (strMsg);
			return FALSE;
		}
	}
	m_userData4GetRow.InitResourceString();
	return bOk;
}

int CSvriiaApp::ExitInstance() 
{
	if (m_pServer)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	return CWinApp::ExitInstance();
}

BOOL CSvriiaApp::UnicodeOk()
{
	BOOL bOk = TRUE;
	TCHAR tchszUserName[256];
	DWORD dwSize = 256;

	if (!GetUserName(tchszUserName, &dwSize))
		bOk = ERROR_CALL_NOT_IMPLEMENTED == GetLastError() ? FALSE : TRUE;
	return bOk;
}


//
//  Function: DllRegisterServer
//
//  Summary:  The standard exported function that can be called to command
//            this DLL server to register itself in the system registry.
//
//  Args:
//
//  Returns:  HRESULT
//              NOERROR
//  ***********************************************************************************************
STDAPI DllRegisterServer()
{
	TCHAR tchszCLSID[128+1];
	TCHAR tchszModulePath[MAX_PATH];

	//
	// Obtain the path to this module's executable file for later use.
	GetModuleFileName(theApp.m_hInstance, tchszModulePath, sizeof(tchszModulePath)/sizeof(TCHAR));

	//
	// Create some base key strings.
	lstrcpy(tchszCLSID, TEXT("CLSID\\"));
	lstrcat(tchszCLSID, cstrCLSID_IMPxEXPxASSISTANCT);

	//
	// Create ProgID keys.
	if (!SetRegKeyValue(TEXT("IngresImportExport.1"), NULL, TEXT("IngresImportExport - Import/Export Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("IngresImportExport.1"), TEXT("CLSID"), cstrCLSID_IMPxEXPxASSISTANCT))
		return E_FAIL;

	//
	// Create VersionIndependentProgID keys.
	if (!SetRegKeyValue(TEXT("IngresImportExport"), NULL, TEXT("IngresImportExport - Import/Export Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("IngresImportExport"), TEXT("CurVer"), TEXT("IngresImportExport.1")))
		return E_FAIL;
	if (!SetRegKeyValue(TEXT("IngresImportExport"), TEXT("CLSID"), cstrCLSID_IMPxEXPxASSISTANCT))
		return E_FAIL;

	//
	// Create entries under CLSID.
	if (!SetRegKeyValue(tchszCLSID, NULL, TEXT("IngresImportExport - Import/Export Assistant")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("ProgID"), TEXT("IngresImportExport.1")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("VersionIndependentProgID"), TEXT("IngresImportExport")))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("NotInsertable"), NULL))
		return E_FAIL;
	if (!SetRegKeyValue(tchszCLSID, TEXT("InprocServer32"), tchszModulePath))
		return E_FAIL;
	if (!AddRegNamedValue(tchszCLSID, TEXT("InprocServer32"), TEXT("ThreadingModel"), TEXT("Both")))
		return E_FAIL;

	return NOERROR;
}



//
//  Function: DllUnregisterServer
//
//  Summary:  The standard exported function that can be called to command
//            this DLL server to unregister itself from the system Registry.
//
//  Args: 
//  Returns:  HRESULT
//              NOERROR
//  ***********************************************************************************************
STDAPI DllUnregisterServer()
{
	TCHAR tchszCLSID[128+1];
	TCHAR tchszTemp[512];

	lstrcpy(tchszCLSID, TEXT("CLSID\\"));
	lstrcat(tchszCLSID, cstrCLSID_IMPxEXPxASSISTANCT);

	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IngresImportExport\\CurVer")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IngresImportExport\\CLSID")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IngresImportExport")) != ERROR_SUCCESS)
		return E_FAIL;

	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IngresImportExport.1\\CLSID")) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IngresImportExport.1")) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\ProgID"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\VersionIndependentProgID"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\NotInsertable"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	lstrcpy(tchszTemp, tchszCLSID);
	lstrcat(tchszTemp, TEXT("\\InprocServer32"));
	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszTemp) != ERROR_SUCCESS)
		return E_FAIL;

	if (RegDeleteKey(HKEY_CLASSES_ROOT, tchszCLSID) != ERROR_SUCCESS)
		return E_FAIL;

	return NOERROR;
}












//
//  Function: DllGetClassObject
//
//  Summary:  The standard exported function that the COM service library
//            uses to obtain an object class of the class factory for a
//            specified component provided by this server DLL.
//
//  Args:     REFCLSID rclsid,
//              [in] The CLSID of the requested Component.
//            REFIID riid,
//              [in] GUID of the requested interface on the Class Factory.
//            LPVOID* ppv)
//              [out] Address of the caller's pointer variable that will
//              receive the requested interface pointer.
//
//  Returns:  HRESULT
//              E_FAIL if requested component isn't supported.
//              E_OUTOFMEMORY if out of memory.
//              Error code out of the QueryInterface.
//  ***********************************************************************************************
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hError = CLASS_E_CLASSNOTAVAILABLE;
	IUnknown* pCob = NULL;
	CaComServer* pServer = theApp.GetServer();

	if (CLSID_IMPxEXPxASSISTANCT == rclsid)
	{
		hError = E_OUTOFMEMORY;
		pCob = new CaFactoryImportExport(NULL, pServer);
	}

	if (NULL != pCob)
	{
		hError = pCob->QueryInterface(riid, ppv);
		if (FAILED(hError))
		{
			delete pCob;
			pCob = NULL;
		}
	}

	return hError;
}


//
//  Function: DllCanUnloadNow
//
//  Summary:  The standard exported function that the COM service library
//            uses to determine if this server DLL can be unloaded.
//
//  Args:
//
//  Returns:  HRESULT
//              S_OK if this DLL server can be unloaded.
//              S_FALSE if this DLL can not be unloaded.
//  ***********************************************************************************************
STDAPI DllCanUnloadNow()
{
	HRESULT hError;
	CaComServer* pServer = theApp.GetServer();

	//LOGF2("S: DllCanUnloadNow. cObjects=%i, cLocks=%i.", g_pServer->m_cObjects, g_pServer->m_cLocks);

	// We return S_OK of there are no longer any living objects AND
	// there are no outstanding client locks on this server.
	hError = (0L==pServer->m_cObjects && 0L==pServer->m_cLocks) ? S_OK : S_FALSE;

	return hError;
}



//
//  Function: SetRegKeyValue
//
//  Summary:  Internal utility function to set a Key, Subkey, and value
//            in the system Registry.
//
//  Args:     LPTSTR pszKey,
//            LPTSTR pszSubkey,
//            LPTSTR pszValue)
//
//  Returns:  BOOL
//              TRUE if success; FALSE if not.
//  ***********************************************************************************************
BOOL SetRegKeyValue(LPTSTR pszKey, LPTSTR pszSubkey, LPCTSTR pszValue)
{
	BOOL bOk = FALSE;

	LONG ec;
	HKEY hKey;
	TCHAR szKey[256];

	lstrcpy(szKey, pszKey);

	if (NULL != pszSubkey)
	{
		lstrcat(szKey, TEXT("\\"));
		lstrcat(szKey, pszSubkey);
	}

	ec = RegCreateKeyEx(
	    HKEY_CLASSES_ROOT,
	    szKey,
	    0,
	    NULL,
	    REG_OPTION_NON_VOLATILE,
	    KEY_ALL_ACCESS,
	    NULL,
	    &hKey,
	    NULL);

	if (ERROR_SUCCESS == ec)
	{
		if (NULL != pszValue)
		{
			ec = RegSetValueEx(
				hKey,
				NULL,
				0,
				REG_SZ,
				(BYTE *)pszValue,
				(lstrlen(pszValue)+1)*sizeof(TCHAR));
			if (ERROR_SUCCESS == ec)
				bOk = TRUE;
			RegCloseKey(hKey);
		}

		if (ERROR_SUCCESS == ec)
			bOk = TRUE;
	}
	return bOk;
}

//
//  Function: AddRegNamedValue
//
//  Summary:  Internal utility function to add a named data value to an
//            existing Key (with optional Subkey) in the system Registry
//            under HKEY_CLASSES_ROOT.
//
//  Args:     LPTSTR pszKey,
//            LPTSTR pszSubkey,
//            LPTSTR pszValueName,
//            LPTSTR pszValue)
//
//  Returns:  BOOL
//              TRUE if success; FALSE if not.
// ************************************************************************************************
BOOL AddRegNamedValue(LPTSTR pszKey, LPTSTR pszSubkey, LPTSTR pszValueName, LPCTSTR pszValue)
{
	BOOL bOk = FALSE;
	LONG ec;
	HKEY hKey;
	TCHAR szKey[256];

	lstrcpy(szKey, pszKey);

	if (NULL != pszSubkey)
	{
		lstrcat(szKey, TEXT("\\"));
		lstrcat(szKey, pszSubkey);
	}

	ec = RegOpenKeyEx(
	    HKEY_CLASSES_ROOT,
	    szKey,
	    0,
	    KEY_ALL_ACCESS,
	    &hKey);

	if (NULL != pszValue && ERROR_SUCCESS == ec)
	{
		ec = RegSetValueEx(
		    hKey,
		    pszValueName,
		    0,
		    REG_SZ,
		    (BYTE *)pszValue,
		    (lstrlen(pszValue)+1)*sizeof(TCHAR));
		if (ERROR_SUCCESS == ec)
		  bOk = TRUE;
		RegCloseKey(hKey);
	}

	return bOk;
}


BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += theApp.m_strHelpFile;
	}
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}


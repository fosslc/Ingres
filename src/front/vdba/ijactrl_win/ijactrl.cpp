/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijactrl.cpp , Implementation File File for IJACTRL.DLL
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Main header file for IJACTRL.DLL  (ActiveX)
**
** History:
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 10-Oct-2001 (hanje04)
**    Bug:105483
**    Added m_strLocalIITemporary so that a temporary location doesn't have to
**    be constructed from strLocalIISystem, which is wrong for UNIX. This will
**    be set to II_TEMPORARY as it should be.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 09-Jul-2002 (noifr01)
**    (bug 108204) Display float types according to the LOCALE.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale,
**    for sorting object name in tree and Rigth pane list.
**/

#include "stdafx.h"
#include <locale.h>
#include <htmlhelp.h>
#include "ijactrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CappIjaCtrl NEAR theApp;
CString  mystring;

const GUID CDECL BASED_CODE _tlid =
		{ 0xc92b8424, 0xb176, 0x11d3, { 0xa3, 0x22, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;







//
// This function is DBCS compliant.
static BOOL IVM_VarableExtract (LPCTSTR lpszVar, CString& strName, CString& strValue)
{
	CString strV = lpszVar;
	int n0A = strV.Find (0xA);
	if (n0A != -1)
		strV.SetAt(n0A, _T(' '));
	strV.TrimRight();
	int nEq = strV.Find (_T('='));
	if (nEq == -1)
		return FALSE;
	strName = strV.Left (nEq);
	strValue= strV.Mid (nEq+1);
	return TRUE;
}

//
// Ingres Environment Variable:
// Ex: 
//    lpszVaraiable = II_DATE_FORMAT
//    strValue = Empty string if not set
static BOOL INGRESII_CheckVariable (LPCTSTR lpszVaraiable, CString& strValue)
{
	HANDLE hChildStdoutRd, hChildStdoutWr;
	HANDLE hChildStdinRd, hChildStdinWr;
	DWORD  dwExitCode, dwLastError;
	DWORD  dwWaitResult = 0;
	BOOL   bError = FALSE;
	long   lTimeOut = 5*60*15; // 5 mins (Max time taken to finish execution of ingprenv)
	
	CString strResult = _T("");
	CString strError  = _T("Cannot execute command <ingprenv> !");
	CString strCmd = _T(""); //theApp.m_strIngresBin;
#if defined (MAINWIN)
	strCmd += _T("ingprenv");
#else
	strCmd += _T("ingprenv.exe");
#endif

	strValue = _T("");
	STARTUPINFO         StartInfo;
	PROCESS_INFORMATION ProcInfo;
	//
	// Init security attributes
	SECURITY_ATTRIBUTES sa;
	memset (&sa, 0, sizeof (sa));
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE; // make pipe handles inheritable
	sa.lpSecurityDescriptor = NULL;

	memset (&StartInfo, 0, sizeof(StartInfo));
	memset (&ProcInfo,  0, sizeof(ProcInfo));

	try
	{
		//
		// Create a pipe for the child's STDOUT
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		//
		// Create a pipe for the child's STDIN
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		
		//
		// Duplicate the write handle to the STDIN pipe so it's not inherited
		if (!DuplicateHandle(
			 GetCurrentProcess(), 
			 hChildStdinWr, 
			 GetCurrentProcess(), 
			 &hChildStdinWr, 
			 0, FALSE, 
			 DUPLICATE_SAME_ACCESS)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		//
		// Don't need it any more
		CloseHandle(hChildStdinWr);

		//
		// Create the child process
		StartInfo.cb = sizeof(STARTUPINFO);
		StartInfo.lpReserved = NULL;
		StartInfo.lpReserved2 = NULL;
		StartInfo.cbReserved2 = 0;
		StartInfo.lpDesktop = NULL;
		StartInfo.lpTitle = NULL;
		StartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		StartInfo.wShowWindow = SW_HIDE;
		StartInfo.hStdInput = hChildStdinRd;
		StartInfo.hStdOutput = hChildStdoutWr;
		StartInfo.hStdError = hChildStdoutWr;
		if (!CreateProcess (
			NULL, 
			(LPTSTR)(LPCTSTR)strCmd, 
			NULL, 
			NULL, 
			TRUE, 
			NORMAL_PRIORITY_CLASS, 
			NULL, 
			NULL, 
			&StartInfo, 
			&ProcInfo)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);
		
		WaitForSingleObject (ProcInfo.hProcess, lTimeOut);
		if (!GetExitCodeProcess(ProcInfo.hProcess, &dwExitCode))
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}

		switch (dwExitCode)
		{
		case STILL_ACTIVE:
			//
			// Time out
			return FALSE;
			break;
		default:
			//
			// Execution complete successfully
			break;
		}

		//
		// Reading data from the pipe:
		const int nSize = 48;
		TCHAR tchszBuffer [nSize];
		DWORD dwBytesRead;

		while (ReadFile(hChildStdoutRd, tchszBuffer, nSize-2, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				tchszBuffer [(int)dwBytesRead] = _T('\0');
				strResult+= tchszBuffer;
				tchszBuffer[0] = _T('\0');
			}
			else
			{
				//
				// The file pointer was beyond the current end of the file
				break;
			}
		}

		TCHAR  chszSep[] = _T("\n");
		TCHAR* token;
		CString strName;
		CString strValSet;
		// 
		// Establish string and get the first token:
		token = _tcstok ((TCHAR*)(LPCTSTR)strResult, chszSep);
		while (token != NULL )
		{
			BOOL bOK = IVM_VarableExtract ((LPCTSTR)token, strName, strValSet);
			if (bOK)
			{
				//
				// Variable has been set:
				if (strName.CompareNoCase (lpszVaraiable) == 0)
				{
					strValue = strValSet;
					return TRUE;
				}
			}
			//
			// While there are tokens in "strResult"
			token = _tcstok(NULL, chszSep);
		}
		return FALSE;
	}
	catch (LPCTSTR lpszError)
	{
		TRACE1 ("Internal error(1): %s\n", lpszError);
		lpszError = NULL;
		bError = TRUE;
	}
	catch (...)
	{
		bError = TRUE;
	}
	return bError? FALSE: TRUE;
}

////////////////////////////////////////////////////////////////////////////
// CappIjaCtrl::InitInstance - DLL initialization

BOOL CappIjaCtrl::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
	m_bHelpFileAvailable = TRUE;
	m_strHelpFile = _T("ija.chm");
	m_strNewHelpPath = _T("");
	_tsetlocale(LC_TIME, _T(""));
	_tsetlocale(LC_NUMERIC,_T(""));
	_tsetlocale(LC_COLLATE,_T(""));
	_tsetlocale(LC_CTYPE,_T(""));
	m_sessionManager.SetDescription (_T("Ingres Journal Analyzer"));
	m_dateFormat = II_DATE_US;
	if (bInit)
	{
		//
		// Allow to use the owner prefixed of table in the argument of the auditdb command:
		m_bTableWithOwnerAllowed = TRUE;

		m_strAllUser = _T("<all>");
		LPCTSTR pEnv  = _tgetenv(_T("II_SYSTEM"));
		if (!pEnv)
		{
			CString strMsg = _T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg);
			m_strLocalIISystem = _T("");
		}
		else
			m_strLocalIISystem = (LPCTSTR)pEnv;

		CString strValue;
		BOOL bOK = INGRESII_CheckVariable (_T("II_DATE_FORMAT"), strValue);
		if (bOK)
		{
			if (strValue.IsEmpty())
			{
				//
				// US (default format)
				m_dateFormat = II_DATE_US;
			}
			else
			if (strValue.CompareNoCase (_T("US")) == 0)
			{
				m_dateFormat = II_DATE_US;
			}
			else
			if (strValue.CompareNoCase (_T("MULTINATIONAL")) == 0)
			{
				m_dateFormat = II_DATE_MULTINATIONAL;
			}
			else
			if (strValue.CompareNoCase (_T("II_DATE_MULTINATIONAL4")) == 0)
			{
				m_dateFormat = II_DATE_MULTINATIONAL4;
			}
			if (strValue.CompareNoCase (_T("ISO")) == 0)
			{
				m_dateFormat = II_DATE_ISO;
			}
			else
			if (strValue.CompareNoCase (_T("SWEDEN")) == 0 || strValue.CompareNoCase (_T("FINLAND")) == 0)
			{
				m_dateFormat = II_DATE_SWEDEN; // Use the Sweden II_DATE_FORMAT
			}
			else
			if (strValue.CompareNoCase (_T("GERMAN")) == 0)
			{
				m_dateFormat = II_DATE_GERMAN;
			}
			else
			if (strValue.CompareNoCase (_T("YMD")) == 0)
			{
				m_dateFormat = II_DATE_YMD;
			}
			else
			if (strValue.CompareNoCase (_T("DMY")) == 0)
			{
				m_dateFormat = II_DATE_DMY;
			}
			else
			if (strValue.CompareNoCase (_T("MDY")) == 0)
			{
				m_dateFormat = II_DATE_MDY;
			}
		}

		m_dateCenturyBoundary = 0;
		strValue = _T("");
		bOK = INGRESII_CheckVariable (_T("II_DATE_CENTURY_BOUNDARY"), strValue);
		if (bOK && !strValue.IsEmpty())
			m_dateCenturyBoundary = _ttoi (strValue);
		
#ifdef MAINWIN
		m_strLocalIITemporary = _T("/tmp");
#else
		m_strLocalIITemporary = m_strLocalIISystem + _T("\\ingres\\temp");
#endif
		strValue = _T("");
		bOK = INGRESII_CheckVariable (_T("II_TEMPORARY"), strValue);
		if (bOK && !strValue.IsEmpty())
			m_strLocalIITemporary = strValue;
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CappIjaCtrl::ExitInstance - DLL termination

int CappIjaCtrl::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	if (theApp.m_strNewHelpPath.IsEmpty())
		strTemp = theApp.m_pszHelpFilePath;
	else
		strTemp = theApp.m_strNewHelpPath;

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
	else
		strHelpFilePath = theApp.m_strHelpFile;
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (theApp.m_bHelpFileAvailable)
	{
		if (nHelpID == 0)
			HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
		else
			HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);
	}
	else
	{
		TRACE0 ("Help file is not available: <theApp.m_bHelpFileAvailable = FALSE>\n");
		return FALSE;
	}

	return TRUE;
}

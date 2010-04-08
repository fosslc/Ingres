/*
**
**  Name: iilink.cpp
**
**  Description:
**	This file defines the class behaviors for the application.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
**	30-jul-2001 (somsa01)
**	    Fixed up compiler warnings.
**	26-dec-2001 (somsa01)
**	    In CompareVersion(), allow for VC 7.0.
**	18-sep-2002 (somsa01)
**	    If VC 6.0 and VC .NET both exist, ask the user which one
**	    he wants to use. Also, added the capability to set up our
**	    environment for the VC .NET compiler.
**	31-oct-2002 (penga03)
**	    Disable the F1 Help.
**	04-sep-2003 (somsa01)
**	    Updated for VS .NET 2003.
**	14-oct-2003 (somsa01)
**	    In CompareVersion(), allow for VC 8.0 (compiler on AMD64 Windows).
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	22-jun-2004 (somsa01)
**	    More Open Source cleanup.
**	19-apr-2005 (penga03)
**	    Show IDS_VC7_EXISTS only when Visual Studio .NET is not the default
**	    compiler.
**	03-apr-2008 (drivi01)
**	    Put spatial objects support back.
**	06-May-2008 (drivi01)
**	    Update the way visual controls are loaded. Enable3dControls has been
**	    been depricated.
**	07-May-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008, update all compilation and
**          link command lines with Visual Studio 2008 locations.
**	01-Oct-2009 (drivi01)
**	     Made the following changes:	    
**		- Make Visual Studio 2008 a default compiler 
**		- Add routines for detecting Microsoft SDK 6.0A to complete search 
**		  path for libraries when UDT/SOL are being linked.
**		- Update search pattern in a PATH to Visual Studio 2005 and 2008 
**		  compilers.
**		- Add additional command to build manifest into iilibudt.dll
**		  produced by iilink.
**		- Block this utility if compiler is older or newer than 8.0 or 9.0.
**		
**	    
*/

#include "stdafx.h"
#include "iilink.h"
#include "PropSheet.h"
#include <winver.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct node *FileSrcList, *FileObjList;
HPALETTE hSystemPalette=0;
BOOL IncludeSpat = FALSE;
BOOL IncludeDemo = FALSE;
BOOL UseUserUdts = FALSE;
BOOL LocChanged = FALSE;
BOOL Backup = FALSE;
BOOL IngresUp = FALSE;
CString BackupExt, UserLoc, CompileCmd, LinkCmd, ManifestCmd;
HWND MainWnd;

/*
** CIilinkApp
*/
BEGIN_MESSAGE_MAP(CIilinkApp, CWinApp)
    //{{AFX_MSG_MAP(CIilinkApp)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
END_MESSAGE_MAP()

/*
** CIilinkApp construction
*/
CIilinkApp::CIilinkApp()
{
}

/*
** The one and only CIilinkApp object
*/
CIilinkApp theApp;

/*
** CIilinkApp initialization
*/
BOOL
CIilinkApp::InitInstance()
{
    INT_PTR		nResponse;
    CString		Message, Message2;
    char		CompilerLoc[1024];
    HKEY		hKeyVC9, hWinSDK6;
    int			rc;
    BOOL		bDefault=0;

    /*
    ** Make sure II_SYSTEM is set.
    */
    if (!strcmp(getenv("II_SYSTEM"), ""))
    {
	Message.LoadString(IDS_NO_IISYSTEM);
	Message2.LoadString(IDS_TITLE);
	MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
    }

    rc = GetEnvironmentVariable("PATH", NULL, 0);
    if (rc > 0)
    {
	char *szBuf, *pdest;
	int result1=0, result2=0;

	szBuf = (char *)malloc(rc);
	GetEnvironmentVariable("PATH", szBuf, rc);
	_tcslwr(szBuf);
	pdest=_tcsstr(szBuf, "9.0\\vc");
	if (pdest != NULL) result1=pdest-szBuf+1;
	pdest=_tcsstr(szBuf, "8\\vc");
	if (pdest != NULL) result2=pdest-szBuf+1;
	if (result1 && result2 && result1 < result2 || result1 && !result2)
	    bDefault=1;
	
	free(szBuf);
    }

	/* Find SDK 6.0 that goes along with VC9.0 */
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
			"SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v6.0A",
			0,
			KEY_READ,
			&hWinSDK6) != ERROR_SUCCESS && bDefault)
	{
		Message.LoadString(IDS_NO_SDK6);
		Message2.LoadString(IDS_TITLE);
		MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	}
    /*
    ** Let's find out if Visual Studio 9.0 is installed, and set up the environment to it.
    */
    if (RegOpenKeyEx (
	    HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\VisualStudio\\9.0\\Setup\\VS",
	    0,
	    KEY_READ,
	    &hKeyVC9) == ERROR_SUCCESS)
    {
	if (SearchPath(NULL, "cl.exe", NULL, sizeof(CompilerLoc), CompilerLoc,
			NULL) != 0 && !bDefault)
	{
	    Message.LoadString(IDS_VC9_EXISTS);
	    Message2.LoadString(IDS_TITLE);
	    rc = MessageBox(NULL, Message, Message2, MB_YESNO | MB_ICONQUESTION);
	    if (rc == IDYES)
	    {
			if (hWinSDK6 == NULL)
			{
			Message.LoadString(IDS_NO_SDK6);
			Message2.LoadString(IDS_TITLE);
			MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
			}
			/*
			** Set up our environment to point to the 9.0 compiler.
			*/
			SetupVC9Environment(hKeyVC9, hWinSDK6);
	    }
	}
	else
	{
	    /*
	    ** Set up our environment to point to the VC .NET compiler.
	    */
	    SetupVC9Environment(hKeyVC9, hWinSDK6);
	}

	RegCloseKey(hKeyVC9);
	RegCloseKey(hWinSDK6);
    }

    /*
    ** Make sure we can find the compiler.
    */
    if (!SearchPath(NULL, "cl.exe", NULL, sizeof(CompilerLoc), CompilerLoc,
		    NULL))
    {
	Message.LoadString(IDS_NO_COMPILER);
	Message2.LoadString(IDS_TITLE);
	MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
    }

    /*
    ** Make sure the compiler version is at least at the version that
    ** the product was built with.
    */
    if (!CompareVersion(CompilerLoc))
	return FALSE;

    /*
    ** Make sure the installation is down.
    */
    if (ping_iigcn()!=1)
    {
	int	status;

	Message.LoadString(IDS_INGRES_RUNNING);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(NULL, Message, Message2, MB_YESNO | MB_ICONINFORMATION);
	if (status == IDNO)
	    return FALSE;
	else
	    IngresUp = TRUE;
    }

    AfxEnableControlContainer();

    hSystemPalette=0;
/*  // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);
*/
    InitCommonControls();
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    FileSrcList = FileObjList = NULL;
    UserLoc = "";
    BackupExt = "BAK";
    PropSheet psDlg("Ingres User Defined Data Type Linker");
    m_pMainWnd = &psDlg;
    nResponse = psDlg.DoModal();
    if (nResponse == IDOK)
    {
    }
    else if (nResponse == IDCANCEL)
    {
    }

    if (hSystemPalette)
    {
	HDC hDC=::GetDC(NULL);
	::SelectPalette(hDC,hSystemPalette,FALSE);
	::SelectPalette(hDC,hSystemPalette,TRUE);
	::RealizePalette(hDC);
	::ReleaseDC(NULL,hDC);
    }

    /*
    ** Since the dialog has been closed, return FALSE so that we exit the
    ** application, rather than start the application's message pump.
    */
    return FALSE;
}

INT
ExecuteCmds()
{
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    SECURITY_ATTRIBUTES	sa;
    DWORD		status, wait_status = 0, NumWritten;
    HANDLE		newstdout, newstderr;
    CString		LogFile;
    
	char CompilerLoc[1024];
	SearchPath(NULL, "cl.exe", NULL, sizeof(CompilerLoc), CompilerLoc,NULL);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    LogFile = CString(getenv("II_SYSTEM")) +
	      CString("\\ingres\\files\\iilink.log");
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb = sizeof (siStInfo);
    siStInfo.lpReserved = NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop = NULL;
    siStInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;

    /*
    ** Set the process' output and error handles to the log file.
    */
    newstdout = CreateFile( LogFile, GENERIC_WRITE, FILE_SHARE_WRITE,
			    &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
			    NULL);
    newstderr = CreateFile( LogFile, GENERIC_WRITE, FILE_SHARE_WRITE,
			    &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
			    NULL);
    siStInfo.dwFlags |= STARTF_USESTDHANDLES;
    siStInfo.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    siStInfo.hStdOutput = newstdout;
    siStInfo.hStdError = newstderr;

    if (CompileCmd != "")
    {
	WriteFile(newstdout, CompileCmd, CompileCmd.GetLength(),
		  &NumWritten, NULL);
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
	status = CreateProcess( NULL,
				CompileCmd.GetBuffer(CompileCmd.GetLength()),
				&sa, NULL, TRUE, 0, NULL, NULL, &siStInfo,
				&piProcInfo);
	if (!status)
	{
	    CloseHandle(newstdout);
	    CloseHandle(newstderr);
	    SendMessage(MainWnd, WM_CLOSE, 0, 0);
	    return (status);
	}
	else
	{
	    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	    CloseHandle(piProcInfo.hProcess);
	    CloseHandle(piProcInfo.hThread);
	    if (wait_status)
	    {
		CloseHandle(newstdout);
		CloseHandle(newstderr);
		SendMessage(MainWnd, WM_CLOSE, 0, 0);
		return (wait_status);
	    }
	}
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
    }

    if (LinkCmd != "")
    {
	WriteFile(newstdout, LinkCmd, LinkCmd.GetLength(),
		  &NumWritten, NULL);
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
	status = CreateProcess( NULL,
				LinkCmd.GetBuffer(LinkCmd.GetLength()),
				&sa, NULL, TRUE, 0, NULL, NULL, &siStInfo,
				&piProcInfo);
	if (!status)
	{
	    CloseHandle(newstdout);
	    CloseHandle(newstderr);
	    SendMessage(MainWnd, WM_CLOSE, 0, 0);
	    return (status);
	}
	else
	{
	    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	    CloseHandle(piProcInfo.hProcess);
	    CloseHandle(piProcInfo.hThread);
	    if (wait_status)
	    {
		CloseHandle(newstdout);
		CloseHandle(newstderr);
		SendMessage(MainWnd, WM_CLOSE, 0, 0);
		return (wait_status);
	    }
	}
	WriteFile(newstdout, "\r\n", 1, &NumWritten, NULL);
    }

	if (ManifestCmd != "")
    {
	WriteFile(newstdout, ManifestCmd, ManifestCmd.GetLength(),
		  &NumWritten, NULL);
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
	status = CreateProcess( NULL,
				ManifestCmd.GetBuffer(ManifestCmd.GetLength()),
				&sa, NULL, TRUE, 0, NULL, NULL, &siStInfo,
				&piProcInfo);
	if (!status)
	{
	    CloseHandle(newstdout);
	    CloseHandle(newstderr);
	    SendMessage(MainWnd, WM_CLOSE, 0, 0);
	    return (status);
	}
	else
	{
	    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	    CloseHandle(piProcInfo.hProcess);
	    CloseHandle(piProcInfo.hThread);
	    if (wait_status)
	    {
		CloseHandle(newstdout);
		CloseHandle(newstderr);
		SendMessage(MainWnd, WM_CLOSE, 0, 0);
		return (wait_status);
	    }
	}
	WriteFile(newstdout, "\r\n", 1, &NumWritten, NULL);
    }

    CloseHandle(newstdout);
    CloseHandle(newstderr);
    Sleep(2000);
    SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return 0;
}

INT
CompareVersion(char *FileName)
{
    DWORD	dwHandle = 0L;	/* Ignored in call to GetFileVersionInfo */
    DWORD	cbBuf = 0L;
    LPVOID	lpvData = NULL, lpValue = NULL, lpMsgBuf;
    UINT	wBytes = 0L;
    WORD	wlang = 0, wcset = 0;
    char	SubBlk[128];
    int		status = 0, majver = 0, minver = 0, relno = 0;
    CString	Message, Message2;

    /* Retrieve Size of Version Resource */
    if ((cbBuf = GetFileVersionInfoSize(FileName, &dwHandle)) == 0)
    {
	FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf, 0, NULL );
	Message2.LoadString(IDS_TITLE);
	Message.LoadString(IDS_FAIL_VERS);
	Message += CString((LPTSTR)lpMsgBuf);
	MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(lpMsgBuf);
	return 0;
    }

    lpvData = (LPVOID)malloc(cbBuf);
    if(!lpvData)
    {
	FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf, 0, NULL );
	Message2.LoadString(IDS_TITLE);
	Message.LoadString(IDS_FAIL_VERS);
	Message += CString((LPTSTR)lpMsgBuf);
	MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(lpMsgBuf);
	free(lpvData);
	return 0;
    }

    /* Retrieve Version Resource */
    if (GetFileVersionInfo(FileName, dwHandle, cbBuf, lpvData) == FALSE)
    {
	FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf, 0, NULL );
	Message2.LoadString(IDS_TITLE);
	Message.LoadString(IDS_FAIL_VERS);
	Message += CString((LPTSTR)lpMsgBuf);
	MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(lpMsgBuf);
	free(lpvData);
	return 0;
    }

    /* Retrieve the Language and Character Set Codes */
    VerQueryValue(lpvData, TEXT("\\VarFileInfo\\Translation"),
		  &lpValue, &wBytes);
    wlang = *((WORD *)lpValue);
    wcset = *(((WORD *)lpValue)+1);

    /* Retrieve ProductVersion Information */
    sprintf(SubBlk, "\\StringFileInfo\\%.4x%.4x\\ProductVersion",
	    wlang, wcset);
    VerQueryValue(lpvData, TEXT(SubBlk), &lpValue, &wBytes);
    sscanf((char *)lpValue, "%d.%d.%d", &majver, &minver, &relno);
    free(lpvData);

    /* Check Major & Minor Version Number, should be 8.0 or 9.0 */
    if (((majver == 8) && (minver == 0)) ||
	((majver == 9) && (minver == 0))) 
	return 1;
    else
    {
	char	outstring[1024];

	Message.LoadString(IDS_VERSION_INCORRECT);
	Message2.LoadString(IDS_TITLE);
	sprintf(outstring, Message, majver, minver);
	MessageBox(NULL, outstring, Message2, MB_OK | MB_ICONEXCLAMATION);
	return 0;
    }
}

VOID
SetupVC9Environment(HKEY hKeyVC9, HKEY hWinSDK6)
{
    int			rc;
    DWORD		dwType;
    TCHAR		tchValue[MAX_PATH], tchValue2[MAX_PATH];
    unsigned long	dwSize = sizeof(tchValue);

    /*
    ** Set up our environment to point to the VC .NET compiler.
    */
    if (RegQueryValueEx(hKeyVC9,
			"ProductDir",
			NULL,
			&dwType,
			(unsigned char *)&tchValue,
			&dwSize) == ERROR_SUCCESS)
    {
	LPTSTR  tchBuf, tchBuf2;
	CHAR    temp = '\0';

	rc = GetEnvironmentVariable("PATH", &temp, 1);
	if (rc > 0)
	{
	    tchBuf = (char *)malloc(rc);
	    GetEnvironmentVariable("PATH", tchBuf, rc);
	    rc += 1024;  /* This is to make room for the add-ons. */
	}
	else
	{
	    rc = 1024;
	    tchBuf = NULL;
	}
	
	tchBuf2 = (char *)malloc(rc);
	sprintf(tchBuf2,
	    "%s\\Common7\\IDE;%s\\VC\\BIN;%s\\Common7\\Tools",
	    tchValue, tchValue, tchValue);
	if (tchBuf)
	{
	    strcat(tchBuf2, ";");
	    strcat(tchBuf2, tchBuf);
	}
	SetEnvironmentVariable("PATH", tchBuf2);
	if (tchBuf)
	    free(tchBuf);
	free(tchBuf2);
	
	rc = GetEnvironmentVariable("INCLUDE", &temp, 1);
	if (rc > 0)
	{
	    tchBuf = (char *)malloc(rc);
	    GetEnvironmentVariable("INCLUDE", tchBuf, rc);
	    rc += 1024;  /* This is to make room for the add-ons. */
	}
	else
	{
	    rc = 1024;
	    tchBuf = NULL;
	}
	
	tchBuf2 = (char *)malloc(rc);
	sprintf(tchBuf2,
	    "%s\\VC\\ATLMFC\\INCLUDE;%s\\VC\\INCLUDE",
	    tchValue, tchValue);
	if (tchBuf)
	{
	    strcat(tchBuf2, ";");
	    strcat(tchBuf2, tchBuf);
	}
	SetEnvironmentVariable("INCLUDE", tchBuf2);
	if (tchBuf)
	    free(tchBuf);
	free(tchBuf2);
	

	rc = GetEnvironmentVariable("LIB", &temp, 1);
	if (rc > 0)
	{
	    tchBuf = (char *)malloc(rc);
	    GetEnvironmentVariable("LIB", tchBuf, rc);
	    rc += 1024;  /* This is to make room for the add-ons. */
	}
	else
	{
	    rc = 1024;
	    tchBuf = NULL;
	}
	
	tchBuf2 = (char *)malloc(rc);
	sprintf(tchBuf2,
	    "%s\\VC\\ATLMFC\\LIB;%s\\VC\\LIB",
	    tchValue, tchValue);

	/* Retrieve path to Windows SDK 6.0A to add SDK libs to the LIB */
	dwSize=sizeof(tchValue2);
	if (hWinSDK6 != NULL)
	{
		*tchValue2='\0';
		if (RegQueryValueEx(hWinSDK6,
			"CurrentInstallFolder",
			NULL,
			&dwType,
			(unsigned char *)&tchValue2,
			&dwSize) == ERROR_SUCCESS)
		{
			if (*tchValue2 && tchValue2)
			{
			strcat(tchBuf2, ";");
			strcat(tchBuf2, tchValue2);
			strcat(tchBuf2, "Lib");
			}
		}
	}
	if (tchBuf)
	{
	    strcat(tchBuf2, ";");
	    strcat(tchBuf2, tchBuf);
	}
	SetEnvironmentVariable("LIB", tchBuf2);
	if (tchBuf)
	    free(tchBuf);
	free(tchBuf2);
    }
}

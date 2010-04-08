/*
** Copyright (c) 2001 Ingres Corporation
*/

/*
** Name: iipostinst.cpp - Defines the initialization routines for the EXE.
**
** History:
**	10-apr-2001 (somsa01)
**	    Created.
**	07-may-2001 (somsa01)
**	    Changed m_upgrade to m_DBMSupgrade for clarity.
**	13-jun-2001 (somsa01)
**	    Removed Replicator from the list of reasons for rebooting.
**	06-nov-2001 (penga03)
**	    Added functions CheckMicrosoft, CheckNetscape, CheckSpyGlass and 
**	    GetColdFusionPath to get the install path of the HTTP server and 
**	    the ColdFusion path installed locally.
**	14-jan-2002 (penga03)
**	    If silent mode, pop up a MessageBox to inform user when the post 
**	    installaiton completes successfully.
**	06-mar-2002 (penga03)
**	    Set the proper title for evaluation releases.
**	27-mar-2002 (penga03)
**	    While poping up a message box, if property->m_hWnd is 0, get 
**	    iipostinst window handle using FindWindow().
**	03-july-2002 (penga03)
**	    For a silent install, do not pop up the message box when the 
**	    post install process completes.
**	16-jul-2002 (penga03)
**	    Since Ingres/ICE does not support Spyglass any more, instead, 
**	    it supports Apache, replace wherever that are associated with 
**	    Spyglass with Apache.
**	05-nov-2002 (penga03)
**	    In InitInstance(), made following changes:
**	    Do "m_pMainWnd = &dlg" only in GUI mode, because it may cause 
**	    Dr. Watson in silent mode; 
**	    Removed the AfxEnableControlContainer(), since iipostinst.exe 
**	    does not call ActiveX controls; 
**	    Do not reboot for silent mode, this is what we did in the old 
**	    installer; 
**	    Changed "if (theInstall.m_win95 && theInstall.m_reboot)" to 
**	    "if (theInstall.m_win95 || theInstall.m_reboot)", which is a 
**	    logic mistake by accident.
**	03-jul-2003 (penga03)
**	    In InitInstance(), changed 
**	    "if (theInstall.m_win95 || theInstall.m_reboot)" back to 
**	    "if (theInstall.m_win95 && theInstall.m_reboot)".
**	07-feb-2005 (penga03)
**	     Modified function InitInstance so that ivm can be started at
**	     end of installation for silent install.
**	23-jun-2005 (penga03)
**	    Added ExitInstance() and return_code to return the 
**	    iipostinst.exe's exit code.
**	18-aug-2005 (penga03)
**	    Moved writting the return code to the install.log from 
**	    ThreadPostInstallation() to ExitInstance(). And also initialize 
**	    the return_code in InitInstance().
**	28-Jun-2006 (drivi01)
**	    SIR 116381
**	    iipostinst.exe is renamed to ingconfig.exe. Fix all calls to
**	    iipostinst.exe to call ingconfig.exe instead.
**  15-Nov-2006 (drivi01)
**	    SIR 116599
**	    Enhanced post-installer in effort to improve installer usability.
**	    Enable3DControls function has been deprecated. Since the manifest
**	    was added to take advantage of ComCtl32.dll version 6 visual styles
**	    the function was replaced with  InitCommonControlsEx().
**  04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Adjust the title of the post installer to fit DBA Tools 
**	    installation.
**	    Note: This change updates the title of the dialog by replacing
**	    "Ingres" with "DBA Tools", this will require specials consideration
**	    if these tools are ever to be translated.
*/

#include "stdafx.h"
#include "iipostinst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int	return_code = 0;
HPALETTE    hSystemPalette = 0;
LPCSTR	    END_OF_LINE = "\r\n";

/*
** CIipostinstApp
*/
BEGIN_MESSAGE_MAP(CIipostinstApp, CWinApp)
    //{{AFX_MSG_MAP(CIipostinstApp)
	    // NOTE - the ClassWizard will add and remove mapping macros here.
	    //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CIipostinstApp construction
*/
CIipostinstApp::CIipostinstApp()
{
}

/*
** The one and only CIipostinstApp object
*/
CIipostinstApp	theApp;
CInstallation	theInstall;
CPropSheet	*property = 0;
HICON		theIcon = 0;

BOOL
FileExists(LPCSTR s)
{
    HANDLE File = CreateFile(s,0,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    
    if (File == INVALID_HANDLE_VALUE)
	return FALSE;
    CloseHandle(File);

    return TRUE;
}

static UINT
MyMessageBox(UINT uiStrID, UINT uiFlags, LPCSTR param)
{
    UINT ret = 0;

    CString title;
    CString text;

#ifdef EVALUATION_RELEASE
    title.LoadString(IDS_SDKTITLE);
#else
    if (theInstall.m_DBATools)
    title.Format(IDS_TITLE_DBA, theInstall.m_installationcode);
    else
    title.Format(IDS_TITLE, theInstall.m_installationcode);
#endif /* EVALUATION_RELEASE */

    theInstall.m_halted = TRUE;
    if (param)
	text.Format(uiStrID, param);
    else
	text.LoadString(uiStrID);

    HWND hwnd = property ? property->m_hWnd : 0;
    if (!hwnd)
	hwnd=FindWindow(NULL, title);
    if (!hwnd)
	uiFlags |= MB_APPLMODAL;
    
    if (theInstall.m_UseResponseFile)
    {
	text = "MessageBox - " + text;
	theInstall.AppendToLog(text);
    }
    else
	ret = ::MessageBox(hwnd, text, title, uiFlags);

    theInstall.m_halted = FALSE;
    return ret;
}

void
Message(UINT uiStrID, LPCSTR param)
{
    MyMessageBox(uiStrID, MB_OK | MB_ICONINFORMATION, param);
}

BOOL
AskUserYN(UINT uiStrID, LPCSTR param)
{
    return (MyMessageBox(uiStrID, MB_YESNO | MB_ICONQUESTION, param) == IDYES);
}

void
Error(UINT uiStrID,LPCSTR param)
{
    MyMessageBox(uiStrID, MB_OK | MB_ICONEXCLAMATION, param);
}

BOOL
IsWindows95()
{
    OSVERSIONINFO osver;

    memset((char *) &osver,0,sizeof(osver));
    osver.dwOSVersionInfoSize=sizeof(osver);
    GetVersionEx(&osver);
    
    if ((osver.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) &&
	(osver.dwMajorVersion>3)) // Windows 95
    {
	return TRUE;
    }

    return FALSE;
}

int 
doGetRegistryEntry(HKEY hRootKey, char *szRegKey, char *szKeyName, CString &strRetValue)
{ DWORD dwValLen, dwDataType;
  
  HKEY hRetKey;
  int iKeyFound=0, iRetCode;
  char szValue[512];

  if ((!szRegKey) || (!szKeyName) || (!strRetValue))
     return (-1);

  iRetCode = RegOpenKeyEx (hRootKey,    // Main subkey
                          szRegKey,             // Key name
                          (DWORD)0,                  // Reserved
                          KEY_ALL_ACCESS,            // Use rw access
                          &hRetKey);                 // Return handle
   
   if (iRetCode == ERROR_SUCCESS)  
   { dwValLen = sizeof (szValue);
    
     iRetCode = RegQueryValueEx (hRetKey,          // Handle to open key
                          szKeyName,               // User path
                          0,                       // Reserved
                          &dwDataType,             // Data type
                          (unsigned char *)szValue, // The buffer
                          &dwValLen);              // Buffer len
     
     if (iRetCode == ERROR_SUCCESS)
     { strRetValue = szValue;
       iKeyFound = 1;
     }
   }
  if (hRetKey)
    RegCloseKey(hRetKey);

 return(iKeyFound);   
}

BOOL 
CheckMicrosoft(CString &strValue)
{ 
  strValue="";
  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots", "/", strValue))
    goto ValueFound;

  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots", "/,", strValue))
    goto ValueFound;

  return FALSE;

ValueFound:
  theInstall.m_HTTP_Server = "Microsoft";
  int iPos = strValue.Find(',');
  if (iPos > 0)
    strValue = strValue.Left(iPos);

  return TRUE;
}

BOOL 
CheckNetscape(CString &strValue)
{  
  char szRegKey[512];
  char szComputerName[512];
  CStdioFile      theInputFile;
  CFileException  fileException;
  CString         theInputString;
  int iPos, iPathPos;

  strValue="";
  ULONG ul=sizeof(szComputerName);
  GetComputerName(szComputerName,&ul);

  sprintf(szRegKey, "SOFTWARE\\Netscape\\https-%s", szComputerName);
  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
    goto ValueFound;

  sprintf(szRegKey, "SOFTWARE\\Netscape\\httpd-%s", szComputerName);
  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
    goto ValueFound;

  sprintf(szRegKey, "SOFTWARE\\Netscape\\Enterprise\\3.5.1\\https-%s", szComputerName);
  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
    goto ValueFound;

  sprintf(szRegKey, "SOFTWARE\\Netscape\\Enterprise\\3.5.1\\httpd-%s", szComputerName);
  if (doGetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
    goto ValueFound;

  return FALSE;

ValueFound:
  theInstall.m_HTTP_Server = "Netscape";
  // Convert '/' to '\'
  iPos = strValue.Find('/');
  while (iPos >= 0)
  { strValue.SetAt(iPos, '\\');
    iPos = strValue.Find('/');
  }
  strValue += "\\obj.conf";

  if ( !theInputFile.Open( strValue, CFile::modeRead  | CFile::typeText, &fileException )) 
     return FALSE;
 
  while (theInputFile.ReadString(theInputString) != FALSE)
  {  
     theInputString.TrimRight();
     
     iPathPos = theInputString.Find("root=");
      
     if (iPathPos >= 0)
     { 
       
       theInputString = theInputString.Mid(iPathPos+6);
       theInputString = theInputString.Left(theInputString.GetLength()-1);
            
       // Convert '/' to '\'
       iPos = theInputString.Find('/');
       while (iPos >= 0)
       { theInputString.SetAt(iPos, '\\');
         iPos = theInputString.Find('/');
       }

       theInputFile.Close();
       return TRUE;
     }
  }

  theInputFile.Close();
  return FALSE;
}

BOOL 
CheckApache(CString &strValue)
{ 
    char KeyName[]="SOFTWARE\\Apache Group\\Apache";
    HKEY hKey;

    strValue="";
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
    {
	DWORD dwIndex;

	if(!RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwIndex, NULL, 
	                    NULL, NULL, NULL, NULL, NULL, NULL))
	{
	    char szName[7];

	    dwIndex--;
	    if(!RegEnumKey(hKey, dwIndex, szName, sizeof(szName)))
	    {
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey))
		{
		    char szData[MAX_PATH];
		    DWORD dwSize=sizeof(szData);

		    if(!RegQueryValueEx(hSubKey, "ServerRoot", 0, NULL, (BYTE *)szData, &dwSize))
		    {
			strValue.Format("%shtdocs", szData);
			theInstall.m_HTTP_Server="Apache";
			return TRUE;
		    }
		}
	    }
	}
    }

    if(!RegOpenKeyEx(HKEY_CURRENT_USER, KeyName, 0, KEY_ALL_ACCESS, &hKey))
    {
	DWORD dwIndex;

	if(!RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwIndex, NULL, 
	                    NULL, NULL, NULL, NULL, NULL, NULL))
	{
	    char szName[7];

	    dwIndex--;
	    if(!RegEnumKey(hKey, dwIndex, szName, sizeof(szName)))
	    {
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey))
		{
		    char szData[MAX_PATH];
		    DWORD dwSize=sizeof(szData);

		    if(!RegQueryValueEx(hSubKey, "ServerRoot", 0, NULL, (BYTE *)szData, &dwSize))
		    {
			strValue.Format("%shtdocs", szData);
			theInstall.m_HTTP_Server="Apache";
			return TRUE;
		    }
		}
	    }
	}
    }
    return FALSE;
}

BOOL 
GetColdFusionPath(CString &strCFInstallPath) 
{  HKEY  queryKey=0;	
   CString strValue;
   strCFInstallPath="";

   if (doGetRegistryEntry(HKEY_CURRENT_USER, "Software\\Allaire\\Studio45\\FileLocations", "ProgramDir", strValue))
   { strCFInstallPath=strValue;
     return TRUE;
   }
   else if (doGetRegistryEntry(HKEY_CURRENT_USER, "Software\\Allaire\\Studio4\\FileLocations", "ProgramDir", strValue))
   { strCFInstallPath=strValue;
     return TRUE;
   }
   
   return FALSE;
}

BOOL
CIipostinstApp::InitInstance()
{
    BOOL    silent;

    /* Standard initialization */
    theInstall.m_DBMSupgrade = FALSE;
    theInstall.m_UseResponseFile = FALSE;
    theInstall.m_bSkipLicenseCheck = FALSE;
    theInstall.m_bEmbeddedRelease = FALSE;
    theInstall.m_commandline = "";
    theInstall.m_SoftwareKey = NULL;

	return_code = 0;
    hSystemPalette = 0;
    theIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	/*// InitCommonControlsEx() is required on Windows XP if an application
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

    theInstall.Init();
    silent = theInstall.GetRegValueBOOL("SilentInstall", FALSE);
    theInstall.DeleteRegValue("InstalledFeatures");
    theInstall.DeleteRegValue("RemovedFeatures");

    if (silent)
    {
	theInstall.PostInstallation(NULL);
	
	while(theInstall.IsBusy())
	    Sleep(1500);

	/* Start up IVM */
	if (theInstall.m_StartIVM)
	{
	    SetCurrentDirectory(theInstall.m_installPath + "\\ingres\\bin");
	    theInstall.Execute( "\"" + theInstall.m_installPath + "\\ingres\\bin\\ivm.exe\"", FALSE);
	}

	if (!theInstall.m_postInstRet)
	{
	    return_code=1;
	    return TRUE;
	}
	return FALSE;
    }

    CString s;
#ifdef EVALUATION_RELEASE
    s.LoadString(IDS_SDKTITLE);
#else
    if (theInstall.m_DBATools)
    s.Format(IDS_TITLE_DBA, theInstall.m_installationcode);
    else
    s.Format(IDS_TITLE, theInstall.m_installationcode);
#endif /* EVALUATION_RELEASE */

    CPropSheet dlg(s);
    m_pMainWnd = &dlg;
    dlg.DoModal();

    if (theInstall.m_postInstRet)
    {
	if (theInstall.m_win95 && theInstall.m_reboot) 
	{
	    CWaitCursor	wait;

	    if (!theInstall.m_win95)
	    {
		TOKEN_PRIVILEGES    tp;
		LUID		    luid;
		TOKEN_PRIVILEGES    tpPrevious;
		HANDLE		    hToken = NULL;
		DWORD		    cbPrevious = sizeof(TOKEN_PRIVILEGES);

		OpenProcessToken(GetCurrentProcess(),
				 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				 &hToken);

		if(!LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &luid )) 
		    return FALSE;
		
		/*
		** First pass. Get current privilege setting.
		*/

		tp.PrivilegeCount           = 1;
		tp.Privileges[0].Luid       = luid;
		tp.Privileges[0].Attributes = 0;
		
		AdjustTokenPrivileges(
		    hToken,
		    FALSE,
		    &tp,
		    sizeof(TOKEN_PRIVILEGES),
		    &tpPrevious,
		    &cbPrevious
		    );
		
		/*
		** Second pass. Set privilege based on previous setting.
		*/
		tpPrevious.PrivilegeCount		= 1;
		tpPrevious.Privileges[0].Luid	= luid;
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
		
		AdjustTokenPrivileges(
		    hToken,
		    FALSE,
		    &tpPrevious,
		    cbPrevious,
		    NULL,
		    NULL
		    );
	    }

	    ExitWindowsEx(EWX_REBOOT, 0);
	}
    }
    else
    {
	return_code=1;
	return TRUE;
    }

    if (hSystemPalette)
    {
	HDC hDC=::GetDC(NULL);
	::SelectPalette(hDC,hSystemPalette,FALSE);
	::SelectPalette(hDC,hSystemPalette,TRUE);
	::RealizePalette(hDC);
	::ReleaseDC(NULL,hDC);
    }

    return FALSE;
}

int 
CIipostinstApp::ExitInstance()
{
	char szBuf[1024];

	sprintf(szBuf, "%s\\ingres\\files\\install.log", theInstall.m_installPath);
	
	theInstall.m_logFile=CreateFile(szBuf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL, 0);
	if (theInstall.m_logFile == INVALID_HANDLE_VALUE)
		theInstall.m_logFile=0;
	else
	{
		SetFilePointer(theInstall.m_logFile, 0, 0, FILE_END);
		
		sprintf(szBuf, "ingconfig.exe: RC = %d", return_code);
		theInstall.AppendToLog(szBuf);

		CloseHandle(theInstall.m_logFile);
	}
	
	return return_code;
}

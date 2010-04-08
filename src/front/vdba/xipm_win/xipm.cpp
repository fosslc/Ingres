/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved. 
*/

/*
**    Source   : xipm.cpp  : Implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Defines the class behaviors for the application.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 16-Mar-2004 (uk$so01)
**    BUG #111965 / ISSUE 13296981 The Vdbamon's menu item "Help Topic" is disabled.
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2007 (drivi01)
**    Update the year to 2007.
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 20-Mar-2009 (smeke01) b121832
**    Product year is returned by INGRESII_BuildVersionInfo() so does 
**    not need to be set in here.
*/


#include "stdafx.h"
#include "xipm.h"
#include "mainfrm.h"
#include "ipmdoc.h"
#include "ipmview.h"
#include "libguids.h"
#include "vnodedml.h"
#include "tlsfunct.h"
#include "sessimgr.h"
#include "copyright.h"
extern "C"
{
#include "libmon.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CappIpm, CWinApp)
	//{{AFX_MSG_MAP(CappIpm)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	//}}AFX_MSG_MAP
#if defined (_PRINT_ENABLE_)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappIpm construction

CappIpm::CappIpm()
{
	lstrcpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	_tcscpy (m_tchszIpmProperty, _T("IpmProperty"));
	_tcscpy (m_tchszIpmData,     _T("IpmData"));
	_tcscpy (m_tchszContainerData, _T("ContainerData"));
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappIpm object

CappIpm theApp;

/////////////////////////////////////////////////////////////////////////////
// CappIpm initialization

BOOL CappIpm::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	/* no need for .NET
#ifdef _AFXDLL
	Enable3dControls();       // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
#endif
	*/

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

#if !defined (_DEBUG)
	//
	// The release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba:
	if (!LIBGUIDS_IsCLSIDRegistered(_T("{AB474686-E577-11D5-878C-00C04F1F754A}")))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (_T("ipm.ocx"), NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, _T("ipm.ocx"));
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Seccess:
			break;
		}
	}
#endif
	LIBMON_SetSessionDescription(_T("Ingres Visual Performance Monitor"));
	//
	// Check the if ingres is running:
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_SYSTEM"));
	if (!pEnv)
	{
		AfxMessageBox (IDS_MSG_II_SYSTEM_NOT_DEFINED);
		return FALSE;
	}
	INGRESII_llSetPasswordPrompt();
	CString strCmd = pEnv;
	strCmd += consttchszIngBin;
	strCmd += consttchszWinstart;

	CaIngresRunning ingStart (NULL, strCmd, IDS_MSG_REQUEST_TO_START_INSTALLATION, AFX_IDS_APP_TITLE);
	long nStart = INGRESII_IsRunning(ingStart);
	switch (nStart)
	{
	case 0: // not started
	case 1: // started
		break;
	case 2: // failed to started
		AfxMessageBox (IDS_MSG_INGRES_START_FAILED);
		break;
	default:
		break;
	}
	//
	// Allow to continue to run event if nStart := 1

	// Register document templates
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdIpm),
		RUNTIME_CLASS(CfMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CvIpm));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	// ParseCommandLine(cmdInfo);

	m_cmdLineArg.SetCmdLine(m_lpCmdLine);
	m_cmdLineArg.AddKey(_T("-node"),      TRUE,  TRUE);
	m_cmdLineArg.AddKey(_T("-u"),         FALSE, TRUE);
	m_cmdLineArg.AddKey(_T("-maxapp"),    FALSE, FALSE);
	m_cmdLineArg.AddKey(_T("-noapply"),   FALSE, FALSE);
#if defined (_MIN_TOOLBARS_)
	m_cmdLineArg.AddKey(_T("-mintoolbar"),FALSE, FALSE);
#endif
	int nError = m_cmdLineArg.Parse ();
	if (nError == -1)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


// Menu command to open support site in default browser
void CappIpm::OnHelpOnlineSupport()
{
        ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}


// App command to run the dialog
void CappIpm::OnAppAbout()
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");;
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strDllName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)(LPCTSTR, LPCTSTR, short, UINT);
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "AboutBox");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			CString strTitle;
			CString strAbout;
			int year;
			CString strVer;
			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;
			INGRESII_BuildVersionInfo (strVer, year);

			strAbout.Format (IDS_PRODUCT_VERSION, strVer);
			strTitle.LoadString (AFX_IDS_APP_TITLE);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, (LPCTSTR)strDllName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CappIpm message handlers


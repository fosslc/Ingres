/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xsql.cpp: implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Defines the class behaviors for the application.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
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
#include "xsql.h"
#include "mainfrm.h"
#include "xsqldoc.h"
#include "xsqlview.h"
#include "edbcfram.h"
#include "edbcdoc.h"
#include "libguids.h"
#include "vnodedml.h"
#include "tlsfunct.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CappSqlQuery, CWinApp)
	//{{AFX_MSG_MAP(CappSqlQuery)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	//}}AFX_MSG_MAP

#if defined (_PRINT_ENABLE_)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappSqlQuery construction

CappSqlQuery::CappSqlQuery()
{
	_tcscpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	_tcscpy (m_tchszSqlProperty, _T("SqlProperty"));
	_tcscpy (m_tchszSqlData, _T("SqlData"));
	_tcscpy (m_tchszContainerData, _T("ContainerData"));
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappSqlQuery object

CappSqlQuery theApp;

/////////////////////////////////////////////////////////////////////////////
// CappSqlQuery initialization

BOOL CappSqlQuery::InitInstance()
{
	AfxEnableControlContainer();
	CString strGUID = _T("{634C383D-A069-11D5-8769-00C04F1F754A}");
	CString strOCX  = _T("sqlquery.ocx");
#if defined (EDBC)
	strGUID = _T("{7A9941A3-9987-4914-AB55-F7F630CF8B38}");
	strOCX  = _T("edbquery.ocx");
#endif

	// Standard initialization
	/* No need for .NET
#ifdef _AFXDLL
	Enable3dControls();       // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
#endif
	*/
	CString strTitle;
	strTitle.LoadString(IDS_APP_TITLE_XX);
	free((void*)theApp.m_pszAppName);
	theApp.m_pszAppName =_tcsdup(strTitle);

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

#if !defined (_DEBUG)
	//
	// The release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba:
	if (!LIBGUIDS_IsCLSIDRegistered(strGUID))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (strOCX, NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED_XX);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, strOCX);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Seccess:
			break;
		}
	}
#endif

	//
	// Check the if ingres is running:
	TCHAR* pEnv;
	pEnv = _tgetenv(consttchszII_SYSTEM);
	if (!pEnv)
	{
		AfxMessageBox (IDS_MSG_II_SYSTEM_NOT_DEFINED_XX);
		return FALSE;
	}
	INGRESII_llSetPasswordPrompt();
	CString strCmd = pEnv;
	strCmd += consttchszIngBin;
	strCmd += consttchszWinstart;

	CaIngresRunning ingStart (NULL, strCmd, IDS_MSG_REQUEST_TO_START_INSTALLATION, IDS_APP_TITLE_XX);
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
	// Allow to continue to run event if nStart != 1

	//
	// Register document templates
	CSingleDocTemplate* pDocTemplate;
#if defined (EDBC)
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME_EDBC,
		RUNTIME_CLASS(CdSqlQueryEdbc),
		RUNTIME_CLASS(CfMainFrameEdbc),
		RUNTIME_CLASS(CvSqlQuery));
#else
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdSqlQuery),
		RUNTIME_CLASS(CfMainFrame),
		RUNTIME_CLASS(CvSqlQuery));
#endif
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	// ParseCommandLine(cmdInfo);

	m_cmdLineArg.SetCmdLine(m_lpCmdLine);
#if defined (EDBC)
	m_cmdLineArg.AddKey(_T("-connectinfo"), TRUE,  TRUE);
	m_cmdLineArg.AddKey(_T("-node"),     TRUE,  TRUE);
	m_cmdLineArg.AddKey(_T("-database"), TRUE,  TRUE);
#else
	m_cmdLineArg.AddKey(_T("-node"),     TRUE,  TRUE);
	m_cmdLineArg.AddKey(_T("-database"), TRUE,  TRUE);
	m_cmdLineArg.AddKey(_T("-u"),        FALSE, TRUE);
	#if defined (_OPTION_GROUPxROLE)
	m_cmdLineArg.AddKey(_T("-G"),        FALSE, TRUE);
	m_cmdLineArg.AddKey(_T("-R"),        FALSE, TRUE);
	#endif
	m_cmdLineArg.AddKey(_T("-maxapp"),   FALSE, FALSE);
#endif

	int nError = m_cmdLineArg.Parse ();
	if (nError == -1)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE_XX);
		return FALSE;
	}
	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}



// Menu command to open support ssite in default browser
void CappSqlQuery::OnHelpOnlineSupport()
{
       ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}

// App command to run the dialog
void CappSqlQuery::OnAppAbout()
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");
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
			strTitle.LoadString (IDS_APP_TITLE_XX);
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
// CappSqlQuery message handlers



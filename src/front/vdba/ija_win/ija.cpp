/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ija.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main App IJA (Ingres Journal Analyser)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 22-Mar-2001 (noifr01)
**    removed the warning when starting the product
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 11-Jun-2001
**    SIR #104918, If ijactrl.ocx is not registered then register it.
** 28-Sep-2001 (hanje04)
**    BUG 105941
**    To stop SEGV on UNIX when loading ActiveX object (ijactrl.ocx) at 
**    startup. InitCommonControls must be called during InitInstance.
** 07-Nov-2001 (uk$so01)
**    SIR #106290, new corporate guidelines and standards about box.
**    (the old code of about box and its resource are not cleaned yet because
**     of the ivm.dsp contains the 64bits target and we don't want this change)
** 29-Jan-2003 (noifr01)
**    (SIR 108139) get the version string and year by parsing information 
**    from the gv.h file 
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Use the aboutbox provided by ijactrl.ocx. and remove
**    the local about.bmp.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 06-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale,
**    for sorting object name in tree and Rigth pane list.
** 10-Mar-2005 (nansa02) 
**    Bug # 113544 Help window in IJA should always start with
**    contents page.Similar to change 473497.
** 11-May-2007 (karye01)
**    SIR #118282, added Help menu item for support url.
**/

#include "stdafx.h"
#include <htmlhelp.h>
#include "ija.h"
#include "mainfrm.h"
#include "maindoc.h"
#include "viewleft.h"
#include "viewrigh.h"
#include "ijadml.h"
#include "libguids.h"
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CappIja, CWinApp)
	//{{AFX_MSG_MAP(CappIja)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	//ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappIja construction

CappIja::CappIja()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappIja object

CappIja theApp;

/////////////////////////////////////////////////////////////////////////////
// CappIja initialization

BOOL CappIja::InitInstance()
{
	AfxEnableControlContainer();
	_tsetlocale(LC_COLLATE  ,_T(""));
	_tsetlocale(LC_CTYPE  ,_T(""));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	/* no need in .NET
#ifdef _AFXDLL
	Enable3dControls();         // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif
	*/

#ifdef MAINWIN
	InitCommonControls();
#endif
	m_sessionManager.SetDescription(_T("Ingres Journal Analyzer"));
	//
	// The release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba:
	if (!LIBGUIDS_IsCLSIDRegistered(_T("{C92B8427-B176-11D3-A322-00C04F1F754A}")))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (_T("ijactrl.ocx"), NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, _T("ijactrl.ocx"));
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Seccess:
			break;
		}
	}
	INGRESII_llSetPasswordPrompt();



	m_strAllUser.LoadString (IDS_ALLUSER);
	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	CString strCmdLine = m_lpCmdLine;
	if (m_cmdLine.Parse (strCmdLine) == -1)
	{
		CString strMsg;
		if (!strMsg.LoadString (IDS_INVALID_CMDLINE))
		{
			strMsg = _T("Invalid command line.\nThe syntax is:\n");
			strMsg+= _T("Ija.exe \t[-node=<node name> [-database=<database name>  [-table=<owner>.<table name>]]]\n\t");
			strMsg+= _T("[-splitleft] [-notoolbar] [-nostatusbar]");
		}
		AfxMessageBox (strMsg);
	}
	//
	// Check to see if specified Star database at the command line:
	if (!m_strMsgErrorDBStar.LoadString(IDS_ERROR_DATABASE_STAR))
		m_strMsgErrorDBStar = _T("Ija does not accept the Star Database");
	switch (theApp.m_cmdLine.m_nArgs)
	{
	case 2:
	case 3:
		{
			CaLLQueryInfo info (-1, theApp.m_cmdLine.m_strNode, _T("iidbdb"));
			if (INGRESII_llGetTypeDatabase(theApp.m_cmdLine.m_strDatabase, &info, &m_sessionManager) == OBJTYPE_STARLINK)
			{
				AfxMessageBox (m_strMsgErrorDBStar);
				return FALSE;
			}
		}
		break;
	default:
		break;
	}


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdMainDoc),
		RUNTIME_CLASS(CfMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CvViewLeft));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	
	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();


	return TRUE;
}


// Menu command to open support site in default browser
void CappIja::OnHelpOnlineSupport()
{
     ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}

// App command to run the dialog
void CappIja::OnAppAbout()
{
	if (m_pMainWnd && IsWindow (m_pMainWnd->m_hWnd))
	{
		CfMainFrame* pFrame = (CfMainFrame*)m_pMainWnd;
		CvViewRight* pView = (CvViewRight*)pFrame->GetRightView();
		if (pView)
			pView->OnAbout();
	}
}


BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
	CString strHelpFile = _T("ija.chm");

#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += strHelpFile;
	}
	else
		strHelpFilePath = strHelpFile;
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);

	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CappIja commands

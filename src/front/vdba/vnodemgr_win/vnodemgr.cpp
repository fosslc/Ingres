/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : vnodemgr.cpp, Implementation File
**
**
**  Project  : Virtual Node Manager.
**  Author   : UK Sotheavut
**
**  Purpose  : Main Application
**  History  :
**   11-jun-1999 (mcgem01)
**     Only use "Gateway" terminology for EDBC.
**   14-may-2001 (zhayu01) SIR 104724
**     Use "Server" instead of "Node" for EDBC.
**   22-Dec-2000 (noifr01)
**   (SIR 103548) appended the installation ID in the application title.
**   07-Nov-2001 (uk$so01)
**    SIR #106290, new corporate guidelines and standards about box.
**   29-Jan-2003 (noifr01)
**    (SIR 108139) get the version string and year by parsing information 
**    from the gv.h file 
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 13-Oct-2003 (schph01)
**    SIR 109864 add -vnode command line option
** 09-Feb-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Now ingnet uses ingnet.chm instead of vdba.chm 
**    Also eliminate some warnings under .NET
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 08-Jan-2007 (drivi01)
**    Updated the year to 2007.
** 11-May-2007 (karye01)
**    SIR #118282, added menu item for support url.
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
#include "vnodemgr.h"
#include "mainfrm.h"
#include "maindoc.h"
#include "mainview.h"
#include "rcdepend.h"
#include "tlsfunct.h"
#include <htmlhelp.h>
#include <locale.h>
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CappVnodeManager

BEGIN_MESSAGE_MAP(CappVnodeManager, CWinApp)
	//{{AFX_MSG_MAP(CappVnodeManager)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappVnodeManager construction

CappVnodeManager::CappVnodeManager()
{
	m_pNodeAccess = NULL;
	m_pDocument = NULL;
	m_strMessage.GetBuffer (512);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappVnodeManager object

CappVnodeManager theApp;

/////////////////////////////////////////////////////////////////////////////
// CappVnodeManager initialization

void CappVnodeManager::InitStrings()
{
#ifdef EDBC
	if (!m_strFolderNode.LoadString (IDS_FOLDER_SERVER))
		m_strFolderNode = _T("Servers");
#else
	if (!m_strFolderNode.LoadString (IDS_FOLDER_NODE))
		m_strFolderNode = _T("Nodes");
#endif
#ifdef EDBC
    if (!m_strFolderServer.LoadString (IDS_FOLDER_GATEWAY))
		m_strFolderServer = _T("Gateways");
#else
	if (!m_strFolderServer.LoadString (IDS_FOLDER_SERVER))
		m_strFolderServer = _T("Servers");
#endif
	if (!m_strFolderLogin.LoadString (IDS_FOLDER_LOGIN))
		m_strFolderLogin = _T("Logins");
	if (!m_strFolderConnection.LoadString (IDS_FOLDER_CONNECTION))
		m_strFolderConnection = _T("Connection Data");
	if (!m_strFolderAttribute.LoadString (IDS_FOLDER_ATTRIBUTE))
		m_strFolderAttribute = _T("Attribute");
#ifdef EDBC
	if (!m_strFolderAdvancedNode.LoadString (IDS_FOLDER_ADVANCEDSERVER))
		m_strFolderAdvancedNode = _T("Advanced Server Parameters");
#else
	if (!m_strFolderAdvancedNode.LoadString (IDS_FOLDER_ADVANCEDNODE))
		m_strFolderAdvancedNode = _T("Advanced Node Parameters");
#endif
	if (!m_strUnavailable.LoadString (IDS_UNAVAILABLE))
		m_strUnavailable = _T("<Data Unavailable>");
#ifdef EDBC
	if (!m_strNoNodes.LoadString (IDS_NOSERVERS))
		m_strNoNodes = _T("<No Servers>");
#else
	if (!m_strNoNodes.LoadString (IDS_NONODES))
		m_strNoNodes = _T("<No Nodes>");
#endif

	m_strNodeLocal= _T("(local)");              // Local node name
	m_strInstallPath = _T("");                  // Installation path
	m_strHelpFile    = HELPFILE;                // help file
}

BOOL CappVnodeManager::InitInstance()
{

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	/*
#ifdef _AFXDLL
	Enable3dControls();       // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
#endif
	*/
	
	_tsetlocale(LC_COLLATE ,"");
	_tsetlocale(LC_CTYPE ,"");

	int nIDSMsg = IDS_MSG_II_SYSTEM_NOT_DEFINED;
	int nIDSInstallName = IDS_INGRES;

	if (m_pszAppName)
		free((void*)m_pszAppName);
	CString strTitle;
	CString strInstallName;
	UINT nIDSTitle = IDS_APP_TITLE;
#ifdef EDBC
	if (!strTitle.LoadString (AFX_IDS_APP_TITLE))
		strTitle = _T("EDBC Network Utility");
	nIDSTitle = AFX_IDS_APP_TITLE;
	nIDSMsg   = IDS_MSG_EDBCROOT_NOT_DEFINED;
	nIDSInstallName = IDS_EDBC;
#else
	if (!strTitle.LoadString (IDS_APP_TITLE))
		strTitle = _T("Ingres Network Utility");
#endif

	strInstallName.LoadString(nIDSInstallName);
	strTitle += INGRESII_QueryInstallationID();
	m_pszAppName =_tcsdup((LPCTSTR)strTitle);
	
	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	InitStrings();

	//
	// Check the if ingres is running:
	TCHAR* pEnv;
	pEnv = _tgetenv(consttchszII_SYSTEM);
	if (!pEnv)
	{
		AfxMessageBox (nIDSMsg);
		return FALSE;
	}
	m_strInstallPath = pEnv;

	CString strMsg;
	CString strCmd = pEnv;
	strCmd += consttchszIngBin;
	strCmd += consttchszWinstart;
	CString strRequest;
	AfxFormatString1(strRequest, IDS_REQUEST2START_INSTALLATION, (LPCTSTR)strInstallName);

	CaIngresRunning ingStart (NULL, strCmd, strRequest, strTitle);
	long nStart = INGRESII_IsRunning(ingStart);
	switch (nStart)
	{
	case 0: // not started
		return FALSE;
	case 1: // started
		break;
	case 2: // failed to started
		AfxFormatString1(strMsg, IDS_MSG_INSTALLATION_START_FAILED, (LPCTSTR)strInstallName);
		AfxMessageBox (strMsg);
		return FALSE;
	default:
		break;
	}

	m_pNodeAccess = new CaNodeDataAccess();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdMainDoc),
		RUNTIME_CLASS(CfMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CvMainView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	m_cmdLine.AddKey(_T("-vnode"), TRUE,  TRUE);

	m_cmdLine.SetCmdLine(m_lpCmdLine);
	int nError = m_cmdLine.Parse ();
	if (nError == -1)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}
	m_cmdLine.HandleCommandLine();

	if (!m_cmdLine.m_strNode.IsEmpty())
	{
		// Initialize hostname Variable for Low Level connection
		m_pNodeAccess->InitHostName(m_cmdLine.m_strNode);
		CString csTempo;
		// Update the Application title
		AfxFormatString1(csTempo, IDS_VNODELOCALTITLE, m_cmdLine.m_strNode);
		strTitle += csTempo;
		if (m_pszAppName)
			free((void*)m_pszAppName);
		m_pszAppName =_tcsdup((LPCTSTR)strTitle);
		// Change the tree node title
		AfxFormatString1(csTempo, IDS_FOLDER_NET_NODE, m_cmdLine.m_strNode);
		m_strFolderNode += csTempo;
	}

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->SetWindowText((LPCTSTR)m_pszAppName);
	//
	// Set the Initial Window Size (cx, cy): (reduce by 50% and 20%)
	CRect r;
	m_pMainWnd->GetWindowRect (r);
	int x = r.Width() - (int)((double)r.Width() *0.5);
	int y = r.Height()- (int)((double)r.Height()*0.2);
	m_pMainWnd->SetWindowPos (NULL, 0, 0, x, y, SWP_NOMOVE|SWP_NOZORDER);

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


// Menu command to open support site in default browser
void CappVnodeManager::OnHelpOnlineSupport()
{
       ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}

// App command to run the dialog
void CappVnodeManager::OnAppAbout()
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
			CString strAbout;
			CString strTitle;
			int year;
			CString strVer;
			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;
#if defined (EDBC)
			strAbout.LoadString (IDS_PRODUCT_VERSION_EDBC);
			strTitle.LoadString (AFX_IDS_APP_TITLE);
#else
			INGRESII_BuildVersionInfo (strVer, year);
			strAbout.Format (IDS_PRODUCT_VERSION, (LPCTSTR)strVer);
			strTitle.LoadString (IDS_APP_TITLE);
#endif
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, strDllName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CappVnodeManager commands

int CappVnodeManager::ExitInstance() 
{
	if (m_pNodeAccess)
		delete m_pNodeAccess;
	return CWinApp::ExitInstance();
}

BOOL APP_HelpInfo(UINT nContextHelpID)
{
	TRACE1 (_T("APP_HelpInfo: %d\n"), nContextHelpID);

	int  pos = 0;
	CString strHelpFilePath, strTemp;
	strTemp = theApp.m_pszHelpFilePath;
	pos = strTemp.ReverseFind( _T('\\'));
	if ( pos != -1) 
	{
		strHelpFilePath = strTemp.Left (pos +1);
		strHelpFilePath += theApp.m_strHelpFile;
	}
	else
		strHelpFilePath = theApp.m_strHelpFile;
	BOOL bHelpAvailable = theApp.m_strHelpFile.IsEmpty()? FALSE: TRUE;

	// noifr01 18-Jun-99: remapped the application help to the "books" stuff
	if (nContextHelpID == IDHELP_MAIN_APP)
		nContextHelpID = 0;

	if (bHelpAvailable && nContextHelpID > 0)
		HtmlHelp(theApp.m_pMainWnd->m_hWnd, strHelpFilePath, HH_HELP_CONTEXT, nContextHelpID);
	else
		HtmlHelp(theApp.m_pMainWnd->m_hWnd, strHelpFilePath, HH_HELP_CONTEXT, 50000 );//ingnet_overview
	return TRUE;
}

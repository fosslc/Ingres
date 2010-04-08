/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsda.cpp : Defines the class behaviors for the application.
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main application vdda.exe (container of vsda.ocx)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 17-May-2004 (uk$so01)
**    SIR #109220, Eliminate the compiler warnings
** 06-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
*/

#include "stdafx.h"
#include "vsda.h"
#include "mainfrm.h"
#include "vsdadoc.h"
#include "vsdaview.h"
#include "constdef.h"
#include "vnodedml.h"
#include "libguids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CappVsda, CWinApp)
	//{{AFX_MSG_MAP(CappVsda)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappVsda construction

CappVsda::CappVsda()
{
	_tcscpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	m_sessionManager.SetSessionStart(2000);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappVsda object

CappVsda theApp;

/////////////////////////////////////////////////////////////////////////////
// CappVsda initialization

BOOL CappVsda::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	/* No need in .NET
#ifdef _AFXDLL
	Enable3dControls();       // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
#endif
	*/

	INGRESII_llSetPasswordPrompt();
	//
	// Check the if ingres is running:
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_SYSTEM"));
	if (!pEnv)
	{
		AfxMessageBox (IDS_MSG_II_SYSTEM_NOT_DEFINED);
		return FALSE;
	}

	CString strCmd = pEnv;
	strCmd += consttchszIngBin;
	strCmd += consttchszWinstart;
	CaIngresRunning ingStart (NULL, strCmd, IDS_MSG_REQUEST_TO_START_INSTALLATION, AFX_IDS_APP_TITLE);
	long nStart = INGRESII_IsRunning(ingStart);
	switch (nStart)
	{
	case 0: // not started
		break;;
	case 1: // started
		break;
	case 2: // failed to started
		AfxMessageBox (IDS_MSG_INGRES_START_FAILED);
		return FALSE;
	default:
		break;
	}


	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
#if !defined (_DEBUG)
	if (!LIBGUIDS_IsCLSIDRegistered(_T("{CC2DA2B6-B8F1-11D6-87D8-00C04F1F754A}")))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (_T("vsda.ocx"), NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, _T("vsda.ocx"));
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Success:
			break;
		}
	}
#endif
	m_sessionManager.SetDescription(_T("Ingres Visual Database Objects Differences Analyzer"));

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdSda),
		RUNTIME_CLASS(CfSda),       // main SDI frame window
		RUNTIME_CLASS(CvSda));
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




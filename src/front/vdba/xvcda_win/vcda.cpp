/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcda.cpp : Defines the class behaviors for the application.
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application InitInstance
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
**/

#include "stdafx.h"
#include "vcda.h"
#include "mainfrm.h"
#include "vcdadoc.h"
#include "vcdaview.h"
#include "libguids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CappVCDA

BEGIN_MESSAGE_MAP(CappVCDA, CWinApp)
	//{{AFX_MSG_MAP(CappVCDA)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappVCDA construction

CappVCDA::CappVCDA()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappVCDA object

CappVCDA theApp;

/////////////////////////////////////////////////////////////////////////////
// CappVCDA initialization

BOOL CappVCDA::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	/* no need in .NET
#ifdef _AFXDLL
	Enable3dControls();        // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();  // Call this when linking to MFC statically
#endif
	*/

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	//
	// The release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba:
#if !defined (_DEBUG)
	if (!LIBGUIDS_IsCLSIDRegistered(_T("{EAF345EA-D514-11D6-87EA-00C04F1F754A}")))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (_T("vcda.ocx"), NULL);
		switch (nRes)
		{
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, _T("vcda.ocx"));
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Success:
			break;
		}
	}
#endif


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CdCda),
		RUNTIME_CLASS(CfCda),       // main SDI frame window
		RUNTIME_CLASS(CvCda));
	AddDocTemplate(pDocTemplate);

	m_cmdArg.SetCmdLine(m_lpCmdLine);
	m_cmdArg.AddKey(_T("-c"), FALSE, FALSE);
	int nError = m_cmdArg.Parse ();
	if (nError == -1)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	// Ignore the standard argument line process !!!
	// ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CappVCDA message handlers


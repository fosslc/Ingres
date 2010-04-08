/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : iia.cpp : Defines the class behaviors for the application.
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 07-Feb-2001 (uk$so01)
**    Created
** 18-Jul-2002 (hanje04)
**    Bug 108296
**    To stop IIA crashing on UNIX when loading libsvriia.so for the first 
**    time we make a dummy call to InitCommonControls() to force IIA to link
**    to libcomctl32.so.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Enhance library.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, cleanup.
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
**/

#include "stdafx.h"
#include "iia.h"
#include "mainfrm.h"
#include "libguids.h"
#include "constdef.h"
#include "vnodedml.h"
#include "sessimgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CappImportAssistant, CWinApp)
	//{{AFX_MSG_MAP(CappImportAssistant)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CappImportAssistant::CappImportAssistant()
{
}


CappImportAssistant theApp;


BOOL CappImportAssistant::InitInstance()
{
	AfxEnableControlContainer();
	/* no need in .NET
#ifdef _AFXDLL
	Enable3dControls();
#else
	Enable3dControlsStatic();
#endif
	*/

	CString strLibraryName = _T("svriia.dll");
#if defined (MAINWIN)
	InitCommonControls();
	#if defined (hpb_us5)
	strLibraryName  = _T("libsvriia.sl");
	#else
	strLibraryName  = _T("libsvriia.so");
	#endif
#endif

	//
	// If the server is not registered, the release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba
#if !defined (_DEBUG)
	if (!LIBGUIDS_IsCLSIDRegistered(cstrCLSID_IMPxEXPxASSISTANCT))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (strLibraryName, NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, strLibraryName);
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
		return FALSE;
	case 1: // started
		break;
	case 2: // failed to started
		AfxMessageBox (IDS_MSG_INGRES_START_FAILED);
		return FALSE;
	default:
		break;
	}

	m_cmdLine.AddKey(_T("-node"),          TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-database"),      TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-table"),         TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-batch"),         FALSE, FALSE);
	m_cmdLine.AddKey(_T("-log"),           TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-createdtable"),  TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-loop"),          FALSE, FALSE);

	m_cmdLine.SetCmdLine(m_lpCmdLine);
	int nError = m_cmdLine.Parse ();
	if (nError == -1)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}
	m_cmdLine.HandleCommandLine();

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CfMainFrame* pFrame = new CfMainFrame;
	m_pMainWnd = pFrame;

	//
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);

	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	HICON hIc = LoadIcon(IDR_MAINFRAME);
	pFrame->SetIcon (hIc, TRUE);
	DestroyIcon (hIc);

	return TRUE;
}



int CappImportAssistant::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

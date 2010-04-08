/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iea.cpp : Defines the class behaviors for the application.
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application
**
** History:
**	16-Oct-2001 (uk$so01)
**	    Created
**	17-Dec-2002 (uk$so01)
**	    SIR #109220, Enhance the library.
**	26-Feb-2003 (uk$so01)
**	    SIR  #106952, Date conversion and numeric display format + enhance
**	    library.
**	17-Jul-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, cleanup. 
**	22-Aug-2003 (uk$so01)
**	    SIR #106648, Add silent mode in IEA. 
**	02-Feb-2004 (uk$so01)
**	    SIR #106648, Vdba-Split, -loop option. 
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**	    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**	    Write()/Read() can also write more than 64K-1 bytes of data.
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
*/


#include "stdafx.h"
#include "iea.h"
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


BEGIN_MESSAGE_MAP(CappExportAssistant, CWinApp)
	//{{AFX_MSG_MAP(CappExportAssistant)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappExportAssistant construction

CappExportAssistant::CappExportAssistant()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CappExportAssistant object

CappExportAssistant theApp;

/////////////////////////////////////////////////////////////////////////////
// CappExportAssistant initialization

BOOL CappExportAssistant::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	/* no need for .NET
#ifdef _AFXDLL
	Enable3dControls();       // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
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
	// Standard command line options:
#if defined (_IEA_STANDARD_OPTION)
	m_cmdLine.AddKey(_T("-node"),          TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-database"),      TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-statement"),     TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-exportmode"),    TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-exportfile"),    TRUE,  TRUE);
#endif
	//
	// Silent mode compatible options:
	m_cmdLine.AddKey(_T("-loop"),          FALSE, FALSE);
	m_cmdLine.AddKey(_T("-silent"),        FALSE, FALSE);
	m_cmdLine.AddKey(_T("-logfilename"),   TRUE,  TRUE);
	m_cmdLine.AddKey(_T("-l"),             FALSE, TRUE);
	m_cmdLine.AddKey(_T("-o"),             FALSE, FALSE);

	m_cmdLine.SetCmdLine(m_lpCmdLine);
	int nError = m_cmdLine.Parse ();
	if (nError == -1)
	{
		//
		// Invalid command line. The syntax is:
		// \niea.exe \t[-node=<node name> [/serverclass] [-database=<database name> [-statement="<select statement>"]]]
		//         \n\t[ -exportmode= csv | xml | dbf | fixed | copy] [-exportfile=<file name> ] [-loop]
		// \niea.exe \t[<filename.ii_exp>] [-loop]
		// \niea.exe \t-silent <filename.ii_exp> | -l<listfile> [-logfilename=<logfile.txt>] [-o]
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}
	if (!m_cmdLine.HandleCommandLine())
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}
	//
	// Check the semantic of command lines:
	// NOTE:
	//  -silent:  valide only if the file of a .ii_exp or -l<file> option is specified and no 
	//            other standard command line options are specified
	//  -logfilename, -l, -o are valid only if -silent is specified.
	BOOL bB1OK = TRUE;
	if (bB1OK && !m_cmdLine.m_strIIParamFile.IsEmpty() && !m_cmdLine.m_strListFile.IsEmpty())
		bB1OK = FALSE;
	if (bB1OK && m_cmdLine.m_bBatch && m_cmdLine.m_strIIParamFile.IsEmpty() && m_cmdLine.m_strListFile.IsEmpty())
		bB1OK = FALSE;
	if ( !m_cmdLine.m_bBatch && 
	    (!m_cmdLine.m_strLogFile.IsEmpty() || !m_cmdLine.m_strListFile.IsEmpty() || m_cmdLine.m_bOverWrite))
		bB1OK = FALSE;
	if (!bB1OK)
	{
		AfxMessageBox (IDS_INVALID_CMDLINE);
		return FALSE;
	}

	//
	// If specified logfile, first create and make it empty:
	CString strLogFile = m_cmdLine.m_strLogFile;
	if (!strLogFile.IsEmpty())
	{
		CFile f (strLogFile, CFile::modeCreate|CFile::modeWrite);
		f.Close();
	}

	//
	// If the server is not registered, the release version will try to automatically
	// register the Server in the II_SYSTEM\ingres\vdba
#if !defined (_DEBUG)
	if (!LIBGUIDS_IsCLSIDRegistered(cstrCLSID_IMPxEXPxASSISTANCT))
	{
		CString strLogError = _T("");
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (strLibraryName, NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			if (!m_cmdLine.m_bBatch)
				AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			else
			{
				strLogError += strMsg;
				strLogError += consttchszReturn;
				strLogError += consttchszReturn;
			}
			bOK = FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, strLibraryName);
			if (!m_cmdLine.m_bBatch)
				AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			else
			{
				strLogError += strMsg;
				strLogError += consttchszReturn;
				strLogError += consttchszReturn;
			}
			bOK = FALSE;
		case 0:
			// Seccess:
			break;
		}
		
		if (!strLogError.IsEmpty() && !strLogFile.IsEmpty())
		{
			CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
			f.SeekToEnd();
			f.Write((LPCTSTR)strLogError, strLogError.GetLength());
			f.Close();
		}
		if (!bOK)
			return FALSE;
	}
#endif

	//
	// Check the if ingres is running:
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_SYSTEM"));
	if (!pEnv)
	{
		CString strLogError = _T("");
		CString strMsg;
		strMsg.LoadString(IDS_MSG_II_SYSTEM_NOT_DEFINED);
		if (!m_cmdLine.m_bBatch)
			AfxMessageBox (strMsg);
		else
		{
			strLogError += strMsg;
			strLogError += consttchszReturn;
			strLogError += consttchszReturn;

			if (!strLogFile.IsEmpty())
			{
				CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
				f.SeekToEnd();
				f.Write((LPCTSTR)strLogError, strLogError.GetLength());
				f.Close();
			}
		}
		return FALSE;
	}
	INGRESII_llSetPasswordPrompt();
	CString strCmd = pEnv;
	strCmd += consttchszIngBin;
	strCmd += consttchszWinstart;

	if (!m_cmdLine.m_bBatch)
	{
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
	}
	else
	{
		if (!INGRESII_IsRunning())
		{
			CString strLogError = _T("Ingres Installation has not been started.");
			strLogError += consttchszReturn;
			strLogError += consttchszReturn;
			if (!strLogFile.IsEmpty())
			{
				CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
				f.SeekToEnd();
				f.Write((LPCTSTR)strLogError, strLogError.GetLength());
				f.Close();
			}
			return FALSE;
		}
	}

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));


	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	CfMainFrame* pFrame = new CfMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	HICON hIc = LoadIcon(IDR_MAINFRAME);
	pFrame->SetIcon (hIc, TRUE);
	DestroyIcon (hIc);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CappExportAssistant message handlers






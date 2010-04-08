/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp, Implementation file
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main Frame of the application
**
** History:
**
** 14-Dec-1998 (uk$so01)
**    Created
** 27-Jan-2000 (uk$so01)
**     (Bug #100157): Activate Help Topic
** 28-Jan-2000 (noifr01)
**     (Bug 100197) removed the test, when exiting from IVM,  whether 
**	Ingres is started but not as a service, since Ingres is now 
**	always started as a service. (the test was already removed for UNIX,
**	it now is dependent in addition on the OPING20 definition)
** 31-Jan-2000 (noifr01)
**     (SIR 100237) (reenabled the logic where IVM manages the fact that
**	Ingres may be started as a service or not). Reenabled the test
**	(removed in the previous change) when exiting IVM...
** 02-02-2000 (noifr01)
**     (SIR 100311) changed the warning message, when running under Windows
**	95/98, when exiting IVM while the ingres installation is still running
** 02-Feb-2000 (uk$so01)
**     (Sir #100316)
**     Provide the command line "-stop" to stop ivm from the prompt: ivm -stop.
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 02-Mar-2000 (uk$so01)
**    SIR #100690, If user runs the second Ivm than activate the first one.
** 03-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 06-Mar-2000 (uk$so01)
**    SIR #100635, Additional fix the Functionality of Find operation.
** 06-Mar-2000 (uk$so01)
**    SIR #100738, Use the flat toolbar.
** 06-Mar-2000 (uk$so01)
**    BUG #100746, Update to the detection of NT shut down to exit ivm properly.
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 27-jun-2000 (somsa01)
**    Removed m_pDlgFind and added it here as a static.
** 05-Jul-2000 (uk$so01)
**    BUG #102012
**    Add the test if (m_bExit) return; in OnHaltProgram()
** 06-Jul-2000 (uk$so01)
**    BUG #102020
**    Remove the Sleep() in the function OnClose().
**    One minute wait for the Threads to terminate is sufficient, so that
**    if in case of deadlock, the application can always exit.
** 07-Jul-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one IVM per installation ID
** 18-Apr-2001 (noifr01)
**    (bug 102020) don't try any more to shutdown ingres (if not started as a
**    service) upon the QUERYENDSESSION nor ENDSESSION, since it seems that the
**    dbms processes may get killed anyhow by the OS. Just store the "read events"
**    information to the disk upon the QUERYENDSESSION
**	27-Jul-2001 (hanje04)
**	    For MAINWIN moved declaration of  m_findInfo to mainfrm.cpp
**	    from mainfrm.h as static.
** 20-Aug-2001 (uk$so01)
**    Fix BUG #105382, Incorrectly notify an alert message when there are no 
**    events displayed in the list.
** 07-Sep-2001 (uk$so01)
**    Fix BUG #105703, "Status" right pane tabs, the start/stop Event history
**    list blinks periodically (every 4 or 5 seconds)
** 17-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client".
**    17-Sep-2001 (hanje04)
**	BUG 105367
**	Set AfxGetApp()->m_pMainWnd=this at the end of OnCreate to prevent
**	problems with Find button in 'Catagories and Notification Levels'
**	window.
**	Also backed out above change to remove declaration of m_findInfo
**	from mainfrm.h
** 27-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client". (No Start Client if Win9x)
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 04-Jun-2002 (hanje04)
**     Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**     unwanted ^M's in generated text files.
** 17-Jun-2002 (uk$so01)
**    BUG #107930, fix bug of displaying the message box when the number of
**    events reached the limit of setting.
** 19-Jun-2002 (uk$so01)
**    BUG #107930, continue to display the message (if later after changing
**    the preferences, the number of events reach the limit) even if the flag
**    in the .ini file indicates not to do so.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 10-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    Update the previous merge on change #462290
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 13-Mar-2003 (uk$so01)
**    SIR #109775, Additional update to change #462359
** 20-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description of ingres messages.
**    More change: make window visible before sending the WMUSRMSG_UPDATEDATA by 
**    calling m_pPropFrame->HandleData(...) otherwise the data won't be displayed.
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
** 27-Apr-2004 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu.
**    (additional changes: add new menu items and change captions)
** 11-Aug-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Add a menu item "Ingres Command Prompt" in Ivm's taskbar icon menu.
** 19-Oct-2004 (uk$so01)
**    SIR #113260 / ISSUE 13730838. Add a command line -NOMAXEVENTSREACHEDWARNING
**    to disable the Max Event reached MessageBox.
** 23-Feb-2005 (penga03)
**    Bug #113942. "Ingres Documentation" menu should not show up if no documents 
**    installed.
** 04-March-2005 (penga03)
**    Added back the shortcut for Ingres Readme File. 
** 31-Mar-2006 (drivi01)
**    Bug #115919
**    "Ingres User Defined Data Type Linker" should not appear in the ivm menu
**    if the udt binary wasn't installed.
** 07-Aug-2007 (drivi01)
**    Added functions to help display icons in the File menu for start,
**    start client, and stop selections. The icons will indicate 
**    elevation on Vista.
** 23-Oct-2008 (drivi01)
**    For a popup menu, add routines to check if perfwiz.exe was installed
**    if it wasn't, remove the shortcut for perfwiz from popup menu.
**    
**/
#include "stdafx.h"
#include "constdef.h"
#include "ivm.h"
#include "wmusrmsg.h"
#include "mainfrm.h"
#include "mainview.h"
#include "dgnewmsg.h"
#include "dlgmsg.h"
#include "ivmdml.h"
#include "regsvice.h"
#include "ppmsgdes.h"
#include ".\mainfrm.h"
#include <compat.h>
#include <gvcl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static CFindReplaceDialog* pDlgFind;


UINT CfMainFrame::WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);


class CaMsgExceedMax: public CaMessageBoxParam
{
public:
	CaMsgExceedMax(LPCTSTR lpszMessage, HANDLE hEvent):CaMessageBoxParam(lpszMessage)
	{
		m_hEvent = hEvent;
		if (m_hEvent)
			ResetEvent(m_hEvent);
	}
	virtual ~CaMsgExceedMax()
	{
		if (m_hEvent)
			SetEvent(m_hEvent);
	}
	virtual void EndMessage();

protected:
	HANDLE m_hEvent;
};

#define GENERAL_INGRES_STATE             _T("INGRES STATE")
#define GENERAL_INGRES_MAXEVENT_MSGBOX   _T("Ingres.Continue Msg Reach max event")
#define GENERAL_INGRES_MAXEVENT_DEF      TRUE

void CaMsgExceedMax::EndMessage()
{
	//
	// Serialize state of the checkbox: ivm.ini
	// [INGRES STATE]
	// Ingres.Continue Msg Reach max event=0|1
	BOOL bChecked = IsChecked();
	CString strAch;
	CString strFile;
	if (strFile.LoadString (IDS_INIFILENAME) == 0)
		return;

	strAch.Format (_T("%d"), (int)bChecked);
	WritePrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_MAXEVENT_MSGBOX, strAch, strFile);
}


/////////////////////////////////////////////////////////////////////////////
// CfMainFrame

IMPLEMENT_DYNCREATE(CfMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CfMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAll)
	ON_COMMAND(ID_START, OnStart)
	ON_COMMAND(ID_START_CLIENT, OnStartClient)
	ON_COMMAND(ID_STOP, OnStop)
	ON_COMMAND(ID_HALT_PROGRAM, OnHaltProgram)
	ON_COMMAND(ID_ACTIVATE_PROGRAM, OnActivateProgram)
	ON_COMMAND(ID_IGNORE, OnIgnore)
	ON_UPDATE_COMMAND_UI(ID_START, OnUpdateStart)
	ON_UPDATE_COMMAND_UI(ID_START_CLIENT, OnUpdateStartClient)
	ON_UPDATE_COMMAND_UI(ID_STOP, OnUpdateStop)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_CONFIGURE, OnConfigure)
	ON_COMMAND(ID_COLLAPSE_ALL, OnCallapseAll)
	ON_COMMAND(ID_VISUALDBA, OnVisualDBA)
	ON_COMMAND(ID_FIND, OnFind)
	ON_UPDATE_COMMAND_UI(ID_FIND, OnUpdateFind)
	ON_WM_QUERYENDSESSION()
	ON_COMMAND(ID_MESSAGE_DESCRIPTION, OnMessageDescription)
	ON_COMMAND(ID_NETWORKUTILITY, OnNetworkUtility)
	ON_COMMAND(ID_SQLQUERY, OnSqlquery)
	ON_COMMAND(ID_IPM, OnIpm)
	ON_COMMAND(ID_IIA, OnIia)
	ON_COMMAND(ID_IEA, OnIea)
	ON_COMMAND(ID_VCDA, OnVcda)
	ON_COMMAND(ID_VDDA, OnVdda)
	ON_COMMAND(ID_IJA, OnIja)
	ON_COMMAND(ID_IILINK, OnIilink)
	ON_COMMAND(ID_PERFORMANCE_WIZARD, OnPerformanceWizard)
	ON_COMMAND(ID_INGRES_CACHE, OnIngresCache)
	ON_COMMAND(ID_ERROR_LOG, OnErrorLog)
	ON_COMMAND(ID_README, OnReadme)
	ON_COMMAND(ID_IGRESDOC_CMDREF, OnIgresdocCmdref)
	ON_COMMAND(ID_IGRESDOC_DBA, OnIgresdocDba)
	ON_COMMAND(ID_IGRESDOC_DTP, OnIgresdocDtp)
	ON_COMMAND(ID_IGRESDOC_ESQLC, OnIgresdocEsqlc)
	ON_COMMAND(ID_IGRESDOC_EQUEL, OnIgresdocEquel)
	ON_COMMAND(ID_IGRESDOC_GS, OnIgresdocGs)
	ON_COMMAND(ID_IGRESDOC_ICEUG, OnIgresdocIceug)
	ON_COMMAND(ID_IGRESDOC_MG, OnIgresdocMg)
	ON_COMMAND(ID_IGRESDOC_OAPI, OnIgresdocOapi)
	ON_COMMAND(ID_IGRESDOC_OME, OnIgresdocOme)
	ON_COMMAND(ID_IGRESDOC_OSQL, OnIgresdocOsql)
	ON_COMMAND(ID_IGRESDOC_QRYTOOLS, OnIgresdocQrytools)
	ON_COMMAND(ID_IGRESDOC_QUELREF, OnIgresdocQuelref)
	ON_COMMAND(ID_IGRESDOC_REP, OnIgresdocRep)
	ON_COMMAND(ID_IGRESDOC_SQLREF, OnIgresdocSqlref)
	ON_COMMAND(ID_IGRESDOC_STARUG, OnIgresdocStarug)
	ON_COMMAND(ID_IGRESDOC_SYSADM, OnIgresdocSysadm)
	ON_COMMAND(ID_IGRESDOC_UFADT, OnIgresdocUfadt)
	ON_COMMAND(ID_IGRESDOC_INGNET, OnIgresdocConnectivity)
	ON_COMMAND(ID_IGRESDOC_IPM, OnIgresdocIpm)
	ON_COMMAND(ID_IGRESDOC_RELEASE, OnIgresdocReleaseSummary)
	ON_COMMAND(ID_ABOUT_ADVANTAGE, OnAppAbout)
	ON_COMMAND(ID_VCBF, OnVcbf)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpTopic)
	ON_MESSAGE (WMUSRMSG_SHELL_NOTIFY_ICON, OnTrayNotificationCallback)
	ON_MESSAGE (WMUSRMSG_APP_SHUTDOWN, OnExternalRequestShutDown)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT,  OnNotifyNewEvent)
	ON_MESSAGE (WMUSRMSG_IVM_MESSAGEBOX, OnIvmMessageBox)
	ON_WM_WINDOWPOSCHANGING()
	ON_MESSAGE (WMUSRMSG_UPDATE_COMPONENT, OnUpdateComponent)
	ON_MESSAGE (WMUSRMSG_UPDATE_EVENT, OnUpdateEvent)
	ON_MESSAGE (WMUSRMSG_UPDATE_SHELLICON, OnUpdateShellIcon)
	ON_MESSAGE (WMUSRMSG_REINITIALIZE_EVENT, OnReinitializeEvent)
	ON_MESSAGE (WMUSRMSG_SAVE_DATA, OnSaveData)

	ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindReplace )
	ON_MESSAGE(WMUSRMSG_ACTIVATE, OnMakeActive)
	ON_MESSAGE(WMUSRMSG_RUNEXTERNALPROGRAM, OnRunProgram)
	ON_COMMAND(ID_INGRES_PROMPT, OnIngresPrompt)
	ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
    ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame construction/destruction

CfMainFrame::CfMainFrame()
{
	m_bExit = FALSE;
#if defined (NOSHELLICON)
	m_bVisible = TRUE;  // Initially shown
#else
	m_bVisible = FALSE; // Initially hidden
#endif
	m_pDlgMessage = NULL;
	m_bEndSession = FALSE;

	m_pDlgMessageLimitCache = NULL;
	TCHAR   tchszAch [32];
	TCHAR   tchszDef [32];
	CString strFile;
	BOOL bChecked = TRUE;
	m_bContinueShowMessageLimitCache = TRUE;
	if (strFile.LoadString (IDS_INIFILENAME))
	{
		_stprintf (tchszDef, _T("%d"), GENERAL_INGRES_MAXEVENT_DEF);

		if (GetPrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_MAXEVENT_MSGBOX, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bContinueShowMessageLimitCache = (BOOL)_ttoi(tchszAch);
	}

	m_pThreadSave = NULL;
	CString strNamedMutex = _T("m_hEventStoreData");
	strNamedMutex+= theApp.GetCurrentInstallationID();
	m_hEventStoreData = CreateEvent (NULL, TRUE, TRUE, strNamedMutex);
	if (m_hEventStoreData == NULL)
	{
		AfxMessageBox (_T("System errror(m_hEventStoreData): Cannot create Event"));
		m_hEventStoreData = NULL;
	}
	m_bInfromAppShutDown = TRUE;
	pDlgFind = NULL;
	m_bFromDialog = FALSE;
	m_pWndSearchCaller = NULL;

	m_bDoAlert = FALSE;
	m_nNewEventCount = 0;
	m_heventExceedMax = CreateEvent (NULL, TRUE, TRUE, NULL);
	m_pPropFrame = NULL;
}

CfMainFrame::~CfMainFrame()
{
}

static DWORD IFThreadControlSaveData(LPVOID pParam)
{
	CfMainFrame* pFrame = (CfMainFrame*)pParam;
	if (!pFrame)
		return 1;
	return pFrame->ThreadProcControlPeriodicalStoreData(pParam);
}

int CfMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	UINT nStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP;

	if (GVvista())
	{
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, nStyle, CRect(1, 2, 1, 2)) ||
	    !m_wndToolBar.LoadToolBar(IDR_MAINFRAME4VISTA))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	}
	else
	{
		if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT|BTNS_AUTOSIZE , nStyle, CRect(1, 2, 1, 2)) ||
	    !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
		{
			TRACE0("Failed to create toolbar\n");
			return -1;      // fail to create
		}
	}
	//
	// Set the Title of the Main Toolbar:
	CString strToolbarTitle;
	if (!strToolbarTitle.LoadString (IDS_MAINTOOLBAR_TITLE))
		strToolbarTitle = _T("Ingres Visual Manager");
	m_wndToolBar.SetWindowText (strToolbarTitle);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	UINT nExtraStyle = CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC | CBRS_GRIPPER | CBRS_BORDER_3D;
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | nExtraStyle);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
#if !defined (MAINWIN)
	if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
	{
		CMenu* pMenu = (CMenu*)GetMenu();
		if (pMenu)
		{
			CMenu* pFileMenu = (CMenu*)pMenu->GetSubMenu(0);
			if (pFileMenu)
				pFileMenu->DeleteMenu (ID_START_CLIENT, MF_BYCOMMAND);
		}
	}
#endif
	m_pThreadSave = AfxBeginThread((AFX_THREADPROC)IFThreadControlSaveData, (LPVOID)this, THREAD_PRIORITY_BELOW_NORMAL);
#ifdef MAINWIN
	AfxGetApp()->m_pMainWnd=this;
#endif
	CreatePropertyMessageFrame();

	return 0;
}

BOOL CfMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass =_T("IVM");
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame diagnostics

#ifdef _DEBUG
void CfMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame message handlers

//
// If needed, wParam contains the Application-defined identifier of the taskbar icon
LONG CfMainFrame::OnTrayNotificationCallback (WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_LBUTTONDBLCLK:
		m_bVisible = TRUE;
		ActivateFrame();
		SetForegroundWindow();
		break;
	case WM_LBUTTONDOWN:
		if (IsWindowVisible() && !IsIconic())
		{
			m_bVisible = TRUE;
			ActivateFrame();
			SetForegroundWindow();
		}
		break;
	case WM_RBUTTONDOWN:
		OnPopupMenu(0, 0);
		break;
	default:
		break;
	}
	return 0L;
}

void CfMainFrame::OnClose() 
{
#if defined (NOSHELLICON)
	m_bExit = TRUE;
#endif

	CWaitCursor doWaitCursor;
	m_bVisible = FALSE;
	try
	{
		if (m_bExit)
		{
			if (theApp.GetModalDialogCount() > 0 && m_bInfromAppShutDown)
			{
				CString strMsg;
				strMsg.LoadString (IDS_MSG_CLOSE_MODALDIALOG);
				AfxMessageBox (strMsg);
				return;
			}

			BOOL bService = FALSE;
#if !defined (MAINWIN)
			if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
				bService=FALSE;
			else {
				//
				// Check if Ingres is not running as Service then shut down ingres:
				bService = IVM_IsServiceInstalled (theApp.m_strIngresServiceName);
				if (bService)
					bService = IVM_IsServiceRunning (theApp.m_strIngresServiceName);
			}

			if (!(m_bEndSession || bService))
			{
				//
				// Check to see if Ingres is running:
				BOOL bRunning = IVM_IsNameServerStarted();
				if (bRunning && m_bInfromAppShutDown)
				{
					//_T("Ingres is not running as a service.
					//   \nPlease don't not forget to shut it down,
					//   \nbefore logging out or shutting down the computer.");
					CString strMsg;
					if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
						strMsg.LoadString (IDS_WARNING_ON9598EXIT_INSTALLSTARTED);
					else
						strMsg.LoadString (IDS_MSG_INGRES_NOT_RUNNING_AS_SERVICE);
					AfxMessageBox(strMsg);
				}
			}
#endif
			//
			// Prepare to exit:
			theApp.SetRequestExited();
			CWinThread* pThreadComponent = theApp.GetWorkerThreadComponent();
			CWinThread* pThreadEvent =theApp.GetWorkerThreadEvent();
			if (pThreadComponent)
				WaitForSingleObject (pThreadComponent->m_hThread, 60*1000);
			if (pThreadEvent)
				WaitForSingleObject (pThreadEvent->m_hThread, 60*1000);
			theApp.NullWorkerThreads();
			if (m_hEventStoreData)
			{
				DWORD dwWait = WaitForSingleObject (m_hEventStoreData, 2*60*1000);
				if (m_pThreadSave && dwWait == WAIT_OBJECT_0)
				{
					ResetEvent (m_hEventStoreData);
					TerminateThread (m_pThreadSave->m_hThread, 0);
					delete m_pThreadSave;
					m_pThreadSave = NULL;
				}
			}
			CFrameWnd::OnClose();
			return;
		}

		SetWindowPos (&wndBottom, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);
		if (m_pPropFrame && m_pPropFrame->IsWindowVisible())
			m_pPropFrame->ShowWindow(SW_HIDE);
		return;
	}
	catch (...)
	{
		theApp.NullWorkerThreads();
		CFrameWnd::OnClose();
	}
}

void CfMainFrame::OnDestroy() 
{
	IVM_ShellNotification (NIM_DELETE, 1, NULL);
	CFrameWnd::OnDestroy();
}



void CfMainFrame::OnExpandAll() 
{
	TRACE0 ("CfMainFrame::OnExpandAll() ...\n");
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_EXPAND_ALL);
	}
}

void CfMainFrame::OnCallapseAll() 
{
	TRACE0 ("CfMainFrame::OnCallapseAll() ...\n");
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_COLLAPSE_ALL);
	}
}


void CfMainFrame::OnStart() 
{
	TRACE0 ("CfMainFrame::OnStart() ...\n");
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_START);
	}
}

void CfMainFrame::OnStartClient() 
{
	TRACE0 ("CfMainFrame::OnStartClient() ...\n");
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_START_CLIENT);
	}
}

void CfMainFrame::OnStop() 
{
	TRACE0 ("CfMainFrame::OnStop() ...\n");
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_STOP);
	}
}

void CfMainFrame::OnUpdateStart(CCmdUI* pCmdUI) 
{
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc && pDoc->m_hwndLeftView && IsWindow(pDoc->m_hwndLeftView))
	{
		CaTreeComponentItemData* pItem = NULL;
		pItem = (CaTreeComponentItemData*)::SendMessage (pDoc->m_hwndLeftView, WMUSRMSG_IVM_GETITEMDATA, 0, 0);
		if (pItem)
		{
			pCmdUI->Enable (pItem->IsStartEnabled());
			return;
		}
	}
	pCmdUI->Enable (FALSE);
}

void CfMainFrame::OnUpdateStartClient(CCmdUI* pCmdUI) 
{
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc && pDoc->m_hwndLeftView && IsWindow(pDoc->m_hwndLeftView))
	{
		CaTreeComponentItemData* pItem = NULL;
		pItem = (CaTreeComponentItemData*)::SendMessage (pDoc->m_hwndLeftView, WMUSRMSG_IVM_GETITEMDATA, 0, 0);
		if (pItem)
		{
			BOOL bEnable = FALSE;
			if (pItem->IsStartEnabled() && pItem->GetPageInformation()->GetClassName().CompareNoCase(_T("INSTALLATION")) == 0)
				bEnable = TRUE;
			pCmdUI->Enable (bEnable);
			return;
		}
	}
	pCmdUI->Enable (FALSE);
}

void CfMainFrame::OnUpdateStop(CCmdUI* pCmdUI) 
{
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc && pDoc->m_hwndLeftView && IsWindow(pDoc->m_hwndLeftView))
	{
		CaTreeComponentItemData* pItem = NULL;
		pItem = (CaTreeComponentItemData*)::SendMessage (pDoc->m_hwndLeftView, WMUSRMSG_IVM_GETITEMDATA, 0, 0);
		if (pItem)
		{
			pCmdUI->Enable (pItem->IsStopEnabled());
			return;
		}
	}
	pCmdUI->Enable (FALSE);
}


void CfMainFrame::OnHaltProgram() 
{
	if (m_bExit)
		return;
	m_bExit = TRUE;
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc)
	{
		pDoc->StoreEvents();
	}
	OnClose();
}

void CfMainFrame::OnActivateProgram() 
{
	m_bVisible = TRUE;
	ActivateFrame();
	SetForegroundWindow();
}

void CfMainFrame::OnIgnore() 
{

}


LONG CfMainFrame::OnPopupMenu(WPARAM wParam, LPARAM lParam)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUP_TRAY));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CString strFile = theApp.m_strIISystem + consttchszIngFiles;
#if defined (MAINWIN)
	strFile += _T("english/");
#else
	strFile += _T("english\\");
#endif
	strFile += _T("P002612E.pdf");

	if (GetFileAttributes(strFile) == -1)
		pPopup->DeleteMenu(23, MF_BYPOSITION);

	strFile = theApp.m_strIISystem + consttchszIngBin;
	strFile += _T("iilibudt.dll");

	if (GetFileAttributes(strFile) == -1)
		pPopup->DeleteMenu(ID_IILINK, MF_BYCOMMAND);

	strFile = theApp.m_strIISystem + consttchszIngBin;
	strFile += _T("perfwiz.exe");

	if (GetFileAttributes(strFile) == -1)
		pPopup->DeleteMenu(18, MF_BYPOSITION);

	CWnd* pWndPopupOwner = this;
	//
	// Menu handlers are coded in this source:
	// So do not use the following statement:
	// while (pWndPopupOwner->GetStyle() & WS_CHILD)
	//     pWndPopupOwner = pWndPopupOwner->GetParent();
	//
	int nCy = GetSystemMetrics (SM_CYSCREEN);
	CPoint p;
	SetForegroundWindow();
	GetCursorPos (&p);
	pPopup->TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, p.x, nCy, pWndPopupOwner, CRect (0,0,1,1));
	PostMessage (WM_NULL, 0, 0);

	return 0;
}


BOOL CfMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nIDD    = 0;
	UINT nHelpID = 0;
	if (pHelpInfo->iContextType == HELPINFO_MENUITEM)
	{
		//
		// Help requested for a menu item:
		nHelpID = pHelpInfo->iCtrlId;
	}
	else
	{
		//
		// Help requested for a control or window.
		nIDD = GetHelpID(); // It's a IDD_XX of the Dialog

		CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
		if (pDoc && pDoc->m_hwndLeftView && IsWindow(pDoc->m_hwndLeftView))
		{
			CaTreeComponentItemData* pItem = NULL;
			pItem = (CaTreeComponentItemData*)::SendMessage (pDoc->m_hwndLeftView, WMUSRMSG_IVM_GETITEMDATA, 0, 0);
			ASSERT (pItem);
			if (!pItem)
				return FALSE;
			if (pItem->m_nComponentType == -1)
				nHelpID = IVM_HelpBase(TREE_INSTALLATION, pItem->m_nComponentType, nIDD);
			else
			if (pItem->IsFolder())
				nHelpID = IVM_HelpBase(TREE_FOLDER, pItem->m_nComponentType, nIDD);
			else
			{
				if (pItem->IsInstance())
					nHelpID = IVM_HelpBase(TREE_INSTANCE, pItem->m_nComponentType, nIDD);
				else
					nHelpID = IVM_HelpBase(TREE_COMPONENTNAME, pItem->m_nComponentType, nIDD);
			}
		}
	}
	TRACE2 ("CfMainFrame::OnHelpInfo: HelpID = %d. Dialog ID = %d\n", nHelpID, nIDD);
	return APP_HelpInfo(nHelpID);
}

void CfMainFrame::OnHelpTopic()
{
	APP_HelpInfo(0);
}

UINT CfMainFrame::GetHelpID()
{
	UINT nHelpID = 0;
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc && pDoc->m_hwndRightView && IsWindow(pDoc->m_hwndRightView))
	{
		nHelpID = (UINT)::SendMessage (pDoc->m_hwndRightView, WMUSRMSG_IVM_GETHELPID, 0, 0);
	}
	return nHelpID;
}

LONG CfMainFrame::OnSystemShutDown(WPARAM wParam, LPARAM lParam)
{
	m_bEndSession = TRUE;
	OnHaltProgram();
	m_bEndSession = FALSE;
	return 0;
}

LONG CfMainFrame::OnExternalRequestShutDown(WPARAM wParam, LPARAM lParam)
{
	//
	// Silent mode, no message box !!!
	m_bInfromAppShutDown = FALSE;
	OnHaltProgram();
	return TRUE;
}

void CfMainFrame::OnConfigure()
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_CONFIGURE);
	}
}


LONG CfMainFrame::OnNotifyNewEvent (WPARAM wParam, LPARAM lParam)
{
	try
	{
		SetForegroundWindow();
		if (m_pDlgMessage)
		{
			m_pDlgMessage->DestroyWindow();
			m_pDlgMessage = NULL;
		}
		m_pDlgMessage = new CuDlgYouHaveNewMessage(NULL);
		m_pDlgMessage->Create (IDD_YOUHAVEMESSAGE, NULL);
		m_pDlgMessage->ShowWindow (SW_SHOW);
		m_pDlgMessage->UpdateWindow();
	}
	catch (CMemoryException* e)
	{
		IVM_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("CfMainFrame::OnNotifyNewEvent: internal error (1)\n");
	}
	return 0;
}

void CfMainFrame::OnVisualDBA() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_VISUALDBA);
	}
}


void CfMainFrame::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if (!m_bVisible)
		lpwndpos->flags &= ~SWP_SHOWWINDOW ;
	else
		lpwndpos->flags |= SWP_SHOWWINDOW ;
	CFrameWnd::OnWindowPosChanging(lpwndpos);
}


LONG CfMainFrame::OnUpdateComponent(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	try
	{
		CdMainDoc* pMainDoc = theApp.GetMainDocument();
		if (pMainDoc)
		{
			HWND hLeftView = pMainDoc->GetLeftViewHandle();
			if (IsWindow (hLeftView))
			{
				::SendMessage (hLeftView, WMUSRMSG_UPDATE_COMPONENT, wParam, lParam);
			}
		}
	}
	catch (...)
	{

	}
	SetEvent (theApp.GetEventComponent());
	return 0;
}


LONG CfMainFrame::OnUpdateEvent(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	HWND hRightView = NULL;
	try
	{
		CString strTimeM;
		CdMainDoc* pMainDoc = theApp.GetMainDocument();
		if (pMainDoc)
			hRightView = pMainDoc->GetRightViewHandle();
		CWnd* pWndLoggedEventPage = IsPaneCreated(IDD_LOGEVENT_GENERIC);

		CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
		plistEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
		CaQueryEventInformation* pQueryEventInfo = (CaQueryEventInformation*)wParam;

		if (pQueryEventInfo)
		{
			if (pQueryEventInfo->GetBlockSequence() == 0)
			{
				m_nNewEventCount = 0;
				m_bDoAlert = FALSE;
			}
			if (plistEvent)
				m_nNewEventCount += plistEvent->GetCount();
		}

		//
		// Add events to the cache:
		// Let N the number of events that the list 'plistEvent' containts before adding into the cache.
		// Let M the number of events that the list 'plistEvent' containts after adding into the cache.
		// We have M <= N.
		//   1) M = N: all new events fill fit in the cache:
		//   2) M < N: some of new events will be removed out of the cache due to the limitation.
		//             This is the case when Count of (plistEvent) > Cache size.
		//
		// After adding events into the cache, we notify the right view that there are M new events arriving 
		// if we are not being reinitialized !!


		switch (theApp.m_setting.GetEventMaxType())
		{
		case EVMAX_NOLIMIT:
			if (!plistEvent || !pQueryEventInfo)
				break;
			HandleLimitEvents (wParam, lParam, TRUE);
			break;
		case EVMAX_COUNT:
		case EVMAX_MEMORY:
			if (!plistEvent || !pQueryEventInfo)
				break;
			HandleLimitEvents (wParam, lParam);
			break;
		case EVMAX_SINCEDAY:
			strTimeM = IVM_GetTimeSinceNDays(theApp.m_setting.GetMaxDay());
			KeepEvents (strTimeM, wParam, lParam);
			break;
		case EVMAX_SINCENAME:
			if (pQueryEventInfo && pQueryEventInfo->GetBlockSequence() == -1)
			{
				long lLastNameServer = theApp.GetLastNameStartedup();
				if (lLastNameServer != -1)
					::SendMessage (pMainDoc->GetRightViewHandle(), WMUSRMSG_NAMESERVER_STARTED, (WPARAM)0, (LPARAM)lLastNameServer);
			}
			KeepEvents (NULL, wParam, lParam);
			break;
		default:
			ASSERT (FALSE);
			break;
		}
	
		// Fix BUG #105382: notification of alert should be done after the instruction
		// switch (theApp.m_setting.GetEventMaxType()) { ... } above because in that instruction
		// all events in the list 'plistEvent' might or might not be added into the cache.
		// ie: if the list 'plistEvent' is empty then all received events are not added into the cache
		// and so it is wrong to notify user that there is an alert message.
		// Notify the Alert (eg: Notify the Tree of Components to change the Icons):
		if (pQueryEventInfo)
		{
			if (pQueryEventInfo->GetBlockSequence() == -1 && !plistEvent)
			{
				CTypedPtrList<CObList, CaLoggedEvent*> eventSetToAlert;
				BOOL bNotifyAlert = FALSE;
				int count = 0;
				CaIvmEvent& eventData = theApp.GetEventData();
				CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
				POSITION pt = le.GetTailPosition();
				while (pt != NULL && count < m_nNewEventCount)
				{
					CaLoggedEvent* pEvent = le.GetPrev (pt);
					if (pEvent->IsAlert() && !pEvent->IsRead())
					{
						m_bDoAlert = TRUE; // There are alerted events
						if (!pEvent->IsAltertNotify())
							bNotifyAlert = TRUE;
						eventSetToAlert.AddHead(pEvent);
					}

					pEvent->AltertNotify(TRUE);
					count++;
				}
				m_nNewEventCount = 0;

				if (m_bDoAlert)
				{
					if (theApp.m_setting.m_bSoundBeep)
						MessageBeep (MB_ICONEXCLAMATION);
					CaIvmEvent& eventData = theApp.GetEventData();
					CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
					if (pMainDoc && IsWindow (pMainDoc->GetLeftViewHandle()) && !le.IsEmpty())
						::SendMessage (pMainDoc->GetLeftViewHandle(), WMUSRMSG_UPDATE_ALERT, (WPARAM)bNotifyAlert, (LPARAM)&eventSetToAlert);
				}

				eventSetToAlert.RemoveAll();
				m_bDoAlert = FALSE;
				delete pQueryEventInfo;
				return 0;
			}

			if (pQueryEventInfo->GetLimitReach() && pQueryEventInfo->GetExceed() != 0)
			{
				EventOutofCacheMessage(NULL);
			}
		}
		// Remove the fix: BUG #105382 
		// 	if (pMainDoc && IsWindow (pMainDoc->GetLeftViewHandle()) && plistEvent && !plistEvent->IsEmpty())
		// 		::SendMessage (pMainDoc->GetLeftViewHandle(), WMUSRMSG_UPDATE_ALERT, (WPARAM)TRUE, (LPARAM)plistEvent);

		// After adding events into the cache, we notify the right view that there are M new events arriving 
		// if we are not being reinitialized !!
		BOOL bReinit = theApp.IsRequestInitializeEvents();
		if (!bReinit && plistEvent && !plistEvent->IsEmpty())
		{
			CWnd* pActiveRightPane = GetActiveRightPane();
			if (pWndLoggedEventPage && IsWindow (pWndLoggedEventPage->m_hWnd) && pActiveRightPane == pWndLoggedEventPage)
			{
				//
				// Notify the pane of list of events to display these events
				// if they are matched the filter:
				pWndLoggedEventPage->SendMessage (WMUSRMSG_IVM_NEW_EVENT, 0, (LPARAM)plistEvent);
				pWndLoggedEventPage->SendMessage (WMUSRMSG_REPAINT, 0, 0);
			}
			else
			{
				if (pActiveRightPane && IsWindow(pActiveRightPane->m_hWnd))
				{
					pActiveRightPane->SendMessage (WMUSRMSG_IVM_NEW_EVENT, 0, (LPARAM)plistEvent);
				}
			}
		}

		if (pQueryEventInfo->GetBlockSequence() == -1)
		{
			int count = 0;
			CaIvmEvent& eventData = theApp.GetEventData();
			CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
			POSITION pt = le.GetTailPosition();
			while (pt != NULL && count < m_nNewEventCount)
			{
				CaLoggedEvent* pEvent = le.GetPrev (pt);
				if (!m_bDoAlert && pEvent->IsAlert() && !pEvent->IsRead() && !pEvent->IsAltertNotify())
					m_bDoAlert = TRUE;

				pEvent->AltertNotify(TRUE);
				count++;
			}

			m_nNewEventCount = 0;
		}
		if (plistEvent)
			delete plistEvent;
		if (pQueryEventInfo)
			delete pQueryEventInfo;
	}

	catch (...)
	{


	}
	return 0;
}


LONG CfMainFrame::OnUpdateShellIcon(WPARAM wParam, LPARAM lParam)
{
	theApp.UpdateShellIcon();
	return 0;
}


LONG CfMainFrame::OnIvmMessageBox(WPARAM wParam, LPARAM lParam)
{
	LPTSTR lpszMessage = (LPTSTR)lParam;
	SetForegroundWindow();
	if ((int)wParam == 1 && lpszMessage)
	{
		CuDlgShowMessage* pMsgDlg = new CuDlgShowMessage(NULL);
		pMsgDlg->m_bShowCheckBox = FALSE;
		pMsgDlg->m_strStatic2 = lpszMessage;
		pMsgDlg->Create (IDD_MESSAGE, NULL);
		pMsgDlg->ShowWindow (SW_SHOW);
		pMsgDlg->UpdateWindow();
		delete lpszMessage;
	}
	else
	{
		if (lpszMessage)
		{
			AfxMessageBox (lpszMessage);
			delete [] lpszMessage;
		}
		else
		{
			IVM_OutOfMemoryMessage();
		}
	}
	return 0;
}

LONG CfMainFrame::OnReinitializeEvent(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	try
	{
		m_nNewEventCount = 0;
		m_bContinueShowMessageLimitCache = TRUE;
		//
		// Reinitialize also the flag in the .ini file.
		CaMsgExceedMax save(_T(""), NULL);
		save.SetChecked(TRUE);  // mark "Show this message again"
		save.EndMessage();      // Save flag to ini file

		CdMainDoc* pMainDoc = theApp.GetMainDocument();
		if (pMainDoc)
		{
			HWND hRightView = pMainDoc->GetRightViewHandle();
			if (IsWindow (hRightView))
			{
				TRACE0 ("CfMainFrame::OnReinitializeEvent !!!\n");
				//
				// Notify the right view to reinitialize events:
				::SendMessage (hRightView, WMUSRMSG_REINITIALIZE_EVENT, wParam, lParam);
				//
				// Cleanup the cache:
				CaIvmEvent& evdata = theApp.GetEventData();
				evdata.CleanUp();

				CaIvmEventStartStop& loggedEvent = theApp.GetEventStartStopData();
				loggedEvent.CleanUp();
			}
		}
	}
	catch (...)
	{

	}
	SetEvent (theApp.GetEventReinitializeEvent());
	return 0;
}



void CfMainFrame::HandleLimitEvents (WPARAM wParam, LPARAM lParam, BOOL bNoLimit)
{
	class CaBlockReadingEvent
	{
	public:
		CaBlockReadingEvent():m_bLocked(FALSE){};
		void Lock()
		{
			//
			// Block the Thread reading events (set event to non-signal state)
			ResetEvent (theApp.GetEventMaxLimitReached());
			m_bLocked = TRUE;
		}
		void Unlock()
		{
			//
			// Set event to non-signal state:
			if (m_bLocked)
				SetEvent (theApp.GetEventMaxLimitReached());
			m_bLocked = FALSE;
		}

		~CaBlockReadingEvent()
		{
			//
			// Set event to non-signal state:
			if (m_bLocked)
				SetEvent (theApp.GetEventMaxLimitReached());
		}
	protected:
		BOOL m_bLocked;
	};

	BOOL bEventsRemoved = FALSE;
	CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
	plistEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
	CaQueryEventInformation* pQueryEventInfo = (CaQueryEventInformation*)wParam;

	long  nRemoveEventCount = 0;
	CWnd* pWndCreated = NULL;
	HWND hwndViewLeft = NULL;
	CdMainDoc* pDoc = theApp.GetMainDocument();
	pWndCreated = IsPaneCreated (IDD_LOGEVENT_GENERIC);
	if (pDoc)
		hwndViewLeft = pDoc->GetLeftViewHandle();

	BOOL bLimitReached = pQueryEventInfo->GetLimitReach();
	long lExceed = pQueryEventInfo->GetExceed();

	CaIvmEvent& eventData = theApp.GetEventData();
	CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
	CaLoggedEvent* pEvent = NULL;
	CaLoggedEvent* pRemoveEvent = NULL;
	POSITION pos = plistEvent->GetHeadPosition();

	//
	// Put events into the cache (Linked list):
	if (!bLimitReached || (bLimitReached && lExceed == 0) || bNoLimit)
	{
		//
		// Case of not reached or reached but not exceed or No Limitation:
		while (pos != NULL)
		{
			pEvent = plistEvent->GetNext (pos);
			le.AddTail (pEvent);
		}
	}
	else
	{
		//
		// Exceed the limit:
		BOOL bContinue = FALSE;
		if (lExceed > 0)
		{
			//
			// Exceed of event counts:
			// reached and exceed of 'lExceed' events:
			// In this situation, we must delete the first 'lLimitReached' from the cache.
			// Be carefull, the deleted events might be in used in list of events (CListCtrl), 
			// we must notify the Disply List Event to reinitialize the events
			bContinue = (nRemoveEventCount < lExceed && !le.IsEmpty());
		}
		else
		{
			//
			// Exceed of memory unit:
			// In this situation, we must delete the events from the cache.
			// Be carefull, the deleted events might be in used in list of events (CListCtrl), 
			// we must notify the Disply List Event to reinitialize the events
			bContinue = ((-1)*lExceed + theApp.GetEventSize()) > (theApp.m_setting.GetMaxSize() * 1024*1024);
		}
		
		CaBlockReadingEvent LockThreadReadingEvent;
		if (bContinue)
		{
			//
			// Block the Thread reading events (set event to non-signal state)
			LockThreadReadingEvent.Lock();
		}

		//
		// Remove some events out of the cache:
		while (bContinue)
		{
			//
			// Remove the event from the cache:
			pRemoveEvent = le.RemoveHead();

			//
			// Notify the pane of list of events to remove this event
			// if it is being used, because it will be removed from the cache:
			if (pWndCreated && IsWindow (pWndCreated->m_hWnd))
			{
				pWndCreated->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, 0, (LPARAM)pRemoveEvent);
			}
			//
			// Before removing event out of the cache, try to unalert on the left tree:
			if (theApp.IsAutomaticUnalert())
			{
				if (pRemoveEvent->IsAlert() && !pRemoveEvent->IsRead())
				{
					if (IsWindow (hwndViewLeft))
						::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, 0, (LPARAM)pRemoveEvent);
				}
			}
			delete pRemoveEvent;
			nRemoveEventCount++;

			if (lExceed > 0)
			{
				bContinue = (nRemoveEventCount < lExceed && !le.IsEmpty());
			}
			else
			{
				bContinue = (pEvent->GetSize() + theApp.GetEventSize()) > (theApp.m_setting.GetMaxSize() * 1024*1024);
			}
		}

		bEventsRemoved = FALSE;
		while (pos != NULL)
		{
			pEvent = plistEvent->GetNext (pos);
			//
			// Still not have the space necessary for the new events ?
			if (lExceed > 0)
			{
				bEventsRemoved = ((UINT)le.GetCount() + 1 > theApp.m_setting.GetMaxCount());
			}
			else
			{
				bEventsRemoved = (pEvent->GetSize() + theApp.GetEventSize()) > (theApp.m_setting.GetMaxSize() * 1024*1024);
			}
			while (bEventsRemoved)
			{
				pRemoveEvent = le.RemoveHead();

				//
				// Notify the pane of list of events to remove this event
				// if it is being used, because it will be removed from the cache:
				if (pWndCreated && IsWindow (pWndCreated->m_hWnd))
				{
					pWndCreated->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, 0, (LPARAM)pRemoveEvent);
				}
				//
				// Before removing event out of the cache, try to unalert on the left tree:
				if (theApp.IsAutomaticUnalert())
				{
					if (pRemoveEvent->IsAlert() && !pRemoveEvent->IsRead())
					{
						if (IsWindow (hwndViewLeft))
							::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, 0, (LPARAM)pRemoveEvent);
					}
				}
				delete pRemoveEvent;
				nRemoveEventCount++;

				if (lExceed > 0)
				{
					bEventsRemoved = ((UINT)le.GetCount() + 1 > theApp.m_setting.GetMaxCount());
				}
				else
				{
					bEventsRemoved = (pEvent->GetSize() + theApp.GetEventSize()) > (theApp.m_setting.GetMaxSize() * 1024*1024);
				}
			}
			//
			// Add the new event to the cache:
			le.AddTail (pEvent);
		}

		LockThreadReadingEvent.Unlock();
	}


	// At this point, we have added events to cache (list called <le> above).
	// But the count of elements in the cache (<le>) might be less than the count of elements 
	// in <plistEvent> due to the settings,
	// and some elements of <plistEvent> that were transfered to <le> might have been destroyed.
	// So create a new list of events as following:
	//    - all events are elements of <le>
	//    - retrieve the elements of <le> from the tail one by one
	//    - add elements to the new list in the head one by one.
	//    - THE COUNT OF NEW LIST <= THE COUNT OF <plistEvent>

	int count = 0;
	INT_PTR nECount = plistEvent->GetCount();
	plistEvent->RemoveAll();
	pos = le.GetTailPosition();
	while (pos != NULL && count < nECount)
	{
		pEvent = le.GetPrev (pos);
		count++;
		plistEvent->AddHead (pEvent);
	}
	//
	// At this point, <plistEvent> containts only new events that are truly in the cache,
	// for those that have been added to the cache and then removed out afterward
	// are not in the list <plistEvent>.
}



//
// Remove all events whose date < lpszFromTime out of memory:
void CfMainFrame::KeepEvents (LPCTSTR lpszFromTime, WPARAM wParam, LPARAM lParam)
{
	CString strDate;
	BOOL bEventsRemoved = FALSE;
	LONG  nRemoveEventCount = 0;
	CWnd* pWndLoggedEventCreated = IsPaneCreated (IDD_LOGEVENT_GENERIC);

	CaIvmEvent& eventData = theApp.GetEventData();
	CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
	CaLoggedEvent* pEvent = NULL;
	HWND hwndViewLeft = NULL;
	CdMainDoc* pDoc = (CdMainDoc*)theApp.GetMainDocument();
	ASSERT (pDoc);
	if (pDoc)
		hwndViewLeft = pDoc->GetLeftViewHandle();
	
	POSITION p, pos = le.GetHeadPosition();
	if (lpszFromTime != NULL)
	{
		while (pos != NULL)
		{
			p = pos;
			pEvent = le.GetNext (pos);
			strDate = pEvent->GetIvmFormatDate();

			if (strDate.IsEmpty())
			{
				//
				// In this case we cannot manage to keep events from a certain date.
				// So ignore this event:
				le.RemoveAt (p);
				delete pEvent;
				continue;
			}
			
			if (strDate.CompareNoCase (lpszFromTime) < 0)
			{
				//
				// Remove event out of the Display List:
				if (pWndLoggedEventCreated && IsWindow (pWndLoggedEventCreated->m_hWnd))
				{
					pWndLoggedEventCreated->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, 0, (LPARAM)pEvent);
					bEventsRemoved = TRUE;
				}
				//
				// Before removing event out of the cache, try to unalert on the left tree:
				if (theApp.IsAutomaticUnalert())
				{
					if (pEvent->IsAlert() && !pEvent->IsRead())
					{
						if (IsWindow (hwndViewLeft))
							::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, 0, (LPARAM)pEvent);
					}
				}
				
				le.RemoveAt (p);
				delete pEvent;
			}
		}
	}

	//
	// Put events into the cache (Linked list):
	CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
	plistEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
	if (!plistEvent)
		return;
	pos = plistEvent->GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pEvent = plistEvent->GetNext (pos);
		strDate = pEvent->GetIvmFormatDate();

		if (strDate.IsEmpty())
		{
			//
			// In this case we cannot manage to keep events from a certain date.
			// So ignore this event:
			plistEvent->RemoveAt (p);
			delete pEvent;
			continue;
		}

		if (lpszFromTime && strDate.CompareNoCase (lpszFromTime) < 0)
		{
			//
			// This event 'pEvent' might alert the left tree, try to unalert on the left tree:
			if (theApp.IsAutomaticUnalert())
			{
				if (pEvent->IsAlert() && !pEvent->IsRead())
				{
					if (IsWindow (hwndViewLeft))
						::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, 0, (LPARAM)pEvent);
				}
			}
			plistEvent->RemoveAt (p);
			delete pEvent;
		}
		else
		{
			le.AddTail (pEvent);
		}
	}
}


CWnd* CfMainFrame::IsPaneCreated(UINT nPaneID)
{
	CWnd* pWnd = NULL;
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (!pDoc)
		return NULL;
	HWND hwndViewRight = pDoc->GetRightViewHandle();
	if (!hwndViewRight)
		return NULL;
	pWnd = (CWnd*)::SendMessage(hwndViewRight, WMUSRMSG_PANE_CREATED, (WPARAM)nPaneID, 0);
	
	return pWnd;
}


CWnd* CfMainFrame::GetActiveRightPane()
{
	CWnd* pWnd = NULL;
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (!pDoc)
		return NULL;
	HWND hwndViewRight = pDoc->GetRightViewHandle();
	if (!hwndViewRight)
		return NULL;
	pWnd = (CWnd*)::SendMessage(hwndViewRight, WMUSRMSG_PANE_CREATED, (WPARAM)-1, -1);
	return pWnd;
}


BOOL CfMainFrame::EventOutofCacheMessage(CaLoggedEvent* pEvent)
{
	try
	{
#ifdef MAINWIN
		TCHAR tchszReturn [] = {0x0D, 0x0A, 0x00};
#else
		TCHAR tchszReturn [] = {0x0A, 0x00};
#endif

		if (!m_bContinueShowMessageLimitCache)
			return TRUE;
		DWORD dwWait = WaitForSingleObject(m_heventExceedMax, 10);
		if (dwWait !=  WAIT_OBJECT_0)
			return 0;
		if (!theApp.m_bMaxEventReachedWarning)
			return TRUE;

		SetForegroundWindow();
		//_T("You have reached the max events limit defined in the Preferences.\n"
		//   "The oldest messages (possibly including unread messages) "
		//   "will no longer be available.\n"
		//   "You may consider changing the Preferences.");
		CString strMsg;
		strMsg.LoadString(IDS_MSG_EVENTLIMIT_REACHED);
		CaMsgExceedMax* pParam = new CaMsgExceedMax(strMsg, m_heventExceedMax);
		pParam->SetChecked(TRUE); // Always true (fnn's request)
		m_pDlgMessageLimitCache = new CuDlgShowMessage(NULL);
		m_pDlgMessageLimitCache->SetParam(pParam);

		m_pDlgMessageLimitCache->Create (IDD_MESSAGE, NULL);
		m_pDlgMessageLimitCache->ShowWindow (SW_SHOW);
		m_pDlgMessageLimitCache->UpdateWindow();
		m_bContinueShowMessageLimitCache = FALSE; // Only show one !
	}
	catch (CMemoryException* e)
	{
		IVM_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("CfMainFrame::OnNotifyNewEvent: internal error (1)\n");
	}
	return TRUE;
}



UINT CfMainFrame::ThreadProcControlPeriodicalStoreData (LPVOID pParam)
{
	CfMainFrame* pFrame = (CfMainFrame*)pParam;
	UINT lEllapse = 60*60*1000; // 1 Hour
	ASSERT (pFrame);
	if (!pFrame)
		return 1;
	while (1)
	{
		Sleep (lEllapse);
		::PostMessage (pFrame->m_hWnd, WMUSRMSG_SAVE_DATA, 0, 0);
	}
	
	m_pThreadSave = NULL;
	return 0;
}

LONG CfMainFrame::OnSaveData(WPARAM wParam, LPARAM lParam)
{
	try
	{
		CdMainDoc* pDoc = NULL;
		pDoc = (CdMainDoc*)GetActiveDocument();
		if (!pDoc)
			return 0;
		if (m_hEventStoreData)
		{
			DWORD dwWait = WaitForSingleObject (m_hEventStoreData, 0);
			if (dwWait == WAIT_OBJECT_0)
			{
				ResetEvent (m_hEventStoreData);
				pDoc->StoreEvents();
				SetEvent (m_hEventStoreData);
				TRACE0 ("CfMainFrame::ThreadProcControlPeriodicalStoreData: Store Events\n");
			}
		}
	}
	catch (...)
	{
		TRACE0 ("Raise exception: CfMainFrame::ThreadProcControlPeriodicalStoreData\n");
	}
	return 0;
}

void CfMainFrame::OnFind() 
{
	if (pDlgFind == NULL)
	{
		UINT nFlag = FR_HIDEUPDOWN|FR_DOWN;
		pDlgFind = new CFindReplaceDialog();
		pDlgFind->Create (TRUE, theApp.m_strLastFindWhat, NULL, nFlag, this);
		pDlgFind->SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
		m_findInfo.Clean();
	}
	else
	if (IsWindow (pDlgFind->m_hWnd))
	{
		pDlgFind->SetFocus();
	}
}

void CfMainFrame::OnUpdateFind(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CWnd* pWnd = GetActiveRightPane();
	if (pWnd && IsWindow (pWnd->m_hWnd))
	{
		bEnable = (BOOL)pWnd->SendMessage (WMUSRMSG_FIND_ACCEPTED, 0, 0);
	}
	pCmdUI->Enable(bEnable);
}


LONG CfMainFrame::OnFindReplace(WPARAM wParam, LPARAM lParam)
{
	TRACE0("CMainFrame::OnFindReplace\n");
	LPFINDREPLACE lpfr = (LPFINDREPLACE) lParam;
	if (!lpfr)
		return 0;
	
	if (lpfr->Flags & FR_DIALOGTERM)
	{
		theApp.m_strLastFindWhat = pDlgFind->GetFindString();
		pDlgFind = NULL;
		m_bFromDialog = FALSE;
		m_pWndSearchCaller = NULL;
	}
	else
	if (lpfr->Flags & FR_FINDNEXT)
	{
		//
		// Find Next:
		CString strMsg;
		theApp.m_strLastFindWhat = pDlgFind->GetFindString();
		
		CWnd* pWnd = NULL;
		BOOL bRestartSearching = TRUE;
		while (bRestartSearching)
		{
			bRestartSearching = FALSE;
			if (!m_bFromDialog)
				pWnd = GetActiveRightPane();
			else
			{
				ASSERT (m_pWndSearchCaller);
				if (!m_pWndSearchCaller)
					return 0;
				pWnd = m_pWndSearchCaller;
			}

			if (pWnd && IsWindow (pWnd->m_hWnd))
			{
				if (!m_findInfo.GetFindTargetPane())
					m_findInfo.SetFindTargetPane(pWnd);
				else
				{
					if (pWnd != m_findInfo.GetFindTargetPane())
					{
						m_findInfo.SetListWindow(NULL);
						m_findInfo.SetListPos (0);
					}
					m_findInfo.SetFindTargetPane(pWnd);
				}
				m_findInfo.SetFlag (lpfr->Flags);
				m_findInfo.SetWhat (theApp.m_strLastFindWhat);

				int nAnswer =0;
				long lResult = (long)pWnd->SendMessage (WMUSRMSG_FIND, 0, (LPARAM)&m_findInfo);
				switch (lResult)
				{
				case FIND_RESULT_OK:
					break;
				case FIND_RESULT_NOTFOUND:
					AfxFormatString1 (strMsg, IDS_SEARCH_NOTFOUND, (LPCTSTR)theApp.m_strLastFindWhat);
					AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);

					m_findInfo.SetListWindow(NULL);
					m_findInfo.SetListPos (0);
					m_findInfo.SetEditWindow(NULL);
					m_findInfo.SetEditPos (0);
					break;
				case FIND_RESULT_END:
					if (!strMsg.LoadString (IDS_SEARCH_END))
						strMsg = _T("You have reached the end of document.\nWould you like to restart searching from the beginning ?");
					nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
					if (nAnswer == IDYES)
					{
						bRestartSearching = TRUE;
						m_findInfo.SetListWindow(NULL);
						m_findInfo.SetListPos (0);
						m_findInfo.SetEditWindow(NULL);
						m_findInfo.SetEditPos (0);
					}
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}

void CfMainFrame::Search(CWnd* pWndCaller)
{
	try
	{
		m_bFromDialog = TRUE; 
		if (!pWndCaller )
		{
			m_bFromDialog = FALSE;
#ifndef MAINWIN
			if (pDlgFind)
				pDlgFind->DestroyWindow();
#endif
			pDlgFind = NULL;
			return;
		}
		m_pWndSearchCaller = pWndCaller;
		OnFind();
	}
	catch (...)
	{
		TRACE0 ("Exception: CfMainFrame::Search(CWnd* pWndCaller) \n");
	}
}

LONG CfMainFrame::OnMakeActive(WPARAM wParam, LPARAM lParam)
{
	OnActivateProgram();
	return 0;
}

void CfMainFrame::OnEndSession(BOOL bEnding) 
{
	CFrameWnd::OnClose();
	CFrameWnd::OnEndSession(bEnding);
}

BOOL CfMainFrame::OnQueryEndSession() 
{
	if (!CFrameWnd::OnQueryEndSession())
		return FALSE;
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc)
	{
		pDoc->StoreEvents();
	}
	return TRUE;
}


LONG CfMainFrame::OnRunProgram(WPARAM wParam, LPARAM lParam)
{
	CdMainDoc* pDoc = (CdMainDoc*)GetActiveDocument();
	if (pDoc)
	{
		HWND hwndViewLeft = pDoc->GetLeftViewHandle();
		if (hwndViewLeft)
		{
			::PostMessage(hwndViewLeft, WMUSRMSG_RUNEXTERNALPROGRAM, wParam, lParam);
			TRACE0 ("CfMainFrame::OnRunProgram-> PostMessage WMUSRMSG_RUNEXTERNALPROGRAM\n");
		}
	}
	return 0;
}

CfMiniFrame* CfMainFrame::ShowMessageDescriptionFrame(BOOL bCreate, MSGCLASSANDID* pMsg)
{
	// The property sheet attached to your project
	// via this function is not hooked up to any message
	// handler.  In order to actually use the property sheet,
	// you will need to associate this function with a control
	// in your project such as a menu item or tool bar button.
	//
	// If mini frame does not already exist, create a new one.
	// Otherwise, unhide it

	if (m_pPropFrame == NULL && bCreate)
	{
		CTypedPtrList < CObList, CPropertyPage* > listPages;
		listPages.AddTail(new CuPropertyPageMessageDescription());

		m_pPropFrame = new CfMiniFrame(listPages, TRUE);
		CRect rect(0, 0, 0, 0);
		CString strTitle;
		VERIFY(strTitle.LoadString(IDS_MESSAGEDESCRIPTION_CAPTION));
		UINT nStyle = WS_THICKFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
		if (!m_pPropFrame->Create(NULL, strTitle, nStyle, rect, this))
		{
			delete m_pPropFrame;
			m_pPropFrame = NULL;
			return NULL;
		}
		m_pPropFrame->CenterWindow();
	}

	// Before unhiding the modeless property sheet, update its
	// settings appropriately.  For example, if you are reflecting
	// the state of the currently selected item, pick up that
	// information from the active view and change the property
	// sheet settings now.
	if (m_pPropFrame != NULL && !m_pPropFrame->IsWindowVisible() && bCreate)
		m_pPropFrame->ShowWindow(SW_SHOW);
	if (m_pPropFrame != NULL && IsWindow (m_pPropFrame->m_hWnd))
		m_pPropFrame->HandleData((LPARAM)pMsg);
	return m_pPropFrame;
}

void CfMainFrame::OnMessageDescription() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
	{
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_MESSAGE_DESCRIPTION);
	}
}

void CfMainFrame::CreatePropertyMessageFrame() 
{
	if (m_pPropFrame == NULL)
	{
		CTypedPtrList < CObList, CPropertyPage* > listPages;
		listPages.AddTail(new CuPropertyPageMessageDescription());

		m_pPropFrame = new CfMiniFrame(listPages, TRUE);
		CRect rect(0, 0, 0, 0);
		CString strTitle;
		VERIFY(strTitle.LoadString(IDS_MESSAGEDESCRIPTION_CAPTION));
		UINT nStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
		if (!m_pPropFrame->Create(NULL, strTitle, nStyle, rect, this))
		{
			delete m_pPropFrame;
			m_pPropFrame = NULL;
			return;
		}
		m_pPropFrame->CenterWindow();
	}
}

void CfMainFrame::OnNetworkUtility() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_NETWORKUTILITY);
}

void CfMainFrame::OnSqlquery() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_SQLQUERY);
}

void CfMainFrame::OnIpm() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IPM);
}

void CfMainFrame::OnIia() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IIA);
}

void CfMainFrame::OnIea() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IEA);
}

void CfMainFrame::OnVcda() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_VCDA);
}

void CfMainFrame::OnVdda() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_VDDA);
}

void CfMainFrame::OnIja() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IJA);
}

void CfMainFrame::OnIilink() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IILINK);
}

void CfMainFrame::OnPerformanceWizard() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_PERFORMANCE_WIZARD);
}

void CfMainFrame::OnIngresCache() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_INGRES_CACHE);
}

void CfMainFrame::OnErrorLog() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_ERROR_LOG);
}

void CfMainFrame::OnReadme() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_README);
}

void CfMainFrame::OnIgresdocCmdref() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_CMDREF);
}

void CfMainFrame::OnIgresdocDba() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_DBA);
}

void CfMainFrame::OnIgresdocDtp() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_DTP);
}

void CfMainFrame::OnIgresdocEsqlc() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_ESQLC);
}

void CfMainFrame::OnIgresdocEquel() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_EQUEL);
}

void CfMainFrame::OnIgresdocGs() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_GS);
}

void CfMainFrame::OnIgresdocIceug() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_ICEUG);
}

void CfMainFrame::OnIgresdocMg() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_MG);
}

void CfMainFrame::OnIgresdocOapi() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_OAPI);
}

void CfMainFrame::OnIgresdocOme() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_OME);
}

void CfMainFrame::OnIgresdocOsql() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_OSQL);
}

void CfMainFrame::OnIgresdocQrytools() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_QRYTOOLS);
}

void CfMainFrame::OnIgresdocQuelref() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_QUELREF);
}

void CfMainFrame::OnIgresdocRep() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_REP);
}

void CfMainFrame::OnIgresdocSqlref() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_SQLREF);
}

void CfMainFrame::OnIgresdocStarug() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_STARUG);
}

void CfMainFrame::OnIgresdocSysadm() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_SYSADM);
}

void CfMainFrame::OnIgresdocUfadt() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_UFADT);
}

void CfMainFrame::OnIgresdocConnectivity()
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_INGNET);
}

void CfMainFrame::OnIgresdocIpm()
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_IPM);
}

void CfMainFrame::OnIgresdocReleaseSummary()
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_IGRESDOC_RELEASE);
}

void CfMainFrame::OnAppAbout() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_ABOUT_ADVANTAGE);
}

void CfMainFrame::OnVcbf() 
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_VCBF);
}

void CfMainFrame::OnIngresPrompt()
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		pDoc->UpdateAllViews(NULL, (LPARAM)ID_INGRES_PROMPT);
}

HICON CfMainFrame::GetIconForItem(UINT itemID) const
{
    HICON hIcon = (HICON)0;
	if (GVvista())
	{
    if (IS_INTRESOURCE(itemID))
    {    
        hIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(itemID), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
    }

    if (!hIcon)
    {
        CString sItem; // look for a named item in resources

        GetMenu()->GetMenuString(itemID, sItem, MF_BYCOMMAND);
        sItem.Replace(_T(' '), _T('_')); // cannot have resource items with space in name

        if (!sItem.IsEmpty())
            hIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), sItem, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
    }
	}
    return hIcon;
}

void CfMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpdis)
{
    if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
    {
        CFrameWnd::OnDrawItem(nIDCtl, lpdis);
        return; //not for a menu
    }

    HICON hIcon = GetIconForItem(lpdis->itemID);
    if (hIcon)
    {
        ICONINFO iconinfo;
        ::GetIconInfo(hIcon, &iconinfo);

        BITMAP bitmap;
        ::GetObject(iconinfo.hbmColor, sizeof(bitmap), &bitmap);
        ::DeleteObject(iconinfo.hbmColor);
        ::DeleteObject(iconinfo.hbmMask);

        ::DrawIconEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon, bitmap.bmWidth, bitmap.bmHeight, 0, NULL, DI_NORMAL);
//      ::DestroyIcon(hIcon); // we use LR_SHARED instead
    }
}

void CfMainFrame::OnInitMenuPopup(CMenu* pMenu, UINT nIndex, BOOL bSysMenu)
{
    MENUINFO mnfo;
    AfxTrace(_T(__FUNCTION__) _T(": %#0x\n"), pMenu->GetSafeHmenu());
    CFrameWnd::OnInitMenuPopup(pMenu, nIndex, bSysMenu);

    if (bSysMenu)
    {
        pMenu = GetSystemMenu(FALSE);
    }
    mnfo.cbSize = sizeof(mnfo);
    mnfo.fMask = MIM_STYLE;
    mnfo.dwStyle = MNS_CHECKORBMP | MNS_AUTODISMISS;
    pMenu->SetMenuInfo(&mnfo);

    MENUITEMINFO minfo;
    minfo.cbSize = sizeof(minfo);

    for (UINT pos=0; pos < pMenu->GetMenuItemCount(); pos++)
    {
        minfo.fMask = MIIM_FTYPE | MIIM_ID;
        pMenu->GetMenuItemInfo(pos, &minfo, TRUE);

        HICON hIcon = GetIconForItem(minfo.wID);

        if (hIcon && !(minfo.fType & MFT_OWNERDRAW))
        {
            AfxTrace(_T("replace for id=%#0x\n"), minfo.wID);

            minfo.fMask = MIIM_FTYPE | MIIM_BITMAP;
            minfo.hbmpItem = HBMMENU_CALLBACK;
            minfo.fType = MFT_STRING;

            pMenu->SetMenuItemInfo(pos, &minfo, TRUE);
        }
        else
             AfxTrace(_T("keep for id=%#0x\n"), minfo.wID);
//        ::DestroyIcon(hIcon); // we use LR_SHARED instead
    }
}

void CfMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpmis)
{
    if ((lpmis==NULL)||(lpmis->CtlType != ODT_MENU))
    {
        CFrameWnd::OnMeasureItem(nIDCtl, lpmis); //not for a menu
        return;
    }

    lpmis->itemWidth = 16;
    lpmis->itemHeight = 16;

    CString sItem;
    GetMenu()->GetMenuString(lpmis->itemID, sItem, MF_BYCOMMAND);

    HICON hIcon = GetIconForItem(lpmis->itemID);

    if (hIcon)
    {
        ICONINFO iconinfo;
        ::GetIconInfo(hIcon, &iconinfo);

        BITMAP bitmap;
        ::GetObject(iconinfo.hbmColor, sizeof(bitmap), &bitmap);
        ::DeleteObject(iconinfo.hbmColor);
        ::DeleteObject(iconinfo.hbmMask);

        lpmis->itemWidth = bitmap.bmWidth;
        lpmis->itemHeight = bitmap.bmHeight;

        AfxTrace(_T(__FUNCTION__) _T(": %d \"%s\"%dx%d ==> %dx%d\n"), (WORD)lpmis->itemID, (LPCTSTR)sItem, bitmap.bmWidth, bitmap.bmHeight, lpmis->itemWidth, lpmis->itemHeight);
    }
}


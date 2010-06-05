/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp
**    Project  : CA-Visual DBA,
**
** History: (after 24-Feb-2000)
**
** 24-Feb-2000 (uk$so01)
**    SIR #100558. 
**    Add a menu item in the main menu to open the hisory of error dialog box.
** 20-Nov-2000 (schph01)
**    SIR #102821
**    Add Help context ID for comment menu
** 21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 11-Apr-2001 (schph01)
**    (sir 103292) add help context ID for 'usermod' command.
** 31-May-2001 (zhayu01) SIR 104824 for EDBC
**    Use the title name to set the DOM window title for vnodeless connections.
** 07-Nov-2001 (uk$so01)
**    SIR #106290, new corporate guidelines and standards about box.
** 19-Feb-2002 (schph01)
**    SIR #107122 enable the "space calculation" icon even if no DOM is open.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate (ipm and sqlquery).ocx.
** 02-feb-2004 (somsa01)
**    Updated OnActivateApp()'s second parameter to match the definition: DWORD.
** 03-Feb-2004 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX or COM:
**    Integrate Export assistant into vdba
** 09-May-2007 (karye01)
**    SIR #118282, added OnUpdateButtonOnlineSupport.
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
** 19-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings.
** 13-May-2010 (drivi01)
**    Update ConvertCommandId by adding new ID_BUTTON_CREATEIDX 
**    to the list.
**/

// MainFrm.cpp : implementation of the CMainFrame class
//
#define _PERSISTANT_STATE_

#include "stdafx.h"

#include "mainmfc.h"

#include "mainfrm.h"
#include "frmvnode.h"   // For Virtual Node MDI Child
#include "vewvnode.h"   // For Virtual Node MDI Child
#include "docvnode.h"   // For Virtual Node MDI Child

#include "toolbars.h"   // Toolbars dialog box

// Save/Load
#include "cmixfile.h"
#include "monitor.h"

#include "vnitem.h"     // CuTreeServer

#include "splash.h"
#include "sqlquery.h"
#include "taskxprm.h"
#include "extccall.h"
#include "cmdline.h"
#include "property.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dbafile.h"    // MakeNewDomMDIChild
#include "dbaginfo.h"   // GetGWType
#include "winutils.h"   // IconCache management
//#define NOLOADSAVE  // temporary for unix version

// from main.c 
BOOL MainOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void MainOnSize(HWND hwnd, UINT state, int cx, int cy);
void MainOnTimer(HWND hwnd, UINT id);
void MainOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void MainOnF1Down (int nCode);
void MainOnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags);
void MainOnActivateApp(BOOL fActive);
void MainOnDestroy(HWND hwnd);
void MainOnWinIniChange(HWND hwnd, LPCSTR lpszSectionName);

BOOL ProcessFindDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CleanupAndExit(HWND hwnd);

#include "resource.h"   // hdr directory - IDM_xyz

#include "settings.h"   // bBkRefreshOn
#include "dgerrh.h"
};

// C/C++ interface for product name
#include "prodname.h"

/////////////////////////////////////////////////////////////////////////////
// special functions for Open Environment

static BOOL bOpenEnvSuccessFlag = FALSE;
extern "C" void SetOpenEnvSuccessFlag()   { bOpenEnvSuccessFlag = TRUE; }
static     void ResetOpenEnvSuccessFlag() { bOpenEnvSuccessFlag = FALSE; }
static     BOOL GetOpenEnvSuccessFlag()   { return bOpenEnvSuccessFlag; }

/////////////////////////////////////////////////////////////////////////////
// special functions

extern "C" HWND GetMfcNodesToolbarDialogHandle()
{
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  ASSERT (pMain);
  if (!pMain)
    return NULL;
  CuResizableDlgBar* pVNodeDlgBar = pMain->GetVNodeDlgBar();
  ASSERT (pVNodeDlgBar);
  if (!pVNodeDlgBar)
    return NULL;
  return pVNodeDlgBar->m_hWnd;
}

extern "C" BOOL IsNodesWindowVisible()
{
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  ASSERT (pMain);
  if (!pMain)
    return FALSE;
  return pMain->IsNodesWindowVisible();
}

extern "C" BOOL MfcDelayedUpdatesAccepted()
{
  return DelayedUpdatesAccepted();
}

extern "C" void MfcDelayUpdateOnNode(char *nodeName)
{
  DelayUpdateOnNode(nodeName);
}

void ForgetDelayedUpdates()
{
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  ASSERT (pMain);
  if (pMain)
    pMain->ForgetDelayedUpdates();
}

void AcceptDelayedUpdates()
{
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  ASSERT (pMain);
  if (pMain)
    pMain->AcceptDelayedUpdates();
}

BOOL DelayedUpdatesAccepted()
{
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  ASSERT (pMain);
  if (pMain)
    return pMain->DelayedUpdatesAccepted();
  else
    return FALSE;
}

void DelayUpdateOnNode(char *nodeName)
{
  CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMain);
  BOOL bOk = pMain->PostMessage(WM_USER_UPDATEOPENWIN, 0, (LPARAM)nodeName);
  ASSERT (bOk);
}

extern "C" void UpdateNodeUsersList(char *nodeName)
{
  CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMain);
  pMain->UpdateNodeUsersList(nodeName);
}

/////////////////////////////////////////////////////////////////////////////
// static utilities

static void PreManageDocMaximizePref()
{
  // Update maximize state according to current doc state
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMainFrame);
  CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
  if (pActiveMDI) {
    BOOL bZoomed = pActiveMDI->IsZoomed();
    theApp.SetLastDocMaximizedState(bZoomed);
  }
}

static void PostManageDocMaximizePref()
{
  // Maximize the last doc created, if requested
  BOOL bLastDocMaximizedState = theApp.GetLastDocMaximizedState();
  if (bLastDocMaximizedState) {
    CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT (pMainFrame);
    CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
    ASSERT (pActiveMDI);
    if (pActiveMDI)
      pMainFrame->MDIMaximize(pActiveMDI);
  }
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CONNECT, OnUpdateButtonConnect)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DISCONNECT, OnUpdateButtonDisconnect)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PREFERENCES, OnUpdateButtonPreferences)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_VERT, OnUpdateWindowTileVert)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ADDOBJECT, OnUpdateButtonAddobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ALTEROBJECT, OnUpdateButtonAlterobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DROPOBJECT, OnUpdateButtonDropobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PROPERTIES, OnUpdateButtonProperties)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESH, OnUpdateButtonRefresh)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FILTER1, OnUpdateButtonFilter1)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SYSOBJECT, OnUpdateButtonSysobject)
	ON_COMMAND(ID_BUTTON_ADDOBJECT, OnButtonAddobject)
	ON_COMMAND(ID_BUTTON_ALTEROBJECT, OnButtonAlterobject)
	ON_COMMAND(ID_BUTTON_DROPOBJECT, OnButtonDropobject)
	ON_COMMAND(ID_BUTTON_PROPERTIES, OnButtonProperties)
	ON_COMMAND(ID_BUTTON_REFRESH, OnButtonRefresh)
	ON_COMMAND(ID_BUTTON_FILTER1, OnButtonFilter1)
	ON_COMMAND(ID_BUTTON_SYSOBJECT, OnButtonSysobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SQLACT, OnUpdateButtonSqlact)
	ON_MESSAGE(WM_SETUPMESSAGE, OnSetupMessage)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_OPEN, OnUpdateButtonOpen)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_NEW, OnUpdateButtonNew)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SAVE, OnUpdateButtonSave)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SAVE_AS, OnUpdateButtonSaveAs)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PRINT, OnUpdateButtonPrint)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_EXIT, OnUpdateButtonExit)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_HORZ, OnUpdateWindowTileHorz)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CASCADE, OnUpdateWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_ARRANGE, OnUpdateWindowArrange)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SELECTWINDOW, OnUpdateButtonSelectwindow)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_WINDOWCLOSEALL, OnUpdateButtonWindowcloseall)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ABOUT, OnUpdateButtonAbout)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_HELPINDEX, OnUpdateButtonHelpindex)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_HELPSEARCH, OnUpdateButtonHelpsearch)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CONTEXTHELP, OnUpdateButtonContexthelp)
        ON_UPDATE_COMMAND_UI(ID_BUTTON_ONLINESUPPORT, OnUpdateButtonOnlineSupport)
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CHMODREFRESH, OnUpdateButtonChmodrefresh)
	ON_COMMAND(ID_BUTTON_CHMODREFRESH, OnButtonChmodrefresh)
	ON_MESSAGE(WM_F1DOWN, OnF1DownMessage)
	ON_WM_MENUSELECT()
	ON_UPDATE_COMMAND_UI(ID_SERVERS_VIEWNODESWINDOW, OnUpdateServersViewnodeswindow)
	ON_COMMAND(ID_SERVERS_VIEWNODESWINDOW, OnServersViewnodeswindow)
	ON_COMMAND(IDM_TOOLBARS, OnToolbars)
	ON_COMMAND(ID_SERVERS_HIDENODESWINDOW, OnServersHidenodeswindow)
	ON_UPDATE_COMMAND_UI(ID_SERVERS_HIDENODESWINDOW, OnUpdateServersHidenodeswindow)
	ON_WM_INITMENUPOPUP()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
	ON_WM_ACTIVATEAPP()
	ON_WM_DESTROY()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTEISCHANGING()
	ON_COMMAND(ID_BUTTON_HISTORYERROR, OnButtonHistoryerror)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SPACECALC, OnUpdateButtonSpacecalc)
	ON_COMMAND(IDM_PREFERENCE_SAVE, OnPreferenceSave)
	//}}AFX_MSG_MAP

	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR01, OnUpdateButton01)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR02, OnUpdateButton02)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR03, OnUpdateButton03)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR04, OnUpdateButton04)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR05, OnUpdateButton05)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR06, OnUpdateButton06)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR07, OnUpdateButton07)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR08, OnUpdateButton08)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR09, OnUpdateButton09)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR10, OnUpdateButton10)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR11, OnUpdateButton11)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBAR12, OnUpdateButton12)
	ON_UPDATE_COMMAND_UI (IDM_VNODEBARSC, OnUpdateButtonScratchpad)
	ON_UPDATE_COMMAND_UI (ID_SERVERS_TESTNODE, OnUpdateButtonTestNode)
	ON_UPDATE_COMMAND_UI (ID_SERVERS_INSTALLPSW, OnUpdateButtonInstallPassword)

	ON_COMMAND (IDM_VNODEBAR01, OnButton01)
	ON_COMMAND (IDM_VNODEBAR02, OnButton02)
	ON_COMMAND (IDM_VNODEBAR03, OnButton03)
	ON_COMMAND (IDM_VNODEBAR04, OnButton04)
	ON_COMMAND (IDM_VNODEBAR05, OnButton05)
	ON_COMMAND (IDM_VNODEBAR06, OnButton06)
	ON_COMMAND (IDM_VNODEBAR07, OnButton07)
	ON_COMMAND (IDM_VNODEBAR08, OnButton08)
	ON_COMMAND (IDM_VNODEBAR09, OnButton09)
	ON_COMMAND (IDM_VNODEBAR10, OnButton10)
	ON_COMMAND (IDM_VNODEBAR11, OnButton11)
	ON_COMMAND (IDM_VNODEBAR12, OnButton12)
	ON_COMMAND (IDM_VNODEBARSC, OnButtonScratchpad)
	ON_COMMAND (ID_SERVERS_TESTNODE, OnButtonTestNode)
	ON_COMMAND (ID_SERVERS_INSTALLPSW, OnButtonInstallPassword)

	ON_MESSAGE (WM_USER_UPDATEOPENWIN, OnUpdateOpenwin)
	ON_MESSAGE (WM_EXECUTE_TASK, OnActionThreadAnimate)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator

	ID_GLOBSTATUS_CONNSRV_CAPT,   // connected server(s)
	ID_GLOBSTATUS_CONNSRV_VAL,
	//ID_GLOBSTATUS_CURRSRV_CAPT,   // current server
	//ID_GLOBSTATUS_CURRSRV_VAL,
	ID_GLOBSTATUS_OBJECTS_CAPT,   // number of objects
	ID_GLOBSTATUS_OBJECTS_VAL,
	//ID_GLOBSTATUS_TIME,           // current time

	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_IpmSeqNum     = 0;
	m_DbeventSeqNum = 0;
	m_SqlactSeqNum  = 0;

  // for virtual nodes
  m_bNodesVisible       = TRUE;
  m_bNodesToolbarMode   = TRUE;
  m_pVirtualNodeMdiDoc  = 0;

  // delayed mode
  m_bAcceptDelay = TRUE;

  // "no save on exit" context-driven option
  m_bNoSaveOnExit = FALSE;
}

CMainFrame::~CMainFrame()
{
}

extern CMainMfcApp theApp;
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Change title according to vdba requested name
	char buf[BUFSIZE];
	VDBA_GetProductCaption(buf, sizeof(buf));
	SetWindowText(buf);
	// Update m_strTitle for future doc activation title preserve
	GetWindowText(m_strTitle);

	TRACE0("About to show splash screen\n");
		CSplashWnd::ShowSplashScreen(this);
	TRACE0("Splash screen just shown\n");

	// Ensure splash screen will stay at least 1 second
	Sleep(1000);

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	theApp.m_pMainWnd = this;
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	//
	// Status Panes adjust style - assume default to SBPS_NORMAL
	//
	m_wndStatusBar.SetPaneStyle(1, SBPS_NOBORDERS);
	m_wndStatusBar.SetPaneStyle(3, SBPS_NOBORDERS);

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);

	// Set caption to toolbar
	CString str;
	str.LoadString (IDS_APPBAR_TITLE);
	m_wndToolBar.SetWindowText (str);

	//
	// Virtual node control bar
	//
	if (!m_wndVirtualNode.Create(this, 
		IDD_VIRTUALNODEBAR,
		WS_CHILD|WS_THICKFRAME|
		CBRS_SIZE_DYNAMIC  | CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE,
		IDD_VIRTUALNODEBAR))
	{
		TRACE0("Failed to create dialog bar m_wndVirtualNode\n");
		return -1;		// fail to create
	}
	m_wndVirtualNode.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT|CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndVirtualNode);
	CString strTitle;
	strTitle.LoadString (IDS_VIRTNODE_TITLE);
	m_wndVirtualNode.SetWindowText (strTitle);

	// Set tree font without redraw
	m_wndVirtualNode.m_Tree.SetFont (CFont::FromHandle(hFontNodeTree), FALSE);

	//
	// manage initialization code for mainlib
	//

	// frame window handle
	hwndFrame = m_hWnd;

	// client mdi window handle
	CWnd *pChild = GetWindow(GW_CHILD);
	while (pChild) {
		char className[12];
		className[0] = '\0';
		GetClassName(pChild->m_hWnd, className, sizeof(className)-1);
		if (!x_stricmp(className, "mdiclient")) {
			hwndMDIClient = pChild->m_hWnd;
			break;
		}
		pChild = pChild->GetWindow(GW_HWNDNEXT);
	}

	// create code
	if (!MainOnCreate(m_hWnd, lpCreateStruct))
		return -1;

	/* original insertion point by components wizard
		// CG: The following line was added by the Splash Screen component.
		CSplashWnd::ShowSplashScreen(this);
	*/
	PermanentState (TRUE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
    CMDIFrameWnd::OnSize(nType, cx, cy);
    RecalcLayout ();
    MainOnSize(m_hWnd, nType, cx, cy);
}

void CMainFrame::OnPaint() 
{
  CSplashWnd::VdbaHideSplashScreen();

	CPaintDC dc(this);
	RecalcLayout();
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
  MainOnTimer(m_hWnd, nIDEvent);
	
	CMDIFrameWnd::OnTimer(nIDEvent);
}

// special value for ignored id(s)
#define IDM_IGNORE        ((UINT)(-1))
#define IDM_IGNORE_RESET  ((UINT)(-2))
#define IDM_NOCONTEXTHELP ((UINT)(-3))

UINT CMainFrame::ConvertCommandId(UINT cppId, BOOL bContextHelp)
{
  //
  // First step for context help specific
  //
  // NOTE : OFFSETHELPMENU (20000) will be added to returned ids
  //
  if (bContextHelp) {
    if (cppId >= 0xFF00)  // open documents list
      return IDM_NOCONTEXTHELP;

    switch (cppId) {
      // "View" menu
      case ID_VIEW_DBEVENTLISTFORCEREFRESH: return 3027;  // "DB Event list force refresh"
      case IDM_TOOLBARS:                    return 3021;  // "Toolbars..."
      case ID_EDIT_CLEARLIST:               return 3025;  // "Clear Raised DBEvents List"
      case ID_EDIT_POPUPONRAISE:            return 3026;  // "Popup on Raise"
      case IDM_VNODEBAR12:                  return 3022;  // "Nodes Window Toolbar Mode"
      case ID_SERVERS_VIEWNODESWINDOW:      return 3023;  // "Show Nodes Window"
      case ID_BUTTON_REFRESH:               return 3024;  // "Force refresh" (Dom) - Exception - was 23007
      case IDM_IPMBAR_REFRESH:              return 3028;  // "Force Refresh" (Monitor)

      // "Node" menu
      case IDM_VNODEBAR01:  return 9002;  // "Connect/DOM"
      case IDM_VNODEBAR02:  return 9003;  // "SQL Test"
      case IDM_VNODEBAR03:  return 9004;  // "Performance Monitor"
      case IDM_VNODEBAR04:  return 9005;  // "DBEvent Trace"
      case IDM_VNODEBARSC:  return 9006;  // "DOM Scratchpad" - Exception - was 27008
      case IDM_VNODEBAR05:  return 9007;  // "Disconnect"
      case IDM_VNODEBAR08:  return 9008;  // "Add Node Definition ..."
      case IDM_VNODEBAR09:  return 9009;  // "Alter Node Definition ..."
      case IDM_VNODEBAR10:  return 9010;  // "Drop Node Definition ..."
      case IDM_VNODEBAR11:  return 9011;  // "Force Node List Refresh"

      case ID_SERVERS_TESTNODE:  return 9015; // Test node
      case ID_SERVERS_INSTALLPSW: return 9016;  // Add installation password

      // "Edit" menu
      case IDM_IPMBAR_SHUTDOWN: return 2020;  // "Remove / Shut Down"

      // Node popup menu items not in main menu
      case IDM_VNODEBAR06:              // Close Window
        return 9012;
      case IDM_VNODEBAR07:              // Activate Window
        return 9013;
      case ID_SERVERS_HIDENODESWINDOW:  // Hide
        return 9014;

      case ID_BUTTON_INFODB:    return 9018;    // InfoDb for database
      case ID_BUTTON_FASTLOAD:  return 9019;    // Fast load into a table
      case ID_BUTTON_COMMENTS:    return 9026;  // Comment on table,view or columns
      case ID_BUTTON_USERMOD:     return 9027;  // Usermod utility on database or table
      case ID_BUTTON_DUPLICATEDB: return 9020;  // Duplicate Database

      case ID_BUTTON_SECURITYAUDIT: return 9021;
      case ID_BUTTON_EXPIREDATE:    return 9022;

      case IDM_IPMBAR_CLOSESVR:   return 9023;
      case IDM_IPMBAR_OPENSVR:    return 9024;

      case ID_BUTTON_JOURNALING:    return 9025;
      case ID_BUTTON_HISTORYERROR:    return ID_BUTTON_HISTORYERROR;
      case IDM_PREFERENCE_SAVE: return IDM_PREFERENCE_SAVE;

      default:
        break;      // continue translations
    }
    
  }
  //
  // End of context help specific
  //

  // reserved menuitems
  if (cppId == IDM_TOOLBARS)
    return IDM_IGNORE;
  if (cppId ==IDM_IPMBAR_REPLICSVR_CHANGERUNNODE)
    return IDM_IGNORE_RESET;


  /* Obsolete:
  // New mdi documents launch special management
  if (cppId == ID_BUTTON_MONITOR)
    return ID_BUTTON_MONITOR;
  if (cppId == ID_BUTTON_DBEVENT)
    return ID_BUTTON_DBEVENT;
  */

  // ipm special commands: ignored
  switch (cppId) {
    case IDM_IPMBAR_REFRESH:
    case IDM_IPMBAR_SHUTDOWN:
    case IDM_IPMBAR_REPLICSVR_CHANGERUNNODE:
    case IDM_IPMBAR_CLOSESVR:
    case IDM_IPMBAR_OPENSVR:
    case IDM_PREFERENCE_SAVE:

      return IDM_IGNORE;
  }

  // Vnode bar "Dialog like" commands : ignored
  switch (cppId) {
    case IDOK:
    case IDCANCEL:
      return IDM_IGNORE;
  }

  // Vnode commands : ignored
  switch (cppId) {
    case IDM_VNODEBAR01:
    case IDM_VNODEBAR02:
    case IDM_VNODEBAR03:
    case IDM_VNODEBAR04:
    case IDM_VNODEBAR05:
    case IDM_VNODEBAR06:
    case IDM_VNODEBAR07:
    case IDM_VNODEBAR08:
    case IDM_VNODEBAR09:
    case IDM_VNODEBAR10:
    case IDM_VNODEBAR11:
    case IDM_VNODEBAR12:
    case IDM_VNODEBARSC:
    case ID_SERVERS_VIEWNODESWINDOW:
    case ID_SERVERS_HIDENODESWINDOW:
      return IDM_IGNORE;
  }

  // dbevent commands : ignored
  switch (cppId) {
    case IDM_DBEVENT_RAISE_RESET:
    case ID_EDIT_CLEARLIST:
    case ID_EDIT_POPUPONRAISE:
      return IDM_IGNORE;
  }

  // context help special cases
  if (bContextHelp) {
    if (cppId >= 0xFF00)
      return IDM_IGNORE_RESET;
    switch (cppId) {
      case ID_WINDOW_TILE_VERT: return IDM_WINDOWTILE_VERT;
      case ID_WINDOW_TILE_HORZ: return IDM_WINDOWTILE_HORZ;
      case ID_WINDOW_CASCADE:   return IDM_WINDOWCASCADE;
      case ID_WINDOW_ARRANGE:   return IDM_WINDOWICONS;

      // document system menu when maximized
      case SC_RESTORE:
      case SC_MOVE:
      case SC_SIZE:
      case SC_MINIMIZE:
      case SC_MAXIMIZE:
      case SC_CLOSE:
      case SC_NEXTWINDOW:
      case SC_PREVWINDOW:
        return IDM_IGNORE_RESET;
    }
  }

  // standard management
  switch (cppId) {
    case ID_BUTTON_ADDOBJECT             :return IDM_ADDOBJECT            ;
    case ID_BUTTON_ALTEROBJECT           :return IDM_ALTEROBJECT          ;
    case ID_BUTTON_DROPOBJECT            :return IDM_DROPOBJECT           ;
    case ID_BUTTON_REMOVEOBJECT          :return IDM_REMOVEOBJECT         ;
    case ID_BUTTON_CONNECT               :return IDM_CONNECT              ;
    case ID_BUTTON_DISCONNECT            :return IDM_DISCONNECT           ;
    case ID_BUTTON_PREFERENCES           :return IDM_PREFERENCES          ;
    case ID_BUTTON_FIND                  :return IDM_FIND                 ;
    case ID_BUTTON_SQLACT                :return IDM_SQLACT               ;
    case ID_BUTTON_SPACECALC             :return IDM_SPACECALC            ;
    case ID_BUTTON_TEAROUT               :return IDM_TEAROUT              ;
    case ID_BUTTON_RESTARTFROMPOS        :return IDM_RESTARTFROMPOS       ;
    case ID_BUTTON_GRANT                 :return IDM_GRANT                ;
    case ID_BUTTON_REVOKE                :return IDM_REVOKE               ;
    case ID_BUTTON_POPULATE              :return IDM_POPULATE             ;
    case ID_BUTTON_EXPORT                :return IDM_EXPORT             ;
    case ID_BUTTON_LOCATE                :return IDM_LOCATE               ;
    case ID_BUTTON_CLASSB_EXPANDONE      :return IDM_CLASSB_EXPANDONE     ;
    case ID_BUTTON_CLASSB_EXPANDBRANCH   :return IDM_CLASSB_EXPANDBRANCH  ;
    case ID_BUTTON_CLASSB_EXPANDALL      :return IDM_CLASSB_EXPANDALL     ;
    case ID_BUTTON_CLASSB_COLLAPSEONE    :return IDM_CLASSB_COLLAPSEONE   ;
    case ID_BUTTON_CLASSB_COLLAPSEBRANCH :return IDM_CLASSB_COLLAPSEBRANCH;
    case ID_BUTTON_CLASSB_COLLAPSEALL    :return IDM_CLASSB_COLLAPSEALL   ;
    case ID_BUTTON_REFRESH               :return IDM_REFRESH              ;
    case ID_BUTTON_CHMODREFRESH          :return IDM_CHMODREFRESH         ;
    case ID_BUTTON_FILTER                :return IDM_FILTER               ;
    case ID_BUTTON_SHOW_SYSTEM           :return IDM_SHOW_SYSTEM          ;
    case ID_BUTTON_PROPERTIES            :return IDM_PROPERTIES           ;
    case ID_BUTTON_CHECKPOINT            :return IDM_CHECKPOINT           ;
    case ID_BUTTON_ROLLFORWARD           :return IDM_ROLLFORWARD          ;
    case ID_BUTTON_UNLOADDB              :return IDM_UNLOADDB             ;
    case ID_BUTTON_COPYDB                :return IDM_COPYDB               ;
    case ID_BUTTON_GENSTAT               :return IDM_GENSTAT              ;
    case ID_BUTTON_DISPSTAT              :return IDM_DISPSTAT             ;
    case ID_BUTTON_ALTERDB               :return IDM_ALTERDB              ;
    case ID_BUTTON_AUDIT                 :return IDM_AUDIT                ;
    case ID_BUTTON_MODIFYSTRUCT          :return IDM_MODIFYSTRUCT         ;
    case ID_BUTTON_CREATEIDX		 :return IDM_CREATEIDX			  ;
    case ID_BUTTON_SYSMOD                :return IDM_SYSMOD               ;
    case ID_BUTTON_VERIFYDB              :return IDM_VERIFYDB             ;
    case ID_BUTTON_REPLIC_INSTALL        :return IDM_REPLIC_INSTALL       ;
    case ID_BUTTON_REPLIC_PROPAG         :return IDM_REPLIC_PROPAG        ;
    case ID_BUTTON_REPLIC_BUILDSRV       :return IDM_REPLIC_BUILDSRV      ;
    case ID_BUTTON_REPLIC_RECONCIL       :return IDM_REPLIC_RECONCIL      ;
    case ID_BUTTON_REPLIC_DEREPLIC       :return IDM_REPLIC_DEREPLIC      ;
    case ID_BUTTON_REPLIC_MOBILE         :return IDM_REPLIC_MOBILE        ;
    case ID_BUTTON_REPLIC_ARCCLEAN       :return IDM_REPLIC_ARCCLEAN      ;
    case ID_BUTTON_REPLIC_REPMOD         :return IDM_REPLIC_REPMOD        ;
    case ID_BUTTON_REPLIC_CREATEKEYS     :return IDM_REPLIC_CREATEKEYS    ;
    case ID_BUTTON_REPLIC_ACTIVATE       :return IDM_REPLIC_ACTIVATE      ;
    case ID_BUTTON_REPLIC_DEACTIVATE     :return IDM_REPLIC_DEACTIVATE    ;
    case ID_BUTTON_QUERYCLEAR            :return IDM_QUERYCLEAR           ;
    case ID_BUTTON_QUERYOPEN             :return IDM_QUERYOPEN            ;
    case ID_BUTTON_QUERYSAVEAS           :return IDM_QUERYSAVEAS          ;
    case ID_BUTTON_SQLWIZARD             :return IDM_SQLWIZARD            ;
    case ID_BUTTON_QUERYGO               :return IDM_QUERYGO              ;
    case ID_BUTTON_QUERYSHOWRES          :return IDM_QUERYSHOWRES         ;
    case ID_BUTTON_NEW                   :return IDM_NEW                  ;
    case ID_BUTTON_OPEN                  :return IDM_OPEN                 ;
    case ID_BUTTON_SAVE                  :return IDM_SAVE                 ;
    case ID_BUTTON_SAVE_AS               :return IDM_SAVEAS               ;
    case ID_BUTTON_PRINT                 :return IDM_PRINT                ;
    case ID_BUTTON_EXIT                  :return IDM_FILEEXIT             ;
    case ID_BUTTON_LOAD                  :return IDM_LOAD                 ;
    case ID_BUTTON_UNLOAD                :return IDM_UNLOAD               ;
    case ID_BUTTON_DOWNLOAD              :return IDM_DOWNLOAD             ;
    case ID_BUTTON_RUNSCRIPTS            :return IDM_RUNSCRIPTS           ;
	  case ID_BUTTON_UPDSTAT               :return IDM_UPDSTAT              ;

    // windows management: useless for standard commands (not id_button_xxx)
    //case ID_WINDOW_TILE_VERT:           return IDM_WINDOWTILE_VERT;
    //case ID_WINDOW_TILE_HORZ:           return IDM_WINDOWTILE_HORZ;
    //case ID_WINDOW_CASCADE:             return IDM_WINDOWCASCADE;
    //case ID_WINDOW_ARRANGE:             return IDM_WINDOWICONS;
    case ID_BUTTON_SELECTWINDOW:        return IDM_SELECTWINDOW;
    case ID_BUTTON_WINDOWCLOSEALL:      return IDM_WINDOWCLOSEALL;

    // standard clipboard controls
    case ID_EDIT_CUT                     :return IDM_CUT;
    case ID_EDIT_COPY                    :return IDM_COPY;
    case ID_EDIT_PASTE                   :return IDM_PASTE;

	  case ID_BUTTON_NEWWINDOW             :return IDM_NEWWINDOW;

    // help/about menu : need to map on our stuff
    case ID_BUTTON_HELPINDEX             :return IDM_HELPINDEX  ;
    case ID_BUTTON_HELPSEARCH            :return IDM_HELPSEARCH ;
    case ID_BUTTON_CONTEXTHELP           :return IDM_CONTEXTHELP;
    case ID_BUTTON_ONLINESUPPORT         :return IDM_ONLINESUPPORT;
    case ID_BUTTON_ABOUT:                 return IDM_ABOUT;

    case ID_BUTTON_SQLERRORS:             return IDM_SQLERRORS;

    // need to ignore these messages
    case ID_HELP:   return IDM_IGNORE;

    // Star management
    case ID_BUTTON_REGISTERASLINK:        return IDM_REGISTERASLINK;
    case ID_BUTTON_REFRESHLINK:           return IDM_REFRESHLINK;

    // new buttons on sqlact toolbar
    case ID_BUTTON_QEP:                   return ID_BUTTON_QEP;   // Query Execution Plan
    case ID_BUTTON_RAW:                   return ID_BUTTON_RAW;   // Trace (toggle button)

    default:
      return 0;
  }
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
  // vcpp help says:
  // "An application returns nonzero if it processes this message;  otherwise 0."

  // forget cut/copy/paste by accelerator if rich edit doc
  if (HIWORD(wParam) == 1) {
    WORD cmd = LOWORD(wParam);
    if (cmd == ID_EDIT_CUT || cmd == ID_EDIT_COPY || cmd == ID_EDIT_PASTE) {
      CMDIFrameWnd* pMain = (CMDIFrameWnd*)AfxGetMainWnd();
      CMDIChildWnd* pMdi  = (CMDIChildWnd*)pMain->MDIGetActive();
      if (pMdi)
        if (QueryDocType(pMdi->m_hWnd) == TYPEDOC_SQLACT)
          return FALSE;
    }
  }

	BOOL bRet = CMDIFrameWnd::OnCommand(wParam, lParam);
  if (bRet)
    return bRet;   // processed

  UINT nID = LOWORD(wParam);
  nID = ConvertCommandId(nID);
  ASSERT(nID);
  if (nID && nID != IDM_IGNORE) {
    HWND hWndCtrl = (HWND)lParam;     // Control handle
    int nCode = HIWORD(wParam);       // Notification code

    // Call MainOnCommand, with special pre-management for some items
    switch (nID) {
      case IDM_OPEN:
        // reset sequential numbers for mfc mdi docs captions
     	  m_IpmSeqNum     = 0;
	      m_DbeventSeqNum = 0;
     	  m_SqlactSeqNum  = 0;

        // Open
        ResetOpenEnvSuccessFlag();
        ForgetDelayedUpdates();
        MainOnCommand(m_hWnd, nID, hWndCtrl, nCode);
        AcceptDelayedUpdates();
        break;
      case IDM_NEW:
        ForgetDelayedUpdates();
        MainOnCommand(m_hWnd, nID, hWndCtrl, nCode);
        AcceptDelayedUpdates();
        break;
      case IDM_ABOUT:
        theApp.OnAppAbout();
        break;
      case IDM_ONLINESUPPORT:
        ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
        break;
      default:
        MainOnCommand(m_hWnd, nID, hWndCtrl, nCode);
        break;
    }

    // post-management
    switch (nID) {
      case IDM_NEW:
        {
          // 0) Update m_strTitle for future doc activation title preserve
          GetWindowText(m_strTitle);

          // No more refresh of lowlevel and display for node tree, since it has been reset on NEW

          // reset sequential numbers for mfc mdi docs captions
        	m_IpmSeqNum     = 0;
	        m_DbeventSeqNum = 0;
        	m_SqlactSeqNum  = 0;
        }
        break;

      case IDM_OPEN:
        // If successful in opening environment:
        if (GetOpenEnvSuccessFlag()) {
          // 0) Update m_strTitle for future doc activation title preserve
          GetWindowText(m_strTitle);

          // 1)Update Windows handles in node bar
          UpdateOpenWindowsNodeDisplay();

          // 2) update node window special variables
          // for virtual nodes
          UpdateVirtualNodesVariables();
        }
        break;

      case IDM_SAVE:
      case IDM_SAVEAS:
        // 0) Update m_strTitle for future doc activation title preserve
        GetWindowText(m_strTitle);
        break;

    }

    return TRUE;    // assume processed
  }
  else
    return FALSE;
}

void CMainFrame::UpdateVirtualNodesVariables()
{

  // reset them all
  m_bNodesVisible       = FALSE;
  m_bNodesToolbarMode   = TRUE;
  m_pVirtualNodeMdiDoc  = 0;

  // 1) search for toolbar visibility
  if (m_wndVirtualNode.IsVisible()) {
    m_bNodesVisible = TRUE;
    return;
  }

  // 2) search for virtual node mdi document
  int   count = 0;
  HWND  hwndCurDoc;

  // get first document handle (with loop to skip the icon title windows)
  hwndCurDoc = ::GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && ::GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = ::GetWindow (hwndCurDoc, GW_HWNDNEXT);

  while (hwndCurDoc) {
    // test monitor on requested node
    if (QueryDocType(hwndCurDoc) == TYPEDOC_NODESEL) {
      m_bNodesVisible       = TRUE;
      m_bNodesToolbarMode   = FALSE;

      CMDIChildWnd *pFrame = (CMDIChildWnd *)FromHandlePermanent(hwndCurDoc);
      ASSERT (pFrame);
      CDocument* pTheDoc = pFrame->GetActiveDocument();
      ASSERT (pTheDoc);
      m_pVirtualNodeMdiDoc  = pTheDoc;
      return;
    }

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = ::GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && ::GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = ::GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }
}

void CMainFrame::OnUpdateButtonConnect(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable();
}

// exclusively called by extern "C" function MainFrmCreateDom()
// NEED NO HANDLER FOR THE COMMAND!
HWND CMainFrame::OnCreateDom()
{
  int oiVers = GetOIVers();

	CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);

  // prepare keyword
  CString csKeyword = _T("");
  if (oiVers == OIVERS_NOTOI) {
    // Idms, Datacomm, etc
    csKeyword = _T("MainMfNotOi");
  }
  else {
    // Pure Ingres
    csKeyword = _T("MainMf");
  }
  // unicenter driven menu options to be taken in account
  if (IsUnicenterDriven()) {
    CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
    if (pDescriptor->HasNoMenu())
      csKeyword = csKeyword + _T("NoMenu");
    else if (pDescriptor->HasObjectActionsOnlyMenu())
      csKeyword = csKeyword + _T("ObjMenu");
  }

  // Scan available templates
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    BOOL bFits = FALSE;
    if (docName == (LPCTSTR)csKeyword)
      bFits = TRUE;
    if (bFits) {
      CDocument *pNewDoc;
	  if (IsInvokedInContextWithOneWinExactly()) {
		pNewDoc = pTemplate->CreateNewDocument();
		if (pNewDoc) {
			CFrameWnd *pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
			if (pFrame) {
				char buf1[200];
				if (! VDBA_InvokedInContextOneDocExactlyGetDocName(buf1,sizeof(buf1)))
					x_strcpy(buf1,"");
#ifdef	EDBC
						if (buf1[0] == '@' && strstr(buf1, ","))
						{
							char edbctitle[200];
							GetContextDrivenTitleName(edbctitle);
							pNewDoc->SetTitle(edbctitle);
						}
						else
							pNewDoc->SetTitle(buf1);
#else
						pNewDoc->SetTitle(buf1);
#endif
				pNewDoc->OnNewDocument();
				pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
			}
			else {
				delete pNewDoc;
				pNewDoc=NULL;
			}
		}
	  }
	  else 
        pNewDoc = pTemplate->OpenDocumentFile(NULL);
      if (pNewDoc) {
        POSITION pos = pNewDoc->GetFirstViewPosition();
        CView* pFirstView = pNewDoc->GetNextView(pos);
        ASSERT (pFirstView);
        if (pFirstView)
          return pFirstView->m_hWnd;
      }
    }
  }
  return NULL;
}

// exclusively called by extern "C" function MainFrmCreateSqlAct()
// NEED NO HANDLER FOR THE COMMAND!
HWND CMainFrame::OnCreateSqlAct()
{
  CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    if (docName == "SqlAct") {
      CDocument *pNewDoc;
      pNewDoc = pTemplate->OpenDocumentFile(NULL);
      if (pNewDoc) {
        POSITION pos = pNewDoc->GetFirstViewPosition();
        CView* pFirstView = pNewDoc->GetNextView(pos);
        ASSERT (pFirstView);
        if (pFirstView)
          return pFirstView->m_hWnd;
      }
    }
  }
  return NULL;
}



void CMainFrame::OnUpdateButtonDisconnect(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonPreferences(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateWindowTileVert(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonAddobject(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable (TRUE);
    TRACE0 ("CMainFrame::OnUpdateButtonAddobject(CCmdUI* pCmdUI) ....\n");
}

void CMainFrame::OnUpdateButtonAlterobject(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CMainFrame::OnUpdateButtonDropobject(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CMainFrame::OnUpdateButtonProperties(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CMainFrame::OnUpdateButtonRefresh(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CMainFrame::OnUpdateButtonFilter1(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CMainFrame::OnUpdateButtonSysobject(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}



void CMainFrame::OnButtonAddobject() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonAlterobject() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonDropobject() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonProperties() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonRefresh() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonFilter1() 
{
	// TODO: Add your command handler code here
	
}

void CMainFrame::OnButtonSysobject() 
{
	// TODO: Add your command handler code here
	
}


/**************************************************************
  extern "C" functions, called by mainlib
**************************************************************/

extern "C" void MfcDocSetCaption(HWND hWnd, LPCSTR lpszTitle)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hWnd);
  ASSERT (pWnd);
  if (pWnd) {
    CView *pView = (CView *)pWnd;
    CDocument *pDoc = pView->GetDocument();
    ASSERT (pDoc);
    if (pDoc) {
		if (IsInvokedInContextWithOneWinExactly()) {
			char buf1[200];
			if (! VDBA_InvokedInContextOneDocExactlyGetDocName(buf1,sizeof(buf1)))
				x_strcpy(buf1,"");
			pDoc->SetTitle(buf1);
		}
		else
			pDoc->SetTitle(lpszTitle);
	}

    // Added April 1, 97: maintain frame toolbar text
    // pick frame, which can be several parents ahead (splitter bars between)
    CWnd *pParent = pWnd->GetParent();
    while (pParent) {
    	if(pParent->IsKindOf(RUNTIME_CLASS(CMDIChildWnd))) {
        CMDIChildWnd *pMdiParent = (CMDIChildWnd *)pParent;
        if (HasToolbar(pMdiParent)) {
          BOOL bResult = SetToolbarCaption(pMdiParent, lpszTitle);
          ASSERT (bResult);
        }
        break;
      }
      pParent = pParent->GetParent();
    }
    if (!pParent)
      ASSERT (FALSE);
  }
}

extern "C" HWND MainFrmCreateDom()
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  HWND hWnd = pMainFrame->OnCreateDom();
  return hWnd;
}

extern "C" HWND MainFrmCreateSqlAct()
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  return pMainFrame->OnCreateSqlAct();
}

/*
extern "C" HWND MainFrmCreateProperty()
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  return pMainFrame->OnProperty();
}
*/


// for bk refresh function - used in domdbkrf.c
extern "C" void MainFrmUpdateStatusBarPane0(char *text)
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CStatusBar *pStatusBar = pMainFrame->GetMainFrmStatusBar();
  ASSERT (pStatusBar);
  if (pStatusBar) {
    //pStatusBar->SetWindowText(text);
    pStatusBar->SetPaneText(0, text);
    pStatusBar->UpdateWindow();
  }
}

// for global status bar - used in main.c
extern "C" void MainFrmUpdateStatusBar(int nbConnSrv, char *srvName, char *bufNbObjects, char *bufDate, char *bufTime)
{
  // Trial for pane0 update problem when menuitem selected
  //return;
  // does not solve the problem!

  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CStatusBar *pStatusBar = pMainFrame->GetMainFrmStatusBar();
  ASSERT (pStatusBar);
  if (pStatusBar) {
    CString numval;
    numval.Format("%d", nbConnSrv);
    pStatusBar->SetPaneText(2, numval);   // connected servers
    pStatusBar->SetPaneText(4, bufNbObjects);
    //pStatusBar->SetPaneText(5, bufTime);
  }
}

extern "C" int MfcSaveSettings(FILEIDENT idFile)
{
  CMixFile *pMixFile = GetCMixFile();
  CArchive ar(pMixFile, CArchive::store, ARCHIVEBUFSIZE);

  try {
    DWORD dw = (DWORD)pMixFile->GetPosition();
    ASSERT (dw);
  
    CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT (pMainFrame);

    // tree lines in vnode tree
    CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
    ASSERT (pNodeBar);
    CTreeGlobalData *pGD = pNodeBar->GetPTreeGD();
    ASSERT (pGD);
    ar << pGD;

    // application toolbars states
    CDockState  toolbarsState;
    pMainFrame->GetDockState(toolbarsState);
    toolbarsState.Serialize(ar);

    // Complimentary for node window size
    BOOL bNodesFloating = pNodeBar->IsFloating();
    ar << bNodesFloating;
    ar << pNodeBar->m_sizeDocked;
    ar << pNodeBar->m_sizeFloating;

    ar.Close();
  }

  // a file exception occured
  catch (CFileException* pFileException)
  {
    pFileException->Delete();
    ar.Close();
    return RES_ERR;
  }
  // an archive exception occured
  catch (CArchiveException* pArchiveException)
  {
    VDBA_ArchiveExceptionMessage(pArchiveException);
    pArchiveException->Delete();
    return RES_ERR;
  }

  // successful
  return RES_SUCCESS;
}

extern "C" void MfcResetNodesTree()
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMainFrame);

  // delete all lines
  CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
  ASSERT (pNodeBar);
  CTreeGlobalData *pGD = pNodeBar->GetPTreeGD();
  ASSERT (pGD);
  CTreeCtrl *pTree = pGD->GetPTree();
  pGD->FreeAllTreeItemsData();
  BOOL bOk = pTree->DeleteAllItems();
  ASSERT (bOk);

  // only one root line
  CuTreeServerStatic* pItem = new CuTreeServerStatic (pGD);
  pItem->CreateTreeLine();
}

extern "C" int  MfcLoadSettings(FILEIDENT idFile)
{
  CMixFile *pMixFile = GetCMixFile();
  CArchive ar(pMixFile, CArchive::load, ARCHIVEBUFSIZE);

  try {
    DWORD dw = (DWORD)pMixFile->GetPosition();
    ASSERT (dw);
  
    CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
    ASSERT (pMainFrame);

    // tree lines in vnode tree
    CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
    ASSERT (pNodeBar);
    CTreeGlobalData *pGD = pNodeBar->GetPTreeGD();
    ASSERT (pGD);
    CTreeCtrl *pTree = pGD->GetPTree();
    pGD->FreeAllTreeItemsData();
    BOOL bOk = pTree->DeleteAllItems();
    ASSERT (bOk);
    pGD->SetPTree(NULL);
    delete pGD;
    pGD = NULL;
    ar >> pGD;
    pGD->SetPTree(pTree);           // need to set the image list!
    pGD->SetPSFilter(NULL);         // No filter
    pNodeBar->SetPTreeGD(pGD);
    pGD->FillTreeFromSerialList();

    // application toolbars states
    CDockState  toolbarsState;
    toolbarsState.Serialize(ar);
    pMainFrame->SetDockState(toolbarsState);

    // Complimentary for node window size
    BOOL bNodesFloating;
    ar >> bNodesFloating;
    ASSERT (bNodesFloating == pNodeBar->IsFloating());
    ar >> pNodeBar->m_sizeDocked;
    ar >> pNodeBar->m_sizeFloating;
    // ??? if (!pNodeBar->IsFloating())
    pMainFrame->RecalcLayout();

    ar.Close();
  }

  // a file exception occured
  catch (CFileException* pFileException)
  {
    pFileException->Delete();
    ar.Close();
    return RES_ERR;
  }
  // an archive exception occured
  catch (CArchiveException* pArchiveException)
  {
    VDBA_ArchiveExceptionMessage(pArchiveException);
    pArchiveException->Delete();
    return RES_ERR;
  }

  // successful
  return RES_SUCCESS;
}


/**************************************************************
  MainFrame handlers
**************************************************************/


void CMainFrame::OnUpdateButtonSqlact(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

LRESULT CMainFrame::OnSetupMessage(WPARAM wParam, LPARAM lParam)
{
  return 0L;
}

LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  if (ProcessFindDlg(m_hWnd, message, wParam, lParam))
    return 0L;

	return CMDIFrameWnd::WindowProc(message, wParam, lParam);
}

void CMainFrame::OnUpdateButtonOpen(CCmdUI* pCmdUI) 
{
#ifdef NOLOADSAVE
  pCmdUI->Enable (FALSE);
#else
  pCmdUI->Enable (TRUE);
#endif
}

void CMainFrame::OnUpdateButtonNew(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonSave(CCmdUI* pCmdUI) 
{
#ifdef NOLOADSAVE
  pCmdUI->Enable (FALSE);
#else
  pCmdUI->Enable (TRUE);
#endif
}

void CMainFrame::OnUpdateButtonSaveAs(CCmdUI* pCmdUI) 
{
#ifdef NOLOADSAVE
  pCmdUI->Enable (FALSE);
#else
  pCmdUI->Enable (TRUE);
#endif
}

void CMainFrame::OnUpdateButtonPrint(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;    // default to TRUE

  CMDIFrameWnd* pMain = (CMDIFrameWnd*)AfxGetMainWnd();
  CMDIChildWnd* pMdi  = (CMDIChildWnd*)pMain->MDIGetActive();

  if (!pMdi)
    bEnable = FALSE;
  else {
    int type = QueryDocType(pMdi->m_hWnd);
    switch (type) {
      case TYPEDOC_UNKNOWN   :
      case TYPEDOC_NONE      :
      case TYPEDOC_MONITOR   :
      case TYPEDOC_DBEVENT   :
      case TYPEDOC_NODESEL   :
        bEnable = FALSE;
        break;
    }
  }

  pCmdUI->Enable (bEnable);
}

void CMainFrame::OnUpdateButtonExit(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::RelayCommand (WPARAM wParam, LPARAM lParam)
{
    UINT nID = LOWORD(wParam);
    nID = ConvertCommandId(nID);
     ASSERT(nID);
    if (nID) {
        HWND hWndCtrl = (HWND)lParam;     // Control handle
        int nCode = HIWORD(wParam);       // Notification code
        MainOnCommand(m_hWnd, nID, hWndCtrl, nCode);
    }
}

BOOL CMainFrame::GetButtonState(UINT nID)
{
    //
    // nID is the Button Id: ID_BUTTON_XX (ex: ID_BUTTON_ALTEROBJECT)
    //
    return TRUE;
}


void CMainFrame::OnUpdateWindowTileHorz(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateWindowCascade(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateWindowArrange(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonSelectwindow(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonWindowcloseall(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonAbout(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonHelpindex(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonHelpsearch(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonContexthelp(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButtonOnlineSupport(CCmdUI* pCmdUI)
{
  pCmdUI->Enable (TRUE);
}

void CMainFrame::OnClose() 
{
	PermanentState(FALSE);
	// manage "no save on exit" if specified on context-driven command line
	if (NoSaveOnExit())
		bSaveRecommended = FALSE;

	// close only if cleanup and exit said "fine, you can close"
	if (CleanupAndExit(m_hWnd))
		CMDIFrameWnd::OnClose();
}

void CMainFrame::OnUpdateButtonChmodrefresh(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);     // always enabled
  int checkState;           // 0 unchecked, 1 checked
  checkState = bBkRefreshOn ? 1 : 0;
  pCmdUI->SetCheck(checkState);
}


void CMainFrame::OnButtonChmodrefresh() 
{
  WPARAM wParam = MAKEWPARAM((UINT)(ID_BUTTON_CHMODREFRESH), 0);
  LPARAM lParam = (LPARAM)0;
  RelayCommand (wParam, lParam);
}

LRESULT CMainFrame::OnF1DownMessage(WPARAM wParam, LPARAM lParam)
{
  MainOnF1Down ((int)wParam);
  return 0L;
}

void CMainFrame::OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu )
{
  // First, call base class to have status bar updated
  CMDIFrameWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);

  ManageMainOnMenuSelect(nItemID, nFlags, hSysMenu);
}

void CMainFrame::ManageMainOnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu )
{
  // Emb 23/06/93: Always need to call MainOnMenuSelect
  // in order to reset current context id if needed
  // otherwise old context id would be mistakenly used
  // at next help call
  UINT nID;
  if (nFlags == 0xFFFF && hSysMenu ==0)
    nID = 0;    // closing menu
  else if ((nFlags & MF_POPUP) || (nFlags & MF_SEPARATOR) || (nFlags & MF_SYSMENU))
    nID = 0;
  else {
    nID = ConvertCommandId(nItemID, TRUE);

    // checks : transformation should always take place for context help
    ASSERT(nID);
    ASSERT(nID != IDM_IGNORE);
    if (nID == IDM_IGNORE)
      nID = 0;

    // manage ids with no context help
    if (nID == IDM_IGNORE_RESET)
      nID = 0;
    if (nID == IDM_NOCONTEXTHELP)
      nID = 0;
  }

  MainOnMenuSelect(m_hWnd, hSysMenu, nID, NULL, nFlags);
}

//
// Message handlers for Nodes Dialog bar
//

BOOL CMainFrame::UpdateNodeDlgItemButton(UINT idMenu)
{
  // all is disabled if nodes window not visible
  if (!m_bNodesVisible)
    return FALSE;

  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (!pItem)
    return FALSE;
  return pItem->IsEnabled(idMenu);
}

void CMainFrame::OnUpdateButton01 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton02 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton03 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton04 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton05 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton06 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton07 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton08 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton09 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton10 (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CMainFrame::OnUpdateButton11 (CCmdUI* pCmdUI)
{
  //TRACE0 ("CMainFrame::OnUpdateButton11()...\n");
	pCmdUI->Enable (TRUE);
}

void CMainFrame::OnUpdateButton12 (CCmdUI* pCmdUI)
{
  if (m_bNodesToolbarMode)
    pCmdUI->SetCheck (1);
  else
    pCmdUI->SetCheck (0);
}

void CMainFrame::OnUpdateButtonScratchpad (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}


void CMainFrame::OnButton01()
{
    TRACE0 ("CMainFrame::OnButton01()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem) {
    PreManageDocMaximizePref();
    if (pItem->TreeConnect())
      PostManageDocMaximizePref();
  }
}

void CMainFrame::OnButton02()
{
    TRACE0 ("CMainFrame::OnButton02()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem) {
    PreManageDocMaximizePref();
    if (pItem->TreeSqlTest())
      PostManageDocMaximizePref();
  }
}

void CMainFrame::OnButton03()
{
    TRACE0 ("CMainFrame::OnButton03()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem) {
    PreManageDocMaximizePref();
    if (pItem->TreeMonitor())
      PostManageDocMaximizePref();
  }
}

void CMainFrame::OnButton04()
{
    TRACE0 ("CMainFrame::OnButton04()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem) {
    PreManageDocMaximizePref();
    if (pItem->TreeDbevent())
      PostManageDocMaximizePref();
  }
}

void CMainFrame::OnButton05()
{
    TRACE0 ("CMainFrame::OnButton05()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeDisconnect();
}

void CMainFrame::OnButton06()
{
    TRACE0 ("CMainFrame::OnButton06()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeCloseWin();
}

void CMainFrame::OnButton07()
{
    TRACE0 ("CMainFrame::OnButton07()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeActivateWin();
}

void CMainFrame::OnButton08()
{
    TRACE0 ("CMainFrame::OnButton08()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeAdd();
}

void CMainFrame::OnButton09()
{
    TRACE0 ("CMainFrame::OnButton09()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeAlter();
}

void CMainFrame::OnButton10()
{
    TRACE0 ("CMainFrame::OnButton10()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TreeDrop();
}

void CMainFrame::OnButton11()
{
    TRACE0 ("CMainFrame::OnButton11()...\n");
    CWaitCursor hourGlass;
    UpdateMonInfo(0, 0, NULL, OT_NODE);
    UpdateNodeDisplay();

    /* old code, obsolete + not good on login/connection subbranches:
    CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
    if (pItem)
      pItem->RefreshDisplayedItemsList(REFRESH_FORCE);
    */
}

// Modified Emb 24/03 since current doc may not be the "Virtual Node" document
void CMainFrame::OnButton12()
{
    TRACE0 ("CMainFrame::OnButton12()...\n");
    if (m_bNodesToolbarMode)
    {
        // 1) Hide Virtual Node Toolbar - no effect if toolbar visibility flag off
        if (m_bNodesVisible)
          ASSERT (m_wndVirtualNode.IsVisible());
        ShowControlBar (&m_wndVirtualNode, FALSE, TRUE);    	

        // 2) Only if toolbar visibility flag on : create MDI child doc
        ASSERT (m_pVirtualNodeMdiDoc == NULL);
        if (m_bNodesVisible)
        {
          CWinApp* appPtr = (CWinApp*)AfxGetApp ();
	        POSITION curTemplatePos = appPtr->GetFirstDocTemplatePosition();
	        while (curTemplatePos != NULL)
	        {
	    	    CDocTemplate* curTemplate = appPtr->GetNextDocTemplate (curTemplatePos);
	    	    CString str;
	    	    curTemplate->GetDocString(str, CDocTemplate::docName);
	    	    if(str == _T("Virtual Node"))
	    	    {
	    		    m_pVirtualNodeMdiDoc = curTemplate->OpenDocumentFile(NULL);
              ASSERT (m_pVirtualNodeMdiDoc);
	    		    break;
	    	    }
	        }
        }
        m_bNodesToolbarMode = FALSE;
    }
    else
    {
        // 1) close mdi doc if visibility flag state is on
        if (m_bNodesVisible)
        {
          ASSERT (m_pVirtualNodeMdiDoc != NULL);
          if (m_pVirtualNodeMdiDoc)
            m_pVirtualNodeMdiDoc->OnCloseDocument();
          // useless since managed in doc destructor: m_pVirtualNodeMdiDoc = NULL;
          m_bNodesVisible = TRUE;   // Mandatory to cancel management in doc destructor
        }

        // 2) Show Virtual Node Toolbar if visibility flag on
        ASSERT (!m_wndVirtualNode.IsVisible());
        if (m_bNodesVisible)
        {
          ShowControlBar (&m_wndVirtualNode, TRUE, TRUE); 
          m_wndVirtualNode.UpdateDisplay();
          RecalcLayout();
        }
        m_bNodesToolbarMode = TRUE;
    }
}

void CMainFrame::OnButtonScratchpad()
{
    TRACE0 ("CMainFrame::OnButtonScratchpad()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem) {
    PreManageDocMaximizePref();
    if (pItem->TreeScratchpad())
      PostManageDocMaximizePref();
  }
}


void CMainFrame::OnUpdateButtonTestNode(CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}


void CMainFrame::OnButtonTestNode()
{
  TRACE0 ("CMainFrame::OnButtonTestNode()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->TestNode();
}

void CMainFrame::OnUpdateButtonInstallPassword(CCmdUI* pCmdUI)
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}


void CMainFrame::OnButtonInstallPassword()
{
  TRACE0 ("CMainFrame::OnButtonInstallPassword()...\n");
  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  if (pItem)
    pItem->InstallPassword();
}

//
// End of Message handlers for Nodes Dialog bar
//


CTreeItemNodes * CMainFrame::GetSelectedVirtNodeItem()
{
  HTREEITEM hItem = m_wndVirtualNode.m_Tree.GetSelectedItem();
  if (!hItem)
    return 0;
  CTreeItemNodes *pItem = (CTreeItemNodes *)m_wndVirtualNode.m_Tree.GetItemData(hItem);
  ASSERT(pItem);
  return pItem;
}

void CMainFrame::OnUpdateServersViewnodeswindow(CCmdUI* pCmdUI) 
{
  // Preparation for toolbar mode
  if (m_bNodesToolbarMode)
    if (m_wndVirtualNode.IsVisible())
      m_bNodesVisible = TRUE;
    else
      m_bNodesVisible = FALSE;

  // final verdict
  if (m_bNodesVisible)
    pCmdUI->SetCheck (1);
  else
    pCmdUI->SetCheck (0);
}

void CMainFrame::OnServersViewnodeswindow() 
{
  if (m_bNodesToolbarMode) {
    if (m_bNodesVisible) {
      ShowControlBar (&m_wndVirtualNode, FALSE, TRUE);    // hides
      m_bNodesVisible = FALSE;
    }
    else {
      ShowControlBar (&m_wndVirtualNode, TRUE, TRUE);
      m_bNodesVisible = TRUE;
    }
    m_wndVirtualNode.UpdateDisplay();
    RecalcLayout();
  }
  else {
    if (m_bNodesVisible) {
      ASSERT (m_pVirtualNodeMdiDoc != NULL);
      if (m_pVirtualNodeMdiDoc)
        m_pVirtualNodeMdiDoc->OnCloseDocument();
      // useless since managed in doc destructor: m_pVirtualNodeMdiDoc = NULL;
      // useless since managed in doc destructor: m_bNodesVisible = FALSE;
    }
    else {
      ASSERT (m_pVirtualNodeMdiDoc == NULL);
      CWinApp* appPtr = (CWinApp*)AfxGetApp ();
	    POSITION curTemplatePos = appPtr->GetFirstDocTemplatePosition();
	    while (curTemplatePos != NULL)
	    {
	      CDocTemplate* curTemplate = appPtr->GetNextDocTemplate (curTemplatePos);
	      CString str;
	      curTemplate->GetDocString(str, CDocTemplate::docName);
	      if(str == _T("Virtual Node"))
	      {
	        m_pVirtualNodeMdiDoc = curTemplate->OpenDocumentFile(NULL);
          ASSERT (m_pVirtualNodeMdiDoc);
	      }
	    }
      ASSERT (m_pVirtualNodeMdiDoc);
      m_bNodesVisible = TRUE;
    }
  }
}


void CMainFrame::OnToolbars() 
{
  CDlgToolbars dlg;
  PushHelpId(IDD_TOOLBARS);
  PushVDBADlg();
  dlg.DoModal();
  PopVDBADlg();
  PopHelpId();
}

void CMainFrame::OnServersHidenodeswindow() 
{
  ASSERT (m_bNodesVisible);
  OnServersViewnodeswindow();
}

void CMainFrame::OnUpdateServersHidenodeswindow(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable();
}

LONG CMainFrame::OnUpdateOpenwin(WPARAM wParam, LPARAM lParam)
{
  // Modified 24/04/97 not to change selection in the nodes window
  UpdateOpenWindowsNodeDisplay();
  return (LONG)TRUE;

  /* OLD CODE, OBSOLETE
  char *nodeName = (char *)lParam;

  CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
  ASSERT(pItem);
  CTreeGlobalData *pGD  = pItem->GetPTreeGlobData();
  ASSERT (pGD);

  // use tree organization
  HTREEITEM hItem;
  hItem = pGD->GetPTree()->GetRootItem();
  ASSERT (hItem);
  if (hItem) {
    HTREEITEM hChildItem;
    hChildItem = pGD->GetPTree()->GetChildItem(hItem);
    while (hChildItem) {
      CTreeItem *pItem = (CTreeItem *)pGD->GetPTree()->GetItemData(hChildItem);
      ASSERT (pItem);
      if (pItem->GetDisplayName() == nodeName) {
        ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
        CuTreeServer *pItemServer = (CuTreeServer *)pItem;
        // HTREEITEM hOldSelItem = pGD->GetPTree()->GetSelectedItem();
        pGD->GetPTree()->SelectItem(hChildItem);
        pItemServer->UpdateOpenedWindowsList();
        pItemServer->RefreshDisplayedOpenWindowsList();
        // Masqued since hOldSelItem may not exist anymore!
        //pGD->GetPTree()->SelectItem(hOldSelItem);

        break;
      }
      hChildItem = pGD->GetPTree()->GetNextSiblingItem(hChildItem);
    }
    ASSERT (hChildItem);
  }

  return (LONG)TRUE;
  */
}

void CMainFrame::UpdateOpenWindowsNodeDisplay()
{
  // BIG NOTE: we process through all this stuff because we want
  // UpdateMonInfo NOT TO BE CALLED for OT_NODE, in order to be able
  // to load an environment when open-ingres is not launched.

  // However, the following question raises:
  // How will open-ingres installation be started afterwards?
  // Answer : discussed with FNN, who will do a fix which will allow
  // the installation to be started "as soon as needed" from inside vdba.

  CWaitCursor hourGlass;

  int   reqSize             = GetMonInfoStructSize(OT_NODE);
  void *pStructReq          = new char[reqSize];
  memset (pStructReq, 0, reqSize);
  int res = GetFirstMonInfo(0,                // hNode
                            NULL,             // iobjecttypeParent,
                            NULL,             // pstructParent,
                            OT_NODE,          // iobjecttypeReq,
                            pStructReq,       // pStructReq,
                            NULL);            // no filter
  // masqued 14/05 since may have an error if open-ingres not started : ASSERT (res == RES_SUCCESS || res == RES_ENDOFDATA);
  if (res != RES_SUCCESS && res != RES_ENDOFDATA) {
    delete pStructReq;
    return;
  }
  while (res != RES_ENDOFDATA) {
    // force update of windows list in the cache
    push4mong();
    UpdateMonInfo(0, OT_NODE, pStructReq, OT_NODE_OPENWIN);
    pop4mong();

    memset (pStructReq, 0, reqSize);
    res = GetNextMonInfo(pStructReq);
  }
  delete pStructReq;

  // refresh all opened branches of the node window,
  // especially "opened windows" branches
  UpdateNodeDisplay();
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CMDIFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}

void CMainFrame::OnSysColorChange() 
{
	CMDIFrameWnd::OnSysColorChange();

  // 24/06/97: reset icon cache for non-mfc dom trees
  IconCacheFree();
  IconCacheInit();

  // added : force send to non-permanent windows
  SendMessageToDescendants(WM_SYSCOLORCHANGE, 0, 0L, TRUE, FALSE);
}


void CMainFrame::UpdateIpmSeqNum(int val)
{
  if (val > m_IpmSeqNum)
    m_IpmSeqNum = val;
}

void CMainFrame::UpdateDbeventSeqNum(int val)
{
  if (val > m_DbeventSeqNum)
    m_DbeventSeqNum = val;
}

void CMainFrame::UpdateSqlactSeqNum(int val)
{
  if (val > m_SqlactSeqNum)
    m_SqlactSeqNum = val;
}


BOOL CMainFrame::OnQueryEndSession()
{
  // masqued out since we manage documents save differently
  //if (!CMDIFrameWnd::OnQueryEndSession())
  //  return FALSE;

  // test for trace: Beep(440, 1000);

  // specialized query end session code
  BOOL bRet = CleanupAndExit(m_hWnd);
  if (bRet == TRUE) {
  	CWinApp* pApp = AfxGetApp();
	  ASSERT_VALID(pApp);
    pApp->HideApplication();
		pApp->CloseAllDocuments(TRUE);
  }

  return bRet;    // true allows exit, false forbids
}

void CMainFrame::OnEndSession(BOOL bEnding) 
{
  // test for trace: Beep(622, 1000);

	CMDIFrameWnd::OnEndSession(bEnding);

  // test for trace: Beep(880, 1000);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
  // special management in main.c BEFORE base class handling
  MainOnActivateApp(bActive);

	CMDIFrameWnd::OnActivateApp(bActive, hTask);
}


void CMainFrame::OnDestroy() 
{
	CMDIFrameWnd::OnDestroy();
	
  // will clean the clipboard ---> bye bye free chunks!
  MainOnDestroy(m_hWnd);
}


extern "C" BOOL InitialLoadEnvironment(LPSTR lpszCmdLine);
void CMainFrame::OnInitialLoadEnvironment(CString csCmdLine)
{
#ifdef NOLOADSAVE
  return;
#endif

  // reset sequential numbers for mfc mdi docs captions
  m_IpmSeqNum     = 0;
  m_DbeventSeqNum = 0;
  m_SqlactSeqNum  = 0;
  
  ResetOpenEnvSuccessFlag();
  ForgetDelayedUpdates();
  BOOL bSuccess = InitialLoadEnvironment((LPSTR)(LPCTSTR)csCmdLine);

  AcceptDelayedUpdates();
  // If successful in opening environment:
  if (bSuccess) {   // ex -if (GetOpenEnvSuccessFlag()) {
    // 0) Update m_strTitle for future doc activation title preserve
    GetWindowText(m_strTitle);
  
    // 1)Update Windows handles in node bar
    UpdateOpenWindowsNodeDisplay();
  
    // 2) update node window special variables
    // for virtual nodes
    UpdateVirtualNodesVariables();
  }
  else 
    AfxMessageBox(VDBA_MfcResourceString (IDS_E_INIT_ENV), NULL, MB_OK | MB_ICONEXCLAMATION);//"Failure in Loading Ingres VisualDBA Initial Environment"
}


void CMainFrame::UpdateNodeUsersList(CString csNodeName)
{
  // Low level already updated (for 'users')
  // refresh all opened branches of the node window,
  // especially "users" branches
  UpdateNodeDisplay();
}


LONG CMainFrame::OnActionThreadAnimate(WPARAM wParam, LPARAM lParam)
{
	BOOL bActionComplete;
	LONG lresult = ManageBaseListAction(wParam,lParam,&bActionComplete);
	if (bActionComplete)
		return lresult;

	CaExecParamAction* pExecParam = (CaExecParamAction*)lParam;
	ASSERT (pExecParam);
	if (pExecParam)
		pExecParam->Action (wParam);

	return 0;
}

void CMainFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
  TRACE0("OnPaletteChanged...\n");

  CMDIFrameWnd::OnPaletteChanged(pFocusWnd);

  CMDIChildWnd* pActiveChild = MDIGetActive();
  if (pActiveChild)
    pActiveChild->SendMessageToDescendants(WM_PALETTECHANGED);
}

BOOL CMainFrame::OnQueryNewPalette() 
{
  TRACE0("OnQueryNewPalette...\n");

  CMDIChildWnd* pActiveChild = MDIGetActive();
  if (pActiveChild)
    pActiveChild->SendMessageToDescendants(WM_QUERYNEWPALETTE);

  return CMDIFrameWnd::OnQueryNewPalette();
}

void CMainFrame::OnPaletteIsChanging(CWnd* pRealizeWnd) 
{
  TRACE0("OnPaletteIsChanging...\n");

  CMDIFrameWnd::OnPaletteIsChanging(pRealizeWnd);
  
  // TODO: Add your message handler code here
  
}

void CMainFrame::OnButtonHistoryerror() 
{
	TRACE0 (_T("CMainFrame::OnButtonHistoryerror()...\n"));
	DisplayStatementsErrors(m_hWnd);
}

void CMainFrame::OnUpdateButtonSpacecalc(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (TRUE);
}


void CMainFrame::OnPreferenceSave() 
{
	CMenu* pMenu = GetMenu();
	UINT nState = pMenu->GetMenuState(IDM_PREFERENCE_SAVE, MF_BYCOMMAND);
	if (nState == MF_CHECKED)
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
		theApp.SavePropertyAsDefault(FALSE);
	}
	else
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
		theApp.SavePropertyAsDefault(TRUE);
	}
}

void CMainFrame::PermanentState(BOOL bLoad)
{
#if defined (_PERSISTANT_STATE_)
	CaPersistentStreamState file (_T("Vdba"), bLoad);
	if (!file.IsStateOK())
		return;
	if (bLoad)
	{
		BOOL bBool;
		CArchive ar(&file, CArchive::load);
		ar >> bBool; theApp.SavePropertyAsDefault(bBool);

		CMenu* pMenu = GetMenu();
		if (theApp.IsSavePropertyAsDefault())
			pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
		else
			pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
	}
	else
	{
		CArchive ar(&file, CArchive::store);
		ar << theApp.IsSavePropertyAsDefault();
	}
#endif
}

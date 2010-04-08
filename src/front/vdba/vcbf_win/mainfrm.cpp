/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : mainfrm.cpp, Implementation file
**
**  Project  : OpenIngres Configuration Manager
**
**  Author   : UK Sotheavut
**
**  Purpose  : Main Frame
**      See the CONCEPT.DOC for more detail
**
**  History  :
**	01-apr-99 (cucjo01)
**	    Added context sensitive help support for Pop-up menu's
**	    Changed dwContextId to iCtrlId
**	10-jun-1999 (mcgem01)
**	    EDBC has its own independent help file.
**	24-jan-2000 (hanch04)
**	    Updated for UNIX build using mainwin, added FILENAME_SEPARATOR.
**	20-Apr-2000 (uk$so01)
**	    SIR #101252
**	    When user start the VCBF, and if IVM is already running
**	    then request IVM the start VCBF
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	18-Sep-2001 (hanje04)
**	    Set AfxGetApp()->m_pMainWnd=this at the end of OnCreate so that
**	    window is titled corectly.
**	06-Nov-2001 (loera01) Bug 106295
**	    Path references to help files should be in lower case for Unix.
**	30-Jan-2004 (uk$so01)
**	    SIR #111701, Use Compiled HTML Help (.chm file)
**	02-feb-2004 (somsa01)
**	    In CMainFrame::OnHelpInfo(), use the non-class version of
**	    HtmlHelp().
**	 09-Feb-2004 (uk$so01)
**	    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, vcbf now uses vcbf.chm instead of vdba.chm
*/

#include "stdafx.h"
#include "vcbf.h"

#include "mainfrm.h"
#include "vcbfview.h"
#include "lvleft.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef MAINWIN
#define FILENAME_SEPARATOR _T('\\')
#define FILENAME_SEPARATOR_STR _T("\\")
#endif /* not MAINWIN */

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_HELPINFO()
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	//}}AFX_MSG_MAP
	// Global help commands
	//ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	//ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	//ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	//ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
	ON_MESSAGE(WMUSRMSG_ACTIVATE, OnMakeActive)
	ON_MESSAGE (WMUSRMSG_APP_SHUTDOWN, OnExternalRequestShutDown)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_pVcbfView = NULL;
	m_bEndSession = FALSE;
}

CMainFrame::~CMainFrame()
{
}

//#define USE_TOOLBAR_STATUS_BAR
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CMenu* pMenu = (CMenu*)GetMenu();
	SetMenu (NULL);
	pMenu->Detach();
#if defined (USE_TOOLBAR_STATUS_BAR)	
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

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
#endif
#ifdef MAINWIN
	AfxGetApp()->m_pMainWnd=this;
#endif
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass =_T("VCBF");
	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	if (cx < 400)
		cx = 400;
	if (cy < 300)
		cy = 300;
	CFrameWnd::OnSize(nType, cx, cy);
}


BOOL CMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID;

	if (pHelpInfo->iContextType == HELPINFO_MENUITEM)
		nHelpID = pHelpInfo->iCtrlId;
	else
		nHelpID = GetHelpID();

	return APP_HelpInfo(m_hWnd, nHelpID);
}


void CMainFrame::CloseApplication (BOOL bNormal)
{
	TRACE1 ("CMainFrame::CloseApplication(%d)...\n", bNormal);
  if (!bNormal)
  	MessageBox(_T("Fatal error: Terminating OpenIngres Configuration Manager"), NULL, MB_ICONSTOP | MB_OK );
	OnClose();
  if (!bNormal) {
  	VCBFllterminate();
	  exit(EXIT_FAILURE);
  }
}


void CMainFrame::OnClose() 
{
	// First, low level deactivation in CConfLeftDlg instance
	CVcbfApp* pApp = (CVcbfApp*)AfxGetApp();
	ASSERT (pApp);
	ASSERT (pApp->m_pConfListViewLeft);
	pApp->m_pConfListViewLeft->GetConfLeftDlg()->OnMainFrameClose();

	CFrameWnd::OnClose();
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
}

UINT CMainFrame::GetHelpID()
{
	ASSERT (m_pVcbfView);
	if (!m_pVcbfView)
		return 0;
	if (m_pVcbfView->m_pMainTabDlg)
		return m_pVcbfView->m_pMainTabDlg->GetHelpID();
	return 0;
}

BOOL CMainFrame::OnQueryEndSession() 
{
	if (!CFrameWnd::OnQueryEndSession())
		return FALSE;
	m_bEndSession = TRUE;
	return TRUE;
}

void CMainFrame::OnEndSession(BOOL bEnding) 
{
	if (m_bEndSession)
	{
		OnExternalRequestShutDown(0, 0);
	}
	CFrameWnd::OnEndSession(bEnding);
}


LONG CMainFrame::OnExternalRequestShutDown(WPARAM wParam, LPARAM lParam)
{
	OnClose();
	return TRUE;
}

LONG CMainFrame::OnMakeActive(WPARAM wParam, LPARAM lParam)
{
	ActivateFrame();
	SetForegroundWindow();
	return 0;
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFile = _T("vcbf.chm");
#if defined (EDBC)
	strHelpFile = _T("edbcvdba.chm");
#endif
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
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
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return FALSE;
}

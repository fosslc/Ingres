/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mainfrm.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 26-Jul-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management. Add menu Help\Help Topics that invokes the general help.
**
**/

#include "stdafx.h"
#include "ija.h"
#include "viewleft.h"
#include "viewrigh.h"
#include "mainfrm.h"
#include "drefresh.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame

IMPLEMENT_DYNCREATE(CfMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CfMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_DEFAULT_HELP, OnDefaultHelp)
	//}}AFX_MSG_MAP
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
	// TODO: add member initialization code here
	
}

CfMainFrame::~CfMainFrame()
{
}

int CfMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	UINT nStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP;
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, nStyle, CRect(1, 2, 1, 2)) ||
	    !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	//
	// Set the Title of the Main Toolbar:
	CString strToolbarTitle;
	if (!strToolbarTitle.LoadString (IDS_MAINTOOLBAR_TITLE))
		strToolbarTitle = _T("Ingres Journal Analyzer");
	m_wndToolBar.SetWindowText (strToolbarTitle);
	//
	// Set the Button Text:
	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();
	CString strTips;
	UINT nID = 0;
	int nButtonCount = tbctrl.GetButtonCount();
	for (int i=0; i<nButtonCount; i++)
	{
		if (m_wndToolBar.GetButtonStyle(i) & TBBS_SEPARATOR)
			continue;
		nID = m_wndToolBar.GetItemID(i);
		if (strTips.LoadString(nID))
		{
			int nFound = strTips.Find (_T("\n"));
			if (nFound != -1)
				strTips = strTips.Mid (nFound+1);
			m_wndToolBar.SetButtonText(i, strTips);
		}
	}
	CRect temp;
	m_wndToolBar.GetItemRect(0,&temp);
	m_wndToolBar.SetSizes( CSize(temp.Width(),temp.Height()), CSize(16,16));
	//
	// Set the Hot Image:
	CImageList image;
	image.Create(IDB_HOTMAINFRAME, 16, 0, RGB (128, 128, 128));
	tbctrl.SetHotImageList (&image);
	image.Detach();
	//
	// Status bar:
	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	UINT nExtraStyle = CBRS_FLYBY | CBRS_SIZE_DYNAMIC | CBRS_GRIPPER | CBRS_BORDER_3D;
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | nExtraStyle);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	ShowControlBar (&m_wndToolBar,   theApp.m_cmdLine.m_bNoToolbar?   FALSE: TRUE, TRUE);
	ShowControlBar (&m_wndStatusBar, theApp.m_cmdLine.m_bNoStatusbar? FALSE: TRUE, TRUE);

	return 0;
}

BOOL CfMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

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

BOOL CfMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1;
	CRuntimeClass* pView2 = RUNTIME_CLASS(CvViewRight);
	context1.m_pNewViewClass = pView2;
	context1.m_pCurrentDoc   = pContext->m_pCurrentDoc;
	if (!m_Splitterwnd.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("Failed to create Splitter\n");
		return FALSE;
	}
	UINT nSplitSize = theApp.m_cmdLine.m_bSplitLeft? 0: 160;
	if (!m_Splitterwnd.CreateView (0, 0, pContext->m_pNewViewClass, CSize (nSplitSize, 400), pContext)) 
	{
		TRACE0 ("Failed to create first pane\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 1, pView2, CSize (400, 400), &context1)) 
	{
		TRACE0 (" Failed to create second pane\n");
		return FALSE;
	}
	if (nSplitSize == 0)
		m_Splitterwnd.SetTracking(FALSE);

	m_Splitterwnd.SetActivePane( 0,0); 
	CWnd* pWnd = m_Splitterwnd.GetPane (0,0);
	pWnd->Invalidate();
	m_Splitterwnd.RecalcLayout();
	m_bAllViewCreated = TRUE;

	return m_bAllViewCreated;
}

CView* CfMainFrame::GetRightView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CView* pView =(CView*)m_Splitterwnd.GetPane (0, 1);
	ASSERT_VALID(pView);
	return pView;
}

CView* CfMainFrame::GetLeftView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CView* pView =(CView*)m_Splitterwnd.GetPane (0, 0);
	ASSERT_VALID(pView);
	return pView;
}


void CfMainFrame::OnRefresh() 
{
	CvViewLeft* pView = (CvViewLeft*)GetLeftView();
	if (pView && IsWindow (pView->m_hWnd))
	{
		CxDlgRefresh dlg;
		if (dlg.DoModal() != IDOK)
			return;
		switch (dlg.m_nRadioChecked)
		{
		case IDC_RADIO1:
			pView->RefreshData(REFRESH_CURRENT);
			break;
		case IDC_RADIO2:
			pView->RefreshData(REFRESH_CURRENT_SUB);
			break;
		case IDC_RADIO3:
			pView->RefreshData(REFRESH_CURRENT_ALL);
			break;
		default:
			break;
		}
	}
}


BOOL CfMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CvViewLeft* pViewLeft = (CvViewLeft*)GetLeftView();
	if (pViewLeft)
	{
		pViewLeft->ShowHelp();
	}

	return CFrameWnd::OnHelpInfo(pHelpInfo);
}

void CfMainFrame::OnDefaultHelp() 
{
	APP_HelpInfo (m_hWnd, 0);
}

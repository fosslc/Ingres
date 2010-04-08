/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mainfrm.cpp, Implementation File
**    Project  : Virtual Node Manager. 
**    Author   : UK Sotheavut, Detail implementation.
**    Purpose  : Main Frame (Frame, View, Doc design)
**
**
** History:
**
** 24-jan-2000 (uk$so01)
**    (Sir #100102)
**    Add the "Performance Monitor" command in toolbar and menu
** 06-Mar-2000 (uk$so01)
**    SIR #100738, Use the flat toolbar.
** 13-Oct-2003 (schph01)
**    SIR #109864, Add Help ID for ID_NODE_EDIT_VIEW and
**    ID_POPUP_NODE_EDIT_VIEW menus
** 11-May-2007 (karye01)
**    SIR #118282, added Help menu item for support url.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"
static UINT Convert2VdbaHelpID (int nID);

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
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpTopic)
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
#if defined (EDBC)
	CMenu* pMenu = (CMenu*)GetMenu();
	if (pMenu)
	{
		//
		// 1 is the actual position of Node Submenu of IDR_MAINFRAME
		CMenu* pNodeMenu = (CMenu*)pMenu->GetSubMenu(1);
		if (pNodeMenu)
			pNodeMenu->DeleteMenu (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND);
	}
#endif
	UINT nStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP;
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, nStyle, CRect(1, 2, 1, 2)) ||
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
	UINT nExtraStyle = CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC | CBRS_GRIPPER | CBRS_BORDER_3D;
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | nExtraStyle);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

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

BOOL CfMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	int nID = -1;
	UINT nHelpID = 0;
	if (pHelpInfo)
	{
		TCHAR tchszID [16];
		CString strMsg;
		nID = pHelpInfo->iCtrlId;
		switch (pHelpInfo->iContextType)
		{
		case HELPINFO_MENUITEM:
			lstrcpy (tchszID, _T("Menu    ID = "));
			nHelpID = Convert2VdbaHelpID (nID);
			break;
		case HELPINFO_WINDOW:
			lstrcpy (tchszID, _T("Window  ID = "));
			nHelpID = IDHELP_MAIN_APP;
			break;
		default:
			lstrcpy (tchszID, _T("Unknown ID = "));
			break;
		}
		strMsg.Format (_T("CfMainFrame::OnHelpInfo: %s %d\n"), tchszID, nID);
		TRACE0 (strMsg);	
	}

	return APP_HelpInfo(nHelpID);
}

void CfMainFrame::OnHelpTopic()
{
	APP_HelpInfo(0);
}


//
// nID is an ID of the command hander in this application.
// Return value: the related help id of VDBA if existed.
static UINT Convert2VdbaHelpID (int nID)
{
	UINT nHelpID = 0;
	switch (nID)
	{
	case ID_APP_ABOUT:             // menu About
		return 28006;
	case ID_HELP_SUPPORTONLINE:    // menu Support online
                return 28009;
	case ID_APP_EXIT:              // menu exit
		return 21110;
	case ID_VIEW_TOOLBAR:          // menu View/Toolbar
		return 23021;
	case ID_DEFAULT_HELP:          // Help Topic
		return 28005;
	case ID_VIEW_STATUS_BAR:       // menu View/Statusbar
		return ID_VIEW_STATUS_BAR;
	case ID_NODE_ADD:              // menu Node/Add
		return 29008;
	case ID_NODE_ALTER:            // menu Node/Alter
		return 29009;
	case ID_NODE_DROP:             // menu Node/Drop
		return 29010;
	case ID_NODE_REFRESH:          // menu Node/Force Refresh
		return 29011;
	case ID_NODE_SQLTEST:          // menu Node/SQL Test
		return 29003;
	case ID_NODE_ADD_INSTALLATION: // menu Node/Add Installation
		return 29016;
	case ID_NODE_TEST:             // menu Node/Test
		return 29015;
	case ID_NODE_DOM:              // menu Node/DOM
		return 29002;
	case ID_NODE_MONITOR:          // menu Node/Performance Monitor
		return 29004;
	case ID_NODE_EDIT_VIEW:  // Menu Node/Nodes defined on Selected node
	case ID_POPUP_NODE_EDIT_VIEW:
		return 30100;
	}
	return nHelpID;
}


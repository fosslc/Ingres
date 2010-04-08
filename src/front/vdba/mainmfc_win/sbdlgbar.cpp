/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sbdlgbar.cpp, Implementation file 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut  (uk$so01)
** 
**    Purpose  : Virtual Node Dialog Bar Resizable.
**               OnSize and OnPaint:
**               The parent window (MainFrame window) must call the member
**               function RecalcLayout() to resize correctly this control bar.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sbdlgbar.h"
#include "mainfrm.h"
#include "vnitem.h"

extern "C"
{
#include "main.h"
};


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define   XMINSPACE         45
#define   YMINSPACE         80

CWnd* CuResizableDlgBar::GetMDIClientWnd()
{
	TCHAR tchszClass [12];
	if (m_pWndMDIClient)
		return m_pWndMDIClient;

	CFrameWnd* pFrame  = (CFrameWnd*)AfxGetMainWnd();
	CWnd* ChildWPtr    = pFrame->GetWindow (GW_CHILD);
	while (ChildWPtr)
	{
		GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
		if (lstrcmpi (tchszClass, _T("MDIClient")) == 0)
		{
			m_pWndMDIClient = ChildWPtr;
			break;
		}
		ChildWPtr = pFrame->GetWindow (GW_HWNDNEXT);
	}
	ASSERT (m_pWndMDIClient != NULL);
	return m_pWndMDIClient;
}


CuResizableDlgBar::CuResizableDlgBar()
{
	UINT nIdIcon[NUMBEROFBUTTONS] = 
	{
		IDI_DOM,
		IDI_SQL,
		IDI_IPM,
		IDI_DBEVENT,
		IDI_SCRATCH,
		IDI_DISCONNECT,
		IDI_CLOSE,
		IDI_ACTIVATE,
		IDI_NODE_ADD,
		IDI_NODE_ALTER,
		IDI_NODE_DROP,
		IDI_REFRESH
	};

	m_Alignment = 0;
	m_Pa        = CPoint (0,0);
	m_Pb        = CPoint (0,0);
	m_bCapture  = FALSE;
	m_pOldPen   = NULL;
	m_pTreeGlobalData = NULL;
	m_pWndMDIClient   = NULL;
	m_pXorPen   = new CPen (PS_SOLID|PS_INSIDEFRAME, 4, RGB (0xC080, 0xC080, 0xC080));
	m_WindowVersion = CuSplitter::GetWindowVersion();
	m_SplitterLeft.SetAlignment  (CBRS_ALIGN_LEFT);
	m_SplitterRight.SetAlignment (CBRS_ALIGN_RIGHT);
	m_SplitterTop.SetAlignment   (CBRS_ALIGN_TOP);
	m_SplitterBottom.SetAlignment(CBRS_ALIGN_BOTTOM);

	HINSTANCE hInst = AfxGetInstanceHandle();
	for (int i=0; i<NUMBEROFBUTTONS; i++)
	{
		m_arrayIcon[i] = (HICON)::LoadImage(hInst, MAKEINTRESOURCE(nIdIcon[i]), IMAGE_ICON, 16, 16, LR_SHARED);
	}

	m_Button01Position.x = m_Button01Position.y = 0;
}

CuResizableDlgBar::~CuResizableDlgBar()
{
	delete m_pXorPen;
	if (m_pTreeGlobalData)
		delete m_pTreeGlobalData;
}


BOOL CuResizableDlgBar::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	cs.style |= WS_THICKFRAME;
	return CDialogBar::PreCreateWindow(cs);
}


BOOL CuResizableDlgBar::Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, nIDTemplate, nStyle, nID))
		return FALSE;
	m_bChangeDockedSize = bChange;
	m_sizeFloating      = m_sizeDocked = m_sizeDefault;
	OnInitDialog();
	m_sizeDocked.cx = 180;
	m_sizeDocked.cy = 120;
	return TRUE;
}

BOOL CuResizableDlgBar::Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange)
{
	if (!CDialogBar::Create (pParent, lpszTemplateName, nStyle, nID))
		return FALSE;
	m_bChangeDockedSize = bChange;
	m_sizeFloating      = m_sizeDocked = m_sizeDefault;
	OnInitDialog();
	m_sizeDocked.cx = 180;
	m_sizeDocked.cy = 120;
	return TRUE;
}

UINT CuResizableDlgBar::GetCurrentAlignment()
{
	int   x1, y1, x2, y2;
	int   X1, Y1, X2, Y2;
	CRect r, rMDIClient;
	CWnd* WndMDIClient = NULL;//GetMDIClientWnd();
	TCHAR tchszClass [12];
	BOOL  bLeft, bRight, bTop, bBottom;

	if (IsFloating())
		return 0;
	CFrameWnd* pFrame  = (CFrameWnd*)AfxGetMainWnd();
	CWnd* ChildWPtr    = pFrame->GetWindow (GW_CHILD);
	while (ChildWPtr)
	{
		GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
		if (lstrcmpi (tchszClass, _T("MDIClient")) == 0)
		{
			WndMDIClient = ChildWPtr;
			break;
		}
		ChildWPtr = pFrame->GetWindow (GW_HWNDNEXT);
	}
	ASSERT (ChildWPtr != NULL);
	WndMDIClient->GetWindowRect (rMDIClient);
	GetWindowRect (r);
	pFrame->ScreenToClient (rMDIClient);
	pFrame->ScreenToClient (r);
	x1 = r.left;
	x2 = r.right;
	y1 = r.top;
	y2 = r.bottom;
	X1 = rMDIClient.left;
	X2 = rMDIClient.right;
	Y1 = rMDIClient.top;
	Y2 = rMDIClient.bottom;

	bLeft  = (x1 < X1) && ((x2-8) < X1);
	bRight = (x2 > X2) && ((x1+8) > X2);
	bTop   = (y1 < Y1) && ((y2-8) < Y1);
	bBottom= (y2 > Y2) && ((y1+8) > Y2);

	CString str;
	str.Format (_T("r: (%d,%d,  %d %d)  rMDIClient: (%d,%d,  %d %d)"), x1, y1, x2, y2, X1, Y1, X2, Y2);

	if (bLeft && !bTop && !bBottom)
	{
//        TRACE1 ("Left  :...%s\n", str); 
		return CBRS_ALIGN_LEFT;
	}
	else
	if (bRight && !bTop && !bBottom)
	{
//        TRACE1 ("Right :...%s\n", str); 
		return CBRS_ALIGN_RIGHT;
	}
	else
	if (bTop && ! bLeft && !bRight)
	{
//        TRACE1 ("Top   :...%s\n", str); 
		return CBRS_ALIGN_TOP;
	}
	else 
	if (bBottom && ! bLeft && !bRight)
	{
//        TRACE1 ("Bottom:...%s\n", str); 
		return CBRS_ALIGN_BOTTOM;
	}
	else
	{
//        TRACE1 ("No Ali:...%s\n", str);
		return 0;
	}
}

UINT CuResizableDlgBar::GetCurrentAlignment(CRect r, CRect rMDIClient)
{
	int   x1, y1, x2, y2;
	int   X1, Y1, X2, Y2;
	BOOL  bLeft, bRight, bTop, bBottom;

	x1 = r.left;
	x2 = r.right;
	y1 = r.top;
	y2 = r.bottom;
	X1 = rMDIClient.left;
	X2 = rMDIClient.right;
	Y1 = rMDIClient.top;
	Y2 = rMDIClient.bottom;

	bLeft  = (x1 < X1) && ((x2-8) < X1);
	bRight = (x2 > X2) && ((x1+8) > X2);
	bTop   = (y1 < Y1) && ((y2-8) < Y1);
	bBottom= (y2 > Y2) && ((y1+8) > Y2);
	CString str;
	str.Format (_T("r: (%d,%d,  %d %d)  rMDIClient: (%d,%d,  %d %d)"), x1, y1, x2, y2, X1, Y1, X2, Y2);


	if (bLeft && !bTop && !bBottom)
	{
//        TRACE1 ("xLeft  :...%s\n", str); 
		return CBRS_ALIGN_LEFT;
	}
	else
	if (bRight && !bTop && !bBottom)
	{
//        TRACE1 ("xRight :...%s\n", str); 
		return CBRS_ALIGN_RIGHT;
	}
	else
	if (bTop && ! bLeft && !bRight)
	{
//        TRACE1 ("xTop   :...%s\n", str); 
		return CBRS_ALIGN_TOP;
	}
	else 
	if (bBottom && ! bLeft && !bRight)
	{
//        TRACE1 ("xBottom:...%s\n", str); 
		return CBRS_ALIGN_BOTTOM;
	}
	else
	{
//        TRACE1 ("xNo Ali:...%s\n", str);
		return 0;
	}
}


CSize CuResizableDlgBar::CalcDynamicLayout (int nLength, DWORD dwMode)
{
	CWnd* WndMDIClient = NULL; //GetMDIClientWnd();
	TCHAR tchszClass [12];
	CRect rFrame;
	CFrameWnd* pFrame  = (CFrameWnd*)AfxGetMainWnd();
	CWnd* ChildWPtr    = pFrame->GetWindow (GW_CHILD);
	m_SplitterRight.ShowWindow (SW_SHOW);
	while (ChildWPtr)
	{
		GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
		if (lstrcmpi (tchszClass, _T("MDIClient")) == 0)
		{
			WndMDIClient = ChildWPtr;
			break;
		}
		ChildWPtr = pFrame->GetWindow (GW_HWNDNEXT);
	}
	ASSERT (ChildWPtr != NULL);

	if (IsFloating())
	{
		m_SplitterTop.ShowWindow   (SW_HIDE);
		m_SplitterBottom.ShowWindow(SW_HIDE);
		m_SplitterRight.ShowWindow (SW_HIDE);
		m_SplitterLeft.ShowWindow  (SW_HIDE);
	}
	else 
	{
		/*
		CFrameWnd* pFrame  = (CFrameWnd*)AfxGetMainWnd();
		CWnd* ChildWPtr    = pFrame->GetWindow (GW_CHILD);
		m_SplitterRight.ShowWindow (SW_SHOW);
		while (ChildWPtr)
		{
			GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
			if (lstrcmpi (tchszClass, "MDIClient") == 0)
			{
				WndMDIClient = ChildWPtr;
				break;
			}
			ChildWPtr = pFrame->GetWindow (GW_HWNDNEXT);
		}
		ASSERT (ChildWPtr != NULL);
		*/

		CRect r, rMDIClient;
		WndMDIClient->GetWindowRect (rMDIClient);
		GetWindowRect (r);
		pFrame->ScreenToClient (rMDIClient);
		pFrame->ScreenToClient (r);

		m_Alignment = GetCurrentAlignment();
		switch (m_Alignment)
		{
		case CBRS_ALIGN_LEFT:
			m_SplitterLeft.ShowWindow  (SW_HIDE);
			m_SplitterTop.ShowWindow   (SW_HIDE);
			m_SplitterBottom.ShowWindow(SW_HIDE);
			m_SplitterRight.ShowWindow (SW_SHOW);
			break;
		case CBRS_ALIGN_RIGHT:
			m_SplitterTop.ShowWindow   (SW_HIDE);
			m_SplitterBottom.ShowWindow(SW_HIDE);
			m_SplitterRight.ShowWindow (SW_HIDE);
			m_SplitterLeft.ShowWindow  (SW_SHOW);
			break;
		case CBRS_ALIGN_TOP:
			m_SplitterTop.ShowWindow   (SW_HIDE);
			m_SplitterRight.ShowWindow (SW_HIDE);
			m_SplitterLeft.ShowWindow  (SW_HIDE);
			m_SplitterBottom.ShowWindow(SW_SHOW);
			break;
		case CBRS_ALIGN_BOTTOM:
			m_SplitterBottom.ShowWindow(SW_HIDE);
			m_SplitterRight.ShowWindow (SW_HIDE);
			m_SplitterLeft.ShowWindow  (SW_HIDE);
			m_SplitterTop.ShowWindow   (SW_SHOW);
			break;
		default:
			m_SplitterTop.ShowWindow   (SW_HIDE);
			m_SplitterBottom.ShowWindow(SW_HIDE);
			m_SplitterRight.ShowWindow (SW_HIDE);
			m_SplitterLeft.ShowWindow  (SW_HIDE);
			break;
		}
		WndMDIClient->GetClientRect (rFrame);
	}
	if ((dwMode & LM_VERTDOCK) || (dwMode & LM_HORZDOCK))
	{
		CRect r;
		/*
		if (WndMDIClient &&  (m_Alignment== CBRS_ALIGN_LEFT || m_Alignment == CBRS_ALIGN_RIGHT || m_Alignment== CBRS_ALIGN_TOP || m_Alignment== CBRS_ALIGN_BOTTOM))
			WndMDIClient->GetClientRect (r);
		else
			pMain->GetClientRect (r);
		*/

		WndMDIClient->GetClientRect (r);
		if (dwMode & LM_STRETCH)
		{
			return CSize ((dwMode & LM_HORZ)? r.Width(): m_sizeDocked.cx, (dwMode & LM_HORZ)? m_sizeDocked.cy: r.Height()+8);
		}
		else
		{
//            CRect r;
//            CFrameWnd* pMain  = (CFrameWnd*)AfxGetMainWnd();
//            if (WndMDIClient &&  (m_Alignment== CBRS_ALIGN_LEFT || m_Alignment == CBRS_ALIGN_RIGHT || m_Alignment== CBRS_ALIGN_TOP || m_Alignment== CBRS_ALIGN_BOTTOM))
//                WndMDIClient->GetClientRect (r);
//            else
//                pMain->GetClientRect (r);
			if (dwMode & LM_VERTDOCK)
				return CSize (m_sizeDocked.cx, r.Height()+8);
			else
				return CSize (r.Width()+8, m_sizeDocked.cy);
			return CSize (m_sizeDocked.cx, r.Height());
		}
	}
	if (dwMode & LM_MRUWIDTH)
	{
		return m_sizeFloating;
	}
	if (dwMode & LM_LENGTHY)
	{
		return CSize (m_sizeFloating.cx, (m_bChangeDockedSize)? m_sizeFloating.cy = m_sizeDocked.cy = nLength: m_sizeFloating.cy = nLength);
	}
	else
	{
		return CSize (m_sizeFloating.cx = nLength, m_sizeFloating.cy);
	}
}

BEGIN_MESSAGE_MAP(CuResizableDlgBar, CDialogBar)
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnItemexpandingTree1)
	ON_WM_DESTROY()
	ON_WM_MOVE()
END_MESSAGE_MAP()


void CuResizableDlgBar::OnMove (int x, int y)
{
	CDialogBar::OnMove (x, y);
	UpdateDisplay();
}

BOOL CuResizableDlgBar::OnInitDialog() 
{
	CRect r;
	CMainFrame* mainFramePtr;
	mainFramePtr = (CMainFrame*)AfxGetMainWnd();
	VERIFY(m_SplitterLeft.SubclassDlgItem  (IDC_DIVIDER1, this));
	VERIFY(m_SplitterRight.SubclassDlgItem (IDC_DIVIDER2, this));
	VERIFY(m_SplitterTop.SubclassDlgItem   (IDC_DIVIDER3, this));
	VERIFY(m_SplitterBottom.SubclassDlgItem(IDC_DIVIDER4, this));
	VERIFY(m_Tree.SubclassDlgItem (IDC_TREE1, this));
	VERIFY(m_Button01.SubclassDlgItem (IDM_VNODEBAR01, this));
	VERIFY(m_Button02.SubclassDlgItem (IDM_VNODEBAR02, this));
	VERIFY(m_Button03.SubclassDlgItem (IDM_VNODEBAR03, this));
	VERIFY(m_Button04.SubclassDlgItem (IDM_VNODEBAR04, this));
	VERIFY(m_ButtonSC.SubclassDlgItem (IDM_VNODEBARSC, this));
	VERIFY(m_Button05.SubclassDlgItem (IDM_VNODEBAR05, this));
	VERIFY(m_Button06.SubclassDlgItem (IDM_VNODEBAR06, this));
	VERIFY(m_Button07.SubclassDlgItem (IDM_VNODEBAR07, this));
	VERIFY(m_Button08.SubclassDlgItem (IDM_VNODEBAR08, this));
	VERIFY(m_Button09.SubclassDlgItem (IDM_VNODEBAR09, this));
	VERIFY(m_Button10.SubclassDlgItem (IDM_VNODEBAR10, this));
	VERIFY(m_Button11.SubclassDlgItem (IDM_VNODEBAR11, this));

	CuBitmapButton* btnArray [NUMBEROFBUTTONS] = 
	{
		&m_Button01,
		&m_Button02,
		&m_Button03,
		&m_Button04,
		&m_ButtonSC,
		&m_Button05,
		&m_Button06,
		&m_Button07,
		&m_Button08,
		&m_Button09,
		&m_Button10,
		&m_Button11
	};

	CSize buttonSize = GetToolBarButtonSize(this, IDR_VNODEMDI);
	UINT nFlags = SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE;
	for (int i=0; i<NUMBEROFBUTTONS; i++)
	{
		btnArray[i]->SetIcon(m_arrayIcon[i]);
		btnArray[i]->SetWindowPos(NULL, -1, -1, buttonSize.cx, buttonSize.cy, nFlags);
	}

	m_Button01.GetWindowRect (r);
	ScreenToClient (r);
	m_Button01Position.x = r.left;
	m_Button01Position.y = r.top;

	m_pTreeGlobalData = new CTreeGlobalData (&m_Tree, NULL, 0); // hnode must be 0, not default -1!

	CuTreeServerStatic* pItem = new CuTreeServerStatic (m_pTreeGlobalData);
	pItem->CreateTreeLine();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuResizableDlgBar::OnSize(UINT nType, int cx, int cy) 
{
	int i, y, x;
	int nDividerWidth = 6; 
	CRect rb1, rDlg;
	CuBitmapButton* tab [] = 
	{
		&m_Button01, 
		&m_Button02, 
		&m_Button03, 
		&m_Button04, 
		&m_ButtonSC, 
		&m_Button05, 
		&m_Button06, 
		&m_Button07, 
		&m_Button08, 
		&m_Button09, 
		&m_Button10,
		&m_Button11
	};

	CMainFrame* MainFramePtr = (CMainFrame*)AfxGetMainWnd();
	if (!IsWindow (m_Tree.m_hWnd))
		return;

	CRect rMDIClient;
	CWnd* WndMDIClient = NULL;
	TCHAR tchszClass [12];
	CWnd* ChildWPtr    = MainFramePtr->GetWindow (GW_CHILD);
	while (ChildWPtr)
	{
		GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
		if (lstrcmpi (tchszClass, _T("MDIClient")) == 0)
		{
			WndMDIClient = ChildWPtr;
			break;
		}
		ChildWPtr = MainFramePtr->GetWindow (GW_HWNDNEXT);
	}
	ASSERT (ChildWPtr != NULL);
	WndMDIClient->GetWindowRect (rMDIClient);
	MainFramePtr->ScreenToClient (rMDIClient);

	CRect rClient, r2, r3, r4;
	GetClientRect (r2);
	if (m_WindowVersion != VER_PLATFORM_WIN32_NT)
		nDividerWidth = 7;
	m_SplitterRight.MoveWindow (CRect (r2.Width()-nDividerWidth, 0, r2.Width(), r2.Height()));
	m_SplitterRight.GetClientRect (r4);
	m_SplitterLeft.MoveWindow  (CRect (0, 0, nDividerWidth, r2.Height()));
	m_SplitterTop.MoveWindow   (CRect (0, 0, r2.Width(), nDividerWidth));
	m_SplitterBottom.MoveWindow(CRect (0, r2.Height()-nDividerWidth, r2.Width(), r2.Height()));

	//
	// Size the NUMBEROFBUTTONS buttons and replace then at the appropriate place.
	m_Tree.ShowWindow (SW_HIDE);
	m_Button01.GetClientRect (rb1);
	x = m_Button01Position.x;
	y = m_Button01Position.y;
	GetClientRect (rDlg);
	m_Tree.GetWindowRect (r2);
	ScreenToClient (r2);
	for (i=1; i<NUMBEROFBUTTONS; i++)
	{
		if ((x + 2*rb1.Width()) > (rDlg.Width()-m_Button01Position.x))
		{
			x  = m_Button01Position.x;
			y += rb1.Height();
		}
		else
		{
			x += rb1.Width();
		}
		tab [i]->SetWindowPos (NULL, x, y, 0, 0, SWP_NOSIZE);
	}
	tab [NUMBEROFBUTTONS-1]->Invalidate();
	CRect r (m_Button01Position.x-2, y + rb1.Height() +6, cx-12, cy-10);
	m_Tree.MoveWindow (r);
	m_Tree.ShowWindow (SW_SHOW);
	CRect rx;
	GetWindowRect (rx);
	MainFramePtr->ScreenToClient (rx);
	CString str;
	str.Format (
		_T("CuResizableDlgBar::OneSize, r: (%d,%d,  %d %d), rMDIClient: (%d,%d,  %d %d): Floating= %d\n"), 
		rx.left, rx.top, rx.right, rx.bottom, 
		rMDIClient.left, rMDIClient.top, rMDIClient.right, rMDIClient.bottom,
		IsFloating());
//    TRACE0 (str);
	m_Alignment = GetCurrentAlignment (rx, rMDIClient);
	if (IsFloating())
		m_Alignment = 0;
	switch (m_Alignment)
	{
	case CBRS_ALIGN_LEFT:
		m_SplitterLeft.ShowWindow  (SW_HIDE);
		m_SplitterTop.ShowWindow   (SW_HIDE);
		m_SplitterBottom.ShowWindow(SW_HIDE);
		m_SplitterRight.ShowWindow (SW_SHOW);
		break;
	case CBRS_ALIGN_RIGHT:
		m_SplitterTop.ShowWindow   (SW_HIDE);
		m_SplitterBottom.ShowWindow(SW_HIDE);
		m_SplitterRight.ShowWindow (SW_HIDE);
		m_SplitterLeft.ShowWindow  (SW_SHOW);
		break;
	case CBRS_ALIGN_TOP:
		m_SplitterTop.ShowWindow   (SW_HIDE);
		m_SplitterRight.ShowWindow (SW_HIDE);
		m_SplitterLeft.ShowWindow  (SW_HIDE);
		m_SplitterBottom.ShowWindow(SW_SHOW);
		break;
	case CBRS_ALIGN_BOTTOM:
		m_SplitterBottom.ShowWindow(SW_HIDE);
		m_SplitterRight.ShowWindow (SW_HIDE);
		m_SplitterLeft.ShowWindow  (SW_HIDE);
		m_SplitterTop.ShowWindow   (SW_SHOW);
		break;
	default:
		m_SplitterTop.ShowWindow   (SW_HIDE);
		m_SplitterBottom.ShowWindow(SW_HIDE);
		m_SplitterRight.ShowWindow (SW_HIDE);
		m_SplitterLeft.ShowWindow  (SW_HIDE);
		break;
	}
	MainFramePtr->SetMessageText ("");
}

void CuResizableDlgBar::UpdateDisplay()
{
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
}


void CuResizableDlgBar::OnContextMenu(CWnd*, CPoint point)
{
	// get menu id according to current menuitem
	UINT menuId = m_pTreeGlobalData->GetContextMenuId();
	if (!menuId)
		return;

	// load menu
	CMenu menu;
	BOOL bSuccess = menu.LoadMenu (menuId);   // Note : DestroyMenu automatic in CMenu destructor
	ASSERT (bSuccess);
	if (!bSuccess)
		return;
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CRect r;
	CWnd* pWndPopupOwner = AfxGetMainWnd();
	m_Tree.GetWindowRect (r);
	if (!r.PtInRect(point))
		return;
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWndPopupOwner);
}

BOOL CuResizableDlgBar::PreTranslateMessage(MSG* pMsg)
{
	//
	// Shift+F10: show pop-up menu.
	if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) &&    // If we hit a key and
		(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||        // it's Shift+F10 OR
		(pMsg->message == WM_CONTEXTMENU))                                        // Natural keyboard key
	{
		CRect rect;
		m_Tree.GetClientRect(rect);
		ClientToScreen(rect);
		CPoint point = rect.TopLeft();
		point.Offset(5, 5);
		OnContextMenu(NULL, point);
		return TRUE;
	}
	return CDialogBar::PreTranslateMessage(pMsg);
}

void CuResizableDlgBar::OnVnodebarHide() 
{
	// TODO: Add your command handler code here
}

void CuResizableDlgBar::OnVnodebarDockedview() 
{
}

BOOL CuResizableDlgBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	HCURSOR hCursor;
	CRect r;
	POINT p;
	UINT  uCursor = (UINT)IDC_ARROW;

	switch (m_Alignment)
	{
	case CBRS_ALIGN_LEFT:
		m_SplitterRight.GetWindowRect (r);
		uCursor = AFX_IDC_HSPLITBAR;
		break;
	case CBRS_ALIGN_RIGHT:
		m_SplitterLeft.GetWindowRect (r);
		uCursor = AFX_IDC_HSPLITBAR;
		break;
	case CBRS_ALIGN_TOP:
		m_SplitterBottom.GetWindowRect (r);
		uCursor = AFX_IDC_VSPLITBAR;
		break;
	case CBRS_ALIGN_BOTTOM:
		m_SplitterTop.GetWindowRect (r);
		uCursor = AFX_IDC_VSPLITBAR;
		break;
	default:
		break;
	}
	ScreenToClient(r);
	GetCursorPos  (&p);
	ScreenToClient(&p);
	if (r.PtInRect(p) && !IsFloating())
	{
		hCursor = AfxGetApp()->LoadCursor(uCursor);
		SetCursor(hCursor);
	}
	else
	{
		hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		SetCursor(hCursor);
	}
	return TRUE;
}


void CuResizableDlgBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	int oldROP2;
	CRect r;
	CPoint p = point;
	CFrameWnd* pFrame  = (CFrameWnd*)AfxGetMainWnd();

	GetWindowRect (r);
	if (!m_bCapture || IsFloating())
	{
		CDialogBar::OnMouseMove(nFlags, point);
		return;
	}
	CClientDC dc(pFrame);
	m_pOldPen = dc.SelectObject (m_pXorPen);
	oldROP2   = dc.SetROP2 (R2_XORPEN);
	if (m_Alignment == CBRS_ALIGN_LEFT || m_Alignment == CBRS_ALIGN_RIGHT)
	{
		dc.MoveTo (m_Pa.x, m_Pa.y+2);
		dc.LineTo (m_Pb.x, m_Pb.y-6);
	}
	else
	{
		dc.MoveTo (m_Pa.x+2, m_Pa.y);
		dc.LineTo (m_Pb.x-6, m_Pb.y);
	}

	ClientToScreen (&p);
	pFrame->ScreenToClient (&p);
	pFrame->ScreenToClient (r);
	if (m_Alignment == CBRS_ALIGN_LEFT || m_Alignment == CBRS_ALIGN_RIGHT)
	{
		dc.MoveTo  (p.x, r.top+2);
		dc.LineTo (p.x, r.bottom-6);
		m_Pa.x = p.x;
		m_Pa.y = r.top;
		m_Pb.x = p.x;
		m_Pb.y = r.bottom;
	}
	else
	{
		dc.MoveTo (r.left +2, p.y);
		dc.LineTo (r.right-6, p.y);
		m_Pa.x = r.left;
		m_Pa.y = p.y;
		m_Pb.x = r.right;
		m_Pb.y = p.y;
	}
	dc.SelectObject (m_pOldPen);
	dc.SetROP2 (oldROP2);
	CDialogBar::OnMouseMove(nFlags, point);
}


void CuResizableDlgBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int        oldROP2;
	BOOL       bVertical = TRUE;
	CRect      r;
	CPoint     p      = point;
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	pFrame->SetMessageText (IDS_DOCKINGINFO);
	switch (m_Alignment)
	{
	case CBRS_ALIGN_LEFT:
		m_SplitterRight.GetWindowRect (r);
		bVertical = TRUE;
		break;
	case CBRS_ALIGN_RIGHT:
		m_SplitterLeft.GetWindowRect (r);
		bVertical = TRUE;
		break;
	case CBRS_ALIGN_TOP:
		m_SplitterBottom.GetWindowRect (r);
		bVertical = FALSE;
		break;
	case CBRS_ALIGN_BOTTOM:
		m_SplitterTop.GetWindowRect (r);
		bVertical = FALSE;
		break;
	default:
		break;
	}
	ScreenToClient(r);
	if (r.PtInRect(point) && !IsFloating())
	{
		CClientDC dc(pFrame);
		GetWindowRect (r);
		SetCapture();
		m_bCapture = TRUE;
		m_pOldPen  = dc.SelectObject (m_pXorPen);
		oldROP2    = dc.SetROP2 (R2_XORPEN);
		ClientToScreen (&p);
		pFrame->ScreenToClient (&p);
		pFrame->ScreenToClient (r);
		if (bVertical)
		{
			dc.MoveTo (p.x, r.top+2);
			dc.LineTo (p.x, r.bottom-6);
			m_Pa.x = p.x;
			m_Pa.y = r.top;
			m_Pb.x = p.x;
			m_Pb.y = r.bottom;
		}
		else
		{
			dc.MoveTo (r.left +2, p.y);
			dc.LineTo (r.right-6, p.y);
			m_Pa.x = r.left;
			m_Pa.y = p.y;
			m_Pb.x = r.right;
			m_Pb.y = p.y;
		}
		dc.SelectObject (m_pOldPen);
		dc.SetROP2 (oldROP2);
	}
	CDialogBar::OnLButtonDown(nFlags, point);
}

void CuResizableDlgBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
	int oldROP2;
	CRect r;
	CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
	pFrame->SetMessageText ("");

	GetClientRect (r);
	if (!m_bCapture || IsFloating())
	{
		CDialogBar::OnLButtonUp(nFlags, point);
		pFrame->RecalcLayout();
		return;
	}
	TCHAR tchszClass [12];
	CRect rFrame;
	CWnd* WndMDIClient = NULL;
	CWnd* ChildWPtr    = pFrame->GetWindow (GW_CHILD);
	while (ChildWPtr)
	{
		GetClassName (ChildWPtr->m_hWnd, tchszClass, sizeof (tchszClass));
		if (lstrcmpi (tchszClass, _T("MDIClient")) == 0)
		{
			WndMDIClient = ChildWPtr;
			break;
		}
		ChildWPtr = pFrame->GetWindow (GW_HWNDNEXT);
	}
	ASSERT (ChildWPtr != NULL);
	CClientDC dc(pFrame);
	WndMDIClient->GetWindowRect (rFrame);
	ScreenToClient (rFrame);
	m_pOldPen = dc.SelectObject (m_pXorPen);
	oldROP2   = dc.SetROP2 (R2_XORPEN);
	ReleaseCapture();
	m_bCapture = FALSE;
	if (m_Alignment == CBRS_ALIGN_LEFT || m_Alignment == CBRS_ALIGN_RIGHT)
	{
		dc.MoveTo (m_Pa.x, m_Pa.y+2);
		dc.LineTo (m_Pb.x, m_Pb.y-6);
	}
	else
	{
		dc.MoveTo (m_Pa.x+2, m_Pa.y);
		dc.LineTo (m_Pb.x-6, m_Pb.y);
	}
	dc.SetROP2 (oldROP2);
	dc.SelectObject (m_pOldPen);
	//
	// Calculate the new size (Width) of the Dialog Bar.
	switch (m_Alignment)
	{
	case CBRS_ALIGN_LEFT:
		if (point.x < XMINSPACE)
		{
			m_sizeDocked.cx   = XMINSPACE;
			m_sizeFloating.cx = XMINSPACE;
		}
		else
		if (point.x < (rFrame.right -XMINSPACE))
		{
			m_sizeDocked.cx   = point.x;
			m_sizeFloating.cx = point.x;
		}
		else
		{
			m_sizeDocked.cx   = rFrame.right - XMINSPACE;
			m_sizeFloating.cx = rFrame.right - XMINSPACE;
		}
		break;
	case CBRS_ALIGN_RIGHT:
		if (point.x > 0)
		{
			if (point.x > (r.Width() - XMINSPACE))
			{
				m_sizeDocked.cx   = XMINSPACE;
				m_sizeFloating.cx = XMINSPACE;
			}
			else
			{
				m_sizeDocked.cx   = r.Width() - point.x;
				m_sizeFloating.cx = r.Width() - point.x;
			}
		}
		else
		{
			if (point.x > (rFrame.left + XMINSPACE))
			{
				m_sizeDocked.cx   = r.Width() - point.x;
				m_sizeFloating.cx = r.Width() - point.x;
			}
			else
			{
				m_sizeDocked.cx   = r.Width() + rFrame.Width() - XMINSPACE;
				m_sizeFloating.cx = r.Width() + rFrame.Width() - XMINSPACE;
			}
		}
		break;
	case CBRS_ALIGN_TOP:
		if (point.y < YMINSPACE)
		{
			m_sizeDocked.cy   = YMINSPACE;
			m_sizeFloating.cy = YMINSPACE;
		}
		else
		if (point.y < (rFrame.bottom -YMINSPACE))
		{
			m_sizeDocked.cy   = point.y;
			m_sizeFloating.cy = point.y;
		}
		else
		{
			m_sizeDocked.cy   = rFrame.bottom - YMINSPACE;
			m_sizeFloating.cy = rFrame.bottom - YMINSPACE;
		}
		break;
	case CBRS_ALIGN_BOTTOM:
		if (point.y > 0)
		{
			if (point.y >= (r.Height() - YMINSPACE))
			{
				m_sizeDocked.cy   = YMINSPACE;
				m_sizeFloating.cy = YMINSPACE;
			}
			else
			{
				m_sizeDocked.cy   = r.Height() - point.y;
				m_sizeFloating.cy = r.Height() - point.y;
			}
		}
		else
		{
			if (point.y > (rFrame.top + YMINSPACE))
			{
				m_sizeDocked.cy   = r.Height() - point.y;
				m_sizeFloating.cy = r.Height() - point.y;
			}
			else
			{
				m_sizeDocked.cy   = r.Height() + rFrame.Height() - YMINSPACE;
				m_sizeFloating.cy = r.Height() + rFrame.Height() - YMINSPACE;
			}
		}
		break;
	default:
		break;
	}
	CDialogBar::OnLButtonUp(nFlags, point);
	pFrame->RecalcLayout();
	pFrame->SetMessageText (_T(""));
}

void CuResizableDlgBar::OnItemexpandingTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;     // default to allow expanding
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	if (pNMTreeView->action == TVE_EXPAND) {
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		CTreeItem *pItem;
		pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
		if (pItem)
			if (!pItem->IsAlreadyExpanded()) {
				if (pItem->CreateSubBranches(hItem))
					 pItem->SetAlreadyExpanded(TRUE);
				else
					*pResult = 1;     // prevent expanding
			}
	}
	bSaveRecommended = TRUE;
}

void CuResizableDlgBar::OnDestroy()
{
	m_pTreeGlobalData->FreeAllTreeItemsData();
	for (int i=0; i<NUMBEROFBUTTONS; i++)
	{
		DestroyIcon(m_arrayIcon[i]);
	}

	CDialogBar::OnDestroy();
}


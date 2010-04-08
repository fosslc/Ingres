/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**  Source   : fminifrm.cpp : implementation file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : This file contains the mini-frame that controls modeless 
**             property sheet CuPropertySheet
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "psheet.h"
#include "fminifrm.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CfMiniFrame, CMiniFrameWnd)

CfMiniFrame::CfMiniFrame()
{
	m_pModelessPropSheet = NULL;
	m_bDeleteOnDestroy = FALSE;
	m_lpData = NULL;
	m_nActive = 0;
	m_strEmpty = _T("No properties for this item.");
	m_strDefaultTextPageCaption = _T("");
}

CfMiniFrame::CfMiniFrame(CTypedPtrList < CObList, CPropertyPage* >& listPages, BOOL bDeleteOnDestroy, int nActive)
{
	m_pModelessPropSheet = NULL;
	m_bDeleteOnDestroy = bDeleteOnDestroy;
	m_lpData = NULL;
	m_nActive = nActive;
	m_strEmpty = _T("No properties for this item.");
	m_strDefaultTextPageCaption = _T("");
	POSITION pos = listPages.GetHeadPosition();
	while (pos != NULL)
	{
		CPropertyPage* pPage = listPages.GetNext(pos);
		m_listUserPages.AddTail(pPage);
	}
}


CfMiniFrame::~CfMiniFrame()
{
	while (!m_listUserPages.IsEmpty())
	{
		CPropertyPage* pPage = m_listUserPages.RemoveHead();
		if (m_bDeleteOnDestroy)
			delete pPage;
	}
}


BEGIN_MESSAGE_MAP(CfMiniFrame, CMiniFrameWnd)
	//{{AFX_MSG_MAP(CfMiniFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CfMiniFrame message handlers

int CfMiniFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMiniFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_listUserPages.GetCount() > 0)
		m_pModelessPropSheet = new CuPropertySheet(this, m_listUserPages);
	else
		m_pModelessPropSheet = new CuPropertySheet(this);

	if (!m_pModelessPropSheet->Create(this, WS_CHILD | WS_VISIBLE, 0))
	{
		//
		// Modeless property page: if Create failed, the PostNcDestroy() has been invoked,
		// so do not delete m_pModelessPropSheet otherwize GPF
		m_pModelessPropSheet = NULL;
		return -1;
	}
	//
	// Resize the mini frame so that it fits around the child property  sheet.
	CRect rectClient, rectWindow;
	m_pModelessPropSheet->GetWindowRect(rectClient);
	rectWindow = rectClient;
	//
	// CMiniFrameWnd::CalcWindowRect adds the extra width and height
	// needed from the mini frame.
	UINT nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
	CalcWindowRect(rectWindow);
	SetWindowPos(NULL, rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(), nFlags);
	m_pModelessPropSheet->SetWindowPos(NULL, 0, 0, rectClient.Width(), rectClient.Height(), nFlags);

	CTabCtrl* pParent = (CTabCtrl*)m_pModelessPropSheet->GetTabControl();
	CRect r;
	pParent->GetClientRect(r);
	pParent->AdjustRect(FALSE, r);
	m_pModelessPropSheet->m_cMsg.Create(m_strEmpty, WS_CHILD|SS_CENTER|SS_CENTERIMAGE, r, pParent);

	return 0;
}

void CfMiniFrame::OnDestroy() 
{
	CMiniFrameWnd::OnDestroy();
}


void CfMiniFrame::OnClose() 
{
	// Instead of closing the modeless property sheet, just hide it.
	ShowWindow(SW_HIDE);
}

void CfMiniFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMiniFrameWnd::OnSetFocus(pOldWnd);
}

void CfMiniFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CMiniFrameWnd::OnActivate(nState, pWndOther, bMinimized);

	// Forward any WM_ACTIVATEs to the property sheet...
	// Like the dialog manager itself, it needs them to save/restore the focus.
	ASSERT_VALID(m_pModelessPropSheet);

	// Use GetCurrentMessage to get unmodified message data.
	const MSG* pMsg = GetCurrentMessage();
	ASSERT(pMsg->message == WM_ACTIVATE);
	m_pModelessPropSheet->SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
	if (WA_INACTIVE != nState)
	{
		UINT nFlags = SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED;
		SetWindowPos(NULL, 0,0,0,0, nFlags);
	}
}

void CfMiniFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMiniFrameWnd::OnSize(nType, cx, cy);
	if (!m_pModelessPropSheet)
		return;
	if (!IsWindow (m_pModelessPropSheet->m_hWnd))
		return;
	CRect r;
	GetClientRect (r);

	r.top    += 4;
	r.left   += 6;
	r.right  -= 6;
	r.bottom -= 4;
	m_pModelessPropSheet->MoveWindow (r);
	HandleNoPage();
}

BOOL CfMiniFrame::OnEraseBkgnd(CDC* pDC) 
{
	//
	// Set brush to desired background color
	CBrush backBrush(GetSysColor (COLOR_MENU) /*RGB(192, 192, 192)*/);
	//
	// Save the old brush
	CBrush* pOldBrush = pDC->SelectObject(&backBrush);
	//
	// Get the current clipping boundary
	CRect rect;
	pDC->GetClipBox(&rect);
	//
	// Erase the area needed
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),  PATCOPY);

	pDC->SelectObject(pOldBrush); // Select the old brush back
	return TRUE;                  // message handled
}
	BOOL m_bDeleteOnDestroy;
	CTypedPtrList < CObList, CPropertyPage* > m_listUserPages;
	LPARAM m_lpData;

void CfMiniFrame::NewPages (CTypedPtrList < CObList, CPropertyPage* >& listPages, BOOL bDeleteOnDestroy, int nActive)
{
	m_nActive = nActive;
	int nCount = m_pModelessPropSheet->GetPageCount();
	if (nCount == 0)
	{
		//
		// Work around: when there is no page, add new pages to the property sheet,
		// the page will not be displayed (even if we call SetCurSel(0) of member of
		// tab control of the property sheet).
		m_pModelessPropSheet->AddPage(&(m_pModelessPropSheet->m_Page1));
		nCount++;
	}

	POSITION pos = listPages.GetHeadPosition();
	while (pos != NULL)
	{
		CPropertyPage* pPage = listPages.GetNext(pos);
		m_pModelessPropSheet->AddPage(pPage);
	}
	for (int i=0; i< nCount; i++)
	{
		m_pModelessPropSheet->RemovePage(0);
	}
	while (!m_listUserPages.IsEmpty())
	{
		CPropertyPage* pPage = m_listUserPages.RemoveHead();
		if (m_bDeleteOnDestroy)
			delete pPage;
	}
	pos = listPages.GetHeadPosition();
	while (pos != NULL)
	{
		CPropertyPage* pPage = listPages.GetNext(pos);
		m_listUserPages.AddTail(pPage);
	}
	m_bDeleteOnDestroy = bDeleteOnDestroy;
	CTabCtrl* pTab = m_pModelessPropSheet->GetTabControl();
	pTab->SetCurSel(m_nActive);
	HandleNoPage();
}

void CfMiniFrame::RemoveAllPages()
{
	int nCount = m_pModelessPropSheet->GetPageCount();
	for (int i=0; i< nCount; i++)
	{
		m_pModelessPropSheet->RemovePage(0);
	}

	while (!m_listUserPages.IsEmpty())
	{
		CPropertyPage* pPage = m_listUserPages.RemoveHead();
		if (m_bDeleteOnDestroy)
			delete pPage;
	}
	HandleNoPage();
}

void CfMiniFrame::HandleData(LPARAM lpData)
{
	m_lpData = lpData;
	HandleDefaultPageCaption();
	m_pModelessPropSheet->SendMessage (WMUSRMSG_UPDATEDATA, 0, m_lpData);
}


void CfMiniFrame::SetText (LPCTSTR lpszText, LPCTSTR lpszTabCaption)
{
	if (!m_pModelessPropSheet)
		return;
	if (!IsWindow (m_pModelessPropSheet->m_hWnd))
		return;

	CPropertyPage* pPage = m_pModelessPropSheet->GetActivePage();
	if (!pPage)
		return;
	if (!IsWindow (pPage->m_hWnd))
		return;

	if(pPage->IsKindOf( RUNTIME_CLASS(CuPPageEdit)))
	{
		if (lpszTabCaption)
		{
			TCITEM tcItem;
			tcItem.mask = TCIF_TEXT;
			tcItem.pszText = (LPTSTR)lpszTabCaption;
			CTabCtrl* pTab = m_pModelessPropSheet->GetTabControl();
			if (pTab && IsWindow (pTab->m_hWnd))
				pTab->SetItem(0, &tcItem);
		}

		CuPPageEdit* pDlgPage = (CuPPageEdit*)pPage;
		pDlgPage->SetText (lpszText);
	}
	else
	{
		ASSERT(FALSE); 
	}
}

void CfMiniFrame::HandleNoPage()
{
	if (IsWindow(m_pModelessPropSheet->m_cMsg.m_hWnd))
	{
		if (m_pModelessPropSheet->GetPageCount() == 0)
		{
			CTabCtrl* pParent = (CTabCtrl*)m_pModelessPropSheet->GetTabControl();
			CRect r;
			pParent->GetClientRect(r);
			pParent->AdjustRect(FALSE, r);
			m_pModelessPropSheet->m_cMsg.SetWindowText(m_strEmpty);
			m_pModelessPropSheet->m_cMsg.MoveWindow(r);
			m_pModelessPropSheet->m_cMsg.ShowWindow(SW_SHOW);
		}
		else
		{
			m_pModelessPropSheet->m_cMsg.ShowWindow(SW_HIDE);
		}
	}
}

void CfMiniFrame::HandleDefaultPageCaption()
{
	if (!m_pModelessPropSheet)
		return;
	if (!IsWindow (m_pModelessPropSheet->m_hWnd))
		return;
	CPropertyPage* pPage = m_pModelessPropSheet->GetActivePage();
	if (!pPage)
		return;
	if (!IsWindow (pPage->m_hWnd))
		return;
	if(pPage->IsKindOf( RUNTIME_CLASS(CuPPageEdit)))
	{
		int nIndx = m_pModelessPropSheet->GetActiveIndex();

		TCITEM tcItem;
		tcItem.mask = TCIF_TEXT;
		tcItem.pszText = (LPTSTR)(LPCTSTR)m_strDefaultTextPageCaption;
		CTabCtrl* pTab = m_pModelessPropSheet->GetTabControl();
		if (pTab && IsWindow (pTab->m_hWnd))
			pTab->SetItem(nIndx, &tcItem);
	}
}



/*
void CfMiniFrame::SetText (LPCTSTR lpszText, LPCTSTR lpszTabCaption)
{
	if (!m_pModelessPropSheet)
		return;
	if (!IsWindow (m_pModelessPropSheet->m_hWnd))
		return;

	CPropertyPage* pPage = m_pModelessPropSheet->GetActivePage();
	if (!pPage)
		return;
	if (!IsWindow (pPage->m_hWnd))
		return;

	if(pPage->IsKindOf( RUNTIME_CLASS(CuPPage1DetailText)))
	{
		if (lpszTabCaption)
		{
			TC_ITEM tcItem;
			tcItem.mask = TCIF_TEXT;
			tcItem.pszText = (LPTSTR)lpszTabCaption;
			CTabCtrl* pTab = m_pModelessPropSheet->GetTabControl();
			if (pTab && IsWindow (pTab->m_hWnd))
				pTab->SetItem(0, &tcItem);
		}

		CuPPage1DetailText* pDlgPage = (CuPPage1DetailText*)pPage;
		pDlgPage->SetText (lpszText);
	}

}
*/

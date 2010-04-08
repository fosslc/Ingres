/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lsctlhug.cpp : implementation of the CuListCtrlHuge class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The special list control that enable scrolling with 
**               the huge amount of item..
**
** History:
**
** 12-Feb-2001 (uk$so01)
**    Created
** 18-Sep-2001 (somsa01)
**    Created missing definition of method for OnKeydownListCtrl().
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/


/*
****  HOW TO USE THIS CONTROL
**
** 1) You must Construct and create the control
**    Ex: CuListCtrlHuge m_wndMyList;
**        GetClientRect (r);
**        m_wndMyList.Create (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE, r, this, 1000);
**
** 2) You must initialize the header and set the callback function for displaying item.
**    Ex:
**        LSCTRLHEADERPARAMS lsp[5] =
**        {
**            {_T("col0"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col1"),     80,  LVCFMT_LEFT,  FALSE},
**            {_T("col2"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col3"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col4"),    150,  LVCFMT_LEFT,  FALSE}
**        };
**        m_wndMyList.InitializeHeader(lsp, 5);
**        m_wndMyList.SetDisplayFunction(myDisplayItem);
**        where myDisplayItem is defined as:
**        void CALLBACK myDisplayItem(void* pItemData, CuListCtrl* pListCtrl, int nItem);
**
** 3) Add/Remove item by using the class CPtrArray. After that you must call
**    the member ResizeToFit() to manage the scrollbar.
**    Ex:
**        CPtrArray& arrayData = m_wndMyList.GetArrayData();
**        arrayData.SetSize (100000); // 100000 items
**        for (int i = 0; i<100000; i++)
**        {
**            CMyObject* myObject= Alocate my object;
**            arrayData.SetAt(i, (void*)myObject);
**        }
**        m_wndMyList.ResizeToFit();
**
** 4) The caller is responsible to destroy the allocated data:
**    Ex:
**        CPtrArray& arrayData = m_wndMyList.GetArrayData();
**        for (int i = 0; i<100000; i++)
**        {
**            CMyObject* myObject = (CMyObject*) arrayData.GetAt(i);
**            delete myObject;
**        }
**        arrayData.RemoveAll();
**        m_wndMyList.ResizeToFit();
**
*/



#include "stdafx.h"
#include "lsctlhug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_VSCROLL_HUGE  2000
#define IDC_LISTCTRL_HUGE 2001


// CLASS CuListCtrlHugeChildList
// ************************************************************************************************
BEGIN_MESSAGE_MAP(CuListCtrlHugeChildList, CuListCtrl)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CuListCtrlHugeChildList::OnPaint()
{
	CuListCtrlHuge* pParent = (CuListCtrlHuge*)GetParent();
	ASSERT (pParent);
	if (!pParent)
	{
		CuListCtrl::OnPaint();
		return;
	}
	
	//
	// Ensure that the total items count are always = items count per page.
	int nIndex = pParent->GetCurPos();
	CPtrArray* pArrayData = pParent->GetArrayData();
	if (pArrayData->GetSize() == 0)
	{
		DeleteAllItems();
		CuListCtrl::OnPaint();
		return;
	}

	PfnListCtrlDisplayItem pfnDisplayItem = pParent->GetPfnDisplayItem();
	ASSERT (pfnDisplayItem);
	if (!pfnDisplayItem)
		return;
	int nCountPerPage = GetCountPerPage();
	//
	// The control becomes smaller:
	
	int nCount = GetItemCount();
	while  (nCount > nCountPerPage)
	{
		DeleteItem (nCount -1);
		nCount = GetItemCount();
	}

	//
	// The control becomes larger:
	if (nCount < nCountPerPage)
	{
		nCount = GetItemCount();
		nIndex = nIndex + nCount;
		while  (nIndex < pArrayData->GetSize() && nCount < GetCountPerPage())
		{
			void* pItemData = pArrayData->GetAt(nIndex);
			pfnDisplayItem(pItemData, this, GetItemCount(), FALSE, pParent->GetUserInfo());

			nCount = GetItemCount();
			nIndex+= 1;
		}
	}
	CuListCtrl::OnPaint();
	pParent->SetCountPerPage(GetCountPerPage());
	CScrollBar& vcroll = pParent->GetVirtScrollBar();
	vcroll.SetScrollRange (0, pArrayData->GetSize() - GetCountPerPage());
}

// CLASS CuListCtrlHuge
// ************************************************************************************************
IMPLEMENT_DYNCREATE(CuListCtrlHuge, CWnd)
CuListCtrlHuge::CuListCtrlHuge()
{
	m_pData = &m_arrayPtr;
	m_pfnDisplayItem = NULL;
	m_nItemCountPerPage = 0;
	m_nCurPos = 0;
	m_bTrackPos = FALSE;
	m_scrollInfo.cbSize = sizeof (m_scrollInfo);
	m_pointInfo = CPoint(0,0);
	m_bDrawInfo = FALSE;
	m_pUserInfo = NULL;

	m_pListCtrl = (CuListCtrl*)new CuListCtrlHugeChildList();
	m_nListCtrlStyle = WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS;
}

CuListCtrlHuge::~CuListCtrlHuge()
{
	if (m_pListCtrl)
		delete m_pListCtrl;
}


BEGIN_MESSAGE_MAP(CuListCtrlHuge, CWnd)
	//{{AFX_MSG_MAP(CuListCtrlHuge)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_VSCROLL()
	ON_NOTIFY(LVN_KEYDOWN, IDC_LISTCTRL_HUGE, On_KeydownListCtrl)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTCTRL_HUGE, On_ItemchangedListCtrl)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LISTCTRL_HUGE, On_ColumnclickListCtrl)
END_MESSAGE_MAP()


int CuListCtrlHuge::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect r;
	GetClientRect (r);
	r.left = r.right - GetSystemMetrics (SM_CXVSCROLL);
	m_vcroll.Create (WS_CHILD|SBS_VERT|SBS_RIGHTALIGN, r, this, IDC_VSCROLL_HUGE);

	r.left  = 0;
	((CuListCtrlHugeChildList*)m_pListCtrl)->Create (m_nListCtrlStyle, r, this, IDC_LISTCTRL_HUGE);
	return 0;
}

void CuListCtrlHuge::InitializeHeader(LSCTRLHEADERPARAMS* pArrayHeader, int nSize)
{
	if (!IsWindow(m_pListCtrl->m_hWnd))
		return;
	m_pListCtrl->DeleteAllItems();

	LVCOLUMN lvc; 
	memset (&lvc, 0, sizeof (lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	while (m_pListCtrl->GetColumn(0, &lvc))
		m_pListCtrl->DeleteColumn(0);

	//
	// Construct new columns headers:
	lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	for (int i=0; i<nSize; i++)
	{
		lvc.fmt      = pArrayHeader[i].m_fmt;
		lvc.pszText  = (LPTSTR)pArrayHeader[i].m_tchszHeader;
		lvc.iSubItem = i;
		lvc.cx       = pArrayHeader[i].m_cxWidth;
		m_pListCtrl->InsertColumn (i, &lvc, pArrayHeader[i].m_bUseCheckMark);
	}
	m_nItemCountPerPage = m_pListCtrl->GetCountPerPage();
}


void CuListCtrlHuge::ResizeToFit()
{
	ASSERT (IsWindow(m_pListCtrl->m_hWnd));
	if (!IsWindow(m_pListCtrl->m_hWnd))
		return;
	int nSize = m_pData->GetSize();
	int nCount = 0;

	CRect r;
	GetClientRect (r);
	int cxScrollBar = GetSystemMetrics (SM_CXVSCROLL);
	//
	// We must resize the list control first to see how many item per page
	// it can contain:
	m_pListCtrl->MoveWindow (r);
	m_nItemCountPerPage = m_pListCtrl->GetCountPerPage();

	//
	// Show the V-scrollbar ?
	if (nSize > m_nItemCountPerPage)
	{
		//
		// Show Scrollbar:
		int nMaxRange = nSize - m_nItemCountPerPage;
		if (nMaxRange < 0)
			nMaxRange = m_nItemCountPerPage;
		m_vcroll.SetScrollRange (0, nMaxRange);
		if (m_nCurPos > nMaxRange)
			m_nCurPos = nMaxRange;

		r.left = r.right - cxScrollBar;
		m_vcroll.MoveWindow(r);
		r.left  = 0;
		r.right = r.right - cxScrollBar -1;
		m_pListCtrl->MoveWindow (r);
		m_vcroll.ShowWindow(SW_SHOW);
	}
	else
	{
		//
		// Hide Scrollbar:
		m_vcroll.ShowWindow(SW_HIDE);
		m_pListCtrl->MoveWindow (r);
	}

	if (m_pfnDisplayItem)
		Display(m_nCurPos);
}

void CuListCtrlHuge::Display (int nCurrentScrollPos, BOOL bUpdate)
{
	ASSERT (m_pData && m_pfnDisplayItem);
	if (!m_pData || !m_pfnDisplayItem)
		return;
	int i, nSize = m_pData->GetSize();
	int nCount = 0, nItemPerPage = m_pListCtrl->GetCountPerPage();
	if (!bUpdate)
		m_pListCtrl->DeleteAllItems();

	i = nCurrentScrollPos;
	while (i < nSize && nCount < nItemPerPage)
	{
		m_pfnDisplayItem (m_pData->GetAt(i), m_pListCtrl, nCount, bUpdate, m_pUserInfo);

		nCount++;
		i++;
	}

	if (bUpdate && nCount < nItemPerPage)
	{
		m_pListCtrl->LockWindowUpdate();
		while (m_pListCtrl->DeleteItem (nCount));
		m_pListCtrl->UnlockWindowUpdate();
	}
	m_nCurPos = nCurrentScrollPos;
	m_vcroll.SetScrollPos(m_nCurPos);
}


void CuListCtrlHuge::Cleanup()
{
	if (m_pData && !m_bSharedData)
	{
		m_pData->RemoveAll();
	}
	ResizeToFit();
}



void CuListCtrlHuge::OnDestroy() 
{
	m_arrayPtr.RemoveAll();
	CWnd::OnDestroy();
}

void CuListCtrlHuge::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	if (!IsWindow(m_pListCtrl->m_hWnd))
		return;
	m_nItemCountPerPage = m_pListCtrl->GetCountPerPage();
	ResizeToFit();
}

void CuListCtrlHuge::ScrollLineUp()
{
//	m_pListCtrl->LockWindowUpdate();
//	int nCount  = m_pListCtrl->GetItemCount();
//	m_pListCtrl->DeleteItem (nCount-1);

//	int nCurPos = m_vcroll.GetScrollPos();
	Display(m_nCurPos, TRUE);
//	m_pfnDisplayItem (m_pData->GetAt(nCurPos), m_pListCtrl, 0, FALSE, m_pUserInfo);
//	m_pListCtrl->UnlockWindowUpdate();
}

void CuListCtrlHuge::ScrollLineDown()
{/*
	m_pListCtrl->DeleteItem (0);
	int nCount  =m_pListCtrl->GetItemCount();
	int nCurPos = m_vcroll.GetScrollPos();
	int nItemPerPage = m_pListCtrl->GetCountPerPage();
	int nLastItem = nCurPos + nItemPerPage -1;
	m_pfnDisplayItem (m_pData->GetAt(nLastItem), m_pListCtrl, nItemPerPage-1, FALSE, m_pUserInfo);
*/
//	int nCurPos = m_vcroll.GetScrollPos();
	Display(m_nCurPos, TRUE);

}

void CuListCtrlHuge::ScrollPageUp()
{
	Display(m_nCurPos, TRUE);
}

void CuListCtrlHuge::ScrollPageDown()
{
	Display(m_nCurPos, TRUE);
}


afx_msg void CuListCtrlHuge::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	if (pScrollBar == &m_vcroll)
	{
		m_nCurPos = m_vcroll.GetScrollPos();

		switch (nSBCode)
		{
		case SB_TOP:
			m_nCurPos = 0;
			break;
		case SB_BOTTOM:
			m_nCurPos = m_vcroll.GetScrollLimit()-1;
			break;
		case SB_LINEUP:
			if (m_nCurPos == 0)
				return;
			m_nCurPos --;
			m_vcroll.SetScrollPos(m_nCurPos);
			ScrollLineUp();
			break;
		case SB_LINEDOWN:
			if (m_nCurPos == (m_vcroll.GetScrollLimit()-1))
				return;
			m_nCurPos++;
			m_vcroll.SetScrollPos(m_nCurPos);
			ScrollLineDown();
			break;
		case SB_PAGEUP:
			if (m_nCurPos == 0)
				return;
			m_nCurPos -= m_nItemCountPerPage;
			if (m_nCurPos < 0)
				m_nCurPos = 0;
			m_vcroll.SetScrollPos(m_nCurPos);
			ScrollPageUp();
			break;
		case SB_PAGEDOWN:
			if (m_nCurPos == (m_vcroll.GetScrollLimit()-1))
				return;
			m_nCurPos += m_nItemCountPerPage;
			if (m_nCurPos >= m_vcroll.GetScrollLimit())
				m_nCurPos = m_vcroll.GetScrollLimit()-1;
			m_vcroll.SetScrollPos(m_nCurPos);
			ScrollPageDown();
			break;
		
		case SB_THUMBTRACK:
			m_vcroll.GetScrollInfo(&m_scrollInfo);
			m_nCurPos = m_scrollInfo.nTrackPos;
			m_bTrackPos = TRUE;
			DrawThumbTrackInfo(m_nCurPos);
			break;

		case SB_THUMBPOSITION:
			m_vcroll.GetScrollInfo(&m_scrollInfo);
			m_nCurPos = m_scrollInfo.nTrackPos;
			m_bTrackPos = TRUE;
			break;

		case SB_ENDSCROLL:
			m_vcroll.SetScrollPos(m_nCurPos);
			if (m_bTrackPos)
			{
				Display(m_nCurPos, TRUE);
				DrawThumbTrackInfo(m_nCurPos, TRUE); // Erase
			}
			m_bTrackPos = FALSE;
			break;
		}
		m_vcroll.SetScrollPos(m_nCurPos);
	}
}

void CuListCtrlHuge::On_KeydownListCtrl(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	WORD wVKey = pLVKeyDow->wVKey;
	POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();
	if (pos == NULL)
		return;
	int nCount= m_pListCtrl->GetItemCount();
	int nItem = m_pListCtrl->GetNextSelectedItem (pos);
	if (nItem == -1)
		return;

	switch (wVKey)
	{
	case VK_DOWN:
		if (m_nCurPos == (m_vcroll.GetScrollLimit()-1))
			return;
		if (nItem != (nCount-1))
			return; // The current selected item is not the bottom item
		m_nCurPos++;
		m_vcroll.SetScrollPos(m_nCurPos);
		ScrollLineDown();
		break;

	case VK_UP:
		if (m_nCurPos == 0)
			return;
		if (nItem != 0)
			return; // The current selected item is not the top item
		m_nCurPos --;
		m_vcroll.SetScrollPos(m_nCurPos);
		ScrollLineUp();
		break;

	case VK_NEXT:
		if (m_nCurPos == (m_vcroll.GetScrollLimit()-1))
			return;
		m_nCurPos += m_nItemCountPerPage;
		if (m_nCurPos >= m_vcroll.GetScrollLimit())
			m_nCurPos = m_vcroll.GetScrollLimit()-1;
		m_vcroll.SetScrollPos(m_nCurPos);
		ScrollPageDown();
		break;

	case VK_PRIOR:
		if (m_nCurPos == 0)
			return;
		m_nCurPos -= m_nItemCountPerPage;
		if (m_nCurPos < 0)
			m_nCurPos = 0;
		m_vcroll.SetScrollPos(m_nCurPos);
		ScrollPageUp();
		break;

	case VK_HOME:
		if (m_nCurPos == 0)
			return;
		m_nCurPos = 0;
		m_vcroll.SetScrollPos(m_nCurPos);
		Display(m_nCurPos);
		break;

	case VK_END:
		if (m_nCurPos == (m_vcroll.GetScrollLimit()-1))
			return;
		m_nCurPos = m_vcroll.GetScrollLimit()-1;
		m_vcroll.SetScrollPos(m_nCurPos);
		Display(m_nCurPos);
		break;

	default:
		break;
	}
	*pResult = 0;
	OnKeydownListCtrl(pNMHDR, pResult);
}

void CuListCtrlHuge::DrawThumbTrackInfo (int nPos, BOOL bErase)
{
	CClientDC dc (this);
	CFont* pFont = m_pListCtrl->GetFont();
	CFont* pOldFond = dc.SelectObject (pFont);
	
	if (!m_bDrawInfo)
	{
		GetCursorPos(&m_pointInfo);
		ScreenToClient (&m_pointInfo);
		m_pointInfo.x -= 20;
	}

	int nSize = m_pData->GetSize();
	CString strSize;
	strSize.Format (_T("Scroll To Line (%d,%d)"), nSize, nSize);

	CRect r;
	CString strMsg;
	strMsg.Format (_T("Scroll To Line (%d/%d)"), nPos+1, m_pData->GetSize());
	CSize txSize = dc.GetTextExtent ((LPCTSTR)strSize);
	
	r.top    = m_pointInfo.y;
	r.left   = m_pointInfo.x - txSize.cx - 10;
	r.right  = m_pointInfo.x;
	r.bottom = r.top + txSize.cy + 4;

	COLORREF oldTextBkColor = dc.SetBkColor (RGB(244, 245, 200));
	CBrush br (RGB(244, 245, 200));
	dc.FillRect(r, &br);
	if (bErase)
	{
		Invalidate();
		m_bDrawInfo = FALSE;
	}
	else
	{
		dc.DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER|DT_NOCLIP);
		m_bDrawInfo = TRUE;
	}
	dc.SetBkColor (oldTextBkColor);
	dc.SelectObject (pOldFond);
}

void CuListCtrlHuge::On_ItemchangedListCtrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	OnItemchangedListCtrl(pNMHDR, pResult);
}

void CuListCtrlHuge::On_ColumnclickListCtrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	OnColumnclickListCtrl(pNMHDR, pResult);
}

void CuListCtrlHuge::OnKeydownListCtrl (NMHDR* pNMHDR, LRESULT* pResult)
{
}

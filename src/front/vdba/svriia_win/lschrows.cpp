/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lschrows.cpp: implementation of the CuListCtrlDouble class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The control that has 3 controls, header control, and 2 CListCtrl.
**               The first list control has no scrolling management.
**
** History:
**
** 03-Jan-2001 (uk$so01)
**    Created
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 23-Mar-2005 (komve01)
**    Issue# 13919436, Bug#113774. 
**    Added error handling to the function OnEditDlgOK, to return an error
**    if the validation went wrong somewhere.
**/

#include "stdafx.h"
#include "lschrows.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Class CuListCtrlDoubleUpper
// ************************************************************************************************
void CuListCtrlDoubleUpper::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	EditValue (index, iColumnNumber, rCell);
}


void CuListCtrlDoubleUpper::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber = 1;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	EditValue (index, iColumnNumber, rCell);
}


LONG CuListCtrlDoubleUpper::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	EDIT_GetEditItem(iItem, iSubItem);
	CString s = EDIT_GetText();
	LONG lRetVal = 0L;

	if (iItem < 0)
	{
		SetFocus();
		return lRetVal;
	}

	try
	{
		s.TrimLeft();
		s.TrimRight();
		CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(iItem);
		if (pItemData)
		{
			if (pItemData->UpdateValue (this, iItem, iSubItem, s))
				SetItemText (iItem, iSubItem, s);
			else
				lRetVal = 1L;
		}
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)(LPCTSTR)m_EditDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuListCtrlDoubleUpper::OnEditDlgOK, Cannot change edit text."));
	}
	SetFocus();
	return lRetVal;
}


LONG CuListCtrlDoubleUpper::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	EDITNUMBER_GetEditItem(iItem, iSubItem);
	CString s = EDITNUMBER_GetText();

	s.TrimLeft();
	s.TrimRight();

	try
	{
		s.TrimLeft();
		s.TrimRight();
		if (iItem < 0 || s.IsEmpty())
			return 0;
		int i, nLen = s.GetLength();
		for (i=0; i<nLen; i += _tclen((const TCHAR*)s + i))
		{
			if (!_istdigit(s.GetAt(i)))
			{
				SetFocus();
				return 0L;
			}
		}

		CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(iItem);
		if (pItemData)
		{
			if (pItemData->UpdateValue (this, iItem, iSubItem, s))
				SetItemText (iItem, iSubItem, s);
		}
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTEDITNUMBERDLG_OK, (WPARAM)(LPCTSTR)m_EditNumberDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuListCtrlDoubleUpper::OnEditNumberDlgOK, Cannot change edit text."));
	}
	SetFocus();
	return 0L;
}


LONG CuListCtrlDoubleUpper::OnComboDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	COMBO_GetEditItem(iItem, iSubItem);
	CString s = COMBO_GetText();
	s.TrimLeft();
	s.TrimRight();

	if (iItem < 0 || s.IsEmpty())
		return 0;
	if (iItem != -1 && iSubItem > 0)
	{
		CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(iItem);
		if (pItemData)
		{
			if (pItemData->UpdateValue (this, iItem, iSubItem, s))
				SetItemText (iItem, iSubItem, s);
		}
	}
	//
	// Inform the parent that something changes:
	GetParent()->SendMessage (WM_LAYOUTCOMBODLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);
	
	return 0L;
}



void CuListCtrlDoubleUpper::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(iItem);
	if (!pItemData)
		return;
	pItemData->EditValue (this, iItem, iSubItem, rcCell);
}


void CuListCtrlDoubleUpper::DisplayItem(int nItem)
{
	int i, nCount = GetItemCount();
	if (nItem == -1)
	{
		for (i=0; i<nCount; i++)
		{
			CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(i);
			if (pItemData)
			{
				pItemData->Display (this, i);
			}
		}
	}
	else
	{
		CaHeaderRowItemData* pItemData = (CaHeaderRowItemData*)GetItemData(nItem);
		if (pItemData)
		{
			pItemData->Display (this, nItem);
		}
	}
}


BEGIN_MESSAGE_MAP(CuListCtrlDoubleUpper, CuEditableListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()

	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,  OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL, OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK, OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK, OnComboDlgOK)
END_MESSAGE_MAP()




// CLASS CuListCtrlDoubleLower
// ************************************************************************************************
CuListCtrlDoubleLower::CuListCtrlDoubleLower():CuListCtrlHugeChildList()
{
	m_pParent = NULL;
	m_pHeader = NULL;
	m_pHeaderRows = NULL;
	m_nCurrentScrollPos = 0;
}

CuListCtrlDoubleLower::~CuListCtrlDoubleLower()
{
}


BEGIN_MESSAGE_MAP(CuListCtrlDoubleLower, CuListCtrlHugeChildList)
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CuListCtrlDoubleLower::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int nPageWidth = 100;
	int nCurPos  = GetScrollPos(SB_HORZ);
	int nPrevPos = nCurPos;

	switch(nSBCode)
	{
		case SB_THUMBPOSITION:
			if(nPos==0)
				m_nCurrentScrollPos = 0;
			else
				m_nCurrentScrollPos = nPos; 
			return;
			break;
		default:
			break;
	}

	CWnd* pParent = GetParent();
	CuListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
	m_nCurrentScrollPos  = GetScrollPos(SB_HORZ);
	if (nPrevPos == m_nCurrentScrollPos)
		return;

	SetScrollPos(SB_HORZ, m_nCurrentScrollPos);
	CRect r;
	GetClientRect (r);
	int nWewWidtht = r.Width();
	m_pHeader->GetWindowRect (r);
	ScreenToClient (r);
	m_pHeader->SetWindowPos(&wndTop, - m_nCurrentScrollPos, 0, nWewWidtht + m_nCurrentScrollPos, r.Height(), SWP_SHOWWINDOW);

	m_pHeaderRows->GetWindowRect (r);
	pParent->ScreenToClient (r);
	m_pHeaderRows->SetWindowPos(&wndTop, - m_nCurrentScrollPos, r.top, nWewWidtht + m_nCurrentScrollPos, r.Height(), SWP_SHOWWINDOW);

	if (nSBCode == SB_ENDSCROLL)
	{
		int nNewX =GetScrollPos(SB_HORZ);
		CRect r;
		GetClientRect (r);
		int nWewWidtht = r.Width();
		m_pHeader->GetWindowRect (r);
		pParent->ScreenToClient (r);
		m_pHeader->SetWindowPos(&wndTop, -nNewX, 0, nWewWidtht + nNewX, r.Height(), SWP_SHOWWINDOW);

		m_pHeaderRows->GetWindowRect (r);
		pParent->ScreenToClient (r);
		m_pHeaderRows->SetWindowPos(&wndTop, -nNewX, r.top, nWewWidtht + nNewX, r.Height(), SWP_SHOWWINDOW);

		m_pHeader->UpdateWindow();
		m_pHeaderRows->UpdateWindow();
		return;
	}

	CRect rx;
	m_pHeader->GetWindowRect (rx);
	m_pHeader->ScreenToClient(rx);
	m_pHeader->UpdateWindow();

	m_pHeaderRows->GetWindowRect (rx);
	m_pHeaderRows->ScreenToClient(rx);
	m_pHeaderRows->UpdateWindow();
}

void CuListCtrlDoubleLower::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//
	// Just ignore the base class implementation:
	((CuListCtrlDoubleUpper*)m_pHeaderRows)->HideProperty(TRUE);
	return;
}



//
// Class CuListCtrlDouble
// ************************************************************************************************
IMPLEMENT_DYNCREATE(CuListCtrlDouble, CuListCtrlHuge)
CuListCtrlDouble::CuListCtrlDouble():CuListCtrlHuge()
{
	IDC_HEADER     = 1500;
	IDC_HEADERROWS = 1501;
	IDC_LISTCTRL   = 1502;
	IDC_STATICDUMMY= 1503;
	m_nCurrentScrollPos = 0;

	m_nHeaderHeight= 25;
	m_nRowInfo = 5;

	if (m_pListCtrl)
		delete m_pListCtrl;
	CuListCtrlDoubleLower* pListCtrl = new CuListCtrlDoubleLower();
	m_pListCtrl = pListCtrl;
	m_nListCtrlStyle |= LVS_NOCOLUMNHEADER;
}

CuListCtrlDouble::~CuListCtrlDouble()
{
}


BEGIN_MESSAGE_MAP(CuListCtrlDouble, CuListCtrlHuge)
	//{{AFX_MSG_MAP(CuListCtrlDouble)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CuListCtrlDouble::InitializeHeader(LSCTRLHEADERPARAMS* pArrayHeader, int nSize)
{
	LVCOLUMN lvc;
	HDITEM hitem;
	memset (&lvc, 0, sizeof (lvc));
	memset (&hitem, 0, sizeof (hitem));
	//
	// Delete Old data;
	Cleanup();
	hitem.mask = HDI_WIDTH;
	lvc.mask = LVCF_FMT;
	while (m_header.GetItem (0, &hitem))
		m_header.DeleteItem (0);
	while (m_listCtrlHeader.GetColumn (0, &lvc))
		m_listCtrlHeader.DeleteColumn (0);

	//
	// Initialize header of List Control 'm_pListCtrl' i.e
	// the lower list (CuListCtrlDoubleLower)
	CuListCtrlHuge::InitializeHeader(pArrayHeader, nSize);

	//
	// Initialize header of List Control 'm_listCtrlHeader' i.e
	// the upper list (CuListCtrlDoubleUpper)
	memset (&hitem, 0, sizeof (hitem));
	memset (&lvc, 0, sizeof (lvc));
	lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	hitem.mask = HDI_TEXT|HDI_WIDTH ;

	for (int i=0; i<nSize; i++)
	{
		hitem.pszText  = pArrayHeader[i].m_tchszHeader;
		hitem.cxy  = pArrayHeader[i].m_cxWidth;
		m_header.InsertItem (i, &hitem);

		lvc.fmt      = pArrayHeader[i].m_fmt;
		lvc.pszText  = pArrayHeader[i].m_tchszHeader;
		lvc.iSubItem = i;
		lvc.cx       = pArrayHeader[i].m_cxWidth;

		m_listCtrlHeader.InsertColumn (i, &lvc);
	}
	
	// 
	// SetWindowPos of Header Control:
	CRect r;
	GetClientRect (r);
	int nWewWidtht = r.Width();
	m_header.GetWindowRect (r);
	ScreenToClient (r);
	m_header.SetWindowPos(
		&wndTop, 
		0, 
		0, // Always
		nWewWidtht, 
		r.Height(), 
		SWP_SHOWWINDOW);

	// 
	// SetWindowPs of List Control 1:
	m_listCtrlHeader.GetWindowRect (r);
	ScreenToClient (r);
	m_listCtrlHeader.SetWindowPos(
		&wndTop, 
		0, 
		r.top, 
		nWewWidtht , 
		r.Height(), 
		SWP_SHOWWINDOW);
	GetClientRect (r);

	ResizeControl(r);
}



int CuListCtrlDouble::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	LPCREATESTRUCT lpCS = lpCreateStruct;
	if (CuListCtrlHuge::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_ImageList.Create(1, 18, TRUE, 1, 0);
	CFont* pFont = NULL;
	int nCxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	CRect rcClient;
	GetClientRect (rcClient);

	CRect r (
		rcClient.right - nCxVScroll, 
		rcClient.left, 
		rcClient.right, 
		rcClient.top + m_nHeaderHeight+1 + 50);
	m_cStaticDummy.Create (NULL, WS_CHILD, r, this, IDC_STATICDUMMY);

	//
	// Create the header control:
	r  = CRect(
		rcClient.left, 
		rcClient.top, 
		rcClient.right - nCxVScroll, 
		rcClient.top + m_nHeaderHeight);
	UINT nStyle = WS_CHILD|WS_VISIBLE|HDS_HORZ|CCS_TOP;
	m_header.Create   (nStyle, r, this, IDC_HEADER);
	m_header.ModifyStyle(HDS_DRAGDROP|HDS_FULLDRAG, 0, 0 );
	m_header.ShowWindow (SW_SHOW);
	
	//
	// Create the first list control (fixed rows info or upper list)
	r  = CRect(
		rcClient.left, 
		rcClient.top + m_nHeaderHeight+1, 
		rcClient.right - nCxVScroll, 
		rcClient.top + m_nHeaderHeight + 1 + 50);
	nStyle = WS_CHILD|LVS_REPORT|LVS_NOSCROLL|LVS_NOCOLUMNHEADER|WS_CLIPSIBLINGS;
	//
	// Draw only vertical line:
	m_listCtrlHeader.SetLineSeperator(TRUE, TRUE, FALSE);
	m_listCtrlHeader.SetFullRowSelected(TRUE);
	m_listCtrlHeader.ActivateColorItem();

	m_listCtrlHeader.Create (nStyle, r, this, IDC_HEADERROWS);
	m_listCtrlHeader.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_listCtrlHeader.InsertItem (0, _T("Xyz"));
	m_listCtrlHeader.GetItemRect(0, m_rcItem1, LVIR_BOUNDS);
	m_listCtrlHeader.DeleteAllItems();
	r  = CRect(
		rcClient.left, 
		rcClient.top + m_nHeaderHeight+1,
		rcClient.right - nCxVScroll, 
		rcClient.top + m_nHeaderHeight + 1 + 50);
	m_listCtrlHeader.MoveWindow (r);
	m_listCtrlHeader.ShowWindow (SW_SHOW);

	//
	// Create the second list control:
	CRect rcList1;
	m_listCtrlHeader.GetWindowRect (rcList1);
	ScreenToClient(rcList1);
	r = CRect (
		lpCS->x, 
		rcList1.bottom+1, 
		lpCS->x + lpCS->cx-4,
		lpCS->y + lpCS->cy);

	nStyle = WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_NOCOLUMNHEADER|WS_CLIPSIBLINGS;

	//
	// Draw only vertical line:
	CuListCtrlDoubleLower* pCtrl = (CuListCtrlDoubleLower*)m_pListCtrl;
	pCtrl->SetLineSeperator(TRUE, TRUE, FALSE);
	pCtrl->SetHeader(&m_header);
	pCtrl->SetHeaderRows(&m_listCtrlHeader);
	pCtrl->ShowWindow (SW_SHOW);

	pFont = pCtrl->GetFont();
	ASSERT (pFont);
	if (pFont)
		m_header.SetFont(pFont);

	return 0;
}

void CuListCtrlDouble::OnDestroy() 
{
	m_ImageList.DeleteImageList();
	CuListCtrlHuge::OnDestroy();
}

void CuListCtrlDouble::Cleanup()
{
	CuListCtrlHuge::Cleanup();
	m_listCtrlHeader.DeleteAllItems();
	m_pListCtrl->DeleteAllItems();
	ResizeToFit();
}



void CuListCtrlDouble::OnSize(UINT nType, int cx, int cy) 
{
	CuListCtrlHuge::OnSize(nType, cx, cy);

	CRect r;
	GetClientRect (r);
	ResizeControl(r);
}



BOOL CuListCtrlDouble::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pNotifyStruct = (NMHDR*)lParam;
	if (!pNotifyStruct)
		return CuListCtrlHuge::OnNotify(wParam, lParam, pResult);

	if (pNotifyStruct->idFrom == IDC_HEADER)
	{
		if (pNotifyStruct->code == HDN_ENDTRACK)
		{
			NMHEADER* pHdr = (NMHEADER*)lParam;
			CuListCtrlDoubleLower* pCtrl = (CuListCtrlDoubleLower*)m_pListCtrl;

			m_listCtrlHeader.SetColumnWidth (pHdr->iItem, pHdr->pitem->cxy);
			pCtrl->SetColumnWidth (pHdr->iItem, pHdr->pitem->cxy);

			int nNewX = pCtrl->GetScrollPos(SB_HORZ);
			pCtrl->SetCurrentScrollPos (nNewX);

			int nSize = m_pData->GetSize();
			m_nItemCountPerPage = m_pListCtrl->GetCountPerPage();
			BOOL bList2HasVScroll = nSize > m_nItemCountPerPage;
			int nCxDummy = bList2HasVScroll? GetSystemMetrics(SM_CXVSCROLL): 0;
			
			// 
			// SetWindowPs of Header Control:
			CRect r;
			GetClientRect (r);
			int nWewWidtht = r.Width();
			m_header.GetWindowRect (r);
			ScreenToClient (r);
			m_header.SetWindowPos(
				&wndTop, 
				-nNewX, 
				0, // Always
				nWewWidtht + nNewX - nCxDummy, 
				r.Height(), 
				SWP_SHOWWINDOW);

			// 
			// SetWindowPs of List Control 1:
			m_listCtrlHeader.GetWindowRect (r);
			ScreenToClient (r);
			m_listCtrlHeader.SetWindowPos(
				&wndTop, 
				-nNewX, 
				r.top, 
				nWewWidtht + nNewX - nCxDummy, 
				r.Height(), 
				SWP_SHOWWINDOW);
			/*
			// 
			// Show/Hide and Resize the Dummy Static Control:
			if (bList2HasVScroll)
			{
				CRect rx, rcClient;
				GetClientRect (rcClient);
				m_pListCtrl->GetWindowRect (rx);
				ScreenToClient(rx);
				int nY = rx.top;
				r = rcClient;
				r.left   = r.right - GetSystemMetrics(SM_CYVSCROLL);
				r.bottom = nY;
				m_cStaticDummy.MoveWindow (r);
				m_cStaticDummy.ShowWindow (SW_SHOW);
			}
			else
			{
				m_cStaticDummy.ShowWindow (SW_HIDE);
			}
			*/

			m_listCtrlHeader.Invalidate();
			m_header.Invalidate();
		}
	}
	else
	if (pNotifyStruct->idFrom == IDC_HEADERROWS)
	{

	}
	else
	if (pNotifyStruct->idFrom == IDC_LISTCTRL)
	{
	}

	return CuListCtrlHuge::OnNotify(wParam, lParam, pResult);
}



void CuListCtrlDouble::ResizeControl (CRect rcClient)
{
	if (!IsWindow(m_header.m_hWnd))
		return;
	CRect r, rcPos;
	int nCxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	//
	// Resize the header:
	m_header.GetWindowRect (rcPos);
	ScreenToClient (rcPos);
	rcPos.top = 0; // Always
	m_header.SetWindowPos (
		&wndTop, 
		rcPos.left,
		rcPos.top, 
		rcClient.right - rcPos.left - nCxVScroll,
		rcPos.top + m_nHeaderHeight,
		SWP_SHOWWINDOW);
	//
	// Resize the list control 1:
	m_header.GetWindowRect (r);
	ScreenToClient (r);
	m_listCtrlHeader.GetWindowRect (rcPos);
	ScreenToClient (rcPos);
	m_listCtrlHeader.SetWindowPos (
		&wndTop, 
		rcPos.left, 
		r.bottom+1, 
		rcClient.right - rcPos.left - nCxVScroll,
		m_nRowInfo*m_rcItem1.Height(), 
		SWP_SHOWWINDOW);
	//
	// Resize the list control 2:
	m_listCtrlHeader.GetWindowRect (r);
	ScreenToClient (r);
	r.top   = r.bottom + 1;
	r.left  = rcClient.left;
	r.right = rcClient.right;
	r.bottom= rcClient.bottom;
	m_pListCtrl->MoveWindow(r);

	//
	// Detect if the vertical scrollbar should be shown:
	int nSize = m_pData->GetSize();
	m_nItemCountPerPage = m_pListCtrl->GetCountPerPage();
	BOOL bList2HasVScroll = nSize > m_nItemCountPerPage;
	//
	// Resize the static control (dummy):
	if (bList2HasVScroll)
	{
		int nY  = r.top;
		r.right-= nCxVScroll;
		m_pListCtrl->MoveWindow(r);

		r = rcClient;
		r.left   = r.right - nCxVScroll;
		r.bottom = nY;
		m_cStaticDummy.MoveWindow (r);
		m_cStaticDummy.ShowWindow (SW_SHOW);

		//
		// Show the vertical scrollbar:
		r = rcClient;
		m_vcroll.SetScrollRange (0, nSize - m_nItemCountPerPage);
		r.left = r.right - nCxVScroll;
		r.top  = nY;
		m_vcroll.MoveWindow(r);
		m_vcroll.ShowWindow (SW_SHOW);
	}
	else
	{
		m_cStaticDummy.ShowWindow (SW_HIDE);
		m_vcroll.ShowWindow (SW_HIDE);
	}

	if (!bList2HasVScroll)
	{
		nCxVScroll = 0;
		//
		// Need to resize the 'm_header' and 'm_listCtrlHeader'again if bList2HasVScroll = FALSE
		// because previusly we set nCxVScroll = GetSystemMetrics(SM_CXVSCROLL);
		//
		// Resize the header:
		m_header.GetWindowRect (rcPos);
		ScreenToClient (rcPos);
		rcPos.top = 0; // Always
		m_header.SetWindowPos (
			&wndTop, 
			rcPos.left, 
			rcPos.top, 
			rcClient.right - rcPos.left - nCxVScroll,
			rcPos.top + m_nHeaderHeight,
			SWP_SHOWWINDOW);
		//
		// Resize the list control 1:
		m_header.GetWindowRect (r);
		ScreenToClient (r);
		m_listCtrlHeader.GetWindowRect (rcPos);
		ScreenToClient (rcPos);
		m_listCtrlHeader.SetWindowPos (
			&wndTop, 
			rcPos.left, 
			r.bottom+1, 
			rcClient.right - rcPos.left - nCxVScroll,
			m_nRowInfo*m_rcItem1.Height(), 
			SWP_SHOWWINDOW);
	}
}

void CuListCtrlDouble::ResizeToFit()
{
	CRect r;
	GetClientRect (r);
	ResizeControl (r);
	m_nCurPos = 0;
	m_vcroll.SetScrollPos(m_nCurPos);
	if (m_pfnDisplayItem)
	{
		Display(m_nCurPos);
	}
}




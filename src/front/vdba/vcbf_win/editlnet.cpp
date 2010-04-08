/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : editlnet.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : Blattes Emmanuel
**    Purpose  : Special Owner draw List Control for the Net Protocol Page 
**               See the CONCEPT.DOC for more detail
**
** History:
**
** xx-Sep-1997: (blaem01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
**/

#include "stdafx.h"
#include "vcbf.h"
#include "editlnet.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlNet

CuEditableListCtrlNet::CuEditableListCtrlNet()
	:CuEditableListCtrl()
{

}

CuEditableListCtrlNet::~CuEditableListCtrlNet()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlNet, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlNet)
	ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,		OnEditDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,			OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL,	OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,		OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL,	OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,		OnEditSpinDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlNet message handlers
	
LONG CuEditableListCtrlNet::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	ASSERT (iSubItem == 2);

	// optimization: if text not changed or if text empty, behave as if cancelled
	if (s.IsEmpty())
	  return 0L;
	if (s == m_EditDlgOriginalText)
	  return 0L;

	try
	{
		// Test acceptation at low level side
		LPNETPROTLINEINFO lpProtocol = (LPNETPROTLINEINFO)GetItemData(iItem);
		ASSERT (lpProtocol);
		AfxGetApp()->DoWaitCursor(1);
		BOOL bSuccess = VCBFll_netprots_OnEditListen(lpProtocol, (LPTSTR)(LPCTSTR)s, lpProtocol->nProtocolType);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
		  MessageBeep(MB_ICONEXCLAMATION);
		
		// always display text as returned in the structure by VCBFllBlahBlah()
		SetItemText (iItem, iSubItem, (LPCTSTR)lpProtocol->szlisten);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlNet::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlNet::OnEditDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrlNet::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();

	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrlNet::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}



LONG CuEditableListCtrlNet::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	int iMin, iMax;
	CString s = EDITSPIN_GetText();
	if (s.IsEmpty())
	{
		EDITSPIN_GetRange (iMin, iMax);
		s.Format (_T("%d"), iMin);
	}
	EDITSPIN_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrlNet::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

void CuEditableListCtrlNet::OnRButtonDown(UINT nFlags, CPoint point) 
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
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	switch (iColumnNumber)
	{
	case 2:
		SetEditText   (index, iColumnNumber, rCell);
		break;

	default:
		break;
	}
}




void CuEditableListCtrlNet::OnDestroy() 
{
	CuEditableListCtrl::OnDestroy();
}



void CuEditableListCtrlNet::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnLButtonDblClk(nFlags, point);
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
	switch (iColumnNumber)
	{
	case 2:
		SetEditText   (index, iColumnNumber, rCell);
		break;

	default:
		break;
	}
}



void CuEditableListCtrlNet::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}


LONG CuEditableListCtrlNet::ManageEditDlgOK (UINT wParam, LONG lParam)
{
  return OnEditDlgOK(wParam, lParam);
}


void CuEditableListCtrlNet::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	ASSERT (iSubItem == 1);
	
	try
	{
		// Test acceptation at low level side
		LPNETPROTLINEINFO lpProtocol = (LPNETPROTLINEINFO)GetItemData(iItem);
		ASSERT (lpProtocol);
		AfxGetApp()->DoWaitCursor(1);
		BOOL bSuccess;
		if (bCheck)
		  bSuccess = VCBFll_netprots_OnEditStatus(lpProtocol, _T("ON"), lpProtocol->nProtocolType);
		else
		  bSuccess = VCBFll_netprots_OnEditStatus(lpProtocol, _T("OFF"), lpProtocol->nProtocolType);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
		  MessageBeep(MB_ICONEXCLAMATION);
		
		// always display check state as returned in the structure by VCBFllBlahBlah()
		if (_tcsicmp((LPCTSTR)lpProtocol->szstatus, _T("ON")) == 0) // On/Off
			SetCheck (iItem, 1, TRUE);
		else
			SetCheck (iItem, 1, FALSE);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CCuEditableListCtrlNet::OnCheckChange has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

int CuEditableListCtrlNet::GetCurSel()
{
  // ASSUME MONOSELECTION! IF SEVERAL, RETURNS THE FIRST ONE

	int nCount = GetItemCount();
	for (int i=0; i<nCount; i++) {
		UINT nState = GetItemState (i, LVIS_SELECTED);
		if (nState & LVIS_SELECTED)
      return i;
  }
  return -1;
}

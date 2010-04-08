/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : Editlsco.cpp 
**    Project  : OpenInges Configuration Manager 
**    Author   : Emmanuel Blattes - UK Sotheavut 
**    Purpose  : list control customized for components list in cbf
**               derived from CuEditableListCtrl
**
** History:
**
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 31-Oct-2001 (hanje04)
**    Move declaration of cpt outside for loops to stop non-ANSI compiler 
**    errors on Linux.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "vcbf.h"
#include "editlsco.h"
#include "cbfitem.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlComponentComponent

CuEditableListCtrlComponent::CuEditableListCtrlComponent()
	:CuEditableListCtrl()
{
}

CuEditableListCtrlComponent::~CuEditableListCtrlComponent()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlComponent, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlComponent)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,			OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,		OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,		OnEditSpinDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK,			OnComboDlgOK)

	ON_MESSAGE (WM_LAYOUTEDITINTEGERSPINDLG_OK, OnEditSpinDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITINTEGERDLG_OK, OnEditNumberDlgOK)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlComponent message handlers
	
LONG CuEditableListCtrlComponent::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0) {
	  MessageBeep(MB_ICONEXCLAMATION);
			return 0L;
	}
	
	// optimization: if text not changed or if text empty, behave as if cancelled
	if (s.IsEmpty())
	  return 0L;
	if (s == m_EditDlgOriginalText)
	  return 0L;

  // Emb Oct 22, 97: don't accept underscore in name
  if (s.Find(_T('_')) != -1) {
    AfxMessageBox(IDS_ERR_UNDERSCORE);
  	SetItemText (iItem, iSubItem, m_EditDlgOriginalText);
    return 0L;    // As if not changed
  }

  // Emb Oct 22, 97: must check whether another item has the same name
  int count = GetItemCount();
  CuCbfListViewItem* pCurItemData = (CuCbfListViewItem*)GetItemData(iItem);
	LPCOMPONENTINFO lpCurComponentInfo = &pCurItemData->m_componentInfo;
  for (int cpt = 0; cpt < count; cpt++) {
    // exclude current item
    if (cpt == iItem)
      continue;

    // get component info of item
    CuCbfListViewItem* pItem = (CuCbfListViewItem*)GetItemData(cpt);
	  LPCOMPONENTINFO lpItemInfo = &pItem->m_componentInfo;

    // compare types
    if (lpItemInfo->itype != lpCurComponentInfo->itype)
      continue;

    // compare names - INSENSIVIVE
    if (s.CompareNoCase((LPCTSTR)lpItemInfo->szname) != 0)
      continue;

    // same type and name : denied
    AfxMessageBox(IDS_ERR_DUPLICATE_NAME);
  	SetItemText (iItem, iSubItem, m_EditDlgOriginalText);
    return 0L;    // As if not changed
  }

	// Emb Sept 25, 97: preliminary test at low level side
	CuCbfListViewItem* pItemData = (CuCbfListViewItem*)GetItemData(iItem);
	LPCOMPONENTINFO lpComponentInfo = &pItemData->m_componentInfo;
	AfxGetApp()->DoWaitCursor(1);
	try
	{
		BOOL bSuccess = VCBFllValidRename(lpComponentInfo, (LPTSTR)(LPCTSTR)s);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
			MessageBeep(MB_ICONEXCLAMATION);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlComponent::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
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
	// always display text as returned in the structure by VCBFllValidRename()
	SetItemText (iItem, iSubItem, (LPCTSTR)lpComponentInfo->szname);

	return 0L;
}

LONG CuEditableListCtrlComponent::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITSPIN_GetText();

	// optimization: if text not changed or if text empty, behave as if cancelled
	if (s.IsEmpty())
	  return 0L;
	if (s == m_EditSpinDlgOriginalText)
	  return 0L;

	EDITSPIN_GetEditItem(iItem, iSubItem);
	if (iItem < 0) {
	  MessageBeep(MB_ICONEXCLAMATION);
			return 0L;
	}
	// DBCS Compliant
	// Oct 23, 97: skip leading Zero(s)
	int len = s.GetLength();
	if (len > 1) {
		int cpt=0;
		for (cpt=0; cpt<len; cpt++)
			if (s[cpt] != _T('0'))
				break;
		if (cpt == len)
			s = _T("0"); // Only zeros found
		else
		s = s.Right(len-cpt);		// start from first non-zero digit
	}
 
	// Emb Sept 25, 97: preliminary test at low level side
	CuCbfListViewItem* pItemData = (CuCbfListViewItem*)GetItemData(iItem);
	LPCOMPONENTINFO lpComponentInfo = &pItemData->m_componentInfo;
	AfxGetApp()->DoWaitCursor(1);
	try
	{
		BOOL bSuccess = VCBFllValidCount(lpComponentInfo, (LPTSTR)(LPCTSTR)s);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
			MessageBeep(MB_ICONEXCLAMATION);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlComponent::OnEditSpinDlgOK has caught exception: %s\n", e.m_strReason);
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
	
	// always display text as returned in the structure by VCBFllValidRename()
	SetItemText (iItem, iSubItem, (LPCTSTR)lpComponentInfo->szcount);
	return 0L;
}

LONG CuEditableListCtrlComponent::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	ASSERT (FALSE);   // NOT USED YET - NEED TO ADD CHECK BY LOW LEVEL

	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();

	// optimization: if text not changed or if text empty, behave as if cancelled
	if (s.IsEmpty())
		return 0L;
	if (s == m_EditNumberDlgOriginalText)
		return 0L;

	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrlComponent::OnComboDlgOK (UINT wParam, LONG lParam)
{
	ASSERT (FALSE);   // NOT USED YET - NEED TO ADD CHECK BY LOW LEVEL

	int iItem, iSubItem;
	CString s = COMBO_GetText();

	// optimization: if text not changed or if text empty, behave as if cancelled
	if (s.IsEmpty())
		return 0L;
	if (s == m_ComboDlgOriginalText)
		return 0L;

	COMBO_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

void CuEditableListCtrlComponent::OnRButtonDown(UINT nFlags, CPoint point) 
{
	OnOpenCellEditor(nFlags, point);
}

void CuEditableListCtrlComponent::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	OnOpenCellEditor(nFlags, point);
}

void CuEditableListCtrlComponent::OnOpenCellEditor(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = UD_MAXVAL;
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

	// Emb Sept 24, 97: modify for preliminary test at low level side
	CuCbfListViewItem* pItemData = (CuCbfListViewItem*)GetItemData(index);
	LPCOMPONENTINFO lpComponentInfo = &pItemData->m_componentInfo;
	BOOL bCanEditName  = VCBFllCanNameBeEdited(lpComponentInfo);
	BOOL bCanEditCount = VCBFllCanCountBeEdited(lpComponentInfo);

	HideProperty();
	switch (iColumnNumber)
	{
	case 1:
		if (bCanEditName)
			SetEditText (index, iColumnNumber, rCell);
		else
			MessageBeep(MB_ICONEXCLAMATION);
		break;
	case 2:
		if (bCanEditCount)
			SetEditSpin (index, iColumnNumber, rCell, iNumMin, iNumMax);
		else
			MessageBeep(MB_ICONEXCLAMATION);
		break;
	default:
		break;
	}
}

void CuEditableListCtrlComponent::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}

//
// virtual interface to afx_msg OnXyzDlgOK methods
//
LONG CuEditableListCtrlComponent::ManageEditDlgOK (UINT wParam, LONG lParam)
{
  return OnEditDlgOK(wParam, lParam);
}

LONG CuEditableListCtrlComponent::ManageEditSpinDlgOK (UINT wParam, LONG lParam)
{
  return OnEditSpinDlgOK(wParam, lParam);
}


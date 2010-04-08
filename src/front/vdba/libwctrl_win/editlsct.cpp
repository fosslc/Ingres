/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : Editlsct.cpp,  Implementation file
**    Project  : 
**    Author   : UK Sotheavut 
**    Purpose  : Owner draw LISTCTRL to provide an editable fields (edit box, combobox 
**               and spin control
**
** History:
**
** xx-Jan-1996 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 08-Nov-2000 (uk$so01)
**    Create library ctrltool.lib for more resuable of this control
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Clean up the members when the window is destroyed but the object
**    is still alive.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vcbf uses libwctrl.lib).
** 23-Mar-2005 (komve01)
**    Issue# 13919436, Bug#113774. 
**    Changed the return type of HideProperty function to return false,
**    incase of any errors from the down stream.
** 17-May-2005 (komve01)
**    Issue# 13864404, Bug#114371. 
**    Added a parameter for making the list control a simple list control.
**    The default behavior is the same as earlier, but for a special case
**    like VCDA (Main Parameters Tab), we do not want the default behavior
**    rather we would just like to have it as a simple list control.
**    Added SetSimpleList function to make the list a simple list control.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "editlsct.h"

#include "calsctrl.h"
#include "layeddlg.h"    // Edit box (Text)
#include "layspin.h"     // Edit and Spin (Numeric)
#include "combodlg.h"    // Combobox


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuLayoutSpinDlg*  CuEditableListCtrl::GetEditNumberDlg(){return (CuLayoutSpinDlg*)m_EditNumberDlgPtr;}
CuLayoutSpinDlg*  CuEditableListCtrl::GetEditSpinDlg(){return (CuLayoutSpinDlg*)m_EditSpinDlgPtr;}
CuLayoutEditDlg*  CuEditableListCtrl::GetEditDlg(){return (CuLayoutEditDlg*)m_EditDlgPtr;}
CuLayoutComboDlg* CuEditableListCtrl::GetComboDlg(){return (CuLayoutComboDlg*)m_ComboDlgPtr;}
CuLayoutSpinDlg*  CuEditableListCtrl::GetEditIntegerDlg(){return (CuLayoutSpinDlg*)m_EditIntegerDlgPtr;}
CuLayoutSpinDlg*  CuEditableListCtrl::GetEditIntegerSpinDlg(){return (CuLayoutSpinDlg*)m_EditIntegerSpinDlgPtr;}

//
// COMBOBOX
// ************************************************************************************************
CComboBox* CuEditableListCtrl::GetComboBox ()
{
	return (CComboBox*)((CuLayoutComboDlg*)m_ComboDlgPtr)->GetComboBox();
}

CComboBox* CuEditableListCtrl::COMBO_GetComboBox()
{
	return (CComboBox*)GetComboDlg()->GetComboBox();
}

void CuEditableListCtrl::COMBO_SetEditableMode(BOOL bEditable)
{
	CuLayoutComboDlg* pComboDlg = GetComboDlg();
	pComboDlg->SetMode(bEditable);
}

BOOL CuEditableListCtrl::COMBO_IsEditableMode()
{
	CuLayoutComboDlg* pComboDlg = GetComboDlg();
	return pComboDlg->IsEditableMode();
}



CString CuEditableListCtrl::COMBO_GetText ()
{
	CuLayoutComboDlg* pComboDlg = GetComboDlg();
	ASSERT(pComboDlg);
	if (!pComboDlg)
		return _T("");
	return pComboDlg->GetText();
}

void CuEditableListCtrl::COMBO_GetText (CString& strText)
{
	CuLayoutComboDlg* pComboDlg = GetComboDlg();
	ASSERT(pComboDlg);
	if (!pComboDlg)
		return;
	pComboDlg->GetText (strText);
}

void CuEditableListCtrl::COMBO_GetEditItem(int& iItem, int& iSubItem)
{
	iItem = -1;
	iSubItem = -1;
	CuLayoutComboDlg* pComboDlg = GetComboDlg();
	ASSERT(pComboDlg);
	if (!pComboDlg)
		return;
	pComboDlg->GetEditItem(iItem, iSubItem);
}


//
// EDIT
// ************************************************************************************************
CString CuEditableListCtrl::EDIT_GetText ()
{
	CuLayoutEditDlg*  pEditDlg = GetEditDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return _T("");
	return pEditDlg->GetText();
}

void CuEditableListCtrl::EDIT_GetText (CString& strText)
{
	CuLayoutEditDlg*  pEditDlg = GetEditDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	strText = pEditDlg->GetText();

}

void CuEditableListCtrl::EDIT_GetEditItem(int& iItem, int& iSubItem)
{
	CuLayoutEditDlg*  pEditDlg = GetEditDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	iItem = -1;
	iSubItem = -1;
	pEditDlg->GetEditItem(iItem, iSubItem);
}

CEdit* CuEditableListCtrl::EDIT_GetEditCtrl()
{
	CuLayoutEditDlg*  pEditDlg = GetEditDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return NULL;
	return (CEdit*)pEditDlg->GetDlgItem(IDC_RCT_EDIT1);
}

void CuEditableListCtrl::EDIT_SetExpressionDialog(BOOL bExpressionDld)
{
	CuLayoutEditDlg*  pEditDlg = GetEditDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->SetExpressionDialog(bExpressionDld);
}

//
// EDITNUMBER
// ************************************************************************************************
CString CuEditableListCtrl::EDITNUMBER_GetText ()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditNumberDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return _T("");
	return pEditDlg->GetText();
}

void CuEditableListCtrl::EDITNUMBER_GetText (CString& strText)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditNumberDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	strText = pEditDlg->GetText();

}

void CuEditableListCtrl::EDITNUMBER_GetEditItem(int& iItem, int& iSubItem)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditNumberDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	iItem = -1;
	iSubItem = -1;
	pEditDlg->GetEditItem(iItem, iSubItem);
}

CEdit* CuEditableListCtrl::EDITNUMBER_GetEditCtrl()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditNumberDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return NULL;
	return (CEdit*)pEditDlg->GetDlgItem(IDC_RCT_EDIT1);
}

void CuEditableListCtrl::EDITNUMBER_SetSpecialParse(BOOL bSet)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditNumberDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->SetSpecialParse(bSet);
}

//
// EDITSPIN
// ************************************************************************************************
CString CuEditableListCtrl::EDITSPIN_GetText ()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return _T("");
	return pEditDlg->GetText();
}

void CuEditableListCtrl::EDITSPIN_GetText (CString& strText)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	strText = pEditDlg->GetText();

}

void CuEditableListCtrl::EDITSPIN_GetEditItem(int& iItem, int& iSubItem)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	iItem = -1;
	iSubItem = -1;
	pEditDlg->GetEditItem(iItem, iSubItem);
}

CEdit* CuEditableListCtrl::EDITSPIN_GetEditCtrl()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return NULL;
	return (CEdit*)pEditDlg->GetDlgItem(IDC_RCT_EDIT1);
}

void CuEditableListCtrl::EDITSPIN_SetRange (int  Min, int  Max)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->SetRange (Min, Max);
}

void CuEditableListCtrl::EDITSPIN_GetRange (int& Min, int& Max)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->GetRange (Min, Max);
}

//
// EDITINTEGER
// ************************************************************************************************
CString CuEditableListCtrl::EDITINTEGER_GetText ()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return _T("");
	return pEditDlg->GetText();
}

void CuEditableListCtrl::EDITINTEGER_GetText (CString& strText)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	strText = pEditDlg->GetText();

}

void CuEditableListCtrl::EDITINTEGER_GetEditItem(int& iItem, int& iSubItem)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	iItem = -1;
	iSubItem = -1;
	pEditDlg->GetEditItem(iItem, iSubItem);
}

CEdit* CuEditableListCtrl::EDITINTEGER_GetEditCtrl()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return NULL;
	return (CEdit*)pEditDlg->GetDlgItem(IDC_RCT_EDIT1);
}

//
// EDITINTEGERSPIN
// ************************************************************************************************
CString CuEditableListCtrl::EDITINTEGERSPIN_GetText ()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return _T("");
	return pEditDlg->GetText();
}

void CuEditableListCtrl::EDITINTEGERSPIN_GetText (CString& strText)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	strText = pEditDlg->GetText();

}

void CuEditableListCtrl::EDITINTEGERSPIN_GetEditItem(int& iItem, int& iSubItem)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	iItem = -1;
	iSubItem = -1;
	pEditDlg->GetEditItem(iItem, iSubItem);
}

CEdit* CuEditableListCtrl::EDITINTEGERSPIN_GetEditCtrl()
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return NULL;
	return (CEdit*)pEditDlg->GetDlgItem(IDC_RCT_EDIT1);
}

void CuEditableListCtrl::EDITINTEGERSPIN_SetRange (int  Min, int  Max)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->SetRange (Min, Max);
}

void CuEditableListCtrl::EDITINTEGERSPIN_GetRange (int& Min, int& Max)
{
	CuLayoutSpinDlg*  pEditDlg = GetEditIntegerSpinDlg();
	ASSERT (pEditDlg);
	if (!pEditDlg)
		return;
	pEditDlg->GetRange (Min, Max);
}


CuEditableListCtrl::CuEditableListCtrl()
{
	m_EditDlgPtr            = NULL;
	m_EditNumberDlgPtr      = NULL;
	m_EditSpinDlgPtr        = NULL;
	m_ComboDlgPtr           = NULL;
	m_EditIntegerDlgPtr     = NULL;
	m_EditIntegerSpinDlgPtr = NULL;

	// text when entering edition mode
	// (used for optimization in OnEditXyzDlgCancel() )
	m_ComboDlgOriginalText           = _T("");;
	m_EditDlgOriginalText            = _T("");;
	m_EditNumberDlgOriginalText      = _T("");;
	m_EditSpinDlgOriginalText        = _T("");;
	m_EditIntegerDlgOriginalText     = _T("");
	m_EditIntegerSpinDlgOriginalText = _T("");

	SetFullRowSelected (FALSE);
	m_bCallingCreate = FALSE;
	m_bSimpleList = FALSE;
}

CuEditableListCtrl::~CuEditableListCtrl()
{
}

void CuEditableListCtrl::SetSimpleList(BOOL bSimpleList)
{
	m_bSimpleList = bSimpleList;
}

BOOL CuEditableListCtrl::HideProperty(BOOL bValidate)
{
	//
	BOOL bRetVal = TRUE;
	// Edit Cell
	if (m_EditDlgPtr && m_EditDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageEditDlgOK(0, 0))        // Use Virtuality!
				bRetVal = FALSE;
		m_EditDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}
	//
	// Edit Numeric
	if (m_EditNumberDlgPtr && m_EditNumberDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageEditNumberDlgOK(0, 0))  // Use Virtuality!
				bRetVal = FALSE;
		m_EditNumberDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}
	//
	// Edit Numeric (spin)
	if (m_EditSpinDlgPtr && m_EditSpinDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageEditSpinDlgOK(0, 0))    // Use Virtuality!
				bRetVal = FALSE;
		m_EditSpinDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}
	//
	// ComboBox
	if (m_ComboDlgPtr && m_ComboDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageComboDlgOK(0, 0))       // Use Virtuality!
				bRetVal = FALSE;
		m_ComboDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}
	//
	// Edit Integer
	if (m_EditIntegerDlgPtr && m_EditIntegerDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageEditIntegerDlgOK(0, 0))  // Use Virtuality!
				bRetVal = FALSE;
		m_EditIntegerDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}
	//
	// Edit Integer Spin
	if (m_EditIntegerSpinDlgPtr && m_EditIntegerSpinDlgPtr->IsWindowVisible())
	{
		if (bValidate)
			if(ManageEditIntegerSpinDlgOK(0, 0))  // Use Virtuality!
				bRetVal = FALSE;
		m_EditIntegerSpinDlgPtr->ShowWindow(SW_HIDE); 
		return bRetVal;
	}

	if (m_EditDlgPtr) m_EditDlgPtr->ShowWindow (SW_HIDE);
	if (m_EditNumberDlgPtr) m_EditDlgPtr->ShowWindow (SW_HIDE);
	if (m_EditSpinDlgPtr) m_EditSpinDlgPtr->ShowWindow (SW_HIDE);
	if (m_ComboDlgPtr) m_ComboDlgPtr->ShowWindow (SW_HIDE);
	if (m_EditIntegerDlgPtr) m_EditIntegerDlgPtr->ShowWindow (SW_HIDE);
	if (m_EditIntegerSpinDlgPtr) m_EditIntegerSpinDlgPtr->ShowWindow (SW_HIDE);

	return bRetVal;
}


void CuEditableListCtrl::SetEditText (int iIndex, int iSubItem, CRect rCell, LPCTSTR lpszText, BOOL bReadOnly)
{
	CuLayoutEditDlg* pEditDlgPtr = (CuLayoutEditDlg*)m_EditDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (lpszText)
		lpszItem = lpszText;
	if (!pEditDlgPtr)
		return;
	pEditDlgPtr->SetReadOnly (bReadOnly);
	if (IsWindow (pEditDlgPtr->m_hWnd) && pEditDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pEditDlgPtr->GetEditItem (iMain, iSub);
		pEditDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pEditDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pEditDlgPtr->MoveWindow (rCell);
	pEditDlgPtr->ShowWindow (SW_SHOW);
	m_EditDlgOriginalText = lpszItem;
}


void CuEditableListCtrl::SetEditNumber (int iIndex, int iSubItem, CRect rCell, BOOL bReadOnly)
{
	CuLayoutSpinDlg* pEditNumberDlgPtr = (CuLayoutSpinDlg*)m_EditNumberDlgPtr;
	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (!pEditNumberDlgPtr)
		return;
	pEditNumberDlgPtr->SetReadOnly (bReadOnly);
	if (IsWindow (pEditNumberDlgPtr->m_hWnd) && pEditNumberDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pEditNumberDlgPtr->GetEditItem (iMain, iSub);
		pEditNumberDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pEditNumberDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pEditNumberDlgPtr->MoveWindow (rCell);
	pEditNumberDlgPtr->ShowWindow (SW_SHOW);
	m_EditNumberDlgOriginalText = lpszItem;
}


void CuEditableListCtrl::SetEditSpin (int iIndex, int iSubItem, CRect rCell, int Min, int Max)
{
	CuLayoutSpinDlg* pEditSpinDlgPtr = (CuLayoutSpinDlg*)m_EditSpinDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (!pEditSpinDlgPtr)
		return;
	if (IsWindow (pEditSpinDlgPtr->m_hWnd) && pEditSpinDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pEditSpinDlgPtr->GetEditItem (iMain, iSub);
		pEditSpinDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pEditSpinDlgPtr->SetRange (Min, Max);
	pEditSpinDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pEditSpinDlgPtr->MoveWindow (rCell);
	pEditSpinDlgPtr->ShowWindow (SW_SHOW);
	m_EditSpinDlgOriginalText = lpszItem;
}

void CuEditableListCtrl::SetComboBox   (int iIndex, int iSubItem, CRect rCell, LPCTSTR lpszText)
{
	CuLayoutComboDlg* pComboDlgPtr = (CuLayoutComboDlg*)m_ComboDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (lpszText)
		lpszItem = lpszText;
	if (!pComboDlgPtr)
		return;

	if (IsWindow (pComboDlgPtr->m_hWnd) && pComboDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pComboDlgPtr->GetEditItem (iMain, iSub);
		pComboDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pComboDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pComboDlgPtr->MoveWindow (rCell);
	pComboDlgPtr->ShowWindow (SW_SHOW);
	m_ComboDlgOriginalText = lpszItem;
}

void CuEditableListCtrl::SetComboBox   (int iIndex, int iSubItem, CRect rCell, int iSelect)
{
	CuLayoutComboDlg* pComboDlgPtr = (CuLayoutComboDlg*)m_ComboDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	if (!pComboDlgPtr)
		return;
	if (IsWindow (pComboDlgPtr->m_hWnd) && pComboDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pComboDlgPtr->GetEditItem (iMain, iSub);
		pComboDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	CComboBox* pCombo = GetComboBox();
	pCombo->SetCurSel (iSelect);
	pComboDlgPtr->MoveWindow (rCell);
	pComboDlgPtr->ShowWindow (SW_SHOW);
	m_ComboDlgOriginalText = _T("???");     // ???
}

void CuEditableListCtrl::SetEditInteger (int iIndex, int iSubItem, CRect rCell, BOOL bReadOnly)
{
	CuLayoutSpinDlg* pEditIntegerDlgPtr = (CuLayoutSpinDlg*)m_EditIntegerDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (!pEditIntegerDlgPtr)
		return;
	pEditIntegerDlgPtr->SetReadOnly (bReadOnly);
	if (IsWindow (pEditIntegerDlgPtr->m_hWnd) && pEditIntegerDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pEditIntegerDlgPtr->GetEditItem (iMain, iSub);
		pEditIntegerDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pEditIntegerDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pEditIntegerDlgPtr->MoveWindow (rCell);
	pEditIntegerDlgPtr->ShowWindow (SW_SHOW);
	m_EditIntegerDlgOriginalText = lpszItem;
}


void CuEditableListCtrl::SetEditIntegerSpin (int iIndex, int iSubItem, CRect rCell, int Min, int Max, BOOL bReadOnly)
{
	CuLayoutSpinDlg* pEditIntegerSpinDlgPtr = (CuLayoutSpinDlg*)m_EditIntegerSpinDlgPtr;

	CString strItem= GetItemText (iIndex, iSubItem);
	LPCTSTR lpszItem = strItem.IsEmpty()? NULL: (LPCTSTR)strItem;
	if (!pEditIntegerSpinDlgPtr)
		return;
	if (IsWindow (pEditIntegerSpinDlgPtr->m_hWnd) && pEditIntegerSpinDlgPtr->IsWindowVisible())
	{
		int iMain, iSub;
		pEditIntegerSpinDlgPtr->GetEditItem (iMain, iSub);
		pEditIntegerSpinDlgPtr->SetFocus(); 
		if (iIndex == iMain && iSubItem == iSub)
			return;
	}
	HideProperty();
	pEditIntegerSpinDlgPtr->SetRange (Min, Max);
	pEditIntegerSpinDlgPtr->SetData (lpszItem, iIndex, iSubItem);
	pEditIntegerSpinDlgPtr->MoveWindow (rCell);
	pEditIntegerSpinDlgPtr->ShowWindow (SW_SHOW);
	m_EditIntegerSpinDlgOriginalText = lpszItem;
}


BEGIN_MESSAGE_MAP(CuEditableListCtrl, CuListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrl)
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCRBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,        OnEditDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,            OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL,  OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,      OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL,    OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,        OnEditSpinDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_CANCEL,       OnComboDlgCancel)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK,           OnComboDlgOK)
	//
	// Special Negative Integer Management
	ON_MESSAGE (WM_LAYOUTEDITINTEGERDLG_CANCEL,    OnEditIntegerDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITINTEGERDLG_OK,        OnEditIntegerDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITINTEGERSPINDLG_CANCEL,OnEditIntegerSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITINTEGERSPINDLG_OK,    OnEditIntegerSpinDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrl message handlers
	
LONG CuEditableListCtrl::OnEditDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutEditDlg* EditDlgPtr = (CuLayoutEditDlg*)m_EditDlgPtr;

	int iItem, iSubItem;
	CString s = EditDlgPtr->GetText();
	EditDlgPtr->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrl::OnEditDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrl::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutSpinDlg* pEditSpinDlg = GetEditSpinDlg();

	int iItem, iSubItem;
	int iMin, iMax;
	CString s = pEditSpinDlg->GetText();
	pEditSpinDlg->GetRange (iMin, iMax);

	if (s.IsEmpty() || _ttoi (s) < iMin)
	{
		s.Format (_T("%d"), iMin);
	}
	else
	if (s.IsEmpty() || _ttoi (s) > iMax)
	{
		s.Format (_T("%d"), iMax);
	}

	pEditSpinDlg->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}



LONG CuEditableListCtrl::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutSpinDlg* pEditNumberDlg = GetEditNumberDlg();

	int iItem, iSubItem;
	CString s = pEditNumberDlg->GetText();

	pEditNumberDlg->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrl::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrl::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrl::OnComboDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutComboDlg* pComboDlgPtr = GetComboDlg();

	int iItem, iSubItem;
	CString s = pComboDlgPtr->GetText();
	pComboDlgPtr->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrl::OnComboDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

void CuEditableListCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
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
	if(!m_bSimpleList)
	{
	switch (iColumnNumber)
	{
	case 1:
		SetEditText (index, iColumnNumber, rCell);
		break;
	case 2:
		SetEditSpin (index, iColumnNumber, rCell, iNumMin, iNumMax);
		break;
	default:
		break;
	}
	}
}

void CuEditableListCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	UINT uFlag = 0;
	int  iIndex;

	HideProperty();
	iIndex = CListCtrl::HitTest (point, &uFlag);
	CRect r;
	if (iIndex == -1)
		return;
	GetItemRect (iIndex, r, LVIR_BOUNDS);
	if (r.PtInRect (point))
		CuListCtrl::OnLButtonDown(nFlags, point);
}



void CuEditableListCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CuListCtrl::OnKillFocus(pNewWnd);
}

BOOL CuEditableListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	int   iItem, iSubItem;
	CRect rect, rCell;
	CuLayoutSpinDlg*  pEditNumberDlgPtr = (CuLayoutSpinDlg*)m_EditNumberDlgPtr;
	CuLayoutSpinDlg*  pEditSpinDlgPtr = (CuLayoutSpinDlg*)m_EditSpinDlgPtr;
	CuLayoutEditDlg*  pEditDlgPtr = (CuLayoutEditDlg*)m_EditDlgPtr;
	CuLayoutComboDlg* pComboDlgPtr = (CuLayoutComboDlg*)m_ComboDlgPtr;
	CuLayoutSpinDlg*  pEditIntegerDlgPtr = (CuLayoutSpinDlg*)m_EditIntegerDlgPtr;
	CuLayoutSpinDlg*  pEditIntegerSpinDlgPtr = (CuLayoutSpinDlg*)m_EditIntegerSpinDlgPtr;

	switch (message)
	{
	case WM_DRAWITEM:
		ASSERT(pLResult == NULL); 
		if (!(((LPDRAWITEMSTRUCT)lParam)->itemAction && ODA_DRAWENTIRE))
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			break;
		}
		else
		if (pEditDlgPtr && pEditDlgPtr->m_hWnd && pEditDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pEditDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pEditDlgPtr->MoveWindow (rCell);
		}
		else
		if (pEditNumberDlgPtr && pEditNumberDlgPtr->m_hWnd && pEditNumberDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pEditNumberDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pEditNumberDlgPtr->MoveWindow (rCell);
		}
		else
		if (pEditSpinDlgPtr && pEditSpinDlgPtr->m_hWnd && pEditSpinDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pEditSpinDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pEditSpinDlgPtr->MoveWindow (rCell);
		}
		else
		if (pComboDlgPtr && pComboDlgPtr->m_hWnd && pComboDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pComboDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pComboDlgPtr->MoveWindow (rCell);
		}
		else
		if (pEditIntegerSpinDlgPtr && pEditIntegerSpinDlgPtr->m_hWnd && pEditIntegerSpinDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pEditIntegerSpinDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pEditIntegerSpinDlgPtr->MoveWindow (rCell);
		}
		else
		if (pEditIntegerDlgPtr && pEditIntegerDlgPtr->m_hWnd && pEditIntegerDlgPtr->IsWindowVisible())
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			pEditIntegerDlgPtr->GetEditItem(iItem, iSubItem);
			GetItemRect (iItem, rect, LVIR_BOUNDS);
			GetCellRect (rect, iSubItem, rCell);
			rCell.top    -= 2;
			rCell.bottom += 2;
			pEditIntegerDlgPtr->MoveWindow (rCell);
		}
		else
		{
			CuListCtrl::PredrawItem ((LPDRAWITEMSTRUCT)lParam);
			break;
		}
		break;
	default:
		return CuListCtrl::OnChildNotify(message, wParam, lParam, pLResult);
	}
	return TRUE;
}

BOOL CuEditableListCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	m_bCallingCreate = TRUE;
	return CuListCtrl::Create(dwStyle, rect, pParentWnd, nID);
}


int CuEditableListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CuListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (m_bCallingCreate)
		CreateChildren();
	return 0;
}

void CuEditableListCtrl::OnDestroy() 
{
	CuListCtrl::OnDestroy();
	if (m_EditDlgPtr)
		m_EditDlgPtr->DestroyWindow();
	if (m_EditSpinDlgPtr)
		m_EditSpinDlgPtr->DestroyWindow();
	if (m_EditNumberDlgPtr)
		m_EditNumberDlgPtr->DestroyWindow();
	if (m_ComboDlgPtr)
		m_ComboDlgPtr->DestroyWindow();
	if (m_EditIntegerDlgPtr)
		m_EditIntegerDlgPtr->DestroyWindow();
	if (m_EditIntegerSpinDlgPtr)
		m_EditIntegerSpinDlgPtr->DestroyWindow();
	m_EditDlgPtr = NULL;
	m_EditSpinDlgPtr = NULL;
	m_EditNumberDlgPtr = NULL;
	m_ComboDlgPtr = NULL;
	m_EditIntegerDlgPtr = NULL;
	m_EditIntegerSpinDlgPtr = NULL;

}

void CuEditableListCtrl::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
	HideProperty(TRUE);
	CuListCtrl::OnNcLButtonDown(nHitTest, point);
}

void CuEditableListCtrl::OnNcRButtonDown(UINT nHitTest, CPoint point) 
{
	CuListCtrl::OnNcRButtonDown(nHitTest, point);
}


void CuEditableListCtrl::CreateChildren()
{
	if (!IsWindow(this->m_hWnd))
		return;
	LONG style = GetWindowLong (this->m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN|LVS_SINGLESEL|LVS_SHOWSELALWAYS;
	SetWindowLong (this->m_hWnd, GWL_STYLE, style);

	//
	// Edit the text
	if (!m_EditDlgPtr)
	{
		CuLayoutEditDlg* pDlg = new CuLayoutEditDlg (this);
		pDlg->Create (IDD_LAYOUTCTRLEDIT, this);
		pDlg->SetExpressionDialog(FALSE);
		pDlg->SetReadOnly(FALSE);

		m_EditDlgPtr = (CDialog*)pDlg;
	}
	//
	// Edit numeric (spin)
	if (!m_EditSpinDlgPtr)
	{
		CuLayoutSpinDlg* pDlg = new CuLayoutSpinDlg (this);
		pDlg->Create (IDD_LAYOUTCTRLSPIN, this);
		pDlg->SetReadOnly(FALSE);

		m_EditSpinDlgPtr = pDlg;
	}
	//
	// Edit numeric (do not use spin)
	if (!m_EditNumberDlgPtr)
	{
		CuLayoutSpinDlg* pDlg = new CuLayoutSpinDlg (this);
		pDlg->Create (IDD_LAYOUTCTRLSPIN, this);
		pDlg->SetUseSpin (FALSE);
		pDlg->SetReadOnly(FALSE);

		m_EditNumberDlgPtr = pDlg;
	}
	//
	// ComboBox
	if (!m_ComboDlgPtr)
	{
		CuLayoutComboDlg* pDlg = new CuLayoutComboDlg (this);
		pDlg->Create (IDD_LAYOUTCTRLCOMBO, this);
		
		m_ComboDlgPtr = pDlg;
	}
	//
	// Integer Edit Box
	if (!m_EditIntegerDlgPtr)
	{
		CuLayoutSpinDlg* pDlg = new CuLayoutSpinDlg (this);
		pDlg->SetUseSpin (FALSE);
		pDlg->SetSpecialParse (TRUE);
		pDlg->SetParseStyle (PES_INTEGER);
		pDlg->Create (IDD_LAYOUTCTRLSPIN, this);
		pDlg->SetReadOnly(FALSE);

		m_EditIntegerDlgPtr = pDlg;
	}
	//
	// Integer Edit Box (Use Spin)
	if (!m_EditIntegerSpinDlgPtr)
	{
		CuLayoutSpinDlg* pDlg = new CuLayoutSpinDlg (this);
		pDlg->SetUseSpin (TRUE);
		pDlg->SetSpecialParse (TRUE);
		pDlg->SetParseStyle (PES_INTEGER);
		pDlg->Create (IDD_LAYOUTCTRLSPIN, this);
		pDlg->SetReadOnly(FALSE);

		m_EditIntegerSpinDlgPtr = pDlg;
	}
}


void CuEditableListCtrl::PreSubclassWindow() 
{
	//
	// When user calls Create explicitely, just do nothing.
	// Otherwise it calls CreateChildren(), this is usually done when user
	// calls SubclassDlgItem() from the OnInitDialog() 
	if (m_bCallingCreate)
		return;
	CreateChildren();
}




BOOL CuEditableListCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= LVS_SHOWSELALWAYS|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
	return CuListCtrl::PreCreateWindow(cs);
}


void CuEditableListCtrl::OnPaint() 
{
	CListCtrl::OnPaint();
}


void CuEditableListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
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
	case 1:
		SetEditText (index, iColumnNumber, rCell);
		break;
	case 2:
		SetEditSpin (index, iColumnNumber, rCell, iNumMin, iNumMax);
		break;
	default:
		break;
	}
}

void CuEditableListCtrl::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}


LONG CuEditableListCtrl::OnEditIntegerDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutSpinDlg* pEditIntegerDlgPtr = (CuLayoutSpinDlg*)m_EditIntegerDlgPtr;

	int iItem, iSubItem;
	CString s = pEditIntegerDlgPtr->GetText();

	pEditIntegerDlgPtr->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}

LONG CuEditableListCtrl::OnEditIntegerDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrl::OnEditIntegerSpinDlgOK (UINT wParam, LONG lParam)
{
	CuLayoutSpinDlg* pEditIntegerSpinDlg = GetEditIntegerSpinDlg();

	int iItem, iSubItem;
	int iMin, iMax;
	CString s = pEditIntegerSpinDlg->GetText();
	if (s.IsEmpty())
	{
		pEditIntegerSpinDlg->GetRange (iMin, iMax);
		s.Format (_T("%d"), iMin);
	}
	pEditIntegerSpinDlg->GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	SetItemText (iItem, iSubItem, (LPCTSTR)s);
	return 0L;
}
	
LONG CuEditableListCtrl::OnEditIntegerSpinDlgCancel(UINT wParam, LONG lParam)
{
	return 0L;
}

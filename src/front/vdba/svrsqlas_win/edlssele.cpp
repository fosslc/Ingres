/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : edlssele.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 3) 
**               Editable List control to provide the sort order (Order by clause)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "edlssele.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CaSqlWizardDataField::CaSqlWizardDataField(const CaSqlWizardDataField& a)
{
	duplicate(a);
}

CaSqlWizardDataField CaSqlWizardDataField::operator = (const CaSqlWizardDataField & b)
{
	if (this == &b)
		ASSERT (FALSE);
	duplicate (b);
	return *this;
}

void CaSqlWizardDataField::SetTable(LPCTSTR lpszTable, LPCTSTR lpszTableOwner, int nObjectType)
{
	m_strObjectOwner = lpszTableOwner;
	m_strObjectName = lpszTable;
	m_nObjType = nObjectType;
}

void CaSqlWizardDataField::duplicate(const CaSqlWizardDataField& p)
{
	m_iType          = p.m_iType;
	m_strColumn      = p.m_strColumn;
	m_strObjectName  = p.m_strObjectName;
	m_strObjectOwner = p.m_strObjectOwner;
	m_nObjType       = p.m_nObjType;
}

CString CaSqlWizardDataField::FormatString4Display()
{
	CString csTempo;
	if (!m_strObjectOwner.IsEmpty())
	{
		csTempo =  INGRESII_llQuoteIfNeeded(m_strObjectOwner);
		csTempo += _T(".");
		csTempo += INGRESII_llQuoteIfNeeded(m_strObjectName),
		csTempo += _T(".");
		csTempo += m_strColumn;
	}
	else
	{
		csTempo = INGRESII_llQuoteIfNeeded(m_strObjectName),
		csTempo += _T(".");
		csTempo += m_strColumn;
	}

	return csTempo;
}


CString CaSqlWizardDataField::FormatString4SQL()
{
	CString csTempo;
	if (!m_strObjectOwner.IsEmpty())
	{
		csTempo =  INGRESII_llQuoteIfNeeded(m_strObjectOwner);
		csTempo += _T(".");
		csTempo += INGRESII_llQuoteIfNeeded(m_strObjectName),
		csTempo += _T(".");
		csTempo += INGRESII_llQuoteIfNeeded(m_strColumn);
	}
	else
	{
		csTempo = INGRESII_llQuoteIfNeeded(m_strObjectName);
		csTempo += _T(".");
		csTempo += INGRESII_llQuoteIfNeeded(m_strColumn);
	}
	return csTempo;
}



CuEditableListCtrlSqlWizardSelectOrderColumn::CuEditableListCtrlSqlWizardSelectOrderColumn()
{
	const int nSize = 3;
	CString strItem;
	UINT nIDS[nSize] = {IDS_SORT_NONE, IDS_SORT_ASC, IDS_SORT_DESC};
	for (int i=0; i<nSize; i++)
	{
		strItem.LoadString(nIDS[i]);
		lstrcpy (m_strArraySort[i], strItem);
		m_nArraySort[i] = i;
	}
}

CuEditableListCtrlSqlWizardSelectOrderColumn::~CuEditableListCtrlSqlWizardSelectOrderColumn()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlSqlWizardSelectOrderColumn, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlSqlWizardSelectOrderColumn)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,  OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK, OnComboDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlSqlWizardSelectOrderColumn message handlers

void CuEditableListCtrlSqlWizardSelectOrderColumn::OnDestroy() 
{
	CaSqlWizardDataFieldOrder* pItemData = NULL;
	int i, nCount = GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pItemData = (CaSqlWizardDataFieldOrder*)GetItemData(i);
		if (pItemData)
			delete pItemData;
		SetItemData(i, (LPARAM)0);
	}
	CuEditableListCtrl::OnDestroy();
}

void CuEditableListCtrlSqlWizardSelectOrderColumn::OnLButtonDblClk(UINT nFlags, CPoint point) 
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

void CuEditableListCtrlSqlWizardSelectOrderColumn::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CuEditableListCtrl::OnRButtonDblClk(nFlags, point);
}

void CuEditableListCtrlSqlWizardSelectOrderColumn::OnRButtonDown(UINT nFlags, CPoint point) 
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

void CuEditableListCtrlSqlWizardSelectOrderColumn::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	
	CuEditableListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


LONG CuEditableListCtrlSqlWizardSelectOrderColumn::OnComboDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = COMBO_GetText();
	COMBO_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
	{
		SetFocus();
		return 0L;
	}
	try
	{
		CaSqlWizardDataFieldOrder* pData = (CaSqlWizardDataFieldOrder*)GetItemData (iItem);
		pData->m_strSort = s;
		UpdateDisplay (iItem);
		GetParent()->SendMessage (WM_LAYOUTCOMBODLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		//
		// _T("Cannot change combo text.")
		AfxMessageBox (IDS_MSG_CHANGE_COMBO, MB_ICONEXCLAMATION|MB_OK);
	}

	SetFocus();
	return 0L;
}



LONG CuEditableListCtrlSqlWizardSelectOrderColumn::OnEditDlgOK (UINT wParam, LONG lParam)
{
	return 0L;
}


void CuEditableListCtrlSqlWizardSelectOrderColumn::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem < 1)
		return;

	CString strItem = GetItemText (iItem, iSubItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	CComboBox* pCombo = (CComboBox*)COMBO_GetComboBox();
	int iIndex = -1;

	switch (iSubItem)
	{
	case SORT:
		InitSortComboBox();
		SetComboBox (iItem, iSubItem, rcCell, (LPCTSTR)strItem);
		break;
	default:
		break;
	}
}

void CuEditableListCtrlSqlWizardSelectOrderColumn::InitSortComboBox()
{
	int i, iIndex;
	CComboBox* pCombo = COMBO_GetComboBox();
	pCombo->ResetContent();
	for (i=1; i<m_nSortItem; i++)
	{
		iIndex = pCombo->AddString (m_strArraySort[i]);
		pCombo->SetItemData (iIndex, (DWORD)m_nArraySort[i]);
	}
}


void CuEditableListCtrlSqlWizardSelectOrderColumn::GetSortOrder(CStringList& listColumn)
{
	try
	{
		CString strSortType;
		CString strSortOrder;
		CString strItem;
		int i, nCount = GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strSortOrder = GetItemText (i, COLUMN);
			strSortType  = GetItemText (i, SORT);
			//
			// Format: <owner.object.column asc|desc>
			strItem.Format (_T("%s %s"), (LPCTSTR)strSortOrder, (LPCTSTR)strSortType);
			listColumn.AddTail (strItem);
		}
	}
	catch (...)
	{
		// _T("Cannot get the sort order column.");
		AfxMessageBox (IDS_MSG_SORTORDER, MB_ICONEXCLAMATION|MB_OK);
	}
}


void CuEditableListCtrlSqlWizardSelectOrderColumn::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CaSqlWizardDataFieldOrder* pItemData = (CaSqlWizardDataFieldOrder*)GetItemData (pNMListView->iItem);
	if (pItemData)
		delete pItemData;
	*pResult = 0;
}


void CuEditableListCtrlSqlWizardSelectOrderColumn::FormatString4display (CaSqlWizardDataFieldOrder* pItemTemp, CString& csTempo)
{
	if(!pItemTemp)
		return;
	if (pItemTemp->m_bMultiTablesSelected &&
	    pItemTemp->m_iType == CaSqlWizardDataFieldOrder::COLUMN_NORMAL)
	{
		CString strOwner = pItemTemp->GetTableOwner();
		CString strName  = pItemTemp->GetTable();
		CString strCol   = pItemTemp->GetColumn();

		if (strOwner.FindOneOf(_T(".\"")) != -1)
			strOwner = INGRESII_llQuoteIfNeeded(pItemTemp->GetTableOwner());
		if (strName.FindOneOf(_T(".\"")) != -1)
			strName = INGRESII_llQuoteIfNeeded(pItemTemp->GetTable());
		if (strCol.FindOneOf(_T(".\"")) != -1)
			strCol = INGRESII_llQuoteIfNeeded(pItemTemp->GetColumn());

		if (!strOwner.IsEmpty())
		{
			csTempo  = strOwner;
			csTempo += _T(".");
		}
		csTempo += strName; 
		csTempo += _T(".");
		csTempo += strCol;
	}
	else
		csTempo = pItemTemp->GetColumn();
}

void CuEditableListCtrlSqlWizardSelectOrderColumn::UpdateDisplay (int iItem)
{
	int i, nCount = GetItemCount();
	CaSqlWizardDataFieldOrder* pItem = NULL;
	CString StrItemDisplay;

	if (iItem == -1)
	{
		for (i=0; i<nCount; i++)
		{
			StrItemDisplay = _T("");
			pItem = (CaSqlWizardDataFieldOrder*)GetItemData (i);

			FormatString4display(pItem , StrItemDisplay);

			SetItemText (i, COLUMN,   StrItemDisplay);
			SetItemText (i, SORT,     pItem->m_strSort);
		}
	}
	else
	{
		pItem = (CaSqlWizardDataFieldOrder*)GetItemData (iItem);

		FormatString4display(pItem , StrItemDisplay);

		SetItemText (iItem, COLUMN,   StrItemDisplay);
		SetItemText (iItem, SORT,     pItem->m_strSort);
	}
}


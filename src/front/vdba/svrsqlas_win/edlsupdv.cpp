/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edlsupdv.cpp, Implementation File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 3)
**               Editable List control to provide the update value
**
** History:
**
** xx-Jun-1998 (uk$so01)
**    Created
** 31-May-2001 (uk$so01)
**    SIR #104796 (Sql Assistant, put the single quote to surrounds the values
**    of the insert or update statement.
** 28-Jun-2001 (noifr01)
**    (sir 103694) when inserting a unicode constant, prefill the control
**    with U'' and preset the selection between the ' characters
** 18-Jul-2001 (noifr01)
**    (sir 103694) replaced U char with N for unicode constants
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "edlsupdv.h"
#include "sqlepsht.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuEditableListCtrlSqlWizardUpdateValue::CuEditableListCtrlSqlWizardUpdateValue()
{
}

CuEditableListCtrlSqlWizardUpdateValue::~CuEditableListCtrlSqlWizardUpdateValue()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlSqlWizardUpdateValue, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlSqlWizardUpdateValue)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,  OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITDLG_EXPR_ASSISTANT,  OnEditAssistant)
END_MESSAGE_MAP()


void CuEditableListCtrlSqlWizardUpdateValue::OnLButtonDblClk(UINT nFlags, CPoint point) 
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


void CuEditableListCtrlSqlWizardUpdateValue::OnRButtonDown(UINT nFlags, CPoint point) 
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


LONG CuEditableListCtrlSqlWizardUpdateValue::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
	{
		SetFocus();
		return 0L;
	}
	try
	{
		SetItemText (iItem, iSubItem, s);
		GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		//
		// _T("Cannot change edit text.")
		AfxMessageBox (IDS_MSG_CHANGE_EDIT, MB_ICONEXCLAMATION|MB_OK);
	}

	SetFocus();
	return 0L;
}

LONG CuEditableListCtrlSqlWizardUpdateValue::OnEditAssistant (UINT wParam, LONG lParam)
{
	CString strText = (LPCTSTR)lParam;
	CxDlgPropertySheetSqlExpressionWizard wizdlg;
	wizdlg.m_queryInfo = m_queryInfo;
	wizdlg.m_nFamilyID    = -1;
	wizdlg.m_nAggType     = IDC_RADIO1;
	wizdlg.m_nCompare     = IDC_RADIO1;
	wizdlg.m_nIn_notIn    = IDC_RADIO1;
	wizdlg.m_nSub_List    = IDC_RADIO1;

	//
	// It is table or view because we are updating data to table/view:
	CaDBObject* pNewObject = new CaDBObject(m_queryInfo.GetItem2(), m_queryInfo.GetItem2Owner());
	pNewObject->SetObjectID(m_queryInfo.GetObjectType());
	wizdlg.m_listObject.AddTail (pNewObject);

	if (wizdlg.DoModal() != IDCANCEL)
	{
		CString strStatement;
		CEdit*  edit1 = EDIT_GetEditCtrl();

		wizdlg.GetStatement (strStatement);
		if (!strText.IsEmpty() && (strText == _T("''") || strText == _T("N''")))
		{
			if (edit1 && IsWindow (edit1->m_hWnd))
			{
				edit1->SetFocus();
				edit1->SetWindowText (strStatement);
				edit1->SetSel(strStatement.GetLength(), strStatement.GetLength());
			}
		}
		else
		{
			if (edit1 && IsWindow (edit1->m_hWnd))
			{
				edit1->ReplaceSel (strStatement);
				edit1->SetFocus();
			}
		}
	}
	return 0;
}


void CuEditableListCtrlSqlWizardUpdateValue::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem < 1)
		return;
	EDIT_SetExpressionDialog ();
	CString strItem = GetItemText (iItem, iSubItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	BOOL bQuote = FALSE;
	int iIndex = -1;
	int nType = (int)GetItemData (iItem);
	int ipossel;

	switch (iSubItem)
	{
	case VALUE:
		switch (nType)
		{
		case INGTYPE_C:
		case INGTYPE_CHAR:
		case INGTYPE_TEXT:
		case INGTYPE_VARCHAR:
		case INGTYPE_LONGVARCHAR:
		case INGTYPE_DATE:
		case INGTYPE_BYTE:
		case INGTYPE_LONGBYTE:
		case INGTYPE_BYTEVAR:
			if (strItem.IsEmpty())
			{
				strItem = _T("''");
				bQuote  = TRUE;
				ipossel=1;
			}
			break;
		case INGTYPE_UNICODE_NCHR:
		case INGTYPE_UNICODE_NVCHR:
		case INGTYPE_UNICODE_LNVCHR:
			if (strItem.IsEmpty())
			{
				strItem = _T("N''");
				bQuote  = TRUE;
				ipossel=2;
			}
			break;
		}
		SetEditText (iItem, iSubItem, rcCell, (LPCTSTR)strItem);
		if (bQuote)
		{
			CEdit*  edit1 = (CEdit*)EDIT_GetEditCtrl();
			if (edit1 && IsWindow (edit1->m_hWnd))
			{
				edit1->SetFocus();
				edit1->SetSel(ipossel, ipossel);
			}
		}
		break;
	default:
		break;
	}
}

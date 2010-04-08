/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
/*/

/*
**    Source   : sqlwpsht.cpp, Implementation File
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select, Insert, Update, Delete) 
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 23-May-2000 (noifr01)
**    (bug 99242) cleanup for DBCS compliance
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (sqlassis.chm file)
**    Activate Help Button.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sqlwpsht.h"
#include "ingobdml.h" // Low level query objects


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CxDlgPropertySheetSqlWizard, CPropertySheet)

CxDlgPropertySheetSqlWizard::CxDlgPropertySheetSqlWizard(CWnd* pWndParent)
	 : CPropertySheet(_T("Sql Wizard") /*IDS_PROPSHT_CAPTION*/, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().
	m_psh.dwFlags |= PSH_HASHELP;
	m_pPointTopLeft = NULL;
	m_bSelectOnly = FALSE;
	m_nHelpLevel    = 0;

	if (!m_bSelectOnly)
		AddPage(&m_PageMain);
	else
		SetSelectOnlyMode();
	SetWizardMode();
}

CxDlgPropertySheetSqlWizard::CxDlgPropertySheetSqlWizard(BOOL bSelectOnly, CWnd* pWndParent)
{
	m_psh.dwFlags |= PSH_HASHELP;
	m_pPointTopLeft = NULL;
	m_bSelectOnly = bSelectOnly;
	m_nHelpLevel    = 0;

	if (!m_bSelectOnly)
		AddPage(&m_PageMain);
	else
		SetSelectOnlyMode();
	SetWizardMode();
}


CxDlgPropertySheetSqlWizard::~CxDlgPropertySheetSqlWizard()
{

}

void CxDlgPropertySheetSqlWizard::SetSelectOnlyMode()
{
	SetCategory(SQLW_SELECT);
	AddPage(&m_PageSelect1);
	AddPage(&m_PageSelect2);
	AddPage(&m_PageSelect3);
}

void CxDlgPropertySheetSqlWizard::GetStatement(CString& strStatement)
{
	strStatement = m_strStatement;
}

void CxDlgPropertySheetSqlWizard::SetStatement(LPCTSTR lpszStatement)
{
	m_strStatement = lpszStatement? lpszStatement: _T("");
}

void CxDlgPropertySheetSqlWizard::SetCategorySelect()
{
	int i, nPages = GetPageCount();
	for (i=1; i<nPages; i++)
	{
		RemovePage (1);
	}
	SetCategory(SQLW_SELECT);
	AddPage(&m_PageSelect1);
	AddPage(&m_PageSelect2);
	AddPage(&m_PageSelect3);
}

void CxDlgPropertySheetSqlWizard::SetCategoryInsert()
{
	int i, nPages = GetPageCount();
	for (i=1; i<nPages; i++)
	{
		RemovePage (1);
	}
	SetCategory(SQLW_INSERT);
	AddPage(&m_PageInsert1);
}

void CxDlgPropertySheetSqlWizard::SetCategoryInsert2(BOOL bManual)
{
	int i, nPages = GetPageCount();
	for (i=2; i<nPages; i++)
	{
		RemovePage (2);
	}
	if (bManual)
	{
		m_nInsertType = SQLW_INSERT_MANUAL;
		AddPage(&m_PageInsert2);
	}
	else
	{
		m_nInsertType = SQLW_INSERT_SUBSELECT;
		AddPage(&m_PageSelect1);
		AddPage(&m_PageSelect2);
		AddPage(&m_PageSelect3);
	}
}

void CxDlgPropertySheetSqlWizard::SetCategoryUpdate()
{
	int i, nPages = GetPageCount();
	for (i=1; i<nPages; i++)
	{
		RemovePage (1);
	}
	SetCategory(SQLW_UPDATE);
	AddPage(&m_PageUpdate1);
	AddPage(&m_PageUpdate2);
	AddPage(&m_PageUpdate3);
}

void CxDlgPropertySheetSqlWizard::SetCategoryDelete()
{
	int i, nPages = GetPageCount();
	for (i=1; i<nPages; i++)
	{
		RemovePage (1);
	}
	SetCategory(SQLW_DELETE);
	AddPage(&m_PageDelete1);
}

BOOL SQLW_ComboBoxTablesClean (CComboBox* pCombo)
{
	CaDBObject* pObj = NULL;
	if (!pCombo)
		return FALSE;
	if (!IsWindow (pCombo->m_hWnd))
		return FALSE;
	int i, nCount = pCombo->GetCount();
	for (i=0; i<nCount; i++)
	{
		pObj = (CaDBObject*)pCombo->GetItemData (i);
		if (pObj)
			delete pObj;
		pCombo->SetItemData (i, (DWORD)0);
	}
	pCombo->ResetContent();
	return TRUE;
}


BOOL SQLW_ComboBoxFillTables (CComboBox* pCombo, CaLLQueryInfo* pInfo)
{
	int idx = -1;
	CString strItem;
	CTypedPtrList<CObList, CaDBObject*> listObject;

	BOOL bOk = theApp.INGRESII_QueryObject (pInfo, listObject);
	if (!bOk)
		return FALSE;

	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(pos);
#if defined (_DISPLAY_OWNERxITEM)
		AfxFormatString2 (strItem, IDS_OWNERxITEM, (LPCTSTR)pObj->GetOwner(), (LPCTSTR)pObj->GetItem());
#else
		strItem = pObj->GetName();
#endif
		idx = pCombo->AddString (strItem);
		if (idx != CB_ERR)
			pCombo->SetItemData (idx, (DWORD)pObj);
		else
			delete pObj;
	}

	return TRUE;
}


BOOL SQLW_CuCheckListBoxTablesClean (CuListCtrlCheckBox* pListBox)
{
	CaDBObject* pObj = NULL;
	if (!pListBox)
		return FALSE;
	if (!IsWindow (pListBox->m_hWnd))
		return FALSE;
	int i, nCount = pListBox->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pObj = (CaDBObject*)pListBox->GetItemData (i);
		if (pObj)
			delete pObj;
		pListBox->SetItemData (i, (DWORD)0);
	}
	pListBox->DeleteAllItems();
	return TRUE;
}

BOOL SQLW_CuCheckListBoxFillTables (CuListCtrlCheckBox* pListBox, CaLLQueryInfo* pInfo)
{
	int idx = -1;
	CString strItem;
	CTypedPtrList<CObList, CaDBObject*> listObject;

	BOOL bOk = theApp.INGRESII_QueryObject (pInfo, listObject);
	if (!bOk)
		return FALSE;

	LVITEM lvi;
	memset (&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE|LVIF_STATE;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(1);
	lvi.iImage = 0;
	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(pos);
		lvi.iItem = pListBox->GetItemCount();
		lvi.iSubItem = 0;
		lvi.lParam = (LPARAM)pObj;
		lvi.pszText = (LPTSTR)(LPCTSTR)pObj->GetName();
		idx = pListBox->InsertItem (&lvi);

		if (idx != -1)
		{
			pListBox->SetItemText (idx, 1, pObj->GetOwner());
		}
		else
		{
			delete pObj;
		}
	}

	return TRUE;
}


BOOL SQLW_CuCheckListBoxColumnsClean (CuCheckListBox* pListBox)
{
	CaDBObject* pObj = NULL;
	if (!pListBox)
		return FALSE;
	if (!IsWindow (pListBox->m_hWnd))
		return FALSE;
	int i, nCount = pListBox->GetCount();
	for (i=0; i<nCount; i++)
	{
		pObj = (CaDBObject*)pListBox->GetItemData (i);
		if (pObj)
			delete pObj;
		pListBox->SetItemData (i, (DWORD)0);
	}
	pListBox->ResetContent();
	return TRUE;
}

BOOL SQLW_CuCheckListBoxFillColumns (CuCheckListBox* pListBox, CaLLQueryInfo* pInfo)
{
	int idx = -1;
	CTypedPtrList<CObList, CaDBObject*> listObject;

	BOOL bOk = theApp.INGRESII_QueryObject (pInfo, listObject);
	if (!bOk)
		return FALSE;

	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(pos);
		idx = pListBox->AddString ((LPCTSTR)pObj->GetName());
		if (idx != LB_ERR)
			pListBox->SetItemData (idx, (DWORD)pObj);
		else
			delete pObj;
	}
	return TRUE;
}


BOOL SQLW_ListBoxColumnsClean (CListBox* pListBox)
{
	CaDBObject* pObj = NULL;
	if (!pListBox)
		return FALSE;
	if (!IsWindow (pListBox->m_hWnd))
		return FALSE;
	int i, nCount = pListBox->GetCount();
	for (i=0; i<nCount; i++)
	{
		pObj = (CaDBObject*)pListBox->GetItemData (i);
		if (pObj)
			delete pObj;
		pListBox->SetItemData (i, (DWORD)0);
	}
	pListBox->ResetContent();
	return TRUE;
}

BOOL SQLW_ListBoxFillColumns (CListBox* pListBox, CaLLQueryInfo* pInfo)
{
	int idx = -1;
	CTypedPtrList<CObList, CaDBObject*> listObject;

	BOOL bOk = theApp.INGRESII_QueryObject (pInfo, listObject);
	if (!bOk)
		return FALSE;

	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = listObject.GetNext(pos);
		idx = pListBox->AddString ((LPCTSTR)pObj->GetName());
		if (idx != LB_ERR)
			pListBox->SetItemData (idx, (DWORD)pObj);
		else
			delete pObj;
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CxDlgPropertySheetSqlWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CxDlgPropertySheetSqlWizard)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()



void CxDlgPropertySheetSqlWizard::OnDestroy() 
{
	CPropertySheet::OnDestroy();
}

void SQLW_SelectObject (CuListCtrlCheckBox& listCtrl, CaDBObject* pObj)
{
	int i, nCount = listCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		CaDBObject* pItem = (CaDBObject*)listCtrl.GetItemData(i);
		if (pItem)
		{
			if (pObj->GetName().CompareNoCase (pItem->GetName()) == 0 && 
			    pObj->GetOwner().CompareNoCase (pItem->GetOwner()) == 0)
			{
				listCtrl.CheckItem (i, TRUE);
			}
		}
	}
}


BOOL CxDlgPropertySheetSqlWizard::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	if (m_pPointTopLeft)
	{
		SetWindowPos(NULL, m_pPointTopLeft->x, m_pPointTopLeft->y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	
	return bResult;
}

LRESULT CxDlgPropertySheetSqlWizard::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	//
	// The HELP Button is always disable. This is the reason why I overwrite this
	// function.
	// Due to the documentation, if PSH_HASHELP is set for the page (in property page,
	// the member m_psp.dwFlags |= PSH_HASHELP) then the help button is enable. But it
	// does not work.

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);
	
	return CPropertySheet::WindowProc(message, wParam, lParam);
}

void CxDlgPropertySheetSqlWizard::OnHelp()
{
	CTabCtrl* pTab = GetTabControl();
	ASSERT(pTab);
	if (pTab)
	{
		int nActivePage = pTab->GetCurSel();
		CPropertyPage* pPage = GetPage(nActivePage);
		if (pPage)
			pPage->SendMessage (WM_HELP);
	}
}
